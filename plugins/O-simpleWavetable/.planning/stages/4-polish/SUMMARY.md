# Stage 4 (Polish) — SUMMARY

**Plugin:** O-simpleWavetable · **Stage:** 4 of 4 (Polish) · **Executed:** 2026-10-06 – 2026-10-07 (Part 1 `d174623f`, Part 2 `ef46ffd3`, this docs commit) · **Version:** 1.0.0 (no tag, no publish)
**Plan:** `stages/4-polish/PLAN.md` (24 tasks, 2 parts; D-AA…D-AW; PLAN-signoff 1–5).

---

## Outcome

- **FUNC-08:** the preset manager runs inside the Field Guide page.
  - **Factory bank:** 9 factory presets from one §A9 table (`Source/PresetRecipes.h`); the 5 lessons are a subset of it.
  - **User presets:** save, replace, delete, and ◀ ▶ through one walk order. Factory names are refused, and names are sanitised.
  - **Display:** a truthful name, modified dot, lesson light and caption, driven by the `presetState` event.
  - **Limits:** presets never set `output_level`, never touch `IMPORTED_BANK`, and never call the module's XML state path or apply path. `getNumPrograms() == 1`.
- **Critic fixes:**
  - **Fixed:** every Stage 2 / Stage 3 warning (S2 W3, W4; S3 W1–W5) and the triaged notes (S2 notes 3 / 4 / 8; S3 N1, N4, N5, N7–N13). Each DSP fix has a gate with a negative control.
  - **Logged:** everything else is in `CODE_REVIEW.md`: 31 findings, `open_findings: none`.
- **QUAL-04:** three measured contrast gates plus Taylor's by-ear sign-off.
- **PERF-02:** measured on a Release build at four rates, well inside budget.
- **COMPAT-02:** a green Windows CI build plus pluginval (see the CI section below).
- **Release:** VERSION 1.0.0, CHANGELOG `## [1.0.0]`, CODE_REVIEW, ARCHITECTURE Amendments 19–23, and the PLUGINS row `📦 Installed | 1.0.0`.

## Files

**Created:**
- `Source/TestHooks.h`, `Source/PresetRecipes.h`
- `tests/perf-check/main.cpp`, `tests/ui-probes/wheel-gesture-probe.mjs`, `tests/ui-stub/generic-overrides.json`
- `CHANGELOG.md`, `CODE_REVIEW.md`, `.planning/stages/4-polish/SUMMARY.md`

**Modified:**
- `CMakeLists.txt`: TestHooks / PresetRecipes sources, `ouaricon_add_module(... preset-manager)`, binary-data entry, `perf-check` target, VERSION 1.0.0
- `Source/PluginProcessor.{h,cpp}`: `prepared` guard, byte-capped MIDI fill, `wheelNow` seed, `resetDisplayState`, apply core, preset identity / API, `currentPreset` state, `pollBankForImportStatus`, `uiHeld`, `stepKnobGesture`
- `Source/PluginEditor.{h,cpp}`: 20 natives, `presetState`, W1 snapshot-first, N1 FileChooser member, dtor W5 / N13
- `Source/WtVoice.h`: W4 `xfPhase` / `xfInc`
- `Source/WavetableImporter.{h,cpp}`: 16 MiB cap, `isStrippedNameChar`
- `Source/WavetableBank.h`, `MipmapBuilder.h`, `PositionLfo.h`, `PositionSmoother.h`: TestHooks include
- `Source/ui/public/index.html`, `js/i18n.js`
- `tests/{dsp,viz,state}-check/main.cpp`, `tests/i18n-states.json`
- `.planning/research/ARCHITECTURE.md`, `.planning/STATUS.md`
- `PLUGINS.md`: this row only

Part 2 changed no `Source/` file.

## Decisions as applied

