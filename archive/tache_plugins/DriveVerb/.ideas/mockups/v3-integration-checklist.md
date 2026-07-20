# DriveVerb v3 - WebView Integration Checklist

**Generated:** 2025-11-11
**Mockup Version:** v3
**Window Size:** 1000x500px (fixed, non-resizable)
**Parameters:** 6 total (5 sliders + 1 toggle)

---

## Overview

This checklist guides Stage 5 (GUI) integration of the DriveVerb WebView UI mockup. Follow steps sequentially to ensure correct implementation.

**Key files:**
- `v3-ui.html` → Copy to `Source/ui/public/index.html`
- `v3-PluginEditor.h` → Reference for member declarations
- `v3-PluginEditor.cpp` → Reference for initialization sequence
- `v3-CMakeLists.txt` → CMake configuration snippets

---

## Stage 5 (GUI) Integration Steps

### 1. Copy UI Files

- [ ] Create directory: `Source/ui/public/`
- [ ] Create directory: `Source/ui/public/js/juce/`
- [ ] Copy `v3-ui.html` to `Source/ui/public/index.html`
- [ ] Download JUCE frontend library to `Source/ui/public/js/juce/index.js`
  - Source: https://github.com/juce-framework/JUCE/blob/master/modules/juce_gui_extra/embedding/js/juce_frontend.js
  - Or copy from existing JUCE 8 project

### 2. Update PluginEditor Files

#### PluginEditor.h

- [ ] Replace with `v3-PluginEditor.h` content
- [ ] Verify class name matches: `DriveVerbAudioProcessorEditor`
- [ ] **CRITICAL:** Verify member declaration order:
  ```cpp
  // 1️⃣ Relays FIRST
  std::unique_ptr<juce::WebSliderRelay> sizeRelay;
  std::unique_ptr<juce::WebSliderRelay> decayRelay;
  std::unique_ptr<juce::WebSliderRelay> dryWetRelay;
  std::unique_ptr<juce::WebSliderRelay> driveRelay;
  std::unique_ptr<juce::WebSliderRelay> filterRelay;
  std::unique_ptr<juce::WebToggleButtonRelay> filterPositionRelay;

  // 2️⃣ WebView SECOND
  std::unique_ptr<juce::WebBrowserComponent> webView;

  // 3️⃣ Attachments LAST
  std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
  std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
  std::unique_ptr<juce::WebSliderParameterAttachment> dryWetAttachment;
  std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
  std::unique_ptr<juce::WebSliderParameterAttachment> filterAttachment;
  std::unique_ptr<juce::WebToggleButtonParameterAttachment> filterPositionAttachment;
  ```
- [ ] Verify `timerCallback()` override is present (for VU meter updates)

#### PluginEditor.cpp

- [ ] Replace with `v3-PluginEditor.cpp` content
- [ ] Verify initialization order matches member declaration order
- [ ] Verify window size: `setSize(1000, 500)`
- [ ] Verify timer setup: `startTimerHz(16)` for VU meter updates
- [ ] Add `getCurrentDriveLevel()` method to `DriveVerbAudioProcessor` (returns current drive saturation level in dB for VU meter)

### 3. Update CMakeLists.txt

- [ ] Add `juce_add_binary_data` for UI resources (see `v3-CMakeLists.txt`)
- [ ] Link `DriveVerb_UIResources` to plugin target
- [ ] Add `JUCE_WEB_BROWSER=1` definition to `target_compile_definitions`
- [ ] Verify `juce::juce_gui_extra` is linked (provides WebBrowserComponent)

### 4. Create Parameter Specification

- [ ] Create `plugins/DriveVerb/.ideas/parameter-spec.md` with all 6 parameters
- [ ] Verify parameter IDs match exactly:
  - `size` (0-100%, default 40%)
  - `decay` (0.5-10s, default 2s)
  - `dryWet` (0-100%, default 30%)
  - `drive` (0-24dB, default 6dB)
  - `filter` (-100% to +100%, default 0%)
  - `filterPosition` (0=PRE, 1=POST, default POST)

### 5. Build and Test (Debug)

