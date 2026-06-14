#pragma once

#include "nexus/Nexus.h"

#include <imgui.h>
#include <string>

namespace cm {

class DatAssetIconService {
public:
    static constexpr int kInteractBubbleAssetId = 156775;

    static void Initialize(AddonAPI_t* api, const std::string& addonDir);
    static void Shutdown();
    static ImTextureID Request(int assetId);
    static void ProcessDownloads();

private:
    static ImTextureID LoadFromDisk(int assetId);
    static void QueueDownload(int assetId);
};

}  // namespace cm
