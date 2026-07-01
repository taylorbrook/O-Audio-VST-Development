---
phase: O-AnalogEQ-code-review
reviewed: 2026-06-30T00:00:00Z
depth: standard
files_reviewed: 6
files_reviewed_list:
  - plugins/O-AnalogEQ/Source/PluginProcessor.cpp
  - plugins/O-AnalogEQ/Source/PluginProcessor.h
  - plugins/O-AnalogEQ/Source/PluginEditor.cpp
  - plugins/O-AnalogEQ/Source/PluginEditor.h
  - plugins/O-AnalogEQ/Source/ui/public/index.html
  - plugins/O-AnalogEQ/Source/ui/public/modules/preset-manager.js
findings:
  critical: 1
  warning: 4
  info: 5
  total: 10
status: issues_found
---

# O-AnalogEQ: Code Review Report

**Reviewed:** 2026-06-30
**Depth:** standard
**Files Reviewed:** 6
**Status:** issues_found

## Summary

Reviewed the audio processor, WebView editor bridge, UI HTML/JS, and the preset-manager JS module for O-AnalogEQ (v1.1.7, shipped/installed). The plugin is structurally sound: the WebView native-function bridge is complete (every `getNativeFunction` call in JS has a matching `withNativeFunction` registration), the resource provider uses explicit path mapping with no traversal risk, denormals are handled, and the VU meter uses a lock-free atomic. No hardcoded secrets, no injection surfaces, no `eval`.

However there is one real-time-safety defect that violates the plugin's own audio-thread contract, plus a user-facing frequency-display correctness bug and several robustness gaps. Key concerns:

1. **BLOCKER** — `processBlock` heap-allocates filter coefficients on the audio thread every block.
2. **WARNING** — Frequency tooltips/readouts display wrong Hz values because the JS ignores the parameter skew.
3. **WARNING** — No coefficient smoothing (zipper noise on automation) and no Nyquist clamp on HF frequency.

## Critical Issues

### CR-01: Filter coefficients heap-allocated on the audio thread every block

