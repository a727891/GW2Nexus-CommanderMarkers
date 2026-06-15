#pragma once

#include <imgui.h>
#include <imgui_internal.h>

namespace cm {

inline void SendWindowToDisplayBack() {
    if (ImGuiWindow* window = ImGui::GetCurrentWindow()) {
        ImGui::BringWindowToDisplayBack(window);
    }
}

}  // namespace cm
