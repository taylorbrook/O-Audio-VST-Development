---
title: "O-MicrotonalSampler Stage 3 (GUI) — Context"
created: 2026-04-27
stage: 3-gui
phase: discuss
status: complete
---

# Stage 3 (GUI) — Context

## Discussion Summary

**Date:** 2026-04-27
**Participants:** User, Claude
**Inputs reviewed:** BRIEF.md, REQUIREMENTS.md, STATUS.md, Stage 2 VERIFICATION.md / SUMMARY artefacts, existing `Source/PluginEditor.{h,cpp}` (Phase 2.2 placeholder), suite tuning-panel reference (O-Bells `Resources/ui/`).

## Inheritance from Stage 2 (frozen surface)

- 7-parameter APVTS frozen and bound: `attack`, `decay`, `sustain`, `release`, `polyphony`, `velocity_crossfade`, `output_gain`.
- `OMicrotonalSamplerAudioProcessor::loadSampleFolder(juce::File)` exists and is the load entry point.
- `getLastSkippedFiles()` returns `juce::StringArray` — read-only surface for "skipped files" UI feedback.
- `currentSampleMap` is a `std::shared_ptr<SampleMap>` mutated by the background `SampleLoader` and atomic-loaded by voices. UI must read snapshot via `std::atomic_load`, not the bare member.
- Voice-side DSP (varispeed, ADSR, vel xfade, voice-steal, loop auto-detect) is verified — Stage 3 is purely additive on the editor side.
- `Source/PluginEditor.{h,cpp}` is the Phase 2.2 placeholder (`GenericAudioProcessorEditor` + Load Folder button, 500×400). To be **replaced wholesale** by the WebView editor.

## Requirements Confirmed (Stage 3 scope)

Per REQUIREMENTS.md §Traceability, Stage 3 verifies 5 requirements:

- **FUNC-05** Drag-drop folder load with filename-convention auto-mapping (UI surface — DSP path landed in 2.2).
- **FUNC-06** Per-cell manual sample assignment (override path).
- **DSP-06** Manual loop-point override per sample (UI surface; Loop-detect engine landed in 2.5, override hook needs writeback path).
- **UI-01** Sample-mapping grid (pitch × velocity layer) is the primary editing surface.
- **UI-02** Loop-point editor with waveform view and draggable markers (on demand).

Out of scope this stage: anything in BRIEF.md "Out of Scope (v1.0)" — round-robin, onboard FX, sample browser/preset library, mono/legato.

