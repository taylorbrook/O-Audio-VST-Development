---
task: Remove TÂCHES attribution and all taches plugins from the repo
slug: 260720-rtc-remove-taches-attribution-and-plugins
date: 2026-07-20
mode: quick
status: planned
---

# Remove TÂCHES attribution + taches plugins

## Objective
Remove (1) all attribution/credit to **TÂCHES** and (2) all of the **taches plugins**
from the repository.

## Scope

### Plugin removal
- `archive/tache_plugins/` — the complete directory of 17 archived taches plugins
  (AngelGrain, AutoClip, ClapMachine, DriveVerb, Drum808, DrumRoulette, FlutterVerb,
  GainKnob, LushPad, LushVerb, MinimalKick, OrganicHats, PadForge, Scatter, TapeAge,
  TEMPLATE-HEADLESS-EDITOR, Words). 348 git-tracked files, ~6.9 MB. Fully recoverable
  from git history. These are not part of any active build (already archived out of
  `plugins/`), so no build/CMake wiring depends on them.

### Attribution removal (real hits only — `attaches`/`attached` substring matches excluded)
- `README.md:5` — delete the `#VST building system based upon **[TÂCHES](...)**` line.
- `PLUGINS.md:68` — delete the `Plugins created by **[TÂCHES](...)**` heading and the
  entire taches-plugins registry table beneath it (through the AngelGrain row).
- `.planning/research/STACK.md:9` — strip the "by TACHES" credit from the GSD sentence.
- `plugins/O-GrainScatter/NOTES.md:22` — strip the "(TACHES)" tag (keep heritage note).
- `plugins/O-GrainScatter/.planning/BRIEF.md:24,114` — strip the two "(TACHES)" tags.

## Out of scope / preserved
- Heritage/lineage notes in O-GrainScatter that Scatter's granular engine was the
  ancestor are kept as technical fact; only the "(TACHES)" attribution tag is removed.
- False-positive `attaches`/`attached` substring hits are left untouched.

## Steps
1. `git rm -r archive/tache_plugins/`
2. Edit the 5 doc files above.
3. Update STATE.md "Quick Tasks Completed" table.
4. Atomic commits (removal, then attribution edits, then STATE.md).

## Verification
- `grep -riE 't[aâ]ches' ...` returns no attribution hits (only `attaches` false positives).
- `git ls-files archive/tache_plugins/` returns empty.
- No dangling references to removed plugin names in active build configs.
