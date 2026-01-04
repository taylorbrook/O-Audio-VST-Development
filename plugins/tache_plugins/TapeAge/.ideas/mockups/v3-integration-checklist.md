# WebView UI Integration Checklist - v3

**Plugin:** TapeAge
**UI Version:** v3 (Vintage Hardware Aesthetic)
**Date:** 2025-11-10

## Prerequisites

- [ ] UI design approved (tested in browser: v3-ui-test.html)
- [ ] parameter-spec.md finalized (will be generated during Stage 0-1)
- [ ] Plugin shell exists (Stage 3 complete)

## Stage 5 (GUI) Integration Steps

### 1. Copy UI Files

**Files to copy during Stage 5:**

```bash
# Create UI directory structure
mkdir -p plugins/TapeAge/Source/ui/public/js/juce

# Copy production HTML
cp plugins/TapeAge/.ideas/mockups/v3-ui.html \
   plugins/TapeAge/Source/ui/public/index.html

# Copy JUCE frontend library (from JUCE installation)
# gui-agent will handle this during Stage 5
```

**Verify:**

- [ ] HTML file exists at `Source/ui/public/index.html`
- [ ] JUCE frontend library at `Source/ui/public/js/juce/index.js`
- [ ] File contains production UI code (not browser test)

### 2. Update PluginEditor Files

**Replace existing PluginEditor.h with v3-PluginEditor.h:**

```bash
cp plugins/TapeAge/.ideas/mockups/v3-PluginEditor.h \
   plugins/TapeAge/Source/PluginEditor.h
```

**Critical checks:**

- [ ] Member order: relays → webView → attachments
- [ ] Three relay declarations (drive, age, mix)
- [ ] Three attachment declarations (drive, age, mix)
- [ ] Timer inheritance for VU meter updates

**Replace existing PluginEditor.cpp with v3-PluginEditor.cpp:**

```bash
cp plugins/TapeAge/.ideas/mockups/v3-PluginEditor.cpp \
   plugins/TapeAge/Source/PluginEditor.cpp
```

**Critical checks:**

- [ ] Initialization order matches member order
- [ ] Relays created BEFORE WebView
- [ ] Attachments created AFTER WebView
- [ ] Window size: 500x450 (matches mockup)
- [ ] Timer started for VU meter updates

### 3. Update CMakeLists.txt

**Append v3-CMakeLists.txt snippet to plugin CMakeLists.txt:**

```bash
# Add WebView configuration to bottom of plugins/TapeAge/CMakeLists.txt
cat plugins/TapeAge/.ideas/mockups/v3-CMakeLists.txt >> plugins/TapeAge/CMakeLists.txt
```

**Verify:**

- [ ] `juce_add_binary_data(TapeAge_UIResources ...)` present
- [ ] `index.html` and `js/juce/index.js` listed in SOURCES
- [ ] `JUCE_WEB_BROWSER=1` definition present
- [ ] `juce::juce_gui_extra` linked
- [ ] Platform-specific config (macOS/Windows/Linux)

### 4. Build and Test (Debug)

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel
```

**Verify:**

- [ ] Build succeeds without errors
- [ ] No WebView-related warnings
- [ ] Binary data embedded correctly
- [ ] UI resources found in build output

### 5. Test in Standalone (Debug)

```bash
# Run standalone app
./build/plugins/TapeAge/TapeAge_artefacts/Debug/Standalone/TapeAge.app/Contents/MacOS/TapeAge
```

**Visual checks:**

- [ ] Plugin window opens (500x450)
- [ ] Vintage hardware aesthetic loads
- [ ] VU meter visible with needle
- [ ] "TAPE AGE" title visible
- [ ] Three knobs visible (DRIVE, AGE, MIX)
- [ ] Dark brown radial gradient background
- [ ] Gold/bronze accents present

**Interaction checks:**

- [ ] Knobs respond to mouse drag
- [ ] Knob indicators rotate smoothly
- [ ] VU meter needle animates (if timer implemented)
- [ ] Right-click inspect works (dev tools)
- [ ] No JavaScript errors in console

**Parameter binding checks:**

- [ ] window.__JUCE__ object exists (check in console)
- [ ] `Juce.getSliderState("drive")` returns state object
- [ ] `Juce.getSliderState("age")` returns state object
- [ ] `Juce.getSliderState("mix")` returns state object
- [ ] Moving knob logs parameter change

### 6. Build and Test (Release)

**CRITICAL: Test release build to verify member order logic**

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
```

