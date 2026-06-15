#pragma once

#include "core/AppState.h"
#include "core/Types.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace cm {
namespace LibrarySearchUi {

constexpr float kSearchFieldWidth = 220.0f;

inline std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

inline bool Matches(const AppState& state,
                    const MarkerSet& markerSet,
                    const std::string& queryLower) {
    if (queryLower.empty()) {
        return true;
    }

    const auto contains = [&](const std::string& text) {
        return ToLowerCopy(text).find(queryLower) != std::string::npos;
    };

    return contains(markerSet.name) || contains(markerSet.description) ||
           contains(state.mapData.Describe(markerSet.mapId));
}

}  // namespace LibrarySearchUi
}  // namespace cm
