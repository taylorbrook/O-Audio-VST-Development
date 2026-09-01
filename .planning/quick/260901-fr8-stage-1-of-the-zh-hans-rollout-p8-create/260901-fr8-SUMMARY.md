---
phase: quick-260901-fr8
plan: 01
subsystem: i18n tooling
status: complete
tags: [i18n, zh-Hans, glossary, lint, back-translation, opencc, tooling]
dependency_graph:
  requires: [quick-260901-c3s]
  provides:
    - scripts/i18n-zh-glossary.js
    - scripts/i18n-zh-lint.js
    - scripts/i18n-zh-backtranslate.js
  affects: [Stage 2 pilot O-Chorus, Stages 3-4 per-plugin zh dispatch]
tech_stack:
  added: []
  patterns:
    - "report-to-gate lifecycle: exit 0 until the pilot is at zero findings"
    - "self-test as proof of implementation: a rule that cannot be shown to fire is decorative"
    - "derived-not-stored: charCount computed at require time, never written beside the rendering"
    - "network data crosses into source only as a derived, provenance-stamped literal"
key_files:
  created:
    - scripts/i18n-zh-glossary.js
    - scripts/i18n-zh-lint.js
    - scripts/i18n-zh-backtranslate.js
  modified:
    - .planning/STATE.md
decisions:
  - "TERMS keeps the French glossary's value type (EN key -> array of renderings, root first); BUDGETS and SAME_AS_EN are separate exports so Stage-2 tooling can consume fr and zh through one code path."
  - "EQ stays OUT of SAME_AS_EN: 均衡 is real and readable, so 'eq' lives in TERMS with 'EQ' as an accepted alternate."
  - "轨道 was REJECTED from FORBIDDEN_IN_LABELS: it is the wrong sense of Track but the RIGHT word for O-Orbit, and buying one catch elsewhere would cost a termNote on every O-Orbit entry."
  - "发布 is forbidden in LABELS only, not PROSE: 'v1.2 发布' is a correct sentence about a software release."
  - "Three single-site terms (direct, hold, diffusion) were settled beyond the measured 551 and are disclosed as such in the file header."
metrics:
  duration: ~50min
  completed: 2026-09-01
  tasks: 3
  commits: 3
actuals:
  tokens: 41000
  tasks: 3
  commits: 3
---

# Quick Task 260901-fr8: zh-Hans Rollout Stage 1 (P8) Summary

Three report-only zh tools now exist and the 551-term shared-string glossary is settled
**before** any translator is dispatched — the ordering the French rollout did not get.

## What Landed

| Task | Commit | What |
|------|--------|------|
| 1 (tracer) | `68cd81c6` | End-to-end skeleton: all three files, Z5 and R1 proven, 43-plugin vacuity run |
| 2 | `34211ef7` | The remaining seven rules, OpenCC-derived Z3 set, back-translation independence |
| 3 | `6d52bb04` | Full 552-term glossary, Z6 coverage disclosure, five rendering corrections |

