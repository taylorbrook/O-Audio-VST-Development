# Stage 4 — Polish / Validation — Stage Verification (roll-up of 4.1 + 4.2)

**Plugin:** O-Octagon
**Stage:** 4 of 4 — Polish / Validation · **stage-level roll-up — supersedes the 2026-08-14 morning
roll-up (verdict PARTIAL, preserved at commit `b628f4b5`), which predated Block C part 2**
**GSD phase:** verify
**Date:** 2026-08-14 (evening)
**Branch:** `feat/o-octagon` @ `41c1b891`
**Freeze commit under test:** `378fb4cdc70ef7e7b4523771dd4f014f189246ec`
**Worktree:** `/Users/taylorbrook/Dev/VST-development-octagon`

---

## Verdict in one line

**Stage 4 is COMPLETE — ✅ VERIFIED.** The morning roll-up's two blockers are both discharged by
measurement: Block C part 2 ran all nine remaining gates (17–25, recorded in
`evidence/session-gates-4.2.txt` with a close block), and the stage goal's second clause — "with the
bounce path confirmed" — is met by Gate 17's measured bounce order `1,2,3,4,7,8,5,6` at 158.3 dB
minimum isolation, confirmed by Gate 18's before-the-bounce prediction landing exactly. Every
machine-checkable figure was re-measured at this boundary, including a second-person re-derivation
of both transcribed-figure gates (CR-a and NC4) from their checksum-verified WAVs.

All four stages are now complete. The plugin is ready for `/install-plugin`.

---

## Entry Check — contract checksums

Measured here against **`STATUS.md`'s live `contract_checksums` block** (`STATUS.md:1469-1477`),
never against a prior artifact's prose.

| Contract | `shasum -a 256` measured here | Ledger | Result |
|---|---|---|---|
| `BRIEF.md` | `697a4f32890d7420…f6b9fbd6` | `697a4f32…f6b9fbd6` | ✅ **fourteen consecutive phases unmoved** |
| `parameter-spec.md` | `b45f88dc5017ec2c…cbb9e02f` | `b45f88dc…cbb9e02f` | ✅ unmoved since Stage 1 |
| `research/ARCHITECTURE.md` | `2806c788092d9ec9…57bceb17` | `2806c788…57bceb17` | ✅ |
| `ROADMAP.md` | `ea50d991d1a6b158…1063d424` | `ea50d991…1063d424` | ✅ |

**No drift. No contract amended at this boundary.**

---

## Goal-Backward Analysis — the stage goal, not a phase goal

### Stage 4 goal (ROADMAP, as amended by D11)

> **Logic Pro 12.3 on BlackHole 64ch, with the bounce path confirmed.**

### Both clauses now close on measurement

| Clause | Status | Evidence measured or re-derived here |
|---|---|---|
| **Logic Pro 12.3 on BlackHole 64ch** | ✅ **Achieved** | Gates 12–16 (part 1, re-verified at the morning boundary) + gates 17–25 (part 2) all ran on that exact rig against the frozen binary |
| **With the bounce path confirmed** | ✅ **Achieved** | Gate 17 (CR-a) **measured** Logic's canonical interleaved 7.1 bounce order: `1,2,3,4,7,8,5,6` — the WAVEFORMATEXTENSIBLE channel-mask order — at 158.3 dB minimum isolation. Gate 18 (CR-b) confirmed it the strong way: a non-identity, no-fixed-point expectation **derived before the bounce** from the fixture's labels composed with CR-a's map, returned `2,3,4,7,8,5,6,1` exactly. **Re-derived at this boundary** — see below |

**Three measured channel orders now coexist and must never be conflated** (the close block's headline
sentence, re-affirmed here): plugin **buffer** order (= venue table, identity), **device** order
(CT: `1,2,5,6,7,8,3,4` = `Emagic_Default_7_1`), and **bounce** order (CR-a: `1,2,3,4,7,8,5,6` =
WAVEFORMATEXTENSIBLE). The morning roll-up's insistence that the device measurement could not stand
in for the bounce measurement was correct — Gate 17 proved the two orders differ.

