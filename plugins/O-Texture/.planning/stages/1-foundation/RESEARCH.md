# Stage 1: Foundation - Research

**Date:** 2026-02-14
**Phase:** research
**Topics investigated:** ANIRA CMake integration, ONNX placeholder generation, IS_SYNTH bus patterns, existing codebase patterns, juce_add_binary_data limits

---

## 1. ANIRA CMake Integration

### Version & Backend
- **ANIRA v2.0.3** is the latest stable release (GitHub tag)
- Bundles **ONNX Runtime 1.19.2** (not 1.17.1 as stated in the ANIRA paper)
- ANIRA builds as a **SHARED library** by default
- Can disable unused backends: `-DANIRA_WITH_LIBTORCH=OFF -DANIRA_WITH_TFLITE=OFF`

### FetchContent Pattern
```cmake
include(FetchContent)
FetchContent_Declare(
    anira
    GIT_REPOSITORY https://github.com/anira-project/anira.git
    GIT_TAG v2.0.3
)
set(ANIRA_WITH_ONNXRUNTIME ON CACHE BOOL "" FORCE)
set(ANIRA_WITH_LIBTORCH OFF CACHE BOOL "" FORCE)
set(ANIRA_WITH_TFLITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(anira)

target_link_libraries(${PROJECT_NAME} PRIVATE anira::anira)
```

### Key API Classes
- **`anira::InferenceConfig`** - Defines model path, input/output shapes, backend selection
- **`anira::InferenceHandler`** - Manages inference sessions, thread pool, double buffering
- **`anira::PrePostProcessor`** - Custom subclass for pre/post processing of audio data
- **`anira::HostAudioConfig`** - Host sample rate and buffer size info

### Threading Model
- Static thread pool shared across all InferenceHandler instances (prevents oversubscription)
- Double-buffered: while one buffer plays, the next is being inferred
- Audio thread never blocks -- sends data to inference thread via lock-free queue

### Latency Formula
```
L_total = H_adapt + ceil(I_max / H_host) * H_host + M_int
```
Where: H_adapt = adaptive buffer, I_max = max inference time, H_host = host buffer size, M_int = interpolation margin

### Custom PrePostProcessor
Required for O-Texture because input is a latent vector (not raw audio):
```cpp
class TexturePrePostProcessor : public anira::PrePostProcessor {
    void preProcess(anira::RingBuffer& input, anira::AudioBufferF& buffer,
                    anira::InferenceBackend backend) override {
        // Fill buffer with latent vector instead of audio samples
    }
    void postProcess(anira::AudioBufferF& buffer, anira::RingBuffer& output,
                     anira::InferenceBackend backend) override {
        // Copy decoded audio to output ring buffer
    }
};
```

### Distribution Consideration
- ANIRA shared library (.dylib/.dll/.so) must be distributed alongside plugin
- On macOS: embed in plugin bundle's Frameworks/ directory
- On Windows: place DLL alongside VST3
- Alternative: build ANIRA as static library (may need CMake option investigation)

---

## 2. ONNX Placeholder Models

### Generated Models (Verified)
All 3 placeholder ONNX models generated and verified at:
`plugins/O-Texture/Resources/models/placeholder/`

| Model | Size | Input Shape | Output Shape(s) |
|-------|------|------------|-----------------|
| decoder.onnx | 132,450 bytes (129 KB) | [1, 32] float32 | [1, 1, 4096] float32 |
| encoder.onnx | 133,597 bytes (131 KB) | [1, 1, 4096] float32 | mu: [1, 32], logvar: [1, 32] float32 |
| prior.onnx | 6,697 bytes (6.5 KB) | [1, 16, 32] float32 | mu: [1, 32], logvar: [1, 32] float32 |

- **ONNX opset:** 17
- **Architecture:** Small fully-connected networks (match I/O shapes but produce noise)
- **Verified:** Load and run correctly with ONNX Runtime 1.24.1
- **Memory loading:** Confirmed `CreateSessionFromArray(data, size)` works (required for binary resource embedding)

### Generation Script
`scripts/generate_placeholder_models.py` -- regenerates all 3 models. Requires `pip install onnx onnxruntime`.

---

## 3. IS_SYNTH Bus Patterns

### Critical Finding
**IS_SYNTH TRUE creates fundamentally different plugin types per format:**

| Format | IS_SYNTH TRUE Type | Audio Input Support |
|--------|-------------------|-------------------|
| AU | kAudioUnitType_MusicDevice (aumu) | **NO standard audio input in Logic Pro** |
| VST3 | Instrument with kMain input bus | Input bus configurable |
| AAX | Instrument | Varies by DAW |

### The Problem
O-Texture has two modes:
- **Generate mode** -- no audio input needed (pure synth)
- **Transform mode** -- requires audio input (effect-like)

With IS_SYNTH TRUE, Logic Pro will NOT route audio input to the plugin. Transform mode would be non-functional in Logic.

### Options Identified

