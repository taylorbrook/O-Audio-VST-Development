/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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
// i18n.js — O-Bitrot page labels and hover-help: English, French and
// Simplified Chinese (v1.16.0)
//
// ── v1.15.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 49 entries (22 terminology, 17 typography, 5 grammar/agreement,
// 5 meaning). sameAsEn: kept 8, translated 0. termNote exemptions: 1 (listed
// below). Left as drafted: the rest. reviewed: false throughout — no native
// speaker has read this file yet, and that flag records the human, not a lint.
// (The banner above said v1.14.0 through v1.15.0; corrected here.)
//
// The lint went 44 findings -> 1. THE ONE THAT STAYS is ROT_ENABLE's body:
// `--strict` reports T2 (decimal point) on the version token "1.10". A version
// identifier is not a decimal number — the English body carries the SAME token
// ("a pre-1.10 session"), which is the discriminator T2 already applies to
// surround-format names. Dropping the version would lose a clause the English
// has. Reported to the orchestrator, not worked around with a termNote (a
// termNote exempts G1/F1 only, and would have been the wrong instrument).
//
// ── The decisions a later reader needs ─────────────────────────────────────
//
// MEASURED, with the gate's own method (Range.selectNodeContents on the
// shipping node at 900 x 740). Three of this file's captions could not take
// their glossary ROOT term:
//   * label.depth -> `Prof.`, not `Profondeur`. Profondeur measures 75.36 px
//     against .mix-text's 76 px pin — 0.64 px of margin, which no other font
//     face survives. Prof. is 31.39.
//   * label.conceal -> `Dissim.`, not `Dissimulation` (88.97 px, one
//     unbreakable word, in an 82 px .ctl). Dissim. is 42.98 and shares its
//     stem with the tip title, which `Masquage` did not.
//   * label.pop stays `Clics`: `Craquements` is 84.47 px in a 64 px cell. The
//     VINYL_POP tip title is now "Clics et craquements" so the caption's own
//     word appears in the tip that explains it.
// label.warp DID take the abbreviation: `Déform.` is 50.38 px in 64, so
// `Voile` was never a width defence. The tip title is the root `Déformation`;
// `voile` survives in the BODIES (`un disque voilé`, `l'ondulation du
// voilage`), which is where the idiom for a warped LP belongs.
//
// TERMINOLOGY SETTLED ON THE PAGE, not only against the list:
//   * AGC had two French faces on one page — the caption said AGC, the tip
//     title said "Gain automatique". The title is now AGC, and both bodies say
//     AGC, expanded once as `commande automatique de gain (AGC)`.
//   * Germe -> Graine everywhere. diceBtn's title read `Retirer un germe`,
//     which says REMOVE a seed; it is now `Nouvelle graine`.
//   * `lit de souffle` / `lit de bruit` -> `nappe de …` on four bodies. `lit`
//     is a calque of "bed"; French audio says `nappe`.
//   * CODEC_MAINS's body named `Bruit de ligne`; the caption on the page is
//     `Bruit`, and the English body names its caption too. Now `Bruit`.
//   * PACKET_CONCEAL's body now names the four French faces the user can see
//     (Silence / Répéter / Déclin / Substituer), as the English body names its
//     own. label.decay went `Fondu` -> `Déclin` for the same reason: fondu
//     alone is a fade. The <select> is fixed-width and did not move.
//
// GEOMETRY. label.annotSplices was `fondus contournés lorsque allumé` — a
// missing elision. The correct `lorsqu'il est allumé` measured 117.95 px but
// WRAPPED to two lines in the 132 px .annot box and moved 164 non-label
// elements. `lorsqu'allumé` is 124.8 px, one line, elision correct, and the
// geometry diff is back to its baseline of exactly one element (the #viewSync
// <select>, dw=7.0, unchanged since before this pass).
//
// LABEL-IN-NAME (WCAG 2.5.3) holds by STEM on the three abbreviations that
// have a tip title to hold to: Déform. ⊂ Déformation, Dissim. ⊂ Dissimulation,
// Fréq. ⊂ Fréquence d'échantillonnage. `Prof.` has no stem in its own tip
// title (`Gravité d'inversion`) — the ENGLISH pair disagrees the same way
// (Depth / Flip severity), so the French mirrors it rather than inventing one.
//
// label.prob (`Prob.`) and label.env (`Env.`) now carry sameAsEn: true. They
// are the same abbreviation in both languages, differing only by the French
// period, and the flag is the existing declaration that a human looked.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz), which on
// this plugin means the ENTIRE UI — O-Bitrot's controller is one inline
// <script type="module"> in index.html, evaluated top to bottom, with no init()
// function to isolate a failure. scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTip() builds the tip
// with createElement + textContent, and check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing `<`. A line break,
// if one is ever needed, is \n plus CSS white-space: pre-line, never a tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below is byte-for-byte
// what index.html carried through v1.13.0, extracted mechanically rather than
// re-typed, with its HTML entities decoded to the characters they named
// (&amp; -> &) because setAttribute + textContent do not decode entities.
//
// KEYS ARE THE PARAMETER ID where the anchor has one, and the element id
// otherwise. 41 of the 53 anchors on this page carry NO id — they are `.ctl`
// and `.g-group` wrappers around a `[data-param]` knob — so binding by id was
// never an option here. The canonical [selector, key, wrapper] triple is what
// addresses them: the selector finds the knob, and closest(wrapper) walks back
// up to the cell the tip actually belongs on.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================
//
// ── v1.16.0: SIMPLIFIED CHINESE (zh-Hans rollout Stage 3, 2026-09-04) ───────
//
// 117 entries — 55 I18N (a title and a body each) + 62 LABELS (a title each) —
// which is 172 back-translation rows. LANGUAGES is three long, the widening
// shape check-i18n assertion [1] accepts alongside the two-language one.
//
// ── THE RENDERINGS ARE THE GLOSSARY'S, NOT THIS FILE'S ──────────────────────
//
// Every English string that is a TERMS key in scripts/i18n-zh-glossary.js takes
// that term's ROOT rendering. Lint rule Z5 holds this file to them; the run is
// ZERO findings across all nine rules (Z1 Z2 Z3 Z4 Z5 Z6 Z7 F1 R1).
//
// FOUR termNote EXEMPTIONS, and they are TWO homograph collisions, each one hit
// twice because the caption and the tooltip title are separate rows:
//
//   Mains  -> 市电,   NOT the glossary root 主输出.
//       Two senses of one English word. 主输出 is a mixer's main OUTPUT bus;
//       this control names the 50/60 Hz hum frequency of a line-noise bed, and
//       the root would label a thing that does not exist on this page. 市电 is
//       the power supply. (label.mains, CODEC_MAINS)
//
//   Jitter -> 时基抖动, NOT the shared root 抖动.
//       DITHER AND JITTER SHARE ONE GLOSSARY ROOT, and on this page they are
//       TWO KNOBS TWO CELLS APART inside the same Crush plate. Rendering both
//       抖动 would ship an identical caption on two different controls. Dither
//       KEEPS the root — 抖动 is the DSP process's own Chinese name and takes no
//       qualifier — and Jitter takes the standard qualified form 时基抖动,
//       time-base jitter, which is what Chinese audio writing calls it.
//       Qualifying jitter is idiomatic; qualifying dither is not. That
//       asymmetry is why the root went to dither. (label.jitter, CRUSH_JITTER)
//
// TWO sameAsEn ENTRIES: label.agc and CODEC_AGC's title, both the token AGC.
// Flagged rather than exempted, for the reason the French entry gives — an
// identical string that is identical ON PURPOSE still needs a human to agree.
//
// TERMS THIS PLUGIN SETTLED that the 552-term glossary does not carry. Recorded
// so the Stage-4 lo-fi/degradation plugins inherit rather than re-derive:
//
//     tape 磁带          hiss 嘶声 (root)     wow & flutter 慢抖与快抖
//     dropout 失落       stop (tape) 停带     CD skip CD 跳碟
//     locked groove 锁纹  groove jump 跳纹    vinyl 黑胶      pops 爆音
//     rot (bit rot) 腐化  bit flip 位翻转     garble 乱码     sticky 卡死
//     packet 数据包       loss rate 丢包率    burstiness 突发性
//     concealment 丢包隐藏  comfort noise 舒适噪声
//     codec 编解码器      line coding 线路编码  line noise 线路噪声
//     mains (electrical) 市电   decimation 抽取   sample rate 采样率
//     jitter 时基抖动     dither 抖动          seed 种子 (root)
//     reseed 重设种子     splice 剪接          hard edges 硬边缘 (root)
//     anti-shock loop 防震循环   servo seek 伺服寻道   sector 扇区
//     warp (a disc) 扭曲 in the caption, 翘曲 in the BODIES — the glossary root
//         is 扭曲 and Z5 requires it on the title, but the physical deformation
//         of an LP is 翘曲 in Chinese and that is the word the two bodies use.
//
// ── THE TYPOGRAPHY RULES ────────────────────────────────────────────────────
//   Z1  full-width punctuation throughout — ，。；、 never ASCII.
//   Z2  NO U+00A0 anywhere. The French bodies on this same page carry them by
//       rule; the full-width forms already carry their own sidebearing. Same
//       file, opposite rule.
//   Z4  ONE PLAIN U+0020 at every Latin/digit-to-Han boundary — "20 ms 数据包",
//       "1.10 之前的会话", "μ-law 压扩或 GSM 全速率". Chosen once and applied
//       TABLE-WIDE; the unspaced count is zero, so the rule holds by
//       consistency rather than by majority.
//   Z7  no full-width Latin or digits. Units stay ASCII — ms, Hz, kHz, LSB, %.
//
//   AND ONE NO LINT CAN SEE: a space between TWO HAN RUNS. Z4's census only
//   matches a Latin/digit run adjacent to a Han run, so 写入 源 is invisible to
//   it and to the other eight rules. O-Octagon shipped one and caught it by
//   READING, not by a gate. This table was scanned for /\p{Script=Han}
//   \p{Script=Han}/u and returns 0.
//
// ── THE CJK FONT TAIL: ONE TOKEN, ONE OMISSION ──────────────────────────────
//
// The tail is `, 'PingFang SC', 'Microsoft YaHei', sans-serif`. All 13
// `font-family` declarations in index.html read `var(--serif)`, so it went on
// the TOKEN — see the block at the top of the inline <style>.
//
// The set was MEASURED: the page served, switched to Chinese, and
// getComputedStyle().fontFamily read on every node that holds OR CAN RECEIVE a
// Han codepoint (own text, data-tip, data-tip-title, aria-label, data-label,
// data-on/off/confirm). 118 nodes resolved through --serif. Two of them a
// [data-i18n]-derived scan would have MISSED, and they are the same two
// O-Chorus and O-Octagon found — the #tooltip surface, filled from data-tip at
// hover time and never keyed, and the endonym <option>, the only Han in the
// markup. That is now 3-for-3; treat it as universal.
//
// LEFT UNTOUCHED (1): #diceBtn.die resolves to Chromium's UA button default,
// Arial. It is reached by the Han scan only through its data-tip — the die face
// is the glyph U+2685 — and a data-tip paints into #tooltip, which IS --serif.
// There is no Han glyph to fall back for. The omission is recorded because a
// wave that appends the tail to everything it measured can no longer tell a
// needed tail from a decorative one.
//
// ── GEOMETRY: CHINESE BUYS WIDTH AND SPENDS HEIGHT ──────────────────────────
//
// The first Chinese run moved 185 non-label elements. Nine CSS pins closed all
// of them, and every value is a MEASURED English number — see the v1.16.0 block
// in index.html for the table. Seven are unitless line-heights against
// `line-height: normal`, which is the FONT'S OWN metrics; two are widths on
// content-sized <select>s. NO GLOBAL line-height was added: a global one moves
// English geometry, which is the regression the gates exist to catch.
//
// THE TWO <select>s MOVED IN OPPOSITE DIRECTIONS, and that is the finding:
//   #viewSync   en 58 -> fr 65 -> zh 66   pinned 66   GREW
//   PACKET_CONCEAL  en 82 -> fr 82 -> zh 57   pinned 82   SHRANK by 25 px
// An assertion phrased "the non-English pass must not GROW box X" is VACUOUS
// against Chinese. Assert equality, not an inequality.
//
// ── THE CHARACTER BUDGETS: NONE WAS ADDED, AND THE MEASUREMENT IS WHY ───────
//
// maxChars = floor(cellWidthPx / fontSizePx). A knob column is pinned at 64 px
// and .ctl-label renders at 9.5 px, so the budget is 6 characters. The two
// longest Chinese captions on the page are 时基抖动 and 舒适噪声, four each,
// measuring 43.64 px in a 64 px cell. check-ui-labels assertion 4 passes on all
// three arms. The three existing BUDGETS cells are O-Chorus's and are
// UNTOUCHED; scripts/i18n-zh-glossary.js is not in this change.
//
// This is the opposite of French, where THREE of this page's captions had to be
// abbreviated to fit — Prof., Dissim., Clics. Every Chinese caption here is
// NARROWER than its English original except two.
//
// ── THE BAR IS reviewed: 'bt', AND WHAT THAT ASSERTS ────────────────────────
//
// 'bt' means a SECOND, INDEPENDENT pass — one that never saw the English source
// — rendered the Chinese back into English, and the drift was read against the
// original. 'native' stays OPEN and is NOT a blocker: THIS PROJECT HAS NO
// NATIVE CHINESE READER, and that is a disclosed quality level rather than a
// hidden one.
//
// AUTHOR AT 'mt'. Every entry below was inserted at 'mt' and promoted only
// after the reverse pass, because a premature 'bt' is INVISIBLE to every
// automated check in this repo: check-i18n accepts it as a valid enum member
// and the zh lint's R1 only reports entries BELOW the bar. The flag is the one
// field no gate can validate.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.14.0) ──────────────────────────────────────
    // The gear is new. The hover-help entry below is the v1.12.0 "?" toggle's
    // copy, MOVED here unchanged along with the control itself — not duplicated.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this hover help, and turn the hover help on or off. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de ces infobulles et les activer ou les désactiver. Les deux choix sont conservés avec la session.',
              reviewed: true },
        'zh-Hans': { t: '设置',
                     b: '选择这些悬停帮助的语言，并开启或关闭悬停帮助。两项选择都会随会话一起保存。',
                     reviewed: 'mt' },
    },

    // v1.16.0: the body ENUMERATED its own options — "English and French are
    // available" — which the new zh-Hans <option> made false in the shipped copy
    // of BOTH languages. The enumeration is REMOVED rather than extended: naming
    // all three would put Han inside the en and fr bodies, which moves the
    // English tooltip's own geometry and drags the CJK tail onto the very
    // baseline every gate measures against. The selector already lists the
    // languages, in their endonyms — the one form a reader recognises without
    // knowing the page language — so a body that counts them duplicates the
    // control it describes and goes stale again at the next language added.
    //
    // v1.15.0: through v1.14.0 this entry told the user, in both languages,
    // that the labels on the page do not change. That is now false — they do.
    // Rewritten to say what is true, INCLUDING the half that stayed true:
    // value readouts are English in both languages (D-03), so a knob still
    // reads `20.0 kHz` either way, and preset names are English because the
    // name IS the filename (D-02).
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. Value readouts and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de ces infobulles et des libellés de la page. Les valeurs affichées et les noms de préréglages restent en anglais.',
              reviewed: true },
        'zh-Hans': { t: '语言',
                     b: '这些悬停帮助和页面标签的语言。数值读数与预设名称始终保持英文。',
                     reviewed: 'mt' },
    },

    'preset-prev': {
        en: { t: 'Previous preset',
              b: 'Step back through the preset list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Revenir en arrière dans la liste des préréglages.',
              reviewed: true },

        'zh-Hans': { t: '上一个预设',

                 b: '在预设列表中后退一步。',

                 reviewed: 'mt' },
    },

    'preset-next': {
        en: { t: 'Next preset',
              b: 'Step forward through the preset list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Avancer dans la liste des préréglages.',
              reviewed: true },

        'zh-Hans': { t: '下一个预设',

                 b: '在预设列表中前进一步。',

                 reviewed: 'mt' },
    },

    'preset-select': {
        en: { t: 'Preset',
              b: 'The preset currently loaded — click to browse all of them by category. The 28 factory presets are read-only; saving under the same name writes a user copy instead.' },
        fr: { t: 'Préréglage',
              b: 'Le préréglage actuellement chargé — cliquer pour les parcourir tous par catégorie. Les 28 préréglages d’usine sont en lecture seule ; enregistrer sous le même nom crée une copie utilisateur à la place.',
              reviewed: true },

        'zh-Hans': { t: '预设',

                 b: '当前载入的预设 —— 点击可按类别浏览全部预设。28 个出厂预设为只读；以同名保存时会改为写入一份用户副本。',

                 reviewed: 'mt' },
    },

    'preset-save': {
        en: { t: 'Save',
              b: 'Save the current settings as a user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Enregistrer les réglages actuels comme préréglage utilisateur.',
              reviewed: true },

        'zh-Hans': { t: '保存',

                 b: '将当前设置保存为用户预设。',

                 reviewed: 'mt' },
    },

    'preset-load': {
        en: { t: 'Load',
              b: 'Load a preset from a file.' },
        fr: { t: 'Charger',
              b: 'Charger un préréglage depuis un fichier.',
              reviewed: true },

        'zh-Hans': { t: '载入',

                 b: '从文件载入一个预设。',

                 reviewed: 'mt' },
    },

    'preset-delete': {
        en: { t: 'Delete',
              b: 'Delete the current user preset. Click once to arm it, again to confirm.' },
        fr: { t: 'Supprimer',
              b: 'Supprimer le préréglage utilisateur actuel. Un premier clic arme, un second confirme.',
              reviewed: true },

        'zh-Hans': { t: '删除',

                 b: '删除当前的用户预设。第一次点击进入待命，再次点击确认。',

                 reviewed: 'mt' },
    },

    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Infobulles',
              b: 'Affiche une courte description lorsque le pointeur s’arrête sur une commande. Le réglage est conservé avec la session.',
              reviewed: true },

        'zh-Hans': { t: '悬停帮助',

                 b: '当指针停在某个控件上时显示一段简短说明。该设置会随会话一起保存。',

                 reviewed: 'mt' },
    },

    'TAPE_ENABLE': {
        en: { t: 'Tape',
              b: 'Enable the tape family — stop gestures, oxide dropouts, wow and flutter, and hiss.' },
        fr: { t: 'Bande',
              b: 'Active la famille bande — arrêts de défilement, pertes d’oxyde, pleurage et scintillement, et souffle.',
              reviewed: true },

        'zh-Hans': { t: '磁带',

                 b: '启用磁带族 —— 停带动作、氧化层失落、慢抖与快抖，以及嘶声。',

                 reviewed: 'mt' },
    },

    'TAPE_PROB': {
        en: { t: 'Probability',
              b: 'How often the tape family fires an event, per clock tick. At 0 the family is silent even while enabled.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille bande déclenche un événement, par top d’horloge. À 0 la famille reste muette même si elle est activée.',
              reviewed: true },

        'zh-Hans': { t: '概率',

                 b: '磁带族每个时钟脉冲触发一次事件的频繁程度。为 0 时即使已启用，该族也保持静默。',

                 reviewed: 'mt' },
    },

    'TAPE_STOP_PROB': {
        en: { t: 'Stop share',
              b: 'The share of tape events that become a full stop-and-restart gesture rather than a dropout.' },
        fr: { t: 'Part d’arrêts',
              b: 'La part des événements de bande qui deviennent un arrêt-redémarrage complet plutôt qu’une perte de niveau.',
              reviewed: true },

        'zh-Hans': { t: '停带占比',

                 b: '磁带事件中成为完整停带再启动动作、而非一次失落的比例。',

                 reviewed: 'mt' },
    },

    'TAPE_DROP': {
        en: { t: 'Dropout share',
              b: 'The share of tape events that become an oxide dropout — a dip to 10–70% of level with a filter blended in. Real dropouts almost never mute; a full mute reads as an edit.' },
        fr: { t: 'Part de pertes',
              b: 'La part des événements de bande qui deviennent une perte d’oxyde — une chute à 10–70 % du niveau avec un filtre mêlé. Les vraies pertes ne coupent presque jamais ; une coupure franche s’entend comme un montage.',
              reviewed: true },

        'zh-Hans': { t: '失落占比',

                 b: '磁带事件中成为氧化层失落的比例 —— 电平跌至 10–70%，并混入一个滤波器。真实的失落几乎从不完全静音；彻底静音听起来像一处剪辑。',

                 reviewed: 'mt' },
    },

    'TAPE_WOW': {
        en: { t: 'Wow & flutter',
              b: 'Depth of the slow speed drift and its faster flutter. This modulates the read rate, so its slope is pitch.' },
        fr: { t: 'Pleurage et scintillement',
              b: 'Profondeur de la dérive lente de vitesse et de son scintillement plus rapide. Ceci module la vitesse de lecture : sa pente est donc une variation de hauteur.',
              reviewed: true },

        'zh-Hans': { t: '慢抖与快抖',

                 b: '慢速漂移及其较快抖动的深度。它调制的是读取速率，因此其斜率就是音高。',

                 reviewed: 'mt' },
    },

    'TAPE_HISS': {
        en: { t: 'Hiss',
              b: 'Level of the tape hiss bed. Runs whenever the family is enabled, independent of events.' },
        fr: { t: 'Souffle',
              b: 'Niveau de la nappe de souffle de bande. Il joue dès que la famille est activée, indépendamment des événements.',
              reviewed: true },

        'zh-Hans': { t: '嘶声',

                 b: '磁带嘶声底层的电平。只要该族已启用就持续存在，与事件无关。',

                 reviewed: 'mt' },
    },

    'TAPE_RAMP': {
        en: { t: 'Stop ramp',
              b: 'How long a stop gesture takes to spin down and back up again. 20–500 ms.' },
        fr: { t: 'Rampe d’arrêt',
              b: 'Le temps que met un arrêt à ralentir puis à repartir. 20–500 ms.',
              reviewed: true },

        'zh-Hans': { t: '停带斜坡',

                 b: '一次停带动作减速停止再重新起转所需的时间。20–500 ms。',

                 reviewed: 'mt' },
    },

    'CD_ENABLE': {
        en: { t: 'CD Skip',
              b: 'Enable the CD family — anti-shock loop stutters, sector-quantised buzz, and servo seeks.' },
        fr: { t: 'Saut de CD',
              b: 'Active la famille CD — bégaiements de boucle anti-choc, bourdonnement quantifié au secteur et recherches de servo.',
              reviewed: true },

        'zh-Hans': { t: 'CD 跳碟',

                 b: '启用 CD 族 —— 防震循环卡顿、按扇区量化的嗡鸣，以及伺服寻道。',

                 reviewed: 'mt' },
    },

    'CD_PROB': {
        en: { t: 'Probability',
              b: 'How often the CD family fires a skip, per clock tick.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille CD déclenche un saut, par top d’horloge.',
              reviewed: true },

        'zh-Hans': { t: '概率',

                 b: 'CD 族每个时钟脉冲触发一次跳碟的频繁程度。',

                 reviewed: 'mt' },
    },

    'CD_SEVERITY': {
        en: { t: 'Severity',
              b: 'How broken the disc is. Past the upper thresholds, loop windows quantise to the sector quantum — the 75 Hz-family buzz of a real anti-shock loop — and releases go through a servo seek instead of recovering instantly.' },
        fr: { t: 'Gravité',
              b: 'À quel point le disque est abîmé. Au-delà des seuils hauts, les fenêtres de boucle se quantifient sur le pas de secteur — le bourdonnement de la famille des 75 Hz d’une vraie boucle anti-choc — et les relâchements passent par une recherche de servo au lieu de se rétablir instantanément.',
              reviewed: true },

        'zh-Hans': { t: '程度',

                 b: '光盘损坏的程度。超过较高的阈值后，循环窗口会量化到扇区步长 —— 真实防震循环那种 75 Hz 一族的嗡鸣 —— 释放时也会经过一次伺服寻道，而不是立即恢复。',

                 reviewed: 'mt' },
    },

    'CD_SEGMENT': {
        en: { t: 'Loop length',
              b: 'Length of the segment an anti-shock stutter repeats. 10–400 ms.' },
        fr: { t: 'Longueur de boucle',
              b: 'Longueur du segment que répète un bégaiement anti-choc. 10–400 ms.',
              reviewed: true },

        'zh-Hans': { t: '循环长度',

                 b: '防震卡顿所重复片段的长度。10–400 ms。',

                 reviewed: 'mt' },
    },

    'VINYL_ENABLE': {
        en: { t: 'Vinyl',
              b: 'Enable the vinyl family — groove jumps and locked grooves, surface pops, wear and warp.' },
        fr: { t: 'Vinyle',
              b: 'Active la famille vinyle — sauts de sillon et sillons fermés, craquements de surface, usure et déformation.',
              reviewed: true },

        'zh-Hans': { t: '黑胶',

                 b: '启用黑胶族 —— 跳纹与锁纹、表面爆音、磨损与扭曲。',

                 reviewed: 'mt' },
    },

    'VINYL_PROB': {
        en: { t: 'Probability',
              b: 'How often the vinyl family fires a groove jump or a locked groove, per clock tick.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille vinyle déclenche un saut de sillon ou un sillon fermé, par top d’horloge.',
              reviewed: true },

        'zh-Hans': { t: '概率',

                 b: '黑胶族每个时钟脉冲触发一次跳纹或锁纹的频繁程度。',

                 reviewed: 'mt' },
    },

    'VINYL_RPM': {
        en: { t: 'Speed',
              b: 'Disc speed. Sets the revolution period, which both the groove-jump distance and the warp wobble are locked to.' },
        fr: { t: 'Vitesse',
              b: 'Vitesse du disque. Fixe la période de révolution, à laquelle sont asservies la distance du saut de sillon et l’ondulation du voilage.',
              reviewed: true },

        'zh-Hans': { t: '速度',

                 b: '唱片转速。它决定每转的周期，跳纹的距离与翘曲的摆动都锁定在这个周期上。',

                 reviewed: 'mt' },
    },

    'VINYL_POP': {
        en: { t: 'Pops',
              b: 'Density of surface crackle and pops. Runs whenever the family is enabled, independent of events.' },
        fr: { t: 'Clics et craquements',
              b: 'Densité des craquements et crépitements de surface. Ils jouent dès que la famille est activée, indépendamment des événements.',
              reviewed: true },

        'zh-Hans': { t: '爆音',

                 b: '表面噼啪声与爆音的密度。只要该族已启用就持续存在，与事件无关。',

                 reviewed: 'mt' },
    },

    'VINYL_WEAR': {
        en: { t: 'Wear',
              b: 'Level of the worn-groove noise bed — the dull roar under a played-out record.' },
        fr: { t: 'Usure',
              b: 'Niveau de la nappe de bruit de sillon usé — le grondement sourd sous un disque trop joué.',
              reviewed: true },

        'zh-Hans': { t: '磨损',

                 b: '磨损纹路噪声底层的电平 —— 一张放旧了的唱片下面那种沉闷的轰鸣。',

                 reviewed: 'mt' },
    },

    'VINYL_WARP': {
        en: { t: 'Warp',
              b: 'Depth of the once-per-revolution pitch wobble of a warped disc. At 100% the read rate deviates 0.6%, which is the far end of what a visibly warped LP does.' },
        fr: { t: 'Déformation',
              b: 'Profondeur de l’ondulation de hauteur d’un disque voilé, une fois par tour. À 100 %, la vitesse de lecture dévie de 0,6 %, ce qui correspond à l’extrême d’un microsillon visiblement voilé.',
              reviewed: true },

        'zh-Hans': { t: '扭曲',

                 b: '一张翘曲唱片每转一次的音高摆动深度。在 100% 时读取速率偏离 0.6%，这已是肉眼可见翘曲的密纹唱片的极限。',

                 reviewed: 'mt' },
    },

    'PACKET_ENABLE': {
        en: { t: 'Packet',
              b: 'Enable the packet family — dropped 20 ms packets in bursts, with a concealment strategy.' },
        fr: { t: 'Paquets',
              b: 'Active la famille paquets — pertes de paquets de 20 ms en rafales, avec une stratégie de dissimulation.',
              reviewed: true },

        'zh-Hans': { t: '数据包',

                 b: '启用数据包族 —— 成串丢失的 20 ms 数据包，并带有一种隐藏策略。',

                 reviewed: 'mt' },
    },

    'PACKET_LOSS': {
        en: { t: 'Loss rate',
              b: 'Share of 20 ms packets that fail to arrive.' },
        fr: { t: 'Taux de perte',
              b: 'Part des paquets de 20 ms qui n’arrivent pas.',
              reviewed: true },

        'zh-Hans': { t: '丢包率',

                 b: '未能到达的 20 ms 数据包所占的比例。',

                 reviewed: 'mt' },
    },

    'PACKET_BURST': {
        en: { t: 'Burstiness',
              b: 'How much losses clump. At 0 they are independent; higher values hold the chain in its bad state, so packets drop in runs rather than singly.' },
        fr: { t: 'Groupement en rafales',
              b: 'À quel point les pertes se groupent. À 0 elles sont indépendantes ; plus haut, la chaîne reste dans son mauvais état et les paquets tombent par séries plutôt qu’un par un.',
              reviewed: true },

        'zh-Hans': { t: '突发性',

                 b: '丢包聚集的程度。为 0 时各次丢包彼此独立；数值越高，链路越是停留在坏状态，数据包便成串丢失而不是逐个丢失。',

                 reviewed: 'mt' },
    },

    'PACKET_CONCEAL': {
        en: { t: 'Concealment',
              b: 'What the decoder does with a missing packet — go Silent, Repeat the last one, let it Decay, or Substitute new material.' },
        fr: { t: 'Dissimulation',
              b: 'Ce que fait le décodeur d’un paquet manquant — Silence, Répéter le précédent, le laisser en Déclin ou Substituer de la matière nouvelle.',
              reviewed: true },

        'zh-Hans': { t: '丢包隐藏',

                 b: '解码器如何处理一个缺失的数据包 —— 静音、重复上一个、让它衰减，或者替换为新的素材。',

                 reviewed: 'mt' },
    },

    'PACKET_COMFORT': {
        en: { t: 'Comfort noise',
              b: 'Level of the comfort noise injected under concealed packets, as a real codec does to keep the line from sounding dead.' },
        fr: { t: 'Bruit de confort',
              b: 'Niveau du bruit de confort injecté sous les paquets dissimulés, comme le fait un vrai codec pour éviter que la ligne ne semble morte.',
              reviewed: true },

        'zh-Hans': { t: '舒适噪声',

                 b: '在被隐藏的数据包下方注入的舒适噪声电平，真实的编解码器正是这样做，以免线路听起来像断了。',

                 reviewed: 'mt' },
    },

    'CODEC_ENABLE': {
        en: { t: 'Codec',
              b: 'Enable the codec family — a telephone chain: band-limit, μ-law or GSM coding, AGC and line noise.' },
        fr: { t: 'Codec',
              b: 'Active la famille codec — une chaîne téléphonique : limitation de bande, codage μ-law ou GSM, AGC et bruit de ligne.',
              reviewed: true },

        'zh-Hans': { t: '编解码器',

                 b: '启用编解码器族 —— 一条电话链路：带宽限制、μ-law 或 GSM 编码、AGC 与线路噪声。',

                 reviewed: 'mt' },
    },

    'CODEC_MODE': {
        en: { t: 'Line coding',
              b: 'μ-law companding or GSM full-rate. GSM adds frame structure, so a lost packet takes its whole frame with it.' },
        fr: { t: 'Codage de ligne',
              b: 'Compression μ-law ou GSM plein débit. Le GSM ajoute une structure de trames : un paquet perdu emporte donc toute sa trame.',
              reviewed: true },

        'zh-Hans': { t: '线路编码',

                 b: 'μ-law 压扩或 GSM 全速率。GSM 增加了帧结构，因此一个丢失的数据包会连同它所在的整帧一起丢掉。',

                 reviewed: 'mt' },
    },

    'CODEC_MIX': {
        en: { t: 'Blend',
              b: 'How much of the coded signal replaces the dry one through this stage.' },
        fr: { t: 'Mix',
              b: 'Quelle part du signal codé remplace le signal direct à travers cet étage.',
              reviewed: true },

        'zh-Hans': { t: '混融',

                 b: '经过这一级时，编码后的信号替换掉直达信号的比例。',

                 reviewed: 'mt' },
    },

    'CODEC_AGC': {
        en: { t: 'AGC',
              b: 'Depth of the fast automatic gain control after the codec — a large part of why a phone sounds like a phone. At 0 the gain is exactly unity.' },
        fr: { t: 'AGC',
              b: 'Profondeur de la commande automatique de gain (AGC) rapide après le codec — une grande part de ce qui fait qu’un téléphone sonne comme un téléphone. À 0 le gain est exactement unitaire.',
              reviewed: true },

        'zh-Hans': { t: 'AGC',

                 b: '编解码器之后那道快速自动增益控制（AGC）的深度 —— 电话之所以听起来像电话，很大程度上就在于此。为 0 时增益恰为 1。',

                 sameAsEn: true,

                 reviewed: 'mt' },
    },

    'CODEC_MAINS': {
        en: { t: 'Mains',
              b: 'Hum frequency and its harmonics in the line-noise bed. Inert while Noise is 0.' },
        fr: { t: 'Secteur',
              b: 'Fréquence du ronflement secteur et de ses harmoniques dans la nappe de bruit de ligne. Sans effet tant que Bruit est à 0.',
              reviewed: true },

        'zh-Hans': { t: '市电',

                 b: '线路噪声底层中的交流声频率及其谐波。噪声为 0 时不起作用。',

                 reviewed: 'mt',

                 termNote: 'the electrical MAINS, not a mixer’s main outputs. Same reasoning as label.mains: the glossary root 主输出 renders the OTHER sense of this English word and would be flatly wrong on a hum-frequency control.' },
    },

    'CODEC_NOISE': {
        en: { t: 'Line noise',
              b: 'Level of the line-noise bed — mains hum plus the hiss of a bad connection.' },
        fr: { t: 'Bruit de ligne',
              b: 'Niveau de la nappe de bruit de ligne — ronflement secteur et souffle d’une mauvaise connexion.',
              reviewed: true },

        'zh-Hans': { t: '线路噪声',

                 b: '线路噪声底层的电平 —— 市电交流声，加上接触不良时的嘶声。',

                 reviewed: 'mt' },
    },

    'CRUSH_ENABLE': {
        en: { t: 'Crush',
              b: 'Enable the crush family — bit-depth reduction, sample-rate decimation with jitter, and dither.' },
        fr: { t: 'Écrasement',
              b: 'Active la famille écrasement — réduction de résolution, décimation de fréquence d’échantillonnage avec gigue et dither.',
              reviewed: true },

        'zh-Hans': { t: '压碎',

                 b: '启用压碎族 —— 位深削减、带时基抖动的采样率抽取，以及抖动。',

                 reviewed: 'mt' },
    },

    'CRUSH_BITS': {
        en: { t: 'Bit depth',
              b: 'Quantisation depth, 1–16 bits. At 16 the stage is bit-transparent.' },
        fr: { t: 'Résolution',
              b: 'Profondeur de quantification, 1–16 bits. À 16 l’étage est transparent au bit près.',
              reviewed: true },

        'zh-Hans': { t: '位深',

                 b: '量化深度，1–16 位。为 16 时该级做到逐位透明。',

                 reviewed: 'mt' },
    },

    'CRUSH_RATE': {
        en: { t: 'Sample rate',
              b: 'Decimation rate — the grid the signal is re-sampled onto. 500 Hz to 20 kHz.' },
        fr: { t: 'Fréquence d’échantillonnage',
              b: 'Fréquence de décimation — la grille sur laquelle le signal est ré-échantillonné. De 500 Hz à 20 kHz.',
              reviewed: true },

        'zh-Hans': { t: '采样率',

                 b: '抽取频率 —— 信号被重新采样到的栅格。500 Hz 到 20 kHz。',

                 reviewed: 'mt' },
    },

    'CRUSH_JITTER': {
        en: { t: 'Jitter',
              b: 'Random timing error on the decimation grid, so crossings land off the clock.' },
        fr: { t: 'Gigue',
              b: 'Erreur temporelle aléatoire sur la grille de décimation, si bien que les instants d’échantillonnage tombent à côté de l’horloge.',
              reviewed: true },

        'zh-Hans': { t: '时基抖动',

                 b: '抽取栅格上的随机时间误差，使采样点落在时钟之外。',

                 reviewed: 'mt',

                 termNote: 'the same 抖动 collision as label.jitter, resolved the same way: 时基抖动 for the timing error, 抖动 kept for dither.' },
    },

    'CRUSH_ENV_AMT': {
        en: { t: 'Envelope',
              b: 'Bipolar: how much the input envelope pushes bit depth around. Positive cleans up loud passages, negative dirties them.' },
        fr: { t: 'Enveloppe',
              b: 'Bipolaire : dans quelle mesure l’enveloppe d’entrée fait varier la résolution. En positif, les passages forts se nettoient ; en négatif, ils se salissent.',
              reviewed: true },

        'zh-Hans': { t: '包络',

                 b: '双极性：输入包络牵动位深的幅度。正值让强奏段落变干净，负值让它们变脏。',

                 reviewed: 'mt' },
    },

    'CRUSH_DITHER': {
        en: { t: 'Dither',
              b: 'Noise added before quantisation, in LSBs — trades quantisation distortion for a steady noise floor.' },
        fr: { t: 'Dither',
              b: 'Bruit ajouté avant la quantification, en LSB — il échange la distorsion de quantification contre un plancher de bruit stable.',
              reviewed: true },

        'zh-Hans': { t: '抖动',

                 b: '量化之前加入的噪声，以 LSB 为单位 —— 用一层稳定的本底噪声换掉量化失真。',

                 reviewed: 'mt' },
    },

    'ROT_ENABLE': {
        en: { t: 'Rot',
              b: 'Enable the rot family — bit flips, sticky decode holds, and wrong-decode garble stretches. While off it takes no random draws at all, so a pre-1.10 session renders bit-identically.' },
        fr: { t: 'Corruption',
              b: 'Active la famille corruption — inversions de bits, blocages de décodage et plages de décodage erroné. Désactivée, elle ne tire aucun nombre aléatoire : une session antérieure à la version 1.10 rend donc un résultat identique au bit près.',
              reviewed: true },

        'zh-Hans': { t: '腐化',

                 b: '启用腐化族 —— 位翻转、解码卡死，以及错误解码造成的乱码段落。关闭时它完全不抽取随机数，因此 1.10 之前的会话渲染结果逐位一致。',

                 reviewed: 'mt' },
    },

    'ROT_PROB': {
        en: { t: 'Probability',
              b: 'How often the rot family fires an event, per clock tick.' },
        fr: { t: 'Probabilité',
              b: 'À quelle fréquence la famille corruption déclenche un événement, par top d’horloge.',
              reviewed: true },

        'zh-Hans': { t: '概率',

                 b: '腐化族每个时钟脉冲触发一次事件的频繁程度。',

                 reviewed: 'mt' },
    },

    'ROT_DEPTH': {
        en: { t: 'Flip severity',
              b: 'Sweeps the bit-flip rate from an occasional tick to a dense digital hash, and opens the reachable bit field from bit 3 up to bit 14. At most one sample in four is ever touched.' },
        fr: { t: 'Gravité d’inversion',
              b: 'Fait passer le taux d’inversion de bits d’un tic occasionnel à un hachis numérique dense et ouvre le champ de bits atteignable du bit 3 jusqu’au bit 14. Au plus un échantillon sur quatre est touché.',
              reviewed: true },

        'zh-Hans': { t: '翻转程度',

                 b: '把位翻转的速率从偶尔一声轻响一路推到密集的数字碎噪，并把可触及的位域从第 3 位一直放开到第 14 位。任何时候最多只有四分之一的采样被动过。',

                 reviewed: 'mt' },
    },

    'ROT_STICK': {
        en: { t: 'Sticky share',
              b: 'The share of rot events that become a sticky decode hold — the decoder hangs on one value.' },
        fr: { t: 'Part de blocages',
              b: 'La part des événements de corruption qui deviennent un blocage de décodage — le décodeur reste accroché à une seule valeur.',
              reviewed: true },

        'zh-Hans': { t: '卡死占比',

                 b: '腐化事件中成为解码卡死的比例 —— 解码器停在某一个数值上不动。',

                 reviewed: 'mt' },
    },

    'ROT_GARBLE': {
        en: { t: 'Garble share',
              b: 'The share of the remaining rot events that become a wrong-decode stretch. Whatever survives both shares is a bit-flip window.' },
        fr: { t: 'Part de brouillage',
              b: 'La part des événements de corruption restants qui deviennent une plage de décodage erroné. Ce qui survit aux deux parts est une fenêtre d’inversion de bits.',
              reviewed: true },

        'zh-Hans': { t: '乱码占比',

                 b: '剩余的腐化事件中成为错误解码段落的比例。两道占比之后仍然存留的，就是一段位翻转窗口。',

                 reviewed: 'mt' },
    },

    'clockModeSeg': {
        en: { t: 'Clock',
              b: 'Whether events are scheduled against the host tempo or a free-running rate.' },
        fr: { t: 'Horloge',
              b: 'Détermine si les événements sont cadencés sur le tempo de l’hôte ou sur une fréquence libre.',
              reviewed: true },

        'zh-Hans': { t: '时钟',

                 b: '决定事件是按宿主速度排程，还是按一个自由运行的频率排程。',

                 reviewed: 'mt' },
    },

    'CLOCK_SYNC_DIV': {
        en: { t: 'Division',
              b: 'Musical division the event clock ticks on, locked to host tempo.' },
        fr: { t: 'Division',
              b: 'Division musicale sur laquelle bat l’horloge d’événements, asservie au tempo de l’hôte.',
              reviewed: true },

        'zh-Hans': { t: '分割',

                 b: '事件时钟所走的音乐分割，锁定到宿主速度。',

                 reviewed: 'mt' },
    },

    'viewFree': {
        en: { t: 'Free rate',
              b: 'Free-running event clock rate, 0.1–20 Hz. Ignores host tempo.' },
        fr: { t: 'Fréquence libre',
              b: 'Fréquence de l’horloge d’événements en marche libre, 0,1–20 Hz. Ignore le tempo de l’hôte.',
              reviewed: true },

        'zh-Hans': { t: '自由频率',

                 b: '自由运行的事件时钟频率，0.1–20 Hz。忽略宿主速度。',

                 reviewed: 'mt' },
    },

    'seedRo': {
        en: { t: 'Seed',
              b: 'The seed every random stream is derived from. The same seed at the same transport position gives the same events on every render.' },
        fr: { t: 'Graine',
              b: 'La graine dont dérive chaque flux aléatoire. La même graine à la même position de transport donne les mêmes événements à chaque rendu.',
              reviewed: true },

        'zh-Hans': { t: '种子',

                 b: '每一路随机流所派生自的种子。同一个种子在同一个走带位置上，每次渲染都给出相同的事件。',

                 reviewed: 'mt' },
    },

    'diceBtn': {
        en: { t: 'Reseed',
              b: 'Draw a new seed. Everything stochastic re-rolls, so the take becomes a different one.' },
        fr: { t: 'Nouvelle graine',
              b: 'Tirer une nouvelle graine. Tout ce qui est stochastique est relancé : la prise devient donc une autre prise.',
              reviewed: true },

        'zh-Hans': { t: '重设种子',

                 b: '抽取一个新的种子。所有随机的部分都会重掷，于是这一条就变成了另一条。',

                 reviewed: 'mt' },
    },

    'edgeBtn': {
        en: { t: 'Hard edges',
              b: 'Bypass the short crossfades at event boundaries, so entries and exits become true steps. Lit means bypassed.' },
        fr: { t: 'Fronts francs',
              b: 'Contourne les courts fondus aux limites des événements, si bien que les entrées et les sorties deviennent de vraies marches. Allumé signifie contourné.',
              reviewed: true },

        'zh-Hans': { t: '硬边缘',

                 b: '旁通事件边界处的短交叉淡化，使进入与退出成为真正的阶跃。点亮表示已旁通。',

                 reviewed: 'mt' },
    },

    'MIX': {
        en: { t: 'Mix',
              b: 'Dry/wet blend of the whole processed chain.' },
        fr: { t: 'Mix',
              b: 'Équilibre direct/traité de toute la chaîne de traitement.',
              reviewed: true },

        'zh-Hans': { t: '混合',

                 b: '整条处理链的干湿混合。',

                 reviewed: 'mt' },
    },
});

