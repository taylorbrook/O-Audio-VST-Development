# Stage 1: Foundation - Execution Plan

**Plugin:** O-SpectralShaper
**Stage:** 1-foundation
**Created:** 2026-02-03
**Status:** Ready for execution

---

## Goal

Establish the complete build system and APVTS parameter infrastructure for O-SpectralShaper. The Foundation stage creates a buildable plugin shell with all 7 parameters registered, WebView capability enabled, and proper latency reporting. No DSP processing—audio passes through unchanged.

---

## Tasks

### Task 1: Create CMakeLists.txt
- **Files:** `plugins/O-SpectralShaper/CMakeLists.txt`
- **Depends on:** None
- **Description:**
  - Plugin code `OSpS`, manufacturer code `OuDv`
  - Version 1.0.0
  - Formats: VST3, AU, Standalone
  - `NEEDS_WEB_BROWSER TRUE` for Stage 3 WebView
  - `IS_SYNTH FALSE` (audio effect, not instrument)
  - Link all required JUCE modules including `juce_dsp` (for Stage 2)
  - `juce_generate_juce_header()` AFTER `target_link_libraries()` (JUCE 8 pattern)
  - Binary data for placeholder WebView resources

### Task 2: Create Source Directory Structure
- **Files:** `plugins/O-SpectralShaper/Source/` (directory)
- **Depends on:** None
- **Description:**
  - Create Source directory for processor/editor files
  - Create Resources/ui/js/juce/ directory for WebView assets

### Task 3: Create PluginProcessor.h
- **Files:** `plugins/O-SpectralShaper/Source/PluginProcessor.h`
- **Depends on:** Task 2
- **Description:**
  - Class `OSpectralShaperAudioProcessor`
  - APVTS member with 7 parameters
  - Static `createParameterLayout()` declaration
  - Placeholder curve arrays (std::array<float, 32>) for attack/sustain
  - `getLatencySamples()` returns 512 (fixed latency)
  - Standard AudioProcessor overrides

### Task 4: Create PluginProcessor.cpp
- **Files:** `plugins/O-SpectralShaper/Source/PluginProcessor.cpp`
- **Depends on:** Task 3
- **Description:**
  - Implement `createParameterLayout()` with all 7 parameters:
    - MIX (0-1, default 1.0)
    - ATTACK_TIME (0.1-50ms, skew 0.3, default 10ms)
    - SUSTAIN_TIME (10-500ms, skew 0.3, default 100ms)
    - SENSITIVITY (0-1, default 0.5)
    - LOOKAHEAD_ENABLED (bool, default false)
    - LOOKAHEAD_TIME (0.1-10ms, default 2ms)
    - OUTPUT_GAIN (-12 to +12dB, default 0)
  - BusesProperties: stereo in, stereo out
  - `processBlock()`: pass-through (copy input to output)
  - `prepareToPlay()`: report latency via `setLatencySamples(512)`
  - State save/restore with APVTS (curve persistence deferred to Stage 3)

### Task 5: Create PluginEditor.h
- **Files:** `plugins/O-SpectralShaper/Source/PluginEditor.h`
- **Depends on:** Task 3
- **Description:**
  - Class `OSpectralShaperAudioProcessorEditor`
  - WebBrowserComponent member
  - WebSliderRelay members for 6 float parameters
  - WebToggleButtonRelay for LOOKAHEAD_ENABLED
  - Corresponding WebSliderParameterAttachment/WebToggleButtonParameterAttachment members
  - Member order: Relays → WebView → Attachments (JUCE 8 pattern #11)

### Task 6: Create PluginEditor.cpp
- **Files:** `plugins/O-SpectralShaper/Source/PluginEditor.cpp`
- **Depends on:** Task 5
- **Description:**
  - Initialize relays with parameter names matching JS IDs
  - Create WebBrowserComponent with chained `.withOptionsFrom()` for all relays
  - Create attachments with 3-parameter constructor (param, relay, nullptr)
  - `getResource()` helper for serving HTML/JS from binary data
  - Window size: 800x600 (placeholder, will be refined in Stage 3)

### Task 7: Create Placeholder WebView Resources
- **Files:**
  - `plugins/O-SpectralShaper/Resources/ui/index.html`
  - `plugins/O-SpectralShaper/Resources/ui/js/juce/index.js`
  - `plugins/O-SpectralShaper/Resources/ui/js/juce/check_native_interop.js`
- **Depends on:** Task 2
- **Description:**
  - Minimal HTML that displays "O-SpectralShaper" and confirms WebView is working
  - Copy JUCE interop scripts from JUCE examples or existing plugins
  - No curve editing UI yet (Stage 3)

### Task 8: Build Verification
- **Files:** None (build system)
- **Depends on:** Tasks 1, 4, 6, 7
- **Description:**
  - Configure CMake: `cmake -B build -G Ninja`
  - Build VST3 and AU: `ninja -C build O-SpectralShaper_VST3 O-SpectralShaper_AU`
  - Verify no build errors or warnings

### Task 9: Install and DAW Verification
- **Files:** None (system installation)
- **Depends on:** Task 8
- **Description:**
  - Clear AU cache
  - Install to system plugin folders
  - Verify AU detection with `auval -a | grep -i spectral`
  - Load in DAW (Logic Pro or Ableton)
  - Verify all 7 parameters visible in automation
  - Verify latency compensation (512 samples)

---

## File Summary

| File | Action | Task |
|------|--------|------|
| `CMakeLists.txt` | Create | 1 |
| `Source/PluginProcessor.h` | Create | 3 |
| `Source/PluginProcessor.cpp` | Create | 4 |
| `Source/PluginEditor.h` | Create | 5 |
| `Source/PluginEditor.cpp` | Create | 6 |
| `Resources/ui/index.html` | Create | 7 |
| `Resources/ui/js/juce/index.js` | Create | 7 |
| `Resources/ui/js/juce/check_native_interop.js` | Create | 7 |

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

- [ ] `ninja O-SpectralShaper_VST3 O-SpectralShaper_AU` completes without errors
- [ ] VST3 and AU binaries generated in `build/plugins/O-SpectralShaper/O-SpectralShaper_artefacts/Release/`
- [ ] Plugin loads in DAW without crash
- [ ] All 7 parameters visible in DAW automation lane
- [ ] Parameters respond to DAW automation writes
- [ ] Latency correctly reported (512 samples ≈ 11.6ms @ 44.1kHz)
- [ ] WebView placeholder displays in plugin window
- [ ] Audio passes through unchanged (no processing yet)
- [ ] State save/load preserves parameter values

---

## JUCE 8 Patterns Applied

| Pattern | Location | Description |
|---------|----------|-------------|
| #1 | CMakeLists.txt | `juce_generate_juce_header()` after `target_link_libraries()` |
| #9 | CMakeLists.txt | `NEEDS_WEB_BROWSER TRUE` for VST3 WebView |
| #11 | PluginEditor.h | Member order: Relays → WebView → Attachments |
| #12 | PluginEditor.cpp | Three-parameter attachment constructor |
| #19 | PluginEditor.cpp | `WebToggleButtonRelay` for bool parameter |

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
- Command: `/plugin-verify O-SpectralShaper 1`
- Then: `/plugin-handoff O-SpectralShaper 2-dsp`

---

*Plan created: 2026-02-03*
