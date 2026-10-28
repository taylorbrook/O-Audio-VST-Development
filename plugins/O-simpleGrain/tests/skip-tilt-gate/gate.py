# O-simpleGrain v1.8.0 gate: Grain Skip + Grain Tilt (research/granular-synthesis-
# 2026-update.md §5 item 6, Tempera 2.3 style). Runs on the INSTALLED VST3 in
# pedalboard; every render is a fresh instance.
#
# A  defaults are v1.7.0. Template states are written by the OLD binary (no
#    grainSkip / grainTilt in them), then rendered by OLD, by a fresh NEW, and by
#    a NEW instance that had Skip 60 / Tilt -70 set before the state loaded (the
#    old-session migration must reset them). Three configs: built-in Fire at
#    defaults; every spray + Scan + Grain Pitch +7 (band-limited path) on Tukey;
#    rect past the grain cap (500 ms x 200 g/s, steals). Verdict: bit-exact.
#    Live controls: Skip 30 and Tilt +50 must each differ.
# B  Skip. A DC source makes every grain a clean burst (the output IS the window
#    times a constant), so grains are counted directly. Isolated grains (10 ms
#    every 25 ms), Pan Spray 100 so each burst carries its random pan.
#      - Skip 25/50/75: the count is (1-p) of Skip 0's within 4 sigma.
#      - every surviving burst starts on a Skip-0 onset and has the SAME pan as
#        the Skip-0 burst there (skip draws its own RNG slot; nothing shifts).
#      - Skip 100: exactly silent.
#      - Skip 50 renders bit-exactly at blocks 37 and 1024 vs 512.
# C  Tilt shape. One 100 ms Hann grain from the DC source: the envelope peak sits
#    at 0.5 + 0.48 t of the grain (t = tilt/100) within 0.5 %, and tilt +t is the
#    time reverse of -t (max diff <= 2e-3 of the peak, LUT interpolation).
# D  Click floor. Every shape x Tilt -100/+100 x grain 2/5/30/200/500 ms: the
#    steepest per-sample step of the envelope (DC source, peak-normalised) is no
#    steeper (x 1.05) than that shape's own symmetric 2 ms grain, whose halves are
#    1 ms — the floor tilt is built on (or the Rectangular 1 ms guard, if larger).
# E  Level. A white-noise source, incoherent cloud (Position Spray 100, Scatter
#    100, Pan Spray 0): RMS at Tilt -100 / +100 within 1.5 dB of Tilt 0 (Hann and
#    Tukey 50 under the cap, Hann past it), and Skip 50 within 1.5 dB of Skip 0
#    on a dense cloud (overlap 10 -> 5 that sound). Past the cap a +100 swell is
#    stolen before its peak and is genuinely quieter; the normalization never
#    boosts, so that one case is gated only on "not louder" and its level printed.
#
# usage: uv run --python 3.12 --with pedalboard --with numpy --with soundfile \
#            python -I gate.py OLD_v1.7.0.vst3 [NEW.vst3]
import base64
import io
import os
import re
import struct
import sys
import numpy as np
import pedalboard
import soundfile as sf

OLD = sys.argv[1]
NEW = sys.argv[2] if len(sys.argv) > 2 else \
    "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3"
FS = 48000
fails = 0


def verdict(ok, msg):
    global fails
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'} {msg}")


# --- state framing (same as tests/alias-gate/gate.py) -----------------------------
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


def edit_inner_xml(raw, fn):
    outer, _ = vc2_unpack(raw)
    comp = juce_b64_decode(re.search(r"<IComponent>([^<]*)</IComponent>", outer).group(1))
    xml, tail = vc2_unpack(comp)
    comp = vc2_pack(fn(xml), tail)
    return vc2_pack(re.sub(r"<IComponent>[^<]*</IComponent>",
                           lambda _: f"<IComponent>{juce_b64_encode(comp)}</IComponent>", outer))


def inner_xml(raw):
    outer, _ = vc2_unpack(raw)
    comp = juce_b64_decode(re.search(r"<IComponent>([^<]*)</IComponent>", outer).group(1))
    return vc2_unpack(comp)[0]


def with_source(raw, identity, x):
    b = io.BytesIO()
    sf.write(b, x.astype(np.float32), FS, subtype="FLOAT", format="WAV")
    data = base64.b64encode(b.getvalue()).decode()

    def fn(xml):
        xml = re.sub(r"<SOURCE[^>]*/>", "", xml)
        return re.sub(r"(</[^>]+>\s*)$", f'<SOURCE identity="{identity}" data="{data}"/>\\1', xml)
    return edit_inner_xml(raw, fn)


