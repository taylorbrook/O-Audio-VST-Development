---
title: "O-MicrotonalSampler Stage 3 (GUI) — Plan"
created: 2026-04-27
stage: 3-gui
phase: plan
status: ready_for_execute
inputs:
  - .planning/stages/3-gui/CONTEXT.md
  - .planning/stages/3-gui/RESEARCH.md
  - .planning/REQUIREMENTS.md
  - .planning/BRIEF.md
  - .planning/STATUS.md
verifies_requirements:
  - FUNC-05  # Drag-drop folder load + filename auto-mapping (UI surface)
  - FUNC-06  # Per-cell manual sample assignment
  - DSP-06   # Manual loop-point override per sample (UI surface)
  - UI-01    # Sample-mapping grid (pitch × velocity layer)
  - UI-02    # Loop-point editor with waveform view
---

# Stage 3 (GUI) — Execution Plan

## Goal

Replace the Phase 2.2 placeholder editor with a WebView-based UI that
delivers the five Stage 3 requirements (FUNC-05, FUNC-06, DSP-06, UI-01,
UI-02) while preserving every Stage 2 invariant (RT-safety, latency,
voice-steal, loop fields). The UI surfaces a tabbed interface with a
sample-mapping grid (88-key piano strip × 4 velocity rows), a side-panel
loop-point editor, an embedded read-only TuningPanel, and a global
control strip — all in the Ouaricon house aesthetic, cross-platform
correct (macOS/Windows WebView2 critical-pattern compliance).

Stage 2 audio behavior must be unchanged after Stage 3 lands — the only
non-additive change to the audio path is the `SampleSlot::audio` storage
swap to `std::shared_ptr<juce::AudioBuffer<float>>` (RQ3-3), which is
verified regression-free in 3.1.

## Open-Question Resolutions (from RESEARCH.md §8)

| # | Decision |
|---|---|
| RP3-1 | **Single-click loaded cell → open loop editor.** **Double-click loaded cell → replace via FileChooser.** **Right-click → context menu** (Replace…, Clear, Open Loop Editor). **Single-click empty cell → open FileChooser** (no editor surface to show on an empty slot). |
| RP3-2 | **Crossfade-length stays global** (Phase 2.5 constant). v1.0 loop editor exposes only `loopStart` / `loopEnd`. Per-slot xfade is a v1.1 candidate. |
| RP3-3 | **Tuning-state readout polls** on Tuning-tab activation + once on editor open. No background polling. |
| RP3-4 | **About tab** — empty in 3.1; minimal version + license link in 3.5 (`O-MicrotonalSampler v0.1.0` + `Ouaricon` link). |
| RP3-5 | **Narrow-window clamp** — horizontal scroll on the grid container when minimum cell width (8 px) is hit. No octave grouping in v1.0. |

## Sub-stage Map

| Phase | Goal | Verifies | Gate |
|---|---|---|---|
| 3.1 | Foundation: Stage 2 invariant addition + WebView shell + tabs + TuningPanel mount + 7-param relays + JSON broadcast scaffold | (infra) | Plugin opens, 7 sliders move, Tuning tab renders read-only, Stage 2 audio regression-free |
| 3.2 | Sample-mapping grid (FUNC-06, UI-01) | FUNC-06, UI-01 | Per-cell replace mid-session retunes only that note; grid reflects load state in <100 ms |
| 3.3 | Folder drop-zone + skipped-files surfacing (FUNC-05) | FUNC-05 | Drag a folder onto the zone → loads identically to button path; skipped files appear in disclosure |
| 3.4 | Loop-point editor side panel (DSP-06, UI-02) | DSP-06, UI-02 | Editing loop points produces audibly different sustain on next note-on; reset restores auto-detect |
| 3.5 | Bottom control strip + aesthetic polish + tuning-readout + About tab | (visual) | Visual review against O-Bells aesthetic; pluginval --strictness 5 SUCCESS; resize behaves |

Each sub-stage commits atomically with its `gate-report.json` and
`PHASE-3.N-SUMMARY.md`, matching the Stage 2 cadence.

---

## Tasks

### Phase 3.1 — Foundation (WebView shell + Stage 2 invariant)

#### Task 1 — Stage 2 invariant addition: SampleSlot/SampleMap surface
- [ ] Change `SampleSlot::audio` from `juce::AudioBuffer<float>` (inline) to `std::shared_ptr<juce::AudioBuffer<float>>`.
- [ ] Add `juce::String filename` to `SampleSlot` (basename only, populated by loader).
- [ ] Add `enum class LoopMode { OneShot, Auto, Manual }` to `SampleSlot` (loader sets `Auto` on detect-success, `OneShot` on fallback; UI flips to `Manual` on override).
- [ ] Add `int version = 0` field to `SampleMap`. Increment on every store.
- **Files:** `Source/SampleMap.h`
- **Depends on:** none

#### Task 2 — Propagate buffer ownership through loader, voice, render harness
- [ ] Update `SampleLoader::buildSlot(...)` (or equivalent) to allocate via `std::make_shared<juce::AudioBuffer<float>>(...)`.
- [ ] Populate `slot.filename = file.getFileName()`.
- [ ] Set `slot.loopMode = LoopMode::Auto` on `LoopDetector::detect` success; `OneShot` on fallback.
- [ ] Update `MicrotonalSamplerVoice` audio access path: `slot->audio->getReadPointer(ch)` (one extra indirection vs current).
- [ ] Update `tests/render-harness/main.cpp` (and any other test fixture) for the new ownership.
- [ ] Update `LoopDetector` if it accepts `AudioBuffer<float>&` — switch to `const AudioBuffer<float>&` accessed via the shared_ptr at the call site (interface unchanged if it already takes `const float*`).
- **Files:** `Source/SampleLoader.cpp`, `Source/SampleLoader.h`, `Source/MicrotonalSamplerVoice.cpp`, `Source/MicrotonalSamplerVoice.h`, `Source/LoopDetector.cpp` (verify), `tests/render-harness/main.cpp`
- **Depends on:** Task 1

