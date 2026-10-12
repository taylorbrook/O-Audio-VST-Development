# Stage 2 — DSP · Phase 2.2 (SH rotation + rotated meter tap) — Research

**Plugin:** O-simpleAmbisonics
**Stage:** 2 of 4 — DSP
**Phase:** 2.2 of 3 — scene rotation in the ambisonic domain (yaw / pitch / roll), rotated meter tap
**GSD phase:** research
**Date:** 2026-10-12
**Branch:** `main` @ `1d79e7b3` (CONTEXT-2.2 committed)
**Inputs:** `CONTEXT-2.2.md` (D1–D10, R1–R7), `research/ARCHITECTURE.md` §SH Rotator / §SH rotation (Ivanic–Ruedenberg) / §Visualization Tap / Decisions 5, 9 (immutable), `research/ambisonics-binaural-decoding-deep-dive.md` §5.2–5.3, `Source/dsp/{SpatialEngine,Smoothers,ShEncoder,MeterFrame}.h`, `tests/render-harness/{main.cpp,ReferenceModel.h}`, IEM SceneRotator `calcRotationMatrix` (GitHub mirror `tu-studio/IEMPluginSuite`, read for index conventions only — D9), project memory (`index_dsp_gate_design`)
**Numerics:** every number below was computed, not estimated — scratch scripts `r1_r3_recursion.py`, `r2_r6_lerp_click.py`, `r6_sweep_start.py`, `r_extra.py` (scratchpad; the recursion transcription is reproduced in full in R1 because the executor transcribes it)

---

## Entry check

| Item | State |
|---|---|
| Contract drift | None. ARCHITECTURE.md, ROADMAP.md §2.2, REQUIREMENTS.md unchanged since the 2.2 discuss |
| Engine seam | `SpatialEngine::{prepare, reset, setParams, process, busTap, frame, resetPeaks}` on the persistent 64-sample grid; `Params` carries `yawDeg / pitchDeg / rollDeg` (read, ignored) — unchanged since 2.1 verify |
| Harness | `renderHooked` (processor, `hook (n)` before each block, parameters via `setParam (q, ParamIDs::yaw, …)` — the three IDs exist in `ParamIDs`), `renderEngine` (engine-direct, `hook (pos, osa::Params&)`, ragged `blockSizes`, `recordTaps` logs **only** `busTap()[4]` and `[9]`), `renderRef` (same shape for `ReferenceModel`), `worstCurvature` / `ClickReading` / `kClickRatioCeiling = 2.0` / `checkFailsAsExpected`, rtcheck |
| 2.1 probes that touch 2.2 semantics | `meter frame: azRotDeg == azEffDeg (identity rotation)` is asserted **bitEqual** (main.cpp:1659) — see F1; every other 2.1 probe leaves yaw / pitch / roll at the exact 0° default |

Scope discipline: no algorithm in ARCHITECTURE was re-opened. The findings are (a) the validated recursion the executor transcribes, (b) numbers that turn D2 / D3 / D7 / D8 and the click gates into checkable statements, (c) three hazards found while checking the discuss decisions against source.

---

## Answers to the open questions

### R1 — Recursion validated: 1.8e-15 in double, 1.3e-7 in float32 against re-encoding at 200 random tuples; transpose fails by 1.24; index map agrees with IEM

The own transcription of Ivanic & Ruedenberg (1996) with the 1998 erratum, written from the paper's structure (not from code) and then cross-checked index-by-index against IEM SceneRotator's `P / U / V / W` and `calcRotationMatrix` (structure identical: same three `P` cases, same `u / v / w` with `denom = (2l)(2l−1)` at `|n| = l` else `l² − n²`, same δ factors in `V`, same `Rl(0,0) = rotMat(1,1) … Rl(2,2) = rotMat(0,0)` i.e. `p = (1, 2, 0)`). **This is the code the executor transcribes into `ShRotator.h`** (Python → C++ is mechanical; keep the comments):

