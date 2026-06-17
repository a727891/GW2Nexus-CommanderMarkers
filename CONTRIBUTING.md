# Contributing

Thanks for helping maintain **Commander Markers** for Nexus. This addon is a port of the [BlishHUD Commander Markers](https://github.com/manlaan/BlishHud-CommanderMarkers) module. Community marker sets are shared between both clients via JSON hosted by Freesnöw.

## Development setup

### Requirements

- Linux with MinGW-w64 cross-compiler (or native Windows with MSVC/MinGW)
- CMake 3.20+, Ninja, Python 3
- Git

Fedora/Nobara toolchain:

```bash
sudo dnf install mingw64-gcc-c++ mingw64-winpthreads-static cmake ninja git python3
```

### Build

```bash
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Output: `build/NexusCommanderMarkers.dll`

Release export (stripped, embedded textures):

```bash
./scripts/build-release.sh
```

### Deploy locally

```bash
./scripts/deploy-to-gw2.sh --release
./scripts/deploy-to-gw2.sh --release --ftue   # clear cache to test first-load community sync
```

See [README.md](README.md) for details.

### Project layout

| Path | Purpose |
|------|---------|
| `src/core/` | Settings, app state, mumble helpers, branding |
| `src/data/` | JSON models, map cache, static loader |
| `src/services/` | HTTP, community sync, marker listing, placement, map watch, optional RTAPI |
| `src/ui/` | Settings window, marker set editor, library/community UI, overlays, in-game world markers, corner icon |
| `src/utils/` | GameBind placement, Win32 cursor helpers |
| `assets/textures/` | PNG sources embedded at build time |
| `assets/data/` | Default local marker library seed |
| `extern/nexus/` | Nexus addon API headers |
| `extern/rtapi/` | Real-Time API header (`RTAPI.h`) for optional squad role integration |
| `docs/` | Maintainer handoffs (`DEBUGGING.md`, `MUMBLE_HANDOFF.md`, port plan) |
| `cmake/` | MinGW cross-compile toolchain |

Key UI entry points: `SettingsWindow.cpp` (owned options window), `MarkerSetEditorWindow.cpp` (library editor), `OptionsPanel.cpp` (tab content), `LibraryEditorUi.cpp` / `CommunityLibraryUi.cpp` (library tabs), `LibrarySearchUi.h` (shared search filter).

---

## Maintaining community marker data

Most community updates **do not require a Nexus code change**. Marker sets are loaded from static JSON at runtime and cached under `<GW2>/addons/NexusCommanderMarkers/commanderMarkers/`.

### Repositories involved

| Repo | Role |
|------|------|
| [BlishHud-CommanderMarkers](https://github.com/manlaan/BlishHud-CommanderMarkers) | Primary module; reference for behavior |
| `bhud-static/Manlaan.CommanderMarkers` branch | Community `Markers.json` (local clone often named `BlishHud-CommanderMarkersStatic`) |
| **This repo** (`NexusCommanderMarkers`) | Nexus port; consumes the same community JSON |

Community data is served from:

`https://bhm.blishhud.com/Manlaan.CommanderMarkers/Community/Markers.json`

HTTP requests send `User-Agent: COMM-MARKERS-Nexus/<version>`.

### Sync behavior

1. On boot, the addon loads cached `commanderMarkers/Markers.json` if present.
2. A background sync fetches the remote JSON.
3. If remote `lastEdit` matches cached `community_meta.json`, no re-download occurs (small HTTP body only).
4. If `lastEdit` changed, the full JSON is written to cache and the in-memory library updates.

### Adding or updating community marker sets

1. Clone the static branch or fork [BlishHud-CommanderMarkers](https://github.com/manlaan/BlishHud-CommanderMarkers) and check out `bhud-static/Manlaan.CommanderMarkers`.
2. Edit `Community/Markers.json`:
   - Bump `lastEdit` to a new ISO-8601 timestamp (required for clients to pick up changes).
   - Add marker sets under the appropriate category in `categories[].markerSets[]`.
3. Each marker set needs at minimum: `name`, `mapId`, `trigger` `{x,y,z}`, and `markers[]` with `i` (squad marker index 1–8), `x`, `y`, `z`, optional `d` (description).
4. Open a pull request against the static branch. BlishHUD and Nexus clients will sync after merge and deploy to `bhm.blishhud.com`.

Use the Blish module's AutoMarker Library editor or existing entries in `Markers.json` as schema reference.

### Local default library

The built-in default presets (`assets/data/default_markers.json`) seed the user's local AutoMarker library on first run. Changing defaults requires a Nexus release; they are not fetched from the static host.

---

## Code changes

When behavior differs from Blish, treat the C# module as the source of truth and mirror logic in C++ with Nexus APIs (`GameBinds_*`, ImGui overlays, mumble identity for commander checks). When [Real-Time API](https://raidcore.gg/gw2/nexus) is installed and active, `RtApiService` and `HasCommanderPermissions()` also honor squad commander/lieutenant roles.

Keybind policy for Nexus:

- **Do not** add squad marker keybind UI — GW2 Controls owns those binds.
- Use `GameBinds_IsBound` / `CheckRequiredBinds()` and show a warning when binds are missing.
- AutoMarker interact uses Nexus bind `CM_INTERACT` (registered in `entry.cpp`).
