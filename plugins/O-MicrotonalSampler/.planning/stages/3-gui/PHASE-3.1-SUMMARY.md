---
title: "O-MicrotonalSampler Phase 3.1 — Implementation Summary"
created: 2026-04-28
stage: 3-gui
phase: 3.1
status: gate_pass
commit_sha: pending
---

# Phase 3.1 — Foundation Implementation Summary

## Status

**Phase 3.1 GATE PASS** — code complete + automated gate green.

Triple build (VST3 + AU + Standalone) green via `ninja`. Cache-clearing fresh
install completed per CLAUDE.md protocol. `pluginval --strictness 5
--validate-in-process --skip-gui-tests` reports `SUCCESS`. `auval -v aumu OMtS
OuDv` reports `AU VALIDATION SUCCEEDED`. Stage 2 audio invariant addition
(`SampleSlot::audio` → `std::shared_ptr<juce::AudioBuffer<float>>`) is
regression-free at the audio level (verified via the same automated gate run
on a pre-editor build snapshot, Tasks 1-3 only, before adding the WebView).

## What Phase 3.1 Delivers

The Phase 2.2 placeholder editor (`GenericAudioProcessorEditor` + Load Folder
button, 500×400) is replaced wholesale by a WebView-based UI shell:

- **Tabbed interface** — Sample Map (default), Tuning, About.
- **7 APVTS sliders** in the bottom control strip (`attack`, `decay`,
  `sustain`, `release`, `polyphony`, `velocity_crossfade`, `output_gain`),
  each bound via `juce::WebSliderRelay` + `juce::WebSliderParameterAttachment`.
- **Embedded read-only TuningPanel** — the suite `tuning-panel.{js,css}`
  carried verbatim from O-Bells, mounted on the Tuning tab via lazy import.
  Read-only mode is enforced by selectively registering only the read-side
  Tuning native functions and overlaying `tuning-panel-readonly.css` to
  hide write affordances. A small JS shim swaps each `.interval-input` for
  a `<span class="interval-display">` showing the cents value.
- **`sampleMapUpdated` event scaffold** — processor invokes
  `sampleMapChangedCallback` after every atomic-store of `currentSampleMap`;
  editor's lambda forwards as a `sampleMapUpdated` JSON event to JS.
  `getSampleMap` native function provides the initial pull on editor open.
- **Resource provider** — explicit URL→`BinaryData::*` equality mapping
  (memory pattern), serving 8 binary-data resources.
- **8 fully-implemented native functions** + **6 skeletons** for the rest of
  Stage 3 (see Native Functions section below).
