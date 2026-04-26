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
#include "speaker.h"
#include "screen.h"
#include "ui/menuitem.h"
#include "ui/uimain.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "inputdev.h"

namespace fmtowns::screen_capture {
} // namespace fmtowns::screen_capture

namespace {

enum retro_key : unsigned
{
	RETROK_BACKSPACE = 8,
	RETROK_TAB = 9,
	RETROK_CLEAR = 12,
	RETROK_RETURN = 13,
	RETROK_PAUSE = 19,
	RETROK_ESCAPE = 27,
	RETROK_SPACE = 32,
	RETROK_MINUS = 45,
	RETROK_COMMA = 44,
	RETROK_PERIOD = 46,
	RETROK_SLASH = 47,
	RETROK_0 = 48,
	RETROK_1 = 49,
	RETROK_2 = 50,
	RETROK_3 = 51,
	RETROK_4 = 52,
	RETROK_5 = 53,
	RETROK_6 = 54,
	RETROK_7 = 55,
	RETROK_8 = 56,
	RETROK_9 = 57,
	RETROK_SEMICOLON = 59,
	RETROK_QUOTE = 39,
	RETROK_EQUALS = 61,
	RETROK_LEFTBRACKET = 91,
	RETROK_BACKSLASH = 92,
	RETROK_RIGHTBRACKET = 93,
	RETROK_BACKQUOTE = 96,
	RETROK_a = 97,
	RETROK_b = 98,
	RETROK_c = 99,
	RETROK_d = 100,
	RETROK_e = 101,
	RETROK_f = 102,
	RETROK_g = 103,
	RETROK_h = 104,
	RETROK_i = 105,
	RETROK_j = 106,
	RETROK_k = 107,
	RETROK_l = 108,
	RETROK_m = 109,
	RETROK_n = 110,
	RETROK_o = 111,
	RETROK_p = 112,
	RETROK_q = 113,
	RETROK_r = 114,
	RETROK_s = 115,
	RETROK_t = 116,
	RETROK_u = 117,
	RETROK_v = 118,
	RETROK_w = 119,
	RETROK_x = 120,
	RETROK_y = 121,
	RETROK_z = 122,
	RETROK_DELETE = 127,
	RETROK_KP0 = 256,
	RETROK_KP1 = 257,
	RETROK_KP2 = 258,
	RETROK_KP3 = 259,
	RETROK_KP4 = 260,
	RETROK_KP5 = 261,
	RETROK_KP6 = 262,
	RETROK_KP7 = 263,
	RETROK_KP8 = 264,
	RETROK_KP9 = 265,
	RETROK_KP_PERIOD = 266,
	RETROK_KP_DIVIDE = 267,
	RETROK_KP_MULTIPLY = 268,
	RETROK_KP_MINUS = 269,
	RETROK_KP_PLUS = 270,
	RETROK_KP_ENTER = 271,
	RETROK_KP_EQUALS = 272,
	RETROK_UP = 273,
	RETROK_DOWN = 274,
	RETROK_RIGHT = 275,
	RETROK_LEFT = 276,
	RETROK_INSERT = 277,
	RETROK_HOME = 278,
	RETROK_END = 279,
	RETROK_PAGEUP = 280,
	RETROK_PAGEDOWN = 281,
	RETROK_F1 = 282,
	RETROK_F2 = 283,
	RETROK_F3 = 284,
	RETROK_F4 = 285,
	RETROK_F5 = 286,
	RETROK_F6 = 287,
	RETROK_F7 = 288,
	RETROK_F8 = 289,
	RETROK_F9 = 290,
	RETROK_F10 = 291,
	RETROK_F11 = 292,
	RETROK_F12 = 293,
	RETROK_NUMLOCK = 300,
	RETROK_CAPSLOCK = 301,
	RETROK_SCROLLOCK = 302,
	RETROK_RSHIFT = 303,
	RETROK_LSHIFT = 304,
	RETROK_RCTRL = 305,
	RETROK_LCTRL = 306,
	RETROK_RALT = 307,
	RETROK_LALT = 308,
	RETROK_RMETA = 309,
	RETROK_LMETA = 310,
	RETROK_MENU = 319,
	RETROK_PRINT = 316,
	RETROK_BREAK = 318,
	RETROK_OEM_102 = 323,
};

enum retro_joypad_id : unsigned
{
	RETRO_DEVICE_ID_JOYPAD_B = 0,
	RETRO_DEVICE_ID_JOYPAD_Y = 1,
	RETRO_DEVICE_ID_JOYPAD_SELECT = 2,
	RETRO_DEVICE_ID_JOYPAD_START = 3,
	RETRO_DEVICE_ID_JOYPAD_UP = 4,
	RETRO_DEVICE_ID_JOYPAD_DOWN = 5,
	RETRO_DEVICE_ID_JOYPAD_LEFT = 6,
	RETRO_DEVICE_ID_JOYPAD_RIGHT = 7,
	RETRO_DEVICE_ID_JOYPAD_A = 8,
	RETRO_DEVICE_ID_JOYPAD_X = 9,
	RETRO_DEVICE_ID_JOYPAD_L = 10,
	RETRO_DEVICE_ID_JOYPAD_R = 11,
	RETRO_DEVICE_ID_JOYPAD_L2 = 12,
	RETRO_DEVICE_ID_JOYPAD_R2 = 13,
	RETRO_DEVICE_ID_JOYPAD_L3 = 14,
	RETRO_DEVICE_ID_JOYPAD_R3 = 15,
};

struct keyboard_item_mapping
{
	input_item_id item_id;
	retro_key key;
	const char *name;
};

enum class pad_item_kind
{
	axis_x,
	axis_y,
	up,
	down,
	left,
	right,
	button1,
	button2,
	button3,
	button4,
	button5,
	button6,
	start,
	select
};

struct pad_item_mapping
{
	input_item_id item_id;
	pad_item_kind kind;
	const char *name;
};

struct pad_device_state
{
	unsigned port = 0;
};

constexpr std::array<keyboard_item_mapping, 112> k_keyboard_items = {{
	{ ITEM_ID_A, RETROK_a, "A" },
	{ ITEM_ID_B, RETROK_b, "B" },
	{ ITEM_ID_C, RETROK_c, "C" },
	{ ITEM_ID_D, RETROK_d, "D" },
	{ ITEM_ID_E, RETROK_e, "E" },
	{ ITEM_ID_F, RETROK_f, "F" },
	{ ITEM_ID_G, RETROK_g, "G" },
	{ ITEM_ID_H, RETROK_h, "H" },
	{ ITEM_ID_I, RETROK_i, "I" },
	{ ITEM_ID_J, RETROK_j, "J" },
	{ ITEM_ID_K, RETROK_k, "K" },
	{ ITEM_ID_L, RETROK_l, "L" },
	{ ITEM_ID_M, RETROK_m, "M" },
	{ ITEM_ID_N, RETROK_n, "N" },
	{ ITEM_ID_O, RETROK_o, "O" },
	{ ITEM_ID_P, RETROK_p, "P" },
	{ ITEM_ID_Q, RETROK_q, "Q" },
	{ ITEM_ID_R, RETROK_r, "R" },
	{ ITEM_ID_S, RETROK_s, "S" },
	{ ITEM_ID_T, RETROK_t, "T" },
	{ ITEM_ID_U, RETROK_u, "U" },
	{ ITEM_ID_V, RETROK_v, "V" },
	{ ITEM_ID_W, RETROK_w, "W" },
	{ ITEM_ID_X, RETROK_x, "X" },
	{ ITEM_ID_Y, RETROK_y, "Y" },
	{ ITEM_ID_Z, RETROK_z, "Z" },
	{ ITEM_ID_0, RETROK_0, "0" },
	{ ITEM_ID_1, RETROK_1, "1" },
	{ ITEM_ID_2, RETROK_2, "2" },
	{ ITEM_ID_3, RETROK_3, "3" },
	{ ITEM_ID_4, RETROK_4, "4" },
	{ ITEM_ID_5, RETROK_5, "5" },
	{ ITEM_ID_6, RETROK_6, "6" },
	{ ITEM_ID_7, RETROK_7, "7" },
	{ ITEM_ID_8, RETROK_8, "8" },
	{ ITEM_ID_9, RETROK_9, "9" },
	{ ITEM_ID_F1, RETROK_F1, "F1" },
	{ ITEM_ID_F2, RETROK_F2, "F2" },
	{ ITEM_ID_F3, RETROK_F3, "F3" },
	{ ITEM_ID_F4, RETROK_F4, "F4" },
	{ ITEM_ID_F5, RETROK_F5, "F5" },
	{ ITEM_ID_F6, RETROK_F6, "F6" },
	{ ITEM_ID_F7, RETROK_F7, "F7" },
	{ ITEM_ID_F8, RETROK_F8, "F8" },
	{ ITEM_ID_F9, RETROK_F9, "F9" },
	{ ITEM_ID_F10, RETROK_F10, "F10" },
	{ ITEM_ID_F11, RETROK_F11, "F11" },
	{ ITEM_ID_F12, RETROK_F12, "F12" },
	{ ITEM_ID_ESC, RETROK_ESCAPE, "Esc" },
	{ ITEM_ID_TILDE, RETROK_BACKQUOTE, "`/~" },
	{ ITEM_ID_MINUS, RETROK_MINUS, "-/_" },
	{ ITEM_ID_EQUALS, RETROK_EQUALS, "=/+" },
	{ ITEM_ID_BACKSPACE, RETROK_BACKSPACE, "Backspace" },
	{ ITEM_ID_TAB, RETROK_TAB, "Tab" },
	{ ITEM_ID_OPENBRACE, RETROK_LEFTBRACKET, "[/{" },
	{ ITEM_ID_CLOSEBRACE, RETROK_RIGHTBRACKET, "]/}" },
	{ ITEM_ID_ENTER, RETROK_RETURN, "Enter" },
	{ ITEM_ID_COLON, RETROK_SEMICOLON, ";/:" },
	{ ITEM_ID_QUOTE, RETROK_QUOTE, "'\"" },
	{ ITEM_ID_BACKSLASH, RETROK_BACKSLASH, "\\/|" },
	{ ITEM_ID_BACKSLASH2, RETROK_OEM_102, "Backslash2" },
	{ ITEM_ID_COMMA, RETROK_COMMA, ",/<" },
	{ ITEM_ID_STOP, RETROK_PERIOD, "./>" },
	{ ITEM_ID_SLASH, RETROK_SLASH, "?/" },
	{ ITEM_ID_SPACE, RETROK_SPACE, "Space" },
	{ ITEM_ID_INSERT, RETROK_INSERT, "Insert" },
	{ ITEM_ID_DEL, RETROK_DELETE, "Delete" },
	{ ITEM_ID_HOME, RETROK_HOME, "Home" },
	{ ITEM_ID_END, RETROK_END, "End" },
	{ ITEM_ID_PGUP, RETROK_PAGEUP, "Page Up" },
	{ ITEM_ID_PGDN, RETROK_PAGEDOWN, "Page Down" },
	{ ITEM_ID_LEFT, RETROK_LEFT, "Left" },
	{ ITEM_ID_RIGHT, RETROK_RIGHT, "Right" },
	{ ITEM_ID_UP, RETROK_UP, "Up" },
	{ ITEM_ID_DOWN, RETROK_DOWN, "Down" },
	{ ITEM_ID_0_PAD, RETROK_KP0, "Keypad 0" },
	{ ITEM_ID_1_PAD, RETROK_KP1, "Keypad 1" },
	{ ITEM_ID_2_PAD, RETROK_KP2, "Keypad 2" },
	{ ITEM_ID_3_PAD, RETROK_KP3, "Keypad 3" },
	{ ITEM_ID_4_PAD, RETROK_KP4, "Keypad 4" },
	{ ITEM_ID_5_PAD, RETROK_KP5, "Keypad 5" },
	{ ITEM_ID_6_PAD, RETROK_KP6, "Keypad 6" },
	{ ITEM_ID_7_PAD, RETROK_KP7, "Keypad 7" },
	{ ITEM_ID_8_PAD, RETROK_KP8, "Keypad 8" },
	{ ITEM_ID_9_PAD, RETROK_KP9, "Keypad 9" },
	{ ITEM_ID_SLASH_PAD, RETROK_KP_DIVIDE, "Keypad /" },
	{ ITEM_ID_ASTERISK, RETROK_KP_MULTIPLY, "Keypad *" },
	{ ITEM_ID_MINUS_PAD, RETROK_KP_MINUS, "Keypad -" },
	{ ITEM_ID_PLUS_PAD, RETROK_KP_PLUS, "Keypad +" },
	{ ITEM_ID_DEL_PAD, RETROK_KP_PERIOD, "Keypad ." },
	{ ITEM_ID_ENTER_PAD, RETROK_KP_ENTER, "Keypad Enter" },
	{ ITEM_ID_BS_PAD, RETROK_BACKSPACE, "Keypad Backspace" },
	{ ITEM_ID_TAB_PAD, RETROK_TAB, "Keypad Tab" },
	{ ITEM_ID_00_PAD, RETROK_0, "Keypad 00" },
	{ ITEM_ID_000_PAD, RETROK_0, "Keypad 000" },
	{ ITEM_ID_COMMA_PAD, RETROK_COMMA, "Keypad ," },
	{ ITEM_ID_EQUALS_PAD, RETROK_KP_EQUALS, "Keypad =" },
	{ ITEM_ID_PRTSCR, RETROK_PRINT, "Print Screen" },
	{ ITEM_ID_PAUSE, RETROK_PAUSE, "Pause" },
	{ ITEM_ID_LSHIFT, RETROK_LSHIFT, "Left Shift" },
	{ ITEM_ID_RSHIFT, RETROK_RSHIFT, "Right Shift" },
	{ ITEM_ID_LCONTROL, RETROK_LCTRL, "Left Ctrl" },
	{ ITEM_ID_RCONTROL, RETROK_RCTRL, "Right Ctrl" },
	{ ITEM_ID_LALT, RETROK_LALT, "Left Alt" },
	{ ITEM_ID_RALT, RETROK_RALT, "Right Alt" },
	{ ITEM_ID_SCRLOCK, RETROK_SCROLLOCK, "Scroll Lock" },
	{ ITEM_ID_NUMLOCK, RETROK_NUMLOCK, "Num Lock" },
	{ ITEM_ID_CAPSLOCK, RETROK_CAPSLOCK, "Caps Lock" },
	{ ITEM_ID_LWIN, RETROK_LMETA, "Left Win" },
	{ ITEM_ID_RWIN, RETROK_RMETA, "Right Win" },
	{ ITEM_ID_MENU, RETROK_MENU, "Menu" },
	{ ITEM_ID_CANCEL, RETROK_BREAK, "Cancel" }
}};

constexpr std::array<pad_item_mapping, 14> k_pad_items = {{
	{ ITEM_ID_XAXIS, pad_item_kind::axis_x, "X Axis" },
	{ ITEM_ID_YAXIS, pad_item_kind::axis_y, "Y Axis" },
	{ ITEM_ID_UP, pad_item_kind::up, "Up" },
	{ ITEM_ID_DOWN, pad_item_kind::down, "Down" },
	{ ITEM_ID_LEFT, pad_item_kind::left, "Left" },
	{ ITEM_ID_RIGHT, pad_item_kind::right, "Right" },
	{ ITEM_ID_BUTTON1, pad_item_kind::button1, "A" },
	{ ITEM_ID_BUTTON2, pad_item_kind::button2, "B" },
	{ ITEM_ID_BUTTON3, pad_item_kind::button3, "X/C" },
	{ ITEM_ID_BUTTON4, pad_item_kind::button4, "Y/X" },
	{ ITEM_ID_BUTTON5, pad_item_kind::button5, "L/Y" },
	{ ITEM_ID_BUTTON6, pad_item_kind::button6, "R/Z" },
	{ ITEM_ID_START, pad_item_kind::start, "Start" },
	{ ITEM_ID_SELECT, pad_item_kind::select, "Select" }
}};

constexpr s32 k_axis_min = osd::input_device::ABSOLUTE_MIN;
constexpr s32 k_axis_max = osd::input_device::ABSOLUTE_MAX;

s32 keyboard_item_state(void *, void *item_internal)
{
	const auto *mapping = static_cast<const keyboard_item_mapping *>(item_internal);
	return mapping && fmtowns::libretro_osd::keyboard_pressed(mapping->key) ? 1 : 0;
}

s32 pad_item_state(void *device_internal, void *item_internal)
{
	const auto *device = static_cast<const pad_device_state *>(device_internal);
	const auto *mapping = static_cast<const pad_item_mapping *>(item_internal);
	if (!device || !mapping)
		return 0;

	auto pressed = [port = device->port](unsigned id) { return fmtowns::libretro_osd::joypad_pressed(port, id); };

	switch (mapping->kind)
	{
	case pad_item_kind::axis_x:
		if (pressed(RETRO_DEVICE_ID_JOYPAD_LEFT) == pressed(RETRO_DEVICE_ID_JOYPAD_RIGHT))
			return 0;
		return pressed(RETRO_DEVICE_ID_JOYPAD_LEFT) ? k_axis_min : k_axis_max;

	case pad_item_kind::axis_y:
		if (pressed(RETRO_DEVICE_ID_JOYPAD_UP) == pressed(RETRO_DEVICE_ID_JOYPAD_DOWN))
			return 0;
		return pressed(RETRO_DEVICE_ID_JOYPAD_UP) ? k_axis_min : k_axis_max;

	case pad_item_kind::up:
		return pressed(RETRO_DEVICE_ID_JOYPAD_UP) ? 1 : 0;
	case pad_item_kind::down:
		return pressed(RETRO_DEVICE_ID_JOYPAD_DOWN) ? 1 : 0;
	case pad_item_kind::left:
		return pressed(RETRO_DEVICE_ID_JOYPAD_LEFT) ? 1 : 0;
	case pad_item_kind::right:
		return pressed(RETRO_DEVICE_ID_JOYPAD_RIGHT) ? 1 : 0;
	case pad_item_kind::button1:
		return pressed(RETRO_DEVICE_ID_JOYPAD_A) ? 1 : 0;
	case pad_item_kind::button2:
		return pressed(RETRO_DEVICE_ID_JOYPAD_B) ? 1 : 0;
	case pad_item_kind::button3:
		return pressed(RETRO_DEVICE_ID_JOYPAD_X) ? 1 : 0;
	case pad_item_kind::button4:
		return pressed(RETRO_DEVICE_ID_JOYPAD_Y) ? 1 : 0;
	case pad_item_kind::button5:
		return pressed(RETRO_DEVICE_ID_JOYPAD_L) ? 1 : 0;
	case pad_item_kind::button6:
		return pressed(RETRO_DEVICE_ID_JOYPAD_R) ? 1 : 0;
	case pad_item_kind::start:
		return pressed(RETRO_DEVICE_ID_JOYPAD_START) ? 1 : 0;
	case pad_item_kind::select:
		return pressed(RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;
	}

	return 0;
}

void register_libretro_input_devices(running_machine &machine)
{
	auto &input = machine.input();
	static pad_device_state pad_state[2] = {{ 0 }, { 1 }};

	auto &keyboard = input.add_device(DEVICE_CLASS_KEYBOARD, "Libretro Keyboard", "libretro_keyboard", nullptr);
	for (const auto &mapping : k_keyboard_items)
		keyboard.add_item(mapping.name, mapping.name, mapping.item_id, keyboard_item_state, const_cast<keyboard_item_mapping *>(&mapping));

	auto &pad1 = input.add_device(DEVICE_CLASS_JOYSTICK, "Libretro Pad 1", "libretro_pad1", &pad_state[0]);
	auto &pad2 = input.add_device(DEVICE_CLASS_JOYSTICK, "Libretro Pad 2", "libretro_pad2", &pad_state[1]);
	for (const auto &mapping : k_pad_items)
	{
		pad1.add_item(mapping.name, mapping.name, mapping.item_id, pad_item_state, const_cast<pad_item_mapping *>(&mapping));
		pad2.add_item(mapping.name, mapping.name, mapping.item_id, pad_item_state, const_cast<pad_item_mapping *>(&mapping));
	}
}

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

class headless_osd_interface : public osd_interface
{
public:
	void init(running_machine &machine) override { register_libretro_input_devices(machine); }
	void update(bool) override { }
	void input_update(bool) override { }
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
	uint32_t sound_stream_sink_open(uint32_t, std::string, uint32_t) override { return ++m_next_stream_id; }
	uint32_t sound_stream_source_open(uint32_t, std::string, uint32_t) override { return ++m_next_stream_id; }
	void sound_stream_close(uint32_t) override { }
	void sound_stream_sink_update(uint32_t, const int16_t *, int) override { }
	void sound_stream_source_update(uint32_t, int16_t *buffer, int samples_this_frame) override
	{
		if (buffer)
			std::fill(buffer, buffer + samples_this_frame, 0);
	}
	void sound_stream_set_volumes(uint32_t, const std::vector<float> &) override { }
	void sound_begin_update() override { }
	void sound_end_update() override { }

	void customize_input_type_list(std::vector<input_type_entry> &) override { }
	void add_audio_to_recording(const int16_t *buffer, int samples_this_frame) override
	{
		if (!buffer || samples_this_frame <= 0)
			return;

		fmtowns::libretro_osd::push_mame_audio_samples(buffer, static_cast<std::size_t>(samples_this_frame));
	}
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
};

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
		set_option(*m_options, OPTION_CFG_DIRECTORY, config.cfg_directory);
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
		unsigned audio_channels = 0;
		for (speaker_device &speaker : speaker_device_enumerator(m_machine->root_device()))
			audio_channels += speaker.inputs();
		fmtowns::libretro_osd::set_mame_audio_channels(audio_channels);
		for (screen_device &screen : screen_device_enumerator(m_machine->root_device()))
			screen.set_video_attributes(VIDEO_ALWAYS_UPDATE);
		m_running = true;
		return true;
	}

	void run_slice()
	{
		if (m_running && m_machine)
		{
			m_machine->libretro_run_slice();
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
void emulator_info::sound_hook(const std::map<std::string, std::vector<std::pair<const float *, int>>> &sound)
{
	(void)sound;
}
void emulator_info::layout_script_cb(layout_file &, const char *) { }
bool emulator_info::standalone() { return false; }
