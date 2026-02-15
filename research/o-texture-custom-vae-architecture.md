# O-Texture: Custom 1D CNN VAE Architecture Design

## Design Goals

1. Generate infinite evolving audio textures from learned distributions
2. Lightweight enough for real-time CPU inference in a JUCE plugin
3. Controllable latent space for user navigation (XY pad, knobs)
4. No external dependencies on RAVE/IRCAM tooling
5. Simple training pipeline (PyTorch only)
6. Deployable via ONNX Runtime + ANIRA or RTNeural

---

## 1. Model Architecture

### 1.1 Overview

```
ENCODER (Analysis)                    DECODER (Synthesis)
==================                    ==================

Input: 4096 samples (mono, 48kHz)     Input: z (32-dim latent vector)
       = 85.3ms of audio                     + conditioning (optional)
       |                                     |
       v                                     v
[Conv1D 1x16, k=7, s=2] + LReLU     [Dense 32 -> 256*16]
       |  2048 samples                       |
       v                                     v
[Conv1D 16x32, k=7, s=2] + LReLU    [Reshape to 256 x 16]
       |  1024 samples                       |
       v                                     v
[Conv1D 32x64, k=7, s=2] + LReLU    [ConvT1D 256x128, k=8, s=4] + LReLU
       |  512 samples                        |  64 samples
       v                                     v
[Conv1D 64x128, k=7, s=2] + LReLU   [ConvT1D 128x64, k=8, s=4] + LReLU
       |  256 samples                        |  256 samples
       v                                     v
[Conv1D 128x256, k=7, s=2] + LReLU  [ConvT1D 64x32, k=8, s=4] + LReLU
       |  128 samples                        |  1024 samples
       v                                     v
[Conv1D 256x256, k=3, s=2] + LReLU  [ConvT1D 32x16, k=8, s=4] + LReLU
       |  64 samples                         |  4096 samples
       v                                     v
[AdaptiveAvgPool -> 16]              [Conv1D 16x1, k=7, s=1] + Tanh
       |                                     |
       v                                     v
[Dense 256*16 -> 64]                 Output: 4096 samples (mono)
       |
   +---+---+
   |       |
   v       v
 mu(32)  logvar(32)
   |       |
   +---+---+
       |
       v
   z = mu + sigma * epsilon  (reparameterization trick)
```

### 1.2 Architecture Rationale

**Block size 4096 samples (85.3ms at 48kHz)**:
- Large enough to capture texture statistics (frequency content, amplitude envelope)
- Small enough for reasonable latency when host-compensated
- Powers of 2 for clean downsampling

**32-dimensional latent space**:
- Much smaller than RAVE's 128 dimensions
- Textures are lower-complexity than general audio -- 32 dims is sufficient
- Easier to navigate and control (can map all dims to a few macro controls)
- Smaller latent = faster prior model training

**~1.2M parameters total** (estimated):
- Encoder: ~600k
- Decoder: ~600k
- Compare: RAVE standard = 9M, RAVE v2_small = ~3M, DDSP = 160k

**No PQMF**:
- RAVE uses 16-band filterbank for efficiency
- Our model is small enough to operate on raw waveform directly
- Simpler architecture, fewer failure modes

### 1.3 PyTorch Implementation

