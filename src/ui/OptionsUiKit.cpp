#include "ui/OptionsUiKit.h"

#include <imgui.h>

namespace cm {
namespace OptionsUiKit {

ImVec4 GoldColor() { return ImVec4(1.0f, 0.78f, 0.0f, 1.0f); }

ImVec4 GrayColor() { return ImVec4(0.7f, 0.7f, 0.7f, 1.0f); }

ImVec4 WhiteColor() { return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); }

void SectionHeading(const char* text) {
    ImGui::Spacing();
    ImGui::TextColored(GoldColor(), "%s", text);
    ImGui::Separator();
}

void SectionSubtext(const char* text) { ImGui::TextColored(GrayColor(), "%s", text); }

void WarningText(const char* text) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", text);
}

void WarningBanner(const char* text) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.45f, 0.2f, 0.05f, 0.85f));
    ImGui::BeginChild("##cm_warning_banner", ImVec2(0.0f, 48.0f), true);
    WarningText(text);
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void DisabledGateText(const char* text) { ImGui::TextDisabled("%s", text); }

bool SettingCheckbox(const char* label, bool* value, const char* tooltip) {
    const bool changed = ImGui::Checkbox(label, value);
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return changed;
}

bool SettingSliderFloat(const char* label, float* value, float min, float max, const char* format,
                        const char* tooltip) {
    const bool changed = ImGui::SliderFloat(label, value, min, max, format);
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return changed;
}

bool SettingSliderInt(const char* label, int* value, int min, int max, const char* tooltip) {
    const bool changed = ImGui::SliderInt(label, value, min, max);
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return changed;
}

bool SettingCombo(const char* label, int* currentIndex, const char* const* items, int itemCount,
                  const char* tooltip) {
    const bool changed = ImGui::Combo(label, currentIndex, items, itemCount);
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    return changed;
}

void BeginContentPanel(ImTextureID backgroundTexture) {
    const float panelHeight = ImGui::GetContentRegionAvail().y;
    ImGui::BeginChild("##cm_options_content", ImVec2(0.0f, panelHeight), false,
                      ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (backgroundTexture) {
        const ImVec2 p0 = ImGui::GetWindowPos();
        const ImVec2 p1(p0.x + ImGui::GetWindowWidth(), p0.y + ImGui::GetWindowHeight());
        const ImU32 tint =
            IM_COL32(255, 255, 255, static_cast<int>(255.0f * kContentBgAlpha));
        ImGui::GetWindowDrawList()->AddImage(backgroundTexture, p0, p1, ImVec2(0.0f, 0.0f),
                                             ImVec2(1.0f, 1.0f), tint);
    }
}

void EndContentPanel() { ImGui::EndChild(); }

bool IconButton(const char* id, ImTextureID icon, float size, const char* tooltip) {
    ImGui::PushID(id);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 2.0f));
    const bool pressed =
        icon ? ImGui::ImageButton(icon, ImVec2(size, size)) :
               ImGui::Button("?", ImVec2(size, size));
    ImGui::PopStyleVar();
    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltip);
    }
    ImGui::PopID();
    return pressed;
}

bool TexturedButton(const char* id, const char* label, ImTextureID icon, float iconSize,
                    const ImVec2& size) {
    ImGui::PushID(id);

    const ImVec2 labelSize = ImGui::CalcTextSize(label);
    const ImGuiStyle& style = ImGui::GetStyle();
    const float height = ImGui::GetFrameHeight();
    const bool hasIcon = icon != nullptr;
    const float iconPart = hasIcon ? iconSize + style.ItemInnerSpacing.x : 0.0f;

    ImVec2 buttonSize;
    if (size.x < 0.0f) {
        buttonSize = ImVec2(-1.0f, size.y > 0.0f ? size.y : height);
    } else if (size.x > 0.0f) {
        buttonSize = size;
    } else if (size.y > 0.0f) {
        buttonSize = ImVec2(style.FramePadding.x * 2.0f + iconPart + labelSize.x, size.y);
    } else {
        buttonSize = ImVec2(style.FramePadding.x * 2.0f + iconPart + labelSize.x, height);
    }

    const bool pressed = ImGui::Button("##tex_btn", buttonSize);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    const float yCenter = (min.y + max.y) * 0.5f;
    const float contentWidth = iconPart + labelSize.x;
    const float buttonWidth = max.x - min.x;

    float x = min.x + (buttonWidth - contentWidth) * 0.5f;
    if (hasIcon) {
        const float y = yCenter - iconSize * 0.5f;
        ImGui::GetWindowDrawList()->AddImage(icon, ImVec2(x, y), ImVec2(x + iconSize, y + iconSize));
        x += iconSize + style.ItemInnerSpacing.x;
    }

    const ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
    ImGui::GetWindowDrawList()->AddText(ImVec2(x, yCenter - labelSize.y * 0.5f), textColor, label);

    ImGui::PopID();
    return pressed;
}

}  // namespace OptionsUiKit
}  // namespace cm
