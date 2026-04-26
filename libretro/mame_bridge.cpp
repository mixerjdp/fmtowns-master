#include "mame_bridge.h"

#include "emu.h"
#include "drivenum.h"
#include "emuopts.h"
#include "main.h"
#include "mconfig.h"
#include "fujitsu/fmtowns_capture.h"
#include "render.h"
#include "osd_libretro.h"
#include "osdepend.h"
#include "screen.h"
#include "ui/menuitem.h"
#include "ui/uimain.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace fmtowns::screen_capture {
} // namespace fmtowns::screen_capture

namespace {

class headless_font : public osd_font
{
public:
	bool open(std::string const &, std::string const &, int &) override { return false; }
	void close() override { }
	bool get_bitmap(char32_t, bitmap_argb32 &, std::int32_t &, std::int32_t &, std::int32_t &) override { return false; }
};

class headless_ui_manager : public ui_manager
{
public:
	using ui_manager::ui_manager;
};

struct audio_sink_info {
    bool is_left = false;
    bool is_right = false;
};

class headless_osd_interface : public osd_interface
{
public:
	void init(running_machine &machine) override { m_machine = &machine; }
	void update(bool) override { }
	void input_update(bool force) override;
	void check_osd_inputs() override { }
	void set_verbose(bool print_verbose) override { m_verbose = print_verbose; }

	void init_debugger() override { }
	void wait_for_debugger(device_t &, bool) override { }

	bool no_sound() override { return false; }
	bool sound_external_per_channel_volume() override { return false; }
	bool sound_split_streams_per_source() override { return false; }
	uint32_t sound_get_generation() override { return m_audio_generation; }
	osd::audio_info sound_get_information() override
	{
		osd::audio_info info = {};
		info.m_generation = m_audio_generation;
		info.m_default_sink = m_default_sink_id;
		info.m_default_source = 0;

		osd::audio_info::node_info sink = {};
		sink.m_id = m_default_sink_id;
		sink.m_name = "libretro";
		sink.m_display_name = "Libretro Audio";
		sink.m_rate = { 44100, 8000, 96000 };
		sink.m_sinks = 2;
		sink.m_sources = 0;
		sink.m_port_names = { "Left", "Right" };
		sink.m_port_positions = { osd::channel_position::FL(), osd::channel_position::FR() };
		info.m_nodes.emplace_back(std::move(sink));

		return info;
	}
	uint32_t sound_stream_sink_open(uint32_t channels, std::string name, uint32_t sample_rate) override 
    { 
        uint32_t id = ++m_next_stream_id;
        auto& info = m_sinks[id];
        info.is_left = (name.find("lspeaker") != std::string::npos || name.find("left") != std::string::npos);
        info.is_right = (name.find("rspeaker") != std::string::npos || name.find("right") != std::string::npos);
        if (!info.is_left && !info.is_right) {
            info.is_left = info.is_right = true; // Mono sink
        }
        return id;
    }
	uint32_t sound_stream_source_open(uint32_t, std::string, uint32_t) override { return ++m_next_stream_id; }
	void sound_stream_close(uint32_t) override { }
	void add_audio_to_recording(const int16_t *buffer, int samples_this_frame) override
	{
		// Recording hook sounds bad according to user, using sink updates instead.
	}

	void sound_stream_sink_update(uint32_t id, const int16_t *buffer, int samples_this_frame) override
	{
        auto it = m_sinks.find(id);
        if (it != m_sinks.end())
        {
		    fmtowns::libretro_osd::mix_audio(buffer, samples_this_frame, it->second.is_left, it->second.is_right);
        }
	}
	void sound_stream_source_update(uint32_t, int16_t *buffer, int samples_this_frame) override
	{
		if (buffer)
			std::fill(buffer, buffer + samples_this_frame, 0);
	}
	void sound_stream_set_volumes(uint32_t, const std::vector<float> &) override { }
	void sound_begin_update() override { }
    void sound_end_update() override { }
    void customize_input_type_list(std::vector<input_type_entry> &) override { }
	std::vector<ui::menu_item> get_slider_list() override { return {}; }

	osd_font::ptr font_alloc() override { return std::make_unique<headless_font>(); }
	bool get_font_families(std::string const &, std::vector<std::pair<std::string, std::string>> &) override { return false; }

