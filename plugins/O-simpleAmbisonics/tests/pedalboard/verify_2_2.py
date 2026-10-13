# O-simpleAmbisonics — Phase 2.2 verify: independent measurer on the INSTALLED VST3 (scene rotation).
# Run: uv run --no-project --python 3.12 --with pedalboard --with numpy python tests/pedalboard/verify_2_2.py
#
# Nothing here shares code with the project harness, ReferenceModel.h or ShRotator.h. The anchors are the
# ±60° cardioid pair evaluated by hand on (W, Y, X) of the ROTATED first-order bus:
#     L = 0.5·W + 0.4330127·Y + 0.25·X        R = 0.5·W − 0.4330127·Y + 0.25·X
# with W = 1, X = cos e·cos a, Y = cos e·sin a at 1 m / order 1 (g = 1). A scene rotation must therefore read
# EXACTLY like the source re-encoded at the rotated direction — yaw +90 on a front source == azimuth +90.
#
# Measurer notes carried from verify_2_1.py: leave distance_m at the APVTS default for the exact anchors; a
# pedalboard set quantises to its own grid. Stereo mode reads W/Y/X only, so pitch / roll signs are pinned with
# the source off the equator (el +45), where "front rises" vs "front sinks" changes the X component.
"""Independent measurer for O-simpleAmbisonics Phase 2.2 — installed VST3 via pedalboard."""
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

ANGLES = {"azimuth": (-180.0, 180.0), "elevation": (-90.0, 90.0), "yaw": (-180.0, 180.0), "pitch": (-180.0, 180.0), "roll": (-180.0, 180.0)}

def setp(p, **kw):
    # Angles go in as the NORMALISED host value: a pedalboard setattr quantises to its own inferred grid
    # (123 -> 123.12, 37 -> 37.08), which is a measurer artefact, not the plugin's. raw_value is exact to a float ulp.
    for k, v in kw.items():
        if k in ANGLES:
            lo, hi = p.parameters[k].min_value, p.parameters[k].max_value
            p.parameters[k].raw_value = (float(v) - lo) / (hi - lo)
        else:
            setattr(p, k, v)

def run(p, x, reset=True):
    return p(x, FS, reset=reset)

def rms(a):
    return math.sqrt(float(np.mean(a.astype(np.float64) ** 2)) + 1e-30)

def db(a, b):
    return 20 * math.log10(max(a, 1e-30) / max(b, 1e-30))

# hand-evaluated cardioid pair on the first-order bus at 1 m / order 1 (W = 1)
def card(az, el=0.0):
    a, e = math.radians(az), math.radians(el)
    X, Y = math.cos(e) * math.cos(a), math.cos(e) * math.sin(a)
    return 0.5 + 0.4330127 * Y + 0.25 * X, 0.5 - 0.4330127 * Y + 0.25 * X

p = fresh()
keys = sorted(p.parameters.keys())
print("parameters:", keys)
check("P0 yaw / pitch / roll are host parameters", all(k in keys for k in ("yaw", "pitch", "roll")), "")
setp(p, azimuth=37.0, yaw=-37.0, pitch=123.0)
print(f"    raw_value sets read back: azimuth {float(p.azimuth):.6f} yaw {float(p.yaw):.6f} pitch {float(p.pitch):.6f}")
print(f"    ranges: yaw [{p.parameters['yaw'].min_value}, {p.parameters['yaw'].max_value}]"
      f"  pitch [{p.parameters['pitch'].min_value}, {p.parameters['pitch'].max_value}]"
      f"  roll [{p.parameters['roll'].min_value}, {p.parameters['roll'].max_value}]")

N = FS
rng = np.random.default_rng(11)
noise = rng.standard_normal((2, N)).astype(np.float32) * 0.1
tone = (0.5 * np.sin(2 * np.pi * 200 * np.arange(N) / FS)).astype(np.float32)
tone2 = np.stack([tone, tone])
imp = np.zeros((2, 4096), np.float32); imp[:, 100] = 1.0

def stereo(x, **kw):
    p = fresh(); setp(p, decoder="Stereo", order="1", **kw)
    return run(p, x)

def impulse_at(**kw):
    """Impulse anchor: (index of the peak, L, R at that index, max |other sample|)."""
    y = stereo(imp, **kw)
    k = int(np.argmax(np.abs(y[0]) + np.abs(y[1])))
    rest = np.abs(y[0]) + np.abs(y[1]); rest[k] = 0
    return k, float(y[0, k]), float(y[1, k]), float(rest.max())

