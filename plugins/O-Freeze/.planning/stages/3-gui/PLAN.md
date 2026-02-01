# Stage 3: GUI - Execution Plan

**Plugin:** O-Freeze
**Stage:** 3 of 4 (GUI)
**Created:** 2026-02-01

---

## Goal

Implement the WebView-based UI for O-Freeze with the Ouaricon Botanical aesthetic: aged paper texture background, anatomical illustration overlay, central organic freeze button with animation, and botanical-styled rotary knobs with vine indicators.

---

## Tasks

### Phase 1: Infrastructure Setup

#### Task 1: Create UI directory structure
- **Files:** `Source/ui/public/`, `Source/ui/public/js/juce/`
- **Depends on:** None
- **Action:** Create directory tree for WebView resources

#### Task 2: Copy JUCE JavaScript bridge files
- **Files:**
  - `Source/ui/public/js/juce/index.js`
  - `Source/ui/public/js/juce/check_native_interop.js`
- **Depends on:** Task 1
- **Action:** Copy from existing plugin (O-Detune) - these are standard JUCE 8 bridge files

#### Task 3: Update CMakeLists.txt for WebView
- **Files:** `CMakeLists.txt`
- **Depends on:** None
- **Action:**
  - Uncomment `juce_add_binary_data` section
  - Uncomment `JUCE_WEB_BROWSER=1` definition
  - Add image assets to binary data

---

### Phase 2: HTML/CSS/JS Implementation

#### Task 4: Create index.html with botanical layout
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Tasks 1, 2
- **Action:** Create main HTML file with:
  - 450×450 layout (square aspect)
  - Header bar with "Ouaricon Granular Freeze" branding
  - Central freeze button area
  - Surrounding knob positions for THRESHOLD, DRIFT, MIX
  - MODE toggle near freeze button
  - Paper texture background (Base64 embedded or binary resource)
  - Anatomical illustration overlay (reduced opacity)

#### Task 5: Implement botanical rotary knobs
- **Files:** `Source/ui/public/index.html` (inline or separate CSS/JS)
- **Depends on:** Task 4
- **Action:** Create custom knob component with:
  - Vine/tendril indicator arc that grows with value
  - Serif label typography
  - Value display
  - Disabled state (dim when MODE=Manual for THRESHOLD)

#### Task 6: Implement organic freeze button
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Task 4
- **Action:** Create freeze button with:
  - Organic/irregular shape (SVG or CSS)
  - Engaged animation (pulse/shimmer)
  - Color state indication
  - Disabled state (dim when MODE=Threshold)

#### Task 7: Implement mode toggle
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Task 4
- **Action:** Create Manual/Threshold toggle with:
  - Botanical styling
  - State indication
  - JS handlers to update disabled states of FREEZE/THRESHOLD

#### Task 8: Implement grain activity visualization
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Task 6
- **Action:** Add subtle particle/dot animation near freeze button when engaged

#### Task 9: Wire JUCE relay bindings in JavaScript
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Tasks 5, 6, 7
- **Action:** Connect UI controls to JUCE parameters via relay API:
  - `FREEZE` → freeze button
  - `THRESHOLD` → threshold knob
  - `MODE` → mode toggle
  - `DRIFT` → drift knob
  - `MIX` → mix knob

---

### Phase 3: C++ WebView Integration

#### Task 10: Update PluginEditor.h for WebView
- **Files:** `Source/PluginEditor.h`
- **Depends on:** None
- **Action:** Add member declarations in correct order:
  1. Relays first (WebSliderRelay for THRESHOLD/DRIFT/MIX, WebToggleButtonRelay for FREEZE/MODE)
  2. WebBrowserComponent second
  3. hasNavigated flag
  4. Attachments last

#### Task 11: Update PluginEditor.cpp constructor
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 10
- **Action:**
  - Create relays with matching parameter IDs
  - Create WebView with all relays registered via withOptionsFrom
  - Create attachments connecting relays to APVTS parameters
  - Set size to 450×450

#### Task 12: Implement getResource() method
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 11
- **Action:** Map URLs to binary resources:
  - `/` or `/index.html` → index.html
  - `/js/juce/index.js` → JUCE bridge
  - `/js/juce/check_native_interop.js` → interop checker
  - Image assets if needed

