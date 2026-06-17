# Changelog

All notable changes to the Nexus **Commander Markers** addon are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Initial Nexus port of the BlishHUD Commander Markers module
- Clickable squad marker panel with ground/object placement via GW2 `GameBinds_*`
- AutoMarker library with map triggers, interact bind (`CM_INTERACT`, default `F`), and placement queue
- In-game world markers (`BillboardRenderer`) and screen-map overlay (trigger hearts + preview ghosts)
- Community marker library with `lastEdit` sync from `bhm.blishhud.com`
- Dedicated in-game settings window (six tabs): Clickable Markers, AutoMarker Settings, AutoMarker Library, Community Library, General, About
- Marker set editor window (local library): per-slot editing, Base64 import/export, RTAPI squad-marker import when Real-Time API is active
- Library and community search filters (name, description, map name)
- Community **Only Show available** filter (hide sets already in local library)
- Nexus `GUI_SendAlert` feedback on successful community import
- Optional Real-Time API integration: squad commander/lieutenant permission checks; RTAPI status on About tab
- Corner icon quick access and lieutenant mode toggle
- Embedded PNG textures (~33 assets) in the release DLL
- Identifiable `User-Agent` (`COMM-MARKERS-Nexus/1.0.0`) for HTTP requests

### Changed

- Nexus addon config page shows a launcher button only; full options UI lives in the owned settings window
- AutoMarker settings use Blish-aligned labels (Map Icons / In Game World Icons sections)
- Local library UI: icon action buttons, enable/disable on the right, search on Add New row, pale highlight for disabled sets
- Community library: fixed import column, redownload on category row, duplicate import disabled via `ContainsMarkerSet`
- Editor and library flows: input hints on empty fields, delete/unsaved confirmations (Shift+click skips confirm dialogs)

### Notes

- Squad marker keybinds are configured in GW2 Controls, not in addon options
- No Blish Keybinds options tab - Nexus uses `GameBinds_IsBound` with a warning banner when binds are missing
- Commander permission checks prefer RTAPI squad roles when Real-Time API is active; otherwise mumble commander tag / identity JSON
