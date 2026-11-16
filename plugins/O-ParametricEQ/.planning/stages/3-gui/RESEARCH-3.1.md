# O-ParametricEQ Stage 3 (GUI), Phase 3.1: Research (layout, bindings, scale check)

**Researched:** 2026-11-15
**Scope:** ROADMAP Phase 3.1 and Stage 3 CONTEXT U4, U6, U8. It covers:
- the 196-relay scale check (U6, open question 1);
- the editor template against the processor that Stage 2 actually built;
- the dragonfly plate's cost (U4, open question 4);
- the gate fixtures Stage 3 must add;
- the render-harness rows owed by VERIFICATION-2.6 (U8).

Out of scope, and left to RESEARCH-3.2 / 3.3:
- G9 (b) and the JS evaluator (open question 5);
- Logic gesture recording for the two-parameter drag (open question 6);
- vu-meter fit (open question 2);
- the listen state machine (open question 3);
- the 25th-band tip (U5; band creation is 3.2).

**Method:** three scratch benches, kept in `research-3.1/`. Nothing in `Source/`, the harnesses, the goldens or the contracts was changed.

| Bench | Path | What it is |
|---|---|---|
| Relay bench | `research-3.1/relay-bench/` | A JUCE GUI app that runs the real `OuariconParametricEQAudioProcessor` and an editor with all 196 relays and attachments. It loads the unmodified `v1-ui.html` in WKWebView, with an injected timing script. The preset-manager and UI-state native functions are stubbed. |
| Plate bench | `research-3.1/plate-bench/` | Headless Chromium (Playwright) on `v1-ui-test.html`, 1600 x 1000 at 2x, run with the plate shown and with it hidden. |
| Probe port | `research-3.1/probe-port/` | A scratch copy of the plugin tree with the eight new rows added to the render harness, plus a mutation re-run that reuses `verify-2.6/mutation_run.py`'s mutants. |

- **[BENCH]:** a number measured on this machine (Apple M4 Max, macOS 26.6.2, Apple clang 21, JUCE 8.0.15, Release with LTO unless stated).
- **[ARGUED]:** a statement from reading JUCE 8.0.15, the templates or `Source/` at `b68aa1cb`.

**Contracts read:** CONTEXT (Stage 3), ROADMAP § Stage 3, `mockups/v1-integration-checklist.md`, `v1-PluginEditor-TEMPLATE.h / .cpp`, `v1-CMakeLists-SNIPPET.txt`, `v1-ui.html`, `v1-ui-test.html`, `parameter-spec.md`, VERIFICATION-2.6.

## Summary

1. **All 196 relays pass U6's gates with a wide margin. Keep them, and drop the fallback.**
   - The worst cold open, from the editor constructor to a painted page, is **274 ms** (median 257 ms) against the 500 ms gate [BENCH].
   - A 196-value preset load reaches the page in **≤ 3.9 ms** and paints on the next frame (**1 frame**, by all three load paths) [BENCH].
   - Relays for one band only save about **13–18 ms** per open [BENCH], and the unmodified page cannot run that way: it logs 184 "unknown to the backend" warnings, because `bindBandStates` builds all 192 band states.
   - 10 open / close cycles: no crash, no leak report, in Release and in Debug [BENCH].
   - Measured in WKWebView only. WebView2 is a Stage 4 CI item (call C1).
2. **The template does not compile against the Stage 2 processor as written.** It assumes six processor members that do not exist, or exist under other names: `displayModel`, `editorOpen`, `outputLevelDB`, `presetManager`, `takeAnalyzerFrame`, and the analyzer as a processor member. The mapping is in Q2. The UI-state atomics, the `UI` state child and the language codec are already in place [ARGUED].
3. **The page's native-function calls match the template's registrations exactly:** 18 names called (page and preset-manager.js), 18 registered, no difference [BENCH, grep]. Every one must stay registered in 3.1, including the ten preset functions. That forces a decision on the preset manager now rather than in Stage 4 (call C2).
4. **The template's 30 Hz timer fires before the page has loaded.** It hits a Debug assertion (`juce_WebBrowserComponent.cpp:170`, "window.__JUCE__.backend undefined") 21 times per 10 open / close cycles. It is harmless in Release [BENCH]. Gate the timer's sends on a page-ready native call.
5. **The dragonfly plate costs nothing measurable.** It is a static DOM `<svg>` of 13 elements (3390 path commands, 39.5 KB of path data) behind the canvases. The canvas loop never redraws it. Shown against hidden, main-thread time at idle reads 199 / 202 ms/s against 202 / 195 ms/s, and 20 resizes read 57 / 61 ms against 71 / 78 ms: within noise [BENCH]. Two edits are needed to port it: the geometry and the colours (Q4).
6. **Gate fixtures:** the generic stub seeds slider ranges from `.planning/params.tsv`, which always gives skew 1, and gives every combo four one-character choices.
   - `tests/ui-stub/generic-overrides.json` must therefore carry the real skews for the 48 freq / Q sliders and the real choice lists for the 72 combos.
   - Generate the file from the param dump; do not hand-write it (Q5).
