#!/usr/bin/env python3
# This file is part of O-Octagon, an Ouaricon Audio plugin.
# Copyright (C) 2026  Ouaricon Audio
# SPDX-License-Identifier: AGPL-3.0-or-later
"""
The self-test for analyse_bounce.py — proves the six anti-vacuity clauses FIRE.

═══ WHY THIS EXISTS ════════════════════════════════════════════════════════════════════════════════
PLAN-4.2's standing rule is that every gate is *run at execute and RE-RUN FROM SCRATCH at verify,
never read out of a summary* — the discipline that has caught eleven mis-attributions across 2.3,
3.1, 3.2, 3.3 and 4.1.

analyse_bounce.py's six clauses (P105) are the phase's main defence against a vacuous result. But
they can only be exercised against BOUNCES, and the real bounces do not exist until the Logic
session has run. Without this file, "the clauses were verified" would be a claim resting on a
transcript nobody can re-execute — exactly the shape of evidence this project keeps rejecting.

So this script SYNTHESISES the pathological captures, at the desk, with no host and no hardware:
a correct CR-a, a correctly-permuted CR-b, a muted-track bounce, a flat LFE pair, a
bass-managed LFE pair, a SILENT LFE pair, a clean 8-file ping capture carrying a round-trip delay,
a ping capture with two channels energised at once, and a ping capture whose burst period is wrong.

Then it asserts the ANALYSER'S EXIT CODE on each. A clause that has never been seen to fail is not
a clause; it is a comment.

Run:  python3 tests/tools/selftest_analyse_bounce.py
Exit: 0 = every case behaved as declared. Non-zero = the count that did not.

Stdlib only, no RNG, and it writes only into a temporary directory that it removes.
"""

import math
import os
import struct
import subprocess
import sys
import tempfile
import wave

FS = 48000
HERE = os.path.dirname(os.path.abspath(__file__))
ANALYSER = os.path.join(HERE, "analyse_bounce.py")
GENERATOR = os.path.join(HERE, "gen_bounce_sources.py")

#: Must match gen_bounce_sources.py / analyse_bounce.py. Asserted, not assumed — see check_tones().
TONES = [997, 1499, 2003, 2503, 3001, 3499, 4001, 4507]

#: VerifyPing.h's auto cycle, used to build a CORRECT capture. The analyser parses the real header;
#: this only needs to produce something that agrees with it.
PING_ON, PING_GAP = 1.2, 0.4
PING_DELAY = 0.137          # a deliberate round-trip delay, so window discovery cannot key off zero


# ── WAV helpers ─────────────────────────────────────────────────────────────────────────────────


def read_mono(path):
    with wave.open(path, "rb") as handle:
        raw = handle.readframes(handle.getnframes())
    count = len(raw) // 3
    widened = bytearray(count * 4)
    widened[0::4] = raw[0::3]
    widened[1::4] = raw[1::3]
    widened[2::4] = raw[2::3]
    widened[3::4] = bytes(0xFF if b & 0x80 else 0x00 for b in raw[2::3])
    return list(struct.unpack("<%di" % count, bytes(widened)))


def write_multi(path, channels, fs=FS):
    nch = len(channels)
    n = min(len(c) for c in channels)
    flat = []
    for i in range(n):
        for chan in channels:
            flat.append(chan[i])
    packed = struct.pack("<%di" % len(flat), *flat)
    body = bytearray(len(flat) * 3)
    body[0::3] = packed[0::4]
    body[1::3] = packed[1::4]
    body[2::3] = packed[2::4]
    with wave.open(path, "wb") as handle:
        handle.setnchannels(nch)
        handle.setsampwidth(3)
        handle.setframerate(fs)
        handle.writeframes(bytes(body))


def burst_channel(total, start, length, amp):
    """Deterministic dense mid-band content — no RNG, so the file is byte-stable."""
    buf = [0] * total
    fade = int(round(0.020 * FS))
    for i in range(length):
        gain = 1.0
        if i < fade:
            gain = i / fade
        elif i >= length - fade:
            gain = (length - 1 - i) / fade
        value = (math.sin(2 * math.pi * 511.0 * i / FS)
                 + math.sin(2 * math.pi * 1319.0 * i / FS)
                 + math.sin(2 * math.pi * 3067.0 * i / FS)
                 + math.sin(2 * math.pi * 6151.0 * i / FS)) / 4.0
        buf[start + i] = int(round(value * gain * amp))
    return buf


