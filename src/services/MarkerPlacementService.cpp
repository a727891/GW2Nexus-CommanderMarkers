#include "services/MarkerPlacementService.h"

#include "utils/MarkerBindHelper.h"

#include <algorithm>
#include <windows.h>

namespace cm {

MarkerPlacementService::MarkerPlacementService(AddonAPI_t* api) : api_(api) {}

void MarkerPlacementService::SetApi(AddonAPI_t* api) {
    api_ = api;
}

bool MarkerPlacementService::IsBusy() const {
    return phase_ != Phase::Idle;
}

const std::vector<std::string>& MarkerPlacementService::LastErrors() const {
    return errors_;
}

const std::vector<std::string>& MarkerPlacementService::LastMissingBinds() const {
    return missingBinds_;
}

void MarkerPlacementService::BeginWait(Phase nextPhase, uint64_t delayMs) {
    phaseAfterWait_ = nextPhase;
    waitUntilMs_ = GetTickCount64() + delayMs;
    phase_ = Phase::Wait;
}

void MarkerPlacementService::Finish() {
    SetPlacementMousePosition(savedCursor_, useScreenCoords_);
    mapData_ = nullptr;
    markerIndex_ = 0;
    phase_ = Phase::Idle;
    phaseAfterWait_ = Phase::Idle;
}

void MarkerPlacementService::ProcessNextMarker() {
    while (markerIndex_ < markerSet_.markers.size()) {
        const MarkerCoord& marker = markerSet_.markers[markerIndex_];
        ++markerIndex_;

        if (marker.icon > 9 || marker.icon < 0 || marker.icon == 0) {
            continue;
        }

        const SquadMarker squadMarker = static_cast<SquadMarker>(marker.icon);
        const Vec3f worldPos{marker.x, marker.y, marker.z};
        const Vec2f blishCoord =
            MapDataCache::WorldToScreenMap(markerSet_.mapId, worldPos, screenMap_, *mapData_);
        if (!screenMap_.screenBounds.Contains(blishCoord.x, blishCoord.y)) {
            errors_.push_back(SquadMarkerDisplayName(squadMarker) + " " + marker.name);
            continue;
        }

        const PlacementPoint placementPos = ClientPlacementFromScreen(blishCoord);
        SetPlacementMousePosition(placementPos, useScreenCoords_);
        pendingMarker_ = squadMarker;
        BeginWait(Phase::PlaceInvoke, static_cast<uint64_t>(std::max(1, delayMs_ / 2)));
        return;
    }

    phase_ = Phase::RestoreCursor;
}

bool MarkerPlacementService::QueuePlaceMarkerSet(const MarkerSet& markerSet, MapDataCache* mapData,
                                                 const ScreenMapData& screenMap, int delayMs) {
    if (IsBusy()) {
        return false;
    }
    if (!api_ || !mapData) {
        return false;
    }

    const BindCheckResult binds = CheckRequiredBinds(api_, false);
    if (!binds.ok) {
        missingBinds_ = binds.missing;
        return false;
    }

    missingBinds_.clear();
    mapData_ = mapData;
    markerSet_ = markerSet;
    screenMap_ = screenMap;
    delayMs_ = std::max(1, delayMs);
    errors_.clear();
    markerIndex_ = 0;
    pendingMarker_ = SquadMarker::None;
    useScreenCoords_ = UseScreenCoordinatesForPlacement();
    savedCursor_ = GetPlacementCursorPosition(useScreenCoords_);
    phase_ = Phase::ClearAll;
    return true;
}

void MarkerPlacementService::Tick() {
    if (phase_ == Phase::Idle || !api_ || !mapData_) {
        return;
    }

    const uint64_t now = GetTickCount64();
    if (phase_ == Phase::Wait) {
        if (now < waitUntilMs_) {
            return;
        }
        phase_ = phaseAfterWait_;
    }

    switch (phase_) {
        case Phase::ClearAll:
            InvokeGroundMarker(SquadMarker::Clear, api_, delayMs_ / 2);
            BeginWait(Phase::ProcessMarkers, static_cast<uint64_t>(std::max(1, delayMs_ / 2)));
            break;

        case Phase::ProcessMarkers:
            ProcessNextMarker();
            if (phase_ == Phase::RestoreCursor) {
                Finish();
            }
            break;

        case Phase::PlaceInvoke:
            InvokeGroundMarker(pendingMarker_, api_, delayMs_);
            BeginWait(Phase::ProcessMarkers, static_cast<uint64_t>(delayMs_));
            break;

        case Phase::RestoreCursor:
            Finish();
            break;

        case Phase::Idle:
        case Phase::Wait:
            break;
    }
}

}  // namespace cm
