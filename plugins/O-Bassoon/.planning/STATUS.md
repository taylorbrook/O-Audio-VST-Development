---
plugin: O-Bassoon
stage: 2
status: in_progress
phase: plan_complete
last_updated: 2026-04-29
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: phase_2_3_atomic_commit_then_phase_2_4_execute_phase
next_stage: 2
phase_cycle: "Phase 2.4 — Voice Manager + Attack Character + Note Expression Integration (research complete 2026-04-29; RESEARCH-rev-4 addendum landed at .planning/stages/2-dsp/RESEARCH.md). All 10 OQs resolved with JUCE 8.0.4 source-line citations (juce_Synthesiser.h:336/339/381/577/600-612, juce_Synthesiser.cpp:403-410/509-523/525-594; juce_AudioProcessorValueTreeState.h:340-346) and module citations (TuningEngine.cpp:714-728/770-775; NoteExpression.h:66-79; O-Lyrica HarpSynthVoice.cpp:113-147). Lock decisions — (OQ#1) BassoonSynthesiser subclass overrides findFreeVoice only with manual active-voice loop via getNumVoices+getVoice+isVoiceActive (JUCE 8 has no getNumActiveVoices), delegates to base findVoiceToSteal for release-tail-first stealing; (OQ#2) voice_count snapshot at processBlock prologue HEAD before tone-dispatch with integer-comparison throttle; (OQ#3) keep Phase 2.1 5ms half-sine×exp as softShape (rename onsetBuffer→softShape), generate NEW tonguedShape as 7.5ms exp-decay×white-noise with juce::Random deterministic seed 12345, peak-normalised; (OQ#4) velocity bias magnitude 0.3 (CONTEXT default — between O-Lyrica brightness ±12.5% and O-Wind settle ±33% precedent); (OQ#5) TuningEngine::getFrequency global namespace, double, bit-identical to MidiMessage::getMidiNoteInHertz at default 12-TET A=440 (octaveStretch=1.0f cast preserves value); (OQ#6) compose chain `f_double = tuningEngine->getFrequency → applyPendingTuning → currentFrequencyBase = static_cast<float>(f_double)` mirrors O-Lyrica HarpSynthVoice:113-147 verbatim minus humanize; (OQ#7) MPE per-channel pitch-bend routes automatically via juce::Synthesiser::handlePitchWheel iterating voices and calling pitchWheelMoved only when voice->isPlayingChannel(midiChannel) — Stage 0 D3 (juce::Synthesiser not MPESynthesiser) lock confirmed correct; (OQ#8) NoiseExciter additive composition `excitation = noiseExciter.getNextSample(breath) + exciter.getNextSample()` during onset, exciter auto-zeros after onset window; (OQ#9) manual active-voice count loop allocation-free RT-safe; (OQ#10) Gate 4 NE verification via synthetic test fixture (temporary debug TextButton writing pendingTable[60].store(+50cents), removed pre-commit), MPE via Bitwig+MPE-controller OR Stage 4 deferred. 7 rev-4 discrepancies registered (D1-empty TU pair convention, D2-pad-zero benign, D3-onsetBuffer rename grep clean, D4-double→float cast <16µHz error, D5-mono mode standard JUCE behaviour, D6-setNoteStealingEnabled redundant explicit, D7-effectiveAttackChar default safe). 16-item static-check grep battery locked for verify (1-RT-safety; 2-NE-drain ordering; 3-BassoonSynthesiser type swap; 4-setActiveVoiceCap snapshot site; 5-applyPendingTuning call; 6-tuningEngine->getFrequency call; 7-exciter.startOnset call; 8-additive composition site; 9-DSP-07 regression; 10-1/8 scaler retention; 11-0.001f epsilon hits ≥10; 12-setExpression single site; 13-applyGainRamp single site after renderNextBlock; 14-modeBank.setFundamental 3 sites; 15-auval; 16-pluginval-5). Implementation skeletons cover BassoonSynthesiser.{h,cpp} NEW (~30-line header-only override + optional .cpp pair for Phase 2.3 NoiseExciter/Vibrato consistency), Exciter.{h,cpp} MOD (rename onsetBuffer→softShape, ADD tonguedShape array + startOnset velocity-bias snapshot + getNextSample juce::jmap morph), BassoonVoice.cpp MOD (replace line 59 with 9-line compose chain, replace line 64 with 3-line exciter.startOnset, modify renderNextBlock per-sample loop with 4-line additive excitation), PluginProcessor.{h,cpp} MOD (BassoonSynthesiser type swap + lastDispatchedVoiceCount=-1 member + 7-line voice_count snapshot at prologue head), CMakeLists.txt MOD (target_sources +1 or +2 entries), ARCHITECTURE.md rev-4 backfill template (4-paragraph as-shipped note). 8 user-confirmed approach decisions inherited verbatim from CONTEXT-rev-4 across 2 AskUserQuestion batches — Batch 1 — (Q1) single-pass cycle scope (all 4 systems in one cycle), (Q2) startNote-only NE consumption snapshot, (Q3) ear-only A/B for attack-character at v1.0, (Q4) include 60s gate as Gate 4 item; Batch 2 — (Q1) voice_count change applies next note-on per ROADMAP, (Q2) release-tail-first stealing then oldest-noteOn (JUCE default findVoiceToSteal), (Q3) inline iteration ceiling rev-3 (Phase 2.2/2.3 precedent), (Q4) atomic commit subject `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`. Plus 12 derived decisions (BassoonSynthesiser subclass, voice_count snapshot at processBlock prologue, Exciter dual-shape morph + onset-window latch, velocity bias 0.3 starting magnitude, soft/tongued shape provenance, NoiseExciter additive during onset, f_base compose at startNote = TuningEngine→applyPendingTuning→currentFrequencyBase, MPE per-channel routing already wired at Phase 2.1, NE event handling two-path verification, Gate 4 protocol). 10 open questions handed to research-phase. 10 risks documented with mitigations. Cycle scope: Phase 2.4 closes FUNC-02 (polyphony 1-16 cap), FUNC-05 (voice stealing), DSP-05 (attack-character morph re-engages retained Phase 2.1 Exciter member), DSP-06 (VST3 NE per-voice consumption + MPE pitch-bend per-channel + TuningEngine getFrequency()), and revisits QUAL-02 60s gate (Phase 2.3 skipped per user authority). PERF-02 8-voice <25% final measurement under enforced cap. Process invariant locked: Phase 2.3 atomic commit MUST land on `main` BEFORE Phase 2.4 execute-phase begins (PLAN-rev-4 task #1 hard gate). Phase 2.3 commit is PENDING explicit user trigger as of CONTEXT-rev-4 write."
contract_checksums:
  brief: sha256:4989e5389e14e7bf29ae16b3923e9a70438fd3b0b0e0a6405be9f6983763265f
  parameter_spec: sha256:708ecb2bf2a49fcc5b8f4a8b745859d652edc81aff53ef55071b69ca62b6b875
  architecture: sha256:d54a0e95fcd63e9a21554ec77024d78c52b11a88119074022a910e1a42bad641
  roadmap: sha256:2e69806dfcecbd80acccc1d42d2eb9dfe26945cd2be8d615dc7f64ae2978be2e
