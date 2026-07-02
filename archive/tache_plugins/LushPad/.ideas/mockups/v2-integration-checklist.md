# Stage 5 (GUI) Integration Checklist - v2

**Plugin:** LushPad
**Mockup Version:** v2
**Generated:** 2025-11-13
**Parameters:** 3 (timbre, filter_cutoff, reverb_amount)

## 1. Copy UI Files

```bash
# Create UI directory structure
mkdir -p plugins/LushPad/Source/ui/public/js/juce

# Copy production HTML
cp plugins/LushPad/.ideas/mockups/v2-ui.html \
   plugins/LushPad/Source/ui/public/index.html

# Copy JUCE frontend library (from working plugin or JUCE examples)
# Example: Copy from GainKnob reference implementation
cp plugins/GainKnob/Source/ui/public/js/juce/index.js \
   plugins/LushPad/Source/ui/public/js/juce/index.js

# CRITICAL (Pattern #13): Copy check_native_interop.js
cp plugins/GainKnob/Source/ui/public/js/juce/check_native_interop.js \
   plugins/LushPad/Source/ui/public/js/juce/check_native_interop.js
```

**Verify:**

- [ ] `Source/ui/public/index.html` exists
- [ ] `Source/ui/public/js/juce/index.js` exists
- [ ] `Source/ui/public/js/juce/check_native_interop.js` exists (REQUIRED)
- [ ] HTML contains production UI code (not browser test with `.plugin-frame`)

## 2. Update PluginEditor Files

```bash
# Replace PluginEditor.h with template content
cp plugins/LushPad/.ideas/mockups/v2-PluginEditor-TEMPLATE.h \
   plugins/LushPad/Source/PluginEditor.h

# Replace PluginEditor.cpp with template content
cp plugins/LushPad/.ideas/mockups/v2-PluginEditor-TEMPLATE.cpp \
   plugins/LushPad/Source/PluginEditor.cpp
```

**Verify:**

- [ ] Member order correct in .h: relays → webView → attachments
- [ ] Class name is `LushPadAudioProcessorEditor` (matches PluginProcessor.h)
- [ ] 3 relay declarations (timbre, filter_cutoff, reverb_amount)
- [ ] 3 attachment declarations (matching relays)
- [ ] Initialization order in .cpp matches declaration order
- [ ] Window size is 600×300 (from v2-ui.yaml)

## 3. Update CMakeLists.txt

**CRITICAL: Update juce_add_plugin() declaration first:**

```cmake
juce_add_plugin(LushPad
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Ycom
    PLUGIN_CODE Lush
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "LushPad"
    NEEDS_WEB_BROWSER TRUE  # Pattern #9: REQUIRED for VST3 WebView
)
```

**Then append WebView configuration from v2-CMakeLists-SNIPPET.txt:**

- [ ] `juce_add_binary_data(LushPad_UIResources ...)` added
- [ ] Binary data includes `index.html`, `index.js`, `check_native_interop.js`
- [ ] `juce::juce_gui_extra` linked
- [ ] `JUCE_WEB_BROWSER=1` definition present
- [ ] `NEEDS_WEB_BROWSER TRUE` in juce_add_plugin()

## 4. Build and Test (Debug)

```bash
# Generate build files
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build plugin
cmake --build build --parallel

# Check for errors
echo $?  # Should be 0 (success)
```

**Verify:**

