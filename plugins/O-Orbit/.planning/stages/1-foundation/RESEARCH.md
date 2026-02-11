# Stage 1: Foundation - Research

> **Stage:** 1 (Foundation)
> **Date:** 2026-02-09
> **Status:** Complete

---

## 1. SAF Integration (Git Submodule + CMake)

### Submodule Setup

```bash
cd plugins/O-Orbit
git submodule add https://github.com/leomccormack/Spatial_Audio_Framework.git libs/SAF
```

Pin to latest stable release (v1.3.5). The key build target is `saf` which is created by `libs/SAF/framework/CMakeLists.txt`.

### CMake Integration Pattern

SAF provides a `saf` CMake target when added via `add_subdirectory()`. The SAF framework subdirectory is at `libs/SAF/framework/` (not the root).

**Critical SAF CMake variables:**

| Variable | Value | Purpose |
|----------|-------|---------|
| `SAF_BUILD_TESTS` | OFF | Skip SAF test binary |
| `SAF_BUILD_EXAMPLES` | OFF | Skip SAF example binaries |
| `SAF_ENABLE_SOFA_READER_MODULE` | OFF | Not needed for VBAP |
| `SAF_ENABLE_TRACKER_MODULE` | OFF | GPLv2 - avoid |
| `SAF_ENABLE_HADES_MODULE` | OFF | GPLv2 - avoid |

**Platform BLAS/LAPACK:**

| Platform | Backend | CMake/Env Variable | Notes |
|----------|---------|-------------------|-------|
| macOS | Apple Accelerate | Auto-detected | Zero config, ships with macOS |
| Windows | Intel MKL | `SAF_PERFORMANCE_LIB=SAF_USE_INTEL_MKL_LP64` | Via Intel oneAPI |
| Windows (alt) | OpenBLAS | `SAF_PERFORMANCE_LIB=SAF_USE_OPEN_BLAS_AND_LAPACKE` | Simpler CI |
| Linux | OpenBLAS | `SAF_PERFORMANCE_LIB=SAF_USE_OPEN_BLAS_AND_LAPACKE` | apt install |

### Linking SAF to Plugin

```cmake
target_link_libraries(OuariconOrbit PRIVATE saf)
```

SAF propagates include directories via CMake, so `#include "saf.h"` works automatically. **Must wrap in `extern "C"`** since SAF is C89:

```cpp
extern "C" {
#include "saf.h"
}
```

### Where to Place SAF Config in Build System

**Problem:** The root `CMakeLists.txt` auto-discovers plugins via `add_subdirectory()`. SAF config must be set BEFORE the `add_subdirectory(libs/SAF/framework)` call inside the plugin CMakeLists.

**Solution:** Configure SAF variables and add SAF subdirectory inside the plugin's own CMakeLists.txt. This keeps O-Orbit self-contained without modifying the root build file.

```cmake
# Inside plugins/O-Orbit/CMakeLists.txt:
set(SAF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SAF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SAF_ENABLE_SOFA_READER_MODULE OFF CACHE BOOL "" FORCE)
set(SAF_ENABLE_TRACKER_MODULE OFF CACHE BOOL "" FORCE)
set(SAF_ENABLE_HADES_MODULE OFF CACHE BOOL "" FORCE)

add_subdirectory(libs/SAF/framework saf_build)

# Then link:
target_link_libraries(OuariconOrbit PRIVATE saf ...)
```

The `saf_build` binary dir argument prevents collision with other plugins that might also include SAF.

### SAF Modules Needed

For O-Orbit VBAP, only these modules are exercised:

- **saf_vbap**: `generateVBAPgainTable3D()`, `findLsTriplets()`, `invertLsMtx3D()`, `vbap3D()`
- **saf_utilities**: Internal dependency of saf_vbap

All core modules compile together as the single `saf` target. We don't pick modules individually.

### Known Build Gotchas

| Issue | Cause | Fix |
|-------|-------|-----|
| `saf.h: No such file` | Wrong subdirectory path | Must be `libs/SAF/framework`, not `libs/SAF` |
| `Undefined symbols: cblas_*` | BLAS not found | macOS auto-detects; Windows needs `SAF_PERFORMANCE_LIB` env var |
| `getSHreal_recur undeclared` | Missing extern "C" | Wrap: `extern "C" { #include "saf.h" }` |
| `multiple definition` | SAF linked twice | Ensure `saf` target only in one `target_link_libraries` |

---

## 2. Existing Plugin CMakeLists.txt Template

### Standard Template (from O-GrainScatter, O-AnalogSaturation, O-Chorus)

```cmake
cmake_minimum_required(VERSION 3.15)

include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)

juce_add_plugin(PluginTargetName
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuXX
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-PluginName${OUARICON_DEV_SUFFIX}"
    VERSION X.Y.Z
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
)

target_sources(PluginTargetName PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(PluginTargetName PRIVATE Source)

target_link_libraries(PluginTargetName
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# CRITICAL: must come AFTER target_link_libraries
juce_generate_juce_header(PluginTargetName)

# Compile definitions
target_compile_definitions(PluginTargetName
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        JUCE_USE_CURL=0
)
```