### Phase roll-up

| Phase | Verdict | Source |
|---|---|---|
| **4.1 — machine gates** | ✅ **VERIFIED** | `VERIFICATION-4.1.md` — all 18 gates re-run from scratch |
| **4.2 — host-and-ear** | ✅ **VERIFIED** | Blocks A + B re-verified at the 4.2 boundary; Block C part 1 re-verified at the morning roll-up; **Block C part 2 recorded complete in `session-gates-4.2.txt` and re-derived here** |

There is no 4.3 (D18). Block C completed phase 4.2, which completes Stage 4.

---

## Automated Checks — RE-RUN FROM SCRATCH at this boundary

Every figure below was measured here at `41c1b891`, not read out of any summary or gate transcript.

| # | Check | Result | Figure measured **here** |
|---|---|---|---|
| 1 | Contract checksums vs the live ledger | ✅ | four exact; no amendment |
| 2 | Build state | ✅ | `ninja` both targets: up to date, no work, no errors |
| 3 | Probe suites | ✅ | **45 + 50 = 95 probes, 0 failures** |
| 4 | `node tests/ui_frontend_check.js` | ✅ | exit 0 — **42 sections** |
| 5 | `node tests/ui_layout_check.js` | ✅ | exit 0 — **28 sections** |
| 6 | P110 invariant | ✅ | **42 + 28 = 70** JS gate sections |
| 7 | `Source/` moved since the freeze | ✅ **No** | `git diff --name-only 378fb4cd..HEAD -- Source/` is **empty** |
| 8 | Installed VST3 vs freeze record | ✅ | `928cd447…97e0bb42` — **exact match** |
| 9 | Installed AU vs freeze record | ✅ | `cc54db02…6fe1fc99` — **exact match** |
| 10 | Dual-variant sweep | ✅ | `-dev` only on disk; no alternate to shadow the AU slot |
| 11 | **`auval -v aufx OuOc OuDv`** | ✅ | **AU VALIDATION SUCCEEDED** — run at this boundary on the freeze-identical binary |
| 12 | Gate 9, run as the literal recorded string | ✅ | exit 0 — `--check OK — 102 cases` |
| 13 | `selftest_analyse_bounce.py` | ✅ | **24 cases, every clause seen to fire** |
| 14 | **Gate 25 — `--check` on the committed manifest** | ✅ | exit 0, **3 runs re-derived** (Gate 12 probe · CR-b · CS bounce) |
| 15 | **Gate 25 negative control** | ✅ | `--session-root /tmp/nonexistent-octagon` → **exit 1**, so the gate is not vacuous |
| 16 | **Gate 13 banner screenshots** | ✅ | both committed; sha256 **exact** vs the values recorded in the gate entry |
| 17 | **Session WAV integrity** | ✅ | **all 7 recorded sha256s re-verified exact**: `cr-a` `cr-b` `cs-bounce` `cs-nc4` `cs-nc4-air035` `cu-renderA` `cu-renderB` |
| 18 | **Gate 17 / CR-a, fully re-derived** | ✅ | see below — every figure reproduces, including the failure |
| 19 | **Gate 21 / NC4, re-derived at both operating points** | ✅ | see below — exact to 0.01 dB |
| 20 | Ledger recount from the status column | ✅ | **30 complete · 0 partial · 0 pending of 30** |
| 21 | `git status` | ✅ | clean before this verify's own artifact writes |

### CR-a — the transcribed figures were re-derived, and they hold (both ways)

The Block C close decided (Deferral 1) that CR-a stands as a **transcribed-figure gate** — the tool
declines to `--emit-json` a run that failed its own assertion, so its figures live in the gate entry
with the WAV staged and checksummed. The mitigation named there was that "a second person re-derives
every figure from the committed tool." This verify is that second person. Against
`~/Dev/octagon-4.2-session/cr-a.wav` (sha256 `3a8ee8c5…a1c58f51`, **exact match** first):

