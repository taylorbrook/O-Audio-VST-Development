"""
O-Texture Latent Space Analysis
t-SNE visualization, per-dimension variance, and dim_map generation.

Usage:
  python analyze_latent.py --checkpoint checkpoints/best_spectral.pt
"""

import torch
import numpy as np
import json
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

import config
from models import TextureVAE
from dataset import TextureDataset


def extract_latents(model, dataset, device, n_samples=2000):
    """Extract latent mu vectors from dataset samples."""
    model.eval()
    latents = []

    with torch.no_grad():
        for i in range(min(n_samples, len(dataset))):
            x = dataset[i].unsqueeze(0).to(device)
            mu, _ = model.encoder(x)
            latents.append(mu.cpu().numpy())

    return np.concatenate(latents, axis=0)  # (N, 32)


def analyze_dimensions(latents, output_dir):
    """Analyze per-dimension variance and activity."""
    dim_variance = np.var(latents, axis=0)
    dim_mean = np.mean(latents, axis=0)
    dim_std = np.std(latents, axis=0)

    # Sort dimensions by variance (most active first)
    sorted_dims = np.argsort(dim_variance)[::-1]

    print("\n=== Dimension Activity ===")
    print(f"{'Dim':>4} | {'Variance':>10} | {'Mean':>8} | {'Std':>8} | Status")
    print("-" * 55)

    active_dims = []
    for rank, dim in enumerate(sorted_dims):
        var = dim_variance[dim]
        status = "ACTIVE" if var > 0.1 else "inactive"
        if var > 0.1:
            active_dims.append(int(dim))
        print(f"{dim:4d} | {var:10.4f} | {dim_mean[dim]:8.4f} | {dim_std[dim]:8.4f} | {status}")

    print(f"\nActive dimensions (var > 0.1): {len(active_dims)}/32")
    print(f"Active dim indices: {active_dims}")

    # Plot dimension variances
    fig, ax = plt.subplots(figsize=(14, 5))
    colors = ['#2196F3' if v > 0.1 else '#BDBDBD' for v in dim_variance]
    ax.bar(range(config.LATENT_DIM), dim_variance, color=colors)
    ax.axhline(y=0.1, color='red', linestyle='--', alpha=0.5, label='Activity threshold')
    ax.set_xlabel("Latent Dimension")
    ax.set_ylabel("Variance")
    ax.set_title("Per-Dimension Variance (Blue = Active)")
    ax.legend()
    plt.tight_layout()
    plt.savefig(str(output_dir / "dimension_variance.png"), dpi=150)
    plt.close()

    return active_dims, dim_variance


def visualize_tsne(latents, output_dir):
    """Create t-SNE visualization of latent space."""
    from sklearn.manifold import TSNE

    print("\nRunning t-SNE (this may take a minute)...")
    tsne = TSNE(n_components=2, perplexity=30, random_state=42, n_iter=1000)
    latents_2d = tsne.fit_transform(latents)

    fig, ax = plt.subplots(figsize=(10, 8))
    ax.scatter(latents_2d[:, 0], latents_2d[:, 1], alpha=0.4, s=8, c='#2196F3')
    ax.set_title("Latent Space (t-SNE)")
    ax.set_xlabel("t-SNE 1")
    ax.set_ylabel("t-SNE 2")
    plt.tight_layout()
    plt.savefig(str(output_dir / "latent_tsne.png"), dpi=150)
    plt.close()

    print(f"  t-SNE saved: {output_dir / 'latent_tsne.png'}")

    # Check structure
    from scipy.spatial.distance import pdist
    dists = pdist(latents_2d)
    print(f"  Distance stats: mean={np.mean(dists):.2f}, std={np.std(dists):.2f}")


def generate_dim_map(active_dims, dim_variance, output_dir):
    """
    Generate dim_map_rain.json mapping active dims to control parameters.

    Assignment strategy:
    - Top 4 by variance -> X, Y, CHARACTER_A, CHARACTER_B
    - Remaining active dims -> EVOLVE modulation targets
    - Inactive dims -> sampled from N(0,1) each block
    """
    sorted_active = sorted(active_dims, key=lambda d: dim_variance[d], reverse=True)

    dim_map = {
        "texture": "rain",
        "latent_dim": config.LATENT_DIM,
        "active_dims": [int(d) for d in sorted_active],
        "mapping": {}
    }

    # Top 4 -> user controls
    param_names = ["X", "Y", "CHARACTER_A", "CHARACTER_B"]
    for i, name in enumerate(param_names):
        if i < len(sorted_active):
            dim_map["mapping"][name] = {
                "dim": int(sorted_active[i]),
                "variance": float(dim_variance[sorted_active[i]])
            }

    # Remaining active -> evolve
    evolve_dims = sorted_active[4:] if len(sorted_active) > 4 else []
    dim_map["evolve_dims"] = [int(d) for d in evolve_dims]

    # Inactive dims
    all_dims = set(range(config.LATENT_DIM))
    inactive = sorted(all_dims - set(active_dims))
    dim_map["inactive_dims"] = [int(d) for d in inactive]

    # Save
    out_path = output_dir / "dim_map_rain.json"
    with open(str(out_path), 'w') as f:
        json.dump(dim_map, f, indent=2)

    print(f"\n=== Dimension Map ===")
    for name, info in dim_map["mapping"].items():
        print(f"  {name} -> dim {info['dim']} (var={info['variance']:.4f})")
    print(f"  EVOLVE -> {len(evolve_dims)} dims: {evolve_dims}")
    print(f"  Inactive: {len(inactive)} dims")
    print(f"  Saved: {out_path}")

    return dim_map


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Analyze O-Texture latent space")
    parser.add_argument("--checkpoint", type=str, required=True)
    parser.add_argument("--n-samples", type=int, default=2000)
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    model = TextureVAE().to(device)
    checkpoint = torch.load(args.checkpoint, map_location=device)
    model.load_state_dict(checkpoint['model'])
    print(f"Loaded: {args.checkpoint} (step {checkpoint['step']})")

    dataset = TextureDataset()

    output_dir = config.OUTPUT_DIR
    output_dir.mkdir(parents=True, exist_ok=True)

    latents = extract_latents(model, dataset, device, args.n_samples)
    print(f"Extracted {len(latents)} latent vectors")

    active_dims, dim_variance = analyze_dimensions(latents, output_dir)
    visualize_tsne(latents, output_dir)
    dim_map = generate_dim_map(active_dims, dim_variance, output_dir)
