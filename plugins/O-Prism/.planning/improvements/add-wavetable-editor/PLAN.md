---
milestone: add-wavetable-editor
domain: mixed
execute_agent: general-purpose
version_bump: minor
base_version: 1.9.0
target_version: 1.10.0
created: 2026-03-08
---

# Plan: Wavetable Editor for O-Prism

## Task Breakdown

### Task 1: Per-Frame Mipmap Regeneration
**File:** `Source/dsp/WavetableGenerator.h`, `Source/dsp/WavetableGenerator.cpp`
**Dependencies:** None
**Estimated complexity:** Low

Add `generateMipmapsForFrame(WavetableData& table, int frameIndex)` to WavetableGenerator. The existing `generateMipmaps()` processes all frames across all 10 levels. The editor needs to regenerate mipmaps for a single frame after harmonic edits (~0.05ms vs ~12ms for all 256 frames).

**Steps:**
1. Add static method declaration in WavetableGenerator.h
2. Implement in .cpp: for level 1..9, forward FFT the level-0 frame, zero bins above `(1024 >> level)`, inverse FFT, store in `table.getFrameData(level, frameIndex)`, set guard sample
3. Reuse the same FFT buffer pattern from existing `generateMipmaps()`

**Verification:**
- [ ] Method compiles and links
- [ ] Calling it on a single frame produces identical mipmap data as the full `generateMipmaps()` for that frame

---

### Task 2: WavetableEditor C++ Class
**Files:** `Source/dsp/WavetableEditor.h` (new), `Source/dsp/WavetableEditor.cpp` (new)
**Dependencies:** Task 1
**Estimated complexity:** High

Core DSP engine for the wavetable editor. Manages a working copy of the active oscillator's table and provides FFT-based harmonic analysis/synthesis.

**Steps:**

1. **Header (WavetableEditor.h):**
   - Include WavetableData.h, WavetableGenerator.h, juce_dsp FFT
   - Class with: `loadTable(const WavetableData* source)`, `getFrameHarmonics(int frame, int numBins)`, `setFrameHarmonics(int frame, const std::vector<float>& mags)`, `getFrameWaveform(int frame)`, `getAllFrameWaveforms(int samplesPerFrame)`, frame operations (normalize, fade, reverse, reverseOrder, smooth), `saveAsUserWavetable(name, UserWavetableManager&)`, `getWorkingTable()`, `getNumFrames()`, `hasWorkingTable()`
   - Private members: `std::unique_ptr<WavetableData> workingTable`, `juce::dsp::FFT fft{11}`

2. **Implementation (WavetableEditor.cpp):**
   - `loadTable()`: Deep-copy source table level 0 into new WavetableData, then generate all mipmaps via WavetableGenerator::generateMipmaps
   - `getFrameHarmonics()`: Forward FFT on level-0 frame data, extract magnitudes for bins 1..numBins (skip DC), normalize to 0..1
   - `setFrameHarmonics()`: Forward FFT to get current phases, reconstruct complex bins from new magnitudes + original phases, zero bins above N, mirror negative frequencies, inverse FFT, store in level 0, call `generateMipmapsForFrame()`, set guard samples
   - `getFrameWaveform()`: Return level-0 frame data as vector (2048 samples)
   - `getAllFrameWaveforms()`: For each frame, downsample level-0 to `samplesPerFrame` via min/max pairs for waveform display
   - `normalizeFrames(frames, perFrame)`: Per-frame peak normalize to ±1.0 (or global peak across all selected)
   - `fadeEdges(frames, fadePercent)`: Linear fade-in/out at frame boundaries (time domain)
   - `reverseFrames(frames)`: Reverse audio within each selected frame
   - `reverseOrder(frames)`: Swap frame data to reverse ordering of selected frames
   - `smoothFrames(frames, strength)`: FFT each frame, apply 6dB/oct rolloff above cutoff harmonic (cutoff = strength * 1024), inverse FFT
   - `saveAsUserWavetable()`: Use UserWavetableManager's save pattern — concatenate level-0 frames into AudioBuffer, write as 32-bit float WAV to `~/.ouaricon/wavetables/`, register in manager, return name

**Verification:**
- [ ] `loadTable()` produces a deep copy — modifying working copy does not affect source
- [ ] `getFrameHarmonics()` returns sensible magnitudes (sine wave → single peak at bin 1)
- [ ] `setFrameHarmonics()` round-trips: get harmonics → set same harmonics → waveform unchanged
- [ ] Frame operations modify working table data correctly
- [ ] `saveAsUserWavetable()` creates valid WAV readable by UserWavetableManager

