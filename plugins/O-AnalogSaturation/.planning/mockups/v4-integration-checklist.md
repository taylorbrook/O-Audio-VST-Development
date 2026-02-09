# Stage 3 (GUI) Integration Checklist - v4

**Plugin:** OuariconSaturationModeling
**Mockup Version:** v4
**Generated:** 2026-01-09
**Target:** WebView-based UI with botanical naturalist aesthetic

---

## Prerequisites

Before starting Stage 3 (GUI) integration, verify:

- [ ] Stage 1 (Foundation) complete - Plugin builds successfully
- [ ] Stage 2 (DSP) complete - Parameters added to APVTS in correct order
- [ ] parameter-spec.md exists and matches v4 mockup (4 parameters: INTENSITY, MODEL, QUALITY, AUTOGAIN)
- [ ] v4-ui.html exists (production HTML)
- [ ] v4-PluginEditor-TEMPLATE.h exists (C++ header reference)
- [ ] v4-PluginEditor-TEMPLATE.cpp exists (C++ implementation reference)
- [ ] v4-CMakeLists-SNIPPET.txt exists (CMake configuration)

---

## Phase 1: Prepare Image Assets

### Copy Snake Images (4 files)

Source: `/Users/taylorbrook/Dev/Ouaricon Audio Images/fauna/`

Copy to: `plugins/OuariconSaturationModeling/Source/ui/public/img/`

```bash
mkdir -p Source/ui/public/img
cp "/Users/taylorbrook/Dev/Ouaricon Audio Images/fauna/snake_mobot31753000317195_0068.png" \
   Source/ui/public/img/
cp "/Users/taylorbrook/Dev/Ouaricon Audio Images/fauna/snake_mobot31753000317195_0070.png" \
   Source/ui/public/img/
cp "/Users/taylorbrook/Dev/Ouaricon Audio Images/fauna/snake_NA_0145.png" \
   Source/ui/public/img/
cp "/Users/taylorbrook/Dev/Ouaricon Audio Images/fauna/snake_snakesaustralia00kref_0145.png" \
   Source/ui/public/img/
```

- [ ] snake_mobot31753000317195_0068.png copied (MAGNETIC model)
- [ ] snake_mobot31753000317195_0070.png copied (TUBE model)
- [ ] snake_NA_0145.png copied (TRANSFORMER model)
- [ ] snake_snakesaustralia00kref_0145.png copied (DIODE model)

### Copy Paper Background (1 file)

Source: `/Users/taylorbrook/Dev/Ouaricon Audio Images/paper/`

Copy to: `plugins/OuariconSaturationModeling/Source/ui/public/img/`

```bash
cp "/Users/taylorbrook/Dev/Ouaricon Audio Images/paper/paper1.jpg" \
   Source/ui/public/img/
```

- [ ] paper1.jpg copied (background texture)

### Verify All Images Present

```bash
ls -lh Source/ui/public/img/
# Should show 5 files: 1 JPG + 4 PNG
```

- [ ] All 5 image files exist in Source/ui/public/img/

---

## Phase 2: Copy UI Files

### Copy Production HTML

Source: `.ideas/mockups/v4-ui.html`

Destination: `Source/ui/public/index.html`

```bash
cp .ideas/mockups/v4-ui.html Source/ui/public/index.html
```

- [ ] index.html copied to Source/ui/public/
- [ ] Verify file contains ES6 module imports (`type="module"`)
- [ ] Verify no viewport units (`100vh`, `100vw`) in CSS

### Copy JUCE Frontend Library

Source: Working plugin with WebView (e.g., GainKnob, TapeAge, FlutterVerb)

Destination: `Source/ui/public/js/juce/`

```bash
mkdir -p Source/ui/public/js/juce
cp /path/to/reference/plugin/Source/ui/public/js/juce/index.js \
   Source/ui/public/js/juce/
cp /path/to/reference/plugin/Source/ui/public/js/juce/check_native_interop.js \
   Source/ui/public/js/juce/
```

- [ ] index.js copied to Source/ui/public/js/juce/
- [ ] check_native_interop.js copied to Source/ui/public/js/juce/
- [ ] Both files are JUCE 8 compatible (ES6 exports)

---

## Phase 3: Update PluginEditor Files

