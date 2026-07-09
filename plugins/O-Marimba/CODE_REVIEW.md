---
phase: O-Marimba-v1.12.0
reviewed: 2026-07-08
depth: deep
files_reviewed: 15
files_reviewed_list:
  - plugins/O-Marimba/Source/PluginProcessor.cpp
  - plugins/O-Marimba/Source/PluginProcessor.h
  - plugins/O-Marimba/Source/PluginEditor.cpp
  - plugins/O-Marimba/Source/PluginEditor.h
  - plugins/O-Marimba/Source/MarimbaVoice.cpp
  - plugins/O-Marimba/Source/MarimbaVoice.h
  - plugins/O-Marimba/Source/MarimbaSound.h
  - plugins/O-Marimba/Source/BodyResonance.cpp
  - plugins/O-Marimba/Source/BodyResonance.h
  - plugins/O-Marimba/Source/TuningEngine.cpp
  - plugins/O-Marimba/Source/TuningEngine.h
  - plugins/O-Marimba/Source/PresetManager.cpp
  - plugins/O-Marimba/Source/PresetManager.h
  - plugins/O-Marimba/Source/ui/public/index.html
  - plugins/O-Marimba/CMakeLists.txt
  # cross-referenced: modules/effects/analog-eq-unit/{js,cpp}, modules/effects/compressor-unit CompressorUnit.h
findings:
  critical: 3
  warning: 10
  info: 16
  total: 29
status: issues_found
---

# O-Marimba v1.12.0: Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: [A] mallet-strike voice + modal resonators + body IR · [B] processor + parameter layout + preset system · [C] WebView editor bridge + tuning engine + HTML/JS UI + build)
**Files Reviewed:** 15 O-Marimba source/build files, with the shared `analog-eq-unit` / `compressor-unit` modules cross-referenced for param-binding and readout checks.
**Status:** issues_found

## Summary

O-Marimba is a modal physical-model marimba: a mallet-strike exciter driving an 8-mode two-pole
resonator bank per voice, a synthetic-IR convolution body resonator, a tone one-pole, Scala/KBM
microtonal tuning, a shared analog-EQ + compressor FX chain, and a WebView UI. **The per-voice DSP
core is clean and RT-safe** — modal coefficients are hand-rolled floats computed once at note-on (no
`IIR::Coefficients` heap alloc), the voice render path is denormal-guarded, per-note reset hygiene is
complete, and the WebView param plumbing has **no dead knobs** (a common failure mode in this suite).

The defects cluster in three high-value areas: a **microtonal correctness bug** (the JS scale model
drops the octave period, so every non-12-TET scale mistunes across octave boundaries — the headline
tuning feature is silently broken for anything but equal temperament), a **WebView-teardown UAF**
(six `launchAsync` completions with no `SafePointer`), and the **preset system** (name-slash data
loss + no reset-to-defaults on recall + factory presets authored as partials that omit the entire FX
section). A second tier of audible-but-narrower voice issues (Nyquist fold-back on high notes, a
per-note tail-truncation click) and the usual RT-hygiene cleanups round it out.

Several of this codebase's recurring failure modes are handled **correctly** in O-Marimba:

- **`ScopedNoDenormals` at the top of `processBlock`** (PluginProcessor.cpp:189) *and* at the top of
  the voice `renderNextBlock` (MarimbaVoice.cpp:112). Master + voice both protected. ✓
- **`getLatencySamples()` NOT overridden** — no declaration/definition; correct for JUCE 8 (zero-latency
  synth). ✓
- **RT-safe modal coefficients** — `calculateModalCoefficients` (MarimbaVoice.cpp:346-380) writes raw
  `float b0/a1/a2` into pre-existing `ModalMode` structs via `std::exp`/`std::cos`, runs **once per
  note in `startNote`**, never per-block. No `Coefficients::makeXXX`, no audio-thread heap alloc, and
  no per-block modal-coeff zipper. ✓ (`pattern_arraycoefficients_rt_safe_iir` not violated here.)
- **Clean per-note reset hygiene** — `startNote` resets every mode (`mode.reset()` loop), the exciter
  (`filterState=0`), and the tone filter (`toneFilterState=0`) so a stolen/re-triggered voice inherits
  no stale state (MarimbaVoice.cpp:44-75). ✓
