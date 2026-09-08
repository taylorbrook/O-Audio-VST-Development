---
phase: quick-260908-a8w
plan: 01
subsystem: i18n
tags: [i18n, zh-Hans, glossary, corpus-hygiene, deferred-closure, O-GrainScatter, O-simpleGrain, O-simpleSampler]
status: complete

requires:
  - scripts/i18n-zh-glossary.js
  - scripts/i18n-zh-lint.js
  - scripts/check-i18n.js
  - scripts/i18n-fr-lint.js
provides:
  - "zh-Hans glossary root `dist lpf` = the DISTANCE low-pass"
  - "zh-Hans glossary root `load your own` carries its head noun"
  - "wave-4g ledger: I1 and G4 row 4 closed; carry-forward item 13 opened"
affects:
  - .planning/quick/260907-qda-stage-4-wave-4g-of-the-zh-hans-rollout-l/deferred-items.md

tech-stack:
  added: []
  patterns:
    - "A glossary root derived from ONE plugin's caption+title pair looks measured (2 occurrences) but has no second site to check against — both roots corrected here failed exactly this way."
    - "A `termNote` exempts its entry from BOTH Z5 and F1, so a note written to explain a divergence keeps exempting the row after the divergence is gone."

key-files:
  created:
    - .planning/quick/260908-a8w-fix-the-two-wrong-zh-glossary-roots-dist/260908-a8w-SUMMARY.md
  modified:
    - scripts/i18n-zh-glossary.js
    - .planning/quick/260907-qda-stage-4-wave-4g-of-the-zh-hans-rollout-l/deferred-items.md

decisions:
  - "Kept ONE accepted rendering per key, not two. The distortion reading is not a narrower alternate of the distance reading — it is a different control; and no cell here is width-pinned, so an alternate would have no consumer."
  - "Held every file under `plugins/` out of scope. All four corpus rows already ship the corrected renderings, so the fix costs zero rendering change and users see byte-identical text."
  - "Left the four now-obsolete `termNote` fields in place rather than retiring them — retiring means rewriting three authored header blocks for zero rendering gain. Recorded as ledger item 13 instead."
  - "Did NOT write a `charCount` beside either rendering. The file computes it at require time on purpose ('Derived, never stored'); a written count is the mirrored constant this repo has already been bitten by."
  - "Left the 260907-qda and 260907-ja8 SUMMARY / TASK-SUMMARY files untouched. They are sealed records of completed work; the ledger is the live surface."

metrics:
  duration: ~12min
  completed: 2026-09-08
  tasks: 2
  commits: 2
  files: 2

actuals:
  tokens: 1785
  tasks: 2
  commits: 2
---

# Quick Task 260908-a8w: Fix the Two Wrong zh-Hans Glossary Roots — Summary

Corrected `dist lpf` from the distortion reading to the **distance** low-pass and gave `load your own`
the head noun it was missing, then re-proved all 44 plugins against the corrected glossary at five
unchanged baseline numbers — closing the ledger's oldest open item, carried by five waves because a
shared-script edit falls outside every per-plugin task's path scope.

## What Changed

`scripts/i18n-zh-glossary.js`, two values, no keys, no entries:

| line | key | was | now |
|---|---|---|---|
| 212 | `'load your own'` | a bare possessive, no head noun | `载入自己的素材` (7 chars) |
| 433 | `'dist lpf'` | the distortion reading | `距离低通` (4 chars) |

Each gained a trailing `//` provenance comment in the house style of the `'mid'` entry at line 169 —
naming the corpus site that settled it, the sense it carries, and this task's ID. Neither comment
spells its superseded rendering: writing the old string into the comment would defeat the file-scoped
absence check and leave the wrong rendering greppable in the very file that is supposed to no longer
contain it.

**No file under `plugins/` was touched.** All four corpus rows — O-GrainScatter's `tip.distLpf` and
`label.distLpf`, and the `loadSource` rows in O-simpleGrain and O-simpleSampler — already shipped the
corrected renderings at `reviewed:'bt'`. The text users see is byte-identical before and after.

