---
phase: O-Bowed-v1.3.0
reviewed: 2026-07-08
depth: deep
files_reviewed: 28
files_reviewed_list:
  - plugins/O-Bowed/Source/PluginProcessor.cpp
  - plugins/O-Bowed/Source/PluginProcessor.h
  - plugins/O-Bowed/Source/PluginEditor.cpp
  - plugins/O-Bowed/Source/PluginEditor.h
  - plugins/O-Bowed/Source/BowedStringVoice.cpp
  - plugins/O-Bowed/Source/BowedStringVoice.h
  - plugins/O-Bowed/Source/BowedMPESynthesiser.h
  - plugins/O-Bowed/Source/DSP/WaveguideString.cpp
  - plugins/O-Bowed/Source/DSP/WaveguideString.h
  - plugins/O-Bowed/Source/DSP/ElastoPlasticFriction.h
  - plugins/O-Bowed/Source/DSP/ThermalFriction.h
  - plugins/O-Bowed/Source/DSP/BowNoiseGenerator.h
  - plugins/O-Bowed/Source/DSP/SubHarmonicsGenerator.h
  - plugins/O-Bowed/Source/DSP/BodyResonator.cpp
  - plugins/O-Bowed/Source/DSP/BodyResonator.h
  - plugins/O-Bowed/Source/DSP/SympatheticStringEngine.cpp
  - plugins/O-Bowed/Source/DSP/SympatheticStringEngine.h
  - plugins/O-Bowed/Source/DSP/StereoWidthProcessor.h
  - plugins/O-Bowed/Source/DSP/HumanizeEngine.h
  - plugins/O-Bowed/Source/EmbeddedTunings.h
  - plugins/O-Bowed/Resources/ui/index.html
  - plugins/O-Bowed/Resources/ui/js/juce/index.js
  - plugins/O-Bowed/Resources/ui/js/juce/check_native_interop.js
  - plugins/O-Bowed/CMakeLists.txt
  - plugins/O-Bowed/tests/render-harness/main.cpp
findings:
  critical: 5
  warning: 7
  info: 13
  total: 25
status: issues_found
---

# O-Bowed v1.3.0: Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: [A] bow/friction/waveguide voice · [B] resonance/effects DSP + processor · [C] editor/WebView bridge + tuning usage + build/harness)
**Files Reviewed:** 28 (23 substantive Source files — 4 tuning headers are shared-module redirects — plus UI, build, and harness)
**Status:** issues_found

## Summary

O-Bowed is a physical-modeling bowed-string synth: a digital-waveguide string with a nonlinear
bow-friction excitation, an 8-mode morphing body resonator, a sympathetic-string feedback bank,
bow-noise / sub-harmonics / stereo-width / humanize stages, an MPE synthesiser, Scala/KBM
microtonal tuning (shared `scala-tuning-engine` module) with VST3 Note Expression, and a WebView
UI. **The core waveguide DSP is well-guarded** — tanh rail saturation, ρ clamps, manual denormal
flushes, per-note clean resets, and a stable output limiter/DC-blocker are all present. The
defects cluster in three high-value, recurring areas: **audio-thread heap allocation on
coefficient rebuilds**, **UI ↔ parameter/native-fn drift** (four dead knobs + a largely-dead
Tuning panel), and a **WebView teardown UAF**.

Several of this codebase's recurring failure modes are handled **correctly** in O-Bowed:

- **`ScopedNoDenormals` at the top of `processBlock`** (PluginProcessor.cpp:291); body ring-out
  and sympathetic tails are denormal-protected on the master path. ✓
- **`getLatencySamples()` NOT overridden** — latency published via `setLatencySamples()` in
  `prepareToPlay` (PluginProcessor.cpp:276). Correct for JUCE 8. ✓
- **Factory-preset skew handling is CORRECT** — skewed params (`bowSpeed`/`bowPressure`/
  `stringGauge`, skew 0.5) are authored via `convertTo0to1` (sqrt), not raw linear fractions;
  Violin bowSpeed 0.30151 → exactly 0.2 m/s. Not affected by
  `pattern_factory_preset_normalized_ignores_skew` (only the *readout* JS is, see WR-07). ✓
- **Both Windows WebView flags present** — `NEEDS_WEBVIEW2 TRUE` (CMakeLists.txt:18) +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (CMakeLists.txt:110); `withUserDataFolder()`
  set to a temp dir. No blank-UI-on-Windows. ✓
