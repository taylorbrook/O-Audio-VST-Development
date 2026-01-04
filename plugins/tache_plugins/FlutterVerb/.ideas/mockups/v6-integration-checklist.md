# FlutterVerb Mockup v6 - Integration Checklist

**UI Design:** 600×640px, TapeAge-inspired dark gradient with brass accents
**Parameters:** 7 total (SIZE, DECAY, MIX, AGE, DRIVE, TONE, MOD_MODE)
**Status:** Ready for Stage 5 (GUI) implementation

---

## Stage 5 (GUI) Integration Steps

### Phase 1: Copy UI Files

- [ ] **Create directory structure**
  ```bash
  mkdir -p Source/ui/public/js/juce
  ```

- [ ] **Copy production HTML**
  ```bash
  cp .ideas/mockups/v6-ui.html Source/ui/public/index.html
  ```

- [ ] **Copy JUCE frontend library**
  ```bash
  # Download from JUCE modules:
  cp /path/to/JUCE/modules/juce_gui_extra/embedding/assets/index.js \
     Source/ui/public/js/juce/index.js
  ```

  **Note:** The JUCE frontend library is located in:
  `JUCE/modules/juce_gui_extra/embedding/assets/index.js`

- [ ] **Verify file structure**
  ```
  Source/
  └── ui/
      └── public/
          ├── index.html (v6-ui.html)
          └── js/
              └── juce/
                  └── index.js (JUCE frontend library)
  ```

### Phase 2: Update C++ Files

- [ ] **Replace PluginEditor.h**
  ```bash
  cp .ideas/mockups/v6-PluginEditor.h Source/PluginEditor.h
  ```

  **Critical checks:**
  - [ ] Member order: relays → webView → attachments
  - [ ] All 7 parameters have relay declarations (6 sliders + 1 toggle)
  - [ ] Includes `<JuceHeader.h>` and `"PluginProcessor.h"`

- [ ] **Replace PluginEditor.cpp**
  ```bash
  cp .ideas/mockups/v6-PluginEditor.cpp Source/PluginEditor.cpp
  ```

  **Critical checks:**
  - [ ] Initialization order matches member order
  - [ ] All 7 relays created BEFORE WebView
  - [ ] All 7 relays registered via `.withOptionsFrom(*relay)`
  - [ ] All 7 attachments created AFTER WebView
  - [ ] Window size: `setSize(600, 640)`
  - [ ] Resizable: `setResizable(false, false)`

### Phase 3: Update CMakeLists.txt

- [ ] **Add WebView configuration**
  - [ ] Open `CMakeLists.txt`
  - [ ] Locate `juce_add_plugin(FlutterVerb ...)` call
  - [ ] ADD contents of `v6-CMakeLists.txt` AFTER `juce_add_plugin()`

  **What to add:**
  - `juce_add_binary_data(FlutterVerb_UIResources ...)` - Embed UI files
  - `target_link_libraries(... FlutterVerb_UIResources ...)` - Link resources
  - `target_compile_definitions(... JUCE_WEB_BROWSER=1 ...)` - Enable WebView
  - Platform-specific configuration (macOS/Windows/Linux)

- [ ] **Verify resource paths in `juce_add_binary_data`**
  - [ ] `Source/ui/public/index.html` exists
  - [ ] `Source/ui/public/js/juce/index.js` exists

### Phase 4: Build and Test (Debug)

- [ ] **Reconfigure CMake** (picks up new UI resources)
  ```bash
  cmake -B build
  ```

- [ ] **Build Debug**
  ```bash
  cmake --build build --config Debug
  ```

- [ ] **Launch Standalone (Debug)**
  ```bash
  open build/FlutterVerb_artefacts/Debug/Standalone/FlutterVerb.app
  ```

- [ ] **Verify WebView loads**
  - [ ] Plugin window opens (not blank screen)
  - [ ] UI visible (VU meter, title, 6 knobs, toggle)
  - [ ] Right-click → Inspect works (dev tools open)
  - [ ] No blank screen