- **No dead parameter knobs** — all 10 synth params + the full `fx_eq_*` / `fx_comp_*` sets each have a
  `WebSliderRelay` + `WebSliderParameterAttachment` + matching HTML binding (PluginEditor.cpp:18-56,
  168-247). This suite's most common regression (`pattern_webview_native_fn_bridge_gap` /
  dead-knob drift) is **absent**. ✓
- **Both Windows WebView flags present** — `NEEDS_WEBVIEW2 TRUE` (CMakeLists.txt:14) +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (CMakeLists.txt:66); `withUserDataFolder()` set. ✓
- **Resource provider compares BARE paths** (`url == "/" || url == "/index.html"`, PluginEditor.cpp:397)
  — no "Frame load interrupted." ✓
- **Tuning UI uses the ES-module `Juce` namespace** for `getNativeFunction`/`getSliderState`, not
  `window.__JUCE__` (index.html:1230) — `critical_juce_webview_namespace_vs_postmessage` avoided. ✓
- **Single `juce_add_binary_data` target** (`OMarimba_UIResources`) — no `BinaryData` namespace
  collision. ✓
- **Editor destruction order correct** (relays → webView → attachments declared; `stopTimer()` first in
  the dtor, PluginEditor.cpp:265). ✓
- **Real `.scl` loading preserves the period** — `parseScalaFile` keeps the file's final interval as
  the equave; the period bug (CR-01) is confined to the JS-preset/interval-edit path. ✓

---

## Critical (3)

### CR-01 — JS scale model omits the octave period → every non-12-TET scale mistunes across octaves
- **File:** `Source/ui/public/index.html:1567,1629` (scale model + `applyTuning`) → `Source/TuningEngine.cpp:120-144` (`setCustomIntervals`), `:196-231` (`calculateScala`, esp. :208-209, :222)
- **Severity:** Critical
- **What:** The UI models a 12-note scale as 12 pitch-class entries ending at the **11th degree, not the
  period**: `currentIntervals = [0,100,…,1100]` (Just = `[0,112,…,1088]`), and `applyTuning()` sends
  `currentIntervals.slice(1)` — 11 values ending at 1100/1088, **never 1200/2:1**. `setCustomIntervals`
  prepends `0.0` and sets `scaleDegrees = size-1 = 11`. In `calculateScala`, note→degree wrapping uses
  `noteOffset % 11` / `noteOffset / 11` and `octaveCents = scaleIntervals.back()` = **1100/1088**. A
  12-key chromatic octave is thus folded onto an 11-degree cycle with a compressed equave.
- **Failure scenario:** Click **CUSTOM** (loads Just Intonation) → C#5 (MIDI 73) computes
  `scaleIntervals[13%11=2]=204 + 1×1088 = 1292` cents, but a true JI C#5 is 1312¢ — **20¢ flat**, and
  the drift compounds one step per octave (equave compresses ~112¢/octave). Every non-equal scale, and
  every hand-edited interval, plays audibly wrong up and down the keyboard. **12-TET survives only by
  arithmetic accident** (`100·(k mod 11) + 1100·⌊k/11⌋ ≡ 100·k`).
- **Root cause:** `pattern_embedded_tuning_period_dropped` — the equave (1200¢ / 2:1) must be the final
  interval so `scaleDegrees` and `back()` are correct; the JS scale array drops it, leaving `back()`
  pointing at a scale degree instead of the period. (Direct `.scl` file load is unaffected — those files
  include the period as their last entry.)
- **Fix:** Append the period as the final interval in the load path itself so callers can't forget it:
  in `setCustomIntervals`, `scaleIntervals.push_back(period)` (default 1200¢) → `scaleDegrees = 12`,
  `octaveCents = 1200`. Equivalently include 1200 as the last entry of `currentIntervals`/`scalePresets`
  in JS. Prefer the C++ push_back so the period is guaranteed regardless of the JS payload.

### CR-02 — Six `launchAsync` completions capture `this` with no SafePointer → editor-teardown use-after-free
- **File:** `Source/PluginEditor.cpp:469` (`loadScalaFile`), `:501` (`loadKBMFile`), `:634` (`savePreset`), `:767` (`loadPresetFromFile`), `:811` (`saveScalaFile`), `:863` (`saveKBMFile`)
- **Severity:** Critical (narrow trigger: window closed while a native file dialog is open)
- **What:** Every `fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc){ … })` captures a
  bare `this` and, inside, dereferences `processorRef`, editor members (`lastScalaExportPath`,
  `lastKBMExportPath`), and calls `webView->evaluateJavascript(...)` / `notifyPresetLoaded(...)` — none
  guarded by a `juce::Component::SafePointer`.
