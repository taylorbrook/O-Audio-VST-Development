---
phase: quick-261019-ryp
plan: 05
subsystem: webview-ui / shared effect modules
status: complete
tags: [value-entry, click-away, alt-click-reset, modules/effects, O-Marimba]
requires: []
provides:
  - "modules/effects analog-eq-unit 1.4.0, compressor-unit 1.4.0, reverb-unit 1.1.0: double-click value entry + Alt-click gesture reset"
  - "click-away commit (D-01) in O-Marimba, O-Formant, O-Texture, O-MicrotonalSampler, O-TextureForge"
affects: [O-Marimba, O-Formant, O-Texture, O-MicrotonalSampler, O-TextureForge, modules/registry.yaml]
tech-stack:
  added: []
  patterns:
    - "module-private attachValueEntry/scaledToNorm/parsers per unit, unit-specific input class (rev-/comp-/eq-value-input)"
    - "Alt reset = capture-phase mousedown registered before the drag listener + stopImmediatePropagation, gesture-bracketed"
key-files:
  created: []
  modified:
    - modules/effects/reverb-unit/js/reverb-unit.js
    - modules/effects/compressor-unit/js/compressor-unit.js
    - modules/effects/analog-eq-unit/js/analog-eq-unit.js
    - modules/effects/*/module.yaml
    - modules/effects/analog-eq-unit/README.md
    - modules/effects/compressor-unit/README.md
    - modules/registry.yaml
    - plugins/O-Marimba/Source/ui/public/js/app.js
    - plugins/O-Formant/Source/ui/public/js/main.js
    - plugins/O-Texture/Source/ui/public/js/main.js
    - plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js
    - plugins/O-TextureForge/Source/ui/src/app.js
    - plugins/O-TextureForge/Source/ui/public/js/app.bundle.js
decisions:
  - "EQ unit: one isOuterRing(e) helper (the unit's existing rect.width/2 distance test) shared by drag, Alt reset and dblclick"
  - "EQ formatFn reads state.getScaledValue() (skew-correct) rather than re-deriving scaled from norm"
  - "Unit headers' Module System version lines bumped with the module versions"
metrics:
  duration: ~80 min
  completed: 2026-10-19
estimate:
  tokens: 130000
  tasks: 3
actuals:
  tokens: 14300      # chars/4 over the five commits' diffs (bundle excluded)
  tasks: 3
  commits: 28        # MEASURED rev-list ledger..HEAD; includes plans 01-04 commits on shared main. This plan's own commits: 5
plan_head_before: ecb9db2599969aff636b64c73d956c794af5abd9
plan_head_after: c9d70b26710be61436873b85b4d1f79ab414de47
---

# Phase quick-261019-ryp Plan 05: O-Marimba both gaps + modules/effects + module-JS instruments Summary

O-Marimba's Effects-tab EQ / compressor / reverb knobs now open self-contained inline value entry on double-click and reset on Alt/Option-click inside one host gesture. The three shared units are bumped to 1.4.0 / 1.4.0 / 1.1.0. The click-away commit fix ships in O-Marimba, O-Formant, O-Texture, O-MicrotonalSampler and O-TextureForge. Each plugin is built, installed, auval-listed and committed path-scoped.

`commits: 28` is the measured `rev-list` count on shared `main`. It includes the commits plans 01-04 made at the same time. This plan's own commits are the five listed below.

## Commits (this plan)

| Plugin | Old -> New | Commit | Paths |
|---|---|---|---|
| O-Marimba (+ modules) | 1.16.1 -> 1.16.2 | 34c0c90c | plugins/O-Marimba, modules/effects/{analog-eq,compressor,reverb}-unit, modules/registry.yaml (12 files) |
| O-Formant | 1.34.1 -> 1.34.2 | d10214d9 | plugins/O-Formant (3 files) |
| O-Texture | 0.6.1 -> 0.6.2 | be5e6d1e | plugins/O-Texture (3 files; `ONNXRUNTIME_VERSION 1.19.2` untouched) |
| O-MicrotonalSampler | 1.28.1 -> 1.28.2 | 218c123a | plugins/O-MicrotonalSampler (3 files) |
| O-TextureForge | 1.5.1 -> 1.5.2 | c9d70b26 | plugins/O-TextureForge (src/app.js + rebuilt app.bundle.js + CMake + CHANGELOG) |

No tags. PLUGINS.md is in none of these commits, and neither are the five excluded plugins.

## Gap 2: modules/effects (D-02, D-03)

| Unit | Version | Controls converted | Parse | Alt reset (event / default source) | Input class |
|---|---|---|---|---|---|
| reverb-unit | 1.0.0 -> 1.1.0 | Size, Damp, Pre-dly, Mod, Shimmer, Mix (knob + readout dblclick) | percent (`30` / `30%` -> 0.30) for all but Pre-dly; Pre-dly ms (`50`, `50 ms`; explicit `s` x1000) | capture `mousedown` on the knob, before the drag listener / `this.defaults[param]` (typeof-number guard) | `rev-value-input` |
| compressor-unit | 1.3.0 -> 1.4.0 | Thresh, Ratio, Attack, Release (knob + readout dblclick) | Thresh dB direct; Ratio `4` / `4:1`; Attack / Release ms (explicit `s` x1000) | capture `mousedown` on the knob / `this.defaults[param]` (typeof-number guard, replaces the `|| 0.5`) | `comp-value-input` |
| analog-eq-unit | 1.3.0 -> 1.4.0 | 4 bands x outer ring (freq) + inner dial (gain), by the drag's distance test (> 0.60 = freq) | freq: `k` x1000, a bare number is Hz; gain: dB direct | capture `mousedown` on `.dual-knob-container` (skips presses inside the field; never touches isDragging/currentState) / `this.DEFAULT_VALUES[<band>_freq or _gain]` | `eq-value-input` |

- Every unit defines its own `scaledToNorm` and `attachValueEntry`, with the click-away handler and its removal, plus its own parsers and CSS from `injectStyles()`. Nothing is exported or put on `window`, and no unit has an `import` line.
- `updateKnobVisual` (reverb, compressor) skips the readout while `dataset.editing === '1'`. In the EQ, `updateTooltip` writes the `.vt-freq` and `.vt-gain` spans separately and skips whichever one is being edited.
- EQ CSS: the field takes `pointer-events: auto`, and `.value-tooltip:has(.eq-value-input)` keeps the tooltip visible and opaque while a field is open.
- module.yaml: version and a new top changelog item (2026-10-19) for each unit. registry.yaml: only the three `version:` lines, because the file was clean at Step 0 and the `used_by` entries were left alone. READMEs: analog-eq-unit's Interaction section was rewritten and gets a `### 1.4.0` entry; compressor-unit gets a `### v1.4.0` entry. The reverb-unit README was left alone as planned, and its header comment documents the gestures instead.

### ryp-unit-probe, before (the 10 failures expected at baseline)
```
PROBE-FAIL fx_rev_size A: double-click opens a focused value-entry field
PROBE-FAIL fx_rev_size A: double-click writes nothing (log: start/end/start/end/set)
PROBE-FAIL fx_rev_predelay A: double-click opens a focused value-entry field
PROBE-FAIL fx_rev_predelay A: double-click writes nothing (log: start/end/start/end/set)
PROBE-FAIL fx_comp_threshold A: double-click opens a focused value-entry field
PROBE-FAIL fx_comp_threshold A: double-click writes nothing (log: set)
PROBE-FAIL fx_eq_lf_freq A: double-click opens a focused value-entry field
PROBE-FAIL fx_eq_lf_freq A: double-click writes nothing (log: start/end/start/end/set)
PROBE-FAIL fx_eq_lf_gain A: double-click opens a focused value-entry field
PROBE-FAIL fx_eq_lf_gain A: double-click writes nothing (log: start/end/start/end/set)
PROBE-OK   page errors: 0
ryp-unit-probe O-Marimba effects: FAIL (10 failure(s))
```
Midway check, as the tracer required: after the reverb change alone, both reverb cases passed A-F while compressor and EQ still failed (6 failures).

### ryp-unit-probe, after (`/tmp/ryp-unit-probe-final.log`, re-run after the commit)
56 PROBE-OK lines, 0 failures. Every case passes A through F: reverb Size (30 -> 0.3), reverb Pre-dly (50 -> 50), compressor Thresh (-12 -> -12), EQ LF freq (200 -> 200.00000000000003) and EQ LF gain (3 -> 3). Each commit and each Alt reset logs exactly `start/set/end`. Esc and an untouched Enter both log nothing. A host write while the field is open leaves the field in place, and page errors are 0. Ending line: `ryp-unit-probe O-Marimba effects: PASS (0 failure(s))`.

The EQ LF freq Alt reset lands on 99.956 Hz rather than exactly 100 Hz, because the unit's existing hard-coded `DEFAULT_VALUES` (0.5647) are rounded to 4 decimals. This is within the probe's tolerance and was not changed.

### Gap-2 gates
- gk9-gate on each unit file: 0 failures (`OK-ENTRY` on every knob/readout dblclick; the only `CHECK-NON-DIAL` is the field's own `dblclick` stopPropagation, the same as Marimba's).
- ryp-gate `=1` on each unit file: OK, 1 handler, removed in close().
- `--syntax` OK for all four files. `--boot O-Marimba`: pageErrors=0 before and after. `check-i18n --plugin O-Marimba`: ALL CHECKS PASS.
- Extra click-away probes beyond the plan (all PASS):
  - comp Thresh -> comp Ratio: `--pre '#tab-effects' --pre '.comp-bypass-toggle'`
  - EQ LF gain -> EQ LMF: `--pre .eq-bypass-toggle`
  - EQ LF freq (`--a-offset 16,0`) -> reverb Mix
- Installed binary check: `O-Marimba-dev.component` contains `rev-value-input`, `comp-value-input`, `eq-value-input` and `onOutsidePointer`, so the module JS was re-embedded.
- Screenshots of each open field (EQ freq, EQ gain, comp Thresh, reverb Pre-dly) show the field overlaid on the readout with no layout shift.
  - **Visual note:** the open EQ tooltip covers the W/M/T selector under the knob. The hover tooltip already does this, and the O-AnalogEQ reference behaves the same way.

## Gap 1: click-away fix (D-01)

| Plugin | Entry site fixed (handler line, after edit) | Probe args | Before | After | Gate / syntax / boot | auval |
|---|---|---|---|---|---|---|
| O-Marimba | app.js `attachValueEntry`, handler :571 | `--plugin O-Marimba --a '#mallet-knob' --b '#material-knob' --type 10`; `--pre '#tab-effects' --a '#fx_rev_size_knob' --b '#fx_rev_mix_knob' --type 10` | FAIL / FAIL (no field: dblclick reset) | PASS / PASS | OK / OK / 0 -> 0 | `aumu OuMa OuDv - O-Marimba-dev` |
| O-Formant | main.js `attachValueEntry`, handler :1195 | `--plugin O-Formant --a '[data-param="glottalRd"] .knob' --b '[data-param="breathiness"] .knob' --type 0.2` | FAIL (reproduced) | PASS | OK / OK / 0 -> 0 | `aumu OuFm OuDv - O-Formant-dev` |
| O-Texture | main.js `attachValueEntry`, handler :579 (also serves the Char sliders) | `--plugin O-Texture --a '#knob-brightness' --b '#knob-mix' --type 0.2` | FAIL (reproduced) | PASS | OK / OK / 0 -> 0 | `aumu OuTx OuDv - O-Texture-dev` |
| O-MicrotonalSampler | sampler-app.js `attachValueEntry`, handler :661 (knobs, tech and layer trims share it) | `--plugin O-MicrotonalSampler --a '.ouaricon-knob:has(#ctrl-attack)' --b '.ouaricon-knob:has(#ctrl-decay)' --type 0.2` | FAIL (reproduced) | PASS | OK (=1) / OK / 0 -> 0 | `aumu OMtS OuDv - O-MicrotonalSampler-dev` |
| O-TextureForge | src/app.js `attachValueEntry`, handler :222 (readout and on-dial paths) | `--plugin O-TextureForge --a '.knob[data-param="energy"]' --b '.knob[data-param="brightness"]' --type 20` | FAIL (reproduced) | PASS (probe serves the rebuilt bundle) | OK / OK (+ bundle OK) / 0 -> 0 | `aumu OuTF OuDv - O-TextureForge-dev` |

- Every build log `/tmp/ryp-build-<Name>.log` ends `RYP-EXIT=0` with the auval line above it. All builds went through the `/tmp/gk9-build.lock` lock, one plugin per acquisition, and none used `--reconfigure`.
- **Non-entry input left alone:** sampler-app.js :4125 (now about :4133) is the trigger-slot table, built in a loop: 8 rows of `type=number` fields with a focus guard. It has no commit-on-blur overlay and is not an entry field, so the gate arg stays `=1`.
- **TextureForge bundle parity before the edit:** `npm --prefix plugins/O-TextureForge/Source/ui run build` printed `[compared for emit]` and `git diff --quiet` held, so source and bundle agreed. After the edit and rebuild, the bundle has `document.addEventListener("pointerdown",u,!0)` and the matching remove, and the diff is a 1-line change. No `npm install` or `npm ci` was run.
- MicrotonalSampler `tests/*.js` were not run (optional in the plan).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Shared scratchpad script clobbered by a parallel executor**
- **Found during:** Task 3 (version bumps)
- **Issue:** Another executor overwrote my helper `bump.py` in the shared session scratchpad. The O-MicrotonalSampler and O-TextureForge CHANGELOG headings came out as `## [X.Y.Z] em 2026-10-19`.
- **Fix:** I corrected both headings to the em-dash form before committing. I also checked that the CMakeLists diffs changed only the VERSION line, and moved my helpers into a private `scratchpad/p05-ryp/` subdirectory. The committed files are correct.
- **Commit:** fixed before 218c123a / c9d70b26 (never committed wrong)

**2. [Scope note] Extra EQ / compressor click-away probes and screenshots**
These were added as evidence and changed no code.

The plan was otherwise executed as written. The only refactor in the EQ unit is pulling the distance test into `isOuterRing`; the drag behaviour is identical.

## Follow-ups
- **PLUGINS.md version rows** (not touched, per D-05): O-Marimba 1.16.2, O-Formant 1.34.2, O-Texture 0.6.2, O-MicrotonalSampler 1.28.2, O-TextureForge 1.5.2.
- **compressor-unit drag sends no sliderDragStarted/Ended.** This was there before and is untouched, so host automation-write does not record compressor drags. Value entry and Alt reset are gesture-bracketed.
- **analog-eq-unit `DEFAULT_VALUES` are 4-decimal constants.** The LF reset lands on 99.956 Hz. A `getParameterDefaults` native function would make them exact, as the WR-09 comment already suggests.
- **The five excluded plugins still need the click-away fix (D-05):** O-Bassoon, O-Freeze, O-Gain, O-Reed, O-simpleFM.

## Known Stubs
None.

## Threat Flags
None. The new surface (typed text -> parameter, a document capture listener while a field is open) is covered by T-ryp-13..16 and mitigated as specified.

## Self-Check: PASSED
- Commits 34c0c90c, d10214d9, be5e6d1e, 218c123a and c9d70b26 are all present in `git log`.
- All three Task verify blocks passed: TASK1-VERIFY-PASS, TASK2-VERIFY-PASS, and the Task 3 block apart from the SUMMARY-exists check, which this file satisfies.
