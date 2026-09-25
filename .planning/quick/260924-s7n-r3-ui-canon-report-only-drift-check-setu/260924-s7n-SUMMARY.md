---
phase: quick-260924-s7n
plan: 01
subsystem: ui-tooling
status: complete
tags: [R3, ui-canon, drift-report, ci, report-only]
requires: [scripts/serve-ui.js, scripts/i18n-extract.js]
provides: [scripts/ui-canon.js, scripts/check-ui-canon.js, ui-static-gates report step]
affects: [.github/workflows/ui-static-gates.yml]
tech-stack:
  added: []
  patterns: [canon-as-data with //| line-comment storage, machine-emitted canon block, paired positive/negative self-test controls]
key-files:
  created: [scripts/ui-canon.js, scripts/check-ui-canon.js]
  modified: [.github/workflows/ui-static-gates.yml]
decisions:
  - "Canon anchored at O-ReverseDelay ed1fc465e67973c3df2e365a7a1f70341b3aecac (v1.20.0), embedded as data; re-anchor = --emit-canon + reviewed diff"
  - "Alias list kept exactly as planned; showTip/hideTip (4 arrow-bound plugins) and applyTooltipsEnabled (O-Tapestop) recorded as follow-up alias candidates, not added"
metrics:
  duration: ~35 min
  completed: 2026-09-24
plan_head_before: 7c0baac560fdac06630c26db53da49593fa886c7
actuals:
  tokens: 18000
  tasks: 3
  commits: 3
---

# Phase quick-260924-s7n Plan 01: R3 UI canon report-only drift check Summary

This adds a report-only drift tool. `scripts/ui-canon.js` holds the O-ReverseDelay canon for 9 functions as data, machine-emitted from `ed1fc465` (v1.20.0). `scripts/check-ui-canon.js` uses a real JS lexer and function extractor to classify each plugin's tooltip renderer, hover-help init, knob binding and knob visual as canon, shape, v:hash, nd or absent. It also checks every tracked `modules/` copy by sha256 and reports MATCH or FORK. The report runs as a continue-on-error last step of `ui-static-gates`.

## Canon anchor

- `UI_CANON_SOURCE.commit` = `ed1fc465e67973c3df2e365a7a1f70341b3aecac`. This is the last commit touching `plugins/O-ReverseDelay/Source/ui/public/js/app.js`, and its CMake `VERSION` is 1.20.0.
- The block was produced twice by `--emit-canon ed1fc465…` and spliced in with a node script: once in Task 1 (1 function) and once in Task 2 (9 functions). It was never hand-typed. Self-test N4 re-emits and byte-compares it, and it passes.

## Commits

| Task | Commit | Message |
|---|---|---|
| 1 (tracer) | 817348e9 | feat(quick-260924-s7n): R3 ui-canon tracer — knob-visual canon + report-only CI step |
| 2 | fcba1b03 | feat(quick-260924-s7n): R3 ui-canon — tooltip renderer, hover-help init, knob binding components + paired self-test controls |
| 3 | 20ece0b7 | feat(quick-260924-s7n): R3 check-ui-canon — tracked module-copy fork census, direct embeds, --json, exit-0 resilience |

The three commits touch exactly `scripts/ui-canon.js`, `scripts/check-ui-canon.js` and `.github/workflows/ui-static-gates.yml`. Nothing under `plugins/` changed. Other sessions' O-Formant and O-Prism work-in-progress was left unstaged and untouched.

## Report at HEAD (git-archive snapshot)

`--self-test`: `SELF-TEST PASS 8/8 (skipped 0)`. The tests are L1 (with the naive-counter negative control), L2, N1–N4, G1 and M1.

Per-component counts lines:

```
tooltip-renderer: canon 1 · shape 0 · variant 39 (distinct 27) · non-decl 0 · absent 4 · error 0
hover-help-init: canon 1 · shape 0 · variant 38 (distinct 30) · non-decl 4 · absent 1 · error 0
knob-binding: canon 1 · shape 0 · variant 21 (distinct 19) · non-decl 2 · absent 20 · error 0
knob-visual: canon 1 · shape 0 · variant 19 (distinct 14) · non-decl 0 · absent 24 · error 0
```

Per-name counts compared with an independent `git grep` over HEAD. All match:

| Name | Report | git grep |
|---|---|---|
| setupTooltips | 29 | 29 |
| initializeTipsToggle | 23 | 23 |
| bindKnob | 15 | 15 |
| updateKnobVisual | 20 | 20 |

The other names match the planning census too: showTooltip 10, initTooltips 3, initializeTooltips 8, applyTipsEnabled 31, initializeHelpToggle 8, setTooltipsEnabled 8, setupKnob 7 and initTipsToggle 1.

Distinct bodies compared with the review's 18 / 22 / 13 / 15. These counts are informational. Read per name, from the variant groups in the report, they match the review exactly:

| Review name | Review distinct | This report (per-name reading) | Component-level distinct (grouped names) |
|---|---|---|---|
| setupTooltips | 18 | 18 across 29 plugins | tooltip-renderer: 27 variants + canon |
| initializeTipsToggle | 22 | 22 across 23 (hashed with its applyTipsEnabled pair) | hover-help-init: 30 variants + canon |
| bindKnob | 13 | 12 variants + canon = 13 across 15 | knob-binding: 19 variants + canon |
| updateKnobVisual | 15 | 14 variants + canon = 15 across 20 | knob-visual: 14 variants + canon |

Module-copy census. The snapshot run used the filesystem walk and the working-tree run used `git ls-files`. The two agree, and both match an independent `git ls-tree` blob comparison:

```
preset-manager.js: 12 tracked copies — 6 MATCH, 6 FORK
tuning-panel.css: 3 tracked copies — 0 MATCH, 3 FORK
tuning-panel.js: 4 tracked copies — 0 MATCH, 4 FORK
total: 19 tracked copies — 6 MATCH, 13 FORK; direct embeds in 8 plugins
```

- **preset-manager forks:** O-Bass, O-Comp, O-DigiDelay, O-FreqPulse, O-Polystutter and O-SpectralShaper. All six are `ships yes / sync frozen`, as in review §1.3 and A.2. O-FreqPulse has the same line count as the module (447/447) but a different hash.
- **preset-manager MATCH:** O-AnalogEQ, O-Chorus, O-Detune, O-SimpleReverb and O-Tremolo are `sync configure`. O-Orbit is `sync frozen`, because its copy lives at `Resources/ui/js/modules/` and a configure run will not refresh it. O-SimpleReverb reads `ships no`: its copy is not a `juce_add_binary_data` SOURCES entry.

**Direct embeds (8 plugins, as in review §1.3):** O-Bassoon, O-Bowed, O-Contrabass, O-Marimba, O-MicrotonalSampler, O-Reed, O-ReverseDelay and O-Wind.

**Resilience:** every one of these runs exits 0 with the expected output:
- a missing `--repo-root` prints an ERROR line;
- an empty root prints `WARNING: 0 plugins scanned …`;
- O-Chorus with `index.html` replaced by a directory shows `ERR` cells plus an `── errors` entry, while O-ReverseDelay still reads canon ×4;
- an unknown `--plugin` prints a message;
- `--emit-canon --output=<file>` is rejected by the rev pattern, so git never runs and no file is written.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Block-comment terminator in the new script's own doc header**
- **Found during:** Task 1
- **Issue:** The planned header text `Source/ui/src/**/*.js` contains `*/`, which closed the header comment and caused a SyntaxError. It is the same trap the plan documents for the canon itself.
- **Fix:** The header now reads "every .js under <plugin>/Source/ui/src".
- **Commit:** 817348e9

**2. [Rule 1 - Bug] `process.exit(0)` could truncate piped output on macOS**
- **Found during:** Task 1
- **Issue:** Stdout to a pipe is asynchronous on macOS, so an explicit `process.exit` can cut off a long report.
- **Fix:** The script sets `process.exitCode = 0` instead. Exit 0 is still guaranteed in every mode.
- **Commit:** 817348e9

**3. [Rule 1 - Bug] Write tool turned `\u00a0` / `\ufeff` / `\u2028` / `\u2029` escapes into literal characters**
- **Found during:** Task 1
- **Issue:** The literal U+2028/2029 characters broke the whitespace regex literal and caused a SyntaxError.
- **Fix:** The escapes were rewritten as ASCII `\uXXXX` sequences with a Python pass. The only non-ASCII left in the file is prose punctuation.
- **Commit:** 817348e9

No other deviations. `COMPONENTS`, the categories, the report formats and the CLI follow the plan as written.

## Observations (follow-up candidates, not fixed: out of scope)

- **Four plugins read `—` for tooltip-renderer: O-Bitrot, O-Contrabass, O-Octagon and O-Orbit.** They do have a renderer, but it is arrow-bound under names outside the planned alias list (`const showTip =` and `const hideTip =`). Their hover-help cell correctly reads `nd`. Adding `showTip` and `hideTip` as tooltip-renderer aliases would turn these cells into `nd [showTip+hideTip]`.
- **O-Tapestop reads `—` for hover-help-init.** Its switch is `function applyTooltipsEnabled`, which is not in the alias list.
- **No plugin other than O-ReverseDelay reads `canon` or `shape` on any component.** At this anchor, the burn-down starts at 1/44 for every component.

## Known Stubs

None.

## Threat Flags

None. The subprocess surface (`git show`, `rev-parse`, `ls-files`, `cat-file`) is covered by T-s7n-01. All calls use `execFileSync` with an argv array, no shell, and a validated rev.

## Self-Check: PASSED

- FOUND: scripts/ui-canon.js
- FOUND: scripts/check-ui-canon.js
- FOUND: .github/workflows/ui-static-gates.yml (step `ui-canon drift report (report-only)`, last, continue-on-error)
- FOUND commits: 817348e9, fcba1b03, 20ece0b7 (`git rev-list --count 7c0baac5..HEAD` = 3)