```python
import torch
import torch.nn as nn
import torch.nn.functional as F


class TextureEncoder(nn.Module):
    """Encodes 4096 audio samples to 32-dim latent vector."""

    def __init__(self, latent_dim=32):
        super().__init__()

        self.conv_stack = nn.Sequential(
            # 4096 -> 2048
            nn.Conv1d(1, 16, kernel_size=7, stride=2, padding=3),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(16),

            # 2048 -> 1024
            nn.Conv1d(16, 32, kernel_size=7, stride=2, padding=3),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(32),

            # 1024 -> 512
            nn.Conv1d(32, 64, kernel_size=7, stride=2, padding=3),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(64),

            # 512 -> 256
            nn.Conv1d(64, 128, kernel_size=7, stride=2, padding=3),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(128),

            # 256 -> 128
            nn.Conv1d(128, 256, kernel_size=7, stride=2, padding=3),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(256),

            # 128 -> 64
            nn.Conv1d(256, 256, kernel_size=3, stride=2, padding=1),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(256),
        )

        # Adaptive pooling to fixed size regardless of input length
        self.pool = nn.AdaptiveAvgPool1d(16)

        # Project to latent space
        self.fc_mu = nn.Linear(256 * 16, latent_dim)
        self.fc_logvar = nn.Linear(256 * 16, latent_dim)

    def forward(self, x):
        # x: (batch, 1, 4096)
        h = self.conv_stack(x)       # (batch, 256, 64)
        h = self.pool(h)             # (batch, 256, 16)
        h = h.flatten(1)             # (batch, 4096)
        mu = self.fc_mu(h)           # (batch, 32)
        logvar = self.fc_logvar(h)   # (batch, 32)
        return mu, logvar


class TextureDecoder(nn.Module):
    """Decodes 32-dim latent vector to 4096 audio samples."""

    def __init__(self, latent_dim=32):
        super().__init__()

        # Project latent to initial feature map
        self.fc = nn.Linear(latent_dim, 256 * 16)

        self.deconv_stack = nn.Sequential(
            # 16 -> 64
            nn.ConvTranspose1d(256, 128, kernel_size=8, stride=4, padding=2),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(128),

            # 64 -> 256
            nn.ConvTranspose1d(128, 64, kernel_size=8, stride=4, padding=2),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(64),

            # 256 -> 1024
            nn.ConvTranspose1d(64, 32, kernel_size=8, stride=4, padding=2),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(32),

            # 1024 -> 4096
            nn.ConvTranspose1d(32, 16, kernel_size=8, stride=4, padding=2),
            nn.LeakyReLU(0.2),
            nn.BatchNorm1d(16),
        )

        # Final projection to mono audio
        self.out_conv = nn.Conv1d(16, 1, kernel_size=7, stride=1, padding=3)

    def forward(self, z):
        # z: (batch, 32)
        h = self.fc(z)                        # (batch, 4096)
        h = h.view(-1, 256, 16)               # (batch, 256, 16)
        h = self.deconv_stack(h)               # (batch, 16, 4096)
        h = torch.tanh(self.out_conv(h))       # (batch, 1, 4096)
        return h


class TextureVAE(nn.Module):
    """Complete VAE for audio texture synthesis."""

    def __init__(self, latent_dim=32):
        super().__init__()
        self.latent_dim = latent_dim
        self.encoder = TextureEncoder(latent_dim)
        self.decoder = TextureDecoder(latent_dim)

    def reparameterize(self, mu, logvar):
        std = torch.exp(0.5 * logvar)
        eps = torch.randn_like(std)
        return mu + eps * std

    def forward(self, x):
        mu, logvar = self.encoder(x)
        z = self.reparameterize(mu, logvar)
        recon = self.decoder(z)
        return recon, mu, logvar

    def encode(self, x):
        mu, logvar = self.encoder(x)
        return mu  # Return mean for deterministic encoding

    def decode(self, z):
        return self.decoder(z)

    def generate(self, batch_size=1, device='cpu'):
        """Sample from prior (standard normal) and decode."""
        z = torch.randn(batch_size, self.latent_dim, device=device)
        return self.decode(z)
```

---

## 2. Training Pipeline

### 2.1 Loss Function

Standard VAE loss with multi-scale spectral reconstruction:

