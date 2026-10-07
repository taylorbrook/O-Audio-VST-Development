# Stage 4: Polish - Verification

## Verification Date

2026-10-07

## Method

Goal-backward against CONTEXT.md and the PLAN.md success criteria S1–S14. The automated claims in SUMMARY.md were **re-run independently** in this verify session, at HEAD `ea7d6fad`, with the plugin tree clean. `git diff ef46ffd3 HEAD -- Source CMakeLists.txt` is empty, so the CI run's commit is the same code as HEAD.

- **Offline gates:** a fresh out-of-repo Debug tree (R-DBG) with 0 plugin warnings. All 6 drivers and `dsp-check --alloc-check` were re-run.
- **PERF-02:** a fresh out-of-repo **Release** tree, `perf-check` built without hooks and re-run on a quiet machine.
- **Host checks** on the installed binaries:
  - targeted `auval -v aumu OSiW OuDv`
  - pluginval VST3 and AU at strictness 10, with the editor tests on
- **Static and bridge:** R-STATIC S1–S14 and R-BRIDGE, re-run.
- **UI gates:** all five, plus the wheel probe.
- **CI:** the run and its Windows log were pulled with `gh` and read directly. This did not rely on the SUMMARY's account.
- **New independent check: §A9 cross-check.** Every entry in `Source/PresetRecipes.h` was decoded against the choice orders in `parameter-spec.md` and compared with ARCHITECTURE §A9:
  - bank indices 0–4 = Sine→Saw, Sine→Square, Pulse Width, Formant, Drive
  - `lfo_shape` 1 = Triangle, 2 = Saw, 4 = S&H
  - `bit_depth` 9 = 8 bit, 13 = 4 bit

## Goal-Backward Analysis

### Original Goals (CONTEXT.md)

1. **FUNC-08:** the full preset-manager module and browser panel.
   - The factory bank is Init plus the 8 §A9 recipes.
   - Presets never set `output_level`, and `getNumPrograms() == 1`.
   - The lesson buttons are reconciled to §A9 through one table.
2. **Critic fixes:** every warning (S2 W3/W4, S3 W1–W5) and the triaged notes. Each DSP fix has a gate with a negative control. The by-design and logged items go in CODE_REVIEW.md.
3. **PERF-02:** 16 voices at four rates, on a Release build.
4. **COMPAT-02:** a green Windows build plus pluginval, from a validate-only CI run.
5. **QUAL-04:** measured contrast gates plus a by-ear sign-off.
6. **Release:** v1.0.0, CHANGELOG, CODE_REVIEW and the PLUGINS row. No tag.
7. **Regression guard:** goldens move only where a fix intends it. The null test passes, alloc is 0, and the UI gates are green.

### Deliverables (SUMMARY.md + code)

1. `Source/PresetRecipes.h`: 9 recipes in one table, with the lessons as a subset. The editor has 20 natives and a `presetState` event, and the browser panel is at 352 px.
2. Each fix has its gate: `wheelNow` seed, `xfPhase` / `xfInc`, a byte-capped MIDI fill, `prepared`, `TestHooks.h`, the 16 MiB caps, `pollBankForImportStatus`, `uiHeld`, the burst wheel accumulator, `stepKnobGesture`, `resetDisplayState` and `isStrippedNameChar`. CODE_REVIEW.md has 31 findings and `open_findings: none`.
3. `tests/perf-check` (Release, no hooks).
4. CI run 37670481560.
5. The G-Q4-STEP, G-Q4-ALIAS and G-Q4-BITS gates, and the Task 23 listening sign-off.
6. VERSION 1.0.0, CHANGELOG `## [1.0.0]`, CODE_REVIEW.md and the PLUGINS row.
7. R-GOLD and R-NULL, recorded in SUMMARY.

### Goal Achievement

