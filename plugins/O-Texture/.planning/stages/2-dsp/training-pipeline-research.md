# Stage 2: DSP (Training Pipeline) - Research

**Researched:** 2026-02-14
**Domain:** PyTorch training pipeline for 1D CNN VAE audio texture synthesis
**Confidence:** HIGH (verified across multiple authoritative sources)

## Summary

This research covers practical training implementation for O-Texture's 1D CNN VAE, focusing on Rain texture as the proof-of-concept model. The architecture is already designed (~1.2M parameters, 32-dim latent, 4096-sample blocks at 48kHz). This document addresses five key areas: rain dataset sourcing and preprocessing, VAE training hyperparameters and best practices, ONNX export considerations for ONNX Runtime 1.19.2, GRU prior model training specifics, and quality validation methods.

The standard approach is: curate ~30 minutes of rain audio from Freesound.org under Creative Commons licenses, train the VAE with AdamW optimizer using cyclical KL annealing (not simple linear warmup), use BatchNorm1d (matches the architecture spec and works well with batch size 32), export to ONNX opset 17 (well within ONNX Runtime 1.19.2's opset 21 ceiling), and validate via multi-scale spectral convergence metrics plus latent space visualization.

**Primary recommendation:** Use cyclical KL annealing with 4 cycles (R=0.5) instead of simple linear warmup -- this is the established solution for preventing posterior collapse in audio VAEs. Use AdamW with cosine annealing LR schedule starting at 1e-4.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Full ANIRA inference pipeline with real trained ONNX model (Rain texture)
- PyTorch training pipeline included in this milestone (Rain VAE + Rain prior)
- Local NVIDIA GPU available for training (~6-12 hours per texture)
- Generative mode ONLY; Transform mode deferred
- O-Texture (synth variant) only; O-Texture FX deferred
- Stereo decorrelation via dual-decode latent offset
- 1D Perlin noise for smooth Evolve random walk
- 48kHz only (no resampler)
- Overlap-add crossfading: 50% overlap, 4096-sample blocks, 2048-sample hop, Hann window
- Stage 2 verification requires real model output (not placeholder noise)
- Rain only (1 texture) -- validate full pipeline before training more

### Claude's Discretion
- Training hyperparameters (batch size, learning rate, scheduler, KL warmup strategy)
- Dataset curation approach and preprocessing pipeline details
- ONNX export opset version (within ONNX Runtime 1.19.2 compatibility)
- Prior model training sequence length and teacher forcing strategy
- Quality validation metrics and thresholds

### Deferred Ideas (OUT OF SCOPE)
- Adversarial fine-tuning (multi-scale discriminator)
- Transform mode / audio input processing
- INT8 quantization
- Remaining 5 texture categories (Metal, Wind, Crowd, Synth, Organic)
- User-trainable models
</user_constraints>

---

## 1. Rain Audio Dataset Sources and Curation

### 1.1 Primary Source: Freesound.org

**Confidence: HIGH** -- Freesound is the standard source for CC-licensed environmental audio in ML research (used by ESC-50, FSD50K, UrbanSound8K).

**Recommended Packs/Searches:**

| Source | URL | Notes |
|--------|-----|-------|
| BonnyOrbit Rain Pack | https://freesound.org/people/BonnyOrbit/packs/22535/ | Field recordings, Zoom H1 |
| be-steele Heavy Weather | https://freesound.org/people/be-steele/packs/39862/ | Unprocessed rainstorm field recordings |
| RJStefanski Thunderstorm | https://freesound.org/people/RJStefanski/sounds/256483/ | 96kHz/24-bit audiophile quality |
| Arctura Rain Loops | https://freesound.org/people/Arctura/sounds/39828/ | Long duration, H4 recorder |
| inchadney Light Rain | https://freesound.org/people/inchadney/sounds/11485/ | Professional equipment, forest setting |
| Opticreep Rain | https://freesound.org/people/Opticreep/sounds/320520/ | Field recording |
| kwazi Rain & Storm | https://freesound.org/people/kwazi/packs/3588/ | Varied rain types |

**Search Strategy:**
```
Freesound API search queries (use freesound-python client):
- "rain" filter:tag=field-recording,rain duration:[30 TO *]
- "rain ambient" filter:license="Creative Commons 0"
- "rainfall" filter:samplerate:48000 OR samplerate:44100
- "light rain" "heavy rain" "rain on roof" "rain on leaves"
```

**Freesound API Setup:**
```bash
pip install freesound-python
# Apply for API key at https://freesound.org/apiv2/apply/
```

```python
import freesound

client = freesound.FreesoundClient()
client.set_token("<your_api_key>", "token")

# Search for rain sounds, sorted by duration (longest first)
results = client.text_search(
    query="rain field recording",
    filter="tag:rain duration:[30 TO *]",
    sort="duration_desc",
    fields="id,name,duration,samplerate,bitdepth,license,previews,download"
)
```

### 1.2 Secondary Sources

| Source | License | Notes |
|--------|---------|-------|
| ESC-50 (rain class) | CC-BY-NC 3.0 | 40 clips x 5 seconds = ~3 min (supplementary only) |
| DataSEC (Zenodo) | CC-BY 4.0 | 23 hours environmental audio, may contain rain |
| Personal field recordings | N/A | If available, highest quality control |

### 1.3 Minimum Quality Requirements

**Confidence: HIGH** -- Based on RAVE training practices and audio ML standards.

| Requirement | Minimum | Recommended | Why |
|-------------|---------|-------------|-----|
| Sample rate | 44.1kHz (resample to 48k) | 48kHz native | Model trains at 48kHz |
| Bit depth | 16-bit | 24-bit | More dynamic range, less quantization noise |
| Noise floor | < -40 dBFS | < -50 dBFS | Model learns noise if present |
| Duration per file | 10 seconds | 30+ seconds | Longer files yield more training crops |
| Total duration | 20 minutes | 30-45 minutes | More data = better generalization |
| Format | WAV/FLAC | WAV (lossless) | No lossy compression artifacts |
| Content | Pure rain | Diverse rain types | Variety in latent space |

### 1.4 Rain Diversity Targets

Aim for coverage across these rain characteristics (each contributes different spectral content):

| Rain Type | Spectral Character | Target Duration |
|-----------|-------------------|-----------------|
| Light drizzle | High-frequency dominated, sparse transients | 5 min |
| Steady medium rain | Broadband, dense | 8 min |
| Heavy rain / downpour | Low-mid frequency dominated, very dense | 5 min |
| Rain on roof/metal | Strong transients, resonant | 4 min |
| Rain on leaves/foliage | Diffuse, organic flutter | 4 min |
| Rain with distant thunder | Low rumble + rain texture | 4 min |

### 1.5 Preprocessing Pipeline

**Confidence: HIGH** -- Standard audio ML preprocessing.

```python
import torchaudio
import torch
import torch.nn.functional as F
import os
from pathlib import Path

SAMPLE_RATE = 48000
BLOCK_SIZE = 4096
SILENCE_THRESHOLD_DB = -40  # dBFS

def preprocess_rain_dataset(input_dir: str, output_dir: str):
    """
    Full preprocessing pipeline for rain audio files.

    Steps:
    1. Load and convert to mono
    2. Resample to 48kHz
    3. Peak normalize to [-1, 1]
    4. Remove silence (below threshold)
    5. Save as 48kHz mono WAV
    """
    os.makedirs(output_dir, exist_ok=True)

    for filepath in Path(input_dir).glob("*"):
        if filepath.suffix.lower() not in ('.wav', '.flac', '.mp3', '.ogg', '.aiff'):
            continue

        waveform, sr = torchaudio.load(str(filepath))

        # 1. Convert to mono
        if waveform.shape[0] > 1:
            waveform = waveform.mean(dim=0, keepdim=True)

        # 2. Resample to 48kHz
        if sr != SAMPLE_RATE:
            resampler = torchaudio.transforms.Resample(sr, SAMPLE_RATE)
            waveform = resampler(waveform)

        # 3. Remove silence segments
        waveform = remove_silence(waveform, SAMPLE_RATE, SILENCE_THRESHOLD_DB)

        if waveform.shape[1] < BLOCK_SIZE:
            print(f"Skipping {filepath.name}: too short after silence removal")
            continue

        # 4. Peak normalize to [-1, 1]
        peak = waveform.abs().max()
        if peak > 0:
            waveform = waveform / peak

        # 5. Save
        out_path = Path(output_dir) / f"{filepath.stem}_48k.wav"
        torchaudio.save(str(out_path), waveform, SAMPLE_RATE)

        duration = waveform.shape[1] / SAMPLE_RATE
        print(f"Processed {filepath.name}: {duration:.1f}s")


def remove_silence(waveform: torch.Tensor, sr: int,
                   threshold_db: float = -40,
                   frame_length: int = 4096,
                   hop_length: int = 2048) -> torch.Tensor:
    """
    Remove segments below threshold from audio.
    Uses energy-based voice activity detection.
    """
    # Compute frame-level energy in dB
    frames = waveform.unfold(1, frame_length, hop_length)  # (1, n_frames, frame_length)
    energy = frames.pow(2).mean(dim=-1)  # (1, n_frames)
    energy_db = 10 * torch.log10(energy + 1e-10)

    # Create mask for active frames
    active = energy_db > threshold_db  # (1, n_frames)

    # Reconstruct audio from active frames only
    # Use overlap-add to avoid clicks at boundaries
    active_segments = []
    in_active = False
    start = 0

    for i in range(active.shape[1]):
        if active[0, i] and not in_active:
            start = i * hop_length
            in_active = True
        elif not active[0, i] and in_active:
            end = min(i * hop_length + frame_length, waveform.shape[1])
            active_segments.append(waveform[:, start:end])
            in_active = False

    if in_active:
        active_segments.append(waveform[:, start:])

    if not active_segments:
        return waveform  # Return original if all silence

    return torch.cat(active_segments, dim=1)
```

**Key preprocessing notes:**
- Do NOT apply any EQ, compression, or other processing -- the model should learn raw rain characteristics
- Do NOT remove transients (thunder rumbles are part of the texture vocabulary)
- Peak normalization is preferred over LUFS normalization for this use case because we want the model to learn amplitude variation within the [-1, 1] range
- Each preprocessed file should be checked by ear for quality before training

---

## 2. VAE Training Best Practices

### 2.1 Optimizer and Learning Rate

**Confidence: HIGH** -- Standard practice across audio ML literature.

| Setting | Recommended | Alternative | Rationale |
|---------|-------------|-------------|-----------|
| Optimizer | AdamW | Adam | AdamW decouples weight decay from gradient updates, better generalization |
| Initial LR | 1e-4 | 3e-4 | Conservative start, audio quality sensitive to LR |
| Weight decay | 1e-5 | 0 | Light regularization prevents overfitting |
| Betas | (0.9, 0.999) | (0.8, 0.99) | Standard Adam betas work well |
| LR Schedule | Cosine annealing | Step decay | Smooth decay, no sharp transitions |
| Warmup | 1000 steps linear | 500 steps | Prevents early instability |

**Learning Rate Schedule Implementation:**
```python
from torch.optim.lr_scheduler import CosineAnnealingLR, LinearLR, SequentialLR

optimizer = torch.optim.AdamW(model.parameters(), lr=1e-4, weight_decay=1e-5)

# 1000-step linear warmup then cosine decay
warmup = LinearLR(optimizer, start_factor=0.01, total_iters=1000)
cosine = CosineAnnealingLR(optimizer, T_max=total_steps - 1000, eta_min=1e-6)
scheduler = SequentialLR(optimizer, [warmup, cosine], milestones=[1000])
```

### 2.2 Batch Size

**Confidence: HIGH** -- Based on model size and VRAM constraints.

| GPU | VRAM | Max Batch Size (est.) | Recommended |
|-----|------|----------------------|-------------|
| RTX 3080 | 10 GB | ~48-64 | 32 |
| RTX 3090 | 24 GB | ~96-128 | 64 |
| RTX 4090 | 24 GB | ~96-128 | 64 |

**Use batch size 32 as baseline.** The model is small (~1.2M params), the input is small (1x4096 floats), but the multi-scale spectral loss computes 4 STFTs per sample which is the memory bottleneck.

With batch_size=32 and 30 minutes of audio:
- Total samples at 48kHz: 30 * 60 * 48000 = 86.4M samples
- 4096-sample crops: ~21,000 unique non-overlapping crops
- With crops_per_file=200 and random cropping: effectively unlimited
- Steps per epoch: depends on dataset virtual size, but aim for ~1000 steps/epoch

### 2.3 KL Warmup Strategy: Cyclical Annealing

**Confidence: HIGH** -- Cyclical annealing (Fu et al., 2019) is the established solution for KL vanishing in audio VAEs. Used by RAVE and related audio models.

**Do NOT use simple linear warmup.** Linear warmup often leads to posterior collapse where the KL term dominates too early and the decoder ignores the latent code.

**Cyclical annealing schedule (recommended):**

```python
import numpy as np

def frange_cycle_linear(n_iter, start=0.0, stop=1.0, n_cycle=4, ratio=0.5):
    """
    Cyclical annealing schedule for KL weight (beta).

    Args:
        n_iter: Total training iterations
        start: Starting beta value (0.0)
        stop: Maximum beta value (use 0.001 for audio VAEs, not 1.0)
        n_cycle: Number of cycles (4 recommended)
        ratio: Proportion of each cycle spent ramping (0.5 = half ramp, half constant)

    Returns:
        Array of beta values, one per iteration
    """
    L = np.ones(n_iter) * stop
    period = n_iter / n_cycle
    step = (stop - start) / (period * ratio)

    for c in range(n_cycle):
        v, i = start, 0
        while v <= stop and (int(i + c * period) < n_iter):
            L[int(i + c * period)] = v
            v += step
            i += 1
    return L

# Usage: 200k total steps, 4 cycles, max beta = 0.001
total_steps = 200_000
beta_schedule = frange_cycle_linear(total_steps, start=0.0, stop=0.001,
                                     n_cycle=4, ratio=0.5)
```

**How it works:**
- Each cycle: beta ramps from 0 to `stop` over first 50% of cycle, then stays at `stop`
- 4 cycles over 200k steps = each cycle is 50k steps (25k ramp + 25k constant)
- At the start of each new cycle, beta resets to 0 -- this lets the decoder use high-quality latent codes learned in the previous cycle
- The `stop` value for audio VAEs should be 0.001 (not 1.0 as in text VAEs) because the spectral loss magnitude is much larger than KL

**Alternative (simpler, less robust): linear warmup**
```python
# Only if cyclical is too complex -- but cyclical is strongly preferred
beta = min(0.001, step * 0.001 / 20000)  # Linear ramp over 20k steps
```

### 2.4 Gradient Clipping

**Confidence: HIGH** -- Standard practice for all generative models.

```python
torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
```

- **Use max_norm=1.0** as the starting point (matches the architecture spec)
- Monitor gradient norms during training -- if they regularly exceed 5-10, there may be a training stability issue
- The spectral loss can produce large gradients due to STFT computation; clipping at 1.0 prevents explosions

### 2.5 Normalization: BatchNorm1d

**Confidence: HIGH** -- The architecture already specifies BatchNorm1d, and this is the right choice.

**Use BatchNorm1d** (already in the architecture). Reasons:
- Batch size 32+ is large enough for stable BatchNorm statistics
- BatchNorm1d is well-supported for ONNX export (runs as identity-like affine transform in eval mode)
- RAVE uses BatchNorm in its convolutional stacks
- BatchNorm converges faster than LayerNorm for CNNs
- GroupNorm would be needed only if batch size < 8 (not our case)

**ONNX export consideration:** Call `model.eval()` before export. This folds running mean/variance into the BatchNorm parameters, making it a simple affine transformation in inference. No special handling needed.

### 2.6 Training Duration Estimation

**Confidence: MEDIUM** -- Based on model size extrapolation, not benchmarked.

| Parameter | Value |
|-----------|-------|
| Model parameters | ~1.2M |
| Input size | (32, 1, 4096) per batch |
| Forward pass ops | ~4M FLOPs (tiny) |
| STFT loss computation | ~50M FLOPs (dominates) |
| Estimated step time | 50-100ms on RTX 3080/4090 |
| Target steps | 150k-200k |
| Estimated wall time | **3-6 hours** on RTX 4090, **5-10 hours** on RTX 3080 |

**Recommendation:** Start with 100k steps and evaluate. If reconstruction quality is good, stop. The architecture is small enough that 200k steps is a generous upper bound.

**Training acceleration tips:**
- Use `torch.compile(model)` (PyTorch 2.x) for ~20-30% speedup
- Use `torch.amp.autocast('cuda')` for automatic mixed precision -- safe for this architecture
- Set `pin_memory=True` and `num_workers=4` in DataLoader
- Pre-load all audio to GPU memory if it fits (30 min mono 48kHz = ~350MB float32)

### 2.7 Loss Function Weights

**Confidence: HIGH** -- From the architecture spec, with refinements.

```python
# Recommended loss weights
total_loss = spectral_loss + 0.1 * waveform_l1_loss + beta * kl_loss
```

Where:
- `spectral_loss`: Multi-scale spectral convergence + log magnitude (4 scales: 2048, 1024, 512, 256)
- `waveform_l1_loss`: L1 distance between input and reconstruction waveforms
- `beta`: From cyclical annealing schedule (max 0.001)
- `kl_loss`: Standard KL divergence: `-0.5 * mean(1 + logvar - mu^2 - exp(logvar))`

**Important:** The spectral loss is the primary driver of perceptual quality. The waveform L1 loss provides phase/amplitude alignment but should be weighted lower (0.1x) because exact waveform matching is not the goal for textures.

---

## 3. ONNX Export Considerations

### 3.1 Opset Version Compatibility

**Confidence: HIGH** -- Verified from official ONNX Runtime compatibility table.

| ONNX Runtime Version | Max Opset | ONNX Spec |
|----------------------|-----------|-----------|
| 1.19.2 | **21** | 1.16.1 |
| 1.20 | 21 | 1.16.1 |

**Use opset 17** (as specified in architecture). This is well within ONNX Runtime 1.19.2's ceiling of opset 21. Opset 17 supports all operators used by the architecture (Conv, ConvTranspose, BatchNormalization, GRU, Linear, LeakyReLU, Tanh, AdaptiveAvgPool1d).

### 3.2 BatchNorm in ONNX Export

**Confidence: HIGH** -- Well-documented behavior.

```python
model.eval()  # CRITICAL: sets BatchNorm to inference mode

# In eval mode, BatchNorm uses running_mean and running_var (accumulated during training)
# ONNX export captures these as static constants
# No special handling needed -- just ensure model.eval() is called before export
```

**What happens internally:**
- Training mode: `output = (input - batch_mean) / sqrt(batch_var + eps) * gamma + beta`
- Eval mode: `output = (input - running_mean) / sqrt(running_var + eps) * gamma + beta`
- In ONNX: running_mean, running_var, gamma, beta are all stored as constant tensors

### 3.3 Encoder and Decoder Export

**Confidence: HIGH** -- Straightforward, no dynamic shapes needed.

```python
def export_vae_to_onnx(model, output_dir):
    model.eval()
    model.cpu()

    # --- Decoder (most important -- runs in real-time) ---
    z_dummy = torch.randn(1, 32)
    torch.onnx.export(
        model.decoder,
        z_dummy,
        f"{output_dir}/rain_decoder.onnx",
        input_names=["latent"],
        output_names=["audio"],
        dynamic_axes={"latent": {0: "batch"}, "audio": {0: "batch"}},
        opset_version=17,
        do_constant_folding=True,  # Folds BatchNorm constants
    )

    # --- Encoder (used in Transform mode -- export anyway for completeness) ---
    x_dummy = torch.randn(1, 1, 4096)
    torch.onnx.export(
        model.encoder,
        x_dummy,
        f"{output_dir}/rain_encoder.onnx",
        input_names=["audio"],
        output_names=["mu", "logvar"],
        dynamic_axes={"audio": {0: "batch"}},
        opset_version=17,
        do_constant_folding=True,
    )
```

### 3.4 GRU Prior Export -- CRITICAL COMPLEXITY

**Confidence: MEDIUM** -- GRU export works but requires careful handling for stateful inference.

**The problem:** ONNX Runtime is intentionally stateless. The GRU's hidden state must be managed externally between inference calls.

**Solution: Export GRU with explicit hidden state I/O:**

```python
class TexturePriorONNX(nn.Module):
    """Wrapper for ONNX-friendly prior model with explicit hidden state."""

    def __init__(self, prior: TexturePrior):
        super().__init__()
        self.gru = prior.gru
        self.fc_mu = prior.fc_mu
        self.fc_logvar = prior.fc_logvar

    def forward(self, z_input, hidden):
        """
        Args:
            z_input: (1, 1, 32) -- single latent vector as sequence of length 1
            hidden: (2, 1, 128) -- GRU hidden state (n_layers=2, batch=1, hidden=128)
        Returns:
            mu: (1, 32)
            logvar: (1, 32)
            hidden_out: (2, 1, 128) -- updated hidden state
        """
        output, hidden_out = self.gru(z_input, hidden)
        mu = self.fc_mu(output[:, -1, :])
        logvar = self.fc_logvar(output[:, -1, :])
        return mu, logvar, hidden_out


def export_prior_to_onnx(prior, output_dir):
    wrapper = TexturePriorONNX(prior)
    wrapper.eval()
    wrapper.cpu()

    z_dummy = torch.randn(1, 1, 32)
    h_dummy = torch.zeros(2, 1, 128)  # n_layers=2, batch=1, hidden=128

    torch.onnx.export(
        wrapper,
        (z_dummy, h_dummy),
        f"{output_dir}/rain_prior.onnx",
        input_names=["z_input", "hidden_in"],
        output_names=["mu", "logvar", "hidden_out"],
        opset_version=17,
        do_constant_folding=True,
        # No dynamic axes needed -- all shapes are fixed at inference
    )
```

**C++ inference pattern (ONNX Runtime):**
```cpp
// Initialize hidden state to zeros
std::vector<float> hidden(2 * 1 * 128, 0.0f);

// Each inference call:
// 1. Feed previous latent + current hidden state
// 2. Get predicted mu, logvar, and updated hidden state
// 3. Sample z_next = mu + exp(0.5*logvar) * epsilon * temperature
// 4. Use z_next as decoder input AND as next prior input
// 5. Save hidden_out as next hidden_in
```

**Important:** Do NOT export the prior with variable-length sequence input. Instead, export it as a single-step model (seq_len=1) with explicit hidden state. This is more efficient for real-time inference and avoids ONNX dynamic axis complications with GRU.

### 3.5 ONNX Validation

**Confidence: HIGH** -- Standard practice.

```python
import onnxruntime as ort
import numpy as np

def validate_onnx_export(pytorch_model, onnx_path, test_input, atol=1e-5):
    """Verify ONNX model produces same output as PyTorch model."""
    # PyTorch inference
    pytorch_model.eval()
    with torch.no_grad():
        pt_output = pytorch_model(test_input)

    # ONNX Runtime inference
    session = ort.InferenceSession(onnx_path)
    ort_inputs = {session.get_inputs()[0].name: test_input.numpy()}
    ort_output = session.run(None, ort_inputs)

    # Compare
    if isinstance(pt_output, tuple):
        for i, (pt, ort_out) in enumerate(zip(pt_output, ort_output)):
            diff = np.abs(pt.numpy() - ort_out).max()
            print(f"Output {i}: max diff = {diff:.8f}, pass = {diff < atol}")
    else:
        diff = np.abs(pt_output.numpy() - ort_output[0]).max()
        print(f"Max diff = {diff:.8f}, pass = {diff < atol}")
```

**Expected differences:** With `do_constant_folding=True` and float32, max differences should be < 1e-5. If differences exceed 1e-4, something is wrong with the export.

---

## 4. Prior Model Training Specifics

### 4.1 Training Data Extraction

**Confidence: HIGH** -- Straightforward approach, well-established pattern.

After training the VAE, extract consecutive latent vectors from the training audio:

```python
def extract_latent_sequences(vae_model, audio_files, block_size=4096,
                              sample_rate=48000, device='cuda'):
    """
    Extract sequences of latent vectors from audio files.
    Each audio file produces one continuous sequence of latent vectors.
    """
    vae_model.eval()
    sequences = []

    for filepath in audio_files:
        waveform, sr = torchaudio.load(filepath)
        if sr != sample_rate:
            waveform = torchaudio.transforms.Resample(sr, sample_rate)(waveform)
        if waveform.shape[0] > 1:
            waveform = waveform.mean(dim=0, keepdim=True)

        # Extract consecutive non-overlapping blocks
        n_blocks = waveform.shape[1] // block_size
        latents = []

        with torch.no_grad():
            for i in range(n_blocks):
                block = waveform[:, i * block_size : (i + 1) * block_size]
                block = block.unsqueeze(0).to(device)  # (1, 1, 4096)
                mu, _ = vae_model.encoder(block)
                latents.append(mu.cpu())

        if len(latents) > 0:
            sequence = torch.cat(latents, dim=0)  # (n_blocks, 32)
            sequences.append(sequence)
            print(f"{filepath}: {len(latents)} blocks ({len(latents) * block_size / sample_rate:.1f}s)")

    return sequences

# 30 min of audio at 48kHz / 4096 samples per block = ~21,000 latent vectors
# These form ~N continuous sequences (one per audio file)
```

### 4.2 Sequence Length for GRU Training

**Confidence: MEDIUM** -- Based on general RNN practice and the specific latent dimension.

| Parameter | Recommended | Range | Rationale |
|-----------|-------------|-------|-----------|
| Sequence length | 32 | 16-64 | Each latent = 85ms; 32 = 2.7 seconds of temporal context |
| Batch size | 32 | 16-64 | More sequences per batch = more stable GRU training |
| Training epochs | 100-200 | 50-300 | GRU is small (~100k params), converges fast |
| Learning rate | 1e-3 | 5e-4 to 3e-3 | Higher LR than VAE because model is simpler |

**Why seq_len=32:**
- At 48kHz with 4096-sample blocks, each block = 85.3ms
- 32 blocks = 2.73 seconds of temporal context
- This captures the characteristic temporal patterns of rain (e.g., intensity fluctuations, gust patterns)
- Too short (8-16): GRU cannot learn long-range temporal patterns
- Too long (64+): Diminishing returns, training slower, gradient issues

### 4.3 Teacher Forcing Strategy

**Confidence: HIGH** -- Teacher forcing with scheduled sampling is the standard approach.

**Use full teacher forcing** for the prior model. Reasons:
- The prior model is simple (2-layer GRU, ~100k params)
- The training data is the deterministic mu output from the VAE encoder (no sampling noise)
- Exposure bias is minimal because at inference, the prior's output is passed through the decoder which is robust to small latent perturbations
- The temperature parameter at generation time provides the diversity needed

```python
def train_prior(vae_model, audio_files, output_dir, device='cuda',
                epochs=200, seq_len=32, batch_size=32, lr=1e-3):
    """Train the GRU prior model on latent sequences."""

    # 1. Extract all latent sequences
    sequences = extract_latent_sequences(vae_model, audio_files, device=device)

    # 2. Create sliding window training examples
    #    Input: z[t:t+seq_len], Target: z[t+1:t+seq_len+1]
    train_inputs = []
    train_targets = []

    for seq in sequences:
        for start in range(len(seq) - seq_len):
            train_inputs.append(seq[start : start + seq_len])
            train_targets.append(seq[start + 1 : start + seq_len + 1])

    train_inputs = torch.stack(train_inputs)   # (N, seq_len, 32)
    train_targets = torch.stack(train_targets)  # (N, seq_len, 32)

    dataset = torch.utils.data.TensorDataset(train_inputs, train_targets)
    loader = torch.utils.data.DataLoader(dataset, batch_size=batch_size,
                                          shuffle=True, pin_memory=True)

    # 3. Train
    prior = TexturePrior(latent_dim=32, hidden_dim=128, n_layers=2).to(device)
    optimizer = torch.optim.Adam(prior.parameters(), lr=lr)
    scheduler = CosineAnnealingLR(optimizer, T_max=epochs, eta_min=1e-5)

    for epoch in range(epochs):
        total_loss = 0
        n_batches = 0

        for inputs, targets in loader:
            inputs = inputs.to(device)   # (B, seq_len, 32)
            targets = targets.to(device) # (B, seq_len, 32)

            # Teacher forcing: use ground truth as input at each step
            output, _ = prior.gru(inputs)            # (B, seq_len, 128)
            pred_mu = prior.fc_mu(output)             # (B, seq_len, 32)

            # MSE loss on predicted means vs actual next latent
            loss = F.mse_loss(pred_mu, targets)

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(prior.parameters(), max_norm=1.0)
            optimizer.step()

            total_loss += loss.item()
            n_batches += 1

        scheduler.step()

        if epoch % 20 == 0:
            avg_loss = total_loss / n_batches
            print(f"Prior epoch {epoch}/{epochs}: loss={avg_loss:.6f}")

    torch.save(prior.state_dict(), f"{output_dir}/prior_final.pt")
    return prior
```

**Key difference from architecture spec:** The architecture spec shows training with sequential forward passes through the GRU for each timestep. The implementation above is more efficient: pass the entire sequence through the GRU at once (teacher forcing), then compute loss on all timestep predictions simultaneously. This is ~32x faster than the sequential approach.

### 4.4 Temperature Parameter

**Confidence: HIGH** -- Standard sampling technique.

```python
def generate_with_temperature(prior, decoder, n_blocks, temperature=0.8,
                               device='cuda'):
    """Generate audio using prior model with temperature-controlled sampling."""
    prior.eval()
    decoder.eval()

    # Initialize with random latent
    z = torch.randn(1, 1, 32, device=device)
    hidden = torch.zeros(2, 1, 128, device=device)

    audio_blocks = []

    with torch.no_grad():
        for _ in range(n_blocks):
            # Prior predicts next latent
            output, hidden = prior.gru(z, hidden)
            mu = prior.fc_mu(output[:, -1:, :])
            logvar = prior.fc_logvar(output[:, -1:, :])

            # Temperature-scaled sampling
            std = torch.exp(0.5 * logvar) * temperature
            eps = torch.randn_like(std)
            z_next = mu + eps * std

            # Decode to audio
            audio = decoder(z_next.squeeze(1))  # (1, 1, 4096)
            audio_blocks.append(audio.cpu())

            # Next input
            z = z_next.unsqueeze(1)  # (1, 1, 32)

    return audio_blocks
```

**Temperature guidelines:**
| Temperature | Effect | Use Case |
|-------------|--------|----------|
| 0.0 | Deterministic (use mu only) | Testing, debugging |
| 0.3-0.5 | Very stable, minimal variation | Calm, steady rain |
| 0.7-0.9 | Moderate variation | Natural-sounding rain (default) |
| 1.0 | Full variance | More dynamic rain |
| 1.2-1.5 | High variation | Experimental, may produce artifacts |

**Recommendation:** Default temperature = 0.8. Map Evolve parameter (0.0-1.0) to temperature range (0.3-1.2).

---

## 5. Quality Validation

### 5.1 Quantitative Metrics

**Confidence: HIGH** -- Standard audio quality metrics.

#### Multi-Scale Spectral Convergence (Primary Metric)

```python
def spectral_convergence(x, x_hat, fft_sizes=[2048, 1024, 512, 256]):
    """
    Compute spectral convergence across multiple FFT scales.
    Lower is better. Target: < 0.3 for good quality.
    """
    total_sc = 0
    for fft_size in fft_sizes:
        hop = fft_size // 4
        window = torch.hann_window(fft_size, device=x.device)

        X = torch.stft(x.squeeze(1), fft_size, hop, window=window, return_complex=True)
        X_hat = torch.stft(x_hat.squeeze(1), fft_size, hop, window=window, return_complex=True)

        X_mag = torch.abs(X)
        X_hat_mag = torch.abs(X_hat)

        sc = torch.norm(X_mag - X_hat_mag, p='fro') / (torch.norm(X_mag, p='fro') + 1e-7)
        total_sc += sc.item()

    return total_sc / len(fft_sizes)
```

#### Log Spectral Distance (LSD)

```python
def log_spectral_distance(x, x_hat, fft_size=2048, hop=512):
    """
    Average L1 distance between log-magnitude spectra.
    Lower is better. Target: < 1.0 dB for good quality.
    """
    window = torch.hann_window(fft_size, device=x.device)
    X = torch.stft(x.squeeze(1), fft_size, hop, window=window, return_complex=True)
    X_hat = torch.stft(x_hat.squeeze(1), fft_size, hop, window=window, return_complex=True)

    lsd = F.l1_loss(
        torch.log(torch.abs(X) + 1e-7),
        torch.log(torch.abs(X_hat) + 1e-7)
    )
    return lsd.item()
```

#### Quality Thresholds

| Metric | Poor | Acceptable | Good | Excellent |
|--------|------|-----------|------|-----------|
| Spectral Convergence | > 0.5 | 0.3-0.5 | 0.15-0.3 | < 0.15 |
| Log Spectral Distance | > 2.0 | 1.0-2.0 | 0.5-1.0 | < 0.5 |
| Waveform L1 | > 0.3 | 0.15-0.3 | 0.05-0.15 | < 0.05 |

**Note:** For texture synthesis (not speech), waveform L1 is less important than spectral metrics. Two different rain sounds can have very different waveforms but nearly identical spectral characteristics.

### 5.2 Reconstruction Test

```python
def reconstruction_test(model, dataset, device='cuda', n_samples=100):
    """Test VAE reconstruction quality on held-out samples."""
    model.eval()
    metrics = {'sc': [], 'lsd': [], 'l1': []}

    with torch.no_grad():
        for i in range(n_samples):
            x = dataset[i].unsqueeze(0).to(device)  # (1, 1, 4096)
            recon, mu, logvar = model(x)

            metrics['sc'].append(spectral_convergence(x, recon))
            metrics['lsd'].append(log_spectral_distance(x, recon))
            metrics['l1'].append(F.l1_loss(x, recon).item())

    for key, values in metrics.items():
        mean_val = np.mean(values)
        std_val = np.std(values)
        print(f"{key}: {mean_val:.4f} +/- {std_val:.4f}")

    return metrics
```

### 5.3 Generation Test (Prior Quality)

```python
def generation_test(prior, decoder, n_blocks=200, temperature=0.8,
                    sample_rate=48000, output_path="generated_rain.wav"):
    """Generate audio from the prior and save for listening evaluation."""
    blocks = generate_with_temperature(prior, decoder, n_blocks, temperature)

    # Simple concatenation (overlap-add not needed for evaluation)
    audio = torch.cat([b.squeeze() for b in blocks], dim=0)

    # Check for NaN/Inf
    assert not torch.isnan(audio).any(), "Generated audio contains NaN!"
    assert not torch.isinf(audio).any(), "Generated audio contains Inf!"

    # Check amplitude range
    peak = audio.abs().max().item()
    print(f"Generated {len(audio)/sample_rate:.1f}s, peak={peak:.4f}")
    assert peak < 2.0, f"Peak amplitude too high: {peak}"

    # Save
    torchaudio.save(output_path, audio.unsqueeze(0), sample_rate)
    print(f"Saved to {output_path}")
```

### 5.4 Latent Space Visualization

**Confidence: HIGH** -- Standard VAE evaluation technique.

```python
from sklearn.manifold import TSNE
import matplotlib.pyplot as plt

def visualize_latent_space(model, dataset, device='cuda', n_samples=1000):
    """Visualize the latent space structure using t-SNE."""
    model.eval()
    latents = []

    with torch.no_grad():
        for i in range(min(n_samples, len(dataset))):
            x = dataset[i].unsqueeze(0).to(device)
            mu, logvar = model.encoder(x)
            latents.append(mu.cpu().numpy())

    latents = np.concatenate(latents, axis=0)  # (N, 32)

    # t-SNE reduction to 2D
    tsne = TSNE(n_components=2, perplexity=30, random_state=42)
    latents_2d = tsne.fit_transform(latents)

    plt.figure(figsize=(10, 8))
    plt.scatter(latents_2d[:, 0], latents_2d[:, 1], alpha=0.5, s=10)
    plt.title("Latent Space (t-SNE)")
    plt.xlabel("t-SNE 1")
    plt.ylabel("t-SNE 2")
    plt.savefig("latent_space_tsne.png", dpi=150)
    plt.close()

    # Also check per-dimension statistics
    dim_variance = np.var(latents, axis=0)
    active_dims = np.sum(dim_variance > 0.1)
    print(f"Active latent dimensions (var > 0.1): {active_dims}/32")

    return latents
```

**What to look for in t-SNE visualization:**
- **Good:** Smooth, continuous distribution (textures are similar, should be blob-like)
- **Bad:** Disconnected clusters (suggests mode collapse or insufficient data diversity)
- **Bad:** All points at center (suggests posterior collapse -- KL is too high)

### 5.5 Interpolation Test

```python
def interpolation_test(model, dataset, device='cuda', n_steps=10,
                       output_dir="interpolation_test"):
    """Test latent interpolation smoothness."""
    model.eval()
    os.makedirs(output_dir, exist_ok=True)

    # Pick two random samples
    x1 = dataset[0].unsqueeze(0).to(device)
    x2 = dataset[len(dataset) // 2].unsqueeze(0).to(device)

    with torch.no_grad():
        mu1, _ = model.encoder(x1)
        mu2, _ = model.encoder(x2)

        audios = []
        for i in range(n_steps + 1):
            alpha = i / n_steps
            z = (1 - alpha) * mu1 + alpha * mu2
            audio = model.decoder(z)
            audios.append(audio.cpu().squeeze())

        # Concatenate and save
        full_audio = torch.cat(audios, dim=0)
        torchaudio.save(f"{output_dir}/interpolation.wav",
                       full_audio.unsqueeze(0), 48000)
```

**What to listen for:**
- **Good:** Smooth, gradual transition between two rain characteristics
- **Bad:** Abrupt changes, clicks, or silence in between
- **Bad:** All interpolation points sound identical (underfitting)

### 5.6 Common Failure Modes and Detection

| Failure Mode | Symptoms | Detection | Fix |
|-------------|----------|-----------|-----|
| **Posterior collapse** | All generated samples sound identical; KL loss near 0 | Monitor KL loss; if < 0.01 after warmup, collapsed | Use cyclical annealing; reduce beta max; check decoder capacity |
| **Mode collapse** | VAE only produces 1-2 types of rain, ignores variety | t-SNE shows tight clusters; listen to random generations | More diverse training data; lower KL weight |
| **Blurry/muffled output** | Reconstructions lack high frequencies | Compare spectrograms of input vs output; spectral rolloff | Increase spectral loss weight at small FFT sizes (256, 512) |
| **Checkerboard artifacts** | Periodic tonal artifacts in audio | Listen for periodic buzzing; check spectrogram for horizontal lines | Verify ConvTranspose1d kernel/stride alignment (kernel divisible by stride) |
| **Exploding KL** | KL loss grows unbounded; training becomes unstable | Monitor KL loss per epoch | Lower beta; add gradient clipping; check for NaN in logvar |
| **Prior drift** | Generated sequences gradually deviate from rain-like textures | Listen to long generations (>30s); compute running spectral metrics | Lower temperature; train prior longer; increase hidden dim |

---

## Architecture Patterns

### Recommended Project Structure

```
plugins/O-Texture/
  training/
    config.py              # Hyperparameters, paths, constants
    models.py              # TextureVAE, TextureEncoder, TextureDecoder, TexturePrior
    losses.py              # TextureVAELoss, spectral_convergence, etc.
    dataset.py             # TextureDataset, preprocessing utilities
    train_vae.py           # Main VAE training script
    train_prior.py         # Prior model training script (runs after VAE)
    export_onnx.py         # ONNX export with validation
    evaluate.py            # Quality metrics, visualization, interpolation tests
    analyze_latent.py      # Latent space analysis, dimension mapping
    download_rain.py       # Freesound API download script
  training/data/
    raw/                   # Downloaded audio files (gitignored)
    processed/             # Preprocessed 48kHz mono WAVs (gitignored)
  training/checkpoints/    # Model checkpoints (gitignored)
  training/output/         # ONNX models, evaluation results (gitignored)
  training/requirements.txt # Python dependencies
  Resources/models/rain/   # Final ONNX models (committed)
    rain_decoder.onnx
    rain_encoder.onnx
    rain_prior.onnx
    dim_map_rain.json      # Latent dimension mapping
```

### Training Flow Pattern

```
1. download_rain.py     -- Download from Freesound, manual curation
2. dataset.py           -- Preprocess: resample, mono, normalize, remove silence
3. train_vae.py         -- Train VAE (150k-200k steps, ~3-10 hours)
4. evaluate.py          -- Reconstruction quality, spectrograms, listening
5. analyze_latent.py    -- t-SNE, dimension activity, create dim_map_rain.json
6. train_prior.py       -- Train GRU prior on latent sequences (~1-2 hours)
7. evaluate.py          -- Generation quality, long-form listening test
8. export_onnx.py       -- Export all 3 models to ONNX, validate
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Audio loading/resampling | Custom WAV parser | `torchaudio.load()` + `torchaudio.transforms.Resample` | Handles all formats, GPU-accelerated resampling |
| STFT computation | Manual DFT | `torch.stft()` | GPU-accelerated, numerically stable |
| Cyclical annealing | Custom scheduler | `frange_cycle_linear()` function above (from official repo) | Battle-tested implementation |
| t-SNE/UMAP visualization | Custom dimensionality reduction | `sklearn.manifold.TSNE` or `umap-learn` | Standard, optimized implementations |
| ONNX validation | Manual comparison | `onnxruntime.InferenceSession` + numpy comparison | Official tooling |
| Mixed precision training | Manual casting | `torch.amp.autocast('cuda')` | Automatic, handles edge cases |

---

## Common Pitfalls

### Pitfall 1: ConvTranspose1d Checkerboard Artifacts
**What goes wrong:** Periodic tonal artifacts in generated audio due to uneven overlap in transposed convolution.
**Why it happens:** When kernel_size is not divisible by stride, some output positions receive contributions from more input positions than others.
**How to avoid:** The architecture uses kernel_size=8, stride=4 -- **this is safe** because 8 is divisible by 4. Verify by listening to decoder-only output (random z -> decode) early in training.
**Warning signs:** Periodic buzzing or metallic tone in outputs; horizontal lines in spectrogram.

### Pitfall 2: KL Weight Too High for Audio
**What goes wrong:** Posterior collapse -- all latent vectors collapse to N(0,1), decoder ignores z entirely.
**Why it happens:** Text VAEs use beta=1.0, but audio spectral loss is on a completely different scale. Using beta=1.0 overwhelms the reconstruction loss.
**How to avoid:** Use beta_max = 0.001 (not 1.0). Use cyclical annealing. Monitor KL loss -- it should be between 1.0 and 50.0 after warmup.
**Warning signs:** KL < 0.1; all generated samples sound identical; latent interpolation produces no change.

### Pitfall 3: Not Calling model.eval() Before ONNX Export
**What goes wrong:** BatchNorm uses batch statistics instead of running statistics, causing inference/training mismatch.
**Why it happens:** BatchNorm has different behavior in train vs eval mode.
**How to avoid:** Always call `model.eval()` before `torch.onnx.export()`. Add assertion.
**Warning signs:** ONNX model produces different output than PyTorch model; output quality degrades with batch size 1.

### Pitfall 4: Prior Model Input Mismatch
**What goes wrong:** Prior model trained on full sequences but exported for single-step inference.
**Why it happens:** Training uses teacher forcing with seq_len=32, but real-time inference processes one latent at a time.
**How to avoid:** Export the prior as a single-step model with explicit hidden state I/O (see Section 3.4). Test that single-step autoregressive generation matches multi-step teacher-forced generation.
**Warning signs:** Prior output diverges rapidly; generated audio sounds nothing like rain after a few seconds.

### Pitfall 5: Inadequate Data Preprocessing
**What goes wrong:** Model learns recording artifacts (noise floor, clipping, codec artifacts) instead of rain texture.
**Why it happens:** Training on unfiltered downloads from Freesound without quality checking.
**How to avoid:** Listen to every file before training. Remove files with audible noise floor, clipping, compression artifacts, or non-rain sounds. Normalize consistently.
**Warning signs:** Generated audio has consistent hiss/buzz undertone; model produces artifacts on some random seeds.

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| PyTorch | 2.x (latest stable) | Model definition, training loop | Industry standard for ML research |
| torchaudio | matching PyTorch | Audio I/O, resampling, transforms | Official PyTorch audio library |
| ONNX | 1.16.x | Model export format | Standard ML interop format |
| onnxruntime | 1.19.2+ | ONNX model validation | Must match ANIRA's bundled version |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| numpy | latest | Array operations, beta schedule | Always |
| matplotlib | latest | Training curves, spectrograms | Evaluation phase |
| scikit-learn | latest | t-SNE for latent visualization | Evaluation phase |
| freesound-python | latest | Freesound API client | Dataset download |
| tensorboard | latest | Training monitoring (optional) | If wanted for real-time loss plots |
| soundfile | latest | Additional audio I/O format support | If torchaudio has format issues |

### Installation

```bash
pip install torch torchaudio --index-url https://download.pytorch.org/whl/cu121
pip install onnx onnxruntime-gpu numpy matplotlib scikit-learn
pip install freesound-python soundfile tensorboard
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Linear KL warmup | Cyclical annealing | 2019 (Fu et al.) | Prevents posterior collapse reliably |
| Adam optimizer | AdamW | 2019 (Loshchilov & Hutter) | Better generalization via decoupled weight decay |
| torch.onnx.export (legacy) | torch.onnx.export (TorchScript) | PyTorch 2.x | More reliable export, better op coverage |
| Manual mixed precision | torch.amp.autocast | PyTorch 1.6+ | Automatic, ~2x training speedup |
| RAVE v1 (BatchNorm) | RAVE v2 (causal, reduced latency) | 2024-2025 | Reduced latency, but more complex; our model follows v1 pattern |

---

## Open Questions

1. **Exact training duration on user's GPU**
   - What we know: ~1.2M params, batch 32, estimated 50-100ms/step
   - What's unclear: Actual GPU model, available VRAM, whether GPU is shared
   - Recommendation: Run 1000-step benchmark, extrapolate to full training

2. **Optimal KL weight ceiling (beta_max)**
   - What we know: 0.001 is the architecture spec value; should be in [0.0001, 0.01] range
   - What's unclear: Exact value depends on relative magnitudes of spectral loss and KL for this specific model
   - Recommendation: Start with 0.001, monitor KL vs spectral loss ratio during training, adjust if KL < 0.1 or > 100

3. **Rain dataset quality threshold**
   - What we know: Need 30 min diverse rain, 48kHz preferred
   - What's unclear: How much quality variation is acceptable; exact number of files needed
   - Recommendation: Download 60+ minutes, curate down to best 30-45 minutes after listening

4. **Prior model hidden dimension**
   - What we know: Architecture spec says 128
   - What's unclear: Whether 128 is sufficient for capturing rain temporal dynamics
   - Recommendation: Start with 128; if prior loss plateaus high, try 256

---

## Sources

### Primary (HIGH confidence)
- ONNX Runtime compatibility table: https://onnxruntime.ai/docs/reference/compatibility.html -- opset 21 support for ORT 1.19.2
- PyTorch ONNX export docs: https://docs.pytorch.org/docs/stable/onnx.html
- Cyclical annealing paper: https://arxiv.org/abs/1903.10145 (Fu et al., 2019)
- Cyclical annealing code: https://github.com/haofuml/cyclical_annealing
- ONNX Runtime GRU stateful inference: https://github.com/microsoft/onnxruntime/issues/11085
- Checkerboard artifacts reference: https://distill.pub/2016/deconv-checkerboard/
- RAVE repository: https://github.com/acids-ircam/RAVE

### Secondary (MEDIUM confidence)
- Freesound API documentation: https://freesound.org/docs/api/
- Freesound Python client: https://github.com/MTG/freesound-python
- ESC-50 dataset: https://github.com/karolpiczak/ESC-50
- PyTorch CosineAnnealingLR docs: https://docs.pytorch.org/docs/stable/generated/torch.optim.lr_scheduler.CosineAnnealingLR.html
- PyTorch AdamW docs: https://docs.pytorch.org/docs/stable/generated/torch.optim.AdamW.html
- BatchNorm1d docs: https://docs.pytorch.org/docs/stable/generated/torch.nn.BatchNorm1d.html
- Teacher forcing reference: https://machinelearningmastery.com/teacher-forcing-for-recurrent-neural-networks/

### Tertiary (LOW confidence)
- Training duration estimates: Extrapolated from general PyTorch benchmarks, not benchmarked on this specific model
- Spectral convergence thresholds: Based on general audio quality literature, not validated for 1D CNN VAE textures specifically

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- PyTorch + torchaudio + ONNX is the established pipeline
- Architecture patterns: HIGH -- Training flow matches RAVE and similar audio VAE papers
- KL annealing: HIGH -- Cyclical annealing is well-established with reference implementation
- ONNX export: HIGH -- Opset compatibility verified, GRU stateful pattern documented
- Dataset curation: MEDIUM -- Freesound sources identified but actual quality not verified
- Training duration: MEDIUM -- Estimated from model size, not benchmarked
- Quality thresholds: MEDIUM -- Based on literature, not validated for this specific model

**Research date:** 2026-02-14
**Valid until:** 2026-04-14 (60 days -- stable domain, PyTorch/ONNX APIs well-established)
