# Stage 4: Polish — Execution Plan

**Date:** 2026-08-15
**Inputs:** CONTEXT.md, RESEARCH.md (both stages/4-polish/)
**Executor:** stage-4 polish agent (single context — all tasks touch the same three TUs + UI)

## Goal

Wire the shared preset-manager module v1.0.5 into O-Tapestop (preset band, save/load/browse/delete, 8-preset factory bank with the scratch-envelope blob riding `customState`), initialize CHANGELOG.md, and pass the automated validation gate (harness 47/47, pluginval ×2–3 VST3+AU, auval, parity checks) ending in a local install. Session-state format stays byte-identical; Stage-2 DSP stays frozen.

## Non-Goals (locked in CONTEXT.md)

- No `/publish`, no `/package` — local install only.
- Manual DAW checklist items from stages 2/3 remain open, NOT gating.
- No `getStateAsXml`/`setStateFromXml` adoption — session format untouched.
- `currentPresetName` session persistence — **skipped** (research default).
- Windows CI — deferred to a future publish cycle.

## Tasks

### 1. [ ] Module dependency wiring
- Run `/module-add O-Tapestop preset-manager` → `ouaricon_add_module(OuariconTapestop preset-manager)` in CMakeLists (the `include(OuariconModules.cmake)` line already exists at `CMakeLists.txt:3`), registry updated.
- **Include path, not vendored copy** (O-AnalogEQ reference, `CMakeLists.txt:55-56`).
- Relocate the module-copied JS from `Source/ui/public/modules/preset-manager.js` → `Source/ui/js/modules/preset-manager.js` (O-Tapestop's UI root has no `public/`); delete the `public/` stray.
- Add the JS to the **existing** `juce_add_binary_data(OuariconTapestop_UIResources NAMESPACE UIBinaryData ...)` target (`CMakeLists.txt:62-78`) — no second binary-data target. Hyphen-stripped symbol: `UIBinaryData::presetmanager_js`.
- Regenerate registry `used_by` via `scripts/regen-registry-used-by.sh`.
- Files: `CMakeLists.txt`, `Source/ui/js/modules/preset-manager.js` (moved), module registry.
- Depends on: none.

### 2. [ ] Processor: manager member + custom-state callbacks
- Member `OuariconPresetManager presetManager { parameters, "Ouaricon Tapestop" };` — **literal** name so dev/release builds share one preset library (`~/Library/Ouaricon Tapestop/Presets/`).
- `setCustomStateCallbacks(save, load)`:
  - Save: `DynamicObject` with single property `"scratchEnvelope"` = `scratchEnvelope.toJson()` string (same string `getStateInformation` persists, `PluginProcessor.cpp:535-544`). Blob stays opaque/self-versioned.
  - Load: defensive ladder (null obj → return; missing/non-string property → leave envelope alone), then `commitScratchEnvelopeJson()` (`PluginProcessor.h:95-100`) — does the message-thread bake + atomic publish + bumps `uiEnvGeneration`, so the 30 Hz editor timer pushes the sanitized echo to the WebView with zero new plumbing. Shape reference: O-SpectralShaper `PluginProcessor.cpp:122-165`.
- `initializeFactoryPresets(...)` once in the constructor (v1.0.5 sentinel-gated — scan-storm safe).
- **Do NOT touch `getStateInformation`/`setStateInformation`.** No AsyncUpdater exists here; no `cancelPendingUpdate()` needed (grep-confirmed in research).
- Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`.
- Depends on: Task 1.

### 3. [ ] Factory bank: 8 presets
- Authoring: **engineering units + batch `convertTo0to1` loop** (CR-02 pattern, O-SpectralShaper `PluginProcessor.cpp:302-310`). Never raw fractions — `STOP_FREE_MS`/`START_FREE_MS`/`ENV_FREE_MS` are skewed. Choice params authored as index (`MODE`, `SYNC_MODE`, `STOP_SYNC_DIV`, `START_SYNC_DIV`, `ENV_SYNC_DIV`).
- Every preset lists **all 14 param IDs** and carries an **explicit `customState` envelope blob — including Stop-mode presets** (`applyPresetJson` only invokes the load callback when `customState` exists; a preset without it inherits the live envelope).
- Bank (names finalized in code; coverage matrix: both MODEs, both SYNC_MODEs, curve extremes, all three time params exercised):
  1. **Classic ½-Bar Stop** — Stop, Sync, STOP_SYNC_DIV=1/2 bar, curves 50% (tape-physics default)
  2. **Classic 1-Bar Stop** — Stop, Sync, 1 bar, STOP_CURVE high (exponential sag)
  3. **DJ Spinup** — Stop, Sync, fast start (1/4 bar), START_CURVE low, MIX 100
  4. **Scratch Gesture: Baby Scratch** — Scratch, Sync, ENV_SYNC_DIV=1/4 bar, bipolar wobble envelope
  5. **Scratch Gesture: Chirp Flare** — Scratch, Sync, 1/2 bar, sharper multi-segment envelope w/ reverse dips
  6. **Tempo-Synced Short Stop** — Stop, Sync, STOP_SYNC_DIV=1/8, snappy curves
  7. **Slow-Tape Drag** — Stop, Free, STOP_FREE_MS long (~4000 ms), heavy TONE_TRACK (~90%)
  8. **Stutter-Scratch** — Scratch, Sync, short env (1/16 or 1/8), aggressive square-ish envelope
- Envelope blobs authored as JSON strings matching the versioned `scratchEnvelope.toJson()` schema; Stop-mode presets carry the default gentle-wobble blob.
- Files: `Source/PluginProcessor.cpp` (or a `Source/FactoryPresets.h` if the table exceeds ~150 lines — executor's call, single TU either way).
- Depends on: Task 2.

### 4. [ ] Editor: 10 new native functions (13 total)
- Register all 10 names `preset-manager.js:108-117` requests: `savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset` — alongside existing `getParameterDefaults`, `commitEnvelope`, `requestEnvelope`.
- `savePresetWithDialog` (undocumented but required): `FileChooser` saveMode into `getUserPresetsDirectory()`, write via `savePresetToFile()` (honors chosen path). **Both dialog fns return `{success, name}` objects, not bools.**
- FileChooser shape copied from O-Polystutter `PluginEditor.cpp:416-497`: `std::shared_ptr<FileChooser>` captured into its own callback; `SafePointer` **hoisted to a local** (MSVC); on dead editor **`return` — never `complete(false)`** (UAF).
- Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp` (`:151-205` native-fn block).
- Depends on: Task 2.

### 5. [ ] WebView JS wiring + band enable
- Add `/js/modules/preset-manager.js` route to `TapestopEditor::getResource` (`PluginEditor.cpp:77-116`) serving `UIBinaryData::presetmanager_js` — provider matches bare paths.
- `app.js`: `import { PresetManager } from "./js/modules/preset-manager.js"`; construct with **explicit DOM refs for all six elements** (`preset-prev`, `preset-next`, `preset-name`, `preset-save`, `preset-load`, `preset-delete`) and `getNativeFunction: Juce.getNativeFunction` (omitting it → silently inert bar). **Do NOT use `createPresetBar()`** (innerHTML-wipes the styled band, no delete button).
- `onConfirmDelete`: two-click `data-armed="1"` pattern the CSS anticipates (`styles.css:226-231`) — not `window.confirm`.
- `initialize()` called from `app.js` **after** the existing bridge setup (view visible → completions not dropped); not at module-eval time.
- Un-disable the six band buttons in `index.html:56-66`; remove the `.preset-name` opacity-0.65 dim per the inline stage-4 note. Frame stays 860×580 — zero layout work.
- Keep `#preset-name` childless (`_updateDisplay()` writes `textContent`).
- Files: `Source/ui/index.html`, `Source/ui/js/app.js`, `Source/ui/styles.css` (dim removal only), `Source/PluginEditor.cpp` (resource route).
- Depends on: Tasks 1, 4.

### 6. [ ] CHANGELOG.md
- Create `plugins/O-Tapestop/CHANGELOG.md` with a `1.0.0` entry (dated at verify): initial release — stop/start engine, scratch mode, preset system, factory bank. Version in CMakeLists stays 0.1.0 until verify.
- Files: `CHANGELOG.md`.
- Depends on: none (content references Tasks 1–5).

### 7. [ ] Native-fn + event parity gate
- Re-run the stage-3 grep-diff (VERIFICATION.md:58 method) **both directions at 13↔13**: every `getNativeFunction` name in JS has a `withNativeFunction` in C++ and vice versa. Event-name parity unchanged: `transportFrame`, `envelopeState`.
- Depends on: Tasks 4, 5.

### 8. [ ] Render harness re-run — 47/47
- Rebuild with `-DOUARICON_BUILD_TESTS=ON`, re-run `tests/render-harness`. **Mandatory** — the processor TU gains the manager member + callbacks. Header-only module (`juce_core` + `juce_audio_processors`, no WebView types) → harness-safe; confirm `JucePlugin_VersionString` is defined in the harness TU.
- Gate: **47/47 or stage 4 does not pass.** Any failure → fix without touching frozen Stage-2 DSP semantics.
- Depends on: Tasks 2, 3.

### 9. [ ] Regression probes: bit-transparency + state round-trip
- Re-run the stage-1 memcmp bit-transparency probe (512 + 4096) and the state round-trip probe. Session format untouched → must pass **byte-identical**.
- Depends on: Task 8.

### 10. [ ] Plugin validation + install
- Build both formats; `./scripts/build-and-install.sh O-Tapestop` (Phase 4 dual-variant sweep).
- `pluginval` strictness 10, VST3 **and** AU, each run **2–3×** (latent-NaN pattern).
- `auval -v aufx OTsp <mfr>` after install.
- Smoke the preset band in Standalone (`/show-standalone` optional): save → load → prev/next → delete-armed → factory preset applies envelope (name display + envelope redraw).
- Depends on: Tasks 7, 8, 9.

### 11. [ ] Version bump — deferred to verify
- `VERSION 0.1.0` → `1.0.0` in CMakeLists happens **at stage-4 verify**, not execute (regenerates the factory sentinel + preset version stamps automatically). Execute phase leaves 0.1.0. CHANGELOG date finalized then too.
- Depends on: verify phase.

## Task Order

1 → 2 → 3 → 4 → 5 → 6 (anytime) → 7 → 8 → 9 → 10. (11 at verify.)

## Success Criteria

- [ ] Preset band live in the WebView: save, save-as (dialog), load, load-from-file, prev/next, two-click delete all function; factory presets listed and marked
- [ ] Factory bank: 8 presets, all 14 params authored in engineering units → `convertTo0to1`, every preset (incl. Stop-mode) carries a `customState` envelope blob
- [ ] Loading any preset resets params to defaults first (WR-01) and applies its envelope; envelope echo reaches the UI via the existing generation counter
- [ ] Native-fn parity 13↔13 both directions; event parity unchanged
- [ ] Render harness 47/47
- [ ] Bit-transparency memcmp (512+4096) and state round-trip byte-identical
- [ ] pluginval strictness 10 VST3+AU pass ×2–3 each; auval passes
- [ ] Session save/load from stages 1–3 sessions unaffected (state format untouched)
- [ ] CHANGELOG.md exists; installed locally via build-and-install.sh
- [ ] No edits to Stage-2 DSP semantics; no `getStateAsXml` adoption; no second binary-data target

## Pitfall Checklist (from RESEARCH.md — executor MUST read)

1. Constructor + explicit DOM refs, never `createPresetBar()`
2. `savePresetWithDialog` implemented; dialog fns return `{success, name}`
3. FileChooser: shared_ptr + hoisted SafePointer + `return` on dead editor
4. Every factory preset carries `customState`
5. `convertTo0to1` batch loop, never raw fractions
6. JS relocated to `ui/js/modules/`; symbol `presetmanager_js`; existing binary-data target
7. Session state untouched
8. Parity gate at 13
9. Harness re-run mandatory
10. Delete via `onConfirmDelete` armed pattern
11. `initialize()` from app.js post-bridge, not module-eval time
