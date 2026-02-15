# Stage 2: DSP - Execution Plan

**Plugin:** O-Texture
**Stage:** 2-dsp
**Created:** 2026-02-14
**Tasks:** 12 (split into Phase A: Training Pipeline + Phase B: JUCE DSP)

---

## Goal

Implement the complete DSP engine for O-Texture: a PyTorch training pipeline that produces a trained Rain VAE + Prior model, ONNX export, and a JUCE ANIRA inference pipeline with latent space control, overlap-add crossfading, stereo decorrelation, evolve noise modulation, and post-processing tilt EQ. At stage completion, playing the plugin produces real neural-generated rain texture audio in stereo.

---

## Phase A: PyTorch Training Pipeline (Rain)

### Task 1: Create training infrastructure
- **Files to create:**
  - `training/requirements.txt` (PyTorch, torchaudio, onnx, onnxruntime, etc.)
  - `training/config.py` (all hyperparameters, paths, constants)
  - `training/models.py` (TextureEncoder, TextureDecoder, TextureVAE, TexturePrior, TexturePriorONNX)
  - `training/losses.py` (MultiScaleSpectralLoss, TextureVAELoss)
  - `training/dataset.py` (TextureDataset with random 4096-sample cropping)
- **Architecture spec to implement:**
  - Encoder: Conv1D stack (1→16→32→64→128→256→256), k=7 s=2 × 5 then k=3 s=2, BatchNorm1d+LeakyReLU, AdaptiveAvgPool1d→16, Dense→mu(32)+logvar(32)
  - Decoder: Dense 32→256×16, ConvTranspose1D stack (256→128→64→32→16) k=8 s=4 × 4 with BatchNorm1d+LeakyReLU, final Conv1D 16→1 k=7 s=1 Tanh
  - Prior: 2-layer GRU (hidden=128), fc_mu(32)+fc_logvar(32)
  - Loss: multi-scale spectral (FFT 2048/1024/512/256) + 0.1×L1 + beta×KL
- **Depends on:** None

### Task 2: Create dataset download and preprocessing scripts
- **Files to create:**
  - `training/download_rain.py` (Freesound API search + download for rain recordings)
  - `training/preprocess.py` (resample to 48kHz mono, peak normalize, silence removal, quality report)
- **Target:** 30-45 min of diverse rain audio at 48kHz mono in `training/data/processed/`
- **Rain diversity:** light drizzle, steady rain, heavy rain, rain on roof/metal, rain on foliage, rain with distant thunder
- **Depends on:** Task 1 (needs config.py for paths/constants)

### Task 3: Train VAE (Rain)
- **Files to create:**
  - `training/train_vae.py` (main training loop with logging, checkpointing, metrics)
- **Hyperparameters:**
  - AdamW, lr=1e-4, weight_decay=1e-5
  - 1000-step linear warmup + cosine decay to 1e-6
  - Cyclical KL annealing: 4 cycles, ratio=0.5, beta_max=0.001
  - Batch size 32, gradient clipping max_norm=1.0
  - Target: 150k-200k steps (~3-10 hours GPU)
  - Mixed precision via torch.amp.autocast
- **Checkpointing:** Save every 10k steps + best model by spectral convergence
- **Depends on:** Tasks 1, 2

### Task 4: Evaluate VAE + Analyze latent space
- **Files to create:**
  - `training/evaluate.py` (reconstruction quality, spectral convergence, LSD, listening samples)
  - `training/analyze_latent.py` (t-SNE visualization, per-dimension variance analysis, dim_map generation)
- **Quality gates:**
  - Spectral convergence < 0.3 (good), target < 0.15
  - Log spectral distance < 1.0 dB
  - t-SNE: smooth continuous blob (not clusters or single point)
  - Listening test: reconstructions sound like rain, not muffled/artifacted
- **Output:** `dim_map_rain.json` mapping active dims to X/Y/CharA/CharB/Evolve
- **Depends on:** Task 3

### Task 5: Train Prior (GRU) + Export ONNX
- **Files to create:**
  - `training/train_prior.py` (extract latent sequences, train GRU with teacher forcing)
  - `training/export_onnx.py` (export encoder/decoder/prior to ONNX opset 17, validate vs PyTorch)
