#include "ui/CommunityLibraryUi.h"

#include "core/AppState.h"
#include "core/MumbleUtils.h"
#include "ui/OptionsUiKit.h"
#include "ui/TextureService.h"

#include <algorithm>
#include <imgui.h>

namespace cm {
namespace CommunityLibraryUi {
namespace {

struct UiState {
    int categoryIndex = 0;
};

UiState g_state;

bool NuclearModifierHeld() {
    return ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift;
}

float UiScale(const AppState& state) {
    if (!state.nexusLink || state.nexusLink->Scaling <= 0.0f) {
        return 1.0f;
    }
    return state.nexusLink->Scaling;
}

}  // namespace

void Render(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("Community Library");
    SectionSubtext("Browse and import community-created marker sets.");

    const CommunitySets& sets = state.communityMarkers.GetCommunitySets();
    const int currentMapId =
        state.mumbleLink ? static_cast<int>(state.mumbleLink->Context.MapID) : 0;

    if (sets.categories.empty()) {
        DisabledGateText("Community library not loaded yet.");
        if (ImGui::Button("Download Community Library")) {
            state.RequestCommunitySync();
        }
        return;
    }

    if (g_state.categoryIndex >= static_cast<int>(sets.categories.size())) {
        g_state.categoryIndex = 0;
    }

    std::vector<const char*> categoryLabels;
    categoryLabels.reserve(sets.categories.size());
    for (const auto& category : sets.categories) {
        categoryLabels.push_back(category.categoryName.c_str());
    }

    SettingCombo("Category", &g_state.categoryIndex, categoryLabels.data(),
                 static_cast<int>(categoryLabels.size()));
    SettingCheckbox("Filter to current map", &state.settings.libraryFilterCurrentMap);

    ImGui::SameLine();
    if (NuclearModifierHeld() && ImGui::Button("Redownload")) {
        state.RequestCommunitySync();
    } else {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
        ImGui::Button("Redownload");
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Hold Ctrl+Shift to force a community library redownload.");
        }
    }

    ImGui::Separator();

    const CommunityCategory& category = sets.categories[static_cast<size_t>(g_state.categoryIndex)];
    if (ImGui::BeginChild("##cm_community_list", ImVec2(0.0f, 360.0f), true)) {
        for (size_t i = 0; i < category.markerSets.size(); ++i) {
            const CommunityMarkerSet& markerSet = category.markerSets[i];
            if (state.settings.libraryFilterCurrentMap && markerSet.mapId != currentMapId) {
                continue;
            }

            ImGui::PushID(static_cast<int>(i));
            const int iconIndex = markerSet.markers.empty() ? 1 : markerSet.markers[0].icon;
            const SquadMarker squadMarker = static_cast<SquadMarker>(std::clamp(iconIndex, 1, 8));
            const ImTextureID icon = TextureService::GetTexture(squadMarker);

            if (icon) {
                ImGui::Image(icon, ImVec2(24.0f, 24.0f));
                ImGui::SameLine();
            }

            ImGui::BeginGroup();
            ImGui::Text("%s", markerSet.name.c_str());
            ImGui::TextDisabled("%s", markerSet.description.c_str());
            ImGui::TextDisabled("Map: %s | Author: %s",
                                state.mapData.Describe(markerSet.mapId).c_str(),
                                markerSet.author.c_str());
            ImGui::EndGroup();

            if (markerSet.mapId == currentMapId) {
                ImGui::SameLine();
                if (ImGui::SmallButton("Preview")) {
                    state.mapWatch.SetPreviewMarkerSet(markerSet);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Stop")) {
                    state.mapWatch.RemovePreviewMarkerSet();
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Place") && state.mumbleLink && state.nexusLink) {
                    state.mapWatch.PlaceMarkerSet(markerSet, state.mumbleLink, state.nexusLink,
                                                  UiScale(state));
                }
            }

            ImGui::SameLine();
            if (ImGui::SmallButton("Import")) {
                MarkerSet imported = markerSet;
                state.markerListing.SaveMarker(imported);
            }

            ImGui::Separator();
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    SectionSubtext(
        "Contribute marker sets via the Commander Markers static data repository on GitHub.");
}

}  // namespace CommunityLibraryUi
}  // namespace cm
