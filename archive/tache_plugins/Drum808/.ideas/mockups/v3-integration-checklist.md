# Stage 5 (GUI) Integration Checklist - Drum808 v3

**Plugin:** Drum808
**Mockup Version:** v3
**Generated:** 2025-11-13
**Parameters:** 24 drum parameters + 3 preset buttons = 27 total

## Prerequisites

- [ ] Stage 3 (Shell) complete - plugin shell exists
- [ ] Stage 4 (DSP) complete - audio processing implemented
- [ ] parameter-spec.md finalized (24 parameters)
- [ ] UI design approved (v3 mockup tested in browser)

## Step 1: Copy UI Files

```bash
# Create UI directory structure
mkdir -p plugins/Drum808/Source/ui/public/js/juce

# Copy production HTML
cp plugins/Drum808/.ideas/mockups/v3-ui.html \
   plugins/Drum808/Source/ui/public/index.html

# Copy JUCE frontend library (from existing plugin or JUCE examples)
cp [source]/index.js \
   plugins/Drum808/Source/ui/public/js/juce/index.js

cp [source]/check_native_interop.js \
   plugins/Drum808/Source/ui/public/js/juce/check_native_interop.js
```

**Verify:**
- [ ] `Source/ui/public/index.html` exists
- [ ] `Source/ui/public/js/juce/index.js` exists (JUCE frontend library)
- [ ] `Source/ui/public/js/juce/check_native_interop.js` exists (REQUIRED)
- [ ] All files have correct content (not empty)

## Step 2: Update PluginEditor.h

**Replace existing PluginEditor.h with v3-PluginEditor-TEMPLATE.h content:**

```bash
# Backup current file
cp plugins/Drum808/Source/PluginEditor.h \
   plugins/Drum808/Source/PluginEditor.h.backup

# Apply template (manually merge if needed)
# IMPORTANT: Adjust class name from Drum808AudioProcessorEditor
#            to match your processor class name
```

**Key changes:**
- [ ] Add 27 relay declarations (24 params + 3 preset buttons)
- [ ] Add WebBrowserComponent declaration
- [ ] Add 27 attachment declarations
- [ ] Verify member order: relays → webView → attachments
- [ ] Add `getResource()` method declaration
- [ ] Class name matches processor (e.g., `Drum808AudioProcessorEditor`)

**Critical member order verification:**
```cpp
private:
    // 1️⃣ RELAYS FIRST (27 total)
    std::unique_ptr<juce::WebSliderRelay> kickLevelRelay;
    // ... (all 27 relays)

    // 2️⃣ WEBVIEW SECOND
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (27 total)
    std::unique_ptr<juce::WebSliderParameterAttachment> kickLevelAttachment;
    // ... (all 27 attachments)
};
```

## Step 3: Update PluginEditor.cpp

**Replace existing PluginEditor.cpp with v3-PluginEditor-TEMPLATE.cpp content:**

```bash
# Backup current file
cp plugins/Drum808/Source/PluginEditor.cpp \
   plugins/Drum808/Source/PluginEditor.cpp.backup

# Apply template (manually merge if needed)
```

**Key changes:**
- [ ] Constructor creates 27 relays
- [ ] WebView options register all 27 relays
- [ ] Constructor creates 27 attachments (with nullptr for undo manager)
- [ ] Verify initialization order matches declaration order
- [ ] `getResource()` method serves index.html and JUCE JS files
- [ ] Window size set to 1000x550 (from YAML)

**Initialization order verification:**
```cpp
Drum808AudioProcessorEditor::Drum808AudioProcessorEditor(Drum808AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // 1. Create relays FIRST
    kickLevelRelay = std::make_unique<juce::WebSliderRelay>("kick_level");
    // ... (all 27 relays)

    // 2. Create WebView with relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*kickLevelRelay)
            // ... (all 27 .withOptionsFrom calls)
    );

    // 3. Create attachments LAST
    kickLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("kick_level"), *kickLevelRelay, nullptr);
    // ... (all 27 attachments)
}
```

## Step 4: Update CMakeLists.txt

**Append v3-CMakeLists-SNIPPET.txt to plugins/Drum808/CMakeLists.txt:**

1. **Add NEEDS_WEB_BROWSER to juce_add_plugin():**
   ```cmake
   juce_add_plugin(Drum808
       # ... existing options ...
       NEEDS_WEB_BROWSER TRUE  # <-- Add this line
   )
   ```

