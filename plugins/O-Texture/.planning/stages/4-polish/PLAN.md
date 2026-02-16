# Stage 4: Polish - Execution Plan

## Goal

Complete v0.1.0 pre-release polish: fix real-time safety issue, run pluginval validation at maximum strictness, clean up debug artifacts, update version to 0.1.0, create CHANGELOG, ad-hoc code sign, and verify clean build + install.

---

## Tasks

### 1. [ ] Pre-allocate ONNX decoder output buffer (real-time safety fix)
- **Files:** Source/PluginProcessor.h, Source/PluginProcessor.cpp
- **Depends on:** none
- **Details:**
  - Add `std::vector<float> decoderOutputBuffer;` member to TextureProcessor (PluginProcessor.h)
  - In `prepareToPlay()`: `decoderOutputBuffer.resize(static_cast<size_t>(kBlockSize), 0.0f);`
  - In `runDecoder()`: replace `std::vector<float> outputBuffer(...)` with `decoderOutputBuffer` member
  - Use `decoderOutputBuffer.data()` for the output tensor and final memcpy
  - This eliminates a 16KB heap allocation on every decoder call (~42ms at 48kHz)

### 2. [ ] Remove debug console.log statements from main.js
- **Files:** Source/ui/public/js/main.js
- **Depends on:** none
- **Details:**
  - Remove lines 29-31: `console.log('O-Texture UI initializing...')`, backend log, resource address log
  - Remove line 56: `console.log('O-Texture UI initialized')`
  - Remove lines 80-81: X/Y properties debug listeners
  - Remove lines 83, 87: Source/Mode choices debug logs
  - Keep the `propertiesChangedEvent` listeners for `sourceState` and `modeState` (they call `updateSourceButtons()` / `updateModeButtons()`) but remove the `console.log` calls within them

### 3. [ ] Remove unused data-parameter-index HTML attributes
- **Files:** Source/ui/public/index.html
- **Depends on:** none
- **Details:**
  - Remove all `data-parameter-index="..."` attributes from HTML elements
  - These are never read by JavaScript (the JS uses relay names, not indices)
  - Affects: mode-toggle buttons, xy-pad canvas, vertical slider divs, source-button buttons, knob containers, freeze-toggle div

### 4. [ ] Update version from 1.0.0 to 0.1.0
- **Files:** CMakeLists.txt
- **Depends on:** none
- **Details:**
  - Change line 30: `VERSION 1.0.0` → `VERSION 0.1.0`
  - 0.x.y signals pre-release per SemVer
  - Plugin name remains `O-Texture-dev` (suffix controlled by OUARICON_DEV_SUFFIX)

### 5. [ ] Create CHANGELOG.md
- **Files:** plugins/O-Texture/CHANGELOG.md (new)
- **Depends on:** Task 4
- **Details:**
  - Follow Keep a Changelog format + SemVer
  - Document all features from Stages 1-3 under v0.1.0
  - Note placeholder models in Technical Notes section
  - Sections: Added, Technical Notes

### 6. [ ] Build plugin (clean verify)
- **Files:** build/ (CMake + Ninja)
- **Depends on:** Tasks 1, 2, 3, 4
- **Details:**
  - Run `ninja OuariconTexture_VST3 OuariconTexture_AU` from build/
  - Verify clean compile with no warnings (that weren't there before)
  - This is NOT a full clean rebuild (delete build/); that's overkill for these changes

### 7. [ ] Run pluginval - Phase 1: Strictness 5 baseline
- **Files:** none (validation only)
- **Depends on:** Task 6
- **Details:**
  - `pluginval --strictness-level 5 --validate <VST3 path> --timeout-ms 30000 --verbose`
  - `pluginval --strictness-level 5 --validate <AU component path> --timeout-ms 30000 --verbose`
  - Both should pass (previously passed at Stage 2)

### 8. [ ] Run pluginval - Phase 2: Strictness 10 without GUI
- **Files:** none (validation only)
- **Depends on:** Task 7
- **Details:**
  - `pluginval --strictness-level 10 --skip-gui-tests --validate <VST3 path> --timeout-ms 60000 --verbose`
  - Tests binary-exact state restoration, subnormal detection, extended validation
  - Isolates DSP validation from WebView framework issues

### 9. [ ] Run pluginval - Phase 3: Strictness 10 with GUI
- **Files:** none (validation only)
- **Depends on:** Task 8
- **Details:**
  - `pluginval --strictness-level 10 --validate <VST3 path> --timeout-ms 120000 --verbose`
  - Full validation including 1000-iteration editor open/close
  - Known risk: WebView may crash during high-iteration editor automation (JUCE framework limitation)
  - If this fails on GUI tests only, document as known issue — DSP validation (Task 8) is the priority

### 10. [ ] Install plugin and verify registration
- **Files:** system plugin folders
- **Depends on:** Task 6
- **Details:**
  - Follow CLAUDE.md install sequence: clear AU cache, remove old binaries, install fresh
  - Verify AU registration: `auval -a | grep -i texture`
  - Verify VST3 presence in ~/Library/Audio/Plug-Ins/VST3/
  - Verify AU presence in ~/Library/Audio/Plug-Ins/Components/

### 11. [ ] Ad-hoc code sign plugin bundles
- **Files:** build artefacts
- **Depends on:** Task 6
- **Details:**
  - `codesign --deep --force --sign - <VST3 path>`
  - `codesign --deep --force --sign - <AU component path>`
  - Verify: `codesign --verify --verbose=2 <VST3 path>`

---

## Success Criteria

- [ ] No heap allocation in runDecoder() — decoderOutputBuffer is pre-allocated member
- [ ] No console.log statements in production JS
- [ ] No unused data-parameter-index attributes in HTML
- [ ] VERSION is 0.1.0 in CMakeLists.txt
- [ ] CHANGELOG.md exists with v0.1.0 entry
- [ ] Plugin compiles cleanly
- [ ] pluginval passes at strictness 5 (VST3 + AU)
- [ ] pluginval passes at strictness 10 without GUI (VST3)
- [ ] pluginval strictness 10 with GUI attempted (pass or documented known issue)
- [ ] Plugin installs and registers correctly (AU + VST3)
- [ ] Plugin bundles are ad-hoc signed

---

## Known Limitations (Document, Don't Fix)

- WebView may crash during high-iteration editor automation at strictness 10 (JUCE framework issue)
- Placeholder models produce noise, not real textures
- Transform mode limited in Logic Pro (IS_SYNTH sidechain limitations)
- Cross-DAW manual testing (Logic, Ableton, Reaper) is out of scope for automated plan — user handles manually
