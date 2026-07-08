---
phase: O-SpectralShaper-code-review
reviewed: 2026-07-07T00:00:00Z
depth: deep
files_reviewed: 12
files_reviewed_list:
  - plugins/O-SpectralShaper/Source/PluginProcessor.h
  - plugins/O-SpectralShaper/Source/PluginProcessor.cpp
  - plugins/O-SpectralShaper/Source/PluginEditor.h
  - plugins/O-SpectralShaper/Source/PluginEditor.cpp
  - plugins/O-SpectralShaper/Source/STFTProcessor.h
  - plugins/O-SpectralShaper/Source/STFTProcessor.cpp
  - plugins/O-SpectralShaper/Source/OuariconPresetManager.h
  - plugins/O-SpectralShaper/Resources/ui/js/app.js
  - plugins/O-SpectralShaper/Resources/ui/js/components/CurveEditor.js
  - plugins/O-SpectralShaper/Resources/ui/js/juce/index.js
  - plugins/O-SpectralShaper/Resources/ui/modules/preset-manager.js
  - plugins/O-SpectralShaper/CMakeLists.txt
findings:
  critical: 2
  warning: 3
  info: 5
  total: 10
status: issues
---

# O-SpectralShaper v1.3.1: Code Review Report

**Reviewed:** 2026-07-07
**Depth:** deep
**Status:** issues_found

## Summary

STFT (512-pt, 50% overlap, WOLA synthesis window) spectral transient shaper with 32 logarithmic bands, per-band attack/sustain envelope shaping, and a WebView UI (freehand/node curve editors + WebGL spectrogram). The core is well-built: the overlap-add scheme is correct, WOLA synthesis normalization is right, `ScopedNoDenormals` is present, `numSamples == 0` is guarded, the per-bin gain array correctly prevents multiplicative stacking of collapsed low-frequency bands, the resource provider compares **bare paths** (correct), all `getNativeFunction` calls have matching `withNativeFunction` registrations, and — rare for this repo — CMake has **both** `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. The `parentHierarchyChanged` deferred callback even uses `SafePointer` correctly.

Two Critical findings: (1) the two async `FileChooser` completions capture raw `this`/`complete` without a `SafePointer` — the known shipped-crash class in this repo, and notably the *same file* gets the pattern right elsewhere; (2) every factory preset's `ATTACK_TIME`/`SUSTAIN_TIME` value was authored as a **linear fraction that ignores the 0.3 skew**, so all 9 presets recall materially wrong times and the "Default" preset does not even reproduce the plugin's own power-on defaults (stores 0.198 where 0.615 is required for 10 ms).

Three Warnings: a JS knob readout that re-derives the skewed range with the **wrong formula** (attack shows ~0.3 ms when the real value is 10 ms), curve state that is **not reset** when loading curve-less presets (stale curves persist), and a "Lookahead" control that is **functionally inert** (delays detection and signal equally, so it can never look ahead).

---

## Critical Issues

### CR-01 — Async `FileChooser` completions capture raw `this` + `complete` without `SafePointer` (use-after-free)

**File:** `Source/PluginEditor.cpp:70-89` (`savePresetWithDialog`) and `Source/PluginEditor.cpp:126-145` (`loadPresetFromFile`)

Both preset dialogs launch a system file chooser whose completion lambda captures raw `this` and the WebView-owned `complete` callback:

```cpp
fileChooser->launchAsync(flags,
    [this, complete, makeDialogResult](const juce::FileChooser& fc) {
        auto results = fc.getResults();
        if (results.isEmpty()) { complete(makeDialogResult(false, "")); return; }
        auto presetName = results.getFirst().getFileNameWithoutExtension();
        bool success = processorRef.presetManager.savePreset(presetName);  // this→processorRef via dead editor
        complete(makeDialogResult(success, ...));                          // complete owned by dead WebView Impl
    });
