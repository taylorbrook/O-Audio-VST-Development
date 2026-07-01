---
plugin: O-DigiDelay
version: 1.2.9
reviewed: 2026-07-01T14:09:01Z
depth: deep
files_reviewed: 7
files_reviewed_list:
  - plugins/O-DigiDelay/Source/PluginProcessor.cpp
  - plugins/O-DigiDelay/Source/PluginProcessor.h
  - plugins/O-DigiDelay/Source/PluginEditor.cpp
  - plugins/O-DigiDelay/Source/PluginEditor.h
  - plugins/O-DigiDelay/Source/OuariconPresetManager.h
  - plugins/O-DigiDelay/Source/ui/public/index.html
  - plugins/O-DigiDelay/Source/ui/public/modules/preset-manager.js
findings:
  critical: 0
  warning: 7
  info: 4
  total: 11
status: issues_found
---

# O-DigiDelay: Full-Plugin Code Review Report

**Reviewed:** 2026-07-01T14:09:01Z
**Depth:** deep (DSP → parameter-binding → WebView UI data flow)
**Files Reviewed:** 7
**Status:** issues_found

## Summary

O-DigiDelay is a well-structured JUCE 8 stereo digital delay. The core audio path is
solid on the fundamentals adversarial review usually catches first: `processBlock` is
allocation-free and lock-free, feedback is hard-clamped to 0.95 (no runaway),
`ScopedNoDenormals` is present, all parameter reads go through cached atomic pointers,
`numSamples == 0` is guarded before the RMS division, tempo-sync degrades correctly when
the playhead or BPM is absent, and the division index is `jlimit`-clamped before the LUT
lookup. The WebView bridge is also clean: **all 11 `getNativeFunction()` calls in JS have
matching `withNativeFunction()` registrations** (no silent bridge gaps), the resource
provider uses bare-path equality (per the codebase convention), and preset names are
injected into the DOM via `textContent` (no XSS).

No BLOCKER-class defects (crash, data loss, injection, auth bypass) were proven. The
findings below are correctness/robustness gaps: a delay-buffer that consumes 100% of its
declared modulation headroom (zero margin at 48/96/192 kHz), a persistent-NaN hazard in
the feedback loop, a save-dialog that silently ignores the folder the user chose, an
unsanitized preset filename (the known codebase regression), a shared WebView2 user-data
folder, and a cross-thread meter race.

---

## Resolution Log

- **v1.2.10** (DSP robustness): WR-01, WR-02, WR-06, IN-04 — resolved.
- **v1.2.11** (preset system): WR-03, WR-04, IN-02, IN-03 — resolved; IN-01 — doc corrected (path intentionally unchanged, migration deferred).
- **Outstanding:** WR-05 (WebView2 shared user-data folder — Windows-only), WR-07 (butterfly asset filename has spaces — percent-encoding-fragile).

---

## Structural Findings (fallow)

No `<structural_findings>` block was provided with this review; no structural pre-pass to
reconcile. All findings below are from direct (narrative) code review.

---

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: Delay modulation consumes 100% of buffer headroom — zero margin at 48/96/192 kHz

**File:** `plugins/O-DigiDelay/Source/PluginProcessor.cpp:117-118, 208-213`
**Issue:** The buffer is sized for `2000ms + 25ms` headroom
(`maxDelaySamples = ceil(2.025 * sampleRate)`), but the maximum read index consumes
*exactly* that 25 ms:

- base = `time(≤2000ms)` → `2.000·fs`
- spread = `spread(≤1.0) · 15ms` → `0.015·fs`
- mod = `mod(≤1.0) · 10ms · lfo(≤+1.0)` → `0.010·fs`
- max read = `2.025·fs`

At 48 kHz, 96 kHz, and 192 kHz, `2.025·fs` is an integer that equals `ceil(2.025·fs)`
exactly (verified: margin = 0.0 samples; only 44.1 kHz has a 0.5-sample cushion). The read
therefore lands on `maximumDelayInSamples` with no headroom, relying entirely on JUCE's
internal `totalSize = maxDelay + 1` allocation to stay in-bounds for the 4-tap Lagrange3rd
interpolator. The "25 ms headroom" comment is misleading — it is fully consumed, not
reserved. Any future increase to the 15 ms spread scale or 10 ms mod scale (line 209-210)
will silently push the read past `maximumDelayInSamples`.
**Fix:** Add genuine headroom so the read never reaches the buffer edge:
```cpp
// PluginProcessor.cpp:117 — reserve modulation depth PLUS a safety pad
const double maxDelaySeconds = (2000.0 + 15.0 + 10.0 + 5.0) / 1000.0; // +5ms pad
```

