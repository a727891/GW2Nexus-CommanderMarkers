#include "services/SubtokenService.h"

#include "services/HttpClient.h"

#include <nlohmann/json.hpp>

namespace cm {

void SubtokenService::SetManifest(const CommanderMarkersManifest* manifest) {
    manifest_ = manifest;
}

std::optional<std::string> SubtokenService::GetSubtoken(const std::string& apiKey) {
    if (apiKey.empty() || !manifest_) {
        return std::nullopt;
    }

    {
        std::lock_guard lock(mutex_);
        if (!cachedSubtoken_.empty() && cachedApiKey_ == apiKey && !expiresAt_.empty()) {
            return cachedSubtoken_;
        }
    }

    nlohmann::json body = {{"apiKey", apiKey}};
    HttpRequestOptions options;
    const auto response =
        HttpPostJsonUrlEx(manifest_->Absolute(manifest_->subtokenUrl), body.dump(), options);
    if (response.statusCode < 200 || response.statusCode >= 300 || response.body.empty()) {
        return std::nullopt;
    }

    try {
        const auto j = nlohmann::json::parse(response.body);
        const std::string subtoken = j.value("subtoken", "");
        if (subtoken.empty()) {
            return std::nullopt;
        }
        std::lock_guard lock(mutex_);
        cachedSubtoken_ = subtoken;
        cachedApiKey_ = apiKey;
        expiresAt_ = j.value("expiresAt", "");
        return cachedSubtoken_;
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace cm
