# AutoClip - WebView UI Integration Checklist

**Mockup Version:** v5
**Plugin:** AutoClip
**Window Size:** 300×500px (portrait, fixed)
**Parameters:** 2 (clipThreshold, soloClipped)

This checklist guides integration of the finalized UI mockup during Stage 3 (GUI implementation). The gui-agent subagent will execute these steps.

---

## Prerequisites

Before starting integration, verify:

- [ ] Stage 3 (DSP) complete - Audio processing implemented
- [ ] Stage 4 (Parameters) complete - APVTS configured with clipThreshold and soloClipped
- [ ] Build system functional - Debug builds succeed
- [ ] parameter-spec.md matches mockup (2 parameters: clipThreshold, soloClipped)

---

## Phase 1: Copy UI Files

### 1.1 Create UI Directory Structure

```bash
mkdir -p Source/ui/public/js/juce
```

### 1.2 Copy Production HTML

```bash
cp plugins/AutoClip/.ideas/mockups/v5-ui.html Source/ui/public/index.html
```

**Verify:**
- [ ] File exists: `Source/ui/public/index.html`
- [ ] File size > 5KB (not empty)
- [ ] Contains `<script src="js/juce/index.js"></script>` reference

### 1.3 Copy JUCE Frontend Library

The JUCE frontend library (`index.js`) provides the `window.__JUCE__` object for WebView communication.

**Download from JUCE WebView examples or create minimal stub:**

```bash
# Option 1: Copy from JUCE examples
cp /Applications/JUCE/examples/GUI/WebBrowserDemo/Source/ui/js/juce/index.js Source/ui/public/js/juce/

# Option 2: Create minimal stub (for testing)
cat > Source/ui/public/js/juce/index.js << 'EOF'
// JUCE WebView Frontend Library (minimal stub)
window.__JUCE__ = window.__JUCE__ || {};
window.__JUCE__.backend = {
    emitEvent: function(event, detail) {
        if (window.juce && window.juce.emitEvent) {
            window.juce.emitEvent(event, JSON.stringify(detail));
        }
    },
    addEventListener: function(event, callback) {
        document.addEventListener(event, callback);
    },
    getSliderState: function(parameterId) {
        if (window.juce && window.juce.getNativeFunction) {
            const result = window.juce.getNativeFunction('getSliderState')(parameterId);
            return result ? JSON.parse(result) : null;
        }
        return null;
    },
    getToggleButtonState: function(parameterId) {
        if (window.juce && window.juce.getNativeFunction) {
            return window.juce.getNativeFunction('getToggleButtonState')(parameterId);
        }
        return false;
    }
};
EOF
```

**Verify:**
- [ ] File exists: `Source/ui/public/js/juce/index.js`
- [ ] File defines `window.__JUCE__` object
- [ ] File exports `backend.emitEvent`, `backend.getSliderState`, `backend.getToggleButtonState`

---

## Phase 2: Update PluginEditor Files

### 2.1 Replace PluginEditor.h

```bash
# Backup existing file
cp Source/PluginEditor.h Source/PluginEditor.h.backup

# Copy template
cp plugins/AutoClip/.ideas/mockups/v5-PluginEditor-TEMPLATE.h Source/PluginEditor.h
```

**CRITICAL: Verify Member Order**

Open `Source/PluginEditor.h` and confirm this exact order in private section:

```cpp
private:
    juce::AudioProcessorValueTreeState& apvts;

    // 1. Relays (declared first)
    juce::AudioProcessorValueTreeState::SliderAttachment::Relay clipThresholdRelay;
    juce::AudioProcessorValueTreeState::ButtonAttachment::Relay soloClippedRelay;

    // 2. WebView (declared second)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. Attachments (declared third)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clipThresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloClippedAttachment;
```

**Why this matters:** Incorrect member order causes crashes in Release builds due to initialization order dependencies.

**Verify:**
- [ ] Member order: relays → webView → attachments
- [ ] Both parameters present: clipThresholdRelay, soloClippedRelay
- [ ] Both attachments present: clipThresholdAttachment, soloClippedAttachment
- [ ] Class name matches: `AutoClipAudioProcessorEditor`

