#pragma once

#include "services/Gw2ApiClient.h"

#include <optional>
#include <string>
#include <vector>

namespace cm {

struct RegisteredAccount {
    std::string tokenId;
    std::string keyName;
    std::string accountName;
    std::string apiKey;
    std::vector<std::string> permissions;
};

struct RegisterKeyResult {
    bool success = false;
    std::string message;
    RegisteredAccount account;
};

class AccountRegistry {
public:
    void Load(const std::string& path);
    void Save(const std::string& path) const;

    const std::vector<RegisteredAccount>& Accounts() const { return accounts_; }
    std::optional<std::string> ActiveApiKey() const;
    std::optional<std::string> ActiveAccountName() const;

    void SetActiveTokenId(const std::string& tokenId);
    RegisterKeyResult RegisterKey(Gw2ApiClient& client, const std::string& apiKey);
    bool RemoveKey(const std::string& tokenId);

private:
    std::vector<RegisteredAccount> accounts_;
    std::optional<std::string> activeTokenId_;
};

}  // namespace cm
