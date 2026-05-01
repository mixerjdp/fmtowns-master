@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "MSYS2_ROOT=C:\msys64"
set "RETROARCH_DIR=C:\juegos\RetroArch-Win32"
set "CORE_NAME=fmtowns_libretro"
set "SOURCE_DIR=%ROOT%libretro"
set "BUILD_DIR=%ROOT%build\libretro32"
set "MAME_OUT_DIR=%ROOT%build\mingw-gcc\bin\x32\Release"
set "MAME_OBJ_DIR=%ROOT%build\mingw-gcc\obj\x32\Release"
set "MAME_PROJECT_DIR=%ROOT%build\projects\windows\mamefmtowns\gmake-mingw32-gcc"
set "MAME_FRONTEND_MAKEFILE=%ROOT%build\projects\windows\mamefmtowns\gmake-mingw32-gcc\frontend.make"
set "MAME_OPT_MARKER=%MAME_OUT_DIR%\.release32.opt3.sse2"
set "GENIE_MAKEFILE=build/projects/windows/mamefmtowns/gmake-mingw32-gcc/Makefile"
set "JOBS=%NUMBER_OF_PROCESSORS%"
if "%JOBS%"=="" set "JOBS=1"
set "MAKEFLAGS=-j%JOBS%"
set "LIBRETRO_MARKER=%BUILD_DIR%\.libretro.make.v3.sse2"
set "EXIT_CODE=0"
set "CLEAN_BUILD=0"
if /I "%~1"=="clean" set "CLEAN_BUILD=1"

if not exist "%MSYS2_ROOT%\msys2_shell.cmd" (
	echo MSYS2 not found at "%MSYS2_ROOT%".
	echo Install MSYS2 or edit MSYS2_ROOT in this script.
	set "EXIT_CODE=1"
	goto :cleanup
)

if "%CLEAN_BUILD%"=="1" (
	echo Cleaning previous x32 outputs...
	if exist "%MAME_OBJ_DIR%" rmdir /s /q "%MAME_OBJ_DIR%"
	if exist "%MAME_OUT_DIR%" rmdir /s /q "%MAME_OUT_DIR%"
	if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
	if exist "%MAME_PROJECT_DIR%" rmdir /s /q "%MAME_PROJECT_DIR%"
	if exist "%MAME_OPT_MARKER%" del /q "%MAME_OPT_MARKER%" >nul 2>nul
	if exist "%LIBRETRO_MARKER%" del /q "%LIBRETRO_MARKER%" >nul 2>nul
)

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

set "REGEN_PROJECT=0"
if not exist "%MAME_PROJECT_DIR%\Makefile" (
	set "REGEN_PROJECT=1"
) else (
	if not exist "%MAME_FRONTEND_MAKEFILE%" (
		set "REGEN_PROJECT=1"
	) else (
		findstr /I /C:"libretro_stub.cpp" "%MAME_FRONTEND_MAKEFILE%" >nul 2>nul || set "REGEN_PROJECT=1"
	)
)

if "%REGEN_PROJECT%"=="1" (
	echo Generating libretro-minimal mingw32-gcc project for x32...
	call "%MSYS2_ROOT%\msys2_shell.cmd" -defterm -no-start -mingw32 -c "cd /c/sw/fmtowns-master && make -j%JOBS% %GENIE_MAKEFILE% SUBTARGET=fmtowns OPTIMIZE=3 OPT_FLAGS='-msse2 -mfpmath=sse' NO_OPENGL=1 NO_USE_PORTAUDIO=1 NO_USE_MIDI=1 LIBRETRO_BUILD=1 MAME_LIBRETRO=1 LIBRETRO_MINIMAL=1"
	if errorlevel 1 (
		echo Project generation failed.
		set "EXIT_CODE=1"
		goto :cleanup
	)
) else (
	echo Reusing existing libretro-minimal project files.
)

if not exist "%MAME_OUT_DIR%\libfrontend.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\libemu.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\libosd_windows.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\libqtdbg_windows.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\libutils.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\libbgfx.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\libbimg.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\libbx.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\mame_fmtowns\liboptional.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\mame_fmtowns\libformats.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\mame_fmtowns\libdasm.a" set "REBUILD_X32=1"
if not exist "%MAME_OUT_DIR%\mame_fmtowns\libfujitsu.a" set "REBUILD_X32=1"
if not exist "%MAME_OPT_MARKER%" set "REBUILD_X32=1"