	bool execute_command(const char *) override { return false; }

	std::unique_ptr<osd::midi_input_port> create_midi_input(std::string_view) override { return nullptr; }
	std::unique_ptr<osd::midi_output_port> create_midi_output(std::string_view) override { return nullptr; }
	std::vector<osd::midi_port_info> list_midi_ports() override { return {}; }

	std::unique_ptr<osd::network_device> open_network_device(int, osd::network_handler &) override { return nullptr; }
	std::vector<osd::network_device_info> list_network_devices() override { return {}; }

private:
	static constexpr uint32_t m_audio_generation = 1;
	static constexpr uint32_t m_default_sink_id = 1;
	bool m_verbose = false;
	uint32_t m_next_stream_id = 0;
	running_machine *m_machine = nullptr;
    std::map<uint32_t, audio_sink_info> m_sinks;
};

struct key_map_entry {
    const char *mame_name;
    unsigned retrok;
};

static const key_map_entry s_key_map[] = {
    { "A", RETROK_a }, { "B", RETROK_b }, { "C", RETROK_c }, { "D", RETROK_d }, { "E", RETROK_e },
    { "F", RETROK_f }, { "G", RETROK_g }, { "H", RETROK_h }, { "I", RETROK_i }, { "J", RETROK_j },
    { "K", RETROK_k }, { "L", RETROK_l }, { "M", RETROK_m }, { "N", RETROK_n }, { "O", RETROK_o },
    { "P", RETROK_p }, { "Q", RETROK_q }, { "R", RETROK_r }, { "S", RETROK_s }, { "T", RETROK_t },
    { "U", RETROK_u }, { "V", RETROK_v }, { "W", RETROK_w }, { "X", RETROK_x }, { "Y", RETROK_y },
    { "Z", RETROK_z },
    { "0", RETROK_0 }, { "1", RETROK_1 }, { "2", RETROK_2 }, { "3", RETROK_3 }, { "4", RETROK_4 },
    { "5", RETROK_5 }, { "6", RETROK_6 }, { "7", RETROK_7 }, { "8", RETROK_8 }, { "9", RETROK_9 },
    { "ESC", RETROK_ESCAPE }, { "RETURN", RETROK_RETURN }, { "Space", RETROK_SPACE },
    { "Tab", RETROK_TAB }, { "Backspace", RETROK_BACKSPACE }, { "Shift", RETROK_LSHIFT },
    { "Ctrl", RETROK_LCTRL }, { "ALT", RETROK_LALT }, { "CAP", RETROK_CAPSLOCK },
    { "Up", RETROK_UP }, { "Down", RETROK_DOWN }, { "Left", RETROK_LEFT }, { "Right", RETROK_RIGHT },
    { "HOME", RETROK_HOME }, { "Insert", RETROK_INSERT }, { "Delete", RETROK_DELETE },
    { "BREAK", RETROK_PAUSE }, { "PF1", RETROK_F1 }, { "PF2", RETROK_F2 }, { "PF3", RETROK_F3 },
    { "PF4", RETROK_F4 }, { "PF5", RETROK_F5 }, { "PF6", RETROK_F6 }, { "PF7", RETROK_F7 },
    { "PF8", RETROK_F8 }, { "PF9", RETROK_F9 }, { "PF10", RETROK_F10 }, { "PF11", RETROK_F11 },
    { "PF12", RETROK_F12 }, { "PF13", RETROK_F1 }, { "PF14", RETROK_F2 }, { "PF15", RETROK_F3 },
    { "PF16", RETROK_F4 }, { "PF17", RETROK_F5 }, { "PF18", RETROK_F6 }, { "PF19", RETROK_F7 },
    { "PF20", RETROK_F8 },
    { "Hiragana", RETROK_LSUPER }, { "Katakana", RETROK_RSUPER },
    { "Conversion", RETROK_LALT }, { "Non-conversion", RETROK_RALT },
    { "Kana Kanji", RETROK_RALT },
};

static bool is_key_match(const std::string& name, const char* label)
{
    if (name == label) return true;
    size_t len = strlen(label);
    // Smart match for decorated names like "1 ! ..."
    if (name.compare(0, len, label) == 0)
    {
        if (name.length() == len || name[len] == ' ') return true;
    }
    // Match Japanese labels with English in parentheses
    if (name.find(std::string("(") + label + ")") != std::string::npos) return true;
    return false;
}

void headless_osd_interface::input_update(bool force)
{
    if (!m_machine) return;

    fmtowns::libretro_osd::poll_input();

    bool shifted = fmtowns::libretro_osd::key_pressed(RETROK_RSHIFT) || fmtowns::libretro_osd::key_pressed(RETROK_LSHIFT);

    for (auto &port : m_machine->ioport().ports())
    {
        for (auto &field : port.second->fields())
        {
            if (field.type() == IPT_KEYBOARD)
            {
                std::string name = field.name();
                for (const auto &entry : s_key_map)
                {
                    if (is_key_match(name, entry.mame_name))
                    {
                        bool is_ext_pf = (name.find("PF13") != std::string::npos || name.find("PF14") != std::string::npos ||
                                          name.find("PF15") != std::string::npos || name.find("PF16") != std::string::npos ||
                                          name.find("PF17") != std::string::npos || name.find("PF18") != std::string::npos ||
                                          name.find("PF19") != std::string::npos || name.find("PF20") != std::string::npos);
                        
                        bool is_std_pf = !is_ext_pf && (name.find("PF1") != std::string::npos || name.find("PF2") != std::string::npos ||
                                                        name.find("PF3") != std::string::npos || name.find("PF4") != std::string::npos ||
                                                        name.find("PF5") != std::string::npos || name.find("PF6") != std::string::npos ||
                                                        name.find("PF7") != std::string::npos || name.find("PF8") != std::string::npos ||
                                                        name.find("PF9") != std::string::npos || name.find("PF10") != std::string::npos);

                        if (is_ext_pf) {
                            field.set_value(shifted && fmtowns::libretro_osd::key_pressed(entry.retrok));
                        } else if (is_std_pf) {
                            field.set_value(!shifted && fmtowns::libretro_osd::key_pressed(entry.retrok));
                        } else {
                            field.set_value(fmtowns::libretro_osd::key_pressed(entry.retrok));
                        }
                        break;
                    }
                }
            }
            else if (field.type() >= IPT_JOYSTICK_UP && field.type() <= IPT_JOYSTICK_RIGHT)
            {
                unsigned id = 0;
                switch (field.type()) {
                    case IPT_JOYSTICK_UP: id = RETRO_DEVICE_ID_JOYPAD_UP; break;
                    case IPT_JOYSTICK_DOWN: id = RETRO_DEVICE_ID_JOYPAD_DOWN; break;
                    case IPT_JOYSTICK_LEFT: id = RETRO_DEVICE_ID_JOYPAD_LEFT; break;
                    case IPT_JOYSTICK_RIGHT: id = RETRO_DEVICE_ID_JOYPAD_RIGHT; break;
                }
                field.set_value(fmtowns::libretro_osd::joypad_pressed(field.player(), id));
            }
            else if (field.type() >= IPT_BUTTON1 && field.type() <= IPT_BUTTON16)
            {
                unsigned id = 0;
                switch (field.type()) {
                    case IPT_BUTTON1: id = RETRO_DEVICE_ID_JOYPAD_B; break; // A
                    case IPT_BUTTON2: id = RETRO_DEVICE_ID_JOYPAD_A; break; // B
                    case IPT_BUTTON3: id = RETRO_DEVICE_ID_JOYPAD_Y; break; // Marty Zoom
                    case IPT_BUTTON4: id = RETRO_DEVICE_ID_JOYPAD_X; break;
                    case IPT_BUTTON5: id = RETRO_DEVICE_ID_JOYPAD_L; break;
                    case IPT_BUTTON6: id = RETRO_DEVICE_ID_JOYPAD_R; break;
                    case IPT_BUTTON7: id = RETRO_DEVICE_ID_JOYPAD_L2; break;
                    case IPT_BUTTON8: id = RETRO_DEVICE_ID_JOYPAD_R2; break;
                }
                if (id) field.set_value(fmtowns::libretro_osd::joypad_pressed(field.player(), id));
            }
            else if (field.type() == IPT_START)
            {
                field.set_value(fmtowns::libretro_osd::joypad_pressed(field.player(), RETRO_DEVICE_ID_JOYPAD_START));
            }
            else if (field.type() == IPT_SELECT)
            {
                field.set_value(fmtowns::libretro_osd::joypad_pressed(field.player(), RETRO_DEVICE_ID_JOYPAD_SELECT));
            }
        }
    }
}


class headless_machine_manager : public machine_manager
{
public:
	headless_machine_manager(emu_options &options, osd_interface &osd)
		: machine_manager(options, osd)
	{
	}

