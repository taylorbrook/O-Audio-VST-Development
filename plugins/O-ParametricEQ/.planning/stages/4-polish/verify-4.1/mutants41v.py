#!/usr/bin/env python3
"""VERIFY 4.1 mutation run. New mutants only (none of execute's M1 - M14).

harness scorer: scratch copy of Source/ + main.cpp, built with execute-4.1's
build-scratch-harness.sh (no write to build/ or the repo), run with --rows41
(the harness re-execs under its own CFFIXED_USER_HOME). CAUGHT when any target
row FAILs. Targets [] = a probe (is there any row at all?): reported with every
FAIL row it produced.
node scorer: scratch copy of the plugin's Source/ui + tests, PN1 or
check-ui-labels on a scratch root.
"""
import json, os, re, shutil, subprocess, sys, tempfile

REPO = '/Users/taylorbrook/Dev/VST-development'
PLUGIN = REPO + '/plugins/O-ParametricEQ'
EX = PLUGIN + '/.planning/stages/4-polish/execute-4.1/mutants'
SCR = os.path.dirname(os.path.abspath(__file__))
FH = 'Source/FactoryPresets.h'
PT = 'Source/ParamText.h'
PP = 'Source/PluginProcessor.cpp'
PK = 'Source/editor/PeakHold.h'
HTML = 'Source/ui/public/index.html'

H = [
    ('control-h', None, None, None, ['FP1', 'FP1o', 'FP2', 'FP3', 'FP4', 'HT1', 'HT1g', 'M1j', 'G10d0'], 'no edit'),
    ('V1', PK, 'static constexpr double jitterMargin = 1.3;', 'static constexpr double jitterMargin = 1.0;',
     ['M1j'], 'PeakHold jitter margin 1.3 -> 1.0 (the pre-Taylor-call depth)'),
    ('V2', PT, '        if (std::abs (r) < 0.05)\n            return "0.0 dB";\n', '',
     ['HT1'], 'dbToText "-0.0" guard removed'),
    ('V3', PT, 'if (roundTo (v, 0) < 1000.0)', 'if (v < 1000.0)',
     ['HT1'], 'freqToText Hz/kHz threshold on the raw value (999.9998 -> "1000 Hz")'),
    ('V4', PT, '            if (s[i] == \'-\')\n                digits << "-";\n', '',
     ['HT1'], 'parser drops a leading minus'),
    ('V5', PT, '        if (unit.startsWith ("k"))\n            v *= 1000.0;\n', '',
     ['HT1'], 'textToFreq ignores k / kHz'),
    ('V6', PP, 'if (! current && dir.isDirectory())', 'if (dir.isDirectory())',
     ['FP2'], 'initFactoryBank deletes + rewrites on every construct (no stamp short-cut)'),
    ('V7', PP, '        dir.getChildFile (".factory-version").deleteFile();\n        stampFile.deleteFile();\n', '        stampFile.deleteFile();\n',
     ['FP2'], 'mismatch deletes the json but keeps .factory-version (manager init then a no-op: empty bank)'),
    ('V8', FH, '                mo << id << "=" << juce::String::toHexString ((int) bits) << ";";\n', '',
     ['FP2'], 'bankHash ignores parameter values (a re-voice keeps the stamp)'),
    ('V9', FH, '        if (placement != stereo) out.push_back ({ p + "placement", (float) placement });\n', '',
     [], 'PROBE: placement dropped from every def (Mono Low End / Side Air / L-R Pair Match lose M/S, L/R)'),
    ('V10', FH, '          addBand (v, 1, lowCut, 28.0f, 0.0f, 1.0f, s96); }', '          addBand (v, 1, lowCut, 28.0f, 0.0f, 1.0f, s6); }',
     [], 'PROBE: Rumble Cut slope 96 -> 6 dB/oct (preset content vs RESEARCH table)'),
    ('V11', PP, '    if (! current && dir.getChildFile (".factory-version").existsAsFile())\n        stampFile.replaceWithText (stamp);',
     '    if (! current)\n        stampFile.replaceWithText (stamp);',
     [], 'PROBE: stamp written even when the bank write failed'),
]

NODE = [
    ('control-pn1', None, None, None, 'pn1', 'no edit'),
    ('V12', HTML, '  .hgroup .cap[data-i18n="analyzer"] { display: none; }\n', '', 'pn1',
     '800 px header keeps the Analyzer caption (deviation 1 regressed)'),
    ('control-labels', None, None, None, 'labels', 'no edit'),
    ('V13', HTML, '#seg-analyzer button:nth-child(2) { width: 39px; }', '#seg-analyzer button:nth-child(2) { width: 35px; }', 'labels',
     'Pre segment back to 35 px (zh 处理前 36.8 px)'),
]

ROW = re.compile(r'^\s+\[(PASS|FAIL)\] (\S+)\s')


def apply(root, path, old, new, mid):
    if not path:
        return None
    t = os.path.join(root, path)
    text = open(t, encoding='utf-8').read()
    n = text.count(old)
    if n != 1:
        return 'edit applies %d times' % n
    open(t, 'w', encoding='utf-8').write(text.replace(old, new))
    return None


