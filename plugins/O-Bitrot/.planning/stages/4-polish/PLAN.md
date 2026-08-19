# Stage 4: Polish — Execution Plan

**Date:** 2026-08-16
**Inputs:** CONTEXT.md, RESEARCH.md (both stages/4-polish/)
**Executor:** stage-4 polish agent (single context — all tasks touch the same three TUs + UI)

## Goal

Wire the shared preset-manager module v1.0.5 into O-Bitrot (header-center preset band, save/load/browse/delete, 8-preset factory bank covering all 6 degradation families), initialize CHANGELOG.md, and pass the automated validation gate (harness 44/44, pluginval s10 ×2–3 VST3+AU, auval, native-fn parity 10↔10) ending in a local install. Stage-2 DSP stays frozen; session-state format stays byte-identical; frame stays 900×620 with zero grid/global geometry change.

## Non-Goals (locked in CONTEXT.md)

- No `/publish`, no `/package` — local install only.
- Manual DAW checklist items from stages 2/3 remain open, NOT gating.
- No `getStateAsXml`/`setStateFromXml` adoption — session format untouched (`PluginProcessor.cpp:653-666` stays exactly as-is).
- No `setCustomStateCallbacks` — O-Bitrot has no custom state; SEED is an APVTS param. `FactoryPresetDef::customState = juce::var()` throughout.
- EB Garamond woff2 bundling declined — system Garamond fallback stays.
- Windows CI — deferred to a future publish cycle.
- AsyncUpdater `cancelPendingUpdate()` — N/A, grep-verified absent (RESEARCH G2). Recorded as checked; no task.

## Tasks

### 1. [ ] Module dependency wiring (CMake + registry)
- `ouaricon_add_module(OBitrot preset-manager)` in `CMakeLists.txt` — after `juce_generate_juce_header(OBitrot)` (:92), before the binary-data target (Tapestop precedent). `include(OuariconModules.cmake)` already exists at :2.
- **Include path, never a vendored copy.** Configure-copies the JS to `Source/ui/public/modules/preset-manager.js` — that IS O-Bitrot's canonical UI root; **do NOT relocate it** (Tapestop's plan tried; a relocated copy is re-created every configure — SUMMARY Task 1 deviation).
- Add `Source/ui/public/modules/preset-manager.js` to the **existing** `OBitrot_UIResources` target (`CMakeLists.txt:95-102`). No second binary-data target; default `BinaryData` namespace; hyphen-stripped symbol: `BinaryData::presetmanager_js`.
- Regenerate registry `used_by` via `scripts/regen-registry-used-by.sh` — do not hand-edit `registry.yaml`.
- Files: `plugins/O-Bitrot/CMakeLists.txt`, `Source/ui/public/modules/preset-manager.js` (generated), `modules/registry.yaml` (scripted).
- Depends on: none.