# ---------- V1: yaw moves the image by the azimuth pan law (closed-form anchors) ----------
for yaw, az_equiv in ((90.0, 90.0), (-90.0, -90.0), (180.0, 180.0), (37.0, 37.0)):
    k, L, R, rest = impulse_at(azimuth=0.0, yaw=yaw)
    eL, eR = card(az_equiv)
    check(f"V1 yaw {yaw:+.0f} on a front source: impulse at 100 (latency still 64, compensated)", k == 100, f"index {k}")
    check(f"V1 yaw {yaw:+.0f}: L / R == hand cardioid at azimuth {az_equiv:+.0f} ({eL:.6f} / {eR:.6f}) within 1e-5",
          abs(L - eL) < 1e-5 and abs(R - eR) < 1e-5, f"L {L:.7f} R {R:.7f}; other samples max {rest:.1e}")
y = stereo(tone2, azimuth=0.0, yaw=90.0)[:, FS // 2:]
sep = db(rms(y[0]), rms(y[1]))
check("V1 yaw +90, Stereo: L louder than R by 22.88 dB (0.05) — the azimuth pan law", abs(sep - 22.88) < 0.05, f"{sep:.2f} dB")
y = stereo(tone2, azimuth=0.0, yaw=-90.0)[:, FS // 2:]
sep = db(rms(y[1]), rms(y[0]))
check("V1 yaw -90, Stereo: R louder than L by 22.88 dB (0.05)", abs(sep - 22.88) < 0.05, f"{sep:.2f} dB")

# ---------- V2: yaw ≡ azimuth on the installed binary (whole render, noise) ----------
for ang in (90.0, 37.0, -123.0):
    a = stereo(noise, azimuth=0.0, yaw=ang)
    b = stereo(noise, azimuth=ang, yaw=0.0)
    d = float(np.max(np.abs(a - b)))
    check(f"V2 yaw {ang:+.0f} (az 0) == azimuth {ang:+.0f} (yaw 0): max |diff| < 1e-5 over 1 s of noise", d < 1e-5,
          f"max |diff| {d:.2e}; rmsL {rms(a[0]):.4f} rmsR {rms(a[1]):.4f}")

# ---------- V3: yaw = −azimuth brings the source back to front (FUNC-01 third box) ----------
for az in (90.0, 37.0, -150.0):
    y = stereo(noise, azimuth=az, yaw=-az)
    f = stereo(noise, azimuth=0.0, yaw=0.0)
    d = float(np.max(np.abs(y[0] - y[1])))
    dfront = float(np.max(np.abs(y - f)))
    check(f"V3 az {az:+.0f} / yaw {-az:+.0f}: L == R (max |L−R| < 1e-5) and == the front render (< 1e-5)",
          d < 1e-5 and dfront < 1e-5, f"|L−R| {d:.2e}; vs front {dfront:.2e}; rmsL {rms(y[0]):.4f}")
# the host parameter the puck shows is untouched by the rotation
p = fresh(); setp(p, decoder="Stereo", azimuth=90.0, yaw=-90.0); run(p, noise[:, :4096])
check("V3 azimuth host parameter stays +90 under yaw −90 (the puck does not move)", abs(float(p.azimuth) - 90.0) < 1e-6, f"azimuth reads {float(p.azimuth):.4f}")

# ---------- V4: pitch + raises the front, roll + raises the left (signs pinned off the equator) ----------
# source front-up (az 0, el 45): pitch +45 → overhead (X = 0 → L = R = 0.5); pitch −45 → front (X = 1 → 0.75)
k, L, R, _ = impulse_at(azimuth=0.0, elevation=45.0, pitch=45.0)
eL, eR = card(0.0, 90.0)
check("V4 pitch +45 on (az 0, el 45): source goes OVERHEAD — L = R = 0.5 (X vanishes)", abs(L - eL) < 1e-5 and abs(R - eR) < 1e-5, f"L {L:.6f} R {R:.6f}")
k, L, R, _ = impulse_at(azimuth=0.0, elevation=45.0, pitch=-45.0)
eL, eR = card(0.0, 0.0)
check("V4 pitch −45 on (az 0, el 45): source comes DOWN to the front — L = R = 0.75", abs(L - eL) < 1e-5 and abs(R - eR) < 1e-5, f"L {L:.6f} R {R:.6f}")
# source left-up (az 90, el 45): roll +45 → overhead (Y = 0 → L = R = 0.5); roll −45 → hard left (Y = 1 → 0.933 / 0.067)
k, L, R, _ = impulse_at(azimuth=90.0, elevation=45.0, roll=45.0)
eL, eR = card(90.0, 90.0)
check("V4 roll +45 on (az 90, el 45): the LEFT rises to overhead — L = R = 0.5", abs(L - eL) < 1e-5 and abs(R - eR) < 1e-5, f"L {L:.6f} R {R:.6f}")
k, L, R, _ = impulse_at(azimuth=90.0, elevation=45.0, roll=-45.0)
eL, eR = card(90.0, 0.0)
check("V4 roll −45 on (az 90, el 45): the left SINKS to the equator — L = 0.933 / R = 0.067", abs(L - eL) < 1e-5 and abs(R - eR) < 1e-5, f"L {L:.6f} R {R:.6f}")
# pitch does not touch a source on the roll axis' equator the way roll does: pitch +90 on a front source → overhead
k, L, R, _ = impulse_at(azimuth=0.0, elevation=0.0, pitch=90.0)
check("V4 pitch +90 on a front source: overhead (L = R = 0.5), impulse still at 100", k == 100 and abs(L - 0.5) < 1e-5 and abs(R - 0.5) < 1e-5, f"index {k} L {L:.6f} R {R:.6f}")
# roll is about the FRONT axis: a source on the axis (az 0, el 0) is fixed; a front-UP source (az 0, el 45) swings to the
# RIGHT (left rises => up goes right): Rx(+90)·(x, y, z) = (x, −z, y) => (az −45, el 0)
k, L, R, _ = impulse_at(azimuth=0.0, elevation=0.0, roll=90.0)
check("V4 roll +90 on the front axis: invariant (L = R = 0.75)", abs(L - 0.75) < 1e-5 and abs(R - 0.75) < 1e-5, f"L {L:.6f} R {R:.6f}")
k, L, R, _ = impulse_at(azimuth=0.0, elevation=45.0, roll=90.0)
eL, eR = card(-45.0, 0.0)
check("V4 roll +90 on a front-UP source (el 45): lands at (az −45, el 0) — the up direction goes RIGHT when the left rises", abs(L - eL) < 1e-5 and abs(R - eR) < 1e-5, f"L {L:.6f} R {R:.6f} expected {eL:.6f} / {eR:.6f}")

# ---------- V5: composition order R = Rz(yaw)·Ry(pitch)·Rx(roll) — pitch applied before yaw ----------
# (az 0, el 0), pitch +90 then yaw +90: pitch sends the front overhead, yaw about z leaves overhead → L = R = 0.5.
# If yaw were applied first (front → left) and pitch after (pitch about the y axis leaves the left fixed) → L/R = 0.933/0.067.
k, L, R, _ = impulse_at(azimuth=0.0, elevation=0.0, yaw=90.0, pitch=90.0)
check("V5 yaw +90 & pitch +90 on a front source: overhead (pitch before yaw, R = Rz·Ry·Rx)", abs(L - 0.5) < 1e-5 and abs(R - 0.5) < 1e-5, f"L {L:.6f} R {R:.6f} (yaw-first would read 0.933 / 0.067)")
# (az 90, el 0) left source: roll +90 then yaw: roll sends the left overhead; yaw leaves it → 0.5 / 0.5
k, L, R, _ = impulse_at(azimuth=90.0, elevation=0.0, yaw=-90.0, roll=90.0)
check("V5 yaw −90 & roll +90 on a left source: overhead (roll before yaw)", abs(L - 0.5) < 1e-5 and abs(R - 0.5) < 1e-5, f"L {L:.6f} R {R:.6f} (yaw-first would read 0.75 / 0.75)")

# ---------- V6: orders change nothing in Stereo mode even under rotation (block-diagonal R, decoder reads W/Y/X) ----------
outs = {}
for o in ("1", "2", "3"):
    p = fresh(); setp(p, decoder="Stereo", azimuth=37.0, elevation=-12.0, distance_m=3.3, order=o, yaw=25.0, pitch=-40.0, roll=70.0)
    outs[o] = run(p, noise)
check("V6 Stereo output bit-equal for order 1 / 2 / 3 at (ψ 25, θ −40, φ 70) — the order-1 block is self-contained",
      np.array_equal(outs["1"], outs["2"]) and np.array_equal(outs["2"], outs["3"]) and bool(np.all(np.isfinite(outs["3"]))), f"rmsL {rms(outs['3'][0]):.4f}")
check("V6 the rotated render is live and differs from the unrotated one", float(np.max(np.abs(outs["1"] - stereo(noise, azimuth=37.0, elevation=-12.0, distance_m=3.3)))) > 1e-3, "")

# ---------- V7: yaw moving — a 360° sweep over 1 s is finite, live and smooth; a 0 → 180 flick is finite ----------
p = fresh(); setp(p, decoder="Stereo", azimuth=0.0, order="3")
blk = 256
out = np.zeros((2, N), np.float32)
run(p, np.zeros((2, 4096), np.float32), reset=True)
for i in range(0, N, blk):
    setp(p, yaw=((360.0 * i / N + 180.0) % 360.0) - 180.0)
    out[:, i:i + blk] = run(p, tone2[:, i:i + blk], reset=False)
q = N // 4
e = [rms(out[0, i * q:(i + 1) * q]) for i in range(4)]
check("V7 yaw 360°/s sweep (256-sample host steps): finite, L energy swings ≥ 6 dB across quarter-turns",
      bool(np.all(np.isfinite(out))) and db(max(e), min(e)) >= 6.0, " ".join(f"{v:.4f}" for v in e))
# zipper: the second difference of the output, normalised by its peak, vs the same tone rendered static. A 200 Hz tone's
# own second difference is a·(ωT)² ≈ 6.9e-4·a; a per-block angle jump of 1.92° would add a 1e-2-sized kink (ratio ≈ 20).
def norm2(y):
    return float(np.max(np.abs(np.diff(y.astype(np.float64), 2)))) / float(np.max(np.abs(y)))
stat = stereo(tone2, azimuth=0.0, yaw=0.0)[:, 4096:]
ratio = max(norm2(out[c, 4096:]) / norm2(stat[c]) for c in (0, 1))
check("V7 yaw sweep: normalised second-difference ratio vs the static tone ≤ 2.0 on both channels (zipper gate, installed binary)",
      ratio <= 2.0, f"ratio {ratio:.3f}")
# negative control for the gate: the same sweep with the smoothing defeated is not available from outside the plugin; instead the
# tone with a synthetic 1.92° per-256-block amplitude STEP (the hand cardioid, snapped) reads the ratio a zipper would produce
snap = np.concatenate([tone[i:i + blk] * (0.5 + 0.5 * math.cos(math.radians(((360.0 * i / N + 180.0) % 360.0) - 180.0 - 60.0))) for i in range(0, N, blk)])
check("V7 negative control: a per-block-snapped cardioid envelope FAILS the gate (expect ≥ 5)", norm2(snap[4096:]) / norm2(stat[0]) >= 5.0,
      f"ratio {norm2(snap[4096:]) / norm2(stat[0]):.1f}  FAIL-as-expected")
p = fresh(); setp(p, decoder="Stereo", azimuth=0.0, order="3", yaw=0.0)
run(p, tone2[:, :FS // 2], reset=True)
setp(p, yaw=180.0)
y = run(p, tone2[:, :FS // 2], reset=False)
check("V7 yaw 0 → 180 flick at order 3: finite; settles to the rear anchor (L = R, −9.54 dB re front) within 0.25 s",
      bool(np.all(np.isfinite(y))) and abs(rms(y[0, FS // 4:]) - rms(y[1, FS // 4:])) < 1e-4
      and abs(db(rms(y[0, FS // 4:]), 0.75 * 0.5 / math.sqrt(2)) + 9.54) < 0.05,
      f"rear L {rms(y[0, FS//4:]):.5f} R {rms(y[1, FS//4:]):.5f} ({db(rms(y[0, FS//4:]), 0.75 * 0.5 / math.sqrt(2)):+.2f} dB re front)")

# ---------- V8: extremes with the three angles at their corners, both decoders, orders 1 / 3 ----------
bad = 0; n = 0
for yaw in (-180.0, 180.0):
    for pitch in (-90.0, 90.0):
        for roll in (-180.0, 180.0):
            for o in ("1", "3"):
                for dec in ("Binaural", "Stereo"):
                    p = fresh(); setp(p, decoder=dec, azimuth=-180.0, elevation=90.0, distance_m=0.2, order=o, yaw=yaw, pitch=pitch, roll=roll, orbit=True, orbit_rate_hz=2.0)
                    y = run(p, noise[:, :4096]); n += 1
                    if not np.all(np.isfinite(y)) or rms(y) == 0.0: bad += 1
check(f"V8 {n} rotation corners (±180 / ±90 / ±180 × orders × decoders, pole source, orbit 2 Hz) finite and not silenced", bad == 0, f"{bad} bad")

# ---------- V9: state round-trip of the three angles (decoded values) ----------
p = fresh(); setp(p, yaw=123.0, pitch=-45.0, roll=77.0)
src = (float(p.yaw), float(p.pitch), float(p.roll))
st = p.raw_state
q = fresh(); q.raw_state = st
vals = (float(q.yaw), float(q.pitch), float(q.roll))
check("V9 yaw / pitch / roll survive a state round-trip (decoded values, source read-back vs restored)",
      all(abs(a - b) < 1e-3 for a, b in zip(src, vals)) and abs(vals[0] - 123) < 1e-3 and abs(vals[1] + 45) < 1e-3 and abs(vals[2] - 77) < 1e-3, f"source {src} restored {vals}")

print()
print("ALL PASS" if not fails else f"{len(fails)} FAILED: {fails}")
sys.exit(1 if fails else 0)