### 2.2 Replace PluginEditor.cpp

```bash
# Backup existing file
cp Source/PluginEditor.cpp Source/PluginEditor.cpp.backup

# Copy template
cp plugins/AutoClip/.ideas/mockups/v5-PluginEditor-TEMPLATE.cpp Source/PluginEditor.cpp
```

**CRITICAL: Verify Initialization Order**

Open `Source/PluginEditor.cpp` and confirm constructor initialization list matches member declaration order:

```cpp
AutoClipAudioProcessorEditor::AutoClipAudioProcessorEditor (...)
    : AudioProcessorEditor (&p), apvts (vts),
      // MUST match header declaration order
      clipThresholdRelay (apvts, "clipThreshold", [this](float value) { ... }),
      soloClippedRelay (apvts, "soloClipped", [this](float value) { ... })
{
    setSize (300, 500);  // Match mockup dimensions
    // ... rest of constructor
}
```

**Verify:**
- [ ] Initialization order matches header declaration order
- [ ] `setSize(300, 500)` matches mockup dimensions
- [ ] Resource provider serves `index.html` and `index.js`
- [ ] Backend handles `sliderValueChanged`, `toggleButtonValueChanged`, `getSliderState`, `getToggleButtonState`
- [ ] Parameter IDs match APVTS: "clipThreshold", "soloClipped"

---

## Phase 3: Update CMakeLists.txt

### 3.1 Add UI Resources Binary Data

Open `CMakeLists.txt` and add this block **before** the `juce_add_plugin` call:

```cmake
# UI Resources Binary Data
juce_add_binary_data(AutoClip_UIResources SOURCES
    Source/ui/public/index.html
    Source/ui/public/js/juce/index.js
)
```

### 3.2 Link UI Resources

Find the `target_link_libraries(AutoClip ...)` section and add `AutoClip_UIResources`:

```cmake
target_link_libraries(AutoClip
    PRIVATE
        AutoClip_UIResources  # <-- ADD THIS
        juce::juce_audio_utils
        juce::juce_dsp
        # ... other libraries
)
```

### 3.3 Enable WebBrowser Component

Find the `target_compile_definitions(AutoClip ...)` section and add `JUCE_WEB_BROWSER=1`:

```cmake
target_compile_definitions(AutoClip
    PUBLIC
        JUCE_WEB_BROWSER=1  # <-- ADD THIS
        JUCE_VST3_CAN_REPLACE_VST2=0
        # ... other definitions
)
```

**Verify:**
- [ ] `juce_add_binary_data(AutoClip_UIResources ...)` present
- [ ] `AutoClip_UIResources` linked in `target_link_libraries`
- [ ] `JUCE_WEB_BROWSER=1` defined in `target_compile_definitions`

---

## Phase 4: Build and Test (Debug Mode)

### 4.1 Clean and Build

```bash
cd build
cmake --build . --config Debug --clean-first
```

**Expected output:**
- [ ] Build succeeds without errors
- [ ] No warnings about member initialization order
- [ ] Binary data resources compiled (index.html, index.js)

**If build fails:**
- Check CMakeLists.txt syntax (missing commas, parentheses)
- Verify UI files exist in `Source/ui/public/`
- Check PluginEditor includes: `#include <juce_gui_extra/juce_gui_extra.h>`

### 4.2 Test Standalone (Debug)

```bash
open build/AutoClip_artefacts/Debug/Standalone/AutoClip.app
```

**Visual checks:**
- [ ] Window opens at 300×500px (portrait)
- [ ] Background: Dark brown radial gradient
- [ ] Title: "AUTOCLIP" at top
- [ ] Large rotary knob visible (190px, Bakelite style)
- [ ] Toggle switch visible below knob
- [ ] Label: "CLIP SOLO" below toggle

### 4.3 Test WebView Inspector (Debug Only)

Right-click on plugin UI → "Inspect" (opens WebView DevTools)

