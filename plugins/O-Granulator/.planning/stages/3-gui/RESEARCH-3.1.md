# Stage 3 — GUI · Phase 3.1 (Layout + controls + bindings) — Research

**Plugin:** O-Granulator
**Stage / phase:** 3-gui / 3.1 of 3
**GSD phase:** research
**Date:** 2026-11-06
**Tree:** `main` @ `ba80f6e3` (`Source/`, `tests/`, `modules/` unchanged since the Stage-2-verified `9831f498`)
**Inputs:**
- `CONTEXT-3.1.md` (D26–D29 locked; four open questions)
- `ROADMAP.md` Stage 3; `mockups/v2-integration-checklist.md` §1–9; `mockups/v2-PluginEditor.h/.cpp` (template); `Source/ui/public/index.html` (= `v2-ui.html`, byte-identical, `cmp` clean) + `js/i18n.js` (= `v2-i18n.js`)
- Source: `PluginProcessor.h/.cpp`, `PluginEditor.h/.cpp` (Stage 1 resource map), `GranulatorParamIds.h`, `dsp/Engine.h/.cpp`, `dsp/WindowMorphBank.h`, `dsp/ModMatrix.h`, `dsp/ControlGrid.h`, `CMakeLists.txt`; `.planning/params.tsv` (runtime param-dump)
- Modules: preset-manager 1.1.1 (`cpp/OuariconPresetManager.h`, `js/preset-manager.js`), pitch-collections 1.0.0 (`cpp/PitchCollections.h`)
- Repo tooling: `scripts/check-ui-labels.js` (state runner), `scripts/serve-ui.js` (stub seeding), `scripts/ui-stub/generic-juce-stub.js`, `scripts/param-dump/ParamDump.cmake`; `plugins/O-ReverseDelay/tests/ui_frontend_check.js` (closure-gate precedent); `plugins/O-Lyrica/Source/PluginProcessor.cpp` (preset-manager + custom state precedent)
- Memory: native-fn bridge gap, one-shot state push stale on preset load, stale `<CustomState>` child, presets never set output gain, generic ui-stub neutral seeds, forced click under popover, UI fixture state producible by C++, Standalone hands-on via AX, lambda range invisible to the JS frontend, webview completion gated on isVisible

**Scratch:** `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/432dc842-8c6f-4381-8b30-b1391444c480/scratchpad/` (parity lists `page.txt` / `cpp.txt`). No file in the plugin tree or `modules/` was edited. Nothing was built or installed.

All four open questions are answered (§9). There are **thirteen findings** (R104–R116). One is a **blocker** the checklist does not name: **R105 — the processor has no preset manager**, so 10 of the 21 native functions have nothing to call. Two would make a 3.1 gate pass vacuously if missed: **R112** (the gate stub never seeds combos or scale state) and **R113** (no existing method proves 220 bindings both ways).

---

## Summary for the planner

