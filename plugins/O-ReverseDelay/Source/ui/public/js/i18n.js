/*
   This file is part of O-ReverseDelay, an Ouaricon Audio plugin.
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
// i18n.js — O-ReverseDelay hover-help copy, English + French (v1.9.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). Two
// independent gates enforce that — scripts/check-i18n.js assertion 7, and
// section 2 of tests/ui_frontend_check.js, which additionally requires init()
// to remain the literal last statement of app.js. That is why app.js calls
// initI18n() from INSIDE init() rather than from a foot-of-file block.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTooltip() in app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
// A line break, if one is ever needed, is \n plus CSS white-space: pre-line,
// never a markup tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below is byte-for-byte
// what index.html carried through v1.7.3. This page authored no HTML entities
// inside a data-tip, so nothing had to be decoded — but if one is ever added,
// it must be decoded here, because setAttribute + textContent do not decode.
//
// D13 — THIS PLUGIN HAS NO HOVER-HELP TOGGLE. Its settings popover carries the
// language selector alone. tests/ui_frontend_check.js asserts by NAME that no
// setTooltipsEnabled native function exists, in app.js or in PluginEditor.cpp;
// that assertion is a locked user decision and is untouched by v1.9.0. There is
// deliberately no 'tips-toggle' key below.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title (the small-caps line), b = tooltip body.
//
// Object.freeze() rather than a bare `{...}` literal for two reasons. It says
// out loud that this module is inert data nothing may mutate at runtime — and
// it keeps the export a SINGLE top-level statement, because a statement written
// `export const X = {...};` closes its brace at depth zero and segments the
// trailing `;` off on its own.
//
// PARAMETERISED ENTRIES carry {token} placeholders substituted by tr()'s `vars`
// argument. They are NOT template literals: the table is inert data, evaluated
// once at module load, so a value interpolated here would be frozen at whatever
// the language happened to be then. O-ReverseDelay needs no parameterised entry
// today; tr() carries the machinery regardless, because the canon is one shape.
export const I18N = Object.freeze({

    // ── The settings popover (v1.9.0) ───────────────────────────────────────
    // The gear and the language selector are new to this plugin. There is no
    // hover-help row here and there must not be one — see the D13 note above.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this hover help. The choice is remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette aide au survol. Le choix est conservé avec la session.',
              reviewed: false },
    },
    // v1.10.0: through v1.9.0 this entry told the user, in both languages, that
    // the labels on the page do not change. That is now false — they do.
    // Rewritten to say what is true, INCLUDING the half that stayed true: value
    // readouts are English in both languages (D-03), so a knob still reads
    // `250 ms`, and preset names are English because the name IS the filename
    // (D-02).
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. English and French are available; value readouts and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de cette aide au survol et des libellés de la page. L’anglais et le français sont disponibles ; les valeurs affichées et les noms de préréglages restent en anglais.',
              reviewed: false },
    },

    // ── TIME ────────────────────────────────────────────────────────────────
    'syncSegments': {
        en: { t: 'Sync Mode',
              b: 'Free reads the delay in milliseconds; Sync locks it to the host\'s tempo grid.' },
        fr: { t: 'Mode de synchronisation',
              b: 'Free lit le délai en millisecondes ; Sync le verrouille sur la grille de tempo de l’hôte.',
              reviewed: false },
    },
    'knob-delayTime': {
        en: { t: 'Delay',
              b: 'How far back the grains reach. Long settings read as separate reversed phrases; short ones fuse into a smear.' },
        fr: { t: 'Délai',
              b: 'Jusqu’où les grains remontent dans le temps. Les réglages longs s’entendent comme des phrases inversées distinctes ; les courts fusionnent en une traînée.',
              reviewed: false },
    },
    'combo-noteDivision': {
        en: { t: 'Division',
              b: 'The note value the delay follows while Sync is lit — dotted (D) and triplet (T) included.' },
        fr: { t: 'Division',
              b: 'La valeur de note que suit le délai lorsque Sync est actif — pointées (D) et triolets (T) compris.',
              reviewed: false },
    },

    // ── GRAIN ───────────────────────────────────────────────────────────────
    'knob-grainSize': {
        en: { t: 'Grain Size',
              b: 'Length of each reversed fragment. Long grains bloom and swell; short grains chatter.' },
        fr: { t: 'Taille de grain',
              b: 'Longueur de chaque fragment inversé. Les grains longs s’épanouissent et enflent ; les grains courts crépitent.',
              reviewed: false },
    },
    'knob-density': {
        en: { t: 'Density',
              b: 'How many grains overlap at once. Sparse settings stutter; dense settings pour.' },
        fr: { t: 'Densité',
              b: 'Nombre de grains qui se superposent à la fois. Les réglages clairsemés bégaient ; les réglages denses ruissellent.',
              reviewed: false },
    },

    // ── FEEDBACK ────────────────────────────────────────────────────────────
    'knob-feedback': {
        en: { t: 'Feedback',
              b: 'How much of the wash returns to the buffer. Each pass re-reverses, so the tail keeps folding back on itself.' },
        fr: { t: 'Réinjection',
              b: 'Quelle part de la nappe retourne dans le tampon. Chaque passage réinverse le signal, si bien que la queue se replie sans cesse sur elle-même.',
              reviewed: false },
    },
    'knob-lowCut': {
        en: { t: 'Low Cut',
              b: 'Trims low frequencies inside the feedback loop — every pass grows lighter.' },
        fr: { t: 'Coupe-bas',
              b: 'Atténue les basses fréquences à l’intérieur de la boucle de réinjection — chaque passage s’allège.',
              reviewed: false },
    },
    'knob-highCut': {
        en: { t: 'High Cut',
              b: 'Trims high frequencies inside the feedback loop — every pass grows darker and further away.' },
        fr: { t: 'Coupe-haut',
              b: 'Atténue les hautes fréquences à l’intérieur de la boucle de réinjection — chaque passage s’assombrit et s’éloigne.',
              reviewed: false },
    },

    // ── OUTPUT ──────────────────────────────────────────────────────────────
    'knob-width': {
        en: { t: 'Width',
              b: 'Spreads grains across the stereo field. At zero they stack in the centre.' },
        fr: { t: 'Largeur',
              b: 'Répartit les grains dans le champ stéréo. À zéro, ils s’empilent au centre.',
              reviewed: false },
    },
    'knob-mix': {
        en: { t: 'Mix',
              b: 'Balance of dry input against the reversed wash. Equal-power, so the total stays level.' },
        fr: { t: 'Mixage',
              b: 'Équilibre entre le signal direct et la nappe inversée. À puissance constante, le niveau total reste stable.',
              reviewed: false },
    },

    // ── RANDOM (v1.1.0) ─────────────────────────────────────────────────────
    'knob-jitter': {
        en: { t: 'Jitter',
              b: 'Scatters the timing of each new grain. At zero the grains arrive on a strict grid, which combs sustained material; raise it and the wash loosens into a cloud.' },
        fr: { t: 'Gigue',
              b: 'Disperse le déclenchement de chaque nouveau grain. À zéro, les grains arrivent sur une grille stricte, ce qui filtre en peigne les sons tenus ; en montant, la nappe se relâche en nuage.',
              reviewed: false },
    },
    'knob-delayScatter': {
        en: { t: 'Scatter',
              b: 'Spreads how far back each grain reaches. Thickens the smear without moving the delay\'s rhythmic anchor, because the average reach is unchanged.' },
        fr: { t: 'Dispersion',
              b: 'Disperse la profondeur temporelle atteinte par chaque grain. Épaissit la traînée sans déplacer l’ancrage rythmique du délai, car la portée moyenne reste inchangée.',
              reviewed: false },
    },
    'knob-sizeRandom': {
        en: { t: 'Size Random',
              b: 'Varies the length of each grain. Jitter alone leaves every grain the same shape; this removes the last of the regularity.' },
        fr: { t: 'Aléa de taille',
              b: 'Fait varier la longueur de chaque grain. La gigue seule laisse à tous les grains la même forme ; ceci en supprime la dernière régularité.',
              reviewed: false },
    },
    'knob-gainRandom': {
        en: { t: 'Gain Random',
              b: 'Varies the level of each grain for depth and shimmer. Applied after the feedback tap, so it never changes how long the tail lasts.' },
        fr: { t: 'Aléa de gain',
              b: 'Fait varier le niveau de chaque grain pour donner de la profondeur et du miroitement. Appliqué après la prise de réinjection, il ne modifie donc jamais la durée de la queue.',
              reviewed: false },
    },

    // ── WINDOW (v1.2.0 / v1.4.0) ────────────────────────────────────────────
    'combo-grainShape': {
        en: { t: 'Shape',
              b: 'The envelope each grain is played through. Hann is the shipped bell; Tukey holds its level and sounds more open; Expo-Decay plucks. Level is matched across all five, so this changes colour and not loudness.' },
        fr: { t: 'Forme',
              b: 'L’enveloppe à travers laquelle chaque grain est joué. Hann est la cloche d’origine ; Tukey tient son niveau et sonne plus ouvert ; Expo-Decay pince. Le niveau est apparié sur les cinq formes : ceci change la couleur, pas le volume.',
              reviewed: false },
    },
    'knob-grainTilt': {
        en: { t: 'Tilt',
              b: 'Moves the envelope\'s peak within the grain. Centre is the symmetric window. Turn up and each grain swells slowly then cuts — backwards-swell-into-a-transient, the reason to reach for a reverse delay. Turn down for a plucked, decaying grain.' },
        fr: { t: 'Inclinaison',
              b: 'Déplace le sommet de l’enveloppe à l’intérieur du grain. Au centre, la fenêtre est symétrique. En montant, chaque grain enfle lentement puis se coupe — une montée à l’envers vers un transitoire, la raison même d’un délai inversé. En descendant, le grain devient pincé et décroissant.',
              reviewed: false },
    },
    'knob-tukeyTaper': {
        en: { t: 'Taper',
              b: 'How much of the Tukey grain is tapered. Low is nearly rectangular — a fast edge, open and gated. 1.00 is the full raised cosine, which is exactly the Hann window. Level is matched across the whole range, so this changes character and not loudness. Applies to the Tukey shape only.' },
        fr: { t: 'Adoucissement',
              b: 'Quelle part du grain Tukey est adoucie. En bas, la fenêtre est presque rectangulaire — une attaque franche, ouverte et abrupte. À 1,00, c’est le cosinus surélevé complet, c’est-à-dire exactement la fenêtre de Hann. Le niveau est apparié sur toute la plage : ceci change le caractère, pas le volume. Ne s’applique qu’à la forme Tukey.',
              reviewed: false },
    },
    'envelopeCell': {
        en: { t: 'Envelope',
              b: 'The amplitude envelope applied to every grain, as Shape, Tilt and Taper currently set it. Time runs left to right across one grain; the dotted line is the halfway point.' },
        fr: { t: 'Enveloppe',
              b: 'L’enveloppe d’amplitude appliquée à chaque grain, telle que Forme, Inclinaison et Adoucissement la règlent actuellement. Le temps se lit de gauche à droite sur un grain ; la ligne pointillée en marque le milieu.',
              reviewed: false },
    },

    // ── COUNT (v1.3.0) ──────────────────────────────────────────────────────
    'knob-grainCount': {
        en: { t: 'Grain Count',
              b: 'The most grains allowed to overlap at once, which Density then scales into. Raise it for a denser, smoother, more reverb-like wash; the shipped setting is 8. Density at zero always gives two overlapping grains whatever this is set to.' },
        fr: { t: 'Nombre de grains',
              b: 'Le nombre maximal de grains autorisés à se superposer, dans lequel Densité vient ensuite se répartir. Montez-le pour une nappe plus dense, plus lisse, plus proche d’une réverbération ; le réglage d’origine est 8. À Densité zéro, il y a toujours deux grains superposés, quelle que soit cette valeur.',
              reviewed: false },
    },
    'grainMeter': {
        en: { t: 'Grain Meter',
              b: 'Grains actually sounding right now, and the overlap Size, Density and Count are producing between them. Reads zero when nothing is playing.' },
        fr: { t: 'Indicateur de grains',
              b: 'Les grains réellement audibles à cet instant, et le taux de superposition que Taille, Densité et Nombre produisent ensemble. Affiche zéro lorsque rien ne joue.',
              reviewed: false },
    },

    // ── MOTION (v1.6.0) ─────────────────────────────────────────────────────
    'freezeSegments': {
        en: { t: 'Freeze',
              b: 'Stops writing into the buffer while the grains keep reading it, so the wash holds indefinitely. Dry passes through untouched, and the buffer resumes capturing where it left off on release.' },
        fr: { t: 'Gel',
              b: 'Interrompt l’écriture dans le tampon pendant que les grains continuent d’y lire, si bien que la nappe se maintient indéfiniment. Le signal direct passe intact, et le tampon reprend sa capture là où il s’était arrêté au relâchement.',
              reviewed: false },
    },
    'knob-direction': {
        en: { t: 'Direction',
              b: 'How many grains play forwards instead of backwards. At zero every grain is reversed. Turn it up and forward grains blend in as a clean delay tap — add Scatter to break them apart into a forward cloud. Level is matched across the whole range.' },
        fr: { t: 'Direction',
              b: 'Combien de grains sont lus à l’endroit plutôt qu’à l’envers. À zéro, tous les grains sont inversés. En montant, les grains à l’endroit se fondent en une répétition nette — ajoutez de la Dispersion pour les éclater en un nuage à l’endroit. Le niveau est apparié sur toute la plage.',
              reviewed: false },
    },
    'knob-regenMakeup': {
        en: { t: 'Regen',
              b: 'Extra gain inside the feedback loop. The topology loses about 7 dB each time round, so at zero even Feedback 100 eventually fades; raise this to reach true endless wash and, past it, self-oscillation into the loop\'s soft clip.' },
        fr: { t: 'Regain',
              b: 'Gain supplémentaire à l’intérieur de la boucle de réinjection. La topologie perd environ 7 dB à chaque tour : à zéro, même une Réinjection à 100 finit par s’éteindre. Montez ce réglage pour atteindre la nappe véritablement infinie et, au-delà, l’auto-oscillation jusqu’à l’écrêtage doux de la boucle.',
              reviewed: false },
    },

    // ── SOURCE / DUCK / DRIFT (v1.7.0) ──────────────────────────────────────
    'sourceSegments': {
        en: { t: 'Source',
              b: 'What each grain reads. Mono sums the input before granulating, so Width spreads copies of one signal. Stereo reads left or right per grain, following that grain\'s position, so a wide source keeps its image through the wash.' },
        fr: { t: 'Source',
              b: 'Ce que lit chaque grain. Mono somme l’entrée avant la granulation : Largeur répartit alors des copies d’un même signal. Stéréo lit à gauche ou à droite selon la position de chaque grain, si bien qu’une source large conserve son image à travers la nappe.',
              reviewed: false },
    },
    'knob-duck': {
        en: { t: 'Duck',
              b: 'Pulls the wash down while the dry signal is playing and lets it swell back in the gaps. At zero the wet is untouched. It never changes how long the tail lasts — only when you hear it.' },
        fr: { t: 'Atténuation dynamique',
              b: 'Abaisse la nappe pendant que le signal direct joue et la laisse remonter dans les silences. À zéro, le signal traité reste intact. Ceci ne change jamais la durée de la queue — seulement le moment où on l’entend.',
              reviewed: false },
    },
    'knob-driftRate': {
        en: { t: 'Drift Rate',
              b: 'How fast the delay time wanders. Slow settings read as tape wow under a long wash; fast ones as vibrato on the tail. Has no effect until Depth is raised.' },
        fr: { t: 'Vitesse de dérive',
              b: 'À quelle vitesse le temps de délai vagabonde. Les réglages lents s’entendent comme le pleurage d’une bande sous une longue nappe ; les rapides, comme un vibrato sur la queue. Sans effet tant que la Profondeur reste à zéro.',
              reviewed: false },
    },
    'knob-driftDepth': {
        en: { t: 'Drift Depth',
              b: 'How far the delay time wanders, as a share of whatever the delay is set to. Each grain is fixed at the moment it starts, so this smears and detunes the tail without ever clicking.' },
        fr: { t: 'Profondeur de dérive',
              b: 'Jusqu’où le temps de délai vagabonde, en proportion du délai réglé. Chaque grain est figé à l’instant où il démarre : ceci étale et désaccorde la queue sans jamais produire de clic.',
              reviewed: false },
    },

    // ── COLOUR (v1.7.2) ─────────────────────────────────────────────────────
    'knob-diffusion': {
        en: { t: 'Diffusion',
              b: 'Smears each repeat as it recirculates, so the tail blurs into a wash instead of restating the grain cloud verbatim. It cannot make the delay louder or push it into feedback — it only rearranges what is already there.' },
        fr: { t: 'Diffusion',
              b: 'Étale chaque répétition à mesure qu’elle recircule, si bien que la queue se fond en nappe au lieu de redire le nuage de grains à l’identique. Elle ne peut ni rendre le délai plus fort ni le pousser à l’emballement — elle ne fait que redistribuer ce qui est déjà là.',
              reviewed: false },
    },
    'knob-drive': {
        en: { t: 'Drive',
              b: 'Saturates the feedback loop at a matched level, so it changes the tail\'s colour rather than its length. Loud repeats compress and dull while quiet ones stay clean, which makes the tail bloom as it decays. Regen Makeup sets how long the tail lasts; this sets what it sounds like.' },
        fr: { t: 'Saturation',
              b: 'Sature la boucle de réinjection à niveau apparié : ceci change la couleur de la queue plutôt que sa durée. Les répétitions fortes se compriment et s’assombrissent tandis que les faibles restent nettes, ce qui fait s’épanouir la queue à mesure qu’elle décroît. Regain règle la durée de la queue ; ceci règle son timbre.',
              reviewed: false },
    },
});

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars]. The selector is the BINDING SITE.
// Every anchor on this page already carries an id — tests/ui_frontend_check.js
// section 14 and tests/ui_tooltip_clamp_check.js both enumerate BY id — so all
// of these are plain '#id' forms and none needs a wrapper.
// ============================================================================
// LABELS — the on-page text (v1.10.0, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 230 px tooltip. LABELS is ON-PAGE copy: one string dropped into a fixed cell
// that does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE ─────────────────────────────────────────────────────────
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption carries ONE key. This page reuses more
// than any other in the suite — SIXTEEN keys — because its tooltip titles were
// authored as the captions they sit under: knob-delayTime (Delay / Délai),
// combo-noteDivision, knob-density, knob-lowCut, knob-highCut, knob-width,
// knob-mix, knob-jitter, knob-delayScatter, combo-grainShape, knob-grainTilt,
// knob-tukeyTaper, knob-direction, knob-regenMakeup, knob-diffusion,
// knob-drive, plus freezeSegments, sourceSegments, syncSegments, knob-feedback
// and settings for the group labels and accessible names that match exactly.
// None of those appears below.
//
// It is deliberately NOT used where only the English matches, and this page has
// six such cases: knob-grainSize's title is "Grain Size" under a caption
// reading "Size"; knob-sizeRandom is "Size Random" under "Size Rnd";
// knob-gainRandom, knob-grainCount, knob-driftRate and knob-driftDepth are the
// same shape. Reusing there would make the next tooltip copy edit a silent
// change to a control.
//
// knob-duck is the case worth naming: its title is "Atténuation dynamique",
// which is the right phrase for a sentence and 20 characters too many for a
// 9.5 px knob caption in a 190 px column.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is what index.html carried through v1.9.0, taken from
// scripts/i18n-extract.js's inventory rather than transcribed, with HTML
// entities decoded to the characters they named (&#183; -> ·) because
// textContent does not decode.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: exactly ONE
// French string per key, and nothing chooses between variants at runtime.
// 940 x 768 is the roomiest frame of the five, and the three specimen rows share
// ONE pinned width contract (190 | 190 | 276 | 190) which French cannot move —
// see CHANGELOG v1.10.0 for what did have to move.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'label.subtitle':  { en: { t: 'Granular Reverse Delay · A Field Guide' },
                         fr: { t: 'Délai inversé granulaire · Guide de terrain', reviewed: false } },

    // ── Preset bar ──────────────────────────────────────────────────────────
    'label.save':      { en: { t: 'Save' },   fr: { t: 'Enreg.',  reviewed: false } },
    'label.load':      { en: { t: 'Load' },   fr: { t: 'Ouvrir',  reviewed: false } },
    'label.delete':    { en: { t: 'Delete' }, fr: { t: 'Suppr.',  reviewed: false } },
    // The armed face of the delete button. It goes through setLabel(), so the
    // element becomes a [data-i18n] element and the language sweep owns it —
    // through v1.9.0 it was a data-confirm ATTRIBUTE, which was the right
    // answer while the page was English-only and the wrong one the moment it
    // had two languages: an attribute holds ONE string, so a language switch
    // mid-arm would have restored the ENGLISH armed face.
    'ui.confirm':      { en: { t: 'Confirm?' }, fr: { t: 'Confirmer ?', reviewed: false } },

    // ── Group headings and captions ─────────────────────────────────────────
    'label.time':      { en: { t: 'Time' },     fr: { t: 'Temps',     reviewed: false } },
    'label.free':      { en: { t: 'Free' },     fr: { t: 'Libre',     reviewed: false } },
    'label.sync':      { en: { t: 'Sync' },     fr: { t: 'Synchro',   reviewed: false } },
    'label.grain':     { en: { t: 'Grain' },    fr: { t: 'Grain',     reviewed: false, sameAsEn: true } },
    'label.size':      { en: { t: 'Size' },     fr: { t: 'Taille',    reviewed: false } },
    'label.amount':    { en: { t: 'Amount' },   fr: { t: 'Quantité',  reviewed: false } },
    'label.output':    { en: { t: 'Output' },   fr: { t: 'Sortie',    reviewed: false } },
    'label.random':    { en: { t: 'Random' },   fr: { t: 'Aléa',      reviewed: false } },
    'label.sizeRnd':   { en: { t: 'Size Rnd' }, fr: { t: 'Aléa taille', reviewed: false } },
    'label.gainRnd':   { en: { t: 'Gain Rnd' }, fr: { t: 'Aléa gain', reviewed: false } },
    'label.window':    { en: { t: 'Window' },   fr: { t: 'Fenêtre',   reviewed: false } },
    // These two do NOT reuse knob-grainTilt / knob-tukeyTaper, and the reason is
    // measured: the WINDOW group's cells are 66 px (every other knob cell on the
    // page is 72), and "Inclinaison" is 71.1 px and "Adoucissement" 91.9. The
    // label gate caught them as an OVERLAP between the two captions, assertion 8
    // — the clip check never saw it, because .knob-label is a shrink-to-fit flex
    // item whose box is always exactly its text. The group's 190 px column is
    // part of the pinned 190|190|276|190 width contract the three specimen rows
    // share, so the cells cannot grow; the words do the work instead. Both are
    // ordinary French for what the control does to a window's shape.
    'label.tilt':      { en: { t: 'Tilt' },     fr: { t: 'Pente',     reviewed: false } },
    'label.taper':     { en: { t: 'Taper' },    fr: { t: 'Biseau',    reviewed: false } },
    'label.count':     { en: { t: 'Count' },    fr: { t: 'Nombre',    reviewed: false } },
    'label.motion':    { en: { t: 'Motion' },   fr: { t: 'Mouvement', reviewed: false } },
    'label.off':       { en: { t: 'Off' },      fr: { t: 'Arrêt',     reviewed: false } },
    'label.mono':      { en: { t: 'Mono' },     fr: { t: 'Mono',      reviewed: false, sameAsEn: true } },
    'label.stereo':    { en: { t: 'Stereo' },   fr: { t: 'Stéréo',    reviewed: false } },
    // The loanword, and the one entry here chosen against a translation that
    // exists. knob-duck's tip says "Atténuation dynamique" and explains what the
    // control does; the caption is the word this technique is called by in a
    // French control room.
    'label.duck':      { en: { t: 'Duck' },     fr: { t: 'Ducking',   reviewed: false } },
    'label.drift':     { en: { t: 'Drift' },    fr: { t: 'Dérive',    reviewed: false } },
    'label.rate':      { en: { t: 'Rate' },     fr: { t: 'Vitesse',   reviewed: false } },
    // "Profondeur" is 72.4 px in a 72 px cell — 0.4 px over, which is a clip
    // rather than a near miss. "Ampleur" is the same idea for the amount of a
    // modulation and is what the tip's fuller phrase can afford to spell out.
    'label.depth':     { en: { t: 'Depth' },    fr: { t: 'Ampleur',   reviewed: false } },
    'label.colour':    { en: { t: 'Colour' },   fr: { t: 'Couleur',   reviewed: false } },

    // ── The grain meter's two captions ──────────────────────────────────────
    // Their VALUE spans are readouts and are never keyed: updateGrainMeter()
    // writes `${active}` and `${overlap.toFixed(1)}×` into siblings, which is
    // exactly the split contract §5 asks for and which this page already had.
    'label.active':    { en: { t: 'Active' },   fr: { t: 'Actifs',    reviewed: false } },
    'label.overlap':   { en: { t: 'Overlap' },  fr: { t: 'Recouvr.',  reviewed: false } },

    'label.footer':    { en: { t: 'Drag vertically · wheel or arrows to trim · double-click to reset' },
                         fr: { t: 'Glisser verticalement · molette ou flèches pour ajuster · double-clic pour réinitialiser', reviewed: false } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. None
    // has a rendered box, so none is a geometry risk.
    'aria.presetPrev':   { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: false } },
    'aria.presetNext':   { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: false } },
    // v1.10.0: this was ALSO false copy. It read "Hover help language" while
    // the control now sets the language of the whole page.
    'aria.langSelect':   { en: { t: 'Interface language' },
                           fr: { t: 'Langue de l’interface', reviewed: false } },
    'aria.noteDivision': { en: { t: 'Note Division' }, fr: { t: 'Division de note', reviewed: false } },
    'aria.grainShape':   { en: { t: 'Grain Shape' },  fr: { t: 'Forme de grain', reviewed: false } },
    'aria.envCanvas':    { en: { t: 'Grain amplitude envelope' },
                           fr: { t: 'Enveloppe d’amplitude de grain', reviewed: false } },
    'aria.sourceMode':   { en: { t: 'Source Mode' },  fr: { t: 'Mode de source', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    // The product name, split across the <h1>'s own text node and the italic
    // .title-accent span, so both halves need an entry. The markup authors HAIR
    // SPACES around the en dash (&#8202;), but the scanner collapses every
    // whitespace run to one U+0020 before it classifies, so this entry carries
    // ORDINARY spaces — exempting the as-authored form instead would silently
    // fail to match and report the product name as an unlocalized label.
    ['O – Reverse', 'half of the product name O–ReverseDelay — a product name is never translated'],
    ['Delay',       'the italic half of the product name O–ReverseDelay, in .title-accent'],

    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall.
    // "Default" is the placeholder the manager overwrites on its first pass.
    ['Default', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],
];

export const TIP_BINDINGS = [
    ['#gear-btn',            'settings'],
    ['#lang-select',         'lang-select'],

    ['#syncSegments',        'syncSegments'],
    ['#knob-delayTime',      'knob-delayTime'],
    ['#combo-noteDivision',  'combo-noteDivision'],

    ['#knob-grainSize',      'knob-grainSize'],
    ['#knob-density',        'knob-density'],

    ['#knob-feedback',       'knob-feedback'],
    ['#knob-lowCut',         'knob-lowCut'],
    ['#knob-highCut',        'knob-highCut'],

    ['#knob-width',          'knob-width'],
    ['#knob-mix',            'knob-mix'],

    ['#knob-jitter',         'knob-jitter'],
    ['#knob-delayScatter',   'knob-delayScatter'],
    ['#knob-sizeRandom',     'knob-sizeRandom'],
    ['#knob-gainRandom',     'knob-gainRandom'],

    ['#combo-grainShape',    'combo-grainShape'],
    ['#knob-grainTilt',      'knob-grainTilt'],
    ['#knob-tukeyTaper',     'knob-tukeyTaper'],
    ['#envelopeCell',        'envelopeCell'],

    ['#knob-grainCount',     'knob-grainCount'],
    ['#grainMeter',          'grainMeter'],

    ['#freezeSegments',      'freezeSegments'],
    ['#knob-direction',      'knob-direction'],
    ['#knob-regenMakeup',    'knob-regenMakeup'],

    ['#sourceSegments',      'sourceSegments'],
    ['#knob-duck',           'knob-duck'],
    ['#knob-driftRate',      'knob-driftRate'],
    ['#knob-driftDepth',     'knob-driftDepth'],

    ['#knob-diffusion',      'knob-diffusion'],
    ['#knob-drive',          'knob-drive'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. O-ReverseDelay needs neither arm
    // today, but the resolving arm is what lets a plugin compose a localized
    // name into a tip without pinning TIP_BINDINGS — which is static data
    // evaluated once — to the load-time language. The canon is one shape across
    // all 43 plugins; this function is not trimmed per plugin.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
