# Stage 2: DSP — Phase 2.1 (Section designer and offline gate) - Verification

## Verification Date

2026-11-13 (verify session; independent re-run against `998bbd37`, forced recompile of the designer harness, a scorer written in this session, a 15-mutant run, and a line-by-line read of the four `Source/dsp` headers)

Scope: Phase 2.1 only. Stage 2 has six more phases (2.1b – 2.6); this file does not close the stage.

## Goal-Backward Analysis

### Original Goals (from PLAN-2.1.md, ROADMAP Phase 2.1, CONTEXT D6–D11)

1. The closed-form designer exists as pure headers: eight types, eight slopes, Nyquist policy, boost / cut by inversion; interface "band settings in, up to eight section parameter sets out", true analog section exposed per section.
2. Gate G1 passes at the Stage 0 tolerance at 44.1, 48, 88.2, 96, 192 kHz; the RBJ control fails it (DSP-01, design side).
3. The ARCHITECTURE "Model results" table is reproduced within 0.05 dB.
4. Reciprocity within 1e-9 dB; the 10 Hz, Q 40 bell at 192 kHz within 1e-6 dB.
5. Every design finite with `g, k, G > 0` over a 1e6-point sweep at 22.05 – 192 kHz (DSP-06, design side).
6. G2 static part: corner gains and stop-band slopes (DSP-03, design side).
7. Notch / Band Pass ceilings hold below the clamps; above-clamp figures reported.
8. The designer agrees with the Python reference model on a committed golden.
9. D6 slope inert on Bell / Notch / Band Pass; D8 tilt ±g/2; D9 Q 1 = Butterworth and Q inert at 6 dB/oct; D10 frequency policy.
10. Usable on the audio thread: allocation-free, deterministic, thread-independent.
11. No processor code, plugin binary unchanged, Stage 1 still green, contracts untouched.

### Deliverables (from SUMMARY-2.1.md, confirmed in the tree)

1. `Source/dsp/EqTypes.h`, `ResponseEval.h`, `Prototype.h`, `Designer.h` (1181 lines, std-only C++17).
2. `tests/designer-harness/main.cpp` (28 rows + 6 INFO) and its CMake target.
3. `tests/model/eqmodel.py`, `dump_designs.py`, `README.md`; `tests/golden/designs.tsv` (162,855 rows).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Designer and interface | ✅ Achieved | Headers read in full in this session: the interface matches PLAN "Interface" field for field; `MatchedPair` has both branches, the `expm1` forms, the `th` clamp and the `P1` floor; P3 inversion, D8 scalar fold and `trueProto` are as planned. No defect found. |
| 2 G1 | ✅ Achieved | Harness G1-bell / G1-sh12 / G1-sh6 pass at five rates; G1n fails RBJ at 17.39 dB. **Independent scorer** (time-domain SVF + one-pole written here, analog targets written from the signed gain, 44.1 and 48 kHz): shelves reproduce the harness's worst cells to three decimals (12 dB/oct 0.367 / 0.687 / 1.188, 6 dB/oct 0.173 / 0.312 / 0.490); bells 0.284 / 0.514 / 0.874 on a coarser grid (harness 0.338 / 0.612 / 0.975). Simulation and `ResponseEval` agree within 1.3e-8 dB. |
| 3 Model table | ✅ Achieved | T1: 0 of 33 cells off. |
| 4 Reciprocity, precision | ✅ Achieved | R1 4.62e-13 dB; N1 2.95e-13 dB. Scorer: tilt(+g) + tilt(−g) = 1.5e-14 dB. See Issues 3 for N1's limit. |
| 5 Finite sweep | ✅ Achieved | F1: 8,008,192 designs, 0 bad; F1n fires. |
| 6 G2 static | ✅ Achieved | G2s corner rows pass (144 and 126 cells); slope row passes on 133 of 144 cells. The 11 skipped High Cut cells were measured here another way: see Issues 1. |
| 7 Notch / Band Pass | ✅ Achieved | NB1–NB4 pass with the model's figures; INFO block present. |
| 8 Golden | ✅ Achieved | D1 bit-exact on 1,176,175 cells. Re-derived here: the model regenerated the golden and the C++ `--dump-designs` (twice) all give SHA-1 `98175688…fd0327`, equal to the committed file. |
| 9 D6 / D8 / D9 / D10 | ✅ Achieved | SL1, TL1 ×3, Q1, G2s corner rows, P1 ×2. Scorer: tilt at eight slopes within 0.071 dB of a from-scratch prototype at 96 kHz, DC = −g/2. |
| 10 Audio-thread fitness | ✅ Achieved | A1 0 / 0 with the planted allocation counted; B1 10,000 / 10,000; no statics or heap types in the headers (read). |
| 11 No processor change | ✅ Achieved | Processor files identical to the Stage 1 commit; no `dsp/` include in them; render harness 36 / 0; `check_spec.py` OK; four contract checksums equal STATUS. |

