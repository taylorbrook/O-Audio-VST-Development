const path = require('path');
const S = require('/Users/taylorbrook/Dev/VST-development/scripts/serve-ui.js');
const { chromium } = S.resolvePlaywright();
const FONTS = '/Users/taylorbrook/Dev/VST-development/modules/ui/eb-garamond/fonts';
const groups = {
  label:   { css: "font-size:10.5px;text-transform:uppercase;letter-spacing:1.2px", items: { en: ['Presets'], fr: ['Préréglages'], 'zh-Hans': ['预设'] } },
  btn:     { css: "font-size:12px;letter-spacing:0.4px", items: { en: ['Save','Delete','Cancel'], fr: ['Enregistrer','Supprimer','Annuler'], 'zh-Hans': ['保存','删除','取消'] } },
  select13:{ css: "font-size:13px;letter-spacing:0.3px", items: {
     en: ['Init · Additive Build','Stepped Scan','Smooth Scan','Alias Demo','Drive Sweep','Vowel Pad','Pulse Narrowing','8-bit PPG','4-bit PPG'],
     fr: ['Init · construction additive','Balayage par paliers','Balayage fondu','Repliement','Balayage saturé','Voyelles','Impulsion qui rétrécit','PPG 8 bits','PPG 4 bits'],
     'zh-Hans': ['初始 · 加法叠加','阶梯扫描','平滑扫描','混叠演示','过载扫描','元音铺底','脉冲收窄','8 bit PPG','4 bit PPG'] } },
  msg12i:  { css: "font-size:12px;font-style:italic;letter-spacing:0.2px", items: {
     en: ['That is a factory preset name — choose another.','Replace “Bright Pad”?','Delete preset “Bright Pad”?','Could not save the preset.','Type a name first.','Save preset','Preset name'],
     fr: ['C’est le nom d’un préréglage d’usine — choisissez-en un autre.','Remplacer « Bright Pad » ?','Supprimer le préréglage « Bright Pad » ?','Impossible d’enregistrer le préréglage.','Saisissez d’abord un nom.','Enregistrer le préréglage','Nom du préréglage'],
     'zh-Hans': ['这是出厂预设的名称——请换一个。','替换“Bright Pad”？','删除预设“Bright Pad”？','无法保存预设。','请先输入名称。','保存预设','预设名称'] } },
  srcmeta: { css: "font-size:10.5px;font-style:italic;letter-spacing:0.4px", items: {
     en: ['saved by a newer version — kept, but it cannot play here','import failed — the current bank is unchanged'],
     fr: ['enregistré par une version plus récente — conservé, mais inutilisable ici','échec de l’import — la banque actuelle est inchangée'],
     'zh-Hans': ['由更新版本保存——已保留，但此处无法播放','导入失败——当前波表库未改变'] } },
  tourCaption: { css: "font-size:12px;font-style:italic", items: {
     en: ['Hover any control for an explanation · pick a lesson to hear one idea.','Interpolation Off: the scan jumps frame by frame. Switch it On to hear the morph.'],
     fr: ['Survolez une commande pour une explication · choisissez une leçon pour entendre une idée.','Interpolation désactivée : le balayage saute de trame en trame. Activez-la pour entendre la transformation.'],
     'zh-Hans': ['悬停在任意控件上查看说明 · 选择一节课来聆听一个概念。','插值关闭：扫描逐帧跳变。打开插值即可听到平滑过渡。'] } },
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
