---
task: 260901-akh-research-and-plan-chinese-localization-a
type: execute
mode: quick
status: complete
completed: 2026-09-01
subsystem: i18n / documentation
tags: [i18n, localization, chinese, zh-Hans, documentation, research]

dependency_graph:
  requires:
    - ".planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-RESEARCH.md"
    - ".planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/ (French rollout prior art)"
  provides:
    - "research/i18n-zh-hans-localization.md — durable zh-Hans reference"
    - ".planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md — 6-stage executable plan"
  affects:
    - "Any future zh-Hans rollout stage (Stage 0 through Stage 5)"

tech_stack:
  added: []
  patterns:
    - "Research documents live in research/, never docs/ (CLAUDE.md project rule)"
    - "Path-scoped commits: git add <new file> then git commit -- <same path>"

key_files:
  created:
    - research/i18n-zh-hans-localization.md
    - .planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
  modified: []

decisions:
  - "zh-Hans is both the i18n table key and the document.documentElement.lang value — one string, two jobs, and zh-Hant slots in later with zero renaming"
  - "The reviewed flag becomes a per-language enum 'mt' | 'bt' | 'native' for zh only; fr keeps its boolean at BOTH assertion sites"
  - "Ship bar is 'bt' (developer read an independent back-translation), not 'native' — this is the one decision that gives the work a termination condition"
  - "CORRECTION to RESEARCH §5.1 P6: check-ui-labels.js has SIX ['en','fr'] loops (598, 658, 669, 703, 717, 851), not seven — line 635 is the EN->EN control-spread block"
  - "CORRECTION to RESEARCH §5.1 P5: there are TWO reviewed-flag assertions — check-i18n.js:576-578 (tooltips) and check-i18n.js:623 (LABELS)"

metrics:
  duration: "~25 min"
  tasks: 2
  commits: 2
  files_created: 2
  files_modified: 0

actuals:
  tokens: 17600
  tasks: 2
  commits: 2
---

# Quick Task 260901-akh: Chinese (zh-Hans) Localization Research + Plan — Summary

Two documentation artifacts committed: a durable `research/` reference answering *how* Chinese
localization is realized across the 43-plugin suite, and a six-stage implementation plan whose every
stage can be handed to a `/gsd-quick` executor as written. Zero code changed.

## What was built

### Task 1 — `research/i18n-zh-hans-localization.md` (35,169 bytes) — commit `94d1eafd`

The quick-task research promoted into the project's durable research directory, per the CLAUDE.md
rule that research goes in `research/` and not `docs/`. Carries house frontmatter matching
`research/cross-platform-webview-best-practices.md` (`title`, `created`, `last_verified`, `summary`,
`domain: ui`, `type: research`, `keywords`, `stages: [3]`) and all eleven required `##` sections:
recommendation summary, current state, variant/schema key, fonts and typography, width geometry,
schema and runtime changes, translation production and review model, repo-specific pitfalls,
rollout shape, assumptions log, sources.

Quick-task scaffolding stripped (no "this session", no task id in prose); every `file:line`
citation and every `[VERIFIED]` / `[CITED]` marker preserved — those are the document's value.

The reader-facing payload: a newcomer can decide the variant (`zh-Hans`, Simplified first), the
schema key (one string that is both table key and `document.documentElement.lang`), the font policy
(no bundled webfont exists anywhere, so there is no silent-fallback hazard; append a CJK tail to
the ~10 stacks localized nodes resolve through, not all 450), the geometry law
(`zh width = charCount x font-size`, exactly, inverting to `maxChars = floor(cellWidthPx / fontSizePx)`)
and the review model — without opening the quick-task directory.

### Task 2 — `260901-akh-IMPLEMENTATION-PLAN.md` (35,192 bytes) — commit `038bdedd`

Opens with a six-row **Decisions to confirm before Stage 0** table — variant, schema key, review
model, ship bar, pilot, and the four unserved draft files — each with a recommendation and the
consequence of the other branch. Decision (d), ship at `'bt'` versus hold for a native reader, is
named explicitly as the single choice that determines whether the work has an end.

Six stages follow, each carrying the five bold fields in order — `Files touched`, `Gates`,
`Commit scope`, `Size`, `Invocation` — with the invocation being copy-paste `/gsd-quick` text and
the commit scope a literal path-scoped `git commit --` line (with the preceding `git add` shown
wherever a file is new):

| Stage | Content | Scale |
|---|---|---|
| 0 | Repo-wide prerequisites P1-P8, each with file:line, exact edit, verification command, ordering constraint | ~47 files, 2 commits |
| 1 | Glossary (543 shared strings / 64% coverage, character budget per term) + Z1-Z7 lint + back-translate, all as reports first | 3 new scripts |
| 2 | Pilot O-Chorus, 7 numbered items including the `line-height` audit and the px-cliff -> character-budget conversion | 1 plugin, ~28 entries |
| 3 | Hard cases: O-Octagon, O-Bitrot, O-MicrotonalSampler — why each is hard, what each teaches | 3 plugins, ~518 entries |
| 4 | 7 volume waves of 5-6, each mixing one heavy with several lights; per-plugin approx. entry table with the UI-root split marked | 39 plugins, ~3,187 entries |
| 5 | Repo-wide QA and the definition of done — `'mt'` fails, `'bt'` ships, `'native'` stays open | 43 plugins verified |

