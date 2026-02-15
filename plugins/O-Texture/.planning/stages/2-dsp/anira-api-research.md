# ANIRA v2.0.3 API Research for O-Texture DSP Stage

**Researched:** 2026-02-14
**Domain:** ANIRA real-time neural network inference library (v2.0.3)
**Confidence:** HIGH (source code analysis, not secondhand documentation)
**Source:** Direct source code analysis of `/build/_deps/anira-src/`

## Summary

ANIRA v2.0.3 provides a high-level, real-time-safe API for neural network inference in audio plugins. The library manages a global thread pool with high-priority background threads that process inference requests from a lock-free concurrent queue (moodycamel::ConcurrentQueue). The primary user-facing class is `anira::InferenceHandler`, which wraps an `InferenceManager`, `SessionElement`, and `Context` (singleton).

**Key discovery for O-Texture:** ANIRA fully supports loading ONNX models from in-memory binary buffers, not just file paths. The `ModelData` struct accepts `is_binary=true` with a void* pointer and size. The ONNX Runtime backend (`OnnxRuntimeProcessor.cpp` line 45) uses `Ort::Session(env, model_data->m_data, model_data->m_size, session_options)` for binary loading.

**Key discovery for stereo:** Running two simultaneous decoder inferences (L/R) requires two separate `InferenceHandler` instances with independent `InferenceConfig` and `PrePostProcessor` objects. The RAVE encoder/decoder example (MODEL_TO_USE == 7) demonstrates exactly this pattern.

**Primary recommendation:** Use the push_data/pop_data decoupled pattern for O-Texture's generative synthesis, with two InferenceHandler instances for stereo L/R decoder calls.

---

## 1. InferenceHandler API

### Construction

```cpp
// Required objects (must outlive InferenceHandler):
anira::InferenceConfig inference_config;  // Model + tensor config
anira::PrePostProcessor pp_processor(inference_config);  // Pre/post processing

// Optional: ContextConfig for thread pool control
anira::ContextConfig context_config(4);  // 4 background threads

// Construction (no default constructor -- deleted)
anira::InferenceHandler handler(pp_processor, inference_config, context_config);
```

**Confidence: HIGH** -- verified from `InferenceHandler.h` lines 27-71 and `InferenceHandler.cpp` lines 7-51.

### Key Methods

| Method | Signature | Thread Safety | Purpose |
|--------|-----------|---------------|---------|
| `prepare()` | `void prepare(HostConfig)` | NOT RT-safe | Initialize buffers, calculate latency |
| `process()` | `size_t process(float* const* data, size_t num_samples, size_t tensor_index=0)` | RT-safe | In-place processing (same input/output shape) |
| `push_data()` | `void push_data(const float* const* input_data, size_t num_input_samples, size_t tensor_index=0)` | RT-safe | Push input to ring buffer (decoupled mode) |
| `pop_data()` | `size_t pop_data(float* const* output_data, size_t num_output_samples, size_t tensor_index=0)` | RT-safe | Pop output from ring buffer (non-blocking) |
| `set_inference_backend()` | `void set_inference_backend(InferenceBackend)` | RT-safe (atomic store) | Select ONNX/LibTorch/TFLite/Custom |
| `get_latency()` | `unsigned int get_latency(size_t tensor_index=0)` | Safe | Get latency in samples |
| `reset()` | `void reset()` | NOT RT-safe (waits for inferences) | Clear all buffers and state |

**Confidence: HIGH** -- verified from headers and source.

### Object Lifecycle

1. `InferenceConfig` and `PrePostProcessor` must be created first and outlive `InferenceHandler`
2. `InferenceHandler` is non-copyable, non-movable (all copy/move deleted)
3. `InferenceHandler` constructor creates the singleton `Context` (thread pool) via `Context::get_instance()`
4. `prepare()` must be called before any processing (allocates buffers, computes latency)
5. Backend must be set via `set_inference_backend()` -- defaults to `CUSTOM` (passthrough)
6. Destructor releases the session from the context

---

## 2. Model Loading: File Path vs. In-Memory Binary

### ONNX supports BOTH file paths and memory buffers

**Confidence: HIGH** -- verified from source code.

#### File Path Loading
```cpp
// ModelData constructor for file path (is_binary = false by default):
anira::ModelData model_data("path/to/model.onnx", anira::InferenceBackend::ONNX);
```

