#!/usr/bin/env bash
set -euo pipefail

# Build the Trout menu-bar app and input method bundle.
# Usage: scripts/build.sh [Debug|Release] [--no-llama]

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_TYPE="${1:-Release}"
LLAMA=ON
for arg in "$@"; do
    [ "$arg" = "--no-llama" ] && LLAMA=OFF
done

cmake -S "$ROOT" -B "$ROOT/build" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DTROUT_WITH_LLAMA="$LLAMA" \
    -G "Unix Makefiles"

cmake --build "$ROOT/build" -j"$(sysctl -n hw.ncpu)"

echo
echo "Built: $ROOT/build/platform/macos/Trout.app"
echo "Built: $ROOT/build/inputmethod/TroutIM.app"
