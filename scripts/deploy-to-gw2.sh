#!/usr/bin/env bash
set -euo pipefail

# Deploy NexusCommanderMarkers.dll to a local GW2 addons folder.
#
# Usage:
#   ./scripts/deploy-to-gw2.sh              # build DLL + default marker seed
#   ./scripts/deploy-to-gw2.sh --ftue       # DLL only, clear cache (test first-load sync)
#   ./scripts/deploy-to-gw2.sh --release    # stripped dist DLL + default marker seed
#   ./scripts/deploy-to-gw2.sh --release --ftue

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
DIST_DIR="$ROOT/dist"
ADDONS_DIR="/home/soeed/steam/steamapps/common/Guild Wars 2/addons"
ADDON_DATA_DIR="NexusCommanderMarkers"
CACHE_DIR="$ADDON_DATA_DIR/commanderMarkers"
DEFAULT_MARKERS_SRC="$ROOT/assets/data/default_markers.json"
DLL_NAME="NexusCommanderMarkers.dll"

FTUE_MODE=false
RELEASE_MODE=false
for arg in "$@"; do
  case "$arg" in
    --ftue) FTUE_MODE=true ;;
    --release) RELEASE_MODE=true ;;
    *)
      echo "error: unknown argument: $arg" >&2
      echo "usage: $0 [--release] [--ftue]" >&2
      exit 1
      ;;
  esac
done

if $RELEASE_MODE; then
  DLL_SRC="$DIST_DIR/$DLL_NAME"
  if [[ ! -f "$DLL_SRC" ]]; then
    echo "error: $DLL_SRC not found. Run ./scripts/build-release.sh first." >&2
    exit 1
  fi
else
  DLL_SRC="$BUILD_DIR/$DLL_NAME"
  if [[ ! -f "$DLL_SRC" ]]; then
    echo "error: $DLL_SRC not found. Build first." >&2
    exit 1
  fi
fi

mkdir -p "$ADDONS_DIR"
cp -f "$DLL_SRC" "$ADDONS_DIR/$DLL_NAME"
if $RELEASE_MODE; then
  echo "Deployed release export $DLL_NAME to $ADDONS_DIR"
else
  echo "Deployed $DLL_NAME to $ADDONS_DIR"
fi

if $FTUE_MODE; then
  rm -rf "$ADDONS_DIR/$CACHE_DIR"
  rm -f "$ADDONS_DIR/$ADDON_DATA_DIR/custom_markers.json"
  rm -f "$ADDONS_DIR/$ADDON_DATA_DIR/default_markers.json"
  echo "FTUE mode: cleared community cache and local marker library seed"
  exit 0
fi

mkdir -p "$ADDONS_DIR/$ADDON_DATA_DIR"
if [[ -f "$DEFAULT_MARKERS_SRC" ]]; then
  cp -f "$DEFAULT_MARKERS_SRC" "$ADDONS_DIR/$ADDON_DATA_DIR/default_markers.json"
  echo "Seeded default_markers.json in $ADDONS_DIR/$ADDON_DATA_DIR"
fi

echo "Community JSON syncs on first boot into $ADDONS_DIR/$CACHE_DIR"
echo "Textures and corner icon are embedded in the DLL."
