---
phase: O-Bells-v4.1.0
reviewed: 2026-07-08
depth: deep
files_reviewed: 27
files_reviewed_list:
  - plugins/O-Bells/Source/PluginProcessor.cpp
  - plugins/O-Bells/Source/PluginProcessor.h
  - plugins/O-Bells/Source/PluginEditor.cpp
  - plugins/O-Bells/Source/PluginEditor.h
  - plugins/O-Bells/Source/BellVoice.cpp
  - plugins/O-Bells/Source/BellVoice.h
  - plugins/O-Bells/Source/BellSound.h
  - plugins/O-Bells/Source/DSP/EQProcessor.cpp
  - plugins/O-Bells/Source/DSP/EQProcessor.h
  - plugins/O-Bells/Source/DSP/DelayProcessor.cpp
  - plugins/O-Bells/Source/DSP/DelayProcessor.h
  - plugins/O-Bells/Source/DSP/ReverbProcessor.cpp
  - plugins/O-Bells/Source/DSP/ReverbProcessor.h
  - plugins/O-Bells/Source/EmbeddedTunings.cpp
  - plugins/O-Bells/Source/EmbeddedTunings.h
  - plugins/O-Bells/Source/TuningEngine.cpp
  - plugins/O-Bells/Source/TuningEngine.h
  - plugins/O-Bells/Source/ScaleGenerator.cpp
  - plugins/O-Bells/Source/ScaleGenerator.h
  - plugins/O-Bells/Source/TuningExporter.cpp
  - plugins/O-Bells/Source/TuningExporter.h
  - plugins/O-Bells/Source/OuariconPresetManager.h
  - plugins/O-Bells/Resources/ui/index.html
  - plugins/O-Bells/Resources/ui/js/tuning-panel.js
  - plugins/O-Bells/Resources/ui/modules/instrument-footer-panel.js
  - plugins/O-Bells/Resources/ui/js/juce/index.js
  - plugins/O-Bells/CMakeLists.txt
findings:
  critical: 3
  warning: 12
  info: 13
  total: 28
status: issues_found
---

# O-Bells v4.1.0: Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: DSP/RT-safety · WebView bridge/editor · tuning/presets/build)
**Files Reviewed:** 27
**Status:** issues_found

## Summary

O-Bells is a procedural physical-modeling bell synth: multi-stage modal voice DSP, a full
effects tab (Chorus/Delay/EQ/8-channel FDN Reverb), a Scala/KBM tuning engine, VST3 Note
Expression for Dorico microtonal playback, and a WebView UI. The core real-time path and the
tuning math are in good shape; the defects cluster in **parameter/preset plumbing** and
**WebView readout/binding**.

Several of this codebase's recurring high-value failure modes are handled **correctly** in
O-Bells:

- **Embedded-tuning PERIOD is appended before `setCustomIntervals`** (`PluginEditor.cpp:626-627`,
  `intervals.push_back(tuning->period)`), so O-Bells is **not** affected by the
  `pattern_embedded_tuning_period_dropped` bug that hit O-Formant v1.25.0. All 24 embedded
  tunings carry correct `period` fields (1200 octave, 1901.955 tritave for Bohlen-Pierce,
  Carlos Alpha/Beta/Gamma). ✓
- **Windows WebView2 static linking:** both `NEEDS_WEBVIEW2 TRUE` (CMakeLists.txt:21) and
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (CMakeLists.txt:82) present — no blank-UI-on-Windows. ✓
- **Single `juce_add_binary_data` target** (`O-Bells_UIResources`) — no `BinaryData` namespace
  collision (procedural synth, no embedded audio). ✓
- **processBlock RT-safety:** `ScopedNoDenormals` at the top (PluginProcessor.cpp:737); all
  params read via cached `std::atomic<float>*` acquired in `prepareToPlay`; no audio-thread heap
  allocation or locking **except CR-02**; buffers pre-allocated in `prepare()`. ✓
- **Native-function bridge parity:** every `getNativeFunction("…")` the JS calls is registered
  in C++ (presets + all 19 tuning fns + on-screen keyboard `sendMidiNote`) — no silently-dead
  native-fn control. ✓
