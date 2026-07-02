# Stage 5 (GUI) Integration Checklist - v4

**Plugin:** Scatter
**Mockup Version:** v4
**Generated:** 2025-11-13

## 1. Copy UI Files

```bash
# Create UI directory structure
mkdir -p plugins/Scatter/Source/ui/public/js/juce

# Copy production HTML
cp plugins/Scatter/.ideas/mockups/v4-ui.html \
   plugins/Scatter/Source/ui/public/index.html

# Copy JUCE frontend library from working plugin
cp plugins/GainKnob/Source/ui/public/js/juce/index.js \
   plugins/Scatter/Source/ui/public/js/juce/index.js

cp plugins/GainKnob/Source/ui/public/js/juce/check_native_interop.js \
   plugins/Scatter/Source/ui/public/js/juce/check_native_interop.js
```

**Verify:**

- [ ] `Source/ui/public/index.html` exists
- [ ] `Source/ui/public/js/juce/index.js` exists
- [ ] `Source/ui/public/js/juce/check_native_interop.js` exists

## 2. Update PluginEditor Files

**Replace PluginEditor.h:**

```bash
# Backup existing (if any)
mv plugins/Scatter/Source/PluginEditor.h \
   plugins/Scatter/Source/PluginEditor.h.backup

# Copy template
cp plugins/Scatter/.ideas/mockups/v4-PluginEditor-TEMPLATE.h \
   plugins/Scatter/Source/PluginEditor.h
```

**Replace PluginEditor.cpp:**

```bash
# Backup existing (if any)
mv plugins/Scatter/Source/PluginEditor.cpp \
   plugins/Scatter/Source/PluginEditor.cpp.backup

# Copy template
cp plugins/Scatter/.ideas/mockups/v4-PluginEditor-TEMPLATE.cpp \
   plugins/Scatter/Source/PluginEditor.cpp
```

**Verify member order in PluginEditor.h:**

- [ ] Relays declared BEFORE webView
- [ ] webView declared BEFORE attachments
- [ ] Total: 9 relays (7 sliders + 2 combo boxes)
- [ ] Total: 9 attachments (matching relays)

**Verify initialization order in PluginEditor.cpp:**

- [ ] Relays created FIRST (before WebView construction)
- [ ] WebView created with `.withOptionsFrom()` for all 9 relays
- [ ] Attachments created LAST (after WebView construction)
- [ ] All attachments use 3-parameter constructor (parameter, relay, nullptr)

## 3. Update CMakeLists.txt

**Append WebView configuration snippet:**

```bash
# Open CMakeLists.txt and ADD v4-CMakeLists-SNIPPET.txt content
# Location: After juce_add_plugin(), before final target_compile_definitions()
```

**Verify:**

- [ ] `juce_add_binary_data(Scatter_UIResources ...)` present
- [ ] Binary data sources include all 3 files (index.html, index.js, check_native_interop.js)
- [ ] `target_link_libraries` includes `Scatter_UIResources`
- [ ] `target_link_libraries` includes `juce::juce_gui_extra`
- [ ] `JUCE_WEB_BROWSER=1` definition present
- [ ] `JUCE_USE_CURL=0` definition present
- [ ] `NEEDS_WEB_BROWSER TRUE` in juce_add_plugin() (CRITICAL for VST3)

## 4. Build and Test (Debug)

```bash
# Clean build
rm -rf build
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build . --parallel
```

**Verify:**

- [ ] Build succeeds without warnings
- [ ] No WebView-related errors
- [ ] Binary data embedded correctly

**Test standalone:**

```bash
# Run standalone app
./plugins/Scatter/Scatter_artefacts/Debug/Standalone/Scatter.app/Contents/MacOS/Scatter
```

**Check:**

- [ ] Plugin window opens (550×600px)
- [ ] UI loads and displays correctly
- [ ] Cream/spacey textured design visible
- [ ] Particle field animates in center
- [ ] All 7 knobs visible and labeled
- [ ] 2 combo boxes visible in header (Scale, Root Note)
- [ ] Right-click → Inspect works (shows console)
- [ ] Console shows no JavaScript errors
- [ ] `window.__JUCE__` object exists in console

## 5. Build and Test (Release)

```bash
# Clean build
rm -rf build
mkdir build && cd build

# Configure for release
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --parallel
```

**Verify:**

- [ ] Release build succeeds
- [ ] No crashes on plugin reload (test 10 times)
- [ ] Member order correct (no release-only crashes)

