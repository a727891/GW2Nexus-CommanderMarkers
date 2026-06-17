#pragma once

#include "core/Types.h"

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace cm {

class MarkerListing {
public:
    static constexpr const char* kFilename = "custom_markers.json";

    explicit MarkerListing(std::string addonDir = {});

    void SetAddonDir(std::string addonDir);

    void Load();
    void Save();

    std::vector<MarkerSet> GetAllMarkerSets() const;
    std::vector<MarkerSet> GetMarkersForMap(int mapId) const;

    void SaveMarker(const MarkerSet& markerSet);
    bool ContainsMarkerSet(const MarkerSet& markerSet) const;
    void EditMarker(size_t index, const MarkerSet& markerSet);
    void DeleteMarker(const MarkerSet& markerSet);
    void ResetToDefault();
    void ReloadFromFile();

    const std::string& Version() const { return version_; }

    void SetOnMarkersChanged(std::function<void()> callback);

private:
    static bool LoadBuiltinDefaults(MarkerListingFile& out);
    void ApplyListing(MarkerListingFile listing);
    void NotifyChanged();

    std::string addonDir_;
    std::string version_ = "2.0.0";
    std::vector<MarkerSet> presets_;
    std::function<void()> onMarkersChanged_;
    bool suppressNotify_ = false;
    mutable std::mutex mutex_;
};

}  // namespace cm
