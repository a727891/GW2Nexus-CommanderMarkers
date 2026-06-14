#pragma once

#include "core/Types.h"

#include "nexus/Nexus.h"

namespace cm {

struct PlacementPoint {
    int x = 0;
    int y = 0;
};

bool TryGetGameClientOrigin(int& left, int& top);

PlacementPoint BlishToPlacementPosition(Vec2f blishCoord, float uiScaleMultiplier);

void SetPlacementMousePosition(PlacementPoint placementPosition, bool useScreenCoordinates);

PlacementPoint GetPlacementCursorPosition(bool useScreenCoordinates);

bool UseScreenCoordinatesForPlacement();

}  // namespace cm
