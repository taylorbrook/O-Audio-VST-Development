---
plugin: O-Granulator
stage: 0
status: complete
last_updated: 2026-11-04
complexity_score: 5.0
complexity_tier: 6
staged_implementation: true
orchestration_mode: true
next_action: create_ui_mockup_and_finalize_parameter_spec
next_stage: 1
ready_for_implementation: false
blocked_on: parameter-spec.md (finalize from UI mockup; draft only)
contract_checksums:
  brief: sha256:de163958ee8eb83ed4a3fdd391389078eeac5eeffcb1d689510b8f99d2210efa
  requirements: sha256:15bce1f9a9c847f902559754fcdebc8b6b1eeb6920372d885c7010c1fdb35198
  parameter_spec_draft: sha256:d0b1457ea7c1624ff757fd12c1acb7bd62c9acf7bebbdd3460c7f1d30cdbbce3
  parameter_spec: pending
  architecture: sha256:f0cf753b4de2cb6099ad327b0887a0c254f013746b373000a28f01ad7949d9d9
  roadmap: sha256:9016e0b8d70db50fa395a8fadf3e10cb9b131c11cfc718b7c1f0fd8a4ca50b5b
---

# O-Granulator Status

## Current Position

Stage: 0 of 4 (Research & Planning) — complete
Status: Research & Planning complete. Implementation is pending a UI mockup and the final `parameter-spec.md`.
Progress: [##..................] 10%

## Completed So Far

**Ideation:** ✓ Complete. Brief, 30 requirements, and a draft param spec (214).

**Stage 0:** ✓ Research & Planning complete. ARCHITECTURE.md and ROADMAP.md are documented (Complexity 5.0, raw 22.0, tier 6, DEEP).
- Plugin type: Audio Effect (live-input granular, 4 streams, shared pool)
- Professional references: Clouds/Beads, Granulator III, Efx Fragments, Output Portal, Bitwig/Ableton modulators
- JUCE: juce_audio_utils, juce_dsp (FFT for MPM, ReverbUnit), WebView; grain-engine v1.1.0 (MipRing<2>, GrainPool, GrainScheduler, CounterRng, WindowLut); reverb-unit v1.1.0; preset-manager
- Strategy: phased. Stage 2 has 6 phases, Stage 3 has 3.

## Next Steps

1. Create the UI mockup (`/start O-Granulator` → option 3). Then finalize `.planning/parameter-spec.md`, including the proposed `scale_root`, `sN_onset_trigger` ×4 and `grain_limit` (220 params), and freeze the 113-entry mod-destination order.
2. Stage 1: Foundation (`/implement O-Granulator`). Run it only after step 1.

## Context to Preserve

- Architecture: `plugins/O-Granulator/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-Granulator/.planning/ROADMAP.md`
- Stage 0 decisions: `plugins/O-Granulator/.planning/stages/0-ideation/CONTEXT.md`
- Key decisions:
  - Absolute 32-sample pass grid with a 32-sample feedback return (write-before-read).
  - Freeze is a head-stop and wins over feedback (no overdub in v1.0).
  - Coherent feedback-tap law plus 0.95 cap plus tanh plus AGC plus wet safety.
  - MPM pitch follower on the audio thread.
  - Append-only 113-destination matrix.
  - Reported latency is 0.
  - CPU budget: ≤ 30 % of one core @ 48 k/128 worst case.
- Module gaps (optional): G1 WindowMorphBank, G2 GrainPool::fadeOut, G3 GrainScheduler::rescheduleIn, G4 ReverbUnit raw-atomic bind (critical path; fallback is a local copy)

## Files Created
- plugins/O-Granulator/.planning/research/ARCHITECTURE.md
- plugins/O-Granulator/.planning/ROADMAP.md
- plugins/O-Granulator/.planning/stages/0-ideation/CONTEXT.md
