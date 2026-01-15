# Stage 3 (GUI) Integration Checklist - OuariconPolystutter v5

**Plugin:** OuariconPolystutter
**Mockup Version:** v5 (TAPE KNOBS HORIZONTAL FIX)
**Generated:** 2026-01-14
**Total Parameters:** 70 (56 lane params + 6 tape params + 4 global params + 64 pattern steps)

---

## Overview

This checklist guides integration of the finalized v5 UI mockup into the OuariconPolystutter plugin during Stage 3 (GUI implementation). The v5 design features:

- 4 independent repeater lanes with 14 parameters each (56 total)
- 6 tape degradation controls (horizontal row layout)
- 4 global toggles (envelope, sidechain, MIDI, manual trigger)
- 64-step pattern sequencer (4 lanes × 16 steps)
- Paper texture background with botanical overlay
- Fixed 1000×750px window

---

## Pre-Integration Verification

Before starting, verify these files exist:

- [ ] `v5-ui.html` - Production HTML (clean, no debug elements)
- [ ] `v5-PluginEditor.h` - C++ header with correct member order
- [ ] `v5-PluginEditor.cpp` - C++ implementation with 70 parameter bindings
- [ ] `v5-CMakeLists.txt` - WebView CMake configuration snippet
- [ ] `parameter-spec.md` - Complete parameter specification (70 parameters)

---

## 1. Copy UI Files to Source Tree

**Target directory:** `Source/ui/public/`

### Step 1.1: Create directory structure

```bash
cd plugins/OuariconPolystutter
mkdir -p Source/ui/public/js/juce
mkdir -p Source/ui/public/img
```

### Step 1.2: Copy production HTML

```bash
cp .ideas/mockups/v5-ui.html Source/ui/public/index.html
```

**Verify:**
- [ ] No debug monitor HTML
- [ ] No console.log statements
- [ ] ES6 module imports present (`type="module"`)
- [ ] All 70 parameters have `data-param` attributes

### Step 1.3: Copy JUCE WebView library

```bash
# Copy from working plugin (e.g., Ouaricon Digital Delay)
cp ../OuariconDigitalDelay/Source/ui/public/js/juce/index.js Source/ui/public/js/juce/
cp ../OuariconDigitalDelay/Source/ui/public/js/juce/check_native_interop.js Source/ui/public/js/juce/
```

**Verify:**
- [ ] `index.js` exports `getSliderState`, `getToggleState`, `getComboBoxState`
- [ ] `check_native_interop.js` exists (prevents frozen UI)

### Step 1.4: Copy images

```bash
# Copy paper background and botanical overlay
cp path/to/paper-background.jpg Source/ui/public/img/
cp path/to/botanical-bug.png Source/ui/public/img/
```

**Verify:**
- [ ] `paper-background.jpg` exists (used as body background)
- [ ] `botanical-bug.png` exists (decorative overlay, 32% opacity)

---

## 2. Update PluginEditor Files

### Step 2.1: Read existing PluginEditor.h

```bash
cat Source/PluginEditor.h
```

**Purpose:** Check current implementation before replacing.

### Step 2.2: Replace with v5-PluginEditor.h content

**CRITICAL: Verify member order before copying!**

```cpp
// CORRECT ORDER (from v5-PluginEditor.h):
private:
    // 1. Reference to processor
    OuariconPolystutterAudioProcessor& audioProcessor;

    // 2. RELAYS FIRST (70 relays)
    std::unique_ptr<juce::WebSliderRelay> lane1RepeatsRelay;
    // ... (all 70 relay declarations)

    // 3. WEBVIEW SECOND
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 4. ATTACHMENTS LAST (70 attachments)
    std::unique_ptr<juce::WebSliderParameterAttachment> lane1RepeatsAttachment;
    // ... (all 70 attachment declarations)
```

