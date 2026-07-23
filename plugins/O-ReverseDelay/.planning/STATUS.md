---
plugin: O-ReverseDelay
stage: 1-foundation
phase: discuss
phase_status: complete
status: in_progress
last_updated: 2026-07-23
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
workflow_mode: manual
next_action: /plugin-research O-ReverseDelay
ready_for_implementation: true
contract_checksums:
  brief: sha256:7075c269df79e5dc1cf7f28d6ff4dda88805abb2c8086fc751e737f626efb07e
  parameter_spec: sha256:34f6fdf831f785784354d09ff8df864f9c4df42e0fb2175fe5645be0ade3ef39
  architecture: sha256:28f04b7ddc3e2d6d5dbb20616fde05a9f9e1665d8a4a14b8afd2a7765f0eecfa
  roadmap: sha256:854f43dbdabb6bed6a7f7042d024d72b8a61afd7e95913d65fa12ef62258f98e
---

# O-ReverseDelay Status

## Current Position

Stage: 1 (Foundation) — discuss phase complete
Status: CONTEXT.md written; next phase: research
Progress: [##..................] 12%

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-07-23 | |
| research | | | |
| plan | | | |
| execute | | | |
| verify | | | |

**Stage 1 discuss decisions:**
- D1: Bus layouts mono→mono, mono→stereo, stereo→stereo (user choice; deviates from suite stereo-only)
- D2: pluginval strictness 10 from Stage 1 (COMPAT-01 gate)
- D3: Render harness deferred to Phase 2.1 per roadmap
- PLUGIN_CODE `ORvD`; target `OuariconReverseDelay`; VERSION 1.0.0

## Completed So Far

**Ideation:** ✓ Complete
- Creative brief, 14 requirements, 10 draft parameters

**Stage 0:** ✓ Complete — Research & Planning
- Plugin type: stereo audio effect (granular reverse delay, time-domain, stateful)
- Complexity tier 3, research depth MODERATE
- 7 features researched: capture buffer, reverse grain engine, tempo sync, damping filters, feedback stability, width, mix
- Professional references: Strymon TimeLine (REV/grain), ValhallaDelay Reverse, Red Panda Particle 2, Boss DD-7 (contrast), yaleD
- JUCE APIs verified against local 8.0.14 source: `dsp::IIR::ArrayCoefficients`, `AudioPlayHead::PositionInfo::getBpm()`, `dsp::DryWetMixer` (rejected), `SmoothedValue`
- In-suite prior art mapped: O-GrainScatter (DelayBuffer, GrainScheduler), O-simpleGrain (WindowLuts, harness)
- Every HIGH/MEDIUM-risk feature has a documented fallback (dual crossfaded reverse heads; RMS energy limiter)
- Complexity score: 5.0 (capped) — **Staged implementation** (DSP phases 2.1–2.3, GUI phases 3.1–3.2)
- ARCHITECTURE.md and ROADMAP.md documented

## Next Steps

1. Stage 1: Foundation — build system + APVTS shell (foundation-shell-agent via `/implement O-ReverseDelay`)
2. Create UI mockup before Stage 3 (none exists yet; UI-02 needs sync/free conditional display)
3. Review `research/ARCHITECTURE.md` and `ROADMAP.md`

## Context to Preserve

**Key Decisions:**
- Granular reverse smear engine (reverse read offset D+2n over 3.5 s capture ring); per-grain parameter latching for click-free changes
- Feedback through shared capture buffer (alternating-direction regenerations = intended character); tanh loop stability at unity gain
- Damping: 2nd-order Butterworth IIR + ArrayCoefficients in-place (DSP-02); cutoff clamp 0.49·fs
- Custom equal-power mix (DryWetMixer rejected — zero latency); density compensation before feedback tap
- Render harness FIRST in Phase 2.1 — all Stage-2 acceptance criteria are offline-render assertions

## Files Created (Stage 0)
- plugins/O-ReverseDelay/.planning/research/ARCHITECTURE.md
- plugins/O-ReverseDelay/.planning/ROADMAP.md
- plugins/O-ReverseDelay/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-ReverseDelay/NOTES.md