| # | Finding | Severity | Plan action |
|---|---|---|---|
| R104 | **Parity holds today (measured).** Native fns: page (`getNativeFunction('…')` ×11) + `preset-manager.js` (×10) = **21 = 21** `withNativeFunction` in the template, 0 diff. Relay type: APVTS (runtime dump) ↔ page tables ↔ editor `kSliderIds/kComboIds/kToggleIds` = **140 / 59 / 21, 0 mismatches, 0 duplicates** (page: KNOB 124 + FADER 16 float; CHOICE_KNOB 12 + SEGMENT 9 + SELECT 2 + 4 scale + 32 matrix choice; TOGGLE 21) | Baseline | Pin both in `tests/ui_frontend_check.js` (§1) |
| **R105** | **No `OuariconPresetManager` in the processor.** The template calls `processorRef.getPresetManager()` in 10 fns; it does not exist. | **Blocker** | Add the manager (files only); keep the Stage 1 session codec; custom-state callbacks; exclude `output_gain`; validated apply (§2). **Sign-off: seed in presets** |
| R106 | **Telemetry consumer audit.** Every field the page reads is guarded (`Array.isArray`, `Number(x‖0)`, `typeof`). An omitted key means "default". Nothing throws in the 30 Hz loop. `modNorm` is an **object keyed by destination index**, not an array. `pitch` is an **object** `{midi, cents, clarity}`, omitted when there is none | Q1 closed | 3.1 payload in §3. Expected stub artefacts listed so verify doesn't flag them |
| R107 | **`getWindowCurve` can be real in 3.1.** The WindowMorphBank is immutable after the constructor, so a message-thread read is safe and copy-only | Scope (cheap) | One public `const` Engine method (§4) — no stub |
| R108 | **Scale revision is bumped by the setters too** (`setDegreeMask`, `setCustomScale`), not "only restore / preset" as the checklist says. Effect: extra re-pulls only. Native completions are ordered, so at worst one chip flickers for a frame during a paint-drag. No draft clobber | INFO | Keep the processor as it is (Stage 2 verified). Correct the checklist text. Hands-on watch item (§5) |
| R109 | **`getCollectionTable(31)` returns `&customTable` with no lock.** A host that calls `setStateInformation` off the message thread can tear `buildScaleState`'s read. It heals itself on the next revision pull, but it is a race | Minor defect | Editor: for index 31 use `getCustomTable()` (a locked copy) (§5) |
| R110 | **`getParameterDefaults` needs no special case** for the 12 division knobs. `convertFrom0to1(getDefaultValue())` gives a choice INDEX. Every Float range is plain skew (`setSkewForCentre` only sets skew), interval 0, no lambdas. So the page's `scaledToNorm` is exact | Q2 closed | Template as is (sliders + combos; toggles not needed) |
| R111 | **`i18n-states.json` is drivable by clicks.** The generic stub's `setChoiceIndex` fires `valueChangedEvent`, so the Tempo segment click runs `applySync()`. The runner is cumulative with `force: true`, so the popover goes LAST | Q3 closed | Order and selectors in §6 |
| **R112** | **The gate stub is half-seeded.** `params.tsv` seeds sliders (linear, skew 1: the TSV has no skew) and toggles. **Combos are never seeded:** every combo gets `['1','2','3','4']`. `getScaleState` and `getTelemetry` return `null`. So check-ui-labels measures division readouts of "1", empty chip rows and "0 sounding" | **Vacuous if missed** | `tests/ui-stub/generic-overrides.json` generated from C++: 59 combos `{choices, def}`, a `getScaleState` fixture, and a widest-value `getTelemetry` (§7) |
| **R113** | **UI-01 "bound both ways" has no method yet.** Playwright cannot attach to WKWebView, and pedalboard sees parameters, not the UI | **Gate design** | Three layers: static closure (all 220) + a page two-way probe under the stub (all 220) + a hands-on spot check in the real WebView (§8) |
| R114 | **Resource provider:** the Stage 1 free function `granulatorUiResource` already maps all 9 binary-data entries with bare paths (it passes check-i18n [8]). The template's member `getResource` duplicates it | Simplify | `withResourceProvider ([] (auto& u) { return granulatorUiResource (u); })`, and drop the duplicate |
| R115 | **Console builds.** `ParamDump.cmake` drops any source whose path matches `PluginEditor`, and builds with `JUCE_WEB_BROWSER=0`. Processor additions (preset manager, telemetry read, window curve) DO compile into the harness and param-dump | Constraint | WebBrowserComponent code only in `PluginEditor.*`. Harness default set + N3/N2/N1/N0 `--strict` + R12 re-run after the processor edits (audio must not move) |
| R116 | The template's `buildTelemetry` drops every live key when `readTelemetry` returns false. In 3.1 nothing is seqlocked (it is all relaxed atomics), so `readTelemetry` always returns true | Design | Define the full `Telemetry` struct now, with 3.2/3.3 fields zeroed; 3.2 adds the seqlock |

---

## 1. Parity baseline (R104)

**Native functions, measured.** `grep -o "getNativeFunction('…')"` over `index.html` gives 11 names. The 10 preset names come from `preset-manager.js:108-117`. Together they equal the 21 `withNativeFunction ("…"` in `v2-PluginEditor.cpp`, with an empty diff (`scratchpad/page.txt` vs `cpp.txt`).