```python
class TextureVAELoss(nn.Module):
    """
    Combined loss:
    1. Multi-scale spectral loss (perceptual quality)
    2. Waveform L1 loss (temporal alignment)
    3. KL divergence (latent regularization)
    """

    def __init__(self, fft_sizes=[2048, 1024, 512, 256],
                 kl_weight=0.001):
        super().__init__()
        self.fft_sizes = fft_sizes
        self.kl_weight = kl_weight

    def spectral_loss(self, x, x_hat):
        """Multi-scale spectral convergence + log magnitude loss."""
        total = 0.0
        for fft_size in self.fft_sizes:
            hop = fft_size // 4
            window = torch.hann_window(fft_size, device=x.device)

            X = torch.stft(x.squeeze(1), fft_size, hop, window=window,
                          return_complex=True)
            X_hat = torch.stft(x_hat.squeeze(1), fft_size, hop, window=window,
                              return_complex=True)

            X_mag = torch.abs(X)
            X_hat_mag = torch.abs(X_hat)

            # Spectral convergence
            sc = torch.norm(X_mag - X_hat_mag, p='fro') / (torch.norm(X_mag, p='fro') + 1e-7)

            # Log magnitude loss
            lm = F.l1_loss(torch.log(X_mag + 1e-7), torch.log(X_hat_mag + 1e-7))

            total += sc + lm

        return total / len(self.fft_sizes)

    def forward(self, x, x_hat, mu, logvar):
        # Reconstruction losses
        spec_loss = self.spectral_loss(x, x_hat)
        wave_loss = F.l1_loss(x_hat, x)

        # KL divergence
        kl_loss = -0.5 * torch.mean(1 + logvar - mu.pow(2) - logvar.exp())

        # Weighted combination
        total = spec_loss + 0.1 * wave_loss + self.kl_weight * kl_loss

        return total, {
            'spectral': spec_loss.item(),
            'waveform': wave_loss.item(),
            'kl': kl_loss.item(),
            'total': total.item(),
        }
```

### 2.2 Optional: Adversarial Fine-Tuning (Phase 2)

After initial VAE training, add a multi-scale discriminator for perceptual quality:

```python
class MultiScaleDiscriminator(nn.Module):
    """Discriminator operating at multiple temporal scales."""

    def __init__(self):
        super().__init__()
        self.discriminators = nn.ModuleList([
            self._make_disc(1),    # Full resolution
            self._make_disc(2),    # 2x downsampled
            self._make_disc(4),    # 4x downsampled
        ])

    def _make_disc(self, downsample_factor):
        return nn.Sequential(
            nn.AvgPool1d(downsample_factor) if downsample_factor > 1 else nn.Identity(),
            nn.Conv1d(1, 16, kernel_size=15, stride=2, padding=7),
            nn.LeakyReLU(0.2),
            nn.Conv1d(16, 32, kernel_size=15, stride=2, padding=7),
            nn.LeakyReLU(0.2),
            nn.Conv1d(32, 64, kernel_size=15, stride=2, padding=7),
            nn.LeakyReLU(0.2),
            nn.Conv1d(64, 1, kernel_size=3, stride=1, padding=1),
        )

    def forward(self, x):
        outputs = []
        for disc in self.discriminators:
            outputs.append(disc(x))
        return outputs
```

### 2.3 Training Schedule

```
Phase 1: VAE Reconstruction (Required)
=======================================
- Loss: Multi-scale spectral + waveform L1 + KL divergence
- Optimizer: Adam, lr=1e-4
- Batch size: 32
- Steps: 100k-200k
- Duration: 6-12 hours on RTX 3080/4090
- KL weight warmup: 0 -> 0.001 over first 10k steps
- Data: 4096-sample random crops from training audio

Phase 2: Adversarial Fine-Tuning (Optional, improves quality)
=============================================================
- Freeze encoder
- Add multi-scale discriminator
- Loss: Original VAE loss + adversarial loss (hinge)
- Optimizer: Adam, lr=1e-5 (generator), lr=1e-4 (discriminator)
- Steps: 50k-100k
- Duration: 3-6 hours
- Alternating generator/discriminator updates

Phase 3: Prior Model Training (Required for generative mode)
============================================================
- Separate model -- see Section 4
- Duration: 2-4 hours
```

### 2.4 Dataset Preparation

