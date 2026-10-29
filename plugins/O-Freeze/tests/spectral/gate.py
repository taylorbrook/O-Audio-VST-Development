# O-Freeze v2.9.0 Spectral engine gates (milestone spectral-diffusion-freeze,
# RESEARCH § Objective Metrics). Hosts the VST3 in pedalboard (fresh instance per
# render, fixed seed via a template state) and measures with numpy/scipy.
#
#   signal    frozen output carries signal: chord + pink noise, 48 k and 192 k
#   level     steady RMS within +-3 dB of beta = 0 over beta {0,.25,.5,.75,1}
#             (chord, pink noise) - the D2 makeup refit
#   engines   Granular vs Spectral (beta 0) RMS within 1 dB on the gate stimulus
#   monotone  chord: peak-to-floor strictly down, hop-phase circular variance
#             strictly up over beta; beta 0 CV < 0.05
#   metallic  pink noise, beta 1, drift 0: residual std (PSD_dB - 1/3-oct smoothed)
#             N = 8 <= 0.6 x N = 1
#   hopam     beta 0 chord: envelope AM at fs/H and 2 fs/H < 0.5 dB
#   clicks    engage / release / engine switch while frozen: max |2nd difference|
#             <= 1.5 x the same motion with Mix 0 (pure chord source: a noise
#             channel makes a curvature probe vacuous)
#
# usage: uv run --no-project --python 3.12 --with pedalboard --with numpy --with scipy \
#            python -I gate.py [plugin.vst3] [--only=a,b,...]
import sys

import numpy as np
import pedalboard
from scipy import signal as sps

args = [a for a in sys.argv[1:] if not a.startswith("--")]
ONLY = None
for a in sys.argv[1:]:
    if a.startswith("--only="):
        ONLY = set(a.split("=", 1)[1].split(","))
PLUGIN = args[0] if args else "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-Freeze-dev.vst3"

W, H = 8192, 2048
CHORD = [220.0, 277.18, 329.63, 466.16]
BETAS = [0.0, 0.25, 0.5, 0.75, 1.0]
_template_cache = {}


def db(v):
    return 20.0 * np.log10(max(float(v), 1e-30))


def rms_db(y):
    return db(np.sqrt(np.mean(np.asarray(y, dtype=np.float64) ** 2)))


def chord(seconds, fs, amp=0.12):
    t = np.arange(int(seconds * fs)) / fs
    x = sum(amp * np.sin(2 * np.pi * f * t + p) for f, p in zip(CHORD, [0.0, 1.0, 2.0, 0.5]))
    return np.stack([x, x]).astype(np.float32)


def pink(seconds, fs, seed=5, level_db=-20.0):
    rng = np.random.default_rng(seed)
    n = int(seconds * fs)
    out = []
    for _ in range(2):
        spec = np.fft.rfft(rng.standard_normal(n))
        f = np.fft.rfftfreq(n, 1 / fs)
        f[0] = f[1]
        x = np.fft.irfft(spec / np.sqrt(f), n)
        out.append(x / np.sqrt(np.mean(x ** 2)) * 10 ** (level_db / 20))
    return np.stack(out).astype(np.float32)


def gate_stimulus(seconds, fs, seed=11):
    # tests/blocksize-null stimulus: chord + sweep + noise
    rng = np.random.default_rng(seed)
    n = int(seconds * fs)
    t = np.arange(n) / fs
    c = sum(0.12 * np.sin(2 * np.pi * f * t + p) for f, p in zip(CHORD, [0.0, 1.0, 2.0, 0.5]))
    sweep = 0.1 * np.sin(2 * np.pi * (200 * t + 300 * t * t / seconds))
    x = np.stack([c + sweep, c - 0.5 * sweep]) + 0.05 * rng.standard_normal((2, n))
    return x.astype(np.float32)


def set_raw(p, key, value):
    par = p.parameters[key]
    par.raw_value = (value - par.min_value) / (par.max_value - par.min_value)


def template():
    # one fixed seed for every render (a fresh instance draws its own; the state carries it)
    if "t" not in _template_cache:
        _template_cache["t"] = pedalboard.load_plugin(PLUGIN).raw_state
    return _template_cache["t"]


