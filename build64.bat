@echo off
setlocal

set "ROOT=%~dp0"
set "MSYS2_ROOT=C:\msys64"
set "RETROARCH_DIR=D:\Emulation\Emulators\RetroArch"
set "CORE_NAME=fmtowns_libretro"
set "SOURCE_DIR=%ROOT%libretro"
set "BUILD_DIR=%ROOT%build\libretro64"

if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
	echo MSYS2 not found at "%MSYS2_ROOT%".
	echo Install MSYS2 or edit MSYS2_ROOT in this script.
	exit /b 1
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

echo Building %CORE_NAME%.dll with MSYS2 MinGW64...
call "%MSYS2_ROOT%\msys2_shell.cmd" -defterm -no-start -mingw64 -c "mkdir -p /c/sw/fmtowns-master/build/libretro64 && cd /c/sw/fmtowns-master/build/libretro64 && rm -f fmtowns_libretro.o osd_libretro.o fmtowns_libretro.dll && g++ -O2 -std=gnu++17 -Wall -Wextra -I/c/sw/fmtowns-master/libretro -c /c/sw/fmtowns-master/libretro/fmtowns_libretro.cpp -o fmtowns_libretro.o && g++ -O2 -std=gnu++17 -Wall -Wextra -I/c/sw/fmtowns-master/libretro -c /c/sw/fmtowns-master/libretro/osd_libretro.cpp -o osd_libretro.o && g++ fmtowns_libretro.o osd_libretro.o -shared -o fmtowns_libretro.dll"
if errorlevel 1 (
	echo Build failed.
	exit /b 1
)

if not exist "%BUILD_DIR%\%CORE_NAME%.dll" (
	echo Build did not produce %CORE_NAME%.dll.
	exit /b 1
)

echo Built "%BUILD_DIR%\%CORE_NAME%.dll".

if exist "%RETROARCH_DIR%\cores" if exist "%RETROARCH_DIR%\info" (
	echo Installing to "%RETROARCH_DIR%"...
	copy /Y "%BUILD_DIR%\%CORE_NAME%.dll" "%RETROARCH_DIR%\cores\%CORE_NAME%.dll" >nul
	copy /Y "%SOURCE_DIR%\%CORE_NAME%.info" "%RETROARCH_DIR%\info\%CORE_NAME%.info" >nul
	echo Installed core and info files.
) else (
	echo RetroArch install folders not found at "%RETROARCH_DIR%"; skipping install.
)

endlocal