2. **Add binary data target (after juce_add_plugin):**
   ```cmake
   juce_add_binary_data(Drum808_UIResources
       SOURCES
           Source/ui/public/index.html
           Source/ui/public/js/juce/index.js
           Source/ui/public/js/juce/check_native_interop.js
   )
   ```

3. **Link UI resources and juce_gui_extra:**
   ```cmake
   target_link_libraries(Drum808
       PRIVATE
           Drum808_UIResources
           juce::juce_gui_extra  # REQUIRED for WebBrowserComponent
       # ... existing libraries ...
   )
   ```

4. **Add WebView definitions:**
   ```cmake
   target_compile_definitions(Drum808
       PUBLIC
           JUCE_WEB_BROWSER=1
           JUCE_USE_CURL=0
       # ... existing definitions ...
   )
   ```

**Verify:**
- [ ] `NEEDS_WEB_BROWSER TRUE` in juce_add_plugin()
- [ ] `juce_add_binary_data(Drum808_UIResources ...)` present
- [ ] All 3 UI files listed in SOURCES
- [ ] `juce::juce_gui_extra` linked
- [ ] `JUCE_WEB_BROWSER=1` defined

## Step 5: Build (Debug)

```bash
# Configure CMake
cd plugins/Drum808
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --parallel
```

**Verify:**
- [ ] Build succeeds without errors
- [ ] No warnings about WebView or binary data
- [ ] No missing symbol errors (JuceHeader.h, WebBrowserComponent)

**Common build issues:**
- Missing `juce_generate_juce_header(Drum808)` if using `<JuceHeader.h>`
- Missing `juce::juce_gui_extra` causes undefined WebBrowserComponent
- Missing `NEEDS_WEB_BROWSER TRUE` causes silent VST3 loading failure

## Step 6: Test in Standalone (Debug)

```bash
# Run standalone app
./build/Drum808_artefacts/Debug/Standalone/Drum808.app/Contents/MacOS/Drum808
```

**Verify UI loads:**
- [ ] Plugin window opens (1000x550 size)
- [ ] WebView displays (not blank)
- [ ] All 6 drum channel strips visible
- [ ] All 24 knobs render correctly
- [ ] Preset browser buttons visible at top
- [ ] Master meter visible on right
- [ ] No console errors (check right-click → Inspect if available)

**Test parameter binding:**
- [ ] Drag kick level knob - value updates
- [ ] Drag all 24 knobs - all respond
- [ ] Parameter changes reflect in display values
- [ ] Knobs return to correct position after release

## Step 7: Test Parameter Binding

**C++ → HTML (automation, preset recall):**
- [ ] Open standalone, change parameter externally (automation)
- [ ] UI updates to reflect new value
- [ ] All 24 parameters sync correctly

**HTML → C++ (user interaction):**
- [ ] Drag kick level knob
- [ ] Audio output changes (kick gets louder/quieter)
- [ ] DSP receives parameter changes
- [ ] All 24 parameters control audio correctly

**Preset browser buttons:**
- [ ] Click PREV button - triggers preset change
- [ ] Click NEXT button - triggers preset change
- [ ] Click SAVE button - triggers preset save

## Step 8: Install and Test in DAW

```bash
# Build and install (uses build-and-install.sh script)
./scripts/build-and-install.sh Drum808
```

**Script performs:**
- Builds Debug and Release configurations
- Removes old versions from system folders
- Installs VST3 and AU to `~/Library/Audio/Plug-Ins/`
- Signs plugins (macOS code signing)
- Clears DAW plugin caches

**Verify installation:**
- [ ] VST3 exists: `~/Library/Audio/Plug-Ins/VST3/Drum808.vst3`
- [ ] AU exists: `~/Library/Audio/Plug-Ins/Components/Drum808.component`
- [ ] Both formats signed correctly
- [ ] Cache cleared (Ableton PluginDatabase.cfg removed)

**Test in DAW:**
- [ ] Restart DAW (to rescan plugins)
- [ ] Plugin appears in plugin list (both VST3 and AU)
- [ ] Plugin loads without crashes
- [ ] UI displays correctly in plugin window
- [ ] All 24 knobs functional
- [ ] Parameter automation works
- [ ] Preset recall works
- [ ] Project save/load preserves state