### 2. [ ] Processor: manager member
- `#include <OuariconPresetManager.h>` + public member `OuariconPresetManager presetManager { apvts, "Ouaricon Bitrot" };` declared **after** `apvts` (`PluginProcessor.h:77` — member init order). **Literal** name so dev/release variants share one library (`~/Library/Ouaricon Bitrot/Presets/`).
- `initializeFactoryPresets(factoryPresets)` once at the **end** of the constructor, after param-pointer caching (`PluginProcessor.cpp:315-360`) and after the Task-3 convertTo0to1 batch loop.
- **Do NOT touch `getStateInformation`/`setStateInformation`.** No custom-state callbacks.
- Seed determinism needs zero new code: preset apply is a message-thread `setValueNotifyingHost`; `processBlock` detects the SEED change at the block boundary and reseeds all 8 streams (`PluginProcessor.cpp:461-468`). Harness re-confirms in Task 8.
- Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`.
- Depends on: Task 1.

### 3. [ ] Factory bank: 8 presets
- Authoring: **engineering units + batch `convertTo0to1` loop** in the constructor before `initializeFactoryPresets` (Tapestop `PluginProcessor.cpp:232-242` shape). Never raw fractions — `CLOCK_FREE_RATE` (skew centre 1.414 Hz) and `CRUSH_RATE` (skew centre 3162 Hz) are skewed.
- Choices authored as **index in engineering units** (5 choice params); bools as 0/1 (7 bools); SEED as the integer 0–9999. **Every preset lists all 31 param IDs** (explicit is auditable; module WR-01 covers omissions as defense in depth).
- `customState = juce::var()` on every def.
- Bank per the RESEARCH draft table (values ear-tunable at execute; names + coverage matrix are the contract; all names slash-free per G4):
  1. **Worn Cassette** — Tape showcase, Free 1.2 Hz, SEED 1111
  2. **Skipping Disc** — CD showcase, Sync 1/16, SEED 2222
  3. **Locked Groove** — Vinyl showcase, Free 0.4 Hz, SEED 3333
  4. **Dropped Call** — Packet showcase, Sync 1/8, SEED 4444
  5. **Cellphone 1998** — Codec (GSM) + light Packet, Sync 1/4, SEED 5555
  6. **Eight-Bit Ruin** — Crush showcase, SEED 6666
  7. **Total Media Failure** — ALL six ON, Sync 1/16, HARD_EDGES On, Mu-law, SEED 7777
  8. **Gentle Rot** — subtle Tape+CD+Vinyl patina, Free 0.7 Hz, MIX 85, SEED 8888
- Coverage invariants: 6 family showcases · sync (2,4,5,7) + free (1,3,8) · extreme (7) + subtle (8) combos · HARD_EDGES exercised (7) · both CODEC modes (5 GSM, 7 Mu-law) · both skewed params exercised (1/3/8, 6/7).
- **Dev sentinel note (G13):** while iterating at 0.1.0, delete `~/Library/Ouaricon Bitrot/Presets/Factory/.factory-version` (or the Factory dir) to force a bank rewrite; the 1.0.0 bump at verify regenerates automatically.
- Post-build spot check: one skewed value on disk (e.g. CRUSH_RATE 11025 Hz in Eight-Bit Ruin must NOT serialize as a linear fraction).
- Files: `Source/PluginProcessor.cpp` (or `Source/FactoryPresets.h` if the table exceeds ~150 lines — executor's call, single TU either way).
- Depends on: Task 2.

### 4. [ ] Editor: 10 native functions (from zero) + resource route
- O-Bitrot currently registers **zero** native fns (`PluginEditor.cpp:66`). Register exactly the 10 `preset-manager.js:108-117` requests: `savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`. Copy O-Tapestop `PluginEditor.cpp:231-366` in shape verbatim, rename types.
- Simple fns: thin `complete(juce::var(...))` wrappers.
- Dialog fns (`savePresetWithDialog`, `loadPresetFromFile`): `std::shared_ptr<juce::FileChooser>` captured into its own callback; `juce::Component::SafePointer<OBitrotAudioProcessorEditor>` **hoisted to a local** (MSVC init-capture trap); on dead editor **bare `return` — never `complete(false)`** (UAF). Both return **`{success, name}` DynamicObjects, not bools**. `savePresetWithDialog` writes via `savePresetToFile()` (honors chosen path — the O-DigiDelay bug).
- `.withNativeFunction(...)` calls fold into the existing `Options{}` chain among the 31 `.withOptionsFrom` calls.
- Resource route in `getResource()` (`PluginEditor.cpp:224-272`, bare paths): `/modules/preset-manager.js` → `{ BinaryData::presetmanager_js, "application/javascript" }`.
- Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`.
- Depends on: Task 2.

