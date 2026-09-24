#!/usr/bin/env python3
# O-Bowed register gate (v1.9.2, CR-01).
#
# Renders every factory preset across MIDI 45-96 with the render harness and
# checks that the string sounds its fundamental: steady-state rms above a floor,
# the harmonics present within 30 dB of the strongest share gcd 1 (not locked
# to H2/H3), and H1 within +-10 c. The upper register (MIDI >= 74, where the
# CR-01 bow-position floor applies) must pass for every preset not listed in
# KNOWN; everything else is reported but not gated.
#
#   python3 plugins/O-Bowed/tests/register_gate.py [--sample-rate 44100] [--keep DIR]
#
# Needs numpy and the harness built with -DOUARICON_BUILD_TESTS=ON.

import argparse, itertools, math, os, re, subprocess, sys, tempfile, wave
from concurrent.futures import ThreadPoolExecutor
import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
HARNESS = os.path.join(ROOT, 'build/plugins/O-Bowed/tests/render-harness/'
                       'O-Bowed-render-test_artefacts/Release/O-Bowed-render-test')
NOTES = list(range(45, 97, 3)) + [73, 74, 76, 77, 79, 86, 88, 91]
GATED_FROM = 74

# Pre-existing, documented in NOTES.md Known Issues — reported, not gated.
KNOWN = {
    'Impossible Strings': 'reversed friction sharpens pitch as beta grows',
    'Double Bass': 'MIDI 96 is far outside the instrument; mid-register H2 islands',
}


def factory_presets():
    src = open(os.path.join(ROOT, 'plugins/O-Bowed/Source/PluginProcessor.cpp')).read()
    src = src[src.index('initializeFactoryPresets'):]
    out = {}
    for m in re.finditer(r'\{\s*"([^"]+)",\s*\{(.*?)\},\s*juce::var', src, re.S):
        out[m.group(1)] = dict(re.findall(r'\{"(\w+)",\s*([\d.]+)f\}', m.group(2)))
    return out


def read_left(path):
    with wave.open(path) as w:
        sr, ch, sw, n = w.getframerate(), w.getnchannels(), w.getsampwidth(), w.getnframes()
        raw = np.frombuffer(w.readframes(n), dtype=np.uint8).reshape(-1, ch, sw)[:, 0, :]
    if sw == 3:
        v = (raw[:, 0].astype(np.int32) | (raw[:, 1].astype(np.int32) << 8)
             | (raw[:, 2].astype(np.int32) << 16))
        v = np.where(v & 0x800000, v - 0x1000000, v)
        return v / 8388608.0, sr
    return np.frombuffer(raw.tobytes(), dtype='<i2') / 32768.0, sr


def classify(path, note):
    x, sr = read_left(path)
    seg = x[int(1.5 * sr):int(3.5 * sr)]
    if math.sqrt(float(np.mean(seg ** 2))) < 5e-3:
        return 'silent'
    f0 = 440.0 * 2 ** ((note - 69) / 12)
    X = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
    fr = np.fft.rfftfreq(len(seg), 1 / sr)
    levels, freqs = [], []
    for k in range(1, 9):
        if f0 * k > 0.95 * sr / 2:
            break
        band = (fr > f0 * k * 2 ** (-0.5 / 12)) & (fr < f0 * k * 2 ** (0.5 / 12))
        i = int(np.argmax(np.where(band, X, 0)))
        levels.append(20 * math.log10(X[i] + 1e-20))
        freqs.append(fr[i])
    top = max(levels)
    g = 0
    for k, l in enumerate(levels, 1):
        if l > top - 30:
            g = math.gcd(g, k)
    if g > 1:
        return f'H{g}'
    cents = 1200 * math.log2(freqs[0] / f0)
    return 'ok' if abs(cents) <= 10 else f'{cents:+.0f}c'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sample-rate', default='44100')
    ap.add_argument('--keep')
    a = ap.parse_args()
    if not os.path.exists(HARNESS):
        sys.exit(f'harness not built: {HARNESS}')
    out = a.keep or tempfile.mkdtemp(prefix='obowed-register-')
    os.makedirs(out, exist_ok=True)
    presets = factory_presets()
    notes = sorted(set(NOTES))

    def job(key):
        name, n = key
        wav = os.path.join(out, f'{name.replace(" ", "_")}_n{n}.wav')
        cmd = [HARNESS, '--note', str(n), '--sustain', '4', '--sample-rate', a.sample_rate,
               '--out', wav, '--json', os.devnull]
        for k, v in presets[name].items():
            cmd += ['--param', f'{k}={v}']
        # Exit status is ignored: the harness's block-time sentinel is load-flaky
        # under parallel renders. Only the WAV matters here.
        subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if not os.path.exists(wav):
            raise RuntimeError(f'harness wrote no WAV for {name} MIDI {n}')
        return key, classify(wav, n)

    with ThreadPoolExecutor(os.cpu_count()) as ex:
        res = dict(ex.map(job, itertools.product(presets, notes)))

    print(f'sample rate {a.sample_rate}; gated from MIDI {GATED_FROM}')
    print(f'{"":20}' + ''.join(f'{n:>7}' for n in notes))
    failures = []
    for name in presets:
        print(f'{name:20}' + ''.join(f'{res[(name, n)]:>7}' for n in notes)
              + (f'   (known: {KNOWN[name]})' if name in KNOWN else ''))
        if name not in KNOWN:
            failures += [(name, n, res[(name, n)]) for n in notes
                         if n >= GATED_FROM and res[(name, n)] != 'ok']
    for f in failures:
        print('FAIL', *f)
    print('PASS' if not failures else f'FAIL ({len(failures)})')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
