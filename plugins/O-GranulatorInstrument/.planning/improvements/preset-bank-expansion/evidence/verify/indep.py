#!/usr/bin/env python3
"""Independent check of FactoryPresets.h (not the harness rows): counts, names,
seeds, output_gain, enabled-stream source coverage, off-C3 roots, realism wobble."""
import re, json, sys, collections
src = open(sys.argv[1], encoding='utf-8').read()
man = json.load(open(sys.argv[2]))['sources']
roots = {s['id']: (s['root_note'], s['root_cents']) for s in man}
body = src[src.index('inline std::vector<FactoryPresetSpec> factoryPresetSpecs()'):]
starts = [m for m in re.finditer(r'\{\s*"([A-Za-z]+) \\xc2\\xb7 ([^"]+)",\s*\n\s*"', body)]
fails = []
presets = []
for i, m in enumerate(starts):
    end = starts[i+1].start() if i+1 < len(starts) else len(body)
    blk = body[m.start():end]
    cat, short = m.group(1), m.group(2)
    seed = re.search(r'(0x[0-9A-Fa-f]+)ll', blk).group(1)
    params = re.findall(r'\{\s*"([a-z0-9_*]+)",\s*([^}]+?)\s*\}', blk)
    slots = re.findall(r'\{\s*(\d)\s*,\s*"factory:([a-z_]+)"\s*,\s*"([^"]*)"\s*\}', blk)
    presets.append(dict(cat=cat, short=short, seed=int(seed, 16), params=params, slots=slots))
N = len(presets)
print('presets', N)
cc = collections.Counter(p['cat'] for p in presets)
print('categories', dict(sorted(cc.items())))
want = dict(Drones=20, Expressive=20, Field=12, Keys=20, Live=8, Motion=20, Pads=20, Rhythmic=20, Textures=20)
if N != 160 or dict(cc) != want: fails.append('counts')
names = [f"{p['cat']} · {p['short']}" for p in presets]
for n in names:
    if len(n) > 24 or any(c in n for c in '/\\:'): fails.append('name ' + n)
if len(set(names)) != N: fails.append('dup names')
seeds = [p['seed'] for p in presets]
if len(set(seeds)) != N: fails.append('dup seeds')
cmap = dict(Drones=1, Expressive=2, Keys=3, Live=4, Motion=5, Pads=6, Rhythmic=7, Textures=8, Field=9)
for p in presets:
    if (p['seed'] >> 32) & 0xFF != cmap[p['cat']] or p['seed'] >> 40 != 0x4F4749:
        fails.append(f"seed pattern {p['short']} {p['seed']:#x}")
print('max name len', max(len(n) for n in names), max(names, key=len))
# output_gain
for p in presets:
    if any(k == 'output_gain' for k, _ in p['params']): fails.append('output_gain ' + p['short'])
# enabled streams
def val(p, k, d=None):
    for kk, v in p['params']:
        if kk == k:
            try: return float(v.rstrip('f'))
            except ValueError: return v
    return d
use = collections.Counter()
for p in presets:
    seen = set()
    for idx, sid, lane in p['slots']:
        s = int(idx) + 1
        en = val(p, f's{s}_enable', 1.0 if s == 1 else 0.0)
        if en == 1.0: seen.add(sid)
        rn, rc = roots[sid]
        if (rn, rc) != (48, 0):
            gn, gf = val(p, f's{s}_root_note', 48.0), val(p, f's{s}_root_fine', 0.0)
            if (gn, gf) != (rn, rc): fails.append(f"root {p['short']} s{s} {sid} {gn}/{gf} want {rn}/{rc}")
    for sid in seen: use[sid] += 1
under = {s: use[s] for s in roots if use[s] < 2}
print('sources used >=2 on an enabled stream:', sum(1 for s in roots if use[s] >= 2), '/', len(roots), 'min', min(use[s] for s in roots))
if under: fails.append(f'coverage {under}')
# realism wobble (P3/P5): preset-added pitch_rand, LFO/env -> pitch routes, root_fine detune between enabled streams on the same source
REAL = r'\b(Piano|Rhodes|Celesta|Harp|Strings|Choir|Organ|Kalimba|Marimba|Guitar|Bassoon)\b'
nreal = 0
for p in presets:
    if not re.search(REAL, p['short']): continue
    nreal += 1
    bad = [k for k, v in p['params'] if k.endswith('pitch_rand') and val(p, k) not in (0.0, None)]
    for j in range(1, 9):
        srcv = val(p, f'mod{j}_source'); dst = val(p, f'mod{j}_dest')
        if srcv is None or dst is None: continue
        if isinstance(dst, str) and re.search(r'_(pitch|pitch_rand|fine)"', dst) and str(srcv) in ('Lfo1','Lfo2','Lfo3','Lfo4','Env1','Env2','1','2','3','4','5','6','1.0f','2.0f','3.0f','4.0f','5.0f','6.0f'):
            bad.append(f'mod{j} {srcv}->{dst}')
    if bad: fails.append(f"wobble {p['short']} {bad}")
print('realism-titled', nreal)
print('FAILS' if fails else 'ALL PASS', *fails, sep='\n  ')
