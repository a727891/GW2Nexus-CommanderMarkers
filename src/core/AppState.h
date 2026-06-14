#pragma once

#include "core/SettingsStore.h"
#include "data/MapDataCache.h"
#include "services/CommunityMarkerService.h"
#include "services/MapWatchService.h"
#include "services/MarkerListing.h"
#include "services/MarkerPlacementService.h"

#include "mumble/Mumble.h"
#include "nexus/Nexus.h"

#include <atomic>
#include <string>

namespace cm {

class TextureService;

class AppState {
public:
    static AppState& Instance();

    AddonAPI_t* api = nullptr;
    NexusLinkData_t* nexusLink = nullptr;
    Mumble::Data* mumbleLink = nullptr;
    std::string addonDir;

    SettingsStore settings;
    bool ltMode = false;

    MapDataCache mapData;
    MarkerListing markerListing;
    CommunityMarkerService communityMarkers;
    MapWatchService mapWatch;
    MarkerPlacementService placementService;

    std::atomic<bool> communitySyncPending{false};
    std::atomic<bool> communitySyncInProgress{false};
    std::atomic<bool> mapRefreshPending{false};
    std::atomic<bool> mapRefreshInProgress{false};
    int lastMapRefreshBuildId = 0;

    bool requiredBindsOk = false;
    bool warnedUnboundBinds = false;

    void Initialize(AddonAPI_t* apiPtr);
    void Shutdown();

    void RequestCommunitySync();
    void ProcessPendingCommunitySync();

    void RequestMapRefresh();
    void ProcessPendingMapRefresh();

    std::string settingsPath() const;

private:
    void CheckRequiredBinds();
    float UiScale() const;

    AppState();
};

}  // namespace cm
