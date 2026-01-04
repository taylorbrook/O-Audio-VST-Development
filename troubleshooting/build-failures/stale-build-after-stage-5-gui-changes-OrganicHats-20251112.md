---
plugin: OrganicHats
date: 2025-11-12
problem_type: build_error
component: system
symptoms:
  - "Plugin shows 'Stage 2' placeholder UI after completing Stage 5 WebView integration"
  - "Installed binary timestamp predates Stage 5 code changes"
  - "validator subagent reported successful build but artifacts were stale"
root_cause: logic_error
resolution_type: environment_setup
severity: moderate
tags: [build, stage-6, validator, webview, installation, workflow]
---

# Troubleshooting: Stale Build Installed After Stage 5 GUI Changes

## Problem
After completing Stage 5 (WebView GUI integration) and Stage 6 (validation), the installed plugin still showed the Stage 2 placeholder UI ("OrganicHats - Stage 2" text on dark background) instead of the complete WebView UI with 6 knobs, dual panels, and studio hardware aesthetic.

## Environment
- Plugin: OrganicHats
- JUCE Version: 8.x
- Affected: Stage 6 validation workflow, build-and-install process
- Date: 2025-11-12

## Symptoms
- Plugin UI displays "OrganicHats - Stage 2" placeholder text
- WebView UI components not visible (no knobs, no dual panels, no power LED)
- Installed binary timestamp (12:40) predates Stage 5 code changes (12:52)
- validator subagent reported "Build verified" but used stale artifacts
- Restarting DAW didn't fix the issue (confirming it's not a cache problem)

## What Didn't Work

**Attempted Solution 1:** Restart DAW and clear caches
- **Why it failed:** The problem wasn't DAW caching - the installed binary genuinely lacked the Stage 5 changes

**Attempted Solution 2:** Check for missing WebView files (Source/ui/public/index.html)
- **Why it failed:** Files existed in source tree with correct timestamps, but weren't in the built binary

## Solution

**Root cause:** The validator subagent during Stage 6 checked that Release build artifacts existed and reported success, but those artifacts were from Stage 4 (before Stage 5 GUI changes). The workflow never rebuilt after Stage 5 code was committed.

**Fix:**
```bash
# 1. Clean stale build artifacts
rm -rf build

# 2. Rebuild from root with Stage 5 changes
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target OrganicHats_VST3 OrganicHats_AU

# 3. Remove old installations
rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/OrganicHats.vst3"
rm -rf "$HOME/Library/Audio/Plug-Ins/Components/OrganicHats.component"

# 4. Install fresh builds
cp -R "build/plugins/OrganicHats/OrganicHats_artefacts/Release/VST3/OrganicHats.vst3" \
    ~/Library/Audio/Plug-Ins/VST3/
cp -R "build/plugins/OrganicHats/OrganicHats_artefacts/Release/AU/OrganicHats.component" \
    ~/Library/Audio/Plug-Ins/Components/

# 5. Set permissions and clear caches
chmod -R 755 "$HOME/Library/Audio/Plug-Ins/VST3/OrganicHats.vst3"
chmod -R 755 "$HOME/Library/Audio/Plug-Ins/Components/OrganicHats.component"
killall -9 AudioComponentRegistrar 2>/dev/null

# 6. Restart DAW
```

**Verification:**
```bash
# Check installed binary timestamp is AFTER Stage 5 changes
stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" \
  ~/Library/Audio/Plug-Ins/VST3/OrganicHats.vst3/Contents/MacOS/OrganicHats

# Should show recent time (e.g., 13:07:27), not old time (12:40:xx)
```

## Why This Works

**Root cause:** The validator subagent's build verification in Stage 6 only checked if Release artifacts *existed*, not whether they were *up-to-date* with recent code changes. The build artifacts from Stage 4 (12:40) were still present when Stage 6 ran, so the validator passed them as valid.

