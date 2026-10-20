---
phase: quick-261019-ryp
plan: 02
subsystem: plugin-ui / value entry
tags: [value-entry, click-away, webview, O-Prism, O-Strata, O-simpleWavetable, O-Bells, O-Bowed, O-Lyrica, O-Wind]
status: complete
requires: [quick-261019-gk9 value entry (attachValueEntry in each page)]
provides: [click-away commit at every value-entry site in 7 plugins; O-Prism reference attachValueEntry now carries the fix]
affects: [PLUGINS.md version rows (follow-up, not touched)]
tech-stack:
  added: []
  patterns: ["document capture-phase pointerdown -> input.blur() when e.target !== input; removed as first statement of close()"]
key-files:
  created: []
  modified:
    - plugins/O-Prism/Source/ui/public/index.html
    - plugins/O-Prism/CMakeLists.txt
    - plugins/O-Prism/CHANGELOG.md
    - plugins/O-Strata/Source/ui/public/index.html
    - plugins/O-Strata/CMakeLists.txt
    - plugins/O-Strata/CHANGELOG.md
    - plugins/O-simpleWavetable/Source/ui/public/index.html
    - plugins/O-simpleWavetable/CMakeLists.txt
    - plugins/O-simpleWavetable/CHANGELOG.md
    - plugins/O-Bells/Resources/ui/index.html
    - plugins/O-Bells/CMakeLists.txt
    - plugins/O-Bells/CHANGELOG.md
    - plugins/O-Bowed/Resources/ui/index.html
    - plugins/O-Bowed/CMakeLists.txt
    - plugins/O-Bowed/CHANGELOG.md
    - plugins/O-Lyrica/Resources/ui/index.html
    - plugins/O-Lyrica/CMakeLists.txt
    - plugins/O-Lyrica/CHANGELOG.md
    - plugins/O-Wind/Resources/ui/index.html
    - plugins/O-Wind/CMakeLists.txt
    - plugins/O-Wind/CHANGELOG.md
decisions:
  - "O-Lyrica probe uses the Effects-tab FX knobs (--pre '#tab-effects'): the Sound-tab controls are native horizontal <input type=range>, which the probe's vertical drag cannot move and whose focus reads as a stray INPUT, so a range-pair probe FAILs on probe artifacts both before and after the fix"
  - "O-Bells and O-Wind probes use the Effects-tab FX knobs, the controls whose mousedown preventDefault() actually reproduces the bug; the Instrument/Sound sliders do not preventDefault"
metrics:
  duration: 32min
  completed: 2026-10-19
  tasks: 3
  files: 21
actuals:
  tokens: 4100      # chars/4 over the 7 commits' diffs (~16.4k chars)
  tasks: 3
  commits: 16       # MEASURED rev-list plan_head_before..HEAD on shared trunk: 7 are this plan's, 9 are parallel ryp executors' commits interleaved on main
plan_head_before: ecb9db2599969aff636b64c73d956c794af5abd9
plan_head_after: 64edd5f02f1add3fc99b55e702fd4aaa151fed97
---

# Phase quick-261019-ryp Plan 02: click-away commit for the Prism-lineage synths and Family-C instruments Summary

All seven plugins now commit the typed value and close the field when another dial is pressed. Each `attachValueEntry` registers a document capture-phase `pointerdown` handler (`e.target !== input` -> `input.blur()`). The handler is removed as the first statement of the single `close()` teardown that Enter, Esc, blur and the commit path all go through. O-Prism's suite-reference copy carries the fix too.

## Per-plugin results

| Plugin | Version | Commit | Entry site (add / remove) | Blur commits? | Probe before | Probe after | ryp-gate | syntax | boot pageErrors | auval |
|---|---|---|---|---|---|---|---|---|---|---|
| O-Prism | 1.30.2 -> 1.30.3 | 64edd5f0 | Source/ui/public/index.html:3129 / :3132 | yes | PASS (inert) | PASS | 0 -> 1 handler (=1) | OK | 0 / 0 | `aumu OuPr OuDv - O-Prism-dev` |
| O-Strata | 1.1.1 -> 1.1.2 | 2b7ac8d7 | Source/ui/public/index.html:3319 / :3322 | yes | PASS (inert) | PASS | 0 -> 1 (=1) | OK | 0 / 0 | `aumu OuSt OuDv - O-Strata-dev` |
| O-simpleWavetable | 1.0.3 -> 1.0.4 | 43424699 | Source/ui/public/index.html:1554 / :1557 | yes | PASS (inert: knob pointerdown calls knob.focus()) | PASS | 0 -> 1 | OK | 0 / 0 | `aumu OSiW OuDv - O-simpleWavetable-dev` |
| O-Bells | 4.9.1 -> 4.9.2 | 2b07b199 | Resources/ui/index.html:2337 / :2340 | yes | **FAIL (reproduces)** | PASS | 0 -> 1 | OK | 0 / 0 | `aumu OBls OuDv - O-Bells-dev` |
| O-Bowed | 1.10.1 -> 1.10.2 | 9f6a52f4 | Resources/ui/index.html:1738 / :1741 | yes | **FAIL (reproduces)** | PASS | 0 -> 1 | OK | 0 / 0 | `aumu OBwd OuDv - O-Bowed-dev` |
| O-Lyrica | 2.6.1 -> 2.6.2 | 6d80162d | Resources/ui/index.html:3132 / :3135 | yes | **FAIL (reproduces)** | PASS | 0 -> 1 (=1) | OK | 0 / 0 | `aumu OLyr OuDv - O-Lyrica-dev` |
| O-Wind | 1.22.1 -> 1.22.2 | 4a8cec0e | Resources/ui/index.html:2350 / :2353 | yes | **FAIL (reproduces)** | PASS | 0 -> 1 | OK | 0 / 0 | `aumu OWnd OuDv - O-Wind-dev` |

