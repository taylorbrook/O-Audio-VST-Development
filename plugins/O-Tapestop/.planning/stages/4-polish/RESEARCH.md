# Stage 4: Polish — Research

**Date:** 2026-08-15
**Scope:** preset-manager v1.0.5 integration (envelope blob carriage, band wiring, factory bank), CHANGELOG, automated validation gate, local install.

---

## Answers to the discuss-phase open questions

### Q1: Envelope blob — schema extension or opaque hook?

**The opaque hook covers it. No schema extension needed.**

`OuariconPresetManager::setCustomStateCallbacks(save, load)` (module header `modules/persistence/preset-manager/cpp/OuariconPresetManager.h:100`) embeds whatever `juce::var` the save callback returns as a `customState` property in the preset JSON, and hands it back to the load callback on `loadPreset()`/`loadPresetFromFile()`. `FactoryPresetDef` has a `customState` field too (`:190`), so factory presets carry the envelope the same way.

Recommended carriage: a `DynamicObject` with one property, `"scratchEnvelope"`, holding the existing versioned JSON **string** from `scratchEnvelope.toJson()` — the blob stays opaque and self-versioned; the preset layer never parses it.

- **Save callback:** `obj->setProperty("scratchEnvelope", scratchEnvelope.toJson())` — the same string `getStateInformation` already persists (`PluginProcessor.cpp:535-544`).
- **Load callback:** defensive-shape check (null obj / missing property → leave envelope alone), then reuse the existing `commitScratchEnvelopeJson()` entry point (`PluginProcessor.h:95-100`) — it does the message-thread `setFromJson` bake + atomic double-buffer publish **and bumps `uiEnvGeneration`**, so the 30 Hz editor timer pushes the sanitized `envelopeState` echo to the WebView with zero new plumbing. `loadPreset` is invoked from a native fn → message thread → thread contract holds.

Reference implementation shape: **O-SpectralShaper** `Source/PluginProcessor.cpp:122-165` (32-pt curve blobs through the same callbacks, with the null-check → hasProperty → size-check defensive ladder).

**Load-bearing module fact** (documented in O-Octagon `PluginEditor.cpp:1083-1100`): `applyPresetJson` calls the load callback **only when `customState` exists** (`OuariconPresetManager.h:346-349`). `applyPresetJson` resets all APVTS params to defaults first (WR-01, fixed in v1.0.3) — but custom state has no such reset. A preset without `customState` would silently inherit the live envelope. **Therefore every factory preset — including Stop-mode ones — must carry an explicit envelope blob**, and user saves always will (the save callback is unconditional).

### Q2: Preset-band placement in the 860×580 frame?

**Already solved — stage 3 shipped it.** `Source/ui/index.html:56-66` contains a complete, disabled `div.preset-bar`: 32 px + 12 px margin = the 44 px band already counted in the height budget comment (`styles.css:29-33`). Element IDs (`preset-prev`, `preset-next`, `preset-name`, `preset-save`, `preset-load`, `preset-delete`) were authored to exactly match what `preset-manager.js` binds. `#preset-delete` already has a `data-armed="1"` confirm style (`styles.css:226-231`); `.preset-name` ships dimmed at opacity 0.65 with an inline note that Stage 4 removes the dim. **Stage 4 un-disables the buttons and wires the manager — no layout work, frame stays 860×580.**

Corollary: **do NOT use `createPresetBar()`** — it `innerHTML`-wipes the container (JS `:419`), destroying the styled band and fleurons, and it never creates a delete button. Use the `PresetManager` constructor (Option 2) with explicit DOM refs for all six elements.

---

## Integration mechanics

### C++ side (processor)

- Wire via **module include path, not a vendored copy**: `/module-add O-Tapestop preset-manager` appends `ouaricon_add_module(OuariconTapestop preset-manager)` (the `include(OuariconModules.cmake)` line already exists at `CMakeLists.txt:3`) and updates the registry. `ouaricon_add_module` adds `modules/persistence/preset-manager/cpp` as a PRIVATE include dir — header compiled in place. All 8 vendored copies in the repo have drifted behind the module; O-AnalogEQ is the clean modern reference (`plugins/O-AnalogEQ/CMakeLists.txt:55-56`).
- Member `OuariconPresetManager presetManager { parameters, "Ouaricon Tapestop" };` — the name is the presets directory `~/Library/{name}/Presets/`. Pass a **literal** so dev (`-dev`) and release builds share one preset library.
- `initializeFactoryPresets(...)` once in the constructor. v1.0.5 is sentinel-gated (`.factory-version`, WR-04): file writes happen only when `JucePlugin_VersionString` changes — auval/pluginval scan-storm safe. The 0.1.0 → 1.0.0 bump at verify regenerates the bank automatically.
- **Keep `getStateInformation`/`setStateInformation` exactly as they are.** Do NOT adopt the module's `getStateAsXml`/`setStateFromXml` — that changes the session-state format (envelope would move from an APVTS-root attribute to a `CustomState` child element) for zero benefit, and stage-2/3 verified the current path. The module's preset functions work independently of its session functions. (Optional, non-breaking: persist `currentPresetName` as a second tree property so sessions reopen showing the preset name — plan-phase decision, default skip.)
- **No AsyncUpdater exists in O-Tapestop** (grep-confirmed) and no meta parameters — the `cancelPendingUpdate()` guard pattern from CONTEXT does not apply here. The only queued-apply channel is the generation counter, which is idempotent (latest generation wins).

