#pragma once

#include "core/Types.h"

#include <string>

namespace cm {

class SyncBaselineHash {
public:
    static std::string Compute(const MarkerSet& markerSet);
};

}  // namespace cm