```
Paper index m, n ∈ [−l, l]  ↔  ACN l² + l + m.   Order-1 block in ACN order (Y, Z, X):
    R1[i][j] = R3[p(i)][p(j)],  p = (1, 2, 0),  i, j = m + 1 ∈ {0, 1, 2}   (m = −1 → Y = y, 0 → Z = z, +1 → X = x)

P(i, a, b, l):                         # R1 indexed by paper m ∈ {−1, 0, 1}; Rprev = R_{l−1} with centred (a, b)
    |b| < l :  R1[i][ 0] · Rprev[a][b]
    b == l  :  R1[i][ 1] · Rprev[a][l−1]   − R1[i][−1] · Rprev[a][−l+1]
    b == −l :  R1[i][ 1] · Rprev[a][−l+1]  + R1[i][−1] · Rprev[a][l−1]

for l in 2, 3;  for m, n in [−l, l]:
    δ      = (m == 0)
    denom  = (l+n)(l−n)            if |n| < l   else   (2l)(2l−1)
    u      =        sqrt( (l+m)(l−m)                          / denom )
    v      =  0.5 · sqrt( (1+δ)(l+|m|−1)(l+|m|)                / denom ) · (1 − 2δ)
    w      = −0.5 · sqrt( (l−|m|−1)(l−|m|)                    / denom ) · (1 − δ)
    U      = P(0, m, n, l)                                                             (only if u ≠ 0 — |m| = l gives u = 0 and an out-of-range a)
    V      = m == 0 :  P(1,  1, n, l) + P(−1, −1, n, l)
             m  > 0 :  P(1, m−1, n, l) · sqrt(1 + δ_{m,1})  −  P(−1, −m+1, n, l) · (1 − δ_{m,1})
             m  < 0 :  P(1, m+1, n, l) · (1 − δ_{m,−1})     +  P(−1, −m−1, n, l) · sqrt(1 + δ_{m,−1})
    W      = m == 0 :  0                                                               (w = 0 — never evaluated)
             m  > 0 :  P(1, m+1, n, l) + P(−1, −m−1, n, l)                             (only if w ≠ 0 — |m| = l−1 gives w = 0 and an out-of-range a)
             m  < 0 :  P(1, m−1, n, l) − P(−1, −m+1, n, l)
    R_l[m][n] = u·U + v·V + w·W
```

The guards `if (u != 0) … if (w != 0) …` are **not** optimisations: without them `P` reads `R_{l−1}` at `a = ±l` (one past the block). IEM has the same three guards. `v` is never zero for `l ≥ 1`.

Validation (double unless stated; `R3 = Rz·Ry′·Rx` with the ARCHITECTURE matrices; oracle = generic SN3D `refSh16 (dir (R3·d (az, el)))`, the RESEARCH-2.1 R4 table that shares nothing with `encodeSH16`):

| Check, 200 random `(az, el, ψ, θ, φ)` | Order 1 | Order 2 | Order 3 |
|---|---|---|---|
| `R_SH · refSh16 (az, el)` vs `refSh16 (dir (R3·d))`, max abs | 8.3e-16 | 1.3e-15 | **1.8e-15** |
| same with the recursion run entirely in **float32** | 4.1e-08 | 1.2e-07 | **1.3e-07** (gate 1e-4 — 750× margin) |
| orthogonality `|R_lᵀ R_l − I|` | 4.4e-16 | 8.9e-16 | 1.2e-15 |
| worst tuple | `(az −25.35, el 0.25, ψ 160.03, θ 58.51, φ −73.05)`, order 3 | | |

- **Convention trap closed:** at one tuple `|R_SH·c − oracle| = 3.3e-16` while `|R_SHᵀ·c − oracle| = 1.24` — the recursion gives the *active* convention (source at `d` moves to `R3·d`) directly from `R1 = P·R3·Pᵀ`; no transpose anywhere.
- **Independent cross-check (D10 fallback, also written out):** the yaw-only closed form — per order `l`, `Y_{l,−m} / Y_{l,+m}` rotate as `[[cos mψ, sin mψ], [−sin mψ, cos mψ]]` on the `(−m, +m)` pair, `m = 0` unchanged — matches the recursion to 0.0 (orders 1, 2) and 2.2e-16 (order 3) at ψ = 73°. The 2×2 orientation (`+sin` in the `[−m][+m]` slot) is the one that passes; the other sign would be the transposed trap again.
- **D7 cardinal literals through `dir()`:** ψ = +90° maps front → `azRot = +90.0000`; θ = +90° maps front → `elRot = +90.0000`; φ = +90° maps left → `elRot = +90.0000`; `az = +90°, ψ = −90°` → `(0.0000, 0.0000)`. A transposed `Rz` gives `azRot = −90` for ψ = +90 — the literal catches it, as D7 intends.

Precision choice: run the recursion in **float** inside `ShRotator.h` (JUCE-free, same precision as the vector path; 1.3e-7 ≪ 1e-4; identity is exact in float — R3). No double anywhere in `Source/dsp/`.