```python
import torchaudio
import os
from torch.utils.data import Dataset


class TextureDataset(Dataset):
    """
    Loads audio files, extracts random 4096-sample crops.
    All audio resampled to 48kHz mono.
    """

    BLOCK_SIZE = 4096
    SAMPLE_RATE = 48000

    def __init__(self, audio_dir, crops_per_file=100):
        self.crops_per_file = crops_per_file
        self.files = []

        for f in os.listdir(audio_dir):
            if f.endswith(('.wav', '.flac', '.mp3', '.ogg', '.aiff')):
                self.files.append(os.path.join(audio_dir, f))

        # Pre-load and resample all audio
        self.audio_chunks = []
        for filepath in self.files:
            waveform, sr = torchaudio.load(filepath)

            # Convert to mono
            if waveform.shape[0] > 1:
                waveform = waveform.mean(dim=0, keepdim=True)

            # Resample to 48kHz
            if sr != self.SAMPLE_RATE:
                resampler = torchaudio.transforms.Resample(sr, self.SAMPLE_RATE)
                waveform = resampler(waveform)

            # Normalize to [-1, 1]
            waveform = waveform / (waveform.abs().max() + 1e-7)

            self.audio_chunks.append(waveform)

    def __len__(self):
        return len(self.audio_chunks) * self.crops_per_file

    def __getitem__(self, idx):
        chunk_idx = idx // self.crops_per_file
        audio = self.audio_chunks[chunk_idx]

        # Random crop of BLOCK_SIZE samples
        max_start = audio.shape[1] - self.BLOCK_SIZE
        if max_start <= 0:
            # Pad if audio is too short
            audio = F.pad(audio, (0, self.BLOCK_SIZE - audio.shape[1]))
            start = 0
        else:
            start = torch.randint(0, max_start, (1,)).item()

        crop = audio[:, start:start + self.BLOCK_SIZE]
        return crop  # (1, 4096)
```

### 2.5 Training Script Outline

```python
def train_texture_vae(data_dir, output_dir, epochs=200, batch_size=32,
                      latent_dim=32, lr=1e-4, device='cuda'):

    dataset = TextureDataset(data_dir, crops_per_file=200)
    loader = DataLoader(dataset, batch_size=batch_size, shuffle=True,
                        num_workers=4, pin_memory=True)

    model = TextureVAE(latent_dim=latent_dim).to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=lr)
    criterion = TextureVAELoss(kl_weight=0.0)  # Start with no KL

    for epoch in range(epochs):
        # KL warmup over first 20 epochs
        kl_weight = min(0.001, epoch * 0.001 / 20)
        criterion.kl_weight = kl_weight

        for batch in loader:
            batch = batch.to(device)
            recon, mu, logvar = model(batch)
            loss, metrics = criterion(batch, recon, mu, logvar)

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()

        if epoch % 10 == 0:
            print(f"Epoch {epoch}: {metrics}")
            # Save checkpoint
            torch.save(model.state_dict(), f"{output_dir}/vae_epoch_{epoch}.pt")

    # Final save
    torch.save(model.state_dict(), f"{output_dir}/vae_final.pt")
    return model
```

### 2.6 Training Data Requirements

| Texture Category | Recommended Sources | Min Duration | Notes |
|---|---|---|---|
| Rain | Field recordings, freesound.org | 30 min | Various intensities |
| Metal | Industrial recordings, foley | 30 min | Scrapes, drones, resonances |
| Wind | Field recordings | 30 min | Various speeds, surfaces |
| Crowd | Ambience recordings | 30 min | Indoor/outdoor, various sizes |
| Synth | Synthesizer pads, drones | 30 min | Analog and digital textures |
| Organic | Nature recordings | 30 min | Fire, water, forest, insects |

**Total**: ~3 hours of curated audio across 6 categories.
**Train one model per category** (6 models total).

---

## 3. Latent Space Design

### 3.1 Post-Training Analysis

After training, analyze the latent space to identify meaningful dimensions:

