---
phase: quick-260904-q4j
plan: 01
subsystem: i18n-tooling
status: complete
tags: [i18n, zh-Hans, lint, self-test, stage-4-prep]
requires:
  - scripts/i18n-zh-lint.js at SELF-TEST 9/9 (quick task 260904-f2k)
  - scripts/i18n-zh-glossary.js TERMS/BUDGETS/FORBIDDEN tables
provides:
  - scripts/i18n-zh-lint.js at SELF-TEST 10/10 with 0 repo-wide findings — the
    promotion criterion Stage 4 needs before it can flip the gate to exit 2
  - rule Z8 (intra-Han U+0020), the first rule that can see the defect class
    Stage 3 could only find by reading
affects:
  - Stage 4 of the zh-Hans rollout (owns the gate flip; this task does not)
tech-stack:
  added: []
  patterns:
    - RED-first control observation before every rule change
      (pattern_probe_must_target_the_branch_the_fix_changed)
    - new-lint zero validated against an independent standalone scan
      (pattern_new_lint_zero_must_agree_with_independent_scan)
    - corpus positive control to prove a declared rule is not inert
    - reverse targeted edit, never git checkout, to revert a negative control
      (pattern_negative_control_checkout_wipes_uncommitted_fix)
key-files:
  created: []
  modified:
    - scripts/i18n-zh-lint.js
    - plugins/O-MicrotonalSampler/Resources/ui/js/i18n.js
decisions:
  - D-01 maskLatin widened on two axes (leading-dot extension, end-of-string
    unmatched paren), each with a control observed RED first, and the one
    residual blindness disclosed in the file
  - D-02 Z8 scoped to the PLAIN U+0020 only, so its zero can be checked against
    an independent scan defined on the same predicate
  - Z8 appended LAST in CODES, and the header block and legend appended to
    match, so all three reader-facing enumerations agree with column order
metrics:
  duration: ~35min
  completed: 2026-09-04
actuals:
  tokens: 3605
  tasks: 3
  commits: 4
---

# Quick Task 260904-q4j: Fix i18n-zh-lint before Stage 4 Summary

`scripts/i18n-zh-lint.js` goes from 9/9 self-test with 8 standing repo-wide
findings to **10/10 with 0**, by fixing the two `maskLatin()` gaps that made Z1
contradict Z5 and by adding rule Z8 for the intra-Han space that Stage 3 could
only find by reading. Every change was proven by a control observed RED before
the fix landed. The lint stays REPORT ONLY — the gate flip is Stage 4's.

## The two RED observations, verbatim

These are the load-bearing readings. A summary carrying only the green numbers
would not have demonstrated that either control discriminates.

**Task 1, before the `maskLatin` edit** (fixtures added, rule untouched):

```
  SELF-TEST Z1 BROKEN: fires on a control
SELF-TEST: 8/9
```

The wording is the proof. `fires on a control` — not `silent on a violation AND
fires on a control` — means Controls A and B were pointed at the branch the fix
would change, while Violations C, D and E were already firing and stayed firing.

**Task 2, before `ruleZ8` existed** (code declared in `CODES`, fixture present,
no implementation):

```
  SELF-TEST Z8 BROKEN: silent on a violation
SELF-TEST: 9/10
```

Declaring the code *and* the fixture together was required for this reading: a
code in `CODES` with no `SELF_TESTS` entry prints `NOT YET IMPLEMENTED` and is
skipped rather than counted as broken, which is the weaker signal.

## Before / after

| Measurement | Before | After |
|---|---|---|
| `i18n-zh-lint.js --verbose` finding lines, 43 plugins | **8** (all Z1, all O-MicrotonalSampler) | **0** |
| `--self-test` | 9/9 | **10/10** |
| Rules proven | Z1 Z2 Z3 Z4 Z5 Z6 Z7 F1 R1 | + **Z8** |
| zh-Hans entries checked | 612 | 612 |
| Entries at `reviewed:'mt'` | 0 | 0 |
| `check-i18n.js` | exit 0 | exit 0 |
| `i18n-fr-lint.js` | exit 0 | exit 0 |
| REPORT ONLY banner | present | **present** (untouched) |

The 8 findings that disappeared are exactly the 8 in the plan's baseline table —
6 leading-dot extension shapes (`label.loadScl`, `label.loadKbm`, `label.saveScl`,
`label.saveKbm`, `aria.savePreset`, `aria.loadPreset`) and 2 unbalanced trailing
parens (`label.floTokensBefore`, `label.floTokensBefore2`). No other finding was
silenced: the total was 8 and it is now 0, with every other column still at 0.

## Z8 agreement — both numbers side by side

| Source | Han-space-Han occurrences, all 43 plugins |
|---|---|
| `i18n-zh-lint.js` Z8 column (TOTAL row) | **0** |
| Independent standalone scan (`/\p{Script=Han} \p{Script=Han}/gu`) | **0** |

