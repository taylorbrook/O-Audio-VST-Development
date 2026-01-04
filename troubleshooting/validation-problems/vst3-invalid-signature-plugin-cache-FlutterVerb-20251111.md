---
plugin: FlutterVerb
date: 2025-11-11
problem_type: validation_failure
component: system
symptoms:
  - "VST3 disappeared from Ableton after rebuild with code changes"
  - "Ableton scanner error: 'Failed to load plugin: a sealed resource is missing or invalid'"
  - "AU loaded but showed old functionality despite binary timestamp being current"
root_cause: config_error
juce_version: 8.0.x
resolution_type: environment_setup
severity: critical
tags: [code-signing, plugin-cache, vst3, audio-unit, macos, ableton, system-cache]
---

# VST3 Invalid Signature and Plugin Cache Issues After Rebuild

## Problem

After rebuilding FlutterVerb v1.0.1 with code changes (5 bug fixes), the plugin appeared to not update:

1. **VST3 completely disappeared** from Ableton's plugin list
2. **AU loaded but showed old behavior** - none of the 5 fixes were active (VU meter dead, MOD_MODE switch non-functional, drive latency present, etc.)
3. Binary timestamps were correct (14:05:52) but functionality was old

## Symptoms

**VST3:**
```
2025-11-11T14:06:56.107087: error: Failed to load plugin: a sealed resource is missing or invalid
2025-11-11T14:06:56.107105: info: VST3: not a plugin
```
(From `~/Library/Preferences/Ableton/Live 12.2.6/PluginScanner.txt`)

**AU:**
- Loaded successfully in Ableton
- Binary timestamp showed 14:05:52 (correct rebuild time)
- All 5 code changes missing (VU meter, MOD_MODE, drive latency, age scaling, decay)
- Binary hash differed from build artifact

## Context

- **Plugin**: FlutterVerb
- **Stage**: Post-implementation (v1.0.0 → v1.0.1 bug fix release)
- **Build**: CMake + Ninja, Release mode, macOS (Apple Silicon)
- **DAW**: Ableton Live 12.2.6
- **JUCE**: 8.0.x
- **OS**: macOS 14.x (Sonoma)

## Investigation Steps

### Attempt 1: Verify Build Success ❌
Checked build log - showed successful compilation and installation at 14:05:52.

**Why this didn't help**: Build completed but code signing failed silently.

### Attempt 2: Check Binary Timestamps ❌
```bash
stat -f "%Sm" ~/Library/Audio/Plug-Ins/VST3/FlutterVerb.vst3/Contents/MacOS/FlutterVerb
# Output: 2025-11-11 14:05:52
```

**Why this didn't help**: Timestamp was correct but signature was invalid.

### Attempt 3: Clear DAW Caches ❌
```bash
rm ~/Library/Preferences/Ableton/Live*/PluginScanner.txt
killall AudioComponentRegistrar
```

**Why this didn't help**: VST3 signature was actually invalid, not just cached wrong.

### Attempt 4: Check Ableton Scanner Log ✅
```bash
tail -50 ~/Library/Preferences/Ableton/Live\ 12.2.6/PluginScanner.txt
```

**Found the smoking gun**:
```
2025-11-11T14:06:56.107087: error: Failed to load plugin: a sealed resource is missing or invalid
2025-11-11T14:06:56.107105: info: VST3: not a plugin
```

### Attempt 5: Compare Binary Hashes ✅
```bash
md5 build/plugins/FlutterVerb/FlutterVerb_artefacts/Release/AU/FlutterVerb.component/Contents/MacOS/FlutterVerb
# d12cefa58bdd11ef4ba0563af88a3bea

md5 ~/Library/Audio/Plug-Ins/Components/FlutterVerb.component/Contents/MacOS/FlutterVerb
# 4b8896984ed342ba4716dedc891f44df  (DIFFERENT!)
```

**Discovered**: Installed AU binary was corrupted/different from build artifact.

## Root Cause

**Two separate issues:**

### Issue 1: VST3 Invalid Code Signature

CMake build process attempted to re-sign the VST3 bundle but failed:

```
/path/to/FlutterVerb.vst3: code has no resources but signature indicates they must be present
-- Replacing invalid signature with ad-hoc signature
```

The "replacement" failed, leaving an invalid signature that macOS rejected. Ableton's plugin scanner refused to load it.

### Issue 2: AU Binary Corruption from Deep Code Signing

Initial fix attempt used:
```bash
codesign --force --deep --sign - ~/Library/Audio/Plug-Ins/Components/FlutterVerb.component
```

The `--deep` flag **modifies the binary** by embedding signatures in nested frameworks, changing the hash. This corrupted the binary, making it different from the build artifact with the new code.

### Issue 3: Aggressive Plugin Caching

Even after fixing Issues 1 and 2, multiple cache layers prevented the new binary from loading:

1. **Ableton plugin scanner cache** (`PluginScanner.txt`)
2. **macOS Audio Unit cache** (`~/Library/Caches/AudioUnitCache`)
3. **System-level AudioComponentRegistrar** (process-level cache)

## Solution

### Step 1: Reinstall from Clean Build Artifacts

```bash
# Remove corrupted installed plugins
rm -rf ~/Library/Audio/Plug-Ins/Components/FlutterVerb.component
rm -rf ~/Library/Audio/Plug-Ins/VST3/FlutterVerb.vst3

# Copy fresh from build (no corruption)
cp -R build/plugins/FlutterVerb/FlutterVerb_artefacts/Release/AU/FlutterVerb.component \
      ~/Library/Audio/Plug-Ins/Components/

cp -R build/plugins/FlutterVerb/FlutterVerb_artefacts/Release/VST3/FlutterVerb.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/
```

