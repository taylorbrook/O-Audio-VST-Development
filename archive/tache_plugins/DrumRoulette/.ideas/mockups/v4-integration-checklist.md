# DrumRoulette UI Integration Checklist (v4)

**Generated:** 2025-11-12
**Mockup Version:** v4
**Window Size:** 1400 × 950
**Parameters:** 57 total (8 slots × 7 params + 1 master)

## Stage 5 (GUI) Integration Steps

### 1. Copy UI Files

- [ ] Create directory structure: `Source/ui/public/js/juce/`
- [ ] Copy `v4-ui.html` to `Source/ui/public/index.html`
- [ ] Copy JUCE frontend library to `Source/ui/public/js/juce/index.js`
  - Library location: JUCE examples or `juce_gui_extra` module
  - File provides `window.__JUCE__` API for parameter binding

### 2. Update PluginEditor Files

- [ ] Replace `Source/PluginEditor.h` with `v4-PluginEditor.h`
- [ ] **CRITICAL:** Verify member order in header:
  - ✓ Relays declared first (57 relay members)
  - ✓ `webView` declared after relays
  - ✓ Attachments declared last (57 attachment members)
- [ ] Replace `Source/PluginEditor.cpp` with `v4-PluginEditor.cpp`
- [ ] **CRITICAL:** Verify initialization order matches declaration order:
  - ✓ Relays initialized first (in constructor initializer list)
  - ✓ `webView` initialized after relays
  - ✓ Attachments initialized last

### 3. Update CMakeLists.txt

- [ ] Append contents of `v4-CMakeLists.txt` to main `CMakeLists.txt`
- [ ] Add `juce_add_binary_data(DrumRoulette_UIResources ...)` section
- [ ] Link `DrumRoulette_UIResources` to plugin target
- [ ] Add `JUCE_WEB_BROWSER=1` compile definition
- [ ] Remove any duplicate `target_link_libraries()` or `target_compile_definitions()` calls

### 4. Build and Test (Debug)

- [ ] Build succeeds without warnings
- [ ] Standalone application launches
- [ ] WebView loads (window is NOT blank)
- [ ] All 8 channel strips visible
- [ ] Right-click → Inspect opens DevTools (verify in Debug build)
- [ ] Console shows no JavaScript errors
- [ ] `window.__JUCE__` object exists in console
- [ ] Parameter state objects accessible (check one: `__JUCE__.getSliderState("DECAY_1")`)

### 5. Build and Test (Release)

- [ ] Release build succeeds without warnings
- [ ] Release build runs without crashes
- [ ] **CRITICAL:** Test member order logic - reload plugin 10 times (no crashes)
- [ ] WebView renders correctly in Release build

### 6. Test Parameter Binding

**Per-Slot Parameters (test at least 2 slots):**

- [ ] Moving Decay knob in UI changes parameter value
- [ ] Moving Attack knob in UI changes parameter value
- [ ] Moving Tilt knob in UI changes parameter value
- [ ] Moving Pitch knob in UI changes parameter value
- [ ] Moving Volume fader in UI changes parameter value
- [ ] Clicking Solo button toggles state
- [ ] Clicking Mute button toggles state

**Cross-Slot Behavior:**

- [ ] Parameters for different slots are independent
- [ ] Solo on one slot doesn't affect other slots
- [ ] Mute on one slot doesn't affect other slots

**Master Parameters:**

- [ ] Master Volume knob changes master parameter value
- [ ] LED meter animates with audio output

**DAW Integration:**

- [ ] Changing parameter in DAW updates UI control
- [ ] Parameter values persist after plugin reload
- [ ] Automation works (record automation, playback reflects in UI)
- [ ] Preset recall updates UI to match preset values

### 7. WebView-Specific Validation

- [ ] Verify member order in `PluginEditor.h`:
  - ✓ 57 relays → `webView` → 57 attachments
- [ ] Test resource provider returns all files (no 404 in DevTools console)
- [ ] Verify parameter binding (automation/preset recall)
- [ ] Test in Debug and Release builds
- [ ] Check for crashes on plugin close (reload 10 times)
- [ ] CSS does NOT use viewport units (`100vh`, `100vw`)
- [ ] Native feel CSS present (`user-select: none`)
- [ ] Context menu disabled (right-click doesn't show browser context menu in Release)

### 8. Visual/Interactive Testing

- [ ] All 8 channel strips render correctly
- [ ] Knobs rotate smoothly with mouse drag
- [ ] Faders slide smoothly with mouse drag
- [ ] Buttons highlight on hover
- [ ] Buttons change state on click (Solo/Mute/Lock)
- [ ] Sample name displays update (mock for now)
- [ ] Trigger LEDs flash (mock for now)
- [ ] Master LED meter animates
- [ ] Rack mounting holes visible
- [ ] Brushed metal texture visible
- [ ] Typography renders correctly

### 9. Cross-Browser/Platform Testing (Optional)

- [ ] macOS: WebKit backend works
- [ ] Windows: WebView2 backend works
- [ ] Linux: WebKitGTK backend works (if targeting Linux)

### 10. Integration Complete

- [ ] All parameters bound correctly (57 total)
- [ ] UI matches mockup design exactly
- [ ] No console errors in Debug or Release
- [ ] No crashes after 10 plugin reloads
- [ ] Ready to proceed to Stage 4 (DSP implementation)

---

## Common Issues and Solutions

**Issue:** WebView shows blank screen
**Solution:** Check BinaryData includes index.html. Verify resource provider paths match.

**Issue:** Console shows "Uncaught ReferenceError: window.__JUCE__ is not defined"
**Solution:** Copy JUCE frontend library to `Source/ui/public/js/juce/index.js`

**Issue:** Crashes on plugin reload (Release build only)
**Solution:** Verify member order in PluginEditor.h (relays → webView → attachments)

**Issue:** Parameter changes in DAW don't update UI
**Solution:** Verify attachments are created with correct relay and parameter ID

**Issue:** UI shows but parameters don't respond
**Solution:** Check parameter IDs match between APVTS, HTML, and attachments

**Issue:** DevTools not available (Release build)
**Solution:** This is expected. DevTools only available in Debug builds.

---

## Next Steps After Integration

1. Test UI in your DAW of choice (Logic Pro, Ableton, etc.)
2. Verify all 57 parameters are automatable
3. Proceed to Stage 4 (DSP implementation)
4. Wire up sample loading, randomization, and playback logic
5. Connect trigger LEDs to MIDI note-on events
6. Implement folder browser (native OS dialog)
7. Return to Stage 5 to wire UI events to DSP logic
