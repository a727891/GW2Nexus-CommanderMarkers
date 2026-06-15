#include "utils/WorldProjection.h"

#include "core/MumbleUtils.h"

#include <cmath>
#include <nlohmann/json.hpp>
#include <windows.h>

namespace cm {
namespace {

constexpr Vec3f kWorldUp{0.0f, 0.0f, 1.0f};

float Length(const Vec3f& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vec3f Normalize(const Vec3f& v) {
    const float len = Length(v);
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

void BuildCameraBasis(const Vec3f& front, Vec3f& outRight, Vec3f& outUp) {
    // Left-handed Z-up (GW2 game space): right = forward × up.
    outRight = Cross(front, kWorldUp);
    const float rightLen = Length(outRight);
    if (rightLen <= 0.0001f) {
        // Pitch near straight up/down: keep screen X aligned with east/west.
        outRight = {1.0f, 0.0f, 0.0f};
    } else {
        outRight = {outRight.x / rightLen, outRight.y / rightLen, outRight.z / rightLen};
    }
    outUp = Normalize(Cross(outRight, front));
}

float ReadVerticalFovRadians(const Mumble::Data* mumble) {
    // GW2 identity "fov" is vertical radians (~0.873). Blish clamps directly to radians.
    constexpr float kDefaultFov = 0.873f;
    if (!mumble) {
        return kDefaultFov;
    }

    try {
        const std::wstring identityWide(mumble->Identity);
        const int size = WideCharToMultiByte(CP_UTF8, 0, identityWide.c_str(), -1, nullptr, 0,
                                             nullptr, nullptr);
        if (size <= 1) {
            return kDefaultFov;
        }

        std::string identity(static_cast<size_t>(size - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, identityWide.c_str(), -1, identity.data(), size, nullptr,
                            nullptr);
        const auto j = nlohmann::json::parse(identity);
        const float fov = j.value("fov", 0.0f);
        if (fov <= 0.0f) {
            return kDefaultFov;
        }
        if (fov > 3.0f) {
            return fov * 3.14159265f / 180.0f;
        }
        return std::max(0.01f, std::min(fov, 3.13159265f));
    } catch (...) {
    }

    return kDefaultFov;
}

}  // namespace

GameCamera BuildGameCamera(const Mumble::Data* mumble) {
    GameCamera camera{};
    if (!mumble) {
        return camera;
    }

    camera.position = MumbleToGame(mumble->CameraPosition);
    camera.front = Normalize(MumbleToGame(mumble->CameraFront));
    if (camera.front.x == 0.0f && camera.front.y == 0.0f && camera.front.z == 0.0f) {
        return camera;
    }

    BuildCameraBasis(camera.front, camera.right, camera.up);
    camera.fovVerticalRadians = ReadVerticalFovRadians(mumble);
    return camera;
}

bool WorldToScreen(const Vec3f& world, const GameCamera& camera, const NexusLinkData_t* nexus,
                   Vec2f& out) {
    if (!nexus || nexus->Width == 0 || nexus->Height == 0) {
        return false;
    }
    if (camera.front.x == 0.0f && camera.front.y == 0.0f && camera.front.z == 0.0f) {
        return false;
    }

    const Vec3f delta = {world.x - camera.position.x, world.y - camera.position.y,
                         world.z - camera.position.z};
    const float depth = Dot(delta, camera.front);
    if (depth <= 0.05f) {
        return false;
    }

    const float x = Dot(delta, camera.right);
    const float y = Dot(delta, camera.up);
    const float aspect =
        static_cast<float>(nexus->Width) / static_cast<float>(nexus->Height > 0 ? nexus->Height : 1);

    // GW2 / Blish PlayerCamera uses vertical field of view (XNA PerspectiveFOV).
    const float tanHalfFovY = std::tan(camera.fovVerticalRadians * 0.5f);
    const float tanHalfFovX = tanHalfFovY * aspect;

    const float clipX = x / (depth * tanHalfFovX);
    const float clipY = y / (depth * tanHalfFovY);
    if (clipX < -1.5f || clipX > 1.5f || clipY < -1.5f || clipY > 1.5f) {
        return false;
    }

    out.x = (clipX + 1.0f) * 0.5f * static_cast<float>(nexus->Width);
    out.y = (1.0f - clipY) * 0.5f * static_cast<float>(nexus->Height);
    return true;
}

}  // namespace cm
