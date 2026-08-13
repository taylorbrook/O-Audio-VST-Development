#!/usr/bin/env python3
# This file is part of O-Octagon, an Ouaricon Audio plugin.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
The CU source — Phase 4.2's audible clause, QUAL-01 criterion 2.

CU asks whether the hull-crossing discontinuity is audible.  RESEARCH-2.3 measured it as a
one-sample step of roughly 15 % of an 8 kHz component.  That is a signal-dependent artifact: it is
inaudible on material with no energy up there and it is masked on material that is dense and loud.
So CU runs on TWO sources, and this script emits the FIRST of them:

  audible-probe.wav   the LOCATOR.  A deterministic bright signal, engineered so that if the step
                      is ever audible it is audible here.  Half 1 of D12 — the difference signal,
                      soloed: *is there a step, and where?*

The SECOND source is ecological and is NOT generated or committed: one bright commercial track or
Apple Loop, NAMED in VERIFICATION-4.2.md (licence — D12's stated residual).  Half 2 is the actual
requirement; half 1 only locates the artifact so half 2 knows where to listen.

**Audibility in half 1 does NOT mean audibility in context.**  That sentence belongs in the artifact,
because a signal built to expose an artifact will expose it.

═══ WHAT IS IN THE SIGNAL, AND WHY ═════════════════════════════════════════════════════════════════
  * SUSTAINED 6-16 kHz — six mutually inharmonic partials, Schroeder-phased.  This is the band the
    step actually lives in, and inharmonic spacing means the ear hears a bright wash rather than a
    pitch, so a click is not mistaken for a beat between partials.
  * TRANSIENTS — a short, fast-decaying burst of the same stack every 0.5 s at higher level.  A
    steady wash MASKS a one-sample step; the quiet tail after each transient is where a step is
    most exposed.  Having both in one file means one render answers both cases.
  * NO low end at all.  Anything below 6 kHz only adds masking energy and loudness without adding
    any chance of hearing a 8 kHz-band discontinuity.

═══ WHY airAmount IS NOT ZEROED FOR CU ═════════════════════════════════════════════════════════════
PLAN-4.2 Execution Constraint 1: airAmount = 0 on CR-a, CR-b, CT and CS — and NEVER on CU.  The air
filter is position-dependent (HullProcessor.h:123) and is the artifact under test here.  Zeroing it
for CU, by reflex from the other four tests, would erase the measurement.

═══ DETERMINISM IS THE POINT ═══════════════════════════════════════════════════════════════════════
CU's method is a DIFFERENCE SIGNAL between two renders that differ only in the gesture.  If the
source were noise from a seeded RNG the difference would still work, but a re-run on another machine
or interpreter could produce a different file and the measurement could not be reproduced.  Every
sample below is a closed-form sum: no RNG, not even a seeded one.  Byte-identical on re-run, which
is exactly what makes the difference signal exact rather than approximate.

Stdlib only.
"""

import argparse
import math
import os
import struct
import wave

#: Mutually inharmonic, spanning the band where the hull-crossing step lives.
PARTIALS = [6301.0, 7919.0, 9677.0, 11311.0, 13109.0, 15101.0]

PEAK_DBFS = -12.0
SUSTAIN_FRACTION = 0.35   # sustained wash level, relative to the transient peak
TRANSIENT_PERIOD_S = 0.5
TRANSIENT_DECAY_S = 0.030
FS_DEFAULT = 48000
DURATION_DEFAULT = 20.0


def write_wav24(path, samples, fs):
    """24-bit integer mono PCM; bulk three-byte pack.  Duplicated from gen_bounce_sources.py on
    purpose — every script in tests/tools/ runs standalone on a bare interpreter."""
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


def db(amplitude):
    return -240.0 if amplitude <= 1e-12 else 20.0 * math.log10(amplitude)


def main():
    parser = argparse.ArgumentParser(
        description="Generate the CU locator source — Phase 4.2's audible clause.")
    parser.add_argument("--output", default=".", help="output directory (default: cwd)")
    parser.add_argument("--fs", type=int, default=FS_DEFAULT,
                        help="sample rate (default %d)" % FS_DEFAULT)
    parser.add_argument("--duration", type=float, default=DURATION_DEFAULT,
                        help="seconds (default %.1f)" % DURATION_DEFAULT)
    args = parser.parse_args()

    fs = args.fs
    n = int(round(fs * args.duration))
    count = len(PARTIALS)
    phases = [-math.pi * (k + 1) * k / count for k in range(count)]

    nyquist = fs / 2.0
    live = [p for p in PARTIALS if p < nyquist]
    if len(live) != count:
        raise SystemExit("gen_audible_source.py: FAILED — partials above Nyquist at fs %d. "
                         "The locator needs its whole 6-16 kHz stack." % fs)

    period = int(round(TRANSIENT_PERIOD_S * fs))
    decay = TRANSIENT_DECAY_S * fs
    fade = int(round(0.020 * fs))

    raw = []
    for i in range(n):
        stack = 0.0
        for idx, freq in enumerate(PARTIALS):
            stack += math.sin(2.0 * math.pi * freq * i / fs + phases[idx])
        stack /= count

        # A fast-decaying burst every TRANSIENT_PERIOD_S, riding on a steady wash. The quiet tail
        # after each burst is where a one-sample step is least masked.
        since = i % period
        transient = math.exp(-since / decay)
        envelope = SUSTAIN_FRACTION + (1.0 - SUSTAIN_FRACTION) * transient

        raw.append(stack * envelope)

    largest = max(abs(min(raw)), abs(max(raw)))
    scale = (10.0 ** (PEAK_DBFS / 20.0)) / largest if largest > 0 else 0.0

    out = []
    for i, value in enumerate(raw):
        gain = 1.0
        if i < fade:
            gain = 0.5 - 0.5 * math.cos(math.pi * i / fade)
        elif i >= n - fade:
            gain = 0.5 - 0.5 * math.cos(math.pi * (n - 1 - i) / fade)
        out.append(value * scale * gain)

    path = os.path.join(args.output, "audible-probe.wav")
    os.makedirs(args.output, exist_ok=True)
    write_wav24(path, out, fs)

    peak = max(abs(min(out)), abs(max(out)))
    rms = math.sqrt(sum(v * v for v in out) / len(out))

    print("── gen_audible_source ────────────────────────────────────────────────────")
    print("  fs %d Hz, %.1f s, 24-bit integer mono PCM" % (fs, args.duration))
    print("  partials    : %s Hz" % ", ".join("%.0f" % p for p in PARTIALS))
    print("  transients  : every %.2f s, %.0f ms decay" % (TRANSIENT_PERIOD_S,
                                                           TRANSIENT_DECAY_S * 1000))
    print("  sustain     : %.0f %% of transient peak" % (SUSTAIN_FRACTION * 100))
    print("  peak %.2f dBFS, rms %.2f dBFS, crest %.2f dB"
          % (db(peak), db(rms), db(peak) - db(rms)))
    print("  file        : %s" % os.path.basename(path))
    print("")
    print("  REMINDER — PLAN-4.2 Execution Constraint 1: airAmount is NOT zeroed for CU. The air")
    print("  filter is the artifact under test; zeroing it by reflex from CR-a/CR-b/CT/CS would")
    print("  erase the measurement.")
    print("")
    print("  REMINDER — D12: this is the LOCATOR (half 1). Audibility here does NOT mean")
    print("  audibility in context. Half 2 runs on a NAMED ecological source, and the headphones")
    print("  are named in VERIFICATION-4.2.md.")
    print("")
    print("gen_audible_source OK — %s" % os.path.abspath(path))


if __name__ == "__main__":
    main()
