---
title: "O-Bassoon Implementation Roadmap (Stage 0)"
created: 2026-04-27
last_verified: 2026-04-27
juce_version: "8.0.4"
summary: "Implementation plan for O-Bassoon. Complexity score 5.0 (capped) — staged DSP implementation across 4 phases. Foundation + shell + DSP (4 phases) + GUI + validation. Reuses shared note-expression and scala-tuning-engine modules. No O-Reed dependency. UI mockup deferred — Stage 3 plans against parameter-spec-draft.md until mockup is delivered."
domain: workflow
type: guide
keywords:
  - implementation-plan
  - staged-implementation
  - modal-synthesis
  - bassoon
  - microtonal
  - juce8
  - polyphony
  - note-expression
stages: [0, 1, 2, 3, 4]
agents: [build, dsp, ui, research]
---

# O-Bassoon — Implementation Roadmap

**Date:** 2026-04-27
**Complexity Score:** 5.0 (capped from raw 9.0) — Complex
**Strategy:** Phase-based (staged) implementation
**Prereq:** UI mockup (deferred — Stage 3 will block on it; Stages 1-2 do not)

---

## Complexity Factors

- **Parameters:** 10 parameters → `min(10/5, 2.0) = 2.0`
- **Algorithms:** 5 discrete DSP algorithms = **5.0**
  - Mode Bank (16-mode parallel biquad resonator, custom direct-form)
  - Excitation Generator (impulse + filtered noise, attack-character morph)
  - ADSR amplitude envelope (`juce::ADSR`)
  - Vibrato LFO + onset envelope (custom + `juce::SmoothedValue`)
  - Output Gain (post-summation)
- **Features:** 2 points = **2.0**
  - Modulation system (vibrato LFO + onset envelope + parameter smoothing): +1
  - External MIDI control / Note Expression integration (Dorico microtonality + MPE pitch-bend + CC routing): +1
  - Feedback loops: 0 (none — modal synthesis is feed-forward)
  - FFT / frequency domain: 0
  - Multiband: 0
- **Total:** 2.0 + 5.0 + 2.0 = **9.0 → capped at 5.0**

**Why staged:** Score ≥ 3.0 mandates phased DSP implementation. Although individual components are LOW-risk, the **bassoon partial table tuning** is the highest-value-and-uncertainty step and benefits from being its own phase with dedicated A/B-vs-recording listening loop. Staging also lets Stage 1 (Foundation) and Stage 2.1 (basic mode bank) ship before the spectral tuning loop begins.

---

## Stages

- Stage 0: Research & Planning — **Done** (this document + ARCHITECTURE.md)
- Stage 1: Foundation — Next
- Stage 2: DSP — 4 phases
- Stage 3: GUI — blocks on UI mockup; 2 phases when unblocked
  - **D8 amendment (2026-05-01):** Tuning-tab embed added to Phase 3.1 scope (was: 10-param-only) — see Stage 3 CONTEXT D7.
- Stage 4: Validation — pluginval, Dorico parity test, presets, changelog

---

## Stage 1: Foundation

**Goal:** Set up the plugin shell, CMake, APVTS parameters, and core scaffolding. No DSP yet.

