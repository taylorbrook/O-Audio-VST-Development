---
phase: ui-frontend
reviewed: 2026-06-30T00:00:00Z
depth: deep
files_reviewed: 3
files_reviewed_list:
  - plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js
  - plugins/O-MicrotonalSampler/Resources/ui/index.html
  - plugins/O-MicrotonalSampler/Resources/ui/css/sampler-shell.css
findings:
  critical: 0
  warning: 5
  info: 7
  total: 12
status: issues_found
---

# UI Frontend: Code Review Report

**Reviewed:** 2026-06-30
**Depth:** deep (cross-checked against Source/PluginEditor.cpp + Source/PluginProcessor.cpp)
**Files Reviewed:** 3
**Status:** issues_found

## Summary

Reviewed the O-MicrotonalSampler WebView frontend (sampler-app.js ~3460 lines, index.html, sampler-shell.css) at deep depth, with cross-checks into the C++ native-function registry and APVTS parameter layout.

**JUCE bridge audit — clean.** Every `Juce.getNativeFunction(...)` and `invokeNative(...)` call in sampler-app.js maps to a registered handler in `PluginEditor::buildNativeFunctionRegistry()` (all 41 names verified present). The known "namespace vs postMessage" bug class is correctly avoided: `getNativeFunction`/`getSliderState`/`getComboBoxState` are always called on the `Juce` ES-module namespace, and the tuning panel is constructed with `new TuningPanel(container, Juce)` (line 561), not `window.__JUCE__`. `bindWebViewFileDrop` is passed `juce: Juce` (line 3426). No silent-swallow bridge gaps found.

**XSS audit — clean.** All user/backend-derived strings (filenames, skipped-file reasons, technique names, RR duplicate lists) are inserted via `textContent` or the `.title`/`.value` DOM properties, never via `innerHTML`. The only `innerHTML` writes use developer-controlled constants (`renderControlStrip`, trigger/CC tables built from numeric state).

The defects below are correctness/robustness issues. The most impactful is **WR-01**: the ADSR knob numeric readouts display values against the wrong range and ignore the parameter skew, so the on-screen number is roughly half the real value at the top of the knob.

## Warnings

### WR-01: ADSR knob readouts use wrong range + ignore skew — displayed value is incorrect

**File:** `Resources/ui/js/sampler-app.js:161-182` (KNOB_FORMATS) and `:205-217` (knobUpdateVisual)
**Issue:** The numeric readout is computed as a **linear** map of the normalised value against `KNOB_FORMATS`:
```js
const real = fmt.min + norm * (fmt.max - fmt.min);   // line 214
```
But the C++ parameters do not match either the JS range or the JS linearity:

| Param | JS KNOB_FORMATS (min/max) | C++ NormalisableRange (PluginProcessor.cpp) |
|-------|---------------------------|---------------------------------------------|
| attack  | 0.001 – **5.0** s | `(0.0, 10.0, 0.001, 0.5f)` — max 10 s, **skew 0.5** (line 105) |
| decay   | 0.001 – **5.0** s | `(0.0, 10.0, 0.001, 0.5f)` (line 113) |
| release | 0.001 – **5.0** s | `(0.0, 10.0, 0.001, 0.5f)` (line 128) |

Two compounding errors: (a) the JS max (5.0) is half the real max (10.0), and (b) the JS map is linear while the parameter is skewed (0.5). At full knob the label reads `5.00 s` while the actual attack sent to the host is `10.0 s`; at mid-knob the label reads ~`2.5 s` while the true skewed value is ~`2.5 s`-ish only by coincidence and diverges elsewhere. The audio is correct (the normalised value round-trips through the relay), but the readout misinforms the user by up to 2x. sustain/velocity_crossfade/expression/dynamic_range/output_gain all match their C++ ranges and are fine.
**Fix:** Either (a) plumb the true display value from C++ (add a native fn returning `param->getCurrentValueAsText()` or the denormalised value) and stop recomputing in JS, or (b) at minimum correct the ranges (`max: 10.0`) and apply the same skew: `real = fmt.min + Math.pow(norm, 1/0.5) * (fmt.max - fmt.min)` for the skewed params. Option (a) is the robust fix and eliminates future drift.

### WR-02: `subscribeTechniqueStateUpdates` / `subscribeTriggerStateUpdates` dereference `.backend` without guarding it

