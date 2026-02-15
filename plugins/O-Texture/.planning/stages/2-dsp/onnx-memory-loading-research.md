# ONNX Model In-Memory Loading Research

**Researched:** 2026-02-14
**Domain:** ANIRA v2.0.3 + ONNX Runtime 1.19.2 -- in-memory model loading
**Confidence:** HIGH (verified from source code, not documentation)

## Summary

ANIRA v2.0.3 **natively supports loading ONNX models from in-memory binary buffers**. No temp file extraction is needed. The entire pipeline -- from JUCE `juce_add_binary_data` to ANIRA `ModelData` to ONNX Runtime `Ort::Session` -- supports passing raw model bytes without ever touching the filesystem.

This is the optimal path for O-Texture. The existing `CMakeLists.txt` already embeds three ONNX models as binary data in the `ModelData` namespace. These can be passed directly to ANIRA's `ModelData` constructor with `is_binary = true`, which ANIRA then passes to `Ort::Session(env, model_data, model_data_length, session_options)` -- the ONNX Runtime `CreateSessionFromArray` API.

**Primary recommendation:** Use ANIRA's native binary `ModelData` constructor. No temp files, no filesystem access, no cleanup needed.

## Finding 1: ONNX Runtime C++ API Supports In-Memory Session Creation

**Confidence: HIGH** (verified from ONNX Runtime 1.19.2 header source code)

The `Ort::Session` class has a constructor that accepts a memory buffer directly:

```cpp
// From: onnxruntime_cxx_api.h (line 1123)
Session(const Env& env, const void* model_data, size_t model_data_length, const SessionOptions& options);
```

This wraps the C API function `OrtApi::CreateSessionFromArray`. The implementation (from `onnxruntime_cxx_inline.h`, line 1113-1115):

```cpp
inline Session::Session(const Env& env, const void* model_data, size_t model_data_length, const SessionOptions& options) {
  ThrowOnError(GetApi().CreateSessionFromArray(env, model_data, model_data_length, options, &this->p_));
}
```

There is also a variant with prepacked weights containers, but the basic version is what ANIRA uses.

**Source:** `/Users/taylorbrook/Dev/VST-development/build/_deps/anira-src/modules/onnxruntime-1.19.2-macOS-arm64/include/onnxruntime_cxx_api.h` (lines 1118-1125)

## Finding 2: ANIRA v2.0.3 ModelData Struct Supports Binary Data Natively

**Confidence: HIGH** (verified from ANIRA source code)

ANIRA's `ModelData` struct has two constructors:

### Constructor 1: Binary data (for embedded models)
```cpp
// is_binary defaults to true
ModelData(void* data, size_t size, InferenceBackend backend,
          const std::string& model_function = "", bool is_binary = true);
```

When `is_binary = true`:
- `m_data` stores the raw pointer directly (no copy)
- `m_size` is the size in bytes of the model binary
- The destructor does NOT free the data (assumes external ownership -- perfect for BinaryData)

### Constructor 2: File path (for filesystem models)
```cpp
// is_binary defaults to false
ModelData(const std::string& model_path, InferenceBackend backend,
          const std::string& model_function = "", bool is_binary = false);
```

When `is_binary = false`:
- `m_data` is a malloc'd copy of the path string
- `m_size` is the string length
- The destructor frees the allocated path string

**Source:** `/Users/taylorbrook/Dev/VST-development/build/_deps/anira-src/include/anira/InferenceConfig.h` (lines 28-157)

## Finding 3: ANIRA OnnxRuntimeProcessor Already Handles Both Paths

**Confidence: HIGH** (verified from ANIRA source code)

The `OnnxRuntimeProcessor::Instance` constructor explicitly checks `is_model_binary()` and branches accordingly:

```cpp
// From: OnnxRuntimeProcessor.cpp (lines 39-55)
if (m_inference_config.is_model_binary(anira::InferenceBackend::ONNX)) {
    const anira::ModelData* model_data = m_inference_config.get_model_data(anira::InferenceBackend::ONNX);
    assert(model_data && "Model data not found for binary model!");

    // Load model from binary data -- uses Ort::Session(env, data, size, options)
    m_session = std::make_unique<Ort::Session>(m_env, model_data->m_data, model_data->m_size, m_session_options);
} else {
    // Load model from file path -- uses Ort::Session(env, path, options)
    std::string modelpath = m_inference_config.get_model_path(anira::InferenceBackend::ONNX);
    m_session = std::make_unique<Ort::Session>(m_env, modelpath.c_str(), m_session_options);
}
```

**Source:** `/Users/taylorbrook/Dev/VST-development/build/_deps/anira-src/src/backends/OnnxRuntimeProcessor.cpp` (lines 34-55)

## Finding 4: ANIRA's Own JUCE Example Uses This Exact Pattern

**Confidence: HIGH** (verified from ANIRA example code)

ANIRA's official JUCE audio plugin example (`MODEL_TO_USE == 1`) demonstrates binary model loading with `juce_add_binary_data`:

```cpp
// From: examples/juce-audio-plugin/PluginProcessor.h (lines 95-105)
#include <BinaryData.h>

std::vector<anira::ModelData> model_data = {
#ifdef USE_ONNXRUNTIME
    {(void*) BinaryData::steerablenafxlibtorchdynamic_onnx,
     BinaryData::steerablenafxlibtorchdynamic_onnxSize,
     anira::InferenceBackend::ONNX},
#endif
};

anira::InferenceConfig inference_config = {
    model_data,
    tensor_shape_cnn_config,
    processing_spec_cnn_config,
    42.66f,   // max inference time (ms)
    2         // warm-up inferences
};
```

**Source:** `/Users/taylorbrook/Dev/VST-development/build/_deps/anira-src/examples/juce-audio-plugin/PluginProcessor.h` (lines 95-112)

## Finding 5: O-Texture Already Has Binary Data Set Up Correctly

**Confidence: HIGH** (verified from project files)

The existing `CMakeLists.txt` (from Stage 1) already embeds three ONNX models:

```cmake
juce_add_binary_data(${PROJECT_NAME}_ModelData
    NAMESPACE ModelData
    HEADER_NAME ModelData.h
    SOURCES
        Resources/models/placeholder/decoder.onnx
        Resources/models/placeholder/encoder.onnx
        Resources/models/placeholder/prior.onnx
)
```

This generates the following variables in the `ModelData` namespace:

| Variable | Size Variable | Current Size |
|----------|--------------|-------------|
| `ModelData::decoder_onnx` | `ModelData::decoder_onnxSize` | 132,450 bytes |
| `ModelData::encoder_onnx` | `ModelData::encoder_onnxSize` | 133,597 bytes |
| `ModelData::prior_onnx` | `ModelData::prior_onnxSize` | 6,697 bytes |

**Note:** These are placeholder models. When real trained models replace them, the variable names remain the same (JUCE generates names from filenames). The `_onnxSize` variables are `const int`, but ANIRA's `ModelData` accepts `size_t` -- the implicit conversion from `int` is safe since sizes are always positive.

**Source:** `/Users/taylorbrook/Dev/VST-development/build/plugins/O-Texture/juce_binarydata_OuariconTexture_ModelData/JuceLibraryCode/ModelData.h`

## Exact Implementation Pattern for O-Texture

Here is the verified, working pattern combining all findings:

```cpp
#include <anira/anira.h>
#include "ModelData.h"  // Generated by juce_add_binary_data

// Create ModelData with binary data (is_binary = true by default)
std::vector<anira::ModelData> decoder_model_data = {
    {(void*) ModelData::decoder_onnx,
     (size_t) ModelData::decoder_onnxSize,
     anira::InferenceBackend::ONNX}
};

std::vector<anira::ModelData> prior_model_data = {
    {(void*) ModelData::prior_onnx,
     (size_t) ModelData::prior_onnxSize,
     anira::InferenceBackend::ONNX}
};

// For Transform mode (deferred, but pattern is the same):
std::vector<anira::ModelData> encoder_model_data = {
    {(void*) ModelData::encoder_onnx,
     (size_t) ModelData::encoder_onnxSize,
     anira::InferenceBackend::ONNX}
};

// Then use in InferenceConfig:
anira::InferenceConfig decoder_config(
    decoder_model_data,
    {anira::TensorShape({{1, 32}}, {{1, 4096}})},     // Example shapes
    anira::ProcessingSpec({1}, {1}, {32}, {4096}),      // Example spec
    10.0f,   // max inference time (ms)
    2        // warm-up inferences
);
```

