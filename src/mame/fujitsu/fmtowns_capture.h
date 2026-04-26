#ifndef MAME_FUJITSU_FMTOWNS_CAPTURE_H
#define MAME_FUJITSU_FMTOWNS_CAPTURE_H

#include <cstddef>
#include <cstdint>

namespace fmtowns::screen_capture {

using capture_callback = void (*)(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch);

inline capture_callback g_capture_callback = nullptr;

inline void set_callback(capture_callback cb)
{
	g_capture_callback = cb;
}

inline void capture_xrgb8888(const uint32_t *pixels, unsigned width, unsigned height, std::size_t pitch)
{
	if (g_capture_callback)
		g_capture_callback(pixels, width, height, pitch);
}

} // namespace fmtowns::screen_capture

#endif
