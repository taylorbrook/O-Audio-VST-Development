---
task: Remove TÂCHES attribution and all taches plugins from the repo
slug: 260720-rtc-remove-taches-attribution-and-plugins
date: 2026-07-20
mode: quick
status: complete
---

# Summary — Remove TÂCHES attribution + taches plugins

## What was done

### 1. Removed all taches plugins
- Deleted `archive/tache_plugins/` in full: 348 git-tracked files + 43 gitignored
  mockup-test leftovers (~6.9 MB), 17 plugins — AngelGrain, AutoClip, ClapMachine,
  DriveVerb, Drum808, DrumRoulette, FlutterVerb, GainKnob, LushPad, LushVerb,
  MinimalKick, OrganicHats, PadForge, Scatter, TapeAge, TEMPLATE-HEADLESS-EDITOR,
  Words. `archive/` became empty and was dropped from the tree.
- These were already archived out of the active `plugins/` build, so no CMake/build
  wiring depended on them. Fully recoverable from git history.

### 2. Removed TÂCHES/TACHES attribution
- `README.md` — deleted the `#VST building system based upon **[TÂCHES]**` line.
- `PLUGINS.md` — deleted the "Plugins created by **[TÂCHES]**" heading and the entire
  taches-plugins registry table (18 rows).
- `.planning/research/STACK.md` — stripped "by TACHES" from the GSD description.
- `plugins/O-GrainScatter/NOTES.md` — stripped the "(TACHES)" tag (heritage note kept).
- `plugins/O-GrainScatter/.planning/BRIEF.md` — stripped both "(TACHES)" tags.

## Verification
- `grep -riE 't[aâ]ches'` across source/docs returns only the "de**taches**" false
  positive (O-Prism CHANGELOG) and this task's own PLAN/SUMMARY — no attribution left.
- `git ls-files archive/` → empty. `archive/` gone from disk.
- No dangling references to removed plugins in active build configs.

## Deferred / not done (scope fork surfaced to user)
The taches plugin **names** (TapeAge, GainKnob, FlutterVerb, OrganicHats, LushPad, …)
still appear as *illustrative examples* in ~60+ skill/command docs, aesthetic
templates, historical troubleshooting records (documenting real solved problems), and
other plugins' prior-art references. These are not TÂCHES credit and touching them
risks damaging working documentation and valuable solved-problem knowledge. Left as-is
pending an explicit user decision on whether to scrub example/historical name mentions.

## Commits
- (removal)      — `git rm -r archive/tache_plugins/` + disk sweep of ignored leftovers
- (attribution)  — strip TÂCHES credit from README, PLUGINS.md, STACK.md, O-GrainScatter
- (planning)     — PLAN.md, SUMMARY.md, STATE.md quick-tasks row
