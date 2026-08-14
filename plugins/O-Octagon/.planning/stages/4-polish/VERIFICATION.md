# Stage 4 — Polish / Validation — Stage Verification (roll-up of 4.1 + 4.2)

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **stage-level roll-up**
**GSD phase:** verify
**Date:** 2026-08-14
**Branch:** `feat/o-octagon` @ `bc8819df`
**Freeze commit under test:** `378fb4cdc70ef7e7b4523771dd4f014f189246ec`
**Worktree:** `/Users/taylorbrook/Dev/VST-development-octagon`

---

## Verdict in one line

**Stage 4 is NOT complete. Nine of twenty-five gates (17–25) have not run, and the stage goal's
second clause — "with the bounce path confirmed" — is unmet by construction: the bounce-order
measurement is Gate 17 (CR-a) and it did not run.** Everything that *has* run re-measured green at
this boundary, including a full re-derivation of the one gate whose figures were recorded as
transcribed.

The ledger reads **30 / 0 / 0**. That is true and it is not the completion signal — see Finding 2.

---

## Entry Check — contract checksums

Measured here against **`STATUS.md`'s live `contract_checksums` block** (`STATUS.md:1369-1372`),
never against a prior artifact's prose.

| Contract | `shasum -a 256` measured here | Ledger | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ **thirteen consecutive phases unmoved** |
| `parameter-spec.md` | `b45f88dc5017ec2c…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ unmoved since Stage 1 |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…57bceb17` | `2806c788…57bceb17` | ✅ |
| `ROADMAP.md` | `ea50d991d1a6b158…1063d424` | `ea50d991…1063d424` | ✅ |

**No drift. No contract amended at this boundary.**

---

## Goal-Backward Analysis — the stage goal, not a phase goal

### Stage 4 goal (ROADMAP, as amended by D11)

> **Logic Pro 12.3 on BlackHole 64ch, with the bounce path confirmed.**

### The goal has two clauses and they separate cleanly

| Clause | Status | Evidence measured here |
|---|---|---|
| **Logic Pro 12.3 on BlackHole 64ch** | ✅ **Achieved** | Gates 12–16 ran on that exact rig. Gate 13 read `outputSetName == "7.1 Surround"`, `safeMode false`, `mapInvalid false`, 8 ch |
| **With the bounce path confirmed** | ❌ **Not achieved** | The bounce **order** is Gate 17 (CR-a) and it did not run. Gate 12 confirms the bounce *container* — 8 ch / 48 kHz / 24-bit, re-derived here — **not** which channel carries which label |

**This is not a technicality.** Gate 16's own scope note draws the line explicitly and correctly:

> *"CT measured the REALTIME / DEVICE order. A Logic bounce writes channels per the file's layout
> tag, which need not equal the device order. CR-a (Gate 17) measures the BOUNCE order and is a
> genuinely separate measurement… Nothing here is claimed about the bounce order."*

The stage named the bounce path in its goal line. The session measured the **device** path. Those
are two different orders, the artifact says so itself, and the second one is unmeasured.

### Phase roll-up

| Phase | Verdict | Source |
|---|---|---|
| **4.1 — machine gates** | ✅ **VERIFIED** | all 18 gates re-run from scratch; `COMPAT-04` 3 of 3, `COMPAT-01` re-confirmed; CI residual closed |
| **4.2 — host-and-ear** | ⚠️ **PARTIAL** | Blocks A + B verified (11 desk gates re-run); Block C **part 1 only** — gates 12–16 of 25 |

Stage 4 inherits the weaker of the two: **PARTIAL**.

---

## Automated Checks — RE-RUN FROM SCRATCH at this boundary

Every figure below was measured here at `bc8819df`, not read out of `SUMMARY-4.2.md`,
`desk-gates-4.2.txt`, or `session-gates-4.2.txt`.

