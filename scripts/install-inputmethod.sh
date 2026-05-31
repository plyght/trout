#!/usr/bin/env bash
set -euo pipefail

# Install the Trout input method into ~/Library/Input Methods and register it.
# After running, add "Trout" under System Settings > Keyboard > Input Sources.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/build/inputmethod/TroutIM.app"
DEST="$HOME/Library/Input Methods/TroutIM.app"

if [ ! -d "$SRC" ]; then
    echo "TroutIM.app not found. Build first: scripts/build.sh" >&2
    exit 1
fi

mkdir -p "$HOME/Library/Input Methods"
rm -rf "$DEST"
cp -R "$SRC" "$DEST"

# Register so it appears in Input Sources without a logout.
"/System/Library/Frameworks/Carbon.framework/Versions/A/Support/registerInputSource" "$DEST" 2>/dev/null || true

echo "Installed: $DEST"
echo "Now open System Settings > Keyboard > Input Sources and add 'Trout'."
