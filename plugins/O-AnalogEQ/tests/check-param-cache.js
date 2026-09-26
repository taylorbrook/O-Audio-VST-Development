#!/usr/bin/env node
'use strict';

/*
    check-param-cache.js — O-AnalogEQ's raw-parameter-pointer cache, STATICALLY.

    WHY THIS EXISTS
    ───────────────
    v1.5.3 (IN-07) replaced sixteen per-block
    `parameters.getRawParameterValue("…")->load()` calls in processBlock with sixteen
    pointers resolved once in the constructor. The change is output-identical by
    construction — and that is exactly the problem. Its ONE failure mode is a silent
    mapping typo:

        pHmfFreq = parameters.getRawParameterValue ("hmf_gain");   // swapped

    which compiles, runs, allocates nothing, and makes the plugin read the wrong
    parameter forever. The render harness would not catch it: G1 (bus layouts),
    G2 (output_gain ramp), G3 (band crossfade) and G4 (preset save guard) all still
    pass with two same-band reads exchanged, because each gate drives the parameter
    it measures and reads the audio back — a swap between two parameters that both
    move within one band leaves every one of those four verdicts intact.

    So the mapping is gated by NAME instead of by audio. Each cache member is `p` +
    the CamelCase of the parameter ID it holds, and this file asserts that over every
    declaration/assignment pair. A swap becomes a name mismatch and fails here.

    It also asserts the cache is COMPLETE against the parameter layout, and that no
    getRawParameterValue call survives outside the constructor — otherwise a
    parameter added later would quietly reintroduce the per-block lookup this change
    removed, and nothing would say so.

    USAGE
        node plugins/O-AnalogEQ/tests/check-param-cache.js
        node plugins/O-AnalogEQ/tests/check-param-cache.js --verbose

    Exit 0 = pass. Exit 1 = at least one check failed.
*/

const fs   = require('fs');
const path = require('path');

const REPO_ROOT = path.resolve(__dirname, '..', '..', '..');
const PLUGIN    = 'O-AnalogEQ';
const SRC       = path.join(REPO_ROOT, 'plugins', PLUGIN, 'Source');
const verbose   = process.argv.includes('--verbose');

let failed = 0;
let passes = 0;

function check(cond, msg, detail) {
    if (cond) { ++passes; console.log(`  PASS: ${msg}`); }
    else      { ++failed; console.log(`  FAIL: ${msg}`); if (detail) console.log(`        ${detail}`); }
    return !!cond;
}

// `lf_freq` -> `pLfFreq`,  `output_gain` -> `pOutputGain`,  `lmf_q` -> `pLmfQ`
function memberFor(id) {
    return 'p' + id.split('_')
        .map((w) => w.charAt(0).toUpperCase() + w.slice(1))
        .join('');
}

console.log(`check-param-cache — ${PLUGIN} raw parameter pointer cache\n`);

const hdrPath = path.join(SRC, 'PluginProcessor.h');
const cppPath = path.join(SRC, 'PluginProcessor.cpp');
for (const f of [hdrPath, cppPath]) {
    if (!fs.existsSync(f)) {
        console.error(`FATAL: ${f} not found`);
        process.exit(1);
    }
}
const hdr = fs.readFileSync(hdrPath, 'utf8');
const cpp = fs.readFileSync(cppPath, 'utf8');

// ── 1. the declarations ─────────────────────────────────────────────────────
const declared = [...hdr.matchAll(/std::atomic<float>\*\s+(p[A-Za-z0-9]+)\s*=\s*nullptr\s*;/g)]
    .map((m) => m[1]);

check(declared.length > 0,
    '[1] the header declares at least one std::atomic<float>* cache member — a gate over '
    + 'zero members is vacuous');

if (verbose) console.log(`   declared: ${declared.join(', ')}`);

// ── 2. the assignments ──────────────────────────────────────────────────────
const assigned = [...cpp.matchAll(
    /(p[A-Za-z0-9]+)\s*=\s*parameters\.getRawParameterValue\s*\(\s*"([a-z0-9_]+)"\s*\)\s*;/g)]
    .map((m) => ({ member: m[1], id: m[2] }));

check(assigned.length === declared.length,
    `[2] every declared cache member is assigned exactly once — ${declared.length} declared, `
    + `${assigned.length} assignments found`,
    (() => {
        const a = new Set(assigned.map((x) => x.member));
        const unassigned = declared.filter((d) => !a.has(d));
        const dupes = assigned.map((x) => x.member)
            .filter((m, i, arr) => arr.indexOf(m) !== i);
        const undeclared = assigned.map((x) => x.member).filter((m) => !declared.includes(m));
        return [
            unassigned.length ? `never assigned: ${unassigned.join(', ')}` : '',
            dupes.length      ? `assigned twice: ${[...new Set(dupes)].join(', ')}` : '',
            undeclared.length ? `assigned but not declared: ${undeclared.join(', ')}` : '',
        ].filter(Boolean).join('; ');
    })());

