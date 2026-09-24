---
phase: O-Bowed-v1.9.0
reviewed: 2026-09-23
verified: 2026-09-23T21:35:00-07:00  # v1.9.1 resolution (CR-02, WR-01, WR-05); v1.9.2 resolution (CR-01, 2026-09-24)
depth: deep
files_reviewed: 26
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
  - plugins/O-Bowed/Source/DSP/BowNoiseGenerator.h
  - plugins/O-Bowed/Source/DSP/SubHarmonicsGenerator.h
  - plugins/O-Bowed/Source/DSP/BodyResonator.cpp
  - plugins/O-Bowed/Source/DSP/BodyResonator.h
  - plugins/O-Bowed/Source/DSP/SympatheticStringEngine.cpp
  - plugins/O-Bowed/Source/DSP/SympatheticStringEngine.h
  - plugins/O-Bowed/Source/DSP/StereoWidthProcessor.h
  - plugins/O-Bowed/Source/DSP/HumanizeEngine.h
  - plugins/O-Bowed/Resources/ui/index.html
  - plugins/O-Bowed/Resources/ui/js/i18n.js
  - plugins/O-Bowed/CMakeLists.txt
  - plugins/O-Bowed/tests/render-harness/main.cpp
  - modules/synthesis/bow-friction/cpp/BowModel.h
  - modules/synthesis/bow-friction/cpp/HyperbolicFriction.h
  - modules/tuning/note-expression/cpp/NoteExpression.h
  - modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
findings:
  critical: 4
  warning: 12
  info: 10
  total: 26
status: issues_found
---

# O-Bowed v1.9.0: Code Review Report

**Reviewed:** 2026-09-23
**Depth:** deep. DSP and processor read in full; editor and WebView reviewed by a subagent, with its
two Critical claims re-checked against `TuningEngine.cpp`; DSP claims checked by render-harness
measurement and a numeric loop model.
**Supersedes:** the v1.3.0 review of 2026-07-08. Its CR/WR items were resolved in v1.4.0/v1.4.1.
Its still-open Info items are carried forward below where still true (see git history for the
original file).
**Status:** issues_found

## Summary

The July fixes hold. RT allocation on coefficient updates, NaN guards, the SafePointer dialogs,
relay/attachment wiring and skew-correct readouts all check out, with no regressions.

The new defects are in behaviour the July review didn't measure:

- **Pitch path.** The upper register doesn't speak (CR-01). Pitch bend empties the string (CR-02).
  Dorico microtonal offsets are dropped on the first bend (WR-01). Pitch goes sharp at low Brightness
  (WR-05).
- **Tuning ownership.** The tuning panel and the panel's reference knob are overridden every block
  by `processBlock` (CR-03, CR-04). The editor and audio thread also race on the engine (WR-07).
- **Sympathetic strings.** The loop filter is so heavy that the strings don't resonate (WR-02).

### Evidence method (reuse for verification)

The render harness (`-DOUARICON_BUILD_TESTS=ON`, target `O-Bowed-render-test`) was run at the
default parameters, then analysed with an FFT over 1.5–3.5 s:

```bash
B=build/plugins/O-Bowed/tests/render-harness/O-Bowed-render-test_artefacts/Release/O-Bowed-render-test
for n in 45 57 69 72 74 76 77 79 81 84 88 93; do
  $B --note $n --sustain 4 --out r_n$n.wav --json r_n$n.json
done
# optional: --bow-position <norm> --bow-pressure <norm> --bow-speed <norm>
```

Harness gaps worth closing while fixing: it has no `--brightness`, `--pitchbend`, `--cc11` or
sympathetic flags, and no pitch-accuracy assertion.

---

## Critical (4)

