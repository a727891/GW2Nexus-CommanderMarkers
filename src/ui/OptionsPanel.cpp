#include "ui/OptionsPanel.h"

#include "core/AppState.h"
#include "core/Branding.h"
#include "data/MarkerDefinition.h"
#include "ui/CommunityLibraryUi.h"
#include "ui/LibraryEditorUi.h"
#include "ui/MarkersPanel.h"
#include "ui/OptionsUiKit.h"
#include "ui/TextureService.h"

#include <imgui.h>
#include <vector>

namespace cm {
namespace OptionsPanel {
namespace {

void RenderClickableMarkersTab(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("Clickable Markers");
    SectionSubtext("Configure the on-screen squad marker panel.");

    SettingCheckbox("Show clickable markers on screen", &state.settings.showMarkerPanel);
    SettingCheckbox("Only when commander", &state.settings.onlyCommanderPanel);
    SettingCheckbox("Drag panel to reposition", &state.settings.dragPanel);
    SettingCheckbox("Ground markers enabled", &state.settings.groundMarkersEnabled);
    SettingCheckbox("Object markers enabled", &state.settings.objectMarkersEnabled);

    SettingSliderInt("Icon width", &state.settings.iconWidth, 16, 64);
    SettingSliderFloat("Panel opacity", &state.settings.panelOpacity, 0.2f, 1.0f);

    static const char* kLayouts[] = {"Vertical", "Horizontal"};
    int layoutIndex = static_cast<int>(state.settings.panelOrientation);
    if (SettingCombo("Panel orientation", &layoutIndex, kLayouts, 2)) {
        state.settings.panelOrientation =
            layoutIndex == 0 ? PanelLayout::Vertical : PanelLayout::Horizontal;
    }

    ImGui::Spacing();
    SectionSubtext("Preview");
    MarkersPanel::Render(state, true);
}

void RenderAutoMarkerSettingsTab(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("AutoMarker Settings");
    SectionSubtext("Saved marker sets, map icons, and in-world billboards.");

    SettingCheckbox("AutoMarker feature enabled", &state.settings.autoMarkerEnabled);
    SettingCheckbox("Only when commander", &state.settings.autoMarkerOnlyCommander);
    SettingCheckbox("Show trigger hearts on map/compass", &state.settings.autoMarkerShowTrigger);
    SettingCheckbox("Show preview markers on map/compass", &state.settings.autoMarkerShowPreview);

    SectionHeading("In Game World Icons");
    SettingCheckbox("Billboard feature enabled", &state.settings.billboardEnabled);
    SettingCheckbox("Allow placement prompt when map is closed", &state.settings.billboardPlacement);
    SettingCheckbox("Show preview markers near trigger", &state.settings.billboardPreview);
    SettingCheckbox("Allow placement while in combat", &state.settings.combatPlacement);
    SettingSliderInt("Placement delay (ms)", &state.settings.placementDelayMs, 25, 500);
}

void RenderGeneralTab(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("General");

    if (!state.requiredBindsOk) {
        WarningBanner(
            "One or more squad marker keybinds are missing in Guild Wars 2. Configure ground "
            "marker binds under Squad Markers in game options.");
    }

    SettingCheckbox("Corner icon enabled", &state.settings.cornerIconEnabled);

    static const char* kCornerActions[] = {"Show icon menu", "Show settings", "Open library",
                                           "Toggle lieutenant mode", "Toggle clickable markers"};
    int actionIndex = static_cast<int>(state.settings.cornerIconAction);
    if (SettingCombo("Corner icon left-click action", &actionIndex, kCornerActions, 5)) {
        state.settings.cornerIconAction = static_cast<CornerIconAction>(actionIndex);
    }

    std::vector<const char*> markerLabels;
    std::vector<int> markerValues;
    for (std::size_t i = 0; i < kMarkerDefinitionCount; ++i) {
        if (kAllMarkers[i].markerType == SquadMarker::None ||
            kAllMarkers[i].markerType == SquadMarker::Clear) {
            continue;
        }
        markerLabels.push_back(kAllMarkers[i].displayName);
        markerValues.push_back(static_cast<int>(kAllMarkers[i].markerType));
    }

    int textureIndex = 0;
    for (size_t i = 0; i < markerValues.size(); ++i) {
        if (markerValues[i] == static_cast<int>(state.settings.cornerTexture)) {
            textureIndex = static_cast<int>(i);
            break;
        }
    }

    if (SettingCombo("Corner icon texture", &textureIndex, markerLabels.data(),
                     static_cast<int>(markerLabels.size()))) {
        state.settings.cornerTexture = static_cast<SquadMarker>(markerValues[textureIndex]);
    }

    SettingSliderInt("Corner icon priority", &state.settings.cornerPriority, 0, 999);

    const ImTextureID preview = TextureService::GetTexture(state.settings.cornerTexture);
    if (preview) {
        ImGui::Image(preview, ImVec2(48.0f, 48.0f));
    }
}

}  // namespace

void Render(AppState& state) {
    using namespace OptionsUiKit;

    ImGui::TextColored(GoldColor(), kDisplayName);
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("##cm_options_tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Clickable Markers")) {
            BeginContentPanel();
            RenderClickableMarkersTab(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("AutoMarker Settings")) {
            BeginContentPanel();
            RenderAutoMarkerSettingsTab(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("AutoMarker Library")) {
            BeginContentPanel();
            LibraryEditorUi::Render(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Community Library")) {
            BeginContentPanel();
            CommunityLibraryUi::Render(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("General")) {
            BeginContentPanel();
            RenderGeneralTab(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    state.settings.Save(state.settingsPath());
}

}  // namespace OptionsPanel
}  // namespace cm
