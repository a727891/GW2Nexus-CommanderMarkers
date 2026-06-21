#pragma once

#include "core/SettingsStore.h"
#include "core/AccountRegistry.h"
#include "data/MapDataCache.h"
#include "services/CommunityCatalogService.h"
#include "services/Gw2ApiClient.h"
#include "services/MapWatchService.h"
#include "services/MarkerListing.h"
#include "services/MarkerPlacementService.h"
#include "services/ModuleManifestService.h"
#include "services/PreviewImageCache.h"
#include "services/SubtokenService.h"

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
    const Mumble::Identity* playerIdentity = nullptr;
    std::string addonDir;

    SettingsStore settings;
    bool ltMode = false;

    MapDataCache mapData;
    MarkerListing markerListing;
    ModuleManifestService moduleManifest;
    CommunityCatalogService communityCatalog;
    PreviewImageCache previewImageCache;
    AccountRegistry accountRegistry;
    Gw2ApiClient gw2ApiClient;
    SubtokenService subtokenService;
    MapWatchService mapWatch;
    MarkerPlacementService placementService;

    std::atomic<bool> communitySyncPending{false};
    std::atomic<bool> communitySyncInProgress{false};
    std::atomic<bool> communityUpdatedNotice{false};
    std::atomic<bool> mapRefreshPending{false};
    std::atomic<bool> mapRefreshInProgress{false};
    std::atomic<bool> mapUpdatedNotice{false};
    int lastMapRefreshBuildId = 0;
    int initStep = 0;
    uint32_t stableMapId = 0;
    uint32_t stableMumbleTick = 0;
    int stableFrameCount = 0;

    static constexpr int kStableFramesRequired = 30;

    bool loadInitialized = false;
    bool optionsReady = false;
    bool renderInitialized = false;

    bool requiredBindsOk = false;
    bool warnedUnboundBinds = false;

    void PrepareLoad(AddonAPI_t* apiPtr);
    void EnsureOptionsReady();
    void ProcessDeferredInit();
    bool IsReady() const { return loadInitialized; }
    bool IsOptionsReady() const { return optionsReady; }
    void Shutdown();

    void RequestCommunitySync();
    void ProcessPendingCommunitySync();

    void RequestMapRefresh();
    void ProcessPendingMapRefresh();

    void ProcessRenderInit();
    void ProcessBackgroundNotices();

    std::string settingsPath() const;
    std::string apiAccountsPath() const;

private:
    void CheckRequiredBinds();
    bool AdvanceMumbleStability();
    float UiScale() const;

    AppState();
};

}  // namespace cm
