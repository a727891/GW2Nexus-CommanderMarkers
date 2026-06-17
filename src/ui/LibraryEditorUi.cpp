#include "ui/LibraryEditorUi.h"

#include "core/AppState.h"
#include "core/MumbleUtils.h"
#include "ui/LibrarySearchUi.h"
#include "ui/MarkerSetEditorWindow.h"
#include "ui/OptionsUiKit.h"
#include "ui/TextureService.h"

#include <algorithm>
#include <imgui.h>

namespace cm {
namespace LibraryEditorUi {
namespace {

constexpr float kIconColumnWidth = 36.0f;
constexpr float kIconSize = 24.0f;
constexpr float kActionsColumnWidth = 220.0f;
constexpr float kStatusColumnWidth = 96.0f;

char g_searchQuery[128] = "";

bool NuclearModifierHeld() {
    return ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift;
}

float UiScale(const AppState& state) {
    if (!state.nexusLink || state.nexusLink->Scaling <= 0.0f) {
        return 1.0f;
    }
    return state.nexusLink->Scaling;
}

float CalcLibraryRowHeight() {
    return ImGui::GetTextLineHeightWithSpacing() * 2.0f + ImGui::GetTextLineHeight();
}

void CenterCursorInCell(float columnWidth, float contentWidth, float rowHeight,
                        float contentHeight) {
    const ImVec2 cellPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(cellPos.x + (columnWidth - contentWidth) * 0.5f,
                               cellPos.y + (rowHeight - contentHeight) * 0.5f));
}

float ActionButtonHeight() { return ImGui::GetFrameHeight(); }

SquadMarker ListingIconForRow(size_t listingIndex) {
    return static_cast<SquadMarker>(((listingIndex + 1) % 8) + 1);
}

bool ActionIconButton(const char* id, ImTextureID icon, float height, const char* tooltip) {
    ImGui::PushID(id);
    const ImGuiStyle& style = ImGui::GetStyle();
    const float iconSize = std::max(12.0f, height - style.FramePadding.y * 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
                        ImVec2(style.FramePadding.x, (height - iconSize) * 0.5f));
    const bool pressed =
        icon ? ImGui::ImageButton(icon, ImVec2(iconSize, iconSize)) :
               ImGui::Button("?", ImVec2(height, height));
    ImGui::PopStyleVar();
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
    return pressed;
}

bool DrawEnabledStatusToggle(const char* id, bool* enabled) {
    const bool isActive = *enabled;
    const char* statusText = isActive ? "Active" : "Disabled";
    const ImVec4 statusColor =
        isActive ? ImVec4(0.55f, 0.95f, 0.55f, 1.0f) : ImVec4(0.85f, 0.55f, 0.55f, 1.0f);
    const ImTextureID icon =
        TextureService::GetUiTexture(isActive ? "check" : "clear");

    ImGui::PushID(id);

    const ImGuiStyle& style = ImGui::GetStyle();
    const float iconSize = 16.0f;
    const float height = ActionButtonHeight();
    const ImVec2 labelSize = ImGui::CalcTextSize(statusText);
    const float contentWidth = iconSize + style.ItemInnerSpacing.x + labelSize.x;

    const bool pressed = ImGui::Button("##status", ImVec2(-1.0f, height));
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const float yCenter = (min.y + max.y) * 0.5f;
    const float buttonWidth = max.x - min.x;

    float x = min.x + (buttonWidth - contentWidth) * 0.5f;
    if (icon) {
        const float y = yCenter - iconSize * 0.5f;
        ImGui::GetWindowDrawList()->AddImage(icon, ImVec2(x, y), ImVec2(x + iconSize, y + iconSize));
        x += iconSize + style.ItemInnerSpacing.x;
    }
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(x, yCenter - labelSize.y * 0.5f), ImGui::GetColorU32(statusColor), statusText);

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(isActive ? "Click to disable this marker set" :
                                     "Click to enable this marker set");
    }

    ImGui::PopID();

    if (pressed) {
        *enabled = !*enabled;
    }
    return pressed;
}