### Native functions (editor)

The JS requests **10** native fns by name (`preset-manager.js:108-117`); all 10 must be registered or the bridge-gap pattern fires:

`savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`

- **`savePresetWithDialog` is undocumented** — absent from module.yaml and the README example — but it's what the Save button actually calls (JS `:131` → `:273`). Implement with `FileChooser` saveMode into `getUserPresetsDirectory()`, writing via `savePresetToFile()` (honors the chosen path — the O-DigiDelay bug was a dialog whose destination `savePreset()` ignored).
- **Both dialog fns must return `{success, name}` objects** — the JS reads `result.success`/`result.name` (`:274`, `:296`); a bare bool reads as failure even when the file was written.
- **FileChooser pattern:** copy O-Polystutter `PluginEditor.cpp:416-497` verbatim in shape: `std::shared_ptr<FileChooser>` captured into its own callback; `SafePointer` **hoisted to a local** (MSVC rejects `SafePointer(this)` init-captures in nested lambdas); on dead editor **`return` — never `complete(false)`** (UAF on the dead WebView impl).
- Total registered fns become 13 (existing `getParameterDefaults`, `commitEnvelope`, `requestEnvelope` + 10). **The stage-3 grep-diff parity check (JS↔C++ both directions, VERIFICATION.md:58) must be re-run at 13↔13**, plus the event-name parity check (`transportFrame`, `envelopeState` unchanged).

### JS side (WebView)

- `app.js` already imports the `Juce` ES-module namespace and `window.__JUCE__.backend` is confirmed populated (`app.js:505` uses it for events) — so the module's readiness gate (`_waitForNative()` polls `window.__JUCE__.backend`, 5 s timeout) passes immediately. Pass `getNativeFunction: Juce.getNativeFunction` in the constructor options — **if omitted, the bar renders but is silently inert** (single console error, `isInitialized` never set).
- Wire `deleteButton` + **`onConfirmDelete`** using the two-click `data-armed` pattern the CSS anticipates. Do not rely on the module's `window.confirm()` fallback — native confirm dialogs inside a JUCE WebView are unreliable, and the module treats a throwing/falsy confirm as "abort" (fail-safe, but the button would just do nothing).
- Known module gotchas to design around: every native call is awaited with no timeout — a dropped completion while the view is hidden (repo memory: completions gated on `isVisible`) hangs that flow silently, and `initialize()` awaits `refresh()` before setting `isInitialized`. Initialize from `app.js` after the existing bridge setup (view is visible by then); don't call it at module-eval time.
- `_updateDisplay()` writes `textContent` into `#preset-name` — keep that element childless (it already is).

### Resource embedding

