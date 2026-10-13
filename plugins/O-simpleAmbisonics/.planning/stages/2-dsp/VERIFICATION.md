# Stage 2: DSP - Verification (roll-up of Phases 2.1 / 2.2 / 2.3)

## Verification Date

2026-10-13 (Phase 2.1 verified 2026-10-12, 2.2 verified 2026-10-12, 2.3 verified 2026-10-13)

## Method

Stage 2 was built in three phases, each with its own discuss → research → plan → execute → verify cycle and its own `VERIFICATION-2.N.md` (the per-phase evidence lives there; this roll-up names the gate and the result for every stage-2 requirement ID). Every phase verify re-ran its SUMMARY's claims independently on the **installed** bundles (`cmp`-identical to the build), rebuilt under `juce_recommended_warning_flags` with 0 warnings, ran the project harness twice (byte-identical output), ran `auval -v aufx OSAm OuDv` and pluginval strictness 10 ×2 on VST3 + AU, and loaded the installed VST3 in an independent pedalboard measurer (`tests/pedalboard/verify_2_{1,2,3}.py` — 25 / 35 / 42 checks, all passing).

Final state at the 2.3 verify: harness **241 / 241**, digests **2.1 `9189d911a1fe98d5`** (zero-rotation Stereo render, a verdict since 2.2) and **2.2 `7f35bc9f91e321b9`** (rotated Stereo render, a verdict since 2.3) reproduced; digest-2.3 `edd1acfd21f54bcd` (Binaural null render, informational).

## Goal-Backward Analysis

### Original Goals (ROADMAP §Stage 2 + the three CONTEXTs)

1. **2.1** — the audible chain: distance model → SN3D / ACN encoder with order gating → 16-channel bus → ±60° stereo decoder, 20 ms control-grid smoothing everywhere, meter frame, latency 64, a headless harness with a reference model.
2. **2.2** — scene rotation in the SH domain (Ivanic–Ruedenberg), yaw / pitch / roll after encoding; the puck stays put while the image moves; the meter frame publishes the rotated vector and direction.
3. **2.3** — the real binaural leg: SADIE II KU100 MagLS banks per order embedded as a generated header, grid-synchronous UPOLS on the rotated bus, click-free dual-bank order fade, level-matched decoder switch, 64 latency in both modes at every rate.
4. Across all three: nothing a previous phase proved changes (digests as verdicts); RT-safe; pluginval-clean.

### Goal Achievement

| Goal | Status | Evidence (phase verify) |
|------|--------|--------------------------|
| 1. Audible chain (2.1) | ✅ Achieved | VERIFICATION-2.1: harness 118 / 118, P1 null vs the reference, DSP-01 2.07e-7, DSP-04 exact literals, DSP-05 clamp gate, DSP-06 click ratios 1.0003 / 1.0000, pedalboard 25 / 25 |
| 2. Scene rotation (2.2) | ✅ Achieved | VERIFICATION-2.2: DSP-02 7.2e-7 over 200 tuples, cardinal signs exact, yaw ≡ azimuth bit-equal on the installed VST3, yaw step 1.0003 vs the same-motion reference, pedalboard 35 / 35 |
| 3. Binaural leg (2.3) | ✅ Achieved | VERIFICATION-2.3: header regenerates byte-identical, UPOLS null ≤ 1.4e-7 at six host blocks, onsets / latency 64 at three rates, ILD +12.28 / +13.87 dB with ITD +36 / +37, order and decoder clicks 0.998–1.004, level delta +0.004 dB, pedalboard 42 / 42 (installed IRs == Σ Y·h from the header within 5.6e-8) |
| 4. Regression / RT / hosts | ✅ Achieved | both Stereo digests reproduced at every later phase; PERF-01 0 allocations re-gated at each phase; auval SUCCEEDED and pluginval s10 ×2 both formats at each phase |

## Requirements Verification

**Stage:** stage-2
**Requirements for this stage (REQUIREMENTS.md §Traceability):** FUNC-01…06, DSP-01…07, PERF-01, PERF-02, QUAL-01 — 16 total (11 must, 5 should)

