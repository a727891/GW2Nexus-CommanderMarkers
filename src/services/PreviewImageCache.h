#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace cm {

class PreviewImageCache {
public:
    explicit PreviewImageCache(std::string addonDir = {});

    void SetAddonDir(std::string addonDir);
    void SetServerUrl(const std::string& serverUrl);

    std::string ThumbPathForSet(const std::string& communitySetId) const;
    std::string PreviewPathForSet(const std::string& communitySetId) const;
    void RequestThumb(const std::string& communitySetId,
                      const std::string& previewThumbUrl,
                      std::function<void(const std::string& path)> onReady = {});
    void RequestPreview(const std::string& communitySetId,
                        const std::string& previewLargeUrl,
                        std::function<void(const std::string& path)> onReady = {});

private:
    std::string ResolveDownloadUrl(const std::string& communitySetId,
                                   const std::string& previewThumbUrl) const;
    std::string ResolvePreviewDownloadUrl(const std::string& communitySetId,
                                          const std::string& previewLargeUrl) const;
    bool DownloadImage(const std::string& path, const std::string& url);

    std::string addonDir_;
    std::string serverUrl_;
    mutable std::mutex mutex_;
};

}  // namespace cm
