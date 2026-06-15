#include "ui/SettingsWindow.h"

#include "core/AppState.h"
#include "core/Branding.h"
#include "data/MarkerDefinition.h"
#include "services/RtApiService.h"
#include "ui/CommunityLibraryUi.h"
#include "ui/LibraryEditorUi.h"
#include "ui/MarkersPanel.h"
#include "ui/OptionsUiKit.h"

#include <imgui.h>
#include <shellapi.h>
#include <vector>

namespace cm {
namespace OptionsPanel {
namespace {

void OpenExternalUrl(const char* url) {
    if (!url || url[0] == '\0') {
        return;
    }
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

void StatusIndicator(const char* label, bool ok, const char* okText, const char* noText) {
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    if (ok) {
        ImGui::TextColored(ImVec4(0.45f, 0.95f, 0.45f, 1.0f), "%s", okText);
    } else {
        ImGui::TextDisabled("%s", noText);
    }
}

ImGuiTabItemFlags PendingTabFlags(SettingsTab tab, SettingsTab pendingTab, bool applyPendingTab) {
    if (applyPendingTab && tab == pendingTab) {
        return ImGuiTabItemFlags_SetSelected;
    }
    return ImGuiTabItemFlags_None;
}

void RenderAboutTab() {
    using namespace OptionsUiKit;

    SectionHeading("About");
    ImGui::TextWrapped(
        "%s is a Nexus port of the BlishHUD Commander Markers module. Place squad markers "
        "with a click, or auto-place saved marker sets near triggers.",
        kDisplayName);

    ImGui::Spacing();
    ImGui::Text("Version: %s", kVersionString);
    ImGui::Text("Author: %s", kAuthor);

    ImGui::Spacing();
    if (ImGui::Button("Patch Notes")) {
        OpenExternalUrl(kPatchNotesUrl);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open patch notes in your default web browser.");
    }

    SectionHeading("Thank you to:");
    ImGui::BulletText("Freesnow — continued static data and config file hosting");
    ImGui::BulletText("Manlaan — original clickable markers in BlishHUD");

    SectionHeading("Links");
    ImGui::TextWrapped("Original module:");
    ImGui::TextWrapped("%s", kSourceRepoUrl);
    ImGui::Spacing();
    ImGui::TextWrapped("Community marker library:");
    ImGui::TextWrapped("%s", kCommunityMarkersUrl);

    SectionHeading("Integrations");
    SectionSubtext("Optional co-modules detected at runtime.");
    const bool rtapiDetected = RtApiService::IsDetected();
    const bool rtapiActive = RtApiService::IsActive();
    StatusIndicator("Real-Time API detected:", rtapiDetected, "Yes", "No");
    StatusIndicator("Real-Time API active:", rtapiActive, "Yes", "No");
    if (!rtapiDetected) {
        SectionSubtext(
            "Install Real-Time API from the Nexus addon library to enable squad "
            "commander/lieutenant permission detection.");
    } else if (!rtapiActive) {
        SectionSubtext("Real-Time API is installed but not currently loaded or ready.");
    } else {
        SectionSubtext(
            "Squad commander and lieutenant roles are available for commander permission checks.");
    }
}

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

    static const char* kLayoutLabels[] = {"Stacked", "Side-by-side", "Single row",
                                          "Single column"};
    static constexpr PanelLayout kLayoutOrder[] = {PanelLayout::Stacked, PanelLayout::SideBySide,
                                                   PanelLayout::SingleRow,
                                                   PanelLayout::SingleColumn};

    int layoutIndex = 0;
    for (int i = 0; i < 4; ++i) {
        if (kLayoutOrder[i] == state.settings.panelOrientation) {
            layoutIndex = i;
            break;
        }
    }
    if (SettingCombo("Panel orientation", &layoutIndex, kLayoutLabels, 4)) {
        state.settings.panelOrientation = kLayoutOrder[layoutIndex];
    }

    ImGui::Spacing();
    SectionSubtext("Preview");
    if (state.IsReady()) {
        MarkersPanel::Render(state, true);
    } else {
        DisabledGateText("Preview available after entering gameplay.");
    }
}

void RenderAutoMarkerSettingsTab(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("AutoMarker Settings");
    SectionSubtext("Allows for rapidly placing saved marker sets.");

    SettingCheckbox("Enable", &state.settings.autoMarkerEnabled,
                    "Enable/Disable the entire AutoMarker feature.");
    SettingCheckbox("Only show when I am the Commander", &state.settings.autoMarkerOnlyCommander,
                    "Only show the AutoMarker activation zones on the map when you are the "
                    "Commander.");

    SectionHeading("Map Icons");
    SettingCheckbox("Enable Map Marker", &state.settings.autoMarkerShowTrigger,
                    "Display the map marker icon in locations where AutoMarker sets may be "
                    "activated from the map.");
    SettingCheckbox("Show preview when map is open", &state.settings.autoMarkerShowPreview,
                    "Show a preview of the markers when the map is open and you are close "
                    "enough to place the set.");

    SectionHeading("In Game World Icons");
    SettingCheckbox("Enable markers in 3D game world", &state.settings.billboardEnabled,
                    "Show markers in the 3D game world.");
    SettingCheckbox("Allow placement without having the map open",
                    &state.settings.billboardPlacement,
                    "Allow the marker placement even when the map is closed.");
    SettingCheckbox("Preview marker set when near trigger", &state.settings.billboardPreview,
                    "Show a preview of the markers to be placed in the game world.");

    static const char* kTriggerMarkerSizes[] = {"Small", "Normal", "Large"};
    int triggerSizeIndex = static_cast<int>(state.settings.triggerMarkerSize);
    if (SettingCombo("In-world trigger marker size", &triggerSizeIndex, kTriggerMarkerSizes, 3,
                     "Size of the trigger marker icon shown in the 3D game world.")) {
        state.settings.triggerMarkerSize = static_cast<TriggerMarkerSize>(triggerSizeIndex);
    }

    SettingCheckbox("Allow AutoMarker features while In Combat", &state.settings.combatPlacement,
                    "AutoMarker features will be available while in combat.");
    SettingSliderInt("Placement delay", &state.settings.placementDelayMs, 25, 500,
                     "Delay in milliseconds to wait between marker placement.");

    SectionHeading("Library Editor");
    SettingCheckbox("Confirm before closing with unsaved changes",
                    &state.settings.libraryEditorConfirmUnsaved,
                    "Prompt to save or discard when closing the marker set editor.");
}

void RenderGeneralTab(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("General");

    if (!state.requiredBindsOk) {
        WarningBanner(
            "One or more squad marker keybinds are missing in Guild Wars 2. Configure ground "
            "and object marker binds under Squad Markers in game options.");
    }

    SettingCheckbox("Quick access icon enabled", &state.settings.cornerIconEnabled);

    static const char* kCornerActions[] = {"Show icon menu", "Show settings", "Open library",
                                           "Toggle lieutenant mode", "Toggle clickable markers"};
    int actionIndex = static_cast<int>(state.settings.cornerIconAction);
    if (SettingCombo("Quick access icon left-click action", &actionIndex, kCornerActions, 5)) {
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

    if (SettingCombo("Quick access icon texture", &textureIndex, markerLabels.data(),
                     static_cast<int>(markerLabels.size()))) {
        state.settings.cornerTexture = static_cast<SquadMarker>(markerValues[textureIndex]);
    }
}

}  // namespace

void RenderNexusConfigEntry(AppState& state) {
    using namespace OptionsUiKit;

    (void)state;

    ImGui::TextColored(GoldColor(), "%s", kDisplayName);
    ImGui::Spacing();
    ImGui::TextWrapped("%s", kDescription);
    ImGui::Spacing();
    ImGui::TextWrapped(
        "Configure clickable markers, AutoMarker, marker libraries, and quick access from a "
        "dedicated in-game settings window.");
    ImGui::Spacing();
    ImGui::Spacing();

    const ImVec2 buttonSize(ImGui::GetContentRegionAvail().x, 52.0f);
    if (ImGui::Button("Open Commander Markers Settings", buttonSize)) {
        SettingsWindow::Open();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Opens the full settings window.");
    }
}

void RenderWindow(AppState& state, SettingsTab pendingTab, bool applyPendingTab) {
    using namespace OptionsUiKit;

    ImGui::TextColored(GoldColor(), kDisplayName);
    ImGui::Separator();
    ImGui::Spacing();

    if (!state.IsReady()) {
        DisabledGateText("Enter gameplay to load marker libraries and previews.");
    }

    if (ImGui::BeginTabBar("##cm_options_tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Clickable Markers", nullptr,
                                PendingTabFlags(SettingsTab::ClickableMarkers, pendingTab,
                                                applyPendingTab))) {
            BeginContentPanel();
            RenderClickableMarkersTab(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("AutoMarker Settings", nullptr,
                                PendingTabFlags(SettingsTab::AutoMarkerSettings, pendingTab,
                                                applyPendingTab))) {
            BeginContentPanel();
            RenderAutoMarkerSettingsTab(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("AutoMarker Library", nullptr,
                                PendingTabFlags(SettingsTab::AutoMarkerLibrary, pendingTab,
                                                applyPendingTab))) {
            BeginContentPanel();
            if (state.IsReady()) {
                LibraryEditorUi::Render(state);
            } else {
                DisabledGateText("Available after entering gameplay.");
            }
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Community Library", nullptr,
                                PendingTabFlags(SettingsTab::CommunityLibrary, pendingTab,
                                                applyPendingTab))) {
            BeginContentPanel();
            if (state.IsReady()) {
                CommunityLibraryUi::Render(state);
            } else {
                DisabledGateText("Available after entering gameplay.");
            }
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("General", nullptr,
                                PendingTabFlags(SettingsTab::General, pendingTab,
                                                applyPendingTab))) {
            BeginContentPanel();
            RenderGeneralTab(state);
            EndContentPanel();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("About", nullptr,
                                PendingTabFlags(SettingsTab::About, pendingTab, applyPendingTab))) {
            BeginContentPanel();
            RenderAboutTab();
            EndContentPanel();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    if (state.IsOptionsReady()) {
        state.settings.Save(state.settingsPath());
    }
}

}  // namespace OptionsPanel
}  // namespace cm
