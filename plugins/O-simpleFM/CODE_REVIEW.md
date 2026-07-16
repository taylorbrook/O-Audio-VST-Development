---
phase: O-simpleFM-v1.2.1-full-review
reviewed: 2026-07-15T00:00:00Z
depth: standard
files_reviewed: 16
files_reviewed_list:
  - plugins/O-simpleFM/CMakeLists.txt
  - plugins/O-simpleFM/Source/FactoryPresets.cpp
  - plugins/O-simpleFM/Source/FactoryPresets.h
  - plugins/O-simpleFM/Source/FmVizAnalyzer.h
  - plugins/O-simpleFM/Source/FMVoice.h
  - plugins/O-simpleFM/Source/Operator.h
  - plugins/O-simpleFM/Source/PluginEditor.cpp
  - plugins/O-simpleFM/Source/PluginEditor.h
  - plugins/O-simpleFM/Source/PluginProcessor.cpp
  - plugins/O-simpleFM/Source/PluginProcessor.h
  - plugins/O-simpleFM/Source/ui/public/index.html
  - plugins/O-simpleFM/Source/ui/public/css/styles.css
  - plugins/O-simpleFM/Source/ui/public/js/app.js
  - plugins/O-simpleFM/Source/ui/public/modules/preset-manager.js
  - plugins/O-simpleFM/tests/render-harness/CMakeLists.txt
  - plugins/O-simpleFM/tests/render-harness/main.cpp
findings:
  critical: 1
  warning: 6
  info: 4
  total: 11
status: issues_found
---

# O-simpleFM v1.2.1: Code Review Report

**Reviewed:** 2026-07-15
**Depth:** standard
**Files Reviewed:** 16
**Status:** issues_found

## Summary

Full-plugin review of the pedagogical 2-operator FM synth (JUCE 8.0.9, VST3/AU,
WebView UI). The DSP core is solid: `processBlock` and everything it calls is
allocation/lock-free (voices pre-allocated, viz ring is copy-only atomics, FFT
runs on the editor Timer), `ScopedNoDenormals` is present, NaN is killed at the
feedback source AND scrubbed at the output (no sticky-silence pattern — there
are no IIR filters holding state besides the oversampler, which is
JUCE-managed), `setLatencySamples` is used correctly, and the sine LUT phase is
floor-wrapped before lookup as the architecture requires.

Suite-recurring defect patterns explicitly checked and CLEAR:
- **Native-fn bridge:** all 12 JS `getNativeFunction` names (`savePreset`,
  `savePresetWithDialog`, `loadPreset`, `loadPresetFromFile`, `getPresetList`,
  `getCurrentPreset`, `selectNextPreset`, `selectPreviousPreset`,
  `deletePreset`, `isFactoryPreset`, `uiMidi`, `getSampleRate`) have matching
  C++ `withNativeFunction` registrations. No gaps.
- **Knob readouts** use `SliderState.getScaledValue()` — no hardcoded JS
  min/max maps. `PresetManager` is passed `Juce.getNativeFunction` (ES-module
  namespace), not `window.__JUCE__`.
- **Resource provider** does direct bare-path equality — no `://` stripping.
- **Factory presets** authored in raw engineering units + `convertTo0to1`
  (skew-safe); "Default" derives from `getDefaultValue()`. The vendored
  preset-manager module is v1.0.4: `applyPresetJson` resets ALL params to
  defaults before applying, and `sanitizePresetName` handles the "/" filename
  hazard.