## Why the Change Could Not Move a Finding

`scripts/i18n-zh-lint.js:420-423` gates Z5 on `!exempt`, and `exempt` is true for any row with a
non-empty `termNote`. All four corpus rows carry one. So the corrected roots now agree with rows the
lint was already not checking — the finding total had to stay at zero, and the fact that it did is
the proof the change is corpus-safe rather than merely non-fatal. Neither key is in `BUDGETS`, so Z6
is inert on both and the 5→7 character growth on `'load your own'` could not fire a budget finding.

## Gate Output (verbatim)

**1. `node scripts/i18n-zh-lint.js --self-test`** — exit 0, zero `BROKEN` lines:

```
SELF-TEST: 10/10
```

**2. `node scripts/i18n-zh-lint.js`** — exit 0:

```
-- summary
  zh-Hans entries checked: 4329   plugins with findings: 0 / 44
  straight copies zh === en (info): 34   termNote exemptions (info): 74
  BELOW SHIP BAR — entries at reviewed:'mt' (machine draft, unchecked): 0
  Z6 coverage: 3 of 552 glossary terms carry a measured budget; 549 are UNBUDGETED and Z6 is inert on them — Stages 2-4 fill these from the check-ui-labels zh arm
  codes: Z1 ASCII punctuation  Z2 U+00A0 before punctuation  Z3 Traditional-only  Z4 Latin/CJK spacing  Z5 glossary  Z6 budget  Z7 full-width Latin  F1 forbidden  R1 reviewed enum  Z8 intra-Han space

GATE PASSED — exit 0. 0 findings across 44 plugin(s).
```

**3. `node scripts/check-i18n.js`** — exit 0:

```
ALL CHECKS PASS — 44 localized plugin(s)
```

**4. `node scripts/i18n-fr-lint.js`** — exit 0:

```
  plugins with findings: 0 / 44
  straight copies fr === en (info): 382, of which 382 are covered (sameAsEn: true, or a title over a translated body)   termNote exemptions (info): 61
  codes: T1 apostrophe  T2 decimal point  T3 % spacing  T4 colon  T5 ;!?  T6 minus  T7 unit  G1 glossary  C1 casing  F1 forbidden word

CLEAN — exit 0
```

Every one of the five baseline numbers in the plan's `<preflight_facts>` reproduced exactly: `0`
findings, `4329` entries, `0 / 44`, `74` termNote exemptions, `3 of 552` Z6 coverage. The unchanged
`74` and `552` are what prove no plugin file and no glossary key moved.

The `MODULE_TYPELESS_PACKAGE_JSON` warning about O-TextureForge appears on stderr in both lint runs.
It is pre-existing, unrelated, and not a finding.

**Structural assertions, all green:**

```
ROOTS OK, DERIVED OK, 552 KEYS          (both roots exact; TERM_META charCount recomputed 4 and 7)
FILE-SCOPED RENDERING CENSUS OK         (superseded form absent; each corrected form present once)
TWO LINES IN ONE FILE, ZERO PLUGIN FILES
RESOLUTION MARKER AT >=4 SITES (4)
G4 COUNT DECREMENTED
I1 RESOLUTION INSIDE ITS OWN SECTION, NOT ORPHANED
ORIGINAL FINDING BODIES AND THE NEW CARRY-FORWARD BOTH PRESENT
```

## Ledger Closure

Four sites in `260907-qda/deferred-items.md` now carry the verbatim marker `RESOLVED 260908-a8w`, one
grep finding all four. Every original finding body survives word for word — the ledger is a history,
so a closed item **gains** a resolution note, it does not lose its body. Diff shape: 27 insertions,
2 deletions, and both deletions are the two next-wave lines that were deliberately reworded.

1. **G4 table** — a resolution paragraph after the table (not inside it), stating row 4 is closed and
   the other five rows are unchanged and still open. The row itself is untouched.