- **Resource provider compares BARE paths** (`url == "/"`), not stripped URLs — no "Frame load
  interrupted." ✓
- **Single `juce_add_binary_data` target** (`O-Bowed_UIResources`) — no `BinaryData` namespace
  collision. ✓
- **Editor destruction order correct** (relays → webView → attachments declared; reverse-destructs
  attachments before webView). ✓
- **Render-harness compiles the WebView editor under `JUCE_WEB_BROWSER=1`** — not broken by the
  Stage-3 WebView swap (`pattern_render_harness_breaks_on_webview_editor`). ✓
- **Tuning panel handed the ES-module `Juce` namespace, not `window.__JUCE__`** (index.html:1695)
  — `getNativeFunction` resolves (`critical_juce_webview_namespace_vs_postmessage`). ✓
- **Waveguide stability nets:** tanh soft-saturation on both delay rails, ρ clamped at multiple
  stages, manual denormal flush before push, per-note `reset()` on trigger, bus layout gated to
  stereo-only (no OOB write pointers). ✓

---

## Critical (5)

### CR-01 — Audio-thread heap allocation: bridge loss-filter `Coefficients` reconstructed on every note-on & brightness automation
- **File:** `Source/DSP/WaveguideString.cpp:94-95` (`updateBridgeFilterCoeffs`)
- **Severity:** Critical
- **What:** `*bridgeLossFilter.coefficients = juce::dsp::IIR::Coefficients<float>(g*(1-p), 0, 1, -p);`
  constructs a temporary `Coefficients<float>` — whose internal `juce::Array<float>` heap-allocates
  — then copy-assigns it (a second alloc + free). This is the exact banned `Coefficients::makeXXX`
  RT violation (`pattern_arraycoefficients_rt_safe_iir`), just spelled with the raw ctor. It runs on
  the audio thread from two live paths: (1) `trigger()` → `updateBridgeFilterCoeffs()` on **every
  MIDI note-on** (`noteStarted` runs inside `processBlock`), and (2) `readJunction()` →
  `if (filterDirty) updateBridgeFilterCoeffs()` whenever **Brightness or Infinite-Sustain** is
  automated.
- **Failure scenario:** A busy MPE passage or any Brightness automation triggers 1-2 heap
  allocations per event on the RT thread → priority inversion / page fault → xrun/dropout.
- **Root cause:** Constructing a fresh `Coefficients` object instead of writing into the
  already-allocated coefficient storage in place.
- **Fix:** Seed the coefficient array once in `prepare()` (off the audio thread), then mutate in
  place: `auto& c = bridgeLossFilter.coefficients->coefficients; c.getReference(0)=g*(1-p);
  c.getReference(1)=0; c.getReference(2)=-p;` (a0=1 is normalized out). No per-call allocation.

### CR-02 — Audio-thread heap allocation: BodyResonator rebuilds 8 `makePeakFilter` coeffs on Material/Size automation
- **File:** `Source/DSP/BodyResonator.cpp:155-158` (`updateCoefficients`, called from `setMaterial`/`setSize`)
- **Severity:** Critical
- **What:** `updateCoefficients()` calls `juce::dsp::IIR::Coefficients<float>::makePeakFilter(...)`
  for all 8 modes; each returns a heap-allocated `ReferenceCountedObject::Ptr`, assigned into
  `bodyModesL[i]`/`bodyModesR[i]` (freeing the prior Ptr). Both alloc and free happen on the audio
  thread. `setMaterial`/`setSize` are called from `processBlock` (PluginProcessor.cpp:355-356); the
  `< 0.001f` change guard is exceeded by any live knob move (0.01 quantization).
- **Failure scenario:** Dragging/automating **Material** or **Size** triggers 8 `new` + up to 16
  `delete` inside `processBlock` per block → allocator lock / priority inversion → xrun. Same class
  as the documented O-Formant EQ regression.
- **Root cause:** `Coefficients::makePeakFilter` (heap) instead of `ArrayCoefficients::makePeakFilter`
  (stack) with in-place assignment into pre-allocated coefficient objects.
- **Fix:** Allocate one `Coefficients<float>::Ptr` per mode in `prepare()` (share between matched
  L/R — coeffs identical), then in `updateCoefficients` fill from
  `ArrayCoefficients<float>::makePeakFilter(...)` (a stack `std::array`) copied element-wise into the
  existing `->coefficients` array. No alloc/free on the audio thread.

