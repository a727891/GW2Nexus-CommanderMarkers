#include "data/CommunityMeta.h"

#include "data/StaticDataLoader.h"

#include <nlohmann/json.hpp>

namespace cm {

bool CommunityMeta::Load(const std::string& addonDir, CommunityMeta& out) {
    std::string json;
    if (!StaticDataLoader::LoadCached(addonDir, kFilename, json)) {
        out = {};
        return false;
    }

    try {
        const auto j = nlohmann::json::parse(json);
        out.lastEdit = j.value("lastEdit", "");
        return true;
    } catch (...) {
        out = {};
        return false;
    }
}

bool CommunityMeta::Save(const std::string& addonDir, const CommunityMeta& meta) {
    const nlohmann::json j = {{"lastEdit", meta.lastEdit}};
    return StaticDataLoader::WriteCached(addonDir, kFilename, j.dump());
}

}  // namespace cm