## Requirements Verification

**Stage:** stage-2 (Phase 2.1 of 7)
**Requirements touched by this phase:** 3 (all must), none closed

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-01: analog-matched bell and shelf | must | ⏸️ Pending | Design side met at the Stage 0 tolerance. Closes at 2.1b verify (tightened G1) and needs the rendered path (2.2) for "latency stays 0". |
| DSP-03: slopes | must | ⏸️ Pending | Static half met (Issues 1). "Slope changes are click-free" is Phase 2.4. |
| DSP-06: parameter ranges | must | ⏸️ Pending | Design side met (F1, P1). Reachability and above-Nyquist rendering are 2.2 / Stage 3. |

**Requirements Summary:**
- ✅ Complete: 0 new (COMPAT-01 from Stage 1)
- ⚠️ Partial: 1 (DSP-07, unchanged; engine half is Phase 2.2)
- ⏸️ Pending / later phase: 25
- ❌ Failed: 0

`REQUIREMENTS.md` is unchanged, as PLAN-2.1 T9 specifies.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | brief, parameter-spec, ARCHITECTURE, ROADMAP equal STATUS |
| Build (harness object deleted, recompiled) | ✅ Pass | rc 0, no warning in this plugin's paths (`-Wall -Wextra -Wconversion -Wshadow -Wsign-conversion -Wfloat-equal`) |
| Designer harness | ✅ Pass | `28 PASS, 0 FAIL, 6 INFO`; output identical to the SUMMARY block line for line, timing row aside (48.9 ns this run, machine shared with other builds) |
| Determinism / golden | ✅ Pass | Three SHA-1s equal (see goal 8); 162,857 lines |
| Negative controls | ✅ Pass | `--neg rbj`, `f1`, `alloc` rc 0; unknown name rc 255; one golden line deleted → rc 1, D1 only |
| Stage 1 render harness, `check_spec.py` | ✅ Pass | 36 / 0; `OK: 196 params` |
| Static gates | ✅ Pass | Four greps / diffs empty |
| Commit hygiene | ✅ Pass | `998bbd37` lists only this plugin's paths; no tag on HEAD |
| Independent scorer | ✅ Pass | 0 FAIL (after fixing its own impulse length for overdamped cuts) |

Not run, by design: plugin build, install, pluginval, auval. The plugin binary does not include `Source/dsp/` and is still 0.1.0.

### Mutation run (new in verify)

Fifteen mutated copies of `Source/dsp` in the session scratchpad, each compiled with the unmodified harness. Nothing in the repo or in `build/` was changed. D1 (the golden) catches all fifteen; the column that matters is what catches a mutant **besides** D1, because D1 only proves the C++ equals the model.