**The agreement is only load-bearing because the corpus positive control proved
the rule is not inert.** Two zeros agree trivially when one of them comes from a
rule that never runs — and this branch *actually exhibited that state* at commit
`d8684e49`, where Z8 was declared in `CODES`, unimplemented, and its column read
a clean `0` across all 43 plugins, byte-identical to a genuine pass:

```
  plugin                       rows   zh   Z1  Z2  Z3  Z4  Z5  Z6  Z7  F1  R1  Z8   total
  TOTAL                              612    0   0   0   0   0   0   0   0   0   0       0
```

That is threat T-q4j-04 observed live, not argued in the abstract.

## Corpus positive control (Task 2 STEP 4)

| | |
|---|---|
| Plugin | **O-Chorus** |
| File | `plugins/O-Chorus/Source/ui/public/js/i18n.js` line 887 |
| Key | `label.hoverHelp` |
| Before | `悬停帮助` (U+60AC U+505C U+5E2E U+52A9) |
| After | `悬停 帮助` (U+60AC U+505C **U+0020** U+5E2E U+52A9) |
| Result | `--plugin O-Chorus` read **Z8 = 1** through `lintPlugin`, not through the fixture path |

The injection also tripped **Z5 = 1**, because `悬停 帮助` is no longer the
glossary's accepted rendering of `Hover help`. That was not planned for, and it
is corroboration the mutation was real rather than a display artifact.

**O-Chorus resolves to `Source/ui/public/js/i18n.js`, NOT the `Resources/ui/js/`
sibling.** The plan warned against assuming; the warning was warranted.

**Revert, and proof of no residue.** Reverted by a reverse targeted edit — never
`git checkout --` or `git restore`, because Task 2's own uncommitted work on
`scripts/i18n-zh-lint.js` was in the same tree (T-q4j-01,
`pattern_negative_control_checkout_wipes_uncommitted_fix`). Proven two ways:

- `shasum -a 256` of the file after revert is `52bfca0c03da3f1caa8c5e2bac7a6889a963e1f2236dc4b71eb34dd811598bca`, **byte-identical** to the digest taken before the injection.
- `git diff --stat -- plugins/` printed **0 lines**.

## The `maskLatin` blindness, disclosed

Widening a mask can only *reduce* findings, so the widening's cost is stated in
the file itself rather than left silent — the same house style as the Z6
coverage disclosure:

> A Han character followed **immediately** by an ASCII period and then Latin with
> no space between (e.g. `混音.mix`) now reads as an extension token, and its
> period goes unseen.

That shape does not occur in the corpus and would be a typing slip rather than a
typography choice. The wider, spaced form `混音. Mix` still fires, because the
period there is followed by a space. Three violation fixtures hold the line
around the widening:

- **D** `混音, 深度.` — genuine Han-prose comma and period; the leading-dot mask must not swallow them.
- **C** `混音 ( 深度` — an unmatched open paren *not* at end of string; the end-of-string mask must not reach it.
- **E** `混音 (深度)` — a balanced ASCII pair with Han inside; the new branch must not reach around it and half-consume the pair.

Violation E was declared in the plan's `<behavior>` block but not named in STEP 1's
literal instruction list; it was added, because the behavior block is the contract
and E is the discriminator that keeps the balanced branch honest.

## Deviations from Plan

### 1. [baseline correction] The `.scl`/`.kbm` plugin census is 7, not 8

