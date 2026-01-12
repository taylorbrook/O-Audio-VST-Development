# Stage 3 (GUI) Integration Checklist - v3

**Plugin:** OuariconAnalogEQ
**Mockup Version:** v3
**Generated:** 2026-01-11
**Window Size:** 920×220px (compact rack-unit style)

## 1. Copy UI Files

- [ ] Copy `v3-ui.html` to `Source/ui/public/index.html`
- [ ] Copy JUCE frontend library to `Source/ui/public/js/juce/index.js`
  - Source: Working plugin (e.g., OuariconMarimba) or JUCE examples
- [ ] Copy background image: `paper1.jpg` to `Source/ui/public/images/`
- [ ] Copy botanical overlay: `flower_ferdinandibauer00baue_0021.png` to `Source/ui/public/images/`

## 2. Update PluginEditor Files

- [ ] Replace `PluginEditor.h` with `v3-PluginEditor-TEMPLATE.h` content
- [ ] Verify member order: relays → webView → attachments (CRITICAL for release builds)
- [ ] Update class name to `OuariconAnalogEQAudioProcessorEditor`
- [ ] Replace `PluginEditor.cpp` with `v3-PluginEditor-TEMPLATE.cpp` content
- [ ] Verify initialization order matches declaration order
- [ ] Verify all 16 parameter IDs match PluginProcessor exactly

## 3. Update CMakeLists.txt

- [ ] Append `v3-CMakeLists-SNIPPET.txt` to CMakeLists.txt
- [ ] Verify `juce_add_binary_data` includes all UI files (index.html, index.js, 2 images)
- [ ] Verify `JUCE_WEB_BROWSER=1` definition present
- [ ] Verify `juce::juce_gui_extra` linked
- [ ] Verify `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin()` call

## 4. Build and Test (Debug)

- [ ] Build succeeds without warnings
- [ ] Standalone loads WebView (not blank)
- [ ] Right-click → Inspect works (verify Console accessible)
- [ ] Console shows no JavaScript errors
- [ ] `window.__JUCE__` object exists
- [ ] Paper texture background visible
- [ ] Botanical overlay visible (rotated 90° clockwise)

## 5. Build and Test (Release)

- [ ] Release build succeeds
- [ ] No crashes on plugin reload (test 10 times in DAW)
- [ ] Member order prevents use-after-free (attachments destroyed before webView)

## 6. Test Parameter Binding

- [ ] All 16 parameters sync UI ↔ APVTS:
  - [ ] `lf_freq` - LF frequency knob (outer ring)
  - [ ] `lf_gain` - LF gain knob (inner dial)
  - [ ] `lf_on` - LF band toggle
  - [ ] `lmf_freq` - LMF frequency knob (outer ring)
  - [ ] `lmf_gain` - LMF gain knob (inner dial)
  - [ ] `lmf_q` - LMF Q selector (3-way toggle)
  - [ ] `lmf_on` - LMF band toggle
  - [ ] `hmf_freq` - HMF frequency knob (outer ring)
  - [ ] `hmf_gain` - HMF gain knob (inner dial)
  - [ ] `hmf_q` - HMF Q selector (3-way toggle)
  - [ ] `hmf_on` - HMF band toggle
  - [ ] `hf_freq` - HF frequency knob (outer ring)
  - [ ] `hf_gain` - HF gain knob (inner dial)
  - [ ] `hf_on` - HF band toggle
  - [ ] `output_gain` - Output gain knob (standard single-layer)
  - [ ] `analog` - Analog toggle (large toggle)
- [ ] DAW automation updates UI correctly
- [ ] Preset recall updates all UI controls
- [ ] Parameter values persist after plugin reload

## 7. WebView-Specific Validation

- [ ] No viewport units in CSS (`100vh`, `100vw`, etc.)
- [ ] Native feel CSS present (`user-select: none`)
- [ ] Resource provider returns all files without 404s (check browser console)
- [ ] Correct MIME types:
  - [ ] `text/html` for index.html
  - [ ] `application/javascript` for index.js
  - [ ] `image/jpeg` for paper1.jpg
  - [ ] `image/png` for flower overlay

