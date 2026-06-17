#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
EXPORT_DIR="$ROOT/dist"
DLL_NAME="CommanderMarkers.dll"
TOOLCHAIN_FILE="$ROOT/cmake/mingw-w64-toolchain.cmake"

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "error: required command not found: $1" >&2
    exit 1
  fi
}

resolve_strip_bin() {
  for candidate in x86_64-w64-mingw32-strip mingw64-strip mingw-strip; do
    if command -v "$candidate" >/dev/null 2>&1; then
      command -v "$candidate"
      return 0
    fi
  done
  echo "error: MinGW strip tool not found" >&2
  exit 1
}

require_command cmake
STRIP_BIN="$(resolve_strip_bin)"

GENERATE_METADATA() {
  local build_dir="$1"
  local ua_prefix="$2"
  mkdir -p "$build_dir/generated"
  python3 "$ROOT/scripts/generate_addon_metadata.py" \
    --user-agent-prefix "$ua_prefix" \
    --output-dir "$build_dir/generated"
}

require_command python3

if [[ ! -f "$TOOLCHAIN_FILE" ]]; then
  echo "error: missing toolchain file: $TOOLCHAIN_FILE" >&2
  exit 1
fi

if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  echo "Configuring Release build..."
  cmake -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    "$ROOT"
fi

GENERATE_METADATA "$BUILD_DIR" "COMM-MARKERS-Nexus"

echo "Building $DLL_NAME..."
cmake --build "$BUILD_DIR"

built_dll="$BUILD_DIR/$DLL_NAME"
if [[ ! -f "$built_dll" ]]; then
  echo "error: build did not produce $built_dll" >&2
  exit 1
fi

mkdir -p "$EXPORT_DIR"
exported_dll="$EXPORT_DIR/$DLL_NAME"
cp -f "$built_dll" "$exported_dll"
"$STRIP_BIN" --strip-debug "$exported_dll"

built_size="$(du -h "$built_dll" | awk '{print $1}')"
export_size="$(du -h "$exported_dll" | awk '{print $1}')"

echo "Build output:  $built_dll ($built_size, includes debug info)"
echo "Release export: $exported_dll ($export_size, stripped)"
