---
task: Clean up all git branches and worktrees; bring unmerged work into main
slug: clean-up-branches-and-worktrees-merge-to-main
date: 2026-08-21
mode: quick
status: in-progress
---

# Quick Task: Branch & Worktree Cleanup

## Goal

Bring all outstanding branch work into `main`, then remove every extra branch and
worktree, leaving a single clean `main` checkout.

## Survey (verified before planning)

**Worktrees** (all clean — zero uncommitted files):
- `~/Dev/VST-development-emulator` → `feat/o-emulator-impl`
- `~/Dev/VST-development-octagon` → `improve/o-octagon-v1.2.0`
- `~/Dev/VST-development-prism` → `improve/o-prism-v1.20.0`

**Already merged into main** (0 ahead — delete only):
- `improve/o-multibandcompressor-v1.8.0`
- `improve/o-prism-v1.20.0`
- `improve/o-spectralshaper-v1.6.2`

**Unmerged** (merge-tree dry run: all conflict-free):
- `improve/o-contrabass-v1.6.0` — 4 commits, O-Contrabass v1.7.2 (tuning tab fix, scala-tuning-engine v3.0.1)
- `improve/o-octagon-v1.2.0` — 2 commits, O-Octagon v1.3.0 (flat-field audibility fix)
- `feat/o-emulator-impl` — 10 commits, O-Emulator v1.0.0 WIP (stage 2 verified, entering stage 3)

Dirty `.claude/` files in main checkout do NOT overlap any branch's changes.
All three branches touch `PLUGINS.md` (union merge driver) → duplicate-row check after every merge.

## Tasks

1. Merge `improve/o-contrabass-v1.6.0` → main (no-ff); PLUGINS.md dup check
2. Merge `improve/o-octagon-v1.2.0` → main (no-ff); PLUGINS.md dup check
3. Merge `feat/o-emulator-impl` → main (no-ff); PLUGINS.md dup check
4. `git worktree remove` emulator, octagon, prism worktrees
5. `git branch -d` all six branches (safe delete — refuses if unmerged)
6. SUMMARY.md + STATE.md quick-tasks table + docs commit

## Safety rails

- All worktrees verified clean; nothing uncommitted anywhere → merges lose nothing
- `-d` (never `-D`) so git itself verifies merged state before deleting
- Re-check `git branch --show-current` == main immediately before each commit (shared-checkout HEAD race)
- No push (not requested); main will be ahead of origin/main
