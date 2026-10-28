# O-simpleGrain alias gate (v1.7.0): up-transposed grains read the band-limited
# mipmap (grain-engine BandlimitedReader) instead of Lagrange + a one-pole.
#
# Source: a 10 s sine at the engine rate (48 kHz, float WAV), injected as a dropped
# file through the session's embedded copy (v1.5.6 state), so nothing is resampled.
# Isolated grains (60 ms Hann every 200 ms, no spray / scatter, frozen) — no two
# overlap, so no inter-grain phase interference at any rate (2x-overlapped grains
# cancel by up to 4 dB at non-octave rates, in both versions). The ideal output is
# a train of tone bursts at rate * f0 — or silence once that is past Nyquist.
# Everything else in 20 Hz .. 20 kHz is spurious. At rate 127 a grain reads 7.6 s
# from the 1 s Position, inside the 10 s source (past the end it would hold).
#
# Verdicts, all relative to the energy of the same sine at rate 1 (note 60, pitch 0):
#   alias   rate * f0 > 24 kHz: total audible energy <= -60 dB (the reader's design
#           point for residual rates <= 2; every case here stays inside it)
#   pass    rate * f0 <= 0.2 fs: energy within +-1.5 dB of the rate-1 render, and
#           the peak within 1 % of rate * f0
#   info    the 16 kHz roll-off, printed next to v1.6.0 (see CASES)
# Rates cover every mip level 0..6, both sides of each octave edge (S1 / S6).
#
# Positive control: run against v1.6.0 with --expect-fail; the alias cases must fail.
#
# usage: uv run --python 3.12 --with pedalboard --with numpy --with soundfile \
#            python -I gate.py [plugin.vst3] [--expect-fail]
import base64
import io
import re
import struct
import sys
import numpy as np
import pedalboard
import soundfile as sf

ARGS = [a for a in sys.argv[1:] if not a.startswith("--")]
EXPECT_FAIL = "--expect-fail" in sys.argv
PLUGIN = ARGS[0] if ARGS else \
    "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3"
FS = 48000
ALIAS_LIMIT_DB = -60.0
PASS_TOL_DB = 1.5

# (kind, name, f0 Hz, MIDI note, grain pitch st). rate = 2^((note - 60 + pitch) / 12).
# Each octave edge is tested from both sides: +0.1 st over it reads level l with
# sinc set S1, -0.1 st under it reads level l - 1 with S6 (the narrowest set).
# Exact octave rates are not used: the Grain Pitch read-back lands a hair either
# side of an integer, so "exact" cases silently test the wrong level.
CASES = [
    ("alias", "+7 st (L0 S3)",          20000.0,  60,  7.0),   # 29.97 k -> 18.03 k
    ("alias", "+11.9 st (L0 S6)",       15000.0,  60, 11.9),   # 29.83 k -> 18.17 k
    ("alias", "+12.1 st (L1 S1)",       15000.0,  72,  0.1),   # 30.17 k -> 17.83 k
    ("alias", "+24.1 st (L2 S1)",        8000.0,  84,  0.1),   # 32.19 k -> 15.81 k
    ("alias", "+31 st (L2 S3)",          5000.0,  67, 24.0),   # 29.97 k -> 18.03 k (6 k = L2 Nyquist: decimates to 0)
    ("alias", "+35.9 st (L2 S6)",        4000.0,  96, -0.1),   # 31.82 k -> 16.18 k
    ("alias", "+36.1 st (L3 S1)",        4000.0,  96,  0.1),   # 32.19 k -> 15.81 k
    ("alias", "+48.1 st (L4 S1)",        2500.0, 108,  0.1),   # 40.23 k -> 7.77 k
    ("alias", "+59.9 st (L4 S6)",        1000.0,  96, 23.9),   # 31.82 k -> 16.18 k
    ("alias", "+60.1 st (L5 S1)",        1000.0,  97, 23.1),   # 32.19 k -> 15.81 k
    ("alias", "+72.1 st (L6 S1)",         500.0, 109, 23.1),   # 32.19 k -> 15.81 k
    ("alias", "+83.9 st (L6 S6)",         250.0, 120, 23.9),   # 31.82 k -> 16.18 k
    ("pass",  "+7 st (L0 S3)",           6000.0,  60,  7.0),   # 8.99 k
    ("pass",  "+11.9 st (L0 S6)",        4500.0,  60, 11.9),   # 8.95 k
    ("pass",  "+12.1 st (L1 S1)",        4500.0,  72,  0.1),   # 9.05 k
    ("pass",  "+31 st (L2 S3)",          1500.0,  67, 24.0),   # 8.99 k
    ("pass",  "+48.1 st (L4 S1)",         500.0, 108,  0.1),   # 8.05 k
    ("pass",  "+83.9 st (L6 S6)",          70.0, 120, 23.9),   # 8.91 k
    ("info",  "+11.9 st (L0 S6)",        8000.0,  60, 11.9),   # 15.91 k
    ("info",  "+12.1 st (L1 S1)",        8000.0,  72,  0.1),   # 16.09 k
    ("info",  "+7 st (L0 S3)",          10700.0,  60,  7.0),   # 16.03 k
]

# --- session state framing (see tests/state-persistence/persist_test.py) ----------
JT = ".ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+"


def juce_b64_decode(s):
    size, enc = s.split(".", 1)
    size = int(size); bits = 0; nb = 0; out = bytearray()
    for c in enc:
        bits |= JT.index(c) << nb; nb += 6
        while nb >= 8:
            out.append(bits & 0xFF); bits >>= 8; nb -= 8
    return bytes(out[:size]).ljust(size, b"\0")


