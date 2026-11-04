# O-simpleGrain block-size null gate (v1.6.0).
#
# Every render runs on a FRESH instance of the installed VST3 that has loaded one
# template instance's state (its parameters and its random per-instance RNG seed).
# The same MIDI phrase is rendered at several host block sizes; each must null
# against the 512-sample reference to <= -120 dBFS (peak of the difference).
#
# Controls:
#   - non-vacuous: every reference render must carry signal
#   - seed is live: references rendered from different templates must NOT null
#     (skipped for "scan-only", which draws no randomness)
#   - positive control: run against v1.5.6 with --expect-fail; "scan-only" (no
#     spray/scatter, so the old clock-seeded RNG has no effect) must still fail
#     there, from the block-start playhead alone
#
# Reaper check: 400 state loads alternating fire/water with no processBlock in
# between. Each load publishes a new source; before v1.6.0 every one stayed
# parked (2.6 s mono @ 48 kHz = 0.48 MiB each, ~190 MiB in all). Gate: current
# RSS grows < 20 MiB.
#
# v2.0.0: three Pulsar configs (prefix "pulsar-"; Sine, Sample + FM + glisson + Scan,
# Mask 3 + Skip 30 % with the ADSR off) run the same nulls. The pulse clock is a
# double phase accumulator with fractional onsets, so these must be block-size
# independent too. Their extra settings are applied by name (choices) or by
# bisecting raw_value on the read-back value (Formant / FM Ratio are skewed).
# --cloud-only skips them (a pre-2.0.0 binary has no Pulsar parameters).
#
# usage: uv run --python 3.12 --with pedalboard --with numpy python -I gate.py [plugin.vst3] [--expect-fail] [--cloud-only]
import os
import subprocess
import sys
import numpy as np
import pedalboard

ARGS = [a for a in sys.argv[1:] if not a.startswith("--")]
EXPECT_FAIL = "--expect-fail" in sys.argv
CLOUD_ONLY = "--cloud-only" in sys.argv
PLUGIN = ARGS[0] if ARGS else \
    "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3"
FS = 48000
REF_BLOCK = 512
BLOCKS = [32, 441, 7]
LIMIT_DB = -120.0
N_TEMPLATES = 2
N_SEED_CHECK = 3
DURATION = 6.0
N_RELOADS = 400


def param_key(p, name):
    """pedalboard keys carry unit suffixes on some params — match by prefix."""
    if name in p.parameters:
        return name
    keys = [k for k in p.parameters if k.startswith(name + "_")]
    assert len(keys) == 1, f"param {name!r}: {keys} in {list(p.parameters)}"
    return keys[0]


def set_raw(p, name, value):
    par = p.parameters[param_key(p, name)]
    par.raw_value = (value - par.min_value) / (par.max_value - par.min_value)


def set_exact(p, name, value):
    """Choices by name; numbers by bisecting raw_value on the read-back (skew-safe)."""
    k = param_key(p, name)
    if isinstance(value, (str, bool)):
        setattr(p, k, value)
        return
    par = p.parameters[k]
    lo, hi = 0.0, 1.0
    for _ in range(48):
        mid = 0.5 * (lo + hi)
        par.raw_value = mid
        if float(getattr(p, k)) < value:
            lo = mid
        else:
            hi = mid
    par.raw_value = hi


# name: (numeric params as plain values, window, adsr on, uses randomness)
CONFIGS = {
    # moving playhead + every spray and scatter
    "scan-spray": (dict(grain_size=87.0, density=53.0, scan=73.0, position=20.0,
                        position_spray=40.0, pitch_spray=5.0, pan_spray=80.0, scatter=60.0,
                        vel_density=50.0), "Hann", True, True),
    # reverse, over the 24-grain cap (steals), full scatter, ADSR bypassed
    "reverse-steal": (dict(grain_size=211.0, density=170.0, scan=-137.0, position=60.0,
                           position_spray=15.0, pitch_spray=2.0, pan_spray=30.0, scatter=100.0,
                           vel_density=0.0), "Tukey", False, True),
    # no randomness at all: only the per-sample playhead can make block sizes differ
    "scan-only": (dict(grain_size=41.0, density=97.0, scan=-61.0, position=45.0,
                       position_spray=0.0, pitch_spray=0.0, pan_spray=0.0, scatter=0.0,
                       vel_density=0.0), "Hann", True, False),
    # v2.0.0 Pulsar: Sine pulsarets with pitch / pan spray
    "pulsar-sine": (dict(pitch_spray=3.0, pan_spray=60.0), "Hann", True, True,
                    dict(grain_mode="Pulsar", pulsaret="Sine", formant=5.0)),
    # Sample content on a moving playhead, rate FM (non-integer ratio), glisson, Tukey
    "pulsar-sample-fm": (dict(scan=73.0, position=20.0, position_spray=40.0, pan_spray=80.0),
                         "Tukey", True, True,
                         dict(grain_mode="Pulsar", pulsaret="Sample", formant=7.0, fm_depth=70.0,
                              fm_ratio=1.37, glisson=5.0)),
    # Mask 3 + Skip 30 %, ADSR bypassed (release drains), FM on
    "pulsar-mask-skip": (dict(pan_spray=30.0), "Welch", False, True,
                         dict(grain_mode="Pulsar", pulsaret="Sine", formant=3.0, pulse_mask="3",
                              grain_skip=30.0, fm_depth=40.0, fm_ratio=0.5)),
}
if CLOUD_ONLY:
    CONFIGS = {k: v for k, v in CONFIGS.items() if not k.startswith("pulsar-")}

