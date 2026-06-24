#pragma once

#include "core/Types.h"

#include "nexus/Nexus.h"

namespace cm {

struct PlacementPoint {
    int x = 0;
    int y = 0;
};

bool TryGetGameClientOrigin(int& left, int& top);

PlacementPoint ClientPlacementFromScreen(Vec2f screenCoord);

void SetPlacementMousePosition(PlacementPoint placementPosition, bool useScreenCoordinates);

PlacementPoint GetPlacementCursorPosition(bool useScreenCoordinates);

bool UseScreenCoordinatesForPlacement();

}  // namespace cm
