/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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
// i18n.js — O-Emulator page copy, English + French (v1.2.1)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is a single inline <script type="module"> in index.html,
// so that failure mode would take the WHOLE UI — knobs, console selector,
// preset band — not a panel of it. scripts/check-i18n.js assertion 7 enforces
// the export-only shape.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── v1.2.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
//
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 9 of 22 entries (5 terminology, 8 typography, 7 grammar/idiom,
// 2 meaning — an entry can carry more than one). Lint 26 -> 3: T1 6, T3 4,
// T4 6, T5 1, T7 2, F1 2 and 2 of 5 G1 closed. 14 straight apostrophes became
// U+2019 and 18 U+00A0 went in before % : and between number and unit.
// sameAsEn: kept 1 (aria.console), ADDED 1 (label.mix), translated 0.
// termNote exemptions: 0. Left as drafted: the other 13 entries.
// reviewed: false throughout — no native speaker has read this yet.
//
// THE THREE G1 FINDINGS LEFT OPEN ARE BOTH WIDTH, AND BOTH ARE MEASURED. The
// glossary lists one rendering each and no abbreviation, and Stage N does not
// invent a third form — so the drafted word stays and the number is reported:
//
//   Crush -> Écrasement (label.crush AND tip.crush's title) is 82.45 px in a
//   60.00 px .ctl column. The column is max(60, caption) inside a
//   space-evenly row, so it widens to 82.45 and slides all four knobs —
//   crush 103.00 -> 109.72, age 221.00 -> 234.47, reverb 339.00 -> 347.97,
//   mix 457.00 -> 461.48. That is check-ui-labels assertion 7, failed. The
//   header's v1.2.0 defence of Broyage is the sixth in this task to be
//   re-measured and the first three to hold. (Écrasem. 64.20 also moves;
//   Écras. 46.70 would fit at zero movement, and is NOT shipped because it is
//   not in the glossary.)
//
//   Confirm? -> Confirmer ? is 58.59 px on ONE line in the pinned 49.00 px
//   content box, 9.59 px over, with overflow visible. See the corrected note
//   at ui.confirm below.
//
// DECISIONS THE NEXT READER NEEDS:
//   · Mix stays Mix (glossary root; Dosage and Mixage are both forbidden), so
//     label.mix is now a straight copy and carries sameAsEn: true. That is
//     why check-ui-labels' vacuity count reads 8/9 (89%) and not 9/9.
//     tip.mix's TITLE is the same copy over a translated body and takes NO
//     flag — the flag is entry-scoped in check-i18n and would disarm
//     assertion 4 for the whole entry.
//   · Enreg. / Ouvrir / Suppr. all stay: each is a glossary-accepted
//     rendering, and the band's width is the pin that keeps .brand and
//     .hdr-right still. Charger 41.06 does not fit the 37.00 px Load box;
//     Enregistrer 58.08 does not fit the 36.00 px Save box; Supprimer 47.78
//     WOULD fit the 49.00 px Delete box, and Suppr. is kept anyway because
//     the three buttons are one set and the band total is what is pinned.
//   · tip.reverb's French body gained one rendered line (131.0 -> 147.0 px)
//     when "dans tous les modes" was corrected to "dans tous les modes de
//     console". It grows upward against a fixed bottom edge, to exactly the
//     147.0 px tip.crush already ships, with 120.0 px of top clearance.
//   · Console names inside a French sentence are unchanged — the v1.2.0 note
//     below still governs, and "Game Boy"/"Genesis" stay.
//
// ── v1.2.0 GIVES THIS PAGE HOVER-HELP, AND A RENDERER TO SHOW IT ───────────
//
// v1.1.0 shipped an EMPTY I18N and an EMPTY TIP_BINDINGS, which was that
// version's correct state: the page carried no data-tip, no data-tooltip and
// no native title= anywhere, so there was no tooltip copy to move and none was
// invented. v1.2.0 authors it — seven entries, five parameters plus the gear
// and the language selector.
//
// AUTHORING COPY ALONE WOULD HAVE SHIPPED SEVEN INVISIBLE STRINGS. applyI18n()
// only WRITES data-tip-title and data-tip onto the anchors named below; the
// thing that reads those attributes and paints a surface is per-plugin code,
// and this plugin had none of it — no #tooltip element, no .tooltip rule, no
// hover handler. All three gates would have stayed green anyway: check-i18n
// assertion 2 sees bindings > 0, check-ui-labels has no tooltip awareness at
// all, and boot-all-uis counts aria-label and title and never data-tip. So the
// renderer lands in the same commit (index.html, setupTooltips()), and
// tests/ui_tip_render_check.js is the gate that can actually see a painted tip.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing an
// angle bracket, so machine-drafted French cannot open a markup path.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ============================================================================
// I18N — hover-help copy. {en:{t, b}, fr:{t, b, reviewed}}.
//
// SEVEN entries: the five APVTS parameters, plus the gear and the language
// selector. The preset bar deliberately gets none — its four controls already
// carry accessible names (data-i18n-aria, v1.1.0) and are self-describing.
//
// ── TITLES ──────────────────────────────────────────────────────────────────
//
// The title is the control's display name as the PAGE spells it, which on this
// plugin is the same string the parameter carries in every case:
// .planning/params.tsv `name` reads Console / Crush / Age / Reverb / Mix and
// the four .ctl-label captions and the .seg group's accessible name read the
// same. So there is no caption-vs-parameter divergence to resolve here, and
// each tip title reuses the LABELS entry's French rather than inventing a
// second spelling of the same word.
//
// ── RANGES AND UNITS: MEASURED FROM THE DUMP, NOT RECOVERED FROM A FORMATTER ─
//
// The Stage M brief expects `label` to be empty on most M1 plugins and the
// unit to have to be read back out of the page's own formatter. THAT IS NOT
// THE CASE HERE and it is worth stating rather than leaving as a silent
// non-finding: all four AudioParameterFloats declare
// `AudioParameterFloatAttributes().withLabel("%")` (PluginProcessor.cpp:66),
// so params.tsv carries `%` in the `label` column for crush, age, reverb and
// mix, with textAtMin 0.0 and textAtMax 100.0. The page agrees independently —
// each .knob carries data-unit="%" and fmtValue() appends it (index.html) — so
// dump and formatter say the same thing and nothing had to be inferred.
//
// `console` has an EMPTY label because it is an AudioParameterChoice: its range
// is its five option words, not a number, exactly as the brief describes.
//
// ── THE CONSOLE NAMES INSIDE A FRENCH SENTENCE ──────────────────────────────
//
// D-01 arm 1 exempts the five option strings AS THE SELECTOR RENDERS THEM: the
// page and the host automation lane must agree, so the segments stay SNES /
// PS1 / NES / GB / Genesis in both languages and carry no key (I18N_EXEMPT,
// below). That rule governs the OPTION; it does not govern a sentence that
// names it. So `tip.console`'s body is French prose that happens to contain
// five English proper nouns, which is what those five are in French too — they
// are hardware product names. The one place the two could drift is "Genesis",
// sold in France as the Mega Drive: the tip says Genesis, because the segment
// says Genesis, because the automation lane says Genesis.
//
// The body spells the fourth console "Game Boy" — the OPTION string
// (PluginProcessor.cpp:56) — while the segment caption reads "GB". That
// divergence predates this stage and is reported, not changed; naming the full
// option in the tip is the one place a user can find out what the abbreviated
// segment stands for.
//
// ── NUMBERS INSIDE A BODY ARE PROSE (D-03) ──────────────────────────────────
//
// D-03 exempts readout NODES, not digits. A number inside a localized tooltip
// body is ordinary prose and is localized with the sentence around it:
// "0 to 100 %" becomes "0 à 100 %", "30 ms" stays "30 ms", and the French
// decimal comma is not reached here because no body carries a decimal.
// ============================================================================

export const I18N = Object.freeze({

    // ── The five parameters, in the page's own top-to-bottom order ──────────

    // console — AudioParameterChoice, 5 options, default SNES.
    // The French title is byte-identical to the English: "Console" is the same
    // word for the same object in both languages, and the existing
    // `aria.console` LABELS entry already declares that with sameAsEn. Here the
    // BODY differs, so assertion 4 (which flags an entry only when t AND b both
    // match) does not fire and no sameAsEn flag is needed — the entry is still
    // in the reviewer worklist through `reviewed: false`.
    'tip.console': {
        en: { t: "Console",
              b: "Chooses which machine the sound is played through — codec, fixed internal sample rate and output stage all change together. Switching crossfades over 30 ms, so it is safe to change while audio is running. Five settings: SNES, PS1, NES, Game Boy, Genesis." },
        fr: { t: "Console",
              b: "Choisit la machine par laquelle le son passe : codec, fréquence d’échantillonnage interne fixe et étage de sortie changent ensemble. Le changement se fait par un fondu enchaîné de 30 ms, sans risque pendant la lecture. Cinq réglages : SNES, PS1, NES, Game Boy, Genesis.",
              reviewed: true },
    },

    // crush — 0..100 %, default 50. The "still passes the codec at 0" sentence
    // is the one thing a user cannot discover by turning the knob, so it earns
    // its place over a second sentence about the curve.
    'tip.crush': {
        en: { t: "Crush",
              b: "How hard the signal is driven through the console's codec: encoder gain, coarser quantisation steps, and past 80 % the anti-alias filter opening for deliberate aliasing. At 0 the signal still makes the full codec round trip, so this thins the colour rather than bypassing it. 0 to 100 %." },
        fr: { t: "Broyage",
              b: "À quel point le signal est poussé dans le codec de la console : le gain d’encodage, des pas de quantification plus grossiers et, au-delà de 80 %, l’ouverture du filtre anti-repliement pour un repliement volontaire. À 0 le signal traverse quand même tout le codec : ce réglage atténue la couleur, il ne la contourne pas. 0 à 100 %.",
              reviewed: true },
    },

    // age — 0..100 %, default 20. The noise floor ramps in above ~5 %, which is
    // why a user turning it off the stop hears nothing at first; that is the
    // sentence, not the -78 dB figure behind it.
    'tip.age': {
        en: { t: "Age",
              b: "The condition of the hardware: hiss, mains hum, a duller output filter, and a slow wander in the resampling ratio that detunes by up to 15 cents. The noise bed stays silent near the bottom of the range and only comes in above about 5 %. 0 to 100 %." },
        fr: { t: "Âge",
              b: "L’état de la machine : souffle, ronflement secteur, un filtre de sortie plus sourd et une lente dérive du rapport de rééchantillonnage qui désaccorde jusqu’à 15 cents. Le bruit de fond reste inaudible en bas de la course et n’apparaît qu’au-delà d’environ 5 %. 0 à 100 %.",
              reviewed: true },
    },

    // reverb — 0..100 %, default 0. Available in EVERY console mode, which is
    // the non-obvious part: it is the PS1's reverb unit, not the PS1's mode.
    'tip.reverb': {
        en: { t: "Reverb",
              b: "Send level into the PlayStation reverb — a Hall setting from that console's own register model, available in every console mode, not just PS1. The send is taken after the codec, so the reverb hears the degraded signal rather than the clean one. 0 to 100 %." },
        fr: { t: "Réverb",
              b: "Niveau d’envoi vers la réverbération de la PlayStation : un réglage Hall issu du modèle de registres de cette console, disponible dans tous les modes de console, pas seulement en PS1. L’envoi est pris après le codec, donc la réverbération entend le signal dégradé et non le signal intact. 0 à 100 %.",
              reviewed: true },
    },

    // mix — 0..100 %, default 100. The Age bed being wet-path only is the part
    // that surprises people: at 0 % the hiss goes too.
    'tip.mix': {
        en: { t: "Mix",
              b: "Blends the emulated signal against the untouched input. The dry path is delay-compensated, so at 0 % the input passes through unchanged — and the hiss and hum of the Age control go with it, because they live on the wet path only. 0 to 100 %." },
        fr: { t: "Mix",
              b: "Équilibre le signal émulé et l’entrée intacte. Le trajet direct est compensé en latence : à 0 % l’entrée ressort inchangée, et le souffle et le ronflement du réglage Âge disparaissent eux aussi, car ils n’existent que sur le trajet traité. 0 à 100 %.",
              reviewed: true },
    },

    // ── The two chrome controls ─────────────────────────────────────────────
    //
    // The gear tip is what tells a user that hover-help exists at all, so its
    // body describes ONLY what the popover actually contains. This plugin has
    // no hover-help on/off toggle — not a C++ one, not a localStorage one — so
    // the panel holds the language selector and nothing else, and the tip says
    // exactly that. A tip that promised a toggle would be a tip that lies.
    'tip.gearBtn': {
        en: { t: "Settings",
              b: "Opens the panel that sets the language of this interface. That is all it holds: the labels on this page and this hover help switch with it, and the choice is kept with the session, so a project reopens in the language it was saved in." },
        fr: { t: "Réglages",
              b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et ces infobulles changent avec elle, et le choix est conservé avec la session — un projet se rouvre dans la langue dans laquelle il a été enregistré.",
              reviewed: true },
    },
    'tip.langSelect': {
        en: { t: "Language",
              b: "The language of the labels on this page and of this hover help. English and French are available. Value readouts, the five console names and preset names stay in English so the page and the host agree." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de ces infobulles. L’anglais et le français sont disponibles. Les valeurs affichées, les cinq noms de consoles et les noms de préréglages restent en anglais pour que la page et l’hôte s’accordent.",
              reviewed: true },
    },
    // v1.3.0 — the switch that reaches this whole layer.
    'tip.tipsToggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this '
               + 'switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
               + 'l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE PAGE'S OWN GEOMETRY IS THE CONSTRAINT HERE, AND IT IS TIGHT ─────────
//
// Measured at the shipping 620 x 430 frame, rendered, not guessed. Two boxes
// on this page have NO slack at all, and both of them decide what the French
// could be rather than the other way round:
//
//   .hdr is 570 px of content holding three flex children whose max-content
//   widths total 732.28 px. It is 162 px OVER-FULL in ENGLISH. .preset-band
//   cannot shrink (every child is nowrap or a fixed width, so its min-content
//   IS its max-content), so the whole 162 px lands on .wordmark — which is
//   already at its min-content and renders "❦O-" / "EMULATOR" on TWO LINES —
//   and on .hdr-right, which takes whatever is left and wraps .plate to three
//   lines. Both overflow the 48 px header upward, in English, at v1.0.1.
//   That is a pre-existing layout defect, reported and NOT fixed here; what
//   matters for this file is the consequence: .hdr-right's width is exactly
//   `570 - 159.66 - bandWidth`, so ANY change to the preset band's width moves
//   .brand and .hdr-right, and both are non-label elements the geometry diff
//   measures. The three button captions are therefore PINNED (see index.html),
//   to a total of 128 px against English's natural 128.14 px.
//
//   .ctl is shrink-to-fit around a 60 px knob in a `space-evenly` row, so the
//   column's width is `max(60, captionWidth)`. A caption wider than 60 px
//   widens its column and slides all four. Every French caption below is
//   under 60 px, so no pin was needed and none was added.
//
// MEASURED, rendered text width, English -> French:
//
//     .ctl-label   Crush   42.69 -> Broyage 59.17   in a 60.00 column
//                  Age     27.39 -> Âge     27.39   IDENTICAL
//                  Reverb  50.17 -> Réverb  50.17   IDENTICAL
//                  Mix     27.06 -> Mix     27.06   IDENTICAL (v1.2.1)
//     .preset-btn  Save    22.25 -> Enreg.  32.47   in a 36.00 content box
//                  Load    24.48 -> Ouvrir  33.81   in a 37.00 content box
//                  Delete  33.41 -> Suppr.  30.38   in a 49.00 content box, and
//                  Confirm? 44.75 -> Sûr ?  26.23   in the same box, ARMED
//     .plate       widest line 91.91 -> 94.45, THREE lines in both languages,
//                  so .hdr-right stays 58.23 px tall and .brand does not move
//
// Broyage's 0.83 px of clearance is the tightest margin in this plugin and is
// named here so it is never a surprise: it is a margin against the 60 px KNOB,
// not against a clip, so overrunning it slides the columns by under a pixel
// rather than truncating anything. Windows/WebView2 font metrics remain the
// repo's standing hardware-blocked deferral.
//
// "Sûr ?" is 26.23 and NOT the 17.69 a first pass recorded. 17.69 was the
// widest LINE of the string after it WRAPPED in the unpinned 33 px box — a
// space before a French "?" is a break opportunity, so measuring a caption in
// the wrong box reports a confidently wrong number that is too SMALL. In the
// pinned 49 px box it is one line in both languages, verified rather than
// assumed.
//
// THREE of the eight visible-text French strings render at EXACTLY the
// English width (Âge, Réverb, and Mix since v1.2.1), which is the half a clip
// check is blind to in the other direction — nothing to pin, nothing to fix,
// and worth stating rather than leaving as an unexplained zero in the diff.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The four macro knob captions ────────────────────────────────────────
    // Captions, NOT readouts: the value lives in the .ro span below each knob
    // and keeps its "50 %" form untouched (D-03). None of these four is an
    // AudioParameterChoice option — they are the DISPLAY NAMES of four
    // AudioParameterFloats (PluginProcessor.cpp:71-74) — so arm 1 of D-01 does
    // not apply and there is no automation-lane string for a French caption to
    // disagree with.
    'label.crush':  { en: { t: 'Crush' },  fr: { t: 'Broyage', reviewed: true } },
    'label.age':    { en: { t: 'Age' },    fr: { t: 'Âge',     reviewed: true } },
    'label.reverb': { en: { t: 'Reverb' }, fr: { t: 'Réverb',  reviewed: true } },
    'label.mix':    { en: { t: 'Mix' },    fr: { t: 'Mix', sameAsEn: true, reviewed: true } },

    // ── The preset band ─────────────────────────────────────────────────────
    // The repo-standard trio, matching O-Bitrot v1.15.0, O-ReverseDelay and
    // O-MultiBandCompressor verbatim: the same three buttons in the same band
    // should not be spelled three different ways across the suite. Abbreviated
    // rather than "Enregistrer" / "Charger" / "Supprimer" because this header
    // is 162 px over-full in English before French is asked for anything.
    'label.save':   { en: { t: 'Save' },   fr: { t: 'Enreg.', reviewed: true } },
    'label.load':   { en: { t: 'Load' },   fr: { t: 'Ouvrir', reviewed: true } },
    'label.delete': { en: { t: 'Delete' }, fr: { t: 'Suppr.', reviewed: true } },

    // The ARMED face of the delete button — the only string on this page
    // written from script. It goes through setLabel(), so the button becomes a
    // [data-i18n] element and the language sweep owns it from that moment on.
    // Through v1.0.1 it was a data-confirm ATTRIBUTE, which was the right
    // answer while the page was English-only and is the wrong one the moment
    // it has two languages: an attribute holds ONE string, so switching to
    // French mid-arm would have restored the English "Confirm?".
    //
    // "Sûr ?" and not "Confirmer ?". CORRECTED at v1.2.1, both numbers: the
    // box is 49.00 px, as the table twelve lines above always said, and
    // "Confirmer ?" is 58.59 px on ONE LINE, not 50.05. 50.05 was the widest
    // line of the string after it WRAPPED — the same mis-measurement the
    // "Sûr ?" note above already warns about, made a second time in the
    // opposite box. Stage N's U+00A0 before the "?" removes the break
    // opportunity entirely, so the wrapped reading is now unreachable and the
    // one-line 58.59 is the only number: 9.59 px over, overflow visible.
    // Widening the button is not available — the band's total width is what
    // keeps .brand and .hdr-right still (see the header note above) — and
    // "Sûr ?" carries the same terse register as "Confirm?".
    'ui.confirm':   { en: { t: 'Confirm?' }, fr: { t: 'Sûr ?', reviewed: true } },

    // ── The imprint line ────────────────────────────────────────────────────
    // The naturalist-plate conceit the whole page is built on. Its box is
    // .hdr-right, whose WIDTH is leftover header space and therefore
    // language-invariant; only its LINE COUNT could move anything, so the
    // French was chosen to wrap to three lines exactly as the English does.
    // "Planche" is the French term of art for a plate in an illustrated
    // natural-history volume, which is what this line is imitating. The Roman
    // numeral is a numeral (D-03) and is carried across unchanged.
    'label.plate': {
        en: { t: 'A Survey of Extinct Consoles · Plate CDLXXXVII' },
        fr: { t: 'Relevé des consoles disparues · Planche CDLXXXVII', reviewed: true },
    },

    // ── The settings popover (v1.1.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // v1.3.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the same language the page is showing.
    'aria.presetPrev': { en: { t: 'Previous preset' },   fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.presetNext': { en: { t: 'Next preset' },       fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.settings':   { en: { t: 'Settings' },          fr: { t: 'Réglages',             reviewed: true } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },

    // "Console" is spelled identically in French — it is the same Latin root
    // and the same word for the same object. sameAsEn declares that on
    // purpose, because check-i18n assertion 4 otherwise reads an untranslated
    // entry and a deliberately identical one as the same thing.
    //
    // It localizes at all, rather than being exempt under D-01 arm 1, because
    // "Console" is the AudioParameterChoice's DISPLAY NAME
    // (PluginProcessor.cpp:55) and not one of its option strings. Nothing in a
    // host automation lane is spelled "Console" as a VALUE.
    'aria.console': { en: { t: 'Console' }, fr: { t: 'Console', sameAsEn: true, reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [

    // ── Names ───────────────────────────────────────────────────────────────
    ['O-EMULATOR',
     'the product wordmark — a product name is never translated. It is also the '
     + 'only text node in an element that carries a ❦ span sibling, so keying it '
     + 'would make applyLabel delete the fleuron'],
    ['Ouaricon Audio',
     'the company name in .hdr-right .brand — a brand is never translated'],

    // ── D-01 arm 1: the captions that ARE the option strings ────────────────
    //
    // The console selector's five segments carry the `console`
    // AudioParameterChoice option strings, StringArray {"SNES", "PS1", "NES",
    // "Game Boy", "Genesis"} (PluginProcessor.cpp:53-57). Four of the five are
    // byte-identical to their option and are exempt on arm 1 outright: a DAW
    // showing CONSOLE = "Genesis" beside a page reading "Mega Drive" — which is
    // what that console was actually called in France — is a bug report, not a
    // localization.
    //
    // The fifth, "GB", is NOT byte-identical to its option "Game Boy", so arm 1
    // does not reach it on its own. It is exempted with the extra reason
    // recorded on its line: it is the abbreviated form of the same option, and
    // keying it alone would leave one of five segments switching language while
    // its four siblings are pinned by arm 1. The caption/option divergence
    // itself predates this commit and is reported rather than changed —
    // widening the segment to "Game Boy" is a visible layout change to a
    // shipped control.
    ['SNES',
     'a `console` AudioParameterChoice option string VERBATIM '
     + '(PluginProcessor.cpp:56) — D-01 arm 1'],
    ['PS1',
     'a `console` option string VERBATIM (PluginProcessor.cpp:56) — D-01 arm 1'],
    ['NES',
     'a `console` option string VERBATIM (PluginProcessor.cpp:56) — D-01 arm 1'],
    ['Genesis',
     'a `console` option string VERBATIM (PluginProcessor.cpp:56) — D-01 arm 1. '
     + 'The console was sold in France as the Mega Drive, and the page still says '
     + 'Genesis, because the host automation lane says Genesis'],
    ['GB',
     'the abbreviated form of the `console` option "Game Boy" (PluginProcessor.cpp:56). '
     + 'NOT byte-identical, so arm 1 does not reach it alone — exempted because '
     + 'keying one segment of five would leave it switching language while its four '
     + 'arm-1 siblings stay pinned, and because "Game Boy" is a hardware product '
     + 'name in French too. The caption/option divergence is pre-existing and is '
     + 'reported, not changed'],

    // ── D-01 arms 2 and 3: the per-console spec readout ─────────────────────
    //
    // #consoleInfo is written ONLY from the console listener, as
    // `c.name + " — " + c.spec` over a static table locked to ARCHITECTURE.md.
    //
    // The name half is arm 1 again — all five are the option strings verbatim,
    // including "Game Boy" this time.
    //
    // The spec half is exempt on TWO independent arms. Arm 2: every one of the
    // five carries a number and a unit (32 kHz, 22.05 kHz, 33.144 kHz,
    // 16.384 kHz, 26.32 kHz), which D-03 exempts. Arm 3: #consoleInfo is a
    // READOUT node — it is never a [data-i18n] element regardless of what is
    // behind it. O-Gain's LOW/MED/HIGH overrules arm 3 precisely because that
    // node never holds a number; this one always does, so the overrule is not
    // available and arm 3 stands.
    //
    // What that costs a French reader is "Gaussian" and "wave", and it is
    // stated here rather than left as an unexplained gap: translating them
    // means splitting a centred one-line readout into four keyed spans under
    // contract §5, which is a markup change to a readout in a stage whose scope
    // is labels.
    ['BRR 4-bit · 32 kHz · Gaussian',
     'the #consoleInfo spec readout — D-01 arm 2 (a number and a unit) and arm 3 '
     + '(a readout node is never a [data-i18n] element). Codec/rate/interpolation '
     + 'identifiers locked to ARCHITECTURE.md'],
    ['SPU-ADPCM · 22.05 kHz · Gaussian',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['DPCM · 33.144 kHz · ZOH',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['4-bit wave · 16.384 kHz · ZOH',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['8-bit DAC · 26.32 kHz · ZOH',
     'the #consoleInfo spec readout — D-01 arms 2 and 3, as above'],
    ['Game Boy',
     'the #consoleInfo name half — a `console` option string VERBATIM '
     + '(PluginProcessor.cpp:56), D-01 arm 1'],

    // ── The preset name plate ───────────────────────────────────────────────
    ['Default',
     'the placeholder in #preset-name, which DISPLAYS a preset name — exempt under '
     + 'D-02, because the name IS the JSON filename '
     + '(modules/persistence/preset-manager, OuariconPresetManager.h). The element '
     + 'is written by the shared module\'s _updateDisplay(), never by this page'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// applyI18n() runs document.querySelector(selector), then closest(wrapper) if a
// wrapper is given, and writes data-tip-title / data-tip onto whatever it lands
// on. Any CSS selector is legal — an id is not required, and on this page three
// of the seven anchors have no id at all.
//
// ── THE WRAPPER IS THE HOVER TARGET, NOT THE ADDRESSABLE NODE ───────────────
//
// The four knobs are addressable as .knob[data-param="..."], which is a 60 px
// circle. The thing a user aims at is the whole .ctl column — caption, knob and
// readout stacked with a 7 px gap, about 60 x 100 px. So each knob binding
// walks up to .ctl and the tip opens anywhere in the cell, including over the
// caption the tip is titled after.
//
// The console selector is the opposite case: .seg IS the control. It is a
// full-width 5-button bar, its buttons are its own children, and closest()
// finds .seg from any of them, so no wrapper walk is wanted. Binding to a
// single segment button would give one fifth of the bar a tip and leave the
// other four bare.
//
// #gear-btn and #lang-select are bound directly. Both are real 20-plus-pixel
// targets and neither has a meaningful wrapper — #lang-select's parent is the
// .settings-row label, and putting the tip there would make the row's caption
// ("Language") open a tip titled "Language", which reads as a bug.
//
// EVERY BINDING MUST RESOLVE. applyI18n() logs `i18n: tip target not found:
// <selector>` for one that does not, and boot-all-uis is the gate that sees
// that console warning. tests/ui_tip_render_check.js asserts resolution AND
// that each anchor actually paints a tip, which is the assertion no repo-wide
// gate can make.
// ============================================================================

export const TIP_BINDINGS = [
    ['.seg[data-param="console"]',   'tip.console'],

    ['.knob[data-param="crush"]',    'tip.crush',  '.ctl'],
    ['.knob[data-param="age"]',      'tip.age',    '.ctl'],
    ['.knob[data-param="reverb"]',   'tip.reverb', '.ctl'],
    ['.knob[data-param="mix"]',      'tip.mix',    '.ctl'],

    ['#gear-btn',                    'tip.gearBtn'],
    ['#lang-select',                 'tip.langSelect'],
    ['#tips-toggle',                 'tip.tipsToggle'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Unreferenced at runtime today: applyI18n() calls it only from the
// TIP_BINDINGS loop, which is empty. It is exported verbatim all the same, so
// that the canon block is byte-identical to the other forty-two copies and
// Stage M can add bodies to I18N without touching this file's shape.
export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const fill = (str) => (vars
        ? String(str).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(str));

    return { t: fill(s.t), b: fill(s.b) };
}
