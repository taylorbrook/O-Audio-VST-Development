/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later
*/
// ============================================================================
// RESEARCH-3.3 Q8 / Q9 / Q11 measurement.
//
// Precedent: tests/tools/venue_layout_study.js (3.2 research, Q11).
//
// THIS RENDERS THE REAL PAGE, NOT A MOCK. It serves Source/ui/public with the
// bridge stub swapped in — byte-identical to what ui_layout_check.js serves —
// lets app.js / roomplan.js / venue.js run, and only THEN injects the candidate
// 3.3 components into the live .controls-column. Every number is read from
// getBoundingClientRect() on that tree.
//
// CONTEXT-3.3 constraint 12: "Research measures; it does not compute."
//
// FOUR PARTS:
//   A  scene-row variants -> the elevation strip's real height budget (Q9)
//   B  the recommended candidate at DPR 1 and 2, with a +120 px negative
//      control, against BOTH the coarse column assertion and the fitted-box
//      guard (Q11 / D25)
//   C  the same negative control against three stage constructions, to find
//      which one reproduces D-2's vacuity (Q11)
//   D  NC3 replayed on the venue rail as a METHOD CONTROL — if this does not
//      reproduce 3.2's verified 592<=592-passes-while-375<=213-fires result,
//      nothing in part B or C is trustworthy
//
// Usage: node plugins/O-Octagon/tests/tools/room_layout_study.js
// ============================================================================

'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const http = require('http');

const pluginRoot = path.resolve(__dirname, '..', '..');
const publicDir = path.join(pluginRoot, 'Source', 'ui', 'public');

const SHIP_W = 1100;
const SHIP_H = 720;

const MIME = {
    '.html': 'text/html; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.js': 'application/javascript; charset=utf-8',
};

function buildRoot() {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'ooct-room-study-'));
    fs.cpSync(publicDir, root, { recursive: true });
    fs.copyFileSync(path.join(pluginRoot, 'tests', 'ui-stub', 'juce-stub.js'),
                    path.join(root, 'js', 'juce', 'index.js'));
    return root;
}

function serve(root) {
    const server = http.createServer((req, res) => {
        const rel = decodeURIComponent(req.url.split('?')[0]);
        const file = path.join(root, rel === '/' ? 'index.html' : rel);
        if (!file.startsWith(root) || !fs.existsSync(file) || fs.statSync(file).isDirectory()) {
            res.writeHead(404); res.end('not found'); return;
        }
        res.writeHead(200, { 'Content-Type': MIME[path.extname(file)] || 'application/octet-stream' });
        fs.createReadStream(file).pipe(res);
    });
    return new Promise(resolve => server.listen(0, '127.0.0.1',
        () => resolve({ server, port: server.address().port })));
}

// Installs NOTHING — a study that mutates the machine to make itself run is not
// a study (the ui_layout_check.js rule, inherited).
function resolvePlaywright() {
    const { execSync } = require('child_process');
    const candidates = ['playwright'];
    try { candidates.push(path.join(execSync('npm root -g', { encoding: 'utf8' }).trim(), 'playwright')); }
    catch { /* npm not on PATH */ }
    try {
        const npxCache = path.join(os.homedir(), '.npm', '_npx');
        for (const dir of fs.readdirSync(npxCache))
            candidates.push(path.join(npxCache, dir, 'node_modules', 'playwright'));
    } catch { /* no npx cache */ }
    for (const c of candidates) { try { return require(c); } catch { /* next */ } }
    return null;
}