| ID | Applied |
|----|---------|
| D-AA | The module is used for folders, `savePreset` / `deletePreset`, the factory sentinel and the JS only. S10 count: 0 forbidden calls. |
| D-AB | One apply core, `applyNormalisedTargets`: clears `pendingAutoSelect` (N5), skips `output_level`, applies defaults first, one gesture per changed param |
| D-AC | Imported × presets = bank choice only (G-PRESET-IMPORTED); Amendment 19 |
| D-AD | 9-entry table, fill rule, ids → names; Stepped Scan / Alias Demo / Drive Sweep / Vowel Pad / 8-bit PPG changes as listed |
| D-AE | Walk order: factory in table order, then user stems; `sanitisePresetName` |
| D-AF | `currentPreset` root property; absent → `""`; `presetLock` and `presetRevision` |
| D-AG | `presetState` push; tick order `bankUpdate` → `importStatus` → `presetState` → `cycleUpdate` |
| D-AH | 20 natives (two refusal stubs); `ensureFactoryBankOnDisk` is lazy |
| D-AI | Panel 352 px, keyboard 556 px (37 px white keys), popover, dynamic import of `preset-manager.js` |
| D-AJ | Stage 2 fixes from the prototypes with all three named deviations (`prepared` stays set after release; `exactlyEqual`; hooks behind `#if`) |
| D-AK | 16 MiB drop cap on both sides; "use the Import button" copy; error mapping keeps a specific error |
| D-AL | `pollBankForImportStatus`, with `lastStatusBank` seeded in the ctor and in `setStateInformation` |
| D-AM | The processor owns UI-held notes; the editor dtor releases them; the page sends `visibilitychange` → `allNotesOff` |
| D-AN | `uiReady` snapshots its counters first |
| D-AO | Burst-aware wheel accumulator on stepped knobs, the bank stack and continuous knobs (PLAN-signoff 2) |
| D-AP | N13 bracket in the processor; N8 refcount; N1 FileChooser member; N9, N11, N12 |
| D-AQ | Fix-or-log verdicts recorded in CODE_REVIEW.md |
| D-AR | G-Q4-STEP / ALIAS / BITS in dsp-check, recipes taken through `applyFactoryPreset` |
| D-AS | `tests/perf-check`, no hooks, Release, duty witness |
| D-AT | Null baselines: the Stage 3 snapshot (Part 1), then the Part 1 snapshot (Part 2) |
| D-AU | R-GOLD closed allowlist over 7 logs |
| D-AV | VERSION 1.0.0, CHANGELOG, CODE_REVIEW, PLUGINS row; no tag |
| D-AW | CI pushed and dispatched only after Taylor's explicit "yes" |

**PLAN-signoffs:**
1. **Drive Sweep:** keeps §A9; the copy was rewritten in en / fr / zh.
2. **W3b:** fixed and gated by G-S3W3-WHEEL's continuous arm.
3. **37 px keys:** confirmed at Task 14.
4. **CI:** Taylor said "yes" at Task 22.
5. **Mono legato octave jump:** in the Task 23 script; signed off.

## Measured values

### Part 1 (Tasks 1–13; full line in STATUS)

**Build and drivers** (Debug, out-of-repo):
- 0 plugin warnings, 0 module warnings.
- 7 drivers ALL PASS; state 12/12 (P11); `--alloc-check` 0, including a 6000-event flood.

**DSP fixes:**

| Gate | Result | Negative control |
|---|---|---|
| G-MONO-WHEEL | 0.00 / −0.00 / 0.00 c | +199.98 / +100.00 |
| G-LEGATO-XF | ≤ 7.1e-8 | ≥ 0.544 |
| G-LEGATO-ALIAS | −102.0 / −107.2 dB (= floor) | −24.3 / −26.0 |
| G-LEGATO-CLICK | 1.000 | 1.532 |
| G-MIDI-FLOOD | −0.024 c | alloc NC 1/1 |
| G-UNPREPARED | PASS | SIGSEGV |

