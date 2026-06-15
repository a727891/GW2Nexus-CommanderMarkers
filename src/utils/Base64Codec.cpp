#include "utils/Base64Codec.h"

#include <cctype>

namespace cm {
namespace Base64Codec {
namespace {

constexpr char kEncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int DecodeChar(unsigned char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    if (c == '=') {
        return -2;
    }
    return -1;
}

std::string TrimWhitespace(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

}  // namespace

std::string Encode(const std::uint8_t* data, std::size_t length) {
    if (!data || length == 0) {
        return {};
    }

    std::string out;
    out.reserve(((length + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 2 < length) {
        const unsigned int block =
            (static_cast<unsigned int>(data[i]) << 16) |
            (static_cast<unsigned int>(data[i + 1]) << 8) |
            static_cast<unsigned int>(data[i + 2]);
        out.push_back(kEncodeTable[(block >> 18) & 0x3F]);
        out.push_back(kEncodeTable[(block >> 12) & 0x3F]);
        out.push_back(kEncodeTable[(block >> 6) & 0x3F]);
        out.push_back(kEncodeTable[block & 0x3F]);
        i += 3;
    }

    const std::size_t remaining = length - i;
    if (remaining == 1) {
        const unsigned int block = static_cast<unsigned int>(data[i]) << 16;
        out.push_back(kEncodeTable[(block >> 18) & 0x3F]);
        out.push_back(kEncodeTable[(block >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (remaining == 2) {
        const unsigned int block =
            (static_cast<unsigned int>(data[i]) << 16) |
            (static_cast<unsigned int>(data[i + 1]) << 8);
        out.push_back(kEncodeTable[(block >> 18) & 0x3F]);
        out.push_back(kEncodeTable[(block >> 12) & 0x3F]);
        out.push_back(kEncodeTable[(block >> 6) & 0x3F]);
        out.push_back('=');
    }

    return out;
}

std::string EncodeUtf8(const std::string& utf8) {
    return Encode(reinterpret_cast<const std::uint8_t*>(utf8.data()), utf8.size());
}

std::optional<std::vector<std::uint8_t>> Decode(const std::string& encoded) {
    const std::string trimmed = TrimWhitespace(encoded);
    if (trimmed.empty()) {
        return std::vector<std::uint8_t>{};
    }

    std::vector<std::uint8_t> out;
    out.reserve((trimmed.size() / 4) * 3);

    int accumulator = 0;
    int bits = -8;
    bool sawData = false;

    for (unsigned char c : trimmed) {
        if (c == '=') {
            break;
        }

        const int value = DecodeChar(c);
        if (value < 0) {
            continue;
        }

        sawData = true;
        accumulator = (accumulator << 6) | value;
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xFF));
            bits -= 8;
        }
    }

    if (!sawData) {
        return std::nullopt;
    }

    return out;
}

std::optional<std::string> DecodeUtf8(const std::string& encoded) {
    const std::optional<std::vector<std::uint8_t>> bytes = Decode(encoded);
    if (!bytes) {
        return std::nullopt;
    }
    return std::string(bytes->begin(), bytes->end());
}

}  // namespace Base64Codec
}  // namespace cm