### O-Orbit Differences from Template

1. **SAF dependency**: `add_subdirectory(libs/SAF/framework saf_build)` + `target_link_libraries(... saf)`
2. **Effect-specific flags**: `IS_SYNTH FALSE`, `NEEDS_MIDI_INPUT FALSE`, `IS_MIDI_EFFECT FALSE`
3. **Subfolder includes**: `target_include_directories` needs Source, Source/DSP, Source/Data
4. **Plugin code**: `OuOr` (unique 4-char code)
5. **VST3 categories**: `"Spatial" "Fx"`
6. **AU type**: `kAudioUnitType_Effect`
7. **No UI binary data yet**: Stage 3 adds `juce_add_binary_data` for WebView resources

### Global Variables from Root CMakeLists.txt

- `${OUARICON_COMPANY_NAME}` -- "Ouaricon Audio" (release) or "Ouaricon Audio Development" (dev)
- `${OUARICON_MANUFACTURER_CODE}` -- OuAu (release) or OuDv (dev)
- `${OUARICON_DEV_SUFFIX}` -- "" (release) or "-dev" (dev)

---

## 3. Multi-Channel Bus Configuration

### Constructor: Default to Stereo

```cpp
OOrbitProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}
```

Start with stereo default. The user changes speaker layout via the plugin UI, and the plugin requests a bus change from the host.

### isBusesLayoutSupported(): Accept 2-24 Channels

```cpp
bool isBusesLayoutSupported(const BusesLayout& layouts) const override
{
    auto mainInput = layouts.getMainInputChannelSet();
    auto mainOutput = layouts.getMainOutputChannelSet();

    if (mainInput.isDisabled() || mainOutput.isDisabled())
        return false;

    // Input: mono or stereo
    if (mainInput != juce::AudioChannelSet::mono()
        && mainInput != juce::AudioChannelSet::stereo())
        return false;

    // Output: 2-24 channels of any configuration
    int numOut = mainOutput.size();
    return numOut >= 2 && numOut <= 24;
}
```

This accepts named formats (stereo, 5.1, 7.1, 7.1.4) AND discrete channel counts. The plugin's internal speaker layout maps output channels to speaker positions regardless of the host's channel type.

### Runtime Channel Detection

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    int outputChannels = getTotalNumOutputChannels();
    int inputChannels = getTotalNumInputChannels();

    // Compare with configured speaker layout
    int layoutChannels = currentSpeakerLayout.speakers.size();

    if (outputChannels < layoutChannels) {
        isDownmixActive = true;
        // Generate downmix matrix
    } else {
        isDownmixActive = false;
    }
}
```

### Available Named Channel Sets (from JUCE 8.0.4)

| Format | Channels | JUCE Method |
|--------|----------|-------------|
| Stereo | 2 | `AudioChannelSet::stereo()` |
| Quad | 4 | `AudioChannelSet::quadraphonic()` |
| 5.1 | 6 | `AudioChannelSet::create5point1()` |
| 7.1 | 8 | `AudioChannelSet::create7point1()` |
| 5.1.4 | 10 | `AudioChannelSet::create5point1point4()` |
| 7.1.4 | 12 | `AudioChannelSet::create7point1point4()` |
| Discrete N | N | `AudioChannelSet::discreteChannels(N)` |

### DAW Compatibility

| DAW | Multi-Ch Support | Notes |
|-----|------------------|-------|
| Logic Pro | Up to 7.1.4 | AU format; clear cache on build |
| Reaper | All formats | VST3; best multi-channel support |
| Nuendo | All formats | VST3; professional immersive mixing |
| Ableton | Stereo only | Auto-downmix handles this |
| Pro Tools | Max 7.1.2 | AAX; 7.1.4 not supported |

### Critical: No PLUGIN_CHANNEL_CONFIGURATIONS

Never use `PLUGIN_CHANNEL_CONFIGURATIONS` in CMakeLists.txt. Use `isBusesLayoutSupported()` for dynamic negotiation. The legacy option breaks ambisonics/Atmos detection.

---

## 4. APVTS Parameter Patterns (from existing plugins)

### Parameter Group Pattern (from O-GrainScatter)

```cpp
auto motionGroup = std::make_unique<juce::AudioProcessorParameterGroup>(
    "motion", "Motion", "|");

motionGroup->addChild(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "path", 1 },
    "Path",
    juce::StringArray { "Orbit", "Pendulum", "Linear", "Drift" },
    0  // default index
));

