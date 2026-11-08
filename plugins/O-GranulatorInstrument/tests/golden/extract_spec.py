#!/usr/bin/env python3
"""O-GranulatorInstrument golden extractor (Stage 1, PLAN T2).

Derives the Stage 1 parameter goldens from the LOCKED contract only, never from
the code (a code-derived golden is self-referential).

Input:   .planning/parameter-spec.md ONLY (group tables, choice lists, the
         32-entry scale grid, the 18-row source table, the 126-entry dest grid
         and the Full ID list, which carries every host name). No YAML.
Outputs: <out>/params.tsv       idx id name type start end interval default skew centre choices
         <out>/mod-dests.tsv    idx id name class
         <out>/mod-sources.tsv  idx name class
         UTF-8, LF newlines, each starting with a "# GENERATED ... do not edit" line.

Usage:   python3 plugins/O-GranulatorInstrument/tests/golden/extract_spec.py [--spec FILE] [--out DIR]
         Defaults: the plugin's .planning/parameter-spec.md and this directory.
Exit 1 (and nothing written) if any cross-check fails; every failure is printed.
"""
import argparse
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PLUGIN = os.path.normpath(os.path.join(HERE, '..', '..'))

ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
ap.add_argument('--spec', default=os.path.join(PLUGIN, '.planning', 'parameter-spec.md'))
ap.add_argument('--out', default=HERE)
args = ap.parse_args()

errors = []


def err(msg):
    errors.append(msg)


def fatal(msg):
    for e in errors:
        print('FAIL:', e)
    print('FAIL:', msg)
    sys.exit(1)


with open(args.spec, encoding='utf-8', newline='') as f:
    raw = f.read()
if '\r' in raw:
    err('spec contains CR characters')
spec = raw.replace('−', '-')


def section(start, stop):
    a = spec.find(start)
    if a < 0:
        fatal('section not found: %r' % start)
    b = spec.find(stop, a + len(start))
    if b < 0:
        fatal('section end not found: %r' % stop)
    return spec[a + len(start):b]


def cells(line):
    return [c.strip() for c in line.strip().strip('|').split('|')]


# ---------------------------------------------------------------- Full ID list
idblock = spec.split('## Full ID list (registration order)')[1].split('```')[1]
idlist = []   # (id, type, host name)
for line in idblock.strip('\n').split('\n'):
    parts = line.split(None, 3)
    if len(parts) != 4:
        fatal('Full ID list row: %r' % line)
    k, pid, ptype, name = parts
    if int(k) != len(idlist):
        err('Full ID list index %s (expected %d)' % (k, len(idlist)))
    idlist.append((pid, ptype, name.strip()))
name_of = {pid: nm for pid, _, nm in idlist}
type_of = {pid: t for pid, t, _ in idlist}

# ---------------------------------------------------------------- group tables
ROW = re.compile(r'^\| `([a-zA-Z0-9_]+)` \| (Float(?: \(interval 1\))?|Choice|Bool) \| (.*?) \| (.*?) \| (.*?) \| (.*?) \| (.*) \|$')
rows = {}
for line in spec.split('\n'):
    m = ROW.match(line)
    if m:
        if m.group(1) in rows:
            err('duplicate template row %s' % m.group(1))
        rows[m.group(1)] = dict(type=m.group(2), range=m.group(3), default=m.group(4), skew=m.group(5), dest=m.group(6))

TEMPLATES = ((r'^s([1-4])_(.+)$', r'sN_\2'), (r'^menv([12])_(.+)$', r'menvN_\2'), (r'^lfo([1-4])_(.+)$', r'lfoN_\2'),
             (r'^macro([1-4])$', 'macroN'), (r'^mod(\d+)_(.+)$', r'modM_\2'))


def template_for(pid):
    for pat, tpl in TEMPLATES:
        m = re.match(pat, pid)
        if m:
            return re.sub(pat, tpl, pid), int(m.group(1))
    return pid, 0


# ---------------------------------------------------------------- choice lists
def listed(s, pid):
    m = re.match(r'^(\d+): (.*)$', s)
    if not m:
        return None
    items = [x.strip() for x in m.group(2).split(', ')]
    if len(items) != int(m.group(1)):
        err('choice count %s: header %s, listed %d' % (pid, m.group(1), len(items)))
    return items


