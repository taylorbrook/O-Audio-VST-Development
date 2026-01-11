# Stage 3 (GUI) Integration Checklist - v1

**Plugin:** OuariconMarimba (Ouaricon Marimba)
**Mockup Version:** v1
**Generated:** 2026-01-09
**Window Size:** 600 x 400 pixels

## Overview

This checklist guides the gui-agent through integrating the finalized v1 WebView UI mockup into the OuariconMarimba plugin during Stage 3 (GUI) implementation.

## Parameter Summary

**Total Parameters:** 7

| Parameter ID | Type | Relay Type | Attachment Type |
|--------------|------|------------|-----------------|
| MALLET_HARDNESS | Float | WebSliderRelay | WebSliderParameterAttachment |
| BAR_MATERIAL | Float | WebSliderRelay | WebSliderParameterAttachment |
| RESONANCE | Float | WebSliderRelay | WebSliderParameterAttachment |
| TUNING_MODE | Choice (0-2) | WebComboBoxRelay | WebComboBoxParameterAttachment |
| REFERENCE_PITCH | Float | WebSliderRelay | WebSliderParameterAttachment |
| VEL_CURVE | Float | WebSliderRelay | WebSliderParameterAttachment |
| OUTPUT_GAIN | Float | WebSliderRelay | WebSliderParameterAttachment |

## 1. Copy UI Files

- [ ] Create directory `Source/ui/public/` in OuariconMarimba project
- [ ] Create directory `Source/ui/public/js/juce/` for JUCE frontend library
- [ ] Copy `v1-ui.html` to `Source/ui/public/index.html`
- [ ] Copy JUCE frontend library to `Source/ui/public/js/juce/index.js`
  - Source: `/path/to/JUCE/modules/juce_gui_basics/native/javaScriptObjects/index.js`
- [ ] Verify no external image dependencies (mockup uses CSS-only graphics)

## 2. Update PluginEditor Files

### 2.1 Replace PluginEditor.h

- [ ] Copy content from `v1-PluginEditor.h`
- [ ] Update class name to `OuariconMarimbaAudioProcessorEditor`
- [ ] Update processor reference to `OuariconMarimbaAudioProcessor`
- [ ] Verify member order is correct:
  - [ ] 7 relay declarations (all before webView)
  - [ ] 1 webView declaration (after relays, before attachments)
  - [ ] 7 attachment declarations (all after webView)
- [ ] Verify relay types match parameter types:
  - [ ] 6 × WebSliderRelay (float parameters)
  - [ ] 1 × WebComboBoxRelay (choice parameter: TUNING_MODE)

### 2.2 Replace PluginEditor.cpp

- [ ] Copy content from `v1-PluginEditor.cpp`
- [ ] Update class name to `OuariconMarimbaAudioProcessorEditor`
- [ ] Update processor reference to `OuariconMarimbaAudioProcessor`
- [ ] Verify parameter ID strings match APVTS exactly:
  - [ ] "MALLET_HARDNESS"
  - [ ] "BAR_MATERIAL"
  - [ ] "RESONANCE"
  - [ ] "TUNING_MODE"
  - [ ] "REFERENCE_PITCH"
  - [ ] "VEL_CURVE"
  - [ ] "OUTPUT_GAIN"
- [ ] Verify initialization order matches declaration order
- [ ] Verify all 7 relays registered with `.withOptionsFrom()`
- [ ] Verify window size: `setSize(600, 400)`

## 3. Update CMakeLists.txt

- [ ] Open `plugins/OuariconMarimba/CMakeLists.txt`
- [ ] Locate existing `juce_add_plugin()` declaration
- [ ] Append content from `v1-CMakeLists.txt` AFTER plugin declaration
- [ ] Verify `juce_add_binary_data()` includes:
  - [ ] `Source/ui/public/index.html`
  - [ ] `Source/ui/public/js/juce/index.js`
- [ ] Verify `target_link_libraries()` includes:
  - [ ] `OuariconMarimba_UIResources`
  - [ ] `juce::juce_gui_extra`
- [ ] Verify `target_compile_definitions()` includes:
  - [ ] `JUCE_WEB_BROWSER=1`
  - [ ] `JUCE_USE_CURL=0`

## 4. Build and Test (Debug)

- [ ] Run CMake configuration: `cmake -B build -DCMAKE_BUILD_TYPE=Debug`
- [ ] Build succeeds without warnings
- [ ] Launch standalone plugin
- [ ] WebView loads (not blank screen)
- [ ] Right-click → Inspect works (developer tools open)
- [ ] Console shows no JavaScript errors
- [ ] Console shows: `window.__JUCE__` object exists
- [ ] Console shows: "Ouaricon Marimba UI initialized"

## 5. Test Parameter Binding (Debug)

Test each of the 7 parameters:

### 5.1 MALLET_HARDNESS
- [ ] Knob drag updates value in UI
- [ ] DAW automation updates knob position
- [ ] Preset recall updates knob position
- [ ] Value persists after plugin reload

### 5.2 BAR_MATERIAL
- [ ] Knob drag updates value in UI
- [ ] DAW automation updates knob position
- [ ] Preset recall updates knob position
- [ ] Value persists after plugin reload

