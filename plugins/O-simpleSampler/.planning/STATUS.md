---
plugin: O-simpleSampler
stage: 0
status: complete
last_updated: 2026-06-25
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
contract_checksums:
  brief: sha256:96debe9dfd2c5a92362d6ec3a6ba0fb26bf684b33aef76e5d78312690d5ff7ee
  parameter_spec: sha256:3d6d4582e204bbd0aa01a2a473604c70df4572827e88773e3b37bc2fe22b5854
  architecture: sha256:acbb55e7dd04c8fd1fee401f64f1f1e79858958961d12c57a74f03f1f372212f
  roadmap: sha256:ee2b65d0db577b8324f8340a600cf35fb5537cde7f9efa1358419ce183e32a6b
---

# O-simpleSampler Status

## Current Position

Stage: 0 of 4 (Ideation / Research & Planning) — complete
Status: ARCHITECTURE.md + ROADMAP.md produced; Stretch-vs-Repitch engine resolved; ready for Stage 1 (Foundation).
Progress: [##..................] 10%

## Completed So Far

**Ideation:** ✓ Complete
- Core concept: pedagogical keyboard sampler (source → region/loop/reverse → pitch → Vintage → filter → VCA)
- Scope: classic melodic keyboard sampler only (wk05); slicing/sequencer out of scope
- 24 requirements extracted (must 16 / should 6 / nice 2)

**Stage 0:** ✓ Complete
- Plugin type: Synth (pedagogical sampler), MIDI-in → audio-out, 16-voice poly, WebView UI
- Complexity tier 5–6 (synth + file I/O + load-your-own streaming + interactive waveform editor) → research depth DEEP
- **Stretch algorithm RESOLVED:** synchronous-granular (SOLA) pitch shifter reusing the O-simpleGrain/O-GrainScatter scheduler + overlap-add (Repitch = fractional-read varispeed); phase vocoder rejected for v1.0 (deferred to v1.1 as optional HQ Stretch)
- Anti-aliasing RESOLVED: 4-pt Lagrange + rate-tracking one-pole (`fc=0.5fs/rate`) on upward transposition
- Loop RESOLVED: equal-power crossfade + ping-pong + zero-crossing snap; Vintage RESOLVED: S&H decimation + bit-crush, bypass at 0 (SP-1200)
- Filter: per-voice `juce::dsp::StateVariableTPTFilter` LP + closed-form curve (QUAL-02); full per-voice chain; lead-voice drives the curve
- Sample loading: embedded `.wav` (2nd binary-data target, distinct NAMESPACE) + reused `webview-drop-streaming.js` (`juce::Base64::convertFromBase64`) + picker fallback
- Params resolved: keep `tune`+`fine`; defer `velToFilter`; 16 voices; 30 s cap → **21 core parameters**
- Professional references: SP-1200/MPC, Mellotron/Fairlight, Kontakt/Ableton Sampler/EXS24, Serato/élastique
- JUCE 8 APIs verified against local source (8.0.9) + shipped siblings; built-in `juce::Sampler` rejected (too limited)
- Complexity score: **5.0** (capped; raw 12.0)
- Strategy: **staged** (Stage 2 DSP × 3 phases, Stage 3 GUI × 3 phases)
- ARCHITECTURE.md + ROADMAP.md + Stage-0 CONTEXT.md documented

## Next Steps

1. Stage 1: Foundation — CMake (synth + WebView2 flags + dual binary-data NAMESPACE) + 21-param APVTS + state persistence (silent shell). Run `/implement O-simpleSampler` (or invoke foundation-shell-agent).
2. Review ARCHITECTURE.md (Repitch↔Stretch engine + sample-loading I/O) and ROADMAP.md.
3. Pause here.

## Context to Preserve

**Key decisions:**
- Stretch = synchronous-granular SOLA (time 1× + per-grain resample, Hann overlap-add, fixed/hidden grain) reusing O-simpleGrain; Repitch = continuous fractional-read varispeed
- Anti-alias: 4-pt Lagrange + rate-tracking one-pole; no oversampling; zero latency
- Loop: equal-power crossfade + ping-pong + zero-cross snap; Vintage: S&H decimation + bit-crush, bypass at 0, before the filter
- Full per-voice chain (source→region→pitch→Vintage→filter→VCA); lead-voice drives the filter curve
- Filter: `StateVariableTPTFilter` LP (linear) + closed-form magnitude curve (QUAL-02)
- Sample loading: 2nd `juce_add_binary_data` distinct NAMESPACE; `webview-drop-streaming.js` + `juce::Base64::convertFromBase64`; picker fallback; 30 s cap
- 21 core params; `Load…` is a native-fn + custom state, not a param; `velToFilter`/HQ phase-vocoder Stretch deferred to v1.1
- Highest risk: Stretch + upward-transposition AA (de-risked by shipped O-simpleGrain DSP); no feedback loop

**Strategy:** complexity 5.0, staged implementation.

**Files created:**
- plugins/O-simpleSampler/.planning/research/ARCHITECTURE.md
- plugins/O-simpleSampler/.planning/ROADMAP.md
- plugins/O-simpleSampler/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-simpleSampler/.planning/STATUS.md (this file)

**Sibling references:** O-simpleGrain (PRIMARY reuse), O-simpleSubtractive (filter/ADSR/voice/doc format), O-simpleFM/O-simpleAdditive (voice skeleton + bit-depth lesson), O-MicrotonalSampler (drag-drop + Base64), O-GrainScatter/O-Freeze (overlap-add + loop crossfade).
