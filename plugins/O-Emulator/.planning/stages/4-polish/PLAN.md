# Stage 4: Polish - Plan

**Date:** 2026-08-21
**Inputs:** `stages/4-polish/CONTEXT.md`, `stages/4-polish/RESEARCH.md`, `parameter-spec.md` (frozen, 5 params)
**Reference implementation:** O-Bitrot v1.13.0 (`plugins/O-Bitrot/Source/PluginEditor.cpp`, `PluginProcessor.cpp`, `CMakeLists.txt`, `Source/ui/public/index.html`)

## Goal

Complete O-Emulator v1.0.0: integrate preset-manager v1.0.6 via `ouaricon_add_module` (10 native fns, preset band in the reserved header slot), ship a 16-preset factory bank, pass all automated gates with **digest-identical** harness anchors, close the 6 inherited human gates in Logic Pro, write CHANGELOG/docs, and install locally.

## Resolved Open Items (from RESEARCH.md §10)

1. **Factory bank:** 16 presets — 10 console signatures (2×5) + 6 utility. Values in Task 3.
2. **Preset band UI:** **no click-menu** in v1.0 — prev/next + name + save/load/delete only. Walker-safe by construction; a flat menu can come later the O-Bitrot way.
3. **Preset-band width:** expect a small `.hdr` layout tune beyond the 200px reservation; final call at the visual gate (Task 8, gate 1).
4. **CHANGELOG:** single `## 1.0.0` section, house format (prose-first, Added), no migration notes.

## Tasks

1. [ ] **CMake module wiring**
   - `ouaricon_add_module(OEmulator preset-manager)` placed **after** `juce_generate_juce_header(OEmulator)` (O-Bitrot CMakeLists.txt:97 pattern)
   - Add `Source/ui/public/modules/preset-manager.js` to the **existing** `OEmulator_UIResources` target (never a second binary-data target); symbol will be `BinaryData::presetmanager_js` (hyphens stripped)
   - Do NOT commit the generated JS (gitignored, configure-copied); do NOT touch the harness CMake
   - Files: `plugins/O-Emulator/CMakeLists.txt`
   - Depends on: none

2. [ ] **Processor integration**
   - `PluginProcessor.h`: `#include <OuariconPresetManager.h>` (angle brackets); public member declared **after `apvts`**: `OuariconPresetManager presetManager { apvts, "O-Emulator" };` (no dev suffix — dev/release share the preset dir)
   - `getStateInformation`/`setStateInformation`: delegate to `presetManager.getStateAsXml()` / `setStateFromXml()`; after `getStateAsXml()`, re-add `xml->setAttribute("pluginVersion", JucePlugin_VersionString)` (keep the version stamp present)
   - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
   - Depends on: Task 1

3. [ ] **Factory preset bank (16 presets)**
   - `std::vector<FactoryPresetDef>` in PluginProcessor.cpp, authored **denormalized**; batch `convertTo0to1` pass then `presetManager.initializeFactoryPresets(...)` in the constructor (O-Bitrot PluginProcessor.cpp:1249-1252 idiom)
   - Every preset lists **all 5 IDs** (`console`, `crush`, `age`, `reverb`, `mix`); console authored as choice index (SNES=0, PS1=1, NES=2, Game Boy=3, Genesis=4)
   - Bank (names ASCII, no `/ \ :`, flat case-insensitive alphabetical is the display order):

     | Preset | console | crush | age | reverb | mix |
     |---|---|---|---|---|---|
     | Crush Extreme | 2 | 100 | 30 | 0 | 100 |
     | GB Pocket Speaker | 3 | 75 | 55 | 0 | 100 |
     | GB Signature | 3 | 40 | 12 | 0 | 100 |
     | Genesis Arcade Floor | 4 | 70 | 50 | 25 | 100 |
     | Genesis Signature | 4 | 45 | 10 | 0 | 100 |
     | Lo-Fi Drums | 0 | 55 | 25 | 8 | 100 |
     | NES Basement | 2 | 80 | 70 | 10 | 100 |
     | NES Signature | 2 | 45 | 15 | 0 | 100 |
     | Parallel Grit | 4 | 85 | 35 | 0 | 45 |
     | PS1 Demo Disc | 1 | 70 | 45 | 55 | 100 |
     | PS1 Signature | 1 | 40 | 8 | 30 | 100 |
     | Reverb Chamber | 1 | 20 | 10 | 85 | 100 |
     | SNES Signature | 0 | 35 | 10 | 12 | 100 |
     | SNES Worn Cart | 0 | 65 | 60 | 20 | 100 |
     | Subtle Glue | 0 | 15 | 5 | 0 | 35 |
     | Tape Wash | 1 | 30 | 65 | 40 | 85 |

   - Values are starting points — audition during Task 8 and tune in place if a preset is unmusical (value edits only; renames require re-checking alphabetical intent)
   - Files: `Source/PluginProcessor.cpp`
   - Depends on: Task 2

4. [ ] **Editor: 10 native functions + resource provider**
   - Register exactly 10 with `.withNativeFunction(...)` on the options chain **before** WebView construction (member order stays Relays → WebView → Attachments): `savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset` — copy shapes from O-Bitrot PluginEditor.cpp:184-333, **skipping** its 3 extras (tooltips ×2, `getPresetListGrouped`)
   - Async dialog fns (`savePresetWithDialog`, `loadPresetFromFile`): SafePointer hoisted to a **local** before `launchAsync`; dead editor → **bare `return`, never `complete(false)`**; `std::shared_ptr<juce::FileChooser>` captured into its own callback; complete with `{success, name}` DynamicObject
   - Resource provider: serve `"/modules/preset-manager.js"` → `BinaryData::presetmanager_js`, MIME `application/javascript`
   - Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
   - Depends on: Task 1

