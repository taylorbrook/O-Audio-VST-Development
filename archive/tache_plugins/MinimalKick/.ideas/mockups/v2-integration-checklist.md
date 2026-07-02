# MinimalKick - Stage 5 (GUI) Integration Checklist

**Generated from:** UI Mockup v2
**Window size:** 730×280px
**Parameters:** 5 (sweep, time, attack, decay, drive)
**Aesthetic:** vintage-hardware-001

---

## Prerequisites

- [ ] Stage 4 (DSP) complete - All audio processing implemented
- [ ] JUCE frontend library available at `Source/ui/public/js/juce/index.js`
- [ ] CMakeLists.txt exists with juce_add_plugin() call

---

## Step 1: Copy UI Files

- [ ] Create directory: `Source/ui/public/`
- [ ] Copy `v2-ui.html` → `Source/ui/public/index.html`
- [ ] Copy JUCE frontend library → `Source/ui/public/js/juce/index.js`
- [ ] Verify file paths are correct (case-sensitive)

---

## Step 2: Update PluginEditor Files

### PluginEditor.h

- [ ] Replace existing PluginEditor.h with `v2-PluginEditor-TEMPLATE.h`
- [ ] Verify member order: relays → webView → attachments (CRITICAL for release builds)
- [ ] Check all 5 parameter IDs match APVTS: sweep, time, attack, decay, drive
- [ ] Verify include guards and namespace

### PluginEditor.cpp

- [ ] Replace existing PluginEditor.cpp with `v2-PluginEditor-TEMPLATE.cpp`
- [ ] Verify window size: setSize(730, 280)
- [ ] Check all 5 relay integrations: addWebSliderListener() calls
- [ ] Verify resource provider mappings:
  - `https://juce.backend/index.html` → BinaryData::indexhtml
  - `https://juce.backend/js/juce/index.js` → BinaryData::indexjs
- [ ] Verify initialization order matches member order

---

## Step 3: Update CMakeLists.txt

- [ ] Append `v2-CMakeLists-SNIPPET.txt` content to CMakeLists.txt
- [ ] Add juce_add_binary_data() for UI resources
- [ ] Link MinimalKick_UIResources to plugin target
- [ ] Add JUCE_WEB_BROWSER=1 compile definition
- [ ] (Optional) Add platform-specific WebView config for cross-platform builds

---

## Step 4: Build and Test (Debug)

- [ ] Clean build directory: `rm -rf build/`
- [ ] Configure: `cmake -B build -DCMAKE_BUILD_TYPE=Debug`
- [ ] Build: `cmake --build build --config Debug`
- [ ] Build succeeds without warnings
- [ ] Standalone loads WebView (not blank screen)
- [ ] Right-click → Inspect works (DevTools available)
- [ ] Browser console shows no JavaScript errors
- [ ] Check window.__JUCE__ object exists in console
- [ ] Verify parameter state objects: Juce.getSliderState("sweep"), etc.

---

## Step 5: Build and Test (Release)

**CRITICAL: Release build tests member order logic**

- [ ] Build Release: `cmake --build build --config Release`
- [ ] Release build succeeds without warnings
- [ ] Release build runs (no crashes on load)
- [ ] Test plugin reload 10 times (close/reopen in DAW)
- [ ] No crashes on plugin close
- [ ] All parameters still work in Release

**If Release crashes:**
- Check member order in PluginEditor.h (relays → webView → attachments)
- Verify initialization order in PluginEditor.cpp constructor
- See troubleshooting/patterns/juce8-critical-patterns.md Pattern #15

---

## Step 6: Test Parameter Binding

- [ ] Move sweep knob in UI → verify audio changes
- [ ] Move time knob in UI → verify audio changes
- [ ] Move attack knob in UI → verify audio changes
- [ ] Move decay knob in UI → verify audio changes
- [ ] Move drive knob in UI → verify audio changes
- [ ] Automation: Automate parameter in DAW → verify UI updates
- [ ] Preset recall: Load preset → verify UI reflects preset values
- [ ] All 5 parameters sync bidirectionally (UI ↔ DSP)

---

## Step 7: WebView-Specific Validation

### CSS Validation

- [ ] No viewport units used (100vh, 100vw, 100dvh, 100svh)
- [ ] html, body { height: 100%; } present
- [ ] user-select: none present (native feel)
- [ ] Context menu disabled in JavaScript

### Resource Provider Validation

- [ ] All UI resources load (no 404 in DevTools console)
- [ ] JavaScript module imports work (ES6 modules)
- [ ] JUCE frontend library loads successfully

### Parameter Binding Validation

- [ ] All 5 relays registered with WebView
- [ ] All 5 attachments created correctly
- [ ] Parameter IDs match between APVTS, relays, and JavaScript
- [ ] valueChangedEvent listeners fire correctly

---

## Step 8: DAW Testing

- [ ] Load plugin in DAW (Logic Pro, Ableton, etc.)
- [ ] UI displays correctly (not blank, not misaligned)
- [ ] All 5 knobs respond to mouse drag
- [ ] Automation works (record/playback parameter changes)
- [ ] Preset recall works (save/load plugin state)
- [ ] Plugin survives DAW project close/reopen
- [ ] No crashes during normal operation

---

## Step 9: Final Verification

- [ ] Debug build: All tests passed
- [ ] Release build: All tests passed
- [ ] No console errors in DevTools
- [ ] No C++ warnings during compilation
- [ ] All 5 parameters functional in both builds
- [ ] Plugin ready for Stage 6 (Validation)

---

## Common Issues and Solutions

### Blank WebView on load
- **Cause:** Viewport units (100vh, 100vw) in CSS
- **Fix:** Replace with `html, body { height: 100%; }`

### Release build crashes on load
- **Cause:** Wrong member order in PluginEditor.h
- **Fix:** Ensure order is relays → webView → attachments

### Parameters don't update UI
- **Cause:** valueChangedEvent listener not registered
- **Fix:** Check sweepState.valueChangedEvent.addListener() calls in JavaScript

### 404 errors in DevTools console
- **Cause:** Resource provider not mapping URL correctly
- **Fix:** Verify URL matches resource provider string exactly

### JavaScript module import errors
- **Cause:** Missing `type="module"` in script tag
- **Fix:** Ensure `<script type="module">` in index.html

---

## Reference Documentation

- JUCE WebView patterns: troubleshooting/patterns/juce8-critical-patterns.md
- Member order logic: Pattern #15 (WebSliderRelay)
- Parameter binding: Pattern #16 (SliderAttachment)
- WebView constraints: references/ui-design-rules.md

---

## Next Steps After Integration

Once all checklist items are complete:

1. Commit changes: `git add . && git commit -m "feat(MinimalKick): integrate WebView UI (Stage 5)"`
2. Proceed to Stage 6 (Validation): `/test MinimalKick`
3. Create factory presets (5 presets recommended)
4. Generate CHANGELOG.md
5. Run pluginval (if available)
6. Install to system folders: `/install-plugin MinimalKick`
