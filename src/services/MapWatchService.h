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
                    MarkerPlacementService* placementService, SettingsStore* settings);

    void Update(Mumble::Data* mumble, NexusLinkData_t* nexus, bool ltMode, float uiScale);
    void OnInteractKey(Mumble::Data* mumble, NexusLinkData_t* nexus, bool ltMode, float uiScale);

    void PreviewClosestMarkerSet(Mumble::Data* mumble);
    void SetPreviewMarkerSet(const MarkerSet& markerSet);
    void RemovePreviewMarkerSet();
    const MarkerSet* ActivePreview() const;

    void PlaceMarkerSet(const MarkerSet& markerSet, Mumble::Data* mumble, NexusLinkData_t* nexus,
                        float uiScale);

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

    int currentMapId_ = 0;
    std::vector<MarkerSet> currentMarkers_;
    std::optional<MarkerSet> previewMarkerSet_;
    uint64_t lastTriggerMs_ = 0;
};

}  // namespace cm