| # | Check | Result | Figure measured **here** |
|---|---|---|---|
| 1 | Contract checksums vs the live ledger | ✅ | four exact; no amendment |
| 2 | Both C++ targets build | ✅ | exit 0; `warning:` 0, `error:` 0, `FAILED` 0 |
| 3 | Probe suites | ✅ | **45 + 50 = 95 probes, 0 failures** |
| 4 | `node tests/ui_frontend_check.js` | ✅ | exit 0 — **42 sections** |
| 5 | `node tests/ui_layout_check.js` | ✅ | exit 0 — **28 sections**, did not SKIP |
| 6 | P110 invariant | ✅ | **42 + 28 = 70** JS gate sections |
| 7 | `Source/` moved since the freeze | ✅ **No** | `git diff --name-only 378fb4cd..HEAD -- Source/` is **empty** |
| 8 | Installed VST3 vs freeze record | ✅ | `928cd447…97e0bb42` — **exact match** |
| 9 | Installed AU vs freeze record | ✅ | `cc54db02…6fe1fc99` — **exact match** |
| 10 | Dual-variant sweep | ✅ | `-dev` only on disk; no alternate to shadow the AU slot |
| 11 | **Gate 9, run as the literal recorded string** | ✅ | exit 0 — `--check OK — 102 cases` |
| 12 | `selftest_analyse_bounce.py` | ✅ | **24 cases, every clause seen to fire** |
| 13 | **Gate 25 — `--check` on the committed manifest** | ✅ | exit 0, **1 run re-derived** |
| 14 | **Gate 25 negative control** | ✅ | `--session-root /tmp/nonexistent` → **exit 1**, so the gate is not vacuous |
| 15 | **Gate 12 probe, re-derived** | ✅ | 8 ch / 48000 Hz / 24-bit / 19.500 s — exact |
| 16 | **Gate 16 / CT, fully re-derived** | ✅ **substance** | see below — every figure reproduces |
| 17 | Ledger recount from the status column | ✅ | **30 complete · 0 partial · 0 pending of 30** |

### Gate 9 — the 4.2 Issue-1 spelling defect is genuinely closed

`VERIFICATION-4.2` Issue 1 recorded that Gate 9's transcribed invocation could not be executed as
written. It has since been corrected in `desk-gates-4.2.txt:146`. **Verified the only way this class
of defect can be verified — by running the literal recorded string**
(`pattern_recorded_gate_command_not_executable_as_spelled`):

```
python3 tests/tools/gen_dbap_reference.py --check --output tests/fixtures/DbapReferenceFixture.h
  -> gen_dbap_reference.py: --check OK — 102 cases   exit 0
```

Runs as spelled. The solver is untouched by Stage 4 and this is what proves it.

### Gate 16 / CT — the transcribed figures were re-derived, and they hold

`session-gates-4.2.txt:315-317` records a known gap: CT's figures are **transcribed from stdout**,
because `--emit-json` correctly declined to record a run that failed its own assertion. The capture
WAVs are still on disk at `~/Dev/octagon-4.2-session/`, so this verify **re-ran the analysis rather
than accepting the transcript**:

| Quantity | Recorded at execute | **Re-derived here** |
|---|---|---|
| bursts found | 8 | **8** |
| burst period | 1.600 s constant | **1.600 s constant** (tol 0.050 s) |
| burst length | 1.205 s vs `kOnSeconds` 1.200 | **1.205 s** |
| per-file spread | 0 frames | **0 frames** |
| energised per window | exactly one, all 8 | **exactly one, all 8** |
| per-window isolation | 220.0 220.0 220.0 220.2 219.9 220.1 220.0 220.1 | **identical, all eight** |
| minimum isolation | 219.9 dB vs a 40.0 dB floor | **219.9 dB** |
| observed sequence | 1,2,5,6,7,8,3,4 | **1,2,5,6,7,8,3,4** |
| tool verdict | FAILED (sequence) | **FAILED, exit 1** |

**Every figure reproduces exactly, including the failure.** The gap recorded at execute is therefore
narrower than it was stated: the figures are *not in the manifest*, but they **are re-derivable by a
second person** from the committed tool and the capture files. The obligation that survives is
archival — the WAVs live outside the repo and nothing pins them.

