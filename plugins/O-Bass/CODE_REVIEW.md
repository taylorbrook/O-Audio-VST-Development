---
phase: O-Bass-v1.3.2
reviewed: 2026-07-08T01:28:02Z
depth: deep
files_reviewed: 15
files_reviewed_list:
  - plugins/O-Bass/Source/PluginProcessor.cpp
  - plugins/O-Bass/Source/PluginProcessor.h
  - plugins/O-Bass/Source/PluginEditor.cpp
  - plugins/O-Bass/Source/PluginEditor.h
  - plugins/O-Bass/Source/OuariconPresetManager.h
  - plugins/O-Bass/Source/DSP/CrossoverFilter.cpp
  - plugins/O-Bass/Source/DSP/CrossoverFilter.h
  - plugins/O-Bass/Source/DSP/HarmonicGenerator.cpp
  - plugins/O-Bass/Source/DSP/CleanModeProcessor.cpp
  - plugins/O-Bass/Source/DSP/CleanModeProcessor.h
  - plugins/O-Bass/Source/DSP/MonoSummer.cpp
  - plugins/O-Bass/Source/ui/public/index.html
  - plugins/O-Bass/Source/ui/public/modules/preset-manager.js
  - plugins/O-Bass/CMakeLists.txt
findings:
  critical: 1
  warning: 3
  info: 5
  total: 9
status: issues_found
---

# O-Bass v1.3.2: Code Review Report

**Reviewed:** 2026-07-08T01:28:02Z
**Depth:** deep
**Files Reviewed:** 15
**Status:** issues_found

> Supersedes the previous narrative review (2026-01-27, v1.2.0), whose Priority 1–3 items
> are all resolved in the current tree. This is a fresh pass against the shipped v1.3.2 code
> using the CR/WR/IN severity convention.

## Summary

O-Bass is a well-structured bass enhancer: LR4/FIR dual-mode crossover, Chebyshev harmonic
generation, mono-summed bass processing, and a WebView UI. Most of the codebase's recurring
high-value failure modes are handled correctly:

- **Windows WebView2 static linking:** `NEEDS_WEBVIEW2 TRUE` (CMakeLists.txt:12) +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (CMakeLists.txt:65) are both present — no
  blank-UI-on-Windows regression.
- **Native-function bridge parity:** All 10 preset native functions the JS calls are
  registered in C++; no dead controls. (Two registered fns — `getLimitIndicator`,
  `getOutputLevel` — are unused because meters are *pushed* via `evaluateJavascript`; see IN-05.)
- **processBlock RT-safety:** `ScopedNoDenormals` present; no heap allocation, no locking;
  buffers pre-allocated in `prepareToPlay`; runtime resize checks correctly reduced to
  `jassert`. Params read via cached atomic pointers.
- **IIR coefficient updates:** Cutoff recompute throttled to every 16 samples during
  smoothing (CrossoverFilter.cpp:128) — the old per-sample cost is gone.
- **FIR RT-safety:** FIR bank pre-computed in `prepare()`; runtime changes deferred to the
  next `prepare()` (no `loadImpulseResponse` on the audio thread).
- **Preset name sanitization:** `sanitizePresetName()` strips `/ \ :` (OuariconPresetManager.h:193)
  — the "/" filename regression is handled.
- **Knob readouts:** `formatFrequency` correctly replicates the 0.5 skew, so the on-screen
  Hz value matches the parameter (no 2× drift).

The one blocker is a use-after-free in the async FileChooser completions — the codebase's
documented "WebView `launchAsync` SafePointer" pattern is not applied here. The warnings are
a skew-miscalibrated factory-preset table, an inert `latency_mode` parameter, and a
preset-apply that doesn't reset omitted parameters (O-Bass is still on preset-manager v1.0.2,
predating the v1.0.3 reset-to-defaults fix).

---

## Critical Issues

### CR-01: FileChooser `launchAsync` completions capture raw `this` → use-after-free on editor teardown

**File:** `Source/PluginEditor.cpp:61-85` (`savePresetWithDialog`) and `:132-153` (`loadPresetFromFile`)

Both async dialog completions capture `[this, complete]` with no lifetime guard. If the
editor is destroyed while the native Save/Load dialog is open — close the plugin window,
switch the track, remove the plugin, or reopen the editor — the completion fires against a
freed editor: `processorRef.presetManager…` dereferences freed memory and `complete(...)`
invokes a callback owned by the already-destroyed `WebBrowserComponent` Impl. Both dialogs
are reachable from the UI (the Load and Save buttons in index.html), so this is a live crash
path, not theoretical.

This is exactly the recurring failure documented for this codebase
(`pattern_webview_launchasync_safepointer_no_complete`, shipped as O-Tremolo CR-01 and
O-MicrotonalSampler W12). Note the codebase-specific subtlety: on the teardown path you must
**not** call `complete(false)` — that call is itself a UAF because the completion object is
owned by the dead WebView. Bail with a bare `return`.