Source: `InferenceConfig.h` line 69 -- the string constructor defaults `is_binary=false`.

#### In-Memory Binary Loading
```cpp
// ModelData constructor for binary data:
anira::ModelData model_data(
    (void*)BinaryData::model_onnx,        // pointer to binary data
    BinaryData::model_onnxSize,            // size in bytes
    anira::InferenceBackend::ONNX,
    "",      // model_function (LibTorch only)
    true     // is_binary = true
);
```

Source: `InferenceConfig.h` line 38 and `OnnxRuntimeProcessor.cpp` lines 40-45:

```cpp
if (m_inference_config.is_model_binary(anira::InferenceBackend::ONNX)) {
    const anira::ModelData* model_data = m_inference_config.get_model_data(anira::InferenceBackend::ONNX);
    m_session = std::make_unique<Ort::Session>(m_env, model_data->m_data, model_data->m_size, m_session_options);
} else {
    // File path loading...
    m_session = std::make_unique<Ort::Session>(m_env, modelpath.c_str(), m_session_options);
}
```

The JUCE example also demonstrates binary loading at `PluginProcessor.h` lines 95-105 (MODEL_TO_USE == 1):

```cpp
std::vector<anira::ModelData> model_data = {
    {(void*) BinaryData::steerablenafxlibtorchdynamic_onnx,
     BinaryData::steerablenafxlibtorchdynamic_onnxSize,
     anira::InferenceBackend::ONNX},
};
```

### Implication for O-Texture

O-Texture already embeds placeholder ONNX models as binary data in `ModelData` namespace (Stage 1). The real trained Rain models can be embedded the same way using JUCE's BinaryData. **No temp file extraction needed.**

---

## 3. Request/Response Pattern

### Internal Architecture

The data flow is:

```
Audio Thread (RT-safe)
    |
    v
InferenceHandler.push_data() --> pushes samples to session's send RingBuffer
InferenceHandler.pop_data()  --> pops samples from session's receive RingBuffer
    |
    v (InferenceManager calls Context::new_data_submitted())
Context::new_data_submitted()
    |
    v (pre-processes send buffer -> tensor, enqueues to ConcurrentQueue)
moodycamel::ConcurrentQueue<InferenceData>  (LOCK-FREE)
    |
    v (InferenceThread dequeues and processes)
InferenceThread.execute()
    --> do_inference() --> OnnxRuntimeProcessor.process()
    --> post_process() --> writes to session's receive RingBuffer
```

### Lock-Free Guarantees

**Confidence: HIGH** -- verified from source.

1. **Audio thread side:** `push_data()` and `pop_data()` are marked `ANIRA_REALTIME`. They interact with `RingBuffer` (push_sample/pop_sample) which are simple pointer-arithmetic operations with no locks.

2. **Queue:** Uses `moodycamel::ConcurrentQueue` -- a well-known lock-free MPMC queue. Inference requests are enqueued via `Context::new_data_submitted()`.

3. **Background threads:** `InferenceThread` instances run in a high-priority thread pool. They dequeue from the ConcurrentQueue and call the backend's `process()` method. The thread uses exponential backoff (including x86_64 `_mm_pause()` instructions) when idle.

4. **Completion signal:** Results flow back via `RingBuffer` (write by inference thread, read by audio thread). There's also an atomic `m_done_atomic` flag and `m_done_semaphore` for blocking pop variants.

### Two Processing Modes

#### Mode A: Synchronous `process()` (in-place)
```cpp
// Audio thread calls process() -- pushes input, triggers inference, pops output
inference_handler.process(buffer.getArrayOfWritePointers(), buffer.getNumSamples());
```
Best for: Traditional audio effects where input and output have the same shape.

#### Mode B: Decoupled `push_data()` / `pop_data()`
```cpp
// Push input data
inference_handler.push_data(input_ptrs, num_input_samples);
// ... later or in the same call ...
size_t received = inference_handler.pop_data(output_ptrs, num_output_samples);
```
Best for: Encoder/decoder chains, different input/output sizes, generative models.

### For O-Texture

The decoupled push/pop pattern is ideal because:
- Input (32-dim latent vector) has different shape than output (4096 audio samples)
- Generative mode: input comes from prior model or parameter mapping, not audio
- Two decoder handlers (L/R stereo) operate independently

