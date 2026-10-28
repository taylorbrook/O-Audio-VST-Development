# O-TextureForge alias gate (v1.7.0): up-transposed grains read the grain-engine
# band-limited mipmap instead of the linear interpolator.
#
# Offline, on the scheduler itself (alias.cpp): a 10 s sine at 48 kHz, one isolated
# 60 ms Hann grain in Pitch-Mapped mode, rate 2^((note - 60) / 12). The engine's
# only rate source is the MIDI note (whole semitones, up to +67 st = rate 48.5,
# mip level 5). The ideal output is one tone burst at rate * f0, or silence once
# that is past Nyquist; everything else in 20 Hz .. 20 kHz is spurious.
#
# Verdicts, relative to the energy of the same sine at rate 1 (note 60):
#   alias   rate * f0 > 24 kHz: total audible energy <= -60 dB
#   pass    rate * f0 ~ 9 kHz: energy within +-1.5 dB, peak within 1 % of rate * f0
#   info    ~16 kHz roll-off, printed for both versions
#   far     band-limited read 95 s and 6.4 min into a periodic corpus nulls the
#           same grain near the start (<= -120 dBFS): read precision does not
#           degrade with position
# The v1.6.2 build (tfa_old) runs the same cases as the positive control: its
# alias cases must fail.
#
# usage: OUT=<dir> ./build.sh && OUT=<dir> python3 gate.py
import os
import subprocess
import sys
import numpy as np

H = os.environ.get("OUT", os.path.join(os.environ.get("TMPDIR", "/tmp"), "tfa"))
REN = os.path.join(H, "renders"); os.makedirs(REN, exist_ok=True)
FS = 48000
ALIAS_LIMIT_DB = -60.0
PASS_TOL_DB = 1.5

# (kind, name, f0, note). Levels / sets: rate r -> level floor(log2 r), set from r / 2^level.
CASES = [
    ("alias", "+7 st (L0 S4)",    20000.0,  67),   # 29.97 k
    ("alias", "+11 st (L0 S6)",   16000.0,  71),   # 30.20 k
    ("alias", "+12 st (L1 S0)",   15000.0,  72),   # 30.00 k
    ("alias", "+13 st (L1 S1)",   15000.0,  73),   # 31.78 k
    ("alias", "+23 st (L1 S6)",    8000.0,  83),   # 30.20 k
    ("alias", "+25 st (L2 S1)",    7500.0,  85),   # 31.78 k
    ("alias", "+35 st (L2 S6)",    4000.0,  95),   # 30.20 k
    ("alias", "+37 st (L3 S1)",    4000.0,  97),   # 33.90 k
    ("alias", "+47 st (L3 S6)",    2000.0, 107),   # 30.20 k
    ("alias", "+49 st (L4 S1)",    2000.0, 109),   # 33.90 k
    ("alias", "+59 st (L4 S6)",    1000.0, 119),   # 30.20 k
    ("alias", "+61 st (L5 S1)",    1000.0, 121),   # 33.90 k
    ("alias", "+67 st (L5 S4)",     700.0, 127),   # 33.94 k
    ("pass",  "+7 st (L0 S4)",     6000.0,  67),   # 8.99 k
    ("pass",  "+11 st (L0 S6)",    4800.0,  71),   # 9.06 k
    ("pass",  "+13 st (L1 S1)",    4200.0,  73),   # 8.90 k
    ("pass",  "+35 st (L2 S6)",    1200.0,  95),   # 9.06 k
    ("pass",  "+49 st (L4 S1)",     500.0, 109),   # 8.48 k
    ("pass",  "+67 st (L5 S4)",     180.0, 127),   # 8.73 k
    ("info",  "+11 st (L0 S6)",    8500.0,  71),   # 16.04 k
    ("info",  "+13 st (L1 S1)",    7600.0,  73),   # 16.10 k
    ("info",  "+67 st (L5 S4)",     330.0, 127),   # 16.00 k
]


def render(eng, f0, note):
    f = os.path.join(REN, f"{eng}_{int(f0)}_{note}.f32")
    subprocess.run([os.path.join(H, f"tfa_{eng}"), "grain", str(f0), str(note), f], check=True)
    y = np.fromfile(f, dtype=np.float32).astype(np.float64)
    assert np.isfinite(y).all()
    spec = np.abs(np.fft.rfft(y * np.hanning(len(y)))) ** 2
    fr = np.fft.rfftfreq(len(y), 1 / FS)
    band = (fr >= 20.0) & (fr <= 20000.0)
    return float(np.sum(spec[band])), float(fr[np.argmax(spec * band)])


def db(r): return 10.0 * np.log10(max(r, 1e-30))


def run(eng):
    print(f"== {eng} engine")
    fails = alias_fails = 0
    refs = {}
    rows = {}
    for kind, name, f0, note in CASES:
        if f0 not in refs:
            e, pk = render(eng, f0, 60)
            assert abs(pk - f0) / f0 < 0.01 and e > 0, f"rate-1 reference {f0}: peak {pk}"
            refs[f0] = e
        rate = 2.0 ** ((note - 60) / 12.0)
        fe = rate * f0
        e, pk = render(eng, f0, note)
        rel = db(e / refs[f0])
        rows[(kind, name, f0)] = rel
        if kind == "info":
            print(f"INFO  16k   {name:16s} f0 {f0:7.0f} rate {rate:7.3f}: {fe/1000:6.2f} kHz level {rel:+6.2f} dB")
            continue
        if kind == "alias":
            ok = rel <= ALIAS_LIMIT_DB
            alias_fails += not ok
            print(f"{'PASS' if ok else 'FAIL'} alias {name:16s} f0 {f0:7.0f} rate {rate:7.3f}: "
                  f"{fe/1000:6.2f} kHz -> audible {rel:7.1f} dB (limit {ALIAS_LIMIT_DB:.0f})")
        else:
            ok = abs(rel) <= PASS_TOL_DB and abs(pk - fe) / fe < 0.01
            print(f"{'PASS' if ok else 'FAIL'} pass  {name:16s} f0 {f0:7.0f} rate {rate:7.3f}: "
                  f"{fe/1000:6.2f} kHz level {rel:+6.2f} dB, peak {pk/1000:.2f} kHz")
        fails += not ok
    return fails, alias_fails


def far(eng, off, note):
    out = subprocess.run([os.path.join(H, f"tfa_{eng}"), "far", str(off), str(note)],
                         capture_output=True, text=True, check=True).stdout.split()
    return [float(x) for x in out]


def main():
    fails, _ = run("new")
    print("== far grain (new engine): band-limited read precision deep in a long corpus")
    for off in (4194304, 16900000):
        for note in (67, 89, 127):
            res, peak = far("new", off, note)
            ok = res <= -120 and peak > -20
            print(f"{'PASS' if ok else 'FAIL'} far offset {off} note {note}: residual {res:.1f} dBFS, peak {peak:.1f} dBFS")
            fails += not ok
    _, old_alias_fails = run("old")
    ctrl = old_alias_fails > 0
    print("POSITIVE CONTROL (v1.6.2 alias cases fail):", f"OK ({old_alias_fails} fail)" if ctrl else "BROKEN")
    fails += not ctrl
    print("GATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
