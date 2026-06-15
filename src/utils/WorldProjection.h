#pragma once

#include "core/Types.h"

#include "mumble/Mumble.h"
#include "nexus/Nexus.h"

namespace cm {

struct GameCamera {
    Vec3f position{};
    Vec3f front{};
    Vec3f right{};
    Vec3f up{};
    float fovVerticalRadians = 0.873f;
};

GameCamera BuildGameCamera(const Mumble::Data* mumble);

// Project a game-space world point (X=east, Y=north, Z=elevation meters) to Nexus pixels.
bool WorldToScreen(const Vec3f& world, const GameCamera& camera, const NexusLinkData_t* nexus,
                   Vec2f& out);

}  // namespace cm