// [selector, key] or [selector, key, wrapperSelector]. The selector is the
// BINDING SITE, and on this page it usually is NOT the element carrying the
// key: most controls here are an idless wrapper around a [data-param] knob, so
// the tip binds through the knob and `closest(wrapper)` walks back up to the
// cell the tip belongs on. That is exactly what the wrapper slot is for.
// ============================================================================
// LABELS — the on-page text (v1.16.0 adds zh-Hans; canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 230 px tooltip. LABELS is ON-PAGE copy: one string dropped into a fixed cell
// that does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE ─────────────────────────────────────────────────────────
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption carries ONE key. That fallback is used
// ONLY where the string is identical in BOTH languages: `lang-select`
// (Language / Langue), `help-toggle` (Hover help / Infobulles), `settings`
// for the gear and the popover's accessible name, `preset-prev` / `preset-next`
// for the two nav buttons' accessible names, `clockModeSeg` (Clock / Horloge),
// `seedRo` (Seed / Germe) and `MIX` (Mix / Dosage). None of those appears below.
//
// It is deliberately NOT used where only the English matches. #edgeBtn's tip
// title is "Hard edges"; its caption is "Hard Edges", and a shared key would
// make the next copy edit to either one a silent change to the other. The
// panel captions carry their em-dash ("— Tape"), so they are not the family
// names in I18N even where the words coincide.
//
// The knob captions are the hardest case in this plugin and NONE of them
// reuses its tip title. This page is a 3 x 2 grid of 254 px panels at 900 x 740
// and a `.ctl` column is ~76 px, so "Probabilité" (the TAPE_PROB tip title)
// does not go where "Prob" goes. The tips wrap under a 230 px cap; the captions
// have nothing to wrap into.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is what index.html carried through v1.14.0, taken from
// scripts/i18n-extract.js's inventory rather than transcribed, with HTML
// entities decoded to the characters they named (&amp; -> &) because
// textContent does not decode.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: exactly ONE
// French string per key, and nothing chooses between variants at runtime.
// Where French did not fit, the fix was this plugin's own CSS — see CHANGELOG
// v1.15.0 for the measured table.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Preset band ─────────────────────────────────────────────────────────
    // NOT `preset-save` / `preset-load` / `preset-delete`: those tip titles are
    // "Enregistrer" (76 px), "Charger" and "Supprimer", and this band's three
    // buttons sit in a header row that already carries a wordmark, a preset
    // readout and a two-line imprint. Measured, not guessed — see the CHANGELOG.
    'label.save':      { en: { t: 'Save' },   fr: { t: 'Enreg.',  reviewed: true }, 'zh-Hans': { t: '保存', reviewed: 'mt' } },
    'label.load':      { en: { t: 'Load' },   fr: { t: 'Ouvrir',  reviewed: true }, 'zh-Hans': { t: '载入', reviewed: 'mt' } },
    'label.delete':    { en: { t: 'Delete' }, fr: { t: 'Suppr.',  reviewed: true }, 'zh-Hans': { t: '删除', reviewed: 'mt' } },
    // The armed face of the delete button, and the two faces of every on/off
    // toggle on the page. These are the only strings here written from script.
    // They go through setLabel(), so the element becomes a [data-i18n] element
    // and the language sweep owns it from that moment on — through v1.14.0 they
    // were data-on / data-off / data-confirm ATTRIBUTES, which was the right
    // answer while the page was English-only and the wrong one the moment it
    // had two languages: an attribute holds ONE string, so switching to French
    // mid-session restored an English "On".
    'ui.confirm':      { en: { t: 'Confirm?' }, fr: { t: 'Confirmer ?', reviewed: true }, 'zh-Hans': { t: '确认？', reviewed: 'mt' } },
    // "Marche" / "Arrêt" rather than "Activé" / "Désactivé": the seven panel
    // buttons are 34 px, and this is the vocabulary a piece of hardware uses,
    // which is the register this whole catalogue is written in.
    'ui.on':           { en: { t: 'On' },  fr: { t: 'Marche', reviewed: true }, 'zh-Hans': { t: '开', reviewed: 'mt' } },
    'ui.off':          { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: true }, 'zh-Hans': { t: '关', reviewed: 'mt' } },

    // ── Header imprint ──────────────────────────────────────────────────────
    'label.plate':     { en: { t: 'A Catalogue of Failing Media · Plate XLVII' },
                         fr: { t: 'Catalogue des supports défaillants · Pl. XLVII', reviewed: true },
                         'zh-Hans': { t: '失效媒介图录 · 图版 XLVII',
                                      reviewed: 'mt' } },

    // ── Panel captions ──────────────────────────────────────────────────────
    // The em-dash belongs to the caption, not to the plate number beside it.
    'label.capTape':   { en: { t: '— Tape' },    fr: { t: '— Bande',       reviewed: true }, 'zh-Hans': { t: '— 磁带', reviewed: 'mt' } },
    'label.capCd':     { en: { t: '— CD Skip' }, fr: { t: '— Saut de CD',  reviewed: true }, 'zh-Hans': { t: '— CD 跳碟', reviewed: 'mt' } },
    'label.capVinyl':  { en: { t: '— Vinyl' },   fr: { t: '— Vinyle',      reviewed: true }, 'zh-Hans': { t: '— 黑胶', reviewed: 'mt' } },
    'label.capPacket': { en: { t: '— Packet' },  fr: { t: '— Paquets',     reviewed: true }, 'zh-Hans': { t: '— 数据包', reviewed: 'mt' } },
    'label.capCodec':  { en: { t: '— Codec' },   fr: { t: '— Codec',       reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '— 编解码器', reviewed: 'mt' } },
    'label.capCrush':  { en: { t: '— Crush' },   fr: { t: '— Écrasement',  reviewed: true }, 'zh-Hans': { t: '— 压碎', reviewed: 'mt' } },
    'label.capRot':    { en: { t: '— Rot' },     fr: { t: '— Corruption',  reviewed: true }, 'zh-Hans': { t: '— 腐化', reviewed: 'mt' } },
    'label.capGlobal': { en: { t: '— Global · Clock & Provenance' },
                         fr: { t: '— Global · Horloge et provenance', reviewed: true },
                         'zh-Hans': { t: '— 全局 · 时钟与来源',
                                      reviewed: 'mt' } },

    // ── Knob and control captions ───────────────────────────────────────────
    // "Prob" is already the abbreviation of "Probability" in English; "Prob."
    // is the same abbreviation in French and is what fits the same cell.
    'label.prob':      { en: { t: 'Prob' },     fr: { t: 'Prob.',      reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '概率', reviewed: 'mt' } },
    'label.stop':      { en: { t: 'Stop' },     fr: { t: 'Arrêt',      reviewed: true }, 'zh-Hans': { t: '停止', reviewed: 'mt' } },
    'label.drop':      { en: { t: 'Drop' },     fr: { t: 'Pertes',     reviewed: true }, 'zh-Hans': { t: '失落', reviewed: 'mt' } },
    'label.wow':       { en: { t: 'Wow' },      fr: { t: 'Pleurage',   reviewed: true }, 'zh-Hans': { t: '慢抖', reviewed: 'mt' } },
    'label.hiss':      { en: { t: 'Hiss' },     fr: { t: 'Souffle',    reviewed: true }, 'zh-Hans': { t: '嘶声', reviewed: 'mt' } },
    'label.ramp':      { en: { t: 'Ramp' },     fr: { t: 'Rampe',      reviewed: true }, 'zh-Hans': { t: '斜坡', reviewed: 'mt' } },
    'label.severity':  { en: { t: 'Severity' }, fr: { t: 'Gravité',    reviewed: true }, 'zh-Hans': { t: '程度', reviewed: 'mt' } },
    'label.segment':   { en: { t: 'Segment' },  fr: { t: 'Segment',    reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '片段', reviewed: 'mt' } },
    'label.speed':     { en: { t: 'Speed' },    fr: { t: 'Vitesse',    reviewed: true }, 'zh-Hans': { t: '速度', reviewed: 'mt' } },
    // The VINYL_POP tip says "Craquements", which is the right word and 11
    // characters too many for a 76 px column. "Clics" is what the same defect
    // is called in the shorter register a caption is written in.
    'label.pop':       { en: { t: 'Pop' },      fr: { t: 'Clics',      reviewed: true }, 'zh-Hans': { t: '爆音', reviewed: 'mt' } },
    'label.wear':      { en: { t: 'Wear' },     fr: { t: 'Usure',      reviewed: true }, 'zh-Hans': { t: '磨损', reviewed: 'mt' } },
    'label.warp':      { en: { t: 'Warp' },     fr: { t: 'Déform.',    reviewed: true }, 'zh-Hans': { t: '扭曲', reviewed: 'mt' } },
    'label.loss':      { en: { t: 'Loss' },     fr: { t: 'Pertes',     reviewed: true }, 'zh-Hans': { t: '丢包', reviewed: 'mt' } },
    'label.burst':     { en: { t: 'Burst' },    fr: { t: 'Rafales',    reviewed: true }, 'zh-Hans': { t: '突发', reviewed: 'mt' } },
    'label.conceal':   { en: { t: 'Conceal' },  fr: { t: 'Dissim.',    reviewed: true }, 'zh-Hans': { t: '隐藏', reviewed: 'mt' } },
    'label.comfort':   { en: { t: 'Comfort' },  fr: { t: 'Confort',    reviewed: true }, 'zh-Hans': { t: '舒适噪声', reviewed: 'mt' } },
    'label.line':      { en: { t: 'Line' },     fr: { t: 'Ligne',      reviewed: true }, 'zh-Hans': { t: '线路', reviewed: 'mt' } },
    'label.blend':     { en: { t: 'Blend' },    fr: { t: 'Mix',        reviewed: true }, 'zh-Hans': { t: '混融', reviewed: 'mt' } },
    'label.agc':       { en: { t: 'AGC' },      fr: { t: 'AGC',        reviewed: true, sameAsEn: true }, 'zh-Hans': { t: 'AGC', sameAsEn: true, reviewed: 'mt' } },
    'label.mains':     { en: { t: 'Mains' },    fr: { t: 'Secteur',    reviewed: true },
        'zh-Hans': { t: '市电',
                     reviewed: 'mt',
                     termNote: 'the electrical MAINS. This control names the 50/60 Hz hum frequency of the line-noise bed. The glossary root 主输出 is a mixer’s main OUTPUT bus — the other sense of the same English word — and would name a thing that does not exist on this page. 市电 is the power supply.' } },
    'label.noise':     { en: { t: 'Noise' },    fr: { t: 'Bruit',      reviewed: true }, 'zh-Hans': { t: '噪声', reviewed: 'mt' } },
    'label.bits':      { en: { t: 'Bits' },     fr: { t: 'Bits',       reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '位深', reviewed: 'mt' } },
    'label.rate':      { en: { t: 'Rate' },     fr: { t: 'Fréq.',      reviewed: true,
                                                     termNote: 'the control IS a frequency in Hz — CRUSH_RATE is the decimation grid, 500 Hz to 20 kHz, and its own tip title is « Fréquence d’échantillonnage ». Vitesse would rename a sample rate after a speed. Not width: Vitesse measures 46.41 px in the 64 px .ctl and would fit.' },
        'zh-Hans': { t: '速率',
                     reviewed: 'mt' } },
    'label.jitter':    { en: { t: 'Jitter' },   fr: { t: 'Gigue',      reviewed: true },
        'zh-Hans': { t: '时基抖动',
                     reviewed: 'mt',
                     termNote: 'Dither and Jitter share ONE glossary root, 抖动, and both controls sit in the SAME Crush panel two cells apart — an identical caption on two different knobs. Dither keeps the root (抖动 is the DSP process’s own Chinese name and takes no qualifier); Jitter takes the standard qualified form 时基抖动, time-base jitter, which is what Chinese audio literature calls it.' } },
    'label.env':       { en: { t: 'Env' },      fr: { t: 'Env.',       reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '包络', reviewed: 'mt' } },
    // The loanword. French audio work says "dithering"; the tip spells it out,
    // the caption keeps the four-letter form the English caption uses.
    'label.dither':    { en: { t: 'Dither' },   fr: { t: 'Dither',     reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '抖动', reviewed: 'mt' } },
    'label.depth':     { en: { t: 'Depth' },    fr: { t: 'Prof.',      reviewed: true }, 'zh-Hans': { t: '深度', reviewed: 'mt' } },
    'label.sticky':    { en: { t: 'Sticky' },   fr: { t: 'Blocages',   reviewed: true }, 'zh-Hans': { t: '卡死', reviewed: 'mt' } },
    'label.garble':    { en: { t: 'Garble' },   fr: { t: 'Brouillage', reviewed: true }, 'zh-Hans': { t: '乱码', reviewed: 'mt' } },

    // ── Choices inside two <select>s and two segmented controls ─────────────
    'label.silence':   { en: { t: 'Silence' },    fr: { t: 'Silence',    reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '静音', reviewed: 'mt' } },
    'label.repeat':    { en: { t: 'Repeat' },     fr: { t: 'Répéter',    reviewed: true }, 'zh-Hans': { t: '重复', reviewed: 'mt' } },
    'label.decay':     { en: { t: 'Decay' },      fr: { t: 'Déclin',     reviewed: true }, 'zh-Hans': { t: '衰减', reviewed: 'mt' } },
    'label.substitute':{ en: { t: 'Substitute' }, fr: { t: 'Substituer', reviewed: true }, 'zh-Hans': { t: '替换', reviewed: 'mt' } },
    'label.sync':      { en: { t: 'Sync' },       fr: { t: 'Synchro',    reviewed: true }, 'zh-Hans': { t: '同步', reviewed: 'mt' } },
    'label.free':      { en: { t: 'Free' },       fr: { t: 'Libre',      reviewed: true }, 'zh-Hans': { t: '自由', reviewed: 'mt' } },
    'label.oneBar':    { en: { t: '1 bar' },      fr: { t: '1 mes.',     reviewed: true }, 'zh-Hans': { t: '1 小节', reviewed: 'mt' } },

    // ── Global strip ────────────────────────────────────────────────────────
    'label.splices':   { en: { t: 'Splices' },    fr: { t: 'Raccords',    reviewed: true }, 'zh-Hans': { t: '剪接', reviewed: 'mt' } },
    'label.hardEdges': { en: { t: 'Hard Edges' }, fr: { t: 'Fronts francs', reviewed: true }, 'zh-Hans': { t: '硬边缘', reviewed: 'mt' } },

    // ── Annotations ─────────────────────────────────────────────────────────
    // Set in the small italic hand this catalogue uses for a marginal note.
    'label.annotRevQuantum': { en: { t: 'rev. quantum' },
                               fr: { t: 'quantum de tour', reviewed: true },
                               'zh-Hans': { t: '每转步长',
                                            reviewed: 'mt' } },
    'label.annotPackets':    { en: { t: '20 ms packets' },
                               fr: { t: 'paquets de 20 ms', reviewed: true },
                               'zh-Hans': { t: '20 ms 数据包',
                                            reviewed: 'mt' } },
    'label.annotHum':        { en: { t: 'hum + harmonics' },
                               fr: { t: 'ronflement + harmoniques', reviewed: true },
                               'zh-Hans': { t: '交流声 + 谐波',
                                            reviewed: 'mt' } },
    'label.annotSplices':    { en: { t: 'crossfades bypassed when lit' },
                               fr: { t: 'fondus contournés lorsqu’allumé', reviewed: true },
                               'zh-Hans': { t: '点亮时旁通交叉淡化',
                                            reviewed: 'mt' } },
    'label.annotRot':        { en: { t: 'bit flips · sticky decode · wrong-decode stretches' },
                               fr: { t: 'inversions de bits · décodage bloqué · plages mal décodées', reviewed: true },
                               'zh-Hans': { t: '位翻转 · 解码卡死 · 错误解码段落',
                                            reviewed: 'mt' } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. None
    // has a rendered box, so none is a geometry risk.
    'aria.presetBrowse': { en: { t: 'Browse presets' },
                           fr: { t: 'Parcourir les préréglages', reviewed: true },
                           'zh-Hans': { t: '浏览预设',
                                        reviewed: 'mt' } },
    'aria.presets':      { en: { t: 'Presets' }, fr: { t: 'Préréglages', reviewed: true }, 'zh-Hans': { t: '预设', reviewed: 'mt' } },
    // v1.15.0: this one was ALSO false copy. It read "Hover help language"
    // while the control now sets the language of the whole page.
    'aria.langSelect':   { en: { t: 'Interface language' },
                           fr: { t: 'Langue de l’interface', reviewed: true },
                           'zh-Hans': { t: '界面语言',
                                        reviewed: 'mt' } },
    'aria.helpToggle':   { en: { t: 'Toggle hover help' },
                           fr: { t: 'Activer ou désactiver les infobulles', reviewed: true },
                           'zh-Hans': { t: '开关悬停帮助',
                                        reviewed: 'mt' } },
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
    ['O-BITROT',       'the product name — a product name is never translated'],
    ['Ouaricon Audio', 'the company name'],

    // v1.16.0. The markup writes it as the numeric references
    // &#31616;&#20307;&#20013;&#25991; (index.html), matching the existing
    // Fran&ccedil;ais convention — but the parser DECODES them before the
    // uncovered-text sweep runs, so assertion 10 sees these four characters and
    // the exemption has to carry the DECODED text, not the entity spelling.
    ['简体中文', 'endonym — a language name is never translated'],

    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall: a
    // session saved against "Cassette Eject" would not resolve its French.
    // "Default" is the placeholder the manager overwrites on its first pass.
    ['Default', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // The plate numbering of the catalogue conceit. "Tab." abbreviates Table in
    // English and Tableau in French to the same three characters, a Roman
    // numeral is a Roman numeral, and the whole token is the catalogue's own
    // typographic device rather than prose — so every one of these renders
    // identically in ALL THREE languages, Chinese included. Listed individually
    // rather than as a pattern: an
    // exemption that matched "Tab. *" would silently swallow a future caption
    // that happened to start the same way.
    ['Tab. I',    'plate numbering — "Tab." + a Roman numeral reads identically in French, and is left as-is in Chinese, where it is the catalogue\'s typographic device rather than prose'],
    ['Tab. II',   'plate numbering — identical in French and in Chinese'],
    ['Tab. III',  'plate numbering — identical in French and in Chinese'],
    ['Tab. IV',   'plate numbering — identical in French and in Chinese'],
    ['Tab. V',    'plate numbering — identical in French and in Chinese'],
    ['Tab. VI',   'plate numbering — identical in French and in Chinese'],
    ['Tab. VII',  'plate numbering — identical in French and in Chinese'],
    ['Tab. VIII', 'plate numbering — identical in French and in Chinese'],

    // The two line-coding standards named on the Codec plate. A codec's name is
    // a proper noun: ITU-T G.711 μ-law and ETSI GSM 06.10 are called that in
    // every language — Chinese technical prose writes both in Latin — and the
    // tooltip for CODEC_MODE explains what they are.
    ['μ-law', 'the name of a line-coding standard (ITU-T G.711) — a standard is not translated'],
    ['GSM',   'the name of a line-coding standard (ETSI GSM 06.10) — a standard is not translated'],
];

export const TIP_BINDINGS = [
    ['#gear-btn',                        'settings'],
    ['#lang-select',                     'lang-select'],

    ['#preset-prev',                     'preset-prev'],
    ['#preset-next',                     'preset-next'],
    ['#preset-select',                   'preset-select'],
    ['#preset-save',                     'preset-save'],
    ['#preset-load',                     'preset-load'],
    ['#preset-delete',                   'preset-delete'],
    ['#help-toggle',                     'help-toggle'],
    ['[data-param="TAPE_ENABLE"]',       'TAPE_ENABLE'],
    ['[data-param="TAPE_PROB"]',         'TAPE_PROB',          '.ctl'],
    ['[data-param="TAPE_STOP_PROB"]',    'TAPE_STOP_PROB',     '.ctl'],
    ['[data-param="TAPE_DROP"]',         'TAPE_DROP',          '.ctl'],
    ['[data-param="TAPE_WOW"]',          'TAPE_WOW',           '.ctl'],
    ['[data-param="TAPE_HISS"]',         'TAPE_HISS',          '.ctl'],
    ['[data-param="TAPE_RAMP"]',         'TAPE_RAMP',          '.ctl'],
    ['[data-param="CD_ENABLE"]',         'CD_ENABLE'],
    ['[data-param="CD_PROB"]',           'CD_PROB',            '.ctl'],
    ['[data-param="CD_SEVERITY"]',       'CD_SEVERITY',        '.ctl'],
    ['[data-param="CD_SEGMENT"]',        'CD_SEGMENT',         '.ctl'],
    ['[data-param="VINYL_ENABLE"]',      'VINYL_ENABLE'],
    ['[data-param="VINYL_PROB"]',        'VINYL_PROB',         '.ctl'],
    ['[data-param="VINYL_RPM"]',         'VINYL_RPM',          '.ctl'],
    ['[data-param="VINYL_POP"]',         'VINYL_POP',          '.ctl'],
    ['[data-param="VINYL_WEAR"]',        'VINYL_WEAR',         '.ctl'],
    ['[data-param="VINYL_WARP"]',        'VINYL_WARP',         '.ctl'],
    ['[data-param="PACKET_ENABLE"]',     'PACKET_ENABLE'],
    ['[data-param="PACKET_LOSS"]',       'PACKET_LOSS',        '.ctl'],
    ['[data-param="PACKET_BURST"]',      'PACKET_BURST',       '.ctl'],
    ['[data-param="PACKET_CONCEAL"]',    'PACKET_CONCEAL',     '.ctl'],
    ['[data-param="PACKET_COMFORT"]',    'PACKET_COMFORT',     '.ctl'],
    ['[data-param="CODEC_ENABLE"]',      'CODEC_ENABLE'],
    ['[data-param="CODEC_MODE"]',        'CODEC_MODE',         '.ctl'],
    ['[data-param="CODEC_MIX"]',         'CODEC_MIX',          '.ctl'],
    ['[data-param="CODEC_AGC"]',         'CODEC_AGC',          '.ctl'],
    ['[data-param="CODEC_MAINS"]',       'CODEC_MAINS',        '.ctl'],
    ['[data-param="CODEC_NOISE"]',       'CODEC_NOISE',        '.ctl'],
    ['[data-param="CRUSH_ENABLE"]',      'CRUSH_ENABLE'],
    ['[data-param="CRUSH_BITS"]',        'CRUSH_BITS',         '.ctl'],
    ['[data-param="CRUSH_RATE"]',        'CRUSH_RATE',         '.ctl'],
    ['[data-param="CRUSH_JITTER"]',      'CRUSH_JITTER',       '.ctl'],
    ['[data-param="CRUSH_ENV_AMT"]',     'CRUSH_ENV_AMT',      '.ctl'],
    ['[data-param="CRUSH_DITHER"]',      'CRUSH_DITHER',       '.ctl'],
    ['[data-param="ROT_ENABLE"]',        'ROT_ENABLE'],
    ['[data-param="ROT_PROB"]',          'ROT_PROB',           '.g-group'],
    ['[data-param="ROT_DEPTH"]',         'ROT_DEPTH',          '.g-group'],
    ['[data-param="ROT_STICK"]',         'ROT_STICK',          '.g-group'],
    ['[data-param="ROT_GARBLE"]',        'ROT_GARBLE',         '.g-group'],
    ['#clockModeSeg',                    'clockModeSeg'],
    ['[data-param="CLOCK_SYNC_DIV"]',    'CLOCK_SYNC_DIV'],
    ['#viewFree',                        'viewFree'],
    ['#seedRo',                          'seedRo'],
    ['#diceBtn',                         'diceBtn'],
    ['#edgeBtn',                         'edgeBtn'],
    ['[data-param="MIX"]',               'MIX',                '.g-group'],
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. This plugin needs neither arm
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