### Gate 16's routing argument — verified from source, not accepted

Criterion 2 closed on a **substance pass over a tool FAILED verdict**. That is the single most
load-bearing judgement in Stage 4, so its premises were checked against JUCE itself rather than
against the artifact that asserts them:

| Claim | Verified against | Result |
|---|---|---|
| `create7point1()` = `{left, right, centre, LFE, leftSurroundSide, rightSurroundSide, leftSurroundRear, rightSurroundRear}` | `juce_AudioChannelSet.cpp:567` | ✅ **exact** |
| Enum bit values 1, 2, 3, 4, **10, 11, 20, 21** | `juce_AudioChannelSet.h:407-430` | ✅ **exact** — so buffer order is `L R C Lfe Lss Rss Lrs Rrs` |
| JUCE names corroborate the AU reading | same header | ✅ `leftSurroundSide` = *'Left Centre "LC" (AU)'*; `leftSurroundRear` = *'Rls (AU)'* |
| Inverted measurement = Emagic 7.1 | arithmetic on the re-derived sequence | ✅ dev order `L R Lrs Rrs C Lfe Lss Rss` = `L R Ls Rs C LFE Lc Rc` |

The permutation `1,2,5,6,7,8,3,4` is the JUCE-buffer-order → Logic-device-order mapping, exactly as
recorded. **The plugin's own speaker→buffer map is an identity and is not defective.** The
disposition is sound, and the decision *not* to loosen `ping` mode's assertion mid-session was the
right one (`pattern_zipper_sweep_probe_needs_liveness_gate`'s sibling failure — fitting the
assertion to the result it just produced).

---

## Requirements Verification

**Ledger re-counted here** from `REQUIREMENTS.md`'s own status column, not copied from any summary:

**30 complete · 0 partial · 0 pending — of 30.** `openRows:` empty.

| Requirement | Priority | Status | Basis measured here |
|---|---|---|---|
| `COMPAT-02` — Logic Pro, 8 discrete channels | must | ✅ **complete, 3 of 3** | Gate 14 (crit 1), Gate 15 (crit 3), Gate 16 / CT (crit 2, re-derived here) |
| `COMPAT-01` — VST3/AU load and validate | must | ✅ complete | 4.1 + 4.2 desk gates on the frozen binary |
| `COMPAT-04` — channel configurations | must | ✅ complete, 3 of 3 | 95 probes re-run here + `auval`'s six `AUChannelInfo` configs |
| `QUAL-01` — no audio artifacts | must | ⚠️ **`complete`, but criterion 2's audible clause is UNCONCLUDED** | rides Gate 22 (CU), **not run** — see Finding 2 |

### `COMPAT-02` — criterion by criterion

| # | Criterion | Status | Closed by |
|---|---|---|---|
| 1 | Instantiates on a 7.1 surround track | ✅ closed | Gate 14 — **and survives save / quit / reopen**, which had never been observed before |
| 2 | Verify-ping reaches 8 distinct physical channels | ✅ closed on its **routing** half | Gate 16 / CT, 219.9 dB isolation. **D11 residual stands**: the specific-hardware-interface half has owner `none` |
| 3 | `srcX/Y/Z` + `w1..w8` visible **and writable** | ✅ closed | Gate 15 — 11 lanes written **and read back**. Operator-reported; labelled as such, not dressed as machine-measured |

---

## Issues Found

### 1. Gate 13's evidence artifact does not exist — a STOP-GATE with nothing behind it

`session-gates-4.2.txt:95-97` records:

> *"EVIDENCE: banner-state screenshots of both the Room and Venue screens, supplied by the operator
> at the boundary. **Still owed as a committed file:** `~/Dev/octagon-4.2-session/gate13-banner.png`"*

Measured here:

```
ls ~/Dev/octagon-4.2-session/*.png        -> no matches
git ls-files | grep -iE 'gate13|banner'   -> empty
```

