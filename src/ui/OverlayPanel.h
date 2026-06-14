#pragma once

#include "core/Types.h"

#include <cstdint>
#include <imgui.h>

namespace cm {
namespace OverlayPanel {

bool Begin(const char* id, Vec2f& position, bool lockPosition);
bool End(bool screenClamp, uint32_t screenWidth, uint32_t screenHeight);
bool IsDragging();

}  // namespace OverlayPanel
}  // namespace cm
