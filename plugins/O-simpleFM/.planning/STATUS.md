---
plugin: O-simpleFM
stage: 1
phase: verify
status: stage_1_complete
workflow_mode: manual
last_updated: 2026-06-20
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: implement_stage_2_dsp
next_stage: 2
ready_for_implementation: true
contract_checksums:
  brief: sha256:86cfcfb5ebf9181fd4d3c889a15931df918833295b8d5446d044e27db855dd81
  parameter_spec: sha256:b9915e21a6e8f79748a6e3456dbda1cba31a0e84e6d38844aedfa3c28dcbdbe4
  architecture: sha256:33721b1b925f5113564124f7fdae0c44dc6468dfd9b7b81af4de5be4df038ef2
  roadmap: sha256:bcad5b7e708eab6ae60ec488da615632f6ecfd0a433cd8abd7cc6379cc05df58
---

# O-simpleFM Status

## Current Position

Stage: 1 of 4 (Foundation) — ✅ complete (all 5 phases). Next: Stage 2 (DSP).
Status: Silent synth shell builds + auval PASS + 17-param APVTS + state persistence.
Progress: [#####...............] 25%

## Stage 1 Phase Progress
| Phase | Status | Notes |
|-------|--------|-------|
| discuss | ✓ | CONTEXT.md (auto-derived; scope locked at Stage 0) |
| research | ✓ | RESEARCH.md (patterns from O-Bassoon/O-AnalogEQ) |
| plan | ✓ | PLAN.md (17-param contract + 5 tasks) |
| execute | ✓ | CMake + processor + editor; built VST3/AU/Standalone |
| verify | ✓ | VERIFICATION.md — auval SUCCEEDED, 17 params, state persist |

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (pedagogical 2-op FM/PM synth)
- Architecture chosen (2-operator + modulator self-feedback)
- Core parameters specified
- Requirements extracted (22: must 14 / should 6 / nice 2)

**Stage 0:** ✓ Complete
- Plugin type: Synth (pedagogical 2-operator FM/PM), MIDI-in → audio-out, 16-voice poly, WebView UI
- Complexity tier 4 (synth + MIDI + oscillators) escalated toward 6 by first-class real-time FFT/scope viz → research depth DEEP
- Professional plugins researched: DX7, Ableton Operator, NI FM8, Syntorial (all confirmed PM, not true FM)
- JUCE 8 modules verified against local source (8.0.9): juce::Synthesiser, SynthesiserVoice, ADSR, dsp::LookupTableTransform, dsp::FFT, dsp::WindowingFunction, dsp::Oversampling (filterHalfBandPolyphaseIIR), AbstractFifo, SmoothedValue
- DSP feasibility verified; per-feature risk assessment with fallbacks documented
- Parameter ranges researched (core 17-param set)
- Complexity score: 5.0 (capped; raw 11.0)
- Strategy: Staged implementation (3 DSP phases + 3 GUI phases)
- ARCHITECTURE.md, ROADMAP.md, CONTEXT.md documented; upstream research doc folded in

## Next Steps

1. Stage 1: Foundation — create build system (IS_SYNTH + WebView2 flags) and APVTS parameters → run `/implement O-simpleFM`
2. Review ARCHITECTURE.md and ROADMAP.md
3. (Non-blocking) Confirm DSP-06 optional params + non-sine operators at mockup phase

## Context to Preserve

**Key Decisions:**
- Plugin type: Synth (FM/PM), 16-voice polyphony
- **PM not true FM** — radians phase convention internally (1:1 with Bessel/Chowning math); never mix with normalized-turns
- **Modulation index = raw radian index `I`, 0–20**, perceptual taper (`I = 20·norm^1.7`), displayed linearly; carrier null at I≈2.405 = marquee teaching annotation
- **Mod env → index multiplicative, depth default 1.0**; amp ADSR independent, governs voice lifetime
- **DX7 self-feedback** with two-sample average (Tomisawa anti-hunting); clamp history + NaN scrub + reset on note-on
- **Anti-aliasing floor:** sine LUT + key-tracked index ceiling (Carson) + 2× polyphase-IIR oversampling always-on; 4× + band-limited wavetables only when non-sine operators enabled; NO PolyBLEP (doesn't compose with hard FM)
- **Real-time-safe viz:** audio thread copy-only into AbstractFifo ring; FFT (4096 / Blackman-Harris) on message-thread Timer (30 Hz); copy scope window BEFORE in-place FFT
- WebView2 flags required (NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 + withUserDataFolder)
- CMake: IS_SYNTH TRUE + NEEDS_MIDI_INPUT TRUE (or silent in DAW)

**File Locations:**
- plugins/O-simpleFM/.planning/research/ARCHITECTURE.md
- plugins/O-simpleFM/.planning/ROADMAP.md  (complexity 5.0, raw 11.0, staged)
- plugins/O-simpleFM/.planning/stages/0-ideation/CONTEXT.md
- research/fm-phase-modulation-synthesis-o-simplefm.md  (upstream Level-3 research)

**Reference plugins for implementation:**
- O-Bassoon (SynthesiserVoice skeleton + block-param push) · O-Marimba (WaveformFifo + native-fn scope, 30 Hz Timer) · O-Prism (APVTS NormalisableRange/skews, WebView relay/attachment order, dsp::FFT) · O-AnalogEQ (cross-platform WebView2 CMake) · O-MultiBandCompressor (dsp::FFT)

## Files Created
- plugins/O-simpleFM/.planning/research/ARCHITECTURE.md
- plugins/O-simpleFM/.planning/ROADMAP.md
- plugins/O-simpleFM/.planning/stages/0-ideation/CONTEXT.md
- research/fm-phase-modulation-synthesis-o-simplefm.md (frontmatter added Stage 0)