**Components:**
- `plugins/O-Bassoon/CMakeLists.txt` with:
  - `juce_add_plugin(O-Bassoon ... IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_WEB_BROWSER TRUE ...)` (ref: `juce8-critical-patterns.md` #22)
  - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (ref: project memory — required for Windows WebView)
  - Link `juce::juce_audio_processors`, `juce::juce_audio_basics`, `juce::juce_dsp`, `juce::juce_gui_extra`
  - Link shared modules: `Ouaricon::note_expression` and the scala-tuning-engine equivalent
  - `juce_generate_juce_header(O-Bassoon)` after `target_link_libraries`
- `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` skeleton:
  - Output-only `BusesProperties` (synth — see `juce8-critical-patterns.md` #4)
  - APVTS with all 10 parameters (10 `AudioParameterFloat` + 1 `AudioParameterInt` for `voice_count`)
  - `juce::Synthesiser` member
  - Pre-allocated 16 `BassoonVoice` instances + 1 `BassoonSound` registered with the synth
  - `getVST3ClientExtensions()` returning `Ouaricon::NoteExpression::VST3Extensions` instance (long-lived member)
  - NE drain call at top of `processBlock` (placeholder — voices ignore for now)
  - Empty `BassoonVoice` shell that returns silence
- `plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` placeholder (generic JUCE GenericAudioProcessorEditor or empty editor — replaced in Stage 3)
- `plugins/O-Bassoon/Source/BassoonVoice.{h,cpp}` skeleton:
  - `class BassoonVoice : public juce::SynthesiserVoice`
  - Stores raw pointers to APVTS, `PendingTuningTable`, `TuningEngine`
  - `canPlaySound`, `startNote`, `stopNote`, `pitchWheelMoved`, `controllerMoved`, `renderNextBlock` stubs
- `plugins/O-Bassoon/Source/BassoonSound.h` — `class BassoonSound : public juce::SynthesiserSound { bool appliesToNote(int) override { return true; } bool appliesToChannel(int) override { return true; } };`

**Test Criteria:**
- [ ] `cmake --build build --target O-Bassoon_VST3` succeeds
- [ ] `cmake --build build --target O-Bassoon_AU` succeeds
- [ ] `pluginval --strictness 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon.vst3` passes
- [ ] Plugin loads in Ableton / Logic without crash; appears in instrument category
- [ ] No O-Reed source files referenced (verified via `grep -rn "O-Reed\|OReed" plugins/O-Bassoon/`) — satisfies DSP-07
- [ ] Plays silence (no audio bug, no crash) when MIDI notes are sent

**Verifies requirements:** COMPAT-01 (pluginval pass), DSP-07 (no O-Reed dependency)

---

## Stage 2: DSP — Phased

### Phase 2.1: Core Modal Voice (Basic Tone Generation)

**Goal:** Validate that the modal-synthesis architecture produces sustained tones at correct pitch with no artifacts. Lock in the per-voice DSP chain skeleton.

**Components:**
- `plugins/O-Bassoon/Source/ModeBank.{h,cpp}` — direct-form biquad array (16 modes), `setFundamental(float f0, float fs)`, `setTone(float tone)`, `processSample(float excitation)`, `reset()`. Initial partial table: simple integer harmonics (`PARTIAL_RATIOS = {1.0, 2.0, 3.0, ..., 16.0}`) with flat amplitudes (placeholder — tuned in Phase 2.2).
- `plugins/O-Bassoon/Source/Exciter.{h,cpp}` — impulse-only at note-on (defer attack-character morph + sustain noise to Phase 2.4). Single 5ms exponentially-decaying impulse.
- `BassoonVoice::startNote` — compute `f_base = midiToFreq(note)` (use plain MIDI conversion, not yet TuningEngine), call `modeBank.setFundamental(f_base, sampleRate)`, fire impulse exciter, reset `juce::ADSR` with `{attack=10ms, decay=0, sustain=1, release=200ms}` (hardcoded — APVTS connection in Phase 2.3).
- `BassoonVoice::renderNextBlock` — per-sample loop: excitation = exciter sample + (sustain noise placeholder = 0), voice_out = modeBank.processSample(excitation), voice_out *= adsr.getNextSample(), write to mono output buffer.

**Test Criteria:**
- [ ] Plays a sustained tone at correct pitch (verified with tuner: A4 = 440 Hz ±2 cents)
- [ ] No clicks at note-on / note-off
- [ ] No NaN/inf in output (verified by audio recording → numerical scan)
- [ ] Note sustains for >10 seconds without amplitude drift
- [ ] CPU at 1-voice / 48kHz / 256: <5%
- [ ] Plays across full C1-C6 range without obvious resonator instability or detuning at extremes

**Verifies requirements (partial):** FUNC-01 (sustained tones — basic), FUNC-03 (C1-C6 range), DSP-01 (mode bank exists)

---

### Phase 2.2: Bassoon Spectral Tuning + Tone Control

**Goal:** Tune the partial table and amplitude shaping until the voice sounds bassoon-like, not bell-like or generically "harmonic". Connect `tone` parameter.

**Components:**
- Update `ModeBank::PARTIAL_RATIOS` to bassoon-tuned near-integer values (per ARCHITECTURE.md "Bassoon Partial Table"): `{1.000, 2.005, 3.010, 4.018, ...}`
- Update per-mode amplitude weighting: `computeModeAmplitude(k, f0)` with formant-region Gaussian weighting at 475 Hz + spectral roll-off
- Implement `ModeBank::setTone(float tone)`:
  - For modes k > 4: `T60_k_scaled = T60_k * mix(0.3, 1.5, tone)` → recompute pole radius `R_k`
  - For modes k <= 4: T60 unchanged
- Connect `tone` APVTS parameter:
  - In `processBlock`, read `tone` smoothed value
  - In voice render, on parameter change > epsilon, call `modeBank.setTone(toneValue)`
  - Use `juce::SmoothedValue<float>` with 50ms ramp to avoid zipper noise
- A/B listening loop:
  - Capture render of held C3 (130 Hz) sustained 3 seconds
  - Compare to reference real-bassoon C3 recording (royalty-free, e.g., Philharmonia Orchestra sample library) on same pitch
  - Iterate partial amplitudes / formant location until timbre matches qualitatively
  - Document final partial table values in code comments + ARCHITECTURE.md

**Test Criteria:**
- [ ] Spectrum analyzer (e.g., iZotope Insight or fanout to plugin) shows peak energy in the 400-600 Hz region for a held C3 (matches bassoon's first formant emphasis)
- [ ] `tone = 0` produces audibly darker / woodier sustain compared to `tone = 1`
- [ ] `tone` sweep from 0→1 is smooth (no clicks, no zipper noise)
- [ ] Listening: held C3 is recognizable as "bassoon-like" (subjective — requires sign-off from project owner)
- [ ] CPU at 8-voice / 48kHz / 256: <20%

**Verifies requirements (full):** FUNC-01 (recognizable bassoon-like timbre), DSP-03 (tone control), DSP-01 (full mode bank with tuned partials)

---

### Phase 2.3: Per-Note Expression — Envelope, Breath, Vibrato

**Goal:** Connect all per-note expression APVTS parameters to DSP. Add vibrato system. The plugin becomes musically usable for sustained playing.

**Components:**
- Connect APVTS to `juce::ADSR`:
  - In `processBlock` (or per-voice `startNote`), read `attack_time` (ms) and `release_time` (ms)
  - Call `adsr.setParameters({attack_time/1000, 0.0, 1.0, release_time/1000})`
- Connect `breath` parameter:
  - Per-voice `juce::SmoothedValue<float>` with 20ms ramp
  - In `controllerMoved`, sum CC2 input on top of UI value (clamped 0-1)
  - In renderNextBlock per-sample: `voice_out *= breath_smoothed.getNextValue()`
- Connect `output_gain`:
  - Post-summation `juce::SmoothedValue<float>` (linear in dB → linear in amplitude via `Decibels::decibelsToGain`)
  - 30ms ramp
- Implement `Vibrato.{h,cpp}` per-voice helper:
  - Sine LFO with phase accumulator
  - `juce::SmoothedValue<float>` for onset envelope (linear ramp 0→1 over `vibrato_onset` ms)
  - `getCurrentCents()` → returns `vibrato_depth * onsetEnvelope * sin(phase)`
- In `BassoonVoice::startNote`: reset vibrato onset to 0, call `setTargetValue(1)` to start ramp
- In `BassoonVoice::renderNextBlock`:
  - At block start: sample vibrato cents (use block-start LFO value for the whole block — per-block coefficient update is sufficient for ≤10 Hz vibrato)
  - Compute `f_modulated = f_voice * pow(2, vibratoCents / 1200)`
  - On any frequency change > 0.1 Hz: call `modeBank.setFundamental(f_modulated, sampleRate)`

**Test Criteria:**
- [ ] `attack_time` parameter sweep 0-2000 ms produces audibly different onset slopes
- [ ] `release_time` parameter sweep 0-3000 ms produces audibly different release tails (no click at any setting)
- [ ] `breath` parameter changes audibly affect sustain volume; CC2 from a controller ramps loudness in real-time
- [ ] Vibrato LFO at 5 Hz with 50-cent depth produces clearly audible sine modulation, in tune (not detuned-sounding)
- [ ] `vibrato_onset = 1000ms`: vibrato fades in over ~1 second (verified with frequency-tracking analyzer)
- [ ] `vibrato_onset = 0`: vibrato is full immediately at note-on
- [ ] Held note for 60 seconds with vibrato active: no drift, no NaN, no denormal slowdown (CPU steady-state)
- [ ] No clicks during parameter sweeps (smoothing is working)

**Verifies requirements:** DSP-02 (vibrato — full), DSP-04 (breath/dynamics with CC2 + velocity), FUNC-04 (long-tone envelope), QUAL-02 (stable long-tone behavior, 60s hold)

---

### Phase 2.4: Voice Manager + Attack Character + Note Expression Integration

**Goal:** Polyphony, attack-character morph, microtonal pitch via NE + MPE, TuningEngine plumbing. Final DSP feature complete state.

**Components:**

**Voice manager:**
- Subclass `juce::Synthesiser` as `BassoonSynthesiser`, override `findFreeVoice` to enforce `voice_count` cap (return oldest active voice if `getActiveVoiceCount() >= voice_count` and stealing requested)
- Connect `voice_count` APVTS parameter (read at `startNote`, NOT during render — applies "next note-on")

**Attack character morph:**
- Update `Exciter::startOnset(float attack_character, float velocity)`:
  - Apply velocity bias: `effective_attack_char = clamp(attack_character + (velocity - 0.5) * 0.3, 0, 1)`
  - Pre-computed `softShape` and `tonguedShape` arrays (in `prepareToPlay`)
  - During onset window, output: `mix(softShape[onsetIdx], tonguedShape[onsetIdx], effective_attack_char)`
- Add sustain noise component: low-level filtered white noise scaled by `breath` (5% nominal level)

**Note Expression integration:**
- In `PluginProcessor`:
  - Instantiate `Ouaricon::NoteExpression::VST3Extensions` as long-lived member
  - Override `getVST3ClientExtensions()` to return its address
  - In `processBlock`, top-of-block: `Ouaricon::NoteExpression::updatePendingFromEvents(extensions, pendingTuningTable)`
- Pass `pendingTuningTable` pointer to each `BassoonVoice` in constructor / setup
- In `BassoonVoice::startNote`:
  - `f_base = tuningEngine->getFrequency(midiNoteNumber)` (TuningEngine wired in but defaulting to 12-TET A=440)
  - `f_with_NE = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningTable, midiNoteNumber, f_base)`
  - `currentFrequency = f_with_NE`
  - Then proceed with mode bank tuning

**MPE pitch-bend:**
- In `BassoonVoice::pitchWheelMoved(int newValue)`:
  - Compute `pitchBendSemitones = (newValue - 8192) / 8192.0 * pitchBendRange` (default range = 2)
  - `pitchBendMultiplier = pow(2, pitchBendSemitones / 12)`
  - Trigger mode bank coefficient update with `currentFrequency * pitchBendMultiplier * vibratoMultiplier`

**TuningEngine wiring:**
- Add `Ouaricon::TuningEngine` member to `PluginProcessor`
- Default-construct (12-TET, A4=440)
- Pass pointer to each voice (read-only access)
- v1.0 has no UI exposure; voices use it transparently for `f_base` lookup

**Test Criteria:**
- [ ] Polyphony: play 8 simultaneous notes → 8 distinct voices audible
- [ ] Voice cap: set `voice_count = 3`, play 4 notes → only 3 sound, oldest is stolen (verify via metering or console log)
- [ ] No stuck notes after rapid noteOn/noteOff sequence (10 Hz alternating noteOn/noteOff for 30 seconds)
- [ ] Attack character `0` (Soft) at low velocity (vel=20) → audibly gentle attack
- [ ] Attack character `1` (Tongued) at high velocity (vel=120) → audibly percussive attack
- [ ] Attack character `0.5` mid velocity → mid morph (no discontinuity)
- [ ] CC2 → breath: real-time loudness control via MIDI controller
- [ ] CC1 → vibrato_depth (additive): mod wheel adds to UI vibrato depth
- [ ] MPE pitch-bend per channel routes to per-voice bend correctly (test in DAW with MPE enabled)
- [ ] VST3 Note Expression: with the JUCE patch in place, `kTuningTypeID` events shift voice pitch as expected (test fixture: synthetic VST3 NE event sequence; or empirical via Dorico microtonal score in Stage 4)
- [ ] CPU at 8-voice / 48kHz / 256: ≤25% (PERF-02 target)
- [ ] No allocations in `processBlock` (verified by mock allocator or RT-safety checker)
- [ ] All NaN/inf checks remain clean across full parameter sweep
- [ ] Plugin builds and runs with O-Reed deleted from the workspace (DSP-07 final verification)

**Verifies requirements (full):** FUNC-02 (polyphony), FUNC-05 (voice stealing), DSP-05 (attack character morph), DSP-06 (microtonal pitch — full NE + MPE), PERF-01 (no allocations in processBlock), PERF-02 (8-voice CPU under 25%), QUAL-01 (no artifacts)

---

## Stage 3: GUI — Phased

**BLOCKER:** UI mockup is deferred. Stage 3 cannot start until a mockup is delivered (per BRIEF.md: "UI Concept: To be designed in a separate UI mockup pass").

### Phase 3.1: Layout and Basic Controls

**Goal:** Integrate the (forthcoming) UI mockup into a WebView-based plugin editor. Basic parameter binding for all 10 controls.

**Components:**
- Convert `mockups/v[N]-ui.html` to `Source/ui/public/index.html`
- Embed JUCE WebView resources (`js/juce/index.js`, `js/juce/check_native_interop.js` — see `juce8-critical-patterns.md` #13)
- Update `CMakeLists.txt`:
  - Add `juce_add_binary_data(O-Bassoon_UIResources SOURCES Source/ui/public/...)`
  - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
  - `NEEDS_WEB_BROWSER TRUE`
- Implement `PluginEditor.{h,cpp}`:
  - `std::unique_ptr<juce::WebSliderRelay>` per parameter (10 relays)
  - `std::unique_ptr<juce::WebBrowserComponent> webView`
  - `std::unique_ptr<juce::WebSliderParameterAttachment>` per parameter (10 attachments — use 3-arg constructor with `nullptr` undoManager per `juce8-critical-patterns.md` #12)
  - Resource provider with explicit URL mapping (per pattern #8)
- Use ES6 module loading (`type="module"`, per `juce8-critical-patterns.md` #21)

**Test Criteria:**
- [ ] WebView opens at correct size, displays all 10 controls
- [ ] All controls visible and styled per mockup
- [ ] No "Frame load interrupted" errors in DAW console
- [ ] On Windows: WebView2 loads (verify static linking works)

---

### Phase 3.2: Parameter Binding + Interaction

**Goal:** Two-way parameter communication between UI and DSP. Knob drag, host automation, preset changes.

**Components:**
- JavaScript: relative-drag knob handlers (per `juce8-critical-patterns.md` #16) — frame-delta pattern, NOT absolute positioning
- `valueChangedEvent.addListener` callbacks: read via `getNormalisedValue()` (per pattern #15 — callback receives no params)
- `voice_count` is `AudioParameterInt` → use `getSliderState` (slider for int per #19 decision tree)
- All other parameters are `AudioParameterFloat` → `getSliderState`
- Host automation: dragging knobs in DAW automation lane updates UI in real-time
- Preset changes: full UI refresh on `setStateInformation`

**Test Criteria:**
- [ ] All knobs respond to mouse drag with relative-drag behavior (no jump-to-cursor)
- [ ] All knob movements update DSP parameters audibly within 1 block
- [ ] DAW automation playback updates UI in real-time
- [ ] Preset load updates all 10 controls correctly
- [ ] No UI freezes / parameter desync

**Note:** No advanced UI elements (VU meters, spectrum, etc.) at v1.0 — Stage 3 is two phases only, not three.

---

## Stage 4: Validation

**Goal:** Final verification, presets, pluginval strict, Dorico parity, changelog.

**Components:**
- Run `pluginval --strictness 10` for VST3 + AU on macOS, VST3 on Windows → all pass
- Create 3-5 factory presets covering main use cases:
  - "Long Drone" (default-ish, low breath, slow attack/release)
  - "Microtonal Pad" (8 voices, slow attack, vibrato fade-in)
  - "Tongued Long Tone" (attack_character = 1, faster attack, mod wheel vibrato)
  - "Bright Bassoon" (tone = 1, soft attack, lighter vibrato)
- Dorico parity test (COMPAT-02): load via Playback Template (.dorico_pt from `note-expression` module v1.1.0), play microtonal score, verify per-note tuning audibly correct on quarter-sharp / quarter-flat / etc.
- CHANGELOG.md with v1.0.0 entry
- Update PLUGINS.md to "✅ Working" → "📦 Installed" after install verification

**Test Criteria:**
- [ ] pluginval strictness 10 passes (VST3 + AU + Win VST3)
- [ ] All 5 factory presets recall correctly
- [ ] Dorico microtonal score plays at correct microtonal pitches
- [ ] No regressions on any prior phase test
- [ ] Documentation complete (CHANGELOG, NOTES.md updated)

**Verifies requirements:** COMPAT-01 (pluginval strict), COMPAT-02 (Dorico parity), all remaining

---

## Implementation Notes

### Thread Safety

- All parameter reads via `APVTS::getRawParameterValue()->load()` once per block, smoothed across the block
- `PendingTuningTable` is `std::array<std::atomic<double>, 128>` — lock-free
- No mutexes in audio path
- No file I/O at v1.0

### Performance

- Estimated 8-voice CPU @ 48kHz/256 buffer on M1: ~16% (within PERF-02's 25% target with margin)
- Estimated 16-voice CPU: ~32% (above target, but PERF-02 only specifies 8-voice as the budget)
- Mode bank coefficient updates are block-rate (not per-sample) — keeps coefficient computation overhead minimal even with vibrato active
- 16 modes × 8 voices = 128 biquads at audio rate (no SIMD initially; SIMD optimization deferred to v1.1+ if needed)

### Latency

- **Zero processing latency** — modal synthesis is feed-forward
- `getLatencySamples()` returns 0; no host compensation needed

### Denormal Protection

- `juce::ScopedNoDenormals` at top of `processBlock`
- Mode bank G-normalised peak gain prevents accumulation of sub-LSB energy
- Voice clears mode-bank state on `clearCurrentNote` (after ADSR releases) — no stale state across note retriggering

### Known Challenges

1. **Bassoon partial table tuning** (Phase 2.2): Needs A/B-vs-recording listening loop. Plan for 2-3 iterations on this phase. **Reference:** capture a royalty-free bassoon C3 recording, target the first-formant emphasis at ~475 Hz.
2. **Voice-count cap with stealing** (Phase 2.4): Override `juce::Synthesiser::findFreeVoice` carefully — must return a non-null voice when stealing is requested. Stuck notes are the failure mode if logic is wrong.
3. **MPE pitch-bend with NE simultaneously** (Phase 2.4): NE is per-noteId; MPE pitch-bend is per-channel. Both should compose multiplicatively into `currentFrequency`. Verified via O-Lyrica precedent.
4. **JUCE-NE-PATCH presence** (Phase 2.4 / Stage 1): The `note-expression` module includes a CMake-time check for the patch marker. If JUCE is upgraded post-development without re-applying the patch, build fails. Documented in `spike-findings-VST-development` skill.
5. **CPU profiling early** (Phase 2.1 → 2.2): Profile at 8-voice / 48kHz / 256 BEFORE finalizing partial-table iteration count. Falling back to 8 modes is much cheaper before the table is tuned for 16.

---

## References

- Creative brief: `plugins/O-Bassoon/.planning/BRIEF.md`
- Parameter spec: `plugins/O-Bassoon/.planning/parameter-spec-draft.md` (v1.0 source of truth — final spec written at mockup finalization)
- Requirements: `plugins/O-Bassoon/.planning/REQUIREMENTS.md`
- DSP architecture: `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md`
- UI mockup: **deferred** — to be created via separate `/start O-Bassoon` UI mockup pass

### Reference Plugins

- **O-Lyrica** — Voice ownership pattern, NE drain pattern, pendingTuningSource per-voice. Use `Source/HarpSynthVoice.{h,cpp}` and `Source/PluginProcessor.cpp` (NE drain section) as direct templates.
- **O-Wind** — Voice/Synthesiser/SynthesiserSound integration with TuningEngine, parameter smoothing, MIDI/MPE routing. Use `Source/FluteSynthVoice.{h,cpp}` as a structural reference.
- **O-Bowed / O-Contrabass** — sustained-instrument voice management. Less directly applicable (waveguide-based), but useful for voice-count cap and stealing logic patterns.
- **O-Reed** — **DO NOT REUSE.** Explicit non-dependency per DSP-07.

### Research

- `research/modal-synthesis-bells-academic-research.md` — biquad math, frequency-dependent damping, Risset / Carillon DAFx patterns
- `spike-findings-VST-development` skill — VST3 Note Expression patterns, JUCE-NE-PATCH discipline
- `troubleshooting/patterns/juce8-critical-patterns.md` — pattern #4 (BusesProperties for synths), #8 (resource provider explicit URL mapping), #11 (WebView unique_ptr ordering), #12 (3-arg WebSliderParameterAttachment), #13 (check_native_interop.js), #15 (valueChangedEvent no-param callback), #16 (relative-drag knobs), #19 (boolean toggle API), #21 (ES6 module loading), #22 (IS_SYNTH TRUE for instruments)

### Shared Modules

- `modules/tuning/note-expression/` v1.1.0 — VST3 Note Expression for Dorico (production-validated by O-Lyrica 2.3.0)
- `modules/tuning/scala-tuning-engine/` v2.1.0 — Wired in headless at v1.0 (12-TET default); UI exposure deferred to v1.1