// ── The candidate markup, in the SAME token vocabulary as styles.css ────────
// `kind` selects the stage construction; `over` oversizes the strip to make a
// negative control; `rows` is the scene-button arrangement.
const BUILD = (rows, kind, over, storeInTitle) => `
(() => {
  document.querySelectorAll('#group-scenes,#group-elevation,#study-css').forEach(e => e.remove());
  const col = document.querySelector('.controls-column');
  const css = document.createElement('style');
  css.id = 'study-css';
  css.textContent = \`
    .scene-head{display:flex;align-items:center;justify-content:space-between;margin:0 0 8px 0}
    .scene-head .group-title{margin:0}
    .store-toggle{font-family:var(--mono);font-size:9px;letter-spacing:.08em;text-transform:uppercase;
      color:var(--ink-faint);background:var(--panel-lift);border:1px solid var(--rule);
      border-radius:2px;height:20px;padding:0 8px}
    .scene-body{display:flex;flex-direction:column;gap:8px}
    .scene-row{display:grid;gap:8px}
    .scene-btn{font-family:var(--mono);font-size:10px;letter-spacing:.06em;text-transform:uppercase;
      color:var(--ink-dim);background:var(--panel-lift);border:1px solid var(--rule);
      border-radius:2px;height:30px;padding:0 4px;white-space:nowrap}
    /* A: plain flex:1 stage, canvas in normal flow */
    .stage-A{flex:1;min-height:0;position:relative}
    /* B: EXACT clone of .plan-stage — centred flex */
    .stage-B{flex:1;min-height:0;position:relative;display:flex;align-items:center;justify-content:center}
    /* C: relative stage, ABSOLUTELY positioned canvas (the #plan-layers z-stack shape) */
    .stage-C{flex:1;min-height:0;position:relative}
    .stage-C > canvas{position:absolute;left:0;top:0}
    #elev-strip{display:block}
  \`;
  document.head.appendChild(css);

  const rows = ${JSON.stringify(rows)};
  const scenes = document.createElement('section');
  scenes.className = 'group'; scenes.id = 'group-scenes';
  scenes.innerHTML =
    ${storeInTitle
        ? `'<div class="scene-head"><h2 class="group-title">Scenes</h2>'
           + '<button type="button" class="store-toggle" aria-pressed="false">Store</button></div>'`
        : `'<h2 class="group-title">Scenes</h2>'`} +
    '<div class="group-body scene-body">' +
    rows.map(r => '<div class="scene-row" style="grid-template-columns:repeat(' + r.length
      + ',minmax(0,1fr))">' + r.map(n => '<button type="button" class="scene-btn">' + n
      + '</button>').join('') + '</div>').join('') + '</div>';
  col.appendChild(scenes);

  const elev = document.createElement('section');
  elev.className = 'group'; elev.id = 'group-elevation';
  elev.style.cssText = 'flex:1;min-height:0;display:flex;flex-direction:column';
  elev.innerHTML = '<h2 class="group-title">Elevation</h2>'
    + '<div class="elev-stage stage-${kind}" id="elev-stage"><canvas id="elev-strip"></canvas></div>';
  col.appendChild(elev);

  // Sized the way roomplan.js sizes the plan: from a MEASURED stage rect, with
  // an explicit width/height and a DPR backing store. A canvas is a replaced
  // element — left+right does not stretch it (o-textureforge-cursor-bug).
  const stage = document.getElementById('elev-stage');
  const c = document.getElementById('elev-strip');
  const r = stage.getBoundingClientRect();
  const dpr = window.devicePixelRatio || 1;
  const H = Math.floor(r.height) + ${over};
  c.style.width = Math.floor(r.width) + 'px';
  c.style.height = H + 'px';
  c.width = Math.round(r.width * dpr);
  c.height = Math.round(H * dpr);
})();`;

