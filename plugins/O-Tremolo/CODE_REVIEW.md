---
phase: O-Tremolo-v1.4.7
reviewed: 2026-07-07T23:06:51Z
depth: deep
files_reviewed: 8
files_reviewed_list:
  - plugins/O-Tremolo/Source/PluginProcessor.cpp
  - plugins/O-Tremolo/Source/PluginProcessor.h
  - plugins/O-Tremolo/Source/PluginEditor.cpp
  - plugins/O-Tremolo/Source/PluginEditor.h
  - plugins/O-Tremolo/Source/ui/public/index.html
  - plugins/O-Tremolo/Source/ui/public/modules/preset-manager.js
  - plugins/O-Tremolo/CMakeLists.txt
  - modules/persistence/preset-manager/cpp/OuariconPresetManager.h
findings:
  critical: 1
  warning: 2
  info: 5
  total: 8
status: issues_found
---

# O-Tremolo v1.4.7: Code Review Report

**Reviewed:** 2026-07-07T23:06:51Z
**Depth:** deep
**Files Reviewed:** 8
**Status:** issues_found

## Summary

Cross-module review of the O-Tremolo JUCE 8 WebView plugin as shipped (v1.4.7), plus the referenced C++ preset-manager module header. The recurring high-value failure modes are mostly handled well:

- **Windows WebView2 static linking:** `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` IS present alongside `NEEDS_WEBVIEW2 TRUE` (CMakeLists.txt:15, 64). Correct — no blank-UI-on-Windows regression.
- **Native-function bridge:** JS `getNativeFunction` set (11) and C++ `withNativeFunction` set (11) match exactly. No silent dead controls.
- **Preset name sanitization:** `sanitizePresetName()` strips `/ \ :` consistently (OuariconPresetManager.h:193). The "/" filename regression is handled.
- **applyPresetJson reset-to-defaults:** Present and correct — all params reset to default before applying (OuariconPresetManager.h:292-294).
- **State round-trip:** getStateInformation/setStateInformation round-trip all 6 APVTS params via `copyState`/`replaceState`; setState is null/parse-safe.
- **processBlock RT-safety:** No heap alloc, no locking; params read via cached atomic pointers; `ScopedNoDenormals` present; `juce::Random::nextFloat` is RT-safe.

