# FM Towns BIOS layout for RetroArch

The libretro core looks for loose FM Towns BIOS ROMs in this order:

1. `system/fmtowns`
2. `system`

For the default Phase 1 model, `fmtownsux`, keep the files uncompressed:

```text
fmt_dos_a.rom
fmt_dic.rom
fmt_fnt.rom
fmt_sys_a.rom
mytownsux.rom
```

Your current source BIOS folder is:

```text
C:\juegos\FMTowns
```

The intended RetroArch install folder is:

```text
D:\Emulation\Emulators\RetroArch\system\fmtowns
```

MAME-compatible ZIP sets are not required for this libretro route. Phase 1 is
starting with loose `.rom` lookup so the later MAME bootstrap can receive a
normal media path rooted in RetroArch's `system` directory.
