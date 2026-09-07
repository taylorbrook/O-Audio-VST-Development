---
phase: quick-260907-bgx
plan: 01
subsystem: i18n tooling / repo hygiene
tags: [i18n, cleanup, dead-code, git-hygiene, reference-grep]
status: complete
requires: []
provides:
  - "33 plugins free of unserved i18n scaffolding"
  - "classified reference-grep method for proving a generated file has no consumer"
affects:
  - plugins/*/.planning/
key-files:
  created:
    - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-MANIFEST.md
    - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/260907-bgx-deferred-items.md
    - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-before.txt
    - .planning/quick/260907-bgx-delete-the-33-unserved-plugins-planning-/check-i18n-after.txt
  deleted:
    - plugins/*/.planning/i18n-index-draft.html (33)
    - plugins/*/.planning/i18n-labels-skeleton.js (33 of 37)
    - plugins/*/.planning/i18n-inventory.tsv (33 of 37)
decisions:
  - "A generator naming a file is not a consumer of it — proved write-only independently before treating scripts/i18n-extract.js as non-blocking."
  - "Basename-exact git rm only; params.tsv lives in the same directory and is read live by serve-ui.js."
  - "Skipped the plan's Task 3 Step 7 docs commit — orchestrator constraint reserves the docs commit."
  - "The 4 sibling-only plugins are deferred to a user decision, not silently swept in."
metrics:
  duration: ~8min
  completed: 2026-09-07
actuals:
  tokens: 575795
  tasks: 3
  commits: 33
---

# Quick Task 260907-bgx: Delete the 33 Unserved i18n Drafts Summary

Removed 99 tracked throwaway i18n intermediates from 33 plugins across 33 path-scoped commits,
gated on a classified reference grep that proved `scripts/i18n-extract.js` writes these files
and nothing anywhere reads them — `check-i18n` output is byte-identical before and after.

## What Happened

The three files (`i18n-index-draft.html`, `i18n-labels-skeleton.js`, `i18n-inventory.tsv`) are
generator output that both `scripts/i18n-extract-README.md` and each skeleton's own header
describe as disposable ("NOT a shippable file... then delete this file"). 33 plugins still
carried all three. They are now gone.

The load-bearing question was never "do these files matter" — it was "does a naive grep
mistake the generator for a consumer and block everything". It does: `scripts/i18n-extract.js`
names all three basenames. The gate had to distinguish *writes the path* from *reads the file*,
and it had to do so from evidence rather than from the plan's assurance.

## Task-by-Task

### Task 1 (tracer): gate + one plugin end-to-end — `c268f72a`

Re-verified every planning-time observation on live state before touching anything. All agreed:

| Glob | Disk | Tracked | Plan expected |
|---|---|---|---|
| `i18n-index-draft.html` | 33 | 33 | 33 |
| `i18n-labels-skeleton.js` | 37 | 37 | 37 |
| `i18n-inventory.tsv` | 37 | 37 | 37 |

The live draft list `diff`ed identical to the plan's hardcoded 33, and the derived list — not
the hardcoded one — drove the deletions. Skeleton and inventory sets were identical to each
other; `comm -13` derived the sibling-only four independently rather than trusting the plan.

**check-i18n baseline** captured first, before any deletion: exit 0, 1949 lines,
`ALL CHECKS PASS — 43 localized plugin(s)`.

**Grep classification — 124 hits across 50 paths, every one classified, buckets summing to 124:**

