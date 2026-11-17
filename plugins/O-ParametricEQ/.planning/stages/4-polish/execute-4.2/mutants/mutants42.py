#!/usr/bin/env python3
"""EXECUTE 4.2 mutation run (PLAN-4.2 task 6). Modelled on verify-4.1/mutants41v.py.

harness scorer: scratch copy of Source/ + main.cpp, built with execute-4.1's
build-scratch-harness.sh (no write to build/ or the repo), run with --rows42
(FC1, FC2, FP5m only). CAUGHT when every expected target row FAILs.
node scorer: scratch root (plugin copied under plugins/, scripts / modules /
node_modules symlinked), PN1 run from the scratch copy with NO workaround:
since the W1 repoRoot token, the scratch copy serves its own page.
"""
import json, os, re, shutil, subprocess, sys, tempfile

REPO = '/Users/taylorbrook/Dev/VST-development'
PLUGIN = REPO + '/plugins/O-ParametricEQ'
EX41 = PLUGIN + '/.planning/stages/4-polish/execute-4.1/mutants'
SCR = os.path.dirname(os.path.abspath(__file__))
TMP = os.environ.get('MUT_TMP', SCR)
FH = 'Source/FactoryPresets.h'
HTML = 'Source/ui/public/index.html'

H = [
    ('control-h', None, None, None, ['FC1', 'FC2', 'FP5m'], 'no edit'),
    ('V9', FH, '        if (placement != stereo) out.push_back ({ p + "placement", (float) placement });\n', '',
     ['FC2', 'FP5m'], 'placement dropped from every def (Mono Low End / Side Air / L-R Pair Match lose M/S, L/R)'),
    ('V10', FH, '          addBand (v, 1, lowCut, 28.0f, 0.0f, 1.0f, s96); }', '          addBand (v, 1, lowCut, 28.0f, 0.0f, 1.0f, s6); }',
     ['FC2'], 'Rumble Cut slope 96 -> 6 dB/oct'),
    ('V14', FH, '          addBand (v, 2, bell,   62.0f,    3.0f, 1.4f, s12);', '          addBand (v, 2, bell,   63.0f,    3.0f, 1.4f, s12);',
     ['FC1'], 'Kick Tighten b02 62 -> 63 Hz (one-value drift, no feature change)'),
]

NODE = [
    ('control-pn1', None, None, None, 'no edit'),
    ('V12', HTML, '  .hgroup .cap[data-i18n="analyzer"] { display: none; }\n', '',
     '800 px header keeps the Analyzer caption; scratch root, no workaround'),
]

ROW = re.compile(r'^\s+\[(PASS|FAIL)\] (\S+)\s')


def apply(root, path, old, new):
    if not path:
        return None
    t = os.path.join(root, path)
    text = open(t, encoding='utf-8', newline='').read()
    n = text.count(old)
    if n != 1:
        return 'edit applies %d times' % n
    open(t, 'w', encoding='utf-8', newline='').write(text.replace(old, new))
    return None


def run_h(m):
    mid, path, old, new, rows, desc = m
    tmp = tempfile.mkdtemp(prefix='opeq-m42-%s-' % mid, dir=TMP)
    shutil.copytree(PLUGIN + '/Source', tmp + '/Source')
    os.makedirs(tmp + '/tests/render-harness')
    shutil.copy(PLUGIN + '/tests/render-harness/main.cpp', tmp + '/tests/render-harness/main.cpp')
    err = apply(tmp, path, old, new)
    if err:
        return {'id': mid, 'desc': desc, 'error': err}
    env = dict(os.environ, SRC_DIR=tmp + '/Source', MAIN=tmp + '/tests/render-harness/main.cpp', OUT_DIR=tmp + '/out')
    b = subprocess.run(['bash', EX41 + '/build-scratch-harness.sh'], env=env, capture_output=True, text=True)
    if b.returncode != 0:
        return {'id': mid, 'desc': desc, 'error': 'build failed: ' + (b.stdout + b.stderr)[-600:]}
    r = subprocess.run([tmp + '/out/harness', '--rows42'], cwd=PLUGIN, capture_output=True, text=True, timeout=1200)
    got = {}
    for line in r.stdout.splitlines():
        mm = ROW.match(line)
        if mm:
            got[mm.group(2)] = (mm.group(1), line.strip()[:600])
    shutil.rmtree(tmp, ignore_errors=True)
    fails = [k for k, v in got.items() if v[0] == 'FAIL']
    if mid.startswith('control'):
        ok = all(got.get(k, ('MISSING',))[0] == 'PASS' for k in rows) and not fails
        verdict = 'control PASS' if ok else 'control FAIL'
    else:
        caught = [k for k in rows if got.get(k, ('MISSING',))[0] == 'FAIL']
        verdict = 'CAUGHT by ' + ','.join(caught) if len(caught) == len(rows) else \
                  ('PARTIAL ' + ','.join(caught) if caught else 'SURVIVED')
    return {'id': mid, 'desc': desc, 'targets': rows, 'verdict': verdict, 'fails': fails,
            'evidence': {k: got[k][1] for k in set(rows) | set(fails) if k in got}}


def run_n(m):
    mid, path, old, new, desc = m
    tmp = tempfile.mkdtemp(prefix='opeq-m42n-%s-' % mid, dir=TMP)
    for d in ('scripts', 'modules', 'node_modules'):
        if os.path.exists(REPO + '/' + d):
            os.symlink(REPO + '/' + d, tmp + '/' + d)
    dst = tmp + '/plugins/O-ParametricEQ'
    os.makedirs(tmp + '/plugins')
    shutil.copytree(PLUGIN, dst, ignore=shutil.ignore_patterns('.planning', 'build*'))
    err = apply(dst, path, old, new)
    if err:
        return {'id': mid, 'desc': desc, 'error': err}
    r = subprocess.run(['node', dst + '/tests/preset-name-fit/preset_name_fit.js'], cwd=tmp, capture_output=True, text=True, timeout=600)
    out = r.stdout + r.stderr
    fails = [l.strip()[:300] for l in out.splitlines() if 'FAIL' in l][:6]
    shutil.rmtree(tmp, ignore_errors=True)
    if mid.startswith('control'):
        verdict = 'control PASS' if r.returncode == 0 and not fails else 'control FAIL rc %d' % r.returncode
    else:
        verdict = 'CAUGHT by PN1' if r.returncode == 1 and fails else 'SURVIVED rc %d' % r.returncode
    return {'id': mid, 'desc': desc, 'verdict': verdict, 'rc': r.returncode, 'fails': fails, 'pn1': out}


def main():
    only = set(sys.argv[1:])
    res = []
    for m in H:
        if not only or m[0] in only:
            x = run_h(m); res.append(x); print('%-12s %s' % (x['id'], x.get('error') or x['verdict']), flush=True)
    for m in NODE:
        if not only or m[0] in only:
            x = run_n(m); res.append(x); print('%-12s %s' % (x['id'], x.get('error') or x['verdict']), flush=True)
    json.dump(res, open(SCR + '/mutants42-results.json', 'w'), indent=1, ensure_ascii=False)


if __name__ == '__main__':
    main()
