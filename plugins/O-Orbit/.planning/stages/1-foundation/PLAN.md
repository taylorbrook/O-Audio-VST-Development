# Stage 1: Foundation - Execution Plan

> **Plugin:** O-Orbit
> **Stage:** 1 (Foundation)
> **Date:** 2026-02-10
> **Status:** Ready for execution

---

## Goal

Build the complete project skeleton for O-Orbit: CMake build system with SAF integration, all 17 APVTS parameters in groups, multi-channel bus configuration (2-24 channels), stub DSP classes, and a compiling Standalone/VST3/AU target.

---

## Tasks

### 1. [ ] Add SAF as git submodule
- **Files:** `libs/SAF/` (submodule)
- **Depends on:** None
- **Details:**
  - `git submodule add https://github.com/leomccormack/Spatial_Audio_Framework.git plugins/O-Orbit/libs/SAF`
  - Pin to latest stable release tag (v1.3.5 or latest)
  - Verify `libs/SAF/framework/CMakeLists.txt` exists after checkout

### 2. [ ] Create CMakeLists.txt with SAF integration
- **Files:** `plugins/O-Orbit/CMakeLists.txt`
- **Depends on:** Task 1
- **Details:**
  - Standard Ouaricon plugin template (from O-GrainScatter)
  - `cmake_minimum_required(VERSION 3.15)`
  - Include OuariconModules.cmake
  - `juce_add_plugin(OuariconOrbit ...)` with:
    - `PLUGIN_CODE OuOr`
    - `FORMATS VST3 AU Standalone`
    - `PRODUCT_NAME "O-Orbit${OUARICON_DEV_SUFFIX}"`
    - `VERSION 1.0.0`
    - `NEEDS_WEB_BROWSER TRUE`
    - `NEEDS_WEBVIEW2 TRUE`
    - `IS_SYNTH FALSE`
    - `NEEDS_MIDI_INPUT FALSE`
    - `IS_MIDI_EFFECT FALSE`
  - SAF config BEFORE `add_subdirectory`:
    - `SAF_BUILD_TESTS OFF`, `SAF_BUILD_EXAMPLES OFF`
    - `SAF_ENABLE_SOFA_READER_MODULE OFF`, `SAF_ENABLE_TRACKER_MODULE OFF`, `SAF_ENABLE_HADES_MODULE OFF`
  - `add_subdirectory(libs/SAF/framework saf_build)`
  - `target_sources`: all Source files (Processor, Editor, DSP stubs, Data stubs)
  - `target_include_directories`: Source, Source/DSP, Source/Data
  - `target_link_libraries`: standard JUCE modules + `saf`
  - `juce_generate_juce_header(OuariconOrbit)` AFTER `target_link_libraries`
  - Compile definitions: `JUCE_VST3_CAN_REPLACE_VST2=0`, `JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`
  - No binary data yet (Stage 3 adds WebView resources)

### 3. [ ] Create PluginProcessor with APVTS (17 parameters)
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Task 2
- **Details:**
  - Class: `OOrbitProcessor : public juce::AudioProcessor`
  - Constructor: `BusesProperties` with stereo input + stereo output (default)
  - `isBusesLayoutSupported()`: accept mono/stereo input, 2-24 channel output
  - Static `createParameterLayout()` with 3 parameter groups:
    - **Motion** (9 params): PATH (Choice), SPEED (Float 0.01-20 skew 0.5), WIDTH (Float 0-360), DEPTH (Float 0-100), TILT (Float -90 to +90), PHASE (Float 0-360), ELEVATION_ENABLE (Bool), ELEVATION_RANGE (Float 0-90), TEMPO_SYNC (Choice: Off + 14 divisions)
    - **Spatial** (5 params): SPEAKER_LAYOUT (Choice: 9 presets), DISTANCE (Float 0.1-30 skew 0.5), AIR_ABSORPTION (Float 0-100), ATTENUATION_CURVE (Choice: 3 curves), CENTER_DIVERGE (Float 0-100)
    - **Mix** (3 params): SOURCE_MODE (Choice: Mono/L+R Split), LR_OFFSET (Float 0-360), MIX (Float 0-100)
  - Cached `std::atomic<float>*` pointers for all 17 parameters
  - `prepareToPlay()`: store sample rate, init smoothed values (20ms ramp)
  - `processBlock()`: stub with `juce::ScopedNoDenormals`, read params, pass-through audio
  - `getStateInformation()` / `setStateInformation()`: APVTS XML serialization
  - Standard boilerplate: `getName()`, `acceptsMidi()`, `producesMidi()`, `isMidiEffect()`, `getTailLengthSeconds()`, `hasEditor()`, program stubs
  - `setLatencySamples(0)` in prepareToPlay

