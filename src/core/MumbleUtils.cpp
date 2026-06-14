#include "core/MumbleUtils.h"

#include <cmath>
#include <nlohmann/json.hpp>
#include <windows.h>

namespace cm {

namespace {

constexpr int kMinCompassWidth = 170;
constexpr int kMaxCompassWidth = 362;
constexpr int kMinCompassHeight = 170;
constexpr int kMaxCompassHeight = 338;
constexpr int kMinCompassOffset = 19;
constexpr int kMaxCompassOffset = 40;
constexpr int kCompassSeparation = 40;
constexpr double kBlishScale = 1.0 / 0.897;

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

std::string WideToUtf8(const wchar_t* wide) {
    if (!wide || wide[0] == L'\0') return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr);
    return result;
}

}  // namespace

ScreenRect GetMapBounds(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    ScreenRect rect{};
    if (!mumble || !nexus) return rect;

    const float compassW = static_cast<float>(mumble->Context.Compass.Width);
    const float compassH = static_cast<float>(mumble->Context.Compass.Height);
    if (compassW < 1.0f || compassH < 1.0f) return rect;

    if (mumble->Context.IsMapOpen) {
        rect.x = 0;
        rect.y = 0;
        rect.w = static_cast<int>(nexus->Width);
        rect.h = static_cast<int>(nexus->Height);
        return rect;
    }

    const int offsetW =
        GetCompassOffset(compassW, static_cast<float>(kMinCompassWidth), static_cast<float>(kMaxCompassWidth));
    const int offsetH =
        GetCompassOffset(compassH, static_cast<float>(kMinCompassHeight), static_cast<float>(kMaxCompassHeight));

    rect.w = static_cast<int>(compassW) + offsetW;
    rect.h = static_cast<int>(compassH) + offsetH;
    rect.x = static_cast<int>(nexus->Width) - static_cast<int>(compassW) - offsetW;
    rect.y = mumble->Context.IsCompassTopRight
                 ? 0
                 : static_cast<int>(nexus->Height) - static_cast<int>(compassH) - offsetH - kCompassSeparation;
    return rect;
}

ScreenMapData BuildScreenMapData(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    ScreenMapData data{};
    if (!mumble || !nexus) return data;

    data.mapCenter = {mumble->Context.Compass.Center.X, mumble->Context.Compass.Center.Y};
    data.mapRotation = (mumble->Context.IsCompassRotating && !mumble->Context.IsMapOpen)
                           ? mumble->Context.Compass.Rotation
                           : 0.0f;
    data.screenBounds = GetMapBounds(mumble, nexus);
    const float mapScale =
        mumble->Context.Compass.Scale > 0.0f ? mumble->Context.Compass.Scale : 1.0f;
    data.scale = static_cast<float>(kBlishScale / mapScale);
    data.boundsCenter = {static_cast<float>(data.screenBounds.x) + data.screenBounds.w * 0.5f,
                         static_cast<float>(data.screenBounds.y) + data.screenBounds.h * 0.5f};
    return data;
}

bool IsInGame(const Mumble::Data* mumble, const NexusLinkData_t* nexus) {
    return mumble && nexus && nexus->IsGameplay;
}

bool IsCommander(const Mumble::Data* mumble) {
    if (!mumble) return false;
    const std::string identity = WideToUtf8(mumble->Identity);
    if (identity.empty()) return false;
    try {
        const auto j = nlohmann::json::parse(identity);
        return j.value("com", false);
    } catch (...) {
        return false;
    }
}

bool IsInCombat(const Mumble::Data* mumble) {
    return mumble && mumble->Context.IsInCombat;
}

bool MapUiIsOpen(const Mumble::Data* mumble) {
    return mumble && mumble->Context.IsMapOpen;
}

Vec3f PlayerPosition(const Mumble::Data* mumble) {
    if (!mumble) return {};
    return {mumble->AvatarPosition.X, mumble->AvatarPosition.Y, mumble->AvatarPosition.Z};
}

}  // namespace cm
