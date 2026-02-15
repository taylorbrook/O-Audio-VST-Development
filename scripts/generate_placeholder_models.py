#!/usr/bin/env python3
"""
Generate tiny placeholder ONNX models for O-Texture plugin development.

These models have the correct I/O shapes for the neural texture synthesizer
but use minimal parameters (bottleneck architectures). They produce valid
but meaningless output -- sufficient for validating the ONNX Runtime
inference pipeline, ANIRA integration, and binary resource loading.

Requirements:
    pip install onnx onnxruntime numpy

Models generated:
    1. decoder.onnx  - [1, 32] -> [1, 1, 4096]   (latent -> audio)
    2. encoder.onnx  - [1, 1, 4096] -> [1, 32] (mu) + [1, 32] (logvar)
    3. prior.onnx    - [1, 16, 32] -> [1, 32] (mu) + [1, 32] (logvar)

All models use ONNX opset 17 to match the target real models.
"""

import os
import sys
import numpy as np
import onnx
from onnx import TensorProto, helper, numpy_helper, checker


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

OPSET_VERSION = 17
# IR version 9 is compatible with ONNX Runtime 1.19.2 (max IR version 10)
# The onnx python library auto-sets IR version based on its own version,
# so we override it explicitly after model creation.
IR_VERSION = 9
LATENT_DIM = 32
AUDIO_SAMPLES = 4096
PRIOR_SEQ_LEN = 16
SEED = 42  # For deterministic weight initialization


def _seeded_weights(shape, seed_offset=0):
    """Generate small deterministic weights for a given shape."""
    rng = np.random.RandomState(SEED + seed_offset)
    # Small weights (stddev=0.01) so outputs are non-zero but bounded
    return (rng.randn(*shape) * 0.01).astype(np.float32)


def _make_opset():
    """Create opset import for opset 17."""
    return [helper.make_opsetid("", OPSET_VERSION)]


# ---------------------------------------------------------------------------
# Model 1: Decoder
# ---------------------------------------------------------------------------

def create_decoder_model():
    """
    Decoder: latent vector -> audio samples.

    Input:  float32 [1, 32]        (latent vector z)
    Output: float32 [1, 1, 4096]   (mono audio block)

    Architecture (minimal bottleneck):
        MatMul [1,32] x [32,8]  -> [1,8]       (project down)
        MatMul [1,8]  x [8,4096] -> [1,4096]   (project up to audio)
        Reshape -> [1, 1, 4096]                 (add channel dim)

    Weight data: 32*8 + 8*4096 = 33,024 floats = ~129 KB
    """
    bottleneck = 8

    # --- Weights (initializers) ---
    w1_data = _seeded_weights((LATENT_DIM, bottleneck), seed_offset=0)
    w2_data = _seeded_weights((bottleneck, AUDIO_SAMPLES), seed_offset=1)
    shape_data = np.array([1, 1, AUDIO_SAMPLES], dtype=np.int64)

    w1 = numpy_helper.from_array(w1_data, name="decoder_w1")
    w2 = numpy_helper.from_array(w2_data, name="decoder_w2")
    output_shape = numpy_helper.from_array(shape_data, name="output_shape")

    # --- Graph I/O ---
    input_z = helper.make_tensor_value_info("z", TensorProto.FLOAT, [1, LATENT_DIM])
    output_audio = helper.make_tensor_value_info("audio", TensorProto.FLOAT, [1, 1, AUDIO_SAMPLES])

    # --- Nodes ---
    node_mm1 = helper.make_node("MatMul", ["z", "decoder_w1"], ["hidden"])
    node_mm2 = helper.make_node("MatMul", ["hidden", "decoder_w2"], ["flat_audio"])
    node_reshape = helper.make_node("Reshape", ["flat_audio", "output_shape"], ["audio"])

    # --- Graph ---
    graph = helper.make_graph(
        nodes=[node_mm1, node_mm2, node_reshape],
        name="decoder",
        inputs=[input_z],
        outputs=[output_audio],
        initializer=[w1, w2, output_shape],
    )

    # --- Model ---
    model = helper.make_model(graph, opset_imports=_make_opset())
    model.producer_name = "o-texture-placeholder"
    model.doc_string = "Placeholder decoder: [1,32] -> [1,1,4096]"

    checker.check_model(model)
    model.ir_version = IR_VERSION
    return model


