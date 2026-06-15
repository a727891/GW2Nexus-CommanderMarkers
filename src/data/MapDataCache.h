#pragma once

#include "core/Types.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cm {

struct Gw2MapRect {
    float swX = 0.0f;
    float swY = 0.0f;
    float neX = 0.0f;
    float neY = 0.0f;

    float Width() const { return neX - swX; }
    float Height() const { return neY - swY; }
};

struct Gw2ContinentRect {
    float nwX = 0.0f;
    float nwY = 0.0f;
    float seX = 0.0f;
    float seY = 0.0f;

    float Width() const { return seX - nwX; }
    float Height() const { return seY - nwY; }
};

struct Gw2Map {
    int id = 0;
    std::string name;
    Gw2MapRect mapRect{};
    Gw2ContinentRect continentRect{};
};

class MapDataCache {
public:
    static constexpr const char* kCacheFilename = "map_data_cache.json";
    static constexpr float kMetersToInches = 39.3700787f;

    explicit MapDataCache(std::string addonDir = {});

    void SetAddonDir(std::string addonDir);

    bool LoadFromDisk();
    bool SaveToDisk() const;

    void SetBuildId(int buildId);
    int BuildId() const;

    void SetMaps(std::unordered_map<int, Gw2Map> maps);
    void UpsertMap(const Gw2Map& map);

    const Gw2Map* GetMap(int mapId) const;
    std::vector<Gw2Map> GetAllMaps() const;
    bool MapHasGeometry(int mapId) const;
    std::string Describe(int mapId) const;

    static Vec2f WorldToScreenMap(int mapId,
                                  const Vec3f& worldMeters,
                                  const ScreenMapData& screenData,
                                  const MapDataCache& cache);
    static Vec2f MapToScreenMap(const Vec2f& mapCoords, const ScreenMapData& screenData);
    static Vec2f WorldMetersToMap(const Gw2Map& map, const Vec3f& worldMeters);

private:
    static Vec2f WorldInchesToMap(const Gw2Map& map, const Vec3f& worldInches);

    std::string addonDir_;
    int buildId_ = 0;
    std::unordered_map<int, Gw2Map> maps_;
    mutable std::mutex mutex_;
};

}  // namespace cm
