const path = require('path');
const S = require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js');
const { chromium } = S.resolvePlaywright();
const FONTS = '/Users/taylorbrook/Dev/VST-development/modules/ui/eb-garamond/fonts';
const groups = {
  srcmeta: { css: "font-size:10.5px;font-style:italic;letter-spacing:0.4px", items: {
     en: ['saved by a newer version — kept, not playable here','from a newer version — kept, but not playable'],
     fr: ['d’une version plus récente — conservé, non jouable ici','version plus récente — conservée, non jouable','d’une version plus récente — conservé'],
     'zh-Hans': ['由更新版本保存——已保留，此处无法播放','来自更新版本——已保留，但无法播放'] } },
  msg12i:  { css: "font-size:12px;font-style:italic;letter-spacing:0.2px", items: {
     en: ['A factory preset has that name — choose another.'],
     fr: ['Nom réservé à un préréglage d’usine — choisissez-en un autre.','Nom d’un préréglage d’usine — choisissez-en un autre.'],
     'zh-Hans': ['该名称属于出厂预设——请换一个。'] } },
  btn11: { css: "font-size:11px;letter-spacing:0.3px", items: { en: ['Save','Delete','Cancel'], fr: ['Enregistrer','Supprimer','Annuler'], 'zh-Hans': ['保存','删除','取消'] } },
};
const html = `<!doctype html><html><head><meta charset="utf-8"><style>
@font-face{font-family:'EB Garamond';src:url('file://${FONTS}/EBGaramond-Regular.woff2') format('woff2');font-weight:400;font-style:normal}
@font-face{font-family:'EB Garamond';src:url('file://${FONTS}/EBGaramond-Italic.woff2') format('woff2');font-weight:400;font-style:italic}
body{font-family:'EB Garamond','Georgia','Times New Roman','PingFang SC','Microsoft YaHei',serif}
span{white-space:nowrap;display:inline-block}
</style></head><body></body></html>`;
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1120, height: 780 } });
  require('fs').writeFileSync(__dirname+'/m.html', html); await p.goto('file://'+__dirname+'/m.html');
  const out = await p.evaluate(async (groups) => {
    const res = {};
    for (const [g, def] of Object.entries(groups)) for (const [lang, arr] of Object.entries(def.items)) for (const t of arr) {
      const s = document.createElement('span'); s.lang = lang; s.style.cssText = def.css; s.textContent = t; document.body.appendChild(s);
    }
    await document.fonts.ready;
    for (const s of document.querySelectorAll('span')) {}
    await new Promise(r => setTimeout(r, 300));
    const spans = [...document.querySelectorAll('span')];
    let i = 0;
    for (const [g, def] of Object.entries(groups)) { res[g] = {}; for (const [lang, arr] of Object.entries(def.items)) { res[g][lang] = arr.map(t => [t, +spans[i++].getBoundingClientRect().width.toFixed(2)]); } }
    res.fontsLoaded = [...document.fonts].map(f => f.family + ' ' + f.style + ' ' + f.status);
    return res;
  }, groups);
  for (const [g,v] of Object.entries(out)) { if (g==='fontsLoaded') { console.log('fonts', v.join(', ')); continue; } for (const [l,arr] of Object.entries(v)) console.log(g.padEnd(12), l.padEnd(8), arr.map(([t,w])=>w+' '+JSON.stringify(t)).join(' | ')); }
  await b.close();
})();
