---
plugin: O-simpleGrain
stage: 3
status: complete
last_updated: 2026-06-25
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: stage_4_validation_polish
next_stage: 4
human_checkpoint_pending: live DAW listen of viz animation + 8 presets + drag-drop (7 runtime criteria DEFERRED from Stage 3 verify)
ready_for_implementation: true
gate_pre_stage_1: satisfied (parameter-spec.md finalized; UI mockup DEFERRED to Stage 3 per user decision 2026-06-24)
contract_checksums:
  brief: sha256:04e459d5ea7419ee195da953f932401fe006e51f2d73eefae37224566b5afe7a
  parameter_spec_draft: sha256:603ec27bdaca550a5ec2a07f98effcc886a412b9037be13e06fb8507767aeaf7
  architecture: sha256:df90905b8dec0e9fad63011020aaaa768169d3478adfc95c622386eeaf83cf61
  roadmap: sha256:35e8f6de2155d8ce3f665e8d9c61db6f4e6d720d7d493cf2df8d39ea008fc135
---

# O-simpleGrain Status

## Current Position

Stage: 3 of 4 (GUI) — ✅ complete (code+build; 7 runtime criteria pending human DAW listen)
Status: Full WebView field-guide UI — all 18 params two-way bound, drag-drop+picker source load, four live visualizations + grain/overlap/CPU readout, per-control tooltips, 8 concept presets. Builds VST3+AU+Standalone clean; **auval SUCCEEDED**; **13/13 statically-verifiable Stage-3 criteria PASS, no defects**. Ready for Stage 4 (Validation/Polish).
Progress: [###############.....] 75%

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-06-24 |
| research | ✓ | 2026-06-24 |
| plan | ✓ | 2026-06-24 |
| execute | ✓ | 2026-06-24 |
| verify | ✓ | 2026-06-24 |

### Stage 2: DSP
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-06-24 |
| research | ✓ | 2026-06-24 |
| plan | ✓ | 2026-06-24 |
| execute (2.1→2.2→2.3) | ✓ | 2026-06-24 |
| verify | ✓ | 2026-06-24 |

### Stage 3: GUI
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-06-25 |
| research | ✓ | 2026-06-25 |
| plan | ✓ | 2026-06-25 |
| execute (3.1→3.2→3.3) | ✓ | 2026-06-25 |
| verify | ✓ (human_needed: 7 runtime criteria deferred to DAW listen) | 2026-06-25 |

### Stage 4: Validation / Polish
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-06-25 |
| research | → | |
| plan | | |
| execute | | |
| verify | | |

**Stage 4 discuss decisions (locked 2026-06-25):** (1) Automated validation FIRST, then human DAW listen (batched checklist at end). (2) Baseline validation only — no new code unless a defect surfaces. (3) Windows deferred entirely to publish/CI. See `stages/4-polish/CONTEXT.md`.

## Completed So Far

**Stage 3 (GUI):** ✓ Complete (code+build) — full Ouaricon-Naturalist "Field Guide" WebView UI in 3 sub-phases. **3.1** production `index.html` + adapted FM CSS base (2×2 viz grid + side control rail + 8-button preset bar + drop zone), PluginEditor rewrite (relays→WebView→attachments; **15 WebSliderRelay + 2 WebComboBoxRelay + 1 WebToggleButtonRelay**, 3-arg attach/nullptr undoManager), bare-path resource provider, 5 fixed-name drag-drop/picker native fns wired, CMake UI-resources binary-data (distinct `UIBinaryData` namespace vs `.wav` `BinaryData`) + WebView flags + Windows `withUserDataFolder`. **3.2** 30 Hz editor Timer consuming the Stage-2 taps → grain-cloud scatter (UI-01), source-waveform live playheads/freeze-pin/spray-band (UI-02, + `getSourceThumbnail` fn), scope+spectrum via message-thread FFT (UI-04, scope copied before in-place FFT), window-envelope inset (UI-03, JS recompute), grain/overlap/CPU readout (UI-05). **3.3** plain-language hover tooltips on every control (33 `data-tip`≡33 `TIPS` keys, FUNC-07), `applyFactoryPreset` with 8 distinct concept snapshots writing APVTS + 8 wired tour buttons (HTML≡C++≡JS name parity, FUNC-06), cloud/waveform annotations. Builds VST3+AU+Standalone clean; **auval SUCCEEDED**; **13/13 statically-verifiable criteria PASS, zero defects**. **DEFERRED to human DAW listen + Stage 4:** audio-driven viz animation, audible preset character, live drag-drop, host-automation round-trip (cannot be driven headlessly).



**Stage 2 (DSP):** ✓ Complete — full granular engine in 3 sub-phases. **2.1** core engine (8-voice `GrainVoice`, preallocated `std::array<Grain,24>`/voice + steal-oldest, density scheduler, 5 window LUTs, Lagrange read, overlap-add, key-tracked resample, amp ADSR). **2.2** read head (scan/time-stretch/freeze, smoothed/click-free) + per-grain spray/scatter + velToDensity + rate-tracking AA one-pole. **2.3** 4 embedded built-ins (`juce_add_binary_data` + decode/resample + atomic hot-swap) + load-your-own (drag-drop `convertFromBase64` C++ handlers + picker + 10 s cap) + three lock-free viz taps (`VizRing` / `TripleBuffer<GrainCloudFrame>` / `atomic<int>` count). Discuss decisions: procedural samples = shipping built-ins; express run. Builds VST3+AU+Standalone clean; **auval SUCCEEDED**; **8/8 offline DSP-correctness harness PASS** (`tests/render-harness/`, `-DOUARICON_BUILD_TESTS=ON`) — incl. exact MIDI key-tracking (C2/C3/C4 within <1% of `130.81·2^((N−60)/12)`). RT-safe, `setLatencySamples(0)`.

**Ideation:** ✓ Complete — BRIEF.md, REQUIREMENTS.md (24 reqs), parameter-spec-draft.md.

**Pre-Stage-1 gate:** ✓ Satisfied — parameter-spec.md finalized (18 params, research-locked). UI mockup DEFERRED to Stage 3 per user decision.

**Stage 1 (Foundation):** ✓ Complete — CMake (synth + WebView2 flags, `PLUGIN_CODE OsGr`), 18-param APVTS, custom loaded-source-identity state, silent allocation-free `processBlock`, placeholder editor. Builds VST3+AU+Standalone; **auval SUCCEEDED** (18 Global Scope Parameters). Installed as `O-simpleGrain-dev` (`aumu OsGr OuDv`).

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

1. **Human DAW-listen checkpoint (the 7 deferred Stage-3 runtime criteria)** — load `O-simpleGrain-dev` (`aumu OsGr OuDv`) in a DAW/Standalone with a MIDI keyboard and confirm: grain **cloud accumulates** (density thickens, spray widens); **spectrum shows discrete sidebands at scatter=0** and smears toward noise at high scatter; **scope** moves with output; **grain/overlap/CPU readout** counts `N/192`; **freeze pins the playhead** (❄ pin + shaded spray band); **window inset** redraws on Window combo change; **each of the 8 presets** snaps knobs/combos/toggle + caption/active state; **every control** shows its hover tooltip; **drag-drop a .wav AND Load…** both granulate a user source; **host-automation→UI** round-trip. Any miss folds into Stage 4.
2. **Stage 4 (Validation/Polish)** — pluginval (VST3+AU) strictness sweep, preset audit, artifact/aliasing/freeze listen audit, drag-drop smoke test (macOS + the Windows config-parity check), changelog. Run `/clear` then `/implement O-simpleGrain`.
3. Optional: `./build/plugins/O-simpleGrain/tests/render-harness/.../O-simpleGrain-render-test` (after `cmake -DOUARICON_BUILD_TESTS=ON`) to re-run the 8 DSP gates after any engine change.

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
