# O-ParametricEQ Stage 2 (DSP), Phase 2.1b: Research on polish continuity

**Researched:** 2026-11-13
**Scope:** the C1 failure of `SUMMARY-2.1b.md` (the polished map jumps for resonant shelf sections with the corner near or above Nyquist). Route 1 of "Decision needed". Everything else in `RESEARCH-2.1.md` stands unless amended below.
**Method:** a C++ bench over the real `Source/dsp` headers, `stages/2-dsp/research-2.1b/cont_bench.cpp`: the shipped polish with run-time knobs. With default knobs it is bit-identical to `eq::Designer::design` on 600 000 random designs (`sanity` mode). Every number below is from that bench on this machine. Nothing in `Source/`, the harness, the model or the goldens was changed.
**Not in the bench:** the Python model, the D2 golden, the real harness rows. The bench's accuracy grids are close to the harness's but not the same code (its shipped-polish figures agree with the harness to 0.001-0.005 dB on the gated rows).

## Summary

1. **The jump is a diverging fit, not a basin switch.** In the failing section the second Gauss-Newton step moves `ln mB` by +19.5 and the match-grid error goes from 5.2 dB to 118.8 dB RMS. The four-step end point is chaotic and the accept weight follows it.
2. **There is a second defect that C1 and NW1 do not see.** Off the gated grid a polished section can be far worse than its seed between the match points: up to +54 dB at 44.1 kHz and +19 dB at 48 kHz on random full-range shelves. The 26-point accept test does not catch it.
3. **Both come from one region and one cause:** a section with Q of about 4 or more whose upper pair (zeros or poles) is above about 0.9 Nyquist. There the closed-form seed is 5-12 dB off, which is too far for four undamped steps. Everywhere else the shipped polish is clean.
4. **Recommended fix, two parts, both needed:**
   - **A re-centre rule for shelf sections, on the seed side** (bells already have one). A pair above 0.95 pi is designed at 0.95 pi with an effective Q, faded in on the pair's Q.
   - **A smooth limit on the length of every polish step** (0.35 log units).
5. **Result at 44.1 / 48 kHz:** 0 non-shrinking jumps on 5340 lines (shipped: 19 on 2700 of them); no runaway section in 4.5 million; worst excess over the seed +0.67 dB (shipped +54); every gated G1b row keeps its margin; the weak spots improve, including the "stubborn" cell (4.46 -> 0.45 dB).
6. **Two things remain and need a call:** about 1 line in 1000 is still steeper than 2x the seed (continuous, 2.4x at worst), so C1's 2x limit fails occasionally as written; and at 22.05 / 32 kHz the polish is not continuous with or without the fix (the seed alone is).

---

## Q1. What the C1 failure is

`cont_bench diag` on the harness's worst line (48 dB/oct High Shelf, 48 kHz), section 0 (wz 2.98, wp 4.40 rad/sample, Q 37.4), log-parameter moves from the seed:

| | ln g | ln k | ln mB | ln mH | ln G | ln cH |
|---|---|---|---|---|---|---|
| step 1 | -2.89 | +0.90 | -0.47 | +1.82 | -0.96 | 0.00 |
| step 2 | -3.34 | +1.90 | **+19.51** | +0.94 | -10.24 | -0.02 |
| step 4 | -5.83 | +1.07 | +9.43 | 0.00 | -14.84 | -4.47 |

Match-grid RMS: seed 5.16 dB, after two steps 118.8 dB, final 5.06 dB (`S1/S0` 0.958, accept weight 0.37). Bisecting the line shows no true discontinuity: the response moves 7.3 dB over 4.9e-7 of the line and the gap closes below that. So the map is continuous and useless there.

Why: with the pole pair far above the band, `X = t / g^2` is at most 0.012 across the match grid, the section is nearly linear in `X`, and four of the six parameters are almost unobserved. The anchor `mu = 0.1` is weak against 5 dB residuals, so the step along those directions is huge. Sections 1-3 of the same band (Q 0.5-0.9) converge in two steps with moves below 0.06.

## Q2. Where the polish is unreliable

`cont_bench map`: random shelves at all slopes (fc 10 Hz-30 kHz, gain 0-60 dB, Q 0.025-40), 2.25 million sections per rate, each scored against its own prototype on 400 log points plus points tied to its pairs. Binned by `u` = upper pair / Nyquist and the section Q. Shipped polish, 44.1 kHz, the bad cells:

| u | Q | n | seed mean | polished mean | worst polished | worse than seed (max) | runaway |
|---|---|---|---|---|---|---|---|
| 1.0-1.1 | >= 12 | 1722 | 5.32 dB | 0.60 | 23.0 | 37 (+15.7 dB) | 17 |
| 1.1-1.3 | >= 12 | 3066 | 7.96 | 1.28 | 33.8 | 68 (+22.7) | 260 |
| 1.3-2.0 | >= 12 | 4638 | 10.51 | 2.54 | 67.9 | 91 (+54.2) | 708 |
| above 2 | >= 12 | 1646 | 12.35 | 1.05 | 23.6 | 4 (+8.1) | 70 |
| 1.1-2.0 | 4-12 | 5094 | 5.2-6.3 | 0.23-0.37 | 8.1 | 7 (+1.1) | 6 |

"Runaway" = accepted with g or k more than 10x from the seed. Every cell with `u < 0.9`, and every cell with Q below 4, has 0 runaway sections and at most +0.01 dB excess. Totals for the region `u >= 0.9, Q >= 4` are in Q4.

The polish is still worth having there on average (mean 6.8 -> 1.0 dB), which is why switching it off in that region (route 2) gives up the weak spots.

## Q3. Candidates

All at 44.1 / 48 kHz. "Jumps" = lines whose 32 000-step change is more than half the 8000-step change (C1's second test). "Steep" = lines failing only the 2x-of-seed test. Lines: 1800 random shelf / tilt lines (12, 24, 48, 96 dB/oct shelves; 12 and 48 dB/oct tilt), 150 per family and rate.

| Variant | Jumps | Steep | Hard region: polished mean | worst excess over seed | runaway |
|---|---|---|---|---|---|
| Shipped | 5 of 840 (first set) | 2 | 0.99 / 0.63 dB | +54.2 / +18.6 | 1072 / 662 |
| Step limit 0.5 alone | 0 | 6 (up to 18.7x) | 0.63 / 0.28 | +0.80 / +0.09 | 0 |
| Step limit 0.7 (with the own-pole accept points) | 0 | 6 (up to 28x) | 0.40 / 0.19 | +0.87 / +0.64 | 0 |
| Step limit 1.0 alone | not run | | 0.38 / 0.22 | +5.1 / +8.7 | 0 |
| Residual-scaled damping (`nuRes` 10) | not run | | 0.69 / 0.61 | +7.7 / +13.2 | 0 |
| Accept also scored at the fitted section's own pole and zero | not run | | 0.98 / 0.62 | +22.7 / +14.1 | 1115 / 688 |
| 4 more fixed points in the top octave | not run | | 1.35 / 1.20 | +48 / +42 | 1492 / 1073 |
| Re-centre alone | 7 (up to 1.4 dB) | 6 | 0.24 / 0.10 | +0.55 / +0.55 | 0 |
| Re-centre + step limit 1.0 | 1 | 4 | 0.24 / 0.10 | +0.55 / +0.56 | 0 |
| Re-centre + step limit 0.7 | 1 | 4 | not run | | |
| **Re-centre + step limit 0.5** | **0** | **0** | 0.24 / 0.10 | +0.54 / 0 | 0 |
| **Re-centre + step limit 0.35** | **0** | **0** | 0.24 / 0.10 | +0.48 / 0 | 0 |

Readings:
- **A step limit alone removes the jumps and not the steepness.** With the seed 5-7 dB away the first two steps saturate the limit and their direction is itself sensitive; four steps do not converge (0.5-0.6 dB RMS left).
- **The re-centre alone removes the runaway fits and most of the error but not every jump.** The seed is then 2 dB away instead of 7.
- **Together they pass.** The limit is 0.35 or 0.5; 0.7 is too loose.
- The distance-from-seed fade probed in the execute session was not re-run; it acts after the divergence and cannot help.

Not tried: more iterations in the hard region (cost), a homotopy on the target, a table.

## Q4. The recommended change

**(a) Shelf re-centre, seed side** (`Prototype.h`, `detail::shelfSections`: `designSide` differs from `trueSide`, as it already does for a bell). For each pair `(w0, Q)` of a second-order shelf / tilt section with `w0 > wL = 0.95 pi`:

```
beta = sstep ((ln Q - ln 1.5) / (ln 4 - ln 1.5))        # 0 at Q <= 1.5, 1 at Q >= 4
wT   = w0 (wL / w0)^beta
x    = wT / w0
Qe   = 1 / sqrt ((1 - x^2)^2 + x^2 / Q^2)               # clamp to [0.025, 160]
design the pair as (wT, Qe)
```

The pair keeps its DC value and its magnitude at `wT`. It is continuous at `w0 = wL` (`Qe = Q`) and at `beta = 0`. The polish target stays the true section. `0.95 pi` is the existing `bellRecentre`; 0.9 pi and 1.0 pi both measured worse (0.9: 3608 sections worse than the seed; 1.0: seed mean 2.57 against 2.15 dB).

Without the Q fade the rule makes low-Q pairs slightly worse (rest of the space, seed mean 0.049 -> 0.061 dB); with it 0.050 and the hard region is unchanged. Fades 1-3 and 2-6 behave the same within 0.05 dB.

**(b) Step limit** (`Polish.h`, `polishStep`, after the solve): `delta <- delta * R / sqrt (R^2 + |delta|^2)`, `R = 0.35`, `|delta|` the Euclidean norm of the six-parameter step. No branch. One `sqrt` per step.

**Effect on the closed form alone** (hard region, 44.1 / 48 kHz): mean 6.81 / 5.51 -> 2.15 / 2.21 dB, worst 16.6 / 12.2 -> 3.9 / 3.8 dB. So the fallback improves too.

**Totals with (a) + (b), R 0.35**, 2.25 million sections per rate:

| | 44.1 kHz shipped | 44.1 kHz fixed | 48 kHz shipped | 48 kHz fixed |
|---|---|---|---|---|
| Hard region, polished mean | 0.985 dB | 0.240 | 0.625 | 0.097 |
| Hard region, worst polished | 67.9 dB | 1.49 | 26.6 | 2.05 |
| Hard region, worse than seed (max) | 256 (+54.2) | 331 (+0.48) | 337 (+18.6) | 0 |
| Rest, polished mean | 0.007 dB | 0.007 | 0.003 | 0.003 |
| Rest, worse than seed (max) | 17 (+0.01) | 82 (+0.14) | 260 (+0.25) | 250 (+0.22) |
| Runaway | 1072 | 0 | 662 | 0 |

The 48 kHz "rest" figure is in the shipped design already and includes bells (13-15 of 200 000 random bells, +0.22 dB, gains to 60 dB). It is off the NW1 grid.

## Q5. Continuity with the fix

Harness criteria (8000 and 32 000 steps, 160-point grid, 2x of the seed, shrink 0.5).

| Lines | Shipped | Fixed, R 0.5 | Fixed, R 0.35 |
|---|---|---|---|
| 840, bench default seed, 7 families, 44.1 / 48 k | 7 violations, 5 jumps | not run | 0 |
| 1800 shelf / tilt, 44.1 / 48 k | not run | 0 | 0 |
| 2700, fresh seed, 9 families (adds 18 and 72 dB/oct), 44.1 / 48 k | 27 violations, 19 jumps | 3 steep, 0 jumps | 2 steep, 0 jumps |
| 1080, 88.2 / 96 / 192 k | not run | 0 | not run |
| 720, 22.05 / 32 k | 70 violations, 30 jumps | 59 violations, 7 jumps | not run |
| 720, 22.05 / 32 k, seed only | 0 | | |

The two remaining lines at R 0.35 (12 dB/oct Low Shelf and 72 dB/oct High Shelf, 44.1 kHz): polished step 2.41x and 2.40x the seed's (0.041 against 0.017 dB; 0.41 against 0.19 dB at 8000 steps), shrinking 4x per 4x steps. One was traced: section 0 at Q 32 with both pairs above Nyquist, four limited steps not yet converged (0.15 dB RMS left). A wider accept ramp does not change them; removing the extension makes it worse (4 lines).

The bench's lines are not the harness's 341 lines (different family order in the generator), so "C1 passes" is not shown here; the rate of about 1 steep line per 1000 says a 341-line draw usually passes and sometimes will not.

## Q6. Accuracy with the fix

Worst band error at +-6 / +-12 / +-30 dB, shipped -> fixed (R 0.35). Tolerance 0.122 / 0.194 / 0.410 at 44.1 / 48 kHz.

| Row, 44.1 kHz | Shipped | Fixed |
|---|---|---|
| Bell, Q 0.025-40 | 0.043 / 0.081 / 0.192 | the same |
| Shelf 12 dB/oct, Q <= 1.41 | 0.043 / 0.120 / 0.182 | 0.043 / 0.120 / 0.184 |
| Shelf 24 dB/oct, Q 1 | 0.038 / 0.091 / 0.177 | 0.038 / 0.092 / 0.186 |
| Shelf 48 dB/oct, Q 1 | 0.043 / 0.074 / 0.143 | 0.043 / 0.079 / 0.153 |
| Shelf 72 dB/oct, Q 1 | 0.052 / 0.103 / 0.161 | 0.052 / 0.109 / **0.247** |
| Shelf 96 dB/oct, Q 1 | 0.059 / 0.117 / 0.232 | 0.059 / 0.119 / 0.213 |
| Weak: 12 dB/oct, Q 2-10, 10-20 k | 0.17 / 0.43 / **4.46** | 0.18 / 0.44 / **0.45** |
| Weak: 12 dB/oct, Q 20-40, 10-20 k | 0.82 / 3.12 / 0.35 | 0.75 / 1.03 / 0.43 |
| Weak: 12 dB/oct, Q 2-10 and 20-40, fc <= 10 k | 0.04 / 0.07-0.10 / 0.16-0.21 | 0.04 / 0.07-0.10 / 0.25-0.39 |
| 96 dB/oct, Q 4 / 10 / 40 | 0.30 / 0.31 / 0.69 | 0.31 / 0.33 / 0.31 |
| 12 dB/oct, Q 2-40, fc 20-30 k | **16.5 / 38.7** / 0.71 (seed 11.8 / 14.8 / 15.0) | 0.58 / 0.22 / 0.38 |

At 48 kHz every gated row is within 0.02 dB of the shipped figure (worst 0.118 against 0.410) and the weak rows are at or below 0.34 / 0.52 / 0.17. At 96 kHz nothing changes. With R 0.5 the 72 dB/oct cell reads 0.202 and the rest is within 0.05 dB of the R 0.35 column.

The thinnest gated cell is still shelf 12 dB/oct at +-12 dB (0.120 against 0.194), unchanged. The 72 dB/oct cell at +-30 dB loses margin (0.161 -> 0.247 against 0.410).

**The Phase 2.1 seed changes** only for shelf / tilt sections with Q above 1.5 and a pair above 0.95 pi. Bells and the 2.1-gated shelf rows (12 dB/oct, Q <= 1.41; 6 dB/oct) are bit-unchanged. Steep-shelf seeds at +-30 dB move both ways by up to 0.4 dB (36 dB/oct 3.26 -> 3.64, 96 dB/oct 4.91 -> 5.04 at 44.1 kHz; these are INFO in 2.1).

## Q7. Rates below 44.1 kHz

At 22.05 and 32 kHz the match band runs to 0.998 Nyquist, above the re-centre point, and the polish is not continuous for any family, bells included (shipped: 30 jumping lines of 720; fixed: 7, up to 3.6 dB). The seed alone is clean. `RESEARCH-2.1.md` Q7 already found no reliable accuracy gain there.

Recommendation: do not polish below 44.1 kHz (`Designer::prepare` decides once per sample rate, so it is not a per-design switch and cannot make a jump). Not tested in the bench beyond the seed-only row.

## Pitfalls

1. **NW1 on the G1 grid is not "never worse".** The shipped design passes NW1 with 0 violations and is +54 dB worse than the seed off the grid. A random full-range never-worse row with a ceiling is needed.
2. **C1's worst line hides the rest.** The 637x line dominated the row; the steep-only lines only show once the jumps are gone.
3. **A step limit that scales a diverging step keeps its direction.** That direction is the unstable part; the limit needs a seed close enough that the steps are not saturated.
4. **The accept weight cannot repair a bad fit.** It compares 26 points; a resonance between them is invisible. Scoring the fitted section's own pole and zero frequency halves the worst case and does not remove it.
5. **zsh does not word-split `$v`.** Variant loops in this session silently ran only the first knob until `${=v}` was used; the affected rows were re-run.

## Consequences for the plan

- **Contracts:** ARCHITECTURE ("Algorithm Details": shelf re-centre; "Accuracy polish": step limit, new constants), ROADMAP Phase 2.1b criteria; checksums refreshed. The re-centre is a Phase 2.1 change.
- **Phase 2.1 artefacts to redo:** `tests/model/eqmodel.py` (the rule), `tests/golden/designs.tsv` regenerated (30 MB), D1 and the 2.1 rows re-run with `designSeed`. P1's bell re-centre continuity row needs a shelf twin.
- **Phase 2.1b artefacts:** `Prototype.h`, `Polish.h`, `PolishConstants.h` (step limit; the re-centre constants belong with `bellRecentre`), `polish_model.py`, `designs-polished.tsv` regenerated (17 MB).
- **New or changed harness rows:**
  - C1: add 24 and 96 dB/oct shelf and 48 dB/oct tilt families and more lines (the shipped row has 2 of the 6 failing families).
  - C1n: score only lines the shipped design passes (SUMMARY-2.1b caveat).
  - A random full-range never-worse row per section with a ceiling (measured worst +0.67 dB over 2.8 million sections at 44.1 kHz).
  - A runaway count row (accepted with g or k more than 10x from the seed): 0.
  - I-weak ceilings from Q6.
- **Alternative with a smaller footprint, not recommended:** use the re-centred section only as the polish start point and keep the 2.1 seed as the fallback. It leaves `designs.tsv` alone, and gives up the 3x better fallback and makes the accept blend run between two different sections. Not tested.

## Open questions for the user or the plan (answered)

**Decided by Taylor 2026-11-13 (all four as recommended):** (1) C1's ratio limit becomes 4x of the seed, the shrink test (0.5) stays binding; (2) no polish below 44.1 kHz, decided once per sample rate in `Designer::prepare`; (3) the shelf re-centre is a seed change (Phase 2.1 model, designs golden and 2.1 rows are redone), not a polish start point; (4) step limit 0.35.

1. **C1's 2x-of-seed limit.** With the fix about 1 random line in 1000 reads 2.0-2.4x (continuous, shrinking). Options: raise the limit to 4x and keep the shrink test binding (recommended: the shrink test is the one that detects a jump); or keep 2x and accept an occasional red C1 depending on the draw.
2. **Polish off below 44.1 kHz** (Q7). Recommended.
3. **Step limit 0.35 or 0.5.** Both pass; 0.35 has one fewer steep line and costs 0.05 dB in one gated cell. Recommended 0.35.
4. **Re-centre as a seed change (recommended) or as a polish start point only.**

## Not verified

- The Python model with these changes, D2, and the real harness: all numbers are from the bench.
- The harness's own 341 C1 lines.
- Reciprocity, tilt identity, allocation and thread rows with the fix (the fix is on the boost side, before the inversion and the tilt scalar, and adds no state; expected unchanged).
- Cost. The step limit adds one `sqrt` per step and the re-centre a few operations per pair above 0.95 pi; not timed.
- Shelf re-centre continuity as a row of its own (covered only through the C1 lines).
- Whether a 2.4x-steeper spot or a 0.45 dB residual in a resonant shelf above 10 kHz is audible.

## Bench index (`stages/2-dsp/research-2.1b/cont_bench.cpp`)

`clang++ -O3 -std=c++17 -I ../../../../Source cont_bench.cpp -o cont_bench`, then `./cont_bench <mode> key=value ...`.

| Mode | Produces |
|---|---|
| `sanity` | bench == `eq::Designer::design`, bit for bit |
| `diag` | the worst step of one line and the per-step trace (Q1); `dtype dsi dfs dfc0 dfc1 dg0 dg1 dq0 dq1` |
| `map` | reliability by pair position and Q (Q2, Q4); `n`, `rates`, `quiet=1` for totals |
| `c1` | the line test (Q3, Q5); `lines`, `fam` (bit mask), `seed`, `rates`, `verbose=1` |
| `acc` | accuracy rows (Q6) |
| `rand` | per-family never-worse and runaway census |

The recommended design: `rc=1 rcQa=1.5 rcQb=4 trust=0.35`. Other knobs: `trust`, `nuRes`, `muRes`, `val`, `topPts`, `rcW`, `ramp`, `extra`, `polish=0`.