```python
def analyze_latent_space(model, dataset, device='cuda', n_samples=1000):
    """
    Analyze which latent dimensions carry information
    vs which are noise (close to standard normal prior).
    """
    model.eval()
    all_mu = []
    all_logvar = []

    with torch.no_grad():
        for i in range(min(n_samples, len(dataset))):
            x = dataset[i].unsqueeze(0).to(device)
            mu, logvar = model.encoder(x)
            all_mu.append(mu.cpu())
            all_logvar.append(logvar.cpu())

    all_mu = torch.cat(all_mu, dim=0)      # (N, 32)
    all_logvar = torch.cat(all_logvar, dim=0)

    # Variance of means across dataset = how much each dim is used
    dim_variance = all_mu.var(dim=0)        # (32,)

    # Mean of logvar = average uncertainty per dim
    dim_uncertainty = all_logvar.mean(dim=0).exp()  # (32,)

    # Active dimensions: high variance of mu, low uncertainty
    # Inactive dimensions: low variance of mu (collapsed to prior)
    active_ratio = dim_variance / (dim_uncertainty + 1e-7)

    # Sort dimensions by activity
    sorted_dims = torch.argsort(active_ratio, descending=True)

    print("Latent dimension activity (most active first):")
    for i, dim in enumerate(sorted_dims):
        status = "ACTIVE" if active_ratio[dim] > 0.5 else "inactive"
        print(f"  Dim {dim.item():2d}: variance={dim_variance[dim]:.4f}, "
              f"uncertainty={dim_uncertainty[dim]:.4f}, "
              f"ratio={active_ratio[dim]:.4f} [{status}]")

    return sorted_dims, active_ratio


def create_control_mapping(sorted_dims, active_ratio, n_controls=4):
    """
    Map the top N active latent dimensions to user controls.
    Remaining active dims get PCA-compressed into additional controls.
    """
    active_dims = sorted_dims[active_ratio[sorted_dims] > 0.5]
    n_active = len(active_dims)

    mapping = {
        'direct_controls': active_dims[:n_controls].tolist(),
        'n_active_dims': n_active,
        'n_inactive_dims': 32 - n_active,
    }

    print(f"\nControl mapping:")
    print(f"  Direct controls (XY pad + knobs): dims {mapping['direct_controls']}")
    print(f"  Active dimensions: {n_active}/32")
    print(f"  Inactive dimensions (sample from prior): {32 - n_active}/32")

    return mapping
```

### 3.2 User Control Strategy

```
LATENT SPACE CONTROL MAPPING
==============================

32 latent dimensions total:
|
+-- Top 2 active dims -> XY Pad (primary texture navigation)
|   These capture the most perceptual variation in the texture.
|   Example for "Rain": X = intensity, Y = surface type
|
+-- Next 2 active dims -> Character A / Character B knobs
|   Secondary variation axes.
|   Example for "Rain": A = distance, B = wind content
|
+-- Remaining active dims -> "Evolve" modulation
|   Slowly wandering random walk through these dimensions
|   Rate controlled by "Evolve Speed" knob
|
+-- Inactive dims -> Sampled from N(0,1) each block
    These add micro-variation without affecting overall character
```

---

## 4. Prior Model (Generative Mode)

### 4.1 Architecture

A small autoregressive model that generates plausible sequences of latent vectors:

