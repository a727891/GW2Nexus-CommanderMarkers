#pragma once

#include "core/Types.h"

#include <cstdint>
#include <imgui.h>

namespace cm {
namespace OverlayPanel {

bool Begin(const char* id, Vec2f& position, bool draggable);
bool End(bool screenClamp, uint32_t screenWidth, uint32_t screenHeight);
bool IsDragging();
bool ConsumeDragFinished();

}  // namespace OverlayPanel
}  // namespace cm
