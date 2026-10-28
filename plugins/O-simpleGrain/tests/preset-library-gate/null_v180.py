# O-simpleGrain v1.9.0 — audio null against v1.8.0. v1.9.0 adds presets and the
# sample library and changes no DSP, so every render from the SAME state (written
# by v1.8.0, i.e. a session saved by the old version) must be bit-exact between
# the two binaries. Configs: Fire defaults; every spray + Scan + Grain Pitch +7
# on Tukey with Skip 35 / Tilt -60; rect past the cap with Tilt +80; and a
# DROPPED user source (embedded in the session, v1.5.6 format) with Skip 20.
#
# usage: uv run --python 3.12 --with pedalboard --with numpy --with soundfile \
#            python -I null_v180.py OLD_v1.8.0.vst3 [NEW.vst3]
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "skip-tilt-gate"))
OLD = sys.argv[1]
NEW = sys.argv[2] if len(sys.argv) > 2 else \
    os.path.expanduser("~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3")
sys.argv = [sys.argv[0], OLD, NEW]
import numpy as np        # noqa: E402
import gate as G          # noqa: E402  (helpers only; its gates run under __main__)

ev = [(1009, 0x90, 60, 100), (1022, 0x90, 48, 70), (1027, 0x90, 55, 120),
      (70417, 0x80, 48, 0), (90001, 0x90, 36, 90), (150333, 0x80, 60, 0),
      (150346, 0x80, 55, 0), (230011, 0x80, 36, 0)]
tone = 0.4 * np.sin(2 * np.pi * 523.25 * np.arange(3 * G.FS) / G.FS)
cfgs = {
    "fire-defaults": dict(),
    "spray-tukey-up-skip-tilt": dict(nums=dict(grain_size_ms=87.0, density_g_s=53.0, scan=73.0, position=20.0,
                                               position_spray=40.0, pan_spray=80.0, scatter=60.0,
                                               vel_density=50.0, pitch_spray_st=5.0, taper=30.0),
                                     lin=dict(grain_pitch_st=7.0, grain_skip=35.0, grain_tilt=-60.0),
                                     window="Tukey"),
    "rect-past-cap-tilt": dict(nums=dict(grain_size_ms=500.0, density_g_s=200.0, scan=-61.0,
                                         position_spray=20.0), lin=dict(grain_tilt=80.0), window="Rectangular"),
    "dropped-source-skip": dict(nums=dict(grain_size_ms=60.0, density_g_s=80.0, position_spray=30.0),
                                lin=dict(grain_skip=20.0), source=("tone", tone)),
}
for name, kw in cfgs.items():
    st = G.build(OLD, **kw).raw_state
    a = G.render(OLD, st, ev, 6.0)
    b = G.render(NEW, st, ev, 6.0)
    d = float(np.max(np.abs(a - b)))
    rms = float(np.sqrt(np.mean(a ** 2)))
    G.verdict(d == 0.0 and rms > 1e-4,
              f"{name}: v1.8.0 state, v1.8.0 vs v1.9.0 render max|diff| = {d:.3g} (ref RMS {G.db(rms):.1f} dBFS)")
print("\nGATE", "PASS" if G.fails == 0 else f"FAIL ({G.fails})")
sys.exit(1 if G.fails else 0)