---

## 4. Thread Pool Configuration

### Default Thread Count

```cpp
// Default: half of hardware_concurrency, minimum 1
unsigned int default_threads = (std::thread::hardware_concurrency() / 2 > 0)
    ? std::thread::hardware_concurrency() / 2 : 1;
```

Source: `ContextConfig.h` line 54.

### Custom Thread Count via ContextConfig

```cpp
anira::ContextConfig context_config(4);  // 4 inference threads
anira::InferenceHandler handler(pp_processor, config, context_config);
```

### Singleton Pattern -- CRITICAL

**The Context is a singleton.** All `InferenceHandler` instances share the same thread pool.

```cpp
// Context.h line 91:
static std::shared_ptr<Context> get_instance(const ContextConfig& context_config);
```

If a context already exists, the provided `ContextConfig` is IGNORED. Only the FIRST handler's context config is used. This means:

- If O-Texture creates two InferenceHandlers (L/R decoders), only the first one's ContextConfig matters
- The thread pool is shared across all handlers in the process

### Parallel Processor Instances (Within a Backend)

Separate from thread pool size, each backend can have multiple parallel processor instances:

```cpp
// InferenceConfig::Defaults::m_num_parallel_processors
// Default: hardware_concurrency() / 2 (same as thread count)
```

The ONNX backend creates `m_num_parallel_processors` independent `Instance` objects, each with its own `Ort::Session`. When `process()` is called, it finds a free instance via atomic compare-exchange:

```cpp
// OnnxRuntimeProcessor.cpp lines 23-32:
void OnnxRuntimeProcessor::process(...) {
    while (true) {
        for(auto& instance : m_instances) {
            if (!(instance->m_processing.exchange(true))) {
                instance->process(input, output, session);
                instance->m_processing.exchange(false);
                return;
            }
        }
    }
}
```

Each instance sets `SetIntraOpNumThreads(1)` for ONNX Runtime, so each inference uses exactly 1 thread.

### session_exclusive_processor Flag

