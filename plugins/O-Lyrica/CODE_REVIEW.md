---
phase: O-Lyrica-v2.3.0
reviewed: 2026-07-08
depth: deep
files_reviewed: 39
files_reviewed_list:
  - plugins/O-Lyrica/Source/PluginProcessor.cpp
  - plugins/O-Lyrica/Source/PluginProcessor.h
  - plugins/O-Lyrica/Source/PluginEditor.cpp
  - plugins/O-Lyrica/Source/PluginEditor.h
  - plugins/O-Lyrica/Source/HarpSynthVoice.cpp
  - plugins/O-Lyrica/Source/HarpSynthVoice.h
  - plugins/O-Lyrica/Source/HarpSynthSound.h
  - plugins/O-Lyrica/Source/DSP/WaveguideString.cpp
  - plugins/O-Lyrica/Source/DSP/WaveguideString.h
  - plugins/O-Lyrica/Source/DSP/PluckExciter.cpp
  - plugins/O-Lyrica/Source/DSP/PluckExciter.h
  - plugins/O-Lyrica/Source/DSP/StiffnessFilter.cpp
  - plugins/O-Lyrica/Source/DSP/StiffnessFilter.h
  - plugins/O-Lyrica/Source/DSP/StringMaterial.cpp
  - plugins/O-Lyrica/Source/DSP/StringMaterial.h
  - plugins/O-Lyrica/Source/DSP/BodyResonance.cpp
  - plugins/O-Lyrica/Source/DSP/BodyResonance.h
  - plugins/O-Lyrica/Source/DSP/SympatheticResonance.cpp
  - plugins/O-Lyrica/Source/DSP/SympatheticResonance.h
  - plugins/O-Lyrica/Source/DSP/EQProcessor.cpp
  - plugins/O-Lyrica/Source/DSP/EQProcessor.h
  - plugins/O-Lyrica/Source/DSP/ReverbProcessor.cpp
  - plugins/O-Lyrica/Source/DSP/ReverbProcessor.h
  - plugins/O-Lyrica/Source/DSP/DelayProcessor.cpp
  - plugins/O-Lyrica/Source/DSP/DelayProcessor.h
  - plugins/O-Lyrica/Source/DSP/TuningEngine.cpp
  - plugins/O-Lyrica/Source/DSP/TuningEngine.h
  - plugins/O-Lyrica/Source/DSP/ScaleGenerator.cpp
  - plugins/O-Lyrica/Source/DSP/ScaleGenerator.h
  - plugins/O-Lyrica/Source/DSP/TuningExporter.cpp
  - plugins/O-Lyrica/Source/DSP/TuningExporter.h
  - plugins/O-Lyrica/Source/DSP/EmbeddedTunings.cpp
  - plugins/O-Lyrica/Source/DSP/EmbeddedTunings.h
  - plugins/O-Lyrica/Source/DSP/GlissandoController.cpp
  - plugins/O-Lyrica/Source/DSP/GlissandoController.h
  - plugins/O-Lyrica/Resources/ui/index.html
  - plugins/O-Lyrica/Resources/ui/js/app.js
  - plugins/O-Lyrica/Resources/ui/css/styles.css
  - plugins/O-Lyrica/CMakeLists.txt
findings:
  critical: 7
  warning: 12
  info: 18
  total: 37
status: issues_found
---

# O-Lyrica v2.3.0: Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: [A] voice / waveguide / pluck-excitation / stiffness-dispersion / string-material · [B] body resonance / sympathetic feedback bank / EQ / reverb / delay / processor host glue · [C] editor / WebView bridge / Scala-KBM tuning engine / scale generator / exporter / glissando / build)
**Files Reviewed:** 39 (35 source + index.html/app.js/styles.css + CMakeLists.txt)
**Commit reviewed:** current HEAD (O-Lyrica clean/committed at v2.3.0, commit `e695256`)
**Status:** issues_found

> **Version-doc drift (fix alongside):** the CHANGELOG top entry is **v2.3.0** (2026-04-24, note-expression module adoption) and `CMakeLists.txt`/git agree, but `PLUGINS.md` still lists O-Lyrica at **2.2.2**. Bump PLUGINS.md to 2.3.0.

## Summary

