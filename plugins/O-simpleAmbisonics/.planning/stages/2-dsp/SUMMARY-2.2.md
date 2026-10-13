# Stage 2 — DSP · Phase 2.2 (SH rotation + rotated meter tap) — SUMMARY

**Plugin:** O-simpleAmbisonics · **Stage:** 2 DSP · **Phase:** 2.2 of 3 · **GSD phase:** execute · **Date:** 2026-10-12
**Inputs:** `PLAN-2.2.md` (T1–T8, B1–B14, the shared rotation control-step spec), `CONTEXT-2.2.md` (D1–D10), `RESEARCH-2.2.md` (R1–R7, F1–F6), `research/ARCHITECTURE.md` §SH Rotator / §SH rotation, the 2.1 source + harness
**Executor:** dsp-agent role (orchestrator session with Bash for T7 — as in 2.1)
**Branch:** `main` (trunk-based; `plugins/O-simpleAmbisonics/**` only; `PLUGINS.md` untouched)
**Gate:** `run-gate.sh O-simpleAmbisonics 1-foundation 2-dsp` → **PASSED** (build, pluginval; schema skipped — no `HANDOFF.json` at Stage 1, standing pattern)

## Result

`ShRotator` is in the engine and the **scene rotates after encoding**: per control step the three wrap-smoothed angles build `R₃ = Rz(ψ)·Ry′(θ)·Rx(φ)`, the order-1 block is the ACN permutation `p = (1, 2, 0)`, the 5 × 5 and 7 × 7 blocks come from the Ivanic–Ruedenberg recursion transcribed verbatim from RESEARCH-2.2 R1 (float, three guards, no transpose), and the 83 nonzero elements are lerped per sample (fused lerp-and-apply, W implicit) onto the 16-channel bus. The bus tap and the peak scan are **post-rotation**; the frame publishes `shGain = R_SH·c_eff` and `azRot / elRot = dir(R₃·d)` with the identity fast path, while `azEff / elEff` stay pre-rotation.

**Zero rotation is bit-transparent: digest-2.1 `9189d911a1fe98d5` reproduces exactly** and is now asserted as a `check` (D3 / B12) — the entire 2.1 probe set runs unchanged (bodies and verdict strings untouched) and passes. Latency is exactly 64; `setLatencySamples` and the 12 parameters are untouched.

Harness: **160 checks, ALL PASS** (118 from 2.1 + 42 new), output byte-identical across two runs of the final binary; **digest-2.2 `7f35bc9f91e321b9`** (rotated P1-null render). Build: **0 warnings** on all four targets. Installed bundles (`cmp`-identical to the build): `build-and-install.sh` OK, `auval -v aufx OSAm OuDv` **SUCCEEDED**, pluginval strictness 10 — see §Build / install / validation.

Covers (gated below): FUNC-04 (yaw must; pitch / roll should — all three), DSP-02, FUNC-01 third box (`yaw = −azimuth` → front), DSP-06 extended to the three angles, PERF-01 re-gated with rotation moving, QUAL-01 partial (±180° on all three angles, gimbal).

## Files

