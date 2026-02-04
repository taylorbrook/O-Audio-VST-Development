# Stage 1: Foundation - Execution Plan

**Phase:** Plan Complete
**Date:** 2026-02-03
**Context Sources:** CONTEXT.md, RESEARCH.md, parameter-spec.md

---

## Goal

Create a buildable O-FreqPulse plugin shell with CMake configuration, all 165 APVTS parameters organized in DAW-friendly groups, cached parameter pointers for real-time access, and passthrough audio processing.

---

## Tasks

### Task 1: Create CMakeLists.txt
**Files:** `plugins/O-FreqPulse/CMakeLists.txt`
**Depends on:** None

- Plugin configuration (O-FreqPulse, version 1.0.0)
- Unique plugin code: OFPu (matches ARCHITECTURE.md)
- Formats: VST3, AU, Standalone
- NEEDS_WEB_BROWSER TRUE (for Stage 3 WebView)
- Link all required JUCE modules (juce_dsp, juce_gui_extra, etc.)
- Generate JuceHeader.h
- Standard compile definitions (JUCE_WEB_BROWSER=1)
- NOTE: No binary data yet (WebView resources added in Stage 3)

### Task 2: Create PluginProcessor.h
**Files:** `plugins/O-FreqPulse/Source/PluginProcessor.h`
**Depends on:** Task 1

- AudioProcessor class declaration
- BandParams struct for cached per-band pointers
- 5 global atomic pointers (mix, steps, rate, swing, smoothing)
- 4 BandParams instances with 8 control + 32 step pointers each
- APVTS member (must be declared after cached pointers)
- Static createParameterLayout() declaration
- Standard JUCE boilerplate (getName, acceptsMidi, etc.)

### Task 3: Create PluginProcessor.cpp - Parameter Layout
**Files:** `plugins/O-FreqPulse/Source/PluginProcessor.cpp`
**Depends on:** Task 2

- Implement createParameterLayout() with AudioProcessorParameterGroup:
  - Global group (5 params)
  - Band 0-3 groups (8 control params each)
  - Step parameters nested within band groups (32 per band)
- Use ParameterID with version hint 1 for all parameters
- Correct ranges per parameter-spec.md:
  - mix: 0-1, default 1.0
  - steps: Choice [4,8,16,32], default index 2
  - rate: Choice with 10 sync values, default index 4
  - swing: 0-1, default 0.0
  - smoothing: 0-100, default 5.0
  - band_low/high: 20-20000 with skew 0.3
  - band_depth: 0-1, default 1.0
  - euc_steps: 1-32, default 16
  - euc_pulses: 1-32, default 8
  - euc_offset: 0-31, default 0

### Task 4: Create PluginProcessor.cpp - Constructor & Caching
**Files:** `plugins/O-FreqPulse/Source/PluginProcessor.cpp`
**Depends on:** Task 3

- Constructor initializes APVTS with createParameterLayout()
- Cache all 5 global parameter pointers
- Loop to cache all 4 band parameter sets (8 control + 32 step each)
- Total: 5 + (4 × 40) = 165 cached pointers
- BusesProperties: stereo input, stereo output

### Task 5: Create PluginProcessor.cpp - Audio Passthrough
**Files:** `plugins/O-FreqPulse/Source/PluginProcessor.cpp`
**Depends on:** Task 4

- prepareToPlay(): store sampleRate (placeholder for Stage 2)
- releaseResources(): empty (no resources to release yet)
- processBlock(): audio passthrough only (copy input to output, or do nothing if same buffer)
- Proper silence flag handling

### Task 6: Create PluginProcessor.cpp - State & Metadata
**Files:** `plugins/O-FreqPulse/Source/PluginProcessor.cpp`
**Depends on:** Task 4

- getStateInformation(): APVTS copyState() to XML
- setStateInformation(): XML to APVTS replaceState()
- All metadata methods (getName, acceptsMidi, etc.)
- getTailLengthSeconds(): return 0.0 (will update in Stage 2)

### Task 7: Create PluginEditor.h
**Files:** `plugins/O-FreqPulse/Source/PluginEditor.h`
**Depends on:** Task 2

- Minimal placeholder editor
- Reference to processor
- Fixed window size (800x600 as placeholder)

### Task 8: Create PluginEditor.cpp
**Files:** `plugins/O-FreqPulse/Source/PluginEditor.cpp`
**Depends on:** Task 7

- Constructor: set size, display "O-FreqPulse" label
- paint(): fill background, draw placeholder text
- resized(): empty or minimal layout

### Task 9: Build and Validate
**Files:** None (build command)
**Depends on:** Tasks 1-8

- Run CMake configuration
- Build VST3 and AU targets
- Verify no compile errors
- Check parameter count in DAW automation list

### Task 10: DAW Integration Test
**Files:** None (manual verification)
**Depends on:** Task 9

- Clear AU cache and install plugins
- Load in DAW (Logic/Ableton)
- Verify all 165 parameters visible in automation
- Verify parameters grouped by band (VST3)
- Test state save/load (preset recall)
- Verify audio passthrough (input = output)

---

## File Creation Summary

| File | Action | Lines (est.) |
|------|--------|--------------|
| `CMakeLists.txt` | Create | ~45 |
| `Source/PluginProcessor.h` | Create | ~70 |
| `Source/PluginProcessor.cpp` | Create | ~280 |
| `Source/PluginEditor.h` | Create | ~25 |
| `Source/PluginEditor.cpp` | Create | ~35 |

**Total:** 5 files, ~455 lines

---

## Success Criteria

- [ ] Plugin compiles for VST3 and AU without errors
- [ ] Plugin loads in DAW without crash
- [ ] All 165 parameters visible in DAW automation list
- [ ] Parameters grouped logically in VST3 automation (Global, Band 1-4)
- [ ] Audio passes through unchanged (verified with test signal)
- [ ] State save/load works (modify params, save preset, reload, verify values)
- [ ] No memory leaks or warnings in debug build

---

## Dependencies Graph

```
Task 1 (CMakeLists.txt)
    └── Task 2 (PluginProcessor.h)
            ├── Task 3 (Parameter Layout)
            │       └── Task 4 (Constructor & Caching)
            │               ├── Task 5 (Audio Passthrough)
            │               └── Task 6 (State & Metadata)
            └── Task 7 (PluginEditor.h)
                    └── Task 8 (PluginEditor.cpp)

Tasks 5, 6, 8 → Task 9 (Build)
Task 9 → Task 10 (DAW Test)
```

---

## Risk Mitigations

| Risk | Mitigation |
|------|------------|
| Parameter ID typos | Use loop-generated IDs matching parameter-spec.md exactly |
| Group hierarchy errors | Follow RESEARCH.md pattern with nested AudioProcessorParameterGroup |
| Build failures | Use O-Freeze CMakeLists.txt as verified template |
| AU cache issues | Follow CLAUDE.md cache-clearing protocol |

---

## Notes

- WebView UI resources NOT added in Stage 1 (deferred to Stage 3)
- No DSP processing - passthrough only (FFT deferred to Stage 2)
- Editor is placeholder - just shows plugin name
- Step parameters nested within band groups for cleaner DAW display

---

**Status:** PLAN COMPLETE - Ready for /plugin-execute