**Relay type, measured.** APVTS types come from `.planning/params.tsv`: `boolean` → bool, `discrete` → choice, else float. Page types come from `KNOB_IDS` / `FADER_IDS` (float), `CHOICE_KNOB_IDS` / `SEGMENT_IDS` / `SELECT_IDS` + `sN_scale` (`initScaleBand`) + `modM_source/dest` (`initMatrix`) (choice), and `TOGGLE_IDS` (bool). Result: 220 = 220 = 220 by ID and TYPE, with no ID bound twice.

**Gate form.** Port `O-ReverseDelay/tests/ui_frontend_check.js` (§3 bridge closure, §11 member order, §15 four-way closure) to `plugins/O-Granulator/tests/ui_frontend_check.js`. One difference:
- **The APVTS side must come from the runtime dump, not a regex.** Granulator's `createParameterLayout()` is table-driven (`GranulatorParamIds.h`, IDs built by concatenation, `PluginProcessor.cpp:195-214`), so a regex finds nothing (the param-dump README's O-Prism case). The gate step re-runs `O-Granulator-param-dump > .planning/params.tsv`, and `git diff --exit-code .planning/params.tsv` must be clean.
- Also pin: `node --check` on the inline module, a single `init()` as the last statement, the `Juce` namespace, no viewport units, every `juce_add_binary_data` SOURCE ↔ a resource branch, and `Source/ui/public/modules/*` not tracked (it is gitignored via `.gitignore:27`; the build drops `preset-manager.js` and `reverb-unit.js` there).

## 2. Preset manager (R105) — blocker

The processor (`PluginProcessor.h`) has no `OuariconPresetManager`. The page's preset bar (`initPresetBar`) and the template's 10 preset fns need one. Factory presets are Stage 4. 3.1 needs the instance and the bridge.

**Recommended wiring** (O-Lyrica `PluginProcessor.cpp:531, 605-640` is the precedent):

1. `OuariconPresetManager presetManager { parameters, "O-Granulator" };` declared after `parameters`, plus `OuariconPresetManager& getPresetManager()`. Directory creation is lazy (v1.5.0, constructor comment), so there is no I/O during auval.
2. **Session state stays on the Stage 1 codec** (`copyState()` / `replaceState()` + `removeCustomChildren`). Never route it through `getStateAsXml` / `setStateFromXml`. That pair leaves `<CustomState>` in the live tree (critical_preset_manager_stale_customstate_child), and Granulator is immune today exactly because it does not use them (O-Octagon class). The HA host-cache and S1–S4 state gates already cover this codec.
3. `setCustomStateCallbacks`:
   - **save** → `{ custom: {name, text}, masks: [{stream, collection, bits(16 hex)}…non-default only] }`. These are the same encodings as the XML codec (16-hex masks, never `getLargeIntValue`).
   - **load** fires on EVERY apply, with a void var when the preset has no custom state (module v1.0.7). Order: `resetNonParameterState()` FIRST, then apply what is present, then `bumpScaleRevision()`. That is the same order as `setStateInformation`, so factor a shared `applyNonParameterState (custom, masks)` helper used by both paths.
4. `setExcludedParameters ({ "output_gain" })`. Output gain is the user's control, and preset load must not reset it either (feedback_presets_never_set_output_gain). This shared module copy already has the v1.1.0 hook, so no fork is needed.
5. `setValidatedApply (true)`: gestures per parameter, so Touch/Latch hosts record a load, plus a 0–1 clamp.
6. **Sign-off — seed in presets.** The seed is non-parameter state, and it decides the grain pattern. Options: (a) carry it in `customState` (a preset reproduces its texture; recommended); (b) leave it out (a preset sets the parameters only). COMPAT-02 (Stage 4) already round-trips it through the session.
7. The page's `onPresetChanged → pullScaleState()` and the revision bump in step 3 both re-pull. That is redundant but harmless.

The header compiles into the consoles (`ParamDump.cmake:197-214` defines `JucePlugin_VersionString`). The member adds no audio path, but the harness default set and the N-suites re-run anyway (R115).

## 3. `getTelemetry` in 3.1 (R106, R116)

**The page's consumer** (`index.html:2829-2847, 3022-3250`), field by field:

