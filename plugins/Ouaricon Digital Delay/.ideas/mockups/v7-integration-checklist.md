# Stage 3 (GUI) Integration Checklist - v7

**Plugin:** Ouaricon Digital Delay
**Mockup Version:** v7
**Generated:** 2026-01-12
**Window Size:** 700×196px (ultra-compact rack mount)

## Overview

This checklist guides gui-agent through integrating the v7 WebView UI mockup into the plugin during Stage 3 (GUI) implementation.

**Total Parameters:** 8
- 6 sliders (time, feedback, spread, mod, wet, dry)
- 1 toggle (sync)
- 1 combo (division)

**Visual Elements:**
- 14-segment LED output meter (requires C++ → JS messaging)

## 1. Copy UI Files

- [ ] Create directory: `Source/ui/public/`
- [ ] Create directory: `Source/ui/public/js/juce/`
- [ ] Create directory: `Source/ui/public/img/`
- [ ] Copy `v7-ui.html` to `Source/ui/public/index.html`
- [ ] Copy JUCE frontend library to `Source/ui/public/js/juce/index.js`
  - Source: `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/index.js`
- [ ] Copy `check_native_interop.js` to `Source/ui/public/js/juce/check_native_interop.js`
  - Source: `/Users/taylorbrook/JUCE/modules/juce_gui_extra/native/javascript/check_native_interop.js`
- [ ] Copy image assets:
  - `mockups/img/paper1.jpg` → `Source/ui/public/img/paper1.jpg`
  - `mockups/img/butterfly2_Black and white.png` → `Source/ui/public/img/butterfly2_Black and white.png`

## 2. Update PluginEditor Files

### PluginEditor.h
- [ ] Replace with `v7-PluginEditor.h` content
- [ ] Update class name: `OuariconDigitalDelayAudioProcessorEditor` → actual plugin class name
- [ ] Update processor reference type
- [ ] Verify member order: relays → webView → attachments
- [ ] Verify all 8 relay declarations present (6 slider, 1 toggle, 1 combo)
- [ ] Verify all 8 attachment declarations present

### PluginEditor.cpp
- [ ] Replace with `v7-PluginEditor.cpp` content
- [ ] Update class name and constructor signature
- [ ] Update processor type in constructor
- [ ] Verify initialization order matches declaration order
- [ ] Verify all 8 relays in initializer list
- [ ] Verify all 8 attachments in initializer list
- [ ] Verify all `.withOptionsFrom()` calls present (8 total)
- [ ] Update `getResource()` with correct BinaryData names
  - Check CMake output for exact BinaryData symbol names
  - Spaces in filenames become underscores: `butterfly2_Black and white.png` → `butterfly2_Black_and_white_png`

## 3. Update CMakeLists.txt

- [ ] Append `v7-CMakeLists.txt` snippet to plugin's CMakeLists.txt
- [ ] Verify `juce_add_binary_data` includes all UI files (5 total)
- [ ] Verify `target_link_libraries` includes `juce::juce_gui_extra`
- [ ] Verify `JUCE_WEB_BROWSER=1` definition present
- [ ] Add `NEEDS_WEB_BROWSER TRUE` to `juce_add_plugin()` call
- [ ] Verify product name matches plugin name

## 4. Build and Test (Debug)

- [ ] Clean build directory: `rm -rf build/`
- [ ] Configure: `cmake -B build -DCMAKE_BUILD_TYPE=Debug`
- [ ] Build: `cmake --build build`
- [ ] Build succeeds without warnings
- [ ] Install to system: `./scripts/build-and-install.sh "Ouaricon Digital Delay"`
- [ ] Launch standalone
- [ ] WebView loads (not blank white screen)
- [ ] Right-click → Inspect works (developer tools)
- [ ] Console shows no JavaScript errors
- [ ] `window.__JUCE__` object exists in console
- [ ] All 8 controls visible and styled correctly

## 5. Build and Test (Release)

- [ ] Build release: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
- [ ] Install: `./scripts/build-and-install.sh "Ouaricon Digital Delay"`
- [ ] Release build succeeds
- [ ] Standalone launches without crashes
- [ ] Reload plugin 10 times (tests member order correctness)
- [ ] No crashes during reload
- [ ] Clear caches and restart DAW
- [ ] Plugin loads in DAW

## 6. Test Parameter Binding

Test each parameter for bidirectional sync:

### TIME (slider, 1.0-2000.0ms, default 500.0)
- [ ] Drag knob in UI → value updates
- [ ] Value persists after plugin reload
- [ ] DAW automation → UI knob updates
- [ ] Preset recall → UI reflects new value

### FEEDBACK (slider, 0.0-100.0%, default 30.0)
- [ ] Drag knob in UI → value updates
- [ ] Value persists after plugin reload
- [ ] DAW automation → UI knob updates
- [ ] Preset recall → UI reflects new value

### SPREAD (slider, 0.0-100.0%, default 0.0)
- [ ] Drag knob in UI → value updates
- [ ] Value persists after plugin reload
- [ ] DAW automation → UI knob updates
- [ ] Preset recall → UI reflects new value

### MOD (slider, 0.0-100.0%, default 0.0)
- [ ] Drag knob in UI → value updates
- [ ] Value persists after plugin reload
- [ ] DAW automation → UI knob updates
- [ ] Preset recall → UI reflects new value