def juce_b64_encode(b):
    n = (len(b) * 8 + 5) // 6; out = []; bits = 0; nb = 0
    for byte in b:
        bits |= byte << nb; nb += 8
        while nb >= 6:
            out.append(JT[bits & 63]); bits >>= 6; nb -= 6
    if len(out) < n:
        out.append(JT[bits & 63])
    return f"{len(b)}." + "".join(out)


def vc2_unpack(raw):
    assert raw[:4] == b"VC2!", raw[:8]
    size = struct.unpack("<I", raw[4:8])[0]
    return raw[8:8 + size].decode("utf-8"), raw[8 + size + 1:]


def vc2_pack(xml, tail=b""):
    body = xml.encode("utf-8")
    return b"VC2!" + struct.pack("<I", len(body)) + body + b"\0" + tail


def with_source(raw, identity, data_b64):
    outer, _ = vc2_unpack(raw)
    comp = juce_b64_decode(re.search(r"<IComponent>([^<]*)</IComponent>", outer).group(1))
    xml, tail = vc2_unpack(comp)
    xml = re.sub(r"<SOURCE[^>]*/>", "", xml)
    xml = re.sub(r"(</[^>]+>\s*)$", f'<SOURCE identity="{identity}" data="{data_b64}"/>\\1', xml)
    comp = vc2_pack(xml, tail)
    return vc2_pack(re.sub(r"<IComponent>[^<]*</IComponent>",
                           lambda _: f"<IComponent>{juce_b64_encode(comp)}</IComponent>", outer))


def sine_b64(f0):
    t = np.arange(10 * FS) / FS
    x = (0.5 * np.sin(2 * np.pi * f0 * t)).astype(np.float32)
    b = io.BytesIO()
    sf.write(b, x, FS, subtype="FLOAT", format="WAV")
    return base64.b64encode(b.getvalue()).decode()


def set_value(p, key, value):
    """Grain Size and Density are skewed: a linear raw_value lands far off (20 ms
    -> 2.04 ms, 100 /s -> 19.7 /s). Bisect raw_value on the read-back value."""
    par = p.parameters[key]
    lo, hi = 0.0, 1.0
    for _ in range(40):
        mid = 0.5 * (lo + hi)
        par.raw_value = mid
        if float(getattr(p, key)) < value:
            lo = mid
        else:
            hi = mid
    par.raw_value = hi


def render(f0, note, pitch):
    p = pedalboard.load_plugin(PLUGIN)
    for k, v in dict(grain_size_ms=60.0, density_g_s=5.0, position=10.0, scan=0.0,
                     pitch_spray_st=0.0, position_spray=0.0, scatter=0.0, pan_spray=0.0,
                     vel_density=0.0, grain_pitch_st=float(pitch), output_level_db=0.0).items():
        set_value(p, k, v)
    p.window = "Hann"
    p.freeze = True          # pins the playhead where prepare seeds it (Position 10 %)
    p.raw_state = with_source(p.raw_state, f"dropped:sine{int(f0)}.wav", sine_b64(f0))
    msgs = [(bytes([0x90, note, 100]), 0.05), (bytes([0x80, note, 0]), 1.9)]
    y = p(msgs, duration=2.0, sample_rate=FS, num_channels=2, buffer_size=512, reset=True)
    assert np.isfinite(y).all()
    m = y.mean(0)[int(0.4 * FS):int(1.8 * FS)]
    spec = np.abs(np.fft.rfft(m * np.hanning(len(m)))) ** 2
    f = np.fft.rfftfreq(len(m), 1 / FS)
    band = (f >= 20.0) & (f <= 20000.0)
    return float(np.sum(spec[band])), float(f[np.argmax(spec * band)])


def db(r):
    return 10.0 * np.log10(max(r, 1e-30))


def main():
    print(f"plugin: {PLUGIN}{'  (expect fail)' if EXPECT_FAIL else ''}")
    fails = alias_fails = 0
    refs = {}
    for kind, name, f0, note, pitch in CASES:
        if f0 not in refs:
            e, pk = render(f0, 60, 0)
            assert abs(pk - f0) / f0 < 0.01 and e > 0, f"rate-1 reference {f0}: peak {pk}"
            refs[f0] = e
        rate = 2.0 ** ((note - 60 + pitch) / 12.0)
        fe = rate * f0
        e, pk = render(f0, note, pitch)
        rel = db(e / refs[f0])
        if kind == "info":
            print(f"INFO  16k   {name:24s} f0 {f0:7.0f} rate {rate:7.3f}: "
                  f"{fe/1000:6.2f} kHz level {rel:+6.2f} dB")
            continue
        if kind == "alias":
            ok = rel <= ALIAS_LIMIT_DB
            alias_fails += not ok
            print(f"{'PASS' if ok else 'FAIL'} alias {name:24s} f0 {f0:7.0f} rate {rate:7.3f}: "
                  f"{fe/1000:6.2f} kHz -> audible {rel:7.1f} dB (limit {ALIAS_LIMIT_DB:.0f})")
        else:
            ok = abs(rel) <= PASS_TOL_DB and abs(pk - fe) / fe < 0.01
            print(f"{'PASS' if ok else 'FAIL'} pass  {name:24s} f0 {f0:7.0f} rate {rate:7.3f}: "
                  f"{fe/1000:6.2f} kHz level {rel:+6.2f} dB, peak {pk/1000:.2f} kHz")
        fails += not ok

    if EXPECT_FAIL:
        ok = alias_fails > 0
        print("POSITIVE CONTROL", "OK (alias cases fail)" if ok else "BROKEN (gate passed)")
        return 0 if ok else 1
    print("GATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
