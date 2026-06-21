#pragma once

#include "core/Branding.h"

#include <string>

namespace cm {

struct CommanderMarkersManifest {
    std::string serverUrl = kDefaultServerUrl;
    std::string communityCheckUrl = "/commander-markers/v1/community/check";
    std::string communityMarkersUrl = "/commander-markers/v1/community/markers.json";
    std::string setsUrl = "/commander-markers/v1/sets";
    std::string setDetailUrl = "/commander-markers/v1/sets/{id}";
    std::string thumbUrl = "/commander-markers/v1/sets/{id}/thumb.png";
    std::string categoriesUrl = "/commander-markers/v1/categories";
    std::string submissionsUrl = "/commander-markers/v1/submissions";
    std::string submissionsMineUrl = "/commander-markers/v1/submissions/mine";
    std::string subtokenUrl = "/v4/auth/subtoken";
    std::string libraryUrl = "/markers";

    std::string Resolve(const std::string& pathTemplate, const std::string& id = {}) const;
    std::string Absolute(const std::string& path) const;
};

class ModuleManifestService {
public:
    bool LoadOrFetch(const std::string& manifestUrl);
    const CommanderMarkersManifest& Get() const { return manifest_; }
    bool IsLoaded() const { return loaded_; }
    bool FetchSucceeded() const { return fetchSucceeded_; }

private:
    void ApplyLocalTestOverrides();

    CommanderMarkersManifest manifest_{};
    bool loaded_ = false;
    bool fetchSucceeded_ = false;
};

}  // namespace cm
