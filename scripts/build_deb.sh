#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="1.0.0"
PKG="obs-scripture-dock"
STAGE="$REPO_DIR/dist/deb_stage"
OUT_DEB="$REPO_DIR/dist/${PKG}_${VERSION}_amd64.deb"
OUT_TAR="$REPO_DIR/dist/${PKG}-${VERSION}-linux-x86_64.tar.gz"

cmake -B "$REPO_DIR/build" -S "$REPO_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$REPO_DIR/build" -j"$(nproc)"

rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" "$STAGE/usr/lib/x86_64-linux-gnu/obs-plugins"
cp "$REPO_DIR/build/obs-scripture-dock.so" "$STAGE/usr/lib/x86_64-linux-gnu/obs-plugins/obs-scripture-dock.so"

cat > "$STAGE/DEBIAN/control" <<EOF
Package: $PKG
Version: $VERSION
Section: video
Priority: optional
Architecture: amd64
Maintainer: Philippe Demartin <phdemartin@gmail.com>
Depends: obs-studio (>= 30.0), libsword1.9.0t64 | libsword1.9.0 | libsword11v5, libqt6core6t64 | libqt6core6, libqt6gui6t64 | libqt6gui6, libqt6widgets6t64 | libqt6widgets6, curl, unzip
Description: Native OBS Studio Scripture Dock & True-Alpha Verse Overlay
 Replaces browser-based OBS Bible plugins without needing obs-browser,
 Chrome windows, Electron servers, or Color Key filters. Provides a
 native Qt6 Dock inside OBS Studio with SWORD (~/.sword) and .js/.json
 Bible support, online CrossWire Bible downloader, preaching playlist,
 and a true-alpha BGRA video overlay.
EOF

chmod 755 "$STAGE/DEBIAN"
dpkg-deb --build --root-owner-group "$STAGE" "$OUT_DEB"

TAR_STAGE="$REPO_DIR/dist/tar_stage"
rm -rf "$TAR_STAGE"
mkdir -p "$TAR_STAGE/obs-scripture-dock/bin/64bit"
cp "$REPO_DIR/build/obs-scripture-dock.so" "$TAR_STAGE/obs-scripture-dock/bin/64bit/obs-scripture-dock.so"
tar -C "$TAR_STAGE" -czf "$OUT_TAR" obs-scripture-dock

echo "Paquet .deb : $OUT_DEB"
echo "Archive portable : $OUT_TAR"
