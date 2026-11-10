---
title: "O-GranulatorInstrument Stage 3 (GUI) — Phase 3.1 Execution Plan"
created: 2026-11-10
juce_version: "8.0.15"
phase: "3.1"
summary: "Five waves: (W0) pre-flight + Stage 2 baselines + UI-gate pre-state; (W1) audio-neutral processor surface: preset manager (files only, output_gain + rec_arm excluded, slots as sources without blobs), scale / slot revisions, window curve, pristine overview fold, telemetry accessors, shared JSON encoders, harness rows ug31 + --dump-ui-overrides, commit C1; (W2) the editor: UI files copied, 3 CMake edits, DevLoader removed, template adapted to the real processor, createEditor switched, build / install / validators, commit C2; (W3) UI proof: overrides, gate baseline on the unmodified page, value-entry port (D41), state sweep, static closure, 261 x 2 binding probe, five UI gates, commit C3; (W4) Stage 2 regression re-run, HANDS-ON-3.1, SUMMARY / STATUS, commit C4. Version 0.1.0, no tag, no new params, no forked-file edit, no module edit."
stages: [3]
agents: [gui-agent, build-gate-runner]
---

# O-GranulatorInstrument — Stage 3 / Phase 3.1 Execution Plan (layout, controls, bindings)

## Goal

The finalized v1 page ships in the WebView editor at a fixed 1200 × 800, with all **261** parameters bound both ways by ID and type (176 slider / 70 combo / 15 toggle), all **34** native functions registered and completing on every path, presets live, slot actions live (load / relink / reset / embed / state), the DevLoader gone, and the parent 1.2.0 value entry ported. **Stage 2 audio does not move.**

3.1 closes neither UI-01 nor UI-02 (CONTEXT). ROADMAP 3.1 test criteria are the target: window opens at 1200 × 800 and matches the mockup; all pages render; controls bound two-way; host automation updates the UI.

**Out of scope:** lane live overview / record head / playheads / grain dots / drop UI checks / Record-in states / `clearSlot` / `saveRecording` (3.2); MOD sources, voices, `modNorm`, meters (3.3); HANDS-ON-2.1–2.6 and the Logic DAW pass (3.3 verify, D39); factory presets (Stage 4).

---

## Inputs (required reading for the executor)

| File | Purpose |
|---|---|
| `stages/3-gui/CONTEXT-3.1.md` | D37–D42, constraints. **Binding, not reopened.** |
| `stages/3-gui/RESEARCH-3.1.md` | Summary 1–12 (cited **RSx**), §1–§10 (**R§x**). Where it differs from the integration checklist, RESEARCH wins (checklist §3 "land v1-CMakeLists" is wrong, RS1). |
| `mockups/v1-integration-checklist.md` | §1–§9, as corrected by RESEARCH and the PDs below. |
| `mockups/v1-ui.html` (**M**), `v1-i18n.js`, `v1-PluginEditor.{h,cpp}` (**T**), `v1-i18n-states.json` | Sources for the copy / adaptation. `v1-CMakeLists.txt` is **not landed**. |
| This plugin | `Source/{PluginProcessor.h,PluginProcessor.cpp,PluginEditor.cpp,PluginEditorDevLoader.*,GiParamIds.h}`, `Source/dsp/{SlotLoader.h,SlotLoader.cpp,GiTelemetry.h,GiRecorder.h,Engine.h}`, `Source/dsp/forked/WindowMorphBank.h` (read only), `CMakeLists.txt`, `tests/render-harness/main.cpp`. |
| Parent (read, never edit) | `plugins/O-Granulator/`: `Source/PluginProcessor.{h,cpp}` (preset wiring `:185-188, 620-660`, window curve `:790-800`), `Source/dsp/Engine.h:155-165`, `tests/ui_frontend_check.js`, `tests/ui_binding_probe.js`, `tests/render-harness/main.cpp:765, 905` (`--dump-ui-overrides`), `.planning/stages/3-gui/{PLAN,SUMMARY}-3.1.md`, commit `e77d29c8` (value entry). |
| Repo | `scripts/{check-i18n,i18n-fr-lint,i18n-zh-lint,check-ui-labels,boot-all-uis,serve-ui}.js`, `scripts/ui-stub/*`, `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`, `modules/core/webview-drop-streaming/cpp/WebViewDropStreaming.h`. |

