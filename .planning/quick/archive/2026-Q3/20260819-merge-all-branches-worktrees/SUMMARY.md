---
task: commit and merge all branches and worktrees into main
slug: merge-all-branches-worktrees
date: 2026-08-19
status: complete
---

# Summary: commit and merge all branches and worktrees into main

## What happened
1. **Committed** dirty O-SpectralShaper v1.6.1 fix (curve editors update on preset load) — was sitting uncommitted on `improve/o-contrabass-v1.5` (9872bd56)
2. **Merged 5 branches** into main (--no-ff), in order:
   - `improve/o-contrabass-v1.5` — O-Contrabass v1.5.0 bow-noise realism + the SpectralShaper commit (clean)
   - `improve/o-grainscatter-v2.4.3` — UI layout fix **renumbered to v2.4.4** (branch claimed 2.4.3, but main had already released 2.4.3 as the licensing release; CHANGELOG conflict resolved keeping both entries, CMake/NOTES/PLUGINS.md bumped). Also backfilled the missing v2.4.3 licensing entry in NOTES.md
   - `improve/o-multibandcompressor-v1.6.1` — code-review fixes CR-01, WR-01..05 (clean)
   - `improve/o-multibandcompressor-v1.7.0` — 50 presets + categorised browser. Disjoint lineage vs 1.6.1; C++ auto-merged, metadata trio resolved by hand, **combined version = 1.7.0**. Verified: `ninja O-MultiBandCompressor_VST3` compiles clean post-merge
   - `improve/o-orbit-v1.0.1` — O-Orbit v1.1.0 Parts A–D (clean)
3. **PLUGINS.md union-merge dup check after every merge** — fired twice (O-SpectralShaper, O-MultiBandCompressor), both deduped keeping the newest row; final check clean
4. **Deleted 6 branches** with `-d` (incl. `improve/o-bitrot-v1.13`, already merged / ahead 0)
5. **Removed both worktrees**: `../VST-development-mbc`, `../VST-development-orbit` (both clean)

## End state
- Only `main` remains, **ahead of origin/main by 14 commits — not pushed** (push wasn't requested)
- No worktrees, no dirty files (besides these planning artifacts)

## Follow-ups (all resolved same session)
- ~~Build+install~~ — all 5 plugins built and installed via `./scripts/build-and-install.sh`; all 5 AUs confirmed registered via `auval -a`
- ~~MBC preset drift~~ — re-ran the preset render harness on the MERGED sources: 47/47 real presets engage, load-order-independence clean across all 50, 0 failures (same result as v1.6.1's pre-merge run). Audible differences are the intended WR fixes only (e.g. Parallel Crush loses its baked-in WR-03 combing)
- ~~Push~~ — main pushed to origin
