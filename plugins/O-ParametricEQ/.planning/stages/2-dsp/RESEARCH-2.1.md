# O-ParametricEQ Stage 2 (DSP), Phases 2.1 and 2.1b: Research

**Researched:** 2026-11-12
**Domain:** the section designer alone. Phase 2.1 is the closed-form designer (MLR, cuts, notch, band pass) and gate G1. Phase 2.1b is the accuracy polish that D7 put into v1.
**Method:** a float64 NumPy model of ARCHITECTURE.md "Algorithm Details", written this session, plus a C++ port of the seed and the polish for a cross-check and a timing. All scripts are in `stages/2-dsp/research-2.1/` (index at the end).
**Evidence labels:** **[MODEL]** number from that model, run this session. **[C++]** measured from the C++ port on the development machine (Apple M4 Max, `clang++ -O3`, scalar, no fast-math). **[INFERRED]** reasoning, not run.
**Inputs read:** `stages/2-dsp/CONTEXT.md` (D6–D11, open questions 1–10), `research/ARCHITECTURE.md` (all), `ROADMAP.md`, `stages/0-ideation/CONTEXT.md`, `REQUIREMENTS.md` (DSP-01, PERF-02), `STATUS.md`.

## User constraints (CONTEXT.md, binding; not reopened)

- **D7a.** One section per bell. Section counts, CPU budget and display packet format do not change. Target about 0.15 / 0.30 / 0.70 dB at ±6 / ±12 / ±30 dB at 44.1 kHz; research sets the formula.
- **D7b.** Polish bells and the second-order shelf / tilt sections, weak spots included. Cuts, Notch and Band Pass keep the closed form.
- **D7c.** Closed form is the seed and the fallback. A polished design is used only when finite, stable and closer than the seed.
- **D6, D8–D11** as decided. Phase 2.1 lands the closed form and G1 at the Stage 0 tolerance first; 2.1b tightens it.
- The polish runs on both threads: allocation-free, bounded, deterministic.

## Summary