2. **I1 section** — a resolution line appended inside its own body, before `### I2`. Placed with the
   orphaning trap in mind: an evidence line past the next heading reads as belonging to I2.
3. **Next-wave item 2** — prefixed with the marker. It asked for its own commit; that is what it got.
4. **Next-wave item 3** — "five more single-sited roots" → **four**, naming `'load your own'` as the
   one that closed. Its surrounding argument for a glossary-level pass over every root with a corpus
   site count of 1 is untouched and remains the highest-value remaining localization work.

**New carry-forward, item 13:** the four `termNote` fields the fix made obsolete — but **not inert**.
A `termNote` exempts its entry from *both* Z5 and F1, so those four rows now sit outside term coverage
they would pass on merit. O-GrainScatter's file header block also narrates the exemption. Grep-able in
one search: `grep -rnE '距离低通|载入自己的素材' plugins/`.

## Deviations from Plan

**1. [Rule 3 — Blocking] The plan's post-commit diff-shape verifier is unreliable in this shared checkout**

- **Found during:** Task 1, immediately after the commit.
- **Issue:** The plan verifies with `git diff HEAD~1 --name-only`. With no second ref, that diffs
  `HEAD~1` against the **working tree**, not against `HEAD` — so it lists the pre-existing dirty
  `.claude/agent-memory/research-planning-agent.md`, which is another session's file and was never
  staged. Read literally, that reads as a scope violation when the commit is in fact clean.
- **Fix:** Ran the plan's assertion as written (it passed for the parts that matter — `-- plugins/`
  empty, glossary numstat `2 2`) and additionally asserted the commit-scoped
  `git diff HEAD~1 HEAD --name-only`, which returned exactly one file for each commit. Task 2's
  equality assertion was evaluated the same way.
- **Files modified:** None — a verification-method correction, no code change.

**2. [Observation] Another session moved HEAD mid-task**

Commit `148741aa` (`phase(O-Strata): Stage 2 dsp/discuss complete`) landed between this plan's
measured baseline `935a6c71` and Task 1's commit. It carried the `.claude/agent-memory` file. Both
commits here are path-scoped and picked up none of it; `git diff 148741aa HEAD --name-only` returns
exactly the two files this plan declares, and `-- plugins/` is empty. Recorded because a
session-start snapshot of the tree would have been stale by commit time — which is exactly why
CLAUDE.md requires re-checking branch and staging *in the same step as* the commit, and why both
commits here did so.

No Rule 1, Rule 2, or Rule 4 deviations. No authentication gates. No package installs.

## Known Stubs

None.

## Threat Flags

None. No new network endpoint, auth path, file-access pattern, or schema change at a trust boundary —
the only source change is two string literals in a data table.

The plan's registered threats resolved as follows: **T-a8w-01** (shared index) mitigated — both
commits path-scoped naming one file, branch and staging re-checked in the commit step, commit-scoped
diff asserted after each. **T-a8w-02** (key set) mitigated — module-load assertions passed at require
time, 552-key count and both exact root values asserted, glossary diff pinned to `2 2`.
**T-a8w-03** (watchdog) accepted — the 44-plugin walk was backgrounded and read from its log.
**T-a8w-04** (shipped renderings) accepted — `git diff … -- plugins/` empty across both commits.

## Commits

| Task | Commit | Files | Shape |
|---|---|---|---|
| 1 | `03485deb` | `scripts/i18n-zh-glossary.js` | 2 insertions, 2 deletions |
| 2 | `801ef27b` | `260907-qda/deferred-items.md` | 27 insertions, 2 deletions |

Both on `main`, both path-scoped, submodule guard run green before each.

## Self-Check: PASSED

- `scripts/i18n-zh-glossary.js` — FOUND, both roots assert correct via `require()`.
- `.planning/quick/260907-qda-.../deferred-items.md` — FOUND, marker at 4 sites.
- `03485deb` — FOUND in `git log`.
- `801ef27b` — FOUND in `git log`.