| Goal | Status | Evidence (this session) |
|------|--------|--------------------------|
| FUNC-08 presets | ✅ Achieved | G-FACTORY [table / apply / init / load-by-name / defs] PASS: 9 recipes, 50 entries, raw round trip worst 4.66e-7, output level untouched by all 9. G-FACTORY-NC (skew) fires. The **§A9 cross-check** is 9/9 exact. S3 lesson ids ⊂ table (empty diff). `FACTORY_LABELERS` = the 9 table ids. |
| FUNC-08 constraints | ✅ Achieved | S10 forbidden module calls = 0, and `presetManager.savePreset (` = 1. S11 `"output_level"` in the table = 0. G-PRESET-IMPORTED PASS, and its NC (a module-style XML restore empties the snapshot) fires. `getNumPrograms() { return 1; }` = 1. State 12/12, including P11 `currentPreset` round trip. |
| Critic fixes | ✅ Achieved | Every gate from the Critic-item coverage table PASS on the re-run, with 27 `FAILS as designed` NC lines and 0 `vacuous`. Examples: G-MONO-WHEEL 0.00 c (NC 199.98 c); G-LEGATO-ALIAS −102.0 dB = floor (NC −24.3 dB). The wheel probe is 6/6. |
| PERF-02 | ✅ Achieved | Release re-run: worst steady p99 1.006 %, worst crossfade 2.140 % (96k) of the budget; all within ±10 % of SUMMARY |
| COMPAT-02 | ✅ Achieved | `gh run view 37670481560`: conclusion success, headSha `ef46ffd3`, build-windows ✓, build-macos ✓, create-release skipped. The Windows log has 25 `Completed tests in pluginval` groups and 0 `FAILED`. CMake has `NEEDS_WEBVIEW2 TRUE` and `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`; `withUserDataFolder` = 1; S13 MSVC SafePointer trap = 0. |
| QUAL-04 | ✅ Achieved | G-Q4-STEP R ≥ 10 PASS (NC1 R 1.000, NC2 0 steps). G-Q4-ALIAS PASS (NC1 −101.9 dB, NC2 0.0 dB). G-Q4-BITS PASS, SNR(16) 95.1 dB (NCa / b / c fire). The by-ear sign-off is recorded: Task 23 "approved", 2026-10-07. |
| Release | ✅ Achieved | The installed VST3, AU and Standalone report `CFBundleShortVersionString` 1.0.0. The CHANGELOG has one `## [1.0.0]` heading, and the awk extract is 134 lines (non-empty). CODE_REVIEW.md is at the canonical path. The PLUGINS row reads `📦 Installed \| 1.0.0` in both HEAD and the working tree. `git tag -l '*simpleWavetable*'` is empty. |
| Regression | ✅ Achieved | 0 plugin warnings. All 6 drivers ALL PASS, with 0 `FAIL` lines and 0 JUCE assertions. `--alloc-check` 0 allocations (including the 6000-event flood). Unmoved references: G-PITCH 0.0009 c, G-Q2-C8 −114.3, G-Q2-SWEEP −74.9, G-Q2-PULSE −67.0 dB, G-VEL −11.905 dB, G-STEAL 1.076, G-SWITCH-TAIL 0.656, G-RETRIG-VEL 0.589. R-NULL is taken from SUMMARY: Part 2 vs Part 1 8/8 bit-identical, and source unchanged since then. |

## Success Criteria (PLAN S1–S14)

| # | Status | Evidence |
|---|--------|----------|
| S1 Factory presets, one table, lessons ⊂ | ✅ | G-FACTORY, §A9 cross-check 9/9, S3 empty, `FACTORY_LABELERS` 9 |
| S2 User presets save / replace / delete / ◀ ▶ | ✅ | G-PRESET-USER / NAME / WALK PASS (state + viz logs); R-BRIDGE 20/20; Task 14 visual (Taylor) |
| S3 No `output_level`, no `IMPORTED_BANK`, 1 program, no module XML path | ✅ | S10 = 0, S11 = 0, G-PRESET-IMPORTED (+ NC), P11 |
| S4 Truthful name / dot / lesson light | ✅ | G-PRESET-STATE PASS; `presetState` in the event set {bankUpdate, cycleUpdate, importStatus, presetState} |
| S5 PERF-02 | ✅ | Release re-run (below); `nm -gU` `ForTesting` = 0 on the installed VST3 |
| S6 COMPAT-02 | ✅ | CI run as above |
| S7 QUAL-04 measured | ✅ | G-Q4-* PASS, all NCs fire |
| S8 QUAL-04 by ear | ✅ | Task 23 sign-off (SUMMARY / STATUS) |
| S9 Regression | ✅ | Drivers, alloc 0, auval, pluginval VST3 + AU s10 (re-run) |
| S10 UI gates | ✅ | check-i18n ALL PASS; fr CLEAN; zh O-simpleWavetable 168 entries, 0 findings; check-ui-labels ALL PASSED (every resource served); boot-all-uis 1120×780, 0 DEAD, 0 warn; G-S3W3-WHEEL 6/6 |
| S11 Release documented, untagged | ✅ | as above |
| S12 Scoped, consented commits | ✅ | No tag. The push / dispatch "yes" is recorded at Task 22. `origin/main..main` = 1 docs commit (`ea7d6fad`, unpushed). |
| S13 Contracts consistent | ✅ | ARCHITECTURE Amendment 21 (§A9 as shipped) matches the table |
| S14 Taylor signed off | ✅ | Task 14, Task 22, Task 23 |

