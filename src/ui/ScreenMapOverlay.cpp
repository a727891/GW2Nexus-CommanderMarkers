#include "ui/ScreenMapOverlay.h"

#include "core/AppState.h"
#include "core/FeatureFlags.h"
#include "core/MumbleUtils.h"
#include "data/MapDataCache.h"
#include "services/MapWatchService.h"
#include "services/MarkerListing.h"
#include "ui/TextureService.h"
#include "ui/UiFontService.h"
#include "ui/UiLayer.h"
#include "utils/InteractBindService.h"

#include <cmath>
#include <imgui.h>
#include <vector>

namespace cm {
namespace ScreenMapOverlay {
namespace {

constexpr float kFloorFilterMeters = 30.0f;
constexpr float kTriggerMarkerIconSize = 32.0f;
constexpr float kPreviewMarkerIconSize = 32.0f;
constexpr float kProjectionDebugSquareSize = 24.0f;

constexpr ImGuiWindowFlags kMapOverlayFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
    ImGuiWindowFlags_NoBringToFrontOnFocus;

constexpr ImU32 kDebugOverlayFill = IM_COL32(255, 0, 255, 96);

struct MapOverlayWindow {
    int styleVars = 0;
    int styleColors = 0;

    bool Begin(const ScreenRect& bounds, bool debugBounds) {
        if (bounds.w <= 0 || bounds.h <= 0) {
            return false;
        }

        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(bounds.x), static_cast<float>(bounds.y)),
                                ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(static_cast<float>(bounds.w), static_cast<float>(bounds.h)), ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, debugBounds ? 2.0f : 0.0f);
        styleVars = 2;
        if (debugBounds) {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, kDebugOverlayFill);
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 255, 220));
            styleColors = 2;
        } else {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 0));
            styleColors = 1;
        }

        ImGuiWindowFlags flags = kMapOverlayFlags;
        if (!debugBounds) {
            flags |= ImGuiWindowFlags_NoBackground;
        }

        if (!ImGui::Begin("##cm_screen_map_overlay", nullptr, flags)) {
            End();
            return false;
        }
        SendWindowToDisplayBack();
        return true;
    }

    void End() {
        ImGui::End();
        if (styleColors > 0) {
            ImGui::PopStyleColor(styleColors);
            styleColors = 0;
        }
        if (styleVars > 0) {
            ImGui::PopStyleVar(styleVars);
            styleVars = 0;
        }
    }
};

bool ShouldRender(const AppState& state) {
    const bool debugMode =
        cm::features::kDebugMapOverlayBounds || cm::features::kDebugMapProjection;
    if (!state.settings.autoMarkerEnabled && !debugMode) {
        return false;
    }
    if (!IsInGame(state.mumbleLink, state.nexusLink)) {
        return false;
    }
    if (IsInCombat(state.mumbleLink) && !state.settings.combatPlacement) {
        return false;
    }
    if (!debugMode && (state.settings.autoMarkerOnlyCommander || state.ltMode)) {
        if (!HasCommanderPermissions(state.mumbleLink, state.playerIdentity) && !state.ltMode) {
            return false;
        }
    }
    if (!debugMode && !state.settings.autoMarkerShowTrigger &&
        !state.settings.autoMarkerShowPreview) {
        return false;
    }
    return true;
}

bool SameFloor(const Vec3f& player, const Vec3f& marker) {
    return std::fabs(player.z - marker.z) <= kFloorFilterMeters;
}

std::vector<MarkerSet> EnabledMarkersOnMap(const AppState& state, int mapId) {
    std::vector<MarkerSet> markers;
    for (const MarkerSet& markerSet : state.markerListing.GetMarkersForMap(mapId)) {
        if (markerSet.enabled) {
            markers.push_back(markerSet);
        }
    }
    return markers;
}

