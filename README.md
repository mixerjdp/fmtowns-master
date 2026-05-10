# FM Towns libretro core

FM Towns libretro is a **RetroArch / libretro core** based on **MAME 0.287** for the **FM Towns / FM Towns Marty** family.

The goal of this core is to provide a practical FM Towns experience inside RetroArch, with separate Win32 and Win64 builds, RetroArch metadata, and no redistribution of proprietary BIOS or ROM files.

## Core features

- FM Towns / FM Towns Marty libretro core.
- Based on the MAME FM Towns driver set.
- In-process libretro runtime for RetroArch.
- 32-bit and 64-bit Windows build scripts.
- RetroArch `.info` metadata file.
- BIOS lookup through RetroArch's `system` directory.
- Support for CD and disk images through the libretro content extensions listed below.

## What is included

- The libretro core binaries for Win32 and Win64.
- The RetroArch `.info` file.
- Build scripts for both Windows architectures.
- Libretro-specific source code and wrapper glue.

## What is not included

- System BIOS files.
- ROM images or commercial game media.
- Third-party redistributable packages that are not part of this repository.

## Requirements

To use the core you need:

- RetroArch installed.
- The matching `fmtowns_libretro.dll` for your RetroArch architecture.
- The `fmtowns_libretro.info` file.
- Legal FM Towns BIOS files placed in RetroArch's system directory.

## Install the core

### 64-bit

Copy:

```text
fmtowns_libretro.dll
```

into your RetroArch 64-bit cores directory, for example:

```text
RetroArch/cores/
```

### 32-bit

If you use a 32-bit RetroArch installation, copy the Win32 build of the same DLL into that installation's `cores` directory.

## Install the `.info` file

Copy:

```text
fmtowns_libretro.info
```

into your RetroArch info directory, for example:

```text
RetroArch/info/
```

RetroArch uses this file to show the core name and to recognize the supported content extensions.

## Install BIOS and ROM files

Place the FM Towns system files in RetroArch's `system` directory. A common layout is:

```text
RetroArch/system/fmtowns/
```

If your RetroArch setup uses a different system path, update the BIOS directory used by the core or place the files where your build expects them.

Typical BIOS file names can include:

```text
FMT_SYS.ROM
FMT_DOS.ROM
FMT_FNT.ROM
FMT_DIC.ROM
```

Exact filenames can vary depending on the BIOS set you use. The important part is that the files are available to RetroArch and match the model selected in the core.

## Supported content

The core currently advertises the following extensions in its RetroArch metadata:

```text
chd|cue|toc|nrg|gdi|iso|cdr|mfi|dfi|mfm|td0|imd|86f|d77|d88|1dd|cqm|cqi|dsk|bin|hdm|hd|hdv|2mg|hdi|h0|m3u
```

Notes:

- CD media formats include `chd`, `cue`, `toc`, `nrg`, `gdi`, `iso`, `cdr`, and `bin`.
- Disk and floppy formats include `mfi`, `dfi`, `mfm`, `td0`, `imd`, `86f`, `d77`, `d88`, `1dd`, `cqm`, `cqi`, and `dsk`.
- Hard-disk style images include `hdm`, `hd`, `hdv`, `2mg`, `hdi`, and `h0`.
- `m3u` can be used for multi-disc workflows when your RetroArch setup supports it.

## How to run

1. Start RetroArch.
2. Load the `FM Towns (MAME)` core.
3. Load a supported FM Towns image.
4. If the content requires BIOS files, confirm they are present in RetroArch's `system` directory.

## Local build

### Windows

Build scripts are provided for both architectures:

```bat
build64.bat
build32.bat
```

The scripts build the core, install the DLL and `.info` file into your configured RetroArch directories, and keep the wrapper build incremental so repeated builds are faster.

### Linux

A build script is provided for Linux:

```sh
./build_linux.sh
```

This script locates the MAME static libraries previously built in `build/`, then compiles the libretro shared object using `libretro/Makefile.libretro`. The resulting `fmtowns_libretro.so` is placed in `build/libretro/linux64/`.

Requirements: `g++`, `make`, `pkg-config`, SDL2 and X11 development libraries.

## Release v1.00

The `Fm Towns v1.00` release publishes these files:

- `fmtowns_libretro_32bit.dll`
- `fmtowns_libretro_64bit.dll`
- `fmtowns_libretro.info`

No BIOS files are included in the release.

## Notes

- The libretro-specific sources live in `libretro/`.
- The phase documents live in `libretro/README_PHASE0.md` through `libretro/README_PHASE3.md`.
- The core is intended to be used through RetroArch, not as a standalone FM Towns emulator.

## Thanks

- The MAME project and its contributors.
- The RetroArch / libretro community.
- Everyone documenting FM Towns hardware, media formats, and BIOS requirements.