### 5.3 RESONANCE
- [ ] Knob drag updates value in UI
- [ ] DAW automation updates knob position
- [ ] Preset recall updates knob position
- [ ] Value persists after plugin reload

### 5.4 TUNING_MODE
- [ ] Button clicks update active state
- [ ] Scale name display updates correctly:
  - [ ] 12-TET → "12-TET Standard"
  - [ ] SCALA → "No scale loaded"
  - [ ] MTS-ESP → "Waiting for MTS-ESP..."
- [ ] DAW automation updates button state
- [ ] Preset recall updates button state
- [ ] Value persists after plugin reload

### 5.5 REFERENCE_PITCH
- [ ] Knob drag updates Hz value (400.0 - 480.0 Hz)
- [ ] DAW automation updates knob position
- [ ] Preset recall updates knob position
- [ ] Value persists after plugin reload

### 5.6 VEL_CURVE
- [ ] Knob drag updates curve display
- [ ] Curve visualization updates smoothly
- [ ] DAW automation updates knob position
- [ ] Preset recall updates knob and curve
- [ ] Value persists after plugin reload

### 5.7 OUTPUT_GAIN
- [ ] Knob drag updates dB value (-24.0 to 12.0 dB)
- [ ] DAW automation updates knob position
- [ ] Preset recall updates knob position
- [ ] Value persists after plugin reload

## 6. Build and Test (Release)

- [ ] Run CMake configuration: `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- [ ] Release build succeeds without warnings
- [ ] Launch standalone plugin
- [ ] WebView loads (not blank screen)
- [ ] All 7 parameters sync correctly
- [ ] No crashes on plugin reload (test 10 times minimum)
  - This tests member order correctness
- [ ] No crashes when closing DAW with plugin open
- [ ] Performance is smooth (no UI lag)

## 7. WebView-Specific Validation

- [ ] No viewport units in CSS (`100vh`, `100vw`, `100dvh`)
  - Verified: Uses `height: 100%` on html, body
- [ ] Native feel CSS present:
  - [ ] `user-select: none`
  - [ ] `-webkit-touch-callout: none`
  - [ ] `cursor: default`
- [ ] Resource provider returns all files (check console for 404s)
- [ ] Correct MIME types:
  - [ ] `index.html` → `text/html`
  - [ ] `juce/index.js` → `application/javascript`
- [ ] Context menu disabled (right-click shows developer tools, not browser menu)

## 8. Tab Functionality

- [ ] "SOUND" tab shows by default
- [ ] Clicking "TUNING" tab switches content
- [ ] Clicking "SOUND" tab switches back
- [ ] Tab state does NOT persist (always starts on SOUND)
- [ ] No console errors during tab switching

## 9. Visual Verification

- [ ] Ouaricon Naturalist aesthetic present:
  - [ ] Aged paper background color (#F5E6D3)
  - [ ] Botanical seed-cross-section knobs
  - [ ] Brown/green color palette
  - [ ] Garamond serif typography
- [ ] All knobs render correctly (conic gradient pattern)
- [ ] All labels visible and readable
- [ ] Window size is exactly 600 x 400 pixels
- [ ] Border frame visible (3px solid #5C4033)

## 10. Documentation

- [ ] Update plugin README with WebView requirements
- [ ] Note JUCE 8+ requirement
- [ ] Note platform-specific WebView dependencies:
  - macOS: Built-in WebKit
  - Windows: WebView2 runtime required
  - Linux: webkit2gtk-4.0 required

## Known Limitations

1. **No waveform animation:** Waveform display is static (no real-time audio visualization in v1)
2. **No VU meter animation:** VU meter is static (no real-time level metering in v1)
3. **No keyboard visualization:** Tuning tab shows controls only (no keyboard display in v1)
4. **No Scala/KBM file loading:** File load buttons are visual only (functionality to be added later)
5. **No MTS-ESP connection:** MTS indicator is visual only (functionality to be added later)

## Success Criteria

Integration is complete when:

- [ ] All 7 parameters sync bidirectionally (UI ↔ APVTS)
- [ ] No crashes in debug or release builds
- [ ] No console errors in browser developer tools
- [ ] Preset recall works correctly
- [ ] DAW automation works correctly
- [ ] Both tabs functional
- [ ] Window size correct (600 x 400)
- [ ] Visual design matches mockup

## Next Steps

After successful integration:

1. Continue to Stage 4 (DSP) to implement physical modeling engine
2. Add real-time waveform visualization (optional enhancement)
3. Add real-time VU metering (optional enhancement)
4. Implement Scala/KBM file loading (TUNING_MODE = 1)
5. Implement MTS-ESP integration (TUNING_MODE = 2)
6. Add keyboard visualization to TUNING tab (optional enhancement)

---

**Reference Files:**
- Mockup: `v1-ui.html`
- Header: `v1-PluginEditor.h`
- Implementation: `v1-PluginEditor.cpp`
- CMake: `v1-CMakeLists.txt`
- Parameter Spec: `../parameter-spec.md`
