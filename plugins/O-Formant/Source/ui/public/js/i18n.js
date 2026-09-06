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

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

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
    

        'zh-Hans': { t: '歌词',
              b: '',
              reviewed: 'bt' },
    },

    // The manner-of-articulation word in the consonant pad's live readout,
    // "3.0kHz fricative". The number and the unit stay (D-03); the WORD does
    // not, because the axis captions directly above it — Fric / Plos — are
    // [data-i18n] elements and reading French on the axis with English in the
    // readout under it is the exact split this stage exists to close.
    'canvas.plosive':   { en: { t: 'plosive',   b: '' }, fr: { t: 'occlusive', b: '', reviewed: true } , 'zh-Hans': { t: '塞音', b: '', reviewed: 'bt' },},
    'canvas.fricative': { en: { t: 'fricative', b: '' }, fr: { t: 'fricative', b: '', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '擦音', b: '', reviewed: 'bt' },},
    'canvas.mixed':     { en: { t: 'mixed',     b: '' }, fr: { t: 'mixte',     b: '', reviewed: true } , 'zh-Hans': { t: '塞擦音', b: '', reviewed: 'bt' },},

    // ── Runtime-composed strings that are not element text ──────────────────

    // window.prompt caption for Save. Not a DOM node, so no [data-i18n]
    // element can own it. See the CHANGELOG note about prompt() itself.
    'js.savePresetAs': {
        en: { t: 'Save preset as:', b: '' },
        fr: { t: 'Enregistrer le préréglage sous :', b: '', reviewed: true },
    

        'zh-Hans': { t: '预设另存为：',
              b: '',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '元音变形',
              b: '拖动光标即可在面板上排布的各个元音之间连续变形；从左到右是 Vowel X，从下到上是 Vowel Y。国际音标符号标出每个基本元音的位置，F1 至 F5 的标记跟踪各共振峰。两个轴的范围都是 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '元音聚焦',
              b: '设定面板向最近的基本元音吸附的锐利程度。低值把周围的元音平滑地融合在一起，高值把声音拉向光标最靠近的那一个。范围 1 到 6。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '嗓音音质',
              b: '设定 Liljencrants-Fant 声门脉冲的 Rd 形状，低端是紧绷、挤压的嗓音，高端是放松、带气声的嗓音。当一个声音听起来过硬或过软时，先动这个参数。范围 0.30 到 2.70。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '气声度',
              b: '把送气噪声混入声门源，也就是空气通过未完全闭合的声门时产生的湍流。少量会加宽共振峰的带宽并柔化音色；大量则把嗓音变成耳语。范围 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.vibratoRate': {
        en: { t: 'Vibrato Rate',
              b: 'Speed of the pitch vibrato. Around 5 to 7 Hz is the classical singing range; '
               + 'slower reads as a wobble and faster as a tremble. Range 0.5 to 12 Hz.' },
        fr: { t: 'Vitesse du vibrato',
              b: 'Vitesse du vibrato de hauteur. La plage du chant classique se situe entre 5 et '
               + '7 Hz ; plus lent s’entend comme une oscillation, plus rapide comme un '
               + 'tremblement. Plage 0,5 à 12 Hz.', reviewed: true },
    

        'zh-Hans': { t: '颤音速率',
              b: '音高颤音的速度。5 到 7 Hz 左右是古典演唱的范围；更慢听起来像晃动，更快像发抖。范围 0.5 到 12 Hz。',
              reviewed: 'bt' },
    },
    'tip.vibratoDepth': {
        en: { t: 'Vibrato Depth',
              b: 'How far the vibrato swings the pitch, in cents either side of the note. Operatic '
               + 'vibrato sits near 50 cents; 15 is a light shimmer. Range 0 to 100 cents.' },
        fr: { t: 'Profondeur du vibrato',
              b: 'Amplitude du balancement de hauteur, en cents de part et d’autre de la note. Le '
               + 'vibrato d’opéra tourne autour de 50 cents ; 15 donne un léger frémissement. '
               + 'Plage 0 à 100 cents.', reviewed: true },
    

        'zh-Hans': { t: '颤音深度',
              b: '颤音在音符两侧摆动音高的幅度，以音分计。歌剧式颤音接近 50 音分；15 是轻微的闪烁。范围 0 到 100 音分。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '颤音延迟',
              b: '一个音符被持续多久之后颤音才淡入。歌手起音时唱得平直，在延长音上才加入颤音，因此设置延迟会让短音符听起来像说话而不是歌唱。范围 0 到 2000 ms。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '抖动',
              b: '声门周期逐周期的随机变化。少量正是让合成嗓音不像蜂鸣器的原因；过多则听成粗糙或嘶哑的嗓音。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '振幅微扰',
              b: '声门振幅逐周期的随机变化，是抖动在响度上的对应量。要做出苍老或不稳的嗓音，请与抖动一起提高它。范围 0 到 1。',
              termNote: 'the GLOTTAL shimmer — cycle-to-cycle amplitude perturbation of the voice source. The glossary root 微光 is the REVERB sense and is already spent on tip.reverbShimmer on this same plugin\'s effects tab',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: 'Rd 调制深度',
              b: '音高、力度和 MPE 压力在发音过程中推动嗓音音质 Rd 变化的程度。为零时嗓音保持一种固定的音质；数值越高，响亮的高音越挤压，轻柔的音越放松。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '频谱倾斜',
              b: '倾斜声源的整体频谱，零以下变暗，零以上变亮。用它在混音中安置人声而不必触碰共振峰。范围 −12 到 +12 dB。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '辅音的部位与方式',
              b: '拖动即可塑造辅音噪声。从左到右设定发音部位，由双唇经齿龈、硬腭到软腭；从下到上设定发音方式，由塞音到擦音。角落的读数显示噪声的中心频率和所落入的发音方式。两个轴的范围都是 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.consonantLevel': {
        en: { t: 'Consonant Level',
              b: 'Loudness of the consonant noise relative to the voiced sound. At zero the plugin '
               + 'sings pure vowels. Range 0 to 2.' },
        fr: { t: 'Niveau de consonne',
              b: 'Intensité du bruit de consonne par rapport au son voisé. À zéro le plugin ne '
               + 'chante que des voyelles. Plage 0 à 2.', reviewed: true },
    

        'zh-Hans': { t: '辅音电平',
              b: '辅音噪声相对于浊音部分的响度。为零时插件只唱纯元音。范围 0 到 2。',
              reviewed: 'bt' },
    },
    'tip.consonantVoicing': {
        en: { t: 'Voicing',
              b: 'Blends the consonant between voiceless and voiced — the difference between an s and '
               + 'a z, or a p and a b. Range 0 to 1.' },
        fr: { t: 'Voisement',
              b: 'Fait passer la consonne du non-voisé au voisé — la différence entre un s et un z, ou '
               + 'entre un p et un b. Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '清浊度',
              b: '让辅音在清音与浊音之间过渡——也就是 s 与 z、p 与 b 之间的差别。范围 0 到 1。',
              termNote: 'the CONSONANT voicing, voiceless against voiced. The glossary root 和声排列 is chord voicing, a different sense that does not occur on this page',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '自动辅音',
              b: '开启后，每次音符触发都会在元音之前先发一个辅音，而不必等待单独的触发。想用一个琴键弹出完整的辅音加元音音节时就打开它。关或开。',
              reviewed: 'bt' },
    },
    'tip.consonantAttack': {
        en: { t: 'Cons Attack',
              b: 'Rise time of the consonant noise envelope. Short values give a plosive click, longer '
               + 'ones an approach that reads as a fricative. Range 1 to 100 ms.' },
        fr: { t: 'Attaque de consonne',
              b: 'Temps de montée de l’enveloppe du bruit de consonne. Les valeurs brèves donnent un '
               + 'claquement occlusif, les plus longues une approche qui s’entend comme une '
               + 'fricative. Plage 1 à 100 ms.', reviewed: true },
    

        'zh-Hans': { t: '辅音起音',
              b: '辅音噪声包络的上升时间。短值给出塞音式的爆破，较长的值给出听起来像擦音的渐进。范围 1 到 100 ms。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '辅音保持',
              b: '辅音噪声在开始衰减之前停留在满电平的时长。擦音会保持，塞音几乎不保持。范围 0 到 200 ms。',
              reviewed: 'bt' },
    },
    'tip.consonantDecay': {
        en: { t: 'Cons Decay',
              b: 'Fall time of the consonant noise envelope into the vowel that follows. '
               + 'Range 5 to 200 ms.' },
        fr: { t: 'Déclin de consonne',
              b: 'Temps de descente de l’enveloppe du bruit de consonne vers la voyelle qui suit. '
               + 'Plage 5 à 200 ms.', reviewed: true },
    

        'zh-Hans': { t: '辅音衰减',
              b: '辅音噪声包络进入随后那个元音时的下降时间。范围 5 到 200 ms。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '辅音过渡',
              b: '元音起始时，辅音把第二和第三共振峰拉向自身位置的强度——这正是听者据以判断听到了哪个辅音的线索。为零时共振峰直接跳到元音。范围 0 到 1。',
              termNote: 'the CONSONANT transition. This page also carries a Transition Time knob in the Character section, so both sides are qualified rather than one',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '共振峰拓扑',
              b: '选择五个共振峰谐振器的连接方式。Cascade 是 Klatt 的串联链，对元音最自然；Parallel 让每个共振峰有各自的增益，适合噪声更多的声音；Hybrid 两者并用。Cascade、Parallel 或 Hybrid。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '共振峰移位',
              b: '在不改变音高的前提下把所有共振峰一起上移或下移。下移听起来像更大的身体和更深的喉咙，上移则像更小的。范围 −24 到 +24 个半音。',
              reviewed: 'bt' },
    },
    'tip.formantSpread': {
        en: { t: 'Formant Spread',
              b: 'Scales how far each of the five formants sits from their average frequency. '
               + 'Below 1 crowds them together and thickens the vowel; above 1 opens them out. '
               + 'Range 0.50 to 2.00.' },
        fr: { t: 'Étalement des formants',
              b: 'Multiplie la distance de chacun des cinq formants à leur fréquence moyenne. '
               + 'En dessous de 1 ils se resserrent et la voyelle s’épaissit ; au-dessus de 1 '
               + 'ils s’écartent. Plage 0,50 à 2,00.', reviewed: true },
    

        'zh-Hans': { t: '共振峰展宽',
              b: '缩放五个共振峰各自与其平均频率之间的距离。小于 1 时它们靠拢，元音变厚；大于 1 时它们散开。范围 0.50 到 2.00。',
              reviewed: 'bt' },
    },
    'tip.pitchGlide': {
        en: { t: 'Pitch Glide',
              b: 'Time taken to slide from the previous note to the new one, the portamento of the '
               + 'voice. At zero every note starts on pitch. Range 0 to 1000 ms.' },
        fr: { t: 'Portamento de hauteur',
              b: 'Temps mis pour glisser de la note précédente à la nouvelle, le portamento de la '
               + 'voix. À zéro chaque note démarre directement sur sa hauteur. Plage 0 à 1000 ms.', reviewed: true },
    

        'zh-Hans': { t: '音高滑音',
              b: '从上一个音符滑向新音符所用的时间，也就是嗓音的滑音。为零时每个音符都直接从自身的音高开始。范围 0 到 1000 ms。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '过渡时间',
              b: '元音改变时共振峰滤波器移动的快慢。低值在元音之间跳变，高值像真实的声道那样把一个元音抹入下一个。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '歌手共振峰',
              b: '把第三、第四和第五共振峰聚成 3 kHz 附近的一簇并加以提升——这是受过训练的歌手用来穿透乐队的共鸣。当人声在密集的混音中消失时提高它。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '鼻音度',
              b: '打开软腭，把鼻腔耦合到声道并加入它的极点零点对。发 m、n 和 ng 时必需；在元音上加一点听起来像感冒。范围 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.nasalPlace': {
        en: { t: 'Nasal Place',
              b: 'Moves the nasal resonance along the tract, from m at the low end through n to ng at '
               + 'the high end. Only audible while Nasality is above zero. Range 0 to 1.' },
        fr: { t: 'Lieu nasal',
              b: 'Déplace la résonance nasale le long du conduit, de m en bas à ng en haut en passant '
               + 'par n. Audible seulement quand la Nasalité est au-dessus de zéro. '
               + 'Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '鼻音位置',
              b: '让鼻腔共振沿声道移动，低端是 m，经过 n，到高端的 ng。只有当鼻音度大于零时才听得到。范围 0 到 1。',
              reviewed: 'bt' },
    },

    // ── Envelope and output ─────────────────────────────────────────────────
    'tip.attack': {
        en: { t: 'Attack',
              b: 'Time the note takes to reach full level after a key is pressed. '
               + 'Range 0.001 to 5 s.' },
        fr: { t: 'Attaque',
              b: 'Temps que met la note à atteindre son niveau plein après l’enfoncement d’une '
               + 'touche. Plage 0,001 à 5 s.', reviewed: true },
    

        'zh-Hans': { t: '起音',
              b: '按下琴键后音符达到满电平所需的时间。范围 0.001 到 5 s。',
              reviewed: 'bt' },
    },
    'tip.decay': {
        en: { t: 'Decay',
              b: 'Time the note takes to fall from full level to the Sustain level. '
               + 'Range 0.001 to 5 s.' },
        fr: { t: 'Déclin',
              b: 'Temps que met la note à redescendre du niveau plein au niveau de Maintien. '
               + 'Plage 0,001 à 5 s.', reviewed: true },
    

        'zh-Hans': { t: '衰减',
              b: '音符从满电平降到延音电平所需的时间。范围 0.001 到 5 s。',
              reviewed: 'bt' },
    },
    'tip.sustain': {
        en: { t: 'Sustain',
              b: 'Level the note holds while the key stays down, as a fraction of full level. '
               + 'Range 0 to 1.' },
        fr: { t: 'Maintien',
              b: 'Niveau que tient la note tant que la touche reste enfoncée, en fraction du niveau '
               + 'plein. Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '延音',
              b: '琴键保持按下时音符维持的电平，以满电平的比例表示。范围 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.release': {
        en: { t: 'Release',
              b: 'Time the note takes to fade to silence after the key is let go. '
               + 'Range 0.001 to 10 s.' },
        fr: { t: 'Relâchement',
              b: 'Temps que met la note à s’éteindre après le relâchement de la touche. '
               + 'Plage 0,001 à 10 s.', reviewed: true },
    

        'zh-Hans': { t: '释音',
              b: '松开琴键后音符淡至无声所需的时间。范围 0.001 到 10 s。',
              reviewed: 'bt' },
    },
    'tip.outputGain': {
        en: { t: 'Output Gain',
              b: 'Final level of the plugin, applied after the effects rack. '
               + 'Range −60 to +12 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Niveau final du plugin, appliqué après le rack d’effets. '
               + 'Plage −60 à +12 dB.', reviewed: true },
    

        'zh-Hans': { t: '输出增益',
              b: '插件的最终电平，施加在效果机架之后。范围 −60 到 +12 dB。',
              reviewed: 'bt' },
    },
    'tip.stereoWidth': {
        en: { t: 'Stereo Width',
              b: 'Spreads the voices across the stereo field by note number, low notes to the left and '
               + 'high notes to the right. At zero every voice sits in the centre. Range 0 to 1.' },
        fr: { t: 'Largeur stéréo',
              b: 'Répartit les voix dans le champ stéréo selon le numéro de note, les graves à gauche '
               + 'et les aigus à droite. À zéro toutes les voix sont au centre. Plage 0 à 1.',
              reviewed: true },
    

        'zh-Hans': { t: '立体声宽度',
              b: '按音符编号把各声部铺开在立体声场中，低音在左，高音在右。为零时每个声部都位于中央。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '合唱旁通',
              b: '把合唱接入或移出信号路径。合唱运行时按钮显示开，被旁通时显示关，因此按钮上的字与参数名是有意相反的。关或开。',
              reviewed: 'bt' },
    },
    'tip.chorusRate': {
        en: { t: 'Chorus Rate',
              b: 'Speed of the chorus delay modulation. Slow settings drift, fast ones warble. '
               + 'Range 0.1 to 10 Hz.' },
        fr: { t: 'Vitesse du chorus',
              b: 'Vitesse de modulation du retard du chorus. Les réglages lents dérivent, les rapides '
               + 'chevrotent. Plage 0,1 à 10 Hz.', reviewed: true },
    

        'zh-Hans': { t: '合唱速率',
              b: '合唱延迟调制的速度。慢的设置会漂移，快的会颤动。范围 0.1 到 10 Hz。',
              reviewed: 'bt' },
    },
    'tip.chorusDepth': {
        en: { t: 'Chorus Depth',
              b: 'How far the chorus modulates its delay time, which is how much detuning you hear. '
               + 'Range 0 to 1.' },
        fr: { t: 'Profondeur du chorus',
              b: 'Amplitude de la modulation du temps de retard, c’est-à-dire le désaccord que l’on '
               + 'entend. Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '合唱深度',
              b: '合唱调制其延迟时间的幅度，也就是你听到的失谐量。范围 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.chorusMix': {
        en: { t: 'Chorus Mix',
              b: 'Balance between the dry voice and the chorused copy. At zero the chorus is inaudible '
               + 'even while it is running. Range 0 to 1.' },
        fr: { t: 'Mix du chorus',
              b: 'Équilibre entre la voix directe et la copie traitée par le chorus. À zéro le chorus '
               + 'reste inaudible même en fonctionnement. Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '合唱混合',
              b: '干声与经过合唱的副本之间的平衡。为零时即使合唱在运行也听不见。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '延迟旁通',
              b: '把延迟接入或移出信号路径。延迟运行时按钮显示开，被旁通时显示关，因此按钮上的字与参数名是有意相反的。关或开。',
              reviewed: 'bt' },
    },
    'tip.delayTime': {
        en: { t: 'Delay Time',
              b: 'Time between the voice and its first echo. Range 0.001 to 2 s.' },
        fr: { t: 'Durée du délai',
              b: 'Temps entre la voix et son premier écho. Plage 0,001 à 2 s.', reviewed: true },
    

        'zh-Hans': { t: '延迟时间',
              b: '人声与它第一次回声之间的时间。范围 0.001 到 2 s。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '延迟反馈',
              b: '每次回声被送回以产生下一次的比例，它决定你听到多少次重复。上限止于 1 之前，使这条线路不会失控。范围 0 到 0.95。',
              reviewed: 'bt' },
    },
    'tip.delayMode': {
        en: { t: 'Delay Mode',
              b: 'Normal sends both channels through the same delay line. PingPong cross-feeds them so '
               + 'the repeats alternate between left and right. Normal or PingPong.' },
        fr: { t: 'Mode de délai',
              b: 'Normal envoie les deux canaux dans la même ligne à retard. PingPong les croise pour '
               + 'que les répétitions alternent entre la gauche et la droite. Normal ou PingPong.',
              reviewed: true },
    

        'zh-Hans': { t: '延迟模式',
              b: 'Normal 把两个声道送入同一条延迟线。PingPong 把它们交叉馈送，使重复在左右之间交替。Normal 或 PingPong。',
              reviewed: 'bt' },
    },
    'tip.delayMix': {
        en: { t: 'Delay Mix',
              b: 'Balance between the dry voice and the delayed copy. At zero the delay is inaudible '
               + 'even while it is running. Range 0 to 1.' },
        fr: { t: 'Mix du délai',
              b: 'Équilibre entre la voix directe et la copie retardée. À zéro le délai reste '
               + 'inaudible même en fonctionnement. Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '延迟混合',
              b: '干声与延迟副本之间的平衡。为零时即使延迟在运行也听不见。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '混响旁通',
              b: '把混响接入或移出信号路径。混响运行时按钮显示开，被旁通时显示关，因此按钮上的字与参数名是有意相反的。关或开。',
              reviewed: 'bt' },
    },
    'tip.reverbSize': {
        en: { t: 'Reverb Size',
              b: 'Size of the simulated room, which sets how long the tail rings on. Range 0 to 1.' },
        fr: { t: 'Taille de la réverb',
              b: 'Taille de la salle simulée, ce qui fixe la longueur de la queue de réverbération. '
               + 'Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '混响尺寸',
              b: '所模拟房间的尺寸，它决定尾音延续的长度。范围 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.reverbDamp': {
        en: { t: 'Reverb Damping',
              b: 'How fast the high frequencies are absorbed in the tail. Low values give a bright, '
               + 'tiled room; high values a soft, curtained one. Range 0 to 1.' },
        fr: { t: 'Amortissement de la réverb',
              b: 'Rapidité d’absorption des aigus dans la queue. Les valeurs basses donnent une salle '
               + 'claire et carrelée, les valeurs hautes une salle douce et tendue de rideaux. '
               + 'Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '混响阻尼',
              b: '尾音中高频被吸收的快慢。低值给出明亮的瓷砖房间，高值给出柔和的挂帘房间。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '混响预延迟',
              b: '干声与混响尾音开始之间的间隔。几十毫秒就能让歌词在长混响里保持清晰。范围 0 到 200 ms。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '混响调制',
              b: '用一组慢速 LFO 调制梳状延迟的长度，使尾音不会停在固定的共振上。一点点就能去掉长尾音的金属味。范围 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.reverbShimmer': {
        en: { t: 'Reverb Shimmer',
              b: 'Feeds an octave-up copy of the tail back into the reverb, so it climbs as it decays. '
               + 'Range 0 to 1.' },
        fr: { t: 'Shimmer de la réverb',
              b: 'Réinjecte dans la réverb une copie de la queue transposée à l’octave supérieure, si '
               + 'bien qu’elle monte en s’éteignant. Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '混响微光',
              b: '把尾音升高八度的副本送回混响，使它在衰减的同时向上攀升。范围 0 到 1。',
              reviewed: 'bt' },
    },
    'tip.reverbMix': {
        en: { t: 'Reverb Mix',
              b: 'Balance between the dry voice and the reverb. At zero the reverb is inaudible even '
               + 'while it is running. Range 0 to 1.' },
        fr: { t: 'Mix de la réverb',
              b: 'Équilibre entre la voix directe et la réverbération. À zéro la réverb reste '
               + 'inaudible même en fonctionnement. Plage 0 à 1.', reviewed: true },
    

        'zh-Hans': { t: '混响混合',
              b: '干声与混响之间的平衡。为零时即使混响在运行也听不见。范围 0 到 1。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '均衡旁通',
              b: '把均衡器接入或移出信号路径。均衡运行时按钮显示开，被旁通时显示关，因此按钮上的字与参数名是有意相反的。关或开。',
              reviewed: 'bt' },
    },
    'tip.eqLowGain': {
        en: { t: 'EQ Low Gain',
              b: 'Cut or boost of the low shelf, hinged at 200 Hz. Range −12 to +12 dB.' },
        fr: { t: 'Gain grave de l’EQ',
              b: 'Atténuation ou accentuation du plateau grave, articulé à 200 Hz. '
               + 'Plage −12 à +12 dB.', reviewed: true },
    

        'zh-Hans': { t: '均衡低频增益',
              b: '低频搁架的衰减或提升，转折点在 200 Hz。范围 −12 到 +12 dB。',
              reviewed: 'bt' },
    },
    'tip.eqMidGain': {
        en: { t: 'EQ Mid Gain',
              b: 'Cut or boost of the mid peaking band, centred on EQ Mid Freq. '
               + 'Range −12 to +12 dB.' },
        fr: { t: 'Gain médium de l’EQ',
              b: 'Atténuation ou accentuation de la cloche médium, centrée sur la Fréq. méd. de l’EQ. '
               + 'Plage −12 à +12 dB.', reviewed: true },
    

        'zh-Hans': { t: '均衡中频增益',
              b: '中频钟形带的衰减或提升，中心位于均衡的中频频率。范围 −12 到 +12 dB。',
              reviewed: 'bt' },
    },
    'tip.eqMidFreq': {
        en: { t: 'EQ Mid Freq',
              b: 'Centre frequency of the mid peaking band. Most of the vowel character sits between '
               + '500 and 3000 Hz. Range 200 to 8000 Hz.' },
        fr: { t: 'Fréq. médium de l’EQ',
              b: 'Fréquence centrale de la cloche médium. L’essentiel du caractère des voyelles se '
               + 'situe entre 500 et 3000 Hz. Plage 200 à 8000 Hz.', reviewed: true },
    

        'zh-Hans': { t: '均衡中频频率',
              b: '中频钟形带的中心频率。元音特征的大部分位于 500 到 3000 Hz 之间。范围 200 到 8000 Hz。',
              reviewed: 'bt' },
    },
    'tip.eqHighGain': {
        en: { t: 'EQ High Gain',
              b: 'Cut or boost of the high shelf, hinged at 8 kHz. Range −12 to +12 dB.' },
        fr: { t: 'Gain aigu de l’EQ',
              b: 'Atténuation ou accentuation du plateau aigu, articulé à 8 kHz. '
               + 'Plage −12 à +12 dB.', reviewed: true },
    

        'zh-Hans': { t: '均衡高频增益',
              b: '高频搁架的衰减或提升，转折点在 8 kHz。范围 −12 到 +12 dB。',
              reviewed: 'bt' },
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
    

        'zh-Hans': { t: '启用歌词',
              b: '把元音和辅音光标交给歌词引擎，它会在每个音符上依次走过上方输入的 ARPABET 音素。开启期间两个面板不再跟随鼠标。关或开。',
              reviewed: 'bt' },
    },

    // ── Chrome ──────────────────────────────────────────────────────────────
    //
    // The gear body describes ONLY what this popover actually contains. It
    // opens BELOW the button (.settings-popover is top: 34px), and it holds
    // TWO controls: the language selector and the hover-help switch.
    //
    // v1.29.0 CORRECTION. Through v1.28.1 this body asserted that the panel
    // held the language selector and no other control, and the comment above
    // it said the same in stronger terms. Both were written before v1.28.0
    // added tip.tipsToggle, label.hoverHelp and aria.helpToggle to this file
    // and #tips-toggle to the popover; check-i18n assertion 16 has required
    // that switch on every localized plugin since. The comment was stale and
    // the table was the truth, so the sentence a reader could check against
    // the live panel is the one that moved.
    'tip.gear': {
        en: { t: 'Settings',
              b: 'Opens the settings panel just below this button. It holds the interface language '
               + 'and the hover-help switch.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages juste sous ce bouton. Il contient la langue de '
               + 'l’interface et le commutateur des infobulles.', reviewed: true },
    

        'zh-Hans': { t: '设置',
              b: '在此按钮正下方打开设置面板。面板中有界面语言和悬停帮助开关。',
              reviewed: 'bt' },
    },
    // v1.29.0 CORRECTION. Through v1.28.1 this body named the selector's
    // options and gave their count, in both languages. The selector now offers
    // a third, so an enumeration written into the copy is a sentence that goes
    // false every time a language is added. The body now says what the control
    // DOES and leaves the list to the control itself, which is the only place
    // it can be right by construction.
    'tip.language': {
        en: { t: 'Language',
              b: 'Switches every label, heading, button caption and hover-help entry on the page '
               + 'into the language you pick. Value readouts keep their English number format and '
               + 'unit symbols. The choice is remembered with the session.' },
        fr: { t: 'Langue',
              b: 'Fait passer toutes les étiquettes, tous les titres, toutes les légendes de bouton '
               + 'et toutes les infobulles de la page dans la langue choisie. Les valeurs '
               + 'affichées gardent leur format numérique et leurs unités en anglais. Le choix est '
               + 'conservé avec la session.', reviewed: true },
    

        'zh-Hans': { t: '语言',
              b: '把页面上的每一个标签、每一个标题、每一处按钮文字和每一条悬停帮助都切换成所选的语言。数值读数保持英文的数字格式和单位符号。所选语言随会话一起记住。',
              reviewed: 'bt' },
    },
    // v1.28.0 — the switch that reaches this whole layer.
    'tip.tipsToggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this '
               + 'switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
               + 'l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    

        'zh-Hans': { t: '悬停帮助',
              b: '开启或关闭这些悬停帮助。关闭之后，只有齿轮和这个开关仍会解释自己。',
              reviewed: 'bt' },
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
    'label.subtitle':      { en: { t: 'vocal synthesizer' }, fr: { t: 'synthétiseur vocal', reviewed: true } , 'zh-Hans': { t: '人声合成器', reviewed: 'bt' },},

    // ── Settings popover (v1.26.0) ──────────────────────────────────────────
    'label.settings':      { en: { t: 'Settings' },   fr: { t: 'Réglages',  reviewed: true } , 'zh-Hans': { t: '设置', reviewed: 'bt' },},
    'label.language':      { en: { t: 'Language' },   fr: { t: 'Langue',    reviewed: true } , 'zh-Hans': { t: '语言', reviewed: 'bt' },},

    // v1.28.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } , 'zh-Hans': { t: '悬停帮助', reviewed: 'bt' },},
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true } , 'zh-Hans': { t: '开', reviewed: 'bt' },},
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true } , 'zh-Hans': { t: '关', reviewed: 'bt' },},

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
    'label.save':          { en: { t: 'Save' },       fr: { t: 'Enreg.',    reviewed: true } , 'zh-Hans': { t: '保存', reviewed: 'bt' },},
    // The "no filter" sentinel of #preset-category. Its VALUE is the string
    // "all" and that is what populateCategories() and the change handler
    // compare, so the visible text is free to change.
    'label.allCategories': { en: { t: 'All' },        fr: { t: 'Toutes',    reviewed: true } , 'zh-Hans': { t: '全部', reviewed: 'bt' },},

    // ── Tabs ────────────────────────────────────────────────────────────────
    'label.tabSynth':      { en: { t: 'Synth' },      fr: { t: 'Synthé',    reviewed: true } , 'zh-Hans': { t: '合成器', reviewed: 'bt' },},
    'label.tabLyrics':     { en: { t: 'Lyrics' },     fr: { t: 'Paroles',   reviewed: true } , 'zh-Hans': { t: '歌词', reviewed: 'bt' },},
    'label.tabTuning':     { en: { t: 'Tuning' },     fr: { t: 'Accord',    reviewed: true } , 'zh-Hans': { t: '调音', reviewed: 'bt' },},
    'label.tabEffects':    { en: { t: 'Effects' },    fr: { t: 'Effets',    reviewed: true } , 'zh-Hans': { t: '效果', reviewed: 'bt' },},

    // ── Synth tab: vowel pad and glottal source ─────────────────────────────
    'label.vowelMorph':    { en: { t: 'Vowel Morph' },    fr: { t: 'Morphose vocalique', reviewed: true } , 'zh-Hans': { t: '元音变形', reviewed: 'bt' },},
    'label.glottalSource': { en: { t: 'Glottal Source' }, fr: { t: 'Source glottique',   reviewed: true } , 'zh-Hans': { t: '声门源', reviewed: 'bt' },},
    // glottalRd drives the LF model's Rd shape, which IS the voice quality.
    'label.voiceQ':        { en: { t: 'Voice Q' },    fr: { t: 'Qualité',   reviewed: true } , 'zh-Hans': { t: '音质', reviewed: 'bt' },},
    'label.breath':        { en: { t: 'Breath' },     fr: { t: 'Souffle',   reviewed: true } , 'zh-Hans': { t: '气息', reviewed: 'bt' },},
    'label.vibRate':       { en: { t: 'Vib Rate' },   fr: { t: 'Vit. vibrato', reviewed: true } , 'zh-Hans': { t: '颤音速率', reviewed: 'bt' },},
    // "Prof. vibrato" at v1.27.1. The v1.26.0 note chose "Ampleur" to keep the
    // 55 px cell clear of the effects rack's standalone "Profondeur"; the
    // glossary's abbreviated root answers both worries at once — it cannot be
    // confused with the bare "Profondeur" and it is NARROWER than what shipped
    // (45.50 against "Vib Ampleur" 46.72; "Vit. vibrato" 40.95 against
    // "Vib Vitesse" 41.02). Measured on this page, nowrap, at 800 x 600.
    'label.vibDepth':      { en: { t: 'Vib Depth' },  fr: { t: 'Prof. vibrato', reviewed: true } , 'zh-Hans': { t: '颤音深度', reviewed: 'bt' },},
    'label.vibDelay':      { en: { t: 'Vib Delay' },  fr: { t: 'Vib Retard',  reviewed: true } , 'zh-Hans': { t: '颤音延迟', reviewed: 'bt' },},
    // Shimmer stays — the glossary carries it as a loanword the French audio
    // press uses. Jitter does NOT: the glossary settles gigue, so the caption,
    // tip.jitter's title and the two cross-references inside tip.shimmer's body
    // moved together at v1.27.1. French voice-science papers do write "jitter",
    // and that argument was weighed and lost to one suite-wide word (22.00 px
    // against 18.00 in a 55 px cell, so width had nothing to say).
    'label.jitter':        { en: { t: 'Jitter' },     fr: { t: 'Gigue',     reviewed: true } , 'zh-Hans': { t: '抖动', reviewed: 'bt' },},
    'label.shimmer':       { en: { t: 'Shimmer' },    fr: { t: 'Shimmer',   reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '振幅微扰', termNote: 'the GLOTTAL shimmer — cycle-to-cycle amplitude perturbation. 微光 is the reverb sense and is already spent on the effects tab. RE-AUTHORED after the blind reverse read: the first rendering came back as "Amplitude Jitter", colliding with the Jitter knob beside it', reviewed: 'bt' },},
    'label.rdMod':         { en: { t: 'Rd Mod' },     fr: { t: 'Mod Rd',    reviewed: true } , 'zh-Hans': { t: 'Rd 调制', reviewed: 'bt' },},
    'label.tilt':          { en: { t: 'Tilt' },       fr: { t: 'Inclinaison', reviewed: true } , 'zh-Hans': { t: '倾斜', reviewed: 'bt' },},

    // ── Synth tab: consonant ────────────────────────────────────────────────
    'label.consonant':     { en: { t: 'Consonant' },  fr: { t: 'Consonne',  reviewed: true } , 'zh-Hans': { t: '辅音', reviewed: 'bt' },},
    'label.level':         { en: { t: 'Level' },      fr: { t: 'Niveau',    reviewed: true } , 'zh-Hans': { t: '电平', reviewed: 'bt' },},
    'label.voicing':       { en: { t: 'Voicing' },    fr: { t: 'Voisement', reviewed: true } , 'zh-Hans': { t: '清浊度', termNote: 'the CONSONANT voicing, voiceless against voiced; 和声排列 is chord voicing and does not occur on this page', reviewed: 'bt' },},
    'label.auto':          { en: { t: 'Auto' },       fr: { t: 'Auto',      reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '自动', reviewed: 'bt' },},
    // Place and manner of articulation, abbreviated to fit the 8 px overlay on
    // the consonant pad. The French terms are labial / alvéolaire / palatal /
    // vélaire and fricative / occlusive, so four of the six abbreviate the
    // same way and two do not.
    'label.placeLabial':     { en: { t: 'Lab' },  fr: { t: 'Lab',  reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '双唇', reviewed: 'bt' },},
    'label.placeAlveolar':   { en: { t: 'Alv' },  fr: { t: 'Alv',  reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '齿龈', reviewed: 'bt' },},
    'label.placePalatal':    { en: { t: 'Pal' },  fr: { t: 'Pal',  reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '硬腭', reviewed: 'bt' },},
    'label.placeVelar':      { en: { t: 'Vel' },  fr: { t: 'Vél',  reviewed: true } , 'zh-Hans': { t: '软腭', reviewed: 'bt' },},
    'label.mannerFricative': { en: { t: 'Fric' }, fr: { t: 'Fric', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '擦音', reviewed: 'bt' },},
    'label.mannerPlosive':   { en: { t: 'Plos' }, fr: { t: 'Occl', reviewed: true } , 'zh-Hans': { t: '塞音', reviewed: 'bt' },},
    // The consonant envelope column: 42 px cells, the tightest on the page.
    'label.attackShort':   { en: { t: 'Atk' },    fr: { t: 'Att',   reviewed: true } , 'zh-Hans': { t: '辅音起音', reviewed: 'bt' },},
    // "Tenue", not the glossary's "Maintien": Maintien is already this page's
    // Sustain caption (label.sustain, 32.50 px in the 55 px ADSR cell three
    // rows away and visible at the same time), and one French word on two
    // different controls is the N1 correction-11 defect in mirror image.
    // Width is not the reason — Maintien measures 32.50 px in this 42 px cell.
    'label.hold':          { en: { t: 'Hold' },   fr: { t: 'Tenue', reviewed: true,
                             termNote: 'the consonant envelope\'s HOLD stage; Maintien is already '
                                     + 'label.sustain on this same tab, and two controls sharing '
                                     + 'one French name is a defect' } ,

        'zh-Hans': { t: '保持',
              reviewed: 'bt' },
    },
    'label.transShort':    { en: { t: 'Trans' },  fr: { t: 'Trans', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '辅音过渡', reviewed: 'bt' },},

    // ── Synth tab: character ────────────────────────────────────────────────
    'label.character':     { en: { t: 'Character' },  fr: { t: 'Caractère', reviewed: true } , 'zh-Hans': { t: '特性', reviewed: 'bt' },},
    'label.topology':      { en: { t: 'Topology' },   fr: { t: 'Topologie', reviewed: true } , 'zh-Hans': { t: '拓扑', reviewed: 'bt' },},
    'label.shift':         { en: { t: 'Shift' },      fr: { t: 'Décalage',  reviewed: true } , 'zh-Hans': { t: '移位', reviewed: 'bt' },},
    'label.spread':        { en: { t: 'Spread' },     fr: { t: 'Étalement', reviewed: true } , 'zh-Hans': { t: '展宽', reviewed: 'bt' },},
    'label.glide':         { en: { t: 'Glide' },      fr: { t: 'Portamento', reviewed: true } , 'zh-Hans': { t: '滑音', reviewed: 'bt' },},
    'label.transition':    { en: { t: 'Transition' }, fr: { t: 'Transition', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '过渡时间', termNote: 'the Transition Time knob in the Character section; the consonant envelope on the same tab carries its own Transition, so both are qualified', reviewed: 'bt' },},
    'label.focus':         { en: { t: 'Focus' },      fr: { t: 'Focalisation', reviewed: true } , 'zh-Hans': { t: '聚焦', reviewed: 'bt' },},
    // "Formant du chanteur" measures 74.98 px in this 55 px .knob-wrap, which
    // is shrink-to-fit with overflow: visible — it would overhang 9.99 px per
    // side into a 10.00 px gap and clear its neighbour by 0.01 px. This is the
    // one width defence on the page that HELD when Stage N re-measured it.
    // "F. chanteur" (39.78) stays, and it is what a French singing-synthesis
    // UI uses anyway.
    'label.singersFormant':{ en: { t: "Singer's F" }, fr: { t: 'F. chanteur', reviewed: true } , 'zh-Hans': { t: '歌手共振峰', reviewed: 'bt' },},
    'label.nasality':      { en: { t: 'Nasality' },   fr: { t: 'Nasalité',  reviewed: true } , 'zh-Hans': { t: '鼻音度', reviewed: 'bt' },},
    'label.nasalPlace':    { en: { t: 'Nasal Place' }, fr: { t: 'Lieu nasal', reviewed: true } , 'zh-Hans': { t: '鼻音位置', reviewed: 'bt' },},

    // ── Synth tab: envelope and output ──────────────────────────────────────
    'label.envelope':      { en: { t: 'Envelope' },   fr: { t: 'Enveloppe', reviewed: true } , 'zh-Hans': { t: '包络', reviewed: 'bt' },},
    'label.attack':        { en: { t: 'Attack' },     fr: { t: 'Attaque',   reviewed: true } , 'zh-Hans': { t: '起音', reviewed: 'bt' },},
    // Shared by the ADSR decay (55 px) and the consonant decay (42 px):
    // "Déclin" is 6 characters and fits both, so one key, one string.
    'label.decay':         { en: { t: 'Decay' },      fr: { t: 'Déclin',    reviewed: true } , 'zh-Hans': { t: '衰减', reviewed: 'bt' },},
    'label.sustain':       { en: { t: 'Sustain' },    fr: { t: 'Maintien',  reviewed: true } , 'zh-Hans': { t: '延音', reviewed: 'bt' },},
    'label.release':       { en: { t: 'Release' },    fr: { t: 'Relâchement', reviewed: true } , 'zh-Hans': { t: '释音', reviewed: 'bt' },},
    'label.output':        { en: { t: 'Output' },     fr: { t: 'Sortie',    reviewed: true } , 'zh-Hans': { t: '输出', reviewed: 'bt' },},
    'label.gain':          { en: { t: 'Gain' },       fr: { t: 'Gain',      reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '增益', reviewed: 'bt' },},
    'label.width':         { en: { t: 'Width' },      fr: { t: 'Largeur',   reviewed: true } , 'zh-Hans': { t: '宽度', reviewed: 'bt' },},

    // ── Effects tab ─────────────────────────────────────────────────────────
    'label.chorus':        { en: { t: 'Chorus' },     fr: { t: 'Chorus',    reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '合唱', reviewed: 'bt' },},
    'label.delay':         { en: { t: 'Delay' },      fr: { t: 'Délai',     reviewed: true } , 'zh-Hans': { t: '延迟', reviewed: 'bt' },},
    'label.reverb':        { en: { t: 'Reverb' },     fr: { t: 'Réverb',    reviewed: true } , 'zh-Hans': { t: '混响', reviewed: 'bt' },},
    'label.eq':            { en: { t: 'EQ' },         fr: { t: 'EQ',        reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '均衡', reviewed: 'bt' },},
    // The four bypass buttons' two faces. Written from script, so they go
    // through setLabel() and the element becomes a [data-i18n] element from
    // that moment on — a raw literal there is stranded in the previous
    // language the instant the selector fires.
    'label.on':            { en: { t: 'On' },         fr: { t: 'Marche',    reviewed: true } , 'zh-Hans': { t: '开', reviewed: 'bt' },},
    'label.off':           { en: { t: 'Off' },        fr: { t: 'Arrêt',     reviewed: true } , 'zh-Hans': { t: '关', reviewed: 'bt' },},
    'label.rate':          { en: { t: 'Rate' },       fr: { t: 'Vitesse',   reviewed: true } , 'zh-Hans': { t: '速率', reviewed: 'bt' },},
    'label.depth':         { en: { t: 'Depth' },      fr: { t: 'Profondeur', reviewed: true } , 'zh-Hans': { t: '深度', reviewed: 'bt' },},
    'label.mix':           { en: { t: 'Mix' },        fr: { t: 'Mix',       reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '混合', reviewed: 'bt' },},
    'label.time':          { en: { t: 'Time' },       fr: { t: 'Durée',     reviewed: true } , 'zh-Hans': { t: '时间', reviewed: 'bt' },},
    'label.feedback':      { en: { t: 'Feedback' },   fr: { t: 'Réinjection', reviewed: true } , 'zh-Hans': { t: '反馈', reviewed: 'bt' },},
    // Shared by the delay-mode caption and the tuning panel's rotation-table
    // column header: one word, identical in both languages, one key.
    'label.mode':          { en: { t: 'Mode' },       fr: { t: 'Mode',      reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '模式', reviewed: 'bt' },},
    'label.size':          { en: { t: 'Size' },       fr: { t: 'Taille',    reviewed: true } , 'zh-Hans': { t: '尺寸', reviewed: 'bt' },},
    'label.damp':          { en: { t: 'Damp' },       fr: { t: 'Amort.',    reviewed: true } , 'zh-Hans': { t: '阻尼', reviewed: 'bt' },},
    'label.preDelay':      { en: { t: 'Pre-dly' },    fr: { t: 'Pré-délai', reviewed: true } , 'zh-Hans': { t: '预延迟', reviewed: 'bt' },},
    'label.mod':           { en: { t: 'Mod' },        fr: { t: 'Mod',       reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '调制', reviewed: 'bt' },},
    'label.low':           { en: { t: 'Low' },        fr: { t: 'Grave',     reviewed: true } , 'zh-Hans': { t: '低', reviewed: 'bt' },},
    'label.mid':           { en: { t: 'Mid' },        fr: { t: 'Médium',    reviewed: true } , 'zh-Hans': { t: '中', reviewed: 'bt' },},
    'label.midFreq':       { en: { t: 'Mid Freq' },   fr: { t: 'Fréq. méd.', reviewed: true } , 'zh-Hans': { t: '中频频率', reviewed: 'bt' },},
    'label.high':          { en: { t: 'High' },       fr: { t: 'Aigu',      reviewed: true } , 'zh-Hans': { t: '高', reviewed: 'bt' },},

    // ── Lyrics tab ──────────────────────────────────────────────────────────
    'label.arpabetInput':  { en: { t: 'ARPABET Input' }, fr: { t: 'Saisie ARPABET', reviewed: true } , 'zh-Hans': { t: 'ARPABET 输入', reviewed: 'bt' },},
    'label.enable':        { en: { t: 'Enable' },     fr: { t: 'Activer',   reviewed: true } , 'zh-Hans': { t: '启用', reviewed: 'bt' },},
    'label.loop':          { en: { t: 'Loop' },       fr: { t: 'Boucle',    reviewed: true } , 'zh-Hans': { t: '循环', reviewed: 'bt' },},
    // "Réinit.", not "Réinitialiser": 13 characters against 5 grew
    // .lyrics-controls by 45 px and dragged four elements left. The full
    // sentence survives on the button's accessible name (aria.resetLyrics),
    // which has no box to fit.
    'label.reset':         { en: { t: 'Reset' },      fr: { t: 'Réinit.',   reviewed: true } , 'zh-Hans': { t: '重置', reviewed: 'bt' },},
    // The help line under the ARPABET box is ONE text node in v1.25.4 holding
    // two captions around two runs of phoneme codes. Split into two keyed
    // spans so applyLabel cannot delete the codes with them; the code runs
    // themselves are I18N_EXEMPT notation.
    'label.vowels':        { en: { t: 'Vowels:' },     fr: { t: 'Voyelles :', reviewed: true } , 'zh-Hans': { t: '元音：', reviewed: 'bt' },},
    'label.consonants':    { en: { t: 'Consonants:' }, fr: { t: 'Consonnes :', reviewed: true } , 'zh-Hans': { t: '辅音：', reviewed: 'bt' },},
    'label.syllables':     { en: { t: 'Syllables' },   fr: { t: 'Syllabes',  reviewed: true } , 'zh-Hans': { t: '音节', reviewed: 'bt' },},
    'label.tuningPanelFailed': {
        en: { t: 'Tuning panel failed to load.' },
        fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: true },
    

        'zh-Hans': { t: '调音面板载入失败。',
              reviewed: 'bt' },
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
                             fr: { t: 'Intervalles ({n} notes)', reviewed: true } ,

        'zh-Hans': { t: '音程（{n} 个音）',
              reviewed: 'bt' },
    },
    'tuning.tonic':        { en: { t: 'Tonic' },      fr: { t: 'Tonique',   reviewed: true } , 'zh-Hans': { t: '主音', reviewed: 'bt' },},
    // The note count under each library row. "notes" is the same word in
    // French, so the entry exists to KEY the node rather than to change it:
    // an unkeyed node here is indistinguishable from one somebody forgot, and
    // this template is an `html +=` accumulator, which assertion 12 cannot
    // read at all.
    'tuning.noteCount':    { en: { t: '{n} notes' },  fr: { t: '{n} notes', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '{n} 个音', reviewed: 'bt' },},
    'tuning.vizCircle':    { en: { t: 'Circle' },     fr: { t: 'Cercle',    reviewed: true } , 'zh-Hans': { t: '圆周', reviewed: 'bt' },},
    'tuning.vizPolar':     { en: { t: 'Polar' },      fr: { t: 'Polaire',   reviewed: true } , 'zh-Hans': { t: '极坐标', reviewed: 'bt' },},
    'tuning.vizMatrix':    { en: { t: 'Matrix' },     fr: { t: 'Matrice',   reviewed: true } , 'zh-Hans': { t: '矩阵', reviewed: 'bt' },},
    'tuning.vizTrueKeys':  { en: { t: 'True Keys' },  fr: { t: 'Touches',   reviewed: true } , 'zh-Hans': { t: '真实键位', reviewed: 'bt' },},
    'tuning.vizRotation':  { en: { t: 'Rotation' },   fr: { t: 'Rotation',  reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '旋转', reviewed: 'bt' },},
    'tuning.scaleIntervals': { en: { t: 'Scale Intervals' },
                               fr: { t: 'Intervalles de la gamme', reviewed: true } ,

        'zh-Hans': { t: '音阶音程',
              reviewed: 'bt' },
    },
    'tuning.tkHint':       { en: { t: 'Hold 2+ notes to see intervals' },
                             fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: true } ,

        'zh-Hans': { t: '按住 2 个以上音符可查看音程',
              reviewed: 'bt' },
    },
    'tuning.library':      { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque', reviewed: true } , 'zh-Hans': { t: '调音库', reviewed: 'bt' },},
    // The library filter <option>s. Their VALUE is matched against the
    // `category` field of getEmbeddedTuningList(), so only the text moves.
    'tuning.catAll':       { en: { t: 'All Categories' },   fr: { t: 'Toutes catégories', reviewed: true } , 'zh-Hans': { t: '全部类别', reviewed: 'bt' },},
    'tuning.catHistorical':{ en: { t: 'Historical' },       fr: { t: 'Historiques',   reviewed: true } , 'zh-Hans': { t: '历史音律', reviewed: 'bt' },},
    'tuning.catJust':      { en: { t: 'Just Intonation' },  fr: { t: 'Intonation juste', reviewed: true } , 'zh-Hans': { t: '纯律', reviewed: 'bt' },},
    'tuning.catEqual':     { en: { t: 'Equal Divisions' },  fr: { t: 'Divisions égales', reviewed: true } , 'zh-Hans': { t: '等分', reviewed: 'bt' },},
    'tuning.catNonOctave': { en: { t: 'Non-Octave' },       fr: { t: 'Non octaviantes', reviewed: true } , 'zh-Hans': { t: '非八度', reviewed: 'bt' },},
    'tuning.catWorld':     { en: { t: 'World' },            fr: { t: 'Du monde',      reviewed: true } , 'zh-Hans': { t: '世界音律', reviewed: 'bt' },},
    // A4 stays: it is scientific pitch notation, and the French octave
    // numbering for the same pitch is La3, which would silently rename the
    // reference the .scl / .kbm files are written against.
    'tuning.a4Ref':        { en: { t: 'A4 REF' },      fr: { t: 'RÉF. A4',   reviewed: true } , 'zh-Hans': { t: 'A4 基准', reviewed: 'bt' },},
    'tuning.stretch':      { en: { t: 'Stretch' },     fr: { t: 'Étirement', reviewed: true } , 'zh-Hans': { t: '延展', reviewed: 'bt' },},
    'tuning.loadScl':      { en: { t: 'Load .SCL' },   fr: { t: 'Ouvrir .SCL', reviewed: true } , 'zh-Hans': { t: '载入 .scl', reviewed: 'bt' },},
    'tuning.loadKbm':      { en: { t: 'Load .KBM' },   fr: { t: 'Ouvrir .KBM', reviewed: true } , 'zh-Hans': { t: '载入 .kbm', reviewed: 'bt' },},
    'tuning.saveScl':      { en: { t: 'Save .SCL' },   fr: { t: 'Enreg. .SCL', reviewed: true } , 'zh-Hans': { t: '保存 .scl', reviewed: 'bt' },},
    'tuning.saveKbm':      { en: { t: 'Save .KBM' },   fr: { t: 'Enreg. .KBM', reviewed: true } , 'zh-Hans': { t: '保存 .kbm', reviewed: 'bt' },},
    'tuning.exportHtml':   { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: true } , 'zh-Hans': { t: '导出 HTML', reviewed: 'bt' },},
    'tuning.generateScale':{ en: { t: 'Generate Scale' }, fr: { t: 'Générer une gamme', reviewed: true } , 'zh-Hans': { t: '生成音阶', reviewed: 'bt' },},
    'tuning.genEdo':       { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: true } , 'zh-Hans': { t: '等分八度 (EDO)', reviewed: 'bt' },},
    'tuning.genHarmonic':  { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique', reviewed: true } , 'zh-Hans': { t: '泛音列', reviewed: 'bt' },},
    'tuning.genRank2':     { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: true } , 'zh-Hans': { t: '二阶音律', reviewed: 'bt' },},
    'tuning.divisions':    { en: { t: 'Divisions' },      fr: { t: 'Divisions', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '分割', reviewed: 'bt' },},
    'tuning.period':       { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: true } , 'zh-Hans': { t: '周期 (C)', reviewed: 'bt' },},
    'tuning.startHarmonic':{ en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: true } , 'zh-Hans': { t: '起始泛音', reviewed: 'bt' },},
    'tuning.endHarmonic':  { en: { t: 'End Harmonic' },   fr: { t: 'Harmonique de fin', reviewed: true } , 'zh-Hans': { t: '终止泛音', reviewed: 'bt' },},
    'tuning.generator':    { en: { t: 'Generator (c)' },  fr: { t: 'Générateur (c)', reviewed: true } , 'zh-Hans': { t: '生成元 (C)', reviewed: 'bt' },},
    'tuning.notes':        { en: { t: 'Notes' },          fr: { t: 'Notes',    reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '音符', reviewed: 'bt' },},
    'tuning.generate':     { en: { t: 'Generate' },       fr: { t: 'Générer',  reviewed: true } , 'zh-Hans': { t: '生成', reviewed: 'bt' },},

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is the accessible NAME. A screen reader in French reading
    // an English name is the same failure as a French page with an English
    // caption. None of these has a rendered box, so none is a geometry risk.
    // The four below replace the four native title= attributes v1.25.4
    // carried: their text is MOVED, not re-authored (contract §4).
    'aria.presetPrev':     { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: true } , 'zh-Hans': { t: '上一个预设', reviewed: 'bt' },},
    'aria.presetNext':     { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: true } , 'zh-Hans': { t: '下一个预设', reviewed: 'bt' },},
    'aria.loopToggle':     { en: { t: 'Toggle loop' },     fr: { t: 'Activer ou désactiver la boucle', reviewed: true } , 'zh-Hans': { t: '开关循环', reviewed: 'bt' },},
    'aria.resetLyrics':    { en: { t: 'Reset to first syllable' },
                             fr: { t: 'Revenir à la première syllabe', reviewed: true } ,

        'zh-Hans': { t: '回到第一个音节',
              reviewed: 'bt' },
    },
    'aria.langSelect':     { en: { t: 'Interface language' },
                             fr: { t: 'Langue de l’interface', reviewed: true } ,

        'zh-Hans': { t: '界面语言',
              reviewed: 'bt' },
    },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } , 'zh-Hans': { t: '开关悬停帮助', reviewed: 'bt' },},
    'placeholder.lyrics':  { en: { t: 'Type ARPABET phonemes separated by spaces (e.g. HH AH L OW W ER L D)' },
                             fr: { t: 'Saisir des phonèmes ARPABET séparés par des espaces (ex. HH AH L OW W ER L D)',
                                   reviewed: true } ,

        'zh-Hans': { t: '输入以空格分隔的 ARPABET 音素（例如 HH AH L OW W ER L D）',
              reviewed: 'bt' },
    },
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
    ['#tips-toggle',          'tip.tipsToggle'],
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
