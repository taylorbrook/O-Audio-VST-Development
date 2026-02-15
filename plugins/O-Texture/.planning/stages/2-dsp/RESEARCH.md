# Stage 2: DSP - Research

**Date:** 2026-02-14
**Plugin:** O-Texture
**Stage:** 2-dsp
**Confidence:** HIGH (source code analysis, official docs, fundamental DSP)

---

## Research Summary

Six research areas were investigated for O-Texture Stage 2 (DSP) implementation. All critical open questions from the discuss phase have been resolved.

### Key Findings

| Topic | Finding | Confidence |
|-------|---------|------------|
| ANIRA model loading | In-memory binary loading natively supported -- no temp files needed | HIGH |
| ANIRA inference API | Decoupled push_data/pop_data pattern ideal for decoder (32-float input -> 4096-float output) | HIGH |
| Stereo dual-decode | Two separate InferenceHandler instances sharing singleton Context thread pool | HIGH |
| Evolve noise | Value noise with quintic interpolation (not gradient Perlin) -- simplest, identical smoothness in 1D | HIGH |
| Overlap-add | Linear accumulator + Hann window, 6144-sample buffer, shift pattern | HIGH |
| Tilt EQ | 1-pole musicdsp.org algorithm, 800Hz pivot, SmoothedValue for zipper-free | HIGH |
| PyTorch training | Cyclical KL annealing (beta_max=0.001), AdamW, cosine LR, opset 17 ONNX export | HIGH |

### Open Questions Resolved

| Question (from CONTEXT.md) | Answer |
|----------------------------|--------|
| Can ANIRA load models from memory? | YES -- `ModelData(void* data, size_t size, InferenceBackend::ONNX)` with `is_binary=true` |
| What is ANIRA's API for inference? | `push_data()` / `pop_data()` for decoupled I/O; `process()` for in-place |
| How does ANIRA handle stereo? | Two InferenceHandler instances, each with own Config/PrePostProcessor, shared singleton Context |
| Prior model via ANIRA or direct ONNX? | Recommend ANIRA for decoder (RT-critical), consider direct ONNX for prior (not RT-critical) |

---

## 1. ANIRA v2.0.3 Inference API

**Source:** Direct source code analysis of `/build/_deps/anira-src/`

### InferenceHandler Lifecycle

1. Create `InferenceConfig` + `PrePostProcessor` (must outlive handler)
2. Create `InferenceHandler(pp_processor, config, context_config)`
3. Call `set_inference_backend(InferenceBackend::ONNX)` -- defaults to CUSTOM (passthrough)
4. Call `prepare(HostConfig)` -- allocates buffers, computes latency
5. Use `push_data()` / `pop_data()` in processBlock (RT-safe, lock-free)
6. Handler destructor releases session from Context

### Key API Methods

| Method | Thread Safety | Purpose |
|--------|---------------|---------|
| `prepare(HostConfig)` | NOT RT-safe | Initialize buffers |
| `push_data(input, num_samples)` | RT-safe | Push latent vector to inference queue |
| `pop_data(output, num_samples)` | RT-safe | Pop decoded audio (returns 0 if not ready) |
| `get_latency()` | Safe | Get latency in samples |
| `set_inference_backend()` | RT-safe (atomic) | Select ONNX backend |

### Decoder Configuration Pattern

```cpp
// Binary model data from JUCE BinaryData
std::vector<anira::ModelData> decoder_model = {
    {(void*)ModelData::decoder_onnx, (size_t)ModelData::decoder_onnxSize,
     anira::InferenceBackend::ONNX}
};

// Tensor shapes: latent[1,32] -> audio[1,4096]
std::vector<anira::TensorShape> decoder_shape = {
    {{{1, 32}}, {{1, 4096}}}
};

anira::ProcessingSpec decoder_spec({1}, {1}, {32}, {4096}, {0});

anira::InferenceConfig decoder_config(
    decoder_model, decoder_shape, decoder_spec,
    10.0f,  // max inference time (ms)
    2,      // warm-up inferences
    false,  // session_exclusive (no cached state)
    0.0f,   // non-blocking
    2       // parallel processors (for L+R)
);
```

### HostConfig Scaling for Decoder

Follow RAVE decoder pattern -- scale buffer/sample-rate by compression factor:

```cpp
anira::HostConfig decoder_host{
    static_cast<float>(samplesPerBlock) / 128.f,  // 4096/32 = 128
    static_cast<float>(sampleRate) / 128.f
};
```

### Critical Pitfalls

1. **Singleton Context** -- first handler's ContextConfig wins, subsequent ignored
2. **Backend defaults to CUSTOM** -- must call `set_inference_backend(ONNX)` explicitly
3. **Warm-up >= 2 needed** -- ONNX first inference is slow (JIT)
4. **PrePostProcessor must outlive InferenceHandler** -- stores reference
5. **pop_data returns 0** when no data available -- must handle gracefully