Closes with a six-row risk list (WKWebView line-height re-measurement per A5, the sub-9px
legibility tier as a disclosed limitation, the O-MultiBandCompressor whitespace outlier that a
sed sweep skips silently, the four draft files, the shared-checkout index race, and a
back-translation that is not independent) and an effort-by-stage table totalling ~54 commits,
43 builds, ~11-14 sessions.

## Corrections carried into both documents

Two facts in `260901-akh-RESEARCH.md` were wrong; correcting them was an explicit goal of this task,
and both were re-verified against the tree before writing.

**1. `check-ui-labels.js` has SIX `['en','fr']` loops, not seven.**
Measured: `598, 658, 669, 703, 717, 851`. RESEARCH §5.1 P6 additionally listed **635**, which sits
inside the EN->EN control-spread block (`const enControl = []` and its preceding comment) — not a
language loop. Editing it would corrupt the assertion-2 vacuity control.
```
$ grep -n "\['en', *'fr'\]" scripts/check-ui-labels.js
598:        for (const lang of ['en', 'fr']) {
658, 669, 703, 717, 851 — same form
```

**2. `check-i18n.js` has TWO `reviewed`-flag assertion sites, not one.**
RESEARCH §5.1 P5 named only the tooltip assertion at `:576-578`. The LABELS analogue is at
**`check-i18n.js:623`**:
```js
typeof ((LABELS_EARLY[k] && LABELS_EARLY[k].fr) || {}).reviewed !== 'boolean');
```
Patching only the tooltip site would leave all 2,367 zh *label* entries failing assertion [5] — a
failure that surfaces only once translation begins. Both documents now name both sites.

## Deviations from Plan

**None.** Both tasks executed exactly as written. No Rule 1-4 deviation was triggered.

One observation worth recording, which did not require a deviation: the plan's verification step 3
names baseline `9eb8564b`, but HEAD at the start of execution was `03b7d3c7` (the docs commit that
landed RESEARCH.md and PLAN.md themselves). The check was run against both baselines and printed
`0` for each, so the assertion holds either way.

## Verification

All four verification steps from the plan passed.

| # | Check | Result |
|---|---|---|
| 1a | Task 1 structural check (11 headings, 5 frontmatter keys, 5 anchors, >12 KB) | `research doc OK, 35169 bytes` |
| 1b | Task 2 structural check (7 sections, 5 fields x >=6, P1-P8, 8 anchors, >14 KB) | `implementation plan OK, 35192 bytes` |
| 2 | Scope fence — `git status --porcelain` filtered to non-allowed paths | `0` |
| 3 | Gates untouched — `git diff --stat 9eb8564b..HEAD -- scripts plugins` | `0` (also `0` from `03b7d3c7`) |
| 4 | Location — `git branch --show-current` / `git status --short` | `main`, clean |

Submodule commit guard (`plugins/O-Orbit/libs/SAF`) ran before each commit and reported `GUARD OK`;
each commit staged exactly one file.

## Commits

| Task | Commit | Files |
|---|---|---|
| 1 | `94d1eafd` | `research/i18n-zh-hans-localization.md` |
| 2 | `038bdedd` | `.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md` |

## Known Stubs

None. Both deliverables are complete documents; nothing is placeholdered and no section defers to a
later pass.

## Scope compliance

Zero files changed under `plugins/`, `scripts/`, or any gate. No version bump, no CHANGELOG entry,
no `PLUGINS.md` row, no build, no install. The eight prerequisite edits, the three new zh scripts
and the 43-plugin sweep are all *planned* here and *executed* nowhere.

## Self-Check: PASSED

- `research/i18n-zh-hans-localization.md` — FOUND
- `.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md` — FOUND
- commit `94d1eafd` — FOUND in `git log`
- commit `038bdedd` — FOUND in `git log`

## Next Up

**Step 1:** `/clear`

**Step 2:** Confirm the six decisions at the top of the implementation plan — especially (d), ship
at `'bt'` or hold for a native reader — then:
```
/gsd-quick Stage 0 of the zh-Hans rollout: make the i18n gates language-agnostic. Follow
.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
"Stage 0 — Repo-wide prerequisites", items P1 through P7.
```

**Alternatives:**
- Read `research/i18n-zh-hans-localization.md` first if the *why* behind any decision is unclear.
- Shelve the rollout entirely — both documents are durable and carry a `Valid until: ~2026-10-01`
  marker; re-run the counts if `scripts/i18n-*.js` changes before work resumes.