**The file exists in neither place.** Gate 13 is one of the two STOP-gates; it gates every bounce
that follows it, and Gate 14's criterion-1 half 1 does not re-observe the surround track — it
**inherits** Gate 13's reading (`session-gates-4.2.txt:155`, *"PASS — established at Gate 13"*).
So a single unevidenced observation carries two gates and one closed requirement criterion.

The observation is very likely correct — Gate 12's eight-live-channel spread independently
corroborates `mapInvalid == false`, and that corroboration is real and was re-derived here. But the
artifact the gate itself says it owes is absent, and this verify will not record an owed artifact as
a delivered one.

**Disposition:** re-capture both banner screenshots at the start of Block C part 2 (the session
reopens the same project) and commit them under `evidence/`. Not a blocker on its own — it is
subsumed by the Block C blocker below — but it must not survive the stage close unrecorded.

### 2. The 30/0/0 ledger overstates completeness — `QUAL-01`'s audible clause is unconcluded

`REQUIREMENTS.md` shows `QUAL-01 | must | complete | stage-2 (2.3)`, and the ledger totals 30/0/0
with `openRows:` empty. Both the frontmatter and `session-gates-4.2.txt` state plainly that
**criterion 2's audible clause is still unconcluded and rides Gate 22 (CU), which has not run.**

This is not a new defect and nothing is being hidden — every artifact says it out loud, which is why
this is Finding 2 and not a blocker. The point for the stage close is narrower:

> **A 30/0/0 ledger with an empty `openRows` is the exact shape a "ready to ship" reader looks for,
> and O-Octagon is not ready to ship.** The row-level status column cannot express "complete except
> for one clause that needs an ear", so the ledger reads clean while a `must` requirement carries an
> open clause.

**Disposition:** the completion signal for Stage 4 is **gates 17–25 green**, not the ledger. Stated
here so no later reader treats 30/0/0 as the gate. `openRows:` already carries the residual in prose;
this verify does not amend the row, because the row is correct at the granularity it has.

### 3. `ping` mode's `1..8` expectation is now known-wrong and still hard-coded

Gate 16 falsified `PLAN-4.2`'s "sequence 1..8" assumption. The tool still hard-codes it and takes no
`--expect` in ping mode, so **CT fails its own assertion every time it is run correctly** — as it did
again in this verify. The named deferral from execute (add a mandatory `--expect` to ping mode, or
formally accept CT as a transcribed-figure gate) is **still undecided**.

**Disposition:** decide at Block C's close. This verify records the second option as now
better-supported than it was — the figures were re-derived here, so "transcribed" understates what a
second person can reproduce.

### 4. Gate 23's throttling-recovery relabelling is still owed

Carried since P101 and named again at 4.2 verify. It requires the interactive drive to have
happened, so writing it now would assert an unrun gate. **Correctly still owed, not absorbed.** This
is its third boundary; the risk named in `PLAN-4.2` is that the false premise gets inherited a fifth
time, and an unrun gate is how that happens quietly.

### 5. `REQUIREMENTS.md` `lastVerified` was execute-side — **FIXED here**

It read *"…EXECUTE, NOT VERIFY … These figures have NOT yet been re-run from scratch at a verify
boundary — that is owed before the row is treated as settled."*

That debt is discharged for gates 12, 16 and 25 by this verify's re-derivations, and for gates 13, 14
and 15 it cannot be — they are human-observation gates by construction. Frontmatter updated to say
exactly that, and no more.

### What was checked and found clean

- **`STATUS.md` is accurate this time.** `VERIFICATION-4.2` Issue 2 recorded that execute never
  updated it; `phase: execute` / `status: block_c_in_progress` now matches reality, and the stale
  4.2 note is preserved as superseded history rather than overwritten. The failure did not recur.