void DrawColoredDebugSquare(ImDrawList* draw, const Vec2f& screenPos, ImU32 fill, float size) {
    if (!draw) {
        return;
    }

    const float half = size * 0.5f;
    const ImVec2 p0(screenPos.x - half, screenPos.y - half);
    const ImVec2 p1(screenPos.x + half, screenPos.y + half);
    draw->AddRectFilled(p0, p1, fill);
    draw->AddRect(p0, p1, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
}

void DrawMarkerAt(ImDrawList* draw,
                  const Vec2f& screenPos,
                  ImTextureID texture,
                  float iconSize,
                  float alpha,
                  bool debugSquare) {
    if (!draw) {
        return;
    }

    const float half = iconSize * 0.5f;
    const ImVec2 p0(screenPos.x - half, screenPos.y - half);
    const ImVec2 p1(screenPos.x + half, screenPos.y + half);

    if (debugSquare) {
        draw->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255));
        draw->AddRect(p0, p1, IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.0f);
        return;
    }

    if (!texture) {
        return;
    }

    const ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(255.0f * alpha));
    draw->AddImage(texture, p0, p1, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
}

ImTextureID TriggerMarkerTexture() {
    return TextureService::GetTriggerMarkerTexture();
}

void DrawTriggerHearts(const AppState& state,
                       ImDrawList* draw,
                       const ScreenMapData& screenMap,
                       const Vec3f& player,
                       const std::vector<MarkerSet>& markers,
                       bool debugSquare) {
    if (!state.settings.autoMarkerShowTrigger) {
        return;
    }

    const ImTextureID triggerIcon = TriggerMarkerTexture();
    for (const MarkerSet& markerSet : markers) {
        const Vec3f trigger{markerSet.trigger.x, markerSet.trigger.y, markerSet.trigger.z};
        if (!debugSquare && !SameFloor(player, trigger)) {
            continue;
        }

        const Vec2f screenPos =
            MapDataCache::WorldToScreenMap(markerSet.mapId, trigger, screenMap, state.mapData);
        if (!debugSquare && !screenMap.screenBounds.Contains(screenPos.x, screenPos.y)) {
            continue;
        }
        DrawMarkerAt(draw, screenPos, triggerIcon, kTriggerMarkerIconSize, 1.0f, debugSquare);
    }
}

void DrawPreviewMarkers(const AppState& state,
                        ImDrawList* draw,
                        const ScreenMapData& screenMap,
                        const Vec3f& player,
                        bool debugSquare) {
    if (!state.settings.autoMarkerShowPreview) {
        return;
    }

    if (!state.mapWatch.ShouldShowMapPreview(state.mumbleLink)) {
        return;
    }

    const MarkerSet* preview = state.mapWatch.ActivePreview();
    if (!preview) {
        return;
    }

    for (const MarkerCoord& marker : preview->markers) {
        const Vec3f world{marker.x, marker.y, marker.z};
        if (!debugSquare && !SameFloor(player, world)) {
            continue;
        }

        const SquadMarker squadMarker =
            marker.icon > 0 && marker.icon <= 9 ? static_cast<SquadMarker>(marker.icon)
                                                : SquadMarker::Arrow;
        const ImTextureID texture = TextureService::GetTexture(squadMarker);
        const Vec2f screenPos =
            MapDataCache::WorldToScreenMap(preview->mapId, world, screenMap, state.mapData);
        if (!debugSquare && !screenMap.screenBounds.Contains(screenPos.x, screenPos.y)) {
            continue;
        }
        DrawMarkerAt(draw, screenPos, texture, kPreviewMarkerIconSize, 0.85f, debugSquare);
    }
}

