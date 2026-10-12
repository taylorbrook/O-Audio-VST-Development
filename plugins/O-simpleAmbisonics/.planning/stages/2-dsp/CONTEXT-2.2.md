# Stage 2 — DSP · Phase 2.2 (SH rotation + rotated meter tap) — Context

**Plugin:** O-simpleAmbisonics
**Stage:** 2 of 4 — DSP
**Phase:** 2.2 of 3 — scene rotation in the ambisonic domain (yaw / pitch / roll), rotated meter tap
**GSD phase:** discuss
**Date:** 2026-10-12
**Branch:** `main` @ `5c1c7db5` (Phase 2.1 verified)
**Participants:** Taylor Brook, Claude
**Inputs:** `research/ARCHITECTURE.md` (immutable contract — §SH Rotator, §SH rotation (Ivanic–Ruedenberg), Decisions 5 and 9), `ROADMAP.md` §Phase 2.2, `REQUIREMENTS.md` (FUNC-01, FUNC-04, DSP-02), `stages/2-dsp/{CONTEXT,RESEARCH,PLAN,SUMMARY,VERIFICATION}-2.1.md`, `Source/dsp/{SpatialEngine,Smoothers,ShEncoder,MeterFrame}.h`, `tests/render-harness/{main.cpp,ReferenceModel.h}`

---

## Entry check — what Phase 2.1 hands over

Re-read at this boundary, not taken on STATUS.md's word:

| Carry-forward | State |
|---|---|
| Harness on the installed bundles | 118 / 118 PASS, digest `9189d911a1fe98d5` reproduced ×4 and at verify. **Every 2.1 probe leaves yaw / pitch / roll at 0°** — see D3 for what that buys 2.2 |
| Engine seam | `SpatialEngine::process (mono, outL, outR, n)` on the persistent 64-sample control grid (`gridPos`), `controlStep()` → `renderSample (x, t, …)`, `t = (j+1)/64`, every blend `b + t·(a − b)`. `Params` **already carries `yawDeg / pitchDeg / rollDeg`** (read by `fillParams`, ignored by the engine) |
| Rotation slot | `renderSample`: `tap[acn] = y·c` with the `shPeak` scan, then the comment `── identity rotation slot: ShRotator (Phase 2.2) applies R to tap[] here ──`, then `StereoDecoder::decode (tap, …)`. **The tap and the peak scan are currently PRE-rotation** — 2.2 must move them after the rotation (ARCHITECTURE §Processing Order 4–5: "rotate … → write the bus; meter tap on the rotated bus") |
| Meter frame | `azRotDeg / elRotDeg = azEff / elEff` and `shGain = c_eff` (identity) — the Stage-3 reader contract in `MeterFrame.h` already names the 2.2 semantics (`R·c_eff`, rotated direction) |
| Smoothers | `WrapSmoother` (one-pole on the wrapped delta, A2 snap at 1e-6, `[−180, 180)`), `kOnePole = 0.0645` at 48 kHz from the grid — reusable as-is for the three rotation angles |
| Rotation parameters | `yaw`, `pitch`, `roll` all `degrees (…, 180.0f)` — **±180°, continuous (interval 0), exact 0° default** |
| Reference model | `tests/render-harness/ReferenceModel.h` — double transcription, recursion-free, generic SN3D table `refSh16`, switches `snapCoefficientsPerGrid` / `snapOrderGains` for negative controls |
| Harness infra | `renderHooked` (processor, hook before each block), engine-direct ragged / fixed-block renders under `ScopedNoDenormals`, `checkFailsAsExpected`, second-difference click helpers, rtcheck allocation counter |
| Deferred into 2.2 by name (SUMMARY-2.1) | `ShRotator` slot; `azRot / elRot == azEff / elEff` until now; ReferenceModel "extends with a rotation matrix at 2.2" — **amended by D6 below** (the reference gets a rotated *direction*, never a recursion) |
| Still deferred past 2.2 | A14 binaural placeholder, `kStereoLevelMatch`, TripleBuffer module extraction, −180 / 180 display, human DAW hands-on (COMPAT-02 / QUAL-02) |

No contract drift: ARCHITECTURE.md, ROADMAP.md and the 12-parameter set are unchanged since Stage 0. The parameter count, the latency (64) and the processor's `setLatencySamples` call site are not touched by this phase.

---

## Discussion summary

