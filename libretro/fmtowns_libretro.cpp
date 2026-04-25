#include "libretro.h"

#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

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

retro_environment_t g_environment = nullptr;
retro_video_refresh_t g_video = nullptr;
retro_audio_sample_t g_audio = nullptr;
retro_audio_sample_batch_t g_audio_batch = nullptr;
retro_input_poll_t g_input_poll = nullptr;
retro_input_state_t g_input_state = nullptr;
retro_log_printf_t g_log = nullptr;

bool g_loaded = false;
std::array<uint32_t, k_width * k_height> g_framebuffer = {};
std::array<int16_t, 2 * 735> g_silence = {};

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

void set_pixel_format()
{
	if (!g_environment)
		return;

	retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	g_environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
}

void set_support_no_game()
{
	if (!g_environment)
		return;

	bool support_no_game = true;
	g_environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &support_no_game);
}

} // namespace

RETRO_API_EXPORT void retro_set_environment(retro_environment_t cb)
{
	g_environment = cb;

	if (g_environment)
	{
		retro_log_callback logging = {};
		if (g_environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
			g_log = logging.log;
	}

	set_pixel_format();
	set_support_no_game();
}

RETRO_API_EXPORT void retro_set_video_refresh(retro_video_refresh_t cb)
{
	g_video = cb;
}

RETRO_API_EXPORT void retro_set_audio_sample(retro_audio_sample_t cb)
{
	g_audio = cb;
}

RETRO_API_EXPORT void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
	g_audio_batch = cb;
}

RETRO_API_EXPORT void retro_set_input_poll(retro_input_poll_t cb)
{
	g_input_poll = cb;
}

RETRO_API_EXPORT void retro_set_input_state(retro_input_state_t cb)
{
	g_input_state = cb;
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
	info->library_version = "0.0-phase0";
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
	set_pixel_format();
	set_support_no_game();
	log(RETRO_LOG_INFO, "fmtowns_libretro Phase 0 initialized; MAME bootstrap is not connected yet.\n");
}

RETRO_API_EXPORT void retro_deinit(void)
{
	g_loaded = false;
}

RETRO_API_EXPORT void retro_reset(void)
{
}

RETRO_API_EXPORT bool retro_load_game(const retro_game_info *game)
{
	g_loaded = true;
	log(RETRO_LOG_INFO, "fmtowns_libretro Phase 0 loaded placeholder session%s%s.\n",
			game && game->path ? ": " : "",
			game && game->path ? game->path : "");
	return true;
}

RETRO_API_EXPORT void retro_unload_game(void)
{
	g_loaded = false;
}

RETRO_API_EXPORT unsigned retro_get_region(void)
{
	return RETRO_REGION_NTSC;
}

RETRO_API_EXPORT void retro_run(void)
{
	if (g_input_poll)
		g_input_poll();

	if (g_audio_batch)
		g_audio_batch(g_silence.data(), g_silence.size() / 2);
	else if (g_audio)
		g_audio(0, 0);

	if (g_video)
		g_video(g_framebuffer.data(), k_width, k_height, k_width * sizeof(uint32_t));
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