O-Lyrica is a physical-modeling harp synth: a digital-waveguide string with a stiffness/dispersion
allpass chain, a plucked-string comb-filter excitation, string-material morphing, a 5-mode body
resonator, a sympathetic-string feedback bank, EQ/reverb/delay/chorus effects, Scala/KBM microtonal
tuning (with VST3 Note Expression — **O-Lyrica is this project's validated NE reference/spike**), a
glissando controller, and a self-contained WebView UI. **The waveguide core is well-hardened** —
`StiffnessFilter` flushes both state and output and clamps its allpass coefficients away from the
`(1+a)=0` singularity, the loop feedback gain is provably sub-unity, per-note reset hygiene is
thorough, and the master `processBlock` opens with `ScopedNoDenormals`. Preset state-reset,
name-sanitization, factory `decayTime` skew, Windows WebView flags, the resource provider, and the
single-`BinaryData`-namespace build are all **correct**.

The defects cluster in five high-value, recurring areas:

1. **Audio-thread heap allocation on IIR coefficient rebuilds** — the single most pervasive defect.
   `grep` finds **zero** `ArrayCoefficients` usage anywhere in the plugin; instead EQ, BodyResonance,
   SympatheticResonance, and PluckExciter all call `Coefficients::makeXXX` (heap) on the audio thread,
   plus `StringMaterial` builds a `juce::String` on the render path. (CR-03..07)
2. **WebView teardown UAF** across all 7 `FileChooser::launchAsync` sites (no `SafePointer`). (CR-01)
3. **`loadEmbeddedTuning` drops the tuning period** → all 24 factory library tunings silently
   mistuned; a one-line fix with very high impact. (CR-02)
4. **UI ↔ parameter / readout drift** — one dead knob (`stringCrosstalk`), a broken double-click
   reset that pushes `NaN`, and knob readouts off by up to 24 dB. (WR-06..08)
5. **Sample-rate-dependent stability** — filter cutoffs clamped in absolute Hz go unstable/NaN at
   ≤ 40 kHz, the delay buffer overflows above 96 kHz, and the rail delay is unbounded-below so the
   top octave dies on dark patches. (WR-01, WR-03, WR-04)

### Handled correctly (recurring failure modes this codebase gets right)

- **`ScopedNoDenormals` is the first statement of `processBlock`** (PluginProcessor.cpp:695) — body/
  sympathetic/reverb tails are denormal-protected on the master path. ✓
- **`getLatencySamples()` NOT overridden** and no lookahead → 0 latency is correct for JUCE 8; only
  `getTailLengthSeconds()` is overridden. ✓
- **`applyPresetJson` resets ALL params to defaults before applying** (OuariconPresetManager.h:296-300)
  — partial/older presets do not inherit stale state (`pattern_preset_apply_needs_reset_to_defaults`). ✓
- **Preset name "/" is sanitized** — `sanitizePresetName()` replaces `/ \ :` with `_` at every filename
  site (OuariconPresetManager.h:199-202) (`critical_preset_name_slash_path_separator`). ✓
- **Factory `decayTime` (skew 0.4) authored to musical seconds** via `convertFrom0to1` — NOT affected
  by `pattern_factory_preset_normalized_ignores_skew` (only `sympatheticQ` is — see WR-05). ✓
- **Both Windows WebView flags present** — `NEEDS_WEBVIEW2 TRUE` (CMakeLists.txt:16) +
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (:109); `withUserDataFolder(tempDirectory)`
  (PluginEditor.cpp:119). No blank-UI-on-Windows. ✓
- **Resource provider compares BARE paths** (`url == "/" || url == "/index.html"`,
  PluginEditor.cpp:1194-1226) — no "Frame load interrupted." ✓
- **Single `juce_add_binary_data` target** (`OLyricaBinaryData`, CMakeLists.txt:88) — no namespace
  collision (note-expression module is header-only). ✓
- **Editor destruction order correct** — relays → webView → attachments declared, so attachments
  reverse-destruct before the webView. ✓
- **`Juce` ES-module namespace, not `window.__JUCE__`** — `import * as Juce` + `Juce.getNativeFunction`;
  `window.__JUCE__` used only for `backend.addEventListener` (`critical_juce_webview_namespace_vs_postmessage`). ✓
- **Waveguide stability nets:** `StiffnessFilter` flushes `z1` + output `< 1e-15` and clamps allpass
  coeffs to `[-0.9, 0.9]`; loop `feedbackCoefficient` clamped `[0.9, 0.99999]`; per-note `trigger()`
  resets both rails, all filters + shadow copies, stiffness, exciter, and buzz state. ✓
- **Reverb feedback provably < 1** (RT60-derived per-sample gain + ±2 tank clip); **sympathetic loop
  uses an explicit sub-unity leaky-integrator gain** (not a one-pole-pole-as-decay — contrast O-Bowed
  WR-05). ✓
- **Effects params ARE cached** as `std::atomic<float>*` (`fxCache` + `crosstalkParam`) — the synth/
  tuning/body reads are not (see WR-09). ✓

---

## Critical (7)

### CR-01 — All 7 FileChooser `launchAsync` completions are UAF-vulnerable on editor teardown (no SafePointer)
- **File:** `Source/PluginEditor.cpp:204, 238, 352, 392, 425, 459, 723`
- **Severity:** Critical (trigger: plugin window closed while a native Save/Load dialog is open)
- **What:** Every dialog completion captures the WebView-owned `complete` callback (and 4 of them also
  capture `this`) with **no `juce::Component::SafePointer` guard**, and calls `complete(...)`
  unconditionally. The four `this`-capturing sites — `savePresetWithDialog` (:204), `loadPresetFromFile`
  (:238), `loadScalaFile` (:352), `loadKBMFile` (:392) — dereference `this->processorRef` inside the
  lambda. The three others — `saveScalaFile` (:425), `saveKBMFile` (:459), `exportTuningHTML` (:723) —
  capture only `complete`.
- **Failure scenario:** The user opens a native dialog, then closes the plugin editor (or the DAW tears
  it down) before choosing. Because `chooser` is captured by value (`shared_ptr`), the `FileChooser`
  outlives the editor and the callback still fires after `~OLyricaAudioProcessorEditor`; `this` is now
  dangling and `complete` is a `std::function` owned by the destroyed `WebBrowserComponent` impl →
  **use-after-free / host crash** (`pattern_webview_launchasync_safepointer_no_complete`). The v1.7.10
  "keep the chooser alive with a shared_ptr" fix addressed the FileChooser's own lifetime but *guarantees*
  the post-teardown fire that this finding is about.
- **Root cause:** No liveness check on the editor/WebView before touching `this` or the WebView-owned
  completion.
- **Fix:** Capture `juce::Component::SafePointer<OLyricaAudioProcessorEditor> safe(this)` in all 7
  completions. On the null path **bail with a bare `return`** — do NOT call `complete(false)` (that is
  itself a UAF against the destroyed WebView impl, per the shipped O-MicrotonalSampler W12 fix). Guard the
  three non-`this` sites on the SafePointer before invoking `complete` as well.

### CR-02 — `loadEmbeddedTuning` drops the period → every factory-library tuning is silently mistuned
- **File:** `Source/PluginEditor.cpp:672` — vs `Source/DSP/EmbeddedTunings.h:30-31`, `EmbeddedTunings.cpp:35`, `Source/DSP/TuningEngine.cpp:254, 1016`
- **Severity:** Critical
- **What:** `EmbeddedTuning::intervals` **excludes** the period (stored separately as `double period`
  — e.g. Young 1799 is 12 values ending `1091.7¢`, period `1200.0`; Bohlen-Pierce is 13 values ending
  `1755.6¢`, period `1901.955`). `loadEmbeddedTuning` calls `setCustomIntervals(tuning->intervals,
  tuning->name)` **without appending `tuning->period`**. `setCustomIntervals` then computes
  `scaleDegrees = size-1` (TuningEngine.cpp:254) and `calculateCustomFrequency` uses
  `scalePeriod = scaleIntervals.back()` (:1016) — the last *degree*, not the octave.
- **Failure scenario:** Loading any library tuning yields the wrong note count **and** wrong repeat
  period: Young 1799 → 11 notes / 1091.7¢; 17-EDO → 16 notes / 1129.4¢; Bohlen-Pierce → 12 notes /
  1755.6¢. All 24 factory tunings are audibly mistuned, while `getEmbeddedTuningList` still reports the
  *correct* period/noteCount to the UI (PluginEditor.cpp:651-652) — so the library panel looks right
  while playback is wrong. Blast radius: a subsequent `setSingleInterval` sees `size()==12` and silently
  re-initializes to 12-TET (TuningEngine.cpp:283), and "Save .scl" exports the period-less scale. Exactly
  `pattern_embedded_tuning_period_dropped` (confirmed O-Formant v1.25.0).
- **Root cause:** The embedded-tuning load path omits the period append that `loadScalaFile`,
  `setBuiltInPreset`, and `applyGeneratedScale` all perform.
- **Fix:**
  ```cpp
  std::vector<double> intervals = tuning->intervals;
  intervals.push_back(tuning->period);          // <-- required
  processorRef.getTuningEngine()->setCustomIntervals(intervals, tuning->name);
  ```
  Verify the resulting `getScaleDegrees()` equals `tuning->intervals.size()`.

### CR-03 — Audio-thread heap allocation: EQProcessor rebuilds 3 shelf/peak `Coefficients` on every automated block
- **File:** `Source/DSP/EQProcessor.cpp:54-56, 62-64, 71-73` (`process`)
- **Severity:** Critical
- **What:** `process()` does `*lowShelf.state = *FilterCoeffs::makeLowShelf(...)`,
  `*midPeak.state = *FilterCoeffs::makePeakFilter(...)`, `*highShelf.state = *FilterCoeffs::makeHighShelf(...)`.
  Each `makeXXX` heap-allocates a `Coefficients<float>::Ptr` (its internal `juce::Array<float>`), and the
  `*state = *(...)` copy-assign frees the previous Ptr — both `new` and `delete` on the audio thread. The
  exact documented O-Formant/O-Bowed EQ regression (`pattern_arraycoefficients_rt_safe_iir`). Runs from
  `processBlock` at PluginProcessor.cpp:951; the `!= prev` guards (:52,60,69) are exceeded on any live
  knob move (0.1 dB / 1 Hz quantization).
- **Failure scenario:** Automating/dragging EQ Low, Mid, Mid-Freq, or High → up to 3 `new` + 3 `delete`
  per block inside `processBlock` → allocator lock / priority inversion → xrun.
- **Root cause:** `Coefficients::makeXXX` (heap) instead of `ArrayCoefficients::makeXXX` (stack) copied
  in place.
- **Fix:** Fill from `juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf/makePeakFilter/makeHighShelf(...)`
  and copy the returned `std::array` element-wise into `lowShelf.state->coefficients` (etc.). Also reset
  the `prev*` cache in `prepare()` so the first post-prepare block does not force a spurious rebuild (IN-07).

### CR-04 — Audio-thread heap allocation: BodyResonance rebuilds 5 `makePeakFilter` coeffs on Size/Wood/Spread change
- **File:** `Source/DSP/BodyResonance.cpp:139-144` (`updateFilterCoefficients`)
- **Severity:** Critical
- **What:** `updateFilterCoefficients()` assigns `bodyModes[i].coefficients =
  juce::dsp::IIR::Coefficients<float>::makePeakFilter(...)` for all 5 modes — 5 heap-allocated Ptrs,
  freeing the prior 5. Call chain is entirely on the audio thread: PluginProcessor.cpp:880
  (`bodyResonance.process(...)`, per sample) → `process()`:89 `applyPendingFilterUpdates()` → :113
  `updateFilterCoefficients()`. The pending flag is set by `setBodyParameters`/`setModeSpread`
  (PluginProcessor.cpp:874-875, also audio thread) whenever `bodySize` moves > 0.001 or wood/spread changes.
- **Failure scenario:** Automating **Body Size**, **Wood Type**, or **Body Mode Spread** → 5 `new` + up
  to 5 `delete` inside `processBlock` per changed block → xrun.
- **Root cause:** `Coefficients::makePeakFilter` (heap) instead of `ArrayCoefficients::makePeakFilter`
  (stack) into pre-allocated coefficient storage.
- **Fix:** Seed one `Coefficients<float>::Ptr` per mode in `prepare()`, then in `updateFilterCoefficients`
  fill from `ArrayCoefficients<float>::makePeakFilter(currentSampleRate, scaledFreq, Q, gain)` copied
  element-wise into `bodyModes[i].coefficients->coefficients`. Bonus: hoist the per-sample
  `applyPendingFilterUpdates` atomic load out of the sample loop (IN-06).

### CR-05 — Audio-thread heap allocation: SympatheticResonance designs a `makeBandPass` filter on every note-on
- **File:** `Source/DSP/SympatheticResonance.cpp:452-458` (`designResonatorFilter`)
- **Severity:** Critical
- **What:** `designResonatorFilter` does `auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(...)`
  (heap Ptr) then `*filter.coefficients = *coefficients`. Called from **`registerVoice`** (:122, :152) —
  which runs inside `HarpSynthVoice::startNote` (HarpSynthVoice.cpp:264) on the audio thread during
  `synthesiser.renderNextBlock` at **every MIDI note-on** — and from **`syncBeforeBlock`** (:219) on the
  audio thread whenever Sympathetic Sharpness (`sympatheticQ`) is automated (re-designs the filter for
  every active slot).
- **Failure scenario:** Any note-on (or Sharpness automation with N voices held) → 1× (or N×) `new`+`delete`
  on the RT thread → xrun on dense passages / glissandi.
- **Root cause:** `Coefficients::makeBandPass` (heap) instead of `ArrayCoefficients::makeBandPass` (stack).
- **Fix:** `auto arr = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(sampleRate, clampedFreq, Q);`
  copy `arr` element-wise into `filter.coefficients->coefficients` (pre-allocated by `filter.prepare`),
  then `filter.reset()`. No alloc in `registerVoice`/`syncBeforeBlock`.

### CR-06 — Audio-thread heap allocation: PluckExciter rebuilds `Coefficients` on every note-on & technique change
- **File:** `Source/DSP/PluckExciter.cpp:263` (brightness), `:275, 290, 297, 303` (technique)
- **Severity:** Critical
- **What:** `brightnessFilter.coefficients = Coefficients<float>::makeFirstOrderLowPass(...)` and the four
  `techniqueFilter.coefficients = ...make{FirstOrderLowPass,PeakFilter,HighShelf}(...)` each heap-allocate
  a `Coefficients<float>` and copy-assign into the filter's `Ptr` (second alloc + free of prior Ptr). Call
  chain is entirely on the audio thread: `HarpSynthVoice::startNote` (from `synthesiser.renderNextBlock`,
  PluginProcessor.cpp:804) → `stringModel.setTechnique` (HarpSynthVoice.cpp:214) → `updateTechniqueFilter`
  (1 alloc), **and** → `stringModel.trigger` (HarpSynthVoice.cpp:259) → `exciter.trigger`
  (PluckExciter.cpp:92) → `updateBrightnessFilter` + `updateTechniqueFilter` (2 more). ≈3 alloc/free pairs
  per note-on.
- **Failure scenario:** A dense passage or high polyphony triggers ~3 allocations per note-on on the RT
  thread → allocator lock / priority inversion / page fault → xrun.
- **Root cause:** Constructing fresh `Coefficients` objects instead of writing stack coefficients into
  pre-allocated storage.
- **Fix:** Seed one `Coefficients<float>::Ptr` for `brightnessFilter` and one for `techniqueFilter` in
  `prepare()`; in the update functions fill from `ArrayCoefficients<float>::makeFirstOrderLowPass/
  makePeakFilter/makeHighShelf(...)` copied element-wise into the existing `->coefficients` array.

### CR-07 — Audio-thread `juce::String` construction: `StringMaterial::fromType`/`interpolate` build the material name on the render path
- **File:** `Source/DSP/StringMaterial.cpp:26, 35, …, 120`; callers `Source/HarpSynthVoice.cpp:163, 582, 596`
- **Severity:** Critical
- **What:** `StringMaterial` carries a `juce::String name` (StringMaterial.h:48). `fromType()` assigns a
  string literal (a non-empty `juce::String` heap-allocates its ref-counted buffer), and `interpolate()`
  does `result.name = name + " → " + other.name + " (" + juce::String(percentage) + "%)"`
  (StringMaterial.cpp:120) — several allocations. Both run on the audio thread: `fromType` at every note-on
  (HarpSynthVoice.cpp:163) and on any material change (:582); `interpolate` **every block** for the ~50 ms
  of a material crossfade (:596, inside `updateParametersFromAPVTS` → `renderNextBlock`) while `0.01 < t < 0.99`.
- **Failure scenario:** Changing/automating `stringMaterial` produces a burst of per-block `String`
  allocations for 50 ms on the RT thread; each note-on adds one more → xrun (the banned "String construction
  on the audio path" case). The DSP never reads `name`.
- **Root cause:** `StringMaterial` doubles as the DSP parameter carrier and the UI display struct; the
  cosmetic `name` is dragged onto the audio thread.
- **Fix:** Keep `name` off the render path — give `fromType` a DSP-only overload that skips `name`, or drop
  `name` and resolve the display name lazily on the message thread via the existing `getNameFromType()`. In
  `interpolate`, do not build the mixed-name string (compute it only for UI, off-thread).

---

## Warning (12)

### WR-01 — Filter cutoffs clamped in absolute Hz exceed Nyquist at sample rates ≤ 40 kHz → unstable / NaN one-pole coefficients inside the feedback loop
- **File:** `Source/DSP/WaveguideString.cpp:484` (bridge `jlimit(300,20000)`), `:488` (nut `jlimit(1000,20000)`), `:495` (damping `jlimit(200,14000)`); `OnePoleLPF::setCutoff` `WaveguideString.h:207-215`
- **Severity:** Warning (Critical at the affected sample rates)
- **What:** `setCutoff` computes `n = tan(π·cutoff/sampleRate)`. The cutoffs are clamped to **absolute** Hz,
  not a fraction of Nyquist. Crystal material (`brightnessCutoff = 16000`, StringMaterial.cpp:76) with high
  brightness/tension saturates the bridge/nut clamp at 20 kHz. At `sampleRate = 32000`, Nyquist is 16 kHz,
  so `tan(π·20000/32000)` is negative → `a1` magnitude > 1 → the pole leaves the unit circle. At
  `sampleRate = 40000`, cutoff 20000 = Nyquist → `tan(π/2)=∞` → `a0=∞`, `b0=NaN`. Either way a broken/
  unstable filter sits inside the waveguide feedback loop.
- **Failure scenario:** Bright patch (Crystal / high brightness / high tension) at 32 kHz (or 40 kHz) →
  unstable/NaN bridge/nut filter → runaway or NaN in the loop → note blows up or silences (see WR-02).
  Silent at 44.1/48 kHz.
- **Root cause:** Cutoff clamps are in Hz, independent of `sampleRate`; no Nyquist ceiling.
- **Fix:** Clamp all three cutoffs to `jmin(clampHi, 0.45f * sampleRate)` inside `calculateFilterCutoffs`.

### WR-02 — No finite guard in the waveguide loop; `trigger()` accepts an unvalidated frequency, and the denormal flush neither covers the feedback push nor catches NaN
- **File:** `Source/DSP/WaveguideString.cpp:69` (`trigger` sets `currentFrequency = frequency` unguarded), `:227-228` (feedback push), `:235-236` (flush), `:415` (`setFrequency` *does* guard `20 < f < 20000`)
- **Severity:** Warning (Critical if the tuning engine can emit 0/NaN)
- **What:** `trigger()` stores `frequency` with no range/finite check, unlike `setFrequency()`.
  `calculateRailDelay()` then computes `sampleRate / currentFrequency`; a NaN/0 frequency yields a NaN/Inf
  rail delay, and `DelayLine::setDelay` does `jlimit(0, upper, x)` which **passes NaN through unchanged**
  (juce_DelayLine.cpp:62). `delayInt = (int) std::floor(NaN)` is UB → garbage index → `interpolateSample`
  reads out of bounds. Separately, the only per-sample stability net is the `< 1e-15f` denormal flush at
  :235, which (a) is applied to the *read* `output`, not to `nutReflection`/`bridgeReflection` pushed back
  into the rails at :227-228, and (b) `std::abs(NaN) < 1e-15` is false, so NaN is never cleared. Once NaN
  enters the loop it persists → `currentEnergy` NaN → `isActive()` false → `clearCurrentNote()` kills the note.
- **Failure scenario:** A malformed Scala scale or out-of-range degree makes `tuningEngine->getFrequency()`
  return 0/NaN → `startNote` passes it into `trigger` → NaN rail delay → OOB read / note silenced.
- **Root cause:** `trigger` trusts its frequency argument; the loop has no `isfinite` recovery and the
  denormal flush is misplaced (output, not feedback) and cannot catch NaN.
- **Fix:** In `trigger()`: `frequency = juce::jlimit(20.0, 20000.0, std::isfinite(frequency) ? frequency :
  440.0)` before use. Add a per-sample finite guard at the push boundary that resets the **source**:
  `if (!std::isfinite(nutReflection) || !std::isfinite(bridgeReflection)) { reset(); exciter.reset();
  return 0.0f; }`.

### WR-03 — Rail delay can go to zero for high notes (no floor) when filter group delay exceeds the string period → dead/detuned top octave
- **File:** `Source/DSP/WaveguideString.cpp:566-569` (`calculateRailDelay`, no `jmax`); group delay `:585-587`
- **Severity:** Warning
- **What:** `compensatedDelay = totalDelay - filterGroupDelay`, then `return compensatedDelay * 0.5f` with
  no lower bound. `filterGroupDelay` sums `sampleRate/(2π·cutoff)` for bridge/nut/damping using the
  *minimum* clamped cutoffs (300/1000/200 Hz). For a dark patch at f = 3000 Hz, 44.1 kHz:
  `totalDelay ≈ 14.7`, but bridge ≈ 23.4 + nut ≈ 7.0 + damping ≈ 35.1 ≈ 65 samples → `compensatedDelay ≈
  −50` → rail ≈ −25 → `DelayLine::setDelay` clamps to 0. A rail delay of 0 short-circuits the loop — the
  string cannot resonate at pitch.
- **Failure scenario:** High notes on a dark/low-brightness patch are silent or badly detuned; worsens as
  brightness/tension are lowered.
- **Root cause:** Group-delay compensation subtracts an unbounded amount with no floor on the result.
- **Fix:** `return juce::jmax(2.0f, compensatedDelay * 0.5f);` (≥ 2 keeps Lagrange3rd valid), or cap
  `filterGroupDelay` at a fraction of `totalDelay`.

### WR-04 — Delay line fixed at 192000 samples → wrong (aliased) delay time for `delayTime` near max above 96 kHz
- **File:** `Source/DSP/DelayProcessor.h:29-30` (`DelayLine {...} { 192000 }`); `.cpp:45` (`setTime`), `:87-88` (`popSample`); param range `PluginProcessor.cpp:430` (`delayTime` 0.001–2.0 s)
- **Severity:** Warning
- **What:** `delayL/delayR` are constructed with `maximumDelayInSamples = 192000` and `prepare(spec)` never
  updates it. `setTime` sets `delaySamples = seconds * sampleRate` (up to `2.0 * sr`). At `sr > 96 kHz`,
  `2.0*sr > 192002`; `setDelay` (DelayLine.cpp:60) only `jassert`s the bound (no-op in Release) and reads a
  modulo-wrapped index → **wrong (aliased, too-short) delay time**, not a crash.
- **Failure scenario:** A 176.4 kHz / 192 kHz session with Delay Time near 2 s → delay time silently
  collapses to `(delaySamples mod 192002)/sr`.
- **Root cause:** Compile-time-constant buffer size assumes ≤ 96 kHz.
- **Fix:** In `prepare()` call `delayL.setMaximumDelayInSamples((int) std::ceil(2.0 * spec.sampleRate) + 4)`
  (and R), or clamp `delaySamples` to `maximumDelayInSamples` in `setTime`.

### WR-05 — `sympatheticQ` factory-preset values (skew 0.5) collapse to Q ≈ 0.12–0.75 for all 48 presets
- **File:** `Source/PluginProcessor.cpp:382-386` (param: range 0.1–20, **skew 0.5**, default **5.0**) vs factory table values `sympatheticQ ∈ {0.03…0.18}` (e.g. `:1150, 1166, 1181, 1197`)
- **Severity:** Warning
- **What:** Factory `FactoryPresetDef` floats are applied verbatim as **normalized** values
  (OuariconPresetManager.h:602-604 → `applyPresetJson:306` `setValueNotifyingHost`). For the skewed
  `sympatheticQ`, `convertFrom0to1(p) = 0.1 + 19.9·p^(1/0.5) = 0.1 + 19.9·p²`. So authored `0.03…0.18` map
  to **Q 0.12…0.75** — every one of the 48 factory presets pins "Sympathetic Sharpness" below 1.0, an order
  of magnitude under the 5.0 default, and never reaches the moderate/sharp region. The
  `pattern_factory_preset_normalized_ignores_skew` signature. (`decayTime`, the other skewed preset param,
  coincidentally lands on musical seconds and is fine.)
- **Failure scenario:** All factory content sounds uniformly diffuse; the Sharpness control's upper ~95 %
  is unused by any preset.
- **Root cause:** Skewed param authored as bare fractions without accounting for the 0.5 skew.
- **Fix:** Author `sympatheticQ` in engineering Q units and store `NormalisableRange::convertTo0to1(Q)`
  (Q=3 → 0.382, not 0.03). Verify the intended Q per preset with the sound designer.

### WR-06 — `stringCrosstalk` is a real audible DSP parameter with no UI relay/attachment/control (unreachable from the editor)
- **File:** `Source/PluginProcessor.cpp:459` (param, default 0.2), `:537` (`crosstalkParam` DSP cache) — vs `Source/PluginEditor.cpp` (0 occurrences) and `Resources/ui/*` (0 occurrences)
- **Severity:** Warning
- **What:** `stringCrosstalk` (NormalisableRange 0-1, default 0.2) is created in `createParameterLayout`,
  cached into a raw parameter pointer in `prepareToPlay`, but has **no `WebSliderRelay`, no attachment, no
  PARAMS entry, and no HTML knob** (verified 0 occurrences).
- **Failure scenario:** A shipped, non-zero-default (0.2), audible parameter the user cannot reach from the
  UI. Still recalled by presets/automation, so it silently participates in the sound with no way to view or
  edit it. (Class of `pattern_activating_dead_param_default_timbre` in reverse — it's already active.)
- **Root cause:** UI/param drift — added to the DSP layout, never surfaced in the editor/HTML.
- **Fix:** Add a relay + `.withOptionsFrom` + attachment + HTML knob (mirror `stringTension`), or, if
  intentionally hidden, document it and consider marking it non-automatable. Do not leave a shipped audible
  param with no surface.

### WR-07 — Double-click "reset to default" reads a non-existent `properties.defaultValue` → sends NaN
- **File:** `Resources/ui/index.html:2934` — vs `Resources/ui/js/juce/index.js:147-156` and JUCE `juce_ParameterAttachments.cpp:299-313`
- **Severity:** Warning
- **What:** `slider.addEventListener('dblclick', () => sliderState.setNormalisedValue(sliderState.properties.defaultValue));`.
  The WebSlider `properties` object contains only `{start,end,skew,name,label,numSteps,interval,parameterIndex}`
  — the JUCE 8.0.9 `propertiesChanged` payload emits **no `defaultValue`**. So `properties.defaultValue` is
  `undefined` → `setNormalisedValue(undefined)` → `NaN` → emits `{value: NaN}` to the backend.
- **Failure scenario:** Double-clicking any of the ~24 sliders (masterVolume, brightness, decayTime,
  sympatheticQ, all glissando sliders, …) does not reset; it pushes a NaN scaled value to the parameter
  (`pattern_webview_knob_readout_scaled_value`, "properties has no default" trap). (The A4 knob's hard-coded
  `setNormalisedValue(0.5)` at :4184 is correct only because 440 Hz is the true midpoint of 400-480.)
- **Root cause:** Assumes a `defaultValue` field the JUCE bridge does not provide.
- **Fix:** Register a `getParameterDefaults` native fn returning each parameter's normalised
  `getDefaultValue()`, cache in JS, reset via `setNormalisedValue(defaults[id])`.

### WR-08 — Knob readouts use hard-coded per-knob math instead of `getScaledValue()`; several drift badly from the real NormalisableRange
- **File:** `Resources/ui/index.html:2861` (masterVolume), `:2864-2868` (decayTime), `:2876-2880` (sympatheticQ), `:4141` (A4 REF) — vs `Source/PluginProcessor.cpp:22, 51, 302, 384`; correct API at `Resources/ui/js/juce/index.js:226` (`getScaledValue()`)
- **Severity:** Warning
- **What:** Each readout formats `getNormalisedValue()` through bespoke JS math that mismatches the C++
  `NormalisableRange`:
  - **masterVolume:** JS `(v*48)-36` displays −36…+12 dB, real range is **−60…+6 dB** (PluginProcessor.cpp:22).
    At norm 0.5 reads −12 dB vs actual −27 dB. Wrong across the whole travel.
  - **A4 REF:** JS `415 + v*50` displays 415…465 Hz, masterTune range is **400…480 Hz** (:302); only the
    440 Hz center matches.
  - **sympatheticQ:** JS `0.1 + v*19.9` (linear) vs C++ **skew 0.5** (:384) → ~2× wrong at midpoint (Q 10.1
    shown vs ~5.1 actual).
  - **decayTime:** JS `0.1 + v²*19.9` vs C++ **skew 0.4** (:51) → ~40% high at midpoint (5.1 s vs 3.6 s).
  (`glissandoSpeed`/`glissandoTime` happen to match because their JS mirrors the skew.)
- **Failure scenario:** Users read misleading values; master-volume and A4 never reach the true endpoints
  and are off by up to 24 dB / 15 Hz.
- **Root cause:** Hard-coded JS ranges/skews drift from the C++ `NormalisableRange`.
- **Fix:** Format directly from `sliderState.getScaledValue()` (skew-aware), reducing each `format` to unit
  labeling/rounding. Reusable `ui_frontend_check.js` shipped with O-MicrotonalSampler v1.23.7.

### WR-09 — Per-block/per-sample APVTS string-keyed lookups on the audio thread (voice + processor), including a per-sample `"timbre"` lookup inside the glissando loop
- **File:** `Source/HarpSynthVoice.cpp:545-567` (~10/block) & `:646` (per-sample during glissando); `Source/PluginProcessor.cpp:709-955` (15/block) + `:990,992`
- **Severity:** Warning
- **What:** `updateParametersFromAPVTS` performs ~10 `getRawParameterValue("id")->load()` hashed-string map
  lookups every block per voice (+ ~20 more per note-on in `startNote`); `processBlock` performs 15 more for
  the synth/tuning/body/master params. Worst case: during an active glissando the `"timbre"` parameter is
  looked up by string **every sample** inside the `while (--numSamples >= 0)` loop (HarpSynthVoice.cpp:646).
  This violates the mandated "cache `std::atomic<float>*` in `prepareToPlay`" pattern (the effects chain
  `fxCache` already follows it). Same class as O-Bowed WR-06.
- **Fix:** Resolve `std::atomic<float>*` members once in `prepare()`/`setAPVTS` and in the constructor/
  `prepareToPlay` (mirror `EffectsParamCache`); read `->load()` in the loops. Hoist the glissando `"timbre"`
  read out of the per-sample loop (constant across the block).

### WR-10 — Per-block `juce::MidiBuffer` construction allocates on the audio thread
- **File:** `Source/PluginProcessor.cpp:746` (`juce::MidiBuffer filteredMidi;`), `:786` (`filteredMidi.addEvent(...)`)
- **Severity:** Warning
- **What:** A fresh local `MidiBuffer` is constructed every block with zero capacity; the first `addEvent`
  grows its internal `Array<uint8>` via `insertMultiple` → heap allocation on the RT thread on every block
  that carries MIDI (MidiBuffer.cpp:155). `MidiBuffer::clear()` is `data.clearQuick()` (retains capacity),
  so a reused member buffer would allocate only during warm-up.
- **Failure scenario:** Any block with note/keyswitch events → per-block allocation → jitter/xrun risk.
- **Root cause:** Local (0-capacity) `MidiBuffer` re-created each block instead of a reused member.
- **Fix:** Make `filteredMidi` a member; `filteredMidi.clear()` at the top of the keyswitch loop, or
  `ensureSize()` once in `prepareToPlay`.

### WR-11 — `triggerNoteOn/Off` invoke `registerVoice`/`unregisterVoice` on the message thread → data race with audio-thread `syncBeforeBlock`
- **File:** `Source/PluginProcessor.cpp:1021` (`triggerNoteOn`→`synthesiser.noteOn`), `:1033` (`triggerNoteOff`); races `:740` (`sympatheticEngine.syncBeforeBlock()`)
- **Severity:** Warning (narrow trigger: on-screen-keyboard use while audio runs)
- **What:** `triggerNoteOn/Off` are called from the WebView/message thread and synchronously run
  `HarpSynthVoice::startNote/stopNote` → `SympatheticResonanceEngine::registerVoice/unregisterVoice`, which
  write `VoiceSlot.frequency/materialCoupling/lastSample/energyDecay` and call `designResonatorFilter`
  (mutating the ref-counted `resonatorFilter.coefficients`). `synthesiser.noteOn` takes the Synthesiser
  lock, but `processBlock` calls `sympatheticEngine.syncBeforeBlock()` **outside** that lock (:740, before
  `renderNextBlock`); `rebuildCouplingMatrix` reads those same `VoiceSlot` fields on the audio thread →
  concurrent read/write with no synchronization.
- **Failure scenario:** Click the on-screen keyboard while notes sound → torn `frequency`/`materialCoupling`
  reads (mistuned/mis-weighted coupling) or a race on the `resonatorFilter.coefficients` Ptr refcount → rare
  corruption/crash.
- **Root cause:** The engine's mutators are designed for the audio thread (via `startNote` under the synth
  lock), but the message-thread UI keyboard reaches them while `syncBeforeBlock` runs lock-free.
- **Fix:** Route `triggerNoteOn/Off` through a lock-free MIDI FIFO consumed inside `processBlock` (like the
  existing `MidiEventQueue`), so all `registerVoice`/`syncBeforeBlock` access stays on the audio thread; or
  take the synthesiser lock around `syncBeforeBlock`.

### WR-12 — No `ScopedNoDenormals` on the voice render path; nut/loop-damping feedback not manually flushed
- **File:** `Source/HarpSynthVoice.cpp:613` (`renderNextBlock`); `Source/DSP/WaveguideString.cpp:118` (`processSample`)
- **Severity:** Warning
- **What:** Neither `renderNextBlock` nor `processSample` declares a `ScopedNoDenormals` (grep finds it only
  on the master path, PluginProcessor.cpp:695). Inside the loop, `bridgeReflection` is flushed by
  `StiffnessFilter`, but `nutReflection = -nutFilter.processSample(lowerOut)` (an `OnePoleLPF` with no
  internal flush) is pushed straight into `upperRail` at :227 with no flush — the upper-rail feedback path
  can spin denormals during long decays. Works in-plugin only because the master `ScopedNoDenormals` sets
  FTZ/DAZ globally; the voice is unprotected in isolation (the render harness compiles the voice without the
  processor).
- **Fix:** Add `const juce::ScopedNoDenormals noDenormals;` at the top of `renderNextBlock` (and/or
  `processSample`).

---

## Info (18)

### IN-01 — BodyResonance biquad bank has no NaN/Inf guard (sticky-silence hazard)
- **File:** `Source/DSP/BodyResonance.cpp:92-97` — a transient NaN from the synth/crosstalk path latches all 5 `bodyModes` biquad states to NaN permanently (`pattern_biquad_nan_guard_sticky_silence`). Add `if (!std::isfinite(resonantOutput)) { reset(); resonantOutput = 0.0f; }` before mixing. Complements WR-02's source reset.

### IN-02 — FDN reverb / delay / sympathetic feedback states have no NaN guard; a NaN latches forever
- **File:** `Source/DSP/ReverbProcessor.cpp:421-424` (the `> 2.0f`/`< -2.0f` clamp is false for NaN); `Source/DSP/DelayProcessor.cpp:90-91` (`feedbackL/R` latch NaN); `Source/DSP/SympatheticResonance.cpp:319-322` (`lastSample` accumulator). Defense-in-depth: flush the feedback state to 0 when `!std::isfinite`.

### IN-03 — Sympathetic leaky-integrator has very high DC gain (up to ~5000×)
- **File:** `Source/DSP/SympatheticResonance.cpp:319` — steady-state gain of `lastSample = energyDecay·lastSample + drive` is `1/(1-energyDecay)` = 200×…5000× for `energyDecay` 0.995…0.9998. The soft clip (:331-334) protects the *output* but not the integrator *state*, so DC in `voiceOutput` balloons the internal state. Add a DC-blocker on `voiceOutput` or clamp `lastSample`.

### IN-04 — `setFrequency` silently no-ops outside 20–20000 Hz → sticky glissando
- **File:** `Source/DSP/WaveguideString.cpp:415` — the `if (frequency > 20 && frequency < 20000)` guard leaves the string at its *previous* pitch instead of clamping to the boundary, so an extreme gliss can stick. Clamp with `jlimit` rather than skipping the update.

### IN-05 — Heavy per-sample recompute during glissando
- **File:** `Source/HarpSynthVoice.cpp:642, 647` → `WaveguideString::setFrequency` (`pow` ×3 + a 4-stage group-delay loop) and `setDamping` → `updateFilters` are called **every sample** while gliding; `setDamping`'s per-sample `updateFilters` also restarts the 64-sample filter crossfade every sample. Not incorrect, but a large avoidable CPU load — update at a decimated rate (every 8–16 samples) during glissando.

### IN-06 — Deferred-atomic "thread-safe" machinery is redundant here and adds a per-sample atomic load
- **File:** `Source/DSP/BodyResonance.cpp:89` / `Source/DSP/SympatheticResonance.cpp:207-238` — `setBodyParameters`/`setModeSpread`/`setIntensity`/`setResonatorQ` are all called from `processBlock` (audio thread), so the pending-atomic/double-buffer apparatus never crosses threads (the real cross-thread path is WR-11, which it does not cover). `BodyResonance::process` runs `applyPendingFilterUpdates()` (an acquire load) once per output sample (:89) — hoist to once-per-block.

### IN-07 — `releaseResources()` is a no-op; EQProcessor `prev*` cache not reset in `prepare` → one redundant coeff rebuild per prepare
- **File:** `Source/PluginProcessor.cpp:688-691`; `Source/DSP/EQProcessor.h:47-50`. `prepareToPlay` re-prepares/resets the sympathetic engine, body filters, crosstalk state, reverb, and chorus (no startup transient in practice), but `EQProcessor::prepare` never resets its `prev*` cache → one redundant coeff rebuild (a heap alloc, per CR-03) on the first block after every prepare. Reset `prev*` in `prepare()` and/or call `eq.reset()` from `prepareToPlay`.

### IN-08 — `isRestoringState` guard is incomplete
- **File:** `Source/PluginProcessor.cpp:718-725` vs custom-load callback `:575-632`. The flag gates only `tuningEngine.setMode` in `processBlock`; `setMasterTune` (:713), `setPitchBendRange` (:714), `setOctaveStretch` (:725), and the voices' `getFrequency` still run on the audio thread while `setStateInformation` (message thread) calls `setBuiltInPreset/setCustomIntervals/setTonicNote/setOctaveStretch`. If `TuningEngine::setCustomIntervals` mutates a container the audio thread reads, this is a race — verify TuningEngine thread-safety.

### IN-09 — `rebuildCouplingMatrix` reads voice `active` with `memory_order_relaxed`
- **File:** `Source/DSP/SympatheticResonance.cpp:255, 267` — `active.load(relaxed)` is not paired with the `release` store in `registerVoice:155`. Same-thread today, but if `registerVoice` ever runs cross-thread (WR-11), the preceding `frequency`/`materialCoupling` writes are not guaranteed visible even after `active` reads true. Use `memory_order_acquire`.

### IN-10 — Stiffness group-delay math duplicated (drift hazard)
- **File:** `Source/DSP/WaveguideString.cpp:596-619` re-implements `StiffnessFilter::updateCoefficients` + `calculateFrequencyScaling` to estimate group delay. Any tweak to the real filter must be mirrored by hand or pitch compensation silently drifts. Factor the coefficient calc into a shared helper on `StiffnessFilter` and query it.

### IN-11 — Dead member `HarpSynthVoice::currentVelocity`
- **File:** `Source/HarpSynthVoice.h:132`, assigned `Source/HarpSynthVoice.cpp:110`, never read (velocity is passed directly to `stringModel.trigger`). Remove.

### IN-12 — Dead member `PluckExciter::pluckVelocity`
- **File:** `Source/DSP/PluckExciter.h:135`; assigned/scaled at `Source/DSP/PluckExciter.cpp:58, 80` but never read — `process()` drives output from `burstAmplitude`. The glissando `pluckVelocity *= velScale` at :80 is a no-op. Remove the member and the dead scaling.

### IN-13 — `Resources/ui/css/styles.css` is orphaned dead code
- **File:** `Resources/ui/css/styles.css` (296 lines) — not in `juce_add_binary_data` (removed v2.1.6, CMakeLists.txt:86-87), not `<link>`ed in index.html, not served by `getResource`. CSS is inlined in index.html. Delete the file.

### IN-14 — Five native functions are registered but never called by the JS
- **File:** `Source/PluginEditor.cpp:135` (`savePreset`), `:301` (`setSingleInterval`), `:510` (`getTemperamentPreset`), `:692` (`getEmbeddedTuningCategories`), `:753` (`getTooltipsEnabled`). JS uses `savePresetWithDialog` / `setSingleIntervalEncoded` instead, and restores tooltip state via `evaluateJavascript`. Remove or wire.

### IN-15 — `temperamentPreset` has no relay/attachment; driven only by the `setTemperamentPreset` native fn
- **File:** `Source/PluginProcessor.cpp:333` (param) vs `Source/PluginEditor.cpp:482` (native-fn setter only). The choice param is set through `setValueNotifyingHost` from a native fn and never bound via a `WebComboBoxParameterAttachment`, so the UI will not reflect **host automation** of it. Functional today, but inconsistent with every other choice param. Consider a relay/attachment for automation round-trip.

### IN-16 — `getEmbeddedTuningList` builds JSON by unescaped string concatenation
- **File:** `Source/PluginEditor.cpp:647-652` — `"name":"…"`, `"description":"…"` concatenated without escaping `"`/`\`. All 24 factory strings are quote/backslash-free today (no live defect), but any future tuning name containing a `"` emits malformed JSON and breaks `JSON.parse`. Use `juce::JSON`/`DynamicObject` or escape.

### IN-17 — "Tonic" behaves differently between the KBM and linear-mapping paths
- **File:** `Source/DSP/TuningEngine.cpp:351-398` (rotation) vs `:988-1012` (anchor shift). With a KBM loaded, `calculateCustomFrequency` uses `rotatedIntervals` (modal rotation); on the default linear path it ignores rotation and shifts the anchor note (`60 + tonic`, transposition). Also `setTonicNote` clamps to 0-11 (:402), so for scales > 12 degrees only the first 12 are selectable as tonic. Documented as intentional (v1.13.0), but the rotation-vs-transposition split is a latent inconsistency worth a comment or reconciliation.

### IN-18 — Stale effects-chain-order comment
- **File:** `Source/PluginProcessor.h:232` says "Chorus → Delay → EQ → Reverb", but `processBlock` runs **Chorus → Delay → Reverb → EQ** (:889-952, matching the :886 "v2.1.2" comment). Harmless, but the header comment misleads. Reconcile.

---

## Recommended resolution order

1. **CR-01** (UAF crash — highest user-facing risk) — SafePointer + bare `return`, all 7 launchAsync sites.
2. **CR-02** (all 24 factory tunings mistuned — one-line, high impact) — append `tuning->period` before
   `setCustomIntervals`; verify `getScaleDegrees()`.
3. **CR-03 → CR-07** (RT allocation / xrun) — the `ArrayCoefficients`-in-place conversion applies verbatim
   to EQ, BodyResonance, SympatheticResonance, and PluckExciter; move `StringMaterial::name` off the render
   path. `grep -r "ArrayCoefficients" Source/` should be non-empty when done. Reset EQ `prev*` in `prepare`
   (IN-07) as part of CR-03.
4. **WR-01 + WR-02 + IN-01/IN-02** (sample-rate stability + NaN source/body guards) — Nyquist-clamp cutoffs,
   validate `trigger()` frequency, add finite guards that reset the **source**, floor the rail delay (WR-03).
5. **WR-07 + WR-08** (broken dbl-click reset + wrong readouts) — both fixed by adopting
   `sliderState.getScaledValue()` + a `getParameterDefaults` native fn.
6. **WR-04** (HF-rate delay overflow), **WR-05** (`sympatheticQ` skew), **WR-06** (expose or document
   `stringCrosstalk`), **WR-09..WR-12** (cached-pointer discipline, member MidiBuffer, keyboard thread race,
   voice `ScopedNoDenormals`), then the remaining Info items as a cleanup sweep.
7. **Version-doc drift:** bump O-Lyrica to 2.3.0 in `PLUGINS.md`.

Resolve via `/improve-review O-Lyrica` (this file is the completed investigation — root causes and fixes are
prescribed; verify each against source, then apply).
