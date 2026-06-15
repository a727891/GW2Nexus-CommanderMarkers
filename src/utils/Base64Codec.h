#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cm {
namespace Base64Codec {

std::string Encode(const std::uint8_t* data, std::size_t length);
std::string EncodeUtf8(const std::string& utf8);

std::optional<std::vector<std::uint8_t>> Decode(const std::string& encoded);
std::optional<std::string> DecodeUtf8(const std::string& encoded);

}  // namespace Base64Codec
}  // namespace cm