## Approach Decisions (Locked)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D3-1 | UI tech stack | **WebView** (`juce::WebBrowserComponent` with resource provider + `Juce` JS bridge) | Matches O-Bells / O-Lyrica / O-Wind / O-Formant / O-Prism. Memory recipe (NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1) is non-negotiable for Windows correctness. |
| D3-2 | Aesthetic | **Ouaricon house aesthetic** | Suite consistency. Pull palette/typography/control conventions from O-Bells (closest analog: tabs + tuning panel + bottom control band). |
| D3-3 | Mockup phase | **Skip dedicated `/ui-mockup`** — design is specified here in prose; ui-finalization-agent generates production HTML during execute (3.1) | Design space is constrained (one novel surface = the sample-mapping grid; everything else inherits suite patterns). Mockup iteration would be lower-value than direct execute. Re-opens to a separate ui-mockup pass only if 3.1 review surfaces visual ambiguity. |
| D3-4 | Top-level layout | **Tabbed interface**: `Sample Map` (default) ⟂ `Tuning` ⟂ optional `About` | Matches suite norm (O-Bells). TuningPanel demands its own tab — don't bury it in chrome. |
| D3-5 | Sample-map grid | **Horizontal piano-strip × 4 stacked vel-layer rows** | Keyboard metaphor is universally legible; matches Kontakt/EXS24 mental model; window width drives octave compression naturally; 4-row stack scales gracefully when only 1–3 layers loaded (empty rows hidden). |
| D3-6 | Loop-point editor surface | **Side panel within Sample Map tab** — clicking a loaded cell slides the grid into a narrower column and reveals the editor on the right | Keeps grid context, no z-order traps, matches O-Bells side-panel idiom. Closes via header X or Escape. |
| D3-7 | Tuning panel integration | **Carry the suite TuningPanel** (`tuning-panel.js` + `tuning-panel.css`) into `Resources/ui/` and mount on Tuning tab via `<div id="tuning-container">` + dynamic import — exact O-Bells pattern | TuningPanel is currently copy-pasted across 5 plugins (no shared module). Match the established pattern for v1.0; the in-flight `generalize-microtones` skill will extract it later. Read-only consumption — no APVTS for tuning. |
| D3-8 | Control strip placement | **Bottom band, always visible across tabs** | ADSR + Velocity Crossfade + Output Gain are global controls, not tab-scoped. Matches O-Bells bottom-strip convention. |
| D3-9 | Drag-drop folder zone | **Promotes Phase 2.2 button to a drop region inside Sample Map tab** — also keeps a "Load Folder…" button as accessibility fallback | Drop region uses HTML5 dragenter/dragover/drop; resolves the dropped path through C++ via a `Juce` bridge function that calls `processorRef.loadSampleFolder(...)`. Button uses existing FileChooser path. |
| D3-10 | Skipped-files surfacing | **Toast + collapsible "Issues" disclosure inside Sample Map tab** sourced from `getLastSkippedFiles()` | Toast appears on load completion (3 s auto-dismiss); detail panel persists for the session. Empty array = no toast. |
| D3-11 | JS↔C++ relay strategy | **Parameter attachments via `juce::WebSliderRelay` / `WebComboBoxRelay`** for the 7 APVTS params + a custom `WebControlRelay` named `sampleMap` that mirrors a JSON snapshot of `currentSampleMap` to JS | Snapshot rebroadcast on background-loader completion (callback already runs on message thread). Per-cell drag-drop replace POSTs a path through the relay; C++ updates the SampleMap and rebroadcasts. |
| D3-12 | Per-cell replace path (FUNC-06) | **Click cell → native FileChooser → on selection, swap that one slot in `currentSampleMap` via a single-cell loader call** | Reuses 2.2 loader machinery (background thread, atomic swap). Drop on cell from OS DnD also lands here. Slot resolves to (pitch × velocity-layer); replacing an empty slot is just an insert. |
| D3-13 | Loop-override path (DSP-06) | **UI emits `(slotKey, loopStart, loopEnd, crossfadeLen)` as JSON via relay; processor swaps the slot's loop metadata in the live SampleMap (atomic shared_ptr replace at the slot granularity)** | Stage 2.5 LoopDetector's outputs are stored on each `SampleSlot` — the override writes into the same fields. Voices reading mid-note keep their own snapshot via shared_ptr; new note-ons see new loop points. No allocations on audio thread. |
| D3-14 | Window size | **Resizable, default 900×640, min 720×480, max 1600×1080**; cells reflow on resize | Wider than the placeholder's 500×400 to fit the piano strip; min size still shows ~3 octaves at a time + control strip. Resize via `juce::Component::setResizable(true, true)` + JS-side viewport recompute. |
| D3-15 | Sub-stage execution order | **3.1 → 3.2 → 3.3 → 3.4 → 3.5** | DSP-style first-pass: shell + relays first (3.1), then the highest-risk surface (the grid, 3.2), then drag-drop polish (3.3), then the optional UI-02 loop editor (3.4), then control strip + aesthetic polish (3.5). Tuning tab works from 3.1 onward (TuningPanel is already a vetted module). |

## Sub-stage Plan

| Phase | Goal | Verifies | Gate |
|---|---|---|---|
| 3.1 | WebView shell + tab framework + TuningPanel mount + 7-param relays + sampleMap JSON relay scaffold | (none — infra) | Plugin opens in DAW; 7 sliders move; Tuning tab renders TuningPanel; sampleMap JSON arrives on load |
| 3.2 | Sample-mapping grid (FUNC-06 + UI-01) — 88-key piano strip × 4 vel rows; cell states (loaded / empty / active / stolen); per-cell click-to-replace via FileChooser; OS DnD onto cell | FUNC-06, UI-01 | Replacing one cell mid-session retunes only that note; grid reflects load state in <100 ms |
| 3.3 | Folder drop-zone (FUNC-05) — promotes button to drop region; skipped-files toast + disclosure (D3-10) | FUNC-05 | Drag a folder onto the zone → loads identically to the button path; skipped files appear in disclosure |
| 3.4 | Loop-point editor side panel (DSP-06 + UI-02) — waveform render, draggable start/end markers, crossfade-length slider, "Reset to auto-detect" button | DSP-06, UI-02 | Editing loop points produces audibly different sustain on next note-on; reset restores the auto-detected values |
| 3.5 | Bottom control strip + Ouaricon aesthetic polish + tuning-state readout in chrome (e.g. "12-EDO" / "Werckmeister III" / current Scala filename) | (visual polish; no new requirements) | Visual review against O-Bells aesthetic; window resize behaves; pluginval --strictness 5 still SUCCESS |

