# Stage 4 — Polish · Phase 4.2 (host-and-ear) — Verification

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **Phase 4.2 of 2**
**GSD phase:** verify
**Date:** 2026-08-13
**Branch:** `feat/o-octagon` @ `300d8cf02a18cf0c8b4d3f4a4721cbc76a3bfbd6`
**Freeze commit under test:** `378fb4cdc70ef7e7b4523771dd4f014f189246ec`
**Worktree:** `/Users/taylorbrook/Dev/VST-development-octagon`

---

## Verdict in one line

**Blocks A and B are verified — all eleven desk gates were re-run from scratch at this boundary and
all eleven are green. Block C did not run. `COMPAT-02` remains the project's only open row, and
Stage 4 is therefore NOT complete.**

The phase delivered **half of its goal, and it is the half that was in its power to deliver.** The
goal's second clause — *be honest in the artifact about which half of each claim the rig could
reach* — is met unusually well: the execute summary claims nothing from the unrun session, and this
verify found no instance of a Block C result being asserted, implied, or inherited.

---

## Entry Check — contract checksums

Per the standing rule (VERIFICATION-4.1 Issue 2), measured against **`STATUS.md`'s live
`contract_checksums` block** — never against a prior artifact's prose.

> **Line-number note, recorded so the next reader does not score it as drift.** At the moment of
> measurement the block sat at `STATUS.md:1217-1220`. This verify then wrote its own `phase` /
> `status` / `next_action` entries into the frontmatter above it, so **the block now sits at
> `STATUS.md:1259-1262`**. The four *values* are byte-identical before and after — re-measured after
> the edit — and the contracts themselves are untouched. Same bookkeeping Gate 5 did for
> `PresetPolicy.h:222 → :226`.

| Contract | `shasum -a 256` measured here | Ledger | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ **twelve consecutive phases unmoved** |
| `parameter-spec.md` | `b45f88dc5017ec2c…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ unmoved since Stage 1 |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…57bceb17` | `2806c788…57bceb17` | ✅ |
| `ROADMAP.md` | `ea50d991d1a6b158…1063d424` | `ea50d991…1063d424` | ✅ |

**No drift. No contract is amended at this boundary** — `parameter-spec.md` does not move, as
Constraint 8 requires.

---

## Goal-Backward Analysis

### Original goal (PLAN-4.2)

> **Close the last requirement row with measurements a second person could re-derive, and be honest
> in the artifact about which half of each claim the rig could reach.**

### The goal has two clauses, and they separate cleanly

| Clause | Status | Evidence |
|---|---|---|
| **Close the last requirement row** (`COMPAT-02`) | ❌ **Not achieved** | Block C never ran. The ledger stands at 29 · 0 · 1, re-counted here from `REQUIREMENTS.md` rather than read from the summary |
| **Measurements a second person could re-derive** | ✅ **Achieved for everything delivered** | Every desk gate re-ran from scratch at this boundary and reproduced its recorded figure. The analyser ships a 24-case self-test; the CR-b fixture is verifiable against labels parsed from source |
| **Be honest about which half each claim reached** | ✅ **Achieved** | `SUMMARY-4.2.md` opens by stating Block C did not run and that `COMPAT-02` is open; `desk-gates-4.2.txt` names gates 12–25 as NOT RUN and claims none of them |

### What "works" meant concretely at 4.2 — the plan's own five bullets

| # | Bullet | Status |
|---|---|---|
| 1 | `COMPAT-02`/2 is per-channel sample data via **CT** | ⛔ **not run** — needs the host |
| 2 | The bounce-order test is a **pair**, anti-vacuity enforced by the tool | ⚠️ **built and self-tested, not executed.** The tool's refusal of an identity `--expect` on a `CR-b` label is exercised by the self-test; the bounces do not exist |
| 3 | The LFE claim cites a measurement or triggers D16 | ⛔ **not run** — `VenueModel.cpp:84` still asserts what the research rates MEDIUM-LOW |
| 4 | **Q5's mechanism executed, with a control proving it can fail** | ✅ **Achieved — verified here by re-running NC1 end to end** |
| 5 | Every named deferral has an owner | ✅ **Achieved** — checked item by item below |

### Success Criteria from PLAN-4.2 — thirteen boxes, scored

