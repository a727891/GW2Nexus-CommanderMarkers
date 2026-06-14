#pragma once

#include "core/Types.h"
#include "data/MapDataCache.h"

#include "nexus/Nexus.h"
#include "utils/MarkerPlacementHelper.h"

#include <cstdint>
#include <vector>

namespace cm {

class MarkerPlacementService {
public:
    explicit MarkerPlacementService(AddonAPI_t* api = nullptr);

    void SetApi(AddonAPI_t* api);
    void Tick();

    bool QueuePlaceMarkerSet(const MarkerSet& markerSet, MapDataCache* mapData,
                             const ScreenMapData& screenMap, int delayMs, float uiScale);

    bool IsBusy() const;
    const std::vector<std::string>& LastErrors() const;
    const std::vector<std::string>& LastMissingBinds() const;

private:
    enum class Phase {
        Idle,
        Wait,
        ClearAll,
        ProcessMarkers,
        PlaceInvoke,
        RestoreCursor,
    };

    void BeginWait(Phase nextPhase, uint64_t delayMs);
    void ProcessNextMarker();
    void Finish();

    AddonAPI_t* api_ = nullptr;
    MapDataCache* mapData_ = nullptr;
    MarkerSet markerSet_{};
    ScreenMapData screenMap_{};
    std::vector<std::string> errors_{};
    std::vector<std::string> missingBinds_{};
    PlacementPoint savedCursor_{};
    bool useScreenCoords_ = false;
    int delayMs_ = 100;
    float uiScale_ = 1.0f;
    size_t markerIndex_ = 0;
    Phase phase_ = Phase::Idle;
    Phase phaseAfterWait_ = Phase::Idle;
    uint64_t waitUntilMs_ = 0;
    SquadMarker pendingMarker_ = SquadMarker::None;
};

}  // namespace cm
