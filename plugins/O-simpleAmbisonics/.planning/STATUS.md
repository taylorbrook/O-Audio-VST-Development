---
plugin: O-simpleAmbisonics
stage: 2
stage_name: dsp
stage_phase: 1
stage_phase_name: chain-and-stereo
stage_phase_total: 3
artifact_suffix: "-2.1"
status: stage_2_phase_2_1_research_complete
last_updated: 2026-10-12
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
workflow_mode: manual
current_phase: research
next_phase: plan
next_action: plugin-plan
next_stage: 2
ready_for_implementation: true
parameter_count: 12
mockup_finalized: false
plugin_code: OSAm
---

# O-simpleAmbisonics Status

## Current Position

Stage: 2 of 4 (DSP) — Phase 2.1 of 3 (chain + stereo) — research ✓ (2026-10-12)
Status: Phase 2.1 RESEARCH-2.1.md written: R1–R7 answered with computed numbers (click metric = second-difference ratio ≤ 2.0 vs matched control on a 100 Hz tone; allocation counter = O-Octagon probe AO verbatim; chord error −1.3/−3.75/−5.56 dB per band at 4096-sample slices → D5 AMENDMENT: fixed 64-sample control grid, which also makes moving-parameter renders bit-identical across slicing; DSP-01 generic table verified to 4.7e-16 / float 2.8e-08; DSP-05 two-tone 200 Hz + 12 kHz with closed-form anchors, flat on [0.2, 1] by design; TripleBuffer not a module — copy with provenance; peaks = max-since-UI-last-read via hasNewData()). Findings F1–F9: one-poles never reach exact 0/1 → linear ramps for order gains + decoder mix and a 1e-6 snap; mixer/lerp form b + t·(a − b) so the D6 placeholder and the P1 anchor are bit-exact; D7 motivation corrected (TPT pole at 0.45·fs is −0.06 dB at 10 kHz, not −0.85). Next: 2.1 / plan.
Stage 1 was: Stage 1 complete. Verify re-ran everything independently on the INSTALLED bundles (byte-identical to the build): harness P0–P5 ALL PASS (digest 6a1df555bc125ae3 reproduced), 0 warnings on a forced TU rebuild, auval SUCCEEDED (caps [1,2] [2,2]), pluginval s10 SUCCESS VST3 + AU, pedalboard V1–V3 (12-param contract, pass-through with reported == measured latency, 12-value state round-trip) ALL PASS. COMPAT-01 + FUNC-07 complete. Next: Stage 2 / discuss (Phase 2.1).
Progress: [#########...........] 42%

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-11 | |
| research | ✓ | 2026-10-11 | |
| plan | ✓ | 2026-10-12 | |
| execute | ✓ | 2026-10-12 | |
| verify | ✓ | 2026-10-12 | |

### Stage 2: DSP — Phase 2.1 (chain + stereo)
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-12 | |
| research | ✓ | 2026-10-12 | |
| plan | | | |
| execute | | | |
| verify | | | |

### Stage 2: DSP — Phase 2.2 (rotation) · Phase 2.3 (binaural)
Not started. Per-phase cycles (D1); roll-up `stages/2-dsp/VERIFICATION.md` at the 2.3 verify.

## Completed So Far

**Ideation:** ✓ Complete
- Core concept, architecture, parameters, UI vision, use cases, 27 requirements with acceptance criteria.

**Stage 0:** ✓ Complete — Research & Planning (2026-10-11)
- Plugin type: audio effect, stereo/mono in → stereo out, 12 parameters, WebView UI
- Prior art reused: O-GrainScatter `SpatialEncoder.h` (encoder), `TripleBuffer.h`, header-generation pipeline; both in-house ambisonics deep-dives
- JUCE 8.0.15 classes verified in local source: `dsp::FFT`, `Interpolators::WindowedSinc`, `dsp::FirstOrderTPTFilter`, `SmoothedValue`; `dsp::Convolution` read and rejected for the 16→2 topology
- Eight research questions resolved (see `stages/0-ideation/CONTEXT.md`): MagLS from SADIE II KU100 (Apache-2.0), fixed-partition UPOLS P=64 → constant 64-sample latency, Ivanic–Ruedenberg rotation, soft-knee distance + air LP, always-running decoders with 20 ms crossfades, FIFO block-size independence, wrap-aware smoothing, ±60° virtual-cardioid stereo pair, 12-parameter v1.0 set (spread / weighting / solo deferred to v1.1)
- Complexity score: 5.0 (capped; raw 10.0) → staged implementation (DSP 3 phases, GUI 3 phases)
- ROADMAP covers every must-priority requirement by ID with a named phase and gate

**Stage 1 / discuss:** ✓ Complete (2026-10-11)
- `stages/1-foundation/CONTEXT.md` — foundation scope: effect CMake (VST3 + AU + Standalone, WebView2 flags, `juce_dsp`), mono/stereo-in → stereo-out buses, 12-param APVTS, pass-through `processBlock` (input conditioner → 64-sample delay → seeded output gain), `setLatencySamples(64)`, placeholder editor, `Source/dsp/SpatialEngine.h` seam, `tests/render-harness/` scaffold
- Decisions: `PLUGIN_CODE OSAm` (Stage 0's `OSiA` is taken by O-simpleAdditive); proceed without `parameter-spec.md` (sibling precedent — ARCHITECTURE.md §Parameter Mapping is the authoritative 12-param contract; the mockup may not add parameters)

**Stage 1 / research:** ✓ Complete (2026-10-11)
- `stages/1-foundation/RESEARCH.md` — templates pinned: O-simpleSubtractive `CMakeLists.txt` + Stage-1 APVTS skeleton (commit 62d4c0e8), O-Emulator Stage-1 effect shell + `tests/render-harness` scaffold with derived version code (commit 8ec32da9), O-Orbit mono/stereo-in → stereo-out bus check
- Pass-through chain designed from JUCE 8.0.15 source: `setLatencySamples(64)` once in `prepareToPlay`; recommended `Source/dsp/FixedDelay.h` (`FixedDelay<kPartition>`, std::array, zero alloc, survives into Stage 2's `DecoderMixer`) over `dsp::DelayLine`; `LinearSmoothedValue` output gain seeded via `setCurrentAndTargetValue(decibelsToGain(dB, −60))`; slice loop for oversized host blocks; finite scrub
- 12-parameter construction table verified (`setSkewForCentre(2 m)` / `(0.2 Hz)` — skews transmit to the WebView frontend, lambda ranges do not); JUCE headers scanned — no `juce::` name collisions with any of the 12 IDs
- Editor recommendation: O-Emulator shape (inline `GenericAudioProcessorEditor`, harness at `JUCE_WEB_BROWSER=0`); Stage-1 harness probes P0–P5 specified (contract, pass-through bit-equal + no fade-in, Left-only + mono-in, ragged blocks, oversized block, impulse latency = 64, −60 dB silence)
- Verify tooling: pluginval strictness 10 (CI parity), targeted `auval -v aufx OSAm OuDv`

**Stage 1 / plan:** ✓ Complete (2026-10-12)
- `stages/1-foundation/PLAN.md` — T1 CMake (effect, `OSAm`, no BinaryData) · T2 `Source/dsp/{SpatialEngine,FixedDelay}.h` seam with `kPartition = 64` · T3 processor (buses, 12-param table, seeded pass-through with slice loop, XML state + `pluginVersion`) · T4 inline `GenericAudioProcessorEditor` (no editor TU) · T5 `tests/render-harness` scaffold from O-Emulator 8ec32da9 with probes P0–P5 + FNV digest · T6 build / harness / build-and-install / `auval -v aufx OSAm OuDv` / pluginval s10 ×2 · T7 SUMMARY + status
- Success criteria mirror CONTEXT.md; gotchas checklist from RESEARCH §8 embedded for the executor

**Stage 1 / execute:** ✓ Complete (2026-10-12)
- `stages/1-foundation/SUMMARY.md` — files: `CMakeLists.txt` (effect, `OSAm`, VST3+AU+Standalone, WebView2 flags, no BinaryData), `Source/PluginProcessor.{h,cpp}` (12-param APVTS, mono/stereo-in → stereo-out, pass-through conditioner → `FixedDelay<64>` → seeded gain, slice loop, finite scrub, Generic editor inline, XML state + `pluginVersion`), `Source/dsp/{SpatialEngine,FixedDelay}.h` seam (`kPartition = 64`, no-op engine), `tests/render-harness/{CMakeLists.txt,main.cpp}` (derived version, probes P0–P5 + digest)
- Gate 0→1 bypassed with `--force` (no source tree yet — standing pattern; logged)
- Harness: P0 (45 checks) / P1 / P1b / P2 / P3 / P4 (impulse 100 → 164) / P5 ALL PASS; digest `6a1df555bc125ae3`
- Build 0 warnings; `build-and-install.sh` OK; `auval -v aufx OSAm OuDv` SUCCEEDED; pluginval strictness 10 ×2 SUCCESS for VST3 and AU
- **Deviation:** the five degree parameters use `NormalisableRange` interval **0** (continuous) instead of 0.01 — JUCE snaps every host value to `start + interval·floor(…)`, and `−180 + 0.01f·18000 = −4.02e-06`, so the 0° default was never exactly 0 (P0 caught it). Siblings carry the same latent offset unobserved.
- Build-time corrections: namespace alias `osa_dsp` (bare `dsp::` ambiguous with `juce::dsp` under JuceHeader's `using namespace juce`); input legs via `getChannelLayoutOfBus(true, 0)`

**Stage 1 / verify:** ✓ Complete (2026-10-12)
- `stages/1-foundation/VERIFICATION.md` — ✅ VERIFIED, every SUMMARY claim re-run independently on the installed bundles; COMPAT-01 and FUNC-07 → complete in REQUIREMENTS.md
- Independent measurer: pedalboard on the installed VST3 — 12 params with contract ranges/defaults/choices; stereo noise → both outputs == 0.5·(L+R) 8192/8192 exact with residual lag 0 after host latency compensation (reported 64 == actual); impulse amplitude exactly 1.0; 12-value `raw_state` round-trip restores every decoded value
- Test-harness lesson (not a defect): a live VST3 instance's `raw_value` is the wrapper's cached un-snapped host float — round-trip gates compare DECODED values (memory written)
- Human DAW checks listed as non-blocking; COMPAT-02 (Stage 4) repeats them in Logic + Live

**Stage 2 / Phase 2.1 / discuss:** ✓ Complete (2026-10-12)
- `stages/2-dsp/CONTEXT-2.1.md` — D1 three per-phase cycles; D2 Stage-1 engine seam kept (processor owns conditioner + TripleBuffer, publish once per host block); D3 2.1 headers JUCE-free; D4 own TPT one-pole with per-sample-lerped G; D5 block-rate smoothers keyed to the slice length; D6 binaural leg = stereo placeholder (2.3 obligation); D7 air-LP wet mix m(d) = clamp(d − 1, 0, 1); D8 P1 → reference null, front/1 m/order 1/Stereo = exactly 0.75·s[n−64]; D9 gate discipline (ratios, fresh instance, analytic goldens, alloc counter)
- Open for research: R1 click metric, R2 alloc-counter precedent, R3 lerp chord error at 4096-sample orbit, R4 independent DSP-01 table, R5 HF/LF ratio bands, R6 TripleBuffer module status, R7 meter publish cadence
- External risks retired early: SADIE II D1 HRIR SOFA at Zenodo 12092466 (38.2 MB, apache2.0, HTTP 200); `sofar` 1.3.0 resolves via `uv run --with sofar`

**Stage 2 / Phase 2.1 / research:** ✓ Complete (2026-10-12)
- `stages/2-dsp/RESEARCH-2.1.md` — R1 click metric: worst |second difference| over the event vs a matched static control, ratio ≤ 2.0 (O-Octagon v1.15.0 `worstCurvature`), 100 Hz tone (a 1 kHz tone cannot see a 64-block zipper: step/2nd-diff 0.24 vs 24 at 100 Hz), negative control = the harness reference model with per-block snap, liveness half-circle ≥ 6 dB; R2 allocation counter: O-Octagon `main.cpp:111-241` verbatim (8 new/delete variants, thread-filtered, `foreignNote()`), pre-sized buffers + warm-up block + liveness `new`, standing coverage statement; R3 chord error at D5's slice-rate control: −1.31 / −3.75 / −5.56 dB per band at 4096-sample slices @ 2 Hz → **D5 amendment proposed: fixed 64-sample control grid** (≤ 0.002 dB, constant k = 0.0645, renders bit-identical across slicing with moving parameters → new P2b/P3b gates); R4 generic SN3D table (factorials + §2.5 Legendre) vs `encodeSH16`: 4.7e-16 double, 2.8e-08 float — AmbiX sign/ACN confirmed; R5 DSP-05 = two-tone 200 Hz + 12 kHz exact-bin DFT, closed-form bilinear anchors (0 dB at ≤ 1 m, −10.79 dB at 20 m; level +8.87 / 0 / −24.19 dB), ratio flat on [0.2, 1] and strictly decreasing on (1, 20] — criterion reworded; R6 TripleBuffer not in `modules/registry.yaml` (copies in O-GrainScatter + O-simpleGrain) → copy with provenance, module candidate after Stage 2; R7 publish once per host block, peaks accumulate until the reader consumed (`hasNewData()` false → reset), engine owns accumulation
- Findings: F1 one-pole smoothers never reach exactly 0/1 (1.75 s to FTZ) → order gains + decoder mix as 20 ms linear ramps, one-poles snap at 1e-6; F2 mixer/lerp form `b + t·(a − b)` (D6 placeholder exact in Binaural mode); F3 P1 anchor bit-exact with literal `0.25f`/`0.4330127f` matrix; F4 `wrapTo180` is [−180, 180); F5 `Params` filled before `engine.prepare`, every smoother snapped; F6 engine-direct gates under `ScopedNoDenormals`; F7 DSP-04 margin 22.9 dB vs 20, non-negative for all directions incl. elevation; F8 P2b/P3b orbit-On slicing invariance; F9 `tests/render-harness/ReferenceModel.h`
- Correction recorded: D7's "−0.85 dB at 10 kHz" is the analog pole; the TPT (bilinear) pole at 0.45·fs is −0.06 dB at 10 kHz — D7's wet mix stands for exactness, not for tone

## Next Steps

1. Stage 2 / Phase 2.1 / plan: `/plugin-plan O-simpleAmbisonics 2-dsp` — adopt the D5 amendment (64-sample control grid) and F1–F3 forms, write the task list from RESEARCH-2.1 §Open items 1–10, produce `PLAN-2.1.md`
2. Non-blocking human checks from `stages/1-foundation/VERIFICATION.md` (Logic stereo + mono track, Generic editor 12 rows, project save/reopen) — repeated formally under COMPAT-02 at Stage 4
3. UI mockup (`/start O-simpleAmbisonics` → option 3) before Stage 3; it finalises `parameter-spec.md` and must keep the 12-parameter set

## Context to Preserve

**Key decisions (contract — see ARCHITECTURE.md Architecture Decisions §1–10):**
- Binaural = MagLS, SADIE II D1 (KU100), 256 taps @ 48 kHz, three per-order sets in a generated constexpr header `Source/dsp/BinauralFilterData.h` (no second BinaryData target); runtime fixed-partition UPOLS P=64 on `juce::dsp::FFT`; **latency 64 samples constant** in both modes at all sample rates
- Rotation = Ivanic–Ruedenberg recursion; `R = Rz(yaw)·Ry′(pitch)·Rx(roll)`; yaw + = scene CCW from above, pitch + = front rises, roll + = left rises
- Distance: `g = 1.25/(d+0.25)`, air LP `f_c = 20 kHz·min(1, 1/√d)` clamped to 0.45·fs, pre-encode
- Stereo decode: ±60° first-order cardioid pair on rotated W/Y/X, delayed 64; decoders always run; 20 ms crossfades everywhere
- Smoothing: wrap-aware one-pole (τ 20 ms) at block rate + per-sample lerp of coefficient vector / rotation matrix
- Meters: post-rotation `shGain = R·c_eff` (signed) + peaks via `TripleBuffer<MeterFrame>`; polar plot from `order` in JS
- Mono sum = 0.5·(L+R); presets never set `outputLevel`
- `PLUGIN_CODE OSAm`

**Open until later stages:**
- Mockup (Stage 3 input) — no mockup exists yet; `parameter-spec.md` to be produced at finalisation
- SADIE II SOFA download + `sofar` availability at Phase 2.3 (fallback: synthetic spherical-head generator)

**Sibling references:** O-simpleSubtractive (editor/CMake/harness template), O-simpleWavetable (latest WebView + gate conventions), O-simpleFM (viz Timer), O-GrainScatter (encoder, TripleBuffer, generator, fallback decoder); a suite stereo effect (O-Freeze / O-Gain) for effect bus boilerplate.

## Files Created
- plugins/O-simpleAmbisonics/.planning/BRIEF.md (ideation)
- plugins/O-simpleAmbisonics/.planning/REQUIREMENTS.md (ideation)
- plugins/O-simpleAmbisonics/.planning/parameter-spec-draft.md (ideation)
- plugins/O-simpleAmbisonics/.planning/research/ARCHITECTURE.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/ROADMAP.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/stages/0-ideation/CONTEXT.md (Stage 0)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/CONTEXT.md (Stage 1 discuss)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/RESEARCH.md (Stage 1 research)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/PLAN.md (Stage 1 plan)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/SUMMARY.md (Stage 1 execute)
- plugins/O-simpleAmbisonics/.planning/stages/1-foundation/VERIFICATION.md (Stage 1 verify)
- plugins/O-simpleAmbisonics/CMakeLists.txt (Stage 1 execute)
- plugins/O-simpleAmbisonics/Source/PluginProcessor.h, PluginProcessor.cpp (Stage 1 execute)
- plugins/O-simpleAmbisonics/Source/dsp/SpatialEngine.h, FixedDelay.h (Stage 1 execute)
- plugins/O-simpleAmbisonics/Source/ui/.gitkeep (Stage 1 execute)
- plugins/O-simpleAmbisonics/tests/render-harness/CMakeLists.txt, main.cpp (Stage 1 execute)
- plugins/O-simpleAmbisonics/.planning/stages/2-dsp/CONTEXT-2.1.md (Stage 2 / 2.1 discuss)
- plugins/O-simpleAmbisonics/.planning/stages/2-dsp/RESEARCH-2.1.md (Stage 2 / 2.1 research)
