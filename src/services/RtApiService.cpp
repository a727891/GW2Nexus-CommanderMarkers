#include "services/RtApiService.h"

#include "core/Branding.h"

#include "RTAPI.h"

#include <cmath>
#include <optional>

namespace cm {
namespace RtApiService {
namespace {

AddonAPI_t* g_api = nullptr;
RealTimeData* g_rtapi = nullptr;
bool g_subscribed = false;
bool g_selfIsCommander = false;
bool g_selfIsLieutenant = false;

bool RtApiLinkActive() { return g_rtapi && g_rtapi->GameBuild != 0; }

bool IsSquadGroupType(uint32_t groupType) {
    return groupType == GT_Squad || groupType == GT_RaidSquad;
}

void ClearSelfRole() {
    g_selfIsCommander = false;
    g_selfIsLieutenant = false;
}

void UpdateSelfFromMember(const GroupMember* member) {
    if (!member) {
        return;
    }
    g_selfIsCommander = member->IsCommander != 0;
    g_selfIsLieutenant = member->IsLieutenant != 0;
}

void RefreshDataLink() {
    if (!g_api) {
        g_rtapi = nullptr;
        ClearSelfRole();
        return;
    }

    g_rtapi = static_cast<RealTimeData*>(g_api->DataLink_Get(DL_RTAPI));
    if (!RtApiLinkActive()) {
        ClearSelfRole();
    }
}

void OnAddonLoaded(void* eventArgs) {
    const auto* signature = static_cast<const uint32_t*>(eventArgs);
    if (!signature || *signature != RTAPI_SIG) {
        return;
    }

    RefreshDataLink();
    if (RtApiLinkActive() && g_api) {
        g_api->Log(LOGL_DEBUG, kLogChannel, "RTAPI loaded; squad role integration active.");
    }
}

void OnAddonUnloaded(void* eventArgs) {
    const auto* signature = static_cast<const uint32_t*>(eventArgs);
    if (!signature || *signature != RTAPI_SIG) {
        return;
    }

    g_rtapi = nullptr;
    ClearSelfRole();
}

void OnGroupMemberJoined(void* eventArgs) {
    if (!RtApiLinkActive()) {
        return;
    }

    const auto* member = static_cast<const GroupMember*>(eventArgs);
    if (member && member->IsSelf) {
        UpdateSelfFromMember(member);
    }
}

void OnGroupMemberUpdated(void* eventArgs) { OnGroupMemberJoined(eventArgs); }

void OnGroupMemberLeft(void* eventArgs) {
    if (!RtApiLinkActive()) {
        return;
    }

    const auto* member = static_cast<const GroupMember*>(eventArgs);
    if (member && member->IsSelf) {
        ClearSelfRole();
    }
}

void Subscribe(AddonAPI_t* api) {
    if (!api || !api->Events_Subscribe || g_subscribed) {
        return;
    }

    api->Events_Subscribe(EV_ADDON_LOADED, OnAddonLoaded);
    api->Events_Subscribe(EV_ADDON_UNLOADED, OnAddonUnloaded);
    api->Events_Subscribe(EV_RTAPI_GROUP_MEMBER_JOINED, OnGroupMemberJoined);
    api->Events_Subscribe(EV_RTAPI_GROUP_MEMBER_UPDATED, OnGroupMemberUpdated);
    api->Events_Subscribe(EV_RTAPI_GROUP_MEMBER_LEFT, OnGroupMemberLeft);
    g_subscribed = true;
}

void Unsubscribe(AddonAPI_t* api) {
    if (!api || !api->Events_Unsubscribe || !g_subscribed) {
        return;
    }

    api->Events_Unsubscribe(EV_ADDON_LOADED, OnAddonLoaded);
    api->Events_Unsubscribe(EV_ADDON_UNLOADED, OnAddonUnloaded);
    api->Events_Unsubscribe(EV_RTAPI_GROUP_MEMBER_JOINED, OnGroupMemberJoined);
    api->Events_Unsubscribe(EV_RTAPI_GROUP_MEMBER_UPDATED, OnGroupMemberUpdated);
    api->Events_Unsubscribe(EV_RTAPI_GROUP_MEMBER_LEFT, OnGroupMemberLeft);
    g_subscribed = false;
}

}  // namespace

void Initialize(AddonAPI_t* api) {
    if (!api) {
        return;
    }

    g_api = api;
    RefreshDataLink();
    Subscribe(api);

    if (IsActive()) {
        api->Log(LOGL_DEBUG, kLogChannel, "RTAPI detected; squad role integration active.");
    }
}

void Shutdown() {
    Unsubscribe(g_api);
    g_api = nullptr;
    g_rtapi = nullptr;
    ClearSelfRole();
}

void Tick() {
    if (g_rtapi && g_rtapi->GameBuild == 0) {
        g_rtapi = nullptr;
        ClearSelfRole();
    }
}

bool IsDetected() {
    if (!g_api) {
        return false;
    }
    return g_api->DataLink_Get(DL_RTAPI) != nullptr;
}

bool IsActive() {
    if (!g_api) {
        return false;
    }
    const auto* data = static_cast<const RealTimeData*>(g_api->DataLink_Get(DL_RTAPI));
    return data && data->GameBuild != 0;
}

bool IsAvailable() { return IsActive(); }

bool GrantsCommanderPermissions() {
    if (!IsActive() || !g_rtapi || !IsSquadGroupType(g_rtapi->GroupType)) {
        return false;
    }
    return g_selfIsCommander || g_selfIsLieutenant;
}

Vec3f RtApiPositionToGame(const float position[3]) {
    // RTAPI uses game meters with the vertical axis in the middle component.
    return {position[0], position[2], position[1]};
}

bool IsSquadMarkerPlaced(const float position[3]) {
    if (!position) {
        return false;
    }

    if (!std::isfinite(position[0]) || !std::isfinite(position[1]) ||
        !std::isfinite(position[2])) {
        return false;
    }

    return std::fabs(position[0]) > 0.01f || std::fabs(position[1]) > 0.01f ||
           std::fabs(position[2]) > 0.01f;
}

std::optional<Vec3f> GetSquadMarkerPosition(int slotIndex) {
    if (!IsActive() || !g_rtapi || slotIndex < 0 || slotIndex >= kSquadMarkerSlotCount) {
        return std::nullopt;
    }

    const float* position = g_rtapi->SquadMarkers[slotIndex];
    if (!IsSquadMarkerPlaced(position)) {
        return std::nullopt;
    }

    return RtApiPositionToGame(position);
}

bool ImportSquadMarkerPosition(int slotIndex, MarkerCoord& marker) {
    const std::optional<Vec3f> position = GetSquadMarkerPosition(slotIndex);
    if (!position) {
        return false;
    }

    marker.icon = slotIndex + 1;
    marker.x = position->x;
    marker.y = position->y;
    marker.z = position->z;
    return true;
}

}  // namespace RtApiService
}  // namespace cm