- **Failure scenario:** User opens a Save/Load dialog, then closes the plugin window (or the DAW tears
  down the editor) before choosing a file. When the async panel fires, the lambda runs after
  `~OMarimbaAudioProcessorEditor` → **use-after-free on `processorRef`/`webView` → host crash**
  (`pattern_webview_launchasync_safepointer_no_complete`).
- **Root cause:** No liveness check on the editor before the async completion touches editor/WebView state.
- **Fix:** Capture `juce::Component::SafePointer<OMarimbaAudioProcessorEditor> safe{this}` and, at the top
  of each completion, `if (safe == nullptr) return;` — **bail with a bare `return`** (do NOT call any
  `complete`, per the shipped O-MicrotonalSampler W12 fix). Route all member/`webView` access through
  `safe->…`. Apply to all six sites.

### CR-03 — Preset name containing "/" is silently dropped (data loss)
- **File:** `Source/PresetManager.cpp:167` (`savePreset`); same verbatim-name pattern at :41, :188, :191, :225
- **Severity:** Critical
- **What:** `getUserPresetsDirectory().getChildFile(presetName + ".json")` uses the raw preset name as the
  filename. `juce::File::getChildFile` treats "/" as a path separator, so `"Warm / Bright"` resolves to
  `…/User/Warm /Bright.json` — written into an unexpected/absent subdir. `getPresetList()` uses
  `findChildFiles(..., false, …)` (non-recursive), so the file never reappears in the list and the bar
  shows a garbled name.
- **Failure scenario:** User saves `"Marimba 1/2 Damped"` → `savePreset` reports success but the patch is
  invisible forever. Silent data loss (`critical_preset_name_slash_path_separator`).
- **Root cause:** No sanitization of `presetName` before using it as a filesystem path.
- **Fix:** Sanitize the *filename* while keeping the display name: `auto safeName =
  juce::File::createLegalFileName(presetName);` and build the file from `safeName + ".json"` — applied
  consistently in `savePreset`/`loadPreset`/`deletePreset`/`isFactoryPreset`. (Or reject/replace "/" at
  the save boundary.)

---

## Warning (10)

### WR-01 — `applyPresetJson` never resets to defaults; factory presets omit ALL `fx_eq_*`/`fx_comp_*` params → stale FX on recall
- **File:** `Source/PresetManager.cpp:85` (`applyPresetJson`, restore loop :100-106), `:571-583` (`initializeFactoryPresets`)
- **Severity:** Warning
- **What:** `applyPresetJson` iterates **only** the keys present in the preset JSON and never resets omitted
  params to defaults first. `initializeFactoryPresets` writes only 10 synth params — **no** `fx_eq_*`,
  `fx_eq_enabled`, or `fx_comp_*`. So every factory preset is a partial preset.
- **Failure scenario:** User enables EQ + heavy compression, then selects "Default Marimba" → the clean
  preset recalls with the prior FX fully engaged. Also affects legacy user presets saved before the
  timbre params existed (omitted keys inherit whatever was last loaded).
  (`pattern_preset_apply_needs_reset_to_defaults`.)
- **Fix:** At the top of `applyPresetJson`, before the restore loop, reset every APVTS param to its
  default (iterate `processor.getParameters()`, `param->setValueNotifyingHost(param->getDefaultValue())`),
  then apply the stored subset. Optionally also make factory presets write the full param set.

### WR-02 — `scaleIntervals`/`scaleDegrees` read on the audio thread while mutated on the message thread with no lock → torn read / OOB
- **File:** `Source/TuningEngine.cpp:79-80` (members), `:196-231` (`calculateScala` reads), `:120-144` (`setCustomIntervals` clear+push_back), `:68-94` (`loadScalaFile`); driven from `Source/PluginProcessor.cpp:230-235`
- **Severity:** Warning
- **What:** `processBlock` calls `tuningEngine.setMode(...)`/`setReferencePitch(...)` every block; on change
  these call `rebuildFrequencyTable()` → `calculateScala()`, which reads the plain `std::vector<double>
  scaleIntervals` / `int scaleDegrees` **on the audio thread**. Meanwhile `setCustomIntervals`/`loadScalaFile`
  `clear()` + `push_back()` (reallocating) **on the message thread**. No synchronization exists — despite
  the header comment (:78) claiming a mutex "for loading from message thread."