| # | Criterion | Verdict |
|---|---|---|
| 1 | `COMPAT-02` closed 3 of 3 | ❌ 0 of 3 |
| 2 | `QUAL-01`/2 audible clause concluded | ❌ still bounded, not concluded |
| 3 | Ledger 30 · 0 · 0, `openRows:` empty | ❌ 29 · 0 · 1 |
| 4 | CR-a and CR-b both pass | ❌ not run (fixture + generators built) |
| 5 | NC2 and NC3 behaved as declared | ⚠️ not run **as controls**; both behaviours are proven in the analyser self-test |
| 6 | LFE claim settled on both paths | ❌ not run |
| 7 | **Q5's mechanism executed; NC1 proves it can fail** | ✅ **met** |
| 8 | **Gate 16b re-spelled and its own comment de-matched** | ✅ **met** |
| 9 | **Re-freeze recorded and bit-reproducible** | ✅ **met, and exceeded** — see below |
| 10 | **`COMPAT-01` and `COMPAT-04` re-confirmed on the shipping binary** | ✅ **met** |
| 11 | `User/` presets byte-identical across desk **and session** | ⚠️ desk half only, by construction |
| 12 | No `*.log` evidence; `analyse_bounce.py --check` green on committed artifacts | ⚠️ no `*.log` ✅; `--check` has no session artifacts to grade yet |
| 13 | Every deferral has an owner; hidden-editor check labelled throttling-recovery | ⚠️ owners ✅; the relabelling is Gate 23's and Gate 23 did not run |

**4 met · 4 partial · 5 not met.** The five failures are one failure with five faces: Block C.

### One result stronger than the plan asked for

Criterion 9 asked that the re-freeze be bit-reproducible. It was already demonstrated twice at
execute. **This verify built it a third time — from a fresh `rm -rf build` plus full reconfigure, on
a different day, in a tree that had meanwhile been mutated and reverted by NC1 — and both bundle
binaries reproduced byte-for-byte:**

```
VST3  928cd447c57435c93554fbb90fd14ec035cd39e8a8db54a5aba37a1597e0bb42
AU    cc54db026875173e47daf691228c4c80c52da4c9050880aea0976bc16fe1fc99
```

Three independent full builds, three identical pairs. The freeze is not merely recorded; it is
**reconstructible from source by anyone with the commit**, which is exactly the property Block C
needs in order to run later against the same binary the desk gated.

---

## Automated Checks — all 11 desk gates, RE-RUN FROM SCRATCH

Per PLAN-4.2's standing rule, every figure below was **measured at this boundary**, not read out of
`SUMMARY-4.2.md` or `desk-gates-4.2.txt`. Where a figure differs from the execute record it is
called out.

| # | Gate | Result | Figure measured **here** |
|---|---|---|---|
| 1 | Contract checksums vs the ledger | ✅ | four exact; no amendment |
| 2 | Forced full recompile + both C++ targets | ✅ | exit 0; `warning:` 0, `error:` 0, `FAILED` 0; **45 + 50 = 95 probes, 0 failures** |
| 3 | `node tests/ui_frontend_check.js` | ✅ | exit 0 — **42 sections** |
| 4 | `node tests/ui_layout_check.js` | ✅ | exit 0 — **28 sections**, did not SKIP |
| 5 | Gate 16b re-spelled | ✅ | **exactly 1 hit**, and it is the call: `PresetPolicy.h:226` |
| 6 | **NC1** | ✅ | section 28 **fails** under the mutation (`dropped = 0`, both mechanism clauses); tree byte-identical after revert |
| 7 | pluginval s10 ×3 per format + `auval` | ✅ | six runs exit 0, `FAILED` 0, terminal `SUCCESS` ×6; **AU VALIDATION SUCCEEDED** + the six `AUChannelInfo` configs |
| 8 | CI green on the freeze SHA | ✅ | run `31753001642`, `headSha` = `378fb4cd…`, both jobs success |
| 9 | `gen_dbap_reference.py --check` | ✅ **on substance** | `--check OK — 102 cases`. **The gate's recorded spelling does not run — see Issue 1** |
| 10 | Install + dual-variant sweep | ✅ | installed binaries **match the freeze checksums exactly**; only `-dev` variants on disk, no alternate to shadow the AU slot |
| 11 | Re-freeze bit-reproducible | ✅ | third independent full build, both checksums identical |
| 24 | `User/` presets byte-identical | ⚠️ **desk half** | `e3b0c442…7852b855` — the empty-tree hash; `User/` does not exist. Session half cannot be measured |