### WET (slider, 0.0-100.0%, default 30.0)
- [ ] Drag knob in UI → value updates
- [ ] Value persists after plugin reload
- [ ] DAW automation → UI knob updates
- [ ] Preset recall → UI reflects new value

### DRY (slider, 0.0-100.0%, default 100.0)
- [ ] Drag knob in UI → value updates
- [ ] Value persists after plugin reload
- [ ] DAW automation → UI knob updates
- [ ] Preset recall → UI reflects new value

### SYNC (toggle, on/off, default false)
- [ ] Click toggle in UI → state changes
- [ ] Visual feedback (OFF/ON text, color change)
- [ ] DAW automation → UI toggle updates
- [ ] Preset recall → UI reflects new value

### DIVISION (combo, 12 choices, default 1 = "1/8")
- [ ] Dropdown shows all 12 options
- [ ] Select option → value updates
- [ ] DAW automation → UI dropdown updates
- [ ] Preset recall → UI reflects new value

## 7. WebView-Specific Validation

- [ ] No viewport units in CSS (`100vh`, `100vw`, `100dvh`)
  - Check: `grep -r "100vh\|100vw\|100dvh" Source/ui/public/`
  - Should use: `html, body { height: 100%; }`
- [ ] Native feel CSS present
  - `user-select: none` on body
  - `cursor: default` on body
  - Context menu disabled in JavaScript
- [ ] Resource provider returns all files
  - Check browser console for 404 errors
  - Verify all 5 resources load (HTML, 2 JS, 2 images)
- [ ] Correct MIME types
  - HTML: `text/html`
  - JS: `application/javascript` (NOT `text/javascript`)
  - JPG: `image/jpeg`
  - PNG: `image/png`
- [ ] ES6 module loading works
  - `<script type="module" src="js/juce/index.js"></script>`
  - `<script type="module">` for inline code
  - `import { getSliderState, ... } from './js/juce/index.js'`

## 8. LED Meter Integration (Future Work)

The v7 mockup includes a 14-segment LED output meter. This requires C++ → JS messaging:

### C++ Implementation (in PluginProcessor)
```cpp
// In processBlock(), calculate RMS and send to UI
float rmsLevel = calculateRMS(buffer);  // 0.0-1.0 normalized

if (auto* editor = dynamic_cast<OuariconDigitalDelayAudioProcessorEditor*>(getActiveEditor()))
{
    editor->updateMeter(rmsLevel);
}
```

### JavaScript Handler (in index.html)
```javascript
// Already implemented in v7-ui.html:
// updateLEDMeter(rmsLevel) function
// Expects rmsLevel: 0.0-1.0 normalized
```

**Implementation steps:**
- [ ] Add RMS calculation to processBlock()
- [ ] Implement updateMeter() in PluginEditor
- [ ] Send JS message via WebView evaluateJavascript()
- [ ] Test meter responds to audio levels
- [ ] Verify ballistic motion (fast attack, slow decay)

## Parameter Summary (from parameter-spec.md)

| Parameter | Type   | Range       | Default | UI Control |
|-----------|--------|-------------|---------|------------|
| time      | Float  | 1.0-2000.0  | 500.0   | Knob       |
| feedback  | Float  | 0.0-100.0   | 30.0    | Knob       |
| spread    | Float  | 0.0-100.0   | 0.0     | Knob       |
| mod       | Float  | 0.0-100.0   | 0.0     | Knob       |
| wet       | Float  | 0.0-100.0   | 30.0    | Knob       |
| dry       | Float  | 0.0-100.0   | 100.0   | Knob       |
| sync      | Bool   | On/Off      | Off     | Toggle     |
| division  | Choice | 12 options  | 1 (1/8) | Dropdown   |

**Total:** 8 parameters
- 6 WebSliderRelay + WebSliderParameterAttachment
- 1 WebToggleButtonRelay + WebToggleButtonParameterAttachment
- 1 WebComboBoxRelay + WebComboBoxParameterAttachment

## Common Issues and Solutions

### Knobs don't respond to drag
- **Cause:** Missing `nullptr` in WebSliderParameterAttachment constructor (JUCE 8 requires 3 parameters)
- **Fix:** Verify all attachments have `nullptr` as third parameter

### WebView blank (white screen)
- **Cause:** Resource provider not serving files correctly
- **Fix:** Check BinaryData symbol names match getResource() mapping

### Plugin doesn't appear in DAW (VST3 only)
- **Cause:** Missing `NEEDS_WEB_BROWSER TRUE` in juce_add_plugin()
- **Fix:** Add flag to CMakeLists.txt and rebuild

### Images don't load (404 errors)
- **Cause:** BinaryData symbol name mismatch (spaces → underscores)
- **Fix:** Check build output for exact symbol names, update getResource()

### Preset recall doesn't update UI
- **Cause:** Missing `valueChangedEvent.addListener()` in JavaScript
- **Fix:** Verify all parameters have event listeners

## References

- JUCE 8 Critical Patterns: `troubleshooting/patterns/juce8-critical-patterns.md`
- UI Design Rules: `ui-mockup/references/ui-design-rules.md`
- WebView Integration: Stage 3 gui-agent documentation