- **Editor teardown order:** `~Editor` calls `stopTimer()` first; member order (attachments →
  webView → relays) is the documented-correct reverse-destruction order. ✓
- **`getLatencySamples` not overridden** (correct — non-virtual in JUCE 8). ✓
- **TuningEngine threading:** lock-free atomic frequency table read on the audio thread;
  message-thread rebuilds under `intervalMutex` with no recursive-lock deadlock. ✓

The three blockers: (CR-01) the **factory preset library is effectively broken** — factory
values are authored in engineering units but applied through JUCE's *normalized*
`setValueNotifyingHost`, so every non-[0,1]-ranged param recalls slammed to a rail; (CR-02)
the EQ recomputes IIR coefficients with the heap-allocating `Coefficients::makeXXX` factories
on the audio thread — the codebase's documented `ArrayCoefficients` pattern is not applied
here; and (CR-03) all seven FileChooser `launchAsync` completions capture raw `this` with no
`SafePointer`, a use-after-free on editor teardown. The warnings are a dead `material` control
(slider-vs-combobox channel mismatch) and a family of knob readouts that re-derive engineering
units from hardcoded JS constants instead of `SliderState.getScaledValue()`.

---

## Critical Issues

### CR-01: Factory presets store engineering-unit values but apply through the *normalized* parameter setter → every non-[0,1] param recalls at its rail

**Files:** `Source/OuariconPresetManager.h:203-207` (apply) + `:506-507` (store); factory table
`Source/PluginProcessor.cpp:1077+`

Factory presets are authored in **engineering units** and written to JSON verbatim
(`initializeFactoryPresets`, OuariconPresetManager.h:506-507). On load, `applyPresetJson`
applies each value through `param->setValueNotifyingHost(static_cast<float>(prop.value))`
(OuariconPresetManager.h:206) — but `RangedAudioParameter::setValueNotifyingHost` takes a
**normalized [0,1]** value and clamps. No `convertTo0to1` exists anywhere in the plugin.
(User-saved presets are *not* affected: `createPresetJson` stores `getValue()`, already
normalized — OuariconPresetManager.h:174-175 — which is why the bug is invisible in normal use.)

Verified against the factory table (`PluginProcessor.cpp:1077+`): the table mixes correctly-
normalized values (`strikePosition 0.25`, `damping 0.95` — fine, those ranges are 0–1) with raw
engineering units on skewed/wide-range params, all of which clamp to a rail:

| Param | Range | Factory value(s) | Recalls as |
|---|---|---|---|
| `airAbsorptionTime` | 0.1–10 (skew .5) | 2.0–6.0 | 10 s (max) |
| `unisonDetune` | 0–50 | 8–18 | 50 (max) |
| `strikeTime` | 5–100 | 8–60 | 100 (max) |
| `brilliance` | 0–100 | 22–92 | 100 (max) |
| `bodyTime` | 100–5000 | 800–4500 | 5000 (max) |
| `humSustain` | 0–100 | 15–95 | 100 (max) |
| `lpFilterCutoff` | 200–20000 (skew .3) | 6500–8000 | 20000 Hz (max) |
| `pitchEnvTime` | 5–200 (skew .5) | 12–120 | 200 (max) |
| `partialTuning` | −100..100 | −5, −8 | −100 (min rail) |
| `material` (5-choice) | index 0–4 | 1.0, 4.0 | index 4 "Cast Iron" |

**Impact:** the ~20 curated factory presets collapse toward identical, maxed-out timbres —
the opposite of the research-informed design in the changelog. This is a more severe variant
of `pattern_factory_preset_normalized_ignores_skew`: O-Bells doesn't merely ignore skew, it
pushes raw engineering units through a normalized setter.

**Fix (pick one canonical convention — do not double-convert):**
- Preferred: author the factory table in engineering units and store
  `range.convertTo0to1(value)` in `initializeFactoryPresets`, so on-disk JSON matches the
  normalized convention `createPresetJson`/user presets already use; or
- Convert on apply: `if (auto* rp = dynamic_cast<juce::RangedAudioParameter*>(param))
  rp->setValueNotifyingHost(rp->getNormalisableRange().convertTo0to1((float)prop.value));`
  — but then user presets (already normalized) would double-convert, so this requires a
  format flag. The store-side fix is cleaner.