7. **All 11 owed mutants are killed by 8 new render-harness rows.** Each row passes on the unmodified source and fails under its mutant [BENCH].
   - Full run: **163 PASS / 0 FAIL** (155 + 8). The new rows add 0.5–0.7 s.
   - The patch applies cleanly (`git apply --check`) against HEAD.
   - The "nine probe-only survivors" in CONTEXT U8 and ROADMAP 3.1 are **eight**. The commit message for VERIFICATION-2.6 swapped them with the nine equivalent survivors.

Two calls are Taylor's (C1, C2). The rest are the plan's.

---

## Q1. The 196-relay scale check (U6)

### How JUCE moves state to the page [ARGUED]

- **Registration.** Each relay adds one event listener and one name to the page's start-up data (`juce_WebControlRelays.cpp:74-80`, `:155-161`, `:230-236`). The names are grouped into one assignment per kind (`juce_WebBrowserComponent.cpp:175-189`). On macOS every injected script runs as one document-start script (`juce_WebBrowserComponent_mac.mm:865-873`).
- **Quadratic script size.** `juce_WebBrowserComponent.cpp:397-404` appends a full copy of the event-ID list once per listener. With 198 listeners that is about 1.06 MB of injected script; with the fallback's 14 it is about 12 KB. This is part of the ~4 ms of extra constructor time and the ~9 ms later script start (warm). It is harmless at this size, and it is JUCE's behaviour, not ours.
- **Initial state is pulled by the page.**
  - The attachments' constructor sends are dropped: the WebView is not yet visible (`juce_WebBrowserComponent.cpp:607-611`).
  - Importing `index.js` creates a state object for every registered name. Each one sends `requestInitialUpdate` (`index.js:164`, `:275-276`).
  - C++ answers each with two script calls, properties then value (`juce_ParameterAttachments.cpp:295-316`, `:352-360`, `:401-415`).
  - At boot that is 196 messages in and 393 script calls out [BENCH].
- **Every change is one `evaluateJavaScript` call. Nothing is batched.**
  - Path: attachment (synchronous on the message thread, async from any other thread; `juce_ParameterAttachments.cpp:100-113`) → relay → `emitEventIfBrowserIsVisible` → JSON → one script call (`juce_WebBrowserComponent.cpp:414-429`).
  - A 196-change preset is 196 calls carrying about 10 KB in total. The page coalesces its redraws to one per animation frame (`v1-ui.html:1329-1337`).

### Results [BENCH]

