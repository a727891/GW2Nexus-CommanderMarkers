#include "utils/MumbleIdentity.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <windows.h>

namespace cm {
namespace MumbleIdentity {
namespace {

std::string WideToUtf8(const wchar_t* wide) {
    if (!wide || wide[0] == L'\0') {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }

    std::string result(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::optional<nlohmann::json> ParseIdentityJson(const Mumble::Data* data) {
    if (!data) {
        return std::nullopt;
    }

    const std::string identity = WideToUtf8(data->Identity);
    if (identity.empty()) {
        return std::nullopt;
    }

    try {
        return nlohmann::json::parse(identity);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace

int ParseUisz(const Mumble::Data* data) {
    const auto json = ParseIdentityJson(data);
    if (!json) {
        return 1;
    }
    return json->value("uisz", 1);
}

float ParseUiScale(const Mumble::Data* data) {
    switch (ParseUisz(data)) {
        case 0:
            return 0.9f;
        case 2:
            return 1.111f;
        case 3:
            return 1.224f;
        default:
            return 1.0f;
    }
}

bool ParseCommander(const Mumble::Data* data) {
    const auto json = ParseIdentityJson(data);
    if (!json) {
        return false;
    }
    if (json->contains("commander")) {
        return (*json)["commander"].get<bool>();
    }
    return json->value("com", false);
}

std::string ParseCharacterName(const Mumble::Data* data) {
    const auto json = ParseIdentityJson(data);
    if (!json || !json->contains("name")) {
        return {};
    }
    return (*json)["name"].get<std::string>();
}

bool IsParsedIdentityUsable(const Mumble::Data* mumble, const Mumble::Identity* identity) {
    return mumble && mumble->UITick > 0 && identity != nullptr;
}

}  // namespace MumbleIdentity
}  // namespace cm
