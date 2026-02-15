"""
O-Texture VAE Evaluation
Reconstruction quality metrics, spectral analysis, and listening samples.

Usage:
  python evaluate.py --checkpoint checkpoints/best_spectral.pt
  python evaluate.py --checkpoint checkpoints/final.pt --n-samples 200
"""

import torch
import torch.nn.functional as F
import torchaudio
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

import config
from models import TextureVAE
from dataset import TextureDataset


def spectral_convergence(x, x_hat, fft_sizes=config.SPECTRAL_FFT_SIZES):
    """Compute multi-scale spectral convergence. Lower is better."""
    total_sc = 0.0
    x_flat = x.squeeze(1)
    x_hat_flat = x_hat.squeeze(1)

    for fft_size in fft_sizes:
        hop = fft_size // 4
        window = torch.hann_window(fft_size, device=x.device)
        X = torch.stft(x_flat, fft_size, hop, window=window, return_complex=True)
        X_hat = torch.stft(x_hat_flat, fft_size, hop, window=window, return_complex=True)
        X_mag = torch.abs(X)
        X_hat_mag = torch.abs(X_hat)
        sc = torch.norm(X_mag - X_hat_mag, p='fro') / (torch.norm(X_mag, p='fro') + 1e-7)
        total_sc += sc.item()

    return total_sc / len(fft_sizes)


def log_spectral_distance(x, x_hat, fft_size=2048, hop=512):
    """Average L1 distance between log-magnitude spectra. Lower is better."""
    window = torch.hann_window(fft_size, device=x.device)
    X = torch.stft(x.squeeze(1), fft_size, hop, window=window, return_complex=True)
    X_hat = torch.stft(x_hat.squeeze(1), fft_size, hop, window=window, return_complex=True)
    return F.l1_loss(torch.log(torch.abs(X) + 1e-7),
                     torch.log(torch.abs(X_hat) + 1e-7)).item()


def reconstruction_test(model, dataset, device, n_samples=100):
    """Test VAE reconstruction quality."""
    model.eval()
    metrics = {'sc': [], 'lsd': [], 'l1': []}

    with torch.no_grad():
        for i in range(min(n_samples, len(dataset))):
            x = dataset[i].unsqueeze(0).to(device)
            recon, mu, logvar = model(x)

            metrics['sc'].append(spectral_convergence(x, recon))
            metrics['lsd'].append(log_spectral_distance(x, recon))
            metrics['l1'].append(F.l1_loss(x, recon).item())

    print("=== Reconstruction Quality ===")
    for key, values in metrics.items():
        mean = np.mean(values)
        std = np.std(values)
        print(f"  {key}: {mean:.4f} +/- {std:.4f}")

    sc_mean = np.mean(metrics['sc'])
    if sc_mean < 0.15:
        print("  Rating: EXCELLENT")
    elif sc_mean < 0.3:
        print("  Rating: GOOD")
    elif sc_mean < 0.5:
        print("  Rating: ACCEPTABLE")
    else:
        print("  Rating: POOR - consider more training")

    return metrics


def generation_test(model, device, n_blocks=200, temperature=0.8, output_dir=None):
    """Generate audio from random latent vectors and save for listening."""
    model.eval()
    output_dir = Path(output_dir or config.OUTPUT_DIR)
    output_dir.mkdir(parents=True, exist_ok=True)

    blocks = []
    with torch.no_grad():
        for _ in range(n_blocks):
            z = torch.randn(1, config.LATENT_DIM, device=device) * temperature
            audio = model.generate(z)
            blocks.append(audio.cpu().squeeze())

    audio = torch.cat(blocks, dim=0)
    peak = audio.abs().max().item()
    duration = len(audio) / config.SAMPLE_RATE

    print(f"\n=== Generation Test ===")
    print(f"  Duration: {duration:.1f}s")
    print(f"  Peak amplitude: {peak:.4f}")
    print(f"  NaN: {torch.isnan(audio).any().item()}")
    print(f"  Inf: {torch.isinf(audio).any().item()}")

    out_path = output_dir / "generated_rain.wav"
    torchaudio.save(str(out_path), audio.unsqueeze(0), config.SAMPLE_RATE)
    print(f"  Saved: {out_path}")

    return audio


def save_reconstruction_samples(model, dataset, device, output_dir=None, n=5):
    """Save original + reconstruction pairs for listening comparison."""
    model.eval()
    output_dir = Path(output_dir or config.OUTPUT_DIR) / "reconstructions"
    output_dir.mkdir(parents=True, exist_ok=True)

    with torch.no_grad():
        for i in range(min(n, len(dataset))):
            x = dataset[i].unsqueeze(0).to(device)
            recon, _, _ = model(x)

            torchaudio.save(str(output_dir / f"original_{i}.wav"),
                           x.cpu().squeeze(0), config.SAMPLE_RATE)
            torchaudio.save(str(output_dir / f"recon_{i}.wav"),
                           recon.cpu().squeeze(0), config.SAMPLE_RATE)

    print(f"\n  Saved {n} original/reconstruction pairs to {output_dir}")


def plot_spectrograms(model, dataset, device, output_dir=None):
    """Plot spectrograms of original vs reconstruction."""
    model.eval()
    output_dir = Path(output_dir or config.OUTPUT_DIR)
    output_dir.mkdir(parents=True, exist_ok=True)

    with torch.no_grad():
        x = dataset[0].unsqueeze(0).to(device)
        recon, _, _ = model(x)

    fig, axes = plt.subplots(2, 1, figsize=(12, 8))

    for idx, (audio, title) in enumerate([(x, "Original"), (recon, "Reconstruction")]):
        spec = torch.stft(audio.cpu().squeeze(), 2048, 512,
                          window=torch.hann_window(2048), return_complex=True)
        spec_db = 20 * torch.log10(torch.abs(spec) + 1e-7)
        axes[idx].imshow(spec_db.squeeze().numpy(), aspect='auto', origin='lower',
                        cmap='magma', vmin=-80, vmax=0)
        axes[idx].set_title(title)
        axes[idx].set_ylabel("Frequency bin")

    axes[1].set_xlabel("Time frame")
    plt.tight_layout()
    plt.savefig(str(output_dir / "spectrogram_comparison.png"), dpi=150)
    plt.close()
    print(f"\n  Spectrogram saved: {output_dir / 'spectrogram_comparison.png'}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Evaluate O-Texture VAE")
    parser.add_argument("--checkpoint", type=str, required=True,
                        help="Path to model checkpoint")
    parser.add_argument("--n-samples", type=int, default=100,
                        help="Number of samples for reconstruction test")
    parser.add_argument("--generate-blocks", type=int, default=200,
                        help="Number of blocks to generate")
    parser.add_argument("--temperature", type=float, default=0.8)
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Device: {device}")

    model = TextureVAE().to(device)
    checkpoint = torch.load(args.checkpoint, map_location=device)
    model.load_state_dict(checkpoint['model'])
    print(f"Loaded checkpoint: {args.checkpoint} (step {checkpoint['step']})")

    dataset = TextureDataset()

    reconstruction_test(model, dataset, device, args.n_samples)
    generation_test(model, device, args.generate_blocks, args.temperature)
    save_reconstruction_samples(model, dataset, device)
    plot_spectrograms(model, dataset, device)