| Action | Path | Content |
|---|---|---|
| create | `Source/dsp/ShRotator.h` | Provenance header (own IR 1996 + 1998-erratum transcription; IEM / Green read for indices only — D9), conventions (frame, matrices + signs, ACN permutation, active convention / no transpose), storage (`kNumRotElems = 83`, `blockOffset = {0, 9, 34}`, `rotIndex`, four `static_assert`s), precision (float, identity exact), cost (≈ 700 flops / step, 332 flops / sample ≈ 16 MFLOPS). `kRadToDeg` (the one new literal). `ShRotator::prepare()` fills `uTab / vTab / wTab[83]` once (B3; `sqrt2 = std::sqrt (2.0f)` member); `buildCartesian` (`Rz·(Ry′·Rx)`); `build` (R₁ by permutation → R₂ → R₃ in place through `rotIndex` on the same array; `u ≠ 0` / `w ≠ 0` guards; `P` with the three cases); `apply` (W implicit, row-major); `applyLerped` (B5 fused `prev + t·(cur − prev)`, distinct in/out) |
| modify | `Source/dsp/SpatialEngine.h` | `#include "ShRotator.h"`; members `WrapSmoother yaw, pitch, roll; ShRotator rot; float prevR[83], curR[83], curR3[3][3], bus[16]` (`tap[16]` now post-rotation). `prepare`: `rot.prepare()` before `snapToParams()`. `snapToParams`: three snaps, `build` → `curR`, `prevR ← curR`. `controlStep`: three steps after `mixer.controlStep`, `prevR ← curR` + rebuild after `evaluateCoefficients()`, before `fillFrameState()`. `fillFrameState`: `shGain = apply (curR, curC)`, `azEff / elEff` pre-rotation, identity fast path (B6) else D8 trig with `clamp` before `asin` and `wrapTo180` on `azRot`. `renderSample`: encode → `bus`, `applyLerped (prevR, curR, t, bus, tap)`, peak scan on `tap`, decoder reads `tap`. Header comment: chain with the rotator, D1 / D2 / B6 blocks, the R2 step-response note with the v1.1 candidates |
| modify | `Source/dsp/MeterFrame.h` | Comment only: `shGain = R_SH·c_eff` from 2.2; `shPeak` post-rotation; `azRot / elRot = dir(R₃·d)`, exactly `azEff / elEff` at zero rotation, **undefined within ≈ 0.01° of the pole**, gimbal lock a display matter |
| modify | `Source/PluginProcessor.cpp` | Header comment only: the chain now has the rotator after the encoder; the tap is post-rotation. No code change (`fillParams` already copied the three angles) |
| modify | `CMakeLists.txt` | `target_sources` + `Source/dsp/ShRotator.h` |
| modify | `tests/render-harness/ReferenceModel.h` | `RefParams` + `yawDeg / pitchDeg / rollDeg`; `buildR3` (double, written from the ARCHITECTURE matrices, composed `Rz·Ry′·Rx`), `direction`, `dir` (`atan2` / `asin (clamp)`, degrees, az wrapped), `rotatedDirection`; `Wrap yaw, pitch, roll` snapped in `prepare`, stepped in `controlStep` after the mixer (same k, same place as the engine); `evaluateC()` = `refSh16 (rotatedDirection (…)) × gains` (D6: re-encode — this file never contains `R_SH`); `snapCoefficientsPerGrid` / `snapOrderGains` untouched; header comment |
| modify | `tests/render-harness/main.cpp` | Title "Phase 2.1 + 2.2"; probe list appended; `#include "dsp/ShRotator.h"`; `refParamsFrom` (both overloads) copy the three angles; `EngineRender / RefRender::tap16` (stride 16) recorded next to `tap4 / tap9`; helpers `Tuple`, `makeTuples` (`juce::Random` seed `0x2D5A2D5A`, printed), `gatedRefSh16`, `oracleSh16`, `maxAbsDiff16`, `transposeBlocks`, `blockOrthoError`, `orthoError3`, `tapRatio`; the T6 probe block between the 2.1 extremes and PERF-01; PERF-01 `switch` + cases 15 / 25 / 35 / 70 / 90 and its verdict string; digest-2.2 print. **Every 2.1 probe body and verdict string is untouched** (B13) |
| create | `.planning/stages/2-dsp/SUMMARY-2.2.md` | this file |
| modify | `.planning/STATUS.md` | Phase 2.2 execute ✓, frontmatter, Current Position, Completed So Far, Next Steps, Files Created |

Not touched (per plan): `Source/PluginProcessor.h`, `Source/dsp/{Smoothers,ShEncoder,DistanceModel,OrbitLfo,StereoDecoder,DecoderMixer,TripleBuffer,FixedDelay}.h`, `tests/render-harness/CMakeLists.txt`, `tests/pedalboard/*`, `PLUGINS.md`, `setLatencySamples`, the 12 parameters, any `BinauralConvolver` / `BinauralFilterData` (2.3), any editor file. (`.planning/stages/1-foundation/gate-report.json` was rewritten by the gate run.)

## The rotation control step, as implemented (PLAN-2.2 shared spec — engine and reference op-for-op)

