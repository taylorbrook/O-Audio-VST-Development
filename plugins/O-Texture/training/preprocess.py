"""
O-Texture Audio Preprocessing Pipeline
Resample to 48kHz mono, peak normalize, remove silence, quality report.
"""

import torch
import torchaudio
from pathlib import Path
import argparse
import config


def remove_silence(waveform, threshold_db=config.SILENCE_THRESHOLD_DB,
                   frame_length=config.SILENCE_FRAME_LENGTH,
                   hop_length=config.SILENCE_HOP_LENGTH):
    """Remove segments below threshold from audio using energy-based VAD."""
    if waveform.shape[1] < frame_length:
        return waveform

    frames = waveform.unfold(1, frame_length, hop_length)
    energy = frames.pow(2).mean(dim=-1)
    energy_db = 10 * torch.log10(energy + 1e-10)

    active = energy_db > threshold_db

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
        return waveform

    return torch.cat(active_segments, dim=1)


def preprocess_file(filepath, output_dir):
    """Preprocess a single audio file."""
    try:
        waveform, sr = torchaudio.load(str(filepath))
    except Exception as e:
        print(f"  SKIP {filepath.name}: failed to load ({e})")
        return None

    # Convert to mono
    if waveform.shape[0] > 1:
        waveform = waveform.mean(dim=0, keepdim=True)

    # Resample to 48kHz
    if sr != config.SAMPLE_RATE:
        resampler = torchaudio.transforms.Resample(sr, config.SAMPLE_RATE)
        waveform = resampler(waveform)

    # Remove silence
    waveform = remove_silence(waveform)

    if waveform.shape[1] < config.BLOCK_SIZE:
        print(f"  SKIP {filepath.name}: too short after silence removal "
              f"({waveform.shape[1]} samples)")
        return None

    # Peak normalize to [-1, 1]
    peak = waveform.abs().max()
    if peak > 0:
        waveform = waveform / peak

    # Save
    out_path = Path(output_dir) / f"{filepath.stem}_48k.wav"
    torchaudio.save(str(out_path), waveform, config.SAMPLE_RATE)

    duration = waveform.shape[1] / config.SAMPLE_RATE
    return duration


def preprocess_dataset(input_dir=None, output_dir=None):
    """Preprocess all audio files in input directory."""
    input_dir = Path(input_dir or config.DATA_RAW_DIR)
    output_dir = Path(output_dir or config.DATA_PROCESSED_DIR)
    output_dir.mkdir(parents=True, exist_ok=True)

    extensions = {'.wav', '.flac', '.mp3', '.ogg', '.aiff', '.aif'}
    files = [f for f in input_dir.iterdir()
             if f.suffix.lower() in extensions]

    if not files:
        print(f"No audio files found in {input_dir}")
        return

    print(f"Processing {len(files)} files from {input_dir}")
    print(f"Output: {output_dir}")
    print()

    total_duration = 0.0
    processed = 0

    for filepath in sorted(files):
        duration = preprocess_file(filepath, output_dir)
        if duration is not None:
            total_duration += duration
            processed += 1
            print(f"  OK {filepath.name}: {duration:.1f}s")

    print()
    print(f"Processed: {processed}/{len(files)} files")
    print(f"Total duration: {total_duration:.1f}s ({total_duration/60:.1f} min)")
    print(f"Output: {output_dir}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Preprocess rain audio for training")
    parser.add_argument("--input", type=str, default=None,
                        help="Input directory (default: training/data/raw/)")
    parser.add_argument("--output", type=str, default=None,
                        help="Output directory (default: training/data/processed/)")
    args = parser.parse_args()

    preprocess_dataset(args.input, args.output)
