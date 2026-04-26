#ifndef FMTOWNS_OSD_LIBRETRO_H
#define FMTOWNS_OSD_LIBRETRO_H

#include "libretro.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fmtowns::libretro_osd {

void set_environment(retro_environment_t cb);
void set_video_refresh(retro_video_refresh_t cb);
void set_audio_sample(retro_audio_sample_t cb);
void set_audio_sample_batch(retro_audio_sample_batch_t cb);
void set_input_poll(retro_input_poll_t cb);
void set_input_state(retro_input_state_t cb);

void configure_environment(const retro_variable *variables);
void poll_input();
bool joypad_pressed(unsigned port, unsigned id);

std::string system_directory();
std::string variable_value(const char *key, const char *fallback);

void capture_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch);
bool copy_captured_xrgb8888(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height);

void present_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch);
void push_silence(double sample_rate, double fps);
void push_interleaved_audio(const int16_t *samples, std::size_t frames);
void push_mame_audio(const float *left, const float *right, std::size_t frames);

void log(enum retro_log_level level, const char *fmt, ...);

} // namespace fmtowns::libretro_osd

#endif