- **The self-contradiction fixed at `bc8819df` was a real hazard, correctly handled.** The stale
  "NOTHING ABOVE IS CLAIMED / 29 · 0 · 1" summary would have made this verify's evidence contradict
  the committed ledger, and the safe reading of a contradiction — trust the pessimistic line — would
  have reopened a correctly closed row and re-run four host gates to do it. The original text is
  preserved above the correction, so P109 is honoured.
- **No `*.log` evidence files; no RNG in the tools; `Source/` untouched since the freeze.**

---

## What did NOT run — measured against the gate record, not inherited

| Gate | What it measures | Blocked on |
|---|---|---|
| **17 — CR-a** | **canonical BOUNCE order** — the stage goal's second clause | Logic session |
| **18 — CR-b** | permuted venue under the committed 8-cycle | Logic session |
| **19 — NC2 + NC3** | the controls that make 17/18 non-vacuous | Logic session |
| **20 — CS** | LFE on **both** render and monitor paths | Logic session · **`--channels` must be re-derived per path** |
| **21 — NC4** | `airAmount` confound control, **before** any D16 disposition | Logic session |
| **22 — CU** | `QUAL-01`/2's audible clause, **headphones NAMED** | Logic session + a human ear |
| **23** | interactive drive + the throttling-recovery relabelling | Logic session |
| **24** | `User/` presets byte-identical **after** the session | Logic session |
| **25** | `--check` on the committed session artifacts | the artifacts 17–24 produce |

**Nine gates. Every one of them is recorded NOT RUN in `session-gates-4.2.txt:335-343`, and no result
from any of them is claimed, implied, or inherited anywhere in the stage's artifacts.** This verify
looked for such a leak specifically and found none.

### Permanent residuals — owner `none`, not blockers

