// Q11 measurement — does D9's venue table + rail fit inside 1100 x 720?
// Renders a faithful D9 mock against the REAL styles.css tokens, at the real
// type sizes, and MEASURES. No arithmetic on row sums
// (pattern_flex1_container_slack_invisible_to_row_sum).

const fs = require('fs');
const path = require('path');
const http = require('http');
const { execSync } = require('child_process');

const pluginRoot = '/Users/taylorbrook/Dev/VST-development/plugins/O-Octagon';
const cssPath = path.join(pluginRoot, 'Source/ui/public/css/styles.css');
const realCss = fs.readFileSync(cssPath, 'utf8');

function resolvePlaywright() {
    const candidates = ['playwright'];
    try { candidates.push(path.join(execSync('npm root -g', { encoding: 'utf8' }).trim(), 'playwright')); } catch {}
    try {
        const npxCache = path.join(require('os').homedir(), '.npm/_npx');
        for (const dir of fs.readdirSync(npxCache))
            candidates.push(path.join(npxCache, dir, 'node_modules', 'playwright'));
    } catch {}
    for (const c of candidates) { try { return require(c); } catch {} }
    return null;
}

// Eight rows of real default-venue numbers so column widths are honest.
const ROWS = [
    [1, 'L',   '0.50', '4.50', '3.20', '+0.0', 'VERTEX'],
    [2, 'R',   '12.50', '4.50', '3.20', '+0.0', 'VERTEX'],
    [3, 'C',   '6.50', '2.25', '3.20', '-1.5', 'ON_EDGE'],
    [4, 'Lss', '0.50', '9.00', '3.20', '+0.0', 'VERTEX'],
    [5, 'Rss', '12.50', '9.00', '3.20', '+0.0', 'VERTEX'],
    [6, 'Lsr', '0.50', '13.50', '3.20', '+2.5', 'VERTEX'],
    [7, 'Rsr', '12.50', '13.50', '3.20', '+0.0', 'VERTEX'],
    [8, 'Cs',  '6.50', '15.75', '3.20', '+0.0', 'ON_EDGE'],
];