- **CMake:** `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
  correctly paired; single `juce_add_binary_data` target (no BinaryData
  namespace collision); Windows `withUserDataFolder` set.
- **Render harness** compiles `PluginEditor.cpp` WITH `JUCE_WEB_BROWSER=1` and
  links the UI BinaryData target — the known harness/WebView breakage is
  handled.

One Critical remains: the two async `FileChooser::launchAsync` completions in
the editor capture raw `this` + the WebView-owned `complete` callback — the
exact UAF pattern already fixed suite-wide in O-MicrotonalSampler v1.23.5
(W12). Warnings are concentrated in the on-screen keyboard's stuck-note paths,
preset-UX data-loss/mismatch behavior, and a harness version-string mismatch
that mutates the user's real factory-preset directory.

## Critical Issues

### CR-01: `launchAsync` completions capture raw `this` and the WebView-owned `complete` callback — use-after-free on editor teardown

**File:** `plugins/O-simpleFM/Source/PluginEditor.cpp:106-125` (savePresetWithDialog) and `:132-151` (loadPresetFromFile)
**Issue:** Both native-fn handlers do:

```cpp
fileChooser->launchAsync (…, [this, complete] (const juce::FileChooser& fc) {
    …
    bool ok = processorRef.getPresetManager().savePreset (name);
    …
    complete (juce::var (result));
});
```

If the host destroys the editor while the native dialog is open (user closes
the plugin window mid-dialog — trivially reachable), the completion can fire
against a dead editor: `this`/`processorRef` access is a use-after-free, and
`complete` is owned by the destroyed WebView `Impl`, so even invoking
`complete(...)` on the teardown path is itself a UAF. This is the exact defect
class shipped as W12 in O-MicrotonalSampler v1.23.5 with a suite-wide audit
directive for all WebView editors
(`pattern_webview_launchasync_safepointer_no_complete`).
**Fix:** Capture a `Component::SafePointer` and bail with a **bare return**
(do NOT call `complete(false)` — the review-standard "call complete on every
path" is the UAF here):

```cpp
.withNativeFunction ("savePresetWithDialog", [this] (auto&, auto complete) {
    fileChooser = std::make_unique<juce::FileChooser> (…);
    juce::Component::SafePointer<OSimpleFMAudioProcessorEditor> safeThis (this);
    fileChooser->launchAsync (flags,
        [safeThis, complete] (const juce::FileChooser& fc) {
            if (safeThis == nullptr)
                return;                      // editor gone — complete is dead too; bare return
            auto* result = new juce::DynamicObject();
            …
            bool ok = safeThis->processorRef.getPresetManager().savePreset (name);
            …
            complete (juce::var (result));
        });
})
```

Apply identically to `loadPresetFromFile`.

## Warnings

### WR-01: On-screen keyboard leaks stuck notes on pointer-release-outside and window blur

**File:** `plugins/O-simpleFM/Source/ui/public/js/app.js:657-671` (pointer), `:674-684` (QWERTY)
**Issue:** Mouse: `pointerdown` on a key starts a note, but release is only
handled by `window`'s `pointerup`. For mouse pointers there is no implicit
pointer capture, so dragging off the plugin window and releasing there never
delivers `pointerup` → the note sustains until the next click. QWERTY: a held
letter's `keyup` is lost if the WebView loses focus mid-hold (click into the
DAW, Cmd-Tab) → permanently held note. No panic/all-notes-off path exists.
**Fix:** On `pointerdown`, call `kb.setPointerCapture(e.pointerId)` (then
`pointerup`/`pointercancel` are guaranteed to the capturer), and add a blur
sweep:

```js
function allNotesOff() {
  [...heldNotes].forEach(noteOff);
  if (pointerNote != null) { noteOff(pointerNote); pointerNote = null; }
}
window.addEventListener("blur", allNotesOff);
kb.addEventListener("pointercancel", () => { if (pointerNote != null) { noteOff(pointerNote); pointerNote = null; } });
```

### WR-02: Global QWERTY note handler fires regardless of focus target

**File:** `plugins/O-simpleFM/Source/ui/public/js/app.js:674-684`
**Issue:** The `keydown` listener maps a–k / w–u to notes with no check of
`e.target`. Every knob, toggle, preset-bar button, and dropdown item is
focusable (`tabindex=0`); keyboard users navigating the preset dropdown or
focused on a toggle trigger synth notes when pressing letters (e.g. "s" while
a dropdown item has focus plays D4 and `preventDefault`s the key). Any future
text input (rename field) would be unusable.
**Fix:** Bail when focus is on an interactive control:

```js
window.addEventListener("keydown", (e) => {
  if (e.repeat || e.metaKey || e.ctrlKey || e.altKey) return;
  const t = e.target;
  if (t.closest && t.closest("input, textarea, [contenteditable], .preset-bar, .preset-dropdown")) return;
  …
});
```

### WR-03: Delete button destroys the current user preset with zero confirmation

**File:** `plugins/O-simpleFM/Source/ui/public/js/app.js:399-403`
**Issue:** The header Delete button calls
`presetManager.deletePreset(presetManager.getCurrentPreset())` directly. The
vendored module provides `promptDelete()` (with an `onConfirmDelete` hook)
precisely because this is destructive and irreversible — one misclick
permanently removes the user's preset file. The button sits 30 px from Save.
**Fix:** Route through the confirmation path:

```js
presetManager.onConfirmDelete = (name, msg) => confirmInDom(msg); // small in-DOM dialog
delBtn.addEventListener("click", async () => { await presetManager.promptDelete(); closeDropdown(); });
```

(`window.confirm` is unreliable in JUCE WebViews per the module's own docs —
supply the in-DOM hook.)

### WR-04: `savePresetWithDialog` silently discards the folder the user chose in the Save dialog

**File:** `plugins/O-simpleFM/Source/PluginEditor.cpp:106-124`
**Issue:** The handler extracts only `getFileNameWithoutExtension()` from the
dialog result and calls `savePreset(name)`, which always writes to
`…/Presets/User/`. If the user navigates to Desktop and saves "MySound.json",
no file appears there — it lands in the User presets folder with no
indication. Additionally, choosing a factory preset's name makes
`savePreset` return false (factory-overwrite guard) and the UI shows nothing.
**Fix:** The module already supports arbitrary paths — honor the choice:

```cpp
auto file = results.getFirst();
bool ok = processorRef.getPresetManager().savePresetToFile (file);
// then refresh list; report name = file.getFileNameWithoutExtension()
```

or, if the User folder is intentionally the only destination, lock the dialog
to it and surface the factory-name rejection to the page (the JS already
displays `{success:false}` silently — add a visible failure state).

### WR-05: Render harness stamps `JucePlugin_VersionString="1.0.0"` and rewrites the user's REAL factory-preset directory

**File:** `plugins/O-simpleFM/tests/render-harness/CMakeLists.txt:49` (with `plugins/O-simpleFM/CMakeLists.txt:14` at VERSION 1.2.1)
**Issue:** The harness constructs the real `OSimpleFMAudioProcessor`, whose
constructor calls `initializeFactoryPresets()`. The module's v1.0.4
`.factory-version` sentinel compares against `JucePlugin_VersionString` —
hardcoded here as "1.0.0" while the plugin is 1.2.1. Every harness run
therefore (a) rewrites the user's actual
`~/Library/O-simpleFM/Presets/Factory/*.json` stamped `version: 1.0.0`, and
(b) flips the sentinel so the next real plugin instantiation rewrites them all
again — the sentinel's whole purpose (WR-04 in the module changelog) is
defeated, permanently, for any machine that runs the test.
**Fix:** Derive the macro from the plugin version instead of a literal:

```cmake
JucePlugin_VersionString="1.2.1"   # minimum: keep in lockstep with juce_add_plugin VERSION
```

better: read one `set(OSIMPLEFM_VERSION 1.2.1)` variable in both files, or
point the harness processor at a temp preset root so a DSP test never touches
user data.

### WR-06: Spectrum frequency axis and FM sideband markers use a boot-time Nyquist that never refreshes

**File:** `plugins/O-simpleFM/Source/ui/public/js/app.js:433, 688-694` (with `drawSidebandMarkers` at `:490-533`)
**Issue:** `nyquistHz` is fetched exactly once in `boot()` via
`getSampleRate`. `FmVizAnalyzer` maps its 256 bins against the *live*
`getCurrentSampleRate()` every frame. If the editor opens before the first
`prepareToPlay` (fetch returns the stale 44100 default) or the host changes
sample rate while the editor is open (44.1k → 96k), the JS log-axis and the
fc/sideband marker positions are computed against the wrong Nyquist while the
bars use the right one — the teaching overlay (the plugin's headline feature)
draws markers that visibly miss the actual peaks.
**Fix:** Cheapest correct option: piggyback the rate on the existing 30 Hz
push — emit `sampleRateUpdate` from `timerCallback` when it changes, or have
the `carrierUpdate` handler re-invoke `fetchSampleRate()` when a note starts.

## Info

### IN-01: The index taper/range constants (20, ^1.7) are duplicated in three places

**File:** `plugins/O-simpleFM/Source/FMVoice.h:79`, `plugins/O-simpleFM/Source/PluginProcessor.cpp:196`, `plugins/O-simpleFM/Source/ui/public/js/app.js:233`
**Issue:** The DSP taper `baseIndex = 20·(I/20)^1.7` lives in `FMVoice`, the
`/20.0f` re-normalization in `pushParamsToVoices` hardcodes the param range
max, and the carrier-null badge in JS re-implements the full formula. A future
range or taper change silently desynchronizes the badge (and the harness'
carrier-null test).
**Fix:** Define `kIndexMax = 20.0f` / `kIndexTaper = 1.7f` once in
`ParamIDs`-adjacent constants; derive the JS copy's values via
`propertiesChanged`/a comment pointing at the single source, or push the
effective index from C++.

### IN-02: `handleUiMidi` does not range-check the note number from JS

**File:** `plugins/O-simpleFM/Source/PluginProcessor.cpp:211-218` (called from `PluginEditor.cpp:180-185`)
**Issue:** `noteNumber` arrives from the WebView as `(int) args[0]` and goes
straight into `juce::MidiMessage::noteOn`, which jasserts (debug) on values
outside 0–127 and would build malformed MIDI in release. The JS currently
guards 0–127, but the native boundary should not trust the page.
**Fix:** `noteNumber = juce::jlimit (0, 127, noteNumber);` at the top of
`handleUiMidi` (velocity is already clamped).

### IN-03: `scaledMidi` can heap-grow on the audio thread under a dense MIDI burst

**File:** `plugins/O-simpleFM/Source/PluginProcessor.cpp:152, 265-270`
**Issue:** `scaledMidi.ensureSize (4096)` pre-allocates ~4 KB; a pathological
block (dense sysex/CC flood + UI notes) exceeding that makes
`MidiBuffer::addEvent` reallocate inside `processBlock`. Extremely unlikely in
practice, but it is the one allocation path left on the audio thread.
**Fix:** `ensureSize (16384)` or clamp/skip non-note messages when the buffer
approaches capacity.

### IN-04: Knobs have no double-click-reset-to-default

**File:** `plugins/O-simpleFM/Source/ui/public/js/app.js:103-162`
**Issue:** Suite standard (O-MicrotonalSampler v1.23.7,
`pattern_webview_knob_readout_scaled_value`) is dblclick → parameter default
via a `getParameterDefaults` native fn; this UI offers no way to reset a knob
short of loading the Default preset. All other current-gen Ouaricon WebView
editors are converging on this affordance.
**Fix:** Register a `getParameterDefaults` native fn returning
`{ id: normalisedDefault }` (from `RangedAudioParameter::getDefaultValue()`)
and add a `dblclick` handler in `bindKnob` that runs the
dragStarted/setNormalisedValue/dragEnded sequence.

---

_Reviewed: 2026-07-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
