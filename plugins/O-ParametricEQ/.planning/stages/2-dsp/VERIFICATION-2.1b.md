# Stage 2: DSP — Phase 2.1b (Designer polish, with the continuity rework) - Verification

## Verification Date

2026-11-13 (verify session). What was run:

- An independent re-run against `6974cf2b`: the harness object was deleted and recompiled, and the run was compared with the SUMMARY block.
- An independent audit of the seed change against the verified Phase 2.1 golden (`291abd62`), with the true pairs written from ARCHITECTURE, not taken from the model.
- Both goldens regenerated from the model, plus a C++ dump.
- Every negative control.
- A 17-mutant run of the designer headers.
- A full read of `Polish.h` and `PolishConstants.h`, and of the 2.1 → 2.1b diff of `Prototype.h`, `EqTypes.h` and `Designer.h`.

Scope: Phase 2.1b only, which is the first build plus the rework. Stage 2 has five more phases (2.2 – 2.6), so this file does not close the stage.

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 2.1b as amended; PLAN-2.1b, PLAN-2.1b-rework; CONTEXT D7)

1. The accuracy polish exists behind the same designer interface: anchored Gauss-Newton on six log parameters, blend weights rather than switches, cuts made by exact inversion, and one constants header.
2. G1 at the 2.1b thresholds at five rates. The RBJ build and the closed-form seed both fail the 44.1 kHz bell row (DSP-01).
3. Never worse per section on the G1 grid (+1e-3 dB), and never far worse anywhere in the full range (+1.0 dB). No runaway fit.
4. Continuous along random lines at all five rates (4x of the seed, with the shrink test binding). The first 2.1b build fails this row, the never-worse row and the runaway count.
5. Below 44.1 kHz the design is the closed form, bit for bit.
6. The shelf re-centre is continuous at its boundaries and reaches nothing outside its region. Every design outside the region is bit-unchanged from the verified 2.1 golden.
7. The usual protections hold with the polish on: reciprocity, finite output over the full range, skip exactness, the tilt identity, no allocation, and thread independence. Cost is reported.
8. Every Phase 2.1 row still passes.
9. No processor code, the plugin binary unchanged, and Stage 1 still green.

### Deliverables (from SUMMARY-2.1b-rework.md, confirmed in the tree)

1. `Source/dsp/Polish.h` (468 lines) and `PolishConstants.h` (90). `Designer.h` gains `design` (polished), `designSeed`, `designWith<Policy>`, the rate flag in `prepare` and `isPolishEnabled()`. `Prototype.h` gains `detail::recentrePair` and three constants. `EqTypes.h` holds the single `smoothStep` and `BandDesign::polishWeight`.
2. `tests/model/polish_model.py` and `dump_polish.py`; `eqmodel.py` gains the re-centre.
3. `tests/golden/designs.tsv` regenerated, plus the new `designs-polished.tsv` (88,935 rows).
4. `tests/designer-harness/main.cpp`: 54 rows + 12 INFO, `--neg seed | rbj-b | binary-gate | first-build`.
5. ARCHITECTURE "Model results", row "Shelf 48 dB/oct, Q 1, 10–20 k" amended to 0.84 / 1.85 / 4.33 (Taylor's call during execute).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Polish behind the interface | ✅ Achieved | Read in full. The six free parameters are `g k mB mH G cH`, while `mL` and `cL` are copied from the seed, so DC stays exact. The 26-point grid is 16 fixed points plus 10 tied to the true pairs, clamped to the band. The step is a 6x6 Cholesky with the step limit under `if constexpr`, so there is no runtime switch. The accept weight is `smoothStep × smoothStep`; the extension is blended by `wx`. The cut is the inverse of the polished boost (`finish` runs after the polish). Each constant is defined in exactly one place (grep below). |
| 2 G1b | ✅ Achieved | Every cell is inside tolerance at five rates. The thinnest margin is the 96 dB/oct ±18 dB cell at 44.1 kHz, 0.205 against 0.266. G1bn-seed fails at 0.975 dB and G1bn-rbj at 17.39 dB. `--neg seed`, `--neg rbj-b` and `--neg rbj` have rc 0. |
| 3 Never worse, no runaway | ✅ Achieved | NW1: 0 of 178,000 sections worse. NW2: worst +0.660 dB at 44.1 kHz and +0.270 dB at 48 kHz, over 2,598,194 sections per rate (ceiling +1.0). RA1: 0 runaways in 1,451,300. |
| 4 Continuity | ✅ Achieved | C1: 0 of 1101 lines; worst 2.167x, shrink 0.295. C1-hi: 0 of 216. `--neg first-build` fires on all three rows (NW2, RA1 1949, C1 637.7x / shrink 0.988). `--neg binary-gate` fires (13 of 181). See Issue 1 for what C1 does **not** detect. |
| 5 No polish below 44.1 kHz | ✅ Achieved | PR1: 20,000 designs, all `memcmp`-equal to `designSeed` with weight 0, while 1773 of the same settings are polished at 44.1 kHz. In the code: `polishEnabled = fs >= 44100` in `prepare`, and `numToPolish = 0` when it is off. |
| 6 Re-centre region and boundaries | ✅ Achieved | **Independent audit, written in this session.** Of 162,855 lines, 4800 changed against the `291abd62` golden; 4800 lines are expected by the ARCHITECTURE rule (true pair `w0 > 0.95 π` and `Q > 1.5`), and the two sets are equal: 0 lines changed that should not have, and 0 expected lines unchanged. All the changes are `gain2` sections, 1600 each of Low Shelf, High Shelf and Tilt; no key column differs. P1s: boundaries within 3.27e-08 relative; 0 of 499,568 pairs changed outside the region. In the code, `trueSide` is never touched (`addSection (…, d, a, …)`). |
| 7 Protections with the polish on | ✅ Achieved | R2 4.62e-13 dB; F2 8,008,192 designs with 0 bad; S1 0 of 10,000 differ; TL2 5.79e-15 dB; A2 0 / 0 allocations with the planted one counted; B2 10,000 / 10,000. I-cost 685 ns per polished bell and 90 ns for the seed. |
| 8 Phase 2.1 rows | ✅ Achieved | All of them pass. T1 passes against the amended row; the move it records is the re-centre's own (the section has Q 2.56), and the audit above accounts for it. |
| 9 No processor change | ✅ Achieved | `git diff 291abd62 HEAD -- Source/*.cpp Source/*.h CMakeLists.txt` is empty, and no `dsp/` include appears in the processor. Render harness 36 / 0; `check_spec.py` `OK: 196 params`. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.1b of 7)
**Requirements touched by this phase:** 2 (both must)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-01: analog-matched bell and shelf | must | ⚠️ Partial | Criterion 1 is met on the design side at the tightened Phase 2.1b tolerance (G1b at five rates, controls fire). It does not close yet: criterion 2 ("no oversampling, latency 0") and G1's rendered-impulse half need the audio path (Phase 2.2, Gate G8). Status `pending → partial`. |
| DSP-06: parameter ranges | must | ⏸️ Pending | The design side holds with the polish on (F2: gain to ±60 dB, 22.05 – 192 kHz). Reachability and above-Nyquist rendering are 2.2 / Stage 3. |

