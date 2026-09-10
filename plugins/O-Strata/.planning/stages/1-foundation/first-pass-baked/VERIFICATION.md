# Stage 1: Foundation - Verification

## Verification Date

2026-09-07

**Plugin:** O-Strata · **Stage:** 1 of 4 (Foundation + Shell, single phase) · **Mode:** manual
**Inputs:** `CONTEXT.md` (D1–D6, test criteria), `PLAN.md` (success criteria), `SUMMARY.md` (execute report, commit `0e838512`), `ROADMAP.md` Stage 1, `REQUIREMENTS.md`
**Method:** every SUMMARY claim was re-measured from scratch in this session (build, dump, validators, gates); nothing below is copied from SUMMARY without a fresh run. The three "manual smoke left for verify" items that do not need a window were turned into a headless harness (`smoke/`) linked against the param-dump objects.

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / ROADMAP.md Stage 1)

1. A buildable, validating O-Strata that is O-Prism v1.24.0 minus the wavetable library plus 48 inert geometry parameters, oscillators playing the sine placeholder (COMPAT-01).
2. Full `Prism` → `Strata` rename; O-Strata is an independent plugin (`OuSt`, 1.0.0); `plugins/O-Prism/` untouched (D1).
3. Parameter contract: 219 IDs in `parameter-spec.md` order, diff vs O-Prism = −2 +48, `allSliderIds()` = 188 incl. `delayDivision` (D4), relays generated, bake params not in the mod matrix.
4. Stripped UI passes every repo gate (check-i18n, fr/zh lint, check-ui-labels, boot-all-uis, tip render) (D3).
5. Inherited sections still work: notes sound on both oscillators, Scala loads, state round-trips 219 params + tuning + uiLanguage with an empty `geometryImports` child (FUNC-09/10 smoke).
6. Docs and counts: CHANGELOG, PLUGINS.md row, ROADMAP/ARCHITECTURE corrections (D5), checksums re-recorded; one path-scoped commit, no tag.

### Deliverables (from SUMMARY.md)

1. `plugins/O-Strata/` fork; VST3/AU/Standalone build; installed `-dev` bundles; pluginval ×2 + auval.
2. Three-case sed over 62 files, three renamed files, three tolerated provenance mentions.
3. `StrataParamIds.h` 35 suffixes/osc, 48 geometry pushes, `Table` removed, `params.tsv` dump.
4. `index.html` 4740 → 4299, `i18n.js` 2568 → 2483, fixtures 23 → 20 states, tip check 108 → 106, `openTab()` left-edge click fix.
5. Sine placeholder published to both `oscTablePtr` slots; `geometryImports` stub in get/setState.
6. CHANGELOG, NOTES, PLUGINS.md, ROADMAP/ARCHITECTURE edits, STATUS checksums, commit `0e838512`.

### Goal Achievement