### CR-01 — Upper register doesn't play at the default bow position: harmonic lock-in above ~E5, silence above ~D6
- **File:** `Source/DSP/WaveguideString.cpp:84-106` (`updateDelayLengths`), `:212-261` (read/write junction); bow-position default `Source/PluginProcessor.cpp:62-68`
- **Severity:** Critical
- **What:** Measured with the default parameters. Bow position 0.12 is also the value in the Violin,
  Erhu, Sarangi and Nyckelharpa presets (normalised 0.357). "H1" is the fundamental.

  | Note | f0 | Result |
  |---|---|---|
  | 45–69 | 110–440 | Correct pitch (A4 +4 c) |
  | 74 | 587 | H1 −20 dB below H3 |
  | 76 | 659 | H1 −34 dB: the fundamental is gone |
  | 79–81 | 784–880 | Locked to H3, sounding an octave and a fifth high |
  | 84 | 1047 | Locked to H2, sounding an octave high |
  | 88, 93 | 1319, 1760 | **Silent** (rms 0.0000) |

  A bow-position sweep on notes 81, 88 and 93 found correct H1-dominant output only at normalised
  1.0 (β = 0.30). At 0.6 the strings locked to H2 or went silent. Pressure 0.1/0.6 and speed 0.6 did
  not help.
- **Failure scenario:** Any melody above about E5 with factory presets plays the wrong octave or
  nothing.
- **Root cause (hypothesis, confirm first):** At high f0 the bridge rail is only a few samples long
  (E6 at 88.2 kHz: period ≈ 67 samples, bridge rail ≈ 8). Two effects then distort the geometry:
  1. **Extra sample per rail.** `readJunction` pops before `writeJunction` pushes, so each rail's
     real delay is `setDelay + 1` (see `critical_delayline_push_without_pop_shifts_delay`).
     That adds 2 samples per loop and moves the effective β.
  2. **Minimum clamps.** The 2-sample per-rail minimum and the 4-sample total minimum distort short
     loops.

  The bridge loss filter's delay (WR-05) also takes a proportionally large bite. Those short rails
  plus Helmholtz-mode selection at small β drive the lock-in.
- **Fix:**
  1. Subtract the extra sample from each rail's delay (fixes the pitch error too).
  2. Compute the bridge rail from the post-compensation loop length.
  3. Consider a pitch-dependent minimum bridge-rail length in samples, or scale β toward 0.3 at the
     top of the range, as a real bow sits further from the bridge in relative terms on a stopped
     string.
  4. **Gate it:** add a harness check that sweeps MIDI 45–96 and asserts H1 is the strongest
     partial within ±10 c.

  Fix this after CR-02 and WR-05, which touch the same code.

### CR-02 — Pitch bend empties the string: `notePitchbendChanged` → `trigger()` → `reset()`
- **File:** `Source/BowedStringVoice.cpp:93-105`; `Source/DSP/WaveguideString.cpp:65-74`
- **Severity:** Critical
- **What:** `notePitchbendChanged()` calls `waveguideString.trigger(currentFrequency)`. `trigger()`
  calls `reset()`, which clears both delay lines, the bridge filter state and `energyEstimate`.
- **Failure scenario:** With a pitch wheel (legacy mode) or an MPE controller, every bend message
  restarts the string from silence. Vibrato and glides produce a stream of choked restarts.
  `energyEstimate = 0` can also make `isActive()` false during release, so the voice clears
  mid-tail.
- **Root cause:** `trigger()` handles two jobs: starting a note (needs a clean start) and retuning a
  held note (must keep its state).
- **Fix:** Add `WaveguideString::setFrequency(float)`, which updates `currentFrequency` and calls
  `updateDelayLengths()` without resetting. Call it from `notePitchbendChanged`. Smooth the change
  per sample with the existing `bowPositionSmoothed` pattern (a `frequencySmoothed`, ~5 ms) so
  14-bit bends don't step the Thiran delay. Keep `trigger()` for `noteStarted` only.

### CR-03 — Tuning-panel scales are never heard: `processBlock` sets the engine mode from `tuningSystem` on every block
- **File:** `Source/PluginProcessor.cpp:380-387`; setters at `Source/PluginEditor.cpp:409, 502, 536`; engine `modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp:231-252, 285, 328`
- **Severity:** Critical
- **What:** `loadScalaFile`, `setCustomIntervals` (library tuning or generator) and the embedded
  tunings put the engine in `Mode::Scala`. The next `processBlock` calls
  `tuningEngine.setMode(...)` from the `tuningSystem` parameter. That parameter defaults to index 2
  (12-TET), every factory preset stores 1.0 (12-TET), and nothing in the UI can change it: no
  `<select>`, and `bindComboBox` is never called. The mode goes back to 12-TET and the table is
  rebuilt.