### CR-03 — Eight tuning-panel native functions unregistered → most of the Tuning panel is silently dead
- **File:** `Source/PluginEditor.cpp` (registration block) vs `modules/tuning/scala-tuning-engine/js/tuning-panel.js:677,720,811,819,825,862,871,880`
- **Severity:** Critical
- **What:** The shared `tuning-panel.js` calls 8 native functions O-Bowed's editor never registers
  (verified: `grep "\"<fn>\"" PluginEditor.cpp` → 0 for each): `getEmbeddedTuningList`,
  `loadEmbeddedTuning`, `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`,
  `saveScalaFile`, `saveKBMFile`, and `exportTuningHTML` (C++ registers it under the wrong name
  `getTuningHTML`).
- **Failure scenario:** In the Tuning panel the factory-tuning library list is empty, clicking any
  embedded tuning does nothing, **and because every generator funnels its result through the missing
  `applyGeneratedScale`, even the registered `generateEDO` can never apply its scale.** Save .scl /
  Save .kbm / Export HTML are dead. All failures are swallowed by the panel's try/catch — no console
  error, passes build/auval/harness (`pattern_webview_native_fn_bridge_gap`).
- **Root cause:** O-Bowed's registration block predates / was not synced with the current shared
  tuning-panel API. Other pitched plugins register `loadEmbeddedTuning` et al.; O-Bowed was left
  behind. `getTuningHTML` vs `exportTuningHTML` is a straight naming divergence.
- **Fix:** Add the 8 missing `withNativeFunction` registrations, delegating to the shared
  `TuningEngine`/`ScaleGenerator`/`TuningExporter` calls a known-good plugin uses (O-Formant/O-Prism),
  matching each function's expected JSON return shape; rename/alias `getTuningHTML` → `exportTuningHTML`.
  **When wiring `loadEmbeddedTuning`, confirm the path appends the tuning PERIOD before
  `setCustomIntervals` (`pattern_embedded_tuning_period_dropped`) — this correctness lives entirely in
  the shared module and is currently unexercised from O-Bowed.**

### CR-04 — Four real APVTS parameters have no editor relay/attachment → uncontrollable from the UI
- **File:** `Source/PluginProcessor.cpp:113,121,129,137` (params exist) vs `Source/PluginEditor.cpp` (0 bindings)
- **Severity:** Critical
- **What:** `sympatheticDecay`, `bodyAmount`, `stringGauge`, and `bowHairStiffness` are real params
  in `createParameterLayout()` with NormalisableRanges, but the editor creates **no `WebSliderRelay`,
  no `WebSliderParameterAttachment`, and no PARAMS binding** for any of them (verified: each has 0
  occurrences in `PluginEditor.cpp`; the bound `rosin` has 4). Their knobs render in the HTML but the
  binding chain to the APVTS parameter is never established.
- **Failure scenario:** Dragging these four knobs does nothing; they hold a frozen placeholder value.
  Three of them (`bodyAmount`, `stringGauge`, `sympatheticDecay`) DO drive DSP and are recalled by
  presets/automation — so they are shipped, audible parameters that a user simply **cannot reach from
  the UI**. (`bowHairStiffness` is additionally DSP-dead — see WR-02.)
- **Root cause:** UI/param drift — the parameter layout + HTML gained these four but the editor's
  relay/attachment/PARAMS tables were never extended to match.
- **Fix:** For each, add (a) a `WebSliderRelay` member + `.withOptionsFrom(*relay)`, (b) a
  `WebSliderParameterAttachment`, and (c) a matching PARAMS entry (min/max/default/decimals from
  PluginProcessor.cpp:113-142). Mirror the existing `rosin`/`bowNoise` wiring exactly. Decide
  `bowHairStiffness`'s fate jointly with WR-02.

### CR-05 — FileChooser `launchAsync` completions capture `this`+`complete` with no SafePointer → use-after-free on editor teardown
- **File:** `Source/PluginEditor.cpp` — `savePresetWithDialog` (~:200), `loadScalaFile` (~:321), `loadKBMFile` (~:339)
- **Severity:** Critical (narrow trigger: window closed while a native file dialog is open)
- **What:** All three `fileChooser->launchAsync(..., [this, complete](const juce::FileChooser& fc){ … complete(...); })`
  completions capture `this` and the WebView `complete` callback with no `juce::Component::SafePointer`
  guard, and invoke `complete(...)` unconditionally.
