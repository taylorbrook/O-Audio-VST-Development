# Stage 1: Foundation - Execution Plan

**Plugin:** O-Chorus
**Stage:** 1-foundation
**Created:** 2026-02-07
**Status:** Ready for execution

---

## Goal

Establish the complete build system and APVTS parameter infrastructure for O-Chorus. The Foundation stage creates a buildable plugin shell with all 6 parameters registered, WebView capability enabled, and zero latency reporting. No DSP processing — audio passes through unchanged.

---

## Tasks

### Task 1: Create CMakeLists.txt
- **Files:** `plugins/O-Chorus/CMakeLists.txt`
- **Depends on:** None
- **Description:**
  - Plugin code `OuCh`, manufacturer code `${OUARICON_MANUFACTURER_CODE}`
  - Version 1.0.0
  - Formats: VST3, AU, Standalone
  - `NEEDS_WEB_BROWSER TRUE` for Stage 3 WebView
  - `NEEDS_WEBVIEW2 TRUE` for Windows WebView2 support
  - `IS_SYNTH FALSE` (audio effect, not instrument)
  - Link required modules: `juce_audio_processors`, `juce_dsp`, `juce_core`, plus standard GUI/audio modules
  - `juce_generate_juce_header()` AFTER `target_link_libraries()` (JUCE 8 requirement)
  - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` compile definition (cross-platform WebView)
  - Binary data target for placeholder WebView resources
  - Include `modules/cmake/OuariconModules.cmake` for shared infrastructure

### Task 2: Create Source Directory Structure
- **Files:** `plugins/O-Chorus/Source/` (directory)
- **Depends on:** None
- **Description:**
  - Create `Source/` directory for processor/editor files
  - Create `Source/ui/public/js/juce/` directory for WebView assets

### Task 3: Create PluginProcessor.h
- **Files:** `plugins/O-Chorus/Source/PluginProcessor.h`
- **Depends on:** Task 2
- **Description:**
  - Class `OChorusAudioProcessor` inheriting from `juce::AudioProcessor`
  - Public `juce::AudioProcessorValueTreeState parameters` member
  - Private static `createParameterLayout()` declaration
  - BusesProperties: stereo in, stereo out
  - Standard AudioProcessor overrides (prepareToPlay, processBlock, etc.)
  - `getLatencySamples()` returns 0 (chorus is zero-latency)
  - Licensing boilerplate (compile-flag gated)

### Task 4: Create PluginProcessor.cpp
- **Files:** `plugins/O-Chorus/Source/PluginProcessor.cpp`
- **Depends on:** Task 3
- **Description:**
  - Implement `createParameterLayout()` with all 6 parameters:
    - `rate`: Float, 0.05-5.0 Hz, default 1.0, skew 0.35 (logarithmic)
    - `depth`: Float, 0.0-1.0, default 0.5, linear
    - `voices`: Int, 1-8, default 4
    - `width`: Float, 0.0-1.0, default 0.7, linear
    - `tone`: Float, -1.0 to +1.0, default 0.0, step 0.01
    - `mix`: Float, 0.0-1.0, default 0.5, linear
  - Constructor with BusesProperties (stereo in/out) and APVTS initialization
  - `prepareToPlay()`: empty (no DSP yet)
  - `processBlock()`: pass-through with `ScopedNoDenormals`, clear unused channels
  - `getStateInformation()` / `setStateInformation()` via APVTS XML serialization
  - `createEditor()` returning new editor instance
  - `createPluginFilter()` factory function

### Task 5: Create PluginEditor.h
- **Files:** `plugins/O-Chorus/Source/PluginEditor.h`
- **Depends on:** Task 3
- **Description:**
  - Class `OChorusAudioProcessorEditor` inheriting from `juce::AudioProcessorEditor`
  - `WebBrowserComponent` member (unique_ptr)
  - `WebSliderRelay` members for 5 float parameters (rate, depth, width, tone, mix)
  - `WebSliderRelay` for voices (int param uses slider relay too)
  - Corresponding `WebSliderParameterAttachment` members
  - Member order: Relays -> WebView -> Attachments (JUCE 8 destruction order pattern)
  - `getResource()` helper declaration for serving binary data
  - Window size: 600x400 (placeholder)

### Task 6: Create PluginEditor.cpp
- **Files:** `plugins/O-Chorus/Source/PluginEditor.cpp`
- **Depends on:** Task 5
- **Description:**
  - Initialize relays with parameter IDs matching JS bridge names
  - Create `WebBrowserComponent` with:
    - `Backend::webview2` option
    - `withUserDataFolder()` for Windows DAW compatibility
    - `withNativeIntegrationEnabled()`
    - `withResourceProvider()` (guarded by `JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE`)
    - Chained `.withOptionsFrom()` for all 6 relays
  - Create attachments with 3-parameter constructor (param, relay, nullptr)
  - `getResource()` implementation serving HTML/JS from binary data
  - `resized()` setting webView bounds to full editor area

### Task 7: Create Placeholder WebView Resources
- **Files:**
  - `plugins/O-Chorus/Source/ui/public/index.html`
  - `plugins/O-Chorus/Source/ui/public/js/juce/index.js`
  - `plugins/O-Chorus/Source/ui/public/js/juce/check_native_interop.js`
- **Depends on:** Task 2
- **Description:**
  - Minimal HTML that displays "O-Chorus" and confirms WebView is working
  - Copy JUCE interop scripts from existing plugins (O-AnalogEQ reference)
  - `type="module"` for ES6 imports in HTML
  - No parameter controls yet (Stage 3)

### Task 8: Build Verification
- **Files:** None (build system)
- **Depends on:** Tasks 1, 4, 6, 7
- **Description:**
  - Configure CMake: `cmake -B build -G Ninja`
  - Build VST3 and AU: `ninja -C build OuariconChorus_VST3 OuariconChorus_AU`
  - Verify no build errors or warnings

### Task 9: Install and DAW Verification
- **Files:** None (system installation)
- **Depends on:** Task 8
- **Description:**
  - Clear AU cache
  - Install VST3 and AU to system plugin folders
  - Verify AU detection with `auval -a | grep -i chorus`
  - Confirm 6 parameters visible in DAW automation
  - Verify audio passes through unchanged

---

## File Summary

| File | Action | Task |
|------|--------|------|
| `CMakeLists.txt` | Create | 1 |
| `Source/PluginProcessor.h` | Create | 3 |
| `Source/PluginProcessor.cpp` | Create | 4 |
| `Source/PluginEditor.h` | Create | 5 |
| `Source/PluginEditor.cpp` | Create | 6 |
| `Source/ui/public/index.html` | Create | 7 |
| `Source/ui/public/js/juce/index.js` | Create | 7 |
| `Source/ui/public/js/juce/check_native_interop.js` | Create | 7 |

---

## Dependency Graph

```
Task 1 (CMakeLists.txt) ─────────────────────────────────┐
                                                          │