def make(engine="Spectral", beta=0.0, avg=8, drift=0.0, detune=0.0, lfo_depth=0.0, mix=100.0, **extra):
    p = pedalboard.load_plugin(PLUGIN)
    p.raw_state = template()
    p.mode = "Manual"
    p.engine = engine
    set_raw(p, "diffusion", beta * 100.0)
    set_raw(p, "avg_frames", avg)
    set_raw(p, "drift", drift)
    set_raw(p, "detune_cents", detune)
    set_raw(p, "lfo_depth", lfo_depth)
    set_raw(p, "mix", mix)
    for k, v in extra.items():
        set_raw(p, k, v)
    p.freeze = False
    return p


def render_freeze(p, x, fs, fill_s=2.0, hold_s=None, block=512, events=()):
    """fill (dry), freeze until hold end, release; events = [(time_s, attr, value)]
    applied between process calls (times relative to the freeze sample)."""
    a = int(fill_s * fs)
    b = x.shape[1] if hold_s is None else a + int(hold_s * fs)
    ys = [p.process(x[:, :a], fs, buffer_size=block, reset=True)]
    p.freeze = True
    cuts = sorted((a + int(t * fs), attr, val) for t, attr, val in events)
    pos = a
    for c, attr, val in cuts:
        ys.append(p.process(x[:, pos:c], fs, buffer_size=block, reset=False))
        setattr(p, attr, val)
        pos = c
    ys.append(p.process(x[:, pos:b], fs, buffer_size=block, reset=False))
    if b < x.shape[1]:
        p.freeze = False
        ys.append(p.process(x[:, b:], fs, buffer_size=block, reset=False))
    y = np.concatenate(ys, axis=1)
    assert np.isfinite(y).all(), "non-finite output"
    return y, a, b


FAILS = []


def verdict(ok, msg):
    print(("PASS " if ok else "FAIL ") + msg)
    if not ok:
        FAILS.append(msg)


