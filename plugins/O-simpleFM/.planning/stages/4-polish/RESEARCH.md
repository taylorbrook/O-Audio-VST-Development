# Stage 4 (Polish) — RESEARCH

**Date:** 2026-06-20 · Sources: live codebase (O-AnalogEQ, O-Prism), `modules/persistence/preset-manager`.

## Canonical preset pattern (from suite scan)
- **Mechanism:** shared C++ `OuariconPresetManager` (header-only, inline) + on-disk JSON in
  `~/Library/{PluginName}/Presets/{Factory,User}/*.json`. Surfaced through the WebView, never
  the DAW host program menu. No `.vstpreset`/`.aupreset` files anywhere in the suite.
- **Shared module** `modules/persistence/preset-manager/` (flat `Factory/*.json`) is the version
  used via `ouaricon_add_module` by O-AnalogEQ, O-Chorus, O-Detune, O-Tremolo, O-SimpleReverb.
  O-Prism vendored a **newer** category-subdir variant — overkill for 5 flat presets, NOT used here.
- **Reference to copy: O-AnalogEQ** (cleanest shared-module integration):
  - `CMakeLists.txt`: `ouaricon_add_module(<target> preset-manager)` BEFORE `juce_add_binary_data`;
    add `Source/ui/public/modules/preset-manager.js` to binary data.
  - `ouaricon_add_module` GLOBs module `cpp/*.h` (adds include dir → `#include "OuariconPresetManager.h"`)
    and `configure_file(COPYONLY)` copies `js/preset-manager.js` → `Source/ui/public/modules/`.
  - Processor: member `presetManager(parameters, "<Name>")`; build `std::vector<FactoryPresetDef>`
    (normalized [0,1] values); `presetManager.initializeFactoryPresets(...)` **unconditional** in ctor
    (file I/O during auval is fine in practice — O-AnalogEQ ships it). `getStateInformation` →
    `presetManager.getStateAsXml()`; `setStateInformation` → `presetManager.setStateFromXml()`.
  - Editor: 10 `.withNativeFunction(...)` (`savePreset`, `savePresetWithDialog`, `loadPreset`,
    `loadPresetFromFile`, `getPresetList`, `getCurrentPreset`, `selectNextPreset`,
    `selectPreviousPreset`, `deletePreset`, `isFactoryPreset`) → `processorRef.getPresetManager()`.
    `savePresetWithDialog`/`loadPresetFromFile` need a `std::unique_ptr<juce::FileChooser>` member.
    Resource provider serves `/modules/preset-manager.js` → `BinaryData::presetmanager_js`.

## JS UI module (`preset-manager.js`)
- `class PresetManager({displayElement, prevButton, nextButton, saveButton, loadButton,
  deleteButton, getNativeFunction, onPresetChanged, onPresetListUpdated})`.
- `getNativeFunction` MUST be `Juce.getNativeFunction` (ES-module namespace). It waits on
  `window.__JUCE__.backend` then resolves the 10 native fns. Methods: `initialize()`, `refresh()`,
  `loadPreset(name)`, `selectNext/Previous()`, `saveWithDialog()`, `loadFromFile()`,
  `deletePreset(name)`, `getPresetList()`. No built-in dropdown — build one from
  `getPresetList()` + `onPresetListUpdated`. README ships dropdown CSS + a `:has()` z-index escape.
- Import path from `/js/app.js`: `../modules/preset-manager.js`.

## FactoryPreset normalization
- `FactoryPresetDef.parameters` are **normalized** (manager calls `param->getValue()` to save,
  `setValueNotifyingHost(value)` to load). Build via `apvts.getParameter(id)->convertTo0to1(scaled)`
  (O-Prism pattern). "Default" preset uses `getDefaultValue()` (already normalized) per param.

## Deferred Stage-3 critic notes (from 3-gui VERIFICATION.md)
- Pointer-only tooltips → add `focus`/`blur` triggers + `tabindex` on `[data-tip]` + Escape-to-hide
  (and ArrowUp/Down knob nudge, since knobs become focusable).
- Fleuron `❦`/`♪` may miss on Windows default serif → add a symbol-font fallback stack.
- Resource provider returns bare MIME types → append `; charset=utf-8` to text/html, text/css, js.

## Aliasing / edge-case validation
- DSP unchanged: 2× `filterHalfBandPolyphaseIIR` always-on + key-tracked Carson index ceiling
  (sine-only v1.0). Harness already proves index→sidebands, carrier-null, feedback-stable.
- Add harness scenarios: extreme aliasing case (high note + max index + max feedback → bounded,
  finite, no spectral energy folding above a budget) and a fixed-mode high-Hz case. Validate
  pluginval s10 covers sample-rate/buffer/state matrices.
