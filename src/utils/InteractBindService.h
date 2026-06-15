#pragma once

#include "nexus/Nexus.h"

#include <functional>

namespace cm {

class InteractBindService {
public:
    using Handler = std::function<void()>;

    static void Register(AddonAPI_t* api, Handler handler);
    static void Unregister(AddonAPI_t* api);
    static const char* GetPrimaryKeyLabel();
};

}  // namespace cm
