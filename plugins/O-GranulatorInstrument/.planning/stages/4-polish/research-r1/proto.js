// THROWAWAY measuring prototype (in-browser DOM moves on the served tree page; nothing written to the repo).
const S = require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js');
(async () => {
  const pw = S.resolvePlaywright();
  const built = S.buildRoot('O-GranulatorInstrument');
  const srv = await S.serve(built.root, () => {});
  const browser = await pw.chromium.launch();
  for (const variant of ['B', 'A']) {
    const page = await browser.newPage({ viewport: { width: 1200, height: 800 }, deviceScaleFactor: 2 });
    await page.goto(`http://127.0.0.1:${srv.port}/`);
    await page.waitForFunction(() => { const v = document.getElementById('val-polyphony'); return v && v.textContent !== '—'; }, null, { timeout: 15000 });
    await page.evaluate(() => document.fonts.ready);
    await page.click('#tab-fx');
    for (const lang of ['en', 'fr', 'zh-Hans']) {
      await page.evaluate((l) => window.__setLanguage(l), lang); await page.waitForTimeout(120);
      const info = await page.evaluate(({ variant, lang }) => {
        const pg = document.getElementById('page-fx'), X0 = 18;
        const $ = (id) => document.getElementById(id);
        if (!pg.dataset.proto) {
          pg.dataset.proto = '1';
          pg.querySelectorAll('.fig, .section-title, .flow-arrow, #write-ro, .heading').forEach((e) => { e.style.display = 'none'; });
          [...pg.querySelectorAll('.col-rule')].filter((e) => e.style.left === '788px').forEach((e) => { e.style.display = 'none'; });
          const st = document.createElement('style');
          st.textContent = `.pt{position:absolute;width:64px;font-style:italic;font-size:12.5px;line-height:14px;color:var(--ink-2);white-space:normal}
            .seg.v{flex-direction:column}.seg.v button{border-left:0;border-top:1px solid var(--rule);flex:1}.seg.v button:first-child{border-top:0}
            .knob-cell.tight{height:80px}.knob-cell.tight .knob-value{top:67px}`;
          document.head.appendChild(st);
          const mk = (cls, txt, css) => { const e = document.createElement('div'); e.className = cls; e.id = 'pt' + pg.querySelectorAll('.pt,.ph').length; if (txt !== null) { const s = document.createElement('span'); s.textContent = txt; e.appendChild(s); } e.style.cssText = css; pg.appendChild(e); return e; };
          window.__pt = { mk };
          const sw = $('toggle-s1_enable').cloneNode(true); sw.id = 'proto-fb-on'; sw.setAttribute('aria-pressed', 'true'); pg.appendChild(sw); 
          const seg = $('seg-s1_filter_type').cloneNode(true); seg.id = 'proto-prepost'; [...seg.children].slice(2).forEach((b) => b.remove()); seg.children[0].setAttribute('aria-pressed', 'false'); seg.children[1].setAttribute('aria-pressed', 'true'); pg.appendChild(seg);
          ['proto-t1', 'proto-t2', 'proto-t3', 'proto-hfb', 'proto-hrv'].forEach((id) => { const e = document.createElement('div'); e.id = id; pg.appendChild(e); });
        }
        const T = { en: { f: 'Grain filter', d: 'Drive · Crush', r: 'Ring modulator', fb: 'Feedback', rv: 'Reverb', pre: 'Pre', post: 'Post' },
                    fr: { f: 'Filtre de grain', d: 'Saturation · Écrasement', r: 'Modulateur en anneau', fb: 'Réinjection', rv: 'Réverb', pre: 'Pré', post: 'Post' },
                    'zh-Hans': { f: '颗粒滤波', d: '驱动 · 压碎', r: '环形调制器', fb: '反馈', rv: '混响', pre: '前置', post: '后置' } }[lang];
        const place = (id, l, t, tight) => { const c = $('cell-' + id); c.style.left = l + 'px'; c.style.top = t + 'px'; c.classList.toggle('tight', !!tight); };
        const title = (id, txt, l, t) => { const e = $(id); e.className = 'pt'; e.textContent = txt; e.style.left = l + 'px'; e.style.top = t + 'px'; };
        const head = (id, txt, l, t, w) => { const e = $(id); e.className = 'heading'; e.style.display = 'flex'; e.textContent = ''; const s = document.createElement('span'); s.textContent = txt; e.appendChild(s); e.style.left = l + 'px'; e.style.top = t + 'px'; e.style.width = w + 'px'; };
        const seg1 = $('seg-s1_filter_type'), sw = $('proto-fb-on'), pp = $('proto-prepost');
        pp.children[0].textContent = T.pre; pp.children[1].textContent = T.post;
        let R;
        if (variant === 'B') {
          R = [320, 408, 496, 608, 699];
          ['filter_cutoff', 'filter_rand', 'filter_res'].forEach((k, i) => place('s1_' + k, [90, 166, 238][i], R[0]));
          ['drive', 'crush', 'crush_prob'].forEach((k, i) => place('s1_' + k, [90, 166, 238][i], R[1]));
          ['ring_freq', 'ring_rand', 'ring_mix'].forEach((k, i) => place('s1_' + k, [90, 166, 238][i], R[2]));
          ['feedback', 'fb_low_cut', 'fb_high_cut', 'fb_saturation', 'fb_diffusion'].forEach((k, i) => place(k, 14 + i * 57, R[3]));
          ['reverb_mix', 'reverb_size', 'reverb_damp'].forEach((k, i) => place(k, [90, 166, 238][i], R[4]));
          title('proto-t1', T.f, 18, R[0]); seg1.classList.add('v'); seg1.style.cssText += `;left:18px;top:${R[0] + 17}px;width:62px;height:66px`;
          title('proto-t2', T.d, 18, R[1]); title('proto-t3', T.r, 18, R[2]);
          head('proto-hfb', T.fb, 18, 590, 242); sw.style.cssText += ';left:268px;top:587px;display:block';
          title('proto-hrv', T.rv, 18, R[4]); pp.classList.add('v'); pp.style.cssText += `;left:18px;top:${R[4] + 20}px;width:62px;height:40px`;
        } else {
          R = [319, 404, 489, 595, 699];
          ['filter_cutoff', 'filter_rand', 'filter_res'].forEach((k, i) => place('s1_' + k, [90, 166, 238][i], R[0], 1));
          ['drive', 'crush', 'crush_prob'].forEach((k, i) => place('s1_' + k, [90, 166, 238][i], R[1], 1));
          ['ring_freq', 'ring_rand', 'ring_mix'].forEach((k, i) => place('s1_' + k, [90, 166, 238][i], R[2], 1));
          ['feedback', 'fb_low_cut', 'fb_high_cut', 'fb_saturation'].forEach((k, i) => place(k, [18, 90, 166, 238][i], R[3], 1));
          ['fb_diffusion', 'reverb_mix', 'reverb_size', 'reverb_damp'].forEach((k, i) => place(k, [18, 90, 166, 238][i], R[4], 1));
          title('proto-t1', T.f, 18, R[0]); seg1.classList.add('v'); seg1.style.cssText += `;left:18px;top:${R[0] + 16}px;width:62px;height:62px`;
          title('proto-t2', T.d, 18, R[1]); title('proto-t3', T.r, 18, R[2]);
          head('proto-hfb', T.fb, 18, 578, 242); sw.style.cssText += ';left:268px;top:575px;display:block';
          head('proto-hrv', T.rv, 90, 682, 126); pp.classList.remove('v'); pp.style.cssText += ';left:224px;top:680px;width:76px;height:18px';
        }
        // measurements
        const tw = (e) => { const r = document.createRange(); r.selectNodeContents(e); const b = r.getBoundingClientRect(); return [+b.left.toFixed(1), +b.right.toFixed(1), +b.top.toFixed(1), +b.bottom.toFixed(1)]; };
        const out = {};
        const cells = [...pg.querySelectorAll('.knob-cell')].filter((c) => { const r = c.getBoundingClientRect(); return r.left < 305 && r.width; });
        // min horizontal gap between label texts and between readout texts in the same row
        const rows = {};
        cells.forEach((c) => { const t = Math.round(c.getBoundingClientRect().top); (rows[t] = rows[t] || []).push(c); });
        out.rows = Object.entries(rows).map(([t, cs]) => { cs.sort((a, b) => a.getBoundingClientRect().left - b.getBoundingClientRect().left);
          const L = cs.map((c) => tw(c.querySelector('.knob-label'))), V = cs.map((c) => tw(c.querySelector('.knob-value')));
          const gaps = (A) => A.slice(1).map((x, i) => +(x[0] - A[i][1]).toFixed(1));
          return `top ${t}: n=${cs.length} labelGaps[${gaps(L)}] valueGaps[${gaps(V)}] bottom ${Math.round(cs[0].getBoundingClientRect().bottom)} leftText ${L[0][0]} rightText ${L[L.length - 1][1]}`; });
        out.titles = ['proto-t1', 'proto-t2', 'proto-t3', 'proto-hrv', 'proto-hfb'].map((id) => { const e = $(id); const b = e.getBoundingClientRect(); const t = tw(e.firstChild.nodeType === 3 ? e : e.firstChild); return `${id} "${e.textContent}" box ${b.width.toFixed(0)}x${b.height.toFixed(0)} text ${(t[1] - t[0]).toFixed(1)}x${(t[3] - t[2]).toFixed(1)} @${t[0]},${t[2]}`; });
        out.ctl = [seg1, sw, pp].map((e) => { const b = e.getBoundingClientRect(); return `${e.id} ${b.left},${b.top} ${b.width}x${b.height} btn ${[...e.children].map((c) => { const t = tw(c); return (t[1] - t[0]).toFixed(1) + '/' + c.getBoundingClientRect().width.toFixed(1) + 'x' + c.getBoundingClientRect().height.toFixed(1); }).join(' ')}`; });
        return out;
      }, { variant, lang });
      console.log(`== variant ${variant} ${lang}`); console.log(info.rows.join('\n')); console.log(info.titles.join('\n')); console.log(info.ctl.join('\n'));
      await page.screenshot({ path: `proto-${variant}-${lang}.png`, clip: { x: 0, y: 280, width: 330, height: 520 } });
    }
    await page.close();
  }
  await browser.close(); await srv.close(); require('fs').rmSync(built.root, { recursive: true, force: true });
})().catch(e => { console.error(e); process.exit(70); });