### 4. [ ] Create PluginEditor (minimal shell)
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Task 3
- **Details:**
  - Class: `OOrbitEditor : public juce::AudioProcessorEditor`
  - Minimal: set window size (900x600), display plugin name text
  - No WebView yet (Stage 3)
  - Reference to processor for future parameter access

### 5. [ ] Create SpeakerLayout data structures
- **Files:** `Source/Data/SpeakerLayout.h`, `Source/Data/SpeakerLayout.cpp`
- **Depends on:** None
- **Details:**
  - `struct Speaker { float azimuth, elevation, distance; juce::String label; }`
  - `struct SpeakerLayout { juce::String name; std::vector<Speaker> speakers; bool is3D; }`
  - Azimuth convention: counter-clockwise, 0=front, +90=left (matches SAF)
  - Helper: `int getChannelCount() const { return (int)speakers.size(); }`

### 6. [ ] Create SpeakerPresets with preset layout data
- **Files:** `Source/Data/SpeakerPresets.h`
- **Depends on:** Task 5
- **Details:**
  - Static factory functions for each preset layout:
    - `SpeakerPresets::stereo()` — 2 speakers: L(-30,0), R(30,0)
    - `SpeakerPresets::quad()` — 4 speakers: L(-45,0), R(45,0), Ls(-135,0), Rs(135,0)
    - `SpeakerPresets::surround51()` — 6 speakers: L(-30,0), R(30,0), C(0,0), LFE(0,0), Ls(-110,0), Rs(110,0)
    - `SpeakerPresets::surround71()` — 8 speakers: 5.1 + Lss(-90,0), Rss(90,0)
    - `SpeakerPresets::surround514()` — 10 speakers: 5.1 + 4 height at elevation 45
    - `SpeakerPresets::surround714()` — 12 speakers: 7.1 + 4 height
    - `SpeakerPresets::hexaphonic()` — 6 speakers at 60deg intervals
    - `SpeakerPresets::octaphonic()` — 8 speakers at 45deg intervals
  - `SpeakerPresets::getPreset(int index)` — returns layout by APVTS choice index
  - All positions in degrees, distance=1.0m default

### 7. [ ] Create DSP stub classes (MotionEngine, VBAPRenderer, DistanceModel)
- **Files:** `Source/DSP/MotionEngine.h`, `Source/DSP/MotionEngine.cpp`, `Source/DSP/VBAPRenderer.h`, `Source/DSP/VBAPRenderer.cpp`, `Source/DSP/DistanceModel.h`, `Source/DSP/DistanceModel.cpp`
- **Depends on:** Task 5
- **Details:**
  - **MotionEngine**: `prepare(double sampleRate)`, `process(int numSamples)` returning `MotionState { float azimuth, elevation, distance }`; setters for path, speed, width, depth, tilt, phase, elevationEnable, elevationRange; stub implementations (return 0,0,1.0)
  - **VBAPRenderer**: `prepare(const SpeakerLayout& layout)`, `computeGains(float azimuth, float elevation, float* gains, int numSpeakers)`; stub returns equal gain to all speakers
  - **DistanceModel**: `prepare(double sampleRate)`, `updateDistance(float distance, float airAbsorption, int attenuationCurve)`, `processSample(float sample)` returning filtered sample; stub returns input unchanged
  - All stubs compile and link but produce pass-through behavior