	ui_manager *create_ui(running_machine &machine) override
	{
		m_ui = std::make_unique<headless_ui_manager>(machine);
		return m_ui.get();
	}

	void ui_initialize(running_machine &) override { }
	void before_load_settings(running_machine &) override { }
	void create_custom(running_machine &) override { }
	void load_cheatfiles(running_machine &) override { }
	void update_machine() override { }
	void destroy_ui() { m_ui.reset(); }

private:
	std::unique_ptr<ui_manager> m_ui;
};

std::string join_path(const std::string &base, const char *leaf)
{
	if (base.empty())
		return leaf ? leaf : "";

	const char last = base[base.size() - 1];
	const bool has_separator = last == '/' || last == '\\';
	return base + (has_separator ? "" : "\\") + (leaf ? leaf : "");
}

void set_option(emu_options &options, const char *name, const std::string &value)
{
	options.set_value(name, value.c_str(), OPTION_PRIORITY_CMDLINE);
}

std::string lowercase_extension(const std::string &path)
{
	const std::string::size_type slash = path.find_last_of("/\\");
	const std::string::size_type dot = path.find_last_of('.');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
		return {};

	std::string extension = path.substr(dot + 1);
	for (char &ch : extension)
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	return extension;
}

const char *slot_for_extension(const std::string &extension)
{
	if (extension == "chd" || extension == "cue" || extension == "toc" ||
			extension == "nrg" || extension == "gdi" || extension == "iso" ||
			extension == "cdr")
		return "cdrom";

	if (extension == "mfi" || extension == "dfi" || extension == "mfm" ||
			extension == "td0" || extension == "imd" || extension == "86f" ||
			extension == "d77" || extension == "d88" || extension == "1dd" ||
			extension == "cqm" || extension == "cqi" || extension == "dsk" ||
			extension == "bin")
		return "floppydisk1";

	if (extension == "hd" || extension == "hdv" || extension == "2mg" ||
			extension == "hdi")
		return "harddisk1";

	return nullptr;
}

std::string parent_path(const std::string &path)
{
	const std::string::size_type slash = path.find_last_of("/\\");
	return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

std::string trim(std::string value)
{
	const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch); });
	const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch); }).base();
	if (first >= last)
		return {};
	return std::string(first, last);
}

