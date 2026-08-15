# Stage 3 — GUI — Verification

**Plugin:** O-Octagon
**Stage:** 3 of 4 — GUI
**Phases:** 3.1 (Room screen shell + plan) · 3.2 (Venue screen, venue store, verify-ping) ·
3.3 (Scenes, meters, gradient, elevation)
**GSD phase:** verify
**Date:** 2026-08-12
**Branch:** `feat/o-octagon`

> This is the **stage-level** report. Each phase carries its own: `VERIFICATION-3.1.md`,
> `VERIFICATION-3.2.md`, `VERIFICATION-3.3.md`. This document closes Stage 3 as a whole and does not
> restate their detail.

---

## Verdict

**✅ VERIFIED.** Ready for Stage 4 (Polish): **yes**, with no blockers.

**All nine requirements on Stage 3's traceability line close ✅ complete. Zero partial, zero
failed.** Every one of the 40 acceptance criteria across those nine rows is met with named,
re-measured evidence.

Stage 3 ran **one full discuss → research → plan → execute → verify cycle per phase**, inheriting
Stage 2's structure. It kept Stage 2's defining property and added one of its own:

- **Every partial was declared at a discuss boundary and closed at its named destination.** Not one
  was discovered at verify — three stages running.
- **Five requirement rows entered this stage with a summary-table line and NO acceptance criteria at
  all** (`FUNC-06`, `UI-02`, `UI-03`, `UI-04`, `UI-05`). They would have been "verified" against
  nothing. Criteria were derived at the discuss boundary of the phase that owned each, from
  `ARCHITECTURE.md` and `ROADMAP.md`, **before** any implementation existed to shape them.

| | 3.1 | 3.2 | 3.3 | Stage |
|---|---|---|---|---|
| C++ probes | 65 | 78 | **92** | 44 unit + 48 harness |
| Failures | 0 | 0 | **0** | — |
| JS gate sections | 30 | 49 | **69** | 42 static + 27 Playwright |
| Gates re-run at verify | 13 | 15 | **16** | all from scratch, forced full recompile |
| Negative controls at verify | 2 | 3 | **3** | **8 total, all fired** |
| Requirements closed | 1 ✅ | 4 ✅ | **4 ✅** | **9 ✅ 0 ⚠️ 0 ❌** |

---

## Contract checksums at the stage close