## Step 9: Build and Test (Release)

```bash
# Build release configuration
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Install release build
./scripts/build-and-install.sh Drum808
```

**Release-specific tests:**
- [ ] Release build succeeds
- [ ] No crashes on plugin load (test 10 times)
- [ ] No crashes on plugin reload
- [ ] UI loads correctly in release
- [ ] All parameters work in release
- [ ] Member order correct (no release-only crashes)

**Why test release separately:**
- Debug builds hide member order bugs
- Release optimization exposes destructor issues
- 90% of WebView crashes only occur in release builds

## Step 10: Verify WebView-Specific Patterns

**CSS constraints (in index.html):**
- [ ] `html, body { height: 100%; }` (NOT `100vh`)
- [ ] `user-select: none` present
- [ ] No viewport units (`100vh`, `100vw`, `100dvh`)

**JavaScript patterns:**
- [ ] `type="module"` on script tags
- [ ] ES6 imports: `import { getSliderState } from ...`
- [ ] Context menu disabled
- [ ] Relative drag for knobs (frame-to-frame delta)

**C++ patterns:**
- [ ] Member order: relays → webView → attachments
- [ ] All attachments have third parameter (nullptr for undo manager)
- [ ] Resource provider returns correct MIME types
- [ ] `check_native_interop.js` served by resource provider

## Troubleshooting

### WebView blank on load

**Check:**
```cpp
// Verify goToURL called in constructor
webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
```

**Solution:** Ensure resource provider returns index.html for "/" URL.

### Knobs don't rotate

**Check JavaScript console (if accessible):**
- Parameter IDs match between HTML and C++ exactly (case-sensitive)
- `getSliderState()` returns valid state object
- ES6 module imports working (`type="module"` present)

**Solution:** Verify parameter IDs in HTML match `apvts.getParameter()` calls.

### Parameters don't persist

**Check:**
- APVTS parameter tree includes all 24 parameters
- Parameter IDs in C++ match YAML specification exactly
- Attachments created with correct parameter pointers

**Solution:** Verify parameter-spec.md parameter IDs match APVTS and relay IDs.

### VST3 doesn't appear in DAW

**Check:**
- `NEEDS_WEB_BROWSER TRUE` in CMakeLists.txt
- VST3 binary exists in build artifacts
- Code signing valid: `codesign --verify --deep build/.../Drum808.vst3`

**Solution:** Add `NEEDS_WEB_BROWSER TRUE` flag and rebuild.

### Release build crashes

**Check member order in PluginEditor.h:**
```cpp
// Correct order (top to bottom):
// 1. Relays (27 declarations)
// 2. WebView (1 declaration)
// 3. Attachments (27 declarations)
```

**Solution:** Reorder members to match template exactly.

## Parameter List (24 drum parameters)

**Kick drum (4):**
- kick_level, kick_tone, kick_decay, kick_tuning

**Low tom (4):**
- lowtom_level, lowtom_tone, lowtom_decay, lowtom_tuning

**Mid tom (4):**
- midtom_level, midtom_tone, midtom_decay, midtom_tuning

**Clap (4):**
- clap_level, clap_tone, clap_snap, clap_tuning

**Closed hat (4):**
- closedhat_level, closedhat_tone, closedhat_decay, closedhat_tuning

**Open hat (4):**
- openhat_level, openhat_tone, openhat_decay, openhat_tuning

**Preset browser (3 buttons):**
- preset_previous, preset_next, preset_save

## Success Criteria

UI integration complete when:

- ✅ Plugin builds without errors (debug and release)
- ✅ WebView loads in standalone app
- ✅ All 24 knobs visible and functional
- ✅ All parameters sync correctly (C++ ↔ HTML)
- ✅ Plugin loads in DAW (both VST3 and AU)
- ✅ Parameter automation works
- ✅ Preset recall works
- ✅ No crashes on reload (tested 10 times)
- ✅ Release build stable (member order correct)

## Next Steps

After successful integration:

1. Continue to Stage 6 (Validation) - run pluginval tests
2. Manual DAW testing across multiple DAWs
3. Test with real MIDI input (drum pads, keyboard, sequencer)
4. Verify individual drum outputs work correctly
5. Test preset browser functionality
6. Install for production use (`/install-plugin Drum808`)
