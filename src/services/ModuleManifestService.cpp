#include "services/ModuleManifestService.h"

#include "core/Branding.h"
#include "services/HttpClient.h"

#include <nlohmann/json.hpp>

namespace cm {

std::string CommanderMarkersManifest::Resolve(const std::string& pathTemplate,
                                              const std::string& id) const {
    std::string path = pathTemplate;
    const auto pos = path.find("{id}");
    if (pos != std::string::npos) {
        path.replace(pos, 4, id);
    }
    return Absolute(path);
}

std::string CommanderMarkersManifest::Absolute(const std::string& path) const {
    if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) {
        return path;
    }
    std::string base = serverUrl;
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }
    if (path.empty() || path.front() != '/') {
        return base + "/" + path;
    }
    return base + path;
}

void ModuleManifestService::ApplyLocalTestOverrides() {
#if defined(CM_LOCAL_TEST)
    manifest_.serverUrl = kDefaultServerUrl;
#endif
}

bool ModuleManifestService::LoadOrFetch(const std::string& manifestUrl) {
    fetchSucceeded_ = false;
    const auto body = HttpGetUrl(manifestUrl);
    if (!body) {
        manifest_.serverUrl = kDefaultServerUrl;
        ApplyLocalTestOverrides();
        loaded_ = true;
        return false;
    }

    try {
        const auto j = nlohmann::json::parse(*body);
        manifest_.serverUrl = j.value("server_url", manifest_.serverUrl);
        manifest_.communityCheckUrl =
            j.value("community_check_url", manifest_.communityCheckUrl);
        manifest_.communityMarkersUrl =
            j.value("community_markers_url", manifest_.communityMarkersUrl);
        manifest_.setsUrl = j.value("sets_url", manifest_.setsUrl);
        manifest_.setDetailUrl = j.value("set_detail_url", manifest_.setDetailUrl);
        manifest_.thumbUrl = j.value("thumb_url", manifest_.thumbUrl);
        manifest_.categoriesUrl = j.value("categories_url", manifest_.categoriesUrl);
        manifest_.submissionsUrl = j.value("submissions_url", manifest_.submissionsUrl);
        manifest_.submissionsMineUrl =
            j.value("submissions_mine_url", manifest_.submissionsMineUrl);
        manifest_.subtokenUrl = j.value("subtoken_url", manifest_.subtokenUrl);
        manifest_.libraryUrl = j.value("library_url", manifest_.libraryUrl);
        ApplyLocalTestOverrides();
        loaded_ = true;
        fetchSucceeded_ = true;
        return true;
    } catch (...) {
        ApplyLocalTestOverrides();
        loaded_ = true;
        return false;
    }
}

}  // namespace cm
