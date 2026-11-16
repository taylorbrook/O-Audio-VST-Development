# Stage 3: GUI — Phase 3.1 (layout, bindings, scale check) - Verification

## Verification Date

2026-11-15 (verify session). Everything below was run against `9f517c00` (0.7.0). `git status` showed no changes under `plugins/O-ParametricEQ` before the run.

What the verify session did:

- **Independent harness re-run.** The harness objects in `build/` were deleted and recompiled: `tests/*/main.cpp` and the harness copies of the `Source/` TUs (`verify-3.1/run-harness.sh`). The session then ran the designer harness, the render harness default, and the render harness `--long`.
- **Independent no-audio-change check.** Instead of reusing the execute session's 0.6.0 dump, the session took the 16 XR-lid sessions from the 0.7.0 harness (`--dump-sessions`). Their SHA-256 hashes were compared with the 0.5.0 hashes recorded at verify 2.6 (`2-dsp/verify-2.6/sessions-050.sha`). XR-lid then ran against that dump.
- **Code read, in full:**
  - `PluginEditor.h/.cpp`;
  - the processor diff `1f656db2..9f517c00`: the include, the preset-manager member and setup, and `createEditor`;
  - `PresetSaveGuard.h`, diffed against O-AnalogEQ's;
  - the ctor, `isFactoryPreset`, the name sanitiser and the apply loop of `OuariconPresetManager.h`.

  Diffs against the sources:
  - `index.html` against `v1-ui.html`;
  - `i18n.js` against `v1-i18n.js`;
  - `js/juce/index.js` against JUCE 8.0.15.
- **Independent gates:**
  - the native-function diff, with the session's own grep (single and double quotes, both files);
  - all five UI gates;
  - the static greps (DSP / analysis diff since `52257b89`, `OPEQ_EDITOR_BENCH`, tracked module JS, plate hex literals);
  - installed bundles `diff -rq` against the artefacts.
- **Validators** on the installed bundles: pluginval strictness 10 (VST3, AU) and `auval -v aufx OuPQ OuDv`.
- **Editor mutation run** (`verify-3.1/mutants.py`, `verdict.py`, `mutants-results.txt`):
  - 23 one-line mutants: 17 in the editor, 4 in the page, 2 in the processor's preset setup.
  - Each ran through a scratch copy of the T11 editor bench (`bench-CMakeLists.txt`: source dir from `PEQ_SRC`, no LTO), 2 cycles per run.
  - Verdicts come from the session's own `verdict.py` over the bench JSON, not from `analyze.py`. The unmodified control passes every row.

Nothing in the repo source changed. Scratch builds and the mutable source copy are in the session scratchpad. The scripts keep their scratchpad paths.

Scope: Phase 3.1. Stage 3 stays open (3.2, 3.3).

## Goal-Backward Analysis

### Original Goals (ROADMAP Phase 3.1 as amended 2026-11-15; PLAN-3.1)

1. Editor opens at the mockup size in VST3 and AU.
2. All 196 relays bound (C1, no fallback). Open → painted ≤ 500 ms. Idle relay traffic 0.
3. Host automation moves the controls. A preset load refreshes every control.
4. Controls for inert parameters are disabled per type / slope.
5. The processor owns `OuariconPresetManager` (C2): `output_gain` / `bypass` excluded, validated apply, `PresetSaveGuard`, empty factory set.
6. The eight owed probe rows plus DM2 / AN1 / PX22 are render-harness rows (U8).
7. The plate ships with token colours (U4), EB Garamond, and the five UI gates pass.
8. No DSP change. 0.7.0 builds, installs and validates.

### Deliverables (from SUMMARY-3.1.md, confirmed in the tree)

1. **Editor:** `PluginEditor.h/.cpp`.
   - 4 global relays plus 24 × `BandRelays` (8 each), every one registered with `withOptionsFrom`.
   - Attachments are declared after the WebView, so destruction order is correct.
   - 19 natives.
   - The timer starts in `pageReady`, and `timerCallback` also checks `pageIsReady`.
   - The destructor stops the timer and sets `listenBand` to −1.
   - Async preset dialogs hold a `SafePointer` and skip `complete` once the editor is gone (the UAF reasoning in the summary is correct).
