"""
O-Texture GRU Prior Training
Trains autoregressive prior on latent sequences from trained VAE.

Usage:
  python train_prior.py --vae-checkpoint checkpoints/best_spectral.pt
"""

import torch
import torch.nn.functional as F
import torchaudio
from torch.utils.data import DataLoader, TensorDataset
from torch.optim.lr_scheduler import CosineAnnealingLR
import numpy as np
from pathlib import Path
import argparse

import config
from models import TextureVAE, TexturePrior


def extract_latent_sequences(vae_model, audio_files, device):
    """Extract consecutive latent vectors from audio files."""
    vae_model.eval()
    sequences = []

    for filepath in audio_files:
        waveform, sr = torchaudio.load(str(filepath))
        if sr != config.SAMPLE_RATE:
            waveform = torchaudio.transforms.Resample(sr, config.SAMPLE_RATE)(waveform)
        if waveform.shape[0] > 1:
            waveform = waveform.mean(dim=0, keepdim=True)

        n_blocks = waveform.shape[1] // config.BLOCK_SIZE
        if n_blocks < 2:
            continue

        latents = []
        with torch.no_grad():
            for i in range(n_blocks):
                block = waveform[:, i * config.BLOCK_SIZE:(i + 1) * config.BLOCK_SIZE]
                block = block.unsqueeze(0).to(device)
                mu, _ = vae_model.encoder(block)
                latents.append(mu.cpu())

        sequence = torch.cat(latents, dim=0)  # (n_blocks, 32)
        sequences.append(sequence)
        duration = n_blocks * config.BLOCK_SIZE / config.SAMPLE_RATE
        print(f"  {filepath.name}: {n_blocks} blocks ({duration:.1f}s)")

    return sequences


def create_training_data(sequences, seq_len=config.PRIOR_SEQ_LEN):
    """Create sliding window training examples from latent sequences."""
    inputs = []
    targets = []

    for seq in sequences:
        if len(seq) <= seq_len:
            continue
        for start in range(len(seq) - seq_len):
            inputs.append(seq[start:start + seq_len])
            targets.append(seq[start + 1:start + seq_len + 1])

    inputs = torch.stack(inputs)   # (N, seq_len, 32)
    targets = torch.stack(targets)  # (N, seq_len, 32)

    print(f"  Training examples: {len(inputs)}")
    return inputs, targets


def train_prior(args):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Device: {device}")

    # Load VAE
    vae = TextureVAE().to(device)
    vae_ckpt = torch.load(args.vae_checkpoint, map_location=device)
    vae.load_state_dict(vae_ckpt['model'])
    print(f"Loaded VAE: {args.vae_checkpoint} (step {vae_ckpt['step']})")

    # Extract latent sequences from processed audio
    audio_files = sorted(config.DATA_PROCESSED_DIR.glob("*.wav"))
    if not audio_files:
        print(f"ERROR: No processed audio in {config.DATA_PROCESSED_DIR}")
        return

    print(f"\nExtracting latent sequences from {len(audio_files)} files...")
    sequences = extract_latent_sequences(vae, audio_files, device)
    total_blocks = sum(len(s) for s in sequences)
    print(f"Total latent vectors: {total_blocks}")

    # Create training data
    print(f"\nCreating training data (seq_len={args.seq_len})...")
    train_inputs, train_targets = create_training_data(sequences, args.seq_len)

    dataset = TensorDataset(train_inputs, train_targets)
    loader = DataLoader(dataset, batch_size=args.batch_size, shuffle=True,
                        pin_memory=True, drop_last=True)

    # Prior model
    prior = TexturePrior().to(device)
    param_count = sum(p.numel() for p in prior.parameters())
    print(f"\nPrior parameters: {param_count:,}")

    optimizer = torch.optim.Adam(prior.parameters(), lr=args.lr)
    scheduler = CosineAnnealingLR(optimizer, T_max=args.epochs,
                                   eta_min=config.PRIOR_LR_MIN)

    # Training loop
    config.CHECKPOINT_DIR.mkdir(parents=True, exist_ok=True)
    best_loss = float('inf')

    print(f"\nTraining prior for {args.epochs} epochs")

    for epoch in range(args.epochs):
        prior.train()
        total_loss = 0.0
        n_batches = 0

        for batch_inputs, batch_targets in loader:
            batch_inputs = batch_inputs.to(device)
            batch_targets = batch_targets.to(device)

            # Teacher forcing: full sequence forward pass
            pred_mu, pred_logvar, _ = prior(batch_inputs)
            loss = F.mse_loss(pred_mu, batch_targets)

            optimizer.zero_grad()
            loss.backward()
            torch.nn.utils.clip_grad_norm_(prior.parameters(), config.PRIOR_GRAD_CLIP)
            optimizer.step()

            total_loss += loss.item()
            n_batches += 1

        scheduler.step()
        avg_loss = total_loss / max(n_batches, 1)

        if epoch % 20 == 0 or epoch == args.epochs - 1:
            print(f"  Epoch {epoch:>4d}/{args.epochs} | loss={avg_loss:.6f} | "
                  f"lr={optimizer.param_groups[0]['lr']:.2e}")

        if avg_loss < best_loss:
            best_loss = avg_loss
            torch.save(prior.state_dict(),
                       str(config.CHECKPOINT_DIR / "prior_best.pt"))

    # Final save
    torch.save(prior.state_dict(),
               str(config.CHECKPOINT_DIR / "prior_final.pt"))

    print(f"\nPrior training complete. Best loss: {best_loss:.6f}")
    print(f"  Best: {config.CHECKPOINT_DIR / 'prior_best.pt'}")
    print(f"  Final: {config.CHECKPOINT_DIR / 'prior_final.pt'}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train O-Texture GRU Prior")
    parser.add_argument("--vae-checkpoint", type=str, required=True)
    parser.add_argument("--epochs", type=int, default=config.PRIOR_EPOCHS)
    parser.add_argument("--batch-size", type=int, default=config.PRIOR_BATCH_SIZE)
    parser.add_argument("--lr", type=float, default=config.PRIOR_LEARNING_RATE)
    parser.add_argument("--seq-len", type=int, default=config.PRIOR_SEQ_LEN)
    args = parser.parse_args()

    train_prior(args)