| Contract | SHA-256 | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…` | ✅ **unchanged across all three phases — and all six phases of Stages 2 and 3** |
| `parameter-spec.md` | `b45f88dc5017ec2c…` | ✅ **unchanged across all three phases.** 17 parameters; Stage 3 adds none |
| `research/ARCHITECTURE.md` | `32a850181b8ca95e…` | ✅ matches the 3.3-discuss re-pin |
| `ROADMAP.md` | `643471baa689b634…` | ✅ matches the 3.3-plan re-pin |

**Two pins moved in Stage 3, each at a phase boundary and never mid-phase**, and each because a
measurement disqualified what the contract said:

| Boundary | Contract | Change | Superseded |
|---|---|---|---|
| 3.3 discuss | `ARCHITECTURE.md` | Four §-level amendments: §4.1's tree becomes three-node (`SCENES`), §6.3's `SIDES` predicate becomes the measured one, §6.3's gesture mechanism gains the bracket obligation, and **§4.3's "~30 Hz on a Timer"** — found at that boundary and on nobody's list | `a8a358f4…` |
| 3.3 plan | `ROADMAP.md` | The gradient formula **`max_i v_i²` → `1/k = √denom`** (N10 measured the first as identically 1.0000 wherever one weight is non-zero), and the meter bullet's **same Timer error** | `aec7d0ce…` |

**The rule this stage leaves behind:** when an amendment corrects a claim, **grep the other
contracts for the same claim before closing.** §4.3's Timer phrase was corrected at 3.3 discuss;
nobody checked whether a second document said it too. `ROADMAP.md` did — **and so did the acceptance
criterion derived from it.**

---

## Goal-backward analysis

### What Stage 3 set out to achieve (`ROADMAP.md`, phases 3.1–3.3)

1. **A Room screen** — a top-down plan proportioned to the measured geometry, a draggable source
   puck, the convex hull drawn explicitly.
2. **A Venue screen** — 42 measured values typed by a user, a `.venue` file store, a label map, per
   speaker trims, and a verify-ping that confirms physical wiring in under a minute.
3. **Separate stores** — a musical preset can never reach venue geometry, trims or the label map.
4. **The spatial state made legible** — weight scenes, live per-speaker meters, the DBAP level field
   as the plan backdrop, and the height axis as a side elevation.

### What was delivered

| Goal | Phase | Status | The evidence that carries it |
|---|---|---|---|
| 1 — Room screen | 3.1 | ✅ | `UI-02` 7/7. The plan is proportioned by **one projection** (`metresToPx`, asserted by a gate that fires on a second one), and the metres readout is resolved against the **live** venue rather than a captured lambda |
| 2 — Venue screen + store + ping | 3.2 | ✅ | `FUNC-02`, `FUNC-04`, `UI-01`. 42 values through **one guarded apply path**; a `.venue` round-trip bit-identical into a **fresh** model; a ping with **seven EXACT zero** lanes on a **non-identity** map that self-stops at exactly 120 s |
| 3 — store separation | 3.2, re-measured 3.3 | ✅ | `FUNC-05`, then probe **CL** re-running the same 42-value bit-compare **after** the `SCENES` node existed — measured against the new tree shape rather than assumed inherited |
| 4 — the spatial state made legible | 3.3 | ✅ | `FUNC-06`, `UI-03`, `UI-04`, `UI-05`. Scenes as eight bracketed automation writes with **geometry-derived** membership; meters on the **written buffer**; a gradient matching the shipping solver to **5.5e-8**; an elevation strip whose front endpoint is bit-exactly immobile under `rakeRear` |

**Delivery matches the roadmap.** The two `nice`-priority rows §R7 kept on a descope path
(`UI-04`, `UI-05`) were **both shipped**, and both remain descopable — asserted structurally, not
promised.

---

## Requirements closed in Stage 3

| Requirement | Priority | Phase | Criteria |
|---|---|---|---|
| `UI-02` — Room screen plan | should | 3.1 | 7/7 |
| `FUNC-02` — Measured venue entry | must | 3.2 | 3/3 |
| `FUNC-04` — Verify-ping | must | 3.2 | 3/3 |
| `FUNC-05` — Preset separation | must | 3.2 | 3/3 |
| `UI-01` — Venue measurement screen | must | 3.2 | 3/3 |
| `FUNC-06` — Weight scenes | should | 3.3 | 6/6 |
| `UI-03` — Live per-speaker meters | should | 3.3 | 4/4 |
| `UI-04` — DBAP level-field gradient | nice | 3.3 | 4/4 |
| `UI-05` — Side-elevation strip | nice | 3.3 | 4/4 |

**9 rows · 40 criteria · 0 partial · 0 failed.** Combined with Stage 1 (2 rows) and Stage 2
(18 rows), **29 of the project's 30 requirement rows are complete**; `COMPAT-04` remains the only
row without a derived criteria section and is Stage 4's.

---

## What the stage found that no plan predicted

Four findings changed a decision or repaired shipped code. Each was a **measurement**, not an
argument.

1. **N8 (3.2) — `mapInvalid` is AUDIBLE.** Not "the last valid map is retained": `GainStage` takes
   its else arm and speaker 1 gets L while speakers **2–8 all get R at unity**. This made the label
   swap's transient non-benign and forced pre-commit validation in front of the single apply path.
   It then acquired a second consumer at 3.3 — the meters show that fold, **and that is correct**.
2. **N9 (3.3) — a dropped completion latches a guard forever, and it was LIVE in shipped 3.2 code.**
   A JUCE native completion is *dropped*, not rejected, when the browser is hidden, so neither
   `catch` nor `finally` runs and a flag cleared in a `finally` stays true for the life of the page.
   Measured on the shipping page. Repaired at 3.3: **every in-flight guard now releases on a
   deadline**, and `pollStatus` deliberately still has none.
3. **N10 (3.3) — `ROADMAP`'s own gradient formula was degenerate.** `max_i v_i²` is identically
   1.0000 wherever one weight is non-zero, because DBAP normalises to `Σv² = 1` — the picture goes
   blank exactly when the spatial situation is most extreme.
4. **The AO residual (3.3 verify) — a probe defect masquerading as an RT-safety risk.** Carried out
   of execute as *"unreproduced and unattributed"*; verify reproduced it **4 times in 40 runs under
   load** and attributed **4 of 4** to a thread other than the one calling `processBlock`. The
   allocation counter was process-wide. **Fixed in the harness; `PERF-01` never regressed.**

### And two the gates could not have found

Both were found **by looking at the rendered page** at 3.3: a field gradient whose alpha washed out
the hull polygon, and an elevation strip whose four mirrored speaker pairs stacked their numerals
illegibly. Neither is expressible as an assertion anyone had written. The fix for the second is the
stage in miniature — **the dots stay exact, because moving one would make the strip lie about a
depth; only the label steps aside.**

---

## The vacuity discipline, which is what this stage was actually about

Stage 3 caught the same class of defect — *an assertion that reports green because it is not
looking* — **five** times, in five different shapes:

| # | Shape | Caught at | Fix |
|---|---|---|---|
| 1 | A new JS module invisible to two gate enumerations | 3.2 plan | **P51** derives `PAGE_MODULES` from the directory; 3.3's four modules landed automatically |
| 2 | `railScrollHeight <= railClientHeight` passing over a **162 px** overflow | 3.2 execute, reproduced at 3.2 verify | the fitted-box-vs-stage `[guard]`, with the `[coarse]` one kept and relabelled |
| 3 | The same shape one level up — the **column** check going vacuous when anything is inserted after `#group-elevation` | 3.3 | §22 asserts the **ordering fact** §21 depends on. Reproduced at 3.3 verify with a **zero-height** node: §21 passed at 592 ≤ 592 at both DPRs while §22 fired |
| 4 | CB's 20 sample cells all landing **inside** the hull, so `hullTrimGain` multiplied by bit-exact unity and half the chain went untested | 3.3 execute — **by the probe's own non-vacuity clause** | 14 strided + 6 drawn from the 66 outside-hull cells |
| 5 | Frontend §18's **unanchored** per-class regex harvesting `.elev-readouts .cell-value` instead of `.cell-value` | 3.3 execute | selector anchored at a rule boundary |

