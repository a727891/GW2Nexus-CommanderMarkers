#include "ui/MarkersPanel.h"

#include "core/AppState.h"
#include "core/MumbleUtils.h"
#include "data/MarkerDefinition.h"
#include "ui/OverlayPanel.h"
#include "ui/TextureService.h"
#include "utils/MarkerBindHelper.h"
#include "utils/MarkerPlacementHelper.h"

#include <imgui.h>
#include <optional>
#include <windows.h>

namespace cm {
namespace MarkersPanel {
namespace {

constexpr int kHotkeyDelayMs = 50;

struct PanelUiState {
    std::optional<SquadMarker> selectedGroundMarker;
    bool mouseInsidePanel = false;
};

PanelUiState g_uiState;

bool ShouldShowPanel(const AppState& state) {
    if (!state.settings.showMarkerPanel) {
        return false;
    }
    if (!IsInGame(state.mumbleLink, state.nexusLink)) {
        return false;
    }
    if (MapUiIsOpen(state.mumbleLink)) {
        return false;
    }
    if (state.settings.onlyCommanderPanel || state.ltMode) {
        return IsCommander(state.mumbleLink) || state.ltMode;
    }
    return true;
}

bool RenderMarkerButton(ImTextureID texture, int size, float opacity, const char* tooltip,
                        bool selected) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.0f, 0.45f));
    }

    const bool clicked = ImGui::ImageButton(
        reinterpret_cast<ImTextureID>(const_cast<void*>(reinterpret_cast<const void*>(texture))),
        ImVec2(static_cast<float>(size), static_cast<float>(size)),
        ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 0,
        ImVec4(0.0f, 0.0f, 0.0f, 1.0f - opacity));

    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }

    if (selected) {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
    return clicked;
}

void RenderMarkerRow(AppState& state, bool groundRow, bool optionsPreview) {
    const SettingsStore& settings = state.settings;
    const bool rowEnabled =
        groundRow ? settings.groundMarkersEnabled : settings.objectMarkersEnabled;
    if (!rowEnabled) {
        return;
    }

    if (settings.panelOrientation == PanelLayout::Vertical) {
        ImGui::BeginGroup();
    }

    for (std::size_t i = 0; i < kMarkerDefinitionCount; ++i) {
        const MarkerDefinition& def = kAllMarkers[i];
        if (groundRow && !def.supportsGroundTarget) {
            continue;
        }
        if (!groundRow && !def.supportsObjectTarget) {
            continue;
        }

        const ImTextureID texture = TextureService::GetTexture(def.markerType);
        if (!texture) {
            continue;
        }

        char tooltip[64];
        if (groundRow) {
            std::snprintf(tooltip, sizeof(tooltip), "%s Ground", def.displayName);
        } else {
            std::snprintf(tooltip, sizeof(tooltip), "%s Object", def.displayName);
        }

        const bool selected = groundRow && g_uiState.selectedGroundMarker &&
                              *g_uiState.selectedGroundMarker == def.markerType;

        if (RenderMarkerButton(texture, settings.iconWidth, settings.panelOpacity, tooltip,
                               selected)) {
            if (settings.dragPanel || optionsPreview) {
                continue;
            }

            if (groundRow) {
                if (selected) {
                    g_uiState.selectedGroundMarker.reset();
                } else {
                    g_uiState.selectedGroundMarker = def.markerType;
                }
            } else if (!optionsPreview && !state.placementService.IsBusy()) {
                InvokeObjectMarker(def.markerType, state.api, kHotkeyDelayMs);
            }
        }

        if (groundRow && ImGui::IsItemClicked(ImGuiMouseButton_Right) && !settings.dragPanel &&
            !optionsPreview) {
            InvokeGroundMarker(def.markerType, state.api, kHotkeyDelayMs);
            InvokeGroundMarker(def.markerType, state.api, kHotkeyDelayMs);
        }

        if (settings.panelOrientation == PanelLayout::Horizontal) {
            ImGui::SameLine(0.0f, 2.0f);
        }
    }

    if (settings.panelOrientation == PanelLayout::Vertical) {
        ImGui::EndGroup();
    }
}

void HandleGroundPlacementClick(AppState& state) {
    if (!g_uiState.selectedGroundMarker || state.settings.dragPanel) {
        return;
    }
    if (g_uiState.mouseInsidePanel) {
        return;
    }
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    const bool useScreenCoords = UseScreenCoordinatesForPlacement();
    POINT pt{};
    if (GetCursorPos(&pt)) {
        SetPlacementMousePosition({pt.x, pt.y}, useScreenCoords);
    }

    InvokeGroundMarker(*g_uiState.selectedGroundMarker, state.api, kHotkeyDelayMs);
    g_uiState.selectedGroundMarker.reset();
}

}  // namespace

void Render(AppState& state, bool optionsPreview) {
    if (!optionsPreview && !ShouldShowPanel(state)) {
        g_uiState.selectedGroundMarker.reset();
        return;
    }

    Vec2f panelPos = state.settings.panelLocation;
    if (optionsPreview) {
        panelPos = {20.0f, 20.0f};
    }

    const bool lockPosition = optionsPreview ? true : state.settings.dragPanel;
    if (!OverlayPanel::Begin(optionsPreview ? "##cm_markers_panel_preview" : "##cm_markers_panel",
                             panelPos, lockPosition)) {
        return;
    }

    g_uiState.mouseInsidePanel =
        ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, state.settings.panelOpacity);

    if (state.settings.panelOrientation == PanelLayout::Vertical) {
        RenderMarkerRow(state, true, optionsPreview);
        RenderMarkerRow(state, false, optionsPreview);
    } else {
        ImGui::BeginGroup();
        RenderMarkerRow(state, true, optionsPreview);
        ImGui::EndGroup();
        ImGui::SameLine(0.0f, 4.0f);
        ImGui::BeginGroup();
        RenderMarkerRow(state, false, optionsPreview);
        ImGui::EndGroup();
    }

    if (state.settings.dragPanel && !optionsPreview) {
        const ImVec2 min = ImGui::GetWindowPos();
        const ImVec2 max(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());
        ImDrawList* draw = ImGui::GetWindowDrawList();
        draw->AddRectFilled(min, max, IM_COL32(96, 96, 96, 192));
        const char* label = "Drag";
        const ImVec2 textSize = ImGui::CalcTextSize(label);
        draw->AddText(
            ImVec2(min.x + (ImGui::GetWindowWidth() - textSize.x) * 0.5f,
                   min.y + (ImGui::GetWindowHeight() - textSize.y) * 0.5f),
            IM_COL32(0, 0, 0, 255), label);
    }

    ImGui::PopStyleVar();

    const uint32_t screenW = state.nexusLink ? state.nexusLink->Width : 0;
    const uint32_t screenH = state.nexusLink ? state.nexusLink->Height : 0;
    OverlayPanel::End(!optionsPreview, screenW, screenH);

    if (!state.settings.dragPanel && !optionsPreview) {
        HandleGroundPlacementClick(state);
    }
}

}  // namespace MarkersPanel
}  // namespace cm