**P110's invariant re-confirmed independently: 42 + 28 = 70 JS gate sections.**

### Gate 6 — NC1, re-run mutation and revert

The control was executed in full rather than accepted from the record.

| Step | Measured |
|---|---|
| Baseline `meters.js` | `9f121647cc6d34df762f05a7d72314b84893821a5e1ed7a83c3cf05fc7ab6541` — matches the execute record |
| Mutation (delete `:151-152`, the deadline release) | `9827eec982d0b90a…2642bebd`; file is LF-only, 0 CRLF |
| Section 28 under mutation | **exit 2 — 2 FAILED.** `the guard RELEASED on its 165 ms deadline … dropped = 0` and `the poll CONTINUED past it … called null time(s)` |
| Revert | `9f121647…7ab6541` — **byte-identical**, `git status` clean |

**The corrected third clause behaved exactly as `SUMMARY-4.2.md` describes it.** Under the mutation
the non-vacuity clause still passes (`14 getMeters calls in 480 ms`) — because it now claims only
that the stimulus reached the module, which is true in both worlds. The clause that was removed was
the one asserting a *reason* that held only for the shipped guard. **A control that corrects the
positive control is the control working**, and this verify reproduced both halves of that.

### Verified beyond the gate list

Four claims in `SUMMARY-4.2.md` that no gate covers were checked directly, because each is a
known drift hazard in this project:

| Claim | Verified how | Result |
|---|---|---|
| Section 28 reads the deadline **out of the shipped module at run time** | `tests/ui_layout_check.js:1595-1596` parses `METER_POLL_MS` and `GUARD_DEADLINE_TICKS` from `metersSrc` by regex | ✅ **not mirrored** — `pattern_test_fixture_mirrors_drift_silently` does not apply |
| The CR-b fixture is a true 8-cycle over the shipped label set | Fixture labels compared against those parsed from `VenueModel.cpp:92-99` | ✅ same set, **no fixed point**, permutation `2,3,4,5,6,7,8,1`; only `@index`/`@label` present, so the "otherwise identical" claim holds by construction |
| The new tools contain **no RNG** | grep for `import random`, `numpy.random`, `.shuffle(`, `randint` across all three tools | ✅ **0 hits in all three** |
| The analyser's six anti-vacuity clauses are enforced **in the tool** | `die()` sites at `analyse_bounce.py:458, 464, 512, 519, 620, 802, 847` | ✅ all six present and reachable |
| `selftest_analyse_bounce.py` | re-run here | ✅ **24 cases, every clause seen to fire** |

### The section-27 flake fix — re-tested, because it is the one statistical claim in the phase

