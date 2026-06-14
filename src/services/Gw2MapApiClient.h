#pragma once

#include "data/MapDataCache.h"

#include "nexus/Nexus.h"

namespace cm {

class Gw2MapApiClient {
public:
    static bool NeedsRefresh(const MapDataCache& cache, int buildId);
    static bool Refresh(MapDataCache& cache, int buildId, AddonAPI_t* api = nullptr);
};

}  // namespace cm
