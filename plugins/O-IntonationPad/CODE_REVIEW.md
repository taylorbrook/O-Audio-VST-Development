---
phase: O-IntonationPad-v2.8.0
reviewed: 2026-07-08
depth: deep
files_reviewed: 26
files_reviewed_list:
  - plugins/O-IntonationPad/Source/PluginProcessor.cpp
  - plugins/O-IntonationPad/Source/PluginProcessor.h
  - plugins/O-IntonationPad/Source/PluginEditor.cpp
  - plugins/O-IntonationPad/Source/PluginEditor.h
  - plugins/O-IntonationPad/Source/PresetManager.cpp
  - plugins/O-IntonationPad/Source/PresetManager.h
  - plugins/O-IntonationPad/Source/DSP/WavetableVoice.cpp
  - plugins/O-IntonationPad/Source/DSP/WavetableVoice.h
  - plugins/O-IntonationPad/Source/DSP/WavetableOscillator.h
  - plugins/O-IntonationPad/Source/DSP/WavetableData.h
  - plugins/O-IntonationPad/Source/DSP/WavetableSound.h
  - plugins/O-IntonationPad/Source/DSP/ChordGenerator.cpp
  - plugins/O-IntonationPad/Source/DSP/ChordGenerator.h
  - plugins/O-IntonationPad/Source/DSP/ScaleGenerator.cpp
  - plugins/O-IntonationPad/Source/DSP/ScaleGenerator.h
  - plugins/O-IntonationPad/Source/DSP/TuningEngine.cpp
  - plugins/O-IntonationPad/Source/DSP/TuningEngine.h
  - plugins/O-IntonationPad/Source/DSP/TuningExporter.cpp
  - plugins/O-IntonationPad/Source/DSP/EmbeddedTunings.cpp
  - plugins/O-IntonationPad/Source/DSP/EQProcessor.cpp
  - plugins/O-IntonationPad/Source/DSP/DelayProcessor.h
  - plugins/O-IntonationPad/Source/DSP/ReverbProcessor.cpp
  - plugins/O-IntonationPad/Source/Util/JsonHelper.h
  - plugins/O-IntonationPad/Source/ui/public/index.html
  - plugins/O-IntonationPad/Source/ui/public/js/tuning-panel.js
  - plugins/O-IntonationPad/CMakeLists.txt
findings:
  critical: 6
  warning: 6
  info: 11
  total: 23
status: issues_found
---

# O-IntonationPad v2.8.0: Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: [A] wavetable voice + chord/scale synthesis · [B] processor + effects + tuning engine + presets · [C] editor/WebView bridge + UI + build). Every finding below was re-verified against source by the orchestrator.
**Files Reviewed:** 26
**Status:** issues_found

## Summary

O-IntonationPad is a wavetable pad synth: dual morphing-wavetable oscillators driving up to 12
chord sub-voices per note, a full microtonal `TuningEngine` (Scala/KBM + built-in temperaments +
VST3 Note Expression for Dorico), a chorus→delay→EQ→reverb effects chain, 12 factory presets, and
a WebView UI with a shared tuning panel.

**The core render path is disciplined and several of this codebase's recurring failure modes are
already handled correctly** (see "Verified correct" below). The defects cluster in four
high-value, recurring areas this project has a documented history with:

1. **Audio-thread allocation / locking** — three separate paths allocate or lock on the audio
   thread (wavetable bank generation, EQ coefficient rebuilds, and tuning-param automation).
2. **WebView knob readouts ignore `NormalisableRange` skew** — ~10 knobs display wrong numbers and
   double-click-edit writes wrong values; Master Volume shows nonsense dB.
3. **WebView FileChooser teardown UAF** — five tuning file dialogs capture raw `this`.
4. **Preset recall non-determinism** — factory presets seed from live state, not defaults.

### Verified correct (not reported — several are prior-fix confirmations)

- `juce::ScopedNoDenormals` is the first statement of `processBlock` (PluginProcessor.cpp:510); no
  `std::vector`/`std::string`/`new` on the steady-state render path — the prior vector→`constexpr
  std::array` fix for `defaultDegrees` held (line 597). ✓
- Audio-thread frequency lookup (`TuningEngine::getFrequency`) is lock-free — atomic
  `frequencyTable` + a release/acquire version fence; `intervalMutex` is never taken from
  `renderNextBlock`. ✓ (It *is* reachable from `parameterChanged` — see CR-05.)
