#include "ui/MapPreviewPopupUi.h"

#include "ui/TextureService.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace cm {
namespace MapPreviewPopupUi {
namespace {

constexpr float kHoverImageSize = 768.0f;
constexpr float kLegendIconSize = 20.0f;
constexpr float kLegendPad = 8.0f;
constexpr float kLegendRowGap = 4.0f;
constexpr float kOverlayLineGap = 2.0f;

struct MarkerCacheEntry {
    std::vector<MarkerCoord> markers;
    std::atomic<bool> loading{false};
    std::atomic<bool> ready{false};
};

std::mutex g_markerCacheMutex;
std::unordered_map<std::string, MarkerCacheEntry> g_markerCache;

SquadMarker MarkerTypeForIcon(int icon) {
    if (icon >= static_cast<int>(SquadMarker::Arrow) &&
        icon <= static_cast<int>(SquadMarker::Clear)) {
        return static_cast<SquadMarker>(icon);
    }
    return SquadMarker::Arrow;
}

std::vector<const MarkerCoord*> CustomLegendEntries(const std::vector<MarkerCoord>& markers) {
    std::vector<const MarkerCoord*> entries;
    entries.reserve(markers.size());
    for (const MarkerCoord& marker : markers) {
        if (marker.name.empty()) {
            continue;
        }
        if (marker.icon == static_cast<int>(SquadMarker::Clear)) {
            continue;
        }
        entries.push_back(&marker);
    }
    std::sort(entries.begin(), entries.end(),
              [](const MarkerCoord* a, const MarkerCoord* b) { return a->icon < b->icon; });
    return entries;
}

std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    if (text.empty()) {
        return lines;
    }

    size_t start = 0;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == '\n') {
            std::string line = text.substr(start, i - start);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines.push_back(std::move(line));
            start = i + 1;
        }
    }
    return lines;
}

void DrawPanel(ImDrawList* drawList, const ImVec2& panelMin, const ImVec2& panelMax) {
    drawList->AddRectFilled(panelMin, panelMax, IM_COL32(12, 12, 12, 205), 4.0f);
    drawList->AddRect(panelMin, panelMax, IM_COL32(255, 255, 255, 55), 4.0f);
}

void DrawHeaderOverlay(const ImVec2& imageMin, const ImVec2& imageMax, const std::string& name,
                       const std::string& description) {
    if (name.empty() && description.empty()) {
        return;
    }

    const ImFont* font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize();

    const std::vector<std::string> descriptionLines = SplitLines(description);

    float contentWidth = 0.0f;
    if (!name.empty()) {
        contentWidth = std::max(contentWidth, ImGui::CalcTextSize(name.c_str()).x);
    }
    for (const std::string& line : descriptionLines) {
        contentWidth = std::max(contentWidth, ImGui::CalcTextSize(line.c_str()).x);
    }

    const float panelWidth = contentWidth + kLegendPad * 2.0f;
    float panelHeight = kLegendPad;
    if (!name.empty()) {
        panelHeight += fontSize + kOverlayLineGap;
    }
    if (!descriptionLines.empty()) {
        panelHeight += static_cast<float>(descriptionLines.size()) * fontSize +
                       static_cast<float>(descriptionLines.size() - 1) * kOverlayLineGap;
    }
    panelHeight += kLegendPad;

    const ImVec2 panelMin(imageMin.x + kLegendPad, imageMin.y + kLegendPad);
    const ImVec2 panelMax(panelMin.x + panelWidth, panelMin.y + panelHeight);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    DrawPanel(drawList, panelMin, panelMax);

    float y = panelMin.y + kLegendPad;
    if (!name.empty()) {
        drawList->AddText(font, fontSize, ImVec2(panelMin.x + kLegendPad, y),
                          IM_COL32(245, 245, 245, 255), name.c_str());
        y += fontSize + kOverlayLineGap;
    }
    for (const std::string& line : descriptionLines) {
        drawList->AddText(font, fontSize, ImVec2(panelMin.x + kLegendPad, y),
                          IM_COL32(190, 190, 190, 255), line.c_str());
        y += fontSize + kOverlayLineGap;
    }
}