**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:262-265`
**Issue:** Each `IIRCoefficients::makeLowShelf` / `makePeakFilter` / `makeHighShelf` call returns a newly `new`-allocated reference-counted `Coefficients<float>` object. These run unconditionally on every `processBlock` invocation (4 allocations + 4 frees per block, on the real-time audio thread), directly violating the plugin's stated contract ("no allocations, locks, or unbounded work on the audio thread"). Under heavy track counts or small buffer sizes this risks priority-inversion glitches and dropouts. The allocation also happens even when no parameter changed, so it is pure overhead.

```cpp
// Current (allocates 4 Coefficients objects on the audio thread every block):
*lfFilter.state  = *IIRCoefficients::makeLowShelf(currentSampleRate, lfFreq, 0.707f, dBtoGain(lfGain));
*lmfFilter.state = *IIRCoefficients::makePeakFilter(currentSampleRate, lmfFreq, qValues[lmfQ], dBtoGain(lmfGain));
*hmfFilter.state = *IIRCoefficients::makePeakFilter(currentSampleRate, hmfFreq, qValues[hmfQ], dBtoGain(hmfGain));
*hfFilter.state  = *IIRCoefficients::makeHighShelf(currentSampleRate, hfFreq, 0.707f, dBtoGain(hfGain));
```

**Fix:** Only recompute when inputs change, and compute coefficients into pre-allocated stack/member storage instead of via the allocating `make*` factory helpers. Two options:

1. Cache last-seen parameter values and skip the whole coefficient update when nothing changed (eliminates the allocation in the common steady-state case):
```cpp
if (lfFreq != lastLfFreq || lfGain != lastLfGain) {
    *lfFilter.state = *IIRCoefficients::makeLowShelf(currentSampleRate, lfFreq, 0.707f, dBtoGain(lfGain));
    lastLfFreq = lfFreq; lastLfGain = lfGain;
}
// ...repeat per band
```
This still allocates on change; for full RT-safety, compute the biquad coefficients directly into the existing `state->coefficients` array (which is already sized) using the RBJ formulas, avoiding the `make*` temporary entirely. Combine with smoothing (see WR-02) so changes are gradual.

## Warnings

### WR-01: Frequency tooltips/readouts show wrong Hz (skew ignored in JS)

**File:** `plugins/O-AnalogEQ/Source/ui/public/index.html:710-717, 742-744`
**Issue:** The C++ frequency parameters use `NormalisableRange<float>(min, max, 0.1f, 0.3f)` — a 0.3 skew — and the factory-preset comment at `PluginProcessor.cpp:86` confirms `normalized = pow((hz-min)/(max-min), 0.3)`. But the JS formatters map the normalized value **linearly**: `v => Math.round(30 + v * 470) + ' Hz'`. `getNormalisedValue()` returns the skewed 0–1 proportion, so the displayed frequency does not match the actual filter frequency. Example: stored value `0.577` (the default = 100 Hz in C++) is displayed as `30 + 0.577*470 ≈ 301 Hz`. Every band's frequency tooltip is wrong, worsening toward the low end of each range. (The dB readouts are correct — gain uses the default skew of 1.0.)

**Fix:** Apply the same skew inverse the C++ range uses before formatting, e.g.:
```js
// proportion (0..1) -> Hz for a NormalisableRange with skew s:
const toHz = (v, min, max, skew) => min + (max - min) * Math.pow(v, 1 / skew);
lf_freq: { format: v => Math.round(toHz(v, 30, 500, 0.3)) + ' Hz' },
// ...per band, using each band's min/max and skew 0.3
```
Update all four `*_freq` formatters (and confirm the SVG notch labels/knob-rotation mapping match the skewed positions).

### WR-02: No parameter smoothing — coefficient jumps cause zipper noise on automation

**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:238-267`
**Issue:** Parameters are read once per block and coefficients are recomputed from the raw values with no smoothing. When a user automates or drags frequency/gain, the coefficients step per-block, producing audible zipper/click artifacts — noticeable on an EQ, especially with the skewed frequency ranges where small normalized moves cause large Hz jumps. `output_gain` is also applied via `Gain` (which smooths internally), but the filter cutoffs/gains are not smoothed.

**Fix:** Smooth the frequency/gain/dB values with `juce::SmoothedValue<float>` (or `LinearSmoothedValue`) prepared in `prepareToPlay`, and either recompute coefficients on sub-block boundaries or per small chunk. This also complements the CR-01 fix (recompute only when the smoothed target is still moving).

### WR-03: HF frequency (up to 20 kHz) is not clamped to Nyquist

