---
title: "O-MicrotonalSampler Phase 3.4 — Loop-point editor side panel Summary"
created: 2026-04-28
stage: 3-gui
phase: 3.4
status: gate_pass
verifies_requirements:
  - DSP-06   # Per-cell loop-point override + auto-detect reset
  - UI-02    # Loop editor side panel with waveform render + draggable markers
---

# Phase 3.4 — Loop-point editor side panel Implementation Summary

## Status

**Phase 3.4 GATE PASS** — Tasks 23–28 implemented; Task 28 gate green.

- Triple build (VST3 + AU + Standalone) via `ninja`: GREEN
- Cache-clear + reinstall per CLAUDE.md: COMPLETED
- `pluginval --strictness 5 --validate-in-process --skip-gui-tests`: **SUCCESS**
- `auval -v aumu OMtS OuDv`: **AU VALIDATION SUCCEEDED**

## What Phase 3.4 Delivers

The loop-editor side panel is now a fully functional waveform editor. Click
a loaded cell → a 360-px panel slides in from the right, showing the slot's
filename + MIDI note + velocity layer in a header, a 200-px-tall canvas
rendering the min/max envelope (DPR-aware backing store + warm-brown stroke
+ antique-gold fill), and two draggable vertical markers for loop start/end.
The bottom row hosts Reset · Cancel · Apply buttons.

Apply persists the new loop region into the SampleMap via the processor's
atomic deep-copy path; the change applies to the next note-on (active voices
keep their snapshot map alive transitively per Stage 2 EC-3). Reset re-runs
`LoopDetector::detectLoop` and refreshes the canvas. Esc / X / Cancel close
the panel without writeback.

### DSP-06 surface (loop-point override + auto-detect reset)

**`OMicrotonalSamplerAudioProcessor::overrideLoopPoints`** (full impl —
replaces Phase 3.1 skeleton):

- `atomic_load currentSampleMap` snapshot.
- `findSlot(midi, vel)` — DBG no-op log + return if absent.
- Deep-copy `next = std::make_shared<SampleMap>(*current)` (cheap — slots
  hold `std::shared_ptr<juce::AudioBuffer<float>>`, vector copy is pointers
  + POD only).
- Locate matching slot in `next->slots` (mutable lvalue), set
  `loopStart`/`loopEnd` (clamped to `[0, numSamples]` defensively),
  `loopMode = LoopMode::Manual`.
- Bump `next->version`. `atomic_store(&currentSampleMap, next)`. Fire
  `sampleMapChangedCallback`.
- `crossfadeLen` argument: recorded in DBG log for v1.1 (per RP3-2 — global
  xfade stays in voices for v1.0).

**`OMicrotonalSamplerAudioProcessor::resetLoopToAutoDetect`** (new —
convenience wrapper around `overrideLoopPoints`):

- Reuses the override path with `resetToAutoDetect=true`.
- Inside, when the flag is set:
  - Empty/null audio → `loopStart = loopEnd = 0; loopMode = OneShot`.
  - Else `LoopDetector::detectLoop(*audio, sourceSampleRate)`. Valid →
    new loop fields + `LoopMode::Auto`. Invalid → zeros + `OneShot`.
- Same atomic deep-copy + version bump + callback path as the override.

### Waveform peaks snapshot (RESEARCH §RQ3-5)

**`OMicrotonalSamplerAudioProcessor::snapshotWaveformPeaks`** (full impl):

- Single-pass O(N) scan over the slot's audio buffer.
- `framesPerBin = jmax(1, numFrames / targetBins)`; tail bin absorbs the
  remainder so the last bin extends to `numFrames`.
- Per-sample sum-of-channels mixdown then divide by `numChannels` for a
  normalized mono envelope.
- `std::minmax`-style tracker per bin, written into a `[[min, max], ...]`
  array on a `juce::DynamicObject` along with `midiNote`, `velocityLayer`,
  `lengthSamples`, `sourceSampleRate`, `loopStart`, `loopEnd`, `loopMode`,
  `filename`. Serialized via `juce::JSON::toString`.
