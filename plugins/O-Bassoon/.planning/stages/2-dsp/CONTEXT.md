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

**rev-1 (2026-04-27):** Phase 2.1 opening — Core Modal Voice + First Audio. 8 user-confirmed approach decisions (Q1 single Phase 2.1 cycle, Q2 strict-ROADMAP minimal wiring, Q3 on-note-on/pitch-bend coefficient cadence, Q4 reference recording sourced during Phase 2.1, Q5 atomic commit on Gate 1 PASS, Q6 DAW + tuner only, Q7 ten-item Gate 1 bar, Q8 centered mono-to-stereo write) plus 4 derived (exciter onset-buffer storage = class-level `std::array`, voice exit path via `clearCurrentNote` + `modeBank.reset()`, pitch-bend wired even without MPE testing, ADSR-exit mode-bank reset). 10 open questions handed to research-phase: ADSR API, voice-output convention, biquad numerical stability, exciter impulse shape, Nyquist muting policy, MidiMessage::getMidiNoteInHertz signature, reference recording sourcing, Logic CPU meter protocol, pitchWheelMoved raw value range, spectrum-analyzer install recommendation. 8 risks documented with mitigations.

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

---

## rev-2 — Phase 2.2 Opening (2026-04-27)

**Cycle Scope (rev-2):** **Phase 2.2 — Bassoon Spectral Tuning + Tone Control.** Replaces Phase 2.1's placeholder integer-harmonic ratios + flat amplitudes with bassoon-tuned near-integer ratios + first-formant-Gaussian × 1/k roll-off amplitude shaping, and wires the `tone` APVTS parameter (per-mode T60 scaling for upper modes k > 4) end-to-end. A/B-vs-archived-reference-WAV listening loop is the primary acceptance signal. All other 9 APVTS parameters, vibrato, breath, polyphony, and NE/MPE remain deferred to Phases 2.3-2.4.

Phase 2.1 atomic commit landed at `d1b3370` (`feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS`). Working tree starts from that commit on `main`. Reference WAVs are already archived at `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v{1,2}.wav` (sourced during Phase 2.1 per locked Q4-rev-1).

### Cycle Scope (rev-2)

**Goal:** Voice produces a recognizable bassoon-like timbre at C3 (130 Hz) with energy concentrated in the 400-600 Hz region (first formant). `tone` parameter audibly sweeps dark↔bright across [0, 1] with no zipper noise, no clicks, and no NaN/inf. CPU at 8-voice / 48 k / 256 stays under 20 % (ROADMAP test criterion, early signal — voice cap not in place yet, simulated by holding 8 keys in Logic-AU).

**In scope:**

- `Source/ModeBank.h` — replace `PARTIAL_RATIOS` with bassoon-tuned near-integer ratios `{1.000, 2.005, 3.010, 4.018, 5.024, 6.032, 7.041, 8.052, 9.064, 10.078, 11.092, 12.108, 13.125, 14.144, 15.164, 16.186}` (verbatim from ARCHITECTURE.md §"Bassoon Partial Table"). Add `static constexpr float FORMANT_F1 = 475.0f` and `FORMANT_BW = 200.0f` constants. Add private `static float computeModeAmplitude(int k, float f0)` helper. Replace `setTone(float /*tone01*/) noexcept {}` stub with a real implementation: store the smoothed tone value, mark coefficients dirty, recompute upper-half (k > 4) `R_k` next time `setFundamental` or a tone-driven recompute fires. Drop the `1 / NUM_MODES` headroom scaler in `processSample` (formant-Gaussian + 1/k roll-off naturally constrains peak gain — verify empirically; if peaks > -3 dBFS, retain a tighter scaler informed by measured peak).
- `Source/ModeBank.cpp` — `setFundamental` calls `computeModeAmplitude(k, f0)` per mode; `setTone(toneSmoothed)` recomputes upper-half `R_k` only (mode indices 5-15 by zero-indexed convention, i.e., `k > 4`); add a `setToneAndFundamental(float tone01, float f0)` convenience for the note-on path so both fire in one pass.
- `Source/PluginProcessor.{h,cpp}` — add a single processor-level `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` for `tone` (50 ms ramp), `reset(sampleRate, 0.050)` in `prepareToPlay`. In `processBlock`, BEFORE the NE drain, read `*params.getRawParameterValue("tone")`, `setTargetValue` on the smoother, advance the smoother once per block (using `getNextValue()` after `skip(numSamples - 1)` or equivalent — confirm exact pattern in research-phase), call `voice->setTone(toneSmoothed)` on every active voice with throttled-epsilon dispatch (`if (std::abs(newTone - lastDispatchedTone) > 0.001f)`).
- `Source/BassoonVoice.{h,cpp}` — new public method `void setTone(float tone01) noexcept` that forwards to `modeBank.setTone(tone01)`. No new APVTS reads inside the voice itself; the value is pushed in by the processor (locked Q3-rev-2 (a) processor-level smoother).
- `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` — backfill the **final** partial-table values (whatever they end up as after the listening loop, even if the rev-2 starting set is shipped unchanged — document the rev that was shipped). Append a rev note at the end documenting "as-shipped Phase 2.2".

**Out of scope (deferred per ROADMAP):**

