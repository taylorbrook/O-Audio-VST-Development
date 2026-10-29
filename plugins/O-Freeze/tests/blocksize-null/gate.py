# O-Freeze block-size null gate (v2.8.0; Spectral configs added in v2.9.0).
#
# Every render runs on a FRESH instance of the installed VST3 that has loaded one
# template instance's state (its parameters and its random per-instance RNG seed).
# The same stimulus is rendered at several host block sizes; each must null against
# the 512-sample reference to <= -120 dBFS (peak of the difference).
#
# Controls:
#   - non-vacuous: the frozen section of every reference render must carry signal
#   - seed is live: references rendered from different templates must NOT null
#
# usage: uv run --python 3.12 --with pedalboard --with numpy python -I gate.py [plugin.vst3]
import sys
import numpy as np
import pedalboard

PLUGIN = sys.argv[1] if len(sys.argv) > 1 else \
    "/Users/taylorbrook/Library/Audio/Plug-Ins/VST3/O-Freeze-dev.vst3"
FS = 48000
REF_BLOCK = 512
BLOCKS = [32, 441, 7]
LIMIT_DB = -120.0
N_TEMPLATES = 2       # seeds carried through the full block-size null
N_SEED_CHECK = 4      # extra templates rendered at REF_BLOCK only (distinctness)


def stimulus(seconds, seed):
    rng = np.random.default_rng(seed)
    n = int(seconds * FS)
    t = np.arange(n) / FS
    chord = sum(0.12 * np.sin(2 * np.pi * f * t + p)
                for f, p in [(220.0, 0.0), (277.18, 1.0), (329.63, 2.0), (466.16, 0.5)])
    sweep = 0.1 * np.sin(2 * np.pi * (200 * t + 300 * t * t / seconds))
    noise = 0.05 * rng.standard_normal((2, n))
    x = np.stack([chord + sweep, chord - 0.5 * sweep]) + noise
    return x.astype(np.float32)


def set_raw(p, key, value):
    par = p.parameters[key]
    par.raw_value = (value - par.min_value) / (par.max_value - par.min_value)


CONFIGS = {
    # name: (mode, params{key: plain value}, choice/bool setters)
    "manual-fwd": (0, dict(drift=40.0, grain_size_ms=337.0, grain_count=7, lfo_rate_hz=2.0,
                           lfo_depth=60.0, detune_cents=12.0, mix=100.0),
                   dict(lfo_shape="Random", reverse=False)),
    "manual-rev": (0, dict(drift=25.0, grain_size_ms=53.0, grain_count=32, lfo_rate_hz=5.3,
                           lfo_depth=35.0, detune_cents=0.0, mix=80.0),
                   dict(lfo_shape="Sine", reverse=True)),
    "threshold": (1, dict(drift=55.0, grain_size_ms=211.0, grain_count=11, lfo_rate_hz=0.7,
                          lfo_depth=50.0, detune_cents=7.0, mix=100.0, threshold_db=-40.0),
                  dict(lfo_shape="Random", reverse=False)),
    # v2.9.0 Spectral engine. Every one has a seed-dependent draw (drift walk, beta
    # phase noise or detune), so the seed-liveness control applies to them too.
    "spec-manual-b0": (0, dict(drift=30.0, lfo_depth=0.0, detune_cents=10.0, mix=100.0,
                               diffusion=0.0, avg_frames=8),
                       dict(lfo_shape="Sine", reverse=False, engine="Spectral")),
    "spec-manual-b60-lfo": (0, dict(drift=70.0, lfo_rate_hz=1.3, lfo_depth=40.0, detune_cents=25.0,
                                    mix=90.0, diffusion=60.0, avg_frames=3),
                            dict(lfo_shape="Random", reverse=False, engine="Spectral")),
    "spec-threshold-b100": (1, dict(drift=0.0, lfo_depth=0.0, detune_cents=0.0, mix=100.0,
                                    threshold_db=-40.0, diffusion=100.0, avg_frames=16),
                            dict(lfo_shape="Sine", reverse=False, engine="Spectral")),
    # engine switch while frozen, both directions (Granular -> Spectral -> Granular)
    "spec-switch": (0, dict(drift=20.0, grain_size_ms=150.0, grain_count=9, lfo_rate_hz=0.9,
                            lfo_depth=30.0, detune_cents=8.0, mix=100.0, diffusion=35.0, avg_frames=5),
                    dict(lfo_shape="Triangle", reverse=False, engine="Granular", switch=True)),
}
GRANULAR_CONFIGS = [c for c in CONFIGS if "engine" not in CONFIGS[c][2]]


