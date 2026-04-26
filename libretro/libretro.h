// Minimal libretro ABI declarations used by the Phase 0 FM Towns core stub.
// Keep this header intentionally small until the real MAME bridge needs more.

#ifndef FMTOWNS_LIBRETRO_H
#define FMTOWNS_LIBRETRO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RETRO_API_VERSION 1
#define RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY 9
#define RETRO_ENVIRONMENT_SET_PIXEL_FORMAT 10
#define RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME 18
#define RETRO_ENVIRONMENT_SET_VARIABLES 16
#define RETRO_ENVIRONMENT_GET_VARIABLE 15
#define RETRO_ENVIRONMENT_GET_LOG_INTERFACE 27
#define RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK 31

#define RETRO_DEVICE_JOYPAD 1
#define RETRO_DEVICE_KEYBOARD 3
#define RETRO_DEVICE_MOUSE 2

#define RETRO_REGION_NTSC 0

enum retro_pixel_format
{
	RETRO_PIXEL_FORMAT_0RGB1555 = 0,
	RETRO_PIXEL_FORMAT_XRGB8888 = 1,
	RETRO_PIXEL_FORMAT_RGB565 = 2
};

enum retro_log_level
{
	RETRO_LOG_DEBUG = 0,
	RETRO_LOG_INFO,
	RETRO_LOG_WARN,
	RETRO_LOG_ERROR
};

typedef bool (*retro_environment_t)(unsigned cmd, void *data);
typedef void (*retro_video_refresh_t)(const void *data, unsigned width, unsigned height, size_t pitch);
typedef void (*retro_audio_sample_t)(int16_t left, int16_t right);
typedef size_t (*retro_audio_sample_batch_t)(const int16_t *data, size_t frames);
typedef void (*retro_input_poll_t)(void);
typedef int16_t (*retro_input_state_t)(unsigned port, unsigned device, unsigned index, unsigned id);
typedef void (*retro_log_printf_t)(enum retro_log_level level, const char *fmt, ...);

typedef void (*retro_keyboard_event_t)(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers);

struct retro_keyboard_callback
{
	retro_keyboard_event_t callback;
};
typedef struct retro_keyboard_callback retro_keyboard_callback_t;

struct retro_log_callback
{
	retro_log_printf_t log;
};

struct retro_variable
{
	const char *key;
	const char *value;
};

struct retro_game_geometry
{
	unsigned base_width;
	unsigned base_height;
	unsigned max_width;
	unsigned max_height;
	float aspect_ratio;
};

struct retro_system_timing
{
	double fps;
	double sample_rate;
};

struct retro_system_av_info
{
	struct retro_game_geometry geometry;
	struct retro_system_timing timing;
};

struct retro_system_info
{
	const char *library_name;
	const char *library_version;
	const char *valid_extensions;
	bool need_fullpath;
	bool block_extract;
};

struct retro_game_info
{
	const char *path;
	const void *data;
	size_t size;
	const char *meta;
};

#define RETRO_DEVICE_ID_JOYPAD_B        0
#define RETRO_DEVICE_ID_JOYPAD_Y        1
#define RETRO_DEVICE_ID_JOYPAD_SELECT   2
#define RETRO_DEVICE_ID_JOYPAD_START    3
#define RETRO_DEVICE_ID_JOYPAD_UP       4
#define RETRO_DEVICE_ID_JOYPAD_DOWN     5
#define RETRO_DEVICE_ID_JOYPAD_LEFT     6
#define RETRO_DEVICE_ID_JOYPAD_RIGHT    7
#define RETRO_DEVICE_ID_JOYPAD_A        8
#define RETRO_DEVICE_ID_JOYPAD_X        9
#define RETRO_DEVICE_ID_JOYPAD_L        10
#define RETRO_DEVICE_ID_JOYPAD_R        11
#define RETRO_DEVICE_ID_JOYPAD_L2       12
#define RETRO_DEVICE_ID_JOYPAD_R2       13
#define RETRO_DEVICE_ID_JOYPAD_L3       14
#define RETRO_DEVICE_ID_JOYPAD_R3       15

enum retro_key
{
	RETROK_UNKNOWN = 0,
	RETROK_BACKSPACE = 8,
	RETROK_TAB = 9,
	RETROK_CLEAR = 12,
	RETROK_RETURN = 13,
	RETROK_PAUSE = 19,
	RETROK_ESCAPE = 27,
	RETROK_SPACE = 32,
	RETROK_EXCLAIM = 33,
	RETROK_QUOTEDBL = 34,
	RETROK_HASH = 35,
	RETROK_DOLLAR = 36,
	RETROK_AMPERSAND = 38,
	RETROK_QUOTE = 39,
	RETROK_LEFTPAREN = 40,
	RETROK_RIGHTPAREN = 41,
	RETROK_ASTERISK = 42,
	RETROK_PLUS = 43,
	RETROK_COMMA = 44,
	RETROK_MINUS = 45,
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
	RETROK_COLON = 58,
	RETROK_SEMICOLON = 59,
	RETROK_LESS = 60,
	RETROK_EQUALS = 61,
	RETROK_GREATER = 62,
	RETROK_QUESTION = 63,
	RETROK_AT = 64,
	RETROK_LEFTBRACKET = 91,
	RETROK_BACKSLASH = 92,
	RETROK_RIGHTBRACKET = 93,
	RETROK_CARET = 94,
	RETROK_UNDERSCORE = 95,
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
	RETROK_LEFTBRACE = 123,
	RETROK_BAR = 124,
	RETROK_RIGHTBRACE = 125,
	RETROK_TILDE = 126,
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
	RETROK_F13 = 294,
	RETROK_F14 = 295,
	RETROK_F15 = 296,

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
	RETROK_LSUPER = 311,
	RETROK_RSUPER = 312,
	RETROK_MODE = 313,
	RETROK_COMPOSE = 314,

	RETROK_HELP = 315,
	RETROK_PRINT = 316,
	RETROK_SYSREQ = 317,
	RETROK_BREAK = 318,
	RETROK_MENU = 319,
	RETROK_POWER = 320,
	RETROK_EURO = 321,
	RETROK_UNDO = 322,

	RETROK_LAST
};

#ifdef __cplusplus
}
#endif

#endif
