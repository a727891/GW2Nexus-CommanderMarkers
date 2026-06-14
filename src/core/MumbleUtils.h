#pragma once

#include "core/Types.h"

#include "mumble/Mumble.h"
#include "nexus/Nexus.h"

namespace cm {

ScreenRect GetMapBounds(const Mumble::Data* mumble, const NexusLinkData_t* nexus);

ScreenMapData BuildScreenMapData(const Mumble::Data* mumble, const NexusLinkData_t* nexus);

bool IsInGame(const Mumble::Data* mumble, const NexusLinkData_t* nexus);
bool IsCommander(const Mumble::Data* mumble);
bool IsInCombat(const Mumble::Data* mumble);
bool MapUiIsOpen(const Mumble::Data* mumble);

Vec3f PlayerPosition(const Mumble::Data* mumble);

}  // namespace cm