Cost: the `u / v / w` coefficients depend only on `(l, m, n)` → **precompute them once** (a `constexpr` table of 25 + 49 triples built by a `constexpr` function, or filled in the constructor — either way no `sqrt` on the audio thread). The control step is then ≈ 83 elements × ≤ 3 `P` evaluations × 3 flops ≈ 700 flops + 3 `sin` + 3 `cos` (R3) + 1 `atan2` + 1 `asin` (frame) per 64 samples. Per sample (D1): 83 element lerps + 83 MACs = **332 flops ≈ 15.9 MFLOPS at 48 kHz** — CONTEXT D1's "≈ 4 MFLOPS" counted the multiplies only; still invisible next to the 2.3 convolver, and D1 stands.

### R2 — Chord error: −0.41 dB band-3 norm dip at 11.6°/grid, −0.001 dB at 0.48°/grid; the same lerp as the reference model, so the STEP gate must be matched against the same-motion reference, not a static render

**(a) Norm dip of the mid-grid lerped matrix.** Closed form: for a per-grid rotation step δ about any axis, `(R_prev + R_cur)/2 = R_prev·(I + Q)/2` with `Q` the step rotation, whose eigenvalues in the order-l representation are `e^{imδ}`, so the worst gain is `cos (l·δ/2)`. Numeric (smallest singular value of the midpoint block from the recursion):

| δ per grid | Band 1 | Band 2 | Band 3 |
|---|---|---|---|
| 11.6° (first grid of a 180° step through the one-pole, k = 0.0645) | −0.045 dB | −0.179 dB | **−0.407 dB** |
| 0.48° (360°/s sweep) | −0.000 dB | −0.000 dB | −0.001 dB |
| pitch 7.3° + roll 8.9° (net 11.51°, not yaw) | | | −0.400 dB (closed form −0.400) |

Axis-independent, as the closed form says. Same class as RESEARCH-2.1 R3's vector-lerp numbers (0.96°/grid → −0.002 dB). Nothing to split: the D5-amendment grid already made the chord negligible for every *continuous* motion.