- **Failure scenario:** Drag an interval box (or JS `applyTuning`) at the instant the A4-reference/tuning
  mode is nudged → audio-thread `calculateScala` indexes `scaleIntervals[degree]`/`.back()`
  mid-reallocation → out-of-bounds/torn read → garbage frequency or crash. (Secondary: `rebuildFrequencyTable`
  runs 128 `std::pow` on the audio thread on ref/mode change — bounded but avoidable RT work.)
- **Fix:** Move `rebuildFrequencyTable` fully off the audio thread — have `processBlock` publish param
  changes via an atomic flag consumed on the message thread, or double-buffer the frequency table and swap
  an atomic pointer so the audio thread never reads the mutating vector. At minimum, guard the shared
  containers with a lock taken only off-audio (then actually implement the promised mutex).

### WR-03 — No Nyquist guard on modal frequencies → upper modes alias/fold back on high notes
- **File:** `Source/MarimbaVoice.cpp:351-379` (`calculateModalCoefficients`); ratios in `MarimbaVoice.h:58-60`
- **Severity:** Warning
- **What:** `modeFreq = baseFreq * MODE_RATIOS[i]` (ratios up to `54.0`), then `theta = twoPi*modeFreq/fs`,
  with no check that `modeFreq < fs/2`. A two-pole resonator with `theta > π` resonates at the aliased
  `fs − modeFreq`. Mode 3 (`16.27×`) exceeds 22.05 kHz for any fundamental above ~1355 Hz; at MIDI 96
  (2093 Hz) mode 3 = 34 kHz → folds back to ~10.1 kHz as an inharmonic partial. Higher modes fold worse,
  and Material boost (0.4×–4.0×) can amplify them.
- **Failure scenario:** Play the top ~octave → modes 3-7 fold into the audible band as metallic,
  inharmonic, aliased partials.
- **Fix:** In the mode loop, zero/skip any mode with `modeFreq >= 0.45f * sampleRate`:
  `if (modeFreq >= 0.45f * sampleRate) { modes[i].amplitude = 0.0f; modes[i].b0 = 0.0f; continue; }`.

### WR-04 — Voice hard-terminates at 1.5× decay time (~−13 dB) with no fade → click on every note
- **File:** `Source/MarimbaVoice.cpp:72` (startNote arms `samplesUntilRelease`) + `:148-158` (renderNextBlock terminate)
- **Severity:** Warning
- **What:** `samplesUntilRelease = int(maxDecayTime * sampleRate * 1.5f)`. The modal ring envelope is
  `exp(-t/decayTime)`; at `t = 1.5·decayTime` the fundamental is at `exp(-1.5) ≈ 0.223` = **−13 dB of
  peak**, still clearly audible. When the counter hits 0 the voice does an abrupt
  `clearCurrentNote(); isActive=false; break;` — the last sample written is at full ring amplitude, then
  output stops.
- **Failure scenario:** Any single note → hard discontinuity at −13 dB → audible click / truncated tail on
  every note, worst on long RESONANCE settings.
- **Fix:** Either extend the multiplier to ~7× (`exp(-7) ≈ −60 dB`, inaudible cut) or — better — apply a
  short linear fade-out over the final few ms before `clearCurrentNote()`, or terminate only once summed
  modal output falls below a −60 dB threshold.

### WR-05 — Modal biquad has a denormal flush but no NaN guard → latent sticky-silence
- **File:** `Source/MarimbaVoice.h:84-101` (`ModalMode::processSample`)
- **Severity:** Warning (low organic trigger probability; catastrophic if hit)
- **What:** After `output = b0*input + a1*y1 + a2*y2`, the flush `if (std::abs(y1) < 1e-8f && std::abs(y2)
  < 1e-8f){ y1=y2=0; }` does **not** filter NaN (`std::abs(NaN) < 1e-8f` is false), so a NaN in `y1`/`y2`
  latches permanently into `y2=y1; y1=output;`. The guard also only touches filter state, never the
  exciter (`pattern_biquad_nan_guard_sticky_silence`). Excitation is bounded (≈[−2,2]) and `r` is clamped
  to 0.9999, so there is no *identified* NaN source today — hence Warning, not Critical — but the
  consequence (permanent per-voice silence, NaN propagating through the tone filter and out to the WR-06
  output path) is catastrophic.
