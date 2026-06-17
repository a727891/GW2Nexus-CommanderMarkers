#!/usr/bin/env bash
set -euo pipefail

# Deploy CommanderMarkers.dll to a local GW2 addons folder.
#
# Usage:
#   ./scripts/deploy-to-gw2.sh
#   ./scripts/deploy-to-gw2.sh --release

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
DIST_DIR="$ROOT/dist"
ADDONS_DIR="${GW2_ADDONS_DIR:-/home/soeed/steam/steamapps/common/Guild Wars 2/addons}"
DLL_NAME="CommanderMarkers.dll"

RELEASE_MODE=false
for arg in "$@"; do
  case "$arg" in
    --release) RELEASE_MODE=true ;;
    *)
      echo "error: unknown argument: $arg" >&2
      echo "usage: $0 [--release]" >&2
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
echo "Deployed $DLL_NAME to $ADDONS_DIR"
