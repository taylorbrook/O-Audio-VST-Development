---
phase: O-Wind-v1.16.0
reviewed: 2026-07-09
depth: deep
files_reviewed: 25
files_reviewed_list:
  - plugins/O-Wind/Source/PluginProcessor.cpp
  - plugins/O-Wind/Source/PluginProcessor.h
  - plugins/O-Wind/Source/PluginEditor.cpp
  - plugins/O-Wind/Source/PluginEditor.h
  - plugins/O-Wind/Source/FluteSynthVoice.cpp
  - plugins/O-Wind/Source/FluteSynthVoice.h
  - plugins/O-Wind/Source/FluteSynthSound.h
  - plugins/O-Wind/Source/DSP/BoreWaveguide.h
  - plugins/O-Wind/Source/DSP/JetExciter.h
  - plugins/O-Wind/Source/DSP/JetNonlinearity.h
  - plugins/O-Wind/Source/DSP/ToneHoleSystem.h
  - plugins/O-Wind/Source/DSP/SubHarmonics.h
  - plugins/O-Wind/Source/DSP/DCBlocker.h
  - plugins/O-Wind/Source/DSP/StereoWidth.h
  - plugins/O-Wind/Source/DSP/EQProcessor.cpp
  - plugins/O-Wind/Source/DSP/EQProcessor.h
  - plugins/O-Wind/Source/DSP/DelayProcessor.cpp
  - plugins/O-Wind/Source/DSP/DelayProcessor.h
  - plugins/O-Wind/Source/DSP/ReverbProcessor.cpp
  - plugins/O-Wind/Source/DSP/ReverbProcessor.h
  - plugins/O-Wind/Source/DSP/InstrumentPresets.h
  - plugins/O-Wind/Resources/ui/index.html
  - plugins/O-Wind/Resources/ui/js/juce/index.js
  - plugins/O-Wind/CMakeLists.txt
  - plugins/O-Wind/CHANGELOG.md
shared_modules_reviewed:
  - modules/tuning/scala-tuning-engine/js/tuning-panel.js (served as /js/tuning-panel.js)
  - modules/persistence/preset-manager/cpp/OuariconPresetManager.h
findings:
  critical: 8
  warning: 13
  info: 19
  total: 40
status: issues_found
---

# O-Wind v1.16.0: Code Review Report

**Reviewed:** 2026-07-09
**Depth:** deep (parallel three-subsystem review: DSP/RT-safety · WebView bridge/editor · params/presets/state/build)
**Files Reviewed:** 25 (+2 shared modules)
**Status:** issues_found

## Summary

O-Wind is a 2×-oversampled waveguide physical-model flute synth (jet exciter → bore
waveguide → tone color chain), with a v1.14.0 effects chain (Chorus/Delay/EQ/FDN Reverb),
Scala/KBM tuning via the shared scala-tuning-engine, VST3 Note Expression for Dorico
microtonal playback, and a WebView UI.

Several of this codebase's recurring failure modes are handled **correctly** in O-Wind:

- **Factory preset values** — full param-by-param audit is clean: every value is authored in
  engineering units and stored through `convertTo0to1` (`PluginProcessor.cpp:691-695`); no
  skewed param appears in any factory preset. Neither O-Bells CR-01 nor
  `pattern_factory_preset_normalized_ignores_skew` applies. ✓
- **Preset apply resets all params to defaults first** (OuariconPresetManager.h:295-300). ✓
- **Windows WebView2:** both `NEEDS_WEBVIEW2 TRUE` (CMakeLists.txt:18) and
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (CMakeLists.txt:108) present; user-data
  folder set (PluginEditor.cpp:74-78). Not one of the 34/35 audit offenders. ✓
- **Resource provider bare-path handling**, **`Juce` ES-module namespace** (not
  `window.__JUCE__`) passed to TuningPanel, **relay→webView→attachment order** with explicit
  reverse teardown, **single BinaryData target**, **no param-ID/class shadowing**. ✓
- **Oversampled-path filter corners:** all voice filters are designed AND run at
  `internalSampleRate` (2× host); post-voice FX designed and run at native rate — no
  corner-shift bug. `setLatencySamples` from the oversampler latency. ✓
- **Note Expression per-voice tuning:** atomic `exchange` consumption at startNote, no
  cross-voice bleed, lock-free per-block tuning re-query — matches the O-Lyrica validated
  pattern. Embedded-tuning `period` pushed before `setCustomIntervals`. ✓
- **Embedded-tuning period preserved** (`PluginEditor.cpp:450-452`). ✓

The blockers cluster in two places. **The WebView bridge was never finished for two whole
releases:** the entire v1.14.0 Effects tab (21 params) and four v1.12.0 Sound-tab knobs
(Growl, Formant, Drift Depth/Speed) have no relays or attachments — the controls render but
are dead, and since all FX mix params default to 0 the effects chain is unreachable from the
UI (CR-01/CR-02). **The RT/state layer has four independent criticals:** ~80 unconditional
heap alloc/free pairs per audio block in the voice filter-update path (CR-03); a stuck-drone
voice when ADSR is disabled mid-release (CR-04); tuning-engine state (Scala/KBM/tonic/master
tune) silently lost on session reload because `setCustomStateCallbacks` was never registered
(CR-05); and the CMake `PLUGIN_VERSION` keyword bug just fixed in O-IntonationPad, which makes
the bundle report 1.0.0 and permanently neutralizes the factory-preset version sentinel
(CR-06). Plus the fleet-wide `launchAsync` UAF in all four FileChooser completions (CR-07)
and three dead tuning-panel buttons calling unregistered native fns (CR-08).