---

### CR-02: `EQProcessor::process()` recomputes IIR coefficients with the heap-allocating `Coefficients::makeXXX` factories on the audio thread

**File:** `Source/DSP/EQProcessor.cpp:54, 62, 71` (called from `PluginProcessor.cpp:951`)

`EQProcessor::process()` runs on the audio thread and, when a band's gain/freq differs from its
cached `prev*`, calls the ref-counted-`Coefficients` factories:
- `:54` `*lowShelf.state  = *FilterCoeffs::makeLowShelf(...)`
- `:62` `*midPeak.state   = *FilterCoeffs::makePeakFilter(...)`
- `:71` `*highShelf.state = *FilterCoeffs::makeHighShelf(...)`

Each `makeXXX` returns a `Coefficients<float>::Ptr` — it **heap-allocates** a new ref-counted
object (and its internal coefficient `Array`) every call, then frees it when the temporary
`Ptr` dies. That is a malloc+free on the audio thread. This is exactly the documented
`pattern_arraycoefficients_rt_safe_iir` (shipped as O-Formant v1.25.1 WR-08).

**Failure scenario:** at rest the dirty-flag gate suppresses the alloc, but dragging or
automating `eqLowGain` / `eqMidGain` / `eqMidFreq` / `eqHighGain` fires `makeXXX` **every block**
for the gesture's duration → repeated audio-thread allocation → priority-inversion / dropout
risk under load. (Also fires 3× on the first processed block because `prev*` init to `-999`.)

**Fix:** use `juce::dsp::IIR::ArrayCoefficients<float>::makeXXX(...)` (stack `std::array<float,6>`,
identical math) and copy in place into the already-allocated storage — no alloc:
```cpp
auto c = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(
    currentSampleRate, 200.0f, 0.707f, juce::Decibels::decibelsToGain(lowGain));
std::copy(c.begin(), c.end(), lowShelf.state->getRawCoefficients());
```
Same for mid/high. Reference the O-Formant EQProcessor for the exact in-place idiom. (The
`prepare()`-time `makeXXX` calls, EQProcessor.cpp:21-29, run on the message thread and are fine,
but migrate for consistency.)

---

### CR-03: FileChooser `launchAsync` completions capture raw `this` with no `SafePointer` (7 sites) → use-after-free on editor teardown

**File:** `Source/PluginEditor.cpp` — `savePresetWithDialog` (286-300), `loadPresetFromFile`
(310-323), `loadScalaFile` (450-461), `saveScalaFile` (470-481), `loadKBMFile` (489-499),
`saveKBMFile` (508-519), `exportTuningHTML` (644-655)

Every `launchAsync` completion captures `[this, complete]` with no lifetime guard. The
`FileChooser` is a `shared_ptr` **member** of the editor (`fileChooser` h:204,
`tuningFileChooser` h:207), so if the host tears down the editor while a native dialog is open
(close the plugin window, switch track, remove the plugin), the completion later fires against
a freed editor and dereferences `this->processorRef` / `this->fileChooser`.

**Codebase-specific subtlety (verified):** on the cancel/teardown path these call
`complete(juce::var(""))` (e.g. PluginEditor.cpp:292) before returning. Per
`pattern_webview_launchasync_safepointer_no_complete` (shipped O-MicrotonalSampler v1.23.5 W12),
`complete` is owned by the already-destroyed `WebBrowserComponent` Impl, so **calling
`complete()` on the null path is itself a UAF** — the null path must bail with a **bare
`return`**, not `complete(false)`/`complete("")`.

**Fix:** at the top of each completion:
```cpp
juce::Component::SafePointer<OBellsAudioProcessorEditor> safeThis(this);
// inside completion:
if (safeThis == nullptr) return;   // bare return — do NOT call complete()
// ... use safeThis->processorRef ...
```

---

## Warnings

### WR-01: `applyPresetJson` never resets parameters to defaults before applying → omitted keys inherit stale state

**File:** `Source/OuariconPresetManager.h:189-215`