- **Failure scenario:** The user loads a 19-EDO or just-intonation scale. The panel shows it, but
  every note plays 12-TET. It only works if the host automates `tuningSystem` to 0. The v1.3.0
  review filed the missing select as dead code (IN-10); in practice it disables the whole panel.
- **Fix:**
  1. When a scale-loading native function succeeds, set the `tuningSystem` parameter to Scala
     through the APVTS (`setValueNotifyingHost(0.0f)`), so the parameter owns the mode.
  2. Add a visible Tuning System select (Scala / MTS-ESP / 12-TET) to the tuning overlay and call
     `bindComboBox('tuningSystem', …)`.
  3. Only call `setMode` when the parameter value changes: cache the last index. `setMode(Scala)`
     takes `intervalMutex` on the audio thread; see WR-07.
- **Note:** Presets store `tuningSystem = 12-TET`, so loading a preset leaves Scala mode. Decide
  whether presets should carry `tuningSystem` at all (probably not, like `uiLanguage`), or only when
  the preset also carries a scale.

### CR-04 — Ref Pitch only works from 400 to 480 Hz; the panel's own reference knob is overridden every block
- **File:** `Source/PluginProcessor.cpp:215-222` (range 220–880), `:380`; `TuningEngine.cpp:101-109` (`jlimit(400, 480)`); panel knob `tuning-panel.js:1044` → `Source/PluginEditor.cpp:372` (`setMasterTune`); `index.html:1362`
- **Severity:** Critical
- **What:**
  - **Clamped range.** The main Ref Pitch knob spans 220–880 Hz and shows its value, but the engine
    limits it to 400–480. A knob showing 220.0 Hz plays at A=400; 880 plays at 480.
  - **Duplicate control.** The tuning panel's reference knob calls `setMasterTune` from the message
    thread, and `processBlock` overwrites it with the parameter value on the next block, so dragging
    it has no lasting effect.
  - **Data race.** Both threads write the plain `double a4Frequency`.
- **Failure scenario:** Baroque A=415 works, but A=392 (French baroque) or anything outside 400–480
  silently doesn't. The panel knob snaps back.
- **Fix (a genuine choice between two options; decide at resolution):**
  - **(a)** Narrow `referencePitch` to 400–480. This changes the parameter range, which is breaking
    for automation and stored sessions (see `critical_apvts_denormalised_vs_preset_normalised`).
    Factory presets store 0.333 normalised and would need rewriting.
  - **(b)** Widen the engine clamp in the shared module. This affects all five plugins that embed
    it, so check their parameter ranges first.

  Either way, rewire the panel knob to `Juce.getSliderState('referencePitch')` and drop the
  `setMasterTune`/`getMasterTune` native functions, so there is a single owner.

---

## Warning (12)

### WR-01 — Dorico Note Expression tuning is lost on the first pitch bend
- **File:** `Source/BowedStringVoice.cpp:93-105, 373-389`; `modules/tuning/note-expression/cpp/NoteExpression.h:85-98`
- **What:** `applyPendingTuning` removes the pending offset with `exchange(0.0)`, so it applies
  once, in `noteStarted`. `notePitchbendChanged` calls `getBaseFrequencyFromTuning` again, finds the
  slot empty and rebuilds the frequency without the microtonal offset. The comment at `:379-384`
  calls this correct; it isn't.
- **Failure scenario:** In Dorico, a quarter-tone note followed by any pitch-bend event (or a
  controller sending bends) snaps back to the 12-TET pitch.
- **Fix:** Store the tuned base frequency (after tuning engine and NE) in a member such as
  `noteBaseFrequency` in `noteStarted`. Have `notePitchbendChanged` compute
  `noteBaseFrequency * 2^(bend/12)` without calling the helper again. Fix together with CR-02.