---

### Task 3: PluginProcessor Integration
**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
**Dependencies:** Task 2
**Estimated complexity:** Low

Expose WavetableEditor to PluginEditor and provide methods to link editor working copy to oscillator for live preview.

**Steps:**
1. Add `#include "dsp/WavetableEditor.h"` and `WavetableEditor wavetableEditor;` member in PluginProcessor.h
2. Add accessor: `WavetableEditor& getWavetableEditor() { return wavetableEditor; }`
3. Add `startEditing(int oscIndex)` method: loads the active table (user or factory) for the specified oscillator into WavetableEditor's working copy, then points the oscillator at the working copy via the existing user table atomic pointer mechanism
4. Add `stopEditing(int oscIndex)` method: if user saved, the saved table is already in UserWavetableManager — point oscillator to saved table or revert to factory. Clear working copy.
5. Add `getEditingOscIndex()` accessor to track which osc is being edited (-1 = none)

**Verification:**
- [ ] `startEditing(0)` clones Osc A's table into working copy and oscillator plays from it
- [ ] `stopEditing(0)` reverts oscillator to its original table source
- [ ] Editing working copy data is audible in real-time through the oscillator

---

### Task 4: Native Functions in PluginEditor
**Files:** `Source/PluginEditor.cpp`, `Source/PluginEditor.h`
**Dependencies:** Task 3
**Estimated complexity:** Medium

Add 8 native functions following the existing `withNativeFunction()` pattern.

**Steps:**

1. **`startWavetableEditor(oscIndex)`** — Call `processorRef.startEditing(oscIndex)`, return frame count + current harmonics for frame 0
2. **`stopWavetableEditor()`** — Call `processorRef.stopEditing(currentOsc)`, clean up
3. **`getEditorFrameWaveform(frameIndex)`** — Get time-domain waveform from working copy, return as JSON float array (strided to ~256 points for display)
4. **`getFrameHarmonics(frameIndex, numBins)`** — Get magnitude array from WavetableEditor, return as JSON float array
5. **`setFrameHarmonics(frameIndex, magnitudesJsonArray)`** — Parse magnitudes from args, call WavetableEditor::setFrameHarmonics, return updated waveform for display
6. **`applyFrameOperation(opType, frameIndicesArray, params)`** — Parse operation type string ("normalize", "normalizeGlobal", "fade", "reverse", "reverseOrder", "smooth"), parse frame indices, call appropriate WavetableEditor method, return updated waveform data for affected frames
7. **`saveEditedWavetable(name)`** — Call WavetableEditor::saveAsUserWavetable, return success/error + new table name
8. **`getAllFrameWaveforms(samplesPerFrame)`** — Get downsampled waveforms for all frames, return as JSON 2D array

All functions follow the pattern: `[this](const juce::Array<juce::var>& args, auto complete) { ... complete(result); }`

**Verification:**
- [ ] Each native function is callable from JavaScript
- [ ] `setFrameHarmonics` triggers audible change in oscillator output
- [ ] `saveEditedWavetable` creates a .wav file in `~/.ouaricon/wavetables/`
- [ ] Error cases (invalid frame index, no working table) return error responses gracefully

---

### Task 5: Wavetable Editor CSS
**File:** `Source/ui/public/css/wavetable-editor.css` (new)
**Dependencies:** None
**Estimated complexity:** Low

Styling consistent with O-Prism's existing dark naturalist aesthetic (greens, tans, dark backgrounds).

**Steps:**
1. Container layout: full-height flex column
2. Osc toggle bar: top row with A/B buttons (styled like existing tab buttons)
3. Frame strip: horizontal scrollable container, fixed height (~80px), thumbnails with selection highlighting
4. Harmonic editor: flex-grow canvas area with bin count selector
5. Operations bar: row of styled buttons at bottom
6. Undo/redo bar: small icon buttons
7. Color scheme: match existing `--bg-main`, `--accent-green`, `--text-primary` CSS variables
8. Selected frame highlight: accent green border/glow
9. Harmonic bar color: green gradient matching vine-arc knob aesthetic

**Verification:**
- [ ] Styles don't conflict with existing tab content
- [ ] Layout is responsive within reasonable plugin window sizes (800x600 to 1400x900)

---

### Task 6: Wavetable Editor JavaScript
**File:** `Source/ui/public/js/wavetable-editor.js` (new)
**Dependencies:** Task 4 (native functions must exist), Task 5 (CSS must exist)
**Estimated complexity:** High

Full editor UI with harmonic bar editing, frame strip, selection, operations, and undo/redo.

