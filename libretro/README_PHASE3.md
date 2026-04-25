# FM Towns Libretro Phase 3

Phase 3 starts the libretro OSD layer as its own adapter instead of keeping
callbacks buried in the core entry point.

Current OSD adapters:

- Environment setup: pixel format, no-game support, core variables.
- Video sink: XRGB8888 frames to `retro_video_refresh`.
- Audio sink: frame-sized silent batches to `retro_audio_sample_batch`.
- Input source: per-frame polling plus joypad state queries.
- Paths/options: RetroArch system directory and core option lookup.

The real MAME bridge should send `screen_device` output through
`present_xrgb8888()` first. Once MAME audio is wired, replace the silent audio
batch in `push_silence()` with the mixed sample buffer from the machine.

No MAME UI, native window, BGFX renderer, CLI frontend, or external process is
used by this layer.
