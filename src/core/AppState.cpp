#include "core/AppState.h"

#include "core/Branding.h"
#include "core/FeatureFlags.h"
#include "core/MumbleUtils.h"
#include "services/Gw2MapApiClient.h"
#include "ui/DatAssetIconService.h"
#include "services/RtApiService.h"
#include "ui/QuickAccessService.h"
#include "ui/SettingsWindow.h"
#include "ui/TextureService.h"
#include "ui/UiFontService.h"
#include "utils/MarkerBindHelper.h"

#include <filesystem>
#include <sstream>
#include <thread>

namespace cm {

AppState::AppState()
    : mapData(""),
      markerListing(""),
      communityMarkers(""),
      placementService(nullptr) {}

AppState& AppState::Instance() {
    static AppState state;
    return state;
}

std::string AppState::settingsPath() const {
    return (std::filesystem::path(addonDir) / "settings.json").string();
}

float AppState::UiScale() const {
    if (!nexusLink) {
        return 1.0f;
    }
    return nexusLink->Scaling > 0.0f ? nexusLink->Scaling : 1.0f;
}

void AppState::CheckRequiredBinds() {
    if (!api) {
        requiredBindsOk = false;
        return;
    }

    const BindCheckResult result =
        cm::CheckRequiredBinds(api, settings.objectMarkersEnabled);
    requiredBindsOk = result.ok;

    if (!result.ok && !warnedUnboundBinds) {
        warnedUnboundBinds = true;
        std::ostringstream message;
        message << "Missing squad marker keybinds: ";
        for (size_t i = 0; i < result.missing.size(); ++i) {
            if (i > 0) {
                message << ", ";
            }
            message << result.missing[i];
        }
        api->Log(LOGL_WARNING, kLogChannel, message.str().c_str());
    }
}

void AppState::PrepareLoad(AddonAPI_t* apiPtr) {
    api = apiPtr;
    nexusLink = static_cast<NexusLinkData_t*>(api->DataLink_Get(DL_NEXUS_LINK));
    mumbleLink = static_cast<Mumble::Data*>(api->DataLink_Get(DL_MUMBLE_LINK));
    playerIdentity =
        static_cast<const Mumble::Identity*>(api->DataLink_Get(DL_MUMBLE_LINK_IDENTITY));
    RtApiService::Initialize(apiPtr);
}

void AppState::EnsureOptionsReady() {
    if (optionsReady || !api) {
        return;
    }

    addonDir = api->Paths_GetAddonDirectory("NexusCommanderMarkers");
    settings.Load(settingsPath());
    optionsReady = true;
}

bool AppState::AdvanceMumbleStability() {
    if (!mumbleLink) {
        stableFrameCount = 0;
        return false;
    }

    const uint32_t mapId = mumbleLink->Context.MapID;
    const uint32_t tick = mumbleLink->UITick;
    if (mapId == 0 || tick == 0 || mumbleLink->Context.Compass.Width < 1 ||
        mumbleLink->Context.Compass.Height < 1) {
        stableMapId = 0;
        stableMumbleTick = 0;
        stableFrameCount = 0;
        return false;
    }

    if (mapId == stableMapId) {
        if (tick != stableMumbleTick) {
            stableMumbleTick = tick;
            stableFrameCount++;
        }
    } else {
        stableMapId = mapId;
        stableMumbleTick = tick;
        stableFrameCount = 1;
    }

    return stableFrameCount >= kStableFramesRequired;
}

void AppState::ProcessDeferredInit() {
    if (loadInitialized || !api) {
        return;
    }
    if (!IsWorldReady(mumbleLink, nexusLink)) {
        stableFrameCount = 0;
        return;
    }
    if (!AdvanceMumbleStability()) {
        return;
    }

    switch (initStep) {
        case 0:
            EnsureOptionsReady();
            mapData.SetAddonDir(addonDir);
            markerListing.SetAddonDir(addonDir);
            communityMarkers.SetAddonDir(addonDir);
            markerListing.SetOnMarkersChanged(nullptr);
            placementService.SetApi(api);
            initStep = 1;
            break;

        case 1:
            mapData.LoadFromDisk();
            if (mumbleLink) {
                const int mapId = static_cast<int>(mumbleLink->Context.MapID);
                if (mapId != 0 && !mapData.MapHasGeometry(mapId)) {
                    mapData.SetBuildId(0);
                    RequestMapRefresh();
                }
            }
            initStep = 2;
            break;

        case 2:
            markerListing.Load();
            initStep = 3;
            break;

        case 3:
            communityMarkers.LoadCached();
            mapWatch.Initialize(&mapData, &markerListing, &placementService, &settings,
                                &playerIdentity);
            if (cm::features::kDeferredInit) {
                TextureService::Initialize(api, addonDir);
            }
            if (cm::features::kDatAssetIcons) {
                DatAssetIconService::Initialize(api, addonDir);
            }
            UiFontService::Initialize(api, nexusLink);
            initStep = 4;
            break;

        case 4:
            CheckRequiredBinds();
            if (cm::features::kBackgroundSync) {
                const int buildId =
                    mumbleLink ? static_cast<int>(mumbleLink->Context.BuildID) : 0;
                if (Gw2MapApiClient::NeedsRefresh(mapData, buildId)) {
                    RequestMapRefresh();
                }
                RequestCommunitySync();
            }
            loadInitialized = true;
            renderInitialized = false;
            initStep = 5;
            api->Log(LOGL_INFO, kLogChannel, "Initialized.");
            break;

        default:
            break;
    }
}

void AppState::Shutdown() {
    markerListing.SetOnMarkersChanged(nullptr);
    SettingsWindow::Shutdown(api);
    RtApiService::Shutdown();
    if (api) {
        UiFontService::Shutdown(api);
    }
    if (loadInitialized || optionsReady) {
        settings.Save(settingsPath());
    }
    loadInitialized = false;
    renderInitialized = false;
    optionsReady = false;
    initStep = 0;
    stableMapId = 0;
    stableMumbleTick = 0;
    stableFrameCount = 0;
}

void AppState::RequestCommunitySync() { communitySyncPending.store(true); }

void AppState::ProcessPendingCommunitySync() {
    if (!loadInitialized || !communitySyncPending.load()) {
        return;
    }
    if (communitySyncInProgress.exchange(true)) {
        return;
    }
    communitySyncPending.store(false);

    std::thread([this]() {
        const bool updated = communityMarkers.SyncCommunity();
        if (updated) {
            communityUpdatedNotice.store(true);
        }
        communitySyncInProgress.store(false);
    }).detach();
}

void AppState::RequestMapRefresh() { mapRefreshPending.store(true); }

void AppState::ProcessPendingMapRefresh() {
    if (!loadInitialized) {
        return;
    }

    const int buildId = mumbleLink ? static_cast<int>(mumbleLink->Context.BuildID) : 0;
    if (buildId != 0 && buildId != lastMapRefreshBuildId) {
        lastMapRefreshBuildId = buildId;
        if (Gw2MapApiClient::NeedsRefresh(mapData, buildId)) {
            RequestMapRefresh();
        }
    }

    if (!mapRefreshPending.load()) {
        return;
    }
    if (buildId == 0) {
        return;
    }
    if (mapRefreshInProgress.exchange(true)) {
        return;
    }
    mapRefreshPending.store(false);

    std::thread([this, buildId]() {
        if (Gw2MapApiClient::Refresh(mapData, buildId, nullptr)) {
            mapUpdatedNotice.store(true);
        }
        mapRefreshInProgress.store(false);
    }).detach();
}

void AppState::ProcessRenderInit() {
    if (renderInitialized || !api || !loadInitialized || !cm::features::kQuickAccess) {
        return;
    }

    QuickAccessService::SyncVisibility(api, *this);
    if (QuickAccessService::IsRegistered() || !settings.cornerIconEnabled) {
        renderInitialized = true;
    }
}

void AppState::ProcessBackgroundNotices() {
    if (!api) {
        return;
    }

    if (communityUpdatedNotice.exchange(false)) {
        api->Log(LOGL_INFO, kLogChannel, "Community marker library updated.");
    }
    if (mapUpdatedNotice.exchange(false)) {
        api->Log(LOGL_INFO, kLogChannel, "Updated map data cache from GW2 API.");
    }
}

}  // namespace cm