std::string first_m3u_entry(const std::string &playlist)
{
	std::ifstream file(playlist);
	if (!file)
		return {};

	const std::string base = parent_path(playlist);
	std::string line;
	while (std::getline(file, line))
	{
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;

		if (line.find(':') != std::string::npos || (line.size() >= 2 && (line[0] == '\\' || line[0] == '/')))
			return line;
		return base.empty() ? line : join_path(base, line.c_str());
	}

	return {};
}

constexpr const char *k_canary_path = "C:\\sw\\fmtowns-master\\build\\libretro64\\boot_canary.log";

void append_canary_line(const char *line)
{
	if (!line)
		return;

	if (FILE *canary = std::fopen(k_canary_path, "ab"))
	{
		std::fputs(line, canary);
		std::fclose(canary);
	}
}

const char *phase_name(machine_phase phase)
{
	switch (phase)
	{
	case machine_phase::PREINIT: return "PREINIT";
	case machine_phase::INIT: return "INIT";
	case machine_phase::RESET: return "RESET";
	case machine_phase::RUNNING: return "RUNNING";
	case machine_phase::EXIT: return "EXIT";
	}
	return "UNKNOWN";
}

} // namespace

namespace fmtowns::mame_bridge {

class session::impl
{
public:
	bool start(const boot_config &config, std::string &error)
	{
		stop();
		append_canary_line("session_start_enter\n");

		const int driver_index = driver_list::find(config.model.c_str());
		if (driver_index < 0)
		{
			error = "Unknown FM Towns model: " + config.model;
			return false;
		}

		m_options = std::make_unique<emu_options>(emu_options::option_support::FULL);
		m_osd = std::make_unique<headless_osd_interface>();
		m_manager = std::make_unique<headless_machine_manager>(*m_options, *m_osd);

		m_options->set_system_name(std::string(config.model));
		m_options->set_value(OPTION_READCONFIG, false, OPTION_PRIORITY_CMDLINE);
		m_options->set_value(OPTION_WRITECONFIG, false, OPTION_PRIORITY_CMDLINE);
		m_options->set_value(OPTION_SKIP_GAMEINFO, true, OPTION_PRIORITY_CMDLINE);
		m_options->set_value(OPTION_UI_ACTIVE, false, OPTION_PRIORITY_CMDLINE);
		m_options->set_value(OPTION_THROTTLE, false, OPTION_PRIORITY_CMDLINE);
		m_options->set_value(OPTION_SLEEP, false, OPTION_PRIORITY_CMDLINE);
		m_options->set_value(OPTION_NVRAM_SAVE, true, OPTION_PRIORITY_CMDLINE);
		set_option(*m_options, OPTION_MEDIAPATH, config.bios_directory);
		set_option(*m_options, OPTION_SAMPLERATE, "44100");
		set_option(*m_options, OPTION_CFG_DIRECTORY, config.cfg_directory);

		// Support for Port 2 Pad/Mouse option
		if (config.port2_type == "pad")
		{
			set_option(*m_options, "pad2", "townspad");
		}
		else
		{
			set_option(*m_options, "pad2", "mouse");
		}

		set_option(*m_options, OPTION_NVRAM_DIRECTORY, config.nvram_directory);
		set_option(*m_options, OPTION_STATE_DIRECTORY, join_path(config.nvram_directory, "state"));
		set_option(*m_options, OPTION_SNAPSHOT_DIRECTORY, join_path(config.nvram_directory, "snap"));

		if (!config.content_path.empty())
		{
			std::string content_path = config.content_path;
			std::string extension = lowercase_extension(content_path);
			if (extension == "m3u")
			{
				content_path = first_m3u_entry(config.content_path);
				if (content_path.empty())
				{
					error = "M3U playlist is empty or unreadable: " + config.content_path;
					stop();
					return false;
				}
				extension = lowercase_extension(content_path);
			}

			const char *slot = slot_for_extension(extension);
			if (!slot)
			{
				error = "Unsupported direct content extension for v1: ." + extension;
				stop();
				return false;
			}

			if (!m_options->has_image_option(slot))
			{
				error = std::string("FM Towns model does not expose expected media slot: ") + slot;
				stop();
				return false;
			}

			m_options->image_option(slot).specify(content_path);
		}

		m_config = std::make_unique<machine_config>(driver_list::driver(driver_index), *m_options);
		m_machine = std::make_unique<running_machine>(*m_config, *m_manager);
		m_manager->set_machine(m_machine.get());

		append_canary_line("session_before_libretro_start\n");
		const int result = m_machine->libretro_start(true);
		if (result != EMU_ERR_NONE)
		{
			char buffer[96];
			std::snprintf(buffer, sizeof(buffer), "session_libretro_start_fail_%d\n", result);
			append_canary_line(buffer);
			error = "MAME machine start failed with code " + std::to_string(result);
			stop();
			return false;
		}

		append_canary_line("session_after_libretro_start_ok\n");
		for (screen_device &screen : screen_device_enumerator(m_machine->root_device()))
			screen.set_video_attributes(VIDEO_ALWAYS_UPDATE);

		// Boost MAME master gain to 2.0 (standard for many MAME cores to reach 0dB)
		m_machine->sound().set_master_gain(2.0f);

		m_running = true;
		return true;
	}

