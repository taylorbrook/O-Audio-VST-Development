# O-simpleAmbisonics — Phase 2.1 verify: independent measurer on the INSTALLED VST3.
# Run: uv run --python 3.12 --with pedalboard --with numpy python tests/pedalboard/verify_2_1.py
# Measurer notes: a pedalboard set of distance_m quantises to its own grid (1.0 -> ~1.004 m, engages the air LP);
# leave distance at the APVTS default (or set raw_value) for the 0.75 anchor. Short-way gates are relative to the
# static cardioid asymmetry at the start angle. `orbit` reads back as a numpy bool.
# Stage 4: the Binaural V3 arm was retired — it asserted the 2.1 single-sample response, which the 2.3 MagLS
# filters replaced by design. The real Binaural onset / latency is verify_2_3.py V2 (onset = 100 +
# kBinauralOnsetFrontSamples[order], same 64-sample latency) and its IR identity is verify_2_3.py V1.
"""Independent measurer for O-simpleAmbisonics Phase 2.1 — installed VST3 via pedalboard.

Nothing here shares code with the project harness or ReferenceModel.
"""
import math, os, sys
import numpy as np
from pedalboard import load_plugin

PATH = os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-simpleAmbisonics-dev.vst3")
FS = 48000
fails = []
def check(name, ok, detail=""):
    print(f"  [{'PASS' if ok else 'FAIL'}] {name} — {detail}")
    if not ok: fails.append(name)

def fresh():
    return load_plugin(PATH)

p = fresh()
print("parameters:", sorted(p.parameters.keys()))

def setp(p, **kw):
    for k, v in kw.items():
        setattr(p, k, v)

def run(p, x, reset=True):
    # x: (channels, n) float32
    return p(x, FS, reset=reset)

def rms(a):
    return math.sqrt(float(np.mean(a.astype(np.float64) ** 2)) + 1e-30)

def db(a, b):
    return 20 * math.log10(max(a, 1e-30) / max(b, 1e-30))

N = FS  # 1 s
rng = np.random.default_rng(7)
noise = rng.standard_normal((2, N)).astype(np.float32) * 0.1
tone = (0.5 * np.sin(2 * np.pi * 200 * np.arange(N) / FS)).astype(np.float32)
tone2 = np.stack([tone, tone])

# ---------- V1: default (Binaural) is NOT silent ----------
p = fresh()
y = run(p, noise)
check("V1 default (Binaural, az 0, d 1) not silent, both channels", rms(y[0]) > 0.01 and rms(y[1]) > 0.01,
      f"rmsL {rms(y[0]):.4f} rmsR {rms(y[1]):.4f}")
check("V1 default output finite", bool(np.all(np.isfinite(y))), "")