`ouaricon_add_module` copies JS to `Source/ui/public/modules/` — **wrong for O-Tapestop**, whose UI root is `Source/ui/` (no `public/`). After `/module-add`, move the file to `Source/ui/js/modules/preset-manager.js`, then:
1. Add it to the existing `juce_add_binary_data(OuariconTapestop_UIResources NAMESPACE UIBinaryData ...)` target — no second binary-data target (namespace-collision pattern). Hyphens strip: symbol is `UIBinaryData::presetmanager_js`.
2. Add the `/js/modules/preset-manager.js` route to `TapestopEditor::getResource` (`PluginEditor.cpp:77-116`) — provider matches bare paths.
3. `import { PresetManager } from "./js/modules/preset-manager.js"` in `app.js` (it's an ES module; the global-name fallback only exists under `type="module"` anyway).

---

## Factory bank (8 presets)

**Authoring style: engineering units + batch `convertTo0to1`** — the CR-02 pattern, O-SpectralShaper `PluginProcessor.cpp:302-310`:

```cpp
for (auto& preset : factoryPresets)
    for (auto& [paramId, value] : preset.parameters)
        if (auto* p = parameters.getParameter(paramId))
            value = p->convertTo0to1(value);
presetManager.initializeFactoryPresets(factoryPresets);
```

`FactoryPresetDef::parameters` stores **normalized** values verbatim (raw-literal authoring like O-Comp/O-AnalogEQ is the known trap: `STOP_FREE_MS`/`START_FREE_MS`/`ENV_FREE_MS` are skewed ranges). Choice params (`MODE`, `SYNC_MODE`, the three `*_SYNC_DIV`s) are authored as the **index** in engineering units — `convertTo0to1(index)` handles the 0..N-1 range. Every preset lists **all 14 param IDs** (defense in depth, though WR-01 reset-to-defaults covers omissions) **plus an explicit `customState` envelope blob** (see Q1 — required even for Stop-mode presets).

Bank sketch (names/values finalized at plan): the 4 roadmap presets — Classic ½-Bar Stop, Classic 1-Bar Stop (or a curve variant), DJ Spinup, 2 scratch gestures — plus tempo-synced short stop (1/8), slow-tape drag (long free-ms stop, heavy TONE_TRACK), stutter-scratch (short synced envelope, aggressive shape). Coverage matrix from CONTEXT: both MODEs, both SYNC_MODEs, curve extremes.

Version metadata: presets are stamped with `JucePlugin_VersionString` (IN-02) — correct automatically after the 1.0.0 bump.

---

## Validation gate (automated only, per CONTEXT)

| Check | Command / note |
|---|---|
| Render harness 47/47 | rebuild with `-DOUARICON_BUILD_TESTS=ON`, re-run `tests/render-harness` — **required because the processor gains the preset-manager member + callbacks**. Header-only, `juce_core`+`juce_audio_processors` only, no WebView types → harness-safe. Verify `JucePlugin_VersionString` is defined in the harness TU (it references it via `createPresetJson`). |
| pluginval strictness 10, VST3 + AU | `/Applications/pluginval.app/Contents/MacOS/pluginval` — run **2–3× each** (latent-NaN pattern) |
| auval | `auval -v aufx OTsp <mfr>` after install |
| Native-fn parity | grep-diff JS↔C++ at 13↔13, both directions; event parity unchanged |
| Bit-transparency + state round-trip | re-run the stage-1 memcmp/round-trip probes (session format untouched — must still pass byte-identical) |
| Install | `./scripts/build-and-install.sh O-Tapestop` (Phase 4 dual-variant sweep) |

Also at execute/verify: CHANGELOG.md created; `VERSION 0.1.0` → `1.0.0` in CMakeLists at verify; registry `used_by` gains O-Tapestop (prefer `scripts/regen-registry-used-by.sh` — the survey found `used_by` broadly stale, 5 integrations missing).

---

## Pitfall summary (carry into PLAN.md)

1. **Don't use `createPresetBar`** — innerHTML-wipes the styled band, no delete button. Constructor + explicit DOM refs.
2. **`savePresetWithDialog`** must be implemented despite being absent from module docs; dialog fns return `{success, name}` objects, not bools.
3. **FileChooser**: shared_ptr chooser + hoisted SafePointer + `return` (not `complete(false)`) on dead editor.
4. **Every factory preset carries `customState`** — presets without it inherit the live envelope (no custom-state reset in `applyPresetJson`).
5. **Factory values normalized via `convertTo0to1` batch loop** — never raw fractions (skewed ms ranges).
6. **Module JS copy path mismatch** — relocate from `ui/public/modules/` to `ui/js/modules/`, add to `UIBinaryData` + resource provider route (hyphen-stripped symbol `presetmanager_js`).
7. **Session state stays untouched** — module preset fns only; no `getStateAsXml` adoption.
8. **Parity gate widens to 13 native fns** — re-run grep-diff both directions.
9. **Harness re-run is mandatory** — processor TU changes; 47/47 or stage 4 doesn't pass.
10. **Delete confirm via `onConfirmDelete`** two-click armed pattern, not `window.confirm`.
11. Module quirks (accepted, no action): no JS version marker; `menuButton` option dead; `initialize()` not awaited by design — init from `app.js` post-bridge; name sanitization (`/ \ :` → `_`) already handled module-side (WR-04).

## Reference map

- Custom-state callbacks: O-SpectralShaper `PluginProcessor.cpp:122-165`; O-Bells `:582-615`
- Factory authoring (correct style): O-SpectralShaper `PluginProcessor.cpp:280-310`; O-Wind `:805-823`
- FileChooser/SafePointer: O-Polystutter `PluginEditor.cpp:416-497`
- Modern CMake module wiring: O-AnalogEQ `CMakeLists.txt:3,55-56`
- Module internals: `modules/persistence/preset-manager/cpp/OuariconPresetManager.h` (WR-01 reset-first `:315-341`, customState gate `:346-349`, sentinel `:614-622`, `savePresetToFile` `:124`)
- O-Tapestop touch points: `PluginProcessor.cpp:535-572` (state), `PluginProcessor.h:95-104` (envelope commit/read), `PluginEditor.cpp:151-205` (native fns), `:275-299` (generation echo), `index.html:56-66` + `styles.css:159-250` (band), `CMakeLists.txt:62-78` (binary data)