- **Fix:** Add `if (!std::isfinite(output)) { output = 0.0f; y1 = 0.0f; y2 = 0.0f; }` before storing state,
  and reset the `exciter` on the same condition (source reset, not just filter). Complements WR-06.

### WR-06 — No master-output DC blocker, NaN/Inf guard, or ceiling limiter
- **File:** `Source/PluginProcessor.cpp:255-291` (`processBlock` output stage)
- **Severity:** Warning
- **What:** After voices → body resonator → EQ → compressor → `applyGain`, there is no output safety net:
  no master DC blocker, no finite sanitization, no ceiling limiter. A resonator/biquad blow-up (WR-05) or
  runaway feedback propagates straight to the output *and* into `waveformFifo.write` + the VU calc
  (`getMagnitude` returns NaN → corrupt meter/scope).
- **Failure scenario:** A single non-finite sample corrupts the FIFO/VU and can silence/garble downstream
  audio until the host is reset.
- **Fix:** Add a final finite-check + soft-clip pass over the master buffer (replace non-finite with 0,
  then `jlimit(-2.f, 2.f, x)` or `tanh` soft clip) before the FIFO write and VU calc.

### WR-07 — Per-block string-keyed APVTS lookups (11) in processBlock
- **File:** `Source/PluginProcessor.cpp:218-231,264` (`processBlock`); compressor repeats internally at :269
- **Severity:** Warning
- **What:** processBlock performs 11 `parameters.getRawParameterValue("id")->load()` hashed-string map
  lookups every callback (OUTPUT_GAIN, VEL_CURVE, MALLET_HARDNESS, BAR_MATERIAL, RESONANCE,
  STRIKE_POSITION, OVERTONE_DAMPING, TONE, TUNING_MODE, REFERENCE_PITCH, fx_eq_enabled), plus the
  `fx_comp_`-keyed reads inside `compressorUnit.process`. Violates the mandated cache-`atomic<float>*`
  pattern.
- **Fix:** Resolve `std::atomic<float>*` members once in `prepareToPlay`; read `->load()` in `processBlock`.
  No functional change.

### WR-08 — No `isBusesLayoutSupported` override → unguarded channel layout
- **File:** `Source/PluginProcessor.h:96-186` (no declaration); consumer at `Source/PluginProcessor.cpp:278`
- **Severity:** Warning
- **What:** Only a stereo output bus is declared in the ctor, but `isBusesLayoutSupported` is not
  overridden, so the base default accepts any host-proposed layout. processBlock then unconditionally
  `buffer.getReadPointer(0)` (:278) and prepares EQ for `getTotalNumOutputChannels()` (:175). A host that
  negotiates 0-channel/surround silently changes behavior vs. the prepared channel count.
- **Fix:** Override `isBusesLayoutSupported` to accept only mono/stereo output
  (`layouts.getMainOutputChannelSet()` == `mono()` or `stereo()`), rejecting others.

### WR-09 — EQ frequency readouts use a linear map on skew-0.3 params → ~3.5× wrong Hz + off-default reset (shared `analog-eq-unit` module)
- **File:** `modules/effects/analog-eq-unit/js/analog-eq-unit.js:71-77,534-535,588`; ranges in `modules/effects/analog-eq-unit/cpp/AnalogEQUnit.h:53,70,92,114` (all `NormalisableRange(..., 0.3f)`)
- **Severity:** Warning
- **What:** Formatters compute display Hz **linearly from the normalised value** (`lf_freq: v =>
  Math.round(30 + v*470)`, fed `freqState.getNormalisedValue()`), but the C++ params are skewed 0.3. At
  norm 0.5 the LF knob displays `265 Hz` while the real filter sits at `30 + 470·0.5^(1/0.3) ≈ 77 Hz` — a
  ~3.5× error; double-click reset (`setNormalisedValue(0.5)`) likewise lands off the C++ default
  (`pattern_webview_knob_readout_scaled_value`).
- **Failure scenario:** User voices around a filter they believe is at 265 Hz but is actually near 77 Hz;
  "reset" doesn't return to the shipped default.
