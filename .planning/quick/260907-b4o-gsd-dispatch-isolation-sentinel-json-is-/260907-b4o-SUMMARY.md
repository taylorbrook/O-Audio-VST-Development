---
phase: quick-260907-b4o
plan: 01
subsystem: repo-hygiene
tags: [gitignore, gsd-hooks, dispatch-isolation, untracking, git-pathspec]
status: complete
requires: []
provides:
  - ".gitignore rule ignoring .gsd/dispatch-isolation-sentinel.json"
  - "clean `git status --short` across sessions (no permanent .gsd noise)"
affects:
  - ".gitignore"
  - ".gsd/dispatch-isolation-sentinel.json (now untracked + ignored)"
tech-stack:
  added: []
  patterns:
    - "path-scoped commit of a deletion requires the file ABSENT from disk"
    - "git check-ignore is index-aware; --no-index separates rule-miss from stale-index"
key-files:
  created: []
  modified:
    - .gitignore
  deleted:
    - .gsd/dispatch-isolation-sentinel.json  # untracked from git; regenerated on disk, now ignored
decisions:
  - "Ignore the single sentinel path, NOT a directory-wide `.gsd/` rule — a directory rule would silently untrack a future `.gsd/` file that should be tracked (same reasoning the repo already records for the rejected plugin-`dist` wildcard)."
  - "T-b4o-01 (a forged sentinel silencing the guard) accepted, not mitigated — pre-existing and explicitly accepted upstream as #3045 SECURITY F3; this change grants no new write privilege."
metrics:
  duration: ~6min
  completed: 2026-09-07
  tasks: 2
  commits: 1
actuals:
  tokens: 5200
  tasks: 2
  commits: 1
---

# Quick Task 260907-b4o: Untrack the GSD dispatch-isolation sentinel — Summary

`.gsd/dispatch-isolation-sentinel.json` is GSD run-scoped state whose `written_at` is rewritten by every `gsd-tools query dispatch-isolation` call, so tracking it left ` M .gsd/dispatch-isolation-sentinel.json` in the working tree of every session. It is now untracked and ignored by an explicit single-path `.gitignore` rule, and the #3045 isolation guard was proven to reach the same `allow` decision in both the sentinel-absent and sentinel-present states.

**Commit:** `e8dff114` — `chore: untrack .gsd/dispatch-isolation-sentinel.json — run-scoped state, not source`
Diffstat: `.gitignore | 16 ++++++++++++++++` and `.gsd/dispatch-isolation-sentinel.json | 1 -` (`delete mode 100644` recorded).

## Evidence — raw probe output, both sentinel states

Pre-change baseline (recorded in the plan, re-confirmed before any edit): `git ls-files .gsd` listed the file, `git status --short` showed ` M`, `git check-ignore -v` produced no output and exited 1.

**Sentinel ABSENT** (Task 2 Part A, file deleted, driven in-process from the repo root):

```
ABSENT readSentinel: {"present":false}
ABSENT resolveIsolationState: {"gsdProject":true,"isolation":"none","harnessFlag":"isolation=\"worktree\"","error":null}
ABSENT evaluateDispatch: {"action":"allow"}
```

**Regeneration** (Task 2 Part B, the real writer):

```
node ~/.claude/gsd-core/bin/gsd-tools.cjs query dispatch-isolation --raw  ->  none
.gsd/dispatch-isolation-sentinel.json recreated:
{"isolation":"none","harness_flag":null,"phase":null,"plan":null,"written_at":1788793630905}
```

`written_at` 1788793630905 > baseline 1788793257561 — refreshed, as expected.

**Sentinel PRESENT** (Task 2 Part C, against the regenerated file):

```
PRESENT readSentinel: {"present":true,"stale":false,"malformed":false,"isolation":"none","harnessFlag":null,"phase":null,"plan":null,"writtenAt":1788793630905}
PRESENT resolveIsolationState: {"gsdProject":true,"isolation":"none","harnessFlag":null,"error":null}
PRESENT evaluateDispatch: {"action":"allow"}
```

Field-for-field identical to the pre-change baseline (`writtenAt` aside, which is the point of the file). `error:null` in both states is the load-bearing assertion: a non-null error is the guard's fail-closed branch and would have turned every executor dispatch into a hard block.

**Git quiet after regeneration:**

```
git check-ignore -v            -> .gitignore:293:.gsd/dispatch-isolation-sentinel.json	.gsd/dispatch-isolation-sentinel.json
git check-ignore --no-index -v -> .gitignore:293:.gsd/dispatch-isolation-sentinel.json	.gsd/dispatch-isolation-sentinel.json
git status --short | grep -c '\.gsd'  -> 0
git ls-files .gsd                     -> (empty)
git ls-tree -r --name-only HEAD -- .gsd -> (empty)
```

**The index is clean of `.gsd/` after regeneration** — the regenerated file appears neither as tracked nor as an untracked entry, and `git diff --cached --stat` is empty.

**Success criterion, "any number of times":** the writer was run three more times after regeneration; each printed `none` and each left `git status --short | grep -c '\.gsd'` at `0`.

## Findings

### 1. The `git commit -- <pathspec>` resurrection trap (generalizes to every untrack-a-generated-file task here)

A pathspec commit builds its tree from HEAD plus the **working-tree** content of the named paths, disregarding the index. Committing a `git rm --cached` removal while the file is still on disk therefore silently **re-adds** it: the commit succeeds, `git status` reads clean, and `git ls-files` still lists the file as tracked. The untracking fails invisibly — a textbook repudiation failure (T-b4o-04).

