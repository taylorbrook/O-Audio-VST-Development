---
plugin: O-simpleAdditive
stage: 2
status: in_progress
last_updated: 2026-06-22
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
stage_2_mode: incremental_dsp_builds
next_action: implement_phase_2_3_spectral_decay_bitdepth_viz
next_stage: 2
ready_for_implementation: true
contract_checksums:
  brief: sha256:2270ef614ef3fbac3778fc7157e8c2ce62d2d04a8dcc0b1348b07278b2803435
  parameter_spec: sha256:1a05abe8a73cb8fc9678b3d3eb570fb79aae09f28d17b8efb473c1056f98c26c
  architecture: sha256:76bcd9f438a6a694a7ad9a8c686b63f3eb8e6da69ded3b9639dc094e3a429a00
  roadmap: sha256:b8377a47e3fe71191749afaad537f7c72ed183d23c7f233117c55b4f5303288c
---

# O-simpleAdditive Status

## Current Position

Stage: 2 of 4 (DSP) — in progress (incremental-DSP-builds mode)
Status: Phase 2.2 (Scan/Morph + Mod-Env + LFO) built + auval-validated. **Wavetable dimension live.** Ready for Phase 2.3 (spectral-decay + bit-depth + viz tap).
Progress: [############........] 60%

## Stage 2 (DSP) Phase Progress — incremental builds (build + auval per sub-phase)

| Phase | Status | Artifact / Gate |
|-------|--------|-----------------|
| 2.1 Core additive voice | ✓ | stages/2-dsp/SUMMARY-2.1.md — build clean + **auval SUCCEEDED** |
| 2.2 Scan/morph + mod-env + LFO | ✓ | stages/2-dsp/SUMMARY-2.2.md — build clean + **auval SUCCEEDED** |
| 2.3 Spectral-decay + bit-depth + viz tap | ▶ next | — |

**Phase 2.1 result:** `AdditiveVoice.h` (new) — 16-partial band-limited single-cycle table (2048-pt), per-note `Kmax` band-limit + raised-cosine taper, headroom-normalized sum, amp ADSR + velocity, control-rate dirty-refill. Synthesiser wired (16 voices). VST3 + AU clean; `auval` SUCCEEDED incl. MIDI test at 11k–192k Hz.

**Phase 2.2 result:** Wavetable dimension. Frame B presets (Sine/Saw/Square/Odd) + per-partial spectral morph `lerp(A_k, B_k, scan)` in `refillTable`. Per-voice mod-env (2nd ADSR) + global sine scan LFO; scan = `clamp(scanPosition + lfo·depth + modEnv·envAmount)` → 20 ms smoother → control-rate refill (refill only when scan moves > 1e-4). VST3 + AU clean; `auval` SUCCEEDED incl. MIDI test. Default patch (scan=0) bit-identical to 2.1 — no regression. `spectralDecay`/`bitDepth`/`velToDecay`/viz APVTS params present but not yet consumed (2.3).

## Stage 1 Phase Progress (express mode) — ✓ complete

| Phase | Status | Artifact |
|-------|--------|----------|
| discuss | ✓ | stages/1-foundation/CONTEXT.md |
| research | ✓ | stages/1-foundation/RESEARCH.md |
| plan | ✓ | stages/1-foundation/PLAN.md |
| execute | ✓ | stages/1-foundation/SUMMARY.md (foundation-shell-agent) |
| verify | ✓ | stages/1-foundation/VERIFICATION.md — **PASS** |

**Stage 1 result:** VST3 + AU build clean; `auval` SUCCEEDED (aumu/OSiA/OuDv); **33 Global Scope Parameters**; zero latency (`setLatencySamples(0)`); state persistence wired; `GenericAudioProcessorEditor` placeholder. Port of O-simpleFM minus oversampling/voices/viz/preset.

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

1. **Phase 2.3: Spectral-decay + bit-depth + viz tap** — per-partial exponential tilt `D_k = exp(−rate·k·tau)` (composes into `refillTable()` after morph, before band-limit), discrete bit-depth quantizer at read time, and the lock-free `VizRing` + active-spectrum snapshot tap. → `/clear` then `/implement O-simpleAdditive`
2. Stage 3: GUI (3 phases) · Stage 4: Validation/Polish
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