# ---------- V2: Stereo pan law ----------
def stereo_at(az, el=0.0, d=1.0, order="1"):
    p = fresh()
    setp(p, decoder="Stereo", azimuth=az, elevation=el, distance_m=d, order=order)
    y = run(p, tone2)
    return y[:, FS // 2:]  # steady state (smoothers settled)

y = stereo_at(0.0)
check("V2 front: L == R within 1e-4 (rms)", abs(rms(y[0]) - rms(y[1])) < 1e-4, f"L {rms(y[0]):.5f} R {rms(y[1]):.5f}")
y = stereo_at(90.0)
sep = db(rms(y[0]), rms(y[1]))
check("V2 +90 deg: L louder than R by >= 20 dB (AmbiX left-positive)", sep >= 20.0, f"{sep:.2f} dB")
y = stereo_at(-90.0)
sep = db(rms(y[1]), rms(y[0]))
check("V2 -90 deg: R louder than L by >= 20 dB", sep >= 20.0, f"{sep:.2f} dB")
y = stereo_at(180.0)
check("V2 rear: L == R within 1e-4 (rms)", abs(rms(y[0]) - rms(y[1])) < 1e-4, f"L {rms(y[0]):.5f} R {rms(y[1]):.5f}")
# in-phase: correlation of L and R with the source positive at front
y = stereo_at(0.0)
src = tone2[0, FS // 2:]
c = float(np.dot(y[0], src) / (np.linalg.norm(y[0]) * np.linalg.norm(src) + 1e-30))
check("V2 front: L in phase with the (delayed) source — corr computed after host latency comp > 0.99", c > 0.99, f"corr {c:.5f}")

# ---------- V3: impulse latency + anchor level ----------
imp = np.zeros((2, 4096), np.float32); imp[:, 100] = 1.0
for dec in ("Stereo",):  # Binaural onset is gated by verify_2_3.py V2 (see header)
    p = fresh(); setp(p, decoder=dec, azimuth=0.0, elevation=0.0, order="1")  # distance left at the APVTS default (1 m); a pedalboard set quantises to ~1.004 m
    print(f"    (fresh distance_m reads {p.distance_m}, raw {p.parameters['distance_m'].raw_value:.6f})")
    y = run(p, imp)
    k = int(np.argmax(np.abs(y[0])))
    check(f"V3 {dec}: reported latency compensated by pedalboard -> impulse back at 100", k == 100, f"index {k}")
    check(f"V3 {dec}: impulse amplitude == 0.75 (front / 1 m / order 1 anchor) both ch",
          abs(float(y[0, k]) - 0.75) < 1e-6 and abs(float(y[1, k]) - 0.75) < 1e-6,
          f"L {float(y[0,k]):.7f} R {float(y[1,k]):.7f}")
    rest = np.abs(y[0]); rest[k] = 0
    check(f"V3 {dec}: single-sample impulse response (no filter smear at d = 1 m)", float(rest.max()) < 1e-6, f"max other {float(rest.max()):.2e}")

# ---------- V4: distance law ----------
def level_at(d, f):
    p = fresh(); setp(p, decoder="Stereo", azimuth=0.0, distance_m=d, order="1")
    x = (0.25 * np.sin(2 * np.pi * f * np.arange(N) / FS)).astype(np.float32)
    y = run(p, np.stack([x, x]))[0, FS // 2:]
    return rms(y)

ds = [0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0]
lv = [level_at(d, 200) for d in ds]
check("V4 200 Hz level strictly decreasing over 0.2..20 m", all(a > b for a, b in zip(lv, lv[1:])),
      " ".join(f"{db(v, lv[2]):+.2f}" for v in lv) + " dB re 1 m")
check("V4 anchor 0.2 m = +8.87 dB re 1 m (0.1)", abs(db(lv[0], lv[2]) - 8.874) < 0.1, f"{db(lv[0], lv[2]):+.3f} dB")
check("V4 anchor 20 m = -24.19 dB re 1 m (0.1)", abs(db(lv[-1], lv[2]) + 24.198) < 0.1, f"{db(lv[-1], lv[2]):+.3f} dB")
hf = [db(level_at(d, 12000), level_at(d, 200)) for d in ds]
check("V4 HF/LF (12 kHz vs 200 Hz) flat on [0.2, 1] (|d| < 0.02 dB)", max(abs(h - hf[2]) for h in hf[:3]) < 0.02,
      " ".join(f"{h:+.3f}" for h in hf[:3]))
check("V4 HF/LF strictly decreasing on (1, 20]", all(a > b for a, b in zip(hf[2:], hf[3:])), " ".join(f"{h:+.2f}" for h in hf))
check("V4 HF/LF at 20 m = -10.79 dB (0.1)", abs(hf[-1] + 10.788) < 0.1, f"{hf[-1]:+.3f} dB")

# ---------- V5: order changes nothing in Stereo mode; orders not silent ----------
outs = {}
for o in ("1", "2", "3"):
    p = fresh(); setp(p, decoder="Stereo", azimuth=37.0, elevation=-12.0, distance_m=3.3, order=o)
    outs[o] = run(p, noise)
check("V5 Stereo output bit-equal for order 1 / 2 / 3 (decoder reads W/Y/X only)",
      np.array_equal(outs["1"], outs["2"]) and np.array_equal(outs["2"], outs["3"]), "documented: orders change nothing audible in Stereo mode")

# ---------- V6: orbit moves the source ----------
p = fresh(); setp(p, decoder="Stereo", azimuth=0.0, orbit=True, orbit_rate_hz=1.0, order="1")
y = run(p, np.stack([tone, tone]))
q = N // 4
e = [rms(y[0, i*q:(i+1)*q]) for i in range(4)]
check("V6 orbit On 1 Hz: L energy differs between quarter-cycles by >= 6 dB", db(max(e), min(e)) >= 6.0,
      " ".join(f"{v:.4f}" for v in e))
p = fresh(); setp(p, decoder="Stereo", azimuth=0.0, orbit=False, order="1")
y = run(p, np.stack([tone, tone]))
e = [rms(y[0, i*q:(i+1)*q]) for i in range(1, 4)]
check("V6 orbit Off: L energy static across quarters (< 0.01 dB)", db(max(e), min(e)) < 0.01, " ".join(f"{v:.5f}" for v in e))

# ---------- V7: shortest path across +/-180 ----------
# Start at +170, then jump the parameter to -170 mid-stream; the smoothed source must pass behind (L≈R stays ~equal),
# never swing through the front where it would also be L≈R... so instead track the |L-R| excursion: the long way (through 0)
# crosses +/-90 where |L-R| is maximal; the short way (through 180) keeps |L-R| small throughout.
p = fresh(); setp(p, decoder="Stereo", azimuth=170.0, order="1")
seg = np.stack([tone[:N // 2], tone[:N // 2]])
run(p, seg, reset=True)
setp(p, azimuth=-170.0)
y = run(p, seg, reset=False)
blk = 64
diff = max(abs(rms(y[0, i:i+blk]) - rms(y[1, i:i+blk])) / (rms(y[0, i:i+blk]) + rms(y[1, i:i+blk]) + 1e-30) for i in range(0, N // 2 - blk, blk))
# at az +/-90 the normalised |L-R|/(L+R) is ~1; through 180 it stays below ~0.1 (within 10 deg of 180)
def nd(az):
    a = math.radians(az); L = 0.5*(0.5 + 0.5*(0.5*math.cos(a) + math.sqrt(3)/2*math.sin(a))); R = 0.5*(0.5 + 0.5*(0.5*math.cos(a) - math.sqrt(3)/2*math.sin(a)))
    return abs(L - R) / (L + R)
static170, longway90 = nd(170.0), nd(90.0)
check("V7 +170 -> -170 goes the short way: max normalised |L-R| over the glide <= static(170) * 1.05 (long way would reach the 90-deg value)",
      diff <= static170 * 1.05, f"max {diff:.3f}; static at 170 deg {static170:.3f}; long way through 90 deg would hit {longway90:.3f}")

# ---------- V8: extremes finite ----------
bad = 0
for az in (-180.0, 180.0):
    for el in (-90.0, 90.0):
        for d in (0.2, 20.0):
            for o in ("1", "3"):
                for dec in ("Binaural", "Stereo"):
                    p = fresh(); setp(p, decoder=dec, azimuth=az, elevation=el, distance_m=d, order=o, yaw=180.0, pitch=90.0, roll=-180.0, orbit=True, orbit_rate_hz=2.0, output_level_db=0.0)
                    y = run(p, noise[:, :4096])
                    if not np.all(np.isfinite(y)) or rms(y) == 0.0: bad += 1
check("V8 32 parameter corners x 4096 samples finite and not silenced", bad == 0, f"{bad} bad")

# ---------- V9: state round-trip of the engine-facing values ----------
p = fresh(); setp(p, decoder="Stereo", azimuth=90.0, distance_m=5.0, order="3", orbit=True, orbit_rate_hz=0.5, input_mode="Left only", output_level_db=-6.0)
st = p.raw_state
q = fresh(); q.raw_state = st
vals = {k: getattr(q, k) for k in ("decoder", "azimuth", "distance_m", "order", "orbit", "orbit_rate_hz", "input_mode", "output_level_db")}
ok = (vals["decoder"] == "Stereo" and abs(vals["azimuth"] - 90) < 1e-3 and abs(vals["distance_m"] - 5.0) < 0.01
      and str(vals["order"]) in ("3", "3.0") and bool(vals["orbit"]) == True and abs(vals["orbit_rate_hz"] - 0.5) < 0.01
      and vals["input_mode"] == "Left only" and abs(vals["output_level_db"] + 6) < 1e-3)
check("V9 state round-trip restores decoded values", ok, str(vals))

print()
print("ALL PASS" if not fails else f"{len(fails)} FAILED: {fails}")
sys.exit(1 if fails else 0)
