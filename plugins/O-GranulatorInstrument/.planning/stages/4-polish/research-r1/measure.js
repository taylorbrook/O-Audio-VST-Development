const path = require('path');
const S = require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js');
(async () => {
  const pw = S.resolvePlaywright();
  if (!pw) { console.log('NO PLAYWRIGHT'); process.exit(77); }
  const built = S.buildRoot('O-GranulatorInstrument');
  console.log('stub', built.stubKind, built.seedFrom, 'root', built.root);
  const srv = await S.serve(built.root, () => {});
  const browser = await pw.chromium.launch();
  const page = await browser.newPage({ viewport: { width: 1200, height: 800 } });
  const errs = []; page.on('pageerror', e => errs.push(e.message));
  await page.goto(`http://127.0.0.1:${srv.port}/`);
  await page.waitForFunction(() => { const v = document.getElementById('val-polyphony'); return v && v.textContent !== '—'; }, null, { timeout: 15000 });
  await page.evaluate(() => document.fonts.ready);
  const out = {};
  out.inner = await page.evaluate(() => [innerWidth, innerHeight]);
  const rect = (sel) => page.evaluate((s) => [...document.querySelectorAll(s)].map(e => { const r = e.getBoundingClientRect(); const cs = getComputedStyle(e); return { id: String(e.id || e.getAttribute("class")), x: +r.left.toFixed(2), y: +r.top.toFixed(2), w: +r.width.toFixed(2), h: +r.height.toFixed(2), b: +r.bottom.toFixed(2), r: +r.right.toFixed(2), fs: cs.fontSize, lh: cs.lineHeight, ff: cs.fontFamily.slice(0,40), vis: r.width>0 }; }), sel);
  // header-level (all pages)
  out.header = await rect('#toggle-freeze, #reset-all, #tab-streams, #tab-fx, #tab-mod, #tab-voice, #strip, #gear-btn, #frame');
  out.streams = await rect('#toggle-s1_enable, #toggle-s1_mute, #toggle-s1_solo, #page-streams > .snum:first-of-type, #cell-s1_position, #cell-s1_position .knob-label, #cell-s1_position .knob-dial, #cell-s1_position .knob, #cell-s1_position .knob-value');
  // extent of streams page content
  const extent = (pg) => page.evaluate((pg) => { let top=1e9,bot=0,lft=1e9,rgt=0, who=''; document.querySelectorAll(`#${pg} > *`).forEach(e=>{ const r=e.getBoundingClientRect(); if(!r.width) return; if(r.top<top) top=r.top; if(r.bottom>bot){bot=r.bottom; who=String(e.id||e.getAttribute("class"));} if(r.left<lft) lft=r.left; if(r.right>rgt) rgt=r.right; }); return {top,bot,lft,rgt,who}; }, pg);
  out.streamsExtent = await extent('page-streams');
  await page.click('#tab-fx');
  await page.waitForTimeout(200);
  out.fxExtent = await extent('page-fx');
  out.fxS1 = await page.evaluate(() => {
    const pg = document.getElementById('page-fx'); const res = [];
    [...pg.children].forEach(e => { const r = e.getBoundingClientRect(); if (!r.width) return; if (r.left > 305 && r.top < 670) return; const cs=getComputedStyle(e);
      res.push([ String(e.id||e.getAttribute("class")).slice(0,34), e.dataset.display||'', +r.left.toFixed(1), +r.top.toFixed(1), +r.width.toFixed(1), +r.height.toFixed(1), +r.bottom.toFixed(1), cs.fontSize, (e.textContent||'').trim().slice(0,22)].join(' | ')); });
    return res; });
  out.fxParts = await rect('#cell-s1_filter_cutoff .knob-label, #cell-s1_filter_cutoff .knob-dial, #cell-s1_filter_cutoff .knob, #cell-s1_filter_cutoff svg.arc, #cell-s1_filter_cutoff .knob-value, #cell-s4_ring_mix, #cell-s4_ring_mix .knob-value, #cell-reverb_damp .knob-value, #write-ro, #write-ro .k, #write-ro .v, #write-ro .bar, #write-ro .note, .fig.wide canvas, #seg-s1_filter_type button');
  // arc extent (visual knob radius incl. overflow visible arc)
  out.arc = await page.evaluate(() => { const c = document.querySelector('#cell-s1_filter_cutoff'); const a=[...c.querySelectorAll('svg.arc *')].map(p=>{const r=p.getBoundingClientRect(); return [p.getAttribute('class'), +r.left.toFixed(1), +r.top.toFixed(1), +r.width.toFixed(1), +r.height.toFixed(1), +r.bottom.toFixed(1)].join(' ');}); return a; });
  // text widths per language for candidate captions (knob-label style 11px, heading 12.5 italic, seg button)
  const langs = ['en','fr','zh-Hans'];
  const cands = {
    label: { en:['Amount','Low Cut','High Cut','Saturate','Diffuse','Mix','Size','Damp','Feedback','Reverb'], fr:['Quantité','Coupe-bas','Coupe-haut','Saturation','Diffusion','Mix','Taille','Amort.','Réinjection','Réverb'], 'zh-Hans':['量','低切','高切','饱和','扩散','混合','大小','阻尼','反馈','混响'] },
    heading: { en:['Feedback','Reverb','Feedback network','Feedback · Reverb','Grain filter','Ring modulator','Crush','Drive'], fr:['Réinjection','Réverb','Réverbération','Réseau de réinjection','Réinjection · Réverb','Filtre de grain','Modulateur en anneau','Écrasement','Saturation'], 'zh-Hans':['反馈','混响','反馈网络','反馈 · 混响','颗粒滤波','环形调制器','压碎','驱动'] },
    seg: { en:['Pre','Post','Off','On','LP','BP','HP'], fr:['Pré','Post','Arrêt','Marche','Avant','Après','PB','PBDE','PH'], 'zh-Hans':['前','后','前置','后置','关','开','低通','带通','高通'] },
  };
  out.widths = {};
  for (const lang of langs) {
    await page.evaluate((l) => window.__setLanguage(l), lang);
    await page.waitForTimeout(150);
    out.widths[lang] = await page.evaluate(({cands, lang}) => {
      const m = (host, cls, txt, tag) => { const e = document.createElement(tag||'div'); e.className = cls; e.style.cssText += ';position:absolute;left:0;top:0;width:auto;right:auto;overflow:visible;display:inline-block'; e.textContent = txt; host.appendChild(e); const rg = document.createRange(); rg.selectNodeContents(e); const r = rg.getBoundingClientRect(); const cs = getComputedStyle(e); const o = [txt, +r.width.toFixed(1), +r.height.toFixed(1), cs.fontSize]; e.remove(); return o.join(' '); };
      const cell = document.getElementById('cell-s1_filter_cutoff'); const pg = document.getElementById('page-fx'); const seg = document.getElementById('seg-s1_filter_type');
      const r = {};
      r.label = cands.label[lang].map(t => m(cell, 'knob-label', t));
      r.heading = cands.heading[lang].map(t => { const e=document.createElement('div'); e.className='heading'; e.style.left='0';e.style.top='0'; const s=document.createElement('span'); s.textContent=t; e.appendChild(s); pg.appendChild(e); const rr=s.getBoundingClientRect(); const o=[t,+rr.width.toFixed(1),+rr.height.toFixed(1)].join(' '); e.remove(); return o; });
      r.seg = cands.seg[lang].map(t => { const b=document.createElement('button'); b.textContent=t; b.style.flex='none'; seg.appendChild(b); const rg=document.createRange(); rg.selectNodeContents(b); const rr=rg.getBoundingClientRect(); const o=[t,+rr.width.toFixed(1),+rr.height.toFixed(1), getComputedStyle(b).fontSize].join(' '); b.remove(); return o; });
      // existing rendered label widths on the page (real nodes)
      const real = (sel) => [...document.querySelectorAll(sel)].map(e => { const rg=document.createRange(); rg.selectNodeContents(e); const rr=rg.getBoundingClientRect(); return (e.textContent+' '+rr.width.toFixed(1)+'/'+e.getBoundingClientRect().width.toFixed(1)); });
      r.real = real('#cell-feedback .knob-label, #cell-fb_low_cut .knob-label, #cell-fb_high_cut .knob-label, #cell-fb_saturation .knob-label, #cell-fb_diffusion .knob-label, #cell-reverb_mix .knob-label, #cell-reverb_size .knob-label, #cell-reverb_damp .knob-label, #cell-s1_filter_res .knob-label, #cell-s1_crush_prob .knob-label, #page-fx .shead:first-of-type .sname, #tab-fx span[data-i18n], #seg-s1_filter_type button');
      r.readouts = real('#val-fb_low_cut, #val-fb_high_cut, #val-feedback, #val-reverb_mix, #val-s1_filter_cutoff');
      return r; }, {cands, lang});
  }
  await page.evaluate(() => window.__setLanguage('en'));
  // tooltips: enable help? check tipsEnabled default; hover bottom-right knob and the right-most stream IV knob
  out.tips = [];
  for (const lang of langs) {
    await page.evaluate((l) => window.__setLanguage(l), lang); await page.waitForTimeout(100);
    for (const sel of ['#knob-feedback', '#knob-reverb_damp', '#knob-s4_ring_mix', '#knob-s1_filter_cutoff', '#knob-s4_filter_res']) {
      await page.mouse.move(600, 150); await page.waitForTimeout(80);
      const bb = await page.locator(sel).boundingBox();
      await page.mouse.move(bb.x + bb.width/2, bb.y + bb.height/2); await page.waitForTimeout(600);
      out.tips.push(await page.evaluate(({sel,lang}) => { const t=document.querySelector('.tooltip'); const r=t.getBoundingClientRect(); const a=document.querySelector(sel).getBoundingClientRect(); return [lang, sel, 'vis', t.classList.contains('visible'), 'tip', +r.left.toFixed(1), +r.top.toFixed(1), +r.width.toFixed(1), +r.height.toFixed(1), 'bottom', +r.bottom.toFixed(1), 'right', +r.right.toFixed(1), t.dataset.placement, 'anchorTop', +a.top.toFixed(1)].join(' '); }, {sel,lang}));
    }
  }
  await page.evaluate(() => window.__setLanguage('en'));
  await page.mouse.move(600,150);
  // value entry at bottom-right
  await page.click('#cell-reverb_damp .knob-label');
  await page.waitForTimeout(100);
  out.entry = await rect('#cell-reverb_damp .knob-value-input, #cell-reverb_damp .knob-value');
  await page.keyboard.press('Escape');
  await page.screenshot({ path: 'fx-en.png' });
  await page.click('#tab-streams'); await page.screenshot({ path: 'streams-en.png' });
  out.errs = errs;
  console.log(JSON.stringify(out, null, 1));
  await browser.close(); await srv.close(); require('fs').rmSync(built.root, { recursive: true, force: true });
})().catch(e => { console.error(e); process.exit(70); });