- **Backlog #9 resolved:** `rebuildFrequencyTable` takes `intervalMutex` **once** (TuningEngine.cpp:852)
  and loops `calculateCustomFrequencyUnlocked` — no more 128 lock/unlock cycles. ✓
- **Backlog #10 resolved:** all three voice casts are `static_cast<WavetableVoice*>`
  (PluginProcessor.cpp:480/613/880) — no `dynamic_cast` on the audio thread. ✓
- **Backlog #11 resolved:** the copy-pasted `vector<double>`→JSON loop is now `JsonHelper::arrayToJSON()`
  in `getTuningIntervals`/`generateEDO`/`generateHarmonicSeries`/`generateRank2`. ✓
- **Backlog #12 resolved:** a single shared `ProcessSpec` is used for all DSP modules
  (PluginProcessor.cpp:451) — no duplicate `filterSpec`/`fxSpec`. ✓
- Embedded-tuning load appends the period before `setCustomIntervals` (PluginEditor.cpp:362;
  TuningEngine.cpp:151) — the scala-tuning-engine "dropped period" bug is **not** present. ✓
- Factory-preset *values* are authored in engineering units (cutoff `"3500"`, attack `"2.0"`, lfo
  `"0.15"`) stored as the APVTS denormalized `value` — the skewed-preset recall bug is **avoided**
  (the seeding bug in CR-06 is a separate issue). ✓