if "%REBUILD_X32%"=="1" (
	echo Building MAME x32 static libraries with the mingw32 toolchain...
	call "%MSYS2_ROOT%\msys2_shell.cmd" -defterm -no-start -mingw32 -c "cd /c/sw/fmtowns-master/build/projects/windows/mamefmtowns/gmake-mingw32-gcc && make config=release32 OPTIMIZE=3 OPT_FLAGS='-msse2 -mfpmath=sse' precompile -j%JOBS% && make config=release32 OPTIMIZE=3 OPT_FLAGS='-msse2 -mfpmath=sse' -j%JOBS%"
	if errorlevel 1 (
		echo MAME static library build failed.
		set "EXIT_CODE=1"
		goto :cleanup
	)
) else (
	echo Reusing existing MAME x32 static libraries. Use build32.bat clean to rebuild them.
)

echo Building %CORE_NAME%.dll with the mingw32 toolchain in x32 mode...
if not exist "%LIBRETRO_MARKER%" (
	if exist "%BUILD_DIR%\fmtowns_libretro.dll" del /q "%BUILD_DIR%\fmtowns_libretro.dll" >nul 2>nul
	if exist "%BUILD_DIR%\fmtowns_libretro.o" del /q "%BUILD_DIR%\fmtowns_libretro.o" >nul 2>nul
	if exist "%BUILD_DIR%\osd_libretro.o" del /q "%BUILD_DIR%\osd_libretro.o" >nul 2>nul
	if exist "%BUILD_DIR%\mame_bridge.o" del /q "%BUILD_DIR%\mame_bridge.o" >nul 2>nul
	if exist "%BUILD_DIR%\drivlist.o" del /q "%BUILD_DIR%\drivlist.o" >nul 2>nul
)
	call "%MSYS2_ROOT%\msys2_shell.cmd" -defterm -no-start -mingw32 -c "cd /c/sw/fmtowns-master/build/libretro32 && make -f /c/sw/fmtowns-master/libretro/Makefile.libretro TARGET_OS=windows SRCROOT=/c/sw/fmtowns-master OBJDIR=. LIBROOT=/c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release GENDRIVLIST=/c/sw/fmtowns-master/build/generated/mame/fmtowns/drivlist.cpp CXX=g++ STRIP=strip PORTABLE_CPUFLAGS='-msse2 -mfpmath=sse' EXTRA_LIBS='-Wl,--start-group /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libfrontend.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/mame_fmtowns/liboptional.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libemu.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libosd_windows.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libqtdbg_windows.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/mame_fmtowns/libformats.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/mame_fmtowns/libdasm.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/mame_fmtowns/libfujitsu.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libutils.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libexpat.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libsoftfloat.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libsoftfloat3.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libwdlfft.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libymfm.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libjpeg.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/lib7z.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/liblua.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/liblualibs.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/liblinenoise.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libzlib.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libzstd.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libflac.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libutf8proc.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libsqlite3.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libbgfx.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libbimg.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libbx.a /c/sw/fmtowns-master/build/mingw-gcc/bin/x32/Release/libocore_windows.a -Wl,--end-group -luser32 -ladvapi32 -lwsock32 -lws2_32 -liphlpapi -lshell32 -luserenv -lgdi32 -ldsound -ldxguid -loleaut32 -lwinmm -lmingw32 -lcomctl32 -lcomdlg32 -ldinput8 -lksguid -lole32 -lpsapi -lshlwapi -luuid'"
if errorlevel 1 (
	echo Build failed.
	set "EXIT_CODE=1"
	goto :cleanup
)

if not exist "%BUILD_DIR%\%CORE_NAME%.dll" (
	echo Build did not produce %CORE_NAME%.dll.
	set "EXIT_CODE=1"
	goto :cleanup
)

echo Built "%BUILD_DIR%\%CORE_NAME%.dll".
> "%MAME_OPT_MARKER%" echo release32-opt3
> "%LIBRETRO_MARKER%" echo libretro-make-v2

if exist "%RETROARCH_DIR%\cores" if exist "%RETROARCH_DIR%\info" (
	echo Installing to "%RETROARCH_DIR%"...
	copy /Y "%BUILD_DIR%\%CORE_NAME%.dll" "%RETROARCH_DIR%\cores\%CORE_NAME%.dll" >nul
	copy /Y "%SOURCE_DIR%\%CORE_NAME%.info" "%RETROARCH_DIR%\info\%CORE_NAME%.info" >nul
	echo Installed core and info files.
) else (
	echo RetroArch install folders not found at "%RETROARCH_DIR%"; skipping install.
)

:cleanup
endlocal & exit /b %EXIT_CODE%