Each plugin has a single `attachValueEntry` implementation and that is the only site. Multiple callers (slider readouts, FX knobs, Lyrica's A4 REF knob, Lyrica's `js/app.js` Effects knobs) all route through it, so one edit per page covers every entry. I grepped each plugin's whole UI tree. No other file builds an `<input>`, except Lyrica's `js/app.js`, which only calls the page-global `attachValueEntry`. The installed AU bundle's `CFBundleShortVersionString` matched the new version for all seven before each commit.

### Probe args (exact; logs in /tmp/ryp-probe-<Name>{-before,}.log)

- O-Prism: `--plugin O-Prism --a '[data-knob="oscAPos"] .knob' --b '[data-knob="oscALevel"] .knob' --type 20`
- O-Strata: `--plugin O-Strata --a '[data-knob="oscAPos"] .knob' --b '[data-knob="oscALevel"] .knob' --type 20`
- O-simpleWavetable: `--plugin O-simpleWavetable --a '#knob-position' --b '#knob-lfo_rate' --type 20`
- O-Bells: `--plugin O-Bells --pre '.tab[data-tab="effects"]' --a '#delayTimeKnob .knob-visual' --b '#delayFeedbackKnob .knob-visual' --type 20`
- O-Bowed: `--plugin O-Bowed --a '[data-param="bowSpeed"] .knob-wrapper' --b '[data-param="bowPressure"] .knob-wrapper' --type 0.6`
- O-Lyrica: `--plugin O-Lyrica --pre '#tab-effects' --a '#chorusRateKnob .knob-visual' --b '#chorusDepthKnob .knob-visual' --type 0.2`
- O-Wind: `--plugin O-Wind --pre '.tab-btn[data-tab="effects"]' --a '#delayTimeKnob .knob-visual' --b '#delayFeedbackKnob .knob-visual' --type 20`

The FAIL baselines all failed the same three checks: field not closed by the press on B, typed value not committed, field still focused after the drag. The fix turned all three green.

## Non-entry inputs left alone

- **O-Prism :3806 / :3843** (now shifted +8), **O-Strata :4133 / :4170**: the mod-matrix row's on/off `checkbox` and its amount `range` slider. Neither commits on blur, so a dial press cannot strand them. Gate arg `=1`.
- **O-Lyrica :3355** (`genRow`): a `type=number` field in the generator dialog. It is read when the dialog's action runs, not committed on blur. Gate arg `=1`.

## Deviations from Plan

### Probe selector choices (no code impact)

**1. O-Lyrica probe moved to the Effects-tab knobs.** The facts table suggested a generic stub probe, and the obvious pair is the Sound-tab `#brightness` / `#timbre`. Those are native horizontal `<input type="range">`. The probe's drag is vertical, so it can't move them, and its "no INPUT left focused" check counts the focused range itself. That pair fails 2 checks both before and after the fix: logs `/tmp/ryp-probe-O-Lyrica-range-before.log` and `/tmp/ryp-probe-O-Lyrica-range.log`. On both runs the field did close and commit, so the bug is inert on the ranges. The recorded probe uses the `js/app.js` FX knobs, whose mousedown `preventDefault()`s. That pair reproduces the bug (FAIL) before the fix and passes after.

**2. O-Bells / O-Wind probes use Effects-tab FX knobs** for the same reason: they are the controls that `preventDefault()` their mousedown. The Instrument/Sound sliders do not.

### Process notes

- **Shared scratchpad:** a parallel executor overwrote `scratchpad/bump.py` mid-run, and my first bump attempt failed harmlessly on its argument shape. After that I used uniquely named helpers (`scratchpad/ryp02-bump.py`, `/tmp/ryp02-build.sh`, `/tmp/ryp02-commit.sh`).
- **Build order:** I launched builds as each plugin passed its gates. The lock grants in random order, so the commit order was Bowed, simpleWavetable, Bells, Wind, Strata, Lyrica, Prism, not task order. Every build held the lock alone, and every commit followed its own green build.

None of the code deviates from the plan: every edit is the canonical D-01 block, with no changes to drag handlers, Alt-click resets, other document listeners, CSS or i18n.

## Follow-ups

- PLUGINS.md version rows: O-Prism 1.30.3, O-Strata 1.1.2, O-simpleWavetable 1.0.4, O-Bells 4.9.2, O-Bowed 1.10.2, O-Lyrica 2.6.2, O-Wind 1.22.2.
- The five excluded plugins (O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM, D-05) still need the click-away fix once their sessions commit.
- Nothing tagged. PLUGINS.md and the excluded plugins were not touched.

## Known Stubs

None.

## Self-Check: PASSED

- All 7 commits present on main (64edd5f0, 2b7ac8d7, 43424699, 2b07b199, 9f6a52f4, 6d80162d, 4a8cec0e), each listing only `plugins/<Name>/` paths. `git status --short` is clean for all 7 plugin folders.
- The plan's three `<automated>` verify blocks re-run over all 7 plugins: PASS (gate, syntax, boot, probe log PASS, RYP-EXIT=0, clean status, subject prefix, path-scoped files).
