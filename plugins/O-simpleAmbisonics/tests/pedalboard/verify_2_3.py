# O-simpleAmbisonics — Phase 2.3 verify: independent measurer on the INSTALLED VST3 (the binaural leg).
# Run: uv run --no-project --python 3.12 --with pedalboard --with numpy --with scipy python tests/pedalboard/verify_2_3.py
#
# Nothing here shares code with the project harness, ReferenceModel.h, BinauralConvolver.h or ShEncoder.h. The
# filter banks are the ARTIFACT under test: the float literals are parsed straight out of the generated
# Source/dsp/BinauralFilterData.h. The SH encoder is an own SN3D / ACN table built from scipy's associated
# Legendre functions (Condon–Shortley phase removed), checked against closed-form anchors before use.
#
# Expected Binaural impulse response at 48 k, 1 m, no rotation (g = 1, air-LP mix 0 → the source is bit-exact):
#     out[ear][100 + t] = Σ_ch Y_ch(az, el) · h[order][ch][ear][t]        (pedalboard compensates the reported 64)
# so a match within 1e-5 pins the convolution AND the latency alignment at once. At fs ≠ 48 k the bank is
# scipy.signal.resample_poly(h, up, down) · down / up (DC-gain preserving — SUMMARY-2.3 C3), compared below
# min(16 kHz, 0.68·Nyquist) where the two anti-alias designs are transparent.
#
# Measurer notes carried from verify_2_2.py: every numeric parameter is set by raw_value (a pedalboard setattr
# quantises to its own grid); leave distance_m at the APVTS default for the exact anchors.
#
# NEW measurer lesson (this verify): pedalboard flushes the reported latency with SILENCE at every call, so a
# multi-call render (reset=False) carries a 64-sample silent hole in the input stream at each call boundary.
# On the pure-delay Stereo path the hole's output lands in the samples pedalboard discards (invisible — the 2.2
# sweep read 1.000); on the FIR path it is smeared over 64 + 256 samples (a 0.40 dip on a constant input, and a
# 48° phase shift of a 100 Hz tone against a one-call render). Switch gates therefore park the call boundary in
# SILENCE (the hole becomes a pure 64-sample shift, asserted) and compare the switched two-call render against
# two-call STATIC renders with the same call structure: the residual sw − s1 must be the smooth fade m(t)·(s2 − s1),
# whose first difference is bounded by that of (s2 − s1) itself (ceiling 2.0 — see V5 for the two-ramp order case).
# ITD uses the generator's C10 cutoff 0.8·f_a per order ({499, 998, 1497} Hz): above f_a the MagLS phase is
# synthetic and a 1.5 kHz low-pass reads +16 at order 1 where the LS region reads +36.
"""Independent measurer for O-simpleAmbisonics Phase 2.3 — installed VST3 via pedalboard."""
import math, os, re, sys
import numpy as np
from scipy.signal import resample_poly, butter, sosfiltfilt
from scipy.special import lpmv, factorial
from pedalboard import load_plugin

HERE = os.path.dirname(os.path.abspath(__file__))
HEADER = os.path.join(HERE, "..", "..", "Source", "dsp", "BinauralFilterData.h")
PATH = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3")
FS = 48000
fails = []
def check(name, ok, detail=""):
    print(f"  [{'PASS' if ok else 'FAIL'}] {name} — {detail}")
    if not ok: fails.append(name)

def fresh():
    return load_plugin(PATH)

RANGES = {"azimuth": (-180.0, 180.0), "elevation": (-90.0, 90.0), "yaw": (-180.0, 180.0),
          "pitch": (-180.0, 180.0), "roll": (-180.0, 180.0), "output_level_db": (-60.0, 0.0)}

def setp(p, **kw):
    for k, v in kw.items():
        if k in RANGES:
            lo, hi = RANGES[k]
            p.parameters[k].raw_value = (float(v) - lo) / (hi - lo)
        elif k == "order":
            p.parameters[k].raw_value = (int(v) - 1) / 2.0
        elif k == "decoder":
            p.parameters[k].raw_value = 0.0 if v == "Binaural" else 1.0
        else:
            setattr(p, k, v)

