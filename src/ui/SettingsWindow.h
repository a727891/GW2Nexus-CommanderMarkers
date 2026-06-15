#pragma once

#include "nexus/Nexus.h"

namespace cm {

class AppState;

enum class SettingsTab : int {
    ClickableMarkers = 0,
    AutoMarkerSettings = 1,
    AutoMarkerLibrary = 2,
    CommunityLibrary = 3,
    General = 4,
    About = 5,
};

namespace SettingsWindow {

void Open(SettingsTab tab = SettingsTab::ClickableMarkers);
void Close();
void Toggle(SettingsTab tab = SettingsTab::ClickableMarkers);
bool IsOpen();
void Shutdown(AddonAPI_t* api);
void Render(AppState& state);

}  // namespace SettingsWindow

namespace OptionsPanel {

void RenderNexusConfigEntry(AppState& state);
void RenderWindow(AppState& state, SettingsTab pendingTab, bool applyPendingTab);

}  // namespace OptionsPanel
}  // namespace cm