5. [ ] **HTML/JS preset band**
   - Fill the reserved `.preset-band` with the O-Bitrot markup family (index.html:769-798): `preset-prev`, `preset-next`, `preset-name`, `preset-save`, `preset-load`, `preset-delete`; all buttons ship `disabled`, un-disabled after `initialize()` resolves; restyle to O-Emulator brown/accent variables
   - `import { PresetManager } from "./modules/preset-manager.js";` in the existing inline module script — **never `createPresetBar()`**
   - Pass `prevButton`/`nextButton` **straight to the module constructor** (flat list ⇒ native walkers agree); no `stepPreset` override, no menu
   - `onConfirmDelete`: two-click armed confirm with 2.5 s auto-disarm (O-Bitrot index.html:1415-1441); button copy via `data-label`/`data-confirm` attributes
   - Mount the preset block **last** in the module script, after knob/segment wiring; never await a completion across editor close
   - Files: `Source/ui/public/index.html`
   - Depends on: Task 4

6. [ ] **Build + automated re-validation**
   - Configure + `ninja OEmulator_VST3 OEmulator_AU OEmulator_Standalone` (Standalone explicitly — needed for gate 6)
   - Render harness: 52 checks ALL PASS, digest anchors **identical** (9cf6baa8d3b61b14 / b23fe10b74526fab / dad157a01f7c393f) — any drift is a defect, not a re-anchor case
   - pluginval strictness 10 SUCCESS VST3+AU; `auval -v aufx OEmu OuDv` PASS
   - Bridge audit re-anchored **10↔10**: `withNativeFunction` in PluginEditor.cpp (10) ↔ `getNativeFunction` in generated `modules/preset-manager.js` (10) + index.html (0); `window.__JUCE__` still 0 in authored code
   - One binary-data target; factory JSON appears at `~/Library/O-Emulator/Presets/Factory/` with `.factory-version` sentinel
   - Depends on: Tasks 2, 3, 5

7. [ ] **Install**
   - `./scripts/build-and-install.sh O-Emulator` (dual-variant sweep + AU cache clear built in)
   - Verify `auval -a | grep -i emulator`; Standalone dev build kept for the inspector gate
   - Depends on: Task 6

8. [ ] **Human gates — Logic Pro (6, inherited from Stage 3)**
   1. Visual pass at 620×430 incl. populated preset band (specimen, segments, header spacing, band width; final ±10px height call)
   2. Console switch audibly rides the 30 ms crossfade across all 5 consoles; accent + readout follow
   3. Knob feel: relative drag, shift-fine, wheel, double-click typed entry, Alt/Option-click reset
   4. Host automation of all 5 params updates the UI live
   5. Preset/session reload: segments, accent, readout, knobs AND preset-name display refresh on preset load; session save/reload restores `currentPreset`; prev/next walks the flat alphabetical bank; save/delete round-trip on a user preset
   6. No WebView console errors — Safari Web Inspector on the dev **Standalone**
   - Also: audition the 16 factory presets; tune Task-3 values in place where unmusical
   - Depends on: Task 7

9. [ ] **Docs + closure**
   - Create `plugins/O-Emulator/CHANGELOG.md` — single `## 1.0.0` section, house (O-Bitrot) format
   - PLUGINS.md own row: 🚧 Stage 3 → 📦 Installed (v1.0.0)
   - REQUIREMENTS.md: UI-01 partial → complete, UI-02 deferred → complete
   - STATUS.md stage-4 execute results
   - Files: `plugins/O-Emulator/CHANGELOG.md`, `PLUGINS.md`, `.planning/REQUIREMENTS.md`, `.planning/STATUS.md`
   - Depends on: Task 8

## Task Dependency Graph

```
1 (CMake) ─→ 2 (processor) ─→ 3 (factory bank) ─┐
      └────→ 4 (editor fns) ─→ 5 (preset band) ─┼─→ 6 (build+validate) → 7 (install) → 8 (human gates) → 9 (docs)
```

## Success Criteria

- [ ] Preset band live in the reserved header slot: prev/next, name display, save (dialog), load (file dialog), delete (armed confirm) — all functional, no menu
- [ ] 16 factory presets on disk, flat alphabetical, each storing all 5 params; loading any preset snaps every control (WR-01 defense in depth)
- [ ] Harness digest anchors byte-identical to Stage-2 baseline (9cf6baa8d3b61b14 / b23fe10b74526fab / dad157a01f7c393f)
- [ ] pluginval strictness 10 SUCCESS VST3+AU; auval PASS
- [ ] Bridge audit passes at the new 10↔10 anchor (index.html still 0; `window.__JUCE__` 0 authored)
- [ ] All 6 human gates pass in Logic Pro (inspector gate via dev Standalone)
- [ ] `currentPreset` survives a Logic session save/reload; `pluginVersion` attribute still present in saved state
- [ ] CHANGELOG.md exists; PLUGINS.md row flipped; REQUIREMENTS.md UI-01/UI-02 complete (15/15)
- [ ] Installed via build-and-install.sh with dual-variant sweep clean

## Risks

| Risk | Mitigation |
|---|---|
| Harness digest drift from processor changes | State-fn + ctor factory-init changes never touch the audio path; if drift appears, treat as defect and bisect the processor diff |
| Dialog completion UAF on editor close | SafePointer-local + bare-return rule (Task 4); buttons disabled until init |
| Preset band overflows 200px reservation | Header layout tune allowed; settled at visual gate |
| Factory values unmusical | Audition at Task 8; values are data-only edits, re-run of Task 6 gates required only if C++ recompiled (it will be — re-run harness digests after tuning) |
