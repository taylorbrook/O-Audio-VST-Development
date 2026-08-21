# Stage 4: Polish - Research

**Date:** 2026-08-21
**Scope (from CONTEXT.md):** preset-manager v1.0.6 + 15+ factory presets, 6 inherited human gates in Logic Pro, final validation, local install only.

---

## 1. Reference integration: O-Bitrot v1.13.0 (confirmed best model)

Newest preset work in the repo (`87d732e2`, 2026-08-19), v1.0.6, JUCE 8 WebView, same Naturalist aesthetic O-Emulator's Stage 3 was built from. Its Stage-4 planning docs are direct prior art: `plugins/O-Bitrot/.planning/stages/4-polish/{RESEARCH,PLAN,SUMMARY}.md` (bridge-audit parity method at RESEARCH.md:70).

**Key structural finding: nothing is vendored.** Despite CONTEXT.md's "vendored" wording, the house pattern (11 plugins, incl. O-Bitrot, O-Tapestop, O-simpleFM, O-Tremolo) is:

- C++: canonical header compiled in place — `ouaricon_add_module(<Target> preset-manager)` adds `modules/persistence/preset-manager/cpp` to the include path. `#include <OuariconPresetManager.h>` (angle brackets).
- JS: `configure_file(... COPYONLY)` in `OuariconModules.cmake:109-116` regenerates `Source/ui/public/modules/preset-manager.js` (byte-identical to canonical) **at every CMake configure**. The copy is **gitignored** (`.gitignore:26`) — never edit or relocate it.

This auto-tracks the canonical version — no vendored-copy sync debt (10 older plugins still carry stale pre-1.0.5 forks; do not imitate them).

## 2. Module surface (canonical v1.0.6, verified from source)

`modules/persistence/preset-manager/` — `module.yaml` v1.0.6; `cpp/OuariconPresetManager.h` (688 lines, header-only); `js/preset-manager.js` (447 lines). Version lives only in module.yaml (registry.yaml agrees this time, but module.yaml is authoritative).

Already built in (no plugin-side work needed):
- **WR-01** (v1.0.3): `applyPresetJson()` resets every param to default before applying — CONTEXT constraint satisfied by the module itself. Order: migration hook → reset-to-defaults (meta-first) → apply (meta-first) → customLoad. O-Emulator has no meta params and no custom state — both passes degenerate cleanly.
- **WR-04** (v1.0.3/1.0.4): factory writes gated by a `.factory-version` sentinel (only rewritten on version change — auval/pluginval scan-storm safe); factory names sanitized.
- **Name sanitization** (v1.0.2): `/ \ :` → `_` everywhere a name becomes a filename — the "/" silent-drop constraint is handled.
- **IN-03**: prev/next resume from last in-list index after an out-of-list file load.
- **IN-04/IN-05**: bounded `_waitForNative()` poll; `promptDelete()` prefers an `onConfirmDelete` hook (never rely on `window.confirm`).
- **v1.0.6 migration hook**: `setMigrationCallback(parameters&, presetVersion)` — **not needed at v1.0.0** (nothing older exists to migrate). Future gate only: if `console` ever gains a 6th entry, register the remap (O-Bitrot `PluginProcessor.cpp:609-636` is the template; per-param version gate).

## 3. C++ integration plan (processor)

- `PluginProcessor.h`: `#include <OuariconPresetManager.h>`; public member **declared after `apvts`**:
  ```cpp
  OuariconPresetManager presetManager { apvts, "O-Emulator" };
  ```
  Name string: literal plugin name, **no dev suffix** — dev and release builds share `~/Library/O-Emulator/Presets/`. (House convention is mixed: older = "Ouaricon Bitrot"/"Ouaricon Tapestop", newer = "O-Tremolo"/"O-ReverseDelay". Follow the newer.)