`.planning/STATE.md` was updated but deliberately **left uncommitted** — the orchestrator
owns the docs commit (execution constraint overrode the plan's Task-3 STATE.md commit step).

## Headline Results

- **`SELF-TEST: 9/9`** — every rule (Z1 Z2 Z3 Z4 Z5 Z6 Z7 F1 R1) fires on a deliberate
  violation and stays silent on a clean control. No rule is decorative.
- **Vacuity, not a pass.** All three tools over 43 plugins:
  `VACUITY: 0 zh-Hans entries found across 43 plugins — nothing was checked.`
  followed by `This is not a pass.` in the tool's own words. All exit 0.
- **Glossary: 552 terms, 3 budgeted, 549 unbudgeted.** The lint prints
  `Z6 coverage: 3 of 552 glossary terms carry a measured budget; 549 are UNBUDGETED and
  Z6 is inert on them — Stages 2-4 fill these from the check-ui-labels zh arm`.
- **Regression guards green:** `check-i18n` ALL PASS 43/43 (exit 0), `i18n-fr-lint` exit 0
  (a real gate — a non-zero there would be a hard failure). Zero files under `plugins/`.
  No `package.json`, no `node_modules`, no install of any kind.

## The Corpus Re-measurement

Regenerated live at execution time by the same dynamic-ESM walk the lint uses — not copied
from the plan:

| Fact | Value |
|---|---|
| plugins with `i18n.js` under either UI root | 43 / 43 |
| entries needing a zh value | 3789 (2367 label + 1422 title) |
| unique short strings | 1802 |
| shared strings (>1 site) | **551**, covering **2538 / 3789 = 67.0 %** |

Identical to the planning-time table. No drift, nothing papered over.

## Rule Design Notes Worth Keeping

**The controls are where the rules are hard, not the violations.** Several rules carry more
than one control because a single one would let a broken rule pass:

- **Z1** must stay silent on `延迟 (delay) 20 ms` and on `截止频率 1.5 kHz，范围 20 Hz-20 kHz。`
  A Latin/unit-token mask runs first: parenthesised asides containing no Han are removed
  whole, then Latin/number tokens with any ASCII punctuation *between* two alphanumerics.
- **Z4** is two rules in one. The Latin/Han spacing consistency half is **table-scoped** —
  neither the spaced nor the unspaced form is wrong on its own, only the mixture is — so the
  finding lands on the minority-form entries. The thin-space half (U+2009/U+200A) fires
  unconditionally, on the same reasoning that chose U+00A0 over U+202F for French.
- **Z6** must stay silent on an UNBUDGETED key at *any* length. Inertness is the design.
- **Z7** must stay silent on the full-width **punctuation** that Z1 *requires*.
- **F1** must stay silent on a rendering the glossary itself accepts, and on a `termNote`
  exemption — both carried from the French precedent verbatim.

**F1 tests containment, not a word stem.** Chinese has no word delimiter, so the French
stem/lookahead machinery has nothing to anchor to. Two consequences are recorded in the
glossary header for the next editor: a forbidden rendering that is a substring of a correct
one (混音 ⊂ 混音器, covered by the glossary-accepted escape hatch), and a word that is
genuinely right somewhere in the suite (轨道, rejected for that reason).

**Not ported, deliberately:** French T1–T7 (Z2 is the exact inverse of three of them) and C1
(Han has no case; `text-transform: uppercase` is a no-op). `--codes` emits exactly
`Z1 Z2 Z3 Z4 Z5 Z6 Z7 F1 R1` and no French code.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] OpenCC comment lines counted as dictionary keys**
- **Found during:** Task 2, first generator run
- **Issue:** The first derivation produced **5047 characters / 77 KB** against the
  planning-time measurement of 4139 / 12.8 KB. Cause: the served dictionaries carry a `#`
  header *plus* 895 `# @tofu-risk:` annotation lines, and the parser was treating each as a
  key. 77 KB at 15.3 bytes per "character" is arithmetically impossible for CJK — that
  inconsistency is what exposed it.
- **Fix:** Filter lines beginning with `#`; additionally assert every TS key is a single code
  point and stop if one is not (a multi-character key in a *character* table means the parse
  is wrong). Re-derived: **4139 characters, 12773 bytes** — matching the planning measurement
  exactly, with all four spot-checks passing (這 IN, 後 IN, 这 OUT, 音 OUT).
- **Also fixed:** the in-file provenance block now records the comment-filter rule explicitly
  and says it is load-bearing, so the derivation is reproducible without the throwaway
  generator (which, per plan, is not committed).
- **Commit:** `34211ef7`

**2. [Rule 1 - Bug] Five wrong or under-specified glossary renderings**
- **Found during:** Task 3 human-check spot-read, by opening the actual label sites rather
  than trusting the English key
