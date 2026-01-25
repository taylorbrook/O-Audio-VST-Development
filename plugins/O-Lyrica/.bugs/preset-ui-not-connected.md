# Preset UI Not Connected to Backend

**Status:** 🟢 RESOLVED
**Version:** v1.5.4
**Date:** 2026-01-18
**Priority:** Medium

## Resolution

**Root cause identified:** The ComboBox (dropdown) binding code was throwing an uncaught JavaScript error that crashed the entire ES module before the preset event listeners could be attached.

**Investigation revealed:**
1. Script executed successfully through slider bindings
2. Crashed at `comboState.valueChangedEvent.addListener()` for `stringMaterial` dropdown
3. The uncaught error stopped all subsequent JavaScript execution
4. Preset event listeners (which come after dropdown bindings) were never attached
5. Buttons animated on click (CSS `:active` works) but had no JavaScript handlers

**Fix applied (v1.5.4):**
1. Wrapped ComboBox binding in try-catch to prevent cascade failures
2. Changed preset native function calls to use inline get-and-call pattern for robustness

```javascript
// ComboBox binding now protected:
try {
    comboState.valueChangedEvent.addListener(() => { ... });
} catch (err) {
    console.warn('[Dropdown] Failed to bind ' + paramId + ':', err);
}
```

---

## Historical Investigation Notes

This section preserved for reference.

## Summary (Original)

Preset bar UI elements are visible and styled correctly, but clicking arrows, dropdown, Save, and Load buttons has no effect. The issue persists despite multiple fix attempts.

## Symptoms

- ✓ Preset bar UI elements visible and styled correctly
- ✓ Hover and click animations work on buttons
- ✓ Dropdown arrow (▼) displays correctly
- ❌ Clicking prev/next arrows has no effect
- ❌ Clicking preset name (dropdown) has no effect
- ❌ Save button has no effect
- ❌ Load button has no effect

## Environment

- Plugin: OuariconLyrica v1.5.3
- Presets location: `~/Library/OuariconLyrica/Presets/`
- Factory presets: 48 presets verified present in Factory/ folder
- Reference implementation: Ouaricon Marimba (presets work correctly)

## What Has Been Tried

### v1.5.2 Attempt 1: Add await to getNativeFunction
- **Hypothesis:** `Juce.getNativeFunction()` returns a Promise
- **Fix:** Added `await` to all getNativeFunction calls
- **Result:** ❌ Failed - top-level await broke ES module (WebKit doesn't support it)

### v1.5.2 Attempt 2: Refactor to avoid top-level await
- **Hypothesis:** Top-level await incompatible with WebKit
- **Fix:** Moved all awaits inside async `initializePresetSystem()` function
- **Result:** ❌ Partial - CSS fixed, buttons still non-functional

### v1.5.3 Attempt 3: Remove await from getNativeFunction (match Marimba pattern)
- **Hypothesis:** `getNativeFunction()` returns sync, shouldn't be awaited
- **Fix:** Changed to `const fn = Juce.getNativeFunction("name")` (no await)
- **Comparison:** Matched exact pattern from working Ouaricon Marimba
- **Result:** ❌ Still not working

## Key Differences from Working Marimba Implementation

| Aspect | Lyrica | Marimba |
|--------|--------|---------|
| Native function pattern | Now matches | `const fn = Juce.getNativeFunction()` |
| Script type | `<script type="module">` | `<script type="module">` |
| Preset storage | `~/Library/OuariconLyrica/Presets/` | `~/Library/Ouaricon Marimba/Presets/` |
| C++ registration | PluginEditor.cpp lines 63-181 | PluginEditor.cpp lines 100-160 |

## Next Steps to Investigate

1. **Add visible debug output to UI**
   - Show function types in a debug div: `typeof loadPresetNative`
   - Confirm if functions are actually obtained

2. **Check browser console in standalone mode**
   - Run standalone app, check for JavaScript errors
   - Look for `[Presets]` console logs

3. **Verify C++ native function registration**
   - Add DBG() statements in PluginEditor.cpp native function handlers
   - Confirm if C++ side is receiving calls at all

4. **Compare JUCE bridge script loading**
   - Verify `/js/juce/index.js` is being served correctly
   - Check if `Juce` object exists in WebView context

5. **Test with simpler native function**
   - `getVoiceCount` native function works (voice counter updates)
   - Why do preset functions fail but getVoiceCount succeeds?

6. **Check event propagation**
   - Verify click events are reaching handlers
   - Add `console.log` at start of each click handler

## Files Involved

- `plugins/OuariconLyrica/Resources/ui/index.html` - JavaScript preset system (lines 993-1199)
- `plugins/OuariconLyrica/Source/PluginEditor.cpp` - Native function registration (lines 63-181)
- `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` - Preset manager implementation

## Reference

Working implementation for comparison:
- `plugins/OuariconMarimba/Source/ui/public/index.html` (lines 2073-2259)
- `plugins/OuariconMarimba/Source/PluginEditor.cpp` (lines 133-160)

## Backup

Pre-fix backup available at: `backups/OuariconLyrica/v1.5.2/`
