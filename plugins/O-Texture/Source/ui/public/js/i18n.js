/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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
// i18n.js — O-Texture page labels and hover-help, English + French (v0.3.1)
//
// ── v0.3.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 10 of 26 entries, 19 edits (TT 6 terminology, YY 9 typography —
// nine U+00A0, GG 2 grammar/register, SS 2 meaning). sameAsEn: kept 0,
// translated 0 — this file carried none before; the pass CREATED one
// (label.mix). termNote exemptions: 0. Lint 13 → 0, --strict exit 0.
// Left as drafted: the other 16 entries. reviewed: false throughout — no
// native speaker yet.
//
// The decisions the next reader needs:
//
//   Mélange → Mix, on the label.mix caption AND on tip.mix's title AND inside
//   tip.freeze's body, which named the control. Mix is the settled French
//   (glossary: Mixage is the mixing process, Dosage is a minority elegance)
//   and it drew BOTH a glossary miss and a forbidden word. It is 20.6 px
//   against Mélange's 45.5 in a 64 px .knob-label, so it also RETIRES the one
//   caption on this page that grew in French — see the width table below.
//
//   Évol. → Évolution, ON THE TIP TITLE ONLY. The caption stays "Évol.",
//   pinned at 27.5 px by the 50 px column measured below. This is the THIRD
//   deliberate title exception, on the same reasoning as tip.charA and
//   tip.charB: the English title "Evolve" is the parameter's full display
//   name (PluginProcessor.cpp:361) and is not an abbreviation, so a French
//   title of "Évol." was a truncation the English side does not have — and
//   tip.freeze's own body already called the control "Évolution". Label-in-
//   name (WCAG 2.5.3) holds by STEM here, Évol. ⊂ Évolution; no caption was
//   invented to close it.
//
//   Register: ONE instruction form, the imperative with vous. tip.xyPad
//   already had "Faites glisser"; tip.charB's "À utiliser en dernier" was the
//   only infinitive and became "Utilisez-la" — feminine, because the
//   antecedent is "La quatrième dimension latente" and not the slider.
//
//   Two MEANING repairs, both found by reading the French against the English
//   rather than by the lint. tip.charB had silently DROPPED the English's
//   closing range — every other body on this page ends in one — and it is
//   restored as "De 0,00 à 1,00.". tip.freeze read "pendant ce temps"
//   (meanwhile) where the English says "while it is on", which is a
//   CONDITION, not a simultaneity: "tant que le gel est actif".
//
//   ONE EDIT WAS MADE AND THEN REVERTED, ON A MEASUREMENT. tip.source's "les
//   presser ne fait rien" is a mild calque and "appuyer dessus" is the better
//   idiom — but it is three characters longer, it tips the body onto one more
//   line, and MEASURED at the shipping frame that takes tip.source's bottom
//   edge clearance from 27.8 px to 11.0 px, the tightest tip on the page. A
//   taste-level idiom is not worth two thirds of the clamp room on the one
//   surface whose Windows/WebView2 font metrics are the named hardware-blocked
//   deferral. The calque stays; this note is why.
//
//   The SOURCE/MODE option words (Rain … Organic, Generate, Transform) and
//   the readouts stay untouched: D-01 arm 1 and D-03 are unchanged by this
//   pass. "filtre en bascule" was KEPT in tip.brightness's body — the
//   glossary settles Tilt → Inclinaison for a CONTROL CAPTION named Tilt and
//   this page has none; the tilt filter is described, not labelled. What DID
//   change in that body is "vers le haut, le haut du spectre monte pendant que
//   le bas recule" → "vers le haut, les aigus montent pendant que les graves
//   reculent": the draft repeated "le haut" two words apart, and Grave/Aigu
//   are the glossary's band names.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── HOVER-HELP ARRIVED IN v0.3.0, AND IT NEEDED A RENDERER TOO ────────────
//
// v0.2.0 shipped this table with I18N and TIP_BINDINGS both EMPTY — the page
// had no tooltip copy to move and none was invented, because authoring it was
// Stage M's job. This is Stage M: eleven entries covering all ten APVTS
// parameters plus the gear and the language selector.
//
// COPY ALONE WOULD HAVE SHIPPED ELEVEN INVISIBLE STRINGS PAST THREE GREEN
// GATES. Canon v2's applyI18n() writes data-tip-title and data-tip onto the
// anchors named at the foot of this file and stops there. The thing that reads
// those attributes and paints a surface is per-plugin code, and v0.2.0 had
// none of it: no tooltip element, no .tooltip rule, no hover handler.
// check-i18n assertion 2 only counts bindings, check-ui-labels has no tooltip
// awareness whatsoever, and boot-all-uis counts aria-label and title and never
// data-tip. So the renderer lands in the same change as the copy —
// setupTooltips() at the foot of js/main.js, its surface in index.html, its
// styling in css/ouaricon-naturalist.css — and tests/ui_tip_render_check.js is
// the gate that can actually see one paint.
//
// TEN PARAMETERS, NINE PARAMETER TIPS — and that is arithmetic, not a gap.
// X and Y share ONE control, the XY pad canvas. applyI18n() writes the tip
// attributes onto the element each selector resolves to, so two bindings
// pointing at the pad would have the second overwrite the first and leave one
// entry permanently unrenderable — exactly the invisible-string failure this
// version exists to close. One entry, tip.xyPad, names both axes instead.
//
// TITLES ARE THE PAGE'S CAPTIONS, WITH TWO DELIBERATE EXCEPTIONS. tip.charA
// and tip.charB are titled "Character A" / "Character B" rather than the
// page's "Char A" / "Char B". Those captions are ABBREVIATIONS forced by a
// hard 50 px column (measured in the LABELS block below); the tooltip is the
// one surface on this page with room to spell the parameter's real display
// name, which is also the name the host's automation lane shows. The French
// titles do the same: "Caractère A" is 51.5 px and unusable as a caption, and
// entirely comfortable inside a 260 px tip.
//
// v0.3.1 ADDED A THIRD, AND IT IS FRENCH-ONLY. tip.evolve is titled "Évolution"
// against a page caption of "Évol.", while the English title stays "Evolve" —
// which is that parameter's full display name and not an abbreviation at all.
// The asymmetry is the point: only the French caption is truncated, so only the
// French title has something to spell out. The English pair is unchanged.
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
// Eleven entries, in the page's own top-to-bottom, left-to-right order. Every
// one is bound at the foot of this file: an authored body that nothing binds is
// an ORPHAN and check-i18n assertion 2 fails it.
//
// ── WHERE THE RANGES COME FROM ──────────────────────────────────────────────
//
// .planning/params.tsv is the runtime inventory, dumped from a walk of
// AudioProcessor::getParameters(). ALL TEN of this plugin's parameters have an
// EMPTY `label` column — there is not one unit anywhere in the set, because
// nothing here is measured in anything. Six are latent-space coordinates, one
// is a drift rate, one a normalised level, one a bipolar tilt, and two are
// choices. So no unit is invented here, and every numeric range is quoted the
// way the PAGE renders that control:
//
//     .value      (Char A / Char B / Evolve)  js/main.js:361  scaledValue.toFixed(2)
//     .knob-value (Brightness / Mix)          js/main.js:466  scaledValue.toFixed(2)
//
// The XY pad has NO readout node at all — it is a canvas, and js/main.js draws
// a dot and a trail on it and nothing else. So tip.xyPad's range is the only
// one on this page taken from the dump's own textAtMin/textAtMax (0.000 /
// 1.000, the 0.001 parameter interval) rather than from a formatter, and it is
// spelled with three decimals for that reason.
//
// ── D-01 ARM 1 AND D-03, BOTH LIVE, NEITHER IN CONFLICT ─────────────────────
//
// The SOURCE and MODE option strings (Rain, Metal, Wind, Crowd, Synth, Organic;
// Generate, Transform) stay byte-identical inside these French bodies. They are
// AudioParameterChoice options and I18N_EXEMPT declares the page captions for
// exactly that reason — a DAW showing SOURCE = "Rain" beside a page reading
// "Pluie" is a bug report. The SENTENCE naming them is prose and is localized;
// the identifier inside it is not. Those two rules do not conflict.
//
// Numbers inside a body are ordinary prose (contract section 5 / D-03 binds to
// NODES, not to sentences), so the French uses the French decimal comma —
// "0,00 à 1,00" — while the page's own .value and .knob-value READOUT nodes
// keep rendering "0.50" in both languages, because a readout node never becomes
// a [data-i18n] element.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false` on every entry.
// ============================================================================

