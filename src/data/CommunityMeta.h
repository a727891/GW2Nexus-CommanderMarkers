#pragma once

#include <string>

namespace cm {

struct CommunityMeta {
    std::string lastEdit;

    static constexpr const char* kFilename = "community_meta.json";

    static bool Load(const std::string& addonDir, CommunityMeta& out);
    static bool Save(const std::string& addonDir, const CommunityMeta& meta);
};

}  // namespace cm