#### Task 3 — Processor surface additions (skeletons + helpers)
- [ ] Add `void loadSingleSample(int midiPitch, int velocityLayer, const juce::File& file)` (full implementation in 3.2 — skeleton here logs + returns).
- [ ] Add `void overrideLoopPoints(int midi, int vel, int loopStart, int loopEnd, int crossfadeLen, bool resetToAutoDetect = false)` (full implementation in 3.4).
- [ ] Add `juce::String snapshotSampleMapJson() const` — walks `currentSampleMap` (`std::atomic_load`) + `getLastSkippedFiles()`, returns the schema in RESEARCH.md §RQ3-2.
- [ ] Add `juce::String snapshotWaveformPeaks(int midi, int vel, int targetBins = 512) const` (skeleton; full impl in 3.4 — returns empty JSON for now).
- [ ] Add `void setSampleMapChangedCallback(std::function<void()>)` + `std::function<void()> sampleMapChangedCallback` member. Invoke on the message thread after every map atomic-store.
- [ ] Wire `loadSampleFolder` completion path to bump `version` and invoke the callback (Stage 2 path stays intact; only the version bump + callback are additive).
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Task 2

#### Task 4 — Stage 2 regression verification (pre-flight after invariant change)
- [ ] Build VST3 + AU + Standalone (Release, ninja).
- [ ] Cache-clear + fresh install per `CLAUDE.md` recipe.
- [ ] Run `pluginval --strictness 5 --validate-in-process --skip-gui-tests` on VST3.
- [ ] Run `auval -v aumu OMtS OuDv`.
- [ ] Re-run render-harness against an existing fixture; diff null vs Stage 2 baseline (sample-accurate identity expected — buffer ownership change must not alter output).
- [ ] Block on any regression; do not proceed to Task 5 until green.
- **Files:** none (CI/test step)
- **Depends on:** Task 3

#### Task 5 — CMake recipe (NEEDS_WEBVIEW2 + binary data)
- [ ] Add `NEEDS_WEB_BROWSER TRUE` and `NEEDS_WEBVIEW2 TRUE` to `juce_add_plugin(O-MicrotonalSampler ...)`.
- [ ] Add `target_compile_definitions(O-MicrotonalSampler PUBLIC JUCE_WEB_BROWSER=1 JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 JUCE_USE_CURL=0)`.
- [ ] Add `juce_add_binary_data(O-MicrotonalSampler_UIResources SOURCES ...)` referencing the eight resource files (Task 6).
- [ ] `target_link_libraries(O-MicrotonalSampler PRIVATE O-MicrotonalSampler_UIResources)`.
- **Files:** `plugins/O-MicrotonalSampler/CMakeLists.txt`
- **Depends on:** Task 4

#### Task 6 — Carry-verbatim resources (TuningPanel + JUCE JS helpers)
- [ ] Copy `O-Bells/Resources/ui/js/tuning-panel.js` → `Resources/ui/js/tuning-panel.js` (no edits).
- [ ] Copy `O-Bells/Resources/ui/css/tuning-panel.css` → `Resources/ui/css/tuning-panel.css` (no edits).
- [ ] Copy `O-Bells/Resources/ui/js/juce/index.js` → `Resources/ui/js/juce/index.js`.
- [ ] Copy `O-Bells/Resources/ui/js/juce/check_native_interop.js` → `Resources/ui/js/juce/check_native_interop.js`.
- **Files:** `Resources/ui/js/tuning-panel.js`, `Resources/ui/css/tuning-panel.css`, `Resources/ui/js/juce/index.js`, `Resources/ui/js/juce/check_native_interop.js`
- **Depends on:** none (parallelizable with Task 5)

#### Task 7 — New resources (HTML shell + sampler CSS + readonly overlay + sampler-app)
- [ ] `Resources/ui/index.html`: header (title + tab strip + tuning-state readout span), tab containers (`#tab-samplemap`, `#tab-tuning`, `#tab-about`), bottom control strip with 7 sliders (`<input type="range" data-juce-relay-id="attack">` etc.), import map for sampler-app.js + tuning-panel.js. Mount-points: `#sample-map-grid`, `#folder-drop-zone`, `#loop-editor-panel` (hidden), `#issues-disclosure`, `#tuning-container`, `#about-content`, `#toast-region`.
- [ ] `Resources/ui/css/sampler-shell.css`: palette + typography from RESEARCH.md §1.4, tab strip, bottom-strip layout, grid container scaffold, side-panel layout (closed state), toast styles, base hover/active states.
- [ ] `Resources/ui/css/tuning-panel-readonly.css`: ~30 lines per RESEARCH.md §RQ3-1 — hides write affordances (`.interval-input, .tonic-selector, .octave-stretch-section, .ref-knob-container, .tuning-file-section, .generator-section, .library-list .library-item-apply`); restyles interval rows as read-only.
- [ ] `Resources/ui/js/sampler-app.js`: entry point. Tab activation, TuningPanel lazy-mount on Tuning-tab activate (with interval-input → span swap shim), `__JUCE__.backend.addEventListener("sampleMapUpdated", …)` registration, `getSampleMap()` initial pull on load, slider relay binding lookup, stub grid renderer (placeholder until 3.2), tuning-state readout poll function.
- **Files:** `Resources/ui/index.html`, `Resources/ui/css/sampler-shell.css`, `Resources/ui/css/tuning-panel-readonly.css`, `Resources/ui/js/sampler-app.js`
- **Depends on:** none (parallelizable with Task 5/6)

