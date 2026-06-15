#include "core/MumbleUtils.h"

#include "services/RtApiService.h"
#include "utils/MumbleIdentity.h"

#include <cmath>

namespace cm {

namespace {

constexpr int kMinCompassWidth = 170;
constexpr int kMaxCompassWidth = 362;
constexpr int kMinCompassHeight = 170;
constexpr int kMaxCompassHeight = 338;
constexpr int kMinCompassOffset = 19;
constexpr int kMaxCompassOffset = 40;
constexpr int kCompassSeparation = 40;

struct CompassMetrics {
    float uiScale = 1.0f;
    float mapScale = 1.0f;
    float contentW = 0.0f;
    float contentH = 0.0f;
    float paddingLeft = 0.0f;
    float paddingTop = 0.0f;
};

float ScaleValue(float curr, float min, float max, float outMin, float outMax) {
    if (max <= min) return outMin;
    const float t = (curr - min) / (max - min);
    return outMin + t * (outMax - outMin);
}

int GetCompassOffset(float curr, float min, float max) {
    return static_cast<int>(std::lround(
        ScaleValue(curr, min, max, static_cast<float>(kMinCompassOffset),
                   static_cast<float>(kMaxCompassOffset))));
}

float ResolveScreenScale(const NexusLinkData_t* nexus, const Mumble::Data* mumble) {
    if (nexus && nexus->Scaling > 0.01f) {
        return nexus->Scaling;
    }
    return MumbleIdentity::ParseUiScale(mumble);
}

CompassMetrics ComputeCompassMetrics(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    CompassMetrics metrics{};
    if (!mumble || !nexus) {
        return metrics;
    }

    const float compassW = static_cast<float>(mumble->Context.Compass.Width);
    const float compassH = static_cast<float>(mumble->Context.Compass.Height);
    if (compassW < 1.0f || compassH < 1.0f) {
        return metrics;
    }

    metrics.uiScale = ResolveScreenScale(nexus, mumble);
    metrics.mapScale =
        mumble->Context.Compass.Scale > 0.0f ? mumble->Context.Compass.Scale : 1.0f;
    metrics.contentW = compassW * metrics.uiScale;
    metrics.contentH = compassH * metrics.uiScale;

    if (!mumble->Context.IsMapOpen) {
        metrics.paddingLeft = static_cast<float>(GetCompassOffset(
            compassW, static_cast<float>(kMinCompassWidth), static_cast<float>(kMaxCompassWidth))) *
                              metrics.uiScale;
        metrics.paddingTop = static_cast<float>(GetCompassOffset(
            compassH, static_cast<float>(kMinCompassHeight), static_cast<float>(kMaxCompassHeight))) *
                             metrics.uiScale;
    }

    return metrics;
}

Vec2f ComputeMapContentCenter(const ScreenRect& bounds,
                              const CompassMetrics& metrics,
                              bool mapOpen) {
    if (mapOpen) {
        return {static_cast<float>(bounds.x) + bounds.w * 0.5f,
                static_cast<float>(bounds.y) + bounds.h * 0.5f};
    }
    return {static_cast<float>(bounds.x) + metrics.paddingLeft + metrics.contentW * 0.5f,
            static_cast<float>(bounds.y) + metrics.paddingTop + metrics.contentH * 0.5f};
}

}  // namespace

ScreenRect GetMapBounds(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    ScreenRect rect{};
    if (!mumble || !nexus) {
        return rect;
    }

    const CompassMetrics metrics = ComputeCompassMetrics(mumble, nexus);
    if (metrics.contentW < 1.0f || metrics.contentH < 1.0f) {
        return rect;
    }

    if (mumble->Context.IsMapOpen) {
        // Overlay window covers the full Nexus viewport (matches Blish SpriteScreen). Projection
        // math uses map content center via ComputeMapContentCenter, not this rect's geometry.
        rect.x = 0;
        rect.y = 0;
        rect.w = static_cast<int>(nexus->Width);
        rect.h = static_cast<int>(nexus->Height);
        return rect;
    }

    const int offsetW = static_cast<int>(std::lround(metrics.paddingLeft));
    const int offsetH = static_cast<int>(std::lround(metrics.paddingTop));
    const int scaledCompassW = static_cast<int>(std::lround(metrics.contentW));
    const int scaledCompassH = static_cast<int>(std::lround(metrics.contentH));

    rect.w = scaledCompassW + offsetW;
    rect.h = scaledCompassH + offsetH;
    rect.x = static_cast<int>(nexus->Width) - scaledCompassW - offsetW;
    rect.y = mumble->Context.IsCompassTopRight
                 ? 0
                 : static_cast<int>(nexus->Height) - scaledCompassH - offsetH - kCompassSeparation;
    return rect;
}

ScreenMapData BuildScreenMapData(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    ScreenMapData data{};
    if (!mumble || !nexus) {
        return data;
    }

    const CompassMetrics metrics = ComputeCompassMetrics(mumble, nexus);
    data.mapCenter = {mumble->Context.Compass.Center.X, mumble->Context.Compass.Center.Y};
    data.mapRotation = (mumble->Context.IsCompassRotating && !mumble->Context.IsMapOpen)
                           ? mumble->Context.Compass.Rotation
                           : 0.0f;
    data.screenBounds = GetMapBounds(mumble, nexus);
    // Nexus draws in game screen pixels. Blish's 1/0.897 factor is a Blish HUD overlay quirk.
    data.scale = 1.0f / metrics.mapScale;
    data.boundsCenter =
        ComputeMapContentCenter(data.screenBounds, metrics, mumble->Context.IsMapOpen != 0);
    return data;
}

bool IsInGame(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    return mumble && nexus && nexus->IsGameplay;
}

bool IsGameplayReady(const NexusLinkData_t* nexus) {
    return nexus && nexus->IsGameplay && nexus->Width > 0 && nexus->Height > 0;
}

bool IsWorldReady(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    if (!IsGameplayReady(nexus) || !mumble) {
        return false;
    }
    if (mumble->UITick == 0) {
        return false;
    }
    if (mumble->Context.MapID == 0) {
        return false;
    }
    if (mumble->Context.Compass.Width < 1 || mumble->Context.Compass.Height < 1) {
        return false;
    }
    return true;
}

bool IsCommander(const Mumble::Data* mumble, const Mumble::Identity* identity) {
    if (MumbleIdentity::IsParsedIdentityUsable(mumble, identity)) {
        return identity->IsCommander;
    }
    return MumbleIdentity::ParseCommander(mumble);
}

bool HasCommanderPermissions(const Mumble::Data* mumble, const Mumble::Identity* identity) {
    if (IsCommander(mumble, identity)) {
        return true;
    }
    return RtApiService::GrantsCommanderPermissions();
}

bool IsInCombat(const Mumble::Data* mumble) {
    return mumble && mumble->Context.IsInCombat;
}

bool MapUiIsOpen(const Mumble::Data* mumble) {
    return mumble && mumble->Context.IsMapOpen;
}

Vec3f PlayerPosition(const Mumble::Data* mumble) {
    if (!mumble) {
        return {};
    }
    return MumbleToGame(mumble->AvatarPosition);
}

Vec3f MumbleToGame(const Vector3& v) {
    return MumbleToGame(v.X, v.Y, v.Z);
}

Vec3f MumbleToGame(float x, float y, float z) {
    return {x, z, y};
}

}  // namespace cm
