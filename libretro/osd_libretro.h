#ifndef FMTOWNS_OSD_LIBRETRO_H
#define FMTOWNS_OSD_LIBRETRO_H

#include "libretro.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace fmtowns::libretro_osd {

void set_environment(retro_environment_t cb);
void set_video_refresh(retro_video_refresh_t cb);
void set_audio_sample(retro_audio_sample_t cb);
void set_audio_sample_batch(retro_audio_sample_batch_t cb);
void set_input_poll(retro_input_poll_t cb);
void set_input_state(retro_input_state_t cb);
void set_keyboard_callback(const retro_keyboard_callback_t *cb);

void configure_environment(const retro_variable *variables);
void poll_input();
bool joypad_pressed(unsigned port, unsigned id);
bool key_pressed(unsigned id);

std::string system_directory();
std::string variable_value(const char *key, const char *fallback);

void capture_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch);
bool copy_captured_xrgb8888(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height);

void present_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch);
void push_silence(double sample_rate, double fps);
void push_audio(const int16_t *buffer, int samples);
void mix_audio(const int16_t *buffer, int samples, bool left, bool right);
void audio_begin_frame();
void audio_end_frame();
void reset_audio();
void flush_audio_frame();

void log(enum retro_log_level level, const char *fmt, ...);

} // namespace fmtowns::libretro_osd

#endif