| Requirement | Priority | Status | Gate result (phase) |
|-------------|----------|--------|---------------------|
| FUNC-01: Complete ambisonic chain inside one stereo-out plugin | must | ✅ Complete | Buses `[1, 2] [2, 2]` only (auval, 2.1); order 1 / +90° / Binaural: LEFT louder by +12.31 dB on the installed VST3 with the real leg (2.3; 22.88 dB under the 2.1 placeholder); yaw = −azimuth → front, `shGain == refSh16 (0, 0)` 2.2e-8, \|L − R\| = 0, pedalboard ≤ 1.5e-8 (2.2) |
| FUNC-02: Azimuth / Elevation / Distance | must | ✅ Complete | Automatable with the stated ranges (pedalboard); el +90 → W / Z encode, DSP-04 L == R == 0.5f (2.1) |
| FUNC-03: Order 1 / 2 / 3 switchable while running | must | ✅ Complete | `orderActive` 4 / 9 / 16, bands above the order exactly 0 (2.1); bus-side switch ratio ≤ 1.0 (2.1); decoder filter-bank swap click-free, DSP-07 (2.3) |
| FUNC-04: Scene rotation Yaw (must) / Pitch / Roll (should) after encoding | must | ✅ Complete | Puck fixed under yaw while `azRot` moves; yaw ψ ≡ azimuth ψ bit-equal; pitch + raises the front, roll + raises the left (cardinal + pedalboard anchors exact); composition order pinned (2.2) |
| FUNC-05: Decoder Binaural / Stereo | must | ✅ Complete | Both modes live (Binaural ≠ Stereo 1.3988); B→S→B click ratios 1.0000–1.0007 (neg 12–28×), pedalboard 1.000 (neg 41–68×); latency 64 in both modes at 44.1 / 48 / 96 k, stereo leg delayed 64 so the switch does not shift time (2.3) |
| FUNC-06: Orbit on / off + rate | should | ✅ Complete | Orbit trajectory vs the reference, Off glides back the short way, P2b / P3b slicing bit-equal with orbit On (2.1) |
| DSP-01: AmbiX SN3D / ACN encoder, orders 1–3, within 1e-5 of the reference table | must | ✅ Complete | 13 × 7 grid vs an independent double table: max 2.07e-7; W == 1.0f everywhere (2.1); re-anchored by the pedalboard 2.3 own-encoder (scipy Legendre) via the installed IRs |
| DSP-02: SH rotation == re-encoding at the rotated direction | must | ✅ Complete | 200 tuples × orders 1 / 2 / 3: 2.6e-7 / 5.2e-7 / 7.2e-7 (gate 1e-4); orthogonality 5.6e-7; transpose trap FAILs (1.974); identity exact in float (2.2) |
| DSP-03: HRTF binaural decoder, embedded per-order filter set, partitioned convolution, latency reported | must | ✅ Complete | SADIE II D1 MagLS banks as `inline constexpr` arrays (no runtime I/O; regenerate byte-identical from the pinned SOFA); UPOLS P 64 null ≤ 1.4e-7 vs direct-form at six host blocks and at 96 k; `getLatencySamples() == 64` both modes at three rates with the impulse onset at 100 + 64 + the table onset; ILD +12.28 / +13.87 dB (sign AmbiX) and ITD +36 / +37 samples at +90°, mirrored at −90° (2.3) |
| DSP-04: Stereo decode mode without HRTF | should | ✅ Complete | ±60° cardioid pair on W / Y / X with exact literals 0.5f / 0.25f / 0.4330127f; +90° 22.88 dB; `kStereoLevelMatch` 1.0f by design (2.1, 2.3) |
| DSP-05: Distance model — inverse-distance gain with near-field clamp + air LP, monotonic | should | ✅ Complete | g(d) = 1.25 / (d + 0.25) anchors exact (0 dB at 1 m, +8.87 dB at 0.2 m, −24.19 dB at 20 m); TPT air LP with the 0.45·fs clamp gated on the clamp target; monotonic sweep (2.1) |
| DSP-06: Shortest-path angle smoothing, no zipper; distance / gain smoothed | must | ✅ Complete | +170 → −170 through 180; 1 s full-circle sweep click ratio 1.0003 / 1.0000 (neg 24.8) (2.1); yaw / pitch / roll on the same `WrapSmoother`: yaw sweep 1.127, yaw step 1.0003 vs the same-motion reference (neg 49–544) (2.2) |
| DSP-07: Order and decoder changes click-free; crossfaded bank swap; unused channels zeroed; buffers pre-sized for 16 ch + max block | must | ✅ Complete | Order 1→3 / 3→1 ratios 0.998–1.004 at 64 and 1024 blocks (neg 11–179×), B→S→B 1.0000–1.0007 (neg 12–28×), spectral liveness +6.28 dB; pedalboard on the installed binary 1.00–1.53 (ceiling 2.0, snapped 67×); every convolver vector assigned in `prepare()`, FDL decays to exact 0, oversized host blocks bit-equal (P3e) (2.3) |
| PERF-01: RT-safe processBlock; lock-free meter handoff | must | ✅ Complete | Global new / delete counter 0 across 200 calls with every parameter moving — re-gated at 2.1 (chain), 2.2 (rotation) and 2.3 (convolver live, both fades inside the window); `TripleBuffer<MeterFrame>` + two atomics, no locks (inspection) |
| PERF-02: Order-3 binaural without dropouts at 64–1024 buffers; sized for the maximum host block | must | ✅ Complete | Release best-of-3: 193–203× real time at 48 k / 64 blocks, 195–206× at 1024, 72–76× at 96 k (gate ≥ 20); grid-sized convolver + processor slice loop, P3e prep-64 + 4096 feed bit-equal (2.3) |
| QUAL-01: No audio artifacts at normal ranges | must | ⚠️ Partial | No clicks on order / decoder switch (DSP-07), no zipper on sweeps (DSP-06), no NaN / denormal at the extremes with the HRTF leg (harness 16 + 8 + 8 corners, gimbal, headroom; pedalboard 32 Binaural corners; pluginval s10) — all done. **No clicks on preset load → Stage 4** (no presets yet) |

