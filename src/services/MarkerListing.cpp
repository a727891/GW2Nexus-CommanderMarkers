#include "services/MarkerListing.h"

#include "EmbeddedDefaults.h"
#include "data/MarkerSetJson.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace cm {

namespace {

bool MarkerSetsEqual(const MarkerSet& a, const MarkerSet& b) {
    return a.name == b.name && a.mapId == b.mapId && a.description == b.description &&
           a.trigger.x == b.trigger.x && a.trigger.y == b.trigger.y &&
           a.trigger.z == b.trigger.z && a.markers.size() == b.markers.size();
}

bool ReadFileText(const std::filesystem::path& path, std::string& outContent) {
    if (!std::filesystem::exists(path)) {
        return false;
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    outContent.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return !outContent.empty();
}

bool ParseListingJson(const std::string& json, MarkerListingFile& out) {
    try {
        out = MarkerSetJson::MarkerListingFileFromJson(nlohmann::json::parse(json));
        return !out.squadMarkerPreset.empty();
    } catch (...) {
        return false;
    }
}

}  // namespace

MarkerListing::MarkerListing(std::string addonDir) : addonDir_(std::move(addonDir)) {}

void MarkerListing::SetAddonDir(std::string addonDir) { addonDir_ = std::move(addonDir); }

void MarkerListing::SetOnMarkersChanged(std::function<void()> callback) {
    std::lock_guard lock(mutex_);
    onMarkersChanged_ = std::move(callback);
}

void MarkerListing::NotifyChanged() {
    if (suppressNotify_ || !onMarkersChanged_) {
        return;
    }
    onMarkersChanged_();
}

bool MarkerListing::LoadBuiltinDefaults(MarkerListingFile& out) {
    const char* json = EmbeddedDefaults::DefaultMarkersJson();
    const std::size_t size = EmbeddedDefaults::DefaultMarkersJsonSize();
    if (!json || size == 0) {
        return false;
    }
    return ParseListingJson(std::string(json, size), out);
}

void MarkerListing::ApplyListing(MarkerListingFile listing) {
    version_ = std::move(listing.version);
    presets_ = std::move(listing.squadMarkerPreset);
}

void MarkerListing::Load() {
    suppressNotify_ = true;
    const auto configPath = std::filesystem::path(addonDir_) / kFilename;
    if (!std::filesystem::exists(configPath)) {
        MarkerListingFile listing{};
        if (!LoadBuiltinDefaults(listing)) {
            suppressNotify_ = false;
            return;
        }
        ApplyListing(std::move(listing));
        Save();
        suppressNotify_ = false;
        return;
    }

    std::string fileText;
    if (!ReadFileText(configPath, fileText)) {
        MarkerListingFile listing{};
        if (LoadBuiltinDefaults(listing)) {
            ApplyListing(std::move(listing));
            Save();
        }
        suppressNotify_ = false;
        return;
    }

    try {
        MarkerListingFile listing =
            MarkerSetJson::MarkerListingFileFromJson(nlohmann::json::parse(fileText));
        if (listing.version == "1.0.0") {
            ResetToDefault();
            suppressNotify_ = false;
            return;
        }
        ApplyListing(std::move(listing));
    } catch (...) {
        MarkerListingFile listing{};
        if (LoadBuiltinDefaults(listing)) {
            ApplyListing(std::move(listing));
            Save();
        }
    }
    suppressNotify_ = false;
}

void MarkerListing::Save() {
    MarkerListingFile listing{};
    std::function<void()> changedCallback;
    {
        std::lock_guard lock(mutex_);
        listing.version = version_;
        listing.squadMarkerPreset = presets_;
        changedCallback = onMarkersChanged_;
    }

    const auto configPath = std::filesystem::path(addonDir_) / kFilename;
    if (configPath.has_parent_path()) {
        std::filesystem::create_directories(configPath.parent_path());
    }

    const auto json = MarkerSetJson::MarkerListingFileToJson(listing).dump(2);
    std::ofstream out(configPath);
    if (!out.is_open()) {
        return;
    }
    out << json;

    if (changedCallback && !suppressNotify_) {
        changedCallback();
    }
}

std::vector<MarkerSet> MarkerListing::GetAllMarkerSets() const {
    std::lock_guard lock(mutex_);
    return presets_;
}

std::vector<MarkerSet> MarkerListing::GetMarkersForMap(int mapId) const {
    std::lock_guard lock(mutex_);
    std::vector<MarkerSet> result;
    for (const auto& markerSet : presets_) {
        if (markerSet.mapId == mapId) {
            result.push_back(markerSet);
        }
    }
    return result;
}

void MarkerListing::SaveMarker(const MarkerSet& markerSet) {
    {
        std::lock_guard lock(mutex_);
        for (const auto& existing : presets_) {
            if (MarkerSetsEqual(existing, markerSet)) {
                return;
            }
        }
        presets_.push_back(markerSet);
    }
    Save();
}

bool MarkerListing::ContainsMarkerSet(const MarkerSet& markerSet) const {
    std::lock_guard lock(mutex_);
    for (const auto& existing : presets_) {
        if (MarkerSetsEqual(existing, markerSet)) {
            return true;
        }
    }
    return false;
}

void MarkerListing::EditMarker(size_t index, const MarkerSet& markerSet) {
    {
        std::lock_guard lock(mutex_);
        if (index >= presets_.size()) {
            return;
        }
        presets_[index] = markerSet;
    }
    Save();
}

void MarkerListing::DeleteMarker(const MarkerSet& markerSet) {
    {
        std::lock_guard lock(mutex_);
        presets_.erase(
            std::remove_if(presets_.begin(), presets_.end(),
                           [&](const MarkerSet& existing) { return MarkerSetsEqual(existing, markerSet); }),
            presets_.end());
    }
    Save();
}

void MarkerListing::ResetToDefault() {
    MarkerListingFile listing{};
    if (!LoadBuiltinDefaults(listing)) {
        return;
    }
    ApplyListing(std::move(listing));
    version_ = "2.0.0";
    Save();
}

void MarkerListing::ReloadFromFile() { Load(); }

}  // namespace cm