Plan-time facts (2026-11-10): last code commit under `Source/ tests/ CMakeLists.txt` = **`e4987f8a`**. `PluginProcessor.cpp` 807 lines (`createEditor:357`, DevLoader include `:27-33`, `applyNonParameterState:719`), `PluginProcessor.h` 321, `SlotLoader.h` 354 (`SlotInfo:86`, `getOverview:189`, `restoreSlots:206`, `getSavedSlots:209`), `CMakeLists.txt` 236 (DevLoader source `:67-69`, UIResources block commented `:145-164`, `OGI_DEV_LOADER` `:205-212`, tests block `:228-235`), harness `main.cpp` 1302 (arg ladder `:1165-1275`). `Source/ui/public/` holds only gitignored `modules/`. `tests/` has no UI files. Contract hashes as in STATUS frontmatter. **Line numbers drift — re-grep.**

---

## Sign-off (Taylor, 2026-11-10, at plan)

| Item | Decision |
|---|---|
| R§10.1 presets and slots | **Sources, no blobs.** A preset stores each slot's source / name / fileSize / contentHash / embed / trimmed, never `embedded`. Departs from `parameter-spec.md:355` ("embedded blobs"); the spec is LOCKED and is **not edited** — recorded as a signed-off deviation in SUMMARY-3.1, revisited at Stage 4 with factory presets. |

## Plan-phase decisions (resolve R§10)

- **PD1 [R§10.1] Slots in presets.** `customState.slots` = array of `{ index, source, name, fileSize (decimal string), contentHash (16 hex), embed, trimmed }` built from `loader->getSavedSlots()` with the blob dropped; only non-empty slots listed. Apply: **key present → `loader->restoreSlots()`** with exactly that list (slots not listed are cleared; an identical source is a no-op, PLAN-2.6 PD12); **key absent → slots untouched.** `rec:` sources resolve by path, then the Recordings search (rungs 1, 3).
- **PD2 [R§10.5] Preset manager.** `OuariconPresetManager presetManager { parameters, "O-GranulatorInstrument" }` after `parameters`; `getPresetManager()`; `setValidatedApply (true)`; `setExcludedParameters ({ "output_gain", "rec_arm" })`; `freeze` stays in. `customState` = `{ seed (int64 decimal string), custom { name, text }, masks [ { stream, collection, bits } ], slots (PD1) }`; apply → the existing `applyNonParameterState` (reset first; absent seed keeps the live seed) → slots (PD1) → bump scale revision. Files only: **never** call `getStateAsXml` / `setStateFromXml`; the session codec is untouched (RS8). No directory I/O in any constructor. No factory presets.
- **PD3 [R§10.3] Slot revision = fingerprint.** `getSlotStateRevision()` (message thread, non-const): copy the four `SlotInfo`, hash `generation, loaded, loading, missing, embed, hasBlob, tooLarge, trimmed, source, name, lastError`, bump a member `uint32` when the hash changes. No `SlotLoader` change.
- **PD4 Scale revision.** `std::atomic<uint32_t> scaleRevision`; bumped in `setCustomScale`, `setDegreeMask`, `resetNonParameterState`, `applyNonParameterState`. The engine never reads it.
- **PD5 [R§10.2] Pristine overview in 3.1.** `bool getSlotOverview (int slot0, float* peaks, uint8_t* dirty, int n, uint32_t& rev) const` folds `loader->getOverview (slot)` (min, max per 512 frames) to `n = 656` peaks in 0…1 (`max (|min|, |max|)` per bin); `dirty` all 0; false + zeros when not loaded. `rev` = `ovRev`.
- **PD6 `ovRev`.** Per slot `(uint32) (generation · 2 + (loaded ? 1 : 0))`. The executor confirms `generation` moves on load, relink, reset-done and record commit; if a path publishes a new tape without bumping it, fold in `getOverviewEpoch (slot)` and record the deviation. Gate: SR rows (PD11).
- **PD7 [R§10.6] Telemetry accessors, not an aggregate.** Narrow message-thread getters on the processor wrapping existing lock-free readers; **3.1 adds only what 3.1 reads** (`getSidechainChannelCount` exists). Consuming feeds (`takeOutPeak`, `takeScPeak`, `readUiFrame`) are **not** added or read in 3.1 (R§3: do not half-read); 3.2 / 3.3 add them.
- **PD8 `getTelemetry` 3.1 shape (R§3, exact).** Always the full object, never a partial (RS4):
  `{ scaleRev, slotRev, lanes: [ { ovRev, fb: null, rec: null } ×4 ] (no lo / hi), voices: [], newest: -1, menv: [], grains: [], counts: [0,0,0,0], limit: 0, out: [0,0], rec: { bus: 0|1, signal: false, run: 0, slot: 0, pos: 0, len: 0, capped: false, peak: [0,0] }, sources: [], modNorm: {} }`; `lfoPh`, `frozen`, `modRouted` omitted. `rec.bus = getSidechainChannelCount() > 0 ? 1 : 0`.
