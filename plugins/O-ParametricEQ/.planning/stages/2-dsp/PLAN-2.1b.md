---
title: "O-ParametricEQ Stage 2 (DSP) — Phase 2.1b Execution Plan"
created: 2026-11-13
juce_version: "8.0.15"
phase: "2.1b"
summary: "Five waves, serial: (W0) pre-flight with Phase 2.1 green, (W1) Python reference of the polish promoted to tests/model and the polish golden, (W2) eq::Polish + constants header + Designer wiring behind the 2.1 interface, (W3) harness rows for the tightened G1, never-worse, continuity, robustness, cost; build; negative controls; (W4) SUMMARY, REQUIREMENTS, STATUS, path-scoped commit. No processor code, no plugin binary change, no install, no tag."
stages: [2]
agents: [dsp-agent, build-gate-runner]
---

# O-ParametricEQ — Stage 2 / Phase 2.1b Execution Plan (Designer polish)

## Goal

Every second-order gain section (Bell, and the MLR sections of Low Shelf, High Shelf, Tilt) is refined by the anchored Gauss-Newton polish of ARCHITECTURE.md "Accuracy polish (v1, Phase 2.1b)", behind the unchanged `eq::Designer` interface. Gate G1 passes at the Phase 2.1b thresholds (`0.05 + 0.012 |g|` at 44.1 / 48 kHz, `0.03 + 0.002 |g|` at 88.2 kHz and above) and both the RBJ build and the closed-form seed fail it. Per section the polish is never worse than the seed, the map is continuous, and it is allocation-free, bounded and deterministic.

Out of scope: processor code, the audio engine, the correction hold (documented fallback only), polishing the first-order shelf section, a better seed for the one stubborn weak spot.

**Precondition:** Phase 2.1 executed and verified (`SUMMARY-2.1.md` exists, STATUS says so). If 2.1 ended in the P8 escalation, this plan does not run.

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/2-dsp/CONTEXT.md` | D7a–D7c. Binding. |
| `research/ARCHITECTURE.md` § "Accuracy polish (v1, Phase 2.1b)" | The algorithm and its constants. The contract. |
| `stages/2-dsp/RESEARCH-2.1.md` | Q3 (Jacobian, grid, iteration, evidence), Q4 (blends, continuity test), Q5 (inverse), Q6 (cost), Q7, Q8 (shelf tables), Q10 (rows), Pitfalls 1–9. |
| `stages/2-dsp/research-2.1/cand.py`, `polish.py` | **The recommended algorithm** in NumPy. Source for `tests/model/polish_model.py`. |
| `stages/2-dsp/research-2.1/polish_bench.cpp` | A working scalar C++ port (timed; dump agreed with `cand.py`). Start here for `Polish.h`. |
| `stages/2-dsp/PLAN-2.1.md` § Interface, `SUMMARY-2.1.md` | What exists. |
| `Source/dsp/*.h`, `tests/designer-harness/main.cpp`, `tests/model/eqmodel.py` | The 2.1 code this phase extends. |

---

## Plan-phase decisions (resolve RESEARCH "Open questions" and "Calls for the plan")

- **P1 G1 tolerance for 2.1b is the research proposal, adopted as is** (RESEARCH Q10): bell Q 0.025–40, shelf 12 dB/oct Q ≤ 1.41, shelf 24–96 dB/oct even orders Q 1.0: `0.05 + 0.012 |g|` at 44.1 / 48 kHz, `0.03 + 0.002 |g|` at ≥ 88.2 kHz. Tighter than D7a's 0.15 / 0.30 / 0.70 at every gain (0.12 / 0.19 / 0.41) with at least 1.6x margin over the model. ARCHITECTURE G1 is amended accordingly.
- **P2 The polish runs on every control tick** (RESEARCH Q6 option 1). The steep-shelf sweep case (model: 46 % of a core for 24 × 96 dB/oct shelves all sweeping) becomes a G10 INFO row and is read in the Phase 2.2 smoke measurement. The correction hold is the documented fallback and is not built now. PERF-02's rows are cuts and are unaffected.
- **P3 The first-order shelf section stays unpolished** (D7b names the second-order sections). Odd-order shelves (18 dB/oct) are INFO rows with the ceiling 0.10 / 0.15 / 0.30.
- **P4 The stubborn weak spot is reported, not gated** (12 dB/oct, Q 2–10, ±30 dB, corner above 10 kHz at 44.1 kHz; D7c). v1.x.
- **P5 The Designer polishes by default; the seed stays reachable.** `Designer::design (settings, out)` polishes. `Designer::designSeed (settings, out)` is the 2.1 behaviour, kept for the harness baseline, the never-worse row and the seed negative control. Every Phase 2.1 row keeps running against `designSeed` at the 2.1 thresholds.
- **P6 Where the accept weight is exactly 0 the output is the seed's `SectionParams` object itself, not `exp (log (seed))`.** The skip test (`S0 / 26 ≤ Sa²`) runs after one residual pass and before any Jacobian; it is exactly equivalent to the full formula (the first accept factor is 0 there) and is where most of the saving is.
- **P7 `mL` and `cL` are copied from the seed, never exponentiated.** DC gain stays exact.
- **P8 Polish on the boost side, invert after** (2.1's P3 convention, RESEARCH Q5). The polish sees only boost-side sections; `invert` already exists.
- **P9 The polish target is `BandDesign::trueProto`** (the true analog section, also for a re-centred bell).
- **P10 Constants live in one header, `Source/dsp/PolishConstants.h`,** and nowhere else: M 16, tied scales 0.5 / 1.5, K 2 (+2), λ 1e-3, μ 0.1, `Sa` 0.002, `Sb` 0.006, `Sx` 0.03, accept ramp 0.1, band 20 Hz – `min (20 kHz, 0.499 fs)`. The harness prints them; SUMMARY records them. Changing one invalidates every acceptance number, so a constant change is an escalation, not a tuning step.
- **P11 Research-untested cases are measured here and are not all binding.** Tilt: binding by identity (its error equals the high shelf's, row TL2). 36 and 72 dB/oct shelves: inside the binding even-order row; if they alone fail, STOP and report rather than loosen (R2). 96 dB/oct with user Q above 4: INFO, covered by the finite row.
- **P12 The polish golden's tolerance is looser than the seed's** (1e-6, against 1e-9): a 6×6 solve with a condition number up to about 1e7, LAPACK in the model against Cholesky in C++.

---

## Interface additions

```cpp
namespace eq
{
struct PolishGrid { void prepare (double fs) noexcept; /* 16 fixed tan² values, band edges */ };
struct PolishInfo { double weight; int steps; };          // accept weight a in [0, 1]; steps 0, 2 or 4

