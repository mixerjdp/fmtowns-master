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

echo Refreshing MAME fmtowns static libraries...
call "%MSYS2_ROOT%\msys2_shell.cmd" -defterm -no-start -ucrt64 -c "cd /c/sw/fmtowns-master && CPPFLAGS+='-I/c/sw/fmtowns-master/src/mame' make SUBTARGET=fmtowns -j6"
if errorlevel 1 (
	echo MAME static library build failed.
	exit /b 1
)

echo Building %CORE_NAME%.dll with MSYS2 UCRT64...
call "%MSYS2_ROOT%\msys2_shell.cmd" -defterm -no-start -ucrt64 -c "mkdir -p /c/sw/fmtowns-master/build/libretro64 && cd /c/sw/fmtowns-master/build/libretro64 && rm -f fmtowns_libretro.o osd_libretro.o mame_bridge.o drivlist.o fmtowns_libretro.dll && CXXFLAGS='-O2 -std=c++20 -Wall -Wextra -I/c/sw/fmtowns-master/libretro -I/c/sw/fmtowns-master/src/osd -I/c/sw/fmtowns-master/src/emu -I/c/sw/fmtowns-master/src/devices -I/c/sw/fmtowns-master/src/mame -I/c/sw/fmtowns-master/src/frontend/mame -I/c/sw/fmtowns-master/src/lib -I/c/sw/fmtowns-master/src/lib/util -I/c/sw/fmtowns-master/3rdparty -I/c/sw/fmtowns-master/3rdparty/asio/include -I/c/sw/fmtowns-master/3rdparty/rapidjson/include -I/c/sw/fmtowns-master/3rdparty/expat/lib -I/c/sw/fmtowns-master/3rdparty/zlib -I/c/sw/fmtowns-master/3rdparty/flac/include -I/c/sw/fmtowns-master/build/generated/emu -I/c/sw/fmtowns-master/build/generated/emu/layout -I/c/sw/fmtowns-master/build/generated/mame/layout' && LIBROOT=/c/sw/fmtowns-master/build/mingw-gcc/bin/x64/Release && g++ $CXXFLAGS -c /c/sw/fmtowns-master/libretro/fmtowns_libretro.cpp -o fmtowns_libretro.o && g++ $CXXFLAGS -c /c/sw/fmtowns-master/libretro/osd_libretro.cpp -o osd_libretro.o && g++ $CXXFLAGS -c /c/sw/fmtowns-master/libretro/mame_bridge.cpp -o mame_bridge.o && g++ $CXXFLAGS -I/c/sw/fmtowns-master/build/generated/mame/fmtowns -c /c/sw/fmtowns-master/build/generated/mame/fmtowns/drivlist.cpp -o drivlist.o && g++ -shared -static -static-libgcc -static-libstdc++ -o fmtowns_libretro.dll fmtowns_libretro.o osd_libretro.o mame_bridge.o drivlist.o -Wl,--start-group $LIBROOT/mame_fmtowns/libmame_fmtowns.a $LIBROOT/libfrontend.a $LIBROOT/mame_fmtowns/liboptional.a $LIBROOT/libemu.a $LIBROOT/libosd_windows.a $LIBROOT/libqtdbg_windows.a $LIBROOT/mame_fmtowns/libformats.a $LIBROOT/mame_fmtowns/libdasm.a $LIBROOT/libutils.a $LIBROOT/libexpat.a $LIBROOT/libsoftfloat.a $LIBROOT/libsoftfloat3.a $LIBROOT/libwdlfft.a $LIBROOT/libymfm.a $LIBROOT/libjpeg.a $LIBROOT/lib7z.a $LIBROOT/liblua.a $LIBROOT/liblualibs.a $LIBROOT/liblinenoise.a $LIBROOT/libzlib.a $LIBROOT/libzstd.a $LIBROOT/libflac.a $LIBROOT/libutf8proc.a $LIBROOT/libsqlite3.a $LIBROOT/libportmidi.a $LIBROOT/libbgfx.a $LIBROOT/libbimg.a $LIBROOT/libbx.a $LIBROOT/libocore_windows.a -Wl,--end-group -luser32 -ladvapi32 -lwsock32 -lws2_32 -liphlpapi -lshell32 -luserenv -lgdi32 -ldsound -ldxguid -loleaut32 -lwinmm -lmingw32 -lcomctl32 -lcomdlg32 -ldinput8 -lksguid -lole32 -lpsapi -lshlwapi -luuid"
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