Also proven: naming only `.gitignore` in the pathspec does not commit the deletion either — it stays staged and uncommitted indefinitely.

The working sequence, in this order:

```bash
git rm --cached <path>          # index only, file stays on disk
# append the ignore rule
rm -f <path>                    # MUST be gone from disk before the commit
git commit -m "..." -- .gitignore <path>   # both paths in the pathspec
```

This matters specifically in this repo because CLAUDE.md makes path-scoped commits **mandatory** (shared `.git/index` between concurrent sessions), so the trap is on the only sanctioned commit path. `git rm --cached` needed no `-f` here: the index matched HEAD, only the working tree differed.

**Verify against `git ls-files` and `git ls-tree HEAD`, never `git status`** — status is exactly the surface the trap leaves looking correct.

### 2. A library comment documenting its OWN repo's invariant as if it were the caller's

`~/.claude/hooks/lib/isolation-sentinel.js:26-30` asserts:

> Sentinel path: `<cwd>/.gsd/dispatch-isolation-sentinel.json`. `.gsd` is gitignored (root `.gitignore`'s bare `.gsd` entry matches at any depth) …

That is true of the **GSD-core upstream repo**, not of a consumer checkout. This repo's `.gitignore` had no `.gsd` entry at all — `grep -n gsd .gitignore` found nothing and `git check-ignore` exited 1. The file was committed in `71a0e9c7` precisely because the invariant the library documents was never satisfied here, and nothing checks it. This is a reusable bug class: a shared library that states a repo-level precondition in prose, with no runtime assertion, is stating it about whichever repo the author was looking at.

### 3. `git check-ignore` is index-aware

It reported "not ignored" (exit 1) before the untracking *even though the rule would have matched*, because a tracked path is never ignored. It must therefore be run **after** untracking, or it produces a false negative that reads like a broken rule. `git check-ignore --no-index -v <path>` separates the two causes: "the rule does not match" vs "a stale index entry is shadowing it". Both forms were run here and both name `.gitignore:293`.

### 4. `harnessFlag` diverges between the two resolution paths (inert, but do not read it as a regression)

The one field that differs between sentinel-absent and sentinel-present:

| path | `isolation` | `harnessFlag` |
|---|---|---|
| sentinel present (read from file) | `none` | `null` |
| sentinel absent (registry+config fallback) | `none` | `isolation="worktree"` |

Cause, in `resolveRegistryIsolation` (guard lines ~286-338): the capability registry declares `harness-worktree` for this runtime, so `harnessFlag` is populated from the registry; the `workflow.use_worktrees: false` opt-out ladder then demotes `isolation` to `'none'` **without** clearing `harnessFlag`.

It is inert for the decision: `evaluateDispatch` returns `allow` at `if (state.isolation !== 'harness-worktree')` before it ever reads `state.harnessFlag`. Recorded because a future reader diffing the two states against the plan's baseline (`harnessFlag:null`) would otherwise flag it as a regression this change caused. It is not — it is a pre-existing property of the fallback path that was simply never exercised while the sentinel was always present.

### 5. Directory-wide `.gsd/` deliberately rejected — do not re-litigate blind

`.gsd/` holds exactly one file today, so a `.gsd/` rule would be equally correct *now* and would additionally pre-ignore future run-scoped siblings. Rejected anyway: a directory rule would silently swallow a future `.gsd/` file that *should* be tracked — the same reasoning already recorded in this `.gitignore` for the rejected plugin-`dist` wildcard, where hand-authored `installer.iss` and readme files live beside built artifacts. If GSD later writes more run-scoped files under `.gsd/`, widening is a deliberate one-line follow-up.

## Deviations from Plan

None — plan executed exactly as written. No auto-fixes were required.

## Threat Model Outcome

| Threat ID | Disposition | Outcome |
|---|---|---|
| T-b4o-01 (forged sentinel) | accept | Unchanged. Pre-existing, accepted upstream as #3045 SECURITY F3; this change moves the repo *into* the regime that comment already assumes and grants no new write privilege. |
| T-b4o-02 (guard fails open) | mitigate | Asserted in both states: `evaluateDispatch` → `allow`, `resolveIsolationState` → `isolation:"none", error:null`. No fail-closed flip, no throw. |
| T-b4o-03 (concurrent session's staging joins the commit) | mitigate | Path-scoped `git commit -- .gitignore .gsd/...` only; `git branch --show-current` (`main`) and `git status --short` re-checked immediately before committing. Diffstat shows exactly the two intended files. |
| T-b4o-04 (silent non-untracking) | mitigate | Task 1 Step 3 (`rm -f` before commit) applied; verified against `git ls-files` and `git ls-tree HEAD`, both empty, plus `delete mode 100644` in the commit output. |

## Other Task's Files — Untouched

The eight untracked `.planning/quick/260906-uu7-*` files were verified before and after: still exactly 8, still all `??`, none staged, none committed.

## Self-Check: PASSED

- `.gitignore` — FOUND, carries the rule exactly once outside comments (`grep -cx` = 1), at line 293.
- `.gsd/dispatch-isolation-sentinel.json` — present on disk, untracked, ignored.
- Commit `e8dff114` — FOUND in `git log`.
- All 7 plan verification points pass; `GREEN task1` and `GREEN task2` both emitted by the plan's own verify blocks.
