#include "data/SyncBaselineHash.h"

#include <algorithm>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>
#include <windows.h>
#include <wincrypt.h>

namespace cm {
namespace {

nlohmann::json CanonicalPayloadJson(const MarkerSet& markerSet) {
    std::vector<MarkerCoord> sorted = markerSet.markers;
    std::sort(sorted.begin(), sorted.end(),
              [](const MarkerCoord& a, const MarkerCoord& b) { return a.icon < b.icon; });

    nlohmann::json markers = nlohmann::json::array();
    for (const auto& marker : sorted) {
        nlohmann::json row = {{"i", marker.icon},
                              {"x", marker.x},
                              {"y", marker.y},
                              {"z", marker.z}};
        if (!marker.name.empty()) {
            row["d"] = marker.name;
        }
        markers.push_back(std::move(row));
    }

    nlohmann::json payload = {{"name", markerSet.name},
                              {"description", markerSet.description},
                              {"mapId", markerSet.mapId},
                              {"trigger",
                               {{"x", markerSet.trigger.x},
                                {"y", markerSet.trigger.y},
                                {"z", markerSet.trigger.z}}},
                              {"markers", std::move(markers)}};
    return payload;
}

std::string Sha256Hex(const std::string& text) {
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    if (!CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return {};
    }
    if (!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) {
        CryptReleaseContext(provider, 0);
        return {};
    }
    CryptHashData(hash, reinterpret_cast<const BYTE*>(text.data()),
                  static_cast<DWORD>(text.size()), 0);

    DWORD hashLen = 0;
    DWORD hashLenSize = sizeof(hashLen);
    CryptGetHashParam(hash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&hashLen), &hashLenSize, 0);
    std::vector<BYTE> buffer(hashLen);
    CryptGetHashParam(hash, HP_HASHVAL, buffer.data(), &hashLen, 0);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (DWORD i = 0; i < hashLen; ++i) {
        ss << std::setw(2) << static_cast<int>(buffer[i]);
    }

    CryptDestroyHash(hash);
    CryptReleaseContext(provider, 0);
    return ss.str();
}

}  // namespace

std::string SyncBaselineHash::Compute(const MarkerSet& markerSet) {
    const auto json = CanonicalPayloadJson(markerSet).dump();
    return Sha256Hex(json);
}

}  // namespace cm
