#include "libretro.h"
#include "osd_libretro.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
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
		m_reset_pending = true;

		fmtowns::libretro_osd::log(RETRO_LOG_INFO,
				"Phase 3 runtime loaded: model=%s, content=%s, bios=%s, bios_ready=%s.\n",
				m_model.c_str(),
				m_content.empty() ? "<none>" : m_content.c_str(),
				m_bios_directory.empty() ? "<unset>" : m_bios_directory.c_str(),
				m_bios_ready ? "yes" : "no");
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

		if (m_state == runtime_state::loaded)
			m_state = runtime_state::running;

		if (m_reset_pending)
		{
			m_reset_pending = false;
			m_frame = 0;
			clear_framebuffer();
			fmtowns::libretro_osd::log(RETRO_LOG_INFO, "Phase 3 runtime reset applied.\n");
		}

		// This is the non-blocking seam where the MAME OSD bridge will call the
		// scheduler one frame/slice at a time after running_machine exists.
		render_placeholder_frame();
		push_audio_frame();
		++m_frame;
	}

	void unload()
	{
		if (m_state == runtime_state::stopped)
			return;

		fmtowns::libretro_osd::log(RETRO_LOG_INFO, "Phase 3 runtime unloaded after %llu frames.\n",
				static_cast<unsigned long long>(m_frame));
		m_state = runtime_state::stopped;
		m_frame = 0;
		m_reset_pending = false;
	}

	bool loaded() const
	{
		return m_state != runtime_state::stopped;
	}

private:
	void clear_framebuffer()
	{
		g_framebuffer.fill(0);
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
	runtime_state m_state = runtime_state::stopped;
	uint64_t m_frame = 0;
	bool m_bios_ready = false;
	bool m_reset_pending = false;
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
	info->library_version = "0.0-phase3";
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
	fmtowns::libretro_osd::log(RETRO_LOG_INFO, "fmtowns_libretro Phase 3 initialized; libretro OSD adapters are active.\n");
}

RETRO_API_EXPORT void retro_deinit(void)
{
	g_runtime.unload();
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
	fmtowns::libretro_osd::log(RETRO_LOG_INFO, "fmtowns_libretro Phase 3 bootstrap placeholder: model=%s, content=%s, bios=%s.\n",
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
	fmtowns::libretro_osd::poll_input();

	if (g_runtime.loaded())
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
