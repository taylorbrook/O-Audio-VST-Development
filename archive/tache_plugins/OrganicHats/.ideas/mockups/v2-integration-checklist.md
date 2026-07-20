# OrganicHats - UI Integration Checklist (v2)

## Stage 5 (GUI) Integration Steps

### 1. Copy UI Files
- [ ] Create directory: `Source/ui/public/`
- [ ] Copy `v2-ui.html` to `Source/ui/public/index.html`
- [ ] Verify HTML file is valid and complete

### 2. Update PluginEditor Files
- [ ] Replace `Source/PluginEditor.h` with `v2-PluginEditor.h`
- [ ] Verify member order: relays → webView → attachments (CRITICAL for release builds)
- [ ] Replace `Source/PluginEditor.cpp` with `v2-PluginEditor.cpp`
- [ ] Verify initialization order matches member order

### 3. Update CMakeLists.txt
- [ ] Add `juce_add_binary_data` section from `v2-CMakeLists.txt`
- [ ] Link `OrganicHats_UIResources` to plugin target
- [ ] Add `JUCE_WEB_BROWSER=1` definition
- [ ] Add `juce::juce_gui_extra` to target_link_libraries

### 4. Build and Test (Debug)
- [ ] Build succeeds without warnings
- [ ] Standalone loads WebView (not blank screen)
- [ ] Right-click → Inspect works (dev tools available)
- [ ] Console shows no JavaScript errors
- [ ] `window.__JUCE__` object exists in console
- [ ] Parameter state objects accessible via `window.__JUCE__.backend.getSliderState()`

### 5. Build and Test (Release)
- [ ] Release build succeeds without warnings
- [ ] Release build runs (tests member order - crashes indicate ordering issue)
- [ ] No crashes on plugin reload (test 10 times in DAW)

### 6. Test Parameter Binding
- [ ] Moving UI knob (CLOSED_TONE) changes audio
- [ ] Changing parameter in DAW updates UI knob
- [ ] Parameter values persist after reload
- [ ] All 6 parameters sync independently (closed tone, decay, noise + open tone, release, noise)

### 7. WebView-Specific Validation
- [ ] Verify member order in PluginEditor.h: relays declared BEFORE webView
- [ ] Verify attachments declared AFTER webView
- [ ] Test resource provider returns index.html (no 404 in console)
- [ ] Verify parameter binding works for all 6 controls
- [ ] Test in Debug and Release builds
- [ ] Check for crashes on plugin close (reload 10 times in DAW)
- [ ] Verify CSS does NOT use viewport units (100vh, 100vw)
- [ ] Verify native feel CSS present (user-select: none)

### 8. Visual Verification
- [ ] Window size is 600x590px (matches mockup)
- [ ] Dual-panel layout renders correctly (closed left, open right)
- [ ] Knobs display with machined texture (conic gradient)
- [ ] Digital readouts show correct values and units
- [ ] Power LED indicator visible and pulsing
- [ ] Mounting holes visible at corners
- [ ] Brushed metal background texture renders
- [ ] Yellow-gold color scheme (#ffc864) displays correctly

### 9. Interaction Testing
- [ ] Knobs respond to mouse drag (up = increase, down = decrease)
- [ ] Knob indicators rotate smoothly (-135° to +135° range)
- [ ] Digital readouts update in real-time
- [ ] Values display with correct units (% for tone/color, ms for decay/release)
- [ ] Context menu disabled (right-click doesn't show browser menu)

### 10. Parameter Verification
- [ ] CLOSED_TONE: 0-100%, default 50%
- [ ] CLOSED_DECAY: 20-200ms, default 80ms
- [ ] CLOSED_NOISE_COLOR: 0-100%, default 50%
- [ ] OPEN_TONE: 0-100%, default 50%
- [ ] OPEN_RELEASE: 100-1000ms, default 400ms
- [ ] OPEN_NOISE_COLOR: 0-100%, default 50%

## Common Issues & Solutions

### Blank WebView
- **Cause:** Resource provider not returning HTML
- **Fix:** Verify `BinaryData::indexhtml` exists and `juce_add_binary_data` configured correctly

### Parameter Not Updating
- **Cause:** Parameter ID mismatch between APVTS and UI
- **Fix:** Verify parameter IDs match exactly (case-sensitive): `CLOSED_TONE`, `CLOSED_DECAY`, etc.

### Release Build Crash
- **Cause:** Member initialization order doesn't match declaration order
- **Fix:** Ensure relays → webView → attachments order in both .h and .cpp

### Knobs Not Dragging
- **Cause:** JavaScript not loading or `window.__JUCE__` undefined
- **Fix:** Check console for errors, verify WebView initialized before attachments

## Notes

- Velocity sensitivity is always enabled (no UI controls)
- Preset management handled by DAW
- Studio Hardware aesthetic with industrial styling
- 6 parameters total (3 closed + 3 open)
