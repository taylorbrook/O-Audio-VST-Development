#!/usr/bin/env python3
"""O-GranulatorInstrument: factory-sample ingest (1.2.0).

Reads the "ingest" block of every kind == "sample" entry in
Resources/factory/manifest.json and writes Resources/factory/<id>.flac:

  1. read the window from the original (float64), resample to 48 kHz
  2. seam sources: fold the last 0.5 s into the first 0.5 s with sin / cos
     gains (the file gets 0.5 s shorter; its last sample runs into its first;
     no end fades). Everything else: 10 ms linear fades.
  3. level rule: gain so the 90th-percentile 400 ms frame of the channel mean
     sits at -20 dBFS, capped so the peak stays under -1 dBFS. The gain is
     floored to 0.01 dB and recorded in the manifest ("gain_db"); a recorded
     gain is applied as it stands (delete it to re-measure).
  4. TPDF dither (seed = crc32 of the id), 16-bit, FLAC.

The originals live outside the repo (ROOT below). The committed .flac files are
the source of truth; this script and the manifest record how they were cut.
Only the folders Taylor allowed are read (ALLOWED), plus one named file.

  factory_ingest.py [--out DIR] [id ...]      default: every sample, into Resources/factory

For the seam sources it also writes tests/fixtures/factory-seam/<id>.flac
(0.3 s each): the reference and the control of the harness row FSM1.

Needs numpy, scipy, soundfile. Two runs give byte-identical files.
"""
import glob, json, math, os, sys, zlib
from math import gcd

import numpy as np
import soundfile as sf
from scipy.signal import resample_poly

HERE = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
MANIFEST = os.path.join(HERE, "Resources/factory/manifest.json")
ROOT = "/Users/taylorbrook/Projects/"
SR = 48000
ALLOWED = [
    "gameAudio/SP - Sound Effects/",
    "TensedComputerImproviser/",
    "electric guitar and improvisor/",
    "Soundscaper/git/soundscaper/audio/",
    "Nox/NOX - Performance Package/max/max/06_cry",
]
ALLOWED_FILES = ["five weather reports/Solo Version/samples/contrabassoon/G#0-31.wav"]
OLD8 = ["harmonic", "pluck", "bell", "vowel", "noise_bed", "dust", "drone", "pulse"]


def allowed(path):
    real = os.path.realpath(path)
    root = os.path.realpath(ROOT) + os.sep
    if not real.startswith(root):
        return False
    rel = real[len(root):]
    if "jeff" in rel.lower():          # the Jeff* corpora: another performer
        return False
    return rel in ALLOWED_FILES or any(rel.startswith(a) for a in ALLOWED)


