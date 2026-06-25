---
plugin: O-simpleGrain
stage: 0
status: complete
last_updated: 2026-06-24
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: finalize_mockup_then_invoke_foundation_shell_agent
next_stage: 1
ready_for_implementation: true
contract_checksums:
  brief: sha256:04e459d5ea7419ee195da953f932401fe006e51f2d73eefae37224566b5afe7a
  parameter_spec_draft: sha256:603ec27bdaca550a5ec2a07f98effcc886a412b9037be13e06fb8507767aeaf7
  architecture: sha256:df90905b8dec0e9fad63011020aaaa768169d3478adfc95c622386eeaf83cf61
  roadmap: sha256:35e8f6de2155d8ce3f665e8d9c61db6f4e6d720d7d493cf2df8d39ea008fc135
---

# O-simpleGrain Status

## Current Position

Stage: 0 of 4 (Ideation / Research & Planning) — complete
Status: Research & Planning complete; ready for mockup finalization → Stage 1
Progress: [##..................] 10%

## Completed So Far

**Ideation:** ✓ Complete — BRIEF.md, REQUIREMENTS.md (24 reqs), parameter-spec-draft.md.

**Stage 0 (Research & Planning):** ✓ Complete
- Plugin type confirmed: Synth (Pedagogical Granular), MIDI-in poly (8 voices), Freeze mode, WebView UI.
- Complexity Tier 5–6 (DEEP); complexity score 5.0 (capped; raw 13.0).
- Strategy: phased — Stage 2 DSP (3 phases) + Stage 3 GUI (3 phases) + Stage 4 validation.
- Grain engine mined near-verbatim from shipped **O-GrainScatter** (`GrainPool`/`GrainScheduler`/`LagrangeInterpolation`/`TripleBuffer`/`FreezeManager`); infrastructure inherited from **O-simpleFM/O-simpleAdditive** (`Synthesiser`/`ADSR`/`VizRing`/`FmVizAnalyzer`/WebView CMake); load-your-own from **O-MicrotonalSampler v1.0.4** (`juce::Base64::convertFromBase64`).
- All 11 open research questions resolved with concrete recommendations (see CONTEXT.md).
- ARCHITECTURE.md (11 sections, all required) + ROADMAP.md + CONTEXT.md documented.
- JUCE 8 APIs verified against local source (/Users/taylorbrook/JUCE/modules, v8.0.9).
- Requirements mapped to stages (COMPAT→S1; FUNC/DSP/PERF/QUAL→S2; FUNC-06/07/UI→S3).

## Next Steps

1. **(Gate) Mockup finalization** — produce the full `parameter-spec.md` (only the draft exists). The mockup becomes the source of truth for the final 18-param set.
2. Stage 1: Foundation (CMake synth + WebView2 flags, APVTS 18 params, state persistence incl. loaded-source identity, embedded-sample binary-data target; silent shell) — run `/implement O-simpleGrain` (or `/plugin-discuss O-simpleGrain` for the next stage in manual mode).
3. Review ARCHITECTURE.md and ROADMAP.md.
4. Pause here.

## Context to Preserve

**Key decisions (locked at planning 2026-06-24):**
- `density` = grains/sec (1–200) + derived live overlap readout `(grainSizeMs/1000)·density`.
- MIDI = key-tracked resample, combined multiplicatively with `grainPitch` + `pitchSpray`; root C3.
- Anti-aliasing = 4-pt Lagrange read + per-grain rate-tracking one-pole (no global oversampling; 2× OS fallback).
- 8 voices; `MaxGrainsPerVoice=24` (steal-oldest); global cap 192 grains (no xrun).
- `velToDensity` + `panSpray` adopted (default 0); velocity→amp always-on.
- Source-length cap 10 s; built-ins embedded `.wav` via `juce_add_binary_data`; load-your-own = content-streaming drag-drop (`juce::Base64::convertFromBase64`) + picker fallback.
- Feed-forward, no feedback loop; spectral STFT deferred to O-simpleSpectral.

**Gotchas (must hold through implementation):**
- No alloc/locks in processBlock (preallocated grain pool).
- Windows: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder`.
- Base64 decode: `juce::Base64::convertFromBase64`, NOT `MemoryBlock::fromBase64Encoding`.
- WebView: pass `Juce` namespace (not `window.__JUCE__`) to shared panels; resource provider gets bare paths.
- Rectangular window click = intended teaching artifact.
- Lock-free viz: `TripleBuffer` (grain events) + `VizRing` (samples) + `atomic<int>` (count); FFT on message thread.
- `setLatencySamples(0)` (getLatencySamples non-virtual in JUCE 8); source hot-swap via atomic pointer swap.

**Complexity score:** 5.0 (raw 13.0). **Strategy:** staged (4 stages).

**Pre-Stage-1 gate:** full `parameter-spec.md` required at mockup finalization (only draft exists now).

## Files Created
- plugins/O-simpleGrain/.planning/research/ARCHITECTURE.md
- plugins/O-simpleGrain/.planning/ROADMAP.md
- plugins/O-simpleGrain/.planning/stages/0-ideation/CONTEXT.md
- plugins/O-simpleGrain/.planning/STATUS.md (this file)
</content>
</invoke>
