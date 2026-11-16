#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PLUGIN_DIR="$HOME/.config/obs-studio/plugins/obs-scripture-dock/bin/64bit"

cmake -B "$REPO_DIR/build" -S "$REPO_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$REPO_DIR/build" -j"$(nproc)"

mkdir -p "$PLUGIN_DIR"
cp "$REPO_DIR/build/obs-scripture-dock.so" "$PLUGIN_DIR/obs-scripture-dock.so"
echo "Plugin installé dans : $PLUGIN_DIR/obs-scripture-dock.so"
echo "Redémarrez OBS Studio :"
echo "  1. Ouvrez le dock : Docks -> Scripture Dock"
echo "  2. Ajoutez la source : Sources (+) -> Bible Verse Overlay"