### WR-02: NaN/Inf in input persists in the feedback loop indefinitely

**File:** `plugins/O-DigiDelay/Source/PluginProcessor.cpp:218-229`
**Issue:** `ScopedNoDenormals` flushes denormals but does nothing for NaN/Inf. If an
upstream plugin emits a single NaN/Inf sample, it is written into the delay line and
recirculated: `feedbackLeft = delayedSample * currentFeedback` becomes NaN, is pushed back
via `pushSample(0, drySample + feedbackLeft)`, and contaminates the delay buffer
permanently. The delay output stays NaN (silent/garbage) until `prepareToPlay` is called
again (sample-rate/block-size change or re-instantiation). There is no `isfinite` guard on
the feedback state.
**Fix:** Sanitize the recirculated feedback each sample (cheap, RT-safe):
```cpp
feedbackLeft = delayedSample * currentFeedback;
if (! std::isfinite(feedbackLeft)) feedbackLeft = 0.0f;   // break NaN/Inf recirculation
// (same for feedbackRight)
```

### WR-03: `savePresetWithDialog` discards the folder the user chose in the dialog

**File:** `plugins/O-DigiDelay/Source/PluginEditor.cpp:51-70`
**Issue:** The native save dialog lets the user navigate to any folder and pick a filename,
but the handler only extracts `getFileNameWithoutExtension()` (line 63) and passes it to
`presetManager.savePreset(presetName)`, which *always* writes to
`getUserPresetsDirectory()` (`OuariconPresetManager.h:306`). If a user opens the dialog,
navigates to the Desktop, and saves "MyPreset", the file is silently written to
`~/Library/O-DigiDelay/Presets/User/MyPreset.json` instead — the chosen destination is
ignored, and the dialog's returned path is thrown away. The UI reports success with the
name, so the user believes the file is on their Desktop.
**Fix:** Either (a) honor the chosen path by writing the JSON to `results.getFirst()`
directly, or (b) if "save into the managed User folder only" is the intended behavior,
replace the folder-navigable `FileChooser` with a simple name-entry prompt so the UI does
not imply a destination it ignores.

### WR-04: Preset name is not sanitized before use as a filename (path-separator drop)

**File:** `plugins/O-DigiDelay/Source/OuariconPresetManager.h:306, 227, 386`
**Issue:** `savePreset` builds the file as
`getUserPresetsDirectory().getChildFile(presetName + ".json")` with no sanitization. A name
containing `/` (or `:` on some platforms) is interpreted by `getChildFile` as a path
separator, so the file is written to an unexpected location or silently dropped, and
`getPresetList()` (non-recursive `findChildFiles`) will not find it — the exact regression
documented for `OuariconPresetManager` elsewhere in this codebase (O-simplePhysicalModelSynth
"Koto / Harp"). In this specific UI the risk is reduced because the only wired entry point
is the native save dialog (WR-03), but the `savePreset(name)` native function is still
exposed to JS and this is a shared module reused verbatim across plugins.
**Fix:** Strip path separators before constructing the filename:
```cpp
auto safeName = presetName.replaceCharacters("/\\:", "___");
auto presetFile = getUserPresetsDirectory().getChildFile(safeName + ".json");
```
(Apply consistently in `isFactoryPreset`, `loadPreset`, and `deletePreset` so the same name
round-trips.)

### WR-05: WebView2 user-data folder is the shared temp root, not a plugin-specific subfolder

**File:** `plugins/O-DigiDelay/Source/PluginEditor.cpp:31-33`
**Issue:** `withUserDataFolder(File::getSpecialLocation(SpecialLocationType::tempDirectory))`
points every Ouaricon plugin's WebView2 instance at the *same* bare temp directory. The
project MEMORY explicitly prescribes a plugin-specific child
(`...getChildFile("PluginName_WebView")`). WebView2 places a lock on its user-data folder;
loading two Ouaricon plugins (or two instances) in the same Windows host can produce
contention over the shared folder, and a failed WebView2 construction silently falls back
to IE (blank UI, no error). This is Windows-only but a real cross-instance hazard.
**Fix:**
```cpp
.withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                        .getChildFile("O-DigiDelay_WebView"))
```

### WR-06: Non-atomic RMS meter state shared across audio and message threads

