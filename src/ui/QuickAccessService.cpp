#include "ui/QuickAccessService.h"

#include "core/AppState.h"
#include "core/Branding.h"
#include "EmbeddedTextures.h"
#include "ui/TextureService.h"

#include <filesystem>
#include <imgui.h>
#include <string>

namespace cm {
namespace QuickAccessService {
namespace {

constexpr const char* kShortcutId = "CM_QUICKACCESS";
constexpr const char* kToggleBind = "CM_TOGGLE";
constexpr const char* kIconTex = "CM_ICON";
constexpr const char* kIconHoverTex = "CM_ICON_HOVER";
constexpr const char* kContextMenuId = "CM_CTX_MENU";

bool registered_ = false;
std::string cachedTooltip_;

void RenderContextMenu() {
    auto& state = AppState::Instance();
    auto& settings = state.settings;

    ImGui::Separator();

    if (ImGui::Selectable("Open Settings")) {
        (void)settings;
    }

    if (ImGui::MenuItem("Lieutenant mode", nullptr, &state.ltMode)) {
    }

    ImGui::Separator();
    ImGui::TextDisabled("Library (current map)");
    const int mapId = state.mumbleLink ? static_cast<int>(state.mumbleLink->Context.MapID) : 0;
    for (const MarkerSet& markerSet : state.markerListing.GetMarkersForMap(mapId)) {
        if (!markerSet.enabled) {
            continue;
        }
        if (ImGui::Selectable(markerSet.name.c_str())) {
            if (state.mumbleLink && state.nexusLink) {
                const float uiScale =
                    state.nexusLink->Scaling > 0.0f ? state.nexusLink->Scaling : 1.0f;
                state.mapWatch.PlaceMarkerSet(markerSet, state.mumbleLink, state.nexusLink, uiScale);
            }
        }
    }

    ImGui::Separator();
    if (ImGui::Selectable(settings.showMarkerPanel ? "Hide Clickable Markers"
                                                   : "Show Clickable Markers")) {
        settings.showMarkerPanel = !settings.showMarkerPanel;
        state.settings.Save(state.settingsPath());
    }
}

bool RegisterTextureAlias(AddonAPI_t* api, const char* identifier, const char* assetId) {
    if (!api || !api->Textures_GetOrCreateFromMemory) {
        return false;
    }

    const EmbeddedTextures::Asset* asset = EmbeddedTextures::Find(assetId);
    if (!asset) {
        return false;
    }

    return api->Textures_GetOrCreateFromMemory(
               identifier, const_cast<unsigned char*>(asset->data),
               static_cast<uint32_t>(asset->size)) != nullptr;
}

bool RegisterTextureFromFile(AddonAPI_t* api, const char* identifier,
                             const std::filesystem::path& path) {
    if (!api || !api->Textures_GetOrCreateFromFile) {
        return false;
    }
    if (!std::filesystem::exists(path)) {
        return false;
    }
    return api->Textures_GetOrCreateFromFile(identifier, path.string().c_str()) != nullptr;
}

void LoadTextures(AddonAPI_t* api, const AppState& state) {
    const char* markerAsset = "heart";
    switch (state.settings.cornerTexture) {
        case SquadMarker::Arrow:
            markerAsset = "arrow";
            break;
        case SquadMarker::Circle:
            markerAsset = "circle";
            break;
        case SquadMarker::Square:
            markerAsset = "square";
            break;
        case SquadMarker::Star:
            markerAsset = "star";
            break;
        case SquadMarker::Spiral:
            markerAsset = "spiral";
            break;
        case SquadMarker::Triangle:
            markerAsset = "triangle";
            break;
        case SquadMarker::Cross:
            markerAsset = "x";
            break;
        case SquadMarker::Heart:
        default:
            markerAsset = "heart";
            break;
    }

    if (!RegisterTextureAlias(api, kIconTex, markerAsset)) {
        RegisterTextureFromFile(api, kIconTex,
                                std::filesystem::path(state.addonDir) / "textures" /
                                    (std::string(markerAsset) + ".png"));
    }

    if (!RegisterTextureAlias(api, kIconHoverTex, "cornerIcon")) {
        RegisterTextureFromFile(api, kIconHoverTex,
                                std::filesystem::path(state.addonDir) / "textures" /
                                    "cornerIcon.png");
    }

    (void)TextureService::GetTexture(state.settings.cornerTexture);
}

std::string BuildTooltip(const AppState& state) {
    std::string tooltip = kDisplayName;
    if (state.mumbleLink && state.mumbleLink->Name[0] != L'\0') {
        tooltip += "\nUse the context menu for library and settings.";
    }
    return tooltip;
}

void ReregisterShortcut(AddonAPI_t* api, AppState& state) {
    if (!api || !registered_) return;

    api->QuickAccess_RemoveContextMenu(kContextMenuId);
    api->QuickAccess_Remove(kShortcutId);
    registered_ = false;
    Register(api, state);
}

void ExecuteLeftClickAction(AppState& state) {
    switch (state.settings.cornerIconAction) {
        case CornerIconAction::ShowSettings:
            break;
        case CornerIconAction::Library:
            break;
        case CornerIconAction::Lieutenant:
            state.ltMode = !state.ltMode;
            break;
        case CornerIconAction::ClickMarkerToggle:
            state.settings.showMarkerPanel = !state.settings.showMarkerPanel;
            state.settings.Save(state.settingsPath());
            break;
        case CornerIconAction::ShowIconMenu:
        default:
            break;
    }
}

}  // namespace

void Register(AddonAPI_t* api, AppState& state) {
    if (!api || registered_) return;

    LoadTextures(api, state);

    cachedTooltip_ = BuildTooltip(state);
    api->QuickAccess_Add(kShortcutId, kIconTex, kIconHoverTex, kToggleBind, cachedTooltip_.c_str());
    api->QuickAccess_AddContextMenu(kContextMenuId, kShortcutId, RenderContextMenu);
    registered_ = true;
}

void Unregister(AddonAPI_t* api) {
    if (!api || !registered_) return;

    api->QuickAccess_RemoveContextMenu(kContextMenuId);
    api->QuickAccess_Remove(kShortcutId);
    registered_ = false;
    cachedTooltip_.clear();
}

void Refresh(AddonAPI_t* api, AppState& state) {
    if (!api || !state.settings.cornerIconEnabled) return;

    if (!registered_) {
        Register(api, state);
        return;
    }

    const auto tooltip = BuildTooltip(state);
    if (tooltip != cachedTooltip_) {
        ReregisterShortcut(api, state);
    }
}

void OnShortcutActivated(AppState& state) { ExecuteLeftClickAction(state); }

void SyncVisibility(AddonAPI_t* api, AppState& state) {
    if (!api) return;
    if (state.settings.cornerIconEnabled) {
        Refresh(api, state);
    } else {
        Unregister(api);
    }
}

}  // namespace QuickAccessService
}  // namespace cm
