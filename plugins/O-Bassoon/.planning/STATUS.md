---
plugin: O-Bassoon
stage: 0
status: complete
last_updated: 2026-04-27
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
contract_checksums:
  brief: sha256:4989e5389e14e7bf29ae16b3923e9a70438fd3b0b0e0a6405be9f6983763265f
  parameter_spec: sha256:708ecb2bf2a49fcc5b8f4a8b745859d652edc81aff53ef55071b69ca62b6b875
  architecture: sha256:d54a0e95fcd63e9a21554ec77024d78c52b11a88119074022a910e1a42bad641
  roadmap: sha256:2e69806dfcecbd80acccc1d42d2eb9dfe26945cd2be8d615dc7f64ae2978be2e
---

# O-Bassoon Status

## Current Position

Stage: 0 of 4 (Ideation / Research-Planning) — complete
Status: Stage 0 complete; ready for Stage 1 (Foundation)
Progress: [##..................] 10%

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

1. **Stage 1: Foundation** (CMake shell, APVTS, voice scaffolding) — invoke via `/implement O-Bassoon` or `/plugin-execute O-Bassoon`
2. UI mockup pass (parallel-eligible with Stage 1 + 2) — required to unblock Stage 3
3. Stage 2 phased DSP implementation begins after Stage 1 completes

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

**Files Created (Ideation, pre-Stage-0):**
- `plugins/O-Bassoon/.planning/BRIEF.md`
- `plugins/O-Bassoon/.planning/REQUIREMENTS.md`
- `plugins/O-Bassoon/.planning/parameter-spec-draft.md`