| Mutation | Rows failing besides D1 |
|---|---|
| M0 unmodified control | none (28 / 0) |
| M2 `invert` keeps `g` | G1-bell, G1-sh12, R1, T1, TL1 |
| M3 user Q on every section, not `Q0` only | G2s |
| M4 tilt scalar `10^(−g/20)` | TL1 ×3 |
| M5 pole-angle clamp at π removed | T1 |
| M7 bell re-centre at 0.90 π | G1-bell, T1 |
| M9 corrector `cH` without the Nyquist match | G1-bell, G1-sh12, NB1, NB2, T1 |
| M11 band-pass clamp at 0.95 π | NB4, P1 |
| M15 re-centre `Ae = Ge` | P1, T1 |
| M1 low-pass match point 0.70 π | **none** |
| M6 first-order shelf match 0.8 | **none** |
| M8 notch clamp lowered to 0.90 π | **none** |
| M10 `Om` as `1 − exp` | **none** |
| M12 slope table `…, 12, 15` (96 dB/oct becomes 90) | **none** |
| M13 matched high-pass `mH = 1` | **none** |
| M14 `P0` from `exp − 1` | **none** |

Eight of fifteen are caught by a physics row; seven only by the golden. For those seven the code was checked by reading against ARCHITECTURE (lines 361, 374, 381, 411; `parameter-spec.md` "Order N = slope / 6") and is correct. The gaps are in the gates, not in the designer: Issues 2–5.

## Human Verification

None for this phase: no audio path exists yet. The three Stage 1 hands-on items remain open and non-blocking.

The quality-gate code-review checkpoint skipped at execute (`--skip-review`) is covered here for the designer: the four headers were read line by line. The 2217-line harness was not; its scoring is corroborated by the independent scorer and the mutation run.

## Issues Found

1. **G2s skips the High Cut slope at 6 dB/oct (9 cells) and two 12 dB/oct cells.** The −50 dB point of a first-order low-pass at fc ≥ 100 Hz lies above 20 kHz, so the planned window cannot exist. Measured here instead: at 6 dB/oct the slope from the −20 dB point to 20 kHz is measurable on 7 of 9 cells and is within 0.43 dB/oct of the analog prototype over the same span (6.42 against 5.99); the last two cells (fc 1.4 and 2 kHz) have under an octave below −20 dB. All nine are within 0.97 dB of the analog curve above −60 dB. At 12 dB/oct all nine cells measure within 0.64 dB/oct on the shortened span. DSP-03's static half holds. The upward bias is the Nyquist match steepening the last octave, an ARCHITECTURE weak spot already listed.
2. **G2s takes its nominal slope from `eq::slopeOrder`, the table under test** (`main.cpp:1501`). M12 passes it. The table is right by reading; the row should use literal 6, 12, 18, 24, 36, 48, 72, 96.
3. **N1's 1e-6 dB limit does not separate the `expm1` form from the naive one.** The row's own INFO line shows the naive `MatchedPair` at 4.83e-07 dB, inside the limit; M10 and M14 pass. The ROADMAP criterion as written is met (2.95e-13 dB). A limit of 1e-9 would give the row teeth.
4. **Cut accuracy against the prototype is INFO only**, so the matched high-pass and the low-pass match point are held by the golden alone. Measured here at 44.1 kHz, fc 2 – 20 kHz, above −60 dB: Low Cut 12 / 24 / 96 dB/oct 1.35 / 0.82 / 0.01 dB; with M13 4.19 / 3.26 / 0.37 dB. A regression ceiling on these would catch it.
5. **P1's notch clamp row is one-sided:** a clamp moved down passes (M8), because the reference design is built from the same constant and NB2's region shrinks with it. The first-order shelf match constant (M6) is likewise golden-only; the 6 dB/oct shelf stays inside its tolerance at 0.8.
6. **`design()` does not guard its inputs** (frequency, Q or sample rate of 0, non-finite values). By design: PLAN-2.1 leaves sanitising to the control layer (Phase 2.3 "target sanitising"). Phase 2.2 must not call it with unsanitised values.

None of these blocks. Items 2–5 are harness rows; Phase 2.1b edits the harness anyway ("every Phase 2.1 row still passes") and is the natural place to add them.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 2.1)

**Ready for next phase:** Yes — execute Phase 2.1b (`PLAN-2.1b.md`).

**Blockers (if any):** none. Carried to 2.1b execute, non-blocking: harness hardening per Issues 2–5 (literal nominal slopes; N1 limit 1e-9; cut-skirt ceilings; a two-sided notch clamp check), and a slope measurement for the cells G2s skips (Issues 1).