The rotation is closed at the algorithm level: ARCHITECTURE fixes the Cartesian matrices and their signs, the Euler composition `R = Rz(ψ)·Ry′(θ)·Rx(φ)`, the ACN permutation of the order-1 block, the Ivanic–Ruedenberg recursion for the 5×5 and 7×7, the per-sample matrix lerp, and the DSP-02 gate. Discuss did **not** re-open any of that. No decision in this phase needed Taylor's call; the one choice that *could* have gone either way (D1 — rotate the bus or fold `R` into the coefficient vector) is settled by the contract's own rationale and is recorded with its alternative so it can be overturned at research if wanted.

What discuss settled is **how the rotation lands in the 2.1 engine without disturbing anything 2.1 proved**: where the tap moves, how the lerp is written so that static settings stay bit-exact, what "zero rotation" must mean for the 2.1 digest, how the reference model stays an *oracle* rather than a second copy of the recursion, and which gates make the known-errata risk of the recursion mechanical rather than a matter of reading the paper carefully.

---

## Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D1** | Where the rotation acts | **Rotate the 16-channel bus per sample** with the block-diagonal `R_SH` (9 + 25 + 49 = 83 multiplies per sample), exactly as ARCHITECTURE Decision 5 says. **Not** folded into the encoder vector (`c_rot = R_SH·c_eff` lerped per sample, 16 multiplies) even though for one mono source the two are the same signal to first order | The lesson *is* that the same matrix would rotate any number of sources — folding it into the encoder would make the rotation a second panner in disguise. The 2.3 convolver consumes the rotated bus anyway. Cost is ≈ 4 MFLOPS at 48 kHz, invisible next to the 82 MFLOPS convolver. `shGain = R_SH·c_eff` for the meter frame is still computed once per control step from the block matrices (Decision 9) |
| **D2** | Lerp form on the control grid | Per control step: smooth the three angles → build `R₃` → permute to `R₁` (ACN Y, Z, X) → recurse `R₂`, `R₃` (the SH blocks) → store **83 elements contiguously** (`prevR`, `curR`). Per sample: `R_t[k] = prevR[k] + t·(curR[k] − prevR[k])` for all 83 (A3 form, A5 `t = (j+1)/64`), one apply. **W (ACN 0) is implicit 1** — never stored, lerped or multiplied, so W passes bit-exactly. **All three blocks are always applied**, no order-dependent skipping | Same structure as the 2.1 coefficient lerp, so P2b / P3b slicing invariance carries over by construction. With `curR == prevR` (static) the lerp returns `curR` exactly, so a static render after a move is bit-identical to a fresh render (A2 snap rule holds through the rotation). Skipping the 7×7 at order < 3 would save ~50 multiplies and buy a branch plus a `−0.0` vs `+0.0` difference in the bus — not worth it |
| **D3** | Zero rotation is **bit-transparent** | `R_SH(0, 0, 0)` must be the **exact** identity at every order (exact `1` / `0` entries: `cos 0 = 1`, `sin 0 = 0` exactly in float; in the recursion every identity element reduces to `sqrt(1)·1` or to products with exact zeros — research R3 confirms element-by-element). Consequence and **the 2.2 regression gate**: the entire 2.1 probe set runs unchanged and **its digest `9189d911a1fe98d5` must reproduce bit-for-bit** (kept as `digest-2.1` over the 2.1 render set; 2.2's new renders get their own `digest-2.2`) | Every 2.1 probe has rotation at 0°, so bit-transparency turns 118 proven checks into a free regression suite for "the slot was inserted without touching anything else" — including the P1 anchor `0.75·s[n−64]`, the bit-exact order-fade arrivals and the DSP-05 identity. If R3 finds the recursion cannot be exact at identity, the fallback is a `< 1e-6` null against the 2.1 renders, recorded as a deviation — but the order-1 block (the one the stereo decoder reads) is exact regardless |
| **D4** | Smoothing of yaw / pitch / roll | **Three independent `WrapSmoother`s** (τ = 20 ms, `kOnePole`, A2 snap), stepped at the control step like `az`; then compose. No quaternion / slerp, no coupling between axes | ARCHITECTURE §Angle Smoothing names all three as wrap-aware one-poles; all three parameters are ±180° continuous, so each takes the short way round its own axis — which is what the UI knobs show. Euler-angle smoothing is the contract; a slerp would be smoother for simultaneous large moves but would make the *displayed* yaw / pitch / roll and the audible path disagree |
| **D5** | Chain order in `renderSample` | `y = dist(x)` → `b[acn] = y·lerp(c)` (16, as now) → **`tap = R_t·b`** (83) → `shPeak` scan on `tap` → `StereoDecoder::decode (tap)` → delay → mixer. `busTap()` therefore becomes **post-rotation** | ARCHITECTURE §Processing Order 4–5 and §Visualization Tap: peaks are post-rotation, the tap follows rotation. The 2.1 order-fade click gate reads `busTap()` at zero rotation — unchanged by D3; and a rotation is orthogonal per block, so per-band norms (what that gate measures) are rotation-invariant anyway |
| **D6** | Reference model stays **recursion-free** | `ReferenceModel` gets **its own 3×3 Cartesian builder** (written from the ARCHITECTURE matrices) and rotates by **re-encoding at the rotated direction**: `c_ref = gated refSh16 (dir (R₃·d(az, el)))`. It never contains `R_SH`. The P1 null stays a *static-setting* null (now with a non-zero rotation tuple added). The DSP-02 gate's right-hand side is this reference path; its left-hand side is `ShRotator`'s `R_SH` applied to `refSh16 (az, el)` | For one source, `R_SH·c_eff == gated encode (R₃·d)` is an identity (gating is per band and commutes with the block-diagonal `R_SH`) — so the recursion-free form **is the oracle by construction**, independent of the thing under test (`pattern_test_fixture_mirrors_drift_silently`, 2.1 D8 / R4 discipline). Transcribing the recursion into the reference would make the null blind to a shared index error |
| **D7** | Sign convention pinned through the **engine**, not only the harness | Three cardinal assertions run through the full engine path (frame `shGain`, order 1): `ψ = +90°` maps front → `azRot = +90°` (left); `θ = +90°` maps front → `elRot = +90°`; `φ = +90°` maps left (`az = +90°`) → `elRot = +90°`. Plus the ROADMAP criterion **`yaw = −azimuth` returns the source to front**: `az = +90°, ψ = −90°` → frame `shGain == refSh16 (0, 0)` within 1e-5 at order 3, and Stereo `L == R` (front symmetry) | ARCHITECTURE notes the DSP-02 gate "also pins the UI-facing sign convention" only if the convention is asserted against something *outside* the builder under test; the harness builder (D6) is that something. A transposed `Rz` would pass a tuple gate that used the engine's builder on both sides |
| **D8** | Rotated direction for the frame | At each control step: `azRot = wrapTo180 (atan2 (y′, x′) / kDegToRad)`, `elRot = asin (clamp (z′, −1, 1)) / kDegToRad` with `(x′, y′, z′) = R₃·d(azEff, elEff)` — one `atan2` + one `asin` per 64 samples. `azEff / elEff` stay pre-rotation (UI-02: the puck never moves with yaw) | Trig at block rate only; the frame's rotated direction is exactly what the polar-plot centre and the compass need (ARCHITECTURE §Visualization Tap). At `|elRot| → 90°` the azimuth is degenerate (`atan2 (0, 0)` = 0) — documented in `MeterFrame.h`, harmless to the UI |
| **D9** | Recursion provenance | **Derive from the Ivanic–Ruedenberg 1996 formulas with the 1998 erratum** (public mathematics), transcribed into `ShRotator.h` with the `u / v / w` coefficient functions and the `P(i, a, b, l)` helper written out and commented. **IEM SceneRotator `calcRotationMatrix` and Green 2003 are consulted for index conventions and as a cross-check — no verbatim lift** | The suite's own-code policy, and IEM is GPL-3.0 (combinable with AGPL-3.0, but an independent transcription keeps provenance one-line). The 1996 paper's errata are the known risk — research R1 validates the formulas numerically *before* planning, so execute transcribes validated code, not a paper |
| **D10** | Fallback stays armed | If the recursion fails DSP-02 after two execute attempts, ship **yaw-only** (closed-form per-order `cos (mψ) / sin (mψ)` 2×2 blocks, exact and trivial) and move pitch / roll to v1.1 — FUNC-04's *must* is yaw; pitch / roll are *should* | ARCHITECTURE §SH rotation recursion names exactly this fallback. Not expected to be needed: the DSP-02 gate is mechanical and the research pre-validation (R1) front-loads the risk |

Execution notes, not decisions:

- **Processor:** nothing changes — `fillParams` already copies the three angles; no new atomics, no snapping (continuous ranges). `CMakeLists.txt` gains `Source/dsp/ShRotator.h` in the sources list. `setLatencySamples` untouched.
- **`MeterFrame.h` comment** updates to say `shGain` and `shPeak` are post-rotation *from this phase* and that `azRot / elRot` now differ from `azEff / elEff`.
- **Chord error of the matrix lerp** is bounded by the one-pole: the largest per-grid step is a 180° jump × `k = 0.0645` = 11.6° in the first grid, decaying — research R2 puts a dB number on the mid-grid norm dip per order (the band-3 harmonics turn at 3× the angle). A 360°/s yaw sweep is 0.48° per grid — negligible by inspection.
- **Negative control for the yaw click gate** already exists: the ReferenceModel with `snapCoefficientsPerGrid = true` driven by the same yaw sweep (its rotation is a re-encode, so snapping its vector per grid is the zipper). No test hook is added to the engine.

---

## Phase goal

Drop `ShRotator` into the 2.1 engine so the **scene** rotates after encoding: yaw spins the decoded image while the puck (`azEff / elEff`) stays put, `yaw = −azimuth` brings the source back to front, pitch raises the front and roll raises the left side with the ARCHITECTURE signs, the meter frame publishes the rotated coefficient vector and the rotated direction, and **nothing 2.1 proved changes** — the 2.1 digest reproduces bit-for-bit at zero rotation. Latency stays exactly 64.

## Scope (in)

New header in `Source/dsp/` (JUCE-free, double-testable):

1. **`ShRotator.h`** — `buildCartesian (ψ, θ, φ) → R₃` with the exact ARCHITECTURE matrices (`Rz · Ry′ · Rx`, radians via `kDegToRad`); `R₁[i][j] = R₃[p(i)][p(j)]`, `p = (1, 2, 0)`; Ivanic–Ruedenberg recursion for the 5×5 and 7×7 (D9); contiguous `float R[83]` storage with a documented index helper (block offsets 0 / 9 / 34); `static apply (const float R[83], const float in[16], float out[16])` (W passes through, D2); no state beyond the two matrices the engine owns.

Engine changes (`SpatialEngine.h`):

2. Three `WrapSmoother`s `yaw / pitch / roll` stepped in `controlStep` (D4); `prevR / curR` swap-and-rebuild per control step, snapped in `snapToParams`; `renderSample` reordered per D5 with the per-element lerp (D2); `fillFrameState` writes `shGain = R_SH·c_eff` (from `curR`, `curC`) and `azRot / elRot` per D8.

Harness (`tests/render-harness/`):

3. **`ReferenceModel.h`** — `RefParams` gains `yaw / pitch / roll`; own Cartesian builder + `dir()`; re-encode at the rotated direction (D6); the three wrap smoothers in double with the same `k` and snap.
4. **`main.cpp`** — all 2.1 probes unchanged (digest-2.1 must equal `9189d911a1fe98d5`); new probes: **DSP-02** (200 random `(az, el, ψ, θ, φ)` × orders 1 / 2 / 3, `R_SH·refSh16 (az, el)` vs `refSh16 (dir (R₃·d))`, max |err| < 1e-4, worst tuple printed), **orthogonality** (`R_blockᵀ·R_block = I` within 1e-5 per block on the same tuples — catches a norm error independently of the direction formula), **cardinal signs ×3 + yaw = −azimuth** through the frame (D7), **P1 null** repeated at a non-zero rotation tuple (< 1e-5, fresh instance), **yaw-sweep click gate** (360° over 1 s at order 3, 100 Hz tone, second-difference ratio ≤ 2.0 on **all 16 bus-tap channels** vs the matched static control; negative control = ReferenceModel `snapCoefficientsPerGrid`; liveness: Stereo-output half-circle level swing ≥ 6 dB), **yaw-step click gate** (0° → 180° step mid-tone at order 3 — the worst one-pole chord — same ratio), **P2b / P3b extended** (64- vs 4096-sample slicing bit-equal with yaw / pitch / roll automation and orbit On), **PERF-01** re-run with all three angles moving, **meter frame** (`shGain`, `azRot / elRot` vs the reference for a static tuple; `azEff` unchanged by yaw), **extremes** (±180° on all three angles × orders, 48 k / 96 k, finite), **digest-2.2**.
5. `tests/pedalboard/verify_2_2.py` at verify (independent measurer on the installed VST3): yaw ±90° in Stereo mode moves the image left / right by the same 22.88 dB as azimuth; `yaw = −azimuth` restores front `L == R`.

## Scope (out — later phases)

- Binaural convolver, real `kStereoLevelMatch`, DSP-03 / DSP-07 / PERF-02 / QUAL-01 full sweep, A14 placeholder removal → **Phase 2.3**; the Stage-2 roll-up `VERIFICATION.md` is written at the 2.3 verify.
- Polar plot (`g_N(γ)` in JS, centred on `azRot`), compass, puck → **Stage 3**. Presets, hands-on → **Stage 4**.
- `PLUGINS.md` row untouched until the Stage-2 roll-up.
- Head tracking, head-locked stereo, order-4 — out of v1.0 (ARCHITECTURE Decision 10).

## Requirements covered this phase (verify gates them)

| Requirement | Priority | Phase 2.2 delivers | Gate |
|---|---|---|---|
| FUNC-04 | must (yaw) / should (pitch, roll) | scene rotation in the SH domain after encoding; `azEff` unchanged | DSP-02 + cardinal signs + meter-frame `azEff` probe |
| DSP-02 | must | `R_SH·encode (d) == encode (R₃·d)` within 1e-4 at every order | DSP-02 (200 tuples × 3 orders) |
| FUNC-01 (third box) | must | `yaw = −azimuth` returns the source to front, audibly and in the frame | D7 probe + pedalboard V-check |
| DSP-06 (extends) | must | yaw / pitch / roll take the short way, no zipper on a 360° yaw sweep | yaw-sweep + yaw-step click gates |
| PERF-01 (re-gate) | must | zero allocations with the rotation moving | rtcheck counter |
| QUAL-01 (partial) | must | finite at ±180° on all three angles (full sweep in 2.3) | extremes render |

FUNC-02 / FUNC-06 / DSP-01 / DSP-04 / DSP-05 stay complete — the unchanged 2.1 probe set and digest-2.1 are the proof. pluginval s10 VST3 + AU and `auval -v aufx OSAm OuDv` re-run at verify.

## Constraints honored

- JUCE 8.0.15; `setLatencySamples` untouched; 12 parameters untouched.
- RT safety: no allocation (`float R[83]` ×2 as members); trig only at the control step (3 cos / 3 sin for `R₃`, 1 `atan2` + 1 `asin` for the frame — all per 64 samples); `ScopedNoDenormals` on engine-direct harness renders (F6).
- Control-grid invariants from 2.1 (A1–A7): `k` from 64 never from the slice; every smoother snapped in `prepare`; blends `b + t·(a − b)`; static after a move == fresh render.
- Harness: JUCE-free `Source/dsp/` (D3 of 2.1); click gates are ratios with a negative control and a liveness check (`index_dsp_gate_design`); fresh instance per render; analytic goldens only; digests informational except digest-2.1, which is a **verdict** this phase (D3).
- Provenance: own transcription of the recursion (D9); the 2.1 encoder lift from O-GrainScatter is unchanged.
- Trunk-based, path-scoped, **temp-index commit** (`index_git_shared_checkout`): `plugins/O-simpleAmbisonics` only.
- After every plugin build: `./scripts/build-and-install.sh O-simpleAmbisonics`, then targeted `auval -v aufx OSAm OuDv` (never `auval -a`).

## Open questions for research (2.2)

- **R1 — Recursion formulas, validated before planning.** Write out the Ivanic–Ruedenberg `u / v / w` coefficient functions, the `P(i, a, b, l)` helper with its three cases (`|b| < l`, `b = l`, `b = −l`), and the index map (paper `m ∈ [−l, l]` ↔ ACN `l² + l + m`, order-1 block indexed from `R₃` with `p = (1, 2, 0)`), incorporating the 1998 erratum. **Validate numerically in a scratch script** (numpy, double) against re-encoding at 200 random tuples at orders 2 and 3 *before* RESEARCH-2.2 is closed — the execute phase transcribes validated formulas. Cross-check index conventions against IEM `calcRotationMatrix` / Green 2003 without lifting code.
- **R2 — Chord error of the per-element matrix lerp.** For a per-grid rotation step δ, the lerped matrix is not orthogonal mid-grid; quantify the worst mid-grid norm dip per band for δ = 11.6° (first grid after a 180° yaw step through the one-pole) and δ = 0.48° (360°/s sweep). Confirm the dip is inside what the ≤ 2.0 second-difference ratio tolerates, or recommend splitting large steps.
- **R3 — Identity exactness.** Walk the recursion at `R₁ = I` element by element in float: confirm every element is exactly `0` or `1` (the `v·V` term for `m = n = l` reduces to `½ · 2`, `u` to `sqrt (1)`). If any element is inexact, D3's digest verdict becomes a `< 1e-6` null and the SUMMARY records it.
- **R4 — `dir()` convention.** Confirm `atan2 (y, x)` with `+y = +90°` (left) and `asin (z)` match the encoder's azimuth sign on the harness side, so the DSP-02 right-hand side cannot pass with a mirrored builder. Behaviour at `|el| = 90°`.
- **R5 — Index helper for the 83-element layout.** Block offsets and the `(l, m, n) → k` map; whether the recursion wants the previous-order block in a `(2l−1)²` scratch with a centred index, and a compile-time sanity (`static_assert` on offsets).
- **R6 — Yaw-sweep stimulus check.** A 360°/s sweep under τ = 20 ms trails by ≈ 7° — fine; confirm the half-circle liveness (≥ 6 dB swing in Stereo) is measurable at 100 Hz with the sweep, and pick the sweep rate / window so the gate is not vacuous (`pattern_gate_stimulus_below_threshold_is_vacuous`).
- **R7 — Processor-level slicing gate with rotation automation.** Confirm `renderHooked` + the engine ragged driver can automate yaw / pitch / roll per block as they do azimuth (P2b / P3b extension needs no new infrastructure) — otherwise name the one helper to add.

## Success criteria (verify phase checks)

- [ ] `ninja O-simpleAmbisonics_VST3 O-simpleAmbisonics_AU O-simpleAmbisonics-render-test` builds with 0 warnings.
- [ ] **digest-2.1 == `9189d911a1fe98d5`** — the entire 2.1 probe set PASS and bit-identical at zero rotation (D3).
- [ ] DSP-02 < 1e-4 at orders 1 / 2 / 3 over 200 tuples; orthogonality < 1e-5; three cardinal sign assertions and `yaw = −azimuth → front` PASS through the frame.
- [ ] P1 null < 1e-5 at a non-zero rotation tuple on a fresh instance.
- [ ] Yaw-sweep and yaw-step click ratios ≤ 2.0 on all 16 bus-tap channels; negative control FAILs as expected; liveness ≥ 6 dB.
- [ ] P2b / P3b bit-equal across 64 / 4096 slicing with yaw / pitch / roll + orbit moving; PERF-01 0 allocations with rotation moving; extremes finite at 48 k / 96 k.
- [ ] Meter frame: `shGain == R_SH·c_eff`, `azRot / elRot` match the reference direction, `azEff` unchanged under yaw.
- [ ] P4 impulse latency still 64; pluginval s10 VST3 + AU and `auval -v aufx OSAm OuDv` PASS on the installed bundles.
- [ ] Independent pedalboard measurer: yaw ±90° in Stereo mode moves the image by the azimuth pan law; `yaw = −azimuth` restores `L == R`.
- [ ] Installed plugin hands-on in a DAW (non-blocking): Yaw spins the image, the Generic editor's Azimuth value does not move; Pitch + raises the front, Roll + raises the left.
- [ ] `SUMMARY-2.2.md` records digest-2.1 (unchanged), digest-2.2, every gate literal, and the deferred list (A14, `kStereoLevelMatch`, TripleBuffer module, −180 / 180 display).

## Sibling references (read at research / execute)

- `research/ARCHITECTURE.md` §SH Rotator, §SH rotation (Ivanic–Ruedenberg), §Visualization Tap, Decisions 5 and 9 — the matrices and signs are quoted there verbatim.
- `research/ambisonics-binaural-decoding-deep-dive.md` §5.2 (block-diagonal structure), §5.3 (the Z-Y-Z / `Ry90` alternative — **not** the chosen route; useful as an independent cross-check in the R1 scratch script).
- `stages/2-dsp/PLAN-2.1.md` §The control step (the op-for-op spec the rotation extends) and `RESEARCH-2.1.md` R3 (chord-error method, reused for R2).
- O-GrainScatter `Source/dsp/SpatialEncoder.h` — encoder convention the rotation must agree with (`+y` = left = `az +90°`).
- O-Octagon `tests/render-harness` — click-gate idiom (`worstCurvature`, matched static control, `checkFailsAsExpected`).
