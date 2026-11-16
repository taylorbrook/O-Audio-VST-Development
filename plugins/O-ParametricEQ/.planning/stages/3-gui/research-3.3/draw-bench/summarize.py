#!/usr/bin/env python3
"""research-3.3 draw-bench: per-case table across runs -> stdout + results/summary.json.
Usage: summarize.py results/rel-1.json results/cache-1.json ..."""
import json, sys, os

def pct(d, q):
    d = sorted(d)
    return d[min(len(d) - 1, int(len(d) * q))] if d else None

cases = {}
meta = {}
for path in sys.argv[1:]:
    run = os.path.splitext(os.path.basename(path))[0]
    rows = json.load(open(path))['rows'].get('draw33', {})
    for name, r in rows.items():
        if not isinstance(r, dict) or 'draw' not in r:
            continue
        c = cases.setdefault(name, {'runs': [], 'pooled': []})
        d = r['draw']
        c['runs'].append({'run': run, 'n': d['n'], 'median': d['median'], 'p99': d['p99'], 'max': d['max'],
                          'mean': d.get('mean'), 'sections': r.get('sections'),
                          'cache': r.get('cacheStats'), 'phases': r.get('phasesMeanMs') or None,
                          'frameIntervalMedian': (r.get('frameInterval') or {}).get('median'),
                          'eqSectionsDuringDrag': r.get('eqSectionsDuringDrag')})
        c['pooled'] += r.get('raw', [])
        meta.setdefault('geom', r.get('geom'))

out = {'geom': meta.get('geom'), 'cases': {}}
print('%-28s %5s %7s %7s %7s %7s  %s' % ('case', 'n', 'median', 'p99', 'max', 'mean', 'per-run median/p99'))
for name, c in cases.items():
    p = c['pooled']
    mean = sum(p) / len(p) if p else None
    row = {'n': len(p), 'median': pct(p, 0.5), 'p99': pct(p, 0.99), 'max': max(p) if p else None, 'mean': mean, 'runs': c['runs']}
    out['cases'][name] = row
    per = ', '.join('%s %g/%g' % (x['run'], round(x['median'], 1), round(x['p99'], 1)) for x in c['runs'])
    print('%-28s %5d %7.1f %7.1f %7.1f %7.2f  %s' % (name, len(p), row['median'], row['p99'], row['max'], mean, per))
    for x in c['runs']:
        if x['phases']:
            print('    phases %s: %s' % (x['run'], ', '.join('%s %.2f' % (k, v) for k, v in x['phases'].items())))
json.dump(out, open(os.path.join(os.path.dirname(sys.argv[1]), 'summary.json'), 'w'), indent=1)
