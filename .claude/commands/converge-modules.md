---
name: converge-modules
description: Work through the module fork-vs-drift upgrade plan (261029-md9 FORK-REVIEW) one step or plugin per fresh context
argument-hint: "[step 1-5] [PluginName?]"
---

# /converge-modules

Carry out the upgrade plan in `.planning/quick/261029-md9-do-a-thorough-review-through-this-repo-f/261029-md9-FORK-REVIEW.md` ("Recommended upgrades", steps 1–5). Step 6 is "leave as intentional forks". Never touch those copies.

## Progress file

The progress file is `.planning/quick/261029-md9-do-a-thorough-review-through-this-repo-f/CONVERGENCE.md`.

- If it doesn't exist, create it from FORK-REVIEW's per-copy table: one checkbox per (step, plugin, file) with its class.
- After each plugin, tick its boxes and add the commit SHA.
- This file is the only state shared between fresh contexts. Keep it accurate.

## Routing

- **No args:** open CONVERGENCE.md, find the first step that has unticked boxes, and pick its first unticked plugin.
- **`[step]`:** take the first unticked plugin in that step.
- **`[step] [PluginName]`:** do exactly that plugin.

**One plugin per invocation, except step 1.** Step 1 is module work, done as one unit.

**Steps run in order.** Steps 2 and 3 never start before step 1 is ticked, because converging a fork+ahead copy before its fixes are upstreamed would regress it. If the requested step isn't unblocked yet, say so and stop.

## Per step

1. **Upstream into scala-tuning-engine 3.3.0.**
   - Port the fixes FORK-REVIEW lists from O-Formant, O-Prism/O-Strata and O-Bells into `modules/`.
   - Give each fix a module unit test where the module has tests.
   - Bump the module version in `modules/registry.yaml` and the module CHANGELOG.
   - The plugins aren't changed in this step.
   - Gate: module tests pass, and each source plugin's copy and the new module are equivalent for the fixed paths.
2. **preset-manager drift.** Move a plugin's `OuariconPresetManager.h` and/or `preset-manager.js` to the module copy.
   - O-DigiDelay first needs bypass-param exclusion upstreamed, or keep it via `setExcludedParameters`; see the FORK-REVIEW caveat.
   - Gate with the plugin's state-persistence and factory-preset tests. Watch for the memory patterns: the factory-bank sentinel, reset-to-defaults-before-apply, and the stale `<CustomState>` child.
3. **scala 3.2.0 KBM reference and the 1c5dca67 Scala-mode fix.** Port these per plugin, after step 1.
4. **O-Lyrica small scala files.** Replace them with the module copies, and change the one `toHTML(engine, pluginName)` call site.
5. **grain-engine CounterRng.** Migrate O-Freeze, O-TextureForge, O-simpleGrain and O-GrainScatter.
   - Gate: the render output must be **bit-identical** to before. Use a null test on a seeded render, or the plugin's goldens.
   - O-GrainScatter keeps its Stream/slot enums.
   - Skip O-simpleGrain while it's in flight (`git status` shows it dirty).

## How each plugin change runs

Steps 2–5 change plugin code, so they go through the `plugin-improve` engine (`Skill("plugin-improve")`). That gives a PATCH version bump, a `backups/` snapshot, a CHANGELOG entry in `## [X.Y.Z]` form, a STATUS.md `version:` line, and `./scripts/build-and-install.sh <Plugin>`, which handles the AU cache and the dual-variant sweep.

- Treat the FORK-REVIEW entry as the completed investigation: verify it, then apply it. Don't re-research.
- Update that module's `used_by` with `bash scripts/regen-registry-used-by.sh` and commit it alongside.
- Don't touch any plugin path that `git status` shows as another session's in-flight work.
- Commit through a temp index scoped to that plugin's paths plus `modules/registry.yaml` and CONVERGENCE.md. Use the `pc` helper from `/finish-cleanup`, or `GIT_INDEX_FILE` the same way. Never `git add -A`.
- Never tag. Releases go through `/publish`.

## Handoff, then STOP

Report the plugin or module done, its version, the gate results with numbers, and the SHA. Then:

```
Next Up
  Step 1: /clear
  Step 2: /converge-modules <step> <next unticked plugin from CONVERGENCE.md>
Alternatives:
  /publish <Plugin>          — release what was just converged
  /improve O-Contrabass      — oversampler overrun on oversized host block
```

When every box is ticked, the Step 2 command is `/finish-cleanup`'s close-out: add "Convergence complete" to the AUDIT "Closed" section.