```
snapToParams():   az / el / dist / orderGain / mixer / orbit as 2.1
                  yaw.snapTo (p.yawDeg); pitch.snapTo (…); roll.snapTo (…)
                  evaluateCoefficients(); prevC ← curC
                  rot.build (yaw.cur, pitch.cur, roll.cur, curR, curR3); prevR ← curR
                  bus = tap = 0; gridPos = 0; resetPeaks(); fillFrameState()

controlStep():    az / el / dist / orderGain / mixer as 2.1
                  yaw.step (p.yawDeg, kOnePole); pitch.step (…); roll.step (…)       ← D4, after mixer.controlStep
                  prevC ← curC; evaluateCoefficients()
                  prevR ← curR; rot.build (yaw.cur, pitch.cur, roll.cur, curR, curR3)
                  fillFrameState()

fillFrameState(): shGain = ShRotator::apply (curR, curC)                                ← R_SH·c_eff, W passes
                  azEff = az.cur; elEff = el.cur                                        ← pre-rotation
                  if (yaw.cur == 0 && pitch.cur == 0 && roll.cur == 0) azRot = azEff; elRot = elEff      ← B6
                  else d = (cos e cos a, cos e sin a, sin e); d′ = curR3·d
                       azRot = wrapTo180 (atan2f (d′y, d′x)·kRadToDeg); elRot = asinf (clamp (d′z))·kRadToDeg

renderSample():   y = dist.processSample (x, t)
                  bus[acn] = y·(prevC + t·(curC − prevC))                               ← pre-rotation, local
                  ShRotator::applyLerped (prevR, curR, t, bus, tap)                     ← B5: tap[0] = bus[0]; blocks 1–3 fused
                  shPeak[acn] = max (shPeak, |tap[acn]|)                                ← POST-rotation (D5)
                  StereoDecoder::decode (tap) → delay → mixer                           ← unchanged
```

Reference twin (double): same three `Wrap` smoothers stepped at the same place; `evaluateC() = refSh16 (dir (R₃·d (az.cur, el.cur))) × gains[band]` through its own `buildR3`.

## Harness — the probe table (every literal; run 2 of the final binary, run 3 byte-identical)

**2.1 set (118 checks):** ALL PASS, unchanged bodies, **digest `9189d911a1fe98d5`** — the `meter frame: azRotDeg == azEffDeg (identity rotation)` bitEqual check at `main.cpp` passes through the B6 fast path.