| Run | Recorded at execute | **Re-derived here** |
|---|---|---|
| As spelled (`--expect 1,2,3,4,5,6,7,8`) | FAILED, exit 1, "observed 1,2,3,4,7,8,5,6" | **FAILED, exit 1, identical mismatch message** |
| Against the measured order (`--expect 1,2,3,4,7,8,5,6`) | — | **exit 0 — `order OK [CR-a] — 1,2,3,4,7,8,5,6, minimum isolation 158.3 dB`** |

**Every figure reproduces exactly, including the recorded failure.** No `--emit-json` was passed on
either run — the committed manifest was not touched (and `--check` still re-derives 3 runs after).
The same disposition CT earned at the morning roll-up now holds for CR-a: "transcribed" understates
what a second person can reproduce.

CR-b and the CS bounce needed no separate treatment — they are **in the manifest**, and check 14
re-derived both from their WAVs at this boundary: CR-b observed `2,3,4,7,8,5,6,1` against its
derived-in-advance expectation at 158.3 dB minimum isolation; CS bounce all ten partials at
delta 0.00 dB, broadband delta 0.00 dB.

### NC4 — both operating points re-derived, exact against the recorded tables

Run here with `--mode lfe --channels 4,1`, no emit, against the checksum-verified WAVs:

| Quantity | Recorded (air 1.00) | **Here** | Recorded (air 0.35) | **Here** |
|---|---|---|---|---|
| 31 Hz delta | −0.00 | **−0.00** | −0.00 | **−0.00** |
| 16 kHz delta | −5.85 | **−5.85** | −2.46 | **−2.46** |
| spread | 5.85 dB | **5.85 dB** | 2.46 dB | **2.46 dB** |
| LF / HF mean | −0.00 / −1.48 | **−0.00 / −1.48** | −0.00 / −0.58 | **−0.00 / −0.58** |

This matters beyond bookkeeping: NC4 is the proof that Gate 20's null (delta 0.00, byte-identical
channels) came from an apparatus that **could** have detected a difference. That proof is now
machine-reproduced at a verify boundary, at the shipped default operating point among others.

---

## Requirements Verification

**Ledger re-counted here** from `REQUIREMENTS.md`'s own status column, not copied from any summary:

**30 complete · 0 partial · 0 pending — of 30.** And unlike at the morning boundary, **the ledger
and the completion signal now agree**: gates 17–25 all ran, so 30/0/0 is no longer ahead of the
evidence. The morning roll-up's Finding 2 — "the ledger reads clean while a `must` requirement
carries an open clause" — is discharged, not waved off: the open clause (QUAL-01/2 audible) was
concluded by Gate 22.

| Requirement | Priority | Status | Basis |
|---|---|---|---|
| `COMPAT-02` — Logic Pro, 8 discrete channels | must | ✅ **complete, 3 of 3** | Gates 14, 15, 16 (part 1, re-derived at the morning boundary) — now with the **bounce-order complement** from CR-a/CR-b in the row's notes |
| `COMPAT-01` — VST3/AU load and validate | must | ✅ complete | 4.1 + 4.2 desk gates on the frozen binary; **auval re-run PASS at this boundary** |
| `COMPAT-04` — channel configurations | must | ✅ complete, 3 of 3 | 95 probes re-run here + auval's six `AUChannelInfo` configs |
| `QUAL-01` — no audio artifacts | must | ✅ **complete — audible clause CONCLUDED** | Gate 22 (CU): machine null-scan at sample resolution, 16.384 s × 8 ch, no discontinuity; gesture proven non-vacuous (HF tilt onset 12.0 s vs 12.1 s predicted from geometry); operator PASS. Monitoring path recorded honestly as weak (MacBook Pro speakers) — the machine half carries the weight |

`REQUIREMENTS.md`'s frontmatter (`lastVerified`, `openRows`) was rewritten at the Block C close and
was checked here against the gate record line by line: accurate, including the honest labelling of
Gate 20's unmeasured loopback delta and Gate 22's monitoring path.

---

