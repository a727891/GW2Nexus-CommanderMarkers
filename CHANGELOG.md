# Changelog

All notable changes to the Nexus **Commander Markers** addon are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- Initial Nexus port of the BlishHUD Commander Markers module
- Clickable squad marker panel with ground/object placement via GW2 `GameBinds_*`
- AutoMarker library with map triggers, interact bind (`CM_INTERACT`, default `F`), and placement queue
- 3D billboards and screen-map overlay (trigger hearts + preview ghosts)
- Community marker library with `lastEdit` sync from `bhm.blishhud.com`
- Options UI (5 tabs): Clickable Markers, AutoMarker Settings, AutoMarker Library, Community Library, General
- Corner icon quick access and lieutenant mode toggle
- Embedded PNG textures (~33 assets) in the release DLL
- Identifiable `User-Agent` (`COMM-MARKERS-Nexus/1.0.0`) for HTTP requests

### Notes

- Squad marker keybinds are configured in GW2 Controls, not in addon options
- No Blish Keybinds options tab — Nexus uses `GameBinds_IsBound` with a warning banner when binds are missing