### WR-02 — Sympathetic strings don't resonate: the in-loop damping filter is a ~35 Hz lowpass
- **File:** `Source/DSP/SympatheticStringEngine.h:64` (`DAMP_POLE = 0.995f`); `.cpp:216-225`
- **What:** The feedback loop runs through `y = 0.995·y + 0.005·x`, a one-pole filter with a corner
  near 35 Hz at 44.1 kHz. Numeric model of the loop (delay = sr/f, decayGain 0.9995):

  | f target | loop gain per pass | real resonance | T60 |
  |---|---|---|---|
  | 110 | 0.37 | 89 Hz (−363 c) | 77 ms |
  | 440 | 0.10 | 339 Hz (−452 c) | 9 ms |
  | 880 | 0.05 | 674 Hz (−462 c) | 3.5 ms |

  The Decay knob can't help, because the loss sits in the filter. The pre-v1.4.0 code had the same
  structure with a pole of 0.99–0.9999, so this has never worked. The tap output is the lowpassed
  state, and with the `* 0.01` excitation scale the feature is close to inaudible.
- **Failure scenario:** The Sarangi (Amount 0.4, 5 strings) and Nyckelharpa (Amount 0.5, 10 strings)
  presets sound almost the same as with sympathetics off; what remains is sub-bass rumble.
- **Fix:**
  1. Replace the filter with a Karplus-Strong loop filter: the two-point average `0.5·(x[n]+x[n−1])`,
     or a one-pole with a small coefficient (≈0.1–0.3).
  2. Subtract its phase delay at f from the delay length (use the WR-05 formula).
  3. Keep `decayGain` as the ring-time control.
  4. Re-balance the `0.01` excitation scale by ear against the Sarangi preset, and add a harness
     check for sympathetic T60.
  5. This changes the sound of those two presets; mention it in the CHANGELOG.

### WR-03 — Sympathetic strings get reassigned between blocks (unstable sort, unconditional retune)
- **File:** `Source/DSP/SympatheticStringEngine.cpp:106-123, 153-157, 179-197`; caller `Source/PluginProcessor.cpp:409-420`
- **What:** Candidate harmonics are sorted with `std::sort`, which doesn't keep the order of equal
  entries, and the sort key is only the harmonic number. `updateTunings` then calls `tuneString` on
  every string every block, which sets its delay length and pan. Whenever the note set changes (or
  the unstable sort reorders ties), strings that are still ringing jump to a new pitch and pan.
- **Fix:**
  - Use `std::stable_sort`, with a secondary key on frequency.
  - Only call `tuneString(i, f)` when the target differs by more than ~1 cent.
  - Keep a string's current tuning when its fundamental is released (`numFundamentals == 0` already
    keeps it; per-string it doesn't).

### WR-04 — Bow noise ignores the bow envelope: full level at note-on and after note-off
- **File:** `Source/BowedStringVoice.cpp:272`; `Source/DSP/BowNoiseGenerator.h:48-63`
- **What:** Noise level is `effectivePressure · effectiveSpeed · amount`. Those are the knob and
  controller targets, not the `BowModel` envelope. So the noise starts at full level on the first
  sample of the note (no attack) and keeps going after `noteStopped(true)` until the waveguide
  decays, which with Infinite Sustain can take tens of seconds.
- **Failure scenario:** On "Breath of Strings" (bowNoise 0.7, infiniteSustain 0.15), releasing a
  note leaves a sustained hiss after the bow has lifted.
- **Fix:** Drive the noise level from the envelope, `bowModel.getBowForce() * bowModel.getBowVelocity()`
  taken at the 2× rate (last value, or the block average), instead of the targets.

### WR-05 — Pitch goes sharp at low Brightness: bridge-filter delay compensated with its low-frequency value
- **File:** `Source/DSP/WaveguideString.cpp:84-106`
- **What:** `filterGroupDelay = sr / (2π·fc)` is roughly the one-pole's delay at DC. The loop needs
  the phase delay at f0, `atan2(p·sin ω, 1 − p·cos ω) / ω` with ω = 2π·f0/sr. When fc is near or
  below f0 the code over-compensates. Numeric model at 88.2 kHz:

  | Note | Brightness | Pitch error |
  |---|---|---|
  | A4 | 300 Hz | +161 c |
  | A4 | 100 Hz | +1634 c |
  | A6 | 1 kHz | +255 c |
  | A6 | 3 kHz | +33 c |

  This is on top of the extra sample per rail from popping before pushing (CR-01), which makes
  pitch flat: about −17 c at A4 and −70 c at A6 at 88.2 kHz. At the default 8 kHz the two
  partially cancel, which is why A4 measured +4 c.