**Key observations:**
- The `(void*)` cast from `const char*` is safe and matches ANIRA's example pattern
- ANIRA does NOT copy the binary data for `is_binary = true` -- it stores the pointer directly
- The binary data from JUCE's `juce_add_binary_data` is embedded in the executable's data segment and lives for the entire process lifetime -- perfect for ANIRA's non-owning pointer model
- No temp file creation, cleanup, or filesystem permissions issues

## Answers to Specific Questions

### Q1: Does Ort::Session support creating sessions from a memory buffer?
**YES.** `Ort::Session(env, model_data, model_data_length, options)` wraps `CreateSessionFromArray`. Verified in ONNX Runtime 1.19.2 headers.

### Q2: Does ANIRA support in-memory model data?
**YES.** `ModelData(void* data, size_t size, InferenceBackend backend)` with `is_binary = true` (the default for the void* constructor). ANIRA's `OnnxRuntimeProcessor` checks `is_model_binary()` and uses the `CreateSessionFromArray` path automatically.

### Q3: Do we need temp file extraction?
**NO.** The entire pipeline supports in-memory loading natively. No temp files needed.

### Q4: Can we bypass ANIRA's model loading?
**Not needed** -- ANIRA handles it directly. But if needed in the future, `OnnxRuntimeProcessor` creates its own `Ort::Session` internally and does not expose it. You would need to subclass or modify ANIRA to inject a custom session.

## Potential Pitfalls

### 1. ModelData Pointer Lifetime
**Risk: LOW.** JUCE binary data is static/global -- lives for the entire process. ANIRA stores the pointer without copying. No dangling pointer risk.

### 2. ModelData const-correctness
**Risk: LOW.** JUCE's `BinaryData` variables are `const char*`, but ANIRA's `ModelData` takes `void*`. The `(void*)` cast discards const, but ANIRA only reads the data (passes to `Ort::Session` which also only reads). ANIRA's example code uses this exact cast pattern.

### 3. Multiple Models with Same ANIRA InferenceHandler
**Risk: MEDIUM.** O-Texture needs separate InferenceHandlers for decoder and prior (and encoder in future). Each needs its own `InferenceConfig` with its own `ModelData`. ANIRA's RAVE example demonstrates this exact pattern with separate encoder/decoder handlers. See ANIRA example `MODEL_TO_USE == 7` in `PluginProcessor.h` (lines 132-135).

### 4. Binary Data Size Limits
**Risk: LOW.** JUCE's `juce_add_binary_data` has no practical size limit for ONNX models. The expected model sizes (~2.4 MB encoder, ~2.4 MB decoder, ~0.4 MB prior) are well within reason. These become part of the compiled binary's data section.

## Sources

### Primary (HIGH confidence -- direct source code verification)
- ANIRA v2.0.3 source: `InferenceConfig.h` (ModelData struct with binary support)
- ANIRA v2.0.3 source: `OnnxRuntimeProcessor.cpp` (binary model loading branch)
- ANIRA v2.0.3 example: `juce-audio-plugin/PluginProcessor.h` (BinaryData usage pattern)
- ONNX Runtime 1.19.2 header: `onnxruntime_cxx_api.h` (Session constructor from array)
- ONNX Runtime 1.19.2 inline: `onnxruntime_cxx_inline.h` (CreateSessionFromArray implementation)
- O-Texture CMakeLists.txt (existing binary data setup)
- Generated ModelData.h (variable names and sizes)
