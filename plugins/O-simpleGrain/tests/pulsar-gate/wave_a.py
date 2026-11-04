# O-simpleGrain v2.0.0 Pulsar mode — Wave A DSP checks (A3 / A4 / A5) on a built VST3.
# Every render is a fresh instance; settings are applied by bisecting raw_value on
# the read-back value (skewed ranges), choices by name.
#
#   A3  f_p      MIDI 57 (220 Hz), Formant 1x, Sine: FFT fundamental within 0.1 cent at
#                44.1 / 48 / 96 kHz. Liveness: the estimator on a synthetic 220 Hz sine is
#                < 0.01 cent off, and a +1 cent Grain Pitch render must FAIL the 0.1 cent bound.
#       formant  Formant 2 -> 8x at 220 Hz: spectral centroid rises monotonically, f_p stays.
#       alias    C8 (and C7, A6) x Formant 32 at 44.1 kHz: the largest non-harmonic
#                component <= -60 dB re the largest harmonic. STIMULUS CONTROL: the
#                --control build (ceiling 0.45 fs) must FAIL the C8 probe.
#       bh-band  110 Hz, Sine, L 0.5 .. 1.6 ms (the 0.75-1.25 ms Hann -> BH band): level per
#                duty, P / d, varies <= 0.5 dB. CONTROL: the --control build has no per-window
#                power gains and must FAIL (proves the BH band engages and the gains work).
#   A4  alias      Sample content (fire / voice / piano), MIDI 108/96/84 x 32, 72 x 8 at 44.1 kHz:
#                  <= -60 dB; STIMULUS CONTROL: the 0.45 fs build must fail. (The Sine<->Sample
#                  switch click and Sine-without-source checks are in tests/render-harness.)
#   A5  mask       Mask 3 puts a line at f_p / 3 (absent with Mask Off).
#       skip       Skip p: (1 - p) of the pulses within 4 sigma, all on Skip-0 onsets.
#       FM         integer ratios leave the spectrum unchanged (onsets stay 1/f_p apart -
#                  the sidebands land on the harmonics); ratios 0.5 / 1.5 put a line at f_p / 2;
#                  the mean pulse rate stays f_p at depth 100 %.
#       glisson    a one-cycle Sine pulsaret's half-cycle ratio: 1 at 0, 1.299 at +12 st
#                  (rising), 0.770 at -12 st.
#       alias      sweep C8/E7/F#6 x 32 x FM 100 % x glisson 0/+24, both contents: <= -60 dB;
#                  Pitch Spray 12: the 0.47..0.5 fs band <= -60 dB (control build must fail).
#       neutral    --absent BUILD (Mask/Skip/FM/Glisson code removed): neutral settings render
#                  bit-identically; each feature moved off neutral differs.
#
# usage: uv run --no-project --python 3.12 --with pedalboard --with numpy --with soundfile \
#            python -I wave_a.py PLUGIN.vst3 [--control CTRL.vst3] [--absent ABSENT.vst3] [--only A3,A4,A5]
import os
import re
import sys
import numpy as np
import pedalboard

ARGV = sys.argv[1:]
def opt(name, default=None):
    if name in ARGV:
        i = ARGV.index(name); v = ARGV[i + 1]; del ARGV[i:i + 2]; return v
    return default
CONTROL = opt("--control")
ABSENT = opt("--absent")       # build with the Mask/Skip/FM/Glisson code removed (A5 neutral null)
ONLY = (opt("--only", "A3,A4,A5")).split(",")
PLUGIN = ARGV[0]
fails = 0

# state framing helpers (VC2! / JUCE base64) from the skip-tilt gate
_argv = sys.argv
sys.argv = [_argv[0], PLUGIN, PLUGIN]
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "skip-tilt-gate"))
import gate as G          # noqa: E402  (helpers only)
sys.argv = _argv


def verdict(ok, msg):
    global fails
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'} {msg}", flush=True)


def db(x):
    return 20 * np.log10(max(float(x), 1e-30))


def key(p, name):
    if name in p.parameters:
        return name
    ks = [k for k in p.parameters if k.startswith(name + "_")]
    assert len(ks) == 1, (name, ks)
    return ks[0]


