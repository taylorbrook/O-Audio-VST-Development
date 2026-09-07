# 260907-bgx — Deferred items

Recorded at the end of the deletion sweep. Nothing here was touched by this task.

## 1. The 4 sibling-only plugins — ONE decision for the user

These four have **no** `i18n-index-draft.html` — it was already removed from each of them in
earlier work (they are the clamp-carrying four). What remains is the other two thirds of the
same throwaway triple:

| Plugin | `i18n-labels-skeleton.js` | `i18n-inventory.tsv` | Tracked? |
|---|---|---|---|
| O-AnalogSaturation | present | present | yes |
| O-Bitrot | present | present | yes |
| O-Emulator | present | present | yes |
| O-SimpleReverb | present | present | yes |

**8 tracked files.** They are exactly the same class of artifact this task just removed from
33 other plugins:

- `scripts/i18n-extract-README.md` L19-21 documents both as intermediates, and says of the
  skeleton: *"Paste the reviewed entries into `js/i18n.js`, then **delete**."*
- Each surviving `i18n-labels-skeleton.js` carries its own generated header:
  *"**NOT a shippable file.** Paste the reviewed entries into `<uiroot>/js/i18n.js` as the
  LABELS export, then delete this file."*
- The same classified reference grep run in Task 1 found **no consumer for these either** —
  the only non-narrative, non-self reference in the repo is `scripts/i18n-extract.js`, proved
  write-only, and `scripts/i18n-extract-README.md`, which is documentation.

They were left alone **only** because this task's scope was worded as "the 33 drafts plus
their siblings", and these four have no draft to anchor them. There is no evidence they are
needed.

> **Decision for the user:** delete these 8 files too (same mechanism — `git rm`, one
> path-scoped commit per plugin, 4 commits), or keep them.

## 2. BLOCKED plugins

**None.** All 33 in-scope plugins were classified `CLEAR`. No BLOCK-class grep hit exists —
zero references in any `CMakeLists.txt`, `.github/` workflow, `.claude/` hook, test, `Makefile`,
`*.sh`, or `*.py`, and no read construct anywhere in the repo targets the three basenames.

## 3. Narrative references that now point at deleted files

Recorded, not treated as blocks — these are prose, not dependencies. A future reader following
one of these paths will find nothing there. Harmless, but worth knowing about:

| Path | Hits |
|---|---|
| `.planning/quick/260901-c3s-.../260901-c3s-PLAN.md` | 15 |
| `.planning/quick/260901-c3s-.../260901-c3s-SUMMARY.md` | 7 |
| `.planning/quick/260901-akh-.../260901-akh-IMPLEMENTATION-PLAN.md` | 5 |
| `.planning/quick/260826-ieq-.../260826-ieq-STAGE-L-BRIEF.md` | 4 |
| `research/i18n-zh-hans-localization.md` | 3 |
| `.planning/quick/260901-akh-.../260901-akh-RESEARCH.md` | 3 |
| `.planning/quick/260904-g5l-.../260904-g5l-PLAN.md` | 2 |
| `.planning/quick/260901-akh-.../260901-akh-PLAN.md` | 2 |
| `.planning/quick/260826-ieq-.../260826-ieq-SUMMARY.md` | 2 |
| `.planning/quick/260826-ieq-.../260826-ieq-PLAN.md` | 2 |
| `.planning/STATE.md` | 1 |
| `.planning/quick/260826-ieq-.../260826-ieq-STAGE-L-EXECUTOR-REPORT.md` | 1 |

These are historical records of work that *produced* the files. Rewriting them would falsify
the record; leaving them is the right call. No action proposed.

## 4. Out-of-scope stray observed during the run

`research/wavetable-synthesis-3d-geometry.md` appeared as a new untracked file partway through
this task — absent at the precondition check, present at the end. It lies outside every
pathspec used here and belongs to a concurrent session in this shared checkout. Left
untouched, and named here so it is not mistaken for fallout from the deletions.
