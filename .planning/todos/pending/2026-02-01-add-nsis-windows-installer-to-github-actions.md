---
created: 2026-02-01T11:01
title: Add NSIS Windows installer to GitHub Actions
area: tooling
files:
  - .github/workflows/build-and-release.yml:116-171
---

## Problem

The current GitHub Actions workflow (`build-and-release.yml`) builds Windows VST3 plugins but only packages them as a raw ZIP archive. Users must manually extract and copy the `.vst3` folder to `C:\Program Files\Common Files\VST3\`.

This creates friction for Windows users who expect a standard installer experience with:
- One-click installation to correct system folder
- Uninstaller in Add/Remove Programs
- License agreement display
- Professional distribution appearance

## Solution

Extend the `build-windows` job to create NSIS-based `.exe` installers:

1. **Create NSIS script template** (`installer/windows.nsi.template`)
   - Install VST3 to `$PROGRAMFILES64\Common Files\VST3\`
   - Register uninstaller in Windows registry
   - Display license from `LICENSE` or `LICENSE.txt`
   - Plugin name and version injected via defines

2. **Update workflow** (`.github/workflows/build-and-release.yml`)
   - NSIS is pre-installed on `windows-latest` runners (no setup needed)
   - Add step after build to run `makensis` with version/name defines
   - Upload both ZIP (for manual install) and EXE (for installer)

3. **Artifact naming**
   - Keep: `PluginName-version-windows.zip` (manual install)
   - Add: `PluginName-version-windows-installer.exe` (NSIS installer)

Reference: NSIS is already available on GitHub Actions Windows runners.
