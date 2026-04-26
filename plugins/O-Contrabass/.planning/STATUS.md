---
plugin: O-Contrabass
stage: 0
status: complete
last_updated: 2026-04-25
complexity_score: 5.0
complexity_score_raw: 16.0
complexity_tier: 6
research_depth: DEEP
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
contract_checksums:
  brief: sha256:6ea840bbe4c34855397111896b9f6fd52cb5f66c597676d300b0c5ce33c8988d
  parameter_spec: sha256:c47fe7361a55e1d64b906ef7194894f4a2490744b35a644c76b6e1a632282d0d
  architecture: sha256:3cb26814bcd830cfba0b3bba42c096bdbf5b1449f52825a167cde09e114855a0
  roadmap: sha256:106639f633b6b3a2cfeb41eb07640d3ac0e01ed0832c33a9da45faf2b97aca7e
---

# O-Contrabass Status

## Current Position

Stage: 0 of 4 (Ideation / Research & Planning) — complete
Status: Research & Planning complete, ready for Stage 1 implementation
Progress: [##..................] 10%

## Completed So Far

**Ideation:** Complete
- Core concept defined: bass-only specialized 4-string contrabass physical model, sustained orchestral arco + ambient drone
- Differentiation from O-Bowed clarified (deep specialization vs general-purpose)
- Parameters specified across 8 sections (Bow / Body / Strings / Detune / Expression / Drone / Output / Microtonal) — 29 total
- Sonic targets locked: deep wood body resonance, bow noise / rosin grit, slow expressive attack
- Drone-first features designed in (infinite sustain, sub-harmonics, slow-bow LFO, per-string detuning)
- Layered expression model defined (intrinsic CC + dedicated vibrato + Expression Macro)
- Full Ouaricon microtonal convention (Note Expression + MTS-ESP + Scala/TUN + MPE)
- Requirements extracted: 14 must / 7 should / 3 nice across FUNC, DSP, UI, PERF, COMPAT, QUAL

**Stage 0:** Complete (Research & Planning)
- Synthesized 4 deep-research documents (synthesis + waveguide-stability + body-acoustics + drone-and-subharmonics) into canonical architecture
- ARCHITECTURE.md documented with 11 required sections (immutable contract for Stages 1-4)
- All 29 parameters mapped to DSP components (no orphans, no unmapped components)
- 9 distinct DSP components specified with JUCE class assignments
- Integration analysis: feature dependencies, parameter interactions, processing order, thread boundaries
- 5 high-risk components documented with fallback architectures
- 6 architecture decisions recorded with rationale and alternatives
- Complexity score: 5.0 (capped from raw 16.0) — Tier 6 (Deep)
- Implementation strategy: phased across all stages with 6 DSP sub-phases
- 6 open decisions resolved with recommendations and defer-to-v1.1 flags
- ROADMAP.md documents Stage 1 (Foundation) → Stage 2 (DSP, 6 phases) → Stage 3 (GUI, 3 phases) → Stage 4 (Polish)
- Estimated total effort: 10–15 days

## Next Steps

1. **Stage 1: Foundation** — Run `/implement O-Contrabass` to invoke `foundation-shell-agent`
   - Create `CMakeLists.txt` with `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` + WebView flags
   - Create `Source/PluginProcessor.{h,cpp}` with 29-parameter APVTS
   - Verify pluginval strictness 10 baseline
   - Confirm macOS VST3+AU and Windows VST3 build
2. Review `ARCHITECTURE.md` and `ROADMAP.md` if any Stage 0 decisions need revisiting before Stage 1
3. Pause point established at end of Stage 0 (per CLAUDE.md handoff protocol)

## Context to Preserve

**Architecture highlights:**
- 4-string EADG digital waveguide (E1-G3) with `juce::dsp::DelayLine<float, Lagrange3rd>` (8192-sample buffer)
- Hyperbolic friction junction at 2x oversampling (`filterHalfBandPolyphaseIIR`)
- Cascaded allpass dispersion per string (M=4/3/2/1 for E/A/D/G)
- 8-mode parallel biquad body bank (Askenfelt-derived, Body Size scales freq, Q invariant)
- 3-band BPF bow noise (700/1500/3000 Hz) summed AFTER body resonator
- Drone features: Infinite Sustain (loop gain 0.997 → 0.99995), Sub-Harmonics (period-doubling friction bias), Slow-Bow LFO (Schelleng-aware diagonal modulation)
- Vibrato modulates delay-line length (physically correct; Lagrange3rd absorbs cleanly)
- Master saturator (polynomial) + zero-latency feedforward limiter (-1 dBFS)
- Ouaricon microtonal: priority Note Expression > MTS-ESP > Scala/TUN > MPE pitch-bend > 12-TET

**Module dependencies:**
- `modules/tuning/scala-tuning-engine` v2.1.0 (existing) — Scala/TUN + MTS-ESP
- `modules/tuning/note-expression` (existing) — VST3 Note Expression helper + JUCE patch
- `modules/dsp/bow-friction` (TO BE CREATED in Phase 2.1b) — extracted from O-Bowed `HyperbolicFriction.h`

**Reference plugins:**
- O-Bowed — friction junction, waveguide string, body resonator (extraction source)
- O-Lyrica — Note Expression integration pattern (BowedStringVoice template)

**Performance projection:** ~3.2% CPU on M1 (well under PERF-02's 5% target)

**Highest-risk component:** Friction Junction at E1 + max INFINITE_SUSTAIN + max SUB_HARMONICS — represents ~50% of project risk. Phase 2.1 must validate stability before any further features added.

**Key constraints (from juce8-critical-patterns + memory file):**
- `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` mandatory
- `BusesProperties` output-only in constructor
- `JUCE-NE-PATCH` must be applied to `~/JUCE/` for Note Expression
- `getLatencySamples()` is NOT virtual in JUCE 8 — use `setLatencySamples()` in `prepareToPlay`
- WebView2 needs both `NEEDS_WEBVIEW2 TRUE` AND `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
- Resource provider receives PATHS, not full URLs

**Open decisions resolved (see ROADMAP.md §"Open Decisions Resolved"):**
1. Friction tier: Hyperbolic only in v1.0 (defer elasto-plastic to v1.1)
2. Module extraction: DURING Stage 2 (mid-Phase 2.1)
3. Authentic Arco wolf toggle: defer to v1.1
4. Sub-harmonic max depth: 1 octave (f0/2)
5. Body Size mapping: full 1/4 → 4/4 span
6. Wood variants: single fixed wood for v1.0 (defer 2-variant to v1.1)

## Files Created

- `plugins/O-Contrabass/.planning/BRIEF.md` (Stage Ideation)
- `plugins/O-Contrabass/.planning/REQUIREMENTS.md` (Stage Ideation)
- `plugins/O-Contrabass/.planning/parameter-spec-draft.md` (Stage Ideation, 29 parameters)
- `plugins/O-Contrabass/.planning/research/ARCHITECTURE.md` (Stage 0 — DSP contract, 11 sections)
- `plugins/O-Contrabass/.planning/ROADMAP.md` (Stage 0 — implementation plan with phase breakdown)
- `plugins/O-Contrabass/.planning/stages/0-ideation/CONTEXT.md` (Stage 0 — discuss findings)
- `plugins/O-Contrabass/.planning/STATUS.md` (this file)

## Lifecycle Timeline

- **2026-04-25 (Stage Ideation):** Creative brief and requirements documented (FUNC, DSP, UI, PERF, COMPAT, QUAL across 24 reqs).
- **2026-04-25 (Stage 0):** Research & Planning complete — ARCHITECTURE.md and ROADMAP.md documented (Complexity 5.0, Tier 6, phased strategy with 6 DSP sub-phases).
