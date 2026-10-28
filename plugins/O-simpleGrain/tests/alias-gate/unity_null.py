# O-simpleGrain v1.7.0 regression control: grains at rate <= 1 still read through
# the 4-point Lagrange, so a session with no up-transposed grain must render
# bit-identically to v1.6.0. The same state (parameters + RNG seed, built-in Fire)
# is loaded into a fresh instance of each binary and rendered at 512.
#
#   down-spray  Grain Pitch -12, Pitch Spray 5 (max rate 2^(-7/12)), every spray,
#               Scatter, Scan 73 %, notes <= C3            verdict: bit-exact
#   unity       Grain Pitch 0, no pitch spray, notes <= C3 (rate exactly 1 at C3)
#                                                           verdict: bit-exact
#   up (live)   Grain Pitch +7: the change must be audible  verdict: differs
#
# usage: uv run --python 3.12 --with pedalboard --with numpy \
#            python -I unity_null.py OLD.vst3 [NEW.vst3]
import sys
import numpy as np
import pedalboard

OLD = sys.argv[1]
NEW = sys.argv[2] if len(sys.argv) > 2 else \
    "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3"
FS = 48000

COMMON = dict(grain_size_ms=87.0, density_g_s=53.0, scan=73.0, position=20.0,
              position_spray=40.0, pan_spray=80.0, scatter=60.0, vel_density=50.0)
CONFIGS = {
    "down-spray": (dict(COMMON, pitch_spray_st=5.0), -12.0, True),
    "unity":      (dict(COMMON, pitch_spray_st=0.0),   0.0, True),
    "up (live)":  (dict(COMMON, pitch_spray_st=0.0),   7.0, False),
}
EVENTS = [(1009, 0x90, 60, 100), (1022, 0x90, 48, 70), (1027, 0x90, 55, 120),
          (70417, 0x80, 48, 0), (90001, 0x90, 36, 90), (150333, 0x80, 60, 0),
          (150346, 0x80, 55, 0), (230011, 0x80, 36, 0)]


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


def template(nums, pitch):
    p = pedalboard.load_plugin(NEW)
    for k, v in nums.items():
        set_value(p, k, v)
    # Grain Pitch is linear (-24..24): set it exactly, so "unity" is rate 1, not 1 + eps.
    p.parameters["grain_pitch_st"].raw_value = (pitch + 24.0) / 48.0
    return p.raw_state


def render(plugin, state):
    p = pedalboard.load_plugin(plugin)
    p.raw_state = state
    msgs = [(bytes([st, n, v]), (s + 0.25) / FS) for s, st, n, v in EVENTS]
    y = p(msgs, duration=6.0, sample_rate=FS, num_channels=2, buffer_size=512, reset=True)
    assert np.isfinite(y).all()
    return y


fails = 0
for name, (nums, pitch, identical) in CONFIGS.items():
    st = template(nums, pitch)
    a, b = render(OLD, st), render(NEW, st)
    rms = 20 * np.log10(np.sqrt(np.mean(a ** 2)) + 1e-30)
    d = float(np.max(np.abs(a - b)))
    ok = (d == 0.0) if identical else (d > 1e-3)
    fails += not ok
    tag = "bit-exact" if d == 0 else f"max diff {20 * np.log10(d):.1f} dBFS"
    want = "bit-exact" if identical else "must differ"
    print(f"{'PASS' if ok else 'FAIL'} {name:11s} ({want}): {tag}   (ref RMS {rms:.1f} dBFS)")
    if rms < -70:
        print(f"FAIL {name}: vacuous reference"); fails += 1
print("GATE", "PASS" if fails == 0 else f"FAIL ({fails})")
sys.exit(1 if fails else 0)