**Timeline of events:**
1. **Stage 4 (12:40):** DSP implementation complete, Release build created
2. **Stage 5 (12:52):** WebView UI code committed (PluginEditor.cpp, index.html, etc.)
3. **Stage 6 (12:56):** validator checked for artifacts, found Stage 4 builds, reported success
4. **Installation (12:59):** Installed the stale Stage 4 builds that lack Stage 5 WebView changes

**Why clean rebuild works:**
- Removes all stale artifacts (`rm -rf build`)
- Forces fresh compilation of Stage 5 changes
- Ensures binary includes all recent code (WebView UI, parameter bindings, resource provider)

**The validator's assumption:** If Release build exists and is recent, it's valid. This breaks when code changes happen between stages without triggering a rebuild.

## Prevention

**RESOLVED: 2025-11-12**

This issue has been fixed in the Stage 6 workflow. The fix adds an explicit rebuild step at the start of Stage 6 validation.

**Implementation:**
- **File modified:** `.claude/skills/plugin-workflow/references/stage-6-validation.md`
- **Location:** Lines 21-76 (new Step 1: Rebuild with Stage 5 changes)
- **What changed:**
  1. Added mandatory rebuild before preset creation
  2. Added timestamp verification (binary vs source files)
  3. Fails fast if binary is stale or build fails

**For validator subagent (Stage 6):**
1. **Don't assume existing builds are current** - Check artifact timestamps against source file timestamps ✅ **FIXED**
2. **Always rebuild in Stage 6** - Run `cmake --build build --config Release` even if artifacts exist ✅ **FIXED**
3. **Verify build includes recent changes** - Check that binary size/timestamp changed after rebuild ✅ **FIXED**

**For workflow orchestration:**
Explicit rebuild step now exists in Stage 6 orchestration (stage-6-validation.md, Step 1):

```bash
# Rebuild from root directory (matches foundation-agent build location)
echo "Rebuilding ${PLUGIN_NAME} with Stage 5 GUI changes..."

cmake --build build --config Release \
  --target ${PLUGIN_NAME}_VST3 \
  --target ${PLUGIN_NAME}_AU \
  --parallel

# Timestamp verification ensures binary is newer than source
LATEST_SOURCE=$(find plugins/${PLUGIN_NAME}/Source -type f -exec stat -f "%m" {} \; 2>/dev/null | sort -n | tail -1)
VST3_BINARY="build/plugins/${PLUGIN_NAME}/${PLUGIN_NAME}_artefacts/Release/VST3/${PRODUCT_NAME}.vst3/Contents/MacOS/${PRODUCT_NAME}"

if [ -f "$VST3_BINARY" ]; then
  BINARY_TIME=$(stat -f "%m" "$VST3_BINARY" 2>/dev/null)
  if [ -n "$BINARY_TIME" ] && [ -n "$LATEST_SOURCE" ] && [ $BINARY_TIME -lt $LATEST_SOURCE ]; then
    echo "❌ ERROR: Binary is older than source files"
    exit 1
  fi
fi
```

**For installation process:**
Verify installed binary timestamp is newer than source file timestamps:

```bash
# Get most recent source modification time
LATEST_SOURCE=$(find plugins/${PLUGIN_NAME}/Source -type f -exec stat -f "%m" {} \; | sort -n | tail -1)

# Get installed binary modification time
INSTALLED_TIME=$(stat -f "%m" ~/Library/Audio/Plug-Ins/VST3/${PLUGIN_NAME}.vst3/Contents/MacOS/${PLUGIN_NAME})

# Warn if binary is older than source
if [ $INSTALLED_TIME -lt $LATEST_SOURCE ]; then
    echo "⚠️  WARNING: Installed binary is older than source code"
    echo "   Consider rebuilding with: cmake --build build --config Release"
fi
```

**Detection pattern:**
If user reports UI shows "Stage 2" or placeholder text after completing later stages:
1. Check installed binary timestamp vs source timestamps
2. Check git log for recent commits vs build artifact times
3. Clean rebuild and reinstall

## Related Issues

No related issues documented yet.
