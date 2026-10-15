# O-simpleReverb — v3.0.0 family retrofit: bit-exact null against v2.0.0.
#
# Run (one process per VST3: both builds share the OuSr class ID):
#   uv run --no-project --python 3.12 --with pedalboard --with numpy python \
#       plugins/O-simpleReverb/tests/pedalboard/verify_3_null.py render <bundle.vst3> <out.npz> [--flip-lpon]
#   uv run ... verify_3_null.py compare <a.npz> <b.npz>
"""Null the v3 build against v2.0.0 through a real host wrapper.

The v3 retrofit is a rename plus a UI rewrite. The DSP is frozen; the one audio-path
edit is a pure static extraction of the tone maps. So every render below must be
bit-identical between the two builds.

Matrix (per sample rate 44.1 k and 96 k):
  - all 48 factory presets (normalised values read from the installed Factory/*.json,
    set by raw_value and matched by NAME, never by ID) x 3 stimuli:
    pink burst + silence, log sine sweep + silence, impulse + silence
  - one automation run: the pink stimulus in 512-sample calls with TYPE / SIZE / DECAY
    moved between calls (reset=False, so the moves land mid-stream)

Procedure per render: set the parameters, one second of silence with reset=True
(the smoothers settle), then the stimulus with reset=False. Both builds see exactly
the same call sequence.

--flip-lpon is the positive control: it toggles LOW CUT on every render, so the
compare must FAIL against an un-flipped render.
"""
import glob, json, os, sys
import numpy as np

RATES = (44100, 96000)
FACTORY = os.path.expanduser("~/Library/O-simpleReverb/Presets/Factory")

# APVTS ID -> pedalboard key (snake_case of the host NAME; pedalboard appends unit labels)
NAME_KEYS = {
    "TYPE": "type", "CHARACTER": "character", "WET": "wet", "DRY": "dry",
    "DECAY": "decay", "SIZE": "size", "LPFREQ": "low_cut_freq", "LPON": "low_cut_on",
}

def stimuli(fs):
    rng = np.random.default_rng(1234)   # fixed: both processes build the same inputs
    n_tail = int(3.0 * fs)
    # pink burst: 0.4 s of 1/f noise at about -12 dBFS
    nb = int(0.4 * fs)
    w = rng.standard_normal(nb)
    W = np.fft.rfft(w); f = np.fft.rfftfreq(nb, 1 / fs); f[0] = f[1]
    pink = np.fft.irfft(W / np.sqrt(f), nb); pink *= 0.25 / np.max(np.abs(pink))
    # log sine sweep 20 Hz .. 0.45 fs, 1 s
    ns = int(1.0 * fs); t = np.arange(ns) / fs
    f0, f1 = 20.0, 0.45 * fs; k = np.log(f1 / f0)
    sweep = 0.25 * np.sin(2 * np.pi * f0 * (np.exp(t * k) - 1) / k)
    imp = np.zeros(1); imp[0] = 0.5
    out = {}
    for name, x in (("pink", pink), ("sweep", sweep), ("impulse", imp)):
        y = np.concatenate([x, np.zeros(n_tail)])
        out[name] = np.stack([y, np.roll(y, 7) * 0.9]).astype(np.float32)   # decorrelated-ish stereo
    return out

def resolve_keys(p):
    keys = {}
    for pid, base in NAME_KEYS.items():
        hits = [k for k in p.parameters.keys() if k == base or k.startswith(base + "_")]
        assert len(hits) == 1, f"{pid}: {hits} in {sorted(p.parameters.keys())}"
        keys[pid] = hits[0]
    return keys

def set_norm(p, keys, values, flip_lpon):
    for pid, v in values.items():
        if pid == "LPON" and flip_lpon:
            v = 0.0 if v >= 0.5 else 1.0
        p.parameters[keys[pid]].raw_value = float(v)

def render(bundle, out_path, flip_lpon):
    from pedalboard import load_plugin
    p = load_plugin(bundle)
    keys = resolve_keys(p)
    presets = sorted(glob.glob(os.path.join(FACTORY, "*.json")))
    assert len(presets) == 48, f"{len(presets)} factory presets in {FACTORY}"
    res = {}
    for fs in RATES:
        st = stimuli(fs)
        silence = np.zeros((2, fs), np.float32)
        for path in presets:
            vals = json.load(open(path))["parameters"]
            stem = os.path.splitext(os.path.basename(path))[0]
            for sname, x in st.items():
                set_norm(p, keys, vals, flip_lpon)
                p(silence, fs, reset=True)
                y = p(x, fs, reset=False)
                assert np.isfinite(y).all(), f"non-finite: {stem} {sname} {fs}"
                res[f"{fs}|{stem}|{sname}"] = y
        # automation: pink stimulus in 512-sample calls, TYPE / SIZE / DECAY moved between calls
        x = st["pink"]; blk = 512
        base = {"TYPE": 0.2, "CHARACTER": 0.5, "WET": 0.3, "DRY": 1.0, "DECAY": 0.5,
                "SIZE": 0.5, "LPFREQ": 0.47368, "LPON": 0.0}
        set_norm(p, keys, base, flip_lpon)
        p(silence, fs, reset=True)
        moves = {7: ("TYPE", 0.6), 19: ("SIZE", 0.9), 31: ("DECAY", 0.95), 47: ("TYPE", 1.0),
                 61: ("SIZE", 0.1), 83: ("DECAY", 0.2), 101: ("TYPE", 0.0), 140: ("TYPE", 0.8)}
        ys = []
        for i, s in enumerate(range(0, x.shape[1], blk)):
            if i in moves:
                pid, v = moves[i]; p.parameters[keys[pid]].raw_value = v
            ys.append(p(x[:, s:s + blk], fs, reset=False))
        y = np.concatenate(ys, axis=1)
        assert np.isfinite(y).all()
        res[f"{fs}|automation|pink"] = y
    np.savez(out_path, **{k.replace("|", "__"): v for k, v in res.items()})
    print(f"rendered {len(res)} buffers from {bundle} -> {out_path}")

def compare(a_path, b_path):
    a = np.load(a_path); b = np.load(b_path)
    ka, kb = set(a.files), set(b.files)
    if ka != kb:
        print(f"FAIL: key sets differ ({len(ka ^ kb)} keys)"); return 1
    bad = []
    for k in sorted(ka):
        x, y = a[k], b[k]
        if x.shape != y.shape or x.tobytes() != y.tobytes():
            d = float(np.max(np.abs(x - y))) if x.shape == y.shape else float("nan")
            bad.append((k, d))
    print(f"{len(ka)} buffers compared; {len(bad)} differ")
    for k, d in bad[:12]:
        print(f"  DIFF {k.replace('__', ' | ')}  max|d| = {d:.3e}")
    print("NULL: BIT-EXACT" if not bad else "NULL: DIFFERS")
    return 0 if not bad else 1

if __name__ == "__main__":
    if sys.argv[1] == "render":
        render(sys.argv[2], sys.argv[3], "--flip-lpon" in sys.argv)
    elif sys.argv[1] == "compare":
        sys.exit(compare(sys.argv[2], sys.argv[3]))
    else:
        sys.exit(f"usage: {sys.argv[0]} render|compare ...")