```

If the host closes the plugin window while the dialog is open, the completion still fires against the destroyed editor: reading `processorRef` dereferences a member of a freed object, and invoking `complete` is itself a UAF because its owner (the WebView `Impl`) is gone. This is the exact class fixed in O-MicrotonalSampler v1.23.5 (W12) and flagged as CR-01 in the O-SimpleReverb review. The fix note is explicit that even `complete(false)` on the teardown path is a UAF — bail with a **bare return**.

Note the same file already applies the correct pattern at `PluginEditor.cpp:209-214` (`parentHierarchyChanged`), so this is an inconsistency, not an unknown idiom.

**Fix:** capture a `SafePointer` and return bare on the null path:

```cpp
juce::Component::SafePointer<OSpectralShaperAudioProcessorEditor> safeThis(this);
fileChooser->launchAsync(flags, [safeThis, complete, makeDialogResult](const juce::FileChooser& fc) {
    if (safeThis == nullptr) return;   // bare return — complete() is owned by the dead WebView
    auto results = fc.getResults();
    ...
});
```

Applies to both `savePresetWithDialog` and `loadPresetFromFile`.

---

### CR-02 — Factory preset `ATTACK_TIME`/`SUSTAIN_TIME` values ignore the 0.3 skew → all presets recall wrong times; "Default" ≠ plugin defaults

**File:** `Source/PluginProcessor.cpp:205-269` (factory preset table), applied at `OuariconPresetManager.h:293` via `setValueNotifyingHost`

`FactoryPresetDef::parameters` values are **normalised (0–1)** — `applyPresetJson` feeds them to `param->setValueNotifyingHost(...)`, which runs them through `NormalisableRange::convertFrom0to1` (skew-aware). But the skewed parameters were authored as **linear fractions** `(target − start)/(end − start)`, which is only correct for un-skewed ranges.

`ATTACK_TIME` range is `NormalisableRange(0.1, 50, 0.1, skew=0.3)`. The "Default" preset stores `0.198`:

- `0.198 = (10 − 0.1) / (50 − 0.1)` → the linear fraction for the intended **10 ms**.
- Correct normalised value is `pow(0.198, 0.3) = 0.615`.
- What actually gets applied: `convertFrom0to1(0.198) = 0.1 + 49.9·pow(0.198, 1/0.3) ≈ 0.33 ms`.

Same error for `SUSTAIN_TIME` (`0.184 = (100 − 10)/490`, applied as ≈ 11.7 ms instead of 100 ms). The un-skewed params are fine (`LOOKAHEAD_TIME 0.192` → 2.0 ms, `OUTPUT_GAIN 0.5` → 0 dB) because for a linear range the fraction *is* the normalised value — which is exactly why only the two skewed params are wrong.

Consequences:
- The **"Default"** factory preset does not reproduce the plugin's own power-on state (APVTS defaults 10 ms / 100 ms).
- All 9 factory presets recall attack/sustain times ~10–30× shorter than intended, which changes the envelope-follower coefficients and therefore the audible transient shaping.
- Combined with WR-01, the parameter, its on-screen readout, and the preset that set it will all show three different numbers — the same failure mode called out as Critical in the O-SimpleReverb review.

**Fix (robust):** author factory presets in **engineering units** and convert through the parameter's own range so the skew is handled once, correctly:

```cpp
if (auto* p = parameters.getParameter(paramId))
    normalised = p->convertTo0to1(engineeringValue);   // handles skew for you