- [ ] Clean build directory: `cmake --build build --target clean`
- [ ] Configure: `cmake -B build -DCMAKE_BUILD_TYPE=Debug`
- [ ] Build: `cmake --build build`
- [ ] Build succeeds without warnings
- [ ] Launch Standalone build
- [ ] WebView loads (not blank screen)
- [ ] Right-click → Inspect works (opens dev tools)
- [ ] Console shows no JavaScript errors
- [ ] Console shows: `JUCE backend connected: [object Object]`
- [ ] Console shows: `DriveVerb UI initialized`
- [ ] Verify `window.__JUCE__` object exists in console

### 6. Test Parameter Binding (Debug)

- [ ] **Size knob**: Drag knob, verify audio changes (reverb room size adjusts)
- [ ] **Decay knob**: Drag knob, verify audio changes (reverb tail length adjusts)
- [ ] **Dry/Wet knob**: Drag knob, verify audio changes (mix adjusts)
- [ ] **Drive knob**: Drag knob, verify audio changes (saturation amount adjusts)
- [ ] **Filter knob**: Drag knob, verify audio changes (DJ-style filter frequency adjusts)
- [ ] **Filter Position toggle**: Click toggle, verify state changes (PRE/POST routing)
- [ ] Automation: Automate any parameter in DAW, verify UI knob follows automation
- [ ] Preset recall: Save preset, change values, recall preset → verify UI updates
- [ ] All 6 parameters sync bidirectionally (UI ↔ audio processor)

### 7. Test VU Meter (Debug)

- [ ] VU meter needle visible at startup
- [ ] Needle animates with ballistic motion (fast attack, slow decay)
- [ ] Needle responds to drive knob changes
- [ ] Needle shows correct range (-20dB to +3dB scale)
- [ ] Red zone indicators visible (+1, +2, +3 dB markings)

### 8. Build and Test (Release)

- [ ] Configure: `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- [ ] Build: `cmake --build build --config Release`
- [ ] Release build succeeds without warnings
- [ ] Launch Release Standalone build
- [ ] WebView loads correctly (not blank)
- [ ] Test all 6 parameters work in Release
- [ ] **Crash test:** Close and reopen plugin 10 times in a row
  - If crashes on close: Member order is wrong in PluginEditor.h
  - Verify: Relays → WebView → Attachments

### 9. WebView-Specific Validation

- [ ] CSS does NOT use viewport units (`100vh`, `100vw`, `100dvh`)
  - Check `v3-ui.html`: Search for `vh` and `vw` → Should find none
  - Should use `height: 100%` instead
- [ ] Native feel CSS present:
  - `user-select: none` on body
  - `cursor: default` on body
  - `-webkit-tap-highlight-color: transparent`
- [ ] Context menu disabled in JavaScript:
  ```javascript
  document.addEventListener("contextmenu", (e) => {
      e.preventDefault();
      return false;
  });
  ```
- [ ] Resource provider returns all files (check console for 404 errors)
- [ ] No `file://` URLs in code (use resource provider root only)

### 10. Cross-Platform Testing (Optional)

#### macOS
- [ ] Builds and runs without additional dependencies
- [ ] WebView uses native WebKit
- [ ] All parameters work correctly

#### Windows (if targeting)
- [ ] WebView2 Runtime installed or downloads automatically
- [ ] All parameters work correctly
- [ ] No blank screen issues

#### Linux (if targeting)
- [ ] webkit2gtk-4.0 installed
- [ ] All parameters work correctly

### 11. Final Validation

- [ ] Parameter values persist after closing and reopening plugin
- [ ] Multiple instances of plugin work independently
- [ ] Plugin passes DAW validation (Logic, Ableton, etc.)
- [ ] No memory leaks (run with Valgrind or Instruments)
- [ ] UI remains responsive under heavy audio load

---

## Common Issues and Solutions

### Issue: Blank screen on plugin load

**Symptoms:** Plugin window is black/blank, no UI visible

**Causes:**
1. Resource provider not returning files correctly
2. Using viewport units (`100vh`) instead of `height: 100%`
3. JUCE_WEB_BROWSER=1 not defined

**Solution:**
1. Verify `getResource()` returns `BinaryData::index_html` for "/" and "/index.html"
2. Check CSS uses `height: 100%` not `100vh`
3. Confirm `target_compile_definitions(DriveVerb PUBLIC JUCE_WEB_BROWSER=1)` in CMakeLists.txt