**Requirements Summary:**
- ✅ Complete: 0 new (COMPAT-01 from Stage 1)
- ⚠️ Partial: 2 (DSP-07 unchanged; DSP-01 new)
- ⏸️ Pending / later phase: 24
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | brief, parameter-spec, ARCHITECTURE and ROADMAP all equal STATUS |
| Commit hygiene | ✅ Pass | `6974cf2b` lists only this plugin's paths; the plugin tree is clean; no tag on HEAD |
| Build (harness object deleted, recompiled) | ✅ Pass | rc 0. The one warning comes from the SDK's libc++ header (`availability.h`, "platform no longer supported"), not from this plugin. |
| Designer harness | ✅ Pass | `54 PASS, 0 FAIL, 12 INFO` in 35 s. The 2.1b block (171 lines, timing rows excluded) is identical to the SUMMARY's verbatim block. |
| Determinism / goldens | ✅ Pass | `designs.tsv`: committed file, model regeneration and C++ `--dump-designs` share SHA-1 `3a733591…`. `designs-polished.tsv`: committed file and model regeneration share SHA-1 `c09ce316…`. |
| Negative controls | ✅ Pass | `first-build`, `binary-gate`, `seed`, `rbj-b`, `rbj`, `f1`, `alloc` all have rc 0; an unknown name gives rc 255 |
| Constants in one home | ✅ Pass | `0.35` and `44100` appear only in `PolishConstants.h`. The `1.5` and `4.0` in `Prototype.h` are the two definitions plus the 2.1 exponent in `analogSecondOrderMag2`. |
| Static gates | ✅ Pass | `Source/dsp` contains no JUCE, heap, string, vector or static-state tokens. The processor and CMake are unchanged. |
| Stage 1 render harness, `check_spec.py` | ✅ Pass | 36 / 0; `OK: 196 params` |
| Independent re-centre audit | ✅ Pass | 4800 = 4800, with 0 differences in either direction (goal 6) |

Not run, by design: plugin build, install, pluginval, auval. The plugin binary does not include `Source/dsp/` and is still 0.1.0.

### Mutation run (new in verify)

Seventeen mutated copies of `Source/dsp` were made in the session scratchpad. Each was compiled with the unmodified harness (clang++ -O3, the committed goldens), and nothing in the repo or in `build/` was changed. SUMMARY-2.1b-rework noted that the new rows had not been run against mutants; this run closes that gap. The column that matters is what catches a mutant **besides** D1 / D2, which only prove the C++ equals the model.

