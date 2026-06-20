#pragma once

#include "core/Types.h"

#include <string>

namespace cm {

std::string GenerateUuidV4();
std::string NormalizeNameKey(const std::string& name);
std::string CurrentIso8601Utc();

}  // namespace cm
