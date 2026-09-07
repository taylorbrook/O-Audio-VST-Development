/*
   This file is part of O-GrainScatter, an Ouaricon Audio plugin.
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
// i18n.js — O-GrainScatter LABEL + HOVER-HELP table, English + French +
// Simplified Chinese (v2.8.0)
//
// ── v2.8.0: SIMPLIFIED CHINESE (Stage 4, wave 4f tracer) ───────────────────
// 131 rows — 39 tip titles, 39 tip bodies, 53 labels. Authored at
// reviewed:'mt', read back through a blind reverse pass, then promoted to
// reviewed:'bt'. `reviewed: 'native'` stays open: this project has no native
// Chinese reader, and lint rule R1 prints that disclosure on every run.
//
// ONE termNote EXEMPTION, hit twice because the caption and the tooltip title
// are separate rows: Dist LPF -> 距离低通, NOT the glossary root 失真低通.
// "Dist" is DISTANCE on this page — the body reads "Distance LPF sets how
// much Distance darkens the cloud" — and the glossary root renders the other
// sense of the abbreviation. O-GrainScatter is the only site in the corpus
// that carries the term, so the root was settled without one; reported so the
// glossary row can be corrected rather than worked around again.
//
// TWO STALE BODIES were corrected BEFORE a byte of Chinese was authored, and
// the Chinese was written from the corrected English rather than from the
// English as found. Both are recorded at their entries.
//
// ── v2.6.1: FRENCH QA PASS (Stage N, 2026-08-31) ───────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 43 entries of 87 (16 terminology, 21 typography, 5 grammar/register,
// 1 meaning). sameAsEn: kept 10, translated 0. termNote exemptions: 2 (both on
// Grain Size, listed at their entries). Left as drafted: the rest.
// reviewed: false throughout — that flag records a native speaker, and this
// pass is a second machine reading against a glossary and a lint.
//
// The lint went 52 findings -> 0, `--strict` exit 0: 17 T3 (% spacing), 15 T4
// (colon), 7 T5 (;!?), 5 T7 (number-unit) and 8 G1 (glossary). NOT the 68 the
// batch table quotes — the N1 pilots' three lint fixes (`daed4a2e`, `8a387f1c`,
// `6eb042c8`) re-attributed 21 T7 into T4 + T7 and stopped `%` double-reporting
// before this file was read.
//
// ── ÉTALEMENT IS SPREAD, DISPERSION IS SCATTER — settled on this page ───────
// The draft spent "Dispersion" on BOTH, which is the exact collision the
// glossary exists to prevent (its "écart" row says so). This page has a real
// Scatter — the spatial_mode option and the #grain-canvas caption — so:
//   * label.spread / tip.spread  Dispersion -> Étalement  (53.83 px, 0.19 px
//     NARROWER than the string it replaces, 8.17 px under the 62 px cap)
//   * label.azSpread / label.elSpread  Disp. az. / Disp. él. -> Étal. az. /
//     Étal. él.  (41.30 / 41.17 px, +1.89 each, in the same 62 px cells whose
//     min-content driver is the longest WORD — 23.14 -> 25.03 px)
//   * label.vizGrain KEEPS "Dispersion de grains": its English is Grain
//     SCATTER, and that is the glossary's word for it.
// The glossary has no `az spread` / `el spread` row; "Étal." is its accepted
// abbreviation for spread and the composite follows the shipped "Disp. az."
// shape, not a third invented form. Reported so the row can grow.
//
// ── THE OTHER TWO GLOSSARY ROOTS, MEASURED ─────────────────────────────────
//   * label.reverse  Inverse -> Inversion (49.63 px, 12.37 under the cap). The
//     root fits; the draft's noun was the adjective.
//   * label.feedback Réinject. -> Réinj., the glossary's listed abbreviation.
//     The ROOT "Réinjection" measures 60.27 px in a 62.00 px .knob-container —
//     1.73 px, which is this file's v2.6.0 header pin and holds when measured
//     (three of five headers re-measured in this stage did not). Réinj. is
//     29.39 px, so the caption gets the settled stem with room rather than a
//     margin no other font face survives. The header's old fallback "Retour"
//     is a monitor send and is forbidden; that line is rewritten below.
//
// ── THE TITLE IS STILL THE CAPTION, AND THAT IS A CHOICE ───────────────────
// M2 correction 9 accepts both branches. This page keeps the v2.6.0 branch:
// all 38 tip TITLES equal the caption the tip hangs off, and every abbreviated
// title's BODY opens by naming the control in full ("La réinjection renvoie…",
// "L’étalement en azimut détermine…"). The cost is visible and paid here: the
// Grain Size termNote is spent TWICE, on the caption and on the title, rather
// than once. Label-in-name holds by stem throughout (Réinj. ⊂ Réinjection,
// Taille ⊂ Taille de grain, Étal. ⊂ Étalement).
//
// ── WHAT THE READING FOUND, beyond the lint ────────────────────────────────
//   * tip.scan had DROPPED the control's own name: the English opens "Scan
//     Position moves the read point", the French opened with a bare verb.
//     Restored — it is the one entry on this page whose French said less than
//     its English (grew that tip 108.5 -> 123.9 px, still 253 px clear of the
//     floor).
//   * tip.reverse: "mêlent grains" -> "mêlent des grains" (missing partitive).
//   * tip.doppler: "comme une sirène qui passe monte puis descend" is a garden
//     path in French — the reader parses "qui passe monte". Now "comme une
//     sirène qui passe, dont le son monte puis redescend".
//   * tip.spread: "Un peu estompe l’attaque" is a calque; a bare "un peu"
//     cannot be a French subject. Now the "une faible valeur / une valeur
//     élevée" pair this file already uses in tip.ampRnd.
//   * tip.syncMode: "qu'une fois une division choisie" -> "la division".
//   * tip.feedback: comma splice before "il réchauffe donc" -> colon.
// Register is unchanged and consistent: vous is never used, instructions are
// infinitive ("À utiliser pour…", "À monter avec…"), and the option words the
// dropdowns show stay English inside the French sentences that name them.
//
// U+00A0 was inserted before % : ; ! ? and between every number and its unit,
// inside fr t:/b: string VALUES only — 51 of them, none in a key, a selector,
// a comment or a termNote, and 0 en values changed (both revisions were
// imported as modules and compared, not eyeballed in a diff full of invisible
// characters).
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
// HOVER-HELP ARRIVES IN v2.6.0, AND IT IS NOT ONLY COPY. v2.4.4 shipped ZERO
// native title=, zero aria-label, zero placeholder and zero data-tip — verified
// by grep, not assumed — so v2.5.0 correctly left `I18N` and `TIP_BINDINGS`
// empty and check-i18n assertion 2 reported "0 tip(s) bound". This version
// authors 38 entries (36 parameters + the gear + the language selector) and
// binds every one. It ALSO adds the thing that paints them: applyI18n() writes
// data-tip-title and data-tip onto the anchors and stops, and until this commit
// no code on this page read those attributes. See the I18N header below.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH IN `LABELS` WAS MOVED, NOT REWRITTEN. Every `en` entry in that
// block is byte-for-byte what index.html carried through v2.4.4, extracted
// mechanically rather than re-typed, with its HTML entities decoded to the
// characters they named (&amp; -> &) because textContent does not decode.
// `I18N` is the one block that is NOT moved English — hover-help prose did not
// exist on this page before v2.6.0, and its provenance is the parameter dump in
// .planning/params.tsv plus the DSP that reads each parameter, cited per entry.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: there is
// exactly ONE French string per key here and nothing chooses between variants
// at runtime. Every caption below was measured IN THIS PAGE'S OWN ELEMENT — a
// width borrowed from another plugin is a wrong number that reads exactly like
// a right one (K2 measured the same two words 8.24 px apart on two pages).
//
// The two caps that matter on this page:
//   .knob-name     lives in a .knob-container of width: 62px
//   .dropdown-name lives in a .dropdown-container of width: 80px
// Both containers carry min-width: auto, so a caption whose LONGEST WORD passes
// the cap raises the container's min-content and pushes its whole flex row.
// The spatial row has ZERO px of slack, so that push is not cosmetic there.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

// ============================================================================
// I18N — HOVER-HELP, authored in v2.6.0. `{en:{t,b}, fr:{t,b,reviewed}}`.
//
// EMPTY THROUGH v2.5.0, and that was this page's correct state: v2.4.4 carried
// no title=, no data-tip and no aria-label anywhere in index.html, so Stage K
// had nothing to move and forbade inventing any. v2.6.0 is Stage M and authors
// all 38 — 36 parameters plus the gear and the language selector.
//
// THE COPY ALONE WOULD HAVE BEEN INVISIBLE. applyI18n() writes data-tip-title
// and data-tip onto the anchors named in TIP_BINDINGS and stops there; the code
// that READS those attributes and paints a surface is per-plugin, and this page
// had none. check-i18n assertion 2 only counts bindings, check-ui-labels has no
// tooltip awareness at all, and boot-all-uis counts aria-label and title and
// never data-tip — so 38 unpaintable strings would have shipped past three
// green gates. setupTooltips() lands in js/app.js and the .tooltip rules in
// index.html in the SAME commit, and tests/ui_tip_render_check.js is the only
// gate in this repo that can see a rendered tip.
//
// ── THE TITLE IS THE PAGE'S CAPTION, NOT THE DUMP'S NAME ────────────────────
// Twelve of the 36 differ, and the caption wins per the Stage M brief: the user
// is reading the page, not the automation lane. Six of those twelve are
// ABBREVIATIONS forced by this page's 62 px .knob-container cap (Size Rnd, Amp
// Rnd, Pitch Rnd, Traj Speed, Dist LPF, and their French, which abbreviates
// harder still — "Alé. haut.", "Vit. traj.", "PB dist.", "Réinj.",
// "Porte bég.", "Répét."). A 280 px tooltip has no such cap, so every
// abbreviated title's BODY opens by naming the control in full. Recorded here
// rather than silently widened: the alternative — a tip titled differently from
// the caption it hangs off — is the one that leaves a user unable to tell which
// control they are reading about.
//
// ── RANGES COME FROM THE PAGE'S OWN FORMATTER ──────────────────────────────
// ALL 36 rows of .planning/params.tsv carry an EMPTY `label` column, so not one
// unit here came from the dump. Each was recovered from the formatter that
// renders that knob's readout, in js/app.js:
//   pctFormatter          app.js:249   -> %   (density, scan_position, spread,
//                                       reverse, feedback, dry_wet, size_random,
//                                       amp_random, pitch_random, pan_random,
//                                       probability, distance, spatial_width,
//                                       dist_lpf, doppler)
//   grainSizeFormatter    app.js:250   -> ms  (grain_size)
//   repeatsFormatter      app.js:251   -> bare integer (repeats,
//                                       euclidean_pulses/steps/rotation)
//   swingFormatter        app.js:255   -> %   (euclidean_swing)
//   degreeFormatter       app.js:259   -> deg (azimuth, elevation, az_spread,
//                                       el_spread)
//   trajSpeedFormatter    app.js:265   -> %   (traj_speed)
//   spatialSmoothFormatter app.js:266  -> ms  (spatial_smooth)
// The five choice and two boolean parameters take their option words as their
// range instead of a number, per the brief.
//
// ── OPTION WORDS STAY ENGLISH INSIDE A FRENCH SENTENCE ─────────────────────
// D-01 arm 1 exempts an AudioParameterChoice option ON THE PAGE so the host
// automation lane agrees character for character. A tooltip BODY is prose and
// is localized — but the option words it NAMES are the words the user has to
// find in the dropdown beside it, and those are still English. So a French body
// reads "Sept réglages : Free, 1/4, 1/8 ...". The two rules do not conflict:
// the option in the selector stays English, the sentence around it is French.
// This page already made that choice once, in label.spatialHint at v2.5.0.
//
// A CONTROL THIS TABLE ALSO CAPTIONS IS NAMED BY ITS FRENCH CAPTION, not by an
// invented synonym — "Sans effet tant que le réglage Alé. haut. reste à zéro",
// because that is the string the user is looking at 4 px away. tr() cannot
// resolve a {n} token to a LABELS key (its resolve() reads I18N only, and the
// canon is not trimmed per plugin), so these are written literally and a
// reviewer changing a caption must change the sentences that name it.
//
// NUMBERS INSIDE A BODY ARE PROSE, NOT READOUTS. D-03 exempts the readout NODE;
// it has nothing to say about a sentence. So the French takes French
// convention — a space before %, U+2212 for the minus in "−90 à +90°". Every
// range on this page renders as an integer (every formatter above calls
// Math.round), so the decimal comma never arises here; it is still the settled
// rule and the entries were scanned for a decimal POINT before commit.
// ============================================================================

export const I18N = Object.freeze({

    // ── Core Engine ─────────────────────────────────────────────────────────

    'tip.grainSize': {
        en: { t: 'Grain Size',
              b: 'Sets the length of every grain the engine plays. Short grains sound like '
               + 'texture or buzz; long ones keep enough of the source to stay recognisable. '
               + 'Range 10 to 500 ms.' },
        fr: { t: 'Taille',
              b: 'Règle la longueur de chaque grain lu par le moteur. Les grains courts donnent '
               + 'une texture ou un bourdonnement ; les longs conservent assez de la source pour '
               + 'rester reconnaissables. Plage de 10 à 500 ms.',
              reviewed: true,
              termNote: 'width, measured on this page at the shipping 900x800 frame: the glossary '
                         + 'roots taille de grain (76.94 px nowrap) and taille grain (62.28 px) '
                         + 'BOTH wrap to two 8.80 px lines inside .knob-name’s 18.00 px content '
                         + 'box — 17.80 px used, 0.20 px of clearance against check-ui-labels '
                         + 'assertion 4, on a page whose Windows metrics are unmeasured. Taille '
                         + 'is 31.33 px on one line and the tip body names the control in full.' },
        'zh-Hans': { t: '颗粒尺寸',
                     b: '设定引擎播放的每一个颗粒的长度。短颗粒听起来像织体或嗡鸣；长颗粒保留了足够多的源信号，仍然可以辨认。范围 10 到 500 ms。',
                     reviewed: 'mt' },
    },

    // The exponential mapping is quoted from GrainScheduler.h:49-50, not
    // guessed: the readout is a percentage and the audible result is a rate.
    'tip.density': {
        en: { t: 'Density',
              b: 'Sets how often grains are spawned while Sync Mode is Free, on an exponential '
               + 'curve: 1 % is about one grain a second, 50 % about ten, 100 % about a hundred. '
               + 'High settings thicken the cloud into a continuous tone. Range 1 to 100 %.' },
        fr: { t: 'Densité',
              b: 'Règle la fréquence d’apparition des grains lorsque Mode synchro est sur Free, '
               + 'selon une courbe exponentielle : 1 % donne environ un grain par seconde, 50 % '
               + 'environ dix, 100 % environ cent. Les valeurs élevées épaississent le nuage '
               + 'jusqu’au son continu. Plage de 1 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '密度',
                     b: '设定同步模式为 Free 时颗粒的生成频率，按指数曲线变化：1% 约为每秒一个颗粒，50% 约为十个，100% 约为一百个。高设定会把颗粒云加厚成连续的音。范围 1 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.scan': {
        en: { t: 'Scan',
              b: 'Scan Position moves the read point through the two seconds of audio the plugin '
               + 'keeps behind it, so grains are taken from further back rather than from the '
               + 'newest input. It is the control to reach for once Freeze is engaged. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Balayage',
              b: 'La position de balayage déplace le point de lecture dans les deux secondes '
               + 'd’audio conservées en mémoire : les grains sont alors prélevés plus en arrière '
               + 'plutôt que sur l’entrée la plus récente. C’est la commande à utiliser une '
               + 'fois Geler activé. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '扫描',
                     b: '扫描位置在插件保留的两秒音频中移动读取点，使颗粒取自更靠后的位置，而不是最新的输入。冻结启用之后，它就是首先该动的那个控制。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.spread': {
        en: { t: 'Spread',
              b: 'Scatters the read position of each grain around the scan point, by up to one '
               + 'grain length at full setting. A little blurs the attack; a lot smears the '
               + 'source into a cloud. Range 0 to 100 %.' },
        fr: { t: 'Étalement',
              b: 'Disperse la position de lecture de chaque grain autour du point de balayage, '
               + 'jusqu’à une longueur de grain au maximum. Une faible valeur estompe '
               + 'l’attaque ; une valeur élevée dissout la source en nuage. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '扩散',
                     b: '把每个颗粒的读取位置在扫描点周围散开，最大设定下的偏移可达一个颗粒长度。少量会让起音变模糊；大量则把源信号抹成一片颗粒云。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.reverse': {
        en: { t: 'Reverse',
              b: 'Sets the chance that any given grain plays backwards. It is a probability and '
               + 'not a switch, so mid settings mix forward and reversed grains in the same '
               + 'cloud. Range 0 to 100 %.' },
        fr: { t: 'Inversion',
              b: 'Détermine la probabilité qu’un grain donné soit lu à l’envers. C’est une '
               + 'probabilité et non un commutateur : les réglages intermédiaires mêlent des '
               + 'grains à l’endroit et à l’envers dans le même nuage. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '反向',
                     b: '设定任意一个颗粒倒着播放的概率。它是一个概率而不是开关，因此中间设定会让正放和倒放的颗粒混在同一片云里。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.feedback': {
        en: { t: 'Feedback',
              b: 'Feeds the granulated output back into the buffer it reads from, so grains '
               + 're-granulate themselves into longer tails. The path is tanh-saturated and '
               + 'gain-limited, so it warms rather than runs away. Range 0 to 100 %.' },
        fr: { t: 'Réinj.',
              b: 'La réinjection renvoie la sortie granulée dans la mémoire qu’elle relit : les '
               + 'grains se regranulent alors en traînes plus longues. Le trajet est saturé par '
               + 'tanh et limité en gain : il réchauffe donc au lieu de s’emballer. Plage de 0 à '
               + '100 %.',
              reviewed: true },
        'zh-Hans': { t: '反馈',
                     b: '把颗粒化之后的输出送回它所读取的缓冲区，使颗粒对自身再次颗粒化，形成更长的尾音。该路径经过 tanh 饱和并做了增益限制，因此只会变暖，不会失控。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.dryWet': {
        en: { t: 'Dry/Wet',
              b: 'Balances the untreated input against the granulated output. At 0 % only the '
               + 'dry signal passes; at 100 % only grains are heard. Range 0 to 100 %.' },
        fr: { t: 'Sec/Effet',
              b: 'Équilibre l’entrée non traitée et la sortie granulée. À 0 % seul le signal sec '
               + 'passe ; à 100 % on n’entend que les grains. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '干湿',
                     b: '在未处理的输入与颗粒化输出之间做平衡。在 0% 时只有干信号通过；在 100% 时只听到颗粒。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.sizeRnd': {
        en: { t: 'Size Rnd',
              b: 'Size Random varies the length of each grain, stretching it by up to double its '
               + 'Grain Size at full setting. It breaks up the metallic pitch that a constant '
               + 'grain length produces. Range 0 to 100 %.' },
        fr: { t: 'Alé. taille',
              b: 'L’aléa de taille fait varier la longueur de chaque grain, jusqu’au double de '
               + 'la valeur de Taille au réglage maximal. Cela casse la hauteur métallique '
               + 'qu’engendre une longueur de grain constante. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '尺寸随机',
                     b: '尺寸随机改变每个颗粒的长度，在最大设定下最多可以拉长到其颗粒尺寸的两倍。它可以打散恒定颗粒长度所产生的金属感音高。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.ampRnd': {
        en: { t: 'Amp Rnd',
              b: 'Amp Random varies the level of each grain, attenuating it by up to its full '
               + 'amplitude. Small amounts make a mechanical cloud breathe; large ones thin it '
               + 'out. Range 0 to 100 %.' },
        fr: { t: 'Alé. ampl.',
              b: 'L’aléa d’amplitude fait varier le niveau de chaque grain, jusqu’à l’atténuer '
               + 'complètement. De faibles valeurs font respirer un nuage mécanique ; de fortes '
               + 'valeurs l’éclaircissent. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '振幅随机',
                     b: '振幅随机改变每个颗粒的电平，最多可以把它完全衰减掉。少量能让机械的颗粒云呼吸起来；大量则会把它变稀。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.shape': {
        en: { t: 'Shape',
              b: 'Grain Shape chooses the amplitude envelope applied to every grain, which is '
               + 'what decides whether a grain sounds soft-edged or percussive. Hann and '
               + 'Blackman are the smoothest, Trapezoid holds a flat top, and Exp Decay gives '
               + 'each grain a plucked attack. Six shapes: Hann, Triangle, Trapezoid, Tukey, '
               + 'Blackman, Exp Decay.' },
        fr: { t: 'Forme',
              b: 'La forme de grain choisit l’enveloppe d’amplitude appliquée à chaque grain, ce '
               + 'qui détermine s’il sonne doux ou percussif. Hann et Blackman sont les plus '
               + 'douces, Trapezoid garde un plateau et Exp Decay donne à chaque grain une '
               + 'attaque pincée. Six formes : Hann, Triangle, Trapezoid, Tukey, Blackman, '
               + 'Exp Decay.',
              reviewed: true },
        'zh-Hans': { t: '形状',
                     b: '颗粒形状选择施加在每个颗粒上的振幅包络，也就决定了颗粒听起来是柔和的还是打击性的。Hann 和 Blackman 最平滑，Trapezoid 保持一段平顶，Exp Decay 则给每个颗粒一个拨奏般的起音。六种形状：Hann、Triangle、Trapezoid、Tukey、Blackman、Exp Decay。',
                     reviewed: 'mt' },
    },

    // ── Pitch & Scale ───────────────────────────────────────────────────────

    'tip.pitchRnd': {
        en: { t: 'Pitch Rnd',
              b: 'Pitch Random sets how far each grain may be transposed away from the source '
               + 'pitch, quantised to the Scale and Root Note beside it. At 0 % it is off, and '
               + 'the Scale, Root Note and Pitch Mode controls are dimmed with it. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Alé. haut.',
              b: 'L’aléa de hauteur détermine l’ampleur de la transposition possible de chaque '
               + 'grain par rapport à la hauteur d’origine, quantifiée sur Gamme et '
               + 'Fondamentale. À 0 % la fonction est inactive et les commandes Gamme, '
               + 'Fondamentale et Mode hauteur sont estompées avec elle. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '音高随机',
                     b: '音高随机设定每个颗粒相对源音高最多可以移调多远，并量化到旁边的音阶和根音上。在 0% 时它处于关闭状态，音阶、根音和音高模式三个控制也随之变暗。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.panRnd': {
        en: { t: 'Pan Rnd',
              b: 'Pan Random scatters each grain across the stereo image instead of leaving the '
               + 'whole cloud centred. It applies to the stereo path only and is ignored while '
               + 'Spatial Audio is engaged. Range 0 to 100 %.' },
        fr: { t: 'Alé. pan',
              b: 'L’aléa de panoramique disperse chaque grain dans l’image stéréo au lieu de '
               + 'laisser tout le nuage au centre. Ne s’applique qu’au trajet stéréo et reste '
               + 'sans effet lorsque l’Audio spatial est engagé. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '声像随机',
                     b: '声像随机把每个颗粒散布到立体声像各处，而不是让整片云都居中。它只作用于立体声路径，空间音频启用时会被忽略。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.scale': {
        en: { t: 'Scale',
              b: 'Quantises every random transposition to a musical scale, so a scattered cloud '
               + 'stays in key rather than drifting. It does nothing until Pitch Rnd is above '
               + 'zero. Five scales: Chromatic, Major, Minor, Pentatonic, Whole Tone.' },
        fr: { t: 'Gamme',
              b: 'Quantifie chaque transposition aléatoire sur une gamme musicale : le nuage '
               + 'dispersé reste alors dans la tonalité au lieu de dériver. Sans effet tant que '
               + 'le réglage Alé. haut. reste à zéro. Cinq gammes : Chromatic, Major, Minor, '
               + 'Pentatonic, Whole Tone.',
              reviewed: true },
        'zh-Hans': { t: '音阶',
                     b: '把每一次随机移调都量化到一个音乐音阶上，使散开的颗粒云保持在调内而不是跑调。在音高随机大于零之前它不起作用。五种音阶：Chromatic、Major、Minor、Pentatonic、Whole Tone。',
                     reviewed: 'mt' },
    },

    'tip.rootNote': {
        en: { t: 'Root Note',
              b: 'Sets the tonic the chosen Scale is built on, so the quantised grains land in '
               + 'the key of the track. It does nothing until Pitch Rnd is above zero. '
               + 'Twelve semitones, C through B.' },
        fr: { t: 'Fondamentale',
              b: 'Définit la tonique sur laquelle la Gamme choisie est construite, afin que les '
               + 'grains quantifiés tombent dans la tonalité du morceau. Sans effet tant que le '
               + 'réglage Alé. haut. reste à zéro. Douze demi-tons, de C à B.',
              reviewed: true },
        'zh-Hans': { t: '根音',
                     b: '设定所选音阶所构建于的主音，使量化后的颗粒落在乐曲的调上。在音高随机大于零之前它不起作用。十二个半音，从 C 到 B。',
                     reviewed: 'mt' },
    },

    'tip.pitchMode': {
        en: { t: 'Pitch Mode',
              b: 'Chooses how successive grains pick their transposition: freely at random, '
               + 'climbing or descending the scale a degree at a time, or bouncing between the '
               + 'two. Ladder and Pendulum give an arpeggio where Random gives a cloud. '
               + 'Four modes: Random, Ladder Up, Ladder Down, Pendulum.' },
        fr: { t: 'Mode hauteur',
              b: 'Choisit la façon dont les grains successifs prennent leur transposition : '
               + 'librement au hasard, en montant ou en descendant la gamme degré par degré, ou '
               + 'en faisant l’aller-retour entre les deux. Ladder et Pendulum donnent un arpège '
               + 'là où Random donne un nuage. Quatre modes : Random, Ladder Up, Ladder Down, '
               + 'Pendulum.',
              reviewed: true },
        'zh-Hans': { t: '音高模式',
                     b: '选择相继的颗粒如何取得各自的移调量：完全随机，沿音阶逐级上行或下行，或者在两者之间往复。Ladder 和 Pendulum 给出琶音，Random 给出颗粒云。四种模式：Random、Ladder Up、Ladder Down、Pendulum。',
                     reviewed: 'mt' },
    },

    // ── Beat Sync ───────────────────────────────────────────────────────────

    'tip.syncMode': {
        en: { t: 'Sync Mode',
              b: 'Chooses whether grains are spawned at the Density rate or locked to the host '
               + 'tempo on a musical division. Probability, Repeats, Stutter Gate and the whole '
               + 'Euclidean Rhythm group only apply once a division is chosen. Seven settings: '
               + 'Free, 1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T.' },
        fr: { t: 'Mode synchro',
              b: 'Détermine si les grains sont déclenchés au rythme de Densité ou verrouillés '
               + 'sur le tempo de l’hôte selon une division musicale. Probabilité, Répét., '
               + 'Porte bég. et tout le groupe Rythme euclidien ne s’appliquent qu’une fois la '
               + 'division choisie. Sept réglages : Free, 1/4, 1/8, 1/16, 1/32, 1/8T, 1/16T.',
              reviewed: true },
        'zh-Hans': { t: '同步模式',
                     b: '选择颗粒是按密度速率生成，还是锁定到宿主速度的某个音乐细分上。概率、重复、断续门以及整个欧几里得节奏组，只有在选定一个细分之后才起作用。七种设定：Free、1/4、1/8、1/16、1/32、1/8T、1/16T。',
                     reviewed: 'mt' },
    },

    'tip.probability': {
        en: { t: 'Probability',
              b: 'Sets the chance that a scheduled trigger actually spawns a grain, thinning a '
               + 'regular pattern into an irregular one. At 100 % every trigger fires. '
               + 'Range 0 to 100 %.' },
        fr: { t: 'Probabilité',
              b: 'Détermine la probabilité qu’un déclenchement prévu produise réellement un '
               + 'grain, ce qui éclaircit un motif régulier en motif irrégulier. À 100 % chaque '
               + 'déclenchement se produit. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '概率',
                     b: '设定一次预定的触发实际生成颗粒的概率，把规整的节奏图案变得不规则。在 100% 时每一次触发都会发声。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.repeats': {
        en: { t: 'Repeats',
              b: 'Sets how many grains a single trigger fires, spaced one subdivision apart, '
               + 'which is what turns a hit into a stutter. It applies in the tempo-locked Sync '
               + 'Modes only. Range 1 to 16 grains.' },
        fr: { t: 'Répét.',
              b: 'Le nombre de répétitions détermine combien de grains produit un même '
               + 'déclenchement, espacés d’une subdivision, ce qui transforme une frappe en '
               + 'bégaiement. Ne s’applique qu’aux modes synchronisés au tempo. '
               + 'Plage de 1 à 16 grains.',
              reviewed: true },
        'zh-Hans': { t: '重复',
                     b: '设定单次触发发射多少个颗粒，彼此相隔一个细分，这正是把一次击打变成断续的原因。它只在锁定速度的同步模式下起作用。范围 1 到 16 个颗粒。',
                     reviewed: 'mt' },
    },

    'tip.stutterGate': {
        en: { t: 'Stutter Gate',
              b: 'Mutes the dry signal for as long as a repeat burst lasts, so the stutter '
               + 'replaces the source instead of sitting on top of it. It needs a tempo-locked '
               + 'Sync Mode and a Repeats count above one to be heard at all. Off or On.' },
        fr: { t: 'Porte bég.',
              b: 'La porte de bégaiement coupe le signal sec pendant toute la durée d’une rafale '
               + 'de répétitions : le bégaiement remplace alors la source au lieu de s’y '
               + 'superposer. Nécessite un Mode synchro verrouillé au tempo et un réglage '
               + 'Répét. supérieur à un. Désactivé ou activé.',
              reviewed: true },
        'zh-Hans': { t: '断续门',
                     b: '在一串重复持续期间把干信号静音，使断续取代源信号，而不是叠在它上面。它需要一个锁定速度的同步模式，并且重复次数大于一，才听得到。关或开。',
                     reviewed: 'mt' },
    },

    // ── Freeze ──────────────────────────────────────────────────────────────

    'tip.freeze': {
        en: { t: 'Freeze',
              b: 'Captures the last two seconds of input and holds it, so the engine keeps '
               + 'granulating that snapshot while live audio passes by underneath. Use Scan to '
               + 'move through what was captured. Off or On.' },
        fr: { t: 'Geler',
              b: 'Capture les deux dernières secondes d’entrée et les maintient : le moteur '
               + 'continue de granuler cet instantané pendant que l’audio en direct défile. '
               + 'Utiliser Balayage pour parcourir ce qui a été capturé. Désactivé ou activé.',
              reviewed: true },
        'zh-Hans': { t: '冻结',
                     b: '捕获最近两秒的输入并把它保持住，使引擎持续对这份快照做颗粒化，而现场音频在其下方经过。用扫描在已捕获的内容中移动。关或开。',
                     reviewed: 'mt' },
    },

    // ── Euclidean Rhythm ────────────────────────────────────────────────────

    'tip.pulses': {
        en: { t: 'Pulses',
              b: 'Sets how many hits the Euclidean pattern spreads as evenly as it can across '
               + 'its steps. Few pulses in many steps give the sparse, off-kilter figures the '
               + 'algorithm is known for. Range 1 to 16 pulses.' },
        fr: { t: 'Impulsions',
              b: 'Détermine le nombre de frappes que le motif euclidien répartit aussi '
               + 'régulièrement que possible sur ses pas. Peu d’impulsions dans beaucoup de pas '
               + 'donnent les figures clairsemées et décalées propres à cet algorithme. '
               + 'Plage de 1 à 16 impulsions.',
              reviewed: true },
        'zh-Hans': { t: '脉冲',
                     b: '设定欧几里得图案在它的步数上尽可能均匀地分布多少次击打。在很多步中放入少量脉冲，就得到该算法著称的稀疏而错位的节奏型。范围 1 到 16 个脉冲。',
                     reviewed: 'mt' },
    },

    'tip.steps': {
        en: { t: 'Steps',
              b: 'Sets the length of the Euclidean pattern in subdivisions, which is the cycle '
               + 'the pulses are distributed over. A step count that is not a multiple of the '
               + 'pulse count is what produces the interesting patterns. Range 2 to 16 steps.' },
        fr: { t: 'Pas',
              b: 'Détermine la longueur du motif euclidien en subdivisions, soit le cycle sur '
               + 'lequel les impulsions sont réparties. C’est lorsque le nombre de pas n’est pas '
               + 'un multiple du nombre d’impulsions que les motifs deviennent intéressants. '
               + 'Plage de 2 à 16 pas.',
              reviewed: true },
        'zh-Hans': { t: '步',
                     b: '以细分为单位设定欧几里得图案的长度，也就是脉冲所分布的那个循环。步数不是脉冲数的整数倍时，才会产生有趣的图案。范围 2 到 16 步。',
                     reviewed: 'mt' },
    },

    'tip.rotation': {
        en: { t: 'Rotation',
              b: 'Turns the Euclidean pattern in place, moving where its first pulse falls '
               + 'without changing which pulses exist. It is the quickest way to shift a figure '
               + 'off the downbeat. Range 0 to 15 steps.' },
        fr: { t: 'Rotation',
              b: 'Fait tourner le motif euclidien sur lui-même : la première impulsion change de '
               + 'place sans que les impulsions elles-mêmes changent. C’est le moyen le plus '
               + 'rapide de décaler une figure hors du temps fort. Plage de 0 à 15 pas.',
              reviewed: true },
        'zh-Hans': { t: '旋转',
                     b: '就地转动欧几里得图案，改变它的第一个脉冲落在何处，而不改变有哪些脉冲。这是把节奏型移离强拍最快的办法。范围 0 到 15 步。',
                     reviewed: 'mt' },
    },

    // The 50-75 % range is not a coincidence of the readout: swingRatio is
    // (pct - 50) / 50, so 75 % is exactly half a subdivision (GrainScheduler.h:85-87).
    'tip.swing': {
        en: { t: 'Swing',
              b: 'Delays every off-beat division, so the pattern falls with a shuffle instead of '
               + 'straight. 50 % is straight and 75 % pushes the off-beat a full half-subdivision '
               + 'late. Range 50 to 75 %.' },
        fr: { t: 'Swing',
              b: 'Retarde chaque division à contretemps, ce qui fait tomber le motif en shuffle '
               + 'plutôt qu’en binaire. 50 % est binaire et 75 % repousse le contretemps d’une '
               + 'demi-subdivision complète. Plage de 50 à 75 %.',
              reviewed: true },
        'zh-Hans': { t: '摇摆',
                     b: '延迟每一个弱拍细分，使图案带着摇摆而不是平直地落下。50% 是平直的，75% 把弱拍整整推后半个细分。范围 50 到 75%。',
                     reviewed: 'mt' },
    },

    // ── Spatial Audio ───────────────────────────────────────────────────────

    'tip.mode': {
        en: { t: 'Mode',
              b: 'Spatial Mode chooses how grains are placed in three dimensions. Scatter throws '
               + 'each grain to a fixed random point inside the spread; Trajectory moves them '
               + 'along a path instead, which is what brings the last four controls in this row '
               + 'to life. Three modes: Off, Scatter, Trajectory.' },
        fr: { t: 'Mode',
              b: 'Le mode spatial détermine la façon dont les grains sont placés en trois '
               + 'dimensions. Scatter projette chaque grain vers un point aléatoire fixe à '
               + 'l’intérieur de l’étalement ; Trajectory les déplace plutôt le long d’un '
               + 'parcours, ce qui donne vie aux quatre dernières commandes de cette rangée. '
               + 'Trois modes : Off, Scatter, Trajectory.',
              reviewed: true },
        'zh-Hans': { t: '模式',
                     b: '空间模式选择颗粒如何在三维中被摆放。Scatter 把每个颗粒抛到扩散范围内一个固定的随机点上；Trajectory 则让它们沿一条路径移动，这也正是让本行最后四个控制起作用的设定。三种模式：Off、Scatter、Trajectory。',
                     reviewed: 'mt' },
    },

    'tip.azimuth': {
        en: { t: 'Azimuth',
              b: 'Sets the horizontal direction the grain cloud is centred on, measured '
               + 'clockwise around the listener. Az Spread then scatters grains either side of '
               + 'it. Range 0 to 360°.' },
        fr: { t: 'Azimut',
              b: 'Définit la direction horizontale autour de laquelle le nuage de grains est '
               + 'centré, mesurée dans le sens horaire autour de l’auditeur. Étal. az. disperse '
               + 'ensuite les grains de part et d’autre. Plage de 0 à 360°.',
              reviewed: true },
        'zh-Hans': { t: '方位角',
                     b: '设定颗粒云所居中的水平方向，以听者为中心顺时针度量。方位扩散随后把颗粒散布在它的两侧。范围 0 到 360°。',
                     reviewed: 'mt' },
    },

    'tip.elevation': {
        en: { t: 'Elevation',
              b: 'Sets the height the grain cloud is centred on, from directly below the '
               + 'listener to directly above. El Spread then scatters grains either side of it. '
               + 'Range −90 to +90°.' },
        fr: { t: 'Élévation',
              b: 'Définit la hauteur autour de laquelle le nuage de grains est centré, de la '
               + 'verticale sous l’auditeur à la verticale au-dessus. Étal. él. disperse '
               + 'ensuite les grains de part et d’autre. Plage de −90 à +90°.',
              reviewed: true },
        'zh-Hans': { t: '仰角',
                     b: '设定颗粒云所居中的高度，从听者的正下方直到正上方。仰角扩散随后把颗粒散布在它的两侧。范围 −90 到 +90°。',
                     reviewed: 'mt' },
    },

    'tip.azSpread': {
        en: { t: 'Az Spread',
              b: 'Azimuth Spread sets how wide an arc grains are scattered over horizontally, '
               + 'around the Azimuth centre. Width scales this and the vertical spread together. '
               + 'Range 0 to 360°.' },
        fr: { t: 'Étal. az.',
              b: 'L’étalement en azimut détermine la largeur de l’arc sur lequel les grains '
               + 'sont dispersés horizontalement, autour du centre défini par Azimut. Largeur '
               + 'met cet étalement et l’étalement vertical à l’échelle ensemble. Plage de 0 à '
               + '360°.',
              reviewed: true },
        'zh-Hans': { t: '方位扩散',
                     b: '方位扩散设定颗粒在水平方向上散布的弧有多宽，围绕方位角中心展开。宽度会同时缩放它和垂直方向的扩散。范围 0 到 360°。',
                     reviewed: 'mt' },
    },

    'tip.elSpread': {
        en: { t: 'El Spread',
              b: 'Elevation Spread sets how far grains are scattered vertically, around the '
               + 'Elevation centre. Width scales this and the horizontal spread together. '
               + 'Range 0 to 180°.' },
        fr: { t: 'Étal. él.',
              b: 'L’étalement en élévation détermine la hauteur de l’arc sur lequel les grains '
               + 'sont dispersés verticalement, autour du centre défini par Élévation. Largeur '
               + 'met cet étalement et l’étalement horizontal à l’échelle ensemble. Plage de 0 '
               + 'à 180°.',
              reviewed: true },
        'zh-Hans': { t: '仰角扩散',
                     b: '仰角扩散设定颗粒在垂直方向上散布得有多远，围绕仰角中心展开。宽度会同时缩放它和水平方向的扩散。范围 0 到 180°。',
                     reviewed: 'mt' },
    },

    'tip.distance': {
        en: { t: 'Distance',
              b: 'Places the grain cloud nearer or further from the listener. Reach for it to '
               + 'push a texture behind the mix rather than turning it down; with Dist LPF up it '
               + 'darkens as it recedes as well. Range 0 to 100 %.' },
        fr: { t: 'Distance',
              b: 'Place le nuage de grains plus près ou plus loin de l’auditeur. À utiliser pour '
               + 'reculer une texture derrière le mixage plutôt que d’en baisser le niveau ; avec '
               + 'PB dist. relevé, elle s’assombrit aussi en s’éloignant. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '距离',
                     b: '把颗粒云放得离听者更近或者更远。想把一层织体推到混音之后而不是把它调小时，就用它；距离低通调高之后，它在远去的同时也会变暗。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.width': {
        en: { t: 'Width',
              b: 'Spatial Width scales both spread arcs at once, collapsing the cloud onto its '
               + 'Azimuth and Elevation centre at 0 % and opening it to the full spread at '
               + '100 %. Range 0 to 100 %.' },
        fr: { t: 'Largeur',
              b: 'La largeur spatiale met les deux arcs d’étalement à l’échelle en même temps : '
               + 'à 0 % le nuage se referme sur le centre défini par Azimut et Élévation, '
               + 'à 100 % il s’ouvre à l’étalement complet. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '宽度',
                     b: '空间宽度同时缩放两个扩散弧，在 0% 时把颗粒云收拢到它的方位角和仰角中心上，在 100% 时展开到完整的扩散范围。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.trajectory': {
        en: { t: 'Trajectory',
              b: 'Chooses the path grains travel while they sound, once Mode is set to '
               + 'Trajectory. Orbital circles the listener, Spiral climbs as it turns, and '
               + 'Random drifts. Four paths: Static, Orbital, Spiral, Random.' },
        fr: { t: 'Trajectoire',
              b: 'Choisit le parcours suivi par les grains pendant qu’ils sonnent, une fois Mode '
               + 'réglé sur Trajectory. Orbital tourne autour de l’auditeur, Spiral monte en '
               + 'tournant et Random dérive. Quatre parcours : Static, Orbital, Spiral, Random.',
              reviewed: true },
        'zh-Hans': { t: '轨迹',
                     b: '在模式设为 Trajectory 之后，选择颗粒发声期间所走的路径。Orbital 绕着听者环行，Spiral 一边转一边上升，Random 则四处漂移。四条路径：Static、Orbital、Spiral、Random。',
                     reviewed: 'mt' },
    },

    'tip.trajSpeed': {
        en: { t: 'Traj Speed',
              b: 'Trajectory Speed scales how fast grains travel along the chosen path, from '
               + 'held still to four times the base rate. Raise it with Doppler up for an '
               + 'audible fly-past. Range 0 to 400 %.' },
        fr: { t: 'Vit. traj.',
              b: 'La vitesse de trajectoire met à l’échelle la rapidité de déplacement des '
               + 'grains le long du parcours choisi, de l’immobilité à quatre fois la vitesse de '
               + 'base. À monter avec Doppler pour un passage audible. Plage de 0 à 400 %.',
              reviewed: true },
        'zh-Hans': { t: '轨迹速度',
                     b: '轨迹速度缩放颗粒沿所选路径行进的快慢，从静止不动直到基准速率的四倍。把它和多普勒一起调高，就能听到一次飞掠。范围 0 到 400%。',
                     reviewed: 'mt' },
    },

    // 20 kHz down to 5 kHz is the actual coefficient, from
    // PluginProcessor.cpp:750 — lpfCutoff = 20000 - distanceNorm * 15000 * distLpfAmt.
    'tip.distLpf': {
        en: { t: 'Dist LPF',
              b: 'Distance LPF sets how much Distance darkens the cloud, by scaling a low-pass '
               + 'that closes from 20 kHz down to 5 kHz at full distance. At 0 % a distant cloud '
               + 'stays bright. Range 0 to 100 %.' },
        fr: { t: 'PB dist.',
              b: 'Le passe-bas de distance détermine dans quelle mesure Distance assombrit le '
               + 'nuage, en dosant un filtre qui se referme de 20 kHz à 5 kHz à distance '
               + 'maximale. À 0 % un nuage éloigné reste brillant. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '距离低通',
                     b: '距离低通设定距离把颗粒云变暗的程度，做法是缩放一个从 20 kHz 一直收到最远处 5 kHz 的低通。在 0% 时，远处的颗粒云依然明亮。范围 0 到 100%。',
                     reviewed: 'mt',
                     termNote: 'Dist is DISTANCE here, not distortion. The glossary root 失真低通 renders the other sense of the abbreviation and would be flatly wrong on a control whose own body reads "Distance LPF sets how much Distance darkens the cloud" — and it would also stop colliding with label.distance 距离, which is the control it scales. Same reasoning as label.distLpf. Reported so the glossary row can be corrected: O-GrainScatter is the only site in the corpus and the root was derived without one.' },
    },

    'tip.doppler': {
        en: { t: 'Doppler',
              b: 'Shifts a moving grain’s pitch with its motion, the way a passing siren rises '
               + 'and then falls. It only speaks when grains are actually moving, so it needs a '
               + 'Trajectory and some Traj Speed. Range 0 to 100 %.' },
        fr: { t: 'Doppler',
              b: 'Décale la hauteur d’un grain en mouvement selon son déplacement, comme une '
               + 'sirène qui passe, dont le son monte puis redescend. Ne se manifeste que si '
               + 'les grains bougent réellement : il faut donc une Trajectoire et un peu de Vit. '
               + 'traj. Plage de 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '多普勒',
                     b: '让移动中的颗粒的音高随它的运动而偏移，就像驶过的警笛先升后降。只有颗粒真的在移动时它才发声，因此需要一条轨迹和一些轨迹速度。范围 0 到 100%。',
                     reviewed: 'mt' },
    },

    'tip.smoothing': {
        en: { t: 'Smoothing',
              b: 'Spatial Smooth sets how long the encoder takes to follow a change of position, '
               + 'gliding the ambisonic coefficients rather than jumping them. Short values '
               + 'track a fast trajectory; long ones remove the zipper a jumped position makes. '
               + 'Range 1 to 200 ms.' },
        fr: { t: 'Lissage',
              b: 'Le lissage spatial détermine le temps que met l’encodeur à suivre un '
               + 'changement de position, en faisant glisser les coefficients ambisoniques au '
               + 'lieu de les faire sauter. Les valeurs courtes suivent une trajectoire rapide ; '
               + 'les longues suppriment le crépitement d’un saut de position. '
               + 'Plage de 1 à 200 ms.',
              reviewed: true },
        'zh-Hans': { t: '平滑',
                     b: '空间平滑设定编码器跟随一次位置变化所需要的时间，让高保真立体声的系数滑过去而不是跳过去。短值能跟上快速的轨迹；长值则消除位置跳变所产生的阶跃噪声。范围 1 到 200 ms。',
                     reviewed: 'mt' },
    },

    // ── Chrome ──────────────────────────────────────────────────────────────
    //
    // The gear tip is what tells a user hover-help exists at all, so its body
    // must describe only what the popover ACTUALLY contains.
    //
    // ── v2.8.0: THE SETTINGS BODY WAS FALSE, AND SO WAS THIS COMMENT ────────
    // Both used to assert the popover reaches one control. v2.7.0 put the
    // hover-help switch inside it — #tips-toggle sits in .settings-popover at
    // index.html, and check-i18n assertion 16 REQUIRES that switch to exist,
    // keyed and bound, which is exactly what makes the old claim false. The
    // clause is DELETED rather than widened to name two controls, because a
    // body that inventories a panel has to be re-edited in every language every
    // time the panel grows, and that is the edit that gets forgotten. The
    // superseded phrasings live in the CHANGELOG and are deliberately not
    // repeated here, so a repo probe for either stays at zero.
    //
    // The language body closed by naming the selector's options. That was true
    // for exactly as long as the selector held two entries, and this version
    // adds a third. Deleted for the same reason, and because the selector
    // already lists the languages in their own endonyms — the one place the
    // list cannot go stale. Everything else in that body is still true and
    // stays: the host automation lane and the on-screen values remain English.

    'tip.settings': {
        en: { t: 'Settings',
              b: 'Opens the settings panel.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages.',
              reviewed: true },
        'zh-Hans': { t: '设置',
              b: '打开设置面板。',
              reviewed: 'mt' },
    },

    // The endonyms are quoted as the selector spells them — a language name is
    // never translated, which is why they are not keyed on the page either.
    'tip.language': {
        en: { t: 'Language',
              b: 'Chooses the language of the interface text and of this hover-help. Parameter '
               + 'names in the host automation lane and the values on screen stay English.' },
        fr: { t: 'Langue',
              b: 'Choisit la langue du texte de l’interface et de ces infobulles. Les '
               + 'noms de paramètres dans la voie d’automatisation de l’hôte et les valeurs '
               + 'affichées restent en anglais.',
              reviewed: true },
        'zh-Hans': { t: '语言',
              b: '选择界面文字和这些悬停帮助的语言。宿主自动化通道中的参数名称和屏幕上的数值保持英文。',
              reviewed: 'mt' },
    },
    // v2.7.0 — the switch that reaches this whole layer.
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
                     reviewed: 'mt' },
    },
});

// ============================================================================
// LABELS — one string per key, no body. `{en:{t}, fr:{t, reviewed}}`.
// Widths in the comments are the RENDERED line box measured in this page's own
// element at 900x800, in px.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The <h1> is the product name and is NOT keyed — see I18N_EXEMPT.
    'label.tagline':       { en: { t: 'Harmonic Stutter Engine' },
                             fr: { t: 'Moteur de bégaiement harmonique', reviewed: true },
        'zh-Hans': { t: '谐波断续引擎', reviewed: 'mt' },
    },

    // ── Visualisation panel captions ────────────────────────────────────────
    // Both are position:absolute inside their .viz-panel, so neither can push
    // anything; the only budget is the panel's own width (~558 and ~300 px).
    // NOT the product name: "O-GrainScatter" is one token and is exempt, while
    // this is a description of what the panel draws.
    'label.vizGrain':      { en: { t: 'Grain Scatter' },
                             fr: { t: 'Dispersion de grains', reviewed: true },
        'zh-Hans': { t: '颗粒散布', reviewed: 'mt' },
    },   // 84.67 -> 126.45
    'label.vizEuclidean':  { en: { t: 'Euclidean' },
                             fr: { t: 'Euclidien', reviewed: true },
        'zh-Hans': { t: '欧几里得', reviewed: 'mt' },
    },              // 61.97 -> 59.44  SHRANK

    // ── Freeze toggle ───────────────────────────────────────────────────────
    // Alone in a full-width, centre-justified .freeze-bar, so its own box is
    // the only thing that changes and it is a [data-i18n] element. No pin.
    'label.freeze':        { en: { t: 'Freeze' },
                             fr: { t: 'Geler', reviewed: true },
        'zh-Hans': { t: '冻结', reviewed: 'mt' },
    },                  // 71.67 -> 66.72 border box  SHRANK

    // ── Group headings ──────────────────────────────────────────────────────
    // Every .group-label is a full-width block, so its rectangle is
    // language-invariant by construction and none of these can move anything.
    'label.coreEngine':      { en: { t: 'Core Engine' },
                               fr: { t: 'Moteur principal', reviewed: true },
        'zh-Hans': { t: '核心引擎', reviewed: 'mt' },
    },
    'label.pitchScale':      { en: { t: 'Pitch & Scale' },
                               fr: { t: 'Hauteur et gamme', reviewed: true },
        'zh-Hans': { t: '音高与音阶', reviewed: 'mt' },
    },
    'label.beatSync':        { en: { t: 'Beat Sync' },
                               fr: { t: 'Synchro rythmique', reviewed: true },
        'zh-Hans': { t: '节拍同步', reviewed: 'mt' },
    },
    'label.euclideanRhythm': { en: { t: 'Euclidean Rhythm' },
                               fr: { t: 'Rythme euclidien', reviewed: true },
        'zh-Hans': { t: '欧几里得节奏', reviewed: 'mt' },
    },
    'label.spatialAudio':    { en: { t: 'Spatial Audio' },
                               fr: { t: 'Audio spatial', reviewed: true },
        'zh-Hans': { t: '空间音频', reviewed: 'mt' },
    },

    // ── Core Engine ─────────────────────────────────────────────────────────
    // "Taille" rather than the glossary's "Taille de grain" / "Taille grain",
    // and v2.6.1 re-measured both rather than inheriting the claim: 76.94 px
    // and 62.28 px nowrap, so BOTH wrap to two 8.80 px line boxes inside an
    // 18.00 px content box — 17.80 px used, 0.20 px of clearance against
    // check-ui-labels assertion 4, tighter than the v2.6.0 note's "0.5 px".
    // Every knob in this group acts on a grain and the sibling GRAIN SHAPE
    // caption is already the bare word "Shape". Carried as the termNote on the
    // entry, with the numbers, so the glossary can grow an abbreviation.
    'label.grainSize':     { en: { t: 'Grain Size' },
                             fr: { t: 'Taille', reviewed: true,
                                   termNote: 'width, measured on this page at the shipping '
                                            + '900x800 frame: the glossary roots taille de grain '
                                            + '(76.94 px nowrap) and taille grain (62.28 px) BOTH '
                                            + 'wrap to two 8.80 px lines inside .knob-name’s '
                                            + '18.00 px content box — 17.80 px used, 0.20 px of '
                                            + 'clearance against check-ui-labels assertion 4, on '
                                            + 'a page whose Windows metrics are unmeasured. '
                                            + 'Taille is 31.33 px on one line and the tip body '
                                            + 'names the control in full.' },
        'zh-Hans': { t: '颗粒尺寸', reviewed: 'mt' },
    },   // 50.61 -> 31.33  SHRANK
    'label.density':       { en: { t: 'Density' },
                             fr: { t: 'Densité', reviewed: true },
        'zh-Hans': { t: '密度', reviewed: 'mt' },
    },                // 38.34 -> 38.64
    'label.scan':          { en: { t: 'Scan' },
                             fr: { t: 'Balayage', reviewed: true },
        'zh-Hans': { t: '扫描', reviewed: 'mt' },
    },               // 23.14 -> 46.11
    'label.spread':        { en: { t: 'Spread' },
                             fr: { t: 'Étalement', reviewed: true },
        'zh-Hans': { t: '扩散', reviewed: 'mt' },
    },              // 34.58 -> 53.83, 8.17 under the 62 px cap
    'label.reverse':       { en: { t: 'Reverse' },
                             fr: { t: 'Inversion', reviewed: true },
        'zh-Hans': { t: '反向', reviewed: 'mt' },
    },              // 40.23 -> 49.63, 12.37 under the cap
    // v2.6.1: the glossary's listed abbreviation, re-measured on this page.
    // "Réinjection" is 60.27 px in the 62.00 px .knob-container — the v2.6.0
    // pin holds — and "Réinj." is 29.39. The old note offered "Retour" as the
    // roomier lever; Retour is a monitor send and the glossary forbids it.
    // The tip body opens "La réinjection renvoie…", so the root is one hover
    // away from the caption that abbreviates it.
    'label.feedback':      { en: { t: 'Feedback' },
                             fr: { t: 'Réinj.', reviewed: true },
        'zh-Hans': { t: '反馈', reviewed: 'mt' },
    },                 // 46.53 -> 29.39  SHRANK
    'label.dryWet':        { en: { t: 'Dry/Wet' },
                             fr: { t: 'Sec/Effet', reviewed: true },
        'zh-Hans': { t: '干湿', reviewed: 'mt' },
    },              // 41.77 -> 48.09
    'label.sizeRnd':       { en: { t: 'Size Rnd' },
                             fr: { t: 'Alé. taille', reviewed: true },
        'zh-Hans': { t: '尺寸随机', reviewed: 'mt' },
    },            // 41.33 -> 53.33
    'label.ampRnd':        { en: { t: 'Amp Rnd' },
                             fr: { t: 'Alé. ampl.', reviewed: true },
        'zh-Hans': { t: '振幅随机', reviewed: 'mt' },
    },             // 40.84 -> 49.16
    'label.shape':         { en: { t: 'Shape' },
                             fr: { t: 'Forme', reviewed: true },
        'zh-Hans': { t: '形状', reviewed: 'mt' },
    },                  // 28.98 -> 31.52

    // ── Pitch & Scale ───────────────────────────────────────────────────────
    'label.pitchRnd':      { en: { t: 'Pitch Rnd' },
                             fr: { t: 'Alé. haut.', reviewed: true },
        'zh-Hans': { t: '音高随机', reviewed: 'mt' },
    },             // 48.78 -> 49.56
    'label.panRnd':        { en: { t: 'Pan Rnd' },
                             fr: { t: 'Alé. pan', reviewed: true },
        'zh-Hans': { t: '声像随机', reviewed: 'mt' },
    },               // 39.56 -> 39.89
    'label.scale':         { en: { t: 'Scale' },
                             fr: { t: 'Gamme', reviewed: true },
        'zh-Hans': { t: '音阶', reviewed: 'mt' },
    },                  // 27.55 -> 33.73
    'label.rootNote':      { en: { t: 'Root Note' },
                             fr: { t: 'Fondamentale', reviewed: true },
        'zh-Hans': { t: '根音', reviewed: 'mt' },
    },           // 51.17 -> 73.41, 6.59 under the 80 px cap
    'label.pitchMode':     { en: { t: 'Pitch Mode' },
                             fr: { t: 'Mode hauteur', reviewed: true },
        'zh-Hans': { t: '音高模式', reviewed: 'mt' },
    },           // 56.13 -> 72.30
    // The hint NAMES a control whose caption this table also owns, so it takes
    // the caption as a {n} token rather than a second copy of the same words.
    // trLabel() resolves a token that is itself a LABELS key, so a reviewer who
    // changes label.pitchRnd changes this sentence with it instead of leaving
    // the two to drift. The markup keeps the fully-written English as its
    // render-if-applyI18n-never-runs fallback.
    'label.pitchHint':     { en: { t: 'Increase {n} to activate' },
                             fr: { t: 'Augmenter {n} pour activer', reviewed: true },
        'zh-Hans': { t: '提高 {n} 即可启用', reviewed: 'mt' },
    },

    // ── Beat Sync ───────────────────────────────────────────────────────────
    'label.syncMode':      { en: { t: 'Sync Mode' },
                             fr: { t: 'Mode synchro', reviewed: true },
        'zh-Hans': { t: '同步模式', reviewed: 'mt' },
    },           // 51.70 -> 71.30
    // The TIGHTEST caption on the page: 2.98 px under the 62 px cap. Kept as
    // the whole word because the ENGLISH is the same long word at 58.72 — the
    // French is 0.30 px wider than what already ships, so an abbreviation here
    // would buy nothing English does not already spend. "Probab." (37.94) is
    // the reviewer's lever if a Windows metric ever proves it necessary.
    'label.probability':   { en: { t: 'Probability' },
                             fr: { t: 'Probabilité', reviewed: true },
        'zh-Hans': { t: '概率', reviewed: 'mt' },
    },            // 58.72 -> 59.02  TIGHTEST
    // "Répétitions" (59.16) clears the cap by 2.84 px. Unlike PROBABILITÉ there
    // is no English precedent for spending that: "Repeats" is 39.25. The page's
    // own register already abbreviates (Size Rnd, Amp Rnd, Dist LPF, Traj Speed).
    'label.repeats':       { en: { t: 'Repeats' },
                             fr: { t: 'Répét.', reviewed: true },
        'zh-Hans': { t: '重复', reviewed: 'mt' },
    },                 // 39.25 -> 31.06  SHRANK
    // Pinned to 110px in index.html. "Porte bégaiement" (138.58 border box) is
    // 28.58 px past that pin and would have to move the pin, which would move
    // ENGLISH. "Bégaiement" (102.00) fits and is the reviewer's lever; it drops
    // the gate half of the name, which is why it is not the shipped choice.
    'label.stutterGate':   { en: { t: 'Stutter Gate' },
                             fr: { t: 'Porte bég.', reviewed: true },
        'zh-Hans': { t: '断续门', reviewed: 'mt' },
    },             // 109.95 -> 93.08 border box  SHRANK

    // ── Euclidean Rhythm ────────────────────────────────────────────────────
    'label.pulses':        { en: { t: 'Pulses' },
                             fr: { t: 'Impulsions', reviewed: true },
        'zh-Hans': { t: '脉冲', reviewed: 'mt' },
    },             // 32.97 -> 55.48, 6.52 under the cap
    'label.steps':         { en: { t: 'Steps' },
                             fr: { t: 'Pas', reviewed: true },
        'zh-Hans': { t: '步', reviewed: 'mt' },
    },                    // 26.53 -> 16.23  SHRANK
    'label.rotation':      { en: { t: 'Rotation' },
                             fr: { t: 'Rotation', reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '旋转', reviewed: 'mt' },
    },
    // The musical term is used untranslated in French practice, and there is no
    // French word for it that is not a paraphrase ("balancement" is 66.53 and
    // 4.53 px OVER the cap in its own right).
    'label.swing':         { en: { t: 'Swing' },
                             fr: { t: 'Swing', reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '摇摆', reviewed: 'mt' },
    },

    // ── Spatial Audio ───────────────────────────────────────────────────────
    // This is the ZERO-SLACK row: its twelve controls sum to exactly 846.00 px
    // inside an 846.00 px container. Every French caption below was chosen so
    // its longest word stays under its container's cap, because one that does
    // not wraps SMOOTHING onto a second row, grows #spatial-group by 87 px and
    // pushes the page past its own 800 px frame.
    'label.mode':          { en: { t: 'Mode' },
                             fr: { t: 'Mode', reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '模式', reviewed: 'mt' },
    },
    'label.azimuth':       { en: { t: 'Azimuth' },
                             fr: { t: 'Azimut', reviewed: true },
        'zh-Hans': { t: '方位角', reviewed: 'mt' },
    },                 // 41.73 -> 34.72  SHRANK
    'label.elevation':     { en: { t: 'Elevation' },
                             fr: { t: 'Élévation', reviewed: true },
        'zh-Hans': { t: '仰角', reviewed: 'mt' },
    },              // 50.64 -> 50.64  IDENTICAL WIDTH
    'label.azSpread':      { en: { t: 'Az Spread' },
                             fr: { t: 'Étal. az.', reviewed: true },
        'zh-Hans': { t: '方位扩散', reviewed: 'mt' },
    },              // 48.19 -> 41.30  SHRANK
    'label.elSpread':      { en: { t: 'El Spread' },
                             fr: { t: 'Étal. él.', reviewed: true },
        'zh-Hans': { t: '仰角扩散', reviewed: 'mt' },
    },              // 48.06 -> 41.17  SHRANK
    'label.distance':      { en: { t: 'Distance' },
                             fr: { t: 'Distance', reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '距离', reviewed: 'mt' },
    },
    'label.width':         { en: { t: 'Width' },
                             fr: { t: 'Largeur', reviewed: true },
        'zh-Hans': { t: '宽度', reviewed: 'mt' },
    },                // 30.89 -> 42.00
    // KEYED, while the spatial_mode OPTION spelled the same way is EXEMPT.
    // That is the one state assertion 14 demands a scope for, and the exempt
    // entry below carries `option`.
    'label.trajectory':    { en: { t: 'Trajectory' },
                             fr: { t: 'Trajectoire', reviewed: true },
        'zh-Hans': { t: '轨迹', reviewed: 'mt' },
    },            // 56.88 -> 60.80
    'label.trajSpeed':     { en: { t: 'Traj Speed' },
                             fr: { t: 'Vit. traj.', reviewed: true },
        'zh-Hans': { t: '轨迹速度', reviewed: 'mt' },
    },             // 52.81 -> 44.72  SHRANK
    'label.distLpf':       { en: { t: 'Dist LPF' },
                             fr: { t: 'PB dist.', reviewed: true },
        'zh-Hans': { t: '距离低通', reviewed: 'mt',
                     termNote: 'Dist is DISTANCE here, not distortion. The glossary root 失真低通 renders the other sense of the abbreviation; this caption scales the low-pass that Distance drives, so 距离低通 is the reading and 失真低通 would name a control this plugin does not have. Same reasoning as tip.distLpf, and a termNote is ENTRY-scoped so both rows carry it.' },
    },               // 38.98 -> 36.75  SHRANK
    'label.doppler':       { en: { t: 'Doppler' },
                             fr: { t: 'Doppler', reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '多普勒', reviewed: 'mt' },
    },
    'label.smoothing':     { en: { t: 'Smoothing' },
                             fr: { t: 'Lissage', reviewed: true },
        'zh-Hans': { t: '平滑', reviewed: 'mt' },
    },                // 54.84 -> 36.83  SHRANK
    // "Scatter" and "Trajectory" stay ENGLISH inside the French sentence on
    // purpose: they are the two spatial_mode option strings the user has to
    // find in the dropdown beside it, and those are exempt under D-01 arm 1.
    // A translated instruction naming a control that is not translated is the
    // instruction that cannot be followed.
    'label.spatialHint':   { en: { t: 'Set Mode to Scatter or Trajectory to enable' },
                             fr: { t: 'Régler Mode sur Scatter ou Trajectory pour activer',
                                   reviewed: true },
        'zh-Hans': { t: '把模式设为 Scatter 或 Trajectory 即可启用', reviewed: 'mt' },
    },                              // 154.25 -> 184.22, in an 846 px block

    // ── Settings popover (new in v2.5.0) ────────────────────────────────────
    'label.language':      { en: { t: 'Language' },
                             fr: { t: 'Langue', reviewed: true },
        'zh-Hans': { t: '语言', reviewed: 'mt' },
    },

    // v2.7.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true },
        'zh-Hans': { t: '悬停帮助', reviewed: 'mt' },
    },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true },
        'zh-Hans': { t: '开', reviewed: 'mt' },
    },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true },
        'zh-Hans': { t: '关', reviewed: 'mt' },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. These
    // have no rendered box, so neither is a geometry risk.
    //
    // These two are the ONLY accessible names on the page, and both belong to
    // controls this commit ADDS. No existing control gains one: v2.4.4 carried
    // no title= to move under contract §4, and inventing hover-help prose for
    // the other 46 captions is Stage M.
    'aria.settings':       { en: { t: 'Settings' },
                             fr: { t: 'Réglages', reviewed: true },
        'zh-Hans': { t: '设置', reviewed: 'mt' },
    },
    'aria.langSelect':     { en: { t: 'Interface language' },
                             fr: { t: 'Langue de l’interface', reviewed: true },
        'zh-Hans': { t: '界面语言', reviewed: 'mt' },
    },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true },
        'zh-Hans': { t: '开关悬停帮助', reviewed: 'mt' },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element,
// a setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let
// a missed label hide as a deliberate one.
//
// An entry is [text, reason] or [text, reason, scope]. A scope is REQUIRED
// exactly where the same string is also KEYED on this page — the one state in
// which a text match cannot tell a deliberate skip from a forgotten label.
// On this page that is "Trajectory", and only "Trajectory".
// ============================================================================

export const I18N_EXEMPT = [
    // The product name, in the <h1>. One token, no split.
    ['O-GrainScatter', 'the product name — a product name is never translated'],

    // ── D-01 ARM 1: AudioParameterChoice option strings, BYTE-IDENTICAL ─────
    // Verified against PluginProcessor.cpp rather than assumed: grain_shape at
    // :135, scale at :105, root_note at :111, pitch_mode at :197, sync_mode at
    // :164, spatial_mode at :239, trajectory at :281. The page and the host
    // automation lane must agree on these, character for character.
    //
    // The twelve root_note options (C, C#, D … B) are not listed: they are
    // single letters, the coverage scan never classifies them as LABEL, and an
    // entry the scan cannot reach is decoration rather than an exemption.
    ['Hann',       'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Triangle',   'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Trapezoid',  'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Tukey',      'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Blackman',   'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],
    ['Exp Decay',  'a grain_shape option string VERBATIM (PluginProcessor.cpp:135) — D-01 arm 1'],

    ['Chromatic',  'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Major',      'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Minor',      'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Pentatonic', 'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],
    ['Whole Tone', 'a scale option string VERBATIM (PluginProcessor.cpp:105) — D-01 arm 1'],

    ['Random',       'a pitch_mode AND a trajectory option string VERBATIM '
                   + '(PluginProcessor.cpp:197, :281) — D-01 arm 1. Unscoped is correct: '
                   + 'both nodes carrying this text are options, and no LABELS key resolves to it'],
    ['Ladder Up',    'a pitch_mode option string VERBATIM (PluginProcessor.cpp:197) — D-01 arm 1'],
    ['Ladder Down',  'a pitch_mode option string VERBATIM (PluginProcessor.cpp:197) — D-01 arm 1'],
    ['Pendulum',     'a pitch_mode option string VERBATIM (PluginProcessor.cpp:197) — D-01 arm 1'],

    ['Free',       'a sync_mode option string VERBATIM (PluginProcessor.cpp:164) — D-01 arm 1'],

    ['Off',        'a spatial_mode option string VERBATIM (PluginProcessor.cpp:239) — D-01 arm 1'],
    ['Scatter',    'a spatial_mode option string VERBATIM (PluginProcessor.cpp:239) — D-01 arm 1. '
                 + 'Also the word the French spatial hint keeps in English, for the same reason'],

    // THE ONE SCOPED ENTRY. "Trajectory" is a spatial_mode option AND the
    // caption of the trajectory dropdown, which IS keyed (label.trajectory ->
    // "Trajectoire"). Unscoped, this entry would silence the caption too, and a
    // forgotten label there would read exactly like this deliberate skip —
    // O-Detune's "Random" in Stage K, on a different page. The scope pins it to
    // the <option> and nothing else.
    ['Trajectory', 'a spatial_mode option string VERBATIM (PluginProcessor.cpp:239) — D-01 arm 1. '
                 + 'SCOPED because the .dropdown-name caption spelled the same way IS keyed as '
                 + 'label.trajectory and must translate',
                   'option'],

    ['Static',     'a trajectory option string VERBATIM (PluginProcessor.cpp:281) — D-01 arm 1'],
    ['Orbital',    'a trajectory option string VERBATIM (PluginProcessor.cpp:281) — D-01 arm 1'],
    ['Spiral',     'a trajectory option string VERBATIM (PluginProcessor.cpp:281) — D-01 arm 1'],

    // ── CANVAS TEXT: a named gap, recorded rather than discovered ───────────
    // GrainScatterViz.draw() paints "<n> grains" with ctx.fillText into
    // #grain-canvas. A 2D-context string is not a DOM node: the canon sweep
    // cannot reach it, neither gate can see it, and localizing it would need a
    // repaint hook outside the canon. This is the suite's existing position —
    // O-Orbit ships FRONT and ELEV the same way, O-MultiBandCompressor its
    // analyzer placeholder, O-simpleSampler two waveform-editor strings. Named
    // gap, owner none, carried into Stage M.
    ['grains',     'canvas ctx.fillText in GrainScatterViz.draw (app.js) — not a DOM node, so the '
                 + 'canon sweep cannot reach it; the same named gap as O-Orbit, '
                 + 'O-MultiBandCompressor and O-simpleSampler'],
    // The canvas's other four strings — "0s", "2s", "+24st", "-24st" and the
    // Euclidean centre readout "4/8 r2" — are axis units and numbers, exempt
    // under D-03 in their own right as well as being canvas text.

    // ── Endonyms ────────────────────────────────────────────────────────────
    // The three <option> texts inside #lang-select. index.html writes the
    // Chinese one as numeric character references, but the HTML parser decodes
    // them long before the coverage sweep runs, so the exemption has to carry
    // the DECODED characters.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
    ['简体中文', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — 38 anchors, authored in v2.6.0.
//
// [selector, key] or [selector, key, wrapperSelector]. applyI18n() runs
// document.querySelector(selector), then closest(wrapper) when a third element
// is present, and writes data-tip-title + data-tip onto whatever that lands on.
//
// ── "BIND TO THE IDS THE UI ALREADY USES" IS FALSE HERE, IN BOTH HALVES ─────
//
// T17 says to bind the ids the UI already has. This page has FOUR ids in total
// — #gear-btn, #settings-popover, #lang-select and #stutter-gate-btn — plus
// #grain-canvas, #euclidean-canvas, #pitch-hint and #spatial-hint, none of
// which is a control. Thirty-four of the 36 parameters carry NO id at all: a
// knob is `.knob[data-param="…"]` and a dropdown is `select[data-param="…"]`,
// exactly the shape O-Chorus reported in M1. The selector half fails on 34 of
// 36.
//
// The TARGET half fails independently, and on the same 34: `.knob` is a 48 px
// circle with a caption above it and a readout below, all three inside a 62 px
// `.knob-container`. A tip bound to the circle alone would not open when the
// pointer is on the caption or the value — the two parts a user reads FIRST —
// so every knob and every dropdown walks `closest()` up to its container.
//
// ── THE CHROME IS BOUND BARE, AND THAT IS DELIBERATE ────────────────────────
//
// #gear-btn and #lang-select share an ancestor: .settings-cluster wraps the
// gear button AND the popover the selector lives in (index.html:471-489). A
// wrapper walk from either would land on the cluster, and hovering the language
// selector would then show the GEAR's tip — O-Comp's carried trap 3. Both are
// bound to themselves. #stutter-gate-btn and the Freeze toggle are bound bare
// for a different reason: each IS the whole control, a full button with its own
// caption, so there is no smaller node to walk up from. The Freeze toggle sits
// alone in a full-width .freeze-bar, and binding that bar would arm a tip
// across the entire 900 px width of the page.
// ============================================================================

export const TIP_BINDINGS = [
    // ── Core Engine ─────────────────────────────────────────────────────────
    ['.knob[data-param="grain_size"]',         'tip.grainSize',   '.knob-container'],
    ['.knob[data-param="density"]',            'tip.density',     '.knob-container'],
    ['.knob[data-param="scan_position"]',      'tip.scan',        '.knob-container'],
    ['.knob[data-param="spread"]',             'tip.spread',      '.knob-container'],
    ['.knob[data-param="reverse"]',            'tip.reverse',     '.knob-container'],
    ['.knob[data-param="feedback"]',           'tip.feedback',    '.knob-container'],
    ['.knob[data-param="dry_wet"]',            'tip.dryWet',      '.knob-container'],
    ['.knob[data-param="size_random"]',        'tip.sizeRnd',     '.knob-container'],
    ['.knob[data-param="amp_random"]',         'tip.ampRnd',      '.knob-container'],
    ['select[data-param="grain_shape"]',       'tip.shape',       '.dropdown-container'],

    // ── Pitch & Scale ───────────────────────────────────────────────────────
    ['.knob[data-param="pitch_random"]',       'tip.pitchRnd',    '.knob-container'],
    ['.knob[data-param="pan_random"]',         'tip.panRnd',      '.knob-container'],
    ['select[data-param="scale"]',             'tip.scale',       '.dropdown-container'],
    ['select[data-param="root_note"]',         'tip.rootNote',    '.dropdown-container'],
    ['select[data-param="pitch_mode"]',        'tip.pitchMode',   '.dropdown-container'],

    // ── Beat Sync ───────────────────────────────────────────────────────────
    ['select[data-param="sync_mode"]',         'tip.syncMode',    '.dropdown-container'],
    ['.knob[data-param="probability"]',        'tip.probability', '.knob-container'],
    ['.knob[data-param="repeats"]',            'tip.repeats',     '.knob-container'],
    ['#stutter-gate-btn',                      'tip.stutterGate'],

    // ── Freeze ──────────────────────────────────────────────────────────────
    ['.freeze-toggle',                         'tip.freeze'],

    // ── Euclidean Rhythm ────────────────────────────────────────────────────
    ['.knob[data-param="euclidean_pulses"]',   'tip.pulses',      '.knob-container'],
    ['.knob[data-param="euclidean_steps"]',    'tip.steps',       '.knob-container'],
    ['.knob[data-param="euclidean_rotation"]', 'tip.rotation',    '.knob-container'],
    ['.knob[data-param="euclidean_swing"]',    'tip.swing',       '.knob-container'],

    // ── Spatial Audio ───────────────────────────────────────────────────────
    ['select[data-param="spatial_mode"]',      'tip.mode',        '.dropdown-container'],
    ['.knob[data-param="azimuth"]',            'tip.azimuth',     '.knob-container'],
    ['.knob[data-param="elevation"]',          'tip.elevation',   '.knob-container'],
    ['.knob[data-param="az_spread"]',          'tip.azSpread',    '.knob-container'],
    ['.knob[data-param="el_spread"]',          'tip.elSpread',    '.knob-container'],
    ['.knob[data-param="distance"]',           'tip.distance',    '.knob-container'],
    ['.knob[data-param="spatial_width"]',      'tip.width',       '.knob-container'],
    ['select[data-param="trajectory"]',        'tip.trajectory',  '.dropdown-container'],
    ['.knob[data-param="traj_speed"]',         'tip.trajSpeed',   '.knob-container'],
    ['.knob[data-param="dist_lpf"]',           'tip.distLpf',     '.knob-container'],
    ['.knob[data-param="doppler"]',            'tip.doppler',     '.knob-container'],
    ['.knob[data-param="spatial_smooth"]',     'tip.smoothing',   '.knob-container'],

    // ── Chrome — BARE, see the note above ───────────────────────────────────
    ['#gear-btn',                              'tip.settings'],
    ['#lang-select',                           'tip.language'],
    ['#tips-toggle',                           'tip.tipsToggle'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// LIVE AS OF v2.6.0. applyI18n() calls it once per TIP_BINDINGS row — 38 of
// them now, where v2.5.0 had none — and the {t, b} it returns is written onto
// the anchor as data-tip-title + data-tip. Every visible CAPTION still goes
// through trLabel() instead; the two lookups are separate on purpose, because
// LABELS entries have no body and I18N entries have both.
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