**Requirements Summary:**
- ✅ Complete: 15 of 16
- ⚠️ Partial: 1 (QUAL-01 — preset-load clause owned by Stage 4)
- ⏸️ Deferred (later stage): 12 (UI-01…08, FUNC-08, FUNC-09, COMPAT-02, QUAL-02)
- ❌ Failed: 0
- Stage-1 requirements re-confirmed at every phase: COMPAT-01 (pluginval s10 ×2 VST3 + AU), FUNC-07 (mono sum / Left only, mono-in bus)

## Automated Checks (final state, 2.3 verify)

| Check | Result | Notes |
|-------|--------|-------|
| Build VST3 / AU / Standalone / render-test | ✅ Pass | 0 warnings under `juce_recommended_warning_flags` |
| Installed == build | ✅ Pass | `cmp` identical, both formats |
| Harness ×2 | ✅ Pass | 241 / 241, byte-identical except the PERF-02 wall-clock line; digests 2.1 + 2.2 asserted |
| Generator regeneration | ✅ Pass | 26 self-checks; header + goldens byte-identical |
| `auval -v aufx OSAm OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval s10 ×2 VST3 + AU (installed) | ✅ Pass | SUCCESS ×4 |
| pedalboard measurers on the installed VST3 | ✅ Pass | `verify_2_1.py` 25 / 25, `verify_2_2.py` 35 / 35, `verify_2_3.py` 42 / 42 |

## Human Verification (non-blocking; QUAL-02 and COMPAT-02 repeat them formally at Stage 4)

- [ ] Headphones, Binaural, order 3: front / back / left / right / overhead distinct; +90° louder LEFT; image sharpens 1 → 3; order and decoder switches click-free mid-tone; no loudness jump on the decoder switch.
- [ ] Yaw spins the image while the Azimuth value stays put; Pitch + raises the front; Roll + raises the left; flick Yaw 0 → 180 at order 3 on a low tone and listen for a zip.
- [ ] Stereo mode pans with Azimuth (+90° louder LEFT); Distance 1 → 20 m darkens and attenuates; Orbit On moves, Off glides back the short way.

## Issues Found

- No plugin defect open from Stage 2. Each phase verify's first measurer run surfaced measurer-side lessons only (2.1: pedalboard set-quantisation; 2.2: `raw_value` for linear ranges, roll about the front axis, dB steps through a cardioid null; 2.3: pedalboard's per-call latency flush hole, zero-crossing-blind snaps, two-ramp order switch bound, per-order ITD low-pass) — recorded in the phase documents and in memory.
- Carried to Stage 3: puck readout formats the parameter (the frame carries −11.999999 for −12); −180 displays as 180; gimbal-lock tooltip (compass holds within ≈ 0.01° of the pole); bank-fade progress available from `BinauralConvolver::bankMixCur()` / `isFading()` if the decoder view wants it (no `MeterFrame` field added).
- Carried to Stage 4: preset-load click clause of QUAL-01; Binaural presets must not sit near 0 dBFS (order-3 bank +6.5 dB peak, 0.2 m +8.9 dB — `outputLevel` exists, presets never set it); QUAL-02 listening; COMPAT-02 Logic + Live hands-on.
- v1.1 candidates: TripleBuffer → `core/triple-buffer` module; BiMagLS / DFE option / second head; own Kaiser polyphase resampler (HF above 15–16 kHz at fs ≠ 48 k is the interpolator's own design); a longer τ or quadratic interpolation for 180° rotation steps if the ear objects.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes — Stage 3 (GUI). Prerequisite: the UI mockup (`/start O-simpleAmbisonics` → option 3), which finalises `parameter-spec.md` and must keep the 12-parameter set.

**Blockers:** None