def run(p, x, fs=FS, reset=True):
    return p(x, fs, reset=reset)

def rms(a):
    return math.sqrt(float(np.mean(a.astype(np.float64) ** 2)) + 1e-30)

def db(a, b):
    return 20 * math.log10(max(a, 1e-30) / max(b, 1e-30))

# ---------- the filter banks, parsed from the generated header (the artifact) ----------
def parse_header():
    text = open(HEADER, encoding="utf-8").read()
    const = {}
    m = re.search(r"kBinauralOnsetFrontSamples\[4\]\s*=\s*\{([^}]*)\}", text)
    const["onsets"] = [int(s) for s in m.group(1).split(",")]
    const["levelMatchDb"] = float(re.search(r"kBinauralLevelMatchDb\s*=\s*([-\d.eE+]+)f", text).group(1))
    const["fs"] = int(re.search(r"kBinauralFilterSampleRate\s*=\s*(\d+)", text).group(1))
    const["taps"] = int(re.search(r"kBinauralFilterTaps\s*=\s*(\d+)", text).group(1))
    banks = {}
    num = re.compile(r"[-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?")
    for n in (1, 2, 3):
        start = text.index(f"kMagLsFilters{n}[")
        eq = text.index("=", start)
        nxt = text.find("inline constexpr", eq)
        if nxt < 0: nxt = text.index("} // namespace", eq)
        block = re.sub(r"//[^\n]*", "", text[eq + 1:nxt])
        vals = np.array([float(s) for s in num.findall(block)], dtype=np.float32)
        nch = (n + 1) ** 2
        assert vals.size == nch * 2 * const["taps"], (n, vals.size)
        banks[n] = vals.reshape(nch, 2, const["taps"])
    return const, banks

CONST, BANK = parse_header()
print(f"header: fs {CONST['fs']} taps {CONST['taps']} onsets {CONST['onsets']} levelMatch {CONST['levelMatchDb']:+.3f} dB; "
      f"banks {[BANK[n].shape for n in (1, 2, 3)]}")
check("P0 header parsed: 48 k, 256 taps, three banks, tap 0 exactly 0, all finite",
      CONST["fs"] == 48000 and CONST["taps"] == 256 and all(np.all(np.isfinite(BANK[n])) for n in (1, 2, 3))
      and all(float(np.max(np.abs(BANK[n][:, :, 0]))) == 0.0 for n in (1, 2, 3)), "")

# ---------- own SN3D / ACN encoder (scipy Legendre, Condon–Shortley removed) ----------
def sh16(az_deg, el_deg):
    a, e = math.radians(az_deg), math.radians(el_deg)
    out = np.zeros(16)
    for n in range(4):
        for m in range(-n, n + 1):
            am = abs(m)
            norm = math.sqrt((2.0 if m != 0 else 1.0) * factorial(n - am) / factorial(n + am))
            P = float(lpmv(am, n, math.sin(e))) * ((-1) ** am)      # remove the Condon–Shortley phase
            trig = math.cos(am * a) if m >= 0 else math.sin(am * a)
            out[n * n + n + m] = norm * P * trig
    return out

y = sh16(90.0, 0.0); z = sh16(0.0, 90.0); x = sh16(0.0, 0.0); q = sh16(45.0, 0.0)
check("P0 own encoder anchors: W = 1, Y(+90) = 1, Z(up) = 1, X(front) = 1, ACN 4 at az 45 = √3/2, ACN 15 at front = √(5/8)",
      abs(y[0] - 1) < 1e-12 and abs(y[1] - 1) < 1e-12 and abs(z[2] - 1) < 1e-12 and abs(x[3] - 1) < 1e-12
      and abs(q[4] - math.sqrt(3) / 2) < 1e-12 and abs(x[15] - math.sqrt(5 / 8)) < 1e-12 and abs(x[6] + 0.5) < 1e-12,
      f"Y {y[1]:.6f} Z {z[2]:.6f} X {x[3]:.6f} acn4 {q[4]:.6f} acn15 {x[15]:.6f} acn6(front) {x[6]:.6f}")

