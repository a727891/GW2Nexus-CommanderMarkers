#include "utils/UuidUtils.h"

#include <chrono>
#include <cctype>
#include <iomanip>
#include <random>
#include <sstream>

namespace cm {

std::string GenerateUuidV4() {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 15);
    std::uniform_int_distribution<int> dist2(8, 11);

    std::ostringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; ++i) ss << dist(rng);
    ss << '-';
    for (int i = 0; i < 4; ++i) ss << dist(rng);
    ss << "-4";
    for (int i = 0; i < 3; ++i) ss << dist(rng);
    ss << '-';
    ss << dist2(rng);
    for (int i = 0; i < 3; ++i) ss << dist(rng);
    ss << '-';
    for (int i = 0; i < 12; ++i) ss << dist(rng);
    return ss.str();
}

std::string NormalizeNameKey(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (unsigned char ch : name) {
        if (std::isspace(ch)) {
            continue;
        }
        out.push_back(static_cast<char>(std::tolower(ch)));
    }
    return out;
}

std::string CurrentIso8601Utc() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

}  // namespace cm