- **Failure scenario:** User opens the native Save/Load dialog, then closes the plugin window (or the
  DAW tears down the editor) before choosing a file. When the async panel fires, the lambda runs after
  `~OBowedAudioProcessorEditor`; `complete` is a `std::function` owned by the destroyed
  `WebBrowserComponent` impl → **use-after-free / host crash**
  (`pattern_webview_launchasync_safepointer_no_complete`).
- **Root cause:** No liveness check on the editor before touching the WebView-owned completion.
- **Fix:** Capture `juce::Component::SafePointer<OBowedAudioProcessorEditor> safe(this)` and, on the
  null path, **bail with a bare `return`** — do NOT call `complete(false)` (that is itself a UAF, per
  the shipped O-MicrotonalSampler W12 fix). Apply to all three sites.

---

## Warning (7)

### WR-01 — No NaN/Inf guard on the waveguide excitation; a single bad ρ silences the note mid-sustain
- **File:** `Source/DSP/WaveguideString.cpp:204-235` (`writeJunction`); `Source/BowedStringVoice.cpp:158-190`
- **Severity:** Warning (Critical if any friction model can emit NaN)
- **What:** `std::min(rho, 0.99f)`/`std::min(rho, 0.85f)` do NOT filter NaN (`std::min(NaN,x)==NaN`).
  NaN flows through `tanh(NaN/4)*4 = NaN`; the `< 1e-15f` denormal flush is false for NaN, so NaN is
  pushed into the delay line. `energyEstimate` then becomes NaN → `isActive()` false → `clearCurrentNote()`
  kills the note (audible dropout; recovers on next trigger).
- **Failure scenario:** `stringGauge → 0` feeds `setStringImpedance(0)`; friction reflection divides
  by `R_s` → `Inf` → `rho = NaN` → note silenced.
- **Root cause:** Excitation coefficient trusted finite; the only stability net (tanh) preserves NaN.
- **Fix:** Guard at the write boundary and reset the **source**, not just the line (sticky-NaN pattern):
  `if (!std::isfinite(sample)) { waveguideString.reset(); bowModel.reset(); sample = 0; }`. Also floor
  `R_s`/`stringGauge` away from 0 at the setter.

### WR-02 — `bowHairStiffness` param + the entire ElastoPlasticFriction path are dead (no audio effect)
- **File:** `Source/BowedStringVoice.cpp:158,262,286-288`
- **Severity:** Warning
- **What:** The render loop only calls `frictionModel.computeReflectionCoefficient(v_delta, F_bow)`
  (the `HyperbolicFriction`). The `bristleFriction` (`ElastoPlasticFriction`) has its params set every
  block but its `computeReflectionCoefficient` is **never invoked**, and `bristleBlend` (=`bowHairStiff`)
  is **never read**. `bowHairStiffness` is inert. (Combined with CR-04, this knob is doubly dead: no UI
  binding AND no DSP effect.)
- **Root cause:** The intended core↔bristle blend was parameter-wired but never evaluated in the loop.
- **Fix:** Either implement the blend —
  `rho = (1-bristleBlend)*frictionModel.compute(...) + bristleBlend*bristleFriction.computeReflectionCoefficient(v_delta, F_bow, dt)`
  (note the `dt` arg the elasto model needs) — or remove `bristleFriction`, `bristleBlend`, and the
  `bowHairStiffness` param entirely if out of scope.

### WR-03 — No `ScopedNoDenormals` on the voice render path; bridge loss-filter state denormal-unprotected
- **File:** `Source/BowedStringVoice.cpp:121-214` (`renderNextBlock`); `Source/DSP/WaveguideString.cpp:108/195`
- **Severity:** Warning
- **What:** `renderNextBlock` has no `ScopedNoDenormals`. The manual flush flushes the delay-line feed
  but runs *after* `bridgeLossFilter.processSample`, so the one-pole's own internal state (pole up to
  0.9995, ~15 s tail) decays through denormal range unflushed. The processor's master
  `ScopedNoDenormals` (PluginProcessor.cpp:291) does cover this in the plugin, but the voice is
  unprotected in isolation (e.g. the render harness) and belt-and-suspenders is correct for feedback DSP.
- **Fix:** Add `const juce::ScopedNoDenormals noDenormals;` at the top of `renderNextBlock`.

### WR-04 — Delay length / bridge filter recomputed per-block on MPE-timbre & brightness with no smoothing → zipper/clicks
- **File:** `Source/BowedStringVoice.cpp:270-279`; `Source/DSP/WaveguideString.cpp:157-175`
- **Severity:** Warning
- **What:** `effectivePosition = bowPos + mpeTimbre*0.1` changes continuously with CC74 and is pushed
  to `setBowPosition` every block; any change > 1e-6 recomputes both rail delay lengths (a *step* in
  the Thiran fractional delay) with no per-sample smoothing. Same for Brightness (also feeds CR-01).
