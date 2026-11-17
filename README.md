# OBS Scripture Dock

Native **OBS Studio** C++ / Qt6 plugin (`obs-scripture-dock`) providing a built-in **Scripture Dock** and a **true-alpha Bible Verse Overlay** source — **without requiring `obs-browser`, Custom Browser Docks, Chrome windows, Electron servers, or Color Key filters**.

Designed for preaching and live church streams on Linux (especially Ubuntu/Debian where `obs-studio` omits `obs-browser`).

---

## Features

- **Native Qt6 Dock inside OBS Studio** (`Docks → Scripture Dock`):
  - **SWORD Modules (`~/.sword`, `/usr/share/sword`)** natively supported via `libsword` (automatic UTF-8 / Latin-1 detection + localized French/Spanish/English book names)
  - **Built-in Online Bible Downloader (`🌐`)**: download free CrossWire SWORD Bibles in any language directly from inside OBS
  - **ObsBiblePlugin `.js` & `.json` Bibles** also supported (`📁` folder picker)
  - **Instant Search** by reference range (e.g. `Galates 3:1-6`, `Jean 1:3`, `Sal 23`) or keyword
  - **Preaching Playlist (`Liste`)**: add entire verse ranges in 1 click, grouped by passage title, reorder (`▲`/`▼`), persistent across reboots (`QSettings`)
  - **Compact Live Bar**: `◀` | **`Afficher` / `Masquer`** (single toggle) | `▶`, plus double-click to toggle Show/Hide
  - **Live Styling (`Style`)**: Position (*Bottom / Center / Top*), Font, Size, Text/Background Color, Background Opacity
- **True-Alpha Video Source** (`Sources → + → Bible Verse Overlay`):
  - Renders crisp antialiased text with `QPainter` directly onto `VIDEO_FORMAT_BGRA` with real alpha transparency
  - Instantaneous wakeup (`std::condition_variable`) and `0%` CPU when idle

---

## Installation

### Portable Archive (Any Linux x86_64) — Just Extract!
Download `obs-scripture-dock-1.0.0-linux-x86_64.tar.gz` from [Releases](https://github.com/phdemartin/obs-scripture-dock/releases):
```bash
mkdir -p ~/.config/obs-studio/plugins
tar -xzf obs-scripture-dock-1.0.0-linux-x86_64.tar.gz -C ~/.config/obs-studio/plugins/
```
*(Requires `libsword`, `curl`, and `unzip` installed on your distro.)*

### Debian / Ubuntu `.deb` Package
```bash
sudo apt install ./obs-scripture-dock_1.0.0_amd64.deb
```

### Windows (64-bit) — Installer (Recommended) or Portable ZIP
1. **Installer (`.exe`):** Download `obs-scripture-dock-windows-x64-setup.exe` from [Releases](https://github.com/phdemartin/obs-scripture-dock/releases) and run it (auto-detects your OBS Studio installation folder; `libsword` is statically bundled).
2. **Portable Archive (`.zip`):** Download `obs-scripture-dock-windows-x64.zip` and copy `obs-scripture-dock.dll` into `C:\Program Files\obs-studio\obs-plugins\64bit\`.

### macOS (Universal: Apple Silicon & Intel) — `.pkg` Installer or `.zip`
1. **Installer (`.pkg`):** Download `obs-scripture-dock-macos-universal.pkg` from [Releases](https://github.com/phdemartin/obs-scripture-dock/releases) and open it to install `obs-scripture-dock.plugin` into `/Library/Application Support/obs-studio/plugins/` (`libsword` is statically bundled).
2. **Portable Archive (`.zip`):** Download `obs-scripture-dock-macos-universal.zip`, extract `obs-scripture-dock.plugin`, and move it to `~/Library/Application Support/obs-studio/plugins/`.

### Build from Source (Linux / Cross-platform CMake)
```bash
sudo apt install cmake g++ pkg-config libobs-dev libsword-dev qt6-base-dev
./scripts/install_local.sh
```