- **Fix:** Compensate with the phase delay at the current f0, and subtract 1 sample per rail for
  the pop-before-push order. The smoothed brightness makes this run per sample, and one `atan2`
  per sample per voice at 2× is affordable. Alternatively, recompute only when
  `brightness`/`frequency` move by more than a threshold. Add a harness check: A4 within ±5 c at
  Brightness 300 / 1000 / 8000.

### WR-06 — CC11 Expression isn't remembered per channel
- **File:** `Source/BowedMPESynthesiser.h:47-71`; `Source/BowedStringVoice.h:148`
- **What:** CC11 is only pushed to voices already active on that channel. A voice keeps its last
  `mpeExpression` (1.0 at construction) forever. A note started after a CC11 change plays at
  full, or at whatever expression that voice last had, until the next CC11 arrives.
- **Failure scenario:** CC11 drops to 20 during a rest, and the next note enters at full bow speed.
- **Fix:** Keep `std::array<float, 17> channelExpression` (default 1.0) in the synthesiser and set it
  on every CC11. Have the voice read it in `noteStarted` (via a pointer, as with the tuning engine).
  Reset it on reset-all-controllers (CC121).

### WR-07 — Tuning native functions race the audio thread; `intervalMutex` taken on the audio thread
- **File:** `Source/PluginEditor.cpp:319-385, 409, 430, 502, 536`; `Source/PluginProcessor.cpp:380-387`; `TuningEngine.cpp`
- **What:** `setOctaveStretch`, `setTonicNote`, `setSingleInterval`, `setCustomIntervals` and
  `loadScalaFile` call `rebuildFrequencyTable()` on the message thread. The audio thread can rebuild
  the same table in `setMasterTune`/`setMode`. `a4Frequency`, `octaveStretch` and the KBM fields
  are plain members. `setMode(Scala)` locks `intervalMutex` on the audio thread while the editor may
  hold it copying vectors, which can cause a dropout while loading a large scale.
- **Fix:** One owner. Either:
  - make the editor-side setters post to a lock-free flag and let the audio thread rebuild, or
  - make every rebuild run on the message thread and publish the table (the `frequencyTable` entries
    are already atomics), with the audio thread only reading.

  Stop calling `setMasterTune`/`setMode` every block (see CR-03 and CR-04). This is a shared-module
  fix, so check the other four embedding plugins.

### WR-08 — Blocks larger than the prepared size overrun `voiceBuffer` and the oversampler
- **File:** `Source/BowedStringVoice.cpp:126-153, 175-183`
- **What:** `voiceBuffer` and `oversampling.initProcessing` are sized to `maxBlockSize` from
  `prepareToPlay`. `renderNextBlock` uses `numSamples` directly. `MPESynthesiser` splits only at
  MIDI events, not by size. A host that sends a bigger block (Standalone device change, some
  offline renders) writes past the buffer.
- **Fix:** Loop inside `renderNextBlock` in chunks of at most the prepared size, as O-ReverseDelay
  v1.12.2 did ("chunked oversize blocks").

### WR-09 — Tail length reported as 0 s; long tails hold voice slots, and stealing them clicks
- **File:** `Source/PluginProcessor.h:64`; `Source/DSP/WaveguideString.cpp:112, 263-266`; `Source/BowedStringVoice.cpp:77-91`
- **What:**
  - **Tail length.** `getTailLengthSeconds()` returns 0, but with Infinite Sustain 1.0 (g = 0.9995)
    a released A4 falls about 1.9 dB/s, so T60 is roughly 31 s, and longer for low notes. The
    comment at `:111` ("~15s decay") is wrong. Offline bounces and freeze can cut the tail.
  - **Voice slots.** Voices stay active until `energyEstimate < 1e-7`, so released notes hold the
    8 slots.
  - **Stealing.** Stealing a slot calls `noteStopped(false)`, which resets the voice abruptly and
    clicks.
- **Fix:**
  - Return a realistic tail length (e.g. 3 s, or computed from `infiniteSustain` at the lowest
    playable note, capped).
  - Give the steal path a 2–5 ms fade before the reset.
  - Consider an audibility threshold for freeing voices in addition to the energy floor.