# ---------------------------------------------------------------------------
# Model 2: Encoder
# ---------------------------------------------------------------------------

def create_encoder_model():
    """
    Encoder: audio samples -> latent distribution parameters.

    Input:  float32 [1, 1, 4096]   (mono audio block)
    Output: float32 [1, 32]        (mu - latent mean)
            float32 [1, 32]        (logvar - latent log-variance)

    Architecture (minimal bottleneck):
        Reshape [1,1,4096] -> [1,4096]          (flatten channel dim)
        MatMul [1,4096] x [4096,8] -> [1,8]     (project down)
        MatMul [1,8] x [8,32] -> [1,32]         (mu projection)
        MatMul [1,8] x [8,32] -> [1,32]         (logvar projection)

    Weight data: 4096*8 + 2*(8*32) = 33,280 floats = ~130 KB
    """
    bottleneck = 8

    # --- Weights ---
    w_down_data = _seeded_weights((AUDIO_SAMPLES, bottleneck), seed_offset=10)
    w_mu_data = _seeded_weights((bottleneck, LATENT_DIM), seed_offset=11)
    w_logvar_data = _seeded_weights((bottleneck, LATENT_DIM), seed_offset=12)
    flat_shape_data = np.array([1, AUDIO_SAMPLES], dtype=np.int64)

    w_down = numpy_helper.from_array(w_down_data, name="encoder_w_down")
    w_mu = numpy_helper.from_array(w_mu_data, name="encoder_w_mu")
    w_logvar = numpy_helper.from_array(w_logvar_data, name="encoder_w_logvar")
    flat_shape = numpy_helper.from_array(flat_shape_data, name="flat_shape")

    # --- Graph I/O ---
    input_audio = helper.make_tensor_value_info("audio", TensorProto.FLOAT, [1, 1, AUDIO_SAMPLES])
    output_mu = helper.make_tensor_value_info("mu", TensorProto.FLOAT, [1, LATENT_DIM])
    output_logvar = helper.make_tensor_value_info("logvar", TensorProto.FLOAT, [1, LATENT_DIM])

    # --- Nodes ---
    node_reshape = helper.make_node("Reshape", ["audio", "flat_shape"], ["flat_audio"])
    node_down = helper.make_node("MatMul", ["flat_audio", "encoder_w_down"], ["hidden"])
    node_mu = helper.make_node("MatMul", ["hidden", "encoder_w_mu"], ["mu"])
    node_logvar = helper.make_node("MatMul", ["hidden", "encoder_w_logvar"], ["logvar"])

    # --- Graph ---
    graph = helper.make_graph(
        nodes=[node_reshape, node_down, node_mu, node_logvar],
        name="encoder",
        inputs=[input_audio],
        outputs=[output_mu, output_logvar],
        initializer=[w_down, w_mu, w_logvar, flat_shape],
    )

    # --- Model ---
    model = helper.make_model(graph, opset_imports=_make_opset())
    model.producer_name = "o-texture-placeholder"
    model.doc_string = "Placeholder encoder: [1,1,4096] -> [1,32](mu) + [1,32](logvar)"

    checker.check_model(model)
    model.ir_version = IR_VERSION
    return model


# ---------------------------------------------------------------------------
# Model 3: Prior (GRU-based)
# ---------------------------------------------------------------------------