1. **The Stage 0 model is reproduced.** Every row of ARCHITECTURE "Model results" comes out within 0.01 dB (two cells differ by 0.01). Reciprocity is 3e-11 dB and the 10 Hz, Q 40, +30 dB bell at 192 kHz is within 4e-13 dB. The closed form passes G1 at the Stage 0 tolerance at all five rates on the gate grid. **[MODEL]** The C++ port of `MatchedPair` + MLR matches the model to 1.6e-12 in log parameters. **[C++]**
2. **The premise of D7a was too pessimistic, in the user's favour.** The "second-order optimum" (0.08 / 0.16 / 0.37 dB) is the best *single biquad*. The shipped section is a biquad **plus the corrector one-pole**, and that third-order section can fit the analog prototype to **0.005 / 0.009 / 0.021 dB** at ±6 / ±12 / ±30 dB over the whole G1 grid at 44.1 kHz. Over the full grid a lone biquad is no better than the MLR seed (0.24 / 0.54 / 1.40 dB). **[MODEL]** No section is added: the corrector is already there.
3. **Recommended method: anchored Gauss-Newton on six log parameters.** Two fixed iterations on a 26-point match grid (16 fixed points plus 10 tied to the section's zero and pole), with a Tikhonov pull toward the seed. At 44.1 kHz it reaches **0.04 / 0.08 / 0.19 dB** for bells and **0.04 / 0.12 / 0.19 dB** for 12 dB/oct shelves (Q ≤ 1.41), against the seed's 0.34 / 0.61 / 0.98 and 0.66 / 1.17 / 1.48. That beats the D7a target by 3.5x. **[MODEL]**
4. **The Stage 0 divergence is explained and fixed.** A fixed grid with no points tied to the peak diverges (errors of 1–38 dB; 93–213 of 680 grid settings worse than the seed). Tied points fix the G1 grid; the seed anchor fixes the region above about 0.9 Nyquist. **[MODEL]**
5. **Continuity (question 4) is solved by blend weights, not switches.** The accept test and the extra iterations for hard cases are both smooth weights. Fine parameter sweeps show no jump. A binary version of the same logic does jump (0.06–0.13 dB steps). **[MODEL]**
6. **Reciprocity (question 5) is kept at 1e-9 dB.** Polish the boost, invert algebraically for the cut: 1e-12 dB. **[MODEL]**
7. **Weak spots (question 8) are reachable.** The optimum for resonant shelves near Nyquist is 0.01–0.10 dB, not the "may not be reachable" that CONTEXT feared. The recommended polish gets most of them under 0.25 dB (seed: 5–15 dB). One region stays poor: 12 dB/oct shelf, Q 2–10, ±30 dB, corner above 10 kHz at 44.1 kHz (4.25 dB; optimum 0.01). **[MODEL]**
8. **Cost (question 6): about 0.8 µs per polished section; the seed is 35 ns.** **[C++]** PERF-02's sweeping case is cuts, which are not polished, so it is unaffected. 24 bells sweeping cost about 6 % of a core. 24 shelves at 96 dB/oct all sweeping cost about 46 %, which is over the 35 % line of the cut case. See Q6 for the options; this needs a call in the plan.
9. **Notch and Band Pass above their clamps (question 2) are finite and behave as D10 says, with a visible consequence:** the clamped feature is parked inside the audio band (notch at 20.9 kHz, band-pass peak at 19.8 kHz at 44.1 kHz). Against the true above-Nyquist prototype that is 13–31 dB off below 20 kHz. **[MODEL]** Not reopened here; listed under open questions.

---

## Q1. Closed-form designer reproduced (Phase 2.1)

`eqmodel.py`, `t1_reproduce.py`, `t2_g1_notch_bp.py`. **[MODEL]**

| Row (max \|err\| dB, 10 Hz–20 kHz) | ARCHITECTURE | This session |
|---|---|---|
| Bell 44.1 k, fc ≤ 16 k | 0.32 / 0.58 / 0.92 | 0.32 / 0.58 / 0.92 |
| Bell 44.1 k, 16–20 k | 0.36 / 0.66 / 1.05 | 0.37 / 0.66 / 1.05 |
| Bell 48 k | 0.22 / 0.39 / 0.58 | 0.23 / 0.40 / 0.57 |
| Bell 96 k | 0.06 / 0.10 / 0.14 | 0.06 / 0.10 / 0.14 |
| Bell 44.1 k, 20–30 k (re-centred) | 0.95 / 1.72 / 2.70 | 0.95 / 1.72 / 2.70 |
| Shelf 12, Q 1, fc ≤ 10 k | 0.05 / 0.12 / 0.60 | 0.05 / 0.12 / 0.60 |
| Shelf 12, Q 1, 10–20 k | 0.37 / 0.69 / 1.19 | 0.37 / 0.69 / 1.19 |
| Shelf 12, Q 0.25–1.41, 10–20 k | 0.66 / 1.17 / 1.49 | 0.66 / 1.17 / 1.49 |
| Shelf 6 (Vicanek 2019), 44.1 k | 0.17 / 0.31 / 0.49 | 0.17 / 0.31 / 0.49 |
| Shelf 48, Q 1, 10–20 k | 0.84 / 1.72 / 4.01 | 0.84 / 1.72 / 4.01 |
| Shelf 12, Q 1, 20–30 k | 0.43 / 0.78 / 1.22 | 0.43 / 0.78 / 1.22 |

Boost and cut columns are identical to the printed digits. The bell re-centre rule reproduces as written (`Ge`, `T`, `Qe`).

**G1 on the gate grid at the Stage 0 tolerance** (fc 20 Hz–20 kHz x 40, Q 0.025–40, measured 20 Hz–20 kHz), worst error / tolerance:

| fs | Bell ±6 / ±12 / ±30 | Shelf 12, Q ≤ 1 | Shelf 6 |
|---|---|---|---|
| 44.1 k | 0.35/0.45, 0.61/0.75, 0.98/1.65 | 0.37/0.51, 0.69/0.87, 1.19/1.95 | 0.17/0.22, **0.31/0.34**, 0.49/0.70 |
| 48 k | 0.23/0.45, 0.40/0.75, 0.58/1.65 | 0.32/0.51, 0.62/0.87, 1.16/1.95 | same as 44.1 k |
| 88.2 k | 0.07/0.19, 0.11/0.28, 0.15/0.55 | 0.02/0.19, 0.05/0.28, 0.09/0.55 | 0.12/0.19, 0.21/0.28, 0.33/0.55 |
| 96 k | 0.06/0.19, 0.10/0.28, 0.14/0.55 | 0.02/0.19, 0.04/0.28, 0.09/0.55 | 0.10/0.19, 0.19/0.28, 0.29/0.55 |
| 192 k | 0.02/0.19, 0.03/0.28, 0.04/0.55 | 0.01/0.19, 0.02/0.28, 0.03/0.55 | 0.03/0.19, 0.05/0.28, 0.08/0.55 |

All pass. The thinnest margin is the 6 dB/oct shelf at ±12 dB (8 %). The planner should not tighten that row.

**What remains for Phase 2.1 itself:** the C++ designer and harness. This session ported only `MatchedPair`, MLR and the polish to C++ (60 bell settings, agreement 1.6e-12). The cuts, notch, band pass, first-order shelf and the re-centre rule exist only in the Python model.

## Q2. Notch and Band Pass near and above their clamps

`t2_g1_notch_bp.py`. **[MODEL]** Error counted where the prototype is above −40 dB, measured to 20 kHz, maximum over Q 0.025–40. Every output is finite.

| Type | Zone | 44.1 k | 48 k |
|---|---|---|---|
| Notch | fc ≤ 10 k | 0.63 (Q ≥ 1: 0.13) | 0.51 (Q ≥ 1: 0.07) |
| Notch | 10 k – clamp (0.95 Nyq) | 1.27 (Q ≥ 1: 1.00) | 1.27 |
| Notch | clamp – 25 k, vs true prototype | 13.6 | 5.5 |
| Notch | 25 – 30 k, vs true prototype | 18.9 | 10.9 |
| Band Pass | fc ≤ 10 k | 1.45 | 1.02 |
| Band Pass | 10 k – clamp (0.9 Nyq) | 2.92 (Q ≤ 0.3: 0.48) | 2.92 |
| Band Pass | clamp – 25 k, vs true prototype | 25.4 (Q ≤ 0.3: 2.5) | 9.5 |
| Band Pass | 25 – 30 k, vs true prototype | 30.6 | 14.9 |

Below the clamps the numbers agree with ARCHITECTURE's INFO rows. Above them the large figures are not a design error: the clamp parks the notch at 20.95 kHz and the band-pass peak at 19.85 kHz, where the true prototype would only show a skirt. That is what D10 specifies. Recommended INFO rows for G1: the four "below clamp" cells as regression ceilings; the above-clamp cells reported, not gated.

## Q3. Polish method (Phase 2.1b)

### What the section can reach (question 10: the reference)

`cand.reference` is a converged fit (adaptive Levenberg-Marquardt, p-norm continuation to p = 64, 220 fixed points plus 34 points tied to the peak). It is a local optimiser started at the seed, so each figure is an upper bound on the true minimax optimum. Full G1 grid, 44.1 kHz. **[MODEL]**

| Bell, max err (dB) | ±3 | ±6 | ±12 | ±18 | ±30 |
|---|---|---|---|---|---|
| Seed (MLR) | 0.174 | 0.338 | 0.612 | 0.798 | 0.975 |
| Best single biquad, no corrector (5 free) | 0.127 | 0.242 | 0.544 | 0.790 | 1.397 |
| Same, on Stage 0's subset (fc ≥ 1 k, Q 0.1–10) | 0.039 | 0.069 | 0.127 | 0.181 | 0.646 |
| **Section + corrector, DC exact (6 free)** | **0.003** | **0.005** | **0.009** | **0.014** | **0.021** |

The Stage 0 reference (0.08 / 0.16 / 0.37) is reproduced in kind on its own subset and does not extend to the full grid: at Q below 0.1 a lone biquad cannot follow the prototype to Nyquist. Freeing the DC gain as a seventh parameter changes nothing (0.053 vs 0.054 at ±6 dB in a first run), so DC stays exact as in the seed.

Shelves, same reference, full cascade: 0.00–0.01 dB for Q ≤ 1.41 at every slope; 0.01–0.10 dB in the weak spots (table in Q8).

### Free parameters

`φ = ln (g, k, mB, mH, G, cH)`. `mL` and `cL` stay at the seed's values, so the DC gain is exact by construction. Logs give three properties for free:
- `g, k, G > 0`: every polished design is stable, and linear interpolation between ticks stays stable (the ARCHITECTURE argument is unchanged).
- `mL, mB, mH, cL, cH > 0`: the numerator is minimum phase (DSP-02 holds) and the section has an exact algebraic inverse (Q5).
- The Jacobian is closed form. With `X = t / g²`, `Y = t / G²`, `t = tan²(w/2)`, `u = mL − mH X`, `Nn = u² + mB² X`, `Dn = (1 − X)² + k² X`, `N1 = cL² + cH² Y`, `D1 = 1 + Y`, and `L = ln |H|²`:

```
∂L/∂ln g  = −2X [ (−2 mH u + mB²)/Nn − (−2(1 − X) + k²)/Dn ]
∂L/∂ln k  = −2 k² X / Dn
∂L/∂ln mB =  2 mB² X / Nn
∂L/∂ln mH = −2 mH X u / Nn
∂L/∂ln G  = −2Y [ cH²/N1 − 1/D1 ]
∂L/∂ln cH =  2 cH² Y / N1
residual r_j = (10 / ln 10) (L(w_j) − ln |Ha(w_j)|²)        # dB
```

### Match grid

26 points per section:
- 16 fixed, log-spaced 20 Hz to `min(20 kHz, 0.499 fs)`. Their `tan²` values are constants per sample rate.
- 10 tied to the analog section: for each of the zero pair `(wz, Qz)` and the pole pair `(wp, Qp)`: the centre, and `w0 (sqrt(1 + h²) ± h)` with `h = s / (2Q)`, `s = 0.5` and `1.5`. Clamped into the same 20 Hz–20 kHz band. For a bell `wz = wp`; for a shelf `Qz = Qp`. One rule serves both.

The target is the **true** analog section, also for a bell above 0.95 Nyquist whose seed is re-centred.

### Iteration

```
φ ← φ0 (log of the MLR seed)
repeat K = 2 times:
    A = JᵀJ + (λ diag(JᵀJ) + μ) I          λ = 1e-3,  μ = 0.1 dB² per unit log-parameter²
    b = Jᵀr + μ (φ − φ0)
    φ ← φ − A⁻¹ b                           6x6 Cholesky
```

No step is accepted or rejected inside the loop, so the result is a smooth function of the inputs.

### Evidence for each ingredient

`t5_variants.py`, `t6_robust.py`. **[MODEL]** G1 grid at 44.1 kHz unless stated; counts are settings worse than the seed.

| Variant | ±6 | ±12 | ±30 | Worse than seed |
|---|---|---|---|---|
| 16 fixed points, no tied points, K 2 | 1.41 | 2.91 | 38.3 | 108 / 137 / 213 of 680 |
| 32 fixed points, no tied points, K 3 | 2.05 | 5.57 | 19.9 | 99 / 121 / 168 |
| 16 + tied, K 1, μ 0 | 0.041 | 0.084 | 0.95 | 0 / 0 / 2 |
| 16 + tied, K 2, μ 0 | 0.046 | 0.091 | 0.176 | 0 |
| 8 / 24 / 32 fixed + tied, K 2 | 0.053 / 0.041 / 0.036 | 0.109 / 0.071 / 0.060 | 0.218 / 0.140 / 0.124 | 0 |
| 16 + tied, K 3 | same as K 2 to 0.005 | | | 0 |

- **Tied points are what Stage 0 lacked.** Without them the peak falls between grid points and the fit runs away.
- **K = 2 is converged on the match grid** for everything except the weak spots. K = 1 is not enough at ±30 dB. More fixed points buy accuracy slowly (16 → 32: about 25 %); the residual gap to the optimum is the least-squares-on-26-points objective, not the iteration count.
- **μ matters off the G1 grid.** Random sweep of the full parameter ranges (fc 10 Hz–30 kHz, Q 0.025–40, gain 0–60 dB as `gain_scale` 200 % allows), 30 000 settings per rate, K 2:

| μ | 44.1 k: worse than seed | 48 k | 96 k | G1 grid ±30 |
|---|---|---|---|---|
| 0 | 13, non-finite results | 33, non-finite | 25, non-finite | 0.177 |
| 0.01 | 0 | 2 (+0.79 dB) | 0 | 0.174 |
| **0.1** | **0** | **0** | **0** | **0.165** |
| 1 | 0 | 0 | 0 | 0.176 |
| 10 | 0 | 0 | 0 | 0.312 |

The failures at μ = 0 are bells with Q above 18 and gain above 38 dB centred above 0.92 Nyquist: with the peak outside the measured band several parameter directions are unobserved and plain Gauss-Newton takes huge steps along them. The anchor freezes exactly those directions.

### Alternatives considered

| Alternative | Verdict |
|---|---|
| Plain Gauss-Newton on a fixed grid (Stage 0 trial) | Diverges; table above. |
| Levenberg-Marquardt with accept / reject | Robust offline (it is the reference fitter) but the branch makes the output discontinuous and the iteration count unbounded. |
| Step clamp instead of the anchor | Tried (±1 per log parameter): no gain over μ alone. |
| Precomputed correction table | Not built. A general MLR section is four-dimensional (`wz, Qz, wp, Qp`); bells and shelves are two different 3-D slices, each needing six outputs at a resolution that tracks a 0.02 dB optimum. The runtime fit costs 0.8 µs and has no table to generate, ship or validate. **[INFERRED]** Worth revisiting only if Q6's cost becomes the blocker. |
| Closed-form correction term | No candidate found; the residual has no simple structure across Q. Not pursued. |
| Polish only the five SVF parameters, corrector fixed | Not run separately. The 6-parameter problem is the same size and uses the corrector's freedom, which is where the accuracy comes from. |

## Q4. Continuity under D7c

Three places could make the section parameters jump. Each is a blend. `cand.py`, `t10_cont.py`. **[MODEL]**

1. **Skip.** `S0` = sum of squared residuals of the seed on the match grid. If the seed's RMS error is at or below `Sa = 0.002 dB` the section keeps the seed and no step is run.
2. **Accept ("never worse").** Final parameters are `φ0 + λ (φ − φ0)` in the log domain with
   `λ = sstep((S0/n − Sa²) / (Sb² − Sa²)) · sstep((1 − S1/S0) / 0.1)`, `Sb = 0.006 dB`, `sstep` the cubic smoothstep, and `λ = 0` for any non-finite value. So `λ = 0` where the polish does not reduce the match-grid objective, `λ = 1` where it reduces it by 10 % or more, and it fades in above the skip threshold. The first factor reaches 0 exactly at the skip threshold, so skipping is exact.
3. **Extra iterations for hard cases.** After K = 2, `wx = sstep((rms2 − Sx) / Sx)`, `Sx = 0.03 dB`. Where `wx > 0`, two more iterations run from the K = 2 result and the outcome is `φ2 + wx (φ4 − φ2)`. Bells never trigger it on the G1 grid (share 0.000 in 400 000 random bells **[C++]**).

Test: 21 one-dimensional sweeps at 44.1 kHz (frequency 10 Hz → 30 kHz, gain 0 → 60 dB, Q 0.025 → 40; bells, 12 dB/oct shelves including Q 4 and Q 10, a 48 dB/oct top section), 8000 and 32000 steps. Largest response change between neighbouring steps:

| | 8000 steps | 32000 steps |
|---|---|---|
| Seed, typical | 0.005–0.05 dB | 4x smaller |
| Blended polish | within 2x of the seed on every sweep | 4x smaller on every sweep |
| Same logic with a binary extra-iteration gate | 0.084 / 0.160 / 0.129 dB on three sweeps | 0.058 / 0.094 / 0.129 dB: does not shrink |

A map whose step shrinks in proportion to the sweep step is continuous at this resolution. The binary gate is a real discontinuity; the blend removes it. The bell re-centre boundary (0.95 Nyquist) and the 20 kHz clamp of the tied points are continuous kinks and show no jump.

**Limit of this evidence:** one-dimensional sweeps at one sample rate. Phase 2.1b's harness should run the same test on random lines through parameter space at 44.1 and 48 kHz and gate the ratio.

**Share of sections by weight** (bells, G1 grid, 44.1 kHz, ±6 → ±30 dB): polished 25 → 44 %, blended 7 %, seed kept 68 → 49 %. The seed-kept settings are the ones already within 0.002 dB RMS (low centres, high Q). At 192 kHz 79–94 % keep the seed. The gate can report this histogram directly; it is what D7c asks for.

## Q5. Reciprocity

Design the boost (`gEff ≥ 0`), polish it, and for a cut emit the exact inverse. In log parameters it is linear:

```
SVF      g' = g sqrt(mL/mH)    k' = mB / sqrt(mL mH)
         mL' = 1/mL            mB' = k / sqrt(mL mH)      mH' = 1/mH
one-pole G' = G cL / cH        cL' = 1/cL                 cH' = 1/cH
```

The MLR seed already satisfies this (swapping N and D), so 2.1 and 2.1b use one convention. Measured `|H(+g) · H(−g)|`: at most 1.2e-12 dB over the G1 grid at five rates. **[MODEL]** The 1e-9 dB criterion stands. At 0 dB the target and the seed are both the identity, the residual is zero, and both sides meet there.

For a shelf, "boost" means `Gl > 1` for the whole band; every section of the cascade is then on its boost side. Tilt is a high shelf times a scalar and follows the same rule. **[INFERRED]** (tilt was not run separately).

## Q6. Cost

`polish_bench.cpp`, 400 000 random bells per row, Apple M4 Max, `clang++ -O3`, scalar, one thread. **[C++]**

| | per section |
|---|---|
| Seed (two `MatchedPair` + MLR) | 30–36 ns |
| Seed + polish, K 2, guard evaluation | 765–895 ns |

The ARCHITECTURE cost model assumed 2 µs of design per 8-section band (250 ns per section). The measured seed is 7x cheaper than that; the polish is 3x dearer.

The benchmark runs both iterations before testing the skip. Testing the skip after one objective pass (26 residuals, no Jacobian) would cut skipped sections to roughly a fifth of the cost. **[INFERRED]**

Load at 3000 control ticks per second (K = 16 at 48 kHz, 32 at 96 kHz, 64 at 192 kHz), every section polished on every tick:

| Case, all bands sweeping at once | Polished sections | Design cost |
|---|---|---|
| PERF-02 case: 24 cuts x 96 dB/oct | 0 | unchanged (cuts are not polished) |
| 24 bells | 24 | 5.8 % of a core |
| 24 shelves x 12 dB/oct | 24 | 5.8 % |
| 24 shelves x 48 dB/oct | 96 | 23 % |
| 24 shelves x 96 dB/oct | 192 | 46 % |

These are for this machine; a slower core scales them up. PERF-02's wording ("24 bands at 96 dB/oct within the Stage 0 budget") and gate G10's rows are about cuts, so the binding rows are not affected. The 96 dB/oct shelf row is not a G10 row today, and with the polish it would exceed the 35 % that the cut case is held to.

Options, in the order I would take them:
1. **Polish every tick and add a G10 INFO row** for 24 shelves x 96 dB/oct sweeping. Simplest; correct everywhere; the extreme case is 24 simultaneous automated steep shelves.
2. **Correction hold.** Run the seed every tick (35 ns) and the polish every fourth tick; in between, add the last log-domain correction `φ − φ0` to the new seed. Cuts the table by about 4. The correction varies slowly with the parameters, so this should stay continuous. **Not tested.**
3. **Two-wide NEON / SSE over the 26 points.** About 1.7x. **[INFERRED]**

Rejected: polishing only when the ramp ends. During a drag the response would be the closed form (up to 1 dB off in the top octave) and would then move to the polished one after release. The display shows the target, so curve and sound would disagree while dragging.

The message-thread designer runs the same code: a full display push of 24 bands x 8 sections is about 0.15 ms.

## Q7. Where the polish matters

No sample-rate branch is needed. The skip threshold does it uniformly in normalised frequency: where the seed is within 0.002 dB RMS on the match grid the section keeps the seed. Result on the G1 grid, bells **[MODEL]**:

| fs | Seed ±6 / ±12 / ±30 | Polished | Share polished or blended at ±6 |
|---|---|---|---|
| 44.1 k | 0.338 / 0.612 / 0.975 | 0.043 / 0.081 / 0.193 | 32 % |
| 48 k | 0.227 / 0.396 / 0.573 | 0.016 / 0.026 / 0.073 | 29 % |
| 88.2 k | 0.066 / 0.111 / 0.153 | 0.009 / 0.011 / 0.013 | 17 % |
| 96 k | 0.059 / 0.101 / 0.137 | 0.009 / 0.010 / 0.010 | 16 % |
| 192 k | 0.017 / 0.029 / 0.040 | 0.007 / 0.007 / 0.008 | 9 % |

Zero settings worse than the seed at any rate. The floor of about 0.007 dB is the blend zone between `Sa` and `Sb`.

Bit-identity holds between the two threads and between offline and real-time renders because they call the same compiled function with the same inputs. It does not hold across sample rates and nothing requires it.

Bells above 20 kHz (re-centred seed), random sweep, gain up to ±30 dB: 2.66 → 0.40 dB at 44.1 kHz, 2.51 → 0.17 dB at 48 kHz. **[MODEL]**

Rates below 44.1 kHz (pluginval uses 22.05 and 32 kHz): no non-finite result in 40 000 settings each, and every result is stable by construction. Accuracy there is not improved reliably (2 and 1 settings in 40 000 worse than the seed by more than 0.05 dB, worst +0.35 and +1.57 dB) because centres up to 2.7x Nyquist leave almost nothing to fit. Not gated; G5 covers stability.

## Q8. Shelves and the weak spots

`t8_shelf.py`, `t9_weak.py`. Full cascade, low and high shelf, maximum over the zone. Seed / polished (K 2 plus the blended extension to 4) / reference optimum, at ±6, ±12, ±30 dB. **[MODEL]**

**44.1 kHz**

| Shelf | fc ≤ 10 k | 10 k < fc ≤ 20 k |
|---|---|---|
| 12 dB/oct, Q 0.025–1.0 | 0.09/0.02/0.00, 0.18/0.04/0.00, 0.58/0.10/0.01 | 0.37/0.04/0.00, 0.69/0.12/0.00, 1.19/0.15/0.00 |
| 12 dB/oct, Q 1.0–1.41 | 0.05/0.01/0.00, 0.12/0.01/0.00, 0.76/0.06/0.01 | 0.66/0.04/0.00, 1.17/0.12/0.01, 1.48/0.19/0.00 |
| 24 dB/oct, Q 1 | 0.03/0.01, 0.06/0.01, 0.22/0.04 | 0.63/0.04/0.00, 1.24/0.09/0.00, 2.31/0.18/0.03 |
| 48 dB/oct, Q 1 | 0.03/0.02, 0.05/0.02, 0.15/0.03 | 0.84/0.04/0.00, 1.72/0.07/0.01, 4.01/0.14/0.01 |
| 96 dB/oct, Q 1 | 0.03/0.03, 0.05/0.03, 0.13/0.05 | 0.87/0.06/0.00, 1.77/0.12/0.01, 4.91/0.23/0.02 |
| 48 dB/oct, Q 0.25–4 | 0.05/0.02, 0.11/0.03, 0.29/0.05 | 1.23/0.19/0.02, 2.90/0.14/0.03, 8.23/0.27/0.05 |
| **Weak:** 12 dB/oct, Q 2–10 | 0.13/0.04/0.02, 0.30/0.07/0.03, 4.09/0.19/0.05 | 4.96/0.13/0.03, 7.19/0.25/0.05, **7.63/4.25/0.01** |
| **Weak:** 12 dB/oct, Q 20–40 | 0.14/0.04/0.02, 0.31/0.10/0.04, 6.08/0.23/0.10 | 7.42/0.22/0.08, 11.69/0.93/0.08, 14.91/0.35/0.05 |
| 18 dB/oct, Q 1 (odd order) | 0.07/0.05, 0.14/0.10, 0.43/0.24 | 0.48/0.06, 0.92/0.15, 1.64/0.28 |

**48 kHz:** every regular row at or below 0.03 / 0.06 / 0.11 dB polished; weak rows at or below 0.40 / 0.46 / 0.43 dB (seed up to 11 dB). **96 kHz:** everything at or below 0.01 / 0.01 / 0.06 dB polished.

Findings:
- **The weak spots are reachable** by one section: optimum 0.01–0.10 dB. CONTEXT question 8 expected the opposite.
- **K = 2 alone does not get there**: the seed is 5–15 dB away, too far for two steps (Q 2–10 above 10 kHz: 4.96 → 0.71, 7.19 → 1.00, 7.63 → 5.45; Q 20–40: 7.42 → 3.77, 11.69 unchanged). With the blended extension to 4 iterations the same rows read as in the table. Regular rows are identical with and without the extension.
- **One region stays poor:** 12 dB/oct shelf, Q 2–10, ±30 dB, corner above 10 kHz at 44.1 kHz (4.25 dB). The fit settles in a poor basin there. More iterations do not help (K 6: 4.86). A better seed for that region would; not investigated.
- **Odd orders carry an unpolished first-order section.** D7b names the second-order sections only, so the Vicanek 2019 one-pole is left as is. It then dominates: 18 dB/oct reads 0.24–0.28 dB at ±30 dB where 24 dB/oct reads 0.18. The 6 dB/oct shelf is unchanged (0.17 / 0.31 / 0.49).
- A cascade can read up to about 0.01 dB worse than its seed at error levels of 0.03 dB even though each section is no worse, because section errors of opposite sign no longer cancel. "Never worse" should be gated per section, not per band.

Not run: 96 dB/oct with user Q above 4 (section Q up to 204), and tilt as its own type.

## Q9. Error measure

Keep G1's measure: maximum |dB| over 20 Hz–20 kHz (to `0.499 fs` below 44.1 kHz), against the true analog prototype. Every number in this document uses it, evaluated on 700–1500 log-spaced points **plus** fine points tied to the peak (a log grid alone under-samples a Q 40 peak; the first reference run of this session mis-scored that way).

The polish minimises the unweighted sum of squared dB residuals on its 26 points. That is not the gate's measure, and the gap to the optimum (0.04 vs 0.005 dB at ±6 dB) is the price. It is a deliberate trade: a minimax objective needs reweighting passes, and the result is already 3.5x inside D7a. No frequency weighting is recommended; a weighting was not tested.

## Q10. Proposed G1 tolerance for Phase 2.1b

For the plan to adopt or adjust. Phase 2.1 keeps the Stage 0 tolerance; 2.1b replaces the bell and shelf rows.

| Row | Tolerance | Polished worst (±6 / ±12 / ±30) | Tolerance value | Seed, same cells |
|---|---|---|---|---|
| Bell, Q 0.025–40, 44.1 / 48 k | `0.05 + 0.012 \|g\|` | 0.043 / 0.081 / 0.193 | 0.12 / 0.19 / 0.41 | 0.34 / 0.61 / 0.98 fails |
| Shelf 12 dB/oct, Q ≤ 1.41, 44.1 / 48 k | same | 0.04 / 0.12 / 0.19 | same | 0.66 / 1.17 / 1.48 fails |
| Shelf 24–96 dB/oct, Q 1, 44.1 / 48 k | same | 0.06 / 0.12 / 0.23 | same | 0.87 / 1.77 / 4.91 fails |
| All three, fs ≥ 88.2 k | `0.03 + 0.002 \|g\|` | ≤ 0.013 (bell), ≤ 0.01 (shelves) | 0.04 / 0.05 / 0.09 | 0.07 / 0.11 / 0.15 fails at 88.2 / 96 k |
| Shelf 6 dB/oct | unchanged `0.10 + 0.02 \|g\|` | not polished | | |
| Odd-order shelves (18 dB/oct) | INFO, ceiling 0.10 / 0.15 / 0.30 | 0.06 / 0.15 / 0.28 | | |
| Resonant shelves, Q above 1.41 | INFO, with the ceilings of the Q8 table | | | |

This is tighter than D7a's 0.15 / 0.30 / 0.70 at every gain, with at least 1.6x margin over the model's worst case, and the closed form fails it, so the seed is a second negative control beside the RBJ build. The shelf row widens from Q ≤ 1.0 to Q ≤ 1.41 and the steep shelves move from INFO to binding.

Additional 2.1b rows that this research supports:
- Per section: polished error ≤ seed error + 1e-3 dB over the G1 grid at five rates (measured: zero violations).
- Reciprocity 1e-9 dB (measured 1.2e-12).
- Continuity: neighbour-step response change of the polished design within 2x of the seed's on fine sweeps, and shrinking with the step.
- Weight histogram reported per zone (polished / blended / seed).
- Every output finite with `g, k, G > 0` over the 1e6-point random sweep, gain to ±60 dB, 22.05–192 kHz (this session: 0 non-finite in 8 x 40 000).

---

## Pitfalls

1. **A match grid without points tied to the peak diverges.** This is the Stage 0 failure. The tied points are part of the algorithm, not an optimisation.
2. **Without the seed anchor, a peak outside the measured band produces non-finite parameters** (Q above 18, gain above 38 dB, centre above 0.92 Nyquist). `gain_scale` at 200 % makes ±60 dB reachable, so the robustness sweep must go to ±60, not the ±30 of G1.
3. **Any binary decision in the polish is a parameter jump.** Accept, skip and extra iterations are weights. A binary extra-iteration gate measured 0.13 dB steps.
4. **Scoring on a log grid alone under-reports high-Q errors.** The gate's evaluation grid needs points tied to the peak as well.
5. **The constants are part of the design.** M 16, tied scales 0.5 / 1.5, K 2 (+2), λ 1e-3, μ 0.1, `Sa` 0.002, `Sb` 0.006, `Sx` 0.03, accept ramp 0.1. One header, used by both threads; changing one changes every acceptance number.
6. **No fast-math, and one compiled definition.** The audio and message threads must run the same function for G9 and the bit-identity rows. Keep FP contraction the same for every translation unit that includes the designer.
7. **The target for a re-centred bell is the true prototype**, not the re-centred one.
8. **`mL` and `cL` are not free.** Freeing them gains nothing and gives up exact DC gain.
9. **The inverse needs positive numerator parameters.** The log parametrisation guarantees it for polished sections. Notch (`Qz = ∞`) and Band Pass are not polished and are never inverted.
10. **Skewed defaults** (999.99988 Hz, Q 0.99999976) are ordinary inputs to all of this; nothing here tests for an exact value.

## Consequences for the plan

- **Phase 2.1 is unchanged** from ROADMAP: closed-form designer, `ResponseEval.h`, designer harness, G1 at the Stage 0 tolerance. Add INFO rows for notch and band pass from Q2. The seed's public shape should already be "analog section in, eight parameters out" so 2.1b can wrap it.
- **Phase 2.1b** adds `eq::Polish` behind the same interface: per-rate grid constants prepared once, `refine (grid, analogSection, seed) → parameters, weight`, the inverse map, and the harness rows of Q10. No processor code.
- **Contract edits the plan must make** (CONTEXT already assigns them to the plan phase): ROADMAP new phase 2.1b and its criteria; ARCHITECTURE "Accuracy backlog (not v1)" replaced by the method above; G1 thresholds; Stage 0 decision 14; the Performance table's design-cost line (seed 35 ns, polish 0.8 µs per section, measured); a G10 INFO row for sweeping steep shelves; refresh the checksums in STATUS.
- **D7a's wording** ("near the second-order optimum") should be restated in the amended contract as what was found: the third-order section's optimum is about 0.005 / 0.009 / 0.021 dB and the v1 polish reaches about 0.04 / 0.08 / 0.19 dB.

## Open questions (for the user or the plan; none blocks planning 2.1)

1. **CPU of sweeping steep shelves (Q6).** Accept every-tick polish with an INFO row, or plan the correction hold from the start? Recommendation: every tick, measure in the smoke reading at the end of 2.2, hold as the documented fallback.
2. **The first-order shelf section.** D7b leaves it unpolished, which makes odd-order shelves (18 dB/oct) the least accurate regular shelf after the polish. Extending the polish to it is a two-parameter fit; not studied. Recommendation: leave for v1, INFO row.
3. **The one stubborn weak spot** (12 dB/oct, Q 2–10, ±30 dB, corner above 10 kHz at 44.1 kHz). D7c covers it (reported, not gated). A better seed is the likely fix; v1.x.
4. **Notch and Band Pass above the clamp park a feature in the audio band** (Q2). D10 decided the clamp; this is the measured consequence, raised so it is a known one. No change recommended in v1.

## Not verified

- The closed-form designer in C++ beyond `MatchedPair` + MLR for bells. Phase 2.1 does this.
- The reference optimum is a local fit from the seed; the true minimax optimum may be lower.
- Continuity was tested on 21 one-dimensional sweeps at 44.1 kHz only.
- Timing is from one machine and a research port, not plugin code; the skip-first saving and the SIMD figure are estimates.
- The correction hold (Q6 option 2) is untested.
- Tilt, and 96 dB/oct shelves with user Q above 4, were not run.
- Whether a 0.04–0.19 dB residual versus the seed's 0.3–1.0 dB is audible was not assessed; this is a numerical study.

## Script index (`stages/2-dsp/research-2.1/`)

| File | What it produces |
|---|---|
| `eqmodel.py` | The closed-form designer model (MatchedPair, MLR, bell re-centre, shelf sections, Vicanek 2019 one-pole, responses) |
| `t1_reproduce.py` | Q1 table, reciprocity, 10 Hz / Q 40 / 192 k |
| `t2_g1_notch_bp.py` | G1 margins at five rates; notch and band pass (Q2) |
| `polish.py`, `polish2.py` | Response + Jacobian, reference fitter, early fixed-iteration variants |
| `t5_variants.py`, `t6_robust.py` | Match-grid and μ studies, random full-range sweeps (Q3) |
| `cand.py` | **The recommended algorithm**: grid, anchored iteration, blends, inverse, reference, evaluation |
| `t7_bell_final.py` | Bell tables at five rates, reciprocity, random sweeps at eight rates (Q5, Q7) |
| `t8_shelf.py`, `t9_weak.py` | Shelf tables and weak spots (Q8). `python3 t8_shelf.py 48000` for another rate |
| `t10_cont.py` | Continuity sweeps (Q4) |
| `polish_bench.cpp` | C++ port: timing (Q6) and the dump compared with `cand.py`. `clang++ -O3 -std=c++17 polish_bench.cpp -o polish_bench` |

Run from that directory with `python3 <file>` (NumPy 2.4). The long ones take 1–4 minutes.

## Sources

- `research/ARCHITECTURE.md` (Algorithm Details, Model results, Verification Gates, Performance) and the Vicanek 2016 / 2019 formulas as restated there. The papers were not re-read this session.
- Gauss-Newton, Levenberg-Marquardt and Tikhonov regularisation are textbook methods; no library is used at run time.
