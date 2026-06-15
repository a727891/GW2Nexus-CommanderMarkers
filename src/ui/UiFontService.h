#pragma once

#include "nexus/Nexus.h"

#include <imgui.h>

struct ImDrawList;
struct ImFont;

namespace cm {
namespace UiFontService {

constexpr const char* kMenomoniaIdentifier = "MENOMONIA_S";

void Initialize(AddonAPI_t* api, NexusLinkData_t* nexusLink);
void Shutdown(AddonAPI_t* api);

ImFont* GetUiFont(NexusLinkData_t* nexusLink);
float EffectiveFontSize(ImFont* font, float scale);
ImVec2 MeasureText(const char* text, ImFont* font, float fontSize);

void DrawTextShadowed(ImDrawList* draw,
                      ImFont* font,
                      float fontSize,
                      const ImVec2& pos,
                      ImU32 color,
                      const char* text,
                      float shadowOffset = 2.0f);

}  // namespace UiFontService
}  // namespace cm
