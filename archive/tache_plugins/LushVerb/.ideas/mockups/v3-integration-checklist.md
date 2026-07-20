# LushVerb - WebView UI Integration Checklist

**Mockup Version:** v3
**Parameters:** SIZE, DAMPING, SHIMMER, MIX (4 total)
**Window Size:** 500×300px
**Generated:** 2025-11-11

## Stage 3 (GUI) Integration Steps

### 1. Copy UI Files

- [ ] **Copy production HTML:**
  ```bash
  cp plugins/LushVerb/.ideas/mockups/v3-ui.html \
     plugins/LushVerb/Source/ui/public/index.html
  ```

- [ ] **Copy JUCE frontend library:**
  ```bash
  # Get juce/index.js from JUCE installation
  cp /Applications/JUCE/modules/juce_gui_extra/native/juce_WebBrowserComponent_*.js \
     plugins/LushVerb/Source/ui/public/js/juce/index.js
  ```
  *(Exact path may vary - check JUCE installation for WebView JavaScript bindings)*

- [ ] **Verify file structure:**
  ```
  plugins/LushVerb/Source/ui/public/
  ├── index.html (production UI)
  └── js/juce/index.js (JUCE bindings)
  ```

### 2. Update PluginEditor Files

- [ ] **Backup existing files** (if any):
  ```bash
  mv plugins/LushVerb/Source/PluginEditor.h plugins/LushVerb/Source/PluginEditor.h.backup
  mv plugins/LushVerb/Source/PluginEditor.cpp plugins/LushVerb/Source/PluginEditor.cpp.backup
  ```

- [ ] **Copy WebView boilerplate:**
  ```bash
  cp plugins/LushVerb/.ideas/mockups/v3-PluginEditor.h \
     plugins/LushVerb/Source/PluginEditor.h

  cp plugins/LushVerb/.ideas/mockups/v3-PluginEditor.cpp \
     plugins/LushVerb/Source/PluginEditor.cpp
  ```

- [ ] **Verify member order in PluginEditor.h:**
  - Order: `relays` → `webView` → `attachments`
  - ⚠️ Critical for release builds - incorrect order causes crashes on plugin reload

- [ ] **Verify initialization order in PluginEditor.cpp:**
  - Step 1: Create relays
  - Step 2: Create WebView with options
  - Step 3: Create attachments

### 3. Update CMakeLists.txt

- [ ] **Append WebView configuration:**
  ```bash
  cat plugins/LushVerb/.ideas/mockups/v3-CMakeLists.txt >> \
      plugins/LushVerb/CMakeLists.txt
  ```

- [ ] **Verify CMake additions:**
  - `juce_add_binary_data(${PLUGIN_NAME}_UIResources ...)` present
  - `target_link_libraries(..._UIResources)` present
  - `JUCE_WEB_BROWSER=1` definition present
  - `juce::juce_gui_extra` module linked

### 4. Build and Test (Debug)

- [ ] **Clean build directory:**
  ```bash
  rm -rf plugins/LushVerb/build
  mkdir plugins/LushVerb/build
  cd plugins/LushVerb/build
  ```

- [ ] **Configure with CMake:**
  ```bash
  cmake .. -DCMAKE_BUILD_TYPE=Debug
  ```

- [ ] **Build plugin:**
  ```bash
  cmake --build . --config Debug
  ```

- [ ] **Verify build succeeds** (no warnings about WebView resources)

- [ ] **Launch Standalone mode:**
  ```bash
  open plugins/LushVerb/build/LushVerb_artefacts/Debug/Standalone/LushVerb.app
  ```

- [ ] **WebView loads correctly** (not blank white screen)

- [ ] **Open developer tools:**
  - Right-click in plugin window → "Inspect"
  - Check Console for JavaScript errors
  - Verify `window.__JUCE__` object exists

- [ ] **Test parameter state:**
  - Open Console and run:
    ```javascript
    window.__JUCE__.backend.getSliderState("SIZE")
    window.__JUCE__.backend.getSliderState("DAMPING")
    window.__JUCE__.backend.getSliderState("SHIMMER")
    window.__JUCE__.backend.getSliderState("MIX")
    ```
  - All should return valid state objects

### 5. Test Parameter Binding (Debug)

- [ ] **Test UI → DSP direction:**
  - Move SIZE knob in UI
  - Verify audio changes (reverb tail length)
  - Check Console for parameter messages

- [ ] **Test DSP → UI direction:**
  - Automate parameter in DAW
  - Verify knob moves in UI

- [ ] **Test all 4 parameters:**
  - SIZE: 0.5s - 20s range works
  - DAMPING: 0-100% range works
  - SHIMMER: 0-100% range works
  - MIX: 0-100% range works

- [ ] **Test parameter persistence:**
  - Set all knobs to non-default values
  - Close and reopen plugin
  - Verify values persisted

### 6. Build and Test (Release)

- [ ] **Build release version:**
  ```bash
  cd plugins/LushVerb/build
  cmake .. -DCMAKE_BUILD_TYPE=Release
  cmake --build . --config Release
  ```

- [ ] **Release build succeeds** (no crashes during build)

- [ ] **Launch Release Standalone:**
  ```bash
  open plugins/LushVerb/build/LushVerb_artefacts/Release/Standalone/LushVerb.app
  ```

- [ ] **Release runs without crashing** (tests member order logic)