**Console checks:**
- [ ] No JavaScript errors in console
- [ ] `window.__JUCE__` object exists (type in console: `window.__JUCE__`)
- [ ] Backend functions accessible:
  - `window.__JUCE__.backend.getSliderState('clipThreshold')`
  - `window.__JUCE__.backend.getToggleButtonState('soloClipped')`

**If WebView is blank:**
- Check resource provider URLs (must be `https://juce.backend/`)
- Verify `index.html` loaded (Network tab in DevTools)
- Check for CSS errors (viewport units forbidden)

### 4.4 Test Parameter Binding (Debug)

**Knob interaction:**
- [ ] Click and drag knob vertically
- [ ] Knob rotates visually (±135° range)
- [ ] Texture rotates with knob body
- [ ] Console shows parameter value updates
- [ ] Audio processing reacts to changes (if DSP implemented)

**Toggle interaction:**
- [ ] Click toggle switch
- [ ] Thumb slides left/right
- [ ] Track background changes (inactive → active color)
- [ ] Console shows boolean state changes

**Automation test (in DAW):**
- [ ] Load plugin in DAW
- [ ] Automate clipThreshold parameter
- [ ] UI knob updates during playback
- [ ] Automate soloClipped parameter
- [ ] UI toggle updates during playback

---

## Phase 5: Build and Test (Release Mode)

### 5.1 Build Release

```bash
cd build
cmake --build . --config Release
```

**Expected output:**
- [ ] Build succeeds without errors
- [ ] No crashes during compilation (member order is critical here)

**If Release build crashes:**
- **LIKELY CAUSE:** Member initialization order mismatch
- Verify header declaration order matches .cpp initialization order
- Check for any members initialized before they're declared

### 5.2 Test Release Standalone

```bash
open build/AutoClip_artefacts/Release/Standalone/AutoClip.app
```

**Stability tests:**
- [ ] Plugin launches without crashing
- [ ] UI loads correctly (same as Debug)
- [ ] Knob interaction works
- [ ] Toggle interaction works
- [ ] Close and reopen 10 times (test for reload crashes)

**Common Release build issues:**
- Blank UI → Check member order (webView initialized before relays)
- Crash on close → Check destructor order (attachments deleted after webView)
- Parameters not updating → Check attachment initialization (must be last)

---

## Phase 6: Test Parameter Persistence

### 6.1 Preset Recall Test

1. Set clipThreshold to 50%
2. Enable soloClipped toggle
3. Save preset in DAW
4. Change parameters to different values
5. Recall preset

**Verify:**
- [ ] clipThreshold returns to 50%
- [ ] soloClipped returns to enabled
- [ ] UI updates to reflect recalled values (knob rotates, toggle activates)

### 6.2 Automation Recall Test

1. Create automation for clipThreshold (0% → 100%)
2. Create automation for soloClipped (off → on → off)
3. Play back automation in DAW

**Verify:**
- [ ] Knob rotates smoothly during playback
- [ ] Toggle switches at correct timeline points
- [ ] Audio processing responds to automation

---

## Phase 7: WebView-Specific Validation

### 7.1 Check CSS Constraints

Open `Source/ui/public/index.html` and verify:

```bash
grep -E "100vh|100vw|100dvh|100svh" Source/ui/public/index.html
```

**Expected:** No matches (viewport units forbidden in JUCE WebView)

**Instead verify:**
```bash
grep "html, body.*height: 100%" Source/ui/public/index.html
```

**Expected:** Match found (required for full-screen layouts)

**Verify:**
- [ ] No viewport units (100vh, 100vw, 100dvh, 100svh)
- [ ] `html, body { height: 100%; }` present
- [ ] `user-select: none` present (native feel)
- [ ] Context menu disabled in JavaScript

### 7.2 Resource Provider Test

Check DevTools Network tab (Debug build only):

**Verify:**
- [ ] `https://juce.backend/index.html` loads (200 status)
- [ ] `https://juce.backend/js/juce/index.js` loads (200 status)
- [ ] No 404 errors for UI resources

**If 404 errors:**
- Check resource provider URL mapping in PluginEditor.cpp
- Verify binary data includes all files (CMakeLists.txt)
- Check file paths in `juce_add_binary_data`