2. **Processor:** only the `JUCE_WEB_BROWSER` include, the `presetManager` member (declared after `parameters`), the two setup calls, and `createEditor`.
   - The module ctor does no file I/O.
   - The session codec is untouched.
3. **Page:** `index.html` = `v1-ui.html` plus 4 hunks, all recorded in the summary:
   - the header comment;
   - the zh-Hans `line-height`;
   - the plate (SVG plus `buildPlate()`, `rotate(-21 0 60)` inside the SVG, box geometry as the test page);
   - `pageReady` after two rAFs.

   `i18n.js` is verbatim, and the JUCE frontend is byte-identical to 8.0.15.
4. **`PresetSaveGuard.h`:** O-AnalogEQ's with only the namespace and plugin name changed.
5. **Harness:** owed26 rows and XR-pm / XR-pmd / XR-psg. **Fixtures:** `generic-overrides.json`, `i18n-states.json`, `params.tsv`.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Opens at mockup size | ✅ Achieved (automated) | pluginval 10 ran the Editor and "Open editor whilst processing" tests on VST3 and AU: `SUCCESS`. Bench E-size: 960 × 600 on a fresh processor, clamps at 800 × 500 and 1600 × 1000, reopen restores. Mutants E10 (clamp removed) and E11 (`resized` stops storing) are caught by E-size. **Logic / VST3 host by hand: Taylor (below).** |
| 2 196 relays, ≤ 500 ms, idle 0 | ✅ Achieved (WKWebView). Logic figure owed | Native diff 19 / 19 identical (this session's grep). Execute bench: cold 235.4 / 254.3 ms, idle 0 / 0. This session's non-LTO bench: cold 300.5 ms, warm 114.7, E-props 196, 0 console warnings. Mutants E1 (one band's Q relay unregistered), E2 / E3 / E4 (attachment on the wrong relay) and E14 (`pageReady` unregistered) are caught by E-props / E-bind / E-preset. P4 (page never calls `pageReady`) is caught by E-open. **Logic timing from the flag log: Taylor.** |
| 3 Automation + presets refresh | ✅ Achieved | E-bind-in 196 / 196 and E-bind-out 196 / 196 with one gesture pair each. E-preset: 5 load paths, every one painted in 1 frame, 0 host mismatches. R1 (preset exclusion removed) is caught by E-preset. |
| 4 Inert controls disabled | ✅ Achieved | E-disabled 64 / 64. Mutants P1 (Notch keeps gain), P2 (Tilt loses slope) and P3 (Q inert at 12 dB/oct instead of 6) are each caught. `TYPE_HAS_GAIN` / `TYPE_HAS_SLOPE` match the type list in CONTEXT. |
| 5 Preset manager (C2) | ✅ Achieved (one ungated line, Issue 2) | XR-pm 194 / 194 bit-equal, `output_gain` 6.99999905 dB and `bypass` held. XR-pmd 194 / 194 at default. XR-psg: factory name and `Factory/` file refused, user name allowed. Code read: both save natives call `presetSaveRefusal`; the dialog path checks the file after the `.json` fixup inside the guard. |
| 6 Owed rows | ✅ Achieved | XR-ls10, ls11, ls15, px38, px39, px22, dm2 and an1 all PASS on the fresh rebuild. The 11-mutant control is from execute (11 / 11, not re-run here). |
| 7 Plate, fonts, gates | ✅ Achieved (look by hand) | `check-i18n` 0, `i18n-fr-lint` 0, `i18n-zh-lint` 0 (51 `mt` entries counted, as expected before Stage 4), `check-ui-labels` 0, `boot-all-uis --strict-tips` 0 with 0 DEAD and 0 late. The plate code has no hex literals (the `#FAF0E6` / `#5C4033` hits are the token definitions and the canvas node drawing). E-res: 11 / 11 URLs 200 with the right MIME, EB Garamond loaded. Mutants E12 (woff2 MIME) and E13 (css not served) are caught. **Look against the mockup: Taylor.** |
| 8 No DSP change; 0.7.0 validates | ✅ Achieved | `git diff 52257b89 9f517c00 -- Source/dsp Source/analysis` is empty. The 16 XR-lid sessions from 0.7.0 are **SHA-256-equal to the 0.5.0 hashes from verify 2.6** (16 / 16), so they are equal to 0.6.0 as well. Designer 150 / 0 / 18. Render 166 / 0 default, `--long` 166 / 0 (S5 600 s per rate, 16 / 16). Installed `-dev` bundles `diff -rq` identical to the artefacts, 0.7.0. pluginval 10 VST3 / AU `SUCCESS` (one stock AU `Current program is -1` warning). `auval` `AU VALIDATION SUCCEEDED`. |

## Requirements Verification

**Stage:** stage-3. **Requirements for this stage:** FUNC-08, UI-01, UI-02, UI-03, UI-04, UI-06, UI-07. Phase 3.1 closes none of them. Their criteria belong to 3.2 (UI-01, UI-02, UI-04 typed entry at the range ends) and 3.3 (FUNC-08 latch and drag, UI-03, UI-06, UI-07).

| Requirement | Priority | Status | Note |
|-------------|----------|--------|------|
| UI-01 Interactive display | must | ⏸️ Pending (3.2) | Node create / drag / scroll is 3.2. |
| UI-02 True curve | must | ⏸️ Pending (3.2) | G9 (b) is 3.2. |
| UI-03 Analyzer | should | ⏸️ Pending (3.3) | No `SpectrumAnalyzer` in 3.1, by design. |
| UI-04 Strip, typed entry, double-click reset | should | ⏸️ Pending (3.2) | Strip bound; typed entry proven in the Frequency field (E-type). Range-end entry is 3.2. Double-click reset is not gated (Issue 1). |
| UI-06 Meter | nice | ⏸️ Pending (3.3) | `#meter-mount` is empty. |
| UI-07 Fills | nice | ⏸️ Pending (3.3) | |
| FUNC-08 Listen | should | ⏸️ Pending (3.3) | Transient half: `setListenBand` mapping and editor-close reset proven (E-listen; mutants E5 / E6 / E7 caught). |

**Requirements Summary:** Complete 0, Partial 0, Pending (later phase in this stage) 7, Failed 0. REQUIREMENTS.md: statuses unchanged, one note added.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Harness rebuild | ✅ | `build rc 0` after the harness objects were deleted. |
| Designer harness | ✅ 150 / 0 / 18 | Unchanged since 2.6. |
| Render harness default | ✅ 166 / 0 | XR-lid run against this session's own dump. |
| Render harness `--long` | ✅ 166 / 0 | S5 at 600 s per rate, 16 / 16. |
| Audio vs 0.5.0 | ✅ 16 / 16 SHA-256 equal | `verify-3.1/sessions-070.sha` against `2-dsp/verify-2.6/sessions-050.sha`. |
| Native functions | ✅ 19 / 19 | Page and `preset-manager.js` against `PluginEditor.cpp`. Every call in the page is a literal `getNativeFunction('…')`, except the module, which receives `juce.getNativeFunction` and is included in the grep. |
| UI gates ×5 | ✅ all exit 0 | 0 DEAD, 0 late. |
| Bench-only define | ✅ | 0 hits in the plugin's VST3 / AU / Standalone ninja commands. Only `PluginEditor.h/.cpp` reference it. |
| Module JS untracked | ✅ | `git ls-files Source/ui/public/modules` is empty. |
| Install | ✅ | `diff -rq` identical, 0.7.0, `-dev` only, no orphan. |
| pluginval 10 | ✅ VST3 / AU `SUCCESS` | The editor tests ran. |
| auval | ✅ | `AU VALIDATION SUCCEEDED`. |
| Editor mutation run | ✅ 19 / 23 caught | The 4 survivors are below. |

### Mutation run

| Mutant | Result |
|---|---|
| E1 one band's Q relay not registered | caught: E-open, E-props, E-bind-in / out, E-preset, timeouts |
| E2 gain attachment on the Q relay | caught (same rows) |
| E3 slope / placement relays swapped | caught (same rows + E-disabled) |
| E4 `output_gain` attachment on the `gain_scale` relay | caught (same rows) |
| E5 no listen reset in the destructor | caught: E-listen |
| E6 `setListenBand` off by one | caught: E-listen |
| E7 `setListenBand` upper bound 23 | caught: E-listen |
| E8 `analyzerMode` accepts 4 | caught: E-ui |
| E9 `selectedBand` accepts −1 | caught: E-ui |
| E10 `resizeEditor` unclamped | caught: E-size |
| E11 `resized()` stops storing the size | caught: E-size |
| E12 woff2 served with the wrong MIME | caught: E-res |
| E13 css not served | caught: E-res |
| E14 `pageReady` not registered | caught: E-props (console warning) |
| **E15** `getParameterDefaults` returns normalised values | **survived**: no row reads the defaults (Issue 1) |
| **E16** `savePreset (name)` skips the guard | **survived**: no row drives the native with a factory name (Issue 2) |
| **E17** timer started in the constructor | **survived**, equivalent in 3.1: `timerCallback` also returns while `! pageIsReady` and emits nothing (PLAN R3) |
| P1 Notch keeps gain | caught: E-disabled |
| P2 Tilt Shelf has no slope | caught: E-disabled |
| P3 Q inert at 12 dB/oct instead of 6 | caught: E-disabled |
| P4 page never calls `pageReady` | caught: E-open, timeouts |
| R1 preset exclusion removed | caught: E-preset |
| **R2** `setValidatedApply (true)` removed | **survived**: the bench and XR-pm / XR-pmd use well-formed presets and do not count gestures per applied value (Issue 2) |

## Human Verification

For Taylor, in the DAW. These items come from SUMMARY-3.1's carry-forward. The display shows only the grid and nodes in 3.1: the curve, node lines, analyzer, meter and fills are 3.2 / 3.3.

- [x] Opens at 960 × 600 in Logic (AU) and in one VST3 host.
- [x] Logic timing: create `~/Library/O-ParametricEQ/ui-timing.flag`, do 5 cold opens, read `~/Library/O-ParametricEQ/Logs/ui-timing.log`. Gate ≤ 500 ms (`ctor_to_pageReady_ms`).
- [x] Automating a band's freq / type moves its chip, node and strip.
- [x] Save a user preset, change things, load it back. Output gain and bypass are untouched by the load.
- [x] Note Logic's undo history after a preset load (R4: up to 196 gestures).
- [x] Language switch en / fr / zh-Hans at 800 × 500.
- [x] Plate look against the mockup.
- [x] Taylor's open call from execute: keep `withKeepPageLoadedWhenBrowserIsHidden()` (recommended: keep; it matches the suite rule).

**Result, 2026-11-16 (at verify 3.2, on 0.8.0):** Taylor reports all items pass. `withKeepPageLoadedWhenBrowserIsHidden` stays as built.

## Issues Found

1. **Double-click-reset defaults are not gated (minor; 3.2).** Mutant E15 makes `getParameterDefaults` return normalised values instead of engineering units, and no row notices. The code is correct on reading: `convertFrom0to1 (getDefaultValue())` over the 74 slider IDs. Recommendation: a 3.2 bench row in which a double-click on each strip field (and later each node) lands the host parameter on its default.
2. **Two preset-manager properties are held by the code only (minor; Stage 4).**
   - Mutant E16: the `savePreset (name)` native skips `presetSaveRefusal`. XR-psg tests the guard function, not the native.
   - Mutant R2: `setValidatedApply (true)` removed. No row loads a malformed or out-of-range preset value, or counts one gesture per applied value.
   - Both lines are present and correct. The factory set is empty until Stage 4, so neither can bite now.
   - Recommendation: add both rows with the factory presets in Stage 4:
     - a bench call of `savePreset ("<factory name>")` → false, no file written;
     - an XR row with a string / 1.7 / NaN value → clamped or skipped, with gestures counted.
3. **E17 is vacuous in 3.1** (known, PLAN R3). It is already carried to 3.2: re-run the Debug 0-assertion row with `eqSections` live.
4. **Host text has no units** ("999.9998779", not "1000 Hz"). Already carried to Stage 4 (parameter text functions).

No blocker. Nothing failed.

## Stage Verdict

**Status:** ✅ VERIFIED (Phase 3.1). The automated scope is fully met. The hands-on items above are owed by Taylor and do not block 3.2.

**Ready for next phase:** Yes. Phase 3.2 (interactive response display).

**Blockers:** none.