**Fix (apply to both completions):**
```cpp
juce::Component::SafePointer<OBassAudioProcessorEditor> safeThis(this);
fileChooser->launchAsync(flags,
    [safeThis, complete](const juce::FileChooser& fc) {
        if (safeThis == nullptr)
            return;                     // editor gone — do NOT call complete()
        // ... existing body, using safeThis->processorRef ...
    });
```

---

## Warning Issues

### WR-01: Factory-preset `crossover_freq` values authored as linear fractions ignore the 0.5 skew

**File:** `Source/PluginProcessor.cpp:78-92` (factory table) + `Source/OuariconPresetManager.h:293`

`crossover_freq` uses `NormalisableRange<float>(40, 200, 1, 0.5)` — **skew 0.5**. Factory
presets are applied via `applyPresetJson → setValueNotifyingHost(value)`, i.e. the stored
floats are treated as **normalized** APVTS values and run through the skewed range. The table
authors them as plain fractions, so they land far lower than a linear reading suggests:

| Stored (normalized) | Actual Hz `40 + 160·v²` | Linear-intended Hz `40 + 160·v` |
|---|---|---|
| 0.25 (Default, Fat Synth, Max) | **50 Hz** | 80 Hz |
| 0.375 (Gentle/Warm Bass Gtr)   | **62.5 Hz** | 100 Hz |
| 0.50 (Subtle Glue, Vintage Bus)| **80 Hz** | 120 Hz |

The proof it's an oversight, not intent: the **Default** preset sets `enhance` 0.50 → 50% and
`output` 0.5 → 0 dB, which exactly match the plugin's initial state — but its `crossover_freq`
0.25 gives **50 Hz**, whereas the true default (createParameterLayout, the UI dblclick-reset,
and `getFrequencyDefault()`) is **80 Hz** (normalized 0.5). So "Default" doesn't reproduce the
default, and the whole table is compressed into the bottom quarter (40–80 Hz) of a 40–200 Hz
control. Matches `pattern_factory_preset_normalized_ignores_skew` (O-SpectralShaper CR-02).

**Fix:** Author the table in engineering units (Hz) and convert per-param:
`convertTo0to1(hz)` on the `crossover_freq` range before storing. For a Default that matches
the plugin, `crossover_freq` should be **0.5** (→80 Hz). `enhance`/`output` are unskewed and
already correct.

### WR-02: `latency_mode` parameter is inert at runtime

**File:** `Source/PluginProcessor.cpp` (processBlock, 176-310)

`crossover.setMode()` / `cleanModeProcessor.setMode()` are called **only** in `prepareToPlay`
(:128, :136). `processBlock` never re-reads `latency_mode`, and the public `setLatencyMode()`
(:365) is never called from anywhere (no attachment, no listener). So changing "Mode"
(Low Latency ↔ High Fidelity) via host automation or the generic editor has **no effect**
until the host re-prepares the plugin (transport reconfig, sample-rate change, reload). Even
the cheap, RT-safe atomic IIR/FIR switch never fires. The parameter presents as functional
but is dead during playback.

**Fix:** In `processBlock`, read `latency_mode` and, on change, call `crossover.setMode()` +
`cleanModeProcessor.setMode()` (both are RT-safe atomic flips; the FIR bank is already
pre-loaded, so only the deferred FIR *index* reload — a documented known limitation — needs a
`prepare()`). Cache the last value to avoid redundant calls. If Mode is intentionally not
user-facing, mark the parameter non-automatable / `meta` instead of leaving it live-but-dead.

### WR-03: `applyPresetJson` does not reset omitted parameters to defaults

**File:** `Source/OuariconPresetManager.h:274-306`

`applyPresetJson` only writes the parameters present in the JSON; omitted keys keep their
current values. Every O-Bass factory preset stores just 3 of 5 params (`crossover_freq`,
`enhance`, `output`) — `latency_mode` and `bypass` are omitted. So loading a factory preset
while **bypass is on leaves the plugin silently bypassed**, and a `latency_mode` set earlier
persists across preset loads. User presets saved by a future build with more params would
likewise bleed state into older partial presets.

This is `pattern_preset_apply_needs_reset_to_defaults`, fixed upstream in the shared
preset-manager **v1.0.3** (reset-to-defaults + factory sentinel). O-Bass is still on the
**v1.0.2** copy (commit 54b9f62). 

**Fix:** Sync the v1.0.3 module (`/module-upgrade preset-manager O-Bass`), or inline the
reset: before applying, iterate all APVTS params and set each to its default
(`param->setValueNotifyingHost(param->getDefaultValue())`), then apply the preset's values.

---

## Info / Nitpick Issues

