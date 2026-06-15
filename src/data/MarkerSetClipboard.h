#pragma once

#include "core/Types.h"

#include <optional>
#include <string>

namespace cm {
namespace MarkerSetClipboard {

std::string ExportToBase64(const MarkerSet& markerSet);
std::optional<MarkerSet> ImportFromText(const std::string& text);

}  // namespace MarkerSetClipboard
}  // namespace cm