| Key | Page use | Guard | 3.1 value |
|---|---|---|---|
| `scaleRev` | re-pull on change | `typeof === 'number'` | `getScaleStateRevision() & 0x3fffffff` |
| `sources[11]` | dock chip tint | `Array.isArray` | `engine.getSrcTelemetry (1…11)`. Engine order (`modSources()`: LFO 1–4, Env, Pitch, Onset, Macro 1–4) **==** page `data-src` 1…11 (verified) |
| `lfo[4]` | LFO dot height + readout | `Array.isArray` | `sources[0…3]` |
| `lfoPhase[4]` | LFO dot x | `Array.isArray` | `[0,0,0,0]` (3.3). The dot sits at the left edge |
| `env` | env history | `Number(‖0)` | `sources[4]` |
| `inPeak` | level lane | `Number(‖0)` | omit (3.3) |
| `onset` | pill flash > 0.5 | `Number(‖0)` | omit, or `sources[6]` (the Onset source value). Planner's choice; 3.3 owns it |
| `onsetFired` | history ticks | truthy | omit (3.3: needs an audio-side counter) |
| `pitch` | note / cents / clarity bar | `p && Number(p.midi) > 0` | `{ midi: getPitchNote(), cents: round(100·(n − round n)), clarity: confident ? 1 : 0 }` when note > 0 |
| `modNorm` | ring dot on routed knobs | `typeof === 'object'` | object `{ "<d>": destTelemetry[d] }` for **routed d only**: computed on the message thread from APVTS `modM_source > 0 && modM_dest == d` (≤ 16 keys). The engine's `destTelemetry` is v01 when routed, base01 otherwise (`Engine.h:132-135`) |
| `out[2]` | output meter | `Array.isArray` | omit (3.3) |
| `counts[4]`, `limit` | "N sounding", "N / limit", bars | `Array.isArray`, `> 0` | omit. `limit` falls back to `[32,64,96,128][grain_limit]` (default index 3 → 128) |
| `loopGainDb` | loop readout | `typeof === 'number'` | omit (reads "Open") |
| `wave`, `spanSec`, `grains` | buffer strip | `length > 1`, `> 0`, `Array.isArray` | omit (3.2) |

**Expected 3.1 stub artefacts.** These are not defects: they must not fail 3.1 verify. Per stream: "0 sounding". Header: "0 / 128". Loop: "Open". The LFO dot is pinned at phase 0, the onset history is empty, the level lane is flat, and the buffer strip has no waveform or grains. "Writing stopped · *buffer_length* held" shows the length knob, not heldSpan (3.2).

**Struct now, fill later (R116).** Declare the full `GranulatorProcessor::Telemetry` (every checklist field) and a `readTelemetry (Telemetry&)` that fills the 3.1 fields from relaxed atomics, zeroes the rest, and returns **true**. 3.2 adds the seqlock (grains + wave) and the false-on-torn path. The editor's `buildTelemetry` then only stops omitting keys as fields go live. Copy-only reads, no editor access to the pool or scheduler (CONTEXT constraint).

## 4. `getWindowCurve` (R107)

The figure must be the engine's curve. The pieces already exist:
- `WindowMorphBank` is built once in the Engine constructor and never rebuilt (`WindowMorphBank.h:50`, `Engine.h:566`), so a const read from the message thread is race-free.
- Mapping (`ModMatrix.h:107-108`, `Engine.cpp:352-353`): `step = WindowMorphBank::stepFor (clamp (window, 0, 1))`; `tilt = WindowLut::makeTilt (0.5f + 0.45f · clamp (skew, −100, 100) / 100)`.
- Sample `WindowMorphBank::read (bank.table (step), tilt, i / (n − 1))` for i = 0…63.

Add **one public const Engine method**: `void windowCurve (float window, float skewRaw, float* out, int n) const noexcept`. Today `getBank()` sits under `OGRAN_TEST_HOOKS` (`Engine.h:405`); don't promote the hook. The processor reads the BASE (unmodulated) `sN_window` / `sN_window_skew` raw values from APVTS. Step 0 / skew 0 renders through the closed-form Hann on the audio thread (`Engine.cpp:1341-1344`), and the bank's step 0 is Tukey α = 1 (= Hann), so the figure matches. This is cheap enough that the D26 "flat stub" allowance isn't needed.

