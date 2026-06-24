#include "ui/DatAssetIconService.h"

#include "core/Branding.h"
#include "services/HttpClient.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cm {
namespace {

constexpr const char* kBaseUrl = "https://assets.gw2dat.com/";

AddonAPI_t* g_api = nullptr;
std::string g_addonDir;
std::mutex g_mutex;
std::unordered_map<int, ImTextureID> g_textures;
std::unordered_set<int> g_pending;
std::unordered_set<int> g_downloading;
std::unordered_set<int> g_readyToLoad;
std::atomic<bool> g_workerRunning{false};

std::string CacheDir() {
    return (std::filesystem::path(g_addonDir) / "textures" / "assets").string();
}

std::string CachePath(int assetId) {
    return (std::filesystem::path(CacheDir()) / (std::to_string(assetId) + ".png")).string();
}

std::string TextureIdentifier(int assetId) {
    return "CM_DAT_" + std::to_string(assetId);
}

ImTextureID TextureFromFile(int assetId) {
    if (!g_api || !g_api->Textures_GetOrCreateFromFile) {
        return nullptr;
    }
    const auto path = CachePath(assetId);
    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    Texture_t* texture =
        g_api->Textures_GetOrCreateFromFile(TextureIdentifier(assetId).c_str(), path.c_str());
    if (!texture || !texture->Resource) {
        return nullptr;
    }
    return reinterpret_cast<ImTextureID>(texture->Resource);
}

void DownloadAsset(int assetId) {
    const auto url = std::string(kBaseUrl) + std::to_string(assetId) + ".png";
    HttpRequestOptions options{};
    const auto body = HttpGetUrl(url, options);
    if (!body || body->empty()) {
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(CacheDir(), ec);
    std::ofstream out(CachePath(assetId), std::ios::binary);
    if (!out.is_open()) {
        return;
    }
    out.write(body->data(), static_cast<std::streamsize>(body->size()));
}

void WorkerLoop(std::vector<int> batch) {
    struct WorkerGuard {
        ~WorkerGuard() { g_workerRunning.store(false); }
    } guard;

    for (int assetId : batch) {
        if (assetId <= 0) {
            continue;
        }

        const auto path = CachePath(assetId);
        if (!std::filesystem::exists(path)) {
            DownloadAsset(assetId);
        }

        if (std::filesystem::exists(path)) {
            std::lock_guard lock(g_mutex);
            g_readyToLoad.insert(assetId);
            g_pending.erase(assetId);
        }
    }

    {
        std::lock_guard lock(g_mutex);
        for (int assetId : batch) {
            g_downloading.erase(assetId);
        }
    }
}

void FinalizeDownloadsOnMainThread() {
    if (!g_api) {
        return;
    }

    std::vector<int> ready;
    {
        std::lock_guard lock(g_mutex);
        ready.assign(g_readyToLoad.begin(), g_readyToLoad.end());
    }

    for (int assetId : ready) {
        ImTextureID texture = TextureFromFile(assetId);
        if (!texture) {
            continue;
        }

        std::lock_guard lock(g_mutex);
        g_textures[assetId] = texture;
        g_readyToLoad.erase(assetId);
        g_pending.erase(assetId);
    }
}

}  // namespace

void DatAssetIconService::Initialize(AddonAPI_t* api, const std::string& addonDir) {
    g_api = api;
    g_addonDir = addonDir;
    {
        std::lock_guard lock(g_mutex);
        g_textures.clear();
        g_pending.clear();
        g_downloading.clear();
        g_readyToLoad.clear();
    }

    if (LoadFromDisk(kInteractBubbleAssetId)) {
        return;
    }
    QueueDownload(kInteractBubbleAssetId);
}

void DatAssetIconService::Shutdown() {
    using namespace std::chrono_literals;
    const auto deadline = std::chrono::steady_clock::now() + 20s;
    while (g_workerRunning.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(10ms);
    }

    std::lock_guard lock(g_mutex);
    g_textures.clear();
    g_pending.clear();
    g_downloading.clear();
    g_readyToLoad.clear();
    g_api = nullptr;
    g_addonDir.clear();
}

ImTextureID DatAssetIconService::LoadFromDisk(int assetId) {
    if (assetId <= 0) {
        return nullptr;
    }

    {
        std::lock_guard lock(g_mutex);
        const auto it = g_textures.find(assetId);
        if (it != g_textures.end() && it->second) {
            return it->second;
        }
    }

    if (auto tex = TextureFromFile(assetId)) {
        std::lock_guard lock(g_mutex);
        g_textures[assetId] = tex;
        g_readyToLoad.erase(assetId);
        return tex;
    }
    return nullptr;
}

void DatAssetIconService::QueueDownload(int assetId) {
    if (assetId <= 0) {
        return;
    }

    std::lock_guard lock(g_mutex);
    if (g_textures.count(assetId) > 0) {
        return;
    }
    if (g_pending.count(assetId) > 0 || g_downloading.count(assetId) > 0) {
        return;
    }
    g_pending.insert(assetId);
}

void DatAssetIconService::ProcessDownloads() {
    FinalizeDownloadsOnMainThread();

    std::vector<int> batch;
    {
        std::lock_guard lock(g_mutex);
        if (g_workerRunning.load() || g_pending.empty()) {
            return;
        }
        for (int assetId : g_pending) {
            if (g_downloading.count(assetId) > 0) {
                continue;
            }
            g_downloading.insert(assetId);
            batch.push_back(assetId);
        }
        if (batch.empty()) {
            return;
        }
        g_workerRunning.store(true);
    }

    std::thread(WorkerLoop, std::move(batch)).detach();
}

ImTextureID DatAssetIconService::Request(int assetId) {
    if (assetId <= 0) {
        return nullptr;
    }

    if (auto tex = LoadFromDisk(assetId)) {
        return tex;
    }

    QueueDownload(assetId);
    return nullptr;
}

}  // namespace cm