const page = `
<!doctype html><html><head><meta charset="utf-8"><style>
${realCss}

/* ── D9 venue screen — authored in the SAME token vocabulary as styles.css ── */
#screen-venue { display: flex; gap: 16px; }

.venue-main { flex: 1 1 auto; min-width: 0; display: flex; flex-direction: column; gap: 12px; }
.venue-rail { flex: 0 0 300px; display: flex; flex-direction: column; gap: 10px; min-height: 0; }

.vtable { width: 100%; border-collapse: collapse; }
.vtable thead th {
  font-family: var(--mono); font-size: 9px; letter-spacing: .08em;
  color: var(--ink-faint); text-transform: uppercase; text-align: left;
  padding: 0 6px 6px 6px; border-bottom: 1px solid var(--rule);
  font-weight: 400;
}
.vtable td { padding: 3px 6px; border-bottom: 1px solid var(--rule-soft); }
.vtable td.num { width: 26px; font-family: var(--mono); font-size: 10px; color: var(--ink-faint); }
.vtable td.cls { font-family: var(--mono); font-size: 9px; color: var(--ink-dim); width: 66px; }

.vfield {
  font-family: var(--mono); font-size: 11px; font-variant-numeric: tabular-nums;
  background: var(--panel); color: var(--ink);
  border: 1px solid var(--rule); border-radius: 2px;
  padding: 4px 6px; width: 100%; height: 24px;
}
.vfield.lab { width: 64px; }
.vfield.co  { width: 78px; }
.vfield.tr  { width: 66px; }

.rake-row { display: flex; gap: 14px; align-items: center; }
.rake-row label { font-family: var(--mono); font-size: 9px; letter-spacing: .08em;
  color: var(--ink-faint); text-transform: uppercase; }

.rail-card { background: var(--panel); border: 1px solid var(--rule-soft);
  border-radius: 3px; padding: 8px 10px; }
.rail-k { font-family: var(--mono); font-size: 9px; letter-spacing: .08em;
  color: var(--ink-faint); text-transform: uppercase; }
.rail-v { font-family: var(--mono); font-size: 11px; font-variant-numeric: tabular-nums;
  color: var(--ink); }

/* Mini-plan: SAME portrait aspect the Room plan derives — 0.800 (15.60 x 19.50 m).
   Width-bound inside the rail. */
#mini-plan-box { width: 100%; }
#mini-plan-layers { position: relative; margin: 0 auto; }

.btn-row { display: flex; gap: 8px; }
.btn { flex: 1; font-family: var(--mono); font-size: 10px; letter-spacing: .06em;
  text-transform: uppercase; background: var(--panel-lift); color: var(--ink-dim);
  border: 1px solid var(--rule); border-radius: 2px; padding: 6px 8px; height: 28px; }
.ping-card { display: flex; flex-direction: column; gap: 6px; }
.ping-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 4px; }
.ping-cell { font-family: var(--mono); font-size: 10px; height: 24px;
  background: var(--panel-lift); color: var(--ink-dim);
  border: 1px solid var(--rule); border-radius: 2px; }
</style></head><body>
<div class="frame">
  <div class="header">
    <div style="font-family:var(--serif);font-size:22px">O&#8202;&#183;&#8202;OCTAGON</div>
    <div><button class="screen-tab">ROOM</button><button class="screen-tab is-active">VENUE</button></div>
  </div>

  <div class="stage" style="min-height:0">
    <section class="screen is-active" id="screen-venue">

      <div class="venue-main">
        <table class="vtable" id="vtable">
          <thead><tr>
            <th>#</th><th>Label</th><th>X&#8201;m</th><th>Y&#8201;m</th><th>Z&#8201;m</th>
            <th>Trim&#8201;dB</th><th>Class</th>
          </tr></thead>
          <tbody>
          ${ROWS.map(r => `<tr>
            <td class="num">${r[0]}</td>
            <td><input class="vfield lab" value="${r[1]}"></td>
            <td><input class="vfield co" value="${r[2]}"></td>
            <td><input class="vfield co" value="${r[3]}"></td>
            <td><input class="vfield co" value="${r[4]}"></td>
            <td><input class="vfield tr" value="${r[5]}"></td>
            <td class="cls">${r[6]}</td>
          </tr>`).join('')}
          </tbody>
        </table>

        <div class="rake-row">
          <label>Rake front</label><input class="vfield co" value="0.00">
          <label>Rake rear</label><input class="vfield co" value="1.20">
          <span class="rail-k" style="margin-left:auto">Venue</span>
          <input class="vfield" style="width:220px" value="Default (placeholder &#8212; NOT measured)">
        </div>
      </div>

      <div class="venue-rail">
        <div class="rail-card">
          <div class="rail-k">Negotiated set</div>
          <div class="rail-v" id="setname">7.1 Surround</div>
        </div>

        <div class="rail-card" id="mini-plan-box">
          <div class="rail-k" style="margin-bottom:6px">Plan</div>
          <div id="mini-plan-layers"></div>
        </div>

        <div class="btn-row">
          <button class="btn">Save&#8201;.venue</button>
          <button class="btn">Load&#8201;.venue</button>
        </div>

        <div class="rail-card ping-card">
          <div class="rail-k">Verify ping</div>
          <div class="ping-grid">
            ${[1,2,3,4,5,6,7,8].map(n => `<button class="ping-cell">${n}</button>`).join('')}
          </div>
          <div class="btn-row">
            <button class="btn">Cycle</button><button class="btn">Stop</button>
          </div>
        </div>
      </div>

    </section>
  </div>

  <div class="footer"><span class="rail-k">SOURCE 6.50 &#215; 12.00 m &#183; ENVELOPE 15.60 &#215; 19.50 m</span></div>
</div>

<script>
// Mini-plan sized the SAME way roomplan.js sizes the big one: from a MEASURED
// box and the RETURNED envelope aspect. Width-bound inside the rail here.
const ASPECT = 15.60 / 19.50;               // 0.800, portrait
const box = document.getElementById('mini-plan-box');
const layers = document.getElementById('mini-plan-layers');
// HEIGHT-BOUND, not width-bound. The rail's residual height after every other
// card is what the plan must fit inside — exactly how roomplan.js fits the big
// plan into the stage. Width-binding a PORTRAIT aspect overflows the rail.
const rail = document.querySelector('.venue-rail');
const others = [...rail.children].filter(c => c !== box);
const gaps = 10 * (rail.children.length - 1);
const used = others.reduce((s, c) => s + c.getBoundingClientRect().height, 0);
const chrome = 8 + 8 + 6 + 14;              // card padding + the "Plan" caption
const availH = rail.clientHeight - used - gaps - chrome;
const availW = box.clientWidth - 20;
const h = Math.min(availH, Math.round(availW / ASPECT));
const w = Math.round(h * ASPECT);
const avail = w;
layers.style.width  = w + 'px';
layers.style.height = h + 'px';
window.__fit = { availH, availW, chosenW: w, chosenH: h, heightBound: h === availH };
layers.style.background = 'var(--ground)';
layers.style.border = '1px solid var(--rule-soft)';
</script>
</body></html>`;

