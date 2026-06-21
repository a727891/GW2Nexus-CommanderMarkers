#include "data/MarkerSetJson.h"

namespace cm {

namespace {

WorldCoord WorldCoordFromJson(const nlohmann::json& j) {
    WorldCoord coord{};
    if (!j.is_object()) return coord;
    coord.x = j.value("x", 0.0f);
    coord.y = j.value("y", 0.0f);
    coord.z = j.value("z", 0.0f);
    return coord;
}

nlohmann::json WorldCoordToJson(const WorldCoord& coord) {
    return {{"x", coord.x}, {"y", coord.y}, {"z", coord.z}};
}

MarkerCoord MarkerCoordFromJson(const nlohmann::json& j) {
    MarkerCoord coord{};
    if (!j.is_object()) return coord;
    coord.icon = j.value("i", 0);
    coord.name = j.value("d", "");
    coord.x = j.value("x", 0.0f);
    coord.y = j.value("y", 0.0f);
    coord.z = j.value("z", 0.0f);
    return coord;
}

nlohmann::json MarkerCoordToJson(const MarkerCoord& coord) {
    nlohmann::json row = {{"i", coord.icon}, {"x", coord.x}, {"y", coord.y}, {"z", coord.z}};
    if (!coord.name.empty()) {
        row["d"] = coord.name;
    }
    return row;
}

void ReadMarkerSetFields(const nlohmann::json& j, MarkerSet& markerSet) {
    markerSet.id = j.value("id", "");
    markerSet.communitySetId = j.value("communitySetId", "");
    markerSet.author = j.value("author", "");
    markerSet.source = j.value("source", "");
    markerSet.communityUpdatedAt = j.value("communityUpdatedAt", "");
    markerSet.localModifiedAt = j.value("localModifiedAt", "");
    markerSet.syncBaselineHash = j.value("syncBaselineHash", "");
    markerSet.syncDetached = j.value("syncDetached", false);
    markerSet.name = j.value("name", "");
    markerSet.description = j.value("description", "");
    markerSet.mapId = j.value("mapId", 0);
    markerSet.enabled = j.value("enabled", true);
    if (j.contains("trigger")) {
        markerSet.trigger = WorldCoordFromJson(j["trigger"]);
    }
    markerSet.markers.clear();
    const nlohmann::json* markerArray = nullptr;
    if (j.contains("markers") && j["markers"].is_array()) {
        markerArray = &j["markers"];
    } else if (j.contains("marks") && j["marks"].is_array()) {
        markerArray = &j["marks"];
    }
    if (markerArray) {
        for (const auto& markerJ : *markerArray) {
            markerSet.markers.push_back(MarkerCoordFromJson(markerJ));
        }
    }
}

void WriteMarkerSetFields(const MarkerSet& markerSet, nlohmann::json& j) {
    if (!markerSet.id.empty()) j["id"] = markerSet.id;
    if (!markerSet.communitySetId.empty()) j["communitySetId"] = markerSet.communitySetId;
    if (!markerSet.author.empty()) j["author"] = markerSet.author;
    if (!markerSet.source.empty()) j["source"] = markerSet.source;
    if (!markerSet.communityUpdatedAt.empty()) j["communityUpdatedAt"] = markerSet.communityUpdatedAt;
    if (!markerSet.localModifiedAt.empty()) j["localModifiedAt"] = markerSet.localModifiedAt;
    if (!markerSet.syncBaselineHash.empty()) j["syncBaselineHash"] = markerSet.syncBaselineHash;
    if (markerSet.syncDetached) j["syncDetached"] = true;
    j["name"] = markerSet.name;
    j["description"] = markerSet.description;
    j["mapId"] = markerSet.mapId;
    j["enabled"] = markerSet.enabled;
    j["trigger"] = WorldCoordToJson(markerSet.trigger);
    j["markers"] = nlohmann::json::array();
    for (const auto& marker : markerSet.markers) {
        j["markers"].push_back(MarkerCoordToJson(marker));
    }
}

}  // namespace

MarkerSet MarkerSetJson::MarkerSetFromJson(const nlohmann::json& j) {
    MarkerSet markerSet{};
    if (!j.is_object()) return markerSet;
    ReadMarkerSetFields(j, markerSet);
    return markerSet;
}

nlohmann::json MarkerSetJson::MarkerSetToJson(const MarkerSet& markerSet) {
    nlohmann::json j = nlohmann::json::object();
    WriteMarkerSetFields(markerSet, j);
    return j;
}

CommunityMarkerSet MarkerSetJson::CommunityMarkerSetFromJson(const nlohmann::json& j) {
    CommunityMarkerSet markerSet{};
    if (!j.is_object()) return markerSet;
    ReadMarkerSetFields(j, markerSet);
    markerSet.author = j.value("author", "BlishHud Community");
    return markerSet;
}

nlohmann::json MarkerSetJson::CommunityMarkerSetToJson(const CommunityMarkerSet& markerSet) {
    nlohmann::json j = nlohmann::json::object();
    WriteMarkerSetFields(markerSet, j);
    j["author"] = markerSet.author;
    return j;
}

CommunityCategory MarkerSetJson::CommunityCategoryFromJson(const nlohmann::json& j) {
    CommunityCategory category{};
    if (!j.is_object()) return category;
    category.categoryName = j.value("categoryName", "Community Created");
    category.markerSets.clear();
    if (j.contains("markerSets") && j["markerSets"].is_array()) {
        for (const auto& markerSetJ : j["markerSets"]) {
            category.markerSets.push_back(CommunityMarkerSetFromJson(markerSetJ));
        }
    }
    return category;
}

nlohmann::json MarkerSetJson::CommunityCategoryToJson(const CommunityCategory& category) {
    nlohmann::json j = nlohmann::json::object();
    j["categoryName"] = category.categoryName;
    j["markerSets"] = nlohmann::json::array();
    for (const auto& markerSet : category.markerSets) {
        j["markerSets"].push_back(CommunityMarkerSetToJson(markerSet));
    }
    return j;
}

CommunitySets MarkerSetJson::CommunitySetsFromJson(const nlohmann::json& j) {
    CommunitySets sets{};
    if (!j.is_object()) return sets;
    sets.lastEdit = j.value("lastEdit", "");
    sets.categories.clear();
    if (j.contains("categories") && j["categories"].is_array()) {
        for (const auto& categoryJ : j["categories"]) {
            sets.categories.push_back(CommunityCategoryFromJson(categoryJ));
        }
    }
    return sets;
}

nlohmann::json MarkerSetJson::CommunitySetsToJson(const CommunitySets& sets) {
    nlohmann::json j = nlohmann::json::object();
    j["lastEdit"] = sets.lastEdit;
    j["categories"] = nlohmann::json::array();
    for (const auto& category : sets.categories) {
        j["categories"].push_back(CommunityCategoryToJson(category));
    }
    return j;
}

MarkerListingFile MarkerSetJson::MarkerListingFileFromJson(const nlohmann::json& j) {
    MarkerListingFile listing{};
    if (!j.is_object()) return listing;
    listing.version = j.value("version", "2.0.0");
    listing.migratedAt = j.value("migratedAt", "");
    listing.squadMarkerPreset.clear();
    if (j.contains("squadMarkerPreset") && j["squadMarkerPreset"].is_array()) {
        for (const auto& markerSetJ : j["squadMarkerPreset"]) {
            listing.squadMarkerPreset.push_back(MarkerSetFromJson(markerSetJ));
        }
    }
    return listing;
}

nlohmann::json MarkerSetJson::MarkerListingFileToJson(const MarkerListingFile& listing) {
    nlohmann::json j = nlohmann::json::object();
    j["version"] = listing.version;
    if (!listing.migratedAt.empty()) {
        j["migratedAt"] = listing.migratedAt;
    }
    j["squadMarkerPreset"] = nlohmann::json::array();
    for (const auto& markerSet : listing.squadMarkerPreset) {
        j["squadMarkerPreset"].push_back(MarkerSetToJson(markerSet));
    }
    return j;
}

nlohmann::json MarkerSetJson::PortablePayloadToJson(const MarkerSet& markerSet) {
    nlohmann::json payload = nlohmann::json::object();
    payload["name"] = markerSet.name;
    payload["description"] = markerSet.description;
    payload["mapId"] = markerSet.mapId;
    payload["enabled"] = markerSet.enabled;
    payload["trigger"] = WorldCoordToJson(markerSet.trigger);
    payload["markers"] = nlohmann::json::array();
    for (const auto& marker : markerSet.markers) {
        payload["markers"].push_back(MarkerCoordToJson(marker));
    }
    return payload;
}

nlohmann::json MarkerSetJson::SubmissionPayloadToJson(const MarkerSet& markerSet,
                                                      const std::string& suggestedCategory) {
    nlohmann::json payload = PortablePayloadToJson(markerSet);
    payload["suggestedCategory"] = suggestedCategory;
    return payload;
}

}  // namespace cm