- `getLatencySamples()` is **not** overridden; both Windows WebView flags present
  (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`); single
  `juce_add_binary_data` target (no `BinaryData` namespace collision); resource provider compares
  bare paths (`url == "/"`); editor member/destruction order correct (relays → webView →
  attachments). ✓
- Native-function bridge is fully wired — **every** JS `getNativeFunction(...)` resolves to a C++
  `withNativeFunction(...)`; **zero dead controls**. The tuning panel is constructed with the
  ES-module `Juce` namespace, not `window.__JUCE__` (index.html:1943) — no silent-panel bug. ✓
- Wavetable sample indexing wraps last↔first correctly and clamps frame/mipmap bounds; the v2.8.0
  Note-Expression delta is derived from the original noteOn pitch and applied uniformly to every
  sub-voice, so chord intervals stay correct under a microtonal root. ✓

---

## CRITICAL

### [CR-01] Wavetable bank generation can run on the audio thread (22 MB alloc + mutex + additive synthesis)
**`Source/DSP/WavetableData.h:494-512`** (callers `Source/DSP/WavetableVoice.cpp:481-512`, `Source/PluginProcessor.cpp:615-618`)

`BankCache::getBank(idx)` has a fast path (atomic `generated[idx]` → return) but a slow path that
takes `std::mutex mutexes[idx]`, does `std::make_unique<MipmapTable>()` (~22 MB) and runs `fillBank`
(11 mipmaps × 256 frames × 2048-sample additive sine synthesis ≈ millions of `std::sin`).
`WavetableVoice::setWavetableBank`/`Bank2` call `getBank` **unconditionally every block for all 8
voices** (PluginProcessor.cpp:615/617). The pre-warm is fire-and-forget `std::async`
(PluginProcessor.cpp:445) that `prepareToPlay` never waits on.

- **Failure scenario:** Session opens → host starts processing within milliseconds → the async
  pre-warm has not yet generated the current bank → the **audio thread** hits the slow path and
  stalls tens of ms (buffer underrun / audible glitch). Worse: if the pre-warm thread is currently
  generating that same bank, the audio thread blocks on `mutexes[idx]` for the full generation —
  classic priority inversion. Reachable on load and on any bank switch during the ~1 s warm-up.
- **Fix:** Block on the pre-warm before processing (store the future; `preWarmFuture_.wait()` at the
  end of `prepareToPlay` or gate the first `processBlock`), **and/or** have `getBank` return the
  already-ready bank 0 as a fallback instead of generating when the requested bank isn't ready yet.
  Also gate `setWavetableBank` on an actual index change (see IN-03) so the slow path is only
  reachable at all on a genuine switch.

### [CR-02] WebView knob readouts ignore `NormalisableRange` skew — ~10 knobs show wrong values and edits write wrong values
**`Source/ui/public/index.html:2229` (`setupKnob`), knob setup at 1843-1910**

`setupKnob` computes the display as `realValue = min + normValue * (max - min)` (linear) and the
double-click editor inverts it linearly (`norm = (clamped - min)/(max - min)`, line ~2301). It never
calls `state.getScaledValue()`, so every skewed parameter is wrong. Confirmed against the C++ ranges:

- `filterCutoff` (skew **0.25**, 20–20000 Hz): at 50% knob position the readout shows ~10010 Hz but
  the true cutoff is ~1269 Hz (~8× off).
- `lfoRate`/`lfoRate2` (skew 0.3), `attackTime`/`decayTime` (skew 0.3), `releaseTime` (skew 0.4),
  `delayTime`, `chorusRate`, `eqMidFreq`, `reverbPredelay` — all skewed, all read wrong mid-range.
  The four time knobs also display ms while the C++ unit is seconds (endpoints coincide, middle is
  wrong).
- `masterVolume` — **worst case:** the param is linear **gain 0.0–1.26** but the knob is configured
  `min=-60, max=6, ' dB'`. Unity gain (1.0 → norm 0.794) displays **"-7.6 dB"**; typing "0" to get
  0 dB sets `norm=0.909` → **gain 1.145 (≈ +1.2 dB)**.
- **Failure scenario:** User dials in a bright filter by ear; readout says "~10000 Hz" but true
  cutoff is ~4000 Hz. Double-clicks and types "1000" expecting 1 kHz → `setNormalisedValue(0.049)`
  → actual cutoff ≈ 20 Hz (filter nearly shut). Master Volume never shows a believable dB.
- **Fix:** Drive the display from `state.getScaledValue()` (the same API `tuning-panel.js` already
  uses for octave-stretch/master-tune), not `min + norm*(max-min)`. For `masterVolume` display
  `20*log10(getScaledValue())` (' dB'); for the four time knobs apply a `×1000` (' ms') transform.
  The double-click inverse must map display→scaled→normalised (skew-aware). Linear knobs (0–1 → %,
  ±12 dB) are unaffected and can keep the current path. See `pattern_webview_knob_readout_scaled_value`.

### [CR-03] FileChooser `launchAsync` completions capture raw `this`/`complete` → UAF on editor teardown
**`Source/PluginEditor.cpp:215-292, 424-442`** (5 dialogs: `loadScalaFile` 220, `saveScalaFile` 242, `loadKBMFile` 261, `saveKBMFile` 280, `exportTuningHTML` 430)

Every tuning file dialog launches with `[this, complete](const juce::FileChooser& fc){ ... }`. If the
editor is closed while the OS dialog is open, the completion dereferences a destroyed editor
(`processorRef...`) and invokes `complete(...)`, which is owned by the already-destroyed
`WebBrowserComponent` Impl.

- **Failure scenario:** User clicks "Load .SCL", then closes the plugin window (or the DAW closes
  the editor) before picking a file → on dialog dismissal the lambda fires against freed memory →
  crash / heap corruption.
- **Fix:** Capture `juce::Component::SafePointer<OIntonationPadAudioProcessorEditor> safe(this)` and
  at the top of each completion `if (safe == nullptr) return;` — a **bare return**. Do **not** call
  `complete(false)` on the null path (that callback belongs to the dead WebView and is itself a UAF).
  See `pattern_webview_launchasync_safepointer_no_complete` (shipped O-MicrotonalSampler v1.23.5).

### [CR-04] EQ heap-allocates IIR coefficients on the audio thread during automation
**`Source/DSP/EQProcessor.cpp:48-56`** (called from `process()` line 66 → `PluginProcessor.cpp:718`)

`updateCoefficients()` calls `FilterCoeffs::makeLowShelf/makePeakFilter/makeHighShelf`, each of which
returns a ref-counted `Coefficients::Ptr` via `new`. It runs from `EQProcessor::process()` on the
audio thread. The dirty-flag guard (lines 44-46) only suppresses the allocation when *nothing changed*.

- **Failure scenario:** Host automates or the user drags `eqLowGain`/`eqMidGain`/`eqMidFreq`/
  `eqHighGain` → on every block during the ramp the equality guard fails → 3 heap allocations per
  block on the audio thread → priority-inversion-class glitching / dropouts.
- **Fix:** Replace with `juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf/makePeakFilter/
  makeHighShelf(...)` (returns a stack `std::array<float,6>`, no alloc) and copy the 6 values in place
  into `*state`. See `pattern_arraycoefficients_rt_safe_iir` (shipped O-Formant v1.25.1 WR-08).

### [CR-05] Automatable tuning params run `mutex` + `make_shared` + 128×`pow` on the audio thread
**`Source/PluginProcessor.cpp:756-769` (`parameterChanged`) → `Source/DSP/TuningEngine.cpp:201/834/852`**

APVTS dispatches `parameterChanged` **synchronously on the calling thread** (verified: JUCE
`juce_AudioProcessorValueTreeState.cpp:156` `listeners.call(...)` inline). The four tuning params
(`tuning_masterTune`, `tuning_octaveStretch`, `tuning_pitchBendRange`, `tuning_temperamentPreset`)
are automatable, so host automation applies them on the audio thread and calls `parameterChanged`
there. That handler calls `setMasterTune`/`setOctaveStretch` → `rebuildFrequencyTable()` (locks
`intervalMutex` + 128× `calculateCustomFrequencyUnlocked`, each with `std::pow`), and
`setBuiltInPreset` → `setCustomIntervals` (locks `intervalMutex` + `publishIntervalsSnapshot`'s
`std::make_shared`). The default mode is Just-Intonation (Scala), which takes the expensive locked path.

- **Failure scenario:** A Logic/Cubase automation lane on Master Tune or Temperament → `std::mutex::
  lock()` + `make_shared` + 128 `pow` inside the audio callback. If the message thread simultaneously
  holds `intervalMutex` (user editing intervals in the UI), the audio thread blocks → priority
  inversion / dropout.
- **Fix:** In `parameterChanged`, store incoming values into atomics and `triggerAsyncUpdate()`;
  do the actual `setMasterTune`/`setBuiltInPreset`/`rebuildFrequencyTable` work in
  `handleAsyncUpdate()` (message thread). `getFrequency` already reads the table lock-free, so the
  deferral is safe. (Coarser alternative: mark the four tuning params non-automatable.)

### [CR-06] Factory presets seed from live state, not defaults → non-deterministic recall; "Init" doesn't init
**`Source/PresetManager.cpp:524` (`buildFactoryPresetXml`)**

`buildFactoryPresetXml` starts from `processorRef.getAPVTS().copyState()` — the *current live*
parameter values — then applies only the sparse `paramOverrides`. Every parameter a preset does not
explicitly override retains whatever value was loaded before.

- **Failure scenario:** (a) Load "Cinematic Tension" (sets `velocityToFilter=0.5`,
  `filterLfoDepth=0.4`), then load "Deep Space" (overrides neither) → Deep Space plays with those
  values bleeding in. (b) The "Init" preset has `{}` overrides → loading it leaves the synth exactly
  as-is instead of returning to defaults. Recall depends on prior state.
- **Fix:** Build the base tree from defaults before applying overrides — iterate the PARAM children
  and set each `value` to `param->convertFrom0to1(param->getDefaultValue())` via
  `getAPVTS().getParameter(id)`, *then* apply `paramOverrides`. User presets (full state saves) are
  unaffected. See `pattern_preset_apply_needs_reset_to_defaults`.

---

## WARNING

### [WR-01] `generateChord()` heap-allocates on the audio thread every note-on
**`Source/DSP/WavetableVoice.cpp:122` → `Source/DSP/ChordGenerator.cpp:15-33,71-92,146`**

`startNote` (audio thread) calls `chordGeneratorPtr->generateChord(...)`, which returns a
`std::vector<ChordVoice>` by value and internally allocates more vectors (`buildChordIntervals`,
`distributeVoices`, and a second vector + sort for Drop2). Dense chord passages / fast arpeggios
across 8 voices trigger multiple allocations on the audio thread → non-deterministic latency,
allocator-lock contention, xruns under load.
- **Fix:** Precompute the chord layout off-thread when chord params change and hand the voice a
  fixed-size POD snapshot (`std::array<ChordVoice, MAX_SUB_VOICES>` + count) through the existing
  `setChordGenerationParams` plumbing; have `generateChord` write into a caller-supplied buffer.

### [WR-02] `keyRoot` is an `AudioParameterChoice` wired through a `WebSliderRelay` (works only by luck)
**`Source/PluginEditor.cpp:37,623`, `Source/ui/public/index.html:1859`**

`keyRoot` is defined as `AudioParameterChoice` (12 note names) but the editor builds a
`WebSliderRelay`/`WebSliderParameterAttachment` and the UI uses `Juce.getSliderState('keyRoot')` +
`setupDropdown`. It happens to work because a choice param's normalized mapping is linear `i/(N-1)`,
which `setupDropdown`'s `Math.round(norm*(maxIndex-1))` matches. This is the same class as the prior
`delayMode` bug. The other five choice params (voicingMode, wavetableBank, wavetableBank2, delayMode,
tuning_temperamentPreset) all correctly use `WebComboBoxRelay`.
- **Fix:** Convert `keyRoot` to `WebComboBoxRelay` + `WebComboBoxParameterAttachment` +
  `Juce.getComboBoxState('keyRoot')` / `setupComboBox`, matching the other choice params.

### [WR-03] Delay/reverb delay lines overflow above 96 kHz
**`Source/DSP/DelayProcessor.h:29-30`, `Source/DSP/ReverbProcessor.h:33-34`**

`delayL/R{192000}` and `preDelayL/R{19200}` are fixed maximum sizes set at construction; `prepare()`
never rescales for `sampleRate`. `delayTime` max is 2.0 s and `reverbPredelay` max is 200 ms →
192000/2.0 = 96 kHz, 19200/0.2 = 96 kHz.
- **Failure scenario:** At `fs = 176400/192000`, `setTime(2.0)` → `delaySamples = 384000 > 192000`
  and `predelay 200 ms → 38400 > 19200` → `DelayLine::popSample` asserts in debug / reads out of the
  valid window in release (wrong delay time, garbage tail).
- **Fix:** In each `prepare()`, `setMaximumDelayInSamples((int)std::ceil(2.0 * sampleRate) + 1)` for
  the delays and `(int)std::ceil(0.2 * sampleRate) + 1` for the pre-delays.

### [WR-04] Preset name used verbatim as filename — "/" (and OS-illegal chars) silently breaks save
**`Source/PresetManager.cpp:34 (getPresetFile), 41-73 (savePreset)`**

`getPresetFile` returns `getUserPresetFolder().getChildFile(name + ".xml")`. A name containing "/" is
a path separator; `\ : * ? " < > |` are illegal on Windows. `buildFlatList` uses non-recursive
`findChildFiles`, so a mis-pathed preset never appears in the browser.
- **Failure scenario:** Saving `"Koto / Harp"` → `getChildFile("Koto / Harp.xml")` writes into a
  `Koto ` subfolder (or fails) → silent loss, no error surfaced.
- **Fix:** Sanitize via `juce::File::createLegalFileName(name)` (reject empty results) in
  `getPresetFile`/`savePreset`, or block "/" at the UI entry point. See
  `critical_preset_name_slash_path_separator`.

### [WR-05] Chord base MIDI note passed unclamped — extreme voicings collapse to boundary pitch
**`Source/DSP/WavetableVoice.cpp:136,143`**

`spacingMidiNote`/`inversionMidiNote` are clamped to [0,127] (lines 148-149,156-157) but
`baseMidiNote` (line 136) is passed straight to `resolveFrequency`. `getFrequency` clamps internally
(not an OOB read) but *saturates*.
- **Failure scenario:** Thirds/Quartal/Quintal at a high root (`degreeOffset = interval*i` up to ~77;
  root 100 → note 177) or Drop2 at a very low root makes several sub-voices resolve to the same
  clamped pitch (127 or 0) → chord tones pile onto one pitch instead of stacking.
- **Fix:** Clamp `baseMidiNote` to [0,127] alongside the others, or drop out-of-range voices so they
  don't duplicate the boundary pitch.

### [WR-06] Fallback single-voice path consumes the NE offset then discards it and bypasses TuningEngine
**`Source/DSP/WavetableVoice.cpp:113-115, 231-247`**

`neRatio = applyPendingTuning(...)` (line 115) `exchange(0.0)`-consumes the pending NE slot. The
fallback branch (chord generator null / `cachedEnabledDegrees` empty) builds the note from
`juce::MidiMessage::getMidiNoteInHertz(...)` (line 233) — `neRatio` is never applied and the
TuningEngine is bypassed. Currently latent (the processor substitutes `defaultDegrees{0}` so the
branch isn't taken today), but wrong if it ever is.
- **Fix:** In the fallback use `resolveFrequency(midiNoteNumber, 0.0) * neRatio` so tuning + NE are
  honored consistently.

---

## INFO

### [IN-01] Silence threshold `0.0001f` duplicated 4× (backlog #21)
`Source/DSP/WavetableVoice.cpp:334,398,412,426` — introduce `static constexpr float kSilenceThreshold = 0.0001f;`.

### [IN-02] Dead voice methods `setWavetablePosition` / `setWavetablePosition2` (non-LFO)
`Source/DSP/WavetableVoice.cpp:492-501,514-523` — never called (processor uses the `...WithLFO`
variants only). Delete both.

### [IN-03] Bank setters + `setChordGenerationParams` run every block with no change guard
`Source/DSP/WavetableVoice.cpp:481-512,583`, callers `PluginProcessor.cpp:615-618,625` — 16
`getBank` calls/block in steady state (the sole trigger for CR-01's slow path) and a full
`cachedEnabledDegrees.assign()` every block (reallocates if the degree count grows past capacity).
Gate both on an actual change.

### [IN-04] Five registered native functions are never called from JS (dead code)
`Source/PluginEditor.cpp:157,198,210,353,539` — `setTuningIntervals`, `setTemperamentPreset`,
`getTemperamentPreset`, `getEmbeddedTuningCategories`, `getPresetCategories`. Delete, or switch the
JS hardcoded category lists (index.html:2083, tuning-panel.js:158-163) to call the accessors.

### [IN-05] `tuning-panel.js` docstring still tells callers to pass `window.__JUCE__`
`Source/ui/public/js/tuning-panel.js:18` — the usage example says `new TuningPanel(container,
window.__JUCE__)`, which has no `getNativeFunction` → an all-silent panel for anyone who follows it.
index.html correctly passes `Juce`. Fix the docstring in the module + per-plugin copies (standing
MEMORY.md note).

### [IN-06] `tuning_temperamentPreset` has no UI control (automation-only)
Relay + attachment + change-listener exist (index.html:2525) but there is no `<select>` anywhere.
Reachable only via a DAW automation lane. Combined with IN-04 its get/set accessors are dead.
Confirm intent — add a dropdown or accept automation-only and drop the accessors.

### [IN-07] `wireWavetableDisplay` unused `numBanks` parameter
`Source/ui/public/index.html:1619` (call sites 1868-1869 pass `20`) — never referenced (bank index
comes from `bankState.getChoiceIndex()`). Drop the parameter.

### [IN-08] PLUGINS.md version stale (2.7.2) vs shipped build (2.8.0)
`PLUGINS.md:41` shows `2.7.2` but `CMakeLists.txt:9` + CHANGELOG top are `2.8.0` (Note Expression
already implemented). Bump the registry row.

### [IN-09] Torn read of the frequency table during a concurrent rebuild
`Source/DSP/TuningEngine.cpp:656-657` — `getFrequency` acquire-loads the version counter then does a
relaxed read of one entry with no per-read re-validation; a read racing a rebuild can observe a
half-updated table (one note briefly mistuned). Benign (one block, single note) and moot once CR-05
moves rebuilds off the audio thread.

### [IN-10] Reverb pre-delay not cleared on 0↔N transition; `getActiveNotes` UI race
`Source/DSP/ReverbProcessor.cpp:72-85` — stale pre-delay content clicks on re-engage.
`PluginProcessor.cpp:869-894` (`getActiveNotes`, message thread) reads voice sub-voice floats/ints
the audio thread mutates without synchronization — bounded (fixed arrays) and cosmetic (UI note
display). Low priority.

### [IN-11] `smoothedGainA/B` never flushed to zero (largely mitigated)
`Source/DSP/WavetableVoice.cpp:286-287` — the smoothed osc A/B gains decay toward 0 but are never
snapped; osc-B is processed whenever a mix ≥ 1e-4 regardless of `smoothedGainB`, so a denormal gain
could feed the multiply. **Mitigated** in practice because `renderNextBlock` runs inside
`processBlock`'s `ScopedNoDenormals` (FTZ/DAZ active). Optional hardening: snap `smoothedGainB` to 0
below ~1e-7 and skip the osc-B pass.

---

## Recommended remediation order

1. **CR-03** (crash — FileChooser UAF) and **CR-02** (always-wrong readouts + destructive edits) —
   highest user impact, isolated fixes.
2. **CR-01** (bank-gen on audio thread) + **IN-03** (change-guard, part of the same fix).
3. **CR-04** (EQ ArrayCoefficients) and **CR-05** (defer tuning-param work to async) — the two
   remaining audio-thread hazards; established fix patterns exist in the codebase.
4. **CR-06** (preset seeding from defaults) + **WR-04** (preset-name sanitize) — preset subsystem.
5. **WR-01/02/03/05/06** and the **INFO** cleanups as a follow-up sweep.
