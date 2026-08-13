# Stage 4 — Polish · Phase 4.2 (host-and-ear) — Execute Summary

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **Phase 4.2 of 2**
**GSD phase:** execute
**Date:** 2026-08-13
**Branch:** `feat/o-octagon`
**Plan:** `PLAN-4.2.md` — 15 tasks in three blocks

---

## Status in one line

**Blocks A and B are complete and gated. Block C — the Logic session — has not run,
and nothing from it is claimed.** `COMPAT-02` remains open.

| Block | Tasks | State |
|---|---|---|
| **A — desk, before the freeze** | 1–6 | ✅ complete, committed |
| **B — desk, the re-freeze** | 7 | ✅ complete, 11 desk gates green |
| **C — the session (Logic 12.3 + BlackHole 64ch + ears)** | 8–15 | ⛔ **NOT RUN** |

**The ledger has not moved: 29 complete · 0 partial · 1 pending — of 30.**
PLAN-4.2's stated target of 30/30 is a *close-of-phase* target and is **not met**,
because the only row it closes (`COMPAT-02`) can only be closed in a host.

---

## The freeze record (P110)

Block C runs against **this**, not against whatever is on disk that afternoon.

| Item | Value |
|---|---|
| **Commit SHA** | `378fb4cdc70ef7e7b4523771dd4f014f189246ec` |
| **VST3 bundle binary** | `928cd447c57435c93554fbb90fd14ec035cd39e8a8db54a5aba37a1597e0bb42` |
| **AU bundle binary** | `cc54db026875173e47daf691228c4c80c52da4c9050880aea0976bc16fe1fc99` |
| **C++ probes** | **95 / 0 failures** (unit 45, harness 50) — *unchanged from 4.1, as asserted* |
| **JS gate sections** | **70** — frontend 42 + layout 28 |
| **Installed as** | `O-Octagon-dev.vst3` / `O-Octagon-dev.component` (dev branding) |
| **CI run** | [31753001642](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31753001642) — success, both jobs, `headSha` == the freeze commit |
| **Bit-reproducible** | ✅ both checksums re-derived from a second **full `rm -rf` + reconfigure + rebuild** |

The freeze was cut **exactly once**, at the end of Block A, and **nothing in `Source/`
has moved since** (Constraint 6).

> **The bit-reproducibility check is stronger than 4.1's.** 4.1 used `ninja -t clean`,
> which reuses the configured tree. This one wipes `build/` and reconfigures from
> nothing, so the match also rules out a stale CMake cache.

---

## What each task did

### Block A

**Task 1 (D21) — the 4.1 verify artifacts landed first.** `VERIFICATION-4.1.md` was
untracked; an uncommitted verify is a verify nobody else can see
(`pattern_uncommitted_improve_versions_lost`). Committed at `12ddcb12` with the
4.2 discuss/research/plan set and the re-pinned `ROADMAP.md`.

**Task 2 (D19 / P109) — Gate 16b re-spelled.** Measured before the edit, exactly as
P109 predicted: the receiver-agnostic pattern returned **2** hits (`:202` the
doc-comment, `:222` the real call), and PLAN-4.1's literal spelling returned **0** —
it matched nothing from the day it was written. The comment now states the gate
receiver-agnostically **and names the method without an opening paren**, so it does
not count itself. Post-edit: **exactly 1 hit, and it is the call.**

> **The call moved to `:226`.** PLAN-4.2 quotes `:222`; the re-spelled comment is four
> lines longer. Recorded here so a verify pass does not read the plan's line number as
> drift. `PLAN-4.1.md`'s Gate 16 literal is history and was **not** edited.

**Task 3 (P104 / P105) — `tests/tools/analyse_bounce.py`.** Four modes, stdlib only,
24-bit sign-extending unpack done in bulk through `struct`, Goertzel at `N = fs` so
every tone is bin-exact. One-or-many `--input` (P104). All six anti-vacuity clauses
enforced in the tool.