| Probe (2.2) | Verdict | Literal |
|---|---|---|
| **digest-2.1 verdict** (D3 / B12) | PASS | `fnv1a64 (nullRender) == 9189d911a1fe98d5` |
| **DSP-02** order 1 / 2 / 3, 200 tuples (seed `0x2d5a2d5a`) | PASS ×3 | max\|err\| **2.613e-07 / 5.248e-07 / 7.157e-07** (gate 1e-4); worst tuple az 65.32 el 61.81 ψ 133.59 θ −55.75 φ −60.58 |
| DSP-02 W exact | PASS | `out[0] == 1.0f` on every tuple / order |
| DSP-02 orthogonality blocks 1–3 | PASS | **5.589e-07** (gate 1e-5) |
| DSP-02 orthogonality `R₃ᵀR₃` (engine `buildCartesian`) | PASS | 1.946e-07 |
| DSP-02 **transpose trap** (negative control) | FAIL-as-expected | max\|err\| **1.974** (≥ 1 expected; R1 saw 1.24 at one tuple) |
| DSP-02 yaw-only closed form (ψ 73°) | PASS | 2.464e-07 (gate 1e-6) — the `+sin` in the `[−m][+m]` slot |
| **identity exact** `build (0, 0, 0)` | PASS | diagonal bit-equal 1.0f, every off-diagonal exactly 0, **0 negative zeros**, `R₃` bit-equal `I` (R3 made mechanical) |
| **cardinal (a)** ψ +90, front | PASS | azRot **90.00000**, elRot 0.00000 |
| cardinal (b) θ +90, front | PASS | elRot **90.00000** (azRot −180.000 — pole, printed) |
| cardinal (c) φ +90, left | PASS | elRot **90.00000** (azRot −135.000 — pole, printed) |
| **FUNC-01 yaw = −azimuth** (az +90 / ψ −90 / order 3 / Stereo) | PASS ×3 | frame `shGain` vs `refSh16 (0, 0)` **2.229e-08**; azEff 90.000000 / elEff 0 bitEqual (azRot 0.00000 elRot 0.00000); output max\|L − R\| **0.000e+00**, max\|L\| 0.375 |
| **P1 null rotated** (az 37 / el −12 / d 3.3 / order 3 / Stereo / ψ 25 / θ −40 / φ 70) | PASS | max\|err\| **4.754e-08** over 8192 × 2, peak \|L\| 0.296; liveness ≠ 2.1 null render |
| **meter frame (2.2)** shGain | PASS | vs `refSh16 (rotatedDirection)` **3.456e-07** |
| meter frame (2.2) azRot / elRot | PASS | **48.5942 / −7.6349** (ref 48.5942 / −7.6349) |
| meter frame (2.2) azEff / elEff | PASS | bitEqual to the **raw** parameters: 37.0000000 / **−11.9999990** (see Deviation 1) |
| meter frame (2.2) yaw hook 0 → 90 at 0.25 s, 10 reads | PASS | azEff 37.0f at all 10; azRot 37.0 37.0 37.0 37.0 48.2 120.7 126.5 127.0 127.0 127.0 (Δ 90 ≥ 60) |
| **round trip** ψ 0 → 90 → 0 | PASS ×3 | azRot 37.000000 == azEff (fast path reachable, F2); last 8192 samples **memcmp-equal** to a fresh static render; middle window differs |
| **yaw sweep click** (B8 / B9), 16 channels vs 24 parks | PASS | worst **1.1269 at ACN 9** (expected 1.13); per channel 0:1.000 1:1.009 2:1.000 3:1.009 4:1.052 5:1.009 6:1.000 7:1.009 8:1.051 9:1.127 10:1.051 11:1.009 12:1.000 13:1.009 14:1.049 15:1.126 |
| yaw sweep negative control (snapped reference, m ≠ 0) | FAIL-as-expected | ratio **49.0 … 156.4** (≥ 49 expected) |
| **zonal invariance** (F3) ACN 0 / 2 / 6 / 12 | PASS | bit-identical to the yaw-0 park over the whole render |
| **yaw step click** (B8) vs the same-motion reference | PASS | worst **1.0003 at ACN 6** (expected 1.000); all 16 within 0.9997–1.0003 |
| yaw step negative control (snapped vs lerped reference, m ≠ 0) | FAIL-as-expected | ratio **98.3 … 544.4** |
| yaw step **informational** vs static parks (R2) | printed | band 1 **5.3×**, band 2 **45.3×**, band 3 **61.9×** — grid-boundary kinks of the one-pole + lerp, same class as the 2.1 azimuth path; hands-on at verify |
| **yaw ≡ azimuth cross-gate** (front source) | PASS ×2 | max\|tap16 diff\| **8.941e-08** (gate 1e-5); L/R diff 0.000e+00 |
| yaw sweep liveness (front source) | PASS | **−8.20 dB** (≥ 6; R6 predicted −8.18) |
| **P2c** ragged ≡ flat (rotation ψ 25 / θ −40 / φ 70 + orbit 2 Hz) | PASS | bit-equal; liveness flat ≠ 2.1 `flatMoving` |
| **P3c** prep-64 + 4096 ≡ prep-4096 ≡ flat | PASS ×2 | bit-equal |
| **P2d** yaw / pitch / roll stepped at 4096 boundaries, 64 vs 4096 blocks | PASS ×2 | bit-equal; liveness ≠ hook-less |
| **extremes (2.2)** 8 corners (ψ, θ, φ) ±180 at 48 k / 96 k | PASS ×2 | all finite and live |
| extremes (2.2) gimbal θ +90 under a yaw sweep | PASS | finite, live; frame azRot 124.546 elRot 51.369 |
| **PERF-01** (extended: yaw 90 @15, pitch −45 @25, roll 30 @35, yaw −180 @70, pitch/roll 0 @90) | PASS | **0 allocations**; liveness 1 |
| **digest-2.2** | printed | `fnv1a64 = 7f35bc9f91e321b9` (rotated P1-null render) |

## Build / install / validation (T7)

