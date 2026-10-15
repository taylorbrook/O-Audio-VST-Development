# Stage 4: Polish - Verification

## Verification Date

2026-10-14

## Method

Single-cycle stage (Part 1 T1–T11 → CP1 → Part 2 T12–T15 → CP2 / CP3). I re-ran every automated SUMMARY claim myself. I started from a forced rebuild, touching `PluginProcessor.cpp` and `PluginEditor.cpp` (mtime only, no content change). After `build-and-install.sh` the installed VST3 and AU were `cmp`-identical to the build. I re-checked the PLAN §Success criteria code invariants (R-STATIC S1–S7, R-BRIDGE) at HEAD `38c9c13d`; `plugins/O-simpleAmbisonics/` had nothing uncommitted. Human gates (CP1, CP3: COMPAT-02 hosts, QUAL-02, FUNC-09) were signed off by Taylor during execute. They are taken as recorded in SUMMARY and REQUIREMENTS and were not re-run.

## Goal-Backward Analysis

### Original Goals (CONTEXT.md §Requirements Confirmed + RESEARCH §0 amendments)

1. **FUNC-08:** a factory bank of Init + the 8 lessons via the preset-manager module and a browser in the header. Lesson buttons and the bank read one recipe table. Users can save, replace and delete their own scenes.
2. **Lesson semantics:** a lesson = full reset + recipe. `outputLevel` and `inputMode` are user-owned and never move. orderStep walks 1 → 2 → 3 → 1. No recipe reaches order 3 below 1 m.
3. **QUAL-01 preset-load clause:** preset and lesson loads are click-free.
4. **Critic pass:** every finding triaged; W-tier + triaged N fixed with gates and negative controls; `CODE_REVIEW.md` at the canonical path.
5. **Carried fixes:** verify_2_1 V3 → Stereo, `encReadN` shortened, chevron / puck separated.
6. **COMPAT-02:** Logic + Ableton, mono + stereo tracks; auval; pluginval s10; Windows VST3 via CI.
7. **QUAL-02 / FUNC-09:** a headphone localisation sign-off and a creative-panner sanity check.
8. **Release:** 1.0.0, CHANGELOG, PLUGINS row, ARCHITECTURE amendments, **no tag**.
9. **No regression:** 0 warnings; digests hold; PERF-01 0; latency and parameter layout untouched.

### Deliverables (SUMMARY.md + code inspection)

1. `Source/PresetRecipes.h` (Init + 8; `kLessons` deleted). One apply core, `loadPresetByName`, user presets via a narrowed preset-manager (`savePreset` / `deletePreset` / `initializeFactoryPresets` only). 16 natives plus the `presetState` event. Header preset bar (◀ name ▶ Save Delete) in the retired `#chipLatency` slot.
2. `isUserOwnedParam` (outputLevel, inputMode), an orderStep walk with restart, and G-ORDER3-DIST.
3. G-PRESET-CLICK: 12 transitions; the 3 arms that are legitimately smoothed are checked against the same-motion reference.
4. critic-orchestrator (4 critics), `CODE_REVIEW.md` with `open_findings: none`. WR-01…07 and IN fixes, each with a harness gate + NC.
5. verify_2_1 22/22, header-fit H1 + `encreadn` NC, frame-probe A14 + `chevron` NC.
6. auval, pluginval s10, CI run 38675387588 (Windows pluginval 25/25), CP3 host hands-on.
7. CP3: QUAL-02 PASS, FUNC-09 usable.
8. Commit `6088574a`: CHANGELOG `## [1.0.0]`, CODE_REVIEW, REQUIREMENTS, ARCHITECTURE A1–A6, PLUGINS row.
9. T12 sweep ×2, all green.

### Goal Achievement

| Goal | Status | Evidence (re-run 2026-10-14) |
|------|--------|-------------------------------|
| 1. Factory bank + browser | ✅ Achieved | harness G-FACTORY / G-LESSON / G-PRESET-* lines PASS (289/289); verify_3_state S4 / S5 / S5b on installed VST3; R-BRIDGE `comm` empty, 16 natives, events {meterFrame, presetState}; S1 `kLessons` 0, table has no outputLevel / inputMode; CP1 signed off (Taylor) |
| 2. Lesson semantics + headroom | ✅ Achieved | G-LESSON[walk / restart / owned] + NC, G-ORDER3-DIST + NC in the harness; CP3 item 3 (Taylor) |
| 3. Click-free preset loads | ✅ Achieved | G-PRESET-CLICK + splice NC PASS in both harness runs |
| 4. Critic pass | ✅ Achieved | `CODE_REVIEW.md` `open_findings: none`; S4 WR-02/03/04/05, IN-01/04/19 harness lines PASS; `withBackend` + `withUserDataFolder` present |
| 5. Carried fixes | ✅ Achieved | verify_2_1 22/22; header-fit ALL PASS + `encreadn` NC FAILS H1 [en] as designed; frame-probe 14/14 + `chevron` NC FAILS A14 as designed |
| 6. COMPAT-02 | ✅ Achieved | auval SUCCEEDED; pluginval s10 VST3 + AU SUCCESS (0 FAIL); CI run 38675387588 (recorded); CP3 Logic + Ableton (Taylor) |
| 7. QUAL-02 / FUNC-09 | ✅ Achieved | CP3 sign-off recorded in REQUIREMENTS; DSP-03 ILD-sign gate still in the harness |
| 8. Release | ✅ Achieved | 1.0.0 on installed VST3 / AU / Standalone (`plutil`); one `## [1.0.0]`; PLUGINS row `📦 Installed \| 1.0.0` in HEAD; `git tag -l '*simpleAmbisonics*'` empty |
| 9. No regression | ✅ Achieved | 0 warnings; harness 289/289 ×2, diff = PERF-02 line only; digest-2.1 `9189d911a1fe98d5` + 2.2 `7f35bc9f91e321b9` PASS, 2.3 `edd1acfd21f54bcd` reproduced; PERF-01 0 + liveness 1; PERF-02 199–201×; `setLatencySamples` ×1; `getNumPrograms` returns 1 |

