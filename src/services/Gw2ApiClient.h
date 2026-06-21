#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace cm {

struct TokenInfo {
    bool valid = false;
    std::string id;
    std::string name;
    std::vector<std::string> permissions;
};

class Gw2ApiClient {
public:
    void SetApiKey(const std::string& key);
    TokenInfo FetchTokenInfo();
    std::optional<std::string> FetchAccountName();

private:
    std::optional<std::string> HttpGet(const std::string& path, bool auth);

    std::string apiKey_;
    std::mutex mutex_;
};

}  // namespace cm