def set_value(p, name, value):
    k = key(p, name)
    par = p.parameters[k]
    if isinstance(value, str) or isinstance(value, bool):
        setattr(p, k, value)
        return
    lo, hi = 0.0, 1.0
    for _ in range(48):
        mid = 0.5 * (lo + hi)
        par.raw_value = mid
        if float(getattr(p, k)) < value:
            lo = mid
        else:
            hi = mid
    par.raw_value = hi


BASE = dict(grain_mode="Pulsar", pulsaret="Sine", output_level=0.0, amp_sustain=1.0)


SOURCES = ("fire", "voice", "water", "piano")


def load(plugin, settings, state=None):
    """Fresh instance with settings. A built-in source is selected through the state
    (identity embedded:<name>): the parameter alone only queues an async decode that
    pedalboard never pumps, so it would silently stay on fire."""
    p = pedalboard.load_plugin(plugin)
    if state is not None:
        p.raw_state = state
    settings = {**BASE, **settings}
    src = settings.pop("source", None)
    for k, v in settings.items():
        set_value(p, k, v)
    if src is not None:
        idx = SOURCES.index(src)
        def fn(xml):
            xml = re.sub(r'<SOURCE identity="[^"]*"', f'<SOURCE identity="embedded:{src}"', xml)
            return re.sub(r'(<PARAM id="sourceSample" value=")[^"]*"', rf'\g<1>{idx}.0"', xml)
        st = G.edit_inner_xml(p.raw_state, fn)
        p = pedalboard.load_plugin(plugin)
        p.raw_state = st
        assert getattr(p, key(p, "source")) == src
    return p


def render(plugin, settings, notes, dur, fs=48000, block=512, events=None, state=None):
    p = load(plugin, settings, state)
    if events is None:
        events = [(bytes([0x90, n, 100]), 0.0) for n in notes]
    y = p(events, duration=dur, sample_rate=fs, num_channels=2, buffer_size=block, reset=True)
    assert np.isfinite(y).all(), "non-finite output"
    return y


def bh(n):
    t = np.arange(n) / n
    return 0.35875 - 0.48829 * np.cos(2 * np.pi * t) + 0.14128 * np.cos(4 * np.pi * t) - 0.01168 * np.cos(6 * np.pi * t)


def spectrum(x, fs, pad=8):
    n = len(x)
    N = 1 << int(np.ceil(np.log2(n * pad)))
    X = np.abs(np.fft.rfft(x * bh(n), N))
    return X, fs / N


def peak_near(x, fs, f, width):
    X, df = spectrum(x, fs)
    lo, hi = int((f - width) / df), int((f + width) / df) + 1
    i = lo + int(np.argmax(X[lo:hi]))
    a, b, c = np.log(X[i - 1] + 1e-300), np.log(X[i] + 1e-300), np.log(X[i + 1] + 1e-300)
    off = 0.5 * (a - c) / (a - 2 * b + c)
    return (i + off) * df


def cents(f, ref):
    return 1200 * np.log2(f / ref)


def centroid(x, fs):
    X, df = spectrum(x, fs, pad=1)
    f = np.arange(len(X)) * df
    m = f > 20
    return float(np.sum(f[m] * X[m] ** 2) / np.sum(X[m] ** 2))


def alias_db(x, fs, f0guess):
    """Largest non-harmonic component re the largest harmonic (dB). The steady pulse
    train is exactly periodic at f_p, so anything off the harmonic comb is folded."""
    f0 = peak_near(x, fs, f0guess, 0.02 * f0guess)
    X, df = spectrum(x, fs, pad=2)
    f = np.arange(len(X)) * df
    k = np.round(f / f0)
    near = np.abs(f - k * f0) <= 6.0 * fs / len(x)   # BH main lobe: +-4 natural bins (fs / n), +2 margin
    harm = near & (k >= 1)
    spur = ~near & (f > 30.0)
    return db(X[spur].max() / X[harm].max()), f[spur][np.argmax(X[spur])]


def mono(y):
    return 0.5 * (y[0] + y[1])


