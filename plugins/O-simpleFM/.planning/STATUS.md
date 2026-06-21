---
plugin: O-simpleFM
stage: 2
phase: verify
status: stage_2_complete
workflow_mode: express
last_updated: 2026-06-20
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: implement_stage_3_gui
next_stage: 3
ready_for_implementation: true
contract_checksums:
  brief: sha256:86cfcfb5ebf9181fd4d3c889a15931df918833295b8d5446d044e27db855dd81
  parameter_spec: sha256:b9915e21a6e8f79748a6e3456dbda1cba31a0e84e6d38844aedfa3c28dcbdbe4
  architecture: sha256:33721b1b925f5113564124f7fdae0c44dc6468dfd9b7b81af4de5be4df038ef2
  roadmap: sha256:bcad5b7e708eab6ae60ec488da615632f6ecfd0a433cd8abd7cc6379cc05df58
---

# O-simpleFM Status

## Current Position

Stage: 2 of 4 (DSP) — ✅ complete (all 5 phases + critic gate). Next: Stage 3 (GUI).
Status: Polyphonic 2-op PM synth renders audio; auval + pluginval(s10) PASS; FM math proven (carrier null I≈2.405 ratio 0.0001); feedback stable @100%; 2× OS; viz data path live.
Progress: [##########..........] 50%

## Stage 2 Phase Progress (express mode)
| Phase | Status | Notes |
|-------|--------|-------|
| discuss | ✓ | CONTEXT.md (auto-derived; scope locked at Stage 0) |
| research | ✓ | RESEARCH.md (exact suite reference code: O-Bassoon voice, O-Marimba ring, OS/FFT/LUT call sites) |
| plan | ✓ | PLAN.md (3 DSP sub-phases; file map; goal-backward criteria) |
| execute | ✓ | Operator.h + FMVoice.h + FmVizAnalyzer.h; synth/OS/viz wired; render harness; zero warnings |
| critic | ✓ | Adversarial DSP review — no blockers; W1/W2 fixed (fixed-ch OS + chunked render) |
| verify | ✓ | VERIFICATION.md — auval SUCCEEDED, pluginval s10 SUCCESS, harness 5/5 PASS |

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

1. Stage 3: GUI — `/clear` then `/implement O-simpleFM`. WebView UI: replace GenericAudioProcessorEditor body, emit `vizAnalyzer.getSpectrum()/getScope()` from the existing 30 Hz `timerCallback` via `emitEventIfBrowserIsVisible`, wire 17 relays/attachments. (3 GUI sub-phases per ROADMAP.)
2. (Non-blocking) UI mockup first via `/design-ui O-simpleFM` if a visual system isn't chosen yet.
3. (Stage 4) Critical-listening aliasing audit; factory presets (preset tour); changelog.

**Stage 2 carryover hooks for Stage 3:**
- Editor already a `juce::Timer`; `timerCallback` pumps `FmVizAnalyzer` (ring→FFT/scope) — just add the WebView emit.
- `FmVizAnalyzer::getSpectrum()` (256 log-freq dB bins) + `getScope()` (128 pts) ready to serialize.
- Optional `Δf = I·f_m` readout + on-screen keyboard MIDI injection (nice-only).
- Render harness (`-DOUARICON_BUILD_TESTS=ON`) is a permanent regression gate.

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
