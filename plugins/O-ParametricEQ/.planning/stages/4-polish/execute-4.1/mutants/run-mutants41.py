#!/usr/bin/env python3
"""PLAN-4.1 task 27, harness half: mutants M4 - M14, one scratch build each,
scored against the named render-harness row (run with --rows41). The page
mutants M1 - M3 are scored by the editor bench (mutants41-page-results.json).

Each mutant copies Source/ and tests/render-harness/main.cpp to a scratch
dir, applies ONE text edit (asserted to apply exactly once), builds with
build-scratch-harness.sh and runs `harness --rows41` (the harness re-execs
under its own CFFIXED_USER_HOME temp home, so no mutant touches ~/Library).
A control (no edit) must PASS every scored row.
"""
import json, os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PLUGIN = '/Users/taylorbrook/Dev/VST-development/plugins/O-ParametricEQ'
SRC = os.path.join(PLUGIN, 'Source')
MAIN = os.path.join(PLUGIN, 'tests', 'render-harness', 'main.cpp')

MUTANTS = [
    ('control', None, None, None, ['FP1', 'FP1o', 'FP2', 'FP3', 'FP4', 'HT1', 'G10d0'], 'no edit'),
    ('M4', 'Source/FactoryPresets.h',
     'd.parameters[v.id] = rp->convertTo0to1 (v.value);',
     'd.parameters[v.id] = v.id.endsWith ("_freq") ? (v.value - 10.0f) / (30000.0f - 10.0f) : rp->convertTo0to1 (v.value);',
     ['FP1'], 'freq def hand-normalised as (v - min) / (max - min)'),
    ('M5', 'Source/FactoryPresets.h',
     'd.parameters[v.id] = rp->convertTo0to1 (v.value);',
     'd.parameters[v.id] = (v.id.endsWith ("_type") || v.id.endsWith ("_slope")) ? v.value / 7.0f : (v.id.endsWith ("_placement") ? v.value / 4.0f : rp->convertTo0to1 (v.value));',
     ['FP1'], 'choice def as index / (n - 1) without the range'),
    ('M6', 'Source/FactoryPresets.h',
     'addBand (v, 1, highShelf, 12000.0f, 3.0f, 1.0f, s12); }',
     'addBand (v, 1, highShelf, 12000.0f, 3.0f, 1.0f, s12); v.push_back ({ "output_gain", 3.0f }); }',
     ['FP1o'], 'one def (Air Shelf) names output_gain'),
    ('M7', 'Source/PluginProcessor.cpp',
     'const bool current = stampFile.existsAsFile() && stampFile.loadFileAsString().trim() == stamp;',
     'const bool current = dir.getChildFile (".factory-version").loadFileAsString().trim() == JucePlugin_VersionString;',
     ['FP2'], 'initFactoryBank sentinel-only (no hash compare)'),
    ('M8', 'Source/PluginProcessor.cpp',
     '''        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
            f.deleteFile();
''',
     '',
     ['FP2'], 'no orphan delete'),
    ('M9', 'Source/PluginProcessor.cpp',
     'for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.json"))',
     'for (const auto& f : dir.getParentDirectory().findChildFiles (juce::File::findFiles, true, "*.json"))',
     ['FP2'], 'the delete recurses into User/'),
    ('M10', 'Source/PresetSaveGuard.h',
     'if (target.isAChildOf (pm.getFactoryPresetsDirectory()))',
     'if (false && target.isAChildOf (pm.getFactoryPresetsDirectory()))',
     ['FP3'], 'PresetSaveGuard factory check off'),
    ('M11', 'Source/PluginProcessor.cpp',
     'presetManager.setValidatedApply (true);',
     'presetManager.setValidatedApply (false);',
     ['FP4'], 'setValidatedApply (false)'),
    ('M12', 'Source/ParamText.h',
     'return splitNumber (text, v, unit) ? (float) v : defaultValue;',
     'return splitNumber (text, v, unit) ? (float) v : 0.0f;',
     ['HT1'], 'parser returns 0 on garbage (dB / Q / %; all three sites)'),
    ('M13', 'Source/ParamText.h',
     'const double k = v / 1000.0;',
     'const double k = v;',
     ['HT1'], 'freqToText drops the k scale'),
    ('M14', 'tests/render-harness/main.cpp',
     'return (a - c) <= 24.0 * (d - c) * 1.15;',
     'return a <= 24.0 * d * 1.15;',
     ['G10d0'], 'G10d gross formula'),
]

ROW = re.compile(r'^\s+\[(PASS|FAIL)\] (\S+)\s')


def run(m):
    mid, path, old, new, rows, desc = m
    tmp = tempfile.mkdtemp(prefix='opeq-mut41-%s-' % mid, dir=os.environ.get('TMPDIR'))
    src = os.path.join(tmp, 'Source')
    shutil.copytree(SRC, src)
    main = os.path.join(tmp, 'main.cpp')
    shutil.copy(MAIN, main)
    if path:
        target = main if path.startswith('tests/') else os.path.join(tmp, path)
        text = open(target, encoding='utf-8').read()
        n = text.count(old)
        # M12 edits all three identical sites; everything else exactly one.
        if (mid == 'M12' and n != 3) or (mid != 'M12' and n != 1):
            return {'id': mid, 'description': desc, 'rows': rows, 'error': 'edit applies %d times' % n}
        open(target, 'w', encoding='utf-8').write(text.replace(old, new))
    env = dict(os.environ, SRC_DIR=src, MAIN=main, OUT_DIR=os.path.join(tmp, 'out'))
    b = subprocess.run(['bash', os.path.join(HERE, 'build-scratch-harness.sh')], env=env, capture_output=True, text=True)
    if b.returncode != 0:
        return {'id': mid, 'description': desc, 'rows': rows, 'error': 'build failed: ' + (b.stdout + b.stderr)[-800:]}
    r = subprocess.run([os.path.join(tmp, 'out', 'harness'), '--rows41'], cwd=PLUGIN, capture_output=True, text=True, timeout=900)
    got = {}
    for line in r.stdout.splitlines():
        mm = ROW.match(line)
        if mm:
            got[mm.group(2)] = (mm.group(1), line.strip()[:400])
    shutil.rmtree(tmp, ignore_errors=True)
    scored = {k: got.get(k, ('MISSING', ''))[0] for k in rows}
    if mid == 'control':
        caught = None
        ok = all(v == 'PASS' for v in scored.values())
    else:
        caught = any(v == 'FAIL' for v in scored.values())
        ok = caught
    evidence = '; '.join(got[k][1] for k in rows if k in got)
    return {'id': mid, 'description': desc, 'rows': rows, 'scored': scored, 'caught': caught, 'ok': ok,
            'evidence': evidence, 'allRows': {k: v[0] for k, v in got.items()}}


def main():
    only = set(sys.argv[1:])
    results = []
    for m in MUTANTS:
        if only and m[0] not in only:
            continue
        res = run(m)
        results.append(res)
        print('%-8s %-55s %s' % (res['id'], res['description'][:55],
                                 res.get('error') or ('control PASS' if res['id'] == 'control' and res['ok'] else
                                                      'control FAIL' if res['id'] == 'control' else
                                                      'CAUGHT ' + json.dumps(res['scored']) if res['caught'] else 'SURVIVED ' + json.dumps(res['scored']))),
              flush=True)
    out = os.path.join(HERE, 'mutants41-results.json')
    json.dump(results, open(out, 'w'), indent=1, ensure_ascii=False)
    print('wrote', out)


if __name__ == '__main__':
    main()