The one blocker is a use-after-free in the async FileChooser completions (the codebase's documented "WebView launchAsync SafePointer" pattern is not applied here). The two warnings are missing parameter smoothing (zipper noise) and hardcoded JS parameter ranges.

## Critical Issues

### CR-01: FileChooser `launchAsync` completions capture raw `this` and call `complete()` on teardown → use-after-free

**File:** `plugins/O-Tremolo/Source/PluginEditor.cpp:50-84` (savePresetWithDialog) and `:124-155` (loadPresetFromFile)
**Issue:** Both async file-dialog completions capture `[this, complete]` with no lifetime guard. If the editor is destroyed while the native Save/Load dialog is open (close the plugin window, switch presets in the host, remove the track), the completion fires against a destroyed editor: `processorRef.presetManager…` dereferences freed memory, and `complete(...)` invokes a callback owned by the already-destroyed `WebBrowserComponent` Impl. Both dialogs are reachable from the UI (the Load and Save buttons in index.html), so this is a live crash path, not theoretical. This is exactly the recurring failure documented for this codebase (`pattern_webview_launchasync_safepointer_no_complete` — shipped as a fix across other WebView editors; audit item). Note the codebase-specific subtlety: on the teardown path you must **not** call `complete(false)` — that call is itself a UAF because the completion object is owned by the dead WebView. Bail with a bare `return`.
**Fix:**
```cpp
// Capture a SafePointer to the editor; bail with a bare return on teardown.
juce::Component::SafePointer<OuariconTremoloAudioProcessorEditor> safeThis(this);
fileChooser->launchAsync(
    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
    [safeThis, complete](const juce::FileChooser& fc) {
        if (safeThis == nullptr)
            return;                      // editor gone — do NOT call complete()
        auto results = fc.getResults();
        if (results.isEmpty()) {
            auto* result = new juce::DynamicObject();
            result->setProperty("success", false);
            result->setProperty("name", "");
            complete(juce::var(result));
            return;
        }
        auto file = results.getFirst();
        auto presetName = file.getFileNameWithoutExtension();
        bool success = safeThis->processorRef.presetManager.savePreset(presetName);
        auto* result = new juce::DynamicObject();
        result->setProperty("success", success);
        result->setProperty("name", success ? presetName : juce::String());
        complete(juce::var(result));
    });
```
Apply the same SafePointer guard + bare-return to the `loadPresetFromFile` completion at :133-154.

## Warnings

### WR-01: No parameter smoothing on Depth or Rate → zipper noise / clicks

**File:** `plugins/O-Tremolo/Source/PluginProcessor.cpp:233-234, 276, 327-328, 359`
**Issue:** `depth` and `speedHz` are `load()`ed once per block and applied directly. `depth` feeds the per-sample gain (`1 - lfoValue*depth`), so any depth change between blocks (automation, preset recall, UI drag) produces a step in the gain envelope → audible zipper/click. `lfoPhaseIncrement` likewise steps per block. The architecture doc explicitly calls for parameter smoothing ("Use AudioParameterFloat with smoothing enabled to avoid zipper noise", architecture.md:170, 206) and this is not implemented. Depth is the most audible offender because it directly scales output amplitude every sample.
**Fix:** Wrap depth (and ideally the phase increment / rate) in `juce::SmoothedValue<float>`:
```cpp
// members
juce::SmoothedValue<float> depthSmoothed;
// prepareToPlay
depthSmoothed.reset(sampleRate, 0.02); // 20 ms ramp
depthSmoothed.setCurrentAndTargetValue(depthParam->load() / 100.0f);
// processBlock
depthSmoothed.setTargetValue(depthParam->load() / 100.0f);
// inside the per-sample loop:
const float depth = depthSmoothed.getNextValue();
```

### WR-02: Hardcoded JS parameter ranges drift from the C++ NormalisableRange

**File:** `plugins/O-Tremolo/Source/ui/public/index.html:723-724, 951`
**Issue:** Knob display maps normalized→real with hardcoded literals — `setupKnob('speed', speedState, 0.1, 20, ' Hz')`, `setupKnob('depth', depthState, 0, 100, '%')`, and again in the tempo-sync toggle handler `const realValue = 0.1 + (normValue * (20 - 0.1))` (:951). These duplicate the C++ `NormalisableRange` values by hand. If the DSP range or skew ever changes (e.g. speed given a skew, or the range widened), the readout silently shows wrong numbers with no build error — the exact drift class documented in `pattern_webview_knob_readout_scaled_value`. JUCE pushes the real range/skew to the WebView via the slider state's `properties`.
**Fix:** Read the scaled value from the JUCE slider state instead of remapping in JS, e.g. `state.getScaledValue()` (or read `state.properties.start/end/skew` and apply the same mapping JUCE uses), and drop the per-control min/max literals. This keeps the display authoritative against the C++ range and honors any future skew. Also remove the duplicated remap in the tempo toggle handler and reuse the knob's `updateVisual`.

## Info

### IN-01: Async delete path is inert in this plugin (uncommitted change has no wired caller)

**File:** `plugins/O-Tremolo/Source/ui/public/modules/preset-manager.js:118, 333-361` vs `index.html:743-757`
**Issue:** The working-tree change (async `promptDelete`, `onConfirmDelete` hook) is internally consistent — the only caller is the fire-and-forget `deleteButton` click listener, and `promptDelete` swallows its own errors (no unhandled rejection, fail-safe abort reached when no confirm mechanism exists). However, O-Tremolo's `index.html` constructs `PresetManager` without `deleteButton`, `menuButton`, or `onConfirmDelete`, and the dropdown UI only loads presets. So `promptDelete`/`deletePreset` and the C++ `deletePreset` native fn are unreachable from the UI — users cannot delete user presets at all. The new async logic is correct but dead in this plugin.
**Fix:** Either wire a delete affordance (pass `deleteButton`/`onConfirmDelete`, or add a delete control in the dropdown per preset), or note that delete is intentionally omitted for O-Tremolo.

### IN-02: Preset dropdown labels every preset "Factory"

**File:** `plugins/O-Tremolo/Source/ui/public/index.html:782-801`
**Issue:** The dropdown adds a single "Factory" header and applies `class="preset-dropdown-item factory"` (italic) to every entry, including user presets. The `isFactoryPreset` native function exists but the dropdown never calls it to distinguish user vs factory. User presets are visually misrepresented and get no delete affordance.
**Fix:** Query `isFactoryPreset(name)` (or split the list) and render user presets under a separate "User" header without the `factory` class.

### IN-03: Visualizer smoothing carries stale state across redraws; dead locals in noise case

**File:** `plugins/O-Tremolo/Source/ui/public/index.html:1056, 1133-1138, 1185-1188`
**Issue:** `smoothedY` is module-scoped and persists between `drawWaveform()` calls, so the first sample of each redraw smooths against the previous draw's last value (minor visual artifact). In the `noise` case, `const phase = (t * 4) % 1;` (:1135) is computed but never used. This is cosmetic (visualizer only), not audio.
**Fix:** Reset `let smoothedY = 0;` at the top of `drawWaveform()` and remove the unused `phase` local.

### IN-04: UI tempo-sync readout assumes 120 BPM

**File:** `plugins/O-Tremolo/Source/ui/public/index.html:830-847, 875-878`
**Issue:** When Tempo Sync is ON the knob shows a musical division computed by `getMusicalDivisionName(hz, bpm = 120)` with a hardcoded 120 BPM fallback (the WebView has no access to host tempo). At other tempos the displayed division can disagree with the division the DSP actually locks to (which uses the real host BPM in processBlock). Display-only inaccuracy.
**Fix:** Push the host BPM to the UI via a native function / relay property and pass it into `getMusicalDivisionName`, or display the resolved division the C++ side computed.

### IN-05: Waveform (a Choice param) is bound via WebSliderRelay, deviating from the parameter spec

**File:** `plugins/O-Tremolo/Source/PluginEditor.cpp:20, 163-164` and `index.html:717, 979-1009`
**Issue:** `WAVEFORM_PARAM` is an `AudioParameterChoice`, but it is wired with a `WebSliderRelay` + `getSliderState('waveform')` + `Math.round(norm*5)` index math, rather than the `WebComboBoxRelay` / `getComboBoxState` the parameter-spec.md (lines 135, 172) prescribes. It works correctly today because a 6-item choice normalizes to `index/5`, but it is fragile (adding/removing a waveform silently breaks the `*5` factor) and diverges from the documented contract.
**Fix:** Switch to `WebComboBoxRelay("waveform")` + `getComboBoxState('waveform').getChoiceIndex()/setChoiceIndex()`, or update parameter-spec.md to document the slider-relay approach and derive the index divisor from the option count instead of the literal `5`.

---

_Reviewed: 2026-07-07T23:06:51Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