The plan's baseline recorded **8** plugins carrying `.scl`/`.kbm` in their i18n
table — O-Bells, O-Contrabass, O-Formant, O-IntonationPad, **O-Lyrica**,
O-Marimba, O-MicrotonalSampler, O-Prism — and instructed that 8, not the brief's
12, was the number to report. Re-measured live at execution time, under the same
definition (the token appearing anywhere in the plugin's `i18n.js`), the count is
**7**. **O-Lyrica carries no `.scl`, `.kbm` or `scala` mention anywhere in
`plugins/O-Lyrica/Resources/ui/js/i18n.js`**, and its file has not been touched
since `6e4a1566` (the Infobulles rollout), well before planning — so this is a
planning-time miscount, not drift caused by this task.

The number is also definition-sensitive, which is worth pinning for Stage 4:

| Definition | Count | Plugins |
|---|---|---|
| `.scl`/`.kbm` anywhere in the `i18n.js` file, any language | **7** | O-Bells, O-Contrabass, O-Formant, O-IntonationPad, O-Marimba, O-MicrotonalSampler, O-Prism |
| in a *rendering* of any language | **3** | O-Contrabass, O-IntonationPad, O-MicrotonalSampler |
| in a **zh-Hans** rendering (the only one Z1 can reach) | **1** | O-MicrotonalSampler |

No impact on the fix. Every one of the 8 Z1 findings was on O-MicrotonalSampler
under any of these definitions, and all 8 are gone. Reported because the plan
explicitly asked for the live count and the live count moved.

### 2. [in-plan, flagged by the plan itself] The 2 unbalanced-paren findings

The brief named six leading-dot renderings; the live corpus had eight findings.
The extra two (`label.floTokensBefore`, `label.floTokensBefore2`, both
`否则使用文件名标记 (`) were fixed here under D-01 with their own RED-first
control, exactly as the plan's "In scope beyond the literal brief" section
directed. Not a silent scope slip.

No auto-fixes under deviation Rules 1–3 were needed; no Rule 4 architectural
decision arose.

## Authentication gates

None.

## Task 3 — adjacent-gate regression sweep

The O-MicrotonalSampler header note recorded the Z1 findings as a live, unfixed
tool defect. The diagnosis prose is kept — it is the durable part and it is what
let the fix be surgical rather than a guess — and only the disposition is
rewritten to say the defect was fixed in `scripts/i18n-zh-lint.js` under this
task. **Comment-only: every changed line in that diff begins with `//`.** No `en`,
`fr` or `zh-Hans` rendering moved anywhere in `plugins/`.

Run rather than assumed:

| Gate | Reading |
|---|---|
| `i18n-zh-lint.js --self-test` | `SELF-TEST: 10/10` |
| `i18n-zh-lint.js --verbose` | 0 finding lines, 43 plugins |
| `check-i18n.js` | exit 0 |
| `i18n-fr-lint.js` | exit 0 |
| `git diff --name-only -- plugins/` | `plugins/O-MicrotonalSampler/Resources/ui/js/i18n.js` and nothing else |

## Known Stubs

None. No `TODO`, `FIXME` or placeholder marker was introduced; both new rules are
fully implemented and each is proven by a fixture pair plus, for Z8, a corpus
positive control.

## Threat Flags

None. No new network endpoint, auth path, file access pattern or schema change.
The one deliberate mutation of shipped plugin source (T-q4j-01) was reverted
byte-exactly and proven so two independent ways.

## TDD Gate Compliance

Task 2 carried `tdd="true"` and the gate sequence is intact in git log:

| Gate | Commit | Reading |
|---|---|---|
| RED | `d8684e49` `test(...)` | `SELF-TEST Z8 BROKEN: silent on a violation`, 9/10 |
| GREEN | `97816a94` `feat(...)` | `SELF-TEST: 10/10`, Z8 proven |
| REFACTOR | — | not needed; no cleanup pass changed behavior |

Task 1 was a `type="tracer"` task. Its feedback gate ran in interactive mode with
`human_verify_mode: end-of-phase` and an automated-only `<verify>` block, so the
verify was re-run end-to-end (4/4 pass) and execution continued to the expansion
tasks without a checkpoint, per the tracer gate rules.

## Commits

| Commit | Type | What |
|---|---|---|
| `ec20d0b1` | `fix` | Task 1 — Z1 controls A/B + violations C/E, `maskLatin` widened on both axes, blindness disclosed. 8 → 0. |
| `d8684e49` | `test` | Task 2 RED — Z8 declared in `CODES` with its fixture, unimplemented. 9/10. |
| `97816a94` | `feat` | Task 2 GREEN — `ruleZ8`, its `lintRows` push, header block and legend. 10/10. |
| `18c7f375` | `docs` | Task 3 — O-MicrotonalSampler stale note retired, comment-only. |

All four path-scoped (`git commit -- <paths>`), with `git branch --show-current`
and `git status --short` re-checked immediately before each, and the submodule
guard run on each staged set (PASS every time). No `git add -A`, no `git commit -a`.

## For Stage 4

- **The promotion criterion is now met**: 10/10 self-test, 0 repo-wide findings.
  The gate flip (exit 2) and the removal of the REPORT ONLY banner are Stage 4's
  commit, deliberately not made here.
- **Z8 is scoped to the plain U+0020 only.** U+00A0, U+2009, U+200A and U+3000
  between two Han characters are each defensible additions and each deliberately
  excluded, so Z8's zero stays checkable against a scan defined on the same
  predicate. If Stage 4 widens it, the independent-scan agreement test has to be
  redefined in the same commit or the validation goes vacuous.
- **Z8 sits LAST in `CODES`**, so it is the last column, the last legend entry
  and the last line of the header `── The checks ──` block. All three
  enumerations were updated together; they must stay in step, because the table
  header, separator width and totals row all derive from `CODES.length`.
- **The `.scl`/`.kbm` census is 7 under the file-grep definition and 1 under the
  zh-Hans-rendering definition** (see Deviation 1). Whichever Stage 4 needs, it
  should re-measure rather than inherit a number.

## Self-Check: PASSED

Files claimed modified, verified present and committed:

- `scripts/i18n-zh-lint.js` — FOUND
- `plugins/O-MicrotonalSampler/Resources/ui/js/i18n.js` — FOUND

Commits claimed, verified in `git log`:

- `ec20d0b1` — FOUND
- `d8684e49` — FOUND
- `97816a94` — FOUND
- `18c7f375` — FOUND
