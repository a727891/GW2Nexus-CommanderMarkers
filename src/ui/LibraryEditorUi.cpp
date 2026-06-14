#include "ui/LibraryEditorUi.h"

#include "core/AppState.h"
#include "data/MarkerSetJson.h"
#include "ui/OptionsUiKit.h"

#include <imgui.h>

namespace cm {
namespace LibraryEditorUi {
namespace {

struct EditorState {
    int selectedIndex = -1;
    bool editing = false;
    MarkerSet draft{};
    char importBuffer[4096] = {};
};

EditorState g_state;

bool NuclearModifierHeld() {
    return ImGui::GetIO().KeyCtrl && ImGui::GetIO().KeyShift;
}

void BeginEdit(const MarkerSet& markerSet, int index) {
    g_state.selectedIndex = index;
    g_state.editing = true;
    g_state.draft = markerSet;
}

void CancelEdit() {
    g_state.editing = false;
    g_state.selectedIndex = -1;
}

}  // namespace

void Render(AppState& state) {
    using namespace OptionsUiKit;

    SectionHeading("AutoMarker Library");
    SectionSubtext("Manage saved marker sets for the current account.");

    SettingCheckbox("Filter to current map", &state.settings.libraryFilterCurrentMap,
                    "Only show marker sets for your current map.");

    ImGui::Spacing();

    if (ImGui::Button("Add New")) {
        MarkerSet markerSet{};
        markerSet.name = "new set name";
        markerSet.description = "description";
        markerSet.mapId = state.mumbleLink ? static_cast<int>(state.mumbleLink->Context.MapID) : 0;
        markerSet.markers.push_back(MarkerCoord{.name = "marker name"});
        BeginEdit(markerSet, -1);
    }

    ImGui::SameLine();
    if (NuclearModifierHeld() && ImGui::Button("Reset Library To Default")) {
        state.markerListing.ResetToDefault();
        CancelEdit();
    } else {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
        ImGui::Button("Reset Library To Default");
        ImGui::PopStyleVar();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("Hold Ctrl+Shift to reset the library to defaults.");
        }
    }

    ImGui::Separator();

    const int currentMapId =
        state.mumbleLink ? static_cast<int>(state.mumbleLink->Context.MapID) : 0;
    const auto markerSets = state.markerListing.GetAllMarkerSets();

    if (ImGui::BeginChild("##cm_library_list", ImVec2(0.0f, 220.0f), true)) {
        for (size_t i = 0; i < markerSets.size(); ++i) {
            const MarkerSet& markerSet = markerSets[i];
            if (state.settings.libraryFilterCurrentMap && markerSet.mapId != currentMapId) {
                continue;
            }

            ImGui::PushID(static_cast<int>(i));
            bool enabled = markerSet.enabled;
            if (ImGui::Checkbox("##enabled", &enabled)) {
                MarkerSet updated = markerSet;
                updated.enabled = enabled;
                state.markerListing.EditMarker(i, updated);
            }
            ImGui::SameLine();

            const std::string label =
                markerSet.name + " (" + state.mapData.Describe(markerSet.mapId) + ")";
            if (ImGui::Selectable(label.c_str(), g_state.selectedIndex == static_cast<int>(i))) {
                BeginEdit(markerSet, static_cast<int>(i));
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    if (g_state.editing) {
        ImGui::Separator();
        SectionSubtext("Edit marker set");

        char nameBuf[128];
        char descBuf[256];
        std::snprintf(nameBuf, sizeof(nameBuf), "%s", g_state.draft.name.c_str());
        std::snprintf(descBuf, sizeof(descBuf), "%s", g_state.draft.description.c_str());

        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
            g_state.draft.name = nameBuf;
        }
        if (ImGui::InputText("Description", descBuf, sizeof(descBuf))) {
            g_state.draft.description = descBuf;
        }
        ImGui::InputInt("Map ID", &g_state.draft.mapId);
        ImGui::InputFloat3("Trigger", &g_state.draft.trigger.x);

        if (ImGui::Button("Add Marker")) {
            g_state.draft.markers.push_back(MarkerCoord{.name = "marker"});
        }

        if (ImGui::BeginChild("##cm_marker_coords", ImVec2(0.0f, 140.0f), true)) {
            for (size_t m = 0; m < g_state.draft.markers.size(); ++m) {
                ImGui::PushID(static_cast<int>(m));
                MarkerCoord& marker = g_state.draft.markers[m];
                char markerName[64];
                std::snprintf(markerName, sizeof(markerName), "%s", marker.name.c_str());
                ImGui::InputInt("Icon", &marker.icon);
                ImGui::InputText("Label", markerName, sizeof(markerName));
                marker.name = markerName;
                ImGui::InputFloat3("Position", &marker.x);
                if (ImGui::Button("Remove")) {
                    g_state.draft.markers.erase(g_state.draft.markers.begin() +
                                                static_cast<std::ptrdiff_t>(m));
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Save")) {
            if (g_state.selectedIndex >= 0) {
                state.markerListing.EditMarker(static_cast<size_t>(g_state.selectedIndex),
                                             g_state.draft);
            } else {
                state.markerListing.SaveMarker(g_state.draft);
            }
            CancelEdit();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            CancelEdit();
        }
        ImGui::SameLine();
        if (g_state.selectedIndex >= 0 && ImGui::Button("Delete")) {
            state.markerListing.DeleteMarker(markerSets[static_cast<size_t>(g_state.selectedIndex)]);
            CancelEdit();
        }
        ImGui::SameLine();
        if (ImGui::Button("Export Base64")) {
            const std::string exported = MarkerSetJson::MarkerSetToJson(g_state.draft).dump();
            ImGui::SetClipboardText(exported.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("Import Base64")) {
            ImGui::OpenPopup("Import Marker Set");
        }

        if (ImGui::BeginPopupModal("Import Marker Set", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputTextMultiline("##import", g_state.importBuffer, sizeof(g_state.importBuffer),
                                      ImVec2(420.0f, 120.0f));
            if (ImGui::Button("Import")) {
                try {
                    const nlohmann::json j = nlohmann::json::parse(g_state.importBuffer);
                    g_state.draft = MarkerSetJson::MarkerSetFromJson(j);
                    ImGui::CloseCurrentPopup();
                } catch (...) {
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
}

}  // namespace LibraryEditorUi
}  // namespace cm
