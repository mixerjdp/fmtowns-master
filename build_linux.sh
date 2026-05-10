#!/bin/bash
set -e

cd "$(dirname "$0")"
REPO_DIR="$PWD"
JOBS=$(nproc)

# Check for pkg-config
if ! command -v pkg-config &>/dev/null; then
    echo "WARNING: pkg-config not found. Attempting to continue without it."
fi

echo "=== Find library output directory ==="
LIBROOT=$(find build -name "libfrontend.a" -exec dirname {} \; 2>/dev/null | head -1)
if [ -z "$LIBROOT" ]; then
    echo "ERROR: Could not find libfrontend.a in build/"
    echo ""
    echo "Make sure Step 3 (make SUBTARGET=fmtowns) completed successfully."
    echo "Possible causes:"
    echo "  - The MAME build failed or is incomplete"
    echo "  - The build output is in a non-standard location"
    echo ""
    echo "Checking for alternative locations..."
    find build -name "*.a" -maxdepth 5 2>/dev/null | head -20 || true
    exit 1
fi

# Normalize path
LIBROOT=$(cd "$LIBROOT" && pwd)
echo "Library root: $LIBROOT"

echo "Static libraries found:"
find "$LIBROOT" -name "*.a" | sort

echo ""
echo "=== Check generated files ==="
if [ ! -f "build/generated/mame/fmtowns/drivlist.cpp" ]; then
    echo "ERROR: build/generated/mame/fmtowns/drivlist.cpp not found."
    echo "The MAME build step did not generate the driver list."
    exit 1
fi
echo "drivlist.cpp found OK"

echo ""
echo "=== Build libretro core ==="
ALL_LIBS=$(find "$LIBROOT" -name "*.a" | sort | tr '\n' ' ')
SDL_LIBS=$(pkg-config --libs sdl2 2>/dev/null || echo "-lSDL2")
X11_LIBS=$(pkg-config --libs x11 2>/dev/null || echo "-lX11")

mkdir -p build/libretro/linux64

make -f libretro/Makefile.libretro \
    SRCROOT=. \
    OBJDIR=build/libretro/linux64 \
    LIBROOT="$LIBROOT" \
    TARGET_OS=linux \
    CXX=g++ \
    STRIP=strip \
    EXTRA_LIBS="-Wl,--start-group $ALL_LIBS -Wl,--end-group -Wl,-Bdynamic $SDL_LIBS $X11_LIBS -lpthread -ldl -lrt" \
    -j$JOBS

echo ""
echo "=== Build complete ==="
ls -la build/libretro/linux64/fmtowns_libretro.so
