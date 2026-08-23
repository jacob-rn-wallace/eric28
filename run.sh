#!/bin/sh
#
# run.sh
#
# Builds (via build.sh) and then launches the emulator - see CLAUDE.md
# and README.md for what this actually does under the hood. Exists
# separately from build.sh so double-clicking gets a "it just opens"
# experience without changing build.sh's own documented "build only"
# behavior (README.md and CLAUDE.md both describe `./build.sh &&
# ./build/emu28` as the two explicit steps).

set -e

cd "$(dirname "$0")"

./build.sh
exec ./build/emu28