**Other gates:** G-VIZ-RELEASE, G-SANITISE, G-DROP-CAPSYNC, G-FACTORY (skew NC), G-S3N5, G-PRESET-USER / NAME / WALK / STATE / IMPORTED, G-S3W4-ERRCLEAR, G-S3W5-UIHELD and G-S3N13 all PASS, and every NC fires.

**Static and UI:**
- R-UNDEF: clean (the NC lists 3 headers).
- R-GOLD: diffs on the allowlist only.
- check-i18n PASS; fr CLEAN; zh 168 / 0; labels 16 states; boot-all-uis clean.
- G-S3W3-WHEEL: 6/6, and the Stage 3 page NC fails 5 arms as designed.

**R-NULL vs the Stage 3 VST3:**
- The 5 untouched cases are bit-exact.
- `mono_legato` differs only at samples 28801–29039.
- `legato_60_96_drive` differs only in t+1…t+239.
- `w3_poly_to_mono` goes from +199.98 c to 0.000 c.

### Part 2 (Tasks 15–21)

**Task 15 baseline:** the 7 `p1-*` logs are green (state 12/12, alloc 0), and the Part 1 VST3 / AU snapshot is `p1.npz`.

**QUAL-04** (dsp-check; the values match RESEARCH):

| Gate | Result | Negative controls |
|---|---|---|
| G-Q4-STEP | S_Off 2.35e-2 / S_On 4.35e-4, **R 54.09** (≥ 10) | NC1 R(Off, Off) 1.000; NC2 depth 0 gives 0 steps; record-only W across the LFO reset R 3.22 |
| G-Q4-ALIAS | worst A_8k(Off) **−41.3 dB** (C7 @ 96k, ≥ −50); min contrast **69.0 dB** (≥ 40); worst A_full(On) **−107.3 dB** (≤ −90); max harmonic ≈ −12 dBFS | NC1 Stage 3 recipe −101.9 dB; NC2 On vs On 0.0 dB |
| G-Q4-BITS | SNR 95.1 … 16.5 dB, steps 5.70–6.27 dB, slope 6.03 dB/bit | NCa inf; NCb reversed; NCc offset (slope 0.044); record-only 8-bit PPG monotonic |

**PERF-02** (`perf-check`, Release, no hooks, duty 92 %; ALL PASS). Median CPU per block as a % of the real-time budget:

| Case | 44.1 kHz | 48 kHz | 88.2 kHz | 96 kHz |
|---|---|---|---|---|
| steady bs 64 | 0.333 | 0.363 | 0.655 | 0.719 |
| steady bs 512 | 0.314 | 0.343 | 0.620 | 0.674 |
| crossfade every block, bs 64 (record) | 3.17 | 3.45 | 6.32 | 6.89 |
| crossfade every block, bs 512 | 0.753 | 0.831 | 1.732 | 1.932 |

- Worst steady p99: 1.93 % (96k, bs 64).
- SCALE 10.53×; LIVE RMS ratio 4.44.
- Record-only: 96k bs 16 / 32 crossfade storms, median 23.9 / 12.5 %.

**Cross-checks** (pedalboard, installed VST3; DSP unchanged in Part 2):
- **PERF:** steady 0.35 / 0.36 / 0.66 / 0.72 %. Within 2× of perf-check and under 25 %.
- **ALIAS and BITS:** identical to dsp-check to within 0.1 dB (slope 6.029).
- **STEP:** R 53.9 on the RESEARCH window (±10 %).

**Task 20 sweep:**
- **Static:** R-STATIC S1–S14 clean.
- **Bridge:** R-BRIDGE 20/20 natives, 4 events, FACTORY_LABELERS = 9 table ids.
- **Build and drivers:** Debug 0 / 0 warnings; 7 drivers ALL PASS; state 12/12; alloc 0.
- **R-GOLD vs p1:** only G-TIME, G-FINITE and the soak block counts moved.
- **UI:** R-UNDEF clean; check-i18n / fr / zh (168 / 0) / labels / boot all green; G-S3W3-WHEEL 6/6, and its NC fails 5 arms as designed.

