"""
O-Texture VAE Training Script
Trains the TextureVAE on preprocessed rain audio with cyclical KL annealing.

Usage:
  python train_vae.py
  python train_vae.py --resume checkpoints/step_100000.pt
  python train_vae.py --steps 100000 --batch-size 64
"""

import torch
import torch.amp
from torch.utils.data import DataLoader
from torch.optim.lr_scheduler import CosineAnnealingLR, LinearLR, SequentialLR
import numpy as np
import time
import argparse
from pathlib import Path

import config
from models import TextureVAE
from losses import TextureVAELoss
from dataset import TextureDataset


def frange_cycle_linear(n_iter, start=0.0, stop=1.0, n_cycle=4, ratio=0.5):
    """Cyclical annealing schedule for KL weight (beta)."""
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


def train(args):
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Device: {device}")

    # Dataset
    dataset = TextureDataset()
    loader = DataLoader(
        dataset,
        batch_size=args.batch_size,
        shuffle=True,
        num_workers=4,
        pin_memory=True,
        drop_last=True,
    )

    # Model
    model = TextureVAE().to(device)
    param_count = sum(p.numel() for p in model.parameters())
    print(f"Model parameters: {param_count:,}")

    # Loss
    criterion = TextureVAELoss()

    # Optimizer
    optimizer = torch.optim.AdamW(
        model.parameters(),
        lr=args.lr,
        weight_decay=config.VAE_WEIGHT_DECAY,
    )

    # LR schedule: warmup + cosine decay
    warmup = LinearLR(optimizer, start_factor=0.01, total_iters=config.VAE_WARMUP_STEPS)
    cosine = CosineAnnealingLR(optimizer, T_max=args.steps - config.VAE_WARMUP_STEPS,
                                eta_min=config.VAE_LR_MIN)
    scheduler = SequentialLR(optimizer, [warmup, cosine],
                              milestones=[config.VAE_WARMUP_STEPS])

    # KL annealing
    beta_schedule = frange_cycle_linear(
        args.steps,
        start=0.0,
        stop=config.KL_BETA_MAX,
        n_cycle=config.KL_NUM_CYCLES,
        ratio=config.KL_RATIO,
    )

    # Mixed precision
    scaler = torch.amp.GradScaler('cuda') if device.type == 'cuda' else None

    # Resume from checkpoint
    start_step = 0
    if args.resume:
        checkpoint = torch.load(args.resume, map_location=device)
        model.load_state_dict(checkpoint['model'])
        optimizer.load_state_dict(checkpoint['optimizer'])
        scheduler.load_state_dict(checkpoint['scheduler'])
        start_step = checkpoint['step']
        print(f"Resumed from step {start_step}")

    # Training loop
    config.CHECKPOINT_DIR.mkdir(parents=True, exist_ok=True)
    model.train()
    step = start_step
    best_spectral = float('inf')
    loader_iter = iter(loader)

    print(f"\nTraining for {args.steps} steps (batch_size={args.batch_size})")
    print(f"KL annealing: {config.KL_NUM_CYCLES} cycles, beta_max={config.KL_BETA_MAX}")
    print()

    t_start = time.time()

    while step < args.steps:
        # Get batch (cycle through dataset)
        try:
            batch = next(loader_iter)
        except StopIteration:
            loader_iter = iter(loader)
            batch = next(loader_iter)

        x = batch.to(device)  # (B, 1, 4096)
        beta = float(beta_schedule[step])

        # Forward pass
        optimizer.zero_grad()

        if scaler is not None:
            with torch.amp.autocast('cuda'):
                recon, mu, logvar = model(x)
                total_loss, spec_loss, l1_loss, kl_loss = criterion(
                    x, recon, mu, logvar, beta
                )
            scaler.scale(total_loss).backward()
            scaler.unscale_(optimizer)
            torch.nn.utils.clip_grad_norm_(model.parameters(), config.VAE_GRAD_CLIP)
            scaler.step(optimizer)
            scaler.update()
        else:
            recon, mu, logvar = model(x)
            total_loss, spec_loss, l1_loss, kl_loss = criterion(
                x, recon, mu, logvar, beta
            )
            total_loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), config.VAE_GRAD_CLIP)
            optimizer.step()

        scheduler.step()
        step += 1

        # Logging
        if step % config.LOG_EVERY_STEPS == 0:
            elapsed = time.time() - t_start
            steps_per_sec = step / elapsed if elapsed > 0 else 0
            lr = optimizer.param_groups[0]['lr']
            print(f"Step {step:>7d}/{args.steps} | "
                  f"loss={total_loss.item():.4f} "
                  f"spec={spec_loss.item():.4f} "
                  f"l1={l1_loss.item():.4f} "
                  f"kl={kl_loss.item():.2f} "
                  f"beta={beta:.6f} "
                  f"lr={lr:.2e} "
                  f"| {steps_per_sec:.1f} steps/s")

        # Checkpointing
        if step % config.SAVE_EVERY_STEPS == 0:
            save_checkpoint(model, optimizer, scheduler, step,
                            config.CHECKPOINT_DIR / f"step_{step}.pt")

            # Save best by spectral loss
            if spec_loss.item() < best_spectral:
                best_spectral = spec_loss.item()
                save_checkpoint(model, optimizer, scheduler, step,
                                config.CHECKPOINT_DIR / "best_spectral.pt")
                print(f"  New best spectral: {best_spectral:.4f}")

    # Final save
    save_checkpoint(model, optimizer, scheduler, step,
                    config.CHECKPOINT_DIR / "final.pt")

    elapsed = time.time() - t_start
    print(f"\nTraining complete: {step} steps in {elapsed/3600:.1f} hours")
    print(f"Best spectral convergence: {best_spectral:.4f}")


def save_checkpoint(model, optimizer, scheduler, step, path):
    torch.save({
        'model': model.state_dict(),
        'optimizer': optimizer.state_dict(),
        'scheduler': scheduler.state_dict(),
        'step': step,
    }, str(path))
    print(f"  Checkpoint saved: {path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train O-Texture Rain VAE")
    parser.add_argument("--steps", type=int, default=config.VAE_TOTAL_STEPS)
    parser.add_argument("--batch-size", type=int, default=config.VAE_BATCH_SIZE)
    parser.add_argument("--lr", type=float, default=config.VAE_LEARNING_RATE)
    parser.add_argument("--resume", type=str, default=None,
                        help="Path to checkpoint to resume from")
    args = parser.parse_args()

    train(args)