- **Failure scenario:** A continuous MPE timbre glide or Brightness automation produces stair-stepped
  pitch/tone glitches at block boundaries.
- **Fix:** Smooth `bowPosition` (and brightness) with a `juce::SmoothedValue`/one-pole advanced per
  sample, driving `setDelay` from the smoothed value.

### WR-05 — Sympathetic "Decay" knob barely affects the fundamental; feedback loop gain = 1 at DC
- **File:** `Source/DSP/SympatheticStringEngine.cpp:197-199` (mapping at :167)
- **Severity:** Warning
- **What:** `filterState = lossCoeff*filterState + (1-lossCoeff)*delayed` uses `lossCoeff` as the
  **pole of a one-pole lowpass**, not a sub-unity feedback loss. A one-pole with pole ∈(0.990,0.9999)
  has DC gain exactly 1.0, so the fundamental rings ~forever regardless of the Decay knob; only high
  harmonics are attenuated. Any DC in the excitation (reachable via subHarmonics/reversedFriction)
  sustains indefinitely inside the loop, eroding headroom toward the ±2 limiter. (Compounds CR-04:
  the knob is also UI-unbound.)
- **Root cause:** `lossCoeff` intended as a per-loop decay gain but wired as the smoothing-filter pole.
- **Fix:** Separate damping from decay: keep a fixed damping lowpass, and multiply the feedback by an
  explicit `decayGain < 1` derived from the Decay param — guarantees exponential decay and drains DC.

### WR-06 — Per-block APVTS parameter reads use string lookups instead of cached atomic pointers
- **File:** `Source/PluginProcessor.cpp:302-336,412`
- **Severity:** Warning
- **What:** `processBlock` performs ~25 `parameters.getRawParameterValue("id")->load()` string-keyed
  map lookups every callback, violating the mandated cache-`std::atomic<float>*`-in-`prepareToPlay`
  pattern. (No heap alloc, but avoidable per-block hashed-string lookups on the RT thread.)
- **Fix:** Resolve `std::atomic<float>*` members once in `prepareToPlay`; read `->load()` in `processBlock`.

### WR-07 — Knob readout + double-click reset use a hardcoded linear map, ignoring NormalisableRange skew
- **File:** `Resources/ui/index.html` — PARAMS (~:790), `normToRaw` (~:906), readout (~:936), dblclick reset (~:978)
- **Severity:** Warning
- **What:** `normToRaw` computes `min + norm*(max-min)` (pure linear) and reset computes
  `(default-min)/(max-min)`, but `bowSpeed`/`bowPressure` (skew 0.5) and `brightness` (skew 0.25) are
  skewed in C++. At norm 0.5, `brightness` displays ~10 kHz but the real DSP value is ~1.27 kHz — an
  **~8× wrong** readout; `bowSpeed`/`bowPressure` read ~2× wrong; double-click reset lands off-default
  for skewed params (`pattern_webview_knob_readout_scaled_value`).
- **Fix:** Replace `normToRaw`/format math with `state.getScaledValue()` from the WebSliderState (JUCE
  pushes the true NormalisableRange via `propertiesChanged`); register a `getParameterDefaults` native
  fn for skew-correct resets. Reusable `ui_frontend_check.js` shipped with O-MicrotonalSampler v1.23.7.

---

## Info (13)

### IN-01 — `dt` member computed in `prepareToPlay` but never consumed on the live friction path
- **File:** `Source/BowedStringVoice.cpp:105`, `.h:121` — remove, or pass into the model once WR-02 is wired.

### IN-02 — `ThermalFriction.h` is unused dead code (256-entry LUT + Euler integrator never instantiated)
- **File:** `Source/DSP/ThermalFriction.h` — exclude from build or document as a future tier.

### IN-03 — ElastoPlastic `R_s` / `z_ss` division unguarded + bristle `z` not reset on note-on (latent behind WR-02's dead path)
- **File:** `Source/DSP/ElastoPlasticFriction.h:75,57,112,140-143`; `Source/BowedStringVoice.cpp:25-47` — clamp `R_s = max(1e-3, impedance)`; call `bristleFriction.resetState()` in `noteStarted` when enabling the bristle path. Becomes real the moment WR-02 is fixed.