## Requirements Verification

**Stage:** stage-4
**Requirements for this stage:** 5 total (3 must, 1 should, 1 nice). QUAL-01's preset-load clause was also re-gated here.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-08: Concept-preset tour as factory presets | should | ✅ Complete | Init + 8 from one table, from both the lesson row and the header bar; harness gates + NCs, verify_3_state S4 / S5, CP1 |
| FUNC-09: Creative binaural panner | nice | ✅ Complete | Slow Orbit usable in a headphone mix: Taylor CP3 |
| COMPAT-02: Logic + Ableton, no I/O config | must | ✅ Complete | CP3 mono + stereo in both hosts; auval SUCCEEDED; pluginval s10 VST3 + AU; Windows CI 25/25 |
| QUAL-01: No artifacts (preset-load clause) | must | ✅ Complete | G-PRESET-CLICK + NC; S4 WR-02 NaN latch fixed + NC; S4 WR-04 Input Mode ramp 1.0026 + NC |
| QUAL-02: Localisation sanity | must | ✅ Complete | Taylor CP3: front / back / left / right / above distinct at order 3; +90° louder LEFT; DSP-03 ILD sign |

**Requirements Summary (whole plugin, 27):**
- ✅ Complete: 27
- ⚠️ Partial: 0
- ⏸️ Deferred: 0
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (VST3, AU, Standalone, render-test) | ✅ Pass | forced rebuild, 0 `warning:` |
| Harness ×2 | ✅ Pass | 289/289, 0 FAIL, logs identical except PERF-02 (199× / 201×) |
| Digests | ✅ Pass | 2.1 + 2.2 verdicts PASS, 2.3 `edd1acfd21f54bcd` reproduced |
| Real preset folder | ✅ Pass | `~/Library/O-simpleAmbisonics` listing identical before / after the harness |
| Install | ✅ Pass | VST3 + AU `cmp`-identical; 1.0.0 on VST3 / AU / Standalone; `presetState` in the installed VST3 |
| auval `aufx OSAm OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval s10 VST3 + AU | ✅ Pass | SUCCESS ×2, 0 FAIL (execute ran ×2 each; re-run ×1 each) |
| `verify_2_1` / `2_2` / `2_3` | ✅ Pass | 22 / 35 / 42, 0 FAIL |
| `verify_3_state.py` | ✅ Pass | S1–S5 + S5b ALL PASS |
| check-i18n / fr-lint / zh-lint | ✅ Pass | exit 0; zh 169 entries for O-simpleAmbisonics |
| check-ui-labels | ✅ Pass | exit 0, `seed=overrides` |
| boot-all-uis `--strict-tips` | ✅ Pass | 0 warn, 0 failed, 0 DEAD |
| frame-probe | ✅ Pass | 14/14; NCs fuma → A1, pole → A9, idle → A12, chevron → A14 each FAIL as designed |
| header-fit-probe | ✅ Pass | ALL PASS; NC `encreadn` FAILS H1 [en] as designed |
| lesson-row-budget / tooltip-clamp | ✅ Pass | ALL PASS / ALL PASS |
| `generate_sh_icons.mjs --check` | ✅ Pass | matches generator |
| R-STATIC S1–S7 | ✅ Pass | S1 0 / 0; S2 0 (savePreset 1); S4 one `^juce_add_binary_data`, `preset-manager.js` gitignored; S5 latency ×1, getNumPrograms 1, withBackend / withUserDataFolder / NEEDS_WEBVIEW2 / static WebView2; S6 0 `'mousedown'`, 0 `chipLatency`; S7 0 SafePointer copy-init, 0 `M_PI` |
| R-BRIDGE | ✅ Pass | `comm` empty; 16 natives; events {meterFrame, presetState} |
| Windows CI | ✅ Pass (recorded) | run 38675387588: build-windows ✓, pluginval s10 25/25; not re-dispatched |

## Human Verification (signed off by Taylor during execute)

- [x] CP1: preset-bar visual checkpoint; live Standalone lesson click (Output does not move); IN-07 DPR repaint
- [x] CP3-1: COMPAT-02 Logic + Ableton, mono + stereo, no I/O config; Save field typing
- [x] CP3-2: carried Logic list R1–R3 + general, 10× reopen, preset name across save / reopen
- [x] CP3-3: Standalone list (keyboard, lesson recipes only, orderStep walk, Overhead no clip, click-free loads)
- [x] CP3-4: D16 reflow en / fr / zh
- [x] CP3-5: QUAL-02 headphone localisation
- [x] CP3-6: FUNC-09 Slow Orbit usable
- [ ] CP3-7: R9 long host blocks → idle well. Not reported; informational only, not blocking.

## Issues Found

- None in the re-run.
- Note: my first UI-gate loop was a zsh word-splitting error in the verification script (exit 1, `Cannot find module '… --plugin …'`). The same gates re-ran under `bash` and all exited 0. This is not a plugin finding.
- Carried, by design: fr `reviewed: false` and zh `'mt'` (native review deferred); `CODE_REVIEW.md` items accepted or reviewed but not changed; v1.0.0 untagged (`/publish` is a separate step).

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes. All 4 stages complete; next is install / publish.

**Blockers:** none.
