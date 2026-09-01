/*
   This file is part of O-Formant, an Ouaricon Audio plugin.
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
// i18n.js — O-Formant on-page copy, English + French (v1.27.2, canon v2)
//
// ── v1.27.2: ENGLISH DEFECT FOUND BY THE FRENCH (Stage O, 2026-08-31) ───────
// tip.formantSpread said the formants scale "around the first one". Both banks
// (dsp/FormantFilterBank.h:98-107, dsp/CascadeFormantBank.h:106-116) and the
// pad overlay (main.js applyShiftSpread) scale each formant's distance from the
// MEAN of the five shifted frequencies — centerOfMass = sum/5 — so F1 moves too.
// Both bodies now say "their average frequency"; the title and the range
// sentence are unchanged. French rewritten for meaning -> reviewed: false (1).
// Height 94.6 px in both languages before; measured again after (see CHANGELOG).
//
// ── v1.27.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 60 of 182 entries — 29 terminology, 36 typography, 1 grammar
// (agreement), 4 meaning, 2 reasoned exemptions. Entries carrying two
// categories are counted in both, so the columns sum past 60. sameAsEn: kept
// 21, translated 0. termNote exemptions: 2 (listed below). Left as drafted:
// the other 122. reviewed: false throughout — no native speaker yet.
// Lint: 85 at baseline (24 F1, 21 T7, 20 G1, 12 T5, 8 T4) -> 0, --strict exit 0.
//
// DECISIONS THE NEXT READER NEEDS:
//
//   The width defence three captions above USED to carry is BACKWARDS, measured
//   with Range.selectNodeContents on the real node at the shipping 800x600 frame:
//     - Save: the v1.26.0 note called "Sauver" the width driver of the centred
//       preset bar. .preset-save-btn has min-width: 65px and overflow: visible;
//       "Sauver" measures 46.02 px inside it and "Enreg." 41.84 — NARROWER, same
//       65 px box, nothing moves. Only "Enregistrer" (78.78) grows the box, to
//       96.78. The glossary abbreviation was free.
//     - Vib Rate / Vib Depth: the note defended "Ampleur" over "Profondeur" on a
//       55 px cell. The glossary's own abbreviated roots are narrower than what
//       shipped — "Vit. vibrato" 40.95 vs "Vib Vitesse" 41.02, "Prof. vibrato"
//       45.50 vs "Vib Ampleur" 46.72 — and "Prof. vibrato" cannot be confused
//       with the effects rack's standalone "Profondeur", which was the note's
//       actual worry.
//     - Feedback: "Réinjection" 41.50 is narrower than the forbidden
//       "Rétroaction" 42.00 in the same 50 px cell.
//   The one width defence that HOLDS, to the hundredth: "Formant du chanteur" is
//   74.98 px in a 55 px .knob-wrap with a 10 px gap — it overflows 9.99 px per
//   side and clears its neighbour by 0.01 px. "F. chanteur" (39.78) stays.
//
//   ROOT TERMS APPLIED because they fit (55 px cells, nowrap, overflow: visible,
//   10 px gaps): Étalement 36.48, Portamento 41.50, Relâchement 46.98,
//   Inclinaison 40.00, Gigue 22.00, Focalisation 44.00. Harmonique de départ
//   89.42 and Harmonique de fin 75.55 fit the tuning panel's 118 px .gen-row
//   label slot (198 px row less the 80 px input).
//
//   ABBREVIATION KEPT for width: Damp. "Amortissement" measures 55.00 in the
//   effects rack's 50 px cell — it clears the 10 px gap by 7.50 px, but it would
//   be the only caption on the page wider than its own dial column, and O-Prism
//   already ships the glossary's listed "Amort." (25.75). Convergence, not taste.
//
//   TWO termNote EXEMPTIONS, both the same one: the consonant envelope's HOLD
//   stage keeps "Tenue" where the glossary forbids it, because "Maintien" is
//   already this page's Sustain caption three rows away on the same tab. Width
//   is not the reason — Maintien is 32.50 px in the 42 px cell. One French word
//   on two live controls is the N1 correction-11 defect in mirror image.
//
//   FOCUS has no glossary row and "Focale" is the optical focal length, so the
//   caption and tip.vowelFocus now read Focalisation / Focalisation vocalique.
//   Reported for the list to grow rather than settled here.
//
//   LOANWORDS KEPT: Shimmer (glossary), Chorus, Mix, EQ, Mode, Mod, Gain,
//   Transition, Trans, Fric, Lab, Alv, Pal, Auto, Rotation, Divisions, Notes,
//   {n} notes, and canvas "fricative" — 21 straight copies, every one a word
//   French uses, each declared sameAsEn: true or a title over a translated body.
//   JITTER did NOT stay: the glossary settles gigue, so the caption, the tip
//   title and the two cross-references in tip.shimmer's body all moved together.
//   The AudioParameterChoice faces inside French bodies stay English by design
//   (Cascade / Parallel / Hybrid, Normal / PingPong) — I18N_EXEMPT, untouched.
//
//   REGISTER: vous, imperative for the two pad instructions ("Faites glisser"),
//   elliptical "À monter / À activer" elsewhere. Unchanged, and consistent.
//
//   TYPOGRAPHY was applied by a character-level scanner over the STRING VALUES
//   inside fr: { } only — not a regex over the file, not a line-scoped state
//   machine (a `fr: {` inside a comment opens one 17 lines early). 43 literals
//   took 47 U+00A0; the control imported both revisions and confirmed 0 en
//   values, 0 keys, 0 TIP_BINDINGS rows and 0 I18N_EXEMPT entries changed, and
//   that every U+00A0 in the file lies inside a t:/b: string value.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores (critical_binary_data_strips_hyphens), so one
// combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` below is byte-for-byte what
// index.html / main.js / tuning-panel.js carried through v1.25.4, taken from
// scripts/i18n-extract.js's inventory rather than re-typed.
//
// ── WHY FIVE I18N ENTRIES CARRY AN EMPTY BODY ──────────────────────────────
// v1.27.0 adds hover-help: the 57 `tip.*` entries below carry a real body and
// are bound in TIP_BINDINGS. The FIVE entries inherited from v1.26.0 —
// canvas.lyrics, canvas.plosive, canvas.fricative, canvas.mixed and
// js.savePresetAs — are NOT tooltips and keep `b: ''`. They were not given
// bodies, were not bound and were not deleted, per the Stage K batch K4
// decision. Note the gate consequence of the change: while no body existed
// anywhere, check-i18n assertion 2 accepted `TIP_BINDINGS: []`; the first
// authored body makes bindings MANDATORY, and a bodied entry nothing binds
// fails as ORPHANED.
//
// I18N is used here for the strings that are NOT written into a DOM element:
// canvas ctx.fillText prose and one window.prompt caption. Those are read
// through trLabel(), and a trLabel() call
// is invisible to assertion 15's `referenced` set (which collects markup
// attributes, literal setLabel keys, literal .dataset.i18n* writes and
// innerHTML-injected keys, and nothing else). Housing them in LABELS would
// therefore report every one of them as a DEAD KEY. Housing them in I18N is
// legal under the contract as written — assertion 15's dead sweep runs over
// LABELS only — and is the shape adopted repo-wide after Stage K batch K3.
//
// NEITHER GATE CAN SEE A CANVAS STRING. Assertion 10 walks TEXT NODES,
// assertion 12 scans textContent / innerText writes, and ctx.fillText is
// neither. Leaving them in English passes green. They are verified here by a
// fillText-recording probe, en -> fr -> en, with its own negative control.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

export const I18N = Object.freeze({

    // ── Canvas prose (js/main.js) ───────────────────────────────────────────
    // Five fillText/strokeText sites on this page. Three of them paint
    // notation rather than prose and are exempt under D-01 arm 2 — see
    // I18N_EXEMPT below for the IPA glyph tables and the F1..F5 formant
    // markers. The two that paint WORDS are here.

    // The lyrics-mode badge stamped into the top-right of the vowel XY pad
    // while the lyrics engine is driving the cursor (main.js drawXYPad).
    'canvas.lyrics': {
        en: { t: 'LYRICS', b: '' },
        fr: { t: 'PAROLES', b: '', reviewed: true },
    },

    // The manner-of-articulation word in the consonant pad's live readout,
    // "3.0kHz fricative". The number and the unit stay (D-03); the WORD does
    // not, because the axis captions directly above it — Fric / Plos — are
    // [data-i18n] elements and reading French on the axis with English in the
    // readout under it is the exact split this stage exists to close.
    'canvas.plosive':   { en: { t: 'plosive',   b: '' }, fr: { t: 'occlusive', b: '', reviewed: true } },
    'canvas.fricative': { en: { t: 'fricative', b: '' }, fr: { t: 'fricative', b: '', reviewed: true, sameAsEn: true } },
    'canvas.mixed':     { en: { t: 'mixed',     b: '' }, fr: { t: 'mixte',     b: '', reviewed: true } },

    // ── Runtime-composed strings that are not element text ──────────────────

    // window.prompt caption for Save. Not a DOM node, so no [data-i18n]
    // element can own it. See the CHANGELOG note about prompt() itself.
    'js.savePresetAs': {
        en: { t: 'Save preset as:', b: '' },
        fr: { t: 'Enregistrer le préréglage sous :', b: '', reviewed: true },
    },

    // ── HOVER-HELP (v1.27.0, Stage M batch M3) ──────────────────────────────
    //
    // 55 parameter tips + 2 chrome tips = 57, one per CONTROL on this page.
    // Two of the controls carry two parameters each — the vowel pad is vowelX
    // and vowelY, the consonant pad is consonantTone (Place) and sibilance
    // (Manner) — so 57 parameters reach 55 tips. Seven of the plugin's 64
    // parameters have no control at all and are listed in the v1.27.0
    // CHANGELOG entry; a tip with nothing to bind to is an ORPHAN and fails
    // check-i18n assertion 2, so none was authored.
    //
    // TITLES ARE THE DUMP'S `name` COLUMN, VERBATIM. Every caption on this page
    // that differs from its parameter's name differs by TRUNCATION — "Voice Q"
    // for Voice Quality, "Atk" for Cons Attack, "Pre-dly" for Reverb Pre-delay
    // — and a truncation is not a disagreement (M2 finding 9). A 260 px tooltip
    // is exactly where the full name belongs, and the full name is also what
    // the host automation lane shows.
    //
    // RANGES COME FROM THE PAGE'S OWN FORMATTER, which is updateKnobVisual() at
    // js/main.js:887-911: it prints formatValue(getScaledValue()) and appends
    // `props.label` ONLY when the parameter declares one. 22 of the 64
    // parameters declare a unit; the other 42 render a BARE NUMBER, so their
    // bodies state a bare numeric range. No unit is invented for them.
    //
    // FRENCH BODIES TAKE FRENCH CONVENTION — decimal comma, U+2212 for the
    // minus. The READOUTS keep their point and their English unit: D-03 exempts
    // the readout NODE and that has not moved. A tooltip body is prose.
    //
    // AudioParameterChoice OPTION WORDS STAY ENGLISH INSIDE A FRENCH BODY —
    // Cascade / Parallel / Hybrid, Normal / PingPong — because the control
    // itself keeps them (D-01 arm 1, and they are I18N_EXEMPT below). A French
    // body naming a French option the selector does not offer would be a tip
    // that lies. The sentences AROUND them are French.

    // ── Vowel Morph pad (vowelX + vowelY) ───────────────────────────────────
    'tip.vowelPad': {
        en: { t: 'Vowel Morph',
              b: 'Drag the cursor to morph continuously between the vowels laid out on the pad; '
               + 'left to right is Vowel X and bottom to top is Vowel Y. The IPA glyphs mark where '
               + 'each cardinal vowel sits and the F1..F5 markers track the formants. '
               + 'Both axes run 0 to 1.' },
        fr: { t: 'Morphose vocalique',
              b: 'Faites glisser le curseur pour passer continûment d’une voyelle à l’autre sur la '
               + 'pastille ; de gauche à droite c’est Vowel X, de bas en haut Vowel Y. Les glyphes '
               + 'API marquent la place de chaque voyelle cardinale et les repères F1..F5 suivent '
               + 'les formants. Les deux axes vont de 0 à 1.', reviewed: true },
    },
    'tip.vowelFocus': {
        en: { t: 'Vowel Focus',
              b: 'Sets how sharply the pad snaps to the nearest cardinal vowel. Low values blend '
               + 'the surrounding vowels smoothly, high values pull the sound onto whichever vowel '
               + 'the cursor is closest to. Range 1 to 6.' },
        fr: { t: 'Focalisation vocalique',
              b: 'Règle la netteté avec laquelle la pastille se cale sur la voyelle cardinale la '
               + 'plus proche. Les valeurs basses fondent les voyelles voisines, les valeurs hautes '
               + 'tirent le son vers celle dont le curseur est le plus près. Plage 1 à 6.',
              reviewed: true },
    },

    // ── Glottal source ──────────────────────────────────────────────────────
    'tip.glottalRd': {
        en: { t: 'Voice Quality',
              b: 'Sets the Rd shape of the Liljencrants-Fant glottal pulse, from a tense, pressed '
               + 'voice at the low end to a relaxed, breathy one at the high end. Reach for it '
               + 'first when a voice sounds too hard or too soft. Range 0.30 to 2.70.' },
        fr: { t: 'Qualité vocale',
              b: 'Règle la forme Rd de l’impulsion glottique de Liljencrants-Fant, d’une voix '
               + 'tendue et pressée en bas à une voix détendue et soufflée en haut. C’est le '
               + 'premier réglage à toucher quand une voix sonne trop dure ou trop molle. '
               + 'Plage 0,30 à 2,70.', reviewed: true },
    },
    'tip.breathiness': {
        en: { t: 'Breathiness',
              b: 'Mixes aspiration noise into the glottal source, the turbulence of air passing an '
               + 'incompletely closed glottis. A little widens the formant bandwidths and softens '
               + 'the tone; a lot turns the voice to a whisper. Range 0 to 1.' },
        fr: { t: 'Souffle',
              b: 'Mélange un bruit d’aspiration à la source glottique, la turbulence de l’air '
               + 'traversant une glotte incomplètement fermée. Un peu élargit les bandes '
               + 'passantes des formants et adoucit le timbre ; beaucoup transforme la voix en chuchotement. '
               + 'Plage 0 à 1.', reviewed: true },
    },
    'tip.vibratoRate': {
        en: { t: 'Vibrato Rate',
              b: 'Speed of the pitch vibrato. Around 5 to 7 Hz is the classical singing range; '
               + 'slower reads as a wobble and faster as a tremble. Range 0.5 to 12 Hz.' },
        fr: { t: 'Vitesse du vibrato',
              b: 'Vitesse du vibrato de hauteur. La plage du chant classique se situe entre 5 et '
               + '7 Hz ; plus lent s’entend comme une oscillation, plus rapide comme un '
               + 'tremblement. Plage 0,5 à 12 Hz.', reviewed: true },
    },
    'tip.vibratoDepth': {
        en: { t: 'Vibrato Depth',
              b: 'How far the vibrato swings the pitch, in cents either side of the note. Operatic '
               + 'vibrato sits near 50 cents; 15 is a light shimmer. Range 0 to 100 cents.' },
        fr: { t: 'Profondeur du vibrato',
              b: 'Amplitude du balancement de hauteur, en cents de part et d’autre de la note. Le '
               + 'vibrato d’opéra tourne autour de 50 cents ; 15 donne un léger frémissement. '
               + 'Plage 0 à 100 cents.', reviewed: true },
    },
    'tip.vibratoDelay': {
        en: { t: 'Vibrato Delay',
              b: 'How long a note is held before the vibrato fades in. Singers start a note straight '
               + 'and add vibrato as it sustains, so a delay makes short notes read as speech rather '
               + 'than song. Range 0 to 2000 ms.' },
        fr: { t: 'Retard du vibrato',
              b: 'Durée pendant laquelle la note est tenue avant l’entrée progressive du vibrato. '
               + 'Les chanteurs attaquent droit et ajoutent le vibrato sur la tenue ; un retard fait '
               + 'donc lire les notes brèves comme de la parole plutôt que du chant. '
               + 'Plage 0 à 2000 ms.', reviewed: true },
    },
    'tip.jitter': {
        en: { t: 'Jitter',
              b: 'Cycle-to-cycle random variation in the glottal period. A small amount is what keeps '
               + 'a synthetic voice from sounding like a buzzer; too much reads as a rough or creaky '
               + 'voice. Range 0 to 1.' },
        fr: { t: 'Gigue',
              b: 'Variation aléatoire de la période glottique d’un cycle à l’autre. Une petite dose '
               + 'empêche une voix de synthèse de sonner comme un vibreur ; trop donne une voix '
               + 'rauque ou craquée. Plage 0 à 1.', reviewed: true },
    },
    'tip.shimmer': {
        en: { t: 'Shimmer',
              b: 'Cycle-to-cycle random variation in the glottal amplitude, the loudness counterpart '
               + 'of Jitter. Raise it alongside Jitter for an older or unsteady voice. '
               + 'Range 0 to 1.' },
        fr: { t: 'Shimmer',
              b: 'Variation aléatoire de l’amplitude glottique d’un cycle à l’autre, l’équivalent en '
               + 'intensité de la Gigue. À monter avec la Gigue pour une voix âgée ou mal assurée. '
               + 'Plage 0 à 1.', reviewed: true },
    },
    'tip.rdModDepth': {
        en: { t: 'Rd Mod Depth',
              b: 'How much pitch, velocity and MPE pressure push the Voice Quality Rd around while a '
               + 'note plays. At zero the voice keeps one fixed quality; higher values make loud, high '
               + 'notes press and quiet ones relax. Range 0 to 1.' },
        fr: { t: 'Profondeur de modulation Rd',
              b: 'Degré auquel la hauteur, la vélocité et la pression MPE font varier le Rd de la '
               + 'Qualité vocale pendant la note. À zéro la voix garde une qualité fixe ; plus haut, '
               + 'les notes fortes et aiguës se pressent et les notes douces se détendent. '
               + 'Plage 0 à 1.', reviewed: true },
    },
    'tip.spectralTilt': {
        en: { t: 'Spectral Tilt',
              b: 'Tilts the overall spectrum of the source, darkening it below zero and brightening it '
               + 'above. Use it to place the voice in a mix without touching the formants. '
               + 'Range −12 to +12 dB.' },
        fr: { t: 'Inclinaison spectrale',
              b: 'Incline le spectre général de la source : plus sombre en dessous de zéro, plus clair '
               + 'au-dessus. Sert à placer la voix dans un mixage sans toucher aux formants. '
               + 'Plage −12 à +12 dB.', reviewed: true },
    },

    // ── Consonant ───────────────────────────────────────────────────────────
    'tip.consonantPad': {
        en: { t: 'Consonant Place and Manner',
              b: 'Drag to shape the consonant noise. Left to right sets Place, from labial through '
               + 'alveolar and palatal to velar; bottom to top sets Manner, from plosive to '
               + 'fricative. The corner readout shows the noise centre frequency and the manner it '
               + 'lands in. Both axes run 0 to 1.' },
        fr: { t: 'Lieu et mode de la consonne',
              b: 'Faites glisser pour façonner le bruit de consonne. De gauche à droite le lieu '
               + 'd’articulation, du labial à l’alvéolaire, au palatal puis au vélaire ; de bas en '
               + 'haut le mode, de l’occlusive à la fricative. L’affichage dans le coin donne la '
               + 'fréquence centrale du bruit et le mode obtenu. Les deux axes vont de 0 à 1.',
              reviewed: true },
    },
    'tip.consonantLevel': {
        en: { t: 'Consonant Level',
              b: 'Loudness of the consonant noise relative to the voiced sound. At zero the plugin '
               + 'sings pure vowels. Range 0 to 2.' },
        fr: { t: 'Niveau de consonne',
              b: 'Intensité du bruit de consonne par rapport au son voisé. À zéro le plugin ne '
               + 'chante que des voyelles. Plage 0 à 2.', reviewed: true },
    },
    'tip.consonantVoicing': {
        en: { t: 'Voicing',
              b: 'Blends the consonant between voiceless and voiced — the difference between an s and '
               + 'a z, or a p and a b. Range 0 to 1.' },
        fr: { t: 'Voisement',
              b: 'Fait passer la consonne du non-voisé au voisé — la différence entre un s et un z, ou '
               + 'entre un p et un b. Plage 0 à 1.', reviewed: true },
    },
    'tip.autoConsonant': {
        en: { t: 'Auto Consonant',
              b: 'When on, every note-on fires a consonant before the vowel instead of waiting for one '
               + 'to be triggered. Turn it on to play a whole consonant-and-vowel syllable from a '
               + 'single key. Off or On.' },
        fr: { t: 'Consonne auto',
              b: 'Quand il est activé, chaque note déclenche une consonne avant la voyelle au lieu '
               + 'd’attendre un déclenchement. À activer pour jouer une syllabe consonne-voyelle '
               + 'complète depuis une seule touche. Arrêt ou Marche.', reviewed: true },
    },
    'tip.consonantAttack': {
        en: { t: 'Cons Attack',
              b: 'Rise time of the consonant noise envelope. Short values give a plosive click, longer '
               + 'ones an approach that reads as a fricative. Range 1 to 100 ms.' },
        fr: { t: 'Attaque de consonne',
              b: 'Temps de montée de l’enveloppe du bruit de consonne. Les valeurs brèves donnent un '
               + 'claquement occlusif, les plus longues une approche qui s’entend comme une '
               + 'fricative. Plage 1 à 100 ms.', reviewed: true },
    },
    'tip.consonantHold': {
        en: { t: 'Cons Hold',
              b: 'How long the consonant noise stays at full level before it decays. Fricatives hold; '
               + 'plosives barely do. Range 0 to 200 ms.' },
        fr: { t: 'Tenue de consonne',
              b: 'Durée pendant laquelle le bruit de consonne reste au niveau plein avant de '
               + 'décroître. Les fricatives tiennent ; les occlusives à peine. Plage 0 à 200 ms.',
              reviewed: true,
              termNote: 'the consonant envelope\'s HOLD stage, named for its caption '
                      + 'label.hold; Maintien is this page\'s Sustain' },
    },
    'tip.consonantDecay': {
        en: { t: 'Cons Decay',
              b: 'Fall time of the consonant noise envelope into the vowel that follows. '
               + 'Range 5 to 200 ms.' },
        fr: { t: 'Déclin de consonne',
              b: 'Temps de descente de l’enveloppe du bruit de consonne vers la voyelle qui suit. '
               + 'Plage 5 à 200 ms.', reviewed: true },
    },
    'tip.consonantTransition': {
        en: { t: 'Transition',
              b: 'How strongly the consonant pulls the second and third formants toward its own locus '
               + 'as the vowel begins — the cue that tells a listener which consonant they heard. At '
               + 'zero the formants jump straight to the vowel. Range 0 to 1.' },
        fr: { t: 'Transition',
              b: 'Force avec laquelle la consonne tire les deuxième et troisième formants vers son '
               + 'propre locus au début de la voyelle — l’indice qui dit à l’auditeur quelle consonne '
               + 'il a entendue. À zéro les formants sautent directement à la voyelle. '
               + 'Plage 0 à 1.', reviewed: true },
    },

    // ── Character ───────────────────────────────────────────────────────────
    'tip.formantTopology': {
        en: { t: 'Formant Topology',
              b: 'Chooses how the five formant resonators are wired. Cascade is the Klatt series '
               + 'chain and is the most natural for vowels, Parallel gives each formant its own gain '
               + 'and suits noisier sounds, Hybrid runs both. Cascade, Parallel or Hybrid.' },
        fr: { t: 'Topologie des formants',
              b: 'Choisit le câblage des cinq résonateurs de formant. Cascade est la chaîne série de '
               + 'Klatt, la plus naturelle pour les voyelles ; Parallel donne à chaque formant son '
               + 'propre gain et convient aux sons plus bruités ; Hybrid combine les deux. Cascade, '
               + 'Parallel ou Hybrid.', reviewed: true },
    },
    'tip.formantShift': {
        en: { t: 'Formant Shift',
              b: 'Moves every formant up or down together without changing the pitch. Down reads as a '
               + 'larger body and a deeper throat, up as a smaller one. '
               + 'Range −24 to +24 semitones.' },
        fr: { t: 'Décalage des formants',
              b: 'Déplace tous les formants ensemble vers le haut ou le bas sans toucher à la '
               + 'hauteur. Vers le bas cela s’entend comme un corps plus grand et une gorge plus '
               + 'profonde, vers le haut comme un corps plus petit. '
               + 'Plage −24 à +24 demi-tons.', reviewed: true },
    },
    'tip.formantSpread': {
        en: { t: 'Formant Spread',
              b: 'Scales how far each of the five formants sits from their average frequency. '
               + 'Below 1 crowds them together and thickens the vowel; above 1 opens them out. '
               + 'Range 0.50 to 2.00.' },
        fr: { t: 'Étalement des formants',
              b: 'Multiplie la distance de chacun des cinq formants à leur fréquence moyenne. '
               + 'En dessous de 1 ils se resserrent et la voyelle s’épaissit ; au-dessus de 1 '
               + 'ils s’écartent. Plage 0,50 à 2,00.', reviewed: false },
    },
    'tip.pitchGlide': {
        en: { t: 'Pitch Glide',
              b: 'Time taken to slide from the previous note to the new one, the portamento of the '
               + 'voice. At zero every note starts on pitch. Range 0 to 1000 ms.' },
        fr: { t: 'Portamento de hauteur',
              b: 'Temps mis pour glisser de la note précédente à la nouvelle, le portamento de la '
               + 'voix. À zéro chaque note démarre directement sur sa hauteur. Plage 0 à 1000 ms.', reviewed: true },
    },
    'tip.transitionTime': {
        en: { t: 'Transition Time',
              b: 'How quickly the formant filters move when the vowel changes. Low values snap '
               + 'between vowels, high values smear one into the next the way a real vocal tract '
               + 'does. Range 0 to 1.' },
        fr: { t: 'Temps de transition',
              b: 'Rapidité avec laquelle les filtres de formant se déplacent au changement de '
               + 'voyelle. Les valeurs basses sautent d’une voyelle à l’autre, les valeurs hautes '
               + 'fondent l’une dans l’autre comme le fait un vrai conduit vocal. Plage 0 à 1.', reviewed: true },
    },
    'tip.singersFormant': {
        en: { t: 'Singer’s Formant',
              b: 'Pulls the third, fourth and fifth formants into a cluster near 3 kHz and lifts it — '
               + 'the resonance trained singers use to carry over an orchestra. Raise it when the '
               + 'voice disappears in a dense mix. Range 0 to 1.' },
        fr: { t: 'Formant du chanteur',
              b: 'Rassemble les troisième, quatrième et cinquième formants en un amas autour de 3 kHz '
               + 'et le rehausse — la résonance qui permet aux chanteurs formés de passer par-dessus '
               + 'un orchestre. À monter quand la voix disparaît dans un mixage dense. '
               + 'Plage 0 à 1.', reviewed: true },
    },
    'tip.nasalCoupling': {
        en: { t: 'Nasality',
              b: 'Opens the velum, coupling the nasal cavity to the vocal tract and adding its '
               + 'pole-zero pair. Needed for m, n and ng; a little on a vowel reads as a head cold. '
               + 'Range 0 to 1.' },
        fr: { t: 'Nasalité',
              b: 'Ouvre le voile du palais, couplant la cavité nasale au conduit vocal et ajoutant sa '
               + 'paire pôle-zéro. Indispensable pour m, n et ng ; un peu sur une voyelle s’entend '
               + 'comme un rhume. Plage 0 à 1.', reviewed: true },
    },
    'tip.nasalPlace': {
        en: { t: 'Nasal Place',
              b: 'Moves the nasal resonance along the tract, from m at the low end through n to ng at '
               + 'the high end. Only audible while Nasality is above zero. Range 0 to 1.' },
        fr: { t: 'Lieu nasal',
              b: 'Déplace la résonance nasale le long du conduit, de m en bas à ng en haut en passant '
               + 'par n. Audible seulement quand la Nasalité est au-dessus de zéro. '
               + 'Plage 0 à 1.', reviewed: true },
    },

    // ── Envelope and output ─────────────────────────────────────────────────
    'tip.attack': {
        en: { t: 'Attack',
              b: 'Time the note takes to reach full level after a key is pressed. '
               + 'Range 0.001 to 5 s.' },
        fr: { t: 'Attaque',
              b: 'Temps que met la note à atteindre son niveau plein après l’enfoncement d’une '
               + 'touche. Plage 0,001 à 5 s.', reviewed: true },
    },
    'tip.decay': {
        en: { t: 'Decay',
              b: 'Time the note takes to fall from full level to the Sustain level. '
               + 'Range 0.001 to 5 s.' },
        fr: { t: 'Déclin',
              b: 'Temps que met la note à redescendre du niveau plein au niveau de Maintien. '
               + 'Plage 0,001 à 5 s.', reviewed: true },
    },
    'tip.sustain': {
        en: { t: 'Sustain',
              b: 'Level the note holds while the key stays down, as a fraction of full level. '
               + 'Range 0 to 1.' },
        fr: { t: 'Maintien',
              b: 'Niveau que tient la note tant que la touche reste enfoncée, en fraction du niveau '
               + 'plein. Plage 0 à 1.', reviewed: true },
    },
    'tip.release': {
        en: { t: 'Release',
              b: 'Time the note takes to fade to silence after the key is let go. '
               + 'Range 0.001 to 10 s.' },
        fr: { t: 'Relâchement',
              b: 'Temps que met la note à s’éteindre après le relâchement de la touche. '
               + 'Plage 0,001 à 10 s.', reviewed: true },
    },
    'tip.outputGain': {
        en: { t: 'Output Gain',
              b: 'Final level of the plugin, applied after the effects rack. '
               + 'Range −60 to +12 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Niveau final du plugin, appliqué après le rack d’effets. '
               + 'Plage −60 à +12 dB.', reviewed: true },
    },
    'tip.stereoWidth': {
        en: { t: 'Stereo Width',
              b: 'Spreads the voices across the stereo field by note number, low notes to the left and '
               + 'high notes to the right. At zero every voice sits in the centre. Range 0 to 1.' },
        fr: { t: 'Largeur stéréo',
              b: 'Répartit les voix dans le champ stéréo selon le numéro de note, les graves à gauche '
               + 'et les aigus à droite. À zéro toutes les voix sont au centre. Plage 0 à 1.',
              reviewed: true },
    },

    // ── Effects: chorus ─────────────────────────────────────────────────────
    'tip.chorusBypass': {
        en: { t: 'Chorus Bypass',
              b: 'Switches the chorus in and out of the signal path. The button reads On while the '
               + 'chorus is running and Off while it is bypassed, so the caption and the parameter '
               + 'name are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement du chorus',
              b: 'Insère ou retire le chorus du trajet du signal. Le bouton affiche Marche quand le '
               + 'chorus fonctionne et Arrêt quand il est contourné : la légende et le nom du '
               + 'paramètre sont inversés à dessein. Arrêt ou Marche.', reviewed: true },
    },
    'tip.chorusRate': {
        en: { t: 'Chorus Rate',
              b: 'Speed of the chorus delay modulation. Slow settings drift, fast ones warble. '
               + 'Range 0.1 to 10 Hz.' },
        fr: { t: 'Vitesse du chorus',
              b: 'Vitesse de modulation du retard du chorus. Les réglages lents dérivent, les rapides '
               + 'chevrotent. Plage 0,1 à 10 Hz.', reviewed: true },
    },
    'tip.chorusDepth': {
        en: { t: 'Chorus Depth',
              b: 'How far the chorus modulates its delay time, which is how much detuning you hear. '
               + 'Range 0 to 1.' },
        fr: { t: 'Profondeur du chorus',
              b: 'Amplitude de la modulation du temps de retard, c’est-à-dire le désaccord que l’on '
               + 'entend. Plage 0 à 1.', reviewed: true },
    },
    'tip.chorusMix': {
        en: { t: 'Chorus Mix',
              b: 'Balance between the dry voice and the chorused copy. At zero the chorus is inaudible '
               + 'even while it is running. Range 0 to 1.' },
        fr: { t: 'Mix du chorus',
              b: 'Équilibre entre la voix directe et la copie traitée par le chorus. À zéro le chorus '
               + 'reste inaudible même en fonctionnement. Plage 0 à 1.', reviewed: true },
    },

    // ── Effects: delay ──────────────────────────────────────────────────────
    'tip.delayBypass': {
        en: { t: 'Delay Bypass',
              b: 'Switches the delay in and out of the signal path. The button reads On while the '
               + 'delay is running and Off while it is bypassed, so the caption and the parameter '
               + 'name are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement du délai',
              b: 'Insère ou retire le délai du trajet du signal. Le bouton affiche Marche quand le '
               + 'délai fonctionne et Arrêt quand il est contourné : la légende et le nom du '
               + 'paramètre sont inversés à dessein. Arrêt ou Marche.', reviewed: true },
    },
    'tip.delayTime': {
        en: { t: 'Delay Time',
              b: 'Time between the voice and its first echo. Range 0.001 to 2 s.' },
        fr: { t: 'Durée du délai',
              b: 'Temps entre la voix et son premier écho. Plage 0,001 à 2 s.', reviewed: true },
    },
    'tip.delayFeedback': {
        en: { t: 'Delay Feedback',
              b: 'How much of each echo is fed back to make the next one, which sets how many repeats '
               + 'you hear. The ceiling stops short of 1 so the line cannot run away. '
               + 'Range 0 to 0.95.' },
        fr: { t: 'Réinjection du délai',
              b: 'Part de chaque écho réinjectée pour produire le suivant, ce qui fixe le nombre de '
               + 'répétitions entendues. Le plafond s’arrête avant 1 pour que la ligne ne s’emballe '
               + 'pas. Plage 0 à 0,95.', reviewed: true },
    },
    'tip.delayMode': {
        en: { t: 'Delay Mode',
              b: 'Normal sends both channels through the same delay line. PingPong cross-feeds them so '
               + 'the repeats alternate between left and right. Normal or PingPong.' },
        fr: { t: 'Mode de délai',
              b: 'Normal envoie les deux canaux dans la même ligne à retard. PingPong les croise pour '
               + 'que les répétitions alternent entre la gauche et la droite. Normal ou PingPong.',
              reviewed: true },
    },
    'tip.delayMix': {
        en: { t: 'Delay Mix',
              b: 'Balance between the dry voice and the delayed copy. At zero the delay is inaudible '
               + 'even while it is running. Range 0 to 1.' },
        fr: { t: 'Mix du délai',
              b: 'Équilibre entre la voix directe et la copie retardée. À zéro le délai reste '
               + 'inaudible même en fonctionnement. Plage 0 à 1.', reviewed: true },
    },

    // ── Effects: reverb ─────────────────────────────────────────────────────
    'tip.reverbBypass': {
        en: { t: 'Reverb Bypass',
              b: 'Switches the reverb in and out of the signal path. The button reads On while the '
               + 'reverb is running and Off while it is bypassed, so the caption and the parameter '
               + 'name are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement de la réverb',
              b: 'Insère ou retire la réverbération du trajet du signal. Le bouton affiche Marche '
               + 'quand la réverb fonctionne et Arrêt quand elle est contournée : la légende et le '
               + 'nom du paramètre sont inversés à dessein. Arrêt ou Marche.', reviewed: true },
    },
    'tip.reverbSize': {
        en: { t: 'Reverb Size',
              b: 'Size of the simulated room, which sets how long the tail rings on. Range 0 to 1.' },
        fr: { t: 'Taille de la réverb',
              b: 'Taille de la salle simulée, ce qui fixe la longueur de la queue de réverbération. '
               + 'Plage 0 à 1.', reviewed: true },
    },
    'tip.reverbDamp': {
        en: { t: 'Reverb Damping',
              b: 'How fast the high frequencies are absorbed in the tail. Low values give a bright, '
               + 'tiled room; high values a soft, curtained one. Range 0 to 1.' },
        fr: { t: 'Amortissement de la réverb',
              b: 'Rapidité d’absorption des aigus dans la queue. Les valeurs basses donnent une salle '
               + 'claire et carrelée, les valeurs hautes une salle douce et tendue de rideaux. '
               + 'Plage 0 à 1.', reviewed: true },
    },
    'tip.reverbPredelay': {
        en: { t: 'Reverb Pre-delay',
              b: 'Gap between the dry voice and the start of the reverb tail. A few tens of '
               + 'milliseconds keeps the words intelligible inside a long reverb. '
               + 'Range 0 to 200 ms.' },
        fr: { t: 'Pré-délai de la réverb',
              b: 'Intervalle entre la voix directe et le début de la queue de réverbération. Quelques '
               + 'dizaines de millisecondes gardent les mots intelligibles dans une réverb longue. '
               + 'Plage 0 à 200 ms.', reviewed: true },
    },
    'tip.reverbMod': {
        en: { t: 'Reverb Modulation',
              b: 'Modulates the comb delay lengths with a slow LFO bank so the tail cannot ring on '
               + 'fixed resonances. A little removes the metallic colouration from a long tail. '
               + 'Range 0 to 1.' },
        fr: { t: 'Modulation de la réverb',
              b: 'Module la longueur des lignes en peigne avec un banc de LFO lents pour que la queue '
               + 'ne s’installe pas sur des résonances fixes. Un peu suffit à retirer la coloration '
               + 'métallique d’une queue longue. Plage 0 à 1.', reviewed: true },
    },
    'tip.reverbShimmer': {
        en: { t: 'Reverb Shimmer',
              b: 'Feeds an octave-up copy of the tail back into the reverb, so it climbs as it decays. '
               + 'Range 0 to 1.' },
        fr: { t: 'Shimmer de la réverb',
              b: 'Réinjecte dans la réverb une copie de la queue transposée à l’octave supérieure, si '
               + 'bien qu’elle monte en s’éteignant. Plage 0 à 1.', reviewed: true },
    },
    'tip.reverbMix': {
        en: { t: 'Reverb Mix',
              b: 'Balance between the dry voice and the reverb. At zero the reverb is inaudible even '
               + 'while it is running. Range 0 to 1.' },
        fr: { t: 'Mix de la réverb',
              b: 'Équilibre entre la voix directe et la réverbération. À zéro la réverb reste '
               + 'inaudible même en fonctionnement. Plage 0 à 1.', reviewed: true },
    },

    // ── Effects: EQ ─────────────────────────────────────────────────────────
    'tip.eqBypass': {
        en: { t: 'EQ Bypass',
              b: 'Switches the equaliser in and out of the signal path. The button reads On while the '
               + 'EQ is running and Off while it is bypassed, so the caption and the parameter name '
               + 'are inverted on purpose. Off or On.' },
        fr: { t: 'Contournement de l’EQ',
              b: 'Insère ou retire l’égaliseur du trajet du signal. Le bouton affiche Marche quand '
               + 'l’EQ fonctionne et Arrêt quand il est contourné : la légende et le nom du paramètre '
               + 'sont inversés à dessein. Arrêt ou Marche.', reviewed: true },
    },
    'tip.eqLowGain': {
        en: { t: 'EQ Low Gain',
              b: 'Cut or boost of the low shelf, hinged at 200 Hz. Range −12 to +12 dB.' },
        fr: { t: 'Gain grave de l’EQ',
              b: 'Atténuation ou accentuation du plateau grave, articulé à 200 Hz. '
               + 'Plage −12 à +12 dB.', reviewed: true },
    },
    'tip.eqMidGain': {
        en: { t: 'EQ Mid Gain',
              b: 'Cut or boost of the mid peaking band, centred on EQ Mid Freq. '
               + 'Range −12 to +12 dB.' },
        fr: { t: 'Gain médium de l’EQ',
              b: 'Atténuation ou accentuation de la cloche médium, centrée sur la Fréq. méd. de l’EQ. '
               + 'Plage −12 à +12 dB.', reviewed: true },
    },
    'tip.eqMidFreq': {
        en: { t: 'EQ Mid Freq',
              b: 'Centre frequency of the mid peaking band. Most of the vowel character sits between '
               + '500 and 3000 Hz. Range 200 to 8000 Hz.' },
        fr: { t: 'Fréq. médium de l’EQ',
              b: 'Fréquence centrale de la cloche médium. L’essentiel du caractère des voyelles se '
               + 'situe entre 500 et 3000 Hz. Plage 200 à 8000 Hz.', reviewed: true },
    },
    'tip.eqHighGain': {
        en: { t: 'EQ High Gain',
              b: 'Cut or boost of the high shelf, hinged at 8 kHz. Range −12 to +12 dB.' },
        fr: { t: 'Gain aigu de l’EQ',
              b: 'Atténuation ou accentuation du plateau aigu, articulé à 8 kHz. '
               + 'Plage −12 à +12 dB.', reviewed: true },
    },

    // ── Lyrics ──────────────────────────────────────────────────────────────
    'tip.lyricsEnabled': {
        en: { t: 'Lyrics Enabled',
              b: 'Hands the vowel and consonant cursors to the lyrics engine, which steps through the '
               + 'ARPABET phonemes typed above on every note. The two pads stop following the mouse '
               + 'while it is on. Off or On.' },
        fr: { t: 'Paroles activées',
              b: 'Confie les curseurs de voyelle et de consonne au moteur de paroles, qui parcourt à '
               + 'chaque note les phonèmes ARPABET saisis au-dessus. Les deux pastilles cessent de '
               + 'suivre la souris tant qu’il est actif. Arrêt ou Marche.', reviewed: true },
    },

    // ── Chrome ──────────────────────────────────────────────────────────────
    //
    // The gear body describes ONLY what this popover actually contains. It
    // opens BELOW the button (.settings-popover is top: 34px), and it holds the
    // language selector and nothing else — there is no hover-help toggle on
    // this plugin, so O-Tapestop's wording would be a tip that lies.
    'tip.gear': {
        en: { t: 'Settings',
              b: 'Opens the settings panel just below this button. It holds the interface language '
               + 'and nothing else.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages juste sous ce bouton. Il ne contient que la langue de '
               + 'l’interface.', reviewed: true },
    },
    'tip.language': {
        en: { t: 'Language',
              b: 'Switches every label, heading, button caption and hover-help entry on the page '
               + 'between English and French. Value readouts keep their English number format and '
               + 'unit symbols. English or Français.' },
        fr: { t: 'Langue',
              b: 'Fait passer de l’anglais au français toutes les étiquettes, tous les titres, toutes '
               + 'les légendes de bouton et toute l’aide contextuelle de la page. Les valeurs '
               + 'affichées gardent leur format numérique et leurs unités en anglais. English ou '
               + 'Français.', reviewed: true },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.26.0, canon v2)
// ============================================================================
//
// One string per key, rendered into a fixed cell that mostly does not wrap.
// `.knob-label` is `white-space: nowrap` inside a 55 px (42 px on the
// consonant envelope, 50 px in the effects rack) `.knob-wrap`, so a French
// caption is measured against ~3.84 px per character at 9 px Garamond and the
// collision threshold with the neighbouring cell is width + gap, not width.
//
// FRENCH IS SIZED, NOT SHRUNK. D-04 forbids an auto-shrink font and a
// short-variant fallback: there is exactly ONE French string per key here and
// nothing chooses between variants at runtime. Where the natural French did
// not fit, the shorter phrasing was chosen once, here, and it is the phrasing
// the plugin ships in.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'label.subtitle':      { en: { t: 'vocal synthesizer' }, fr: { t: 'synthétiseur vocal', reviewed: true } },

    // ── Settings popover (v1.26.0) ──────────────────────────────────────────
    'label.settings':      { en: { t: 'Settings' },   fr: { t: 'Réglages',  reviewed: true } },
    'label.language':      { en: { t: 'Language' },   fr: { t: 'Langue',    reviewed: true } },

    // ── Preset bar ──────────────────────────────────────────────────────────
    // "Enreg." at v1.27.1, and the v1.26.0 note this replaces was backwards.
    // .preset-save-btn carries min-width: 65px, so the box is 65 px in BOTH
    // languages whatever the caption; the intrinsic border-box widths are SAVE
    // 46.83, SAUVER 64.02, ENREG. 59.84. The abbreviation is 4.18 px NARROWER
    // than the word it replaces and nothing moved. Only "Enregistrer" (96.78
    // border-box) would break the pin. The pin itself stays load-bearing —
    // without it SAVE and ENREG. differ by 13.01 px and re-centre the cluster.
    // The prompt this button opens still says "Enregistrer le préréglage
    // sous :" in full; it has no box.
    'label.save':          { en: { t: 'Save' },       fr: { t: 'Enreg.',    reviewed: true } },
    // The "no filter" sentinel of #preset-category. Its VALUE is the string
    // "all" and that is what populateCategories() and the change handler
    // compare, so the visible text is free to change.
    'label.allCategories': { en: { t: 'All' },        fr: { t: 'Toutes',    reviewed: true } },

    // ── Tabs ────────────────────────────────────────────────────────────────
    'label.tabSynth':      { en: { t: 'Synth' },      fr: { t: 'Synthé',    reviewed: true } },
    'label.tabLyrics':     { en: { t: 'Lyrics' },     fr: { t: 'Paroles',   reviewed: true } },
    'label.tabTuning':     { en: { t: 'Tuning' },     fr: { t: 'Accord',    reviewed: true } },
    'label.tabEffects':    { en: { t: 'Effects' },    fr: { t: 'Effets',    reviewed: true } },

    // ── Synth tab: vowel pad and glottal source ─────────────────────────────
    'label.vowelMorph':    { en: { t: 'Vowel Morph' },    fr: { t: 'Morphose vocalique', reviewed: true } },
    'label.glottalSource': { en: { t: 'Glottal Source' }, fr: { t: 'Source glottique',   reviewed: true } },
    // glottalRd drives the LF model's Rd shape, which IS the voice quality.
    'label.voiceQ':        { en: { t: 'Voice Q' },    fr: { t: 'Qualité',   reviewed: true } },
    'label.breath':        { en: { t: 'Breath' },     fr: { t: 'Souffle',   reviewed: true } },
    'label.vibRate':       { en: { t: 'Vib Rate' },   fr: { t: 'Vit. vibrato', reviewed: true } },
    // "Prof. vibrato" at v1.27.1. The v1.26.0 note chose "Ampleur" to keep the
    // 55 px cell clear of the effects rack's standalone "Profondeur"; the
    // glossary's abbreviated root answers both worries at once — it cannot be
    // confused with the bare "Profondeur" and it is NARROWER than what shipped
    // (45.50 against "Vib Ampleur" 46.72; "Vit. vibrato" 40.95 against
    // "Vib Vitesse" 41.02). Measured on this page, nowrap, at 800 x 600.
    'label.vibDepth':      { en: { t: 'Vib Depth' },  fr: { t: 'Prof. vibrato', reviewed: true } },
    'label.vibDelay':      { en: { t: 'Vib Delay' },  fr: { t: 'Vib Retard',  reviewed: true } },
    // Shimmer stays — the glossary carries it as a loanword the French audio
    // press uses. Jitter does NOT: the glossary settles gigue, so the caption,
    // tip.jitter's title and the two cross-references inside tip.shimmer's body
    // moved together at v1.27.1. French voice-science papers do write "jitter",
    // and that argument was weighed and lost to one suite-wide word (22.00 px
    // against 18.00 in a 55 px cell, so width had nothing to say).
    'label.jitter':        { en: { t: 'Jitter' },     fr: { t: 'Gigue',     reviewed: true } },
    'label.shimmer':       { en: { t: 'Shimmer' },    fr: { t: 'Shimmer',   reviewed: true, sameAsEn: true } },
    'label.rdMod':         { en: { t: 'Rd Mod' },     fr: { t: 'Mod Rd',    reviewed: true } },
    'label.tilt':          { en: { t: 'Tilt' },       fr: { t: 'Inclinaison', reviewed: true } },

    // ── Synth tab: consonant ────────────────────────────────────────────────
    'label.consonant':     { en: { t: 'Consonant' },  fr: { t: 'Consonne',  reviewed: true } },
    'label.level':         { en: { t: 'Level' },      fr: { t: 'Niveau',    reviewed: true } },
    'label.voicing':       { en: { t: 'Voicing' },    fr: { t: 'Voisement', reviewed: true } },
    'label.auto':          { en: { t: 'Auto' },       fr: { t: 'Auto',      reviewed: true, sameAsEn: true } },
    // Place and manner of articulation, abbreviated to fit the 8 px overlay on
    // the consonant pad. The French terms are labial / alvéolaire / palatal /
    // vélaire and fricative / occlusive, so four of the six abbreviate the
    // same way and two do not.
    'label.placeLabial':     { en: { t: 'Lab' },  fr: { t: 'Lab',  reviewed: true, sameAsEn: true } },
    'label.placeAlveolar':   { en: { t: 'Alv' },  fr: { t: 'Alv',  reviewed: true, sameAsEn: true } },
    'label.placePalatal':    { en: { t: 'Pal' },  fr: { t: 'Pal',  reviewed: true, sameAsEn: true } },
    'label.placeVelar':      { en: { t: 'Vel' },  fr: { t: 'Vél',  reviewed: true } },
    'label.mannerFricative': { en: { t: 'Fric' }, fr: { t: 'Fric', reviewed: true, sameAsEn: true } },
    'label.mannerPlosive':   { en: { t: 'Plos' }, fr: { t: 'Occl', reviewed: true } },
    // The consonant envelope column: 42 px cells, the tightest on the page.
    'label.attackShort':   { en: { t: 'Atk' },    fr: { t: 'Att',   reviewed: true } },
    // "Tenue", not the glossary's "Maintien": Maintien is already this page's
    // Sustain caption (label.sustain, 32.50 px in the 55 px ADSR cell three
    // rows away and visible at the same time), and one French word on two
    // different controls is the N1 correction-11 defect in mirror image.
    // Width is not the reason — Maintien measures 32.50 px in this 42 px cell.
    'label.hold':          { en: { t: 'Hold' },   fr: { t: 'Tenue', reviewed: true,
                             termNote: 'the consonant envelope\'s HOLD stage; Maintien is already '
                                     + 'label.sustain on this same tab, and two controls sharing '
                                     + 'one French name is a defect' } },
    'label.transShort':    { en: { t: 'Trans' },  fr: { t: 'Trans', reviewed: true, sameAsEn: true } },

    // ── Synth tab: character ────────────────────────────────────────────────
    'label.character':     { en: { t: 'Character' },  fr: { t: 'Caractère', reviewed: true } },
    'label.topology':      { en: { t: 'Topology' },   fr: { t: 'Topologie', reviewed: true } },
    'label.shift':         { en: { t: 'Shift' },      fr: { t: 'Décalage',  reviewed: true } },
    'label.spread':        { en: { t: 'Spread' },     fr: { t: 'Étalement', reviewed: true } },
    'label.glide':         { en: { t: 'Glide' },      fr: { t: 'Portamento', reviewed: true } },
    'label.transition':    { en: { t: 'Transition' }, fr: { t: 'Transition', reviewed: true, sameAsEn: true } },
    'label.focus':         { en: { t: 'Focus' },      fr: { t: 'Focalisation', reviewed: true } },
    // "Formant du chanteur" measures 74.98 px in this 55 px .knob-wrap, which
    // is shrink-to-fit with overflow: visible — it would overhang 9.99 px per
    // side into a 10.00 px gap and clear its neighbour by 0.01 px. This is the
    // one width defence on the page that HELD when Stage N re-measured it.
    // "F. chanteur" (39.78) stays, and it is what a French singing-synthesis
    // UI uses anyway.
    'label.singersFormant':{ en: { t: "Singer's F" }, fr: { t: 'F. chanteur', reviewed: true } },
    'label.nasality':      { en: { t: 'Nasality' },   fr: { t: 'Nasalité',  reviewed: true } },
    'label.nasalPlace':    { en: { t: 'Nasal Place' }, fr: { t: 'Lieu nasal', reviewed: true } },

    // ── Synth tab: envelope and output ──────────────────────────────────────
    'label.envelope':      { en: { t: 'Envelope' },   fr: { t: 'Enveloppe', reviewed: true } },
    'label.attack':        { en: { t: 'Attack' },     fr: { t: 'Attaque',   reviewed: true } },
    // Shared by the ADSR decay (55 px) and the consonant decay (42 px):
    // "Déclin" is 6 characters and fits both, so one key, one string.
    'label.decay':         { en: { t: 'Decay' },      fr: { t: 'Déclin',    reviewed: true } },
    'label.sustain':       { en: { t: 'Sustain' },    fr: { t: 'Maintien',  reviewed: true } },
    'label.release':       { en: { t: 'Release' },    fr: { t: 'Relâchement', reviewed: true } },
    'label.output':        { en: { t: 'Output' },     fr: { t: 'Sortie',    reviewed: true } },
    'label.gain':          { en: { t: 'Gain' },       fr: { t: 'Gain',      reviewed: true, sameAsEn: true } },
    'label.width':         { en: { t: 'Width' },      fr: { t: 'Largeur',   reviewed: true } },

    // ── Effects tab ─────────────────────────────────────────────────────────
    'label.chorus':        { en: { t: 'Chorus' },     fr: { t: 'Chorus',    reviewed: true, sameAsEn: true } },
    'label.delay':         { en: { t: 'Delay' },      fr: { t: 'Délai',     reviewed: true } },
    'label.reverb':        { en: { t: 'Reverb' },     fr: { t: 'Réverb',    reviewed: true } },
    'label.eq':            { en: { t: 'EQ' },         fr: { t: 'EQ',        reviewed: true, sameAsEn: true } },
    // The four bypass buttons' two faces. Written from script, so they go
    // through setLabel() and the element becomes a [data-i18n] element from
    // that moment on — a raw literal there is stranded in the previous
    // language the instant the selector fires.
    'label.on':            { en: { t: 'On' },         fr: { t: 'Marche',    reviewed: true } },
    'label.off':           { en: { t: 'Off' },        fr: { t: 'Arrêt',     reviewed: true } },
    'label.rate':          { en: { t: 'Rate' },       fr: { t: 'Vitesse',   reviewed: true } },
    'label.depth':         { en: { t: 'Depth' },      fr: { t: 'Profondeur', reviewed: true } },
    'label.mix':           { en: { t: 'Mix' },        fr: { t: 'Mix',       reviewed: true, sameAsEn: true } },
    'label.time':          { en: { t: 'Time' },       fr: { t: 'Durée',     reviewed: true } },
    'label.feedback':      { en: { t: 'Feedback' },   fr: { t: 'Réinjection', reviewed: true } },
    // Shared by the delay-mode caption and the tuning panel's rotation-table
    // column header: one word, identical in both languages, one key.
    'label.mode':          { en: { t: 'Mode' },       fr: { t: 'Mode',      reviewed: true, sameAsEn: true } },
    'label.size':          { en: { t: 'Size' },       fr: { t: 'Taille',    reviewed: true } },
    'label.damp':          { en: { t: 'Damp' },       fr: { t: 'Amort.',    reviewed: true } },
    'label.preDelay':      { en: { t: 'Pre-dly' },    fr: { t: 'Pré-délai', reviewed: true } },
    'label.mod':           { en: { t: 'Mod' },        fr: { t: 'Mod',       reviewed: true, sameAsEn: true } },
    'label.low':           { en: { t: 'Low' },        fr: { t: 'Grave',     reviewed: true } },
    'label.mid':           { en: { t: 'Mid' },        fr: { t: 'Médium',    reviewed: true } },
    'label.midFreq':       { en: { t: 'Mid Freq' },   fr: { t: 'Fréq. méd.', reviewed: true } },
    'label.high':          { en: { t: 'High' },       fr: { t: 'Aigu',      reviewed: true } },

    // ── Lyrics tab ──────────────────────────────────────────────────────────
    'label.arpabetInput':  { en: { t: 'ARPABET Input' }, fr: { t: 'Saisie ARPABET', reviewed: true } },
    'label.enable':        { en: { t: 'Enable' },     fr: { t: 'Activer',   reviewed: true } },
    'label.loop':          { en: { t: 'Loop' },       fr: { t: 'Boucle',    reviewed: true } },
    // "Réinit.", not "Réinitialiser": 13 characters against 5 grew
    // .lyrics-controls by 45 px and dragged four elements left. The full
    // sentence survives on the button's accessible name (aria.resetLyrics),
    // which has no box to fit.
    'label.reset':         { en: { t: 'Reset' },      fr: { t: 'Réinit.',   reviewed: true } },
    // The help line under the ARPABET box is ONE text node in v1.25.4 holding
    // two captions around two runs of phoneme codes. Split into two keyed
    // spans so applyLabel cannot delete the codes with them; the code runs
    // themselves are I18N_EXEMPT notation.
    'label.vowels':        { en: { t: 'Vowels:' },     fr: { t: 'Voyelles :', reviewed: true } },
    'label.consonants':    { en: { t: 'Consonants:' }, fr: { t: 'Consonnes :', reviewed: true } },
    'label.syllables':     { en: { t: 'Syllables' },   fr: { t: 'Syllabes',  reviewed: true } },
    'label.tuningPanelFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: true },
    },

    // ── Tuning tab (js/tuning-panel.js) ─────────────────────────────────────
    //
    // scripts/i18n-extract.js:442 drops `tuning-panel.js` from the WORKLIST by
    // filename with no ownership test, so none of the keys below appears in
    // this plugin's inventory. The file is nevertheless O-Formant's own copy —
    // its header says so, it is 45 lines diverged from
    // modules/tuning/scala-tuning-engine/js/tuning-panel.js, and O-Formant has
    // no dependencies.json listing that module — so localizing it here does
    // not reach another plugin and /module-upgrade will not revert it.
    //
    // check-i18n DOES reach it: its pageModules set is derived from the js
    // directory, so assertions 12, 13 and 15 scan this file like any other.
    'tuning.intervals':    { en: { t: 'Intervals ({n} notes)' },
                             fr: { t: 'Intervalles ({n} notes)', reviewed: true } },
    'tuning.tonic':        { en: { t: 'Tonic' },      fr: { t: 'Tonique',   reviewed: true } },
    // The note count under each library row. "notes" is the same word in
    // French, so the entry exists to KEY the node rather than to change it:
    // an unkeyed node here is indistinguishable from one somebody forgot, and
    // this template is an `html +=` accumulator, which assertion 12 cannot
    // read at all.
    'tuning.noteCount':    { en: { t: '{n} notes' },  fr: { t: '{n} notes', reviewed: true, sameAsEn: true } },
    'tuning.vizCircle':    { en: { t: 'Circle' },     fr: { t: 'Cercle',    reviewed: true } },
    'tuning.vizPolar':     { en: { t: 'Polar' },      fr: { t: 'Polaire',   reviewed: true } },
    'tuning.vizMatrix':    { en: { t: 'Matrix' },     fr: { t: 'Matrice',   reviewed: true } },
    'tuning.vizTrueKeys':  { en: { t: 'True Keys' },  fr: { t: 'Touches',   reviewed: true } },
    'tuning.vizRotation':  { en: { t: 'Rotation' },   fr: { t: 'Rotation',  reviewed: true, sameAsEn: true } },
    'tuning.scaleIntervals': { en: { t: 'Scale Intervals' },
                               fr: { t: 'Intervalles de la gamme', reviewed: true } },
    'tuning.tkHint':       { en: { t: 'Hold 2+ notes to see intervals' },
                             fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: true } },
    'tuning.library':      { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque', reviewed: true } },
    // The library filter <option>s. Their VALUE is matched against the
    // `category` field of getEmbeddedTuningList(), so only the text moves.
    'tuning.catAll':       { en: { t: 'All Categories' },   fr: { t: 'Toutes catégories', reviewed: true } },
    'tuning.catHistorical':{ en: { t: 'Historical' },       fr: { t: 'Historiques',   reviewed: true } },
    'tuning.catJust':      { en: { t: 'Just Intonation' },  fr: { t: 'Intonation juste', reviewed: true } },
    'tuning.catEqual':     { en: { t: 'Equal Divisions' },  fr: { t: 'Divisions égales', reviewed: true } },
    'tuning.catNonOctave': { en: { t: 'Non-Octave' },       fr: { t: 'Non octaviantes', reviewed: true } },
    'tuning.catWorld':     { en: { t: 'World' },            fr: { t: 'Du monde',      reviewed: true } },
    // A4 stays: it is scientific pitch notation, and the French octave
    // numbering for the same pitch is La3, which would silently rename the
    // reference the .scl / .kbm files are written against.
    'tuning.a4Ref':        { en: { t: 'A4 REF' },      fr: { t: 'RÉF. A4',   reviewed: true } },
    'tuning.stretch':      { en: { t: 'Stretch' },     fr: { t: 'Étirement', reviewed: true } },
    'tuning.loadScl':      { en: { t: 'Load .SCL' },   fr: { t: 'Ouvrir .SCL', reviewed: true } },
    'tuning.loadKbm':      { en: { t: 'Load .KBM' },   fr: { t: 'Ouvrir .KBM', reviewed: true } },
    'tuning.saveScl':      { en: { t: 'Save .SCL' },   fr: { t: 'Enreg. .SCL', reviewed: true } },
    'tuning.saveKbm':      { en: { t: 'Save .KBM' },   fr: { t: 'Enreg. .KBM', reviewed: true } },
    'tuning.exportHtml':   { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: true } },
    'tuning.generateScale':{ en: { t: 'Generate Scale' }, fr: { t: 'Générer une gamme', reviewed: true } },
    'tuning.genEdo':       { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: true } },
    'tuning.genHarmonic':  { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique', reviewed: true } },
    'tuning.genRank2':     { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: true } },
    'tuning.divisions':    { en: { t: 'Divisions' },      fr: { t: 'Divisions', reviewed: true, sameAsEn: true } },
    'tuning.period':       { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: true } },
    'tuning.startHarmonic':{ en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: true } },
    'tuning.endHarmonic':  { en: { t: 'End Harmonic' },   fr: { t: 'Harmonique de fin', reviewed: true } },
    'tuning.generator':    { en: { t: 'Generator (c)' },  fr: { t: 'Générateur (c)', reviewed: true } },
    'tuning.notes':        { en: { t: 'Notes' },          fr: { t: 'Notes',    reviewed: true, sameAsEn: true } },
    'tuning.generate':     { en: { t: 'Generate' },       fr: { t: 'Générer',  reviewed: true } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is the accessible NAME. A screen reader in French reading
    // an English name is the same failure as a French page with an English
    // caption. None of these has a rendered box, so none is a geometry risk.
    // The four below replace the four native title= attributes v1.25.4
    // carried: their text is MOVED, not re-authored (contract §4).
    'aria.presetPrev':     { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: true } },
    'aria.presetNext':     { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: true } },
    'aria.loopToggle':     { en: { t: 'Toggle loop' },     fr: { t: 'Activer ou désactiver la boucle', reviewed: true } },
    'aria.resetLyrics':    { en: { t: 'Reset to first syllable' },
                             fr: { t: 'Revenir à la première syllabe', reviewed: true } },
    'aria.langSelect':     { en: { t: 'Interface language' },
                             fr: { t: 'Langue de l’interface', reviewed: true } },
    'placeholder.lyrics':  { en: { t: 'Type ARPABET phonemes separated by spaces (e.g. HH AH L OW W ER L D)' },
                             fr: { t: 'Saisir des phonèmes ARPABET séparés par des espaces (ex. HH AH L OW W ER L D)',
                                   reviewed: true } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element,
// a setLabel() call, or an entry HERE WITH A REASON. A bare skip list would
// let a missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, scope]. An exemption is matched
// by TEXT, so an unscoped one silences EVERY node carrying that string. A
// scope is REQUIRED where the same string is also keyed on this page —
// assertion 14 enforces it — and is written here for two entries that are not
// strictly ambiguous but sit beside keyed siblings.
// ============================================================================

export const I18N_EXEMPT = [
    // ── Product identity ────────────────────────────────────────────────────
    ['O-Formant', 'the product name — a product name is never translated'],

    // ── Preset names (D-02) ─────────────────────────────────────────────────
    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h), so translating it breaks recall: a session
    // saved against "Cathedral" would not resolve "Cathédrale". "Default" is
    // the placeholder the manager overwrites on its first pass.
    ['Default', 'a factory preset name — the name IS the JSON filename, so translating it breaks recall (D-02)'],

    // ── AudioParameterChoice options, byte-identical (D-01 arm 1) ───────────
    // The page and the host automation lane must agree. Verified verbatim
    // against PluginProcessor.cpp rather than assumed.
    ['Cascade',  'a formantTopology AudioParameterChoice option VERBATIM (PluginProcessor.cpp:218) — D-01 arm 1'],
    ['Parallel', 'a formantTopology AudioParameterChoice option VERBATIM (PluginProcessor.cpp:218) — D-01 arm 1'],
    ['Hybrid',   'a formantTopology AudioParameterChoice option VERBATIM (PluginProcessor.cpp:218) — D-01 arm 1'],
    ['Normal',   'a delayMode AudioParameterChoice option VERBATIM (PluginProcessor.cpp:369) — D-01 arm 1'],
    ['PingPong', 'a delayMode AudioParameterChoice option VERBATIM (PluginProcessor.cpp:369) — D-01 arm 1'],

    // ── Phonetic and scientific notation (D-01 arm 2) ───────────────────────
    // The ARPABET code runs in the lyrics help line. Scoped to .lyrics-help
    // because the two captions that bracket them ARE keyed, and an unscoped
    // entry over a run this long would be the one place a forgotten caption
    // could hide.
    ['AA AE AH AO AW AY EH ER EY IH IY OW OY UH UW |',
     'ARPABET vowel codes — phonetic notation, identical in every language (D-01 arm 2)',
     '.lyrics-help'],
    ['B CH D DH F G HH JH K L M N NG P R S SH T TH V W Y Z ZH',
     'ARPABET consonant codes — phonetic notation, identical in every language (D-01 arm 2)',
     '.lyrics-help'],

    // ── Canvas glyph tables and markers, painted by ctx.fillText ────────────
    // Not reachable by assertion 10 (it walks text nodes) or assertion 12 (it
    // scans textContent writes), so these entries are documentation of a
    // deliberate decision rather than something a gate would otherwise fire
    // on. Written down anyway: the two canvas strings that ARE prose are in
    // I18N above, and an undocumented split between them is exactly how the
    // next reader concludes the rest were forgotten.
    ['i e ɑ o u r l',
     'IPA vowel glyphs painted into the vowel XY pad (main.js vowelLabels) — the International Phonetic Alphabet is notation, not language (D-01 arm 2)'],
    ['p t k f s ʃ m n ŋ',
     'IPA consonant glyphs painted into the consonant XY pad (main.js consonantLabels) — same reason (D-01 arm 2)'],
    ['F1 F2 F3 F4 F5',
     'formant-index markers painted onto the vowel XY pad (main.js drawXYPad) — a letter and a number, language-neutral (D-01 arm 2)'],
    ['Hz',
     'unit symbol in the consonant pad readout, language-neutral (D-03)'],

    // ── Tuning-panel data from the engine ───────────────────────────────────
    // The scale-name display and the library rows render whatever
    // getTuningName() / getEmbeddedTuningList() return. "12-TET Standard" is
    // the authored English fallback that sits in the markup until the first
    // native pull answers; it is an engine tuning NAME, not a caption.
    ['12-TET Standard',
     'the tuning engine\'s own scale name, rendered from getTuningName() — a scale name is data, not a caption'],
];

// ============================================================================
// TIP_BINDINGS — 57 rows (v1.27.0)
// ============================================================================
//
// [selector, key] or [selector, key, wrapper]. applyI18n() calls
// document.querySelector(selector), then closest(wrapper) when a wrapper is
// declared, and writes data-tip-title / data-tip onto whatever it lands on.
//
// THE SELECTOR HALF AND THE TARGET HALF WERE CHECKED SEPARATELY, because they
// fail independently — the naive reading of "bind to the ids the UI already
// uses" has now been wrong on fifteen plugins for a different reason each time:
//
//   SELECTOR half — FALSE for 45 of 57. Not one knob on this page carries an
//   id; they are `.knob-wrap[data-param="…"]`. The twelve id'd anchors are the
//   two canvases, the two toggles, the topology segmented control, the
//   delay-mode <select>, the four effect bypass buttons and the two chrome
//   controls.
//
//   TARGET half — TRUE for 53 of 57, and for the O-Freeze reason: `.knob-wrap`
//   is itself the flex COLUMN holding the dial (55 px, or 42 px in the
//   consonant envelope), its caption and its readout, so the addressable node
//   already IS the cell a user aims at and no closest() walk is needed. Four
//   anchors declare a wrapper: the two XY pads, to pick up the 1 px border and
//   — on the consonant pad — the absolutely-positioned Lab/Alv/Pal/Vél and
//   Fric/Occl overlay that covers the canvas; and the two toggles, whose id is
//   on the 42 x 22 px switch inside a 55 x 35 px cell that also holds its
//   caption.
//
// closest('.toggle-wrap'), NOT querySelector('.toggle-wrap'): the class matches
// TWICE on this page (autoConsonant on the synth tab, lyricsEnabled on the
// lyrics tab), so a bare class query would be right only by document order
// (M2 finding, from O-Tremolo's twice-matching .waveform-section).
//
// THE CHROME BINDS BARE. `.header` holds #gear-btn AND #settings-popover, so a
// wrapper walk from #lang-select would resolve to the header and hand the
// language selector the gear's own tip (M2 finding 7, from O-Comp).
//
// SEVEN PARAMETERS ARE NOT HERE because they have no control on this page:
// consonantVOT, sourceFilterCoupling and the five tuning_* parameters. They are
// host-reachable and page-unreachable; see the v1.27.0 CHANGELOG. A body with
// nothing to bind to is an ORPHAN and fails check-i18n assertion 2, and adding
// a control to satisfy a count is a feature change with a geometry cost.
//
// NOTHING IS BOUND INTO THE TUNING PANEL. js/tuning-panel.js is lazy-imported
// on the first click of the Tuning tab (index.html:1424), so it is absent from
// the DOM when applyI18n() runs and any selector into it would resolve to null
// and warn `i18n: tip target not found` on every load — O-Reed's referencePitch
// trap, and boot-all-uis prints that warning. Force-mounting the panel to
// satisfy a count was not done.
// ============================================================================

export const TIP_BINDINGS = [
    // ── Glottal source (9) ──
    ['.knob-wrap[data-param="glottalRd"]', 'tip.glottalRd'],
    ['.knob-wrap[data-param="breathiness"]', 'tip.breathiness'],
    ['.knob-wrap[data-param="vibratoRate"]', 'tip.vibratoRate'],
    ['.knob-wrap[data-param="vibratoDepth"]', 'tip.vibratoDepth'],
    ['.knob-wrap[data-param="vibratoDelay"]', 'tip.vibratoDelay'],
    ['.knob-wrap[data-param="jitter"]', 'tip.jitter'],
    ['.knob-wrap[data-param="shimmer"]', 'tip.shimmer'],
    ['.knob-wrap[data-param="rdModDepth"]', 'tip.rdModDepth'],
    ['.knob-wrap[data-param="spectralTilt"]', 'tip.spectralTilt'],

    // ── Consonant knobs (6) ──
    ['.knob-wrap[data-param="consonantLevel"]', 'tip.consonantLevel'],
    ['.knob-wrap[data-param="consonantVoicing"]', 'tip.consonantVoicing'],
    ['.knob-wrap[data-param="consonantAttack"]', 'tip.consonantAttack'],
    ['.knob-wrap[data-param="consonantHold"]', 'tip.consonantHold'],
    ['.knob-wrap[data-param="consonantDecay"]', 'tip.consonantDecay'],
    ['.knob-wrap[data-param="consonantTransition"]', 'tip.consonantTransition'],

    // ── Character (8) ──
    ['.knob-wrap[data-param="formantShift"]', 'tip.formantShift'],
    ['.knob-wrap[data-param="formantSpread"]', 'tip.formantSpread'],
    ['.knob-wrap[data-param="pitchGlide"]', 'tip.pitchGlide'],
    ['.knob-wrap[data-param="transitionTime"]', 'tip.transitionTime'],
    ['.knob-wrap[data-param="vowelFocus"]', 'tip.vowelFocus'],
    ['.knob-wrap[data-param="singersFormant"]', 'tip.singersFormant'],
    ['.knob-wrap[data-param="nasalCoupling"]', 'tip.nasalCoupling'],
    ['.knob-wrap[data-param="nasalPlace"]', 'tip.nasalPlace'],

    // ── Envelope and output (6) ──
    ['.knob-wrap[data-param="attack"]', 'tip.attack'],
    ['.knob-wrap[data-param="decay"]', 'tip.decay'],
    ['.knob-wrap[data-param="sustain"]', 'tip.sustain'],
    ['.knob-wrap[data-param="release"]', 'tip.release'],
    ['.knob-wrap[data-param="outputGain"]', 'tip.outputGain'],
    ['.knob-wrap[data-param="stereoWidth"]', 'tip.stereoWidth'],

    // ── Effects: chorus (3) ──
    ['.knob-wrap[data-param="chorusRate"]', 'tip.chorusRate'],
    ['.knob-wrap[data-param="chorusDepth"]', 'tip.chorusDepth'],
    ['.knob-wrap[data-param="chorusMix"]', 'tip.chorusMix'],

    // ── Effects: delay (3) ──
    ['.knob-wrap[data-param="delayTime"]', 'tip.delayTime'],
    ['.knob-wrap[data-param="delayFeedback"]', 'tip.delayFeedback'],
    ['.knob-wrap[data-param="delayMix"]', 'tip.delayMix'],

    // ── Effects: reverb (6) ──
    ['.knob-wrap[data-param="reverbSize"]', 'tip.reverbSize'],
    ['.knob-wrap[data-param="reverbDamp"]', 'tip.reverbDamp'],
    ['.knob-wrap[data-param="reverbPredelay"]', 'tip.reverbPredelay'],
    ['.knob-wrap[data-param="reverbMod"]', 'tip.reverbMod'],
    ['.knob-wrap[data-param="reverbShimmer"]', 'tip.reverbShimmer'],
    ['.knob-wrap[data-param="reverbMix"]', 'tip.reverbMix'],

    // ── Effects: EQ (4) ──
    ['.knob-wrap[data-param="eqLowGain"]', 'tip.eqLowGain'],
    ['.knob-wrap[data-param="eqMidGain"]', 'tip.eqMidGain'],
    ['.knob-wrap[data-param="eqMidFreq"]', 'tip.eqMidFreq'],
    ['.knob-wrap[data-param="eqHighGain"]', 'tip.eqHighGain'],

    // ── The two XY pads — one control, TWO parameters each ──
    // The vowel pad is vowelX + vowelY and the consonant pad is consonantTone
    // (Place) + sibilance (Manner). One hover target cannot carry two tips, so
    // each pad gets ONE tip naming both of its axes — the shape O-AnalogEQ used
    // in M2 for its two concentric rings.
    ['#xy-pad',               'tip.vowelPad',      '.xy-canvas-wrap'],
    ['#consonant-xy-pad',     'tip.consonantPad',  '.consonant-xy-wrap'],

    // ── Non-knob parameter controls ──
    // #topology-control binds BARE. Its wrapper .segmented-wrap is 382 px wide
    // against the control's own 157 px and the extra width is empty row, so a
    // walk would arm a hover area the control does not occupy.
    ['#topology-control',     'tip.formantTopology'],
    ['#delayModeSelect',      'tip.delayMode'],
    ['#autoConsonant-toggle', 'tip.autoConsonant', '.toggle-wrap'],
    ['#lyricsEnabled-toggle', 'tip.lyricsEnabled', '.toggle-wrap'],

    // ── Effect bypass buttons ──
    // Bare: .fx-header holds the button AND the section title, so a walk would
    // put the bypass tip across the whole header row.
    ['#chorusBypassBtn',      'tip.chorusBypass'],
    ['#delayBypassBtn',       'tip.delayBypass'],
    ['#reverbBypassBtn',      'tip.reverbBypass'],
    ['#eqBypassBtn',          'tip.eqBypass'],

    // ── Chrome ──
    ['#gear-btn',             'tip.gear'],
    ['#lang-select',          'tip.language'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. The canon is one shape across
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