- **Issue and fix:**
  | Key | Was | Now | Why |
  |---|---|---|---|
  | `dec` | 衰减 (decay) | 解码 | O-Gain's DEC is M/S **DE**code; its English body says "Decode Mid/Side back to L/R" |
  | `sub oscillator` | 低频振荡器 | 副振荡器 \| 次低音振荡器 | 低频振荡器 is the standard Chinese rendering of **LFO** — it named the wrong oscillator entirely |
  | `sub` | 低频 \| 次 | 低音 \| 超低频 | same collision |
  | `mid` | 中 | 中频 \| 中置 \| 中 | two senses: the mid **band** (O-Formant, O-FreqPulse) and the M/S **MID channel** (O-MultiBandCompressor) |
  | `oct` | 八度 | 八度 \| 八声道 | O-Orbit's `label.fmtOct` is the **octophonic format**, not an octave (French renders it Octo) |
- **Commit:** `6d52bb04`

### Documented Departures

**3. Rules were implemented alongside their declaration in Task 1, not stubbed.**
The plan said to stub Z1 Z2 Z4 Z6 Z7 F1 in Task 1 and implement them in Task 2. In practice
the rule bodies were written in Task 1 and only their **self-test pairs** were added in Task
2. Task 2's TDD loop is therefore honest about the *proof* mechanism (the pairs were written
first and Z3 was genuinely red at `8/9` before its character set existed) but was not a red
loop for the other six rule bodies. No behavioural difference — Task 1's 43-plugin run had no
zh rows for any rule to act on — but the SUMMARY says so rather than claiming a full red/green
cycle for all seven.

**4. STATE.md was updated but not committed.** The plan's Task 3 ends with a separate
STATE.md commit; the execution constraints forbid the executor from committing docs artifacts.
The constraint won. STATE.md is modified in the working tree for the orchestrator to commit.

**5. Three terms beyond the measured 551.** `direct`, `hold` and `diffusion` are single-site
today and were settled anyway, each being an obvious near-neighbour of a term that *is* shared.
Disclosed in the glossary header rather than left for a reader to discover.

### No Architectural Changes

No Rule 4 situation arose. No npm dependency was needed — the Z3 route is a one-time data
fetch whose derived output is embedded, with zero runtime dependency, exactly as planned.

## Threat Mitigations Applied

| Threat ID | Mitigation as shipped |
|---|---|
| T-fr8-01 (OpenCC fetch tampering) | Both upstream sha256 recorded in-file; TSCharacters matched the planning-time value byte for byte. Four spot-check characters asserted by the generator, which throws rather than emit a set that fails them. Fetched files never committed, never executed — only derived character data crosses the boundary. |
| T-fr8-02 (shared git index) | `git branch --show-current` + `git status --short` re-checked immediately before each of the three commits. Every commit `git add <paths>` then `git commit -- <paths>`. No `git add -A`, no `git commit -a`. Submodule guard run before each commit; never tripped. |
| T-fr8-03 (emit batch disclosure) | Accepted as planned. `--out` has no default and the tool refuses to guess a write path. |
| T-fr8-04 / T-fr8-SC (npm) | No install occurred. No root `package.json`, no `node_modules`; asserted by all three task verifications. |
| T-fr8-05 (unrecorded back-translation) | `--ingest` refuses a missing `--provenance` **and** one byte-identical to the forward pass recorded in the emit manifest. Both branches exercised live; both print `REFUSED: ... this triple proves nothing` and exit 0. |

## Known Stubs

None. Every declared rule is implemented and proven. `FORBIDDEN_IN_LABELS` (16 entries) and
`FORBIDDEN_IN_PROSE` (5) are populated; `BUDGETS` holds only the three measured cells by
design, and that inertness is printed on every run rather than left silent.

## What Stage 2 Inherits

- A settled glossary, so 43 reviewers cannot each pick differently.
- A lint that is a **report**, so a half-built rule cannot block the pilot. Promotion to a
  gate (exit 2) is the explicit next step, once O-Chorus is at zero findings.
- 549 unbudgeted terms waiting on the `check-ui-labels` zh arm to measure their cells.
- A back-translation harness whose independence is enforced mechanically rather than trusted.

## Self-Check: PASSED

- `scripts/i18n-zh-glossary.js` — FOUND
- `scripts/i18n-zh-lint.js` — FOUND
- `scripts/i18n-zh-backtranslate.js` — FOUND
- commit `68cd81c6` — FOUND
- commit `34211ef7` — FOUND
- commit `6d52bb04` — FOUND