- **PD9 Window curve = real.** Port the parent: `Engine::windowCurve (float window, float skewRaw, float* out, int n) const noexcept` over the immutable morph bank (`stepFor` / `table` / `read`), and `getWindowCurve (int stream0, float* out, int n) const` reading the BASE `sN_window` / `sN_window_skew`. `Engine.h` is not a forked file; nothing under `forked/` changes.
- **PD10 One JSON encoder, shared with the harness.** New `Source/GiUiJson.h` (`juce_core` + processor types only, no WebView types): `scaleStateToVar`, `slotStateToVar` (SlotInfo → page fields per R§2: `scheme` from the `source` prefix, `seconds = frames / tapeRate` when loaded, `encodedBytes` = 0 or 16 MiB + 1 when `tooLarge`, `dirty` false, `embeddable`), `telemetryToVar` (PD8), `parameterDefaultsToVar`. The editor and `--dump-ui-overrides` both call these, so the stub fixture is produced by the code the page talks to.
- **PD11 Harness rows `tests/render-harness/ui_gates_31.h`** (processor only; the harness never compiles the editor). Names prefixed to avoid collisions with existing rows (check with grep before naming):
  - **PM1** preset round trip (`savePresetToFile` / `loadPresetFromFile`, temp dir, fresh instance): every parameter except `output_gain` / `rec_arm`, custom scale, 3 non-default masks, seed — equal; scale revision moved.
  - **PM2** `output_gain` −12 dB survives a load; no `output_gain` and no `rec_arm` key in the file. **PM2n** exclusion off → lost (must FAIL).
  - **PM3** preset saved with `rec_arm` On, loaded on an instance fed a live sidechain → `recArms` counter stays 0, `rec_arm` unchanged. **PM3n** `rec_arm` not excluded → `recArms` ≥ 1 (must FAIL; proves the stimulus arms).
  - **PM4** two reopens: preset load → get state → fresh set state → change a mask → get state → fresh set state == the changed state; no `CustomState` child in the live tree.
  - **PM5** seed `0xFEDCBA9876543211` exact through a preset (string). **PM5n** written as a JSON number → mismatch.
  - **PM6** slots: load a file into slot 1 and a `factory:` stub into slot 3, save, clear, load the preset → both sources restored (sync hook), slot 2 / 4 empty; the preset JSON contains no `embedded` key and is < 64 KB. **PM6b** preset with no `slots` key leaves loaded slots as they are.
  - **WC1** `windowCurve (0, 0, …, 64)` == closed-form Hann within 1e-6. **WC2** skew ±100 and window 1: finite, in [0, 1], endpoints 0, differ from WC1.
  - **SR1** slot revision moves on load, relink, reset, embed toggle, missing restore, preset apply; stable over 100 idle polls. **SR2** `ovRev` moves on each tape-changing event of SR1 and on a record commit (`commitSync`). **SR1n** fingerprint without `generation` → a same-source reload is missed (must FAIL).
  - **OV1** overview of the tone stub: 656 peaks, max within 1 % of the stub's −12 dBFS peak, raised-cosine ends < 0.05; empty slot → false + zeros.
  - **TL31** `telemetryToVar` at defaults: exact key set of PD8; `lanes[i].fb` / `.rec` are void / null, no `lo` / `hi`, `counts` 4 numbers, `rec.bus` 1 with a sidechain bus and 0 on the gate variant layout.
  - Expected harness total **357 + new rows**; the executor records the exact number.