`applyPresetJson` iterates only the keys present in the preset JSON — no default-reset pass.
This O-Bells copy is a pre-v1.0.3 fork ("Extended for O-Bells", header :9) and lacks the
reset-to-defaults the shared preset-manager module gained in v1.0.3
(`pattern_preset_apply_needs_reset_to_defaults`). Factory presets are partial: e.g. "Massive
Iron Bell" sets `lpFilterEnabled=1`+`lpFilterCutoff`, but "Cavernous Brass" omits both → the
low-pass stays enabled from the previous preset. The entire FX section (`chorus*`, `delay*`,
`eq*`, `reverbBypass`, `outputGain`, `tuning_*`) is never named by any factory preset, so
switching presets never restores those to a known state. Old user saves missing newer keys
behave the same way.

**Fix:** at the top of `applyPresetJson`, iterate all `getParameters()` and set each to its
default (`param->setValueNotifyingHost(param->getDefaultValue())`) before applying the preset's keys.

### WR-02: Preset name not sanitized before use as a filename → "/" silently fails to save

**File:** `Source/OuariconPresetManager.h:229` (+ `:159, 253, 268, 290, 328`)

`savePreset` builds `getUserPresetsDirectory().getChildFile(presetName + ".json")` with no
sanitization (no `createLegalFileName`/`sanitizePresetName`, grep: 0 hits). This is the
documented `critical_preset_name_slash_path_separator` bug — a name like "Bright / Steel" makes
`getChildFile` treat "/" as a path separator, the write lands in a phantom subdir or fails, and
the non-recursive `getPresetList` never surfaces it.

**Fix:** add `sanitizePresetName()` (strip/replace `/ \ :`, e.g. `juce::File::createLegalFileName`),
apply it in `savePreset` before building the path, and reject an empty result.

### WR-03: `material` is a ComboBox in C++ but bound as a slider in JS → dead control

**Files:** `Resources/ui/index.html:1652` (registered in the slider list, `class="slider"
data-param="material"` at :1191) + `getSliderState('material')`; C++ `WebComboBoxRelay`
(`PluginEditor.cpp:90`) + `WebComboBoxParameterAttachment` (`:791`); param is 5-choice
`AudioParameterChoice` (`PluginProcessor.cpp:95-101`).

JUCE keys sliders on `"__juce__slider"+name` and comboboxes on `"__juce__comboBox"+name`, so
the two never meet: dragging the material slider emits on a channel no C++ relay listens to →
**the material parameter never changes from the UI**, and combobox-channel updates
(preset/automation) never move the slider. `getSliderState('material')` also logs a
`console.warn` and proceeds with default `{start:0,end:1,skew:1}` props that never update.
Knock-on: the CPU-decay estimator reads `parameterStates.get('material').getNormalisedValue()`
(index.html:1943) → permanently stuck at Bronze.

**Fix:** bind `material` via `Juce.getComboBoxState('material')` and render it as a 5-way choice
control (preferred), or register a matching `WebSliderRelay`+`WebSliderParameterAttachment`.

### WR-04: `outputGain` dB readout formula is ~3× too steep

**Files:** `Resources/ui/index.html:1699` (main) and `:2011` (footer): `Math.round((v - 0.67) *
36 / 0.33)`. C++ range `NormalisableRange(-24, 12, 0.1)` (`PluginProcessor.cpp:529`).

The correct mapping is `-24 + norm*36`. The JS slope `36/0.33 ≈ 109` dB per full travel (vs the
true 36 dB). It reads ~0 dB near `norm=0.67`, but displays **+36 dB** at max (actual +12) and
**−73 dB** at min (actual −24). Both instances share the bug.

**Fix:** use `state.getScaledValue()` (already dB) for the readout, or `v => (-24 + v*36).toFixed(1)`.

### WR-05: `airAbsorptionTime` readout treats the normalized value as seconds

**File:** `Resources/ui/index.html:1647-1651`. C++ range `NormalisableRange(0.1, 10, 0.1, 0.5)`
— skewed (`PluginProcessor.cpp:84`).

