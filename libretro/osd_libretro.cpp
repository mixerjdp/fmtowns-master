#include "osd_libretro.h"

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

namespace fmtowns::libretro_osd {
namespace {

constexpr unsigned k_audio_channels = 2;
constexpr std::size_t k_max_audio_frames = 4096;

retro_environment_t g_environment = nullptr;
retro_video_refresh_t g_video = nullptr;
retro_audio_sample_t g_audio = nullptr;
retro_audio_sample_batch_t g_audio_batch = nullptr;
retro_input_poll_t g_input_poll = nullptr;
retro_input_state_t g_input_state = nullptr;
retro_log_printf_t g_log = nullptr;

std::array<int16_t, k_audio_channels * k_max_audio_frames> g_silence = {};

// Ring buffer for audio synchronization
std::vector<int16_t> g_audio_ring_buffer;
std::size_t g_rb_read_index = 0;
std::size_t g_rb_write_index = 0;
std::size_t g_rb_count = 0;
constexpr std::size_t k_rb_size = 32768; // Large enough for ~370ms of 44.1kHz stereo audio

double g_audio_remainder = 0.0;

std::vector<uint32_t> g_captured_frame;
unsigned g_captured_width = 0;
unsigned g_captured_height = 0;

// Static buffer for RetroArch batch to avoid per-frame allocations
std::vector<int16_t> g_flush_buffer;

bool g_audio_warmed_up = false;

} // namespace

void set_environment(retro_environment_t cb)
{
	g_environment = cb;
	g_log = nullptr;

	if (g_environment)
	{
		retro_log_callback logging = {};
		if (g_environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
			g_log = logging.log;
	}
}

void set_video_refresh(retro_video_refresh_t cb)
{
	g_video = cb;
}

void set_audio_sample(retro_audio_sample_t cb)
{
	g_audio = cb;
}

void set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	g_audio_batch = cb;
}

void set_input_poll(retro_input_poll_t cb)
{
	g_input_poll = cb;
}

void set_input_state(retro_input_state_t cb)
{
	g_input_state = cb;
}

void set_keyboard_callback(const retro_keyboard_callback_t *cb)
{
	if (g_environment && cb)
		g_environment(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, const_cast<retro_keyboard_callback_t *>(cb));
}

void configure_environment(const retro_variable *variables)
{
	if (!g_environment)
		return;

	retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	g_environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);

	bool support_no_game = true;
	g_environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &support_no_game);

	g_environment(RETRO_ENVIRONMENT_SET_VARIABLES, const_cast<retro_variable *>(variables));
}

void poll_input()
{
	if (g_input_poll)
		g_input_poll();
}

bool joypad_pressed(unsigned port, unsigned id)
{
	return g_input_state && g_input_state(port, RETRO_DEVICE_JOYPAD, 0, id);
}

bool key_pressed(unsigned id)
{
	return g_input_state && g_input_state(0, RETRO_DEVICE_KEYBOARD, 0, id);
}

std::string system_directory()
{
	if (!g_environment)
		return {};

	const char *system_dir = nullptr;
	if (g_environment(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
		return system_dir;

	return {};
}

std::string variable_value(const char *key, const char *fallback)
{
	if (!g_environment)
		return fallback ? fallback : "";

	retro_variable variable = { key, nullptr };
	if (g_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &variable) && variable.value && variable.value[0])
		return variable.value;

	return fallback ? fallback : "";
}

void capture_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch)
{
	if (!pixels || !width || !height)
		return;

	const std::size_t row_pixels = pitch / sizeof(uint32_t);
	if (row_pixels < width)
		return;

	g_captured_frame.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
	for (unsigned y = 0; y < height; ++y)
	{
		const uint32_t *src_row = pixels + (static_cast<std::size_t>(y) * row_pixels);
		uint32_t *dst_row = g_captured_frame.data() + (static_cast<std::size_t>(y) * width);
		std::memcpy(dst_row, src_row, static_cast<std::size_t>(width) * sizeof(uint32_t));
	}

	g_captured_width = width;
	g_captured_height = height;
}

bool copy_captured_xrgb8888(uint32_t *pixels, unsigned max_width, unsigned max_height, unsigned &width, unsigned &height)
{
	width = 0;
	height = 0;

	if (!pixels || g_captured_frame.empty() || !g_captured_width || !g_captured_height || !max_width || !max_height)
		return false;

	width = max_width;
	height = max_height;

	for (unsigned y = 0; y < height; ++y)
	{
		uint32_t *dst = pixels + (static_cast<std::size_t>(y) * max_width);
		const unsigned src_y = std::min<unsigned>(g_captured_height - 1, (static_cast<unsigned long long>(y) * g_captured_height) / height);
		const uint32_t *src_row = g_captured_frame.data() + (static_cast<std::size_t>(src_y) * g_captured_width);
		for (unsigned x = 0; x < width; ++x)
		{
			const unsigned src_x = std::min<unsigned>(g_captured_width - 1, (static_cast<unsigned long long>(x) * g_captured_width) / width);
			dst[x] = src_row[src_x];
		}
	}

	return true;
}

void present_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch)
{
	if (g_video)
		g_video(pixels, width, height, pitch);
}

