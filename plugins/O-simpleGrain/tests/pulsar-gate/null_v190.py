# O-simpleGrain v2.0.0 — Cloud-mode null against v1.9.0 (Wave A hard gate, A1..A7).
#
# Pulsar mode is branched around the Cloud engine, so every Cloud render must be
# bit-exact against the 1.9.0 binary. States come from three sources:
#   bs:*    every tests/blocksize-null config, state written by the OLD binary
#           (a pre-2.0.0 session: no Pulsar keys), rendered at blocks 512 and 441.
#   v180:*  every tests/preset-library-gate/null_v180.py config, OLD-written.
#   mig:*   the OLD-written state loaded into a NEW instance that was put in
#           Pulsar (formant 12, mask 3, FM 80 %, glisson +7) first: setState's
#           migration must reset all seven Pulsar params, so it renders as Cloud.
#   new:*   the state written by NEW with Mode = Cloud and every Pulsar control
#           moved off its default: Cloud must not read them (OLD ignores the keys).
# Liveness: every reference carries signal, and NEW in Pulsar on the same state
# must differ (else the mode switch is dead and the null proves nothing).
#
# usage: uv run --no-project --python 3.12 --with pedalboard --with numpy --with soundfile \
#            python -I null_v190.py OLD_v1.9.0.vst3 NEW.vst3 [--expect-fail] [--no-live]
# --expect-fail: NEW is a deliberately 1-ULP-perturbed Cloud build; the gate must FAIL.
import importlib.util
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ARGS = [a for a in sys.argv[1:] if not a.startswith("--")]
EXPECT_FAIL = "--expect-fail" in sys.argv
NO_LIVE = "--no-live" in sys.argv      # A1/A2 builds: Pulsar is not audible yet
OLD, NEW = ARGS[0], ARGS[1]

sys.path.insert(0, os.path.join(HERE, "..", "skip-tilt-gate"))
sys.argv = [sys.argv[0], OLD, NEW]
import numpy as np        # noqa: E402
import pedalboard         # noqa: E402
import gate as G          # noqa: E402  (helpers only)

spec = importlib.util.spec_from_file_location("bsnull", os.path.join(HERE, "..", "blocksize-null", "gate.py"))
BS = importlib.util.module_from_spec(spec)
spec.loader.exec_module(BS)

PULSAR_PRESET = dict(formant=12.0, fm_depth=80.0, fm_ratio=2.0, glisson=7.0)
fails = 0


def verdict(ok, msg):
    global fails
    fails += not ok
    print(f"{'PASS' if ok else 'FAIL'} {msg}")


def set_pulsar_controls(p, mode):
    setattr(p, BS.param_key(p, "grain_mode"), mode)
    for k, v in PULSAR_PRESET.items():
        G.set_value(p, BS.param_key(p, k), v)
    setattr(p, BS.param_key(p, "pulse_mask"), "3")
    setattr(p, BS.param_key(p, "pulsaret"), "Sample")


def render_with(plugin, state, events, dur, block=512, pre_pulsar=False):
    p = pedalboard.load_plugin(plugin)
    if pre_pulsar:
        set_pulsar_controls(p, "Pulsar")
    p.raw_state = state
    msgs = [(bytes([st, n, v]), (s + 0.25) / G.FS) for s, st, n, v in events]
    y = p(msgs, duration=dur, sample_rate=G.FS, num_channels=2, buffer_size=block, reset=True)
    assert np.isfinite(y).all()
    return y


def compare(name, a, b):
    d = float(np.max(np.abs(a - b)))
    rms = float(np.sqrt(np.mean(a ** 2)))
    verdict(d == 0.0 and rms > 1e-4,
            f"{name}: max|diff| = {d:.3g} ({'bit-exact' if d == 0 else G.db(d)}) ref RMS {G.db(rms):.1f} dBFS")


def main():
    print(f"OLD {OLD}\nNEW {NEW}{'  (expect fail)' if EXPECT_FAIL else ''}")
    states = {}
    BS.PLUGIN = OLD
    for cfg in BS.CONFIGS:
        if cfg.startswith("pulsar-"):
            continue                                  # Pulsar configs: no such params in 1.9.0
        states["bs:" + cfg] = (BS.make_template(cfg), BS.EVENTS, BS.DURATION)
    ev180 = [(1009, 0x90, 60, 100), (1022, 0x90, 48, 70), (1027, 0x90, 55, 120),
             (70417, 0x80, 48, 0), (90001, 0x90, 36, 90), (150333, 0x80, 60, 0),
             (150346, 0x80, 55, 0), (230011, 0x80, 36, 0)]
    tone = 0.4 * np.sin(2 * np.pi * 523.25 * np.arange(3 * G.FS) / G.FS)
    cfg180 = {
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
    for name, kw in cfg180.items():
        states["v180:" + name] = (G.build(OLD, **kw).raw_state, ev180, 6.0)

    for name, (st, ev, dur) in states.items():
        assert "grainMode" not in G.inner_xml(st)
        a = render_with(OLD, st, ev, dur)
        compare(f"{name} block 512", a, render_with(NEW, st, ev, dur))
        if name.startswith("bs:"):
            compare(f"{name} block 441", render_with(OLD, st, ev, dur, 441), render_with(NEW, st, ev, dur, 441))
        compare(f"mig:{name} (NEW pre-set to Pulsar, old state loaded)", a,
                render_with(NEW, st, ev, dur, pre_pulsar=True))

    # NEW-written Cloud states with every Pulsar control off its default.
    for name in ("v180:spray-tukey-up-skip-tilt", "bs:reverse-steal"):
        st, ev, dur = states[name]
        p = pedalboard.load_plugin(NEW)
        p.raw_state = st
        set_pulsar_controls(p, "Cloud")
        st_new = p.raw_state
        assert 'grainMode' in G.inner_xml(st_new)
        compare(f"new:{name} (NEW-written, Cloud, Pulsar controls moved) OLD vs NEW",
                render_with(OLD, st_new, ev, dur), render_with(NEW, st_new, ev, dur))
        # liveness: the same state in Pulsar must sound different.
        if NO_LIVE:
            continue
        setattr(p, BS.param_key(p, "grain_mode"), "Pulsar")
        y = render_with(NEW, p.raw_state, ev, dur)
        d = float(np.max(np.abs(y - render_with(NEW, st_new, ev, dur))))
        verdict(d > 1e-3, f"live:{name}: Pulsar differs from Cloud (max|diff| {G.db(d):.1f} dBFS)")

    if EXPECT_FAIL:
        ok = fails > 0
        print("NEGATIVE CONTROL", "OK (the perturbed build fails the null)" if ok else "BROKEN (gate passed)")
        return 0 if ok else 1
    print("\nGATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