Task 2 (Directory Structure) ──┬── Task 3 (Processor.h) ──┤
                               │                          │
                               │── Task 7 (WebView Assets)┤
                               │                          │
                               └── Task 5 (Editor.h) ←────┤
                                        │                 │
                                   Task 4 (Processor.cpp) │
                                        │                 │
                                   Task 6 (Editor.cpp) ───┤
                                                          │
                                                          ▼
                                              Task 8 (Build) ──► Task 9 (DAW Test)
```

---

## Success Criteria

- [ ] `ninja OuariconChorus_VST3 OuariconChorus_AU` completes without errors
- [ ] VST3 and AU binaries generated in `build/plugins/O-Chorus/OuariconChorus_artefacts/Release/`
- [ ] Plugin loads in DAW without crash
- [ ] All 6 parameters visible in DAW automation lane (Rate, Depth, Voices, Width, Tone, Mix)
- [ ] Parameters respond to DAW automation writes
- [ ] WebView placeholder displays "O-Chorus" in plugin window
- [ ] Audio passes through unchanged (no processing yet)
- [ ] State save/load preserves parameter values
- [ ] Zero latency reported to host

---

## JUCE 8 Patterns Applied

| Pattern | Location | Description |
|---------|----------|-------------|
| Header gen | CMakeLists.txt | `juce_generate_juce_header()` after `target_link_libraries()` |
| WebView2 | CMakeLists.txt | `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` |
| WebBrowser | CMakeLists.txt | `NEEDS_WEB_BROWSER TRUE` for VST3 WebView |
| Member order | PluginEditor.h | Relays -> WebView -> Attachments (correct destruction order) |
| 3-param attach | PluginEditor.cpp | `WebSliderParameterAttachment(param, relay, nullptr)` |
| Resource provider | PluginEditor.cpp | Guarded by `JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE` |
| User data folder | PluginEditor.cpp | `withUserDataFolder()` for Windows DAW plugin hosts |
| ScopedNoDenormals | PluginProcessor.cpp | At start of `processBlock()` |

---

## Estimated Wave Parallelization

**Wave 1 (Parallel):**
- Task 1: CMakeLists.txt
- Task 2: Directory structure

**Wave 2 (Parallel, after Wave 1):**
- Task 3: PluginProcessor.h
- Task 7: WebView assets

**Wave 3 (Parallel, after Wave 2):**
- Task 4: PluginProcessor.cpp
- Task 5: PluginEditor.h

**Wave 4 (Sequential, after Wave 3):**
- Task 6: PluginEditor.cpp

**Wave 5 (Sequential):**
- Task 8: Build verification
- Task 9: DAW verification

---

## Next Phase

After all tasks complete and success criteria are met:
- Update STATUS.md to Stage 1 complete
- Proceed to Stage 2 (DSP Implementation)

---

*Plan created: 2026-02-07*
