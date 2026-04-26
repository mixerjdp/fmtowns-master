#include "libretro.h"
#include "fujitsu/fmtowns_capture.h"
#include "mame_bridge.h"
#include "osd_libretro.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define RETRO_API_EXPORT extern "C" __declspec(dllexport)
#else
#define RETRO_API_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace {

constexpr unsigned k_width = 640;
constexpr unsigned k_height = 480;
constexpr double k_fps = 60.0;
constexpr double k_sample_rate = 44100.0;

std::string g_system_directory;
std::string g_bios_directory;
std::string g_model = "fmtownsux";
std::string g_content_path;
std::array<uint32_t, k_width * k_height> g_framebuffer = {};

constexpr const char *k_screen_trace_path = "C:\\sw\\fmtowns-master\\build\\libretro64\\fmtowns_screen_update.log";
constexpr const char *k_scheduler_trace_path = "C:\\sw\\fmtowns-master\\build\\libretro64\\scheduler_trace.log";
constexpr const char *k_canary_path = "C:\\sw\\fmtowns-master\\build\\libretro64\\boot_canary.log";
constexpr const char *k_watchdog_path = "C:\\sw\\fmtowns-master\\build\\libretro64\\retro_watchdog.log";

enum class run_stage : unsigned
{
	idle,
	retro_run,
	run_frame,
	slice,
	video_update,
	render,
	placeholder,
	audio,
	done
};

std::atomic<uint64_t> g_last_run_tick_ns{0};
std::atomic<uint64_t> g_last_run_frame{0};
std::atomic<uint64_t> g_last_completed_frame{0};
std::atomic<uint64_t> g_last_machine_time_us{0};
std::atomic<uint64_t> g_last_machine_screen_frame{0};
std::atomic<uint64_t> g_last_machine_cpu_pc{0};
std::atomic<unsigned> g_last_machine_phase{0};
std::atomic<unsigned> g_run_stage{static_cast<unsigned>(run_stage::idle)};
std::atomic<unsigned> g_run_pass{0};
std::atomic<unsigned> g_run_slice{0};
std::atomic<bool> g_runtime_loaded{false};
std::atomic<bool> g_watchdog_stop{false};
std::atomic<bool> g_watchdog_running{false};
std::thread g_watchdog_thread;

const char *run_stage_name(unsigned stage)
{
	switch (static_cast<run_stage>(stage))
	{
	case run_stage::idle: return "idle";
	case run_stage::retro_run: return "retro_run";
	case run_stage::run_frame: return "run_frame";
	case run_stage::slice: return "slice";
	case run_stage::video_update: return "video_update";
	case run_stage::render: return "render";
	case run_stage::placeholder: return "placeholder";
	case run_stage::audio: return "audio";
	case run_stage::done: return "done";
	}
	return "unknown";
}

void clear_trace_log()
{
	if (FILE *trace = std::fopen(k_screen_trace_path, "wb"))
		std::fclose(trace);
	if (FILE *trace = std::fopen(k_scheduler_trace_path, "wb"))
		std::fclose(trace);
	if (FILE *trace = std::fopen(k_canary_path, "wb"))
		std::fclose(trace);
	if (FILE *trace = std::fopen(k_watchdog_path, "wb"))
		std::fclose(trace);
}

void append_canary_line(const char *line)
{
	if (!line)
		return;

	if (FILE *trace = std::fopen(k_canary_path, "ab"))
	{
		std::fputs(line, trace);
		std::fclose(trace);
	}
}

void append_watchdog_line(const char *line)
{
	if (!line)
		return;

	if (FILE *trace = std::fopen(k_watchdog_path, "ab"))
	{
		std::fputs(line, trace);
		std::fclose(trace);
	}
}

