#!/bin/sh
#
# build.sh
#
# Ad hoc build script for the Emu28 macOS/Linux port (see CLAUDE.md).
# Not a real build system - CLAUDE.md's own "Not yet decided" list
# still calls for a proper CMake setup; this is just the compile+link
# recipe developed during milestones 2-10, made runnable in one step
# instead of typed out by hand. Objects land in build/ (gitignored).
#
# Usage: ./build.sh && ./build/emu28

set -e

cd "$(dirname "$0")"

VENDOR=vendor/emu28-upstream
SHIM=shim
BUILD=build

SDL_CFLAGS=$(pkg-config --cflags sdl2)
SDL_LIBS=$(pkg-config --libs sdl2)

mkdir -p "$BUILD"

# clang treats upper-case .C as C++ by default (-x c forces C); every
# vendor file needs shim/compat on the include path ahead of
# vendor/emu28-upstream so PCH.H's angle-bracket <windows.h> etc.
# resolve to the shim instead of a real Windows SDK - see CLAUDE.md's
# "compat/" note for why quote-form #include "pch.h" still finds
# vendor/'s own file regardless.
CFLAGS="-x c -I $SHIM/compat -I $SHIM -I $VENDOR $SDL_CFLAGS"

# Every vendor file except EMU28.C (superseded entirely by
# shim/sdl_main.c - see CLAUDE.md's milestone 4 notes) and the
# DirectSound/waveOut/STEGANO.C trio (deliberately out of scope - see
# CLAUDE.md's milestones 5 and 6).
VENDOR_FILES="CURSOR DDESERV DEBUGGER DISASM DISMEM DISPLAY DISPNUM \
DISRPL ENGINE FETCH FILES KEYBOARD KEYMACRO KML MOPS MRU OPCODES \
PNGCRC REDEYE RPL SETTINGS STACK SYMBFILE TIMER UDP"

echo "Compiling vendor sources..."
for f in $VENDOR_FILES; do
	clang $CFLAGS -c "$VENDOR/$f.C" -o "$BUILD/$(echo "$f" | tr 'A-Z' 'a-z').o"
done

# LODEPNG.C needs -DLODEPNG_NO_COMPILE_CRC: PNGCRC.C provides a faster
# lodepng_crc32 that Emu28's own LODEPNG.H expects to be linked in
# externally under this flag - without it, LODEPNG.C's own built-in
# lodepng_crc32 collides with PNGCRC.C's at link time. See CLAUDE.md's
# milestone 8 notes.
clang $CFLAGS -DLODEPNG_NO_COMPILE_CRC -c "$VENDOR/LODEPNG.C" -o "$BUILD/lodepng.o"

echo "Compiling shim..."
for f in gdi win32_handle ini_file sound_stub stegano_stub win32_ui_stub sdl_main; do
	clang $CFLAGS -c "$SHIM/$f.c" -o "$BUILD/$f.o"
done

echo "Linking..."
clang "$BUILD"/*.o $SDL_LIBS -o "$BUILD/emu28"

echo "Built $BUILD/emu28"