#### Task 8 — `PluginEditor.{h,cpp}` wholesale replacement
- [ ] Delete the Phase 2.2 placeholder body (`GenericAudioProcessorEditor` + Load Folder button).
- [ ] Class header: inherit `juce::AudioProcessorEditor` + `juce::FileDragAndDropTarget`. Member ordering per RESEARCH.md §1.1 (relays → webView → attachments).
- [ ] Construct 7 `juce::WebSliderRelay` (one per APVTS param: `attack`, `decay`, `sustain`, `release`, `polyphony`, `velocity_crossfade`, `output_gain`).
- [ ] Construct `juce::WebBrowserComponent` with: `Backend::webview2`, `withWinWebView2Options(WinWebView2{}.withUserDataFolder(File::getSpecialLocation(File::tempDirectory).getChildFile("OMicrotonalSampler_WebView")))`, `withNativeIntegrationEnabled`, `withResourceProvider(...)`, `withOptionsFrom(*relayN)` for all 7 relays, and `withNativeFunction(...)` for the 13 native functions in §Native Functions below.
- [ ] Construct 7 `juce::WebSliderParameterAttachment` — matches relay order, after webView is constructed.
- [ ] Resource provider: explicit URL → `BinaryData::*` map, exact equality on path (memory pattern). `/`, `/index.html`, `/css/sampler-shell.css`, `/css/tuning-panel.css`, `/css/tuning-panel-readonly.css`, `/js/sampler-app.js`, `/js/tuning-panel.js`, `/js/juce/index.js`, `/js/juce/check_native_interop.js`. Default branch logs and returns `std::nullopt`.
- [ ] `webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot())` in constructor.
- [ ] `processorRef.setSampleMapChangedCallback([this]{ webView->emitEventIfBrowserIsVisible("sampleMapUpdated", juce::var(processorRef.snapshotSampleMapJson())); })`.
- [ ] `setResizable(true, true)`; `setSize(900, 640)`; `setResizeLimits(720, 480, 1600, 1080)`.
- [ ] `FileDragAndDropTarget` overrides: skeletons returning silently in 3.1 (full routing in 3.3).
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Tasks 3, 5, 6, 7

