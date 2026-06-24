#include "services/MapWatchService.h"

#include "core/MumbleUtils.h"
#include "core/SettingsStore.h"
#include "data/MapDataCache.h"
#include "services/MarkerListing.h"
#include "services/MarkerPlacementService.h"

#include <cmath>
#include <windows.h>

namespace cm {

namespace {

float DistanceToTrigger(const Vec3f& player, const WorldCoord& trigger) {
    const float dx = player.x - trigger.x;
    const float dy = player.y - trigger.y;
    const float dz = player.z - trigger.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

}  // namespace

void MapWatchService::Initialize(MapDataCache* mapData, MarkerListing* markerListing,
                                 MarkerPlacementService* placementService,
                                 SettingsStore* settings,
                                 const Mumble::Identity* const* playerIdentity) {
    mapData_ = mapData;
    markerListing_ = markerListing;
    placementService_ = placementService;
    settings_ = settings;
    playerIdentity_ = playerIdentity;
    currentMapId_ = -1;

    if (markerListing_) {
        markerListing_->SetOnMarkersChanged([this]() {
            RefreshMapMarkers(currentMapId_);
        });
    }

    if (settings_) {
        lastAutoMarkerEnabled_ = settings_->autoMarkerEnabled;
    }
}

void MapWatchService::RefreshMapMarkers(int mapId) {
    if (!markerListing_ || !settings_) {
        return;
    }

    currentMapId_ = mapId;
    currentMarkers_.clear();
    previewMarkerSet_.reset();
    manualPreviewActive_ = false;

    if (!settings_->autoMarkerEnabled) {
        return;
    }

    for (const auto& markerSet : markerListing_->GetMarkersForMap(mapId)) {
        if (markerSet.enabled) {
            currentMarkers_.push_back(markerSet);
        }
    }
}

bool MapWatchService::ShouldAttemptPlacement(Mumble::Data* mumble, NexusLinkData_t* nexus,
                                             bool ltMode) const {
    if (!settings_ || !mumble || !nexus) {
        return false;
    }

    if (!settings_->autoMarkerEnabled) {
        return false;
    }

    if (!IsInGame(mumble, nexus)) {
        return false;
    }

    if (IsInCombat(mumble) && !settings_->combatPlacement) {
        return false;
    }

    if (settings_->autoMarkerOnlyCommander || ltMode) {
        const Mumble::Identity* identity =
            playerIdentity_ ? *playerIdentity_ : nullptr;
        return HasCommanderPermissions(mumble, identity) || ltMode;
    }

    return true;
}

float MapWatchService::PlacementThreshold(Mumble::Data* mumble) const {
    return cm::MapUiIsOpen(mumble) ? TRIGGER_DISTANCE_OPEN_MAP : TRIGGER_DISTANCE_CLOSED_MAP;
}

const MarkerSet* MapWatchService::FindClosestMarker(Mumble::Data* mumble, float threshold) const {
    if (!mumble || currentMarkers_.empty()) {
        return nullptr;
    }

    const Vec3f player = PlayerPosition(mumble);
    const MarkerSet* closest = nullptr;
    float closestDistance = threshold;

    for (const auto& markerSet : currentMarkers_) {
        const float distance = DistanceToTrigger(player, markerSet.trigger);
        if (distance < closestDistance) {
            closest = &markerSet;
            closestDistance = distance;
        }
    }

    return closest;
}

void MapWatchService::PlaceMarkerSet(const MarkerSet& markerSet, Mumble::Data* mumble,
                                     NexusLinkData_t* nexus) {
    if (!placementService_ || !mapData_ || !settings_ || !mumble || !nexus) {
        return;
    }

    const Mumble::Identity* identity = playerIdentity_ ? *playerIdentity_ : nullptr;
    const ScreenMapData screenMap = BuildScreenMapData(mumble, nexus, identity);
    placementService_->QueuePlaceMarkerSet(markerSet, mapData_, screenMap, settings_->placementDelayMs);
}

void MapWatchService::Update(Mumble::Data* mumble, NexusLinkData_t* nexus, bool ltMode) {
    (void)ltMode;

    if (!mumble) {
        return;
    }

    const int mapId = static_cast<int>(mumble->Context.MapID);
    if (mapId != currentMapId_) {
        RefreshMapMarkers(mapId);
    } else if (settings_) {
        const bool autoMarker = settings_->autoMarkerEnabled;
        if (autoMarker != lastAutoMarkerEnabled_) {
            lastAutoMarkerEnabled_ = autoMarker;
            RefreshMapMarkers(mapId);
        }
    }

    if (!manualPreviewActive_ && settings_ && mumble) {
        if (settings_->autoMarkerShowPreview) {
            PreviewClosestMarkerSet(mumble);
        } else if (previewMarkerSet_) {
            previewMarkerSet_.reset();
        }
    }
}

void MapWatchService::OnInteractKey(Mumble::Data* mumble, NexusLinkData_t* nexus, bool ltMode) {
    const uint64_t now = GetTickCount64();
    if (now - lastTriggerMs_ < static_cast<uint64_t>(kInteractCooldownMs)) {
        return;
    }

    if (currentMarkers_.empty()) {
        return;
    }

    if (!cm::MapUiIsOpen(mumble) && (!settings_ || !settings_->billboardPlacement)) {
        return;
    }

    if (!ShouldAttemptPlacement(mumble, nexus, ltMode)) {
        return;
    }

    const MarkerSet* closest = FindClosestMarker(mumble, PlacementThreshold(mumble));
    if (!closest) {
        return;
    }

    lastTriggerMs_ = now;
    PlaceMarkerSet(*closest, mumble, nexus);
}

void MapWatchService::PreviewClosestMarkerSet(Mumble::Data* mumble) {
    if (manualPreviewActive_) {
        return;
    }

    previewMarkerSet_.reset();

    if (!settings_ || !settings_->autoMarkerShowPreview || !mumble) {
        return;
    }

    const MarkerSet* closest = FindClosestMarker(mumble, TRIGGER_DISTANCE_OPEN_MAP);
    if (closest) {
        previewMarkerSet_ = *closest;
    }
}

void MapWatchService::RemovePreviewMarkerSet() {
    manualPreviewActive_ = false;
    previewMarkerSet_.reset();
}

void MapWatchService::SetPreviewMarkerSet(const MarkerSet& markerSet) {
    manualPreviewActive_ = true;
    previewMarkerSet_ = markerSet;
}

bool MapWatchService::ShouldShowMapPreview(const Mumble::Data* mumble) const {
    (void)mumble;
    return previewMarkerSet_ && settings_ && settings_->autoMarkerShowPreview;
}

const MarkerSet* MapWatchService::ActivePreview() const {
    return previewMarkerSet_ ? &*previewMarkerSet_ : nullptr;
}

}  // namespace cm
