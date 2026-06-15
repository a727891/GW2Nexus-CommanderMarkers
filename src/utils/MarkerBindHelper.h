#pragma once

#include "core/Types.h"

#include "nexus/Nexus.h"

#include <optional>
#include <string>
#include <vector>

namespace cm {

struct BindCheckResult {
    bool ok = false;
    std::vector<std::string> missing;
};

std::optional<EGameBinds> GroundMarkerToBind(SquadMarker marker);
std::optional<EGameBinds> ObjectMarkerToBind(SquadMarker marker);

void InvokeGroundMarker(SquadMarker marker, AddonAPI_t* api, int durationMs);
void InvokeObjectMarker(SquadMarker marker, AddonAPI_t* api, int durationMs);

BindCheckResult CheckRequiredBinds(AddonAPI_t* api, bool needObject);
bool IsBindAvailable(AddonAPI_t* api, EGameBinds bind);

std::string SquadMarkerDisplayName(SquadMarker marker);

}  // namespace cm
