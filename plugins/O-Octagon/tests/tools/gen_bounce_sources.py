#!/usr/bin/env python3
# This file is part of O-Octagon, an Ouaricon Audio plugin.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
Source material for Phase 4.2's CR-a / CR-b bounce-order pair and for CS, the LFE test.

Emits, as 24-bit integer mono PCM WAV (Execution Constraint 4 — Python's `wave` rejects 32-bit
float, RESEARCH-4.2 N12):

  tone-1.wav .. tone-8.wav   the eight bin-exact tones, one per track.  Tone k goes on track k;
                             analyse_bounce.py --mode order then reports which bounce channel
                             carries it.
  lfe-multitone.wav          ten log-spaced partials bracketing any plausible bass-management
                             crossover, Schroeder-phased so the composite crest factor is low
                             enough to sit at -12 dBFS peak without any partial being small.
  lfe-multitone.txt          the SIDECAR.  Per-partial level, written at generation time.

═══ WHY THE SIDECAR EXISTS ═════════════════════════════════════════════════════════════════════════
PLAN-4.2 Task 4: *per-partial level written into a sidecar .txt so the expected deltas are derivable
rather than remembered.*  CS compares an LFE slot against a reference slot; the delta column is
(measured - reference) and is self-normalising, but the ABSOLUTE per-partial levels are what let a
second person confirm the reference channel is intact rather than merely equal to a broken
measurement.  Two equal silences also have a delta of zero.

═══ WHY THE TONES ARE THESE TONES ══════════════════════════════════════════════════════════════════
997, 1499, 2003, 2503, 3001, 3499, 4001, 4507 Hz.  Analysed at N = fs the bin width is exactly
1.000 Hz, so every one of these integer frequencies is BIN-EXACT: no leakage, and no windowing
choice enters the measurement.  They are mutually non-harmonic, so a tone cannot be mistaken for
another tone's distortion product in the 8 x 8 matrix.

═══ DETERMINISM ════════════════════════════════════════════════════════════════════════════════════
No RNG anywhere — not even a seeded one.  Every sample is a closed-form sum, so re-running this
script produces BYTE-IDENTICAL files on any interpreter, which is what lets the committed evidence
and the committed sources be checked against each other later.  Rounding to 24-bit is plain
round-half-away-from-zero with a clamp; no dither, because a deterministic file matters more here
than a -124 dBFS quantisation floor that nothing in this phase measures.

