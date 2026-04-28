---
plugin: O-Bassoon
stage: 2
status: in_progress
phase: verify_complete
last_updated: 2026-04-27
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: atomic_commit_then_phase_2_2_discuss
next_stage: 2
phase_cycle: "Phase 2.1 — Core Modal Voice + First Audio (verified, atomic commit pending)"
contract_checksums:
  brief: sha256:4989e5389e14e7bf29ae16b3923e9a70438fd3b0b0e0a6405be9f6983763265f
  parameter_spec: sha256:708ecb2bf2a49fcc5b8f4a8b745859d652edc81aff53ef55071b69ca62b6b875
  architecture: sha256:d54a0e95fcd63e9a21554ec77024d78c52b11a88119074022a910e1a42bad641
  roadmap: sha256:2e69806dfcecbd80acccc1d42d2eb9dfe26945cd2be8d615dc7f64ae2978be2e
---

# O-Bassoon Status

## Current Position

Stage: 2 of 4 (DSP) — **Phase 2.1 / ✅ VERIFIED (Gate 1 9/9 PASS, item 10 dropped, atomic commit pending)**
Status: Phase 2.1 verified — Gate 1 items 1-6, 7, 8, 9 PASS (auto + user-confirmed Logic-AU); item 10 (SPAN PNG) dropped from gate by user authority; VERIFICATION.md written with ✅ VERIFIED verdict. Atomic commit `feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS` pending explicit user trigger.
Progress: [#############.......] 65%

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

1. **Atomic Phase 2.1 commit** — `feat(O-Bassoon): Phase 2.1 first audio - Gate 1 PASS`. Lands sources (ModeBank, Exciter, BassoonVoice, PluginProcessor, CMakeLists) + reference recordings + planning artefacts in one commit on `main`. Pending explicit user trigger per CLAUDE.md commit protocol.
2. **Phase 2.2 kickoff (after commit lands)** — `/clear` then `/plugin-discuss O-Bassoon 2-dsp` to open Phase 2.2 (bassoon partial-table tuning + `tone` parameter; A/B listening vs. archived VSCO-2-CE C3 sustain). Item 10 SPAN baseline dropped — Phase 2.2 A/B uses ear + reference WAV.
3. UI mockup pass (parallel-eligible with Stage 2) — required to unblock Stage 3
4. Remaining Stage 2 phases: Phase 2.2 (bassoon partial-table tuning + tone) → 2.3 (Expression: vibrato/breath/attack/release/output APVTS) → 2.4 (Polyphony cap + attack-character morph + NE/MPE consumption)

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
- Atomic commit (Task 9): **PENDING** explicit user trigger, per CLAUDE.md commit protocol (orchestrator does NOT auto-commit)

**REQUIREMENTS.md updates (verify-phase 2026-04-27):**
- PERF-01 (Real-time safe): pending → **complete** (RT-safety grep + pluginval-5 fuzz/state PASS)
- FUNC-03 (Range C1-C6): pending → **complete** (audible C1-C6 sweep PASS, all notes track pitch, modal bank reconfigures cleanly)
- QUAL-02 (>60s long-tone stability): pending → **partial** (>10s subset PASS; full 60s = Phase 2.3)
- DSP-07: complete (carried from Stage 1, unchanged)
- COMPAT-01: partial (strictness-5 PASS this phase; full strictness-10 + Windows is Stage 4)
- FUNC-01 / FUNC-04 / DSP-01 / QUAL-01: **partial** (structural halves verified audibly; spectral / expression / parameter-sweep deliverables pending Phase 2.2-2.3)
- FUNC-02 / FUNC-05 / DSP-02-06 / PERF-02: pending — deferred to Phase 2.2-2.4 per ROADMAP
