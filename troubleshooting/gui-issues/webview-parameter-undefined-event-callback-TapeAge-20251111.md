---
plugin: TapeAge
date: 2025-11-11
problem_type: ui_layout
component: webview
symptoms:
  - Parameters reset to zero when closing and reopening plugin in DAW
  - Knobs visually animate from saved values back to zero on plugin load
  - Later changed to knobs appearing at 12 o'clock (0 degrees rotation) with no animation
  - JavaScript valueChangedEvent fires with undefined value
root_cause: event_timing
juce_version: 8.0.9
resolution_type: code_fix
severity: critical
tags: [webview, parameters, state-persistence, javascript, event-callback, getNumPrograms, valueChangedEvent]
---

# Troubleshooting: WebView Parameters Reset to Zero Due to Undefined Event Callback Value

## Problem

WebView-based plugin parameters would not persist between DAW sessions. When closing and reopening a project in Ableton Live, parameter knobs would visually animate back to zero, despite C++ state restoration working correctly. After initial fixes, knobs would appear at 12 o'clock (0-degree rotation) with no visual update at all.

## Environment

- Plugin: TapeAge
- JUCE Version: 8.0.9
- Affected: WebView UI (PluginEditor), Stage 5 post-implementation
- DAW: Ableton Live (macOS)
- Date: 2025-11-11

## Symptoms

- Parameters correctly saved via `getStateInformation()` in C++
- Parameters correctly restored via `setStateInformation()` in C++
- C++ logging showed correct parameter values at editor creation
- WebView knobs would visually animate from correct values to zero
- Later, after removing manual initialization, knobs appeared at 12 o'clock (no rotation applied)
- JavaScript debug logging showed `valueChangedEvent` firing with `value=undefined`
- `updateKnobVisual()` received `undefined`, resulting in `angle=NaN`

## What Didn't Work

**Attempted Solution 1:** Fixed `getNumPrograms()` to return 0 instead of 1
- **Why it failed:** This solved an Ableton-specific preset interference issue (documented in `ableton-preset-interference-state-restoration-JUCE-20251107.md`), but parameters still animated to zero. This was a necessary fix but didn't address the root cause.

**Attempted Solution 2:** Removed manual `getNormalisedValue()` calls during initialization
- **Why it failed:** This eliminated the race condition where JavaScript called `getNormalisedValue()` before C++ sent initial values, but knobs stopped updating entirely. The `valueChangedEvent` listener was receiving `undefined` instead of numeric values.

**Attempted Solution 3:** Added extensive C++ and JavaScript logging to trace event sequence
- **Why it failed:** This wasn't a fix attempt—it revealed the actual problem: the event callback parameter was `undefined`, not a number.

## Solution

The root cause was using the `valueChangedEvent` callback parameter directly, which JUCE's WebView system doesn't populate. The fix is to call `getNormalisedValue()` **inside** the callback to read the current value from the state object.

**Code changes:**

```javascript
// Before (broken):
driveState.valueChangedEvent.addListener((newValue) => {
    updateKnobVisual(driveRotatable, newValue);  // newValue is undefined!
});

// After (fixed):
driveState.valueChangedEvent.addListener(() => {
    const value = driveState.getNormalisedValue();  // Read from state object
    updateKnobVisual(driveRotatable, value);
});
```

**Complete fix for all three parameters:**

```javascript
// Drive parameter
driveState.valueChangedEvent.addListener(() => {
    const value = driveState.getNormalisedValue();
    updateKnobVisual(driveRotatable, value);
});

// Age parameter
ageState.valueChangedEvent.addListener(() => {
    const value = ageState.getNormalisedValue();
    updateKnobVisual(ageRotatable, value);
});

// Mix parameter
mixState.valueChangedEvent.addListener(() => {
    const value = mixState.getNormalisedValue();
    updateKnobVisual(mixRotatable, value);
});
```

## Why This Works

**Root cause explanation:**

JUCE's WebView `valueChangedEvent` is a notification event, not a value-passing event. The callback signature is `addListener(callback)` where the callback receives **no parameters** (or undefined parameters). This is different from typical JavaScript event patterns where callbacks receive event data.

The correct pattern (used by GainKnob reference implementation) is:

1. Register a callback that takes no parameters
2. Inside the callback, call `getNormalisedValue()` to read the current value
3. The state object is guaranteed to be updated when the event fires

**Why the undefined value occurred:**

JavaScript was treating the callback as if it had a parameter: `(newValue) => ...`, but JUCE wasn't passing any value. JavaScript's behavior with missing parameters is to set them to `undefined`. When `updateKnobVisual(undefined)` was called, the angle calculation became `NaN`, and no CSS transform was applied.

**Why getNormalisedValue() works inside the callback:**

By the time the `valueChangedEvent` fires, the C++ side has already updated the parameter value via `sendInitialUpdate()` or parameter changes. The JavaScript `SliderState` object reflects this updated value, so calling `getNormalisedValue()` returns the correct normalized (0-1) value.

**Event sequence (correct):**

1. C++ creates `WebSliderParameterAttachment` with correct parameter value
2. Attachment calls `sendInitialUpdate()`
3. JUCE WebView bridge updates JavaScript `SliderState` object
4. `valueChangedEvent` fires (with no callback parameters)
5. JavaScript callback reads value via `getNormalisedValue()`
6. `updateKnobVisual()` receives valid number, applies CSS transform

## Prevention

**For all future WebView plugins:**

1. **Never use callback parameters in `valueChangedEvent` listeners:**
   ```javascript
   // ❌ WRONG
   state.valueChangedEvent.addListener((value) => { ... });

   // ✅ CORRECT
   state.valueChangedEvent.addListener(() => {
       const value = state.getNormalisedValue();
       ...
   });
   ```

2. **Reference the GainKnob implementation pattern:**
   - GainKnob's `render()` method calls `getNormalisedValue()` every time
   - The `valueChangedEvent` listener just triggers `render()`, doesn't use callback params
   - This is the JUCE-documented approach

3. **Do not manually call `getNormalisedValue()` during page load:**
   - Creates race condition (JavaScript loads before C++ sends initial value)
   - Rely entirely on `valueChangedEvent` for all visual updates
   - The event fires automatically after `sendInitialUpdate()`

4. **Add JavaScript logging during development:**
   ```javascript
   state.valueChangedEvent.addListener(() => {
       const value = state.getNormalisedValue();
       window.__JUCE__.backend.emitEvent("jsLog", `Value: ${value}`);
       updateVisual(value);
   });
   ```

5. **Test parameter persistence early:**
   - Change parameters
   - Save DAW project
   - Close DAW completely
   - Reopen project
   - Verify parameters restored correctly

6. **Check C++ `getNumPrograms()` returns 0:**
   - Returning 1+ causes Ableton to interfere with state restoration
   - See `ableton-preset-interference-state-restoration-JUCE-20251107.md`

## Related Issues

- See also: [ableton-preset-interference-state-restoration-JUCE-20251107.md](../parameter-issues/ableton-preset-interference-state-restoration-JUCE-20251107.md) - Ableton calling `setCurrentProgram()` after state restoration
- See also: Pattern #12 in [juce8-critical-patterns.md](../patterns/juce8-critical-patterns.md) - WebSliderParameterAttachment requires 3 parameters in JUCE 8

## References

- JUCE GainKnob example: Reference implementation showing correct `valueChangedEvent` usage
- Deep research findings: WebView initialization timing and event-driven patterns
