"""
O-Texture ONNX Export
Export trained VAE encoder, decoder, and prior to ONNX format.
Validates exports against PyTorch outputs.

Usage:
  python export_onnx.py --vae-checkpoint checkpoints/best_spectral.pt \
                        --prior-checkpoint checkpoints/prior_best.pt
"""

import torch
import numpy as np
from pathlib import Path
import argparse

import config
from models import TextureVAE, TexturePrior, TexturePriorONNX


def export_decoder(model, output_dir, device):
    """Export decoder: z[1, 32] -> audio[1, 1, 4096]"""
    model.eval()
    decoder = model.decoder.cpu()

    z_dummy = torch.randn(1, config.LATENT_DIM)
    out_path = output_dir / "rain_decoder.onnx"

    torch.onnx.export(
        decoder,
        z_dummy,
        str(out_path),
        input_names=["latent"],
        output_names=["audio"],
        opset_version=config.ONNX_OPSET,
        do_constant_folding=True,
    )
    print(f"  Decoder exported: {out_path}")

    # Validate
    validate_onnx(decoder, str(out_path), z_dummy, "Decoder")
    model.to(device)
    return out_path


def export_encoder(model, output_dir, device):
    """Export encoder: audio[1, 1, 4096] -> mu[1, 32], logvar[1, 32]"""
    model.eval()
    encoder = model.encoder.cpu()

    x_dummy = torch.randn(1, 1, config.BLOCK_SIZE)
    out_path = output_dir / "rain_encoder.onnx"

    torch.onnx.export(
        encoder,
        x_dummy,
        str(out_path),
        input_names=["audio"],
        output_names=["mu", "logvar"],
        opset_version=config.ONNX_OPSET,
        do_constant_folding=True,
    )
    print(f"  Encoder exported: {out_path}")

    validate_onnx(encoder, str(out_path), x_dummy, "Encoder")
    model.to(device)
    return out_path


def export_prior(prior, output_dir):
    """Export prior: z_input[1,1,32] + hidden[2,1,128] -> mu, logvar, hidden_out"""
    prior.eval()
    wrapper = TexturePriorONNX(prior).cpu()

    z_dummy = torch.randn(1, 1, config.LATENT_DIM)
    h_dummy = torch.zeros(config.PRIOR_NUM_LAYERS, 1, config.PRIOR_HIDDEN_DIM)
    out_path = output_dir / "rain_prior.onnx"

    torch.onnx.export(
        wrapper,
        (z_dummy, h_dummy),
        str(out_path),
        input_names=["z_input", "hidden_in"],
        output_names=["mu", "logvar", "hidden_out"],
        opset_version=config.ONNX_OPSET,
        do_constant_folding=True,
    )
    print(f"  Prior exported: {out_path}")

    validate_onnx(wrapper, str(out_path), (z_dummy, h_dummy), "Prior")
    return out_path


def validate_onnx(pytorch_model, onnx_path, test_input, name, atol=1e-5):
    """Validate ONNX model against PyTorch output."""
    try:
        import onnxruntime as ort
    except ImportError:
        print(f"  WARNING: onnxruntime not available, skipping {name} validation")
        return

    pytorch_model.eval()
    with torch.no_grad():
        if isinstance(test_input, tuple):
            pt_output = pytorch_model(*test_input)
        else:
            pt_output = pytorch_model(test_input)

    session = ort.InferenceSession(onnx_path)
    input_names = [inp.name for inp in session.get_inputs()]

    if isinstance(test_input, tuple):
        ort_inputs = {name: inp.numpy() for name, inp in zip(input_names, test_input)}
    else:
        ort_inputs = {input_names[0]: test_input.numpy()}

    ort_output = session.run(None, ort_inputs)

    if isinstance(pt_output, tuple):
        outputs = list(pt_output)
    else:
        outputs = [pt_output]

    all_pass = True
    for i, (pt, ort_out) in enumerate(zip(outputs, ort_output)):
        diff = np.abs(pt.numpy() - ort_out).max()
        passed = diff < atol
        all_pass = all_pass and passed
        status = "PASS" if passed else "FAIL"
        print(f"    Output {i}: max_diff={diff:.8f} [{status}]")

    if all_pass:
        print(f"  {name} validation: PASSED")
    else:
        print(f"  {name} validation: FAILED (max diff > {atol})")


def copy_dim_map(output_dir):
    """Copy dim_map from training output to resources."""
    src = config.OUTPUT_DIR / "dim_map_rain.json"
    dst = output_dir / "dim_map_rain.json"
    if src.exists():
        import shutil
        shutil.copy2(str(src), str(dst))
        print(f"  dim_map copied: {dst}")
    else:
        print(f"  WARNING: {src} not found. Run analyze_latent.py first.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Export O-Texture models to ONNX")
    parser.add_argument("--vae-checkpoint", type=str, required=True)
    parser.add_argument("--prior-checkpoint", type=str, required=True)
    parser.add_argument("--output", type=str, default=None,
                        help="Output directory (default: Resources/models/rain/)")
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    output_dir = Path(args.output) if args.output else config.MODELS_DIR
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"Output: {output_dir}")

    # Load VAE
    vae = TextureVAE().to(device)
    vae_ckpt = torch.load(args.vae_checkpoint, map_location=device)
    vae.load_state_dict(vae_ckpt['model'])
    print(f"Loaded VAE: step {vae_ckpt['step']}")

    # Load Prior
    prior = TexturePrior().to(device)
    prior.load_state_dict(torch.load(args.prior_checkpoint, map_location=device))
    print(f"Loaded Prior: {args.prior_checkpoint}")

    print("\n=== Exporting ONNX Models ===")
    export_decoder(vae, output_dir, device)
    export_encoder(vae, output_dir, device)
    export_prior(prior.cpu(), output_dir)
    copy_dim_map(output_dir)

    print("\n=== Export Complete ===")
    print(f"Models saved to: {output_dir}")
    print("\nFiles:")
    for f in sorted(output_dir.glob("*")):
        size_kb = f.stat().st_size / 1024
        print(f"  {f.name}: {size_kb:.1f} KB")
