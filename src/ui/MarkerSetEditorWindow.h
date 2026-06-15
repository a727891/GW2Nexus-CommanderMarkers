#pragma once

class AppState;

namespace cm {

struct MarkerSet;

namespace MarkerSetEditorWindow {

void OpenNew(AppState& state);
void OpenExisting(AppState& state, const MarkerSet& markerSet, int listingIndex);
void Close();
bool IsOpen();
void Render(AppState& state);

}  // namespace MarkerSetEditorWindow
}  // namespace cm
