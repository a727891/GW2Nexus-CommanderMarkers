#include "data/MarkerSetClipboard.h"

#include "data/MarkerSetJson.h"
#include "utils/Base64Codec.h"

#include <nlohmann/json.hpp>

namespace cm {
namespace MarkerSetClipboard {

std::string ExportToBase64(const MarkerSet& markerSet) {
    if (!markerSet.communitySetId.empty() && !markerSet.syncDetached) {
        const nlohmann::json payload = {{"shareType", "community"},
                                        {"communitySetId", markerSet.communitySetId},
                                        {"name", markerSet.name}};
        return Base64Codec::EncodeUtf8(payload.dump());
    }
    return Base64Codec::EncodeUtf8(MarkerSetJson::MarkerSetToJson(markerSet).dump());
}

std::optional<MarkerSet> ImportFromText(const std::string& text, CommunityRefResolver resolveCommunity) {
    if (text.empty()) {
        return std::nullopt;
    }

    std::string json;
    if (const std::optional<std::string> decoded = Base64Codec::DecodeUtf8(text)) {
        json = *decoded;
    } else {
        json = text;
    }

    try {
        const auto j = nlohmann::json::parse(json);
        if (j.is_object()) {
            const std::string shareType = j.value("shareType", "");
            const bool communityRef = shareType == "community" ||
                                      (j.contains("communitySetId") && !j.contains("mapId"));
            if (communityRef) {
                const std::string communitySetId = j.value("communitySetId", "");
                const std::string name = j.value("name", "");
                if (communitySetId.empty() || name.empty() || !resolveCommunity) {
                    return std::nullopt;
                }
                MarkerSet resolved{};
                if (!resolveCommunity(communitySetId, name, resolved)) {
                    return std::nullopt;
                }
                return resolved;
            }
        }
        return MarkerSetJson::MarkerSetFromJson(j);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace MarkerSetClipboard
}  // namespace cm
