#include "ui/CommunityLibraryUi.h"

#include "core/AppState.h"
#include "core/MumbleUtils.h"
#include "ui/LibrarySearchUi.h"
#include "ui/OptionsUiKit.h"
#include "ui/TextureService.h"

#include <algorithm>
#include <imgui.h>
#include <string>

namespace cm {
namespace CommunityLibraryUi {
namespace {

struct UiState {
    int categoryIndex = 0;
};

UiState g_state;
char g_searchQuery[128] = "";

constexpr float kImportColumnWidth = 72.0f;
constexpr float kIconColumnWidth = 36.0f;
constexpr float kIconSize = 24.0f;
constexpr float kActionsColumnWidth = 180.0f;

bool NuclearModifierHeld() {
    return ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift;
}

float UiScale(const AppState& state) {
    if (!state.nexusLink || state.nexusLink->Scaling <= 0.0f) {
        return 1.0f;
    }
    return state.nexusLink->Scaling;
}

float CalcCommunityRowHeight() {
    return ImGui::GetTextLineHeightWithSpacing() * 2.0f + ImGui::GetTextLineHeight();
}

void CenterCursorInCell(float columnWidth, float contentWidth, float rowHeight,
                        float contentHeight) {
    const ImVec2 cellPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cellPos.x + (columnWidth - contentWidth) * 0.5f,
                               cellPos.y + (rowHeight - contentHeight) * 0.5f));
}

float ActionButtonHeight() { return ImGui::GetFrameHeight(); }

void NotifyImportSuccess(const AppState& state, const std::string& markerSetName) {
    if (!state.api || !state.api->GUI_SendAlert) {
        return;
    }

    const std::string message = markerSetName.empty() ?
                                    "Imported community marker set into your library." :
                                    "Imported \"" + markerSetName + "\" into your library.";
    state.api->GUI_SendAlert(message.c_str());
}

void RenderImportButton(AppState& state, const CommunityMarkerSet& markerSet) {
    using namespace OptionsUiKit;

    const bool alreadyImported = state.markerListing.ContainsMarkerSet(markerSet);
    const float actionHeight = ActionButtonHeight();
    const ImVec2 buttonSize(-1.0f, actionHeight);

    CenterCursorInCell(kImportColumnWidth, kImportColumnWidth, CalcCommunityRowHeight(),
                       actionHeight);

    if (alreadyImported) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
        TexturedButton("import_disabled", "Import", TextureService::GetUiTexture("iconImport"),
                       16.0f, buttonSize);
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Already in your library.");
        }
        return;
    }

    if (TexturedButton("import", "Import", TextureService::GetUiTexture("iconImport"), 16.0f,
                       buttonSize)) {
        MarkerSet imported = markerSet;
        state.markerListing.SaveMarker(imported);
        NotifyImportSuccess(state, markerSet.name);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Import this community marker set into your library.");
    }
}