| Option | Approach | Pros | Cons |
|--------|----------|------|------|
| A | IS_SYNTH TRUE + sidechain | Correct DAW classification | Sidechain routing cumbersome, Logic may still not cooperate |
| B | IS_SYNTH FALSE + NEEDS_MIDI_INPUT | Full audio input access | Not classified as instrument, Generate mode UX weird |
| C | Two separate plugins | Clean separation | Maintenance burden, two builds |
| D | VST3_CATEGORIES override | Custom classification | AU type still determined by IS_SYNTH |

### Recommendation for Stage 1
**Proceed with IS_SYNTH TRUE** for Stage 1:
- Generate mode is the primary mode
- Transform mode is secondary and can be addressed in Stage 2/3
- Sidechain input bus can be added later
- Keep BusesProperties configurable for future changes

### Bus Configuration Pattern (IS_SYNTH TRUE)
```cpp
BusesProperties()
    .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)  // disabled by default
    .withOutput("Output", juce::AudioChannelSet::stereo(), true)
```

### Latency Reporting
- `setLatencySamples()` supported at runtime in VST3 (sends `kLatencyChanged` flag)
- ~6144 samples (4096 block + 2048 overlap) at 48kHz = ~128ms
- Report via `getLatencySamples()` override

---

## 4. Existing Codebase Patterns

### CMakeLists.txt Template
Reference plugins: O-Comp, O-TextureForge, O-AnalogEQ

Standard structure:
```cmake
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME Ouaricon
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Ouar
    PLUGIN_CODE OTex
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Texture"
    NEEDS_WEBVIEW2 TRUE
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)
```

### PluginProcessor Pattern
From O-Comp and O-TextureForge:
- APVTS created in constructor via `createParameterLayout()` static method
- `BusesProperties` configured in constructor initializer list
- State save/restore via `getStateInformation()`/`setStateInformation()` using APVTS `copyState()`/`replaceState()`
- `prepareToPlay()` initializes DSP with sample rate and buffer size

### PluginEditor Pattern
From O-Comp:
- WebView setup with `juce::WebBrowserComponent::Options`
- Resource provider via `withResourceProvider()`
- Backend messaging via `withNativeIntegration()`
- Window size set in constructor (typically 800x600)
- WebView2 user data folder set for Windows

### Binary Data Pattern
```cmake
juce_add_binary_data(${PROJECT_NAME}_ModelData
    NAMESPACE ModelData
    HEADER_NAME ModelData.h
    SOURCES
        Resources/models/placeholder/decoder.onnx
        Resources/models/placeholder/encoder.onnx
        Resources/models/placeholder/prior.onnx
)

juce_add_binary_data(${PROJECT_NAME}_UIData
    NAMESPACE UIData
    HEADER_NAME UIData.h
    SOURCES
        Resources/ui/index.html
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_NAME}_ModelData
    ${PROJECT_NAME}_UIData
)
```

### IS_SYNTH Plugins in Codebase
Found: O-TextureForge, O-Marimba, O-Bells, O-Lyrica -- all use IS_SYNTH TRUE with similar bus configurations.

---

## 5. juce_add_binary_data Limits

### Size Constraints
- **No hard size limit** in JUCE itself
- In CMake mode, one `.cpp` file per source file (`maxFileSize=0` equivalent)
- Binary data expansion: ~3.5-4x (raw bytes -> C++ array literal)
- 31MB of ONNX models -> ~112MB of generated `.cpp` files
- Modern compilers handle 8-10MB `.cpp` files without issue

### Naming Convention
- Dots, spaces, hyphens -> underscores in generated identifiers
- Only `[A-Za-z0-9_]` retained
- `decoder.onnx` -> `BinaryData::decoder_onnx`, `BinaryData::decoder_onnxSize`

### Multiple Targets
- Use separate targets with different `NAMESPACE` and `HEADER_NAME`
- **ModelData** namespace for ONNX models (avoids collision with UI binary data)
- **UIData** namespace for WebView HTML/CSS/JS resources
- Each target becomes a separate static library linked to the plugin

### Memory Loading
```cpp
// Load ONNX model from binary resource
Ort::SessionOptions options;
Ort::Session session(env, ModelData::decoder_onnx,
                     static_cast<size_t>(ModelData::decoder_onnxSize),
                     options);
```

---

## Key Decisions for Plan Phase

### Confirmed
1. **ANIRA v2.0.3** via FetchContent with ONNX Runtime backend only
2. **Placeholder models** generated and verified (3 files, ~267KB total)
3. **Binary data embedding** via two targets: ModelData (models) + UIData (WebView)
4. **IS_SYNTH TRUE** for Stage 1 (Transform mode audio routing deferred to Stage 2)
5. **WebView2 static linking** per codebase standard

### Needs Attention in Plan
1. **ANIRA shared library distribution** -- need to handle .dylib/.dll bundling in CMake
2. **Custom PrePostProcessor** -- latent vector input instead of audio
3. **Sidechain bus** -- add disabled sidechain input for future Transform mode
4. **Latency reporting** -- ~6144 samples, need `getLatencySamples()` override
5. **Model loading from memory** -- ONNX Runtime `CreateSessionFromArray` vs ANIRA's model loading API (may need investigation)

### Open Risk
- **ANIRA's model loading API** may expect file paths, not memory buffers. If so, need to write binary data to temp file at runtime, or use ONNX Runtime directly for session creation while still using ANIRA for thread management.