void start_watchdog()
{
	bool expected = false;
	if (!g_watchdog_running.compare_exchange_strong(expected, true))
		return;

	g_watchdog_stop.store(false);
	g_watchdog_thread = std::thread([]()
	{
		uint64_t last_reported_tick = 0;
		for (;;)
		{
			for (unsigned i = 0; i < 5; ++i)
			{
				if (g_watchdog_stop.load())
					return;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}

			const uint64_t current_tick = g_last_run_tick_ns.load();
			const uint64_t current_frame = g_last_run_frame.load();
			const uint64_t completed_frame = g_last_completed_frame.load();
			const uint64_t machine_time_us = g_last_machine_time_us.load();
			const uint64_t machine_screen_frame = g_last_machine_screen_frame.load();
			const uint64_t machine_cpu_pc = g_last_machine_cpu_pc.load();
			const unsigned machine_phase = g_last_machine_phase.load();
			const unsigned stage = g_run_stage.load();
			const unsigned pass = g_run_pass.load();
			const unsigned slice = g_run_slice.load();
			if (current_tick == 0)
			{
				append_watchdog_line("watchdog waiting_for_retro_run\n");
				continue;
			}

			if (current_tick == last_reported_tick)
			{
				char line[256];
				std::snprintf(line, sizeof(line),
						"watchdog stalled frame=%llu completed=%llu stage=%s pass=%u slice=%u emu_us=%llu screen=%llu pc=%08llx phase=%u loaded=%s\n",
						static_cast<unsigned long long>(current_frame),
						static_cast<unsigned long long>(completed_frame),
						run_stage_name(stage),
						pass,
						slice,
						static_cast<unsigned long long>(machine_time_us),
						static_cast<unsigned long long>(machine_screen_frame),
						static_cast<unsigned long long>(machine_cpu_pc),
						machine_phase,
						g_runtime_loaded.load() ? "yes" : "no");
				append_watchdog_line(line);
			}
			else
			{
				char line[256];
				std::snprintf(line, sizeof(line),
						"watchdog alive frame=%llu completed=%llu stage=%s pass=%u slice=%u emu_us=%llu screen=%llu pc=%08llx phase=%u loaded=%s\n",
						static_cast<unsigned long long>(current_frame),
						static_cast<unsigned long long>(completed_frame),
						run_stage_name(stage),
						pass,
						slice,
						static_cast<unsigned long long>(machine_time_us),
						static_cast<unsigned long long>(machine_screen_frame),
						static_cast<unsigned long long>(machine_cpu_pc),
						machine_phase,
						g_runtime_loaded.load() ? "yes" : "no");
				append_watchdog_line(line);
				last_reported_tick = current_tick;
			}
		}
	});
}

void stop_watchdog()
{
	if (!g_watchdog_running.exchange(false))
		return;

	g_watchdog_stop.store(true);
	if (g_watchdog_thread.joinable())
		g_watchdog_thread.join();
}

const retro_variable k_variables[] = {
	{ "fmtowns_model", "FM Towns model; fmtownsux|fmtmarty|fmtownssj|fmtowns|fmtownsv03|fmtownshr|fmtownsmx|fmtownsftv|fmtmarty2|carmarty" },
	{ "fmtowns_pad1", "Port 1 device; gamepad|none" },
	{ "fmtowns_pad2", "Port 2 device; none|gamepad" },
	{ "fmtowns_mouse", "Mouse; enabled|disabled" },
	{ nullptr, nullptr }
};

struct bios_file
{
	const char *name;
	size_t expected_size;
};

const bios_file k_fmtownsux_bios[] = {
	{ "fmt_dos_a.rom", 0x80000 },
	{ "fmt_dic.rom", 0x80000 },
	{ "fmt_fnt.rom", 0x40000 },
	{ "fmt_sys_a.rom", 0x40000 },
	{ "mytownsux.rom", 0x20 },
};

enum class runtime_state
{
	stopped,
	loaded,
	running,
	exiting
};

std::string join_path(const std::string &base, const char *leaf);
void append_canary_line(const char *line);

