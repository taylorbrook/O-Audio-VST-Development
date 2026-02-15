"""
O-Texture Dataset
TextureDataset with random 4096-sample cropping for VAE training.
"""

import torch
from torch.utils.data import Dataset
import torchaudio
from pathlib import Path
import config


class TextureDataset(Dataset):
    """
    Loads preprocessed audio files and returns random 4096-sample crops.
    Each file can be cropped multiple times per epoch (virtual dataset size).
    """

    def __init__(self, data_dir=None, block_size=config.BLOCK_SIZE,
                 crops_per_file=200):
        super().__init__()
        self.block_size = block_size
        self.crops_per_file = crops_per_file

        data_dir = Path(data_dir or config.DATA_PROCESSED_DIR)
        self.files = sorted(data_dir.glob("*.wav"))

        if len(self.files) == 0:
            raise FileNotFoundError(
                f"No .wav files found in {data_dir}. "
                f"Run preprocess.py first."
            )

        # Pre-load all audio into memory (30 min mono 48kHz = ~350MB float32)
        self.waveforms = []
        total_duration = 0.0

        for filepath in self.files:
            waveform, sr = torchaudio.load(str(filepath))
            if sr != config.SAMPLE_RATE:
                waveform = torchaudio.transforms.Resample(sr, config.SAMPLE_RATE)(waveform)
            if waveform.shape[0] > 1:
                waveform = waveform.mean(dim=0, keepdim=True)

            # Only keep files long enough for at least one crop
            if waveform.shape[1] >= block_size:
                self.waveforms.append(waveform)
                total_duration += waveform.shape[1] / config.SAMPLE_RATE

        print(f"Loaded {len(self.waveforms)} files, "
              f"{total_duration:.1f}s total, "
              f"{len(self)} virtual samples")

        # Build file-level weights for proportional sampling
        lengths = [w.shape[1] for w in self.waveforms]
        total = sum(lengths)
        self.weights = [l / total for l in lengths]

    def __len__(self):
        return len(self.waveforms) * self.crops_per_file

    def __getitem__(self, idx):
        # Map virtual index to file index
        file_idx = idx % len(self.waveforms)
        waveform = self.waveforms[file_idx]

        # Random crop
        max_start = waveform.shape[1] - self.block_size
        start = torch.randint(0, max_start + 1, (1,)).item()
        crop = waveform[:, start:start + self.block_size]

        return crop  # (1, 4096)