- **Fix:** Use `state.getScaledValue()` for the Hz label and drive dbl-click reset from a
  `getParameterDefaults`-style native fn (or engineering-unit default via the range), not a hardcoded 0.5.
  **Shared-module fix — benefits every plugin using `analog-eq-unit`; coordinate the module bump.**

### WR-10 — BodyResonance dry/wet mix doesn't compensate convolution latency → comb filtering
- **File:** `Source/BodyResonance.cpp:47-51` (prepare) + `:59-71` (process)
- **Severity:** Warning (verify `getLatency()` first — moot if 0)
- **What:** `dryWetMixer` is prepared and mixed (`pushDrySamples`/`mixWetSamples`) but
  `setWetLatency(...)` is never called. `juce::dsp::Convolution` reports non-zero latency via
  `getLatency()` (partitioned engine); the un-delayed dry path then sums with a latency-delayed wet path.
- **Failure scenario:** At partial `mix`, dry + delayed-wet sum → comb-filter notches / phasey coloration
  on the tone.
- **Fix:** After `convolution.prepare(spec)`, `dryWetMixer.setWetLatency((float)convolution.getLatency());`
  (re-set on any IR reload). **Verify the reported latency for this short-IR config** — if the engine picks
  the non-partitioned path (latency 0) this is moot; otherwise it is a real comb.

---

## Info (16)

### IN-01 — Factory preset values hand-normalized instead of `convertTo0to1` (latent skew bug)
- **File:** `Source/PresetManager.cpp:578,580,583` — `TUNING_MODE = mode/2.0f`, `REFERENCE_PITCH = (p-400)/80`, `OUTPUT_GAIN = (g+24)/36`. Correct *today* (all ranges are linear, skew==1, verified in `createParameterLayout`), but the moment any gains a skew all 10 factory presets recall ~10-30× wrong (`pattern_factory_preset_normalized_ignores_skew`); `/2.0f` also breaks if a 4th tuning mode is added. Author in engineering units + `getParameter("id")->convertTo0to1(value)`.

### IN-02 — `if (true) // Force regeneration` rewrites all 10 factory preset files on every construction
- **File:** `Source/PresetManager.cpp:563` (called from ctor `PluginProcessor.cpp:150`) — every DAW instance/scan overwrites 10 `.json` files on the message thread and clobbers any external edit. Gate on a version/exists check.

### IN-03 — `midiLock` CriticalSection taken on the audio thread
- **File:** `Source/PluginProcessor.cpp:196` (processBlock) vs `:319` (`addMidiMessage`) — a blocking lock on the RT thread for the UI→audio MIDI hand-off; priority-inversion risk. Sibling plugins use a lock-free SPSC/`AbstractFifo` MIDI ring — switch to that.

### IN-04 — Dead code: `MarimbaVoice::setOutputGain` / `outputGain` never called or read
- **File:** `Source/MarimbaVoice.cpp:162-166` + `MarimbaVoice.h:145` — gain is applied once at end-of-chain (`buffer.applyGain`, PluginProcessor.cpp:274, per the v1.9.8 refactor); the per-voice member is vestigial. Remove.

### IN-05 — Dead store: `velocity` member set in `startNote` but never read
- **File:** `Source/MarimbaVoice.cpp:35` + `MarimbaVoice.h:144` — every use takes the `velocityValue` argument directly (`applyVelocityCurve(velocityValue)`). Remove the member.

### IN-06 — BodyResonance IR normalization applied twice; the 0.45 headroom is discarded
- **File:** `Source/BodyResonance.cpp:169-171` (`0.45f/maxVal` scale) vs `:44` (`loadImpulseResponse(..., Normalise::yes)`) — the convolution re-normalizes, overriding the manual headroom scaling (no-op). Pass `Normalise::no`, or drop the manual scale and control level via `mix`.

### IN-07 — `BodyResonance::process` has no local `ScopedNoDenormals`
- **File:** `Source/BodyResonance.cpp:59` — covered by the master guard today, but belt-and-suspenders (and render-harness isolation) wants a local `juce::ScopedNoDenormals` over the convolution + dry-delay path.

### IN-08 — `setTone` recomputes the tone-filter coefficient every block with no smoothing
- **File:** `Source/MarimbaVoice.cpp:206-214` (called per-block from PluginProcessor.cpp:250) — one-pole coeff jumps are largely click-free (state stays continuous), but fast TONE automation can produce mild stair-stepping. Optional `SmoothedValue`, or only recompute on change. (Modal params are NOT affected — they recompute only at note-on. ✓)