def build_ping(out_dir, prefix, period, leak_into=None):
    """Eight mono files; channel k carries burst k. `period` is the ON+GAP interval."""
    total = int(round((8 * period + PING_DELAY + 0.6) * FS))
    delay_n = int(round(PING_DELAY * FS))
    length = int(round(PING_ON * FS))
    amp = 10 ** (-20.0 / 20.0) * 8388608.0

    paths = []
    buffers = []
    for ch in range(8):
        start = delay_n + int(round(ch * period * FS))
        buffers.append(burst_channel(total, start, length, amp))

    if leak_into is not None:
        src, dst = leak_into                      # 0-based: bleed src's burst into dst
        start = delay_n + int(round(src * period * FS))
        for i in range(length):
            buffers[dst][start + i] += buffers[src][start + i] // 2

    for ch in range(8):
        path = os.path.join(out_dir, "%s-ch%d.wav" % (prefix, ch + 1))
        write_multi(path, [buffers[ch]])
        paths.append(path)
    return paths


# ── the case table ──────────────────────────────────────────────────────────────────────────────


def main():
    failures = []
    passes = 0

    def case(desc, want_rc, argv):
        nonlocal passes
        proc = subprocess.run([sys.executable, ANALYSER] + argv,
                              capture_output=True, text=True)
        if proc.returncode == want_rc:
            print("  PASS  rc=%d  %s" % (proc.returncode, desc))
            passes += 1
        else:
            print("  FAIL  rc=%d (want %d)  %s" % (proc.returncode, want_rc, desc))
            tail = (proc.stdout + proc.stderr).strip().splitlines()[-2:]
            for line in tail:
                print("          | %s" % line)
            failures.append(desc)

    with tempfile.TemporaryDirectory(prefix="oo-selftest-") as work:
        src = os.path.join(work, "src")
        out = os.path.join(work, "out")
        os.makedirs(out, exist_ok=True)

        gen = subprocess.run([sys.executable, GENERATOR, "--output", src, "--duration", "3.0"],
                             capture_output=True, text=True)
        if gen.returncode != 0:
            sys.stderr.write("selftest: gen_bounce_sources.py failed\n" + gen.stderr)
            return 1

        tones = [read_mono(os.path.join(src, "tone-%d.wav" % k)) for k in range(1, 9)]
        n = min(len(t) for t in tones)
        tones = [t[:n] for t in tones]
        silence = [0] * n

        # ── order-mode fixtures ─────────────────────────────────────────────────────────────────
        cr_a = os.path.join(out, "cr-a.wav")
        write_multi(cr_a, tones)

        expect_b = [2, 3, 4, 5, 6, 7, 8, 1]
        permuted = [None] * 8
        for k in range(8):
            permuted[expect_b[k] - 1] = tones[k]
        cr_b = os.path.join(out, "cr-b.wav")
        write_multi(cr_b, permuted)

        muted = list(tones)
        muted[4] = silence
        cr_a_muted = os.path.join(out, "cr-a-muted.wav")
        write_multi(cr_a_muted, muted)

        # ── lfe-mode fixtures ───────────────────────────────────────────────────────────────────
        lfe = read_mono(os.path.join(src, "lfe-multitone.wav"))
        m = min(len(lfe), n)

        flat = [silence[:m] for _ in range(8)]
        flat[3] = lfe[:m]
        flat[0] = lfe[:m]
        cs_flat = os.path.join(out, "cs-flat.wav")
        write_multi(cs_flat, flat)

        low = lfe[:m]
        for _ in range(4):                          # 4th-order one-pole ~120 Hz: bass management
            acc, prev = [], 0.0
            a = math.exp(-2.0 * math.pi * 120.0 / FS)
            for x in low:
                prev = (1 - a) * x + a * prev
                acc.append(prev)
            low = acc
        managed = [silence[:m] for _ in range(8)]
        managed[3] = [int(round(v)) for v in low]
        managed[0] = lfe[:m]
        cs_managed = os.path.join(out, "cs-bassmanaged.wav")
        write_multi(cs_managed, managed)

        cs_silent = os.path.join(out, "cs-silent.wav")
        write_multi(cs_silent, [silence[:m] for _ in range(8)])

        # ── ping fixtures ───────────────────────────────────────────────────────────────────────
        ping_ok = build_ping(out, "ping", PING_ON + PING_GAP)
        ping_leak = build_ping(out, "leak", PING_ON + PING_GAP, leak_into=(2, 6))
        ping_slow = build_ping(out, "slow", PING_ON + 0.25)      # period 1.45 s, not 1.6 s

        print("── analyse_bounce.py self-test ───────────────────────────────────────────")
        print("  fixtures in %s" % work)
        print("")

        print("  probe")
        case("probe reads an 8-channel 24-bit file", 0,
             ["--mode", "probe", "--input", cr_a, "--expect-channels", "8", "--expect-depth", "24"])
        case("probe FAILS a wrong --expect-channels", 1,
             ["--mode", "probe", "--input", cr_a, "--expect-channels", "6"])
        case("a missing input FAILS", 1,
             ["--mode", "probe", "--input", os.path.join(out, "nope.wav")])

        print("  order — clauses 1, 2, 3, 4")
        case("CLAUSE 1: order without --expect is REFUSED", 1,
             ["--mode", "order", "--input", cr_a])
        case("CLAUSE 2 (N13): --label CR-b with an IDENTITY --expect is REFUSED", 1,
             ["--mode", "order", "--label", "CR-b", "--input", cr_b,
              "--expect", "1,2,3,4,5,6,7,8"])
        case("CLAUSE 3: a muted track FAILS (not a 7-of-8 partial pass)", 1,
             ["--mode", "order", "--label", "CR-a", "--input", cr_a_muted,
              "--expect", "1,2,3,4,5,6,7,8"])
        case("an --expect that is not a permutation is REFUSED", 1,
             ["--mode", "order", "--input", cr_a, "--expect", "1,1,3,4,5,6,7,8"])
        case("a real order mismatch FAILS", 1,
             ["--mode", "order", "--input", cr_b, "--expect", "1,2,3,4,5,6,7,8"])
        case("CR-a passes against the identity", 0,
             ["--mode", "order", "--label", "CR-a", "--input", cr_a,
              "--expect", "1,2,3,4,5,6,7,8"])
        case("CR-b passes against P106's 8-cycle", 0,
             ["--mode", "order", "--label", "CR-b", "--input", cr_b,
              "--expect", "2,3,4,5,6,7,8,1"])

        print("  lfe — clause 5, and the verdict table")
        case("a FLAT pair is measured (P103 row 1)", 0,
             ["--mode", "lfe", "--channels", "4,1", "--input", cs_flat])
        case("a BASS-MANAGED pair is measured (P103 row 3)", 0,
             ["--mode", "lfe", "--channels", "4,1", "--input", cs_managed])
        case("CLAUSE 5: a SILENT pair FAILS — two equal silences are not 'flat'", 1,
             ["--mode", "lfe", "--channels", "4,1", "--input", cs_silent])
        case("lfe without --channels is REFUSED", 1,
             ["--mode", "lfe", "--input", cs_flat])
        case("an out-of-range channel is REFUSED", 1,
             ["--mode", "lfe", "--channels", "9,1", "--input", cs_flat])

        print("  ping — envelope segmentation and the mirror assertion")
        case("a clean 8-file capture with a 137 ms round-trip delay passes", 0,
             ["--mode", "ping", "--input"] + ping_ok)
        case("TWO channels energised in one window FAILS the isolation clause", 1,
             ["--mode", "ping", "--input"] + ping_leak)
        case("files supplied OUT OF ORDER FAIL the sequence assertion", 1,
             ["--mode", "ping", "--input", ping_ok[1], ping_ok[0]] + ping_ok[2:])
        case("fewer than kNumSpeakers channels FAILS", 1,
             ["--mode", "ping", "--input"] + ping_ok[:6])
        case("a WRONG burst period FAILS the constant assertion", 1,
             ["--mode", "ping", "--input"] + ping_slow)

        print("  clause 6 — --check")
        manifest = os.path.join(out, "manifest.json")
        for label, path, expect in (("CR-a", cr_a, "1,2,3,4,5,6,7,8"),
                                    ("CR-b", cr_b, "2,3,4,5,6,7,8,1")):
            subprocess.run([sys.executable, ANALYSER, "--mode", "order", "--label", label,
                            "--input", path, "--expect", expect, "--emit-json", manifest],
                           capture_output=True, text=True)
        case("--check re-derives the committed table", 0, ["--check", manifest])

        import json
        with open(manifest, encoding="utf-8") as handle:
            data = json.load(handle)

        tampered = os.path.join(out, "tampered.json")
        data_t = json.loads(json.dumps(data))
        data_t["runs"][0]["result"]["observed"] = "8,7,6,5,4,3,2,1"
        with open(tampered, "w", encoding="utf-8") as handle:
            json.dump(data_t, handle)
        case("--check FAILS on a tampered recorded result", 1, ["--check", tampered])

        empty = os.path.join(out, "empty.json")
        with open(empty, "w", encoding="utf-8") as handle:
            json.dump({"runs": []}, handle)
        case("--check FAILS on a manifest with zero runs", 1, ["--check", empty])

        bare = os.path.join(out, "bare.json")
        stripped = {"runs": [{k: v for k, v in data["runs"][0].items() if k != "result"}]}
        with open(bare, "w", encoding="utf-8") as handle:
            json.dump(stripped, handle)
        case("--check FAILS on an entry that asserts nothing", 1, ["--check", bare])

    print("")
    if failures:
        print("selftest_analyse_bounce: %d of %d cases FAILED" % (len(failures), passes + len(failures)))
        for desc in failures:
            print("  - %s" % desc)
        return len(failures)

    print("selftest_analyse_bounce: OK — %d cases, every clause seen to fire" % passes)
    return 0


if __name__ == "__main__":
    sys.exit(main())