SectionParams polishSection (const PolishGrid&, const AnalogSection& trueProto,
                             const SectionParams& seed, PolishInfo* info = nullptr) noexcept;

class Designer
{
    void prepare (double fs) noexcept;                                   // now also prepares the grid
    void design     (const BandSettings&, BandDesign&) const noexcept;   // polished
    void designSeed (const BandSettings&, BandDesign&) const noexcept;   // Phase 2.1 closed form
};
}
```

`BandDesign` gains `std::array<float, maxSections> polishWeight` (0 for unpolished classes) for the histogram and, later, a debug overlay. Nothing else in the 2.1 interface changes.

---

## Tasks

Roles, `$S`, BG and path conventions as in `PLAN-2.1.md`.

### Wave 0 — pre-flight

**T1. Pre-flight** [run]
- Done when (any mismatch → STOP):
  - Contract hashes equal STATUS `contract_checksums`.
  - `git branch --show-current` = `main`; `git status --short plugins/O-ParametricEQ` empty.
  - Phase 2.1 baseline: `O-ParametricEQ-designer-harness` → `0 FAIL` (record the tally `N`); render harness `36 PASS, 0 FAIL`.
  - `test ! -e plugins/O-ParametricEQ/Source/dsp/Polish.h`.

### Wave 1 — reference model and golden

**T2. `tests/model/polish_model.py`, `tests/model/dump_polish.py`** [agent]
- `polish_model.py`: `cand.polish` and `polish.resp_jac` promoted, operating on `tests/model/eqmodel.py`. Keep it the algorithm of ARCHITECTURE exactly (grid, steps, extension, accept, non-finite → weight 0). Add `design_band_polished (…)` = seed band, polish each `gain2` section on the boost side against the true prototype, invert for cuts.
- `dump_polish.py`: the `dump_designs.py` settings list restricted to Bell, Low Shelf, High Shelf, Tilt, at 44.1 / 48 / 96 kHz, same column format plus a `weight` column. Stdout, `"\n"`, `%.17g`.
- Depends on: T1.

**T3. Generate and sanity-check** [run]
- `python3 …/dump_polish.py > plugins/O-ParametricEQ/tests/golden/designs-polished.tsv`.
- Model self-check in `$S`: bells on the G1 grid at 44.1 kHz reproduce RESEARCH Q7's polished row (0.043 / 0.081 / 0.193) within 0.01 dB, and 12 dB/oct shelves Q ≤ 1.41, fc 10–20 k reproduce Q8's 0.04 / 0.12 / 0.19 within 0.02 dB.
- Done when: both pass; no `\r`, `nan`, `inf`; a second run is byte-identical.

### Wave 2 — the polish

**T4. `Source/dsp/PolishConstants.h` + `Source/dsp/Polish.h`** [agent]
- Same header rules as 2.1 (std only, `noexcept`, no allocation, fixed-size arrays; 26-point and 6×6 work arrays on the stack).
- `PolishGrid::prepare`: 16 log-spaced frequencies 20 Hz → `min (20 kHz, 0.499 fs)`, stored as `tan² (w/2)`.
- `polishSection`, in this order:
  1. 10 tied points from `(wz, Qz)` and `(wp, Qp)` of the true prototype (centre; `w0 (sqrt (1 + h²) ± h)`, `h = s / (2Q)`, `s` 0.5 and 1.5), clamped into the band; targets `ln |Ha|²` at all 26 points.
  2. Residual pass at the seed → `S0`. If `S0 / 26 ≤ Sa²` return the seed (P6).
  3. Two anchored steps with the closed-form Jacobian of RESEARCH Q3 (`A = JᵀJ + (λ diag (JᵀJ) + μ) I`, `b = Jᵀr + μ (φ − φ0)`, 6×6 Cholesky; a non-positive pivot or non-finite value → weight 0, return the seed).
  4. `wx = sstep ((rms − Sx) / Sx)`; if `wx > 0`, two more steps and blend `φ2 + wx (φ4 − φ2)` (a non-finite `φ4` leaves `φ2`).
  5. `S1`, accept weight `a`; `a == 0` → the seed object; else `exp (φ0 + a (φ − φ0))` for the six free parameters, `mL` and `cL` from the seed (P7).
- No `if` on type, Q, gain, frequency or sample rate anywhere in the numeric path beyond the three weights above (RESEARCH pitfall 3).
- Depends on: T3 (ordering only).

**T5. Wire into `Source/dsp/Designer.h`** [agent]
- `prepare` prepares the grid. `design` = `designSeed` on the boost side, then `polishSection` for each `gain2` section against `trueProto`, then the tilt scalar fold and the inversion as in 2.1. `designSeed` is byte-for-byte the 2.1 result (row D1 keeps proving it).
- The tilt scalar is folded **after** the polish, so the polish sees the plain high-shelf section.
- Depends on: T4.

### Wave 3 — harness, build, gates

**T6. Extend `tests/designer-harness/main.cpp`** [agent]
- Every Phase 2.1 row now calls `designSeed` and is otherwise unchanged. New rows (polished unless stated; G1 grid and evaluation grid as in 2.1, shelf Q points extended to 1.41):

  | Row | Check | Threshold |
  |---|---|---|
  | D2 | C++ `design` vs `designs-polished.tsv` | six free parameters within 1e-6 relative; `mL`, `cL` within 1e-9; weight within 1e-6; and the band response within 1e-6 dB on the evaluation grid (P12) |
  | G1b-bell | Bells, Q 0.025–40, five rates | `0.05 + 0.012 \|g\|` at 44.1 / 48 k; `0.03 + 0.002 \|g\|` at ≥ 88.2 k |
  | G1b-sh12 | 12 dB/oct low and high shelf, Q ≤ 1.41, five rates | same |
  | G1b-shN | 24, 36, 48, 72, 96 dB/oct low and high shelf, Q 1.0, five rates | same (P11) |
  | G1b-sh6 | 6 dB/oct shelf | unchanged from 2.1 |
  | G1bn-seed | `designSeed` scored by the G1b-bell code at 44.1 kHz | must FAIL (model 0.34 / 0.61 / 0.98 against 0.12 / 0.19 / 0.41) |
  | G1bn-rbj | The RBJ bell scored by the G1b-bell code | must FAIL |
  | NW1 | Per section, G1 grid, five rates, bells and every `gain2` section of the shelf rows: polished max error vs seed max error against that section's true prototype | polished ≤ seed + 1e-3 dB; count of violations = 0 (model: 0) |
  | R2 | Reciprocity as R1, polished: bells; shelves and tilt at eight slopes, Q 0.3 / 1 / 4 | ≤ 1e-9 dB |
  | C1 | Continuity. 40 seeded random lines through (log fc 10 Hz – 30 kHz, gain −60 … +60, log Q 0.025 – 40) at 44.1 and 48 kHz, for Bell, 12 dB/oct shelf, 48 dB/oct shelf, Tilt; 8000 and 32 000 steps; largest response change between neighbouring steps on the evaluation grid, seed and polished | polished ≤ 2x the seed's on every line at both step counts, and the polished 32 000-step figure ≤ 0.5x its 8000-step figure. Plus RESEARCH Q4's 21 one-dimensional sweeps as fixed lines |
  | C1n | The same test with the extension weight `wx` replaced by a binary gate (harness-local variant through a test hook) | must FAIL on at least one line (model: 0.06–0.13 dB steps that do not shrink) |
  | F2 | F1's 1e6-point sweep with `design`, gain to ±60, eight rates 22.05 – 192 kHz, range ends included | finite; `g, k, G > 0`; `mL … cH > 0` for `gain2`; `weight ∈ [0, 1]` |
  | S1 | Skip exactness: for 10 000 settings where the weight is 0, `design` and `designSeed` | `memcmp` equal; at least 1000 such settings found or the row is void |
  | TL2 | Tilt error equals the high-shelf error for the same settings (12 and 48 dB/oct, G1 grid) | within 1e-9 dB (P11) |
  | A2, B2 | A1 and B1 with `design` | 0 allocations; `memcmp` equal |
  | H1 (INFO) | Share polished (`weight ≥ 0.999`) / blended / seed (`weight = 0`) per zone: bells and shelves, fc ≤ 10 k and 10–20 k, ±6 / ±12 / ±30, five rates | printed; compare with RESEARCH Q4 (bells 44.1 k: polished 25 → 44 %, blended 7 %, seed 68 → 49 %) |
  | I-odd (INFO) | 18 dB/oct shelf, Q 1 | printed against the ceiling 0.10 / 0.15 / 0.30; WARN line if above |
  | I-weak (INFO) | RESEARCH Q8's weak rows at 44.1 and 48 kHz (12 dB/oct Q 2–10, Q 20–40; 48 dB/oct Q 0.25–4): seed / polished | printed beside the Q8 values; the 4.25 dB cell named (P4) |
  | I-new (INFO) | Tilt as a type (all slopes, Q 1); 96 dB/oct shelves with Q 4–40; bells 20–30 kHz at 44.1 / 48 k (model 0.40 / 0.17) ; 22.05 and 32 kHz bells vs seed | printed |
  | I-cost (INFO) | ns per section: seed, polished, and polished on a set where every section skips; 400 000 random bells and 100 000 random 96 dB/oct shelves | printed; model 35 ns / 0.8 µs. WARN above 2 µs |

- `--neg` gains `seed`, `rbj-b`, `binary-gate`.
- The binary-gate hook is a template or constexpr parameter visible only to the harness TU; the shipped `polishSection` has no runtime switch.
- Depends on: T5.

**T7. Build, run, negative controls** [run]
- No CMake change is expected (same target, new headers). If the agent touched `CMakeLists.txt`, reconfigure BG as in 2.1 T8.2.
- Done when, in order:
  1. (BG) `ninja -C build O-ParametricEQ-designer-harness O-ParametricEQ-render-harness > $S/build.log 2>&1` → rc 0, no warnings in this plugin's files.
  2. `$H > $S/designer.log 2>&1` (BG if long; C1 and F2 dominate) → `0 FAIL`; the PASS count is `N` (T1) plus the new binding rows; every row ID appears.
  3. `$H --neg seed`, `--neg rbj-b`, `--neg binary-gate`, and the 2.1 controls (`rbj`, `f1`, `alloc`) → rc 0 each.
  4. Golden negative control: one free parameter of one line of a copy of `designs-polished.tsv` changed in the 4th significant figure → D2 FAIL only.
  5. Constants: `grep -rnE "1e-3|0\.006|0\.002|0\.03\b" plugins/O-ParametricEQ/Source/dsp --include=*.h` hits `PolishConstants.h` only.
  6. Static gates of 2.1 T8.8 re-run (std-only, no FP flags, no processor wiring, processor files unchanged).
  7. Render harness `36 PASS, 0 FAIL`; `check_spec.py` → `OK:`.
- **On a binding failure:** D2 red → port bug, fix. D2 green and G1b / NW1 / C1 red → the model and research disagree: STOP and escalate with the cells; do not change a constant (P10) or a threshold.
- Depends on: T6.

### Wave 4 — bookkeeping + commit

**T8. SUMMARY, REQUIREMENTS, STATUS, commit** [agent writes SUMMARY; run does the rest]
- `.planning/stages/2-dsp/SUMMARY-2.1b.md`: tally; G1b worst / tolerance per row and rate; NW1 count; C1 worst ratios; the H1 histogram (this is D7c's "which regions are polished and which fall back"); every INFO block verbatim, with the research-untested cases (I-new) called out as new evidence; I-cost figures; the constants; negative-control results; deviations.
- Carry-forward notes for 2.2: polish every tick (P2) and the smoke case 24 × 96 dB/oct shelves sweeping; bit-identity harness TU vs processor TU; `polishWeight` available per section.
- `REQUIREMENTS.md`: DSP-01 stays `pending` until verify; no edit here.
- STATUS: phase rows, `next_action: stage_2_verify_phase_2_1b`, files list.
- Commit path-scoped as in 2.1 T9 (`git add` the new files: `Source/dsp/Polish.h`, `PolishConstants.h`, `tests/model/polish_model.py`, `dump_polish.py`, `tests/golden/designs-polished.tsv`, `SUMMARY-2.1b.md`; then `git commit -- plugins/O-ParametricEQ`). No tag.
- Depends on: T7.

---

## Dependency graph

```
T1 → T2 → T3 → T4 → T5 → T6 → T7 (build, gates, negative controls) → T8
                               ↑   │
                               └───┘ gate FAIL → fix, rebuild, rerun T7.2–7.7 in full
