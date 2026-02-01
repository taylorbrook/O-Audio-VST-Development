# Stage 3: GUI - Execution Plan

**Plugin:** O-IntonationPad
**Stage:** 3 - GUI Implementation
**Created:** 2026-01-29
**Status:** Ready for execution

---

## Goal

Implement a WebView-based GUI with Ouaricon Naturalist aesthetic featuring 4 tabbed sections (Voice, Tuning, Modulation, Output), 15 parameter bindings with bidirectional sync, an interactive SVG pitch circle visualization, and an ocean shell botanical overlay.

---

## Tasks

### Phase 1: WebView Scaffold

#### Task 1: Update PluginEditor.h with WebView infrastructure
- **Files:** `Source/PluginEditor.h`
- **Depends on:** none
- **Actions:**
  - Add `#include <juce_gui_extra/juce_gui_extra.h>`
  - Add 15 `WebSliderRelay` unique_ptr members (Pattern #11: relays FIRST)
  - Add `WebBrowserComponent` unique_ptr (SECOND)
  - Add `hasNavigated` boolean member
  - Add 15 `WebSliderParameterAttachment` unique_ptr members (LAST)
  - Add `parentHierarchyChanged()` override
  - Add `getResource()` helper declaration

#### Task 2: Create UI directory structure
- **Files:** `Source/ui/public/` directory
- **Depends on:** none
- **Actions:**
  - Create `Source/ui/public/`
  - Create `Source/ui/public/js/juce/`
  - Create `Source/ui/public/modules/`
  - Create `Source/ui/public/img/`

#### Task 3: Copy JUCE JavaScript bridge files
- **Files:** `Source/ui/public/js/juce/index.js`, `Source/ui/public/js/juce/check_native_interop.js`
- **Depends on:** Task 2
- **Actions:**
  - Copy `index.js` from O-Tremolo (JUCE WebView bridge)
  - Copy `check_native_interop.js` from O-Tremolo

#### Task 4: Copy pitch-circle.js module
- **Files:** `Source/ui/public/modules/pitch-circle.js`
- **Depends on:** Task 2
- **Actions:**
  - Copy from `modules/tuning/scala-tuning-engine/js/pitch-circle.js`

#### Task 5: Copy background and botanical images
- **Files:** `Source/ui/public/img/paper.jpg`, `Source/ui/public/img/shell.png`
- **Depends on:** Task 2
- **Actions:**
  - Copy `paper.jpg` from O-Tremolo
  - Copy ocean shell image from `Ouaricon Audio Images/ocean/shell_conchologiaiconi12reev_0090.png` as `shell.png`

---

### Phase 2: HTML/CSS Tab Structure

#### Task 6: Create index.html with tab layout
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Task 3
- **Actions:**
  - Create HTML boilerplate with inline CSS (no external stylesheet to simplify BinaryData)
  - Add tab navigation (4 tabs: Voice, Tuning, Modulation, Output)
  - Add tab content containers for each section
  - Add botanical overlay positioned right side
  - Apply Ouaricon Naturalist styling:
    - Aged paper background (`paper.jpg`)
    - Garamond serif typography
    - Brown borders and warm earth tones
    - Wide letter-spacing on labels

#### Task 7: Implement CSS styling for Naturalist aesthetic
- **Files:** `Source/ui/public/index.html` (inline styles)
- **Depends on:** Task 6
- **Actions:**
  - Tab navigation styling (green active state, fleuron accents)
  - Seed cross-section knob styling (10-segment conic gradient)
  - Dropdown styling (brown borders, green focus)
  - Button row styling for tuningSystem
  - Botanical overlay positioning (right, 0.35 opacity, click-through)

---

### Phase 3: Parameter Controls (JavaScript)

#### Task 8: Create main.js with tab switching and knob logic
- **Files:** `Source/ui/public/index.html` (inline script)
- **Depends on:** Task 7
- **Actions:**
  - Tab switching logic (show/hide tab content)
  - Generic knob binding function using JUCE WebSlider API:
    - `getSliderState()` for parameter state
    - `valueChangedEvent.addListener()` for JUCE → UI sync
    - Mouse drag with `sliderDragStarted()`/`sliderDragEnded()` for UI → JUCE
  - Dropdown change handlers for choice parameters
  - Button row handler for tuningSystem

#### Task 9: Initialize all 15 parameter bindings in JavaScript
- **Files:** `Source/ui/public/index.html` (inline script)
- **Depends on:** Task 8
- **Actions:**
  - Bind 9 knob parameters (float/int)
  - Bind 2 dropdown parameters (keyRoot, keyScale)
  - Bind 1 button row parameter (tuningSystem)
  - Bind 3 additional knobs (attack, release, filter, volume)
  - Implement value display formatting per parameter type

---

### Phase 4: Pitch Circle Integration

#### Task 10: Integrate pitch-circle.js into Tuning tab
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Task 4, Task 9
- **Actions:**
  - Import PitchCircle module
  - Create 220px pitch circle in Tuning tab content
  - Connect to tuningSystem and keyRoot parameter changes
  - Define interval arrays for each tuning system:
    - 12-TET: `[0, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100]`
    - Just Intonation: `[0, 112, 204, 316, 386, 498, 590, 702, 814, 884, 1018, 1088]` (5-limit)
    - Pythagorean: `[0, 90, 204, 294, 408, 498, 588, 702, 792, 906, 996, 1110]`
    - Historical/Scala: Same as JI initially (placeholders)
  - Update pitch circle when tuningSystem changes
  - Display current key info beside circle

---

### Phase 5: C++ WebView Integration

#### Task 11: Implement PluginEditor.cpp with WebView pattern
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 1, Task 9
- **Actions:**
  - Constructor: Create 15 relays with parameter IDs matching HTML
  - Constructor: Create WebBrowserComponent with:
    - `.withNativeIntegrationEnabled()`
    - `.withResourceProvider()` lambda
    - 15x `.withOptionsFrom(*relay)` calls
  - Constructor: Create 15 attachments (Pattern #12: param, relay, nullptr)
  - Destructor: Empty (automatic cleanup)
  - `parentHierarchyChanged()`: Navigate on first showing
  - `resized()`: WebView fills bounds
  - `paint()`: Empty (WebView handles rendering)

#### Task 12: Implement getResource() URL mapping
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 11
- **Actions:**
  - Map "/" and "/index.html" → BinaryData::index_html
  - Map "/js/juce/index.js" → BinaryData::index_js
  - Map "/js/juce/check_native_interop.js" → BinaryData::check_native_interop_js
  - Map "/modules/pitch-circle.js" → BinaryData::pitchcircle_js
  - Map "/img/paper.jpg" → BinaryData::paper_jpg
  - Map "/img/shell.png" → BinaryData::shell_png
  - Log missing resources for debugging

---

### Phase 6: Build Configuration

#### Task 13: Update CMakeLists.txt with BinaryData resources
- **Files:** `CMakeLists.txt`
- **Depends on:** Task 5, Task 6
- **Actions:**
  - Add `juce_add_binary_data(OIntonationPad_UIResources ...)` with all UI files:
    - `Source/ui/public/index.html`
    - `Source/ui/public/js/juce/index.js`
    - `Source/ui/public/js/juce/check_native_interop.js`
    - `Source/ui/public/modules/pitch-circle.js`
    - `Source/ui/public/img/paper.jpg`
    - `Source/ui/public/img/shell.png`
  - Link `OIntonationPad_UIResources` to main target
  - Ensure `juce_gui_extra` module is included

#### Task 14: Build and verify WebView loads
- **Files:** Build output
- **Depends on:** Tasks 1-13
- **Actions:**
  - Run `cmake --build build --target OIntonationPad_VST3 OIntonationPad_AU`
  - Clear AU cache and install to system folders
  - Open in DAW and verify:
    - WebView renders without errors
    - Tabs switch correctly
    - Aged paper background appears
    - Shell botanical overlay visible

---

### Phase 7: Parameter Sync Verification

#### Task 15: Test bidirectional parameter sync
- **Files:** None (testing)
- **Depends on:** Task 14
- **Actions:**
  - Test UI → APVTS: Move knobs, verify DAW automation lane updates
  - Test APVTS → UI: Move DAW automation, verify knobs move
  - Test choice parameters: Select dropdown items, verify processor values
  - Test button row: Click tuningSystem buttons, verify selection state
  - Test pitch circle: Change tuningSystem, verify circle updates
  - Verify all 15 parameters sync correctly

---

## File Summary

### Files to Create

| File | Description |
|------|-------------|
| `Source/ui/public/index.html` | Main WebView HTML with inline CSS/JS |
| `Source/ui/public/js/juce/index.js` | JUCE WebView JavaScript bridge (copy) |
| `Source/ui/public/js/juce/check_native_interop.js` | Native interop checker (copy) |
| `Source/ui/public/modules/pitch-circle.js` | SVG pitch circle module (copy) |
| `Source/ui/public/img/paper.jpg` | Aged paper background (copy) |
| `Source/ui/public/img/shell.png` | Ocean shell botanical overlay (copy) |

### Files to Modify

| File | Changes |
|------|---------|
| `Source/PluginEditor.h` | Add WebView members, relays, attachments |
| `Source/PluginEditor.cpp` | Complete WebView implementation |
| `CMakeLists.txt` | Add BinaryData resources target |

---

## Parameter Binding Reference

| Parameter ID | Type | Control | Tab | JUCE Type |
|--------------|------|---------|-----|-----------|
| voiceCount | Int | Knob (2-12) | Voice | WebSliderRelay |
| complexity | Float | Knob (0-100%) | Voice | WebSliderRelay |
| keyRoot | Choice | Dropdown (12) | Voice | WebSliderRelay |
| keyScale | Choice | Dropdown (10) | Voice | WebSliderRelay |
| inversionRandom | Float | Knob (0-100%) | Voice | WebSliderRelay |
| tuningSystem | Choice | Button Row (5) | Tuning | WebSliderRelay |
| wavetablePos | Float | Knob (0-100%) | Modulation | WebSliderRelay |
| lfoRate | Float | Knob (0.01-20Hz) | Modulation | WebSliderRelay |
| lfoDepth | Float | Knob (0-100%) | Modulation | WebSliderRelay |
| timingRandom | Float | Knob (0-100ms) | Modulation | WebSliderRelay |
| detuneRandom | Float | Knob (0-50c) | Modulation | WebSliderRelay |
| attackTime | Float | Knob (1-5000ms) | Output | WebSliderRelay |
| releaseTime | Float | Knob (10-10000ms) | Output | WebSliderRelay |
| filterCutoff | Float | Knob (20-20kHz) | Output | WebSliderRelay |
| masterVolume | Float | Knob (0-+6dB) | Output | WebSliderRelay |

---

## Critical Patterns to Follow

### Pattern #11: Member Declaration Order
```cpp
// 1. RELAYS FIRST
std::unique_ptr<juce::WebSliderRelay> voiceCountRelay;
// ...

// 2. WEBVIEW SECOND
std::unique_ptr<juce::WebBrowserComponent> webView;

// 3. ATTACHMENTS LAST
std::unique_ptr<juce::WebSliderParameterAttachment> voiceCountAttachment;
```

### Pattern #12: Attachment Constructor (3 params)
```cpp
attachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.getAPVTS().getParameter("voiceCount"), *voiceCountRelay, nullptr);
```

### Pattern #15: Event Listener (no params)
```javascript
state.valueChangedEvent.addListener(() => updateVisual());
```

### Pattern #16: Relative Drag (delta)
```javascript
const deltaY = lastY - e.clientY;
const newNorm = Math.max(0, Math.min(1, currentNorm + (deltaY * 0.005)));
state.setNormalisedValue(newNorm);
```

---

## Success Criteria

- [ ] WebView loads without console errors
- [ ] 4 tabs switch correctly (Voice, Tuning, Modulation, Output)
- [ ] All 15 knobs/controls respond to interaction
- [ ] Bidirectional sync: UI ↔ DAW automation works
- [ ] Pitch circle displays current tuning intervals
- [ ] Pitch circle updates when tuningSystem changes
- [ ] Botanical shell overlay visible at correct opacity
- [ ] Ouaricon Naturalist aesthetic consistently applied
- [ ] No crashes on plugin load/close (Pattern #11 verified)
- [ ] Plugin window size is 800x500

---

## Deferred to Stage 4 (Polish)

- Active note highlighting on pitch circle (requires MIDI → WebView)
- Wavetable selector UI (placeholder for now)
- Scala file import dialog
- Preset manager integration
- Keyboard navigation for tabs
- Tooltips/help text

---

## Estimated Complexity

| Component | Effort |
|-----------|--------|
| WebView scaffold | Low |
| Tab structure | Low |
| 15 parameter bindings | Medium |
| Knob styling | Medium |
| Pitch circle integration | Low |
| CMake/BinaryData | Low |
| Testing/verification | Medium |
| **Total** | **Medium** |

---

## Next Step

Run `/plugin-execute O-IntonationPad 3-gui` to begin implementation.