Stdlib only, like every other script in tests/tools/.
"""

import argparse
import math
import os
import struct
import sys
import wave

TONES = [997, 1499, 2003, 2503, 3001, 3499, 4001, 4507]
PARTIALS = [31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]

TONE_PEAK_DBFS = -20.0
LFE_PEAK_DBFS = -12.0

FS_DEFAULT = 48000
DURATION_DEFAULT = 10.0


def write_wav24(path, samples, fs):
    """24-bit integer mono PCM.  The three-byte pack is done in bulk through struct rather than a
    per-sample Python loop: pack as 32-bit, then take the low three bytes of each word."""
    limit = 8388607
    ints = []
    for value in samples:
        scaled = int(math.floor(value * 8388608.0 + 0.5)) if value >= 0 \
            else -int(math.floor(-value * 8388608.0 + 0.5))
        ints.append(max(-limit - 1, min(limit, scaled)))

    packed = struct.pack("<%di" % len(ints), *ints)
    body = bytearray(len(ints) * 3)
    body[0::3] = packed[0::4]
    body[1::3] = packed[1::4]
    body[2::3] = packed[2::4]

    with wave.open(path, "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(3)
        handle.setframerate(fs)
        handle.writeframes(bytes(body))


def peak_of(samples):
    return max(abs(min(samples)), abs(max(samples))) if samples else 0.0


def rms_of(samples):
    if not samples:
        return 0.0
    return math.sqrt(sum(v * v for v in samples) / len(samples))


def db(amplitude):
    return -240.0 if amplitude <= 1e-12 else 20.0 * math.log10(amplitude)


def make_tone(freq, fs, duration, peak_dbfs):
    """A sine at exactly `freq`, with 20 ms raised-cosine fades so a bounce cannot clip on the
    first sample and so the file is safe to loop in the arrangement."""
    n = int(round(fs * duration))
    amplitude = 10.0 ** (peak_dbfs / 20.0)
    fade = int(round(0.020 * fs))
    omega = 2.0 * math.pi * freq / fs

    out = []
    for i in range(n):
        gain = 1.0
        if i < fade:
            gain = 0.5 - 0.5 * math.cos(math.pi * i / fade)
        elif i >= n - fade:
            gain = 0.5 - 0.5 * math.cos(math.pi * (n - 1 - i) / fade)
        out.append(amplitude * gain * math.sin(omega * i))
    return out


def make_multitone(partials, fs, duration, peak_dbfs):
    """Equal-amplitude partials with SCHROEDER phases.

    phi_n = -pi * n * (n - 1) / N spreads the partials' phase so the composite crest factor is near
    the theoretical minimum.  That matters: with all phases at zero the ten partials would sum to a
    single tall spike, and normalising THAT to -12 dBFS would leave every individual partial ~20 dB
    lower than it needs to be, pushing the 16 kHz partial toward the presence floor in a channel
    that may legitimately be low-passed.
    """
    n = int(round(fs * duration))
    count = len(partials)
    phases = [-math.pi * (k + 1) * k / count for k in range(count)]
    fade = int(round(0.020 * fs))

    raw = []
    for i in range(n):
        acc = 0.0
        for idx, freq in enumerate(partials):
            acc += math.sin(2.0 * math.pi * freq * i / fs + phases[idx])
        raw.append(acc)

    target = 10.0 ** (peak_dbfs / 20.0)
    scale = target / peak_of(raw) if peak_of(raw) > 0 else 0.0

    out = []
    for i, value in enumerate(raw):
        gain = 1.0
        if i < fade:
            gain = 0.5 - 0.5 * math.cos(math.pi * i / fade)
        elif i >= n - fade:
            gain = 0.5 - 0.5 * math.cos(math.pi * (n - 1 - i) / fade)
        out.append(value * scale * gain)

    # Every partial carries the same linear amplitude `scale` (they were unit-amplitude before
    # scaling), so the per-partial level is derivable exactly rather than measured back off.
    return out, scale, phases


def main():
    parser = argparse.ArgumentParser(
        description="Generate O-Octagon Phase 4.2 bounce source material (CR-a / CR-b / CS).")
    parser.add_argument("--output", default=".", help="output directory (default: cwd)")
    parser.add_argument("--fs", type=int, default=FS_DEFAULT,
                        help="sample rate (default %d)" % FS_DEFAULT)
    parser.add_argument("--duration", type=float, default=DURATION_DEFAULT,
                        help="seconds (default %.1f)" % DURATION_DEFAULT)
    args = parser.parse_args()

    nyquist = args.fs / 2.0
    above = [p for p in PARTIALS if p >= nyquist]
    if above:
        sys.stderr.write("gen_bounce_sources.py: FAILED — partials %s are at or above Nyquist "
                         "(%.0f Hz) at fs %d. Generating them would alias and the LFE table would "
                         "measure the alias.\n"
                         % (",".join(str(p) for p in above), nyquist, args.fs))
        sys.exit(1)

    os.makedirs(args.output, exist_ok=True)

    print("── gen_bounce_sources ────────────────────────────────────────────────────")
    print("  fs %d Hz, %.1f s, 24-bit integer mono PCM" % (args.fs, args.duration))
    print("")
    print("  tone    freq Hz    peak dBFS    rms dBFS    file")
    for index, freq in enumerate(TONES):
        samples = make_tone(freq, args.fs, args.duration, TONE_PEAK_DBFS)
        path = os.path.join(args.output, "tone-%d.wav" % (index + 1))
        write_wav24(path, samples, args.fs)
        print("  %4d    %7d    %9.2f    %8.2f    %s"
              % (index + 1, freq, db(peak_of(samples)), db(rms_of(samples)),
                 os.path.basename(path)))

    print("")
    samples, per_partial_amp, phases = make_multitone(PARTIALS, args.fs, args.duration,
                                                      LFE_PEAK_DBFS)
    lfe_path = os.path.join(args.output, "lfe-multitone.wav")
    write_wav24(lfe_path, samples, args.fs)

    composite_peak = db(peak_of(samples))
    composite_rms = db(rms_of(samples))
    per_partial_db = db(per_partial_amp)
    crest = composite_peak - composite_rms

    print("  lfe-multitone.wav — %d partials, Schroeder-phased" % len(PARTIALS))
    print("  composite peak %.2f dBFS, rms %.2f dBFS, crest factor %.2f dB"
          % (composite_peak, composite_rms, crest))
    print("  per-partial level %.2f dBFS (every partial equal by construction)" % per_partial_db)

    sidecar = os.path.join(args.output, "lfe-multitone.txt")
    with open(sidecar, "w", encoding="utf-8") as handle:
        handle.write("O-Octagon Phase 4.2 — LFE multitone source, per-partial levels\n")
        handle.write("Generated by tests/tools/gen_bounce_sources.py (no RNG; byte-identical on "
                     "re-run)\n")
        handle.write("\n")
        handle.write("sample rate      : %d Hz\n" % args.fs)
        handle.write("duration         : %.3f s\n" % args.duration)
        handle.write("format           : 24-bit integer mono PCM\n")
        handle.write("composite peak   : %.2f dBFS  (target %.1f)\n"
                     % (composite_peak, LFE_PEAK_DBFS))
        handle.write("composite rms    : %.2f dBFS\n" % composite_rms)
        handle.write("crest factor     : %.2f dB\n" % crest)
        handle.write("\n")
        handle.write("Every partial carries the SAME linear amplitude by construction: the "
                     "partials are\n")
        handle.write("summed at unit amplitude and the composite is then scaled once. So the "
                     "expected\n")
        handle.write("per-partial level is derivable, not remembered:\n")
        handle.write("\n")
        handle.write("  partial Hz    amplitude (linear)    level dBFS    phase rad\n")
        for freq, phase in zip(PARTIALS, phases):
            handle.write("  %10d    %18.9f    %10.2f    %9.4f\n"
                         % (freq, per_partial_amp, per_partial_db, phase))
        handle.write("\n")
        handle.write("How CS reads against this:\n")
        handle.write("  analyse_bounce.py --mode lfe --channels 4,1 prints (measured - "
                     "reference) per partial.\n")
        handle.write("  That delta is self-normalising, but the ABSOLUTE level of the REFERENCE "
                     "channel should\n")
        handle.write("  land near %.2f dBFS plus whatever gain the signal path applies. Two equal "
                     "SILENCES also\n" % per_partial_db)
        handle.write("  have a delta of zero, which is why the analyser requires every partial to "
                     "be present in\n")
        handle.write("  the reference channel before it will print a verdict "
                     "(PLAN-4.2 P105 clause 5).\n")

    print("  sidecar: %s" % os.path.basename(sidecar))
    print("")
    print("gen_bounce_sources OK — %d tones + 1 multitone + sidecar in %s"
          % (len(TONES), os.path.abspath(args.output)))


if __name__ == "__main__":
    main()