```

## Success criteria

| Criterion (ROADMAP Phase 2.1b, CONTEXT D7) | Proven by |
|---|---|
| G1 at the 2.1b thresholds, five rates; RBJ and the seed both fail (DSP-01, D7a) | G1b-bell, G1b-sh12, G1b-shN, G1b-sh6; G1bn-seed, G1bn-rbj |
| Coverage: bells and second-order shelf / tilt sections, weak spots included (D7b) | G1b rows, TL2, I-weak, I-new |
| Hybrid, never worse; regions reported (D7c) | NW1, S1, H1 |
| Reciprocity within 1e-9 dB with the polish on | R2 |
| Continuous under parameter sweeps (CONTEXT question 4) | C1, C1n |
| Finite and stable over the full ranges to ±60 dB, 22.05 – 192 kHz (DSP-06) | F2 |
| Allocation-free, bounded, deterministic, same on two threads | A2, B2, fixed step counts in T4 |
| The C++ is the researched algorithm | D2 + golden negative control; T3 self-check |
| One constants header | T7.5 |
| Every Phase 2.1 row still passes with the seed designer | T7.2 tally |
| Cost known before 2.2 | I-cost |
| No processor code; Stage 1 rows green | T7.6, T7.7 |

## Risks

- **R1 C1 is new evidence.** Research tested 21 one-dimensional sweeps at 44.1 kHz; random lines at two rates may find a kink (the tied-point clamp at 20 kHz, the re-centre boundary). A continuous kink passes the shrink test; a jump does not. A jump is a research finding: STOP and escalate.
- **R2 36 / 72 dB/oct shelves and Tilt were not modelled.** They sit inside binding rows by P11. Expected between the 24 and 96 dB/oct figures (0.06 / 0.12 / 0.23 against 0.12 / 0.19 / 0.41).
- **R3 A shelf cascade can read up to about 0.01 dB worse than its seed** while every section is no worse (RESEARCH Q8). NW1 is therefore per section. Do not add a per-band never-worse row.
- **R4 D2 tolerance.** If agreement is worse than 1e-6 only where the weight is near 0 or `wx` is mid-ramp, the two solvers differ in a nearly unobserved direction: report the worst cell and its condition number; do not widen silently.
- **R5 Harness runtime.** C1 is 40 lines × 4 types × 2 rates × 40 000 designs × an evaluation grid: score C1 on a reduced grid (64 log points plus tied points), not the 1000-point G1 grid.
- **R6 Cost on this machine differs from the research port.** I-cost is INFO; the decision it feeds (P2 against the correction hold) is taken at the 2.2 smoke reading, not here.