void push_silence(double sample_rate, double fps)
{
	if (fps <= 0.0)
		return;

	const double exact_frames = (sample_rate / fps) + g_audio_remainder;
	std::size_t frames = static_cast<std::size_t>(exact_frames);
	g_audio_remainder = exact_frames - static_cast<double>(frames);

	if (frames > k_max_audio_frames)
		frames = k_max_audio_frames;

	if (g_audio_batch)
		g_audio_batch(g_silence.data(), frames);
	else if (g_audio)
	{
		for (std::size_t i = 0; i < frames; ++i)
			g_audio(0, 0);
	}
}

static std::vector<int32_t> g_accum_buffer;
static int g_accum_samples = 0;

void audio_begin_frame()
{
	g_accum_samples = 0;
	if (g_accum_buffer.size() < k_rb_size)
		g_accum_buffer.assign(k_rb_size, 0);
	else
		std::fill(g_accum_buffer.begin(), g_accum_buffer.end(), 0);
}

void mix_audio(const int16_t *buffer, int samples, bool left, bool right)
{
	if (!buffer || samples <= 0)
		return;

	if (static_cast<std::size_t>(samples * 2) > g_accum_buffer.size())
		return;

	if (samples > g_accum_samples)
		g_accum_samples = samples;

	for (int i = 0; i < samples; ++i)
	{
		if (left) g_accum_buffer[i * 2] += buffer[i];
		if (right) g_accum_buffer[i * 2 + 1] += buffer[i];
	}
}

void audio_end_frame()
{
	if (g_accum_samples <= 0)
		return;

	if (g_audio_ring_buffer.empty())
		g_audio_ring_buffer.resize(k_rb_size);

	for (int i = 0; i < g_accum_samples; ++i)
	{
		// Left
		int32_t l = g_accum_buffer[i * 2];
		if (l > 32767) l = 32767; else if (l < -32768) l = -32768;
		g_audio_ring_buffer[g_rb_write_index] = static_cast<int16_t>(l);
		g_rb_write_index = (g_rb_write_index + 1) % k_rb_size;

		// Right
		int32_t r = g_accum_buffer[i * 2 + 1];
		if (r > 32767) r = 32767; else if (r < -32768) r = -32768;
		g_audio_ring_buffer[g_rb_write_index] = static_cast<int16_t>(r);
		g_rb_write_index = (g_rb_write_index + 1) % k_rb_size;

		if (g_rb_count < k_rb_size - 1) g_rb_count += 2;
	}
}

void push_audio(const int16_t *buffer, int samples)
{
	if (!buffer || samples <= 0)
		return;

	if (g_audio_ring_buffer.empty())
		g_audio_ring_buffer.resize(k_rb_size);

	const std::size_t total_samples = static_cast<std::size_t>(samples) * k_audio_channels;
	for (std::size_t i = 0; i < total_samples; ++i)
	{
		g_audio_ring_buffer[g_rb_write_index] = buffer[i];
		g_rb_write_index = (g_rb_write_index + 1) % k_rb_size;
		
		if (g_rb_count < k_rb_size)
			g_rb_count++;
		else
			g_rb_read_index = (g_rb_read_index + 1) % k_rb_size; // Overflow
	}
}

void reset_audio()
{
	g_rb_read_index = 0;
	g_rb_write_index = 0;
	g_rb_count = 0;
	g_audio_warmed_up = false;
	g_audio_remainder = 0.0;
}

unsigned calc_audio_frames_for_run()
{
	// Use exactly what MAME gave us, or calculate expected frames
	const double fps = 60.0;
	const double sample_rate = 44100.0;
	const double samples = (sample_rate / fps) + g_audio_remainder;
	unsigned frames = static_cast<unsigned>(samples);
	g_audio_remainder = samples - static_cast<double>(frames);

	if (frames == 0)
		frames = 1;
	if (frames > k_max_audio_frames)
		frames = k_max_audio_frames;

	return frames;
}

void flush_audio_frame()
{
	if (!g_audio_batch || g_rb_count == 0)
		return;

	const std::size_t total_samples = g_rb_count;
	if (g_flush_buffer.size() < total_samples)
		g_flush_buffer.resize(total_samples);

	for (std::size_t i = 0; i < total_samples; ++i)
	{
		g_flush_buffer[i] = g_audio_ring_buffer[g_rb_read_index];
		g_rb_read_index = (g_rb_read_index + 1) % k_rb_size;
	}
	g_rb_count = 0;

	g_audio_batch(g_flush_buffer.data(), total_samples / 2);
}

void log(enum retro_log_level level, const char *fmt, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, fmt);
	std::vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (g_log)
		g_log(level, "%s", buffer);
	else
		std::fputs(buffer, stderr);
}

} // namespace fmtowns::libretro_osd
		frames = k_max_audio_frames;

	return frames;
}

void flush_audio_frame()
{
	if (!g_audio_batch || g_rb_count == 0)
		return;

	const std::size_t total_samples = g_rb_count;
	if (g_flush_buffer.size() < total_samples)
		g_flush_buffer.resize(total_samples);

	for (std::size_t i = 0; i < total_samples; ++i)
	{
		g_flush_buffer[i] = g_audio_ring_buffer[g_rb_read_index];
		g_rb_read_index = (g_rb_read_index + 1) % k_rb_size;
	}
	g_rb_count = 0;

	g_audio_batch(g_flush_buffer.data(), total_samples / 2);
}

void log(enum retro_log_level level, const char *fmt, ...)
{
	char buffer[1024];

	va_list args;
	va_start(args, fmt);
	std::vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	if (g_log)
		g_log(level, "%s", buffer);
	else
		std::fputs(buffer, stderr);
}

} // namespace fmtowns::libretro_osd