## 5. Scale state (R108, R109)

**R108.** `setDegreeMask` (`PluginProcessor.cpp:399`) and `setCustomScale` (`:361`) call `bumpScaleRevision()`. The checklist says the setters never bump. Walk-through of the consequence:
- `commitMask` paints optimistically, then awaits `setDegreeMask`, which answers with the full state, and ingests it.
- The next telemetry poll sees the new revision and re-pulls.
- Native calls complete in issue order on the message thread. A pull issued before commit *k* can land after commit *k*'s optimistic paint, so it shows pre-*k* for one frame, until *k*'s own reply lands. Worst case: a one-frame chip flicker during a paint-drag.
- `ingestScaleState` re-renders triggers, chips, the shared-status line and the circle, **never `#sp-text`**, so a custom-scale draft is never clobbered.

Keep the processor as it is (Stage 2 verified; the HA gate pins the codec). Amend checklist §2 to say "bumped on every scale-state change". Hands-on watch item: paint-drag across 53-EDO chips, no visible flicker.

**R109.** `getCollectionTable (31)` returns `&customTable` without `customLock` (`:367-374`, "message thread: setCustomScale runs there too"). Some hosts call `setStateInformation` off the message thread, and that path runs `setCustomScale`, which writes the table under the lock while `buildScaleState` reads it unlocked. The `std::array` storage can't overrun, so there is no crash, and the revision bump re-pulls a clean frame. It is still a race. Editor fix: `c == kCustomCollection ? tableToVar (processorRef.getCustomTable()) : …`. No processor change.

## 6. `tests/i18n-states.json` (R111, D29)

Runner facts (`check-ui-labels.js:594-645`):
- States run **cumulatively on one page**.
- `click` uses `force: true` and waits 250 ms. Click arrays apply in order, and every selector is asserted to exist.
- `eval` runs in the page and waits 350 ms.
- The generic stub's `StubComboBoxState.setChoiceIndex` calls the listeners (`generic-juce-stub.js:221`). So a segment click (`bindSegmented` → `st.setChoiceIndex`) runs the page's `applySync()` exactly as the relay echo does in the plugin.

Recommended order (the popover goes last: pattern_forced_click_under_popover):

```json
[
  { "name": "mod-free",          "click": "#tab-mod" },
  { "name": "mod-tempo",         "click": "#seg-sync_mode button[data-index=\"1\"]" },
  { "name": "streams-tempo",     "click": "#tab-streams" },
  { "name": "fx",                "click": "#tab-fx" },
  { "name": "streams-free",      "click": ["#tab-streams", "#seg-sync_mode button[data-index=\"0\"]"] },
  { "name": "scale-list-open",   "click": "#scale-trig-s1" },
  { "name": "custom-editor-open","eval": "document.getElementById('scale-trig-s1').click(); document.getElementById('scale-edit-s1').click();" },
  { "name": "settings-open",     "eval": "document.getElementById('sp-close')?.click(); document.getElementById('gear-btn').click();" }
]
```

