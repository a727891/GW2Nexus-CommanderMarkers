#include "ui/TextureService.h"

#include "EmbeddedTextures.h"

#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace cm {
namespace {

AddonAPI_t* g_api = nullptr;
std::string g_addonDir;
std::mutex g_cacheMutex;
std::unordered_map<std::string, ImTextureID> g_textureCache;

ImTextureID TextureFromMemory(const char* identifier, const unsigned char* data, std::size_t size) {
    if (!g_api || !g_api->Textures_GetOrCreateFromMemory || !data || size == 0) {
        return nullptr;
    }

    Texture_t* texture = g_api->Textures_GetOrCreateFromMemory(
        identifier, const_cast<unsigned char*>(data), static_cast<uint32_t>(size));
    if (!texture || !texture->Resource) {
        return nullptr;
    }
    return reinterpret_cast<ImTextureID>(texture->Resource);
}

ImTextureID TextureFromFile(const char* identifier, const std::filesystem::path& path) {
    if (!g_api || !g_api->Textures_GetOrCreateFromFile) {
        return nullptr;
    }
    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    Texture_t* texture =
        g_api->Textures_GetOrCreateFromFile(identifier, path.string().c_str());
    if (!texture || !texture->Resource) {
        return nullptr;
    }
    return reinterpret_cast<ImTextureID>(texture->Resource);
}

}  // namespace

void TextureService::Initialize(AddonAPI_t* api, const std::string& addonDir) {
    g_api = api;
    g_addonDir = addonDir;
}

void TextureService::Shutdown() {
    {
        std::lock_guard lock(g_cacheMutex);
        g_textureCache.clear();
    }
    g_api = nullptr;
    g_addonDir.clear();
}

ImTextureID TextureService::LoadEmbedded(const char* assetId) {
    if (!assetId) {
        return nullptr;
    }

    {
        std::lock_guard lock(g_cacheMutex);
        const auto cached = g_textureCache.find(assetId);
        if (cached != g_textureCache.end() && cached->second) {
            return cached->second;
        }
    }

    const EmbeddedTextures::Asset* asset = EmbeddedTextures::Find(assetId);
    if (!asset) {
        return nullptr;
    }

    const ImTextureID texture =
        TextureFromMemory(asset->identifier, asset->data, asset->size);
    if (texture) {
        std::lock_guard lock(g_cacheMutex);
        g_textureCache[assetId] = texture;
    }
    return texture;
}

ImTextureID TextureService::LoadFromFile(const char* identifier, const std::string& relativePath) {
    return TextureFromFile(identifier, std::filesystem::path(g_addonDir) / relativePath);
}

const char* TextureService::SquadMarkerAssetId(SquadMarker marker) {
    switch (marker) {
        case SquadMarker::Arrow:
            return "arrow";
        case SquadMarker::Circle:
            return "circle";
        case SquadMarker::Heart:
            return "heart";
        case SquadMarker::Square:
            return "square";
        case SquadMarker::Star:
            return "star";
        case SquadMarker::Spiral:
            return "spiral";
        case SquadMarker::Triangle:
            return "triangle";
        case SquadMarker::Cross:
            return "x";
        case SquadMarker::Clear:
            return "clear";
        default:
            return nullptr;
    }
}

const char* TextureService::SquadMarkerFadedAssetId(SquadMarker marker) {
    switch (marker) {
        case SquadMarker::Arrow:
            return "arrow_fade";
        case SquadMarker::Circle:
            return "circle_fade";
        case SquadMarker::Heart:
            return "heart_fade";
        case SquadMarker::Square:
            return "square_fade";
        case SquadMarker::Star:
            return "star_fade";
        case SquadMarker::Spiral:
            return "spiral_fade";
        case SquadMarker::Triangle:
            return "triangle_fade";
        case SquadMarker::Cross:
            return "x_fade";
        default:
            return nullptr;
    }
}

ImTextureID TextureService::GetTexture(SquadMarker marker) {
    const char* assetId = SquadMarkerAssetId(marker);
    if (!assetId) {
        return nullptr;
    }

    if (ImTextureID tex = LoadEmbedded(assetId)) {
        return tex;
    }

    return LoadFromFile(("CM_FILE_" + std::string(assetId)).c_str(),
                        "textures/" + std::string(assetId) + ".png");
}

ImTextureID TextureService::GetFadedTexture(SquadMarker marker) {
    const char* assetId = SquadMarkerFadedAssetId(marker);
    if (!assetId) {
        return nullptr;
    }

    if (ImTextureID tex = LoadEmbedded(assetId)) {
        return tex;
    }

    return LoadFromFile(("CM_FILE_" + std::string(assetId)).c_str(),
                        "textures/" + std::string(assetId) + ".png");
}

ImTextureID TextureService::GetUiTexture(const char* name) {
    if (!name) {
        return nullptr;
    }

    if (ImTextureID tex = LoadEmbedded(name)) {
        return tex;
    }

    return LoadFromFile(("CM_UI_" + std::string(name)).c_str(),
                        "textures/" + std::string(name) + ".png");
}

ImTextureID TextureService::GetTriggerMarkerTexture() {
    if (ImTextureID tex = GetUiTexture("mapmarker")) {
        return tex;
    }
    return GetUiTexture("blish-heart");
}

}  // namespace cm
