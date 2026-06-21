#include "data/MarkerSetClipboard.h"

#include "data/MarkerSetJson.h"
#include "utils/Base64Codec.h"

#include <cctype>
#include <nlohmann/json.hpp>

namespace cm {
namespace MarkerSetClipboard {
namespace {

std::string TrimImportText(std::string text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.erase(text.begin());
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.pop_back();
    }
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }
    return text;
}

bool IsCommunityShareRef(const nlohmann::json& j) {
    if (!j.is_object()) {
        return false;
    }

    if (j.value("shareType", "") == "community") {
        return true;
    }

    const std::string communitySetId = j.value("communitySetId", "");
    if (communitySetId.empty()) {
        return false;
    }

    const bool hasMapPayload =
        (j.contains("mapId") && !j["mapId"].is_null()) ||
        (j.contains("markers") && j["markers"].is_array() && !j["markers"].empty()) ||
        (j.contains("marks") && j["marks"].is_array() && !j["marks"].empty());
    return !hasMapPayload;
}

}  // namespace

std::string ExportToBase64(const MarkerSet& markerSet) {
    return Base64Codec::EncodeUtf8(MarkerSetJson::PortablePayloadToJson(markerSet).dump());
}

std::optional<MarkerSet> ImportFromText(const std::string& text, CommunityRefResolver resolveCommunity) {
    const std::string trimmed = TrimImportText(text);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    std::string json;
    if (const std::optional<std::string> decoded = Base64Codec::DecodeUtf8(trimmed)) {
        json = *decoded;
    } else {
        json = trimmed;
    }

    try {
        const auto j = nlohmann::json::parse(json);
        if (j.is_object() && IsCommunityShareRef(j)) {
            const std::string communitySetId = j.value("communitySetId", "");
            const std::string name = j.value("name", "");
            if (communitySetId.empty() || !resolveCommunity) {
                return std::nullopt;
            }
            MarkerSet resolved{};
            if (!resolveCommunity(communitySetId, name, resolved)) {
                return std::nullopt;
            }
            return resolved;
        }

        return MarkerSetJson::MarkerSetFromJson(j);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace MarkerSetClipboard
}  // namespace cm