- `#seg-sync_mode` is in the header (`index.html:575`), visible on every page. MOD in Tempo shows the four `lfoN_rate_sync` knobs. STREAMS in Tempo shows the eight size/density division knobs.
- Selectors are verified present: `#tab-streams/fx/mod` (`:582-584`), `#scale-trig-s1` (`:625`, click toggles the list, `:2480`), `#scale-edit-s1`, `#gear-btn` (`:580`), `#sp-close`.
- Use the `eval` form wherever the page could paint over the target: the scale list is a floating panel over the STREAMS grid. A click state counts as applied only when `elementFromPoint(centre)` is the target. Check each click state once while writing the file (pattern_forced_click_under_popover, O-Lyrica second instance).
- **Plus one matrix state** (needs R112's real choices): `eval` sets slots 1–3 to the destinations with the longest localised labels, e.g. `setChoiceIndex` on `mod1_dest` via the page's own select `change`. The dest `<select>` caption is content-sized (pattern_appearance_none_select_exposes_caption_width). Default "None" measures the narrowest caption.

## 7. Stub seeding (R112) — vacuous if missed

`serve-ui.js:290-370` `parseParamDumpTsv` / `stubSeed`:
- Sliders get `start / end / def` from `textAtMin / textAtMax / defaultText`, with **skew 1 always**, because the TSV has no skew column.
- Toggles are seeded from the `boolean` flag.
- **Combos are never seeded.** `sync_mode` ("Free") parses to null and is skipped. Numeric-looking discrete rows (`s1_size_sync` "1/64 T" → 1, "2 bars" → 2) become bogus *slider* entries that nothing reads. Every `getComboBoxState` therefore returns `NEUTRAL_CHOICES = ['1','2','3','4']`, default index 0 (`generic-juce-stub.js:161, 210-222`).
- Natives fall to `benignDefault`: `getScaleState` → null (`/^get/`), `getTelemetry` → null, `getWindowCurve` → `[]` (`/Curve$/`), `getParameterDefaults` → null.

Consequences for the 3.1 gates:
- The 12 division knobs read "1" (real: "1/64 T", "2 bars"). `grain_limit` reads "1" (real: "128"). Matrix selects hold 4 options.
- Chip rows are empty: there is no table, so `renderChips` has nothing to draw. The custom editor opens on an empty model.
- "0 sounding" is measured where the real widest is "128 sounding" / "128 actifs".
- The check-ui-labels banner still says `seed=param-dump (220 rows)`, which looks complete.

**Fix: `plugins/O-Granulator/tests/ui-stub/generic-overrides.json`, generated from C++ and never hand-typed** (pattern_ui_fixture_state_must_be_producible_by_cpp):
- `combos`: all 59, `{ choices: AudioParameterChoice::choices, def: default index }`.
- `sliders`: the skewed floats with their real `skew` (from `getNormalisableRange()`), so the stub's knob positions match.
- `natives.getScaleState`: the processor's default state through the same encoding as `buildScaleState` (tables 1–31 from `getCollectionTable`, custom 12-EDO, masks all-on).
- `natives.getTelemetry`: a widest-realistic frame: `counts [128,128,128,128]`, `limit 128`, `loopGainDb −0.4`, `pitch {midi 61, cents −49, clarity 1}`, `scaleRev` equal to the fixture's revision, and every 3.2/3.3 key present. The poll then ingests it at 30 Hz, constant.
- `natives.getParameterDefaults`: the real defaults.
- `natives.getWindowCurve`: 64 points from R107's method.

Producer: a `--dump-ui-overrides <path>` mode in the existing render-harness console. It already constructs the processor, the editor TU is excluded, and the JSON is assembled from processor accessors. Commit the JSON. The gate step regenerates it, and `git diff --exit-code` must be clean.

## 8. UI-01 test method (R113, Q4)

The failure classes that ship green through ninja, auval and pluginval:
- ID typo
- wrong relay TYPE (a Bool or Choice on a slider relay attaches and never updates)
- page binding the wrong ID or type
- a missing attachment (`jassert`, Debug only)
- a page listener that never repaints

JUCE's relay ↔ attachment plumbing itself is library code. Three layers:

1. **Static closure, all 220** (`tests/ui_frontend_check.js`, §1): fresh param-dump == editor tables == page tables, by ID and TYPE; 21 = 21 native; member order relays → webView → attachments; the constructor creates them in the same order. Negative control: a scratch copy with one ID moved from `kSliderIds` to `kToggleIds` must FAIL.
2. **Page two-way probe under the generic stub, all 220** (`tests/ui_binding_probe.js`, Playwright via `serve-ui.js`, with R112's overrides):
   - *host → UI* (what the relay does on automation or preset recall): set each stub state and fire `valueChangedEvent`, then assert the control's DOM moved. For knobs, faders and choice knobs: `val-<id>` text == `FORMAT[id](scaled)` or `choices[idx]`, and the stem transform changed. Toggles: `aria-pressed`. Segments: the pressed button. Selects: `selectedIndex`. Scale triggers: the trigger label.
   - *UI → host*: focus + ArrowUp on knobs and choice knobs; a click on toggles and segments; a `change` on selects; assert the stub state value changed and `sliderDragStarted/Ended` were paired.
   - Liveness: the assertion count must equal 220. Negative control: remove one `bindKnob` call in a copy of the page → FAIL.
   - Quiesce first: wait for `init()` and the first telemetry tick.
3. **Real WebView spot check (hands-on, at 3.1 verify):** Standalone + one DAW. One control of each kind on each page (knob, division knob, fader, segment, select, scale trigger, toggle, matrix row); one automation lane per relay type; preset save/load (preset bar + scale chips + custom name re-pull); **save → reopen → change → save → reopen** (two reopens). Playwright cannot attach to WKWebView. The AX/CGWindow route (pattern_standalone_hands_on_via_ax) steals focus, so keep this human.

Not used: pedalboard sees parameters, not the UI. A runtime self-test inside the shipped page is out, because v2 is locked (D28).

## 9. Open questions → answers

| CONTEXT question | Answer |
|---|---|
| Exact `getTelemetry` shape; do empties throw at 30 Hz? | §3 table. Every read is guarded. Omitted key = default. `modNorm` is an object keyed by dest index; `pitch` is an object omitted when none. Nothing throws (R106) |
| Must `getParameterDefaults` special-case the 12 division knobs? | No. A choice reports its index via `convertFrom0to1`; `bindChoiceKnob` dblclick calls `setIndex(default)`. All float ranges are plain skew, interval 0, no lambdas, so `scaledToNorm` is exact (R110) |
| How to drive Tempo / custom editor in `i18n-states.json`? | A segment click works under the generic stub (`setChoiceIndex` fires listeners). Order and selectors in §6. Use `eval` for overlap-prone targets; popover last (R111) |
| Host-automation reflection method for 220 params? | Static closure + stub two-way probe (both all 220) + a real-WebView hands-on sample (R113, §8) |

## 10. Sign-offs needed at plan (Taylor)

1. **R105 — seed in presets** (recommended: yes, carried in `customState`).
2. **R105 — `output_gain` excluded from presets** (load does not reset it; follows the O-Bells v4.8.0 rule). Recommended.
3. **R108 — keep setter-bumps** and amend the checklist text instead of changing the verified processor. Recommended.

## 11. Carry-ins for plan

- `createEditor()`: `#if JUCE_WEB_BROWSER return new GranulatorEditor (*this);`, include `PluginEditor.h` under the same guard. Consoles keep the generic editor.
- The template's `languageCode/Index`, `uiLanguage`, the scale accessors and `PitchCollections::tableFromText (4-arg) / tableFromScala / toScala` all exist with the template's signatures (verified). The only missing processor API is `getPresetManager`, `Telemetry` / `readTelemetry` and `getWindowCurve`.
- Every `webView->` use outside the constructor is null-guarded; never `setVisible (false)`; fixed 1200 × 800, `setResizable (false, false)`.
- The UI gates (check-i18n, fr-lint, zh-lint, check-ui-labels, boot-all-uis `--strict-tips`) run after R112's overrides and §6's states land. Exit 77 is not a pass. Read the `seed=` banner: it must say `+ overrides`.
- Audio must not move: after the processor edits (R105, R107, R116), re-run the harness default set (463) + N3/N2/N1/N0 `--strict` + R12, with an empty carried diff.
- Windows block kept verbatim. It is not exercisable on macOS until Stage 4 COMPAT-03.
- Carried, not 3.1: Env 1e30 pin → Stage 4; C7 calibration and event-aware `renderPool` (Z6a) harness items; W2o — widen seeds, never the threshold. The DAW pass (16 items) is at 3.3 verify (D27).

## Assumptions log

- `localStorage` for the hover-help toggle persists across editor reopen in the WKWebView and WebView2 hosts. The canon i18n block uses it suite-wide; checklist §9 tests it hands-on.
- The Onset source value (src 7) is a usable stand-in for `onset` (the pill flash) if the planner wants it in 3.1. This was not measured; 3.3 owns the exact pulse.
- Host threading for `setStateInformation` (R109) comes from general host behaviour. It was not reproduced here.
