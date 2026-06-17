# GitHub Actions release template (Nexus addons)

Reusable workflow for Linux-hosted MinGW release builds. Pushes to `main` compile
`scripts/build-release.sh`, upload a workflow artifact, and optionally publish a
GitHub release with the stripped DLL.

## Adopt in a Nexus repo

1. Ensure the repo has:
   - `scripts/build-release.sh`
   - `cmake/mingw-w64-toolchain.cmake`
   - `scripts/generate_addon_metadata.py` (if using build-time `Version.h`)

2. Create and Edit `.github/nexus-release.json`:

   ```json
   {
   "release_title": "TitleHere",
   "publish_release": true,
   "draft": false,
   "prerelease": false
   }  
   ```

   | Field | Purpose |
   |-------|---------|
   | `release_title` | Prefix for GitHub release title (version is appended) |
   | `publish_release` | `true` to create a GitHub release on each `main` push |
   | `draft` | Create draft releases |
   | `prerelease` | Mark releases as pre-release |

3. Commit and push to `main`. The workflow runs automatically.

## Behavior

- **Trigger:** push to `main`, or manual **Actions → Release build → Run workflow**
- **Build:** `./scripts/build-release.sh` (Release MinGW DLL in `dist/`)
- **Version tag:** `v{build_version}` from `build/generated/Version.h`; if the tag
  already exists, `v{build_version}.{run_number}` is used
- **Artifact:** uploaded for every run (download from the Actions summary)
- **Release:** optional; controlled by `publish_release` in `nexus-release.json`

## Requirements
- `scripts/build-release.sh` must leave exactly one `*.dll` in `dist/`.

## Local parity

The same DLL is produced locally:

```bash
./scripts/build-release.sh
```
