#pragma once

#include "core/Types.h"

#include "nexus/Nexus.h"

namespace cm {

namespace RtApiService {

void Initialize(AddonAPI_t* api);
void Shutdown();
void Tick();

bool IsDetected();
bool IsActive();
bool IsAvailable();
bool GrantsCommanderPermissions();

constexpr int kSquadMarkerSlotCount = 8;

bool IsSquadMarkerPlaced(const float position[3]);
std::optional<Vec3f> GetSquadMarkerPosition(int slotIndex);
bool ImportSquadMarkerPosition(int slotIndex, MarkerCoord& marker);

}  // namespace RtApiService

}  // namespace cm
