/*
   This file is part of O-Bowed, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// ============================================================================
// i18n.js — O-Bowed on-page copy, English + French (v1.5.0, canon v2)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). On this
// plugin "every later initializer" is the ENTIRE UI, because the controller is
// one inline <script type="module"> in index.html rather than a file that can
// fail in isolation. check-i18n assertion 7 enforces the rule.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a file named i18n-fr.js would have to be
// reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens). One
// combined file for both languages sidesteps the question.
//
// THIS PLUGIN HAS NO HOVER-HELP. v1.4.1 carried no data-tip anywhere — only
// three native title= attributes, which contract §4 DELETES rather than
// localizes. So I18N is empty and TIP_BINDINGS is empty, and check-i18n
// assertion 2 reports "0 tip(s) bound" as the correct state rather than a gap.
// Authoring hover-help prose is Stage M's job, not this stage's.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
//
// ── EVERY FRENCH STRING HERE WAS MEASURED IN ITS OWN ELEMENT ────────────────
//
// Never in another plugin's: K2 proved two plugins render the same two words at
// the same declared font-size 8.24 px apart, so a borrowed absolute reads
// exactly like a right number and is not one. The three boxes that constrain
// this page, measured at v1.4.1:
//
//   .knob-label          62 px hard cap, `nowrap` + `overflow: hidden` +
//                        `text-overflow: ellipsis`. Past 62 px a caption is
//                        CLIPPED, silently — it never pushes anything, so the
//                        geometry diff cannot see it and only a per-string
//                        measurement can. Two candidates were rejected on this
//                        alone: "Raideur crin" (63.0) and "Tenue infinie"
//                        (65.0). NOTE: "Rev. Friction" ALREADY clips by 1 px in
//                        ENGLISH at v1.4.1 (scrollWidth 63 vs clientWidth 62).
//   .humanize-col-label  no cap at all, and its width DRIVES its grid column,
//                        which is a non-label element the geometry diff
//                        measures. "Colophane" (52.70) would widen the column
//                        from 38 to 52.70 and grow left ON TOP of its
//                        neighbour. "Coloph." (37.53) sits inside the 39.75 px
//                        track and moves nothing — hence two keys for one
//                        English word, decided by geometry.
//   .footer-bar          `min-width: 50px` on the label, so every French
//                        caption at or under 50 px leaves the row untouched.
//                        "Diapason réf." (72.55) would have clipped.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// EMPTY BY CONSTRUCTION, not by omission. A tooltip entry is the one carrying a
// non-empty body `b`; there are none, which is why TIP_BINDINGS may be empty
// without assertion 2 going vacuous.
export const I18N = Object.freeze({});

// ============================================================================
// LABELS — the on-page text
//
// A LABELS entry is {en:{t}, fr:{t, reviewed}} — ONE string, no body, because a
// label is not a tooltip. Rendered into a fixed box on the page rather than
// into a tip that can size itself, which is why every entry below was chosen
// against a measured budget rather than for prose quality alone.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header bar ──────────────────────────────────────────────────────────
    // "Enr." rather than "Enreg." (38.00 px of text) or "Sauver" (39.36): the
    // button is pinned to 54 px so it cannot push the flex:1 preset display,
    // which leaves 28 px of text budget inside its 12 px side padding. English
    // "Save" is 26.55 and "Enr." is 25.09, so both clear it and neither wraps.
    'label.save':      { en: { t: 'Save' },     fr: { t: 'Enr.',     reviewed: false } },
    // "Accordage" (55.09) does not fit the 62 px pin; "Accord" (37.13) does,
    // against English "Tuning" at 37.97.
    'label.tuning':    { en: { t: 'Tuning' },   fr: { t: 'Accord',   reviewed: false } },
    // The settings popover's one row.
    'label.language':  { en: { t: 'Language' }, fr: { t: 'Langue',   reviewed: false } },

    // ── Bow ─────────────────────────────────────────────────────────────────
    'label.bow':       { en: { t: 'Bow' },      fr: { t: 'Archet',   reviewed: false } },
    'label.speed':     { en: { t: 'Speed' },    fr: { t: 'Vitesse',  reviewed: false } },
    'label.pressure':  { en: { t: 'Pressure' }, fr: { t: 'Pression', reviewed: false } },
    'label.position':  { en: { t: 'Position' }, fr: { t: 'Position', reviewed: false, sameAsEn: true } },
    // Colophane is the instrument-maker's word for rosin, not a calque.
    'label.rosin':     { en: { t: 'Rosin' },    fr: { t: 'Colophane', reviewed: false } },
    // THE SAME ENGLISH WORD, A SECOND ANSWER, DECIDED BY GEOMETRY — the O-Bass
    // OUT/OUTPUT precedent. This one labels a Humanize grid COLUMN, whose width
    // is the column's width, and the full "Colophane" would push the column 14
    // px wider than its 39.75 px track and overlap its neighbour. Keyed rather
    // than exempted so the abbreviation stays on the reviewer's list.
    'label.rosinShort': { en: { t: 'Rosin' },   fr: { t: 'Coloph.',  reviewed: false } },
    'label.noise':     { en: { t: 'Noise' },    fr: { t: 'Bruit',    reviewed: false } },
    // "Raideur du crin" is bow-hair stiffness; the English is already
    // abbreviated ("Hair Stiff.") and the French is abbreviated to match,
    // because the unabbreviated form measures 63 px in a 62 px box.
    'label.hairStiff': { en: { t: 'Hair Stiff.' }, fr: { t: 'Raid. crin', reviewed: false } },

    // ── Humanize ────────────────────────────────────────────────────────────
    'label.humanize':  { en: { t: 'Humanize' }, fr: { t: 'Humanisation', reviewed: false } },
    'label.amt':       { en: { t: 'Amt' },      fr: { t: 'Qté',      reviewed: false } },
    // "Fréq." and not "Vitesse": this knob sets the drift rate in Hz and sits
    // directly under a column captioned "Vitesse" (Speed). Two adjacent knobs
    // both reading VITESSE would be a translation that loses information the
    // English carries.
    'label.rate':      { en: { t: 'Rate' },     fr: { t: 'Fréq.',    reviewed: false } },

    // ── Visualisation tabs ──────────────────────────────────────────────────
    // The tab buttons are `flex: 1` in a 498 px bar, so each box is 166 px in
    // both languages by construction. Only the TEXT can misbehave, and the
    // longest French here is 133.19 px — one line, 32.8 px of slack.
    'label.vizBowString':     { en: { t: 'Bow-String' },    fr: { t: 'Archet-corde', reviewed: false } },
    'label.vizBodySpectrum':  { en: { t: 'Body Spectrum' }, fr: { t: 'Spectre de la caisse', reviewed: false } },
    // Schelleng is the physicist; a name is never translated.
    'label.vizSchelleng':     { en: { t: 'Schelleng' },     fr: { t: 'Schelleng', reviewed: false, sameAsEn: true } },

    // ── Impossible physics ──────────────────────────────────────────────────
    // .impossible-label is `writing-mode: vertical-rl`, so its LENGTH is its
    // HEIGHT. The word is identical in both languages, which is the only reason
    // that box needs no pin.
    'label.impossible':   { en: { t: 'Impossible' },    fr: { t: 'Impossible', reviewed: false, sameAsEn: true } },
    'label.infSustain':   { en: { t: 'Inf. Sustain' },  fr: { t: 'Tenue inf.', reviewed: false } },
    'label.revFriction':  { en: { t: 'Rev. Friction' }, fr: { t: 'Frict. inv.', reviewed: false } },
    'label.subHarm':      { en: { t: 'Sub Harm.' },     fr: { t: 'Sous-harm.', reviewed: false } },

    // ── Body ────────────────────────────────────────────────────────────────
    // "Caisse" — the soundbox of a bowed string instrument, not "corps".
    'label.body':       { en: { t: 'Body' },       fr: { t: 'Caisse',    reviewed: false } },
    'label.material':   { en: { t: 'Material' },   fr: { t: 'Matière',   reviewed: false } },
    'label.size':       { en: { t: 'Size' },       fr: { t: 'Taille',    reviewed: false } },
    'label.brightness': { en: { t: 'Brightness' }, fr: { t: 'Brillance', reviewed: false } },
    'label.bodyAmt':    { en: { t: 'Body Amt' },   fr: { t: 'Qté caisse', reviewed: false } },

    // ── String ──────────────────────────────────────────────────────────────
    'label.string':     { en: { t: 'String' },     fr: { t: 'Corde',     reviewed: false } },
    'label.gauge':      { en: { t: 'Gauge' },      fr: { t: 'Calibre',   reviewed: false } },

    // ── Sympathetic strings ─────────────────────────────────────────────────
    'label.sympathetic': { en: { t: 'Sympathetic' }, fr: { t: 'Sympathiques', reviewed: false } },
    'label.count':      { en: { t: 'Count' },      fr: { t: 'Nombre',    reviewed: false } },
    'label.amount':     { en: { t: 'Amount' },     fr: { t: 'Quantité',  reviewed: false } },
    'label.decay':      { en: { t: 'Decay' },      fr: { t: 'Déclin',    reviewed: false } },

    // ── Footer ──────────────────────────────────────────────────────────────
    // "Diapason" is the reference pitch itself (le diapason est à 440 Hz), so
    // the qualifier the English needs is carried by the word. "Diapason réf."
    // measures 72.55 px against a 62 px cap and would have been clipped.
    'label.refPitch':   { en: { t: 'Ref Pitch' },  fr: { t: 'Diapason',  reviewed: false } },
    'label.width':      { en: { t: 'Width' },      fr: { t: 'Largeur',   reviewed: false } },
    'label.output':     { en: { t: 'Output' },     fr: { t: 'Sortie',    reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Every one of these is the text of a native title= attribute v1.4.1
    // carried, MOVED not rewritten (contract §4). No hover-help prose is
    // invented here; that is Stage M.
    'aria.presetPrev':   { en: { t: 'Previous Preset' },
                           fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.presetNext':   { en: { t: 'Next Preset' },
                           fr: { t: 'Préréglage suivant', reviewed: false } },
    'aria.presetBrowse': { en: { t: 'Click to browse presets' },
                           fr: { t: 'Cliquer pour parcourir les préréglages', reviewed: false } },
    'aria.settings':     { en: { t: 'Settings' },
                           fr: { t: 'Réglages', reviewed: false } },
    'aria.langSelect':   { en: { t: 'Interface language' },
                           fr: { t: 'Langue de l’interface', reviewed: false } },

    // ── The one string this page writes from script ─────────────────────────
    // Written through setLabel(), so the element becomes a [data-i18n] element
    // from that moment on and the language sweep owns it. A raw literal there
    // is stranded in the previous language the instant the selector fires.
    'ui.tuningPanelFailed': { en: { t: 'Tuning panel failed to load.' },
                              fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, scope]. A SCOPE is required only
// where a string is exempt AND keyed on the same page — the one state in which
// the gate cannot tell a deliberate skip from a label somebody forgot. None of
// the three below is in that state: each of these strings occurs exactly once
// in the served markup and is keyed nowhere, so all three are correctly
// unscoped. Verified by grep, not assumed.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-Bowed',  'the product name, in .plugin-name — a product name is never translated'],
    ['Ouaricon', 'the maker, in .brand-label — a company name is never translated'],
    // The preset manager writes the loaded preset's name into this node at init
    // and on every preset change; "Default" is the placeholder it overwrites on
    // its first pass.
    ['Default',  'a factory preset name — exempt under D-02, because the name IS the JSON '
               + 'filename (OuariconPresetManager.h): a session saved against "Violin" would '
               + 'not resolve "Violon"'],
];

// This plugin has no hover-help. See the header: empty here is the correct
// state, and check-i18n assertion 2 reports it rather than passing silently.
export const TIP_BINDINGS = [];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm
    // today, but the canon is ONE shape across all 43 plugins and this function
    // is not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