| Class | Hits | Verdict |
|---|---|---|
| SELF (the skeleton's own generated header, L2 × 37) | 37 | not a block |
| NARRATIVE (13 `.md` files under `.planning/` and `research/`) | 77 | not a block — record only |
| GENERATOR/DOC (`i18n-extract.js` ×7, `i18n-extract-README.md` ×3) | 10 | not a block, once proved |
| **BLOCK** | **0** | — |

Zero hits in any `CMakeLists.txt`, `.github/`, `.claude/`, test, hook, `Makefile`, `*.sh`, or
`*.py`. **Verdict: 33 CLEAR, 0 BLOCKED.**

**Write-only proved independently.** The read-construct grep
(`readFileSync|readFile|createReadStream|require|import|fetch|open` against the three
basenames) returned nothing, repo-wide. Reading L688-706 confirmed each of the three
`path.join` calls is immediately followed by `fs.writeFileSync`. The file's other three
`readFileSync` calls target `indexPath`, scanned js sources, and a generic `f` — none resolves
to a target basename.

**No glob dependency.** `grep -rIn "\.planning" scripts/` returns 16 lines, of which exactly
three are path *resolutions*: `serve-ui.js:335` → `params.tsv` (live read),
`verify-suite-battery.sh:39` → a fixed quick dir, and `i18n-extract.js:679` → the write-side
`outDir`. Nothing globs `.planning/*.tsv`.

**Tracer deletion on O-Gain** — `git rm` (never `--cached`, which would leave the file on disk
for a later pathspec commit to resurrect), then a pathspec-scoped commit. Result: 3 files,
1407 deletions, trailer present, and `check-i18n` re-run `diff`ed identical to baseline.
`TRACER_OK`.

**Tracer feedback gate:** auto mode off (`AUTO_CHAIN=false`, `AUTO_CFG=false`),
`HUMAN_VERIFY_MODE=end-of-phase`, verify is automated-only → re-ran the full `<verify>`
end-to-end, passed, continued without a checkpoint.

### Task 2: the remaining 32 — 32 commits

Same mechanism, driven from the manifest's CLEAR list in three batches of 11/11/10 to stay
clear of the executor watchdog. Six guards ran inside every iteration:

1. `git branch --show-current` == `main`, re-checked before **every** commit (CLAUDE.md — a
   session-start snapshot is stale in a shared checkout)
2. plugin name contains no `/`; no constructed path contains `libs/SAF`
3. all three files exist on disk **and** are tracked (`git ls-files --error-unmatch`)
4. `git rm` on three exact basenames — never a wildcard
5. pathspec-scoped `git commit -- <3 paths>` with the `Claude-Session` trailer
6. resulting commit touched exactly 3 files, else abort the whole loop

Plus the mandatory submodule guard before every commit. **No guard tripped at any of the 33
iterations.** `BATCH_OK`.

| # | Plugin | Commit | # | Plugin | Commit |
|---|---|---|---|---|---|
| 1 | O-AnalogEQ | `287927fa` | 18 | O-Orbit | `cc5cd8f1` |
| 2 | O-Bassoon | `0229df50` | 19 | O-Prism | `6e52a976` |
| 3 | O-Bells | `d50a4fc1` | 20 | O-Reed | `c646b80c` |
| 4 | O-Bowed | `3f5e777e` | 21 | O-ReverseDelay | `4e082828` |
| 5 | O-Chorus | `afd92412` | 22 | O-simpleAdditive | `47d55276` |
| 6 | O-Comp | `60ff3b9f` | 23 | O-simpleBeatmaker | `f578d1f5` |
| 7 | O-Contrabass | `fc9c41e6` | 24 | O-simpleFM | `f99c44d9` |
| 8 | O-Detune | `9e54d841` | 25 | O-simpleGrain | `371c87f1` |
| 9 | O-Formant | `58ee8ed6` | 26 | O-simplePhysicalModelSynth | `7b016ad9` |
| 10 | O-Freeze | `f9289a1a` | 27 | O-simpleSampler | `10c8ecd1` |
| 11 | O-FreqPulse | `dd65fb0f` | 28 | O-simpleSubtractive | `de2dc7d2` |
| 12 | **O-Gain** (tracer) | `c268f72a` | 29 | O-Tapestop | `25f371b6` |
| 13 | O-GrainScatter | `ed521aed` | 30 | O-Texture | `48feb7da` |
| 14 | O-Lyrica | `adcc5739` | 31 | O-TextureForge | `4989a968` |
| 15 | O-MicrotonalSampler | `1efc9ff4` | 32 | O-Tremolo | `be645be2` |
| 16 | O-MultiBandCompressor | `42e7c6d7` | 33 | O-Wind | `5d093179` |
| 17 | O-Octagon | `3841a8fa` | | | |

33 commits, 99 files, 49434 deletions. All 33 carry the `Claude-Session` trailer (verified by
grep count == 33). Every commit touched exactly 3 files.

### Task 3: post-deletion sweep — `SWEEP_OK`

| Check | Result |
|---|---|
| `check-i18n` before vs after | **byte-identical**, exit 0 both |
| `check-ui-labels --plugin O-Gain` | exit **0** — `== ALL CHECKS PASSED ==` |
| Draft / skeleton / inventory globs | **0 / 4 / 4** (expected 0 / 4 / 4) |
| Commits matching, each 3 files | **33**, `sort -u` of per-commit file counts == `3` |
| `params.tsv` disk vs tracked | **22 == 22**, unchanged |
| `git status --porcelain -- plugins/` | **empty** |
| `plugins/O-Orbit/libs/SAF` | **silent** — never staged |
| The 8 `260906-uu7` files | still untracked, untouched |

The check-ui-labels run is **not INCONCLUSIVE**: it served O-Gain's own 350 × 500 shell and its
own decoration image, with no port-bind failure and all 28 `[data-i18n]` elements visible — the
known concurrent-session port-clash hazard did not fire, so this is a real pass on the intended
target.

The primary proof is Step 2. `check-i18n` covers all 43 localized plugins; 99 files were
removed and its output did not change by a single byte. Nothing it asserts was reading them.

## Deviations from Plan

**1. [Constraint override] Task 3 Step 7 docs commit NOT made**

- **Found during:** Task 3
- **Issue:** The plan's Step 7 instructs a `docs(quick-260907-bgx): ...` commit of the quick
  dir. The orchestrator's constraints explicitly reserve the docs commit: *"Do NOT commit docs
  artifacts (SUMMARY.md, STATE.md, PLAN.md, MANIFEST.md, deferred-items.md) — the orchestrator
  handles the docs commit afterward."*
- **Resolution:** Constraint takes precedence. MANIFEST.md, deferred-items.md,
  check-i18n-before.txt and check-i18n-after.txt are written to disk and left **untracked** for
  the orchestrator to commit. No `git add` was run on them.
- **Files modified:** none

**2. [Rule 3 - Blocking] Manifest SHA fill stalled in shell, switched to python3**

- **Found during:** Task 3 Step 6 (bookkeeping only — no repo content involved)
- **Issue:** Two defects in one command. The `awk '/^\|---\|---\|---\|---\|$/{exit}'` used to
  truncate the manifest at the per-plugin table matched the **wrong** separator — the counts
  table at the top of the file has four columns too, so it cut at line 9. Then the
  `while read` loop that would have rebuilt the rows stalled and was watchdogged out.
- **Resolution:** The manifest was never overwritten (the stall happened before the `cat`), so
  no content was lost. Replaced the truncate-and-rebuild approach with an in-place python3
  substitution of the 33 `_pending_` cells — no truncation, no separator matching. 33/33 rows
  filled, 0 MISSING.
- **Files modified:** `260907-bgx-MANIFEST.md` (untracked artifact)

**No deviations touched repo content.** All 33 deletion commits went through the mechanism
exactly as the tracer proved it.

## Also Worth Knowing

**A generator naming a path is the default false positive here.** A "any hit in a script blocks
it" rule would have blocked all 33 and made this a no-op, because `i18n-extract.js` names all
three basenames seven times. The distinction that mattered was mechanical and cheap to check:
every `path.join` was immediately followed by `fs.writeFileSync`, and a read-construct grep
came back empty repo-wide. Worth reusing whenever a "is this file dead" question hits a tool
that *produces* the file.

**A basename grep cannot see a wildcard consumer.** The separate `grep -rIn "\.planning"
scripts/` sweep existed only to catch a script reaching these files by glob. It found the real
hazard instead: `serve-ui.js` reads `plugins/<P>/.planning/params.tsv` — a *different* `.tsv`
in the *same directory* as the deletion target. A single `.planning/*.tsv` glob anywhere in
this task would have deleted 22 live files, and `check-i18n` would not have noticed.

**`git rm` vs `git rm --cached` is load-bearing under a pathspec commit.** `--cached` leaves
the file on disk; a later pathspec commit naming that path resurrects it into the index. Using
plain `git rm` is what makes the "one path-scoped commit per plugin" mechanism safe.

**Environment stray:** `research/wavetable-synthesis-3d-geometry.md` appeared untracked partway
through the run — absent at the precondition check, present at the end. It is outside every
pathspec used here; a concurrent session in this shared checkout wrote it. Left untouched, and
named in deferred-items so it is not later mistaken for fallout from these deletions.

**Actuals note (honest scale disclosure):** `actuals.tokens: 575795` is chars/4 over the
realized diff as the rule defines. The plan's `estimate.tokens: 55000` was scoped to *authored*
content, not deleted payload — 49434 lines of removed generated HTML/TSV/JS dominate the diff.
The two numbers are not comparable and the gap is not a planning miss. Authored content this
run was ~3300 chars/4 across the manifest and deferred-items.

## Deferred

See `260907-bgx-deferred-items.md`. One decision for the user: the 4 sibling-only plugins
(**O-AnalogSaturation, O-Bitrot, O-Emulator, O-SimpleReverb**) still carry 8 tracked
`i18n-labels-skeleton.js` + `i18n-inventory.tsv` files. Their drafts were already removed in
earlier work. The same reference grep found **no consumer for these either** — they are the
same class of artifact, left alone only because this task's scope was anchored on the drafts.
Delete them (4 more commits, same mechanism) or keep them.

## Self-Check: PASSED

- `260907-bgx-MANIFEST.md` — FOUND (205 lines, 0 `_pending_` rows)
- `260907-bgx-deferred-items.md` — FOUND (71 lines)
- `check-i18n-before.txt` — FOUND (1949 lines)
- `check-i18n-after.txt` — FOUND
- All 33 commit hashes — FOUND in `git log`, each touching exactly 3 files, each carrying the
  `Claude-Session` trailer
- 99 target files — CONFIRMED ABSENT from disk and index
- 8 sibling-only files + 22 `params.tsv` — CONFIRMED PRESENT