# ------------------------------------------------------------------------------- A3
def a3(plugin, control=None):
    print("\n== A3  pulse clock, Sine pulsarets, duty clamp, BH band")
    # estimator liveness
    fs = 48000
    t = np.arange(int(2.0 * fs)) / fs
    est = peak_near(np.sin(2 * np.pi * 220.0 * t), fs, 220.0, 5.0)
    verdict(abs(cents(est, 220.0)) < 0.01, f"A3 estimator: synthetic 220 Hz sine reads {cents(est, 220.0):+.4f} cent")

    for fs in (44100, 48000, 96000):
        y = mono(render(plugin, dict(formant=1.0), [57], 3.0, fs))
        f = peak_near(y[int(1.0 * fs):], fs, 220.0, 5.0)
        verdict(abs(cents(f, 220.0)) <= 0.1, f"A3 f_p: MIDI 57, Formant 1x @ {fs} Hz -> {f:.4f} Hz ({cents(f, 220.0):+.4f} cent, bound 0.1)")
    y = mono(render(plugin, dict(formant=1.0, grain_pitch=0.01), [57], 3.0, 48000))
    f = peak_near(y[48000:], 48000, 220.0, 5.0)
    verdict(abs(cents(f, 220.0)) > 0.5, f"A3 f_p control: Grain Pitch +1 cent reads {cents(f, 220.0):+.4f} cent (must miss the 0.1 cent bound)")

    fs = 48000
    cs, fps = [], []
    for F in (2.0, 3.0, 4.0, 6.0, 8.0):
        y = mono(render(plugin, dict(formant=F), [57], 2.0, fs))[fs:]
        cs.append(centroid(y, fs)); fps.append(cents(peak_near(y, fs, 220.0, 5.0), 220.0))
    mono_up = all(b > a * 1.05 for a, b in zip(cs, cs[1:]))
    verdict(mono_up, "A3 formant: centroid " + " < ".join(f"{c:.0f}" for c in cs) + " Hz for Formant 2/3/4/6/8x")
    verdict(max(abs(c) for c in fps) <= 0.1, "A3 formant: f_p stays at 220 Hz: " + ", ".join(f"{c:+.3f}" for c in fps) + " cent")

    fs = 44100
    for note in (108, 96, 93):
        a, fa = alias_db(mono(render(plugin, dict(formant=32.0), [note], 2.0, fs))[fs:], fs,
                         440 * 2 ** ((note - 69) / 12))
        verdict(a <= -60.0, f"A3 alias: MIDI {note} x Formant 32 @ 44.1 kHz: worst non-harmonic {a:.1f} dB at {fa:.0f} Hz (bound -60)")
    if control:
        a, fa = alias_db(mono(render(control, dict(formant=32.0), [108], 2.0, fs))[fs:], fs, 4186.01)
        verdict(a > -60.0, f"A3 alias STIMULUS CONTROL (ceiling 0.45 fs): MIDI 108 x 32 -> {a:.1f} dB at {fa:.0f} Hz (must exceed -60)")

    def band(pl):
        fs, fp = 48000, 110.0
        out = []
        for Lms in (0.5, 0.6, 0.7, 0.75, 0.8, 0.9, 1.0, 1.1, 1.2, 1.25, 1.3, 1.4, 1.6):
            F = 1000.0 / (Lms * fp)
            y = render(pl, dict(formant=F), [45], 1.5, fs)[:, int(0.5 * fs):]
            P = float(np.mean(y ** 2))
            out.append((Lms, 10 * np.log10(P * F)))          # P / d, d = 1 / F
        return out
    rows = band(plugin)
    e = np.array([r[1] for r in rows])
    spread = float(e.max() - e.min())
    verdict(spread <= 0.5, "A3 bh-band: level per duty over L 0.5..1.6 ms spans "
            f"{spread:.3f} dB (bound 0.5): " + " ".join(f"{L}:{v - e[-1]:+.2f}" for L, v in rows))
    if control:
        ec = np.array([r[1] for r in band(control)])
        sc = float(ec.max() - ec.min())
        verdict(sc > 0.5, f"A3 bh-band CONTROL (no power gains): spans {sc:.2f} dB (must exceed 0.5)")


