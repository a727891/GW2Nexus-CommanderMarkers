#include "services/Gw2ApiClient.h"

#include "services/HttpClient.h"

#include <nlohmann/json.hpp>

namespace cm {

void Gw2ApiClient::SetApiKey(const std::string& key) {
    std::lock_guard lock(mutex_);
    apiKey_ = key;
}

std::optional<std::string> Gw2ApiClient::HttpGet(const std::string& path, bool auth) {
    std::string token;
    {
        std::lock_guard lock(mutex_);
        if (auth && apiKey_.empty()) return std::nullopt;
        token = apiKey_;
    }
    HttpRequestOptions options;
    if (auth) options.bearerToken = token;
    return HttpGetUrl("https://api.guildwars2.com" + path, options);
}

TokenInfo Gw2ApiClient::FetchTokenInfo() {
    TokenInfo info;
    const auto body = HttpGet("/v2/tokeninfo", true);
    if (!body) return info;
    try {
        const auto j = nlohmann::json::parse(*body);
        info.valid = true;
        info.id = j.value("id", "");
        info.name = j.value("name", "");
        if (j.contains("permissions") && j["permissions"].is_array()) {
            for (const auto& p : j["permissions"]) {
                if (p.is_string()) info.permissions.push_back(p.get<std::string>());
            }
        }
    } catch (...) {
        info.valid = false;
    }
    return info;
}

std::optional<std::string> Gw2ApiClient::FetchAccountName() {
    const auto body = HttpGet("/v2/account", true);
    if (!body) return std::nullopt;
    try {
        const auto j = nlohmann::json::parse(*body);
        const auto name = j.value("name", "");
        if (name.empty()) return std::nullopt;
        return name;
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace cm