### IN-01: `calculateHighBandEnergy` reads stale tail samples — and its result is never consumed

**File:** `Source/PluginProcessor.cpp:236, 371-390` + `Source/DSP/CleanModeProcessor.cpp:79-102`

`calculateHighBandEnergy(highBandBuffer)` iterates `highBand.getNumSamples()`, which is the
**allocated** size (`maxBlockSize`), not the current block's `numSamples`. When the host
sends a smaller block, `crossover.process` only writes the first `numSamples`, so the RMS
sums stale audio in the tail. This is masked only because the value goes nowhere:
`cleanModeProcessor.setHighBandEnergy()` stores `highBandEnergy` but `CleanModeProcessor::process`
never reads it (the header calls it "Reserved for future spectral-aware blending"). So this
is a per-block O(N) loop over the whole buffer computing a dead value from partly-stale data.

**Fix:** Remove `calculateHighBandEnergy` + `setHighBandEnergy` (dead path) — that deletes the
wasted CPU and the latent stale-read together. If kept for future use, pass a `numSamples`-sized
view like `lowBandView`/`monoView` do.

### IN-02: Factory presets rewritten to disk on every construction (incl. AU validation)

**File:** `Source/PluginProcessor.cpp:94` → `OuariconPresetManager.h:543-580`

`initializeFactoryPresets` runs from the processor constructor every instantiation:
`createDirectory()` + `replaceWithText` for all 10 JSON files — including during `auval` /
plugin scanning, which the manager's own ctor comment (:216) says lazy-init exists to avoid.
Harmless but unnecessary I/O on every construct, and it means a corrected factory table (WR-01)
only propagates after the next launch overwrites the files.

**Fix:** Only write a factory file when missing (or when a stored version stamp differs), and
defer the directory creation to first preset use as elsewhere.

### IN-03: Per-sample `getSample`/`setSample` in hot loops

**File:** `Source/PluginProcessor.cpp:261-284` (output gain) and `Source/DSP/CrossoverFilter.cpp:150-160` (IIR)

The output-gain loop is sample-outer / channel-inner calling `buffer.getSample(ch,i)` /
`setSample(ch,i)`, which re-fetches the channel pointer array each access. Hoist
`getWritePointer(ch)` per channel (or swap to channel-outer) for a small, free speedup in the
per-sample path. Same shape in the IIR branch of the crossover.

### IN-04: WebView knob readout hardcodes parameter ranges instead of `getScaledValue()`

**File:** `Source/ui/public/index.html:511-537`

`FREQ_MIN/MAX/SKEW` and `OUTPUT_MIN/MAX` are duplicated in JS. They currently match the C++
`NormalisableRange`, so the readout is correct — but any future range/skew change in
`createParameterLayout` drifts silently. Matches `pattern_webview_knob_readout_scaled_value`:
prefer `SliderState.getScaledValue()` so the frontend inherits the real range/skew from JUCE.
No live defect; maintainability only.

### IN-05: "Bypass" isn't a true bypass; two registered native fns are unused

**File:** `Source/PluginProcessor.cpp:189-284`, `Source/PluginEditor.cpp:38-44`

When `bypass` is on, `targetEnhance` → 0 (no harmonics), but the crossover split/recombine,
**output-gain stage, and soft-clip still run** — so a non-0 dB Output still alters the signal
in "bypass". Consider an early-out passthrough (or apply bypass after the gain stage).
Separately, `getLimitIndicator` / `getOutputLevel` are registered as native functions but never
called from JS (meters are pushed via `updateMeters`); harmless dead registrations.

---

## Non-Issues Verified (checked, no action)

- **Denormals:** the old review's "missing denormal flush in HarmonicGenerator" is a non-issue —
  `ScopedNoDenormals` in `processBlock` sets FTZ/DAZ for the whole nested DSP chain.
- **HarmonicGenerator `makeHighPass`/`makeLowPass` allocations:** only in `prepare()`, not RT.
- **State round-trip:** `getStateInformation`/`setStateInformation` round-trip via
  `copyState`/`replaceState` through the preset manager; `setState` is null/parse-safe.
- **`crossover_freq` UI default / dblclick reset:** `getFrequencyDefault()` computes
  `pow(0.25, 0.5) = 0.5` → 80 Hz, correct.

---

## Recommended Resolution Scope

Default scope for `/improve-review O-Bass`: **CR-01 + WR-01..03** (all Critical + Warning).
IN-01..05 are opt-in. CR-01 and WR-03 are both addressable by syncing preset-manager v1.0.3
plus the SafePointer fix; WR-01 is a factory-table edit; WR-02 is a processBlock change.
All are PATCH-level (no param ID/range/state-format changes) — bump **v1.3.3**.

**Next:** `/improve-review O-Bass CR-01, WR-01..03`