# ------------------------------------------------------------------------------- A4
def a4(plugin, control=None):
    print("\n== A4  sample-content pulsarets")
    fs = 44100
    for src in ("fire", "voice", "piano"):
        worst = []
        for note, F in ((108, 32.0), (96, 32.0), (84, 32.0), (72, 8.0)):
            a, fa = alias_db(mono(render(plugin, dict(pulsaret="Sample", source=src, formant=F), [note], 2.5, fs))[int(1.5 * fs):],
                             fs, 440 * 2 ** ((note - 69) / 12))
            worst.append((a, note, F, fa))
        a, note, F, fa = max(worst)
        verdict(a <= -60.0, f"A4 alias: Sample '{src}', worst over MIDI 108/96/84 x 32, 72 x 8 @ 44.1 kHz: "
                f"{a:.1f} dB (MIDI {note} x {F:g}, at {fa:.0f} Hz; bound -60)")
    if control:
        a, fa = alias_db(mono(render(control, dict(pulsaret="Sample", source="piano", formant=32.0), [108], 2.5, fs))[int(1.5 * fs):],
                         fs, 4186.01)
        verdict(a > -60.0, f"A4 alias STIMULUS CONTROL (ceiling 0.45 fs): Sample 'piano' MIDI 108 x 32 -> {a:.1f} dB (must exceed -60)")
    # live: Sample content must differ from Sine and carry signal
    y1 = render(plugin, dict(pulsaret="Sample", source="fire", formant=4.0), [57], 1.0, 48000)
    y0 = render(plugin, dict(pulsaret="Sine", formant=4.0), [57], 1.0, 48000)
    verdict(db(np.sqrt(np.mean(y1 ** 2))) > -60 and np.max(np.abs(y1 - y0)) > 1e-3,
            f"A4 live: Sample content sounds ({db(np.sqrt(np.mean(y1 ** 2))):.1f} dBFS RMS) and differs from Sine")


# ------------------------------------------------------------------------------- A5
def bursts(x, thresh=1e-6, merge=16):
    m = np.abs(x) > thresh
    d = np.diff(np.concatenate([[0], m.astype(np.int8), [0]]))
    st, en = list(np.where(d == 1)[0]), list(np.where(d == -1)[0])
    out = []
    for a, b in zip(st, en):
        if out and a - out[-1][1] <= merge:
            out[-1][1] = b
        else:
            out.append([a, b])
    return out