class phase3_runtime
{
public:
	bool load(std::string model, std::string content, std::string bios_directory, bool bios_ready)
	{
		unload();

		m_model = std::move(model);
		m_content = std::move(content);
		m_bios_directory = std::move(bios_directory);
		m_bios_ready = bios_ready;
		m_state = runtime_state::loaded;
		m_frame = 0;
		m_reset_pending = false;
		m_run_canary_issued = false;
		m_mame = std::make_unique<fmtowns::mame_bridge::session>();

		std::error_code fs_error;
		const std::string cfg_directory = join_path(m_bios_directory, "cfg");
		const std::string nvram_directory = join_path(m_bios_directory, "nvram");
		std::filesystem::create_directories(cfg_directory, fs_error);
		std::filesystem::create_directories(nvram_directory, fs_error);

		fmtowns::libretro_osd::log(RETRO_LOG_INFO,
				"Phase 3 runtime loaded: model=%s, content=%s, bios=%s, bios_ready=%s.\n",
				m_model.c_str(),
				m_content.empty() ? "<none>" : m_content.c_str(),
				m_bios_directory.empty() ? "<unset>" : m_bios_directory.c_str(),
				m_bios_ready ? "yes" : "no");

		fmtowns::mame_bridge::boot_config boot;
		boot.model = m_model;
		boot.bios_directory = m_bios_directory;
		boot.content_path = m_content;
		boot.cfg_directory = cfg_directory;
		boot.nvram_directory = nvram_directory;

		std::string error;
		if (!m_mame->start(boot, error))
		{
			fmtowns::libretro_osd::log(RETRO_LOG_ERROR, "MAME libretro bootstrap failed: %s\n", error.c_str());
			m_mame.reset();
			m_state = runtime_state::stopped;
			g_runtime_loaded.store(false);
			return false;
		}

		fmtowns::libretro_osd::log(RETRO_LOG_INFO, "MAME libretro bootstrap is running in-process.\n");
		g_runtime_loaded.store(true);
		return true;
	}

	void reset()
	{
		if (m_state == runtime_state::stopped)
			return;

		m_reset_pending = true;
	}

	void run_frame()
	{
		if (m_state == runtime_state::stopped)
			return;

		g_run_stage.store(static_cast<unsigned>(run_stage::run_frame));
		g_run_pass.store(0);
		g_run_slice.store(0);

		if (m_state == runtime_state::loaded)
			m_state = runtime_state::running;

		if (m_reset_pending)
		{
			m_reset_pending = false;
			m_frame = 0;
			clear_framebuffer();
			fmtowns::libretro_osd::log(RETRO_LOG_INFO, "Phase 3 runtime reset applied.\n");
			if (m_mame)
				m_mame->reset();
		}

		bool rendered = false;
		if (m_mame && m_mame->running())
		{
			advance_mame_frame();
			update_execution_snapshot();
			g_run_stage.store(static_cast<unsigned>(run_stage::render));
			rendered = render_mame_frame();
		}
		if (!rendered)
		{
			g_run_stage.store(static_cast<unsigned>(run_stage::placeholder));
			render_placeholder_frame();
		}

		g_run_stage.store(static_cast<unsigned>(run_stage::audio));
		push_audio_frame();

		++m_frame;
		g_last_completed_frame.store(m_frame);
		g_run_stage.store(static_cast<unsigned>(run_stage::done));
	}

	void unload()
	{
		if (m_state == runtime_state::stopped)
			return;

		fmtowns::libretro_osd::log(RETRO_LOG_INFO, "Phase 3 runtime unloaded after %llu frames.\n",
				static_cast<unsigned long long>(m_frame));
		if (m_mame)
			m_mame->stop();
		m_mame.reset();
		m_state = runtime_state::stopped;
		m_frame = 0;
		g_last_completed_frame.store(0);
		g_last_machine_time_us.store(0);
		g_last_machine_screen_frame.store(0);
		g_last_machine_cpu_pc.store(0);
		g_last_machine_phase.store(0);
		g_run_stage.store(static_cast<unsigned>(run_stage::idle));
		g_run_pass.store(0);
		g_run_slice.store(0);
		m_reset_pending = false;
		m_run_canary_issued = false;
		g_runtime_loaded.store(false);
	}

	bool loaded() const
	{
		return m_state != runtime_state::stopped;
	}

	bool has_issued_run_canary() const
	{
		return m_run_canary_issued;
	}

	uint64_t frame_count() const
	{
		return m_frame;
	}

	bool is_running() const
	{
		return m_state == runtime_state::running;
	}

	void mark_run_canary_issued()
	{
		m_run_canary_issued = true;
	}

private:
	void update_execution_snapshot()
	{
		if (!m_mame)
			return;

		fmtowns::mame_bridge::runtime_snapshot snapshot;
		if (!m_mame->execution_snapshot(snapshot))
			return;

		const double seconds = snapshot.machine_time_seconds < 0.0 ? 0.0 : snapshot.machine_time_seconds;
		g_last_machine_time_us.store(static_cast<uint64_t>(seconds * 1000000.0));
		g_last_machine_screen_frame.store(snapshot.screen_frame);
		g_last_machine_cpu_pc.store(snapshot.cpu_pc);
		g_last_machine_phase.store(snapshot.phase);
	}

