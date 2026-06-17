#!/usr/bin/env python3
"""Generate Nexus addon Version.h at build time.

Version scheme matches nxa-addon-template:
  V_MAJOR=year, V_MINOR=month, V_BUILD=day, V_REVISION=hour*60+minute
"""

from __future__ import annotations

import argparse
from datetime import datetime
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate Version.h")
    parser.add_argument(
        "--user-agent-prefix",
        required=True,
        help='HTTP User-Agent prefix, e.g. "ClearsTracker-Nexus"',
    )
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    now = datetime.now()
    revision = now.hour * 60 + now.minute
    version_string = f"{now.year}.{now.month}.{now.day}.{revision}"

    args.output_dir.mkdir(parents=True, exist_ok=True)

    version_path = args.output_dir / "Version.h"
    version_path.write_text(
        "\n".join(
            [
                "#pragma once",
                "// Auto-generated at build time - do not edit.",
                f"#define V_MAJOR {now.year}",
                f"#define V_MINOR {now.month}",
                f"#define V_BUILD {now.day}",
                f"#define V_REVISION {revision}",
                f'#define V_VERSION_STRING "{version_string}"',
                f'#define V_HTTP_USER_AGENT "{args.user_agent_prefix}/" V_VERSION_STRING',
                "",
            ]
        ),
        encoding="utf-8",
    )
    print(f"Wrote {version_path} ({version_string})")


if __name__ == "__main__":
    main()