**File:** `Resources/ui/js/sampler-app.js:2797-2802` and `:3216-3221`
**Issue:** Both functions guard only `window.__JUCE__` and then immediately call `window.__JUCE__.backend.addEventListener(...)`:
```js
function subscribeTechniqueStateUpdates() {
    if (!window.__JUCE__) return;
    window.__JUCE__.backend.addEventListener('techniqueStateUpdated', () => { ... });
}
```
Every other subscriber in this file uses the stronger guard `if (!window.__JUCE__ || !window.__JUCE__.backend) return;` (e.g. `subscribeSampleMapUpdates` line 649, `bindToastEventListener` line 2026, `bindHostDragEvents` line 1956). If `__JUCE__` is populated before `.backend` is attached (boot-ordering), these two throw an uncaught `TypeError` during `DOMContentLoaded`, aborting the remainder of the init sequence that runs after them (`bindHostDragEvents`, `bindToastEventListener`, grid pull, etc. all execute earlier, but `pullTechniqueState`/`pullTriggerState` at lines 3459/3461 would still run — however the throw at 3417/3418 happens mid-`DOMContentLoaded` and stops everything queued after it).
**Fix:** Add the `.backend` guard to match the rest of the file:
```js
function subscribeTechniqueStateUpdates() {
    if (!window.__JUCE__ || !window.__JUCE__.backend) return;
    window.__JUCE__.backend.addEventListener('techniqueStateUpdated', () => { pullTechniqueState(); });
}
```
Same for `subscribeTriggerStateUpdates`.

### WR-03: Trigger CC/PC tables rebuild mid-edit and clobber in-progress input + focus

**File:** `Resources/ui/js/sampler-app.js:3223-3331` (renderTriggerPanel), called from `pullTechniqueState:2785` and `pullTriggerState:3210`
**Issue:** `renderTriggerPanel()` does `ccTbody.innerHTML = ''` then fully rebuilds all 8 rows of number inputs. It is invoked on **every** `techniqueStateUpdated` and `triggerStateUpdated` event. If the user is mid-edit in a CC-range or tech `<input>` when any technique/trigger update echoes back (e.g. the user just committed one field, firing an update that re-renders while they tab to the next), the entire tbody is torn down: the in-progress value is discarded and focus is lost. Note the trim panel (v1.23.0) explicitly solved this exact class of bug with `if (slider && document.activeElement !== slider)` (line 2927) and a static DOM — the trigger panel predates that guard and still rebuilds.
**Fix:** Mirror the trim-panel approach: build the 16 inputs once (static DOM in index.html or a one-time build), and in `renderTriggerPanel` only write `.value`/`.checked`/opacity in place, skipping any input where `document.activeElement === input`.

### WR-04: Knob double-click "reset" snaps to normalised 0.5, not the parameter default

**File:** `Resources/ui/js/sampler-app.js:302-312`
**Issue:** Double-click reset hard-codes `const mid = 0.5;` and pushes that as the normalised value. For skewed params (attack/decay/release, skew 0.5, range 0–10 s) normalised 0.5 denormalises to ~2.5 s, and for output_gain (−24..24) it lands at 0 dB. None of these is the APVTS default (attack default is a short fraction of a second). So "reset" moves attack/decay/release to multiple seconds — the opposite of a helpful default. The comment acknowledges this as a Stage-4 TODO, but it ships today as a misleading control.
**Fix:** Add a native fn returning each parameter's default normalised value (`param->getDefaultValue()`), or plumb defaults into `SLIDER_BINDINGS`, and reset to that instead of a blanket 0.5.

### WR-05: Knob wheel edits are not wrapped in a host automation gesture

**File:** `Resources/ui/js/sampler-app.js:291-299`
**Issue:** The wheel handler calls `state.setNormalisedValue(next)` directly with no surrounding `state.sliderDragStarted()` / `state.sliderDragEnded()`. The pointer-drag path (lines 286/334) and dblclick path (307/309) both wrap their writes in a gesture. Without the begin/end gesture, a DAW in automation-write mode may not record wheel tweaks as parameter changes (and some hosts coalesce or drop ungestured writes). Result: wheel adjustments silently fail to automate in some hosts.
**Fix:** Wrap each wheel tick (or a short debounced gesture) in `sliderDragStarted()` … `setNormalisedValue()` … `sliderDragEnded()`, or start a gesture on first wheel event and end it after a short idle timeout.

## Info

### IN-01: KS-range and technique-tab inputs also overwritten mid-edit

**File:** `Resources/ui/js/sampler-app.js:2859-2862`
**Issue:** `renderTechniqueBar` writes `ksLow.value` / `ksHigh.value` unconditionally on every `techniqueStateUpdated`. Lower-severity than WR-03 (only two fields, `change`-committed), but an echoed update can still overwrite a value the user is typing.
**Fix:** Skip the write when `document.activeElement` is the field being updated.

### IN-02: Octave-label comments are stale under the C3=60 convention

**File:** `Resources/ui/js/sampler-app.js:792-793, 954-959`
**Issue:** Comments state `MIDI_HIGH = 108; // C8` and "renders C1, C2, … C8". Under the intentional C3=60 convention (line 813), `midiToNoteName(108)` returns `C7` and `midiToNoteName(24)` returns `C0`, so the grid actually labels `C0…C7` and the low key (MIDI 21) reads `A-1`. The rendering is correct-by-design (it matches FilenameParser), but the comments describe the C4=60 naming and will mislead a future maintainer.
**Fix:** Update the comments to reflect the C3=60 labels actually produced (`C0…C7`, low key `A-1`).

