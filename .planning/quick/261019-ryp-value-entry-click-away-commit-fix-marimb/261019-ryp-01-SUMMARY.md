---
phase: quick-261019-ryp
plan: 01
subsystem: plugin-ui (value entry)
tags: [webview, value-entry, click-away, patch-release]
status: complete
requires: [quick-261019-gk9 value entry]
provides: [click-away commit in 8 plugins]
affects: [O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bitrot, O-Chorus, O-Comp, O-Emulator, O-Contrabass]
tech-stack:
  added: []
  patterns: ["document capture-phase pointerdown -> input.blur() when e.target !== input, removed first thing in close()"]
key-files:
  modified:
    - plugins/O-AnalogEQ/Source/ui/public/index.html
    - plugins/O-AnalogSaturation/Source/ui/public/index.html
    - plugins/O-Bass/Source/ui/public/index.html
    - plugins/O-Bitrot/Source/ui/public/index.html
    - plugins/O-Chorus/Source/ui/public/index.html
    - plugins/O-Comp/Source/ui/public/index.html
    - plugins/O-Emulator/Source/ui/public/index.html
    - plugins/O-Contrabass/Source/ui/public/index.html
    - plugins/*/CMakeLists.txt (8, VERSION patch bump)
    - plugins/*/CHANGELOG.md (8, new top entry)
decisions:
  - "O-AnalogSaturation has a single dial, so its probe presses the same dial outside the centred entry chip (--b '#intensityKnob' --b-offset 0,40)."
  - "O-Emulator keeps its double-quote style in the inserted lines; the gate accepts it."
metrics:
  duration: ~95 min (most of it waiting on the shared build lock)
  completed: 2026-10-19
actuals:
  tokens: 9000     # chars/4 over the realized 24-file diff (126 insertions, 8 deletions)
  tasks: 3
  commits: 8       # MEASURED: git rev-list --count ecb9db25..HEAD -- <the 8 plugin folders>; the unscoped count is 15 because plans 02-05 commit to the same main concurrently
plan_head_before: ecb9db2599969aff636b64c73d956c794af5abd9
plan_head_after: 743263b71e883a9da4825b79bddd8b451603fc87
---

# Phase quick-261019-ryp Plan 01: click-away commit, 8 plugins

**Pressing another dial while a value-entry field is open now applies the typed value and closes the field. Fixed in O-AnalogEQ, O-AnalogSaturation, O-Bass, O-Bitrot, O-Chorus, O-Comp, O-Emulator and O-Contrabass. Each was patch-bumped, built under the shared lock, installed, listed by auval, and committed path-scoped.**

## Per-plugin results

Every page has one entry site, `attachValueEntry`. In every page `blur` was already wired to `commit` (Prism semantics), so the handler just calls `input.blur()`. Enter, Esc and blur all go through `close()`, and each page has exactly one `input.remove()`, which sits inside `close()`. The removal line is the first statement after `closed = true;` in that teardown.

| Plugin | Version | Commit | Field / add / remove (index.html line) | Probe before | Probe after |
|---|---|---|---|---|---|
| O-AnalogEQ | 1.5.5 -> 1.5.6 | 3fc54ee2 | 1280 / 1304 / 1309 | FAIL (bug reproduces) | PASS |
| O-AnalogSaturation | 1.8.1 -> 1.8.2 | 9d52c071 | 1089 / 1113 / 1118 | FAIL | PASS |
| O-Bass | 1.8.1 -> 1.8.2 | 8a16bb5c | 1286 / 1310 / 1315 | FAIL | PASS |
| O-Bitrot | 1.17.2 -> 1.17.3 | e1e187d9 | 1749 / 1773 / 1778 | PASS (inert) | PASS |
| O-Chorus | 1.10.1 -> 1.10.2 | 2629af21 | 1262 / 1286 / 1291 | FAIL | PASS |
| O-Comp | 1.11.1 -> 1.11.2 | 8259f272 | 1397 / 1421 / 1426 | FAIL | PASS |
| O-Emulator | 1.5.0 -> 1.5.1 | 89cb26d1 | 973 / 997 / 1002 | PASS (inert) | PASS |
| O-Contrabass | 1.12.1 -> 1.12.2 | 743263b7 | 1789 / 1812 / 1817 | FAIL | PASS |

