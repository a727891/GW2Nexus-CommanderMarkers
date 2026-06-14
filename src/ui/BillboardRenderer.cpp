#include "ui/BillboardRenderer.h"

#include "core/AppState.h"
#include "core/MumbleUtils.h"
#include "services/MapWatchService.h"
#include "ui/DatAssetIconService.h"
#include "ui/TextureService.h"

#include <cmath>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <windows.h>

namespace cm {
namespace BillboardRenderer {
namespace {

constexpr float kFloorFilterMeters = 30.0f;
constexpr float kMarkerIconSize = 32.0f;

struct CameraVectors {
    Vec3f position{};
    Vec3f front{};
    Vec3f top{};
    Vec3f right{};
    float fovRadians = 1.0f;
};

Vec3f Normalize(const Vec3f& v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len <= 0.0001f) {
        return {};
    }
    return {v.x / len, v.y / len, v.z / len};
}

Vec3f Cross(const Vec3f& a, const Vec3f& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float Dot(const Vec3f& a, const Vec3f& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

CameraVectors BuildCamera(const Mumble::Data* mumble) {
    CameraVectors camera{};
    if (!mumble) {
        return camera;
    }

    camera.position = {mumble->CameraPosition.X, mumble->CameraPosition.Y,
                       mumble->CameraPosition.Z};
    camera.front = Normalize({mumble->CameraFront.X, mumble->CameraFront.Y, mumble->CameraFront.Z});
    camera.top = Normalize({mumble->CameraTop.X, mumble->CameraTop.Y, mumble->CameraTop.Z});
    camera.right = Normalize(Cross(camera.front, camera.top));

    camera.fovRadians = 1.0472f;
    try {
        const std::wstring identityWide(mumble->Identity);
        const int size = WideCharToMultiByte(CP_UTF8, 0, identityWide.c_str(), -1, nullptr, 0,
                                             nullptr, nullptr);
        if (size > 1) {
            std::string identity(static_cast<size_t>(size - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, identityWide.c_str(), -1, identity.data(), size,
                                nullptr, nullptr);
            const auto j = nlohmann::json::parse(identity);
            const float fovDegrees = j.value("fov", 0.0f);
            if (fovDegrees > 1.0f) {
                camera.fovRadians = fovDegrees * 3.14159265f / 180.0f;
            }
        }
    } catch (...) {
    }

    return camera;
}

bool WorldToScreen(const Vec3f& world, const CameraVectors& camera, const NexusLinkData_t* nexus,
                   Vec2f& out) {
    if (!nexus || nexus->Width == 0 || nexus->Height == 0) {
        return false;
    }

    const Vec3f delta = {world.x - camera.position.x, world.y - camera.position.y,
                         world.z - camera.position.z};
    const float depth = Dot(delta, camera.front);
    if (depth <= 0.05f) {
        return false;
    }

    const float x = Dot(delta, camera.right);
    const float y = Dot(delta, camera.top);
    const float aspect =
        static_cast<float>(nexus->Width) / static_cast<float>(nexus->Height > 0 ? nexus->Height : 1);
    const float tanHalfFov = std::tan(camera.fovRadians * 0.5f);

    const float ndcX = x / (depth * tanHalfFov * aspect);
    const float ndcY = -y / (depth * tanHalfFov);
    if (ndcX < -1.5f || ndcX > 1.5f || ndcY < -1.5f || ndcY > 1.5f) {
        return false;
    }

    out.x = (ndcX + 1.0f) * 0.5f * static_cast<float>(nexus->Width);
    out.y = (ndcY + 1.0f) * 0.5f * static_cast<float>(nexus->Height);
    return true;
}

bool ShouldRender(const AppState& state) {
    if (!state.settings.autoMarkerEnabled || !state.settings.billboardEnabled) {
        return false;
    }
    if (!IsInGame(state.mumbleLink, state.nexusLink)) {
        return false;
    }
    if (MapUiIsOpen(state.mumbleLink)) {
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
    return true;
}

bool SameFloor(const Vec3f& player, const Vec3f& marker) {
    return std::fabs(player.z - marker.z) <= kFloorFilterMeters;
}

float Distance(const Vec3f& a, const Vec3f& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
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

void DrawInteractBubble(AppState& state, const CameraVectors& camera, const Vec3f& player,
                        const MarkerSet& markerSet) {
    if (!state.settings.billboardPlacement) {
        return;
    }

    const Vec3f trigger{markerSet.trigger.x, markerSet.trigger.y, markerSet.trigger.z};
    if (Distance(player, trigger) >= MapWatchService::TRIGGER_DISTANCE_CLOSED_MAP) {
        return;
    }

    if (!state.nexusLink) {
        return;
    }

    const ImTextureID bubble =
        DatAssetIconService::Request(DatAssetIconService::kInteractBubbleAssetId);
    DatAssetIconService::ProcessDownloads();

    const float centerX = static_cast<float>(state.nexusLink->Width) * 0.5f + 150.0f;
    const float centerY = static_cast<float>(state.nexusLink->Height) * 0.5f + 120.0f;
    const ImVec2 p0(centerX - 150.0f, centerY - 75.0f);
    const ImVec2 p1(centerX + 150.0f, centerY + 75.0f);
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    if (bubble) {
        draw->AddImage(bubble, p0, p1);
    } else {
        draw->AddRectFilled(p0, p1, IM_COL32(40, 40, 40, 200), 8.0f);
    }

    char prompt[256];
    std::snprintf(prompt, sizeof(prompt), "Press interact to place markers\n%s",
                  markerSet.name.c_str());
    const ImVec2 textPos(centerX - 80.0f, centerY - 40.0f);
    draw->AddText(ImVec2(textPos.x + 1.0f, textPos.y + 1.0f), IM_COL32(0, 0, 0, 255), prompt);
    draw->AddText(textPos, IM_COL32(255, 255, 255, 255), prompt);
    (void)camera;
}

}  // namespace

void Render(AppState& state) {
    if (!ShouldRender(state) || !state.mumbleLink || !state.nexusLink) {
        return;
    }

    const CameraVectors camera = BuildCamera(state.mumbleLink);
    const Vec3f player = PlayerPosition(state.mumbleLink);
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    const ImTextureID heart = TextureService::GetUiTexture("heart");

    for (const MarkerSet& markerSet : state.mapWatch.CurrentMapMarkers()) {
        const Vec3f trigger{markerSet.trigger.x, markerSet.trigger.y, markerSet.trigger.z};
        if (!SameFloor(player, trigger)) {
            continue;
        }

        const float triggerDistance = Distance(player, trigger);
        Vec2f screenPos{};
        if (WorldToScreen(trigger, camera, state.nexusLink, screenPos)) {
            const float alpha =
                triggerDistance < MapWatchService::TRIGGER_DISTANCE_CLOSED_MAP ? 0.25f : 0.8f;
            DrawMarkerIcon(draw, heart, screenPos, alpha);
        }

        if (state.settings.billboardPreview &&
            triggerDistance < MapWatchService::TRIGGER_DISTANCE_CLOSED_MAP) {
            for (const MarkerCoord& marker : markerSet.markers) {
                const Vec3f world{marker.x, marker.y, marker.z};
                if (!SameFloor(player, world)) {
                    continue;
                }

                const SquadMarker squadMarker =
                    marker.icon > 0 && marker.icon <= 9 ? static_cast<SquadMarker>(marker.icon)
                                                        : SquadMarker::Arrow;
                const ImTextureID texture = TextureService::GetTexture(squadMarker);
                if (WorldToScreen(world, camera, state.nexusLink, screenPos)) {
                    DrawMarkerIcon(draw, texture, screenPos, 1.0f);
                }
            }
        }

        DrawInteractBubble(state, camera, player, markerSet);
    }
}

}  // namespace BillboardRenderer
}  // namespace cm
