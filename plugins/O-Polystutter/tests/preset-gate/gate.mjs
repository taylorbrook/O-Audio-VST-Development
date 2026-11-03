// O-Polystutter preset-manager.js gate (v1.19.1, converge step 2).
//
// The C++ side (OPolystutterPresetManager.h) is untouched by v1.19.1; only the
// served preset-manager.js moved from a 1.0.0-era hand copy to the module copy.
// So the gate is on the JS, driven the way js/app.js drives it:
//
//   A  static   every native name the module JS asks for is registered in
//               PluginEditor.cpp; app.js never calls promptDelete / passes
//               deleteButton or onConfirmDelete (so the reworked delete prompt
//               is dormant here — app.js deletes through its own 2-click arm).
//   B  trace    baseline-1.19.0 copy vs the served copy, same scenario against
//               the same mock backend: native calls + args, display text,
//               callbacks, return values must be IDENTICAL.
//   C  wait     no JUCE backend: served copy resolves initialize() after the
//               bounded 100 x 50 ms poll; baseline still pending at 6 s (control).
//   D  served   Source/ui/public/modules/preset-manager.js == module copy, byte
//               for byte (ouaricon_add_module copied it).
//
// Usage: node tests/preset-gate/gate.mjs        (from plugins/O-Polystutter)

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const PLUGIN = path.resolve(HERE, '../..');
const ROOT = path.resolve(PLUGIN, '../..');
const BASELINE = path.join(HERE, 'baseline-1.19.0/preset-manager.js');
const SERVED = path.join(PLUGIN, 'Source/ui/public/modules/preset-manager.js');
const MODULE = path.join(ROOT, 'modules/persistence/preset-manager/js/preset-manager.js');

