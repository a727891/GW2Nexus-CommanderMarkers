#pragma once

#include "data/MapDataCache.h"

#include <nlohmann/json.hpp>

namespace cm {
namespace Gw2MapJson {

inline bool ParseRectPoint(const nlohmann::json& pointJ, float& x, float& y) {
    if (!pointJ.is_array() || pointJ.empty()) {
        return false;
    }

    const nlohmann::json* coords = &pointJ;
    if (pointJ.size() == 1 && pointJ[0].is_array()) {
        coords = &pointJ[0];
    }
    if (!coords->is_array() || coords->size() < 2) {
        return false;
    }

    x = (*coords)[0].get<float>();
    y = (*coords)[1].get<float>();
    return true;
}

inline bool ParseCornerPair(const nlohmann::json& rectJ,
                            float& firstX,
                            float& firstY,
                            float& secondX,
                            float& secondY) {
    if (!rectJ.is_array() || rectJ.size() < 2) {
        return false;
    }

    return ParseRectPoint(rectJ[0], firstX, firstY) &&
           ParseRectPoint(rectJ[1], secondX, secondY);
}

inline Gw2Map FromJson(const nlohmann::json& j) {
    Gw2Map map{};
    if (!j.is_object()) {
        return map;
    }

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

inline nlohmann::json ToJson(const Gw2Map& map) {
    return {{"id", map.id},
            {"name", map.name},
            {"map_rect", {{map.mapRect.swX, map.mapRect.swY}, {map.mapRect.neX, map.mapRect.neY}}},
            {"continent_rect",
             {{map.continentRect.nwX, map.continentRect.nwY},
              {map.continentRect.seX, map.continentRect.seY}}}};
}

}  // namespace Gw2MapJson
}  // namespace cm