```python
class TexturePrior(nn.Module):
    """
    Autoregressive prior model.
    Given a sequence of past latent vectors, predicts the next one.
    Used for generative mode (no audio input needed).
    """

    def __init__(self, latent_dim=32, hidden_dim=128, n_layers=2):
        super().__init__()
        self.latent_dim = latent_dim

        self.gru = nn.GRU(
            input_size=latent_dim,
            hidden_size=hidden_dim,
            num_layers=n_layers,
            batch_first=True,
        )

        # Predict mean and logvar for next latent
        self.fc_mu = nn.Linear(hidden_dim, latent_dim)
        self.fc_logvar = nn.Linear(hidden_dim, latent_dim)

    def forward(self, z_sequence):
        """
        z_sequence: (batch, seq_len, latent_dim)
        Returns predicted mu, logvar for next timestep.
        """
        output, _ = self.gru(z_sequence)
        last = output[:, -1, :]  # Take last timestep
        mu = self.fc_mu(last)
        logvar = self.fc_logvar(last)
        return mu, logvar

    def generate_sequence(self, length, temperature=1.0, seed=None,
                          device='cpu'):
        """Generate a sequence of latent vectors autoregressively."""
        if seed is not None:
            z = seed.unsqueeze(0).unsqueeze(0)  # (1, 1, latent_dim)
        else:
            z = torch.randn(1, 1, self.latent_dim, device=device)

        sequence = [z.squeeze(0)]
        hidden = None

        for _ in range(length - 1):
            output, hidden = self.gru(z, hidden)
            mu = self.fc_mu(output[:, -1, :])
            logvar = self.fc_logvar(output[:, -1, :])

            # Sample with temperature control
            std = torch.exp(0.5 * logvar) * temperature
            eps = torch.randn_like(std)
            z_next = mu + eps * std

            z = z_next.unsqueeze(1)
            sequence.append(z_next)

        return torch.stack(sequence, dim=1)  # (1, length, latent_dim)
```

### 4.2 Prior Training

```python
def train_prior(vae_model, dataset, output_dir, device='cuda',
                epochs=100, seq_len=32, batch_size=16):
    """
    Train the prior model on latent sequences extracted from the VAE.
    """
    vae_model.eval()
    prior = TexturePrior(latent_dim=vae_model.latent_dim).to(device)
    optimizer = torch.optim.Adam(prior.parameters(), lr=1e-3)

    for epoch in range(epochs):
        # Extract consecutive latent vectors from audio
        for batch in DataLoader(dataset, batch_size=1, shuffle=True):
            audio = batch.to(device)  # (1, 1, total_samples)
            total_samples = audio.shape[2]
            block_size = 4096

            # Extract latent sequence from consecutive blocks
            latents = []
            for start in range(0, total_samples - block_size, block_size):
                chunk = audio[:, :, start:start + block_size]
                with torch.no_grad():
                    mu, _ = vae_model.encoder(chunk)
                latents.append(mu)

            if len(latents) < seq_len + 1:
                continue

            latents = torch.stack(latents, dim=1)  # (1, n_blocks, 32)

            # Train on random subsequences
            for _ in range(4):
                start_idx = torch.randint(0, len(latents[0]) - seq_len - 1, (1,)).item()
                input_seq = latents[:, start_idx:start_idx + seq_len]
                target = latents[:, start_idx + 1:start_idx + seq_len + 1]

                pred_mu, pred_logvar = [], []
                for t in range(seq_len):
                    mu, logvar = prior(input_seq[:, :t + 1])
                    pred_mu.append(mu)
                    pred_logvar.append(logvar)

                pred_mu = torch.stack(pred_mu, dim=1)

                # Simple L2 loss on predicted means
                loss = F.mse_loss(pred_mu, target)

                optimizer.zero_grad()
                loss.backward()
                optimizer.step()

        if epoch % 10 == 0:
            print(f"Prior epoch {epoch}: loss={loss.item():.4f}")

    torch.save(prior.state_dict(), f"{output_dir}/prior_final.pt")
    return prior
```

---

## 5. Deployment: PyTorch to JUCE Plugin

### 5.1 Export to ONNX

