#include "data/MarkerSetClipboard.h"

#include "data/MarkerSetJson.h"
#include "utils/Base64Codec.h"

#include <nlohmann/json.hpp>

namespace cm {
namespace MarkerSetClipboard {

std::string ExportToBase64(const MarkerSet& markerSet) {
    const std::string json = MarkerSetJson::MarkerSetToJson(markerSet).dump();
    return Base64Codec::EncodeUtf8(json);
}

std::optional<MarkerSet> ImportFromText(const std::string& text) {
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
        return MarkerSetJson::MarkerSetFromJson(nlohmann::json::parse(json));
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace MarkerSetClipboard
}  // namespace cm
