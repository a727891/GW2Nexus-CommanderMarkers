#include "services/MarkerListing.h"

#include "data/MarkerSetJson.h"
#include "data/StaticDataLoader.h"

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

}  // namespace

MarkerListing::MarkerListing(std::string addonDir) : addonDir_(std::move(addonDir)) {}

void MarkerListing::SetAddonDir(std::string addonDir) { addonDir_ = std::move(addonDir); }

void MarkerListing::SetOnMarkersChanged(std::function<void()> callback) {
    std::lock_guard lock(mutex_);
    onMarkersChanged_ = std::move(callback);
}

void MarkerListing::NotifyChanged() {
    if (onMarkersChanged_) {
        onMarkersChanged_();
    }
}

MarkerListingFile MarkerListing::InitDefaultPresets() {
    MarkerListingFile listing{};
    listing.version = "2.0.0";

    MarkerSet sabetha{};
    sabetha.name = "Sabetha";
    sabetha.description = "Cannon bomb launch platforms";
    sabetha.mapId = 1062;
    sabetha.trigger = {-78.40887f, 133.5607f, 70.977f};
    sabetha.markers = {
        {-132.6262f, 56.29699f, 62.96119f, 1, "South"},
        {-157.7033f, 87.01214f, 62.27411f, 2, "West"},
        {-126.95f, 111.8531f, 62.94498f, 3, "North"},
        {-101.9202f, 81.18908f, 63.03901f, 4, "East"},
    };
    listing.squadMarkerPreset.push_back(std::move(sabetha));

    MarkerSet gorseval{};
    gorseval.name = "Gorseval";
    gorseval.description = "Spirit Spawns";
    gorseval.mapId = 1062;
    gorseval.trigger = {-2.034508f, -107.3541f, 50.7478f};
    gorseval.markers = {
        {21.407f, -92.894f, 48.572f, 1, ""},
        {64.096f, -93.094f, 48.908f, 2, ""},
        {62.752f, -131.938f, 48.868f, 3, ""},
        {22.018f, -134.791f, 48.618f, 4, ""},
    };
    listing.squadMarkerPreset.push_back(std::move(gorseval));

    MarkerSet slothasor{};
    slothasor.name = "Slothasor";
    slothasor.description = "Mushroom Locations";
    slothasor.mapId = 1149;
    slothasor.trigger = {211.8357f, 36.68708f, 8.145153f};
    slothasor.markers = {
        {208.928f, 25.562f, 4.933f, 1, "1"},
        {174.839f, -9.068f, 2.414f, 2, "2"},
        {192.947f, -35.956f, 0.846f, 3, "3"},
        {224.945f, -13.404f, -0.177f, 4, "4"},
    };
    listing.squadMarkerPreset.push_back(std::move(slothasor));

    return listing;
}

bool MarkerListing::TryLoadDefaultFile(MarkerListingFile& out) const {
    std::string json;
    if (StaticDataLoader::LoadCached(addonDir_, kDefaultFilename, json)) {
        try {
            out = MarkerSetJson::MarkerListingFileFromJson(nlohmann::json::parse(json));
            return !out.squadMarkerPreset.empty();
        } catch (...) {
        }
    }

    const auto addonDefault = std::filesystem::path(addonDir_) / kDefaultFilename;
    if (ReadFileText(addonDefault, json)) {
        try {
            out = MarkerSetJson::MarkerListingFileFromJson(nlohmann::json::parse(json));
            return !out.squadMarkerPreset.empty();
        } catch (...) {
        }
    }

    return false;
}

void MarkerListing::ApplyListing(MarkerListingFile listing) {
    version_ = std::move(listing.version);
    presets_ = std::move(listing.squadMarkerPreset);
}

void MarkerListing::Load() {
    const auto configPath = std::filesystem::path(addonDir_) / kFilename;
    if (!std::filesystem::exists(configPath)) {
        MarkerListingFile listing{};
        if (!TryLoadDefaultFile(listing)) {
            listing = InitDefaultPresets();
        }
        ApplyListing(std::move(listing));
        Save();
        return;
    }

    std::string fileText;
    if (!ReadFileText(configPath, fileText)) {
        ApplyListing(InitDefaultPresets());
        Save();
        return;
    }

    try {
        MarkerListingFile listing =
            MarkerSetJson::MarkerListingFileFromJson(nlohmann::json::parse(fileText));
        if (listing.version == "1.0.0") {
            ResetToDefault();
            return;
        }
        ApplyListing(std::move(listing));
    } catch (...) {
        ApplyListing(InitDefaultPresets());
        Save();
    }
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

    if (changedCallback) {
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
    if (!TryLoadDefaultFile(listing)) {
        listing = InitDefaultPresets();
    }
    ApplyListing(std::move(listing));
    version_ = "2.0.0";
    Save();
}

void MarkerListing::ReloadFromFile() { Load(); }

}  // namespace cm
