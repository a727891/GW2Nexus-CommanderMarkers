#pragma once

#include "core/Types.h"

#include "nexus/Nexus.h"

#include <imgui.h>
#include <string>

namespace cm {

class TextureService {
public:
    static void Initialize(AddonAPI_t* api, const std::string& addonDir);
    static void Shutdown();

    static ImTextureID GetTexture(SquadMarker marker);
    static ImTextureID GetFadedTexture(SquadMarker marker);
    static ImTextureID GetUiTexture(const char* name);
    static ImTextureID GetTriggerMarkerTexture();
    static const char* SquadMarkerAssetId(SquadMarker marker);
    static const char* SquadMarkerFadedAssetId(SquadMarker marker);

private:
    static ImTextureID LoadEmbedded(const char* assetId);
    static ImTextureID LoadFromFile(const char* identifier, const std::string& relativePath);
};

}  // namespace cm
