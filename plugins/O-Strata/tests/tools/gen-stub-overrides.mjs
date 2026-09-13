#!/usr/bin/env node
// gen-stub-overrides.mjs — regenerate the `combos` and `natives` blocks of
// tests/ui-stub/generic-overrides.json from the LIVE binary, never by hand
// (memory pattern_test_fixture_mirrors_drift_silently). Stage 3 plan Decision 20.
//
// Usage (repo root):
//   build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test --dump-choices \
//     | node plugins/O-Strata/tests/tools/gen-stub-overrides.mjs
//
// stdin  = the harness's JSON: { combos: { <id>: { choices: [...], def: <index> } } x 8,
//          modDestNames: [46], modSourceNames: [11] }
// output = generic-overrides.json (resolved relative to this file) with ONLY the
//          `combos` and `natives` blocks rewritten; every other block survives.
//          The two name lists are stored as JSON-encoded STRINGS because the page
//          does JSON.parse(await Juce.getNativeFunction('getModDestNames')()) — the
//          stub returns a native override verbatim.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const target = path.join(here, '..', 'ui-stub', 'generic-overrides.json');

const dump = JSON.parse(fs.readFileSync(0, 'utf8'));
for (const k of ['combos', 'modDestNames', 'modSourceNames'])
    if (!(k in dump)) { console.error(`gen-stub-overrides: dump has no "${k}"`); process.exit(2); }
const comboIds = Object.keys(dump.combos);
if (comboIds.length !== 8) { console.error(`gen-stub-overrides: expected 8 combos, got ${comboIds.length}`); process.exit(2); }
if (dump.modDestNames.length !== 46) { console.error(`gen-stub-overrides: expected 46 destinations, got ${dump.modDestNames.length}`); process.exit(2); }

const existing = fs.existsSync(target) ? JSON.parse(fs.readFileSync(target, 'utf8')) : {};
const combos = {};
for (const id of comboIds) combos[id] = { choices: dump.combos[id].choices, def: dump.combos[id].def };

const natives = Object.assign({}, existing.natives || {}, {
    getModDestNames: JSON.stringify(dump.modDestNames),
    getModSourceNames: JSON.stringify(dump.modSourceNames),
    requestTerrainRepush: true,
    // Round B (plan Decision 43): the three new natives. The chooser "cancels" (the
    // parameter stays), a streamed drop "succeeds" (the page shows no notice), the
    // perf report is acknowledged.
    chooseTerrainImage: { ok: false, reason: 'cancelled' },
    importTerrainImageData: { ok: true },
    reportViewPerf: true,
});

// Key order: the provenance header first, then every block the file already had
// (with combos / natives replaced in place), then any block that is new.
const out = { _generatedBy: 'O-Strata-render-test --dump-choices' };
for (const k of Object.keys(existing)) {
    if (k === '_generatedBy') continue;
    out[k] = k === 'combos' ? combos : k === 'natives' ? natives : existing[k];
}
if (!('combos' in out)) out.combos = combos;
if (!('natives' in out)) out.natives = natives;

fs.writeFileSync(target, JSON.stringify(out, null, 2) + '\n');
console.log(`gen-stub-overrides: wrote ${path.relative(process.cwd(), target)} — ${comboIds.length} combos, ${Object.keys(natives).length} natives`);
