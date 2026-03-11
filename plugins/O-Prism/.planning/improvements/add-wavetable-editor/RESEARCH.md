# Research: Wavetable Editor

**Plugin:** O-Prism
**Milestone:** add-wavetable-editor
**Created:** 2026-03-08
**Phase:** Research

---

## Context Summary

From CONTEXT.md:
- **Core requirement:** Add a 5th "WAVETABLE EDITOR" tab to the WebView UI with per-frame harmonic bar editing, multi-frame selection, frame operations, and save-to-user-wavetables
- **Scope:** Harmonic bar editing, frame operations (normalize/fade/reverse/smooth), undo/redo, save edited tables. No pencil drawing, no additive-from-scratch, no cross-table morphing
- **Success criteria:** Users can view harmonic content of any frame, drag bars to edit, hear changes in real-time, apply batch operations, and save as new user wavetable

---

## Implementation Approach

### Recommended Approach

**Strategy:** Dual-layer architecture — C++ FFT engine + JS harmonic editor UI

**Description:**

The wavetable editor operates on a **working copy** of the active oscillator's wavetable, kept in a new `WavetableEditor` C++ class. When the user opens the editor tab and selects an oscillator, the current table is cloned into the working copy. All edits happen on this working copy, which is simultaneously pointed to by the oscillator for real-time audio preview.

The JS side displays harmonic bars (magnitudes of the first N harmonics) and sends edits back via native functions. On the C++ side, `setFrameHarmonics` receives updated magnitudes, performs iFFT to rebuild the time-domain frame, regenerates mipmaps for that frame, and updates guard samples. Since the oscillator reads from the same `WavetableData*`, changes are immediately audible.

Undo/redo is implemented in JavaScript as a stack of per-frame harmonic snapshots (magnitude arrays). Each edit pushes the previous state. Undo restores the snapshot and calls `setFrameHarmonics` to rebuild. This keeps the C++ side stateless regarding undo — it's purely a "set state" model.

Frame operations (normalize, fade, reverse, smooth) are computed in C++ since they involve FFT and batch processing across multiple frames. The JS side sends the operation name + selected frame indices, and C++ applies the operation, returning updated waveform/harmonic data.

**Pros:**
- Leverages existing JUCE FFT infrastructure already proven in WavetableImporter/WavetableGenerator
- JS-side undo is simple (4KB per snapshot at 256 harmonics * 2 floats) and avoids C++ undo complexity
- Working copy model prevents corruption of factory tables
- Real-time preview is automatic — oscillator already reads from WavetableData pointer

**Cons:**
- Each harmonic edit requires iFFT + mipmap regen (acceptable: single frame is ~0.1ms on modern CPU)
- Working copy doubles memory for one table during editing (~10MB for 256 frames * 10 levels)
- Undo stack lives only in JS — lost on tab switch (acceptable for v1)

### Alternative Approaches Considered

**Alternative 1: Pure JS FFT (no C++ native functions)**
- Description: Do all FFT/iFFT in JavaScript using a JS FFT library, only send final time-domain frames to C++
- Why not chosen: Doubles FFT implementation, harder to keep in sync with C++ mipmap generation, introduces JS FFT library dependency

**Alternative 2: C++ undo manager (juce::UndoManager)**
- Description: Track all edits in C++ with JUCE's built-in UndoManager
- Why not chosen: Over-engineering for v1. Requires custom UndoableAction subclasses for each edit type. JS undo is simpler and sufficient. Can upgrade later if needed.

---

## Affected Components

### Files to Modify

| File | Changes | Complexity |
|------|---------|------------|
| `PluginEditor.cpp` | Add ~6 native functions for editor, register new JS/CSS resources | High |
| `PluginEditor.h` | Add WavetableEditor member, forward declarations | Low |
| `PluginProcessor.h` | Add accessor for WavetableEditor, getEditableTable() | Low |
| `PluginProcessor.cpp` | Construct WavetableEditor, expose table access | Low |
| `Source/ui/public/index.html` | Add 5th tab button + wavetable-tab content div | Medium |
| `CMakeLists.txt` | Add new source files + JS/CSS to binary resources | Low |

### New Files

