# Stage 2: DSP — Context (rev-1)

**Date:** 2026-04-27
**Plugin:** O-Bassoon
**Stage:** 2 of 4 (DSP)
**Phase:** discuss
**Cycle Scope:** **Phase 2.1 — Core Modal Voice + First Audio**

---

## Discussion Summary

**Participants:** User, Claude

This discuss cycle opens Stage 2 by scoping Phase 2.1 of the 4-phase DSP staging locked at Stage 0. Phase 2.1 takes the silent voice stub from Stage 1 (foundation, R-foundation commit) and produces **first audio**: a per-voice mode bank (16 modes, placeholder integer-harmonic partial table, flat amplitudes) excited by a short impulse, gated by a hardcoded ADSR, summed into the stereo bus. Goal is "modal voice rings at correct pitch with no clicks, no NaN, sustains > 10 s, costs < 5 % CPU at 1 voice / 48 k / 256." Bassoon-specific spectral tuning, the `tone` parameter, vibrato, breath, attack-character morph, polyphony, NE/MPE pitch, and TuningEngine integration are all explicitly **deferred** to Phases 2.2 → 2.4 per ROADMAP.

The cycle is a single coupled scope: ModeBank + Exciter + ADSR wiring + per-voice mono-to-stereo write. No sub-phases (a/b/c). Atomic-commit on Gate 1 PASS. After Phase 2.1 verifies, Phase 2.2 (bassoon partial-table tuning + `tone` parameter) opens as a fresh GSD cycle — research phase will then drive the A/B-vs-recording listening loop against the reference bassoon C3 sourced **during Phase 2.1 execute/verify** (locked Q4 below).

---

## Cycle Scope

**Goal:** Replace the Stage-1 silent stub with a working modal-synthesis voice that produces a sustained, in-tune tone for any single MIDI note. The mode bank uses placeholder partial ratios and flat amplitudes — bassoon character is **not** a Phase 2.1 acceptance criterion. The voice path proves the architectural seams (excitation → resonator bank → envelope → stereo write) before Phase 2.2 starts iterating timbre.

**In scope:**

- `Source/ModeBank.{h,cpp}` — direct-form biquad array (16 modes), `std::array<Biquad, 16>` per voice. Public API: `prepare(double sampleRate)`, `setFundamental(float f0)`, `processSample(float excitation) -> float`, `reset()`. Coefficients computed from PARTIAL_RATIOS + BASE_T60 + computeModeAmplitude() per ARCHITECTURE.md §"Modal Resonator Biquad" + §"Bassoon Partial Table". **Phase 2.1 placeholders:** `PARTIAL_RATIOS = {1, 2, 3, ..., 16}` (integer harmonics, NOT bassoon-tuned ratios `{1.000, 2.005, ...}`) and **flat amplitudes** (NOT formant-Gaussian × 1/k roll-off). `setTone()` declared as a no-op stub returning early — wired live in Phase 2.2.
- `Source/Exciter.{h,cpp}` — impulse-only at note-on. Single 5 ms exponentially-decaying impulse generated into a pre-allocated, **shared-static** read-only buffer at `prepare()` time (allocation-free in startNote/processBlock). Public API: `prepare(double sampleRate)`, `start()` (resets onset index), `getNextSample() -> float` (returns `softShape[onsetIdx++]` or 0 once past onset window). **Phase 2.1 stubs:** no `attack_character` morph, no sustain-noise component, no velocity bias — wired live in Phase 2.4.
- `Source/BassoonVoice.{h,cpp}` — replace silent-stub `renderNextBlock` body with the per-sample inner loop. Add `juce::ADSR adsr` member with hardcoded `Parameters{0.010f, 0.0f, 1.0f, 0.200f}` (10 ms attack / 0 decay / 1.0 sustain / 200 ms release). `startNote`: compute `f_base = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber)` (plain MIDI, NOT TuningEngine — confirmed Q2), call `modeBank.setFundamental(f_base)`, call `exciter.start()`, `adsr.noteOn()`. `stopNote`: `adsr.noteOff()` if `allowTailOff`, else `clearCurrentNote()`. `renderNextBlock`: per-sample loop — `ex = exciter.getNextSample(); voice = modeBank.processSample(ex); voice *= adsr.getNextSample();` write `voice` to **both** L and R channels via `outputBuffer.addSample(channel, startSample + i, voice)` (sum, not overwrite — JUCE Synthesiser convention). When `adsr.isActive()` becomes false, call `clearCurrentNote()` and reset mode bank state.
- `plugins/O-Bassoon/CMakeLists.txt` — add `Source/ModeBank.cpp` + `Source/Exciter.cpp` to the source list (header-only files do not need explicit listing, but `target_sources` is the cleanest pattern; mirror existing block at the BassoonVoice line).
- `plugins/O-Bassoon/research/reference-recordings/` — new directory. Source one royalty-free bassoon C3 sustain (Philharmonia free library / freesound.org CC0 / similar) + adjacent `LICENSE.md` documenting source URL and license terms. Not used by Phase 2.1 verification, but archived during this cycle so Phase 2.2 kickoff has zero blocking on reference acquisition (locked Q4).