def hand_ir(az, el, order, bank=None):
    """Σ_ch Y_ch · h[ch][ear][t] in double; (2, taps)."""
    h = BANK[order] if bank is None else bank
    c = sh16(az, el)[: (order + 1) ** 2]
    return np.einsum("c,cet->et", c, h.astype(np.float64))

def plugin_ir(order, dec="Binaural", fs=FS, n=4096, **kw):
    p = fresh(); setp(p, decoder=dec, order=order, **kw)
    imp = np.zeros((2, n), np.float32); imp[:, 100] = 1.0
    return run(p, imp, fs)

N = FS
rng = np.random.default_rng(23)
noise = (rng.standard_normal((2, N)) * 0.1).astype(np.float32)

p = fresh()
print("parameters:", sorted(p.parameters.keys()))
check("P0 decoder / order are host parameters; default decoder is Binaural, order 1",
      str(p.decoder) == "Binaural" and float(p.order) == 1.0, f"decoder {p.decoder} order {p.order}")

# ---------- V1: Binaural IR == header-float convolution at 48 k (convolution + latency alignment in one gate) ----------
cases = [(0.0, 0.0, 1), (90.0, 0.0, 1), (37.0, -12.0, 2), (0.0, 0.0, 3), (90.0, 0.0, 3), (-123.0, 45.0, 3), (180.0, -90.0, 2)]
for az, el, order in cases:
    yv = plugin_ir(order, azimuth=az, elevation=el).astype(np.float64)
    ex = hand_ir(az, el, order)
    seg = yv[:, 100:100 + 256]
    err = float(np.max(np.abs(seg - ex)))
    peak = float(np.max(np.abs(ex)))
    head = float(np.max(np.abs(yv[:, :100])))
    tail = float(np.max(np.abs(yv[:, 100 + 256:])))
    check(f"V1 Binaural IR (az {az:+.0f}, el {el:+.0f}, order {order}) == Σ Y·h from the header at index 100 within 1e-5; "
          f"|out[:100]| < 1e-6; tail < 1e-6",
          err < 1e-5 and head < 1e-6 and tail < 1e-6, f"max err {err:.2e} (peak {peak:.4f}); head {head:.1e} tail {tail:.1e}")
yv = plugin_ir(1, azimuth=0.0, elevation=0.0)
check("V1 out[0..63] EXACTLY 0 in Binaural (the structural 64 + the impulse's own 36 leading zeros)",
      float(np.max(np.abs(yv[:, :64]))) == 0.0, f"first nonzero {int(np.argmax(np.abs(yv).sum(0) > 0))}")

# ---------- V2: the A14 placeholder is gone, and both decoders share the 64 latency ----------
bin3 = plugin_ir(3, azimuth=0.0, elevation=0.0).astype(np.float64)
st3 = plugin_ir(3, "Stereo", azimuth=0.0, elevation=0.0).astype(np.float64)
margin = float(np.max(np.abs(bin3 - st3)))
check("V2 Binaural ≠ Stereo render by a large margin (placeholder gone): max |L_bin − L_st| > 0.05", margin > 0.05, f"{margin:.4f}")
k = int(np.argmax(np.abs(st3[0])))
check("V2 Stereo impulse at the compensated index 100 with L = R = 0.75 exactly (latency 64 reported in Stereo mode)",
      k == 100 and st3[0, 100] == 0.75 and st3[1, 100] == 0.75 and float(np.max(np.abs(np.delete(st3, 100, axis=1)))) == 0.0,
      f"index {k} L {st3[0, 100]:.7f} R {st3[1, 100]:.7f}")