**File:** `plugins/O-DigiDelay/Source/PluginProcessor.cpp:240-251` and `PluginProcessor.h:90-91`
**Issue:** `rmsLevelLeft/Right` (`juce::LinearSmoothedValue<float>`) are written on the audio
thread (`setTargetValue` + `skip`, lines 240-241 / 249-250) and read on the message thread
via `getRmsLevelLeft()`/`getRmsLevelRight()` (`PluginProcessor.h:50-51`), which the 30 Hz
editor timer calls (`PluginEditor.cpp:194-195`). `LinearSmoothedValue` is not thread-safe;
this is a data race on a plain `float` (technically UB). It is benign for a visual meter but
still a genuine cross-thread defect.
**Fix:** Publish the meter value through a `std::atomic<float>` written at the end of
`processBlock` and read (relaxed) in the getters, rather than reading the smoother's internal
state across threads.

### WR-07: Butterfly image path contains spaces — resource-provider match is percent-encoding-fragile

**File:** `plugins/O-DigiDelay/Source/PluginEditor.cpp:257-262`, `index.html:70, 87`
**Issue:** The resource provider matches `url == "/img/butterfly2_Black and white.png"` with
literal spaces, while the CSS references `url('img/butterfly2_Black and white.png')`.
WebView engines commonly percent-encode spaces to `%20` in the request path; if the path
arrives as `/img/butterfly2_Black%20and%20white.png`, the exact-string comparison fails,
`getResource` returns `std::nullopt` (404), and the butterfly overlay/echoes silently
disappear. Even though v1.2.9 is "Installed" (so it currently works on the tested engine),
a filename with spaces is a latent cross-platform/cross-engine hazard (macOS WKWebView vs
Windows WebView2 may differ).
**Fix:** Rename the asset to a space-free filename (e.g. `butterfly2_bw.png`) and update the
CSS + the `getResource` match, or decode the path before comparison
(`juce::URL::removeEscapeChars(url)`).

## Info

### IN-01: Preset directory diverges from its own doc and from the platform convention

**File:** `plugins/O-DigiDelay/Source/OuariconPresetManager.h:206-213`
**Issue:** The doc comment (lines 22-24, 208) says presets live in
`~/Library/Application Support/{pluginName}/Presets/`, but the code builds
`~/Library/{pluginName}/Presets/` (no "Application Support"). The result is a
non-standard top-level `~/Library/O-DigiDelay/` folder. Not a bug, but it contradicts the
documented path and the macOS convention.
**Fix:** Use `File::commonApplicationDataDirectory` / `userApplicationDataDirectory`
(`~/Library/Application Support`) and update the comment to match.

### IN-02: Preset JSON hard-codes `"version": "1.0.0"` regardless of plugin version

**File:** `plugins/O-DigiDelay/Source/OuariconPresetManager.h:254, 540`
**Issue:** Every saved preset records `version = "1.0.0"` even though the plugin is at
v1.2.9. This metadata is currently unused on load, but it defeats any future
version-migration logic.
**Fix:** Write `JucePlugin_VersionString` (already used at `PluginEditor.cpp:126`) instead
of the literal.

### IN-03: `getNextPreset`/`getPreviousPreset` fall back to index 0 after a dialog-loaded file

**File:** `plugins/O-DigiDelay/Source/OuariconPresetManager.h:436-438, 450-452`
**Issue:** After `loadPresetFromFile`, `currentPresetName` is set to the imported file's base
name, which may not exist in the Factory/User list. On the next prev/next press,
`indexOf(currentPresetName)` returns -1 and navigation jumps to `presets[0]` rather than to
a neighbor of the current sound. Minor UX quirk.
**Fix:** When `currentIndex < 0`, remember the last known list index (or treat prev/next as
relative to the last in-list selection) instead of snapping to element 0.

### IN-04: No `isBusesLayoutSupported` override; mono/other layouts unvalidated

**File:** `plugins/O-DigiDelay/Source/PluginProcessor.cpp:91-94` (and absence in header)
**Issue:** The processor declares a fixed stereo-in/stereo-out bus but provides no
`isBusesLayoutSupported`. In a mono context the right channel is null and `delayLineRight`
is simply unused (handled safely at `processBlock:224`), so there is no crash — but hosts
that probe alternate layouts get no explicit accept/reject, which can cause the plugin to be
offered layouts it does not truly process.
**Fix:** Add an `isBusesLayoutSupported` that accepts mono→mono and stereo→stereo (input set
== output set) and rejects the rest.

---

_Reviewed: 2026-07-01T14:09:01Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