---

## Phase 8: Cross-Platform Validation (Optional)

If building for multiple platforms:

### macOS
- [ ] Uses WKWebView (modern WebKit)
- [ ] WebView renders correctly
- [ ] No permission prompts

### Windows
- [ ] Microsoft Edge WebView2 Runtime installed
- [ ] Uses WebView2 (Chromium-based)
- [ ] WebView renders correctly

### Linux
- [ ] libwebkit2gtk-4.0-dev installed
- [ ] WebView renders correctly
- [ ] No GTK warnings

---

## Phase 9: Final Validation

### 9.1 Build Both Formats

```bash
cd build
cmake --build . --config Release --target AutoClip_VST3
cmake --build . --config Release --target AutoClip_AU
```

**Verify:**
- [ ] VST3 builds successfully
- [ ] AU builds successfully (macOS only)
- [ ] Both formats load in DAW
- [ ] UI identical in both formats

### 9.2 Install and Test in DAW

```bash
# Use build-and-install.sh for automated installation
./scripts/build-and-install.sh AutoClip
```

**Verify:**
- [ ] Plugin appears in DAW plugin list
- [ ] Plugin loads without errors
- [ ] UI displays correctly
- [ ] Parameters persist across sessions
- [ ] No crashes after 10+ reloads

---

## Success Criteria

Stage 3 (GUI) is complete when ALL of these are verified:

- ✅ Build succeeds (Debug and Release)
- ✅ UI loads in Standalone and DAW
- ✅ WebView renders correctly (no blank screen)
- ✅ Both parameters functional (knob and toggle)
- ✅ Parameter automation works (UI updates during playback)
- ✅ Preset recall works (UI updates when loading presets)
- ✅ No crashes in Release build (member order correct)
- ✅ No crashes on plugin reload (tested 10 times)
- ✅ WebView constraints validated (no viewport units, proper height)
- ✅ Resource provider serves all files (no 404 errors)

---

## Troubleshooting Reference

### Blank WebView
**Symptom:** UI loads but shows blank/black screen
**Likely causes:**
1. Viewport units in CSS (100vh, 100vw) → Replace with `height: 100%`
2. Resource provider not serving files → Check URLs and binary data
3. JavaScript error preventing render → Open DevTools console

**Fix:** Run WebView constraint validation (Phase 7.1)

### Parameter Not Updating UI
**Symptom:** Changing parameter in DAW doesn't update UI
**Likely causes:**
1. Event listener not registered → Check `backend.addEventListener` in index.html
2. Relay callback not emitting event → Check lambda in PluginEditor.cpp constructor
3. Parameter ID mismatch → Verify "clipThreshold" and "soloClipped" consistent

**Fix:** Add debug logging to relay callbacks, check DevTools console

### Crash on Close (Release Build)
**Symptom:** Plugin crashes when closing/reloading in Release mode
**Likely cause:** Member initialization order mismatch
**Fix:** Verify member declaration order in .h matches initialization order in .cpp

**Critical order:**
1. Relays (declared first, initialized first)
2. webView (declared second, initialized second)
3. Attachments (declared third, initialized third)

### UI Not Updating from Automation
**Symptom:** UI doesn't respond to automation playback
**Likely causes:**
1. Attachments not created → Check constructor creates clipThresholdAttachment and soloClippedAttachment
2. Relay not attached → Verify `SliderAttachment` and `ButtonAttachment` constructed with relay reference

**Fix:** Ensure attachments created AFTER relays and webView in constructor

---

## Next Steps After Integration

After completing this checklist:

1. **Test in DAW** - Load in Logic Pro / Ableton / Reaper
2. **Run pluginval** - Validate with `pluginval --strictness-level 10`
3. **Update PLUGINS.md** - Mark Stage 3 complete
4. **Commit changes** - `git commit -m "feat(AutoClip): Stage 3 complete - WebView UI integrated"`

---

**Integration Template Version:** v5
**Created:** 2025-11-13
**Plugin:** AutoClip
**Mockup:** v5 (Vintage Bakelite, 300×500px portrait)