`SUMMARY-4.2.md` reports a pre-existing flake in section 27 (`UI-04` criterion 2, 3.3's evidence)
failing **~1 run in 5**, traced to `field.js:161` refreshing at most once per 2 Hz status tick — a
recompute owed by an earlier section landing inside section 27's 24-frame drag window
(`pattern_metric_window_vs_modulation_period`). The user-approved, test-only fix opens the drag
window only after the count is stable across three consecutive ticks.

**A claim of the form "fixed, 10/10 green" cannot be verified by reading it.** Re-tested here:

| Runs at this boundary | Section 27 failures |
|---|---|
| Gate 4 (1 run) + NC1 (1 run) + 5 dedicated consecutive runs = **7** | **0** |

Seven consecutive clean runs. Against the reported 1-in-5 base rate, seven clean runs alone would
occur about 21 % of the time by luck, so this verify does **not** claim the flake is proven gone on
its own evidence. Combined with execute's 10/10 it is **17 consecutive clean runs**, which would
occur about 2 % of the time if the original rate still held. **That is good evidence, and it is
stated as evidence rather than as proof** — the honest form for a fix to an intermittent failure.

The assertion itself is unchanged; only its starting state moved. That was confirmed by reading the
section rather than trusting the summary.

### Constraint 6 — no `Source/` edit after the freeze

Verified by diff rather than by assertion:

```
git diff --name-only 378fb4cd..HEAD
  plugins/O-Octagon/.planning/stages/4-polish/SUMMARY-4.2.md
  plugins/O-Octagon/.planning/stages/4-polish/evidence/desk-gates-4.2.txt
  plugins/O-Octagon/tests/tools/selftest_analyse_bounce.py
```

`git diff 378fb4cd..HEAD -- plugins/O-Octagon/Source/` is **empty**. The freeze holds. The one code
file added after it is a **test tool**, which cannot enter the binary — and the bit-reproducibility
check above independently confirms the binary did not move.

---

## Requirements Verification

**Stage/phase:** 4 / 4.2 · **Rows targeted:** `COMPAT-02` (must), `QUAL-01`/2 (must)

| Requirement | Priority | Status | Basis |
|---|---|---|---|
| `COMPAT-02` — Logic Pro, 8 discrete channels | must | ⏸️ **pending, 0 of 3** | All three criteria need a host. Block C did not run |
| `QUAL-01` — no artifacts | must | ✅ complete, **audible clause still bounded** | Clause not concluded; **CU** did not run |
| `COMPAT-01` — VST3/AU load and validate | must | ✅ **re-confirmed on the shipping binary** | Gate 7: six pluginval runs + `auval`, against the frozen bundles |
| `COMPAT-04` — channel configurations | must | ✅ **re-confirmed on the shipping binary** | Gate 2's 95 probes + `auval`'s six `AUChannelInfo` configs |

### `COMPAT-02` — criterion by criterion

| # | Criterion | Status | What is still needed |
|---|---|---|---|
| 1 | Instantiates on a 7.1 surround track | ⏸️ pending | Logic 12.3 + the `getStatus` pre-flight + **save / quit / reopen recall** |
| 2 | Verify-ping reaches 8 distinct physical channels | ⏸️ pending | **CT** — realtime loopback capture. Note N10: it **cannot** be bounced |
| 3 | `srcX/Y/Z` + `w1..w8` visible **and writable** in automation lanes | ⏸️ pending | 11 lanes written and read back, per parameter |

### Ledger — re-counted at this boundary

Counted from `REQUIREMENTS.md`'s own status column, not copied from the summary:

**29 complete · 0 partial · 1 pending — of 30.** `openRows: COMPAT-02`.

**The ledger did not move in 4.2, and the summary says so plainly.** PLAN-4.2's 30/30 was a
close-of-phase target and is not met.

---

## Issues Found

### 1. Gate 9's recorded invocation does not run as spelled — **the one gate defect this verify found**

`PLAN-4.2.md:605` and `evidence/desk-gates-4.2.txt:147` both spell the gate as:

```
gen_dbap_reference.py --check
```

Run exactly as written, that **exits 2**:

```
gen_dbap_reference.py: error: the following arguments are required: --output
```

`--output` is `required=True` (`gen_dbap_reference.py:363`); `--check` is a modifier on it, not a
standalone mode. The working invocation is
`python3 tests/tools/gen_dbap_reference.py --check --output tests/fixtures/DbapReferenceFixture.h`,
which exits 0 with `--check OK — 102 cases`.

**This is the same defect class as VERIFICATION-4.1's Issue 1** (Gate 16b's literal matched nothing
from the day it was written): *a gate whose recorded spelling cannot be re-executed*. The property is
genuinely verified — 102 cases, solver untouched — so this is a **spelling defect, not a behaviour
defect**. But it is the second one in two phases, and the pattern is now established well enough to
name: **a gate command that is transcribed rather than pasted from a run will drift.**

**Disposition:** correct the spelling in `desk-gates-4.2.txt` at the next boundary that touches it, or
carry it into Block C's close. `PLAN-4.2.md` is **not** edited — P109's parting rule is that history
is not rewritten to look correct.

### 2. `STATUS.md` was never updated by the execute phase — **state-tracking, and it misroutes a resume**

`STATUS.md`'s frontmatter still reads:

```yaml
phase: plan
status: phase_complete
next_action: execute_stage_4_phase_4_2
```

But execute **has** run: `SUMMARY-4.2.md` is committed, and Blocks A and B are done and gated.
Confirmed by git — neither execute commit touched the file:

| Commit | Touches `STATUS.md`? |
|---|---|
| `12ddcb12` (4.2 plan artifacts) | yes — **the last update** |
| `378fb4cd` (execute, Block A) | **no** |
| `300d8cf0` (execute summary) | **no** |

**Why this matters more than housekeeping:** a session resuming from `STATUS.md` would read
`next_action: execute_stage_4_phase_4_2` and **re-run execute from Task 1** — re-doing the freeze
that Constraint 6 exists to protect. This is the same hazard D21 was created to close one boundary
ago (*an uncommitted verify is a verify nobody else can see*), reappearing as *an unrecorded phase
transition is a transition nobody else can see*.