- **PD12 `--dump-ui-overrides <path>`** (harness, early exit). Writes `tests/ui-stub/generic-overrides.json`: `_why` + regeneration command; `combos` (70 × `{ choices, def }`); `sliders` (176 × `start / end / def / skew / interval / label`); `natives`: `getScaleState`, `getSlotState` (a loaded fixture: slot 1 factory tone, slot 2 a `file:` source with `trimmed`, slot 3 missing, slot 4 empty), `getTelemetry` (PD8 with `slotRev` / `scaleRev` matching), `getParameterDefaults`, `getWindowCurve` (64 points), `getSlotOverview` (656 peaks of the tone stub), `getUiLanguage`. Written with `"\n"` line ends; generated twice → byte-identical; no clock, no absolute path in the output. Committed; every gate run regenerates it and `git diff --exit-code` must be clean.
- **PD13 Editor = the template adapted (R§2).** Deltas, all recorded in the file header:
  - slots 1…4 → 0…3 once, in `slotArg`;
  - `getSlotState`, `getScaleState`, `getTelemetry`, `getParameterDefaults` through `GiUiJson.h`;
  - `loadSlot` (chooser → `requestSlotLoad`) and `relinkSlot` answer `{ ok: true }`; errors arrive through `lastError` on the next state pull;
  - `resetSlot` → `requestReset (slot)`, `resetAll` → `requestReset (-1)`;
  - `setSlotEmbed`: `"factory"` / `"empty"` derived from `SlotInfo::source` before the call;
  - `clearSlot`, `saveRecording` → `{ ok: false, error: "unavailable" }` (D37); the `notRecording` guard stays;
  - `onCommitFolder` gets the module's 8-parameter signature (`WebViewDropStreaming.h:132-139`); `onCommitFile` → `requestSlotLoad`; `tempDirPrefix = "ogranulatorinstrument-drop-"`;
  - every native calls `complete` on every path; every `launchAsync` callback hoists a `SafePointer`;
  - resource provider keeps the explicit `if (url == "/x")` ladder with `UIBinaryData::sym` adjacent (serve-ui parses it); bare paths; `Juce` namespace passed;
  - Windows block verbatim (`withBackend (webview2)`, `withUserDataFolder`, options probe, null `webView` + notice); never `setVisible (false)`;
  - member order relays (3 vectors) → `dropSessions` → `webView` → attachments (3 vectors) → `webView2RuntimeLink`; constructor in the same order;
  - `setSize (1200, 800)`, `setResizable (false, false)`;
  - no `juce::Timer`, no `emitEvent`.
- **PD14 CMake = three edits to the real file (RS1).** Uncomment the UIResources block + its link (10 sources; the block must be genuinely active, not duplicated); remove `Source/PluginEditorDevLoader.cpp`; remove the `OGI_DEV_LOADER` option + definition. Nothing else; no comment spelling the plugin-target call with its parenthesis. `PluginProcessor.h` stays free of WebView types (consoles build with `JUCE_WEB_BROWSER=0`).
- **PD15 `createEditor` switches in the same task as the editor lands (RS12):** `#if JUCE_WEB_BROWSER` → `new GranulatorInstrumentEditor (*this)`, else the generic editor. DevLoader files, include and comments removed in that task (D38).
- **PD16 [R§10.4] Value entry (D41), ported to BOTH `Source/ui/public/index.html` and `mockups/v1-ui.html`** (they stay byte-identical; `cmp` is an entry check for 3.2 / 3.3). Per R§6:
  - verbatim hunks: `.knob-label` CSS (drop `pointer-events: none`, `cursor: text`, hover / editing colour), `.knob-value-input`, the `updateKnobVisual` guard, `numOf` / `kMul` / `PARSERS`, `parseDivision`, `attachValueEntry`, `bindLabelEntry`, the init calls;
  - new: guard **both** write branches of `updateChoiceKnobVisual`; `fCount` (`polyphony`) and `fBend` (`pitch_bend_range`) parsers, rounding to the integer in the parser; **scan mode = localized match** — the three faces of the current language plus the three English choices, case-insensitive, trimmed;
  - close any open field before a language apply and when a Sync twin cell is hidden;
  - capture-phase `pointerdown` blur commit;
  - write path unchanged: clamp → `sliderDragStarted` → `setNormalisedValue` → `sliderDragEnded` (one gesture); a choice writes only when the index differs; unchanged text writes nothing;
  - covers 144 + 17 = **161** knobs; root fields, loop handles, depth bars excluded. No i18n key added, no C++.