def a5(plugin, control=None, absent=None):
    print("\n== A5  Mask, Skip, rate FM, Glisson")
    fs = 48000
    # --- Mask 3: a line at f_p / 3 (absent with Mask Off)
    def line_db(y, f):
        X, df = spectrum(y, fs)
        i = int(round(f / df))
        return db(X[i - 3:i + 4].max() / X.max())
    y3 = mono(render(plugin, dict(formant=4.0, pulse_mask="3"), [57], 2.5, fs))[fs:]
    y0 = mono(render(plugin, dict(formant=4.0), [57], 2.5, fs))[fs:]
    l3, l0 = line_db(y3, 220.0 / 3), line_db(y0, 220.0 / 3)
    verdict(l3 > -30.0 and l0 < -80.0, f"A5 mask 3: line at f_p/3 = 73.3 Hz {l3:.1f} dB re peak (Mask Off: {l0:.1f} dB)")
    f3 = peak_near(y3, fs, 220.0 / 3, 2.0)
    verdict(abs(cents(f3, 220.0 / 3)) <= 0.1, f"A5 mask 3: the subharmonic sits at {f3:.4f} Hz ({cents(f3, 220.0 / 3):+.3f} cent)")

    # --- Skip: per pulse; survivors keep their Skip-0 onsets, count ~ (1 - p)
    st = dict(formant=4.0)
    ya = mono(render(plugin, st, [33], 3.0, fs))
    b0 = bursts(ya)
    for p in (0.25, 0.5, 0.75):
        yb = mono(render(plugin, dict(st, grain_skip=100 * p), [33], 3.0, fs))
        bp = bursts(yb)
        on0 = {b[0] for b in b0}
        aligned = all(b[0] in on0 for b in bp)
        n, exp = len(bp), (1 - p) * len(b0)
        sd = np.sqrt(len(b0) * p * (1 - p))
        verdict(abs(n - exp) <= 4 * sd and aligned,
                f"A5 skip {p:.2f}: {n} of {len(b0)} pulses sound (expect {exp:.0f} +- {4 * sd:.0f}), all on Skip-0 onsets: {aligned}")

    # --- Rate FM. Integer ratios: the modulator repeats every pulse period, so the onsets stay
    # exactly 1/f_p apart (only the first pulse's phase reference) -> spectrum unchanged.
    # Non-integer ratios move the onsets: ratio 0.5 / 1.5 put lines at odd multiples of f_p/2.
    def half_line(y):
        return line_db(y, 110.0)
    base = mono(render(plugin, dict(formant=4.0), [57], 2.5, fs))[fs:]
    for ratio in (1.0, 2.0):
        y = mono(render(plugin, dict(formant=4.0, fm_depth=60.0, fm_ratio=ratio), [57], 2.5, fs))[fs:]
        X, _ = spectrum(y, fs); B, _ = spectrum(base, fs)
        dev = db(np.max(np.abs(X - B)) / B.max())
        verdict(dev <= -100.0, f"A5 FM ratio {ratio:g} depth 60 %: inert, spectrum = no-FM spectrum within {dev:.1f} dB (bound -100) "
                "(integer ratio: sidebands f_p +- k ratio f_p land ON the harmonics; onsets stay 1/f_p apart)")
    for ratio in (0.5, 1.5):
        y = mono(render(plugin, dict(formant=4.0, fm_depth=60.0, fm_ratio=ratio), [57], 2.5, fs))[fs:]
        h = half_line(y)
        verdict(h > -40.0 and half_line(base) < -80.0,
                f"A5 FM ratio {ratio:g} depth 60 %: sideband at f_p - ratio f_p mod f_p = 110 Hz {h:.1f} dB re peak "
                f"(no FM: {half_line(base):.1f} dB)")
        fq = peak_near(y, fs, 110.0, 2.0)
        verdict(abs(cents(fq, 110.0)) <= 0.1, f"A5 FM ratio {ratio:g}: that sideband sits at {fq:.4f} Hz")
    # mean rate stays f_p under FM (count onsets of isolated pulses)
    yf = mono(render(plugin, dict(formant=8.0, fm_depth=95.0, fm_ratio=0.37), [33], 4.0, fs))
    yn = mono(render(plugin, dict(formant=8.0), [33], 4.0, fs))
    nf, nn = len(bursts(yf)), len(bursts(yn))
    verdict(abs(nf - nn) <= 2, f"A5 FM depth 100 % ratio 0.37: {nf} pulses vs {nn} without FM in 4 s (mean rate kept)")

    # --- Glisson: a one-cycle Sine pulsaret (5 Hz pulses, L = 300 samples). Its first
    # interior zero crossing (content phase 1/2) sits at x = n/L: 0.5 at 0 st; for a
    # geometric glide of g st, phase(x) = (2^(g x/12) - 1) / (g ln2 / 12) -> x(+12) =
    # 0.4293 (rising: the first half-cycle is the long one), x(-12) = 0.6139.
    # Measured relative to the 0 st render (cancels the onset-threshold bias).
    def first_zc(g):
        y = mono(render(plugin, dict(formant=32.0, glisson=g, grain_pitch=-8.5), [0], 1.2, fs))
        xs = []
        for a, b in bursts(y, merge=4)[1:5]:
            seg = y[a:b]
            i = int(np.where(np.diff(np.signbit(seg)))[0][0])
            xs.append((i + seg[i] / (seg[i] - seg[i + 1])) / 300.0)
        return float(np.median(xs))
    x0 = first_zc(0.0)
    xp, xm = first_zc(12.0) - x0 + 0.5, first_zc(-12.0) - x0 + 0.5
    verdict(abs(xp - 0.4293) < 0.01 and abs(xm - 0.6139) < 0.01,
            f"A5 glisson: first half-cycle ends at {xp:.4f} L for +12 st (0.4293, rising), {xm:.4f} L for -12 st "
            f"(0.6139); 0 st reference {x0:.4f} L")

    # --- alias sweep, both contents. Periodic configs (FM ratio 1.5 -> period 2 / f_p).
    fs = 44100
    for content in ("Sine", "Sample"):
        worst = []
        for note in (108, 100, 90):
            for gl in (24.0, 0.0):
                sets = dict(pulsaret=content, source="piano", formant=32.0, fm_depth=100.0, fm_ratio=1.5, glisson=gl)
                y = mono(render(plugin, sets, [note], 2.5, fs))[int(1.5 * fs):]
                fp = min(440 * 2 ** ((note - 69) / 12), 0.1 * fs / 2 ** (gl / 12))
                a, fa = alias_db(y, fs, fp / 2)
                worst.append((a, note, gl, fa))
        a, note, gl, fa = max(worst)
        verdict(a <= -60.0, f"A5 alias sweep {content}: MIDI 108/100/90 x 32 x FM 100 % (ratio 1.5) x glisson 0/+24: "
                f"worst {a:.1f} dB (MIDI {note}, glisson {gl:g}, at {fa:.0f} Hz; bound -60)")
    # Pitch Spray 12 makes every pulse different (not periodic): gate the band next to Nyquist,
    # which only folded energy can fill (legit content of a pulsaret at the ceiling ends at 0.46 fs).
    def nyq_band(pl, content):
        sets = dict(pulsaret=content, source="piano", formant=32.0, fm_depth=100.0, fm_ratio=1.5, pitch_spray=12.0)
        y = mono(render(pl, sets, [108], 2.5, fs))[int(1.5 * fs):]
        X, df = spectrum(y, fs, pad=1)
        f = np.arange(len(X)) * df
        return db(X[(f > 0.47 * fs) & (f < 0.499 * fs)].max() / X.max())
    for content in ("Sine", "Sample"):
        v = nyq_band(plugin, content)
        verdict(v <= -60.0, f"A5 alias Pitch Spray 12 (+ FM 100 % ratio 1.5, x 32, C8) {content}: 0.47..0.5 fs band {v:.1f} dB re peak (bound -60)")
    if control:
        v = nyq_band(control, "Sine")
        verdict(v > -60.0, f"A5 alias Pitch Spray STIMULUS CONTROL (ceiling 0.45 fs): band {v:.1f} dB (must exceed -60)")

    # --- neutral settings: bit-identical to a build with the Mask/Skip/FM/Glisson code removed.
    # Every render loads ONE state (one per-instance RNG seed), so spray draws match.
    if absent:
        fs = 48000
        ev = [(bytes([0x90, 57, 100]), 0.0), (bytes([0x90, 64, 90]), 0.3), (bytes([0x80, 57, 0]), 1.2)]
        for content in ("Sine", "Sample"):
            sets = dict(pulsaret=content, formant=5.0, position_spray=30.0, pan_spray=50.0, pitch_spray=3.0, scan=40.0)
            st = load(plugin, sets).raw_state
            a = render(plugin, {}, None, 2.0, fs, events=ev, state=st)
            b = render(absent, {}, None, 2.0, fs, events=ev, state=st)
            rms = db(np.sqrt(np.mean(a ** 2)))
            verdict(np.array_equal(a, b) and rms > -60, f"A5 neutral {content}: Mask Off, Skip 0, FM 0, Glisson 0 vs the "
                    f"code-absent build: max diff {db(np.max(np.abs(a - b))):.1f} dBFS (bit-exact required; ref {rms:.1f} dBFS)")
            # (FM at the default integer ratio 1 leaves the onsets 1/f_p apart -> FM is moved
            # with a non-integer ratio here; see the integer-ratio check above.)
            for k, v in (("pulse_mask", "3"), ("grain_skip", 30.0), ("fm_depth", 40.0), ("glisson", 5.0)):
                more = {"fm_ratio": 1.37} if k == "fm_depth" else {}
                c = render(plugin, {k: v, **more}, None, 2.0, fs, events=ev, state=st)
                d = render(absent, {k: v, **more}, None, 2.0, fs, events=ev, state=st)
                verdict(np.max(np.abs(c - a)) > 1e-3 and np.max(np.abs(c - d)) > 1e-3,
                        f"A5 live {content}: {k}={v}{' (ratio 1.37)' if more else ''} differs from neutral and from the code-absent build "
                        f"({db(np.max(np.abs(c - a))):.1f} / {db(np.max(np.abs(c - d))):.1f} dBFS)")


def main():
    print(f"plugin {PLUGIN}\ncontrol {CONTROL}\nabsent {ABSENT}")
    if "A3" in ONLY:
        a3(PLUGIN, CONTROL)
    if "A4" in ONLY:
        a4(PLUGIN, CONTROL)
    if "A5" in ONLY:
        a5(PLUGIN, CONTROL, ABSENT)
    print("\nWAVE-A", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
