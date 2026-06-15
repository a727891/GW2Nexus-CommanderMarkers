#pragma once

#include "nexus/Nexus.h"

class AppState;

namespace cm {
namespace MarkersPanel {

void Render(AppState& state, bool optionsPreview = false);
void RegisterInput(AddonAPI_t* api);
void UnregisterInput(AddonAPI_t* api);

}  // namespace MarkersPanel
}  // namespace cm
