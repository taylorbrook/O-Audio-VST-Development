# Stage 3: GUI - Execution Plan

**Plugin:** O-Chorus
**Date:** 2026-02-08
**Goal:** Replace placeholder WebView UI with full Naturalist-styled GUI featuring 7 parameter knobs, LFO ring animation, and paper texture background.

---

## Pre-Conditions

- Stage 2 (DSP) verified complete
- 7 WebSliderRelays + 7 WebSliderParameterAttachments already wired in PluginEditor.cpp
- Resource provider already serves BinaryData assets
- Cross-platform WebView2 config already in CMakeLists.txt

---

## Tasks

### 1. [ ] Copy paper texture asset from O-DigiDelay
- **Files:** `Source/ui/public/img/paper1.jpg` (new, copied)
- **Source:** `plugins/O-DigiDelay/Source/ui/public/img/paper1.jpg`
- **Depends on:** none

### 2. [ ] Add paper1.jpg to CMakeLists.txt binary data
- **Files:** `CMakeLists.txt` (edit)
- **Change:** Add `Source/ui/public/img/paper1.jpg` to `juce_add_binary_data` SOURCES
- **Depends on:** Task 1

### 3. [ ] Add paper1.jpg resource route in PluginEditor.cpp
- **Files:** `Source/PluginEditor.cpp` (edit)
- **Changes:**
  - Add URL mapping: `/img/paper1.jpg` -> `BinaryData::paper1_jpg` with MIME `image/jpeg`
  - Change `setSize(600, 400)` to `setSize(700, 250)`
- **Depends on:** Task 2

### 4. [ ] Replace index.html with full Naturalist GUI
- **Files:** `Source/ui/public/index.html` (replace)
- **Content (~350 lines):**
  - **HTML Structure:**
    - Paper background layer (`img/paper1.jpg`)
    - Header bar: "O-CHORUS" title left, preset placeholder right
    - Controls row: 7 knobs + central LFO ring
    - Layout: 700x250 horizontal strip
  - **CSS:**
    - Naturalist palette: cream `#F5E6D3`, wheat `#F5DEB3`, tan `#E8D5B7`, brown `#8B7355`, dark `#3C2F2F`
    - 10-segment conic-gradient knobs with brown border
    - Knob indicator line (dark brown `#3C2F2F`)
    - 270-degree rotation arc (-135 to +135)
    - Uppercase Garamond labels, value displays below
    - LFO ring: SVG circle with sage green `#5a7a6a` stroke
    - Viewport meta: `width=700, height=250`
    - `user-select: none`, `overflow: hidden`, no vh/vw units
  - **JavaScript (inline module):**
    - Import from `./js/juce/index.js`
    - `getSliderState()` for all 7 params: rate, depth, voices, width, tone, mix, drive
    - Vertical drag interaction (sensitivity 0.005, shift for fine 0.001)
    - Double-click to reset to defaults
    - Mouse wheel support
    - Global drag state (single mousemove/mouseup handler)
    - Voices knob: snap to 8 integer steps visually
    - LFO ring animation: `requestAnimationFrame` loop, dot orbiting circle
      - Speed from rate parameter, pulse from depth parameter
      - Color: sage green `#5a7a6a`
    - Value formatters:
      - Rate: `X.XX Hz` (from `getScaledValue()`)
      - Depth/Width/Mix/Drive: `XX%` (from `getScaledValue()`)
      - Voices: integer (from `getScaledValue()`)
      - Tone: `+XX%` / `-XX%` (from `getScaledValue()`, bipolar)
    - Gesture support: `sliderDragStarted()` / `sliderDragEnded()` for DAW undo
    - Context menu disabled (`contextmenu` preventDefault)
  - **Parameter grouping layout:**
    - Left group (Modulation): RATE, DEPTH, VOICES
    - Center: LFO ring animation
    - Right group (Character): WIDTH, TONE, MIX, DRIVE
- **Depends on:** Tasks 1-3

### 5. [ ] Build and verify
- **Commands:** `ninja OuariconChorus_VST3 OuariconChorus_AU` in build dir
- **Verify:**
  - Zero build errors
  - Zero warnings
  - All 7 knobs render and respond to drag
  - LFO ring animates based on rate/depth
  - Voices knob snaps to integer steps
  - Value displays show correct formatted strings
  - Paper texture visible as background
  - Window size 700x250
- **Depends on:** Task 4

---

## Success Criteria

- [ ] Plugin window opens at 700x250 with Naturalist styling
- [ ] Paper texture background visible
- [ ] All 7 knobs render with segment conic-gradient appearance
- [ ] Vertical drag changes parameter values (JUCE state updates)
- [ ] Double-click resets knobs to defaults
- [ ] Voices knob snaps to integer positions (1-8)
- [ ] LFO ring animates: dot orbits at Rate speed, pulses with Depth
- [ ] Value displays show formatted strings (Hz, %, integer)
- [ ] Tone display shows +/- sign for bipolar range
- [ ] DAW undo/redo works (gesture begin/end signals)
- [ ] Builds with zero errors on macOS (VST3 + AU)

---

## Files Summary

| File | Action | Lines Changed |
|------|--------|---------------|
| `Source/ui/public/img/paper1.jpg` | Copy from O-DigiDelay | binary |
| `CMakeLists.txt` | Add 1 line to binary data | +1 |
| `Source/PluginEditor.cpp` | Add resource route + resize | +5 |
| `Source/ui/public/index.html` | Full replacement | ~350 |

**Total:** 4 files, ~1 asset copy

---

## Reference Patterns

- **Knob interaction:** O-DigiDelay, O-GrainScatter (vertical drag, 0.005 sensitivity)
- **Naturalist styling:** O-DigiDelay (paper texture, conic gradient knobs, Garamond)
- **LFO animation:** O-Comp vine-arc pattern adapted to orbiting dot
- **JUCE WebView API:** `getSliderState()`, `setNormalisedValue()`, `getScaledValue()`

---

*Plan ready for execution.*