### Update PluginEditor.h

Reference: `.ideas/mockups/v4-PluginEditor-TEMPLATE.h`

**CRITICAL: Member declaration order (JUCE 8 critical pattern #11)**

```cpp
private:
    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> intensityRelay;
    std::unique_ptr<juce::WebSliderRelay> modelRelay;
    std::unique_ptr<juce::WebSliderRelay> qualityRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> autogainRelay;

    // 2. WEBVIEW SECOND (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS LAST (depend on relays AND webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> intensityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> modelAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> qualityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> autogainAttachment;
```

- [ ] Relays declared BEFORE webView
- [ ] webView declared BEFORE attachments
- [ ] All 4 parameters have matching relay + attachment
- [ ] Timer inheritance added: `private juce::Timer`
- [ ] timerCallback() override declared
- [ ] getResource() method declared

### Update PluginEditor.cpp

Reference: `.ideas/mockups/v4-PluginEditor-TEMPLATE.cpp`

**Initialization order (matches declaration order):**

```cpp
// 1. Create relays FIRST
intensityRelay = std::make_unique<juce::WebSliderRelay>("INTENSITY");
modelRelay = std::make_unique<juce::WebSliderRelay>("MODEL");
qualityRelay = std::make_unique<juce::WebSliderRelay>("QUALITY");
autogainRelay = std::make_unique<juce::WebToggleButtonRelay>("AUTOGAIN");

// 2. Create WebView with relay options
webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        .withOptionsFrom(*intensityRelay)
        .withOptionsFrom(*modelRelay)
        .withOptionsFrom(*qualityRelay)
        .withOptionsFrom(*autogainRelay)
);

// 3. Create attachments LAST (JUCE 8 requires 3 parameters)
intensityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *audioProcessor.parameters.getParameter("INTENSITY"), *intensityRelay, nullptr);
// ... repeat for MODEL, QUALITY, AUTOGAIN
```

**CRITICAL: Third parameter (nullptr) required for JUCE 8 (critical pattern #12)**

- [ ] Constructor initializes relays first
- [ ] WebView created with `.withOptionsFrom()` for all relays
- [ ] Attachments use 3-parameter constructor (parameter, relay, nullptr)
- [ ] webView->goToURL(getResourceProviderRoot()) called
- [ ] setSize(600, 450) matches v4 window dimensions
- [ ] startTimerHz(60) called for VU meter updates
- [ ] stopTimer() called in destructor

### Implement Resource Provider

Reference: v4-PluginEditor-TEMPLATE.cpp lines ~150-230

**Use EXPLICIT URL MAPPING (JUCE 8 critical pattern #8)**

- [ ] getResource() uses explicit if-else chain (not generic loop)
- [ ] Returns index.html for "/" and "/index.html"
- [ ] Returns index.js with MIME type "application/javascript"
- [ ] Returns check_native_interop.js with correct MIME type
- [ ] Returns paper1.jpg with MIME type "image/jpeg"
- [ ] Returns all 4 snake PNGs with MIME type "image/png"
- [ ] BinaryData symbol names match juce_add_binary_data() (underscores, not dots)

### Implement VU Meter Updates

- [ ] timerCallback() implemented
- [ ] Calls calculateInputLevel() and calculateOutputLevel()
- [ ] Sends data via evaluateJavascript: `window.receiveMeterLevels(inputDB, outputDB)`
- [ ] Placeholder implementations return dummy values (real DSP added later)

---

## Phase 4: Update CMakeLists.txt

Reference: `.ideas/mockups/v4-CMakeLists-SNIPPET.txt`

### Add Binary Data

```cmake
juce_add_binary_data(OuariconSaturationModeling_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper1.jpg
        Source/ui/public/img/snake_mobot31753000317195_0068.png
        Source/ui/public/img/snake_mobot31753000317195_0070.png
        Source/ui/public/img/snake_NA_0145.png
        Source/ui/public/img/snake_snakesaustralia00kref_0145.png
)
```

- [ ] juce_add_binary_data() includes all UI files
- [ ] File paths match actual file locations
- [ ] All 5 image files listed

### Link UI Resources

```cmake
target_link_libraries(OuariconSaturationModeling
    PRIVATE
        OuariconSaturationModeling_UIResources
        juce::juce_gui_extra  # Required for WebBrowserComponent
)
```

- [ ] UIResources binary data linked
- [ ] juce::juce_gui_extra linked

### Add WebView Definitions

```cmake
target_compile_definitions(OuariconSaturationModeling
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
```

- [ ] JUCE_WEB_BROWSER=1 defined
- [ ] JUCE_USE_CURL=0 defined

### Update juce_add_plugin

**CRITICAL (JUCE 8 critical pattern #9):**

```cmake
juce_add_plugin(OuariconSaturationModeling
    # ... existing options ...
    NEEDS_WEB_BROWSER TRUE  # <-- ADD THIS LINE
)
```

- [ ] NEEDS_WEB_BROWSER TRUE added to juce_add_plugin()
- [ ] Required for VST3 format to load in DAWs

---

## Phase 5: Build and Test (Debug)

### Build Debug Configuration

```bash
cmake --build build --config Debug --target OuariconSaturationModeling
```

- [ ] Build succeeds without errors
- [ ] No warnings about missing symbols
- [ ] BinaryData.h generated in build artifacts

### Test Standalone (Debug)

```bash
open build/OuariconSaturationModeling_artefacts/Debug/Standalone/OuariconSaturationModeling.app
```

- [ ] Standalone loads without crash
- [ ] WebView displays UI (not blank white screen)
- [ ] Paper background visible
- [ ] Snake overlay visible (MAGNETIC by default)
- [ ] Right-click → Inspect opens developer tools
- [ ] Console shows no JavaScript errors
- [ ] Console shows: "Ouaricon Saturation v4 - Production UI loaded"
- [ ] window.__JUCE__ object exists in console

### Test Parameter Binding (Debug)

- [ ] Intensity knob rotates on drag (relative drag, not absolute)
- [ ] Model buttons switch active state
- [ ] Snake image changes when MODEL button clicked
- [ ] Snake opacity changes when intensity knob dragged
- [ ] Quality buttons switch active state
- [ ] Autogain toggle switches on/off
- [ ] All parameter changes persist after closing/reopening UI

---

## Phase 6: Build and Test (Release)

### Build Release Configuration

```bash
cmake --build build --config Release --target OuariconSaturationModeling
```

- [ ] Release build succeeds without errors
- [ ] No warnings

### Test Standalone (Release)

```bash
open build/OuariconSaturationModeling_artefacts/Release/Standalone/OuariconSaturationModeling.app
```

- [ ] Standalone loads without crash
- [ ] UI displays correctly (same as Debug)
- [ ] No blank white screen
- [ ] All parameters interactive

### Release Crash Test (Member Order Verification)

**Reload plugin 10 times to verify member order correctness:**

```bash
# Open standalone, close it, repeat 10 times
for i in {1..10}; do
    echo "Test $i/10"
    open build/.../Release/Standalone/OuariconSaturationModeling.app
    sleep 2
    killall OuariconSaturationModeling
    sleep 1
done
```

- [ ] No crashes during 10 reload cycles
- [ ] Proves member order is correct (relays → webView → attachments)

---

## Phase 7: Install and Test in DAW

### Install Plugin

```bash
./scripts/build-and-install.sh OuariconSaturationModeling
```

- [ ] Script copies VST3 to ~/Library/Audio/Plug-Ins/VST3/
- [ ] Script copies AU to ~/Library/Audio/Plug-Ins/Components/
- [ ] Script signs both formats with codesign
- [ ] Script clears DAW caches
- [ ] No errors during installation

### Test in Ableton Live

- [ ] Restart Ableton Live
- [ ] VST3 appears in browser (Audio Effects)
- [ ] AU appears in browser (Audio Units)
- [ ] Drag plugin to track
- [ ] UI loads correctly
- [ ] All parameters work
- [ ] Automation works (move knob → record automation → playback updates UI)

### Test in Logic Pro (if macOS)

- [ ] Restart Logic Pro
- [ ] AU appears in Audio FX browser
- [ ] Insert on track
- [ ] UI loads correctly
- [ ] All parameters work

---

## Phase 8: WebView-Specific Validation

### CSS Validation

- [ ] No viewport units in CSS (`100vh`, `100vw`, `100dvh`, `100svh`)
- [ ] html, body { height: 100%; } present
- [ ] user-select: none present (native feel)
- [ ] No horizontal scrollbars
- [ ] No vertical scrollbars

### Resource Loading Validation

Open developer tools (right-click → Inspect) and check:

- [ ] Network tab shows all resources loaded (200 OK)
- [ ] No 404 errors for images, JS, or HTML
- [ ] All MIME types correct (application/javascript for .js, image/png for .png)

### JavaScript Validation

- [ ] Console shows no errors
- [ ] window.__JUCE__ object exists
- [ ] getSliderState('INTENSITY') returns state object
- [ ] getToggleState('AUTOGAIN') returns state object
- [ ] All 4 parameters accessible via JUCE API

---

## Phase 9: VU Meter Integration

**Note:** VU meters display dummy data until DSP stage implements level tracking.

### Verify Animation Loop

- [ ] VU meter needles move (even with dummy data)
- [ ] Needles use ballistic motion (fast attack, slow decay)
- [ ] Animation smooth at 60fps (requestAnimationFrame loop)
- [ ] No jitter or freezing

### Prepare for DSP Integration

VU meters require processor to track levels. Add to PluginProcessor during DSP stage:

```cpp
// In PluginProcessor.h
std::atomic<float> inputRMS{0.0f};
std::atomic<float> outputRMS{0.0f};

// In processBlock()
inputRMS.store(calculateRMS(inputBuffer));
outputRMS.store(calculateRMS(outputBuffer));
```

Then update PluginEditor.cpp:

```cpp
float calculateInputLevel() {
    return juce::Decibels::gainToDecibels(audioProcessor.inputRMS.load());
}

float calculateOutputLevel() {
    return juce::Decibels::gainToDecibels(audioProcessor.outputRMS.load());
}
```

- [ ] Placeholder for DSP integration noted
- [ ] Will be implemented during DSP refinement

---

## Phase 10: Final Validation

### Parameter Count Verification

From parameter-spec.md: 4 parameters total

- [ ] INTENSITY (float, WebSliderRelay)
- [ ] MODEL (choice, WebSliderRelay)
- [ ] QUALITY (choice, WebSliderRelay)
- [ ] AUTOGAIN (bool, WebToggleButtonRelay)
- [ ] Total: 4 relays, 4 attachments, 1 webView

### Member Count Verification

Count members in PluginEditor.h:

```bash
grep -c "Relay;" Source/PluginEditor.h  # Should be 4
grep -c "Attachment;" Source/PluginEditor.h  # Should be 4
grep -c "WebBrowserComponent" Source/PluginEditor.h  # Should be 1
```

- [ ] 4 relays declared
- [ ] 4 attachments declared
- [ ] 1 webView declared

### Initialization Order Verification

Manually inspect PluginEditor.cpp constructor:

- [ ] Relays created first (4 std::make_unique calls)
- [ ] WebView created second (with .withOptionsFrom for all 4 relays)
- [ ] Attachments created last (4 std::make_unique calls with 3 parameters each)

---

## Troubleshooting Reference

If issues occur, consult JUCE 8 critical patterns:

- **Blank white screen:** Check resource provider returns all files (pattern #8)
- **Frozen knobs:** Verify 3-parameter attachment constructor (pattern #12)
- **VST3 not loading:** Add NEEDS_WEB_BROWSER TRUE (pattern #9)
- **Release crashes:** Verify member order (pattern #11)
- **Knobs jump to cursor:** Use relative drag (pattern #16)
- **VU meters jerky:** Use requestAnimationFrame loop (pattern #20)
- **ES6 import errors:** Add type="module" to script tags (pattern #21)

---

## Completion Criteria

Stage 3 (GUI) is complete when:

- [ ] All 4 parameters bind correctly UI ↔ APVTS
- [ ] Snake image switches per MODEL selection
- [ ] Snake opacity changes with INTENSITY
- [ ] VU meters animate (even with placeholder data)
- [ ] No crashes in debug or release builds
- [ ] No 404 errors in resource loading
- [ ] Plugin loads in Ableton and Logic
- [ ] Automation updates UI correctly
- [ ] Preset recall updates UI correctly
- [ ] All checkboxes in this document are checked

---

**Next Stage:** Stage 4 (DSP) - Implement saturation algorithms and VU meter level tracking