- **Prior training:**
  - Extract latent mu sequences from trained VAE encoder
  - Sliding window: seq_len=32, teacher forcing, MSE loss on predicted mu
  - Adam lr=1e-3, cosine annealing, 100-200 epochs (~1-2 hours)
  - TexturePriorONNX wrapper: explicit (z_input[1,1,32], hidden_in[2,1,128]) → (mu, logvar, hidden_out)
- **ONNX validation:** PyTorch vs ONNX Runtime output diff < 1e-5
- **Output files committed to repo:**
  - `Resources/models/rain/rain_decoder.onnx`
  - `Resources/models/rain/rain_encoder.onnx`
  - `Resources/models/rain/rain_prior.onnx`
  - `Resources/models/rain/dim_map_rain.json`
- **Depends on:** Task 4

---

## Phase B: JUCE DSP Implementation

### Task 6: Create DSP utility classes (HannWindow, OverlapAddProcessor, PerlinNoise1D, TiltFilter)
- **Files to create:**
  - `Source/DSP/HannWindow.h` (header-only, periodic Hann window template)
  - `Source/DSP/OverlapAddProcessor.h` + `.cpp` (6144-sample linear accumulator, Hann-windowed addDecodedBlock, readSamples with hop tracking)
  - `Source/DSP/PerlinNoise1D.h` (header-only, 28-channel value noise with quintic fade, seeded permutation table)
  - `Source/DSP/TiltFilter.h` (header-only, 1-pole musicdsp.org tilt filter with SmoothedValue)
- **Key design decisions (from research):**
  - OLA: Linear accumulator + shift (not ring buffer), BLOCK_SIZE=4096, HOP_SIZE=2048, ACCUM_SIZE=6144
  - Noise: Value noise with quintic fade (not gradient Perlin), quadratic speed mapping, 28 evolve channels
  - Tilt: 1-pole split (not dual-shelf), 800 Hz pivot, gfactor=4, ±6dB max, 50ms SmoothedValue ramp
- **Depends on:** None (can be done in parallel with Phase A)

### Task 7: Update CMakeLists.txt for real Rain models
- **Files to modify:**
  - `CMakeLists.txt` -- update BinaryData SOURCES to include real Rain ONNX models + dim_map JSON, add DSP source files
- **Changes:**
  - Replace placeholder model paths with `Resources/models/rain/rain_decoder.onnx`, etc.
  - Add `Source/DSP/OverlapAddProcessor.cpp` to target_sources
  - Keep placeholders for other textures (encoder/prior not used yet for non-Rain)
- **Depends on:** Task 5 (needs real ONNX models), Task 6 (needs DSP sources)

### Task 8: Implement ANIRA decoder inference pipeline (stereo)
- **Files to modify:**
  - `Source/PluginProcessor.h` -- add ANIRA includes, InferenceConfig/Handler members for L+R decoders, decoded block buffers
  - `Source/PluginProcessor.cpp` -- configure ANIRA: ModelData from BinaryData, TensorShape {1,32}→{1,4096}, ProcessingSpec, HostConfig scaling, prepare(), push_data/pop_data in processBlock