tempo = {}
for line in section('### Tempo divisions', '### `sN_scale` choice list').split('\n'):
    m = re.match(r'^\| `(sN_size_sync|sN_density_sync|lfoN_rate_sync|rec_length_sync)` \| (.*?) \| (\d+) \(', line)
    if m:
        items = m.group(2).split(', ')
        for k, it in enumerate(items):
            if not it.startswith('%d: ' % k):
                err('tempo list %s index %d: %r' % (m.group(1), k, it))
        tempo[m.group(1)] = ([re.sub(r'^\d+: ', '', x) for x in items], int(m.group(3)))
if len(tempo) != 4:
    err('tempo division table rows %d != 4' % len(tempo))

scale_map = {}
for line in section('### `sN_scale` choice list', '### `modM_source` choice list').split('\n'):
    if not re.match(r'^\| \d+ \|', line):
        continue
    c = cells(line)
    if len(c) % 2:
        fatal('scale grid row: %r' % line)
    for k in range(0, len(c), 2):
        idx = int(c[k])
        if idx in scale_map:
            err('scale index %d twice' % idx)
        scale_map[idx] = c[k + 1]
if sorted(scale_map) != list(range(32)):
    err('scale grid indices %s' % sorted(scale_map))
scale_names = [scale_map.get(k, '?') for k in range(32)]

sources = []   # (name, class)
SRC_CLASS = {'—': 'none', 'per-voice': 'per-voice', 'global': 'global'}
for line in section('### `modM_source` choice list', '## Modulation destination table').split('\n'):
    if not re.match(r'^\| \d+ \|', line):
        continue
    c = cells(line)
    if len(c) != 3:
        fatal('source row: %r' % line)
    if int(c[0]) != len(sources):
        err('source index %s (expected %d)' % (c[0], len(sources)))
    cls = 'lfo-mode' if c[2].startswith('per-voice or global') else SRC_CLASS.get(c[2])
    if cls is None:
        err('source %s: unknown class %r' % (c[1], c[2]))
    sources.append((c[1], cls))

dest_map = {}
for line in section('## Modulation destination table', '## Plugin state').split('\n'):
    if not re.match(r'^\| \d+ \|', line):
        continue
    c = cells(line)
    if len(c) % 4:
        fatal('dest grid row: %r' % line)
    for k in range(0, len(c), 4):
        idx = int(c[k])
        did = c[k + 1].strip('`')
        nm = c[k + 2]
        cl = c[k + 3]
        if idx in dest_map:
            err('dest index %d twice' % idx)
        dest_map[idx] = (did, 'None' if did == 'None' else nm, '-' if did == 'None' else cl)
if sorted(dest_map) != list(range(len(dest_map))):
    err('dest grid indices not contiguous')
dests = [dest_map[k] for k in sorted(dest_map)]
dest_ids = [d[0] for d in dests]

# ---------------------------------------------------------------- build rows
def num(s):
    return float(s.strip().split()[0])


