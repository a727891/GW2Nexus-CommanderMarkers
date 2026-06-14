#include "data/MapDataCache.h"

#include "data/StaticDataLoader.h"

#include <cmath>
#include <nlohmann/json.hpp>

namespace cm {

namespace {

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

bool ParseMapRect(const nlohmann::json& rectJ, Gw2MapRect& out) {
    return ParseCornerPair(rectJ, out.swX, out.swY, out.neX, out.neY);
}

bool ParseContinentRect(const nlohmann::json& rectJ, Gw2ContinentRect& out) {
    return ParseCornerPair(rectJ, out.nwX, out.nwY, out.seX, out.seY);
}

Gw2Map Gw2MapFromJson(const nlohmann::json& j) {
    Gw2Map map{};
    if (!j.is_object()) return map;

    map.id = j.value("id", 0);
    map.name = j.value("name", "");

    if (j.contains("map_rect")) {
        ParseMapRect(j["map_rect"], map.mapRect);
    }
    if (j.contains("continent_rect")) {
        ParseContinentRect(j["continent_rect"], map.continentRect);
    }

    return map;
}

nlohmann::json Gw2MapToJson(const Gw2Map& map) {
    return {
        {"id", map.id},
        {"name", map.name},
        {"map_rect",
         {{{map.mapRect.swX, map.mapRect.swY}}, {{map.mapRect.neX, map.mapRect.neY}}}},
        {"continent_rect",
         {{{map.continentRect.nwX, map.continentRect.nwY}},
          {{map.continentRect.seX, map.continentRect.seY}}}}};
}

}  // namespace

MapDataCache::MapDataCache(std::string addonDir) : addonDir_(std::move(addonDir)) {}

void MapDataCache::SetAddonDir(std::string addonDir) { addonDir_ = std::move(addonDir); }

bool MapDataCache::LoadFromDisk() {
    std::string json;
    if (!StaticDataLoader::LoadCached(addonDir_, kCacheFilename, json)) {
        return false;
    }

    try {
        const auto j = nlohmann::json::parse(json);
        std::unordered_map<int, Gw2Map> maps;
        if (j.contains("Maps") && j["Maps"].is_object()) {
            for (const auto& [key, mapJ] : j["Maps"].items()) {
                auto map = Gw2MapFromJson(mapJ);
                if (map.id == 0) {
                    try {
                        map.id = std::stoi(key);
                    } catch (...) {
                        continue;
                    }
                }
                maps[map.id] = map;
            }
        }

        std::lock_guard lock(mutex_);
        buildId_ = j.value("BuildId", 0);
        maps_ = std::move(maps);
        return true;
    } catch (...) {
        return false;
    }
}

bool MapDataCache::SaveToDisk() const {
    nlohmann::json j;
    {
        std::lock_guard lock(mutex_);
        j["BuildId"] = buildId_;
        j["Maps"] = nlohmann::json::object();
        for (const auto& [id, map] : maps_) {
            j["Maps"][std::to_string(id)] = Gw2MapToJson(map);
        }
    }
    return StaticDataLoader::WriteCached(addonDir_, kCacheFilename, j.dump());
}

void MapDataCache::SetBuildId(int buildId) {
    std::lock_guard lock(mutex_);
    buildId_ = buildId;
}

int MapDataCache::BuildId() const {
    std::lock_guard lock(mutex_);
    return buildId_;
}

void MapDataCache::SetMaps(std::unordered_map<int, Gw2Map> maps) {
    std::lock_guard lock(mutex_);
    maps_ = std::move(maps);
}

void MapDataCache::UpsertMap(const Gw2Map& map) {
    std::lock_guard lock(mutex_);
    maps_[map.id] = map;
}

const Gw2Map* MapDataCache::GetMap(int mapId) const {
    std::lock_guard lock(mutex_);
    const auto it = maps_.find(mapId);
    return it == maps_.end() ? nullptr : &it->second;
}

std::string MapDataCache::Describe(int mapId) const {
    const Gw2Map* map = GetMap(mapId);
    if (!map || map->name.empty()) {
        return "(" + std::to_string(mapId) + ")";
    }
    return map->name;
}

Vec2f MapDataCache::WorldInchesToMap(const Gw2Map& map, const Vec3f& worldInches) {
    const float mapWidth = map.mapRect.Width();
    const float mapHeight = map.mapRect.Height();
    if (mapWidth <= 0.0f || mapHeight <= 0.0f) {
        return {};
    }

    const float continentWidth = map.continentRect.Width();
    const float continentHeight = map.continentRect.Height();

    return {
        map.continentRect.nwX +
            (worldInches.x - map.mapRect.swX) / mapWidth * continentWidth,
        map.continentRect.nwY -
            (worldInches.y - map.mapRect.swY) / mapHeight * continentHeight};
}

Vec2f MapDataCache::WorldMetersToMap(const Gw2Map& map, const Vec3f& worldMeters) {
    const Vec3f worldInches = {worldMeters.x * kMetersToInches, worldMeters.y * kMetersToInches,
                               worldMeters.z * kMetersToInches};
    return WorldInchesToMap(map, worldInches);
}

Vec2f MapDataCache::MapToScreenMap(const Vec2f& mapCoords, const ScreenMapData& screenData) {
    const Vec2f translated = {(mapCoords.x - screenData.mapCenter.x) * screenData.scale,
                              (mapCoords.y - screenData.mapCenter.y) * screenData.scale};

    const float c = std::cos(screenData.mapRotation);
    const float s = std::sin(screenData.mapRotation);
    return {translated.x * c - translated.y * s + screenData.boundsCenter.x,
            translated.x * s + translated.y * c + screenData.boundsCenter.y};
}

Vec2f MapDataCache::WorldToScreenMap(int mapId,
                                     const Vec3f& worldMeters,
                                     const ScreenMapData& screenData,
                                     const MapDataCache& cache) {
    const Gw2Map* map = cache.GetMap(mapId);
    if (!map) {
        return {};
    }

    const Vec2f mapCoords = WorldMetersToMap(*map, worldMeters);
    return MapToScreenMap(mapCoords, screenData);
}

}  // namespace cm
