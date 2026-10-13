# Stage 2 — DSP · Phase 2.2 (SH rotation + rotated meter tap) — PLAN

**Plugin:** O-simpleAmbisonics · **Stage:** 2 DSP · **Phase:** 2.2 of 3 · **Date:** 2026-10-12
**Inputs:** `CONTEXT-2.2.md` (D1–D10), `RESEARCH-2.2.md` (R1–R7, F1–F6, open items 1–10), `research/ARCHITECTURE.md` §SH Rotator / §SH rotation (Ivanic–Ruedenberg) / §Visualization Tap (immutable), `ROADMAP.md` §Phase 2.2, the 2.1 source + harness (`SUMMARY-2.1.md`, `VERIFICATION-2.1.md`)
**Executor:** dsp-agent
**Branch:** `main` @ `273a146a` (RESEARCH-2.2 committed)

## Goal

Drop `ShRotator` into the 2.1 engine so the **scene** rotates after encoding: per control step the three wrap-smoothed angles build `R₃ = Rz(ψ)·Ry′(θ)·Rx(φ)`, the order-1 block is the ACN permutation of `R₃`, the 5×5 and 7×7 blocks come from the validated Ivanic–Ruedenberg recursion (RESEARCH R1), and the 83 nonzero elements are lerped per sample between the previous and current step and applied to the 16-channel bus (W implicit). The bus tap and the peak scan move **after** the rotation; the meter frame publishes `shGain = R_SH·c_eff` and the rotated direction `azRot / elRot` while `azEff / elEff` stay pre-rotation. **Zero rotation is bit-transparent**: the entire 2.1 probe set runs unchanged and its digest `9189d911a1fe98d5` is this phase's regression **verdict**. Latency stays exactly 64; `setLatencySamples` is not touched; the 12 parameters are not touched.

Covers (gated in T7): FUNC-04 (yaw must; pitch / roll should), DSP-02, FUNC-01 third box (`yaw = −azimuth` → front), DSP-06 (extended to the three angles), PERF-01 (re-gated with rotation moving), QUAL-01 (partial: ±180° on all three angles).

## Decisions adopted from RESEARCH-2.2 (planner picks — each is binding on the executor)