**(b) What the ≤ 2.0 second-difference ratio sees — the step is a different animal.** Simulated op-for-op (one-pole on the wrapped yaw per grid, 83-element lerp per sample, 100 Hz tone amplitude 0.5, order 3, source az 37° / el −12°, d = 1 m, float32). Control = the same tone with the rotation **parked** at every yaw on a 15° grid, per channel the largest curvature any park produced (the "largest legitimate curvature", as DSP-06b's ±60° parks do for L / R):

| Event | Worst engine ÷ static control | Engine ÷ same-motion reference (vector lerp of `refSh16 (dir (R·d))`) | Snapped-per-grid reference ÷ static |
|---|---|---|---|
| **0° → 180° yaw step** | band 1 **12.3×**, band 2 **24.7×**, band 3 **43.6×** (ACN 15) | **1.0000** on all 15 channels | 64–126× (FAILS as it must) |
| 360°/s sweep, started at the prepared value | **1.127×** (ACN 9) | 1.0007 | 49–156× on the m ≠ 0 channels |

The step numbers are not a rotator defect and not a lerp defect. `lerp (R)·c == lerp (R·c)` exactly (linearity), and the simulation confirms `max |engine bus − reference bus| = 1.3e-7` over the whole step. What the metric sees is the **kink at each grid boundary** of a piecewise-linear interpolation of the one-pole's exponential: slope change ≈ `(l·δ_k)²·|c|/64` per sample with `δ_1 = 11.6°`, i.e. `(3 × 0.2 rad)² / 64 ≈ 0.006` × 0.4 ≈ 2.6e-3 against the tone's own curvature `A·ω² = 8.6e-5` — the worst sample sits at grid phase 63, exactly one grid after the step. **The 2.1 azimuth path has the identical response to a 180° azimuth step** (same one-pole, same vector lerp); 2.1 never click-gated a step, only the trajectory (DSP-06a) and the sweep (DSP-06b). So:

- **Yaw-step gate verdict = engine bus tap vs the ReferenceModel driven by the same step** (lerped), ratio ≤ 2.0 on all 16 channels — exactly the idiom the order-fade gate (b) already uses ("vs the reference model's tap (same ramps)"), expected ≈ 1.000. **Negative control** = the same reference with `snapCoefficientsPerGrid` (64–126×, FAILs as expected). The vs-static ratios are **printed informationally** with this paragraph's explanation in the probe comment, so the SUMMARY does not read 44× as a failure.
- **Yaw-sweep gate verdict = engine bus tap vs the static parked control**, per channel (24 parks at 15°, max per channel — one engine-direct render each, cheap), ratio ≤ 2.0 — this is the gate that *would* see a chord or lerp defect (the same-motion reference cannot, it shares the chord). Expected 1.13 at ACN 9. Negative control = snapped reference vs static on the **m ≠ 0 channels only** (ACN 0, 2, 6, 12 are yaw-invariant and read exactly 1.0 — see F3), ≥ 49×.
- Audibility of the step response is a **hands-on item for verify** (flick Yaw 0 → 180 at order 3 on a low tone). The interpolation error vs the smooth exponential is ≤ `f″·h²/8 ≈ 4.6 %` of the band-3 element during the first grid (a −27 dB ripple at 750 Hz for ≈ 100 ms). ARCHITECTURE accepted "20 ms one-pole + per-sample lerp"; if the ear disagrees, the v1.1 candidates are a longer τ for *jumps* only or a quadratic (prev / cur / next) element interpolation — both out of this phase. Record in SUMMARY-2.2 §Deferred.

Pitch and roll 180° steps are the same class, smaller (worst absolute curvature yaw 2.58e-3, pitch 1.30e-3, roll 8.79e-4 for this source) — one yaw-step gate suffices; pitch / roll ride on P2b / P3b and the extremes.

### R3 — Identity is EXACT in float: every element of all three blocks is bit 0 or 1; D3's digest verdict stands

Run with the recursion in float32 at `R1 = I`: orders 1, 2, 3 — **every element exactly 0.0f or 1.0f**, no residual anywhere (`buildCartesian (0, 0, 0)` in float is exactly `I` too: `cosf (0) = 1`, `sinf (0) = 0`). Mechanism, so the executor can keep it: at identity every `P` is a product of an exact 0 / 1 of `R1` with an exact 0 / 1 of `R_{l−1}`; the diagonal element collects exactly one nonzero term and the coefficient in front of it is `sqrt` of a perfect ratio (`u = sqrt ((l+m)(l−m)/((l+n)(l−n)))` with `m = n` → `sqrt (1)`; at `|m| = |n| = l`, `v·V = ½·sqrt (2·(2l−1)(2l)/((2l)(2l−1)))·(sqrt 2·1) = ½·sqrt 2·sqrt 2 = 1` — `sqrtf (2.f) * sqrtf (2.f)` is **exactly** 2.0f in IEEE single (1.4142135·1.4142135 rounds to 2). The off-diagonals are sums of exact zeros. Also exact: the m = 0 rows under a **pure yaw** (F3).

Signed-zero caveat (the only way bit-transparency can break): `out_i = Σ_j R_ij·in_j` at identity is `1·in_i + Σ 0·in_j`; `0·in_j` is `−0` when `in_j < 0`, and `(−0) + (+0) = +0`, so a bus channel that is exactly `−0` (e.g. Z with `el = 0` and a negative sample) may come out `+0`. That flips no 2.1 verdict: `== 0.0f` and `abs` treat them alike, the stereo decoder adds a non-zero W term, the P4 / P5 renders compare values not bits, and the digest render (P1 null, noise, az 37 / el −12, order 3) has no exactly-zero bus channel. **If digest-2.1 nevertheless differs, suspect a signed zero before anything else** and compare the two renders with `memcmp` to locate the sample.

`±180°`: `sinf (π_f) = −8.74e-8`, so a ±180° angle is not an exact half-turn — finite, irrelevant (extremes gate).

### R4 — `dir()`: `atan2 (y, x)` / `asin (clamp (z))` round-trips to 9e-13°; at the pole the azimuth is noise, so the pitch / roll cardinal checks assert `elRot` only

- `d (az, el) = (cos e·cos a, cos e·sin a, sin e)`; `dir (d) = (atan2 (y, x), asin (z))` in degrees. Round trip over 1000 random points (|el| ≤ 89.9°): worst **9.1e-13°**. `+y` is `+90°` (left) by construction — the same sign the DSP-01 anchor `Y (ACN 1) > 0 at az +90°` pins on the encoder side, so the oracle `refSh16 (dir (R3·d))` and the engine agree on "left" from two independent sources.
- **Pole:** `R3·d` at `|elRot| → 90°` has `x′, y′` at rounding-noise level and `atan2` returns an arbitrary angle — the script's "roll +90° on a left source" case reads `azRot = +45.0` from 1e-17 residuals; the signed-zero cases `atan2 (±0, ±0)` give 0, ±180. Therefore the harness D7 assertions for θ = +90° and φ = +90° compare **`elRot` only** (`azRot` printed), and `MeterFrame.h` documents that `azRot` is undefined within ≈ 0.01° of the pole (Stage 3 compass holds its last value there). `clamp (z′, −1, 1)` before `asin` is mandatory: `|R3·d|` can exceed 1 by an ulp.
- Mirrored-builder trap: both DSP-02 sides use *different* builders (engine `ShRotator::buildCartesian` in float; harness `ReferenceModel` builder in double, written separately from the ARCHITECTURE matrices), and D7 compares the engine frame to **literals** — a transposed engine `Rz` gives `azRot = −90` for ψ = +90 and fails D7 even though a tuple gate with the same transposed builder on both sides would pass.

### R5 — 83-element layout: offsets 0 / 9 / 34, `k (l, m, n) = off (l) + (m + l)(2l + 1) + (n + l)`, no scratch — the recursion reads `R_{l−1}` through the same centred accessor

```
inline constexpr int kNumRotElems = 83;
constexpr int blockOffset (int l) noexcept { return l == 1 ? 0 : l == 2 ? 9 : 34; }   // 9 + 25 + 49 = 83
constexpr int rotIndex (int l, int m, int n) noexcept                              // paper m, n ∈ [−l, l]
{ return blockOffset (l) + (m + l) * (2 * l + 1) + (n + l); }
static_assert (blockOffset (3) + 49 == kNumRotElems && rotIndex (3, 3, 3) == 82 && rotIndex (2, -2, -2) == 9, "…");
```

- Row / column `m` of block `l` is ACN `l² + l + m`; `apply`: `out[0] = in[0]` (W implicit, D2), then `for l: for m: acc = Σ_n R[rotIndex (l, m, n)] · in[l² + l + n]; out[l² + l + m] = acc`. Row-major per block makes the inner loop contiguous.
- The recursion needs `R_{l−1}[a][b]` with centred indices — `R[rotIndex (l − 1, a, b)]` on the **same** array (the `prev`/`cur` 83-vectors hold complete `R1, R2, R3`; the control step builds `cur` in place: `R1` from `R3`, then `R2` from `R1`, then `R3` from `R1, R2`). No `(2l−1)²` scratch.
- `shGain = R_SH·c_eff` for the frame = one `apply (curR, curC, frame.shGain)` per control step.
- The engine's per-sample lerp over 83 contiguous floats is a tight loop the compiler vectorises; keep `prevR[83]`, `curR[83]`, and a local `rt[83]` **or** fuse lerp-and-apply (`acc += (prev + t·(cur − prev))·in`) — the fused form avoids the 83-float temporary and is what D2 describes; either is bit-identical across slicing (same ops at the same absolute positions).

### R6 — Sweep stimulus: start the sweep AT the prepared value; liveness 8.2–8.3 dB on a front source; a front source leaves six channels dead, so the 16-channel click gate uses el ≠ 0

Two stimulus traps found in simulation, both of the `pattern_gate_stimulus_below_threshold_is_vacuous` family:

1. **The 2.1 DSP-06b idiom hides a 180° step at the head of the render.** Its hook sets `azimuth = −180` at `n = 0` on an engine *prepared* at 0°, so the first 1024-sample skip contains a one-pole step, and after 16 grids **61.9° are still unsettled, moving 4.0°/grid** against the sweep's 0.48°/grid. Band 1 lives with it (the kink is 9× smaller than band 3's — the stereo gate passed honestly at ≈ 1); on the 16-channel bus-tap gate it reads **4.6× at ACN 9** — a false FAIL. Fix: `yaw = wrapTo180 (0.48 · (n / 64))` so the sweep starts where `prepare` snapped it (0°); worst ratio then 1.127. Informational for 2.1: the azimuth sweep carried the same hidden step and passed only because Stereo sees band 1; no action in 2.2 beyond a comment.
2. **A front / el 0 source under a yaw sweep leaves ACN 2, 5, 7, 10, 12, 14 exactly zero** (every `sin e` term) — a click gate on them is vacuous. The 16-channel gate therefore uses the P1 tuple **az 37° / el −12°** (every channel ≥ 259 energy units over the sweep, min at Z); the **Stereo liveness** (≥ 6 dB half-circle swing) and the **yaw ≡ azimuth cross-gate** use a **front** source in a second render.

Numbers: one-pole trailing lag at 1 rev/s `atan (2π·1·0.02) = 7.16°` (constant — fine); Stereo L energy second half-circle vs first with the front source, sweep 0 → 360 (left first, then right): **−8.18 dB** (|·| ≥ 6 ✓ — the 2.1 gate's `std::abs` is needed because the half order is reversed vs the −180 start); with the az-37 source only +2.97 dB (so liveness must not be asserted on that render). **Cross-gate (new, cheap, binds 2.2 to 2.1's proven path):** a front source under the yaw sweep must equal the same source under the identical *azimuth* sweep within **1e-5** on all 16 bus channels (simulated 1.5e-7 float32) — the 2.2 way of saying "yaw of a front source *is* azimuth", independent of any rotation matrix in the reference.

### R7 — No new render infrastructure; one helper: a 16-channel tap recorder (the current `recordTaps` logs only ACN 4 and 9)

- Processor path: `renderHooked`'s hook already calls `setParam (*p, ParamIDs::yaw / pitch / roll, …)` — the IDs exist and `fillParams` copies them; P2b / P3b's `movingSetup` lambda gains `yaw / pitch / roll` automation through `renderWith`'s hook-less driver **only if** it is static — so the "moving rotation" slicing gate uses `renderHooked` with the ragged `blockSizes` form or, simpler, `renderEngine` (engine-direct, `hook (pos, osa::Params&)` already mutates `yawDeg` etc.) at `{64}` vs `{1, 7, 64, 333, 4096, 13, 512, 2}` — `memcmp` equal with yaw / pitch / roll ramping per block and orbit On. Processor-level: add the three IDs to the existing P2b `movingSetup` with a static non-zero tuple (the rotation is then static but non-identity — a different, also useful gate) **and** one `renderHooked (…, block = 64)` vs `renderHooked (…, block = 4096)` pair with the hook ramping yaw (the DSP-06b orbit-driven 64 vs 4096 bit-equality idiom).
- PERF-01 window: add `case` lines moving yaw / pitch / roll between blocks (the existing `switch (k)`), nothing else.
- **The one helper:** `EngineRender` / `RefRender` gain `std::vector<float|double> tap16` (stride 16) filled when `recordTaps` is set, next to `tap4 / tap9` (keep those — the 2.1 order-fade probe reads them). `worstCurvature (tap16, ch, 16, from, to)` then gives the per-channel readings for free. `ReferenceModel::busTap()` already exists.
- `refParamsFrom (…)` (both overloads) copies the three angles; `RefParams` gains `yawDeg / pitchDeg / rollDeg`.
- Meter-frame probe: `p->meterFrames.read()` after a static render — the new D7 / D8 checks read `shGain`, `azRotDeg`, `elRotDeg`, `azEffDeg` the same way.

---

## Findings beyond R1–R7

### F1 — The 2.1 probe `azRotDeg == azEffDeg` is asserted bitEqual, and `atan2` does not round-trip in float: the frame direction needs an identity fast path *(HIGH — a 2.1 check fails otherwise)*

`main.cpp:1659`: `check ("meter frame: azRotDeg == azEffDeg (identity rotation)", bitEqual (…))`. D8 computes `azRot = atan2 (y′, x′)` from `R3·d (azEff, elEff)`; at identity `R3·d = d` exactly (R3), but **`atan2f (sinf·cosf, cosf·cosf)` ≠ the input angle in float**: az 37° → `36.999992`, az −170° → `−169.99998` (exact at 0, 45, 90). The 2.1 probe reads the frame with the orbit running (az arbitrary) → FAIL. Options: (a) **when all three smoothed angles are exactly 0 (the snapped identity), write `azRot = azEff, elRot = elEff` and skip the two inverse-trig calls** — a legitimate identity fast path for the *frame only* (the matrix is still built and applied, D2's "always applied" is about the bus); (b) edit the 2.1 probe to a 1e-4 tolerance. Recommend **(a)**: the 2.1 probe text stays untouched (D3's spirit), and it is also exactly what the UI wants (the compass sits on the puck at zero rotation, not 1e-5 off it). `elRot = asinf (sinf (e))` happens to round-trip at the tested angles but is not guaranteed — the fast path covers it too.

### F2 — The wrap smoothers snap at `|Δ| ≤ 1e-6·max (1, |target|)`; the identity fast path and D3 need "exactly 0" — they get it *(confirms, no change)*

A yaw moving back to 0° arrives at `cur = wrapTo180 (0) = 0.0f` exactly via the A2 snap (`cur = wrapTo180 (t)`), so the F1 fast-path test `yaw.cur == 0 && pitch.cur == 0 && roll.cur == 0` is reachable after any motion, and `buildCartesian (0, 0, 0)` is then exactly `I` (R3) → `curR` exactly identity → after one more control step `prevR == curR` → a static render after a rotation round-trip is bit-identical to a fresh render (D2's claim holds through the recursion).

### F3 — Under a pure yaw the m = 0 channels (ACN 0, 2, 6, 12) are bit-identical to the static render — a free physics gate and a negative-control exclusion *(MEDIUM — shapes the sweep gate)*

Float32 recursion over 721 yaw values: the m = 0 row of every block is **exactly** `e₀` (worst deviation 0.0), and the simulated moving bus on ACN 2 / 6 / 12 is **bit-identical** to the parked bus. Two consequences: (1) the sweep's snapped negative control reads exactly 1.0 on those channels — assert it on the m ≠ 0 channels (min 49× at ACN 13); (2) add the one-line positive gate "yaw sweep leaves ACN 0 / 2 / 6 / 12 bit-identical to the parked render" — it is the statement *rotation about z preserves the zonal harmonics*, costs nothing, and would catch an index error that mixed m = 0 into m ≠ 0 rows (a class the DSP-02 tuple gate also catches, but this one names the physics).

### F4 — Per-channel static controls: 24 parks at 15° give ≥ 93 % of each channel's peak coefficient *(LOW — sizing the control)*

Band-3 channels rotate at `3ψ`; parks every 15° sample `sin (3·az + 45°·k)`, worst case 0.934 of the peak → the control is at most 7 % low → ratio inflated by ≤ 1.07. Fine under 2.0 (expected 1.13 incl. this). Engine-direct, 24 × 1 s renders ≈ 1.2 M samples — negligible harness time.

### F5 — Gimbal lock is a display matter, not a DSP one *(LOW — Stage 3 note)*

At θ = ±90° the yaw and roll axes coincide; `R3` is still a proper rotation (no NaN, orthogonality holds — the DSP-02 tuples include |θ| near 90 by chance), the bus is correct, only the UI-facing decomposition is degenerate. Note for the Stage-3 tooltip; no gate beyond extremes.

### F6 — `ReferenceModel` extension is small and stays an oracle *(informs the plan)*

`RefParams` + three `Wrap` smoothers stepped with the same `kOnePole` and snap; `buildR3 (ψ, θ, φ)` in double from the ARCHITECTURE matrices (written in `ReferenceModel.h`, not shared with `ShRotator.h`); `evaluateC()` becomes `refSh16 (dir (R3·d (az.cur, el.cur)))` × gains. ~40 lines. The negative-control switches are untouched (`snapCoefficientsPerGrid` is the yaw zipper by construction). `currentC()` now returns the *rotated* vector — the DSP-02 probe uses the static `refSh16 (az, el)` directly, not `currentC()`.

---

## Reuse — existing code and precedent

| Source | Reuse | Notes |
|---|---|---|
| `Source/dsp/Smoothers.h` `WrapSmoother` | ×3 for yaw / pitch / roll, as-is | D4; snap rule gives exact 0 (F2) |
| `Source/dsp/ShEncoder.h` `kDegToRad`, `kNumSh`, `shBand` | `ShRotator.h` includes it | `buildCartesian` takes degrees × `kDegToRad` |
| IEM SceneRotator `calcRotationMatrix` / `P / U / V / W` | **Read for index conventions only** (D9) — structure confirmed identical to the own transcription above, `p = (1, 2, 0)` confirmed | GPL-3.0; nothing lifted |
| `tests/render-harness/main.cpp` `renderEngine` / `renderRef` / `renderHooked` | Unchanged drivers + the `tap16` helper (R7) | |
| `tests/render-harness/main.cpp` order-fade gate (b) | Idiom for the yaw-step gate (engine tap vs reference same-motion, negative control snapped) | R2 |
| `tests/render-harness/main.cpp` DSP-06b | Idiom for the yaw-sweep gate, **with the start-at-prepared-value fix** (R6) | |
| `ReferenceModel.h` `refSh16` | DSP-02 oracle and the harness `dir()` | D6 |
| `modules/` registry | **No applicable module** (v1.2.1: nothing covers SH rotation) | |

No new third-party dependency.

---

## Pitfalls carried from the knowledge base

| Pattern | Bites where in 2.2 |
|---|---|
| `pattern_gate_stimulus_below_threshold_is_vacuous` | R6 (1) hidden 180° step in the sweep head; R6 (2) six dead channels on a front source; F3 m = 0 channels in the negative control |
| `pattern_zipper_sweep_probe_needs_liveness_gate` | R6 — Stereo liveness on the front-source render only (+2.97 dB on the az-37 render would fail a ≥ 6 dB gate for the wrong reason) |
| `pattern_zipper_gate_absolute_step_is_gain_staging_dependent` | R2 — ratios vs a matched control; the step's control is the same-motion reference, never an absolute threshold |
| `pattern_test_fixture_mirrors_drift_silently` | D6 / R4 — the reference rotates by re-encoding through its own double builder; two builders, literals in D7 |
| `pattern_distinct_buffer_addresses_are_not_an_allocation_bound` | R2 negative controls (64–126× step, ≥ 49× sweep) — the detector is shown to fail before it is believed |
| `pattern_exact_cycle_gate_needs_exactly_representable_f0` | 100 Hz tone (480 samples) unchanged |
| `pattern_gain_ramp_without_seeding_fades_in_from_silence` | `snapToParams` snaps the three wrap smoothers and builds `prevR = curR` before block 0 |
| `pattern_downstream_guard_masks_defect_gate_the_clamp_target` | R4 — `clamp (z′, −1, 1)` before `asin` is a guard; the DSP-02 gate reads the unclamped `R3·d` through `refSh16 (dir (…))` on the oracle side, so a norm error in `R3` still fails orthogonality |
| `pattern_gate_inherits_the_review_findings_scope` | R2 — the step gate is on all 16 channels, not the two the order-fade probe records (hence `tap16`) |
| `pattern_oversampler_overruns_on_oversized_host_block` | Unchanged — no new scratch; P3b with rotation moving |
| `critical_asan_runtime_hangs_at_init_on_macos26` | rtcheck counter, not ASan, for PERF-01 with rotation moving |

---

## Open items handed to the plan phase

1. **Transcribe R1 verbatim** into `Source/dsp/ShRotator.h` (float; `constexpr` `u / v / w` table; the three guards; `P` with the three cases; `p = (1, 2, 0)`; `rotIndex` / `blockOffset` with the `static_assert`s of R5; `apply` with W implicit).
2. **Frame direction identity fast path (F1):** `if (yaw.cur == 0 && pitch.cur == 0 && roll.cur == 0) { azRot = azEff; elRot = elEff; } else { atan2 / asin (clamp) }` — keeps `main.cpp:1659` untouched.
3. **Yaw-step gate = vs same-motion reference**, negative control snapped; vs-static ratios printed informationally with the R2 explanation. **Yaw-sweep gate = vs static per-channel parks (24 × 15°)**, start at the prepared value, az 37 / el −12, negative control on m ≠ 0 channels; separate front-source render for the Stereo liveness (≥ 6 dB, expected 8.2) and the **yaw ≡ azimuth cross-gate** (≤ 1e-5 on 16 channels).
4. **F3 positive gate:** ACN 0 / 2 / 6 / 12 bit-identical to the parked render under a pure yaw sweep.
5. **D7 cardinal assertions:** ψ = +90° → `azRot` literal +90 (and `elRot` 0); θ = +90° and φ = +90° → `elRot` literal +90 **only** (R4 pole); `az +90 / ψ −90` → `shGain == refSh16 (0, 0)` within 1e-5 at order 3 and Stereo `L == R`.
6. **DSP-02 gate** as CONTEXT states it, with the expected literal ≈ 2e-7 printed beside the 1e-4 ceiling; orthogonality < 1e-5 (expected ≈ 1e-7 in float).
7. **Harness helper `tap16`** on `EngineRender` / `RefRender` (R7); `RefParams` + `refParamsFrom` gain the three angles; `ReferenceModel` extension per F6.
8. **P2b / P3b extension:** engine-direct ragged vs flat with yaw / pitch / roll ramping + orbit On (`memcmp`), processor `renderHooked` 64 vs 4096 with a yaw ramp (`memcmp`); PERF-01 `switch` gains the three angles.
9. **SUMMARY-2.2 deferred list** gains: step-response audibility (hands-on at verify; v1.1 candidates named in R2), D1 cost corrected to ≈ 16 MFLOPS, signed-zero caveat (R3) if ever observed.
10. **`MeterFrame.h` comment:** `azRot` undefined within ≈ 0.01° of the pole; `shGain` / `shPeak` post-rotation from this phase.

## Still-open manual gates

Unchanged from 2.1 verify (installed-plugin hands-on: pan sign, distance, orbit, default not silent) plus, new for 2.2 verify: Yaw spins the image while the Generic editor's Azimuth value stays put; Pitch + raises the front, Roll + raises the left; **a Yaw flick 0 → 180 at order 3 on a low tone — listen for a zip** (R2). COMPAT-02 / QUAL-02 repeat these formally at Stage 4.
