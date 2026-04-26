# FM Towns libretro core

This repository contains a work-in-progress FM Towns / Marty libretro core
based on MAME.

## Build

On this Windows workspace, build the core with:

```bat
build64.bat
```

The script uses MSYS2 UCRT64, rebuilds the required MAME static libraries, and
produces:

```text
build\libretro64\fmtowns_libretro.dll
```

If RetroArch is installed in a different location, update `RETROARCH_DIR` in
`build64.bat`.

## Notes

- The libretro-specific sources live in `libretro/`.
- Phase documents are in `libretro/README_PHASE0.md` through
  `libretro/README_PHASE3.md`.