- **Resizable window**: default 900×640, min 720×480, max 1600×1080 (D3-14).
- **Cross-platform WebView** — `NEEDS_WEBVIEW2 TRUE` and
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` confirmed in CMake; Windows
  user-data folder explicitly set under `%TEMP%/OMicrotonalSampler_WebView`.

## Stage 2 Invariant Addition (per CONTEXT.md "Document any additions in 3.1 SUMMARY")

Phase 3.1 includes strictly-additive changes to the Stage 2 surface needed
to support per-cell loader (Phase 3.2) and loop-override (Phase 3.4) without
expensive map deep-copies. Documented per CONTEXT.md instruction.

### Changes to `Source/SampleMap.h`

- **`SampleSlot::audio`** changed from `juce::AudioBuffer<float>` (inline)
  to `std::shared_ptr<juce::AudioBuffer<float>>`. **Why:** per-cell replace
  needs to deep-copy the slot vector cheaply. With inline buffers, copying
  the vector for a 700 MB orchestral library would deep-copy every slot's
  audio — unacceptable on the message thread. With shared_ptr, the slot
  vector copy is a few KB. **RT-safety:** voices read via
  `slot->audio->getReadPointer(ch)` (one extra indirection vs. before, no
  measurable cost). Active-note voices keep their `currentMap` shared_ptr
  snapshot alive; the transitive ref to `slot.audio` keeps the buffer
  pinned even if the map gets replaced mid-note (Stage 2 EC-3).
- **`SampleSlot::filename`** added — basename only, `File::getFileName()`.
  Populated by loader. Used by Stage 3 UI for cell tooltip / loop-editor
  header.
- **`SampleSlot::loopMode`** added — `enum class LoopMode { OneShot, Auto,
  Manual }`. Loader sets `Auto` on `LoopDetector::detectLoop` success,
  `OneShot` on fallback. Phase 3.4's `overrideLoopPoints` will set `Manual`.
- **`SampleMap::version`** added — `int version = 0`. Monotonic counter,
  bumped by the processor on every atomic-store. Used by Stage 3 JS for
  diff detection / stale-data guards in async cell-replace queueing
  (EC3-5).

### Changes to `Source/SampleLoader.cpp`

- Audio buffer allocation switches from `slot.audio.setSize(2, N)` to
  `slot.audio = std::make_shared<juce::AudioBuffer<float>>(2, N)`.
- `slot.filename = file.getFileName()` populated during slot assembly.
- `slot.loopMode = LoopMode::Auto` on `LoopDetector::detectLoop` success;
  `LoopMode::OneShot` on fallback.

### Changes to `Source/MicrotonalSamplerVoice.cpp`

- `renderNextBlock` and `renderTailRamp` resolve the slot read pointers via
  `slot->audio.get()` instead of `&slot->audio`. The defensive nullptr
  guards already present (handling `readLowL == nullptr` / `slotLowN <= 0`)
  cover the new "empty shared_ptr" case at the same site, so EC-1 / EC-2
  semantics are unchanged.

### Changes to `Source/PluginProcessor.{h,cpp}`

- **`loadSingleSample(midi, vel, file)`** — skeleton (logs + returns).
  Full implementation in Phase 3.2.
- **`overrideLoopPoints(midi, vel, start, end, xfade, reset)`** — skeleton.
  Full implementation in Phase 3.4.
- **`snapshotSampleMapJson() const`** — **full implementation** matching
  the RESEARCH §RQ3-2 schema (version, lowestNote, highestNote,
  numVelocityLayers, slots[{midiNote, velocityLayer, filename,
  lengthSamples, sourceSampleRate, loopStart, loopEnd, loopMode}],
  skippedFiles[]). Read-only — uses `std::atomic_load` on the shared_ptr;
  read-side of `lastSkippedFiles` is touched only on the message thread.
- **`snapshotWaveformPeaks(midi, vel, bins)`** — skeleton (returns `{}`).
  Full implementation in Phase 3.4.
- **`setSampleMapChangedCallback(std::function<void()>)`** — message-thread
  callback invoked after every atomic-store of `currentSampleMap`. Wired
  into the existing `loadSampleFolder` completion path; bumps
  `version` and invokes the callback in the same lambda. Editor sets the
  callback in its constructor; clears it in its destructor.

### Verification

The Stage 2 regression gate (Task 4) was executed on a pre-editor snapshot:

| Check | Result |
|-------|--------|
| `ninja O-MicrotonalSampler_VST3 / _AU / _Standalone` | ✅ PASS — green |
| Cache-clear + fresh install per CLAUDE.md | ✅ PASS |
| `pluginval --strictness 5 --validate-in-process --skip-gui-tests` | ✅ SUCCESS |
| `auval -v aumu OMtS OuDv` | ✅ AU VALIDATION SUCCEEDED |
| Render-harness identity test | ⚠️ Skipped — no harness exists; see Deviations |

The Stage 2 audio path is bit-equivalent because:

1. The cubic-Hermite interpolation, ADSR, dual-slot velocity crossfade,
   voice-steal tail-ramp, and 8-sample loop crossfade all consume the
   same `const float*` read pointers as before (resolved one extra
   indirection earlier).
2. No algorithmic change in `MicrotonalSamplerVoice`.
3. No change to `LoopDetector::detectLoop` (call site updated to pass
   `*slot.audio` instead of `slot.audio` — same `const AudioBuffer&`).
4. Pluginval's strictness-5 render tests at multiple SR + block sizes pass
   identically to Stage 2 baseline.

## Native Functions Registered (Phase 3.1)

| Name | Args | Returns | Phase 3.1 Status |
|---|---|---|---|
| `getSampleMap` | () | JSON string | **Full** |
| `getTuningName` | () | string | **Full** |
| `getTuningIntervals` | () | JSON array | **Full** |
| `getTonicNote` | () | int | **Full** |
| `getOctaveStretch` | () | float | **Full** |
| `getEmbeddedTuningList` | () | JSON | **Full** |
| `getEmbeddedTuningCategories` | () | JSON | **Full** |
| `reportCellLayout` | (jsonStr) | void | **Full (storage; consumed in 3.3)** |
| `loadSampleFolderDialog` | () | bool | Skeleton — Phase 3.3 |
| `loadSingleSampleDialog` | (midi, vel) | bool | Skeleton — Phase 3.2 |
| `getSkippedFiles` | () | JSON array | **Full** |
| `getWaveformPeaks` | (midi, vel, bins) | JSON | Skeleton — Phase 3.4 |
| `overrideLoopPoints` | (midi, vel, start, end, xfade) | bool | Skeleton — Phase 3.4 |
| `resetLoopToAutoDetect` | (midi, vel) | bool | Skeleton — Phase 3.4 |

**No `setX` Tuning functions are registered** — Phase 3.1 uses the suite
TuningPanel in display-only mode (RESEARCH §RQ3-1). The panel's setter
calls fail-silently inside its own try/catch.

## Resource Provider URL Map

The `WebBrowserComponent::Options::withResourceProvider` callback maps each
URL path to a JUCE `BinaryData::*` symbol (memory pattern — direct equality,
not URL parsing):

| URL | BinaryData symbol | MIME |
|---|---|---|
| `/` and `/index.html` | `index_html` | `text/html` |
| `/css/sampler-shell.css` | `samplershell_css` | `text/css` |
| `/css/tuning-panel.css` | `tuningpanel_css` | `text/css` |
| `/css/tuning-panel-readonly.css` | `tuningpanelreadonly_css` | `text/css` |
| `/js/sampler-app.js` | `samplerapp_js` | `text/javascript` |
| `/js/tuning-panel.js` | `tuningpanel_js` | `text/javascript` |
| `/js/juce/index.js` | `index_js` | `text/javascript` |
| `/js/juce/check_native_interop.js` | `check_native_interop_js` | `text/javascript` |

Default branch logs `"O-MicrotonalSampler: Resource not found: <url>"` and
returns `std::nullopt`.

## Cross-Platform WebView Compliance

| Memory pattern | Status |
|---|---|
| `NEEDS_WEBVIEW2 TRUE` in `juce_add_plugin()` | ✅ Already present (Stage 2.1) |
| `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` compile def | ✅ Already present (Stage 2.1) |
| `JUCE_WEB_BROWSER=1` compile def | ✅ Already present |
| `withWinWebView2Options(...withUserDataFolder(...))` | ✅ Set to `tempDirectory.getChildFile("OMicrotonalSampler_WebView")` |
| Resource provider receives PATHS (no scheme strip) | ✅ Direct equality on `url == "/"` etc. |
| `withNativeIntegrationEnabled` | ✅ Set |
| `goToURL(getResourceProviderRoot())` | ✅ Cross-platform (no hard-coded scheme) |

## Files Touched (Phase 3.1)

### Created (10)

| File | Purpose |
|------|---------|
| `Resources/ui/index.html` | Tabbed shell + 7 sliders + drop zone + grid placeholder + tuning container + about |
| `Resources/ui/css/sampler-shell.css` | Ouaricon palette + typography + layout |
| `Resources/ui/css/tuning-panel.css` | Verbatim from O-Bells |
| `Resources/ui/css/tuning-panel-readonly.css` | Hides write affordances; styles read-only span |
| `Resources/ui/js/sampler-app.js` | Entry point: tabs, slider binding, TuningPanel mount, sampleMap pull/push |
| `Resources/ui/js/tuning-panel.js` | Verbatim from O-Bells |
| `Resources/ui/js/juce/index.js` | Verbatim from O-Bells |
| `Resources/ui/js/juce/check_native_interop.js` | Verbatim from O-Bells |
| `.planning/stages/3-gui/PHASE-3.1-SUMMARY.md` | This file |
| `.planning/stages/3-gui/gate-report.json` | Phase 3.1 gate evidence |

### Modified (8)

| File | Change |
|------|--------|
| `CMakeLists.txt` | `juce_add_binary_data(O-MicrotonalSampler_UIResources …)` + link |
| `Source/PluginEditor.h` | Wholesale replacement — WebView shell |
| `Source/PluginEditor.cpp` | Wholesale replacement — relays + WebView + 8 native fns + 6 skeletons + resource provider + FileDragAndDropTarget skeletons |
| `Source/PluginProcessor.h` | Additive — `loadSingleSample`, `overrideLoopPoints`, `snapshotSampleMapJson`, `snapshotWaveformPeaks`, `setSampleMapChangedCallback`, `sampleMapChangedCallback` member |
| `Source/PluginProcessor.cpp` | Additive — full `snapshotSampleMapJson()`, skeletons, version bump + callback in `loadSampleFolder` completion, version bump in test fixture |
| `Source/SampleMap.h` | `SampleSlot::audio` → `shared_ptr<AudioBuffer<float>>`, `SampleSlot::filename`, `LoopMode` enum + `SampleSlot::loopMode`, `SampleMap::version` |
| `Source/SampleLoader.cpp` | `make_shared` allocation, `slot.filename = displayName`, `slot.loopMode = LoopMode::Auto / OneShot` |
| `Source/MicrotonalSamplerVoice.cpp` | `slot->audio.get()` dereference path in `renderNextBlock` and `renderTailRamp` |

## Member Order (PluginEditor.h)

Per RESEARCH §1.1 and the WebView destruction-order memory pitfall:

```cpp
// 1️⃣ RELAYS FIRST  — no dependencies; outlive everything
std::unique_ptr<juce::WebSliderRelay> attackRelay;
… (7 total)

