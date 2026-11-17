// PLAN-4.1 P4: independent scan of the 18 new glossary keys against i18n.js
// (pattern_new_lint_zero_must_agree_with_independent_scan). Imports the page's
// tables directly; does not reuse the lint's code.
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
const ROOT = '/Users/taylorbrook/Dev/VST-development';
const fr = require(ROOT + '/scripts/i18n-fr-glossary.js').TERMS;
const zh = require(ROOT + '/scripts/i18n-zh-glossary.js').TERMS;
const { I18N, LABELS } = await import(ROOT + '/plugins/O-ParametricEQ/Source/ui/public/js/i18n.js');
const KEYS = ['bell','low shelf','high shelf','high cut','notch','band pass','tilt shelf','slope','placement',
  'listen','analyzer','pre','post','pre+post','gain scale','output gain','output level','display range',
  'mid signal','side signal'];
const norm = (s) => String(s).toLowerCase().replace(/’/g, "'").replace(/\.$/, '').replace(/\s+/g, ' ').trim();
let rows = 0, bad = 0;
for (const [tbl, name] of [[I18N, 'I18N'], [LABELS, 'LABELS']]) {
  for (const [id, e] of Object.entries(tbl)) {
    const k = norm(e.en.t);
    if (!KEYS.includes(k)) continue;
    rows++;
    const f = e.fr && norm(e.fr.t), z = e['zh-Hans'] && norm(e['zh-Hans'].t);
    const fOk = !fr[k] || fr[k].includes(f) || (e.fr && e.fr.termNote);
    const zOk = !zh[k] || zh[k].includes(z) || (e['zh-Hans'] && e['zh-Hans'].termNote);
    console.log(`${fOk && zOk ? 'ok ' : 'BAD'} ${name} ${id.padEnd(22)} "${e.en.t}"  fr "${e.fr?.t}"  zh "${e['zh-Hans']?.t}"`);
    if (!(fOk && zOk)) bad++;
  }
}
const missing = KEYS.filter(k => !fr[k] && k !== 'high cut' ? true : false).filter(k => !fr[k]);
console.log(`rows ${rows}, mismatches ${bad}; fr keys absent: ${KEYS.filter(k => !fr[k]).join(', ') || 'none'}; zh keys absent: ${KEYS.filter(k => !zh[k]).join(', ') || 'none'}`);
process.exit(bad ? 1 : 0);