---

### Issue: Release build crashes on plugin close

**Symptoms:** Debug builds work fine, release builds crash when closing plugin

**Cause:** Member declaration order wrong in PluginEditor.h

**Solution:**
Verify exact order in PluginEditor.h:
```cpp
// 1️⃣ RELAYS FIRST
std::unique_ptr<juce::WebSliderRelay> sizeRelay;
std::unique_ptr<juce::WebSliderRelay> decayRelay;
// ... all relays ...

// 2️⃣ WEBVIEW SECOND
std::unique_ptr<juce::WebBrowserComponent> webView;

// 3️⃣ ATTACHMENTS LAST
std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
// ... all attachments ...
```

**Why:** Members destroy in reverse order. Attachments call `evaluateJavascript()` during destruction, so WebView must still exist.

---

### Issue: Parameters not syncing between UI and audio

**Symptoms:** Moving knob in UI doesn't change audio, or automation doesn't update UI

**Causes:**
1. Parameter IDs mismatch between HTML and C++
2. Attachment not created correctly
3. Relay not registered with WebView

**Solution:**
1. Verify JavaScript uses exact IDs: `Juce.getSliderState("size")` matches C++ `sizeRelay("size")`
2. Check attachment creation: `*audioProcessor.apvts.getParameter("size")` exists
3. Confirm `.withOptionsFrom(*sizeRelay)` in WebView creation

---

### Issue: VU meter not updating

**Symptoms:** VU meter needle stuck at initial position

**Causes:**
1. Timer not started
2. `getCurrentDriveLevel()` not implemented in processor
3. JavaScript event listener not connected

**Solution:**
1. Verify `startTimerHz(16)` called in constructor
2. Implement `getCurrentDriveLevel()` in DriveVerbAudioProcessor (returns current drive saturation in dB)
3. Check JavaScript: `window.__JUCE__.backend.addEventListener("updateVUMeter", ...)`

---

## Next Steps After Integration

Once all checklist items pass:

1. **Test in multiple DAWs** - Logic, Ableton, FL Studio, Reaper
2. **Create factory presets** - Define 5-10 useful starting points
3. **Run pluginval** - Validate plugin meets VST3/AU specifications
4. **Performance test** - Verify low CPU usage, no audio dropouts
5. **Documentation** - Update CHANGELOG.md with v1.0.0 entry

---

## Reference Files

- **Production HTML:** `plugins/DriveVerb/.ideas/mockups/v3-ui.html`
- **Test HTML (browser):** `plugins/DriveVerb/.ideas/mockups/v3-ui-test.html`
- **C++ Header:** `plugins/DriveVerb/.ideas/mockups/v3-PluginEditor.h`
- **C++ Implementation:** `plugins/DriveVerb/.ideas/mockups/v3-PluginEditor.cpp`
- **CMake Snippet:** `plugins/DriveVerb/.ideas/mockups/v3-CMakeLists.txt`
- **Parameter Spec:** `plugins/DriveVerb/.ideas/parameter-spec.md` (generate in Stage 3)

---

## WebView Debugging Tips

**Open dev tools (Debug builds only):**
- Right-click on plugin window → "Inspect"
- Or add to code: `webView->goToURL("about:blank");` then navigate to DevTools

**Console logging:**
```javascript
console.log("Parameter value:", paramState.getNormalisedValue());
console.log("JUCE backend:", window.__JUCE__.backend);
```

**Check parameter state:**
```javascript
// In browser console:
window.__JUCE__.backend
Juce.getSliderState("size").getNormalisedValue()
```

**Monitor events:**
```javascript
// Log all parameter changes
window.__JUCE__.backend.addEventListener("*", (eventName, data) => {
    console.log("Event:", eventName, data);
});
```

---

## Success Criteria

✅ All checklist items complete
✅ Debug build: Plugin loads, parameters work, no console errors
✅ Release build: Plugin loads, parameters work, no crashes
✅ VU meter animates smoothly with drive changes
✅ All 6 parameters sync bidirectionally (UI ↔ processor)
✅ Plugin closes cleanly 10 times in a row (no crashes)
✅ WebView constraints validated (no viewport units, native feel CSS)

**When complete:** DriveVerb UI is production-ready for Stage 5 implementation!