- `getStateInformation`/`setStateInformation`: delegate to `presetManager.getStateAsXml()` / `setStateFromXml()` so `currentPreset` survives DAW sessions. **Preserve the existing `pluginVersion` stamp** — after `getStateAsXml()` returns, re-add `xml->setAttribute("pluginVersion", JucePlugin_VersionString)` (the current code sets it as a ValueTree property; keep the attribute present either way — house migration gates key off it).
- Factory presets: authored **denormalized in engineering units** in a `std::vector<FactoryPresetDef>` in PluginProcessor.cpp, then batch-converted once (O-Bitrot `PluginProcessor.cpp:1249-1252`):
  ```cpp
  for (auto& preset : factoryPresets)
      for (auto& [paramId, value] : preset.parameters)
          if (auto* p = apvts.getParameter(paramId))
              value = p->convertTo0to1(value);
  presetManager.initializeFactoryPresets(factoryPresets);
  ```
  Called once in the processor constructor. All O-Emulator ranges are linear (floats 0–100, console choice → index 0–4 authored as the index), so the skew trap is moot here — but keep the `convertTo0to1` batch conversion anyway (robust to any future range change, and it's the house idiom).
- **Each preset lists all 5 param IDs** (defense in depth over WR-01, per O-Bitrot's 45-ID-per-preset practice).

### Harness impact (must stay digest-identical)

- The harness compiles `PluginProcessor.cpp` (not the editor). `OuariconPresetManager.h` needs only juce_core/juce_audio_processors + `JucePlugin_VersionString` — the harness defines it (`tests/render-harness/CMakeLists.txt:90`). Compiles clean; **do not touch the harness CMake**.
- Processor-ctor factory init will write `~/Library/O-Emulator/Presets/Factory/*.json` on the first harness run — sentinel-gated afterward, zero effect on the audio path. Do **not** `#if`-gate it out; keep the shipping TU identical (O-Bitrot doesn't gate either).
- The state-function change adds a `currentPreset` XML attribute; APVTS content is unchanged. Digest anchors (9cf6baa8d3b61b14 / b23fe10b74526fab / dad157a01f7c393f) must re-run identical — any drift is a defect, not a re-anchor case.

## 4. C++ integration plan (editor) — 10 native functions

The JS module resolves exactly 10 (`preset-manager.js:108-117`): `savePreset`, `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`. Register all 10 with `.withNativeFunction(...)` on the options chain **before** the WebView is constructed; member order stays Relays → WebView → Attachments. Copy shapes from `plugins/O-Bitrot/Source/PluginEditor.cpp:184-333` (skip its 3 extras: tooltips ×2 + `getPresetListGrouped` — grouped menu explicitly out of scope, flat list decided).

**The two async dialog fns** (`savePresetWithDialog`, `loadPresetFromFile`) carry the danger:
- Hoist `juce::Component::SafePointer` **to a local** before `launchAsync` (MSVC rejects `SafePointer(this)` init-capture in nested lambdas).
- On a dead editor the callback does a **bare `return` — NOT `complete(false)`** (calling complete would UAF the dead WebView impl). O-Bitrot `PluginEditor.cpp:168-182` states the rule; `:200-207`, `:247-254` implement it.
- `std::shared_ptr<juce::FileChooser>` captured into its own callback to keep it alive.
- Complete with a `{success, name}` `DynamicObject` — a bare bool reads as failure JS-side.
- Save dialog: `savePresetToFile(file)` honors the chosen directory (module ≥ the WR-03 fix era); presets only appear in the list when saved into the User dir.

**Bridge-audit re-anchor** (CONTEXT constraint): parity gate becomes **10 ↔ 10**, but the JS side moves — the 10 `getNativeFunction` calls live in `Source/ui/public/modules/preset-manager.js` (configure-generated, gitignored), while `index.html` stays at **0**. New audit spec: `withNativeFunction` count in PluginEditor.cpp (10) ↔ `getNativeFunction` count in the generated module JS (10) + index.html (0). `window.__JUCE__` stays 0 in *authored* code (the module JS's `_waitForNative` references it — that file is generated, same standing as `js/juce/index.js`).

## 5. CMake + resource wiring

- `ouaricon_add_module(OEmulator preset-manager)` — placed **after** `juce_generate_juce_header(OEmulator)` (O-Bitrot `CMakeLists.txt:97`).
- Add `Source/ui/public/modules/preset-manager.js` to the **existing** `OEmulator_UIResources` target (never a second binary-data target — namespace collision). Symbol: hyphens stripped → `BinaryData::presetmanager_js`.
- Resource provider: serve `"/modules/preset-manager.js"` → `presetmanager_js`, MIME `application/javascript` (O-Bitrot `PluginEditor.cpp:574-579`).
- Note: the JS file must exist at configure time for binary-data — `ouaricon_add_module` copies it during the same configure pass, before the build; ordering is safe (proven across 11 plugins).

## 6. JS / HTML integration

- Fill the reserved `.preset-band` (min 200×26, index.html:129) with the O-Bitrot markup family (index.html:769-798): `preset-prev`, `preset-next`, `preset-name`, `preset-save`, `preset-load`, `preset-delete` — all buttons ship `disabled`, un-disabled only after `initialize()` resolves. Restyle to O-Emulator's brown/accent variables; 200px is tight for 6 elements — width is a visual-pass item.
- `import { PresetManager } from "./modules/preset-manager.js";` in the existing inline module script. **Never `createPresetBar()`** — it innerHTML-wipes and creates no delete button.
- **Flat alphabetical list** (CONTEXT decision) means the module's native prev/next walkers agree with any UI ordering — so unlike O-Bitrot (which withheld prev/next from the module to override the walk order for its grouped menu), **pass `prevButton`/`nextButton` straight to the module constructor**. No `stepPreset` override, no `getPresetListGrouped`. If a click-to-open menu is wanted, it must render the same flat `getPresetList()` order — otherwise skip the menu entirely (plan-phase call; skipping is simpler and walker-safe by construction).
- `onConfirmDelete`: O-Bitrot's two-click armed confirm with 2.5 s auto-disarm (index.html:1415-1441) — no `window.confirm`. Button copy via `data-label`/`data-confirm` attributes (a shared textContent updater erases HTML-authored labels).
- Mount the preset block **last** in the module script, after knob/segment wiring (completions are dropped while the WebView is hidden; O-Bitrot orders it this way deliberately). Preset UI must never await a completion across an editor close.
- Preset-load freshness: already solved — relay listeners fire on `setValueNotifyingHost`, so segments/accent/readout/knobs refresh on preset load with no revision counter (Stage-3 finding, unchanged).

## 7. Factory bank (15+) — authoring approach

On-disk: not repo files — C++ definitions written to `~/Library/O-Emulator/Presets/Factory/*.json` at first construction per version. JSON schema: `{"parameters": {id: normalized}, "version": JucePlugin_VersionString, "plugin": "O-Emulator", "factory": true}`.

Authoring table format (denormalized; console = choice index 0–4):

| Preset | console | crush | age | reverb | mix |
|---|---|---|---|---|---|
| SNES Clean Signature | 0 | ~30 | ~10 | 0 | 100 |
| SNES Worn Cart | 0 | ~65 | ~60 | ~15 | 100 |
| … (2 per console × 5, + 5–8 utility) | | | | | |

Exact names/values = **plan phase**, auditioned against the age/crush/reverb ranges (CONTEXT open question). Naming: no `/ \ :` (sanitizer would silently rename); ASCII only; names sort case-insensitively — since the list is flat alphabetical, consider name prefixes deliberately (e.g. console-prefixed signatures group naturally: "GB …", "Genesis …", "NES …", "PS1 …", "SNES …"; utility presets sort among them — prefix choice is a plan decision).

## 8. Validation + human gates (inherited)

- **Automated re-runs after integration:** render harness 52 checks ALL PASS with digests **identical** (any drift = defect); pluginval strictness 10 VST3+AU; `auval -v aufx OEmu OuDv`; bridge audit at the new 10↔10 anchor; one binary-data target; no hyphenated resource filenames served under wrong symbol names.
- **6 human gates in Logic Pro** (CONTEXT list): visual pass at 620×430 (now incl. populated preset band, final ±10px height call), console-switch crossfade audibility, knob feel, automation refresh, preset/session reload, WebView console errors. Gate 5 now also covers: preset load refreshes all controls + preset name display; session save/reload restores `currentPreset`.
- **Console-error gate:** `build-and-install.sh` skips Standalone — rebuild explicitly: `ninja OEmulator_Standalone` (target confirmed in build dir), inspect the dev Standalone via Safari Web Inspector.
- **Docs:** create `plugins/O-Emulator/CHANGELOG.md` (house format: O-Bitrot's — versioned sections, prose-first, Added/Fixed); flip PLUGINS.md row 🚧 Stage 3 → installed/complete; REQUIREMENTS.md UI-01/UI-02 closure.
- **Install:** `./scripts/build-and-install.sh O-Emulator` (does the dual-variant sweep + cache clear).

## 9. Pitfalls checklist (memory-sourced, each mapped)

| Pitfall | Status here |
|---|---|
| Factory presets as linear fractions ignore skew | Author denormalized + `convertTo0to1` batch (all ranges linear anyway) |
| Grouped dropdown desyncs ◀/▶ | Flat list decided; pass prev/next to module — parity by construction |
| "/" in preset name silently fails | Module sanitizes since v1.0.2; still avoid in factory names |
| applyPresetJson stale-value inheritance | WR-01 in module; author all 5 IDs per preset anyway |
| FileChooser launchAsync UAF | SafePointer hoisted local; bare return on dead editor (no `complete(false)`); shared_ptr chooser |
| Completions dropped while WebView hidden | Mount preset block last; never await across editor close; buttons disabled until init |
| Bridge-audit gap fails silently | Re-anchor 10↔10 incl. generated module JS |
| Two binary-data targets collide | Extend `OEmulator_UIResources` only |
| Hyphens stripped by binary data | `preset-manager.js` → `BinaryData::presetmanager_js` |
| One-shot state push stale on preset load | N/A — zero one-shot pushes, relays refresh everything |
| Harness breaks on WebView editor | Already guarded (`JUCE_WEB_BROWSER` around createEditor); module header compiles under harness defines |
| ValueTree XML round-trip loses type | `currentPreset` is a string attribute — safe |
| Choice-param count change repoints presets | No migration needed at v1.0.0; future `console` append ⇒ `setMigrationCallback` (O-Bitrot template) |

## 10. Open items → plan phase

1. Exact factory-preset list: names, denormalized values, alphabetical-sort/prefix strategy (§7).
2. Preset-band UI: with or without a flat click-menu (recommend: without, v1.0 — prev/next + name + save/load/delete only; a flat menu can come later the O-Bitrot way).
3. Preset-band width vs the 200px reservation — expect a small `.hdr` layout tune; confirm at the visual gate.
4. CHANGELOG initial-release wording (v1.0.0 — single section, no migration notes).
