#!/usr/bin/env python3
"""O-Granulator golden extractor — derives the Stage 1 parameter goldens from the
LOCKED contract, never from the code (a code-derived golden is self-referential).

Inputs:  .planning/parameter-spec.md   (group tables, choice lists, dest table, Full ID list)
         .planning/mockups/v2-ui.yaml  (host `name:` of the 112 mod destinations)
         + the P1 naming convention below for the 108 params without a YAML name
Outputs: tests/golden/params.tsv     idx id name type start end default skew centre choices
         tests/golden/mod-dests.tsv  idx id name

Run from anywhere:  python3 plugins/O-Granulator/tests/golden/extract_spec.py
Exit 1 if any cross-check fails (counts, ID order, dest table vs Mod dest column).
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
PLUGIN = os.path.normpath(os.path.join(HERE, '..', '..'))
SPEC = os.path.join(PLUGIN, '.planning', 'parameter-spec.md')
YAML = os.path.join(PLUGIN, '.planning', 'mockups', 'v2-ui.yaml')

spec = open(SPEC, encoding='utf-8').read().replace('−', '-')
yaml = open(YAML, encoding='utf-8').read()

def fail(msg):
    print('FAIL:', msg); sys.exit(1)

# ---------------------------------------------------------------- ID list
idblock = spec.split('## Full ID list (registration order)')[1].split('```')[1]
idlist = []
for line in idblock.strip().split('\n'):
    k, i, t = line.split()
    if int(k) != len(idlist): fail('ID list index %s' % k)
    idlist.append((i, t))

# ---------------------------------------------------------------- group tables
rows = {}   # template id -> dict
for line in spec.split('\n'):
    m = re.match(r'^\| `([a-zA-Z0-9_]+)` \| (Float|Choice|Bool) \| (.*?) \| (.*?) \| (.*?) \| (.*?) \| (.*) \|$', line)
    if m:
        rows[m.group(1)] = dict(type=m.group(2), range=m.group(3), default=m.group(4), skew=m.group(5), dest=m.group(6))

def template_for(pid):
    for pat, tpl in ((r'^s([1-4])_(.+)$', r'sN_\2'), (r'^lfo([1-4])_(.+)$', r'lfoN_\2'), (r'^macro([1-4])$', 'macroN'),
                     (r'^pitch_follow_s([1-4])$', 'pitch_follow_sN'), (r'^mod(\d+)_(.+)$', r'modM_\2')):
        m = re.match(pat, pid)
        if m: return re.sub(pat, tpl, pid), int(m.group(1))
    return pid, 0

# ---------------------------------------------------------------- choice lists
def listed(s):
    m = re.match(r'^(\d+): (.*)$', s)
    if not m: return None
    items = [x.strip() for x in m.group(2).split(', ')]
    if len(items) != int(m.group(1)): fail('choice count %s' % s)
    return items

tempo = {}
for line in spec.split('\n'):
    m = re.match(r'^\| `(sN_size_sync|sN_density_sync|lfoN_rate_sync)` \| (.*?) \| (\d+) \(', line)
    if m:
        tempo[m.group(1)] = [re.sub(r'^\d+: ', '', x) for x in m.group(2).split(', ')]

scale_names = []
for line in spec.split('## `sN_scale` choice list')[1].split('## Modulation destination')[0].split('\n'):
    m = re.match(r'^\| (\d+) \| (.+?) \| (.+?) \| (.+?) \|$', line)
    if m:
        if int(m.group(1)) != len(scale_names): fail('scale index')
        scale_names.append(m.group(2).strip())

dest_ids = {}
for line in spec.split('## Modulation destination table')[1].split('## Non-parameter state')[0].split('\n'):
    for m in re.finditer(r'\| (\d+) \| `([a-zA-Z0-9_]+)` ', line):
        dest_ids[int(m.group(1))] = m.group(2)
dests = [dest_ids[k] for k in range(len(dest_ids))]

# ---------------------------------------------------------------- host names
yaml_names = {}
for b in re.split(r'\n  - id: ', yaml)[1:]:
    pid = b.split('\n')[0].strip()
    nm = re.search(r'\n    name: "([^"]*)"', b)
    if nm and pid not in yaml_names: yaml_names[pid] = nm.group(1)

# P1 convention (plan-phase decision) for params without a YAML `name:`.
STREAM_P1 = {'enable': 'Enable', 'mute': 'Mute', 'solo': 'Solo', 'size_sync': 'Size Sync',
             'density_sync': 'Density Sync', 'scale': 'Scale', 'onset_trigger': 'Onset Trigger', 'filter_type': 'Filter Type'}
LFO_P1 = {'shape': 'Shape', 'rate_sync': 'Rate Sync', 'phase': 'Phase'}
GLOBAL_P1 = {'freeze': 'Freeze', 'sync_mode': 'Sync Mode', 'scale_root': 'Scale Root', 'grain_limit': 'Grain Limit',
             'env_attack': 'Env Attack', 'env_release': 'Env Release', 'env_gain': 'Env Gain', 'onset_threshold': 'Onset Threshold'}
MOD_P1 = {'source': 'Source', 'dest': 'Dest', 'depth': 'Depth'}

def host_name(pid):
    if pid in yaml_names: return yaml_names[pid], 'yaml'
    m = re.match(r'^s([1-4])_(.+)$', pid)
    if m and m.group(2) in STREAM_P1: return 'S%s · %s' % (m.group(1), STREAM_P1[m.group(2)]), 'p1'
    m = re.match(r'^lfo([1-4])_(.+)$', pid)
    if m and m.group(2) in LFO_P1: return 'LFO %s · %s' % (m.group(1), LFO_P1[m.group(2)]), 'p1'
    m = re.match(r'^mod(\d+)_(.+)$', pid)
    if m: return 'Mod %s · %s' % (m.group(1), MOD_P1[m.group(2)]), 'p1'
    m = re.match(r'^macro([1-4])$', pid)
    if m: return 'Macro %s' % m.group(1), 'p1'
    m = re.match(r'^pitch_follow_s([1-4])$', pid)
    if m: return 'Pitch Follow S%s' % m.group(1), 'p1'
    if pid in GLOBAL_P1: return GLOBAL_P1[pid], 'p1'
    fail('no host name for ' + pid)

# ---------------------------------------------------------------- build rows
def num(s):
    return float(s.strip().split()[0])

out, p1 = [], []
for idx, (pid, ptype) in enumerate(idlist):
    tpl, n = template_for(pid)
    if tpl not in rows: fail('no spec row for %s (%s)' % (pid, tpl))
    r = rows[tpl]
    if r['type'] != ptype: fail('type %s' % pid)
    name, src = host_name(pid)
    if src == 'p1': p1.append((pid, name))
    start = end = default = skew = centre = ''
    choices = ''
    if ptype == 'Float':
        m = re.match(r'^(-?[\d.]+) … (-?[\d.]+)', r['range'])
        start, end = float(m.group(1)), float(m.group(2))
        default = num(r['default'])
        sk = r['skew']
        if sk.startswith('linear'): skew = 1.0
        elif sk.startswith('log-like'):
            centre = float(re.search(r'setSkewForCentre \(([\d.]+)\)', sk).group(1))
            skew = float(re.search(r'skew ([\d.]+)', sk).group(1))
        elif sk == '—': skew = 1.0
        else: skew = float(sk)
    elif ptype == 'Choice':
        if tpl == 'sN_scale': items = scale_names
        elif tpl == 'modM_dest': items = None
        elif tpl in tempo: items = tempo[tpl]
        else: items = listed(r['range'])
        if items is None:   # modM_dest
            items = ['None'] + [host_name(d)[0] for d in dests[1:]]
        choices = '|'.join(items)
        default = int(re.match(r'^(\d+)', r['default']).group(1))
        if not 0 <= default < len(items): fail('default index %s' % pid)
        if len(items) < 2: fail('single-choice %s' % pid)
    else:
        if tpl == 'sN_enable': default = 1 if n == 1 else 0
        else: default = 1 if r['default'].strip() == 'On' else 0
    # spec Mod-dest column (cross-checked against the dest table)
    dcol = r['dest'].strip()
    if re.match(r'^\d+$', dcol):
        d = int(dcol)
        if tpl.startswith('sN_'): d += 24 * (n - 1)
        elif tpl.startswith('lfoN_'): d += (n - 1)
        if dests[d] != pid: fail('Mod dest column %s -> %d (%s)' % (pid, d, dests[d]))
    out.append([idx, pid, name, ptype, start, end, default, skew, centre, choices])

def fmt(v):
    if isinstance(v, float): return repr(v)
    return str(v)

# ---------------------------------------------------------------- checks
types = [t for _, t in idlist]
counts = (types.count('Float'), types.count('Choice'), types.count('Bool'))
if len(idlist) != 220 or counts != (140, 59, 21): fail('counts %d %s' % (len(idlist), counts))
if len(dests) != 113 or dests[0] != 'None': fail('dest table size %d' % len(dests))
for d in dests[1:]:
    t = dict(idlist).get(d)
    if t != 'Float': fail('dest %s is %s' % (d, t))
if len(p1) != 108: fail('P1-named params %d != 108' % len(p1))
if len(set(o[2] for o in out)) != 220: fail('host names not unique')

with open(os.path.join(HERE, 'params.tsv'), 'w', encoding='utf-8', newline='\n') as f:
    f.write('# GENERATED by extract_spec.py from parameter-spec.md + v2-ui.yaml — do not edit.\n')
    f.write('idx\tid\tname\ttype\tstart\tend\tdefault\tskew\tcentre\tchoices\n')
    for o in out: f.write('\t'.join(fmt(v) for v in o) + '\n')
with open(os.path.join(HERE, 'mod-dests.tsv'), 'w', encoding='utf-8', newline='\n') as f:
    f.write('# GENERATED by extract_spec.py from parameter-spec.md + v2-ui.yaml — do not edit.\n')
    f.write('idx\tid\tname\n')
    for k, d in enumerate(dests): f.write('%d\t%s\t%s\n' % (k, d, 'None' if k == 0 else host_name(d)[0]))

print('OK: 220 params (140 Float / 59 Choice / 21 Bool), 113 dests, %d P1 names' % len(p1))
if '--p1' in sys.argv:
    for pid, nm in p1: print('  %-18s %s' % (pid, nm))
