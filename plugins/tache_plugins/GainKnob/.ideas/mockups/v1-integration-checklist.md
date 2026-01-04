# GainKnob - UI Integration Checklist (v1)

**Generated:** 2025-11-10
**Plugin:** GainKnob
**UI Version:** v1

## Stage 5 (GUI) Integration Steps

### 1. Copy UI Files

- [ ] Create directory: `Source/ui/public/`
- [ ] Create directory: `Source/ui/public/js/juce/`
- [ ] Copy `v1-ui.html` to `Source/ui/public/index.html`
- [ ] Copy JUCE frontend library to `Source/ui/public/js/juce/index.js`
  - Source: `<JUCE>/modules/juce_gui_extra/native/javaScriptCore/lib/index.js`
  - Or extract from existing JUCE plugin with WebView

### 2. Update PluginEditor Files

- [ ] Backup existing `PluginEditor.h` and `PluginEditor.cpp` (if they exist)
- [ ] Replace `PluginEditor.h` with `v1-PluginEditor.h`
- [ ] Verify member order in header: `gainRelay` → `webView` → `gainAttachment`
- [ ] Replace `PluginEditor.cpp` with `v1-PluginEditor.cpp`
- [ ] Verify initialization order matches member order

### 3. Update CMakeLists.txt

- [ ] Add `juce_add_binary_data` for UI resources (from `v1-CMakeLists.txt`)
- [ ] Link `GainKnob_UIResources` to plugin
- [ ] Add `JUCE_WEB_BROWSER=1` definition
- [ ] Add `JUCE_USE_CURL=0` definition
- [ ] Add platform-specific config if targeting Windows/Linux

### 4. Build and Test (Debug)

- [ ] Clean build directory: `rm -rf build/`
- [ ] Configure CMake: `cmake -B build -DCMAKE_BUILD_TYPE=Debug`
- [ ] Build plugin: `cmake --build build --config Debug`
- [ ] Build succeeds without warnings
- [ ] Launch Standalone: `build/GainKnob_artefacts/Debug/Standalone/GainKnob.app`
- [ ] WebView loads (not blank screen)
- [ ] Right-click → Inspect works (developer tools)
- [ ] Console shows no JavaScript errors
- [ ] Console shows: "JUCE backend connected"
- [ ] Console shows: "GainKnob UI initialized"
- [ ] `window.__JUCE__` object exists in console

### 5. Test Parameter Binding (Debug)

- [ ] Drag knob in UI - value display updates
- [ ] Drag knob in UI - audio level changes
- [ ] Check console for parameter messages: `[GAIN] set to X.X dB`
- [ ] At minimum position, display shows "−∞ dB"
- [ ] At maximum position, display shows "0.0 dB"

### 6. Build and Test (Release)

- [ ] Configure CMake: `cmake -B build -DCMAKE_BUILD_TYPE=Release`
- [ ] Build plugin: `cmake --build build --config Release`
- [ ] Release build succeeds without warnings
- [ ] Launch Standalone: `build/GainKnob_artefacts/Release/Standalone/GainKnob.app`
- [ ] Release build runs without crashes
- [ ] Test parameter binding in Release (same as Debug tests)
- [ ] Reload plugin 10 times (test destructor stability)

### 7. Test in DAW

- [ ] Load plugin in DAW (e.g., Logic Pro, Ableton, FL Studio)
- [ ] UI appears correctly
- [ ] Moving UI knob changes audio output
- [ ] DAW automation moves UI knob
- [ ] Saving and reloading project preserves parameter value
- [ ] Multiple instances work independently

### 8. WebView-Specific Validation

- [ ] Verify member order in `PluginEditor.h`: `gainRelay` → `webView` → `gainAttachment`
- [ ] Resource provider returns all files (no 404 in console)
- [ ] Parameter binding works (automation/preset recall)
- [ ] Test in Debug AND Release builds
- [ ] No crashes on plugin close (reload 10 times)
- [ ] CSS does NOT use viewport units (`100vh`, `100vw`)
- [ ] Native feel CSS present (`user-select: none`)
- [ ] Right-click context menu disabled

## Common Issues

### Blank WebView on Load
- **Cause:** Using `100vh` or `100vw` in CSS
- **Fix:** Replace with `height: 100%` on `html, body`

### JavaScript Error: "window.__JUCE__ is undefined"
- **Cause:** `.withNativeIntegrationEnabled()` missing from WebView options
- **Fix:** Verify WebView options in `PluginEditor.cpp`

### Parameter Binding Doesn't Work
- **Cause:** Parameter ID mismatch between HTML and C++
- **Fix:** Verify `Juce.getSliderState("GAIN")` matches `gainRelay("GAIN")`

### Release Build Crashes
- **Cause:** Wrong member order in header file
- **Fix:** Verify order is `gainRelay` → `webView` → `gainAttachment`

### Resource Not Found (404)
- **Cause:** BinaryData identifier doesn't match file path
- **Fix:** Verify `BinaryData::index_html` and `BinaryData::juce_index_js` exist

## Next Steps

After all checks pass:
- [ ] Stage 5 complete
- [ ] Proceed to Stage 6 (Testing) or Stage 7 (Installation)
- [ ] Update PLUGINS.md status

## Notes

- Window size: 400x400 (fixed, not resizable)
- Single parameter: GAIN (-60 dB to 0 dB)
- Dark theme with blue accent (#4a9eff)
- Rotary knob with canvas rendering
