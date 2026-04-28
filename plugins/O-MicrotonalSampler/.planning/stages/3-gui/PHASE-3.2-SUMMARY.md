---
title: "O-MicrotonalSampler Phase 3.2 — Sample-mapping Grid Summary"
created: 2026-04-28
stage: 3-gui
phase: 3.2
status: gate_pass
commit_sha: 4083582
verifies_requirements:
  - FUNC-06   # Per-cell manual sample assignment
  - UI-01     # Sample-mapping grid (pitch × velocity layer)
---

# Phase 3.2 — Sample-mapping Grid Implementation Summary

## Status

**Phase 3.2 GATE PASS** — Tasks 12–17 implemented; Task 18 gate green.

- Triple build (VST3 + AU + Standalone) via `ninja`: GREEN
- Cache-clear + reinstall per CLAUDE.md: COMPLETED
- `pluginval --strictness 5 --validate-in-process --skip-gui-tests`: **SUCCESS**
- `auval -v aumu OMtS OuDv`: **AU VALIDATION SUCCEEDED**

## What Phase 3.2 Delivers

The placeholder grid surface from Phase 3.1 is now a fully functional
88-key × 4-velocity-layer mapping grid. Single-click empty cells open
a FileChooser; double-click loaded cells replaces the sample without
disturbing adjacent slots; the grid re-renders on every `sampleMapUpdated`
event from the processor; `reportCellLayout` publishes the JS-side grid
geometry back to the host editor for upcoming Phase 3.3 drag-routing.

### FUNC-06 surface (per-cell manual sample assignment)

- `loadSingleSampleDialog(midi, vel)` native function spawns
  `juce::FileChooser::launchAsync` (openMode), forwards the chosen file to
  `processorRef.loadSingleSample(midi, vel, file)`, resolves the JS
  `complete()` callback with `true`/`false` so `await` resumes.
- `processorRef.loadSingleSample` validates inputs (`midi ∈ [0,127]`,
  `vel ∈ [0, numVelocityLayers−1]`, ext ∈ {.wav, .aif, .aiff},
  `file.existsAsFile()`), then dispatches `SampleLoader::loadSingleSlot` on
  the loader thread.
- On loader completion (message thread): atomic-load current map →
  deep-copy header + slots minus matching `(midi, vel)` → push new slot →
  bump `SampleMap::version` → `std::atomic_store` the new map → invoke
  `sampleMapChangedCallback()` to broadcast `sampleMapUpdated`.
- Active voices that snapshotted the previous map keep their
  `std::shared_ptr<SampleMap>` alive for the duration of the held note —
  the per-cell replace does not steal audio for already-sounding notes
  (Stage 2 EC-3 invariant preserved by reference-counted ownership).

### UI-01 surface (88×4 grid)

- `renderGrid(snapshot)` JS function builds a CSS grid with
  `grid-template-columns: repeat(88, minmax(8px, 1fr))` and
  `grid-template-rows: repeat(4, 1fr)`. Rows are inverted (top→bottom =
  layer 3..0) so the loudest layer reads at the top.
- Each cell carries `data-note` and `data-layer` attributes plus class
  `cell-loaded` / `cell-empty` / `cell-loading`. Loaded cells get a
  `title` attribute = filename for hover tooltips.
- Container scrolls horizontally if `clientWidth / 88 < 8`px (RP3-5).
- Re-render on `sampleMapUpdated` event (already wired in 3.1).

### Cell interactions (RP3-1)

- **Single-click EMPTY cell** → `loadSingleSampleDialog(midi, vel)`.
- **Single-click LOADED cell** → placeholder `openLoopEditor(midi, vel)`
  (logs to console; full impl lands in Phase 3.4).
- **Double-click LOADED cell** → `loadSingleSampleDialog(midi, vel)`
  (replace path).
- **Right-click any cell** → CSS-only popup context menu with
  "Replace…" / "Open Loop Editor" (loaded only) / "Clear" (loaded only,
  disabled in v1.0 — surface for completeness).
- 250 ms double-click discrimination: single-click action is fired via
  `setTimeout(..., 250)`, cancelled if a second click lands within the
  window.

### `reportCellLayout` publish (Task 17)

- `publishCellLayout()` walks `document.querySelectorAll('.grid-cell')`,
  builds `{cells: [{midiNote, velocityLayer, x, y, w, h}, ...],
  folderZone: {x, y, w, h}}` from `getBoundingClientRect()` data,
  `JSON.stringify`s, calls `Juce.getNativeFunction('reportCellLayout')`.
- Triggers: `ResizeObserver(document.body)` (rAF-throttled), every
  `sampleMapUpdated` event after grid re-render, and on initial load.

## Files Modified

- `plugins/O-MicrotonalSampler/Source/SampleLoader.h` — `loadSingleSlot`
  declaration + private state for single-slot worker dispatch.
- `plugins/O-MicrotonalSampler/Source/SampleLoader.cpp` — `loadSingleSlot`
  implementation (open via `AudioFormatManager::createReaderFor`,
  Lagrange SR-convert, mono→stereo duplicate, `LoopDetector::detectLoop`,
  assemble `SampleSlot` with shared_ptr buffer + filename + loopMode +
  sourceSampleRate, async completion via `MessageManager::callAsync`).
- `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp` —
  `loadSingleSample` full implementation replacing the Phase 3.1
  skeleton (input validation, loader dispatch, atomic deep-copy +
  version bump + atomic store + callback fire).
- `plugins/O-MicrotonalSampler/Source/PluginEditor.cpp` —
  `loadSingleSampleDialog` native function full implementation
  (FileChooser launch, completion callback, processor dispatch).
- `plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js` —
  `renderGrid`, cell event listeners (single/double/right-click),
  context menu, `publishCellLayout`, ResizeObserver hook, double-click
  discrimination.
- `plugins/O-MicrotonalSampler/Resources/ui/css/sampler-shell.css` —
  88×4 grid layout, `.cell-loaded` / `.cell-empty` / `.cell-loading`
  states, hover/active visuals, context menu CSS.
- `plugins/O-MicrotonalSampler/Resources/ui/index.html` — grid
  container slot wired up; minor mount-point cleanup.

## Files Created

- `plugins/O-MicrotonalSampler/.planning/stages/3-gui/PHASE-3.2-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/3-gui/gate-report.json`
  (overwritten — same shape as Phase 3.1)

## Active-Voice Retention Reasoning (EC-3 invariant)

`MicrotonalSamplerVoice::startNote` snapshots
`std::shared_ptr<SampleMap> currentMap = *sampleMapSource` once per
note — the lock-free refcount inc means the voice keeps the map (and
therefore the slot's `audio` shared_ptr) alive for the duration of
the held note even if the processor's `currentSampleMap` is
atomic-stored to a new map mid-note. The per-cell replace path
constructs a brand-new `SampleMap` and never mutates the existing
one, so a voice holding the old snapshot continues reading the old
audio buffer until release.

## Deviations

None. All Tasks 12–17 implemented as planned.

Phase 3.1's outstanding deviation (no render-harness target) carried
forward unchanged — Phase 3.2 audio path is structurally identical to
Phase 3.1 (no algorithm changes; only an additional code path for
single-slot replace), so pluginval + auval coverage remains adequate.

## Next

Phase 3.3 — Folder drop-zone + skipped-files (FUNC-05). Tasks 19–22 in
PLAN.md.