| Mutation | Rows failing besides D1 / D2 |
|---|---|
| M0 unmodified control | none (54 / 0 / 12) |
| Mc `minSampleRateHz` 48000 | G1b-bell, G1b-sh12, G1b-shN, PR1 (vacuity arm: 0 polished at 44.1 kHz) |
| Md rate gate `>` instead of `>=` | G1b-bell, G1b-sh12, G1b-shN, PR1 |
| Me `minSampleRateHz` 22050 | PR1 (3551 of 20,000 designs polished); D2 does not see it (no golden line below 44.1 kHz) |
| Mf `shelfRecentre` 0.90 π | T1, P1s ×2, C1, NW2 |
| Mg `shelfRecentreQLo` 1.0 | T1, P1s ×2 |
| Mh re-centre also applied to `trueSide` | T1, P1s ×2 (D1 passes: design side unchanged) |
| Mi re-centre keeps Q (no `Qe`) | T1, P1s, C1 |
| Mj only the zero pair re-centred | Z1, P1s, C1 |
| Mm polish target = design side, not true side | G1b-shN |
| Mn `prepare` does not rebuild the grid | G1b ×3, NW1, C1, NW2 |
| Mp extension never runs | G1b-shN, C1n |
| Ma **step limit compiled out** | **none** (D2 only) |
| Mb step limit 0.5 | **none** (D2 only) |
| Mk β by linear clamp, not smoothstep | **none** (D1 / D2 only) |
| Mo tied-point spacing h = s / Q | **none** (D2 only) |
| Ml **accept weight ignores the improvement factor** | **none, and not D2 either**: output identical to the baseline line for line |

Twelve of the sixteen live mutants are caught by a physics row. Mk and Mo are shape or tuning choices that no physics row could pin; the golden is the right guard for them, and they are correct by reading against ARCHITECTURE. Ma and Ml are gate gaps: see Issues 1 and 2.

## Human Verification

None for this phase: there is no audio path yet. The three Stage 1 hands-on items remain open and non-blocking.

Execute skipped its code-review checkpoint (`--skip-review`). This session covers it for the designer: `Polish.h` and `PolishConstants.h` were read in full, and the 2.1 → 2.1b diff of the other three headers line by line. The 5173-line harness was not read; the mutation run and the controls corroborate its scoring.

## Issues Found

1. **The step limit is guarded by the golden only.** With the limit compiled out (Ma), every gated row passes: C1 worst is 3.457x (limit 4), with 8 lines above 2x where the shipped design has 1, and the shrink ratio is 0.353; NW2 is +0.783 / +0.318 dB; RA1 is 0. RESEARCH-2.1b-continuity measured 7 jumps with the re-centre alone, but on the bench's 5340 lines; the harness's 1101 lines do not reproduce them (plan risk R1, inverted). The shipped design is not wrong: the limit is in the code, D2 pins it, and the constants grep pins its value. But the ARCHITECTURE claim that "either one alone leaves jumps" has no harness row behind it. Fix in a later harness pass: add the bench's jumping lines to C1 as fixed sweeps, or add a `--neg no-step-limit` control that must fail C1.
2. **The accept weight's improvement factor is never active.** Over both goldens, 1101 + 216 C1 lines and 5.2 million NW2 sections, `smoothStep ((1 − S1/S0) / 0.1)` is 1 wherever the first factor is above 0: Ml's output is identical to the baseline. The factor is a safety net that no input exercises, so no row can show that it works. Not a defect, since it can only lower a weight. A later harness pass could report, as INFO, the count of sections where it is below 1, plus one constructed case that drives it.
3. **The extension blend switches on a failed step** (`Polish.h:413`, `if (ok)`). This path is unreachable in exact arithmetic, because `A = JᵀJ + λ diag + μI` with μ = 0.1 is positive definite; only a non-finite intermediate reaches it, and the result is then still a valid, accepted-or-seed design. Informational.
4. **For Phase 2.2.**
   - `Designer::prepare` must run before the first `design` at every rate change. A Designer that was never prepared polishes on a 48 kHz grid with `polishEnabled = true` (Mn shows what a stale grid costs: G1b fails at four rates).
   - `design()` still does not sanitise its inputs (VERIFICATION-2.1 issue 6).
   - Nothing is polished below 44.1 kHz.
5. **Open INFO items, unchanged:**
   - The I-odd WARN: 18 dB/oct, 10–20 k, ±12 dB reads 0.151 against a ceiling of 0.15.
   - The I-weak grid difference: the Q 20–40 ±6 dB cell reads 0.94 against the bench's 0.75.

VERIFICATION-2.1 issues 2–5 were closed in this phase:
- G2s now uses literal nominal slopes.
- N1's limit is 1e-9.
- P1 checks the notch and band-pass clamps from both sides.
- A CS row adds cut-skirt regression ceilings.

None of the issues above blocks this phase.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.1b)

**Ready for next phase:** Yes. Phase 2.2 (static engine, placement, transparency) has no research or plan yet; next is research for 2.2.

**Blockers (if any):** none. Carried forward, non-blocking:
- a harness row that fails without the step limit (Issue 1);
- an INFO count for the accept improvement factor (Issue 2);
- for 2.2: `prepare` before `design`, sanitised inputs, and no polish below 44.1 kHz.