	void advance_mame_frame()
	{
		if (!m_mame)
			return;

		fmtowns::mame_bridge::runtime_snapshot snapshot;
		if (!m_mame->execution_snapshot(snapshot))
			return;

		const double start_seconds = snapshot.machine_time_seconds < 0.0 ? 0.0 : snapshot.machine_time_seconds;
		const double target_seconds = start_seconds + (1.0 / k_fps);
		const uint64_t start_screen_frame = snapshot.screen_frame;
		const auto wall_start = std::chrono::steady_clock::now();
		const auto wall_budget = std::chrono::milliseconds(m_have_real_frame ? 16 : 45);
		const unsigned max_slices = m_have_real_frame ? 24000u : 64000u;

		for (unsigned slice = 0; slice < max_slices; ++slice)
		{
			g_run_stage.store(static_cast<unsigned>(run_stage::slice));
			g_run_pass.store(slice / 1024u);
			g_run_slice.store(slice);
			m_mame->run_slice();

			if ((slice & 0x7fu) != 0x7fu)
				continue;

			if (!m_mame->execution_snapshot(snapshot))
				break;

			const double current_seconds = snapshot.machine_time_seconds < 0.0 ? 0.0 : snapshot.machine_time_seconds;
			g_last_machine_time_us.store(static_cast<uint64_t>(current_seconds * 1000000.0));
			g_last_machine_screen_frame.store(snapshot.screen_frame);
			g_last_machine_cpu_pc.store(snapshot.cpu_pc);
			g_last_machine_phase.store(snapshot.phase);

			if (current_seconds >= target_seconds || snapshot.screen_frame != start_screen_frame)
				break;

			if (std::chrono::steady_clock::now() - wall_start >= wall_budget)
				break;
		}
	}

	void clear_framebuffer()
	{
		g_framebuffer.fill(0);
	}

	bool render_mame_frame()
	{
		unsigned width = 0;
		unsigned height = 0;
		if (m_mame->copy_video_frame(g_framebuffer.data(), k_width, k_height, width, height))
		{
			fmtowns::libretro_osd::present_xrgb8888(g_framebuffer.data(), width, height, k_width * sizeof(uint32_t));
			const uint32_t first = g_framebuffer[0];
			const uint32_t center = g_framebuffer[(static_cast<size_t>(height / 2) * k_width) + (width / 2)];
			const uint32_t last = g_framebuffer[(static_cast<size_t>(height - 1) * k_width) + (width - 1)];
			unsigned nonzero_samples = 0;
			for (unsigned y = 0; y < height; y += 32)
			{
				for (unsigned x = 0; x < width; x += 32)
				{
					if (g_framebuffer[(static_cast<size_t>(y) * k_width) + x] != 0)
						++nonzero_samples;
				}
			}
			const bool interesting_frame = (!m_have_real_frame && nonzero_samples > 0) || (m_frame < 2);
			if (nonzero_samples > 0)
				m_have_real_frame = true;
			if (interesting_frame)
			{
				fmtowns::libretro_osd::log(RETRO_LOG_INFO,
						"Frame sample %llu: %ux%u first=%08x center=%08x last=%08x nonzero_samples=%u.\n",
						static_cast<unsigned long long>(m_frame),
						width,
						height,
						first,
						center,
						last,
						nonzero_samples);
			}
			return true;
		}
		return false;
	}

	void render_placeholder_frame()
	{
		clear_framebuffer();

		// A tiny moving scan marker confirms retro_run() is advancing without
		// depending on any MAME UI or native window renderer.
		const unsigned x = static_cast<unsigned>((m_frame / 2) % k_width);
		for (unsigned y = 0; y < k_height; ++y)
			g_framebuffer[(y * k_width) + x] = 0x00181818;

		fmtowns::libretro_osd::present_xrgb8888(g_framebuffer.data(), k_width, k_height, k_width * sizeof(uint32_t));
	}

	void push_audio_frame()
	{
		fmtowns::libretro_osd::push_silence(k_sample_rate, k_fps);
	}