Median / worst. Open: 5 processes per mode; "cold" is the first open in a fresh process, "warm" is opens 2–10 (n = 45). Preset rows: 50 runs per method. The fallback is 12 relays (the 4 globals + band 1's 8).

| Metric | 196 relays | Fallback (12) |
|---|---|---|
| Editor constructor, cold / warm | 63.8 / 71.9; 33.7 / 57.5 ms | 59.0 / 59.1; 29.1 / 43.2 ms |
| All states have properties, cold / warm | 205 / 221; 95 / 116 ms | 186 / 193; 82 / 94 ms |
| **Painted page (second frame), cold** | **257 / 274 ms** | 239 / 254 ms |
| Preset, 196 sets on the message thread → last value in the page | 1.8 / 3.5 ms | 0.4 / 0.9 ms |
| Preset, `setStateInformation` → last value in the page | 1.8 / 2.5 ms | 0.6 / 1.2 ms |
| Preset, set from another thread → last value in the page | 2.0 / 3.9 ms | 0.5 / 1.4 ms |
| **Preset load → painted frame** | 13–15 / 24 ms = **1 frame** | 13.5 / 18 ms = 1 frame |
| One automated parameter → painted | 12.8 / 15.8 ms | 13.2 / 16.9 ms |
| Idle traffic, relays only | 0 messages | 0 |
| Close (editor destroyed) | 0.3 / 2.3 ms | 0.3 / 0.6 ms |
| 10 open / close cycles (Release ×5, Debug) | no crash, no leak report | same |

- **Properties arrive correctly:** `b24_freq` start 10, end 30000, skew 0.203214481472969, interval 0; `b24_type` has 8 choices. The `b01_` equivalents match. Full mode boots with 0 console warnings and exactly the 6 expected boot native calls.
- **Prior art:** O-Polystutter already ships about 156 relays.
- **Caveats:**
  - WKWebView only.
  - The host was a bench app, not a DAW; a loaded DAW opens slower.
  - The response curve came from the real `DisplayModel` through bench glue. A curve after a preset took 1–3 frames, bounded by the 30 Hz poll, not by the relays.

### P1 (for the plan)

- Bind all 196 relays as the template does. Remove the fallback branch from the plan and from the checklist § 0. Keep § 0's measurements as 3.1 acceptance rows, re-run in the Standalone and in Logic on the real editor.
- Gate the timer: the page calls a new native `pageReady()` once `bindBandStates` has run. The timer emits nothing before that call, and the flag is reset in the destructor. This removes the Debug assertion (Summary 4). Add it to the native-function grep list.

---

## Q2. The template against the Stage 2 processor

The processor already has `uiLanguage`, `analyzerMode`, `listenBand`, `uiDisplayRange`, `uiSelectedBand`, `uiEditorWidth`, `uiEditorHeight`, the `UI` child in get / setState, the language codec, `readAndResetOutputPeak (ch)`, `setAnalyzerConsumer`, the taps and `getPreparedRate / Stereo` (`PluginProcessor.h:252-345`) [ARGUED]. Map the template onto it like this:

| Template assumes | Exists as | 3.1 action |
|---|---|---|
| `audioProcessor.displayModel` (member, `takeChangedPackets()`) | `DisplayModel (processor&)`, a standalone class with `poll()`, `packetFor()`, `allPackets()`, `headerVar()` | The editor owns it (3.2). In 3.1, register `requestEqSections` as a no-op that completes, and emit no `eqSections`. |
| `takeAnalyzerFrame (bool)` | `SpectrumAnalyzer (processor&)`, which registers as consumer on construction | The editor owns it (3.3). Do not construct it in 3.1: construction turns the taps on. |
| `outputLevelDB` atomic | `readAndResetOutputPeak (0 / 1)` | 3.3 (meter). In 3.1 emit nothing. |
| `editorOpen` atomic | none | Not needed: consumer registration covers the analyzer. Drop it. |
| `presetManager` | not owned yet (module linked; STATUS: "first used in Stage 4") | Call C2. |
| `createEditor()` | returns `GenericAudioProcessorEditor` (`PluginProcessor.cpp:1240`) | Swap to the WebView editor. |
| `setListenBand (n)`: 1–24, 0 = off | `listenBand`: 0–23, −1 = off | Store `n - 1`, clamped. Reset it to −1 in the editor destructor (U2). The state machine is 3.3. |

- **Memory traps that apply here:**
  - Completions are dropped while the WebView is hidden, and async native calls need a SafePointer.
  - A one-shot state push after a preset load goes stale: the page pulls `getUiState` on init.
  - Readouts use `getScaledValue()`.
- **CMake:** merge `v1-CMakeLists-SNIPPET.txt`. It makes one `UIBinaryData` target holding `index.html`, `js/i18n.js`, the JUCE frontend, `modules/preset-manager.js`, `modules/vu-meter.js`, the eb-garamond css and three woff2 files.
  - Do not reword the existing "plugin-declaration command name" comment guard.
  - Keep the literal `setSize (960, 600);`.

---

## Q3. Native functions

`getNativeFunction(` names in `v1-ui.html` and `preset-manager.js`, against the template's `withNativeFunction` names: 18 against 18, no difference either way [BENCH, grep in this session]. The page adds no vu-meter native calls. The page's `outputLevel` event belongs to the 3.3 mount.

Every one of the 18, plus `pageReady` (P1), must be registered in 3.1. An unregistered preset function would leave the preset bar dead while build, auval and pluginval all pass.

---

## Q4. The dragonfly plate (U4)

### Cost [BENCH]

Headless Chromium on `v1-ui-test.html`, 1600 x 1000 at device scale 2, two runs each, plate shown against `display: none`:

| | Shown | Hidden |
|---|---|---|
| Main-thread task time at idle (mockup's 30 Hz loop) | 199, 202 ms/s | 202, 195 ms/s |
| 20 alternating resizes 800 x 500 ↔ 1600 x 1000: task / layout | 57, 61 / 5.5, 4.6 ms | 71, 78 / 4.4, 4.4 ms |

The plate is a static DOM layer. The canvases redraw above it without invalidating it, and a resize re-rasterises it once. No static-layer cache is needed. Caveat: these are Chromium software-raster numbers, not WKWebView, and compositor raster time is not in TaskDuration. 3.3's 60 fps row re-checks the plate under real load.

### Port (P4)

- **Element:** take the `<svg id="plate" viewBox="-300 -205 600 545">` and `buildPlate()` (`v1-ui-test.html:334`, `:879-912`), not `v1-ui.html`'s `<div id="plate">` placeholder.
- **Geometry:** take the test page's (`:167`: `right 1.5 %`, `top 3 %`, `height 94 %`, `width auto`, `max-width 62 %`, opacity 0.3). Its rotation is inside the SVG (`rotate(-21 0 60)`). `v1-ui.html:269` rotates the box itself and uses `width: 60%`. Remove that, or the plate rotates twice.
- **Colours:**
  - The sketch hard-codes `#5C4033` and `#FAF0E6`. They equal `--frame` and `--paper-light` today, but they would not follow a token change.
  - Use classes with `stroke: var(--frame)` and `fill: var(--paper-light)` (a CSS property accepts `var()`; a presentation attribute does not).
  - The page has a single paper theme (no dark-mode block in `v1-ui.html`), so U4's "both themes" has nothing further to cover.
- **Placeholder text:** remove the placeholder comment at `v1-ui.html:262-268` and the `images/dragonfly_plate.png` reference. No binary asset is added.
- **Seed:** keep the seeded RNG (`sd = 24`), so the plate is identical on every open.

---

## Q5. Gate fixtures

- **`tests/ui-stub/generic-overrides.json`** [ARGUED, `scripts/serve-ui.js:338-372`, `scripts/ui-stub/generic-juce-stub.js:154-215`]:
  - The stub merges `sliders`, `toggles`, `combos` and `natives` from this file last, over `.planning/params.tsv`.
  - The TSV path always gives skew 1, and `params.tsv` does not exist yet.
  - Generate both files from `O-ParametricEQ-param-dump`, plus a small script that writes the 48 freq / Q skews (0.203214, 0.186652) and the 72 choice lists with their defaults from `parameter-spec.md`. Commit the generator's output, with a `_why` key as O-Bells does.
  - Without the skews, the stub's `setNormalisedValue` maps typed frequencies wrongly, and a typed-entry check is vacuous.
- **`tests/i18n-states.json`:** add states, appended last (memory: new states go last; bump the probe's state-count pin):
  - an `eval` that enables band 1 and selects it, so the strip's 14 hidden labels are measured;
  - the existing popover states, copied from the O-AnalogEQ shape (`name`, `click`, `eval`).
- **Order:** the five gates of checklist § 8 must all pass before the 3.1 commit. Exit 77 is not a pass.

---

## Q6. Owed render rows (U8; VERIFICATION-2.6 Issue 2)

### Count

The list names **8** mutants (LS10, LS11, LS12, LS15, PX26, PX27, PX38, PX39). VERIFICATION-2.6's own table splits the 20 survivors three ways:
- 8 real, caught by the probe only;
- 3 real but ungated: DM2, AN1, PX22;
- 9 equivalent or not observable.

"Nine" in CONTEXT U8 and ROADMAP 3.1 is a copy of the commit message's swap. The plan corrects the wording; this is a count, not a scope change.

### Rows

The rows go in a new `owed26RowsXr()`, called after `owedRowsXr()`, and are named `XR-<mutant>` after the XR-px17 precedent. They use a small `owedRender` helper: events cut the blocks, with an after-block hook. `renderSized` has no such hook.

| Row | Port of | Kills | Measures |
|---|---|---|---|
| XR-ls10 | E1 | LS10, PX27 | An f or Q edit of the listened band at R/3 into the incoming voice's ramp (engage 3, change 3 → 7; 48 / 96 kHz, block 64) first changes the output in [move + R, + grid). Settled-listen edits move the output. 10 checks |
| XR-ls11 | T1 | LS11, LS12 | Listen state (move, position, target) after every sample equals the re-stated P6 machine through latch, release, engage, a settled f edit, a gain edit and a type-family edit; 4 rate / width cases. 4 checks |
| XR-ls15 | N1 | LS15, PX26 | A NaN input at 20 037 (listen settled on 3, and no listen; mono / stereo; blocks 64 / 512): the output is finite from the next block start. 8 checks |
| XR-px38 | R1 | PX38 | `listenBand` 24, 1000, −2, INT_MIN, INT_MAX, then −1, random blocks: memcmp-equal to no listen |
| XR-px39 | X1 | PX39 | Listen on a disabled band, bypass on / off 128 samples apart (parameter and `processBlockBypassed`; 48 / 96 kHz): mean \|y − x\| over [r − 4, r + 12) < 0.05 × the settled value. Unmodified reads 1.2e-4 / 2.4e-5 |
| XR-px22 | new | PX22 | Mono, listen settled on a Left bell, placement Left → Right → Mid → Side → Stereo: no move, target stays, memcmp-equal. Control: the same edits on stereo do start moves |
| XR-dm2 | new | DM2 | DisplayModel stereo → mono → stereo at the same rate (48 / 96 kHz): `poll()` returns all 24, the header follows the layout, L / M / S / R packets carry Stereo on mono. A poll with no edit returns empty. 10 checks |
| XR-an1 | new | AN1 | Both taps, a 2 s stall (126 464 drops): each frame's newest N drained indices are consecutive, checked against a re-stated drain model on every call. Vacuity guards: frames occur before the stall and resume after it |

### Results [BENCH]

- **Unmodified:** every row passes. The full harness reads 163 PASS / 0 FAIL in 123.6 s.
- **Under its mutant:** every row fails, and in most cases only the intended row does (LS12 also trips XR-px22).
- **Side kills:** LS8, PX40, PX23, ST2, ST3, ST5, PX21, LS9 and LS34 also fall to the new rows.
- **Equivalent mutants:** the 9 still survive, as expected.
- **Not run:** `--long` and the designer harness. The patch adds rows only and changes no existing one.
- **Caveat:** XR-ls10 at 96 kHz first differs at 3905 against 3904, inside the one-grid tolerance inherited from E1.

### P6

Apply `research-3.1/probe-port/owed26-render-harness.patch` as written. The scratch-only `OPEQ_OWED26_ONLY` switch is not in it. Re-run default and `--long`. Re-run the 11 mutants with `research-3.1/probe-port/run.py` as the 3.1 verify control.

---

## Plan inputs (no call needed)

| ID | Item |
|---|---|
| P1 | 196 relays, no fallback; `pageReady` gate on the timer; checklist § 0 measurements become acceptance rows in the Standalone and Logic |
| P2 | Q2 mapping: the editor owns `DisplayModel` (3.2) and `SpectrumAnalyzer` (3.3); `listenBand = n - 1`, reset in the destructor; drop `editorOpen` / `outputLevelDB` |
| P3 | All 18 natives + `pageReady` registered; the grep diff is a 3.1 acceptance row |
| P4 | Plate port per Q4 (test-page geometry, rotation inside the SVG only, token colours via CSS) |
| P5 | `params.tsv` + generated `generic-overrides.json`; band-selected `i18n-states.json` state appended last |
| P6 | Owed rows patch; ROADMAP / CONTEXT "nine" → "eight" |
| P7 | Disabled-state rules (gain / slope / Q per type and slope) live in the page; the check-ui-labels band state covers their labels |

## Calls

**Resolved 2026-11-15 by Taylor: C1 and C2 both as recommended.** The plan binds all 196 relays, removes the fallback from ROADMAP 3.1, and has the processor own `OuariconPresetManager` from 3.1 (excluded `output_gain` and `bypass`, validated apply, `PresetSaveGuard`, empty factory set).

**C1 — Drop the selected-band fallback (U6).** All 196 relays open in ≤ 274 ms and settle a preset in 1 frame. The fallback would save about 15 ms and needs a page rework.
- **Recommended:** bind all 196 and remove the fallback from ROADMAP 3.1's criterion.
- Keep the 500 ms row as a measured acceptance check in Logic. WebView2 timing becomes a Stage 4 Windows CI note, not a 3.1 gate.

**C2 — Preset manager in 3.1, not Stage 4.** The page mounts `preset-manager.js` and calls ten preset natives, so they must exist in 3.1.
- **Recommended:** the processor owns an `OuariconPresetManager` from 3.1, with:
  - `setExcludedParameters ({ "output_gain", "bypass" })` (the suite rule: presets never set output gain);
  - `setValidatedApply (true)`;
  - O-AnalogEQ's `PresetSaveGuard.h` ported;
  - an empty factory set.
- Factory content stays Stage 4. User save / load work from 3.1, which also lets the preset-settle acceptance row run through the real path.
- **Alternative:** stub the ten natives to complete with empty answers until Stage 4. The preset bar is then inert, and the settle row runs through `setStateInformation` only.
