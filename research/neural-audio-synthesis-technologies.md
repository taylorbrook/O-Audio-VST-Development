# Neural Audio Synthesis Technologies for Real-Time Audio Plugins

## Deep Technical Research Report

**Date**: 2026-02-07
**Focus**: Architecture, deployment, and practical implementation of neural audio synthesis in JUCE-based real-time audio plugins.

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [RAVE (Real-time Audio Variational autoEncoder)](#1-rave-real-time-audio-variational-autoencoder)
3. [BRAVE (Low-Latency RAVE Variant)](#2-brave-low-latency-rave-variant)
4. [DDSP (Differentiable Digital Signal Processing)](#3-ddsp-differentiable-digital-signal-processing)
5. [Neutone SDK](#4-neutone-sdk)
6. [ANIRA (Neural Network Inference for Real-Time Audio)](#5-anira-neural-network-inference-for-real-time-audio)
7. [RTNeural](#6-rtneural)
8. [Other Neural Synthesis Approaches](#7-other-neural-synthesis-approaches)
9. [Reference Implementations](#8-reference-implementations)
10. [Deployment Pathways: Python Training to C++ Plugin](#9-deployment-pathways-python-training-to-c-plugin)
11. [Comparison Tables](#10-comparison-tables)
12. [Practical Recommendations](#11-practical-recommendations)

---

## Executive Summary

Neural audio synthesis has matured to the point where several architectures can operate in real-time within audio plugins. The field divides into two fundamental approaches:

1. **Pure neural synthesis** (RAVE, WaveRNN): The neural network directly generates audio waveforms. Higher quality, higher CPU cost, harder to control.
2. **Hybrid neural-DSP** (DDSP): A neural network predicts parameters for traditional DSP modules (oscillators, filters). Lower CPU cost, more interpretable, but limited to sounds decomposable into harmonic + noise components.

The most practical path for a JUCE plugin today involves:
- **Training** in Python (PyTorch)
- **Exporting** via TorchScript or ONNX
- **Inference** via LibTorch, ONNX Runtime, or RTNeural in C++
- **Thread safety** via background inference with buffer-based latency compensation (or RTNeural for audio-thread-safe inference)

Key finding: RAVE running at its default compression ratio adds 200-500ms of latency. The BRAVE variant achieves sub-10ms latency but with reduced audio quality. DDSP is the most CPU-efficient approach but limited to monophonic pitched sounds.

---

## 1. RAVE (Real-time Audio Variational autoEncoder)

**Source**: https://github.com/acids-ircam/RAVE
**Paper**: [RAVE: A variational autoencoder for fast and high-quality neural audio synthesis](https://arxiv.org/abs/2111.05011) (Caillon & Esling, 2021)
**Developed by**: IRCAM ACIDS (Analysis/Creative Interface for Digital Synthesis)
**License**: MIT

### 1.1 Architecture

```
ARCHITECTURE OVERVIEW (Text Diagram)
=====================================

Input Audio (48kHz mono)
    |
    v
+------------------+
|   PQMF Analysis  |  16-band Pseudo Quadrature Mirror Filter
|   (Multi-band)   |  Splits waveform into 16 sub-bands
+------------------+  Reduces temporal redundancy
    |
    v  [16 x (48000/16) = 16 x 3000 samples/sec per band]
+------------------+
|    ENCODER       |
|  Strided Conv1D  |  Stack of strided convolutions
|  + LeakyReLU     |  with batch normalization
|  + BatchNorm     |  Progressive downsampling
+------------------+
    |
    v
+------------------+
|   LATENT SPACE   |  z ~ N(mu, sigma)
|   R^128          |  128-dimensional latent vector
|   (VAE sampling) |  Compression ratio: 2048x default
+------------------+  (each latent timestep = 2048 audio samples)
    |
    v
+------------------+
|    DECODER       |
|  Upsampling +    |  Alternating upsampling layers
|  Residual Stacks |  and residual convolution blocks
+------------------+
    |
    +-------+-------+-------+
    |       |       |       |
    v       v       v       v
+------+ +------+ +------+
|Wave  | |Loud  | |Noise |
|form  | |ness  | |Gen   |
|(tanh)| |(sig) | |      |
+------+ +------+ +------+
    |       |       |
    v       v       v
  waveform * envelope + filtered_noise
    |
    v
+------------------+
|  PQMF Synthesis  |  Recombines 16 sub-bands
|  (Multi-band)    |  into single waveform
+------------------+
    |
    v
Output Audio (48kHz mono)
```

### 1.2 Key Architecture Details

**PQMF (Pseudo Quadrature Mirror Filter)**:
- Splits input into 16 frequency bands
- Reduces temporal resolution by 16x for efficient processing
- Reconstruction via synthesis filterbank
- Default attenuation: 100 dB between bands
- BRAVE variant uses 40 dB for lower latency (saves ~10ms)

**Encoder**:
- Stack of strided 1D convolutions with LeakyReLU and BatchNorm
- Progressively downsamples the 16-band representation
- Outputs mean (mu) and log-variance (log_sigma) for VAE sampling
- Latent dimension: 128 (standard), configurable

**Latent Space**:
- 128-dimensional continuous vector per timestep
- Compression ratio (Cr): 2048 samples per latent timestep (default)
- At 48kHz, each latent timestep represents ~42.7ms of audio
- Post-training SVD analysis separates informative vs uninformative dimensions
- Uninformative dimensions can be replaced with random noise without quality loss
- This makes the latent space more compact and controllable

**Decoder**:
- Three sub-networks:
  1. **Waveform generator**: produces multi-band signal (tanh activation)
  2. **Loudness envelope**: produces amplitude envelope (sigmoid activation)
  3. **Noise synthesizer**: produces filtered noise added to signal
- Output = (waveform * loudness_envelope) + filtered_noise
- Upsampling via transposed convolutions + residual blocks

### 1.3 RAVE v1 vs v2

| Feature | RAVE v1 | RAVE v2 |
|---------|---------|---------|
| Receptive field | Large | Smaller (adapted) |
| Adversarial training | Standard | Adapted strategy |
| Noise generator | Basic | Improved noise generator |
| Timbre transfer | General | Optimized for stationary signals |
| Regularization | Standard VAE | Additional latent space regularization |
| Data augmentation | None | Available (v2.3+) for low-data regimes |
| Configurations | v1 only | v1, v2, v2_small, v3, discrete |
| GPU requirements | 8GB+ | 5GB (RPi) to 32GB (v3) |
| Prior model | Included | Re-integrated in v2.3 |

**v2 introduces**:
- Smaller receptive field with adapted adversarial training
- Better noise generator for non-harmonic content
- Additional regularization options affecting latent space structure
- Data augmentations for training with less data
- Multiple configuration presets for different hardware targets

### 1.4 Two-Stage Training Process

**Stage 1: Representation Learning**
- Standard VAE training (reconstruction loss + KL divergence)
- Optimizer: Adam
- Batch size: 8
- Duration: ~1.5 million steps (3-4 days on RTX 3080)
- Data augmentation: dequantization, random cropping, all-pass filtering
- Goal: learn a meaningful compressed representation

**Stage 2: Adversarial Fine-Tuning**
- Encoder is FROZEN; only decoder is trained
- Multi-scale discriminator provides adversarial objective
- Duration: 4 days to 3 weeks depending on dataset complexity
- Goal: improve perceptual audio quality
- Result: dramatic quality improvement over stage 1

**Total training time**: ~1 week minimum on a modern GPU (RTX 3080/4090)

### 1.5 Training Requirements

| Requirement | Specification |
|-------------|---------------|
| GPU Memory | 5GB (RPi config) to 32GB (v3 config), 8GB typical |
| Dataset size | Minimum 1 hour, recommended 3+ hours |
| Dataset type | Homogeneous recordings (one instrument/sound class) |
| Sample rate | Internally processes at 48kHz |
| Training time (Phase 1) | 3-4 days (RTX 3080), ~2M steps/day on 4090 |
| Training time (Phase 2) | 4 days to 3 weeks |
| Cloud cost estimate | ~$50-200 on cloud GPUs |

### 1.6 Real-Time Performance

| Metric | Value |
|--------|-------|
| Generation speed | 20-80x faster than real-time on CPU |
| Sample rate | 48kHz |
| Default buffer/block size | 2048 samples |
| Parameters | ~9M (standard), ~17M (larger configs) |
| **Latency** | **200-500ms on MacBook Pro Silicon** |
| Causal mode latency | Lower, but still significant (~130ms+) |
| Model file size | ~35-70MB (TorchScript .ts file, estimated) |

**Critical latency note**: The 200-500ms latency makes standard RAVE unsuitable for live instrumental performance. It IS suitable for:
- Sound design and texture generation
- Non-real-time timbre transfer
- Studio processing where latency compensation is acceptable
- Installation art and experimental performance

**Why GPU acceleration doesn't help**: RAVE uses many small convolutions that are memory-bound, not compute-bound. GPU acceleration often adds overhead from data transfer without net benefit.

### 1.7 Latent Space Manipulation

The latent space offers rich creative possibilities:

1. **Timbre transfer**: Encode audio A, decode with decoder trained on sound B
2. **Interpolation**: Blend between two latent representations for morphing
3. **Style transfer (AdaIN)**: Adaptive Instance Normalization transfers style between source and target, directly in Max/MSP or PureData
4. **Dimensionality reduction**: SVD analysis reveals which latent dimensions carry information vs noise
5. **External control**: Map MIDI controllers, sensors, or other signals to latent dimensions
6. **Prior models**: Autoregressive models can generate plausible latent sequences for unconditional generation
7. **Latent diffusion**: RAVE-Latent-Diffusion project generates new latent codes using denoising diffusion

**Spectrum of use**: RAVE operates on a continuum between:
- **Audio effect** (all latents from audio input -> decoder): timbre transfer
- **Synthesizer** (all latents from user control): generative instrument

### 1.8 Model Export and Deployment

```
DEPLOYMENT PIPELINE
====================

Python Training           Export              C++ Inference
+-------------+     +-------------+     +------------------+
| PyTorch     | --> | TorchScript | --> | LibTorch (C++)   |
| RAVE model  |     | .ts file    |     | in JUCE plugin   |
| (training)  |     | --streaming |     |                  |
+-------------+     +-------------+     +------------------+

Export command:
  rave export --run /path/to/run --streaming [--prior /path/to/prior]

The --streaming flag enables cached convolutions for real-time buffer processing.
Without it, clicking artifacts occur in real-time use.

Three callable methods in exported model:
  - encode(audio) -> latent
  - decode(latent) -> audio
  - forward(audio) -> audio  (encode then decode)
```

### 1.9 nn~ External (Max/MSP and PureData)

The `nn~` external (https://github.com/acids-ircam/nn_tilde) provides:
- Translation layer between Max/MSP/PureData and LibTorch C++ API
- Loads TorchScript (.ts) files directly
- Exposes encode, decode, and forward methods
- Enables latent space manipulation using Max/MSP signal processing
- Supports Adaptive Instance Normalization for style transfer
- Works on embedded platforms (Raspberry Pi)
- Primary deployment target for RAVE in creative applications

### 1.10 cached_conv Library

**Source**: https://github.com/acids-ircam/cached_conv
**Paper**: [Streamable Neural Audio Synthesis With Non-Causal Convolutions](https://arxiv.org/abs/2204.07064)

Key innovation: Allows non-causal (zero-padded) models to run in streaming/real-time mode by:
1. Retaining the end of one computation buffer
2. Using it as left-padding for the next buffer
3. Maintaining continuity between consecutive audio blocks

This means models trained with non-causal convolutions (which have access to "future" samples and thus better quality) can be post-hoc converted to streamable real-time models with zero quality loss and a fixed delay.

```python
import cached_conv as cc
cc.use_cached_conv(True)  # Enable streaming mode
# All convolutions in the model now use cached padding
```

### 1.11 AFTER (Audio Features Transfer and Exploration in Real-time)

**Source**: https://github.com/acids-ircam/AFTER
**Developed by**: IRCAM ACIDS (same team as RAVE)

AFTER is a newer diffusion-based model from the RAVE team that provides:
- More "tweakable" control over timbre (like a traditional synthesizer)
- Real-time use in MaxMSP or PureData
- Max4Live devices for Ableton Live integration
- Three-step training: autoencoder, model training, model export
- Designed as an evolution of RAVE's creative paradigm

---

## 2. BRAVE (Low-Latency RAVE Variant)

**Source**: https://github.com/fcaspe/BRAVE
**Paper**: [Designing Neural Synthesizers for Low-Latency Interaction](https://arxiv.org/abs/2503.11562) (Caspe et al., 2025)
**License**: MIT

### 2.1 Motivation

Standard RAVE's 200-500ms latency makes it unsuitable for responsive instrumental interaction. Musical interaction research establishes <10ms latency with <3ms jitter as the target for acceptable performance feel.

### 2.2 Architecture Modifications from RAVE

```
BRAVE vs RAVE: KEY DIFFERENCES
================================

                        RAVE (Original)     BRAVE
                        ===============     =====
Compression ratio:      2048                128
PQMF attenuation:       100 dB              40 dB
Hidden layers:          [64,128,256,512]    [32,64,128,256]
Parameters:             ~17.3M              ~4.9M
Noise generator:        Yes                 Removed
Training mode:          Non-causal          Causal
Receptive field:        Variable            517ms
Min block size:         2048 samples        128 samples
```

**Specific changes**:
1. **Compression ratio 2048 -> 128**: Each latent timestep covers 128 audio samples instead of 2048. Drastically reduces buffering latency.
2. **PQMF attenuation 100dB -> 40dB**: Relaxed inter-band filtering reduces group delay by ~10ms.
3. **Halved model capacity**: Hidden layers [32,64,128,256] instead of [64,128,256,512]. Reduces parameters from 17.3M to 4.9M.
4. **Removed noise generator**: Simplifies optimization, slight quality trade-off.
5. **Causal training**: Eliminates cumulative delay from non-causal convolutions (which in RAVE add ~566ms).

### 2.3 Latency Measurements

| Model | Latency | Jitter |
|-------|---------|--------|
| RAVE v1 (original) | 130-440ms | High |
| RAVE with Cr=128 | 18-20ms | 2.9ms |
| BRAVE (Cr=128, p40) | **9.75-10.08ms** | **2.47-7.80ms** |

**Latency breakdown for BRAVE**:
- Buffering delay: 5.8ms (two blocks of 128 samples at 44.1kHz)
- PQMF group delay: ~2ms (at 40dB attenuation)
- Model processing: ~2ms
- Total: ~9.75ms

### 2.4 Compression Ratio Experiment Results

| Cr Value | Latency (Filosax) | Latency (Drumset) |
|----------|-------------------|-------------------|
| 2048 | 144.44ms | 130.62ms |
| 1024 | ~75ms | ~68ms |
| 512 | ~40ms | ~36ms |
| 256 | ~25ms | ~22ms |
| 128 | 20.50ms | 18.50ms |

### 2.5 Real-Time Factor (RTF) Comparison

**Critical finding**: Inference framework choice matters enormously.

| Implementation | Block Size=128 | Block Size=2048 |
|----------------|----------------|-----------------|
| RTNeural | RTF=0.29 | RTF=0.29 |
| LibTorch | RTF=1.18 | RTF=0.10 |

- RTNeural: Consistent performance regardless of block size (pre-allocated memory)
- LibTorch: FAILS real-time at small block sizes (RTF > 1.0) but fast at large blocks
- This means **LibTorch cannot run BRAVE at 128-sample blocks in real-time**
- RTNeural or a custom C++ implementation is required for low-latency BRAVE

### 2.6 Audio Quality (FAD Scores)

| Model | Filosax | Drumset |
|-------|---------|---------|
| RAVE v1 | 43.95 | 1.48 |
| BRAVE | 9.03 | 2.21 |

Lower FAD = closer to training distribution. BRAVE actually outperforms RAVE on melodic content (Filosax) while being slightly worse on percussion.

### 2.7 Content Preservation

| Metric | BRAVE | RAVE v1 |
|--------|-------|---------|
| Pitch accuracy | 0.68-0.69 | 0.29-0.53 |
| Loudness L1 error | 4.31-9.94 | 13.75-34.24 |

BRAVE preserves pitch and dynamics significantly better than RAVE, likely because the smaller model relies more on encoder information rather than hallucinating content.

### 2.8 Practical Deployment

The **Minifusion** plugin can run BRAVE models at <10ms latency. For custom JUCE deployment, RTNeural is the recommended inference backend (not LibTorch) due to consistent RTF at small block sizes.

---

## 3. DDSP (Differentiable Digital Signal Processing)

**Source**: https://github.com/magenta/ddsp
**Paper**: [DDSP: Differentiable Digital Signal Processing](https://arxiv.org/abs/2001.04643) (Engel et al., ICLR 2020)
**Developed by**: Google Magenta
**License**: Apache 2.0
**Status**: Archived (October 2024), but functional

### 3.1 Architecture

```
DDSP ARCHITECTURE (Text Diagram)
==================================

Input Audio (monophonic)
    |
    +---> Pitch Detector (CREPE / tiny CREPE)
    |         |
    |         v  f0 (fundamental frequency)
    |
    +---> Loudness Extractor
    |         |
    |         v  loudness (dB)
    |
    +---> (Optional) Z Encoder
              |
              v  z (timbre embedding)

    [f0, loudness, z]
         |
         v
    +------------------+
    |   GRU (RNN)      |  Recurrent neural network
    |   Decoder         |  Maps features to DSP controls
    +------------------+
         |
    +----+----+----+
    |         |         |
    v         v         v
+--------+ +--------+ +-----------+
|Harmonic| |Filtered| |Differenti-|
|Additive| |Noise   | |able       |
|Synth   | |Synth   | |Reverb     |
+--------+ +--------+ +-----------+
    |         |         |
    v         v         v
    +----+----+         |
         |              |
         v              v
    dry signal    +  reverb
         |
         v
    Output Audio
```

### 3.2 DSP Components (Differentiable)

**Harmonic Additive Synthesizer**:
- Generates up to 100 sinusoidal partials at integer multiples of f0
- Neural network predicts amplitude for each partial at each time frame
- Sum of sinusoids = harmonic content (pitched sound)
- Fully differentiable: gradients flow through frequency/amplitude parameters

**Filtered Noise Synthesizer**:
- White noise shaped by time-varying FIR filters
- Models breath, bow noise, consonants, and other aperiodic content
- Filter coefficients predicted by the neural network per frame

**Differentiable Reverberator**:
- Learnable room acoustics
- Impulse response parameterized by neural network
- Adds spatial context to dry synthesis

### 3.3 Key Design Philosophy

DDSP is NOT pure neural synthesis. It is a hybrid approach:
- The neural network does NOT generate audio samples directly
- Instead, it predicts control parameters for traditional DSP modules
- The DSP modules are differentiable, so end-to-end training via backpropagation works
- This dramatically reduces model complexity vs sample-by-sample generation

**What this means**:
- Very small models (~160k-240k parameters)
- Very fast inference (40x real-time on CPU)
- Very low CPU usage (~15 MFLOPS for vocoder)
- BUT: limited to sounds decomposable into harmonics + noise
- Cannot handle polyphonic audio or complex timbres (drums, chords, noise-heavy sounds)

### 3.4 DDSP-VST Plugin

**Source**: https://github.com/magenta/ddsp-vst
**Status**: Archived (October 2024)

Built with JUCE framework. Key implementation details:
- Real-time reimplementation of DDSP in PyTorch (interfacing C++ via LibTorch)
- **Tiny CREPE** pitch detector: ~160k parameters (137x smaller than original CREPE)
- Trained via network distillation from full CREPE
- RNN prediction interval increased from 4ms to 20ms (5x efficiency boost, minimal quality loss)
- Non-causal convolutions avoided (would add latency from "seeing into the future")
- Processes both MIDI input and audio input
- 5 pre-trained instrument models included
- 48kHz synthesis (upgraded from original 16kHz)

### 3.5 Training Requirements

| Requirement | Specification |
|-------------|---------------|
| Dataset size | 10-15 minutes of clean, monophonic recordings |
| Dataset type | Solo instrument, single notes preferred |
| Preprocessing | Resampled to 16kHz, split into 4-sec chunks, 1-sec hops |
| Features extracted | f0, loudness, spectral features at 250 fps |
| Training steps | ~40,000 steps, batch size 32 |
| Training time | 3-20 hours on Colab GPU (< 1 hour on Colab Pro) |
| GPU requirements | Single GPU sufficient (Colab free tier works) |
| Training cost | Free (Colab) to ~$5 (cloud GPU) |

### 3.6 Performance Characteristics

| Metric | Value |
|--------|-------|
| Model parameters | 160k-240k (tiny), up to 400k |
| Model file size | ~1-5 MB |
| Inference speed | 40x real-time on CPU |
| CPU usage | ~15 MFLOPS (vocoder only) |
| Latency | ~20ms (RNN prediction interval) |
| Sample rate | 48kHz (VST), 16kHz (original) |
| Audio quality | Good for monophonic pitched sounds |

### 3.7 Limitations

- **Monophonic only**: Assumes single f0 at each time step
- **Harmonic sounds only**: Cannot model drums, noise, or complex textures
- **Pitch detection dependent**: Quality depends on accurate f0 estimation
- **Project archived**: No active development from Google Magenta
- **Limited timbral range**: Cannot capture the full complexity of some instruments

### 3.8 DDSP Variants and Extensions

**DDSP-SFX** ([paper](https://arxiv.org/abs/2309.08060)):
- Extends DDSP for sound effects generation (footsteps, gunshots, impacts)
- Adds transient modeling for impulsive signals
- Addresses DDSP's limitation with non-harmonic content
- Presented at DAFx 2024

**DDSP-Piano** (HAL archives):
- Specialized for piano synthesis
- Informed by physical modeling principles

**Realtime DDSP v2.1** (Antoine Caillon, IRCAM):
- PyTorch reimplementation
- 40x real-time CPU generation
- Bridge between DDSP and RAVE research

---

## 4. Neutone SDK

**Source**: https://github.com/Neutone/neutone_sdk
**Paper**: [Neutone SDK: An Open Source Framework for Neural Audio Processing](https://arxiv.org/abs/2508.09126) (AES 2025)
**Developed by**: Qosmo Inc.
**License**: MIT

### 4.1 Architecture

```
NEUTONE SYSTEM ARCHITECTURE
=============================

PYTHON SIDE (Training/Wrapping)          C++ SIDE (Plugin Host)
================================         ========================

+------------------+                     +------------------+
| Your PyTorch     |                     | Neutone FX       |
| Audio Model      |                     | (VST3/AU Plugin) |
| (any arch)       |                     |                  |
+------------------+                     | +----------+     |
        |                                | | TorchScrip|     |
        v                                | | Runtime   |     |
+------------------+                     | +----------+     |
| NeutoneModel     |                     |       |          |
| Wrapper          |                     | +----------+     |
| - W2W (realtime) |                     | | SQW      |     |
| - NRB (offline)  |     Export .nm      | | Buffer   |     |
+------------------+  ================>  | | Manager  |     |
        |                                | +----------+     |
        v                                |       |          |
+------------------+                     | +----------+     |
| SampleQueueWrapper|                    | | Sample   |     |
| - Buffer mgmt    |                     | | Rate     |     |
| - Sample rate    |                     | | Convert  |     |
| - Latency calc   |                     | +----------+     |
+------------------+                     +------------------+
        |
        v
+------------------+
| Benchmark &      |
| Profile Tools    |
| - Speed (RTF)    |
| - Latency        |
| - CPU/RAM        |
+------------------+
```

### 4.2 Model Wrapping Process

Neutone provides abstract base classes for wrapping PyTorch models:

**WaveformToWaveformBase (W2W)**: Real-time streaming
- Mono/stereo input (1-2 channels)
- Variable buffer lengths
- Continuous control parameters (0-1 range)
- Categorical parameters (discrete positions)
- Must specify native buffer sizes and sample rates

**NonRealtimeBase (NRB)**: Offline processing
- Multi-track support
- Text parameter input
- Used for intensive models (stem separation, etc.)

**Control Parameters**:
- Up to 4 continuous knobs (0.0 to 1.0)
- Categorical selectors
- Text input (non-realtime only)
- All parameters automatically exposed in the DAW

### 4.3 Buffer and Latency Management (SampleQueueWrapper)

The SampleQueueWrapper (SQW) is the core infrastructure:

1. **Buffer size adaptation**: FIFO queue bridges model's native buffer size with DAW's buffer size
2. **Sample rate conversion**: Two zero-allocation resampling options:
   - Linear interpolator (~40% faster than PyTorch's interpolate)
   - 4-point Hermite spline (~1.8x slower than linear, better quality)
3. **Latency calculation**: Automatic delay computation + DAW reporting for compensation
4. **Pre-allocation**: Circular queues for audio, parameters, and lookbehind buffers

**Delay = buffering_delay + model_delay**
- Buffering delay: computed automatically from buffer size mismatch
- Model delay: reported by model creator in the wrapper

### 4.4 Deployment Process

1. Wrap PyTorch model using W2W or NRB base class
2. Run local benchmarks (speed, latency, CPU/RAM profiling)
3. Export as TorchScript file with bundled metadata and example audio
4. Load into **Neutone FX** (real-time VST3/AU) or **Neutone Gen** (offline)
5. Optional: submit to community model browser via GitHub

**Key advantage**: No C++ code required. Zero JUCE knowledge needed. Model can be deployed in under a day.

### 4.5 Performance Characteristics

| Metric | Value |
|--------|-------|
| Plugin formats | VST3, AU (macOS, Windows) |
| Real-time models | Neutone FX plugin |
| Offline models | Neutone Gen plugin |
| Inference engine | TorchScript (LibTorch) |
| Model size limit | Not explicitly limited (CPU-bounded) |
| Latency | Model-dependent + buffer adaptation overhead |
| Control parameters | Up to 4 knobs + categoricals |

### 4.6 Strengths and Limitations

**Strengths**:
- Fastest path from PyTorch model to DAW deployment
- No C++ required
- Handles buffer/sample rate conversion automatically
- Growing ecosystem of community models
- Active development (v1.1.1+)
- Real-time safety focus (minimizes dynamic allocations)

**Limitations**:
- Locked to Neutone host plugin (cannot embed in your own plugin)
- TorchScript dependency (deprecated by PyTorch, shifting to torch.export)
- Limited to 4 control parameters
- Model runs on CPU only (no GPU acceleration in plugin)
- Cannot customize the plugin UI
- Distribution tied to Neutone ecosystem

### 4.7 Comparison with Direct Deployment

| Aspect | Neutone SDK | Custom JUCE + LibTorch |
|--------|-------------|----------------------|
| Time to deploy | Hours | Weeks to months |
| C++ required | No | Yes |
| Custom UI | No | Yes |
| Control params | 4 knobs | Unlimited |
| Distribution | Neutone ecosystem | Your own channels |
| Plugin branding | Neutone branding | Your branding |
| Thread safety | Handled | Your responsibility |
| Buffer management | Automatic | Your responsibility |

---

## 5. ANIRA (Neural Network Inference for Real-Time Audio)

**Source**: https://github.com/anira-project/anira
**Paper**: [ANIRA: An Architecture for Neural Network Inference in Real-Time Audio Applications](https://arxiv.org/abs/2506.12665)
**License**: Apache 2.0

### 5.1 What It Solves

ANIRA is a meta-framework that wraps inference engines with real-time safe thread pool architecture. It solves the fundamental problem: ML inference engines (LibTorch, ONNX Runtime, TFLite) are NOT real-time safe -- they allocate memory, use locks, and have non-deterministic runtimes.

### 5.2 Architecture

```
ANIRA THREAD ARCHITECTURE
===========================

Audio Thread (real-time safe)
+---------------------------+
| processBlock()            |
| - Writes input to ring    |
|   buffer                  |
| - Reads output from ring  |
|   buffer                  |
| - NO inference here       |
+---------------------------+
        |                ^
        v                |
   [Ring Buffer In]  [Ring Buffer Out]
        |                ^
        v                |
+---------------------------+
| Static Thread Pool        |
| (background, non-RT)      |
| - Picks up input chunks   |
| - Runs inference          |
| - Writes output chunks    |
| - Deterministic scheduling|
+---------------------------+
        |
        v
+---------------------------+
| Inference Backend         |
| (choose one):             |
| - LibTorch                |
| - ONNX Runtime            |
| - TensorFlow Lite         |
+---------------------------+
```

### 5.3 Key Features

- **Backend flexibility**: Switch between LibTorch, ONNX Runtime, TFLite without changing code
- **Static thread pool**: Prevents system oversubscription
- **Real-time safety**: Verified with rtsan sanitizer
- **Pre-allocated buffers**: No runtime memory allocation on audio thread
- **Latency reporting**: `get_latency()` returns processing delay in samples
- **JUCE integration**: Built-in VST3 example plugin
- **Cross-platform**: macOS, Linux, Windows

### 5.4 Backend Performance Summary

| Model Type | Fastest Backend |
|------------|----------------|
| Stateless (CNN, feedforward) | ONNX Runtime |
| Stateful (RNN, GRU, LSTM) | LibTorch |
| Small fixed-architecture | RTNeural (not via ANIRA) |

### 5.5 When to Use ANIRA

- You want to deploy a neural model in a JUCE plugin
- You want backend flexibility (test ONNX vs LibTorch vs TFLite)
- You need guaranteed real-time safety
- You don't want to build your own thread pool architecture
- Good complement to RAVE/BRAVE/DDSP model deployment

---

## 6. RTNeural

**Source**: https://github.com/jatinchowdhury18/RTNeural
**Paper**: [RTNeural: Fast Neural Inferencing for Real-Time Systems](https://arxiv.org/abs/2106.03037)
**License**: BSD-3

### 6.1 Overview

RTNeural is a lightweight C++ inference library designed specifically for real-time audio. Unlike LibTorch/ONNX/TFLite, it can run safely ON the audio thread because it:
- Pre-allocates all memory before inference
- Makes zero allocations during inference
- Has deterministic, bounded execution time
- Supports compile-time model specification for maximum optimization

### 6.2 Supported Layers

- Dense (fully connected)
- Conv1D
- GRU, LSTM (recurrent)
- Activation functions (tanh, sigmoid, ReLU, etc.)
- Batch normalization

### 6.3 Backends

Three interchangeable backends:
- **Eigen**: General-purpose linear algebra
- **xsimd**: SIMD-optimized (SSE, AVX, NEON)
- **STL**: Pure C++ standard library (most portable)

### 6.4 Key Advantage for Neural Synthesis

BRAVE's research demonstrates that RTNeural achieves consistent RTF=0.29 at all block sizes, while LibTorch fails real-time at small block sizes (RTF=1.18 at 128 samples). This makes RTNeural the ONLY viable option for low-latency neural synthesis (<10ms).

### 6.5 Limitations

- Limited operator support (no transposed convolutions, limited layer types)
- Model must be manually specified in C++ or loaded from JSON
- Not suitable for very large/complex architectures
- No GPU support (CPU only, by design)

### 6.6 Commercial Users

- AIDA-X (neural amp modeler)
- BYOD (guitar distortion)
- Chow Centaur (pedal emulation)
- Chow Tape Model (tape emulation)
- Several commercial products

---

## 7. Other Neural Synthesis Approaches

### 7.1 Neural Vocoders

**HiFi-GAN**
- Source: https://github.com/jik876/hifi-gan
- Architecture: Generator + multi-scale/multi-period discriminator
- Performance: 13.4x real-time on CPU (small footprint version)
- Quality: Near-indistinguishable from WaveNet at much lower cost
- Use case: Spectrogram-to-waveform conversion (speech TTS primarily)
- Plugin viability: Possible but primarily designed for speech pipelines
- Could be used as a decoder stage in a neural synthesis plugin

**WaveGlow**
- NVIDIA's flow-based vocoder
- Generates audio in a single forward pass (non-autoregressive)
- Requires GPU for real-time operation
- Not practical for CPU-based audio plugins

### 7.2 Autoregressive Models

**WaveRNN**
- Single-layer RNN with dual softmax
- 4x real-time on GPU at 24kHz
- Sparse variant achieves real-time on mobile CPU
- Very high quality but sequential generation is slow
- NOT practical for real-time audio plugins on standard CPU

**SampleRNN**
- Multi-scale RNN operating at different temporal resolutions
- Memory-efficient training
- Sequential generation prevents real-time use
- Historical significance but superseded by RAVE/DDSP

### 7.3 Language Model Approaches

**AudioLM** (Google Research)
- Treats audio generation as language modeling over discrete tokens
- Semantic tokens (from w2v-BERT) + acoustic tokens (from SoundStream codec)
- Generates coherent speech and music continuations
- **NOT real-time**: Multi-stage autoregressive inference
- Context limited to ~10-15 seconds (quadratic attention cost)
- Significant for research but not deployable in audio plugins

**MusicLM** (Google Research)
- Extends AudioLM with text conditioning
- Generates music from text descriptions
- Entirely offline -- minutes of compute for seconds of audio
- Not applicable to real-time plugin use

### 7.4 Neural Codec Models

**SoundStream / EnCodec / DAC**
- Neural audio codecs that compress audio to discrete tokens
- Very efficient (1.5-24 kbps)
- Real-time encoding and decoding possible
- Could serve as the backbone for a plugin that manipulates compressed representations
- EnCodec (Meta) and DAC (Descript) are open source

---

## 8. Reference Implementations

### 8.1 Scyclone (Torsion Audio)

**Source**: https://github.com/Torsion-Audio/Scyclone
**Type**: Real-time Neural Timbre Transfer Plugin
**Formats**: VST3, AU
**License**: Open source

**Technical details**:
- Uses RAVE v1 models (not v2)
- Configuration: LATENT_SIZE=16, CAPACITY=32
- **Inference engine: ONNX Runtime** (NOT LibTorch)
- ONNX Runtime linked statically (ORT format)
- Built with JUCE framework
- Cross-platform: macOS (x64, ARM64), Windows

**Signal chain**:
1. Pre-processing: Transient Controller + Low/High-Cut Filter
2. Neural inference: RAVE model (single or coupled mode)
3. Post-processing: Grain Delay + Blend + Post-Compressor

**Pre-trained models**:
- Funk Drums: 4 hours of vintage drum breaks
- Djembe: 5 hours of djembe recordings

**Custom model training** (via Scyclone-AI repo):
- Train RAVE model on your dataset
- Export to ONNX format
- Load into Scyclone

**Key lesson**: Scyclone chose ONNX Runtime over LibTorch for deployment, likely due to:
- Smaller binary size
- More consistent cross-platform behavior
- Static linking simplicity
- Better inference performance for this model type (stateless CNN)

### 8.2 DDSP-VST (Google Magenta)

**Source**: https://github.com/magenta/ddsp-vst
**Type**: Neural Synthesizer and Audio Effect
**Status**: Archived but functional

**Technical details**:
- JUCE framework
- LibTorch for inference (PyTorch C++ reimplementation)
- Tiny CREPE pitch detector (~160k params)
- 5 pre-trained instrument models
- MIDI and audio input modes
- 48kHz processing

### 8.3 Minifusion

- Runs BRAVE models at <10ms latency
- Optimized for low-latency interaction
- Uses RTNeural-based inference

### 8.4 OBSIDIAN-Neural

- Described as "world's first neural network VST for live performance"
- Uses Stable Audio Open for stereo loop generation
- Synchronized to DAW tempo and sample rate
- More of a generative AI composition tool than a traditional synthesizer

---

## 9. Deployment Pathways: Python Training to C++ Plugin

### 9.1 Pathway Comparison

```
DEPLOYMENT PATHWAYS OVERVIEW
==============================

PATH A: TorchScript + LibTorch (RAVE default)
----------------------------------------------
Python: model = RAVE()
        train(model)
        scripted = torch.jit.script(model)
        torch.jit.save(scripted, "model.ts")

C++:    torch::jit::Module model = torch::jit::load("model.ts");
        auto output = model.forward({input});

Pros: Direct PyTorch compatibility, supports RNNs well
Cons: Large binary (~267MB), TorchScript deprecated,
      not real-time safe on audio thread
Best for: RAVE models with ANIRA thread pool


PATH B: ONNX Runtime (Scyclone approach)
-----------------------------------------
Python: torch.onnx.export(model, dummy_input, "model.onnx")

C++:    Ort::Session session(env, "model.onnx", options);
        session.Run(...)

Pros: Smaller binary (~50-100MB), faster for CNNs,
      cross-platform, CoreML/TensorRT acceleration
Cons: Limited RNN support, operator compatibility issues,
      not real-time safe on audio thread
Best for: Stateless CNN models (like RAVE) with ANIRA


PATH C: RTNeural (BRAVE approach)
----------------------------------
Python: export_weights_to_json(model, "weights.json")
        # Manual: map architecture to RTNeural layers

C++:    auto model = RTNeural::ModelType<float>{};
        model.load_json("weights.json");
        float output = model.forward(input);
        // SAFE ON AUDIO THREAD

Pros: Audio-thread safe, tiny binary (<1MB added),
      consistent performance at all block sizes,
      fastest for small models
Cons: Limited layer support, manual architecture mapping,
      not suitable for large models
Best for: BRAVE, small custom models, amp modeling


PATH D: Neutone SDK (no C++ required)
--------------------------------------
Python: class MyWrapper(WaveformToWaveformBase):
            def do_forward_pass(self, x, params):
                return self.model(x)
        save_neutone_model(wrapper, "model.nm")

DAW:    Load Neutone FX plugin -> Browse -> Load model

Pros: Zero C++ code, handles everything automatically,
      fastest development time
Cons: Locked to Neutone plugin, limited UI customization,
      4 control params max
Best for: Research prototyping, quick deployment


PATH E: ANIRA Meta-Framework
------------------------------
Python: Export model to TorchScript, ONNX, or TFLite

C++:    anira::InferenceConfig config;
        config.model_path = "model.onnx";
        anira::InferenceHandler handler(config);
        // Handles thread pool, buffering, real-time safety

Pros: Backend-agnostic, real-time safe by design,
      JUCE integration example provided
Cons: Additional abstraction layer, limited community
Best for: Production plugins needing backend flexibility
```

### 9.2 TorchScript Deprecation Note

PyTorch is deprecating TorchScript in favor of `torch.export`. This affects:
- RAVE's default export pipeline
- Neutone SDK's deployment format
- LibTorch-based inference

The ecosystem is shifting toward:
- `torch.export` -> ExecuTorch (mobile/edge) or ONNX
- ONNX becoming the more future-proof export format
- RTNeural remaining stable (doesn't depend on PyTorch serialization)

---

## 10. Comparison Tables

### 10.1 Neural Synthesis Technologies

| Technology | Type | Params | Latency | CPU Cost | Audio Quality | Polyphonic | Real-Time |
|------------|------|--------|---------|----------|--------------|------------|-----------|
| RAVE (standard) | Pure neural | 9-17M | 200-500ms | Moderate | Excellent | Mono only | Yes (high latency) |
| BRAVE | Pure neural | 4.9M | <10ms | Moderate | Good | Mono only | Yes |
| DDSP | Hybrid neural-DSP | 160-400k | ~20ms | Very low | Good (pitched) | No | Yes |
| HiFi-GAN | Neural vocoder | 1-14M | ~5-20ms | Moderate | Excellent (speech) | N/A | Yes (small ver.) |
| WaveRNN | Autoregressive | ~1M | N/A | High | Excellent | No | No (CPU) |
| AudioLM | Language model | Billions | Seconds | Extreme | Excellent | Yes | No |
| SoundStream/EnCodec | Neural codec | 5-15M | ~10ms | Low-Mod | Good | Yes | Yes |

### 10.2 Deployment Frameworks

| Framework | Binary Size | Real-Time Safe | RNN Support | Best For |
|-----------|-------------|---------------|-------------|----------|
| LibTorch | ~267MB | No (needs thread pool) | Excellent | RAVE, large models |
| ONNX Runtime | ~50-100MB | No (needs thread pool) | Limited | CNNs, Scyclone-style |
| TFLite | ~5-10MB | No (needs thread pool) | Good | Mobile, small models |
| RTNeural | <1MB | YES (audio thread safe) | Good (GRU/LSTM) | BRAVE, amp models |
| ANIRA | Wrapper (~5MB + engine) | YES (provides thread pool) | Via backend | Production plugins |
| Neutone SDK | N/A (host plugin) | Handled | Via TorchScript | Quick prototyping |

### 10.3 Training Requirements

| Technology | Dataset Size | Training Time | GPU Needed | Cloud Cost |
|------------|-------------|---------------|------------|------------|
| RAVE | 1-3+ hours audio | 1-3 weeks | 8-32GB VRAM | $50-200 |
| BRAVE | 1-3+ hours audio | 1-2 weeks | 8GB+ VRAM | $50-150 |
| DDSP | 10-15 min audio | 3-20 hours | Any GPU (Colab free) | $0-5 |
| HiFi-GAN | Hours of speech | 1-3 days | 16GB+ VRAM | $30-100 |
| Custom RTNeural | Varies | Hours to days | Varies | $0-50 |

### 10.4 JUCE Plugin Viability

| Technology | JUCE Plugin Viable? | Deployment Complexity | Example Plugin |
|------------|--------------------|-----------------------|----------------|
| RAVE | Yes (high latency) | High (LibTorch/ONNX) | Scyclone |
| BRAVE | Yes (low latency) | High (RTNeural custom) | Minifusion |
| DDSP | Yes | High (archived project) | DDSP-VST |
| Neutone models | Yes (via Neutone host) | Low (no C++) | Neutone FX |
| RTNeural models | Yes | Medium | AIDA-X, Chow* |
| HiFi-GAN | Possible | High | None public |
| AudioLM/MusicLM | No | N/A | N/A |

---

## 11. Practical Recommendations

### 11.1 For Building a Neural Timbre Transfer Plugin (RAVE-based)

**Recommended stack**:
- Train: RAVE v2 in Python (PyTorch)
- Export: ONNX format (following Scyclone's approach)
- Inference: ONNX Runtime via ANIRA thread pool
- Framework: JUCE 8.x
- Latency: Accept 200-500ms (studio use, not live performance)

**Steps**:
1. Collect 3+ hours of target timbre audio
2. Train RAVE v2 model (~1-2 weeks on GPU)
3. Export with `rave export --streaming`
4. Convert to ONNX
5. Integrate ANIRA into JUCE plugin for real-time safe inference
6. Expose latent space dimensions as plugin parameters
7. Add pre/post processing (like Scyclone's transient shaper, blend, etc.)

### 11.2 For Building a Low-Latency Neural Synthesizer (BRAVE-based)

**Recommended stack**:
- Train: BRAVE variant of RAVE (Cr=128, causal)
- Export: RTNeural-compatible weights
- Inference: RTNeural (audio-thread safe)
- Framework: JUCE 8.x
- Latency: <10ms (suitable for live performance)

**Trade-offs**: Lower audio quality, more complex deployment, requires manual RTNeural architecture mapping.

### 11.3 For Building a Neural Instrument (DDSP-based)

**Recommended stack**:
- Train: DDSP autoencoder in Python (TensorFlow/PyTorch)
- Export: ONNX or TorchScript
- Inference: ONNX Runtime or custom C++ DSP
- Framework: JUCE 8.x
- Latency: ~20ms (good for keyboard playing)

**Advantages**: Very small model, very low CPU, easy to train, works with MIDI input.
**Limitation**: Monophonic pitched sounds only.

### 11.4 For Rapid Prototyping

**Recommended**: Neutone SDK
- Train any PyTorch model
- Wrap in <100 lines of Python
- Deploy same day
- Test in DAW immediately
- Move to custom JUCE plugin later if needed

### 11.5 Architecture Decision Tree

```
What kind of neural synthesis plugin?
|
+-- Timbre transfer (studio use, latency OK)
|   |
|   +-> RAVE + ONNX Runtime + ANIRA + JUCE
|       (200-500ms latency, excellent quality)
|
+-- Live performance instrument (<10ms required)
|   |
|   +-> BRAVE + RTNeural + JUCE
|       (sub-10ms latency, good quality)
|
+-- Neural instrument from MIDI
|   |
|   +-> DDSP + ONNX/custom DSP + JUCE
|       (~20ms latency, monophonic only)
|
+-- Quick prototype / research
|   |
|   +-> Neutone SDK (no C++ needed)
|       (model-dependent latency)
|
+-- Amp/pedal modeling
|   |
|   +-> RTNeural + JUCE (proven ecosystem)
|       (<1ms latency, excellent quality)
|
+-- Texture/atmosphere generation (offline OK)
    |
    +-> RAVE + prior model OR AFTER (diffusion)
        (non-real-time, highest quality)
```

### 11.6 Cost and Effort Estimates for a JUCE Neural Synthesis Plugin

| Phase | Time | Cost |
|-------|------|------|
| Dataset collection | 1-2 weeks | $0 (own recordings) |
| Model training (RAVE) | 1-3 weeks GPU time | $50-200 cloud |
| Model training (DDSP) | 1 day | $0-5 cloud |
| C++ integration (ANIRA path) | 2-4 weeks dev time | Dev labor |
| C++ integration (RTNeural path) | 4-8 weeks dev time | Dev labor |
| C++ integration (Neutone path) | 1-2 days | Dev labor |
| UI development | 2-4 weeks | Dev labor |
| Testing and optimization | 2-4 weeks | Dev labor |
| **Total (RAVE + ANIRA)** | **2-3 months** | **$50-200 + labor** |
| **Total (DDSP + custom)** | **1-2 months** | **$0-5 + labor** |
| **Total (Neutone prototype)** | **1-3 days** | **$0-5** |

---

## Sources

### RAVE
- [RAVE GitHub Repository](https://github.com/acids-ircam/RAVE)
- [RAVE Paper (arXiv)](https://arxiv.org/abs/2111.05011)
- [RAVE Official Page](https://acids-ircam.github.io/RAVE/)
- [RAVE Training Tutorial (IRCAM Forum)](https://forum.ircam.fr/article/detail/training-rave-models-on-custom-data/)
- [RAVE Guide (NeuralAnalog)](https://neuralanalog.com/docs/rave-model-ircam)
- [RAVE Training Compendium (GitHub Discussion)](https://github.com/acids-ircam/RAVE/discussions/300)
- [cached_conv Paper](https://arxiv.org/abs/2204.07064)
- [cached_conv GitHub](https://github.com/acids-ircam/cached_conv)
- [nn~ External](https://github.com/acids-ircam/nn_tilde)
- [RAVE Models Download](https://acids-ircam.github.io/rave_models_download)
- [AFTER GitHub](https://github.com/acids-ircam/AFTER)

### BRAVE
- [BRAVE GitHub](https://github.com/fcaspe/BRAVE)
- [BRAVE Paper (arXiv)](https://arxiv.org/abs/2503.11562)
- [BRAVE Project Page](https://fcaspe.github.io/brave/)
- [BRAVE NIME 2025 Paper](https://instrumentslab.org/data/andrew/caspe_nime2025.pdf)

### DDSP
- [DDSP GitHub](https://github.com/magenta/ddsp)
- [DDSP Paper (ICLR 2020)](https://arxiv.org/abs/2001.04643)
- [DDSP Explained (NeuralAnalog)](https://neuralanalog.com/docs/ddsp-model-magenta)
- [DDSP-VST GitHub](https://github.com/magenta/ddsp-vst)
- [DDSP-VST Blog Post](https://magenta.withgoogle.com/ddsp-vst-blog)
- [DDSP Training Colab](https://colab.research.google.com/github/magenta/ddsp/blob/main/ddsp/colab/demos/Train_VST.ipynb)
- [DDSP-SFX Paper](https://arxiv.org/abs/2309.08060)

### Neutone SDK
- [Neutone SDK GitHub](https://github.com/Neutone/neutone_sdk)
- [Neutone SDK Paper (AES 2025)](https://arxiv.org/abs/2508.09126)
- [Neutone FX Plugin](https://neutone.ai/fx)
- [Neutone PyPI](https://pypi.org/project/neutone_sdk/)

### ANIRA
- [ANIRA GitHub](https://github.com/anira-project/anira)
- [ANIRA Paper (arXiv)](https://arxiv.org/abs/2506.12665)
- [ANIRA Documentation](https://anira-project.github.io/anira/)

### RTNeural
- [RTNeural GitHub](https://github.com/jatinchowdhury18/RTNeural)
- [RTNeural Paper](https://ccrma.stanford.edu/~jatin/rtneural/)

### Reference Plugins
- [Scyclone GitHub (Torsion Audio)](https://github.com/Torsion-Audio/Scyclone)
- [Scyclone-AI (Training)](https://github.com/Torsion-Audio/Scyclone-AI)
- [Scyclone KVR Listing](https://www.kvraudio.com/product/scyclone-by-torsion-audio)
- [nn-inference-template (Torsion Audio)](https://github.com/Torsion-Audio/nn-inference-template)
- [OBSIDIAN-Neural](https://obsidian-neural.com/)

### Other
- [AudioLM Paper](https://arxiv.org/abs/2209.03143)
- [HiFi-GAN GitHub](https://github.com/jik876/hifi-gan)
- [WaveRNN Paper](https://arxiv.org/abs/1802.08435)
- [ONNX vs PyTorch Speed Comparison](https://dev-kit.io/blog/machine-learning/onnx-vs-pytorch-speed-comparison)
- [JUCE + LibTorch Discussion](https://forum.juce.com/t/using-libtorch-with-juce/41154)
- [JUCE + ONNX Discussion](https://forum.juce.com/t/deploying-an-onnx-model/59753)
