/*
   This file is part of O-Detune, an Ouaricon Audio plugin.
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
// i18n.js — O-Detune page labels and hover-help, English + French (v1.7.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is a single inline <script type="module"> in index.html,
// so that failure mode would take the WHOLE UI, not one panel of it.
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a file named i18n-fr.js would have to be
// reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens). One
// combined file for both languages sidesteps the question.
//
// ── v1.7.0 GIVES THIS PLUGIN HOVER-HELP, AND A RENDERER TO PAINT IT ────────
//
// v1.6.0 shipped I18N and TIP_BINDINGS both EMPTY, which was the correct state
// for a page that had no tooltips: v1.5.4 carried no data-tip anywhere, only
// five native title= attributes on the preset bar, which contract §4 DELETES
// rather than localizes. Their text moved to data-i18n-aria and nothing was
// invented.
//
// AUTHORING THE COPY IS ONLY HALF THE WORK, AND THE OTHER HALF IS NOT IN THE
// CANON. applyI18n() writes data-tip-title and data-tip ATTRIBUTES onto the
// anchors named in TIP_BINDINGS and stops there. The thing that reads those
// attributes and paints a surface is per-plugin code, and v1.6.0 had none of
// it — no #tooltip node, no .tooltip rule, no hover handler. Eighteen bodies
// bound with no renderer would have shipped eighteen INVISIBLE strings past
// three green gates: check-i18n counts bindings, check-ui-labels has no
// tooltip awareness at all, and boot-all-uis counts aria-label and title and
// never data-tip. So v1.7.0 adds the surface, the CSS and setupTooltips() to
// index.html, and tests/ui_tip_render_check.js is the only gate in this repo
// that can see a rendered tooltip on this page.
//
// SIXTEEN OF THE EIGHTEEN DUMPED PARAMETERS HAVE A CONTROL HERE. The two that
// do not, and the width tip that a pointer cannot reach in the default state,
// are written up in full above the I18N block below. Both are findings, not
// gaps, and neither was fixed by adding a control.
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
// v1.7.0. v1.6.0 shipped this object EMPTY, which was the correct state for a
// page that had no tooltip renderer, no tooltip surface and no hover handler.
// All three now exist (index.html, the .tooltip rule and setupTooltips()), so
// the emptiness is over and check-i18n assertion 2 flips with the first body:
// an authored body that nothing binds is an ORPHAN and fails.
//
// ── ONE TIP PER CONTROL ON THE PAGE, PLUS THE TWO CHROME ANCHORS ────────────
//
// The dump at .planning/params.tsv walks the constructed processor and reports
// EIGHTEEN parameters. SIXTEEN of them have a control here. focus_low and
// focus_high have none — see the FINDING at the foot of this comment — so they
// get no tip, because a body with nothing to bind to fails assertion 2 and the
// alternative (adding two controls to a 600 x 480 frame) is a feature change
// with a geometry cost, not a localization.
//
// 16 parameter tips + 2 chrome tips = 18 entries, 18 TIP_BINDINGS rows.
//
// ── TITLES: THE PAGE CAPTION WINS, WITH ONE STATED EXCEPTION ────────────────
//
// The title is the caption the user is reading, not the automation lane's
// name: "Era" rather than "Wobble Era", "Spread" rather than "Unison Spread".
// The exception is a caption that is a SPACE-FORCED ABBREVIATION of the
// parameter's own name — "Dist" in a 91 px cell and "Pre-Dly" in a 50 px one.
// Expanding an abbreviation is the one thing a tooltip is unambiguously for,
// so those two read "Distribution" and "Pre-Delay": the caption unabbreviated,
// with the redundant panel word ("Unison") dropped. Every other title is the
// caption verbatim, including "Mono-Safe", which the parameter spells
// "Mono Safe" without the hyphen.
//
// "Width:" loses its colon. The colon is layout punctuation joining the
// caption to #width_value under contract section 5, not part of the name.
//
// ── RANGES: FROM THE DUMP, AND ONE FROM THE PAGE'S OWN FORMATTER ────────────
//
// Fifteen of the sixteen ranges are the dump's textAtMin / textAtMax with the
// dump's own `label` as the unit. ONE had to be recovered:
//
//   blend   dumps 0.00 .. 1.00 with an EMPTY label. index.html:1194 renders it
//           `Math.round(v * 100) + '%'`, so the range the user sees is 0 to
//           100 % and the unit is a percent because the formatter says so.
//
// wobble_depth and unison_detune dump `cents` while the readout abbreviates to
// ` ct` (index.html:1196, :1198). The body spells out "cents": it is prose,
// and the abbreviation exists for a 52 px knob face this surface does not have.
//
// ── OPTION WORDS INSIDE A FRENCH BODY: SPLIT, ON A STATED RULE ──────────────
//
// D-01 arm 1 keeps an AudioParameterChoice option string English ON THE PAGE
// so the host automation lane agrees. Inside a tooltip BODY the same word is
// prose, and prose is localized — which would have a French body telling the
// user to choose "Sinus" from a menu that reads "Sine" in both languages. A
// tip that lies is worse than no tip, so the split here is by VISIBILITY:
//
//   VISIBLE on the page (the three dropdowns and the voice-count digits:
//     Sine / Triangle / Random, Linear / Exp / Random, 60s / 70s / 80s,
//     2 / 3 / 4 / 5 / 7) — the token stays VERBATIM ENGLISH in the French
//     body, because the body is naming an entry the user must find in a menu
//     that is exempt under arm 1 and reads English in both languages. The
//     sentence around it is French.
//
//   NOT VISIBLE anywhere (wobble_sync and mono_safe render as a switch with
//     no text at all — "Off" / "On" exist only in the dump and the automation
//     lane) — the range is ordinary prose and IS localized: "désactivé ou
//     activé".
//
// This diverges from O-Comp, which localized its toggle's OFF / ON to
// ARRÊT / MARCHE. That toggle is in the not-visible class and the two calls
// agree; no O-Comp option word is painted on its page.
//
// D-03 still binds and it binds to NODES. A number inside a localized body is
// ordinary prose, so the French bodies take French convention — decimal COMMA,
// a space before %, "0,1 à 10,0 Hz". The READOUT keeps its point, because
// D-03 exempts the readout NODE and that has not moved. They differ on
// purpose: one is a machine-formatted value, the other is a sentence.
//
// ── FINDING: TWO PARAMETERS ARE HOST-REACHABLE AND NOT PAGE-REACHABLE ───────
//
// focus_low (20..500 Hz, skewed) and focus_high (1000..20000 Hz, skewed) are
// automatable, are IMPLEMENTED in the DSP (PluginProcessor.cpp:602-603 writes
// their coefficients into focusHighPass / focusLowPass every block), are
// relayed to the WebView (PluginEditor.cpp:48-49, :85-86, :240-243) and are
// even given slider states and formatters by this page (index.html:1124-1125,
// :1201-1202). They have NO ELEMENT. A DAW can automate a band-limit the user
// cannot see or reach. Reported, not fixed: adding two controls is a feature
// change with a geometry cost in a 600 x 480 frame, and it is not this stage.
//
// ── FINDING: THE WIDTH TIP IS POINTER-UNREACHABLE IN THE DEFAULT STATE ──────
//
// mono_safe defaults to ON, and index.html:1483 puts `.disabled` on
// .slider-container, which is `pointer-events: none` (index.html:432-435,
// shipped in v1.5.4 and untouched here). So a POINTER cannot open the width
// tip until Mono-Safe is switched off. The KEYBOARD still can — focus events
// are unaffected by pointer-events, and Tab reaches #width — so the tip is not
// dead, only half-reachable. Not fixed: making a disabled control hoverable is
// a UX decision about this plugin, not a localization. The render gate drives
// Mono-Safe off before hovering that anchor and says so.
// ============================================================================

export const I18N = Object.freeze({

    // ── The wobble engine ───────────────────────────────────────────────────

    'tip.era': {
        en: { t: "Era",
              b: "Gives the wobble a decade's character by scaling how deep it swings: 60s is the deepest, 70s neutral, 80s the most restrained. Reach for it to age or calm the modulation without touching Depth. Three settings: 60s, 70s and 80s." },
        fr: { t: "Époque",
              b: "Donne au pleurage le caractère d'une décennie en dosant l'amplitude de son oscillation : 60s est la plus marquée, 70s est neutre, 80s la plus retenue. À utiliser pour vieillir ou assagir la modulation sans toucher à Ampleur. Trois positions : 60s, 70s et 80s.",
              reviewed: false },
    },

    'tip.shape': {
        en: { t: "Shape",
              b: "The waveform driving the pitch modulation. Sine glides evenly, Triangle turns more sharply at the extremes, and Random holds a new value each cycle for an unsteady, worn-tape stagger. Three settings: Sine, Triangle and Random." },
        fr: { t: "Forme",
              b: "L'onde qui pilote la modulation de hauteur. Sine glisse régulièrement, Triangle marque davantage les extrêmes, et Random tient une nouvelle valeur à chaque cycle, pour l'irrégularité d'une bande usée. Trois positions : Sine, Triangle et Random.",
              reviewed: false },
    },

    'tip.rate': {
        en: { t: "Rate",
              b: "How fast the pitch drifts up and down. Slow settings read as tape wow, fast ones as flutter, and switching Sync on replaces this reading with a musical division of the host tempo. 0.1 to 10.0 Hz." },
        fr: { t: "Vitesse",
              b: "Rapidité de la dérive de hauteur. Un réglage lent évoque le pleurage d'une bande, un réglage rapide son scintillement, et activer Sync remplace cet affichage par une division musicale du tempo de l'hôte. 0,1 à 10,0 Hz.",
              reviewed: false },
    },

    'tip.sync': {
        en: { t: "Sync",
              b: "Locks the wobble to the host tempo, so the Rate readout becomes a musical division from four bars down to a thirty-second note. Leave it off for a free-running drift that ignores the transport. Off or on." },
        fr: { t: "Sync",
              b: "Cale le pleurage sur le tempo de l'hôte : l'affichage de Vitesse devient une division musicale, de quatre mesures à la triple croche. Laissez-le inactif pour une dérive libre, indifférente au défilement. Désactivé ou activé.",
              reviewed: false },
    },

    'tip.depth': {
        en: { t: "Depth",
              b: "How far the pitch swings on each cycle. A handful of cents drifts almost unnoticed; past about forty the warble becomes the effect rather than the colour. Era scales this amount before it reaches the delay line. 0 to 100 cents." },
        fr: { t: "Ampleur",
              b: "Amplitude de l'oscillation de hauteur à chaque cycle. Quelques cents dérivent presque sans se faire remarquer ; au-delà de quarante environ, le chevrotement devient l'effet plutôt que sa couleur. Époque met cette valeur à l'échelle avant la ligne à retard. 0 à 100 cents.",
              reviewed: false },
    },

    // ── The blend control ───────────────────────────────────────────────────

    'tip.blend': {
        en: { t: "Blend",
              b: "Crossfades the two engines: at 0% you hear Wobble alone, at 100% Unison alone, and anywhere between you hear both. The two panels fade with it, so the page shows which engine is doing the work. 0 to 100%." },
        fr: { t: "Mélange",
              b: "Fond enchaîné entre les deux moteurs : à 0 % vous entendez le Pleurage seul, à 100 % l'Unisson seul, et entre les deux, les deux à la fois. Les panneaux s'estompent en conséquence, la page montre donc quel moteur travaille. 0 à 100 %.",
              reviewed: false },
    },

    // ── The unison engine ───────────────────────────────────────────────────

    'tip.voices': {
        en: { t: "Voices",
              b: "How many detuned copies the unison engine stacks. More voices thicken the chorus and soften transients; the sum is gain-compensated and soft-limited, so the level holds as you add them. Five settings: 2, 3, 4, 5 and 7." },
        fr: { t: "Voix",
              b: "Nombre de copies désaccordées empilées par le moteur d'unisson. Plus de voix épaississent le chœur et adoucissent les transitoires ; la somme est compensée en gain puis limitée en douceur, le niveau tient donc quand vous en ajoutez. Cinq positions : 2, 3, 4, 5 et 7.",
              reviewed: false },
    },

    'tip.dist': {
        en: { t: "Distribution",
              b: "How the voices are spaced around the centre. Linear spreads them evenly, Exp pushes the outermost pair further out than the inner ones, and Random scatters them and reveals the Random knob beside it. Three settings: Linear, Exp and Random." },
        fr: { t: "Répartition",
              b: "Manière dont les voix se répartissent autour du centre. Linear les espace régulièrement, Exp éloigne les voix extrêmes plus que les voix internes, et Random les disperse en faisant apparaître le potentiomètre Aléatoire à côté. Trois positions : Linear, Exp et Random.",
              reviewed: false },
    },

    'tip.detune': {
        en: { t: "Detune",
              b: "How far apart the unison voices are pitched. A few cents shimmer like a chorus; toward the top the stack drifts into a frankly out-of-tune ensemble. 0 to 50 cents." },
        fr: { t: "Désaccord",
              b: "Écart de hauteur entre les voix d'unisson. Quelques cents font miroiter le son comme un chorus ; vers le haut de la course, l'empilement glisse vers un ensemble franchement faux. 0 à 50 cents.",
              reviewed: false },
    },

    'tip.spread': {
        en: { t: "Spread",
              b: "Pans the unison voices across the stereo field on a constant-power law. At 0% they stack in the centre; at 100% the outermost pair sits hard left and hard right. This is the width of the voices, not of the output. 0 to 100%." },
        fr: { t: "Étalement",
              b: "Répartit les voix d'unisson dans l'image stéréo selon une loi à puissance constante. À 0 % elles se superposent au centre ; à 100 % les voix extrêmes occupent les bords gauche et droit. Il s'agit de la largeur des voix, pas de celle de la sortie. 0 à 100 %.",
              reviewed: false },
    },

    'tip.random': {
        en: { t: "Random",
              b: "Humanises the stack by varying each voice's modulation rate and depth on its own, so no two drift in step. At 0% the chosen distribution is followed exactly. This knob is only shown while Distribution is set to Random. 0 to 100%." },
        fr: { t: "Aléatoire",
              b: "Humanise l'empilement en faisant varier séparément la vitesse et l'amplitude de modulation de chaque voix, pour qu'aucune ne dérive au même rythme. À 0 % la répartition choisie est suivie à la lettre. Ce potentiomètre n'apparaît que lorsque Répartition est sur Random. 0 à 100 %.",
              reviewed: false },
    },

    // ── The output section ──────────────────────────────────────────────────

    'tip.width': {
        en: { t: "Width",
              b: "Scales the side channel of the finished signal: 0% collapses it to mono, 100% leaves the image as the engines made it, and 200% pushes it wider than the source. Mono-Safe pins this at 0% and greys the slider out. 0 to 200%." },
        fr: { t: "Largeur",
              b: "Dose le canal latéral du signal fini : 0 % le replie en mono, 100 % laisse l'image telle que les moteurs l'ont faite, et 200 % l'élargit au-delà de la source. Mono-Sûr fixe ce réglage à 0 % et grise le curseur. 0 à 200 %.",
              reviewed: false },
    },

    'tip.preDelay': {
        en: { t: "Pre-Delay",
              b: "Holds the signal back before either engine sees it, which reads as depth and distance rather than as an echo. Raise Feedback alongside it to turn the same line into a short repeat. 0 to 50 ms." },
        fr: { t: "Pré-Délai",
              b: "Retient le signal avant que les moteurs ne le reçoivent, ce qui s'entend comme de la profondeur et de la distance plutôt que comme un écho. Montez Retour en même temps pour transformer cette même ligne en courte répétition. 0 à 50 ms.",
              reviewed: false },
    },

    'tip.feedback': {
        en: { t: "Feedback",
              b: "Sends the pre-delay output back into its own input, so each repeat arrives quieter than the last. It has nothing to recirculate while Pre-Delay sits at 0 ms. 0 to 80%." },
        fr: { t: "Retour",
              b: "Réinjecte la sortie du pré-délai dans sa propre entrée : chaque répétition arrive plus faible que la précédente. Il n'a rien à faire recirculer tant que Pré-Délai reste à 0 ms. 0 à 80 %.",
              reviewed: false },
    },

    'tip.monoSafe': {
        en: { t: "Mono-Safe",
              b: "Compresses the side channel so a detuned stack survives a mono fold-down instead of cancelling itself. While it is on, Width is pinned at 0% and its slider is greyed out. Off or on." },
        fr: { t: "Mono-Sûr",
              b: "Comprime le canal latéral pour qu'un empilement désaccordé survive à un repli mono au lieu de s'annuler lui-même. Tant qu'il est actif, Largeur est fixée à 0 % et son curseur est grisé. Désactivé ou activé.",
              reviewed: false },
    },

    'tip.mix': {
        en: { t: "Mix",
              b: "Balances the processed signal against the untouched input, after both engines and before the width stage. Keep it low when you want thickening rather than an effect. 0 to 100%." },
        fr: { t: "Mix",
              b: "Équilibre le signal traité et l'entrée intacte, après les deux moteurs et avant l'étage de largeur. Gardez-le bas quand vous cherchez un épaississement plutôt qu'un effet. 0 à 100 %.",
              reviewed: false },
    },

    // ── The chrome ──────────────────────────────────────────────────────────
    //
    // The gear tip is what tells a user hover-help exists at all, so it must
    // describe what this popover ACTUALLY holds and nothing more. It holds one
    // row. O-Tapestop's wording promises a hover-help toggle; this plugin has
    // none, and a tip that lies is worse than no tip.

    'tip.settings': {
        en: { t: "Settings",
              b: "Opens the settings panel, above the gear. It carries a single control: the language this interface is written in." },
        fr: { t: "Réglages",
              b: "Ouvre le panneau de réglages, au-dessus de la roue dentée. Il ne porte qu'un seul réglage : la langue dans laquelle cette interface est écrite.",
              reviewed: false },
    },

    'tip.language': {
        en: { t: "Interface language",
              b: "Switches every caption, accessible name and hover-help body on this page between English and Français. The value readouts stay as they are — they are numbers and units, which do not translate. The choice is saved with the session." },
        fr: { t: "Langue de l'interface",
              b: "Bascule chaque légende, chaque nom accessible et chaque bulle d'aide de cette page entre English et Français. Les valeurs affichées ne changent pas : ce sont des nombres et des unités, qui ne se traduisent pas. Le choix est enregistré avec la session.",
              reviewed: false },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
// One string per entry, no body: a label is not a tooltip.
//
// ── THE THREE-ARM D-01 TEST, AND WHERE EACH ARM LANDED ON THIS PAGE ─────────
//
// arm 1 (byte-identical to an AudioParameterChoice option) EXEMPTS eleven
//       strings: Sine / Triangle / Random from wobble_shape, Linear / Exp /
//       Random from unison_dist, 60s / 70s / 80s from wobble_era, and the
//       digit captions of unison_voices. See I18N_EXEMPT.
// arm 2 (a number or a unit) exempts every .knob-value readout and the ' Hz'
//       suffix composed in the controller.
// arm 3 (what ELEMENT receives it) exempts #wobble_rate_value's three
//       tempo-sync words — 4 bars / 2 bars / 1 bar — which are English PROSE
//       written into a node that also holds "2.0 Hz". Reasoned in I18N_EXEMPT
//       rather than skipped in silence.
//
// ── THE COLLISION THIS PAGE FOUND, AND WHY 'Random' IS BOTH ─────────────────
//
// check-i18n assertion 10 matches I18N_EXEMPT by TEXT, not by element. The
// word "Random" appears THREE times on this page: as a wobble_shape option, as
// a unison_dist option, and as the CAPTION of the random_amt knob — whose
// parameter is an AudioParameterFloat named "Randomization" and which is
// therefore a plain caption that must localize. The exempt entry needed by the
// two <option> nodes silences the knob caption too. The caption is keyed
// anyway (label.random, below) and IS localized at runtime; the exemption only
// stops assertion 10 asking for coverage it already has. Reported upstream as
// a gate shape finding, with the negative control that proves it.
//
// "Detune" has the same shape and is handled the other way: the .logo span is
// KEYED with sameAsEn: true rather than exempted, so the knob caption "Detune"
// keeps its own coverage requirement. A text exemption there would have hidden
// the one caption on this page most likely to be forgotten.
//
// ── GEOMETRY, MEASURED AT 600 x 480 ────────────────────────────────────────
//
// Three layout rows are content-sized and would move under French, so three
// per-element pins were added and each was reverted alone to confirm it
// re-breaks the gate. They are named in the CSS beside the rule. Full numbers
// are in the commit message; the short form:
//
//   .preset-action-btn   pinned to 60 px so OUVRIR / SAUVER cannot push
//                        #prevPreset, #presetName and #nextPreset leftward
//   .engine-controls .knob   width: 100% so a caption wider than the 52 px
//                        knob face cannot re-centre the face inside its 91 px
//                        grid cell — probed to move ZERO children in English
//   #feedback_knob .knob-label  pinned to 55 px so RETOUR, which is 13 px
//                        NARROWER than Feedback, cannot shrink the auto grid
//                        column and slide the whole output row
//
// THREE of this page's French strings are SHORTER than their English, which is
// the half a clip check is blind to, and two of the three needed a pin BECAUSE
// they shrink: VOIX 37.7 -> 25.8, RETOUR 54.3 -> 42.4, MONO-SÛR 58.9 -> 54.2.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The header ──────────────────────────────────────────────────────────
    //
    // "Ouaricon Detune" is the product name. The brand word is an I18N_EXEMPT
    // entry (it is unique on the page); the product word is KEYED with
    // sameAsEn rather than exempted, because an exemption is matched by text
    // and "Detune" is also the caption of the unison detune knob, which does
    // translate. sameAsEn says "this was looked at and translates to itself"
    // where silence would say nothing.
    'label.productName': { en: { t: 'Detune' }, fr: { t: 'Detune', reviewed: false, sameAsEn: true } },

    // ── The preset bar ──────────────────────────────────────────────────────
    //
    // OUVRIR / SAUVER rather than CHARGER / ENREGISTRER. Both buttons are
    // shrink-to-fit inside a `justify-content: space-between` header, so every
    // pixel a caption gains pushes #prevPreset, #presetName and #nextPreset to
    // the LEFT, toward a 22 px logo that ends at x = 282. The buttons are
    // pinned to 60 px to make the row language-invariant, and CHARGER (47 px
    // of text) plus ENREGISTRER (74 px) would have needed a 66 px pin that
    // leaves a 2 px gap to the logo. OUVRIR and SAUVER are the compact
    // idiomatic pair and fit the 42 px content box with 2 px to spare.
    'label.load': { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: false } },
    'label.save': { en: { t: 'Save' }, fr: { t: 'Sauver', reviewed: false } },

    // ── The wobble engine ───────────────────────────────────────────────────
    //
    // "Pleurage" is the French audio term for tape wow — the exact effect this
    // engine models — where a literal "Oscillation" would name the mechanism
    // and lose the tape. The panel caption is a 191 px block, so length is
    // free here.
    'label.wobble': { en: { t: 'Wobble' }, fr: { t: 'Pleurage',  reviewed: false } },
    'label.era':    { en: { t: 'Era' },    fr: { t: 'Époque',    reviewed: false } },
    'label.shape':  { en: { t: 'Shape' },  fr: { t: 'Forme',     reviewed: false } },
    'label.rate':   { en: { t: 'Rate' },   fr: { t: 'Vitesse',   reviewed: false } },

    // "Sync" is the term in French audio software as well, and this caption
    // sits under a 36 px toggle in a container the toggle sizes: a translation
    // wider than 36 px would grow the container and re-centre the toggle
    // inside its grid cell. Both facts point the same way.
    'label.sync':   { en: { t: 'Sync' },   fr: { t: 'Sync', reviewed: false, sameAsEn: true } },

    // "Ampleur" rather than "Profondeur". Both are correct for modulation
    // depth; Profondeur renders 68 px against a 52 px knob face and would need
    // the caption to overflow its own box, which assertion 4 reads as a spill.
    // Ampleur is 48 px and fits the face with room.
    'label.depth':  { en: { t: 'Depth' },  fr: { t: 'Ampleur',   reviewed: false } },

    // ── The blend control ───────────────────────────────────────────────────
    // The section is sized by the 64 px knob face, not by this caption, so
    // MÉLANGE at 55 px changes nothing.
    'label.blend':  { en: { t: 'Blend' },  fr: { t: 'Mélange',   reviewed: false } },

    // ── The unison engine ───────────────────────────────────────────────────
    'label.unison': { en: { t: 'Unison' }, fr: { t: 'Unisson',   reviewed: false } },
    'label.voices': { en: { t: 'Voices' }, fr: { t: 'Voix',      reviewed: false } },

    // "Dist" is itself an abbreviation of the parameter's display name,
    // "Unison Distribution". The French abbreviates the same way rather than
    // spelling out RÉPARTITION where the English does not.
    'label.dist':   { en: { t: 'Dist' },   fr: { t: 'Répart.',   reviewed: false } },

    // The knob caption, NOT the product name in the logo. This one translates.
    'label.detune': { en: { t: 'Detune' }, fr: { t: 'Désaccord', reviewed: false } },

    // Stereo panning width of the unison voices — distinct from the output
    // Width slider below, which is the stereo image of the whole plugin.
    // Étalement and Largeur keep the two apart in French as Spread and Width
    // do in English.
    'label.spread': { en: { t: 'Spread' }, fr: { t: 'Étalement', reviewed: false } },

    // The per-voice variation knob. Its parameter is an AudioParameterFloat
    // named "Randomization" — NOT one of unison_dist's option strings — so
    // arm 1 does not reach it and it localizes. See the collision note above.
    'label.random': { en: { t: 'Random' }, fr: { t: 'Aléatoire', reviewed: false } },

    // ── The output section ──────────────────────────────────────────────────
    //
    // The colon rides INSIDE the key. French typography sets a space before a
    // colon and English does not, and that difference belongs in the table
    // rather than in the markup, where only one of the two languages could
    // have it. The caption was split out of the node it shared with
    // #width_value per contract §5.
    'label.width':    { en: { t: 'Width:' },  fr: { t: 'Largeur :', reviewed: false } },

    // Pré-Dly mirrors the English abbreviation of "Pre-Delay" rather than
    // spelling out PRÉ-DÉLAI, which is 61 px against a 50 px knob column.
    'label.preDelay': { en: { t: 'Pre-Dly' }, fr: { t: 'Pré-Dly',   reviewed: false } },

    // 13 px NARROWER than the English. The caption is pinned to the English
    // box precisely because of that: the output row's middle columns are
    // `auto`, so a caption that shrinks slides the row exactly as one that
    // grows does.
    'label.feedback': { en: { t: 'Feedback' }, fr: { t: 'Retour',   reviewed: false } },

    // A coined compound mirroring the English one, which is itself coined.
    // "Compatible mono" is the descriptive French but renders 98 px into a
    // 59 px pinned box.
    'label.monoSafe': { en: { t: 'Mono-Safe' }, fr: { t: 'Mono-Sûr', reviewed: false } },

    // "Mix" is the term in French audio software. Kept rather than "Mixage",
    // which is the ACT of mixing rather than the dry/wet control.
    'label.mix':      { en: { t: 'Mix' }, fr: { t: 'Mix', reviewed: false, sameAsEn: true } },

    // ── The settings popover (v1.6.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    //
    // The five entries below are the text of the five native title=
    // attributes v1.5.4 carried, moved verbatim under contract §4 and then
    // translated. Nothing here is new prose.
    'aria.settings':     { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: false } },
    'aria.langSelect':   { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
    'aria.prevPreset':   { en: { t: 'Previous preset' },    fr: { t: 'Préréglage précédent',  reviewed: false } },
    'aria.nextPreset':   { en: { t: 'Next preset' },        fr: { t: 'Préréglage suivant',    reviewed: false } },
    'aria.presetList':   { en: { t: 'Click to see all presets' },
                           fr: { t: 'Cliquez pour voir tous les préréglages', reviewed: false } },
    'aria.loadPreset':   { en: { t: 'Load preset from file' },
                           fr: { t: 'Ouvrir un préréglage depuis un fichier', reviewed: false } },
    'aria.savePreset':   { en: { t: 'Save current settings' },
                           fr: { t: 'Sauver les réglages actuels', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// READ THE COLLISION NOTE ABOVE BEFORE ADDING TO THIS LIST. Assertion 10
// matches these by TEXT, so an entry added for one node silences EVERY node on
// the page carrying the same string.
// ============================================================================

export const I18N_EXEMPT = [

    // ── Names ───────────────────────────────────────────────────────────────
    ['Ouaricon',
     'the brand word of the .logo product name "Ouaricon Detune" — a brand is never translated. Unique on this page, so a text-matched exemption is safe here; the product word beside it is KEYED with sameAsEn instead, because "Detune" is also the unison knob caption'],
    ['Default',
     'the preset NAME rendered in #presetName — exempt under D-02, because the name IS the JSON filename the preset manager reads and writes (OuariconPresetManager createPresetJson / loadPreset). Translating it would ask the loader for a file that does not exist'],

    // ── D-01 arm 1: captions that ARE the AudioParameterChoice option strings
    //
    // Byte-identity is the test. A DAW automation lane showing
    // wobble_shape = "Triangle" beside a page reading "Dents de scie" is a bug
    // report, not a localization.
    ['Sine',
     'a wobble_shape AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:79, StringArray {"Sine","Triangle","Random"}) — D-01 arm 1'],
    ['Triangle',
     'a wobble_shape option string VERBATIM (PluginProcessor.cpp:79) — D-01 arm 1. Also spelled identically in French'],
    ['Random',
     'a wobble_shape AND a unison_dist option string VERBATIM (PluginProcessor.cpp:79 and :113) — D-01 arm 1. SCOPED to option, because the random_amt .knob-label says "Random" too and is NOT exempt: its parameter is an AudioParameterFloat named "Randomization". That caption is keyed as label.random and localizes. Unscoped, this entry silenced it — the collision that put scopes in the contract',
     'option'],
    ['Linear',
     'a unison_dist option string VERBATIM (PluginProcessor.cpp:113, StringArray {"Linear","Exp","Random"}) — D-01 arm 1'],
    ['Exp',
     'a unison_dist option string VERBATIM (PluginProcessor.cpp:113) — D-01 arm 1'],
    ['60s',
     'a wobble_era option string VERBATIM (PluginProcessor.cpp:53, StringArray {"60s","70s","80s"}) — D-01 arm 1, and a decade label besides'],
    ['70s',
     'a wobble_era option string VERBATIM (PluginProcessor.cpp:53) — D-01 arm 1'],
    ['80s',
     'a wobble_era option string VERBATIM (PluginProcessor.cpp:53) — D-01 arm 1'],

    // ── D-01 arm 3: prose written into a READOUT node ───────────────────────
    //
    // #wobble_rate_value holds "2.0 Hz" when tempo sync is off and a musical
    // division when it is on. Three of the eleven divisions are English words.
    // They are NOT keyed, and the reason is arm 3 rather than convenience: the
    // canon's setLabel() sets el.dataset.i18n PERMANENTLY, so keying this node
    // while sync is on would leave it a [data-i18n] element after sync goes
    // off, and the next language change would overwrite the live "2.0 Hz" with
    // a stale "4 mesures". That is exactly the enter-and-leave-the-sweep bug
    // O-Marimba's six timbre words produced.
    ['4 bars',
     'written into the #wobble_rate_value READOUT node by getMusicalDivision() when wobble_sync is on — D-01 arm 3. Keying it would make a readout node a permanent [data-i18n] element and clobber the Hz reading after sync goes off'],
    ['2 bars',
     'a tempo-sync division written into the #wobble_rate_value readout — D-01 arm 3, same reason as "4 bars"'],
    ['1 bar',
     'a tempo-sync division written into the #wobble_rate_value readout — D-01 arm 3, same reason as "4 bars". Also the string that would need a plural engine, which contract §6 declines to build'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper]
//
// applyI18n() runs document.querySelector(selector), then closest(wrapper) if
// a wrapper is given, and writes data-tip-title + data-tip onto whatever that
// resolves to. The wrapper exists because the addressable node is usually NOT
// the thing the user aims at.
//
// ── "BIND TO THE IDS THE UI ALREADY USES" IS TRUE HERE, AND NOT ENOUGH ──────
//
// T17 says to bind the ids the UI already has. On this page every anchor DOES
// have an id — the fifth plugin of the stage where that half is true and the
// first three where it was false. The TARGET half is the one that bites here:
// eleven of the eighteen rows walk to a wrapper, because
//
//   #<name>_knob    is a .knob holding the 52 px face, its caption and its
//                   readout — already the right cell, so the four rows that
//                   name one bare or via .knob-container differ only in how
//                   much padding the hover area gets;
//   #blend_knob     does NOT contain its caption. .blend-label is a SIBLING
//                   inside .blend-section, so the wrapper is what makes
//                   hovering the word "Blend" open the blend tip;
//   #wobble_era     and the other three <select>s do not contain their
//                   .dropdown-label either — same shape, .dropdown-container;
//   #width          is a 6 px slider track. .slider-container adds the caption
//                   and the readout above it;
//   #mono_safe      and #wobble_sync are 36 px switches with their captions
//                   beside them in .toggle-container.
//
// ── TWO ROWS ARE DELIBERATELY BARE ─────────────────────────────────────────
//
// #wobble_rate_knob   its parent .rate-knob-group is a COLUMN holding the rate
//                     knob AND the Sync toggle. Walking up to it would put the
//                     rate tip on the sync switch, which has its own.
//
// #gear-btn and #lang-select   .settings-cluster contains the gear AND the
//                     popover that holds the selector, so a wrapper walk from
//                     either resolves to the same node and hovering the
//                     language selector would open the gear's tip. This is
//                     O-Comp's carried trap, and it applies verbatim here.
//
// ── ONE ANCHOR IS HIDDEN UNTIL A STATE IS DRIVEN ───────────────────────────
//
// #random_amt_container is .random-knob-container, `display: none` until
// unison_dist is set to Random (index.html:519-525). querySelector still finds
// it, so the binding resolves at load and applyI18n writes the attributes;
// tests/ui_tip_render_check.js drives the dropdown before hovering it, and
// tests/i18n-states.json already drove the same state for check-ui-labels.
// ============================================================================

export const TIP_BINDINGS = [

    // ── The wobble engine ───────────────────────────────────────────────────
    ['#wobble_era',          'tip.era',      '.dropdown-container'],
    ['#wobble_shape',        'tip.shape',    '.dropdown-container'],
    ['#wobble_rate_knob',    'tip.rate'],
    ['#wobble_sync',         'tip.sync',     '.toggle-container'],
    ['#wobble_depth_knob',   'tip.depth',    '.knob-container'],

    // ── The blend control ───────────────────────────────────────────────────
    ['#blend_knob',          'tip.blend',    '.blend-section'],

    // ── The unison engine ───────────────────────────────────────────────────
    ['#unison_voices',       'tip.voices',   '.dropdown-container'],
    ['#unison_dist',         'tip.dist',     '.dropdown-container'],
    ['#unison_detune_knob',  'tip.detune',   '.knob-container'],
    ['#unison_spread_knob',  'tip.spread',   '.knob-container'],
    ['#random_amt_knob',     'tip.random',   '.knob-container'],

    // ── The output section ──────────────────────────────────────────────────
    ['#width',               'tip.width',    '.slider-container'],
    ['#delay_knob',          'tip.preDelay', '.knob-container'],
    ['#feedback_knob',       'tip.feedback', '.knob-container'],
    ['#mono_safe',           'tip.monoSafe', '.toggle-container'],
    ['#mix_knob',            'tip.mix',      '.knob-container'],

    // ── The chrome — BARE, see the note above ───────────────────────────────
    ['#gear-btn',            'tip.settings'],
    ['#lang-select',         'tip.language'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE as of v1.7.0: applyI18n() calls it once per TIP_BINDINGS row, on every
// language change. Through v1.6.0 the loop was empty and this function was
// exported unreferenced, so the canon block would stay byte-identical to the
// other forty-two copies; adding the eighteen rows below needed no change to
// its shape, which was the point of exporting it early.
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