#### Task 13: Implement parentHierarchyChanged()
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 11
- **Action:** Navigate to resource provider root when window is attached (JUCE 8 pattern)

#### Task 14: Update paint() and resized()
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 11
- **Action:**
  - paint(): empty (WebView handles rendering)
  - resized(): WebView fills bounds

---

### Phase 4: Asset Integration

#### Task 15: Copy and encode image assets
- **Files:** `Source/ui/assets/`
- **Depends on:** None
- **Action:**
  - Copy paper1.jpg and muscles illustration
  - Consider Base64 encoding for HTML embedding
  - Or add to binary data resources

#### Task 16: Update CMakeLists.txt with asset references
- **Files:** `CMakeLists.txt`
- **Depends on:** Task 15
- **Action:** Add image files to juce_add_binary_data if using binary resources

---

### Phase 5: Build & Test

#### Task 17: Build and verify compilation
- **Files:** None (build step)
- **Depends on:** All previous tasks
- **Action:** `ninja O-Freeze_VST3 O-Freeze_AU`

#### Task 18: Install and test in DAW
- **Files:** None (test step)
- **Depends on:** Task 17
- **Action:**
  - Clear AU cache
  - Install plugins
  - Test in Logic Pro / Ableton
  - Verify parameter bindings work
  - Verify disabled states respond to MODE changes

---

## Dependencies Graph

```
Task 1 → Task 2 → Task 4 → Tasks 5,6,7,8 → Task 9
                    ↓
Task 3 ─────────────┴──────────────────────→ Task 16
                                                ↓
Tasks 10 → Task 11 → Tasks 12,13,14 ──────────→ Task 17 → Task 18
      ↓
Task 15 → Task 16
```

---

## Success Criteria

- [ ] WebView loads and displays without errors
- [ ] Paper texture background visible
- [ ] Anatomical illustration visible as faded overlay
- [ ] Header shows "Ouaricon Granular Freeze"
- [ ] FREEZE button is organic-shaped, large, central
- [ ] FREEZE button animates when engaged
- [ ] THRESHOLD, DRIFT, MIX knobs have botanical vine indicators
- [ ] MODE toggle switches between Manual/Threshold
- [ ] THRESHOLD knob dims when MODE = Manual
- [ ] FREEZE button dims when MODE = Threshold
- [ ] All parameters sync bidirectionally (DAW ↔ UI)
- [ ] Grain activity visualization appears when frozen
- [ ] Typography uses serif fonts
- [ ] Build succeeds (VST3 + AU)
- [ ] pluginval passes (strictness 5)
- [ ] Tested in DAW without crashes

---

## Parameter → Relay Mapping

| Parameter ID | APVTS Type | Relay Type | Relay ID |
|-------------|------------|------------|----------|
| FREEZE | AudioParameterBool | WebToggleButtonRelay | "FREEZE" |
| THRESHOLD | AudioParameterFloat | WebSliderRelay | "THRESHOLD" |
| MODE | AudioParameterChoice | WebToggleButtonRelay | "MODE" |
| DRIFT | AudioParameterFloat | WebSliderRelay | "DRIFT" |
| MIX | AudioParameterFloat | WebSliderRelay | "MIX" |

---

## Files to Create/Modify

### New Files
- `Source/ui/public/index.html` - Main WebView HTML
- `Source/ui/public/js/juce/index.js` - JUCE bridge (copy)
- `Source/ui/public/js/juce/check_native_interop.js` - Interop checker (copy)
- `Source/ui/assets/` - Image assets (optional if Base64)

### Modified Files
- `Source/PluginEditor.h` - Add WebView members
- `Source/PluginEditor.cpp` - Implement WebView integration
- `CMakeLists.txt` - Enable WebView resources

---

## Notes

- **Member order is critical** - relays → webView → attachments (Pattern #11)
- **Navigation timing** - use parentHierarchyChanged() to avoid scanner crashes
- **JUCE 8** - use WebSliderRelay/WebToggleButtonRelay, not older APIs
- **Assets** - paper1.jpg at `/Users/taylorbrook/Dev/Ouaricon Audio Images/paper/paper1.jpg`
- **Anatomical** - muscles at `/Users/taylorbrook/Dev/Ouaricon Audio Images/anatomy/muscles_histoirephysiqu911875gran_0161.png`
