---
title: "O-MicrotonalSampler Phase 3.3 — Folder drop-zone + skipped-files Summary"
created: 2026-04-28
stage: 3-gui
phase: 3.3
status: gate_pass
verifies_requirements:
  - FUNC-05   # Folder drag-drop + button fallback + skipped-files surfacing
---

# Phase 3.3 — Folder drop-zone + skipped-files Implementation Summary

## Status

**Phase 3.3 GATE PASS** — Tasks 19–22 implemented; Task 22 gate green.

- Triple build (VST3 + AU + Standalone) via `ninja`: GREEN
- Cache-clear + reinstall per CLAUDE.md: COMPLETED
- `pluginval --strictness 5 --validate-in-process --skip-gui-tests`: **SUCCESS**
- `auval -v aumu OMtS OuDv`: **AU VALIDATION SUCCEEDED**

## What Phase 3.3 Delivers

The placeholder folder-drop visuals from Phase 3.1 are now a fully functional
folder-drop surface. Drag a folder onto the top zone → identical scan/load
to the file-picker button path. Drop a single audio file onto a cell → the
per-cell loader runs (same path as Phase 3.2's single-cell FileChooser).
Invalid drop targets surface a 3-second toast with the routing hint. Files
skipped by the loader (unparseable filenames, unsupported formats, IO
failures) appear in a `<details>` disclosure under the grid plus a one-shot
toast on transition.

### FUNC-05 surface (folder drag-drop + skipped-files)

**Drop routing matrix** (`OMicrotonalSamplerAudioProcessorEditor::filesDropped`):
- Cell hit + `.wav/.aif/.aiff` → `processorRef.loadSingleSample(midi, vel, file)`
- Cell hit + folder → toast `"Drop a single file on a cell, or a folder on the top zone."` (EC3-3)
- Cell hit + other ext → toast `"Drop a .wav/.aif on a cell"`
- Folder-zone hit + folder → `processorRef.loadSampleFolder(folder)`
- Folder-zone hit + non-folder → toast `"Drop a folder, not a file"`
- Out-of-bounds → silent reject

**Hit-test source.** `cellLayout` and `folderZoneRect` are populated by JS
via `reportCellLayout` (Phase 3.2 Task 17) — coordinates are WebView client
space, which matches what `juce::FileDragAndDropTarget` callbacks receive.

**Drag-hover visuals.** `fileDragEnter` / `fileDragMove` emit
`hostFileDragMove({x,y})`; `fileDragExit` emits `hostFileDragExit({})`. JS
listens, computes `(x,y) ∈ folderZoneRect` from `getBoundingClientRect()`,
and toggles the `.drag-over` class on `#folder-drop-zone` for the dashed-
border + box-shadow glow.

**Button fallback.** `#load-folder-btn` calls
`Juce.getNativeFunction('loadSampleFolderDialog')()`, which spawns
`juce::FileChooser::launchAsync` with `openMode | canSelectDirectories`.
On selection it invokes the same `processorRef.loadSampleFolder` path as
the drop handler — drop and button paths are byte-identical from the
processor's perspective.

**Skipped-files surfacing.**
- `getSkippedFiles` native function: full impl from Phase 3.1 (returns JSON
  array from `processorRef.getLastSkippedFiles()`).
- `snapshotSampleMapJson()` already includes `skippedFiles` array per
  RESEARCH §RQ3-2 schema (verified — line 585–594 of `PluginProcessor.cpp`).
- On every `sampleMapUpdated` event:
  - If `skippedFiles.length > 0`: populate `<ul id="issues-list">` with one
    `<li>` per entry; show the `<details id="issues-disclosure">` with a
    `summary` of `Issues (N file(s) skipped)`.
  - On *transition* (signature change) with `length > 0`: emit a 3-second
    toast `N file(s) skipped`. Idle re-renders with the same set are silent.
  - Empty array → hide disclosure, no toast.

**Toast component.** `#toast-region` (already in DOM from Phase 3.1) hosts
a single-element queue. `showToast(msg)` clears any in-flight toast,
appends a new `.toast` div, triggers the entrance transition on next frame,
and arms a 3-second auto-dismiss timer that fades out and removes from DOM.
Subscribed to the C++ `toast` event so the file-drop routing can surface
invalid-target hints without a custom JS round-trip.

### Cross-platform behaviour

- macOS sandboxed AU/VST3 hosts route real file paths through
  `juce::FileDragAndDropTarget` only — JS `dataTransfer.files` is empty
  inside a WebView (memory pattern). The drop visuals are JS-driven from
  C++-emitted events, but the actual file payload only ever exists in C++.
- Windows WebView2: same routing (host editor receives the drop). The
  `dragover` `preventDefault()` on the zone is a belt-and-suspenders no-op
  on macOS but suppresses the "no drop" cursor on hosts that show one
  inside the WebView frame.

## Files Modified

- `plugins/O-MicrotonalSampler/Source/PluginEditor.cpp`
  - Replaced 4 `FileDragAndDropTarget` skeletons with full hit-test +
    routing implementations (`filesDropped`, `fileDragEnter`,
    `fileDragMove`, `fileDragExit`).
  - Replaced `loadSampleFolderDialog` skeleton with full impl
    (`juce::FileChooser` for directory selection, async forwarding to
    `processorRef.loadSampleFolder`).
  - Added `isAudioFileExt` helper + `emitToast` helper in anonymous
    namespace.
- `plugins/O-MicrotonalSampler/Resources/ui/index.html`
  - Folder-drop-zone: enabled `#load-folder-btn` (was disabled in 3.1
    pending 3.3); added `.drop-zone-or` separator span for clearer
    "drop or button" affordance.
- `plugins/O-MicrotonalSampler/Resources/ui/css/sampler-shell.css`
  - Strengthened `.drag-over` glow (added `box-shadow` inset).
  - New `.drop-zone-or` style (italic muted text).
- `plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js`
  - `bindFolderDropZone()`: button click → `loadSampleFolderDialog`;
    `dragover` `preventDefault()` on the zone.
  - `bindHostDragEvents()`: subscribes to `hostFileDragMove` /
    `hostFileDragExit` and toggles `.drag-over` class.
  - `showToast(msg)` + `bindToastEventListener()`: 3-second single-element
    toast queue with fade-in/out; subscribed to backend `toast` event.
  - `handleSampleMapSnapshot`: added skipped-files transition tracking
    (`lastSkippedSignature`) so toasts fire on transition only, not on
    every idle re-render.
  - `DOMContentLoaded` boot sequence: added `bindHostDragEvents`,
    `bindToastEventListener`, `bindFolderDropZone`.

## Files Created

- `plugins/O-MicrotonalSampler/.planning/stages/3-gui/PHASE-3.3-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/3-gui/gate-report.json`
  (overwritten — same shape as Phase 3.2)

## Gate Criteria — Verification

| Criterion | Status | Note |
|---|---|---|
| Drag a folder onto the drop zone → identical load to button path | infrastructure_ready | `filesDropped` folder-zone branch invokes `processorRef.loadSampleFolder(folder)` — byte-identical to the button's `loadSampleFolderDialog` resolution path. `sampleMapUpdated` push event triggers `renderGrid` + `publishCellLayout`. Manual DAW smoke deferred to user. |
| Drag a file (not folder) onto the zone → toast "Drop a folder, not a file"; no load | infrastructure_ready | `filesDropped` folder-zone + non-directory branch emits the toast event; processor not invoked. |
| Drag a folder containing intentionally unparseable files → load succeeds for parseable files; skipped files appear in disclosure with reasons | infrastructure_ready | `processorRef.loadSampleFolder` populates `lastSkippedFiles` in its async completion (Phase 2.2). `snapshotSampleMapJson` serializes `skippedFiles`. JS `<ul id="issues-list">` renders one `<li>` per entry. |
| Toast appears on completion when `skippedFiles.length > 0`; hides cleanly after 3 s | infrastructure_ready | `showToast` arms a 3000 ms `setTimeout`; CSS fade-out runs on `.toast` class removal; element removed from DOM 300 ms after fade-out begins. Transition tracking ensures re-render with same skip set is silent. |
| Drag-hover visual feedback works on macOS (Logic AU + Reaper VST3 + Standalone) | infrastructure_ready | `fileDragMove`/`Exit` emit events to JS; JS computes hit-test against `getBoundingClientRect()` and toggles `.drag-over` class. CSS provides dashed-border + inset gold-tinted box-shadow. Verification via DAW smoke deferred to user. |
| `pluginval --strictness 5` SUCCESS | passed | Confirmed. |
| `auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED | passed | Confirmed. |

## Cell + Folder-Zone Routing — EC3-3 Resolution

The plan calls out EC3-3 explicitly: a folder dropped onto a *cell* is
ambiguous (the user might mean "load this whole folder" or "use this folder
as the source for this one cell"). The resolution lands on **disallow with
a routing hint** — toast `"Drop a single file on a cell, or a folder on
the top zone."` This avoids the ambiguity entirely and surfaces the
discoverable affordance for both modes.

## Active-Voice Retention (carried from Phase 3.2)

`processorRef.loadSampleFolder` constructs a brand-new `SampleMap` and
atomic-stores it. Voices that have already snapshotted the previous map
keep their `std::shared_ptr<SampleMap>` alive for the held note. The
folder-drop path therefore preserves Stage 2's EC-3 invariant identically
to the per-cell replace path.

## Deviations

None. All Tasks 19–22 implemented as planned.

Phase 3.1's outstanding deviation (no render-harness target) carries
forward unchanged — Phase 3.3 introduces no new audio code paths
(folder load was already complete in Phase 2.2; this phase only wires
the folder path to the existing GUI surfaces). pluginval + auval
coverage remains adequate.

## Next

Phase 3.4 — Loop-point editor side panel (DSP-06 + UI-02). Tasks 23–28 in
PLAN.md.
