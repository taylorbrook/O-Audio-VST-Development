# Stage 3 (GUI) Integration Checklist - v8

**Plugin:** OuariconComp
**Mockup Version:** v8
**Generated:** 2026-01-11

This checklist guides gui-agent through integrating the finalized v8 WebView UI into OuariconComp during Stage 3 (GUI) implementation.

---

## Overview

**Implementation files:**
- v8-ui.html - Production HTML
- v8-PluginEditor.h - C++ header template
- v8-PluginEditor.cpp - C++ implementation template
- v8-CMakeLists.txt - CMake configuration snippet

**Parameters:** 7 total (6 sliders + 1 toggle)
- threshold (slider)
- ratio (slider)
- attack_time (slider)
- release_time (slider)
- knee (slider)
- output_gain (slider)
- auto_gain (toggle)

**Window dimensions:** 620x360px (fixed, non-resizable)

---

## Phase 1: Copy UI Files

**Destination:** `plugins/OuariconComp/Source/ui/public/`

- [ ] Copy v8-ui.html to `Source/ui/public/index.html`
- [ ] Copy JUCE WebView library to `Source/ui/public/js/juce/index.js`
- [ ] Copy JUCE interop checker to `Source/ui/public/js/juce/check_native_interop.js`
- [ ] Copy paper-bg.jpg to `Source/ui/public/paper-bg.jpg`
- [ ] Copy shell.png to `Source/ui/public/shell.png`

**Verify:** All 5 files exist in `Source/ui/public/` directory structure.

---

## Phase 2: Update PluginEditor Files

### 2.1: Update PluginEditor.h

**Location:** `plugins/OuariconComp/Source/PluginEditor.h`

**Actions:**
- [ ] Replace header content with v8-PluginEditor.h template
- [ ] Verify class name: `OuariconCompAudioProcessorEditor`
- [ ] Verify member order: relays → webView → attachments
- [ ] Count relays: Should be 7 (6 sliders + 1 toggle)
- [ ] Count attachments: Should be 7 (matches relay count)

**Critical verification:**
```cpp
// Order MUST be:
// 1. Relays (thresholdRelay, ratioRelay, ...)
// 2. WebView (webView)
// 3. Attachments (thresholdAttachment, ratioAttachment, ...)
```

### 2.2: Update PluginEditor.cpp

**Location:** `plugins/OuariconComp/Source/PluginEditor.cpp`

**Actions:**
- [ ] Replace implementation with v8-PluginEditor.cpp template
- [ ] Verify initialization order matches declaration order
- [ ] Verify all 7 parameters have attachments with THREE arguments (param, relay, nullptr)
- [ ] Verify `sendInitialUpdate()` called for all 7 attachments
- [ ] Verify resource provider handles all 5 files (HTML, 2 JS, 2 images)
- [ ] Verify MIME types correct:
  - index.html → "text/html"
  - index.js → "application/javascript"
  - check_native_interop.js → "application/javascript"
  - paper-bg.jpg → "image/jpeg"
  - shell.png → "image/png"

**Critical verification:**
```cpp
// JUCE 8 requires THREE parameters (not two)
thresholdAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *audioProcessor.parameters.getParameter("threshold"),
    *thresholdRelay,
    nullptr  // ← REQUIRED third parameter
);
```

---

## Phase 3: Update CMakeLists.txt

**Location:** `plugins/OuariconComp/CMakeLists.txt`

**Actions:**
- [ ] Append v8-CMakeLists.txt snippet to CMakeLists.txt
- [ ] Verify `juce_add_binary_data(OuariconComp_UIResources ...)` includes all 5 files
- [ ] Verify `target_link_libraries` includes `juce::juce_gui_extra`
- [ ] Verify `JUCE_WEB_BROWSER=1` definition present
- [ ] Verify `JUCE_USE_CURL=0` definition present
- [ ] Verify `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin()` section

**Critical verification:**
```cmake
# REQUIRED for VST3 WebView support
juce_add_plugin(OuariconComp
    # ... other options ...
    NEEDS_WEB_BROWSER TRUE  # ← CRITICAL for VST3
)
```

---

## Phase 4: Build and Test (Debug)

**Build commands:**
```bash
cd plugins/OuariconComp
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**Verification checklist:**
- [ ] Build succeeds without warnings
- [ ] BinaryData header generated with all 5 resources
- [ ] VST3 and AU binaries created
- [ ] Standalone application created

**Install and test:**
```bash
./scripts/build-and-install.sh OuariconComp
```

**Standalone test:**
- [ ] Standalone loads without crashes
- [ ] WebView displays UI (not blank)
- [ ] Right-click → Inspect works (WebView DevTools)
- [ ] Console shows no JavaScript errors
- [ ] `window.__JUCE__` object exists in console

**Visual verification:**
- [ ] Background shows paper texture
- [ ] Title shows "Ouaricon Compressor" (centered)
- [ ] 6 seed knobs visible with labels
- [ ] Auto-Gain toggle button visible
- [ ] Transfer curve panel shows shell overlay
- [ ] All 4 visualization panels render
- [ ] Input and output meters animate

---

## Phase 5: Build and Test (Release)

**Build commands:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./scripts/build-and-install.sh OuariconComp
```

**Critical member order test:**
- [ ] Release build succeeds
- [ ] Plugin loads in DAW (no crash on open)
- [ ] Plugin reloads successfully (test 10 times)
- [ ] No crashes when closing/reopening plugin window

**If crashes occur:** Check member declaration order in PluginEditor.h (attachments MUST come AFTER webView).

---

## Phase 6: Test Parameter Binding