let fails = 0;
const check = (name, ok, detail = '') => {
    console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}${detail ? '  — ' + detail : ''}`);
    if (!ok) fails++;
};

// ── A: static ───────────────────────────────────────────────────────────────
{
    const js = fs.readFileSync(SERVED, 'utf8');
    const editor = fs.readFileSync(path.join(PLUGIN, 'Source/PluginEditor.cpp'), 'utf8');
    const app = fs.readFileSync(path.join(PLUGIN, 'Source/ui/public/js/app.js'), 'utf8');
    const asked = [...js.matchAll(/getNativeFunction\("(\w+)"\)/g)].map((m) => m[1]);
    const registered = new Set([...editor.matchAll(/withNativeFunction\(\s*"(\w+)"/g)].map((m) => m[1]));
    const missing = asked.filter((n) => !registered.has(n));
    check('A1 native names registered', asked.length === 10 && missing.length === 0,
        `${asked.length} asked, missing [${missing.join(', ')}]`);
    const dormant = !/promptDelete|deleteButton|onConfirmDelete/.test(app);
    check('A2 app.js never reaches promptDelete', dormant);
}

// ── mock DOM / backend ──────────────────────────────────────────────────────
function makeEl(id) {
    const handlers = {};
    return {
        id, textContent: '',
        addEventListener: (t, fn) => { (handlers[t] ||= []).push(fn); },
        click: async () => { for (const fn of handlers.click || []) await fn(); },
    };
}

function makeBackend(trace) {
    const factory = ['Basic Stutter', 'Glitch Storm', 'Half Time'];
    const user = ['My Repeat'];
    let current = 'Basic Stutter';
    const all = () => [...factory, ...user].sort();
    const idx = () => all().indexOf(current);
    const fns = {
        savePreset: (n) => { if (!user.includes(n)) user.push(n); current = n; return true; },
        savePresetWithDialog: () => { user.push('Dialog Save'); current = 'Dialog Save'; return { success: true, name: 'Dialog Save' }; },
        loadPreset: (n) => { if (!all().includes(n)) return false; current = n; return true; },
        loadPresetFromFile: () => ({ success: true, name: 'From Disk' }),
        getPresetList: () => all(),
        getCurrentPreset: () => current,
        selectNextPreset: () => { const l = all(); return l[(idx() + 1) % l.length]; },
        selectPreviousPreset: () => { const l = all(); return l[(idx() - 1 + l.length) % l.length]; },
        deletePreset: (n) => { const i = user.indexOf(n); if (i < 0) return false; user.splice(i, 1); return true; },
        isFactoryPreset: (n) => factory.includes(n),
    };
    return (name) => async (...args) => {
        const r = fns[name](...args);
        trace.push(`native ${name}(${JSON.stringify(args)}) -> ${JSON.stringify(r)}`);
        return r;
    };
}

async function load(file) {
    // Fresh module instance per run (query string busts the ESM cache).
    return (await import(pathToFileURL(file).href + '?t=' + Math.random())).PresetManager;
}

async function runScenario(file) {
    const trace = [];
    globalThis.window = { __JUCE__: { backend: {} } };
    const PM = await load(file);
    const els = Object.fromEntries(['disp', 'prev', 'next', 'save', 'load'].map((k) => [k, makeEl(k)]));
    const pm = new PM({
        displayElement: els.disp, prevButton: els.prev, nextButton: els.next,
        saveButton: els.save, loadButton: els.load,
        getNativeFunction: makeBackend(trace),
        onPresetChanged: (n) => trace.push(`cb changed ${n}`),
        onPresetListUpdated: (l) => trace.push(`cb list ${JSON.stringify(l)}`),
    });
    const snap = (tag, r) => trace.push(`${tag} -> ${JSON.stringify(r)} | disp=${els.disp.textContent} cur=${pm.getCurrentPreset()} list=${JSON.stringify(pm.getPresetList())}`);
    snap('init', await pm.initialize());
    for (const b of ['next', 'next', 'next', 'prev', 'prev', 'prev', 'prev']) { await els[b].click(); snap(`click ${b}`); }
    snap('loadPreset Half Time', await pm.loadPreset('Half Time'));
    snap('loadPreset missing', await pm.loadPreset('Nope'));
    snap('savePreset "  User A "', await pm.savePreset('  User A '));
    snap('savePreset empty', await pm.savePreset('   '));
    await els.save.click(); snap('click save');
    await els.load.click(); snap('click load');
    for (const n of pm.getPresetList()) snap(`isFactory ${n}`, await pm.isFactoryPreset(n));
    snap('delete factory', await pm.deletePreset('Glitch Storm'));
    snap('delete user', await pm.deletePreset('User A'));
    snap('delete missing', await pm.deletePreset('Ghost'));
    await els.next.click(); snap('click next after delete');
    snap('refresh', await pm.refresh());
    snap('initialize again', await pm.initialize());
    return trace;
}

// ── B: trace ────────────────────────────────────────────────────────────────
{
    const a = await runScenario(BASELINE);
    const b = await runScenario(SERVED);
    const diff = a.findIndex((l, i) => l !== b[i]);
    const same = a.length === b.length && diff < 0;
    check('B  baseline vs served trace identical', same,
        same ? `${a.length} lines` : `first diff at ${diff}: ${a[diff]}  ||  ${b[diff]}`);
}

// ── C: bounded wait ─────────────────────────────────────────────────────────
async function waitOutcome(file, budgetMs) {
    globalThis.window = {};
    const PM = await load(file);
    const errs = [];
    const origErr = console.error;
    console.error = (...a) => errs.push(a.join(' '));
    const t0 = Date.now();
    const pm = new PM({ getNativeFunction: () => async () => null });
    const r = await Promise.race([
        pm.initialize().then(() => 'resolved'),
        new Promise((res) => setTimeout(() => res('pending'), budgetMs)),
    ]);
    console.error = origErr;
    return { r, ms: Date.now() - t0, logged: errs.some((e) => e.includes('JUCE backend unavailable')) };
}
{
    const s = await waitOutcome(SERVED, 6000);
    check('C1 served: no backend resolves, logs', s.r === 'resolved' && s.logged && s.ms >= 4500 && s.ms < 6000,
        `${s.r} at ${s.ms} ms, logged=${s.logged}`);
    const o = await waitOutcome(BASELINE, 6000);
    check('C2 control: baseline still pending at 6 s', o.r === 'pending', o.r);
}

// ── D: served copy is the module copy ───────────────────────────────────────
{
    const same = fs.readFileSync(SERVED).equals(fs.readFileSync(MODULE));
    check('D  served == module preset-manager.js', same);
}

console.log(fails ? `\n${fails} FAIL` : '\nALL PASS');
process.exit(fails ? 1 : 0);
