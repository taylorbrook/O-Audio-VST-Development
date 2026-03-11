# Wavetable Editor — Requirements Context

## Milestone
- **Slug:** add-wavetable-editor
- **Plugin:** O-Prism
- **Version:** 1.9.0 → 1.10.0 (MINOR)
- **Date:** 2026-03-08

## Summary

Add a basic wavetable editor as a 5th tab ("WAVETABLE EDITOR") in the O-Prism WebView UI. Allows users to view, edit, and save wavetable frames using per-frame harmonic bar editing, multi-frame selection, and frame-level operations. Follows Serum's wavetable editor as the reference implementation.

## Requirements

### UI Structure
- New **"WAVETABLE EDITOR"** tab (5th tab after Synth, Effects, Tuning, Mod)
- **Osc A / Osc B toggle** at top of editor to switch which oscillator's table is being edited
- **Scrollable waveform strip** showing all frames as small thumbnails — horizontal scroll for tables with many frames
- **Per-frame harmonic bar display** showing frequency-domain content of the selected frame
- **Configurable bin count** (32/64/128/256) — investigate Serum's approach for how bins map to harmonics/FFT

### Frame Selection
- **Click** — select single frame
- **Shift+click** — range select (from last selected to clicked)
- **Ctrl/Cmd+click** — toggle individual frame in/out of selection

### Harmonic Editing
- Harmonic bars are directly editable by click-dragging bar heights
- Changes apply **immediately via inverse FFT** — real-time waveform preview as bars are dragged
- Edits should be audible in real-time if the oscillator is playing (live preview)
- **Undo/redo** support for harmonic edits

### Frame Operations (on selected frames)
- **Normalize** — investigate Serum's implementation for reference
- **Fade** — investigate Serum's implementation for reference
- **Reverse** — investigate Serum's implementation for reference
- **Smooth** — investigate Serum's implementation for reference
- All operations support undo/redo

### Save & Persistence
- Save edited tables as `.wav` to `~/.ouaricon/wavetables/`
- Saving always creates a **new entry** in the user wavetable list (never overwrites factory tables)
- Users **can edit factory tables** but saving produces a new user table
- Saved tables appear in the user wavetable dropdown immediately

### Editor ↔ Oscillator Link
- Editing is live — changes to harmonics are audible in real-time while playing
- The editor operates on the active oscillator's table (Osc A or Osc B based on toggle)

### C++ Native Functions (≈6 new)
1. `getEditorFrameWaveform` — get time-domain waveform data for a specific frame
2. `getFrameHarmonics` — get frequency-domain harmonic magnitudes for a frame
3. `setFrameHarmonics` — set harmonic magnitudes for a frame (triggers iFFT rebuild)
4. `applyFrameOperation` — apply normalize/fade/reverse/smooth to selected frames
5. `saveEditedWavetable` — save current edited table as .wav to user directory
6. `getAllFrameWaveforms` — get all frame waveforms for the scrollable strip display

## Design Reference
- **Primary reference:** Xfer Serum wavetable editor
- Investigate Serum's approach for: bin count meaning, normalize/fade/reverse/smooth behavior, harmonic display conventions

## Constraints
- Must integrate with existing WebView UI architecture (resource provider, native functions)
- Must work with existing `WavetableOscillator` and `UserWavetableManager` classes
- Must not break existing wavetable import functionality (v1.9.0)
- Frame size is 2048 samples (established standard in O-Prism)
- Up to 256 frames per table

## Out of Scope (for this milestone)
- Waveform drawing (pencil tool) — future enhancement
- Additive synthesis from scratch — only editing existing tables
- Wavetable morphing between different tables
- 3D wavetable visualization in editor (existing 3D view stays in Synth tab)