## Disposition of the morning roll-up's findings — all five closed or discharged

| # | Finding (morning) | Disposition measured here |
|---|---|---|
| 1 | Gate 13's banner screenshots did not exist | **Committed** (`479aa017`); both sha256s verified exact here; the gate entry records what each image shows plus a genuine negative control (the 3.3 screenshot with banners PRESENT) |
| 2 | 30/0/0 overstated completeness — QUAL-01's audible clause rode an unrun gate | **Discharged by Gate 22**, not by re-labelling: the clause is concluded with a machine half and a human half, and the completion signal (gates 17–25 green) now exists |
| 3 | `ping` `--expect` decision undecided | **Decided at the close** (Deferral 1): CT and CR-a stand as transcribed-figure gates — same structural reason, and both have now been re-derived by a second pass at a boundary. The tool defect stays on the v1.1 register |
| 4 | Gate 23's throttling-recovery relabelling owed | **Written** in the Gate 23 entry with the load-bearing sentence: this UI polls `getStatus` at 2 Hz, so the check **cannot drop a completion** — it exercises throttling *recovery*. The false premise did not survive a fifth boundary |
| 5 | `REQUIREMENTS.md` `lastVerified` was execute-side | Rewritten at the close for Block C part 2; checked here against the gate record — accurate |

---

## Issues Found at this boundary

**None that block.** Two observations recorded:

1. **The session ran exactly as the runbook feared, and the runbook won.** Three of its own defects
   (Gate 18's stale `--expect`, NC4's missing `dHull > 0` precondition, the one-`lfe`-slot manifest
   eviction) were each caught **before** they cost a result. All four tool defects are consolidated
   in the close block and `NOTES.md`'s v1.1 register; none was patched mid-session, and no recorded
   result depends on a stale constant — every gate was graded against derived-in-advance
   expectations. Nothing to fix at this boundary; the register is v1.1 work.
