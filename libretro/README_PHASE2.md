# FM Towns Libretro Phase 2

Phase 2 adds the non-blocking runtime shell that libretro needs before the
real MAME machine is connected.

Current behavior:

- `retro_load_game()` creates one runtime session.
- `retro_run()` advances exactly one frame at a time.
- `retro_reset()` schedules a reset that is applied on the next frame.
- `retro_unload_game()` tears down the session.
- Audio/video callbacks are driven every frame without using MAME UI,
  windows, BGFX, CLI frontend, or an external process.

The placeholder frame contains a subtle moving scan marker so a smoke test can
confirm that `retro_run()` advances without blocking. This marker is temporary;
Fase 3 replaces it with the raw framebuffer coming from `screen_device`.

The intended MAME insertion point is `phase2_runtime::run_frame()`, where the
current placeholder slice will become a scheduler/timeslice call against an
already-started `running_machine`.
