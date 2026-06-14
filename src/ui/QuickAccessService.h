#pragma once

#include "nexus/Nexus.h"

namespace cm {

class AppState;

namespace QuickAccessService {
void Register(AddonAPI_t* api, AppState& state);
void Unregister(AddonAPI_t* api);
void SyncVisibility(AddonAPI_t* api, AppState& state);
void Refresh(AddonAPI_t* api, AppState& state);
void OnShortcutActivated(AppState& state);
}  // namespace QuickAccessService

}  // namespace cm