	void run_slice()
	{
		if (m_running && m_machine)
		{
            fmtowns::libretro_osd::audio_begin_frame();
			m_machine->libretro_run_slice();
            fmtowns::libretro_osd::audio_end_frame();
		}
	}

	void force_video_update()
	{
		if (m_running && m_machine)
		{
			for (screen_device &screen : screen_device_enumerator(m_machine->root_device()))
				screen.update_now();
		}
	}

	void emit_boot_snapshot(const char *stage)
	{
		if (m_running && m_machine)
			log_boot_snapshot(stage);
	}

	void emit_video_snapshot(const char *stage)
	{
		if (m_running && m_machine)
			log_video_snapshot(stage);
	}

	bool execution_snapshot(runtime_snapshot &snapshot) const
	{
		if (!m_running || !m_machine)
			return false;

		snapshot.machine_time_seconds = m_machine->time().as_double();
		snapshot.screen_frame = current_frame_number();
		snapshot.cpu_pc = first_cpu_pc();
		snapshot.phase = static_cast<unsigned>(m_machine->phase());
		return true;
	}

	void reset()
	{
		if (m_running && m_machine)
			m_machine->schedule_soft_reset();
	}

	void stop()
	{
		if (m_manager)
			m_manager->destroy_ui();

		// TODO: MAME teardown through this no-frontend route currently trips a
		// destructor-order crash. Leak the session objects on unload for now so
		// RetroArch can safely exit/switch while the bootstrap path matures.
		m_machine.release();
		m_config.release();
		m_manager.release();
		m_osd.release();
		m_options.release();
		m_running = false;
	}