### 5. [ ] Band markup + CSS + JS wiring (header center)
- **Placement (G15, measured in F4):** compact band (~26–28 px tall) as a third flex child in the header's ≈400 px empty center (`.hdr`, `index.html:86-94`). **Zero change to grid/global geometry** — the 3×2 grid has no vertical slack; any grid edit is a red flag, stop and reassess.
- Markup: copy O-Tapestop's band structure (`O-Tapestop/Source/ui/public/index.html:56-66`) — IDs `preset-prev`, `preset-next`, `preset-name`, `preset-save`, `preset-load`, `preset-delete`; controls ship `disabled`; button copy in `data-label`/`data-confirm` attributes, never JS literals; `#preset-name` stays childless (`_updateDisplay()` writes `textContent`; name field ~140 px).
- Style in the Naturalist idiom (brown borders, small-caps); no dropdown (README's dropdown CSS skipped — prev/next + dialogs cover browse, Tapestop precedent).
- Inline `<script type="module">` (index.html:729-931), not app.js: `import { PresetManager } from "./modules/preset-manager.js"`; construct with **explicit DOM refs for all six elements** + **`getNativeFunction: Juce.getNativeFunction`** (omitted → silently inert band). **Never `createPresetBar()`** (innerHTML-wipes, no delete button).
- `initialize()` at the **end** of the existing init flow (view visible — completions are dropped while hidden); un-disable the six controls only after `initialize()` resolves (a dead bridge leaves the band honestly disabled).
- Delete confirm via **`onConfirmDelete`** returning the two-click `data-armed` pattern (2.5 s auto-disarm) — never `window.confirm` (unreliable in JUCE WebView).
- Files: `Source/ui/public/index.html` (markup + CSS + inline script).
- Depends on: Tasks 1, 4.

### 6. [ ] CHANGELOG.md
- Create `plugins/O-Bitrot/CHANGELOG.md` from the O-Tapestop template: `# Changelog — O-Bitrot`, `## [1.0.0] — <date>`, "Initial release.", `### Added` bullets (6 degradation families, shared-buffer stochastic engine, seeded determinism, sync/free clocking, Naturalist WebView UI with event LEDs, preset system + 8-preset factory bank). Version in CMakeLists stays 0.1.0 until verify; date finalized at verify.
- Files: `plugins/O-Bitrot/CHANGELOG.md`.
- Depends on: none (content references Tasks 1–5).

### 7. [ ] Native-fn parity gate — 10↔10
- Re-run the stage-3 grep-diff (`3-gui/VERIFICATION.md` method) **both directions at 10↔10**: every `getNativeFunction("...")` in JS has a `withNativeFunction` in C++ and vice versa, excluding the vendored `js/juce/index.js` library matches.
- Event-name parity unchanged: `ledUpdate`.
- Depends on: Tasks 4, 5.

### 8. [ ] Render harness re-run — 44/44
- Rebuild with tests on, re-run `tests/render-harness`. **Mandatory** — the processor TU gains the member + factory bank. Pre-verified compatible (F9): harness compiles PluginProcessor.cpp, inherits the module include dir via target INCLUDE_DIRECTORIES, defines `JucePlugin_VersionString`; `createEditor` already `#if JUCE_WEB_BROWSER`-guarded.
- Gate: **44/44 or stage 4 does not pass.** Any failure → fix without touching frozen Stage-2 DSP semantics. Existing probes cover seed bit-identity — confirms preset apply doesn't break the determinism contract.
- Depends on: Tasks 2, 3.

### 9. [ ] Plugin validation + install
- Build both formats; `./scripts/build-and-install.sh O-Bitrot` (Phase 4 dual-variant sweep).
- `pluginval` strictness 10, VST3 **and** AU, each run **2–3×** (latent-NaN pattern).
- `auval` for the AU after install.
- Smoke the band in Standalone: factory list present and marked, load applies (family enables flip, name displays), save → load round-trip, prev/next wraps, delete-armed two-click on a user preset, dialog save-as + load-from-file.
- Depends on: Tasks 7, 8.

### 10. [ ] Version bump — deferred to verify
- `VERSION 0.1.0` → `1.0.0` at `CMakeLists.txt:11` happens **at stage-4 verify**, not execute (regenerates the factory sentinel + preset version stamps automatically — F6/F10). CHANGELOG date finalized then too.
- Depends on: verify phase.

## Task Order

1 → 2 → 3 → 4 → 5 → 6 (anytime) → 7 → 8 → 9. (10 at verify.)

## Success Criteria

- [ ] Preset band live in the header center: save, save-as (dialog), load, load-from-file, prev/next, two-click delete all function; factory presets listed and marked; frame stays 900×620 with grid/global geometry untouched
- [ ] Factory bank: 8 presets, all 31 params authored in engineering units → batch `convertTo0to1`; skewed-param disk spot-check passes; coverage matrix intact (6 families, sync+free, extremes, both codec modes, HARD_EDGES)
- [ ] Loading any preset resets params to defaults first (module WR-01) and reseeds deterministically via the existing SEED path
- [ ] Native-fn parity 10↔10 both directions; `ledUpdate` event parity unchanged
- [ ] Render harness 44/44
- [ ] pluginval strictness 10 VST3+AU pass ×2–3 each; auval passes
- [ ] Session save/load unaffected (state format untouched; no `getStateAsXml` adoption)
- [ ] CHANGELOG.md exists; installed locally via build-and-install.sh
- [ ] No edits to Stage-2 DSP semantics; no second binary-data target; no relocated module JS

## Pitfall Checklist (from RESEARCH.md — executor MUST read)

1. Constructor + explicit DOM refs, never `createPresetBar()` (G9)
2. `savePresetWithDialog` implemented (10 fns, not module.yaml's 9); dialog fns return `{success, name}` (G10)
3. FileChooser: shared_ptr + hoisted SafePointer + bare `return` on dead editor (G3)
4. No `customState` anywhere — `juce::var()` on every def
5. `convertTo0to1` batch loop, never raw fractions; choices as index, bools 0/1, SEED integer (G5)
6. Module JS stays at `Source/ui/public/modules/` — never relocated; symbol `BinaryData::presetmanager_js`; existing `OBitrot_UIResources` target only (G8)
7. Session state untouched; no custom-state callbacks
8. Parity gate at 10↔10 (G7)
9. Harness re-run mandatory, 44/44 (G6)
10. Delete via `onConfirmDelete` armed pattern, never `window.confirm`
11. `initialize()` at end of init flow (view visible), controls disabled until it resolves (G11/G12)
12. Factory names slash-free (G4); sentinel delete while iterating at 0.1.0 (G13)
13. Band steals zero grid height — any grid geometry change is a plan-phase red flag (G15)
