# Release flow (development → main)

This flow assumes:
- The development branch is named **development**.
- You build locally with PlatformIO and manually upload binaries to GitHub Releases.

## 1) Prepare the version bump on development
1. Checkout the `development` branch.
2. Update `SOFTWARE_VERSION` in [diyp-controller/dp.h](diyp-controller/dp.h). Format used for production releases is: X.Y.Z, e.g. 1.8.0
3. Commit with a message like “Bump version to X.Y.Z”.
4. Push the `development` branch.

## 2) Open and merge PR to main
1. Create a PR from `development` → `main` (VS Code GitHub PR view or GitHub web).
2. Add description of the main changes in the release to the PR.
3. Use **Squash and merge** to keep `main` clean (single release commit).

## 3) Sync main locally
1. Checkout `main`.
2. Pull latest changes from origin.

## 4) Build and release
1. Build locally with PlatformIO and export the firmware binary.
2. Create a Git tag `vX.Y.Z` on `main`.
3. Create a GitHub Release for tag `vX.Y.Z`. With release notes.
4. Upload the firmware binary asset to the Release, with 'firmware.bin' as filename.
5. If the release is set as "Latest", the diyPresso-client will automatically download that version when updating frimware.
