# FM Towns Libretro Phase 0

This folder starts the `fmtowns_libretro` target as a libretro-loadable DLL
stub. It deliberately does not boot MAME yet.

Phase 0 goals covered here:

- Export the libretro ABI entry points.
- Advertise the direct-media extensions planned for FM Towns/Marty.
- Avoid MAME UI, MAME CLI frontend, windows, debugger, Lua, HTTP, BGFX, and
  native renderer dependencies.
- Produce a black video frame and silent audio so RetroArch can load the core
  and drive `retro_run()`.

Build from this directory with a MinGW-style toolchain:

```sh
make -f Makefile.libretro TARGET_OS=windows
```

Or from the repository root on this Windows setup:

```bat
build64.bat
```

This writes build artifacts to:

```text
build\libretro64
```

Expected output:

```text
fmtowns_libretro.dll
```

That DLL is no longer only a Phase 0 placeholder. Phase 1 added RetroArch
system-directory and loose-BIOS discovery, and Phase 2 adds the non-blocking
runtime shell used by `retro_run()`. The direct MAME bootstrap still lands in a
later phase, keeping this libretro ABI surface as the stable outer shell.
