#pragma once

#include "core/Types.h"

#include <string>

namespace cm {

struct SettingsStore {
    // Clickable panel
    bool showMarkerPanel = true;
    bool onlyCommanderPanel = false;
    bool dragPanel = false;
    Vec2f panelLocation{100.0f, 100.0f};
    PanelLayout panelOrientation = PanelLayout::Horizontal;
    int iconWidth = 30;
    float panelOpacity = 1.0f;
    bool groundMarkersEnabled = true;
    bool objectMarkersEnabled = true;

    // AutoMarker
    bool autoMarkerEnabled = true;
    bool autoMarkerOnlyCommander = true;
    bool autoMarkerShowPreview = true;
    bool autoMarkerShowTrigger = true;
    bool billboardEnabled = true;
    bool billboardPlacement = true;
    bool billboardPreview = true;
    bool combatPlacement = false;
    int placementDelayMs = 100;
    bool libraryFilterCurrentMap = false;

    // Corner icon
    bool cornerIconEnabled = true;
    CornerIconAction cornerIconAction = CornerIconAction::ShowIconMenu;
    SquadMarker cornerTexture = SquadMarker::Heart;
    int cornerPriority = 478;

    void Load(const std::string& filePath);
    void Save(const std::string& filePath) const;
};

}  // namespace cm
