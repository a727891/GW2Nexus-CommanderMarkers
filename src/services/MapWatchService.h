#pragma once

#include "core/Types.h"

#include "mumble/Mumble.h"
#include "nexus/Nexus.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace cm {

class MapDataCache;
class MarkerListing;
class MarkerPlacementService;
struct SettingsStore;

class MapWatchService {
public:
    static constexpr float TRIGGER_DISTANCE_OPEN_MAP = 15.0f;
    static constexpr float TRIGGER_DISTANCE_CLOSED_MAP = 2.0f;
    static constexpr int kInteractCooldownMs = 3000;

    void Initialize(MapDataCache* mapData, MarkerListing* markerListing,
                    MarkerPlacementService* placementService, SettingsStore* settings,
                    const Mumble::Identity* const* playerIdentity);

    void Update(Mumble::Data* mumble, NexusLinkData_t* nexus, bool ltMode);
    void OnInteractKey(Mumble::Data* mumble, NexusLinkData_t* nexus, bool ltMode);

    void PreviewClosestMarkerSet(Mumble::Data* mumble);
    void SetPreviewMarkerSet(const MarkerSet& markerSet);
    void RemovePreviewMarkerSet();
    const MarkerSet* ActivePreview() const;
    bool IsManualPreview() const { return manualPreviewActive_; }
    bool ShouldShowMapPreview(const Mumble::Data* mumble) const;

    void PlaceMarkerSet(const MarkerSet& markerSet, Mumble::Data* mumble, NexusLinkData_t* nexus);

    const std::vector<MarkerSet>& CurrentMapMarkers() const { return currentMarkers_; }

private:
    bool ShouldAttemptPlacement(Mumble::Data* mumble, NexusLinkData_t* nexus, bool ltMode) const;
    float PlacementThreshold(Mumble::Data* mumble) const;
    const MarkerSet* FindClosestMarker(Mumble::Data* mumble, float threshold) const;
    void RefreshMapMarkers(int mapId);

    MapDataCache* mapData_ = nullptr;
    MarkerListing* markerListing_ = nullptr;
    MarkerPlacementService* placementService_ = nullptr;
    SettingsStore* settings_ = nullptr;
    const Mumble::Identity* const* playerIdentity_ = nullptr;

    int currentMapId_ = 0;
    std::vector<MarkerSet> currentMarkers_;
    std::optional<MarkerSet> previewMarkerSet_;
    bool manualPreviewActive_ = false;
    uint64_t lastTriggerMs_ = 0;
    bool lastAutoMarkerEnabled_ = true;
};

}  // namespace cm
