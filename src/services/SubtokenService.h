#pragma once

#include "services/ModuleManifestService.h"

#include <mutex>
#include <optional>
#include <string>

namespace cm {

class SubtokenService {
public:
    void SetManifest(const CommanderMarkersManifest* manifest);
    std::optional<std::string> GetSubtoken(const std::string& apiKey);

private:
    const CommanderMarkersManifest* manifest_ = nullptr;
    std::string cachedSubtoken_;
    std::string cachedApiKey_;
    std::string expiresAt_;
    std::mutex mutex_;
};

}  // namespace cm