Each sub-stage commits atomically with its gate-report and SUMMARY (matches Stage 2 cadence).

## Constraints Identified

- **Cross-platform WebView (memory):** `NEEDS_WEBVIEW2 TRUE` in `juce_add_plugin()` AND `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` compile def. Skipping either silently breaks Windows.
- **Resource provider receives PATHS, not URLs (memory):** Compare `url == "/" || url == "/index.html"`; never strip schemes.
- **Windows WebView2 user data folder (memory):** `withUserDataFolder(File::getSpecialLocation(File::tempDirectory).getChildFile("OMicrotonalSampler_WebView"))` on `JUCE_WINDOWS`.
- **RT-safety carries forward (PERF-01):** Editor-driven SampleMap mutations land on the message thread; audio thread reads via `std::atomic_load`. No new allocations on `processBlock`.
- **No mid-note SampleMap swap (Stage 2 EC-3):** Active voices keep their old shared_ptr snapshot; new notes pick up the new map. The cell-replace UX must communicate this — no jarring "voices died" effect; visual cue on cells whose old buffer is being held by an active voice is acceptable but not required for v1.0.
- **TuningPanel is read-only here:** Plugin must not expose tuning *editing* — the TuningPanel's edit affordances (if any in the copied module) need to be feature-flagged off or the panel mounted in display-only mode. Confirm during 3.1.
- **Latency contract (PERF-04):** Editor open/close must not introduce audio-thread latency (no `setLatencySamples` changes). `createEditor` only spawns the WebView; processor stays untouched.
- **Stage 1/2 surface invariant:** No method-signature churn on `OMicrotonalSamplerAudioProcessor` public API except *additions* needed for per-cell loader and loop-override relays. Document any additions in the 3.1 SUMMARY.

## Edge Cases to Handle

| # | Case | Stage 3 behavior |
|---|---|---|
| EC3-1 | User drops a non-folder onto the folder drop zone | Reject silently; no error dialog (matches Phase 2.2 button — `folder.isDirectory()` guard). Optional toast: "Drop a folder, not a file." |
| EC3-2 | User drops a file onto a *cell* (per-cell replace) | Accept any single `.wav`/`.aif`; reject anything else with a brief toast. |
| EC3-3 | User drops a folder onto a cell | Treat as "use the closest match from this folder for this cell" — or simpler: disallow and route to the top-level drop zone. **Decision:** disallow for v1.0; toast: "Drop a single file on a cell, or a folder on the top zone." |
| EC3-4 | Loaded SampleMap is empty (no parseable files) | Grid shows all cells empty; toast surfaces every skipped file's reason. |
| EC3-5 | Cell click during folder load (background thread still running) | Per-cell replace queues behind the bulk load; UI shows a "loading…" indicator on the cell. After bulk load completes, the queued replace runs. |
| EC3-6 | Loop-editor opened on a slot whose buffer is being held by an active voice | Editor renders normally; "Apply" button writes new loop fields; toast: "New loop points apply to next note-on." |
| EC3-7 | Loop-editor "Reset to auto-detect" with no detection result (one-shot fallback was used) | Disable button + tooltip: "Sample is one-shot — no loop region detected." |
| EC3-8 | Window resize below min — JUCE clamps; JS viewport listener must handle the clamped size without overflow | Use CSS grid + `calc()`-driven cell widths; never assume fixed pixel widths. |
| EC3-9 | TuningPanel module fails to import (path typo, missing file) | Tuning tab shows a graceful fallback message; rest of UI still works. Log to stderr. |
| EC3-10 | User opens editor *during* `loadSampleFolder` background thread → JS arrives before snapshot | First `sampleMap` relay broadcast happens on completion. UI shows "Loading…" placeholder until first message arrives. |
| EC3-11 | Editor opened, closed, re-opened mid-session | Re-init pulls fresh snapshot from C++. WebView state is ephemeral; processor is canonical. |

