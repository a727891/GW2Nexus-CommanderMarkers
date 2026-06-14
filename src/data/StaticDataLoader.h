#pragma once

#include <string>

namespace cm {

class StaticDataLoader {
public:
    static constexpr const char* kCacheDirName = "commanderMarkers";

    static bool EnsureCacheDir(const std::string& addonDir);
    static bool LoadCached(const std::string& addonDir,
                           const std::string& filename,
                           std::string& outContent);
    static bool WriteCached(const std::string& addonDir,
                            const std::string& filename,
                            const std::string& content);
    static bool Download(const std::string& addonDir,
                         const std::string& filename,
                         const std::string& url,
                         std::string& outContent);
    static bool LoadOrDownload(const std::string& addonDir,
                               const std::string& filename,
                               const std::string& url,
                               std::string& outContent);
};

}  // namespace cm