- All other 9 APVTS reads in voice DSP (vibrato_*, breath, attack_character, attack_time, release_time, voice_count, output_gain) — Phase 2.3 / 2.4 (locked Q2-rev-2 strict-ROADMAP)
- The `1/N` placeholder headroom scaler stays in place if measured peaks make it necessary; replacement by `output_gain` APVTS read is Phase 2.3
- Vibrato LFO + onset envelope — Phase 2.3
- Sustain noise component in `Exciter` — Phase 2.4 (Phase 2.1 stub remains: impulse-only)
- Attack-character morph — Phase 2.4
- Voice manager / `voice_count` enforcement — Phase 2.4 (Phase 2.2 hold-8-keys CPU measurement runs against `juce::Synthesiser` default behaviour: 16 pre-allocated voices, oldest stealing already enabled at Stage 1)
- VST3 NE consumption per-voice / `applyPendingTuning` — Phase 2.4
- MPE pitch-bend path: pitch-bend mode-bank recompute is already wired at Phase 2.1; no change needed at Phase 2.2
- TuningEngine `getFrequency()` call in startNote — Phase 2.4
- Two-register-table fallback (ARCHITECTURE Risk #2 Fallback 1) — only invoked if rev-3 listening fails the bar
- pluginval `--strictness 10` + Windows VST3 build — Stage 4

---

### Requirements Confirmed (Phase 2.2-relevant subsets of locked contracts)

- **DSP-01** (modal-synthesis voice, tuned to bassoon spectrum): Phase 2.2 satisfies the **spectral** half left open after Phase 2.1's structural verification. Final acceptance is the qualitative "bassoon-like" judgment (locked Q5-rev-2 (d): ear-only A/B + Logic Channel EQ Analyzer overlay confirming peak in 400-600 Hz region).
- **DSP-03** (tone control, dark↔bright): Phase 2.2's primary new wiring. Acceptance: `tone = 0` audibly woody, `tone = 1` audibly brighter; smooth sweep across [0, 1] click-free.
- **FUNC-01** (sustained bassoon-like tones): Phase 2.2 closes this requirement (Phase 2.1 satisfied "sustained" only).
- **PERF-01** (no allocations in `processBlock`): regression check carries forward — no new allocations in `setTone`, `computeModeAmplitude`, or the SmoothedValue dispatch.
- **QUAL-01** (no clicks during parameter sweeps): newly testable now that `tone` is the first APVTS-driven coefficient update. 50 ms ramp + throttled-epsilon dispatch is the smoothing budget.
- **PERF-02** (8-voice CPU < 25 %): early signal at Phase 2.2 against the ROADMAP-tighter 20 % bar (Phase 2.2 test criterion). Voice cap is Phase 2.4; this measurement uses 8 simultaneously-held notes in Logic-AU as a proxy.

**Deferred to Phases 2.3-2.4 / Stage 4:**
- DSP-02 (vibrato) — Phase 2.3
- DSP-04 (breath/dynamics with CC2) — Phase 2.3
- DSP-05 (attack-character morph) — Phase 2.4
- DSP-06 (NE + MPE pitch consumption) — Phase 2.4
- FUNC-02 (1-16 polyphony with cap), FUNC-04 (envelope wiring), FUNC-05 (voice stealing) — Phase 2.3-2.4
- QUAL-02 (60 s long-tone stability) — Phase 2.3 (after envelope wired)
- COMPAT-01 strict-10 + Windows / COMPAT-02 Dorico parity — Stage 4

---

### Constraints Identified (rev-2)

**Locked contracts (do NOT modify in this cycle):**

- All 10 APVTS parameter IDs, ranges, defaults — `parameter-spec-draft.md` and `PluginProcessor.cpp:25-99`. Phase 2.2 reads ONE (`tone`) from APVTS at processor level; voice DSP still reads NONE directly (locked Q2-rev-2 strict-ROADMAP, Q3-rev-2 (a) processor-level smoother).
- DSP architecture (`research/ARCHITECTURE.md`) — Phase 2.2 implements §"Bassoon Partial Table" + §"Tone / Brightness Control" verbatim; ARCHITECTURE.md is appended to (rev note + as-shipped values), not modified.
- ROADMAP Phase 2.2 spec (lines 124-152) — components, test criteria, requirements verified.
- Phase 2.1 wiring contracts that carry forward unchanged:
  - `PluginProcessor::processBlock` runs `vst3Extensions.drainAndUpdate()` BEFORE `synthesiser.renderNextBlock` (PluginProcessor.cpp:178, 182). Phase 2.2 inserts the `tone` smoother advance + voice dispatch in the same processBlock prologue; ordering stays: tone-dispatch → NE drain → renderNextBlock.
  - `BassoonVoice` member layout (parameters / tuningEngine / pendingTuningSource raw pointers, modeBank, exciter, adsr, currentFrequency, currentSampleRate). Phase 2.2 adds a new private `float currentTone = 0.5f;` member and a public `void setTone(float)` method — both additive.
  - Per-sample render-loop ordering: `exciter → modeBank → adsr → addSample(L) + addSample(R)` (BassoonVoice.cpp). Phase 2.2 doesn't touch this.
  - Mode-bank coefficient-update cadence: on note-on + on pitch-bend (locked Q3-rev-1). Phase 2.2 ADDS a third trigger: tone change > epsilon. No per-block recompute introduced.
  - Reference WAV archive (`research/reference-recordings/bassoon-c3-sustain-v{1,2}.wav` + LICENSE.md + README.md) — read-only at Phase 2.2; Phase 2.2 listens against, doesn't re-source.
- Stage 1 build flags + CMakeLists structure — Phase 2.2 only edits `Source/ModeBank.{h,cpp}` + `Source/BassoonVoice.{h,cpp}` + `Source/PluginProcessor.{h,cpp}`. CMakeLists `target_sources` already lists the affected files; no source-list edit needed.
- DSP-07 (no O-Reed dependency) — verified at Stage 1, re-grepped at Phase 2.1 verify, carries forward.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**

- `juce::ScopedNoDenormals` at `processBlock` entry — already in place. Mode bank IIR state benefits from FTZ. Phase 2.2's tone-driven recompute changes `R_k` for upper modes; the G-normalisation form `(1 - R) * amp_k` continues to prevent sub-LSB accumulation.
- `juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>` — `reset(sampleRate, rampDurationSec)` in `prepareToPlay`, `setTargetValue(target)` once per block, `getNextValue()` per sample OR `skip(numSamples)` once per block. For block-rate smoothing of a coefficient-driving parameter, the canonical pattern is one `getNextValue()` at block start + dispatch — confirm exact JUCE 8 idiom in research-phase. O-Wind / O-Lyrica precedent applies.
- Allocation-free `processBlock`: SmoothedValue is stack-only state. `computeModeAmplitude` is a pure function returning float — no allocation. Phase 2.2 mandatory grep stays the same: `new`, `make_unique`, `make_shared`, `push_back`, `resize`, `malloc` zero hits in all touched files.

**Phase 2.2-specific constraints:**

- **Strict ROADMAP — only `tone` wired this cycle** (locked Q2-rev-2). The other 9 APVTS parameters stay unread in voice DSP. The `1/N` placeholder headroom scaler in `ModeBank::processSample` remains the operative loudness control until Phase 2.3 wires `output_gain` + `breath`. **Exception:** if formant-Gaussian + 1/k roll-off measurably reduces peak energy enough that `1/N` becomes overcorrected (audibly quiet), the scaler may be relaxed to `1/8` or `1/4` as an in-cycle tuning constant (NOT an APVTS read — still Phase 2.3's job to wire `output_gain`). Document the chosen scaler in code comment.
- **Processor-level tone smoother** (locked Q3-rev-2 (a)) — single `juce::SmoothedValue<float>` in `PluginProcessor`, 50 ms ramp, advanced once per block, dispatched to all active voices via `voice->setTone(...)` only when the dispatched delta exceeds `0.001f` (locked Q4-rev-2 (b) throttled epsilon). Recomputes upper-half (k > 4) `R_k` for the dispatched voice's mode bank.
- **Tone coefficient cadence** (locked Q4-rev-2 (b)) — `ModeBank::setTone` stores the value, marks dirty; recompute fires lazily on next `setFundamental` OR explicitly via a new `applyToneChange()` call from the dispatch path. Decision deferred to research-phase: lazy-on-next-setFundamental is cleaner but couples tone updates to pitch updates; explicit-`applyToneChange` is more responsive on the slider sweep but doubles the recompute path. Research-phase locks.
- **A/B verification protocol** (locked Q5-rev-2 (d)) — primary: ear-only A/B between O-Bassoon held C3 + reference WAV looped in adjacent Logic audio track; secondary: Logic stock Channel EQ in Analyzer mode confirms peak in 400-600 Hz on held C3. SPAN remains uninstalled (item-10-rev-1 dropped from Gate 1; Gate 2 inherits the drop).
- **Iteration budget** (locked Q6-rev-2 (a)+(b)) — iterate at verify-phase inline (no replan); ceiling at rev-3. After rev-3, ship and document gap as v1.1 partial-table refinement candidate (ARCHITECTURE Risk #2 Fallback 2 framing: "bassoon-inspired").
- **Reference WAV canonical** (locked Q7-rev-2 default) — `bassoon-c3-sustain-v1.wav` is canonical primary; `bassoon-c3-sustain-v2.wav` is secondary cross-check. If v1's pitch / clip quality is unsuitable, fall back to v2 (recorded both versions for exactly this reason — Phase 2.1 sourced both). README at `research/reference-recordings/README.md` documents the audition checklist.
- **8-voice CPU early signal** (locked Q8-rev-2) — hold 8 keys simultaneously in Logic-AU during verify, capture Logic CPU-meter reading. Bar: < 20 % (ROADMAP Phase 2.2 test criterion). If exceeded, trigger ARCHITECTURE Risk #1 Fallback 1 (drop to 8 modes per voice) BEFORE finalising the partial table — cheaper to swap pre-tune than post-tune.
- **Atomic commit on Gate 2 PASS** (locked Q9-rev-2) — single commit lands `Source/ModeBank.{h,cpp}` + `Source/BassoonVoice.{h,cpp}` + `Source/PluginProcessor.{h,cpp}` edits + `research/ARCHITECTURE.md` rev-note backfill + 5 planning artefacts (CONTEXT-rev-2 / RESEARCH-rev-2 / PLAN-rev-2 / SUMMARY-rev-2 / VERIFICATION-rev-2 / STATUS update) on Gate 2 PASS only. Subject pattern: `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`.

**Working-tree starting state (locked from Phase 2.1 atomic commit `d1b3370` on `main`):**

- `Source/ModeBank.{h,cpp}` — 16-mode pole-only resonator bank, integer-harmonic placeholder ratios, flat amplitudes, `setTone` no-op stub, `1/N` headroom scaler, isfinite NaN guard, Nyquist muting at `0.45 × fs`.
- `Source/Exciter.{h,cpp}` — 5 ms half-sine × exp impulse, peak-normalised, class-level `std::array<float, 1024>` storage, no allocation at runtime. Phase 2.2 leaves untouched.
- `Source/BassoonVoice.{h,cpp}` — per-sample render loop (`exciter → modeBank → adsr → addSample(L,R)`), pitch-bend ±2 semis (raw 14-bit), full state-reset on voice exit, NO APVTS reads / NO TuningEngine call. Phase 2.2 adds `setTone(float)` public method only.
- `Source/PluginProcessor.{h,cpp}` — APVTS (10 params), 16-voice `juce::Synthesiser`, headless TuningEngine, NE drain BEFORE renderNextBlock at PluginProcessor.cpp:178 → :182. Phase 2.2 inserts tone smoother + voice dispatch BEFORE the NE drain.
- `CMakeLists.txt` — sources unchanged at Phase 2.2 (no new files).
- Build state: `O-Bassoon_VST3` + `O-Bassoon_AU` + `O-Bassoon_Standalone` install fresh post-Phase-2.1 commit; `auval -v aumu OBsn OuDv` SUCCESS; `pluginval --strictness 5` SUCCESS.

---

### Approach Decisions (rev-2)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q1-rev-2 — Cycle scope** | **Phase 2.2 only** (single GSD cycle): partial-ratio replacement + formant-Gaussian amplitude shaping + `tone` APVTS wiring + A/B-vs-reference listening loop. Defers envelope/breath/vibrato/polyphony/NE to Phases 2.3-2.4. | A/B-vs-recording listening loop is the highest-uncertainty step Stage 0 staged into a dedicated phase. Merging with Phase 2.3 would dilute the listening signal (envelope + breath shape would confound timbre judgment). Single-cycle scope matches Phase 2.1 / O-Contrabass / O-Wind precedent. User-confirmed (all defaults). |
| **Q2-rev-2 — APVTS wiring scope** | **Strict ROADMAP — only `tone` wired this cycle.** All other 9 APVTS parameters stay unread in voice DSP. `1/N` headroom scaler stays as in-cycle tuning constant (may be relaxed to 1/8 or 1/4 if formant + 1/k roll-off measurably reduces peak energy, but **not** replaced by an APVTS read). | Keeps the listening loop unconfounded by amplitude / envelope state. `output_gain` wiring at Phase 2.3 is the right home — at Phase 2.2 we're listening for timbre, not loudness. User-confirmed. |
| **Q3-rev-2 — `tone` smoothing & dispatch** | **(a) Processor-level smoother.** Single `juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` in `PluginProcessor`, 50 ms ramp, advanced once per block. Dispatched to every active voice via `voice->setTone(toneSmoothed)`. | Single source of truth for tone state across all voices. No per-voice phase drift on rapid sweeps. Cheaper (one smoother vs. 16). Matches O-Wind / O-Lyrica processor-level smoother precedent. User-confirmed. |
| **Q4-rev-2 — Coefficient cadence for `tone`** | **(b) Throttled epsilon.** Recompute upper-half (k > 4) `R_k` only when `\|toneSmoothed - lastDispatchedTone\| > 0.001f` (epsilon roughly = 0.1 % of tone range). Throttle gate is at the processor dispatch site (skip the `voice->setTone` call when within epsilon). | Saves ~12 cos/exp evaluations per block when tone is static (the 99 % case during sustained playback). Empirically inaudible at the chosen epsilon — 0.1 % tone change = sub-cent T60 shift in upper modes. Matches family pattern; lazy-vs-eager recompute internal to ModeBank deferred to research-phase. User-confirmed. |
| **Q5-rev-2 — Gate 2 PASS bar** | **(d) Ear-only A/B + Logic Channel EQ Analyzer.** Primary: hold C3 in Logic, switch between O-Bassoon and looped reference WAV, listen for "same neighborhood" timbre. Secondary: Logic stock Channel EQ in Analyzer mode overlay confirms peak in 400-600 Hz region on held C3. SPAN stays uninstalled. | Belt-and-braces. Ear catches timbre quality the spectrum misses; spectrum catches first-formant location quantitatively. Logic EQ Analyzer is free, already installed, and adequate for peak-region check. User-confirmed. |
| **Q6-rev-2 — Iteration budget** | **(a) + (b).** Iterate inline at verify-phase (no replan loop). Ceiling at rev-3 — after that, ship with current partial table and document gap as v1.1 candidate per ARCHITECTURE Risk #2 Fallback 2 framing ("bassoon-inspired"). | ROADMAP says "2-3 iterations". Inline iteration matches Phase 2.1 manual-subset pattern (verify is where the listening happens; if it fails, tweak partials inline rather than reopen plan). rev-3 ceiling prevents endless polish — partial-table refinement is exactly the kind of work that's better served by a v1.1 dedicated phase if the v1.0 ear bar fails. User-confirmed. |
| **Q7-rev-2 — Reference WAV canonical** | **`bassoon-c3-sustain-v1.wav` canonical primary;** `bassoon-c3-sustain-v2.wav` secondary cross-check. | Default to v1 (lower numbered, sourced first). Both files were archived in Phase 2.1 specifically as backup for each other. If v1's pitch/clip quality is unsuitable on first audition, fall back to v2. README at `research/reference-recordings/README.md` documents the audition checklist (octave-convention check D4 from Phase 2.1 RESEARCH still applies). User-confirmed (default — assumes v1 is canonical until first audition reveals otherwise). |
| **Q8-rev-2 — 8-voice CPU early signal** | **Hold 8 keys in Logic-AU during verify.** Capture Logic CPU-meter reading, bar < 20 %. If exceeded, trigger ARCHITECTURE Risk #1 Fallback 1 (drop to 8 modes per voice) BEFORE finalising the partial table. | Free measurement (Logic CPU meter), early signal lets us swap to 8 modes pre-tune (cheap) rather than post-tune (expensive — partial table would need re-tuning for the reduced mode count). Voice cap (Phase 2.4) not in place yet, but `juce::Synthesiser` default polyphony allocates up to 16 voices on 16 simultaneous note-ons, and Logic-AU sends MIDI per held key. User-confirmed. |
| **Q9-rev-2 — Atomic commit unit** | **Single Phase 2.2 atomic commit on Gate 2 PASS.** Subject: `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`. Lands ModeBank + BassoonVoice + PluginProcessor edits + ARCHITECTURE.md rev-note backfill + 6 planning artefacts in ONE commit. | Mirrors Phase 2.1 / O-Contrabass / O-Lyrica precedent. Per-task commits break the gate-first principle (intermediate commits would land mid-tone-wiring with audible tone-sweep zipper noise — fails QUAL-01 mid-flight). User-confirmed. |
| Throttle-gate location | **At the processor dispatch site** (skip `voice->setTone` when within epsilon), NOT inside `ModeBank::setTone`. | Keeps ModeBank's `setTone` unconditional — simpler invariant, easier unit-testing if we add tests later. Throttle is a transport-layer concern. |
| `setTone` recompute path | **Lazy-on-next-setFundamental OR explicit-`applyToneChange`** — research-phase locks. Default fallback: explicit `applyToneChange()` called from the throttle-gate dispatch path, recomputes upper-half `R_k` immediately. | Deferred to research-phase to verify which form composes cleanly with Phase 2.1's pitch-bend-driven `setFundamental` re-trigger (don't want a latent tone update to surprise-fire mid-bend). |
| Tone amplitude vs. T60 scope | **T60 scaling only** at Phase 2.2 (ARCHITECTURE.md spec). Per-mode amplitude `computeModeAmplitude(k, f0)` is f0-driven (formant-Gaussian + 1/k roll-off), NOT tone-driven. | Locks the architecture. `tone` controls damping (ring time of upper modes), not the spectral envelope shape. Spectral envelope is fundamental-frequency-dependent only. |
| Mode-index convention for "k > 4" | **Zero-indexed:** mode indices 5-15 (i.e., k > 4 means k ∈ {5, 6, ..., 15}). Modes 0-4 (the formant-region modes) are NOT tone-scaled. | ARCHITECTURE.md uses zero-indexed `k` throughout (lines 374-382 `computeModeAmplitude(int k, ...)` is zero-indexed). Lock here to avoid off-by-one when implementing. |

---

### Open Questions (handed to research-phase) — rev-2

1. **`juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` block-rate advance idiom.** What's the canonical JUCE 8.0.4 pattern for advancing once per block when the smoothed value is consumed at block start (not per-sample)? Options: (a) `getNextValue()` then `skip(numSamples - 1)`, (b) explicit loop calling `getNextValue()` numSamples times and using the last, (c) `skip(numSamples)` then `getCurrentValue()`. Confirm the no-allocation form. Lookup: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_SmoothedValue.h`. Output: cite line numbers; pick the cheapest correct form for processor-level dispatch. Cross-check O-Wind `Source/PluginProcessor.cpp` SmoothedValue usage for family precedent.

2. **Tone recompute path inside ModeBank — lazy vs. explicit.** Two designs:
   - (lazy) `setTone(t)` stores the value, marks dirty; next `setFundamental(f0)` reads dirty + tone and recomputes everything together. Tone changes alone never trigger a recompute.
   - (explicit) `setTone(t)` stores the value AND immediately recomputes upper-half `R_k` (modes 5-15) using cached f0 and Q (no exp/cos for theta — those depend on f0 only). `setFundamental(f0)` does a full recompute. Two recompute paths; tone alone fires the cheaper one.
   
   The (lazy) form is simpler but couples tone responsiveness to pitch updates — a tone slider sweep on a held note (no pitch change) would not retake effect until the next `setFundamental` trigger (i.e., next note-on or pitch-bend). User-perceptible lag. Recommend (explicit). Research-phase verifies the recompute decomposition is correct (specifically: `R_k = exp(-1/(tau_k * fs))` only depends on tau_k and fs; theta depends on f_k = f0 × ratio; G = (1-R) × amp depends on R and amp). At Phase 2.2 the per-mode `amp` is f0-dependent (formant-Gaussian) so changing R also forces a G recompute — but that's still cheaper than a full theta recompute. Output: lock the design, write the per-mode update sequence verbatim into the implementation skeleton.

3. **Bassoon partial-ratio table verification.** ARCHITECTURE.md §"Bassoon Partial Table" (lines 357-362) gives the 16-element ratio table verbatim. Source for these specific values (Bassoon Operator blog? Kopp Reeds? Carillon DAFx? Author's curated set?) — verify the source and whether there's a "more correct" table from any of the references. If the table is the author's curated synthesis, document that explicitly. Output: RESEARCH.md notes the source + any candidate alternative table.

4. **Formant-Gaussian peak normalisation across f0.** The amplitude weight `formantWeight = exp(-0.5 * dist^2)` peaks at `f_k == FORMANT_F1 == 475 Hz`. For a held C3 (130 Hz), the harmonic closest to 475 Hz is the 4th (520 Hz) — modest distance, modest weight. For a held C5 (523 Hz), the 1st partial IS the formant — full unity weight. For a held C1 (32.7 Hz), the closest partial is the 14th-15th — weight is ~0.3. Net effect: voice loudness varies with played pitch (low notes quieter, formant-peaking notes louder). Is this acceptable, or do we need per-note normalisation (sum of all `formantWeight × rollOff` over modes, then scale so the total equals a constant)? ARCHITECTURE.md is silent. Research-phase recommends a path: (a) accept the natural loudness variation (mirrors real bassoon, where low notes are physically quieter); (b) per-note total-weight normalisation; (c) hybrid — normalise within ±1 octave of formant, accept variation outside. Output: recommend (a) for v1.0; document; revisit at Phase 2.3 when `breath` provides user-controllable loudness.

5. **`1/N` headroom scaler retention vs. relaxation.** With formant-Gaussian + 1/k roll-off, peak amplitude per voice is bounded by max single-mode peak gain × per-mode amp at formant. Empirically: at C3, the 3rd-4th harmonic falls at 390-520 Hz with formant-weight ≈ 0.97-0.99 × roll-off ≈ 0.5-0.4 = effective amp 0.4-0.49. The biquad has unity peak gain by construction (G = (1-R)×amp), so peak voice output ≈ ~0.5 single-mode + smaller contributions from neighbors ≈ 0.6-0.7 mono peak, which means 16-mode sum is far from `NUM_MODES`-times-unity. The `1/16` scaler is overcorrected by ~6-10 dB once the partial table is tuned. Relax to `1/8` or `1/4`? Or leave alone and let Phase 2.3's `output_gain` (default 0 dB) pull loudness up via `breath`-default-0.7? Research-phase: estimate the empirical peak, recommend the scaler value (or stay at `1/N`), and document reasoning. Output: locked scaler value or "stay 1/16, Phase 2.3 wiring fixes loudness".

6. **Logic Channel EQ Analyzer — peak-region readout protocol.** Logic's stock Channel EQ has an Analyzer mode that overlays the input spectrum on the EQ curve. To verify "peak in 400-600 Hz at held C3", the protocol is: insert Channel EQ post-O-Bassoon, set EQ to flat (or bypass — confirm Analyzer works in bypass), hold C3 sustained, observe spectrum overlay, confirm visible peak in 400-600 Hz region. Verify the Analyzer's frequency-axis reads in Hz with sufficient resolution to read 400-600 Hz peaks (vs. log-axis with cluttered low-end labels). Document the procedure including screenshot capture for archival (mirrors Phase 2.1 dropped item 10 — Phase 2.2 captures one for the as-shipped state). Output: RESEARCH.md notes the exact protocol + any Analyzer-mode caveats.

7. **`tone = 0` woody / `tone = 1` bright — qualitative descriptors clarification.** ARCHITECTURE.md says `mix(0.3, 1.5, tone)` for upper-mode T60 scaling. At `tone = 0`, upper modes decay 3× faster (T60 × 0.3) — they damp out almost immediately, leaving only the formant-region modes ringing → "woody, dark." At `tone = 1`, upper modes decay 50 % slower (T60 × 1.5) — sustained brightness from upper harmonics. Verify the math: at BASE_T60[8] = 0.8 s × 0.3 = 0.24 s ring at tone=0; × 1.5 = 1.2 s ring at tone=1. Audibility of T60 differences at the ~250 ms scale is well-documented (ear sensitivity for ringing decay differences is high). Confirm the descriptors map to listener perception, not a bug in the spec. Output: research-phase predicts the audible character at extremes; verify in plan-phase by checking O-Wind / O-Bowed precedents (similar T60-modulation tone controls).

8. **Reference WAV pre-flight audition (D4 carry-forward).** Phase 2.1 RESEARCH flagged D4: VSCO C3 octave-convention check (the WAV may be labelled C3 but actually be C2 or C4 depending on source-library convention). Phase 2.1 dropped this from Gate 1 (item 10 + dropped) but Phase 2.2 needs it resolved. Audition the v1 WAV — what fundamental does it actually play? Use a tuner against a sustained section. If it's not C3 (130.8 Hz), update the README pitch annotation and re-source if needed. Output: RESEARCH.md confirms actual pitch + updates README if mismatch.

9. **8-voice CPU measurement protocol in Logic-AU.** Phase 2.1 verified 1-voice CPU < 5 % via Logic CPU meter (Phase 2.1 Q8 + locked). For 8 voices, hold 8 keys simultaneously: Logic spawns 8 voices in O-Bassoon (since voice cap is 16 default). Logic CPU meter reads aggregate process-level CPU. Confirm the 20 % bar is measured against the same meter mode (System Performance Meter / Process bar — Phase 2.1 RESEARCH locked). Document the exact key combination (e.g., C3 chord-clusters across two octaves to ensure no voice-stealing). Output: RESEARCH.md notes the key set + reading method + any "8 voices vs. 8 simultaneous note-ons" subtlety.

10. **ARCHITECTURE.md backfill format.** Phase 2.2 lands the bassoon-tuned partial table verbatim from ARCHITECTURE.md §"Bassoon Partial Table". If the partial values are shipped unchanged (rev-1 ratios), the backfill is just an append-rev-note "as-shipped Phase 2.2 — partial table values match §Bassoon Partial Table verbatim". If iteration revs the table (rev-2 / rev-3 listening loop changes any value), the backfill writes the new values into a §"Phase 2.2 As-Shipped Partial Table" subsection adjacent to the original spec. Decide format upfront so the verify-phase commit doesn't have to invent it. Output: RESEARCH.md locks the backfill template (filename, section heading, expected diff size).

---

### Risks (Phase 2.2-specific)

1. **Partial table doesn't sound bassoon-like at rev-1 listening.** Highest-likelihood risk per ARCHITECTURE.md §"Implementation Risks" Risk #2 (MEDIUM). The partial values + formant location are research-derived hypotheses, not measured-from-recording. Mitigation: locked Q6-rev-2 inline iteration ceiling at rev-3; ARCHITECTURE Risk #2 Fallback 1 (two-register tables) is a structural option for rev-2/3; Fallback 2 (ship as "bassoon-inspired") is the soft-landing if rev-3 still fails the bar.

2. **Tone sweep introduces zipper noise / clicks.** Phase 2.2 is the first APVTS-driven coefficient update — the smoothing and throttling are both new code paths. Mitigation: 50 ms `Linear` smoother (locked Q3-rev-2) + throttled-epsilon dispatch (locked Q4-rev-2 (b)); QUAL-01 verification at Gate 2 is a parameter-sweep listen-test. If zipper persists, fall back to per-block lerp between current and target coefficients (more code, but eliminates the throttle's discrete-step character).

3. **8-voice CPU exceeds 20 %.** ARCHITECTURE.md estimates 16 % at 8 voices on M1 (within budget). Phase 2.2 adds formant-Gaussian + per-mode amp computation — modest per-voice cost, but the 16-mode coefficient recompute on every `setFundamental` (16 × ~10 flops) was already in Phase 2.1's budget. Phase 2.2's tone recompute (12 modes × ~3 flops if `R_k`-only path) is throttled. Mitigation: locked Q8-rev-2 early measurement; ARCHITECTURE Risk #1 Fallback 1 (8 modes per voice) BEFORE finalising the partial table if exceeded.

4. **Loudness regression vs. Phase 2.1.** Phase 2.1's 16-flat-amplitude modes summed to peak ~3-5 (before `1/N` scaler), giving a roughly even-loudness output. Phase 2.2's formant-Gaussian + 1/k roll-off attenuates most modes — the post-`1/N` voice peak likely drops 6-10 dB below Phase 2.1's level. If the user perceives Phase 2.2 as "quieter than Phase 2.1," that's a regression even if technically expected. Mitigation: locked Q5-rev-2 (relax `1/N` to `1/8` or `1/4` if measured peak post-tuning is too low — research-phase Open Question #5 locks the value).

5. **Tone `mix(0.3, 1.5, tone)` extremes sound artificial.** At `tone = 0`, T60 × 0.3 = 0.24 s for mode 8 — that's "thunk" decay, not "woody." If the upper-half modes damp too aggressively, the voice loses harmonic richness and starts to sound like a band-limited fundamental + formant only. Mitigation: research-phase Open Question #7 (qualitative descriptor clarification) flags this for ear-check at verify-phase rev-1; if confirmed, narrow the mix range (e.g., `mix(0.5, 1.3, tone)`) — small ARCHITECTURE.md deviation, document as Phase 2.2 deviation.

6. **`tone` smoothing & dispatch latency.** 50 ms ramp + throttled-epsilon dispatch means a UI tone change takes ~50 ms to fully apply. For most users this is imperceptible (they're sweeping the knob over hundreds of ms). For DAW automation, 50 ms is sub-block-rate and fine. For a user expecting instant feedback, 50 ms could feel "soggy." Mitigation: 50 ms is the ARCHITECTURE.md spec — don't deviate without listening evidence. If post-listen the response feels too slow, reduce to 25 ms (likely zipper-prone) and re-verify.

7. **Reference WAV pitch mismatch (D4 carry-forward).** v1 WAV may not actually play C3 (130.8 Hz) if the source library uses a non-standard octave convention. Mitigation: locked research-phase Open Question #8 — audition with tuner; re-annotate or re-source if mismatch.

8. **ARCHITECTURE.md backfill drift from as-shipped values.** If iteration revs the partial table, ARCHITECTURE.md must reflect the as-shipped state OR explicitly document the deviation. Risk: backfill is forgotten and ARCHITECTURE.md ends up out-of-sync with code. Mitigation: locked research-phase Open Question #10 + plan-phase task explicitly lists `research/ARCHITECTURE.md` as a Phase 2.2 atomic-commit deliverable; verify-phase grep checks the partial-ratio block in `Source/ModeBank.h` matches the values in ARCHITECTURE.md §"Bassoon Partial Table" (or its as-shipped subsection).

---

### Next Phase

Ready for: **research** phase — `/plugin-research O-Bassoon 2-dsp`

Research focus (Phase 2.2):

1. **Resolve Open Questions #1–#10** — SmoothedValue block-rate idiom, ModeBank tone recompute path (lazy vs. explicit), bassoon partial-ratio source verification, formant-Gaussian peak normalisation, `1/N` scaler retention/relaxation, Logic EQ Analyzer protocol, tone descriptor verification, reference WAV pitch audition, 8-voice CPU protocol, ARCHITECTURE.md backfill format.
2. **Pattern-confirm against O-Wind + O-Lyrica** — processor-level SmoothedValue dispatch loop pattern (cite `Source/PluginProcessor.cpp` exact lines for both); throttled-epsilon dispatch precedent if any (likely O-Wind for `breath` / `output_gain`).
3. **Verify ARCHITECTURE.md §"Bassoon Partial Table" math** — `computeModeAmplitude(k, f0)` formula (lines 374-382), peak amplitude across MIDI 24-84 range, formant-weight × roll-off product at extreme f0 values; surface any latent issue before plan-phase commits to the implementation skeleton.
4. **Pre-flight reference WAV audition** — load v1 in Logic, sustain a section, tuner-check fundamental, confirm C3 (or document the actual pitch + plan re-source if mismatch).
5. **Append RESEARCH.md** at `plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md` (rev-2) with §1 (Open Questions resolved), §2 (Pattern Confirmations), §3 (Implementation Skeletons — ModeBank `setTone` + `computeModeAmplitude` + processor smoother dispatch), §4 (Discrepancies — anything that contradicts CONTEXT-rev-2 or ARCHITECTURE.md).

After research: plan-phase writes Phase 2.2 task breakdown verbatim against this CONTEXT-rev-2 + research findings; execute-phase performs the implementation; verify-phase confirms Gate 2 listening + EQ-Analyzer protocol + 8-voice CPU + atomic commit.

---

### Audit Trail (rev-2 addendum)

**rev-2 (this addendum, 2026-04-27):** Phase 2.2 opening — Bassoon Spectral Tuning + Tone Control. 9 user-confirmed approach decisions (Q1 Phase 2.2-only cycle, Q2 strict-ROADMAP `tone`-only wiring, Q3 processor-level smoother, Q4 throttled-epsilon coefficient cadence, Q5 ear + Logic EQ Analyzer Gate 2 bar, Q6 inline iteration with rev-3 ceiling, Q7 v1 WAV canonical, Q8 hold-8-keys CPU early signal, Q9 atomic commit on Gate 2 PASS) plus 4 derived (throttle gate at processor dispatch site, lazy-vs-explicit setTone recompute deferred to research, T60-only scope for tone, zero-indexed k > 4 convention). 10 open questions handed to research-phase: SmoothedValue idiom, setTone recompute decomposition, partial-ratio source, formant-peak normalisation across f0, `1/N` scaler relaxation, Logic EQ Analyzer protocol, tone descriptor verification, reference WAV pitch audition, 8-voice CPU protocol, ARCHITECTURE.md backfill format. 8 risks documented with mitigations.

**Inherited verbatim from Phase 2.1 (not re-litigated):**
- Per-sample render loop ordering (`exciter → modeBank → adsr → addSample`)
- Centered equal L+R per-sample voice write
- Mode-bank coefficient cadence trigger on note-on + pitch-bend (Phase 2.2 ADDS tone-change > epsilon as a third trigger)
- NE drain BEFORE renderNextBlock at PluginProcessor.cpp:178 → :182
- DSP-07 (no O-Reed dependency) verified at Stage 1
- Reference WAVs archived at `research/reference-recordings/` (sourced during Phase 2.1)
- Atomic-commit gate-first principle
- Primary listening DAW: Logic Pro (AU)
- DAW + tuner verification (no CLI render harness)

**New in rev-2:**
- Cycle scope = Phase 2.2 only (partial-table replacement + formant-Gaussian amplitudes + `tone` wiring + A/B listening)
- One APVTS read added to processor (`tone`); voice DSP still reads zero APVTS directly
- Processor-level `juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` for `tone`, 50 ms ramp
- Throttled-epsilon dispatch with epsilon = 0.001 (gate at processor dispatch site, NOT inside ModeBank)
- Mode-index convention "k > 4" = zero-indexed modes 5-15
- Gate 2 PASS bar: ear-only A/B (primary) + Logic Channel EQ Analyzer overlay (secondary)
- Inline iteration at verify-phase, ceiling at rev-3
- Reference WAV canonical = v1; v2 = secondary cross-check
- 8-voice CPU early signal (hold 8 keys in Logic-AU, < 20 % bar)
- Atomic commit on Gate 2 PASS with subject `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`
- ARCHITECTURE.md backfill (as-shipped partial table) is a Phase 2.2 atomic-commit deliverable

---

## rev-3 — Phase 2.3 Opening (2026-04-28)

**Cycle Scope (rev-3):** **Phase 2.3 — Per-Note Expression: Envelope, Breath, Vibrato, Output Gain.** Wires four APVTS-driven systems: (1) `juce::ADSR` to `attack_time` (0-2000 ms) + `release_time` (0-3000 ms); (2) breath/dynamics with multiplicative composition `breath_voice = ui_breath × cc2_normalised` (velocity sets initial UI breath at startNote, replaced once CC2 events arrive); (3) per-voice sine-LFO vibrato with onset envelope (rate 0-10 Hz, depth 0-100 cents, onset 0-2000 ms); (4) post-summation `output_gain` (-24 to +6 dB). **Architectural pivot:** Phase 2.3 introduces a continuous filtered-noise excitation source feeding `ModeBank` — converts the Phase 2.2 struck-modal architecture (which decays at T60 ≈ 2.5 s for the fundamental) into true breath-driven sustain that maintains amplitude indefinitely while held + breath > 0. The Phase 2.1 impulse exciter is dropped from the voice render path (file retained for Phase 2.4 attack-character morph); the Phase 2.2 rev-3 `strike()` modal-state injection is retained at note-on as the attack transient. `attack_character` morph (DSP-05), polyphony cap (FUNC-02 / FUNC-05), VST3 NE per-voice consumption, MPE pitch-bend per-channel, and TuningEngine `getFrequency()` call remain deferred to Phase 2.4 per ROADMAP.

Phase 2.2 atomic commit landed at `baac74f` (`feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`) on 2026-04-28. Working tree starts from that commit on `main` with the rev-3 `strike()` patch in place + bassoon-tuned partial table + `tone` SmoothedValue dispatch + 1/8 headroom scaler. Phase 2.3 builds on this exactly.

### Cycle Scope (rev-3)

**Goal:** Voice produces a breath-driven sustained bassoon-like tone with full ADSR shaping, audibly-musical vibrato (onset-delayed), CC2-controlled real-time dynamics, and master `output_gain` post-summation. Held note for 60 s with vibrato + CC2 active maintains amplitude with no drift / NaN / denormal slowdown / CPU drift. All parameter sweeps (attack_time, release_time, breath, output_gain, vibrato_*) are click-free and zipper-free. CPU at 8-voice / 48 k / 256 with vibrato + breath active stays under 20 % (early signal — voice cap is Phase 2.4).

**In scope:**

- `Source/Vibrato.{h,cpp}` (NEW) — per-voice helper. Members: phase accumulator (`float phase = 0.0f`), sample-rate-cached phase increment recompute on rate-change, onset `juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` (linear ramp 0→1 over `vibrato_onset` ms). Public API: `prepare(double sampleRate)`, `reset()` (called from BassoonVoice::startNote — phase = 0, onset.reset(0.0f) → setTargetValue(1.0f)), `setRateHz(float rate)`, `setDepthCents(float depthCents)`, `setOnsetMs(float onsetMs)` (forwards to onset.reset(sampleRate, onsetMs/1000.0)), `getCurrentCents() noexcept` (returns `depthCents × onset.getNextValue() × std::sin(phase)` and advances phase). Allocation-free. Phase increment formula: `phaseIncrement = 2π × rate / sampleRate`.
- `Source/NoiseExciter.{h,cpp}` (NEW) — per-voice continuous filtered-noise excitation source. Members: `juce::Random rng` (seeded per-voice from `juce::Time::currentTimeMillis() ^ voiceIndex`), 1-pole low-pass filter state (`float lpState = 0.0f`), cached `float lpCoeff` (computed in `prepare(double sampleRate)` for cutoff = 2000 Hz). Public API: `prepare(double sampleRate)`, `reset()` (lpState = 0), `getNextSample(float breathScaled) noexcept` (returns `lpState += lpCoeff × (rng.nextFloat() × 2.0f - 1.0f - lpState); return lpState × BASE_NOISE_GAIN × breathScaled;`). `BASE_NOISE_GAIN = 0.05f` (starting point — research-phase verifies/tunes against Phase 2.2 modal IR peak ~0.15-0.25). Allocation-free. `juce::Random` is per-voice (no shared mutable state), thread-safe by construction at the audio-thread layer.
- `Source/BassoonVoice.{h,cpp}` (MOD) — additive members: `Vibrato vibrato`, `NoiseExciter noiseExciter`, `juce::SmoothedValue<float, ValueSmoothingTypes::Linear> breathSmoother` (20 ms ramp), `float lastDispatchedFrequency = 0.0f` (for >0.1 Hz frequency-change throttle on setFundamental). Remove `Exciter exciter` from `renderNextBlock` call path (member retained for Phase 2.4 attack-character morph re-introduction; file unchanged). `prepareToPlay`: call `vibrato.prepare(sampleRate)`, `noiseExciter.prepare(sampleRate)`, `breathSmoother.reset(sampleRate, 0.020)`, `adsr.setSampleRate(sampleRate)`. `startNote`: read `*parameters->getRawParameterValue("attack_time")` + `release_time` once → `adsr.setParameters({attack/1000, 0.0, 1.0, release/1000})`; call `modeBank.setFundamental(midiToFreq, sampleRate)`; call `modeBank.strike()` (retain rev-3 transient injection); call `adsr.noteOn()`; reset `vibrato.reset()`, `noiseExciter.reset()`, `breathSmoother.setCurrentAndTargetValue(velocity)` (initial UI breath = velocity-normalised; CC2 takes over on first event). `controllerMoved(2, val)`: `breathSmoother.setTargetValue(val / 127.0f)` (CC2-takeover semantics). `renderNextBlock`: per-block prologue — sample `vibrato.getCurrentCents()` once, compute `f_final = currentFrequencyBase × pow(2, vibratoCents/1200) × pow(2, pitchBendSemitones/12)`, if `|f_final - lastDispatchedFrequency| > 0.1f` call `modeBank.setFundamental(f_final)` and update `lastDispatchedFrequency`. Per-sample inner loop: `breath = breathSmoother.getNextValue(); excitation = noiseExciter.getNextSample(breath); voice = modeBank.processSample(excitation); voice *= adsr.getNextSample(); outputBuffer.addSample(0/1, ...)`. ADSR-idle exit unchanged (`clearCurrentNote()` + `modeBank.reset()`).
- `Source/PluginProcessor.{h,cpp}` (MOD) — additive members: per-voice dispatch loop for `attack_time`, `release_time`, `vibrato_rate`, `vibrato_depth`, `vibrato_onset`, `breath` (UI value only — CC2 routes via voice's `controllerMoved`); processor-level `juce::SmoothedValue<float, ValueSmoothingTypes::Linear> outputGainSmoother` (30 ms ramp). `prepareToPlay`: existing tone smoother reset + `outputGainSmoother.reset(sampleRate, 0.030)`. `processBlock` prologue (BEFORE NE drain, AFTER tone smoother dispatch): read `*params.getRawParameterValue("attack_time")`, `release_time`, `vibrato_rate`, `vibrato_depth`, `vibrato_onset`, `breath` → dispatch to every active voice via `voice->setExpression(...)` aggregate setter (single call per voice per block — minimises virtual-dispatch overhead). Read `output_gain` (dB) → `Decibels::decibelsToGain(...)` → `outputGainSmoother.setTargetValue(linearGain)`. After `synthesiser.renderNextBlock` returns: per-sample `buffer.applyGainRamp(0, numSamples, outputGainSmoother.getCurrentValue(), outputGainSmoother.getNextValue())` OR `buffer.applyGain(outputGainSmoother.getNextValue())` per block (research-phase locks the JUCE 8 idiom for declick-safe applyGain with `juce::SmoothedValue`). Throttled-epsilon dispatch (matches Phase 2.2 pattern) for ADSR + vibrato params (`|new - lastDispatched| > 0.001f` per param) — skips `setExpression` calls when all six APVTS values are within epsilon.
- `BassoonVoice` `setExpression(float attackMs, float releaseMs, float vibRate, float vibDepth, float vibOnsetMs, float uiBreath)` — aggregate setter to receive per-block dispatch from processor. Calls `adsr.setParameters` only when attack/release change > epsilon; calls `vibrato.setRateHz/setDepthCents/setOnsetMs` per sub-param epsilon; updates `breathSmoother.setTargetValue(uiBreath)` when no recent CC2 activity (CC2-takeover state machine: `bool cc2Active = false; juce::int64 lastCC2Sample = 0;` — if `lastCC2Sample` within last 500 ms of samples processed, ignore UI breath; else accept. Research-phase locks the takeover-window value).
- `plugins/O-Bassoon/CMakeLists.txt` (MOD) — `target_sources` adds `Source/Vibrato.cpp` + `Source/NoiseExciter.cpp`. Stage 1 build flags + `juce_generate_juce_header` order unchanged.
- `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` — append rev-3 note documenting the Phase 2.3 architectural pivot (struck-modal → continuous-noise-excited modal). Document the `BASE_NOISE_GAIN` value as-shipped + LP cutoff. Document the breath/CC2-takeover state machine. Document vibrato compose-order with future NE/MPE multiplicative composition (Phase 2.4 will add `pendingTuning` factor).

**Out of scope (deferred per ROADMAP):**

- `attack_character` morph (DSP-05) — Phase 2.4. Phase 2.3 keeps the strike() transient as the only attack mechanism; Phase 2.4 introduces soft↔tongued attack-character crossfade by reusing the Phase 2.1 `Exciter` impulse layer (or a new pre-computed shape table).
- Sustain-noise component placed via `attack_character` (different code path than Phase 2.3's continuous breath-noise) — Phase 2.4. *Note:* Phase 2.3's `NoiseExciter` is the primary continuous excitation source; Phase 2.4 may augment with attack-only impulse layer but does NOT replace `NoiseExciter`.
- `voice_count` APVTS read + `BassoonSynthesiser::findFreeVoice` override — Phase 2.4
- VST3 Note Expression per-voice consumption (`applyPendingTuning`) — Phase 2.4 (drain wires already in place; voices ignore the table at Phase 2.3)
- MPE pitch-bend per-channel (current Phase 2.1/2.2 path uses raw 14-bit `pitchWheelValue` already — Phase 2.4 may refine for per-channel MPE in DAW)
- TuningEngine `getFrequency()` call in `startNote` (Phase 2.4 — at Phase 2.3 we still use plain `MidiMessage::getMidiNoteInHertz`; bit-identical at default 12-TET A=440)
- Aftertouch → vibrato_depth modulation (deferred to v1.1 per Stage 0 D4)
- Two-register-table fallback (ARCHITECTURE Risk #2 Fallback 1) — Phase 2.2 verify cleared with rev-3; not invoked
- pluginval `--strictness 10` + Windows VST3 build (Stage 4)
- Dorico parity (Stage 4)

---

### Requirements Confirmed (Phase 2.3-relevant subsets of locked contracts)

- **DSP-02** (Vibrato — rate 0-10 Hz, depth 0-100 cents, onset 0-2000 ms): Phase 2.3 primary deliverable. Acceptance: 5 Hz / 50 cents at vibrato_onset=0 produces audibly-clean sine pitch modulation; `vibrato_onset = 1000ms` measurably fades vibrato in over ~1 second; tuner confirms depth at 50/100 cents within ±5 cents.
- **DSP-04** (Breath / dynamics — CC2 + velocity, scales loudness 0-1): Phase 2.3 primary deliverable. Acceptance: UI `breath` slider audibly modulates sustained-tone level; CC2 input from controller provides real-time loudness; CC2 = 0 mutes voice; CC2-takeover semantics (UI value yields to CC2 on first CC2 event); velocity sets startNote initial UI breath.
- **FUNC-04** (Long-tone amplitude envelope — attack 0-2000 ms, release 0-3000 ms): Phase 2.3 closes this requirement. Acceptance: full APVTS-driven attack/release ranges produce audibly-different onset/release slopes; no clicks at any setting across the full sweep.
- **QUAL-02** (Stable long-tone — no drift over 60 s): Phase 2.3 closes this requirement (Phase 2.1 verified > 10 s subset; Phase 2.3's continuous-noise excitation makes 60 s testable as true sustain). Acceptance: hold a single note for 60 s with vibrato + breath active — no amplitude drift, no NaN/inf, no denormal slowdown, CPU steady-state.
- **QUAL-01** (No clicks during parameter sweeps): Phase 2.3 closes the parameter-sweep half (Phase 2.2 cleared the `tone`-sweep half). Acceptance: independent sweeps of `attack_time`, `release_time`, `breath`, `output_gain`, `vibrato_rate`, `vibrato_depth`, `vibrato_onset` are all click-free.
- **PERF-01** (No allocations in `processBlock`): regression check carries forward. New code (Vibrato, NoiseExciter, ADSR APVTS reads, output_gain dispatch) must remain allocation-free. `juce::Random::nextFloat()` is allocation-free per JUCE 8 source.
- **PERF-02** (8-voice CPU < 25 %): Phase 2.3 early signal at the ROADMAP-tighter 20 % bar. Hold 8 keys in Logic-AU with vibrato + breath active, capture Logic CPU-meter reading. Voice cap is Phase 2.4; this measurement uses 8 simultaneously-held notes as a proxy. If exceeded, surface for Phase 2.4 budget.
- **FUNC-01** (sustained bassoon-like tones): regression check — Phase 2.2 verified bassoon-like timbre + 400-600 Hz spectrum peak at C3. Phase 2.3's noise-excitation must not regress timbre. Acceptance: held C3 with breath = 0.7, vibrato = 0, ADSR = default still recognisably bassoon-like (ear A/B against the Phase 2.2 reference recording is sufficient — no full-listening loop required since Phase 2.3 doesn't change the partial table).

**Deferred to Phase 2.4 / Stage 4:**
- DSP-05 (attack-character morph) — Phase 2.4
- DSP-06 (NE + MPE pitch consumption) — Phase 2.4
- FUNC-02 (1-16 polyphony with cap) / FUNC-05 (voice stealing) — Phase 2.4
- COMPAT-01 strict-10 + Windows / COMPAT-02 Dorico parity — Stage 4

---

### Constraints Identified (rev-3)

**Locked contracts (do NOT modify in this cycle):**

- All 10 APVTS parameter IDs, ranges, defaults — `parameter-spec-draft.md` and `PluginProcessor.cpp:25-99`. Phase 2.3 reads SIX from APVTS at processor level (`attack_time`, `release_time`, `vibrato_rate`, `vibrato_depth`, `vibrato_onset`, `breath`) + ONE post-summation (`output_gain`). Adds CC2 routing via `controllerMoved` per voice. `voice_count` and `output_gain`'s post-summation placement are the only structural additions; param IDs / ranges / defaults are NOT modified. Phase 2.2's `tone` smoother + dispatch is preserved verbatim.
- DSP architecture (`research/ARCHITECTURE.md`) — Phase 2.3 implements the architecture's Phase-2.3 subset (ADSR wiring, breath/dynamics, vibrato system, output_gain) plus a documented architectural pivot (continuous-noise excitation replacing impulse). ARCHITECTURE.md is appended to (rev-3 note + as-shipped noise spec + breath state machine + vibrato compose order), not rewritten.
- ROADMAP Phase 2.3 spec (lines 155-191) — components, test criteria, requirements verified.
- Phase 2.1 + Phase 2.2 wiring contracts that carry forward unchanged:
  - `PluginProcessor::processBlock` ordering: tone-dispatch → expression-dispatch (NEW Phase 2.3) → NE drain → renderNextBlock → output_gain applyGain (NEW Phase 2.3). Tone-dispatch + NE-drain ordering + the `vst3Extensions.drainAndUpdate()` BEFORE `synthesiser.renderNextBlock` invariant is preserved.
  - `BassoonVoice` member layout: `parameters / tuningEngine / pendingTuningSource` raw pointers + `modeBank / exciter / adsr / pitchWheelValue / pitchBendSemitones / currentFrequencyBase` from Phase 2.1 + `currentTone` from Phase 2.2. Phase 2.3 ADDS `vibrato / noiseExciter / breathSmoother / lastDispatchedFrequency / lastDispatchedAttackMs / lastDispatchedReleaseMs / lastDispatchedVibRate / lastDispatchedVibDepth / lastDispatchedVibOnsetMs / cc2Active / lastCC2SampleCount / [other throttle-state]` — all additive.
  - Per-sample render-loop ordering: `excitation → modeBank → adsr → addSample(L) + addSample(R)`. Phase 2.3 changes the **excitation source** (Phase 2.1/2.2: impulse exciter or strike()-injected free-decay; Phase 2.3: continuous noise excitation + retained strike() at startNote). The structural pipeline is preserved.
  - Mode-bank coefficient-update cadence: on note-on + on pitch-bend (Phase 2.1) + on tone-change > epsilon (Phase 2.2). Phase 2.3 ADDS a fourth trigger: on `f_final` change > 0.1 Hz (block-rate, driven by vibrato + pitch-bend composition). No per-sample mode-bank recompute.
  - Reference WAV archive (`research/reference-recordings/`) — read-only at Phase 2.3 (no listening loop; FUNC-01 regression check is ear-A/B only, no archive update).
  - Stage 1 build flags + CMakeLists structure — Phase 2.3 only adds `Source/Vibrato.{h,cpp}` + `Source/NoiseExciter.{h,cpp}` to `target_sources`. CMake flags unchanged.
  - DSP-07 (no O-Reed dependency) — verified at Stage 1, re-grepped at Phase 2.1 + Phase 2.2 verify, carries forward.
  - rev-3 `strike()` retained at `BassoonVoice::startNote` for the attack transient (Phase 2.2 invariant). Phase 2.3 fades continuous noise in alongside; no removal.

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**

- `juce::ScopedNoDenormals` at `processBlock` entry — already in place. Continuous-noise excitation at low breath levels produces small-amplitude resonator state — FTZ critical to prevent denormal CPU spikes during fade-out tails.
- `juce::SmoothedValue<float, ValueSmoothingTypes::Linear>` for `breath` (20 ms), `output_gain` (30 ms), `vibrato_onset` (linear 0→1 over `vibrato_onset` ms — variable ramp duration). Block-rate advance idiom locked at Phase 2.2 research (RESEARCH §2). Phase 2.3 reuses verbatim. Note: `breath_voice` is `breathSmoother.getNextValue()` per sample (not per-block — sample-rate smoothing necessary for click-free tuner-controlled CC2 sweeps).
- `juce::ADSR` API (JUCE 8.0.4): `setSampleRate(sampleRate)` BEFORE `setParameters(...)` (locked at Stage 1). `setParameters` is allocation-free (stack-only state). `noteOn()`, `noteOff()`, `getNextSample()`, `isActive()` — all RT-safe. ADSR sustain semantics: at `sustain = 1.0f`, `getNextSample()` returns 1.0 indefinitely while ADSR is in sustain phase (after attack completes, before noteOff). This is why the breath multiplier is decoupled from ADSR — ADSR shapes onset/release silhouette, breath shapes mid-sustain dynamics.
- `juce::Random` — JUCE 8.0.4 documents per-instance state; `nextFloat()` is allocation-free deterministic LCG. Per-voice instance avoids shared mutable state. Seed per-voice from `Time::currentTimeMillis() ^ voiceIndex` to avoid identical noise across voices on simultaneous note-ons.
- `juce::AudioBuffer::applyGainRamp(channel, startSample, numSamples, startGain, endGain)` — JUCE 8.0.4 RT-safe linear ramp from startGain to endGain across the buffer range. Use for `output_gain` declick when smoother is advancing. Alternative: `applyGain(buffer, smoother.getNextValue())` per block — coarser but simpler; research-phase locks.
- `juce::SynthesiserVoice::controllerMoved(int controllerNumber, int newControllerValue)` — JUCE 8 callback when MIDI CC events route to voices via Synthesiser. Voice receives all CCs; filter on `controllerNumber == 2` for breath. Allocation-free.
- Allocation-free `processBlock`: SmoothedValues are stack-only state. `juce::Random` per-voice has no allocation in `nextFloat()`. `juce::ADSR::setParameters` is allocation-free. Vibrato phase accumulator + onset SmoothedValue are stack-only. Phase 2.3 mandatory grep stays the same: `new`, `make_unique`, `make_shared`, `push_back`, `resize`, `malloc` zero hits in all touched files.

**Phase 2.3-specific constraints:**

- **Strict ROADMAP — only the four DSP systems wired this cycle** (locked Q2-rev-3): ADSR + breath + vibrato + output_gain. `attack_character` (DSP-05) deferred to Phase 2.4. The `1/8` headroom scaler (Phase 2.2 relaxation) STAYS — `output_gain` is post-summation; the per-voice scaler remains the in-resonator headroom guard.
- **Continuous-noise excitation as primary sustain source** (locked Q3-rev-3): 1-pole LP white noise, 2 kHz cutoff, `BASE_NOISE_GAIN = 0.05f`, scaled by `breath_voice`. Replaces impulse exciter in `renderNextBlock`. `strike()` retained at `startNote` for transient. Drops Phase 2.1 impulse from voice render path.
- **CC2-takeover breath semantics** (locked Q1-rev-3): `breath_voice = ui_breath × cc2_normalised` is the steady-state form. At startNote, `ui_breath = velocity / 127.0f` (initial seed). On first CC2 event, `cc2Active = true` and CC2 takes over; UI slider input is held in shadow but not applied until CC2 idle for >500 ms (research-phase locks the takeover-window value). Single source of truth at any moment. CC2 = 0 mutes the voice (audible-silence; ADSR may still be active so voice is not free).
- **Per-voice sine LFO vibrato, multiplicative compose, block-rate recompute** (locked Q4-rev-2 batch 1 = Q4-rev-3 batch 1): one sine LFO per voice with phase accumulator; sampled once at block start; `f_final = currentFrequencyBase × pow(2, vibratoCents/1200) × pow(2, pitchBendSemitones/12)`; setFundamental fires when `|f_final - lastDispatchedFrequency| > 0.1 Hz`. Per-block coefficient update is sufficient for ≤10 Hz vibrato (audible smearing only above ~500 Hz LFO rates per psychoacoustic precedent). Future NE/MPE multiplier (Phase 2.4) composes as additional factor: `f_final = ... × pow(2, NE_cents/1200)`.
- **Throttled-epsilon dispatch for all six expression APVTS reads** (Phase 2.2 precedent): processor's `processBlock` reads each of `attack_time`, `release_time`, `vibrato_rate`, `vibrato_depth`, `vibrato_onset`, `breath` (UI), tracks `lastDispatched*` per param at processor scope. If ALL six are within epsilon (`0.001f`), skip the per-voice `setExpression` call this block. If ANY changes, dispatch all six in one aggregated call. Saves ~6 × 16 = 96 virtual-dispatch hops/block when expression is static (the 99 % case during sustained playback).
- **Smoothing budgets** (locked from ROADMAP defaults + Phase 2.2 precedent):
  - `breath` per-voice: 20 ms `Linear` (voice-level smoother, sample-rate `getNextValue()` for per-sample multiply)
  - `output_gain` processor-level: 30 ms `Linear` (block-level applyGainRamp OR sample-rate getNextValue — research-phase locks)
  - `vibrato_onset`: variable ramp duration (0-2000 ms, per-voice `Linear` SmoothedValue, reset on startNote)
  - `tone` (Phase 2.2 carry-forward): 50 ms `Linear` processor-level
  - ADSR `setParameters`: block-rate (epsilon-throttled). No internal smoothing — JUCE ADSR re-shapes envelope on next `setParameters` call. Acceptable because ADSR change mid-note is a rare automation case.
  - Vibrato rate / depth: per-voice instant-update on epsilon-throttled dispatch (no smoothing — slight rate/depth zipper is psychoacoustically masked by the LFO modulation itself)
- **Gate 3 PASS bar — 10-item** (locked Q4-rev-3 batch 2): (1) ADSR attack 0→2000 ms sweep audibly different onset slopes, (2) ADSR release 0→3000 ms sweep no-click at any setting, (3) breath UI sweep 0→1 audibly modulates sustained level, (4) CC2 from a hardware/software controller produces real-time loudness change, (5) vibrato 5 Hz / 50 cents at `vibrato_onset = 0` clearly audible sine pitch modulation, (6) `vibrato_onset = 1000 ms` measurably fades vibrato in over ~1 s, (7) `vibrato_onset = 0` instant-vibrato at note-on, (8) 60 s held single-note with vibrato + breath active — no drift / NaN / CPU drift (QUAL-02 final gate), (9) `output_gain` -24 dB → +6 dB sweep no-click, (10) 8-voice CPU < 20 % with vibrato + breath active in Logic-AU. Plus automated invariant battery (RT-safety grep, NE drain ordering, mode-index, tone scaler retention, applyGainRamp idiom, auval, pluginval-5).
- **Iteration ceiling at rev-3** (locked Q4-rev-3 batch 2 + Phase 2.2 precedent): inline iteration at verify-phase. After rev-3, ship and document any gap as v1.1 candidate. Phase 2.3 has 4 APVTS-driven systems vs Phase 2.2's 1 — rev-3 ceiling is judged adequate given the ROADMAP's per-system test criteria are well-bounded.
- **Atomic commit on Gate 3 PASS** (locked Q4-rev-3 batch 2): single commit. Subject pattern: `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`. Lands `Source/Vibrato.{h,cpp}` + `Source/NoiseExciter.{h,cpp}` (NEW) + `Source/BassoonVoice.{h,cpp}` (MOD) + `Source/PluginProcessor.{h,cpp}` (MOD) + `CMakeLists.txt` (MOD: target_sources) + `research/ARCHITECTURE.md` rev-3 note + 5 planning artefacts (CONTEXT-rev-3 / RESEARCH-rev-3 / PLAN-rev-3 / SUMMARY-rev-3 / VERIFICATION-rev-3) + STATUS update + REQUIREMENTS update on Gate 3 PASS only.
- **8-voice CPU early signal at Phase 2.3** — bar < 20 % (ROADMAP). If exceeded, surface for Phase 2.4 polyphony budget; do NOT trigger ARCHITECTURE Risk #1 Fallback 1 (drop to 8 modes) at Phase 2.3 — the partial table is now locked from Phase 2.2's listening loop, and dropping modes would invalidate the timbre. Phase 2.4 gets the optimisation budget instead.

**Working-tree starting state (locked from Phase 2.2 atomic commit `baac74f` on `main`):**

- `Source/ModeBank.{h,cpp}` — 16-mode pole-only resonator bank, bassoon-tuned partial ratios, formant-Gaussian × 1/k roll-off amplitudes, `setTone` + `applyToneChange` + `strike()` (rev-3 patch), `1/8` headroom scaler, isfinite NaN guard, Nyquist muting at `0.45 × fs`. Phase 2.3 leaves untouched.
- `Source/Exciter.{h,cpp}` — 5 ms half-sine × exp impulse, peak-normalised, class-level `std::array<float, 1024>` storage, no allocation at runtime. Phase 2.3 RETAINS the file but stops calling `exciter.getNextSample()` from `renderNextBlock` (Phase 2.4 attack-character morph re-introduces the impulse path with `effective_attack_char` blend).
- `Source/BassoonVoice.{h,cpp}` — per-sample render loop (`exciter → modeBank → adsr → addSample(L,R)`), pitch-bend ±2 semis (raw 14-bit), full state-reset on voice exit, NO continuous noise yet, NO breath APVTS read, hardcoded ADSR, NO vibrato, NO output_gain. `setTone(float)` public method + currentTone member from Phase 2.2.
- `Source/PluginProcessor.{h,cpp}` — APVTS (10 params), 16-voice `juce::Synthesiser`, headless TuningEngine, NE drain BEFORE renderNextBlock, `tone` smoother + per-voice dispatch (Phase 2.2). No expression dispatch yet, no output_gain post-summation.
- `CMakeLists.txt` — sources include ModeBank + Exciter from Phase 2.1; no Vibrato or NoiseExciter yet.
- Build state: `O-Bassoon_VST3` + `O-Bassoon_AU` + `O-Bassoon_Standalone` install fresh post-Phase-2.2 commit; `auval -v aumu OBsn OuDv` SUCCESS; `pluginval --strictness 5` SUCCESS.

---

### Approach Decisions (rev-3)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q1-rev-3 (batch 1) — Phase 2.2 atomic commit landing** | **Land Phase 2.2 commit BEFORE writing CONTEXT-rev-3 addendum.** Single commit `baac74f` lands rev-2 sources + rev-3 strike() + ARCHITECTURE rev-note + 7 planning artefacts on `main`. | Cleanest working tree. Matches Phase 2.1 / O-Contrabass / O-Lyrica precedent (atomic commit lands gate-first). User-confirmed. |
| **Q2-rev-3 (batch 1) — Phase 2.3 cycle scope** | **Strict ROADMAP — vibrato + breath/dynamics + ADSR APVTS wiring + output_gain.** Defers `attack_character` morph (DSP-05) to Phase 2.4 per ROADMAP lines 192-251. 4 APVTS-driven systems in one cycle. | Matches ROADMAP boundary verbatim. Keeps Phase 2.3 narrowly-scoped to per-note expression mechanics (envelope + dynamics + modulation + master) without entangling attack-character morph (which has its own A/B-listening feel and benefits from a dedicated phase with the polyphony cap in place). User-confirmed (recommended). |
| **Q3-rev-3 (batch 1) — Breath-into-modeBank architecture** | **Continuous filtered-noise excitation.** Per-voice `juce::Random` generates white noise, 1-pole LP at 2 kHz, scaled by `breath_voice = ui_breath × cc2_normalised`, fed into `modeBank.processSample(excitation)`. Replaces struck-modal sustain mechanism. Removes Phase 2.1 impulse exciter from `renderNextBlock`. Retains rev-3 `strike()` at `startNote` for transient. | True breath-driven sustain matches real-bassoon air-column physics — voice maintains amplitude indefinitely while held + breath > 0. Removes dependency on `strike()` IR-only path being the sole sustain mechanism (it's now just the attack transient). QUAL-02 60s gate becomes pass-able as continuous-sustain rather than slow-decay. User-confirmed (recommended). |
| **Q4-rev-3 (batch 1) — Vibrato architecture** | **Per-voice sine LFO, multiplicative compose, block-rate recompute.** Each voice has its own phase accumulator + onset SmoothedValue. Per-block sample of LFO. Compose: `f_final = currentFreq × pow(2, vibratoCents/1200) × pow(2, pitchBendSemitones/12)`. setFundamental fires when `|Δf_final| > 0.1 Hz`. Future NE/MPE multiplier (Phase 2.4) composes as additional factor. | Matches ROADMAP Phase 2.3 spec lines 170-178 verbatim. Per-voice phase staggering prevents artificially-synchronised chord vibrato. Block-rate recompute is psychoacoustically sufficient for ≤10 Hz vibrato (per-sample is overkill at 16 cos/exp per sample × 16 voices = 4096 transcendentals/sample — 25× CPU regression). User-confirmed (recommended). |
| **Q1-rev-3 (batch 2) — Breath value composition** | **Multiplicative scaler with CC2-takeover.** `breath_voice = ui_breath × cc2_normalised`. Velocity sets initial `ui_breath = velocity / 127.0f` at `startNote`; first CC2 event sets `cc2Active = true` and CC2 takes over. UI breath ignored until CC2 idle for >500 ms. `breath_voice = 0` mutes the voice (audible silence; ADSR may still be active). | Single source of truth at any moment, no double-counting. Matches BRIEF.md `0.7 default` semantics. CC2 = 0 mutes the voice (real-bassoon "no air = no sound" physics). Velocity-as-initial-breath gives keyboard-only players a meaningful per-note dynamic without requiring a CC2 controller. User-confirmed (recommended). |
| **Q2-rev-3 (batch 2) — Continuous filtered-noise excitation spec** | **1-pole LP white noise, cutoff 2 kHz, scaled by breath.** Per-voice `juce::Random` (seeded `Time::currentTimeMillis() ^ voiceIndex`), `lpState += lpCoeff × (rng - lpState); return lpState × BASE_NOISE_GAIN × breath_voice`. `BASE_NOISE_GAIN = 0.05f` starting point. | Cheap (1 mul + 1 add per sample for filter, plus 1 nextFloat). 2 kHz cutoff is "broadband-but-warm" air-column proxy — wide enough to excite all 16 modes' formant-band partials, narrow enough to avoid near-Nyquist excitation that gets muted at high f0. Modal bank itself does the spectral shaping. `BASE_NOISE_GAIN` to be empirically tuned at verify; 0.05 is a 16-mode-summed RMS-balanced starting point. User-confirmed (recommended). |
| **Q3-rev-3 (batch 2) — Note-on transient with continuous noise** | **Keep strike() + drop impulse exciter, fade noise in.** `startNote`: call `modeBank.strike()` (instant modal-state injection — attack transient), simultaneously start continuous noise at full `breath_voice`. ADSR.attack ramps overall amplitude 0→1 over `attack_time` ms. Phase 2.1 impulse exciter file retained (Phase 2.4 attack-character may re-introduce); NOT called from `renderNextBlock` at Phase 2.3. | Cleanest architecture — strike() is transient, noise is sustain, ADSR is silhouette. Preserves rev-3 patch's bassoon-attack character. Avoids three-tier excitation stacking (which would entangle Phase 2.4 attack-character morph). Soft `attack_time = 0` becomes "instant strike + instant noise + ADSR cliff" — natural behaviour. User-confirmed (recommended). |
| **Q4-rev-3 (batch 2) — Gate 3 PASS bar + iteration budget + atomic commit** | **10-item Gate 3 + rev-3 ceiling + atomic commit on PASS.** Items: (1) attack 0-2000 ms audibly different, (2) release 0-3000 ms no-click, (3) breath UI sweep audible, (4) CC2 real-time loudness, (5) vibrato 5 Hz/50 c audible, (6) `vibrato_onset = 1000 ms` fade-in measurable, (7) `vibrato_onset = 0` instant-vibrato, (8) 60 s held note no drift/NaN/CPU drift (QUAL-02 final), (9) output_gain -24..+6 dB no-click, (10) 8-voice CPU < 20 % with vibrato + breath active. Inline iteration at verify; ceiling rev-3. Atomic commit `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS` on Gate 3 PASS only. | Mirrors Phase 2.2 procedural pattern. 10 items match the four-system surface (3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone + 1 polyphony-CPU = 10). 60 s sustain closes QUAL-02; 8-voice CPU is early-signal for PERF-02 (final at Phase 2.4). User-confirmed (recommended). |
| Throttled-epsilon dispatch for 6 APVTS reads | Per-param epsilon `0.001f` at processor scope. If ALL six within epsilon, skip `setExpression` call this block. Aggregate setter (single virtual call per voice per block when any change). | Phase 2.2 throttled-epsilon precedent (tone) extended to all expression params. Saves ~6 × N_voice virtual-dispatch hops/block when expression is static. |
| `breath` smoothing layer | Per-voice 20 ms `Linear` SmoothedValue; sample-rate `getNextValue()` for per-sample multiply. UI value pushed via `setExpression`; CC2 value pushed via `controllerMoved`. Both target the same smoother (CC2-takeover state machine arbitrates which is the active target). | Per-sample smoothing required for click-free CC2 sweeps — block-rate would zip on rapid controller motion. Per-voice (not processor-level) because each voice's CC2 stream may differ in MPE mode (Phase 2.4). |
| `output_gain` smoothing layer | Processor-level 30 ms `Linear` SmoothedValue. Block-rate `applyGainRamp(...)` from current → next-block target. | Post-summation single multiply across the final stereo bus. ROADMAP-spec'd 30 ms ramp. `applyGainRamp` is JUCE 8's declick-safe primitive for buffer-level gain transitions; research-phase locks the canonical idiom. |
| `vibrato_onset` smoother | Per-voice variable-duration `Linear` SmoothedValue. `reset(0.0f)` + `setTargetValue(1.0f)` at `startNote` with `reset(sampleRate, vibrato_onset_ms / 1000.0)`. Output multiplied with `depthCents × sin(phase)` to produce final cents offset. | Ramp duration is itself a parameter (0-2000 ms). Linear ramp per ROADMAP; `vibrato_onset = 0` collapses to instant target = full vibrato. |
| ADSR `setParameters` cadence | Block-rate, epsilon-throttled. Read APVTS `attack_time` + `release_time` once per block; if either changed > 0.001, call `adsr.setParameters({attack/1000, 0, 1.0, release/1000})`. JUCE ADSR re-shapes envelope on call — no internal smoothing required. | ADSR mid-note parameter change is rare in practice (DAW automation lanes are the primary source). Block-rate dispatch + JUCE's internal envelope re-shaping is sufficient. No internal SmoothedValue — adds complexity without audible benefit. |
| `juce::Random` per-voice seeding | `juce::Time::currentTimeMillis() ^ voiceIndex` at voice construction. Each voice has independent stream. | Avoids identical noise across voices on simultaneous note-ons (chord rendering). Per-voice instance is the JUCE-recommended pattern for per-voice stochastic excitation (precedent: granular/sampler voices). |
| CC2-takeover window | `500 ms` of CC2-idle samples before UI breath value resumes. State: `bool cc2Active; juce::int64 lastCC2SampleCount;` per voice. | Long enough to filter out gaps between CC2 events from a slow controller; short enough that releasing CC2 controller and dragging UI feels responsive. Research-phase locks (may revise to 250-1000 ms after experimentation). |
| Drop Phase 2.1 impulse exciter call | `renderNextBlock` no longer calls `exciter.getNextSample()`. `Source/Exciter.{h,cpp}` file retained verbatim for Phase 2.4 attack-character morph re-introduction. | Architectural cleanup. Avoids dead-code call path during Phase 2.3 verify. Keeps the file for trivial Phase 2.4 re-wire (no re-implement). |
| Voice cap simulation at Phase 2.3 verify | Hold 8 keys in Logic-AU; `juce::Synthesiser` default 16-voice allocation makes 8 simultaneous voices the steady state (matches Phase 2.2 8-voice CPU protocol). | Voice cap is Phase 2.4. At Phase 2.3 the CPU measurement is "8 simultaneous voices" not "voice_count = 8 enforced." Distinction matters only if voice-stealing would alter the load profile — at Phase 2.3 stealing is the JUCE default (oldest), same as 16 voices. |
| `f_final` change throttle | `>0.1 Hz` for setFundamental dispatch. Below threshold → no recompute (matches Phase 2.1 pitch-bend cadence). | 0.1 Hz at C3 (130 Hz) ≈ 1.3 cents — sub-perceptual. Vibrato at 5 Hz × 50 cents → cents range 50, frequency range ±3.7 Hz at C3 — well above threshold every block. |
| Aggregate `setExpression` setter | Single per-voice method receiving 6 floats (attack, release, vibrato_rate, vibrato_depth, vibrato_onset, ui_breath) per block. Internal per-sub-param epsilon throttling. | Reduces virtual-dispatch overhead. Single call per voice per block (when expression is dynamic) vs. 6 calls. |

---

### Open Questions (handed to research-phase) — rev-3

1. **`juce::AudioBuffer::applyGainRamp` vs per-block `applyGain` for output_gain declick.** Phase 2.3 uses `output_gain` post-summation with 30 ms `SmoothedValue`. Two declick-safe idioms: (a) `buffer.applyGainRamp(channel, 0, numSamples, smoother.getCurrentValue(), smoother.skip(numSamples))` — linear ramp from block-start to block-end values; (b) `buffer.applyGain(smoother.getNextValue())` per block — coarser steps. Confirm the JUCE 8.0.4 canonical idiom for declick-safe `SmoothedValue`-driven applyGain. Lookup: `/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_AudioBuffer.h` (applyGainRamp implementation) + O-Wind / O-Lyrica `Source/PluginProcessor.cpp` (output_gain handling precedent). Output: cite line numbers; lock the form.

2. **`juce::ADSR` block-rate `setParameters` semantics in JUCE 8.0.4.** When `setParameters` is called mid-envelope (e.g., during sustain phase), does JUCE re-shape the envelope smoothly or jump? Verify the implementation in `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_ADSR.h` — confirm whether mid-sustain `setParameters` causes any audible click. If yes, document as a known limitation OR add a tiny smoother around `attack_time` / `release_time` mid-note. Output: research-phase locks; default is "JUCE re-shapes smoothly, no smoother needed" pending verification.

3. **`juce::Random` thread-safety semantics on the audio thread.** Phase 2.3 instantiates per-voice `juce::Random`. Confirm `nextFloat()` is allocation-free and lock-free per-instance. Confirm seeding with `Time::currentTimeMillis() ^ voiceIndex` is allowed off the audio thread (voice constructor runs at `prepareToPlay` time). Output: cite source line; document. Cross-check `juce::Random::getSystemRandom()` (shared global random — NOT used here; per-voice instance is the pattern).

4. **Continuous-noise `BASE_NOISE_GAIN` empirical tuning.** Phase 2.3 starts at `BASE_NOISE_GAIN = 0.05f`. The 16-mode-summed RMS-balanced target is "voice peak ≈ Phase 2.2 strike()-injected peak (~0.15-0.25)" so Phase 2.3 voice loudness matches Phase 2.2 baseline at `breath = 0.7` (default). Empirical tuning happens at verify-phase rev-1 — likely revise to 0.03-0.10 range based on ear A/B vs Phase 2.2 reference. Document the as-shipped value in ARCHITECTURE.md rev-3 backfill. Output: research-phase predicts the target gain via simple modal-bank energy calculation (mode amplitude × biquad peak gain × mode count × duty-cycle assumption); plan-phase locks the starting value.

5. **CC2-takeover window value (250 / 500 / 1000 ms).** Default is 500 ms. Research-phase: simulate (or recall O-Wind/O-Lyrica precedent if any) the user-perceptible threshold for "CC2 controller released → UI breath resumes." Too short = CC2 stream gaps cause flicker; too long = sluggish UI takeover. Output: lock the value with reasoning; default 500 ms is acceptable barring evidence otherwise.

6. **Pre-NE-drain dispatch ordering vs current Phase 2.2 pattern.** Phase 2.2 placed `tone` smoother dispatch BEFORE NE drain (PluginProcessor.cpp:178 → :182). Phase 2.3 adds expression dispatch BEFORE NE drain too (same prologue block). Verify ordering: `tone-dispatch → expression-dispatch → NE-drain → renderNextBlock → output_gain-applyGain`. Confirm no implicit dependency between expression-dispatch and NE-drain (NE drain provides per-noteId pitch; expression-dispatch is pitch-orthogonal — vibrato + ADSR + breath + output_gain). Output: lock the order; cross-check against O-Wind / O-Lyrica processor prologue.

7. **Vibrato compose-order with future NE/MPE multiplier.** Phase 2.3 locks `f_final = currentFreq × vibratoMult × pitchBendMult`. Phase 2.4 will add NE per-noteId tuning multiplier. Confirm the multiplicative compose form is correct: `f_final = (NE-tuned-freq) × vibratoMult × pitchBendMult` — i.e., vibrato + pitch-bend operate on the NE-tuned base, not the 12-TET base. This matches O-Lyrica precedent. Document the order in ARCHITECTURE.md rev-3 backfill so Phase 2.4 has zero ambiguity. Output: lock the compose chain; cite O-Lyrica reference.

8. **CC2 normalisation — 0-127 raw vs 0-1 normalised.** JUCE `controllerMoved` delivers `int newControllerValue` in raw 7-bit (0-127). Confirm: divide by 127.0f at the controllerMoved callback or at the smoother target site? Default: divide at controllerMoved (inputs to `breathSmoother.setTargetValue` are normalised 0-1 floats). Document. Output: lock; trivial but worth pinning.

9. **Vibrato phase reset on startNote — instant-zero vs continuous.** At `startNote`, reset `vibrato.phase = 0.0f` (default) — vibrato starts at sin(0) = 0 cents offset. Alternative: continuous phase across notes (don't reset). For polyphonic chord-vibrato with per-voice phase staggering, instant-zero on each note may sound more "synchronised" than expected; continuous-phase preserves natural staggering as voices rotate. Default: instant-zero (simpler; per-voice randomisation can be added later if needed). Research-phase: confirm instant-zero is the right default per O-Wind/O-Bowed precedent. Output: lock; possibly add a small phase randomisation (e.g., `phase = rng.nextFloat() × 2π`) at startNote for natural staggering — research-phase decides.

10. **60 s long-tone QUAL-02 protocol.** Phase 2.3 closes QUAL-02. Protocol: hold a single note in Logic-AU for 60 s with vibrato 5 Hz / 50 cents + breath = 0.7 + ADSR default. Capture: (a) audio recording → numerical scan for NaN/inf/denormal-CPU-spike via `awk` or Python; (b) Logic CPU meter steady-state reading at t=60s vs t=10s (drift detection); (c) ear-listen for amplitude drift. Verify the 60 s test is achievable in single-take Logic-AU (no buffer roll-over issues). Output: research-phase confirms the protocol + locks the numerical-scan tool (Python `numpy.isfinite` over WAV samples, or `awk` over `auvaltool` output). Cross-check Phase 2.1 / Phase 2.2 long-tone protocol.

---

### Risks (Phase 2.3-specific)

1. **Continuous noise excitation regresses Phase 2.2 timbre.** Phase 2.2's listening loop tuned the partial table assuming impulse + strike() injection — that excitation is impulse-like with a flat broadband spectrum. Phase 2.3 replaces with continuous LP-filtered noise — also broadband but persistent. The modal bank spectral shaping should dominate (modes are high-Q resonators), but if the noise's residual broadband floor leaks through audibly, the voice could sound "noisy" in addition to "bassoon-like." Mitigation: locked Q2-rev-3 (b2) `BASE_NOISE_GAIN = 0.05f` starting point + research-phase Open Question #4 (empirical tuning); ear A/B vs Phase 2.2 reference at verify-phase rev-1; if floor is audible, lower `BASE_NOISE_GAIN` (e.g., 0.03) OR raise modal Q (T60 increase) — both cleanup the spectral signature.

2. **CC2-takeover state machine flicker.** If CC2 controller emits sparse events (e.g., 50 Hz update rate) and the takeover window is too short, UI breath could flicker in/out as the state machine flips between active/idle. Mitigation: locked Q5-rev-3 (research-phase) — default 500 ms window; revisable to 1000 ms if flicker is observed. Sample-rate counter `lastCC2SampleCount` tracks elapsed samples since last CC2 event (allocation-free).

3. **Vibrato rate / depth zipper without smoothing.** Locked decision: no smoothing on vibrato_rate or vibrato_depth — slight zipper is masked by LFO modulation itself. Risk: if a user automates vibrato_depth from 0 → 100 cents over 100 ms (DAW automation), the unsmoothed param change could produce an audible step at the dispatch boundary. Mitigation: epsilon-throttled dispatch + LFO continuous-phase (no zeroing on rate change) reduces step amplitude. If audible at verify, add 50 ms `Linear` SmoothedValue per parameter (per-voice) — small ~24 bytes/voice cost, fully RT-safe. Document deviation from CONTEXT-rev-3 if applied.

4. **60 s sustain CPU drift / denormal accumulation.** QUAL-02 is the 60 s gate. Continuous-noise excitation introduces per-sample stochastic variation; modal bank state evolves continuously. Risk: denormal accumulation in mode-bank state if `lpState` or `y1`/`y2` decay below FTZ threshold over 60 s. Mitigation: `juce::ScopedNoDenormals` already in place at processBlock entry — FTZ enforced. `BASE_NOISE_GAIN × breath` ensures mode-bank state stays well above denormal threshold during normal play (breath = 0 → noise = 0 → state decays at T60 to denormals → ScopedNoDenormals catches). Verify at 60 s gate.

5. **`output_gain` post-summation declick on rapid sweep.** ROADMAP spec: 30 ms ramp. Risk: if a user automates `output_gain` from -24 → +6 dB over 50 ms (DAW automation step), the 30 ms ramp produces a fast transition; if `applyGainRamp` is per-block (256 samples = ~5.3 ms at 48 k), the 30 ms ramp spans ~5.6 blocks with linear interpolation between block boundaries — should be declick-safe. Verify at Gate 3 item 9.

6. **Breath multiplicative composition with `breath_voice = 0` mute.** When `breath_voice = 0`, voice produces no audible output but ADSR is still active and `clearCurrentNote()` is NOT called (ADSR exit is the trigger). Risk: stuck-voice perception — user releases key, ADSR enters release phase, but voice is in "silent CC2 hold" state with breath = 0 → user hears nothing → might trigger again → polyphony bookkeeping confused. Mitigation: breath = 0 is "no air" but ADSR release proceeds normally (timer-based). Voice-state transitions are unaffected. Verify: hold note with breath = 0, release key, confirm voice exits at ADSR-release-end.

7. **`juce::Random` per-voice seeding race.** Voice constructors run at plugin instantiation (not audio thread); `Time::currentTimeMillis()` is called ~16 times in rapid succession. If clock granularity is coarse (Windows: ~15 ms), multiple voices could get identical seeds. Mitigation: XOR with `voiceIndex` (0-15) in the seed: `seed = Time::currentTimeMillis() ^ voiceIndex` — guaranteed distinct even with identical millisecond reads. Alternative: use `juce::Random::getSystemRandom().nextInt()` for the seed (per-call distinct).

8. **Drop of Phase 2.1 impulse exciter — Phase 2.4 re-wire risk.** Phase 2.3 stops calling `exciter.getNextSample()` from `renderNextBlock` but retains the file. Phase 2.4 needs to re-wire it for `attack_character` morph. Risk: rot — if Phase 2.3 introduces a new exciter signature mismatch, Phase 2.4 has to refactor. Mitigation: Phase 2.3 leaves `Exciter.{h,cpp}` strictly unchanged. Phase 2.4's attack-character morph calls a new method (or augments `exciter.startOnset(attack_char, vel)`) without restructuring the existing API. Document at Phase 2.3 plan-phase: "Exciter retained verbatim for Phase 2.4."

9. **8-voice CPU regression with vibrato + breath active.** Phase 2.2 verified 8-voice CPU < 20 % with tone-only at minimum coefficient activity. Phase 2.3 adds: per-sample noise generation × 8 voices, per-sample LP filter × 8, per-sample breath multiply × 8, per-sample ADSR × 8 (carry-forward), per-block vibrato LFO × 8 + setFundamental dispatches when |Δf| > 0.1 Hz (frequent during vibrato — every block at 5 Hz × 50 cents), per-block expression dispatch × 8, per-block output_gain ramp × 1 stereo bus. Estimated per-voice overhead: ~3-5 % per voice (modal bank dominates at ~12-15 %). 8-voice total: ~20-25 %. Mitigation: locked Q4-rev-3 (b2) early-signal at Gate 3 item 10. If exceeded, surface for Phase 2.4 polyphony budget; do NOT trigger ARCHITECTURE Risk #1 Fallback 1 (drop modes) — partial table is locked from Phase 2.2.

---

### Next Phase

Ready for: **research** phase — `/plugin-research O-Bassoon 2-dsp`

Research focus (Phase 2.3):

1. **Resolve Open Questions #1–#10** — applyGainRamp vs applyGain idiom, ADSR mid-envelope setParameters semantics, juce::Random thread-safety, BASE_NOISE_GAIN empirical target, CC2-takeover window value, dispatch ordering, vibrato compose chain with future NE/MPE, CC2 normalisation site, vibrato phase reset on startNote, 60 s sustain protocol.
2. **Pattern-confirm against O-Wind + O-Lyrica + O-Bowed** — per-voice vibrato LFO + onset SmoothedValue precedent (cite `Source/*.cpp` exact lines); CC2 routing via `controllerMoved` (cite); `output_gain` post-summation `applyGainRamp` precedent (cite); ADSR APVTS wiring at startNote vs processBlock cadence (cite).
3. **Verify ARCHITECTURE.md continuous-noise excitation compatibility** — modal bank's per-mode `(1-R)·amp` G-normalisation expects bounded inputs; confirm continuous noise at `BASE_NOISE_GAIN × breath` stays in the bounded regime (no peak amplification > unity in any single mode); document.
4. **Pre-flight: run `O-Bassoon_VST3` from `baac74f` once** — confirm working tree starting state matches commit; Logic-AU plays Phase 2.2 baseline tone. Sanity-check before research finalises implementation skeletons.
5. **Append RESEARCH.md** at `plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md` (rev-3) with §1 (OQs resolved), §2 (Pattern Confirmations from O-Wind / O-Lyrica / O-Bowed), §3 (Implementation Skeletons — Vibrato.{h,cpp} + NoiseExciter.{h,cpp} + BassoonVoice rev-3 deltas + PluginProcessor rev-3 dispatch + applyGain), §4 (Discrepancies — anything that contradicts CONTEXT-rev-3 or ARCHITECTURE.md).

After research: plan-phase writes Phase 2.3 task breakdown verbatim against this CONTEXT-rev-3 + research findings; execute-phase performs the implementation; verify-phase confirms Gate 3 listening + 60 s sustain + 8-voice CPU + atomic commit.

---

### Audit Trail (rev-3 addendum)

**rev-3 (this addendum, 2026-04-28):** Phase 2.3 opening — Per-Note Expression: Envelope, Breath, Vibrato, Output Gain. 8 user-confirmed approach decisions across two AskUserQuestion batches:
- Batch 1: (Q1) land Phase 2.2 atomic commit before CONTEXT-rev-3 (committed at `baac74f`); (Q2) strict ROADMAP cycle scope (defer attack_character to Phase 2.4); (Q3) continuous filtered-noise excitation as primary sustain source; (Q4) per-voice sine LFO vibrato, multiplicative compose, block-rate recompute.
- Batch 2: (Q1) breath = ui_breath × cc2_normalised with CC2-takeover; (Q2) 1-pole LP white noise, 2 kHz cutoff, BASE_NOISE_GAIN = 0.05f; (Q3) keep strike() + drop impulse exciter from renderNextBlock; (Q4) 10-item Gate 3 + rev-3 ceiling + atomic commit on Gate 3 PASS.

Plus 12 derived decisions (throttled-epsilon dispatch for 6 expression APVTS, per-voice 20 ms breath smoother, processor-level 30 ms output_gain smoother, variable vibrato_onset SmoothedValue, ADSR setParameters block-rate epsilon-throttled, juce::Random per-voice seeding, CC2-takeover 500 ms window, drop impulse exciter call (file retained), 8-voice CPU simulation via 8 held keys, |Δf_final| > 0.1 Hz throttle, aggregate setExpression setter, vibrato compose with multiplicative pitch-bend).

10 open questions handed to research-phase: applyGainRamp idiom, ADSR mid-envelope semantics, juce::Random thread-safety, BASE_NOISE_GAIN empirical target, CC2-takeover window, dispatch ordering, vibrato compose with NE/MPE, CC2 normalisation site, vibrato phase reset, 60 s sustain protocol.

9 risks documented with mitigations: noise floor regresses timbre, CC2-takeover flicker, vibrato rate/depth zipper, 60 s denormal accumulation, output_gain rapid-sweep declick, breath = 0 stuck-voice, juce::Random seeding race, Phase 2.1 exciter rot, 8-voice CPU regression with vibrato + breath active.

**Inherited verbatim from Phase 2.1 + Phase 2.2 (not re-litigated):**
- Per-sample render loop ordering (`excitation → modeBank → adsr → addSample`) — Phase 2.3 changes excitation source, preserves pipeline.
- Centered equal L+R per-sample voice write
- Mode-bank coefficient cadence: note-on + pitch-bend (Phase 2.1) + tone > epsilon (Phase 2.2) + |Δf_final| > 0.1 Hz (Phase 2.3 ADDS — vibrato + pitch-bend block-rate compose)
- NE drain BEFORE renderNextBlock at PluginProcessor.cpp prologue (Phase 2.3 inserts expression dispatch BEFORE NE drain, AFTER tone dispatch)
- DSP-07 (no O-Reed dependency) verified at Stage 1
- Reference WAVs archived at `research/reference-recordings/` (Phase 2.1)
- Atomic-commit gate-first principle
- Primary listening DAW: Logic Pro (AU)
- DAW + tuner verification (no CLI render harness)
- Bassoon-tuned partial table + formant Gaussian × 1/k roll-off (Phase 2.2 — locked)
- Tone smoother (50 ms Linear) + throttled-epsilon (0.001) dispatch (Phase 2.2 — locked)
- 1/8 headroom scaler (Phase 2.2 — relaxed from 1/16)
- rev-3 strike() at startNote (Phase 2.2 patch — retained as attack transient)

**New in rev-3:**
- Cycle scope = Phase 2.3 only (4 APVTS-driven systems: ADSR + breath + vibrato + output_gain)
- Architectural pivot: continuous-noise excitation replaces struck-modal-only sustain (impulse exciter dropped from voice render path)
- Six APVTS reads at processor (attack_time, release_time, vibrato_rate, vibrato_depth, vibrato_onset, breath UI) + one post-summation (output_gain); CC2 routing per voice via controllerMoved
- Per-voice continuous LP-noise excitation source (`NoiseExciter`) — 1-pole LP @ 2 kHz, BASE_NOISE_GAIN 0.05f, scaled by breath_voice
- Per-voice sine LFO vibrato (`Vibrato`) with onset SmoothedValue — multiplicative compose with pitch-bend, block-rate recompute, |Δf_final| > 0.1 Hz throttle
- Breath state machine: `breath_voice = ui_breath × cc2_normalised`, velocity → initial UI breath, CC2-takeover with 500 ms idle window
- Aggregate `setExpression(...)` per-voice setter for 6 APVTS reads, throttled-epsilon all sub-params at processor scope
- Per-voice 20 ms breath smoother (sample-rate getNextValue)
- Processor-level 30 ms output_gain smoother (block-rate applyGainRamp)
- Variable-duration vibrato_onset smoother (Linear, reset on startNote with vibrato_onset_ms ramp)
- ADSR setParameters block-rate epsilon-throttled (no internal smoothing)
- Gate 3 PASS bar: 10-item (3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone + 1 polyphony-CPU)
- Inline iteration at verify-phase, ceiling at rev-3
- Atomic commit on Gate 3 PASS with subject `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`
- ARCHITECTURE.md backfill (continuous-noise spec + breath state machine + vibrato compose order) is a Phase 2.3 atomic-commit deliverable
- Phase 2.1 impulse `Exciter` file retained verbatim (Phase 2.4 re-wires for attack-character morph) but NOT called from `renderNextBlock`

---

## rev-4 — Phase 2.4 Opening (2026-04-29)

**Cycle Scope (rev-4):** **Phase 2.4 — Voice Manager + Attack Character + Note Expression Integration** (final Stage 2 phase). Closes the 4 remaining DSP requirements in a single coupled cycle: (1) **FUNC-02** polyphony with `voice_count` cap (1-16, default 8) via `BassoonSynthesiser` subclass + `findFreeVoice` override; (2) **FUNC-05** voice stealing (release-tail-first, then oldest-noteOn — JUCE default `findVoiceToSteal` semantics); (3) **DSP-05** attack-character morph — re-engages retained Phase 2.1 `Exciter` member with two pre-baked shape arrays (soft pad + tongued articulation) crossfaded via `attack_character` APVTS with velocity bias; (4) **DSP-06** microtonal pitch via `TuningEngine::getFrequency()` per-voice + `applyPendingTuning` per-noteId at `startNote` (snapshot semantics — NE locked for voice lifetime). MPE per-channel pitch-bend already wired at Phase 2.1 `pitchWheelMoved`; Phase 2.4 confirms per-channel routing in MPE-enabled DAW. Plus **QUAL-02** 60 s long-tone gate (skipped at Phase 2.3 verify per user authority — revisited as Gate 4 item).

Phase 2.3 atomic commit is PENDING explicit user trigger as of 2026-04-29 (working tree contains rev-3 sources + rev-4 inline fixes — `Source/Vibrato.{h,cpp}`, `Source/NoiseExciter.{h,cpp}`, `Source/BassoonVoice.{h,cpp}`, `Source/PluginProcessor.{h,cpp}`, `CMakeLists.txt`, `research/ARCHITECTURE.md` rev-3 note + 5 planning artefacts). Phase 2.4 discuss-phase can proceed against the in-tree state; Phase 2.4 execute-phase MUST NOT begin until Phase 2.3 atomic commit lands on `main` (separates the rev-3/rev-4 expression delivery from the Phase 2.4 voice-manager/NE/attack-character delivery for clean revert and `git log --oneline` legibility).

### Cycle Scope (rev-4)

**Goal:** Plugin reaches Stage 2 feature-complete state. Polyphonic 1-16 voices with cap and stealing; attack character morphs audibly between soft pad and tongued articulation across `attack_character` × velocity space; per-note pitch via VST3 Note Expression (Dorico) and MPE channel pitch-bend with TuningEngine wired headless (12-TET default). 60 s held single-note (revisit) maintains amplitude with no drift / NaN / denormal slowdown. 8-voice CPU at 48 k / 256 stays under 25 % (PERF-02 final). All Phase 2.1-2.3 invariants regression-clean.

**In scope:**

- `Source/BassoonSynthesiser.{h,cpp}` (NEW) — subclass `juce::Synthesiser`. Enables `setNoteStealingEnabled(true)` in constructor. Stores `int activeVoiceCap = 16` member (read from APVTS at processBlock prologue, snapshot for the block — applies at next note-on per ROADMAP locked behavior). Overrides `findFreeVoice(SynthesiserSound*, int channel, int noteNumber, bool stealIfNoneAvailable)`: walks active-voice list, counts voices in non-idle state; if active-count < `activeVoiceCap`, delegates to `juce::Synthesiser::findFreeVoice` (free-pool selection); if active-count >= cap and `stealIfNoneAvailable == true`, calls `findVoiceToSteal(sound, channel, noteNumber)` (JUCE default — release-tail-first, then oldest-noteOn); else returns `nullptr`. `setActiveVoiceCap(int cap)` setter clamped to [1, 16]. PluginProcessor swaps the existing `juce::Synthesiser synthesiser` member to `BassoonSynthesiser synthesiser` (single-line type change at `PluginProcessor.h`).
- `Source/Exciter.{h,cpp}` (MOD) — re-engaged in `BassoonVoice::renderNextBlock`. Member layout extended: existing `softShape` array (Phase 2.1 5 ms half-sine × exp; rename for clarity) PLUS new `tonguedShape` array (5-10 ms exp-decaying noise burst, peak-normalised, generated once at `prepare()` from per-Exciter `juce::Random`). Public API additions: `startOnset(float attackChar01, float velocity01) noexcept` — applies velocity bias `effective = clamp(attackChar + (velocity - 0.5f) * 0.3f, 0.0f, 1.0f)`, snapshots `effective` for the onset window lifetime (Phase 2.4 risk #2 mitigation: attack-character is LOCKED at note-on, mid-onset automation only affects next note-on); `getNextSample() noexcept` outputs `mix(softShape[onsetIdx], tonguedShape[onsetIdx], effectiveAttackChar)` for `onsetIdx < numOnsetSamples`, else `0.0f`. `start()` (Phase 2.1 signature) retained as `startOnset(0.0f, 1.0f)` thin wrapper for backwards compatibility (no caller invokes it after Phase 2.4 wires the new path). Onset duration: covers both softShape (30-50 ms) and tonguedShape (5-10 ms) — buffer sized to longer of the two; tonguedShape pads with zeros after its decay completes. NO allocation at runtime; both arrays sized as `std::array<float, MAX_ONSET_SAMPLES>` in-class storage.
- `Source/BassoonVoice.{h,cpp}` (MOD) — Phase 2.4 deltas:
  - `startNote`: REPLACE `f_base = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber)` with `f_base = tuningEngine->getFrequency(midiNoteNumber)` (TuningEngine call — D6 ROADMAP locked); ADD `f_with_NE = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, f_base)` (NE per-noteId snapshot at note-on per locked Q2-rev-4); STORE `currentFrequencyBase = f_with_NE` (replaces direct `f_base` assignment); ADD `exciter.startOnset(*params->getRawParameterValue("attack_character"), velocity)` (re-engages Phase 2.1 Exciter via Phase 2.4 morph API); existing `modeBank.setFundamental(currentFrequencyBase, sampleRate) + modeBank.strike() + adsr.noteOn() + vibrato.reset() + noiseExciter.reset() + breathSmoother init` carry forward unchanged.
  - `renderNextBlock`: ADD per-sample `excitation += exciter.getNextSample()` BEFORE the `noiseExciter.getNextSample(breath)` summation (additive composition during onset window — Phase 2.4 OQ#8 locked). After onset window completes, `exciter.getNextSample()` returns 0; only `NoiseExciter` contributes to sustain. The per-block vibrato + pitch-bend `f_final` compose chain (`currentFrequencyBase × vibratoMult × pitchBendMult`) carries forward verbatim — no Phase 2.4 changes to the dispatch logic, only the `currentFrequencyBase` source (now NE-tuned) changes.
  - NO new APVTS reads (`attack_character` is read once at `startNote` only — voice doesn't shadow it; processor doesn't dispatch it via `setExpression`). Voice-internal latch via `Exciter::startOnset` snapshot.
- `Source/PluginProcessor.{h,cpp}` (MOD) — Phase 2.4 deltas:
  - `juce::Synthesiser synthesiser` → `BassoonSynthesiser synthesiser` (type swap; member name unchanged; PluginProcessor.cpp constructor `synthesiser.addVoice(...)` loops adapt only if `addVoice` API differs — it does not).
  - `processBlock` prologue: ADD `synthesiser.setActiveVoiceCap(static_cast<int>(*params.getRawParameterValue("voice_count")))` BEFORE expression dispatch + NE drain (snapshot for the block — applies to any note-ons that arrive during this `renderNextBlock` call). Throttle by epsilon equivalent (since `voice_count` is `AudioParameterInt`, integer comparison: skip if unchanged from `lastDispatchedVoiceCount`).
  - `prepareToPlay`: `setVoiceCount` initialisation (default 8) — already pre-allocated 16-voice pool from Stage 1; cap just changes the active-count gate.
- `plugins/O-Bassoon/CMakeLists.txt` (MOD) — `target_sources` adds `Source/BassoonSynthesiser.{h,cpp}` (1 NEW translation unit). Stage 1 build flags + `juce_generate_juce_header` order unchanged.
- `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` — append rev-4 note documenting: (a) `BassoonSynthesiser` subclass + cap-with-stealing semantics; (b) Exciter dual-shape morph + velocity bias formula + onset-window latch; (c) f_base compose chain at startNote (`TuningEngine.getFrequency → applyPendingTuning → currentFrequencyBase`); (d) NoiseExciter additive-during-onset behaviour; (e) regression invariants list (Phase 2.1-2.3 patterns preserved).

**Out of scope (deferred to Stage 4):**

- pluginval `--strictness 10` (Stage 4 — Phase 2.4 stays at strictness-5 regression check)
- Windows VST3 build + WebView2 verification (Stage 4)
- Dorico Playback Template integration + microtonal score parity test (Stage 4 — Phase 2.4 verifies VST3 NE event handling via DAW with NE support OR synthetic test fixture; full Dorico parity is Stage 4)
- Aftertouch → vibrato_depth modulation (deferred to v1.1 per Stage 0 D4)
- CC1 → vibrato_depth additive routing (BRIEF.md mention; deferred to v1.1 per Stage 0 — `vibrato_depth` is APVTS-driven only at v1.0)
- UI mockup integration (Stage 3, blocked on UI mockup pass)
- Factory presets + CHANGELOG.md (Stage 4)
- Per-note loudness normalisation (deferred to v1.1 per Phase 2.2 RESEARCH-rev-2 §1.6)

---

### Requirements Confirmed (Phase 2.4-relevant subsets of locked contracts)

- **FUNC-02** (Polyphonic 1-16, default 8): Phase 2.4 primary deliverable. Acceptance: 8 simultaneous notes produce 8 distinct voices; setting `voice_count = 3` then playing 4 notes → only 3 sound, oldest is stolen; rapid `noteOn/noteOff` 10 Hz × 30 s produces no stuck notes (voice-state bookkeeping clean).
- **FUNC-05** (Voice stealing — oldest-note priority): Phase 2.4 closes. Acceptance: when active count meets cap and a new note-on arrives, the stolen voice is preferentially one in ADSR release stage (release-tail-first); if no release-tail voices exist, oldest-noteOn voice is stolen.
- **DSP-05** (Attack-character morph — soft pad ↔ tongued articulation): Phase 2.4 primary deliverable. Acceptance: `attack_character = 0` at low velocity (vel = 20) produces audibly gentle attack; `attack_character = 1` at high velocity (vel = 120) produces audibly percussive attack; `attack_character = 0.5` at mid-velocity produces a smooth mid-morph (no audible discontinuity through the parameter sweep).
- **DSP-06** (Microtonal pitch — VST3 NE pitch ID 0x00000003 + MPE channel pitch-bend): Phase 2.4 closes the structural half. Acceptance: VST3 NE pitch event applied per-noteId at `startNote` (snapshot via `applyPendingTuning`); MPE per-channel pitch-bend continues to route via `pitchWheelMoved` per-voice (Phase 2.1 mechanism). Dorico parity is Stage 4 deliverable; Phase 2.4 verifies NE event handling either via DAW with VST3 NE support (e.g., test fixture) or via synthetic NE event injection at the `pendingTuningSource` table.
- **TuningEngine** (`getFrequency()` per-voice call): Phase 2.4 wiring deliverable (D6 ROADMAP-locked). Acceptance: `BassoonVoice::startNote` calls `tuningEngine->getFrequency(midiNoteNumber)` instead of `MidiMessage::getMidiNoteInHertz`. At default 12-TET A=440, results are bit-identical (regression: pitch ±0 cents vs Phase 2.3 baseline). UI exposure deferred to v1.1.
- **PERF-02** (8-voice CPU < 25 % at 48 k / 256): Phase 2.4 closes. Acceptance: 8 simultaneous voices with vibrato + breath + (mid-onset attack-character) active in Logic-AU CPU meter < 25 % steady-state. Phase 2.3 verify at rev-4 already confirmed PASS at 8 simultaneously-held notes; Phase 2.4 re-verifies under enforced cap.
- **QUAL-02** (Stable long-tone — no drift over 60 s): Phase 2.4 closes (Phase 2.3 skipped per user authority). Acceptance: hold a single note for 60 s with vibrato + breath active — no amplitude drift, no NaN/inf in WAV output (numpy.isfinite scan), no denormal CPU spike (Logic Process bar drift comparison t=10s vs t=60s within ±2 %).
- **PERF-01** (No allocations in `processBlock`): regression check carries forward. New code (`BassoonSynthesiser::findFreeVoice`, Exciter dual-shape, TuningEngine call, applyPendingTuning) must remain allocation-free. `juce::Synthesiser::findFreeVoice` base is allocation-free per JUCE 8 source; `applyPendingTuning` is read-only on atomic table.
- **QUAL-01** (No clicks at note-on / note-off / parameter sweep): regression check. Voice stealing at active-cap boundary must not produce audible click — release-tail-first stealing minimises this naturally; if no release-tail voice exists, JUCE's findVoiceToSteal applies an internal fast-fade on the stolen voice (verified at research-phase OQ#1).
- **DSP-07** (No O-Reed dependency): regression grep at execute-phase. Phase 2.4 introduces NO O-Reed includes or sources.

**Closes Stage 2 traceability:** FUNC-02, FUNC-05, DSP-05, DSP-06, PERF-02, QUAL-02 transition pending → complete at Phase 2.4 verify. After Phase 2.4 verify-phase atomic commit, the only remaining requirements with non-complete status are COMPAT-01 (Stage 4: strictness-10 + Windows), COMPAT-02 (Stage 4: Dorico parity), and UI-01/UI-02 (Stage 3 blocked on mockup).

---

### Constraints Identified (rev-4)

**Locked contracts (do NOT modify in this cycle):**

- All 10 APVTS parameter IDs, ranges, defaults — `parameter-spec-draft.md` and `PluginProcessor.cpp:25-99`. Phase 2.4 reads TWO additional from APVTS: `voice_count` (`AudioParameterInt` 1-16, default 8) at processor `processBlock` prologue, `attack_character` (`AudioParameterFloat` 0-1, default 0.0) at voice `startNote` only. Param IDs / ranges / defaults are NOT modified.
- DSP architecture (`research/ARCHITECTURE.md`) — Phase 2.4 implements the architecture's Phase-2.4 subset (voice manager, attack-character morph, NE/MPE/TuningEngine integration) per ROADMAP lines 192-251 verbatim. ARCHITECTURE.md is appended to (rev-4 note), not rewritten.
- ROADMAP Phase 2.4 spec (lines 194-251) — components, test criteria, requirements verified. Phase 2.4 implements the locked spec; deviations require explicit CONTEXT-rev-4 callout.
- Phase 2.1 + Phase 2.2 + Phase 2.3 wiring contracts that carry forward unchanged:
  - `PluginProcessor::processBlock` ordering: `voice_count` snapshot (NEW Phase 2.4) → tone-dispatch → expression-dispatch → NE drain → `synthesiser.renderNextBlock` → output_gain `applyGainRamp`. NE-drain BEFORE renderNextBlock invariant is preserved (Phase 2.4 actually consumes the drained table for the first time at voice level — `applyPendingTuning` reads what NE drain just wrote).
  - `BassoonVoice` member layout: all existing Phase 2.1-2.3 members preserved. Phase 2.4 ADDS no new voice-level members (Exciter morph state lives inside Exciter; voice-count cap lives inside BassoonSynthesiser). Voice-level changes are limited to `startNote` body (4 new lines: TuningEngine call, applyPendingTuning call, currentFrequencyBase assignment shift, `exciter.startOnset` call) and `renderNextBlock` per-sample loop (1 new line: `excitation += exciter.getNextSample()`).
  - Per-sample render-loop ordering: `excitation = noiseExciter.getNextSample(breath) + exciter.getNextSample()` → `modeBank.processSample(excitation)` → `× adsr.getNextSample()` → `addSample(L, R)`. Additive Exciter contribution during onset window only (auto-zeros after onset-window completes via `onsetIdx >= numOnsetSamples` guard).
  - Mode-bank coefficient-update cadence: on note-on (Phase 2.1) + on tone-change > epsilon (Phase 2.2) + on |Δf_final| > 0.1 Hz (Phase 2.3). Phase 2.4 changes the *source* of `currentFrequencyBase` (now NE-tuned at startNote) but NOT the dispatch cadence. Same `setFundamental` call site.
  - Stage 1 build flags + CMakeLists structure — Phase 2.4 only adds `Source/BassoonSynthesiser.{h,cpp}` to `target_sources`. CMake flags unchanged. `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `PLUGIN_CODE OBsn` all preserved.
  - DSP-07 (no O-Reed dependency) — verified at Stage 1, re-grepped at Phase 2.1/2.2/2.3 verify, regression-checked at Phase 2.4 verify.
  - Continuous-noise excitation as primary sustain source (Phase 2.3 architectural pivot) — preserved verbatim. Phase 2.4 layers Exciter morph ON TOP during onset window only.
  - rev-3 `strike()` retained at `BassoonVoice::startNote` for the modal-state attack transient — preserved.
  - 1/8 headroom scaler at ModeBank (Phase 2.2 relaxation from 1/16) — preserved.
  - Throttled-epsilon dispatch pattern + aggregate `setExpression` setter (Phase 2.3) — preserved. Phase 2.4 does NOT add `attack_character` to `setExpression` because it's snapshot-at-startNote only (per locked Q3-rev-4 attack-character latch decision).
  - VST3 NE drain wiring at `PluginProcessor::processBlock` (Stage 1 + Phase 2.3 ordering — Phase 2.4 voices now consume the drained table; the drain logic itself is unchanged).

**JUCE 8 critical patterns (auto-loaded `spike-findings-VST-development` + memory):**

- `juce::Synthesiser::setNoteStealingEnabled(true)` — JUCE 8.0.4 default is `true` already, but explicit set in `BassoonSynthesiser` constructor for clarity. Required for stealing to occur when active-cap is hit.
- `juce::Synthesiser::findVoiceToSteal(SynthesiserSound*, int channel, int noteNumber)` — JUCE 8 default implementation: walks active voices, prefers (a) voices in release stage (ADSR isReleasing), (b) oldest-noteOn voice. Two-pass logic. We rely on this via `BassoonSynthesiser::findFreeVoice` delegation.
- `juce::Synthesiser::findFreeVoice(SynthesiserSound*, int channel, int noteNumber, bool stealIfNoneAvailable)` — JUCE 8 default: walks `voices` array, returns first non-active voice; if all active and `stealIfNoneAvailable`, calls `findVoiceToSteal`. Override pattern: gate by active-cap before delegating.
- `juce::SynthesiserVoice::isVoiceActive()` — returns `true` if voice is rendering (note-on through release-tail-end). Used to count active voices for cap enforcement.
- `juce::Random` per-Exciter for `tonguedShape` generation — pre-baked once at `prepare()` (off audio thread); allocation-free. Same pattern as Phase 2.3 NoiseExciter.
- `juce::ScopedNoDenormals` at `processBlock` entry — already in place. 60 s sustain regression: continuous-noise + modal-bank state evolution over 60 s relies on FTZ for denormal protection.
- `Ouaricon::NoteExpression::applyPendingTuning(table, midiNote, baseFreq)` — read-only call on atomic table. RT-safe per O-Lyrica spike validation. Returns `baseFreq` unchanged when no NE event has set the table entry for that noteId — bit-identical fallback to `getFrequency` result.
- `TuningEngine::getFrequency(int midiNote)` — global namespace (D2 from Stage 1). At default 12-TET A=440 with no scale loaded, returns `juce::MidiMessage::getMidiNoteInHertz(midiNote)`-equivalent (research-phase OQ#5 confirms bit-identity).
- Allocation-free `processBlock`: `BassoonSynthesiser::findFreeVoice` is allocation-free (no `new`/`make_unique`). `applyPendingTuning` is allocation-free (atomic load only). `Exciter::startOnset` writes to in-class storage. `tonguedShape` array is in-class `std::array<float, MAX_ONSET_SAMPLES>` — populated once in `prepare()`.

**Phase 2.4-specific constraints:**

- **Single-pass cycle scope** (locked Q1-rev-4 batch 1): all 4 systems (voice manager + attack-character + NE + TuningEngine) ship in one Phase 2.4 cycle, single Gate 4, single atomic commit. Matches Phase 2.1/2.2/2.3 cadence.
- **NE consumption at startNote-only snapshot** (locked Q2-rev-4 batch 1): `applyPendingTuning` runs ONCE in `BassoonVoice::startNote` after `tuningEngine->getFrequency`. Voice frequency for the lifetime of the note is locked at note-on. Live retuning during sustain (per-block `applyPendingTuning` re-read) is explicitly OUT of scope. Matches O-Lyrica precedent. Dorico's NE pitch fires at/before note-on; mid-note retuning is not a current use case.
- **Attack-character ear-only A/B at v1.0** (locked Q3-rev-4 batch 1): subjective verification — `attack_character = 0` at low velocity audibly soft, `= 1` at high velocity audibly tongued, `= 0.5` mid-morph audibly intermediate. Existing VSCO-2-CE C3 sustain WAV stays as the reference for tone (Phase 2.2). No new tongued-articulation reference recording sourced this cycle.
- **QUAL-02 60 s gate included as Gate 4 item** (locked Q4-rev-4 batch 1): Logic-AU 60 s bounce, Python `numpy.isfinite` scan over output WAV, Logic Process bar CPU drift t=10s vs t=60s (±2 % steady-state tolerance), ear-listen for amplitude drift. Closes QUAL-02 partial → complete at Phase 2.4.
- **`voice_count` change applies at next note-on** (locked Q1-rev-4 batch 2 = ROADMAP locked): processBlock prologue snapshots APVTS value into `BassoonSynthesiser::activeVoiceCap`; already-active voices continue rendering until natural release. Lowering cap from 8 → 3 with 8 voices ringing: next 5 note-ons get stolen voices, current 8 sustain. No mid-render voice termination. Matches ROADMAP Phase 2.4 "applies on next note-on" semantics.
- **Voice stealing release-tail-first, then oldest-noteOn** (locked Q2-rev-4 batch 2 = JUCE default): rely on `juce::Synthesiser::findVoiceToSteal` two-pass default behaviour. Override `findFreeVoice` only — does NOT override `findVoiceToSteal`. The two-pass eligibility (release-tail-first) is JUCE 8.0.4 default; verified at research-phase OQ#1.
- **Inline iteration ceiling at rev-3** (locked Q3-rev-4 batch 2): matches Phase 2.2 + Phase 2.3 precedent. Allows in-cycle: rev-1 (initial plan), rev-2 (research/plan refinement), rev-3 (inline verify-phase fix). Forces a fresh discuss-phase cycle if verify-phase finds defects requiring rev-4+. Phase 2.4 has 4 systems vs Phase 2.3's 4; ceiling is judged adequate.
- **Atomic commit subject locked** (locked Q4-rev-4 batch 2): `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`. Single commit on Gate 4 PASS only. Lands `Source/BassoonSynthesiser.{h,cpp}` (NEW) + `Source/Exciter.{h,cpp}` (MOD: tonguedShape + startOnset morph) + `Source/BassoonVoice.{h,cpp}` (MOD: startNote NE/TuningEngine wiring + renderNextBlock additive Exciter) + `Source/PluginProcessor.{h,cpp}` (MOD: BassoonSynthesiser type swap + voice_count snapshot) + `CMakeLists.txt` (MOD: target_sources +1) + `research/ARCHITECTURE.md` rev-4 note + 5 planning artefacts (CONTEXT-rev-4 / RESEARCH-rev-4 / PLAN-rev-4 / SUMMARY-rev-4 / VERIFICATION-rev-4) + STATUS update + REQUIREMENTS update on Gate 4 PASS only.
- **Phase 2.3 atomic commit MUST land BEFORE Phase 2.4 execute-phase begins** (process invariant): cleanest working tree boundary. Phase 2.4 discuss/research/plan can proceed in parallel against the in-tree state, but execute-phase blocks on a clean `main` baseline at `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`.
- **Gate 4 PASS bar — 10 user-checkable items + automated invariants** (research-phase locks final list): user-checkable: (1) 8 simultaneous notes audibly distinct, (2) `voice_count = 3` + 4 notes → 3 sound, oldest-released stolen first, (3) rapid 10 Hz noteOn/noteOff × 30 s no stuck notes, (4) `attack_character = 0` + vel=20 audibly soft, (5) `attack_character = 1` + vel=120 audibly tongued, (6) `attack_character = 0.5` mid-velocity smooth mid-morph, (7) MPE-enabled DAW: per-channel pitch-bend routes per voice, (8) VST3 NE pitch event shifts voice pitch as expected (DAW or synthetic test fixture), (9) 60 s long-tone QUAL-02 final (numpy.isfinite + RMS drift + CPU drift), (10) 8-voice CPU < 25 % @ 48 k / 256 (PERF-02 final). Plus automated invariant battery (RT-safety grep, NE drain ordering, mode-bank dispatch site, applyGainRamp form, 1/8 scaler retention, expression dispatch site, throttle epsilons, DSP-07, auval, pluginval-5).

**Working-tree starting state (locked from Phase 2.3 in-tree state, post-rev-4 inline iteration; atomic commit pending user trigger):**

- `Source/ModeBank.{h,cpp}` — Phase 2.2 bassoon-tuned partial table + formant Gaussian + tone wiring + applyToneChange + strike() + cached cosTheta/sinTheta/amp + 1/8 headroom scaler + isfinite NaN guard + Nyquist mute @ 0.45·fs. Phase 2.4 leaves untouched.
- `Source/Exciter.{h,cpp}` — Phase 2.1 5 ms half-sine × exp impulse + class-level `std::array<float, 1024>` storage. Phase 2.4 RE-ENGAGES via `startOnset(attackChar, velocity)` API + adds `tonguedShape` array.
- `Source/Vibrato.{h,cpp}` — Phase 2.3 per-voice sine LFO + onset SmoothedValue + random phase per startNote. Phase 2.4 leaves untouched.
- `Source/NoiseExciter.{h,cpp}` — Phase 2.3 per-voice 1-pole LP @ 2 kHz + BASE_NOISE_GAIN 0.05f + breath-scaled. Phase 2.4 leaves untouched (additive composition during onset window via voice-render summation, not via NoiseExciter API change).
- `Source/BassoonVoice.{h,cpp}` — Phase 2.3 surface (Vibrato + NoiseExciter + breathSmoother + lastDispatchedFrequency + 5 expression dispatch shadows + CC2-takeover state machine + voiceIndex + setExpression aggregate setter). Phase 2.4 modifies `startNote` (4 new lines for TuningEngine + applyPendingTuning + Exciter.startOnset) + `renderNextBlock` per-sample loop (1 new line: `excitation += exciter.getNextSample()`). NO new members.
- `Source/PluginProcessor.{h,cpp}` — Phase 2.3 surface (toneSmoother + outputGainSmoother + 6 expression dispatch shadows + processBlock prologue ordering). Phase 2.4 modifies `synthesiser` member type (`juce::Synthesiser` → `BassoonSynthesiser`) + adds `lastDispatchedVoiceCount` shadow + adds `voice_count` snapshot dispatch in processBlock prologue.
- `CMakeLists.txt` — Phase 2.3 sources include ModeBank + Exciter + Vibrato + NoiseExciter + BassoonVoice + BassoonSound + PluginProcessor + PluginEditor. Phase 2.4 adds `BassoonSynthesiser.{h,cpp}`.
- Build state at Phase 2.3 verify rev-4: `O-Bassoon_VST3` + `O-Bassoon_AU` + `O-Bassoon_Standalone` install fresh; `auval -v aumu OBsn OuDv` SUCCESS; `pluginval --strictness 5` SUCCESS; 10/10 static-check grep gates PASS. Phase 2.4 baseline matches.

---

### Approach Decisions (rev-4)

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Q1-rev-4 (batch 1) — Phase 2.4 cycle scope** | **Single pass — all 4 systems in one Phase 2.4 cycle.** Voice manager + cap + stealing (FUNC-02/05), attack-character morph (DSP-05), VST3 NE per-voice + MPE pitch-bend per-channel + TuningEngine getFrequency wiring (DSP-06). Single Gate 4, single atomic commit. | Matches Phase 2.2/2.3 cadence. The 4 systems are tightly coupled at the f_base compose chain (NE + TuningEngine compose at startNote; voice cap gates the synthesiser at processBlock prologue; attack-character morph layers on the existing exciter). Sub-phases would 3× planning overhead without delivering more atomicity. User-confirmed (recommended). |
| **Q2-rev-4 (batch 1) — VST3 Note Expression consumption timing** | **startNote-only snapshot.** `applyPendingTuning` runs once in `BassoonVoice::startNote` — note-on tuning locks for the lifetime of the voice. Mid-note retuning is OUT of scope. | Matches O-Lyrica HarpSynthVoice precedent. Dorico's NE pitch fires at/before note-on, so this is sufficient for Ouaricon family parity. Avoids per-block applyPendingTuning re-read overhead (16 voices × 1 atomic load × every block = 16k loads/s — negligible but unnecessary for current use case). User-confirmed (recommended). |
| **Q3-rev-4 (batch 1) — Attack-character A/B verification protocol** | **Ear-only at v1.0.** Subjective verification: attack_character=0 (Soft) vs. =1 (Tongued) extremes audibly different at low/high velocity, mid-morph (=0.5) sounds like a sensible blend. Existing VSCO-2-CE C3 sustain WAV stays as the reference for tone (Phase 2.2 archive). | Tongued-articulation recordings sourced post-v1.0 if needed. Avoids Phase 2.4 prereq task bloat. The character morph is a high-degree-of-freedom design space that benefits from iterative ear-tuning more than spectral A/B analysis. User-confirmed (recommended). |
| **Q4-rev-4 (batch 1) — QUAL-02 60 s long-tone gate placement** | **Include in Gate 4** as item #9 (or wherever in the 10-item bar). Phase 2.3 skipped this; Phase 2.4 closes it. | Continuous-noise excitation + pole-only biquad + isfinite guards make 60 s sustain pass-able; Phase 2.3's actual code already supports it (skip was a user authority choice, not a defect). Closing QUAL-02 at Phase 2.4 keeps Stage 4 narrowly-scoped to compatibility/Dorico parity/strictness-10. User-confirmed (recommended). |
| **Q1-rev-4 (batch 2) — voice_count APVTS change behavior** | **Next note-on** (ROADMAP locked). voice_count snapshot at processBlock prologue stored in `BassoonSynthesiser::activeVoiceCap`. Already-active voices keep playing until natural release. | Per ROADMAP Phase 2.4 spec; matches user expectation of "change applies to next phrase." Immediate prune would cause audible artifacts during DAW automation of voice_count — unacceptable for a long-tone instrument. User-confirmed (recommended). |
| **Q2-rev-4 (batch 2) — Voice stealing eligibility** | **Release-tail-first, then oldest-noteOn sustaining.** Two-pass logic: prefer voices already in ADSR release stage (their tail is fading anyway, abrupt cut is least audible); if no release-tail voices exist, steal oldest-noteOn sustaining voice. JUCE 8 `Synthesiser::findVoiceToSteal` default already implements this. | Free behaviour from JUCE — we just enable `setNoteStealingEnabled(true)` and the default does the right thing. Strict oldest-noteOn (any state) would cut a fresh sustain in favor of preserving an older release tail — audibly worse for the user. User-confirmed (recommended). |
| **Q3-rev-4 (batch 2) — Inline iteration ceiling** | **rev-3 ceiling.** Matches Phase 2.2 + Phase 2.3 precedent. Allows: rev-1 initial plan, rev-2 research/plan refinement, rev-3 inline verify-phase fix. Forces fresh discuss cycle if verify finds defects requiring rev-4+. | Bounds iteration loop so Phase 2.4 ships. Phase 2.3 used rev-3 ceiling and shipped (rev-4 was an inline absorption within rev-3, not a new-cycle trigger). Phase 2.4 has comparable system count; same ceiling. User-confirmed (recommended). |
| **Q4-rev-4 (batch 2) — Atomic commit subject** | **`feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`.** Subject names the 3 user-facing system pivots (polyphony, microtonal pitch, attack character). Body lists FUNC-02 + FUNC-05 + DSP-05 + DSP-06 closure + QUAL-02 60s revisit + PERF-02 final + TuningEngine wiring. | Mirrors Phase 2.1/2.2/2.3 cadence. Subject naming the systems makes `git log --oneline` immediately legible. User-confirmed (recommended). |
| **Derived: BassoonSynthesiser subclass over `juce::Synthesiser`** | NEW translation unit `Source/BassoonSynthesiser.{h,cpp}`. Override `findFreeVoice` only (delegate to base for free-pool selection; gate by `activeVoiceCap`). DO NOT override `findVoiceToSteal` (rely on JUCE default). | ROADMAP-locked. Smallest possible override surface — gets us release-tail-first stealing for free. |
| **Derived: voice_count snapshot at processBlock prologue** | `synthesiser.setActiveVoiceCap(static_cast<int>(*params.getRawParameterValue("voice_count")))` BEFORE expression dispatch + NE drain. Integer comparison throttle (skip if unchanged from `lastDispatchedVoiceCount`). | Reads APVTS once per block off-audio-thread-pattern; voice cap takes effect for note-ons that arrive in this block. Snapshot per block matches ROADMAP "applies on next note-on" semantics. |
| **Derived: Exciter dual-shape morph + onset-window latch** | At `Exciter::startOnset(attackChar, velocity)`: snapshot `effectiveAttackChar = clamp(attackChar + (velocity - 0.5f) * 0.3f, 0, 1)` for the onset window lifetime. Mid-onset automation of attack_character does NOT affect the in-flight onset; it affects the next note-on. | Phase 2.4 risk #2 mitigation (zipper avoidance). Consistent with attack character being a "performance gesture" parameter, not a "modulation" parameter. Eliminates need for parameter smoothing on attack_character. |
| **Derived: Velocity bias formula** | `effective = clamp(attackChar + (velocity - 0.5f) * 0.3f, 0, 1)`. Magnitude 0.3f starting point — research-phase locks final value (likely 0.2-0.4 range). | Per ROADMAP locked formula. Velocity = 0.5 (mid) leaves attack_character unchanged; velocity = 0 (very soft) biases toward 0 (soft); velocity = 1 (very hard) biases toward 1 (tongued). Matches woodwind articulation physics. |
| **Derived: Soft shape array provenance** | Repurpose Phase 2.1 `softShape` array as `softShape` in Phase 2.4 (rename for clarity if conflict). 5 ms half-sine × exp at peak amplitude. Research-phase verifies whether ROADMAP's "30-50 ms half-sine, low-passed at ~600 Hz" requires a longer shape — likely YES, in which case Phase 2.4 generates a longer softShape and replaces. | Phase 2.1 array is closer to "tongued" character (5 ms, full-bandwidth) than "soft pad". Research-phase resolves whether to (a) keep Phase 2.1 array as `tonguedShape`-equivalent and generate new long+lowpassed softShape, or (b) keep Phase 2.1 as softShape and generate new tonguedShape. Reverberates into Risk #6. |
| **Derived: Tongued shape generation** | Pre-baked at `Exciter::prepare()`: `tonguedShape[i] = (rng.nextFloat() * 2.0f - 1.0f) * exp(-i / decaySamples)` where `decaySamples = 7.5 ms × sampleRate`. Peak-normalised to ±1.0. Per-Exciter `juce::Random` (not shared with NoiseExciter; different seed). | Cheap to generate (one-shot at prepare). Exp-decay envelope on white noise gives "puff" articulation. Decay = 7.5 ms is mid-range of ROADMAP's "5-10 ms" spec. Research-phase locks the decay value. |
| **Derived: NoiseExciter behaviour during onset window** | Additive composition: `excitation = noiseExciter.getNextSample(breath) + exciter.getNextSample()`. Both contribute during onset window (~30-50 ms); after onset window completes, only NoiseExciter contributes. | NoiseExciter at low breath-scaled level adds air-column hiss texture under the attack impulse. Locked Q2-rev-4 OQ#8 carry-forward. |
| **Derived: f_base compose chain at startNote** | `f_base = tuningEngine->getFrequency(midiNoteNumber)` → `f_with_NE = applyPendingTuning(*pendingTuningSource, midiNoteNumber, f_base)` → `currentFrequencyBase = f_with_NE`. Per-block f_final compose carries forward: `f_final = currentFrequencyBase × vibratoMult × pitchBendMult`. | Matches Phase 2.3 OQ#7-rev-3 compose order (NE-tuned × vibrato × pitch-bend). NE and TuningEngine fold into `currentFrequencyBase` once at startNote; vibrato + pitch-bend operate on the NE-tuned base per-block. O-Lyrica precedent. |
| **Derived: TuningEngine.getFrequency vs MidiMessage::getMidiNoteInHertz fallback** | At default 12-TET A=440, `TuningEngine::getFrequency` returns same value as `MidiMessage::getMidiNoteInHertz`. Phase 2.4 swap is bit-identical baseline (regression: pitch ±0 cents vs Phase 2.3). UI exposure of TuningEngine deferred to v1.1. | Confirmed via O-Wind/O-Lyrica precedent. Headless wiring at v1.0 means future v1.1 UI changes are pure additive (no rewire). |
| **Derived: MPE per-channel pitch-bend routing** | Already wired at Phase 2.1 `BassoonVoice::pitchWheelMoved`. JUCE Synthesiser routes per-channel pitch-bend events to the voice on that channel automatically in MPE mode. Phase 2.4 verifies this works in MPE-enabled DAW. | No new code — JUCE 8 Synthesiser handles MPE-mode pitch-bend routing transparently. Phase 2.4 verification is empirical (DAW with MPE enabled). |
| **Derived: NE event handling verification protocol** | Two paths: (a) DAW with VST3 NE support (e.g., test fixture sending synthetic NE events); (b) Dorico in Stage 4 (full parity). Phase 2.4 closes structural half via path (a); Stage 4 closes integration half via path (b). | Decouples Phase 2.4 (DSP wiring) from Stage 4 (Dorico Playback Template integration). Allows Phase 2.4 to ship without depending on Dorico installation. |
| **Derived: 8-voice CPU final measurement protocol** | Hold 8 simultaneous notes (chord spread C2-Bb3 or similar) in Logic-AU with vibrato 5 Hz / 50 c + breath = 0.7 + attack_character mid-onset active. Logic Process bar reading at steady-state. Bar < 25 % per ROADMAP PERF-02. | Mirrors Phase 2.3 protocol; Phase 2.4 enforces voice_count = 8 cap (vs Phase 2.3's "8 simultaneously-held" without cap). |
| **Derived: 60 s long-tone protocol (Phase 2.3 carry-forward)** | Logic-AU 60 s bounce at C3 with vibrato + breath active. Python `numpy.isfinite` over output WAV (zero NaN/inf samples). 1-second windowed RMS drift check (max - min < 0.5 dB). Logic Process bar CPU drift t=10s vs t=60s within ±2 %. | Locks the protocol Phase 2.3 deferred. Continuous-noise excitation should pass cleanly (no impulse-decay drift mechanism). |

---

### Open Questions (handed to research-phase) — rev-4

1. **`juce::Synthesiser::findFreeVoice` override — JUCE 8.0.4 canonical pattern.** Subclass `juce::Synthesiser` as `BassoonSynthesiser`. Override signature: `juce::SynthesiserVoice* findFreeVoice(juce::SynthesiserSound*, int channel, int noteNumber, bool stealIfNoneAvailable) const override`. Walk active voices via the inherited `voices` array (or `getNumVoices()` + `getVoice(i)` + `voice->isVoiceActive()`). Active count comparison vs `activeVoiceCap`. Confirm: (a) `voices` is accessible from subclass (protected member or `getVoice()` only?); (b) `findVoiceToSteal` is `const` so override must also be `const`; (c) `setNoteStealingEnabled(true)` is needed (default is `true` — verify JUCE 8.0.4 default). Cite line numbers from `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp`.

2. **`voice_count` APVTS read site — processBlock prologue snapshot.** Pattern: `synthesiser.setActiveVoiceCap(static_cast<int>(*params.getRawParameterValue("voice_count")))`. Confirm: (a) `AudioParameterInt` returns float via `getRawParameterValue` (rounded to nearest int when stored — verify); (b) the snapshot site (BEFORE expression dispatch + NE drain) is correct ordering (voice_count cap takes effect for note-ons that arrive in this `renderNextBlock` call); (c) integer-comparison throttle is sound (skip if unchanged from `lastDispatchedVoiceCount`).

3. **Soft vs Tongued shape arrays — duration, spectrum, generation method.** ROADMAP softShape: "30-50 ms half-sine impulse, low amplitude, low-passed at ~600 Hz". tonguedShape: "5-10 ms exponentially decaying noise burst, full bandwidth, higher peak". Phase 2.1 Exciter is currently 5 ms half-sine × exp — closer to tongued character than soft. Lock the as-shipped Phase 2.4 spec: (a) keep Phase 2.1 array as `tonguedShape`-equivalent (rename + add LP-filtered long softShape), or (b) keep Phase 2.1 as softShape and generate new tonguedShape with noise + exp-decay? Recommend (b) — shorter migration cost; softShape stays at 5 ms. Verify subjective acceptability via internal-listening before plan-phase locks.

4. **Velocity bias magnitude — 0.3 vs 0.2 vs 0.4.** ROADMAP locks formula `effective = clamp(attackChar + (velocity - 0.5f) * 0.3f, 0, 1)`. The 0.3 magnitude is a starting estimate. Research O-Wind / O-Lyrica precedent for velocity-bias magnitudes on similar parameters (NB: O-Reed is excluded per DSP-07 — research only the public API patterns, not O-Reed source). Lock the magnitude; default 0.3 is acceptable barring evidence otherwise.

5. **`TuningEngine::getFrequency` vs `juce::MidiMessage::getMidiNoteInHertz` bit-identity check.** Phase 2.1-2.3 use `getMidiNoteInHertz(midiNote)`. Phase 2.4 swaps to `tuningEngine->getFrequency(midiNote)`. At default 12-TET A=440 with no scale loaded, both should return identical float values. Cite TuningEngine source at `modules/tuning/scala-tuning-engine/src/TuningEngine.{h,cpp}`. Confirm: `TuningEngine::getFrequency(int)` signature, default-constructed behavior (12-TET A=440), thread-safety on audio thread (read-only call).

6. **`applyPendingTuning` API + compose order at startNote.** Compose chain: `f_base = tuningEngine->getFrequency(midiNoteNumber)`; `f_with_NE = Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, f_base)`; `currentFrequencyBase = f_with_NE`. Confirm O-Lyrica precedent (spike findings auto-load) — specifically `Source/HarpSynthVoice.cpp` startNote body. Cite line numbers. Verify: `applyPendingTuning` signature, return semantics when no NE event has set the table entry (returns f_base unchanged?).

7. **MPE channel pitch-bend per-channel — already wired or new code needed.** Phase 2.1 `BassoonVoice::pitchWheelMoved(int newValue)` uses raw 14-bit value with ±2 semitones range. JUCE `juce::Synthesiser` routes per-channel pitch-bend events to voices on that channel in MPE-enabled DAW. Verify: (a) JUCE 8.0.4 Synthesiser pitch-bend routing for MPE — is per-channel routing automatic, or does it require a different Synthesiser variant (`MPESynthesiser`)? (b) D3-Stage-0 ROADMAP locks `juce::Synthesiser` (not MPESynthesiser); confirm per-channel pitch-bend still works correctly in MPE mode. Cite `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.cpp` MIDI dispatch logic.

8. **NoiseExciter additive vs muted during onset window.** Phase 2.3 has continuous noise excitation via NoiseExciter. Phase 2.4 adds attack-character impulse via Exciter. Recommend additive: `excitation = noiseExciter.getNextSample(breath) + exciter.getNextSample()`. NoiseExciter at low breath-scaled level adds air-column hiss texture under the attack. Alternative: mute NoiseExciter during onset (`if (exciter.inOnset()) skip noise`), let Exciter dominate. Research-phase: ear-judgment — does additive composition produce a more natural attack or a noisier-than-expected attack? Likely additive wins, but lock at research-phase.

9. **`juce::Synthesiser::getNumActiveVoices()` or manual count.** Active-voice count for cap enforcement: (a) JUCE 8 may expose `getNumActiveVoices()` as a public method; (b) if not, manual loop: `int active = 0; for (auto* v : voices) if (v->isVoiceActive()) ++active;`. Cite JUCE 8.0.4 source. If manual loop required, confirm allocation-free + RT-safe.

10. **Gate 4 PASS bar — finalise the 10-item user-checkable list + automated invariants.** Items (per CONTEXT-rev-4 §Phase 2.4-specific constraints — locked Q*-rev-4): (1)-(10) as listed. Confirm: (a) item 8 (NE pitch event verification) — DAW path or synthetic test fixture? Recommend DAW with VST3 NE support (Logic Pro Mac MAY work; confirm). If no DAW path is feasible at Phase 2.4, the synthetic test fixture is a `pendingTuningSource->setTuning(noteId, deltaCents)` direct-call from a test harness or runtime debug build. Document. (b) item 7 (MPE pitch-bend) — which DAW for verification? Logic Pro MPE support varies by version; Bitwig/Ableton with M4L/MPE-compatible controller may be more reliable. Lock the verification path.

---

### Risks (Phase 2.4-specific)

1. **Voice stealing produces audible click on stolen voice.** When active count meets cap and a new note-on arrives, JUCE's findVoiceToSteal returns oldest-noteOn or release-tail voice; the caller calls `voice->stopNote(0.0f, false)` (NO tail-off) before `voice->startNote(...)`. The forced-stop on a sustaining voice produces an instantaneous amplitude cut → audible click. Mitigation: JUCE 8 `Synthesiser::findVoiceToSteal` two-pass logic preferentially picks release-tail-first (fade is already in progress, click is masked by the tail). For oldest-noteOn-stolen sustains, the click is psychoacoustically masked by the simultaneous new note-on that triggered the steal (perceptual fusion). If audibly bad at verify, surface as v1.1 enhancement — internal forced fast-fade (5-10 ms ADSR-release before re-trigger).

2. **Attack-character mid-onset zipper.** Crossfading between two pre-baked arrays during onset window (~30-50 ms); user automating attack_character mid-onset (e.g., DAW automation lane stepping at 30 Hz) could produce zipper if `effectiveAttackChar` is read per-sample. Mitigation: locked Q3-rev-4 derived decision — `effectiveAttackChar` is SNAPSHOTTED at note-on (`Exciter::startOnset` latches the value for the onset window lifetime). Mid-onset automation only affects next note-on. Eliminates zipper risk by design.

3. **NE applyPendingTuning thread-safety.** `pendingTuningSource` is shared between processor's NE drain (audio thread, top of processBlock) and per-voice startNote (also audio thread, during `synthesiser.renderNextBlock` MIDI dispatch). Both are on audio thread; ordering matters: NE drain MUST run BEFORE renderNextBlock to ensure voices read fresh data at startNote. Mitigation: Stage 1 wiring already enforces this (`vst3Extensions.drainAndUpdate()` BEFORE `synthesiser.renderNextBlock`); Phase 2.4 verifies the ordering invariant is preserved (regression grep at PluginProcessor.cpp prologue site). Atomic table reads at applyPendingTuning are lock-free per O-Lyrica spike validation.

4. **TuningEngine API mismatch.** Phase 2.4 calls `tuningEngine->getFrequency(midiNoteNumber)`. If the actual TuningEngine API differs (e.g., `getFrequency(int, double)` takes sample rate, or returns `double` not `float`, or namespace mismatch from D2 Stage 1), Phase 2.4 plan-phase has to refactor. Mitigation: research-phase OQ#5 verifies the exact signature. If signature is `double getFrequency(int)`, cast to float at call site.

5. **MPE per-channel pitch-bend not routed correctly in JUCE Synthesiser.** Phase 2.1 wired `pitchWheelMoved` to `pitchBendSemitones` member, recomputed `f_modulated = currentFrequencyBase * pow(2, semis/12)`. In MPE mode, each note is on its own channel; JUCE Synthesiser's MIDI dispatch should route per-channel pitch-bend to the voice on that channel — but only if the Synthesiser is configured for MPE-aware routing. Mitigation: research-phase OQ#7 verifies. Stage 0 D3 locked `juce::Synthesiser` (not `MPESynthesiser`); if per-channel routing requires `MPESynthesiser`, Phase 2.4 surfaces as a deviation requiring rev-2 (research absorbs).

6. **Exciter file rot — Phase 2.1/2.4 API divergence.** Phase 2.3 retained `Source/Exciter.{h,cpp}` verbatim (no calls from renderNextBlock). Phase 2.4 modifies it: adds `tonguedShape` member, adds `startOnset(attackChar, velocity)` method, modifies `getNextSample()` to mix between two arrays. Risk: Phase 2.1 method signatures (`start()`) have no callers post-Phase-2.3, so API removal is safe — but if Phase 2.4 plan-phase hits an unforeseen interaction (e.g., voice render still expects single-array buffer), refactor cost. Mitigation: Phase 2.4 retains `start()` as a thin wrapper (`startOnset(0.0f, 1.0f)`); makes `softShape` + `tonguedShape` distinct in-class arrays; mix happens in `getNextSample()`. Backward-compatible.

7. **`voice_count = 1` mono-mode edge case.** Single voice, fast notes → severe stealing. User expectation: mono synth behavior (each new note steals the previous). Mitigation: documented as expected behavior; no special-case needed. JUCE's findFreeVoice returns the single voice, findVoiceToSteal returns the same voice, caller calls stopNote then startNote — standard mono-cycle.

8. **Polyphony stealing during high-rate note retriggers.** Test criterion: "rapid noteOn/noteOff 10 Hz × 30 s no stuck notes." At 10 Hz alternating noteOn/noteOff with cap = 1, every note steals the previous. Risk: voice-state bookkeeping race (e.g., `clearCurrentNote()` not called before next `startNote`). Mitigation: JUCE 8 Synthesiser's stop-then-start sequence is atomic w.r.t. voice state — no race. Verify at Gate 4 item 3.

9. **60 s long-tone QUAL-02 regression with new Phase 2.4 code.** Phase 2.4 adds: voice manager (cold path during sustain), NE consumption (cold path post-startNote — startNote-only snapshot), TuningEngine call (cold path post-startNote), Exciter morph (cold path post-onset-window). All are cold paths during sustain — no per-block additions to the hot path. QUAL-02 should pass cleanly. Mitigation: Phase 2.4 verify includes the same Phase 2.3-protocol QUAL-02 test (Logic-AU 60 s bounce + numpy.isfinite + RMS drift + CPU drift). Regression detection.

10. **Phase 2.3 atomic commit dependency for Phase 2.4 execute.** Phase 2.3 atomic commit is PENDING explicit user trigger at the time of CONTEXT-rev-4 write. Phase 2.4 discuss/research/plan can proceed against in-tree state, but execute-phase MUST NOT begin until `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS` lands on `main`. Risk: process drift if Phase 2.4 execute starts prematurely → Phase 2.3 + Phase 2.4 changes commingle in a single commit, breaking atomic-commit-per-phase invariant. Mitigation: PLAN-rev-4 task #1 = "Verify Phase 2.3 atomic commit lands on `main`" as hard gate for Phase 2.4 execute kickoff.

---

### Next Phase

Ready for: **research** phase — `/plugin-research O-Bassoon 2-dsp`

Research focus (Phase 2.4):

1. **Resolve Open Questions #1–#10** — JUCE Synthesiser findFreeVoice override pattern, voice_count snapshot site, soft/tongued shape spec, velocity bias magnitude, TuningEngine API + bit-identity check, applyPendingTuning compose order + O-Lyrica precedent, MPE pitch-bend routing in juce::Synthesiser vs MPESynthesiser, NoiseExciter additive vs muted during onset, getNumActiveVoices availability, Gate 4 protocol DAW/fixture paths.
2. **Pattern-confirm against O-Wind + O-Lyrica + O-Bowed** — voice manager subclass override (cite `Source/*.cpp` exact lines if any precedent exists; many Ouaricon plugins use vanilla `juce::Synthesiser` without subclassing — confirm/document); attack-character morph (no direct Ouaricon precedent for parameterised attack-shape — research only the JUCE primitives); TuningEngine.getFrequency call site (cite O-Wind/O-Lyrica startNote bodies); applyPendingTuning compose at startNote (cite O-Lyrica HarpSynthVoice precedent verbatim).
3. **Verify ARCHITECTURE.md Phase 2.4 spec compatibility** — voice manager + cap + stealing; attack-character morph (soft + tongued + velocity bias); NE + MPE + TuningEngine compose chain. Document any deviations from in-tree state caused by Phase 2.3 architectural pivot (continuous-noise excitation may interact with Exciter onset-window additive composition in unexpected ways — research-phase ear-tests at OQ#8).
4. **Pre-flight: confirm working tree state matches Phase 2.3 verify rev-4** — `ninja O-Bassoon_VST3` clean build, install fresh, Logic-AU plays Phase 2.3 baseline tone with vibrato + breath + ADSR active. Sanity-check before research finalises Phase 2.4 implementation skeletons.
5. **Append RESEARCH.md** at `plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md` (rev-4) with §1 (OQs resolved with JUCE 8.0.4 source-line cites), §2 (Pattern Confirmations from O-Wind / O-Lyrica / TuningEngine module + applyPendingTuning), §3 (Implementation Skeletons — `BassoonSynthesiser.{h,cpp}` + `Exciter.{h,cpp}` rev-4 deltas + `BassoonVoice.{h,cpp}` rev-4 deltas + `PluginProcessor.{h,cpp}` rev-4 deltas + `CMakeLists.txt` rev-4 delta + ARCHITECTURE.md rev-4 backfill template), §4 (Discrepancies — anything that contradicts CONTEXT-rev-4 or in-tree state). 10 static-check grep gates locked at research-phase (regression set: RT-safety, NE drain ordering, expression dispatch site, applyGainRamp form, mode-bank dispatch site, 1/8 scaler, throttle epsilons, DSP-07, auval, pluginval-5; PLUS Phase 2.4-new: BassoonSynthesiser type swap at PluginProcessor.h, voice_count snapshot site, applyPendingTuning call site, exciter.startOnset call site).

After research: plan-phase writes Phase 2.4 task breakdown verbatim against this CONTEXT-rev-4 + research findings; execute-phase performs the implementation (after Phase 2.3 atomic commit lands); verify-phase confirms Gate 4 manual checklist + 60 s sustain + 8-voice CPU + atomic commit.

---

### Audit Trail (rev-4 addendum)

**rev-4 (this addendum, 2026-04-29):** Phase 2.4 opening — Voice Manager + Attack Character + Note Expression Integration (final Stage 2 phase). 8 user-confirmed approach decisions across two AskUserQuestion batches:
- Batch 1: (Q1) single-pass cycle scope (all 4 systems in one cycle); (Q2) startNote-only NE consumption snapshot; (Q3) ear-only A/B for attack-character at v1.0; (Q4) include 60 s gate as Gate 4 item.
- Batch 2: (Q1) voice_count change applies next note-on (ROADMAP locked); (Q2) release-tail-first stealing then oldest-noteOn (JUCE default); (Q3) inline iteration ceiling at rev-3 (Phase 2.2/2.3 precedent); (Q4) atomic commit subject `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`.

Plus 12 derived decisions (BassoonSynthesiser subclass, voice_count snapshot at processBlock prologue, Exciter dual-shape morph + onset-window latch, velocity bias formula 0.3 magnitude starting point, soft shape provenance, tongued shape generation method, NoiseExciter additive during onset, f_base compose chain at startNote, TuningEngine bit-identity at default 12-TET, MPE per-channel routing already wired, NE event handling two-path verification, Gate 4 protocol).

10 open questions handed to research-phase: JUCE findFreeVoice override pattern, voice_count snapshot site, soft/tongued shape spec, velocity bias magnitude, TuningEngine API + bit-identity, applyPendingTuning + O-Lyrica precedent, MPE pitch-bend routing, NoiseExciter onset behaviour, getNumActiveVoices availability, Gate 4 DAW/fixture verification paths.

10 risks documented with mitigations: voice-stealing click, attack-character mid-onset zipper, NE applyPendingTuning thread-safety, TuningEngine API mismatch, MPE per-channel pitch-bend routing, Exciter file rot, voice_count = 1 mono-mode, high-rate retrigger stuck-note, 60 s QUAL-02 regression, Phase 2.3 atomic commit dependency.

**Inherited verbatim from Phase 2.1 + Phase 2.2 + Phase 2.3 (not re-litigated):**
- Per-sample render loop ordering (`excitation → modeBank → adsr → addSample`) — Phase 2.4 ADDS Exciter additive contribution during onset window, preserves pipeline.
- Centered equal L+R per-sample voice write
- Mode-bank coefficient cadence: note-on + pitch-bend (Phase 2.1) + tone > epsilon (Phase 2.2) + |Δf_final| > 0.1 Hz (Phase 2.3); Phase 2.4 changes the *source* of currentFrequencyBase (now NE-tuned) but NOT the dispatch cadence
- NE drain BEFORE renderNextBlock at PluginProcessor.cpp prologue (Phase 2.4 voices NOW consume the drained table for the first time at voice level — drain logic itself unchanged)
- DSP-07 (no O-Reed dependency) verified at Stage 1, regression-grepped at every phase
- Reference WAVs archived at `research/reference-recordings/` (Phase 2.1)
- Atomic-commit gate-first principle
- Primary listening DAW: Logic Pro (AU)
- DAW + tuner verification (no CLI render harness — Logic-AU bounce + Python numpy scripts for QUAL-02)
- Bassoon-tuned partial table + formant Gaussian × 1/k roll-off (Phase 2.2)
- Tone smoother (50 ms Linear) + throttled-epsilon (0.001) dispatch (Phase 2.2)
- 1/8 headroom scaler (Phase 2.2)
- rev-3 strike() at startNote (Phase 2.2 — retained as attack transient)
- Continuous filtered-noise excitation as primary sustain source (Phase 2.3 architectural pivot — preserved)
- Per-voice sine LFO vibrato + multiplicative compose + |Δf_final| > 0.1 Hz throttle (Phase 2.3)
- Breath state machine: ui_breath × cc2_normalised + CC2-takeover 500 ms idle window (Phase 2.3)
- Aggregate setExpression per-voice setter for 6 APVTS reads (Phase 2.3) — Phase 2.4 does NOT add attack_character to setExpression (snapshot at startNote only)
- Per-voice 20 ms breath smoother (sample-rate getNextValue)
- Processor-level 30 ms output_gain smoother (block-rate applyGainRamp)
- Throttled-epsilon dispatch pattern + dispatch ordering: tone → expression → NE drain → renderNextBlock → output_gain applyGainRamp (Phase 2.4 ADDS voice_count snapshot at the head of this chain)

**New in rev-4:**
- Cycle scope = Phase 2.4 only (4 systems: voice manager + cap + stealing, attack-character morph, NE per-voice consumption + MPE pitch-bend per-channel, TuningEngine getFrequency() per-voice). Plus QUAL-02 60 s revisit.
- `Source/BassoonSynthesiser.{h,cpp}` (NEW) — subclass juce::Synthesiser with findFreeVoice override; activeVoiceCap snapshot; setNoteStealingEnabled(true); rely on JUCE default findVoiceToSteal (release-tail-first then oldest-noteOn).
- voice_count APVTS read at processBlock prologue snapshot (integer comparison throttle); applies on next note-on per ROADMAP.
- Exciter dual-shape morph (`softShape` + `tonguedShape` arrays) crossfaded via `attack_character` with velocity bias `effective = clamp(attackChar + (vel - 0.5) * 0.3, 0, 1)`. Snapshot at note-on (onset-window latch).
- Phase 2.1 Exciter re-engaged in renderNextBlock per-sample loop: `excitation += exciter.getNextSample()` additive composition with Phase 2.3 NoiseExciter.
- f_base compose chain at startNote: `tuningEngine->getFrequency(midiNote) → applyPendingTuning(table, midiNote, f_base) → currentFrequencyBase`. Replaces Phase 2.1-2.3 plain `MidiMessage::getMidiNoteInHertz` call.
- TuningEngine wired headless at v1.0 (12-TET A=440 default, no UI exposure — deferred v1.1 per Stage 0 D6).
- MPE per-channel pitch-bend verified (no new code — JUCE 8 Synthesiser handles routing automatically in MPE-enabled DAW).
- VST3 NE event handling verified via DAW with NE support OR synthetic test fixture (Phase 2.4 closes structural half; Stage 4 closes Dorico parity).
- Gate 4 PASS bar: 10-item user-checkable (1 polyphony + 1 voice cap + 1 retrigger + 3 attack-character + 1 MPE + 1 NE + 1 QUAL-02 60s + 1 PERF-02 8-voice CPU) + automated invariant battery.
- Inline iteration ceiling at rev-3 (Phase 2.2/2.3 precedent).
- Atomic commit on Gate 4 PASS with subject `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`.
- ARCHITECTURE.md rev-4 backfill (BassoonSynthesiser semantics + Exciter dual-shape morph + f_base compose chain + NoiseExciter additive-during-onset) is a Phase 2.4 atomic-commit deliverable.
- Phase 2.3 atomic commit MUST land on `main` BEFORE Phase 2.4 execute-phase begins (process invariant — PLAN-rev-4 task #1 hard gate).

**Closes Stage 2:** After Phase 2.4 verify-phase atomic commit, Stage 2 transitions to ✅ COMPLETE. Remaining Stage 4 work: pluginval --strictness 10, Windows VST3 build, Dorico Playback Template parity test, factory presets, CHANGELOG.md. Stage 3 (GUI) blocks on UI mockup pass (parallel-eligible track).