The probe used the same arguments before and after the fix. Logs: `/tmp/ryp-probe-<Name>-before.log` and `/tmp/ryp-probe-<Name>.log`.

- O-AnalogEQ: `--a '[data-param-outer="lf_freq"]' --b '[data-param-outer="lmf_freq"]' --type 3`
- O-AnalogSaturation: `--a '#intensityKnob' --b '#intensityKnob' --b-offset 0,40 --type 20`. This page has only one dial, so B is a press on that same dial outside the entry chip. The bug reproduced there too.
- O-Bass: `--a '#frequencyKnob' --b '#enhanceKnob' --type 60`
- O-Bitrot (per-plugin stub): `--a '.knob[data-param="TAPE_PROB"]' --b '.knob[data-param="TAPE_STOP_PROB"]' --ra '.knob[data-param="TAPE_PROB"] + .ro' --rb '.knob[data-param="TAPE_STOP_PROB"] + .ro' --type 60`
- O-Chorus: `--a '.knob[data-param="rate"]' --b '.knob[data-param="depth"]' --type 3`
- O-Comp: `--a '#threshold-knob' --b '#ratio-knob' --type -10`
- O-Emulator: `--a '.knob[data-param="crush"]' --b '.knob[data-param="age"]' --type 10`
- O-Contrabass: `--a '[data-param="BOW_SPEED"] .knob-wrapper' --b '[data-param="BOW_PRESSURE"] .knob-wrapper' --type 0.2`

## Gates (all eight)

- `ryp-gate.js`: baseline `FAIL-HANDLER-COUNT ... click-away handlers=0`, after the fix `OK ... click-away handlers=1`.
- `gk9-gate.js --syntax`: SYNTAX-OK. `--boot`: pageErrors=0 both before and after.
- Plugin-local headless tests: `O-Bitrot/tests/ui_drag_tooltip_check.js`, `ui_preset_menu_check.js`, `ui_tooltip_clamp_check.js` and `O-Contrabass/tests/ui_frontend_check.js` all exited 0 before and after, with 0 FAIL lines.
- Builds: every `/tmp/ryp-build-<Name>.log` ends `RYP-EXIT=0` with an auval line above it (`aufx`, or `aumu` for O-Contrabass, `... <Name>-dev`). The installed `-dev.component` CFBundleShortVersionString matches the new version for all eight.
- Each commit's `show --stat` lists only `plugins/<Name>/` paths. No tags. PLUGINS.md is not in any commit. None of the five excluded plugins were touched.

## Deviations from Plan

None. The plan was executed as written. The only judgement call was O-AnalogSaturation's probe target: the page has one dial, so B is that same dial (see decisions).

## Known Stubs

None.

## Follow-ups

- PLUGINS.md version rows (not touched, per D-05): O-AnalogEQ 1.5.6, O-AnalogSaturation 1.8.2, O-Bass 1.8.2, O-Bitrot 1.17.3, O-Chorus 1.10.2, O-Comp 1.11.2, O-Emulator 1.5.1, O-Contrabass 1.12.2.
- The five excluded plugins (O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM) still need the click-away fix once their sessions commit.
- Pre-existing, not touched: O-AnalogSaturation's in-page `.version-label` still reads `v1.8.0`.

## Self-Check: PASSED

- All 8 entry files exist and pass ryp-gate.
- All 8 commits were found in `git log`: 3fc54ee2, 9d52c071, 8a16bb5c, e1e187d9, 2629af21, 8259f272, 89cb26d1, 743263b7.