**Detailed research:** `anira-api-research.md`

---

## 2. ONNX In-Memory Model Loading

**Finding:** ANIRA v2.0.3 natively supports loading ONNX models from in-memory binary buffers. No temp file extraction needed.

**Evidence chain:**
- `OnnxRuntimeProcessor.cpp:45` -- checks `is_model_binary()`, uses `Ort::Session(env, data, size, options)`
- `InferenceConfig.h:38` -- `ModelData(void* data, size_t size, backend)` constructor
- ANIRA JUCE example (`MODEL_TO_USE==1`) -- demonstrates exact BinaryData pattern

**O-Texture already has correct setup:** `CMakeLists.txt` embeds 3 ONNX models via `juce_add_binary_data` in `ModelData` namespace. Real trained models will use the same variable names.

**Pattern:**
```cpp
{(void*) ModelData::decoder_onnx, (size_t) ModelData::decoder_onnxSize, anira::InferenceBackend::ONNX}
```

**Detailed research:** `onnx-memory-loading-research.md`

---

## 3. Evolve Noise Algorithm

**Recommendation:** Value noise with quintic interpolation (NOT gradient Perlin, NOT Simplex).

**Why value noise wins for 1D block-rate modulation:**
- Simplest algorithm (no gradient computation)
- Quintic fade provides C2 continuity -- identical smoothness to improved Perlin in 1D
- Exact output range [0,1] (trivially mapped to [-1,1])
- Zero allocations, zero branching in hot path
- 28 channels x ~23 Hz evaluation rate -- simplicity matters

**Speed mapping:** Quadratic curve for perceptual sensitivity at low EVOLVE values
- EVOLVE=0.0 -> frozen, 0.3 -> ~7s period (default), 1.0 -> ~2s period

**Implementation:** Complete `PerlinNoise1D<NumChannels>` template class (~150 lines):
- Seeded 256-entry permutation table with Fisher-Yates shuffle
- Per-channel decorrelated offsets (prime spacing: `ch * 37 + 7`)
- `advance(freeze)` + `getValue(ch)` API
- State serializable for preset recall (cursors + seed)

**Detailed research:** `perlin-noise-research.md`

---

## 4. Overlap-Add Crossfading

**Architecture:** Linear accumulator buffer (6144 samples per channel) with Hann window.

**Design:**
- `HannWindow<4096>` -- periodic form (divide by N, not N-1), pre-computed once
- `OverlapAddProcessor` -- prepare(), addDecodedBlock(), readSamples(), shiftAccumulator()
- Each decoded block: Hann-windowed via `FloatVectorOperations::multiply`, then added into accumulator
- After HOP_SIZE (2048) samples consumed: shift left, clear tail, add next block
- Variable processBlock sizes handled via read position counter crossing hop boundaries

**COLA proof:** Hann at 50% overlap: `sin^2(x) + cos^2(x) = 1` (Pythagorean identity). No gain correction needed.

**Underrun fallback:** Repeat last decoded block (brief ~85ms freeze, inaudible for textures, better than silence).

**Thread safety:** OverlapAddProcessor accessed only from audio thread. Decoded blocks handed off from ANIRA via atomic flags or double-buffer slot.

**Detailed research:** `overlap-add-research.md`

---

## 5. Tilt EQ (BRIGHTNESS Parameter)

**Recommendation:** 1-pole tilt filter from musicdsp.org (Elysia mPressor "Niveau" pattern).

**Algorithm:**
```
lowpass_output = a0 * input + b1 * state
highpass_output = input - lowpass_output
output = input + lgain * lowpass_output + hgain * highpass_output
```

**Configuration:**
- Pivot frequency: 800 Hz (standard choice, Quad 34 preamplifier)
- Max gain: +/-6 dB, gfactor=4 (asymmetric boost/cut)
- Slope: ~6 dB/oct (gentle, appropriate for texture shaping)
- Smoothing: `juce::SmoothedValue<float, Linear>` with 50ms ramp

**Why not dual-shelf:** 1-pole is ~5x cheaper, gentler slope suits textures, less phase distortion.

**Integration:** Applied AFTER overlap-add output (never before).

**Detailed research:** `tilt-eq-research.md`

---

## 6. PyTorch Training Pipeline (Rain)

### Dataset
- Primary: Freesound.org (BonnyOrbit, be-steele, RJStefanski packs)
- Target: 30-45 min diverse rain audio, 48kHz mono
- Preprocessing: resample, mono, peak normalize, remove silence (RMS-based)
- Download 60+ min, curate to 30-45 min after listening