**Out of scope (deferred to later Phase 2.x cycles per ROADMAP):**

- Bassoon-tuned partial ratios + formant-Gaussian amplitude shaping + `tone` parameter (Phase 2.2)
- A/B-vs-recording listening loop (Phase 2.2 — uses the reference recording archived by Phase 2.1)
- APVTS reads of `attack_time`, `release_time`, `breath`, `output_gain`, smoothing (Phase 2.3)
- Vibrato LFO + onset envelope (Phase 2.3)
- CC2 routing to breath; `juce::SmoothedValue` parameter ramps (Phase 2.3)
- Voice manager / `voice_count` cap / oldest-note stealing override (Phase 2.4)
- `attack_character` morph (soft ↔ tongued shape crossfade) + sustain noise component + velocity bias (Phase 2.4)
- VST3 Note Expression consumption per-voice (`applyPendingTuning`) — drain wires already in place at Stage 1 but voices ignore the table (Phase 2.4)
- MPE pitch-bend per-voice multiplier (Phase 2.4)
- TuningEngine `getFrequency()` call in startNote (Phase 2.4 — at Phase 2.1 we use plain `MidiMessage::getMidiNoteInHertz`; result is bit-identical at default 12-TET A=440)
- pluginval `--strictness 10` + Windows VST3 build (Stage 4)

---

## Requirements Confirmed (Phase 2.1-relevant subsets of locked contracts)

- **DSP-01** (modal-synthesis voice — bank of damped resonators, pre-allocated): primary deliverable. Phase 2.1 satisfies the **structural** half (mode bank exists, pre-allocated, no allocations in `processBlock`). The **spectral** half ("tuned to bassoon spectrum") is Phase 2.2's deliverable.
- **FUNC-01** (sustained bassoon-like tones): Phase 2.1 satisfies "sustained tones" only. "Bassoon-like" is Phase 2.2.
- **FUNC-03** (C1-C6 range): Phase 2.1 verifies the mode bank reconfigures cleanly across MIDI 24-84 with no obvious resonator instability or detuning at extremes.
- **FUNC-04** (long-tone amplitude envelope): Phase 2.1 satisfies the **structural** half (ADSR present, attack/release respond) with hardcoded times. Parameter wiring and the full 0-2000 ms / 0-3000 ms ranges land in Phase 2.3.
- **PERF-01** (no allocations in `processBlock`): enforced from day one — ModeBank + Exciter + ADSR all preallocated in `prepare()`. Phase 2.1 verifies via grep + manual review.
- **QUAL-01** (no audible clicks, no NaN/inf): Phase 2.1 satisfies for the no-vibrato, no-parameter-sweep subset. Full QUAL-01 (parameter sweeps click-free) lands in Phase 2.3.
- **QUAL-02** (stable long-tone — no drift over 60 s): Phase 2.1 verifies the > 10 s subset (per ROADMAP test criterion 4); 60 s sustain is a Phase 2.3 verify (after envelope wired).
- **DSP-07** (no O-Reed dependency): already complete at Stage 1 (verified `grep -rn "O-Reed\|OReed"` empty). Phase 2.1 carries forward — no new sources reference O-Reed.

**Deferred to Phases 2.2–2.4 / Stage 4 (not Phase 2.1 acceptance):**
- DSP-02 (vibrato), DSP-03 (tone), DSP-04 (breath/dynamics), DSP-05 (attack-character), DSP-06 (NE + MPE pitch) — all wired in later phases.
- FUNC-02 (1-16 voice polyphony), FUNC-05 (voice stealing) — Phase 2.4.
- PERF-02 (8-voice <25 % CPU) — Phase 2.4 (when voice manager is in place).
- COMPAT-01 (`pluginval --strictness 10` + Windows VST3) — Stage 4.
- COMPAT-02 (Dorico parity) — Stage 4.

---

## Constraints Identified

**Locked contracts (do NOT modify in this cycle):**