**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:252, 265`
**Issue:** `hf_freq` ranges to 20000 Hz and is passed straight to `makeHighShelf(currentSampleRate, hfFreq, ...)`. At 44.1/48 kHz this is fine (Nyquist 22.05/24 kHz), but if the host runs at a sample rate below ~40 kHz the cutoff exceeds Nyquist. JUCE's coefficient math `jassert`s `cutoff <= sampleRate/2` and will produce degenerate/NaN coefficients in release builds (no assert). `hmf_freq` (to 8 kHz) has the same theoretical exposure at very low rates.

**Fix:** Clamp each cutoff before building coefficients:
```cpp
const float nyquist = static_cast<float>(currentSampleRate) * 0.5f;
const float hfClamped = std::min(hfFreq, nyquist * 0.99f);
*hfFilter.state = *IIRCoefficients::makeHighShelf(currentSampleRate, hfClamped, 0.707f, dBtoGain(hfGain));
```

### WR-04: FileChooser async callbacks capture `this` — use-after-free if editor closes mid-dialog

**File:** `plugins/O-AnalogEQ/Source/PluginEditor.cpp:87-113, 120-145`
**Issue:** `savePresetWithDialog` and `loadPresetFromFile` launch an async `FileChooser` whose completion lambda captures `[this, complete]` and dereferences `audioProcessor.presetManager`. If the plugin editor window is closed (editor destroyed) while the native dialog is still open, the callback fires against a destroyed `this`/processor reference, risking a crash. The `fileChooser` member being destroyed does not reliably cancel an already-presented OS dialog on all platforms.

**Fix:** Guard the callback with a lifetime token, e.g. capture a `juce::Component::SafePointer<OuariconAnalogEQAudioProcessorEditor>` (or a `std::weak_ptr` flag) and bail early if it has been deleted:
```cpp
juce::Component::SafePointer<OuariconAnalogEQAudioProcessorEditor> safeThis(this);
fileChooser->launchAsync(flags, [safeThis, complete](const juce::FileChooser& fc) {
    if (safeThis == nullptr) { complete(false); return; }
    // ... existing body via safeThis->audioProcessor
});
```

## Info

### IN-01: `output_gain` parameter is processed but has no UI control

**File:** `plugins/O-AnalogEQ/Source/PluginProcessor.cpp:65-67, 256, 267` / `PluginEditor.cpp` (no relay/attachment) / `index.html` (no widget)
**Issue:** `output_gain` is declared, applied in `processBlock`, and set by every factory preset (e.g. "Surgical Cut" = 0.542), but there is no `WebSliderRelay`/attachment in the editor and no control in the HTML. It is only reachable via host automation. Defaults to 0 dB so it is benign, but it is effectively hidden from the plugin UI.
**Fix:** Either add an output-gain knob (relay + attachment + HTML widget) or remove the parameter if it is intentionally not user-facing. If kept hidden, document the decision.

### IN-02: Double-click knob reset uses 0.5, not the parameter default

**File:** `plugins/O-AnalogEQ/Source/ui/public/index.html:697-706, 785-787`
**Issue:** Double-click resets frequency knobs to `DEFAULT_VALUES[...] = 0.5`, but due to the 0.3 skew, normalized 0.5 is ~77 Hz for LF, whereas the actual parameter default is 100 Hz (normalized 0.577). "Reset" therefore does not restore the true default.
**Fix:** Reset to the real parameter defaults (mirror the C++ defaults / preset "Default" normalized values), or call a native "reset to default" that uses the APVTS default.

### IN-03: Dead variable `currentParamName` in `setupDualKnob`

**File:** `plugins/O-AnalogEQ/Source/ui/public/index.html:735, 759, 762, 808`
**Issue:** `currentParamName` is assigned in `handlePointerDown` and cleared in `pointerup` but never read. Dead state.
**Fix:** Remove `currentParamName` (and its assignments) — `currentState` already carries the needed reference.

### IN-04: `_waitForNative` polls indefinitely with no timeout

**File:** `plugins/O-AnalogEQ/Source/ui/public/modules/preset-manager.js:124-135`
**Issue:** If `window.__JUCE__.backend` never appears, the poll loops every 50 ms forever and `initialize()` never resolves, silently hanging the preset UI with no diagnostic.
**Fix:** Add a bounded retry / timeout that rejects (or logs an error) after N attempts so the failure surfaces.

### IN-05: `promptDelete` relies on `confirm()` which is unreliable in JUCE WebView

**File:** `plugins/O-AnalogEQ/Source/ui/public/modules/preset-manager.js:316-320`
**Issue:** The code comment already flags that `confirm()` may be a no-op in some JUCE WebView contexts; if it returns falsy or throws, deletion silently never happens. (Note: `promptDelete` is wired via `deleteButton`, which is not currently supplied by `index.html`, so this path is presently unreached — but the latent fragility remains if a delete button is added.)
**Fix:** Replace with a native confirm dialog (a `withNativeFunction` that shows `AlertWindow`) or an in-DOM confirmation UI instead of `confirm()`.

---

_Reviewed: 2026-06-30_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