out = []
n_centre = n_rawskew = n_interval1 = n_negstart = 0
dest_cover = {}
for idx, (pid, ptype, name) in enumerate(idlist):
    tpl, n = template_for(pid)
    if tpl not in rows:
        err('no spec row for %s (%s)' % (pid, tpl))
        continue
    r = rows[tpl]
    base = r['type'].split()[0]
    interval = 1.0 if '(interval 1)' in r['type'] else 0.0
    if base != ptype:
        err('type %s: Full ID list %s, group table %s' % (pid, ptype, r['type']))
    start = end = default = skew = centre = ''
    choices = ''
    if base == 'Float':
        m = re.match(r'^(-?[\d.]+) … (-?[\d.]+)', r['range'])
        if not m:
            err('range %s: %r' % (pid, r['range']))
            continue
        start, end = float(m.group(1)), float(m.group(2))
        default = num(r['default'])
        sk = r['skew']
        if sk.startswith('linear'):
            if sk != 'linear (1.0)':
                err('skew %s: %r' % (pid, sk))
            skew = 1.0
        elif sk.startswith('log-like'):
            centre = float(re.search(r'setSkewForCentre \(([\d.]+)\)', sk).group(1))
            skew = float(re.search(r'skew ([\d.]+)', sk).group(1))
            recomputed = math.log(0.5) / math.log((centre - start) / (end - start))
            if abs(recomputed - skew) > 5e-5:
                err('setSkewForCentre %s: centre %r gives %.6f, spec prints %r' % (pid, centre, recomputed, skew))
            n_centre += 1
        else:
            skew = float(sk)
            if skew == 1.0:
                err('raw skew 1.0 on %s (write it as linear)' % pid)
            n_rawskew += 1
        if interval == 1.0:
            n_interval1 += 1
            if start < 0:
                err('interval-1 row %s has a negative start' % pid)
        if start < 0:
            n_negstart += 1
        if not (start <= default <= end) or not (start < end):
            err('range / default %s: %r … %r, default %r' % (pid, start, end, default))
    elif base == 'Choice':
        if tpl == 'sN_scale':
            items = scale_names
        elif tpl == 'modM_dest':
            items = ['None'] + [d[1] for d in dests[1:]]
        else:
            items = listed(r['range'], pid)
            if items is None:
                err('choice list %s: %r' % (pid, r['range']))
                continue
            if tpl in tempo and tempo[tpl][0] != items:
                err('tempo list %s differs between the group table and the Tempo divisions table' % pid)
        if tpl == 'modM_source' and items != [s[0] for s in sources]:
            err('modM_source list differs from the source table')
        m = re.match(r'^(\d+)', r['default'])
        if not m:
            err('choice default %s: %r' % (pid, r['default']))
            continue
        default = int(m.group(1))
        if tpl in tempo and tempo[tpl][1] != default:
            err('tempo default %s: group %d, table %d' % (pid, default, tempo[tpl][1]))
        if len(items) < 2:
            err('single-choice %s' % pid)
        if not 0 <= default < len(items):
            err('default index %s: %d of %d' % (pid, default, len(items)))
        if any('|' in x or '\t' in x for x in items):
            err('choice string with | or tab in %s' % pid)
        choices = '|'.join(items)
    else:
        d = r['default'].strip()
        if d == 'On':
            default = 1
        elif d == 'Off':
            default = 0
        elif tpl == 'sN_enable' and re.match(r'^s1 On; s2.s4 Off$', d):
            default = 1 if n == 1 else 0
        else:
            err('bool default %s: %r' % (pid, d))
            continue
    # Mod-dest column, cross-checked against the dest table.
    dcol = r['dest'].strip()
    if re.match(r'^\d+$', dcol):
        if base != 'Float':
            err('Mod dest on a %s row (%s)' % (base, pid))
        d = int(dcol)
        if tpl.startswith('sN_'):
            d += 28 * (n - 1)
        elif tpl.startswith('lfoN_'):
            d += n - 1
        if not 0 < d < len(dest_ids) or dest_ids[d] != pid:
            err('Mod dest column %s -> %d (%s)' % (pid, d, dest_ids[d] if 0 <= d < len(dest_ids) else 'out of range'))
        dest_cover[d] = dest_cover.get(d, 0) + 1
    elif not (dcol == '—' or dcol.startswith('alias →')):
        err('Mod dest column %s: %r' % (pid, dcol))
    out.append([idx, pid, name, base, start, end, interval if base == 'Float' else '', default, skew, centre, choices])

# ---------------------------------------------------------------- checks
types = [t for _, t, _ in idlist]
counts = (types.count('Float'), types.count('Choice'), types.count('Bool'))
if len(idlist) != 261 or counts != (176, 70, 15):
    err('counts: %d params, %d Float / %d Choice / %d Bool (expected 261, 176 / 70 / 15)' % ((len(idlist),) + counts))
if len(out) != len(idlist):
    err('rows built %d of %d' % (len(out), len(idlist)))
if len(sources) != 18:
    err('sources %d != 18' % len(sources))
if len(dests) != 126 or not dests or dests[0][0] != 'None':
    err('dest table size %d (expected 126, None at 0)' % len(dests))

