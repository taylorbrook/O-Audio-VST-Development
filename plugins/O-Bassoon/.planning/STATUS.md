---
plugin: O-Bassoon
stage: 2
status: in_progress
phase: verify_complete
last_updated: 2026-04-27
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: phase_2_2_atomic_commit
next_stage: 2
phase_cycle: "Phase 2.2 — Bassoon Spectral Tuning + Tone Control (verify complete, Gate 2 PASS, rev-3 strike() patch absorbed within ceiling, atomic commit pending user trigger; next: Phase 2.3 expression)"
contract_checksums:
  brief: sha256:4989e5389e14e7bf29ae16b3923e9a70438fd3b0b0e0a6405be9f6983763265f
  parameter_spec: sha256:708ecb2bf2a49fcc5b8f4a8b745859d652edc81aff53ef55071b69ca62b6b875
  architecture: sha256:d54a0e95fcd63e9a21554ec77024d78c52b11a88119074022a910e1a42bad641
  roadmap: sha256:2e69806dfcecbd80acccc1d42d2eb9dfe26945cd2be8d615dc7f64ae2978be2e
---

# O-Bassoon Status

## Current Position

Stage: 2 of 4 (DSP) — **Phase 2.2 / ✅ VERIFY COMPLETE (Gate 2 10/10 PASS, rev-3 strike() patch absorbed within ceiling, atomic commit pending)**
Status: Phase 2.2 verify-phase complete. Automated subset (12-item invariant battery + auval AU VALIDATION SUCCEEDED + pluginval-5 SUCCESS) PASS. Manual Gate 2 (10 items) reported PASS by user via Standalone + Logic-AU 2026-04-27 — bassoon-like timbre at C3, visible 400-600 Hz spectrum peak, audible woody↔bright tone-slider character change, no zipper/clicks/NaN on tone sweep, 8-voice CPU < 20%, 1-voice CPU < 5%, C1-C6 sweep clean (expected C5+ Nyquist thinning), ≥10s long-tone stable. Required rev-3 in-cycle iteration: added `ModeBank::strike()` invoked from `BassoonVoice::startNote` to inject modal state (`y1 = amp·sinθ, y2 = 0` per non-muted mode) that launches the canonical `y[n] = amp·sin((n+1)θ)·R^n` free-decay sinusoid. Without `strike()`, the pole-only resonator's IR peak `b0/sin(θ) ≈ (1-R)·amp/sin(θ)` was ~50 dB below `amp` for high-Q low-frequency modes — sustain inaudible. Diagnostic process also revealed Phase 2.1's Gate 1 PASS was recorded based on incorrect manual testing (brief click rather than press-and-hold); the underlying audibility defect was masked. Rev-3 patch retroactively corrects both Phase 2.1 and Phase 2.2 audible behavior. RT-safety preserved (allocation-free strike loop with cached sinTheta). Verdict ✅ VERIFIED. REQUIREMENTS.md updates: FUNC-01/DSP-01/DSP-03/QUAL-01 promoted partial → complete; FUNC-04/PERF-02/QUAL-02 unchanged at partial (Phase 2.3 / Phase 2.4 deliverables); COMPAT-01 unchanged at partial (Stage 4); FUNC-03/DSP-07/PERF-01 carry-forward complete. Atomic commit `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS` PENDING explicit user trigger per CLAUDE.md commit protocol. Iteration ceiling rev-3 burned but goal achieved within ceiling — next phase (Phase 2.3) starts fresh.
Progress: [##################..] 90%

## Completed So Far

**Ideation:** Complete (2026-04-27)
- Core concept: simple modal-synthesis bassoon for sustained microtonal long tones
- DSP approach: modal synthesis (no O-Reed dependency, no waveguide, no reed self-oscillation)
- Range: C1-C6 extended, polyphonic 1-16 voices (default 8)
- Expression: vibrato, breath/dynamics (CC2 + velocity), tone/brightness, attack character
- Microtonal: VST3 Note Expression + MPE pitch-bend (Ouaricon family pattern)
- Requirements extracted with acceptance criteria

**Stage 0:** Research & Planning complete (2026-04-27)
- Architecture documented at `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md`
- ROADMAP documented at `plugins/O-Bassoon/.planning/ROADMAP.md`
- Discuss-phase findings at `plugins/O-Bassoon/.planning/stages/0-ideation/CONTEXT.md`
- Complexity tier: 3 (MODERATE research depth)
- Complexity score: 9.0 raw → **5.0 capped**
- Strategy: **Phased implementation** (4 DSP phases + 2 GUI phases)
- Highest-risk component: bassoon partial-table spectral tuning (Phase 2.2)
- Reuses shared modules: `note-expression` v1.1.0, `scala-tuning-engine` v2.1.0 (headless wired)

## Next Steps

1. **Phase 2.2 atomic commit** — pending user "commit it"/"land it"/"ship it" trigger. Locked subject: `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`. Single commit lands rev-2 sources + rev-3 strike() patch + ARCHITECTURE rev-note + planning artefacts (CONTEXT/RESEARCH/PLAN/SUMMARY/VERIFICATION rev-2) on `main`.
2. **Phase 2.3 cycle** — `/clear` then `/plugin-discuss O-Bassoon 2-dsp`. Scope: vibrato + breath/dynamics (CC2 + velocity) + attack-character morph + ADSR APVTS wiring (attack_time, release_time) + output_gain. Phase 2.3 introduces continuous breath excitation (CC2 → modeBank input) that converts current struck-modal architecture into true sustained-tone behavior. Phase 2.3 verifies: FUNC-04 complete (ADSR param 0-2000/0-3000 ms ranges), DSP-02/04/05 complete (vibrato + breath + attack-character), QUAL-02 complete (60s stability), PERF-02 final.
3. UI mockup pass (parallel-eligible with Stage 2) — required to unblock Stage 3
4. Remaining Stage 2 phases after 2.3: 2.4 (Polyphony cap + voice stealing + NE/MPE per-voice consumption + TuningEngine call)

## Context to Preserve

**Key Decisions (full rationale in CONTEXT.md):**
- D1: Modal synthesis (parallel biquad bank, 16 modes/voice)
- D2: Fixed 16 modes (frequency-adaptive deferred to v1.1)
- D3: `juce::Synthesiser` (not `MPESynthesiser`)
- D4: Aftertouch → vibrato deferred to v1.1
- D5: Reuse `note-expression` module v1.1.0 (no inline copy)
- D6: TuningEngine wired headless at v1.0 (12-TET default, no UI)
- D7: 4-phase DSP staging
- D8: 2-phase GUI (no Phase 3.3 needed)

**Implementation Strategy:**
- Phased / staged
- Stage 1: Foundation (single pass)
- Stage 2: DSP (4 phases — Core / Tuning / Expression / Polyphony+NE)
- Stage 3: GUI (2 phases — blocks on UI mockup)
- Stage 4: Validation (single pass)

**Files Created (Stage 0):**
- `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md`
- `plugins/O-Bassoon/.planning/ROADMAP.md`
- `plugins/O-Bassoon/.planning/stages/0-ideation/CONTEXT.md`
- `plugins/O-Bassoon/.planning/STATUS.md` (this file — updated)

**Files Created (Stage 1 discuss):**
- `plugins/O-Bassoon/.planning/stages/1-foundation/CONTEXT.md`

**Files Created (Stage 1 research):**
- `plugins/O-Bassoon/.planning/stages/1-foundation/RESEARCH.md` — confirms NE + TuningEngine APIs; surfaces 3 discrepancies for plan phase to absorb (D1: no `Ouaricon::note_expression` CMake target; D2: `TuningEngine` is in **global** namespace, not `Ouaricon::TuningEngine`; D3: `NEEDS_WEBVIEW2 TRUE` must also be in `juce_add_plugin`); reserves `PLUGIN_CODE OBsn` (OBas is taken by O-Bass).

**Files Created (Stage 1 plan):**
- `plugins/O-Bassoon/.planning/stages/1-foundation/PLAN.md` — 9-task single-wave execution plan; absorbs D1/D2/D3; commits `PLUGIN_CODE OBsn`; specifies CMakeLists + 5 source files (BassoonSound.h, BassoonVoice.{h,cpp}, PluginProcessor.{h,cpp}, PluginEditor.{h,cpp}); mirrors O-Wind (NOT O-Lyrica) for scala-tuning-engine wiring; success criteria includes pluginval --strictness 5 pass and DSP-07 grep verification.

**Files Created (Stage 1 execute — foundation-shell):**
- `plugins/O-Bassoon/CMakeLists.txt` — `juce_add_plugin OBsn`, scala-tuning-engine direct sources, `ouaricon_add_module(... note-expression)`, NEEDS_WEBVIEW2 TRUE, no binary-data block
- `plugins/O-Bassoon/Source/BassoonSound.h` — header-only catch-all `juce::SynthesiserSound`
- `plugins/O-Bassoon/Source/BassoonVoice.{h,cpp}` — silent-stub voice with three setter wires (APVTS, `TuningEngine*`, `PendingTuningTable*`)
- `plugins/O-Bassoon/Source/PluginProcessor.{h,cpp}` — `OBassoonAudioProcessor` with APVTS (10 params), `juce::Synthesiser` (16 voices), headless `TuningEngine`, `Ouaricon::NoteExpression::VST3Extensions`, NE drain BEFORE renderNextBlock
- `plugins/O-Bassoon/Source/PluginEditor.{h,cpp}` — `juce::GenericAudioProcessorEditor` placeholder, 500x480
- `plugins/O-Bassoon/.planning/stages/1-foundation/SUMMARY.md` — execution summary

**Stage 1 build pass:**
- `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone`: SUCCESS
- `auval -v aumu OBsn OuDv`: AU VALIDATION SUCCEEDED
- `pluginval --strictness-level 5 ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3`: SUCCESS (exit 0)
- VST3 + AU installed to system plugin folders after AU cache clear
- 0-ideation → 1-foundation gate bypassed via `--force` (initial foundation; build check N/A at ideation)

**Stage 1 verify pass (2026-04-27):**
- VERIFICATION.md written at `plugins/O-Bassoon/.planning/stages/1-foundation/VERIFICATION.md`
- All 8 stage goals achieved (1-7 ✅; 8 ⚠️ partial — DSP-07 ✅, COMPAT-01 strictness-5 ✅, full strictness-10 + Windows is Stage 4)
- 14/14 automated checks pass on re-run
- Requirements updated: DSP-07 → complete, COMPAT-01 → partial (final gate at Stage 4)
- Verdict: ✅ VERIFIED — ready for Stage 2

**Files Created (Ideation, pre-Stage-0):**
- `plugins/O-Bassoon/.planning/BRIEF.md`
- `plugins/O-Bassoon/.planning/REQUIREMENTS.md`
- `plugins/O-Bassoon/.planning/parameter-spec-draft.md`

**Files Created (Stage 2 / Phase 2.1 discuss — 2026-04-27):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md` (rev-1) — 8 user-confirmed approach decisions (cycle scope = Phase 2.1 only, strict-ROADMAP minimal wiring, on-note-on/pitch-bend coefficient cadence, reference recording sourced during Phase 2.1, single atomic commit on Gate 1 PASS, DAW + tuner only, 10-item Gate 1 bar, centered equal L+R per-sample voice write); 10 open questions handed to research-phase; 8 risks documented with mitigations.

**Files Created (Stage 2 / Phase 2.1 research — 2026-04-27):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md` (rev-1) — all 10 OQs resolved with JUCE 8.0.4 source-line cites; pole-only biquad specialised over O-Formant lift (saves 16 bytes + ~25% mul/sample); reference-recording locked to VSCO-2-CE CC0 (PSBassoon_C3_v1_1.wav, direct GitHub raw URL); spectrum-baseline tool locked to Voxengo SPAN; Logic CPU protocol locked to System Performance Meter / Process bar; 6 discrepancies surfaced (D1: prepareToPlay is custom not virtual; D2: 1/N output scaling Phase 2.1 placeholder; D3: confirmed PluginProcessor::processBlock buffer.clear() correct; D4: VSCO C3 octave-convention check at audition; D5: Risk #2 severity downgraded; D6: -24dB peak expected at Phase 2.1).

**Files Created (Stage 2 / Phase 2.1 plan — 2026-04-27):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md` (rev-1) — 9-task single-Wave plan (5 file edits + 1 CMakeLists edit + 1 reference-download + 1 build/verify + 1 atomic commit), 16 file operations total, lifts §3 RESEARCH skeletons verbatim, 10-item Gate 1 PASS bar pinned, pre-commit grep + invariant checks listed, 8 risks carried with mitigations, out-of-scope register matches ROADMAP Phase 2.1 boundary.

**Files Created / Modified (Stage 2 / Phase 2.1 execute — 2026-04-27):**
- `plugins/O-Bassoon/Source/ModeBank.{h,cpp}` (NEW) — 16-mode pole-only resonator bank (Direct-Form I + isfinite guard, integer-harmonic placeholder partials, 1/N headroom scaling)
- `plugins/O-Bassoon/Source/Exciter.{h,cpp}` (NEW) — 5 ms half-sine × exp impulse, peak-normalised, in-class `std::array<float, 1024>` storage (no allocation at runtime)
- `plugins/O-Bassoon/Source/BassoonVoice.{h,cpp}` (MOD) — silent-stub replaced; per-sample render loop (`exciter → modeBank → adsr → addSample`), pitch-bend ±2 semi (raw 14-bit), full state-reset on voice exit, NO APVTS reads / NO TuningEngine call
- `plugins/O-Bassoon/Source/PluginProcessor.cpp` (MOD, prepareToPlay only) — per-voice prepareToPlay dispatch loop added; processBlock / NE drain ordering / param layout / bus contract untouched
- `plugins/O-Bassoon/CMakeLists.txt` (MOD, target_sources only) — ModeBank + Exciter sources added
- `plugins/O-Bassoon/research/reference-recordings/{bassoon-c3-sustain-v1.wav, bassoon-c3-sustain-v2.wav}` (NEW) — VSCO-2-CE bassoon C3 sustain (CC0), 16-bit / 44.1 kHz stereo, ~8.7 s
- `plugins/O-Bassoon/research/reference-recordings/LICENSE.md` (NEW) — VSCO-2-CE provenance + CC0 dedication
- `plugins/O-Bassoon/research/reference-recordings/README.md` (NEW) — source / octave-convention caveat (D4) / audition checklist / SPAN baseline procedure
- `plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md` (NEW) — execution summary with Gate 1 status table

**Files Created (Stage 2 / Phase 2.1 verify — 2026-04-27):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md` (NEW) — goal-backward analysis, 16/16 automated invariants PASS, Gate 1 status (2 auto PASS, 8 manual PENDING), requirements verification, manual checklist, ⚠️ PARTIAL verdict pending manual subset

**Stage 2 / Phase 2.1 build + auto-verified Gate 1 subset (re-verified at verify-phase 2026-04-27):**
- `ninja O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone`: SUCCESS (incremental — `ninja: no work to do.`)
- AU cache cleared, VST3 + AU + Standalone installed fresh (re-installed at verify-phase)
- `auval -v aumu OBsn OuDv`: AU VALIDATION SUCCEEDED (Gate 1 item 7 ✅)
- `/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3`: exit 0, SUCCESS (Gate 1 item 8 ✅, output bus confirmed 0 in / 2 out)
- 16/16 automated invariants PASS (RT-safety grep zero, locked-Q2 grep zero, Stage 1 invariants preserved, CMakeLists flags + `juce_generate_juce_header` order intact, NE drain ordering intact at PluginProcessor.cpp:178 → :182)
- Gate 1 manual subset (items 1–6, 9): ✅ **PASS** (user-confirmed Logic-AU verification 2026-04-27 — pitch ±2c on C3, no clicks, no NaN, >10s sustain stable, 1-voice CPU <5%, C1-C6 sweep clean, AU smoke PASS)
- Gate 1 item 10 (SPAN baseline PNG): 🚫 **DROPPED** from gate per user authority 2026-04-27 — SPAN not installed; Phase 2.2 A/B will use ear + archived reference WAV instead. Non-blocking deviation.
- **Final Gate 1 score: 9/9 PASS**
- Atomic commit landed at `d1b3370` on `main` (`feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS`)

**Files Updated (Stage 2 / Phase 2.2 discuss — 2026-04-27):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md` (rev-2 addendum) — 9 user-confirmed defaults locking Phase 2.2 cycle scope (single-cycle: partial-table replacement + formant-Gaussian amplitude shaping + `tone` APVTS wiring + A/B-vs-reference listening), strict-ROADMAP `tone`-only wiring at processor level, processor-level `SmoothedValue<float, Linear>` 50 ms ramp + throttled-epsilon dispatch (ε = 0.001) at processor dispatch site, Gate 2 bar = ear-only A/B + Logic Channel EQ Analyzer overlay (peak in 400-600 Hz at held C3), inline iteration with rev-3 ceiling, v1 WAV canonical / v2 secondary, 8-voice CPU early signal (hold 8 keys in Logic-AU, < 20 % bar), atomic commit pattern (`feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`). 10 open questions handed to research-phase (SmoothedValue block-rate idiom, lazy-vs-explicit `setTone` recompute, partial-ratio source verification, formant-Gaussian peak normalisation across f0, `1/N` scaler retention/relaxation, Logic EQ Analyzer protocol, tone descriptor verification, reference WAV pitch audition (D4 carry-forward), 8-voice CPU protocol, ARCHITECTURE.md backfill format). 8 risks documented with mitigations.

**Files Updated (Stage 2 / Phase 2.2 research — 2026-04-27):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md` (rev-2 addendum) — all 10 OQs resolved with JUCE 8.0.4 source-line cites + family precedent (O-Bass `PluginProcessor.cpp:230` for `skip(numSamples)` block-rate dispatch, O-Contrabass `WaveguideString.cpp:275` for `jmax(0, numSamples)` defensive idiom). Key locks: explicit `applyToneChange()` over lazy recompute (preserves tone-slider responsiveness on held notes; cached `cosTheta` + `amp` per-mode); `1/N` scaler relaxed from `1/16` to `1/8` (+6 dB lift; quantitative justification — Phase 2.2 amp-sum at C3 ≈ 1.79 vs. Phase 2.1's 16, projecting -43 dBFS without relax → -37 dBFS with `1/8`, polyphony + C5 clip-safe); per-note loudness normalisation deferred to v1.1+ (accepts ~18 dB natural pitch-induced variation); ARCHITECTURE partial-ratio table acknowledged as author-curated synthesis (no single primary source); Logic Channel EQ Analyzer Pre-EQ-mode protocol documented; 8-note chord (C3-Bb4 spread) for 8-voice CPU early-signal; ARCHITECTURE.md backfill template (append-rev-note default, as-shipped subsection iteration-case). 6 discrepancies surfaced (D1-D6 rev-2); none block planning. Implementation skeletons for ModeBank rev-2 + BassoonVoice rev-2 single addition + PluginProcessor rev-2 smoother+dispatch+ordering ready for plan-phase verbatim consumption.

**Files Updated (Stage 2 / Phase 2.2 plan — 2026-04-27):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md` (rev-2 addendum) — 9-task single-Wave plan appended to existing rev-1 (Phase 2.1) plan. Tasks: (1) ModeBank.h rev-2 surface, (2) ModeBank.cpp rev-2 implementation, (3) BassoonVoice.{h,cpp} setTone forwarder, (4) PluginProcessor.h toneSmoother members, (5) PluginProcessor.cpp prepareToPlay reset + processBlock dispatch BEFORE NE drain, (6) ARCHITECTURE.md as-shipped rev-note (append default), (7) build + install + 8 static-check grep gates (RT-safety, NE drain ordering, mode-index, scaler, throttle epsilon, DSP-07, AU validation, pluginval-5), (8) manual Gate 2 verification (10-item checklist: pitch audition, A/B listen, spectrum overlay, tone sweep, descriptors, 8-voice CPU, 1-voice CPU, C1-C6 sweep, ≥10s sustain, VERIFICATION write), (9) atomic commit. Lifts RESEARCH-rev-2 §3 implementation skeletons verbatim. Atomic commit subject locked: `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`. 0 CMakeLists edits. 8 risks carried with mitigations. Inline iteration ceiling at rev-3 (CONTEXT-rev-2 Q6-rev-2).

**REQUIREMENTS.md updates (verify-phase 2026-04-27):**
- PERF-01 (Real-time safe): pending → **complete** (RT-safety grep + pluginval-5 fuzz/state PASS)
- FUNC-03 (Range C1-C6): pending → **complete** (audible C1-C6 sweep PASS, all notes track pitch, modal bank reconfigures cleanly)
- QUAL-02 (>60s long-tone stability): pending → **partial** (>10s subset PASS; full 60s = Phase 2.3)
- DSP-07: complete (carried from Stage 1, unchanged)
- COMPAT-01: partial (strictness-5 PASS this phase; full strictness-10 + Windows is Stage 4)
- FUNC-01 / FUNC-04 / DSP-01 / QUAL-01: **partial** (structural halves verified audibly; spectral / expression / parameter-sweep deliverables pending Phase 2.2-2.3)
- FUNC-02 / FUNC-05 / DSP-02-06 / PERF-02: pending — deferred to Phase 2.2-2.4 per ROADMAP
