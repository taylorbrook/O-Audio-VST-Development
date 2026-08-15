---
plugin: O-Bitrot
stage: 0
status: complete
last_updated: 2026-08-15
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
contract_checksums:
  brief: sha256:b31cd60b3b7e9dca5cbae913ec92e01f3e0ce1af918527d7b56e5fae7ea287cb
  parameter_spec: sha256:95e0e239ab383c9a0fd3fcd680055b7a3b6b09e42b735aeeba21efa61c64bb4f
  architecture: sha256:1496c18e09d39f7ce38ce1f4ee037f9db50bac4002eb8d6a7f7f84e9eb16428a
  roadmap: sha256:bdc18fedff3b03d9e5fe3e39377d749821887d668a34e48cb385c03cca02ef7b
---

# O-Bitrot Status

## Current Position

Stage: 0 of 4 (Research & Planning) — complete
Status: Architecture and roadmap documented, ready for implementation
Progress: [##..................] 10%

## Completed So Far

**Ideation:** ✓ Complete
- Creative brief, requirements (18), parameter draft (31 params)

**Stage 0:** ✓ Complete (2026-08-15)
- Plugin type: Audio Effect (broken-media degradation, stereo)
- Complexity tier 3; complexity score 5.0 (raw 12.0, capped) → staged implementation
- Architecture: shared circular buffer + clocked stochastic read heads; 9 DSP components across
  6 degradation families; chain MediaPlayer → Packet → Codec → Crush → mix
- JUCE APIs verified against local 8.0.14 source (DryWetMixer, IIR ArrayCoefficients,
  FirstOrderTPTFilter, SmoothedValue, Random, AudioPlayHead::PositionInfo)
- Key decisions: packet loss on 20 ms grid (not MediaClock); constant 20 ms reported latency all
  modes; 8 seeded RNG streams; libgsm vendored (permissive license, GPL firewall vs RSBrokenMedia)
- Professional references: RC-20, iZotope Vinyl, Decimort 2, Digitalis, Airwindows DeRez,
  RSBrokenMedia (patterns only)
- ROADMAP: 5 DSP phases + 3 GUI phases with per-phase test criteria

## Next Steps

1. Stage 1: Foundation (build system + APVTS 31 params + passthrough) — Run /implement O-Bitrot
   (note repo pattern: the 0-ideation→1-foundation gate always needs --force)
2. UI mockup (six panels + global strip) → finalize parameter-spec.md (carry Stage 0 param deltas:
   CRUSH_RATE 500 Hz–20 kHz, CLOCK_SYNC_DIV 7 divisions)
3. Review ARCHITECTURE.md and ROADMAP.md

## Context to Preserve

- Architecture: `plugins/O-Bitrot/.planning/research/ARCHITECTURE.md` (BINDING contract)
- Plan: `plugins/O-Bitrot/.planning/ROADMAP.md`
- Stage 0 discuss findings: `plugins/O-Bitrot/.planning/stages/0-ideation/CONTEXT.md`
- Research base: `research/glitch-effects/degradation-dsp-deep-dive.md` (§6 modules, §2.5 anti-zipper)
- License cautions: RSBrokenMedia GPL-3.0 patterns-only; libgsm license file must be vendored+recorded
- Determinism contract: streams reseed in prepareToPlay + on SEED change; RNG consumed only at
  ticks/packets; 512-vs-4096 bit-identity is a Phase 2.1 gate

## Files Created

- plugins/O-Bitrot/.planning/research/ARCHITECTURE.md
- plugins/O-Bitrot/.planning/ROADMAP.md
- plugins/O-Bitrot/.planning/stages/0-ideation/CONTEXT.md