(async () => {
    const pw = resolvePlaywright();
    if (!pw) { console.log('FAIL: playwright not resolvable'); process.exit(1); }

    const server = http.createServer((req, res) => {
        res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
        res.end(page);
    });
    await new Promise(r => server.listen(0, '127.0.0.1', r));
    const port = server.address().port;

    const browser = await pw.chromium.launch();
    const ctx = await browser.newContext({ viewport: { width: 1100, height: 720 }, deviceScaleFactor: 1 });
    const p = await ctx.newPage();
    await p.goto(`http://127.0.0.1:${port}/`, { waitUntil: 'networkidle' });

    const m = await p.evaluate(() => {
        const r = (el) => { const b = el.getBoundingClientRect(); return { w: +b.width.toFixed(1), h: +b.height.toFixed(1), top: +b.top.toFixed(1), bottom: +b.bottom.toFixed(1) }; };
        const table = document.getElementById('vtable');
        const rail  = document.querySelector('.venue-rail');
        const main  = document.querySelector('.venue-main');
        const mini  = document.getElementById('mini-plan-layers');
        const screen = document.getElementById('screen-venue');
        const rows = [...table.querySelectorAll('tbody tr')];
        const firstField = document.querySelector('.vfield');

        return {
            docScrollW: document.documentElement.scrollWidth,
            docScrollH: document.documentElement.scrollHeight,
            screen: r(screen),
            screenScrollH: screen.scrollHeight,
            screenClientH: screen.clientHeight,
            main: r(main),
            mainScrollH: main.scrollHeight,
            rail: r(rail),
            railScrollH: rail.scrollHeight,
            railClientH: rail.clientHeight,
            table: r(table),
            tableScrollW: table.scrollWidth,
            rowH: +rows[0].getBoundingClientRect().height.toFixed(1),
            allRows: rows.length,
            lastRowBottom: +rows[rows.length - 1].getBoundingClientRect().bottom.toFixed(1),
            mini: r(mini),
            miniAspect: +(mini.getBoundingClientRect().width / mini.getBoundingClientRect().height).toFixed(4),
            fieldH: +firstField.getBoundingClientRect().height.toFixed(1),
            fieldFontPx: getComputedStyle(firstField).fontSize,
            fieldTabular: getComputedStyle(firstField).fontVariantNumeric,
            // Does anything overflow its own container?
            railOverflows: rail.scrollHeight > rail.clientHeight + 0.5,
            screenOverflows: screen.scrollHeight > screen.clientHeight + 0.5,
            fit: window.__fit,
        };
    });

    console.log(JSON.stringify(m, null, 2));

    const png = path.join(require('os').tmpdir(), 'octagon-venue-layout.png');
    await p.screenshot({ path: png });
    console.log('\nscreenshot: ' + png);

    await browser.close();
    server.close();
})();