- [ ] Build succeeds without warnings
- [ ] No "JuceHeader.h not found" errors (Pattern #1: juce_generate_juce_header)
- [ ] No WebView-related errors
- [ ] Binary data embedded correctly

## 5. Test Standalone (Debug)

```bash
# Run standalone app
open build/LushPad_artefacts/Debug/Standalone/LushPad.app
```

**Check UI loads:**

- [ ] Plugin window opens (600×300 size)
- [ ] WebView loads (not blank)
- [ ] Right-click → Inspect works (verify browser console available)
- [ ] Console shows "LushPad UI initialized"
- [ ] No JavaScript errors in console

**Check parameter binding:**

- [ ] `window.__JUCE__` object exists in console
- [ ] All 3 knobs visible (Timbre, Filter Cutoff, Reverb Amount)
- [ ] Knobs rotate when dragged (Pattern #16: relative drag)
- [ ] Rotation applies to PARENT .knob element (Pattern from v2 fixes)
- [ ] No "Failed to get slider state" errors

## 6. Test Parameter Synchronization

**Test UI → APVTS:**

- [ ] Drag Timbre knob → value changes in audio processing
- [ ] Drag Filter Cutoff knob → value changes (with skew factor 0.3)
- [ ] Drag Reverb Amount knob → value changes
- [ ] Double-click knob → resets to default value

**Test APVTS → UI (automation):**

- [ ] Load preset → UI updates to reflect preset values
- [ ] DAW automation → UI follows parameter changes
- [ ] Pattern #15: valueChangedEvent callback has NO parameters

## 7. Build and Test (Release)

```bash
# Build release
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Test release build
open build/LushPad_artefacts/Release/Standalone/LushPad.app
```

**Verify no crashes:**

- [ ] Plugin loads without crash
- [ ] Reload plugin 10 times → no crashes (tests member order)
- [ ] Pattern #11: Member order prevents release build crashes
- [ ] All knobs functional in release build

## 8. Install and Test in DAW

```bash
# Install to system folders (Pattern #10: ALWAYS install for GUI testing)
./scripts/build-and-install.sh LushPad

# Verify installation
ls -la ~/Library/Audio/Plug-Ins/VST3/LushPad.vst3
ls -la ~/Library/Audio/Plug-Ins/Components/LushPad.component

# Clear DAW caches (REQUIRED)
killall -9 AudioComponentRegistrar
rm ~/Library/Preferences/Ableton/Live*/PluginDatabase.cfg 2>/dev/null

# Restart DAW
```

**Test in DAW:**

- [ ] Plugin appears in DAW plugin list (both VST3 and AU)
- [ ] Plugin loads without crash
- [ ] UI displays correctly (600×300 window)
- [ ] All 3 knobs functional
- [ ] Parameter automation works
- [ ] Preset recall works
- [ ] Values persist after closing/reopening project

## 9. WebView-Specific Validation

**CSS constraints (Pattern from ui-design-rules.md):**

- [ ] No viewport units in CSS (`100vh`, `100vw`)
- [ ] Uses `html, body { height: 100%; }` instead
- [ ] `user-select: none` present (native feel)
- [ ] Context menu disabled in JavaScript

**Resource provider (Pattern #8):**

- [ ] All resources return correct MIME types
- [ ] `index.html` → `text/html`
- [ ] `index.js` → `application/javascript` (NOT `text/javascript`)
- [ ] `check_native_interop.js` → `application/javascript`
- [ ] No 404 errors in console

**ES6 module loading (Pattern #21):**

- [ ] Script tag has `type="module"` attribute
- [ ] Imports use `import { getSliderState } from ...`
- [ ] NOT accessing via `window.__JUCE__.backend.getSliderState()`

## Parameter List (from parameter-spec.md)

| Parameter | Type | Range | Default | Relay Type | Attachment Type |
|-----------|------|-------|---------|------------|-----------------|
| timbre | Float | 0.0 - 1.0 | 0.35 | WebSliderRelay | WebSliderParameterAttachment |
| filter_cutoff | Float | 20 - 20000 Hz | 2000 | WebSliderRelay | WebSliderParameterAttachment |
| reverb_amount | Float | 0.0 - 1.0 | 0.4 | WebSliderRelay | WebSliderParameterAttachment |

**Total:** 3 relays, 3 attachments

## Troubleshooting

### Knobs frozen (don't respond to drag)

**Pattern #12:** Verify JUCE 8 attachment constructor has 3 parameters:

```cpp
timbreAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *audioProcessor.parameters.getParameter("timbre"),
    *timbreRelay,
    nullptr  // <-- Must be present (undoManager parameter)
);
```

**Pattern #21:** Verify ES6 module import:

```html
<script type="module" src="js/juce/index.js"></script>
<script type="module">
    import { getSliderState } from './js/juce/index.js';
    // ...
</script>
```

### Knobs visible but don't rotate

**Pattern #16:** Verify relative drag (frame-delta pattern):

```javascript
let lastY = 0;  // NOT startY

knob.addEventListener("mousedown", (e) => {
    isDragging = true;
    lastY = e.clientY;  // Store CURRENT position
});

document.addEventListener("mousemove", (e) => {
    if (!isDragging) return;

    const deltaY = lastY - e.clientY;  // Distance since LAST FRAME
    rotation += deltaY * sensitivity;  // INCREMENT, not replace

    lastY = e.clientY;  // Update for next frame
});
```

### Knobs don't update from automation

**Pattern #15:** Verify valueChangedEvent callback reads value:

```javascript
sliderState.valueChangedEvent.addListener(() => {
    // NO parameters passed to callback!
    const value = sliderState.getNormalisedValue();  // Read from state
    updateKnob(value);
});
```

### VST3 doesn't appear in DAW

**Pattern #9:** Verify NEEDS_WEB_BROWSER in CMakeLists.txt:

```cmake
juce_add_plugin(LushPad
    # ... other settings
    NEEDS_WEB_BROWSER TRUE  # REQUIRED for VST3
)
```

### Testing stale builds

**Pattern #10:** ALWAYS install to system folders for GUI testing:

```bash
# Build AND install (not --no-install)
./scripts/build-and-install.sh LushPad

# Clear caches
killall -9 AudioComponentRegistrar
rm ~/Library/Preferences/Ableton/Live*/PluginDatabase.cfg

# Restart DAW
```

## Success Criteria

Stage 5 (GUI) integration is complete when:

- ✅ Plugin builds without errors (Debug and Release)
- ✅ UI loads in standalone app
- ✅ All 3 knobs visible and functional
- ✅ Parameter synchronization works (UI ↔ APVTS bidirectional)
- ✅ Plugin works in DAW (VST3 and AU)
- ✅ Automation and preset recall work
- ✅ No crashes on reload (tests member order)
- ✅ All critical patterns verified (see juce8-critical-patterns.md)

## Next Steps

After successful GUI integration:

1. Proceed to Stage 4 (DSP implementation)
2. Or if DSP already complete, proceed to validation
3. Run pluginval tests
4. Manual DAW testing
5. Install for production use