```python
def export_to_onnx(model, output_dir):
    """Export encoder, decoder, and prior as separate ONNX models."""
    model.eval()
    model.cpu()

    # Export decoder (most important -- this runs in real-time)
    z_dummy = torch.randn(1, 32)
    torch.onnx.export(
        model.decoder,
        z_dummy,
        f"{output_dir}/decoder.onnx",
        input_names=["latent"],
        output_names=["audio"],
        dynamic_axes={"latent": {0: "batch"}, "audio": {0: "batch"}},
        opset_version=17,
    )

    # Export encoder (used in transform mode)
    x_dummy = torch.randn(1, 1, 4096)
    torch.onnx.export(
        model.encoder,
        x_dummy,
        f"{output_dir}/encoder.onnx",
        input_names=["audio"],
        output_names=["mu", "logvar"],
        dynamic_axes={"audio": {0: "batch"}},
        opset_version=17,
    )

    print(f"Exported to {output_dir}/decoder.onnx and encoder.onnx")


def export_prior_to_onnx(prior, output_dir):
    """Export prior model. More complex due to RNN state."""
    prior.eval()
    prior.cpu()

    # For ONNX, unroll the GRU and export as stateful
    # Alternative: export as simple feedforward with external state management
    z_seq = torch.randn(1, 16, 32)  # (batch, seq_len, latent_dim)
    torch.onnx.export(
        prior,
        z_seq,
        f"{output_dir}/prior.onnx",
        input_names=["latent_sequence"],
        output_names=["next_mu", "next_logvar"],
        dynamic_axes={
            "latent_sequence": {0: "batch", 1: "seq_len"},
        },
        opset_version=17,
    )
    print(f"Exported to {output_dir}/prior.onnx")
```

### 5.2 Model Sizes (Estimated)

| Component | Parameters | ONNX Size (FP32) | ONNX Size (INT8) |
|---|---|---|---|
| Encoder | ~600k | ~2.4 MB | ~0.6 MB |
| Decoder | ~600k | ~2.4 MB | ~0.6 MB |
| Prior (per texture) | ~100k | ~0.4 MB | ~0.1 MB |
| **Total per texture** | **~1.3M** | **~5.2 MB** | **~1.3 MB** |
| **All 6 textures** | **~7.8M** | **~31 MB** | **~8 MB** |

### 5.3 JUCE Integration Architecture

```
PLUGIN ARCHITECTURE
=====================

+----------------------------------------------------------+
|  O-Texture JUCE Plugin                                    |
|                                                           |
|  +-----------------+     +---------------------------+    |
|  | Mode Switch     |     | ANIRA InferenceHandler    |    |
|  | [Generate|      |     |                           |    |
|  |  Transform]     |     |  Thread Pool (2 threads)  |    |
|  +-----------------+     |  +---------------------+  |    |
|                          |  | ONNX Runtime        |  |    |
|  +-----------------+     |  | - decoder.onnx      |  |    |
|  | Model Selector  |     |  | - encoder.onnx      |  |    |
|  | [Rain|Metal|    |     |  | - prior.onnx        |  |    |
|  |  Wind|Crowd|    |     |  +---------------------+  |    |
|  |  Synth|Organic] |     +---------------------------+    |
|  +-----------------+                |                     |
|                                     |                     |
|  +-----------------+     +----------v---------+           |
|  | Latent Controls |     | Audio Thread       |           |
|  | - XY Pad        | --> | (processBlock)     |           |
|  | - Character A/B |     |                    |           |
|  | - Evolve Rate   |     | 1. Get latent z    |           |
|  | - Density       |     |    from controls   |           |
|  | - Freeze        |     |    + prior output  |           |
|  +-----------------+     | 2. Read decoded    |           |
|                          |    audio from ANIRA |           |
|  +-----------------+     | 3. Crossfade       |           |
|  | Post-Processing |     |    blocks           |           |
|  | - Brightness    |     | 4. Apply post-FX   |           |
|  | - Warmth        |     | 5. Output          |           |
|  | - Stereo Width  |     +--------------------+           |
|  +-----------------+                                      |
+----------------------------------------------------------+
```

### 5.4 Crossfade Strategy

Since the decoder produces fixed 4096-sample blocks, consecutive blocks must be crossfaded to avoid clicking:

```cpp
// Overlap-add with 50% overlap
// Block N:     [----fade in----][  full  ][----fade out----]
// Block N+1:              [----fade in----][  full  ][----fade out----]
//
// Output:      [  N only  ][  N+N+1 blend  ][  N+1 only  ]

static constexpr int BLOCK_SIZE = 4096;
static constexpr int OVERLAP = 2048;    // 50% overlap
static constexpr int HOP_SIZE = 2048;   // New audio per block

// In processBlock:
// 1. Every HOP_SIZE samples, request new decoder inference
// 2. Crossfade between old and new block in overlap region
// 3. Output from overlap-add buffer
```