## 6. Test Parameter Binding

**Load in DAW (Ableton, Logic, Reaper):**

- [ ] All 9 parameters sync UI ↔ APVTS
- [ ] Automation updates UI (move knobs via DAW automation)
- [ ] Preset recall updates UI (save preset, change values, recall)
- [ ] Values persist after reload (close DAW, reopen project)

**Test each control:**

- [ ] `delay_time` knob: 100-2000ms, default 500ms
- [ ] `grain_size` knob: 5-500ms, default 100ms
- [ ] `density` knob: 0-100%, default 50%
- [ ] `pitch_random` knob: 0-100%, default 30%
- [ ] `scale` combo: 5 options, default Chromatic
- [ ] `root_note` combo: 12 options, default C
- [ ] `pan_random` knob: 0-100%, default 75%
- [ ] `feedback` knob: 0-100%, default 30%
- [ ] `mix` knob: 0-100%, default 50%

## 7. WebView-Specific Validation

**CSS constraints:**

- [ ] No viewport units in CSS (100vh, 100vw) ✅ (using 100%)
- [ ] `user-select: none` present ✅ (native feel)
- [ ] Fixed window dimensions: 550×600px ✅

**Resource provider:**

- [ ] All files return correct MIME types
  - [ ] `index.html` → `text/html`
  - [ ] `index.js` → `application/javascript` (NOT `text/javascript`)
  - [ ] `check_native_interop.js` → `application/javascript`
- [ ] No 404s in console (check Network tab)
- [ ] Explicit URL mapping used (Pattern #8 from juce8-critical-patterns.md)

**Interaction:**

- [ ] Knobs use relative drag (Pattern #16 - frame-delta, not absolute)
- [ ] Mouse wheel works on knobs
- [ ] Combo boxes respond to clicks
- [ ] No lag or freezing on interaction

## Parameter List (from parameter-spec.md)

1. **delay_time** - Float, 100-2000ms, default 500ms → WebSliderRelay
2. **grain_size** - Float, 5-500ms, default 100ms → WebSliderRelay
3. **density** - Float, 0-100%, default 50% → WebSliderRelay
4. **pitch_random** - Float, 0-100%, default 30% → WebSliderRelay
5. **scale** - Choice, 0-4, default 0 (Chromatic) → WebComboBoxRelay
6. **root_note** - Choice, 0-11, default 0 (C) → WebComboBoxRelay
7. **pan_random** - Float, 0-100%, default 75% → WebSliderRelay
8. **feedback** - Float, 0-100%, default 30% → WebSliderRelay
9. **mix** - Float, 0-100%, default 50% → WebSliderRelay

## Troubleshooting

### WebView doesn't load (blank screen)

**Check:**
- Resource provider returns all 3 files
- MIME types correct (`application/javascript` NOT `text/javascript`)
- Binary data regenerated (rebuild CMake)

**Solution:**
```bash
rm -rf build && mkdir build && cd build && cmake .. && cmake --build .
```

### Knobs frozen (don't respond to drag)

**Check:**
- ES6 module loading: `<script type="module">` in HTML ✅
- `import { getSliderState }` at top of script ✅
- All attachments use 3-parameter constructor (Pattern #12) ✅
- `check_native_interop.js` included in binary data ✅

**Solution:**
Verify all patterns from juce8-critical-patterns.md applied.

### Release build crashes on reload

**Check:**
- Member order: relays → webView → attachments ✅
- All members use `std::unique_ptr` (Pattern #11) ✅
- Constructor initialization matches declaration order ✅

**Solution:**
Review PluginEditor.h member order, fix if wrong.

### VST3 doesn't appear in DAW

**Check:**
- `NEEDS_WEB_BROWSER TRUE` in juce_add_plugin() (Pattern #9)

**Solution:**
Add flag to CMakeLists.txt, rebuild.

## Success Criteria

GUI integration is complete when:

- ✅ Plugin builds without errors (debug + release)
- ✅ UI loads in standalone app
- ✅ All 9 controls visible and functional
- ✅ Parameters update correctly (C++ ↔ JavaScript)
- ✅ Plugin works in DAW (VST3 + AU)
- ✅ Automation and preset recall work
- ✅ No release build crashes (tested 10 reloads)

## Next Steps

After successful integration:

1. Continue to Stage 4 (Validation)
2. Run pluginval tests
3. Manual DAW testing
4. Install for production use (`/install-plugin Scatter`)
