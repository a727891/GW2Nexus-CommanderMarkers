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

std::optional<CommanderMarkersManifest> ModuleManifestService::ParseBody(
    const std::string& body) {
    try {
        const auto j = nlohmann::json::parse(body);
        CommanderMarkersManifest manifest{};
        manifest.serverUrl = j.value("server_url", manifest.serverUrl);
        manifest.communityCheckUrl =
            j.value("community_check_url", manifest.communityCheckUrl);
        manifest.communityMarkersUrl =
            j.value("community_markers_url", manifest.communityMarkersUrl);
        manifest.setsUrl = j.value("sets_url", manifest.setsUrl);
        manifest.setDetailUrl = j.value("set_detail_url", manifest.setDetailUrl);
        manifest.thumbUrl = j.value("thumb_url", manifest.thumbUrl);
        manifest.categoriesUrl = j.value("categories_url", manifest.categoriesUrl);
        manifest.submissionsUrl = j.value("submissions_url", manifest.submissionsUrl);
        manifest.submissionsMineUrl =
            j.value("submissions_mine_url", manifest.submissionsMineUrl);
        manifest.subtokenUrl = j.value("subtoken_url", manifest.subtokenUrl);
        manifest.libraryUrl = j.value("library_url", manifest.libraryUrl);
#if defined(CM_LOCAL_TEST)
        manifest.serverUrl = kDefaultServerUrl;
#endif
        return manifest;
    } catch (...) {
        return std::nullopt;
    }
}

void ModuleManifestService::LoadDefaults() {
    manifest_ = CommanderMarkersManifest{};
    ApplyLocalTestOverrides();
    loaded_ = true;
    fetchSucceeded_ = false;
}

bool ModuleManifestService::ApplyParsedManifest(const CommanderMarkersManifest& parsed) {
    manifest_ = parsed;
    ApplyLocalTestOverrides();
    loaded_ = true;
    fetchSucceeded_ = true;
    return true;
}

bool ModuleManifestService::ApplyFromBody(const std::string& body) {
    const auto parsed = ParseBody(body);
    if (!parsed) {
        fetchSucceeded_ = false;
        return false;
    }
    return ApplyParsedManifest(*parsed);
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

    if (ApplyFromBody(*body)) {
        return true;
    }

    ApplyLocalTestOverrides();
    loaded_ = true;
    return false;
}

}  // namespace cm
