#include "core/AppState.h"

#include "core/Branding.h"
#include "services/Gw2MapApiClient.h"
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

    const BindCheckResult result = cm::CheckRequiredBinds(api, false);
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

void AppState::Initialize(AddonAPI_t* apiPtr) {
    api = apiPtr;
    nexusLink = static_cast<NexusLinkData_t*>(api->DataLink_Get(DL_NEXUS_LINK));
    mumbleLink = static_cast<Mumble::Data*>(api->DataLink_Get(DL_MUMBLE_LINK));

    addonDir = api->Paths_GetAddonDirectory("NexusCommanderMarkers");
    mapData.SetAddonDir(addonDir);
    markerListing.SetAddonDir(addonDir);
    communityMarkers.SetAddonDir(addonDir);

    settings.Load(settingsPath());
    placementService.SetApi(api);

    mapData.LoadFromDisk();
    markerListing.Load();
    communityMarkers.LoadCached();

    mapWatch.Initialize(&mapData, &markerListing, &placementService, &settings);

    CheckRequiredBinds();

    const int buildId = mumbleLink ? static_cast<int>(mumbleLink->Context.BuildID) : 0;
    if (Gw2MapApiClient::NeedsRefresh(mapData, buildId)) {
        RequestMapRefresh();
    }

    RequestCommunitySync();
}

void AppState::Shutdown() {
    settings.Save(settingsPath());
}

void AppState::RequestCommunitySync() { communitySyncPending.store(true); }

void AppState::ProcessPendingCommunitySync() {
    if (!communitySyncPending.load()) {
        return;
    }
    if (communitySyncInProgress.exchange(true)) {
        return;
    }
    communitySyncPending.store(false);

    std::thread([this]() {
        const bool updated = communityMarkers.SyncCommunity();
        if (updated && api) {
            api->Log(LOGL_INFO, kLogChannel, "Community marker library updated.");
        }
        communitySyncInProgress.store(false);
    }).detach();
}

void AppState::RequestMapRefresh() { mapRefreshPending.store(true); }

void AppState::ProcessPendingMapRefresh() {
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
        Gw2MapApiClient::Refresh(mapData, buildId, api);
        mapRefreshInProgress.store(false);
    }).detach();
}

}  // namespace cm
