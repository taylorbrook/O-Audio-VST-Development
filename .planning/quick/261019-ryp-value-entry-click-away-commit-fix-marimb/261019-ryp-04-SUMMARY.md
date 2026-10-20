---
phase: quick-261019-ryp
plan: 04
subsystem: webview-ui
tags: [value-entry, click-away, ui, js]
status: complete
requires: []
provides: [click-away commit in 8 js/app.js effect plugins]
affects: [O-GrainScatter, O-FreqPulse, O-Octagon, O-Orbit, O-ReverseDelay, O-Tapestop, O-MultiBandCompressor, O-Polystutter]
tech-stack:
  added: []
  patterns: ["document capture-phase pointerdown -> field.blur() when e.target !== field, removed first in the teardown"]
key-files:
  modified:
    - plugins/O-GrainScatter/Source/ui/public/js/app.js
    - plugins/O-FreqPulse/Resources/ui/js/app.js
    - plugins/O-Octagon/Source/ui/public/js/app.js
    - plugins/O-Orbit/Resources/ui/js/app.js
    - plugins/O-ReverseDelay/Source/ui/public/js/app.js
    - plugins/O-Tapestop/Source/ui/public/js/app.js
    - plugins/O-MultiBandCompressor/Source/ui/public/js/app.js
    - plugins/O-Polystutter/Source/ui/public/js/parameter-bindings.js
    - (each plugin's CMakeLists.txt + CHANGELOG.md)
decisions:
  - "FreqPulse and Octagon probes use the controls that actually preventDefault (FreqPulse step cell, Octagon room-plan puck); their native range sliders let focus move, so the bug never reproduced against them"
  - "FreqPulse and Octagon CHANGELOG wording names the step cell / puck instead of 'another dial' (neither has rotary dials)"
  - "Polystutter keeps .blur(): its blur handler confirms after a 10 ms setTimeout, so the commit lands ~10 ms into the new drag rather than strictly before it (plan instruction; harmless)"
metrics:
  completed: 2026-10-19
  tasks: 3
actuals:
  tokens: 3200
  tasks: 3
  commits: 8
plan_head_before: ecb9db2599969aff636b64c73d956c794af5abd9
plan_head_after: b274e523ee4c0fa067de8721ff2d24e82ea4fcc5
commits_note: "HEAD advanced 35 commits during the run because 4 parallel executors share main; 8 are this plan's (listed below). rev-list count over the shared range would be 35."
---

# Phase quick-261019-ryp Plan 04: js/app.js effects click-away commit Summary

A press anywhere outside an open value-entry field now blurs (commits) it via a document capture-phase `pointerdown` handler. The handler is removed first thing in each site's teardown. All eight plugins went from probe RED to GREEN and were built, installed, auval-listed and committed path-scoped.

## Per plugin

| Plugin | Old -> New | Commit | Entry site fixed (register line) | Blur commits already? | Probe before | Probe after |
|---|---|---|---|---|---|---|
| O-GrainScatter | 3.0.2 -> 3.0.3 | a65b7b68 | `Source/ui/public/js/app.js` attachValueEntry, :216 | yes (`blur -> commit`) | FAIL (3) | PASS |
| O-FreqPulse | 1.20.1 -> 1.20.2 | 24b37c1b | `Resources/ui/js/app.js` attachValueEntry, :282 (removal before the floating-host early return) | yes | FAIL (3) | PASS |
| O-Octagon | 1.16.1 -> 1.16.2 | a6db5f74 | `Source/ui/public/js/app.js` attachValueEntry (field var `field`), :344 | yes | FAIL (3) | PASS |
| O-Orbit | 1.4.3 -> 1.4.4 | b44f5960 | `Resources/ui/js/app.js` attachValueEntry, :424 | yes | FAIL (3) | PASS |
| O-ReverseDelay | 1.22.3 -> 1.22.4 | 5beff464 | `Source/ui/public/js/app.js` attachValueEntry, :478 | yes | FAIL (3) | PASS |
| O-Tapestop | 1.7.2 -> 1.7.3 | 0ba1fc46 | `Source/ui/public/js/app.js` attachValueEntry, :264 | yes | FAIL (3) | PASS |
| O-MultiBandCompressor | 1.14.0 -> 1.14.1 | 8fea90a0 | `Source/ui/public/js/app.js` attachValueEntry, :581 (`set(OMBC_VERSION …)` bumped; the `v1.14.0 (R5)` comment left alone) | yes | FAIL (3) | PASS |
| O-Polystutter | 1.16.1 -> 1.16.2 | b274e523 | `Source/ui/public/js/parameter-bindings.js` startKnobTextInput registers at :114; handler stored on `activeTextInput`; `cleanupTextInput` removes it at :203 (confirm, cancel, invalid-input -> cancel, and re-entry all end there) | yes (10 ms delayed confirm) | FAIL (3) | PASS |

Probe arg lines (identical before and after; logs at `/tmp/ryp-probe-<Name>-before.log` and `/tmp/ryp-probe-<Name>.log`):
- O-GrainScatter: `--a '.knob[data-param="grain_size"]' --b '.knob[data-param="density"]' --type 20`
- O-FreqPulse: `--a '#mix' --b '.step-cell[data-step="4"]' --type 20`
- O-Octagon: `--a '#val-width' --b '#puck' --type 3 --ra '#val-width' --rb '#val-srcY'`
- O-Orbit: `--a '#speed' --b '#width' --type 3`
- O-ReverseDelay: `--a '#knob-density' --b '#knob-grainSize' --type 3 --ra '#val-density' --rb '#val-grainSize'`
- O-Tapestop: `--a '#knob-STOP_CURVE' --b '#knob-START_CURVE' --type 20 --ra '#val-STOP_CURVE' --rb '#val-START_CURVE'`
- O-MultiBandCompressor: `--a '#low-threshold' --b '#low-ratio' --type -30`
- O-Polystutter: `--pre '#lane1_enabled' --a '#lane1_decay' --b '#lane1_repeats' --type 20` (lane 1 is disabled in the stub, so its knobs are `pointer-events: none` until the header is clicked)

Gates (all eight): `ryp-gate` OK (1 handler, 1 site), `gk9-gate --syntax` OK, `--boot` pageErrors=0 before and after. Builds: every `/tmp/ryp-build-<Name>.log` ends `RYP-EXIT=0` with the auval line above it (`aufx` x7, `aumf OuPs` for Polystutter). O-Orbit's old `Cannot open component` did not come back.

Non-entry inputs left alone: O-FreqPulse's band-mix (`band-mix-<id>`) and divider (`divider-slider-<id>`) `type=range` sliders. They are the controls the entry opens over, not commit-on-blur fields, so they get no handler (gate arg `=1`).

Extra regression checks: `plugins/O-Octagon/tests/ui_frontend_check.js` printed the same single failure before and after (section [30], `GainStage.cpp has exactly 20 reset() sites — 23`). That check is about C++ and was already failing. `ui_layout_check.js` passes 39/39. `plugins/O-ReverseDelay/tests/ui_frontend_check.js` passes in full.

## Deviations from Plan

1. **[Rule 3 - probe target] O-FreqPulse B control.** The plan expected a "generic stub" dial pair. FreqPulse has no rotary dials, and its range sliders let focus move, so a slider-to-slider probe found the bug inert. It also failed for reasons that have nothing to do with the fix: the probe drags vertically across a horizontal range, and a focused range `<input>` counts as a "stray field". The step cells do call preventDefault on mousedown and drag vertically, so B = `.step-cell[data-step="4"]` reproduces the bug (RED before, PASS after).
2. **[Rule 3 - probe target] O-Octagon A/B.** A is the readout `#val-width`, because a double-click on the native range moves its thumb before the field opens and would muddy the readout-A check. B is the room-plan `#puck`, whose pointerdown preventDefaults. Its sliders do not.
3. **CHANGELOG wording.** FreqPulse and Octagon name the step cell and the puck instead of "a different dial". The other six use the plan template word for word. The MBC heading uses a hyphen, matching that file.
4. **Process note.** For FreqPulse and Polystutter the edit went in before the baseline probe. The baseline was taken on the HEAD version of the file (`git show HEAD:<file> > <file>`), then the fixed copy was restored from the scratchpad. Both baselines are genuine pre-fix runs.

## Known Stubs

None.

## Follow-ups

- PLUGINS.md version rows: GrainScatter 3.0.3, FreqPulse 1.20.2, Octagon 1.16.2, Orbit 1.4.4, ReverseDelay 1.22.4, Tapestop 1.7.3, MultiBandCompressor 1.14.1, Polystutter 1.16.2.
- The five excluded plugins (O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM) still need the click-away fix once their sessions commit.
- Pre-existing, not touched: on FreqPulse and Octagon a double-click on a native range slider jumps the thumb to the click point before the entry opens (the probe logs it as "written while typing"). Octagon `ui_frontend_check` section [30] (GainStage reset count 23 vs 20).

## Self-Check: PASSED

- All 8 commits found on main: a65b7b68, 24b37c1b, a6db5f74, b44f5960, 5beff464, 0ba1fc46, 8fea90a0, b274e523. Each touches only `plugins/<Name>/`. No PLUGINS.md, no tags.
- `git status --short -- plugins/<Name>` is empty for all eight.