## 8. Dual-Layer Knob Interaction (4 bands)

- [ ] Outer ring controls frequency (rotates independently)
- [ ] Inner dial controls gain (rotates independently)
- [ ] Both knobs draggable without interference
- [ ] Relative drag (frame-delta, not absolute positioning)
- [ ] Knobs rotate -135° to +135° (270° range)

## 9. VU Meter Animation

- [ ] VU meter needle responds to `output_gain` parameter
- [ ] Smooth ballistic motion (fast attack, slow decay)
- [ ] Needle color changes: green → yellow → red based on level
- [ ] Animation runs at ~60fps via `requestAnimationFrame`

## 10. Final Validation

- [ ] Plugin loads in DAW without errors
- [ ] All 4 EQ bands functional
- [ ] Output section (gain + analog toggle) works
- [ ] VU meter visualizes output level
- [ ] UI aesthetic matches mockup (paper texture, botanical overlay, seed knobs)
- [ ] No console errors or warnings

---

## Parameter List (from parameter-spec.md)

### LF Band (Shelf)
- `lf_freq` - WebSliderRelay + WebSliderParameterAttachment (Float, 30-500 Hz)
- `lf_gain` - WebSliderRelay + WebSliderParameterAttachment (Float, -12 to +12 dB)
- `lf_on` - WebToggleButtonRelay + WebToggleButtonParameterAttachment (Bool)

### LMF Band (Bell)
- `lmf_freq` - WebSliderRelay + WebSliderParameterAttachment (Float, 100-2000 Hz)
- `lmf_gain` - WebSliderRelay + WebSliderParameterAttachment (Float, -12 to +12 dB)
- `lmf_q` - WebSliderRelay + WebSliderParameterAttachment (Choice: WIDE/MED/TIGHT)
- `lmf_on` - WebToggleButtonRelay + WebToggleButtonParameterAttachment (Bool)

### HMF Band (Bell)
- `hmf_freq` - WebSliderRelay + WebSliderParameterAttachment (Float, 500-8000 Hz)
- `hmf_gain` - WebSliderRelay + WebSliderParameterAttachment (Float, -12 to +12 dB)
- `hmf_q` - WebSliderRelay + WebSliderParameterAttachment (Choice: WIDE/MED/TIGHT)
- `hmf_on` - WebToggleButtonRelay + WebToggleButtonParameterAttachment (Bool)

### HF Band (Shelf)
- `hf_freq` - WebSliderRelay + WebSliderParameterAttachment (Float, 2000-20000 Hz)
- `hf_gain` - WebSliderRelay + WebSliderParameterAttachment (Float, -12 to +12 dB)
- `hf_on` - WebToggleButtonRelay + WebToggleButtonParameterAttachment (Bool)

### Global
- `output_gain` - WebSliderRelay + WebSliderParameterAttachment (Float, -12 to +12 dB)
- `analog` - WebToggleButtonRelay + WebToggleButtonParameterAttachment (Bool)

**Total:** 16 relays, 16 attachments

---

## Known Issues & Solutions

### Issue: Knobs don't respond to drag
**Solution:** Verify ES6 module imports - all script tags need `type="module"`

### Issue: VST3 doesn't appear in DAW
**Solution:** Add `NEEDS_WEB_BROWSER TRUE` to `juce_add_plugin()` in CMakeLists.txt

### Issue: Release build crashes on reload
**Solution:** Verify member order in PluginEditor.h (relays → webView → attachments)

### Issue: Images don't load (404)
**Solution:** Check BinaryData resource names match exactly (underscores for special chars)

---

## Critical References

- JUCE 8 Critical Patterns: `troubleshooting/patterns/juce8-critical-patterns.md`
- Member Order Pattern: Pattern #11 (WebView Member Initialization)
- ES6 Module Pattern: Pattern #21 (ES6 Module Loading)
- WebView Config: Pattern #3 (WebView UI Module Requirements)
