---
title: "Machine Learning Integration in JUCE Audio Plugins"
created: 2026-02-08
domain: ml
type: guide
keywords:
  - machine-learning
  - juce
  - ml-integration
  - real-time
  - audio-plugins
  - onnx
---
# Machine Learning Integration in JUCE Audio Plugins

## Practical Implementation Guide

---

## Table of Contents

1. [Architecture Patterns](#1-architecture-patterns)
2. [Real-Time Safety](#2-real-time-safety)
3. [Training Pipelines](#3-training-pipelines)
4. [CMake Integration](#4-cmake-integration)
5. [Performance Optimization](#5-performance-optimization)
6. [Open Source Reference Projects](#6-open-source-reference-projects)
7. [Decision Matrix](#7-decision-matrix)

---

## 1. Architecture Patterns

### 1.1 The Core Problem

Neural network inference in audio plugins must run within the audio callback deadline. At 44.1kHz with a 128-sample buffer, the audio thread has approximately **2.9 milliseconds** to process each block. Any deadline miss produces audible glitches.

There are three fundamental architecture patterns, ranging from simplest to most complex:

```
Pattern A: Direct Audio-Thread Inference (RTNeural)
Pattern B: Background Thread with Double Buffer (Custom / anira)
Pattern C: Full Async Pipeline with Lock-Free Queues (Custom)
```

### 1.2 Pattern A: Direct Audio-Thread Inference

**When to use:** Small models (< 100 parameters), guaranteed fixed execution time, sample-by-sample processing (amp modeling, distortion).

This is the simplest pattern. The neural network runs directly inside `processBlock()`. This is ONLY safe when using a library like RTNeural that guarantees real-time safety -- no memory allocation, no locks, deterministic execution time.

```
+------------------------------------------------------------------+
|                        Audio Thread                               |
|                                                                   |
|  processBlock(buffer)                                             |
|  +-------------------------------------------------------------+ |
|  | for each sample:                                             | |
|  |   input[0] = sample                                          | |
|  |   output = neuralNet->forward(input)   <-- RTNeural          | |
|  |   sample = output                                            | |
|  +-------------------------------------------------------------+ |
+------------------------------------------------------------------+
```

**JUCE Implementation:**

```cpp
class NeuralDistortionProcessor : public juce::AudioProcessor
{
public:
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        // Load model weights from binary data (done once, NOT on audio thread)
        juce::MemoryInputStream jsonStream(
            BinaryData::model_weights_json,
            BinaryData::model_weights_jsonSize, false);

        auto jsonInput = nlohmann::json::parse(
            jsonStream.readEntireStreamAsString().toStdString());

        // Create one model per channel for stereo
        for (int ch = 0; ch < 2; ++ch)
        {
            neuralNet[ch] = RTNeural::json_parser::parseJson<float>(jsonInput);
            neuralNet[ch]->reset();
        }
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer&) override
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* x = buffer.getWritePointer(ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                float input[] = { x[n] };
                x[n] = neuralNet[ch]->forward(input);
            }
        }
    }

private:
    std::unique_ptr<RTNeural::Model<float>> neuralNet[2];
};
```

**Compile-Time RTNeural (Faster):**

When the model architecture is known at compile time, use RTNeural's template API for significantly better performance. The compiler can unroll loops and optimize aggressively:

```cpp
// Define model architecture at compile time
using ModelType = RTNeural::ModelT<float, 1, 1,
    RTNeural::DenseT<float, 1, 8>,
    RTNeural::TanhActivationT<float, 8>,
    RTNeural::DenseT<float, 8, 8>,
    RTNeural::TanhActivationT<float, 8>,
    RTNeural::DenseT<float, 8, 1>
>;

class NeuralDistortionProcessor : public juce::AudioProcessor
{
    ModelType model;  // Stack-allocated, no heap allocation

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        // Load weights into compile-time model
        auto jsonStream = juce::MemoryInputStream(
            BinaryData::model_weights_json,
            BinaryData::model_weights_jsonSize, false);

        auto jsonInput = nlohmann::json::parse(
            jsonStream.readEntireStreamAsString().toStdString());

        RTNeural::torch_helpers::loadDense<float>(
            jsonInput, "layers.0", model.get<0>());
        // ... load remaining layers
        model.reset();
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer&) override
    {
        auto* x = buffer.getWritePointer(0);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            x[n] = model.forward(x + n);
        }
    }
};
```

### 1.3 Pattern B: Background Thread with Double Buffer

**When to use:** Larger models, models with variable execution time, models requiring fixed input block sizes different from the host buffer size.

The audio thread writes into one buffer while the inference thread reads from another. A lock-free swap mechanism alternates between them.

```
+------------------+        +------------------------+       +------------------+
|   Audio Thread   |        |    Inference Thread     |      |   Audio Thread   |
|                  |        |                         |      |                  |
| Write input to   |------->| Read from Buffer A      |      | Read output from |
| Buffer A         |        | Run inference           |      | Result Buffer    |
|                  |        | Write result to         |----->|                  |
| Read output from |        | Result Buffer           |      |                  |
| Result Buffer    |        |                         |      |                  |
+------------------+        +------------------------+       +------------------+

       ^                                                           |
       |                  Lock-Free Swap                           |
       +-----------------------------------------------------------+
```

**JUCE Implementation with juce::Thread:**

```cpp
class InferenceThread : public juce::Thread
{
public:
    InferenceThread() : juce::Thread("ML-Inference") {}

    void prepare(int blockSize)
    {
        inferenceBlockSize = blockSize;
        inputBuffer.resize(blockSize, 0.0f);
        outputBuffer.resize(blockSize, 0.0f);
        resultBuffer.resize(blockSize, 0.0f);
        inputReady.store(false);
        outputReady.store(false);
    }

    // Called from audio thread -- lock-free
    void submitInput(const float* data, int numSamples)
    {
        if (numSamples != inferenceBlockSize) return;

        // Only write if inference thread is not currently reading
        if (!inputReady.load(std::memory_order_acquire))
        {
            std::copy(data, data + numSamples, inputBuffer.begin());
            inputReady.store(true, std::memory_order_release);
        }
    }

    // Called from audio thread -- lock-free
    bool getOutput(float* data, int numSamples)
    {
        if (outputReady.load(std::memory_order_acquire))
        {
            std::copy(resultBuffer.begin(),
                      resultBuffer.begin() + numSamples, data);
            outputReady.store(false, std::memory_order_release);
            return true;
        }
        return false;
    }

    void run() override
    {
        while (!threadShouldExit())
        {
            if (inputReady.load(std::memory_order_acquire))
            {
                // Run inference (safe to allocate, take time, etc.)
                runInference(inputBuffer.data(), outputBuffer.data(),
                             inferenceBlockSize);

                // Copy result to output buffer
                std::copy(outputBuffer.begin(), outputBuffer.end(),
                          resultBuffer.begin());

                inputReady.store(false, std::memory_order_release);
                outputReady.store(true, std::memory_order_release);
            }
            else
            {
                // Sleep briefly to avoid busy-waiting
                wait(1);  // 1ms sleep
            }
        }
    }

private:
    virtual void runInference(const float* input, float* output,
                              int numSamples) = 0;

    int inferenceBlockSize = 256;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::vector<float> resultBuffer;
    std::atomic<bool> inputReady{false};
    std::atomic<bool> outputReady{false};
};
```

### 1.4 Pattern C: Full Async Pipeline with Lock-Free Ring Buffers

**When to use:** Complex models, variable-length inference, need to decouple audio block size from model block size, latency-tolerant applications.

This pattern uses JUCE's `AbstractFifo` for lock-free communication between the audio thread and a background inference thread.

```
+---------------------------------------------------------------------+
|                       FULL ASYNC PIPELINE                            |
|                                                                      |
|  Audio Thread            Ring Buffers          Inference Thread       |
|  +-----------+      +------------------+      +------------------+   |
|  |           |      |                  |      |                  |   |
|  | processBlock ====>  Input FIFO      =====>  Accumulate to     |   |
|  |           |      |  (AbstractFifo)  |      |  model block     |   |
|  |           |      +------------------+      |  size (e.g. 256) |   |
|  |           |                                |                  |   |
|  |           |      +------------------+      |  Run inference   |   |
|  |           | <====  Output FIFO      <=====  Write results     |   |
|  |           |      |  (AbstractFifo)  |      |                  |   |
|  +-----------+      +------------------+      +------------------+   |
|                                                                      |
|  Report latency = model_block_size + processing_time_in_samples      |
+---------------------------------------------------------------------+
```

**JUCE Implementation with AbstractFifo:**

```cpp
class AsyncMLProcessor : public juce::AudioProcessor
{
public:
    static constexpr int MODEL_BLOCK_SIZE = 256;
    static constexpr int FIFO_SIZE = 4096;  // Must be > MODEL_BLOCK_SIZE

    AsyncMLProcessor()
        : inputFifo(FIFO_SIZE), outputFifo(FIFO_SIZE)
    {
        inputRingBuffer.resize(FIFO_SIZE, 0.0f);
        outputRingBuffer.resize(FIFO_SIZE, 0.0f);
    }

    void prepareToPlay(double sampleRate, int) override
    {
        // Report latency to host for delay compensation
        setLatencySamples(MODEL_BLOCK_SIZE);

        inputFifo.reset();
        outputFifo.reset();

        inferenceThread.prepare(sampleRate, MODEL_BLOCK_SIZE);
        inferenceThread.startThread(juce::Thread::Priority::high);
    }

    void releaseResources() override
    {
        inferenceThread.stopThread(2000);
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer&) override
    {
        const int numSamples = buffer.getNumSamples();
        auto* channelData = buffer.getWritePointer(0);

        // --- Write input samples into FIFO (audio thread = producer) ---
        {
            const auto scope = inputFifo.write(numSamples);
            if (scope.blockSize1 > 0)
                std::copy(channelData, channelData + scope.blockSize1,
                          inputRingBuffer.data() + scope.startIndex1);
            if (scope.blockSize2 > 0)
                std::copy(channelData + scope.blockSize1,
                          channelData + scope.blockSize1 + scope.blockSize2,
                          inputRingBuffer.data() + scope.startIndex2);
        }

        // --- Read output samples from FIFO (audio thread = consumer) ---
        {
            const int available = outputFifo.getNumReady();
            const int toRead = juce::jmin(numSamples, available);

            if (toRead > 0)
            {
                const auto scope = outputFifo.read(toRead);
                if (scope.blockSize1 > 0)
                    std::copy(outputRingBuffer.data() + scope.startIndex1,
                              outputRingBuffer.data() + scope.startIndex1
                                  + scope.blockSize1,
                              channelData);
                if (scope.blockSize2 > 0)
                    std::copy(outputRingBuffer.data() + scope.startIndex2,
                              outputRingBuffer.data() + scope.startIndex2
                                  + scope.blockSize2,
                              channelData + scope.blockSize1);
            }
            else
            {
                // No output ready yet -- output silence
                std::fill(channelData, channelData + numSamples, 0.0f);
            }
        }

        // Notify inference thread that new data is available
        inferenceThread.notify();
    }

private:
    juce::AbstractFifo inputFifo, outputFifo;
    std::vector<float> inputRingBuffer, outputRingBuffer;

    // The inference thread reads from inputFifo, runs inference,
    // and writes to outputFifo. See InferenceThread implementation above.
    InferenceThread inferenceThread;
};
```

### 1.5 Using anira (Recommended for Production)

The **anira** library provides a production-ready implementation of Pattern C with support for LibTorch, ONNX Runtime, and TensorFlow Lite. It handles thread pool management, latency computation, and real-time safety automatically.

```cpp
#include <anira/anira.h>

class AniraProcessor : public juce::AudioProcessor
{
public:
    AniraProcessor()
        : inferenceHandler(prePostProcessor, inferenceConfig)
    {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        inferenceHandler.prepare({samplesPerBlock, sampleRate});
        inferenceHandler.set_inference_backend(
            anira::InferenceBackend::ONNX);
        setLatencySamples(inferenceHandler.getLatency());
    }

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer&) override
    {
        // anira handles all threading internally
        auto* data = buffer.getWritePointer(0);
        inferenceHandler.process(data, buffer.getNumSamples());
    }

private:
    anira::InferenceConfig inferenceConfig{
        {{"model.onnx", anira::InferenceBackend::ONNX}},
        {{{256, 1, 1}}, {{256, 1}}},  // input/output shapes
        5.33f                          // max inference time (ms)
    };
    anira::PrePostProcessor prePostProcessor{inferenceConfig};
    anira::InferenceHandler inferenceHandler;
};
```

---

## 2. Real-Time Safety

### 2.1 Why ML Inference on the Audio Thread is Problematic

Standard ML frameworks (PyTorch, TensorFlow, ONNX Runtime) violate real-time audio constraints:

| Violation | Why It Matters | Example |
|-----------|---------------|---------|
| **Memory allocation** | `malloc` can block for unbounded time (OS page fault, lock contention) | Creating tensors, resizing buffers |
| **Mutex locks** | Thread scheduler involvement, priority inversion | Internal framework synchronization |
| **Variable execution time** | Different input values can trigger different code paths | Conditional layers, dynamic shapes |
| **System calls** | Kernel transitions, context switches | File I/O, logging |
| **Exception handling** | Stack unwinding is non-deterministic | Error handling in frameworks |

### 2.2 When You CAN Run Inference on the Audio Thread

You can safely run inference on the audio thread when ALL of these conditions are met:

1. **Pre-allocated buffers:** All memory allocated during `prepareToPlay()`, not `processBlock()`
2. **Fixed computation graph:** No conditionals that change execution path based on input
3. **No internal locks:** The inference library is verified lock-free
4. **Deterministic execution time:** Same number of operations regardless of input values
5. **Small model:** Inference completes well within the buffer deadline

**RTNeural satisfies all five conditions** for its supported layer types. It is currently the only mainstream library designed specifically for real-time audio-rate inference.

### 2.3 When You NEED a Separate Thread

Use background thread inference when:

- Model inference time exceeds ~50% of the buffer deadline
- Using ONNX Runtime, LibTorch, or TensorFlow Lite (none are real-time safe)
- Model has variable execution time
- Model requires a fixed input size different from host buffer size
- Running multiple models simultaneously

### 2.4 Lock-Free Queue Patterns for Audio-to-Inference Communication

**JUCE's AbstractFifo** provides a lock-free single-producer, single-consumer FIFO:

```cpp
// AbstractFifo usage pattern
struct AudioToInferenceFifo
{
    static constexpr int FIFO_CAPACITY = 8192;

    juce::AbstractFifo fifo{FIFO_CAPACITY};
    float buffer[FIFO_CAPACITY];

    // Called from AUDIO THREAD (producer)
    void pushSamples(const float* data, int numSamples)
    {
        const auto scope = fifo.write(numSamples);

        if (scope.blockSize1 > 0)
            std::memcpy(buffer + scope.startIndex1,
                        data, scope.blockSize1 * sizeof(float));

        if (scope.blockSize2 > 0)
            std::memcpy(buffer + scope.startIndex2,
                        data + scope.blockSize1,
                        scope.blockSize2 * sizeof(float));
    }

    // Called from INFERENCE THREAD (consumer)
    int pullSamples(float* dest, int maxSamples)
    {
        const int available = fifo.getNumReady();
        const int toRead = juce::jmin(maxSamples, available);

        if (toRead == 0) return 0;

        const auto scope = fifo.read(toRead);

        if (scope.blockSize1 > 0)
            std::memcpy(dest, buffer + scope.startIndex1,
                        scope.blockSize1 * sizeof(float));

        if (scope.blockSize2 > 0)
            std::memcpy(dest + scope.blockSize1,
                        buffer + scope.startIndex2,
                        scope.blockSize2 * sizeof(float));

        return toRead;
    }
};
```

### 2.5 Atomic Double-Buffer Pattern

For simpler cases where only the latest result matters (e.g., ML-driven parameter prediction):

```cpp
template <typename T>
class DoubleBuffer
{
public:
    // Called from inference thread
    void write(const T& value)
    {
        int writeIdx = 1 - activeIndex.load(std::memory_order_relaxed);
        buffers[writeIdx] = value;
        activeIndex.store(writeIdx, std::memory_order_release);
        newDataAvailable.store(true, std::memory_order_release);
    }

    // Called from audio thread
    bool read(T& value)
    {
        if (!newDataAvailable.load(std::memory_order_acquire))
            return false;

        int readIdx = activeIndex.load(std::memory_order_acquire);
        value = buffers[readIdx];
        newDataAvailable.store(false, std::memory_order_release);
        return true;
    }

private:
    std::array<T, 2> buffers;
    std::atomic<int> activeIndex{0};
    std::atomic<bool> newDataAvailable{false};
};
```

---

## 3. Training Pipelines

### 3.1 Common Neural Network Architectures for Audio

#### WaveNet (Dilated Causal Convolutions)

- **Use case:** Amp/pedal modeling, nonlinear distortion emulation
- **Strengths:** Captures long-range temporal dependencies via dilated convolutions, excellent accuracy
- **Weaknesses:** Higher CPU cost than RNNs, longer training time (~1500 epochs)
- **Receptive field:** Exponentially increases with dilated layers (e.g., dilations [1, 2, 4, 8, 16, ...])
- **Real-time:** Yes, with optimized C++ implementation (sample-by-sample or small blocks)

```
Input --> [Conv1D d=1] --> [Conv1D d=2] --> [Conv1D d=4] --> ... --> Output
              |                 |                 |
              +---- Residual ---+---- Residual ---+
```

#### LSTM (Long Short-Term Memory)

- **Use case:** Amp modeling, especially high-gain and dynamic response
- **Strengths:** Excellent at capturing dynamic, nonlinear behavior; moderate training time
- **Weaknesses:** Sequential processing (hard to parallelize), state management
- **Real-time:** Yes, particularly "stateful LSTM" where hidden state persists across blocks

```
Input(t) --> [LSTM Cell] --> [Dense] --> Output(t)
                 |
                 v
            Hidden State (persists between samples)
```

#### GRU (Gated Recurrent Unit)

- **Use case:** Similar to LSTM with fewer parameters
- **Strengths:** Simpler than LSTM, faster training, comparable quality for many use cases
- **Weaknesses:** May not capture extremely complex dynamics as well as LSTM
- **Real-time:** Yes, supported by RTNeural

#### TCN (Temporal Convolutional Network)

- **Use case:** Amp modeling with larger receptive fields
- **Strengths:** Parallelizable (unlike RNNs), larger receptive fields
- **Weaknesses:** More parameters, higher memory usage
- **Real-time:** Yes, with fixed dilation patterns

### 3.2 Dataset Preparation for Amp Modeling

**Required recordings:**

1. **Input signal (dry/DI):** Clean guitar recorded direct (DI box), 44.1kHz or 48kHz, 24-bit, mono WAV
2. **Target signal (reamped):** The input signal played through the target amp/pedal and recorded

**Recording guidelines:**

- Minimum **3-4 minutes** of varied playing (clean passages, strumming, palm mutes, leads)
- Both files must be **sample-aligned** (use reamping to ensure this)
- Normalize to Float32 range [-1.0, 1.0]
- Train/validation split: 80/20

**Data preparation script (Python):**

```python
import numpy as np
import soundfile as sf

# Load aligned input/output pairs
input_audio, sr = sf.read('guitar_DI.wav', dtype='float32')
target_audio, _ = sf.read('guitar_amped.wav', dtype='float32')

# Ensure mono
if input_audio.ndim > 1:
    input_audio = input_audio[:, 0]
if target_audio.ndim > 1:
    target_audio = target_audio[:, 0]

# Trim to same length
min_len = min(len(input_audio), len(target_audio))
input_audio = input_audio[:min_len]
target_audio = target_audio[:min_len]

# Normalize
input_audio = input_audio / np.max(np.abs(input_audio))
target_audio = target_audio / np.max(np.abs(target_audio))

# Split into chunks for training
chunk_size = 4096
num_chunks = min_len // chunk_size

X = input_audio[:num_chunks * chunk_size].reshape(num_chunks, chunk_size)
Y = target_audio[:num_chunks * chunk_size].reshape(num_chunks, chunk_size)

# Train/val split
split = int(0.8 * num_chunks)
X_train, X_val = X[:split], X[split:]
Y_train, Y_val = Y[:split], Y[split:]
```

### 3.3 Training an LSTM Model (PyTorch)

Based on the GuitarLSTM / PedalNetRT approach:

```python
import torch
import torch.nn as nn

class GuitarLSTM(nn.Module):
    def __init__(self, input_size=1, hidden_size=32, num_layers=1):
        super().__init__()
        self.lstm = nn.LSTM(
            input_size=input_size,
            hidden_size=hidden_size,
            num_layers=num_layers,
            batch_first=True
        )
        self.dense = nn.Linear(hidden_size, 1)

    def forward(self, x):
        # x shape: (batch, seq_len, 1)
        lstm_out, _ = self.lstm(x)
        # Take last timestep output
        out = self.dense(lstm_out[:, -1, :])
        return out

# Training loop
model = GuitarLSTM(hidden_size=32)
optimizer = torch.optim.Adam(model.parameters(), lr=0.001)
criterion = nn.MSELoss()

for epoch in range(300):
    for batch_x, batch_y in dataloader:
        optimizer.zero_grad()
        pred = model(batch_x.unsqueeze(-1))  # Add feature dim
        loss = criterion(pred, batch_y.unsqueeze(-1))
        loss.backward()
        optimizer.step()
```

### 3.4 Exporting Trained Models for C++ Inference

#### Option A: Export to JSON for RTNeural

```python
import json
import numpy as np

def export_to_rtneural_json(model, filepath):
    """Export PyTorch model weights to JSON for RTNeural."""
    state = model.state_dict()
    weights = {}

    for name, param in state.items():
        # Convert to numpy, then to list for JSON serialization
        weights[name] = param.detach().cpu().numpy().tolist()

    model_config = {
        "in_shape": [1],
        "layers": []
    }

    # For LSTM model
    if hasattr(model, 'lstm'):
        model_config["layers"].append({
            "type": "lstm",
            "shape": [model.lstm.hidden_size],
            "weights": {
                "W_ih": weights["lstm.weight_ih_l0"],
                "W_hh": weights["lstm.weight_hh_l0"],
                "b_ih": weights["lstm.bias_ih_l0"],
                "b_hh": weights["lstm.bias_hh_l0"]
            }
        })

    if hasattr(model, 'dense'):
        model_config["layers"].append({
            "type": "dense",
            "shape": [1],
            "weights": {
                "weight": weights["dense.weight"],
                "bias": weights["dense.bias"]
            }
        })

    with open(filepath, 'w') as f:
        json.dump(model_config, f, indent=2)

export_to_rtneural_json(model, "model_weights.json")
```

#### Option B: Export to ONNX

```python
import torch

# Create dummy input matching expected shape
dummy_input = torch.randn(1, 256, 1)  # (batch, seq_len, features)

torch.onnx.export(
    model,
    dummy_input,
    "model.onnx",
    input_names=["audio_input"],
    output_names=["audio_output"],
    dynamic_axes={
        "audio_input": {0: "batch_size"},
        "audio_output": {0: "batch_size"}
    },
    opset_version=17
)
```

#### Option C: Export to TorchScript

```python
# Tracing (for models without control flow)
traced = torch.jit.trace(model, dummy_input)
traced.save("model.pt")

# Scripting (for models with control flow)
scripted = torch.jit.script(model)
scripted.save("model_scripted.pt")
```

### 3.5 PedalNetRT WaveNet Export Pattern

The PedalNetRT project uses a specific export pattern that converts PyTorch WaveNet weights to JSON, transposing convolution weights from PyTorch format `(out_channels, in_channels, kernel_size)` to the format expected by the C++ WaveNet implementation `(kernel_size, in_channels, out_channels)`:

```python
# From PedalNetRT export.py -- weight dimension transposition
for name, param in state_dict.items():
    if "weight" in name and "conv" in name:
        # Transpose: PyTorch (out, in, kernel) -> C++ (kernel, in, out)
        param = param.permute(2, 1, 0)
    weights[name] = param.numpy().flatten().tolist()
```

---

## 4. CMake Integration

### 4.1 RTNeural Integration

RTNeural is the easiest ML library to integrate with JUCE. It can be included as a header-only library or built as a static library.

**Method A: Git Submodule + add_subdirectory**

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyNeuralPlugin VERSION 1.0.0)

# Add JUCE
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE)

# Add RTNeural with chosen backend
set(RTNEURAL_STL ON CACHE BOOL "Use STL backend" FORCE)
# Alternatives:
# set(RTNEURAL_XSIMD ON CACHE BOOL "Use XSIMD backend" FORCE)
# set(RTNEURAL_EIGEN ON CACHE BOOL "Use Eigen backend" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/modules/RTNeural)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/modules/RTNeural)

# Define plugin
juce_add_plugin(MyNeuralPlugin
    COMPANY_NAME "MyCompany"
    PLUGIN_MANUFACTURER_CODE Myco
    PLUGIN_CODE Mnpl
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My Neural Plugin"
)

# Add source files
target_sources(MyNeuralPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# Link RTNeural and JUCE modules
target_link_libraries(MyNeuralPlugin
    PRIVATE
        RTNeural
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
)

# Embed model weights as binary data
juce_add_binary_data(ModelData SOURCES
    Resources/model_weights.json
)
target_link_libraries(MyNeuralPlugin PRIVATE ModelData)
```

**Method B: FetchContent (no submodule needed)**

```cmake
include(FetchContent)

FetchContent_Declare(
    RTNeural
    GIT_REPOSITORY https://github.com/jatinchowdhury18/RTNeural.git
    GIT_TAG main
)

set(RTNEURAL_XSIMD ON CACHE BOOL "Use XSIMD backend" FORCE)
FetchContent_MakeAvailable(RTNeural)

target_link_libraries(MyNeuralPlugin PRIVATE RTNeural)
```

### 4.2 ONNX Runtime Integration

ONNX Runtime is more complex because it is not designed to be built as a static library. Most developers use the pre-built dynamic library.

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyOnnxPlugin VERSION 1.0.0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE)

# --- ONNX Runtime ---
# Option A: Pre-built binary (recommended)
set(ONNXRUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/modules/onnxruntime")
find_library(ONNXRUNTIME_LIB onnxruntime
    PATHS ${ONNXRUNTIME_DIR}/lib
    NO_DEFAULT_PATH
)

juce_add_plugin(MyOnnxPlugin
    COMPANY_NAME "MyCompany"
    PLUGIN_MANUFACTURER_CODE Myco
    PLUGIN_CODE Moxp
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My ONNX Plugin"
)

target_sources(MyOnnxPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(MyOnnxPlugin PRIVATE
    ${ONNXRUNTIME_DIR}/include
)

target_link_libraries(MyOnnxPlugin
    PRIVATE
        ${ONNXRUNTIME_LIB}
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
)

# --- Platform-specific ONNX Runtime deployment ---
if(APPLE)
    # Copy dylib next to plugin bundle
    set_target_properties(MyOnnxPlugin PROPERTIES
        INSTALL_RPATH "@loader_path/../Frameworks"
    )
elseif(WIN32)
    # Copy DLL to output directory
    add_custom_command(TARGET MyOnnxPlugin POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_DIR}/lib/onnxruntime.dll"
            $<TARGET_FILE_DIR:MyOnnxPlugin>
    )
endif()
```

**ONNX Runtime deployment challenge:** Since ONNX Runtime is dynamically linked, you must distribute the shared library (`.dylib` / `.dll` / `.so`) alongside your plugin. To avoid conflicts with other plugins that may also bundle ONNX Runtime, consider building ONNX Runtime yourself with a custom library name.

### 4.3 anira Integration

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyAniraPlugin VERSION 1.0.0)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE)

# Add anira -- select which backends to include
set(ANIRA_WITH_LIBTORCH OFF CACHE BOOL "" FORCE)
set(ANIRA_WITH_ONNXRUNTIME ON CACHE BOOL "" FORCE)
set(ANIRA_WITH_TFLITE OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/modules/anira)

juce_add_plugin(MyAniraPlugin
    COMPANY_NAME "MyCompany"
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "My Anira Plugin"
)

target_sources(MyAniraPlugin PRIVATE
    Source/PluginProcessor.cpp
)

target_link_libraries(MyAniraPlugin
    PRIVATE
        anira::anira
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
)
```

### 4.4 Static vs Dynamic Linking Considerations

| Aspect | Static Linking | Dynamic Linking |
|--------|---------------|-----------------|
| **Distribution** | Single binary, simpler | Must bundle shared libraries |
| **Plugin conflicts** | No symbol collisions | Potential version conflicts between plugins |
| **Binary size** | Larger per-plugin | Smaller per-plugin, shared library separate |
| **Build complexity** | More complex build | Simpler build, complex deployment |
| **RTNeural** | Fully static, header-only option | N/A (always static) |
| **ONNX Runtime** | Difficult (not officially supported as static) | Recommended approach |
| **LibTorch** | Possible but large (~100MB+) | Recommended for size |

**Recommendation:** Use RTNeural (static) for small models. Use ONNX Runtime (dynamic) or anira for larger models, and handle distribution carefully.

### 4.5 Cross-Platform Build Considerations

```cmake
# Platform-specific optimizations
if(APPLE)
    # Use Accelerate framework for RTNeural
    # (RTNeural auto-detects on macOS)
    target_link_libraries(MyPlugin PRIVATE "-framework Accelerate")

    # Universal binary (Intel + Apple Silicon)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13")

elseif(WIN32)
    # Enable AVX2 for SIMD optimizations
    target_compile_options(MyPlugin PRIVATE /arch:AVX2)

    # Static MSVC runtime for standalone distribution
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

elseif(UNIX)
    # Enable AVX2 on Linux
    target_compile_options(MyPlugin PRIVATE -mavx2 -mfma)
endif()
```

---

## 5. Performance Optimization

### 5.1 Model Quantization

| Precision | Memory | Speed | Accuracy Loss | Use Case |
|-----------|--------|-------|---------------|----------|
| Float32 | Baseline | Baseline | None | Training, high-quality inference |
| Float16 | 50% | 1.5-2x faster (GPU) | Minimal | GPU inference, mobile |
| Int8 | 75% | 2-4x faster (CPU w/ VNNI) | <1% typical | CPU inference, embedded |
| Int4 | 87.5% | Varies | Higher | Experimental |

**For audio plugins:** Float32 is standard. Float16 offers minimal benefit on CPU. Int8 can help on Intel CPUs with VNNI (AVX-512) support but requires quantization-aware training or post-training calibration.

**ONNX Runtime quantization example:**

```python
from onnxruntime.quantization import quantize_dynamic, QuantType

quantize_dynamic(
    model_input="model_fp32.onnx",
    model_output="model_int8.onnx",
    weight_type=QuantType.QInt8
)
```

### 5.2 SIMD Optimization

RTNeural handles SIMD automatically through its backend selection:

- **XSIMD backend:** Uses portable SIMD abstractions (SSE, AVX, NEON)
- **Eigen backend:** Eigen's expression templates auto-vectorize
- **Accelerate backend:** Apple's vDSP/BLAS optimized for Apple Silicon

**Manual SIMD tips:**

```cmake
# Enable AVX for RTNeural
set(RTNEURAL_USE_AVX ON CACHE BOOL "Enable AVX" FORCE)
```

For custom SIMD code in layer implementations:

```cpp
// Using JUCE's FloatVectorOperations for SIMD-optimized operations
juce::FloatVectorOperations::multiply(output, weights, numSamples);
juce::FloatVectorOperations::add(output, bias, numSamples);

// Or using XSIMD directly
#include <xsimd/xsimd.hpp>
using batch_type = xsimd::batch<float>;
constexpr auto simd_size = batch_type::size;

void vectorized_relu(float* data, int size)
{
    auto zero = batch_type(0.0f);
    int i = 0;
    for (; i + simd_size <= size; i += simd_size)
    {
        auto v = xsimd::load_aligned(data + i);
        auto result = xsimd::max(v, zero);
        xsimd::store_aligned(data + i, result);
    }
    // Handle remainder
    for (; i < size; ++i)
        data[i] = std::max(data[i], 0.0f);
}
```

### 5.3 Batch Processing Strategies

When using models with fixed input sizes (e.g., 256 samples), but the host provides variable buffer sizes (e.g., 64, 128, 512):

```cpp
class FixedBlockProcessor
{
public:
    static constexpr int MODEL_BLOCK = 256;

    void prepare(double sampleRate)
    {
        inputAccumulator.resize(MODEL_BLOCK, 0.0f);
        outputBuffer.resize(MODEL_BLOCK, 0.0f);
        accumulatedSamples = 0;
        outputReadPos = 0;
        outputAvailable = 0;
    }

    void process(float* buffer, int numSamples)
    {
        int bufferPos = 0;

        while (bufferPos < numSamples)
        {
            // If we have output available, copy to output
            if (outputAvailable > 0)
            {
                int toCopy = juce::jmin(numSamples - bufferPos,
                                        outputAvailable);
                std::copy(outputBuffer.data() + outputReadPos,
                          outputBuffer.data() + outputReadPos + toCopy,
                          buffer + bufferPos);
                outputReadPos += toCopy;
                outputAvailable -= toCopy;
                bufferPos += toCopy;
            }

            // Accumulate input
            int toAccumulate = juce::jmin(
                numSamples - bufferPos,
                MODEL_BLOCK - accumulatedSamples);

            std::copy(buffer + bufferPos,
                      buffer + bufferPos + toAccumulate,
                      inputAccumulator.data() + accumulatedSamples);
            accumulatedSamples += toAccumulate;
            bufferPos += toAccumulate;

            // Run inference when we have a full block
            if (accumulatedSamples == MODEL_BLOCK)
            {
                runInference(inputAccumulator.data(),
                             outputBuffer.data(), MODEL_BLOCK);
                accumulatedSamples = 0;
                outputReadPos = 0;
                outputAvailable = MODEL_BLOCK;
            }
        }
    }

private:
    virtual void runInference(const float* in, float* out, int n) = 0;

    std::vector<float> inputAccumulator;
    std::vector<float> outputBuffer;
    int accumulatedSamples = 0;
    int outputReadPos = 0;
    int outputAvailable = 0;
};
```

### 5.4 Profiling ML Inference in Audio Plugins

**CPU time measurement in processBlock:**

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
{
#if JUCE_DEBUG
    auto startTime = std::chrono::high_resolution_clock::now();
#endif

    // ... inference code ...

#if JUCE_DEBUG
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime).count();

    // Calculate deadline
    double deadlineUs = (buffer.getNumSamples() / getSampleRate()) * 1e6;
    double cpuLoad = (duration / deadlineUs) * 100.0;

    // Log if approaching deadline
    if (cpuLoad > 50.0)
        DBG("ML inference: " + juce::String(duration) + "us ("
            + juce::String(cpuLoad, 1) + "% of deadline)");
#endif
}
```

**RTNeural benchmark pattern:**

```cpp
// Measure inference time for a single forward pass
void benchmarkModel()
{
    constexpr int NUM_ITERATIONS = 10000;
    float input[1] = { 0.5f };

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_ITERATIONS; ++i)
        model.forward(input);
    auto end = std::chrono::high_resolution_clock::now();

    auto totalUs = std::chrono::duration_cast<std::chrono::microseconds>(
        end - start).count();
    double avgNs = (totalUs * 1000.0) / NUM_ITERATIONS;

    DBG("Average inference: " + juce::String(avgNs, 1) + " ns/sample");
    DBG("At 44.1kHz: " + juce::String(avgNs * 44100 / 1e9 * 100, 1)
        + "% CPU");
}
```

---

## 6. Open Source Reference Projects

### 6.1 GuitarML Ecosystem

| Project | ML Library | Architecture | Description |
|---------|-----------|--------------|-------------|
| [SmartGuitarAmp](https://github.com/GuitarML/SmartGuitarAmp) | Custom WaveNet C++ | WaveNet | Tube amp emulation with JUCE |
| [SmartGuitarPedal](https://github.com/GuitarML/SmartGuitarPedal) | Custom WaveNet C++ | WaveNet | Pedal emulation with JUCE |
| [SmartAmpPro](https://github.com/GuitarML/SmartAmpPro) | TensorFlow/Keras | LSTM | In-plugin training capability |
| [NeuralPi](https://github.com/GuitarML/NeuralPi) | RTNeural | LSTM | VST3 + Raspberry Pi via Elk Audio OS |
| [PedalNetRT](https://github.com/GuitarML/PedalNetRT) | PyTorch (training) | WaveNet | Training pipeline, exports to JSON |
| [GuitarLSTM](https://github.com/GuitarML/GuitarLSTM) | Keras (training) | LSTM | Training pipeline for LSTM models |

### 6.2 Neural Amp Modeler (NAM)

| Project | Description |
|---------|-------------|
| [neural-amp-modeler](https://github.com/sdatkinson/neural-amp-modeler) | Core ML training code (PyTorch) |
| [NeuralAmpModelerPlugin](https://www.neuralampmodeler.com) | Official iPlug2-based plugin |
| [nam-juce](https://github.com/Tr3m/nam-juce) | Community JUCE port of NAM |

NAM uses a WaveNet-based architecture with Eigen for matrix computation. The nam-juce project demonstrates integrating the NeuralAmpModelerCore library with JUCE, linking against Eigen and nlohmann_json.

### 6.3 AIDA-X

[AIDA-X](https://github.com/AidaDSP/AIDA-X) is an amp model player that uses RTNeural for inference. Supports AU, CLAP, LV2, VST2, and VST3. Features combined models trained at multiple gain stages with real-time parameter adjustment. Uses FFTConvolver for cabinet impulse responses alongside neural network inference.

### 6.4 Production Templates and Frameworks

| Project | Description |
|---------|-------------|
| [nn-inference-template](https://github.com/Torsion-Audio/nn-inference-template) | JUCE template with anira for LibTorch/ONNX/TFLite (ADC23) |
| [anira](https://github.com/anira-project/anira) | Architecture for real-time neural inference with thread pool |
| [Scyclone](https://github.com/Torsion-Audio/Scyclone) | Neural timbre transfer plugin (uses anira) |
| [NeuralNote](https://github.com/DamRsn/NeuralNote) | Audio-to-MIDI with RTNeural + ONNX Runtime |
| [RTNeural-example](https://github.com/jatinchowdhury18/RTNeural-example) | Minimal JUCE + RTNeural example plugin |
| [ml-vst](https://github.com/carlthome/ml-vst) | WIP JUCE + TFLite template |

### 6.5 Chowdhury DSP Plugins (RTNeural Creator)

| Plugin | Description |
|--------|-------------|
| [Chow Centaur](https://github.com/jatinchowdhury18/KlonCentaur) | Klon Centaur pedal emulation using RNN |
| [Chow Tape Model](https://github.com/jatinchowdhury18/AnalogTapeModel) | Analog tape emulation with neural network hysteresis |
| [BYOD](https://github.com/Chowdhury-DSP/BYOD) | Guitar distortion plugin with ML-based effects |

---

## 7. Decision Matrix

### 7.1 Choosing an Inference Library

```
Is your model small (< 200 parameters)?
|
+-- YES --> Can you define the architecture at compile time?
|           |
|           +-- YES --> RTNeural (compile-time API)
|           |           Best performance, fully real-time safe
|           |
|           +-- NO  --> RTNeural (runtime API)
|                       Still real-time safe, slightly slower
|
+-- NO  --> Does inference complete within ~1ms at target sample rate?
            |
            +-- YES --> RTNeural (if layer types are supported)
            |           OR ONNX Runtime on audio thread (risky)
            |
            +-- NO  --> Need background thread inference
                        |
                        +-- Want easy integration? --> anira
                        |   Handles threading, supports ONNX/LibTorch/TFLite
                        |
                        +-- Want full control? --> Custom juce::Thread
                            + AbstractFifo ring buffers
```

### 7.2 Choosing a Neural Network Architecture

```
What are you modeling?
|
+-- Guitar amp/pedal (nonlinear, dynamic)
|   |
|   +-- Highest accuracy needed --> WaveNet (longer training)
|   +-- Good accuracy, faster training --> LSTM (stateful)
|   +-- Balanced --> GRU
|   +-- Parameterizable model --> LSTM + WaveNet combined (PANAMA)
|
+-- Linear/mild nonlinear effect
|   +-- Dense network (small, fast)
|
+-- Audio classification / analysis
|   +-- CNN (Conv1D/Conv2D) + Dense
|   +-- Can run on background thread
|
+-- Source separation / generation
    +-- Large model, must use background thread
    +-- U-Net, Transformer architectures
    +-- ONNX Runtime or LibTorch via anira
```

### 7.3 Typical Performance Budget

At 44.1kHz, 128-sample buffer = 2.9ms deadline:

| Model Type | Typical Inference Time | On Audio Thread? |
|-----------|----------------------|-----------------|
| RTNeural LSTM (32 hidden) | ~50ns/sample | Yes |
| RTNeural Dense (8x8x1) | ~20ns/sample | Yes |
| RTNeural WaveNet (small) | ~200ns/sample | Yes |
| ONNX Runtime LSTM | ~1-10us/sample | Borderline |
| LibTorch Conv1D | ~10-100us/block | No |
| Large CNN (source sep) | ~10-100ms/block | No |

### 7.4 Quick-Start Recommendation

For a first ML plugin project:

1. **Train** an LSTM model using [GuitarLSTM](https://github.com/GuitarML/GuitarLSTM) with Keras
2. **Export** weights to JSON
3. **Integrate** with JUCE using [RTNeural-example](https://github.com/jatinchowdhury18/RTNeural-example) as template
4. **Use** RTNeural compile-time API with XSIMD backend
5. **Embed** model weights as binary data via `juce_add_binary_data`

This gives you a fully real-time safe plugin with minimal complexity and excellent performance.

---

## Sources

### Libraries and Frameworks
- [RTNeural](https://github.com/jatinchowdhury18/RTNeural) - Real-time neural network inferencing for audio
- [anira](https://github.com/anira-project/anira) - Architecture for neural network inference in real-time audio
- [ONNX Runtime](https://onnxruntime.ai/) - Cross-platform ML inference engine
- [JUCE Framework](https://github.com/juce-framework/JUCE) - Cross-platform C++ audio framework

### Research Papers
- [RTNeural: Fast Neural Inferencing for Real-Time Systems](https://ccrma.stanford.edu/~jatin/rtneural/) - Chowdhury, 2021
- [ANIRA: An Architecture for Neural Network Inference in Real-Time Audio Applications](https://arxiv.org/abs/2506.12665) - Ackva & Schulz, 2024
- [Real-Time Guitar Amplifier Emulation with Deep Learning](https://www.mdpi.com/2076-3417/10/3/766) - Wright et al., 2020

### Tutorials and Articles
- [Neural Networks for Real-Time Audio series](https://medium.com/nerd-for-tech/neural-networks-for-real-time-audio-introduction-ed5d575dc341) - Keith Bloemer
- [Real-Time Neural Network Inferencing for Audio Processing](https://medium.com/mlearning-ai/real-time-neural-network-inferencing-for-audio-processing-857313fd84e1) - Jatin Chowdhury
- [Torsion Audio nn-inference-template](https://github.com/Torsion-Audio/nn-inference-template) - ADC23 presentation

### Open Source Plugins
- [GuitarML SmartGuitarAmp](https://github.com/GuitarML/SmartGuitarAmp)
- [GuitarML NeuralPi](https://github.com/GuitarML/NeuralPi)
- [AIDA-X](https://github.com/AidaDSP/AIDA-X)
- [nam-juce](https://github.com/Tr3m/nam-juce)
- [NeuralNote](https://github.com/DamRsn/NeuralNote)
- [Scyclone](https://github.com/Torsion-Audio/Scyclone)