**Steps:**

1. **Module structure:**
   - ES module pattern (IIFE or class) to avoid polluting global scope
   - Export `initWavetableEditor()` called from main page when tab first activated
   - Export `onTabActivated()` / `onTabDeactivated()` for lifecycle

2. **Osc A/B toggle:**
   - Two buttons, one active at a time
   - Switching calls `stopWavetableEditor()` then `startWavetableEditor(newOscIndex)`
   - Reloads frame strip and harmonic display for new oscillator

3. **Frame strip (Canvas-based):**
   - Horizontal scrollable container
   - Call `getAllFrameWaveforms(64)` to get downsampled waveforms
   - Render each frame as a mini waveform thumbnail (64px wide, ~60px tall)
   - Click = select single frame, Shift+click = range select, Ctrl/Cmd+click = toggle
   - Selected frames: highlighted border (accent green)
   - Active frame (for harmonic editing): brighter highlight
   - Frame numbers displayed below thumbnails

4. **Harmonic bar editor (Canvas-based):**
   - Full-width canvas showing harmonic magnitudes as vertical bars
   - Bin count selector (32/64/128/256 buttons) — controls display resolution
   - Bars drawn from bottom up, height = magnitude * canvas height
   - Mouse interaction: click-drag to set bar heights
   - On mousedown: capture starting position, push current harmonics to undo stack
   - On mousemove: update bar height, call `setFrameHarmonics()` via native function
   - Throttle native calls to requestAnimationFrame cadence
   - On mouseup: finalize edit
   - DPR-aware canvas backing store for Retina rendering (use `canvas.width = clientWidth * dpr`)

5. **Waveform preview panel:**
   - Small canvas above or beside harmonic editor showing time-domain waveform of active frame
   - Updated after each `setFrameHarmonics` call (response includes updated waveform)

6. **Operations bar:**
   - Buttons: Normalize (dropdown: Each/Global), Fade (slider for %), Reverse (dropdown: Audio/Order), Smooth (slider for strength), Save
   - Each button calls `applyFrameOperation()` with selected frame indices
   - After operation completes, refresh frame strip thumbnails for affected frames and harmonic display
   - Save button opens a text input modal for wavetable name, then calls `saveEditedWavetable(name)`

7. **Undo/redo system:**
   - UndoStack class: push(frameIndex, harmonics), undo(), redo()
   - Max 50 entries (~50KB)
   - Ctrl/Cmd+Z = undo, Ctrl/Cmd+Shift+Z = redo
   - Undo restores harmonics via `setFrameHarmonics()` native call
   - Frame operations push one undo entry per affected frame (or a batch entry)

8. **Tab lifecycle:**
   - On first tab activation: call `startWavetableEditor(0)`, load frame strip, select frame 0
   - On tab switch away: call `stopWavetableEditor()` to release working copy
   - On tab switch back: re-initialize with current oscillator

**Verification:**
- [ ] Frame strip renders all frames with correct waveform thumbnails
- [ ] Clicking a frame selects it and loads its harmonics in the bar editor
- [ ] Dragging harmonic bars changes magnitudes and triggers live audio preview
- [ ] Multi-frame selection works (shift+click, ctrl+click)
- [ ] Frame operations apply to all selected frames
- [ ] Undo/redo restores previous harmonic states
- [ ] Save creates a new user wavetable visible in the dropdown
- [ ] Bin count selector changes number of displayed bars

---

### Task 7: Tab Integration in index.html
**File:** `Source/ui/public/index.html`
**Dependencies:** Task 5, Task 6
**Estimated complexity:** Low

Add 5th tab to existing tab bar and content area.

**Steps:**
1. Add tab button after "Effects": `<div class="tab" data-tab="wavetable" onclick="switchTab('wavetable')">Wavetable</div>`
2. Add tab content div after effects-tab: `<div id="wavetable-tab" class="tab-content">` with placeholder structure (osc toggle, frame strip container, harmonic editor container, operations bar)
3. Add `<script>` tag to load wavetable-editor.js
4. Add `<link>` or inline reference to wavetable-editor.css
5. Hook tab switch: call `onTabActivated()`/`onTabDeactivated()` in `switchTab()` function for the wavetable tab

**Verification:**
- [ ] 5th tab button appears and is clickable
- [ ] Tab content shows/hides correctly
- [ ] Other tabs still work as before
- [ ] No layout shifts when switching to/from wavetable tab

---

### Task 8: CMakeLists.txt + Resource Provider
**Files:** `CMakeLists.txt`, `Source/PluginEditor.cpp` (resource provider section)
**Dependencies:** Task 1, 2, 5, 6
**Estimated complexity:** Low

