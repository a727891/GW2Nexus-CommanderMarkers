#pragma once

#include "core/SettingsStore.h"

#include <imgui.h>

namespace cm {
namespace OptionsUiKit {

constexpr float kSidebarWidth = 180.0f;
constexpr float kContentBgAlpha = 0.22f;

ImVec4 GoldColor();
ImVec4 GrayColor();
ImVec4 WhiteColor();

void SectionHeading(const char* text);
void SectionSubtext(const char* text);
void WarningText(const char* text);
void WarningBanner(const char* text);
void DisabledGateText(const char* text);

bool SettingCheckbox(const char* label, bool* value, const char* tooltip = nullptr);
bool SettingSliderFloat(const char* label, float* value, float min, float max,
                        const char* format = "%.2f", const char* tooltip = nullptr);
bool SettingSliderInt(const char* label, int* value, int min, int max,
                      const char* tooltip = nullptr);
bool SettingCombo(const char* label, int* currentIndex, const char* const* items, int itemCount,
                  const char* tooltip = nullptr);

void BeginContentPanel(ImTextureID backgroundTexture = nullptr);
void EndContentPanel();

bool IconButton(const char* id, ImTextureID icon, float size = 18.0f, const char* tooltip = nullptr);
bool TexturedButton(const char* id, const char* label, ImTextureID icon, float iconSize = 16.0f,
                    const ImVec2& size = ImVec2(0.0f, 0.0f));

}  // namespace OptionsUiKit
}  // namespace cm