DC = np.full(10 * FS, 0.5)
NOISE = np.random.default_rng(1234).uniform(-0.5, 0.5, 10 * FS)


def set_value(p, key, value):
    """Bisect raw_value on the read-back value (Grain Size / Density are skewed)."""
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


def set_lin(p, key, value):
    par = p.parameters[key]
    par.raw_value = (value - par.min_value) / (par.max_value - par.min_value)


def build(plugin, nums=None, window=None, adsr=None, freeze=None, lin=None, source=None):
    p = pedalboard.load_plugin(plugin)
    for k, v in (nums or {}).items():
        set_value(p, k, v)
    for k, v in (lin or {}).items():
        set_lin(p, k, v)
    if window is not None:
        p.window = window
    if adsr is not None:
        p.adsr = adsr
    if freeze is not None:
        p.freeze = freeze
    if source is not None:
        p.raw_state = with_source(p.raw_state, f"dropped:{source[0]}.wav", source[1])
    return p


def render(plugin, state, events, dur, block=512, pre=None):
    p = pedalboard.load_plugin(plugin)
    for k, v in (pre or {}).items():
        set_lin(p, k, v)
    p.raw_state = state
    msgs = [(bytes([st, n, v]), (s + 0.25) / FS) for s, st, n, v in events]
    y = p(msgs, duration=dur, sample_rate=FS, num_channels=2, buffer_size=block, reset=True)
    assert np.isfinite(y).all()
    return y


def db(x):
    return 20 * np.log10(max(float(x), 1e-30))


# ===================================================================== A
def gate_a():
    print("\n== A  defaults bit-exact vs v1.7.0 (OLD-written states)")
    ev = [(1009, 0x90, 60, 100), (1022, 0x90, 48, 70), (1027, 0x90, 55, 120),
          (70417, 0x80, 48, 0), (90001, 0x90, 36, 90), (150333, 0x80, 60, 0),
          (150346, 0x80, 55, 0), (230011, 0x80, 36, 0)]
    cfgs = {
        "fire-defaults": dict(),
        "spray-tukey-up": dict(nums=dict(grain_size_ms=87.0, density_g_s=53.0, scan=73.0, position=20.0,
                                         position_spray=40.0, pan_spray=80.0, scatter=60.0,
                                         vel_density=50.0, pitch_spray_st=5.0, taper=30.0),
                               lin=dict(grain_pitch_st=7.0), window="Tukey"),
        "rect-past-cap": dict(nums=dict(grain_size_ms=500.0, density_g_s=200.0, scan=-61.0,
                                        position_spray=20.0), window="Rectangular"),
    }
    for name, kw in cfgs.items():
        st = build(OLD, **kw).raw_state
        assert "grainSkip" not in inner_xml(st)
        a = render(OLD, st, ev, 6.0)
        b = render(NEW, st, ev, 6.0)
        c = render(NEW, st, ev, 6.0, pre=dict(grain_skip=60.0, grain_tilt=-70.0))
        rms = db(np.sqrt(np.mean(a ** 2)))
        verdict(rms > -70, f"A {name}: reference RMS {rms:.1f} dBFS (not vacuous)")
        verdict(np.array_equal(a, b), f"A {name}: NEW fresh vs OLD bit-exact"
                f" (max diff {db(np.max(np.abs(a - b))):.1f} dBFS)")
        verdict(np.array_equal(a, c), f"A {name}: NEW pre-set Skip 60 / Tilt -70, old state loaded -> "
                f"bit-exact (migration resets them; max diff {db(np.max(np.abs(a - c))):.1f} dBFS)")
        if name == "spray-tukey-up":
            for k, v in (("grain_skip", 30.0), ("grain_tilt", 50.0)):
                p = pedalboard.load_plugin(NEW); p.raw_state = st; set_lin(p, k, v)
                d = render(NEW, p.raw_state, ev, 6.0)
                verdict(np.max(np.abs(d - a)) > 1e-3, f"A {name}: live control {k}={v:g} differs "
                        f"(max diff {db(np.max(np.abs(d - a))):.1f} dBFS)")


