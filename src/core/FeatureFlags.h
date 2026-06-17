#pragma once

// Incrementally re-enable features while testing for main-thread freezes.
// Rebuild + deploy after each change. See docs/DEBUGGING.md for the order.
//
// Baseline: everything false - load/unload only, optional world-ready log.

namespace cm {
namespace features {

inline constexpr bool kRegisterInputBind = true;
inline constexpr bool kOptionsPanel = true;
inline constexpr bool kDeferredInit = true;
inline constexpr bool kBackgroundSync = true;
inline constexpr bool kQuickAccess = true;
inline constexpr bool kMapWatch = true;
inline constexpr bool kMarkersPanel = true;
inline constexpr bool kScreenMapOverlay = true;
inline constexpr bool kBillboards = true;
inline constexpr bool kDatAssetIcons = true;

// Log once when mumble reports a stable in-world state (no other work).
inline constexpr bool kDebugWorldReadyLog = false;

// Map overlay visual debug (disable after projection/bounds verified).
inline constexpr bool kDebugMapOverlayBounds = false;
inline constexpr bool kDebugMapProjection = false;

}  // namespace features
}  // namespace cm