- All 10 APVTS parameter IDs, ranges, defaults — `parameter-spec-draft.md` (Stage 0 source of truth) and `PluginProcessor.cpp:25–99` (Stage 1 implementation). Phase 2.1 reads NONE of them in voice DSP (locked Q2 — strict ROADMAP minimal wiring).
- DSP architecture (`research/ARCHITECTURE.md`) — biquad math, partial-table structure, processing chain, thread boundaries. Phase 2.1 implements the architecture's Phase-2.1 subset verbatim and does not amend.
- ROADMAP Phase 2.1 spec (lines 102-120) — components, test criteria, requirements verified.
- Stage 1 wiring contract: `PluginProcessor::processBlock` runs `vst3Extensions.drainAndUpdate()` BEFORE `synthesiser.renderNextBlock` (PluginProcessor.cpp:170, 174). Phase 2.1 does NOT change this ordering. Voices ignore the pendingTuningSource table at Phase 2.1 (drain runs but data is unused) — wired live at Phase 2.4.
- `BassoonVoice` member layout: `parameters`, `tuningEngine`, `pendingTuningSource` (raw pointers, set once at construction by PluginProcessor). Phase 2.1 leaves these untouched and does not dereference. New members (`modeBank`, `exciter`, `adsr`, `currentFrequency`, `currentSampleRate`) are additive.
- `BassoonSound::appliesToNote/Channel` returning `true` for all input — unchanged.
- Stage 1 build flags (CMakeLists.txt) — `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `PLUGIN_CODE OBsn`, `juce_generate_juce_header` after `target_link_libraries`. Phase 2.1 only adds source files; flags remain.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**

- `juce::ScopedNoDenormals` at `processBlock` entry — already in place. Mode bank IIR state (per-mode `y[n-1]`, `y[n-2]`) benefits from FTZ. The G-normalised peak gain in the biquad coefficient form (`G = (1-R) * amp`) prevents accumulation of sub-LSB energy by construction; `ScopedNoDenormals` is the belt-and-braces second line.
- `getLatencySamples()` is non-virtual in JUCE 8 — keep using `setLatencySamples(0)` in `prepareToPlay`. Modal synthesis is feed-forward; latency stays at 0.
- `juce::SynthesiserVoice::renderNextBlock` adds samples into the supplied `AudioBuffer`'s sub-range `[startSample, startSample + numSamples)`. **Voices SUM into the buffer, not overwrite** — `juce::Synthesiser` zeroes the output range before iterating voices. Phase 2.1 uses `outputBuffer.addSample(channel, startSample + i, voice_out)` for both channels.
- `juce::ADSR::getNextSample()` returns `0.0f` when ADSR is idle (not active and not in release tail). Per-voice `if (! adsr.isActive()) { clearCurrentNote(); modeBank.reset(); return; }` is the standard exit path — JUCE 8 confirmed.
- `juce::ADSR::setSampleRate` must be called BEFORE `setParameters` for correct internal-rate computation. Both go in `prepareToPlay`. Confirmed.
- Allocation-free `processBlock`: ModeBank's `std::array<Biquad, 16>` is in-class storage (no `new`). Exciter's onset buffer is a `static constexpr` or class-level `std::array<float, NUM_ONSET_SAMPLES>` populated once in `prepare()`. Voice's ADSR uses internal stack-only state. **No `std::vector`, no `juce::Array<>::resize`, no `make_unique`** in any audio-thread path.

**Phase 2.1-specific constraints:**

- **Strict ROADMAP minimal wiring** (locked Q2) — no APVTS reads inside `BassoonVoice`. Frequency comes from `MidiMessage::getMidiNoteInHertz` (plain MIDI, not TuningEngine). ADSR uses literal hardcoded times. The `pendingTuningSource`, `tuningEngine`, and `parameters` pointers stored at construction stay unread until Phases 2.3-2.4.
- **Centered mono-to-stereo write** (locked Q1) — voice computes ONE mono sample per audio sample and writes the same value to both channels via `outputBuffer.addSample(0, ...)` and `outputBuffer.addSample(1, ...)`. No pan law, no per-voice spread. Matches O-Lyrica `HarpSynthVoice::renderNextBlock` and O-Wind `FluteSynthVoice::renderNextBlock` precedent.
- **On-note-on / on-pitch-bend coefficient cadence** (locked Q3) — full 16-mode coefficient recompute fires only in `startNote` (after `f_base` is known) and `pitchWheelMoved` (when MPE host changes per-channel pitch — at Phase 2.1 the bend multiplier is applied as `f_modulated = f_base * pow(2, semis/12)` and triggers a full recompute). No per-block recompute. Tone scaling not yet wired (Phase 2.2). Vibrato not yet wired (Phase 2.3).
- **Atomic Phase 2.1 commit** (locked Q5) — single commit lands `Source/ModeBank.{h,cpp}` + `Source/Exciter.{h,cpp}` + `Source/BassoonVoice.{h,cpp}` edits + CMakeLists source-list update + reference-recording archive + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION/STATUS) on Gate 1 PASS only. Subject line pattern: `feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS`. Mirrors O-Contrabass R7 / R15 / R20 cadence.
- **Reference recording sourced during this cycle** (locked Q4) — does NOT block Phase 2.1 verification (Phase 2.1 uses tuner + spectrum analyzer for the 6-item bar, not vs.-recording A/B). Sourced during execute or verify. Stored at `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain.{wav, license.md}`. Phase 2.2 kickoff consumes it.
- **DAW + tuner only** (locked Q6) — no CLI render harness this cycle. Verification via Logic Pro AU + a tuner plugin (e.g., MMultiAnalyzer, MTuner, or any DAW-stock tuner) for pitch ±2 cents. Spectrum baseline capture (locked as mandatory Gate-1 sub-item — Q7-d) renders sustained C3 in Logic, screenshots the spectrum, archives the screenshot at `plugins/O-Bassoon/research/reference-recordings/phase-2.1-baseline-c3-spectrum.png`. This becomes the pre-tuning A/B reference for Phase 2.2.
- **No Windows build at Phase 2.1** — macOS VST3 + AU + Standalone only, mirroring Stage 1. Windows comes online at Stage 4.

**Working-tree starting state (locked from Stage 1 verify, R-foundation commit on `main`):**

- `plugins/O-Bassoon/CMakeLists.txt` — flags committed; source list = 5 files (BassoonSound.h, BassoonVoice.{h,cpp}, PluginProcessor.{h,cpp}, PluginEditor.{h,cpp}).
- `plugins/O-Bassoon/Source/BassoonSound.h` — header-only catch-all `juce::SynthesiserSound`.
- `plugins/O-Bassoon/Source/BassoonVoice.{h,cpp}` — silent stub. Three setters wired (`setAPVTS`, `setTuningEngine`, `setPendingTuningSource`); `renderNextBlock` body is no-op (BassoonVoice.cpp:45).
- `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` — `OBassoonAudioProcessor` with APVTS (10 params), `juce::Synthesiser` (16 voices pre-allocated), `TuningEngine` member (global ns), `Ouaricon::NoteExpression::VST3Extensions` member, `getVST3ClientExtensions()` returns `&vst3Extensions`, NE drain BEFORE `renderNextBlock` (PluginProcessor.cpp:170, 174).
- `plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` — `juce::GenericAudioProcessorEditor` placeholder, 500 × 480.
- Stage 1 build: VST3 + AU + Standalone all build clean (`ninja: no work to do` on re-run); `auval -v aumu OBsn OuDv` SUCCESS; `pluginval --strictness 5` SUCCESS.

---

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q1 — Cycle scope** | **Phase 2.1 only** (single GSD cycle): ModeBank + Exciter + ADSR + voice integration to "first audio." Defers all bassoon-specific spectral tuning, parameter wiring, vibrato, polyphony, and NE/MPE to later cycles | Five tightly-coupled tasks (mode bank, exciter, ADSR wiring, mono-to-stereo write, frequency conversion). No genuine sub-gate would benefit from a/b/c split — the risk surfaces (biquad stability, exciter shape, voice integration) all resolve together in one listening session. Splitting Phase 2.1 + 2.2 into one cycle was rejected because Phase 2.2's A/B-vs-recording listening loop deserves a dedicated cycle (per ROADMAP Stage 0 staging). User-confirmed. |
| **Q2 — APVTS / TuningEngine wiring** | **Strict ROADMAP minimal wiring.** Hardcoded ADSR `{0.010, 0, 1, 0.200}`. Plain `MidiMessage::getMidiNoteInHertz(midiNote)` for `f_base`. NO APVTS reads in `BassoonVoice`. NO `tuningEngine->getFrequency()` call. Existing pointer wiring (set in Stage 1 ctor) stays in place but unread until Phases 2.3 / 2.4 | Keeps Phase 2.1 unambiguously about "does the modal voice ring?" with zero parameter-state confounders. The TuningEngine path is bit-identical to plain MIDI at default 12-TET A=440 Hz, so calling it now buys no real seam exercise (the seam is already exercised by the pointer being non-null and stored). User-confirmed. |
| **Q3 — Mode-bank coefficient update cadence** | **On note-on + on pitch-bend only.** Full 16-mode recompute in `BassoonVoice::startNote` (after `f_base` known) and `pitchWheelMoved` (after applying `pow(2, bendSemis/12)` multiplier). NO per-block recompute. NO throttled-epsilon recompute | No vibrato → no per-block frequency modulation source. Tone parameter not yet wired → no per-block tone modulation source. Premature per-block recompute is ~16 modes × ~10 flops × block-rate = trivial CPU but adds code complexity for zero observable Phase 2.1 benefit. Throttled-epsilon variant is the Phase 2.3+ pattern but unnecessary at Phase 2.1 with only two trigger sources (note-on + pitch-bend), neither of which fires per-block. User-confirmed. |
| **Q4 — Reference bassoon C3 sourcing** | **Source during Phase 2.1 execute or verify.** Acquire a royalty-free bassoon C3 sustain from Philharmonia Orchestra free library, freesound.org CC0, or similar. Archive at `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain.wav` + adjacent `LICENSE.md` documenting source URL and license terms. Does NOT block Phase 2.1 verification | STATUS.md already lists this as a "Stage 0 carry-forward" Phase 2.2 kickoff input. Sourcing during Phase 2.1 means Phase 2.2 has zero sourcing risk — first source falling through during Phase 2.2 discuss would otherwise stall. Phase 2.1 itself uses tuner + spectrum analyzer for verification (no recording comparison yet — placeholder partials are not bassoon-like by design). User-confirmed. |
| **Q5 — Atomic commit unit** | **Single Phase 2.1 atomic commit on Gate 1 PASS.** Lands ModeBank + Exciter + BassoonVoice + CMakeLists + reference recording + 6 planning artefacts in ONE commit. Subject line: `feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS` | Mirrors O-Contrabass R7/R15/R20 + O-Lyrica precedent. Per-task incremental commits break the gate-first principle (commits in flight may have an audio voice that crashes startNote — gate-first guarantees every commit on `main` keeps build + auval + pluginval green). Two-commits-source-vs-planning splits the artefact-↔-code-state invariant. User-confirmed. |
| **Q6 — Test harness** | **DAW + tuner only at Phase 2.1.** No CLI render harness. Verification uses Logic Pro AU host + a tuner plugin (MMultiAnalyzer / MTuner / DAW stock) for pitch ±2 cents + Logic CPU meter for the < 5 % budget + spectrum analyzer (e.g., MultiInspector) for NaN/inf and basic spectrum baseline | Lower setup cost. Phase 2.1 invariants (pitch, sustain, no clicks, no NaN, CPU, range) are all DAW-observable. Matches O-Wind / O-Lyrica precedent — neither has a CLI harness. If Phase 2.2's partial-table iteration calls for automated A/B against the reference recording, the harness can be added at Phase 2.2 kickoff (deferred decision). User-confirmed. |
| **Q7 — Gate 1 PASS bar** | **Six-item ROADMAP bar + auval + pluginval-5 + Logic AU smoke + spectrum baseline capture (10 items total).** (1) sustained tone at correct pitch ±2 cents on A4=440 Hz (tuner verify); (2) no clicks at note-on/note-off; (3) no NaN/inf in render (spectrum analyzer + listening); (4) > 10 s sustain without amplitude drift; (5) 1-voice CPU < 5 % @ 48 k / 256 buffer (Logic CPU meter); (6) plays C1-C6 without resonator instability or detuning at extremes; (7) `auval -v aumu OBsn OuDv` SUCCESS; (8) `pluginval --strictness-level 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` SUCCESS; (9) Logic AU manual smoke — hold C3, play scale C1→C6, confirm pitch + sustain audible (subjective pass/fail; will NOT sound bassoon-like — placeholder partials); (10) DAW spectrum baseline capture — render sustained C3 to WAV, screenshot spectrum, archive at `research/reference-recordings/phase-2.1-baseline-c3-spectrum.png` | Items 1-6 are ROADMAP-mandated. Item 7-8 catch realtime-safety regressions (allocations, exceptions in startNote) that were dormant under the silent stub — must re-run on the audio-producing build. Item 9 is the qualitative smoke companion. Item 10 documents the pre-tuning state Phase 2.2 will A/B against (cheap to capture now, expensive to backfill). User-confirmed multi-select. |
| **Q8 — Voice mono-to-stereo write** | **Centered, equal L+R per sample.** Voice computes one mono `voice_out` per sample, writes it to BOTH channels via `outputBuffer.addSample(0, startSample + i, voice_out)` and `outputBuffer.addSample(1, startSample + i, voice_out)` (sum, not overwrite — `juce::Synthesiser` zeroes the buffer before iterating voices) | Standard `juce::Synthesiser` voice convention. Matches O-Lyrica `HarpSynthVoice` + O-Wind `FluteSynthVoice` per-sample stereo write pattern. Per-voice pan / pan-law deferred to v1.1+ if ever (no v1.0 user value). User-confirmed. |
| Exciter onset-buffer storage | **Class-level `std::array<float, NUM_ONSET_SAMPLES>`** (per-instance, populated once in `prepare()`) | Simplest RT-safe pattern. Per-voice cost = ~256 floats × 16 voices = 16 KiB total — negligible. Static-shared alternative exists but has thread-safety footprint (multiple voices reading simultaneously is fine for read-only data, but per-instance avoids any future "I bet I can mutate it for velocity scaling" footgun before Phase 2.4). |
| Voice exit path | **`if (! adsr.isActive())` → `clearCurrentNote()` + `modeBank.reset()` + `return` early in renderNextBlock** | Standard JUCE 8 pattern. `juce::Synthesiser` reuses cleared voices for next note-on. Resetting the mode bank clears stale IIR state so retriggered notes start from silence (no stuck-energy bleed-through). |
| Pitch-bend Phase 2.1 wiring | **Wire `pitchWheelMoved` to update `currentFrequency` and trigger mode-bank recompute** even though Phase 2.1 has no MPE testing in scope | Cheap (~5 lines) and exercises the seam Phase 2.4 will rely on for MPE. At Phase 2.1, host pitch-bend smoothly retunes the held note — bonus testable behavior, no extra acceptance criterion. Default range = 2 semitones (matches BRIEF). |
| ADSR exit cleanup | Mode bank `reset()` clears all `y[n-1] / y[n-2]` state to zero on `clearCurrentNote()` | Prevents residual energy from a previous note bleeding into the next when `juce::Synthesiser` reuses the voice slot. ARCHITECTURE.md "Denormal Protection" §confirms this. |

---

## Open Questions (handed to research-phase)

1. **JUCE 8.0.4 `juce::ADSR` API exact signatures.** Verify `Parameters{attack, decay, sustain, release}` brace-initialization is the JUCE 8.0.4 form (vs. older `setParameters(juce::ADSR::Parameters{...})`). Verify `setSampleRate` order (must be called before `setParameters` for internal-rate computation). Verify `getNextSample()` returns `0.0f` when idle and `isActive()` returns false at the same boundary. Lookup: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_ADSR.h`. Output: cite line numbers in RESEARCH.md.