# ===================================================================== B
def bursts(y, thresh=1e-6):
    """(start, end, pan) of each contiguous non-silent region."""
    m = np.abs(y).max(axis=0) > thresh
    d = np.diff(np.concatenate([[0], m.astype(np.int8), [0]]))
    starts, ends = np.where(d == 1)[0], np.where(d == -1)[0]
    out = []
    for s, e in zip(starts, ends):
        L, R = np.sum(np.abs(y[0, s:e])), np.sum(np.abs(y[1, s:e]))
        out.append((int(s), int(e), float(R / (L + R + 1e-30))))
    return out


def gate_b():
    print("\n== B  Skip: count, onset subset, pan identity, silence, block size")
    base = dict(nums=dict(grain_size_ms=10.0, density_g_s=40.0, position=50.0, pan_spray=100.0),
                adsr=False, freeze=True, source=("dc", DC))
    ev = [(4800, 0x90, 60, 127), (int(9.5 * FS), 0x80, 60, 0)]
    template = build(NEW, **base)
    seeded = template.raw_state

    def at(skip, block=512):
        p = pedalboard.load_plugin(NEW); p.raw_state = seeded; set_lin(p, "grain_skip", skip)
        return render(NEW, p.raw_state, ev, 10.0, block=block)

    y0 = at(0.0)
    b0 = bursts(y0)
    n0 = len(b0)
    verdict(n0 > 300, f"B Skip 0: {n0} isolated grains (expect ~{int(9.4 * 40)})")
    onsets0 = {s: pan for s, _, pan in b0}
    for p in (0.25, 0.5, 0.75):
        y = at(100 * p)
        b = bursts(y)
        n = len(b)
        exp = (1 - p) * n0
        sig = np.sqrt(n0 * p * (1 - p))
        verdict(abs(n - exp) <= 4 * sig, f"B Skip {100 * p:.0f}: {n} grains, expected {exp:.0f} "
                f"+- {4 * sig:.0f} (4 sigma)")
        sub = all(s in onsets0 for s, _, _ in b)
        pan = max((abs(onsets0[s] - pn) for s, _, pn in b if s in onsets0), default=1.0)
        verdict(sub and pan < 1e-6, f"B Skip {100 * p:.0f}: every grain on a Skip-0 onset ({sub}) "
                f"with the same pan (max pan diff {pan:.1e})")
    y1 = at(100.0)
    verdict(np.max(np.abs(y1)) == 0.0, "B Skip 100: exactly silent")
    r = at(50.0)
    for bs in (37, 1024):
        d = np.max(np.abs(at(50.0, bs) - r))
        verdict(d == 0.0, f"B Skip 50 block {bs} vs 512: bit-exact (max diff {db(d):.1f} dBFS)")
    # a different seed (fresh template) must skip a different set
    other = build(NEW, **base).raw_state
    p2 = pedalboard.load_plugin(NEW); p2.raw_state = other; set_lin(p2, "grain_skip", 50.0)
    o = render(NEW, p2.raw_state, ev, 10.0)
    verdict(not np.array_equal(o, r), "B Skip 50: a different instance seed skips a different set")


# ===================================================================== C / D
def grain_env(window, tilt, size_ms, taper=50.0):
    p = build(NEW, nums=dict(grain_size_ms=size_ms, density_g_s=1.0, position=50.0, taper=taper,
                             output_level_db=0.0),
              lin=dict(grain_tilt=tilt), window=window, adsr=False, freeze=True, source=("dc", DC))
    dur = max(0.3, 2.5 * size_ms / 1000 + 0.1)
    y = render(NEW, p.raw_state, [(480, 0x90, 60, 127)], dur)
    m = y[0]
    nz = np.where(np.abs(m) > 0)[0]
    first = m[nz[0]:]
    end = np.where(first == 0)[0]
    g = first[:end[0]] if len(end) else first
    return g / np.max(np.abs(g)), size_ms * FS / 1000


def gate_c():
    print("\n== C  Tilt moves the window peak (Hann, 100 ms, DC source)")
    envs = {}
    for t in (-100, -50, 0, 50, 100):
        g, L = grain_env("Hann", float(t), 100.0)
        envs[t] = g
        pk = np.argmax(g) / L
        want = 0.5 + 0.48 * t / 100
        verdict(abs(pk - want) <= 0.005, f"C Tilt {t:+4d}: peak at {pk:.4f} of the grain (want {want:.2f})")
    for t in (50, 100):
        a, b = envs[t], envs[-t][::-1]
        n = min(len(a), len(b))
        # align on the peak (a one-sample offset from the window's discrete end is not a shape error)
        sh = int(np.argmax(a[:n])) - int(np.argmax(b[:n]))
        b2 = np.roll(b[:n], sh)
        d = np.max(np.abs(a[:n] - b2)[5:-5])
        verdict(d <= 2e-3, f"C Tilt +{t} is the time reverse of -{t}: max diff {d:.1e} (shift {sh})")