---

## Critical Issues

### CR-01: Entire Effects tab is dead — no relays or attachments exist for any of the 21 effects parameters

**Files:** `Source/PluginEditor.cpp:26-119, 492-557` + `Source/PluginEditor.h:34-107` vs
`Resources/ui/index.html:1794-1850`

The Effects tab JS binds 16 slider states, 4 toggle states, and 1 combobox state:

```js
const chorusRateState   = Juce.getSliderState('chorusRate');
...
const delayModeState    = Juce.getComboBoxState('delayMode');
const reverbBypassState = Juce.getToggleState('reverbBypass');
```

PluginEditor.cpp creates relays for exactly 26 named parameters — **zero** `chorus*`,
`delay*`, `eq*`, or `reverb*` relay, `withOptionsFrom`, or attachment exists anywhere in
`Source/` (verified by grep). The C++ parameters DO exist (`PluginProcessor.cpp:330-397`).
Git history proves the wiring was never written: the v1.14.0 commit (`5ba4e48`, "add effects
panel") touched PluginProcessor.{h,cpp}, index.html, DSP files, and CMakeLists — **not**
PluginEditor.cpp/h; `git log -S "chorusRateRelay"` finds no commit ever.

Per the JUCE frontend (`js/juce/index.js:135-167`), `getSliderState` on an unregistered name
creates an orphan state, logs an invisible console warning, and emits events no C++ relay
hears. `setupFxKnob` (index.html:1645-1654) only updates knob visuals inside
`valueChangedEvent` listeners, which only fire on a backend echo — so the FX knobs **do not
move, do not display real values, and never touch the APVTS**.

**Impact:** every effects knob, all four bypass toggles, and the delay-mode dropdown are dead.
All mix params default to 0.0 and `processBlock` gates on `mix > 0.001`
(`PluginProcessor.cpp:609-647`), so the entire effects chain is unreachable from the UI —
it works only via host generic-parameter automation. Dead since v1.14.0; build, auval, and
render harness all pass (this is `pattern_webview_native_fn_bridge_gap` at parameter-relay
scale).

**Fix:** in PluginEditor add, in relay→webView→attachment order: 16 `WebSliderRelay` +
`WebSliderParameterAttachment` (chorusRate/Depth/Mix, delayTime/Feedback/Mix,
eqLowGain/MidGain/MidFreq/HighGain, reverbSize/Damp/Predelay/Mix/Mod/Shimmer), 4
`WebToggleButtonRelay` + `WebToggleButtonParameterAttachment` (chorus/delay/eq/reverbBypass),
1 `WebComboBoxRelay` + `WebComboBoxParameterAttachment` (delayMode); add each to
`withOptionsFrom(...)`, following the existing pattern at PluginEditor.cpp:26-66/87-119/
492-557. Regression-check with a `ui_frontend_check.js`-style grep-diff: every
`getSliderState/getToggleState/getComboBoxState` name in index.html must have a matching
relay.

---

### CR-02: Four Sound-tab knobs are dead — Growl, Formant, Drift Depth, Drift Speed have no relays

**Files:** `Source/PluginEditor.cpp:26-66` vs `Resources/ui/index.html:978-1002, 1017-1021,
1126-1132`

`PARAMS` includes `vibratoDriftDepth`, `vibratoDriftSpeed`, `growl`, `formant`, and
`DOMContentLoaded` calls `bindSliderParam()` for every PARAMS key (index.html:1863-1865).
The C++ parameters exist (`growl` PluginProcessor.cpp:118-121, `vibratoDriftDepth` :162-165,
`vibratoDriftSpeed` :170-173, `formant` :197-200) but the editor registers no relay and no
attachment for any of them. The v1.12.0 changelog's "Files Modified" list omits
PluginEditor.cpp — the drift knobs (that release's headline feature) were born dead.

**Impact:** the knobs render at minimum, ignore drag, never reflect presets. Worse: `formant`
is stuck at its 0.5 default, so the +3 dB headjoint formant peak
(`PluginProcessor.cpp:562`, `gainDb = formantVal * 6.0f`) is **permanently applied and
cannot be turned off from the UI**.

**Fix:** add `WebSliderRelay` + `withOptionsFrom` + `WebSliderParameterAttachment` for the
four params, same pattern as the other 18 sliders.

---

### CR-03: Unconditional per-block heap allocation on the audio thread in the voice filter-update path (~80 malloc/free pairs per block)

**Files:** `Source/DSP/BoreWaveguide.h:130-133, 146-147, 154-155, 111-114`;
`Source/DSP/JetExciter.h:216-217`; call sites `Source/FluteSynthVoice.cpp:659-670` — every
block, unconditionally, for every voice.

```cpp
*boreLossLow.coefficients = juce::dsp::IIR::Coefficients<float> (
    juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass (sampleRate, lowCut));
```

The code uses `ArrayCoefficients` (the O-Formant v1.25.1 pattern) but then **wraps the stack
array in a temporary `IIR::Coefficients<float>`**, which defeats the point. Verified against
JUCE 8.0.9 (`juce_IIRFilter.h:157`, `juce_IIRFilter_Impl.h:41-55`): the
`Coefficients(const std::array&)` constructor heap-allocates a fresh `juce::Array`, and the
subsequent copy-assign allocates again. 5 temporaries per voice per block ≈ 10 alloc/free
pairs × 8 always-running voices (see WR-06) = **~80 RT-thread malloc/free pairs per block,
steady-state, even when silent** (~27,500/s at 44.1 kHz / 128 samples). This is the
O-Bells CR-02 / O-Formant WR-08 bug class in a strictly worse, unconditional form.

**Impact:** allocator lock contention / priority inversion with any message-thread
allocation → nondeterministic audible dropouts, worst on small buffers.

**Fix:** assign the array directly — `Coefficients::operator=(const std::array&)`
(juce_IIRFilter.h:161) assigns in place, allocation-free after first use:

```cpp
*boreLossLow.coefficients =
    juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass (sampleRate, lowCut);
```

Apply to all five sites (both boreLoss filters, end-reflection shelf, radiation HP, Strouhal
bandpass) and to `setInharmonicity` (BoreWaveguide.h:111, gated but same pattern). Optional:
epsilon dirty-checks to skip redesign when cutoffs haven't moved — but the in-place assign is
the load-bearing fix. (ToneHoleSystem.h:127-134 has the same pattern but is prepare-time-only
dead code — fix if kept.)

---

### CR-04: Stuck note at full level — disabling ADSR while a note is releasing orphans `pendingJetRelease`

**File:** `Source/FluteSynthVoice.cpp:183-192` (stopNote), `:302-304` (cleanup gate),
`:451, 476-488` (state machine gated on `adsrEnabled`), `:703, 710-711` (per-block re-read).

`stopNote(allowTailOff)` sets `pendingJetRelease = true` and `adsrStage = Release`, deferring
`jetExciter.stopNote()` to the end of the ADSR release. But the state machine only runs
`if (adsrEnabled)`, and `updateParametersFromAPVTS` re-reads `adsrEnabled` every block with
no handling of an in-flight release:

```cpp
if (! adsrEnabled)
    adsrLevel = 1.0f;   // no handling of pendingJetRelease / adsrStage
```

**Failure scenario:** note-off received (stage = Release), user toggles **ADSR Enabled off**
while the tail sounds. The Release stage never advances, `pendingJetRelease` stays true,
`jetExciter.stopNote()` is never called, and the cleanup gate (line 303) requires
`jetExciter.isReleasing()` — false. `adsrLevel` snaps from mid-release to 1.0 (audible jump)
and the voice **drones indefinitely at full level** until voice-stolen. Every
currently-releasing voice becomes a stuck drone simultaneously.

**Fix:** in `updateParametersFromAPVTS()`, resolve any deferred release when ADSR turns off:

```cpp
if (! adsrEnabled)
{
    if (pendingJetRelease) { jetExciter.stopNote(); pendingJetRelease = false; }
    if (adsrStage == ADSRStage::Release) adsrStage = ADSRStage::Idle;
    adsrLevel = 1.0f;   // consider ramping to avoid the click
}
```

---

### CR-05: Tuning engine state is not persisted — Scala/KBM/embedded tunings, tonic, octave stretch, and master tune silently lost on session reload

**Files:** `Source/PluginProcessor.cpp:401-444` (constructor — no callback registration),
`:667-679` (state delegates to presetManager);
`modules/persistence/preset-manager/cpp/OuariconPresetManager.h:521-574`

`grep -rn "setCustomStateCallbacks" plugins/O-Wind/` → no matches. Sibling microtonal synths
on the identical module stack DO register them (O-Lyrica `PluginProcessor.cpp:569`, O-Bells
`PluginProcessor.cpp:563`), persisting intervals, scale name, tonic, preset index, octave
stretch, and tuning mode. O-Wind's editor exposes the full tuning surface
(`setTuningIntervals`, `loadScalaFile`, `loadKBMFile`, `setTonicNote`, `setOctaveStretch`,
`setMasterTune`, `setTemperamentPreset`, `loadEmbeddedTuning` — PluginEditor.cpp:237-459),
all writing straight into `TuningEngine`, none APVTS-backed.

**Failure scenario:** user loads a .scl (or embedded tuning), saves the DAW session. On
reload, APVTS restores `tuningSystem=Scala` but the engine has default intervals — every note
plays 12-TET with no error. User presets have the same hole (`createPresetJson` only writes
APVTS params). This directly undermines the plugin's microtonal/Dorico positioning.

**Fix:** register the same save/load lambda pair as O-Lyrica
(`presetManager.setCustomStateCallbacks(...)` capturing `tuningEngine`), persisting
intervals, scale name, tonic, built-in preset index, octave stretch, and mode.

---

### CR-06: `juce_add_plugin` uses non-existent `PLUGIN_VERSION` keyword — bundle reports 1.0.0 and the factory-preset version sentinel is permanently neutralized

**File:** `CMakeLists.txt:12` — `PLUGIN_VERSION "1.16.0"`

JUCE's `juce_add_plugin` has no `PLUGIN_VERSION` argument; unknown keywords are silently
dropped and the version falls back to root `project(JUCEPlugins VERSION 1.0.0)`. This is the
exact defect fixed yesterday in O-IntonationPad (commit `e87ea36`). Knock-on effects:

1. auval/DAWs see Component Version 1.0.0 (claimed 1.16.0).
2. `JucePlugin_VersionString == "1.0.0"`, so the factory-preset sentinel
   (OuariconPresetManager.h:587-590) is written once as "1.0.0" and matches forever —
   **factory preset JSON is never regenerated on any future version bump** (e.g. the v1.12.0
   factory drift-value updates never reach an older install).
3. Preset JSON `"version"` metadata stamps "1.0.0".

**Fix:** change line 12 to `VERSION "1.16.0"` (matching the O-IntonationPad fix). Rebuild,
sweep both bundle variants, re-auval; delete/bump the factory sentinel once so factory
presets regenerate with the correct stamp.

---

### CR-07: All four FileChooser `launchAsync` completions capture raw `this` and call `complete()` on every path — use-after-free on editor teardown

**File:** `Source/PluginEditor.cpp:181-195` (savePresetWithDialog), `:337-347`
(loadScalaFile), `:355-365` (loadKBMFile), `:468-480` (exportTuningHTML)

```cpp
fileChooser->launchAsync(...,
    [this, complete](const juce::FileChooser& fc) {
        auto result = fc.getResult();
        if (result == juce::File{}) { complete(juce::var("")); return; }  // complete() on cancel
        auto& pm = processorRef.getPresetManager();                        // raw `this`
        ...
```

All four callbacks (a) dereference raw `this` (`processorRef`) and (b) invoke `complete(...)`
on every path, including cancel. If the host destroys the editor while the native dialog is
up, the completion fires after `~OWindAudioProcessorEditor`: dereferencing `this` is a UAF,
and — per `pattern_webview_launchasync_safepointer_no_complete` (O-MicrotonalSampler v1.23.5
W12, standing fleet-audit item) — calling `complete()` is *itself* a UAF because the callback
is owned by the destroyed WebView Impl. Host crash on a routine cancel.

**Fix:** in each callback capture
`juce::Component::SafePointer<OWindAudioProcessorEditor> safeThis(this)`; first line
`if (safeThis == nullptr) return;` (bare return — do **not** call `complete` on the dead
path); use `safeThis->processorRef` throughout.

---

### CR-08: Tuning panel Rank-2 generator and Save SCL / Save KBM buttons are dead — native functions never registered

**Files:** `modules/tuning/scala-tuning-engine/js/tuning-panel.js:819, 862, 871` (served to
O-Wind as `/js/tuning-panel.js`) vs `Source/PluginEditor.cpp:125-481`

The tuning panel calls three native functions PluginEditor.cpp never registers:

```js
intervalsJson = await this.juce.getNativeFunction('generateRank2')(generator, period, count);
await this.juce.getNativeFunction('saveScalaFile')();
await this.juce.getNativeFunction('saveKBMFile')();
```

Registered fns cover `generateEDO`/`generateHarmonicSeries` but not `generateRank2`, and
`loadScalaFile`/`loadKBMFile` but not the save counterparts. The panel exposes the "Rank-2
Temperament" generator option (tuning-panel.js:170) and wires `#btn-save-kbm` etc. (:211).
Selecting Rank-2 + Generate, or clicking Save SCL/KBM: the try/catch (tuning-panel.js:825-830)
swallows the failure — the button silently does nothing (`pattern_webview_native_fn_bridge_gap`).

**Fix:** register `generateRank2` (delegating to the scale generator), `saveScalaFile`, and
`saveKBMFile` (FileChooser saveMode + engine serialization) with the CR-07 SafePointer
pattern — or hide those controls in the panel if intentionally unsupported.

---

## Warnings

### WR-01: Double-click-to-reset ignores NormalisableRange skew — ADSR knobs reset to ~1 ms instead of 10/100/200 ms defaults

**File:** `Resources/ui/index.html:1315-1324`

```js
const defaultNorm = (def.default - def.min) / (def.max - def.min);  // linear — ignores skew
state.setNormalisedValue(defaultNorm);
```

`adsrAttack/adsrDecay/adsrRelease` have skew 0.3 (PluginProcessor.cpp:262/271/288).
`setNormalisedValue` feeds the skewed `normalisedToScaledValue` (juce/index.js:248-254): for
adsrAttack (default 0.01 s), linear norm ≈ 0.0018 → scaled ≈ 0.001 s. Attack resets to ~1 ms
instead of 10 ms, Decay ~1 ms instead of 100 ms, Release ~1 ms instead of 200 ms — the wrong
value is *applied*, not just displayed; the envelope audibly collapses on a routine gesture.

**Fix:** apply the skew — `if (def.skew && def.skew !== 1) norm = Math.pow(norm, def.skew);`
— or better (WR-09), drop the JS map and fetch defaults via a `getParameterDefaults` native
fn as in O-MicrotonalSampler v1.23.7.

### WR-02: Bore/jet delay-line capacity fixed at 2048/1024 samples regardless of rate — low notes mistuned at ≥96 kHz hosts

**Files:** `Source/DSP/BoreWaveguide.h:36, 52-54`; `Source/FluteSynthVoice.h:74` +
`FluteSynthVoice.cpp:267`

The voice runs at `internalSampleRate = 2 × host rate` (FluteSynthVoice.cpp:248); required
delay = `internalSampleRate / f`. JUCE 8.0.9 `DelayLine::setDelay` silently clamps
out-of-range delays (juce_DelayLine.cpp:57-66, plus a per-sample debug jassert). Lowest
renderable fundamental: ~18 Hz at 44.1 kHz (fine), **~39 Hz at 96 kHz (notes below ~E1 play
sharp at a fixed wrong pitch)**, **~78 Hz at 192 kHz (everything below ~E2 clamps)** — and
`appliesToNote` accepts all 128 notes. The jet delay (1024) clamps in the same region,
shifting the bore/jet split too.

**Fix:** size the lines from the prepared rate in `prepare()`, e.g.
`boreFwd.setMaximumDelayInSamples ((int) std::ceil (internalRate / 8.18 / 2.0) + 8)` (MIDI 0
fundamental) and equivalently for boreBwd/jetDelay — or clamp and document the playable range.

### WR-03: EQProcessor updates coefficients with heap-allocating `Coefficients::makeXXX` on the audio thread

**File:** `Source/DSP/EQProcessor.cpp:52-75`

```cpp
*lowShelf.state = *FilterCoeffs::makeLowShelf (currentSampleRate, 200.0f, 0.707f, ...);
```

`makeLowShelf/makePeakFilter/makeHighShelf` each `new` a ref-counted Coefficients on the
audio thread; dirty-check gating means it fires **every block while an EQ knob is dragged or
automated**. Exactly `pattern_arraycoefficients_rt_safe_iir` (O-Formant WR-08 / O-Bells
CR-02).

**Fix:** `*lowShelf.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf (...);`
(in-place assign, allocation-free after first use). Same for mid peak and high shelf.

### WR-04: Formant filter update in processBlock uses `Coefficients::makePeakFilter` — heap alloc on the audio thread

**File:** `Source/PluginProcessor.cpp:571-575`

```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
    getSampleRate(), centerHz, 1.5f, juce::Decibels::decibelsToGain (gainDb));
*formantFilterL.coefficients = *coeffs;
*formantFilterR.coefficients = *coeffs;
```

Fires every block while `formant`/`instrumentPreset` moves (gate is |Δ| > 0.01 dB on a 0–6 dB
range → any drag/automation), plus two Array copy-assign allocations for L/R. Same class as
WR-03.

**Fix:** `auto arr = juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter (...);
*formantFilterL.coefficients = arr; *formantFilterR.coefficients = arr;`

### WR-05: `airColumn` parameter is completely dead — Q computed then explicitly discarded

**Files:** `Source/FluteSynthVoice.cpp:658-659`; `Source/DSP/BoreWaveguide.h:129`

```cpp
float lossQ = 0.707f * (1.0f - airColumn * 0.3f);
boreWaveguide.updateBoreLossFilter (lossCutoff, lossQ);
// BoreWaveguide.h:129:  juce::ignoreUnused (q);
```

The bore loss filters became two cascaded first-order lowpasses (no Q); the `q` argument is
ignored — but the user-facing "Air Column" knob (PluginProcessor.cpp:57-62), its relay
(PluginEditor.cpp:33), and all 8 factory presets (which deliberately vary it 0.3–0.7) remain.
Turning the knob does nothing; the Shakuhachi/Piccolo/Ocarina presets' distinct values are
inert. `InstrumentPreset::boreLossQ` (InstrumentPresets.h:28) is likewise never read.

**Fix:** wire `airColumn` into the DSP (e.g. scale the low/high cutoff ratio or loss-blend
amount) or remove the param + UI + preset entries. Don't ship a knob that does nothing.

### WR-06: No inactive-voice early-out — all 8 voices run the full 2× oversampled physical model at all times

**File:** `Source/FluteSynthVoice.cpp:289-339` (no `isVoiceActive()` check)

`juce::Synthesiser::renderVoices` calls `renderNextBlock` on every voice each block. After a
voice clears, the next block falls through the cleanup gate (line 303 requires
`isReleasing()`) and executes the entire per-sample loop — oversampler, jet exciter, Lagrange
delay, tanh, two Thiran delays, 4+ IIR filters — producing exact zeros. Idle cost = 8 × full
flute model at 2× rate, plus 8 × CR-03's allocations, from instantiation onward, forever.
Filed as Warning (not Info) because the wasted path is also where CR-03's RT allocations occur
— this is an RT-headroom defect.

**Fix:** at the top of `renderNextBlock` (after the numSamples guard):
`if (! isVoiceActive()) return;` — state is already reset on clear, so skipping is safe.

### WR-07: DelayProcessor time can exceed delay-line capacity at high sample rates

**Files:** `Source/DSP/DelayProcessor.h:29-30` (lines fixed at 192000 samples),
`DelayProcessor.cpp:43-46` (unclamped `delaySamples = seconds * currentSampleRate`), `:87-88`;
param range 0.001–2.0 s (`PluginProcessor.cpp:346-348`)

At 176.4 kHz, 2.0 s → 352,800 samples > 192,000 max; at 192 kHz any time > 1.0 s exceeds it.
JUCE clamps silently (debug jassert per sample), so the delay caps at ~1.09 s / 1.0 s,
diverging from the displayed value.

**Fix:** in `prepare()`, `setMaximumDelayInSamples ((int) std::ceil (2.0 * spec.sampleRate) + 4)`
for both lines, or clamp in `setTime` against `getMaximumDelayInSamples()`.

### WR-08: Delay time and reverb size changes applied instantaneously — clicks under automation

**Files:** `Source/DSP/DelayProcessor.cpp:43-46, 87-88`; `Source/DSP/ReverbProcessor.cpp:309-315`

`delaySamples` snaps to the new APVTS value once per block; dragging Delay Time (0.001–2.0 s
continuous) or Reverb Size makes read heads jump arbitrary distances every block → broadband
clicks/zipper (the FDN's Householder feedback recirculates each discontinuity).

**Fix:** smooth `delaySamples` per sample (`juce::SmoothedValue` or one-pole, ~20–50 ms),
matching the voice's existing `embouchureSmoothed`/`totalDelaySmoothed` discipline; smooth
`scaledDelays` toward target for reverb size.

### WR-09: All knob readouts derive values from a hardcoded JS min/max/skew map instead of `SliderState.getScaledValue()`

**File:** `Resources/ui/index.html:1114-1141` (PARAMS), `:1243-1250` (normToRaw), `:1276,
1282` (readouts); `:1645-1651, 1817-1832` (effects `displayMin/displayMax` literals)

Every PARAMS entry and every `setupFxKnob` display range was diffed against
`createParameterLayout` — **they all match today** (incl. jet/endReflection −1..1,
outputLevel −60..12, adsr skew 0.3, delayTime 0.001–2.0 s ↔ 1–2000 ms). But this is exactly
`pattern_webview_knob_readout_scaled_value` (O-MicrotonalSampler read 2× wrong for ~20
versions): the C++ range is already pushed via `propertiesChanged` and `getScaledValue()`
returns the true engineering value — the duplicate JS table silently diverges the moment any
C++ range is retuned.

**Fix:** replace `normToRaw(paramName, state.getNormalisedValue())` with
`state.getScaledValue()`; read min/max/skew from `state.properties`
(subscribe to `propertiesChangedEvent`); keep PARAMS only for decimals/units. Same for
`setupFxKnob` — drop `displayMin/displayMax`.

### WR-10: Reference-pitch knob always drags from 440 Hz and never syncs with the engine — `getMasterTune` registered but never called

**Files:** `modules/tuning/scala-tuning-engine/js/tuning-panel.js:906-947` (shipped in
O-Wind's UI); `Source/PluginEditor.cpp:304-306`

```js
let startValue = 440;                 // never reassigned
knob.addEventListener('mousedown', (e) => { isDragging = true; startY = e.clientY; ...
const newHz = Math.max(400, Math.min(480, startValue + delta));
```

(a) Drag to 452 Hz, release, drag again — the second drag computes from 440, snapping the
setting back. (b) Reopen the editor after setting master tune ≠ 440: the knob shows the
default, because `getMasterTune` is never invoked by the panel.

**Fix (shared module):** on `mousedown` set `startValue` to the current value; call
`getMasterTune` in `loadInitialState()` and drive `updateKnob(hz)` from it. Fix in
`modules/tuning/scala-tuning-engine` and re-sync plugin copies.

### WR-11: `setMasterTune` bypasses the `referencePitch` parameter — one-way divergence, snaps back on reload

**Files:** `Source/PluginEditor.cpp:308-315`; `Source/PluginProcessor.cpp:456-459`

The `setMasterTune` native fn writes `tuningEngine.setMasterTune(...)` directly; the
`referencePitch` APVTS param (400–480 Hz, default 440) drives the same engine value via
`parameterChanged`. Setting A4=432 from the panel leaves `referencePitch` at 440 → session
saves 440 → reload snaps the engine back to 440. Host automation of `referencePitch` stomps a
panel-set master tune at any time. (Related: `referencePitch` and `tuningSystem` have no
direct UI knob — the panel is the only surface, and it bypasses the param.)

**Fix:** route the native fn through the parameter:
`param->setValueNotifyingHost(param->convertTo0to1(hz))` — single source of truth; the
existing listener updates the engine. (CR-05's custom-state blob is not a substitute.)

### WR-12: `savePresetWithDialog` ignores the directory the user chose — preset silently saved elsewhere

**File:** `Source/PluginEditor.cpp:175-196`

A `saveMode` chooser lets the user navigate anywhere, but the completion extracts only
`result.getFileNameWithoutExtension()` and calls `pm.savePreset(name)` — which always writes
to the user-presets dir. Saving `~/Desktop/MyLead.json` produces no file on the Desktop and
no error. The module provides `savePresetToFile(file)` (OuariconPresetManager.h:353) exactly
for this.

**Fix:** if `result` is inside `getUserPresetsDirectory()`, `savePreset(name)`; otherwise
`savePresetToFile(result)`. (Or replace the dialog with an in-UI name prompt — it's only
being used as a name-entry box.)

### WR-13: Four automatable parameters have no UI control at all: `attackChiff`, `humanize`, `vibratoOnset`, `inharmonicity`

**Files:** `Source/PluginProcessor.cpp:83-96, 153-159, 216-221`; `Resources/ui/index.html`
(no PARAMS entry, no DOM knob, no relay)

All four are set by every factory preset (e.g. Shakuhachi humanize 0.5 vs Recorder 0.15), so
users hear presets change them but have no way to see or adjust them outside the host's
generic parameter list.

**Fix:** add knobs + relays (Expression/Sound tab), or document the omission as deliberate in
NOTES.md/CHANGELOG.

---

## Info

### IN-01: Voice reads ~35 parameters per block via string-keyed APVTS lookups (×8 voices)

`Source/FluteSynthVoice.cpp:541-561, 609-626, 639, 703-718, 731`; `PluginProcessor.cpp:553,
558-559`. Verified JUCE 8.0.9: the lookups don't allocate (StringRef-keyed `std::map`), but
they're ~280 O(log n) string-compare tree walks per block on the audio thread. The same
codebase already shows the right pattern (`fxCache` of cached `std::atomic<float>*`,
PluginProcessor.cpp:420-440). Cache the pointers once in the voice at prepare time.

### IN-02: `silentSampleCount` / `silentThreshold` are dead code

`FluteSynthVoice.cpp:503-506` increments/resets the counter; `FluteSynthVoice.h:124-126`
defines the threshold — nothing compares them. Remove, or wire as a cleanup backstop.

### IN-03: Voice cleanup and startNote never reset the per-voice oversampler

`FluteSynthVoice.cpp:314-324` resets jet/nonlinearity/DC/bore/jetDelay but not
`oversampling`; the polyphase half-band state survives into the next note (sub-audible onset
artifact). Add `oversampling.reset()` at the cleanup site.

### IN-04: Dead DSP scaffolding — ToneHoleSystem, SubHarmonics header, bore delay table; `toneHoleToggle` preset values are misleading

`DSP/ToneHoleSystem.h` never instantiated; `DSP/SubHarmonics.h` duplicated inline in
BoreWaveguide.h:187-192; `buildBoreDelayTable`/`getDelayForNote` (BoreWaveguide.h:264-282)
never called. `toneHoleToggle` is an explicit no-op (FluteSynthVoice.cpp:713-715) yet
Bansuri/Recorder/Piccolo presets set it to 1.0 (PluginProcessor.cpp:793, 861, 929). Also:
`InstrumentPreset` fields `noiseLevel`, `noiseCutoffBase`, `boreLossCutoff`, `boreLossQ`,
`embouchureMin/Max`, `defaultBreath`, `attackTimeMs` are never read by any DSP code. Remove
or clearly mark; don't preset a no-op.

### IN-05: CC overrides can never return control to the knob or reach zero

`FluteSynthVoice.cpp:564-566`: `if (ccBreathPressure > 0.0f) breathPressure = ccBreathPressure;`
— CC2=0 (breath fully off) falls back to the knob instead of silencing; same for CC74/CC1. A
breath controller can't end a phrase by dropping to zero. Use a per-controller "CC seen"
latch instead of the >0 test.

### IN-06: `juce::Random::getSystemRandom()` used on the audio thread in startNote

`FluteSynthVoice.cpp:115, 122, 124` — the shared global Random is not thread-safe (message
thread also uses it inside JUCE); consequence is only benign RNG-state corruption. The voice
already owns `voiceRng` (used at :142-151) — use it for the vibrato phases too.

### IN-07: FX `mix > 0.001` gating freezes effect state and cuts tails

`PluginProcessor.cpp:609-613, 630-631, 646-647`: when a mix knob hits 0, `process()` is
skipped — delay/reverb/chorus buffers freeze with content. Automating mix to 0 hard-cuts the
tail; raising it later replays stale audio. Keep processing for a tail period after 0, or
`reset()` on the 0-crossing. (Moot from the UI until CR-01 is fixed.)

### IN-08: `StereoWidthProcessor::reset()` disables width smoothing until the next prepare

`DSP/StereoWidth.h:36-39`: `widthSmoothed.reset (0)` sets steps-to-target to 0. Only
reachable via `releaseResources()`, so impact is nil in practice; use
`setCurrentAndTargetValue` instead.

### IN-09: Nine registered native functions are never called from any served JS

`Source/PluginEditor.cpp`: `savePreset` (:150), `getInstrumentPresets` (:202),
`getInstrumentPreset` (:209), `setInstrumentPreset` (:214), `setTuningIntervals` (:248),
`getMasterTune` (:304 — should become *used* per WR-10), `setTemperamentPreset` (:317),
`getTemperamentPreset` (:328), `getEmbeddedTuningCategories` (:434). Remove or wire up.

### IN-10: Instrument preset selector hardcodes `7` (index count) in three places

`Resources/ui/index.html:1416, 1422, 1429`: `Math.round(norm * 7)` duplicates
`InstrumentPresets::numTotalPresets = 8` and the 8 `<option>`s (:856-863). Adding a 9th
preset silently breaks the mapping. Derive from `state.properties.numSteps`.

### IN-11: `<script src="/js/juce/index.js">` loads an ES module as a classic script → guaranteed console SyntaxError

`Resources/ui/index.html:1105`: index.js ends with `export {...}` → `Unexpected token
'export'` from the classic tag. Harmless only because :1109 re-imports it as a module, but
the noise masks real errors. Remove line 1105.

### IN-12: Hand-built JSON in tuning native fns doesn't escape quotes/backslashes

`Source/PluginEditor.cpp:418-443` (`getEmbeddedTuningList`, `getEmbeddedTuningCategories`)
and the interval-array builders (:239-245, :373-379, :390-396) concatenate strings into JSON.
Current data is safe constants; a future name with `"` breaks the whole list. Use
`juce::JSON::toString` with `DynamicObject`/`Array<var>`.

### IN-13: Per-knob document-level mousemove/mouseup listeners (52 total)

`Resources/ui/index.html:1298-1312`: `bindSliderParam` adds a document mousemove + mouseup
per parameter (26 params); every mouse move runs 26 handlers. Not a leak; hoist a single
shared drag handler like the effects tab's `fxKnobDrag` (:1596-1613).

### IN-14: Effects wheel and dblclick-edit set values without drag gestures

`Resources/ui/index.html:1666-1671`: wheel handler calls `setNormalisedValue` with no
`sliderDragStarted/Ended` bracket — hosts gating automation recording on gestures miss wheel
edits. (Moot until CR-01.)

### IN-15: `exportTuningHTML` reports success even if the file write fails

`Source/PluginEditor.cpp:472-477`: `file.replaceWithText(html)` return ignored;
`complete(true)` regardless. Check the bool.

### IN-16: PLUGINS.md registry stale (1.15.1) vs CMakeLists/CHANGELOG (1.16.0); NOTES.md missing

`PLUGINS.md:56` says 1.15.1 / 2026-04-13; CHANGELOG and CMakeLists say 1.16.0 (2026-04-26).
`plugins/O-Wind/NOTES.md` doesn't exist despite the registry template. Update the row (and
note CR-06: the binary actually reports 1.0.0 until fixed).

### IN-17: Shared-module inconsistency — `initializeFactoryPresets` writes `preset.name + ".json"` unsanitized

`OuariconPresetManager.h:596` skips `sanitizePresetName` while load/save/delete all sanitize.
O-Wind's 8 factory names are safe; fix in the shared module:
`factoryDir.getChildFile(sanitizePresetName(preset.name) + ".json")`.

### IN-18: Dead APVTS listeners for `instrumentPreset` and `toneHoleToggle`

`PluginProcessor.cpp:416-417` registers listeners whose handlers are explicit no-ops
(:471-478). Remove the registrations.

### IN-19: Single shared `fileChooser` member — second dialog drops the first

`PluginEditor.h:113`: launching a second dialog while one is open replaces the `shared_ptr`;
the first completion may never fire, leaving its JS `await` pending forever. Guard re-entry.

---

## Verified Correct (known failure modes explicitly checked)

1. **Factory preset value conventions** — full param-by-param audit clean: engineering units
   through `convertTo0to1` (PluginProcessor.cpp:691-695), applied via the matching normalized
   setter; spot math verified on outputLevel/width/vibratoRate/flutterRate/referencePitch/
   jetReflection/tuningSystem/instrumentPreset; no skewed param appears in any factory preset
   (ADSR omitted → reset to defaults by the module's reset-first apply). ✓
2. **Preset apply resets all params first** (OuariconPresetManager.h:295-300). ✓
3. **Preset name "/" hazard** sanitized for user save/load/delete (OuariconPresetManager.h:199-202);
   see IN-17 for the factory-write gap in the shared module. ✓
4. **Windows WebView2 config** — `NEEDS_WEBVIEW2 TRUE` + static-linking define + user-data
   folder all present. ✓
5. **Single `juce_add_binary_data` target** (`O-Wind_UIResources`) — no namespace collision. ✓
6. **No param-ID/class shadowing**; 56 param IDs, zero duplicates. ✓
7. **setStateInformation robustness** — null/short/corrupt data no-ops; missing params keep
   current values; TuningEngine cross-thread access is atomics + interval mutex. ✓
8. **VST3 Note Expression** matches the O-Lyrica validated pattern (`getVST3ClientExtensions`
   override, `drainAndUpdate()` first in processBlock, pending-table pointer at prepare);
   per-voice tuning consumed via atomic `exchange` — no cross-voice bleed; no keyswitches, so
   no silent opt-in trap. ✓
9. **Embedded-tuning period pushed before `setCustomIntervals`** (PluginEditor.cpp:450-452). ✓
10. **Oversampled-path filter corners** — all voice filters designed AND run at the internal
    2× rate; FX at native rate; no corner shift. `setLatencySamples` from oversampler latency. ✓
11. **Resource provider bare-path handling**; **`Juce` ES-module namespace** passed to
    TuningPanel (not `window.__JUCE__`). ✓
12. **Editor teardown** — relays → webView → attachments declaration order with explicit
    reverse `reset()`; no timers; `resized()` null-guards. ✓
13. **exp/smoothing coefficients + numSamples guards** present at all design sites;
    loop gain bounded (end reflection ≤ 0.99, ≥5% loss blend, jet tanh every cycle); DC
    blocker before the bore; output tanh clip; `ScopedNoDenormals`. No NaN-injection path
    found (all cutoffs clamped to [lo, 0.45·fs] before design) — if a guard is added later,
    follow the O-Formant v1.25.2 pattern (validate coeffs, keep last-known-good, reset
    excitation too). ✓
14. **Voice lifecycle** — hard stopNote fully resets; JUCE steals via `stopNote(0,false)`;
    releaseFade backstop guarantees `clearCurrentNote()` — EXCEPT the ADSR-disable path
    (CR-04) and the missing inactive-voice early-out (WR-06). ✓ (with those exceptions)
