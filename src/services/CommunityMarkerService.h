#pragma once

#include "core/Types.h"

#include <mutex>
#include <string>

namespace cm {

class CommunityMarkerService {
public:
    static constexpr const char* kMarkersFilename = "Markers.json";

    explicit CommunityMarkerService(std::string addonDir = {});

    void SetAddonDir(std::string addonDir);

    void LoadCached();
    bool SyncCommunity();
    const CommunitySets& GetCommunitySets() const;

private:
    std::string addonDir_;
    CommunitySets communitySets_{};
    mutable std::mutex mutex_;
};

}  // namespace cm