	bool running() const
	{
		return m_running;
	}

	bool copy_video_frame(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height)
	{
		width = 0;
		height = 0;

		if (!m_running || !m_machine || !pixels)
			return false;

		for (screen_device &screen : screen_device_enumerator(m_machine->root_device()))
		{
			const rectangle &area = screen.visible_area();
			if (area.empty())
				continue;

			if (fmtowns::libretro_osd::copy_captured_xrgb8888(pixels, max_width, max_height, width, height))
				return true;

			const unsigned src_width = static_cast<unsigned>(area.width());
			const unsigned src_height = static_cast<unsigned>(area.height());
			width = max_width;
			height = max_height;
			if (!width || !height)
				return false;

			const size_t sample_count = static_cast<size_t>(src_width) * static_cast<size_t>(src_height);
			if (m_video_buffer.size() < sample_count)
				m_video_buffer.resize(sample_count);

			screen.pixels(m_video_buffer.data());
			for (unsigned y = 0; y < height; ++y)
			{
				uint32_t *dst = pixels + (static_cast<size_t>(y) * max_width);
				const unsigned src_y = std::min<unsigned>(src_height - 1, (static_cast<unsigned long long>(y) * src_height) / height);
				const uint32_t *src_row = m_video_buffer.data() + (static_cast<size_t>(src_y) * src_width);
				for (unsigned x = 0; x < width; ++x)
				{
					const unsigned src_x = std::min<unsigned>(src_width - 1, (static_cast<unsigned long long>(x) * src_width) / width);
					dst[x] = src_row[src_x];
				}
			}

			return true;
		}

		return false;
	}

private:
	std::string first_cpu_snapshot() const
	{
		if (!m_machine)
			return "cpu=<none>";

		for (device_t &device : device_enumerator(m_machine->root_device()))
		{
			cpu_device *cpu = nullptr;
			if (!device.interface(cpu) || cpu == nullptr)
				continue;

			char buffer[128];
			std::snprintf(buffer, sizeof(buffer), "cpu=%s pc=%08llx",
					device.tag(),
					static_cast<unsigned long long>(cpu->pcbase()));
			return buffer;
		}

		return "cpu=<none>";
	}

	uint64_t first_cpu_pc() const
	{
		if (!m_machine)
			return 0;

		for (device_t &device : device_enumerator(m_machine->root_device()))
		{
			cpu_device *cpu = nullptr;
			if (!device.interface(cpu) || cpu == nullptr)
				continue;

			return static_cast<uint64_t>(cpu->pcbase());
		}

		return 0;
	}

	void log_boot_snapshot(const char *stage) const
	{
		if (!m_machine || !stage)
			return;

		const std::string cpu_snapshot = first_cpu_snapshot();
		fmtowns::libretro_osd::log(RETRO_LOG_INFO,
				"BOOT %s phase=%s time=%.6f frame=%llu %s\n",
				stage,
				phase_name(m_machine->phase()),
				m_machine->time().as_double(),
				static_cast<unsigned long long>(current_frame_number()),
				cpu_snapshot.c_str());
	}

