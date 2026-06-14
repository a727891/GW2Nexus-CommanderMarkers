#include "services/Gw2MapApiClient.h"

#include "core/Branding.h"
#include "services/HttpClient.h"

#include <chrono>
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>

namespace cm {

namespace {

constexpr const char* kMapsUrl = "https://api.guildwars2.com/v2/maps?ids=all";
constexpr int kMaxRetries = 3;
constexpr int kRetryDelayMs = 30000;

bool ParseCornerPair(const nlohmann::json& rectJ,
                     float& firstX,
                     float& firstY,
                     float& secondX,
                     float& secondY) {
    if (!rectJ.is_array() || rectJ.size() < 2) return false;

    const auto& first = rectJ[0];
    const auto& second = rectJ[1];
    if (!first.is_array() || first.size() < 2 || !second.is_array() || second.size() < 2) {
        return false;
    }

    firstX = first[0].get<float>();
    firstY = first[1].get<float>();
    secondX = second[0].get<float>();
    secondY = second[1].get<float>();
    return true;
}

Gw2Map Gw2MapFromJson(const nlohmann::json& j) {
    Gw2Map map{};
    if (!j.is_object()) return map;

    map.id = j.value("id", 0);
    map.name = j.value("name", "");

    if (j.contains("map_rect")) {
        ParseCornerPair(j["map_rect"], map.mapRect.swX, map.mapRect.swY, map.mapRect.neX,
                        map.mapRect.neY);
    }
    if (j.contains("continent_rect")) {
        ParseCornerPair(j["continent_rect"], map.continentRect.nwX, map.continentRect.nwY,
                        map.continentRect.seX, map.continentRect.seY);
    }

    return map;
}

void Log(AddonAPI_t* api, ELogLevel level, const char* message) {
    if (api) {
        api->Log(level, kLogChannel, message);
    }
}

}  // namespace

bool Gw2MapApiClient::NeedsRefresh(const MapDataCache& cache, int buildId) {
    return buildId != 0 && buildId != cache.BuildId();
}

bool Gw2MapApiClient::Refresh(MapDataCache& cache, int buildId, AddonAPI_t* api) {
    if (buildId == 0) {
        return false;
    }

    if (!NeedsRefresh(cache, buildId)) {
        return true;
    }

    std::optional<std::string> body;
    for (int attempt = 0; attempt < kMaxRetries; ++attempt) {
        body = HttpGetUrl(kMapsUrl);
        if (body && !body->empty()) {
            break;
        }

        if (attempt + 1 < kMaxRetries) {
            Log(api, LOGL_WARNING, "Failed to pull map data from the GW2 API. Retrying...");
            std::this_thread::sleep_for(std::chrono::milliseconds(kRetryDelayMs));
        }
    }

    if (!body || body->empty()) {
        Log(api, LOGL_WARNING, "Max retries exceeded. Skipping map data update.");
        return false;
    }

    try {
        const auto j = nlohmann::json::parse(*body);
        if (!j.is_array()) {
            Log(api, LOGL_WARNING, "GW2 maps API returned unexpected payload.");
            return false;
        }

        std::unordered_map<int, Gw2Map> maps;
        maps.reserve(j.size());
        for (const auto& mapJ : j) {
            Gw2Map map = Gw2MapFromJson(mapJ);
            if (map.id == 0) {
                continue;
            }
            maps[map.id] = std::move(map);
        }

        cache.SetMaps(std::move(maps));
        cache.SetBuildId(buildId);
        if (!cache.SaveToDisk()) {
            Log(api, LOGL_WARNING, "Failed to write map data cache.");
            return false;
        }

        Log(api, LOGL_INFO, "Updated map data cache from GW2 API.");
        return true;
    } catch (...) {
        Log(api, LOGL_WARNING, "Failed to parse GW2 maps API response.");
        return false;
    }
}

}  // namespace cm
