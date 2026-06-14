#pragma once

#include "core/Types.h"

#include <nlohmann/json.hpp>
#include <string>

namespace cm {

class MarkerSetJson {
public:
    static MarkerSet MarkerSetFromJson(const nlohmann::json& j);
    static nlohmann::json MarkerSetToJson(const MarkerSet& markerSet);

    static CommunityMarkerSet CommunityMarkerSetFromJson(const nlohmann::json& j);
    static nlohmann::json CommunityMarkerSetToJson(const CommunityMarkerSet& markerSet);

    static CommunityCategory CommunityCategoryFromJson(const nlohmann::json& j);
    static nlohmann::json CommunityCategoryToJson(const CommunityCategory& category);

    static CommunitySets CommunitySetsFromJson(const nlohmann::json& j);
    static nlohmann::json CommunitySetsToJson(const CommunitySets& sets);

    static MarkerListingFile MarkerListingFileFromJson(const nlohmann::json& j);
    static nlohmann::json MarkerListingFileToJson(const MarkerListingFile& listing);
};

}  // namespace cm
