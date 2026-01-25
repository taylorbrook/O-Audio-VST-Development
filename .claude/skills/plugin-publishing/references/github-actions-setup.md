# GitHub Actions Setup Reference

Complete guide for configuring cross-platform plugin builds via GitHub Actions.

---

## 1. Prerequisites

### Repository Requirements
- Git remote `origin` configured
- Repository hosted on GitHub
- Push access to create tags

### Workflow Permissions
GitHub Actions requires write permissions for releases. Configure in:
**Settings → Actions → General → Workflow permissions**

Set to: **Read and write permissions**

### Required Secrets (None for Basic Setup)
Basic builds require no secrets. For Phase 2 (code signing):
- `APPLE_CERTIFICATE` - Base64 encoded .p12
- `APPLE_CERTIFICATE_PASSWORD` - Certificate password
- `APPLE_ID` - Apple Developer account email
- `APPLE_TEAM_ID` - Apple Developer Team ID
- `APPLE_APP_SPECIFIC_PASSWORD` - For notarization

---

## 2. Workflow Structure

### Trigger
```yaml
on:
  push:
    tags:
      - '*-v*'  # Matches: PluginName-vX.Y.Z
```

Tag pattern enables per-plugin releases in multi-plugin repos.

### Job Dependencies
```
parse-tag
    │
    ├── build-macos ─────┐
    ├── build-windows ───┼──→ create-release
    └── build-linux ─────┘
```

### Runners
| Platform | Runner | Architecture |
|----------|--------|--------------|
| macOS | `macos-14` | Apple Silicon (arm64) |
| Windows | `windows-latest` | x64 |
| Linux | `ubuntu-22.04` | x64 |

---

## 3. Build Configuration

### JUCE Setup
Each runner downloads JUCE from official releases:
```bash
# macOS/Linux
curl -L "https://github.com/juce-framework/JUCE/releases/download/8.0.4/juce-8.0.4-osx.zip" -o juce.zip

# Windows (PowerShell)
Invoke-WebRequest -Uri "...juce-8.0.4-windows.zip" -OutFile juce.zip
```

### CMake Flags

**macOS (Universal Binary):**
```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

**Windows:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

**Linux:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

### Linux Dependencies
```bash
sudo apt-get install -y \
  libasound2-dev \
  libjack-jackd2-dev \
  libfreetype6-dev \
  libx11-dev \
  libxcomposite-dev \
  libxcursor-dev \
  libxext-dev \
  libxinerama-dev \
  libxrandr-dev \
  libxrender-dev \
  libwebkit2gtk-4.0-dev \
  libglu1-mesa-dev \
  mesa-common-dev
```

---

## 4. Artifact Handling

### Naming Convention
```
{PluginName}-{Version}-{platform}.{ext}
```

Examples:
- `OuariconTremolo-1.4.0-macos.tar.gz`
- `OuariconTremolo-1.4.0-windows.zip`
- `OuariconTremolo-1.4.0-linux.tar.gz`

### Contents

**macOS tarball:**
- `{ProductName}.vst3/` - VST3 bundle
- `{ProductName}.component/` - AU bundle

**Windows ZIP:**
- `{ProductName}.vst3/` - VST3 bundle

**Linux tarball:**
- `{ProductName}.vst3/` - VST3 bundle

### Upload/Download
```yaml
# Upload
- uses: actions/upload-artifact@v4
  with:
    name: macos-build
    path: Plugin-1.0.0-macos.tar.gz

# Download (in release job)
- uses: actions/download-artifact@v4
  with:
    path: artifacts
```

---

## 5. Release Creation

### Extract Release Notes
```bash
# Extract version section from CHANGELOG.md
awk '/^## \[1.4.0\]/{flag=1; next} /^## \[/{flag=0} flag' CHANGELOG.md
```

### Create Release Action
```yaml
- uses: softprops/action-gh-release@v2
  with:
    name: "Ouaricon Tremolo v1.4.0"
    body_path: release_notes.md
    files: |
      artifacts/macos-build/*
      artifacts/windows-build/*
      artifacts/linux-build/*
```

---

## 6. Troubleshooting

### Common Errors

**"Resource not accessible by integration"**
- Cause: Workflow lacks write permissions
- Fix: Settings → Actions → Workflow permissions → Read and write

**"CMake Error: Could not find JUCE"**
- Cause: JUCE_DIR not set or wrong path
- Fix: Verify JUCE download step succeeded

**macOS build fails with "code signing"**
- Cause: Unsigned builds may fail certain checks
- Fix: For unsigned releases, ensure no signing steps are configured

**Linux build missing WebKit**
- Cause: libwebkit2gtk-4.0-dev not installed
- Fix: Add to apt-get install list

**Windows build fails PowerShell syntax**
- Cause: Mixed bash/PowerShell syntax
- Fix: Use PowerShell syntax in Windows steps

### Debug Tips

1. **Add verbose CMake output:**
   ```yaml
   cmake -B build --debug-output
   ```

2. **List build artifacts:**
   ```yaml
   - run: find build -name "*.vst3" -o -name "*.component"
   ```

3. **Print environment:**
   ```yaml
   - run: env | sort
   ```

---

## 7. Phase 2 Enhancements (Future)

### macOS Code Signing
```yaml
- name: Import Certificate
  run: |
    echo "$APPLE_CERTIFICATE" | base64 --decode > certificate.p12
    security create-keychain -p "" build.keychain
    security import certificate.p12 -k build.keychain -P "$APPLE_CERTIFICATE_PASSWORD"

- name: Sign Plugins
  run: |
    codesign --force --deep --sign "Developer ID Application" dist/*.vst3
    codesign --force --deep --sign "Developer ID Application" dist/*.component
```

### macOS Notarization
```yaml
- name: Notarize
  run: |
    xcrun notarytool submit dist/Plugin.dmg \
      --apple-id "$APPLE_ID" \
      --team-id "$APPLE_TEAM_ID" \
      --password "$APPLE_APP_SPECIFIC_PASSWORD" \
      --wait
```

### Windows Authenticode (Future)
Requires Azure SignTool or similar service.

### Installer Packages (Future)
- macOS: DMG with drag-to-install
- Windows: NSIS or Inno Setup installer
- Linux: .deb package

---

## Quick Reference

### Tag Format
```
{PluginName}-v{X.Y.Z}
```

### Workflow File Location
```
.github/workflows/build-and-release.yml
```

### Manual Trigger (Testing)
```bash
# Create and push tag
git tag OuariconTremolo-v1.4.0
git push origin OuariconTremolo-v1.4.0
```

### Delete Failed Tag
```bash
# Local
git tag -d OuariconTremolo-v1.4.0

# Remote
git push origin :refs/tags/OuariconTremolo-v1.4.0
```
