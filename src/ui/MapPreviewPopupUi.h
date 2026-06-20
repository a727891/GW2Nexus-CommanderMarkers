#pragma once

#include "core/AppState.h"
#include "core/Types.h"

#include <imgui.h>
#include <string>
#include <vector>

namespace cm {
namespace MapPreviewPopupUi {

struct Target {
    std::string communitySetId;
    std::string previewLargeUrl;
    std::string previewThumbUrl;
    std::string label;
    std::string description;
    std::vector<MarkerCoord> markers;
    bool zoomable = false;
};

void RenderIcon(AppState& state, const Target& target, ImTextureID texture, ImVec2 displaySize);

}  // namespace MapPreviewPopupUi
}  // namespace cm