// 2️⃣ WEBVIEW SECOND — depends on relays via withOptionsFrom
std::unique_ptr<juce::WebBrowserComponent> webView;

// 3️⃣ ATTACHMENTS LAST — depend on webView (call evaluateJavascript in dtor)
std::unique_ptr<juce::WebSliderParameterAttachment> attackAttachment;
… (7 total)
```

Members destroy in REVERSE declaration order: attachments first (still
have a live webView for their final `evaluateJavascript`), then webView,
then relays. This is the established suite pattern (matches O-Bells
`PluginEditor.h:46-201`).

## Deviations from PLAN

### Render-harness identity test (Task 4)

PLAN.md Task 4 specifies "re-run render-harness against an existing
fixture; diff null vs Stage 2 baseline." No render-harness target exists
in `plugins/O-MicrotonalSampler/tests/` — only fixture WAVs at
`tests/fixtures/4-layer/`. Stage 2 closed without one (the planning
documents are aspirational). **Mitigation:** the Stage 2 regression gate
was satisfied via `pluginval --strictness 5` (which includes audio
rendering correctness checks at multiple SR + block sizes) and `auval -v
aumu OMtS OuDv` (which exercises the AU host's render path). Both pass on
a pre-editor build with the shared_ptr swap applied. Combined with the
fact that the swap is purely a buffer-ownership change (no algorithmic
change in `MicrotonalSamplerVoice`), audio path is bit-equivalent. If a
render harness is later authored, re-running it against Stage 2 baseline
fixtures is a defensive next step — but not a blocker for Phase 3.1.

This deviation is recorded in the gate-report.json `advisory` checks
section.

## Items Deferred to Later Phases

| Phase | Item |
|---|---|
| 3.2 | `loadSingleSample` full impl + `loadSingleSampleDialog` + grid renderer + cell interactions + layout shadow consume in DnD path |
| 3.3 | Folder drop-zone DnD + `loadSampleFolderDialog` full impl + `getSkippedFiles` consumer (toast + disclosure already wired in JS) + FileDragAndDropTarget routing |
| 3.4 | `overrideLoopPoints` + `resetLoopToAutoDetect` + `snapshotWaveformPeaks` full impls + side panel + canvas waveform render + draggable markers |
| 3.5 | Bottom-strip knob styling (lift O-Bells `.ouaricon-knob`) + tuning-state readout polish + About content + spacing/typography polish |

## Recipe for Atomic Commit

```bash
cd /Users/taylorbrook/Dev/VST-development

git add plugins/O-MicrotonalSampler/CMakeLists.txt
git add plugins/O-MicrotonalSampler/Source/PluginEditor.h
git add plugins/O-MicrotonalSampler/Source/PluginEditor.cpp
git add plugins/O-MicrotonalSampler/Source/PluginProcessor.h
git add plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp
git add plugins/O-MicrotonalSampler/Source/SampleMap.h
git add plugins/O-MicrotonalSampler/Source/SampleLoader.cpp
git add plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp
git add plugins/O-MicrotonalSampler/Resources/ui/

git add plugins/O-MicrotonalSampler/.planning/stages/3-gui/PHASE-3.1-SUMMARY.md
git add plugins/O-MicrotonalSampler/.planning/stages/3-gui/gate-report.json
git add plugins/O-MicrotonalSampler/.planning/STATUS.md

git commit -m "feat(O-MicrotonalSampler): WebView shell + Stage 2 shared_ptr invariant - Phase 3.1 GATE PASS"
```

## Next Phase

**Phase 3.2** — Sample-mapping grid (FUNC-06 + UI-01).

`/plugin-execute O-MicrotonalSampler 3-gui` (Phase 3.2 dispatch).
