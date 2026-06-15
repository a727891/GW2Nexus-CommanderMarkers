#include "services/CommunityMarkerService.h"

#include "core/Branding.h"
#include "data/CommunityMeta.h"
#include "data/MarkerSetJson.h"
#include "data/StaticDataLoader.h"
#include "services/HttpClient.h"

#include <nlohmann/json.hpp>

namespace cm {

CommunityMarkerService::CommunityMarkerService(std::string addonDir)
    : addonDir_(std::move(addonDir)) {}

void CommunityMarkerService::SetAddonDir(std::string addonDir) {
    addonDir_ = std::move(addonDir);
}

void CommunityMarkerService::LoadCached() {
    std::string json;
    if (!StaticDataLoader::LoadCached(addonDir_, kMarkersFilename, json)) {
        return;
    }

    try {
        const auto sets = MarkerSetJson::CommunitySetsFromJson(nlohmann::json::parse(json));
        std::lock_guard lock(mutex_);
        communitySets_ = sets;
    } catch (...) {
    }
}

CommunitySets CommunityMarkerService::GetCommunitySets() const {
    std::lock_guard lock(mutex_);
    return communitySets_;
}

bool CommunityMarkerService::SyncCommunity() {
    const auto body = HttpGetUrl(kCommunityMarkersUrl);
    if (!body || body->empty()) {
        return false;
    }

    try {
        const auto remoteSets = MarkerSetJson::CommunitySetsFromJson(nlohmann::json::parse(*body));

        CommunityMeta meta{};
        CommunityMeta::Load(addonDir_, meta);
        if (!remoteSets.lastEdit.empty() && remoteSets.lastEdit == meta.lastEdit) {
            return false;
        }

        const auto json = MarkerSetJson::CommunitySetsToJson(remoteSets).dump(2);
        if (!StaticDataLoader::WriteCached(addonDir_, kMarkersFilename, json)) {
            return false;
        }

        meta.lastEdit = remoteSets.lastEdit;
        CommunityMeta::Save(addonDir_, meta);

        {
            std::lock_guard lock(mutex_);
            communitySets_ = remoteSets;
        }
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace cm