### VAE Training
- **Optimizer:** AdamW (lr=1e-4, weight_decay=1e-5)
- **LR schedule:** 1000-step warmup + cosine decay to 1e-6
- **KL annealing:** Cyclical (4 cycles, ratio=0.5, beta_max=0.001) -- NOT linear warmup
- **Batch size:** 32 (safe for 10GB+ VRAM)
- **Loss:** multi-scale spectral + 0.1*L1 + beta*KL
- **Duration:** 150k-200k steps, ~3-10 hours on RTX 3080/4090

### Prior Training (GRU)
- Extract latent sequences from trained VAE encoder (teacher forcing)
- Sequence length: 32 blocks (~2.7s temporal context)
- MSE loss on predicted mu vs actual next latent
- ~100-200 epochs, ~1-2 hours
- Export as single-step model with explicit hidden state I/O

### ONNX Export
- Opset 17 (within ONNX Runtime 1.19.2's opset 21 ceiling)
- `model.eval()` before export (folds BatchNorm running stats)
- Prior wrapper: `TexturePriorONNX` with explicit `(z_input, hidden_in) -> (mu, logvar, hidden_out)`
- Validate: PyTorch vs ONNX Runtime outputs must match within 1e-5

### Quality Validation
- Spectral convergence < 0.3 (good), < 0.15 (excellent)
- Log spectral distance < 1.0 dB
- t-SNE latent visualization (should be smooth blob, not clusters)
- Interpolation test (smooth transition between endpoints)

### Failure Modes
- **Posterior collapse:** KL < 0.01, all outputs identical -> use cyclical annealing
- **Checkerboard:** kernel%stride != 0 -> architecture uses 8/4 (safe)
- **Blurry output:** missing highs -> increase spectral loss at small FFT sizes

**Detailed research:** `training-pipeline-research.md`

---

## Architectural Recommendations

### Decoder: Use ANIRA push/pop (RT-critical)
Two InferenceHandler instances (L/R) with shared singleton thread pool. Decoupled push_data/pop_data pattern handles different I/O sizes (32 -> 4096).

### Prior: Consider direct ONNX Runtime or ANIRA
The prior model is autoregressive (single-step with hidden state), which doesn't fit ANIRA's streaming pattern cleanly. Options:
1. ANIRA with non-streamable tensor input (set_input atomics)
2. Direct ONNX Runtime on background timer thread (prior is not RT-critical)

Recommend starting with ANIRA for both, fall back to direct ONNX if complications arise.

### Processing Chain
```
Prior model → latent vector → [user X/Y/Char overlay] → [Evolve noise modulation]
    → Decoder L (ANIRA) → OLA L → Tilt EQ → Output L
    → Decoder R (ANIRA, offset latent) → OLA R → Tilt EQ → Output R
```

### File Organization
```
Source/
  DSP/
    PerlinNoise1D.h       # Value noise for Evolve modulation
    OverlapAddProcessor.h  # OLA buffer management
    OverlapAddProcessor.cpp
    HannWindow.h           # Pre-computed Hann window
    TiltFilter.h           # BRIGHTNESS tilt EQ
  PluginProcessor.h/cpp    # Main processor (existing, to be extended)
  PluginEditor.h/cpp       # WebView editor (existing, unchanged in Stage 2)
```

---

## Risk Assessment (Updated)

| Risk | Level | Mitigation |
|------|-------|------------|
| ANIRA push/pop with non-standard tensor shapes | MEDIUM | RAVE decoder example validates pattern; test early |
| HostConfig scaling for 32->4096 compression | MEDIUM | Follow RAVE pattern, experiment with values |
| VAE training quality (Rain) | MEDIUM | Cyclical annealing, quality metrics, listening tests |
| Prior model ONNX stateful inference | MEDIUM | Single-step export with explicit hidden state |
| ANIRA shared library stability in DAW | LOW | Already verified in Stage 1 |
| Overlap-add correctness | LOW | Mathematically proven (COLA), straightforward implementation |
| Tilt EQ | LOW | Simple 1-pole filter, well-understood |

---

## Individual Research Documents

| Document | Lines | Topic |
|----------|-------|-------|
| `anira-api-research.md` | 672 | ANIRA InferenceHandler API, push/pop, stereo, tensor config |
| `onnx-memory-loading-research.md` | 233 | In-memory binary model loading confirmation |
| `perlin-noise-research.md` | 796 | Value noise algorithm, complete C++ implementation |
| `overlap-add-research.md` | 835 | HannWindow + OverlapAddProcessor class design |
| `tilt-eq-research.md` | 591 | TiltFilter 1-pole implementation |
| `training-pipeline-research.md` | 1172 | PyTorch training, dataset, ONNX export, validation |
