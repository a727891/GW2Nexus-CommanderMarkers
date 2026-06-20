#include "services/PreviewImageCache.h"

#include "services/HttpClient.h"

#include <filesystem>
#include <fstream>
#include <thread>

namespace cm {

namespace {

std::filesystem::path ImagePath(const std::string& addonDir,
                                const char* subdir,
                                const std::string& communitySetId) {
    return std::filesystem::path(addonDir) / "commanderMarkers" / subdir /
           (communitySetId + ".png");
}

std::string ResolveRelativeUrl(const std::string& serverUrl, const std::string& path) {
    if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) {
        return path;
    }
    std::string base = serverUrl;
    while (!base.empty() && base.back() == '/') {
        base.pop_back();
    }
    if (path.empty()) {
        return base;
    }
    if (path.front() == '/') {
        return base + path;
    }
    return base + "/" + path;
}

}  // namespace

PreviewImageCache::PreviewImageCache(std::string addonDir) : addonDir_(std::move(addonDir)) {}

void PreviewImageCache::SetAddonDir(std::string addonDir) { addonDir_ = std::move(addonDir); }

void PreviewImageCache::SetServerUrl(const std::string& serverUrl) { serverUrl_ = serverUrl; }

std::string PreviewImageCache::ThumbPathForSet(const std::string& communitySetId) const {
    if (communitySetId.empty()) {
        return {};
    }
    const auto path = ImagePath(addonDir_, "thumbs", communitySetId);
    if (std::filesystem::exists(path)) {
        return path.string();
    }
    return {};
}

std::string PreviewImageCache::PreviewPathForSet(const std::string& communitySetId) const {
    if (communitySetId.empty()) {
        return {};
    }
    const auto path = ImagePath(addonDir_, "previews", communitySetId);
    if (std::filesystem::exists(path)) {
        return path.string();
    }
    return {};
}

std::string PreviewImageCache::ResolveDownloadUrl(const std::string& communitySetId,
                                                  const std::string& previewThumbUrl) const {
    if (!previewThumbUrl.empty()) {
        return ResolveRelativeUrl(serverUrl_, previewThumbUrl);
    }
    return ResolveRelativeUrl(serverUrl_,
                              "/commander-markers/v1/sets/" + communitySetId + "/thumb.png");
}

std::string PreviewImageCache::ResolvePreviewDownloadUrl(
    const std::string& communitySetId,
    const std::string& previewLargeUrl) const {
    if (!previewLargeUrl.empty()) {
        return ResolveRelativeUrl(serverUrl_, previewLargeUrl);
    }
    return ResolveRelativeUrl(serverUrl_,
                              "/commander-markers/v1/sets/" + communitySetId + "/preview.png");
}

bool PreviewImageCache::DownloadImage(const std::string& path, const std::string& url) {
    const auto response = HttpGetUrlEx(url);
    if (response.statusCode < 200 || response.statusCode >= 300 || response.body.empty()) {
        return false;
    }
    const auto filePath = std::filesystem::path(path);
    std::filesystem::create_directories(filePath.parent_path());
    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }
    out.write(response.body.data(), static_cast<std::streamsize>(response.body.size()));
    return true;
}

void PreviewImageCache::RequestThumb(const std::string& communitySetId,
                                     const std::string& previewThumbUrl,
                                     std::function<void(const std::string& path)> onReady) {
    if (communitySetId.empty()) {
        return;
    }
    if (const auto existing = ThumbPathForSet(communitySetId); !existing.empty()) {
        if (onReady) {
            onReady(existing);
        }
        return;
    }

    const std::string url = ResolveDownloadUrl(communitySetId, previewThumbUrl);
    const std::string dest = ImagePath(addonDir_, "thumbs", communitySetId).string();
    std::thread([this, dest, url, onReady]() {
        if (DownloadImage(dest, url)) {
            if (onReady) {
                onReady(dest);
            }
        }
    }).detach();
}

void PreviewImageCache::RequestPreview(const std::string& communitySetId,
                                       const std::string& previewLargeUrl,
                                       std::function<void(const std::string& path)> onReady) {
    if (communitySetId.empty()) {
        return;
    }
    if (const auto existing = PreviewPathForSet(communitySetId); !existing.empty()) {
        if (onReady) {
            onReady(existing);
        }
        return;
    }

    const std::string url = ResolvePreviewDownloadUrl(communitySetId, previewLargeUrl);
    const std::string dest = ImagePath(addonDir_, "previews", communitySetId).string();
    std::thread([this, dest, url, onReady]() {
        if (DownloadImage(dest, url)) {
            if (onReady) {
                onReady(dest);
            }
        }
    }).detach();
}

}  // namespace cm