	void log_video_snapshot(const char *stage) const
	{
		if (!m_machine || !stage)
			return;

		for (screen_device &screen : screen_device_enumerator(m_machine->root_device()))
		{
			const rectangle &area = screen.visible_area();
			const uint32_t top_left = area.empty() ? 0 : screen.pixel(area.min_x, area.min_y);
			const uint32_t center = area.empty() ? 0 : screen.pixel((area.min_x + area.max_x) / 2, (area.min_y + area.max_y) / 2);
			const bool live = m_machine->render().is_live(screen);
			fmtowns::libretro_osd::log(RETRO_LOG_INFO,
					"VIDEO %s phase=%s time=%.6f frame=%llu vpos=%d hpos=%d partials=%d live=%s vis=%dx%d top_left=%08x center=%08x\n",
					stage,
					phase_name(m_machine->phase()),
					m_machine->time().as_double(),
					static_cast<unsigned long long>(screen.frame_number()),
					screen.vpos(),
					screen.hpos(),
					screen.partial_updates(),
					live ? "yes" : "no",
					area.width(),
					area.height(),
					top_left,
					center);
			break;
		}
	}

	uint64_t current_frame_number() const
	{
		for (screen_device &screen : screen_device_enumerator(m_machine->root_device()))
			return screen.frame_number();
		return 0;
	}

	std::unique_ptr<emu_options> m_options;
	std::unique_ptr<headless_osd_interface> m_osd;
	std::unique_ptr<headless_machine_manager> m_manager;
	std::unique_ptr<machine_config> m_config;
	std::unique_ptr<running_machine> m_machine;
	std::vector<uint32_t> m_video_buffer;
	bool m_running = false;
};

session::session()
	: m_impl(std::make_unique<impl>())
{
}

session::~session() = default;

bool session::start(const boot_config &config, std::string &error)
{
	return m_impl->start(config, error);
}

void session::run_slice()
{
	m_impl->run_slice();
}

void session::reset()
{
	m_impl->reset();
}

void session::stop()
{
	m_impl->stop();
}

bool session::running() const
{
	return m_impl->running();
}

void session::log_boot_snapshot(const char *stage)
{
	m_impl->emit_boot_snapshot(stage);
}

void session::log_video_snapshot(const char *stage)
{
	m_impl->emit_video_snapshot(stage);
}

void session::force_video_update()
{
	m_impl->force_video_update();
}

bool session::execution_snapshot(runtime_snapshot &snapshot) const
{
	return m_impl->execution_snapshot(snapshot);
}

bool session::copy_video_frame(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height)
{
	return m_impl->copy_video_frame(pixels, max_width, max_height, width, height);
}

} // namespace fmtowns::mame_bridge

const char *emulator_info::get_appname() { return "MAME"; }
const char *emulator_info::get_appname_lower() { return "mame"; }
const char *emulator_info::get_configname() { return "mame"; }
const char *emulator_info::get_copyright() { return "Copyright MAMEdev and contributors"; }
const char *emulator_info::get_copyright_info() { return "Copyright MAMEdev and contributors"; }
const char *emulator_info::get_bare_build_version() { return "fmtowns-libretro"; }
const char *emulator_info::get_build_version() { return "fmtowns-libretro"; }
void emulator_info::display_ui_chooser(running_machine &) { }
int emulator_info::start_frontend(emu_options &, osd_interface &, std::vector<std::string> &) { return EMU_ERR_INVALID_CONFIG; }
int emulator_info::start_frontend(emu_options &, osd_interface &, int, char *[]) { return EMU_ERR_INVALID_CONFIG; }
bool emulator_info::draw_user_interface(running_machine &) { return false; }
void emulator_info::periodic_check() { }
bool emulator_info::frame_hook() { return false; }
void emulator_info::sound_hook(const std::map<std::string, std::vector<std::pair<const float *, int>>> &) { }
void emulator_info::layout_script_cb(layout_file &, const char *) { }
bool emulator_info::standalone() { return false; }
