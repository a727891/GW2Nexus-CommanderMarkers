#pragma once

#include <optional>
#include <string>
#include <vector>

namespace cm {

enum class SquadMarker : int {
    None = 0,
    Arrow = 1,
    Circle = 2,
    Heart = 3,
    Square = 4,
    Star = 5,
    Spiral = 6,
    Triangle = 7,
    Cross = 8,
    Clear = 9,
};

enum class PanelLayout : int {
    Vertical = 0,
    Horizontal = 1,
};

enum class CornerIconAction : int {
    ShowIconMenu = 0,
    ShowSettings = 1,
    Library = 2,
    Lieutenant = 3,
    ClickMarkerToggle = 4,
};

struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ScreenRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    bool Contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

struct WorldCoord {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct MarkerCoord : WorldCoord {
    int icon = 0;
    std::string name;
};

struct MarkerSet {
    std::string name;
    std::string description;
    int mapId = 0;
    WorldCoord trigger{};
    std::vector<MarkerCoord> markers;
    bool enabled = true;
};

struct CommunityMarkerSet : MarkerSet {
    std::string author = "BlishHud Community";
};

struct CommunityCategory {
    std::string categoryName = "Community Created";
    std::vector<CommunityMarkerSet> markerSets;
};

struct CommunitySets {
    std::string lastEdit;
    std::vector<CommunityCategory> categories;
};

struct MarkerListingFile {
    std::string version = "2.0.0";
    std::vector<MarkerSet> squadMarkerPreset;
};

struct ScreenMapData {
    Vec2f mapCenter{};
    float mapRotation = 0.0f;
    float scale = 1.0f;
    ScreenRect screenBounds{};
    Vec2f boundsCenter{};
};

}  // namespace cm