def make_template(cfg):
    mode, nums, others = CONFIGS[cfg]
    p = pedalboard.load_plugin(PLUGIN)
    p.mode = "Threshold" if mode == 1 else "Manual"
    for k, v in nums.items():
        set_raw(p, k, v)
    p.lfo_shape = others["lfo_shape"]
    p.reverse = others["reverse"]
    if "engine" in others:
        p.engine = others["engine"]
    p.freeze = False
    return p.raw_state


def render(cfg, state, block):
    p = pedalboard.load_plugin(PLUGIN)
    p.raw_state = state
    mode = CONFIGS[cfg][0]
    if mode == 0:
        # fill 2.0 s, freeze 4.0 s, release 2.0 s. Calls are block-aligned in neither
        # render (lengths are not multiples of 441 or 7), which is the point.
        x = stimulus(8.0, 11)
        a, b = int(2.0 * FS) + 123, int(6.0 * FS) + 457
        p.freeze = False
        y0 = p.process(x[:, :a], FS, buffer_size=block, reset=True)
        p.freeze = True
        if CONFIGS[cfg][2].get("switch"):
            # switch the engine twice while frozen, at odd sample positions
            s1, s2 = a + int(1.3 * FS) + 77, a + int(2.6 * FS) + 311
            y1a = p.process(x[:, a:s1], FS, buffer_size=block, reset=False)
            p.engine = "Spectral"
            y1b = p.process(x[:, s1:s2], FS, buffer_size=block, reset=False)
            p.engine = "Granular"
            y1c = p.process(x[:, s2:b], FS, buffer_size=block, reset=False)
            y1 = np.concatenate([y1a, y1b, y1c], axis=1)
        else:
            y1 = p.process(x[:, a:b], FS, buffer_size=block, reset=False)
        p.freeze = False
        y2 = p.process(x[:, b:], FS, buffer_size=block, reset=False)
        y = np.concatenate([y0, y1, y2], axis=1)
        frozen = (a, b)
    else:
        # loud / near-silent segments at odd sample positions: the gate engages and
        # releases mid-block, several times
        x = stimulus(8.0, 23)
        env = np.ones(x.shape[1], dtype=np.float32)
        for s, e in [(70001, 160777), (230513, 271009), (300003, 371111)]:
            env[s:e] = 1e-4
        x = x * env
        y = p.process(x, FS, buffer_size=block, reset=True)
        frozen = (75000, 160000)
    assert np.isfinite(y).all(), f"{cfg} block {block}: non-finite output"
    return y, frozen


def db(v):
    return 20.0 * np.log10(max(float(v), 1e-30))


def main():
    print(f"plugin: {PLUGIN}")
    fails = 0
    for cfg in CONFIGS:
        refs = []
        for k in range(N_TEMPLATES + N_SEED_CHECK):
            state = make_template(cfg)
            ref, (fa, fb) = render(cfg, state, REF_BLOCK)
            rms = db(np.sqrt(np.mean(ref[:, fa:fb] ** 2)))
            if rms < -40.0:
                print(f"FAIL {cfg} seed#{k}: frozen section {rms:.1f} dBFS (vacuous)")
                fails += 1
            refs.append(ref)
            if k >= N_TEMPLATES:
                continue
            for bs in BLOCKS:
                y, _ = render(cfg, state, bs)
                d = np.max(np.abs(y - ref))
                ok = db(d) <= LIMIT_DB
                fails += not ok
                tag = "bit-exact" if d == 0 else f"{db(d):.1f} dBFS"
                print(f"{'PASS' if ok else 'FAIL'} {cfg} seed#{k} block {bs:>3} vs {REF_BLOCK}: "
                      f"{tag}   (frozen RMS {rms:.1f} dBFS)")
        # seed liveness: every pair of templates must render differently
        same = [(i, j) for i in range(len(refs)) for j in range(i + 1, len(refs))
                if db(np.max(np.abs(refs[i] - refs[j]))) <= LIMIT_DB]
        if same:
            print(f"FAIL {cfg}: templates {same} null against each other (seed not live/restored)")
            fails += 1
        else:
            print(f"PASS {cfg}: {len(refs)} seeds render pairwise distinct")
    print("GATE", "PASS" if fails == 0 else f"FAIL ({fails})")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