// ── 3. THE MAPPING. This is the check the whole file exists for. ────────────
const mismatched = assigned.filter(({ member, id }) => member !== memberFor(id));
check(mismatched.length === 0,
    `[3] every cache member holds the parameter its NAME claims — p + CamelCase(id), over all `
    + `${assigned.length} pairs`,
    mismatched.map(({ member, id }) =>
        `${member} = getRawParameterValue("${id}")  — expected ${memberFor(id)}`).join(' | '));

if (verbose && mismatched.length === 0) {
    for (const { member, id } of assigned) console.log(`   ${member.padEnd(12)} <- "${id}"`);
}

// ── 4. completeness against the parameter layout ────────────────────────────
//
// Scoped to createParameterLayout's own ParameterID list so a parameter added to the
// plugin without a cache entry is caught here rather than surfacing as a per-block
// lookup someone reintroduces by hand.
const layoutMatch = cpp.match(
    /ParameterLayout OuariconAnalogEQAudioProcessor::createParameterLayout\(\)\s*\{([\s\S]*?)\n\}/);
check(layoutMatch !== null,
    '[4] createParameterLayout() is locatable — without it the completeness check below '
    + 'would silently compare against nothing');

if (layoutMatch) {
    const layoutIds = [...new Set(
        [...layoutMatch[1].matchAll(/juce::ParameterID\s*\{\s*"([a-z0-9_]+)"/g)].map((m) => m[1]))];
    const cachedIds = new Set(assigned.map((x) => x.id));

    check(layoutIds.length > 0,
        `[4] the layout declares at least one ParameterID — found ${layoutIds.length}`);

    const uncached = layoutIds.filter((id) => !cachedIds.has(id));
    check(uncached.length === 0,
        `[4] every parameter in the layout has a cache entry — ${layoutIds.length} parameters, `
        + `${cachedIds.size} cached`,
        uncached.length ? `missing a cached pointer: ${uncached.join(', ')} `
                        + `(expected members ${uncached.map(memberFor).join(', ')})` : '');

    const phantom = [...cachedIds].filter((id) => !layoutIds.includes(id));
    check(phantom.length === 0,
        '[4] no cache entry names a parameter the layout does not declare — such a pointer is '
        + 'null at runtime and null-derefs on the first audio block',
        phantom.join(', '));
}

// ── 5. no lookup survives outside the constructor ───────────────────────────
//
// The point of IN-07 was to take this call off the audio thread's path. If a later
// parameter arrives and is read the old way, the waste comes back one line at a time
// and no gate above would notice — every assertion so far is about the cache, not
// about what else the file does.
const ctorMatch = cpp.match(
    /OuariconAnalogEQAudioProcessor::OuariconAnalogEQAudioProcessor\(\)[\s\S]*?\n\}/);
check(ctorMatch !== null, '[5] the constructor body is locatable');

if (ctorMatch) {
    const outsideCtor = cpp.replace(ctorMatch[0], '');
    const strays = outsideCtor.split('\n')
        .filter((line) => /getRawParameterValue/.test(line));
    const ctorLines = ctorMatch[0].split('\n').length;
    check(strays.length === 0,
        `[5] no getRawParameterValue call survives outside the constructor — the per-block `
        + `lookup IN-07 removed must not creep back in (constructor spans ${ctorLines} lines)`,
        strays.map((line) => line.trim()).slice(0, 5).join(' | '));
}

// ── 6. the null assertion covers every member ───────────────────────────────
const jassertMatch = cpp.match(/jassert\s*\(([^;]*?p[A-Za-z0-9]+[^;]*?)\)\s*;/s);
check(jassertMatch !== null,
    '[6] the constructor jasserts the resolved pointers — a layout typo otherwise surfaces '
    + 'as a null deref on the first audio block rather than at construction');

if (jassertMatch) {
    const asserted = new Set(
        [...jassertMatch[1].matchAll(/\b(p[A-Za-z0-9]+)\b/g)].map((m) => m[1]));
    const unasserted = declared.filter((d) => !asserted.has(d));
    check(unasserted.length === 0,
        `[6] the jassert names all ${declared.length} cache members`,
        unasserted.length ? `not asserted: ${unasserted.join(', ')}` : '');
}

console.log(`\n${failed === 0 ? '== ALL CHECKS PASSED ==' : `== ${failed} CHECK(S) FAILED ==`}`
          + `   (${passes} passed)`);
process.exit(failed === 0 ? 0 : 1);