- [ ] **Check JavaScript console**
  - [ ] No errors in console
  - [ ] `window.__JUCE__` object exists
  - [ ] `console.log("FlutterVerb v6 mockup loaded")` visible
  - [ ] No 404 errors for resources

- [ ] **Verify parameter state objects accessible**
  - [ ] Open console, type: `Juce.getSliderState("SIZE")`
  - [ ] Should return state object (not undefined)
  - [ ] Repeat for DECAY, MIX, AGE, DRIVE, TONE
  - [ ] Check toggle: `Juce.getToggleButtonState("MOD_MODE")`

### Phase 5: Build and Test (Release)

- [ ] **Build Release**
  ```bash
  cmake --build build --config Release
  ```

- [ ] **Launch Standalone (Release)**
  ```bash
  open build/FlutterVerb_artefacts/Release/Standalone/FlutterVerb.app
  ```

- [ ] **Verify Release build runs without crash**
  - [ ] Plugin opens successfully
  - [ ] UI renders correctly
  - [ ] No crashes on startup

- [ ] **Test plugin reload (critical for member order validation)**
  - [ ] Open plugin in DAW
  - [ ] Close plugin window
  - [ ] Reopen plugin window
  - [ ] Repeat 10 times (stress test for destructor order)
  - [ ] No crashes during reload cycle

### Phase 6: Test Parameter Binding

- [ ] **Test UI → C++ (user interaction)**
  - [ ] Drag SIZE knob, verify audio changes
  - [ ] Drag DECAY knob, verify audio changes
  - [ ] Drag MIX knob, verify audio changes
  - [ ] Drag AGE knob, verify audio changes
  - [ ] Drag DRIVE knob, verify audio changes
  - [ ] Drag TONE knob, verify audio changes
  - [ ] Click MOD_MODE toggle, verify audio changes

- [ ] **Test C++ → UI (automation/preset recall)**
  - [ ] Automate SIZE parameter in DAW
  - [ ] Verify knob moves in UI during playback
  - [ ] Repeat for other parameters
  - [ ] Create preset, change parameters, recall preset
  - [ ] Verify UI updates to match recalled preset values

- [ ] **Test parameter persistence**
  - [ ] Set all parameters to non-default values
  - [ ] Close plugin
  - [ ] Reopen plugin
  - [ ] Verify parameters retained (not reset to defaults)

### Phase 7: WebView-Specific Validation

- [ ] **Verify CSS constraints**
  - [ ] Inspect index.html
  - [ ] Confirm NO viewport units (`100vh`, `100vw`, `100dvh`, `100svh`)
  - [ ] Confirm `html, body { height: 100%; }` present
  - [ ] Confirm `user-select: none` present (native feel)

- [ ] **Verify resource provider**
  - [ ] Check console for 404 errors
  - [ ] All resources load successfully
  - [ ] `getResource("/")` returns `index_html`
  - [ ] `getResource("/js/juce/index.js")` returns `juce_index_js`

- [ ] **Verify member order (PluginEditor.h)**
  - [ ] Relays declared FIRST (lines ~88-94)
  - [ ] WebView declared SECOND (line ~106)
  - [ ] Attachments declared LAST (lines ~122-128)
  - [ ] NO attachments declared before WebView

- [ ] **Test in Debug and Release builds**
  - [ ] Both builds open successfully
  - [ ] No crashes in either configuration
  - [ ] Parameter binding works in both

### Phase 8: VU Meter Integration (Optional)

**Note:** VU meter currently uses mock animation. To connect to real audio:

- [ ] **Add atomic variable to PluginProcessor**
  ```cpp
  // PluginProcessor.h
  std::atomic<float> outputLevel { -60.0f };
  ```