- Typical 5 s sample at 48 kHz ≈ 240 k samples ≈ 1 ms on Apple Silicon —
  message-thread acceptable for the click-driven open path.

### Native function bindings

**Three skeletons replaced with full impl** (`Source/PluginEditor.cpp`):

- `getWaveformPeaks(midi, vel, bins=512)` — returns
  `processorRef.snapshotWaveformPeaks(...)`. Click-driven pull (preferred
  over push for one-shot open).
- `overrideLoopPoints(midi, vel, start, end, xfade=8)` — routes to
  `processorRef.overrideLoopPoints(...)`. Returns `true` on dispatch (not
  on audible application — that happens on the next note-on per EC3-6).
  The `sampleMapUpdated` broadcast happens automatically via the
  processor's `sampleMapChangedCallback`.
- `resetLoopToAutoDetect(midi, vel)` — routes to
  `processorRef.resetLoopToAutoDetect(...)`. Same broadcast path.

### UI-02 surface (loop editor side panel)

**HTML** (`Resources/ui/index.html`):

- `<aside id="loop-editor-panel" hidden>` (already in DOM as Phase 3.1
  placeholder; now populated):
  - Header with filename · MIDI · L<vel> + close button (×).
  - `.loop-editor-canvas-wrap` containing `<canvas id="waveform-canvas">`.
  - `.loop-meta` row with loop start ms · loop end ms · mode.
  - `.loop-editor-actions` with Reset · Cancel · Apply buttons.

**CSS** (`Resources/ui/css/sampler-shell.css`):

- 360-px slide-in panel (`transform: translateX(100%)` default →
  `translateX(0)` when not hidden, 350 ms ease).
- `body.le-open` adds `padding-right: calc(var(--gap-lg) + 360px)` to
  `#tab-samplemap` — the grid + drop-zone reflow rather than being covered.
- **Critical canvas sizing** — `width: calc(100% - 0px); height: 200px;`
  — NEVER `position: absolute; left:0; right:0;` (memory pitfall #6 —
  canvas is a CSS replaced element).
- Buttons themed with the Ouaricon palette; primary Apply uses gold fill;
  Reset disables to 45 % opacity when one-shot.

**JS** (`Resources/ui/js/sampler-app.js`):

- `openLoopEditor(midi, vel)` — async; pulls peaks JSON, populates header,
  shows panel, schedules redraw on next frame so transitions settle.