def create_prior_model():
    """
    Prior: sequence of past latent vectors -> predicted next latent distribution.

    Input:  float32 [1, 16, 32]    (batch=1, seq_len=16, latent_dim=32)
    Output: float32 [1, 32]        (mu - predicted next latent mean)
            float32 [1, 32]        (logvar - predicted next latent log-variance)

    Architecture (minimal GRU + projection):
        Transpose [1,16,32] -> [16,1,32]        (seq_first for GRU layout=0)
        GRU(input_size=32, hidden_size=8, 1 layer, 1 direction)
            -> Y_h: [1, 1, 8]  (final hidden state)
        Reshape Y_h -> [1, 8]
        MatMul [1,8] x [8,32] -> [1,32]         (mu projection)
        MatMul [1,8] x [8,32] -> [1,32]         (logvar projection)

    GRU weights:
        W: [1, 3*8, 32] = [1, 24, 32] = 768 floats
        R: [1, 3*8, 8]  = [1, 24, 8]  = 192 floats
        B: [1, 6*8]     = [1, 48]     = 48 floats
    Projection weights: 2 * (8*32) = 512 floats
    Total: 1,520 floats = ~5.9 KB
    """
    hidden_size = 8
    num_directions = 1
    num_gates = 3  # GRU has 3 gates: z (update), r (reset), h (hidden)

    # --- GRU Weights ---
    # W: [num_directions, 3*hidden_size, input_size]
    w_gru_data = _seeded_weights(
        (num_directions, num_gates * hidden_size, LATENT_DIM), seed_offset=20
    )
    # R: [num_directions, 3*hidden_size, hidden_size]
    r_gru_data = _seeded_weights(
        (num_directions, num_gates * hidden_size, hidden_size), seed_offset=21
    )
    # B: [num_directions, 6*hidden_size] (Wb_z, Wb_r, Wb_h, Rb_z, Rb_r, Rb_h)
    b_gru_data = np.zeros(
        (num_directions, 2 * num_gates * hidden_size), dtype=np.float32
    )

    # --- Projection Weights ---
    w_mu_data = _seeded_weights((hidden_size, LATENT_DIM), seed_offset=22)
    w_logvar_data = _seeded_weights((hidden_size, LATENT_DIM), seed_offset=23)

    # --- Shape constants ---
    # Transpose perm for [1,16,32] -> [16,1,32]
    hidden_flat_shape = np.array([1, hidden_size], dtype=np.int64)

    # --- Create initializers ---
    w_gru = numpy_helper.from_array(w_gru_data, name="gru_W")
    r_gru = numpy_helper.from_array(r_gru_data, name="gru_R")
    b_gru = numpy_helper.from_array(b_gru_data, name="gru_B")
    w_mu = numpy_helper.from_array(w_mu_data, name="prior_w_mu")
    w_logvar = numpy_helper.from_array(w_logvar_data, name="prior_w_logvar")
    hidden_shape = numpy_helper.from_array(hidden_flat_shape, name="hidden_flat_shape")

    # --- Graph I/O ---
    input_seq = helper.make_tensor_value_info(
        "latent_sequence", TensorProto.FLOAT, [1, PRIOR_SEQ_LEN, LATENT_DIM]
    )
    output_mu = helper.make_tensor_value_info("mu", TensorProto.FLOAT, [1, LATENT_DIM])
    output_logvar = helper.make_tensor_value_info("logvar", TensorProto.FLOAT, [1, LATENT_DIM])

    # --- Nodes ---

    # 1. Transpose [1, 16, 32] -> [16, 1, 32] (GRU expects seq_first with layout=0)
    node_transpose = helper.make_node(
        "Transpose",
        inputs=["latent_sequence"],
        outputs=["seq_first"],
        perm=[1, 0, 2],
    )

    # 2. GRU: [16, 1, 32] -> Y_h [1, 1, 8] (final hidden state only)
    #    We only request Y_h output (second output), leave Y (first output) empty
    node_gru = helper.make_node(
        "GRU",
        inputs=["seq_first", "gru_W", "gru_R", "gru_B"],
        outputs=["", "gru_Y_h"],  # "" = skip first output (all hidden states)
        hidden_size=hidden_size,
    )

    # 3. Reshape Y_h [1, 1, 8] -> [1, 8]
    node_reshape = helper.make_node(
        "Reshape", ["gru_Y_h", "hidden_flat_shape"], ["hidden_flat"]
    )

    # 4. Project to mu and logvar
    node_mu = helper.make_node("MatMul", ["hidden_flat", "prior_w_mu"], ["mu"])
    node_logvar = helper.make_node("MatMul", ["hidden_flat", "prior_w_logvar"], ["logvar"])

    # --- Graph ---
    graph = helper.make_graph(
        nodes=[node_transpose, node_gru, node_reshape, node_mu, node_logvar],
        name="prior",
        inputs=[input_seq],
        outputs=[output_mu, output_logvar],
        initializer=[w_gru, r_gru, b_gru, w_mu, w_logvar, hidden_shape],
    )

    # --- Model ---
    model = helper.make_model(graph, opset_imports=_make_opset())
    model.producer_name = "o-texture-placeholder"
    model.doc_string = (
        "Placeholder prior (GRU): [1,16,32] -> [1,32](mu) + [1,32](logvar)"
    )

    checker.check_model(model)
    model.ir_version = IR_VERSION
    return model


# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------

def verify_model(model_path, input_dict, expected_output_names, expected_output_shapes):
    """
    Load an ONNX model and run inference to verify correct I/O shapes.
    Tests both file-based and memory-based loading.
    """
    import onnxruntime as ort

    print(f"\n{'='*60}")
    print(f"Verifying: {os.path.basename(model_path)}")
    print(f"{'='*60}")

    # --- File size ---
    file_size = os.path.getsize(model_path)
    print(f"  File size: {file_size:,} bytes ({file_size / 1024:.1f} KB)")

    # --- Load from file ---
    print(f"  Loading from file path... ", end="")
    session_file = ort.InferenceSession(
        model_path, providers=["CPUExecutionProvider"]
    )
    print("OK")

    # --- Load from memory (bytes) ---
    print(f"  Loading from memory (bytes)... ", end="")
    with open(model_path, "rb") as f:
        model_bytes = f.read()
    session_mem = ort.InferenceSession(
        model_bytes, providers=["CPUExecutionProvider"]
    )
    print("OK")

    # --- Load from SerializeToString ---
    print(f"  Loading from SerializeToString... ", end="")
    model_proto = onnx.load(model_path)
    session_proto = ort.InferenceSession(
        model_proto.SerializeToString(), providers=["CPUExecutionProvider"]
    )
    print("OK")

    # --- Print model metadata ---
    print(f"\n  Inputs:")
    for inp in session_file.get_inputs():
        print(f"    {inp.name}: {inp.type} {inp.shape}")

    print(f"  Outputs:")
    for out in session_file.get_outputs():
        print(f"    {out.name}: {out.type} {out.shape}")

    # --- Run inference (file-loaded session) ---
    print(f"\n  Running inference (file-loaded)... ", end="")
    results_file = session_file.run(expected_output_names, input_dict)
    print("OK")

    # --- Run inference (memory-loaded session) ---
    print(f"  Running inference (memory-loaded)... ", end="")
    results_mem = session_mem.run(expected_output_names, input_dict)
    print("OK")

    # --- Verify output shapes ---
    print(f"\n  Output verification:")
    all_ok = True
    for i, (name, expected_shape) in enumerate(
        zip(expected_output_names, expected_output_shapes)
    ):
        actual_shape = results_file[i].shape
        shape_ok = list(actual_shape) == list(expected_shape)
        status = "PASS" if shape_ok else "FAIL"

        print(f"    {name}: expected {expected_shape}, got {list(actual_shape)} [{status}]")
        print(f"      values (first 5): {results_file[i].flatten()[:5]}")
        print(f"      min={results_file[i].min():.6f}, max={results_file[i].max():.6f}")

        if not shape_ok:
            all_ok = False

    # --- Verify determinism (file vs memory) ---
    print(f"\n  Determinism check (file vs memory): ", end="")
    deterministic = all(
        np.allclose(rf, rm) for rf, rm in zip(results_file, results_mem)
    )
    print("PASS" if deterministic else "FAIL")

    if not all_ok:
        print(f"\n  *** VERIFICATION FAILED ***")
        return False

    print(f"\n  All checks passed.")
    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    # Determine output directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)

    # Output to plugins/O-Texture/Resources/models/placeholder/
    output_dir = os.path.join(
        project_root, "plugins", "O-Texture", "Resources", "models", "placeholder"
    )
    os.makedirs(output_dir, exist_ok=True)

    print("=" * 60)
    print("O-Texture Placeholder ONNX Model Generator")
    print("=" * 60)
    print(f"Output directory: {output_dir}")
    print(f"ONNX opset version: {OPSET_VERSION}")
    print(f"Latent dimensions: {LATENT_DIM}")
    print(f"Audio block size: {AUDIO_SAMPLES} samples")
    print(f"Prior sequence length: {PRIOR_SEQ_LEN}")
    print()

    # -----------------------------------------------------------------------
    # Generate models
    # -----------------------------------------------------------------------

    models = {
        "decoder.onnx": create_decoder_model(),
        "encoder.onnx": create_encoder_model(),
        "prior.onnx": create_prior_model(),
    }

    for filename, model in models.items():
        path = os.path.join(output_dir, filename)
        onnx.save(model, path)
        size = os.path.getsize(path)
        print(f"  Created {filename}: {size:,} bytes ({size / 1024:.1f} KB)")

    total_size = sum(
        os.path.getsize(os.path.join(output_dir, f)) for f in models
    )
    print(f"\n  Total size: {total_size:,} bytes ({total_size / 1024:.1f} KB)")

    # -----------------------------------------------------------------------
    # Verify models with ONNX Runtime
    # -----------------------------------------------------------------------

    try:
        import onnxruntime as ort

        print(f"\n  ONNX Runtime version: {ort.__version__}")
        print(f"  ONNX version: {onnx.__version__}")
        print(f"  Available providers: {ort.get_available_providers()}")
    except ImportError:
        print("\n  WARNING: onnxruntime not installed. Skipping verification.")
        print("  Install with: pip install onnxruntime")
        return

    rng = np.random.RandomState(SEED)
    all_passed = True

    # --- Decoder verification ---
    decoder_input = rng.randn(1, LATENT_DIM).astype(np.float32)
    ok = verify_model(
        model_path=os.path.join(output_dir, "decoder.onnx"),
        input_dict={"z": decoder_input},
        expected_output_names=["audio"],
        expected_output_shapes=[[1, 1, AUDIO_SAMPLES]],
    )
    all_passed &= ok

    # --- Encoder verification ---
    encoder_input = rng.randn(1, 1, AUDIO_SAMPLES).astype(np.float32)
    ok = verify_model(
        model_path=os.path.join(output_dir, "encoder.onnx"),
        input_dict={"audio": encoder_input},
        expected_output_names=["mu", "logvar"],
        expected_output_shapes=[[1, LATENT_DIM], [1, LATENT_DIM]],
    )
    all_passed &= ok

    # --- Prior verification ---
    prior_input = rng.randn(1, PRIOR_SEQ_LEN, LATENT_DIM).astype(np.float32)
    ok = verify_model(
        model_path=os.path.join(output_dir, "prior.onnx"),
        input_dict={"latent_sequence": prior_input},
        expected_output_names=["mu", "logvar"],
        expected_output_shapes=[[1, LATENT_DIM], [1, LATENT_DIM]],
    )
    all_passed &= ok

    # -----------------------------------------------------------------------
    # Summary
    # -----------------------------------------------------------------------

    print(f"\n{'='*60}")
    if all_passed:
        print("ALL MODELS VERIFIED SUCCESSFULLY")
    else:
        print("SOME VERIFICATIONS FAILED")
        sys.exit(1)
    print(f"{'='*60}")

    print(f"\nModels saved to: {output_dir}")
    print(f"  decoder.onnx  - [1, 32] -> [1, 1, 4096]")
    print(f"  encoder.onnx  - [1, 1, 4096] -> [1, 32] (mu) + [1, 32] (logvar)")
    print(f"  prior.onnx    - [1, 16, 32] -> [1, 32] (mu) + [1, 32] (logvar)")
    print()
    print("These placeholder models can be embedded via juce_add_binary_data")
    print("and loaded by ONNX Runtime from memory using:")
    print("  ort::InferenceSession(env, data_ptr, data_size, session_options)")


if __name__ == "__main__":
    main()
