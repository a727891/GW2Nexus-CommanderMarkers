#include "data/MapDataCache.h"

#include "data/Gw2MapJson.h"
#include "data/StaticDataLoader.h"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

namespace cm {

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
                auto map = Gw2MapJson::FromJson(mapJ);
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
            j["Maps"][std::to_string(id)] = Gw2MapJson::ToJson(map);
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

std::vector<Gw2Map> MapDataCache::GetAllMaps() const {
    std::lock_guard lock(mutex_);
    std::vector<Gw2Map> maps;
    maps.reserve(maps_.size());
    for (const auto& [id, map] : maps_) {
        (void)id;
        maps.push_back(map);
    }

    std::sort(maps.begin(), maps.end(), [](const Gw2Map& a, const Gw2Map& b) {
        if (a.name != b.name) {
            return a.name < b.name;
        }
        return a.id < b.id;
    });
    return maps;
}

bool MapDataCache::MapHasGeometry(int mapId) const {
    const Gw2Map* map = GetMap(mapId);
    if (!map) {
        return false;
    }
    return map->mapRect.Width() > 0.0f && map->mapRect.Height() > 0.0f &&
           map->continentRect.Width() > 0.0f && map->continentRect.Height() > 0.0f;
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
        map.continentRect.nwY +
            (map.mapRect.neY - worldInches.y) / mapHeight * continentHeight};
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
    const float rotatedX = translated.x * c - translated.y * s;
    const float rotatedY = translated.x * s + translated.y * c;
    // Anchor is in client pixels; apply real GW2 uiScale to the map offset only.
    return {rotatedX * screenData.uiScale + screenData.boundsCenter.x,
            rotatedY * screenData.uiScale + screenData.boundsCenter.y};
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
