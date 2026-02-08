# SAF + JUCE 8 Integration Guide: Ambisonics Encoding & Binaural Decoding

**Research Date:** 2026-02-08
**SAF Version:** 1.3.5
**JUCE Version:** 8.0.4
**Targets:** macOS (primary, Apple Accelerate), Windows (cross-compile, OpenBLAS or MKL)
**Use Case:** Per-grain ambisonics encoding and binaural decoding in a spatial granular synthesis plugin

---

## Table of Contents

1. [SAF Library Overview](#1-saf-library-overview)
2. [CMake Integration](#2-cmake-integration)
3. [SAF Ambisonics Encoding API](#3-saf-ambisonics-encoding-api)
4. [SAF Binaural Decoding API](#4-saf-binaural-decoding-api)
5. [SAF VBAP API](#5-saf-vbap-api)
6. [SPARTA Plugins as Reference](#6-sparta-plugins-as-reference)
7. [Alternative: Lightweight Self-Contained Implementation](#7-alternative-lightweight-self-contained-implementation)
8. [Build and Test Verification](#8-build-and-test-verification)

---

## 1. SAF Library Overview

### Repository & License

- **Repository:** https://github.com/leomccormack/Spatial_Audio_Framework
- **Author:** Leo McCormack (Aalto University / Audio Engineering)
- **Language:** C89 (with C++ compatibility)
- **Current Version:** 1.3.5
- **Build System:** CMake

**License breakdown:**

| Component | License | Commercial Use |
|-----------|---------|----------------|
| Core modules (saf_hoa, saf_sh, saf_vbap, saf_hrir, saf_reverb, saf_cdf4sap, saf_utilities) | ISC | Yes, with attribution |
| saf_sofa_reader (optional) | ISC | Yes, with attribution |
| saf_tracker (optional) | GPLv2 | Only in GPL projects |
| saf_hades (optional) | GPLv2 | Only in GPL projects |
| Third-party resources (kissFFT, convhull_3d, afSTFT, md_malloc, speex_resampler, zlib) | MIT / BSD-3-Clause / permissive | Yes |

**For a commercial plugin:** You can use all core modules and the SOFA reader under the ISC license (requires acknowledgment, permits commercial use). Do NOT enable saf_tracker or saf_hades unless your plugin is also GPLv2.

### Module Structure

SAF is organized into seven core modules plus three optional modules:

| Module | Purpose | Needed for Grain Spatialization |
|--------|---------|--------------------------------|
| **saf_utilities** | FFT, linear algebra, SIMD, filtering, convolution, matrix ops | YES (foundation) |
| **saf_sh** | Spherical harmonic computation, SH rotation matrices | YES (encoding coefficients) |
| **saf_hoa** | HOA encoding, decoding (loudspeaker + binaural), channel/norm conversion | YES (encoding + decoding) |
| **saf_hrir** | HRTF loading, interpolation, ITD estimation, diffuse-field EQ | YES (binaural output) |
| **saf_vbap** | VBAP gain computation, gain tables, speaker triplets | YES (speaker output) |
| **saf_cdf4sap** | Covariance domain spatial audio processing | No (research/advanced) |
| **saf_reverb** | Room simulation, IMS reverb | Optional (if adding room sim) |
| saf_sofa_reader | Read SOFA HRTF files | Recommended (custom HRTFs) |
| saf_tracker | 3D multi-target tracking | No |
| saf_hades | Hearing-assistive binaural rendering | No |

### Dependencies: CBLAS/LAPACK

SAF requires **exactly one** CBLAS/LAPACK backend. This is the most important build dependency to get right.

| Platform | Recommended Backend | CMake Variable | Notes |
|----------|-------------------|----------------|-------|
| **macOS** | Apple Accelerate | `SAF_USE_APPLE_ACCELERATE` | Zero setup. Auto-detected. Ships with macOS. |
| **Windows** | Intel MKL (oneAPI) | `SAF_USE_INTEL_MKL_LP64` | Best performance. Free via Intel oneAPI Base Toolkit. |
| **Windows** (alt) | OpenBLAS | `SAF_USE_OPEN_BLAS_AND_LAPACKE` | Easier to set up. Slightly slower than MKL. |
| **Linux** | OpenBLAS | `SAF_USE_OPEN_BLAS_AND_LAPACKE` | `apt install libopenblas-dev liblapacke-dev` |

**macOS specifics:** On macOS, SAF automatically defaults to `SAF_USE_APPLE_ACCELERATE_LP64`. No additional setup is needed. The Accelerate framework is linked via `-framework Accelerate`.

**Windows specifics:** You MUST set the `SAF_PERFORMANCE_LIB` environment variable or CMake cache variable. MKL is strongly recommended for x86_64. For GitHub Actions CI, install Intel oneAPI Base Toolkit and set:
```
SAF_PERFORMANCE_LIB=SAF_USE_INTEL_MKL_LP64
```

### Included Resources

SAF bundles several useful third-party libraries (all permissively licensed):

- **kissFFT** (BSD-3-Clause): Default FFT implementation, used as fallback
- **convhull_3d** (MIT): Convex hull for VBAP speaker triangulation
- **afSTFT** (MIT): Alias-free Short-Time Fourier Transform filterbank
- **speex_resampler** (BSD-3-Clause): Sample rate conversion for HRIRs
- **md_malloc** (MIT): Multi-dimensional array allocation

### Default HRIR Dataset

SAF includes a built-in HRTF dataset:
- **836 directions** (`__default_N_hrir_dirs = 836`)
- **256 samples** per HRIR (`__default_hrir_len = 256`)
- **Binaural** (left + right ear)
- **Data arrays:** `__default_hrirs[836][2][256]`, `__default_hrir_dirs_deg[836][2]`

This means you get binaural rendering out of the box without requiring external SOFA files. The default dataset provides good spatial coverage for most applications.

---

## 2. CMake Integration

### Project Layout

Recommended directory structure for a JUCE + SAF plugin:

```
project-root/
  CMakeLists.txt              <- Top-level
  JUCE/                       <- JUCE 8.0.4 (submodule or local)
  Spatial_Audio_Framework/    <- SAF (submodule or local)
  plugins/
    MyPlugin/
      CMakeLists.txt
      Source/
        PluginProcessor.h
        PluginProcessor.cpp
        PluginEditor.h
        PluginEditor.cpp
```

### Option A: SAF as Git Submodule (Recommended)

```bash
cd project-root
git submodule add https://github.com/leomccormack/Spatial_Audio_Framework.git
```

### Option B: SAF as Downloaded Source

Download and extract the SAF source into your project directory. The key path is `Spatial_Audio_Framework/framework/` which contains the CMakeLists.txt that builds the `saf` library target.

### Top-Level CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.22)
project(MySpatialPlugin VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── JUCE ───────────────────────────────────────────────
add_subdirectory(JUCE)

# ─── SAF Configuration ──────────────────────────────────
# Disable SAF tests and examples (we only need the library)
set(SAF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SAF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Enable SOFA reader if you want custom HRTF loading (ISC license, safe for commercial)
set(SAF_ENABLE_SOFA_READER_MODULE OFF CACHE BOOL "" FORCE)

# Do NOT enable these unless your plugin is GPLv2:
set(SAF_ENABLE_TRACKER_MODULE OFF CACHE BOOL "" FORCE)
set(SAF_ENABLE_HADES_MODULE OFF CACHE BOOL "" FORCE)

# Platform-specific performance library selection
if(APPLE)
    # macOS: Apple Accelerate is auto-detected by SAF
    # No action needed -- SAF defaults to SAF_USE_APPLE_ACCELERATE_LP64
elseif(WIN32)
    # Windows: Set performance library
    # Option 1: Intel MKL (recommended, install via Intel oneAPI Base Toolkit)
    if(NOT DEFINED ENV{SAF_PERFORMANCE_LIB})
        set(ENV{SAF_PERFORMANCE_LIB} "SAF_USE_INTEL_MKL_LP64")
    endif()
    # Option 2: OpenBLAS (easier setup, slightly slower)
    # set(ENV{SAF_PERFORMANCE_LIB} "SAF_USE_OPEN_BLAS_AND_LAPACKE")
elseif(UNIX)
    # Linux: OpenBLAS
    if(NOT DEFINED ENV{SAF_PERFORMANCE_LIB})
        set(ENV{SAF_PERFORMANCE_LIB} "SAF_USE_OPEN_BLAS_AND_LAPACKE")
    endif()
endif()

# Add SAF framework (builds the 'saf' library target)
add_subdirectory(Spatial_Audio_Framework)

# ─── Plugin ─────────────────────────────────────────────
add_subdirectory(plugins/MyPlugin)
```

### Plugin CMakeLists.txt

```cmake
juce_add_plugin(MyPlugin
    PRODUCT_NAME "MyPlugin"
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Yoco
    PLUGIN_CODE Mypl
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Spatial" "Fx"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    # Do NOT use PLUGIN_CHANNEL_CONFIGURATIONS for multichannel
    # Use isBusesLayoutSupported() instead
)

target_sources(MyPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(MyPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MyPlugin
    PRIVATE
        # SAF library
        saf

        # JUCE modules
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

**The key line is `saf` in `target_link_libraries`.** This links the SAF static library to your plugin. SAF exposes its include directories via CMake's `target_include_directories`, so `#include "saf.h"` will work automatically.

### How SPARTA Does It (Reference)

SPARTA's build structure is:
```
SPARTA/
  CMakeLists.txt          <- Top-level config
  SDKs/
    CMakeLists.txt        <- Just: add_subdirectory(JUCE) + add_subdirectory(Spatial_Audio_Framework)
    JUCE/
    Spatial_Audio_Framework/
  audio_plugins/
    CMakeLists.txt        <- Plugin format config + add_subdirectory for each plugin
    _SPARTA_ambiENC_/
      CMakeLists.txt      <- juce_add_plugin + target_link_libraries(... saf_example_ambi_enc ...)
```

Note: SPARTA links against SAF's *example* targets (like `saf_example_ambi_enc`) rather than `saf` directly, because the SPARTA plugins use SAF's pre-built example modules that wrap the low-level SAF API into create/init/process/destroy lifecycle functions. For a custom plugin, you can either:
1. Link against `saf` directly and call low-level SAF functions (more control)
2. Use SAF's example modules as a starting point (faster to get running)

### GitHub Actions CI (Windows Cross-Compilation)

For Windows builds in GitHub Actions, add Intel oneAPI setup:

```yaml
# .github/workflows/build.yml
jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Intel oneAPI
        run: |
          # Install Intel oneAPI Base Toolkit (MKL)
          choco install intel-oneapi-base-toolkit -y
          # Or use the official Intel installer action:
          # uses: intel/setup-oneapi@v1

      - name: Configure CMake
        env:
          SAF_PERFORMANCE_LIB: SAF_USE_INTEL_MKL_LP64
        run: cmake -S . -B build -G "Visual Studio 17 2022"

      - name: Build
        run: cmake --build build --config Release
```

**Alternative for simpler Windows CI (OpenBLAS):**

```yaml
      - name: Install OpenBLAS
        run: |
          vcpkg install openblas:x64-windows

      - name: Configure CMake
        env:
          SAF_PERFORMANCE_LIB: SAF_USE_OPEN_BLAS_AND_LAPACKE
        run: cmake -S . -B build -G "Visual Studio 17 2022"
```

---

## 3. SAF Ambisonics Encoding API

### Two Approaches

SAF offers two levels of API for ambisonics encoding:

1. **Low-level:** Call `getSHreal_recur()` directly to compute spherical harmonics, then manually multiply with input signals. Maximum control, minimum overhead.

2. **High-level (example module):** Use the `ambi_enc` example module which wraps everything into create/init/process/destroy. Easier to use but less flexible for per-grain processing.

### Approach 1: Low-Level Direct SH Encoding (Recommended for Per-Grain)

For per-grain spatialization, the low-level approach is better because you need to encode each grain independently and sum into a shared ambisonics bus.

#### Key SAF Functions

```c
#include "saf.h"

// Compute real-valued spherical harmonics for given directions
// Uses single-precision recursion -- suitable for real-time
void getSHreal_recur(
    int order,          // Ambisonics order (1=FOA, 3=HOA3)
    float* dirs_rad,    // Direction(s) in radians; FLAT: nDirs x 2 (azimuth, elevation)
    int nDirs,          // Number of directions
    float* Y            // Output: SH values; FLAT: (order+1)^2 x nDirs
);

// Compute real-valued spherical harmonics (double precision, for init only)
void getSHreal(
    int order,          // Ambisonics order
    float* dirs_rad,    // Directions in radians; FLAT: nDirs x 2
    int nDirs,          // Number of directions
    float* Y            // Output: SH values; FLAT: (order+1)^2 x nDirs
);

// Generate an SH rotation matrix from a 3x3 rotation matrix
void getSHrotMtxReal(
    float R[3][3],      // 3x3 rotation matrix
    float* RotMtx,      // Output: SH rotation matrix; FLAT: nSH x nSH
    int L               // Ambisonics order
);

// Convert between channel orderings (ACN <-> FuMa)
void convertHOAChannelConvention(
    float* insig,       // Input signals; FLAT: nSH x signalLength
    int order,
    int signalLength,
    HOA_CH_ORDER inConvention,
    HOA_CH_ORDER outConvention
);

// Convert between normalizations (N3D <-> SN3D <-> FuMa)
void convertHOANormConvention(
    float* insig,
    int order,
    int signalLength,
    HOA_NORM inConvention,
    HOA_NORM outConvention
);
```

#### Per-Grain Encoding Wrapper for JUCE

```cpp
// SpatialGrainEncoder.h
#pragma once
#include <JuceHeader.h>

extern "C" {
#include "saf.h"
}

class SpatialGrainEncoder
{
public:
    static constexpr int ambiOrder = 3;                        // HOA3
    static constexpr int numAmbiChannels = (ambiOrder + 1) * (ambiOrder + 1); // 16

    SpatialGrainEncoder() = default;

    // Call once to compute SH coefficients for a grain's position.
    // azimuth: radians, 0=front, pi/2=left
    // elevation: radians, 0=horizon, pi/2=up
    void setPosition (float azimuthRad, float elevationRad)
    {
        float dir[2] = { azimuthRad, elevationRad };

        // SAF's getSHreal_recur computes SH coefficients for one direction
        // Output: shCoeffs[numAmbiChannels] for this direction
        // The function uses single-precision recursion, safe for real-time
        getSHreal_recur (ambiOrder, dir, 1, shCoeffs.data());
    }

    // Encode a mono grain sample into the ambisonics accumulation buffer.
    // Call per-sample. ambiOutput must be a 16-channel buffer that
    // accumulates contributions from all grains.
    void encodeSample (float monoSample, float* ambiOutput) const
    {
        for (int ch = 0; ch < numAmbiChannels; ++ch)
            ambiOutput[ch] += monoSample * shCoeffs[ch];
    }

    // Encode a block of mono grain samples into ambisonics buffer.
    // More efficient: avoids per-sample function call overhead.
    void encodeBlock (const float* monoInput, int numSamples,
                      juce::AudioBuffer<float>& ambiBuffer, int startSample) const
    {
        for (int ch = 0; ch < numAmbiChannels; ++ch)
        {
            float* dest = ambiBuffer.getWritePointer (ch) + startSample;
            float coeff = shCoeffs[ch];

            for (int s = 0; s < numSamples; ++s)
                dest[s] += monoInput[s] * coeff;
        }
    }

    const std::array<float, numAmbiChannels>& getCoeffs() const { return shCoeffs; }

private:
    std::array<float, numAmbiChannels> shCoeffs {};
};
```

#### Integration with JUCE processBlock

```cpp
// In PluginProcessor.h
class MyPluginProcessor : public juce::AudioProcessor
{
    static constexpr int maxGrains = 64;
    static constexpr int ambiOrder = 3;
    static constexpr int numAmbiCh = (ambiOrder + 1) * (ambiOrder + 1); // 16

    struct Grain
    {
        bool active = false;
        float azimuth = 0.0f;       // radians
        float elevation = 0.0f;     // radians
        float distance = 1.0f;      // meters
        SpatialGrainEncoder encoder;
        // ... other grain state (position in source buffer, envelope, etc.)
    };

    std::array<Grain, maxGrains> grains;
    juce::AudioBuffer<float> ambiBuffer;  // internal HOA3 buffer (16 channels)
};

// In PluginProcessor.cpp
void MyPluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ambiBuffer.setSize (numAmbiCh, samplesPerBlock);
}

void MyPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midi)
{
    int numSamples = buffer.getNumSamples();

    // 1. Clear the ambisonics accumulation buffer
    ambiBuffer.clear();

    // 2. For each active grain, encode into ambisonics bus
    for (auto& grain : grains)
    {
        if (! grain.active)
            continue;

        // Update SH coefficients when grain position changes
        // (do this per grain onset, or per block for moving grains)
        grain.encoder.setPosition (grain.azimuth, grain.elevation);

        // Synthesize grain audio into a temporary mono buffer
        // (your grain synthesis code here)
        float grainMono[numSamples];
        synthesizeGrain (grain, grainMono, numSamples);

        // Apply distance attenuation
        float distGain = 1.0f / std::max (grain.distance, 0.1f);
        for (int s = 0; s < numSamples; ++s)
            grainMono[s] *= distGain;

        // Encode into ambisonics bus (accumulates)
        grain.encoder.encodeBlock (grainMono, numSamples, ambiBuffer, 0);
    }

    // 3. Copy ambisonics bus to output
    //    (or decode to binaural/speakers -- see Section 4)
    auto outputLayout = getBusesLayout().getMainOutputChannelSet();
    int outOrder = outputLayout.getAmbisonicOrder();

    if (outOrder >= 1)
    {
        // Direct ambisonics output
        int outChannels = std::min (buffer.getNumChannels(), numAmbiCh);
        for (int ch = 0; ch < outChannels; ++ch)
            buffer.copyFrom (ch, 0, ambiBuffer, ch, 0, numSamples);
    }
    else if (outputLayout == juce::AudioChannelSet::stereo())
    {
        // Binaural decode (see Section 4)
        decodeBinaural (ambiBuffer, buffer, numSamples);
    }
}
```

### Approach 2: High-Level SAF Example Module

For rapid prototyping, you can use SAF's `ambi_enc` example module directly:

```cpp
extern "C" {
#include "ambi_enc.h"
}

class MyPluginProcessor : public juce::AudioProcessor
{
    void* hAmbiEnc = nullptr;  // SAF encoder handle

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        ambi_enc_create (&hAmbiEnc);
        ambi_enc_init (hAmbiEnc, (int) sampleRate);
        ambi_enc_setOutputOrder (hAmbiEnc, SH_ORDER_THIRD);
        ambi_enc_setChOrder (hAmbiEnc, CH_ACN);
        ambi_enc_setNormType (hAmbiEnc, NORM_SN3D);
        ambi_enc_setNumSources (hAmbiEnc, 64); // up to 128 supported

        setLatencySamples (ambi_enc_getProcessingDelay());
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        int nIns = buffer.getNumChannels();
        int nOuts = ambi_enc_getNSHrequired (hAmbiEnc); // e.g. 16 for 3rd order
        int nSamples = buffer.getNumSamples();

        // Update source positions
        for (int i = 0; i < numActiveSources; ++i)
        {
            ambi_enc_setSourceAzi_deg (hAmbiEnc, i, sourcesAzi[i]);
            ambi_enc_setSourceElev_deg (hAmbiEnc, i, sourcesElev[i]);
        }

        // Process
        const float* const* inputs = buffer.getArrayOfReadPointers();
        float* const* outputs = buffer.getArrayOfWritePointers();
        ambi_enc_process (hAmbiEnc, inputs, outputs, nIns, nOuts, nSamples);
    }

    void releaseResources() override
    {
        ambi_enc_destroy (&hAmbiEnc);
    }
};
```

**Tradeoff:** The `ambi_enc` example module handles crossfading, channel ordering conversion, and normalization internally. However, it processes all sources in a single call (each input channel = one source), which doesn't map perfectly to a grain engine where grains are dynamically created/destroyed. The low-level approach (Approach 1) gives you more control for per-grain processing.

### Memory Management Considerations

SAF uses `malloc`/`free` internally for its example modules. Important notes:

- **`ambi_enc_create()`** allocates internal state including FFT buffers. Call this in `prepareToPlay()`, NOT in the constructor.
- **`ambi_enc_destroy()`** frees all internal memory. Call in `releaseResources()`.
- **`getSHreal_recur()`** uses static internal buffers (thread-local). Safe to call from the audio thread for up to 7th order.
- **`getSHreal()`** uses double-precision and is NOT intended for the audio thread. Use only during initialization.
- The low-level SH functions (`getSHreal_recur`) do NOT allocate memory at runtime -- they use stack/static buffers. This makes them safe for real-time use.

---

## 4. SAF Binaural Decoding API

### Overview

SAF provides two approaches to binaural decoding from ambisonics:

1. **Frequency-domain decoder matrices:** `getBinauralAmbiDecoderMtx()` -- generates decoder matrices per frequency band
2. **Time-domain decoder filters:** `getBinauralAmbiDecoderFilters()` -- generates FIR filters for convolution

The SPARTA `binauraliser` example module wraps everything into a high-level create/init/process/destroy API.

### Approach 1: Using the Binauraliser Example Module (Recommended)

This is the fastest path to working binaural output:

```cpp
extern "C" {
#include "binauraliser.h"
}

class BinauralDecoderWrapper
{
public:
    void create()
    {
        binauraliser_create (&hBin);
    }

    void init (int sampleRate)
    {
        binauraliser_init (hBin, sampleRate);

        // Use default HRIRs (836 directions, 256 samples)
        binauraliser_setUseDefaultHRIRsflag (hBin, 1);

        // Or load a custom SOFA file (requires SAF_ENABLE_SOFA_READER_MODULE):
        // binauraliser_setSofaFilePath (hBin, "/path/to/hrtf.sofa");

        // Set up for ambisonics input (e.g., 16 channels = HOA3)
        binauraliser_setNumSources (hBin, 16);

        // Configure source directions to match ambisonics virtual loudspeaker layout
        // (The binauraliser treats each input as a positioned source)
        // For ambisonics decoding, set source positions to a t-design or
        // uniformly distributed points on the sphere
        setupAmbisonicsSources();

        // Enable diffuse-field EQ for better timbral quality
        binauraliser_setEnableHRIRsDiffuseEQ (hBin, 1);

        // Initialize the codec (computes decoder matrices -- NOT real-time safe)
        binauraliser_initCodec (hBin);
    }

    void process (const float* const* ambiInputs, float* const* stereoOutputs,
                  int nInputs, int nOutputs, int nSamples)
    {
        binauraliser_process (hBin, ambiInputs, stereoOutputs,
                              nInputs, nOutputs, nSamples);
    }

    int getProcessingDelay() const
    {
        return binauraliser_getProcessingDelay();
    }

    void destroy()
    {
        binauraliser_destroy (&hBin);
        hBin = nullptr;
    }

private:
    void* hBin = nullptr;

    void setupAmbisonicsSources()
    {
        // For HOA3 (16 channels), use a 16-point t-design or similar
        // These are virtual loudspeaker positions for ambisonics decoding
        // SAF's ambi_dec example has presets for this
        // Alternatively, use equally-spaced directions on the sphere
    }
};
```

### Approach 2: Using SAF's HOA Binaural Decoder Directly

For more control, use the lower-level HOA module functions:

```cpp
extern "C" {
#include "saf.h"
}

class AmbisonicsBinauralDecoder
{
public:
    static constexpr int ambiOrder = 3;
    static constexpr int numSH = (ambiOrder + 1) * (ambiOrder + 1); // 16

    void init (int sampleRate, int fftSize)
    {
        this->fs = (float) sampleRate;
        this->fftSz = fftSize;
        int nBands = fftSz / 2 + 1;

        // 1. Load HRTFs (use default dataset)
        int nDirs = __default_N_hrir_dirs;    // 836
        int hrirLen = __default_hrir_len;     // 256

        // 2. Convert HRIRs to frequency-domain HRTFs
        int nHRTFdirs = nDirs;
        hrtfs = (float_complex*) malloc (nHRTFdirs * 2 * nBands * sizeof (float_complex));
        HRIRs2HRTFs ((float*) __default_hrirs, nHRTFdirs, hrirLen, fftSz, hrtfs);

        // 3. Estimate ITDs for phase correction
        itds = (float*) malloc (nDirs * sizeof (float));
        estimateITDs ((float*) __default_hrirs, nDirs, hrirLen, sampleRate, itds);

        // 4. Compute binaural decoder matrix
        float* freqVector = (float*) malloc (nBands * sizeof (float));
        for (int b = 0; b < nBands; ++b)
            freqVector[b] = (float) b * fs / (float) fftSz;

        decMtx = (float_complex*) malloc (nBands * 2 * numSH * sizeof (float_complex));
        getBinauralAmbiDecoderMtx (hrtfs,
                                   (float*) __default_hrir_dirs_deg,
                                   nDirs,
                                   nBands,
                                   BINAURAL_DECODER_MAGLS, // MagLS decoder
                                   ambiOrder,
                                   freqVector,
                                   itds,
                                   NULL, // weights (NULL = uniform)
                                   1,    // enableDiffCM (diffuse covariance matching)
                                   1,    // enableMaxrE (max-rE weighting)
                                   decMtx);

        free (freqVector);
    }

    // Process one block of ambisonics input to binaural stereo output
    // This requires STFT processing (overlap-save or afSTFT)
    void process (const juce::AudioBuffer<float>& ambiInput,
                  juce::AudioBuffer<float>& stereoOutput,
                  int numSamples)
    {
        // Apply decoder matrix in frequency domain per band:
        // stereo[band] = decMtx[band] * ambi[band]
        // (Implementation requires STFT framework -- see SPARTA binauraliser for reference)
    }

    ~AmbisonicsBinauralDecoder()
    {
        free (hrtfs);
        free (itds);
        free (decMtx);
    }

private:
    float fs = 48000.0f;
    int fftSz = 512;
    float_complex* hrtfs = nullptr;
    float* itds = nullptr;
    float_complex* decMtx = nullptr;
};
```

### SAF Binaural Decoder Methods

SAF supports several binaural decoder methods via the `BINAURAL_AMBI_DECODER_METHODS` enum:

| Method | Description | Quality | CPU Cost |
|--------|-------------|---------|----------|
| `BINAURAL_DECODER_DEFAULT` | SAD (Sampling Ambisonic Decoder) | Basic | Low |
| `BINAURAL_DECODER_LS` | Least-squares | Good | Medium |
| `BINAURAL_DECODER_LSDIFFEQ` | Least-squares with diffuse-field EQ | Good+ | Medium |
| `BINAURAL_DECODER_SPR` | Spatial resampling | Good | Medium |
| `BINAURAL_DECODER_TA` | Time-alignment | Good | Medium |
| `BINAURAL_DECODER_MAGLS` | Magnitude Least Squares | Best | Medium |

**Recommendation:** Use `BINAURAL_DECODER_MAGLS` with `enableMaxrE=1` and `enableDiffCM=1` for the best perceptual quality at HOA3.

### HRTF Loading and Interpolation

```c
// Interpolate HRTFs for arbitrary directions using VBAP gain tables
void interpHRTFs(
    float_complex* hrtfs,        // Original HRTFs; FLAT: N_hrtf_dirs x 2 x N_bands
    float* itds,                 // ITDs per direction; N_hrtf_dirs x 1
    float* freqVector,           // Frequency values; N_bands x 1
    float* interp_table,         // VBAP gain table for interpolation
    int N_hrtf_dirs,             // Number of HRTF measurement directions
    int N_bands,                 // Number of frequency bands
    int N_interp_dirs,           // Number of interpolation directions
    float_complex* hrtf_interp   // Output: interpolated HRTFs
);

// Resample HRIRs to match your plugin's sample rate
void resampleHRIRs(
    float* hrirs_in,             // Input HRIRs
    int hrirs_N_dirs,
    int hrirs_in_len,
    int hrirs_in_fs,             // Input sample rate
    int hrirs_out_fs,            // Desired output sample rate
    int padToNextPow2,           // 1 = pad output length to next power of 2
    float** hrirs_out,           // Output: resampled HRIRs (allocated internally)
    int* hrirs_out_len           // Output: new HRIR length
);
```

### Practical Integration: Ambisonics-to-Binaural in processBlock

The recommended approach for a grain spatialization plugin:

```cpp
void MyPluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& midi)
{
    int numSamples = buffer.getNumSamples();

    // 1. Clear ambisonics accumulation buffer (16 channels)
    ambiBuffer.clear();

    // 2. Encode all active grains into ambisonics bus
    for (auto& grain : grains)
    {
        if (! grain.active) continue;
        grain.encoder.setPosition (grain.azimuth, grain.elevation);
        float grainMono[numSamples]; // or use a pre-allocated buffer
        synthesizeGrain (grain, grainMono, numSamples);
        float distGain = 1.0f / std::max (grain.distance, 0.1f);
        for (int s = 0; s < numSamples; ++s)
            grainMono[s] *= distGain;
        grain.encoder.encodeBlock (grainMono, numSamples, ambiBuffer, 0);
    }

    // 3. Decode to output format
    if (outputIsStereo)
    {
        // Binaural decode using SAF binauraliser
        const float* const* ambiPtrs = ambiBuffer.getArrayOfReadPointers();
        float* const* outPtrs = buffer.getArrayOfWritePointers();
        binauraliser_process (hBin, ambiPtrs, outPtrs,
                              numAmbiCh, 2, numSamples);
    }
    else if (outputIsAmbisonics)
    {
        // Direct copy to ambisonics output
        for (int ch = 0; ch < numAmbiCh; ++ch)
            buffer.copyFrom (ch, 0, ambiBuffer, ch, 0, numSamples);
    }
}
```

---

## 5. SAF VBAP API

### Overview

SAF's `saf_vbap` module provides comprehensive VBAP implementation for both 2D and 3D speaker arrays. For per-grain panning to speaker arrays, you precompute a gain table and look up gains per grain direction.

### Key Functions

#### Speaker Layout Analysis

```c
// Find loudspeaker triangles (3D) via convex hull
void findLsTriplets(
    float* ls_dirs_deg,         // Speaker positions; FLAT: L x 2 (az, el in degrees)
    int L,                      // Number of speakers
    int omitLargeTriangles,     // 1 = remove triangles > 120 degrees
    float** out_vertices,       // Output: speaker Cartesian coords; FLAT: L x 3
    int* numOutVertices,
    int** out_faces,            // Output: triangle indices; FLAT: nFaces x 3
    int* numOutFaces
);

// Compute inverted loudspeaker matrices for each triangle
void invertLsMtx3D(
    float* U_spkr,              // Speaker Cartesian unit vectors; FLAT: L x 3
    int* ls_groups,             // Triangle indices; FLAT: nFaces x 3
    int N_group,                // Number of triangles
    float** layoutInvMtx        // Output: inverted matrices; FLAT: nFaces x 9
);
```

#### Gain Table Generation

```c
// Generate a full 3D VBAP gain table at specified angular resolution
void generateVBAPgainTable3D(
    float* ls_dirs_deg,         // Speaker positions; FLAT: L x 2
    int L,                      // Number of speakers
    int az_res_deg,             // Azimuth resolution (e.g. 1 degree)
    int el_res_deg,             // Elevation resolution (e.g. 1 degree)
    int omitLargeTriangles,     // 0 or 1
    int enableDummies,          // 1 = add imaginary speakers to fill gaps
    float spread,               // Spreading in degrees (0 = point source)
    float** gtable,             // Output: gain table; FLAT: N_gtable x L
    int* N_gtable,              // Output: number of table entries
    int* nTriangles             // Output: number of speaker triangles
);

// Generate gain table for specific source directions
void generateVBAPgainTable3D_srcs(
    float* src_dirs_deg,        // Source directions; FLAT: S x 2
    int S,                      // Number of sources
    float* ls_dirs_deg,         // Speaker positions; FLAT: L x 2
    int L,                      // Number of speakers
    int omitLargeTriangles,
    int enableDummies,
    float spread,
    float** gtable,             // Output: gain table; FLAT: S x L
    int* N_gtable,
    int* nTriangles
);
```

#### Direct VBAP Computation

```c
// Compute VBAP gains for specific source directions
void vbap3D(
    float* src_dirs,            // Source directions in degrees; FLAT: src_num x 2
    int src_num,                // Number of sources
    int ls_num,                 // Number of speakers
    int* ls_groups,             // Triangle indices; FLAT: nFaces x 3
    int nFaces,                 // Number of triangles
    float spread,               // Spreading in degrees
    float* layoutInvMtx,        // Inverted matrices; FLAT: nFaces x 9
    float** GainMtx             // Output: gains; FLAT: src_num x ls_num
);
```

### Per-Grain VBAP Integration

```cpp
class VBAPPanner
{
public:
    void init (const std::vector<std::pair<float, float>>& speakerPositionsDeg)
    {
        numSpeakers = (int) speakerPositionsDeg.size();

        // Flatten speaker positions for SAF (expects FLAT array: L x 2)
        std::vector<float> lsDirs (numSpeakers * 2);
        for (int i = 0; i < numSpeakers; ++i)
        {
            lsDirs[i * 2]     = speakerPositionsDeg[i].first;  // azimuth
            lsDirs[i * 2 + 1] = speakerPositionsDeg[i].second; // elevation
        }

        // Generate VBAP gain table at 1-degree resolution
        float* gtable = nullptr;
        int nTableEntries = 0;
        int nTriangles = 0;

        generateVBAPgainTable3D (lsDirs.data(), numSpeakers,
                                  1,  // 1-degree azimuth resolution
                                  1,  // 1-degree elevation resolution
                                  1,  // omit large triangles
                                  0,  // no dummy speakers
                                  0.0f, // no spreading
                                  &gtable, &nTableEntries, &nTriangles);

        // Store gain table
        gainTable.assign (gtable, gtable + nTableEntries * numSpeakers);
        numTableEntries = nTableEntries;

        free (gtable); // SAF allocates with malloc
    }

    // Look up VBAP gains for a given direction
    void getGains (float azimuthDeg, float elevationDeg, float* gains) const
    {
        // Quantize direction to table index
        int azIdx = ((int) std::round (azimuthDeg) + 180) % 360;
        int elIdx = ((int) std::round (elevationDeg) + 90);
        elIdx = std::clamp (elIdx, 0, 180);
        int idx = azIdx * 181 + elIdx; // assumes 1-degree resolution

        if (idx >= 0 && idx < numTableEntries)
        {
            for (int s = 0; s < numSpeakers; ++s)
                gains[s] = gainTable[idx * numSpeakers + s];
        }
    }

    // Apply gains to a mono sample, accumulating into multichannel output
    void panSample (float monoSample, const float* gains,
                    float** channelOutputs, int sampleIndex) const
    {
        for (int s = 0; s < numSpeakers; ++s)
            channelOutputs[s][sampleIndex] += monoSample * gains[s];
    }

private:
    int numSpeakers = 0;
    int numTableEntries = 0;
    std::vector<float> gainTable;
};
```

### VBAP Spreading (MDAP)

SAF supports MDAP (Multiple Direction Amplitude Panning) for spreading a source across multiple directions:

```c
// Get spread source directions for a given source position
void getSpreadSrcDirs3D(
    float src_azi_rad,          // Source azimuth in radians
    float src_elev_rad,         // Source elevation in radians
    float spread,               // Spread in degrees
    int num_src,                // Number of auxiliary sources per ring
    int num_rings_3d,           // Number of concentric rings
    float* U_spread             // Output: spread directions (Cartesian)
);
```

Use this when you want grains to have spatial "width" rather than being point sources. Higher spread values create a wider perceived source.

---

## 6. SPARTA Plugins as Reference

### Overview

SPARTA is a suite of 20+ audio plugins built with JUCE + SAF. It is the definitive reference for how to integrate SAF into JUCE plugins.

- **Repository:** https://github.com/leomccormack/SPARTA
- **License:** GPLv3 (plugins), ISC (underlying SAF core)
- **Latest release:** v1.8.2 (January 2026)
- **JUCE version used:** Bundled in the `SDKs/JUCE/` directory

### Relevant SPARTA Plugins for Grain Spatialization

| Plugin | What It Does | What to Learn |
|--------|--------------|---------------|
| **AmbiENC** | Encodes up to 128 mono inputs to HOA (up to 10th order) | How SAF handles multi-source ambisonics encoding |
| **AmbiBIN** | Decodes ambisonics to binaural stereo | Binaural decoder setup with custom/default HRTFs |
| **AmbiDEC** | Decodes ambisonics to loudspeaker array | Loudspeaker decoder matrix computation |
| **Panner** | VBAP panner for up to 128 inputs | VBAP gain table generation and speaker layout setup |
| **Binauraliser** | Per-source HRTF binauralization (up to 128 sources) | Per-source binaural rendering with SOFA HRTFs |
| **Spreader** | Spreads mono sources spatially | Source spreading / MDAP techniques |
| **Rotator** | Rotates SH signal (yaw/pitch/roll) | SH rotation matrix application |

### Key Architectural Patterns from SPARTA

#### Pattern 1: SAF Handle Lifecycle

Every SPARTA plugin follows this pattern:

```cpp
// prepareToPlay: create and initialize SAF handle
void prepareToPlay (double sampleRate, int samplesPerBlock) override
{
    ambi_enc_create (&hSAF);
    ambi_enc_init (hSAF, (int) sampleRate);
    setLatencySamples (ambi_enc_getProcessingDelay());
}

// processBlock: delegate to SAF
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
    ambi_enc_process (hSAF, inputs, outputs, nIn, nOut, nSamples);
}

// releaseResources: destroy SAF handle
void releaseResources() override
{
    ambi_enc_destroy (&hSAF);
}
```

#### Pattern 2: Block Adapter for Variable Block Sizes

SAF example modules process fixed-size frames internally. SPARTA uses a "block adapter" to bridge JUCE's variable block sizes with SAF's fixed frame size:

```cpp
// SPARTA wraps SAF processing in a lambda that gets called with
// the correct frame size by the block adapter:
blockAdapter->processBlock (buffer,
    [this](const float* const* inFrame, float* const* outFrame,
           int numIns, int numOuts, int frameSize)
    {
        binauraliser_process (hBin, inFrame, outFrame,
                              numIns, numOuts, frameSize);
    });
```

The block adapter accumulates input samples until it has enough for SAF's internal frame size, then calls the lambda. This handles cases where the DAW's block size differs from SAF's expected frame size.

#### Pattern 3: Codec Initialization on Background Thread

SAF decoder initialization (computing decoder matrices from HRTFs) is computationally expensive and NOT real-time safe. SPARTA calls `initCodec()` on a background thread and checks `getCodecStatus()` before processing:

```cpp
// Called when parameters change (e.g., HRTF file, decoder order)
void parameterChanged()
{
    // Launch codec init on background thread
    std::thread ([this]() {
        binauraliser_initCodec (hBin);
    }).detach();
}

// In processBlock: check if codec is ready
void processBlock (...)
{
    if (binauraliser_getCodecStatus (hBin) != CODEC_STATUS_INITIALISED)
        return; // Output silence while codec initializes

    // Safe to process
    binauraliser_process (...);
}
```

#### Pattern 4: SAF Internal Processing Flow (ambi_enc_process)

The ambi_enc example module internally:
1. Copies input audio into internal frame buffers
2. Recomputes spherical harmonics via `getSHreal_recur()` when source positions change
3. Applies source gains to input frames
4. Performs matrix multiplication via `cblas_sgemm()`: `output = Y^T * input` where Y is the SH coefficient matrix
5. Crossfades between old and new coefficients to avoid clicks
6. Applies optional post-scaling (`1/sqrt(nSources)`)
7. Converts channel ordering/normalization if needed (ACN/SN3D/FuMa)
8. Copies to output buffers

This is the gold standard for understanding how SAF processes ambisonics encoding internally.

---

## 7. Alternative: Lightweight Self-Contained Implementation

### When to Use Self-Contained Instead of SAF

| Factor | SAF | Self-Contained |
|--------|-----|----------------|
| Build complexity | Higher (CBLAS/LAPACK dependency) | Minimal (no external deps) |
| Feature coverage | Comprehensive (VBAP, HOA, binaural, room sim) | Limited to what you implement |
| Encoding quality | Verified, tested, research-grade | Must verify your own math |
| Binaural decoding | Full MagLS decoder with HRTFs | Basic convolution only |
| CI/CD complexity | Must install BLAS on all platforms | Just C++ |
| Performance | BLAS-accelerated matrix ops | Manual SIMD or scalar |
| License | ISC (core) | Your own license |

**Choose SAF if:** You need binaural decoding, VBAP, SOFA file loading, or high-order ambisonics (4th+). The build complexity is worth it.

**Choose self-contained if:** You only need HOA encoding up to 3rd order and will decode to binaural using JUCE's `dsp::Convolution` with pre-baked HRIR filters.

### Self-Contained HOA3 Encoder (No External Dependencies)

This implementation requires zero external libraries. The spherical harmonic math is hardcoded for orders 0-3 (16 channels, ACN/SN3D):

```cpp
// SelfContainedAmbiEncoder.h
#pragma once
#include <array>
#include <cmath>

class SelfContainedAmbiEncoder
{
public:
    static constexpr int numChannels = 16; // (3+1)^2

    // Precompute SH coefficients for a given direction
    // azimuth: radians, 0=front, pi/2=left, -pi/2=right, pi=back
    // elevation: radians, 0=horizon, pi/2=up, -pi/2=down
    void setPosition (float azimuth, float elevation)
    {
        float cosE = std::cos (elevation);
        float sinE = std::sin (elevation);
        float cosA = std::cos (azimuth);
        float sinA = std::sin (azimuth);
        float cos2A = std::cos (2.0f * azimuth);
        float sin2A = std::sin (2.0f * azimuth);
        float cos3A = std::cos (3.0f * azimuth);
        float sin3A = std::sin (3.0f * azimuth);
        float cosE2 = cosE * cosE;
        float sinE2 = sinE * sinE;
        float cosE3 = cosE2 * cosE;

        // SN3D normalization constants
        constexpr float kSqrt3_2  = 0.86602540378f;  // sqrt(3)/2
        constexpr float kSqrt15_2 = 1.93649167310f;  // sqrt(15)/2
        constexpr float kSqrt5_8  = 0.79056941504f;  // sqrt(5/8)
        constexpr float kSqrt3_8  = 0.61237243570f;  // sqrt(3/8)

        // Order 0 (ACN 0)
        coeffs[0]  = 1.0f;                                           // W

        // Order 1 (ACN 1-3)
        coeffs[1]  = sinA * cosE;                                    // Y
        coeffs[2]  = sinE;                                           // Z
        coeffs[3]  = cosA * cosE;                                    // X

        // Order 2 (ACN 4-8)
        coeffs[4]  = kSqrt3_2 * sin2A * cosE2;                      // V
        coeffs[5]  = kSqrt3_2 * sinA * sinE * cosE;                  // T
        coeffs[6]  = 0.5f * (3.0f * sinE2 - 1.0f);                  // R
        coeffs[7]  = kSqrt3_2 * cosA * sinE * cosE;                  // S
        coeffs[8]  = kSqrt3_2 * cos2A * cosE2;                      // U

        // Order 3 (ACN 9-15)
        coeffs[9]  = kSqrt5_8 * sin3A * cosE3;                      // Q
        coeffs[10] = kSqrt15_2 * sin2A * cosE2 * sinE;              // O
        coeffs[11] = kSqrt3_8 * sinA * cosE * (5.0f * sinE2 - 1.0f); // M
        coeffs[12] = 0.5f * sinE * (5.0f * sinE2 - 3.0f);          // K
        coeffs[13] = kSqrt3_8 * cosA * cosE * (5.0f * sinE2 - 1.0f); // L
        coeffs[14] = kSqrt15_2 * cos2A * cosE2 * sinE;              // N
        coeffs[15] = kSqrt5_8 * cos3A * cosE3;                      // P
    }

    // Encode one sample, accumulating into ambisonics output
    void encodeSample (float sample, float* ambiOut) const
    {
        for (int ch = 0; ch < numChannels; ++ch)
            ambiOut[ch] += sample * coeffs[ch];
    }

    // Encode a block of samples
    void encodeBlock (const float* input, int numSamples,
                      float** ambiOut) const
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float c = coeffs[ch];
            float* out = ambiOut[ch];
            for (int s = 0; s < numSamples; ++s)
                out[s] += input[s] * c;
        }
    }

    const std::array<float, numChannels>& getCoeffs() const { return coeffs; }

private:
    std::array<float, numChannels> coeffs {};
};
```

### Self-Contained Binaural Decoder Using JUCE dsp::Convolution

If you want to avoid SAF's binaural decoder entirely, you can bake HRIR filters for each ambisonics channel and convolve using JUCE's built-in `dsp::Convolution`:

```cpp
// SelfContainedBinauralDecoder.h
#pragma once
#include <JuceHeader.h>

class SelfContainedBinauralDecoder
{
public:
    static constexpr int ambiOrder = 3;
    static constexpr int numSH = 16;

    void prepare (double sampleRate, int blockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32) blockSize;
        spec.numChannels = 1;

        // Create one convolver per ambisonics channel per ear (32 total)
        for (int ch = 0; ch < numSH; ++ch)
        {
            convolversL[ch].prepare (spec);
            convolversR[ch].prepare (spec);
        }

        // Load pre-computed binaural decoder IRs
        // These must be pre-computed offline using SAF or Matlab
        // Each IR decodes one SH channel to one ear
        loadDecoderIRs (sampleRate);
    }

    void process (const juce::AudioBuffer<float>& ambiInput,
                  juce::AudioBuffer<float>& stereoOutput, int numSamples)
    {
        stereoOutput.clear();

        juce::AudioBuffer<float> tempMono (1, numSamples);

        for (int ch = 0; ch < numSH; ++ch)
        {
            // Convolve ambi channel with left ear decoder IR
            tempMono.copyFrom (0, 0, ambiInput, ch, 0, numSamples);
            juce::dsp::AudioBlock<float> block (tempMono);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            convolversL[ch].process (ctx);

            // Accumulate into left output
            stereoOutput.addFrom (0, 0, tempMono, 0, 0, numSamples);

            // Convolve ambi channel with right ear decoder IR
            tempMono.copyFrom (0, 0, ambiInput, ch, 0, numSamples);
            juce::dsp::AudioBlock<float> block2 (tempMono);
            juce::dsp::ProcessContextReplacing<float> ctx2 (block2);
            convolversR[ch].process (ctx2);

            // Accumulate into right output
            stereoOutput.addFrom (1, 0, tempMono, 0, 0, numSamples);
        }
    }

private:
    std::array<juce::dsp::Convolution, numSH> convolversL;
    std::array<juce::dsp::Convolution, numSH> convolversR;

    void loadDecoderIRs (double sampleRate)
    {
        // Load pre-baked decoder impulse responses from binary data
        // These IRs are computed offline:
        //   1. Use SAF or Matlab to compute MagLS binaural decoder
        //   2. Export 16 left-ear + 16 right-ear IRs as WAV/raw files
        //   3. Embed as BinaryData in JUCE
        //
        // Example for loading from BinaryData:
        // for (int ch = 0; ch < numSH; ++ch)
        // {
        //     convolversL[ch].loadImpulseResponse (
        //         BinaryData::decoder_L_ch00_wav + ch * irDataSize,
        //         irDataSize,
        //         juce::dsp::Convolution::Stereo::no,
        //         juce::dsp::Convolution::Trim::yes,
        //         juce::dsp::Convolution::Normalise::no);
        // }
    }
};
```

### Tradeoffs Summary

| Aspect | SAF Integration | Self-Contained |
|--------|----------------|----------------|
| Encoding (HOA3) | `getSHreal_recur()` -- verified, supports up to 10th order | Manual SH math -- 16 channels only, must verify |
| Binaural decoding | Full MagLS with diffuse-field matching | Pre-baked convolution IRs, fixed quality |
| HRTF flexibility | Runtime SOFA loading, 836-direction default | Offline baking only |
| VBAP | Full implementation with spreading | Must implement from scratch |
| Build deps | CBLAS/LAPACK (Accelerate on macOS) | None |
| Binary size | ~2-4 MB added | ~500 KB for baked IRs |
| Maintenance | SAF handles edge cases | You handle everything |

### Hybrid Approach (Recommended)

Use SAF for binaural decoding (the hard part) and self-contained SH math for per-grain encoding (the easy part):

1. Self-contained HOA3 encoder: Zero dependencies, trivial to maintain, 16 multiplies per grain per sample
2. SAF binauraliser for binaural output: Proven, high-quality, handles the complex HRTF processing
3. Self-contained VBAP if needed: The math is simple enough for a basic 2D or 3D implementation

This gives you the best of both worlds: no build dependency for the hot path (encoding), but SAF's quality for the decoder.

---

## 8. Build and Test Verification

### Step 1: Verify SAF Builds and Links

After setting up CMake, run:

```bash
mkdir build && cd build
cmake .. -G Ninja
ninja MyPlugin_VST3 MyPlugin_AU
```

**Common build errors and fixes:**

| Error | Cause | Fix |
|-------|-------|-----|
| `saf.h: No such file` | SAF include paths not propagated | Check `add_subdirectory` path for SAF |
| `Undefined symbols: cblas_*` | BLAS/LAPACK not found | Set `SAF_PERFORMANCE_LIB` correctly |
| `Apple Accelerate not found` | Wrong SAF path on macOS | Ensure SAF auto-detects Accelerate (check CMake output) |
| `multiple definition of __default_hrirs` | SAF resources linked twice | Ensure `saf` appears only once in `target_link_libraries` |
| `getSHreal_recur undeclared` | Missing `extern "C"` wrapper | Wrap SAF includes: `extern "C" { #include "saf.h" }` |

### Step 2: Simple Encoding Verification Test

Create a test that verifies SAF spherical harmonics match expected values:

```cpp
// Unit test or debug code
void verifySAFEncoding()
{
    // Test: source at front center (azimuth=0, elevation=0)
    float dir[2] = { 0.0f, 0.0f }; // (azimuth_rad, elevation_rad)
    float Y[16]; // HOA3 = 16 channels
    getSHreal_recur (3, dir, 1, Y);

    // Expected values for front center (az=0, el=0) in SN3D:
    // ACN 0 (W):  1.0          (omnidirectional)
    // ACN 1 (Y):  0.0          (sin(0) * cos(0) = 0)
    // ACN 2 (Z):  0.0          (sin(0) = 0)
    // ACN 3 (X):  1.0          (cos(0) * cos(0) = 1)
    // ACN 4 (V):  0.0          (sin(0) = 0)
    // ACN 5 (T):  0.0
    // ACN 6 (R): -0.5          (0.5 * (3*0 - 1) = -0.5)
    // ACN 7 (S):  0.0
    // ACN 8 (U):  sqrt(3)/2 = 0.866  (cos(0)*cos^2(0))
    // ACN 9-15: various, mostly 0 except ACN 15

    DBG ("Front center SH coefficients:");
    for (int i = 0; i < 16; ++i)
        DBG ("  ACN " + juce::String (i) + ": " + juce::String (Y[i], 4));

    // Verify key values
    jassert (std::abs (Y[0] - 1.0f) < 0.001f);  // W = 1
    jassert (std::abs (Y[1]) < 0.001f);           // Y = 0 (front center has no left/right)
    jassert (std::abs (Y[2]) < 0.001f);           // Z = 0 (on horizon)
    jassert (std::abs (Y[3] - 1.0f) < 0.001f);   // X = 1 (front)

    // Test: source at left (azimuth=pi/2, elevation=0)
    float dirLeft[2] = { 1.5707963f, 0.0f };
    getSHreal_recur (3, dirLeft, 1, Y);

    DBG ("Left SH coefficients:");
    for (int i = 0; i < 16; ++i)
        DBG ("  ACN " + juce::String (i) + ": " + juce::String (Y[i], 4));

    jassert (std::abs (Y[0] - 1.0f) < 0.001f);  // W = 1
    jassert (std::abs (Y[1] - 1.0f) < 0.001f);  // Y = 1 (full left)
    jassert (std::abs (Y[2]) < 0.001f);           // Z = 0
    jassert (std::abs (Y[3]) < 0.001f);           // X = 0 (no front component)
}
```

### Step 3: Expected Output Levels Reference

For a unit-amplitude mono signal encoded at various positions (HOA3, ACN/SN3D):

| Position | ACN 0 (W) | ACN 1 (Y) | ACN 2 (Z) | ACN 3 (X) | Active Higher-Order |
|----------|-----------|-----------|-----------|-----------|---------------------|
| Front (0, 0) | 1.0 | 0.0 | 0.0 | 1.0 | ACN 6=-0.5, ACN 8=0.866 |
| Left (90, 0) | 1.0 | 1.0 | 0.0 | 0.0 | ACN 4=0.0, ACN 8=-0.866 |
| Above (0, 90) | 1.0 | 0.0 | 1.0 | 0.0 | ACN 6=1.0, all others 0 |
| Behind (180, 0) | 1.0 | 0.0 | 0.0 | -1.0 | ACN 6=-0.5, ACN 8=0.866 |

**Note on SAF conventions:** SAF's `getSHreal_recur()` follows the standard SN3D normalization where ACN 0 (W) = 1.0 for any direction. The directional channels (1-15) range from -1 to +1 for first order, and have different ranges for higher orders due to normalization.

### Step 4: Full Integration Test

```cpp
void testFullPipeline()
{
    // 1. Create a test tone (440 Hz sine, 1 second)
    const int sampleRate = 48000;
    const int numSamples = sampleRate;
    std::vector<float> testTone (numSamples);
    for (int i = 0; i < numSamples; ++i)
        testTone[i] = std::sin (2.0f * 3.14159f * 440.0f * i / sampleRate);

    // 2. Encode at front center
    SelfContainedAmbiEncoder encoder;
    encoder.setPosition (0.0f, 0.0f); // front center

    juce::AudioBuffer<float> ambiBuffer (16, numSamples);
    ambiBuffer.clear();
    encoder.encodeBlock (testTone.data(), numSamples,
                          ambiBuffer.getArrayOfWritePointers());

    // 3. Verify channel levels
    float rmsW = ambiBuffer.getRMSLevel (0, 0, numSamples); // ACN 0
    float rmsY = ambiBuffer.getRMSLevel (1, 0, numSamples); // ACN 1
    float rmsX = ambiBuffer.getRMSLevel (3, 0, numSamples); // ACN 3

    DBG ("RMS W (ACN 0): " + juce::String (rmsW, 4));
    DBG ("RMS Y (ACN 1): " + juce::String (rmsY, 4));
    DBG ("RMS X (ACN 3): " + juce::String (rmsX, 4));

    // For front center: W and X should have equal RMS (~0.707 for sine)
    // Y should be near zero
    jassert (rmsW > 0.5f);
    jassert (rmsX > 0.5f);
    jassert (rmsY < 0.01f);

    // 4. Optional: Decode to binaural and listen
    // (requires SAF binauraliser -- see Section 4)
}
```

### Step 5: DAW Validation

After building and installing the plugin:

```bash
# macOS: Clear AU cache and install
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
cp -R build/plugins/MyPlugin/MyPlugin_artefacts/Release/VST3/MyPlugin.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/MyPlugin/MyPlugin_artefacts/Release/AU/MyPlugin.component ~/Library/Audio/Plug-Ins/Components/

# Verify AU registration
auval -a | grep -i myplugin

# Open in DAW and verify:
# 1. Plugin loads without crash
# 2. Channel configuration shows correct ambisonics/stereo options
# 3. Audio passes through (insert on a track with audio, verify output)
# 4. CPU usage is reasonable (< 5% for 64 grains at HOA3)
```

---

## Quick-Start Checklist

1. [ ] Add SAF as submodule: `git submodule add https://github.com/leomccormack/Spatial_Audio_Framework.git`
2. [ ] Configure CMake: disable SAF tests/examples, set performance lib
3. [ ] Add `saf` to `target_link_libraries` in your plugin's CMakeLists.txt
4. [ ] Wrap SAF includes: `extern "C" { #include "saf.h" }`
5. [ ] Call `getSHreal_recur()` for per-grain SH coefficient computation
6. [ ] Multiply grain samples by SH coefficients, accumulate into ambisonics bus
7. [ ] Use SAF `binauraliser` for stereo output, or pass ambisonics directly to output
8. [ ] Verify: front-center source has W=1, X=1, Y=0, Z=0
9. [ ] Build both VST3 and AU, clear AU cache, install, test in DAW

---

## Sources

### Primary References
- SAF Repository: https://github.com/leomccormack/Spatial_Audio_Framework
- SPARTA Repository: https://github.com/leomccormack/SPARTA
- SAF API Documentation: https://leomccormack.github.io/Spatial_Audio_Framework/
- SPARTA Site: https://leomccormack.github.io/sparta-site/
- SAF Framework Structure: https://github.com/leomccormack/Spatial_Audio_Framework/blob/master/docs/FRAMEWORK_STRUCTURE.md

### SAF Module Documentation
- saf_hoa: https://leomccormack.github.io/Spatial_Audio_Framework/group___h_o_a.html
- saf_sh: https://leomccormack.github.io/Spatial_Audio_Framework/group___s_h.html
- saf_vbap: https://leomccormack.github.io/Spatial_Audio_Framework/group___v_b_a_p.html
- saf_hrir: https://leomccormack.github.io/Spatial_Audio_Framework/group___h_r_i_r.html

### JUCE Spatial Audio
- JUCE Forum - Spatial Audio Support: https://forum.juce.com/t/juce-spatial-audio-support/30509
- JUCE CMake API: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md

### Related Research (in this repository)
- JUCE 8 Multichannel & Spatial Audio: `research/juce8-multichannel-spatial-audio.md`
- Per-Grain Spatialization Techniques: `research/spatial-audio-per-grain-spatialization.md`

### Third-Party References
- DeepWiki SAF Overview: https://deepwiki.com/leomccormack/Spatial_Audio_Framework/1-spatial-audio-framework-overview
- SSA Plugins - JUCE for Spatial Audio: https://www.ssa-plugins.com/blog/2017/09/08/juce-for-spatial-audio/
- IEM Plugin Suite: https://github.com/tu-studio/IEMPluginSuite