### IN-09 — `stopNote(allowTailOff=true)` is a no-op; key release doesn't damp the note
- **File:** `Source/MarimbaVoice.cpp:78-93` — the voice rings for the fixed note-on countdown regardless of key release; there's no way to damp. Arguably authentic for a struck bar, but document it or add an optional release fade.

### IN-10 — `releaseResources` resets only `bodyResonance`; synth voices not reset in `prepareToPlay`
- **File:** `Source/PluginProcessor.cpp:181-185` (asymmetric — EQ/comp not reset) + `:157-179` (no `synthesiser.allNotesOff`) — a sample-rate change with notes held can carry stale voice phase/envelope. Reset all sub-processors symmetrically and/or `allNotesOff(0,false)` at the top of `prepareToPlay`.

### IN-11 — Comment/mode-index mismatch in `getModeAmplitude`
- **File:** `Source/MarimbaVoice.cpp:289-290,310` — comments say "mode 2 (the tuned double octave)" but `MODE_RATIOS[1] = 4.00×` is **mode 1**, and the `if (modeIndex > 1)` guard protects modes 0-1. Fix the prose to "mode 1 (4×)".

### IN-12 — Hardcoded preset metadata version `"1.6.2"` drifted from the plugin version
- **File:** `Source/PresetManager.cpp:79,603` — both serializers stamp `"1.6.2"` while the plugin ships 1.12.0; unusable for migration. Source from `ProjectInfo::versionString` / `JucePlugin_VersionString`.

### IN-13 — Tooltip persistence is entirely dead (`window.JuceAPI` doesn't exist; `setTooltipsEnabled` unregistered)
- **File:** `Source/ui/public/index.html:2409-2411,2469` — the save branch guards on `window.JuceAPI` (which is never created; the real handles are `Juce`/`window.__JUCE__`), `setTooltipsEnabled` has no `withNativeFunction` registration, and `restoreTooltipState` is never called from C++. Tooltip state never persists. Register the fn via the imported `Juce` namespace + call restore on load, or delete the dead branch.

### IN-14 — `deletePreset` native fn registered in C++ but never called from JS
- **File:** `Source/PluginEditor.cpp:157-159,739-748` — no `getNativeFunction("deletePreset")` in the UI (no delete affordance). Wire a delete control or drop the registration.

### IN-15 — Waveform + VU `requestAnimationFrame` loops poll unconditionally and are never cancelled
- **File:** `Source/ui/public/index.html:1971-1984,2038-2055` — `updateWaveform` calls `getWaveformData` every frame even off the SOUND tab; `animateVUMeter` runs forever; neither is visibility-gated or stopped on `pagehide`. (C++ level meters correctly use `emitEventIfBrowserIsVisible` and the timer is stopped before teardown, so this is wasted work, not a crash.) Gate on `currentTab === 'main'` + `visibilitychange`.

### IN-16 — WebView2 user-data folder is the shared temp root, not a plugin-specific subdir
- **File:** `Source/PluginEditor.cpp:62-65` — `withUserDataFolder(tempDirectory)` points at the bare temp root shared by all apps; use `…tempDirectory.getChildFile("OMarimba_WebView")` to avoid Windows user-data contention.

---

## Recommended resolution order

1. **CR-02** (UAF crash — highest user-facing risk) — SafePointer + bare `return`, all 6 launchAsync sites.
2. **CR-01** (headline tuning feature broken) — push the period into `setCustomIntervals` so every scale
   gets a correct equave; **CR-03** (preset data loss) — `createLegalFileName` on the save/load/delete paths.
3. **WR-01 + WR-02** (preset stale-FX + tuning audio-thread race) — reset-to-defaults before apply; move
   `rebuildFrequencyTable` off the audio thread / double-buffer the table.
4. **WR-05 + WR-06** (NaN sticky-silence: voice source guard + master output guard together).
5. **WR-03 + WR-04** (audible voice defects — Nyquist fold-back on high notes; per-note tail-cut click).
6. **WR-07 / WR-08 / WR-09 / WR-10** then the IN sweep as a cleanup pass. WR-09 is a shared
   `analog-eq-unit` module fix — coordinate the module bump so all dependents benefit.

Resolve via `/improve-review O-Marimba` (this file is the completed investigation — root causes and
fixes are prescribed; verify each against source, then apply).