| # | Decision | Source |
|---|---|---|
| **B1 — recursion as validated** | Transcribe RESEARCH-2.2 R1 **verbatim** into `ShRotator.h`: paper indices `m, n ∈ [−l, l]`, `P(i, a, b, l)` with its three cases, `u / v / w` with `denom = (2l)(2l−1)` at `|n| = l` else `(l+n)(l−n)`, the three guards (`u ≠ 0`, `w ≠ 0`, and `w` never evaluated at `m = 0`), `R₁ = P·R₃·Pᵀ` with `p = (1, 2, 0)`. **No transpose anywhere** (the active convention falls out of `R₁` directly — R1's 1.24 trap) | R1 |
| **B2 — float throughout** | The recursion, `buildCartesian`, the lerp and `apply` run in **float** (`sinf / cosf / sqrtf`); no `double` in `Source/dsp/`. Validated 1.3e-7 vs the 1e-4 gate; identity is exact in float (R3: `sqrtf(2)·sqrtf(2) == 2.0f`) | R1, R3 |
| **B3 — coefficient table** | `u / v / w` depend only on `(l, m, n)`: fill three `float[83]` member tables **once** in `ShRotator::prepare()` (called from `SpatialEngine::prepare`) with `std::sqrt` on float arguments, indexed by `rotIndex`. No `sqrt` on the audio thread; no `constexpr` sqrt (keeps the float path identical to the validated script) | R1 |
| **B4 — 83-element layout** | `kNumRotElems = 83`, `blockOffset(l) = {0, 9, 34}`, `rotIndex(l, m, n) = blockOffset(l) + (m + l)(2l + 1) + (n + l)`, both `constexpr` with the R5 `static_assert`s. Row-major per block; the recursion reads `R_{l−1}` through `rotIndex(l − 1, a, b)` on the **same** array; `cur` is built in place `R₁ → R₂ → R₃`. No scratch | R5 |
| **B5 — fused lerp-and-apply** | Per sample: `out[0] = in[0]`; for `l = 1..3`, `m`: `acc = Σ_n (prev[k] + t·(cur[k] − prev[k]))·in[l² + l + n]`, `k = rotIndex(l, m, n)`; `out[l² + l + m] = acc`. A3 form, A5 `t`; no `rt[83]` temporary. All three blocks always applied (D2) | R5, D2 |
| **B6 — identity fast path for the frame direction only** | In `fillFrameState`: `if (yaw.cur == 0 && pitch.cur == 0 && roll.cur == 0) { azRot = azEff; elRot = elEff; } else { D8 trig }`. The matrix is still built and applied — the fast path exists because `atan2f` does not round-trip (37° → 36.999992) and `main.cpp:1659` asserts `azRot == azEff` bitEqual. The snap rule gives exact 0 after any motion (F2) | F1, F2 |
| **B7 — D8 trig at the control step** | `d = (cos e·cos a, cos e·sin a, sin e)` from `az.cur / el.cur` (radians via `kDegToRad`); `d′ = R₃·d` with the `R₃` kept from the build; `azRot = wrapTo180(atan2f(y′, x′)·kRadToDeg)`, `elRot = asinf(clamp(z′, −1, 1))·kRadToDeg`. `clamp` is mandatory (|d′| can exceed 1 by an ulp). Comment in `MeterFrame.h`: `azRot` undefined within ≈ 0.01° of the pole | R4 |
| **B8 — step gate vs the same-motion reference; sweep gate vs static parks** | Yaw-**step** verdict = engine `tap16` vs the `ReferenceModel` driven by the same step (lerped), ratio ≤ 2.0 on all 16 channels (expect 1.000); negative control = snapped reference (expect 64–126×); vs-static ratios **printed informationally** with the R2 explanation. Yaw-**sweep** verdict = engine `tap16` vs 24 engine-direct parks at 15°, per channel the max park curvature, ratio ≤ 2.0 (expect 1.13 at ACN 9); negative control = snapped reference on the **m ≠ 0 channels** (expect ≥ 49×) | R2, F3, F4 |
| **B9 — sweep stimulus** | Sweep hook `yaw = wrapTo180(0.48f·(float)(n / 64))` — starts **at the prepared value 0°** (the 2.1 DSP-06b `−180 + …` idiom hides a 180° step and reads 4.6× on band 3). Click gates on the 16-channel tap use **az 37° / el −12°** (every channel live); the Stereo liveness (≥ 6 dB, expect 8.2) and the yaw ≡ azimuth cross-gate (≤ 1e-5 on 16 channels) use a **front** source in separate renders | R6 |
| **B10 — slicing gates with rotation** | A host-side parameter can only be bit-equal across slicings when every change lands on a block boundary of **both** renders. So: **P2c / P3c** = the P2b / P3b shape with a static non-identity tuple + orbit On (ragged / oversized ≡ flat); **P2d** = processor `renderHooked` 64 vs 4096 with yaw / pitch / roll stepped **only at `n % 4096 == 0`** (bit-equal, plus a liveness that the render differs from P2c's). The 2.1 P2b / P3b and their `movingSetup` are **not edited** | R7 |
| **B11 — reference model stays the oracle** | `ReferenceModel` gains `Wrap yaw, pitch, roll`, its own `buildR3` (double, from the ARCHITECTURE matrices, written in `ReferenceModel.h`), `dir()`, and re-encodes at `dir(R₃·d)`. It never contains `R_SH`. `currentC()` becomes the rotated vector; the DSP-02 probe uses `refSh16(az, el)` directly for its left-hand input | D6, F6 |
| **B12 — digest-2.1 is a `check`** | `fnv1a64(nullRender) == 0x9189d911a1fe98d5ULL` is asserted, not only printed. If it fails, compare the two renders with `memcmp` and **suspect a signed zero first** (R3) before anything else; a genuine signed-zero flip is recorded as a deviation and the verdict falls back to a `< 1e-6` null against a 2.1 render (D3) | D3, R3 |
| **B13 — what 2.1 code may change** | `main.cpp` 2.1 probe **bodies and verdict strings are untouched** except: PERF-01's `switch` gains three angle cases (its name string updated), `refParamsFrom` copies the three angles, `EngineRender / RefRender / renderEngine / renderRef` gain `tap16`, the header comment lists the 2.2 probes. `ReferenceModel` extension must leave every 2.1 reference render bit-identical at zero rotation (its `evaluateC` at `R₃ = I` is `refSh16(dir(d))` — `dir()` round-trips to 9e-13°, so the 2.1 P1 null stays ≈ 4.6e-8) | D3 |
| **B14 — verify-phase deliverables stay out of execute** | `tests/pedalboard/verify_2_2.py` and the DAW hands-on belong to `/plugin-verify`; execute ends at the harness + `build-and-install` + `auval -v` + pluginval | CONTEXT scope 5 |

---

## The rotation control step (shared spec for T2 and T4 — the model must match it op-for-op)

```
snapToParams():                                    (prepare / reset — after the 2.1 snaps)
  yaw.snapTo(params.yawDeg); pitch.snapTo(params.pitchDeg); roll.snapTo(params.rollDeg)
  rot.build(yaw.cur, pitch.cur, roll.cur, curR, curR3)      // R3 + 83 elements
  prevR ← curR

controlStep():                                     (after the 2.1 steps, before fillFrameState)
  yaw.step(params.yawDeg, kOnePole); pitch.step(…); roll.step(…)      // three WrapSmoothers, D4
  prevR ← curR
  rot.build(yaw.cur, pitch.cur, roll.cur, curR, curR3)

rot.build(ψ, θ, φ, R[83], R3[3][3]):
  R3 = Rz(ψ·kDegToRad) · Ry′(θ·kDegToRad) · Rx(φ·kDegToRad)        // ARCHITECTURE matrices:
        Rz  = [[c,−s,0],[s,c,0],[0,0,1]]   Ry′ = [[c,0,−s],[0,1,0],[s,0,c]]   Rx = [[1,0,0],[0,c,−s],[0,s,c]]
  R[rotIndex(1, i, j)] = R3[p(i+1)][p(j+1)],  p = (1, 2, 0),  i, j ∈ {−1, 0, 1}      // ACN (Y, Z, X)
  for l in 2, 3:  for m, n in [−l, l]:  R[rotIndex(l, m, n)] = u·U + v·V + w·W        // R1 formulas, B1 guards

fillFrameState():
  ShRotator::apply(curR, curC, meterFrame.shGain)                     // R_SH · c_eff (W passes)
  azEff = az.cur; elEff = el.cur                                      // pre-rotation (UI-02)
  if (yaw.cur == 0 && pitch.cur == 0 && roll.cur == 0): azRot = azEff; elRot = elEff      // B6
  else: d = (cos e cos a, cos e sin a, sin e); d′ = curR3·d
        azRot = wrapTo180(atan2f(d′y, d′x)·kRadToDeg); elRot = asinf(clamp(d′z, −1, 1))·kRadToDeg   // B7

renderSample(x, t):
  y = dist.processSample(x, t)
  bus[acn] = y · (prevC[acn] + t·(curC[acn] − prevC[acn]))           // 16, as 2.1 (local, pre-rotation)
  ShRotator::applyLerped(prevR, curR, t, bus, tap)                    // B5: tap[0] = bus[0]; blocks 1–3 fused lerp
  shPeak[acn] = max(shPeak[acn], |tap[acn]|)                          // POST-rotation (D5)
  StereoDecoder::decode(tap, …) → delay → mixer                       // unchanged
```

At `curR == prevR == I` every lerped element is exactly `1` or `0`, so `tap == bus` to the bit except a possible `−0 → +0` (R3) — the mechanism behind B12.

Reference twin (double): same three `Wrap` smoothers with the same `k` and snap; `evaluateC()` = `refSh16(dir(R3·d(az.cur, el.cur))) × gains[band]`; `snapCoefficientsPerGrid` unchanged (it is the yaw zipper by construction).

---

## Tasks

### T1 — `Source/dsp/ShRotator.h`
**Files:** create. **Depends on:** none (includes `ShEncoder.h` for `kNumSh`, `kDegToRad`, `shBand`).
- Header comment: provenance (own transcription of Ivanic & Ruedenberg 1996 + 1998 erratum; IEM SceneRotator / Green 2003 read for index conventions only — D9), the ACN permutation `p = (1, 2, 0)`, the active convention (no transpose), the 83-element layout, "W implicit 1", "identity exact in float (R3)", cost (≈ 700 flops per control step, 332 flops per sample ≈ 16 MFLOPS at 48 kHz — the corrected D1 number).
- `inline constexpr float kRadToDeg = 57.29577951308232f;` (the one new literal; `kDegToRad` comes from `ShEncoder.h`).
- `inline constexpr int kNumRotElems = 83;` `constexpr int blockOffset(int l) noexcept`; `constexpr int rotIndex(int l, int m, int n) noexcept`; `static_assert(blockOffset(3) + 49 == kNumRotElems && rotIndex(3, 3, 3) == 82 && rotIndex(2, -2, -2) == 9 && rotIndex(1, -1, -1) == 0, …)`.
- `class ShRotator`:
  - `void prepare() noexcept` — fills `float u[kNumRotElems], v[…], w[…]` for `l = 2, 3` per R1 (`δ = (m == 0)`, `denom`, the three formulas); entries for `l = 1` unused (0). B3.
  - `static void buildCartesian(float yawDeg, float pitchDeg, float rollDeg, float R3[3][3]) noexcept` — the three matrices as written above, multiplied in the order `Rz · (Ry′ · Rx)`; `sinf / cosf` of `deg · kDegToRad`. Document the sign of each (`+ψ`: front → left; `+θ`: front rises; `+φ`: left rises).
  - `void build(float yawDeg, float pitchDeg, float rollDeg, float R[kNumRotElems], float R3[3][3]) const noexcept` — `buildCartesian`, then `R₁` by the permutation, then `l = 2`, then `l = 3`, each from `R₁` and `R_{l−1}` read back through `rotIndex` on `R` itself. Private helper `float P(int i, int a, int b, int l, const float* R) const noexcept` with the three cases (`|b| < l`, `b == l`, `b == −l`), reading `R[rotIndex(1, i, 0 / ±1)]` and `R[rotIndex(l − 1, a, ·)]`. Guards `if (u != 0)` / `if (w != 0)` before evaluating `U` / `W` (they are not optimisations — `P` would read `R_{l−1}` at `a = ±l`, out of the block).
  - `static void apply(const float R[kNumRotElems], const float in[kNumSh], float out[kNumSh]) noexcept` — `out[0] = in[0]`; blocks 1–3 row-major.
  - `static void applyLerped(const float prev[kNumRotElems], const float cur[kNumRotElems], float t, const float in[kNumSh], float out[kNumSh]) noexcept` — B5. `in` and `out` must be distinct arrays (document; the engine passes `bus` → `tap`).
- Headless: `<cmath>`, `<algorithm>` + `ShEncoder.h`. No JUCE. Every free function `inline` or `constexpr` (`-Wmissing-prototypes`); all index arithmetic in `int` with explicit `(size_t)` only where an array is subscripted by a computed value — watch `-Wsign-conversion`.

### T2 — `Source/dsp/SpatialEngine.h` (rotation inserted)
**Files:** modify. **Depends on:** T1.
- `#include "ShRotator.h"`. Members: `WrapSmoother yaw, pitch, roll; ShRotator rot; float prevR[kNumRotElems] {}, curR[kNumRotElems] {}; float curR3[3][3] {}; float bus[kNumSh] {};` (`tap[16]` stays — now post-rotation).
- `prepare`: `rot.prepare()` before `snapToParams()`.
- `snapToParams` / `controlStep` / `fillFrameState` / `renderSample` exactly per the shared spec above. Keep the 2.1 op order for everything that exists (the three angle steps go **after** `mixer.controlStep` and before the `prevC ← curC` copy; the `prevR ← curR` copy and the rebuild go after `evaluateCoefficients()`, before `fillFrameState()`).
- `busTap()` comment: now the **post-rotation** bus of the last sample (D5); `frame()` comment: `shGain` is `R_SH·c_eff`.
- Header comment: replace the "identity rotation slot" line with the rotator, add a short block on D1 (rotate the bus, not the vector — the same matrix rotates any number of sources; 2.3 consumes the rotated bus), D2 (83-element lerp, W implicit, identity exact → 2.1 digest is the regression verdict), B6 (frame fast path and why), and the R2 note (a 180° step through the one-pole produces grid-boundary kinks that read 12–44× against a *static* control on bands 1–3 but 1.000 against the same-motion reference — same as the 2.1 azimuth path; v1.1 candidates: longer τ for jumps only, quadratic element interpolation).
- No allocation; no trig per sample; trig per control step = 3 `sinf` + 3 `cosf` (`R₃`) + 2 `cosf` + 2 `sinf` (`d`) + 1 `atan2f` + 1 `asinf` (frame, skipped on the fast path).

### T3 — `MeterFrame.h` comment, `CMakeLists.txt`, processor header comment
**Files:** modify `Source/dsp/MeterFrame.h` (comment only), `CMakeLists.txt` (`target_sources` + `Source/dsp/ShRotator.h`), `Source/PluginProcessor.cpp` (header comment: the chain now has the rotator after the encoder; the tap is post-rotation). **Depends on:** none. No code change in the processor — `fillParams` already copies the three angles; no new atomics; `setLatencySamples` untouched.
- `MeterFrame.h`: `shGain` = `R_SH·c_eff` **from Phase 2.2**; `shPeak` post-rotation; `azRot / elRot` = `dir(R₃·d(azEff, elEff))`, equal to `azEff / elEff` exactly when all three angles are 0 (B6), **undefined within ≈ 0.01° of the pole** (`|elRot| → 90°`: `atan2` of rounding noise — the Stage-3 compass holds its last value there); gimbal lock at `θ = ±90°` is a display matter (F5).

### T4 — `tests/render-harness/ReferenceModel.h` (B11)
**Files:** modify. **Depends on:** none in `Source/` (it must not read `ShRotator.h`).
- `RefParams` gains `float yawDeg = 0, pitchDeg = 0, rollDeg = 0;`.
- `static void buildR3(double yawDeg, double pitchDeg, double rollDeg, double R[3][3])` — written from the ARCHITECTURE matrices (the same text as T1's comment, in double, composed `Rz·Ry′·Rx`).
- `static void direction(double azDeg, double elDeg, double d[3])`; `static void dir(const double d[3], double& azDeg, double& elDeg)` — `atan2(y, x)`, `asin(clamp(z, −1, 1))`, degrees, az wrapped to [−180, 180).
- `static void rotatedDirection(double az, double el, double yaw, double pitch, double roll, double& azR, double& elR)` — the composition above; the meter-frame probe's oracle for `azRot / elRot`.
- `Wrap yaw, pitch, roll;` snapped in `prepare`, stepped in `controlStep` (same `kOnePole`, same place in the order as the engine); `evaluateC()` = `refSh16(azR, elR, y)` with `(azR, elR) = rotatedDirection(az.cur, el.cur, yaw.cur, pitch.cur, roll.cur)`, then the band gains. `snapCoefficientsPerGrid` / `snapOrderGains` untouched.
- Header comment: "Phase 2.2: rotation by **re-encoding at the rotated direction** — this file never contains `R_SH` (D6); for one source `R_SH·c == encode(R₃·d)` is an identity, so this is the oracle by construction".
- Sanity the executor runs once mentally: at `yaw = pitch = roll = 0`, `buildR3 = I` exactly in double and `dir(direction(az, el))` returns `(az, el)` to 9e-13° — the 2.1 P1 null (4.6e-8) is unaffected (B13).

### T5 — Harness infrastructure (`tests/render-harness/main.cpp`, sections before `main`)
**Files:** modify. **Depends on:** T2, T4.
- `#include "dsp/ShRotator.h"`.
- `refParamsFrom` (both overloads): copy `yaw / pitch / roll` (`rawParam(p, ParamIDs::yaw)` etc. — the IDs exist).
- `EngineRender` / `RefRender`: add `std::vector<float|double> tap16;` (stride 16, all 16 channels per sample) filled next to `tap4 / tap9` when `recordTaps` is set (keep `tap4 / tap9` — the 2.1 order-fade probe reads them). `worstCurvature(tap16, ch, 16, from, to)` then works unchanged.
- Helpers: `struct Tuple { double az, el, yaw, pitch, roll; }`; a deterministic tuple generator (`juce::Random r (0x2D5A2D5A)` or an LCG — fixed seed, printed) producing az ∈ [−180, 180), el ∈ [−90, 90], ψ / θ / φ ∈ [−180, 180); `gatedRefSh16(az, el, order, double out[16])` (zero the bands above `order`); `toFloat16` / `fromFloat16`; `blockOf(l)` views over an 83-array for the orthogonality / closed-form checks; `maxAbsDiff16`.
- Header comment of `main.cpp`: title → "Stage 2 / Phase 2.1 + 2.2 DSP gate"; append the 2.2 probe list (T6) and the sentence "digest-2.1 is a verdict from 2.2 (D3)".

### T6 — New probes (`main()`), all after the 2.1 set and before the digest lines
**Files:** `tests/render-harness/main.cpp`. **Depends on:** T5. Fresh instance per render; verdicts are ratios, exact anchors or 1e-4 / 1e-5 ceilings with the expected literal printed; every negative control through `checkFailsAsExpected`.

| Probe | Gate (PASS condition) |
|---|---|
| **DSP-02** | 200 tuples × orders 1 / 2 / 3: left = `ShRotator` (`prepare`, `build` in float) `apply`'d to `(float) gatedRefSh16(az, el, order)`; right = `gatedRefSh16(dir(buildR3_ref·d), order)` (double). `max |err| < 1e-4` per order (expect ≈ 2e-7 — print max and the worst tuple). W component exactly equal (`1.0f`) |
| **DSP-02 orthogonality** | same tuples: for each block `l`, `max |R_lᵀ·R_l − I| < 1e-5` (expect ≈ 1e-7). Also `R₃ᵀR₃` (engine `buildCartesian`) |
| **DSP-02 transpose trap** (negative control) | the same 200 tuples with `R_SHᵀ` (transpose each block in the harness) vs the oracle: `checkFailsAsExpected(max |err| < 1e-4)` (expect ≥ 1, R1: 1.24 at one tuple) |
| **DSP-02 yaw-only closed form** | ψ = 73°, θ = φ = 0: blocks 2 and 3 from `build` vs the closed-form per-order 2×2 (`[[cos mψ, sin mψ], [−sin mψ, cos mψ]]` on the `(−m, +m)` pair, `m = 0` unchanged) within 1e-6 (expect ≤ 2.2e-16 in double, ≈ 1e-7 float). Independent oracle for the 5×5 / 7×7, and the D10 fallback's formula written down |
| **identity exact** | `build(0, 0, 0)`: every one of the 83 elements is bit-equal to `0.0f` or `1.0f` and the diagonal is all ones (R3 made mechanical) |
| **digest-2.1 verdict** (B12) | `fnv1a64(nullRender) == 0x9189d911a1fe98d5ULL` as a `check`; the print line stays |
| **cardinal signs ×3** (D7, through the processor frame) | each a fresh `makeProc` with the tuple set in the setup lambda (so `prepare` snaps — the first frame is at steady state), order 3, one 64-sample block: (a) ψ = +90°, az = 0 → `|azRot − 90| < 1e-3` and `|elRot| < 1e-3`; (b) θ = +90°, az = 0 → `|elRot − 90| < 1e-3` (`azRot` printed only — pole, R4); (c) φ = +90°, az = +90° → `|elRot − 90| < 1e-3` (`azRot` printed). A transposed `Rz` reads −90 at (a) |
| **yaw = −azimuth → front** (FUNC-01 third box) | processor az = +90°, ψ = −90°, order 3, Stereo, 100 Hz tone, 48 000 samples: frame `max |shGain − refSh16(0, 0)| < 1e-5` (order 3, all 16); `azEffDeg == 90.0f` bitEqual and `elEffDeg == 0` (puck unchanged); output `max |L − R| < 1e-5` over the render (front symmetry); `max |L| > 0.1` (live) |
| **P1 null rotated** | processor az 37 / el −12 / d 3.3 / order 3 / Stereo / **ψ 25 / θ −40 / φ 70** vs `ReferenceModel` on the same noise, fresh instances: `max |err| < 1e-5` on both channels over 8192 samples, `peak |L| > 0.01`. This render is **digest-2.2**'s input |
| **meter frame (2.2)** | same tuple, processor, one 1536-sample render: `max |shGain − gatedRefSh16(rotatedDirection(…), 3)| < 1e-5`; `|azRot − azR_ref| < 1e-3`, `|elRot − elR_ref| < 1e-3`; `azEffDeg == 37.0f` and `elEffDeg == −12.0f` bitEqual. Then with a yaw hook (0 → 90° at 0.25 s via `renderHooked`) across 10 frame reads: every `azEffDeg == 37.0f` bitEqual, and `azRotDeg` at the last read differs from the first by ≥ 60° (liveness) |
| **round trip (D2 / F2)** | processor at the 2.1 null tuple (rotation 0), `renderHooked`: ψ → 90° at `n = 0`, ψ → 0° at `n = 24 000`, total 72 000; frame after the last block: `azRotDeg == azEffDeg` bitEqual (the fast path is reachable again); output over the last 8192 samples `memcmp`-equal to a fresh static render's same window (static after a move == fresh render, through the recursion); liveness: the middle window differs from the static render |
| **yaw sweep click** (B8 / B9) | engine-direct, `renderEngine(…, {64}, tone100, hook, recordTaps = true)`, az 37 / el −12 / d 1 / order 3 / Stereo, hook `p.yawDeg = wrapTo180(0.48f·(float)(pos / 64))`, `skip = 64 + 960`, `total = skip + 48 000`. Controls: 24 parks `yawDeg = 15·k` (engine-direct, recordTaps), per channel `control[ch] = max_k worstCurvature(park_k.tap16, ch, 16, skip, total)`. Verdict: `worstCurvature(ev.tap16, ch, 16, skip, total) / control[ch] ≤ 2.0` for **all 16** channels (print the worst channel and ratio — expect ≈ 1.13 at ACN 9). Negative control: `ReferenceModel` with `snapCoefficientsPerGrid` under the same hook (`renderRef`, block 64, recordTaps) → on the **m ≠ 0** channels {1, 3, 4, 5, 7, 8, 9, 10, 11, 13, 14, 15} `checkFailsAsExpected(max ratio ≤ 2.0)` (expect ≥ 49×) |
| **zonal invariance** (F3) | from the same sweep render: channels 0, 2, 6, 12 of `ev.tap16` bit-identical (per-sample) to the `yaw = 0` park's — "rotation about z preserves the zonal harmonics" |
| **yaw ≡ azimuth cross-gate + liveness** (R6) | engine-direct **front** source (az 0 / el 0 / d 1 / order 3 / Stereo): render A = yaw sweep hook as above; render B = the identical schedule on `azimuthDeg` with yaw 0. `max |A.tap16 − B.tap16| < 1e-5` over all 16 channels after `skip` (expect ≈ 1.5e-7) and `max |A.lr − B.lr| < 1e-5`. Liveness on A: `|10·log10(E_L second half-circle / first)| ≥ 6 dB` (expect 8.2) |
| **yaw step click** (B8) | engine-direct, same az 37 / el −12 / order 3 tuple, hook `p.yawDeg = pos >= 24 000 ? 180 : 0`, `total = skip + 48 000`, recordTaps. Event = engine `tap16`; control = `ReferenceModel` (lerped) under the same hook; `ratio ≤ 2.0` on all 16 channels (expect 1.000 — print the worst). Negative control: snapped reference vs the lerped reference → `checkFailsAsExpected` on channels 1–15 (expect 64–126×). **Informational** (no verdict): engine vs the max of the yaw 0 / yaw 180 parks per band — printed with the one-line R2 explanation ("grid-boundary kinks of the one-pole + lerp; same as the 2.1 azimuth path; hands-on at verify") |
| **P2c / P3c** (B10) | `rotatedMovingSetup` = 2.1 `movingSetup` + ψ 25 / θ −40 / φ 70: ragged `{1, 7, 64, 333, 4096, 13, 512, 2}` ≡ flat 512; prep-64 + 4096 feed ≡ prep-4096 ≡ flat — all `buffersBitEqual`. Liveness: the flat render ≠ 2.1's `flatMoving` |
| **P2d** (B10) | processor at the 2.1 null tuple, `renderHooked` block 64 vs block 4096, total 32 768, hook: `if (n % 4096 == 0) { yaw = wrapTo180(30·(n/4096)); pitch = wrapTo180(−20·(n/4096)); roll = wrapTo180(45·(n/4096)); }` → the two renders `buffersBitEqual`; liveness: ≠ the hook-less render |
| **PERF-01** (extended) | existing probe; the `switch` gains `case 15: yaw 90`, `case 25: pitch −45`, `case 35: roll 30`, `case 70: yaw −180`, `case 90: pitch 0; roll 0`; verdict string mentions "yaw / pitch / roll moving"; `allocations == 0`; liveness unchanged |
| **extremes (2.2)** | second corner block: 8 corners over the signs of (ψ, θ, φ) ∈ ±180°, az alternating ±180, el alternating ±90, d 0.2 / 20 alternating, order 3, orbit On 2 Hz, decoder alternating, outputLevel 0 → at fs 48 000 and 96 000: every sample finite and not all-zero. Plus one processor at θ = +90° exactly (gimbal) with a yaw sweep hook — finite |
| **digest-2.2** | FNV-1a 64 of the rotated P1-null render — printed, informational (recorded in SUMMARY; 2.3 may adopt it as the Stereo-mode regression anchor) |

Probe order in `main()`: the entire 2.1 set unchanged → digest-2.1 verdict → DSP-02 (+ orthogonality, transpose trap, closed form, identity exact) → cardinal ×3 → yaw = −azimuth → P1 null rotated → meter frame (2.2) → round trip → yaw sweep (+ zonal) → cross-gate + liveness → yaw step → P2c / P3c / P2d → extremes (2.2) → PERF-01 (extended, stays where it is) → digest-2.1 print, digest-2.2 print.

### T7 — Build, harness, install, validate
**Depends on:** T1–T6. Long commands in the background (600 s watchdog).
1. `ninja -C build O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics_Standalone O-simpleAmbisonics-render-test` — **0 warnings** (`-Wsign-conversion` on `rotIndex` subscripts, `-Wshadow` on `m / n / l` vs any outer names, `-Wconversion` on `(float)(pos / 64)`, unused `tap4 / tap9` fields are fine — they are still read by the order-fade probe).
2. Run the harness: **ALL PASS**, 118 + the new checks; record every literal (DSP-02 max err + worst tuple, orthogonality, transpose-trap value, cardinal literals, yaw = −azimuth `|L − R|`, rotated null err, meter-frame deltas, sweep worst ratio + channel, negative-control ratios, cross-gate err, liveness dB, step ratio + the informational vs-static numbers per band, alloc count, both digests). **digest-2.1 must read `9189d911a1fe98d5`.** Run the binary twice: both digests reproduce.
3. If digest-2.1 fails: `memcmp` the 2.2 `nullRender` against a 2.1-built render (or inspect the first differing sample); if it is a `−0 → +0` on a bus channel (R3), record it as a deviation and demote the digest to a `< 1e-6` null per B12; anything else is a defect — fix it (the slot was not inserted transparently).
4. If DSP-02 fails after two attempts at the transcription: D10 — ship yaw-only (the closed-form blocks from the probe), record pitch / roll as v1.1, and say so in the SUMMARY and STATUS. Not expected.
5. `./scripts/build-and-install.sh O-simpleAmbisonics` (AU cache clear + dual-variant sweep).
6. `auval -v aufx OSAm OuDv` — never `auval -a`.
7. pluginval strictness 10, `--timeout-ms 600000`, **twice each** on the installed `O-simpleAmbisonics-dev.vst3` and `.component`.

### T8 — SUMMARY-2.2, STATUS, commit
**Files:** `.planning/stages/2-dsp/SUMMARY-2.2.md` (create), `.planning/STATUS.md` (modify). **Depends on:** T7. `PLUGINS.md` untouched this phase.
- SUMMARY-2.2: files created / modified; the rotation control step as implemented; probe table with every literal; **digest-2.1 (unchanged — the verdict) and digest-2.2**; pluginval / auval results; allocation-coverage statement re-affirmed (nothing new on the audio thread allocates; the coefficient tables are members); **deferred list** carried forward by name: A14 binaural placeholder → 2.3, `kStereoLevelMatch` → 2.3, TripleBuffer → module candidate, −180 / 180 display → Stage 3, **step-response audibility** (hands-on at verify; v1.1 candidates: longer τ for jumps only, quadratic element interpolation — R2), D1 cost corrected to ≈ 16 MFLOPS, signed-zero caveat (R3) only if observed, gimbal-lock tooltip (F5) → Stage 3; any deviation from this plan with its reason.
- STATUS.md: Phase 2.2 table `execute ✓`, frontmatter `current_phase: execute`, `next_phase: verify`, `next_action: plugin-verify`, `status: stage_2_phase_2_2_execute_complete`, Current Position, Completed So Far entry, Files Created additions.
- Commit **temp-index, path-scoped** (`index_git_shared_checkout`): `git branch --show-current` + `git status --short` immediately before; `GIT_INDEX_FILE=$(mktemp -d)/index; git read-tree HEAD; git update-index --add -- <each file explicitly>; git write-tree; git commit-tree; git update-ref HEAD`; re-check `git log -1` after. Message: `phase: O-simpleAmbisonics 2-dsp/2.2 execute complete (…)`.

## Dependency graph

```
T1 ShRotator ──→ T2 SpatialEngine ──┐
T3 comments + CMake ────────────────┼──→ T5 harness infra ──→ T6 probes ──→ T7 build / validate ──→ T8
T4 ReferenceModel (independent) ────┘
```
T1, T3, T4 are parallel-authorable. T4 must be written from the ARCHITECTURE matrices and this plan's spec, **not** by reading T1 (two builders is the point of D7 / R4).

## Files

| Action | Path |
|---|---|
| create | `Source/dsp/ShRotator.h` |
| modify | `Source/dsp/SpatialEngine.h` |
| modify | `Source/dsp/MeterFrame.h` (comment only) |
| modify | `Source/PluginProcessor.cpp` (header comment only) |
| modify | `CMakeLists.txt` (`target_sources` + one line) |
| modify | `tests/render-harness/ReferenceModel.h` |
| modify | `tests/render-harness/main.cpp` |
| create | `.planning/stages/2-dsp/SUMMARY-2.2.md` |
| modify | `.planning/STATUS.md` |

Not touched: `Source/PluginProcessor.h`, `Source/dsp/{Smoothers,ShEncoder,DistanceModel,OrbitLfo,StereoDecoder,DecoderMixer,TripleBuffer,FixedDelay}.h`, `tests/render-harness/CMakeLists.txt`, `tests/pedalboard/*` (verify phase), `PLUGINS.md`, `setLatencySamples`, the 12 parameters, any `BinauralConvolver` / `BinauralFilterData` (2.3), any editor file (Stage 3).

## Gotchas checklist (executor ticks each)

- [ ] `R₁[i][j] = R₃[p(i)][p(j)]`, `p = (1, 2, 0)` — and **no transpose** anywhere; the transpose-trap probe must FAIL-as-expected.
- [ ] The `u ≠ 0` / `w ≠ 0` guards are in place; `w` never evaluated at `m = 0`; `denom` switches at `|n| = l`.
- [ ] `rotIndex` `static_assert`s compile; `blockOffset = {0, 9, 34}`; the recursion reads `R_{l−1}` through `rotIndex(l − 1, …)` on the same array.
- [ ] W implicit: `out[0] = in[0]`; never stored, lerped or multiplied.
- [ ] Fused lerp `prev + t·(cur − prev)` (A3 form) with the 2.1 `t`; `bus` and `tap` are distinct arrays.
- [ ] Tap + `shPeak` scan are **after** the rotation; the stereo decoder reads `tap`.
- [ ] Three `WrapSmoother`s snapped in `snapToParams`, stepped in `controlStep`; `prevR ← curR` before the rebuild, in both `snapToParams` (after the first build) and `controlStep`.
- [ ] Frame: `shGain = apply(curR, curC)`; `azEff / elEff` pre-rotation; fast path when all three `cur == 0`; `clamp` before `asinf`; `wrapTo180` on `azRot`.
- [ ] `rot.prepare()` called in `SpatialEngine::prepare` before the first `build`; no `sqrt` / allocation in `process`.
- [ ] `ReferenceModel` never includes `ShRotator.h`; rotates by re-encoding at `dir(R₃·d)`; `RefParams` + both `refParamsFrom` overloads carry the three angles.
- [ ] Sweep hook starts at the prepared value (`0.48·(pos/64)` from 0°); 16-channel click gates at az 37 / el −12; front source only for liveness + cross-gate.
- [ ] Step gate verdict vs the same-motion reference; vs-static numbers printed as information with the explanation, never asserted.
- [ ] Negative control for the sweep excludes ACN 0 / 2 / 6 / 12 (they read exactly 1.0 — F3); the zonal probe asserts them bit-identical instead.
- [ ] P2b / P3b / `movingSetup` and every 2.1 verdict string untouched; new slicing gates are P2c / P3c / P2d; host-side changes in P2d only at `n % 4096 == 0`.
- [ ] `digest-2.1 == 9189d911a1fe98d5` is a `check`; both digests reproduce on a second run.
- [ ] 0 warnings; `build-and-install.sh` then targeted `auval -v aufx OSAm OuDv`; pluginval s10 ×2 VST3 + AU.
- [ ] Temp-index, path-scoped commit; re-check branch + status immediately before; `PLUGINS.md` untouched.

## Success criteria (verify phase checks these)

- [ ] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics-render-test` builds with 0 warnings.
- [ ] **digest-2.1 == `9189d911a1fe98d5`** — the entire 2.1 probe set PASS and bit-identical at zero rotation (D3); the identity-exact probe PASS.
- [ ] DSP-02 < 1e-4 at orders 1 / 2 / 3 over 200 tuples (literal ≈ 2e-7 printed); orthogonality < 1e-5; transpose trap FAILs as expected; yaw-only closed form agrees; three cardinal assertions PASS through the frame; `yaw = −azimuth` → `shGain == refSh16(0, 0)` within 1e-5, `azEff` unchanged, `|L − R| < 1e-5`.
- [ ] P1 null < 1e-5 at (ψ 25, θ −40, φ 70) on a fresh instance; meter frame `shGain / azRot / elRot` match the reference, `azEff / elEff` bitEqual to the parameters under yaw; round trip returns to the bit-exact static render and the identity fast path.
- [ ] Yaw-sweep ratio ≤ 2.0 on all 16 bus-tap channels (expect 1.13) with the snapped negative control ≥ 2.0 on m ≠ 0 channels; zonal channels bit-identical; yaw ≡ azimuth within 1e-5 on a front source; liveness ≥ 6 dB; yaw-step ratio ≤ 2.0 vs the same-motion reference (expect 1.000), negative control FAILs as expected.
- [ ] P2c / P3c / P2d bit-equal across slicings with rotation; PERF-01 0 allocations with the three angles moving (liveness 1); extremes finite and live at 48 k / 96 k on ±180° of all three angles and at θ = 90° under a yaw sweep.
- [ ] P4 latency still 64; pluginval s10 ×2 VST3 + AU and `auval -v aufx OSAm OuDv` PASS on the installed bundles.
- [ ] Verify-phase items (not execute): `tests/pedalboard/verify_2_2.py` on the installed VST3 — yaw ±90° in Stereo mode moves the image by the azimuth pan law (≈ 22.88 dB), `yaw = −azimuth` restores `L == R`; DAW hands-on — Yaw spins the image while the Generic editor's Azimuth value stays put, Pitch + raises the front, Roll + raises the left, a Yaw flick 0 → 180 at order 3 on a low tone is listened to for a zip (R2).
- [ ] `SUMMARY-2.2.md` records digest-2.1 (unchanged), digest-2.2, every gate literal, and the deferred list (A14, `kStereoLevelMatch`, TripleBuffer module, −180 / 180 display, step-response audibility + v1.1 candidates, D1 cost ≈ 16 MFLOPS, gimbal tooltip).
