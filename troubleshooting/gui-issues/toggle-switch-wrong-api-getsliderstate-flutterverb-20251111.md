---
plugin: FlutterVerb
date: 2025-11-11
problem_type: ui_layout
component: webview
symptoms:
  - Toggle switch not clickable or responsive to mouse events
  - Visual state doesn't update when clicking
  - Switch stuck in one position (couldn't toggle between modes)
root_cause: wrong_api
juce_version: 8.0.9
resolution_type: code_fix
severity: moderate
tags: [webview, toggle, boolean-parameter, juce-api, getToggleState]
---

# Troubleshooting: Toggle Switch Using Wrong JUCE WebView API (getSliderState → getToggleState)

## Problem
Boolean parameter toggle switches were non-functional in JUCE WebView UI. The MOD_MODE toggle in FlutterVerb was stuck on "WET ONLY" and couldn't be switched to "WET+DRY" mode. Initial fix in v1.0.1 used `getSliderState()` which worked but wasn't the correct API for boolean parameters. v1.0.2 corrected this to use `getToggleState()`.

## Environment
- Plugin: FlutterVerb
- JUCE Version: 8.0.9
- Affected: WebView UI (index.html), Stage 5 (GUI integration)
- Date: 2025-11-11

## Symptoms
- Toggle switch element rendered but not clickable
- No visual feedback when clicking switch
- Switch stuck in one state
- Parameter value in C++ didn't change when interacting with UI

## What Didn't Work

**v1.0.0 (broken):** Used `getToggleButtonState()`
- **Why it failed:** This method doesn't exist in JUCE WebView bridge API

**v1.0.1 (worked but wrong):** Used `getSliderState()` with 0.0/1.0 float values
- **Why it worked:** Boolean parameters ARE internally stored as normalized floats (0.0/1.0)
- **Why wrong:** Using slider API for boolean parameters is semantically incorrect and requires unnecessary float comparisons (`value >= 0.5`)

## Solution

Use the correct `getToggleState()` API specifically designed for boolean parameters.

**Code changes (v1.0.1 → v1.0.2):**

```javascript
// v1.0.1 (worked but wrong API):
const modModeState = Juce.getSliderState("MOD_MODE");

modModeToggle.addEventListener("click", () => {
    const currentValue = modModeState.getValue();
    const newValue = currentValue < 0.5 ? 1.0 : 0.0;  // Float comparison needed
    modModeState.setValue(newValue);
});

modModeState.valueChangedEvent.addListener((value) => {
    updateToggleVisual(value >= 0.5);  // Float threshold check
});

// v1.0.2 (correct API):
const modModeState = Juce.getToggleState("MOD_MODE");

modModeToggle.addEventListener("click", () => {
    const currentValue = modModeState.getValue();
    modModeState.setValue(!currentValue);  // Clean boolean toggle
});

modModeState.valueChangedEvent.addListener(() => {
    updateToggleVisual(modModeState.getValue());  // Direct boolean value
});
```

**Key differences:**
1. `getToggleState()` returns true/false (not 0.0/1.0 float)
2. `setValue()` takes boolean (not float requiring threshold)
3. No need for `>= 0.5` comparisons
4. Cleaner, more semantic code

## Why This Works

**Root cause:** JUCE WebView bridge provides TWO distinct APIs for parameters:
- `getSliderState()` - For continuous/ranged parameters (returns normalized float 0.0-1.0)
- `getToggleState()` - For boolean parameters (returns true/false)

Both work for boolean parameters because booleans ARE stored internally as 0.0/1.0 floats in APVTS, but `getToggleState()` provides the correct type abstraction.

**Why v1.0.2 is better:**
1. **Type safety:** Works with booleans, not floats pretending to be booleans
2. **Cleaner code:** `!value` instead of `value < 0.5 ? 1.0 : 0.0`
3. **Semantic correctness:** API name matches intent
4. **No threshold ambiguity:** What if someone uses 0.3? With booleans, no confusion.

**JUCE WebView API pattern:**
```javascript
// Float/continuous parameters (knobs, faders):
const state = Juce.getSliderState(paramId);
state.setValue(0.75);  // Normalized 0.0-1.0

// Boolean parameters (toggles, switches):
const state = Juce.getToggleState(paramId);
state.setValue(true);   // Boolean true/false
```

## Prevention

**When implementing toggle switches in JUCE WebView UI:**

1. ✅ **Use getToggleState() for boolean parameters:**
   ```javascript
   const toggleState = Juce.getToggleState("PARAM_ID");
   toggleState.setValue(!toggleState.getValue());
   ```

2. ✅ **Use getSliderState() for continuous parameters:**
   ```javascript
   const sliderState = Juce.getSliderState("PARAM_ID");
   sliderState.setValue(0.5);  // Normalized 0-1
   ```

3. ✅ **Event listeners work the same for both:**
   ```javascript
   state.valueChangedEvent.addListener(() => {
       const value = state.getValue();  // Boolean or float depending on API
   });
   ```

4. ❌ **Don't mix APIs:**
   - Don't use `getSliderState()` for boolean parameters (technically works but wrong)
   - Don't use `getToggleState()` for continuous parameters (won't work)

5. 🔍 **Check parameter type in C++ APVTS:**
   ```cpp
   // Boolean parameter → use getToggleState() in JS
   std::make_unique<juce::AudioParameterBool>(
       "MOD_MODE", "Mod Mode", false
   )

   // Float parameter → use getSliderState() in JS
   std::make_unique<juce::AudioParameterFloat>(
       "SIZE", "Size", 0.0f, 100.0f, 50.0f
   )
   ```

**Decision tree:**
```
Parameter type?
├─ Boolean (AudioParameterBool) → getToggleState()
├─ Float (AudioParameterFloat) → getSliderState()
├─ Int (AudioParameterInt) → getSliderState()
└─ Choice (AudioParameterChoice) → getSliderState()
```

## Related Issues

- See also: [vu-meter-no-animation-loop-flutterverb-20251111.md](vu-meter-no-animation-loop-flutterverb-20251111.md) - Other FlutterVerb v1.0.2 WebView fix
- Similar pattern: [webview-parameter-undefined-event-callback-TapeAge-20251111.md](webview-parameter-undefined-event-callback-TapeAge-20251111.md) - WebView event callback issues
- API reference: [webview-parameter-not-updating-wrong-event-format-JUCE-20251108.md](../api-usage/webview-parameter-not-updating-wrong-event-format-JUCE-20251108.md) - WebView parameter patterns

**Promoted to Required Reading:** This pattern is now Pattern #19 in `troubleshooting/patterns/juce8-critical-patterns.md` - all subagents will see this before code generation.