- [ ] **Test plugin reload 10 times:**
  - Close and reopen Standalone 10x
  - No crashes = member order correct

### 7. WebView-Specific Validation

- [ ] **Verify HTML/CSS constraints:**
  - No viewport units (100vh, 100vw, 100dvh, 100svh) in v3-ui.html
  - `html, body { height: 100%; }` present
  - `user-select: none` present (native feel)
  - Context menu disabled via JavaScript

- [ ] **Test resource provider:**
  - All resources load (no 404 in Console)
  - index.html returns correctly
  - js/juce/index.js returns correctly

- [ ] **Test relay registration:**
  - All 4 relays registered with WebView
  - `withOptionsFrom()` called for each relay
  - Parameter attachments created for each relay

### 8. DAW Validation

- [ ] **Load plugin in DAW** (Ableton Live, Logic Pro, etc.)

- [ ] **Plugin loads without errors**

- [ ] **UI renders correctly** (same as Standalone)

- [ ] **Automation works:**
  - Record automation for SIZE parameter
  - Playback moves knob in UI

- [ ] **Preset recall works:**
  - Save preset with custom values
  - Load preset
  - UI updates to match preset

- [ ] **Multiple instances:**
  - Load 3+ instances of plugin
  - Each has independent state
  - No crosstalk between instances

### 9. Final Checklist

- [ ] Debug build: ✓ No crashes
- [ ] Release build: ✓ No crashes
- [ ] Parameter binding: ✓ Bidirectional sync works
- [ ] UI rendering: ✓ Matches mockup design
- [ ] LED meter animates: ✓ (if C++ drives meter data)
- [ ] All 4 knobs functional: ✓ SIZE, DAMPING, SHIMMER, MIX
- [ ] Plugin reload stable: ✓ (10x test passed)
- [ ] DAW compatibility: ✓ (tested in primary DAW)

## Troubleshooting

### Issue: Blank WebView (white screen)

**Symptoms:** Plugin window shows white/blank screen instead of UI

**Causes:**
1. Resource provider not returning index.html
2. BinaryData not linked correctly
3. Wrong URL in goToURL() call

**Solution:**
```cpp
// Check resource provider returns non-null:
if (url == "/" || url == "/index.html") {
    return juce::WebBrowserComponent::Resource{
        BinaryData::index_html,  // Check this exists
        BinaryData::index_htmlSize,
        "text/html"
    };
}

// Check goToURL uses correct protocol:
webView->goToURL("https://juce.backend/");  // NOT "http://localhost/"
```

### Issue: Parameters don't respond

**Symptoms:** Moving knobs in UI doesn't change audio

**Causes:**
1. Relays not registered with WebView (`withOptionsFrom()` missing)
2. Attachments not created
3. Wrong parameter IDs (mismatch between UI and APVTS)

**Solution:**
```cpp
// Verify relay registration:
.withOptionsFrom(*sizeRelay)
.withOptionsFrom(*dampingRelay)
.withOptionsFrom(*shimmerRelay)
.withOptionsFrom(*mixRelay)

// Verify attachment creation:
sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *audioProcessor.apvts.getParameter("SIZE"),  // Must match APVTS ID
    *sizeRelay,
    nullptr
);
```

### Issue: Release build crashes on plugin reload

**Symptoms:** Debug build works, Release crashes when closing/reopening plugin

**Causes:**
1. Member order incorrect in PluginEditor.h
2. Initialization order incorrect in PluginEditor.cpp

**Solution:**
```cpp
// CORRECT order in PluginEditor.h:
private:
    LushVerbAudioProcessor& audioProcessor;

    // 1. Relays first
    std::unique_ptr<juce::WebSliderRelay> sizeRelay;
    std::unique_ptr<juce::WebSliderRelay> dampingRelay;
    // ...

    // 2. WebView second
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments last
    std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dampingAttachment;
    // ...

// ❌ WRONG: attachments before webView (will crash in Release)
// ❌ WRONG: webView before relays (will crash in Release)
```

### Issue: JavaScript errors in Console

**Symptoms:** Console shows errors like "window.__JUCE__ is undefined"

**Causes:**
1. JUCE frontend library (juce/index.js) not loaded
2. Wrong path in script src
3. Resource provider not returning juce/index.js

**Solution:**
```html
<!-- Verify script tag in v3-ui.html: -->
<script src="js/juce/index.js"></script>

<!-- Verify resource provider returns juce/index.js: -->
if (url == "/js/juce/index.js") {
    return juce::WebBrowserComponent::Resource{
        BinaryData::juce_index_js,
        BinaryData::juce_index_jsSize,
        "text/javascript"
    };
}
```

## Success Criteria

Integration is complete when:

- ✅ Plugin loads in both Debug and Release builds
- ✅ WebView renders UI (not blank)
- ✅ All 4 parameters respond to UI interaction
- ✅ Automation updates UI knobs
- ✅ Plugin reload is stable (10x test)
- ✅ No JavaScript errors in Console
- ✅ LED meter animates (if C++ provides data)
- ✅ DAW compatibility verified

## Next Steps After Integration

1. **Stage 4 (Validation):**
   - Run pluginval on VST3 and AU builds
   - Create factory presets
   - Generate CHANGELOG.md

2. **Installation:**
   - Use `/install-plugin LushVerb` to deploy to system folders
   - Test in real DAW sessions

3. **Documentation:**
   - Document any integration issues in troubleshooting/
   - Add to Required Reading if pattern is critical