| Item | Why it cannot close |
|---|---|
| One specific hardware interface driver (`COMPAT-02`/2's "physical" half, D11) | No 8-out interface attached; would not generalise across interfaces even with one |
| Windows **UI correctness** | Hardware. CI's ceiling is pluginval 10 opening the editor |
| RT-safety beyond allocation (D10) | `-fsanitize=realtime` unsupported by Apple clang 17.0.0 |
| The two JS gates in CI | Headless-render determinism |
| Spatial coherence in a hall (D2) | No requirement row asks for it |
| `ARCHITECTURE.md`'s three intermediate checksums | Unreconstructible from git; recorded once |

---

## Human Verification

Block C part 2. None of it has been done.

- [ ] **Gate 17 / 18** — CR-a then CR-b under the committed 8-cycle, non-identity assertion active
- [ ] **Gate 19** — NC2 (tool refuses an identity `--expect` on a CR-b label) and NC3 (a missing tone fails)
- [ ] **Gate 20** — CS: LFE on **both** paths, per band, `--channels` **re-derived per path**
- [ ] **Gate 21** — NC4, run **before** any D16 disposition
- [ ] **Gate 22** — CU: both halves of D12, **headphones named**, outcome recorded either way
- [ ] **Gate 23** — interactive drive; hidden-editor check **relabelled as throttling-recovery**
- [ ] **Gate 24** — `User/` presets re-measured after the session
- [ ] **Gate 25** — `analyse_bounce.py --check` on the committed session artifacts
- [ ] **Gate 13 re-capture** — commit the two banner screenshots (Finding 1)

**Carried into the session unchanged:**

1. **`airAmount = 0` on CR-a, CR-b, CT and CS — never on CU** (Constraint 1). An HF delta from
   `airAmount` reads exactly like bass management and would trigger D16's re-freeze on nothing.
2. **CS runs under the CR-a identity venue.** Under CR-b, speaker 4 is not the LFE slot.
3. **Gate 20's `--channels 4,1` is WRONG for a device-order capture** — CT measured LFE at device 6,
   reference at 1, so `--channels 6,1` on the loopback path. The **bounce** path must be re-derived
   from CR-a's own result. Running it as spelled would report a confident wrong answer.
4. **NC4 runs before any D16 disposition.**
5. **No `Source/` edit** — anything needing one re-enters Block B with a second freeze.

---

## Stage Verdict

**Status: ⚠️ PARTIAL**

**Ready for next stage: No.** Stage 4 is the final stage; the plugin is **not** ready for
`/install-plugin` or release.

**What Stage 4 genuinely closed.** `COMPAT-04` at 3 of 3 with a measured non-redundancy control.
`COMPAT-01` re-confirmed on the binary that will actually ship. CI running a test target for the
first time in this repo. A binary frozen, installed, and proven bit-reproducible across three
independent full builds. And in Block C part 1: the plugin **instantiates on a Logic 7.1 surround
track, survives save / quit / reopen, exposes all 11 automation lanes for write and read-back, and
lands its eight speakers on eight distinct physical device channels at 219.9 dB isolation** — with
the buffer-order → device-order permutation measured, explained from JUCE source, and shown to be a
property of the host rather than a defect in the plugin. `COMPAT-02` is closed 3 of 3 and every one
of those figures that a machine can re-derive was re-derived at this boundary.

**What Stage 4 did not close.** The bounce path. It is in the goal line, it is Gate 17, and it did
not run — along with eight other gates and the one clause in the project that needs a human ear.

**Blockers:**

1. **Block C part 2 has not run** — gates 17–25. Needs Logic Pro 12.3, BlackHole 64ch, and a human
   ear. `QUAL-01` criterion 2's audible clause is unconcluded. Estimated 45–75 minutes.
2. **The stage goal's "bounce path confirmed" clause is unmet** — discharged by Gate 17 (CR-a), which
   is inside blocker 1.

**Owed at the Block C close, not blockers:**

3. Gate 13's banner screenshots, committed (Finding 1).
4. The `ping`-mode `--expect` decision (Finding 3).
5. Gate 23's throttling-recovery relabelling sentence (Finding 4).

### The freeze, re-confirmed at the stage boundary

| Item | Value |
|---|---|
| Commit SHA | `378fb4cdc70ef7e7b4523771dd4f014f189246ec` |
| VST3 bundle binary | `928cd447c57435c93554fbb90fd14ec035cd39e8a8db54a5aba37a1597e0bb42` |
| AU bundle binary | `cc54db026875173e47daf691228c4c80c52da4c9050880aea0976bc16fe1fc99` |
| Installed binaries vs the freeze | **exact match, both** — measured here |
| Probes | **95 / 0 failures** (unit 45, harness 50) — re-run here |
| JS gate sections | **70** (frontend 42 + layout 28) — re-run here |
| Variants on disk | `-dev` only; no alternate to shadow the AU slot |
| `Source/` moved since the freeze | **No** — `git diff` empty |

**Block C part 2 runs against this binary.** If it does not, the mismatch is a real signal.

---

## Verification Environment Note

Ran from the dedicated worktree at `/Users/taylorbrook/Dev/VST-development-octagon`, because
`feat/o-octagon` is checked out there and the shared checkout at `/Users/taylorbrook/Dev/VST-development`
is on `main`. No branch was switched in the shared checkout.

Two configure facts, unchanged and still true:

- **`OUARICON_BUILD_TESTS` defaults OFF** — without `-DOUARICON_BUILD_TESTS=ON` the two test targets
  are not generated at all. The existing `build/` cache already carries `ON`.
- **A bare configure of this repo fails in O-Orbit**, whose `libs/SAF/framework` is not checked out.
  `-DSKIP_PLUGINS="O-Orbit"` is the escape and does not touch O-Octagon.

`git status` is clean apart from this verify's own artifact writes.

---

## Next Phase

**Ready for:** **Block C part 2 of phase 4.2** — gates 17–25 — against commit
`378fb4cdc70ef7e7b4523771dd4f014f189246ec` and the two bundle checksums above.
Procedure: `evidence/BLOCK-C-RUNBOOK.md`.

There is **no 4.3** (D18). Block C completes phase 4.2, which completes Stage 4. A D16 finding
re-enters Block B with a second freeze rather than opening a new phase.