### IN-03: `drawMarker` comment claims one-shot suppression that the code doesn't do

**File:** `Resources/ui/js/sampler-app.js:2295-2297`
**Issue:** Comment says "only draw if not one-shot", but both markers are always drawn. For a one-shot cell (loopStart==loopEnd==0) both markers render stacked at x=0. Cosmetic only.
**Fix:** Either honor the comment (skip marker draw when `isOneShot(editorState.snap)`) or delete the stale comment.

### IN-04: Modal Esc handler bubbles into the loop-editor Esc handler

**File:** `Resources/ui/js/sampler-app.js:112-122` (bindModal onKey, capture phase) vs `:2394-2399` (loop-editor Esc, bubble phase)
**Issue:** `bindModal`'s keydown listener is on the capture phase and calls `e.preventDefault()` but not `e.stopPropagation()`. The loop-editor's document-level Esc listener (bubble phase) then also fires. If a modal (e.g. per-cell merge, batch loop) is opened while the loop editor panel is open, pressing Esc to dismiss the modal will also close the loop editor. Narrow edge case.
**Fix:** Call `e.stopPropagation()` (or `stopImmediatePropagation()`) in `bindModal`'s onKey once it has matched and dispatched a key target.

### IN-05: `window.confirm` fallbacks are dead in WKWebView

**File:** `Resources/ui/js/sampler-app.js:1785` (showEmbedSizeConfirmModal) and `:2658` (showAmbiguousDuplicatesDialog)
**Issue:** Both fall back to `window.confirm(...)` when their modal DOM is missing. Per this file's own comments (index.html line 296, sampler-app.js line 1546), WKWebView does not wire `window.confirm` through the UIDelegate, so the fallback returns `false`/`undefined` silently — the ambiguous-duplicate path would then send a "cancel" RR confirmation the user never saw. The modal elements are always present in index.html, so this only triggers on a markup regression, but the fallback gives false assurance.
**Fix:** On missing modal DOM, log an error and fail safe explicitly (e.g. abort the load and toast) rather than routing through a `window.confirm` that can't work in this host.

### IN-06: `openLoopEditor` parameter named `vel` actually carries the velocity *layer*

**File:** `Resources/ui/js/sampler-app.js:2084` (and `editorState.vel`)
**Issue:** Callers pass the layer index (`openLoopEditor(midi, layer)` from `handleCellSingleClick:1082`), but the param and `editorState.vel` are named `vel`, which reads as a MIDI velocity. It's internally consistent (C++ expects the layer), so no functional bug — purely a naming trap for maintainers, especially given the file also uses real velocity ranges elsewhere.
**Fix:** Rename to `layer` / `editorState.layer` for clarity.

### IN-07: `renderControlStrip` tooltip interpolation escapes only double-quotes

**File:** `Resources/ui/js/sampler-app.js:361`
**Issue:** `title="${b.tooltip.replace(/"/g, '&quot;')}"` is injected via `innerHTML`. The tooltip strings are developer-controlled constants, so there is no live XSS vector, but the pattern escapes only `"` and would break (or inject) if a tooltip ever contained `&`, `<`, or a template with dynamic content. Defensive note, not an active vulnerability.
**Fix:** Render the knob DOM with `createElement` + `el.title = b.tooltip` (property assignment, no escaping needed), consistent with how the rest of the file builds dynamic DOM.

## Native functions called by this file (all verified registered in PluginEditor.cpp)

`getSampleMap`, `getTuningName`, `getPluginVersion`, `getHeldNotesJson`, `loadSingleSampleDialog`, `reportCellLayout`, `pickSampleFolder`, `estimateFolderAudioSize`, `loadSampleFolderByPath`, `getWaveformPeaks`, `resetLoopToAutoDetect`, `overrideLoopPoints`, `applyLoopPointsToAll`, `saveCurrentPreset`, `loadPreset`, `locateMissingFolder`, `getPendingMissingFolder`, `dismissMissingFolder`, `getTechniqueState`, `setActiveTechnique`, `addTechniqueSlot`, `removeTechniqueSlot`, `setTechniqueName`, `applyTechniqueNames`, `setKeyswitchEnabled`, `setKeyswitchRange`, `deleteSampleCell`, `clearVelocityLayer`, `clearSampleMap`, `confirmRoundRobinLoad`, `handleWebViewFileDrop`, `getTriggerState`, `setCcEnabled`, `setCcNumber`, `setPcEnabled`, `setCcMapping`, `setPcMapping`, `resetTriggerMappings`, `setTechniqueTrim`, `setLayerTrim`, `resetTrims`. Plus `getSliderState` (9 relays) and `getComboBoxState('dynamics_mode')`. No unregistered/dead native calls found. (`resetTechniqueNames` is registered in C++ but not called from this file — harmless unused export, out of scope here.)

---

_Reviewed: 2026-06-30_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