2. **`juce::SynthesiserVoice::renderNextBlock` voice-output convention.** Confirm voices SUM into the supplied buffer's sub-range `[startSample, startSample + numSamples)` (not overwrite). Confirm `juce::Synthesiser::renderNextBlock` zeroes the output before iterating voices (so summing is safe even when only one voice is active). Lookup: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.{h,cpp}`. Output: cite line numbers in RESEARCH.md + confirm `addSample` is the canonical write call.

3. **Direct-form biquad numerical stability at long T60.** At BASE_T60 = 2.5 s × sample rate 48 kHz, `R = exp(-1/(0.36 × 48000)) ≈ 0.99994`. With `float` state (`y[n-1], y[n-2]`), is direct-form stable, or do we need TDF-II / state-variable form to avoid limit-cycle artefacts? Reference precedent: O-Formant uses custom 32-byte direct-form biquad structs at long decays. Confirm O-Formant's pattern is portable here. Output: RESEARCH.md notes whether `float` state is sufficient or `double` is required for the 16-mode array.

4. **Exciter impulse shape — exactly what curve at Phase 2.1?** ROADMAP says "5 ms exponentially-decaying impulse" (line 108). Architecture §"Excitation Generator" speaks of `softShape` and `tonguedShape` arrays (Phase 2.4). For Phase 2.1, what's the simplest valid shape? Recommend: half-sine over 5 ms windowed against an exp(-t/τ) envelope with τ = 1.5 ms. Single-mode shape — no morph. Provide concrete formula + a sanity-check render-shape sketch in RESEARCH.md.

