# Stage 4 (Polish) — PLAN

**Goal:** Ship O-simpleFM v1.0.0 release-ready: suite-canonical preset manager (full, with
browser panel) + Lesson tour kept; aliasing/edge-case validation; deferred critic fixes; changelog.

## Tasks

### T1 — Preset manager: C++ backend
- T1.1 Copy `modules/persistence/preset-manager/js/preset-manager.js` → `Source/ui/public/modules/`.
- T1.2 `Source/FactoryPresets.{h,cpp}`: `build(apvts)` returns 6 `FactoryPresetDef` — `Default`
  (per-param `getDefaultValue()`), `E-Piano`, `Tubular Bell`, `Brass`, `Clarinet`, `Clang Bell`
  (scaled snapshots from app.js PRESETS, normalized via `convertTo0to1`).
- T1.3 `PluginProcessor.h`: `#include "OuariconPresetManager.h"`, `OuariconPresetManager presetManager;`
  member + `getPresetManager()` getter.
- T1.4 `PluginProcessor.cpp`: init `presetManager(parameters, "O-simpleFM")`; in ctor call
  `presetManager.initializeFactoryPresets(FactoryPresets::build(parameters))`; route
  `getStateInformation`/`setStateInformation` through `getStateAsXml`/`setStateFromXml`.
- T1.5 `CMakeLists.txt`: `ouaricon_add_module(O-simpleFM preset-manager)` before binary data; add
  `Source/FactoryPresets.cpp/.h` to sources; add `Source/ui/public/modules/preset-manager.js` to binary data.

### T2 — Preset manager: WebView wiring
- T2.1 `PluginEditor.h`: `std::unique_ptr<juce::FileChooser> fileChooser;`.
- T2.2 `PluginEditor.cpp`: resource provider serves `/modules/preset-manager.js`
  (`BinaryData::presetmanager_js`) + add `; charset=utf-8` to text/html, text/css, js MIME.
- T2.3 `PluginEditor.cpp`: 10 preset native functions → `processorRef.getPresetManager()`.
- T2.4 `index.html`: preset bar in header (name button → dropdown, prev/next, save, delete) +
  `<div class="preset-dropdown">` listbox.
- T2.5 `app.js`: import `PresetManager`, instantiate with `Juce.getNativeFunction`, build the
  factory/user dropdown from `getPresetList()`+`isFactoryPreset()`, wire load/save/delete/nav.
- T2.6 `styles.css`: preset-bar + dropdown styles (README z-index `:has()` escape).

### T3 — Deferred Stage-3 critic fixes
- T3.1 Keyboard tooltips: `tabindex="0"` on `[data-tip]`, focus/blur show/hide, Escape-to-hide,
  ArrowUp/Down knob nudge (knobs now focusable).
- T3.2 Fleuron/♪ symbol-font fallback stack in CSS.
- T3.3 (MIME charset done in T2.2.)

### T4 — Aliasing audit + edge cases
- T4.1 Extend render harness: extreme aliasing (note 96, max index, max feedback → finite/bounded,
  aliasing budget) + fixed-mode high-Hz case.
- T4.2 pluginval `--strictness-level 10` (covers sample-rate/buffer/state matrices).

### T5 — Build / validate / release
- T5.1 Build VST3 + AU + Standalone + render harness (`-DOUARICON_BUILD_TESTS=ON`); zero warnings.
- T5.2 Cache-clear + dual-variant sweep + install (per CLAUDE.md / build-and-install.sh).
- T5.3 `auval -v aumu OSiF OuDv` → SUCCEEDED; render harness all-pass; pluginval s10 pass.
- T5.4 `CHANGELOG.md` v1.0.0; confirm version string.
- T5.5 Standalone headless launch → zero WebKit/console/ReferenceError; preset round-trip in JSON dir.

## Success criteria (goal-backward)
- [ ] 6 factory JSON presets written to `~/Library/O-simpleFM/Presets/Factory/`; load + round-trip.
- [ ] Browser panel: list shows factory + user; prev/next/save/delete work; Lesson tour still works.
- [ ] Preset load updates knobs (relay propagation) with no zipper / no dead controls.
- [ ] auval SUCCEEDED; pluginval s10 PASS; render harness all-pass incl. new aliasing scenarios.
- [ ] Tooltips reachable by keyboard (focus) + Escape; fleuron renders with fallback; UTF-8 charset served.
- [ ] No regression to the 17 param bindings, viz emit, routing diagram, or member order.
- [ ] CHANGELOG.md present; version 1.0.0.

## Risk / rollback
- Highest risk: header layout shift from the preset bar (regression-sensitive UI). Mitigate by
  placing the bar in the existing header row; verify `setSize(760,720)` still fits. Each file edit
  is reversible; render harness + auval gate every build.