- **Key implementation details:**
  - Two InferenceHandler instances (decoder_L, decoder_R) with independent Config+PrePostProcessor
  - Shared singleton Context (first handler's ContextConfig wins -- 2 threads)
  - set_inference_backend(ONNX) explicitly after construction
  - warm_up=2 to avoid first-inference latency spike
  - HostConfig scaling: buffer/sr divided by compression factor (4096/32 = 128)
  - pop_data returns 0 if not ready -- handle gracefully (repeat last block)
  - Store last decoded block per channel for underrun fallback
- **Depends on:** Task 7

### Task 9: Implement prior model inference (GRU autoregressive generation)
- **Files to modify:**
  - `Source/PluginProcessor.h` -- add prior inference members (ANIRA or direct ONNX Runtime), hidden state buffer, latent history
  - `Source/PluginProcessor.cpp` -- prior inference loop: single-step GRU with explicit hidden state I/O, reparameterize z_next from mu+logvar with temperature
- **Design choice (from research):**
  - Start with ANIRA InferenceHandler for prior (consistent API)
  - If ANIRA's streaming model doesn't fit GRU's stateful pattern, fall back to direct Ort::Session on background timer thread
  - Hidden state: std::vector<float>(2*1*128) initialized to zeros, carried between calls
  - Temperature: fixed 0.8 default (maps EVOLVE to range later)
- **Depends on:** Task 8

### Task 10: Implement latent space control + evolve + stereo decorrelation
- **Files to modify:**
  - `Source/PluginProcessor.h` -- add PerlinNoise1D<28> evolveNoise, latent vector arrays, dim_map struct, stereo offset
  - `Source/PluginProcessor.cpp` -- construct_latent_vector(): read dim_map JSON, map X/Y/CharA/CharB to active dims, apply evolve noise to remaining dims, sample N(0,1) for inactive dims, create L/R offset copies (±0.1), submit both to ANIRA decoders
- **Key logic:**
  - dim_map loaded from BinaryData (dim_map_rain.json) at construction or SOURCE change
  - User params [0,1] → latent range [-3,3] via `val * 6.0f - 3.0f`
  - Evolve noise: advance once per hop, values × evolveRate × 2.0 for excursion, clamp [-3,3]
  - Stereo: z_left[i] += offset, z_right[i] -= offset, offset = fixed ±0.1 per dim
  - Freeze: skip evolve advance, skip prior update, allow manual X/Y/Char changes
- **Depends on:** Tasks 6, 8, 9

### Task 11: Wire up processBlock with full signal chain
- **Files to modify:**
  - `Source/PluginProcessor.h` -- add OverlapAddProcessor, TiltFilter members
  - `Source/PluginProcessor.cpp` -- complete processBlock implementation:
    1. Read all parameter values (atomic load)
    2. Set evolve speed + tilt brightness
    3. For each sample in buffer, advance hop counter
    4. At hop boundary: advance evolve noise, run prior inference, construct latent L/R, push to ANIRA decoders
    5. Pop decoded blocks from ANIRA when ready, addDecodedBlock to OLA (or repeat last block on underrun)
    6. readSamples from OLA into output buffer
    7. Apply tilt filter processBlock
  - `Source/PluginProcessor.cpp` -- update prepareToPlay: prepare OLA (2 channels), prepare tilt filter (sampleRate, 2ch), prepare ANIRA handlers, set evolve noise seed, setLatencySamples(6144)
  - `Source/PluginProcessor.cpp` -- update releaseResources: reset OLA, reset tilt filter
  - `Source/PluginProcessor.cpp` -- update getStateInformation/setStateInformation: serialize evolve noise cursors + seed
- **Depends on:** Tasks 6, 8, 9, 10

### Task 12: Build, test, and validate
- **Actions:**
  - Build with `ninja OuariconTexture_VST3 OuariconTexture_AU`
  - Clear AU cache, install to system folders
  - Verify plugin loads in DAW (Logic Pro)
  - Verify: audio output is real neural-generated rain texture (not silence or placeholder)
  - Verify: X/Y pad parameters affect timbral character
  - Verify: Evolve produces smooth temporal evolution
  - Verify: Freeze halts evolution, manual controls still work
  - Verify: Brightness tilts spectral balance
  - Verify: Stereo field has width (not mono)
  - Verify: No clicks, pops, or glitches during playback
  - Verify: No CPU spikes or dropouts
  - Run pluginval basic validation
- **Success criteria:**
  - Plugin produces continuous, evolving rain texture audio
  - All 6 active parameters (X, Y, Char A, Char B, Evolve, Freeze) affect output
  - Brightness shapes tone, stereo has width
  - Stable audio output for 60+ seconds without glitches
  - CPU < 20% single core
- **Depends on:** Task 11

---

## Dependency Graph

```
Phase A (Training):
  Task 1 (infra) → Task 2 (dataset) → Task 3 (train VAE) → Task 4 (evaluate) → Task 5 (prior + ONNX)

Phase B (JUCE DSP):
  Task 6 (DSP utils)  ─────────────────┐
                                        ├→ Task 7 (CMake) → Task 8 (ANIRA decoder)
  Task 5 (ONNX models) ────────────────┘                         │
                                                                   ├→ Task 9 (prior inference)
                                                                   │         │
                                                                   ├─────────┤
                                                                   │         │
                                                                   └→ Task 10 (latent control)
                                                                              │
                                                                              ├→ Task 11 (wire processBlock)
                                                                              │
                                                                              └→ Task 12 (build + validate)
```

**Task 6 can run in parallel with Phase A** -- DSP utility classes don't depend on trained models.

---

## File Summary

### New Files (13)
| File | Task | Description |
|------|------|-------------|
| `training/requirements.txt` | 1 | Python dependencies |
| `training/config.py` | 1 | Hyperparameters and constants |
| `training/models.py` | 1 | PyTorch model definitions |
| `training/losses.py` | 1 | Multi-scale spectral + VAE loss |
| `training/dataset.py` | 1 | Dataset loader with random cropping |
| `training/download_rain.py` | 2 | Freesound API download script |
| `training/preprocess.py` | 2 | Audio preprocessing pipeline |
| `training/train_vae.py` | 3 | VAE training loop |
| `training/evaluate.py` | 4 | Quality metrics + visualization |
| `training/analyze_latent.py` | 4 | Latent space analysis + dim_map |
| `training/train_prior.py` | 5 | GRU prior training |
| `training/export_onnx.py` | 5 | ONNX export + validation |
| `Source/DSP/OverlapAddProcessor.cpp` | 6 | OLA implementation |

### New Header Files (4)
| File | Task | Description |
|------|------|-------------|
| `Source/DSP/HannWindow.h` | 6 | Pre-computed periodic Hann window |
| `Source/DSP/OverlapAddProcessor.h` | 6 | OLA accumulator class |
| `Source/DSP/PerlinNoise1D.h` | 6 | Value noise evolve modulation |
| `Source/DSP/TiltFilter.h` | 6 | Brightness tilt EQ |

### Modified Files (3)
| File | Tasks | Changes |
|------|-------|---------|
| `CMakeLists.txt` | 7 | Real model paths, DSP source files |
| `Source/PluginProcessor.h` | 8-11 | ANIRA members, DSP members, latent state |
| `Source/PluginProcessor.cpp` | 8-11 | Full processBlock, prepareToPlay, state serialization |

### New Model Files (committed after training)
| File | Task | Description |
|------|------|-------------|
| `Resources/models/rain/rain_decoder.onnx` | 5 | Trained decoder (~2.4MB) |
| `Resources/models/rain/rain_encoder.onnx` | 5 | Trained encoder (~2.4MB) |
| `Resources/models/rain/rain_prior.onnx` | 5 | Trained prior (~0.4MB) |
| `Resources/models/rain/dim_map_rain.json` | 4 | Latent dimension mapping |

---

## Risk Mitigation

| Risk | Mitigation | Fallback |
|------|------------|----------|
| VAE training quality poor | Cyclical KL annealing, multi-scale spectral loss, early listening tests | Add adversarial fine-tuning; use pre-trained RAVE models |
| ANIRA push/pop doesn't work for 32→4096 tensor shape | Follow RAVE decoder pattern with HostConfig scaling | Wrap ONNX Runtime directly in background thread with SPSC queue |
| Prior model stateful inference via ANIRA problematic | Try ANIRA first (non-streamable tensor input) | Direct Ort::Session on background timer thread |
| Inference underrun (CPU spike) | Repeat last decoded block (brief freeze, inaudible for textures) | Reduce stereo to mono (halves decoder cost) |
| Dataset quality insufficient | Download 60+ min, curate to best 30-45 min | Supplement with personal recordings or ESC-50 rain class |

---

## Success Criteria (Stage Gate)

- [ ] Trained Rain VAE with spectral convergence < 0.3
- [ ] Trained Rain Prior generating coherent latent sequences
- [ ] 3 ONNX models (encoder, decoder, prior) validated against PyTorch
- [ ] Plugin produces continuous stereo rain texture audio (not silence/noise)
- [ ] X/Y/Char A/Char B parameters modify timbral character
- [ ] Evolve produces smooth temporal evolution of texture
- [ ] Freeze halts evolution, manual controls still work
- [ ] Brightness tilts spectral balance audibly
- [ ] Stereo field has perceptible width
- [ ] 60+ seconds of stable playback without clicks/glitches/CPU spikes
- [ ] pluginval basic validation passes
