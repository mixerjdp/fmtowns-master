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

For the in-process MAME bootstrap, keep the ROMs loose but mirror MAME's set
folders under that same directory:

```text
system/fmtowns/fmtownsux/fmt_dos_a.rom
system/fmtowns/fmtownsux/fmt_sys_a.rom
system/fmtowns/fmtownsux/mytownsux.rom
system/fmtowns/fmtowns/fmt_dic.rom
system/fmtowns/fmtowns/fmt_fnt.rom
```

MAME-compatible ZIP sets are not required for this libretro route. The core
uses `system/fmtowns` as `rompath`, so loose files need the set subfolders even
though they are not compressed.