## Requirements Verification

**Stage:** stage-4
**Requirements for this stage:** 4 total (0 must, 3 should, 1 nice)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-08: Factory presets, each isolating one concept | should | ✅ Complete | 9 presets = §A9 + 4-bit variant; G-FACTORY; by-ear sign-off |
| PERF-02: 16 voices at 44.1–96 kHz well within budget | should | ✅ Complete | perf-check Release re-run |
| COMPAT-02: Windows VST3 with WebView2 static | should | ✅ Complete | CI run 37670481560 green; pluginval 25/25 groups, 0 FAILED |
| QUAL-04: Each teaching contrast clearly audible | nice | ✅ Complete | G-Q4-STEP / ALIAS / BITS + Task 23 |

**Requirements Summary:**
- ✅ Complete: 4 (all 30 requirements are now complete)
- ⚠️ Partial: 0
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Debug harness build | ✅ Pass | 0 plugin warnings |
| bank / dsp / mod / import / viz-check | ✅ Pass | ALL PASS, exit 0 each |
| state-check | ✅ Pass | 12 PASS, 0 FAIL (incl. P11) |
| `dsp-check --alloc-check` | ✅ Pass | 0 audio-thread allocations, incl. 6000-event MIDI flood |
| Negative controls | ✅ Pass | 27 `FAILS as designed`, 0 `vacuous` |
| PERF-02 `perf-check` (Release, fresh tree) | ✅ Pass | ALL PASS, exit 0, duty 95.2 %. STEADY worst median 0.800 % / p99 1.006 % of the real-time budget (16 voices, 4 rates × bs 64 / 512; ≤ 25 %). XFADE worst 2.140 % (96k bs 512). SCALE 10.81×, LIVE 4.44. The only `ForTesting` symbol is JUCE's `juce::Path::defaultToleranceForTesting` (SUMMARY deviation 3). |
| auval (targeted) | ✅ Pass | `AU VALIDATION SUCCEEDED`; only the 2 known benign skew-default warnings |
| pluginval VST3 s10 | ✅ Pass | exit 0, 25 groups, SUCCESS, 0 FAILED |
| pluginval AU s10 | ✅ Pass | exit 0, 25 groups, SUCCESS, 0 FAILED |
| Install freshness | ✅ Pass | Installed VST3 / AU `diff -rq` = artefacts; `presetState` in the VST3 + Standalone binaries; Standalone newer than the VST3 artefact; 0 `ForTesting` |
| R-STATIC S1–S14 | ✅ Pass | all expected counts |
| R-BRIDGE | ✅ Pass | `comm -3` empty; 20 natives; 4 events; 9 labelers |
| UI gates (5) + wheel probe | ✅ Pass | see S10 |
| CI Windows (COMPAT-02) | ✅ Pass | run 37670481560, read directly |

## Human Verification

- [x] Task 14: preset-panel visual, 37 px keys, wheel on a real device, language pass (Taylor, 2026-10-06)
- [x] Task 22: CI push and dispatch go-ahead (Taylor, 2026-10-07)
- [x] Task 23: factory-preset listening, plus the C4→C7 Mono legato on Drive (Taylor, 2026-10-07, "approved")
- [ ] **Ship-bar (not a Stage 4 gate):** a native fr / zh-Hans review. The 168 zh entries are at `mt` (PLAN out-of-scope list, as in Stage 3).
- [ ] **Not done in this stage (by decision):** a hands-on Windows DAW test. COMPAT-02 is satisfied by CI.

## Issues Found

- **None blocking.**
- The SUMMARY deviations were reviewed and none of them changes a verdict:
  - **CI log has no `SUCCESS` line (deviation 7):** confirmed in this session. The Windows log has 25/25 groups and 0 FAILED, and the job conclusion is success.
  - **Stale workflow comment (deviation 5):** outside the plugin's scope; noted only.
- Unpushed: `ea7d6fad`, the Stage 4 docs commit. It contains no code. It will go out with the next push or `/publish`.

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes. The plugin is complete, and all 30 requirements are complete.

**Blockers:** none. Publishing (tag `O-simpleWavetable-v1.0.0`) is a separate `/publish` decision.
