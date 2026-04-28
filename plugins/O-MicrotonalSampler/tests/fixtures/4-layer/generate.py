#!/usr/bin/env python3
"""
Generate Phase 2.3 Gate 3 test fixture: 4-velocity-layer C4 sine set.

Outputs (alongside this script):
    C4_v1.wav  amp=0.25
    C4_v2.wav  amp=0.5
    C4_v3.wav  amp=0.75
    C4_v4.wav  amp=1.0

Each is a 1-second 440 Hz sine, stereo, 48 kHz, 24-bit PCM. The
FilenameParser maps "C4_vN" to (midiNote=60, velocityLayer=N-1). The four
layers therefore differ only in amplitude — this lets a velocity sweep make
the equal-power crossfade audibly obvious (the sum of two adjacent layers at
0.707 each gives a known-amplitude middle value).

Note: 440 Hz is the ET frequency of A4 (MIDI 69), not C4 (MIDI 60). This is
intentional — the loader records `slot.midiNote = 60` (from the filename),
and the voice computes playRate so that "play C4" reproduces the recorded
audio at the C4 microtonal frequency. The audible pitch when triggered at
C4 will therefore be C4 (~261.63 Hz), NOT 440 Hz; the source content is just
a convenient reference tone whose absolute frequency is irrelevant for the
crossfade test (only the amplitude per layer matters).

Run:
    python3 generate.py

Requires: numpy, soundfile (`pip install numpy soundfile`).
"""

from pathlib import Path
import numpy as np
import soundfile as sf

OUT_DIR = Path(__file__).parent
SAMPLE_RATE = 48_000
DURATION_S = 1.0
TONE_HZ = 440.0
AMPLITUDES = [0.25, 0.5, 0.75, 1.0]


def main() -> None:
    n = int(SAMPLE_RATE * DURATION_S)
    t = np.arange(n) / SAMPLE_RATE
    sine = np.sin(2.0 * np.pi * TONE_HZ * t).astype(np.float32)

    # 5 ms cosine fade in/out to keep the burst itself click-free; the voice's
    # ADSR will further shape on playback, but a clean source matters.
    fade = max(1, int(0.005 * SAMPLE_RATE))
    window = np.ones(n, dtype=np.float32)
    if 2 * fade < n:
        ramp = 0.5 - 0.5 * np.cos(np.pi * np.arange(fade) / fade)
        window[:fade] = ramp.astype(np.float32)
        window[-fade:] = ramp[::-1].astype(np.float32)
    sine *= window

    for i, amp in enumerate(AMPLITUDES, start=1):
        mono = (sine * amp).astype(np.float32)
        stereo = np.stack([mono, mono], axis=1)
        out_path = OUT_DIR / f"C4_v{i}.wav"
        sf.write(str(out_path), stereo, SAMPLE_RATE, subtype="PCM_24")
        print(f"wrote {out_path.name} amp={amp}")


if __name__ == "__main__":
    main()
