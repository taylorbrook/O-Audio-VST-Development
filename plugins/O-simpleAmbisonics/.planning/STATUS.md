---
plugin: O-simpleAmbisonics
stage: 2
stage_name: dsp
stage_phase: 2
stage_phase_name: rotation
stage_phase_total: 3
artifact_suffix: "-2.2"
status: stage_2_phase_2_2_research_complete
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

Stage: 2 of 4 (DSP) — Phase 2.2 of 3 (rotation) — research ✓ (2026-10-12) → plan next
Status: Phase 2.2 research complete (RESEARCH-2.2.md): R1 the Ivanic–Ruedenberg recursion (1996 + 1998 erratum) transcribed in code form and VALIDATED in a scratch script — 1.8e-15 double / 1.3e-7 float32 against re-encoding at 200 random tuples at orders 1–3, orthogonality 1.2e-15, transposed convention fails by 1.24, yaw-only closed form agrees, index map p = (1, 2, 0) and the P/U/V/W structure confirmed against IEM SceneRotator (read only); R3 identity EXACT in float32 element by element (D3 digest verdict stands; signed-zero caveat recorded); R2 chord dip −0.41 dB band 3 at 11.6°/grid, −0.001 dB at 0.48°/grid, axis-independent — BUT a 180° yaw step reads 12/25/44× vs a static control on bands 1/2/3 (grid-boundary kinks of the one-pole + lerp, identical to the 2.1 azimuth path, engine == same-motion reference 1.0000) → step gate verdict vs the same-motion ReferenceModel, sweep gate vs static parks (1.13 expected); R4 dir() round-trips to 9e-13°, azRot is noise at the pole → pitch/roll cardinal checks assert elRot only; R5 offsets 0/9/34, rotIndex(l,m,n), no scratch; R6 start the sweep AT the prepared value (the 2.1 DSP-06b idiom hides a 180° step — 4.6× false FAIL at band 3), front source leaves 6 channels dead → click gate at az 37/el −12, liveness (8.2 dB) + yaw≡azimuth cross-gate on a front source; R7 no new drivers, one tap16 helper. F1 HIGH: 2.1 probe asserts azRot == azEff bitEqual and atan2 does not round-trip in float → identity fast path for the frame direction. Previous: Phase 2.2 discuss complete (CONTEXT-2.2.md): the algorithm is closed by ARCHITECTURE (matrices, signs, Euler composition, Ivanic–Ruedenberg recursion, per-sample matrix lerp, DSP-02 gate); discuss settled ten execution-level decisions — D1 rotate the 16-channel bus per sample (83 multiplies, W implicit 1), not folded into c_eff; D2 per-element lerp prevR + t·(curR − prevR) on the control grid, all blocks always applied; D3 zero rotation is BIT-TRANSPARENT so the whole 2.1 probe set and digest 9189d911a1fe98d5 become the 2.2 regression verdict (digest-2.1) with a separate digest-2.2; D4 three independent WrapSmoothers; D5 tap + shPeak move POST-rotation; D6 ReferenceModel stays recursion-free (own 3×3 builder, re-encode at dir(R·d) = the oracle by construction); D7 cardinal signs + yaw = −azimuth pinned through the engine frame against the harness builder; D8 azRot/elRot = dir(R·d) per control step; D9 own transcription of the 1996 + 1998-erratum formulas, IEM/Green for index cross-check only; D10 yaw-only fallback stays armed. Open for research R1–R7 (formulas validated numerically in a scratch script BEFORE planning, lerp chord error per band at 11.6°/0.48° per grid, identity exactness element-by-element, dir() convention, 83-element index helper, sweep liveness, hook automation of the angles). Next: 2.2 / research.
Verify was: Phase 2.1 VERIFIED (VERIFICATION-2.1.md): every SUMMARY claim re-run independently on the installed bundles (byte-identical to the build) — harness 118/118 PASS with digest 9189d911a1fe98d5 reproduced, 0 warnings on a forced TU rebuild, auval SUCCEEDED, pluginval s10 ×2 VST3 + AU SUCCESS, and an independent pedalboard measurer (tests/pedalboard/verify_2_1.py, 25 checks ALL PASS). REQUIREMENTS: FUNC-02, FUNC-06, DSP-01, DSP-04, DSP-05, DSP-06, PERF-01 → complete; FUNC-01, FUNC-03, QUAL-01 → partial (rotation 2.2, HRTF/decoder-switch 2.3, preset-load Stage 4). Human DAW hands-on listed as non-blocking.
Execute was: Phase 2.1 executed (T1–T12, SUMMARY-2.1.md written): the audible chain — DistanceModel (soft-knee gain + TPT air LP with the D7 wet mix) → ShEncoder (encodeSH16 lifted verbatim, LinearRamp order gates) → identity rotation slot → 16-ch bus with busTap + peak scan → ±60° StereoDecoder (exact literals) → DecoderMixer (FixedDelay<64> ×2, mix ramp, A14 placeholder explicit at the engine call site) — on the persistent 64-sample control grid (D5 amendment as A1). Processor caches all 12 atomics, fills Params before engine.prepare, slices through monoScratch, applies the seeded gain per sample and publishes one MeterFrame per host block through a public TripleBuffer with the hasNewData()-gated peak reset. Harness: 118 checks ALL PASS on a fresh binary ×4 — P1 null 4.6e-08 vs the double ReferenceModel, P1 anchor bit-exact 0.75·s[n−64] in both decoder modes + engine-direct, P2b/P3b bit-equal with orbit On 2 Hz / order 3, P4 impulse exactly 0.75 at 164, DSP-01 2.07e-07, DSP-04 22.88 dB, DSP-05 anchors −10.788 / +8.874 / 0.000 / −24.198 dB with d ≤ 1 m bit-exact, DSP-06a through 180° (residual 8.4e-04 at 0.2 s), DSP-06b click ratios 1.0003 / 1.0000 (negative control 24.8 FAIL-as-expected, liveness 8.35 dB), order fade exact arrival in 14 / 15 steps with bus-tap ratios 0.9999 / 1.0000 (negative control 66.9), meter frame 4/9/16 + accumulate-then-reset, extremes finite at 48 k / 96 k, PERF-01 0 allocations (liveness 1); digest 9189d911a1fe98d5. Build 0 warnings; build-and-install OK; auval SUCCEEDED; pluginval s10 ×2 VST3 + AU SUCCESS. One real defect found and fixed in the loop: the APVTS raw default of the skewed+stepped distance range is unsnapped (JUCE legacy) → 1.0000001 m → fillParams now snaps distance / orbitRate through their ranges. Next: 2.1 / verify.
Plan was: Phase 2.1 PLAN-2.1.md written: 13 tasks (T1–T13) with decisions A1–A15 adopted from RESEARCH-2.1 — the D5 amendment (persistent 64-sample control grid, constant k = 1 − exp(−64/(τ·fs)) and ramp step 64/(τ·fs)), linear ramps for order gains + decoder mix with exact arrival, 1e-6 snap on the one-poles, every blend as b + t·(a − b), decoder matrix as exact literals 0.5f/0.25f/0.4330127f, per-sample lerp t = (j+1)/64, Params filled before engine.prepare, engine-owned peak accumulation with a hasNewData()-gated reset, D6 placeholder explicit at the engine call site. The control step is written once as a shared spec for SpatialEngine (T7) and the double-precision ReferenceModel (T9, generic SN3D table, snap switches for the negative controls).
Research was: R1–R7 answered with computed numbers (click metric = second-difference ratio ≤ 2.0 vs matched control on a 100 Hz tone; allocation counter = O-Octagon probe AO verbatim; chord error −1.3/−3.75/−5.56 dB per band at 4096-sample slices → D5 AMENDMENT: fixed 64-sample control grid; DSP-01 generic table verified to 4.7e-16 / float 2.8e-08; DSP-05 two-tone 200 Hz + 12 kHz with closed-form anchors, flat on [0.2, 1] by design; TripleBuffer not a module — copy with provenance; peaks = max-since-UI-last-read via hasNewData()). F1–F9: linear ramps for order gains + decoder mix and a 1e-6 snap; b + t·(a − b) forms; D7 motivation corrected (−0.06 dB at 10 kHz at the clamp).
Stage 1 was: Stage 1 complete. Verify re-ran everything independently on the INSTALLED bundles (byte-identical to the build): harness P0–P5 ALL PASS (digest 6a1df555bc125ae3 reproduced), 0 warnings on a forced TU rebuild, auval SUCCEEDED (caps [1,2] [2,2]), pluginval s10 SUCCESS VST3 + AU, pedalboard V1–V3 (12-param contract, pass-through with reported == measured latency, 12-value state round-trip) ALL PASS. COMPAT-01 + FUNC-07 complete. Next: Stage 2 / discuss (Phase 2.1).
Progress: [###########.........] 56%

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
| plan | ✓ | 2026-10-12 | |
| execute | ✓ | 2026-10-12 | |
| verify | ✓ | 2026-10-12 | |

### Stage 2: DSP — Phase 2.2 (rotation)
| Phase | Status | Date | Skipped |
|-------|--------|------|---------|
| discuss | ✓ | 2026-10-12 | |
| research | ✓ | 2026-10-12 | |
| plan | | | |
| execute | | | |
| verify | | | |

### Stage 2: DSP — Phase 2.3 (binaural)
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

**Stage 2 / Phase 2.1 / plan:** ✓ Complete (2026-10-12)
- `stages/2-dsp/PLAN-2.1.md` — decisions A1–A15 (D5 amendment adopted as A1: persistent `gridPos`, constant `kOnePole`/`kRampStep` from 64 samples; A2 ramps with exact arrival + 1e-6 snap; A3 `b + t·(a − b)`; A4 exact decoder literals; A5 `t = (j+1)/64`; A6 `[−180, 180)`; A7 `setParams` before `prepare`; A8 engine-owned peaks, public `meterFrames`; A9 DSP-05 wording; A10 click metric; A11 alloc counter; A12 TripleBuffer copy; A13 JUCE-free; A14 D6 placeholder explicit at the call site; A15 −0.06 dB number)
- Shared control-step spec (one block, op-for-op) binds T7 `SpatialEngine` and T9 `ReferenceModel`
- Tasks: T1 `Smoothers.h` · T2 `ShEncoder.h` (verbatim lift + provenance) · T3 `DistanceModel.h` (`clampedCutoffHz()` accessor) · T4 `OrbitLfo.h` · T5 `StereoDecoder.h` + `DecoderMixer.h` · T6 `MeterFrame.h` + `TripleBuffer.h` · T7 engine composition (no bus buffer, `busTap[16]`) · T8 processor wiring + CMake sources · T9 `tests/render-harness/ReferenceModel.h` · T10 harness infra (rtcheck, click helpers, `checkFailsAsExpected`, drivers) · T11 probes (P0–P5, P2b/P3b, DSP-01/04/05+clamp/06a/06b, order fade on the bus tap, PERF-01, meter frame, extremes, digest) · T12 build / harness / install / auval / pluginval ×2 · T13 SUMMARY-2.1 + STATUS + temp-index commit
- Gate design notes: the order-fade click gate reads the 16-channel bus (Stereo output is blind to bands 2–3); the hook-driven sweep verdict is at 64-sample blocks only (large-block parameter steps are automation granularity under τ = 20 ms, printed informationally); orbit-driven 64 vs 4096 renders must be bit-equal

**Stage 2 / Phase 2.1 / verify:** ✓ Complete (2026-10-12)
- `stages/2-dsp/VERIFICATION-2.1.md` — ✅ VERIFIED; every SUMMARY-2.1 claim re-run independently on the installed bundles (`cmp` identical to the build, still identical after a forced 0-warning rebuild): harness 118/118 PASS, digest `9189d911a1fe98d5` reproduced; `auval -v aufx OSAm OuDv` SUCCEEDED (caps `[1, 2] [2, 2]`, 0 WARN); pluginval s10 VST3 + AU SUCCESS (25 groups each)
- Independent measurer `tests/pedalboard/verify_2_1.py` on the installed VST3 (shares nothing with the harness / ReferenceModel): V1 default Binaural not silent · V2 pan law (+90° 22.88 dB left, −90° 22.88 dB right, front/rear L == R, corr 1.00000) · V3 impulse at the compensated index 100 with amplitude exactly 0.7500000 and zero tail in both decoder modes (confirms Incident 1's `fillParams` snap on the installed binary) · V4 distance +8.895 / 0.000 / −24.178 dB, HF/LF flat ≤ 0.002 dB on [0.2, 1] then −1.95 / −5.15 / −7.91 / −10.788 dB · V5 orders 1/2/3 bit-equal in Stereo · V6 orbit 17.6 dB swing, Off static · V7 short way (max 0.299 vs static 0.296; long way 0.866) · V8 32 corners finite · V9 state round-trip — 25 checks ALL PASS
- Requirements: FUNC-02, FUNC-06, DSP-01, DSP-04, DSP-05, DSP-06, PERF-01 → complete; FUNC-01 / FUNC-03 / QUAL-01 → partial (remainder at 2.2 / 2.3 / Stage 4); 16 acceptance-criteria boxes ticked with evidence
- Measurer lessons recorded (pedalboard `distance_m` set quantises to ≈ 1.004 m and engages the air LP — leave the default or set `raw_value`; short-way gates relative to the static cardioid asymmetry at the start angle; `orbit` reads back as numpy bool)
- Human DAW hands-on (pan sign, distance, orbit, default not silent) listed non-blocking; repeated formally under COMPAT-02 / QUAL-02 at Stage 4

**Stage 2 / Phase 2.2 / discuss:** ✓ Complete (2026-10-12)
- `stages/2-dsp/CONTEXT-2.2.md` — entry check re-read (tap + peak scan are PRE-rotation in 2.1 and must move; `Params` already carries yaw/pitch/roll; all three angles ±180° continuous; every 2.1 probe leaves rotation at 0°)
- Decisions D1–D10: rotate the bus per sample (83 elements, W implicit); per-element matrix lerp on the control grid, all blocks always applied; zero rotation bit-transparent → digest-2.1 `9189d911a1fe98d5` is a VERDICT, digest-2.2 separate; three `WrapSmoother`s; tap/`shPeak`/`busTap()` post-rotation; ReferenceModel recursion-free (re-encode at the rotated direction through its own Cartesian builder); cardinal signs ×3 + `yaw = −azimuth → front` asserted through the engine frame; `azRot/elRot = dir(R·d)` per control step; own transcription of Ivanic–Ruedenberg 1996 + 1998 erratum (IEM / Green consulted for indices only); yaw-only fallback armed
- Gate set named: DSP-02 (200 tuples × 3 orders < 1e-4) + per-block orthogonality, P1 null at a non-zero tuple, yaw-sweep + yaw-step click gates on all 16 bus-tap channels (negative control = ReferenceModel `snapCoefficientsPerGrid`), P2b/P3b with rotation automation, PERF-01 with rotation moving, meter-frame rotated fields, extremes; `tests/pedalboard/verify_2_2.py` at verify
- Open for research R1–R7 — R1 validates the recursion formulas numerically in a scratch script before RESEARCH-2.2 closes; R3 proves identity exactness element-by-element (fallback: < 1e-6 null)

**Stage 2 / Phase 2.2 / research:** ✓ Complete (2026-10-12)
- `stages/2-dsp/RESEARCH-2.2.md` — R1 recursion transcription (code form, `u/v/w`, `P` three cases, `V/W` δ factors, three guards) validated at 200 tuples: 1.8e-15 double, 1.3e-7 float32, orthogonality 1.2e-15, transpose fails (1.24), yaw-only closed form agrees, D7 literals reproduced through `dir()`, IEM index map `p = (1, 2, 0)` confirmed (read only, D9); R2 chord dip per band −0.045 / −0.179 / −0.407 dB at 11.6°/grid (≤ 0.001 dB at 0.48°), and the click-metric finding: a 180° step reads 12.3 / 24.7 / 43.6× vs static (one-pole + 64-grid lerp kinks, same as the 2.1 azimuth path) but 1.0000 vs the same-motion reference → step gate vs reference (negative control 64–126×), sweep gate vs static parks (1.127 at ACN 9, negative control ≥ 49× on m ≠ 0); R3 identity exact in float32 (all three blocks bit 0/1; `sqrtf(2)·sqrtf(2) == 2.0f`), signed-zero caveat; R4 `dir()` 9e-13° round trip, pole azimuth undefined → elRot-only assertions for pitch/roll, `clamp` before `asin`; R5 `blockOffset 0/9/34`, `rotIndex`, `static_assert`s, recursion reads `R_{l−1}` from the same array; R6 sweep must start at the prepared value (2.1 idiom = hidden 180° step, 4.6× at band 3), click gate at az 37/el −12 (front source has 6 dead channels), liveness −8.18 dB and yaw≡azimuth cross-gate (1.5e-7) on a front source; R7 `tap16` helper only
- Findings: F1 HIGH identity fast path for the frame direction (`main.cpp:1659` bitEqual, `atan2f` round trip 36.999992); F2 wrap snap reaches exact 0; F3 m = 0 channels bit-identical under yaw (positive gate + negative-control exclusion); F4 24 parks ≥ 93 % of peak; F5 gimbal = display only; F6 ReferenceModel extension ≈ 40 lines, stays recursion-free
- Corrections recorded: D1 cost is ≈ 16 MFLOPS (332 flops/sample), not 4; step-response audibility is a verify hands-on item with v1.1 candidates named

**Stage 2 / Phase 2.1 / execute:** ✓ Complete (2026-10-12)
- `stages/2-dsp/SUMMARY-2.1.md` — files: `Source/dsp/{Smoothers,ShEncoder,DistanceModel,OrbitLfo,StereoDecoder,DecoderMixer,MeterFrame,TripleBuffer}.h` (created, JUCE-free), `Source/dsp/SpatialEngine.h` (composition on the 64-sample control grid, `busTap[16]`, no allocation), `Source/PluginProcessor.{h,cpp}` (12 cached atomics, `fillParams()` with range snapping, scratch sized in `prepareToPlay`, slice loop → `engine.process`, per-sample seeded gain, public `meterFrames` published once per block), `CMakeLists.txt` (sources), `tests/render-harness/ReferenceModel.h` (double transcription with the generic SN3D table and two negative-control switches), `tests/render-harness/main.cpp` (rtcheck counter verbatim from O-Octagon, click helpers, `checkFailsAsExpected`, drivers, the full 2.1 probe set)
- Harness 118 checks ALL PASS (×4 runs, digest `9189d911a1fe98d5` reproduced); build 0 warnings; `build-and-install.sh` OK; `auval -v aufx OSAm OuDv` SUCCEEDED; pluginval strictness 10 ×2 SUCCESS for VST3 and AU
- Test-and-fix incidents: (1) ENGINE/PROCESSOR defect — APVTS raw default of `distance` is the unsnapped skew round-trip 1.0000001 (JUCE legacy comment in `juce_AudioProcessorValueTreeState.cpp`), g(d) = 1 − 1 ulp, P1 anchor / P4 / P1b / meter peaks missed by 1 ulp → `fillParams()` snaps `distance` and `orbitRate` through their `NormalisableRange`; (2) PROBE errors — DSP-04 correlation gate hit the cardioid's analytic null (float −3e-9) → normalised by source energy with a −1e-6 floor; meter-frame `outPeakL` expectation ignored the 64-sample delay → taken from the output samples
- Deviations: `DecoderMixer` API split into `delayStereo()` + `blend()` (so the A14 placeholder is literally at the engine call site) with `processSample()` kept for 2.3; `DecoderMixer` is a template on the delay length (keeps `kPartition` in `SpatialEngine.h` without a circular include); DSP-04 e = ±90° bit anchor taken at az 0 / 180 (at az 90 the float cos(π/2) residual lands 1 ulp below 0.5); DSP-05 "exactly 0 dB" asserted as the bit-level identity `out == 0.75f·(g·x[n−64])` with the DFT ratio printed (1.1e-07 dB)
- Deferred (SUMMARY-2.1 §Deferred): A14 placeholder → 2.3 obligation; `kStereoLevelMatch` real value → 2.3; TripleBuffer → `core/triple-buffer` module candidate (3 consumers); `ShRotator` slot → 2.2; −180/180 display → Stage 3

## Next Steps

1. Stage 2 / Phase 2.2 / plan: `/plugin-plan O-simpleAmbisonics 2-dsp` — adopt RESEARCH-2.2 open items 1–10: transcribe R1 into `ShRotator.h` (float, constexpr `u/v/w` table, three guards, `rotIndex`); F1 identity fast path for `azRot/elRot`; yaw-step gate vs the same-motion reference + yaw-sweep gate vs static parks started at the prepared value (az 37 / el −12) + front-source liveness and yaw≡azimuth cross-gate; F3 m = 0 bit-identity gate; D7 literals (elRot-only for pitch/roll); DSP-02 + orthogonality; `tap16` helper, `RefParams` angles, ReferenceModel builder; P2b/P3b + PERF-01 with rotation moving; SUMMARY deferred list (step audibility, 16 MFLOPS, signed zero)
2. Non-blocking human item from VERIFICATION-2.1.md: installed plugin hands-on in a DAW (Stereo mode pans with Azimuth, +90° louder LEFT; Distance 1 → 20 m darkens and attenuates; Orbit On moves, Off glides back the short way; default Binaural setting is NOT silent)
3. Non-blocking human checks from `stages/1-foundation/VERIFICATION.md` (Logic stereo + mono track, Generic editor 12 rows, project save/reopen) — repeated formally under COMPAT-02 at Stage 4
4. UI mockup (`/start O-simpleAmbisonics` → option 3) before Stage 3; it finalises `parameter-spec.md` and must keep the 12-parameter set

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
- plugins/O-simpleAmbisonics/.planning/stages/2-dsp/PLAN-2.1.md (Stage 2 / 2.1 plan)
- plugins/O-simpleAmbisonics/.planning/stages/2-dsp/SUMMARY-2.1.md (Stage 2 / 2.1 execute)
- plugins/O-simpleAmbisonics/Source/dsp/Smoothers.h, ShEncoder.h, DistanceModel.h, OrbitLfo.h, StereoDecoder.h, DecoderMixer.h, MeterFrame.h, TripleBuffer.h (Stage 2 / 2.1 execute)
- plugins/O-simpleAmbisonics/tests/render-harness/ReferenceModel.h (Stage 2 / 2.1 execute)
- plugins/O-simpleAmbisonics/.planning/stages/2-dsp/VERIFICATION-2.1.md (Stage 2 / 2.1 verify)
- plugins/O-simpleAmbisonics/tests/pedalboard/verify_2_1.py (Stage 2 / 2.1 verify)
- plugins/O-simpleAmbisonics/.planning/stages/2-dsp/CONTEXT-2.2.md (Stage 2 / 2.2 discuss)
- plugins/O-simpleAmbisonics/.planning/stages/2-dsp/RESEARCH-2.2.md (Stage 2 / 2.2 research)