### WR-10 — Tuning-panel state isn't saved with the session
- **File:** `Source/PluginProcessor.cpp:497-542`
- **What:** State covers only the APVTS tree and `uiLanguage`. Loaded intervals, tonic, octave
  stretch and KBM mapping are lost when the project reopens. This matters once CR-03 is fixed.
- **Fix:** Write the engine state (scale name, cents list, degrees, tonic, stretch, KBM) as a child
  of `parameters.state` in `getStateInformation`, and restore it after `setStateFromXml`. Check how
  another embedding plugin (e.g. O-Bassoon) already does this and follow it.

### WR-11 — Clicking the active Bow/String tab stacks `requestAnimationFrame` loops
- **File:** `Resources/ui/index.html:2206-2213`
- **What:** `switchVizTab('bowString')` cancels the animation only when switching to a different tab,
  then calls `drawBowString()`, which starts a new loop. Each click on the active tab adds a loop, so
  the animation speeds up and CPU use grows. The loop also runs at full frame rate while nothing is
  playing.
- **Fix:** Call `cancelAnimationFrame(bowStringAnimId)` before starting a loop (or return early if
  one is running). Draw one static frame while `!vizState.isPlaying`.

### WR-12 — Preset Save ignores the folder picked in the dialog
- **File:** `Source/PluginEditor.cpp:289-291`
- **What:** The save dialog lets the user browse anywhere, but only
  `getFileNameWithoutExtension()` is used, and `pm.savePreset(name)` always writes to the user
  presets folder.
- **Fix:** Either use a name-entry dialog (`AlertWindow` with a text field) for "Save", or write to
  the chosen file with `savePresetToFile(result)` (keeping the preset-list refresh when it lands in
  the user folder).

---

## Info (10)

### IN-01 — Factory presets: `referencePitch` 0.333 snaps to 439.8 Hz; presets set `outputLevel`
- **File:** `Source/PluginProcessor.cpp:559-672`
- **What:** 220 + 0.333·660 = 439.78, which snaps to 439.8 Hz (−0.8 c). Use 0.333333. Every preset also
  writes `outputLevel` (0.833 → −0.02 dB), which goes against the project rule that presets never
  set output gain (`feedback_presets_never_set_output_gain`). Drop the key.

### IN-02 — The BowNoiseGenerator filter is never prepared or reset: first note allocates on the audio thread
- **File:** `Source/DSP/BowNoiseGenerator.h:40-46`
- **What:** The default-constructed `IIR::Filter` is first-order
  (`critical_iir_filter_default_ctor_is_first_order`). `prepare()` assigns biquad coefficients
  without `bandpassFilter.prepare()`/`reset()`, so the state is resized on the first
  `reset()`/`processSample`, which runs on the audio thread in `noteStarted`. Call `reset()` at the
  end of `prepare()`.

### IN-03 — Standard keyboards bow at half the knob's pressure
- **File:** `Source/BowedStringVoice.cpp:335-338`
- **What:** In legacy mode the note starts with pressure 0, so `effectivePressure = 0.5 × knob`. The
  knob readout overstates the pressure actually used until aftertouch arrives. Consider mapping so
  that zero pressure gives ×1.0 (e.g. `0.5 + mpePressure·1.5` only when MPE is enabled, or `1.0` in
  legacy mode), or document it in the tooltip.