for order in (1, 3):
    yv = plugin_ir(order, azimuth=0.0, elevation=0.0).astype(np.float64)
    pk = float(np.max(np.abs(yv[0])))
    on = int(np.argmax(np.abs(yv[0]) >= 0.1 * pk))
    exp = 100 + CONST["onsets"][order]
    check(f"V2 Binaural order {order} front: 10 % onset at 100 + kBinauralOnsetFrontSamples[{order}] = {exp} exactly "
          f"(same 64 latency in both modes, compensated by the host)", on == exp, f"onset {on}")

# ---------- V3: ILD / ITD at ±90° (FUNC-01 second box with the real leg, DSP-03 third box) ----------
F_A = {1: 623.9, 2: 1247.8, 3: 1871.7}     # N·343/(2π·0.0875) — the MagLS switch frequency per order
def itd_samples(L, R, fs, cutoff):
    """Positive = LEFT leads. Low-pass below the LS region (0.8·f_a, C10), cross-correlation peak over ±100 samples."""
    sos = butter(4, cutoff / (fs / 2), output="sos")
    l = sosfiltfilt(sos, L); r = sosfiltfilt(sos, R)
    lags = range(-100, 101)
    c = [float(np.dot(l[:len(l) - abs(k)] if k >= 0 else l[-k:], r[k:] if k >= 0 else r[:len(r) + k])) for k in lags]
    return list(lags)[int(np.argmax(c))]
# sign anchor: R is L delayed by 17 → L leads by +17
a = rng.standard_normal(4096); b = np.concatenate([np.zeros(17), a[:-17]])
check("V3 ITD helper sign anchor: R = L delayed 17 reads +17 (L leads)", itd_samples(a, b, FS, 499.0) == 17, f"{itd_samples(a, b, FS, 499.0)}")
for order, want, witd in ((1, 12.28, 36), (2, 13.79, 38), (3, 13.87, 37)):
    for az, sgn in ((90.0, +1), (-90.0, -1)):
        yv = plugin_ir(order, azimuth=az, elevation=0.0).astype(np.float64)
        ild = db(rms(yv[0]), rms(yv[1]))
        itd = itd_samples(yv[0], yv[1], FS, 0.8 * F_A[order])
        check(f"V3 az {az:+.0f} order {order}: ILD {'L' if sgn > 0 else 'R'} louder by ≥ 6 dB (AmbiX: +90 = LEFT) and ITD 25–45 samples with the same sign (LP < {0.8 * F_A[order]:.0f} Hz)",
              sgn * ild >= 6.0 and 25 <= sgn * itd <= 45, f"ILD {ild:+.2f} dB (generator {sgn * want:+.2f}), ITD {itd:+d} (generator {sgn * witd:+d})")