void RenderMarkerSetRow(AppState& state,
                        const MarkerSet& markerSet,
                        size_t listingIndex,
                        int currentMapId,
                        int* hoveredPreviewIndex) {
    using namespace OptionsUiKit;

    ImGui::PushID(static_cast<int>(listingIndex));
    ImGui::TableNextRow();

    if (!markerSet.enabled) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                               ImGui::GetColorU32(ImVec4(0.42f, 0.12f, 0.12f, 0.38f)));
    }

    const SquadMarker squadMarker = ListingIconForRow(listingIndex);
    const ImTextureID icon = markerSet.enabled ? TextureService::GetTexture(squadMarker) :
                                                 TextureService::GetTexture(SquadMarker::Clear);
    const float rowHeight = CalcLibraryRowHeight();
    const float actionHeight = ActionButtonHeight();
    const ImGuiStyle& style = ImGui::GetStyle();

    ImGui::TableSetColumnIndex(0);
    CenterCursorInCell(kIconColumnWidth, kIconSize, rowHeight, kIconSize);
    if (icon) {
        ImGui::Image(icon, ImVec2(kIconSize, kIconSize));
    } else {
        ImGui::Dummy(ImVec2(kIconSize, kIconSize));
    }

    ImGui::TableSetColumnIndex(1);
    if (!markerSet.enabled) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
    }
    ImGui::Text("%s", markerSet.name.c_str());
    if (!markerSet.description.empty()) {
        ImGui::TextDisabled("%s", markerSet.description.c_str());
    } else {
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
    }
    ImGui::TextDisabled("Map: %s | Markers: %zu",
                        state.mapData.Describe(markerSet.mapId).c_str(),
                        markerSet.markers.size());
    if (!markerSet.enabled) {
        ImGui::PopStyleColor();
    }

    ImGui::TableSetColumnIndex(2);
    const float editWidth =
        style.FramePadding.x * 2.0f + 16.0f + style.ItemInnerSpacing.x +
        ImGui::CalcTextSize("Edit").x;
    const float previewWidth = actionHeight;
    const float placeWidth = style.FramePadding.x * 2.0f + ImGui::CalcTextSize("Place").x;
    const bool showMapActions = markerSet.mapId == currentMapId;
    const float actionsWidth =
        showMapActions ? editWidth + style.ItemSpacing.x + previewWidth + style.ItemSpacing.x +
                             placeWidth :
                         editWidth;

    CenterCursorInCell(kActionsColumnWidth, actionsWidth, rowHeight, actionHeight);

    if (TexturedButton("edit", "Edit", TextureService::GetUiTexture("iconEdit"), 16.0f,
                       ImVec2(0.0f, actionHeight))) {
        MarkerSetEditorWindow::OpenExisting(state, markerSet, static_cast<int>(listingIndex));
    }

    if (showMapActions) {
        ImGui::SameLine(0.0f, style.ItemSpacing.x);
        if (ActionIconButton("preview", TextureService::GetUiTexture("eye"), actionHeight,
                             "Hover to preview markers on the map")) {
        }
        if (ImGui::IsItemHovered()) {
            *hoveredPreviewIndex = static_cast<int>(listingIndex);
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

    ImGui::TableSetColumnIndex(3);
    CenterCursorInCell(kStatusColumnWidth, kStatusColumnWidth, rowHeight, actionHeight);
    bool enabled = markerSet.enabled;
    if (DrawEnabledStatusToggle("enabled", &enabled)) {
        MarkerSet updated = markerSet;
        updated.enabled = enabled;
        state.markerListing.EditMarker(listingIndex, updated);
    }

    ImGui::PopID();
}

}  // namespace

void Render(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("AutoMarker Library");
    SectionSubtext("Manage saved marker sets. Use Edit to open the marker set editor window.");

    SettingCheckbox("Filter to current map", &state.settings.libraryFilterCurrentMap,
                    "Only show marker sets for your current map.");

    ImGui::Spacing();

    if (ImGui::Button("Add New")) {
        MarkerSetEditorWindow::OpenNew(state);
    }

    ImGui::SameLine();
    if (NuclearModifierHeld() && ImGui::Button("Reset Library To Default")) {
        state.markerListing.ResetToDefault();
        MarkerSetEditorWindow::Close();
    } else {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
        ImGui::Button("Reset Library To Default");
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Hold Ctrl+Shift to reset the library to defaults.");
        }
    }

    ImGui::SameLine();
    const float searchX =
        ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - LibrarySearchUi::kSearchFieldWidth;
    ImGui::SetCursorPosX(searchX);
    ImGui::SetNextItemWidth(LibrarySearchUi::kSearchFieldWidth);
    ImGui::InputTextWithHint("##cm_library_search", "Search", g_searchQuery,
                             sizeof(g_searchQuery));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Filter by marker set name, description, or map name.");
    }

    ImGui::Separator();

    const int currentMapId =
        state.mumbleLink ? static_cast<int>(state.mumbleLink->Context.MapID) : 0;
    const auto markerSets = state.markerListing.GetAllMarkerSets();
    const std::string searchLower = LibrarySearchUi::ToLowerCopy(g_searchQuery);

    if (ImGui::BeginChild("##cm_library_list", ImVec2(0.0f, 0.0f), false)) {
        bool anyVisible = false;

        if (ImGui::BeginTable("##cm_library_rows", 4,
                              ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("##icon", ImGuiTableColumnFlags_WidthFixed, kIconColumnWidth);
            ImGui::TableSetupColumn("##details", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed,
                                    kActionsColumnWidth);
            ImGui::TableSetupColumn("##status", ImGuiTableColumnFlags_WidthFixed,
                                    kStatusColumnWidth);

            int hoveredPreviewIndex = -1;
            for (size_t i = 0; i < markerSets.size(); ++i) {
                const MarkerSet& markerSet = markerSets[i];
                if (state.settings.libraryFilterCurrentMap && markerSet.mapId != currentMapId) {
                    continue;
                }
                if (!LibrarySearchUi::Matches(state, markerSet, searchLower)) {
                    continue;
                }

                anyVisible = true;
                RenderMarkerSetRow(state, markerSet, i, currentMapId, &hoveredPreviewIndex);
            }

            if (hoveredPreviewIndex >= 0 &&
                static_cast<size_t>(hoveredPreviewIndex) < markerSets.size()) {
                state.mapWatch.SetPreviewMarkerSet(
                    markerSets[static_cast<size_t>(hoveredPreviewIndex)]);
            } else if (state.mapWatch.IsManualPreview()) {
                state.mapWatch.RemovePreviewMarkerSet();
            }

            ImGui::EndTable();
        }

        if (!anyVisible) {
            DisabledGateText("No marker sets match the current filters.");
        }
    }
    ImGui::EndChild();
}

}  // namespace LibraryEditorUi
}  // namespace cm