2. **The session WAV directory remains outside the repo and unpinned** (`~/Dev/octagon-4.2-session/`).
   Pre-existing residual, first conceded at CT. This verify re-measured all seven recorded sha256s
   and they are exact, so the archival exposure is unchanged rather than grown: the checksums are
   committed, the bytes are not. If the directory is ever lost, CR-a's and NC4's figures stop being
   re-derivable (CR-b and CS bounce would fall back to the manifest's `--check`… which also reads
   those WAVs — so the manifest's three runs would go dark too). Owner: operator; severity low;
   not a blocker because every figure has now been independently reproduced at two boundaries.

### Residuals carried out of Stage 4 (none is a blocker; none reopens a row)

| Item | Owner | Notes |
|---|---|---|
| Gate 20 realtime-loopback LFE delta unmeasured | operator | Operator-accepted at the boundary; the absolute-level argument is recorded as an argument, not a measurement; the fix is a one-checkbox Software-Monitoring change and a five-minute take |
| Gate 22 audition on revealing monitoring | operator (low) | The sample-resolution null-scan bounds what a better transducer could find |
| D11's physical-interface half | none | Property of an absent machine; criterion keeps its wording |
| v1.1 tool-maintenance register (4 items) | v1.1 | ping-mode hard-coded 1..8 · N13's blind guard + stale refusal message · one-`lfe`-slot manifest eviction · runbook NC4 precondition |
| Session WAVs outside the repo, unpinned | operator (low) | All 7 checksums re-verified exact here |
| Windows UI correctness · RT-sanitizer (D10) · JS gates in CI · spatial coherence in a hall (D2) | none | Permanent residuals, unchanged from the morning roll-up |

---

## Human Verification

All of Block C part 2's human items were performed in-session and are recorded in
`session-gates-4.2.txt` (gates 13 re-capture, 15, 20 disposition, 22 human half, 23 drive):

- [x] Gates 17/18 — CR-a and CR-b under the committed 8-cycle, non-identity assertions active
- [x] Gate 19 — NC2 refusal + NC3 muted-track failure (with an unplanned mapping corroboration)
- [x] Gate 20 — LFE bounce path; loopback disposition recorded as the operator's decision
- [x] Gate 21 — NC4, before any D16 disposition (D16 never invoked)
- [x] Gate 22 — CU, both halves, monitoring path named, outcome recorded
- [x] Gate 23 — interactive drive; relabelling written
- [x] Gate 24 — `User/` re-measured after the drive (empty-input hash; Constraint 9 holds)
- [x] Gate 25 — `--check` on committed artifacts (re-run here, plus negative control)
- [x] Gate 13 re-capture — both banner screenshots committed (Finding 1 closed)

**Nothing remains for a human before install.**

---

## Stage Verdict

**Status: ✅ VERIFIED**

**Ready for next step: Yes — `/install-plugin O-Octagon`.** Stage 4 is the final stage; all four
stages are complete and every requirement row is closed with the completion signal in agreement.

**What Stage 4 closed, end to end.** `COMPAT-04` at 3 of 3 with a measured non-redundancy control;
`COMPAT-01` re-confirmed on the shipping binary, auval re-run green at this boundary; CI running a
test target for the first time in this repo; a binary frozen, installed, and proven bit-reproducible
across three independent full builds. In Logic itself: instantiation on a 7.1 surround track that
survives save/quit/reopen, all 11 automation lanes written and read back, eight speakers on eight
distinct physical device channels at 219.9 dB isolation — and the stage goal's second clause,
**the bounce path, confirmed by measurement**: canonical bounce order `1,2,3,4,7,8,5,6` at 158.3 dB,
predicted-then-confirmed under a permuted venue, LFE byte-identical to an ordinary speaker on that
path, the null's sensitivity proven at two operating points against a forward-derived filter model,
and the last open clause in the project — "does the crossing sound clean?" — concluded with a
sample-resolution machine null and a human pass. Three distinct channel orders measured and named so
they can never again be quietly conflated.

**Blockers: none.**

### The freeze, re-confirmed at the stage close

| Item | Value |
|---|---|
| Commit SHA | `378fb4cdc70ef7e7b4523771dd4f014f189246ec` |
| VST3 bundle binary | `928cd447c57435c93554fbb90fd14ec035cd39e8a8db54a5aba37a1597e0bb42` |
| AU bundle binary | `cc54db026875173e47daf691228c4c80c52da4c9050880aea0976bc16fe1fc99` |
| Installed binaries vs the freeze | **exact match, both** — measured here |
| auval on the installed AU | **PASS** — run here |
| Probes | **95 / 0 failures** — re-run here |
| JS gate sections | **70** (frontend 42 + layout 28) — re-run here |
| Variants on disk | `-dev` only |
| `Source/` moved since the freeze | **No** — `git diff` empty |

The installed `-dev` bundles **are already the frozen, fully-validated binaries** — every session
gate and every re-derivation in this file ran against them. `/install-plugin` formalises the
lifecycle step; it must not rebuild past the freeze without cause.

---

## Verification Environment Note

Ran from the dedicated worktree at `/Users/taylorbrook/Dev/VST-development-octagon`
(`feat/o-octagon`); the shared checkout at `/Users/taylorbrook/Dev/VST-development` stayed on
`main`. `git status` was clean at `41c1b891` apart from this verify's own artifact writes.
Re-derivation runs passed no `--emit-json`; the committed manifest is untouched (confirmed by
`--check` succeeding after them and by `git status`).

One transcription hazard from this verify's own shell, recorded so it isn't repeated: piping a gate
through `tail` makes `$?` report `tail`'s exit, not the tool's — the Gate 25 negative control first
read "exit 0" that way. Re-run without the pipe: **exit 1**, as recorded above
(`pattern_recorded_gate_command_not_executable_as_spelled`'s sibling — verify the exit the way the
gate defines it).

---

## Next Phase

**Stage 4 complete. All stages complete.** Next: `/install-plugin O-Octagon`.

A D16-class finding, if one ever surfaces post-install, re-enters Block B with a second freeze —
there is no 4.3 (D18).