| File | Purpose |
|------|---------|
| `Source/dsp/WavetableEditor.h` | Working copy management, FFT analysis, harmonic set/get, frame operations |
| `Source/dsp/WavetableEditor.cpp` | Implementation of all editor DSP operations |
| `Source/ui/public/js/wavetable-editor.js` | Full editor UI: harmonic bars, frame strip, selection, undo/redo |
| `Source/ui/public/css/wavetable-editor.css` | Editor styling consistent with O-Prism aesthetic |

### Dependencies

**JUCE Modules Used:**
- `juce_dsp` — `juce::dsp::FFT` for forward/inverse transforms
- `juce_gui_extra` — `WebBrowserComponent` native functions (existing)
- `juce_audio_formats` — `WavAudioFormat` for save-to-WAV (existing in UserWavetableManager)

**External Dependencies:**
- None

---

## Pattern Analysis

### Existing Patterns in Codebase

**Pattern 1: Tab system**
- Location: `index.html:791-796` (tab bar), `index.html:1357-1362` (switchTab function)
- Relevance: Adding 5th tab follows identical pattern — new `<div class="tab">` + new `<div id="wavetable-tab" class="tab-content">`

**Pattern 2: Native function registration**
- Location: `PluginEditor.cpp:92-695` (addNativeFunctions method)
- Relevance: All 6 new native functions follow the same `options.withNativeFunction(name, lambda)` pattern with `args` array + `complete` callback

**Pattern 3: Existing getWavetableFrame**
- Location: `PluginEditor.cpp:426-441`
- Relevance: Already fetches frame waveform data for display — editor's `getEditorFrameWaveform` follows same pattern but reads from working copy

**Pattern 4: Binary resource registration**
- Location: `CMakeLists.txt:82-94`
- Relevance: New JS/CSS files added to `juce_add_binary_data()` SOURCES list, then mapped in `getResource()` via resource provider

**Pattern 5: FFT forward/inverse**
- Location: `WavetableGenerator.cpp:110-174` (mipmap generation)
- Relevance: Exact same FFT workflow needed for editor — forward transform to get harmonics, modify bins, inverse transform to rebuild frame

**Pattern 6: WAV save format**
- Location: `UserWavetableManager.cpp:122-149`
- Relevance: `saveEditedWavetable` reuses `saveToWav` — concatenated frames, 32-bit float, 44.1kHz mono

### Serum Reference Research

**Harmonic bins are harmonic partials, not FFT bins.** Bin 1 = fundamental, bin 2 = 2nd harmonic (octave), etc. Serum caps at 512 harmonics. For O-Prism, the configurable bin count (32/64/128/256) controls how many partials to display — a UI zoom, not a data resolution change. Internally, frames have up to 1024 partials (2048/2).

**Normalize operations (Serum):**
- "Normalize Each" — per-frame peak normalization (each frame individually scaled to ±1.0)
- "Normalize Same" — find global peak across all selected frames, apply same gain to all (preserves relative levels)
- O-Prism should implement both, defaulting to "Normalize Each"

**Fade operations (Serum):**
- "Fade Edges" — fade in at waveform start, fade out at end, based on grid size. Ensures zero-crossing at loop boundary to eliminate clicks.
- "X-Fade Edges" — crossfade the waveform start/end to create smooth loop wrapping
- O-Prism implementation: Apply to time-domain data within each selected frame. Fade region = configurable percentage of frame (e.g., 10% = 204 samples).

**Reverse (Serum):**
- "Flip Horizontal" — reverse audio within each frame
- Separate "Reverse table order" — reverses frame ordering
- O-Prism should support both: "Reverse Frames" (audio within each) and "Reverse Order" (reorder selected frames)

**Smooth (Serum):**
- Serum uses spectral low-pass filtering ("Filter") — progressively removes upper harmonics
- Also "Progressive Fade" for gentler HF rolloff
- O-Prism implementation: Apply 6dB/octave rolloff above a cutoff harmonic, configurable via a strength parameter. Operates in frequency domain (zero/attenuate high-frequency bins, then iFFT).

**Real-time preview:** Serum pre-computes wavetables and the editor modifies the same buffer the oscillator reads. O-Prism's architecture already supports this — the oscillator reads from `WavetableData*`, so pointing it at the working copy gives instant preview.

