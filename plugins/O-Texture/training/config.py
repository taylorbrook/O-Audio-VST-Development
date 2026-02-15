"""
O-Texture Training Configuration
All hyperparameters, paths, and constants for Rain VAE + Prior training.
"""

from pathlib import Path

# =============================================================================
# Paths
# =============================================================================
PROJECT_ROOT = Path(__file__).parent
DATA_RAW_DIR = PROJECT_ROOT / "data" / "raw"
DATA_PROCESSED_DIR = PROJECT_ROOT / "data" / "processed"
CHECKPOINT_DIR = PROJECT_ROOT / "checkpoints"
OUTPUT_DIR = PROJECT_ROOT / "output"

# Final model output (committed to repo)
MODELS_DIR = PROJECT_ROOT.parent / "Resources" / "models" / "rain"

# =============================================================================
# Audio
# =============================================================================
SAMPLE_RATE = 48000
BLOCK_SIZE = 4096          # Decoder output size (samples)
HOP_SIZE = 2048            # 50% overlap
NUM_CHANNELS_MONO = 1

# =============================================================================
# VAE Architecture
# =============================================================================
LATENT_DIM = 32

# Encoder: Conv1D stack
ENCODER_CHANNELS = [1, 16, 32, 64, 128, 256, 256]
ENCODER_KERNELS  = [7,  7,  7,  7,   7,   3]
ENCODER_STRIDES  = [2,  2,  2,  2,   2,   2]
ENCODER_POOL_SIZE = 16  # AdaptiveAvgPool1d output

# Decoder: ConvTranspose1D stack
DECODER_FC_OUT_CHANNELS = 256
DECODER_FC_OUT_LENGTH = 16
DECODER_CHANNELS = [256, 128, 64, 32, 16]
DECODER_KERNELS  = [8,    8,   8,  8]
DECODER_STRIDES  = [4,    4,   4,  4]
DECODER_FINAL_KERNEL = 7

# =============================================================================
# VAE Training
# =============================================================================
VAE_BATCH_SIZE = 32
VAE_LEARNING_RATE = 1e-4
VAE_WEIGHT_DECAY = 1e-5
VAE_TOTAL_STEPS = 200_000
VAE_WARMUP_STEPS = 1000
VAE_LR_MIN = 1e-6
VAE_GRAD_CLIP = 1.0

# KL annealing (cyclical)
KL_NUM_CYCLES = 4
KL_RATIO = 0.5            # Proportion of each cycle spent ramping
KL_BETA_MAX = 0.001

# Loss weights
SPECTRAL_FFT_SIZES = [2048, 1024, 512, 256]
WAVEFORM_L1_WEIGHT = 0.1

# Checkpointing
SAVE_EVERY_STEPS = 10_000
LOG_EVERY_STEPS = 100

# =============================================================================
# Prior (GRU)
# =============================================================================
PRIOR_HIDDEN_DIM = 128
PRIOR_NUM_LAYERS = 2
PRIOR_SEQ_LEN = 32
PRIOR_BATCH_SIZE = 32
PRIOR_LEARNING_RATE = 1e-3
PRIOR_EPOCHS = 200
PRIOR_LR_MIN = 1e-5
PRIOR_GRAD_CLIP = 1.0

# Generation
DEFAULT_TEMPERATURE = 0.8

# =============================================================================
# ONNX Export
# =============================================================================
ONNX_OPSET = 17

# =============================================================================
# Preprocessing
# =============================================================================
SILENCE_THRESHOLD_DB = -40
SILENCE_FRAME_LENGTH = 4096
SILENCE_HOP_LENGTH = 2048
