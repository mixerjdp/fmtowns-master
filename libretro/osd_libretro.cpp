#include "osd_libretro.h"

#include <array>
#include <cstdarg>
#include <cstdio>

namespace fmtowns::libretro_osd {
namespace {

constexpr unsigned k_audio_channels = 2;
constexpr std::size_t k_max_audio_frames = 2048;

retro_environment_t g_environment = nullptr;
retro_video_refresh_t g_video = nullptr;
retro_audio_sample_t g_audio = nullptr;
retro_audio_sample_batch_t g_audio_batch = nullptr;
retro_input_poll_t g_input_poll = nullptr;
retro_input_state_t g_input_state = nullptr;
retro_log_printf_t g_log = nullptr;

std::array<int16_t, k_audio_channels * k_max_audio_frames> g_silence = {};
double g_audio_remainder = 0.0;

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