# ---------------------------------------------------------------- signal
def t_signal():
    for fs in (48000, 192000):
        for name, stim in (("chord", chord), ("pink", pink)):
            x = stim(5.0, fs)
            y, a, b = render_freeze(make(), x, fs, hold_s=3.0)
            r = rms_db(y[:, a + fs // 2:b])
            verdict(r > -40.0 and abs(r - rms_db(x[:, :a])) < 6.0,
                    f"signal {name} @{fs}: frozen RMS {r:.1f} dBFS (source {rms_db(x[:, :a]):.1f})")


# ---------------------------------------------------------------- level vs beta
def steady_rms(beta, stim, fs=48000, **kw):
    x = stim(9.0, fs)
    y, a, b = render_freeze(make(beta=beta, **kw), x, fs, hold_s=6.0)
    return rms_db(y[:, a + fs:b])


def t_level():
    for name, stim in (("chord", chord), ("pink", pink)):
        lv = [steady_rms(bt, stim) for bt in BETAS]
        rel = [v - lv[0] for v in lv]
        verdict(max(abs(r) for r in rel) <= 3.0,
                f"level {name}: RMS vs beta {[f'{v:.2f}' for v in lv]} dBFS, re beta 0 "
                f"{[f'{r:+.2f}' for r in rel]} dB (limit +-3)")


# ---------------------------------------------------------------- engines
def t_engines():
    # Granular level varies with the seed (+-0.5 dB), so the comparison is the power
    # mean over N_SEEDS fresh seeds (Spectral at beta 0, drift 0, detune 0 draws nothing).
    fs = 48000
    x = gate_stimulus(9.0, fs)
    n_seeds = 6
    pg = []
    for k in range(n_seeds):
        _template_cache.clear()
        g, a, b = render_freeze(make(engine="Granular", lfo_depth=0.0), x, fs, hold_s=6.0)
        pg.append(np.mean(g[:, a + fs:b].astype(np.float64) ** 2))
    _template_cache.clear()
    s, a, b = render_freeze(make(engine="Spectral", beta=0.0), x, fs, hold_s=6.0)
    rg = 10 * np.log10(np.mean(pg))
    rs = rms_db(s[:, a + fs:b])
    spread = 10 * np.log10(max(pg)) - 10 * np.log10(min(pg))
    verdict(abs(rs - rg) <= 1.0, f"engines: Granular {rg:.2f} dBFS (power mean of {n_seeds} seeds, spread "
            f"{spread:.2f} dB) vs Spectral beta 0 {rs:.2f} dBFS (diff {rs - rg:+.2f} dB, limit 1)")


# ---------------------------------------------------------------- monotone
def chord_metrics(y, fs):
    m = y[0].astype(np.float64)
    f, P = sps.welch(m, fs, nperseg=65536, noverlap=32768, window="hann")
    df = f[1] - f[0]
    ptf = []
    for fc in CHORD:
        pk = P[np.abs(f - fc) <= 3.0].max()
        ring = (np.abs(f - fc) >= 15.0) & (np.abs(f - fc) <= 40.0)
        ptf.append(10 * np.log10(pk / P[ring].mean()))
    # hop-to-hop phase advance in each partial's bin of an output STFT (W, H, Hann)
    win = np.hanning(W + 1)[:-1]
    nfr = (len(m) - W) // H
    frames = np.stack([np.fft.rfft(m[i * H:i * H + W] * win) for i in range(nfr)])
    cv = []
    for fc in CHORD:
        k = int(round(fc * W / fs))
        dphi = np.angle(frames[1:, k] * np.conj(frames[:-1, k]))
        cv.append(1.0 - np.abs(np.mean(np.exp(1j * dphi))))
    return float(np.mean(ptf)), float(np.mean(cv))


def t_monotone():
    fs = 48000
    x = chord(10.0, fs)
    rows = []
    for bt in BETAS:
        y, a, b = render_freeze(make(beta=bt), x, fs, hold_s=7.0)
        rows.append(chord_metrics(y[:, a + fs:b], fs))
    ptf = [r[0] for r in rows]
    cv = [r[1] for r in rows]
    verdict(all(ptf[i] > ptf[i + 1] for i in range(len(ptf) - 1)),
            f"monotone peak-to-floor (dB) strictly down: {[f'{v:.1f}' for v in ptf]}")
    verdict(all(cv[i] < cv[i + 1] for i in range(len(cv) - 1)),
            f"monotone hop-phase circular variance strictly up: {[f'{v:.4f}' for v in cv]}")
    verdict(cv[0] < 0.05, f"beta 0 circular variance {cv[0]:.4f} < 0.05")


# ---------------------------------------------------------------- metallic
def residual_std(y, fs):
    m = y.astype(np.float64)
    f, P = sps.welch(m, fs, nperseg=W, noverlap=W // 2, window="hann", axis=-1)
    P = P.mean(axis=0)
    sel = (f >= 200.0) & (f <= 10000.0)
    sm = np.empty_like(P)
    for i, fc in enumerate(f):
        if fc <= 0:
            sm[i] = P[i]
            continue
        band = (f >= fc * 2 ** (-1 / 6)) & (f <= fc * 2 ** (1 / 6))
        sm[i] = P[band].mean()
    r = 10 * np.log10(P[sel]) - 10 * np.log10(sm[sel])
    return float(np.std(r))


def t_metallic():
    fs = 48000
    x = pink(12.0, fs)
    out = {}
    for n in (1, 8):
        y, a, b = render_freeze(make(beta=1.0, avg=n), x, fs, hold_s=9.0)
        out[n] = residual_std(y[:, a + fs:b], fs)
    # control: the same measurer on the dry pink noise (its own Welch variance)
    ctrl = residual_std(x[:, :int(9 * fs)], fs)
    verdict(out[8] <= 0.6 * out[1],
            f"metallic: residual std N=1 {out[1]:.2f} dB, N=8 {out[8]:.2f} dB "
            f"(ratio {out[8] / out[1]:.2f}, limit 0.6; dry pink control {ctrl:.2f} dB)")


# ---------------------------------------------------------------- hop-rate AM
def am_db(e, fs, f0):
    e = e - 0.0
    n = len(e)
    t = np.arange(n) / fs
    mean = e.mean()
    c = 2 * np.abs(np.mean(e * np.exp(-2j * np.pi * f0 * t)))
    m = c / mean
    return 20 * np.log10((1 + m) / max(1 - m, 1e-9))


def t_hopam():
    fs = 48000
    x = chord(10.0, fs)
    y, a, b = render_freeze(make(beta=0.0), x, fs, hold_s=7.0)
    seg = y[0, a + fs:b].astype(np.float64)
    env = np.abs(sps.hilbert(seg))
    dry = np.abs(sps.hilbert(x[0, :a].astype(np.float64)))
    res = []
    for mult in (1, 2):
        f0 = mult * fs / H
        res.append((f0, am_db(env, fs, f0), am_db(dry, fs, f0)))
    ok = all(r[1] < 0.5 for r in res)
    verdict(ok, "hop-rate AM (beta 0 chord): " + ", ".join(
        f"{f0:.2f} Hz {v:.3f} dB (dry control {c:.3f} dB)" for f0, v, c in res) + " (limit 0.5 dB)")


# ---------------------------------------------------------------- clicks
def curvature(y, i0, i1):
    d2 = np.abs(np.diff(y[:, i0:i1].astype(np.float64), n=2, axis=1))
    return float(d2.max())


def t_clicks():
    fs = 48000
    x = chord(8.0, fs)
    fill = 2.0
    cases = [
        # name, make kwargs, events, window start (s re freeze), window length (s)
        ("engage", dict(beta=0.3), [], 0.0, 0.060),
        ("release", dict(beta=0.3), [], None, 0.110),
        ("switch G->S", dict(engine="Granular", beta=0.3), [(1.5, "engine", "Spectral")], 1.5, 0.060),
        ("switch S->G", dict(engine="Spectral", beta=0.3), [(1.5, "engine", "Granular")], 1.5, 0.060),
    ]
    # An engine switch crossfades two frozen outputs, and the granular one alone can
    # carry up to ~1.4x the dry chord's curvature (seed-dependent: grain sums of the
    # chord at different lags). So a switch is gated against the larger of the Mix-0
    # same-motion render and each engine's own steady render over the same window;
    # the plain Mix-0 ratio is printed too.
    for name, kw, ev, w0, wl in cases:
        res = {}
        for mix in (100.0, 0.0):
            p = make(mix=mix, **kw)
            y, a, b = render_freeze(p, x, fs, fill_s=fill, hold_s=3.0, events=ev)
            start = (b if w0 is None else a + int(w0 * fs)) - int(0.005 * fs)
            res[mix] = curvature(y, start, start + int((wl + 0.010) * fs))
        ctrl = res[0.0]
        extra = ""
        if ev:
            steady = []
            for eng in ("Granular", "Spectral"):
                y, a, b = render_freeze(make(**dict(kw, engine=eng)), x, fs, fill_s=fill, hold_s=3.0)
                start = a + int(w0 * fs) - int(0.005 * fs)
                steady.append(curvature(y, start, start + int((wl + 0.010) * fs)))
            ctrl = max(ctrl, *steady)
            extra = (f"; steady Granular {steady[0]:.2e} / Spectral {steady[1]:.2e}, "
                     f"plain Mix-0 ratio {res[100.0] / res[0.0]:.2f}")
        ratio = res[100.0] / ctrl
        verdict(ratio <= 1.5, f"click {name}: max|d2| {res[100.0]:.2e} vs control {ctrl:.2e} "
                f"(Mix-0 same motion {res[0.0]:.2e}{extra}) ratio {ratio:.2f}, limit 1.5")


TESTS = dict(signal=t_signal, level=t_level, engines=t_engines, monotone=t_monotone,
             metallic=t_metallic, hopam=t_hopam, clicks=t_clicks)

if __name__ == "__main__":
    print(f"plugin: {PLUGIN}")
    for name, fn in TESTS.items():
        if ONLY is None or name in ONLY:
            fn()
    print("GATE", "PASS" if not FAILS else f"FAIL ({len(FAILS)})")
    sys.exit(1 if FAILS else 0)