### Step 2: Sign Properly (Without --deep)

```bash
# Sign WITHOUT --deep flag (doesn't corrupt binary)
codesign --force --sign - ~/Library/Audio/Plug-Ins/VST3/FlutterVerb.vst3
codesign --force --sign - ~/Library/Audio/Plug-Ins/Components/FlutterVerb.component

# Verify signatures are valid
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/VST3/FlutterVerb.vst3
codesign --verify --deep --strict ~/Library/Audio/Plug-Ins/Components/FlutterVerb.component
```

**Critical**: Do NOT use `--deep` flag when re-signing - it modifies nested bundle contents and corrupts the binary.

### Step 3: Nuclear Cache Clear

```bash
# Kill all DAW processes
killall "Ableton Live 12 Suite" 2>/dev/null

# Clear Ableton plugin cache
rm ~/Library/Preferences/Ableton/Live*/PluginScanner.txt

# Clear macOS Audio Unit caches
rm -rf ~/Library/Caches/AudioUnitCache
rm -rf ~/Library/Caches/com.apple.audiounits.cache

# Kill audio system processes
killall -9 AudioComponentRegistrar auval 2>/dev/null
```

### Step 4: Full System Restart (Required)

**Critical**: A system restart is the ONLY way to guarantee AudioComponentRegistrar releases its cache. Killing the process helps but macOS may restart it with cached state.

```bash
# After cache clearing
sudo reboot
```

### Step 5: Fresh Plugin Load

After restart:
1. Launch Ableton (will rescan all plugins)
2. **Do NOT load existing project with old plugin instance**
3. Create NEW track
4. Add FlutterVerb fresh from plugin browser
5. Verify all changes are present

## Prevention

### 1. Fix CMake Code Signing

Add to `CMakeLists.txt` after `juce_add_plugin()`:

```cmake
if(APPLE)
    # Ensure proper code signing for macOS
    set_target_properties(${PROJECT_NAME}_VST3 PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    )
    set_target_properties(${PROJECT_NAME}_AU PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    )
endif()
```

### 2. Verify Signatures After Build

Add to `scripts/build-and-install.sh`:

```bash
# After installation, verify signatures
echo "→ Verifying code signatures..."
codesign --verify --deep --strict "$VST3_INSTALL_PATH" || {
    echo "✗ VST3 signature invalid, re-signing..."
    codesign --force --sign - "$VST3_INSTALL_PATH"
}

codesign --verify --deep --strict "$AU_INSTALL_PATH" || {
    echo "✗ AU signature invalid, re-signing..."
    codesign --force --sign - "$AU_INSTALL_PATH"
}
```

### 3. Always Clear Caches After Plugin Changes

Create helper script `scripts/clear-plugin-caches.sh`:

```bash
#!/bin/bash
# Clear all plugin caches (run after plugin installation)

echo "Clearing plugin caches..."

# Kill DAWs
killall "Ableton Live 12 Suite" "Logic Pro" "Pro Tools" 2>/dev/null

# Clear Ableton cache
rm ~/Library/Preferences/Ableton/Live*/PluginScanner.txt 2>/dev/null

# Clear AU caches
rm -rf ~/Library/Caches/AudioUnitCache 2>/dev/null
rm -rf ~/Library/Caches/com.apple.audiounits.cache 2>/dev/null

# Kill audio processes
killall -9 AudioComponentRegistrar auval 2>/dev/null

echo "✓ Caches cleared. Restart your DAW."
```

### 4. Verify Binary Hashes Before/After Installation

```bash
# Before installation
BUILD_HASH=$(md5 -q build/.../FlutterVerb)

# After installation
INSTALL_HASH=$(md5 -q ~/Library/Audio/Plug-Ins/.../FlutterVerb)

if [ "$BUILD_HASH" != "$INSTALL_HASH" ]; then
    echo "⚠️  Binary hash mismatch - installation may have corrupted plugin"
fi
```

## Related Issues

- None documented yet (first occurrence)

## References

- Ableton scanner log: `~/Library/Preferences/Ableton/Live X.Y.Z/PluginScanner.txt`
- Apple Code Signing: https://developer.apple.com/documentation/security/code_signing_services
- JUCE Forum: https://forum.juce.com/t/vst3-code-signing-issues/
- macOS codesign man page: `man codesign`

## Testing

After applying fix, verify:

1. **VST3 loads in DAW**: Check plugin browser
2. **AU loads in DAW**: Check plugin browser
3. **Code changes are active**: Test specific functionality
4. **Signature is valid**: `codesign --verify --deep --strict [path]`
5. **No scanner errors**: Check DAW's plugin scanner log

**Test case for FlutterVerb v1.0.1:**
- VU meter should move with audio
- MOD_MODE toggle should click between WET ONLY / WET+DRY
- Drive parameter should not create latency
- Age parameter should be usable up to 50-70%
- Decay should work independently of SIZE

## Notes

- **DO NOT** use `codesign --deep` on already-built plugins - it corrupts them
- System restart is not optional - AudioComponentRegistrar cache persists across process kills
- Hash mismatches are EXPECTED after code signing (signatures embed in binary)
- Always check DAW scanner logs for plugin loading errors
- CMake's automatic re-signing can fail silently - always verify