**Install:**
- VST3, AU and Standalone all report `CFBundleShortVersionString` 1.0.0.
- Installed bundles equal the artefacts, and the Standalone is fresh.
- 0 plugin `ForTesting` symbols.
- auval SUCCEEDED (2 benign skew warnings); pluginval VST3 + AU strictness 10 SUCCESS.

**R-NULL Part 2 vs Part 1:** 8/8 cases bit-identical, and Part 2 rendered twice is bit-identical.

## COMPAT-02 CI (Task 22)

Run [37670481560](https://github.com/taylorbrook/O-Audio-VST-Development/actions/runs/37670481560): `workflow_dispatch`, `plugin_name=O-simpleWavetable`, `version=1.0.0-validate`, `validate_only=true`, on `ef46ffd3`.

| Job | Result |
|---|---|
| parse-tag | ✓ |
| build-windows | ✓ 6m19s: MSVC, static WebView2, VST3; pluginval v1.0.3 strictness 10, editor tests on |
| build-macos | ✓ 10m18s: universal build, signing + notarization |
| create-release | skipped (validate-only); no tag, no Release |

**pluginval-windows-log:**
- All 25 test groups completed, including Editor, Editor Automation and Fuzz parameters. 0 FAILED, and no failure markers.
- The step ran `pluginval … | tee` under `pipefail`, and it passed, so pluginval exited 0.
- The final `SUCCESS` banner is not in the uploaded log, because pluginval writes it to stderr and `tee` captures stdout only. The verdict rests on the exit code. See deviation 7.

**COMPAT-02:** green Windows build + pluginval.

## Human checkpoints

- **Task 14 visual:** signed off by Taylor 2026-10-06 (resumed via `/plugin-execute`; no notes).
- **Task 22 CI:** "yes" (2026-10-07). Before the push, `origin/main..main` was re-listed: 26 commits, all O-simpleWavetable (incl. `18638e14 chore(claude)`). Push `987e8707..ef46ffd3`.
- **Task 23 listening:** "approved" (2026-10-07), with no tone or recipe notes.

## Deviations

1. **dispatch E stalled.** It hit the 600 s watchdog after finishing Task 16. `tests/perf-check/main.cpp` was complete. The orchestrator added the `perf-check` CMake target (no `OSIW_TEST_HOOKS`, with a comment saying why).
2. **perf-check sandbox starvation.** The first run was sandboxed: duty 3 %, wall 518 s, exit 77. The witness behaved as designed. The re-run outside the sandbox reached duty 92 % and exited 0.
3. **The `nm` ForTesting check.** It finds JUCE's own `juce::Path::defaultToleranceForTesting`. That is not a plugin hook; `nm -gU` on the plugin binary shows 0.
4. **G-REAP-SOAK.** Its liveness term failed once ("banks reaped 0"; deref 0, finite, max held 1) because the sweep ran at the same time as build-and-install. It passed 3/3 on a quiet machine (reaped 38–40), and R-GOLD was re-run on the quiet log. No import code changed in Part 2.
5. **Stale workflow comment.** The `create-release` comment in `build-and-release.yml` says build-macos is skipped on validate-only. The job's own comment, and the run, show that it runs. The comment is not ours to edit (outside commit scope); noted only.
6. **Part 1 copy deviation.** fr `import.err.tooLarge` was shortened, because the draft measured 259.8 px against a 240 px limit.
7. **No SUCCESS line in the CI log.** The plan's pass wording was "the pluginval log ends in SUCCESS". The artifact log has no `SUCCESS` line: pluginval prints it to stderr, outside the `tee`. Pass was judged on the step's pipefail exit 0, plus 25/25 test groups completed and 0 FAILED.