5. **Mode-bank `setFundamental` numerical conditioning.** When `f0` is recomputed from MIDI + pitch-bend, partial frequencies `f_k = f0 × PARTIAL_RATIOS[k]` for `k = 15` at MIDI 84 (`f0 = 1046.5 Hz`) gives `f_15 = 16744 Hz` — above the Nyquist at 48 kHz. What's the muting policy? Options: (a) clamp `theta_k` at `π × 0.99` and accept aliasing-prone resonators near Nyquist; (b) explicitly mute (`amp_k = 0`) modes with `f_k > 0.45 × fs`; (c) compute `R_k = 0` (zero pole radius — DC pass-through, contributes nothing). Recommend (b). Architecture is silent — research-phase locks the policy.

6. **`juce::MidiMessage::getMidiNoteInHertz` JUCE 8 signature.** Confirm static method exists, takes `int noteNumber`, returns `double Hz`. Verify it uses A4 = 440 Hz (12-TET) by default. Lookup: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/midi/juce_MidiMessage.h`. Confirm there's a no-allocation guarantee.

7. **Reference bassoon C3 sourcing — concrete URLs + licenses.** Surface 2-3 candidate sources with license terms and direct download URLs:
   - Philharmonia Orchestra free sample library (CC license; need to verify exact terms — historically CC-BY-NC-SA, may not be redistributable)
   - Freesound.org CC0 sources (search: "bassoon C3 sustain")
   - University of Iowa Electronic Music Studios (public domain, well-known orchestral samples)
   - Pinpoint a primary recommendation + license-compatible-with-Ouaricon-distribution backup. Output: RESEARCH.md §"Reference Recording Sourcing" with URL + license + ~3 s clip note.

8. **Logic CPU meter accuracy for "1-voice < 5 % @ 48 k / 256" verification.** Logic's per-plugin CPU display is host-aggregated and shows process-level utilization, not strictly plugin-only. Is the < 5 % figure reproducible from Logic's display, or do we need `auval -v` --time output, or a third-party profiler (e.g., AudioPluginHost + Activity Monitor)? Recommend: Logic AU + Logic's "CPU/HD" display showing the I/O thread % is the canonical Ouaricon family verification (matches O-Wind / O-Lyrica practice). Confirm + document the exact reading method in RESEARCH.md.

9. **`pitchWheelMoved` Phase 2.1 semantics.** Default pitch-bend range = ±2 semitones. Recompute formula: `bendSemis = (newPitchWheelValue - 8192) / 8192.0 × 2.0; multiplier = pow(2, bendSemis / 12); f_modulated = currentFrequency × multiplier;` then `modeBank.setFundamental(f_modulated)`. Confirm `pitchWheelMoved` is called with raw 14-bit MIDI value `[0, 16383]` with center = 8192 — not normalized. Lookup JUCE 8 source.

10. **Spectrum-analyzer plugin recommendation for the baseline-capture step.** Logic doesn't ship a spectrum analyzer adequate for screenshot archival. Need a free + always-available tool. Candidates: Voxengo SPAN (free), MeldaProduction MMultiAnalyzer (free tier), Bertom EQ Curve Analyzer (free). Recommend SPAN. Output: RESEARCH.md notes the install path / version and a screenshot capture procedure.

---

## Risks (Phase 2.1-specific)

1. **Mode-bank IIR instability at long T60 + low f0.** `R → 0.99994` at T60 = 2.5 s, 48 kHz. With `float` state, the limit-cycle floor sits ~120 dB below the input — typically fine, but at 60 s sustain the state can integrate denormal-prone residuals. Mitigation: `juce::ScopedNoDenormals` (already in place) + `modeBank.reset()` on `clearCurrentNote`. Open Question #3 confirms whether `float` is sufficient or `double` is mandated. Phase 2.1 verifies with > 10 s sustain (per ROADMAP); the 60 s bar is QUAL-02 Phase 2.3.

2. **High-frequency modes near or above Nyquist.** At MIDI 84 (C6 = 1046.5 Hz), the 16th partial sits at 16.7 kHz — above Nyquist at 48 kHz. Without an explicit muting policy, the biquad coefficients become numerically pathological (`theta_k > π`). Mitigation: Open Question #5 locks the muting strategy (recommend `amp_k = 0` for `f_k > 0.45 × fs`). At Phase 2.1, this manifests at the top of the C1-C6 range as quiet/missing high modes — acceptable; the lower ~12 modes remain audible and tuned.

3. **First-audio click on note-on.** A 5 ms exponentially-decaying impulse is short enough that the ADSR's 10 ms attack should fully cover the audible attack ramp. If listeners hear a click at note-on, root cause is most likely (a) ADSR rising too slowly relative to the impulse decay, leaving the impulse audible standalone; (b) mode bank coefficient set AFTER excitation already injected (ordering bug); (c) `outputBuffer.addSample` writing into uninitialized buffer (Synthesiser zeros the buffer — but only the header range, not the sub-range `[startSample, startSample + numSamples)` — confirm in research-phase Open Question #2). Mitigation: research-phase locks the per-sample loop ordering; Phase 2.1 Gate 1 PASS bar item (2) catches any click empirically.

4. **CPU > 5 % at 1 voice / 48 k / 256.** 16 biquads × 48000 samples/s = 768 k biquad samples/s × ~6 multiplies + 3 adds = ~7 MFLOPs sustained per voice. M1 single-core SIMD ~250 GFLOPs theoretical → < 0.1 % CPU mathematically. Actual Logic display will be higher (overhead, branch mispredict, cache effects) — projected ~1-2 %. Risk is low but not zero. Mitigation: profile early (Phase 2.1 verify); if exceeded, fallback to 8 modes per voice (ROADMAP "Fallback 1") for Phase 2.1, file an ARCHITECTURE deviation note. Won't hide a fundamental issue — would surface a CPU regression worth fixing before Phase 2.4 stresses 8-voice polyphony.

5. **Allocation in startNote / processBlock.** ModeBank coefficient recompute is in-place on `std::array`. ADSR `setParameters` doesn't allocate. `juce::MidiMessage::getMidiNoteInHertz` is a `static double` returning a primitive. Exciter's `start()` resets an integer index. **No `new`, no `std::vector::push_back`, no `juce::Array::resize`.** Mitigation: research-phase Open Question #2 + #6 confirm; Phase 2.1 Gate 1 invariants (7) pluginval-5 + (8) auval re-run catches any RT-safety regression empirically. Manual `grep` for `new`, `make_unique`, `make_shared`, `push_back`, `resize`, `malloc` in `Source/ModeBank.{h,cpp}` + `Source/Exciter.{h,cpp}` + `Source/BassoonVoice.{h,cpp}` is the planning-phase mandatory check.

6. **Reference recording sourcing falls through.** Locked Q4 mitigates by sourcing during Phase 2.1 (not deferred to Phase 2.2 kickoff). If primary candidate (Philharmonia / Freesound / Iowa) is licensure-incompatible, the research-phase Open Question #7 surfaces 2-3 backup candidates. Worst case: source a personal recording from a colleague / community Slack — adds ~1 day but zero blocker.

7. **JUCE Synthesiser sub-buffer zeroing assumption.** If the assumption "`Synthesiser::renderNextBlock` zeroes the output range before iterating voices" is wrong, Phase 2.1's `addSample` summation will accumulate into whatever the host left in the buffer (typically zeros, but not guaranteed). Mitigation: research-phase Open Question #2 verifies via JUCE source; if false, switch from `addSample` to `setSample` for the first voice and `addSample` for subsequent voices — but at Phase 2.1 with single-voice testing, this never manifests. Phase 2.4 (polyphony) is when this matters; verify the assumption now to avoid late surprise.

8. **Pitch-bend semantics surprise.** If `pitchWheelMoved` is called with normalized `[-1.0, 1.0]` rather than raw 14-bit `[0, 16383]`, the bend-semis formula in Q-Pitch-bend is wrong. Mitigation: research-phase Open Question #9 confirms via JUCE source. Phase 2.1 has no automated MPE test (Phase 2.4 does), so silent miscalibration is the failure mode — but Logic AU smoke (Gate 1 item 9) with a held note + pitch-bend wheel sweep catches it qualitatively.

---

## Next Phase

Ready for: **research** phase — `/plugin-research O-Bassoon 2-dsp`

Research focus (Phase 2.1):

1. **Resolve Open Questions #1–#10** — JUCE 8.0.4 ADSR API (line cites), SynthesiserVoice render convention (line cites), biquad numerical stability at long T60 (`float` vs `double`), Phase 2.1 exciter impulse shape (concrete formula), Nyquist muting policy, MidiMessage::getMidiNoteInHertz signature, reference bassoon C3 candidate URLs + licenses, Logic CPU meter verification protocol, pitchWheelMoved raw value range, spectrum-analyzer install recommendation.
2. **Pattern-confirm against O-Wind + O-Lyrica** — voice mono-to-stereo write loop pattern (cite `Source/FluteSynthVoice.cpp` + `Source/HarpSynthVoice.cpp` exact lines). Confirm `addSample` + `clearCurrentNote()` + `modeBank.reset()` exit path is the family standard. Confirm the per-sample loop ordering (excitation → resonator → ADSR multiply → write).
3. **Lift O-Formant biquad implementation** if applicable — O-Formant uses "custom 32-byte direct-form biquad structs" per ARCHITECTURE.md research notes. Cite the file path and confirm whether to copy-with-attribution or re-implement (re-implement preferred — this is the simplest possible biquad).
4. **Pre-flight bassoon C3 reference recording** — test-download a candidate, confirm WAV format, confirm 48 kHz / 24-bit / mono or stereo, listen-check that it's a clean held C3 with minimal vibrato (early portion of a longer take). Document URL + license + ~3 s clip notes.
5. **Append RESEARCH.md** at `plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md` with §1 (Open Questions resolved), §2 (Pattern Confirmations), §3 (Implementation Skeletons — minimal pseudocode for ModeBank/Exciter/voice loop), §4 (Discrepancies — anything that contradicts CONTEXT.md or ROADMAP, with proposed resolution).

After research: plan-phase writes Phase 2.1 task breakdown verbatim against this CONTEXT + research findings; execute-phase performs the implementation; verify-phase confirms 10-item Gate 1 bar + Logic AU smoke + atomic commit.

---

## Audit Trail

**rev-1 (this document, 2026-04-27):** Phase 2.1 opening — Core Modal Voice + First Audio. 8 user-confirmed approach decisions (Q1 single Phase 2.1 cycle, Q2 strict-ROADMAP minimal wiring, Q3 on-note-on/pitch-bend coefficient cadence, Q4 reference recording sourced during Phase 2.1, Q5 atomic commit on Gate 1 PASS, Q6 DAW + tuner only, Q7 ten-item Gate 1 bar, Q8 centered mono-to-stereo write) plus 4 derived (exciter onset-buffer storage = class-level `std::array`, voice exit path via `clearCurrentNote` + `modeBank.reset()`, pitch-bend wired even without MPE testing, ADSR-exit mode-bank reset). 10 open questions handed to research-phase: ADSR API, voice-output convention, biquad numerical stability, exciter impulse shape, Nyquist muting policy, MidiMessage::getMidiNoteInHertz signature, reference recording sourcing, Logic CPU meter protocol, pitchWheelMoved raw value range, spectrum-analyzer install recommendation. 8 risks documented with mitigations.

**Inherited verbatim from Stage 1 (not re-litigated):**
- All 10 APVTS parameter IDs, ranges, defaults
- ROADMAP 4-phase DSP staging (Phase 2.1 → 2.2 → 2.3 → 2.4)
- DSP architecture (modal synthesis, 16 modes, parallel biquad, no waveguide)
- Stage 1 wiring contract (NE drain BEFORE renderNextBlock; raw-pointer voice setters)
- Stage 1 build flags (IS_SYNTH, NEEDS_MIDI_INPUT, NEEDS_WEB_BROWSER, NEEDS_WEBVIEW2, JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING, PLUGIN_CODE OBsn)
- Atomic-commit gate-first principle (Stage 1 R-foundation pattern → Phase 2.1 R-2.1 atomic commit)
- Primary listening DAW: Logic Pro (AU)
- DSP-07 (no O-Reed dependency) verified at Stage 1, carries forward

**New in rev-1:**
- Cycle scope = Phase 2.1 only (no a/b/c sub-split, no Phase 2.1 + 2.2 merge)
- Strict ROADMAP minimal wiring (no APVTS reads in voice DSP at Phase 2.1)
- Coefficient cadence on note-on + pitch-bend only (no per-block recompute)
- Reference bassoon C3 sourced during Phase 2.1 (zero blocking on Phase 2.2 kickoff)
- DAW + tuner verification (no CLI render harness this cycle)
- 10-item Gate 1 PASS bar (6 ROADMAP + auval + pluginval-5 + Logic AU smoke + spectrum baseline capture)
- Centered equal L+R per-sample voice write (matches O-Lyrica / O-Wind precedent)
- Single Phase 2.1 atomic commit with subject `feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS`
