---
task: commit and merge all branches and worktrees into main
slug: merge-all-branches-worktrees
date: 2026-08-19
mode: quick (executed inline — worktree isolation is unsafe for a task that manages branches/worktrees itself)
---

# Plan: commit and merge all branches and worktrees into main

## Survey
- Main checkout on `improve/o-contrabass-v1.5`, dirty: O-SpectralShaper v1.6.1 fix (9 files, uncommitted)
- Worktrees: `VST-development-mbc` (improve/o-multibandcompressor-v1.7.0, clean), `VST-development-orbit` (improve/o-orbit-v1.0.1, clean)
- Branches vs main: bitrot-v1.13 ahead 0 (already merged) · contrabass-v1.5 ahead 1 · grainscatter-v2.4.3 ahead 1/behind 5 · mbc-v1.6.1 ahead 1 · mbc-v1.7.0 ahead 1/behind 5 · orbit-v1.0.1 ahead 2

## Known hazards
1. **GrainScatter version collision**: main released v2.4.3 (licensing); branch's UI fix also claims v2.4.3 → renumber fix to **v2.4.4** during merge (CMakeLists, CHANGELOG, NOTES, PLUGINS.md)
2. **MBC branches have disjoint lineage** (1.6.1 cut from current main; 1.7.0 from 5-commits-ago main) → real conflicts in CHANGELOG/CMakeLists/NOTES/PluginEditor/PluginProcessor; combined result = **v1.7.0** containing both changesets
3. **PLUGINS.md union merge** duplicates adjacent rows → run dup check after EVERY merge: `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d`

## Tasks
1. Commit dirty O-SpectralShaper v1.6.1 on current branch
2. Switch to main; merge --no-ff in order: contrabass → grainscatter (renumber 2.4.4) → mbc-1.6.1 → mbc-1.7.0 (combine, final 1.7.0) → orbit; dup-check after each
3. Delete merged branches (-d only), remove the two worktrees, delete bitrot branch (ahead 0)
4. SUMMARY.md + STATE.md quick-task row, commit planning artifacts