def run_h(m):
    mid, path, old, new, rows, desc = m
    tmp = tempfile.mkdtemp(prefix='opeq-v41-%s-' % mid, dir=SCR)
    shutil.copytree(PLUGIN + '/Source', tmp + '/Source')
    os.makedirs(tmp + '/tests/render-harness')
    shutil.copy(PLUGIN + '/tests/render-harness/main.cpp', tmp + '/tests/render-harness/main.cpp')
    err = apply(tmp, path, old, new, mid)
    if err:
        return {'id': mid, 'desc': desc, 'error': err}
    env = dict(os.environ, SRC_DIR=tmp + '/Source', MAIN=tmp + '/tests/render-harness/main.cpp', OUT_DIR=tmp + '/out')
    b = subprocess.run(['bash', EX + '/build-scratch-harness.sh'], env=env, capture_output=True, text=True)
    if b.returncode != 0:
        return {'id': mid, 'desc': desc, 'error': 'build failed: ' + (b.stdout + b.stderr)[-600:]}
    r = subprocess.run([tmp + '/out/harness', '--rows41'], cwd=PLUGIN, capture_output=True, text=True, timeout=1200)
    got = {}
    for line in r.stdout.splitlines():
        mm = ROW.match(line)
        if mm:
            got[mm.group(2)] = (mm.group(1), line.strip()[:500])
    shutil.rmtree(tmp, ignore_errors=True)
    fails = [k for k, v in got.items() if v[0] == 'FAIL']
    if mid.startswith('control'):
        ok = all(got.get(k, ('MISSING',))[0] == 'PASS' for k in rows) and not fails
        verdict = 'control PASS' if ok else 'control FAIL'
    elif rows:
        verdict = 'CAUGHT' if any(got.get(k, ('MISSING',))[0] == 'FAIL' for k in rows) else 'SURVIVED'
    else:
        verdict = 'PROBE caught by ' + ','.join(fails) if fails else 'PROBE no row'
    return {'id': mid, 'desc': desc, 'targets': rows, 'verdict': verdict, 'fails': fails,
            'evidence': {k: got[k][1] for k in (rows or fails) if k in got}}


def run_n(m):
    mid, path, old, new, kind, desc = m
    tmp = tempfile.mkdtemp(prefix='opeq-v41n-%s-' % mid, dir=SCR)
    for d in ('scripts', 'modules', 'node_modules'):
        if os.path.exists(REPO + '/' + d):
            os.symlink(REPO + '/' + d, tmp + '/' + d)
    if kind == 'pn1':
        dst = tmp + '/plugins/O-ParametricEQ'
        shutil.copytree(PLUGIN + '/Source', dst + '/Source')
        shutil.copytree(PLUGIN + '/tests', dst + '/tests')
        err = apply(dst, path, old, new, mid)
        if err:
            return {'id': mid, 'desc': desc, 'error': err}
        r = subprocess.run(['node', dst + '/tests/preset-name-fit/preset_name_fit.js'], cwd=REPO, capture_output=True, text=True)
    else:
        # check-ui-labels on a scratch repo root: scripts + modules + this plugin
        dst = tmp + '/plugins/O-ParametricEQ'
        os.makedirs(tmp + '/plugins')
        shutil.copytree(PLUGIN, dst, ignore=shutil.ignore_patterns('.planning', 'build*'))
        err = apply(dst, path, old, new, mid)
        if err:
            return {'id': mid, 'desc': desc, 'error': err}
        r = subprocess.run(['node', REPO + '/scripts/check-ui-labels.js', '--plugin', 'O-ParametricEQ', '--root', tmp],
                           cwd=REPO, capture_output=True, text=True, timeout=900)
    out = r.stdout + r.stderr
    fails = [l.strip()[:300] for l in out.splitlines() if 'FAIL' in l][:6]
    shutil.rmtree(tmp, ignore_errors=True)
    if mid.startswith('control'):
        verdict = 'control PASS' if r.returncode == 0 and not fails else 'control FAIL rc %d' % r.returncode
    else:
        verdict = 'CAUGHT' if r.returncode != 0 or fails else 'SURVIVED'
    return {'id': mid, 'desc': desc, 'verdict': verdict, 'rc': r.returncode, 'fails': fails,
            'pn1': out if kind == 'pn1' else None}


def main():
    only = set(sys.argv[1:])
    res = []
    for m in H:
        if not only or m[0] in only:
            x = run_h(m); res.append(x); print('%-15s %s' % (x['id'], x.get('error') or x['verdict']), flush=True)
    for m in NODE:
        if not only or m[0] in only:
            x = run_n(m); res.append(x); print('%-15s %s' % (x['id'], x.get('error') or x['verdict']), flush=True)
    json.dump(res, open(SCR + '/mutants41v-results.json', 'w'), indent=1, ensure_ascii=False)


if __name__ == '__main__':
    main()
