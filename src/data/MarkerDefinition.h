#pragma once

#include "core/Types.h"

namespace cm {

struct MarkerDefinition {
    SquadMarker markerType = SquadMarker::None;
    const char* displayName = "";
    bool supportsGroundTarget = true;
    bool supportsObjectTarget = true;
};

inline constexpr MarkerDefinition kAllMarkers[] = {
    {SquadMarker::Arrow, "Arrow"},
    {SquadMarker::Circle, "Circle"},
    {SquadMarker::Heart, "Heart"},
    {SquadMarker::Square, "Square"},
    {SquadMarker::Star, "Star"},
    {SquadMarker::Spiral, "Spiral"},
    {SquadMarker::Triangle, "Triangle"},
    {SquadMarker::Cross, "X"},
    {SquadMarker::Clear, "Clear", true, true},
};

inline constexpr std::size_t kMarkerDefinitionCount = sizeof(kAllMarkers) / sizeof(kAllMarkers[0]);

}  // namespace cm