### 5.5 Latency

| Component | Samples | Time (48kHz) |
|---|---|---|
| Decoder block size | 4096 | 85.3ms |
| Overlap buffering | 2048 | 42.7ms |
| ANIRA inference overhead | ~1024 | ~21ms |
| **Total reported latency** | **~4096-6144** | **~85-128ms** |

Note: For generative mode (no audio input), latency is irrelevant -- the plugin generates audio continuously. Latency only matters in Transform mode (processing input audio).

---

## 6. Stereo Output

The model is mono internally. Create stereo width with decorrelation:

```cpp
// Simple stereo decorrelation
// 1. Generate mono texture from decoder
// 2. Create a slightly different version by:
//    a. Applying a short allpass filter chain (different per channel)
//    b. Or: generate two consecutive blocks, pan L/R with crossfade
//    c. Or: use two latent vectors slightly offset in latent space

// Option (c) is most interesting:
// z_left  = z + small_offset_left
// z_right = z + small_offset_right
// Decode both -> natural stereo variation from same texture class
```

---

## 7. Estimated Performance

| Metric | Value |
|---|---|
| Model parameters | ~1.3M per texture |
| ONNX decoder inference | ~1-3ms per block (estimated, CPU) |
| Blocks per second | ~23 (4096 samples / 48kHz, 50% overlap) |
| CPU usage | ~5-10% estimated (single core) |
| Memory | ~50-100MB (model + buffers) |
| Plugin binary size | ~30MB (6 models) + ONNX Runtime (~50MB) |

---

## 8. Development Phases

### Phase 1: Training Infrastructure (1-2 weeks)
- Set up PyTorch training environment
- Curate 6 texture datasets (30 min each)
- Implement TextureVAE model
- Implement training loop with multi-scale spectral loss
- Train first model (Rain) as proof of concept
- Validate: can it reconstruct textures? Does latent interpolation sound good?

### Phase 2: Generative Capability (1 week)
- Train prior model on Rain texture latents
- Validate: does autoregressive generation produce coherent, evolving textures?
- Experiment with temperature parameter for variation control
- Train remaining 5 texture models + priors

### Phase 3: JUCE Plugin Shell (2 weeks)
- JUCE project setup with ANIRA + ONNX Runtime
- Implement decoder inference pipeline
- Implement overlap-add crossfading
- Basic parameter controls (model selector, XY pad)
- Latency reporting

### Phase 4: Latent Space Controls (1-2 weeks)
- Post-training latent analysis (SVD, dimension activity)
- Map active dimensions to XY pad and knobs
- Implement evolve modulation (random walk through latent space)
- Implement freeze control
- Stereo decorrelation

### Phase 5: Post-Processing + Polish (1-2 weeks)
- Brightness/warmth post-EQ
- Density control (mix decoder output levels)
- WebView UI
- Preset system
- Testing and optimization

**Total: 6-9 weeks**

---

## 9. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| VAE produces blurry/muffled audio | Medium | High | Add adversarial training (Phase 2 optional); use multi-scale spectral loss |
| Latent space not controllable | Low | High | 32 dims is conservative; analyze post-training; increase dim if needed |
| ONNX Runtime too large for plugin | Low | Medium | Use ort-builder to strip unused operators; or switch to RTNeural for decoder |
| Crossfade artifacts between blocks | Medium | Medium | Tune overlap ratio; use Hann window; test extensively |
| Training data quality insufficient | Medium | High | Curate carefully; normalize loudness; remove silence/transients |
| Prior model generates repetitive sequences | Medium | Medium | Add temperature control; train longer; larger hidden dim |
| CPU too high for real-time | Low | High | Model is small; quantize to INT8 if needed; profile early |