	std::string m_model;
	std::string m_content;
	std::string m_bios_directory;
	std::unique_ptr<fmtowns::mame_bridge::session> m_mame;
	runtime_state m_state = runtime_state::stopped;
	uint64_t m_frame = 0;
	bool m_bios_ready = false;
	bool m_reset_pending = false;
	bool m_have_real_frame = false;
	bool m_run_canary_issued = false;
};

phase3_runtime g_runtime;

bool file_exists(const std::string &path)
{
	FILE *file = std::fopen(path.c_str(), "rb");
	if (!file)
		return false;
	std::fclose(file);
	return true;
}

std::string join_path(const std::string &base, const char *leaf)
{
	if (base.empty())
		return leaf ? leaf : "";

	const char last = base[base.size() - 1];
	const bool has_separator = last == '/' || last == '\\';
	return base + (has_separator ? "" : "\\") + (leaf ? leaf : "");
}

void refresh_environment_paths()
{
	g_system_directory.clear();
	g_bios_directory.clear();

	g_system_directory = fmtowns::libretro_osd::system_directory();

	if (!g_system_directory.empty())
	{
		const std::string fmtowns_dir = join_path(g_system_directory, "fmtowns");
		g_bios_directory = file_exists(join_path(fmtowns_dir, "fmt_dos.rom")) ||
				file_exists(join_path(fmtowns_dir, "fmt_dos_a.rom"))
			? fmtowns_dir
			: g_system_directory;
	}
}

void set_core_options()
{
	fmtowns::libretro_osd::configure_environment(k_variables);
}

void refresh_core_options()
{
	g_model = fmtowns::libretro_osd::variable_value("fmtowns_model", "fmtownsux");
	const std::string pad1 = fmtowns::libretro_osd::variable_value("fmtowns_pad1", "gamepad");
	const std::string pad2 = fmtowns::libretro_osd::variable_value("fmtowns_pad2", "none");
	const std::string mouse = fmtowns::libretro_osd::variable_value("fmtowns_mouse", "enabled");
	fmtowns::libretro_osd::log(RETRO_LOG_INFO, "Input profile: pad1=%s, pad2=%s, mouse=%s.\n",
			pad1.c_str(), pad2.c_str(), mouse.c_str());
}

bool validate_default_bios()
{
	if (g_bios_directory.empty())
	{
		fmtowns::libretro_osd::log(RETRO_LOG_WARN, "No RetroArch system directory reported; FM Towns BIOS lookup is not ready.\n");
		return false;
	}

	bool ok = true;
	for (const bios_file &bios : k_fmtownsux_bios)
	{
		const std::string path = join_path(g_bios_directory, bios.name);
		if (!file_exists(path))
		{
			ok = false;
			fmtowns::libretro_osd::log(RETRO_LOG_WARN, "Missing FM Towns BIOS file for %s: %s\n", g_model.c_str(), path.c_str());
		}
	}

	if (ok)
		fmtowns::libretro_osd::log(RETRO_LOG_INFO, "FM Towns BIOS directory ready: %s\n", g_bios_directory.c_str());

	return ok;
}

} // namespace

RETRO_API_EXPORT void retro_set_environment(retro_environment_t cb)
{
	fmtowns::libretro_osd::set_environment(cb);
	set_core_options();
	refresh_environment_paths();
}

RETRO_API_EXPORT void retro_set_video_refresh(retro_video_refresh_t cb)
{
	fmtowns::libretro_osd::set_video_refresh(cb);
}

RETRO_API_EXPORT void retro_set_audio_sample(retro_audio_sample_t cb)
{
	fmtowns::libretro_osd::set_audio_sample(cb);
}

RETRO_API_EXPORT void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	fmtowns::libretro_osd::set_audio_sample_batch(cb);
}

RETRO_API_EXPORT void retro_set_input_poll(retro_input_poll_t cb)
{
	fmtowns::libretro_osd::set_input_poll(cb);
}

RETRO_API_EXPORT void retro_set_input_state(retro_input_state_t cb)
{
	fmtowns::libretro_osd::set_input_state(cb);
}

RETRO_API_EXPORT void retro_set_controller_port_device(unsigned, unsigned)
{
}

RETRO_API_EXPORT unsigned retro_api_version(void)
{
	return RETRO_API_VERSION;
}

