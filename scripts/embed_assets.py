#!/usr/bin/env python3
"""Generate C++ sources that embed Commander Markers assets into the DLL."""

from __future__ import annotations

import argparse
from pathlib import Path


def sanitize_symbol(name: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in Path(name).stem)


def format_bytes(data: bytes) -> str:
    lines = []
    row = []
    for byte in data:
        row.append(f"0x{byte:02x}")
        if len(row) == 16:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    return "\n".join(lines)


def write_textures(textures_dir: Path, out_cpp: Path, out_h: Path) -> None:
    files = sorted(textures_dir.glob("*.png"))
    if not files:
        raise SystemExit(f"no PNG files in {textures_dir}")

    blob_sections = []
    assets = []
    for png in files:
        data = png.read_bytes()
        symbol = sanitize_symbol(png.name)
        blob_sections.append(
            f"alignas(4) const unsigned char kTextureBlob_{symbol}[] = {{\n"
            f"{format_bytes(data)}\n"
            f"}};\n"
            f"const size_t kTextureBlobSize_{symbol} = {len(data)};\n"
        )
        assets.append(
            f'    {{"{png.stem}", "CM_TEX_{symbol}", '
            f"kTextureBlob_{symbol}, kTextureBlobSize_{symbol}}},"
        )

    out_h.write_text(
        """#pragma once

#include <cstddef>

namespace cm {
namespace EmbeddedTextures {

struct Asset {
    const char* assetId;
    const char* identifier;
    const unsigned char* data;
    std::size_t size;
};

const Asset* Find(const char* assetId);
std::size_t Count();

}  // namespace EmbeddedTextures
}  // namespace cm
""",
        encoding="utf-8",
    )

    cpp = """#include "EmbeddedTextures.h"

#include <cstring>

namespace cm {
namespace EmbeddedTextures {
namespace {

"""
    cpp += "\n".join(blob_sections)
    cpp += "\nconst Asset kAssets[] = {\n"
    cpp += "\n".join(assets)
    cpp += """
};

}  // namespace

const Asset* Find(const char* assetId) {
    if (!assetId) return nullptr;
    for (const auto& asset : kAssets) {
        if (std::strcmp(asset.assetId, assetId) == 0) {
            return &asset;
        }
    }
    return nullptr;
}

std::size_t Count() { return sizeof(kAssets) / sizeof(kAssets[0]); }

}  // namespace EmbeddedTextures
}  // namespace cm
"""
    out_cpp.write_text(cpp, encoding="utf-8")


def write_default_markers(defaults_path: Path, out_cpp: Path, out_h: Path) -> None:
    if not defaults_path.is_file():
        raise SystemExit(f"missing default marker library: {defaults_path}")

    data = defaults_path.read_bytes()
    out_h.write_text(
        """#pragma once

#include <cstddef>

namespace cm {
namespace EmbeddedDefaults {

const char* DefaultMarkersJson();
std::size_t DefaultMarkersJsonSize();

}  // namespace EmbeddedDefaults
}  // namespace cm
""",
        encoding="utf-8",
    )

    out_cpp.write_text(
        """#include "EmbeddedDefaults.h"

namespace cm {
namespace EmbeddedDefaults {

alignas(4) const unsigned char kDefaultMarkersJson[] = {
"""
        + format_bytes(data)
        + f"""
}};

const char* DefaultMarkersJson() {{
    return reinterpret_cast<const char*>(kDefaultMarkersJson);
}}

std::size_t DefaultMarkersJsonSize() {{
    return {len(data)};
}}

}}  // namespace EmbeddedDefaults
}}  // namespace cm
""",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--textures-dir", type=Path, required=True)
    parser.add_argument("--defaults-json", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    write_textures(
        args.textures_dir,
        args.output_dir / "EmbeddedTextures.cpp",
        args.output_dir / "EmbeddedTextures.h",
    )
    write_default_markers(
        args.defaults_json,
        args.output_dir / "EmbeddedDefaults.cpp",
        args.output_dir / "EmbeddedDefaults.h",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
