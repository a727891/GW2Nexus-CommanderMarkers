#pragma once

namespace cm {

class AppState;
struct MarkerSet;

namespace MarkerSetEditorWindow {

void OpenNew(AppState& state);
void OpenExisting(AppState& state, const MarkerSet& markerSet, int listingIndex);
void OpenPersonalizedFromTemplate(AppState& state, const MarkerSet& templateSet);
void Close();
bool IsOpen();
void Render(AppState& state);

}  // namespace MarkerSetEditorWindow
}  // namespace cm