RETRO_API_EXPORT void retro_get_system_info(retro_system_info *info)
{
	if (!info)
		return;

	std::memset(info, 0, sizeof(*info));
	info->library_name = "FM Towns (MAME)";
	info->library_version = "0.0-mame-bridge";
	info->valid_extensions = "chd|cue|toc|nrg|gdi|iso|cdr|mfi|dfi|mfm|td0|imd|86f|d77|d88|1dd|cqm|cqi|dsk|bin|hd|hdv|2mg|hdi|m3u";
	info->need_fullpath = true;
	info->block_extract = true;
}

RETRO_API_EXPORT void retro_get_system_av_info(retro_system_av_info *info)
{
	if (!info)
		return;

	std::memset(info, 0, sizeof(*info));
	info->geometry.base_width = k_width;
	info->geometry.base_height = k_height;
	info->geometry.max_width = k_width;
	info->geometry.max_height = k_height;
	info->geometry.aspect_ratio = 4.0f / 3.0f;
	info->timing.fps = k_fps;
	info->timing.sample_rate = k_sample_rate;
}

RETRO_API_EXPORT void retro_init(void)
{
	refresh_environment_paths();
	set_core_options();
	clear_trace_log();
	start_watchdog();
	fmtowns::screen_capture::set_callback(fmtowns::libretro_osd::capture_xrgb8888);
	append_canary_line("retro_init\n");
	fmtowns::libretro_osd::log(RETRO_LOG_INFO, "fmtowns_libretro Phase 3 initialized; libretro OSD adapters are active.\n");
}

RETRO_API_EXPORT void retro_deinit(void)
{
	fmtowns::screen_capture::set_callback(nullptr);
	g_runtime.unload();
	stop_watchdog();
}

RETRO_API_EXPORT void retro_reset(void)
{
	g_runtime.reset();
}

RETRO_API_EXPORT bool retro_load_game(const retro_game_info *game)
{
	refresh_environment_paths();
	refresh_core_options();
	g_content_path = game && game->path ? game->path : "";
	const bool bios_ready = validate_default_bios();
	append_canary_line("retro_load_game\n");
	fmtowns::libretro_osd::log(RETRO_LOG_INFO, "fmtowns_libretro MAME bootstrap: model=%s, content=%s, bios=%s.\n",
			g_model.c_str(),
			g_content_path.empty() ? "<none>" : g_content_path.c_str(),
			g_bios_directory.empty() ? "<unset>" : g_bios_directory.c_str());
	return g_runtime.load(g_model, g_content_path, g_bios_directory, bios_ready);
}

RETRO_API_EXPORT void retro_unload_game(void)
{
	g_runtime.unload();
}

RETRO_API_EXPORT unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

RETRO_API_EXPORT void retro_run(void)
{
	const auto now = std::chrono::steady_clock::now();
	g_last_run_tick_ns.store(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count()));
	g_run_stage.store(static_cast<unsigned>(run_stage::retro_run));

	fmtowns::libretro_osd::poll_input();
	const bool runtime_loaded = g_runtime_loaded.load();

	if (runtime_loaded && !g_runtime.has_issued_run_canary())
	{
		append_canary_line("retro_run\n");
		g_runtime.mark_run_canary_issued();
	}

	if (runtime_loaded)
	{
		g_last_run_frame.store(g_runtime.frame_count());
	}

	if (runtime_loaded)
		g_runtime.run_frame();
	else
	{
		g_framebuffer.fill(0);
		fmtowns::libretro_osd::present_xrgb8888(g_framebuffer.data(), k_width, k_height, k_width * sizeof(uint32_t));
	}
}

RETRO_API_EXPORT size_t retro_serialize_size(void)
{
	return 0;
}

RETRO_API_EXPORT bool retro_serialize(void *, size_t)
{
	return false;
}

RETRO_API_EXPORT bool retro_unserialize(const void *, size_t)
{
	return false;
}

RETRO_API_EXPORT void retro_cheat_reset(void)
{
}

RETRO_API_EXPORT void retro_cheat_set(unsigned, bool, const char *)
{
}

RETRO_API_EXPORT bool retro_load_game_special(unsigned, const retro_game_info *, size_t)
{
	return false;
}

RETRO_API_EXPORT void *retro_get_memory_data(unsigned)
{
	return nullptr;
}

RETRO_API_EXPORT size_t retro_get_memory_size(unsigned)
{
	return 0;
}