const MEASURE = () => {
    const rr = el => { const b = el.getBoundingClientRect();
        return { w: +b.width.toFixed(1), h: +b.height.toFixed(1) }; };
    const col = document.querySelector('.controls-column');
    const stage = document.getElementById('elev-stage');
    const strip = document.getElementById('elev-strip');
    const btns = [...document.querySelectorAll('.scene-btn')];
    const cs = getComputedStyle(btns[0]);
    const content = btns[0].getBoundingClientRect().width
        - parseFloat(cs.paddingLeft) - parseFloat(cs.paddingRight)
        - parseFloat(cs.borderLeftWidth) - parseFloat(cs.borderRightWidth);
    // Text fit measured as INK, not scrollWidth: a <button> centres its content
    // and reports scrollWidth === clientWidth even when the label overflows.
    const rng = document.createRange();
    const inks = btns.map(b => { rng.selectNodeContents(b);
        return +rng.getBoundingClientRect().width.toFixed(1); });
    const sr = strip.getBoundingClientRect(), tr = stage.getBoundingClientRect();
    return {
        scenesH: rr(document.getElementById('group-scenes')).h,
        elevH: rr(document.getElementById('group-elevation')).h,
        stage: rr(stage), strip: rr(strip),
        backing: { w: strip.width, h: strip.height },
        btn: rr(btns[0]), btnContent: +content.toFixed(1),
        widestInk: Math.max(...inks),
        textFits: Math.max(...inks) <= content,
        dpr: window.devicePixelRatio,
        coarse: col.scrollHeight <= col.clientHeight,
        coarseVals: col.scrollHeight + '<=' + col.clientHeight,
        guard: sr.height <= tr.height + 0.5,
        guardVals: sr.height.toFixed(0) + '<=' + tr.height.toFixed(0),
        sec8: document.documentElement.scrollHeight <= 720,
        sec8Vals: document.documentElement.scrollHeight + '<=720',
    };
};

const NAMED = ['ALL', 'FRONT', 'REAR', 'LEFT', 'RIGHT', 'SIDES'];
const USER = ['U1', 'U2', 'U3', 'U4'];

const VARIANTS = [
    ['V1  2 rows: 6 named / 4 user + STORE', [NAMED, [...USER, 'STORE']], false],
    ['V2  1 row of 11 (STORE inline)', [[...NAMED, ...USER, 'STORE']], false],
    ['V3  1 row of 10, STORE in the title row', [[...NAMED, ...USER]], true],
];

const verdict = (b) => b ? 'PASS' : 'FIRE';

