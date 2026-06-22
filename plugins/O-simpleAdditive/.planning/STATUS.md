---
plugin: O-simpleAdditive
stage: 0
status: complete
last_updated: 2026-06-22
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
contract_checksums:
  brief: sha256:2270ef614ef3fbac3778fc7157e8c2ce62d2d04a8dcc0b1348b07278b2803435
  parameter_spec: sha256:1a05abe8a73cb8fc9678b3d3eb570fb79aae09f28d17b8efb473c1056f98c26c
  architecture: sha256:76bcd9f438a6a694a7ad9a8c686b63f3eb8e6da69ded3b9639dc094e3a429a00
  roadmap: sha256:b8377a47e3fe71191749afaad537f7c72ed183d23c7f233117c55b4f5303288c
---

# O-simpleAdditive Status

## Current Position

Stage: 0 of 4 (Ideation / Research & Planning) — complete
Status: Research & Planning complete, ready for implementation
Progress: [##..................] 10%

## Completed So Far

**Ideation:** ✓ Complete
- Core concept (pedagogical 16-partial additive + wavetable scan/morph; sibling to O-simpleFM)
- Requirements extracted (22) with acceptance criteria
- Parameter draft extracted

**Stage 0 (Research & Planning):** ✓ Complete
- Plugin type confirmed: Synth (Additive + light Wavetable), 16-voice poly, WebView UI
- Professional examples researched: Hammond drawbars, PPG/Waldorf wavetable, Harmor/Razor/Alchemy
- JUCE 8 modules identified & verified against local source (8.0.9): `Synthesiser`, `SynthesiserVoice`, `ADSR`, `dsp::LookupTableTransform`, `dsp::FFT`, `WindowingFunction`, `SmoothedValue`, `AudioParameterChoice` — confirmed NO native additive/wavetable class (engine is custom)
- All 9 open research questions resolved (render strategy, anti-aliasing, morph, Frame B editability, spectral-decay curve, bit-depth typing, polyphony, velocity routing, viz handoff)
- Complexity score: 5.0 (raw 12.0) → staged implementation
- ARCHITECTURE.md and ROADMAP.md documented; decisions folded into parameter-spec-draft.md

## Next Steps

1. Stage 1: Foundation (CMake synth shell + 33-param APVTS + state persistence) — Run `/implement O-simpleAdditive`
2. Review `research/ARCHITECTURE.md` and `ROADMAP.md`
3. Pause here

## Context to Preserve

**Key decisions (Stage 0):**
- Render: precompute band-limited single-cycle wavetable per note (2048 pts), read by phase — NOT per-sample sum-of-sines
- Anti-aliasing: exact band-limit (omit partials k>Kmax=floor(0.5·fs/f0)) + boundary taper — NO oversampling, zero latency
- Morph: linear *spectral* (per-partial lerp), zipper-free
- Frame B: preset-only 4-way choice (sine/saw/square/odd) — NOT a second drawbar set (stays 33 params)
- Spectral-decay: `D_k = exp(−rate·k·tau)` per-partial exponential tilt over the note
- Bit depth: discrete choice {off,12,10,8,6,4,2}, "off" = clean
- Polyphony: 16 voices (matches O-simpleFM)
- Velocity: amp always-on; `velToDecay` opt-in
- Viz: reuse O-simpleFM `VizRing` + `FmVizAnalyzer`; bars from active-spectrum snapshot
- LFO sine-only/global; mod-env→scan only (decay routing deferred to v1.1)
- **Lower-risk than O-simpleFM** (no feedback loop, no oversampling); most infrastructure inherited from O-simpleFM

**Reference model:** O-simpleFM (`plugins/O-simpleFM`) — voice model, dual ADSR, `fastSine`, `VizRing`/`FmVizAnalyzer`, WebView CMake. Reuse near-verbatim.

**Strategy:** Staged (3 DSP phases + 3 GUI phases).

## Files Created / Updated
- plugins/O-simpleAdditive/.planning/research/ARCHITECTURE.md (new)
- plugins/O-simpleAdditive/.planning/ROADMAP.md (new)
- plugins/O-simpleAdditive/.planning/stages/0-ideation/CONTEXT.md (new)
- plugins/O-simpleAdditive/.planning/parameter-spec-draft.md (Resolved Decisions folded in)
- plugins/O-simpleAdditive/.planning/STATUS.md (updated)
- PLUGINS.md (registry row → 🚧 Stage 0)