When `session_exclusive_processor = true`, `m_num_parallel_processors` is forced to 1 (one ONNX session per backend instance). This is used for stateful models with cached layers (e.g., RAVE's cached convolutions).

**For O-Texture:** The decoder likely has no cached state (it's a simple 1D CNN decoder taking a latent vector), so `session_exclusive_processor = false` is appropriate. But if the model uses cached convolutions, set `true`.

**Confidence: HIGH** -- verified from `InferenceConfig.cpp` lines 27-29 and `OnnxRuntimeProcessor.cpp`.

---

## 5. Multiple Simultaneous Inferences (Stereo L/R)

### Approach: Two Separate InferenceHandler Instances

The RAVE encoder/decoder example (MODEL_TO_USE == 7) demonstrates running two completely independent inference pipelines:

```cpp
// From PluginProcessor.h lines 132-149:
anira::InferenceConfig inference_config_encoder = rave_funk_drum_encoder_config;
anira::InferenceConfig inference_config_decoder = rave_funk_drum_decoder_config;
anira::PrePostProcessor pp_processor_encoder;
anira::PrePostProcessor pp_processor_decoder;

anira::InferenceHandler inference_handler_encoder;
anira::InferenceHandler inference_handler_decoder;
```

Each handler gets its own `InferenceConfig`, `PrePostProcessor`, and `SessionElement`, but they share the singleton `Context` (thread pool).

### For O-Texture Stereo

Create two decoder InferenceHandlers:

```cpp
// Each needs its own config and pp_processor (they store references)
anira::InferenceConfig decoder_config_L = createDecoderConfig();
anira::InferenceConfig decoder_config_R = createDecoderConfig();
anira::PrePostProcessor pp_decoder_L(decoder_config_L);
anira::PrePostProcessor pp_decoder_R(decoder_config_R);

anira::InferenceHandler decoder_handler_L(pp_decoder_L, decoder_config_L, context_config);
anira::InferenceHandler decoder_handler_R(pp_decoder_R, decoder_config_R);
// Note: context_config only matters for the first handler (singleton)
```

Both handlers submit inference jobs to the same thread pool. With default config (half of CPU cores), there should be enough threads to process both L and R decodes concurrently.

**Confidence: HIGH** -- this pattern is directly demonstrated in the ANIRA examples.

---

## 6. Input/Output Format and Tensor Configuration

### InferenceConfig Construction

```cpp
anira::InferenceConfig config(
    model_data_vector,       // std::vector<ModelData>
    tensor_shape_vector,     // std::vector<TensorShape>
    processing_spec,         // ProcessingSpec
    max_inference_time_ms,   // float (ms per inference)
    warm_up_count,           // unsigned int (default: 0)
    session_exclusive,       // bool (default: false)
    blocking_ratio,          // float (default: 0.0 = non-blocking)
    num_parallel_processors  // unsigned int (default: hardware_concurrency/2)
);
```

### TensorShape

Defines the shape of input and output tensors:

```cpp
// Universal shape (works for all backends):
anira::TensorShape shape(
    {{1, 32}},       // input shapes: batch=1, features=32 (latent vector)
    {{1, 4096}}      // output shapes: batch=1, samples=4096 (audio block)
);

// The outer vector allows multiple input/output tensors
// For O-Texture decoder: single input tensor [1, 32], single output tensor [1, 4096]
```

**TensorShapeList** is `std::vector<std::vector<int64_t>>`. Each inner vector is one tensor's dimensions.

### ProcessingSpec

Defines the streaming characteristics:

```cpp
anira::ProcessingSpec spec(
    {1},      // preprocess_input_channels: 1 channel per input tensor
    {1},      // postprocess_output_channels: 1 channel per output tensor
    {32},     // preprocess_input_size: 32 samples per input (latent vector size)
    {4096},   // postprocess_output_size: 4096 samples per output (audio block)
    {0}       // internal_model_latency: 0 (decoder has no internal latency)
);
```

**Streamable vs. Non-Streamable Tensors:**
- `preprocess_input_size > 0` = streamable (flows through ring buffer)
- `preprocess_input_size == 0` = non-streamable (stored in atomic storage, set via `pp_processor.set_input()`)

### Buffer Memory Layout

ANIRA's `Buffer<float>` (aka `BufferF`) uses contiguous memory with channel pointers:

```
Memory: [Ch0_S0, Ch0_S1, ..., Ch0_SN, Ch1_S0, Ch1_S1, ..., Ch1_SN]
                     channel 0                     channel 1
```

The ONNX backend creates tensors wrapping the Buffer's data pointer:

```cpp
// OnnxRuntimeProcessor.cpp line 106:
m_inputs[i] = Ort::Value::CreateTensor<float>(
    m_memory_info,
    input[i].data(),                    // raw float* pointer
    input[i].get_num_samples() * input[i].get_num_channels(),  // total elements
    shape_data,                         // int64_t* shape
    shape_size                          // number of dimensions
);
```

### For O-Texture Decoder Configuration

```cpp
// Decoder: latent[1,32] -> audio[1,4096]
static std::vector<anira::ModelData> decoder_model_data = {
    {(void*)ModelData::decoder_onnx, ModelData::decoder_onnxSize,
     anira::InferenceBackend::ONNX, "", true}
};

static std::vector<anira::TensorShape> decoder_tensor_shape = {
    {{{1, 32}}, {{1, 4096}}}  // input: [batch=1, latent_dim=32], output: [batch=1, samples=4096]
};

static anira::ProcessingSpec decoder_processing_spec(
    {1},     // 1 input channel (latent is 1-channel, 32-sample "signal")
    {1},     // 1 output channel (mono audio)
    {32},    // input stream size: 32 floats
    {4096},  // output stream size: 4096 floats
    {0}      // no internal model latency
);

static anira::InferenceConfig decoder_config(
    decoder_model_data,
    decoder_tensor_shape,
    decoder_processing_spec,
    10.0f,   // max inference time (ms) -- need to measure actual decoder perf
    2,       // warm-up inferences
    false,   // session_exclusive_processor (decoder has no cached state)
    0.0f,    // blocking_ratio (non-blocking)
    2        // num_parallel_processors (for L+R simultaneous decode)
);
```

**Confidence: MEDIUM** -- The tensor shapes [1,32] -> [1,4096] match the O-Texture architecture spec. The exact shapes depend on the trained model's ONNX export. The processing spec's `preprocess_input_size` of 32 means ANIRA will stream 32-float blocks through its ring buffer, which maps to one latent vector per inference.

---

## 7. Non-Streamable Input Pattern (Latent Vector Injection)

For O-Texture, the latent vector isn't an audio stream -- it's a control parameter vector that changes each inference block. The "non-streamable" pattern may be more appropriate:

```cpp
// If preprocess_input_size = 0, tensor is non-streamable:
// Set values via PrePostProcessor:
for (int i = 0; i < 32; i++) {
    pp_processor.set_input(latent_vector[i], /*tensor_index=*/0, /*sample_index=*/i);
}
// Then trigger inference via push_data() with 0-length data
// The pre_process() callback reads from atomic storage
```

Source: `PrePostProcessor.h` lines 98-110 and `InferenceManager.cpp` lines 85-88:

```cpp
if (m_inference_config.get_preprocess_input_size()[tensor_index] == 0) {
    // Non-streamable: store via set_input() atomics
    for (size_t sample = 0; sample < num_samples[tensor_index]; ++sample) {
        m_pp_processor.set_input(input_data[tensor_index][0][sample], tensor_index, sample);
    }
}
```

**However**, the non-streamable path uses `std::atomic<float>` for each element (32 atomics). For 32 floats this is fine.

### Alternative: Streamable with push_data

The streamable path (preprocess_input_size = 32) pushes 32 samples into a RingBuffer. This is also fine and may be simpler since push_data/pop_data handle the flow.

**Recommendation:** Use the streamable path (preprocess_input_size = 32) with push_data/pop_data. This is the more standard ANIRA usage pattern and matches the RAVE decoder example.

---

## 8. RAVE Decoder Example (Most Relevant to O-Texture)

The RAVE decoder config (`RaveFunkDrumConfigDecoder.h`) is the closest reference:

```cpp
// RAVE decoder: latent[1,4,1] -> audio[1,1,2048]
static std::vector<anira::TensorShape> tensor_shape_decoder = {
    {{{1, 4, 1}}, {{1, 1, 2048}}}
};

static anira::ProcessingSpec processing_spec_decoder{
    {4},     // 4 input channels (4 latent dimensions as separate channels)
    {1},     // 1 output channel
    {1},     // preprocess_input_size: 1 sample per latent channel (1 time step)
    {2048},  // postprocess_output_size: 2048 audio samples
    {2048}   // internal_model_latency: 2048 samples
};
```

### RAVE uses 4 channels x 1 sample vs. O-Texture's 1 channel x 32 samples

The RAVE model has tensor shape [1, 4, 1] -- interpreted as batch=1, channels=4, time=1. It uses 4 input channels with preprocess_input_size=1.

O-Texture's decoder takes shape [1, 32] -- a flat 32-element vector. This can be represented as 1 channel x 32 samples (preprocess_input_channels=1, preprocess_input_size=32).

### processBlock Pattern from RAVE Example

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    // Push audio to encoder
    inference_handler_encoder.push_data(buffer.getArrayOfWritePointers(), buffer.getNumSamples());

    // Pop latent from encoder when enough samples accumulated
    while (m_count_input_samples >= 2048) {
        size_t received = inference_handler_encoder.pop_data(latent_ptrs, 1);
        if (received == 0) break;
        m_count_input_samples -= 2048;

        // Modulate latent space
        latent[0][0] += param_value;

        // Push latent to decoder
        inference_handler_decoder.push_data(latent_ptrs, received);
    }

    // Pop audio from decoder
    inference_handler_decoder.pop_data(buffer.getArrayOfWritePointers(), buffer.getNumSamples());
}
```

---

## 9. Latency Calculation

ANIRA automatically calculates latency based on:
- Buffer sizes (host buffer size vs. model input/output sizes)
- Number of concurrent inference structs needed
- Max inference time
- Internal model latency

```cpp
// After prepare():
unsigned int latency_samples = handler.get_latency();
setLatencySamples(latency_samples);
```

The RAVE example calculates combined encoder + decoder latency:
```cpp
int new_latency_encoder = (int) inference_handler_encoder.get_latency() * 2048;
int new_latency_decoder = (int) inference_handler_decoder.get_latency();
int new_latency = new_latency_encoder + new_latency_decoder;
```

For O-Texture (generative, no encoder), only decoder latency matters.

---

## 10. HostConfig

```cpp
anira::HostConfig host_config{
    static_cast<float>(samplesPerBlock),  // buffer size
    static_cast<float>(sampleRate),       // sample rate
    false,    // allow_smaller_buffers (default: false)
    0         // input_tensor_index (default: 0)
};
inference_handler.prepare(host_config);
```

For the decoder in RAVE, the host config uses scaled buffer/sample-rate for the latent domain:

```cpp
// Latent domain: 2048x compression
anira::HostConfig host_config_decoder{
    static_cast<float>((float)samplesPerBlock / 2048.f),
    static_cast<float>((float)sampleRate / 2048.f),
};
```

For O-Texture's decoder: The "host buffer size" should be expressed in terms of how many latent vectors arrive per audio callback. With 4096-sample blocks and a host buffer of 512 samples, approximately 512/4096 = 0.125 latent vectors per callback. This needs careful consideration.

---

## 11. Custom PrePostProcessor

The default `PrePostProcessor` does simple copy in/out. For custom preprocessing (e.g., windowing, normalization), subclass it:

```cpp
class MyPrePostProcessor : public anira::PrePostProcessor {
public:
    MyPrePostProcessor(anira::InferenceConfig& config) : PrePostProcessor(config) {}

    void pre_process(std::vector<anira::RingBuffer>& input,
                     std::vector<anira::BufferF>& output,
                     anira::InferenceBackend backend) override {
        // Custom preprocessing: copy from ring buffer to tensor
        pop_samples_from_buffer(input[0], output[0], num_samples);
        // Apply normalization, windowing, etc.
    }

    void post_process(std::vector<anira::BufferF>& input,
                      std::vector<anira::RingBuffer>& output,
                      anira::InferenceBackend backend) override {
        // Custom postprocessing: copy from tensor to ring buffer
        push_samples_to_buffer(input[0], output[0], num_samples);
    }
};
```

Helper methods available:
- `pop_samples_from_buffer(RingBuffer& in, BufferF& out, size_t num_samples)`
- `pop_samples_from_buffer(RingBuffer& in, BufferF& out, size_t new_samples, size_t old_samples)` -- overlapping windows
- `push_samples_to_buffer(BufferF& in, RingBuffer& out, size_t num_samples)`
- `set_input(float val, size_t tensor_idx, size_t sample_idx)` -- non-streamable input
- `get_output(size_t tensor_idx, size_t sample_idx)` -- non-streamable output

---

## 12. Open Questions and Concerns

### Q1: HostConfig for Generative Model (No Audio Input)

O-Texture is a synthesizer -- there's no audio input. The decoder takes a latent vector and produces audio. ANIRA's `HostConfig` is designed around the concept of a host buffer flowing through. For O-Texture:

- **Buffer size:** The DAW's buffer size (e.g., 512)
- **Sample rate:** 48000
- But the decoder input isn't audio at the host sample rate -- it's a 32-float latent vector per 4096-sample block

**Risk:** ANIRA's latency calculation and ring buffer sizing are optimized for audio-rate I/O. For the decoder where input is 32 samples and output is 4096 samples, the ratio is 128:1. The HostConfig's `get_relative_buffer_size()` uses this ratio.

**Recommendation:** Follow the RAVE decoder example pattern. Use scaled HostConfig where buffer_size and sample_rate are divided by the compression factor. For O-Texture: if input is 32 floats producing 4096 output samples, compression = 4096/32 = 128. So:

```cpp
anira::HostConfig decoder_host_config{
    static_cast<float>(samplesPerBlock) / 128.f,  // or / 4096.f
    static_cast<float>(sampleRate) / 128.f,       // or / 4096.f
};
```

This needs experimentation and testing.

**Confidence: MEDIUM** -- the RAVE decoder example uses this scaling pattern, but the specific values for O-Texture's 32->4096 mapping need validation.

### Q2: Overlap-Add with ANIRA

ANIRA manages its own ring buffers for input/output streaming. For O-Texture's overlap-add crossfading (50% overlap, Hann window), we have two options:

1. **Let ANIRA handle buffering, do overlap-add externally:** Use ANIRA for inference only, manage the overlap-add in O-Texture's processBlock
2. **Custom PrePostProcessor:** Override pre_process/post_process to apply Hann windowing

**Recommendation:** Do overlap-add externally. ANIRA's ring buffer handles the streaming; the overlap-add is an additional layer that combines two consecutive decoded blocks.

### Q3: Prior Model via ANIRA or Direct ONNX Runtime?

The prior model (GRU) takes variable-length latent sequences and produces next-latent predictions. This is autoregressive and doesn't fit ANIRA's streaming pattern well.

**Options:**
1. **Separate InferenceHandler for prior** -- non-streamable tensor input
2. **Direct ONNX Runtime call** -- ANIRA already links onnxruntime, so use the Ort API directly for the prior
3. **Background thread with raw ONNX** -- prior runs on a non-RT thread, latents are buffered

**Recommendation:** Consider using ANIRA for the decoder (performance-critical, RT-safe) and direct ONNX Runtime for the prior (not RT-critical, runs on background timer thread). This avoids forcing the prior's variable-length sequence into ANIRA's fixed-tensor model.

**Confidence: MEDIUM** -- this is an architectural decision, not an API limitation.

---

## 13. Key Pitfalls

### Pitfall 1: Singleton Context -- First Config Wins
The `Context` is a singleton. The first `InferenceHandler` created sets the thread pool size. Subsequent handlers' ContextConfig is ignored. Always create the most important handler first with the desired thread count.

### Pitfall 2: session_exclusive_processor Forces 1 Parallel Processor
When `session_exclusive_processor = true`, `m_num_parallel_processors` is forced to 1 regardless of what you pass. This is by design for stateful models.

### Pitfall 3: Backend Defaults to CUSTOM (Passthrough)
After construction, the backend is `CUSTOM` which is a passthrough (copies input to output or clears). You MUST call `set_inference_backend(anira::InferenceBackend::ONNX)` before expecting real inference.

### Pitfall 4: PrePostProcessor Must Outlive InferenceHandler
`InferenceHandler` stores a reference to `PrePostProcessor`. If the pp_processor is destroyed first, the handler will have a dangling reference.

### Pitfall 5: Missing Warm-Up Can Cause First-Call Latency Spike
ONNX Runtime's first inference is significantly slower (JIT compilation). Set `warm_up >= 2` to pre-warm the ONNX session during `prepare()`.

### Pitfall 6: pop_data Returns 0 When No Data Available
The non-blocking `pop_data()` returns 0 samples if inference hasn't completed yet. The caller must handle this gracefully (output silence/previous buffer).

### Pitfall 7: Non-Streamable Tensors Require Channel Count = 1
From `InferenceConfig.cpp` lines 366-372: if `preprocess_input_size[i] == 0` (non-streamable), `preprocess_input_channels[i]` MUST be 1.

---

## 14. Sources

All findings verified from direct source code analysis:

| File | What Was Verified |
|------|-------------------|
| `include/anira/InferenceHandler.h` | All public API methods, class constraints |
| `src/InferenceHandler.cpp` | Implementation of process/push/pop delegation |
| `include/anira/InferenceConfig.h` | ModelData (binary/path), TensorShape, ProcessingSpec, config params |
| `src/InferenceConfig.cpp` | update_processing_spec auto-computation, model path/binary handling |
| `include/anira/ContextConfig.h` | Thread pool config, singleton pattern, default thread count |
| `include/anira/scheduler/Context.h` | Singleton context, session management, lock-free queue |
| `include/anira/scheduler/InferenceThread.h` | Thread execution, ConcurrentQueue usage |
| `include/anira/scheduler/SessionElement.h` | ThreadSafeStruct, latency calculation, ring buffers |
| `src/scheduler/InferenceManager.cpp` | process_input/output flow, push/pop implementation |
| `src/backends/OnnxRuntimeProcessor.cpp` | Binary model loading, parallel instances, tensor creation |
| `include/anira/PrePostProcessor.h` | Custom preprocessing interface, set_input/get_output |
| `include/anira/utils/RingBuffer.h` | Lock-free ring buffer for audio streaming |
| `include/anira/utils/Buffer.h` | Memory layout (contiguous, channel pointers) |
| `examples/juce-audio-plugin/PluginProcessor.cpp` | JUCE integration, RAVE encoder/decoder pattern |
| `extras/models/third-party/ircam-acids/RaveFunkDrumConfigDecoder.h` | RAVE decoder tensor config reference |
