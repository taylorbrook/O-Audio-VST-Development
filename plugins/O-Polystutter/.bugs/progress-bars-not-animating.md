# Bug: Lane Progress Bars Not Animating

**Version:** 1.5.0
**Date:** 2026-01-18
**Status:** FIXED in v1.5.1
**Priority:** Medium
**Fixed:** 2026-01-18

## Resolution

**Root Cause:** CSS selector mismatch in parameter-bindings.js. The HTML has `id="lane1_progress"` directly on the `.progress-fill` element, but the JavaScript was using `document.querySelector("#lane1_progress .progress-fill")` which looks for a nested `.progress-fill` inside `#lane1_progress`. This returned `null`, so the width was never set.

**Fix:** Updated `setupLaneProgressListener()` in parameter-bindings.js:
- Use `document.getElementById("lane1_progress")` directly (this IS the fill element)
- Use `.parentElement` to get the actual `.progress-bar` container for class toggling

---

## Original Bug Report

## Summary

Lane progress bars are visible but do not animate during repeat playback. The implementation is complete but the JUCE WebView → JavaScript event pipeline is not working.

## Expected Behavior

- Progress bars should animate 0-100% during each repeat cycle
- Bars should be dimmed (30% opacity) when lane is not actively repeating
- Updates at ~30Hz for smooth animation

## Actual Behavior

- Progress bars remain static at 0%
- No visual feedback during repeat playback

## Implementation Details

### C++ Side (Complete)

1. **RepeatLane.h/cpp** - Added `getProgress()` method:
   - Returns 0.0-1.0 based on `fractionalPlaybackPosition / (captureLength / pitchRatio)`
   - Location: `Source/DSP/RepeatLane.cpp:546-565`

2. **PluginProcessor.h/cpp** - Added atomic state variables:
   - `lane1Progress`, `lane2Progress`, `lane3Progress`, `lane4Progress` (std::atomic<float>)
   - `lane1Active`, `lane2Active`, `lane3Active`, `lane4Active` (std::atomic<bool>)
   - Updated in `processBlock()` at line 1135-1156

3. **PluginEditor.h/cpp** - Added Timer:
   - Inherits from `juce::Timer`
   - `startTimerHz(30)` in constructor
   - `stopTimer()` in destructor
   - `timerCallback()` builds DynamicObject payload and calls `emitEventIfBrowserIsVisible()`
   - Location: `Source/PluginEditor.cpp:691-734`

### JavaScript Side (Complete)

4. **parameter-bindings.js** - Added event listener:
   - `setupLaneProgressListener()` function at line 708-776
   - Listens via `window.__JUCE__.backend.addEventListener("laneProgress", ...)`
   - Updates `.progress-fill` width and container classes

5. **index.html** - Added CSS:
   - `.progress-active` and `.progress-inactive` classes
   - Location: lines 271-284

## Suspected Issues

1. **Event emission not reaching JavaScript**
   - `emitEventIfBrowserIsVisible()` may not be calling the correct internal method
   - The event ID "laneProgress" may need to be registered somewhere

2. **JUCE WebView event system requirements**
   - May need to register custom event IDs in initialisationData
   - May need to use a different API for custom events

3. **Payload serialization**
   - DynamicObject may not serialize correctly to JavaScript
   - The `emitByBackend()` path may expect a different format

## Debug Steps To Try

1. Add `DBG()` in timerCallback to confirm it's being called
2. Add `console.log` in JS to see if any events arrive
3. Check if `window.__JUCE__.backend` exists at runtime
4. Try using `emitEventIfBrowserIsVisible` with a simple string payload
5. Check JUCE source for how other events are emitted (e.g., slider relays)

## Files Modified

- `Source/DSP/RepeatLane.h` - Added getProgress() declaration
- `Source/DSP/RepeatLane.cpp` - Added getProgress() implementation
- `Source/PluginProcessor.h` - Added atomic progress/active variables
- `Source/PluginProcessor.cpp` - Added progress updates in processBlock
- `Source/PluginEditor.h` - Added Timer inheritance, timerCallback declaration
- `Source/PluginEditor.cpp` - Added timer start/stop, timerCallback implementation
- `Source/ui/public/js/parameter-bindings.js` - Added setupLaneProgressListener()
- `Source/ui/public/index.html` - Added CSS for progress states

## Backup Available

Pre-change backup at: `backups/OuariconPolystutter/v1.4.1/`

## Related Documentation

- JUCE WebBrowserComponent: `/Users/taylorbrook/JUCE/modules/juce_gui_extra/misc/juce_WebBrowserComponent.h`
- JUCE check_native_interop.js: `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/check_native_interop.js`
- Backend.emitByBackend() is the method that forwards C++ events to JS listeners

## Rollback Instructions

To revert to v1.4.1:
```bash
cp -R backups/OuariconPolystutter/v1.4.1/Source/* plugins/OuariconPolystutter/Source/
ninja -C build OuariconPolystutter_All
```
