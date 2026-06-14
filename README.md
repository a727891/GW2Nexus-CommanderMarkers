# Commander Markers (Nexus)

Nexus addon port of the [BlishHUD Commander Markers](https://github.com/manlaan/BlishHud-CommanderMarkers) module.

Place squad markers with a click, or auto-place saved marker sets near triggers. Includes a community marker library synced from the Blish static host.

## Features

- **Clickable markers panel** — ground markers (select icon, click world/minimap) and object markers (instant placement)
- **AutoMarker** — saved marker sets with map triggers, interact placement, and 3D billboards when the map is closed
- **Community library** — browse and import marker sets from shared JSON
- **Local library** — create, edit, enable/disable, import (Base64), export, and reset presets
- **Corner icon** — quick access to settings, library, and lieutenant mode toggle
- **Quick access keybind** — `CM_INTERACT` (default `F`) for AutoMarker interact placement

Squad marker placement uses **Guild Wars 2 in-game keybinds** (via Nexus `GameBinds_*`). This addon does not provide a keybind configuration UI — assign squad marker binds in GW2 Controls. If required binds are missing, a warning is shown and placement is blocked.

## Requirements

- [Nexus](https://raidcore.gg/gw2/nexus) in Guild Wars 2
- Windows x64 at runtime (DLL is cross-compiled from Linux)
- GW2 squad marker keybinds assigned in Controls (ground and object markers 1–8, clear)

## Installation

1. Copy `NexusCommanderMarkers.dll` to `<GW2>/addons/` (directly in `addons/`, not a subfolder)
2. Launch GW2 with Nexus enabled
3. Enable **Commander Markers** in Nexus addon settings
4. Assign squad marker keybinds in GW2 if you have not already

Textures and the corner icon are **embedded in the DLL** at build time. End users only need the single DLL file.

On first run the addon downloads the community marker library into
`<GW2>/addons/NexusCommanderMarkers/commanderMarkers/`. A local marker library is seeded from built-in defaults when no saved library exists.

## Build & deploy

### Toolchain (Fedora/Nobara)

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

Stripped release export (~5 MB):

```bash
./scripts/build-release.sh
```

Output: `dist/NexusCommanderMarkers.dll`

### Deploy to local GW2

```bash
./scripts/deploy-to-gw2.sh --release           # stripped DLL + default marker seed
./scripts/deploy-to-gw2.sh --release --ftue      # stripped DLL, clear cache (test first-load sync)
```

### Nexus does not list the DLL

Rebuild with the provided `CMakeLists.txt` (static MinGW runtime). The DLL should only import Windows system libraries:

```bash
x86_64-w64-mingw32-objdump -p dist/NexusCommanderMarkers.dll | rg "DLL Name"
```

Expect `KERNEL32.dll`, `USER32.dll`, `WINHTTP.dll`, `msvcrt.dll` — **not**
`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, or `libwinpthread-1.dll`.

Also confirm:

- Filename is `NexusCommanderMarkers.dll` (not `libNexusCommanderMarkers.dll`)
- File is x64 and sits directly in `addons/`

## Manual test checklist

- [ ] Addon loads without Nexus errors
- [ ] Clickable panel: ground select+click, object instant, commander/LtMode gates
- [ ] AutoMarker: near trigger → interact key → full set placed with delays
- [ ] Map closed: 2m trigger bubble + 3D billboards visible
- [ ] Corner icon: configured left-click actions work
- [ ] Library CRUD: edit, enable/disable, import Base64, export, nuclear reset (Ctrl+Shift)
- [ ] Community library loads; second launch only checks `lastEdit` (~1 KB if unchanged)
- [ ] Windowed mode: marker placement not offset (Win32 client origin fix)
- [ ] Unbound squad marker binds: warning shown; placement blocked until GW2 binds assigned

## Static data

Community marker sets are fetched from:

`https://bhm.blishhud.com/Manlaan.CommanderMarkers/Community/Markers.json`

Some UI icons may be loaded from `https://assets.gw2dat.com/`. Both hosts receive an identifiable `User-Agent: COMM-MARKERS-Nexus/<version>`.

To add or update community marker sets, see **[CONTRIBUTING.md](CONTRIBUTING.md)**.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for build notes, project layout, and the community JSON PR workflow.

## Changelog

See [CHANGELOG.md](CHANGELOG.md).

## Credits

- **Manlaan** — Commander Markers (BlishHUD)
- **Soeed** — Nexus port
- **freesnow** — static data hosting (`bhm.blishhud.com`, `assets.gw2dat.com`)
- **Raidcore** — Nexus addon platform

## Links

- [BlishHUD Commander Markers](https://github.com/manlaan/BlishHud-CommanderMarkers)
- [Community static branch](https://github.com/manlaan/BlishHud-CommanderMarkers/tree/bhud-static/Manlaan.CommanderMarkers)
- [Patch notes](https://pkgs.blishhud.com/Manlaan.CommanderMarkers.html)
- [Nexus](https://raidcore.gg/gw2/nexus)

## License

MIT — see [LICENSE](LICENSE). Nexus API headers: Raidcore.GG MIT license.