**Undo/redo:** Serum 1 has limited WT editor undo (no keyboard shortcuts). Serum 2 adds comprehensive undo. O-Prism should implement Ctrl/Cmd+Z/Shift+Z from the start — straightforward with JS snapshot stack.

---

## Complexity Assessment

### Overall Complexity: High

**Justification:**
- Files affected: 6 existing + 4 new
- DSP changes: Significant — new WavetableEditor class with FFT analysis, harmonic editing, frame operations, mipmap regeneration
- UI changes: Significant — full new tab with canvas-based harmonic bars, scrollable frame strip, operation buttons, undo/redo
- Parameter changes: None (editor uses native functions, not APVTS parameters)
- Breaking changes: No — additive feature, existing functionality untouched

### Risk Areas

1. **Audio thread safety**
   - Risk: Editing wavetable data while oscillator reads it could cause glitches/crashes
   - Mitigation: Use atomic pointer swap for the working copy. Edit into a staging buffer, then swap pointers. Alternatively, since mipmap regen is fast (<1ms for single frame), do it on message thread and accept brief inconsistency — the oscillator's trilinear interpolation handles partial updates gracefully.

2. **Large data transfer JS↔C++**
   - Risk: Sending 256 harmonic magnitudes per frame as JSON could be slow if frequent
   - Mitigation: Use compact JSON format (4 decimal places), batch updates. The `getFrameHarmonics` call returns ~2KB JSON — negligible latency. Limit update frequency to one native call per mouse drag event (requestAnimationFrame throttle).

3. **Working copy lifecycle**
   - Risk: Memory leak if user switches oscillators or tables without properly releasing working copy
   - Mitigation: Clear working copy on oscillator switch, tab switch away from editor, or plugin close. WavetableEditor destructor handles cleanup.

4. **Canvas rendering performance**
   - Risk: Drawing 256 harmonic bars + 256 frame thumbnails could be slow
   - Mitigation: Use Canvas 2D (not DOM elements). Frame strip uses pre-rendered thumbnails cached as ImageData. Only re-render visible elements. DPR-aware backing store for Retina.

---

## Domain Detection

Based on content analysis:

| Domain | Score | Keywords Found |
|--------|-------|----------------|
| DSP | 12 | processblock, filter, dsp, algorithm, buffer, sample, frequency, audio processing, fft, harmonic, inverse fft, mipmap |
| GUI | 10 | webview, ui, css, html, interface, editor, visual, layout, component, canvas |
| Polish | 1 | preset |

**Detected Domain:** mixed (DSP + GUI roughly equal)
**Recommended Execute Agent:** general-purpose (mixed domain — both significant C++ DSP work and significant JS/HTML/CSS UI work)

---

## Version Impact

### Recommended Version Bump: MINOR

**Justification:**
- [ ] Bug fix only (PATCH)
- [x] New feature, backward compatible (MINOR)
- [ ] Breaking changes (MAJOR)

**Breaking Change Analysis:**
- Parameter IDs: Unchanged
- Parameter ranges: Unchanged
- Preset format: Unchanged (editor state is transient, not saved in presets)
- Public API: Unchanged

---

## Technical Deep-Dive: C++ WavetableEditor Class

### Core API

```cpp
class WavetableEditor {
public:
    // Initialize working copy from an existing table
    void loadTable(const WavetableData* sourceTable);

    // Get harmonics for a frame (magnitudes of first N partials)
    std::vector<float> getFrameHarmonics(int frameIndex, int numBins) const;

    // Set harmonics for a frame (triggers iFFT + mipmap regen)
    void setFrameHarmonics(int frameIndex, const std::vector<float>& magnitudes);

    // Get time-domain waveform for display
    std::vector<float> getFrameWaveform(int frameIndex) const;

    // Frame operations on selected frames
    void normalizeFrames(const std::vector<int>& frames, bool perFrame);
    void fadeEdges(const std::vector<int>& frames, float fadePercent);
    void reverseFrames(const std::vector<int>& frames);     // reverse audio within each
    void reverseOrder(const std::vector<int>& frames);      // reverse frame ordering
    void smoothFrames(const std::vector<int>& frames, float strength);

    // Save to user directory
    bool saveAsUserWavetable(const juce::String& name, UserWavetableManager& manager);

    // Access working copy for oscillator preview
    WavetableData* getWorkingTable() { return workingTable.get(); }

    // Get all frame waveforms (for strip display) — downsampled
    std::vector<std::vector<float>> getAllFrameWaveforms(int samplesPerFrame) const;

    int getNumFrames() const;

private:
    std::unique_ptr<WavetableData> workingTable;
    juce::dsp::FFT fft { 11 }; // 2048-point
};
```

