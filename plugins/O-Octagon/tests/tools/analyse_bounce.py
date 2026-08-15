#!/usr/bin/env python3
# This file is part of O-Octagon, an Ouaricon Audio plugin.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
The Phase 4.2 bounce/capture analyser — COMPAT-02 criterion 2, CR-a/CR-b, and the LFE claim.

Four modes, all reading committed WAV artifacts and printing numbers a second person can re-derive:

  probe   channel count / rate / depth / duration / per-channel peak+RMS.  Q2's two-minute
          pre-flight (PLAN-4.2 Gate 12).
  order   the 8 x 8 Goertzel matrix.  Which bounce channel carries tone k, for k = 1..8.
          CR-a establishes Logic's canonical interleaved 7.1 order; CR-b proves the map is
          actually consulted (PLAN-4.2 P106's 8-cycle).
  lfe     the per-partial table for two named channels.  Does Logic touch the LFE slot?
          (VenueModel.cpp:84's claim, PLAN-4.2 P103's verdict table, run on BOTH paths per P111.)
  ping    PLAN-4.2 P102's envelope-segmented window assertion.  Eight 1.6 s windows, exactly one
          channel energised per window, sequence 1..8.  This is COMPAT-02 criterion 2 as SAMPLE
          DATA rather than as eight moving meters.

═══ THE SIX ANTI-VACUITY CLAUSES (PLAN-4.2 P105) ═══════════════════════════════════════════════════
Mirroring gen_dbap_reference.py's stated discipline — *a vacuous oracle is worse than no oracle,
because it reports green*.  These are the tool's contract, and they are enforced HERE rather than
by the operator remembering them at 1 a.m. after a two-hour session:

  1. --expect is MANDATORY in order mode.  There is no default, because the default would be the
     identity and the identity is the answer CR-a already expects.
  2. A CR-b invocation REFUSES an identity --expect.  This is RESEARCH-4.2 N13.  Without it, a CR-b
     run against a venue that silently failed to load reports a clean CR-a pass — the check bypassed
     by the one mistake it exists to catch.
  3. EVERY expected tone must be found somewhere.  A missing tone is a FAILURE, not a 7-of-8 partial
     pass — it means a track was muted or an instance was not one-hot.
  4. Isolation floor, >= 40 dB argmax over runner-up (generous: v_i is EXACTLY 0.0f when w_i == 0).
     The MEASURED MARGIN IS PRINTED, so the number is in the record and not only the threshold.
  5. Non-zero exit on any failure, and NO "OK" line on zero cases analysed.
  6. --check re-runs the committed expectation table against the committed artifacts, so the pair
     stays verifiable after the session.  The gen_dbap_reference.py --check precedent, which is a
     4.1 gate.

═══ WHY 24-BIT INTEGER PCM, NEVER 32-BIT FLOAT (RESEARCH-4.2 N12) ══════════════════════════════════
Python's `wave` accepts only WAVE_FORMAT_PCM and WAVE_FORMAT_EXTENSIBLE (wave.py:386-387).  A
32-bit float bounce is 0x0003 and dies with "unknown format: 3".  8-channel WAV *is* EXTENSIBLE and
reads fine.  `audioop` was removed in 3.13, so 24-bit needs a manual sign-extending unpack — done
below in bulk through struct rather than a per-sample Python loop.

At -20 dBFS the 24-bit floor is ~124 dB down.  Nothing measured here is near it, which is why a
40 dB isolation floor is generous rather than lenient.

═══ WHY ONE-OR-MANY --input (PLAN-4.2 P104) ════════════════════════════════════════════════════════
CT's loopback rig produces EIGHT MONO FILES (Logic records per track); CR/CS produce ONE 8-CHANNEL
FILE.  Rather than adding an export step that could itself re-order channels — the exact class of
thing this phase exists to measure — --input takes one or more paths:

    one file  -> its channels are the channels;
    N files   -> channel k is channel 0 of file k, IN THE ORDER GIVEN ON THE COMMAND LINE.

Six lines here, and it removes a whole re-ordering step from between the host and the evidence.

═══ THE MIRROR IS ASSERTED, NOT TRUSTED ════════════════════════════════════════════════════════════
pattern_test_fixture_mirrors_drift_silently: a fixture that mirrors plugin constants drifts silently
and keeps passing.  So the ping timings are handled three ways, in order of preference:

  1. PARSED from Source/DSP/VerifyPing.h when it is findable (it is, from tests/tools/);
  2. otherwise the mirror below, with a printed WARNING that it is unverified;
  3. and EITHER WAY the actual burst period is MEASURED from the capture's own envelope and
     asserted against the constant.  A disagreement is a real signal — the constant moved, or the
     capture is not what it is labelled.

═══ DETERMINISM ════════════════════════════════════════════════════════════════════════════════════
No wall-clock anywhere in the output (pattern_wallclock_inside_a_stability_verdict).  Identical
input bytes produce identical stdout, which is what makes --check an equality test rather than a
judgement call.

Stdlib only.  All five existing tests/tools/ scripts are; numpy and scipy are installed locally but
adding them would make this the one tool that cannot run on a bare interpreter.  Goertzel is six
lines.
"""

import argparse
import json
import math
import os
import re
import struct
import sys
import wave

# ── Measurement constants ───────────────────────────────────────────────────────────────────────

#: Bin-exact at N = fs (1 Hz bins), so no windowing choice enters the measurement.
TONES_DEFAULT = [997, 1499, 2003, 2503, 3001, 3499, 4001, 4507]

#: Log-spaced and bracketing any plausible bass-management crossover.
PARTIALS_DEFAULT = [31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000]

#: Clause 4.  Generous, given v_i is exactly 0.0f when w_i == 0.
ISOLATION_DB_DEFAULT = 40.0

#: Clause 3's presence test.  Well above the ~-124 dB 24-bit floor, well below any real tone.
PRESENCE_FLOOR_DB_DEFAULT = -60.0

#: Skipped at the head of a bounce so Logic's transport settling never enters a Goertzel window.
SKIP_SECONDS_DEFAULT = 0.5

# ── The VerifyPing.h mirror.  Preference 2 of three; see the module docstring. ───────────────────
PING_MIRROR = {
    "kNumSpeakers": 8,
    "kOnSeconds": 1.2,
    "kGapSeconds": 0.4,
}

#: How far the measured burst period may sit from the constant before it is a finding, in seconds.
PING_PERIOD_TOLERANCE_S = 0.05

#: Guard trimmed off each end of a burst before analysis, so the 20 ms fades never enter an RMS.
PING_EDGE_GUARD_S = 0.10

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_MANIFEST = os.path.normpath(
    os.path.join(_SCRIPT_DIR, "..", "..", ".planning", "stages", "4-polish",
                 "evidence", "bounce-manifest.json"))
VERIFY_PING_HEADER = os.path.normpath(
    os.path.join(_SCRIPT_DIR, "..", "..", "Source", "DSP", "VerifyPing.h"))


def die(message):
    """Clause 5: non-zero exit on any failure, and the reason on stderr."""
    sys.stderr.write("analyse_bounce.py: FAILED — %s\n" % message)
    sys.exit(1)


def db(amplitude):
    """dBFS from a linear amplitude, floored rather than -inf so tables stay readable."""
    if amplitude <= 1e-12:
        return -240.0
    return 20.0 * math.log10(amplitude)


# ═══ WAV reading ════════════════════════════════════════════════════════════════════════════════


def _decode_frames(raw, sampwidth, nchannels):
    """Return (flat_int_samples, full_scale).  Samples stay INTEGER — Goertzel and RMS are linear,
    so scaling once at the end is both faster and exact."""
    if sampwidth == 2:
        count = len(raw) // 2
        return struct.unpack("<%dh" % count, raw[:count * 2]), 32768.0

    if sampwidth == 3:
        # 24-bit little-endian, sign-extended into 4 bytes and then unpacked in ONE struct call.
        # The per-byte work below is over n bytes, not 3n, and everything else is C.
        count = len(raw) // 3
        body = raw[:count * 3]
        widened = bytearray(count * 4)
        widened[0::4] = body[0::3]
        widened[1::4] = body[1::3]
        widened[2::4] = body[2::3]
        widened[3::4] = bytes(0xFF if b & 0x80 else 0x00 for b in body[2::3])
        return struct.unpack("<%di" % count, bytes(widened)), 8388608.0

    if sampwidth == 4:
        count = len(raw) // 4
        return struct.unpack("<%di" % count, raw[:count * 4]), 2147483648.0

    die("unsupported sample width %d bytes. PLAN-4.2 Execution Constraint 4: bounce 24-bit "
        "integer PCM, never 32-bit float." % sampwidth)


def read_inputs(paths):
    """P104's one-or-many rule.

    Returns dict with: channels (list of int-sample sequences), fs, sampwidth_bits, nframes,
    full_scale, source ('single' | 'multi'), per_file (list of (path, nframes)).
    """
    if not paths:
        die("no --input given. Clause 5: nothing analysed is not a pass.")

    opened = []
    for path in paths:
        if not os.path.exists(path):
            die("input does not exist: %s" % path)
        try:
            handle = wave.open(path, "rb")
        except wave.Error as exc:
            # The single most likely cause, named explicitly so the operator does not re-bounce
            # blind at the end of a session.
            die("could not read %s as PCM WAV (%s).\n"
                "         If this says 'unknown format: 3' the bounce is 32-bit float. Re-bounce "
                "as 24-bit integer PCM (RESEARCH-4.2 N12, Execution Constraint 4)." % (path, exc))
        opened.append((path, handle))

    rates = {h.getframerate() for _, h in opened}
    if len(rates) != 1:
        die("inputs disagree on sample rate: %s" % sorted(rates))
    fs = rates.pop()

    widths = {h.getsampwidth() for _, h in opened}
    if len(widths) != 1:
        die("inputs disagree on sample width: %s" % sorted(w * 8 for w in widths))
    sampwidth = widths.pop()

    per_file = [(p, h.getnframes()) for p, h in opened]

    channels = []
    full_scale = 1.0
    if len(opened) == 1:
        path, handle = opened[0]
        nch = handle.getnchannels()
        nframes = handle.getnframes()
        if nch < 1 or nframes < 1:
            die("%s has %d channels and %d frames. Nothing to analyse (clause 5)."
                % (path, nch, nframes))
        flat, full_scale = _decode_frames(handle.readframes(nframes), sampwidth, nch)
        handle.close()
        channels = [flat[c::nch] for c in range(nch)]
        source = "single"
    else:
        # N files -> channel k is channel 0 of file k, in the order given on the command line.
        lengths = [n for _, n in per_file]
        nframes = min(lengths)
        if nframes < 1:
            die("one or more inputs contain zero frames (clause 5).")
        for path, handle in opened:
            nch = handle.getnchannels()
            flat, full_scale = _decode_frames(handle.readframes(handle.getnframes()),
                                              sampwidth, nch)
            handle.close()
            channels.append(flat[0::nch][:nframes] if nch > 1 else flat[:nframes])
        source = "multi"

    return {
        "channels": channels,
        "fs": fs,
        "sampwidth_bits": sampwidth * 8,
        "nframes": nframes,
        "full_scale": full_scale,
        "source": source,
        "per_file": per_file,
        "paths": list(paths),
    }


# ═══ Measurement primitives ═════════════════════════════════════════════════════════════════════


def goertzel(samples, start, length, freq, fs, full_scale):
    """Amplitude of `freq` over samples[start:start+length].

    Called with length == fs, so the bin width is exactly 1 Hz and every integer-Hz tone lands
    bin-exact: no leakage, and no windowing choice to argue about later.
    """
    k = int(round(freq * length / float(fs)))
    omega = 2.0 * math.pi * k / length
    cosw = math.cos(omega)
    sinw = math.sin(omega)
    coeff = 2.0 * cosw

    s1 = 0.0
    s2 = 0.0
    for i in range(start, start + length):
        s0 = samples[i] + coeff * s1 - s2
        s2 = s1
        s1 = s0

    real = s1 - s2 * cosw
    imag = s2 * sinw
    return 2.0 * math.hypot(real, imag) / (length * full_scale)


def rms(samples, start, length, full_scale):
    if length <= 0:
        return 0.0
    total = 0
    for i in range(start, start + length):
        v = samples[i]
        total += v * v
    return math.sqrt(total / float(length)) / full_scale


def peak(samples, full_scale):
    if not samples:
        return 0.0
    return max(abs(min(samples)), abs(max(samples))) / full_scale


def parse_perm(text, n, flag):
    """A permutation of 1..n, validated as one.  A 'permutation' with a repeat is a typo that would
    otherwise read as a real ordering finding."""
    try:
        values = [int(part) for part in text.split(",") if part.strip() != ""]
    except ValueError:
        die("%s must be a comma-separated list of integers, got %r" % (flag, text))
    if len(values) != n:
        die("%s has %d entries but there are %d channels" % (flag, len(values), n))
    if sorted(values) != list(range(1, n + 1)):
        die("%s is not a permutation of 1..%d: %r" % (flag, n, text))
    return values


def is_identity(perm):
    return perm == list(range(1, len(perm) + 1))


# ═══ Ping constants: parsed, else mirrored, and either way asserted ═════════════════════════════


def load_ping_constants():
    """Preference 1 (parse) then 2 (mirror).  Returns (constants, provenance_line)."""
    if os.path.exists(VERIFY_PING_HEADER):
        try:
            with open(VERIFY_PING_HEADER, "r", encoding="utf-8") as handle:
                text = handle.read()
        except OSError:
            text = ""
        found = {}
        patterns = {
            "kNumSpeakers": r"constexpr\s+int\s+kNumSpeakers\s*=\s*([0-9]+)\s*;",
            "kOnSeconds": r"constexpr\s+double\s+kOnSeconds\s*=\s*([0-9.]+)\s*;",
            "kGapSeconds": r"constexpr\s+double\s+kGapSeconds\s*=\s*([0-9.]+)\s*;",
        }
        for name, pattern in patterns.items():
            match = re.search(pattern, text)
            if match:
                found[name] = int(match.group(1)) if name == "kNumSpeakers" \
                    else float(match.group(1))
        if len(found) == len(patterns):
            drifted = [k for k in found if found[k] != PING_MIRROR[k]]
            note = ""
            if drifted:
                note = ("  NOTE: the in-script mirror disagrees on %s — the PARSED value wins."
                        % ", ".join(sorted(drifted)))
            return found, ("ping constants: PARSED from %s%s"
                           % (os.path.relpath(VERIFY_PING_HEADER, os.getcwd()), note))

    return dict(PING_MIRROR), (
        "ping constants: WARNING — VerifyPing.h not parsed, using the in-script mirror. "
        "The mirror is UNVERIFIED (pattern_test_fixture_mirrors_drift_silently); the measured "
        "period assertion below is the only remaining check on it.")


def find_bursts(channels, fs, full_scale):
    """Envelope segmentation.  P102: window boundaries come from the ENERGY ENVELOPE, not from
    sample zero — Logic is recording the device it is playing to, so every file carries the
    round-trip delay.

    Returns a list of (start_frame, end_frame) for each contiguous run above -20 dB from the
    loudest frame.
    """
    hop = max(1, int(round(0.005 * fs)))
    frame = max(hop, int(round(0.010 * fs)))
    nframes = min(len(c) for c in channels)

    energies = []
    positions = []
    pos = 0
    while pos + frame <= nframes:
        total = 0
        for chan in channels:
            for i in range(pos, pos + frame, 4):  # decimated: envelope only, not a measurement
                v = chan[i]
                total += v * v
        energies.append(math.sqrt(total / float(frame)))
        positions.append(pos)
        pos += hop

    if not energies:
        die("capture is shorter than one 10 ms analysis frame.")

    loudest = max(energies)
    if loudest / full_scale < 10 ** (-60.0 / 20.0):
        die("no signal found in the capture — loudest 10 ms frame is %.1f dBFS. "
            "Nothing was recorded (clause 5)." % db(loudest / full_scale))

    threshold = loudest * (10 ** (-20.0 / 20.0))

    bursts = []
    run_start = None
    for idx, energy in enumerate(energies):
        if energy >= threshold and run_start is None:
            run_start = positions[idx]
        elif energy < threshold and run_start is not None:
            bursts.append((run_start, positions[idx] + frame))
            run_start = None
    if run_start is not None:
        bursts.append((run_start, nframes))

    # Bridge runs separated by less than half a gap — pink noise dips below a fixed threshold.
    merged = []
    bridge = int(round(0.15 * fs))
    for start, end in bursts:
        if merged and start - merged[-1][1] < bridge:
            merged[-1] = (merged[-1][0], end)
        else:
            merged.append((start, end))

    # Discard fragments shorter than a third of the shortest plausible burst.
    return [b for b in merged if (b[1] - b[0]) > int(0.3 * fs)]


# ═══ Modes ══════════════════════════════════════════════════════════════════════════════════════


def mode_probe(data, args):
    fs = data["fs"]
    channels = data["channels"]
    duration = data["nframes"] / float(fs)

    print("── probe ─────────────────────────────────────────────────────────────────")
    print("  inputs        : %d (%s)" % (len(data["paths"]), data["source"]))
    for path, nframes in data["per_file"]:
        print("                  %s  (%d frames, %.3f s)"
              % (os.path.basename(path), nframes, nframes / float(fs)))
    print("  channels      : %d" % len(channels))
    print("  sample rate   : %d Hz" % fs)
    print("  bit depth     : %d-bit integer PCM" % data["sampwidth_bits"])
    print("  duration      : %.3f s (%d frames)" % (duration, data["nframes"]))
    print("")
    print("  ch    peak dBFS    rms dBFS")
    for idx, chan in enumerate(channels):
        print("  %2d      %8.2f    %8.2f"
              % (idx + 1, db(peak(chan, data["full_scale"])),
                 db(rms(chan, 0, len(chan), data["full_scale"]))))

    failures = []
    if data["nframes"] < 1:
        failures.append("zero frames")
    if args.expect_channels is not None and len(channels) != args.expect_channels:
        failures.append("expected %d channels, found %d" % (args.expect_channels, len(channels)))
    if args.expect_rate is not None and fs != args.expect_rate:
        failures.append("expected %d Hz, found %d Hz" % (args.expect_rate, fs))
    if args.expect_depth is not None and data["sampwidth_bits"] != args.expect_depth:
        failures.append("expected %d-bit, found %d-bit"
                        % (args.expect_depth, data["sampwidth_bits"]))

    if failures:
        die("probe: " + "; ".join(failures))

    print("")
    print("probe OK — %d channels, %d Hz, %d-bit, %.3f s"
          % (len(channels), fs, data["sampwidth_bits"], duration))
    return {"channels": len(channels), "rate": fs, "depth": data["sampwidth_bits"],
            "duration_s": round(duration, 3)}


def mode_order(data, args):
    fs = data["fs"]
    channels = data["channels"]
    nch = len(channels)
    full_scale = data["full_scale"]

    tones = [int(t) for t in args.tones.split(",")] if args.tones else list(TONES_DEFAULT)
    if len(tones) != nch:
        die("order: %d tones against %d channels. The matrix must be square — one tone per "
            "channel." % (len(tones), nch))

    # ── Clause 1: --expect is mandatory. ───────────────────────────────────────────────────────
    if not args.expect:
        die("order mode requires --expect (P105 clause 1). There is no default, because the "
            "default would be the identity and the identity is CR-a's answer.")
    expect = parse_perm(args.expect, nch, "--expect")

    # ── Clause 2: a CR-b invocation refuses an identity --expect. THIS IS N13. ─────────────────
    if args.label == "CR-b" and is_identity(expect):
        die("--label CR-b with an identity --expect (%s) is REFUSED (P105 clause 2 / N13).\n"
            "         CR-b exists to prove the permuted venue was consulted. Against an identity "
            "expectation, a venue that silently failed to load reports a clean CR-a pass — the "
            "check bypassed by the one mistake it exists to catch.\n"
            "         The 8-cycle expectation is --expect 2,3,4,5,6,7,8,1 (PLAN-4.2 P106)."
            % args.expect)

    skip = int(round(args.skip * fs))
    length = fs  # N = fs -> 1 Hz bins
    if data["nframes"] < skip + length:
        die("order: need %.2f s of audio after a %.2f s skip, have %.3f s."
            % (length / float(fs), args.skip, data["nframes"] / float(fs)))

    print("── order%s ──────────────────────────────────────────────────────────────"
          % ("  [%s]" % args.label if args.label else ""))
    print("  analysis window: %.3f s .. %.3f s   (N = fs = %d, bin width 1.000 Hz)"
          % (skip / float(fs), (skip + length) / float(fs), fs))
    print("  expectation    : %s" % ",".join(str(v) for v in expect))
    print("")

    matrix = []
    for tone in tones:
        row = [db(goertzel(chan, skip, length, tone, fs, full_scale)) for chan in channels]
        matrix.append(row)

    header = "  tone Hz |" + "".join("  ch%-2d " % (c + 1) for c in range(nch))
    print(header)
    print("  " + "-" * (len(header) - 2))
    for tone, row in zip(tones, matrix):
        print("  %7d |" % tone + "".join("%7.1f" % v for v in row))
    print("")

    observed = []
    margins = []
    failures = []

    for idx, (tone, row) in enumerate(zip(tones, matrix)):
        ranked = sorted(range(nch), key=lambda c: row[c], reverse=True)
        best, runner_up = ranked[0], ranked[1]
        margin = row[best] - row[runner_up]
        observed.append(best + 1)
        margins.append(margin)

        # ── Clause 3: every expected tone must be FOUND. Not a 7-of-8 partial pass. ────────────
        if row[best] < args.floor:
            failures.append(
                "tone %d Hz (k=%d) not found: loudest channel is ch%d at %.1f dBFS, below the "
                "%.1f dBFS presence floor. A track was muted or an instance was not one-hot "
                "(clause 3)." % (tone, idx + 1, best + 1, row[best], args.floor))
            continue

        # ── Clause 4: isolation, with the MEASURED MARGIN PRINTED. ────────────────────────────
        if margin < args.isolation:
            failures.append(
                "tone %d Hz (k=%d): isolation %.1f dB (ch%d over ch%d) is below the %.1f dB "
                "floor (clause 4)." % (tone, idx + 1, margin, best + 1, runner_up + 1,
                                       args.isolation))

    print("  k   tone Hz   -> channel   isolation dB")
    for idx, (tone, chan, margin) in enumerate(zip(tones, observed, margins)):
        print("  %d   %7d   ->   ch%-2d      %8.1f" % (idx + 1, tone, chan, margin))
    print("")
    print("  observed: %s" % ",".join(str(v) for v in observed))
    print("  expected: %s" % ",".join(str(v) for v in expect))
    print("  minimum measured isolation: %.1f dB (floor %.1f dB)"
          % (min(margins) if margins else float("nan"), args.isolation))
    print("")

    if observed != expect:
        failures.append("channel order mismatch: observed %s, expected %s"
                        % (",".join(str(v) for v in observed), ",".join(str(v) for v in expect)))

    if failures:
        die("order%s:\n         - %s"
            % (" [%s]" % args.label if args.label else "", "\n         - ".join(failures)))

    print("order OK%s — %s, minimum isolation %.1f dB"
          % (" [%s]" % args.label if args.label else "",
             ",".join(str(v) for v in observed), min(margins)))
    return {"observed": ",".join(str(v) for v in observed),
            "min_isolation_db": round(min(margins), 1),
            "matrix_db": [[round(v, 1) for v in row] for row in matrix]}


def mode_lfe(data, args):
    fs = data["fs"]
    channels = data["channels"]
    nch = len(channels)
    full_scale = data["full_scale"]

    if not args.channels:
        die("lfe mode requires --channels <measured>,<reference> — e.g. --channels 4,1 "
            "(speaker 4 is the LFE slot under the CR-a IDENTITY venue; under CR-b it is not, "
            "PLAN-4.2 P106).")
    try:
        pair = [int(p) for p in args.channels.split(",")]
    except ValueError:
        die("--channels must be two comma-separated 1-based channel numbers")
    if len(pair) != 2:
        die("--channels takes exactly two channels: measured,reference")
    for c in pair:
        if c < 1 or c > nch:
            die("--channels %d is out of range for a %d-channel input" % (c, nch))
    measured_idx, reference_idx = pair[0] - 1, pair[1] - 1

    partials = [int(p) for p in args.partials.split(",")] if args.partials \
        else list(PARTIALS_DEFAULT)

    skip = int(round(args.skip * fs))
    length = fs
    if data["nframes"] < skip + length:
        die("lfe: need %.2f s after a %.2f s skip, have %.3f s."
            % (length / float(fs), args.skip, data["nframes"] / float(fs)))

    nyquist = fs / 2.0
    usable = [p for p in partials if p < nyquist]
    dropped = [p for p in partials if p >= nyquist]

    print("── lfe ───────────────────────────────────────────────────────────────────")
    print("  measured  : ch%d" % pair[0])
    print("  reference : ch%d" % pair[1])
    print("  window    : %.3f s .. %.3f s   (N = fs = %d, bin width 1.000 Hz)"
          % (skip / float(fs), (skip + length) / float(fs), fs))
    if dropped:
        print("  NOTE      : %s Hz are at or above Nyquist (%.0f Hz) and are not measured."
              % (",".join(str(d) for d in dropped), nyquist))
    print("")
    print("  partial Hz    ch%-2d dBFS    ch%-2d dBFS    delta dB" % (pair[0], pair[1]))
    print("  " + "-" * 52)

    rows = []
    missing = []
    for partial in usable:
        a = db(goertzel(channels[measured_idx], skip, length, partial, fs, full_scale))
        b = db(goertzel(channels[reference_idx], skip, length, partial, fs, full_scale))
        rows.append((partial, a, b, a - b))
        print("  %9d    %9.2f    %9.2f    %8.2f" % (partial, a, b, a - b))

        # Anti-vacuity for CS: presence is required in the REFERENCE channel. Requiring it in the
        # MEASURED channel would beg the question — a genuine bass-management low-pass is exactly
        # a missing HF partial there, and asserting it away would erase the finding.
        if b < args.floor:
            missing.append((partial, b))

    broadband_a = rms(channels[measured_idx], skip, length, full_scale)
    broadband_b = rms(channels[reference_idx], skip, length, full_scale)
    print("")
    print("  broadband RMS: ch%d %.2f dBFS   ch%d %.2f dBFS   delta %.2f dB"
          % (pair[0], db(broadband_a), pair[1], db(broadband_b),
             db(broadband_a) - db(broadband_b)))
    print("")

    if missing:
        die("lfe: %d partial(s) absent from the REFERENCE channel ch%d (%s). The multitone did "
            "not reach the plugin, so the delta column measures nothing — a table of equal "
            "silences reads as 'flat' and would confirm VenueModel.cpp:84 on no evidence "
            "(clause 5)."
            % (len(missing), pair[1],
               ", ".join("%d Hz at %.1f dBFS" % (p, v) for p, v in missing)))

    if db(broadband_b) < args.floor:
        die("lfe: reference channel ch%d broadband RMS is %.1f dBFS, below the %.1f dBFS floor. "
            "Nothing played (clause 5)." % (pair[1], db(broadband_b), args.floor))

    # An advisory reading against PLAN-4.2 P103's verdict table. The DISPOSITION is the operator's
    # (D16 is a re-freeze), and NC4 must have been run first — so this classifies and stops.
    deltas = [r[3] for r in rows]
    spread = max(deltas) - min(deltas)
    mean_delta = sum(deltas) / len(deltas)
    hf = [r[3] for r in rows if r[0] >= 1000]
    lf = [r[3] for r in rows if r[0] <= 125]
    print("  ── reading against PLAN-4.2 P103 (advisory — the disposition is the operator's) ──")
    print("  mean delta %.2f dB, spread %.2f dB, LF mean %.2f dB, HF mean %.2f dB"
          % (mean_delta, spread,
             sum(lf) / len(lf) if lf else float("nan"),
             sum(hf) / len(hf) if hf else float("nan")))
    if abs(mean_delta) < 1.0 and spread < 2.0:
        print("  -> row 1: broadband and per-partial deltas both ~0. VenueModel.cpp:84 CONFIRMED "
              "for this path.")
    elif spread < 2.0 and mean_delta > 6.0:
        print("  -> row 2: flat across partials, ~+%.0f dB. Logic applies an LFE gain offset "
              "-> D16." % mean_delta)
    elif lf and hf and (sum(lf) / len(lf)) - (sum(hf) / len(hf)) > 6.0:
        print("  -> row 3: flat at LF, increasingly negative above. Bass-management low-pass "
              "-> D16.")
    else:
        print("  -> row 4: ANYTHING ELSE. Check airAmount == 0 on both sources and that they are "
              "co-located BEFORE invoking D16 (P103, NC4, Execution Constraints 1 and 12).")
    print("")
    print("lfe OK — %d partials measured on ch%d against ch%d" % (len(rows), pair[0], pair[1]))
    return {"partials_hz": [r[0] for r in rows],
            "delta_db": [round(r[3], 2) for r in rows],
            "broadband_delta_db": round(db(broadband_a) - db(broadband_b), 2),
            "mean_delta_db": round(mean_delta, 2),
            "spread_db": round(spread, 2)}


def mode_ping(data, args):
    fs = data["fs"]
    channels = data["channels"]
    nch = len(channels)
    full_scale = data["full_scale"]

    constants, provenance = load_ping_constants()
    n_speakers = constants["kNumSpeakers"]
    on_s = constants["kOnSeconds"]
    gap_s = constants["kGapSeconds"]
    period_s = on_s + gap_s
    cycle_s = n_speakers * period_s

    print("── ping ──────────────────────────────────────────────────────────────────")
    print("  %s" % provenance)
    print("  kNumSpeakers %d, kOnSeconds %.3f, kGapSeconds %.3f -> period %.3f s, cycle %.3f s"
          % (n_speakers, on_s, gap_s, period_s, cycle_s))
    print("  inputs: %d (%s), %d channels, %d Hz, %.3f s"
          % (len(data["paths"]), data["source"], nch, fs, data["nframes"] / float(fs)))

    if nch != n_speakers:
        die("ping: %d channels captured but kNumSpeakers is %d. Criterion 2 is about EIGHT "
            "distinct physical channels." % (nch, n_speakers))

    if data["source"] == "multi":
        lengths = [n for _, n in data["per_file"]]
        spread_frames = max(lengths) - min(lengths)
        print("  per-file length spread: %d frames (%.3f s); analysed against the shortest."
              % (spread_frames, spread_frames / float(fs)))

    bursts = find_bursts(channels, fs, full_scale)
    print("  envelope segmentation: %d burst(s) found" % len(bursts))

    if len(bursts) != n_speakers:
        die("ping: envelope segmentation found %d bursts, expected %d. Either the capture does "
            "not span a full %.1f s auto cycle, or the transport was rolled part-way through one. "
            "A partial cycle must not be graded (clause 5)." % (len(bursts), n_speakers, cycle_s))

    onsets = [b[0] / float(fs) for b in bursts]
    intervals = [onsets[i + 1] - onsets[i] for i in range(len(onsets) - 1)]
    measured_period = sorted(intervals)[len(intervals) // 2]
    measured_on = sorted((b[1] - b[0]) / float(fs) for b in bursts)[len(bursts) // 2]

    print("  measured burst period: %.3f s (constant %.3f s, tolerance %.3f s)"
          % (measured_period, period_s, PING_PERIOD_TOLERANCE_S))
    print("  measured burst length: %.3f s (kOnSeconds %.3f s, threshold is -20 dB from peak)"
          % (measured_on, on_s))
    print("")

    # Preference 3: the constant is ASSERTED against the capture, never merely trusted.
    if abs(measured_period - period_s) > PING_PERIOD_TOLERANCE_S:
        die("ping: measured burst period %.3f s differs from the constant %.3f s by more than "
            "%.3f s. Either VerifyPing.h moved and this analysis is against a stale expectation, "
            "or the capture is not a verify-ping auto cycle "
            "(pattern_test_fixture_mirrors_drift_silently)."
            % (measured_period, period_s, PING_PERIOD_TOLERANCE_S))

    expect = parse_perm(args.expect, n_speakers, "--expect") if args.expect \
        else list(range(1, n_speakers + 1))

    guard = int(round(PING_EDGE_GUARD_S * fs))
    print("  window   analysed span      energised   runner-up   isolation dB")
    observed = []
    margins = []
    failures = []

    for idx, (start, end) in enumerate(bursts):
        a = start + guard
        b = end - guard
        if b - a < int(0.2 * fs):
            die("ping: burst %d is %.3f s, too short to analyse after a %.0f ms edge guard."
                % (idx + 1, (end - start) / float(fs), PING_EDGE_GUARD_S * 1000))
        length = b - a

        levels = [db(rms(chan, a, length, full_scale)) for chan in channels]
        ranked = sorted(range(nch), key=lambda c: levels[c], reverse=True)
        best, runner_up = ranked[0], ranked[1]
        margin = levels[best] - levels[runner_up]
        observed.append(best + 1)
        margins.append(margin)

        print("  %6d   %6.3f .. %6.3f s      ch%-2d        ch%-2d      %8.1f"
              % (idx + 1, a / float(fs), b / float(fs), best + 1, runner_up + 1, margin))

        if levels[best] < args.floor:
            failures.append("window %d: loudest channel ch%d is %.1f dBFS, below the %.1f dBFS "
                            "presence floor — the window is silent (clause 3)."
                            % (idx + 1, best + 1, levels[best], args.floor))
        elif margin < args.isolation:
            failures.append("window %d: isolation %.1f dB (ch%d over ch%d) is below the %.1f dB "
                            "floor — MORE THAN ONE channel is energised, so the ping is not "
                            "reaching eight DISTINCT physical channels (clause 4)."
                            % (idx + 1, margin, best + 1, runner_up + 1, args.isolation))

    print("")
    print("  observed sequence: %s" % ",".join(str(v) for v in observed))
    print("  expected sequence: %s" % ",".join(str(v) for v in expect))
    print("  minimum measured isolation: %.1f dB (floor %.1f dB)"
          % (min(margins) if margins else float("nan"), args.isolation))
    print("")

    if observed != expect:
        failures.append("sequence mismatch: observed %s, expected %s"
                        % (",".join(str(v) for v in observed), ",".join(str(v) for v in expect)))

    if failures:
        die("ping:\n         - %s" % "\n         - ".join(failures))

    print("ping OK — 8 windows, exactly one channel per window, sequence %s, "
          "minimum isolation %.1f dB"
          % (",".join(str(v) for v in observed), min(margins)))
    return {"observed": ",".join(str(v) for v in observed),
            "min_isolation_db": round(min(margins), 1),
            "measured_period_s": round(measured_period, 3),
            "measured_on_s": round(measured_on, 3)}


MODES = {"probe": mode_probe, "order": mode_order, "lfe": mode_lfe, "ping": mode_ping}


# ═══ Clause 6: --check ══════════════════════════════════════════════════════════════════════════


def run_check(manifest_path, session_root=None):
    """Re-run the committed expectation table against the committed artifacts.

    The manifest records each invocation AND its result. --check re-derives both.

    Where the artifacts are read from, in priority order:

      1. --session-root, for a second person who has the WAVs at a different path;
      2. the run's own recorded `input_dir` — where --emit-json actually read it from;
      3. the manifest's directory (legacy runs, and the self-test's fixtures).

    Rule 2 exists because this phase's bounces are deliberately kept OUTSIDE the repo
    (the gitignore rule does not cover stages/4-polish/evidence/, so a WAV dropped next
    to the manifest would be committed). Resolving manifest-relative therefore looked for
    every artifact in the one directory it is forbidden to be in, and --check could not
    pass by construction. Recording the directory keeps the pair travelling together
    without moving audio into the repo.

    This changes only WHERE a file is looked for. Every assertion still re-runs in full,
    and a missing artifact is still a failure.
    """
    if not os.path.exists(manifest_path):
        die("--check: %s does not exist. The committed expectation table is missing."
            % manifest_path)

    with open(manifest_path, "r", encoding="utf-8") as handle:
        try:
            manifest = json.load(handle)
        except json.JSONDecodeError as exc:
            die("--check: %s is not valid JSON (%s)" % (manifest_path, exc))

    runs = manifest.get("runs", [])
    if not runs:
        die("--check: %s records zero runs. Nothing analysed is not a pass (clause 5)."
            % manifest_path)

    manifest_base = os.path.dirname(os.path.abspath(manifest_path))
    failures = []
    print("── check ─────────────────────────────────────────────────────────────────")
    print("  manifest: %s" % manifest_path)
    if session_root:
        print("  root    : %s (--session-root override)" % session_root)
    if manifest.get("freeze_sha"):
        print("  freeze  : %s" % manifest["freeze_sha"])
    print("  runs    : %d" % len(runs))
    print("")

    for index, run in enumerate(runs):
        label = run.get("label") or run.get("mode", "?")
        mode = run.get("mode")
        if mode not in MODES:
            failures.append("run %d (%s): unknown mode %r" % (index + 1, label, mode))
            continue

        base = session_root or run.get("input_dir") or manifest_base
        paths = [os.path.normpath(os.path.join(base, p)) for p in run.get("input", [])]
        args = argparse.Namespace(
            mode=mode, input=paths,
            expect=run.get("expect"), label=run.get("label"),
            tones=run.get("tones"), partials=run.get("partials"),
            channels=run.get("channels"),
            floor=run.get("floor", PRESENCE_FLOOR_DB_DEFAULT),
            isolation=run.get("isolation", ISOLATION_DB_DEFAULT),
            skip=run.get("skip", SKIP_SECONDS_DEFAULT),
            expect_channels=run.get("expect_channels"),
            expect_rate=run.get("expect_rate"),
            expect_depth=run.get("expect_depth"))

        print("  [%d/%d] %s — %s %s" % (index + 1, len(runs), label, mode,
                                        " ".join(os.path.basename(p) for p in paths)))
        try:
            data = read_inputs(paths)
            result = MODES[mode](data, args)
        except SystemExit:
            failures.append("run %d (%s): re-run FAILED against the committed artifacts" %
                            (index + 1, label))
            continue

        recorded = run.get("result")
        if recorded is None:
            failures.append("run %d (%s): the manifest records no result to compare against — "
                            "an entry that asserts nothing is vacuous (clause 6)."
                            % (index + 1, label))
            continue

        for key, expected_value in recorded.items():
            actual = result.get(key)
            if actual != expected_value:
                failures.append("run %d (%s): %s — recorded %r, re-derived %r"
                                % (index + 1, label, key, expected_value, actual))

    print("")
    if failures:
        die("--check:\n         - %s" % "\n         - ".join(failures))

    print("analyse_bounce.py: --check OK — %d runs re-derived from %s"
          % (len(runs), os.path.basename(manifest_path)))
    return 0


# ═══ Entry point ════════════════════════════════════════════════════════════════════════════════


def main():
    parser = argparse.ArgumentParser(
        description="Analyse O-Octagon Phase 4.2 bounces and realtime captures.",
        epilog="Clause 5: any failure exits non-zero, and zero cases analysed is never an OK.")
    parser.add_argument("--input", nargs="+", metavar="WAV",
                        help="one 8-channel file, OR N mono files where channel k is file k")
    parser.add_argument("--mode", choices=sorted(MODES))
    parser.add_argument("--expect", help="permutation, e.g. 2,3,4,5,6,7,8,1. "
                                         "MANDATORY in order mode (clause 1)")
    parser.add_argument("--label", choices=["CR-a", "CR-b"],
                        help="CR-b refuses an identity --expect (clause 2 / N13)")
    parser.add_argument("--tones", help="csv Hz; default %s" % ",".join(str(t) for t in TONES_DEFAULT))
    parser.add_argument("--partials",
                        help="csv Hz; default %s" % ",".join(str(p) for p in PARTIALS_DEFAULT))
    parser.add_argument("--channels", metavar="MEASURED,REFERENCE",
                        help="lfe mode: two 1-based channels, e.g. 4,1")
    parser.add_argument("--check", nargs="?", const=DEFAULT_MANIFEST, metavar="MANIFEST",
                        help="re-run the committed expectation table (clause 6)")
    parser.add_argument("--session-root", metavar="DIR",
                        help="--check: read every recorded artifact from DIR instead of the "
                             "directory it was recorded from. For a second person re-deriving "
                             "the table with the WAVs at a different path")
    parser.add_argument("--emit-json", metavar="PATH",
                        help="append this run to a --check manifest, so the recorded figures are "
                             "MEASURED rather than transcribed")
    parser.add_argument("--floor", type=float, default=PRESENCE_FLOOR_DB_DEFAULT,
                        help="presence floor dBFS (default %.1f)" % PRESENCE_FLOOR_DB_DEFAULT)
    parser.add_argument("--isolation", type=float, default=ISOLATION_DB_DEFAULT,
                        help="isolation floor dB (default %.1f)" % ISOLATION_DB_DEFAULT)
    parser.add_argument("--skip", type=float, default=SKIP_SECONDS_DEFAULT,
                        help="seconds skipped at the head of a bounce (default %.2f)"
                             % SKIP_SECONDS_DEFAULT)
    parser.add_argument("--expect-channels", type=int, help="probe mode assertion")
    parser.add_argument("--expect-rate", type=int, help="probe mode assertion")
    parser.add_argument("--expect-depth", type=int, help="probe mode assertion")
    args = parser.parse_args()

    if args.check:
        sys.exit(run_check(args.check, args.session_root))

    if not args.mode:
        die("--mode is required (one of %s), or use --check." % ", ".join(sorted(MODES)))
    if not args.input:
        die("--input is required. Clause 5: nothing analysed is not a pass.")

    data = read_inputs(args.input)
    result = MODES[args.mode](data, args)

    if args.emit_json:
        # Record WHERE the artifacts were read from. Without it --check resolves
        # manifest-relative, and this phase's WAVs are deliberately kept outside the
        # repo — so every recorded run would point at a path that cannot exist.
        input_dirs = {os.path.dirname(os.path.abspath(p)) for p in args.input}
        if len(input_dirs) > 1:
            die("--emit-json: this run's inputs span %d directories (%s). A run records ONE "
                "input_dir, so --check could not re-derive it. Stage the inputs together."
                % (len(input_dirs), ", ".join(sorted(input_dirs))))
        entry = {"label": args.label or args.mode, "mode": args.mode,
                 "input": [os.path.basename(p) for p in args.input],
                 "input_dir": input_dirs.pop(), "result": result}
        for key, value in (("expect", args.expect), ("tones", args.tones),
                           ("partials", args.partials), ("channels", args.channels)):
            if value:
                entry[key] = value
        if args.floor != PRESENCE_FLOOR_DB_DEFAULT:
            entry["floor"] = args.floor
        if args.isolation != ISOLATION_DB_DEFAULT:
            entry["isolation"] = args.isolation
        if args.skip != SKIP_SECONDS_DEFAULT:
            entry["skip"] = args.skip

        manifest = {"runs": []}
        if os.path.exists(args.emit_json):
            with open(args.emit_json, "r", encoding="utf-8") as handle:
                manifest = json.load(handle)
        manifest.setdefault("runs", [])
        manifest["runs"] = [r for r in manifest["runs"]
                            if not (r.get("label") == entry["label"]
                                    and r.get("mode") == entry["mode"])]
        manifest["runs"].append(entry)
        os.makedirs(os.path.dirname(os.path.abspath(args.emit_json)), exist_ok=True)
        with open(args.emit_json, "w", encoding="utf-8") as handle:
            json.dump(manifest, handle, indent=2, sort_keys=True)
            handle.write("\n")
        print("recorded to %s (%d run(s))" % (args.emit_json, len(manifest["runs"])))

    sys.exit(0)


if __name__ == "__main__":
    main()
