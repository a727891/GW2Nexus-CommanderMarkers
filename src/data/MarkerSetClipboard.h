#pragma once

#include "core/Types.h"

#include <functional>
#include <optional>
#include <string>

namespace cm {

using CommunityRefResolver =
    std::function<bool(const std::string& communitySetId, const std::string& name, MarkerSet& out)>;

namespace MarkerSetClipboard {

std::string ExportToBase64(const MarkerSet& markerSet);
std::optional<MarkerSet> ImportFromText(const std::string& text,
                                        CommunityRefResolver resolveCommunity = {});

}  // namespace MarkerSetClipboard
}  // namespace cm