- [ ] **Update level in processBlock**
  ```cpp
  // PluginProcessor.cpp processBlock()
  float peakLevel = buffer.getMagnitude(0, buffer.getNumSamples());
  float peakDb = juce::Decibels::gainToDecibels(peakLevel, -60.0f);
  outputLevel.store(peakDb);
  ```

- [ ] **Emit event from timer callback**
  ```cpp
  // PluginEditor.cpp timerCallback()
  float level = audioProcessor.outputLevel.load();
  webView->emitEventIfBrowserIsVisible("vuMeterUpdate", {
      { "level", level }
  });
  ```

- [ ] **Listen in JavaScript (already in index.html mock)**
  - Replace mock animation with real event listener
  - Update needle angle based on `data.level`

---

## Post-Integration Validation

### Checklist Summary

- [ ] **Build Success**
  - [ ] Debug build compiles without warnings
  - [ ] Release build compiles without warnings
  - [ ] No linker errors related to WebView

- [ ] **UI Rendering**
  - [ ] Plugin window opens (not blank)
  - [ ] All controls visible (VU meter, 6 knobs, toggle)
  - [ ] Visual style matches mockup v6
  - [ ] Window size correct (600×640px)

- [ ] **Parameter Binding**
  - [ ] All 7 parameters bindable in UI
  - [ ] UI changes affect audio (user interaction works)
  - [ ] Automation updates UI (C++ → UI works)
  - [ ] Preset recall updates UI

- [ ] **Stability**
  - [ ] No crashes in Debug
  - [ ] No crashes in Release
  - [ ] Plugin reload cycle stable (10+ times)
  - [ ] Resource provider returns all files (no 404s)

- [ ] **WebView Constraints**
  - [ ] No viewport units in CSS
  - [ ] `height: 100%` used instead
  - [ ] Native feel CSS applied (`user-select: none`)
  - [ ] Context menu disabled

---

## Common Issues & Solutions

### Issue: Blank Screen on Load

**Symptoms:** Plugin window opens but shows only white/black screen

**Solutions:**
1. Check CSS: Remove `100vh`/`100vw`, use `height: 100%`
2. Verify resource provider: Add `console.log(url)` in `getResource()`
3. Check console: Right-click → Inspect, look for errors
4. Verify JUCE library: Ensure `/js/juce/index.js` returns correct data

### Issue: Parameters Not Binding

**Symptoms:** Moving UI controls doesn't affect audio

**Solutions:**
1. Check relay creation: Verify parameter IDs match APVTS
2. Check WebView options: Verify `.withOptionsFrom(*relay)` for all relays
3. Check attachments: Verify created AFTER WebView
4. Check console: `Juce.getSliderState("SIZE")` should return object

### Issue: Release Build Crashes

**Symptoms:** Debug works, Release crashes on close/reload

**Solutions:**
1. **CHECK MEMBER ORDER** (most common cause!)
2. Verify PluginEditor.h order: relays → webView → attachments
3. Never declare attachments before WebView
4. Test reload cycle 10+ times in Debug first

### Issue: 404 Errors for Resources

**Symptoms:** Console shows "Failed to load resource: 404"

**Solutions:**
1. Verify file paths in `juce_add_binary_data()`
2. Check `getResource()` URL mapping
3. Reconfigure CMake: `cmake -B build` (picks up new resources)
4. Verify BinaryData symbols match: `index_html`, `juce_index_js`

### Issue: VU Meter Not Updating

**Symptoms:** VU meter stuck or shows mock animation

**Solutions:**
1. Implement atomic variable in PluginProcessor
2. Update level in `processBlock()`
3. Emit event from `timerCallback()`
4. Replace mock animation in JavaScript with event listener

---

## Integration Complete

When all checkboxes are marked, FlutterVerb Stage 5 (GUI) is complete and ready for Stage 6 (Validation).

**Next steps:**
- Run `/test FlutterVerb` for automated validation
- Test in multiple DAWs (Ableton, Logic, FL Studio)
- Create factory presets
- Generate CHANGELOG.md
