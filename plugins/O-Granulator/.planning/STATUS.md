---
plugin: O-Granulator
stage: 2
status: stage_2_phase_2_3_execute_complete
current_phase: execute
current_dsp_phase: 2.3
stage_0_status: ui_design_complete
last_updated: 2026-11-05
latest_mockup_version: 2
mockup_finalized: true
finalized_version: 2
brief_updated_from_mockup: true
complexity_score: 5.0
complexity_tier: 6
staged_implementation: true
orchestration_mode: true
next_action: stage_2_phase_2_3_verify
next_stage: 2
ready_for_implementation: true
blocked_on: null
contract_checksums:
  brief: sha256:f9c748a8a256de9d2fb2bcbe1aff58b12999a640cc8d36fbdbf96e6deb5559d7
  requirements: sha256:6530b7858f6866e47370535980c84ae8dec2d015b7456dd0109505245e19f40e   # 2.2 verify: +FUNC-03, DSP-01/03/04 complete; QUAL-01, PERF-01/02/03 partial
  parameter_spec_draft: sha256:d0b1457ea7c1624ff757fd12c1acb7bd62c9acf7bebbdd3460c7f1d30cdbbce3
  parameter_spec: sha256:f5ce35954c2c18e46faa914d95898ddb718d6b77ec4be8582be23b60a26b3688   # P3 uiLanguage amendment (Stage 1)
  architecture: sha256:f0cf753b4de2cb6099ad327b0887a0c254f013746b373000a28f01ad7949d9d9
  roadmap: sha256:79d69576f70776e92ce498be990e1f3cbdc09e03a081563443545f9650c14b20
---

# O-Granulator Status

## Current Position