The stage's answer in every case is the same: **a negative control that makes the new assertion fire
while the old one stays green.** Eight were run as new work across the three verifies; all eight
fired, and the tree was proved byte-identical afterwards each time.

---

## The one ordering claim that became evidence

`UI-02/6` — *"the page rendered against the stub before any C++ existed"* — rested on a stamp
**transcribed from console output** at 3.1 and could not be re-created at 3.2 without deleting
delivered work. 3.2 verify traced it to the cause: `ui_layout_check.js` §0 emitted **no timestamp at
all**. 3.3 landed the one line (D27), and 3.3 verify checked the emitted stamp against the
filesystem:

> **`2026-08-12T22:47:48.514Z`**, machine-emitted by §0 — and **every** 3.3 C++ file is newer,
> `DbapSolver.h` by 40 seconds and `FieldSampler.cpp` by five minutes.

Three boundaries from finding to closing, and the claim is now self-evidencing for every phase after
it.

---

## Carried forward to Stage 4

| Item | State |
|---|---|
| **Gate 13 — Standalone launch, interactive half** | **Open, ~15 min human.** The static half is discharged: at 3.2 the shell, plan, dashed hull rings and both frame banners were confirmed in WKWebView; at 3.3 all four new components were. Every *interactive* item needs synthetic clicks this environment cannot deliver (`-25208`) |
| **Q5 — a 30 Hz meter poll against a HIDDEN WKWebView** | **Open, and unrun by four consecutive phases.** The JS half is measured (N9), the JUCE drop is read from source, the two together have never been executed. `js/meters.js` exposes a `dropped` counter for exactly this |
| **D5 — the Logic hall session** (QUAL-01's audible clause) | **Open.** Unchanged since 3.1 discuss |
| **CI wiring** | **Open, and every Stage-3 phase widened it.** 92 probes and both JS gates are local-only; the test targets are not in `build-and-release.yml`, so a JUCE bump performed without them ships silently |
| **Locks and file I/O in `processBlock`** | grep + inspection only — `-fsanitize=realtime` is unsupported by Apple clang 17.0.0. **Allocation is now measured soundly**, which makes the remaining gap sharper, not smaller |
| **Windows** | No Windows compiler has seen this code. MSVC habits are authored and asserted |
| **`COMPAT-04`** | Still the only summary row without a derived criteria section |

---

## Stage verdict

**Status:** ✅ **VERIFIED**

**Ready for Stage 4 (Polish):** **Yes.** No blockers.

**Nine requirement rows, 40 criteria, zero partials. 92 C++ probes and 69 JS gate sections, zero
failures, all re-run from a forced full recompile at each of three verify boundaries.**