The formatter receives `getNormalisedValue()` (0–1) but uses it directly as seconds. Since
normalized is always ≤1 it **always shows milliseconds** and ignores the skew: default 2.0 s
(`norm≈0.438`) displays as `438 ms`; full travel shows `1000 ms` instead of `10.0 s`.

**Fix:** format `state.getScaledValue()` (real seconds).

### WR-06: `pitchEnvTime` readout linearly decodes a skew-0.5 range

**File:** `Resources/ui/index.html:1678`: `Math.round(5 + v*195)`. C++ `NormalisableRange(5, 200,
1, 0.5)` (`PluginProcessor.cpp:320`).

Linear JS decode of a skewed C++ range → ~2× off mid-range (`norm=0.5` shows 102 ms; actual ≈54 ms).

**Fix:** use `getScaledValue()`.

### WR-07: `eqMidFreq` readout AND double-click-edit linearly decode a skew-0.5 range

**Files:** display `Resources/ui/index.html:2573` → linear map at `:2376`; edit inverse in
`commitValue` `:2440`. C++ `NormalisableRange(200, 8000, 1, 0.5)` (`PluginProcessor.cpp:498`).

`eqMidFreq` is the only FX param with a skewed C++ range; the other FX knobs are linear so
their linear JS decode is coincidentally correct. Here `norm=0.5` displays **4100 Hz** but the
true center is **2150 Hz**, and typing `1000` Hz sets `norm=0.103` → actual ≈**282 Hz**.

**Fix:** drive both display and edit through `getScaledValue()` / the C++ `properties` skew.

### WR-08: Tuning APVTS params are relayed but unused by the JS — the UI drives `TuningEngine` via native fns (dual source of truth)

**Files:** C++ creates + attaches `tuning_masterTune`, `tuning_octaveStretch`,
`tuning_pitchBendRange` slider relays (`PluginEditor.cpp:86-88, 170-172, 784-789`) and a
`tuning_temperamentPreset` combo relay (`:93, 173, 797`). No JS calls `getSliderState('tuning_*')`
/ `getComboBoxState('tuning_temperamentPreset')`; the tuning UI instead uses a raw
`<input type=range>` + native `setOctaveStretch` (`tuning-panel.js:919`) and a drag knob +
`setMasterTune` (`:964`).

Two independent states exist for master-tune / octave-stretch: the automatable/persisted APVTS
param and the `TuningEngine` value set via native fns. Unless the processor bridges them both
directions, DAW automation of `tuning_masterTune` won't move the A4 knob and the knob won't
write the APVTS param → no automation/state recall. `tuning_pitchBendRange` and
`tuning_temperamentPreset` have **no UI at all**.

**Action:** confirm in the processor whether APVTS `tuning_*` ↔ `TuningEngine` are synced both
directions; if not, bind the tuning UI to the APVTS states like every other control (or drop the
unused relays). *(Flagged for verification — the bridge may exist in the processor's parameter
listener; confirm before fixing.)*

### WR-09: Soft limiter runs *before* the effects chain; post-limiter EQ (+12 dB shelves) can clip the output

**File:** `Source/PluginProcessor.cpp:842-861` (tanh soft limiter) vs `887-952` (FX chain)

The soft limiter (ceiling ~1.0) runs before Chorus/Delay/Reverb and the EQ, whose high/low
shelves boost up to **+12 dB** (ranges ±12, layout :491-501) *after* the limiter. A dense chord
limited to ~0.9 then boosted +12 dB (~4×) peaks ~3.5 → hard digital clipping at the output.

**Fix:** move the soft limiter to the end of the chain (after EQ), or add a second ceiling there.

### WR-10: `DelayProcessor` fixed 192000-sample max delay overflows above 96 kHz

**Files:** `Source/DSP/DelayProcessor.h:29-30` (`{ 192000 }`) + `.cpp:43-46, 87-88`

`delayL/delayR` are constructed with a fixed 192000-sample max and `prepare()` never calls
`setMaximumDelayInSamples`. `setTime` computes `delaySamples = seconds * sampleRate` (param max
2.0 s, layout :476) and `popSample` is unclamped. At 176.4/192 kHz, 2.0 s = 352800/384000
samples > 192000 → `popSample` asserts in debug / wraps to a wrong (aliased) delay in release.
Fine at 44.1/48/96 kHz (192000 = 2.0 s @ 96 kHz).