**Verify:**

- [ ] Release build succeeds without warnings
- [ ] Release build runs (tests member order logic)
- [ ] No crashes on plugin reload (test 10 times)
- [ ] Performance matches debug build

### 7. Test Parameter Binding

**In DAW or standalone:**

- [ ] Moving UI knob changes audio (verify in DAW)
- [ ] Changing parameter in DAW updates UI knob
- [ ] Parameter values persist after plugin reload
- [ ] All three parameters sync independently
- [ ] Automation works (record and playback)
- [ ] Preset recall updates UI correctly

### 8. WebView-Specific Validation

**Check constraints from ui-design-rules.md:**

- [ ] CSS does NOT use viewport units (100vh, 100vw)
- [ ] html, body { height: 100%; } present
- [ ] Native feel CSS present (user-select: none)
- [ ] Context menu disabled in JavaScript
- [ ] No blank screen on first load
- [ ] Resource provider returns all files (no 404)

**Test member order (release build only):**

- [ ] Close plugin 10 times → no crashes
- [ ] Reload plugin in DAW → no crashes
- [ ] Switch between plugins → no crashes

### 9. Install and Test in DAW

```bash
# Use plugin-lifecycle skill
/install-plugin TapeAge
```

**DAW testing (Logic, Ableton, Reaper):**

- [ ] Plugin appears in DAW plugin list
- [ ] Plugin loads without crashes
- [ ] UI displays correctly in plugin window
- [ ] Resizing DAW plugin window works
- [ ] All controls functional
- [ ] Parameter automation works
- [ ] Preset save/recall works
- [ ] Project save/load preserves state

## Troubleshooting

### WebView shows blank screen

**Possible causes:**

1. **Viewport units used**: Check CSS for `100vh`, `100vw`, `100dvh`, `100svh`
   - Solution: Replace with `height: 100%` on html/body
2. **Resource provider 404**: Check console for missing resources
   - Solution: Verify `juce_add_binary_data` lists all files
3. **Member order wrong**: Release build crashes
   - Solution: Verify relays → webView → attachments order

### Knobs don't respond

**Check:**

- [ ] Parameter IDs match APVTS: "drive", "age", "mix"
- [ ] Relays created with correct IDs
- [ ] Attachments link correct parameters
- [ ] JavaScript event listeners attached

### VU meter doesn't animate

**Check:**

- [ ] Timer started in constructor: `startTimerHz(16)`
- [ ] `timerCallback()` implemented
- [ ] Audio level data available from processor
- [ ] `emitEventIfBrowserIsVisible("meterUpdate", ...)` called

### Release build crashes

**CRITICAL: Member order violation**

- [ ] Check PluginEditor.h member order
- [ ] Relays MUST be declared BEFORE webView
- [ ] Attachments MUST be declared AFTER webView
- [ ] See v3-PluginEditor.h for correct order

## Success Criteria

UI integration is complete when:

- ✅ Plugin builds in Debug and Release without errors
- ✅ UI loads correctly in standalone app
- ✅ All three knobs visible and functional
- ✅ VU meter displays (animation optional)
- ✅ Parameters update correctly (C++ ↔ JavaScript)
- ✅ Plugin works in DAW (VST3 + AU)
- ✅ Automation and preset recall work
- ✅ No crashes on reload (10x test)

## Next Steps

After successful integration:

1. **Stage 6 (Validation)**: Run pluginval tests
2. **Manual DAW testing**: Test in Logic, Ableton, Reaper
3. **Install for production**: `/install-plugin TapeAge`
4. **Document issues**: Use `/doc-fix` if problems arise

## Notes

- **v3 aesthetic**: Vintage hardware with dark bronze, deep browns, heavy shadows
- **Window size**: 500x450 (fixed, not resizable)
- **Parameters**: drive, age, mix (all sliders 0-1)
- **VU meter**: Optional real-time visualization (requires timer)
- **Member order**: CRITICAL for release build stability