### FFT Harmonic Extraction

For `getFrameHarmonics(frameIndex, numBins)`:
1. Copy frame data from level 0 into FFT buffer (2048 reals + 2048 zeros)
2. Forward FFT → packed [real0, imag0, real1, imag1, ...]
3. Compute magnitude: `sqrt(real[k]^2 + imag[k]^2)` for k = 1..numBins
4. Skip DC (k=0) — not a harmonic
5. Return magnitudes normalized to 0..1 range (divide by max)

### FFT Harmonic Application

For `setFrameHarmonics(frameIndex, magnitudes)`:
1. Forward FFT the current frame to get phase information
2. For each bin k = 1..N: set `real[k] = mag[k] * cos(phase[k])`, `imag[k] = mag[k] * sin(phase[k])`
3. Zero bins above N (band-limit)
4. Mirror negative frequencies
5. Inverse FFT → time-domain frame
6. Store in working table level 0
7. Regenerate mipmaps for this frame only (not all frames)
8. Set guard samples

Key insight: **Preserve phase angles** from the original frame. Only magnitudes change via the bar editor. This prevents timbral artifacts from phase randomization.

### Per-Frame Mipmap Regeneration

Optimization: `generateMipmaps()` currently processes all frames. For editor use, add `generateMipmapsForFrame(WavetableData& table, int frameIndex)` that only regenerates the 10 mipmap levels for a single frame. This makes real-time editing feasible (~0.05ms per frame vs ~12ms for 256 frames).

---

## Technical Deep-Dive: JS Editor UI

### Component Structure

```
wavetable-tab
├── osc-toggle (A / B buttons)
├── frame-strip-container
│   └── frame-strip-canvas (scrollable, all frame thumbnails)
├── harmonic-editor-container
│   ├── bin-count-selector (32/64/128/256)
│   └── harmonic-canvas (draggable bars)
├── operations-bar
│   ├── normalize-btn (dropdown: Each / Same)
│   ├── fade-btn
│   ├── reverse-btn (dropdown: Audio / Order)
│   ├── smooth-btn (with strength slider)
│   └── save-btn
└── undo-redo-bar
    ├── undo-btn
    └── redo-btn
```

### Undo System

```javascript
class UndoStack {
    constructor(maxSize = 50) {
        this.stack = [];
        this.index = -1;
        this.maxSize = maxSize;
    }
    push(frameIndex, harmonics) {
        // Truncate forward history
        this.stack = this.stack.slice(0, this.index + 1);
        this.stack.push({ frameIndex, harmonics: [...harmonics] });
        if (this.stack.length > this.maxSize) this.stack.shift();
        this.index = this.stack.length - 1;
    }
    undo() { return this.index >= 0 ? this.stack[this.index--] : null; }
    redo() { return this.index < this.stack.length - 1 ? this.stack[++this.index] : null; }
}
```

Each entry is ~1KB (256 floats * 4 bytes). Stack of 50 = ~50KB. Negligible.

---

## Open Questions

1. **Phase editing:** Should the harmonic editor show both magnitude AND phase bars (like Serum's two-row display), or magnitude only for v1? Magnitude-only is simpler but limits editing power.
   - **Recommendation:** Magnitude-only for v1. Phase editing can be added later.

2. **Frame strip downsampling:** How many samples per thumbnail in the scrollable strip? 64 pixels wide per frame seems reasonable — downsample 2048 → 64 via min/max pairs for waveform display.

3. **Live preview oscillator routing:** When editing, should we always preview through Osc A/B as selected, or provide a dedicated "preview" oscillator that bypasses effects? Using the actual oscillator is simpler and shows the real sound.
   - **Recommendation:** Use actual oscillator. User hears exactly what they'll get.

---

## Next Phase

This research document feeds into the **Plan** phase, which will:
- Create atomic task breakdown
- Define dependencies between tasks
- Set verification criteria per task
- Determine execution order

---

*Generated by improve-milestone research phase*