def load(rel, a=None, b=None):
    path = os.path.join(ROOT, rel)
    if not allowed(path):
        sys.exit("REFUSED: %s is outside the allowed sample folders" % path)
    i = sf.info(path)
    x, sr = sf.read(path, start=int((a or 0) * i.samplerate), stop=None if b is None else int(b * i.samplerate),
                    dtype="float64", always_2d=True)
    if sr != SR:
        g = gcd(SR, sr)
        x = resample_poly(x, SR // g, sr // g, axis=0)
    return x


def fade(x, ms=10):
    n = int(SR * ms / 1000)
    r = np.linspace(0, 1, n)[:, None]
    x = x.copy()
    x[:n] *= r
    x[-n:] *= r[::-1]
    return x


def frames_db(m, hop):
    n = len(m) // hop
    return 10 * np.log10(np.mean(m[:n * hop].reshape(n, hop) ** 2, axis=1) + 1e-12)


def trim(x, thr):
    m = x.mean(axis=1)
    hop = 240
    d = frames_db(m, hop)
    ok = np.where(d > d.max() + thr)[0]
    return x[max(0, (ok[0] - 1) * hop):min(len(x), (ok[-1] + 3) * hop)]


def seam(x, seconds=0.5):
    n = int(seconds * SR)
    t = (np.arange(n) + 0.5) / n
    fi = np.sin(0.5 * np.pi * t)[:, None]
    fo = np.cos(0.5 * np.pi * t)[:, None]
    y = x[:len(x) - n].copy()
    y[:n] = x[:n] * fi + x[len(x) - n:] * fo
    return y


def collection(spec):
    """One-shots end to end, as audition_cut.py assembled them: each trimmed at
    -60 dB, mono, 3 ms fades, half-way level matching, a silent gap after each."""
    base = os.path.join(ROOT, spec["dir"])
    files = sorted(g for g in glob.glob(os.path.join(glob.escape(base), "**", spec["pattern"]), recursive=True)
                   if not os.path.basename(g).startswith("._"))
    if len(files) != spec["count"]:
        sys.exit("collection %s: %d files match, the manifest says %d" % (spec["pattern"], len(files), spec["count"]))
    parts = []
    for f in files:
        x = trim(load(os.path.relpath(f, ROOT)), -60.0)
        x = x.mean(axis=1, keepdims=True)
        pk = np.abs(x).max()
        x = fade(x, 3) * (0.6 / pk) ** 0.5
        parts += [x, np.zeros((int(spec["gap_s"] * SR), 1))]
    return np.concatenate(parts)


def cut(spec):
    if "pattern" in spec:
        return collection(spec)
    w = spec.get("window")
    x = load(spec["source"], w[0], w[1]) if w else load(spec["source"])
    if spec.get("head_s"):
        x = x[:int(spec["head_s"] * SR)]
    if spec.get("trim_db") is not None:
        x = trim(x, float(spec["trim_db"]))
    return x


def loud_frame_db(y):
    """The level measure of the harness row FLV1: 90th-percentile 400 ms frame of the channel mean."""
    return float(np.percentile(frames_db(y.mean(axis=1), int(0.4 * SR)), 90))


def main():
    args = sys.argv[1:]
    out = os.path.join(HERE, "Resources/factory")
    fixtures = os.path.join(HERE, "tests/fixtures/factory-seam")
    if args[:1] == ["--out"]:
        out = args[1]
        args = args[2:]
        fixtures = os.path.join(out, "factory-seam")
    with open(MANIFEST) as f:
        m = json.load(f)
    os.makedirs(out, exist_ok=True)
    total = 0
    print("%-17s ch    len    gain    peak     rms   loud |    bytes" % "id")
    for s in m["sources"]:
        if s["kind"] != "sample" or (args and s["id"] not in args):
            continue
        if s["id"] in OLD8:
            sys.exit("REFUSED: %s is one of the 1.0.0 set" % s["id"])
        spec = s["ingest"]
        x = cut(spec)
        pre = x
        x = seam(x) if s.get("seam") else fade(x)
        pk = 20 * math.log10(np.abs(x).max())
        if spec.get("gain_db") is None:
            g = min(-20.0 - loud_frame_db(x), -1.01 - pk)
            spec["gain_db"] = math.floor(g * 100.0) / 100.0
        y = x * 10 ** (spec["gain_db"] / 20)
        rng = np.random.default_rng(zlib.crc32(s["id"].encode()))
        d = rng.random(y.shape) - rng.random(y.shape)                      # TPDF, +-1 LSB
        z = np.clip(np.round(y * 32768.0 + d), -32768, 32767).astype(np.int16)
        fn = os.path.join(out, s["id"] + ".flac")
        sf.write(fn, z, SR, subtype="PCM_16", format="FLAC")
        s["channels"], s["frames"] = int(z.shape[1]), int(z.shape[0])
        if s.get("seam") and fixtures:
            # The harness row FSM1 reads the fold from the shipped file and its
            # reference from here: the centre 100 ms of the two segments the
            # fold mixed (head, tail), then the centre 100 ms of the SAME fold
            # made with linear gains (the row's control). 3 x 4800 frames, at
            # the shipped gain, 16-bit, no dither.
            n = int(0.5 * SR)
            a, b = int(0.2 * SR), int(0.3 * SR)
            head, tail = pre[:n], pre[len(pre) - n:]
            t = ((np.arange(n) + 0.5) / n)[:, None]
            lin = head * t + tail * (1.0 - t)
            fx = np.concatenate([head[a:b], tail[a:b], lin[a:b]]) * 10 ** (spec["gain_db"] / 20)
            os.makedirs(fixtures, exist_ok=True)
            sf.write(os.path.join(fixtures, s["id"] + ".flac"), np.clip(np.round(fx * 32768.0), -32768, 32767).astype(np.int16),
                     SR, subtype="PCM_16", format="FLAC")
        zf = z / 32768.0
        size = os.path.getsize(fn)
        total += size
        print("%-17s %d %7.2f %+7.2f %7.2f %7.2f %6.1f | %8d" % (
            s["id"], z.shape[1], len(z) / SR, spec["gain_db"], 20 * math.log10(np.abs(zf).max()),
            20 * math.log10(math.sqrt((zf ** 2).mean())), loud_frame_db(zf), size))
    print("total %d B" % total)
    with open(MANIFEST, "w") as f:
        json.dump(m, f, indent=1)
        f.write("\n")


if __name__ == "__main__":
    main()