## UI Information Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│  [O-MicrotonalSampler]   [Sample Map][Tuning][About?]   ⋯        │ ← header + tabs + tuning-state readout
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│   ┌─ Sample Map tab ─────────────────────────────────────────┐   │
│   │  ┌─ Folder drop zone ──────────────────────────────────┐ │   │
│   │  │  Drop folder here · or · [Load Folder…]             │ │   │
│   │  └─────────────────────────────────────────────────────┘ │   │
│   │                                                          │   │
│   │  ┌─ Piano strip × 4 vel-layer rows ─────────────────────┐│   │
│   │  │  v4 [██░██░░██░██░██░░██░██░██░░██░██░██░░██░██░██…] ││   │
│   │  │  v3 [█▒░██░░██░██░██░░██░██░██░░██░██░██░░██░██░██…] ││   │
│   │  │  v2 [██░██░░░░░██░██░░██░██░██░░██░██░██░░██░██░██…] ││   │
│   │  │  v1 [██░██░░██░██░██░░██░██░██░░██░██░██░░██░██░██…] ││   │
│   │  └──────────────────────────────────────────────────────┘│   │
│   │                                                          │   │
│   │  ▸ Issues (3 files skipped)                              │   │ ← collapsible
│   └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│   (when cell is selected, grid shrinks left and editor slides    │
│    in from the right within this same tab body)                  │
│                                                                  │
├──────────────────────────────────────────────────────────────────┤
│  Attack   Decay   Sustain   Release   Poly   Vel-XF   Out Gain   │ ← bottom control strip (always visible)
└──────────────────────────────────────────────────────────────────┘
```

## Open Questions for Research Phase

| # | Question | Why research-stage |
|---|---|---|
| RQ3-1 | TuningPanel display-only mode | Inspect the O-Bells `tuning-panel.js` API surface — does it expose a `readonly` flag, or do we need to gate UI affordances at mount time? Determines D3-7 implementation detail. |
| RQ3-2 | SampleMap JSON snapshot schema | Define the minimum fields per slot (filename, length-samples, source-SR ratio applied?, loop-detected?, loop-start/end, vel-layer index, pitch). Optimize for diff-friendly relay updates. |
| RQ3-3 | Per-cell loader API | Add a new `OMicrotonalSamplerAudioProcessor::loadSingleSample(int midiPitch, int velLayer, juce::File)` or extend `SampleLoader` with single-slot mode? Confirm RT-safe atomic-merge semantics. |
| RQ3-4 | Loop-override writeback path | Where does the override land? Mutate the existing `SampleSlot` in place under a SpinLock-free swap, or rebuild a new SampleMap with the slot replaced? Latter matches Stage 2 EC-3 mental model. |
| RQ3-5 | Waveform render strategy | Pre-render a small PNG-style peak summary on the message thread when the cell is selected, or stream raw PCM-derived peaks to JS via the relay? Latter is more responsive; former is simpler. |
| RQ3-6 | Drag-drop on cell — OS DnD vs HTML5 DnD | WebView HTML5 `dataTransfer.files` may not give a real path on macOS sandboxed builds. Confirm against suite precedent (O-Bells doesn't ship per-cell DnD; check O-TextureForge or similar). May need to wire JUCE's `FileDragAndDropTarget` on the host component instead. |
| RQ3-7 | Aesthetic asset inventory | Lift Ouaricon CSS variables, fonts, button styles, and any botanical-overlay assets from O-Bells `Resources/ui/` directly, or pull from the latest aesthetic-template skill output? Confirm the canonical source. |
| RQ3-8 | Resource bundling and `BinaryData` | Confirm whether HTML/JS/CSS get baked via JUCE BinaryData or served via `withResourceProvider` from a `Resources/ui/` source dir during dev. Match O-Bells' choice for consistency. |

## Performance Targets

- **Editor open latency:** ≤ 250 ms cold open on Apple Silicon (window appears, grid renders, params bind).
- **Cell-replace UI feedback:** ≤ 100 ms from drop/click to visual state change (file load itself runs background; UI shows pending state).
- **Folder-load UI feedback:** identical to Phase 2.2 button path — bulk load runs background, UI shows progress + finalizes on completion callback.
- **Audio thread:** unchanged from Stage 2 — Stage 3 must not regress PERF-01 / PERF-02 / PERF-04.
- **No editor → audio thread coupling:** editor closing must not stop voices; reopening must not glitch.

## Aesthetic Direction (Ouaricon house)

Reference: O-Bells `Resources/ui/index.html` + `css/tuning-panel.css`. Key conventions to inherit:

- Cream/parchment background (`rgba(245, 230, 211, ...)`), warm-brown text (`#3C2F2F`), antique-gold accent (`#B8860B`).
- Tab strip in header with active-tab underline.
- Bottom control strip with knob + label + numeric readout pattern.
- Botanical overlay decoration (subtle, non-functional).
- Tuning tab contributes its own theme overrides (already wired in O-Bells; we replicate verbatim).

Concrete asset list deferred to research phase (RQ3-7).

## Next Phase

Ready for: **research** phase

`/plugin-research O-MicrotonalSampler 3-gui`