motionGroup->addChild(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "speed", 1 },
    "Speed",
    juce::NormalisableRange<float>(0.01f, 20.0f, 0.0f, 0.5f), // skew 0.5 = exponential
    1.0f  // default
));
```

### Cached Parameter Pointers (real-time safe access)

```cpp
// In constructor, after APVTS creation:
speedParam = parameters.getRawParameterValue("speed");
widthParam = parameters.getRawParameterValue("width");

// In processBlock:
float speed = speedParam->load();
float width = widthParam->load();
```

### ParameterID Versioning

All parameters use `juce::ParameterID { "name", 1 }` where the second argument is the parameter version number (1 = initial).

### SmoothedValue Pattern

```cpp
juce::SmoothedValue<float> speedSmoothed;

void prepareToPlay(double sampleRate, int) override {
    speedSmoothed.reset(sampleRate, 0.02); // 20ms ramp
}

void processBlock(...) override {
    speedSmoothed.setTargetValue(speedParam->load());
    for (int s = 0; s < numSamples; ++s) {
        float speed = speedSmoothed.getNextValue();
        // use speed
    }
}
```

---

## 5. SAF VBAP API Reference

### Key Functions for O-Orbit

```c
// Generate 3D VBAP gain table (precompute once per layout change)
void generateVBAPgainTable3D(
    float* ls_dirs_deg,     // Speaker dirs; FLAT: L x 2 (az, el in degrees)
    int L,                  // Number of speakers
    int az_res_deg,         // Azimuth resolution (1 degree recommended)
    int el_res_deg,         // Elevation resolution (1 degree)
    int omitLargeTriangles, // 1 = remove triangles > 120 degrees
    int enableDummies,      // 1 = add imaginary speakers to fill gaps
    float spread,           // Spreading in degrees (0 = point source)
    float** gtable,         // Output: gain table; FLAT: N_gtable x L
    int* N_gtable,          // Output: number of table entries
    int* nTriangles         // Output: number of speaker triangles
);

// Direct VBAP computation
void vbap3D(
    float* src_dirs,        // Source dirs in degrees; FLAT: src_num x 2
    int src_num,
    int ls_num,
    int* ls_groups,         // Triangle indices; FLAT: nFaces x 3
    int nFaces,
    float spread,
    float* layoutInvMtx,    // Inverted matrices; FLAT: nFaces x 9
    float** GainMtx         // Output: gains; FLAT: src_num x ls_num
);
```

### Gain Table Lookup Pattern

Pre-generate at 1-degree resolution for real-time lookup (avoids triangle search per block):

```cpp
// Quantize source direction to table index
int azIdx = ((int)std::round(azimuthDeg) + 180) % 360;
int elIdx = std::clamp((int)std::round(elevationDeg) + 90, 0, 180);
int idx = azIdx * 181 + elIdx;
// gains = gainTable[idx * numSpeakers ... (idx+1) * numSpeakers - 1]
```

### Memory Management

SAF allocates output arrays with `malloc()`. Caller must `free()` them:

```cpp
float* gtable = nullptr;
int nEntries = 0, nTris = 0;
generateVBAPgainTable3D(..., &gtable, &nEntries, &nTris);
// ... copy gtable into your own data structure ...
free(gtable);
```

---

## 6. Findings Summary

### Ready to Implement

1. **CMakeLists.txt**: Standard Ouaricon template + SAF subdirectory + `saf` link target
2. **APVTS**: 17 parameters in groups (motion, spatial, mix) with cached pointers
3. **Bus config**: BusesProperties stereo default, isBusesLayoutSupported accepts 2-24
4. **Source structure**: Subfolder organization (DSP/, Data/)
5. **SAF build**: macOS auto-detects Accelerate, no extra config needed for local dev

### Potential Issues

1. **SAF submodule size**: SAF repo is ~150MB (includes HRIRs, examples). The `framework/` subdirectory alone is much smaller, but the full repo is cloned.
2. **SAF build time**: First build compiles all SAF modules (~30-60 seconds). Subsequent builds are incremental.
3. **CI/CD Windows**: Requires `SAF_PERFORMANCE_LIB` env var -- deferred to Stage 4.

### No Blocking Issues Found

All research questions resolved. Ready to proceed to plan phase.

---

## Sources

- `/Users/taylorbrook/Dev/VST-development/research/saf-juce-integration-guide.md` (local)
- `/Users/taylorbrook/Dev/VST-development/research/juce8-multichannel-spatial-audio.md` (local)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-GrainScatter/CMakeLists.txt` (template)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-AnalogSaturation/CMakeLists.txt` (template)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/CMakeLists.txt` (synth template)
- `/Users/taylorbrook/JUCE/modules/juce_audio_processors/processors/juce_AudioProcessor.h`
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_AudioChannelSet.h`
- `/Users/taylorbrook/JUCE/examples/Plugins/SurroundPluginDemo.h`
- SAF GitHub repository (leomccormack/Spatial_Audio_Framework)
- SPARTA plugin suite (reference SAF+JUCE integration)