Stage: 2 of 4 (DSP) — Phase 2.1 (Ring + pass engine + one stream) discuss ✓ · research ✓ 2026-11-04 (RESEARCH-2.1.md: R1 read-fraction quantisation for the bitwise null, R2 0 dB snap + mix-0 bypass, R3 plugin-side grain_limit cap, R4 onset epoch across re-enable) · plan ✓ 2026-11-04 (PLAN-2.1.md: 17 tasks, 5 waves, 13 gate rows) · execute ✓ 2026-11-04 (SUMMARY-2.1.md: engine skeleton in Source/dsp/, harness 77/77 PASS, 0 warnings, auval + pluginval 10 VST3/AU PASS; D3 listening checkpoint pending at verify) · verify ✓ 2026-11-04 (VERIFICATION-2.1.md: independent re-run 77/77, pluginval 10 VST3/AU + auval PASS, D3 listening PASS; FUNC-01/07 + DSP-02 complete, 5 partial). Phase 2.2 discuss ✓ 2026-11-04 (CONTEXT-2.2.md: D4 GS-parity quantiser — uniform pick among active degrees within ±pitch_rand, base pitch unsnapped, ARCHITECTURE §5 erratum; D5 gain_rand cut-only −24 dB, no make-up) · research ✓ 2026-11-04 (RESEARCH-2.2.md: R5 derived partial-coherence output law replaces the κ dB-blend — jitter does not decorrelate, base pitch + reverse do; 2.1 law +19.7 dB hot at spray 0 × 200 g/s × 1 s; R6 cumulative ∫w/∫w² bank tables, worst capped P/μ₂ 333; R7 floor domain for level gates; R8 overQuota = live > ceil(q); R9 mask-by-source-index then sort; R10 spread = per-grain balance × bus balance; R11 sine-probe traps; R12 2.1-equivalence out-of-tree null) · plan ✓ 2026-11-05 (PLAN-2.2.md: 18 tasks / 7 waves, 15 gate criteria; T1 steal-free row lands first on the 2.1 tree; P10 R5 law, P11 custom-table try-lock, P12 10 ms mixer ramps, P15 solo probes) · execute ✓ 2026-11-05 (SUMMARY-2.2.md: 4 streams / water-fill quotas / R8 fair stealing / 65-step window bank / D4 quantiser / R5 law / 10 ms mixer; harness 135/135 PASS, 0 warnings; R12 out-of-tree null PASS both cases; auval + pluginval 10 VST3/AU PASS; CPU 9.01 % @ 48k/128; errata: S0n cap 64, F3 cap-128 steals, W2o scope narrowed) · verify ✓ 2026-11-05 (VERIFICATION-2.2.md: independent re-run 135/135, R12 null max|Δ| 0 both cases, auval + pluginval 10 VST3/AU PASS, CPU 8.92 %; errata 6–8 accepted, W2o margin 0.017 dB flagged fragile; FUNC-03, DSP-01/03/04 complete, QUAL-01 partial). Phase 2.3 discuss ✓ 2026-11-05 (CONTEXT-2.3.md: D6 drive unity at −12 dBFS ref, r·tanh(dx)/tanh(dr), d = 16·drive² → continuous from 0; D7 crush mid-tread (silence stays 0); D8 BP = k·v1 normalised, LP/HP raw, output law ignores FX — three ARCH §9 errata; no listening checkpoint) · research ✓ 2026-11-05 (RESEARCH-2.3.md: R13 segments split at host-block boundaries → every FX op per sample, gather→FX→scatter kernel; R14 ring drift worst at LOW freq, per-pass renorm −115 dB at 5 Hz → per-sample Newton renorm ≤ −135 dB; R15 D6 is peak-unity (RMS +2.24 dB at full drive) → peak gate; R16 guard d < 1e-4; R17 census + BP-centroid/ring-carrier audio rows with fxSharedLatch NC (ρ 0.998 vs 0.17); R18 BP k·v1 exactly 0 dB; R19 LP/HP res 100 % → wet up to +38 dBFS pre-safety (2.4 item); R20 reserved slots FxCutoff/FxCrushProb/FxRing; R21 all FX +4–5.5 % core → ≈ 13–15 % worst; R22 crush 100 % silences < −12 dBFS) · plan ✓ 2026-11-05 (PLAN-2.3.md: 14 tasks / 7 waves, 49 gate rows + 7 INFO, harness target 175; P17 N0 neutral-FX null vs c10d0f2c first in tests/n0/; P18 one gather→FX→scatter kernel; P21 per-sample ring renorm ≤ −135 dB; P22 reserved draw slots, no 2.2 draw moves; P23 Padé drive shaper needs sign-off; P24 crush nearbyint + crushHoldPhase default 1; P26 C3 CPU phase-fail > 30 %) · execute ✓ 2026-11-05 (SUMMARY-2.3.md: GrainFx.h SVF/drive/crush/ring latched per grain, single gather→FX→scatter kernel + P25 NaN kill; harness 175/175 PASS (135 carried, diff empty; W2o +0.483 dB unchanged), 0 warnings; N0 --strict PASS (all four N0n differ), R12 PASS; CPU C3w 17.33 % / C3t 7.31 % (budget 30 / 10), C2′ 9.11 % → P18 lever not applied; auval + pluginval 10 VST3/AU PASS; 11 execution deviations recorded). Next: Phase 2.3 verify
Status: O-Granulator 0.1.0 shell builds VST3 + AU + Standalone; 220 params, pass-through, state codec; pluginval 10 + auval PASS. pitch-collections 1.0.0 module and O-GrainScatter 3.6.0 landed with it.
Progress: [#####...............] 25% (Stage 1 verified — VERIFICATION.md)

## Completed So Far

**Ideation:** ✓ Complete. Brief, 30 requirements, and a draft param spec (214).

**UI mockup:** ✓ v2 FINALIZED (2026-11-04). `.planning/mockups/v2-ui.yaml` is the UI source of truth (`finalized: true`); BRIEF.md UI Concept synced from it (Phase 5.6). It covers all 220 params, incl. full microtonal scales modelled on O-GrainScatter v3.0.0: `sN_scale` has 32 choices (Off, GrainScatter's collections without Free, Custom last), a grouped dropdown, per-stream degree chips and ONE custom scale shared by all streams (state, not a param).

**Implementation scaffolding (Phase B):** ✓ `mockups/v2-ui.html` (production page: Family A knobs, canonical i18n en/fr/zh-Hans, hover-help switch), `v2-i18n.js`, `v2-PluginEditor.h/.cpp` (220 relays → webView → 220 attachments, 21 native fns, Windows WebView2 block), `v2-CMakeLists.txt` (complete plugin CMakeLists), `v2-integration-checklist.md`. Pre-verified on a scratch tree: check-i18n ALL PASS, fr lint clean, zh lint 0 findings, check-ui-labels 0 failures (all pages/popovers forced visible), boot-all-uis --strict-tips clean.

**Parameter contract:** ✓ `.planning/parameter-spec.md` LOCKED — 220 params (140 Float / 59 Choice / 21 Bool), 113-entry append-only mod-destination table, non-parameter state (shared custom scale, per-stream × per-collection uint64 degree masks, seed, table version, uiLanguage), PitchCollections → shared module `modules/tuning/pitch-collections` (provisional).

**Stage 0:** ✓ Research & Planning complete. ARCHITECTURE.md and ROADMAP.md are documented (Complexity 5.0, raw 22.0, tier 6, DEEP).
- Plugin type: Audio Effect (live-input granular, 4 streams, shared pool)
- Professional references: Clouds/Beads, Granulator III, Efx Fragments, Output Portal, Bitwig/Ableton modulators
- JUCE: juce_audio_utils, juce_dsp (FFT for MPM, ReverbUnit), WebView; grain-engine v1.1.0 (MipRing<2>, GrainPool, GrainScheduler, CounterRng, WindowLut); reverb-unit v1.1.0; preset-manager
- Strategy: phased. Stage 2 has 6 phases, Stage 3 has 3.

## Next Steps

1. Stage 2: DSP Phase 2.3 verify (`/plugin-verify O-Granulator 2-dsp`) — SUMMARY-2.3.md (175/175, N0 strict, C3w 17.33 %). Independent re-run: harness, `tests/n0/run.sh --strict`, `tests/r12/run.sh`; carried-row diff must exclude the unseeded B2 "defaults" rows (deviation 4). Carry to 2.4 discuss: R19 resonant headroom vs wet safety, R22 crush silence threshold. Carry: W2o 0.017 dB margin — widen seeds, never the threshold, if 2.3 draw-order changes flip it.
2. Stage 3 integrates the UI with `mockups/v2-integration-checklist.md` (+ check-ui-labels, deferred from Stage 1).

## Context to Preserve

- Architecture: `plugins/O-Granulator/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-Granulator/.planning/ROADMAP.md`
- Stage 0 decisions: `plugins/O-Granulator/.planning/stages/0-ideation/CONTEXT.md`
- Key decisions:
  - Absolute 32-sample pass grid with a 32-sample feedback return (write-before-read).
  - Freeze is a head-stop and wins over feedback (no overdub in v1.0).
  - Coherent feedback-tap law plus 0.95 cap plus tanh plus AGC plus wet safety.
  - MPM pitch follower on the audio thread.
  - Append-only 113-destination matrix.
  - Reported latency is 0.
  - CPU budget: ≤ 30 % of one core @ 48 k/128 worst case.
- Module gaps (optional): G1 WindowMorphBank, G2 GrainPool::fadeOut, G3 GrainScheduler::rescheduleIn, G4 ReverbUnit raw-atomic bind (critical path; fallback is a local copy)

## Files Created
- plugins/O-Granulator/.planning/parameter-spec.md (LOCKED)
- plugins/O-Granulator/.planning/mockups/v2-ui.html, v2-i18n.js, v2-PluginEditor.h, v2-PluginEditor.cpp, v2-CMakeLists.txt, v2-integration-checklist.md
- plugins/O-Granulator/.planning/research/ARCHITECTURE.md
- plugins/O-Granulator/.planning/ROADMAP.md
- plugins/O-Granulator/.planning/stages/0-ideation/CONTEXT.md

## Phase Progress

### Stage 2: DSP — Phase 2.1 (Ring + pass engine + one stream)
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-11-04 | |
| research | → | | |
| plan | | | |
| execute | | | |
| verify | | | |

Decisions: grain-engine 1.1.0 unmodified (G2/G3 local workarounds, promote after ship); ring 41 s at all rates (no ≥96 k clamp); human listening at end of 2.1 and 2.4 only. Stage 1 harness B1–B3 rescope to mix 0 % (mix default 50). See `stages/2-dsp/CONTEXT-2.1.md`.

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-11-04 | |
| research | ✓ | 2026-11-04 | |
| plan | ✓ | 2026-11-04 | |
| execute | ✓ | 2026-11-04 | |
| verify | → | | |

Decisions: PitchCollections extract + migrate O-GrainScatter (→3.6.0) in Stage 1; GenericAudioProcessorEditor shell; all three bus layouts. See `stages/1-foundation/CONTEXT.md`.

Execute (2026-11-04): see `stages/1-foundation/SUMMARY.md` — W1 pitch-collections 1.0.0 (229/229), W2 GS 3.6.0 (pitch-null 415/415, 190 renders byte-identical), W3 shell (render harness 47/47 + golden negative control), W4 pluginval 10 VST3 ×3 + AU, auval PASS.