**Test each parameter:**

### Threshold
- [ ] Drag knob → value updates in display
- [ ] DAW automation → knob rotates in UI
- [ ] Double-click → resets to default
- [ ] Transfer curve updates when changed

### Ratio
- [ ] Drag knob → value updates in display
- [ ] DAW automation → knob rotates in UI
- [ ] Double-click → resets to default
- [ ] Transfer curve updates when changed

### Attack Time
- [ ] Drag knob → value updates in display
- [ ] DAW automation → knob rotates in UI
- [ ] Double-click → resets to default

### Release Time
- [ ] Drag knob → value updates in display
- [ ] DAW automation → knob rotates in UI
- [ ] Double-click → resets to default

### Knee
- [ ] Drag knob → value updates in display
- [ ] DAW automation → knob rotates in UI
- [ ] Double-click → resets to default
- [ ] Transfer curve updates when changed

### Output Gain
- [ ] Drag knob → value updates in display
- [ ] DAW automation → knob rotates in UI
- [ ] Double-click → resets to default

### Auto-Gain Toggle
- [ ] Click → toggles between ON/OFF
- [ ] DAW automation → button state updates
- [ ] Visual state changes (color/text)

**Preset test:**
- [ ] Save preset → all values stored
- [ ] Load preset → all UI controls update
- [ ] Switch presets → UI reflects changes

**DAW test:**
- [ ] Host automation writes parameters
- [ ] UI updates in real-time during playback
- [ ] No parameter jumps or glitches

---

## Phase 7: WebView-Specific Validation

### CSS Constraints
- [ ] No viewport units used (`100vh`, `100vw`)
- [ ] `html, body { height: 100%; }` present
- [ ] `user-select: none` applied to body
- [ ] Context menu disabled in JavaScript

### Resource Loading
- [ ] All 5 resources load (no 404s in console)
- [ ] MIME types correct (check Network tab in DevTools)
- [ ] Paper background image displays
- [ ] Shell overlay displays at 50% opacity

### JavaScript Integration
- [ ] `type="module"` attribute on script tags
- [ ] ES6 imports work (`import { getSliderState } from ...`)
- [ ] `getSliderState()` returns valid state objects
- [ ] `getToggleState()` returns valid state object
- [ ] `valueChangedEvent.addListener()` fires on parameter changes

### Knob Interaction
- [ ] Relative drag (frame-delta, not absolute positioning)
- [ ] Drag up increases value, drag down decreases
- [ ] Smooth rotation (not jumpy)
- [ ] Can drag infinitely (cursor position doesn't matter)

---

## Phase 8: DAW Testing

**Test in multiple DAWs:**

### Ableton Live
- [ ] VST3 appears in browser
- [ ] AU appears in browser
- [ ] Plugin opens without errors
- [ ] UI renders correctly
- [ ] Parameters automate
- [ ] Presets save/load

### Logic Pro
- [ ] AU appears in plugin manager
- [ ] Plugin opens without errors
- [ ] UI renders correctly
- [ ] Parameters automate
- [ ] Presets save/load

### Standalone
- [ ] Application launches
- [ ] UI renders correctly
- [ ] Audio processes
- [ ] Parameters work

---

## Common Issues and Solutions

### Issue: Blank WebView (white screen)

**Possible causes:**
- Resource provider not returning index.html
- MIME type incorrect
- BinaryData not regenerated after copying files

**Solution:**
1. Check console for 404 errors
2. Verify `getResource()` handles "/" and "/index.html"
3. Rebuild: `cmake --build build --clean-first`

### Issue: Knobs don't respond to drag

**Possible causes:**
- Missing `nullptr` third parameter in attachments (JUCE 8)
- Missing `type="module"` in script tags
- `getSliderState()` returning null

**Solution:**
1. Check all attachments have THREE parameters
2. Add `type="module"` to script tags
3. Add `import { getSliderState }` to inline script
4. Check console for "Failed to get slider state" errors

### Issue: Plugin crashes on reload (release build only)

**Possible causes:**
- Member order violation (attachments before webView)

**Solution:**
1. Verify PluginEditor.h member order: relays → webView → attachments
2. Members destroyed in REVERSE order (attachments first, then webView)

### Issue: Transfer curve doesn't update

**Possible causes:**
- Missing parameter change listeners
- Canvas not clearing properly

**Solution:**
1. Verify `updateTransferCurve()` called in knob update handlers
2. Check threshold, ratio, knee listeners trigger curve redraw

---

## Success Criteria

**All phases complete when:**

1. ✅ All 5 UI files copied to `Source/ui/public/`
2. ✅ PluginEditor.h/cpp updated with correct member order
3. ✅ CMakeLists.txt includes WebView configuration
4. ✅ Debug build succeeds without warnings
5. ✅ Release build succeeds without crashes
6. ✅ All 7 parameters bind correctly (UI ↔ APVTS)
7. ✅ WebView loads and renders properly
8. ✅ Visualizations animate (transfer curve, meters, envelope)
9. ✅ DAW automation works
10. ✅ Presets save/load correctly

---

## Next Steps

After successful integration:

1. Mark Stage 3 (GUI) as complete in `.continue-here.md`
2. Run full test suite in all target DAWs
3. Document any visual tweaks needed
4. Consider adding custom visualizations (real-time gain reduction, etc.)
5. Optimize canvas rendering if needed

---

**Generated:** 2026-01-11
**Plugin:** OuariconComp
**Version:** v8
**Parameters:** 7 (threshold, ratio, attack_time, release_time, knee, output_gain, auto_gain)
**Window:** 620x360px fixed