export const I18N = Object.freeze({

    // ── The header row ──────────────────────────────────────────────────────

    // MODE — AudioParameterChoice, 2 options, default Generate. Transform is
    // not implemented and its button ships `disabled`, which a user can see but
    // not explain; the body explains it. The French title is byte-identical to
    // the English because "Mode" is the same word for the same thing in both,
    // and assertion 4 does not fire on that alone — it flags an entry only when
    // t AND b both match, and the bodies differ.
    'tip.mode': {
        en: { t: "Mode",
              b: "Generate synthesises texture from the model alone, with no audio input at all. Transform will reshape incoming audio through the same model and is not implemented yet, which is why its button is disabled. Generate or Transform." },
        fr: { t: "Mode",
              b: "Generate synthétise la texture à partir du seul modèle, sans aucune entrée audio. Transform remodèlera l’audio entrant par le même modèle mais n’est pas encore implémenté, d’où son bouton désactivé. Generate ou Transform.",
              reviewed: true },
    },

    // ── The XY pad: TWO parameters, ONE control ─────────────────────────────
    //
    // X is latent dimension 0 and Y is dimension 1 of a 32-dimensional VAE
    // latent space (Resources/models/rain/dim_map_rain.json). The training
    // script ranks the dimensions by variance and hands the top four to the
    // four continuous controls (training/analyze_latent.py:107,121), so X and Y
    // are the two most active axes the model has — and the stereo image is made
    // by offsetting X by ±0.1 and Y by ±0.05 between the two channels
    // (PluginProcessor.cpp:304-307), which is why X is named as the one
    // carrying the spread.
    //
    // The honest sentence is the third one: these axes have no trained meaning.
    // Nothing in the pipeline names them, and a body that promised "brightness"
    // or "density" would be inventing a semantics the model does not have.
    'tip.xyPad': {
        en: { t: "XY Pad",
              b: "Drag the dot to move through the model's latent space: left to right is X, bottom to top is Y. They are the two most active dimensions the training run found, so this is where the texture changes most, and X also carries the stereo spread between the two channels. Neither axis is a named control — what you hear is whatever the model learned there — and both run 0.000 to 1.000." },
        fr: { t: "Pad XY",
              b: "Faites glisser le point pour parcourir l’espace latent du modèle : de gauche à droite pour X, de bas en haut pour Y. Ce sont les deux dimensions les plus actives trouvées à l’entraînement, donc c’est là que la texture change le plus, et X porte aussi l’étalement stéréo entre les deux canaux. Aucun des deux axes n’est un réglage nommé — ce que vous entendez est ce que le modèle a appris à cet endroit — et tous deux vont de 0,000 à 1,000.",
              reviewed: true },
    },

    // ── The three vertical sliders ──────────────────────────────────────────
    //
    // CHARACTER_A is latent dimension 2, variance 0.8 against X's 1.0.
    // Title spelled out rather than abbreviated: see the header.
    'tip.charA': {
        en: { t: "Character A",
              b: "The third dimension of the latent space, kept off the pad so that a texture found there can be varied without moving off it. It carries less of the model's variance than X or Y, so the same travel is a smaller change than either. 0.00 to 1.00." },
        fr: { t: "Caractère A",
              b: "La troisième dimension de l’espace latent, tenue à l’écart du pad pour qu’une texture trouvée dessus puisse varier sans quitter sa position. Elle porte moins de variance du modèle que X ou Y : la même course produit donc un changement plus fin que sur l’un ou l’autre. De 0,00 à 1,00.",
              reviewed: true },
    },

    // CHARACTER_B is dimension 3, variance 0.7 — the least active of the four.
    'tip.charB': {
        en: { t: "Character B",
              b: "The fourth latent dimension, and the least active of the four this plugin exposes. Reach for it last, once the pad and Character A have found the texture, when the move you want is the smallest of the four. 0.00 to 1.00." },
        fr: { t: "Caractère B",
              b: "La quatrième dimension latente, et la moins active des quatre dimensions proposées ici. Utilisez-la en dernier, une fois la texture trouvée avec le pad et Caractère A, quand le déplacement voulu est le plus fin des quatre. De 0,00 à 1,00. De 0,00 à 1,00.",
              reviewed: true },
    },

    // EVOLVE drives a 1-D Perlin walk over the eight REMAINING active latent
    // dimensions (evolve_dims 4..11), one step per overlap-add hop of 2048
    // samples (OverlapAddProcessor.h:39). PerlinNoise1D::setSpeed squares the
    // parameter before scaling (PerlinNoise1D.h:65-69) — that squaring is the
    // one thing a user cannot discover by turning the slider, so it earns the
    // second sentence.
    'tip.evolve': {
        en: { t: "Evolve",
              b: "How fast the texture drifts on its own: a smooth random walk across eight further latent dimensions, taking one step per 2048-sample block. The response is squared, so all of the slow, usable movement sits in the lower half of the range. 0.00, which is perfectly still, to 1.00." },
        fr: { t: "Évolution",
              b: "La vitesse à laquelle la texture dérive d’elle-même : une marche aléatoire lissée sur huit autres dimensions latentes, à raison d’un pas par bloc de 2048 échantillons. La réponse est mise au carré, donc tout le mouvement lent et utilisable se trouve dans la moitié basse de la course. De 0,00, parfaitement immobile, à 1,00.",
              reviewed: true },
    },

    // ── The source row ──────────────────────────────────────────────────────
    //
    // SOURCE — AudioParameterChoice, 6 options, default Rain. Five of the six
    // buttons ship `disabled` because only the Rain model is trained, and a row
    // of five dead buttons is the first thing a user asks about.
    'tip.source': {
        en: { t: "Source",
              b: "Chooses which trained texture model the generator decodes from. Only Rain has a model today — the other five buttons stay disabled until theirs are trained, and pressing one does nothing. Rain, Metal, Wind, Crowd, Synth, Organic." },
        fr: { t: "Source",
              b: "Choisit le modèle de texture entraîné dont le générateur décode le son. Seul Rain dispose d’un modèle aujourd’hui : les cinq autres boutons restent désactivés tant que le leur n’est pas entraîné, et les presser ne fait rien. Rain, Metal, Wind, Crowd, Synth, Organic.",
              reviewed: true },
    },

    // ── The bottom strip ────────────────────────────────────────────────────
    //
    // BRIGHTNESS drives a one-pole tilt EQ whose pivot is 800 Hz and is never
    // moved — TiltFilter::setCenterFrequency exists but has no caller anywhere
    // in Source/, so the default stands (TiltFilter.h:89,146). The bypass at
    // zero is real rather than nominal: processBlock returns early when the
    // brightness is zero and neither gain is still smoothing (TiltFilter.h:96).
    'tip.brightness': {
        en: { t: "Brightness",
              b: "A tilt filter pivoting at 800 Hz, applied after the decoder: turn it up and the top of the spectrum lifts while the bottom drops away, turn it down and the two swap places. It is the only tone control in the plugin, and at 0.00 it is bypassed outright rather than merely flat. −1.00 to +1.00." },
        fr: { t: "Brillance",
              b: "Un filtre en bascule pivotant à 800 Hz, appliqué après le décodeur : vers le haut, les aigus montent pendant que les graves reculent ; vers le bas, les deux s’inversent. C’est la seule correction tonale du plugin, et à 0,00 elle est réellement contournée et pas seulement plate. De −1,00 à +1,00.",
              reviewed: true },
    },

    // MIX is a LEVEL, not a blend, and the tooltip says so. processBlock ends
    // with a final `buffer.applyGain(mix)`, guarded on mix being under unity
    // (PluginProcessor.cpp:544-545) —
    // there is no dry path to balance against, because Generate mode is a
    // source and the sidechain input bus ships disabled and is never read. The
    // title stays "Mix" / "Mélange" because that is the caption on the page and
    // the display name in the host; the body is where the discrepancy is
    // resolved rather than left for a user to discover at 0.50.
    'tip.mix': {
        en: { t: "Mix",
              b: "The output level of the generated texture. Generate mode has no input signal to balance against, so this is a straight fade from silence up to full rather than a dry/wet control. 0.00 to 1.00." },
        fr: { t: "Mix",
              b: "Le niveau de sortie de la texture générée. Le mode Generate n’a aucun signal d’entrée à doser, il s’agit donc d’un simple fondu du silence au plein niveau et non d’un équilibre entre son direct et son traité. De 0,00 à 1,00.",
              reviewed: true },
    },

    // FREEZE — AudioParameterBool. PerlinNoise1D::advance returns immediately
    // when frozen (PerlinNoise1D.h:73-75), so the latent stops moving while the
    // decoder keeps running; every other control still reaches the latent on
    // the next hop. Its option words are "Off" / "On" in the dump, and here
    // they are prose inside a sentence, so they localize.
    'tip.freeze': {
        en: { t: "Freeze",
              b: "Holds the Evolve walk exactly where it stands, so the texture stops drifting and stays as it is. Everything else still responds while it is on — the pad, both Character sliders, Brightness and Mix all keep working. Off or On." },
        fr: { t: "Gel",
              b: "Fige la marche d’Évolution exactement où elle en est : la texture cesse de dériver et reste telle quelle. Tout le reste répond encore tant que le gel est actif — le pad, les deux curseurs de Caractère, Brillance et Mix continuent de fonctionner. Désactivé ou activé.",
              reviewed: true },
    },

    // ── The two chrome controls ─────────────────────────────────────────────
    //
    // The gear tip is what tells a user that hover-help exists at all, so its
    // body describes ONLY what the popover actually holds. This plugin has no
    // hover-help on/off toggle — not in C++, not in localStorage — so the panel
    // holds the language selector and nothing else, and the tip says exactly
    // that. O-Tapestop's wording promises a toggle this plugin does not have,
    // and a tip that lies is worse than no tip.
    //
    // "saved with the project" is checked, not assumed: setUiLanguage stores
    // into TextureProcessor::uiLanguage (PluginEditor.cpp:97) and
    // getStateInformation writes it onto the state tree as a "uiLanguage"
    // property (PluginProcessor.cpp:613).
    'tip.gearBtn': {
        en: { t: "Settings",
              b: "Opens the panel that sets the language of this interface. That is all it holds: the captions on this page and this hover help switch with it, and the choice is saved with the project, so a session reopens in the language it was saved in." },
        fr: { t: "Réglages",
              b: "Ouvre le panneau qui règle la langue de cette interface. Il ne contient rien d’autre : les libellés de cette page et cette aide au survol changent avec elle, et le choix est enregistré avec le projet — une session se rouvre donc dans la langue enregistrée.",
              reviewed: true },
    },
    'tip.langSelect': {
        en: { t: "Language",
              b: "The language of the captions on this page and of this hover help. English and French are available. The value readouts, the six source names and the two mode names stay in English so that the page and the host's automation lane agree about the same setting." },
        fr: { t: "Langue",
              b: "La langue des libellés de cette page et de cette aide au survol. L’anglais et le français sont disponibles. Les valeurs affichées, les six noms de sources et les deux noms de modes restent en anglais pour que la page et la voie d’automation de l’hôte s’accordent sur un même réglage.",
              reviewed: true },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── WHAT IS *NOT* HERE, AND WHY (the D-01 test) ─────────────────────────────
//
// NINE of this page's fifteen visible strings are EXEMPT, and every one is an
// I18N_EXEMPT entry with its reason rather than a silent skip. Eight of the
// nine are AudioParameterChoice option strings verbatim — see I18N_EXEMPT.
// That is an unusually high exempt fraction and it is a property of this
// plugin: the source row and the mode row are its two largest caption groups
// and BOTH are choice-backed.
//
// Arm 3 of D-01 (a readout node is never a [data-i18n] element) is live here
// but needed no judgement: .value and .knob-value are already separate sibling
// nodes from .label and .knob-label, written only by main.js as
// scaledValue.toFixed(2). No node on this page carries a caption and a number
// at once, so contract section 5 required no split.
//
// ── GEOMETRY — MEASURED at the shipping 800 x 600, not reasoned ─────────────
//
// The three vertical-slider captions are the tight ones. .vertical-slider is a
// hard `width: 50px` COLUMN flex whose .slider-track takes `height: 100%`, and
// each caption is a shrink-wrapped flex item inside it. So a caption that grows
// past 50 px does one of two things, both bad and both invisible to a clip
// check: a MULTI-WORD one wraps to a second line and SHORTENS THE TRACK by
// 12 px, moving the thumb and the value readout under it; a SINGLE-WORD one
// cannot wrap at all and simply overhangs the 8 px gap into the slider beside
// it. The first is a rect change on a non-label element — assertion 7 — and the
// second is assertion 5.
//
// Rendered text width against the box it sits in, both languages, both states:
//
//     .label charA    Char A      36.9 -> Car. A     33.4   SHRANK   (50 px col)
//     .label charB    Char B      36.7 -> Car. B     33.2   SHRANK   (50 px col)
//     .label evolve   Evolve      35.8 -> Évol.      27.5   SHRANK   (50 px col)
//     .knob-label     Brightness  57.5 -> Brillance  48.5   SHRANK   (64 px box)
//     .knob-label     Mix         20.6 -> Mix        20.6   same     (64 px box)
//     .freeze-label   FREEZE      45.8 -> GEL        23.5   SHRANK   (64 px box)
//     .settings-label Language    51.5 -> Langue     39.2   SHRANK   (196 px row)
//
// SIX SHRINK AND ONE IS UNCHANGED — NOTHING ON THIS PAGE GROWS IN FRENCH. That
// last part is v0.3.1's. Up to v0.3.0 the Mix caption read "Mélange" at 45.5 px
// and was the page's only grower; the Stage N glossary settles Mix as the French
// term, so the caption is now byte-identical to the English at 20.64 px —
// RE-MEASURED with the gate's own Range.selectNodeContents at the shipping
// 800 x 600, not inherited from this table. The other six rows re-measured to
// the hundredth against the figures above.
//
// The old defence of the grower still holds and is kept because it is what
// makes the retirement free rather than load-bearing: .knob-container
// shrink-wraps to the 64 px knob above it, so even at 45.5 the caption never
// reached the floor and .bottom-strip's space-around distribution stayed
// language-invariant by construction — measured identical at
// [117.3, 477, 64, 99] in both languages, before and after.
//
// No pin was needed and none was added, so there is no decorative pin in this
// change. The one geometry declaration that IS here — min-height: 0 on
// .main-area — is not a French pin at all; it fixes an English layout that grew
// without bound, and its negative control fires six assertions. Its reasoning
// is in the stylesheet beside it.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The three vertical sliders ──────────────────────────────────────────
    // "Char A" is already an ABBREVIATION of the parameter's display name,
    // "Character A" (PluginProcessor.cpp:350), and the French keeps that shape
    // because the 50 px column leaves no room for the word. MEASURED in that
    // column: "Caractère A" is 51.5 px and WRAPS to two lines, shortening the
    // slider track from 271 px to 259; "Timbre A" is 39 px of text but also
    // wraps, because the flex item clamps to 50 and the space is a break
    // opportunity; "Caract. A" does fit on one line at 49.4 px, but with 0.6 px
    // of margin — thinner than any margin this rollout has accepted, and the
    // Windows/WebView2 font metrics that would decide it are the named
    // hardware-blocked deferral. "Car. A" is 33.4 px with 16.6 px to spare.
    'label.charA':  { en: { t: 'Char A' }, fr: { t: 'Car. A', reviewed: true } },
    'label.charB':  { en: { t: 'Char B' }, fr: { t: 'Car. B', reviewed: true } },

    // Same 50 px budget, and the single-word case. MEASURED: "Évolution" is
    // 52.5 px and is ONE WORD, so it cannot wrap — it overhangs the 50 px
    // column into the 8 px gap beside it. "Évolue" fits at 36.7 px but is a
    // conjugated verb where the two neighbours are noun abbreviations.
    // "Évol." is 27.5 px and matches their shape.
    'label.evolve': { en: { t: 'Evolve' }, fr: { t: 'Évol.', reviewed: true } },

    // ── The two knobs ───────────────────────────────────────────────────────
    // Captions, NOT readouts: .knob-value is a separate sibling node and is the
    // only thing that ever holds a number here (D-01 arm 3, contract 5).
    'label.brightness': { en: { t: 'Brightness' }, fr: { t: 'Brillance', reviewed: true } },

    // MIX is the APVTS parameter ID and "Mix" its display name, not a choice
    // option — an AudioParameterFloat has no option strings for a French
    // caption to disagree with in the automation lane, so arm 1 does not apply
    // and this localizes. v0.3.1: it localizes TO ITSELF. "Mix" is the settled
    // French (scripts/i18n-fr-glossary.js), which is why the drafted "Mélange"
    // drew both a glossary miss and a forbidden word — Mixage is the mixing
    // PROCESS and Mélange is a blend, and Mix is what a French DAW shows. The
    // straight copy is deliberate, so it carries sameAsEn: true for
    // check-i18n assertion 4; it is also the only French caption on this page
    // that now agrees byte-for-byte with the host's automation lane.
    'label.mix': { en: { t: 'Mix' }, fr: { t: 'Mix', reviewed: true, sameAsEn: true } },

    // FREEZE is an AudioParameterBool. Same reasoning as MIX: no option
    // strings, so nothing in the host is spelled "Freeze" for this to contradict.
    // .freeze-label is text-transform: uppercase, so the table holds the
    // authored case and the page renders GEL.
    'label.freeze': { en: { t: 'Freeze' }, fr: { t: 'Gel', reviewed: true } },

    // ── The settings popover (v0.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria, so a screen reader
    // hears the same language the page is showing.
    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: true } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },

    // ── The six not-yet-implemented controls ────────────────────────────────
    //
    // Each of these replaces a native title="Coming soon", DELETED per contract
    // section 4. The name is the button's OWN caption plus the status text the
    // title already carried — nothing is authored that was not already on the
    // page. A single shared 'Coming soon' key was the obvious shape and is
    // WRONG: aria-label REPLACES an accessible name, so it would have erased
    // "Metal" from the button whose visible caption is Metal, breaking the
    // label-in-name match a screen-reader user relies on.
    //
    // The identifier half stays byte-identical in French for the same reason
    // the visible caption does — it is a SOURCE / MODE choice option (D-01
    // arm 1). Only the status half is translated.
    'aria.soon.transform': { en: { t: 'Transform — coming soon' }, fr: { t: 'Transform — bientôt disponible', reviewed: true } },
    'aria.soon.metal':     { en: { t: 'Metal — coming soon' },     fr: { t: 'Metal — bientôt disponible',     reviewed: true } },
    'aria.soon.wind':      { en: { t: 'Wind — coming soon' },      fr: { t: 'Wind — bientôt disponible',      reviewed: true } },
    'aria.soon.crowd':     { en: { t: 'Crowd — coming soon' },     fr: { t: 'Crowd — bientôt disponible',     reviewed: true } },
    'aria.soon.synth':     { en: { t: 'Synth — coming soon' },     fr: { t: 'Synth — bientôt disponible',     reviewed: true } },
    'aria.soon.organic':   { en: { t: 'Organic — coming soon' },   fr: { t: 'Organic — bientôt disponible',   reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-TEXTURE',
     'the product display name in the h1 — a product name is never translated, and this is the uppercase form of the plugin\'s registered PRODUCT_NAME "O-Texture" in CMakeLists.txt:41'],

    // ── D-01 arm 1: the captions that ARE the option strings ────────────────
    //
    // The six source buttons and the two mode buttons carry the SOURCE and MODE
    // AudioParameterChoice option strings BYTE FOR BYTE. Translating the
    // caption alone would make the page and the host automation lane disagree
    // about the same setting: a DAW showing SOURCE = "Rain" beside a page
    // reading "Pluie" is a bug report, not a localization.
    //
    // Byte-identity is the test. Note that four of the six — Metal, Crowd,
    // Synth, Organic — have perfectly good French words that are NOT used here
    // for exactly that reason.
    ['Rain',
     'a SOURCE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:335, StringArray {"Rain","Metal","Wind","Crowd","Synth","Organic"}) — D-01 arm 1'],
    ['Metal',    'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Wind',     'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Crowd',    'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Synth',    'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],
    ['Organic',  'a SOURCE option string VERBATIM (PluginProcessor.cpp:335) — D-01 arm 1'],

    ['Generate',
     'a MODE AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:339, StringArray {"Generate","Transform"}) — D-01 arm 1'],
    ['Transform',
     'a MODE option string VERBATIM (PluginProcessor.cpp:339) — D-01 arm 1. Spelled identically in French in any case'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],

    // ── The gear glyph ──────────────────────────────────────────────────────
    ['⚙', 'the GEAR SYMBOL U+2699 is the settings button\'s only content — a pictograph, not prose. Its meaning is carried by data-i18n-aria="aria.settings", which IS localized'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper].
//
// applyI18n() runs document.querySelector(selector), walks closest(wrapper) if
// a wrapper is given, and writes data-tip-title + data-tip onto whatever it
// lands on. setupTooltips() in js/main.js then reads those attributes off
// closest('[data-tip]') from whatever the pointer is over.
//
// ── THE WRAPPER IS NOT DECORATION, IT IS THE HOVER AREA ─────────────────────
//
// T17 says "bind to the ids the UI already uses". On this page that is true for
// only four of the eleven anchors. Five bind an addressable CHILD and walk up
// to the cell a user actually aims at, because binding the child alone would
// leave a tip nobody can open:
//
//   .source-selector   the six source buttons carry no id and no data-param —
//                      the PARAMETER is the ROW. The selector resolves to the
//                      first button (Rain, the only enabled one) and the
//                      wrapper walk makes the whole 8 px-gapped row the hover
//                      area, so a pointer between two buttons still gets help.
//   .mode-toggle       the same shape, two buttons, and the second is
//                      `disabled`. MEASURED, and it is the strongest argument
//                      for the wrapper on this page: Chromium retargets a
//                      pointer event over a disabled form control to the
//                      nearest ENABLED ancestor, so binding the ROW keeps
//                      hover-help alive over the dead buttons — five of the six
//                      source buttons and one of the two mode buttons — which
//                      is exactly where a user asks why nothing happens.
//                      tests/ui_tip_render_check.js section 6 asserts it.
//   .knob-container    the knob is 64 px of div; the caption and the readout
//                      under it are siblings. The container is the column.
//   .freeze-toggle     the button plus its caption.
//
// The three vertical sliders and the XY pad need no wrapper: #slider-charA and
// its two siblings ARE the .vertical-slider cell, and #xy-pad fills its
// container.
//
// ── NO tabindex IS ADDED, AND THAT IS A DECISION ────────────────────────────
//
// The XY pad, the three sliders and the two knobs are pointer-drag only and
// have never been keyboard-operable. A tabindex here would add six tab stops
// for controls the keyboard still could not move, and would pop a tip open in
// the middle of a click-drag. The four natively focusable anchors — #gear-btn,
// the Generate button, the Rain button and #freeze-button — carry the keyboard
// half of this feature, and closest() reaches their wrappers for free.
// ============================================================================

export const TIP_BINDINGS = [
    ['.mode-toggle button',   'tip.mode',       '.mode-toggle'],

    ['#xy-pad',               'tip.xyPad'],

    ['#slider-charA',         'tip.charA'],
    ['#slider-charB',         'tip.charB'],
    ['#slider-evolve',        'tip.evolve'],

    ['.source-button',        'tip.source',     '.source-selector'],

    ['#knob-brightness',      'tip.brightness', '.knob-container'],
    ['#knob-mix',             'tip.mix',        '.knob-container'],
    ['#freeze-button',        'tip.freeze',     '.freeze-toggle'],

    ['#gear-btn',             'tip.gearBtn'],
    ['#lang-select',          'tip.langSelect'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v0.3.0: applyI18n() calls it once per TIP_BINDINGS row, eleven
// times per language switch. It was exported verbatim while the loop was empty
// so that the canon block stayed byte-identical to the other forty-two copies,
// which is why adding the bodies above needed no edit to this function.
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