(async () => {
    const pw = resolvePlaywright();
    if (pw === null) {
        console.log('FAIL: playwright not resolvable — npx playwright install chromium');
        process.exit(1);
    }

    const root = buildRoot();
    const { server, port } = await serve(root);
    const browser = await pw.chromium.launch();

    // ── A / B ───────────────────────────────────────────────────────────────
    for (const dsf of [1, 2]) {
        const ctx = await browser.newContext({ viewport: { width: SHIP_W, height: SHIP_H },
                                               deviceScaleFactor: dsf });
        const page = await ctx.newPage();
        await page.goto(`http://127.0.0.1:${port}/`, { waitUntil: 'networkidle' });
        await page.waitForTimeout(1400);

        if (dsf === 1) {
            const base = await page.evaluate(() => {
                const col = document.querySelector('.controls-column');
                const gs = [...col.querySelectorAll('.group')];
                return { col: +col.getBoundingClientRect().width.toFixed(0) + 'x'
                              + col.getBoundingClientRect().height.toFixed(0),
                         lastBottom: +gs[gs.length - 1].getBoundingClientRect().bottom.toFixed(0),
                         slack: +(col.getBoundingClientRect().bottom
                                  - gs[gs.length - 1].getBoundingClientRect().bottom).toFixed(0) };
            });
            console.log(`\n== BASELINE (shipping 3.2 tree) ==`);
            console.log(`   .controls-column ${base.col}, four groups end at y=${base.lastBottom}, `
                        + `slack ${base.slack} px\n`);
            console.log('== A · scene-row variants (DPR 1) ==');
            for (const [name, rows, storeInTitle] of VARIANTS) {
                await page.evaluate(BUILD(rows, 'A', 0, storeInTitle));
                await page.waitForTimeout(150);
                const m = await page.evaluate(MEASURE);
                console.log(`${name}`);
                console.log(`   scenes ${m.scenesH} px | elev group ${m.elevH} px | `
                    + `STRIP ${m.strip.w} x ${m.strip.h} | btn ${m.btn.w} px (content ${m.btnContent}) `
                    + `| widest ink ${m.widestInk} | labels fit: ${m.textFits ? 'YES' : 'NO — CLIPS'}`);
            }
        }

        console.log(`\n== B · recommended candidate V3, DPR ${dsf} ==`);
        for (const over of [0, 120]) {
            await page.evaluate(BUILD(VARIANTS[2][1], 'A', over, true));
            await page.waitForTimeout(150);
            const m = await page.evaluate(MEASURE);
            console.log(`   ${over === 0 ? 'as designed      ' : 'NEG CTRL +120 px '}`
                + `strip ${m.strip.w}x${m.strip.h} backing ${m.backing.w}x${m.backing.h} | `
                + `coarse ${verdict(m.coarse)} (${m.coarseVals}) | guard ${verdict(m.guard)} `
                + `(${m.guardVals}) | doc-§8 ${verdict(m.sec8)} (${m.sec8Vals})`);
        }

        if (dsf === 1) {
            console.log('\n== C · which stage construction makes the coarse assertion vacuous? ==');
            for (const kind of ['A', 'B', 'C']) {
                for (const over of [0, 120]) {
                    await page.evaluate(BUILD(VARIANTS[2][1], kind, over, true));
                    await page.waitForTimeout(120);
                    const m = await page.evaluate(MEASURE);
                    console.log(`   stage-${kind} over+${String(over).padStart(3)} | `
                        + `coarse ${verdict(m.coarse)} (${m.coarseVals}) | `
                        + `guard ${verdict(m.guard)} (${m.guardVals}) | doc-§8 ${verdict(m.sec8)}`);
                }
            }

            // ── D · METHOD CONTROL: replay 3.2's NC3 on the venue rail ──────
            console.log('\n== D · METHOD CONTROL — NC3 replayed on the venue rail ==');
            await page.click('#tab-venue');
            await page.waitForTimeout(600);
            const nc3 = await page.evaluate(() => {
                const rail = document.querySelector('.venue-rail');
                const st = document.querySelector('.miniplan');
                const g = document.getElementById('mini-geometry');
                const w = st.getBoundingClientRect().width;
                g.style.width = Math.floor(w) + 'px';
                g.style.height = Math.floor(w / 0.800) + 'px';
                g.style.flex = 'none';
                return new Promise(r => setTimeout(() => r({
                    coarse: rail.scrollHeight <= rail.clientHeight,
                    coarseVals: rail.scrollHeight + '<=' + rail.clientHeight,
                    guard: g.getBoundingClientRect().height <= st.getBoundingClientRect().height + 0.5,
                    guardVals: g.getBoundingClientRect().height.toFixed(0) + '<='
                             + st.getBoundingClientRect().height.toFixed(0),
                    sec8: document.documentElement.scrollHeight <= 720,
                    miniIsLastChild: [...rail.children].indexOf(st) === rail.children.length - 1,
                }), 200));
            });
            console.log(`   rail coarse ${verdict(nc3.coarse)} (${nc3.coarseVals}) | `
                + `guard ${verdict(nc3.guard)} (${nc3.guardVals}) | doc-§8 ${verdict(nc3.sec8)}`);
            console.log(`   .miniplan is the rail's LAST child: ${nc3.miniIsLastChild}`);
            console.log(`   ${!nc3.coarse || nc3.guard
                ? '   *** DID NOT REPRODUCE 3.2 — parts B and C are not trustworthy ***'
                : 'reproduces VERIFICATION-3.2 NC3 exactly; parts B and C stand.'}`);
        }

        await ctx.close();
    }

    await browser.close();
    server.close();
    fs.rmSync(root, { recursive: true, force: true });
})();
