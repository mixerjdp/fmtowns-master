#ifndef FMTOWNS_OSD_LIBRETRO_H
#define FMTOWNS_OSD_LIBRETRO_H

#include "libretro.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#define RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS 31

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

struct retro_input_descriptor
{
	unsigned port;
	unsigned device;
	unsigned index;
	unsigned id;
	const char *description;
};

namespace fmtowns::libretro_osd {

void set_environment(retro_environment_t cb);
void set_video_refresh(retro_video_refresh_t cb);
void set_audio_sample(retro_audio_sample_t cb);
void set_audio_sample_batch(retro_audio_sample_batch_t cb);
void set_input_poll(retro_input_poll_t cb);
void set_input_state(retro_input_state_t cb);
void set_input_descriptors(const struct retro_input_descriptor *desc);

void configure_environment(const retro_variable *variables);
void poll_input();
bool joypad_pressed(unsigned port, unsigned id);
int16_t joypad_analog(unsigned port, unsigned index, unsigned id);
bool keyboard_pressed(unsigned key);

std::string system_directory();
std::string variable_value(const char *key, const char *fallback);

void capture_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch);
bool copy_captured_xrgb8888(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height);

void present_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch);
void push_silence(double sample_rate, double fps);
void set_mame_audio_channels(unsigned channels);
unsigned mame_audio_channels();
void push_mame_audio_samples(const int16_t *samples, std::size_t frames);
void push_mame_audio(const float *left, const float *right, std::size_t frames);

void log(enum retro_log_level level, const char *fmt, ...);

} // namespace fmtowns::libretro_osd

#endif
