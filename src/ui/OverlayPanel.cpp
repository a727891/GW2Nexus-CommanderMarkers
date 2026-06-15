#include "ui/OverlayPanel.h"

#include "ui/UiLayer.h"

#include <cmath>

namespace cm {
namespace OverlayPanel {

namespace {

constexpr ImGuiWindowFlags kOverlayBaseFlags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
    ImGuiWindowFlags_NoBringToFrontOnFocus;

Vec2f* g_windowPosition = nullptr;
bool g_draggable = false;
int g_styleVarCount = 0;
bool g_dragging = false;
bool g_dragFinished = false;

struct DragState {
    bool active = false;
    ImVec2 grabOffset{};
};

DragState g_dragState;

float SnapPos(float value) { return std::roundf(value); }

ImVec2 SnapPos(const ImVec2& value) {
    return {SnapPos(value.x), SnapPos(value.y)};
}

ImVec2 Subtract(const ImVec2& a, const ImVec2& b) {
    return {a.x - b.x, a.y - b.y};
}

void ClampPosition(Vec2f& position, const ImVec2& windowSize, uint32_t screenWidth,
                   uint32_t screenHeight) {
    if (screenWidth == 0 || screenHeight == 0) {
        return;
    }

    const float maxX = static_cast<float>(screenWidth) - windowSize.x;
    const float maxY = static_cast<float>(screenHeight) - windowSize.y;
    if (position.x < 0.0f) {
        position.x = 0.0f;
    }
    if (position.y < 0.0f) {
        position.y = 0.0f;
    }
    if (position.x > maxX) {
        position.x = maxX;
    }
    if (position.y > maxY) {
        position.y = maxY;
    }
}

}  // namespace

bool Begin(const char* id, Vec2f& position, bool draggable) {
    g_windowPosition = &position;
    g_draggable = draggable;
    g_dragFinished = false;

    ImGui::SetNextWindowPos(SnapPos(ImVec2(position.x, position.y)), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    g_styleVarCount = 2;

    if (!ImGui::Begin(id, nullptr, kOverlayBaseFlags)) {
        ImGui::End();
        ImGui::PopStyleVar(g_styleVarCount);
        g_styleVarCount = 0;
        g_windowPosition = nullptr;
        return false;
    }

    SendWindowToDisplayBack();
    return true;
}

bool End(bool screenClamp, uint32_t screenWidth, uint32_t screenHeight) {
    bool dragged = false;
    const bool wasDragging = g_dragState.active;

    if (g_windowPosition) {
        if (g_draggable) {
            const ImVec2 windowPos = ImGui::GetWindowPos();
            const ImVec2 windowSize = ImGui::GetWindowSize();
            ImGui::SetCursorScreenPos(windowPos);
            ImGui::PushID("overlay_drag");
            ImGui::InvisibleButton("##catcher", windowSize);
            ImGui::PopID();

            if (ImGui::IsItemActivated()) {
                g_dragState.active = true;
                g_dragState.grabOffset = Subtract(ImGui::GetMousePos(), windowPos);
            }

            if (g_dragState.active && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                const ImVec2 newPos =
                    SnapPos(Subtract(ImGui::GetMousePos(), g_dragState.grabOffset));
                ImGui::SetWindowPos(newPos);
                dragged = true;
            } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                g_dragState.active = false;
            }
        } else {
            g_dragState.active = false;
        }

        if (wasDragging && !g_dragState.active) {
            g_dragFinished = true;
        }

        const ImVec2 pos = SnapPos(ImGui::GetWindowPos());
        const ImVec2 size = ImGui::GetWindowSize();
        g_windowPosition->x = pos.x;
        g_windowPosition->y = pos.y;
        if (screenClamp) {
            ClampPosition(*g_windowPosition, size, screenWidth, screenHeight);
            if (g_windowPosition->x != pos.x || g_windowPosition->y != pos.y) {
                ImGui::SetWindowPos(
                    SnapPos(ImVec2(g_windowPosition->x, g_windowPosition->y)));
            }
        }
    }

    g_dragging = dragged;

    ImGui::End();
    if (g_styleVarCount > 0) {
        ImGui::PopStyleVar(g_styleVarCount);
        g_styleVarCount = 0;
    }
    g_windowPosition = nullptr;
    return dragged;
}

bool IsDragging() { return g_dragging; }

bool ConsumeDragFinished() {
    if (!g_dragFinished) {
        return false;
    }
    g_dragFinished = false;
    return true;
}

}  // namespace OverlayPanel
}  // namespace cm
