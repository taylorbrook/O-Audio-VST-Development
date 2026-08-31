/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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
// i18n.js — O-Freeze page labels and hover-help, English + French (v2.2.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is an inline <script type="module"> in index.html — there
// is no js/app.js and no stylesheet file — so that failure mode would take the
// WHOLE UI, not a panel of it. scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── v2.2.0: THIS PLUGIN NOW HAS HOVER-HELP ───────────────────────────
//
// v2.1.0 shipped with I18N and TIP_BINDINGS both empty, which was that
// version's correct state: the page carried no data-tip, no data-tooltip and
// no native title= anywhere, so there was nothing to MOVE and Stage K invented
// nothing. Stage M authors that copy.
//
// FOURTEEN entries: one per parameter in .planning/params.tsv — all twelve
// have a control on this page — plus the gear and the language selector.
//
// AUTHORING THE COPY DOES NOT MAKE IT VISIBLE. applyI18n() writes data-tip-title
// and data-tip ATTRIBUTES onto the anchors named in TIP_BINDINGS and stops
// there; the thing that reads those attributes and paints a surface is
// per-plugin code, and on this plugin it did not exist before v2.2.0. Bodies
// with no renderer would have shipped 28 invisible strings under a green gate
// — check-i18n cannot see a rendered tip, check-ui-labels has no tooltip
// awareness at all, and boot-all-uis counts aria-label and title but never
// data-tip. The renderer lives in the inline module in index.html (setupTooltips)
// and tests/ui_tip_render_check.js is the gate that actually sees it.
//
// THE BODIES ARE PROSE, AND PROSE TAKES FRENCH CONVENTION. The decimal comma
// (0,01 à 10 Hz), a space before %, U+2212 for the minus. The READOUT keeps its
// point — `0.50 Hz` renders identically in both languages — because D-03 exempts
// the readout NODE, not the sentence describing it. They differ on purpose.
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
// I18N — hover-help copy. {en:{t,b}, fr:{t,b,reviewed}}.
//
// TITLES MIRROR THE VISIBLE CAPTION, NOT THE PARAMETER NAME, wherever the two
// disagree. The dump calls GRAIN_SIZE "Grain Size", LFO_RATE "LFO Rate" and
// LFO_DEPTH "LFO Depth"; the page says Size, Rate and Depth, inside a group
// already captioned DRIFT LFO. The user is reading the page, not the automation
// lane, and a tip headed "LFO Rate" over a knob captioned RATE reads as a
// second control.
//
// RANGES COME FROM .planning/params.tsv, THE RUNTIME WALK — not from a regex
// over createParameterLayout(). Eleven of the twelve carry a real `label`
// column (dB, %, ms, Hz, cents) or are discrete and take their option words as
// their range. GRAIN_COUNT is the ONE parameter with an empty label AND a
// numeric range: index.html renders it `${Math.round(val)}`, a bare count with
// no unit at all, so the body says "2 to 32 grains" and invents nothing.
//
// DETUNE IS THE ONE PLACE THE DUMP AND THE PAGE DISAGREE ABOUT A UNIT. The dump
// says `cents`; the readout formatter abbreviates it to `ct`. The body spells
// it out — a tooltip is prose and has room — and the readout is untouched.
//
// THE ENGLISH OPTION WORDS INSIDE A FRENCH BODY ARE DELIBERATE. Manual,
// Threshold, Sine, Triangle and Random are AudioParameterChoice option strings
// (PluginProcessor.cpp:55, :108). The strings themselves stay English so the
// page and the host automation lane agree about the same setting — D-01 arm 1,
// the Stage K decision this plugin already carries — while the SENTENCE naming
// them is French. The two rules do not conflict: one governs the option, the
// other the prose around it.
// ============================================================================