---

# O-Bassoon Status

## Current Position

Stage: 2 of 4 (DSP) — **Phase 2.4 / 🟡 RESEARCH COMPLETE (RESEARCH-rev-4 addendum landed; all 10 OQs resolved with JUCE 8.0.4 source-line citations + O-Lyrica/O-Wind precedent + module API confirmations; 16-item static-check grep battery locked; implementation skeletons cover BassoonSynthesiser NEW + Exciter MOD + BassoonVoice MOD + PluginProcessor MOD + CMakeLists MOD + ARCHITECTURE rev-4 backfill template; 7 rev-4 discrepancies registered with resolutions; Phase 2.3 atomic commit STILL PENDING user trigger — Phase 2.4 execute blocks on `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS` landing on `main`)**
Status: Phase 2.3 execute-phase complete 2026-04-28. PLAN-rev-3 Tasks 1-9 executed inline by orchestrator (lifted RESEARCH-rev-3 §3 skeletons verbatim): NEW Source/Vibrato.{h,cpp} (per-voice sine LFO + onset envelope, random phase per startNote O-Wind precedent), NEW Source/NoiseExciter.{h,cpp} (per-voice 1-pole LP @ 2 kHz over white noise, BASE_NOISE_GAIN=0.05f, voiceIndex×31337 seed O-Bowed precedent); MODIFY BassoonVoice.{h,cpp} (Vibrato + NoiseExciter + breathSmoother members; setExpression aggregate setter; CC2 takeover state machine with 500 ms idle window; vibrato compose `f_final = base × pow(2, c/1200) × pow(2, pb/12)` with |Δf| > 0.1 Hz throttle; per-sample render pivot to continuous-noise excitation; Phase 2.1 Exciter call dropped from render path with member retained per D6-rev-3); MODIFY PluginProcessor.{h,cpp} (outputGainSmoother + 6 dispatch shadows; constructor setVoiceIndex wire; expression dispatch BEFORE NE drain with 0.001f epsilon throttle; output_gain applyGainRamp(0, numSamples, current, smoother.skip(N)) AFTER renderNextBlock); MODIFY CMakeLists.txt target_sources +4 entries with D6-rev-3 retention comments; APPEND ARCHITECTURE.md Phase 2.3 as-shipped rev-3 note. Build clean (12/12 targets, zero warnings on Phase 2.3 sources, no -Wunused-private-field on retained Exciter member). VST3 + AU installed fresh (AU cache cleared). 10/10 auto static-check grep gates PASS: RT-safety zero render-path matches; ordering invariant tone→expression→NE-drain→render→output_gain confirmed at PluginProcessor.cpp lines 203/236/249/252/262; setExpression dispatch ONE site at line 236; applyGainRamp ONE site at line 262 AFTER renderNextBlock; modeBank.setFundamental 3 sites in BassoonVoice.cpp (startNote/pitchWheelMoved/renderNextBlock); Phase 2.2 1/8 scaler retained at ModeBank.cpp:114, 1/16 zero matches; 0.001f epsilon 10 hits in PluginProcessor.cpp + EPS in BassoonVoice.cpp:148; DSP-07 zero matches; auval AU VALIDATION SUCCEEDED; pluginval --strictness 5 exit 0 (0 in / 2 out bus confirmed). 1 plan typo resolved at execute-phase (D-exec-1: ModeBank::setFundamental(float) single-arg matches Phase 2.1 API contract; PLAN/RESEARCH skeleton's two-arg call corrected). Tasks 10-11 PENDING USER: Logic-AU 10-item Gate 3 manual checklist (3 ADSR + 1 breath + 1 CC2 + 3 vibrato + 1 output_gain + 1 60s long-tone QUAL-02 with numpy.isfinite + RMS drift + CPU drift + 1 8-voice CPU); bounce phase-2.3-60s-c3-vibrato-breath.wav; write VERIFICATION-rev-3.md; atomic commit. Atomic commit subject locked: `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`. Inline iteration ceiling at rev-3. Atomic commit subject locked: `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`. Inline iteration ceiling at rev-3 (Phase 2.2 precedent). Phase 2.3 discuss-phase + research-phase artefacts (CONTEXT-rev-3 + RESEARCH-rev-3) carry forward unchanged. Phase 2.3 cycle scope locked: 4 APVTS-driven systems — `juce::ADSR` wiring to `attack_time` (0-2000 ms) + `release_time` (0-3000 ms); breath/dynamics with `breath_voice = ui_breath × cc2_normalised` multiplicative composition + CC2-takeover state machine (velocity → initial UI breath, 500 ms idle window); per-voice sine-LFO vibrato (rate 0-10 Hz, depth 0-100 cents, onset 0-2000 ms) with multiplicative pitch-bend compose + block-rate recompute + |Δf_final| > 0.1 Hz throttle; post-summation `output_gain` (-24..+6 dB) with 30 ms `applyGainRamp`. Architectural pivot: continuous filtered-noise excitation source (`NoiseExciter`: per-voice `juce::Random` + 1-pole LP @ 2 kHz + `BASE_NOISE_GAIN = 0.05f` + breath-scaled) replaces struck-modal-only sustain mechanism — voice maintains amplitude indefinitely while held + breath > 0; rev-3 `strike()` retained at `startNote` for attack transient; Phase 2.1 impulse `Exciter` dropped from voice render path (file retained for Phase 2.4 attack-character morph re-introduction). 8 user-confirmed approach decisions across two AskUserQuestion batches; 12 derived decisions; 10 open questions handed to research-phase; 9 risks documented with mitigations. Gate 3 PASS bar: 10-item (3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone-60s + 1 polyphony-CPU). Iteration ceiling at rev-3 (Phase 2.2 precedent), inline at verify-phase. Atomic commit subject locked: `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`. Phase 2.2 atomic commit `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS` landed at `baac74f` on `main` 2026-04-28 (14 files, 2149 insertions, 45 deletions).
Progress: [####################] 94%

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

1. **Phase 2.3 atomic commit** — explicit user trigger ("commit it" / "land it" / "ship it") lands `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS` on `main`. Per CLAUDE.md commit protocol, the orchestrator does NOT auto-commit. Process invariant: Phase 2.3 commit MUST land BEFORE Phase 2.4 execute-phase begins (PLAN-rev-4 task #1 hard gate). Phase 2.4 discuss/research/plan can proceed in parallel against in-tree state. Locked subject + scope: rev-3 sources (Vibrato + NoiseExciter NEW; BassoonVoice + PluginProcessor MOD) + rev-4 fixes (Vibrato lifecycle decouple; BassoonVoice per-sample vibrato + cachedVibratoMult throttle; NoiseExciter BASE_NOISE_GAIN ear-tune) + planning artefacts (CONTEXT-rev-3 + RESEARCH-rev-3 + PLAN-rev-3 + SUMMARY rev-3 + VERIFICATION rev-3+rev-4 + REQUIREMENTS.md v1.0.4 + STATUS.md + ARCHITECTURE.md rev-3 note + CMakeLists target_sources +4).
2. **Phase 2.4 research** — `/clear` then `/plugin-research O-Bassoon 2-dsp`. Resolves 10 OQs handed from CONTEXT-rev-4 (JUCE findFreeVoice override pattern, voice_count snapshot site, soft/tongued shape spec, velocity bias magnitude, TuningEngine API + bit-identity, applyPendingTuning + O-Lyrica precedent, MPE pitch-bend routing, NoiseExciter onset behaviour, getNumActiveVoices availability, Gate 4 DAW/fixture verification paths). Pre-flight: confirm working tree state matches Phase 2.3 verify rev-4 baseline.
3. **Phase 2.4 plan** — task breakdown for `BassoonSynthesiser.{h,cpp}` (NEW) + `Exciter.{h,cpp}` (MOD: dual-shape morph) + `BassoonVoice.{h,cpp}` (MOD: startNote NE/TuningEngine + renderNextBlock additive Exciter) + `PluginProcessor.{h,cpp}` (MOD: BassoonSynthesiser swap + voice_count snapshot) + `CMakeLists.txt` (MOD) + `ARCHITECTURE.md` rev-4 backfill.
4. **Phase 2.4 execute** — runs after Phase 2.3 atomic commit lands. Implements 4 systems (voice manager + cap + stealing, attack-character morph, NE per-voice consumption + MPE pitch-bend per-channel, TuningEngine getFrequency wiring).
5. **Phase 2.4 verify** — Gate 4 PASS bar: 10-item user-checkable (1 polyphony + 1 voice cap + 1 retrigger + 3 attack-character + 1 MPE + 1 NE + 1 QUAL-02 60s + 1 PERF-02 8-voice CPU) + automated invariant battery. Atomic commit `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS` on Gate 4 PASS. Closes Stage 2 (FUNC-02/05, DSP-05/06, PERF-02 final, QUAL-02 final).
6. UI mockup pass (parallel-eligible with Stage 2) — required to unblock Stage 3.
7. After Stage 2 complete: Stage 4 (validation — pluginval --strictness 10, Windows VST3, Dorico parity, presets, CHANGELOG). Stage 3 (GUI) blocks on UI mockup.

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

**Phase 2.2 atomic commit (2026-04-28):**
- Commit `baac74f` on `main`: `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`
- 14 files changed, 2149 insertions(+), 45 deletions(-)
- Sources: `Source/ModeBank.{h,cpp}` (bassoon partial table + formant Gaussian + tone wiring + applyToneChange + strike() + cached cosTheta/sinTheta/amp), `Source/BassoonVoice.{h,cpp}` (setTone forwarder; startNote calls strike() after setFundamental), `Source/PluginProcessor.{h,cpp}` (toneSmoother + dispatch BEFORE NE drain)
- Planning: `.planning/research/ARCHITECTURE.md` (as-shipped rev-2 note), `.planning/REQUIREMENTS.md`, `.planning/STATUS.md`, `.planning/stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}.md` (rev-2 addenda)

**Files Updated (Stage 2 / Phase 2.3 discuss — 2026-04-28):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md` (rev-3 addendum) — Phase 2.3 opening: 4 APVTS-driven systems scoped (ADSR + breath + vibrato + output_gain). Architectural pivot locked: continuous filtered-noise excitation (`NoiseExciter`: 1-pole LP @ 2 kHz, BASE_NOISE_GAIN 0.05f, breath-scaled) replaces struck-modal-only sustain; Phase 2.1 impulse exciter dropped from voice render path (file retained); rev-3 strike() retained at startNote for transient. Per-voice sine LFO vibrato with onset SmoothedValue + multiplicative pitch-bend compose + block-rate recompute + |Δf_final| > 0.1 Hz throttle. Breath = `ui_breath × cc2_normalised` with CC2-takeover (500 ms idle window). Aggregate `setExpression(...)` per-voice setter, throttled-epsilon (0.001) at processor scope for all 6 expression APVTS reads. Per-voice 20 ms breath smoother (sample-rate getNextValue). Processor-level 30 ms output_gain smoother (block-rate applyGainRamp). Variable-duration vibrato_onset SmoothedValue. ADSR setParameters block-rate epsilon-throttled. Gate 3 PASS bar = 10-item (3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone-60s + 1 polyphony-CPU). Inline iteration ceiling at rev-3. 8 user-confirmed approach decisions across 2 AskUserQuestion batches; 12 derived decisions; 10 open questions handed to research-phase; 9 risks documented with mitigations. Atomic commit subject locked: `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`.

**Files Updated (Stage 2 / Phase 2.3 research — 2026-04-28):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md` (rev-3 addendum) — all 10 OQs resolved with JUCE 8.0.4 source-line cites + family precedent. Key locks: (OQ#1) `applyGainRamp(0, numSamples, smoother.getCurrentValue(), smoother.skip(N))` declick-safe idiom (juce_AudioSampleBuffer.h:736-769 + juce_SmoothedValue.h:330-342); (OQ#2) block-rate ADSR `setParameters` with epsilon throttle (no internal smoother needed — juce_ADSR.h:92-99); (OQ#3) per-voice `juce::Random` with `voiceIndex × 31337` seed (O-Bowed `BowNoiseGenerator.h:23` precedent — overrides CONTEXT default `Time::currentTimeMillis() ^ voiceIndex`); (OQ#4) `BASE_NOISE_GAIN = 0.05f` start, verify-phase ear-tunes within [0.03f, 0.20f] bracket; (OQ#5) CC2-takeover 500 ms; (OQ#6) ordering tone → expression (NEW) → NE-drain → render → output_gain-applyGainRamp (NEW); (OQ#7) `f_final = NE-tuned × vibratoMult × pbMult` compose chain; (OQ#8) CC2 normalise at controllerMoved (O-Wind FluteSynthVoice.cpp:227 precedent); (OQ#9) vibrato phase **random** per startNote (O-Wind FluteSynthVoice.cpp:114-116) — overrides CONTEXT default instant-zero; (OQ#10) Logic-AU 60s bounce + Python `numpy.isfinite` + 1s-window RMS drift check + Logic Process bar comparison. 7 discrepancies surfaced (D1-D7-rev-3); none block planning. Implementation skeletons for `Vibrato.{h,cpp}` + `NoiseExciter.{h,cpp}` + `BassoonVoice` rev-3 deltas + `PluginProcessor` rev-3 deltas + CMakeLists ready for plan-phase verbatim consumption. 10 static-check grep gates locked. Pre-flight `ninja O-Bassoon_VST3` from `baac74f` confirmed clean (no work to do).

**Files Updated (Stage 2 / Phase 2.3 plan — 2026-04-28):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md` (rev-3 addendum) — 11-task single-Wave plan appended to existing rev-1 (Phase 2.1) + rev-2 (Phase 2.2). Tasks: (1) Source/Vibrato.{h,cpp} NEW (per-voice sine LFO + onset envelope, random phase per startNote, variable-duration onset SmoothedValue), (2) Source/NoiseExciter.{h,cpp} NEW (per-voice 1-pole LP @ 2 kHz over white noise, BASE_NOISE_GAIN 0.05f, voiceIndex×31337 seed), (3) BassoonVoice.h Phase 2.3 surface (Vibrato + NoiseExciter members, breathSmoother, lastDispatchedFrequency, 5 expression dispatch shadows, CC2-takeover state machine members, voiceIndex), (4) BassoonVoice.cpp deltas (prepareToPlay vibrato/noise/breath prepare; startNote ADSR APVTS reads + reset Phase 2.3 systems + shadow init; controllerMoved CC2 routing with multiplicative compose; setExpression aggregate setter with per-sub-param epsilon + CC2-takeover gate; renderNextBlock pivot — vibrato compose + setFundamental throttle + continuous-noise per-sample loop, drops Phase 2.1 exciter call), (5) PluginProcessor.h outputGainSmoother + 6 dispatch shadows, (6) PluginProcessor.cpp constructor setVoiceIndex wire + prepareToPlay smoother init + processBlock expression dispatch BEFORE NE drain + applyGainRamp AFTER renderNextBlock, (7) CMakeLists.txt target_sources +4 entries, (8) ARCHITECTURE.md rev-3 note backfill (architectural pivot + continuous-noise spec + breath state machine + vibrato compose chain + ordering invariant), (9) build + install + 10 static-check grep gates (RT-safety zero hits, NE drain ordering, expression dispatch site, applyGainRamp form, modeBank.setFundamental ≥2 hits, 1/8 scaler retention, throttle epsilon ≥7 hits, DSP-07, auval, pluginval-5), (10) manual Gate 3 verification (10-item: 3 ADSR + 1 breath + 3 vibrato + 1 output_gain + 1 long-tone-60s with Python numpy.isfinite + RMS drift + CPU drift + 1 polyphony-CPU; bounces phase-2.3-60s-c3-vibrato-breath.wav), (11) atomic commit. Lifts RESEARCH-rev-3 §3 implementation skeletons verbatim. Atomic commit subject locked: `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`. 9 risks carried with mitigations. Inline iteration ceiling at rev-3.

**Files Updated (Stage 2 / Phase 2.4 discuss — 2026-04-29):**
- `plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md` (rev-4 addendum) — Phase 2.4 opening: 4 systems scoped (voice manager + cap + stealing, attack-character morph, NE per-voice consumption + MPE pitch-bend per-channel, TuningEngine getFrequency wiring) plus QUAL-02 60s revisit. Single-pass cycle. NEW translation unit `Source/BassoonSynthesiser.{h,cpp}` (subclass juce::Synthesiser; override findFreeVoice to gate by activeVoiceCap; rely on JUCE default findVoiceToSteal for release-tail-first stealing). Exciter re-engaged with dual-shape morph (softShape 5ms half-sine × exp from Phase 2.1 + tonguedShape NEW 7.5ms exp-decay noise burst) crossfaded via attack_character + velocity bias `effective = clamp(attackChar + (vel-0.5)*0.3, 0, 1)` with onset-window latch (snapshot at note-on, mid-onset automation only affects next note-on). f_base compose at startNote: `tuningEngine->getFrequency(midi) → applyPendingTuning(table, midi, f_base) → currentFrequencyBase`. NoiseExciter additive during onset window. voice_count APVTS read at processBlock prologue snapshot (integer comparison throttle, applies on next note-on). 8 user-confirmed approach decisions across 2 AskUserQuestion batches: Batch 1 — single-pass / startNote-only NE / ear-only A/B / include 60s gate; Batch 2 — voice_count next-note-on / release-tail-first stealing / rev-3 ceiling / commit subject `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`. 12 derived decisions. 10 OQs handed to research-phase (JUCE findFreeVoice override pattern, voice_count snapshot site, soft/tongued shape spec, velocity bias magnitude, TuningEngine bit-identity check, applyPendingTuning + O-Lyrica precedent, MPE pitch-bend routing in juce::Synthesiser vs MPESynthesiser, NoiseExciter onset behaviour, getNumActiveVoices availability, Gate 4 DAW/fixture verification paths). 10 risks documented with mitigations. Process invariant: Phase 2.3 atomic commit MUST land BEFORE Phase 2.4 execute-phase begins (PLAN-rev-4 task #1 hard gate).
- `plugins/O-Bassoon/.planning/STATUS.md` — phase: verify_complete → discuss_complete; stage banner Phase 2.3 verify → Phase 2.4 discuss; next_action phase_2_3_atomic_commit_then_phase_2_4 → phase_2_4_research_phase; Next Steps re-numbered for Phase 2.4 progression (research → plan → execute → verify) with Phase 2.3 atomic commit retained as prerequisite for Phase 2.4 execute.
