# Scala Tuning Persistence Bug

**Status:** ✅ COMPLETE (v1.13.4)
**Date:** 2026-01-24

## Problem

When a Scala (.scl) file was loaded, the tuning would work correctly during the session. However, after saving and reopening the project, the Scala tuning was lost and the plugin reverted to 12-TET or another temperament.

## Root Causes (Multiple Issues)

### Issue 1: APVTS tuningMode not synced
When loading a Scala file, the APVTS `tuningMode` parameter wasn't being updated, causing processBlock to reset the mode on restore.

### Issue 2: temperamentPreset not set to Custom
When loading a Scala file, the APVTS `temperamentPreset` parameter wasn't being set to Custom (10), so the wrong preset index was saved.

### Issue 3: processBlock race condition
During state restoration, processBlock could run between APVTS restore and CustomState restore, overwriting the tuning mode.

### Issue 4: WebView UI overwriting backend
When navigating to the Tuning tab, the JavaScript would push stale `currentIntervals` to the backend instead of loading intervals FROM the backend.

## Fixes Applied

### Fix 1: PluginEditor.cpp - Set temperamentPreset on Scala load
```cpp
// v1.13.3: Also update temperamentPreset to Custom (10) for state persistence
if (auto* presetParam = processorRef.getAPVTS().getParameter("temperamentPreset"))
    presetParam->setValueNotifyingHost(10.0f / 10.0f);
```

### Fix 2: PluginProcessor.cpp - Restoration flag
```cpp
// v1.13.3: Flag to prevent processBlock from syncing mode during state restoration
std::atomic<bool> isRestoringState { false };
```

### Fix 3: PluginProcessor.cpp - APVTS sync in custom state restore
```cpp
// v1.13.3: Also update APVTS parameter to prevent processBlock from resetting
if (auto* tuningModeParam = parameters.getParameter("tuningMode"))
    tuningModeParam->setValueNotifyingHost(static_cast<float>(mode) / 2.0f);
```

### Fix 4: index.html - Load intervals FROM backend on mode switch
```javascript
// v1.13.3: For Custom mode, load intervals FROM backend (don't overwrite with stale UI data)
if (mode === 1) {
    await loadIntervalsFromBackend();
}
```

## Files Modified

- `Source/PluginProcessor.cpp` - Multiple fixes for state restoration
- `Source/PluginProcessor.h` - Added isRestoringState flag
- `Source/PluginEditor.cpp` - Set temperamentPreset to Custom when loading Scala
- `Resources/ui/index.html` - Load intervals from backend instead of pushing to backend

## Testing

1. Create a new project (old projects may have stale data)
2. Add OuariconLyrica
3. Load a Scala file
4. Save the DAW project
5. Quit DAW completely
6. Reopen DAW and project
7. Verify the Scala tuning is restored correctly