export const I18N = Object.freeze({

    // ── The centre button ───────────────────────────────────────────────────
    // Anchored on #freeze-button, the 140 px box, not on #freeze-label: the
    // caption is position:absolute with pointer-events:none, so it could never
    // hold a tip open on its own. AudioParameterBool, range Off / On.
    'tip.freeze': {
        en: { t: 'Freeze',
              b: 'Captures the incoming audio into a grain buffer and holds it as a sustained texture. Press again to release it and let the input through. Off or On; in Threshold mode the input level drives it instead.' },
        fr: { t: 'Geler',
              b: 'Capture l’audio entrant dans une mémoire de grains et le maintient comme une texture soutenue. Appuyer de nouveau pour relâcher et laisser passer l’entrée. Désactivé ou activé ; en mode Threshold, c’est le niveau d’entrée qui commande.',
              reviewed: false },
    },

    // ── The reverse pill ────────────────────────────────────────────────────
    // AudioParameterBool, range Off / On. The title matches label.reverse so the
    // pill and its tip say the same word in both languages.
    'tip.reverse': {
        en: { t: 'Reverse',
              b: 'Plays every grain backwards, which softens transients and turns the frozen texture inside out. It changes the character, not the pitch. Off or On.' },
        fr: { t: 'Inverse',
              b: 'Joue chaque grain à l’envers, ce qui adoucit les transitoires et retourne la texture gelée. Cela change le caractère, pas la hauteur. Désactivé ou activé.',
              reviewed: false },
    },

    // ── The mode toggle ─────────────────────────────────────────────────────
    // Bound to #mode-toggle, which has NO caption of its own on the page — the
    // two buttons inside it are the option strings. So the title is the
    // parameter's own name from the dump, and it happens to be spelled
    // identically in French. The BODY differs, so assertion 4's passthrough
    // guard is not tripped and sameAsEn is neither needed nor claimed.
    'tip.mode': {
        en: { t: 'Mode',
              b: 'Chooses what starts the freeze. Manual arms the button in the centre of the panel; Threshold hands the decision to the input level and the knob beside it. Manual or Threshold.' },
        fr: { t: 'Mode',
              b: 'Choisit ce qui déclenche le gel. Manual arme le bouton au centre du panneau ; Threshold confie la décision au niveau d’entrée et au potentiomètre voisin. Manual ou Threshold.',
              reviewed: false },
    },

    // ── The six main knobs ──────────────────────────────────────────────────
    // THRESHOLD's title stays English in French, and that is the same decision
    // I18N_EXEMPT records for the caption below it: the word is byte-identical
    // to a MODE option string, the knob and the mode button name the SAME
    // setting, and a tip headed SEUIL floating over a knob captioned THRESHOLD
    // would describe one control as two. Only the body is French.
    'tip.threshold': {
        en: { t: 'Threshold',
              b: 'The input level at which the freeze engages by itself, read only in Threshold mode. Lower it to catch quieter material; raise it so only peaks trigger. −60 to 0 dB.' },
        fr: { t: 'Threshold',
              b: 'Le niveau d’entrée à partir duquel le gel s’enclenche de lui-même, lu uniquement en mode Threshold. L’abaisser pour capter des passages plus discrets, le relever pour ne déclencher que sur les crêtes. De −60 à 0 dB.',
              reviewed: false },
    },

    'tip.drift': {
        en: { t: 'Drift',
              b: 'Spreads the grain read positions apart so the frozen texture wanders instead of looping in place. A little removes the static ringing; a lot smears it into a cloud. 0 to 100%.' },
        fr: { t: 'Dérive',
              b: 'Écarte les positions de lecture des grains pour que la texture gelée se déplace au lieu de boucler sur place. Un peu suffit à supprimer la résonance statique ; beaucoup l’étale en nappe. De 0 à 100 %.',
              reviewed: false },
    },

    'tip.size': {
        en: { t: 'Size',
              b: 'The length of each grain taken from the frozen buffer. Short grains give a granular, stuttering texture; long ones keep the source recognisable. 50 to 1000 ms.' },
        fr: { t: 'Taille',
              b: 'La longueur de chaque grain prélevé dans la mémoire gelée. Des grains courts donnent une texture granuleuse et hachée ; des grains longs gardent la source reconnaissable. De 50 à 1000 ms.',
              reviewed: false },
    },

    // The one parameter whose unit had to be recovered from the page rather
    // than from the dump: params.tsv leaves `label` empty and the readout is
    // `${Math.round(val)}` — a bare integer. The range is a count of grains.
    'tip.grains': {
        en: { t: 'Grains',
              b: 'How many grains play at once. A low count sounds sparse and rhythmic; a high count blends into a continuous pad and costs more CPU. 2 to 32 grains.' },
        fr: { t: 'Grains',
              b: 'Le nombre de grains joués simultanément. Un faible nombre donne un rendu clairsemé et rythmique ; un nombre élevé se fond en nappe continue et coûte plus de CPU. De 2 à 32 grains.',
              reviewed: false },
    },

    'tip.detune': {
        en: { t: 'Detune',
              b: 'Spreads the pitch of individual grains across a range, thickening the freeze into a chorus. Small amounts add motion; large ones detune the texture audibly. 0 to 50 cents.' },
        fr: { t: 'Écart',
              b: 'Étale la hauteur des grains sur une plage, ce qui épaissit le gel en un effet de chœur. De faibles valeurs ajoutent du mouvement ; de fortes valeurs désaccordent la texture de façon audible. De 0 à 50 cents.',
              reviewed: false },
    },

    'tip.mix': {
        en: { t: 'Mix',
              b: 'Balances the frozen texture against the untreated input. At 100% only the freeze is heard; pull it back to keep the live signal underneath. 0 to 100%.' },
        fr: { t: 'Dosage',
              b: 'Équilibre la texture gelée et le signal d’entrée non traité. À 100 %, seul le gel est audible ; en réduire la valeur laisse passer le signal direct. De 0 à 100 %.',
              reviewed: false },
    },

    // ── The Drift LFO group ─────────────────────────────────────────────────
    'tip.rate': {
        en: { t: 'Rate',
              b: 'The speed of the LFO that modulates Drift. Slow settings breathe under a pad; fast ones flutter the grain positions. 0.01 to 10 Hz.' },
        fr: { t: 'Vitesse',
              b: 'La vitesse du LFO qui module la dérive. Les réglages lents font respirer une nappe ; les rapides font trembler la position des grains. De 0,01 à 10 Hz.',
              reviewed: false },
    },

    // The title is PROF., the abbreviation the caption already carries: the
    // knob has a hard 60.00 px budget and PROFONDEUR renders at 87.30. The tip
    // has no such budget, so the BODY spells the idea out instead.
    'tip.depth': {
        en: { t: 'Depth',
              b: 'How much of the Drift setting the LFO actually sweeps. At zero the LFO does nothing, however fast it runs. 0 to 100%.' },
        fr: { t: 'Prof.',
              b: 'La part de la dérive que le LFO balaie réellement. À zéro, le LFO n’a aucun effet, quelle que soit sa vitesse. De 0 à 100 %.',
              reviewed: false },
    },

    // Bound to #lfo-shape-toggle, which wraps the SHAPE caption and the three
    // option buttons together — the cell the user aims at. The three buttons
    // read Sin / Tri / Rnd, which are abbreviations and NOT the option strings,
    // so they localize; the option strings themselves are Sine / Triangle /
    // Random and stay English here for the automation lane.
    'tip.shape': {
        en: { t: 'Shape',
              b: 'The waveform the drift LFO follows. Sine glides, Triangle turns sharply at each end, Random steps to a new value every cycle. Sine, Triangle or Random.' },
        fr: { t: 'Forme',
              b: 'La forme d’onde suivie par le LFO de dérive. Sine glisse, Triangle change de sens brusquement aux extrêmes, Random saute à une nouvelle valeur à chaque cycle. Sine, Triangle ou Random.',
              reviewed: false },
    },

    // ── The chrome ──────────────────────────────────────────────────────────
    // The gear tip is what tells a user hover-help exists at all, so its body
    // describes ONLY what the popover actually contains. O-Tapestop's wording
    // promises a hover-help toggle; this plugin has none, and a tip that lies
    // is worse than no tip.
    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens the settings panel. It holds one control, the language of the interface, and the choice is remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages. Il contient une seule commande, la langue de l’interface, et le choix est conservé avec la session.',
              reviewed: false },
    },

    // Bound BARE, not through a wrapper. #gear-btn and #lang-select share
    // #settings-cluster, so a closest() walk would make hovering the selector
    // resolve to the gear's tip (the O-Comp finding). Both anchors are their
    // own hover cell here.
    'tip.langSelect': {
        en: { t: 'Language',
              b: 'The language of these hover descriptions and of the labels on the page. English and French are available; the value readouts stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de ces descriptions au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées restent en anglais.',
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── THE FOUR READOUT NODES, AND WHY NONE OF THEM IS HERE ────────────────────
//
// The extractor classifies four of this page's twenty-two text nodes as
// READOUT: `-40.0 dB` (#threshold-knob), `400 ms` (#grain-size-knob),
// `5.0 ct` (#detune-knob) and `0.50 Hz` (#lfo-rate-knob). Every one is a
// `div.knob-value`, and every one is overwritten on the first frame by
// setupKnob()'s formatValue() from state.getScaledValue(). They are exempt
// THREE TIMES OVER and it is worth naming which arm each rests on, because the
// arms disagree on other plugins:
//
//   arm 2 (D-03)  each is a number plus a unit symbol — dB, ms, ct, Hz — and a
//                 unit symbol is language-neutral.
//   arm 3         each is a READOUT NODE, and a readout node is never a
//                 [data-i18n] element regardless of the parameter behind it.
//                 This is the arm that would still exempt them if a future
//                 version made one of them wear a word instead of a number:
//                 keying it would make the element enter and leave the label
//                 sweep as the knob turns (the O-Marimba finding).
//   contract §5   a readout and a label share no node on this page. `.knob-label`
//                 and `.knob-value` are already two sibling divs in every one of
//                 the eight knobs, so NO SPLIT WAS NEEDED and no markup was
//                 restructured for one. The split that §5 authorises is the
//                 change that can move geometry; not making it is why this
//                 plugin's English before/after diff is clean.
//
// The two remaining `.knob-value` nodes (`0%`, `8`, `100%`, `50%`) are bare
// numbers and do not even reach the READOUT class.
//
// ── GEOMETRY: EVERY FRENCH STRING WAS CHOSEN AGAINST A MEASURED BUDGET ──────
//
// The six main knobs live in `#knobs-container`, a 530 px `justify-content:
// space-around` flex row. Each `.knob` shrink-wraps to max(60 px visual,
// 60 px `.knob-value` min-width, LABEL WIDTH). So a label under 60 px is FREE —
// the item stays 60 px and nothing in the row moves — and a label over 60 px
// widens its item, changes the row's total, and redistributes the slack across
// all six. That 60.00 px is a hard budget, not a guideline. Measured at
// 550 x 530, rendered text width:
//
//     DRIFT      35.59 -> DÉRIVE  44.14    budget 60, spare 15.9
//     SIZE       26.06 -> TAILLE  41.41    spare 18.6
//     GRAINS     44.16 -> GRAINS  44.16    identical word — sameAsEn
//     DETUNE     47.98 -> ÉCART   37.88    SHRANK, and see the note below
//     MIX        23.28 -> DOSAGE  47.05    spare 13.0
//
// The two LFO knobs are `.knob-small`: a 42 px visual, but `.knob-value` still
// carries `min-width: 60px`, so the budget is the same 60.00 px.
//
//     RATE       33.09 -> VITESSE 52.14    spare 7.9
//     DEPTH      42.91 -> PROF.   37.17    SHRANK, and see the note below
//
// Three of seven SHRINK rather than grow. That is the half a clip check is
// blind to, and the half Stage J found four times in twelve.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The freeze button ───────────────────────────────────────────────────
    // #freeze-label is `position: absolute` + `translate(-50%, -50%)` inside a
    // fixed 140 px button, with `pointer-events: none`. Its width is therefore
    // free: it can neither push a sibling nor be pushed. GELER 69.72 against
    // FREEZE's 81.16, comfortably inside the 140 px shape.
    //
    // The VERB, not the noun. This is the button you press to freeze the
    // buffer, and the parameter behind it is an AudioParameterBool named
    // "Freeze" (PluginProcessor.cpp:38-41) — a bool has no option strings, so
    // there is no automation-lane spelling for the caption to disagree with and
    // D-01 arm 1 does not apply.
    'label.freeze': { en: { t: 'Freeze' }, fr: { t: 'GELER', reviewed: false } },

    // ── The reverse toggle ──────────────────────────────────────────────────
    // Also an AudioParameterBool ("Reverse", PluginProcessor.cpp:112-115), so
    // arm 1 does not apply here either. INVERSE is 55.33 against REVERSE's
    // 57.50 — a 2.17 px SHRINK, which would have pulled #reverse-container (a
    // non-label element, centred by translateX(-50%)) in by 1.09 px and failed
    // assertion 7. The `min-width` pin in index.html holds the pill at its
    // English 95.5 px; see the comment there, and the negative control in the
    // commit message.
    'label.reverse': { en: { t: 'Reverse' }, fr: { t: 'INVERSE', reviewed: false } },

    // ── The six main knob captions ──────────────────────────────────────────
    // Each is the plugin's own caption for a FLOAT or INT parameter, not a
    // choice option, so all six are localizable under D-01 arm 1 and none is a
    // readout node under arm 3. THRESHOLD is the exception and is EXEMPT — see
    // I18N_EXEMPT, where the reason is the whole judgement call on this plugin.
    'label.drift':  { en: { t: 'Drift' },  fr: { t: 'DÉRIVE', reviewed: false } },
    'label.size':   { en: { t: 'Size' },   fr: { t: 'TAILLE', reviewed: false } },

    // GRAINS is the same word in both languages — `grain` is French, and the
    // plural is spelled identically. sameAsEn is the explicit declaration that
    // this is a translation and not an untranslated leftover; without it,
    // check-i18n assertion 4 rejects the entry as a silent passthrough, which
    // is exactly the guard that should fire on a string nobody thought about.
    'label.grains': { en: { t: 'Grains' }, fr: { t: 'GRAINS', sameAsEn: true, reviewed: false } },

    // ÉCART rather than DÉSACCORD, and the reason is BOTH width and meaning.
    // DÉSACCORD renders at 70.14 px against the 60.00 px budget: it would widen
    // #detune-knob from 60 to 70.14, change the space-around row total, and move
    // all six knobs and their eighteen children. ÉCART is 37.88. It is also the
    // better word: DETUNE here sets a RANGE of per-grain pitch offsets in cents
    // (`Per-grain pitch micro-detuning range in cents`, PluginProcessor.cpp:117),
    // and `écart` names a spread where `désaccord` names a state of being out
    // of tune. The budget forced the question; the answer stands on its own.
    'label.detune': { en: { t: 'Detune' }, fr: { t: 'ÉCART', reviewed: false } },

    // DOSAGE over MÉLANGE, which also fits (57.02) but with only 2.98 px of
    // slack. Windows/WebView2 font metrics are this rollout's named
    // hardware-blocked deferral, so a 13 px margin is worth more than a
    // marginally more literal word. DOSAGE is the standard French label for a
    // dry/wet blend amount.
    'label.mix':    { en: { t: 'Mix' },    fr: { t: 'DOSAGE', reviewed: false } },

    // ── The LFO group ───────────────────────────────────────────────────────
    // #lfo-group-label is `position: absolute` inside #lfo-group, so its width
    // is free: 89.03 against 79.22 pushes nothing, and 12 + 89.03 is still well
    // inside the group's 283.67 px.
    'label.driftLfo': { en: { t: 'Drift LFO' }, fr: { t: 'LFO DÉRIVE', reviewed: false } },

    'label.rate':  { en: { t: 'Rate' },  fr: { t: 'VITESSE', reviewed: false } },

    // PROF., abbreviated, and this one is a genuine compromise rather than a
    // better word found under pressure. PROFONDEUR is 87.30 px against a 60.00
    // budget and AMPLEUR is 61.16 — over by 1.16, which is above the gate's
    // 0.5 px tolerance and would re-centre the whole translateX(-50%) LFO group.
    // No pin rescues it: pinning `.knob-small` wider moves the ENGLISH layout,
    // and pinning it at 60 converts the overflow into a clip rather than
    // preventing it. So the caption is abbreviated, the way a tight French UI
    // abbreviates it, and flagged for a native speaker like every other string
    // here.
    'label.depth': { en: { t: 'Depth' }, fr: { t: 'PROF.', reviewed: false } },

    // #lfo-shape-label sits above a 105.67 px selector in a column that
    // shrink-wraps to the WIDER of the two, so FORME's 44.89 is free.
    'label.shape': { en: { t: 'Shape' }, fr: { t: 'FORME', reviewed: false } },

    // ── The three LFO shape captions ────────────────────────────────────────
    // NOT exempt under D-01 arm 1. The LFO_SHAPE AudioParameterChoice options
    // are spelled "Sine", "Triangle", "Random" (PluginProcessor.cpp:108) and
    // these captions are "Sin", "Tri", "Rnd" — abbreviations, not the option
    // strings. Byte-identity is the test, and it fails, so they localize: this
    // is the `CUSTOM` against an option spelled `Scala` case from the contract,
    // not the `12-TET` one.
    //
    // Sin and Tri abbreviate identically in French (sinus, triangle), so both
    // are sameAsEn. Only Rnd changes, and it is the string that made the pin
    // necessary: Alé renders 4.69 px NARROWER than Rnd, which would have shrunk
    // #lfo-shape-selector, shrunk #lfo-shape-toggle, shrunk #lfo-group and
    // re-centred every one of its children. A French string getting SHORTER is
    // the failure mode a clip check cannot see.
    'label.shape.sin': { en: { t: 'Sin' }, fr: { t: 'Sin', sameAsEn: true, reviewed: false } },
    'label.shape.tri': { en: { t: 'Tri' }, fr: { t: 'Tri', sameAsEn: true, reviewed: false } },
    'label.shape.rnd': { en: { t: 'Rnd' }, fr: { t: 'Alé', reviewed: false } },

    // ── The settings popover (v2.1.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the same language the page is showing. These are the page's only
    // aria-label attributes; v2.0.1 had none, and none of them replaces a
    // deleted native title= because there were no native title attributes to
    // delete. No hover-help prose is invented here.
    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['Ouaricon Granular Freeze',
     'the product display name in the #header h1 — a product name is never translated. It is the brand-plus-product form of the registered PRODUCT_NAME "O-Freeze" in CMakeLists.txt, and the same shape as O-AnalogSaturation\'s exempt "OUARICON SATURATION"'],

    // ── D-01 arm 1: the MODE captions, and the word they drag with them ─────
    //
    // "Manual" and "Threshold" are the MODE AudioParameterChoice option strings
    // BYTE FOR BYTE (PluginProcessor.cpp:52-56, StringArray {"Manual",
    // "Threshold"}). Translating the buttons alone would make the page and the
    // host automation lane disagree about the same setting.
    //
    // "Threshold" ALSO appears as #threshold-knob's `.knob-label`, and that
    // second occurrence is exempt too — deliberately, for three reasons:
    //
    //   1. Byte-identity is the stated test, and the string IS byte-identical
    //      to a live choice option on this page.
    //   2. The knob and the mode button name the SAME threshold. A page reading
    //      SEUIL over the knob and Threshold on the button beside it describes
    //      one control as two.
    //   3. check-i18n's exempt set is matched by TEXT, not by element, so
    //      "localize one occurrence, exempt the other" is not a state the gate
    //      can hold: an exempt entry for the button silently covers the knob as
    //      well. Exempting both is the only reading the gate can express
    //      faithfully, and the divergence is reported rather than worked around.
    //
    // A French user therefore sees three English words on this page, all three
    // naming the same host-visible setting.
    ['Manual',
     'a MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:55) — D-01 arm 1'],
    ['Threshold',
     'a MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:55) — D-01 arm 1. This entry ALSO covers #threshold-knob\'s .knob-label, which carries the same word for the same setting; see the block comment above for why both occurrences stay English'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper].
//
// FOURTEEN, and EVERY ONE IS BARE. That is a divergence worth naming, because
// the naive reading of T17 — "bind to the ids the UI already uses" — was wrong
// on five plugins out of five before this one, and the two halves of it fail
// independently. Both halves were checked here:
//
//   the SELECTOR half — every anchor on this page HAS an id. Twelve controls,
//   twelve ids, plus #gear-btn and #lang-select. No .knob[data-param] form is
//   needed anywhere.
//
//   the TARGET half — the id is also the hover CELL, which is the half that
//   failed on O-Comp and O-Tremolo. `.knob` is a flex COLUMN holding the 60 px
//   visual, the caption and the readout, so #drift-knob already IS the cell a
//   user aims at; there is no inner 4 px stroke to bind by mistake. #freeze-button
//   is the whole 140 px box (its caption is pointer-events:none and could not
//   hold a tip open). #mode-toggle and #lfo-shape-toggle wrap their own option
//   buttons. So no closest() walk is needed, and adding one would be noise.
//
// #gear-btn and #lang-select are bare for the OPPOSITE reason: they share
// #settings-cluster, so a wrapper walk there would make hovering the selector
// resolve to the gear's tip. That is the O-Comp finding, and it applies here.
//
// applyI18n() warns `i18n: tip target not found: <selector>` for a binding that
// resolves to nothing, and boot-all-uis is the gate that sees that warning.
// tests/ui_tip_render_check.js asserts all fourteen resolve AND that each one
// actually paints.
// ============================================================================

export const TIP_BINDINGS = [
    ['#freeze-button',    'tip.freeze'],
    ['#reverse-toggle',   'tip.reverse'],
    ['#mode-toggle',      'tip.mode'],
    ['#threshold-knob',   'tip.threshold'],
    ['#drift-knob',       'tip.drift'],
    ['#grain-size-knob',  'tip.size'],
    ['#grain-count-knob', 'tip.grains'],
    ['#detune-knob',      'tip.detune'],
    ['#mix-knob',         'tip.mix'],
    ['#lfo-rate-knob',    'tip.rate'],
    ['#lfo-depth-knob',   'tip.depth'],
    ['#lfo-shape-toggle', 'tip.shape'],
    ['#gear-btn',         'tip.settings'],
    ['#lang-select',      'tip.langSelect'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v2.2.0: applyI18n() calls it once per TIP_BINDINGS entry, fourteen
// times per language switch. Through v2.1.0 the loop was empty and this function
// was exported unreferenced, purely so the canon block stayed byte-identical to
// the other forty-two copies.
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