# the audible version: order 1, +90°, white noise, Binaural
p = fresh(); setp(p, decoder="Binaural", order=1, azimuth=90.0)
yv = run(p, noise)[:, FS // 4:]
check("V3 order 1 / +90° / Binaural, white noise: LEFT louder than RIGHT by ≥ 6 dB", db(rms(yv[0]), rms(yv[1])) >= 6.0, f"{db(rms(yv[0]), rms(yv[1])):+.2f} dB")

# ---------- V4: FUNC-05 level delta — A-weighted white front source, order 3, mean of ears ----------
def a_weighted_level(y, fs):
    Y = np.fft.rfft(y.astype(np.float64)); f = np.fft.rfftfreq(y.size, 1 / fs); f2 = f ** 2
    ra = 12194.0 ** 2 * f2 ** 2 / ((f2 + 20.6 ** 2) * np.sqrt((f2 + 107.7 ** 2) * (f2 + 737.9 ** 2)) * (f2 + 12194.0 ** 2))
    w = ra * 10 ** (2.0 / 20)
    return math.sqrt(float(np.sum((np.abs(Y) * w) ** 2)))
for order, want in ((3, 0.0), (1, -0.34), (2, -0.03)):
    pb = fresh(); setp(pb, decoder="Binaural", order=order, azimuth=0.0)
    ps = fresh(); setp(ps, decoder="Stereo", order=order, azimuth=0.0)
    yb = run(pb, noise)[:, 4096:]; ys = run(ps, noise)[:, 4096:]
    lb = 0.5 * (a_weighted_level(yb[0], FS) + a_weighted_level(yb[1], FS))
    ls = 0.5 * (a_weighted_level(ys[0], FS) + a_weighted_level(ys[1], FS))
    d = db(lb, ls)
    if order == 3:
        check("V4 decoder-switch level delta at the front, order 3 (A-weighted white, mean of ears): |Binaural − Stereo| < 1 dB", abs(d) < 1.0,
              f"{d:+.3f} dB (generator {want:+.2f})")
    else:
        print(f"    V4 order {order}: Binaural − Stereo {d:+.3f} dB (generator {want:+.2f}; printed)")

# ---------- V5: order and decoder switches mid-tone are click-free on the installed binary (DSP-07 / FUNC-05) ----------
# The call boundary (where pedalboard's 64-sample latency-flush hole lands — header note) sits in SILENCE: call 1 ends
# with 1024 zero samples (> the 64 + 256 FIR memory), so the inserted zeros change nothing but a 64-sample time shift.
# The tone restarts from phase 0 at the switch. 112.5 Hz: the 20 ms fade (960 samples) is 2.25 periods, so the fade
# ARRIVES on a tone peak (a snap at the arrival would be the loudest possible); the snap sweep below covers the span.
TONE_HZ = 112.5
HALF = N // 2
GAP = 1024
x2 = (0.5 * np.sin(2 * np.pi * TONE_HZ * np.arange(HALF) / FS)).astype(np.float32)
x1 = x2.copy(); x1[HALF - GAP:] = 0.0
def two_call(set_before, set_after=None):
    p = fresh(); setp(p, azimuth=90.0, **set_before)
    a = run(p, np.stack([x1, x1]), reset=True)
    if set_after: setp(p, **set_after)
    b = run(p, np.stack([x2, x2]), reset=False)
    return np.concatenate([a, b], axis=1).astype(np.float64)
def d1max(y, lo, hi):
    return float(np.max(np.abs(np.diff(y[:, lo:hi], axis=1))))
# hole invisibility: the two-call static equals the one-call render of the same stream up to a 64-sample shift
p = fresh(); setp(p, azimuth=90.0, decoder="Binaural", order=3)
one = run(p, np.stack([np.concatenate([x1, x2])] * 2)).astype(np.float64)
two = two_call(dict(decoder="Binaural", order=3))
shifts = {k: float(np.max(np.abs(two[:, HALF + 64:N - 64] - one[:, HALF + 64 - k:N - 64 - k]))) for k in (-64, 0, 64)}
kbest = min(shifts, key=shifts.get)
check("V5 premise: with the call boundary in silence the two-call Binaural render == the one-call render after the boundary up to a 64-sample shift (< 1e-6)",
      shifts[kbest] < 1e-6, f"shift {kbest:+d}: {shifts[kbest]:.1e} (others {', '.join(f'{k:+d}: {v:.2f}' for k, v in shifts.items() if k != kbest)})")
# The residual sw − s1 is the fade m(t)·(s2 − s1) (the pre-switch halves are bit-equal, the plugin is linear in x): its
# first difference is bounded by Δm·|s2 − s1| + |Δ(s2 − s1)| ≤ (1 + 1/(960·ωT))·max|Δ(s2 − s1)| ≈ 1.07× for a 112.5 Hz
# tone and a 20 ms ramp. A SNAPPED switch puts |s2 − s1| into one sample: ratio ≈ 1/(ωT) ≈ 68.
# Ceiling 2.0 (the project's click ceiling), not the 1.07 of a single linear fade: an ORDER switch is two simultaneous
# 20 ms ramps — the bank fade m(t) on the convolver output and the band gain g(t) on the new channels — so the residual
# carries an m·g product whose slope reaches 2·Δm at one end of the fade (the arrival for 1 → 3, the start for 3 → 1).
# With the new bands' contribution larger than the net order-1 / order-3 difference of a 112 Hz tone, that reads ≈ 1.5
# on the steady window; a snap reads ≈ 1/(ωT) ≈ 68. The harness holds the exact same-motion ratio (1.0004 / 0.9995).
# Two windows. FULL starts at the boundary and covers the whole fade; its reference max|Δ(s2 − s1)| is dominated by the
# tone ONSET's transient through the two banks (320 samples of FIR memory), which drowns the small steady-state
# order-1 / order-3 difference of a 112 Hz tone — so for the order switch the snapped control cannot reach 5 there.
# STEADY starts after the FIR memory (HALF + 64 + 320) and covers the fade's last two thirds, its arrival on the tone
# peak, and the settled tail. The gate is STEADY for every case; FULL is gated too wherever its own control reads ≥ 5
# (the decoder switch). The harness gates the fade's head with its same-motion reference (SUMMARY-2.3 DSP-07).
FULL = (HALF - 64, HALF + 4096)
STEADY = (HALF + 64 + 320, HALF + 4096)
def gate(sw, s1, s2, win, snap_lo, snap_hi):
    ref = d1max(s2 - s1, *win)
    ratio = d1max(sw - s1, *win) / ref
    neg = 0.0                                   # a snap anywhere in [snap_lo, snap_hi] (step 60) — the worst must FAIL
    for ks in range(snap_lo, snap_hi + 1, 60):
        snap = np.concatenate([s1[:, :ks], s2[:, ks:]], axis=1)
        neg = max(neg, d1max(snap - s1, *win) / ref)
    return ratio, neg
for label, before, after in (("order 1 → 3 (Binaural)", dict(decoder="Binaural", order=1), dict(order=3)),
                             ("order 3 → 1 (Binaural)", dict(decoder="Binaural", order=3), dict(order=1)),
                             ("Binaural → Stereo (order 3)", dict(decoder="Binaural", order=3), dict(decoder="Stereo")),
                             ("Stereo → Binaural (order 3)", dict(decoder="Stereo", order=3), dict(decoder="Binaural"))):
    sw = two_call(before, after)
    s1 = two_call(before); s2 = two_call({**before, **after})
    r_full, n_full = gate(sw, s1, s2, FULL, HALF + 64, HALF + 64 + 960)
    r_st, n_st = gate(sw, s1, s2, STEADY, HALF + 64 + 320, HALF + 64 + 960)
    pre = float(np.max(np.abs(sw[:, :HALF] - s1[:, :HALF])))
    settle = float(np.max(np.abs(sw[:, HALF + 4096:] - s2[:, HALF + 4096:])))
    full_ok = n_full < 5.0 or r_full <= 2.0
    check(f"V5 {label} mid-tone ({TONE_HZ} Hz, az +90): residual first-difference ratio ≤ 2.0 vs the static pair on the STEADY window "
          f"(and on FULL where its control is valid); pre-switch bit-equal; settles onto the post-mode render within 1e-3; snapped control ≥ 5",
          r_st <= 2.0 and n_st >= 5.0 and full_ok and bool(np.all(np.isfinite(sw))) and pre == 0.0 and settle < 1e-3,
          f"steady ratio {r_st:.4f} (snapped {n_st:.1f} FAIL-as-expected); full ratio {r_full:.4f} (snapped {n_full:.1f}{'' if n_full >= 5 else ', control invalid — not gated'}); "
          f"pre {pre:.1e}; settle {settle:.2e}; rmsL post-fade: pre-mode static {rms(s1[0, HALF + 4096:]):.4f} → switched "
          f"{rms(sw[0, HALF + 4096:]):.4f} ({db(rms(sw[0, HALF + 4096:]), rms(s1[0, HALF + 4096:])):+.2f} dB)")

# ---------- V6: fs ≠ 48 k — the resampled bank vs scipy's, equal latency, cross-rate level ----------
def band_rel_err(a, b, fs, length=1024):
    """Brick-wall both at min(16 kHz, 0.68·Nyquist), then max|a − b| / max|b|."""
    cut = min(16000.0, 0.68 * fs / 2)
    def lp(v):
        V = np.fft.rfft(v, n=length); f = np.fft.rfftfreq(length, 1 / fs); V[f > cut] = 0
        return np.fft.irfft(V, n=length)
    la, lb = lp(a), lp(b)
    return float(np.max(np.abs(la - lb))) / float(np.max(np.abs(lb))), cut
for fs, up, down, tol in ((44100, 147, 160, 1.5e-2), (96000, 2, 1, 6e-3)):
    bank = {n: resample_poly(BANK[n].astype(np.float64), up, down, axis=2) * (down / up) for n in (1, 2, 3)}
    L = bank[3].shape[2]
    st = plugin_ir(1, "Stereo", fs=fs, azimuth=0.0, elevation=0.0).astype(np.float64)
    check(f"V6 {fs} Hz Stereo impulse at the compensated index 100 with L = R = 0.75 exactly (latency 64 at this rate)",
          int(np.argmax(np.abs(st[0]))) == 100 and st[0, 100] == 0.75 and st[1, 100] == 0.75, f"index {int(np.argmax(np.abs(st[0])))} L {st[0, 100]:.7f}")
    worst = 0.0
    for az, el, order in ((0.0, 0.0, 1), (90.0, 0.0, 3), (-123.0, 45.0, 3)):
        yv = plugin_ir(order, fs=fs, azimuth=az, elevation=el).astype(np.float64)
        ex = hand_ir(az, el, order, bank[order])
        for ear in (0, 1):
            e, cut = band_rel_err(yv[ear, 100:100 + L], ex[ear], fs)
            worst = max(worst, e)
        head = float(np.max(np.abs(yv[:, :100])))
        if head >= 1e-6: worst = 1.0
    check(f"V6 {fs} Hz Binaural IRs (3 directions × 2 ears) == scipy resample_poly · down/up convolution below {cut:.0f} Hz within {tol:.1e} of peak; |out[:100]| < 1e-6",
          worst < tol, f"worst rel err {worst:.3e}")
    yv = plugin_ir(1, fs=fs, azimuth=0.0, elevation=0.0).astype(np.float64)
    on = int(np.argmax(np.abs(yv[0]) >= 0.1 * np.max(np.abs(yv[0]))))
    ex = hand_ir(0.0, 0.0, 1, bank[1]); exon = 100 + int(np.argmax(np.abs(ex[0]) >= 0.1 * np.max(np.abs(ex[0]))))
    check(f"V6 {fs} Hz order 1 front: 10 % onset within ±3 of the scipy bank's ({exon})", abs(on - exon) <= 3, f"onset {on}")
# cross-rate absolute level: 1 kHz front tone, order 3 Binaural, L amplitude at 44.1 / 96 k vs 48 k
amp = {}
for fs in (48000, 44100, 96000):
    t = np.arange(fs)
    tn = np.stack([(0.25 * np.sin(2 * np.pi * 1000 * t / fs)).astype(np.float32)] * 2)
    p = fresh(); setp(p, decoder="Binaural", order=3, azimuth=0.0)
    amp[fs] = float(np.max(np.abs(run(p, tn, fs)[0, fs // 2:])))
check("V6 cross-rate level: 1 kHz front tone (order 3, Binaural) L amplitude at 44.1 k and 96 k within ±0.05 dB of 48 k (DC-preserving resample)",
      abs(db(amp[44100], amp[48000])) < 0.05 and abs(db(amp[96000], amp[48000])) < 0.05,
      f"48 k {amp[48000]:.5f}, 44.1 k {db(amp[44100], amp[48000]):+.3f} dB, 96 k {db(amp[96000], amp[48000]):+.3f} dB")

# ---------- V7: Stereo path untouched — the 2.1 / 2.2 anchors on the installed binary ----------
st = plugin_ir(1, "Stereo", azimuth=90.0, elevation=0.0).astype(np.float64)
check("V7 Stereo +90° impulse anchor unchanged: L 0.9330127 / R 0.0669873 at index 100",
      abs(st[0, 100] - 0.9330127) < 1e-6 and abs(st[1, 100] - 0.0669873) < 1e-6, f"L {st[0, 100]:.7f} R {st[1, 100]:.7f}")
a = fresh(); setp(a, decoder="Stereo", order=1, azimuth=0.0, yaw=37.0)
b = fresh(); setp(b, decoder="Stereo", order=1, azimuth=37.0, yaw=0.0)
check("V7 Stereo: yaw 37 (az 0) == azimuth 37 (yaw 0) bit-equal over 1 s of noise (2.2 anchor)",
      np.array_equal(run(a, noise), run(b, noise)), "")

# ---------- V8: rotation reaches the binaural leg — yaw +90 on a front source == a left source, Binaural order 3 ----------
a = fresh(); setp(a, decoder="Binaural", order=3, azimuth=0.0, yaw=90.0)
b = fresh(); setp(b, decoder="Binaural", order=3, azimuth=90.0, yaw=0.0)
ya, yb_ = run(a, noise).astype(np.float64), run(b, noise).astype(np.float64)
d = float(np.max(np.abs(ya - yb_)))
check("V8 Binaural order 3: yaw +90 on a front source == azimuth +90 within 1e-4 over 1 s of noise (the convolver consumes the ROTATED bus)",
      d < 1e-4, f"max |diff| {d:.2e}; ILD {db(rms(ya[0, FS // 4:]), rms(ya[1, FS // 4:])):+.2f} dB")

# ---------- V9: extremes with the HRTF leg, 48 k and 96 k ----------
bad = 0; n = 0; pk = 0.0
for fs in (48000, 96000):
    for order in (1, 3):
        for az, el in ((-180.0, 90.0), (180.0, -90.0)):
            for dist in (0.2, 20.0):
                for ang in (-180.0, 180.0):
                    p = fresh(); setp(p, decoder="Binaural", order=order, azimuth=az, elevation=el, distance_m=dist, yaw=ang, pitch=-ang / 2, roll=ang, orbit=True, orbit_rate_hz=2.0)
                    y = run(p, noise[:, :4096], fs); n += 1
                    if not np.all(np.isfinite(y)) or rms(y) == 0.0: bad += 1
                    pk = max(pk, float(np.max(np.abs(y))))
check(f"V9 {n} Binaural corners (48 k / 96 k × orders × pole sources × 0.2 / 20 m × ±180 rotation, orbit 2 Hz) finite and not silenced", bad == 0,
      f"{bad} bad; peak |out| {pk:.3f} (0.2 m: +8.9 dB + bank headroom, printed)")

# ---------- V10: state round-trip of decoder + order + a Binaural render after restore ----------
p = fresh(); setp(p, decoder="Stereo", order=3, azimuth=123.0, yaw=-45.0)
st_ = p.raw_state
q = fresh(); q.raw_state = st_
check("V10 decoder / order / azimuth / yaw survive a state round-trip (decoded values)",
      str(q.decoder) == "Stereo" and float(q.order) == 3.0 and abs(float(q.azimuth) - 123.0) < 1e-3 and abs(float(q.yaw) + 45.0) < 1e-3,
      f"restored decoder {q.decoder} order {q.order} az {float(q.azimuth):.3f} yaw {float(q.yaw):.3f}")
yq = run(q, noise[:, :8192]).astype(np.float64)
check("V10 the restored instance renders finite and live", bool(np.all(np.isfinite(yq))) and rms(yq) > 1e-3, f"rms {rms(yq):.4f}")

print()
print("ALL PASS" if not fails else f"{len(fails)} FAILED: {fails}")
sys.exit(1 if fails else 0)