void DrawLegendOverlay(const ImVec2& imageMin, const ImVec2& imageMax,
                       const std::vector<MarkerCoord>& markers) {
    const std::vector<const MarkerCoord*> entries = CustomLegendEntries(markers);
    if (entries.empty()) {
        return;
    }

    const ImFont* font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize();

    float maxTextWidth = 0.0f;
    for (const MarkerCoord* marker : entries) {
        maxTextWidth =
            std::max(maxTextWidth, ImGui::CalcTextSize(marker->name.c_str()).x);
    }

    const float legendWidth =
        kLegendPad + kLegendIconSize + 6.0f + maxTextWidth + kLegendPad;
    const float rowHeight = std::max(kLegendIconSize, fontSize) + kLegendRowGap;
    const float legendHeight =
        kLegendPad + static_cast<float>(entries.size()) * rowHeight - kLegendRowGap + kLegendPad;

    const ImVec2 legendMin(imageMin.x + kLegendPad, imageMax.y - legendHeight - kLegendPad);
    const ImVec2 legendMax(legendMin.x + legendWidth, imageMax.y - kLegendPad);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    DrawPanel(drawList, legendMin, legendMax);

    float y = legendMin.y + kLegendPad;
    for (const MarkerCoord* marker : entries) {
        const float iconX = legendMin.x + kLegendPad;
        if (const ImTextureID icon =
                TextureService::GetTexture(MarkerTypeForIcon(marker->icon))) {
            drawList->AddImage(icon, ImVec2(iconX, y),
                               ImVec2(iconX + kLegendIconSize, y + kLegendIconSize));
        }

        const float textX = iconX + kLegendIconSize + 6.0f;
        const float textY = y + (kLegendIconSize - fontSize) * 0.5f;
        drawList->AddText(font, fontSize, ImVec2(textX, textY),
                          IM_COL32(235, 235, 235, 255), marker->name.c_str());
        y += rowHeight;
    }
}

void RenderImageWithOverlays(ImTextureID texture, ImVec2 size, const std::string& name,
                             const std::string& description,
                             const std::vector<MarkerCoord>& markers) {
    ImGui::Image(texture, size);
    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const ImVec2 imageMax = ImGui::GetItemRectMax();
    DrawHeaderOverlay(imageMin, imageMax, name, description);
    DrawLegendOverlay(imageMin, imageMax, markers);
}

ImTextureID HoverPreviewTexture(AppState& state, const Target& target, ImTextureID fallback) {
    if (!target.communitySetId.empty()) {
        state.previewImageCache.RequestPreview(target.communitySetId, target.previewLargeUrl);
        const std::string previewPath =
            state.previewImageCache.PreviewPathForSet(target.communitySetId);
        if (!previewPath.empty()) {
            if (ImTextureID preview = TextureService::GetThumbTexture(
                    "CM_PREVIEW_" + target.communitySetId, previewPath)) {
                return preview;
            }
        }
    }
    return fallback;
}

void EnsureMarkersFetched(AppState& state, const std::string& communitySetId) {
    if (communitySetId.empty()) {
        return;
    }

    {
        std::lock_guard lock(g_markerCacheMutex);
        MarkerCacheEntry& entry = g_markerCache[communitySetId];
        if (entry.ready.load() || entry.loading.load()) {
            return;
        }
        entry.loading.store(true);
    }

    std::thread([&state, communitySetId]() {
        MarkerSet fetched{};
        const bool ok = state.communityCatalog.FetchSetDetail(communitySetId, fetched, nullptr);
        {
            std::lock_guard lock(g_markerCacheMutex);
            MarkerCacheEntry& entry = g_markerCache[communitySetId];
            if (ok) {
                entry.markers = std::move(fetched.markers);
                entry.ready.store(true);
            }
            entry.loading.store(false);
        }
    }).detach();
}

std::vector<MarkerCoord> ResolveMarkers(const Target& target) {
    if (!target.markers.empty()) {
        return target.markers;
    }
    if (target.communitySetId.empty()) {
        return {};
    }

    std::lock_guard lock(g_markerCacheMutex);
    const auto it = g_markerCache.find(target.communitySetId);
    if (it != g_markerCache.end() && it->second.ready.load()) {
        return it->second.markers;
    }
    return {};
}

void RenderHoverTooltip(AppState& state, const Target& target, ImTextureID texture) {
    if (!target.communitySetId.empty() && target.markers.empty()) {
        EnsureMarkersFetched(state, target.communitySetId);
    }

    ImGui::BeginTooltip();

    const ImTextureID previewTexture = HoverPreviewTexture(state, target, texture);
    const std::vector<MarkerCoord> markers = ResolveMarkers(target);
    RenderImageWithOverlays(previewTexture, ImVec2(kHoverImageSize, kHoverImageSize), target.label,
                            target.description, markers);

    ImGui::EndTooltip();
}

}  // namespace

void RenderIcon(AppState& state, const Target& target, ImTextureID texture, ImVec2 displaySize) {
    if (!target.zoomable || !texture) {
        if (texture) {
            ImGui::Image(texture, displaySize);
        } else {
            ImGui::Dummy(displaySize);
        }
        return;
    }

    ImGui::Image(texture, displaySize);
    if (ImGui::IsItemHovered()) {
        RenderHoverTooltip(state, target, texture);
    }
}

}  // namespace MapPreviewPopupUi
}  // namespace cm