**Disposition: FIXED at this boundary.** `STATUS.md` updated below.

### 3. `REQUIREMENTS.md` `lastVerified` is one phase stale — **a recurrence**

Frontmatter reads `lastVerified: stage-4 phase 4.1 (machine gates)`. This was **Issue 3 in
VERIFICATION-4.1** as well. It is a two-line edit that has now been missed twice, which suggests the
close checklist does not name it.

**Disposition: FIXED at this boundary**, and named in the handoff so Block C's close does not miss it
a third time.

---

## What did NOT run — restated at verify, not inherited

Checked against the gate evidence rather than copied from the summary. Every item is a **named
deferral with an owner**, per Stage 4's rule that prose is not a third option.

| Item | Owner | Blocked on |
|---|---|---|
| **Gates 12–25 in full** — `COMPAT-02`/1/2/3, CT, CR-a, CR-b, NC2, NC3, CS, NC4, CU, Gate 13's interactive half, Gate 24's session half, Gate 25 | **Block C** | Logic Pro 12.3, a BlackHole 64ch device, and a human ear |
| `QUAL-01`/2's audible clause | **Block C** (Task 13) | The same session; headphones must be **named** |
| `VenueModel.cpp:87-89`'s "all three containers" prose | **v1.1 doc row** (P107) | Deliberately not in this freeze |
| One specific hardware interface driver (`COMPAT-02`/2's "physical" half) | **none** | No 8-out interface attached; ungeneralisable across interfaces even if present (D11) |
| Windows **UI correctness** | **none** | Hardware. CI's ceiling is pluginval 10 opening the editor without timing out |
| RT-safety beyond allocation (locks, file I/O) | **none** (D10) | `-fsanitize=realtime` unsupported by Apple clang 17.0.0 |
| The two JS gates in CI | **none** | Headless-render determinism, not effort |
| Spatial coherence in a hall | **none** | No requirement row asks for it (D2) |
| `ARCHITECTURE.md`'s three intermediate checksums | **none, permanently** | Unreconstructible from git; recorded once, not to be re-investigated |

**Nine items, nine owners.** Success criterion 13's first half is met.

### The relabelling that is still owed

Gate 23 carries P101's requirement that the hidden-editor check be **recorded as
throttling-recovery**, with the sentence that it *cannot drop a completion*. Gate 23 did not run, so
**the relabelling has not been written into an artifact yet.** It is stated here so the obligation
survives the boundary: the risk PLAN-4.2 named is that the false premise gets inherited a fifth time,
and an unrun gate is exactly how that happens quietly.

---

## Human Verification

Everything below is Block C. None of it has been done.

- [ ] **Gate 12** — surround bounce on BlackHole 64ch produces an 8-channel 24-bit PCM WAV *(STOP-gate)*
- [ ] **Gate 13** — `getStatus` pre-flight: `mapInvalid == false`, `numOutputChannels == 8`, `safeMode == false`, `outputSetName` recorded + screenshot *(STOP-gate)*
- [ ] **Gate 14** — `COMPAT-02`/1: instantiates on 7.1 **and survives save / quit / reopen**
- [ ] **Gate 15** — `COMPAT-02`/3: all 11 lanes written **and read back**, per parameter
- [ ] **Gate 16** — **CT**: realtime loopback ping, eight 1.6 s windows, sequence `1..8`, isolation margin printed
- [ ] **Gate 17/18** — **CR-a** then **CR-b** under the committed 8-cycle, non-identity assertion active
- [ ] **Gate 19** — **NC2** (tool refuses an identity `--expect` on a CR-b label) and **NC3** (a missing tone fails)
- [ ] **Gate 20** — **CS**: LFE on **both** the render and monitor paths, per band
- [ ] **Gate 21** — **NC4**, run **before** any D16 disposition
- [ ] **Gate 22** — **CU**: both halves of D12, **headphones named**, outcome recorded either way
- [ ] **Gate 23** — Gate 13's interactive half; hidden-editor check **relabelled as throttling-recovery**
- [ ] **Gate 24** — `User/` presets re-measured after the session
- [ ] **Gate 25** — `analyse_bounce.py --check` on the committed session artifacts

**Carried into the session unchanged, and worth re-reading before it starts:**

1. **`airAmount = 0` on CR-a, CR-b, CT and CS — never on CU** (Constraint 1). An HF delta from
   `airAmount` reads exactly like bass management and would trigger D16's re-freeze on nothing.
2. **CS runs under the CR-a identity venue.** Under CR-b, speaker 4 is not the LFE slot.
3. **NC4 runs before any D16 disposition.**
4. **Gates 12 and 13 stop the phase** and cost two minutes between them.
5. **No `Source/` edit** — anything that needs one re-enters Block B with a second freeze.

---

## Stage Verdict

**Status: ⚠️ PARTIAL**

**Phase 4.2 achieved the desk half of its goal completely and the host half not at all.**

What is genuinely closed: Q5's mechanism is executed for the first time in five phases and **proven
falsifiable by NC1** — the single most overdue item in the project. Gate 16b is re-spelled and
de-matches its own comment. The re-freeze is recorded, installed, and now **bit-reproducible across
three independent full builds**. `COMPAT-01` and `COMPAT-04` are re-confirmed against the binary that
will actually ship, not inherited from 4.1. Every tool Block C needs — the analyser with its six
clauses enforced in code, a 24-case self-test, deterministic generators, the CR-b 8-cycle — is built,
committed, and verified here to do what it claims.

What is not closed: **`COMPAT-02`, all three criteria.** It is the project's only open row and it was
the row 4.2 existed to close.

**Ready for next stage: No.** Stage 4 has one phase's worth of work outstanding, and the plugin is not
ready for `/install-plugin` or release while its only host-compatibility requirement is unverified.

**Blockers:**

1. **Block C has not run** — 14 gates, `COMPAT-02` 0 of 3, `QUAL-01`/2's audible clause unconcluded.
   Needs Logic Pro 12.3, a BlackHole 64ch device, and a human ear. Estimated 60–90 minutes.

**Not blockers, but owed at the next boundary:**

2. Gate 9's spelling in `desk-gates-4.2.txt` (Issue 1).
3. The Gate 23 relabelling sentence, which no artifact yet carries.

### The freeze, re-confirmed at verify

| Item | Value |
|---|---|
| Commit SHA | `378fb4cdc70ef7e7b4523771dd4f014f189246ec` |
| VST3 bundle binary | `928cd447c57435c93554fbb90fd14ec035cd39e8a8db54a5aba37a1597e0bb42` |
| AU bundle binary | `cc54db026875173e47daf691228c4c80c52da4c9050880aea0976bc16fe1fc99` |
| Probes | **95 / 0 failures** (unit 45, harness 50) — re-run here |
| JS gate sections | **70** (frontend 42 + layout 28) — re-run here |
| Installed as | `O-Octagon-dev.vst3` / `O-Octagon-dev.component`; **no alternate variant on disk** |
| CI run | [31753001642](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/31753001642) — success, both jobs, `headSha` = the freeze commit |
| **Reproducible** | **Yes — three independent full builds, both checksums identical each time** |
| `Source/` moved since the freeze | **No** — verified by `git diff` |

**Block C runs against this.** If it does not, the mismatch is a real signal.

---

## Verification Environment Note

This verify ran from the dedicated worktree at `/Users/taylorbrook/Dev/VST-development-octagon`,
because `feat/o-octagon` is checked out there and the shared checkout at
`/Users/taylorbrook/Dev/VST-development` was on `improve/o-freqpulse-tooltip-measure`. No branch was
switched in the shared checkout — the same courtesy 4.1 verify recorded, and for the same reason.

The build directory was deleted and fully reconfigured as part of Gate 2. Two configure facts worth
carrying, both of which cost time again here:

- **`OUARICON_BUILD_TESTS` defaults OFF.** Without `-DOUARICON_BUILD_TESTS=ON` the two test targets
  are not generated at all.
- **A bare configure of this repo fails in O-Orbit**, whose `libs/SAF/framework` is not checked out.
  `-DSKIP_PLUGINS="O-Orbit"` is the escape and does not touch O-Octagon.

`git status` is clean at the close of this verify; NC1's mutation was reverted and the tree hash
re-measured to prove it.

---

## Next Phase

**Ready for:** **Block C of phase 4.2** — the Logic Pro session — against commit
`378fb4cdc70ef7e7b4523771dd4f014f189246ec` and the two bundle checksums above.

There is **no 4.3** (D18). Block C completes phase 4.2, which completes Stage 4. A D16 finding
re-enters Block B with a second freeze rather than opening a new phase.

`COMPAT-02` is the only requirement row left in the project, and it carries three criteria.