mism = 0
for k, (did, nm, cl) in enumerate(dests[1:], start=1):
    if type_of.get(did) != 'Float':
        err('dest %d %s is %s, not a Float' % (k, did, type_of.get(did)))
    if name_of.get(did) != nm:
        mism += 1
        err('dest %d %s host name %r != Full ID list %r' % (k, did, nm, name_of.get(did)))
    if cl not in ('G', 'V', 'G/V'):
        err('dest %d %s class %r' % (k, did, cl))
    if dest_cover.get(k, 0) != 1:
        err('dest %d %s is named by %d Mod-dest cells (expected 1)' % (k, did, dest_cover.get(k, 0)))

PARENT24 = ['level', 'pan', 'position', 'spray', 'size', 'size_rand', 'density', 'jitter', 'pitch', 'pitch_rand',
            'reverse_prob', 'window', 'window_skew', 'stereo_spread', 'gain_rand', 'filter_cutoff', 'filter_rand',
            'filter_res', 'drive', 'crush', 'crush_prob', 'ring_freq', 'ring_rand', 'ring_mix']
STREAM28 = PARENT24 + ['scan_rate', 'loop_start', 'loop_end', 'key_track']
formula = ['None', 'output_gain']
for N in range(1, 5):
    if len(formula) != 2 + 28 * (N - 1):
        err('formula base for s%d' % N)
    formula += ['s%d_%s' % (N, s) for s in STREAM28]
formula += ['feedback', 'fb_low_cut', 'fb_high_cut', 'fb_saturation', 'fb_diffusion', 'reverb_mix', 'reverb_size', 'reverb_damp']
formula += ['lfo%d_rate' % N for N in range(1, 5)]
if formula != dest_ids:
    bad = [k for k in range(max(len(formula), len(dest_ids)))
           if k >= len(formula) or k >= len(dest_ids) or formula[k] != dest_ids[k]]
    err('dest table != formula b = 2 + 28*(N-1) at indices %s' % bad[:8])
if len(dest_ids) > 122 and (dest_ids[114] != 'feedback' or dest_ids[122] != 'lfo1_rate'):
    err('dest anchors: 114 %s, 122 %s' % (dest_ids[114], dest_ids[122]))

for label, got, want in (('setSkewForCentre', n_centre, 19), ('raw skew', n_rawskew, 14),
                         ('interval-1', n_interval1, 6), ('negative-start', n_negstart, 43)):
    if got != want:
        err('%s rows %d != %d' % (label, got, want))

if len(set(o[2] for o in out)) != len(out):
    err('host names not unique')
if len(set(o[1] for o in out)) != len(out):
    err('IDs not unique')
on = [o[1] for o in out if o[3] == 'Bool' and o[7] == 1]
if on != ['s1_enable']:
    err('bools defaulting On: %s (expected only s1_enable)' % on)
for o in out:
    if '\t' in o[2] or '|' in o[2]:
        err('host name with tab or | (%s)' % o[1])

if errors:
    for e in errors:
        print('FAIL:', e)
    print('FAIL: %d cross-check failure(s); nothing written' % len(errors))
    sys.exit(1)


# ---------------------------------------------------------------- write (LF, UTF-8)
def fmt(v):
    if isinstance(v, float):
        return repr(v)
    return str(v)


HEADER = '# GENERATED by extract_spec.py from parameter-spec.md (LOCKED) - do not edit.\n'
os.makedirs(args.out, exist_ok=True)
with open(os.path.join(args.out, 'params.tsv'), 'w', encoding='utf-8', newline='\n') as f:
    f.write(HEADER)
    f.write('idx\tid\tname\ttype\tstart\tend\tinterval\tdefault\tskew\tcentre\tchoices\n')
    for o in out:
        f.write('\t'.join(fmt(v) for v in o) + '\n')
with open(os.path.join(args.out, 'mod-dests.tsv'), 'w', encoding='utf-8', newline='\n') as f:
    f.write(HEADER)
    f.write('idx\tid\tname\tclass\n')
    for k, (did, nm, cl) in enumerate(dests):
        f.write('%d\t%s\t%s\t%s\n' % (k, did, nm, cl))
with open(os.path.join(args.out, 'mod-sources.tsv'), 'w', encoding='utf-8', newline='\n') as f:
    f.write(HEADER)
    f.write('idx\tname\tclass\n')
    for k, (nm, cl) in enumerate(sources):
        f.write('%d\t%s\t%s\n' % (k, nm, cl))

print('OK: 261 params (176 Float / 70 Choice / 15 Bool), 126 dests, 18 sources')
