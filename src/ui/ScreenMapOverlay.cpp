#include "ui/ScreenMapOverlay.h"

#include "core/AppState.h"
#include "core/MumbleUtils.h"
#include "data/MapDataCache.h"
#include "services/MapWatchService.h"
#include "ui/TextureService.h"

#include <cmath>
#include <imgui.h>

namespace cm {
namespace ScreenMapOverlay {
namespace {

constexpr float kFloorFilterMeters = 30.0f;
constexpr float kMarkerIconSize = 32.0f;

bool ShouldRender(const AppState& state) {
    if (!state.settings.autoMarkerEnabled) {
        return false;
    }
    if (!IsInGame(state.mumbleLink, state.nexusLink)) {
        return false;
    }
    if (IsInCombat(state.mumbleLink) && !state.settings.combatPlacement) {
        return false;
    }
    if (state.settings.autoMarkerOnlyCommander || state.ltMode) {
        if (!IsCommander(state.mumbleLink) && !state.ltMode) {
            return false;
        }
    }
    if (!state.settings.autoMarkerShowTrigger && !state.settings.autoMarkerShowPreview) {
        return false;
    }
    return true;
}

bool SameFloor(const Vec3f& player, const Vec3f& marker) {
    return std::fabs(player.z - marker.z) <= kFloorFilterMeters;
}

void DrawMarkerIcon(ImDrawList* draw, ImTextureID texture, const Vec2f& screenPos, float alpha) {
    if (!texture) {
        return;
    }

    const float half = kMarkerIconSize * 0.5f;
    const ImVec2 p0(screenPos.x - half, screenPos.y - half);
    const ImVec2 p1(screenPos.x + half, screenPos.y + half);
    const ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(255.0f * alpha));
    draw->AddImage(texture, p0, p1, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), tint);
}

void DrawTriggerHearts(AppState& state, const ScreenMapData& screenMap, const Vec3f& player) {
    if (!state.settings.autoMarkerShowTrigger) {
        return;
    }

    const ImTextureID heart = TextureService::GetUiTexture("heart");
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    for (const MarkerSet& markerSet : state.mapWatch.CurrentMapMarkers()) {
        const Vec3f trigger{markerSet.trigger.x, markerSet.trigger.y, markerSet.trigger.z};
        if (!SameFloor(player, trigger)) {
            continue;
        }

        const Vec2f screenPos =
            MapDataCache::WorldToScreenMap(markerSet.mapId, trigger, screenMap, state.mapData);
        if (!screenMap.screenBounds.Contains(screenPos.x, screenPos.y)) {
            continue;
        }
        DrawMarkerIcon(draw, heart, screenPos, 1.0f);
    }
}

void DrawPreviewMarkers(AppState& state, const ScreenMapData& screenMap, const Vec3f& player) {
    if (!state.settings.autoMarkerShowPreview) {
        return;
    }

    const MarkerSet* preview = state.mapWatch.ActivePreview();
    if (!preview) {
        return;
    }

    ImDrawList* draw = ImGui::GetForegroundDrawList();
    for (const MarkerCoord& marker : preview->markers) {
        const Vec3f world{marker.x, marker.y, marker.z};
        if (!SameFloor(player, world)) {
            continue;
        }

        const SquadMarker squadMarker =
            marker.icon > 0 && marker.icon <= 9 ? static_cast<SquadMarker>(marker.icon)
                                                : SquadMarker::Arrow;
        const ImTextureID texture = TextureService::GetTexture(squadMarker);
        const Vec2f screenPos =
            MapDataCache::WorldToScreenMap(preview->mapId, world, screenMap, state.mapData);
        if (!screenMap.screenBounds.Contains(screenPos.x, screenPos.y)) {
            continue;
        }
        DrawMarkerIcon(draw, texture, screenPos, 0.85f);
    }
}

void DrawInteractPrompt(AppState& state, const Vec3f& player) {
    if (!MapUiIsOpen(state.mumbleLink)) {
        return;
    }

    const MarkerSet* closest = nullptr;
    float closestDistance = MapWatchService::TRIGGER_DISTANCE_OPEN_MAP;
    for (const MarkerSet& markerSet : state.mapWatch.CurrentMapMarkers()) {
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
        state.mapWatch.RemovePreviewMarkerSet();
        return;
    }

    state.mapWatch.PreviewClosestMarkerSet(state.mumbleLink);

    if (!state.nexusLink) {
        return;
    }

    char prompt[256];
    std::snprintf(prompt, sizeof(prompt), "Press interact to place markers\n%s",
                  closest->name.c_str());

    const float centerX = static_cast<float>(state.nexusLink->Width) * 0.5f;
    const float bottomY = static_cast<float>(state.nexusLink->Height) - 120.0f;
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    const ImVec2 textSize = ImGui::CalcTextSize(prompt);
    const ImVec2 textPos(centerX - textSize.x * 0.5f, bottomY);
    draw->AddText(ImVec2(textPos.x + 2.0f, textPos.y + 2.0f), IM_COL32(0, 0, 0, 255), prompt);
    draw->AddText(textPos, IM_COL32(255, 165, 0, 255), prompt);
}

}  // namespace

void Render(AppState& state) {
    if (!ShouldRender(state) || !state.mumbleLink || !state.nexusLink) {
        return;
    }

    const ScreenMapData screenMap = BuildScreenMapData(state.mumbleLink, state.nexusLink);
    const Vec3f player = PlayerPosition(state.mumbleLink);

    DrawTriggerHearts(state, screenMap, player);
    DrawPreviewMarkers(state, screenMap, player);
    DrawInteractPrompt(state, player);
}

}  // namespace ScreenMapOverlay
}  // namespace cm