### IN-04 — `computeFrictionDerivative` dead (Newton-Raphson finite-difference helper never wired)
- **File:** `Source/DSP/ElastoPlasticFriction.h:85-121`; `Source/DSP/ThermalFriction.h:60-64` — delete or gate behind an implicit-solver path.

### IN-05 — Factory-preset comment documents the WRONG skew exponent for `brightness` (says 4.0, should be 0.25)
- **File:** `Source/PluginProcessor.cpp:466` — stored preset values are actually correct; only the authoring comment inverts the exponent (`pow(prop, 1/skew)` instead of `pow(prop, skew)`). Fix the comment before it traps a future author into the `pattern_factory_preset_normalized_ignores_skew` bug.

### IN-06 — BodyResonator biquad bank has no NaN guard (sticky-silence hazard)
- **File:** `Source/DSP/BodyResonator.cpp:168-188` — a transient NaN from the voice path latches the 8 biquad states to NaN permanently (`pattern_biquad_nan_guard_sticky_silence`). Add `if (!std::isfinite(resonantL)) { reset(); resonantL = 0; }` (and R). Complements WR-01's source reset.

### IN-07 — HumanizeEngine drift rate assumes a fixed block size (max block, not actual)
- **File:** `Source/DSP/HumanizeEngine.h:34-39,59` — `updatesPerSecond = sampleRate/blockSize` derived from max block size; under 64-sample buffers vs 512 max, humanization drifts ~8× faster than the labeled 0.15–8 Hz. Pass real `numSamples` into `update()`.

### IN-08 — Dead per-block `setPan(0.707,0.707)` constant + mislabeled step comments (no `=== 3 ===`, two `=== 4 ===`)
- **File:** `Source/PluginProcessor.cpp:344-349,351,354` — move the constant pan to `prepareToPlay` (or drop it); renumber comments.

### IN-09 — DC-blocker / DSP state not reset in `prepareToPlay`
- **File:** `Source/PluginProcessor.h:110-111`; `Source/PluginProcessor.cpp:264-282` — `dcBlockX/Y` persist across sample-rate changes; `releaseResources()` is a no-op. Zero them and call `bodyResonator.reset()`/`sympatheticEngine.reset()` in `prepareToPlay` to avoid a startup transient.

### IN-10 — `bindComboBox()` defined but never called; no `tuningSystem` `<select>` in the DOM (dangling relay/attachment)
- **File:** `Resources/ui/index.html:997-1023,1715`; `Source/PluginEditor.cpp:52,425` — remove the dead function + `tuningSystemRelay`/attachment, or add the `<select>` and call it.

### IN-11 — `savePreset` native fn registered but never called from JS (UI uses `savePresetWithDialog`)
- **File:** `Source/PluginEditor.cpp:169-178` — remove if unused, or wire a quick-save path.

### IN-12 — `getVisualizationState` polls at 15 Hz unconditionally, not visibility-gated / never cleared
- **File:** `Resources/ui/index.html:1729` — gate on `activeVizTab`/`document.visibilityState`; optionally `clearInterval` on `pagehide`. Reduces wasted native calls and the teardown-race surface with CR-05.

### IN-13 — Several registered tuning native fns are never called by this plugin's JS (registration drifted from shared panel)
- **File:** `Source/PluginEditor.cpp:232,288,301,312,369` — `setTuningIntervals`, `getMasterTune`, `getTemperamentPreset`, `setTemperamentPreset`, `getTuningHTML` unused. Same root cause as CR-03; reconcile the whole set against a known-good plugin in one pass.

---

## Recommended resolution order

1. **CR-05** (UAF crash — highest user-facing risk) — SafePointer + bare `return`, 3 sites.
2. **CR-01 + CR-02** (RT allocation / xrun) — in-place / `ArrayCoefficients` coefficient writes.
3. **CR-04 + CR-03** (dead UI) — bind the 4 params; register the 8 tuning native fns. Verify the
   embedded-tuning PERIOD append when CR-03 lands.
4. **WR-01 + IN-06** (NaN sticky-silence, source + body guards together).
5. **WR-02 + CR-04's `bowHairStiffness` decision** (implement the bristle blend, or remove the param).
6. **WR-05** (sympathetic decay math), **WR-07** (skew readout), then the remaining WR/IN as a cleanup sweep.

Resolve via `/improve-review O-Bowed` (this file is the completed investigation — root causes and
fixes are prescribed; verify each against source, then apply).