| Goal | Status | Evidence (re-measured this session) |
|------|--------|----------|
| 1. Builds + validates | ✅ Achieved | `ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone` exit 0; pluginval strictness 10 **SUCCESS** on installed VST3 and AU; `auval -v aumu OuSt OuDv` **AU VALIDATION SUCCEEDED**; `auval -a` lists `aumu OuSt OuDv … O-Strata-dev` |
| 2. Rename + isolation | ✅ Achieved | `grep -rln Prism Source/` = exactly `PluginProcessor.h`, `ui/public/index.html`, `ui/public/js/i18n.js` (provenance notes); `git status --short plugins/O-Prism` and `git diff --stat HEAD -- plugins/O-Prism` empty; `PLUGIN_CODE OuSt`, `VERSION 1.0.0`, `juce_cryptography` linked |
| 3. Parameter contract | ✅ Achieved | Fresh `O-Strata-param-dump` run = 219 rows, byte-identical to `params.tsv`; ID diff vs O-Prism `params.tsv` = `−oscATable −oscBTable +48`; 24 `oscA*` geometry rows between `oscAWarpAmt` and `oscBPos` with spec defaults (Torus 0.166667, 256 frames 1.0, 0.6/0.3/0.8/0.2/1.0, `Imported…`); `allSliderIds()` = 35+35+6+4+6+5+5+1+7+6+5+3+3+4+12+48+3 = 188 (summed from the source); 31 `withNativeFunction`; `jassert (sliderAttachments.size() == sliderRelays.size())` present at `PluginEditor.cpp:724` |
| 4. UI gates | ✅ Achieved | `check-i18n --plugin O-Strata` ALL CHECKS PASS; `i18n-fr-lint --strict` CLEAN exit 0; `i18n-zh-lint` GATE PASSED exit 0; `check-ui-labels --plugin O-Strata` ALL CHECKS PASSED (no page error, every resource served); `boot-all-uis --plugin O-Strata --strict-tips` 0 DEAD / 0 late; `tests/ui_tip_render_check.js` ALL CHECKS PASSED (2799) |
| 5. Inherited sections (smoke) | ✅ Achieved (headless) | `smoke/strata-smoke`: **ALL SMOKE CHECKS PASSED — 0 failure(s)** (27 checks; see below). Standalone/WKWebView visual items remain human (non-blocking) |
| 6. Docs + commit | ✅ Achieved | `CHANGELOG.md` `## v1.0.0 (unreleased)`; PLUGINS.md `\| O-Strata \| 🚧 Stage 1 \| 1.0.0 \|`, duplicate-row check empty; ROADMAP/ARCHITECTURE read 219 / 48 / 188 / `StrataParamIds`; all four `contract_checksums` in STATUS.md match `shasum -a 256`; `git tag \| grep -i strata` empty; commit `0e838512` touches only `plugins/O-Strata/**` and `PLUGINS.md` |

## Headless smoke harness (`smoke/main.cpp`, `smoke/build.sh`, `smoke/smoke-output.log`)

Links `scripts/param-dump/main.cpp`'s sibling objects (`JUCE_WEB_BROWSER=0`, no editor TU) with a new `main()`, so it exercises the real `OStrataAudioProcessor` through `createPluginFilter()`.

| # | Check | Result |
|---|-------|--------|
| 1 | Held C4 at `oscMix=0` (Osc A only), 48 kHz, RMS over the second 0.43 s | **0.1800** — sounds |
| 1 | Same with `oscMix=1`, `oscBLevel=0.8` (Osc B only) | **0.1800** — sounds; identical level = same sine placeholder on both slots |
| 1 | Negative controls: `oscALevel=0` / `oscBLevel=0` on the respective A-only / B-only case | 0.0000 both — the sounding oscillator is the one under test |
| 1 | NaN/Inf in output | none |
| 2 | `TuningEngine::loadScalaFile (test-tunings/just-major.scl)` | returns true; 62/60 = 1.250000 (5/4), 64/60 = 1.500000 (3/2), 67/60 = 2.000000 (2/1) — one key per scale degree, 12-EDO 1.259921 before the load |
| 3 | 214 params randomised (5 tuning params held at default), `uiLanguage=1`, Scala loaded → `getStateInformation` → fresh processor `setStateInformation` | **0 mismatches** across all 219 (190 of the randomised values differ from default, so the pass is not vacuous); `uiLanguage` restored as `fr`; notes 48..84 frequency-identical in both instances and still the Scala scale |
| 3 | State XML | root `OStrataParameters`, 219 `PARAM` nodes, `<tuningEngine>` child, `uiLanguage="fr"` as a string, **`<geometryImports/>` present and empty** (0 children, 0 attributes) |

Harness corrections made while writing it (none are plugin faults): `oscBLevel` defaults to 0.000 in O-Prism and O-Strata alike, so the B-only case sets it; the tuning engine maps consecutive keys to consecutive scale degrees; `AudioParameterBool::getValue()` returns the raw float until it snaps through `convertFrom0to1`; the five tuning params are applied to the engine by the audio thread, which the harness never runs.

