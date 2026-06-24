#pragma once

#include "core/Types.h"

#include "mumble/Mumble.h"
#include "nexus/Nexus.h"

namespace cm {

float ResolveScreenScale(const NexusLinkData_t* nexus, const Mumble::Data* mumble,
                         const Mumble::Identity* identity = nullptr);

ScreenRect GetMapBounds(const Mumble::Data* mumble, const NexusLinkData_t* nexus,
                        const Mumble::Identity* identity = nullptr);

ScreenMapData BuildScreenMapData(const Mumble::Data* mumble, const NexusLinkData_t* nexus,
                                 const Mumble::Identity* identity = nullptr);

bool IsInGame(const Mumble::Data* mumble, const NexusLinkData_t* nexus);

// True when Nexus reports in-world gameplay (not login or char select).
bool IsGameplayReady(const NexusLinkData_t* nexus);

// True when mumble reports a loaded map with a valid compass (not loading screen).
bool IsWorldReady(const Mumble::Data* mumble, const NexusLinkData_t* nexus);
// Uses parsed identity only after UITick > 0; otherwise reads commander from Identity JSON.
bool IsCommander(const Mumble::Data* mumble, const Mumble::Identity* identity = nullptr);
// Mumble commander tag, or RTAPI squad commander/lieutenant when RTAPI is available.
bool HasCommanderPermissions(const Mumble::Data* mumble,
                             const Mumble::Identity* identity = nullptr);
bool IsInCombat(const Mumble::Data* mumble);
bool MapUiIsOpen(const Mumble::Data* mumble);

Vec3f PlayerPosition(const Mumble::Data* mumble);

// Mumble meters: X=east/west, Y=elevation, Z=north/south. Game/Blish: X, Y=north, Z=up.
Vec3f MumbleToGame(const Vector3& v);
Vec3f MumbleToGame(float x, float y, float z);

}  // namespace cm
