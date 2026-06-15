#pragma once

#include "data/MapDataCache.h"

namespace cm {
namespace MapPickerUi {

std::string FormatMapLabel(const MapDataCache& cache, int mapId);
bool Draw(const char* label, int* mapId, const MapDataCache& cache);

}  // namespace MapPickerUi
}  // namespace cm
