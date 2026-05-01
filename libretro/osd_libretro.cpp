#include "osd_libretro.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

namespace fmtowns::libretro_osd {
namespace {

constexpr unsigned k_audio_channels = 2;
constexpr std::size_t k_max_audio_frames = 2048;
constexpr int16_t k_default_analog_deadzone = 8000;

retro_environment_t g_environment = nullptr;
retro_video_refresh_t g_video = nullptr;
retro_audio_sample_t g_audio = nullptr;
retro_audio_sample_batch_t g_audio_batch = nullptr;
retro_input_poll_t g_input_poll = nullptr;
retro_input_state_t g_input_state = nullptr;
retro_log_printf_t g_log = nullptr;
input_routing_mode g_input_routing_mode = input_routing_mode::hybrid;

std::array<int16_t, k_audio_channels * k_max_audio_frames> g_silence = {};
double g_audio_remainder = 0.0;
std::vector<uint32_t> g_captured_frame;
unsigned g_captured_width = 0;
unsigned g_captured_height = 0;
unsigned g_mame_audio_channels = 2;

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

void set_input_descriptors(const struct retro_input_descriptor *desc)
{
	if (g_environment && desc)
		g_environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<struct retro_input_descriptor *>(desc));
}

void set_input_routing_mode(input_routing_mode mode)
{
	g_input_routing_mode = mode;
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

bool variable_update_pending()
{
	if (!g_environment)
		return false;

	bool updated = false;
	return g_environment(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated;
}

bool keyboard_input_enabled()
{
	return g_input_routing_mode != input_routing_mode::retropad;
}

bool joypad_input_enabled()
{
	return g_input_routing_mode != input_routing_mode::keyboard;
}

bool joypad_pressed(unsigned port, unsigned id)
{
	return g_input_state && g_input_state(port, RETRO_DEVICE_JOYPAD, 0, id);
}

int16_t joypad_analog(unsigned port, unsigned index, unsigned id)
{
	return g_input_state ? g_input_state(port, RETRO_DEVICE_ANALOG, index, id) : 0;
}

analog_stick_state normalized_joypad_stick(unsigned port, unsigned index)
{
	analog_stick_state stick = {};
	if (!g_input_state)
		return stick;

	int x = g_input_state(port, RETRO_DEVICE_ANALOG, index, RETRO_DEVICE_ID_ANALOG_X);
	int y = g_input_state(port, RETRO_DEVICE_ANALOG, index, RETRO_DEVICE_ID_ANALOG_Y);

	const double radius = std::sqrt(static_cast<double>(x) * static_cast<double>(x) +
			static_cast<double>(y) * static_cast<double>(y));
	if (radius <= static_cast<double>(k_default_analog_deadzone))
		return stick;

	const double angle = std::atan2(static_cast<double>(y), static_cast<double>(x));
	double scaled_radius = (radius - static_cast<double>(k_default_analog_deadzone)) *
			(32767.0 / (32767.0 - static_cast<double>(k_default_analog_deadzone)));
	scaled_radius = std::min(32767.0, scaled_radius);

	x = static_cast<int>(std::lround(scaled_radius * std::cos(angle)));
	y = static_cast<int>(std::lround(scaled_radius * std::sin(angle)));

	x = std::clamp(x, -32767, 32767);
	y = std::clamp(y, -32767, 32767);

	stick.x = static_cast<int16_t>(x);
	stick.y = static_cast<int16_t>(y);
	return stick;
}

int16_t mouse_axis(unsigned port, unsigned id)
{
	return g_input_state ? g_input_state(port, RETRO_DEVICE_MOUSE, 0, id) : 0;
}

bool mouse_pressed(unsigned port, unsigned id)
{
	return g_input_state && g_input_state(port, RETRO_DEVICE_MOUSE, 0, id);
}

bool keyboard_pressed(unsigned key)
{
	return g_input_state && g_input_state(0, RETRO_DEVICE_KEYBOARD, 0, key);
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

	if (g_captured_width > max_width || g_captured_height > max_height)
		return false;

	width = g_captured_width;
	height = g_captured_height;

	for (unsigned y = 0; y < height; ++y)
	{
		uint32_t *dst = pixels + (static_cast<std::size_t>(y) * max_width);
		const uint32_t *src_row = g_captured_frame.data() + (static_cast<std::size_t>(y) * g_captured_width);
		std::memcpy(dst, src_row, static_cast<std::size_t>(width) * sizeof(uint32_t));
	}

	return true;
}

bool set_geometry(unsigned base_width, unsigned base_height, unsigned max_width, unsigned max_height, float aspect_ratio)
{
	if (!g_environment || !base_width || !base_height || !max_width || !max_height)
		return false;

	retro_game_geometry geometry = {};
	geometry.base_width = base_width;
	geometry.base_height = base_height;
	geometry.max_width = max_width;
	geometry.max_height = max_height;
	geometry.aspect_ratio = aspect_ratio;
	return g_environment(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
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

void set_mame_audio_channels(unsigned channels)
{
	g_mame_audio_channels = channels ? channels : 1;
}

unsigned mame_audio_channels()
{
	return g_mame_audio_channels ? g_mame_audio_channels : 1;
}

void push_mame_audio_samples(const int16_t *samples, std::size_t frames)
{
	if (!samples || frames == 0)
		return;

	const unsigned channels = mame_audio_channels();
	std::vector<int16_t> stereo;
	stereo.resize(frames * 2);

	if (channels == 1)
	{
		for (std::size_t i = 0; i < frames; ++i)
		{
			const int16_t sample = samples[i];
			stereo[(i * 2) + 0] = sample;
			stereo[(i * 2) + 1] = sample;
		}
	}
	else
	{
		for (std::size_t i = 0; i < frames; ++i)
		{
			const std::size_t base = i * static_cast<std::size_t>(channels);
			const int16_t left = samples[base + 0];
			const int16_t right = (channels > 1) ? samples[base + 1] : left;
			stereo[(i * 2) + 0] = left;
			stereo[(i * 2) + 1] = right;
		}
	}

	if (g_audio_batch)
		g_audio_batch(stereo.data(), frames);
	else if (g_audio)
	{
		for (std::size_t i = 0; i < frames; ++i)
			g_audio(stereo[(i * 2) + 0], stereo[(i * 2) + 1]);
	}
}

void push_mame_audio(const float *left, const float *right, std::size_t frames)
{
	if (frames == 0)
		return;

	if (!left)
		left = right;
	if (!right)
		right = left;

	static std::vector<int16_t> interleaved;
	interleaved.resize(frames * 2);

	for (std::size_t i = 0; i < frames; ++i)
	{
		const float l = left ? std::clamp(left[i], -1.0f, 1.0f) : 0.0f;
		const float r = right ? std::clamp(right[i], -1.0f, 1.0f) : l;
		interleaved[(i * 2) + 0] = static_cast<int16_t>(l * 32767.0f);
		interleaved[(i * 2) + 1] = static_cast<int16_t>(r * 32767.0f);
	}

	if (g_audio_batch)
		g_audio_batch(interleaved.data(), frames);
	else if (g_audio)
	{
		for (std::size_t i = 0; i < frames; ++i)
		{
			g_audio(interleaved[(i * 2) + 0], interleaved[(i * 2) + 1]);
		}
	}
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