# (sample, status, note, velocity). Odd positions, plus pairs 13 and 5 samples
# apart, which the synth's default 32-sample subdivision used to pull together.
EVENTS = [
    (1009, 0x90, 60, 100), (1022, 0x90, 64, 70), (1027, 0x90, 67, 120),
    (70417, 0x80, 64, 0),
    (90001, 0x90, 55, 90),
    (150333, 0x80, 60, 0), (150346, 0x80, 67, 0),
    (170777, 0x90, 72, 40), (170782, 0x90, 48, 127),
    (230011, 0x80, 55, 0), (240123, 0x80, 72, 0), (250009, 0x80, 48, 0),
]


def midi():
    return [(bytes([st, n, v]), (s + 0.25) / FS) for s, st, n, v in EVENTS]


def make_template(cfg):
    nums, window, adsr, _ = CONFIGS[cfg][:4]
    extra = CONFIGS[cfg][4] if len(CONFIGS[cfg]) > 4 else {}
    p = pedalboard.load_plugin(PLUGIN)
    for k, v in nums.items():
        set_raw(p, k, v)
    setattr(p, param_key(p, "window"), window)
    setattr(p, param_key(p, "adsr"), adsr)
    for k, v in extra.items():
        set_exact(p, k, v)
    return p.raw_state


def render(state, block):
    p = pedalboard.load_plugin(PLUGIN)
    p.raw_state = state
    y = p(midi(), duration=DURATION, sample_rate=FS, num_channels=2,
          buffer_size=block, reset=True)
    assert np.isfinite(y).all(), f"block {block}: non-finite output"
    return y


def k_pulsar_live(cfg):
    """True (= a failure) if the Pulsar config renders the same in Cloud: the mode is dead."""
    state = make_template(cfg)
    p = pedalboard.load_plugin(PLUGIN)
    p.raw_state = state
    setattr(p, param_key(p, "grain_mode"), "Cloud")
    d = np.max(np.abs(render(state, REF_BLOCK) - render(p.raw_state, REF_BLOCK)))
    dead = d < 1e-3
    print(f"{'FAIL' if dead else 'PASS'} {cfg}: Pulsar differs from the same state in Cloud (max diff {db(d):.1f} dBFS)")
    return dead


def db(v):
    return 20.0 * np.log10(max(float(v), 1e-30))


def rss_mib():
    out = subprocess.run(["ps", "-o", "rss=", "-p", str(os.getpid())], capture_output=True, text=True)
    return int(out.stdout.strip()) / 1024.0                                    # current RSS, KiB -> MiB


def reaper_check():
    p = pedalboard.load_plugin(PLUGIN)
    p(midi()[:1], duration=0.05, sample_rate=FS, buffer_size=512, reset=True)   # prepare at 48 k
    src = param_key(p, "source")
    states = []
    for name in ("fire", "water"):
        setattr(p, src, name)
        states.append(p.raw_state)
    before = rss_mib()
    for i in range(N_RELOADS):
        p.raw_state = states[i % 2]          # synchronous re-decode + publish
    grew = rss_mib() - before
    ok = grew < 20.0
    print(f"{'PASS' if ok else 'FAIL'} reaper: RSS +{grew:.1f} MiB over {N_RELOADS} idle source loads")
    return 0 if ok else 1


def main():
    print(f"plugin: {PLUGIN}{'  (expect fail)' if EXPECT_FAIL else ''}")
    fails = 0
    scan_only_failed = False
    for cfg in CONFIGS:
        random_cfg = CONFIGS[cfg][3]
        if cfg.startswith("pulsar-") and k_pulsar_live(cfg):
            fails += 1
        refs = []
        for k in range(N_TEMPLATES + (N_SEED_CHECK if random_cfg else 0)):
            state = make_template(cfg)
            ref = render(state, REF_BLOCK)
            rms = db(np.sqrt(np.mean(ref ** 2)))
            if rms < -70.0:
                print(f"FAIL {cfg} seed#{k}: reference {rms:.1f} dBFS RMS (vacuous)")
                fails += 1
            refs.append(ref)
            if k >= N_TEMPLATES:
                continue
            for bs in BLOCKS:
                d = np.max(np.abs(render(state, bs) - ref))
                ok = db(d) <= LIMIT_DB
                fails += not ok
                if cfg == "scan-only" and not ok:
                    scan_only_failed = True
                tag = "bit-exact" if d == 0 else f"{db(d):.1f} dBFS"
                print(f"{'PASS' if ok else 'FAIL'} {cfg} seed#{k} block {bs:>3} vs {REF_BLOCK}: "
                      f"{tag}   (ref RMS {rms:.1f} dBFS)")
        if random_cfg:
            same = [(i, j) for i in range(len(refs)) for j in range(i + 1, len(refs))
                    if db(np.max(np.abs(refs[i] - refs[j]))) <= LIMIT_DB]
            if same:
                print(f"FAIL {cfg}: templates {same} null against each other (seed not live/restored)")
                fails += 1
            else:
                print(f"PASS {cfg}: {len(refs)} seeds render pairwise distinct")
    fails += reaper_check()

    if EXPECT_FAIL:
        ok = fails > 0 and scan_only_failed
        print("POSITIVE CONTROL", "OK (gate fails, incl. scan-only)" if ok else "BROKEN (gate passed)")
        return 0 if ok else 1
    print("GATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