**Fix:** in `prepare()` call `delayL.setMaximumDelayInSamples((int)(2.0*sampleRate)+4)` (and R),
and/or clamp `delaySamples` to the current max.

### WR-11: `getTailLengthSeconds()` returns 0 for a multi-second-decay synth + reverb tail

**File:** `Source/PluginProcessor.h:44` (`return 0.0;`)

A bell synth with multi-second decays plus an FDN reverb tail reports zero tail. On offline
bounce/freeze, some hosts stop pulling audio at note-off and truncate the bell + reverb tail.

**Fix:** report a representative tail (e.g. `return 15.0;`, or derive from reverb size / max decay).

### WR-12: `ScaleGenerator::generateRank2` clamps the generator against the *un-clamped* period

**File:** `Source/ScaleGenerator.cpp:59-60`

Line 59 clamps `generatorCents` against `periodCents - 1.0`, but `periodCents` is only clamped
to `[100, 2400]` on line 60 (after). A caller passing `periodCents < 101` (the `generateRank2`
native fn forwards raw JS args, `PluginEditor.cpp:557`) pins the generator to a nonsensical
bound before the period is corrected → wrong scale, not a crash.

**Fix:** swap the two lines — clamp `periodCents` first, then clamp `generatorCents` against the
corrected period.

---

## Info

### IN-01: Migrate all WebView knob readouts to `SliderState.getScaledValue()`

Root cause of WR-04..WR-07. All non-trivial readouts re-derive engineering units from hardcoded
JS constants applied to `getNormalisedValue()` instead of `getScaledValue()` (present at
`juce/index.js:226`, already reflects the C++ `NormalisableRange` incl. skew). `lpFilterCutoff`
(index.html:1692, skew 0.3), `partialTuning` (:1676), `delayTime` (:2568) *happen* to match today
only because the constants were hand-copied and will silently drift if any C++ range changes —
the exact `pattern_webview_knob_readout_scaled_value` drift class (O-MicrotonalSampler read 2×
wrong for ~20 versions). Recommend a blanket migration to `getScaledValue()`.

### IN-02: No double-click-reset / no `getParameterDefaults` native fn

Main sliders have no reset handler; FX knobs implement double-click-to-*edit* (index.html:2410),
not reset. The `properties` object has no default field, so adding reset-to-default later will
need a `getParameterDefaults` native fn (none exists).

### IN-03: Dead C++ native functions; `getMasterTune` being unused leaves the A4 knob stuck at 440 Hz

Registered but never called by JS: `getPresetList` (:199), `loadPreset` (:230), `savePreset`
(:254), `setTuningIntervals` (:342), `setSingleIntervalEncoded` (:372), `getMasterTune` (:414),
`setTemperamentPreset` (:429), `getTemperamentPreset` (:440). Consequence: the A4-REF knob is
hardcoded to 440 Hz (`tuning-panel.js:935, 128`) and never initializes from the backend, so it
mis-displays after a preset/state recall that changed master tune.

### IN-04: `instrument-footer-panel.js` is dead code with a latent bind bug

Never `import`ed by index.html (the footer keyboard is built inline at index.html:2044-2149) and
not served by `getResource` (no `/modules/…` mapping, PluginEditor.cpp:915-972). Its
`_bindNativeFunction` (:129-135) is also latently broken (`window.Juce` is never defined; the
`window.__JUCE__.backend.sendMidiNote` fallback is not a valid native-fn path) — moot while
unused; delete the file or wire it up.

### IN-05: `tuning-panel.js` docstring documents `window.__JUCE__` (misleading)

Line 18 documents construction with `window.__JUCE__`, but the actual call correctly passes the
ES-module namespace `Juce` (index.html:2613), which is what exposes `getNativeFunction`. The code
is right; the docstring perpetuates the known `critical_juce_webview_namespace_vs_postmessage`
confusion. Fix the docstring when next touched.

### IN-06: Per-note (not per-channel) pitch-bend storage → MPE same-note collision