SHAPES = ["Rectangular", "Triangular", "Welch", "Gaussian", "Hann", "Tukey"]


def step(g):
    return float(np.max(np.abs(np.diff(np.concatenate([[0], g, [0]])))))


def gate_d():
    print("\n== D  click floor: a tilted extreme is no steeper than the shape's own 2 ms grain (x1.05)")
    # The floor: the steep side never gets shorter than 1 ms, which is exactly one half
    # of the shortest grain (2 ms) every shape already plays symmetrically. Reference =
    # that shape's tilt-0 2 ms step, or the Rectangular 1 ms guard if larger.
    rect2 = step(grain_env("Rectangular", 0.0, 2.0)[0])
    refs = {sh: max(rect2, step(grain_env(sh, 0.0, 2.0)[0])) for sh in SHAPES}
    print("   references (peak-normalised step / sample): "
          + ", ".join(f"{sh[:4]} {refs[sh]:.4f}" for sh in SHAPES))
    for size in (2.0, 5.0, 30.0, 200.0, 500.0):
        worst, worst_at = 0.0, ""
        for sh in SHAPES:
            for t in (-100.0, 100.0):
                st = step(grain_env(sh, t, size)[0])
                r = st / refs[sh]
                if r > worst:
                    worst, worst_at = r, f"{sh} {t:+.0f}"
                if r > 1.05:
                    verdict(False, f"D {size:g} ms {sh} tilt {t:+.0f}: step {st:.4f} > ref {refs[sh]:.4f}")
        verdict(worst <= 1.05, f"D {size:5g} ms: worst tilted step / reference = {worst:.3f} ({worst_at})")


# ===================================================================== E
def level(window, tilt=0.0, skip=0.0, size=50.0, dens=100.0, taper=50.0):
    p = build(NEW, nums=dict(grain_size_ms=size, density_g_s=dens, position=50.0, position_spray=100.0,
                             scatter=100.0, taper=taper, output_level_db=0.0),
              lin=dict(grain_tilt=tilt, grain_skip=skip), window=window, adsr=False,
              source=("noise", NOISE))
    y = render(NEW, p.raw_state, [(480, 0x90, 60, 127)], 8.0)
    m = y[:, int(1.5 * FS):int(7.5 * FS)]
    return db(np.sqrt(np.mean(m ** 2)))


def gate_e():
    print("\n== E  level: tilt and skip keep the density normalization (noise source, incoherent)")
    for name, kw in (("Hann 50 ms x 100/s (overlap 5)", dict(window="Hann")),
                     ("Tukey 50 % 50 ms x 100/s", dict(window="Tukey")),
                     ("Hann 500 ms x 200/s (past the cap)", dict(window="Hann", size=500.0, dens=200.0))):
        l0 = level(tilt=0.0, **kw)
        for t in (-100.0, 100.0):
            lt = level(tilt=t, **kw)
            if "past" in name and t > 0:
                # Past the cap every grain is stolen at phase cap/overlap (24/100 here),
                # so a reverse swell is cut before it rises: it really is quieter, and the
                # normalization never boosts (by design). Gate only that it is not LOUDER.
                verdict(lt - l0 <= 1.5, f"E {name}: Tilt {t:+.0f} {lt - l0:+.2f} dB vs Tilt 0 "
                        f"(INFO: swells stolen before their peak; gated only on not louder)")
                continue
            verdict(abs(lt - l0) <= 1.5, f"E {name}: Tilt {t:+.0f} {lt - l0:+.2f} dB vs Tilt 0 ({l0:.1f} dBFS)")
    l0 = level("Hann", size=100.0, dens=100.0)
    l5 = level("Hann", size=100.0, dens=100.0, skip=50.0)
    verdict(abs(l5 - l0) <= 1.5, f"E Hann 100 ms x 100/s: Skip 50 {l5 - l0:+.2f} dB vs Skip 0 ({l0:.1f} dBFS)")


if __name__ == "__main__":
    print(f"OLD {OLD}\nNEW {NEW}")
    only = os.environ.get("ONLY", "ABCDE")
    for k, f in (("A", gate_a), ("B", gate_b), ("C", gate_c), ("D", gate_d), ("E", gate_e)):
        if k in only:
            f()
    print("\nGATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    sys.exit(1 if fails else 0)
