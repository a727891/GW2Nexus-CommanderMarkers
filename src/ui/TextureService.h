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
    static ImTextureID GetUiTexture(const char* name);

private:
    static ImTextureID LoadEmbedded(const char* assetId);
    static ImTextureID LoadFromFile(const char* identifier, const std::string& relativePath);
    static const char* SquadMarkerAssetId(SquadMarker marker);
};

}  // namespace cm