| Step | Result |
|---|---|
| `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test` | EXIT 0, **0 warnings** (`grep -c warning:` = 0 on the full log) — first attempt |
| harness | **160 / 160 PASS**, runs 2 and 3 of the final binary byte-identical (`diff` empty); both digests reproduce |
| `./scripts/build-and-install.sh O-simpleAmbisonics` | OK — old `-dev` bundles removed, no alternate-variant orphan, installed, DAW caches cleared; installed VST3 and AU binaries `cmp`-identical to the build |
| `auval -v aufx OSAm OuDv` | **AU VALIDATION SUCCEEDED** |
| pluginval strictness 10, `--timeout-ms 600000`, installed VST3 ×2 | **SUCCESS ×2**, 25 test groups each, 0 failures |
| pluginval strictness 10, installed AU ×2 | **SUCCESS ×2**, 25 test groups each, 0 failures |

## Allocation-gate coverage statement (re-affirmed)

Unchanged wording from SUMMARY-2.1: covered = every new-expression on the audio thread (eight global `operator new` / `delete` variants replaced, counted on the arming thread, foreign-thread count reported beside the verdict); not covered = `HeapBlock` / `malloc` paths and the `AsyncUpdater` post path — grep + inspection. 2.2 adds **nothing dynamic**: `ShRotator`'s coefficient tables are `float[83]` members filled in `prepare()`; `prevR / curR / curR3 / bus` are fixed arrays; `build` and `applyLerped` are arithmetic on members and stack locals; the only `std::sqrt` calls are in `prepare()`. PERF-01 now exercises the three angle smoothers and the rebuild inside the counted window — 0 allocations.

## Incidents in the test-and-fix loop

1. **Probe error — meter frame (2.2) `elEffDeg == −12.0f`.** First run: one FAIL, `elEff −11.999999`. The engine publishes exactly what the host holds: the APVTS float round-trip of −12 through a [−90, 90] continuous range is `−90 + 180·(78/180) = −11.999999f`. This is the same artifact that made `refParamsFrom` read the raw atomics in 2.1 (`setParam` → `convertTo0to1` → `convertFrom0to1`). The gate now compares `azEffDeg / elEffDeg` bitEqual to the **raw parameter values** (which is the actual UI-02 contract: the puck shows the host value) and prints both. The engine was right. Note for Stage 3: the puck readout should be formatted from the parameter, not from the frame, if "−12.00" must display exactly.

No engine or reference defect was found; the transcription passed DSP-02 on the first attempt (D10 fallback not needed).

## Deviations from PLAN-2.2 (with reasons)