- **PD17 State sweep (D42).** `tests/i18n-states.json` = the 16 template states + **3** entry states: a field open in the narrowest cell (54 px header cell) in en, the same in fr, and a scan-mode knob open in zh-Hans. Each click state confirmed once with `elementFromPoint (centre) === target`; `eval` clicks where a parked pointer would raise a tooltip. Runner banner must read `states: default + 19`.
- **PD18 Static closure `tests/ui_frontend_check.js`** (copy of the parent's, adapted): fresh `O-GranulatorInstrument-param-dump` == editor `kSliderIds / kComboIds / kToggleIds` == page id tables by ID **and** type, both directions, no duplicates (176 / 70 / 15); 34 page + preset-manager.js + webview-drop-streaming.js native names == 34 `withNativeFunction`; member / constructor order; `node --check` on the inline module; `Juce` namespace; no viewport units; every binary-data SOURCE ↔ one resource branch; `Source/ui/public/modules/*` untracked; telemetry keys `telemetryToVar` emits ⊆ keys the page reads; `withBackend` + `withUserDataFolder` present; no `DevLoader` / `OGI_DEV_LOADER` token left under `Source/` or `CMakeLists.txt` (grep excludes `ui/public/modules`). **NCs** on scratch copies: one id moved slider → toggle FAILS; one `withNativeFunction` removed FAILS.
- **PD19 Binding probe `tests/ui_binding_probe.js`** (copy of the parent's, adapted): headless Chromium through `scripts/serve-ui.js` with PD12's overrides; refuses to run without the overrides banner; the stub answers the ten preset functions. Per id, host → UI and UI → host, switching tab first; binders covered: `bindKnob`, `bindChoiceKnob`, `bindFader`, `bindHandle`, `bindField`, segment rows, `<select>`s, the `rec_target` radio, scale triggers, toggles. **Liveness: 261 assertions each direction.** Value-entry rows (**VE1–VE12**): Enter applies; Esc cancels; outside click applies; outside press on a choice knob (the `preventDefault` case) applies; unchanged text writes nothing; exactly one gesture pair per value; clamp above `end`; `-inf` on a gain; `fCount`; `fBend` with and without `±` / `st`; a division; a scan mode typed in French with the page in fr; language switch with a field open closes it. **NC:** a page copy with one `bindKnob` removed FAILS (260).
- **PD20 Gate order (T14):** regenerate overrides (diff-clean) → `ui_frontend_check.js` → `ui_binding_probe.js` → check-i18n → fr-lint → zh-lint → check-ui-labels (banner `seed=param-dump (261 rows) + overrides`, `states: default + 19`) → boot-all-uis `--strict-tips`. Exit 77 anywhere = not run = stop, install Playwright, re-run.
- **PD21 Design latitude (D40).** v1 locked. A page change other than PD16 is allowed only for breakage a gate or the smoke shot finds; each one is a recorded deviation applied to both `index.html` and `v1-ui.html`. `.knob-label` becoming non-inert widens the check-ui-labels collision set (RS10): a new collision there is a D40 defect to fix (spacing only), not a reason to drop the gate.
- **PD22 Housekeeping.** Version **0.1.0**; **no tag**. No new params; param-dump byte-identical. No edit to `forked/*`, `modules/*`, `scripts/*`, contracts (except REQUIREMENTS status cells if a row changes — none expected in 3.1). Four path-scoped commits C1–C4 on `main`; re-check `git branch --show-current` + `git status --short` immediately before each; never `git add -A`; foreign dirty paths (`modules/registry.yaml`, `.claude/*`, `.planning/STATE.md`, `plugins/O-GrainScatter/*`) never staged; `Source/ui/public/modules/` never committed. Standalone rebuilt explicitly before any Standalone check.

---

## Source layout (deltas)

| File | Change |
|---|---|
| `Source/PluginProcessor.h/.cpp` | preset manager + callbacks (PD1, PD2); `scaleRevision` (PD4); `getSlotStateRevision` (PD3); `getSlotOverview` + `getOverviewRev` (PD5, PD6); `getWindowCurve` (PD9); `createEditor` (PD15); DevLoader include removed |
| `Source/dsp/Engine.h` (`.cpp`) | `windowCurve (…) const noexcept` (PD9) — never on the audio path |
| `Source/GiUiJson.h` (new) | shared encoders (PD10) |
| `Source/PluginEditor.h` (new), `PluginEditor.cpp` | `GranulatorInstrumentEditor` (PD13) |
| `Source/PluginEditorDevLoader.{h,cpp}` | deleted (D38) |
| `Source/ui/public/index.html`, `js/i18n.js`, `js/juce/{index.js,check_native_interop.js}` (new) | copied (T7); `index.html` then gains PD16 |
| `mockups/v1-ui.html` | PD16, kept == `index.html` |
| `CMakeLists.txt` | PD14 (three edits) |
| `tests/render-harness/ui_gates_31.h` (new), `main.cpp` | PD11 rows, `--dump-ui-overrides` (PD12) |
| `tests/ui-stub/generic-overrides.json` (new, generated) | PD12 |
| `tests/i18n-states.json` (new) | PD17 |
| `tests/ui_frontend_check.js`, `tests/ui_binding_probe.js` (new) | PD18, PD19 |
| `mockups/v1-integration-checklist.md` | §3 corrected (RS1); §1 gear item closed (D40) |

---

## Tasks

**Roles.** **[agent]** = gui-agent writes files. **[run]** = builds, gates, validators, commits. **BG** = `run_in_background`, log to `$S/<name>.log` (`$S` = the executing session's scratchpad); anything over ~5 min runs BG (600 s watchdog). `$H` = `build/plugins/O-GranulatorInstrument/O-GranulatorInstrument-render-harness_artefacts/Release/O-GranulatorInstrument-render-harness`. Paths relative to `plugins/O-GranulatorInstrument/` unless they start with `build/`, `scripts/`, `modules/` or `~`.

**Audio-neutral checkpoint (after T2, T3, T4 and T8).** Build the harness (BG); `$H --dump-render-hashes "$S/ck-<T>.sha256"` == `tests/golden/renders-2.4.sha256`; `--dump-render-hashes-legacy` == `renders-2.2.sha256`; param-dump == `tests/golden/params.tsv`. A move → STOP, attribute to that task. Renders are never re-pinned.

### Wave 0 — pre-flight

1. **T1 — pre-flight + baselines** [run]. Output `$S/preflight.txt`; any mismatch → STOP and ask.
   - Contracts re-hashed against STATUS frontmatter; `git log --oneline e4987f8a..HEAD -- plugins/O-GranulatorInstrument/Source plugins/O-GranulatorInstrument/tests plugins/O-GranulatorInstrument/CMakeLists.txt` empty; branch `main`; record `git status --short`.
   - Baselines on the unmodified tree: harness **357 / 0** (BG; carried extract = sorted `[PASS]/[FAIL]` rows, timing detail dropped = **B**), parity 168 / 0 + GV 2 / 0, render hashes, param-dump, fork drift 0 / 13.
   - UI-gate pre-state (five gates; expected: check-ui-labels / boot fail on the placeholder editor's missing `setSize` — record, do not fix).
   - `node -e "require('playwright')"` resolves (else install now).
   - Depends on: none

### Wave 1 — processor surface (audio-neutral)

2. **T2 — preset manager + slots in presets** (PD1, PD2) [agent]. Checkpoint.
   - Files: `Source/PluginProcessor.h/.cpp`
   - Depends on: T1
3. **T3 — revisions + overview** (PD3, PD4, PD5, PD6) [agent]. Checkpoint.
   - Files: `Source/PluginProcessor.h/.cpp`
   - Depends on: T1
4. **T4 — window curve + `GiUiJson.h`** (PD7, PD8, PD9, PD10) [agent]. Checkpoint.
   - Files: `Source/dsp/Engine.h` (`.cpp`), `Source/PluginProcessor.h/.cpp`, `Source/GiUiJson.h`
   - Depends on: T2, T3 (same files; serial)
5. **T5 — harness rows + overrides dump** (PD11, PD12) [agent + run]. `ui_gates_31.h`, `--dump-ui-overrides`; generate `tests/ui-stub/generic-overrides.json` twice, byte-identical.
   - Files: `tests/render-harness/ui_gates_31.h`, `main.cpp`, `tests/ui-stub/generic-overrides.json`
   - Depends on: T4
6. **T6 — neutral proof + commit C1** [run]. Harness full set (BG): carried extract diff vs **B** empty; new rows PASS; PM2n / PM3n / PM5n / SR1n each FAIL their predicate; parity 168 / 0 + GV 2 / 0; renders + legacy byte-identical; param-dump identical; drift 0 / 13. Commit path-scoped.
   - Depends on: T5

### Wave 2 — editor

7. **T7 — UI files + CMake** (PD14, checklist §1) [agent + run]. Copy `v1-ui.html` → `Source/ui/public/index.html`, `v1-i18n.js` → `js/i18n.js`, JUCE frontend from `git show HEAD:plugins/O-Granulator/Source/ui/public/js/juce/{index.js,check_native_interop.js}`; `cmp` each against its source. Three CMake edits. Do not copy module JS / fonts.
   - Files: `Source/ui/public/**`, `CMakeLists.txt`
   - Depends on: T6
8. **T8 — `GranulatorInstrumentEditor` + DevLoader removal** (PD13, PD15, D38) [agent + run]. One task with T7's CMake (the tree does not build between them). Reconfigure; build VST3 + AU + Standalone + both consoles, **0 warnings**. Checkpoint (consoles rebuilt against the new processor).
   - Files: `Source/PluginEditor.h/.cpp`, `Source/PluginProcessor.cpp`, delete `Source/PluginEditorDevLoader.{h,cpp}`
   - Depends on: T7
9. **T9 — install + validate + commit C2** [run]. `./scripts/build-and-install.sh O-GranulatorInstrument` (cache clear, dual-variant sweep); `auval` PASS; pluginval 10 VST3 + AU with the sidechain on, then the `OGI_GATE_SC_DEFAULT_OFF` variant (procedure as PLAN-2.6 W7), shipping build restored + reinstalled after; X2 (test-hook symbols in shipping binaries) = 0; Standalone rebuilt, launch smoke: window 1200 × 800, not blank, one `screencapture` per page (4) into `$S`, read each image for clipping / blank regions; editor close / reopen ×10 via pluginval's editor pass. Commit.
   - Depends on: T8

### Wave 3 — UI proof

10. **T10 — gate baseline on the unmodified page** [run]. With PD12's overrides and the 16-state file copied to `tests/i18n-states.json`: check-i18n, fr-lint, zh-lint, check-ui-labels, boot-all-uis `--strict-tips`. Record results as the pre-port baseline (RS10). A failure here is a v1 defect (PD21), fixed before T11.
    - Files: `tests/i18n-states.json`
    - Depends on: T9
11. **T11 — value entry port** (PD16) [agent]. `index.html` + `v1-ui.html`, `cmp` clean after.
    - Files: `Source/ui/public/index.html`, `.planning/mockups/v1-ui.html`
    - Depends on: T10
12. **T12 — entry states** (PD17) [agent + run]. 16 → 19 states.
    - Files: `tests/i18n-states.json`
    - Depends on: T11
13. **T13 — closure + probe scripts** (PD18, PD19) [agent + run]. Both scripts with their NCs.
    - Files: `tests/ui_frontend_check.js`, `tests/ui_binding_probe.js`
    - Depends on: T11 (probe needs the entry code); closure may start after T8
14. **T14 — full gate run + commit C3** (PD20) [run]. All green with exit codes recorded; any new collision / fit failure against T10's baseline fixed under PD21 and the whole sequence re-run. Rebuild + reinstall (page changed), Standalone smoke shot of one open field. Commit.
    - Depends on: T12, T13

### Wave 4 — regression + docs

15. **T15 — Stage 2 regression on the final tree** [run]. Harness full set (BG) vs **B** + new rows; parity; renders + legacy; param-dump; drift 0 / 13; X2 0; auval; pluginval 10 VST3 + AU, sidechain on and off; final install of the shipping build; `auval -a | grep -i granulatorinstrument`.
    - Depends on: T14
16. **T16 — docs + commit C4** [agent + run].
    - `HANDS-ON-3.1.md` (3.1 rows only, run by Taylor at 3.1 verify): Standalone + one DAW; one control of each kind per page; one automation lane per relay type; preset save / load incl. scale, seed, slot sources; `output_gain` and `rec_arm` survive a load; two reopens; load / relink / reset / embed from the UI with the waveform appearing; value entry by hand (Enter / Esc / click-away, a division, a scan mode in fr); the gear at (1124, 62) beside the source dock (D40); fr / zh-Hans no clipping; Reduce Motion; Inspector console clean, no 404; EB Garamond on canvas; expected 3.1 placeholders listed so verify does not flag them (VOICE "idle", MOD without dots, Record-in "no signal" / "No sidechain", no record head / playheads / grain dots, Clear and Save refuse).
    - `SUMMARY-3.1.md` (complex template): rows, counts, NC results, deviations (incl. the PD1 sign-off vs `parameter-spec.md:355`), timings.
    - Checklist §1 / §3 amendments.
    - STATUS.md → `stage_3_phase_3_1_execute_complete`, next `/plugin-verify O-GranulatorInstrument 3-gui`.
    - Depends on: T15

---

## Dependency graph

```
T1 ─┬─ T2 ─┐
    └─ T3 ─┴─ T4 ── T5 ── T6 ── T7 ── T8 ── T9 ── T10 ── T11 ─┬─ T12 ─┐
                                                              └─ T13 ─┴─ T14 ── T15 ── T16
```

---

## Gate criteria (3.1)

| Gate | Pass |
|---|---|
| Harness | carried extract diff vs B **empty**; new `ui_gates_31` rows PASS; PM2n / PM3n / PM5n / SR1n FAIL their predicate; 0 NOT FIRED |
| Parity | 168 / 0 + GV 2 / 0 |
| Renders | renders-2.4 + legacy byte-identical |
| param-dump | identical to `tests/golden/params.tsv` (261) |
| Fork drift | 0 / 13 |
| Build | VST3 + AU + Standalone + consoles, 0 warnings |
| auval; pluginval 10 VST3 + AU | PASS / SUCCESS, sidechain on and off |
| X2 | 0 test-hook symbols in shipping binaries |
| `generic-overrides.json` | twice byte-identical; diff-clean vs committed |
| `ui_frontend_check.js` | 0 failures; 176 / 70 / 15; 34 = 34; both NCs FAIL |
| `ui_binding_probe.js` | 261 / 261 host → UI, 261 / 261 UI → host; VE1–VE12 PASS; NC FAILS at 260 |
| check-i18n · fr-lint · zh-lint | 0 · 0 · 0 |
| check-ui-labels | 0; banner `(261 rows) + overrides`, `states: default + 19` |
| boot-all-uis `--strict-tips` | completes, 0 DEAD, 0 late |
| `cmp index.html v1-ui.html` | identical |

Exit 77 anywhere = not run = not a pass.

---

## Risks / watch items

- **Preset apply touching slots (PD1)** enqueues loader jobs from the preset callback. It must stay on the message thread and never decode inline; PM6 uses the sync hook only in the harness.
- **`rec_arm` edge on preset load (RS7).** PM3n must actually arm, or PM3 is vacuous (stimulus below threshold).
- **`ovRev` (PD6).** If it does not move after a load the waveform never appears (R§3); SR2 + the T9 smoke shot of a loaded lane are the two proofs.
- **A native that never completes** hangs its promise with no timeout (R§3); `getSlotOverview` stuck = that lane's fetch dead forever. PD18 checks registration; the probe's boot waits on first telemetry + slot state + overview.
- **Stub probe ≠ real WebView.** JUCE relay ↔ attachment plumbing is proven only by pluginval's editor pass and HANDS-ON-3.1.
- **check-ui-labels after the port (RS10).** 161 labels join the collision set; compare against T10, never loosen the gate.
- **Integer knobs.** Whether the frontend snaps `setNormalisedValue` to the interval is unverified (R§8); VE rows for `fCount` / `fBend` assert the exact stub value.
- **Windows block** is compile-only here (Stage 4 COMPAT); kept verbatim, presence grepped in PD18.
- **No parameter writes from listeners**; preset apply resets through `setValueNotifyingHost` on the message thread only.

## Deviations protocol

Record every departure in SUMMARY-3.1 (what, why, evidence, which gate). Stop and ask only for: an audio-moving change, a contract edit, a module or `scripts/` change, a forked-file edit, or a v1 design change beyond a defect (D40).

## Carried (not 3.1)

3.2: live overview + `dirty[]`, record head, playheads, grain dots, drop UI checks, `rec.run` mapping (R§3) and the SC peak choice, `SlotLoader::requestClear`, `saveRecordingTo`, exact `encodedBytes`, consuming peak accessors. 3.3: voices, sources, `modNorm`, meters, the 132 unmeasured `[data-i18n]` nodes' states, HANDS-ON-2.1–2.6 + Logic DAW pass (D39). Stage 4: blobs in presets (PD1 revisit), factory presets, Windows.

## Success criteria (gate for verify)

- [ ] WebView editor loads in Standalone / AU / VST3 at a fixed 1200 × 800; all four pages render; auval + pluginval 10 PASS with the sidechain on and off
- [ ] 261 relays / attachments match APVTS by ID and type (closure, NCs fail); page binds all 261 both ways under the C++-seeded stub (probe, NC fails)
- [ ] 34 = 34 native functions; every one completes on every path; `clearSlot` / `saveRecording` refuse cleanly
- [ ] Presets: round trip incl. scale, masks, seed, slot sources (no blobs); `output_gain` and `rec_arm` untouched; two-reopen clean
- [ ] Slot actions live from the UI: load, relink, reset, reset all, embed; lane shows name, badges and the pristine waveform
- [ ] `getTelemetry` emits the exact 3.1 shape; `getWindowCurve` is the engine's curve
- [ ] Value entry on all 161 knobs (VE rows green); `index.html` == `v1-ui.html`
- [ ] DevLoader and `OGI_DEV_LOADER` gone
- [ ] All five UI gates green with overrides + 19 states
- [ ] Stage 2 unchanged: harness carried diff empty, parity, renders, param-dump, drift, X2
- [ ] HANDS-ON-3.1.md ready for verify