**Task 4 — the generators.** `gen_bounce_sources.py` (eight tones + the
Schroeder-phased LFE multitone + a per-partial sidecar) and `gen_audible_source.py`
(CU's locator). **No RNG anywhere** — re-runs are byte-identical, verified by
double-generation and `shasum`.

**Task 5 (P106) — `tests/fixtures/cr-b-permuted.venue`.** The 8-cycle derangement.
Positions, trims and rake are **omitted**, so "otherwise identical to the shipped
default" holds *by construction* through `VenueFile`'s per-attribute fallback
(`VenueModel.cpp:194-197`) rather than by a transcription that would drift. Verified
against labels **parsed from `VenueModel.cpp`**: same label set, no fixed point,
permutation `2,3,4,5,6,7,8,1`.

**Task 6 (P101) — layout-check section 28, and NC1.** See below; this is the one that
mattered.

### Block B

**Task 7 (P110) — the re-freeze and eleven desk gates.** Full detail and verbatim
figures in `evidence/desk-gates-4.2.txt`. All green.

---

## Q5's mechanism, executed for the first time in five phases

Section 28 constructs a **fresh `createMeters` instance** in page scope via dynamic
import, with a `getMeters` whose first request **never settles**, and asserts:

1. the guard **released on its 165 ms deadline** — `dropped = 1`;
2. the poll **continued past it** — 2 calls when the drop was first observed, **9** by
   the end.

No source change, no `window` handle, and the module under test is the shipped file
byte-for-byte. The deadline (`33 × 5`) is **read out of `meters.js` at run time**, not
mirrored.

**NC1 was run.** Deleting `meters.js:151-152` makes section 28 fail on both mechanism
clauses, and the tree is byte-identical after revert (`9f121647…`).

> **NC1 changed the section.** Its third clause originally printed
> *"request 1 never settled, so call 2 is only reachable through the deadline"* — true
> of the shipped guard, **false under the mutation**, where the guard is inert and the
> count rises for the opposite reason. The clause was passing with a false explanation
> attached. It now claims only what it measures. **A negative control that corrects the
> positive control is the control working**, and it is the reason NC1 was worth the
> hour.

---

## A pre-existing flake, found and fixed with approval

Section 27 (`UI-04` criterion 2, 3.3's evidence) failed **~1 run in 5**, reading
`11 -> 12`. Measured **interleaved** to remove load as a confound: **1/5 on the pristine
3.3 file** and **1/5 with section 28 present**. It predates 4.2 — 4.1's Gate 4 passed by
luck.

**Cause.** `field.js:161` runs `refresh()` *at most once per status tick*, and the tick
is 2 Hz. A field input marked dirty by an **earlier** section is owed a recompute that
lands whenever the next tick fires — including inside section 27's 24-frame drag window,
where it reads as drag-caused. The metric window overlapped the poll period
(`pattern_metric_window_vs_modulation_period`).

**Fix (user-approved, test-only).** The drag window now opens only after the count has
been stable across three consecutive ticks, with a bound and an explicit
`[precondition]` assertion so a count that never settles is a finding rather than a
hang. **The assertion itself is unchanged — only its starting state is.**
**10/10 green afterwards.**

---

## Beyond the plan: the analyser has a committed self-test

`tests/tools/selftest_analyse_bounce.py` — **24 cases, all green.**

PLAN-4.2's standing rule is that every gate is *re-run from scratch at verify*. The
analyser's six clauses can only be exercised against bounces, and the real bounces do
not exist until the session has run — so "the clauses were verified" would otherwise
rest on a transcript nobody could re-execute. The self-test **synthesises** the
pathological captures at the desk (a muted track, a silent LFE pair, a leaky ping
window, a wrong burst period, a tampered manifest) and asserts the analyser's exit
code on each.

**Every clause has now been seen to fail**, including the two that carry the phase:

| Clause | Case | Result |
|---|---|---|
| 2 (**N13**) | `--label CR-b` with an identity `--expect` | **refused**, rc 1 |
| 3 | a muted track | **fails**, not a 7-of-8 partial pass |
| 5 | a **silent** LFE pair | **fails** — two equal silences are not "flat" |
| — | a ping capture with a wrong burst period | **fails** the `VerifyPing.h` constant assertion |
| 6 | a tampered / empty / result-less manifest | all **fail** |

---

## What is NOT done, and what it needs

**Tasks 8–15 and Gates 12–25.** Every one needs Logic Pro 12.3, a BlackHole 64ch
device, or a human ear:

| Task | Gate | Needs |
|---|---|---|
| 8 | 12, 13 | the two stop-gates — a 4 s surround bounce, and the `getStatus` pre-flight |
| 9 | 14, 15 | `COMPAT-02`/1 (instantiation **+ session recall**) and /3 (11 lanes **written and read back**) |
| 10 | 16 | **CT** — the realtime loopback ping capture. `COMPAT-02`/2 |
| 11 | 17, 18, 19 | **CR-a**, **CR-b**, and NC2 + NC3 |
| 12 | 20, 21 | **CS** — the LFE test on **both** paths, and NC4 **before** any D16 disposition |
| 13 | 22 | **CU** — the audible clause, **headphones named** |
| 14 | 23 | Gate 13's interactive half, **relabelled as throttling-recovery** |
| 15 | 25 | the ledger, the docs, and `analyse_bounce.py --check` on the committed artifacts |

Everything those tasks need is now built, committed and self-tested: the analyser, the
generators, the CR-b fixture, and a frozen, installed, bit-reproducible binary.

### Carried into the session unchanged

- **`airAmount = 0` on CR-a, CR-b, CT and CS — never on CU** (Constraint 1). The CU
  generator prints this reminder on every run.
- **CS runs under the CR-a identity venue.** Under CR-b, speaker 4 is not the LFE slot.
- **NC4 runs before any D16 disposition.** A confound there costs a second freeze and
  eighteen re-run gates.
- **Gate 24 is only half-measured.** The desk half holds (empty-tree hash, both blocks).
  Re-measure after the session.

---

## Deferrals and residuals

| Item | Owner |
|---|---|
| `COMPAT-02` — all three criteria | **the session (Block C)** |
| `QUAL-01` criterion 2's audible clause | **the session (Task 13)** |
| `VenueModel.cpp:87-89` "all three containers" prose | **v1.1 doc row** (P107 — not this freeze) |
| One specific hardware driver (criterion 2's scope, D11) | **none** |
| Windows UI correctness · RT-safety beyond allocation · the two JS gates in CI · spatial coherence in a hall | **none** |

Two configure facts worth carrying, both of which cost time here: `OUARICON_BUILD_TESTS`
defaults **OFF** (without it the two test targets are not generated at all), and a bare
configure of this repo **fails** in O-Orbit, whose `libs/SAF/framework` is not checked
out — `-DSKIP_PLUGINS="O-Orbit"` is the escape and does not touch O-Octagon.

---

## Evidence

| File | What |
|---|---|
| `evidence/desk-gates-4.2.txt` | gates 1–11 and the desk half of 24, with verbatim figures |
| `tests/tools/selftest_analyse_bounce.py` | 24 cases; re-runnable at verify with no host |
| CI [31753001642](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31753001642) | `45 probe(s), 0 failure(s)` + `50 probe(s), 0 failure(s)` **in the runner's own log** |

No evidence file is named `*.log` (P108 — `.gitignore:217` is `*.log`, re-verified with
`git check-ignore` at this boundary).
