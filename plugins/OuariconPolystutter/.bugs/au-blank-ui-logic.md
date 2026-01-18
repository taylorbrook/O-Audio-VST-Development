# Bug: AU Blank UI in Logic Pro (v1.3.1)

**Status:** Open
**Severity:** Critical
**Date:** 2026-01-17
**Version:** 1.3.1

## Symptoms

- AU plugin loads in Logic (green checkmark in Plugin Manager)
- UI is completely blank (only shows plugin title "Ouaricon Polystutter")
- No audio processing occurs
- Plugin appears non-functional

## What Works

- ✅ Standalone app - UI displays correctly, audio processes
- ✅ VST3 in other hosts - works normally
- ✅ `auval -v aumf OuPs OuAu` - passes all validation tests
- ✅ Other Ouaricon AU plugins work in Logic (Lyrica, Tremolo, etc.)
- ✅ All other Ouaricon plugins use WebView UI and work fine

## What Was Tried

1. Cleared AU cache (`~/Library/Caches/AudioUnitCache/`)
2. Cleared WebKit cache (`~/Library/WebKit/com.Ouaricon Audio.OuariconPolystutter`)
3. Cleared app caches (`~/Library/Caches/com.Ouaricon Audio.OuariconPolystutter`)
4. Deleted `~/Library/Preferences/com.apple.audio.AudioComponentCache.plist`
5. Killed `AudioComponentRegistrar` process
6. Removed and reinstalled plugin multiple times
7. Clean rebuild (deleted build artifacts, rebuilt from scratch)
8. Removed settings file (`~/Library/Application Support/Ouaricon Polystutter.settings`)
9. Tested in new empty Logic project - same issue
10. Verified binary has no references to removed parameters

## Root Cause (Suspected)

The issue started after v1.3.0 which:
- Removed `envelope_enabled` parameter
- Removed `sidechain_enabled` parameter
- Removed sidechain input bus (was: stereo main + stereo sidechain, now: stereo only)
- Simplified TriggerRouter to MIDI-only

v1.3.1 fixed orphaned references in PluginEditor.cpp/h, which fixed Standalone and VST3, but AU in Logic specifically still fails.

## Possible Causes to Investigate

1. **Logic-specific AU state caching** - Logic may cache AU state/configuration differently than other hosts
2. **Bus configuration change** - Removing the sidechain bus may have triggered some Logic-specific issue
3. **AU type mismatch** - Plugin is `aumf` (MIDI effect), verify this is correct for an audio effect with MIDI input
4. **WebView sandbox issue** - Logic may load AUs in a sandboxed context that affects WebView differently
5. **Parameter ID versioning** - JUCE parameter versioning may cause issues when parameters are removed

## Files Changed in v1.3.0/v1.3.1

- `Source/PluginProcessor.cpp` - Removed parameters, removed sidechain bus
- `Source/PluginProcessor.h` - Removed cached parameter pointers
- `Source/PluginEditor.cpp` - Removed relay/attachment for deleted params (v1.3.1 fix)
- `Source/PluginEditor.h` - Removed relay/attachment declarations (v1.3.1 fix)
- `Source/DSP/TriggerRouter.cpp` - Simplified to MIDI-only
- `Source/DSP/TriggerRouter.h` - Removed envelope/sidechain methods
- `Source/ui/public/index.html` - Removed ENV/SC toggle buttons
- `Source/ui/public/js/parameter-bindings.js` - Removed ENV/SC bindings

## Workaround

Use VST3 version instead of AU in Logic Pro.

## Next Steps

1. Add debug logging to AU initialization path
2. Test in other AU hosts (GarageBand, MainStage, Reaper AU mode)
3. Compare AU binary with working Ouaricon plugins
4. Check if reverting to v1.2.x AU works in Logic
5. Investigate JUCE AU wrapper for bus configuration change handling
