---
phase: quick-261029-md9
plan: 01
subsystem: repo-hygiene
tags: [audit, cleanup, organization, gitignore, repo-size]
status: complete
requires: []
provides:
  - "261029-md9-AUDIT.md: 28 ranked findings (C1-C12) + 14 approval batches"
affects: [CLAUDE.md, README.md, modules/registry.yaml, plugins/O-Contrabass, backups/, .planning/]
tech-stack:
  added: []
  patterns: ["read-only aggregate-evidence audit under concurrent sessions", "path-scoped --only commits"]
key-files:
  created:
    - .planning/quick/261029-md9-do-a-thorough-review-through-this-repo-f/261029-md9-AUDIT.md
  modified: []
decisions:
  - "Committed on main despite the GSD protected-branch guard: project CLAUDE.md mandates trunk-based work on main, and the plan requires branch == main"
  - "Only F-17 (8 ignored __pycache__ dirs) qualified as safe; 0 safe C11 .gitignore candidates, so .gitignore was left untouched"
metrics:
  duration: "~12 min"
  completed: 2026-10-29
actuals:
  tokens: 9000
  tasks: 2
  commits: 2
plan_head_before: 4430624e93a55968312303570650975d3ecdf8dc
plan_head_after: e0372a001b4a72f38a673e1c28589b42517d7941
commits: 2
---

# Quick 261029-md9 Plan 01: Repo cleanup + organization audit Summary

This was a read-only survey of the whole monorepo. It produced 28 findings, each with evidence and a rank, across 12 categories, plus 14 batches the user can approve one at a time. Only 8 regenerable `__pycache__` dirs were deleted.

- **Audit HEAD:** `4430624e` (2026-10-29T23:15:12Z)
- **Counts:** High 3, Med 13, Low 12. 1 safe, 27 needs-approval.
- **Re-measured baselines:** these matched the plan (4773 tracked files, 958.99 MiB pack, 13 tracked-but-ignored, 8 cache dirs). Untracked was 14, because this task's own PLAN.md counts.

## Batches
B1 release-tag naming fix (F-01) · B2 STATE.md frontmatter + dangling registry entry (F-04, F-05) · B3 README catalog + retire PUBLIC-RELEASE-READINESS (F-06, F-09) · B4 stop O-Contrabass golden growth (F-02, F-07; waits on O-Contrabass) · B5 tracked-but-ignored reconcile (F-12, F-22) · B6 local disk reclaim, about 376 MB (F-10, F-18) · B7 registry used_by regen (F-11) · B8 PLUGINS.md/STATUS.md sync (F-08, F-13, F-14; fenced subset waits) · B9 .planning tidy (F-20, F-21, F-23) · B10 codebase-map dedupe (F-16) · B11 backups retention (F-15, F-25) · B12 git refs tidy (F-24) · B13 leftovers, recommend keep (F-19, F-26, F-27, F-28) · B14 history rewrite, after B4 only (F-03)

## Task 2 applied
8 `__pycache__` dirs were removed, one explicit `rm -rf --` each. Each was checked first: untracked, ignored, `.pyc`-only, outside every fence. The census went from 8 to 0. `.gitignore` was not changed. The tracked-ignored count stayed at 13 and the untracked count at 14. No row was reclassified.

## Commits
- `887da0bc` docs(261029-md9): repo cleanup + organization audit
- `e0372a00` chore(261029-md9): apply safe repo hygiene items

## Deviations from Plan
- The plan's planning-time note counted 7 tracked `preset-manager.js` copies. The measured count is 8, because O-Tremolo is the eighth. The total of 13 is unchanged. This is recorded in AUDIT C2.
- `e1-max-sustain.json` at the root is not tracked. It is ignored by `.gitignore:46`, so the audit records it as an ignored stray (F-18), not a tracked file.
- The GSD `git.base-branch --is-protected main` check returned true. Commits still went to `main`, as the project CLAUDE.md (trunk-based) and the plan require.

## Self-Check: PASSED
- AUDIT.md exists; AUDIT-OK and APPLY-OK gates pass.
- 887da0bc and e0372a00 are ancestors of HEAD; the union of both commits touches only AUDIT.md.
- `git status --short -- <AUDIT>` is empty.