void DrawInteractPrompt(AppState& state,
                        ImDrawList* draw,
                        const Vec3f& player,
                        const std::vector<MarkerSet>& markers) {
    if (!MapUiIsOpen(state.mumbleLink)) {
        return;
    }

    const MarkerSet* closest = nullptr;
    float closestDistance = MapWatchService::TRIGGER_DISTANCE_OPEN_MAP;
    for (const MarkerSet& markerSet : markers) {
        const Vec3f trigger{markerSet.trigger.x, markerSet.trigger.y, markerSet.trigger.z};
        const float dx = player.x - trigger.x;
        const float dy = player.y - trigger.y;
        const float dz = player.z - trigger.z;
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (distance < closestDistance) {
            closest = &markerSet;
            closestDistance = distance;
        }
    }

    if (!closest) {
        return;
    }

    if (!state.nexusLink || !draw) {
        return;
    }

    char prompt[256];
    std::snprintf(prompt, sizeof(prompt), "Press '%s' to place markers\n%s",
                  InteractBindService::GetPrimaryKeyLabel(), closest->name.c_str());

    const float scale = state.nexusLink->Scaling > 0.0f ? state.nexusLink->Scaling : 1.0f;
    ImFont* font = UiFontService::GetUiFont(state.nexusLink);
    const float fontSize = UiFontService::EffectiveFontSize(font, scale);
    const ImVec2 textSize = UiFontService::MeasureText(prompt, font, fontSize);

    const float centerX = static_cast<float>(state.nexusLink->Width) * 0.5f;
    const float bottomY = static_cast<float>(state.nexusLink->Height) - 120.0f * scale;
    const ImVec2 textPos(centerX - textSize.x * 0.5f, bottomY - textSize.y * 0.5f);
    UiFontService::DrawTextShadowed(draw, font, fontSize, textPos, IM_COL32(255, 165, 0, 255),
                                    prompt);
}

void DrawDebugOverlayFill(ImDrawList* draw, const ScreenRect& bounds) {
    if (!draw) {
        return;
    }

    const ImVec2 p0(static_cast<float>(bounds.x), static_cast<float>(bounds.y));
    const ImVec2 p1(static_cast<float>(bounds.x + bounds.w),
                      static_cast<float>(bounds.y + bounds.h));
    draw->AddRectFilled(p0, p1, kDebugOverlayFill);
    draw->AddRect(p0, p1, IM_COL32(255, 0, 255, 220), 0.0f, 0, 2.0f);
}

void DrawDebugOverlayLabel(ImDrawList* draw, const ScreenRect& bounds, int mapId,
                           std::size_t markerCount, bool hasGeometry) {
    if (!draw) {
        return;
    }

    char label[160];
    std::snprintf(label, sizeof(label), "CM overlay map=%d markers=%zu geom=%s bounds=%d,%d %dx%d",
                  mapId, markerCount, hasGeometry ? "yes" : "no", bounds.x, bounds.y, bounds.w,
                  bounds.h);
    draw->AddText(ImVec2(static_cast<float>(bounds.x) + 4.0f, static_cast<float>(bounds.y) + 4.0f),
                  IM_COL32(255, 255, 255, 255), label);
}

