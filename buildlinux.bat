@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
set "VM_USER=juan"
set "VM_HOST=192.168.31.128"
set "VM_PORT=22"
set "VM_DIR=fmtowns-master"
set "CORE_NAME=fmtowns_libretro"
REM Limit to 1 job to avoid OOM on VM (3.8GB RAM)
set "JOBS=1"

echo == FM Towns Linux Build Script (Incremental) ==
echo.

REM ---- Step 0: Ensure rsync is available ----
echo [0/6] Checking for rsync...
set "RSYNC_CMD=rsync"
where rsync >nul 2>&1
if not errorlevel 1 goto :rsync_ready

echo rsync not found in PATH, checking common locations...
set "RSYNC_CMD="
if exist "%ProgramFiles%\Git\usr\bin\rsync.exe" set "RSYNC_CMD=%ProgramFiles%\Git\usr\bin\rsync.exe"
if exist "%LocalAppData%\Programs\Git\usr\bin\rsync.exe" set "RSYNC_CMD=%LocalAppData%\Programs\Git\usr\bin\rsync.exe"
if exist "%ProgramFiles(x86)%\Git\usr\bin\rsync.exe" set "RSYNC_CMD=%ProgramFiles(x86)%\Git\usr\bin\rsync.exe"
if exist "%USERPROFILE%\scoop\shims\rsync.exe" set "RSYNC_CMD=%USERPROFILE%\scoop\shims\rsync.exe"
if not "%RSYNC_CMD%"=="" goto :rsync_ready

echo rsync not found. Attempting to download portable version...
set "RSYNC_DIR=%TEMP%\rsync-portable"
set "RSYNC_ZIP=%TEMP%\rsync.zip"
powershell -NoProfile -Command "& { $url='https://github.com/itefix/rsync-windows/releases/download/v3.2.3/rsync-3.2.3-x86_64.zip'; $zip='%RSYNC_ZIP%'; try { Write-Host 'Downloading rsync from itefix/rsync-windows...'; Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing; Expand-Archive -Path $zip -DestinationPath '%RSYNC_DIR%' -Force; Write-Host 'Downloaded successfully'; } catch { Write-Host ('Download failed: ' + $_.Exception.Message); exit 1; } }"
if errorlevel 1 (
    echo WARNING: Could not download rsync. Falling back to tar+ssh sync.
    goto :sync_tar
)
for /r "%RSYNC_DIR%" %%f in (rsync.exe) do if exist "%%f" set "RSYNC_CMD=%%f" & goto :rsync_found
:rsync_found
if "%RSYNC_CMD%"=="" (
    echo WARNING: rsync.exe not found in downloaded archive. Falling back to tar+ssh.
    goto :sync_tar
)
echo rsync downloaded to %RSYNC_CMD%

:rsync_ready
echo rsync ready: %RSYNC_CMD%

echo [1/6] Testing SSH connection to %VM_USER%@%VM_HOST%...
ssh -p %VM_PORT% %VM_USER%@%VM_HOST% "hostname" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Cannot connect to %VM_USER%@%VM_HOST%
    echo Make sure the VM is running and reachable at %VM_HOST%
    exit /b 1
)
echo OK

echo [2/6] Syncing source code to Linux VM (incremental rsync)...
ssh -p %VM_PORT% %VM_USER%@%VM_HOST% "mkdir -p ~/%VM_DIR%"
cd /d "%ROOT%" && "%RSYNC_CMD%" -avz --delete --exclude=/build/ --exclude=/.git/ -e "ssh -p %VM_PORT%" ./ %VM_USER%@%VM_HOST%:~/fmtowns-master/
if errorlevel 1 (
    echo ERROR: rsync sync failed
    exit /b 1
)
echo OK
goto :build

:sync_tar
echo [2/6] Syncing source code to Linux VM (tar+ssh fallback)...
REM Preserve build/ on VM for incremental compilation
ssh -p %VM_PORT% %VM_USER%@%VM_HOST% "mkdir -p ~/%VM_DIR%"
cd /d "%ROOT%" && tar cf - --exclude=build --exclude=.git . | ssh -p %VM_PORT% %VM_USER%@%VM_HOST% "tar xf - -C ~/%VM_DIR%"
if errorlevel 1 (
    echo ERROR: Source sync failed
    exit /b 1
)
echo OK

:build
echo [3/6] Building MAME static libraries on VM (incremental make)...
echo First build can take 10-30 minutes. Subsequent builds only recompile changed files.
ssh -p %VM_PORT% %VM_USER%@%VM_HOST% "cd ~/%VM_DIR% && make SUBTARGET=fmtowns USE_QTDEBUG=0 ARCHOPTS=-fPIC -j%JOBS% 2>&1"
if errorlevel 1 (
    echo ERROR: MAME build failed
    exit /b 1
)
echo OK

echo [4/6] Building %CORE_NAME%.so...
ssh -p %VM_PORT% %VM_USER%@%VM_HOST% "cd ~/%VM_DIR% && bash build_linux.sh 2>&1"
if errorlevel 1 (
    echo ERROR: libretro build failed
    exit /b 1
)
echo OK

echo [5/6] Copying .so back to Windows...
if not exist "%ROOT%build\libretro\linux64" mkdir "%ROOT%build\libretro\linux64"
scp -P %VM_PORT% %VM_USER%@%VM_HOST%:~/fmtowns-master/build/libretro/linux64/%CORE_NAME%.so "%ROOT%build\libretro\linux64\%CORE_NAME%.so"
if errorlevel 1 (
    echo ERROR: Copy back failed
    exit /b 1
)
echo OK

echo.
echo Build complete: %ROOT%build\libretro\linux64\%CORE_NAME%.so
endlocal