### 8. [ ] Create empty Resources/ui directory
- **Files:** `Resources/ui/.gitkeep`
- **Depends on:** None
- **Details:**
  - Empty directory placeholder for Stage 3 WebView resources

### 9. [ ] Verify build compiles (all 3 targets)
- **Files:** None (build verification)
- **Depends on:** Tasks 1-8
- **Details:**
  - Run CMake configure from build directory
  - Build `OuariconOrbit_VST3`, `OuariconOrbit_AU`, `OuariconOrbit_Standalone`
  - Verify SAF compiles with Apple Accelerate (macOS)
  - Verify all 17 parameters appear in APVTS
  - Fix any compile errors

### 10. [ ] Run Standalone and verify parameter presence
- **Files:** None (runtime verification)
- **Depends on:** Task 9
- **Details:**
  - Launch Standalone build
  - Verify it opens without crashing
  - Verify window title shows "O-Orbit" (or "O-Orbit-dev")
  - Audio pass-through works (input → output)

---

## Success Criteria

- [ ] SAF submodule added and compiles on macOS (Apple Accelerate)
- [ ] CMakeLists.txt follows Ouaricon template pattern
- [ ] All 17 parameters defined in APVTS with correct types, ranges, defaults, and skews
- [ ] Parameters organized in 3 groups (Motion, Spatial, Mix)
- [ ] Multi-channel bus: accepts mono/stereo input, 2-24 channel output
- [ ] Default bus config is stereo in/out
- [ ] Speaker preset data structures defined with 8 preset layouts
- [ ] DSP stubs compile and link (MotionEngine, VBAPRenderer, DistanceModel)
- [ ] Plugin builds as VST3, AU, and Standalone on macOS
- [ ] Standalone launches without crash
- [ ] No compiler warnings from O-Orbit source files

---

## File Summary

### New Files (14)
| File | Purpose |
|------|---------|
| `CMakeLists.txt` | Build system with SAF integration |
| `Source/PluginProcessor.h` | Processor header with APVTS + cached params |
| `Source/PluginProcessor.cpp` | Processor implementation, parameter layout, bus config |
| `Source/PluginEditor.h` | Minimal editor shell |
| `Source/PluginEditor.cpp` | Minimal editor implementation |
| `Source/Data/SpeakerLayout.h` | Speaker + SpeakerLayout structs |
| `Source/Data/SpeakerLayout.cpp` | SpeakerLayout helper implementations |
| `Source/Data/SpeakerPresets.h` | 8 preset speaker layouts |
| `Source/DSP/MotionEngine.h` | Motion engine stub header |
| `Source/DSP/MotionEngine.cpp` | Motion engine stub implementation |
| `Source/DSP/VBAPRenderer.h` | VBAP renderer stub header |
| `Source/DSP/VBAPRenderer.cpp` | VBAP renderer stub implementation |
| `Source/DSP/DistanceModel.h` | Distance model stub header |
| `Source/DSP/DistanceModel.cpp` | Distance model stub implementation |

### External Dependencies
| Dependency | Method | Path |
|------------|--------|------|
| SAF | Git submodule | `libs/SAF/` |
| Apple Accelerate | System framework (macOS) | Auto-detected by SAF |

---

## Notes

- Custom path (5th path type) is deferred to v1.1 per roadmap. PATH choice has 4 options: Orbit, Pendulum, Linear, Drift.
- No WebView binary data in Stage 1 — editor is a simple JUCE component placeholder.
- SAF `extern "C"` wrapper required in any C++ file that includes `saf.h`.
- Speaker layout parameter (SPEAKER_LAYOUT) stored in APVTS but not intended for real-time automation.