void DrawProjectionDebug(const AppState& state,
                         ImDrawList* draw,
                         const ScreenMapData& screenMap,
                         int mapId,
                         const Vec3f& player,
                         bool hasGeometry) {
    if (!draw || !state.mumbleLink) {
        return;
    }

    const Mumble::Compass& compass = state.mumbleLink->Context.Compass;
    const Vec2f mumbleContinent{compass.PlayerPosition.X, compass.PlayerPosition.Y};
    const Vec2f cyanScreen = MapDataCache::MapToScreenMap(mumbleContinent, screenMap);
    DrawColoredDebugSquare(draw, cyanScreen, IM_COL32(0, 220, 255, 255), kProjectionDebugSquareSize);

    Vec2f greenScreen{};
    Vec2f apiContinent{};
    bool hasApiContinent = false;
    if (hasGeometry) {
        const Gw2Map* map = state.mapData.GetMap(mapId);
        if (map) {
            apiContinent = MapDataCache::WorldMetersToMap(*map, player);
            hasApiContinent = true;
        }
        greenScreen = MapDataCache::WorldToScreenMap(mapId, player, screenMap, state.mapData);
        DrawColoredDebugSquare(draw, greenScreen, IM_COL32(0, 255, 0, 255), kProjectionDebugSquareSize);
    }

    const ScreenRect& bounds = screenMap.screenBounds;
    float textY = static_cast<float>(bounds.y + bounds.h) - 72.0f;
    const float textX = static_cast<float>(bounds.x) + 4.0f;
    const ImU32 textColor = IM_COL32(255, 255, 255, 255);

    draw->AddText(ImVec2(textX, textY), textColor,
                  "proj: green=API  cyan=mumble  orange=map content center");
    textY += 16.0f;

    if (hasApiContinent) {
        const float continentDx = apiContinent.x - mumbleContinent.x;
        const float continentDy = apiContinent.y - mumbleContinent.y;
        const float screenDx = greenScreen.x - cyanScreen.x;
        const float screenDy = greenScreen.y - cyanScreen.y;

        char deltaLine[192];
        std::snprintf(deltaLine, sizeof(deltaLine),
                      "continent dX=%.1f dY=%.1f  screen dX=%.1f dY=%.1f", continentDx,
                      continentDy, screenDx, screenDy);
        draw->AddText(ImVec2(textX, textY), textColor, deltaLine);
        textY += 16.0f;

        char coordLine[256];
        std::snprintf(coordLine, sizeof(coordLine),
                      "API(%.0f,%.0f) mumble(%.0f,%.0f) center(%.0f,%.0f) scale=%.4f "
                      "bCenter(%.0f,%.0f)",
                      apiContinent.x, apiContinent.y, mumbleContinent.x, mumbleContinent.y,
                      screenMap.mapCenter.x, screenMap.mapCenter.y, screenMap.scale,
                      screenMap.boundsCenter.x, screenMap.boundsCenter.y);
        draw->AddText(ImVec2(textX, textY), textColor, coordLine);
        textY += 16.0f;

        DrawColoredDebugSquare(draw, screenMap.boundsCenter, IM_COL32(255, 128, 0, 255), 16.0f);
    } else {
        draw->AddText(ImVec2(textX, textY), textColor,
                      "green square hidden: map geometry missing from cache");
    }
}

}  // namespace

void Render(AppState& state) {
    if (!ShouldRender(state) || !state.mumbleLink || !state.nexusLink) {
        return;
    }

    const bool debugBounds = cm::features::kDebugMapOverlayBounds;
    const bool debugProjection = cm::features::kDebugMapProjection;
    const bool debugMode = debugBounds || debugProjection;
    const int mapId = static_cast<int>(state.mumbleLink->Context.MapID);
    const std::vector<MarkerSet> markers = EnabledMarkersOnMap(state, mapId);
    const bool hasGeometry = state.mapData.MapHasGeometry(mapId);

    const ScreenMapData screenMap = BuildScreenMapData(state.mumbleLink, state.nexusLink);
    if (screenMap.screenBounds.w <= 0 || screenMap.screenBounds.h <= 0) {
        return;
    }

    if (!debugMode) {
        if (markers.empty() || !hasGeometry) {
            return;
        }
    }

    const Vec3f player = PlayerPosition(state.mumbleLink);

    MapOverlayWindow overlay;
    if (!overlay.Begin(screenMap.screenBounds, debugBounds)) {
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (debugBounds) {
        DrawDebugOverlayFill(draw, screenMap.screenBounds);
        DrawDebugOverlayLabel(draw, screenMap.screenBounds, mapId, markers.size(), hasGeometry);
    }

    if (debugProjection) {
        DrawProjectionDebug(state, draw, screenMap, mapId, player, hasGeometry);
    }

    if (!markers.empty() && hasGeometry) {
        DrawTriggerHearts(state, draw, screenMap, player, markers, debugBounds);
        DrawPreviewMarkers(state, draw, screenMap, player, debugBounds);
        DrawInteractPrompt(state, draw, player, markers);
    }
    overlay.End();
}

}  // namespace ScreenMapOverlay
}  // namespace cm