## Requirements Verification

**Stage:** stage-1
**Requirements for this stage:** 1 total (1 must, 0 should, 0 nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| COMPAT-01: pluginval VST3 + AU at strictness 10, and auval | must | ✅ Complete | pluginval strictness 10 SUCCESS on `O-Strata-dev.vst3` and `O-Strata-dev.component` (fresh runs); `auval -v aumu OuSt OuDv` SUCCEEDED |
| FUNC-09 / FUNC-10 (smoke only, formal at stage-2) | must | ⏸️ Deferred | Smoke evidence here: sine plays on A and B through the voice path; Scala loads and round-trips. Pitch-vs-O-Prism and 31-EDO generator checks remain for Phase 2.1 |

**Requirements Summary:**
- ✅ Complete: 1 (COMPAT-01)
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 26
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (`ninja O-Strata_VST3 O-Strata_AU O-Strata_Standalone`) | ✅ Pass | exit 0 (incremental; SUMMARY's full build listed 28 inherited warnings, none in Stage-1-edited files) |
| Parameter check (param-dump) | ✅ Pass | 219 rows == `params.tsv`; −2 +48 vs O-Prism |
| Removal-set grep (15 tokens over `Source/`, `CMakeLists.txt`, `tests/`) | ✅ Pass | empty |
| `Prism` grep | ✅ Pass | 3 tolerated files in `Source/`; 3 more provenance comments in `tests/` and `CMakeLists.txt` (outside the criterion's `Source/` scope, all comments) |
| pluginval VST3 / AU strictness 10 | ✅ Pass | SUCCESS / SUCCESS |
| auval | ✅ Pass | SUCCEEDED; listed in `auval -a` |
| check-i18n / fr lint / zh lint | ✅ Pass | ALL CHECKS PASS / CLEAN / GATE PASSED |
| check-ui-labels (20 states) | ✅ Pass | ALL CHECKS PASSED |
| boot-all-uis --strict-tips | ✅ Pass | 0 DEAD, 0 late, no page errors |
| ui_tip_render_check.js | ✅ Pass | 2799 passed |
| Headless smoke (notes / Scala / state) | ✅ Pass | 27/27 |
| O-Prism isolation | ✅ Pass | working tree and HEAD diff both empty |
| Checksums / docs / no tag / commit scope | ✅ Pass | see Goal 6 |

## Human Verification

Non-blocking; the headless runs cover the substance. Left for the next time the Standalone or Logic is open:

- [ ] `O-Strata-dev.app`: both oscillator canvases paint the sine (`posState` glue → `getActiveOscFrame`; the generic stub returns null by design, so only the real WKWebView shows it)
- [ ] `O-Strata-dev.app` Inspect (Safari Web Inspector): no console errors, no resource-provider 404s in the **WKWebView** path (Chromium boot shows 0 errors / all resources served)
- [ ] Logic: play a held note on A and on B by ear; load a `.scl` from the tuning tab

## Issues Found

- None in the plugin. Four harness assumptions were corrected (listed above). The pre-existing O-Prism behaviour that the settings popover covers part of the rightmost tab (SUMMARY's `openTab()` fix) is inherited and disappears when Phase 3.1 restores five tabs — carried, not fixed here.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes — Stage 2 (DSP), Phase 2.1 `GeometryBakeScheduler` + mesh slicer (`/plugin-discuss O-Strata 2-dsp`)

**Blockers:** none

**Carry-forward for Stage 2 (from SUMMARY "Notes for later stages"):** reaper is live and idle (`retireTable`, `timerCallback`, `blockGeneration`); publish with `oscTablePtr[osc].store (…, release)` + `retireTable (old)`; `getActiveOscInfo` hard-codes `"Sine"`; every factory preset is re-authored in 4.1; `stereoWidth` still unbound; the `smoke/` harness is a ready seed for the Stage 2 render harness (same object-link trick, add a `tests/render-harness/CMakeLists.txt` when 2.1 needs it as a build target).
