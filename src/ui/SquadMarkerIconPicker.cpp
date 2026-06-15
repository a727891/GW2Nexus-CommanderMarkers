#include "ui/SquadMarkerIconPicker.h"

#include "data/MarkerDefinition.h"
#include "ui/TextureService.h"

#include <algorithm>
#include <imgui.h>

namespace cm {
namespace SquadMarkerIconPicker {
namespace {

constexpr float kSelectedSize = 32.0f;
constexpr float kUnselectedSize = 28.0f;
constexpr float kUnselectedAlpha = 0.3f;

int NormalizeIconIndex(int iconIndex) {
    if (iconIndex < 1 || iconIndex > 8) {
        return 1;
    }
    return iconIndex;
}

bool DrawIconButton(const char* id, ImTextureID texture, float size, float alpha, bool selected) {
    ImGui::PushID(id);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.0f, 0.35f));
    }

    const bool clicked = ImGui::ImageButton(
        texture, ImVec2(size, size), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 0,
        ImVec4(0.0f, 0.0f, 0.0f, 1.0f - alpha));

    if (selected) {
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return clicked;
}

}  // namespace

bool Draw(const char* id, int* iconIndex) {
    if (!iconIndex) {
        return false;
    }

    const int selectedIcon = NormalizeIconIndex(*iconIndex);
    bool changed = false;

    ImGui::PushID(id);
    ImGui::BeginGroup();
    bool first = true;
    for (std::size_t i = 0; i < kMarkerDefinitionCount; ++i) {
        const MarkerDefinition& def = kAllMarkers[i];
        if (def.markerType == SquadMarker::None || def.markerType == SquadMarker::Clear) {
            continue;
        }

        const ImTextureID texture = TextureService::GetTexture(def.markerType);
        if (!texture) {
            continue;
        }

        if (!first) {
            ImGui::SameLine(0.0f, 4.0f);
        }
        first = false;

        const int markerValue = static_cast<int>(def.markerType);
        const bool selected = markerValue == selectedIcon;
        const float size = selected ? kSelectedSize : kUnselectedSize;
        const float alpha = selected ? 1.0f : kUnselectedAlpha;

        if (DrawIconButton(def.displayName, texture, size, alpha, selected)) {
            *iconIndex = markerValue;
            changed = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", def.displayName);
        }
    }
    ImGui::EndGroup();
    ImGui::PopID();

    if (*iconIndex != selectedIcon && (*iconIndex < 1 || *iconIndex > 8)) {
        *iconIndex = selectedIcon;
    }

    return changed;
}

}  // namespace SquadMarkerIconPicker
}  // namespace cm