#### Task 9 — Native functions (Tuning reads + getSampleMap + reportCellLayout)
Register on the WebView via `withNativeFunction`. All run on the message thread.
- [ ] `getSampleMap()` → returns `processorRef.snapshotSampleMapJson()`.
- [ ] `getTuningName()` → returns the active tuning's display name from `processorRef.tuningEngine`.
- [ ] `getTuningIntervals()` → JSON array of cents values (per O-Bells convention).
- [ ] `getTonicNote()` → MIDI note int.
- [ ] `getOctaveStretch()` → cents float.
- [ ] `getEmbeddedTuningList()` → JSON catalog (browse-only; safe to register since it's read-side).
- [ ] `getEmbeddedTuningCategories()` → JSON.
- [ ] `reportCellLayout(jsonString)` → parse + store `cellLayout` + `folderZoneRect` in editor members. No side effects beyond storage.
- [ ] **Skeletons** (full impl in 3.2/3.3/3.4): `loadSampleFolderDialog()`, `loadSingleSampleDialog(midi, vel)`, `getSkippedFiles()`, `overrideLoopPoints(...)`, `resetLoopToAutoDetect(midi, vel)`, `getWaveformPeaks(midi, vel, bins)` — return `false`/empty for now.
- [ ] Confirm we **do NOT** register any `setX` Tuning functions (read-only mode per RESEARCH.md §RQ3-1).
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 8

#### Task 10 — TuningPanel readonly mount
- [ ] In `sampler-app.js`, on Tuning-tab activation: dynamically import `tuning-panel.js`, instantiate `new TuningPanel(container, window.__JUCE__).init()`, then walk `.tuning-panel .interval-input` and replace each with a `<span class="interval-display">{cents}</span>`. The `tuning-panel-readonly.css` overlay is loaded statically from index.html.
- [ ] Verify console produces no errors when the panel attempts setter-side native calls (they fail-silently in try/catch per the panel's existing pattern).
- **Files:** `Resources/ui/js/sampler-app.js`
- **Depends on:** Tasks 6, 7, 9

#### Task 11 — Phase 3.1 gate, summary, atomic commit
- [ ] Build VST3 + AU + Standalone; cache-clear + install.
- [ ] **Gate criteria:**
  - Plugin opens in DAW (Logic AU + Reaper VST3 + Standalone).
  - 7 control-strip sliders move and update their APVTS params (round-trip via `getStateInformation` save/restore).
  - Tuning tab renders TuningPanel with intervals visible and write-affordances hidden.
  - `getSampleMap` returns valid JSON on editor open with `version >= 0`.
  - `pluginval --strictness 5 --validate-in-process --skip-gui-tests` SUCCESS.
  - `auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED.
  - Render-harness identity test passes (Stage 2 audio unchanged).
- [ ] Write `gate-report.json` (mirror Stage 2 schema).
- [ ] Write `PHASE-3.1-SUMMARY.md`: documents the Stage 2 invariant addition (per CONTEXT.md "Document any additions in the 3.1 SUMMARY"), all five processor surface additions, and the editor scaffolding.
- [ ] Atomic commit: all modified Stage 2 sources + new editor + new resources + new artefacts.
- **Files:** `.planning/stages/3-gui/PHASE-3.1-SUMMARY.md`, `.planning/stages/3-gui/gate-report.json`, `.planning/STATUS.md`
- **Depends on:** Tasks 1–10

---

### Phase 3.2 — Sample-mapping grid (FUNC-06 + UI-01)

#### Task 12 — `SampleLoader::loadSingleSlot` extension
- [ ] Add `void loadSingleSlot(const juce::File& file, int midiPitch, int velocityLayer, double sampleRate, std::function<void(SampleSlot, juce::String /*skipReason*/)> completion)`.
- [ ] Worker:
  - Open via `AudioFormatManager::createReaderFor`.
  - SR-convert via `juce::LagrangeInterpolator` per channel (D2-9).
  - Mono → stereo duplicate (D2-10).
  - Run `LoopDetector::detect(buffer)` → `(loopStart, loopEnd, mode)`.
  - Assemble `SampleSlot`: filename, audio shared_ptr, loopStart/loopEnd, loopMode, sourceSampleRate, lengthSamples.
  - On error/parse-fail, completion with empty slot + skip reason.
  - Dispatch completion via `juce::MessageManager::callAsync` (message thread).
- **Files:** `Source/SampleLoader.h`, `Source/SampleLoader.cpp`
- **Depends on:** Phase 3.1 (Tasks 1, 2, 11)

#### Task 13 — `loadSingleSample` full implementation on processor
- [ ] In completion callback (message thread):
  - `auto currentMap = std::atomic_load(&currentSampleMap);`
  - `auto next = std::make_shared<SampleMap>();` deep-copy header + slots minus `(midi, vel)` + push new slot.
  - Update `lowestNote/highestNote/numVelocityLayers` if extended.
  - `next->version = currentMap->version + 1;`
  - `std::atomic_store(&currentSampleMap, next);`
  - Append skip reason to `lastSkippedFiles` if non-empty.
  - Invoke `sampleMapChangedCallback()`.
- [ ] Validation guards: midiPitch ∈ [0,127], velocityLayer ∈ [0, numVelocityLayers−1], file extension ∈ {.wav,.aif,.aiff}, `file.existsAsFile()`.
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 12

#### Task 14 — `loadSingleSampleDialog` native function (FileChooser)
- [ ] Replace skeleton: spawn `juce::FileChooser` modally, on selection call `processorRef.loadSingleSample(midi, vel, file)`. Use the async `launchAsync` pattern (no modal block — JUCE 8 convention).
- [ ] Resolve completion via the `complete(true/false)` callback parameter so JS `await` resumes.
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 13

#### Task 15 — Grid renderer (JS)
- [ ] In `sampler-app.js`, `renderGrid(snapshot)`:
  - Build a CSS grid `grid-template-columns: repeat(88, minmax(8px, 1fr)); grid-template-rows: repeat(4, 1fr);` (rows top→bottom = layer 3..0 so loudest reads at top).
  - For each (midi 21..108) × (layer 0..3) cell, set `data-note` + `data-layer`. Apply class based on slot lookup: `.cell-loaded`, `.cell-empty`, `.cell-loading`, `.cell-active` (Stage 3 active feedback deferred — class reserved).
  - Title attribute = filename for loaded cells.
  - Container scrolls horizontally if `clientWidth / 88 < 8 px` (RP3-5).
- [ ] Re-render on `sampleMapUpdated` event.
- **Files:** `Resources/ui/js/sampler-app.js`, `Resources/ui/css/sampler-shell.css`
- **Depends on:** Phase 3.1 (Tasks 7, 9)

#### Task 16 — Cell interactions (RP3-1 resolution)
- [ ] **Single-click empty cell** → `await Juce.getNativeFunction('loadSingleSampleDialog')(midi, vel)`.
- [ ] **Single-click loaded cell** → `openLoopEditor(midi, vel)` (3.4 hook; 3.2 placeholder logs to console).
- [ ] **Double-click loaded cell** → `loadSingleSampleDialog(midi, vel)` (replace).
- [ ] **Right-click any cell** → context menu (CSS-only popup): "Replace…", "Open Loop Editor" (loaded only), "Clear" (loaded only — disabled in v1.0; surface for completeness).
- [ ] Use a 250 ms double-click discrimination window (delayed `setTimeout` for single-click).
- **Files:** `Resources/ui/js/sampler-app.js`, `Resources/ui/css/sampler-shell.css`
- **Depends on:** Tasks 14, 15

#### Task 17 — Layout shadow publish (`reportCellLayout`)
- [ ] In JS: `publishCellLayout()` walks `document.querySelectorAll('.grid-cell')`, builds the `{cells: [...], folderZone: {...}}` JSON and calls `Juce.getNativeFunction('reportCellLayout')(JSON.stringify(...))`.
- [ ] Trigger on `ResizeObserver(document.body)` (rAF-throttled — RESEARCH.md §9 risk register), and on every `sampleMapUpdated` event after grid re-layout.
- **Files:** `Resources/ui/js/sampler-app.js`
- **Depends on:** Task 15

#### Task 18 — Phase 3.2 gate, summary, atomic commit
- [ ] **Gate criteria:**
  - Single-click empty cell → FileChooser → select WAV → cell becomes loaded with correct filename + audio plays at correct pitch on MIDI input.
  - Double-click loaded cell → FileChooser → swap → only that note's mapping changes (verify by playing pitched MIDI on adjacent loaded notes — pitch unchanged).
  - Mid-session replace measured at <100 ms from FileChooser close to grid state-change reflected (instrument with `performance.now()` in JS, log to console).
  - Grid reflows on window resize without overflow at min size (720×480).
  - Active-voice retention: replacing a slot mid-note doesn't kill the held voice (Stage 2 EC-3 invariant) — held note continues with old buffer until release.
- [ ] Write `gate-report.json` + `PHASE-3.2-SUMMARY.md`.
- [ ] Atomic commit.
- **Files:** `.planning/stages/3-gui/PHASE-3.2-SUMMARY.md`, `.planning/stages/3-gui/gate-report.json`
- **Depends on:** Tasks 12–17

---

### Phase 3.3 — Folder drop-zone + skipped-files (FUNC-05)

#### Task 19 — `FileDragAndDropTarget` routing on host editor
- [ ] Implement `isInterestedInFileDrag` → returns `! files.isEmpty()`.
- [ ] Implement `filesDropped(files, x, y)`:
  - Iterate `cellLayout` for hit-test. Cell hit + extension `.wav/.aif/.aiff` → `loadSingleSample(c.midiNote, c.velocityLayer, file)`. Cell hit + folder/wrong ext → `emitEventIfBrowserIsVisible("toast", "Drop a .wav/.aif on a cell")`.
  - Folder-zone hit + `file.isDirectory()` → `loadSampleFolder(file)`. Folder-zone hit + non-folder → `toast("Drop a folder, not a file")`.
  - Out-of-bounds → silent reject.
  - EC3-3 disallow (folder onto cell): toast "Drop a single file on a cell, or a folder on the top zone."
- [ ] Implement `fileDragEnter/Move/Exit` → emit `hostFileDragMove({x,y})` / `hostFileDragExit({})` to JS for hover feedback.
- **Files:** `Source/PluginEditor.cpp`, `Source/PluginEditor.h`
- **Depends on:** Phase 3.2 (Task 17 publishes the layout shadow this consumes)

#### Task 20 — Folder drop-zone visuals + button fallback
- [ ] In `index.html`: `<div id="folder-drop-zone">Drop folder here · or · <button id="load-folder-btn">Load Folder…</button></div>` above the grid.
- [ ] CSS: dashed border, hover/active glow states tied to `.drag-over` class.
- [ ] JS: `addEventListener('hostFileDragMove', e => ...)` toggles `.drag-over` based on `(x,y)` vs `folderZone` rect from publishCellLayout's source data (recomputed locally for brevity). `hostFileDragExit` clears all hover.
- [ ] HTML5 `dragover` listener on `#folder-drop-zone` calls `e.preventDefault()` (visual prevention only — actual drop is consumed by host editor; smoke-test that WebView2 doesn't intercept).
- [ ] Button → `Juce.getNativeFunction('loadSampleFolderDialog')()`.
- [ ] `loadSampleFolderDialog` native function: spawn modal `juce::FileChooser::launchAsync` for directory selection → `processorRef.loadSampleFolder(folder)`.
- **Files:** `Resources/ui/index.html`, `Resources/ui/css/sampler-shell.css`, `Resources/ui/js/sampler-app.js`, `Source/PluginEditor.cpp`
- **Depends on:** Task 19

#### Task 21 — Skipped-files toast + disclosure
- [ ] `getSkippedFiles()` native function: returns JSON array from `processorRef.getLastSkippedFiles()`.
- [ ] On `sampleMapUpdated`: if `snapshot.skippedFiles.length > 0`, emit toast `"{N} files skipped"` (3 s auto-dismiss) AND populate `<details id="issues-disclosure"><summary>Issues ({N} files skipped)</summary><ul>{lines}</ul></details>`. Empty array → hide disclosure, no toast.
- [ ] Toast component (`#toast-region`): single-element queue, 3 s auto-dismiss, fade-out CSS.
- **Files:** `Resources/ui/js/sampler-app.js`, `Resources/ui/css/sampler-shell.css`, `Source/PluginEditor.cpp`
- **Depends on:** Task 20

#### Task 22 — Phase 3.3 gate, summary, atomic commit
- [ ] **Gate criteria:**
  - Drag a folder onto the drop zone → identical load to button path; grid populates within Stage 2's typical load time.
  - Drag a file (not folder) onto the zone → toast "Drop a folder, not a file"; no load.
  - Drag a folder containing intentionally unparseable filenames (e.g. `not_a_sample.txt`, `mystery.aiff`) → load succeeds for parseable files, skipped files appear in disclosure with reasons.
  - Toast appears on completion when `skippedFiles.length > 0`; hides cleanly after 3 s.
  - Drag-hover visual feedback works on macOS (Logic AU + Reaper VST3 + Standalone).
- [ ] Write `gate-report.json` + `PHASE-3.3-SUMMARY.md`. Atomic commit.
- **Files:** `.planning/stages/3-gui/PHASE-3.3-SUMMARY.md`, `.planning/stages/3-gui/gate-report.json`
- **Depends on:** Tasks 19–21

---

### Phase 3.4 — Loop-point editor side panel (DSP-06 + UI-02)

#### Task 23 — `overrideLoopPoints` + `resetLoopToAutoDetect` full implementation
- [ ] In `overrideLoopPoints`:
  - `auto current = std::atomic_load(&currentSampleMap);`
  - Locate matching slot (linear scan via existing `SampleMap::findSlot` or equivalent).
  - Build `next = std::make_shared<SampleMap>(...)` deep-copy; replace target slot's `loopStart`, `loopEnd`, set `loopMode = LoopMode::Manual`.
  - Bump `version`. `std::atomic_store(&currentSampleMap, next)`. Invoke callback.
- [ ] In `resetLoopToAutoDetect`: same path, but call `LoopDetector::detect` and write result; set `loopMode = Auto` (or `OneShot` on fallback).
- [ ] No-op log if slot absent. Crossfade param recorded for v1.1 (per RP3-2, ignored in v1.0 — global stays).
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Phase 3.1 (Tasks 1–3, 11)

#### Task 24 — `snapshotWaveformPeaks` full implementation
- [ ] Walk slot's audio buffer (sum-of-channels per sample, then `std::minmax` over `framesPerBin = numFrames / targetBins`).
- [ ] Output JSON per RESEARCH.md §RQ3-5 schema: `{midiNote, velocityLayer, lengthSamples, sourceSampleRate, loopStart, loopEnd, loopMode, peaks: [[min,max], …]}`.
- [ ] Single-pass O(N); typical 5 s sample at 48 kHz ≈ 1 ms on Apple Silicon — message-thread acceptable for click-driven path.
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 23

#### Task 25 — Native functions: `getWaveformPeaks`, `overrideLoopPoints`, `resetLoopToAutoDetect`
- [ ] `getWaveformPeaks(midi, vel, bins)` → returns `processorRef.snapshotWaveformPeaks(...)`. Also emit as `waveformPeaks` event (so the JS side can use either pull or push pattern). Recommend pull for click-driven open.
- [ ] `overrideLoopPoints(midi, vel, start, end, xfade)` → routes to processor; on completion, `sampleMapUpdated` broadcast happens automatically via the callback.
- [ ] `resetLoopToAutoDetect(midi, vel)` → routes to processor; same broadcast.
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Tasks 23, 24

#### Task 26 — Loop-editor side panel (HTML + CSS)
- [ ] `index.html` add `<aside id="loop-editor-panel" hidden>` with: header (`{filename} · {midi} · L{vel}` + close button), `<canvas id="waveform-canvas">`, marker labels (loop-start ms / loop-end ms read-only display), `<button id="loop-reset">Reset to auto-detect</button>`, `<button id="loop-apply">Apply</button>`, `<button id="loop-cancel">Cancel</button>`.
- [ ] CSS: side-panel slide-in transform (350 ms ease), grid container narrows to `calc(100% - 360px)` when panel is open. Critical: canvas styled via `width: calc(100% - 16px); height: 200px;` per memory pitfall #6 — never use `position: absolute; left:0; right:0;`.
- [ ] Esc key closes panel; X button closes panel; clicking another cell while open swaps content without animation.
- **Files:** `Resources/ui/index.html`, `Resources/ui/css/sampler-shell.css`
- **Depends on:** Phase 3.1 (Task 7)

#### Task 27 — Loop-editor JS (canvas render + draggable markers)
- [ ] On `openLoopEditor(midi, vel)`: `await Juce.getNativeFunction('getWaveformPeaks')(midi, vel, 512)` → parse → render.
- [ ] DPR-aware canvas backing store: `canvas.width = clientWidth * dpr; canvas.height = clientHeight * dpr; ctx.setTransform(dpr, 0, 0, dpr, 0, 0);` (memory pitfall #6).
- [ ] Draw min/max envelope (green/teal stroke + cream fill).
- [ ] Loop-start/loop-end markers as vertical bars + drag handles. Pointer events (`pointerdown/move/up`); free-drag in v1.0 (no zero-crossing snap).
- [ ] Live update marker positions during drag; emit `getWaveformPeaks` is NOT re-called on drag (markers are pure JS overlay over the cached peak data).
- [ ] Apply → `Juce.getNativeFunction('overrideLoopPoints')(midi, vel, start, end, /*xfade*/8)` → toast "New loop points apply to next note-on." (EC3-6).
- [ ] Reset → `Juce.getNativeFunction('resetLoopToAutoDetect')(midi, vel)` → re-fetch peaks for fresh marker positions.
- [ ] **Reset disabled when loopMode === "one-shot"** (EC3-7) with tooltip "Sample is one-shot — no loop region detected."
- [ ] Cancel → close panel, no writeback.
- **Files:** `Resources/ui/js/sampler-app.js`
- **Depends on:** Tasks 25, 26

#### Task 28 — Phase 3.4 gate, summary, atomic commit
- [ ] **Gate criteria:**
  - Click loaded cell → editor opens with waveform render in <250 ms; loop markers visible at correct positions for the slot's loopStart/loopEnd.
  - Drag start marker rightward, Apply → next note-on plays from the new loop region (audible difference: shorter sustain or different timbre).
  - Reset → markers snap back to LoopDetector's auto-detected values; mode field reverts to `"auto"`.
  - Reset disabled when current cell is one-shot (no loop region).
  - Editor open during active note → marker move + Apply does NOT cut the current voice; new loop applies on next note-on (EC3-6 toast surfaces this).
  - Canvas is crisp on Retina (DPR test) and stretches correctly at min/max window sizes.
- [ ] Write `gate-report.json` + `PHASE-3.4-SUMMARY.md`. Atomic commit.
- **Files:** `.planning/stages/3-gui/PHASE-3.4-SUMMARY.md`, `.planning/stages/3-gui/gate-report.json`
- **Depends on:** Tasks 23–27

---

### Phase 3.5 — Polish (control strip + aesthetic + tuning-readout + About)

#### Task 29 — Bottom control strip styling
- [ ] Lift `.ouaricon-knob` styles from `O-Bells/Resources/ui/index.html` inline `<style>` block into `Resources/ui/css/sampler-shell.css`.
- [ ] Apply to all 7 sliders. Layout: flexbox row, equal-width cells, label above + numeric readout below each knob. ADSR + Polyphony + Vel-XF + Out Gain ordered left→right.
- [ ] Verify each WebSliderRelay-bound `<input type="range">` is correctly wrapped in the knob component without breaking attachment.
- **Files:** `Resources/ui/index.html`, `Resources/ui/css/sampler-shell.css`
- **Depends on:** Phase 3.1

#### Task 30 — Tuning-state readout in chrome
- [ ] In header, `<span id="tuning-readout"></span>` next to tab strip.
- [ ] On editor open + on every Tuning-tab activation: `await Juce.getNativeFunction('getTuningName')()` → set textContent. RP3-3: poll on activation only, no background interval.
- **Files:** `Resources/ui/index.html`, `Resources/ui/js/sampler-app.js`
- **Depends on:** Phase 3.1 (Task 9)

#### Task 31 — About tab content (RP3-4)
- [ ] `<div id="tab-about">`: plugin name + version (lift version string from CMakeLists.txt — hard-code in HTML for v0.1.0; Stage 4 polish will plumb dynamically), short tagline, license link (Ouaricon).
- [ ] Style consistent with Sample Map / Tuning tabs.
- **Files:** `Resources/ui/index.html`, `Resources/ui/css/sampler-shell.css`
- **Depends on:** Phase 3.1

#### Task 32 — Aesthetic polish pass
- [ ] Spacing rhythm pass: 8 px / 16 px / 24 px scale across all margins/padding.
- [ ] Hover states on all interactive elements (cells, buttons, knobs, tabs).
- [ ] Container shadows + border treatments matching O-Bells card style.
- [ ] Typography: Garamond serif for headings, system sans for numeric readouts (per O-Bells convention if used).
- [ ] Active-tab underline animation; subtle.
- [ ] Compare visual against O-Bells screenshots side-by-side; document any deliberate divergences.
- **Files:** `Resources/ui/css/sampler-shell.css`, `Resources/ui/index.html`
- **Depends on:** Tasks 29, 30, 31

#### Task 33 — Window resize + final pluginval gate
- [ ] Resize from min (720×480) to max (1600×1080) — verify no overflow, no clipped controls, grid reflows smoothly, side panel + grid coexist at all sizes ≥ 900 wide.
- [ ] Below 900 wide with side panel open: panel stacks below grid OR auto-closes (decide during pass; recommend auto-close + toast "Resize wider to use the loop editor").
- [ ] Run `pluginval --strictness 5 --validate-in-process --skip-gui-tests` SUCCESS.
- [ ] Run `pluginval --strictness 5 --validate-in-process` (with GUI tests) — expect SUCCESS or document any GUI-spawn benign warnings (per Stage 2 precedent).
- [ ] Run `auval -v aumu OMtS OuDv` SUCCESS.
- [ ] Render-harness identity test against Stage 2 baseline — SUCCESS (audio path unchanged after Stage 3).
- **Files:** none (verification step)
- **Depends on:** Task 32

#### Task 34 — Phase 3.5 gate, summary, atomic commit
- [ ] **Gate criteria:**
  - Visual match against Ouaricon house aesthetic (O-Bells reference).
  - All 5 sub-stage gates green; Stage 2 audio invariant intact.
  - Latency contract preserved (`getLatencySamples()` unchanged from Stage 2).
  - pluginval --strictness 5 SUCCESS; auval AU VALIDATION SUCCEEDED.
  - Render-harness identity test SUCCESS.
- [ ] Write `gate-report.json` + `PHASE-3.5-SUMMARY.md`.
- [ ] Update `.planning/STATUS.md` to `stage_3_execute_complete; ready for verify`.
- [ ] Atomic commit.
- **Files:** `.planning/stages/3-gui/PHASE-3.5-SUMMARY.md`, `.planning/stages/3-gui/gate-report.json`, `.planning/STATUS.md`
- **Depends on:** Task 33

---

## Native Functions (registered on WebView, by sub-stage)

| Name | Args | Returns | Sub-stage |
|---|---|---|---|
| `getSampleMap` | () | JSON string | 3.1 |
| `getTuningName` | () | string | 3.1 |
| `getTuningIntervals` | () | JSON array | 3.1 |
| `getTonicNote` | () | int | 3.1 |
| `getOctaveStretch` | () | float | 3.1 |
| `getEmbeddedTuningList` | () | JSON | 3.1 |
| `getEmbeddedTuningCategories` | () | JSON | 3.1 |
| `reportCellLayout` | (jsonStr) | void | 3.1 (skeleton) / 3.2 (consumed) |
| `loadSampleFolderDialog` | () | bool | 3.3 |
| `loadSingleSampleDialog` | (midi, vel) | bool | 3.2 |
| `getSkippedFiles` | () | JSON array | 3.3 |
| `getWaveformPeaks` | (midi, vel, bins) | JSON | 3.4 |
| `overrideLoopPoints` | (midi, vel, start, end, xfade) | bool | 3.4 |
| `resetLoopToAutoDetect` | (midi, vel) | bool | 3.4 |

## C++ → JS Events

| Event ID | Payload | Trigger |
|---|---|---|
| `sampleMapUpdated` | JSON snapshot | After every map atomic-store (folder load, cell replace, loop override, reset) |
| `waveformPeaks` | JSON peaks | (Optional push variant; pull via `getWaveformPeaks` is preferred) |
| `hostFileDragMove` | `{x,y}` | JUCE host-level fileDragMove |
| `hostFileDragExit` | `{}` | JUCE host-level fileDragExit |
| `toast` | string | C++ rejects a drop / loop-apply success / etc. |

## Files Created

- `plugins/O-MicrotonalSampler/Resources/ui/index.html`
- `plugins/O-MicrotonalSampler/Resources/ui/css/sampler-shell.css`
- `plugins/O-MicrotonalSampler/Resources/ui/css/tuning-panel.css` (verbatim)
- `plugins/O-MicrotonalSampler/Resources/ui/css/tuning-panel-readonly.css`
- `plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js`
- `plugins/O-MicrotonalSampler/Resources/ui/js/tuning-panel.js` (verbatim)
- `plugins/O-MicrotonalSampler/Resources/ui/js/juce/index.js` (from JUCE / O-Bells)
- `plugins/O-MicrotonalSampler/Resources/ui/js/juce/check_native_interop.js` (from JUCE / O-Bells)
- `.planning/stages/3-gui/PHASE-3.{1,2,3,4,5}-SUMMARY.md` (one per sub-stage)
- `.planning/stages/3-gui/gate-report.json` (overwritten per sub-stage; final = 3.5)

## Files Modified

- `plugins/O-MicrotonalSampler/CMakeLists.txt` — `NEEDS_WEB_BROWSER`, `NEEDS_WEBVIEW2`, compile defs, `juce_add_binary_data`.
- `plugins/O-MicrotonalSampler/Source/PluginEditor.{h,cpp}` — wholesale replacement.
- `plugins/O-MicrotonalSampler/Source/PluginProcessor.{h,cpp}` — additive: `loadSingleSample`, `overrideLoopPoints`, `snapshotSampleMapJson`, `snapshotWaveformPeaks`, `setSampleMapChangedCallback`. **No removals.**
- `plugins/O-MicrotonalSampler/Source/SampleMap.h` — `SampleSlot::audio` shared_ptr; `SampleSlot::filename` string; `SampleSlot::loopMode` enum; `SampleMap::version` int.
- `plugins/O-MicrotonalSampler/Source/SampleLoader.{h,cpp}` — `loadSingleSlot` extension; buffer ownership update.
- `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.{h,cpp}` — read-pointer dereference path: `slot->audio->getReadPointer(...)`.
- `plugins/O-MicrotonalSampler/Source/LoopDetector.{h,cpp}` — interface compatible if signature already takes `const float*` (verify in Task 2).
- `plugins/O-MicrotonalSampler/tests/render-harness/main.cpp` — buffer ownership update.
- `.planning/STATUS.md` — phase markers.

## Success Criteria (Stage 3 verify-phase preview)

- [ ] FUNC-05: Drag a folder onto the drop zone → loads identically to button path; skipped files surface in disclosure (verified in 3.3).
- [ ] FUNC-06: Single-cell replace via drop or FileChooser → only that note's mapping changes; adjacent loaded notes unaffected; mid-note voices retain old buffer until release (verified in 3.2).
- [ ] DSP-06: Loop editor → drag markers → Apply → next note-on plays new loop region; Reset returns to auto-detect (verified in 3.4).
- [ ] UI-01: Sample-mapping grid (88 keys × 4 vel-layers) is the primary editing surface; renders < 100 ms on map update (verified in 3.2).
- [ ] UI-02: Loop-point editor with waveform view + draggable markers + crossfade (global v1.0) renders crisply on Retina; reset disabled for one-shots (verified in 3.4).
- [ ] PERF-01: Audio thread allocation-free unchanged; voices read via `slot->audio->getReadPointer` with no per-block allocations (verified in 3.1 render-harness identity).
- [ ] PERF-04: `getLatencySamples()` unchanged; editor open/close does not affect audio (verified in 3.5).
- [ ] Cross-platform: WebView2 critical patterns enforced (`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` + `withUserDataFolder` + resource provider URL=path equality + DPR-aware canvas).
- [ ] pluginval --strictness 5 SUCCESS; auval AU VALIDATION SUCCEEDED.
- [ ] No regression in Stage 2 audio behavior (render-harness identity test against pre-Stage-3 baseline).

## Risk Register (carried from RESEARCH.md §9)

| Risk | Mitigation in plan |
|---|---|
| Stage 2 audio regression from `SampleSlot::audio` shared_ptr swap | Task 4 (full Stage 2 verify gate after Task 3) — block on identity test before proceeding to editor work |
| WebView2 silent fallback to IE on Windows | Task 5 enforces both flags; manual smoke-test in 3.5 |
| macOS sandboxed AU drop failures | Task 19 uses `FileDragAndDropTarget` (host editor) not HTML5 paths |
| TuningPanel readonly CSS leaks | Overlay scoped to `.tuning-panel` selectors only |
| ResizeObserver thrash | Task 17 rAF-throttles publishCellLayout |
| Per-cell replace stalls UI | Task 12 background-thread decode + async completion; cell shows loading state |
| Editor close mid-load orphans broadcast | `emitEventIfBrowserIsVisible` is a no-op when WebView is gone |
| Latency contract violation | No `setLatencySamples` calls anywhere in Stage 3; verified in 3.5 |

## Next Phase

Ready for: **execute** phase

`/plugin-execute O-MicrotonalSampler 3-gui`