`TuningEngine::getFrequency` ignores `midiChannel` (`TuningEngine.cpp:708`) and bends are stored
in a single `[128]` array keyed by note number (`:722`, `.h:317`). Two simultaneous notes of the
same MIDI number on different MPE channels share one bend slot. Acceptable for the Dorico
per-note-expression use case; a real limit if true MPE is ever expected.

### IN-07: `TuningExporter::calculateETDeviation` divides by `totalDegrees` with no guard

`TuningExporter.cpp:118`. Currently only reached with `totalDegrees ≥ 2`, so safe in practice,
but the guard gap is latent. Add `if (totalDegrees <= 0) return 0.0;`.

### IN-08: `loadScalaFile` trusts the degree-count line; `<=0` truncates the scale

`TuningEngine.cpp:459` (`expectedDegrees = getIntValue()`) + break at `:471`. A malformed `.scl`
with a 0/negative count reads one pitch then stops; the `size() < 2` check (:477) may still pass
with a degenerate scale. Validate `expectedDegrees > 0` and warn on `count != expectedDegrees`.
(Note: `parseScalaPitch` treating bare integer `"2"` as ratio 2/1 at :411 is **correct** per the
Scala spec.)

### IN-09: KBM degree clamp is `scaleSize`-inclusive (indexes the period as a degree)

`TuningEngine.cpp:825` `jlimit(0, scaleSize, scaleDegree)` then `activeIntervals[scaleDegree]`
(:827); same at :847-848. No OOB (the period sits at index `scaleSize`), but semantically the
period can be selected as a scale degree at the top boundary. `scaleSize-1` is more correct.
Cosmetic.

### IN-10: `BellVoice` uses the shared non-thread-safe RNG on the audio thread

`BellVoice.cpp:1164, 1261` use `rand()/RAND_MAX`; `BellVoice.h:287-295` `gaussianApprox` →
`juce::Random::getSystemRandom()` (documented non-thread-safe), all reached from `startNote`.
`startNote` is serialized by `juce::Synthesiser`'s internal `CriticalSection`, so voices don't
race each other, but the shared RNG is still touched from the audio thread. Low risk; a per-voice
`juce::Random` member (seeded once) removes the coupling and the `rand()` concern.

### IN-11: Air-absorption coefficient recomputed per-sample per-voice (perf)

`BellVoice.cpp:638`: when `airAbsorption > 0`, `1 - std::exp(...)` is recomputed per sample per
voice (cutoff is time-varying, so it genuinely changes). Correct and NaN-safe (cutoff
`jlimit(400,18000)` at :635), but with 16 voices it is a lot of transcendental math — consider
every-N-samples or a cheaper approximation.

### IN-12: Dead members in `ReverbProcessor`

`ReverbProcessor.h:143-144, 123`: `prevSize`, `prevDamping`, `tankState[]` are declared/initialized
but never read (damping applies unconditionally; delay dirtiness uses `prevSizeForDelays`; mix
uses `prevMix`). Harmless; remove for clarity.

### IN-13: Uninitialized decay-coefficient arrays in `BellVoice` (defensive)

`BellVoice.h:246-248`: `strikeDecayCoeffs/bodyDecayCoeffs/humDecayCoeffs[NUM_PARTIALS]` have no
in-class initializer. They are always written by `calculateMultiStageCoefficients()` in
`startNote` before any read (guarded by `noteActive`), so safe in practice; add `{}` initializers
to be defensive.

---

## Suggested Resolution Order

1. **CR-01** — factory preset library is non-functional; highest user-visible impact.
2. **CR-03** — FileChooser UAF; a real crash on a common host interaction.
3. **CR-02** — EQ audio-thread allocation; dropout risk during automation.
4. **WR-03 / WR-04..WR-07** — dead `material` control + readout drift (fix WR-04..07 together via
   IN-01's `getScaledValue()` migration).
5. **WR-01 / WR-02** — preset reset-to-defaults + name sanitization (both well-trodden module
   fixes; candidates to fold back into the shared preset-manager convention).
6. **WR-08** — verify the tuning APVTS↔engine bridge before acting.
7. Remaining WR/IN as capacity allows.

> Resolve via `/improve-review O-Bells` (reads this file's CR/WR/IN IDs).