void RenderCommunityRow(AppState& state,
                        const CommunityMarkerSet& markerSet,
                        size_t rowIndex,
                        int currentMapId) {
    using namespace OptionsUiKit;

    ImGui::PushID(static_cast<int>(rowIndex));
    ImGui::TableNextRow();

    const float rowHeight = CalcCommunityRowHeight();
    const float actionHeight = ActionButtonHeight();
    const ImGuiStyle& style = ImGui::GetStyle();

    ImGui::TableSetColumnIndex(0);
    RenderImportButton(state, markerSet);

    const int iconIndex = markerSet.markers.empty() ? 1 : markerSet.markers[0].icon;
    const SquadMarker squadMarker = static_cast<SquadMarker>(std::clamp(iconIndex, 1, 8));
    const ImTextureID icon = TextureService::GetTexture(squadMarker);

    ImGui::TableSetColumnIndex(1);
    CenterCursorInCell(kIconColumnWidth, kIconSize, rowHeight, kIconSize);
    if (icon) {
        ImGui::Image(icon, ImVec2(kIconSize, kIconSize));
    } else {
        ImGui::Dummy(ImVec2(kIconSize, kIconSize));
    }

    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%s", markerSet.name.c_str());
    if (!markerSet.description.empty()) {
        ImGui::TextDisabled("%s", markerSet.description.c_str());
    } else {
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
    }
    ImGui::TextDisabled("Map: %s | Author: %s",
                        state.mapData.Describe(markerSet.mapId).c_str(),
                        markerSet.author.c_str());

    ImGui::TableSetColumnIndex(3);
    if (markerSet.mapId == currentMapId) {
        const float previewWidth =
            style.FramePadding.x * 2.0f + ImGui::CalcTextSize("Preview").x;
        const float stopWidth =
            style.FramePadding.x * 2.0f + ImGui::CalcTextSize("Stop").x;
        const float placeWidth =
            style.FramePadding.x * 2.0f + ImGui::CalcTextSize("Place").x;
        const float actionsWidth = previewWidth + style.ItemSpacing.x + stopWidth +
                                   style.ItemSpacing.x + placeWidth;

        CenterCursorInCell(kActionsColumnWidth, actionsWidth, rowHeight, actionHeight);

        if (ImGui::Button("Preview", ImVec2(previewWidth, actionHeight))) {
            state.mapWatch.SetPreviewMarkerSet(markerSet);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Preview markers on the map.");
        }
        ImGui::SameLine(0.0f, style.ItemSpacing.x);
        if (ImGui::Button("Stop", ImVec2(stopWidth, actionHeight))) {
            state.mapWatch.RemovePreviewMarkerSet();
        }
        ImGui::SameLine(0.0f, style.ItemSpacing.x);
        if (ImGui::Button("Place", ImVec2(placeWidth, actionHeight)) && state.mumbleLink &&
            state.nexusLink) {
            state.mapWatch.PlaceMarkerSet(markerSet, state.mumbleLink, state.nexusLink,
                                          UiScale(state));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Place squad markers for this set.");
        }
    }

    ImGui::PopID();
}

}  // namespace

void Render(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("Community Library");
    SectionSubtext("Browse and import community-created marker sets.");

    const CommunitySets sets = state.communityMarkers.GetCommunitySets();
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

    ImGui::SameLine();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float redownloadWidth = style.FramePadding.x * 2.0f + ImGui::CalcTextSize("Redownload").x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - redownloadWidth);
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

    SettingCheckbox("Filter to current map", &state.settings.libraryFilterCurrentMap);
    ImGui::SameLine();
    SettingCheckbox("Only Show available", &state.settings.communityLibraryShowAvailable,
                    "Hide marker sets already in your library.");

    ImGui::SameLine();
    const float searchX =
        ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - LibrarySearchUi::kSearchFieldWidth;
    ImGui::SetCursorPosX(searchX);
    ImGui::SetNextItemWidth(LibrarySearchUi::kSearchFieldWidth);
    ImGui::InputTextWithHint("##cm_community_search", "Search", g_searchQuery, sizeof(g_searchQuery));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Filter by marker set name, description, or map name.");
    }

    ImGui::Separator();

    const CommunityCategory& category = sets.categories[static_cast<size_t>(g_state.categoryIndex)];
    const std::string searchLower = LibrarySearchUi::ToLowerCopy(g_searchQuery);

    if (ImGui::BeginChild("##cm_community_list", ImVec2(0.0f, 360.0f), true)) {
        bool anyVisible = false;

        if (ImGui::BeginTable("##cm_community_rows", 4,
                              ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("##import", ImGuiTableColumnFlags_WidthFixed, kImportColumnWidth);
            ImGui::TableSetupColumn("##icon", ImGuiTableColumnFlags_WidthFixed, kIconColumnWidth);
            ImGui::TableSetupColumn("##details", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed,
                                    kActionsColumnWidth);

            for (size_t i = 0; i < category.markerSets.size(); ++i) {
                const CommunityMarkerSet& markerSet = category.markerSets[i];
                if (state.settings.libraryFilterCurrentMap && markerSet.mapId != currentMapId) {
                    continue;
                }
                if (!LibrarySearchUi::Matches(state, markerSet, searchLower)) {
                    continue;
                }
                if (state.settings.communityLibraryShowAvailable &&
                    state.markerListing.ContainsMarkerSet(markerSet)) {
                    continue;
                }

                anyVisible = true;
                RenderCommunityRow(state, markerSet, i, currentMapId);
            }

            ImGui::EndTable();
        }

        if (!anyVisible) {
            DisabledGateText("No marker sets match the current filters.");
        }
    }
    ImGui::EndChild();

    SectionSubtext(
        "Contribute marker sets via the Commander Markers static data repository on GitHub.");
}

}  // namespace CommunityLibraryUi
}  // namespace cm