### IN-04 — Stale or wrong comments
- **File:** `Source/PluginProcessor.cpp:553` (brightness normalisation says `pow(…, 4.0)`; skew 0.25 means `pow(p, 0.25)`, and stored values are correct: carried from the old IN-05); `Source/DSP/WaveguideString.cpp:111` ("~15s decay at 440Hz"; it's ~31 s T60); `Source/PluginEditor.cpp:201-203` (says anything not "fr" maps to English; `zh-Hans` is also accepted); `Source/BowedStringVoice.cpp:379-384` (the NE "correct" claim; see WR-01); `Source/PluginProcessor.cpp:396-399` (duplicate step numbers: carried from the old IN-08).

### IN-05 — Dead code sweep
- **What:**
  - `BowedStringVoice::outputGainLinear` (computed, never used).
  - `WaveguideString::processSample` (duplicates read/writeJunction, uncalled).
  - The per-block `setPan(0.707f, 0.707f)` loop in `processBlock` (old IN-08).
  - `ThermalFriction.h` and `ElastoPlasticFriction::computeFrictionDerivative` (old IN-02/IN-04).
  - The `savePreset` native function (old IN-11).
  - Unused registered tuning functions (old IN-13; recheck after CR-04 removes `setMasterTune`/`getMasterTune`).
  - `StereoWidthProcessor::reset()` calls `widthSmoothed.reset(0)`, which sets the ramp to zero
    steps and turns smoothing off. It's uncalled today, so the bug is latent; use
    `setCurrentAndTargetValue(target)` if it's ever wired.

### IN-06 — Each voice upsamples a block of silence every block
- **File:** `Source/BowedStringVoice.cpp:175-183`
- **What:** `processSamplesUp` runs the polyphase IIR over zeros only to get a 2× buffer. Up to 8
  voices pay for it every block. Render into a pre-sized 2× buffer and use only the downsampler
  (via a single-stage `Oversampling` with a manual down path, or a dedicated half-band decimator).

### IN-07 — The render-harness block-time check is flaky
- **File:** `tests/render-harness/main.cpp` (maxRatio ≤ 5)
- **What:** One run at note 69 reported `blockTime_max_over_median = 16.46` (FAIL); reruns passed at
  about 2.6. A single-block wall-clock max against the median is sensitive to machine load
  (`pattern_one_shot_latency_ratio_does_not_cancel_runner_load`). Use a percentile or repeat and
  take the minimum.

### IN-08 — Preset name only refreshes on the editor's own actions
- **File:** `Resources/ui/index.html:2266-2281`
- **What:** A session or program restore by the host while the editor is open leaves the name stale,
  and there is no modified marker. Add `presetName` (and a dirty flag) to the existing 15 Hz
  `getVisualizationState` poll.

### IN-09 — `getVisualizationState` polls at 15 Hz unconditionally (carried from old IN-12)
- **File:** `Resources/ui/index.html:1729`
- **What:** Poll only while a viz tab is visible and `document.visibilityState === 'visible'`.

### IN-10 — Bow graphic overflows at high pressure
- **File:** `Resources/ui/index.html` (bow-canvas draw)
- **What:** The bow marker height scales linearly with pressure, so at 5 N it is ~130 px and runs off
  the panel. Normalise through the parameter skew (the Schelleng tab already uses log pressure).

---

## Recommended resolution order

1. **CR-02 + WR-01 + WR-05** (one pitch path in `BowedStringVoice`/`WaveguideString`): a retune
   method that doesn't reset, a stored NE base, phase-delay compensation, and the pop-before-push
   sample. Add harness pitch checks (A4 at three brightness values; a bend round-trip keeps
   amplitude).
2. **CR-01**, re-measured after step 1 because the extra-sample fix moves β. Add the 45–96 sweep
   check.
3. **CR-03 + CR-04 + WR-07 + WR-10** (tuning ownership). CR-04 is a genuine choice between two
   options (parameter range vs shared-module clamp); ask the user.
4. **WR-02 + WR-03** (sympathetic loop redesign; changes the sound of the Sarangi and Nyckelharpa
   presets).
5. **WR-04, WR-06, WR-08, WR-09, WR-11, WR-12**, then the Info sweep.

**Version impact:** CR-04 option (a) changes a parameter range, which is breaking (MAJOR, or a
per-parameter migration: `pattern_preset_migration_per_param_version_gate`). Everything else is PATCH
or MINOR. WR-02 and CR-01 are audible timbre changes on existing presets; state them in the CHANGELOG.

Resolve via `/improve-review O-Bowed` (this file is the completed investigation: verify each
finding against the current source, then apply).

---

## Resolution log

- **v1.9.1 (2026-09-23):** CR-02, WR-01 and WR-05 resolved and measured (see CHANGELOG).
  **Correction:** the "extra sample per rail, `readJunction` pops before `writeJunction` pushes"
  claim (CR-01 root cause 1 and fix item 1, WR-05 fix) is false. JUCE `DelayLine` pop-then-push is
  exactly `setDelay` samples. Applying it made A4 +17.6 c sharp. The v1.9.0 +4 c at A4 was entirely
  the `sr/(2π·fc)` over-compensation (1.755 vs 1.303 samples). Re-measure CR-01 without that premise.