Register new source files and binary resources.

**Steps:**
1. Add `Source/dsp/WavetableEditor.h` and `Source/dsp/WavetableEditor.cpp` to `target_sources()` in CMakeLists.txt
2. Add `Source/ui/public/js/wavetable-editor.js` and `Source/ui/public/css/wavetable-editor.css` to `juce_add_binary_data()` SOURCES
3. Add resource provider mappings in PluginEditor.cpp:
   - `/js/wavetable-editor.js` → `BinaryData::wavetable_editor_js` (application/javascript)
   - `/css/wavetable-editor.css` → `BinaryData::wavetable_editor_css` (text/css)

**Verification:**
- [ ] CMake configure succeeds
- [ ] Build succeeds with new files
- [ ] JS and CSS files are served correctly via resource provider

---

### Task 9: Build, Test & Polish
**Files:** Various
**Dependencies:** All previous tasks
**Estimated complexity:** Medium

End-to-end integration testing and bug fixes.

**Steps:**
1. Build: `ninja O-Prism_VST3 O-Prism_AU`
2. Fix any compilation errors
3. Install to system plugin folders (clear AU cache first)
4. Open in standalone mode — verify 5th tab appears
5. Test: select Osc A, verify frame strip loads with factory table waveforms
6. Test: click frame, verify harmonic bars display
7. Test: drag harmonic bars, verify waveform updates and audio changes
8. Test: multi-frame selection (shift+click, ctrl+click)
9. Test: each frame operation (normalize, fade, reverse, smooth)
10. Test: undo/redo (Ctrl+Z, Ctrl+Shift+Z)
11. Test: save edited wavetable — verify appears in user dropdown
12. Test: switch Osc A/B toggle
13. Test: switch away from wavetable tab and back
14. Fix visual/layout issues
15. Run pluginval if available

**Verification:**
- [ ] Plugin builds without warnings
- [ ] All editor features functional in standalone
- [ ] No audio glitches during live editing
- [ ] Saved wavetables persist across plugin restart
- [ ] No crashes on rapid tab switching or oscillator toggling
- [ ] Existing features (synth, effects, tuning, mod) unaffected

---

## Dependency Graph

```
Task 1 (Per-Frame Mipmap)
    │
    v
Task 2 (WavetableEditor C++)──────────┐
    │                                   │
    v                                   │
Task 3 (Processor Integration)          │
    │                                   │
    v                                   │
Task 4 (Native Functions)              │
    │                                   │
    │   Task 5 (CSS) ──────────────────┤
    │       │                           │
    v       v                           │
Task 6 (JavaScript UI)                 │
    │                                   │
    v                                   │
Task 7 (Tab Integration)               │
    │                                   │
    v                                   v
Task 8 (CMake + Resources) ◄───────────┘
    │
    v
Task 9 (Build, Test & Polish)
```

## Execution Order

| Order | Task | Can Parallelize With |
|-------|------|---------------------|
| 1 | Task 1: Per-Frame Mipmap | Task 5: CSS |
| 1 | Task 5: CSS | Task 1: Per-Frame Mipmap |
| 2 | Task 2: WavetableEditor C++ | — |
| 3 | Task 3: Processor Integration | — |
| 4 | Task 4: Native Functions | — |
| 5 | Task 6: JavaScript UI | — |
| 6 | Task 7: Tab Integration | — |
| 7 | Task 8: CMake + Resources | — |
| 8 | Task 9: Build, Test & Polish | — |

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Audio thread glitch during harmonic edit | Audible click/pop | WavetableEditor edits on message thread; oscillator's trilinear interpolation smooths partial updates. Per-frame mipmap regen is <0.1ms. |
| Large JSON payloads for frame data | UI lag during drag | Throttle setFrameHarmonics calls to rAF cadence (~16ms). Return only updated waveform, not all frames. |
| Canvas rendering perf with 256 frames | Slow frame strip scroll | Cache frame thumbnails as pre-rendered ImageData. Only re-render visible/changed frames. |
| Working copy memory (20MB) | Memory pressure | Single working copy at a time. Release on tab deactivation. |
| Undo stack lost on tab switch | User frustration | Document in UI ("undo history cleared on tab switch"). Acceptable for v1. |

## Version Impact

- **Version:** 1.9.0 → 1.10.0 (MINOR)
- **Breaking changes:** None
- **Parameter changes:** None (editor uses native functions, not APVTS)
- **Preset compatibility:** Full — editor state is transient

---

*Generated by improve-milestone plan phase*
