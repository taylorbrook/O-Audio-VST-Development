---
phase: quick-260813-mz1
plan: 01
subsystem: repo-infrastructure
tags: [git, worktrees, parallel-work, merge-driver, state-retirement]
status: complete
requires: []
provides:
  - ".gitattributes union merge driver for PLUGINS.md"
  - "CLAUDE.md ## Parallel Plugin Development convention"
  - "focus-state layer retired; readers redirected to PLUGINS.md / STATUS.md / modules/registry.yaml"
affects:
  - PLUGINS.md
  - CLAUDE.md
  - .claude/commands/*
  - .claude/skills/*
tech-stack:
  added: []
  patterns:
    - "git built-in `union` merge driver for row-scoped registry tables"
key-files:
  created:
    - .gitattributes
    - .planning/quick/260813-mz1-fix-multi-plugin-parallel-work-isolation/260813-mz1-SUMMARY.md
  modified:
    - CLAUDE.md
    - 15 files under .claude/
  deleted:
    - .planning/workflow/active-plugin.json
    - .planning/workflow/registry.json
decisions:
  - "plugin-focus.md was REWRITTEN (option a), not marked retired — its `skill: plugin-context` frontmatter already points at a skill that implements PLUGINS.md + STATUS.md focus"
  - "Redirect targets split by concern: focus/plugin state → PLUGINS.md + plugins/<Name>/.planning/STATUS.md; module tracking → modules/registry.yaml"
metrics:
  duration: ~25min
  completed: 2026-08-13
actuals:
  tokens: 61000
  tasks: 3
  commits: 5
---

# Quick Task 260813-mz1: Fix Multi-Plugin Parallel Work Isolation Summary

Installed a `union` merge driver for the contended `PLUGINS.md` registry table, documented the branch/worktree convention in `CLAUDE.md`, retired the dead `/plugin-focus` JSON state layer across 15 `.claude/` readers, corrected one false version bump on `feat/o-octagon`, and removed one dead worktree.

## Branch and Base SHA

| Item | Value |
|------|-------|
| Infra branch | `chore/parallel-work-isolation` |
| Resolved base SHA | **`3b5205cf81b9112980db3a8434fe6356267d12e4`** ("merge: integrate improve/o-contrabass-v1.1 — O-Contrabass v1.2.0 → v1.4.0") |
| Briefed SHA | `fa38ff46` — **stale**, 3 commits behind current `main`. The branch was cut from the `main` **ref**, never from the SHA literal, exactly as the plan directed. |

**The main worktree was switched off its original branch.** It sat on `improve/o-freqpulse-tooltip-measure` (1 ahead of main) and is now left on `chore/parallel-work-isolation`.

To return to it:

```bash
cd /Users/taylorbrook/Dev/VST-development
git switch improve/o-freqpulse-tooltip-measure
```

## Commits

All local. **No `git push` and no `git fetch` was run anywhere, on any branch, in any worktree.**

| # | Branch | Hash | Message |
|---|--------|------|---------|
| 1 | chore/parallel-work-isolation | `07e95283` | chore(parallel-work): add union merge driver for PLUGINS.md registry table |
| 2 | chore/parallel-work-isolation | `7849b31b` | docs(parallel-work): document branch/worktree convention and shared PLUGINS.md rule |
| 3 | chore/parallel-work-isolation | `9998167e` | chore(parallel-work): retire dead plugin-focus state layer, redirect readers to PLUGINS.md and modules/registry.yaml |
| 4 | chore/parallel-work-isolation | (this commit) | docs(quick-260813-mz1): plan and summary for parallel-work isolation fix |
| — | feat/o-octagon | `10828090` | fix(o-octagon): restore O-SpectralShaper registry row to 1.3.2 to match its CHANGELOG |

## Task 1 — Union merge driver + CLAUDE.md convention (TASK-1, TASK-5)

`.gitattributes` did not exist at the repo root. Created with a 5-line comment block explaining
why (`PLUGINS.md` is a row-scoped global registry table edited by every plugin branch) plus the
single rule `PLUGINS.md merge=union`. `union` is a git built-in, so no `merge.<driver>` config
stanza was needed anywhere.

**Primary gate — the attribute actually resolves:**

```
$ git check-attr merge -- PLUGINS.md
PLUGINS.md: merge: union
```

`CLAUDE.md` gained a `## Parallel Plugin Development` section immediately before `## Project Structure`,
covering all six required points (branch-per-plugin cut from `main`; worktree-per-plugin;
`VST-development-<slug>` naming with the `VST-contrabass-v1.1` violation named explicitly;
outside-the-repo placement; the shared-row rule with the honest union-merge duplicate-row tradeoff
and the merged-into-working-tree attribute caveat; and the fenced command block).

**Additive-only gate:** `git diff main HEAD --numstat -- CLAUDE.md` → **53 insertions / 0 deletions.**
Every pre-existing line is byte-identical.

**Task 1 verify: PASS** (all 10 assertions, including both single-file atomic commits and the
no-remote-tracking-ref push check).

## Task 2 — Retire the dead focus layer (TASK-4)

Both retired files were git-tracked and were removed with `git rm`:

- `.planning/workflow/active-plugin.json` (10 lines)
- `.planning/workflow/registry.json` (412 lines)

Everything else under `.planning/workflow/` survived untouched: `checkpoints/`, `schemas/`,
`scripts/`, `templates/`, `module-deps.json`, `professional-quality-standards.md`.

### Completeness grep

The authoritative gate `grep -rln "active-plugin\|registry\.json" .claude/` returned **exactly the
15 files measured at planning time — no more, no fewer.** All 15 were updated:

| # | File | Concern |
|---|------|---------|
| 1 | `.claude/commands/continue.md` | focus/state |
| 2 | `.claude/commands/improve-milestone.md` | focus/state (`activeMilestone`) |
| 3 | `.claude/commands/module-add.md` | module tracking |
| 4 | `.claude/commands/module-remove.md` | module tracking |
| 5 | `.claude/commands/module-upgrade.md` | module tracking |
| 6 | `.claude/commands/plugin-focus.md` | focus/state (whole command) |
| 7 | `.claude/commands/plugin-handoff.md` | focus/state |
| 8 | `.claude/commands/reconcile.md` | focus/state (checks removed, not repointed) |
| 9 | `.claude/skills/state-recovery/SKILL.md` | focus/state |
| 10 | `.claude/skills/state-validation/SKILL.md` | focus/state |
| 11 | `.claude/skills/improve-milestone/SKILL.md` | focus/state |
| 12 | `.claude/skills/improve-milestone/BOUNDARIES.md` | focus/state |
| 13 | `.claude/skills/improve-milestone/references/README.md` | focus/state |
| 14 | `.claude/skills/improve-milestone/references/state-management.md` | focus/state |
| 15 | `.claude/skills/improve-milestone/references/version-integration.md` | focus/state (git add line) |

**The briefed skill list was wrong, as the plan's `verified_facts` predicted.** `plugin-context`,
`context-resume`, `plugin-list`, `plugin-status`, `plugin-pause`, and `plugin-resume` contain
**zero** references to either retired file and needed **no change**. `plugin-context/SKILL.md`
already declares "STATUS.md is authoritative" at line 170 — the redirect wording was matched to
that existing contract rather than inventing a new one.

### Redirect targets by concern

- **Focus / plugin state** → `PLUGINS.md` for the roster and status column, and
  `plugins/<Name>/.planning/STATUS.md` for per-plugin state (with `focused: true` in frontmatter,
  matching `plugin-context/SKILL.md`).
- **Module tracking** → `modules/registry.yaml`. The reword accounts for the **inverted relation**:
  the YAML stores `used_by` per module rather than a `modules` array per plugin, and
  `scripts/regen-registry-used-by.sh` regenerates it from disk truth. `module-add.md`'s
  "Registry Changes" JSON before/after block was rewritten as YAML `used_by` before/after.

`reconcile.md`: the retired-schema validation block and the `registry.focused === active-plugin.plugin`
cross-check were **removed**, not repointed, per the plan. The surrounding generic "Registry"
prose was also retargeted to `PLUGINS.md` so no reader is left pointing at a phantom.

### plugin-focus.md — option (a), rewritten

**Chosen: (a) rewrite to focus via `PLUGINS.md` + STATUS.md with no JSON writes.**

Rationale: the command's own frontmatter already reads `skill: plugin-context`, and that skill
already implements exactly this mechanism (`focused: true` in STATUS.md frontmatter, PLUGINS.md
for the roster). Only the command's *description* of the mechanics was stale — the command itself
still works. Marking it retired would have removed a working command to fix a documentation bug.
The module-update-check section was additionally reshaped to scan `used_by` per module, and the
"Parallel Instance Support" section now correctly states that each instance writes only into its
own plugin's STATUS.md — which is what actually makes parallel focus safe.

### Result gate

```
$ grep -rn "active-plugin\|registry\.json" .claude/
(no output — ZERO references of ANY kind, marked or unmarked)
```

Stronger than the plan required: the gate only demanded zero *unmarked* references.

**Task 2 verify: PASS** (all 14 assertions). One atomic commit carries both deletions plus all
15 redirects — 257 insertions / 725 deletions across 17 files.

### Orphaned residuals — REPORT ONLY, not deleted

These now describe deleted files and are outside the authorized deletion scope. Flagged for a
future cleanup:

- `.planning/workflow/schemas/active-plugin.schema.json`
- `.planning/workflow/schemas/registry.schema.json`

## Task 3a — O-SpectralShaper registry row on feat/o-octagon (TASK-2)

Commit `fba35081` had bumped the row to `1.4.0 / 2026-08-12` while that branch's CHANGELOG still
topped out at `[1.3.2] - 2026-07-07`. Work was done **in the octagon worktree**
`/Users/taylorbrook/Dev/VST-development-octagon`; the branch was not checked out anywhere else.
`fba35081` was **not** reverted wholesale — it carries unrelated octagon work.

### CHANGELOG before/after capture

`git show feat/o-octagon:plugins/O-SpectralShaper/CHANGELOG.md | head -5`

**BEFORE the edit:**
```
# O-SpectralShaper Changelog

## [1.3.2] - 2026-07-07

### Fixed
```
sha256 (full file): `b2e8b08316fd737ae2bec7150cc0526bfeacc8c4d0eada6bc41f06ea89daacff`

**AFTER the commit:**
```
# O-SpectralShaper Changelog

## [1.3.2] - 2026-07-07

### Fixed
```
sha256 (full file): `b2e8b08316fd737ae2bec7150cc0526bfeacc8c4d0eada6bc41f06ea89daacff`

**Byte-identical.** The CHANGELOG was never staged or touched.

### Row before/after

```
- | O-SpectralShaper | 📦 Installed | 1.4.0 | Audio Effect (Spectral Transient Shaper) | 2026-08-12 |
+ | O-SpectralShaper | 📦 Installed | 1.3.2 | Audio Effect (Spectral Transient Shaper) | 2026-07-07 |
```

`git show --numstat --format= feat/o-octagon` → `1	1	PLUGINS.md` — exactly one line changed in
exactly one file. `diff` against `main`'s row: identical.

## Task 3b — Dead contrabass worktree removed (TASK-3)

Re-confirmed live before acting (not reused from planning):

```
$ git rev-list --count main..improve/o-contrabass-v1.1
0
$ git rev-list --left-right --count main...improve/o-contrabass-v1.1
1	0          # 1 behind, 0 ahead — fully merged
```

Then, from the main worktree:

```
$ git worktree remove /Users/taylorbrook/Dev/VST-contrabass-v1.1
$ git branch -d improve/o-contrabass-v1.1
Deleted branch improve/o-contrabass-v1.1 (was d5afc056).
```

`-d` was used, never `-D`; `--force` was never used on `worktree remove`. Both worktrees were
verified clean (`git status --porcelain` empty) before anything was removed.

```
$ git worktree list
/Users/taylorbrook/Dev/VST-development          9998167e [chore/parallel-work-isolation]
/Users/taylorbrook/Dev/VST-development-octagon  10828090 [feat/o-octagon]
```

Exactly 2 worktrees, octagon intact.

## Task 3c — Stale-branch report (TASK-6) — REPORT ONLY

Counts gathered **live** during execution via `git rev-list --left-right --count main...<branch>`.
**No merge, rebase, delete, or push was performed on any of these four branches.** All four were
re-verified to still resolve after the task.

| Branch | Behind main | Ahead of main | Contents |
|--------|-------------|---------------|----------|
| `improve/o-spectralshaper-botanical` | 3 | **10** | The entire **O-Octagon** plugin (Stages 0→4.1: 131 files, ~56.7k insertions — DSP, WebView UI, render harness, unit tests, full `.planning/` tree) **plus** O-SpectralShaper v1.4.0 Naturalist UI. Also carries `.github/workflows/ci-tests.yml` (235 lines, new) and build-and-release changes. |
| `improve/o-spectralshaper-tooltips` | 3 | **2** | O-SpectralShaper only: v1.4.0 Naturalist UI **re-committed as a different SHA** (`d9b21119` here vs `5e2c4a3f` on botanical) plus v1.5.0 toggle-gated tooltips. 15 files, 652 insertions. |
| `improve/o-freqpulse-tooltip-measure` | 3 | **1** | O-FreqPulse v1.17.0 tooltip shrink-to-fit measurement + placement fix. 5 files, 104 insertions. **This is the branch the main worktree was originally on.** |
| `docs/logic-multichannel-dbap-research` | 3 | **3** | O-Octagon Stage 0 only (BRIEF/REQUIREMENTS/ROADMAP/ARCHITECTURE + the DBAP research doc). 12 files, 3,471 insertions. **Strict ancestor subset of `improve/o-spectralshaper-botanical`** — its 3 commits (`12ae50dd`, `fb6394ac`, `d8cf882b`) all appear in botanical's 10. |

### CHANGELOG versions on the two o-spectralshaper branches

The brief expected botanical at 1.4.0 and tooltips at 1.5.0. **Both measured as expected — no mismatch.**

| Branch | `plugins/O-SpectralShaper/CHANGELOG.md` top | That branch's PLUGINS.md row |
|--------|--------------------------------------------|------------------------------|
| `improve/o-spectralshaper-botanical` | `## [1.4.0] - 2026-08-12` | `1.4.0 ... 2026-08-12` ✅ consistent |
| `improve/o-spectralshaper-tooltips` | `## [1.5.0] - 2026-08-13` | `1.5.0 ... 2026-08-13` ✅ consistent |

Both branches' registry rows **agree with their own changelogs** — unlike `feat/o-octagon`, which
carried the 1.4.0 row with a 1.3.2 changelog and was the actual defect fixed in 3a.

### Observations for whoever triages these (no action taken)

1. **Three branches all claim the O-SpectralShaper row at different versions** (botanical 1.4.0,
   tooltips 1.5.0, freqpulse 1.3.2). This is precisely the collision class the new union merge
   driver addresses — and also precisely the case where union produces a **duplicate row** rather
   than a conflict, because all three edit the *same* row. Merge these one at a time and check
   the row after each.
2. **`improve/o-spectralshaper-botanical` is misnamed** — 10 of its 11 changed subtrees are
   O-Octagon, not O-SpectralShaper. It bundles an entire unreleased plugin with a UI improvement.
3. **`docs/logic-multichannel-dbap-research` is fully contained in botanical** and can likely be
   deleted once botanical lands — but that is a decision for a later session, not this task.
4. O-SpectralShaper v1.4.0 exists as **two different commits** on two branches; merging both will
   need care.

## Deviations from Plan

### 1. [Gate deviation — external cause, no improvisation] Octagon push-gate literal reports 2, not 1

**Found during:** Task 3 verify.

**The gate:**
```
test "$(git rev-list --count origin/feat/o-octagon..feat/o-octagon)" = "1"
```
**Measured:** `2`.

**Cause — a concurrent session committed to `feat/o-octagon` mid-run.** Full provenance:

```
$ git log --oneline origin/feat/o-octagon..feat/o-octagon
10828090 fix(o-octagon): restore O-SpectralShaper registry row to 1.3.2 ...   <- mine
c1c3101f verify(O-Octagon): Stage 4 phase 4.2 - PARTIAL, desk blocks verified, Block C not run   <- NOT mine
```

| Event | Time |
|-------|------|
| Octagon worktree observed at `300d8cf0` (== `origin/feat/o-octagon`) | run start |
| My branch cut / first commit | 16:44:00 / 16:44:07 |
| **`c1c3101f` committed by another process** | **16:44:16** |
| My octagon fix commit `10828090` | 16:49:36 |

`c1c3101f` touches only `plugins/O-Octagon/.planning/` (REQUIREMENTS.md, STATUS.md,
VERIFICATION-4.2.md — 3 files) and is unrelated to this task.

**The gate's actual purpose is satisfied, and verifiably so:**

- **Nothing was pushed.** `git reflog show origin/feat/o-octagon` shows the last `update by push`
  at **16:23:10**, *before* both commits. There is no push entry after it.
- **My commit is atomic.** `git show --numstat --format= feat/o-octagon` → `1	1	PLUGINS.md`.
  Nothing from `c1c3101f` was swept in; my commit simply sits on top of it as parent.

**Action taken: none.** Per the run constraints I did not improvise a fix — rewriting or resetting
`feat/o-octagon` to make a count literal read `1` would destroy another session's committed work.
The deviation is recorded here instead. This is the same failure mode already in project memory
(an unrelated commit landing mid-task from another process, cf. quick task 260808-uiq).

**The concurrent session kept working after that.** By end of run `feat/o-octagon` was at
`7d9270d7` and **3** ahead of origin:

```
7d9270d7 docs(O-Octagon): Block C pre-flight + runbook; Gate 9 spelling corrected   <- concurrent
10828090 fix(o-octagon): restore O-SpectralShaper registry row to 1.3.2 ...          <- mine
c1c3101f verify(O-Octagon): Stage 4 phase 4.2 - PARTIAL, ...                         <- concurrent
```

Re-verified at end of run that the concurrent activity did not disturb the fix:

- `git merge-base --is-ancestor 10828090 feat/o-octagon` → **true** (not reverted or rewritten)
- `git show feat/o-octagon:PLUGINS.md | grep O-SpectralShaper` → still `1.3.2 ... 2026-07-07`

**Follow-up for the user:** `feat/o-octagon` is 3 commits ahead of its remote. All three are
legitimate; push when ready. Because a second session is actively committing to that branch,
re-check the count before assuming any number in this summary is still current.

### 2. [Rule 2 — completeness] reconcile.md generic "Registry" prose also retargeted

The plan scoped `reconcile.md` to removing the two retired-schema checks. Removing only those left
five lines of surrounding prose ("Sync from Registry", "Registry and STATUS.md disagree",
"Update registry to match STATUS.md") referring to a store that no longer exists — a dangling
pointer in spirit even though it passes the literal grep gate. Retargeted to `PLUGINS.md`.
Same reasoning applied to the corresponding prose in `state-recovery/SKILL.md` and
`state-validation/SKILL.md`.

### 3. [Rule 2 — completeness] state-validation/SKILL.md validation protocol rewritten, not just repointed

Its Schema Validation table, Cross-File Consistency table, Existence Checks table, Check Execution
Order block, and Implementation Guidance bash were all built around the two JSON files. Repointing
filenames alone would have left a protocol that validates nothing. Rewritten to validate
`PLUGINS.md` table shape (including a **duplicate-row check**, the union-merge artifact this task
introduces the risk of), STATUS.md frontmatter, and `modules/registry.yaml`.

No Rule 4 (architectural) decisions arose. No package installs. No authentication gates.

## Verification

Run from `/Users/taylorbrook/Dev/VST-development` after all three tasks:

| # | Check | Result |
|---|-------|--------|
| 1 | `git check-attr merge -- PLUGINS.md` | `PLUGINS.md: merge: union` ✅ |
| 2 | `git rev-parse --abbrev-ref HEAD` | `chore/parallel-work-isolation` ✅ |
| 3 | `git log --oneline main..HEAD` | 4 commits ✅ |
| 4 | `git status --porcelain --untracked-files=no` in both worktrees | empty ✅ |
| 5 | `git ls-files .planning/workflow/` | neither retired JSON listed ✅ |
| 6 | `grep -rn "active-plugin\|registry\.json" .claude/ \| grep -viE "retired\|removed\|no longer"` | zero lines ✅ (in fact zero references of any kind) |
| 7 | `git worktree list` | 2 entries ✅ |
| 8 | `git show feat/o-octagon:PLUGINS.md \| grep O-SpectralShaper` | `1.3.2 ... 2026-07-07` ✅ |
| 9 | `git branch --list` | `improve/o-contrabass-v1.1` absent; all 4 stale branches present ✅ |
| 10 | `git push` | never run — reflog confirms no push after 16:23:10 ✅ |

**Task 1 verify: PASS. Task 2 verify: PASS. Task 3 verify: PASS on all assertions except the
octagon count literal, explained in Deviation 1 above.**

### Human check (from the plan)

> Confirm the SUMMARY.md stale-branch table lists all four branches with live ahead/behind counts
> and contents, and that no merge or delete was performed on any of them.

All four are tabulated in Task 3c with live counts, per-branch commit lists, and diffstat summaries.
The only branch deleted anywhere in this run was `improve/o-contrabass-v1.1` (Task 3b, authorized,
0 ahead, via `-d`).

## Known Stubs

None. No stubs, no skipped tests, no unrun verify gates.

## Self-Check

- `.gitattributes` — FOUND
- `CLAUDE.md` `## Parallel Plugin Development` — FOUND
- `260813-mz1-SUMMARY.md` — FOUND
- `.planning/workflow/active-plugin.json` — absent from disk and index (intended) ✅
- `.planning/workflow/registry.json` — absent from disk and index (intended) ✅
- Commit `07e95283` — FOUND
- Commit `7849b31b` — FOUND
- Commit `9998167e` — FOUND
- Commit `10828090` (feat/o-octagon) — FOUND

## Self-Check: PASSED
