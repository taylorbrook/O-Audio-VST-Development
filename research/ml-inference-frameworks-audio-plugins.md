---
title: "Machine Learning Inference Frameworks for Real-Time Audio Plugins"
created: 2026-02-08
domain: ml
type: research
keywords:
  - ml-inference
  - onnxruntime
  - tensorrt
  - libtorch
  - real-time
  - audio-plugins
---
# Machine Learning Inference Frameworks for Real-Time Audio Plugins

## Comprehensive Research Report

**Date**: 2026-02-07
**Focus**: Evaluating ML inference frameworks for real-time safe deployment in VST3/AU audio plugins built with JUCE and CMake.

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Framework Analysis](#framework-analysis)
   - [RTNeural](#1-rtneural)
   - [ONNX Runtime](#2-onnx-runtime)
   - [LibTorch (PyTorch C++)](#3-libtorch-pytorch-c)
   - [TensorFlow Lite](#4-tensorflow-lite)
   - [NAM (Neural Amp Modeler)](#5-nam-neural-amp-modeler)
   - [Anira (Meta-Framework)](#6-anira-meta-framework)
   - [Nanoflare](#7-nanoflare)
   - [Neutone SDK](#8-neutone-sdk)
3. [Comparative Analysis](#comparative-analysis)
4. [Real-Time Audio Thread Safety](#real-time-audio-thread-safety)
5. [Architectural Patterns for ML in Audio Plugins](#architectural-patterns-for-ml-in-audio-plugins)
6. [Model Quantization for Audio Plugins](#model-quantization-for-audio-plugins)
7. [Recommendations by Use Case](#recommendations-by-use-case)

---

## Executive Summary

Running neural network inference inside an audio plugin's `processBlock()` presents a fundamental tension: ML frameworks allocate memory and use locks internally, while the audio thread demands zero allocations, zero locks, and bounded execution time. Five primary approaches exist to resolve this:

1. **RTNeural**: Purpose-built, allocation-free inference directly on the audio thread. Best for small, fixed-architecture models (amp modeling, saturation).
2. **ONNX Runtime**: Fastest general-purpose engine for stateless models. Requires thread-pool decoupling from audio thread.
3. **LibTorch**: Fastest for stateful (RNN) models. Heaviest deployment footprint (~267MB). Requires thread decoupling.
4. **TensorFlow Lite**: Lightweight general-purpose engine. Competitive for small models. Requires thread decoupling.
5. **Anira**: Meta-framework that wraps ONNX/LibTorch/TFLite with real-time safe thread pool architecture. Best for developers who want backend flexibility.

The critical architectural insight: **Only RTNeural can safely run directly on the audio thread.** All general-purpose frameworks (ONNX, LibTorch, TFLite) perform internal memory allocations and must be decoupled to background threads with buffer-based latency compensation.

---

## Framework Analysis

### 1. RTNeural

**Source**: https://github.com/jatinchowdhury18/RTNeural
**Paper**: [RTNeural: Fast Neural Inferencing for Real-Time Systems](https://arxiv.org/abs/2106.03037) (Jatin Chowdhury, 2021)
**License**: BSD-3-Clause

#### Overview
RTNeural is a C++ library specifically designed for real-time audio-rate neural network inference. It is the only framework in this comparison that was built from the ground up for audio thread safety.

#### Supported Layer Types
- Dense (Fully-Connected)
- LSTM (Long Short-Term Memory)
- GRU (Gated Recurrent Unit)
- Conv1D, Conv2D
- BatchNorm1D, BatchNorm2D
- Activation layers: tanh, ReLU, Sigmoid, SoftMax, ELU, PReLU

#### Computation Backends
| Backend | Best For | Dependency |
|---------|----------|------------|
| **Eigen** | Larger networks | Eigen library |
| **xsimd** | Smaller networks | xsimd library |
| **Accelerate** | Apple platforms | macOS/iOS built-in |
| **STL** | Universal fallback | None (C++ stdlib) |

Selected via CMake: `-DRTNEURAL_EIGEN=ON`, `-DRTNEURAL_XSIMD=ON`, or `-DRTNEURAL_STL=ON`.

#### Real-Time Safety
- **Allocation-free inference**: No memory allocation except during layer construction/destruction. Safe to call directly from `processBlock()`.
- **No locks or system calls** during forward pass.
- **Compile-time model definition** eliminates runtime type dispatch overhead.

#### Performance
- **2-3x faster than TorchScript** for typical audio model sizes (small LSTM/GRU networks).
- Optimized for small layer sizes (typical in audio: 8-64 hidden units).
- Compile-time model API provides significantly better performance than runtime model loading due to compiler optimizations and eliminated polymorphism.
- AVX SIMD support via `-DRTNEURAL_USE_AVX=ON`.

#### Two Model Loading Modes

**Runtime (flexible but slower)**:
```cpp
auto model = RTNeural::json_parser::parseJson<double>(jsonStream);
model->forward(inputData);
```

**Compile-time (fixed architecture, fastest)**:
```cpp
RTNeural::ModelT<float, 1, 1,
    RTNeural::LSTMLayerT<float, 1, 16>,
    RTNeural::DenseT<float, 16, 1>
> model;
model.forward(inputData);
```

#### CMake Integration
```cmake
add_subdirectory(RTNeural)
target_link_libraries(MyPlugin LINK_PUBLIC RTNeural)
```

Also works as header-only with manual backend selection.

#### Notable Plugins Using RTNeural
| Plugin | Use Case |
|--------|----------|
| **Chow Centaur** | Klon Centaur pedal emulation (RNN) |
| **Chow Tape Model** | Analog tape emulation (Dense networks) |
| **BYOD** | Guitar distortion with ML effects |
| **AIDA-X** | Neural amp modeling (AU/CLAP/LV2/VST2/VST3) |
| **GuitarML Proteus** | Amp/pedal capture (LSTM, ~2% CPU on 40-layer network) |
| **GuitarML NeuralPi** | Raspberry Pi guitar pedal |
| **NeuralNote** | Audio-to-MIDI transcription (CNN part via RTNeural) |
| **Tone Empire LVL-01, TM700, Neural Q** | Commercial products |

#### Pros
- Only framework safe for direct audio-thread inference
- Zero-allocation forward pass
- Compile-time model API enables aggressive compiler optimization
- Tiny footprint (header-only option)
- Proven in dozens of shipping plugins
- Active development and community
- Excellent for recurrent networks (LSTM/GRU) typical in amp modeling

#### Cons
- Limited to supported layer types (no attention, no transformer blocks)
- Not suitable for large/complex models
- Must export weights from PyTorch/TF to JSON format
- No GPU support (CPU only, which is correct for audio plugins)
- Runtime model API is slower than compile-time but more flexible

---

### 2. ONNX Runtime

**Source**: https://github.com/microsoft/onnxruntime
**License**: MIT
**Website**: https://onnxruntime.ai

#### Overview
Microsoft's cross-platform inference engine. Supports the ONNX interchange format, making it framework-agnostic (models from PyTorch, TensorFlow, scikit-learn, etc.). Fastest general-purpose engine for stateless models according to ANIRA benchmarks.

#### Real-Time Safety
- **NOT real-time safe**: Performs internal memory allocations (`malloc`, `free`, `posix_memalign`) during inference.
- Must be decoupled from the audio thread via a thread pool / background thread architecture.
- ANIRA benchmarks detected real-time violations (memory operations) during inference.

#### Performance (from ANIRA benchmarks)
- **Fastest for stateless models** (CNN, hybrid networks) across all tested configurations.
- For CNN-29k model: significantly outperforms both LibTorch and TFLite (p<0.0001).
- Typical inference: ~1-2ms for small models on modern CPUs.
- **Does not support stateful RNN models** (LSTM/GRU with internal state) -- a significant limitation for amp modeling.

#### Deployment Size
- Full build: ~7.5MB shared library.
- **Minimal/custom builds**: The [ort-builder](https://github.com/olilarkin/ort-builder) tool prunes operators to only those needed by your specific model, generating slim static libraries.
- Can serialize ONNX models to ORT format embedded as C++ source code.
- `MinSizeRel` CMake config produces smallest binary.
- Windows static builds can be very large due to LTO/LTCG settings.

#### CMake Integration
```cmake
# Using pre-built binaries
find_package(onnxruntime REQUIRED)
target_link_libraries(MyPlugin onnxruntime::onnxruntime)

# Or using ort-builder for custom static lib
add_library(ort STATIC IMPORTED)
set_target_properties(ort PROPERTIES IMPORTED_LOCATION ${ORT_LIB_PATH})
```

#### Audio Plugin Caution
The ort-builder project warns: "Due to risk of ODR violations, global/static variable conflicts, and dependency symbol clashes with DAW Hosts that use ORT themselves -- think hard before you use this in an audio plugin!" This is because some DAWs may already bundle ONNX Runtime, creating symbol conflicts with static linking.

#### Notable Audio Projects Using ONNX Runtime
| Project | Use Case |
|---------|----------|
| **iPlug2OnnxRuntime** | ML audio plugin example (LSTM amp modeling) |
| **NeuralNote** | Audio-to-MIDI (CQT + Harmonic Stacking feature extraction) |
| **nn-inference-template** | ADC23 reference template for real-time ML plugins |
| **Anira** | Backend option in real-time inference architecture |

#### Pros
- Fastest for stateless models (CNNs, feedforward)
- Framework-agnostic (accepts PyTorch, TF, etc. models via ONNX export)
- Mature, well-maintained by Microsoft
- Graph-level optimizations (operator fusion, constant folding)
- Custom build can prune to minimal operator set
- Cross-platform (Windows, macOS, Linux, mobile, WASM)

#### Cons
- Not real-time safe (requires thread decoupling)
- No stateful RNN support (major limitation for amp modeling)
- ODR/symbol clash risk when statically linked in plugins
- CMake integration is non-trivial (no official FindOnnxruntime.cmake)
- Full build adds significant binary size

---

### 3. LibTorch (PyTorch C++)

**Source**: https://pytorch.org/cppdocs/
**License**: BSD-3-Clause

#### Overview
The C++ frontend of PyTorch. Uses TorchScript as the model serialization format. The standard approach for deploying PyTorch models to production C++ applications.

#### Real-Time Safety
- **NOT real-time safe**: Exhibits the highest frequency of real-time violations among tested engines.
- Violations include: `malloc`, `free`, `pthread_mutex_lock` -- especially during initial inferences.
- ANIRA benchmarks show LibTorch CNN-29k has particularly elevated violation counts during inference chain initialization.
- Absolutely requires thread-pool decoupling from the audio callback.

#### Performance (from ANIRA benchmarks)
- **Fastest for stateful models** (LSTM/GRU) -- critical advantage for amp modeling and temporal effects.
- Medium performance for stateless models (behind ONNX Runtime, ahead of TFLite for larger models).
- First inferences are significantly slower due to JIT warmup -- implement warm-up passes before audio processing begins.

#### Deployment Size
- **~267MB without CUDA** (CPU-only). This is the biggest disadvantage.
- ~1.2GB with CUDA support (not relevant for audio plugins).
- Minimal builds possible but complex: disable CUDA, MKL-DNN, FBGEMM, QNNPACK, OpenMP, etc.
- Community tool [build-libtorch](https://github.com/shanemcandrewai/build-libtorch) provides minimal build scripts.
- Alternative: **Nanoflare** (see below) provides a 25-30% faster, lightweight alternative.

#### CMake Integration
```cmake
find_package(Torch REQUIRED)
target_link_libraries(MyPlugin "${TORCH_LIBRARIES}")
set_property(TARGET MyPlugin PROPERTY CXX_STANDARD 17)
```

#### Notable Audio Projects
| Project | Use Case |
|---------|----------|
| **Anira** | Backend option for real-time inference |
| **nn-inference-template** | ADC23 reference template |
| **Scyclone (Torsion Audio)** | Real-time neural timbre transfer |
| **Various JUCE forum projects** | Custom amp modeling plugins |

#### Pros
- Fastest stateful (RNN/LSTM/GRU) inference
- Direct path from PyTorch training to C++ deployment (TorchScript)
- Broadest model architecture support
- Large community and ecosystem
- Active development by Meta

#### Cons
- Enormous deployment size (~267MB CPU-only)
- Worst real-time violation profile (most locks and allocations)
- Slow initial inferences (JIT warmup required)
- Complex minimal build process
- Overkill for simple audio models

---

### 4. TensorFlow Lite

**Source**: https://www.tensorflow.org/lite
**License**: Apache-2.0

#### Overview
Google's lightweight inference engine designed for edge/mobile deployment. Smallest general-purpose engine footprint. Competitive for small models.

#### Real-Time Safety
- **NOT real-time safe**: Performs `malloc`, `free`, `aligned_alloc` during inference.
- Fewer violations than LibTorch but more than ONNX Runtime.
- Requires thread-pool decoupling from audio callback.

#### Performance (from ANIRA benchmarks)
- **Competitive for small models**: As CNN parameters decrease (29k to 1k), TFLite approaches ONNX Runtime performance.
- Slowest for large stateless models.
- Supports stateful RNN models (unlike ONNX Runtime), but slower than LibTorch.
- XNNPACK delegate (enabled by default) provides CPU optimization.

#### Deployment Size
- Smallest of the three general-purpose engines.
- Static library `libtensorflow-lite.a` -- exact size depends on configuration.
- Can further reduce with `-DTFLITE_ENABLE_XNNPACK=OFF` (trades performance for size).
- Note: Static lib is not self-contained; transitive dependencies must be linked.

#### CMake Integration
```cmake
# Build TFLite from source
add_subdirectory(tensorflow/lite)
target_link_libraries(MyPlugin tensorflow-lite)

# Or use pre-built static library
add_library(tf_lite STATIC IMPORTED)
set_target_properties(tf_lite PROPERTIES
    IMPORTED_LOCATION /path/to/libtensorflow-lite.a)
target_include_directories(MyPlugin PRIVATE /path/to/tflite/include)
```

Requires CMake 3.16+.

#### Pros
- Smallest deployment footprint of general-purpose engines
- Good for small models on constrained hardware
- Supports stateful RNN (unlike ONNX Runtime)
- Well-suited for edge/embedded audio (Raspberry Pi, etc.)
- Cross-platform

#### Cons
- Slowest for large models
- Not real-time safe (requires thread decoupling)
- Fewer optimizations than ONNX Runtime for desktop CPUs
- Model conversion from TensorFlow can have compatibility issues
- Less ecosystem support for audio-specific use cases

---

### 5. NAM (Neural Amp Modeler)

**Source**: https://github.com/sdatkinson/neural-amp-modeler (training)
**Plugin**: https://github.com/sdatkinson/NeuralAmpModelerPlugin (inference)
**Website**: https://www.neuralampmodeler.com
**License**: MIT

#### Overview
NAM is a complete system for neural amp/pedal modeling: a training framework (Python/PyTorch Lightning) and a plugin for real-time inference (C++/iPlug2). Unlike general inference frameworks, NAM is an end-to-end solution specifically for guitar amp emulation.

#### Architecture
NAM supports multiple neural network architectures:

| Architecture | Variants | CPU Cost | Quality |
|-------------|----------|----------|---------|
| **WaveNet** | Standard, Lite, Feather, Nano | High to Very Low | Excellent to Fair |
| **LSTM** | 1x8, 1x12, 1x16, 1x24, 2x8, 2x12, 2x16 | Medium to Low | Good to Fair |

- WaveNet: Dilated causal convolutions. Best quality but most expensive. Standard models run acceptably on modern PCs; Feather barely runs on Raspberry Pi 4.
- LSTM: Recurrent architecture. Lower cost, good quality for simpler tones.
- "Nano" LSTM often produces poor quality -- there is a minimum viable model size.

#### Technical Stack
- **Training**: PyTorch Lightning
- **Plugin framework**: iPlug2 (not JUCE)
- **Numerical backend**: Eigen
- **Core inference**: NeuralAmpModelerCore (C++ submodule)
- **IR convolution**: Custom FFTConvolver fork
- **Resampling**: r8brain-free-src

#### Plugin Formats
- VST3 (Windows 10+ 64-bit)
- AudioUnit (macOS 10.15+)
- Standalone
- Future: CLAP, AAX, Linux, iOS

#### Real-Time Performance
- WaveNet Standard: Acceptable on modern desktop CPUs, problematic with multiple instances.
- WaveNet Feather: Marginal on Raspberry Pi 4.
- LSTM models: Generally lighter CPU load.

#### Pros
- End-to-end solution (training + deployment)
- Huge community of shared amp captures
- Very high fidelity modeling
- Free and open source
- Multiple architecture options for CPU/quality tradeoff

#### Cons
- Guitar amp modeling only (not general-purpose)
- Uses iPlug2, not JUCE (would need porting for JUCE integration)
- WaveNet models are CPU-hungry
- Plugin architecture is tightly coupled to NAM model format

---

### 6. Anira (Meta-Framework)

**Source**: https://github.com/anira-project/anira
**Paper**: [ANIRA: An Architecture for Neural Network Inference in Real-Time Audio Applications](https://arxiv.org/abs/2506.12665) (2025)
**License**: Apache-2.0

#### Overview
Anira is not an inference engine itself but a **real-time safe wrapper architecture** that decouples any of three backends (ONNX Runtime, LibTorch, TensorFlow Lite) from the audio callback. Presented at ADC23 and published as a peer-reviewed paper in 2025.

#### Architecture

```
Audio Thread (processBlock)
    |
    v
[InferenceManager] -- buffer alignment, latency calculation
    |
    v
[SessionElement] -- thread-safe data storage (atomic or semaphore)
    |
    v
[Static Thread Pool] -- pre-allocated, high-priority inference threads
    |
    v
[Backend: ONNX / LibTorch / TFLite]
```

**Key design decisions**:
- **Static thread pool**: Pre-allocated at initialization. No thread creation during audio processing. Prevents oversubscription.
- **Configurable thread count**: Default = (hardware_concurrency - 1).
- **Two synchronization modes**:
  - **Atomic-based**: Strict real-time safety, no system calls. Higher latency.
  - **Semaphore-based**: Allows controlled blocking for reduced latency.
- **Parallel inference**: For stateless models, all threads process simultaneously.
- **Serial constraint**: Stateful (RNN) models must process serially due to internal state dependencies.

#### Latency Model
```
L_total = H_adapt + ceil(I_max / H_host) * H_host + M_int
```
Where:
- `H_adapt` = buffer size adaptation latency
- `I_max` = worst-case inference time (samples)
- `H_host` = host buffer size
- `M_int` = model's internal latency

The `get_latency()` API returns inference delay in samples for host latency compensation.

#### Benchmark Results (from paper)

**Stateless models (CNN, Hybrid)**:
1. ONNX Runtime -- fastest
2. LibTorch -- medium
3. TensorFlow Lite -- slowest (but competitive for small models)

**Stateful models (RNN)**:
1. LibTorch -- fastest
2. TensorFlow Lite -- slower
3. ONNX Runtime -- not supported

**Per-sample performance** (RNN-2k model): 5.26 x 10^-5 ms/sample
**Bypass overhead**: 5.26 - 3.78 x 10^-4 ms/sample (well below real-time threshold of 0.0208 ms/sample at 48kHz)

#### Real-Time Violation Analysis
All three backends exhibit violations:
- **LibTorch**: Highest frequency (malloc, free, pthread_mutex_lock)
- **TFLite**: Memory only (malloc, free, aligned_alloc)
- **ONNX Runtime**: Memory only (malloc, free, posix_memalign)

But: "Extensive testing of the anira library has revealed no violations" in the audio callback itself. The violations are contained within the background threads.

#### CMake Integration
```cmake
# As git submodule
add_subdirectory(anira)
target_link_libraries(MyPlugin anira::anira)

# Disable specific backends
set(ANIRA_WITH_LIBTORCH OFF)
set(ANIRA_WITH_TFLITE OFF)
# Only use ONNX Runtime
```

#### Pros
- Proven real-time safety for the audio callback
- Backend-agnostic (swap engines without rewriting code)
- Built-in latency management and reporting
- Built-in benchmarking tools
- JUCE plugin example included
- Peer-reviewed architecture

#### Cons
- Introduces latency (by design -- inference is buffered)
- Additional dependency layer
- All three backends must be available or selectively disabled
- Newer project, smaller community than individual backends

---

### 7. Nanoflare

**Source**: https://github.com/vegapit/nanoflare
**License**: MIT

#### Overview
Header-only C++17 library designed as a lightweight alternative to LibTorch, originally developed for audio plugins.

#### Key Features
- **25-30% faster than LibTorch/TorchScript JIT** across all tested architectures.
- Header-only (minimal integration complexity).
- Reads PyTorch model weights directly.
- No external dependencies beyond C++17 stdlib.

#### CMake Integration
```cmake
add_subdirectory(nanoflare)
target_include_directories(MyPlugin PRIVATE ${NANOFLARE_INCLUDE_DIRS})
```

#### Pros
- Significantly faster than LibTorch
- Tiny footprint (header-only)
- Direct PyTorch weight loading
- Originally built for audio plugins

#### Cons
- Smaller community and less documentation
- May not support all PyTorch layer types
- Less battle-tested than RTNeural or LibTorch
- Real-time safety characteristics not formally evaluated

---

### 8. Neutone SDK

**Source**: https://github.com/Neutone/neutone_sdk
**Website**: https://neutone.ai
**License**: MIT

#### Overview
An open-source framework for deploying PyTorch neural audio models to VST3/AU plugins. Provides host plugins (Neutone FX, Neutone Gen) that load models wrapped with the SDK.

#### Key Features
- Work entirely in Python (PyTorch) -- no C++ required for model development.
- Handles buffer sizes, sample rate conversion, delay compensation automatically.
- Model-agnostic interface.
- Host plugins available as VST3 and AU.

#### Pros
- Lowest barrier to entry (Python-only model development)
- Handles all real-time concerns internally
- Active community and competition ecosystem
- Accepted at AES AI/ML conference 2025

#### Cons
- Models run in Neutone's host plugin, not your own plugin
- Less control over DSP pipeline
- Dependency on Neutone ecosystem
- Not suitable for custom plugin development

---

## Comparative Analysis

### Performance Comparison Table

| Framework | Stateless Speed | Stateful Speed | Real-Time Safe | Deployment Size | JUCE Integration |
|-----------|----------------|----------------|---------------|-----------------|-----------------|
| **RTNeural** | Fast (small models) | Fast | YES | ~0 (header-only) | Excellent |
| **ONNX Runtime** | Fastest | N/A (no state) | NO | ~7.5MB (full), <5MB (trimmed) | Moderate |
| **LibTorch** | Medium | Fastest | NO | ~267MB (CPU) | Moderate |
| **TFLite** | Moderate | Moderate | NO | ~2-5MB | Moderate |
| **Anira** | Depends on backend | Depends on backend | YES (wrapper) | Backend + ~small | Good (example) |
| **Nanoflare** | Fast | Fast | Unknown | ~0 (header-only) | Easy |

### Architecture Support

| Layer Type | RTNeural | ONNX | LibTorch | TFLite |
|-----------|----------|------|----------|--------|
| Dense/FC | Yes | Yes | Yes | Yes |
| Conv1D | Yes | Yes | Yes | Yes |
| Conv2D | Yes | Yes | Yes | Yes |
| LSTM | Yes | Limited | Yes | Yes |
| GRU | Yes | Limited | Yes | Yes |
| Transformer | No | Yes | Yes | Yes |
| BatchNorm | Yes | Yes | Yes | Yes |
| Custom ops | No | Yes | Yes | Yes |

### Decision Matrix

| Use Case | Recommended Framework |
|----------|----------------------|
| Amp/pedal modeling (LSTM/GRU) | **RTNeural** (direct on audio thread) |
| Amp modeling with larger models | **LibTorch via Anira** (background thread) |
| Audio classification / feature extraction | **ONNX Runtime via Anira** |
| Audio-to-MIDI / pitch detection | **RTNeural + ONNX Runtime hybrid** (NeuralNote approach) |
| Timbre transfer / generative | **LibTorch via Anira** or **Neutone SDK** |
| Embedded / Raspberry Pi | **RTNeural** or **TFLite** |
| Rapid prototyping (Python-first) | **Neutone SDK** |
| Maximum flexibility / backend swapping | **Anira** |

---

## Real-Time Audio Thread Safety

### The Fundamental Constraint

The audio thread (`processBlock()` in JUCE) runs under hard real-time constraints:
- Must complete within the buffer period (e.g., 11.6ms at 44.1kHz/512 samples).
- **Cannot allocate memory** (malloc/new have unbounded time).
- **Cannot acquire locks** (mutex/semaphore can block indefinitely, cause priority inversion).
- **Cannot perform I/O** (file reads, network, system calls).
- **Cannot call into the OS thread scheduler**.

### What Each Framework Does Wrong

Based on ANIRA's RadSan (real-time sanitizer) analysis:

**LibTorch violations** (worst):
- `malloc`, `free` during inference
- `pthread_mutex_lock` (worst -- blocks audio thread)
- Especially bad during first few inferences (JIT warmup)

**ONNX Runtime violations**:
- `malloc`, `free`, `posix_memalign` during inference
- No mutex violations (better than LibTorch)

**TensorFlow Lite violations**:
- `malloc`, `free`, `aligned_alloc` during inference
- No mutex violations

**RTNeural**:
- No violations during forward pass
- Allocations only during construction/destruction (done in `prepareToPlay()`, not `processBlock()`)

### Safe Patterns for Audio Thread

```cpp
// SAFE: RTNeural on audio thread
void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float input = buffer.getSample(0, sample);
        float output = rtNeuralModel.forward(&input); // No allocations!
        buffer.setSample(0, sample, output);
    }
}

// SAFE: Atomic parameter updates
std::atomic<float> gain{1.0f};
// GUI thread: gain.store(newValue, std::memory_order_relaxed);
// Audio thread: float g = gain.load(std::memory_order_relaxed);

// UNSAFE: Any general ML framework on audio thread
void processBlock(...) override
{
    auto tensor = torch::from_blob(...); // ALLOCATES!
    auto output = model.forward({tensor}); // LOCKS!
}
```

---

## Architectural Patterns for ML in Audio Plugins

### Pattern 1: Direct Inference (RTNeural Only)

```
Audio Thread: input -> [RTNeural model] -> output
```

- Zero added latency.
- Sample-by-sample processing possible.
- Model must be small and fixed-architecture.
- Best for: amp modeling, saturation, simple effects.

### Pattern 2: Background Thread with Double Buffering

```
Audio Thread:
  1. Write input to ring buffer
  2. Read output from completed inference buffer
  3. Report latency to host

Background Thread:
  1. Wait for full input buffer
  2. Run inference (ONNX/LibTorch/TFLite)
  3. Write result to output buffer
  4. Signal completion (atomic flag)
```

- Adds latency: `ceil(model_input_size / host_buffer_size) * host_buffer_size` samples.
- Thread-safe data exchange via atomics or lock-free queues.
- Used by: Anira, nn-inference-template.

### Pattern 3: Hybrid Architecture (NeuralNote Approach)

```
Audio Thread:
  [RTNeural CNN] for lightweight inference

Background Thread:
  [ONNX Runtime] for heavy feature extraction
```

- Combines strengths: RTNeural for audio-rate processing, ONNX for batch processing.
- NeuralNote splits basic-pitch model: CNN runs via RTNeural, CQT + Harmonic Stacking via ONNX Runtime.
- Best for: complex pipelines with both real-time and non-real-time components.

### Pattern 4: Anira Thread Pool Architecture

```
Audio Thread (processBlock):
  -> InferenceManager (buffer alignment)
  -> SessionElement (atomic data exchange)

Static Thread Pool (pre-allocated):
  -> Backend inference (ONNX/LibTorch/TFLite)
  -> Result written to SessionElement

Audio Thread:
  <- Read completed result from SessionElement
  <- Output with latency compensation
```

- Most robust architecture for production plugins.
- Handles buffer size mismatches automatically.
- Built-in latency reporting for host compensation.
- Validated real-time safety via RadSan testing.

### Warm-Up Strategy

All general-purpose frameworks (especially LibTorch) exhibit elevated latency and violation counts during initial inferences. Implement warm-up:

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // Allocate RTNeural model
    rtModel.reset(sampleRate);

    // Warm up general-purpose backends
    float dummyInput[512] = {0};
    for (int i = 0; i < 10; ++i) {
        inferenceBackend.runInference(dummyInput, 512); // On non-audio thread
    }
}
```

### Look-Ahead Buffering

For models that require future context (non-causal convolutions, bidirectional RNNs):

1. **Output-Delayed Approach**: Delay output relative to input by N samples. The model processes `input[t-N:t]` to produce `output[t-N]`. Report N samples of latency to the host.

2. **Cached Convolution**: Cache intermediate convolution states across buffer boundaries. Used by IRCAM's [cached_conv](https://acids-ircam.github.io/cached_conv/) for streamable non-causal networks.

3. **Latency sources in neural audio**:
   - Buffering delay (waiting for full model input)
   - Representation delay (model internal lookahead)
   - Inference computation time
   - Jitter (variable inference time)

---

## Model Quantization for Audio Plugins

### Overview
Quantization reduces model weights and activations from 32-bit float (FP32) to lower precision (INT8, FP16), yielding:
- **~4x memory reduction** (FP32 to INT8)
- **~2-3x inference speedup** on modern CPUs
- Reduced power consumption and heat

### Quantization Approaches

| Method | Description | Quality Impact | When to Apply |
|--------|-------------|---------------|---------------|
| **Post-Training Dynamic** | Quantize weights statically, activations dynamically | Low | Quick optimization |
| **Post-Training Static** | Quantize both statically using calibration data | Low-Medium | Better speedup |
| **Quantization-Aware Training (QAT)** | Insert fake quantization during training | Minimal | Best quality preservation |

### Framework Support

| Framework | INT8 | FP16 | QAT | Tool |
|-----------|------|------|-----|------|
| ONNX Runtime | Yes | Yes | Via PyTorch | onnxruntime quantization API |
| LibTorch/PyTorch | Yes | Yes | Yes | torch.quantization |
| TensorFlow Lite | Yes | Yes | Yes | TFLite converter |
| RTNeural | No (FP32/FP64 only) | No | N/A | N/A |

### Audio-Specific Considerations

1. **Audio quality sensitivity**: Audio models are more sensitive to quantization artifacts than vision models. Always A/B test quantized vs. original.

2. **INT8 on x86 CPUs**: PyTorch INT8 quantization uses FBGEMM (x86) or QNNPACK (ARM) backends. ~3x speedup on x86 with minimal quality loss for dense/conv layers.

3. **RTNeural limitation**: RTNeural operates in FP32 or FP64 only. For RTNeural models, optimization comes from compile-time model definition and SIMD backends, not quantization.

4. **ONNX Runtime quantization**: Export model to ONNX, then use `onnxruntime.quantization` to produce INT8 model. Custom ORT build with reduced operators further shrinks binary.

5. **Recommended pipeline**:
   ```
   Train in PyTorch (FP32)
   -> Quantization-Aware Training (if quality-critical)
   -> Export to ONNX
   -> ONNX Runtime INT8 quantization
   -> Custom ORT build with pruned operators
   -> Deploy via Anira for real-time safety
   ```

### Intel Neural Compressor
For x86 deployment, Intel Neural Compressor automates quantization, pruning, and knowledge distillation. Supports PyTorch, TensorFlow, and ONNX models. Can produce optimized INT8 models with minimal quality loss.

---

## Recommendations by Use Case

### For This Project (JUCE + CMake VST3/AU Plugins)

**If building an amp/pedal modeling plugin**:
- Use **RTNeural** with compile-time model API.
- Train LSTM or GRU model in PyTorch, export weights to JSON.
- Run inference directly in `processBlock()`. Zero added latency.
- CMake: `add_subdirectory(RTNeural)` as git submodule.

**If building an ML-enhanced effect (e.g., intelligent EQ, de-noise, enhancement)**:
- Use **Anira** with ONNX Runtime backend.
- Train model in PyTorch, export to ONNX, optimize with quantization.
- Anira handles thread safety, buffer management, latency reporting.
- Accept added latency (~1-3 buffer periods depending on model size).

**If building a complex pipeline (feature extraction + real-time processing)**:
- Use **hybrid approach** (NeuralNote pattern).
- RTNeural for lightweight audio-rate inference on audio thread.
- ONNX Runtime (via Anira) for heavy batch processing on background threads.

**If maximum model flexibility is needed**:
- Use **Anira** with LibTorch backend for broadest architecture support.
- Accept the ~267MB deployment size or use Nanoflare as lighter alternative.
- Implement warm-up passes in `prepareToPlay()`.

### General Best Practices

1. **Always profile** on target hardware before choosing a framework.
2. **Use compile-time models** (RTNeural) when architecture is fixed.
3. **Warm up** general-purpose backends before audio processing starts.
4. **Report latency** to the host DAW for proper compensation.
5. **Benchmark** with the actual model, not synthetic tests.
6. **Consider deployment size** -- RTNeural adds essentially zero, LibTorch adds ~267MB.
7. **Test in DAW** -- standalone performance != plugin performance due to host overhead.

---

## Key Sources

- [RTNeural GitHub](https://github.com/jatinchowdhury18/RTNeural) - Jatin Chowdhury
- [RTNeural Paper](https://arxiv.org/abs/2106.03037) - arXiv:2106.03037
- [ANIRA Paper](https://arxiv.org/abs/2506.12665) - arXiv:2506.12665
- [Anira GitHub](https://github.com/anira-project/anira)
- [nn-inference-template](https://github.com/Torsion-Audio/nn-inference-template) - Torsion Audio (ADC23)
- [Neural Amp Modeler](https://github.com/sdatkinson/neural-amp-modeler) - Steve Atkinson
- [NeuralAmpModelerPlugin](https://github.com/sdatkinson/NeuralAmpModelerPlugin)
- [ONNX Runtime](https://onnxruntime.ai/) - Microsoft
- [ort-builder](https://github.com/olilarkin/ort-builder) - Oli Larkin
- [iPlug2OnnxRuntime](https://github.com/olilarkin/iPlug2OnnxRuntime) - Oli Larkin
- [Nanoflare](https://github.com/vegapit/nanoflare) - Lightweight LibTorch alternative
- [NeuralNote](https://github.com/DamRsn/NeuralNote) - Hybrid RTNeural + ONNX approach
- [GuitarML](https://guitarml.com/) - Open source neural guitar plugins
- [AIDA-X](https://github.com/AidaDSP/AIDA-X) - Neural amp modeling plugin
- [Neutone SDK](https://github.com/Neutone/neutone_sdk) - PyTorch to audio plugin framework
- [JUCE Forum: Using Locks in Real-Time Audio](https://timur.audio/using-locks-in-real-time-audio-processing-safely) - Timur Doumler
- [WolfTalk #011](https://thewolfsound.com/talk011/) - Andrew Fyfe on neural networks in audio plugins
- [Towards a CPU-efficient NAM](https://www.neuralampmodeler.com/post/towards-a-good-cpu-efficient-nam)
- [Intel Neural Compressor](https://www.intel.com/content/www/us/en/developer/tools/oneapi/neural-compressor.html)