1. **Meter frame (2.2) `azEff / elEff` gate** compares to the raw parameters, not the literals `37.0f / −12.0f` (Incident 1). The 2.1 `azRotDeg == azEffDeg` bitEqual probe and the FUNC-01 `azEffDeg == 90.0f` literal are unaffected (37 and 90 round-trip exactly).
2. **Yaw-step negative control on the m ≠ 0 channels** (plan: channels 1–15). Under a pure yaw the zonal channels 2 / 6 / 12 are unchanged by the rotation, so the snapped and lerped references are identical there and the ratio reads exactly 1.0 — F3's exclusion applies to the step as it does to the sweep. The verdict (max ≤ 2.0 must fail) would have passed either way; the printed minimum is now meaningful (98.3). Measured 98.3–544.4 vs the plan's "64–126" — the plan's number came from R2's simulation against a *static* control; the harness control is the lerped reference's own curvature.
3. **Identity-exact verdict** asserts value-equality (`== 0.0f`) on the off-diagonals and prints the negative-zero count, rather than bit-equality to `+0.0f`. A `−0` element is harmless by construction (the lerp `prev + t·(cur − prev)` maps it to `+0`, and `apply` adds `∓0` to a sum). **0 negative zeros were observed**, so bit-equality holds in fact; the diagonal and `R₃` are asserted bit-equal.
4. **vs-static step ratios** read 5.3 / 45.3 / 61.9× (plan expected ≈ 12 / 25 / 44). The control here is the max of the yaw-0 and yaw-180 parks (as the plan specified), not R2's max over all 24 parks, and the stimulus phase differs; informational only, explanation printed in the probe output. The verdict (vs the same-motion reference) reads 1.0003.
5. **`sqrt2` is a member set in `prepare()`** (`std::sqrt (2.0f)`) rather than a literal — identical bits, and it keeps the float path identical to the validated R1 script (B3's spirit: no constexpr sqrt).
6. **One extra check**: "P1 null rotated liveness: the rotated render differs from the 2.1 null render" — guards the rotated null against a rotation that silently does nothing.
7. **Executor**: the orchestrator session executed T1–T8 directly with Bash (the `dsp-agent` definition has no Bash and cannot run T7), as 2.1 did.

## Deferred (carried forward by name)

- **A14 / D6 placeholder** — `binL = dL; binR = dR` in `SpatialEngine::renderSample` (and the matching two lines in `ReferenceModel`). **Phase 2.3 obligation**: replace with the UPOLS convolver output on the **rotated** `tap`; the 2.3 verify must assert the placeholder is gone.
- **`kStereoLevelMatch`** — `1.0f` placeholder in `StereoDecoder.h`; 2.3's generator emits the real value into `BinauralFilterData.h`.
- **TripleBuffer → `core/triple-buffer` module candidate** — three verbatim consumers; repo-level housekeeping after Stage 2 (`/module-create`).
- **−180 / 180 display** — `wrapTo180` is [−180, 180) (A6); `azRot` at exactly +180 publishes −180 (cardinal (b) printed −180.000 at the pole). Stage 3 may format −180 as 180.
- **Step-response audibility** (R2) — a 180° yaw step reads 5–62× vs a *static* control on bands 1–3 (grid-boundary kinks of the one-pole + per-sample lerp; 1.0003 vs the same-motion reference; identical in kind to the 2.1 azimuth path's response to a 180° azimuth step). **Hands-on at verify:** flick Yaw 0 → 180 at order 3 on a low tone and listen for a zip. v1.1 candidates if the ear objects: a longer τ for *jumps* only; quadratic (prev / cur / next) element interpolation. ARCHITECTURE accepted "20 ms one-pole + per-sample lerp".
- **D1 cost corrected** — ≈ 16 MFLOPS at 48 kHz (332 flops / sample), not 4 (CONTEXT-2.2 counted multiplies only). Still invisible next to the 2.3 convolver (≈ 82 MFLOPS).
- **Signed-zero caveat (R3)** — **not observed**: digest-2.1 reproduced bit-exactly and the identity has 0 negative zeros. Recorded only as the first suspect if the digest ever fails.
- **Gimbal-lock tooltip (F5)** — at θ = ±90° the yaw and roll axes coincide; the bus is still correctly rotated (extremes gate finite and live); the Stage-3 compass holds its last value within ≈ 0.01° of the pole (`MeterFrame.h`). Stage 3 tooltip text.
- **`azEff` readout formatting (Incident 1)** — the frame carries the host's float (−11.999999 for a −12 parameter); the Stage-3 puck readout should format the parameter value.
- **Human items for verify** — installed plugin hands-on: Yaw spins the image while the Generic editor's Azimuth value stays put; Pitch + raises the front; Roll + raises the left; the Yaw flick above. Plus the 2.1 list (pan sign, distance, orbit, default not silent). `tests/pedalboard/verify_2_2.py` (yaw ±90° in Stereo moves the image by the azimuth pan law ≈ 22.88 dB; `yaw = −azimuth` restores `L == R`) is the verify phase's independent measurer (B14).

## Gotchas checklist (PLAN-2.2) — all ticked

- [x] `R₁[i][j] = R₃[p(i)][p(j)]`, `p = (1, 2, 0)`, **no transpose** — the transpose trap FAILs as expected (1.974).
- [x] `u ≠ 0` / `w ≠ 0` guards in place; `w` never evaluated at `m = 0` (`wTab = 0` there and at `|m| ≥ l − 1`); `denom` switches at `|n| = l`.
- [x] `rotIndex` `static_assert`s compile; `blockOffset = {0, 9, 34}`; the recursion reads `R_{l−1}` through `rotIndex (l − 1, …)` on the same array.
- [x] W implicit: `out[0] = in[0]`; never stored, lerped or multiplied — DSP-02 W exact.
- [x] Fused lerp `prev + t·(cur − prev)` with the 2.1 `t`; `bus` and `tap` are distinct members.
- [x] Tap + `shPeak` scan after the rotation; the stereo decoder reads `tap`.
- [x] Three `WrapSmoother`s snapped in `snapToParams`, stepped in `controlStep`; `prevR ← curR` before the rebuild in both.
- [x] Frame: `shGain = apply (curR, curC)`; `azEff / elEff` pre-rotation; fast path when all three `cur == 0`; `clamp` before `asinf`; `wrapTo180` on `azRot`.
- [x] `rot.prepare()` in `SpatialEngine::prepare` before the first `build`; no `sqrt` / allocation in `process` (PERF-01 0).
- [x] `ReferenceModel` never includes `ShRotator.h`; rotates by re-encoding at `dir (R₃·d)`; `RefParams` + both `refParamsFrom` overloads carry the three angles.
- [x] Sweep hook starts at the prepared value (`0.48·(pos/64)` from 0°); 16-channel click gates at az 37 / el −12; front source only for liveness + cross-gate.
- [x] Step gate verdict vs the same-motion reference (1.0003); vs-static numbers printed with the explanation, never asserted.
- [x] Sweep negative control excludes ACN 0 / 2 / 6 / 12; the zonal probe asserts them bit-identical instead (and the step negative control excludes them too — Deviation 2).
- [x] P2b / P3b / `movingSetup` and every 2.1 verdict string untouched; new slicing gates P2c / P3c / P2d; host-side changes in P2d only at `n % 4096 == 0`.
- [x] `digest-2.1 == 9189d911a1fe98d5` is a `check`; both digests reproduce on a second run.
- [x] 0 warnings; `build-and-install.sh` then targeted `auval -v aufx OSAm OuDv`; pluginval s10 ×2 VST3 + AU.
- [x] Temp-index, path-scoped commit; branch + status re-checked immediately before; `PLUGINS.md` untouched.

## Success criteria (for the verify phase)

- [x] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics-render-test` builds with 0 warnings.
- [x] **digest-2.1 == `9189d911a1fe98d5`** — the entire 2.1 probe set PASS and bit-identical at zero rotation (D3); the identity-exact probe PASS.
- [x] DSP-02 < 1e-4 at orders 1 / 2 / 3 over 200 tuples (2.6e-7 / 5.2e-7 / 7.2e-7); orthogonality 5.6e-7 < 1e-5; transpose trap FAILs as expected (1.974); yaw-only closed form 2.5e-7; three cardinal assertions PASS through the frame; `yaw = −azimuth` → `shGain == refSh16 (0, 0)` within 2.2e-8, `azEff` unchanged, `|L − R| = 0`.
- [x] P1 null 4.8e-8 < 1e-5 at (ψ 25, θ −40, φ 70) on a fresh instance; meter frame `shGain / azRot / elRot` match the reference (3.5e-7; 48.5942 / −7.6349), `azEff / elEff` bitEqual to the host parameters under yaw; round trip returns to the bit-exact static render and the identity fast path.
- [x] Yaw-sweep ratio 1.127 ≤ 2.0 on all 16 bus-tap channels with the snapped negative control 49.0–156.4 on m ≠ 0; zonal channels bit-identical; yaw ≡ azimuth 8.9e-8 on a front source; liveness −8.20 dB; yaw-step ratio 1.0003 vs the same-motion reference, negative control 98.3–544.4 FAILs as expected.
- [x] P2c / P3c / P2d bit-equal across slicings with rotation; PERF-01 0 allocations with the three angles moving (liveness 1); extremes finite and live at 48 k / 96 k on ±180° of all three angles and at θ = 90° under a yaw sweep.
- [x] P4 latency still 64 (2.1 probe unchanged, PASS); `auval -v aufx OSAm OuDv` SUCCEEDED; pluginval s10 ×2 VST3 + AU — SUCCESS ×2 each (25 groups).
- [ ] Verify-phase items (not execute): `tests/pedalboard/verify_2_2.py` on the installed VST3; DAW hands-on (Yaw spins the image while Azimuth stays put, Pitch + raises the front, Roll + raises the left, the Yaw 0 → 180 flick listened to for a zip).
- [x] This SUMMARY records digest-2.1 (unchanged), digest-2.2 (`7f35bc9f91e321b9`), every gate literal, and the deferred list.
