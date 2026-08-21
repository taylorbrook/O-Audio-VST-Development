---
task: Clean up all git branches and worktrees; bring unmerged work into main
slug: clean-up-branches-and-worktrees-merge-to-main
date: 2026-08-21
status: complete
---

# Summary: Branch & Worktree Cleanup (2026-08-21)

## What was done

Repo reduced to a single `main` checkout — 0 extra branches, 0 extra worktrees.

**Merged into main** (all conflict-free, `--no-ff`, merge-tree dry-run verified first):

| Branch | Commits | Content |
|--------|---------|---------|
| `improve/o-contrabass-v1.6.0` | 4 | O-Contrabass **v1.7.2** — tuning tab restored to 3-column layout (scala-tuning-engine v3.0.1 + tuning-panel module updates) |
| `improve/o-octagon-v1.2.0` | 2 | O-Octagon **v1.3.0** — flat-field audibility fix (srcZ/rolloff/width/blur) |
| `feat/o-emulator-impl` | 10 | O-Emulator **v1.0.0 WIP** — stages 0–2 complete (DSP verified), STATUS at stage 3 (GUI). ~6,900 lines: full console-emulation DSP suite + render harness + stage planning docs |

**Deleted branches** (all via safe `git branch -d` after merge):
`feat/o-emulator-impl`, `improve/o-contrabass-v1.6.0`, `improve/o-multibandcompressor-v1.8.0`,
`improve/o-octagon-v1.2.0`, `improve/o-prism-v1.20.0`, `improve/o-spectralshaper-v1.6.2`
(the last three were already 0-ahead of main — delete only, no merge needed).

**Removed worktrees** (all verified clean — zero uncommitted files — before removal):
`~/Dev/VST-development-emulator`, `~/Dev/VST-development-octagon`, `~/Dev/VST-development-prism`

## Safety checks that ran

- PLUGINS.md union-merge duplicate-row check after **each** of the 3 merges: clean all 3 times
- Dirty `.claude/` files in main checkout verified disjoint from all branch changes (untouched, still dirty)
- `git branch --show-current == main` re-verified immediately before each merge (shared-checkout HEAD race)
- `-d` not `-D` throughout — git verified merged state before every deletion

## Follow-ups

- `main` is **ahead 23 of origin/main, unpushed** (push not requested)
- O-Emulator continues from main: next session should cut a fresh branch + worktree
  (`git worktree add ../VST-development-emulator -b feat/o-emulator-stage3 main`) and resume at stage 3 (GUI)
- O-Contrabass v1.7.2 and O-Octagon v1.3.0 are merged but not rebuilt/reinstalled in this session