**Checklist:**
- [ ] Member order is relays → webView → attachments
- [ ] Relay count = 70 (count with `grep -c "Relay;" v5-PluginEditor.h`)
- [ ] Attachment count = 70 (count with `grep -c "Attachment;" v5-PluginEditor.h`)
- [ ] Class name is `OuariconPolystutterAudioProcessorEditor`
- [ ] Includes `<juce_gui_extra/juce_gui_extra.h>`

### Step 2.3: Replace with v5-PluginEditor.cpp content

**CRITICAL: Match initialization order to declaration order!**

```cpp
// Constructor initializer list (from v5-PluginEditor.cpp):
OuariconPolystutterAudioProcessorEditor::OuariconPolystutterAudioProcessorEditor(...)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // 1. Create relays FIRST (same order as declarations)
    lane1EnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("lane1_enabled");
    // ... (70 relay creations)

    // 2. Create WebView with 70 .withOptionsFrom() calls
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*lane1EnabledRelay)
            // ... (70 withOptionsFrom calls)
    );

    // 3. Create attachments LAST (JUCE 8 requires 3 parameters)
    auto& apvts = audioProcessor.getAPVTS();
    lane1EnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("lane1_enabled"), *lane1EnabledRelay, nullptr);
    // ... (70 attachment creations)
}
```