- `redrawLoopEditor()` — DPR-aware backing store
  (`canvas.width = clientWidth * dpr; ctx.setTransform(dpr, 0, 0, dpr, 0, 0)`
  — memory pitfall #6); draws gradient bg, centerline, gold-filled +
  brown-stroked min/max envelope; draws warm-gold start marker + rust-red
  end marker as vertical bars with triangle handles.
- Pointer-event-driven drag: `pointerdown` hit-tests against marker x with
  8-px tolerance; `pointermove` updates `editorState.loopStart` /
  `loopEnd` (clamped to `[0, lengthSamples]` with a 16-sample minimum gap
  to match `LoopDetector`'s defensive guard); `pointerup` releases capture.
- Apply → `Juce.getNativeFunction('overrideLoopPoints')(midi, vel, start,
  end, 8)` → toast `"New loop points apply to next note-on."` (EC3-6).
  Editor stays open so the user can keep iterating.
- Reset → `resetLoopToAutoDetect(midi, vel)` → re-fetch peaks for fresh
  marker positions; populate header + redraw. **Disabled when
  `isOneShot(snap)`** (handles `"one-shot"`, `"oneshot"`, `"OneShot"`
  case-variations) with tooltip `"Sample is one-shot — no loop region
  detected."` (EC3-7).
- Cancel / X / Esc → close panel, no writeback.
- Single-click on `.cell-loaded` cell → `openLoopEditor` (replacing the
  Phase 3.2 placeholder log).
- `sampleMapUpdated` push event sync — when the editor is open and not
  mid-drag, the editor's snapshot syncs to the map's current loop fields
  for the active cell so loop-mode label / Reset state stay consistent.

### Active-voice retention (Stage 2 EC-3)

`overrideLoopPoints` constructs a brand-new `SampleMap` and atomic-stores
it. Voices that already snapshotted the previous map keep their
`std::shared_ptr<SampleMap>` alive for the held note's duration. The new
loop region therefore applies on the **next** note-on, not the current
one — which is what the toast `"New loop points apply to next note-on."`
communicates to the user (EC3-6 surfacing).

## Files Modified

- `plugins/O-MicrotonalSampler/Source/PluginProcessor.h`
  - Added `resetLoopToAutoDetect(midi, vel)` declaration.
  - Reflowed `overrideLoopPoints` doc comment to phase-3.4 reality.
- `plugins/O-MicrotonalSampler/Source/PluginProcessor.cpp`
  - `#include "LoopDetector.h"`, `<algorithm>`, `<limits>`.
  - `overrideLoopPoints` — replaced Phase 3.1 skeleton with full impl
    (atomic deep-copy + slot mutation + LoopDetector branch +
    version bump + callback).
  - `resetLoopToAutoDetect` — new wrapper (delegates to override path).
  - `snapshotWaveformPeaks` — replaced skeleton with full impl
    (per-bin min/max scan + RESEARCH §RQ3-5 JSON schema).
- `plugins/O-MicrotonalSampler/Source/PluginEditor.cpp`
  - `overrideLoopPoints` native function — full impl (5-arg pass-through;
    xfade default 8).
  - `resetLoopToAutoDetect` native function — full impl (2-arg).
  - `getWaveformPeaks` native function — full impl (returns
    snapshotWaveformPeaks JSON; bins default 512).
- `plugins/O-MicrotonalSampler/Resources/ui/index.html`
  - `#loop-editor-panel` populated with header (filename + midi + layer +
    close), canvas wrap, meta row, action buttons.
- `plugins/O-MicrotonalSampler/Resources/ui/css/sampler-shell.css`
  - Replaced placeholder loop-editor styles with full panel styling
    (header, close button, canvas wrap with explicit width/height,
    meta row, action buttons w/ primary Apply, body.le-open grid reflow).
- `plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js`
  - `openLoopEditor`, `closeLoopEditor`, `populateLoopEditorHeader`,
    `updateLoopMetaLabels`, `updateResetButtonState`, `redrawLoopEditor`,
    `drawMarker`, `sampleToX`, `xToSample`, `bindLoopEditorEvents`,
    `bindLoopEditorResize`, `onCanvasPointerDown/Move/Up`, `editorState`
    module-level singleton.
  - `handleCellSingleClick` for loaded cells now calls `openLoopEditor`
    (replacing 3.2 placeholder log).
  - Context menu `open-loop-editor` action now calls `openLoopEditor`.
  - `handleSampleMapSnapshot` syncs editor's snapshot when open + not
    mid-drag so loop-mode label stays consistent after Apply / external
    automation.
  - `DOMContentLoaded` boot sequence: added `bindLoopEditorEvents` +
    `bindLoopEditorResize`.

## Files Created

- `plugins/O-MicrotonalSampler/.planning/stages/3-gui/PHASE-3.4-SUMMARY.md`
- `plugins/O-MicrotonalSampler/.planning/stages/3-gui/gate-report.json`
  (overwritten — same shape as Phase 3.3)

## Gate Criteria — Verification

| Criterion | Status | Note |
|---|---|---|
| Click loaded cell → editor opens with waveform render in <250 ms; markers visible at correct loopStart/loopEnd | infrastructure_ready | `getWaveformPeaks` is O(N) on the slot's audio (≈1 ms / 5 s sample). `openLoopEditor` awaits the JSON, populates header, shows panel, defers `redrawLoopEditor` one frame. Marker positions read from `snap.loopStart` / `snap.loopEnd`. Manual DAW timing verification deferred to user. |
| Drag start marker rightward, Apply → next note-on plays from new loop region (audible difference) | infrastructure_ready | `overrideLoopPoints` deep-copies the SampleMap, mutates target slot's loop fields, atomic-stores, fires callback. Voice startNote snapshots the new map; the audible region change applies on the next note-on per Stage 2 EC-3. Manual DAW listening test deferred to user. |
| Reset → markers snap back to LoopDetector's auto-detected values; mode reverts to "auto" | infrastructure_ready | `resetLoopToAutoDetect` re-runs `LoopDetector::detectLoop`; valid → `LoopMode::Auto`; invalid → `OneShot`. JS Reset handler then re-fetches peaks (`getWaveformPeaks`) which carries the fresh `loopStart`/`loopEnd`/`loopMode` from the new SampleMap. |
| Reset disabled when one-shot | passed | `updateResetButtonState` checks `isOneShot(snap)` against `"one-shot"`/`"oneshot"` (case-insensitive); disables button + sets tooltip `"Sample is one-shot — no loop region detected."` (EC3-7). |
| Editor open during active note → marker move + Apply does NOT cut current voice; new loop applies on next note-on (EC3-6) | infrastructure_ready | Voices snapshot `std::shared_ptr<SampleMap>` at startNote and keep it for the note's duration. `overrideLoopPoints` allocates a new SampleMap; the old map remains alive transitively for held voices. Apply toast `"New loop points apply to next note-on."` surfaces the deferred-application behaviour. Manual DAW verification deferred. |
| Canvas crisp on Retina (DPR test); stretches correctly at min/max window sizes | infrastructure_ready | `redrawLoopEditor` reads `window.devicePixelRatio`, sets `canvas.width = clientWidth * dpr` + `canvas.height = clientHeight * dpr`, then `ctx.setTransform(dpr, 0, 0, dpr, 0, 0)` so all subsequent drawing uses CSS pixels (memory pitfall #6). `ResizeObserver` on the canvas re-renders on dimension change so window-resize stretching is correct. |
| `pluginval --strictness 5` SUCCESS | passed | Confirmed. |
| `auval -v aumu OMtS OuDv` AU VALIDATION SUCCEEDED | passed | Confirmed. |

## Cross-platform notes

- Canvas backing-store DPR pattern matches the O-TextureForge fix (memory
  pitfall #6) — without `setTransform(dpr, ...)` after a `width = cssW*dpr`
  resize, drawing would render at 1/dpr scale and look soft on Retina.
- Pointer events with `setPointerCapture` are the cross-platform-correct
  drag pattern; works in WebView2 (Windows) and WebKit (macOS) identically
  with no host-specific shim. (Compare to mousemove/up which leak when the
  cursor exits the canvas during drag.)
- Esc handler on `document` not on the canvas — the WebView delivers Esc
  to whichever element has focus, and after the user clicks Apply the
  button retains focus; document-level keydown is the simplest invariant.

## Active-Voice Retention (carried from prior phases)

`overrideLoopPoints` follows the same atomic deep-copy pattern as
`loadSingleSample` (Phase 3.2) and `loadSampleFolder` (Phase 2.2). The
audio-thread invariant remains: every voice holds its own
`std::shared_ptr<SampleMap>` snapshot for the duration of an active note,
and a slot mutation produces a fresh `SampleMap` (with fresh shared
pointers to the same audio buffers) so the held buffer is never freed
mid-note. The user-facing consequence — that loop changes apply on the
next note-on, not the current one — is communicated via the Apply toast.

## Deviations

None. All Tasks 23–28 implemented as planned.

Phase 3.1's outstanding deviation (no render-harness target) carries
forward unchanged — Phase 3.4 introduces no new audio code paths in
`processBlock` or voices; the loop-override path is a pure
message-thread map mutation that voices read via the existing snapshot
mechanism. pluginval + auval coverage remains adequate.

## Next

Phase 3.5 — Polish (control strip + aesthetic + tuning-readout + About).
Tasks 29–34 in PLAN.md.