```

**Fix (minimal):** skew-correct the two affected columns — e.g. Default `ATTACK_TIME` `0.198 → 0.615`, `SUSTAIN_TIME` `0.184 → 0.602` — and recompute the rest with `pow(fraction, 0.3)`.

---

## Warnings

### WR-01 — Knob readout re-derives the skewed range with the wrong formula (use `getScaledValue()`)

**File:** `Resources/ui/js/app.js:84-107` (attack/sustain `formatValue`)

The `formatValue` callbacks receive the knob's **normalised** value (`RotaryKnob.value`, `RotaryKnob.js:24,95`) and re-implement the range in JS as an **exponential/log** map:

```js
const value = min * Math.pow(max / min, Math.pow(v, 1.0 / skew));  // WRONG shape
```

But JUCE's `NormalisableRange` (and the bridge's own `normalisedToScaledValue`, `js/juce/index.js:248-252`) uses a **linear-with-skew** map: `pow(v, 1/skew)·(end − start) + start`. The two agree only at the endpoints. At v = 0.5 the correct attack value is ≈ 5.05 ms; this formula shows ≈ 0.19 ms. On plugin open the attack knob (default 10 ms, normalised 0.615) displays ≈ 0.3 ms — the static HTML placeholder "10ms" is overwritten with a wrong number.

This is the exact anti-pattern in memory `pattern_webview_knob_readout_scaled_value` ("read 2× wrong for ~20 versions"). Only the two skewed knobs are affected; mix/sensitivity/lookahead/output-gain happen to match because their ranges are linear.

**Fix:** stop re-deriving the range — read the real engineering value JUCE already computed:

```js
app.knobs.attackTime = new RotaryKnob('attack-time-knob-container', 'attack-time-value', {
    formatValue: () => {
        const v = Juce.getSliderState('ATTACK_TIME').getScaledValue();
        return v < 10 ? `${v.toFixed(1)}ms` : `${Math.round(v)}ms`;
    }
});
```

(Audit O-Bells / O-Formant / O-Prism / o-simple* per the same memory note.)

### WR-02 — Loading a curve-less preset does not reset the attack/sustain curves (stale state persists)

**File:** `Source/PluginProcessor.cpp:119-145` (load callback), `Source/OuariconPresetManager.h:300-303` (`applyPresetJson`)

Curve data lives in the preset's `customState`, and `customLoad` only runs `if (customLoad && preset->hasProperty("customState"))`. Four factory presets — **Default, Gentle Shaping, Aggressive Bite, Sustain Lift** — are defined with `juce::var()` (void) custom state, so `initializeFactoryPresets` omits the `customState` property entirely (`OuariconPresetManager.h:564`). Loading one of them never calls `customLoad`, so the attack/sustain curves from a previously-loaded preset (e.g. Punch Enhancer) **remain applied**. Even if `customLoad` were called with void data, `data.getDynamicObject()` returns null and the curves are left untouched.

This is the repo pattern `pattern_preset_apply_needs_reset_to_defaults`: partial presets silently inherit stale state for omitted keys.

**Fix:** reset curves to neutral before applying, so an absent `customState` means "flat":

```cpp
// before dispatching customLoad (or inside it when data is void):
std::array<float,32> flat{}; flat.fill(0.0f);
setAttackCurve(flat); setSustainCurve(flat);
```

or give the four presets an explicit flat-curve `customState` via `makeCurveState({}, {})`.

### WR-03 — "Lookahead" control is functionally inert (and re-reports latency from the audio thread)

**File:** `Source/PluginProcessor.cpp:371-388` (per-sample path), `PluginProcessor.cpp:350-360` (latency)

Both the dry and wet paths are fed the **same** delayed signal:

```cpp
float lookaheadInput = getLookaheadDelayedSample(ch, input);
float dry = getDryDelayedSample(ch, lookaheadInput);
float wet = stftProcessor[ch].processSample(lookaheadInput);   // detection runs on the delayed stream too
```

Real lookahead requires the **detection** to run ahead of the signal being shaped. Here detection and shaping consume the identical delayed stream, so gain and signal stay perfectly time-aligned exactly as they would with the knob off. The control therefore changes nothing audible — it only adds latency. On top of that, toggling it calls `setLatencySamples(...)` from inside `processBlock` (`:360`) with a changed value, i.e. a mid-stream latency change signalled from the audio thread, which many hosts glitch on or ignore.

**Fix:** either implement true lookahead (delay only the reconstructed/output path while detection reads the un-delayed signal), or remove the parameter and its reported latency contribution. If kept as-is, at minimum stop re-reporting latency every block — only call `setLatencySamples` when the value actually changes, and prefer doing it outside the audio callback.

---

## Info / Nitpicks

### IN-01 — Dry path is delayed 511 samples but the STFT wet path is 512 (1-sample dry/wet misalignment)

**File:** `Source/PluginProcessor.cpp:462-475` (`getDryDelayedSample`)

The dry delay is a 512-length circular buffer read at `(writePos + 1) % 512`, which yields **511** samples of delay; the overlap-add STFT wet path has exactly **512** samples of latency (and the host compensates by the reported 512). So at partial Mix the dry component lands one sample early relative to wet, producing a faint HF comb on broadband transients. To match 512, size the buffer 513 (or read `(writePos + 2)`). Impact is small but it's an off-by-one against the stated "FFT_SIZE samples for latency matching."

### IN-02 — Stale comment claims ±18 dB shaping range; actual is ±12 dB

**File:** `Source/STFTProcessor.cpp:301`

`applyEnvelopeShaping` comments say "Curve −1.0 → −18dB … +1.0 → +18dB", but `MAX_SHAPE_DB = 12.0f` (`STFTProcessor.h:109`) and both the header comment (`:108`) and the UI grid (`CurveEditor.js:6,202`, "±12dB") agree on 12. Only the inline comment is wrong — update it.

### IN-03 — Curve double-buffer can tear under rapid message-thread updates

**File:** `Source/STFTProcessor.cpp:335-349` / `276-282`

The attack/sustain curves use a 2-slot SPSC double buffer (write inactive, release-store the index). With a single writer this is safe only if the writer never laps the reader; a freehand drag can fire `setAttackCurve` many times per audio block, and if the writer flips twice while the audio thread is mid-read of a 32-float array, the read can tear. Impact is low (values are gains, and `gainSmoothed` masks a one-frame glitch), but a triple buffer or a small lock-free ring would remove the race.

### IN-04 — Visualization emits every ready frame (~172/s) instead of coalescing to the latest

**File:** `Source/PluginProcessor.cpp:394-405` (push at hop rate) + `PluginEditor.cpp:405-414` (drain-all timer)

Frames are pushed once per 256-sample hop (≈172 Hz at 44.1 kHz) and the 60 Hz timer's `while` loop emits **all** ready frames each tick, so ~172 `emitEvent`s/s of ~3 KB JSON reach the WebView while the JS only paints at `requestAnimationFrame` (~60 Hz). ~2/3 of the parse work is discarded. Coalesce by reading the FIFO down to the most-recent ready frame and emitting only that.

### IN-05 — WebView2 user-data folder is the shared temp root, not a plugin-specific subfolder

**File:** `Source/PluginEditor.cpp:44-47`

`withUserDataFolder(File::tempDirectory)` points WebView2 at the bare temp directory, so its `EBWebView` store is shared with every other Ouaricon plugin doing the same. The memory note recommends a plugin-specific child, e.g. `tempDirectory.getChildFile("OSpectralShaper_WebView")`, to avoid cross-plugin contention.

---

## What's Correct (verified, no action needed)

- Overlap-add FIFO scheme and WOLA synthesis window (`synthW[i] = w[i]/(w²[i]+w²[i+H])`) are correct; `numSamples == 0` guarded; `ScopedNoDenormals` present.
- Per-bin gain array (`applyEnvelopeShaping`) correctly avoids multiplicative gain stacking when multiple low bands collapse to the same FFT bin.
- Resource provider compares **bare paths** (`url == "/js/app.js"`), matching the known-correct repo pattern.
- All 12 JS `getNativeFunction` names (2 curve + 10 preset) have matching C++ `withNativeFunction` registrations — no bridge gap.
- Editor member order (relays → webView → attachments) and the `parentHierarchyChanged` `SafePointer` guard are correct.
- CMake has both `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (Windows-correct — uncommon in this repo).
- Preset-manager v1.0.2 fixes are present: name sanitization (WR-04), real version string (IN-02), prev/next resume (IN-03).