**Checklist:**
- [ ] Relay creation order matches declaration order
- [ ] WebView has 70 `.withOptionsFrom()` calls
- [ ] Attachment creation uses 3-parameter constructor (see pattern #12)
- [ ] Resource provider has explicit URL mapping (see pattern #8)
- [ ] `setSize(1000, 750)` matches YAML dimensions

---

## 3. Update CMakeLists.txt

### Step 3.1: Read current CMakeLists.txt

```bash
cat CMakeLists.txt
```

### Step 3.2: Modify juce_add_plugin() to include NEEDS_WEB_BROWSER

**Locate this block:**
```cmake
juce_add_plugin(OuariconPolystutter
    COMPANY_NAME "Ouaricon Audio"
    # ... existing settings
)
```

**Add this flag:**
```cmake
juce_add_plugin(OuariconPolystutter
    COMPANY_NAME "Ouaricon Audio"
    PLUGIN_MANUFACTURER_CODE Ouar
    PLUGIN_CODE PoRe
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Ouaricon Polystutter"
    NEEDS_WEB_BROWSER TRUE    # REQUIRED for VST3 WebView
    IS_SYNTH FALSE            # Effect plugin (has audio input)
)
```

**Verify:**
- [ ] `NEEDS_WEB_BROWSER TRUE` present (pattern #9)
- [ ] `IS_SYNTH FALSE` correct for effect plugin

### Step 3.3: Append v5-CMakeLists.txt snippet

**Add after `juce_add_plugin()` block:**

```cmake
# Paste entire contents of v5-CMakeLists.txt here

# WebView UI Resources
juce_add_binary_data(${PROJECT_NAME}_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/img/paper-background.jpg
        Source/ui/public/img/botanical-bug.png
)

# Link UI resources and JUCE modules
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}_UIResources
        juce::juce_gui_extra  # REQUIRED for WebBrowserComponent
        # ... (rest of snippet)
)

# Generate JuceHeader.h (JUCE 8 requirement)
juce_generate_juce_header(${PROJECT_NAME})

# WebView compile definitions
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        # ... (rest of snippet)
)
```

**Verify:**
- [ ] `juce_add_binary_data` includes all 5 UI resources
- [ ] `target_link_libraries` includes `juce::juce_gui_extra`
- [ ] `juce_generate_juce_header` called AFTER `target_link_libraries` (pattern #1)
- [ ] `JUCE_WEB_BROWSER=1` defined (pattern #3)

---

## 4. Build and Test (Debug)

### Step 4.1: Clean build directory

```bash
rm -rf build
mkdir build
cd build
```

### Step 4.2: Configure CMake

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

**Expected output:**
- No warnings about missing JUCE modules
- BinaryData target created with 5 files

### Step 4.3: Build plugin

```bash
cmake --build . --config Debug
```

**Expected:**
- [ ] Build succeeds without errors
- [ ] No WebView-related warnings
- [ ] BinaryData.h generated with `index_html`, `index_js`, etc.

### Step 4.4: Install to system

```bash
# Use build-and-install script
cd ../../../
./scripts/build-and-install.sh OuariconPolystutter
```

**Verify installation:**
```bash
ls -la ~/Library/Audio/Plug-Ins/VST3/ | grep "Ouaricon Polystutter"
ls -la ~/Library/Audio/Plug-Ins/Components/ | grep "Ouaricon Polystutter"
```

**Checklist:**
- [ ] VST3 installed to `~/Library/Audio/Plug-Ins/VST3/`
- [ ] AU installed to `~/Library/Audio/Plug-Ins/Components/`
- [ ] Both formats code-signed (verified with `codesign --verify`)

### Step 4.5: Test in DAW

**Launch Ableton/Logic and load plugin**

**Checklist:**
- [ ] Plugin appears in VST3 browser (if not, check `NEEDS_WEB_BROWSER` flag)
- [ ] Plugin appears in AU browser
- [ ] Standalone launches successfully

**Test WebView loading:**
- [ ] UI displays (not blank white screen)
- [ ] Paper background visible
- [ ] Botanical overlay visible (top-right, 32% opacity)
- [ ] All 4 lane sections visible
- [ ] Pattern sequencer grid visible (64 step buttons)
- [ ] Tape degradation knobs in HORIZONTAL row (v5 fix verified)

**Test developer tools:**
- [ ] Right-click → "Inspect" opens DevTools
- [ ] Console tab shows no JavaScript errors
- [ ] `window.__JUCE__` object exists
- [ ] `window.__JUCE__.backend` has `getSliderState`, `getToggleState`, `getComboBoxState`

---

## 5. Build and Test (Release)

### Step 5.1: Build release version

```bash
cd plugins/OuariconPolystutter/build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Step 5.2: Install release build

```bash
cd ../../../
./scripts/build-and-install.sh OuariconPolystutter --release
```

### Step 5.3: Test member order (crash test)

**Purpose:** Verify correct destruction order prevents release build crashes

**Test procedure:**
1. Load plugin in DAW (release build)
2. Remove plugin from track
3. Re-add plugin to track
4. Repeat 10 times

**Expected result:**
- [ ] NO crashes on plugin reload
- [ ] DAW remains stable

**If crashes occur:**
- Member order is WRONG
- Check PluginEditor.h declaration order
- Must be: relays → webView → attachments

---

## 6. Test Parameter Binding (70 Parameters)

### Lane Parameters (56 total = 4 lanes × 14 params)

**For each lane (1-4), test:**

**Sliders (9 per lane):**
- [ ] `lane[N]_repeats` - Drag knob up/down, value updates
- [ ] `lane[N]_decay` - Knob rotation matches parameter value
- [ ] `lane[N]_pitch` - Automation updates knob position
- [ ] `lane[N]_filter` - Preset recall updates UI
- [ ] `lane[N]_probability` - Values persist after reload
- [ ] `lane[N]_volume`
- [ ] `lane[N]_pan`
- [ ] `lane[N]_swing`

**Toggles (4 per lane):**
- [ ] `lane[N]_pingpong` - Click toggles state, visual updates
- [ ] `lane[N]_reverse` - Automation toggles button
- [ ] `lane[N]_manual_time_enabled` - State persists
- [ ] `lane[N]_freeze`

**ComboBox (1 per lane):**
- [ ] `lane[N]_subdivision` - Click cycles through choices ["1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T"]
- [ ] Selected value displayed correctly
- [ ] Automation updates selection

**Lane Enable (1 per lane):**
- [ ] `lane[N]_enabled` - Toggle button in header
- [ ] Background color changes (gray → green)

### Tape Degradation Parameters (6 total)

**All knobs in horizontal row (v5 layout fix):**
- [ ] `tape_saturation` - Leftmost knob
- [ ] `tape_wow` - 2nd knob
- [ ] `tape_flutter` - 3rd knob
- [ ] `tape_hiss` - 4th knob
- [ ] `tape_rolloff` - 5th knob
- [ ] `tape_dropout` - Rightmost knob

**Verify:**
- [ ] All 6 knobs visible in single horizontal row (NOT stacked)
- [ ] Knobs evenly spaced
- [ ] No visual overlap

### Global Parameters (4 total)

**Footer toggles:**
- [ ] `envelope_enabled` - ENV button
- [ ] `sidechain_enabled` - SC button
- [ ] `midi_enabled` - MIDI button
- [ ] `manual_trigger` - TRIG button (momentary behavior)

### Pattern Sequencer (64 step toggles)

**Test pattern for lane 1:**
- [ ] Click step 1 → Toggles active/inactive
- [ ] Click step 16 → Visual state changes
- [ ] Automation can toggle individual steps
- [ ] Pattern state persists on preset recall

**Verify grid layout:**
- [ ] 4 rows (L1, L2, L3, L4 labels visible)
- [ ] 16 columns per row
- [ ] Step buttons are 22px height (compact)
- [ ] Grid fits within 130px section height

---

## 7. WebView-Specific Validation

### CSS Constraints

**Verify NO viewport units:**
```bash
grep -E "(100vh|100vw|100dvh|100svh)" Source/ui/public/index.html
```

**Expected:** No matches (pattern #16)

**Verify html/body height:**
```css
html, body {
  height: 100%;  /* REQUIRED */
}
```

**Verify native application feel:**
```css
body {
  user-select: none;  /* REQUIRED */
  -webkit-user-select: none;
}
```

### Resource Provider

**Test all resources load:**

Open browser DevTools → Network tab:
- [ ] `index.html` (200 OK, text/html)
- [ ] `js/juce/index.js` (200 OK, application/javascript)
- [ ] `js/juce/check_native_interop.js` (200 OK, application/javascript)
- [ ] `img/paper-background.jpg` (200 OK, image/jpeg)
- [ ] `img/botanical-bug.png` (200 OK, image/png)

**If any 404 errors:**
- Check `getResource()` URL mapping in PluginEditor.cpp
- Verify BinaryData variable names match
- Confirm MIME types are correct

### Knob Interaction

**Test relative drag (pattern #16):**
1. Click and hold knob at 12 o'clock position
2. Drag mouse down slowly → Knob rotates counterclockwise
3. Drag mouse up slowly → Knob rotates clockwise
4. Release mouse → Value persists

**Expected behavior:**
- [ ] Knob rotation is INCREMENTAL (not absolute cursor position)
- [ ] Can drag infinitely without hitting limits
- [ ] Smooth rotation (no jumps)

**If knob jumps to cursor position:**
- Check JavaScript uses `lastY` (frame delta), not `startY` (absolute)
- Verify `rotation += deltaY` (increment), not `rotation = startRotation + deltaY`

---

## 8. Performance and Optimization

### CPU Usage

**Test idle CPU (no audio playing):**
- [ ] Plugin uses <1% CPU when idle
- [ ] No unnecessary redraws

**Test with automation:**
- [ ] Smooth parameter changes (no jitter)
- [ ] UI updates at 60fps (check with DevTools Performance tab)

### Memory Usage

**Check for leaks:**
1. Load plugin
2. Adjust all 70 parameters
3. Remove plugin
4. Check Activity Monitor → Memory

**Expected:**
- [ ] Memory released after plugin removal
- [ ] No memory leaks reported by JUCE LeakDetector

---

## 9. Cross-Format Verification

### VST3

**Test in Ableton Live:**
- [ ] Plugin loads without errors
- [ ] UI displays correctly (1000×750px)
- [ ] All parameters automatable
- [ ] State saves/recalls correctly

### AU

**Test in Logic Pro:**
- [ ] Plugin loads without errors
- [ ] UI displays correctly
- [ ] All parameters visible in automation menu
- [ ] State saves/recalls correctly

### Standalone

**Launch standalone app:**
```bash
open "build/OuariconPolystutter_artefacts/Release/Standalone/Ouaricon Polystutter.app"
```

**Verify:**
- [ ] Window opens at 1000×750px
- [ ] UI fully functional
- [ ] Audio input/output working

---

## 10. Final Verification

### Parameter Count Audit

**Total expected:** 70 parameters

**Breakdown:**
- Lane 1: 14 parameters
- Lane 2: 14 parameters
- Lane 3: 14 parameters
- Lane 4: 14 parameters
- Tape degradation: 6 parameters
- Global: 4 parameters
- Pattern sequencer: 64 parameters (4 lanes × 16 steps)

**Verify in PluginProcessor.cpp:**
```bash
grep -c "addParameter" Source/PluginProcessor.cpp
```

**Expected:** 70 (or 134 if pattern steps counted individually)

**Verify in PluginEditor.h:**
```bash
grep -c "Relay;" Source/PluginEditor.h
# Expected: 70

grep -c "Attachment;" Source/PluginEditor.h
# Expected: 70
```

### UI Completeness

**Visual inspection:**
- [ ] Title: "Ouaricon Polystutter" (centered, top)
- [ ] 4 lane sections (250px each, side-by-side)
- [ ] Pattern sequencer (130px height, ALWAYS VISIBLE)
- [ ] Tape degradation section (6 knobs in horizontal row)
- [ ] Footer toggles (4 buttons)
- [ ] Botanical overlay (top-right, decorative)

### Code Quality

**Check for common mistakes:**
- [ ] No `100vh` or `100vw` in CSS
- [ ] No missing `nullptr` in attachments (JUCE 8 3-param constructor)
- [ ] No `--deep` flag in codesign commands
- [ ] Resource provider returns correct MIME types

---

## Integration Complete

When ALL checklists pass:

- [ ] UI matches v5 mockup design exactly
- [ ] All 70 parameters bind correctly
- [ ] No crashes in debug or release builds
- [ ] Tape knobs display in horizontal row (v5 fix verified)
- [ ] VST3, AU, and Standalone formats work
- [ ] Ready for Stage 4 (Testing)

**Next steps:**
1. Tag release: `v5-ui-integrated`
2. Update documentation with screenshot
3. Proceed to Stage 4 (Testing)

---

## Troubleshooting

### UI is blank (white screen)

**Possible causes:**
1. Missing `JUCE_WEB_BROWSER=1` definition
2. Resource provider returning `std::nullopt` for all requests
3. Missing `check_native_interop.js`

**Solution:**
- Check CMakeLists.txt has `JUCE_WEB_BROWSER=1`
- Add logging to `getResource()` to see which URLs are requested
- Verify BinaryData includes all 5 files

### Knobs frozen (don't respond to drag)

**Possible causes:**
1. Missing `type="module"` on script tags (pattern #21)
2. Missing `nullptr` in attachment constructor (pattern #12)
3. Missing `check_native_interop.js` (pattern #13)

**Solution:**
- Verify `<script type="module">` in index.html
- Check attachment creation uses 3 parameters
- Confirm `check_native_interop.js` in BinaryData

### Release build crashes on reload

**Cause:** Member order violation

**Solution:**
- Verify PluginEditor.h declaration order: relays → webView → attachments
- Members are destroyed in REVERSE order
- Attachments MUST be destroyed before webView (see pattern #11)

### Tape knobs stacked vertically (v5 bug)

**Cause:** Missing `flex-direction: row` in CSS

**Solution:**
```css
.tape-knobs-row {
  display: flex;
  flex-direction: row;  /* HORIZONTAL */
}
```

---

**Generated by:** ui-finalization-agent
**Reference:** troubleshooting/patterns/juce8-critical-patterns.md
