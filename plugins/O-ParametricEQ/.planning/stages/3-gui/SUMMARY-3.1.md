---
title: "O-ParametricEQ Stage 3 (GUI) — Phase 3.1 Summary (layout, bindings, scale check)"
date: 2026-11-15
phase: "3.1"
plan: PLAN-3.1.md
version: 0.7.0
status: executed
---

# O-ParametricEQ — Phase 3.1 Summary

The finalized v1 page is now the plugin's editor. It is bound to all 196 parameters (C1, no fallback), and the processor owns the preset manager (C2). Version 0.7.0 is built, installed and validated, and the audio output is unchanged from 0.6.0.

## Results at a glance

| Check | Result |
|---|---|
| Render harness, default / `--long` (`--lid-dir` = 0.6.0 dump) | **166 PASS / 0 FAIL** both (155 + 8 owed + 3 preset) |
| Designer harness | 150 PASS / 0 FAIL / 18 INFO (unchanged) |
| Audio unchanged | XR-lid: the 16 T1 sessions rendered by 0.7.0 are memcmp-equal to a 0.6.0 `--dump-sessions` taken at T1 (superset of the plan's 4) |
| `git diff HEAD -- Source/dsp Source/analysis` | empty |
| 11-mutant control (real tree, full harness) | 11 / 11 caught, each by its own row (table below) |
| UI gates | `check-i18n` 0, `i18n-fr-lint` 0, `i18n-zh-lint` 0, `check-ui-labels` 0, `boot-all-uis --strict-tips` 0 with 0 DEAD |
| Native functions | 19 / 19, grep diff empty (page + `preset-manager.js` vs `PluginEditor.cpp`) |
| `OPEQ_EDITOR_BENCH` | 0 hits in the plugin's VST3 / AU / Standalone compile commands; referenced only by `PluginEditor.h/.cpp` |
| Install | `build-and-install.sh` rc 0, `O-ParametricEQ-dev.vst3` / `.component`, 0.7.0, no alternate-variant orphan |
| pluginval strictness 10 | VST3 ×2 SUCCESS, AU ×1 SUCCESS (editor opened; one stock `Current program is -1` AU warning) |
| auval `aufx OuPQ OuDv` | AU VALIDATION SUCCEEDED |
| Standalone | page painted (plate, EB Garamond axis, nodes, strip); screenshot in the session scratchpad |
| Plate (P4) | bbox relative to `#display` identical to the test page (0.00 px) at 960 × 600, 800 × 500, 1600 × 1000; idle task 0.05 ms/s shown vs 0.04 hidden |
| Editor bench (T11) | all 13 rows PASS — `execute-3.1/editor-bench/RESULTS.md` |

## Editor bench (real editor, real page) vs research bench

| Row | 3.1 | Research |
|---|---|---|
| E-open cold, ctor → painted | **235.4 / 254.3 ms** (gate 500) | 256.7 / 274.1 |
| E-open warm | 137.7 / 150.3 | 123.5 / 147.6 |
| E-open Debug cold | 264.7 | — |
| E-props | freq start 10 / end 30000 / skew 0.203214481472969; 8 types; boot 0 warnings / 0 unknown; 7 boot natives incl. `pageReady` | same props, 6 natives |
| E-idle 10 s | 0 script calls, 0 messages (see note 1) | 0 / 0 |
| E-preset a / b / c / d | **1 frame** each; 14.5 / 12.2 / 13.1 / 12.9 ms median to painted | 1 frame (d not run) |
| E-bind-in | 196 / 196, max normalised error 5.96e-8 | — |
| E-bind-out | 196 / 196, one gesture pair per parameter | — |
| E-type | 1000 → 0.5 exactly; 42.7 → 0.2500297 (note 2) | — |
| E-disabled | 64 / 64 (Release and Debug) | — |
| E-ui / E-listen | 14 / 14; listen 5 → 4, 0 / 25 / −3 → −1, editor close → −1 | — |
| E-cycle | Release 5 × 10, Debug 1 × 10: no crash, 0 assertions, no leak | — |
| E-res | every URL 200 with the right MIME; canvas "1k" pixel-identical to EB Garamond | — |
| E-size | 960 × 600; 700 × 400 → 800 × 500; 2000 × 2000 → 1600 × 1000; reopen restores | — |

Notes:
1. Release process 1's idle window recorded a band created on b01 followed by ~60 freq / gain writes: the signature of a real click-drag on the bench window, which stays on top. The bench sends no pointer events. A re-run process with real input logged (`idle-recheck.json`) showed 0 / 0, and the only input was the bench's own typed-entry keystrokes.
2. The plan's "42.7 Hz → 0.25 ± 1e-6" is unreachable: normalised 0.25 is 42.6809 Hz. The page writes the exact mapping (error 8.5e-9), and "42.68090" lands on 0.25. The host text reads "999.9998779" rather than "1000 Hz", because the parameters have no text function. That is a processor change, carried to Stage 4.
3. E-bind-out tolerance was checked in Hz / dB / Q (worst 0.11 × bound). Two small-Q targets are 9.6e-6 and 1.8e-6 off in normalised units, which is float storage × 1/skew.

## Mutant control (`execute-3.1/mutants/`)

| Mutant | Fails |
|---|---|
| LS10 | XR-ls10 |
| LS11 | XR-ls11 |
| LS12 | XR-ls11, XR-px22 (expected) |
| LS15 | XR-ls15 |
| PX26 | XR-ls15 |
| PX27 | XR-ls10 |
| PX38 | XR-px38 |
| PX39 | XR-px39 |
| DM2 | XR-dm2 |
| AN1 | XR-an1 |
| PX22 | XR-px22 |

Each was run on a scratch copy of the real 0.7.0 tree with the full harness (165 / 1 or 164 / 2). The research run used an owed-rows-only main. `run.py` is kept with its scratch paths parameterised.

## Files

New: `Source/PluginEditor.h/.cpp`, `Source/PresetSaveGuard.h` (namespace `opeq`), `Source/ui/public/index.html`, `js/i18n.js`, `js/juce/index.js`, `js/juce/check_native_interop.js`, `tests/ui-stub/gen-overrides.py`, `tests/ui-stub/generic-overrides.json`, `tests/i18n-states.json`, `.planning/params.tsv`, `execute-3.1/` (editor bench, plate check, mutants).
Edited: `Source/PluginProcessor.h/.cpp` (P5 + one stale doc-comment line), `CMakeLists.txt` (0.7.0, UI binary data, editor TU), `tests/render-harness/main.cpp` (owed26 patch as written + XR-pm / XR-pmd / XR-psg + fake-home re-exec), `CHANGELOG.md`.
`Source/ui/public/modules/*.js` stay untracked.

## Deviations

**Page (beyond P1)**
- `.settings-toggle { line-height: 14px }` was added, because `check-ui-labels` [7] failed: in zh-Hans, 开 / 关 falls back to a taller line box and the hover-help row grew 4 px. Pinning the line height fixed it, and all five gates were re-run.
- The plate fills use a third class, `.plate-solid { fill: var(--frame) }`, for the inked fills (pterostigma, abdomen marks, eyes). The test page fills these with a hex literal.
- `buildPlate()` is called from `init()`, not as a top-level IIFE.
- The header comment no longer lists `#plate` as a placeholder.
- `pageReady` is one line at the end of `init()`. It runs after two animation frames and sends `performance.now()`.

**Editor**
- When the SafePointer is gone, async natives return without calling `complete`. P2 said to call it, but in JUCE 8.0.15 the completion holds a raw pointer into the destroyed WebView (`juce_WebBrowserComponent.cpp:317-321`), so calling it is a use-after-free.
- `withKeepPageLoadedWhenBrowserIsHidden()` is kept, matching the template, the research bench and the suite rule (memory: completions are dropped while the page is hidden). P2's "Debug: not used" line was ambiguous. **Taylor's call** if it should go.
- `benchReport` is registered by `juce::Identifier`, so the literal-name native grep counts 19.
- The `savePreset` native repeats the module's file-name rule (`/\:` → `_`) to build the path it hands to the guard.

**CMake**
- The binary-data target is `OuariconParametricEQ_UIResources`, not `_UI`, because `ParamDump.cmake` only keeps a UI target out of the test consoles when its name ends in `UIResources`.

**Harness**
- P6 says the harness already has a fake-home re-exec, but it had none. It was added: Apple builds re-exec once with a temp `CFFIXED_USER_HOME`, verify the presets directory before the first write, and delete the temp home at exit. Negative control: a planted bad fake home stops the run with exit 3 before any write. Non-Apple builds have no re-exec and stop the preset rows with exit 3.
- Each preset row's control is folded into that row's verdict, which keeps the count at 166.
- XR-pm compares the held `output_gain` against the value read back after setting +7 dB (6.99999905).
- XR-pmd checks defaults within 1e-6, because the reset goes through the host path.
- The XR-psg control covers the name only. A save into `Factory/` is refused even when the file is absent, by design.
- The owed26 patch adds three warnings (unused captures `b1` / `b2`, unused `owedSame`). They were left as written.

**Fixtures**
- `params.tsv` has no skew column. `gen-overrides.py` takes ranges, skews and choices from `tests/golden/params.tsv` (the harness's `--dump-params`, live NormalisableRange) and cross-checks against `params.tsv` and `parameter-spec.md`; any mismatch exits 1. Two runs are byte-identical.
- `i18n-states.json` has no dropdown state, because v1 has no preset dropdown. `check-ui-labels` has no state-count pin.
- The native-diff grep must accept single quotes: the plan's double-quoted pattern finds 0 hits in `index.html`.

**Plate check**
- The research `plate-bench` measures cost only. `plate-check/plate-check.js` compares bounding boxes relative to `#display` and forces the test page's `#display` to the real size, because the test page wraps the plugin frame in mockup chrome.

## Carry-forward

**Verify 3.1 (hands-on, Taylor)**
- Opens at 960 × 600 in Logic (AU) and one VST3 host.
- Logic timing: create `~/Library/O-ParametricEQ/ui-timing.flag`, do 5 cold opens, read `~/Library/O-ParametricEQ/Logs/ui-timing.log`, ≤ 500 ms.
- Automating a band's freq / type moves the chip, node and strip.
- Save a user preset, change things, load it back; output gain and bypass are untouched by the load.
- Note Logic's undo behaviour across a preset load (R4: 196 gestures).
- Language switch en / fr / zh-Hans at 800 × 500.
- Plate look against the mockup.
- The § 9 items that need no curve.

**3.2**
- `DisplayModel` owned by the editor.
- `eqSections` from the timer (gated by `pageIsReady`), with a full push on `requestEqSections` and after a preset load.
- Re-run the Debug 0-assertion row with something emitted (R3: the 3.1 row is partly vacuous).

**Stage 4**
- Parameter text functions ("1000 Hz", not "999.9998779").
- Session persistence of the preset name (P9).
- Whether the `ui-timing.flag` hook stays.
- WebView2 timing on Windows CI.
- Factory presets.
- Preset rows on non-Apple harness builds.
