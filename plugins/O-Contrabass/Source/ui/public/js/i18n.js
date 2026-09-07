/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
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
// i18n.js — O-Contrabass interface copy, English + French (v1.8.2)
//
// ── v1.8.2: ENGLISH DEFECT FROM THE FRENCH READING (Stage O, 2026-08-31) ────
//
// Item 61: the note-expression-toggle tip TITLE read "Note expression" while
// the caption (label.noteExpression), the VST3 parameter name
// (PluginProcessor.cpp "Note Expression") and NOTES.md all say "Note
// Expression" — the feature is a proper noun in the VST3 SDK. Title
// capitalised. The French title and caption already agreed (both "Note
// Expression"), so the French is byte-untouched and reviewed: true stands.
// The en title now equals the fr title over a translated body: NO sameAsEn
// flag (Stage N correction 26 — the lint counts it as covered). No aria.*
// entry names this control (the toggle is a div with no aria-label; reported,
// not in scope). English changed: 1 entry. French changed: 0.
//
// ── v1.8.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
//
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 47 entries of 103 (17 terminology, 22 typography, 4 grammar/
// agreement, 3 meaning, 1 flag). sameAsEn: kept 9, translated 0, added 1
// (label.mix, now the glossary root "Mix" and therefore a straight copy),
// removed 1 (note-expression-toggle — its BODY is translated, so the entry is
// not a straight copy and the flag was disarming check-i18n assertion 4 for
// it). termNote exemptions: 2 (both listed by
// `node scripts/i18n-fr-lint.js --plugin O-Contrabass --strict`, which exits 0
// against a baseline of 70 findings: 33 T1, 8 T5, 8 T7, 11 G1, 10 F1).
// Left as drafted: the other 56. reviewed: false throughout — the flag means a
// NATIVE SPEAKER read it, and none has; this was a second machine reading.
//
// Decisions the next reader needs:
//
//   * THE KNOB CELL IS 62 px, not the 55 px the LABELS block claimed through
//     v1.8.0, and .knob-label truncates with an ellipsis rather than pushing a
//     neighbour. Three of this file's width claims were wrong when measured
//     (Saturation 55.00 not 66, Amortissement 74.27 not 84, the cell itself);
//     each is corrected at its entry. "Saturation" now fits and ships.
//   * Release: "Extinction" / "Chute" were two French names for one control,
//     and the glossary forbids both. The tip carries the root "Relâchement",
//     the caption the listed abbreviation "Relâch." (the root wraps at 64.95).
//   * Rate/Depth are Vitesse/Profondeur here, NOT Fréquence/Ampleur — even
//     though both LFOs are specified in Hz. O-Bowed's Fréq. exemption was for a
//     knob sitting directly under a column already captioned Vitesse; nothing
//     on this page collides that way, and the body of VIBRATO_RATE already
//     said "Vitesse" while its title said "Fréquence".
//   * "Tenue infinie" / "Tenue inf." KEPT against the glossary's Maintien, with
//     a termNote on each half. This page carries no envelope, only a Release,
//     so there is no ADSR sustain segment for Maintien to name. MEANING, not
//     width: Maint. inf. measures 50.58 px and would have fit. Converges with
//     O-Bowed v1.6.1, which reasoned the same exemption on the same control.
//   * "Corps" is KEPT for Body. O-Bowed says "Caisse" and argues it is the
//     organology term; "body" is not a glossary row, so neither is settled.
//     Reported for the glossary rather than changed on one page — two plugins
//     disagreeing is exactly what the list exists to decide.
//   * "Dosage" stays in two BODIES (INFINITE_SUSTAIN, MASTER_SAT_AMOUNT) where
//     it means an amount, and is gone from both places it was a NAME
//     (label.mix, BODY_MIX). Bodies are not matched against TERMS.
//   * Register: bodies are INFINITIVE throughout (Choisir, Reculer, Cliquer,
//     Double-cliquer, Afficher) and address the user as vous. One register per
//     plugin — keep it.
//   * U+00A0 landed 21 times: before every ';' and between every number and its
//     unit, including m/s, N, dBFS and VU, which the lint's UNITS list does not
//     all carry. The cents sign stays glued (±1200¢) because the English glues
//     it and no gate asks otherwise. Nothing outside a French string VALUE was
//     touched: both revisions were imported and compared field by field — en
//     values changed 0, key sets changed 0, TIP_BINDINGS / I18N_EXEMPT /
//     LANGUAGES byte-identical, and no U+00A0 sits outside a t:/b: value.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz), which on
// this plugin means the ENTIRE UI — O-Contrabass's controller is one inline
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
// innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below was extracted
// mechanically from index.html at v1.7.2 rather than re-typed, with its HTML
// entities decoded to the characters they named (&beta; -> β, &mdash; -> —)
// because setAttribute + textContent do not decode entities. Two deliberate
// normalisations are recorded at their entries.
//
// KEYS ARE THE PARAMETER ID where the anchor has one, and the element id
// otherwise. Five anchors have neither — the two tab buttons, the Active
// Strings block, the Fine Tuners title and the Tuning System field — and are
// addressed by their authored selector in TIP_BINDINGS with a stable key.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.8.0) ───────────────────────────────────────
    // The gear is new. The `help-toggle` entry below is the v1.7.0 "?" toggle's
    // copy, MOVED here unchanged along with the control itself — not
    // duplicated. One place for the two things that decide what the hover help
    // says and whether it says it.
    'gear-btn': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface, and turn the hover help on or off. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: "Choisir la langue de l’interface et activer ou désactiver les infobulles. Les deux choix sont conservés avec la session.",
              reviewed: true },
    
      'zh-Hans': { t: '设置', b: '选择界面语言，并开启或关闭悬停帮助。两项选择都会随会话保存。', reviewed: 'bt' }
  },

    // Written to say what is TRUE of canon v2, whatever the selector offers:
    // the labels DO change, and the halves that stay English are named rather
    // than left to be discovered — value readouts (D-03), note names and preset
    // names (D-02, the name IS the JSON filename).
    //
    // The body no longer counts or names the selector's options. A sentence
    // that enumerates what the selector holds is false the day the selector
    // grows; the exception list beside it is about what does NOT follow the
    // selector and stays true however many options there are.
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. Value readouts, note names and preset names stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de ces infobulles. Les valeurs affichées, les noms de notes et les noms de préréglages restent en anglais.",
              reviewed: true },
    
      'zh-Hans': { t: '语言', b: '本页标签与这些悬停帮助所用的语言。数值读数、音名和预设名称保持英文。', reviewed: 'bt' }
  },

    // ── Header / preset bar ─────────────────────────────────────────────────
    'preset-prev': {
        en: { t: 'Previous preset',
              b: 'Step back through the preset list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Reculer dans la liste des préréglages.',
              reviewed: true },
    
      'zh-Hans': { t: '上一个预设', b: '在预设列表中后退一项。', reviewed: 'bt' }
  },
    'preset-name': {
        en: { t: 'Preset',
              b: 'The preset currently loaded — click to browse the full list. Factory presets are read-only; saving under the same name writes a user copy.' },
        fr: { t: 'Préréglage',
              b: "Le préréglage actuellement chargé — cliquer pour parcourir la liste complète. Les préréglages d’usine sont en lecture seule ; enregistrer sous le même nom écrit une copie utilisateur.",
              reviewed: true },
    
      'zh-Hans': { t: '预设', b: '当前载入的预设 — 点击可浏览完整列表。出厂预设为只读；以同名保存会写入一份用户副本。', reviewed: 'bt' }
  },
    'preset-next': {
        en: { t: 'Next preset',
              b: 'Step forward through the preset list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Avancer dans la liste des préréglages.',
              reviewed: true },
    
      'zh-Hans': { t: '下一个预设', b: '在预设列表中前进一项。', reviewed: 'bt' }
  },
    'preset-save': {
        en: { t: 'Save',
              b: 'Save the current settings as a user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Enregistrer les réglages actuels comme préréglage utilisateur.',
              reviewed: true },
    
      'zh-Hans': { t: '保存', b: '将当前设置保存为用户预设。', reviewed: 'bt' }
  },
    'tab-main': {
        en: { t: 'Main',
              b: 'The playing surface — bow, body, strings, expression, drone and output.' },
        fr: { t: 'Principal',
              b: "La surface de jeu — archet, corps, cordes, expression, bourdon et sortie.",
              reviewed: true },
    
      'zh-Hans': { t: '主界面', b: '演奏界面 — 弓、琴体、琴弦、表情、持续音与输出。', reviewed: 'bt' }
  },
    'tab-tuning': {
        en: { t: 'Tuning',
              b: 'The full tuning panel — intervals table, scale library, generators, .scl/.kbm files.' },
        fr: { t: 'Accord',
              b: "Le panneau d’accord complet — table des intervalles, bibliothèque de gammes, générateurs, fichiers .scl/.kbm.",
              reviewed: true },
    
      'zh-Hans': { t: '调音', b: '完整的调音面板 — 音程表、音阶库、生成器、.scl/.kbm 文件。', reviewed: 'bt' }
  },
    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Infobulles',
              b: "Afficher une brève description lorsque le pointeur s’arrête sur une commande. Le réglage est conservé avec la session.",
              reviewed: true },
    
      'zh-Hans': { t: '悬停帮助', b: '指针停在控件上时显示一段简短说明。该设置会随会话保存。', reviewed: 'bt' }
  },

    // ── I · Bow ─────────────────────────────────────────────────────────────
    'BOW_SPEED': {
        en: { t: 'Bow speed',
              b: 'How fast the bow travels, 0.02–1.5 m/s. Slow bows choke and darken; fast bows loosen and sing.' },
        fr: { t: "Vitesse d’archet",
              b: "Vitesse de déplacement de l’archet, 0,02–1,5 m/s. Un archet lent étouffe et assombrit ; un archet rapide libère et fait chanter.",
              reviewed: true },
    
      'zh-Hans': { t: '弓速', b: '运弓的快慢，0.02–1.5 m/s。慢弓发闷发暗；快弓松弛而歌唱。', reviewed: 'bt' }
  },
    'BOW_PRESSURE': {
        en: { t: 'Bow pressure',
              b: 'Bow force on the string, 0.05–8 N. Too light skates into surface sound, too heavy drives into raucous scratch — the Schelleng diagram shows where you are.' },
        fr: { t: "Pression d’archet",
              b: "Force de l’archet sur la corde, 0,05–8 N. Trop légère, elle fait glisser le son en surface ; trop lourde, elle verse dans le grattement rauque — le diagramme de Schelleng indique où vous êtes.",
              reviewed: true },
    
      'zh-Hans': { t: '弓压', b: '弓对弦的压力，0.05–8 N。过轻会滑成表面音，过重会逼出刺耳的擦声 — 谢伦图显示当前所处的位置。', reviewed: 'bt' }
  },
    'BOW_POSITION': {
        en: { t: 'Bow position',
              b: 'Where the bow crosses the string, as fraction β of its length. Near the bridge (low β) is brighter but harder to speak; toward the fingerboard is warmer.' },
        fr: { t: "Position d’archet",
              b: "Point de contact de l’archet sur la corde, en fraction β de sa longueur. Près du chevalet (β faible), le son est plus brillant mais parle plus difficilement ; vers la touche, il est plus chaud.",
              reviewed: true },
    
      'zh-Hans': { t: '弓位', b: '弓与弦的接触点，以弦长的比例 β 表示。靠近琴马（β 较小）更明亮，但更难发音；靠近指板则更温暖。', reviewed: 'bt' }
  },
    'ROSIN': {
        en: { t: 'Rosin',
              b: 'Bow grip — the strength of the stick-slip friction. More rosin bites harder and speaks faster.' },
        fr: { t: 'Colophane',
              b: "Adhérence de l’archet — la force du frottement adhérence-glissement. Plus de colophane mord davantage et fait parler plus vite.",
              reviewed: true },
    
      'zh-Hans': { t: '松香', b: '弓的抓弦力 — 粘滑摩擦的强度。松香越多咬弦越紧，发音越快。', reviewed: 'bt' }
  },
    'BOW_NOISE': {
        en: { t: 'Bow noise',
              b: 'Level of the bow-hair noise bed — the breathy scratch riding under the tone.' },
        fr: { t: "Bruit d’archet",
              b: "Niveau du lit de bruit de crin — le grattement soufflé qui court sous le son.",
              reviewed: true },
    
      'zh-Hans': { t: '弓噪', b: '弓毛噪声层的电平 — 伏在音色之下的气声擦响。', reviewed: 'bt' }
  },
    'RELEASE': {
        en: { t: 'Release',
              b: 'How long the string keeps ringing once the bow lifts, 0.05–20 s.' },
        fr: { t: 'Relâchement',
              b: "Durée de résonance de la corde après le retrait de l’archet, 0,05–20 s.",
              reviewed: true },
    
      'zh-Hans': { t: '释音', b: '抬弓之后琴弦继续鸣响的时长，0.05–20 s。', reviewed: 'bt' }
  },
    'canvas-schelleng': {
        en: { t: 'Schelleng diagram',
              b: 'Where the current bow speed × pressure sits. The green band is the Helmholtz regime — the singing zone between surface sound and raucous; its wedge shifts with bow position β.' },
        fr: { t: 'Diagramme de Schelleng',
              b: "Où se situe le couple vitesse × pression actuel. La bande verte est le régime de Helmholtz — la zone chantante entre le son de surface et le rauque ; son coin se déplace avec la position d’archet β.",
              reviewed: true },
    
      'zh-Hans': { t: '谢伦图', b: '当前弓速 × 弓压所处的位置。绿色带是亥姆霍兹区 — 介于表面音与刺耳之间的歌唱区；其楔形随弓位 β 移动。', reviewed: 'bt' }
  },

    // ── II · Body ───────────────────────────────────────────────────────────
    'BODY_SIZE': {
        en: { t: 'Body size',
              b: 'Size of the modeled body resonator. Larger bodies shift the eight modes down — more chest, less bark.' },
        fr: { t: 'Taille du corps',
              b: 'Taille du résonateur de corps modélisé. Un corps plus grand abaisse les huit modes — plus de coffre, moins de mordant.',
              reviewed: true },
    
      'zh-Hans': { t: '琴体尺寸', b: '建模琴体共鸣器的尺寸。琴体越大，八个模态越往下移 — 胸腔感更足，爆响更少。', reviewed: 'bt' }
  },
    'BODY_DAMPING': {
        en: { t: 'Body damping',
              b: 'How quickly the body modes ring out. High damping dries and tightens the resonance.' },
        fr: { t: 'Amortissement du corps',
              b: "Vitesse d’extinction des modes du corps. Un amortissement élevé assèche et resserre la résonance.",
              reviewed: true },
    
      'zh-Hans': { t: '琴体阻尼', b: '琴体模态衰减的快慢。阻尼高会让共鸣变干、变紧。', reviewed: 'bt' }
  },
    'BODY_MIX': {
        en: { t: 'Body mix',
              b: 'Balance of body resonance against the raw string signal.' },
        fr: { t: 'Mix du corps',
              b: 'Équilibre entre la résonance du corps et le signal brut de la corde.',
              reviewed: true },
    
      'zh-Hans': { t: '琴体混合', b: '琴体共鸣与原始弦信号之间的平衡。', reviewed: 'bt' }
  },
    'BRIGHTNESS': {
        en: { t: 'Brightness',
              b: 'Master tone low-pass, 80 Hz–12 kHz. Below it the instrument darkens toward felt.' },
        fr: { t: 'Brillance',
              b: "Passe-bas de tonalité générale, 80 Hz–12 kHz. En dessous, l’instrument s’assombrit vers le feutre.",
              reviewed: true },
    
      'zh-Hans': { t: '明亮度', b: '总音色低通，80 Hz–12 kHz。低于此值乐器会变暗，趋向毡感。', reviewed: 'bt' }
  },
    'canvas-spectrum': {
        en: { t: 'Body response',
              b: "The resonator's frequency response — the eight modal peaks the string drives. Moves with Size, Damping and Brightness." },
        fr: { t: 'Réponse du corps',
              b: 'La réponse en fréquence du résonateur — les huit pics modaux que la corde excite. Suit Taille, Amortissement et Brillance.',
              reviewed: true },
    
      'zh-Hans': { t: '琴体响应', b: '共鸣器的频率响应 — 琴弦激励出的八个模态峰。随尺寸、阻尼和明亮度变化。', reviewed: 'bt' }
  },

    // ── III · Strings ───────────────────────────────────────────────────────
    'STRING_TENSION': {
        en: { t: 'Tension',
              b: 'String tension. Tighter strings respond faster and sound more focused; slack strings growl and flex.' },
        fr: { t: 'Tension',
              b: 'Tension des cordes. Des cordes plus tendues répondent plus vite et donnent un son plus focalisé ; des cordes lâches grondent et fléchissent.',
              reviewed: true },
    
      'zh-Hans': { t: '张力', b: '琴弦的张力。张力越高响应越快、音色越集中；松弛的弦则低吼而柔韧。', reviewed: 'bt' }
  },
    'STRING_STIFFNESS': {
        en: { t: 'Stiffness',
              b: 'String inharmonicity. Stiffer strings sharpen the upper partials — wound steel versus gut.' },
        fr: { t: 'Raideur',
              b: 'Inharmonicité des cordes. Des cordes plus raides haussent les partiels aigus — acier filé contre boyau.',
              reviewed: true },
    
      'zh-Hans': { t: '劲度', b: '琴弦的非谐性。劲度越高，高次分音越偏高 — 缠绕钢弦与羊肠弦之别。', reviewed: 'bt' }
  },
    'active-strings': {
        en: { t: 'Active strings',
              b: 'How many of the four strings (E–A–D–G, low to high) are on the instrument. Notes land on the string that can reach them.' },
        fr: { t: 'Cordes actives',
              b: "Combien des quatre cordes (E–A–D–G, de la grave à l’aiguë) sont montées sur l’instrument. Chaque note se place sur la corde capable de l’atteindre.",
              reviewed: true },
    
      'zh-Hans': { t: '活动弦', b: '四根弦（E–A–D–G，由低到高）中有多少根在乐器上。音符会落到够得到它的那根弦上。', reviewed: 'bt' }
  },
    'fine-tuners': {
        en: { t: 'Fine tuners',
              b: 'Per-string detune in cents, ±1200¢. Double-click a fader to return it to 0.' },
        fr: { t: 'Tendeurs fins',
              b: 'Désaccord par corde en cents, ±1200¢. Double-cliquer sur un curseur pour le remettre à 0.',
              reviewed: true },
    
      'zh-Hans': { t: '微调器', b: '每根弦以音分为单位的失谐量，±1200¢。双击推子可将其归零。', reviewed: 'bt' }
  },
    'DETUNE_E': {
        en: { t: 'E fine tuner',
              b: 'Detune of the low E string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin E',
              b: 'Désaccord de la corde grave E en cents, ±1200¢.',
              reviewed: true },
    
      'zh-Hans': { t: 'E 弦微调器', b: '低音 E 弦以音分为单位的失谐量，±1200¢。', reviewed: 'bt' }
  },
    'DETUNE_A': {
        en: { t: 'A fine tuner',
              b: 'Detune of the A string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin A',
              b: 'Désaccord de la corde A en cents, ±1200¢.',
              reviewed: true },
    
      'zh-Hans': { t: 'A 弦微调器', b: 'A 弦以音分为单位的失谐量，±1200¢。', reviewed: 'bt' }
  },
    'DETUNE_D': {
        en: { t: 'D fine tuner',
              b: 'Detune of the D string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin D',
              b: 'Désaccord de la corde D en cents, ±1200¢.',
              reviewed: true },
    
      'zh-Hans': { t: 'D 弦微调器', b: 'D 弦以音分为单位的失谐量，±1200¢。', reviewed: 'bt' }
  },
    'DETUNE_G': {
        en: { t: 'G fine tuner',
              b: 'Detune of the high G string in cents, ±1200¢.' },
        fr: { t: 'Tendeur fin G',
              b: "Désaccord de la corde aiguë G en cents, ±1200¢.",
              reviewed: true },
    
      'zh-Hans': { t: 'G 弦微调器', b: '高音 G 弦以音分为单位的失谐量，±1200¢。', reviewed: 'bt' }
  },

    // ── IV · Expression ─────────────────────────────────────────────────────
    'EXPRESSION_MACRO': {
        en: { t: 'Expression',
              b: 'One-knob intensity macro — leans bow speed, pressure and vibrato together from restful to fervent.' },
        fr: { t: 'Expression',
              b: "Macro d’intensité à un seul bouton — oriente ensemble vitesse d’archet, pression et vibrato, du calme à la ferveur.",
              reviewed: true },
    
      'zh-Hans': { t: '表情', b: '单旋钮的强度宏 — 让弓速、弓压和颤音一起从安静倾向炽烈。', reviewed: 'bt' }
  },
    'VIBRATO_RATE': {
        en: { t: 'Vibrato rate',
              b: 'Vibrato speed, 0.1–12 Hz.' },
        fr: { t: 'Vitesse du vibrato',
              b: 'Vitesse du vibrato, 0,1–12 Hz.',
              reviewed: true },
    
      'zh-Hans': { t: '颤音速率', b: '颤音的快慢，0.1–12 Hz。', reviewed: 'bt' }
  },
    'VIBRATO_DEPTH': {
        en: { t: 'Vibrato depth',
              b: 'Vibrato width in cents, 0–50¢. At 0 the left hand holds still.' },
        fr: { t: 'Profondeur du vibrato',
              b: 'Largeur du vibrato en cents, 0–50¢. À 0, la main gauche reste immobile.',
              reviewed: true },
    
      'zh-Hans': { t: '颤音深度', b: '颤音的幅度，以音分计，0–50¢。为 0 时左手保持不动。', reviewed: 'bt' }
  },
    'VIBRATO_ONSET': {
        en: { t: 'Vibrato onset',
              b: 'Delay before vibrato blooms after a note starts, 0–3000 ms — the straight-tone attack of a real player.' },
        fr: { t: 'Entrée du vibrato',
              b: "Délai avant l’épanouissement du vibrato après le début d’une note, 0–3000 ms — l’attaque en son droit d’un vrai instrumentiste.",
              reviewed: true },
    
      'zh-Hans': { t: '颤音起始', b: '音符开始后颤音展开前的延迟，0–3000 ms — 真实演奏者那种直音起音。', reviewed: 'bt' }
  },
    'SLOW_LFO_RATE': {
        en: { t: 'LFO rate',
              b: 'Rate of the slow bow drift, 0.05–2 Hz — a once-per-phrase wander of the bowing.' },
        fr: { t: 'Vitesse du LFO',
              b: "Vitesse de la dérive lente d’archet, 0,05–2 Hz — une errance du coup d’archet, une fois par phrase.",
              reviewed: true },
    
      'zh-Hans': { t: 'LFO 速率', b: '运弓缓慢漂移的速率，0.05–2 Hz — 每乐句一次的运弓游移。', reviewed: 'bt' }
  },
    'SLOW_LFO_DEPTH': {
        en: { t: 'LFO depth',
              b: 'Depth of the slow bow drift. At 0 the bow holds perfectly steady.' },
        fr: { t: 'Profondeur du LFO',
              b: "Profondeur de la dérive lente d’archet. À 0, l’archet reste parfaitement stable.",
              reviewed: true },
    
      'zh-Hans': { t: 'LFO 深度', b: '运弓缓慢漂移的深度。为 0 时运弓完全稳定。', reviewed: 'bt' }
  },

    // ── V · Drone ───────────────────────────────────────────────────────────
    'INFINITE_SUSTAIN': {
        en: { t: 'Infinite sustain',
              b: 'Bow-forever amount. Raise it and held notes stop decaying — the drone wakes.' },
        fr: { t: 'Tenue infinie',
              termNote: 'not the ADSR sustain segment — this page carries no envelope at all, '
                      + 'only a Release. The control stops the string losing energy so the note '
                      + 'keeps sounding, and "une tenue" is the French musical term for a held '
                      + 'note. MEANING, not width: a tip title is unconstrained, and the caption '
                      + 'half fits too (Maint. inf. 50.58 px in a 62 px cell). Converges with '
                      + 'O-Bowed v1.6.1, which reasoned the same exemption on the same control.',
              b: "Dosage de l’archet perpétuel. En le montant, les notes tenues cessent de décroître — le bourdon s’éveille.",
              reviewed: true },
    
      'zh-Hans': { t: '无限延音', b: '永续运弓的量。提高后按住的音符将不再衰减 — 持续音被唤醒。', reviewed: 'bt' }
  },
    'SUB_HARMONICS': {
        en: { t: 'Sub-harmonics',
              b: 'Level of the sub-harmonic layer beneath the played note — tectonic weight under the drone.' },
        fr: { t: 'Sous-harmoniques',
              b: 'Niveau de la couche sous-harmonique sous la note jouée — un poids tectonique sous le bourdon.',
              reviewed: true },
    
      'zh-Hans': { t: '次谐波', b: '所奏音符之下次谐波层的电平 — 持续音底下的地质般重量。', reviewed: 'bt' }
  },

    // ── VI · Output ─────────────────────────────────────────────────────────
    'OUTPUT_GAIN': {
        en: { t: 'Level',
              b: 'Output level, −60 to +12 dB.' },
        fr: { t: 'Niveau',
              b: 'Niveau de sortie, −60 à +12 dB.',
              reviewed: true },
    
      'zh-Hans': { t: '电平', b: '输出电平，−60 至 +12 dB。', reviewed: 'bt' }
  },
    'WIDTH': {
        en: { t: 'Width',
              b: 'Stereo width. 1.00× is the natural body image; 0 collapses to mono, 2× exaggerates.' },
        fr: { t: 'Largeur',
              b: "Largeur stéréo. 1,00× est l’image naturelle du corps ; 0 replie l’image en mono, 2× exagère.",
              reviewed: true },
    
      'zh-Hans': { t: '宽度', b: '立体声宽度。1.00× 是琴体的自然声像；0 会塌缩为单声道，2× 则夸张化。', reviewed: 'bt' }
  },
    'MASTER_SAT_AMOUNT': {
        en: { t: 'Saturate',
              b: 'Master saturator blend — gentle harmonic warmth on the summed output.' },
        fr: { t: 'Saturation',
              b: 'Dosage du saturateur général — une chaleur harmonique douce sur la sortie sommée.',
              reviewed: true },
    
      'zh-Hans': { t: '饱和', b: '总饱和器的混合量 — 为叠加后的输出增添柔和的谐波暖意。', reviewed: 'bt' }
  },
    'LIMITER_CEILING_DB': {
        en: { t: 'Ceiling',
              b: 'Limiter ceiling, −6 to 0 dBFS. The output never crosses this.' },
        fr: { t: 'Plafond',
              b: 'Plafond du limiteur, −6 à 0 dBFS. La sortie ne le franchit jamais.',
              reviewed: true },
    
      'zh-Hans': { t: '上限', b: '限制器上限，−6 至 0 dBFS。输出绝不会越过此值。', reviewed: 'bt' }
  },
    'canvas-vu': {
        en: { t: 'VU meter',
              b: 'Output level after the limiter, VU-ballistic. 0 VU = −18 dBFS RMS.' },
        fr: { t: 'VU-mètre',
              b: 'Niveau de sortie après le limiteur, balistique VU. 0 VU = −18 dBFS RMS.',
              reviewed: true },
    
      'zh-Hans': { t: 'VU 表', b: '限制器之后的输出电平，VU 弹道特性。0 VU = −18 dBFS RMS。', reviewed: 'bt' }
  },

    // ── VII · Microtonal ────────────────────────────────────────────────────
    'REFERENCE_PITCH': {
        en: { t: 'Reference pitch',
              b: 'Concert pitch for A4, 220–880 Hz. Everything tunes around it.' },
        fr: { t: 'Diapason',
              b: "Hauteur de référence du A4, 220–880 Hz. Tout s’accorde autour d’elle.",
              reviewed: true },
    
      'zh-Hans': { t: '基准音高', b: 'A4 的标准音高，220–880 Hz。一切都围绕它调音。', reviewed: 'bt' }
  },
    'tuning-system': {
        en: { t: 'Tuning system',
              b: 'Where the tuning comes from — a loaded Scala scale, an MTS-ESP master, or plain 12-TET.' },
        fr: { t: "Système d’accord",
              b: "D’où vient l’accord — une gamme Scala chargée, une source maître MTS-ESP ou le 12-TET simple.",
              reviewed: true },
    
      'zh-Hans': { t: '调音体系', b: '调音的来源 — 载入的 Scala 音阶、MTS-ESP 主控，或普通的 12-TET。', reviewed: 'bt' }
  },
    'scl-load-btn': {
        en: { t: 'Load .scl',
              b: 'Load a Scala tuning file (.scl) and switch the tuning system to it.' },
        fr: { t: 'Charger .scl',
              b: "Charger un fichier d’accord Scala (.scl) et y basculer le système d’accord.",
              reviewed: true },
    
      'zh-Hans': { t: '载入 .scl', b: '载入一个 Scala 调音文件（.scl），并将调音体系切换为它。', reviewed: 'bt' }
  },
    'note-expression-toggle': {
        en: { t: 'Note Expression',
              b: 'Per-note VST3 pitch (Note Expression) for hosts like Cubase and Dorico — microtonal scores play in tune.' },
        fr: { t: 'Note Expression',
              b: "Hauteur VST3 par note (Note Expression) pour les hôtes comme Cubase et Dorico — les partitions microtonales sonnent juste.",
              reviewed: true },
    
      'zh-Hans': { t: '音符表情', b: '面向 Cubase、Dorico 等宿主的逐音符 VST3 音高（音符表情）— 微分音乐谱可以准确演奏。', reviewed: 'bt' }
  },
});

// ============================================================================
// LABELS — the page's own captions, v1.8.1
//
// Separate from I18N because a tooltip entry is a {title, body} PAIR and a
// label is one string. trLabel() falls back to I18N, so a control whose tooltip
// TITLE already IS its caption carries ONE key rather than two copies of the
// same string in two tables, drifting apart.
//
// THE REUSE RULE (settled in Stage F, O-Tapestop): a label reuses a tooltip key
// ONLY where the string is identical in BOTH languages. An English-only match
// is not enough — reusing there would make every future tooltip copy edit a
// silent geometry change to a control.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The header plate line ───────────────────────────────────────────────
    // The Latin binomial is a scientific name and is identical in French; only
    // the plate numbering is translated. Rendered inside an SVG <text>, which
    // is text-anchor="middle" in a fixed viewBox, so a wider string grows
    // symmetrically about its own centre and moves no sibling.
    'label.plate': {
        en: { t: 'Physeter macrocephalus — Plate VII' },
        fr: { t: 'Physeter macrocephalus — Planche VII', reviewed: true },
    
      'zh-Hans': { t: 'Physeter macrocephalus — 图版 VII', reviewed: 'bt' }
  },

    // ── Preset bar and tab strip ────────────────────────────────────────────
    // NOT the `preset-save` tooltip key: that title is "Enregistrer", and this
    // button sits in a header band that already carries the wordmark, a preset
    // readout, two nav arrows, the tab pair, the gear and the brand line.
    // v1.8.1 — this defence was TESTED rather than inherited (Stage N found
    // five headers wrong about the string they defended). Planting the root
    // "Enregistrer" (65.73 px) grows the button 66 -> 77.61 px and MOVES 12
    // elements: the nav arrows compress 21.81 -> 20.84, #preset-name shifts
    // left 0.96, and the whole tab strip plus the gear slide right 9.69 px.
    // check-ui-labels assertion 7 would fail on it. "Enreg." (38.00 px) and
    // "Enreg" (34.27) both move nothing; the glossary lists both. The period
    // stays because this button carries no aria-label, so its accessible name
    // IS its caption and there is no label-in-name substring to close.
    'label.save':       { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: true } , 'zh-Hans': { t: '保存', reviewed: 'bt' } },
    // The tab captions DO reuse their tooltip titles: "Main"/"Principal" and
    // "Tuning"/"Accord" are identical strings in both tables, so a single key
    // is correct under the reuse rule. Declared here anyway rather than pointed
    // at I18N, because the tab strip is a fixed two-button row where a caption
    // width change is a geometry change, and a tooltip copy edit must not be
    // able to move it.
    'label.tabMain':    { en: { t: 'Main' },   fr: { t: 'Principal', reviewed: true } , 'zh-Hans': { t: '主界面', reviewed: 'bt' } },
    'label.tabTuning':  { en: { t: 'Tuning' }, fr: { t: 'Accord',    reviewed: true } , 'zh-Hans': { t: '调音', reviewed: 'bt' } },

    // ── The two faces of the hover-help toggle ──────────────────────────────
    // The ONLY strings on this page written from script. They go through
    // setLabel(), so the element becomes a [data-i18n] element and the language
    // sweep owns it from that moment on. Written as two calls behind an
    // if/else, never one call with a ternary — check-i18n assertion 13.
    //
    // "Marche"/"Arrêt" rather than "Activé"/"Désactivé": the button is 44 px,
    // and this is the vocabulary a piece of hardware uses, which is the
    // register this instrument panel is written in.
    'ui.on':            { en: { t: 'On' },  fr: { t: 'Marche', reviewed: true } , 'zh-Hans': { t: '开', reviewed: 'bt' } },
    'ui.off':           { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: true } , 'zh-Hans': { t: '关', reviewed: 'bt' } },

    // ── Panel captions (the Roman numeral is a sibling, never inside these) ──
    // The interpunct belongs to the caption, not to the numeral beside it —
    // the same shape as O-Bitrot's em-dashed plate captions.
    'label.secBow':        { en: { t: ' · Bow' },        fr: { t: ' · Archet',     reviewed: true } , 'zh-Hans': { t: ' · 弓', reviewed: 'bt' } },
    'label.secBody':       { en: { t: ' · Body' },       fr: { t: ' · Corps',      reviewed: true } , 'zh-Hans': { t: ' · 琴体', reviewed: 'bt' } },
    'label.secStrings':    { en: { t: ' · Strings' },    fr: { t: ' · Cordes',     reviewed: true } , 'zh-Hans': { t: ' · 琴弦', reviewed: 'bt' } },
    'label.secExpression': { en: { t: ' · Expression' }, fr: { t: ' · Expression', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: ' · 表情', reviewed: 'bt' } },
    'label.secDrone':      { en: { t: ' · Drone' },      fr: { t: ' · Bourdon',    reviewed: true } , 'zh-Hans': { t: ' · 持续音', reviewed: 'bt' } },
    'label.secOutput':     { en: { t: ' · Output' },     fr: { t: ' · Sortie',     reviewed: true } , 'zh-Hans': { t: ' · 输出', reviewed: 'bt' } },
    'label.secMicrotonal': { en: { t: ' · Microtonal' }, fr: { t: ' · Microtonal', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: ' · 微分音', reviewed: 'bt' } },
    'label.secSchelleng':  { en: { t: 'Schelleng Diagram' },
                             fr: { t: 'Diagramme de Schelleng', reviewed: true } ,
      'zh-Hans': { t: '谢伦图', reviewed: 'bt' }
  },

    // ── Panel sublabels, in the small italic hand ───────────────────────────
    'label.subResonator':  { en: { t: 'resonator · 8 modes' },
                             fr: { t: 'résonateur · 8 modes', reviewed: true } ,
      'zh-Hans': { t: '共鸣器 · 8 个模态', reviewed: 'bt' }
  },
    'label.subVibrato':    { en: { t: 'vibrato · bow drift' },
                             fr: { t: "vibrato · dérive d’archet", reviewed: true } ,
      'zh-Hans': { t: '颤音 · 运弓漂移', reviewed: 'bt' }
  },
    // SIZED, and recorded here with the measurement that forced it. The full
    // form "point de fonctionnement de l'archet" is 156.3 px, and this panel
    // head is 314 px carrying a 167.4 px title beside it, so the pair wrapped
    // to two lines and pushed the Schelleng canvas down 11 px. The panel is
    // already titled "Diagramme de Schelleng", so "de l'archet" was saying
    // twice what the title says once.
    'label.subOperating':  { en: { t: 'bow operating point' },
                             fr: { t: 'point de fonctionnement', reviewed: true } ,
      'zh-Hans': { t: '运弓工作点', reviewed: 'bt' }
  },
    'label.vizCaption':    { en: { t: 'speed × pressure, log–log · wedge shifts with β' },
                             fr: { t: 'vitesse × pression, log–log · le coin suit β', reviewed: true } ,
      'zh-Hans': { t: '弓速 × 弓压，对数–对数 · 楔形随 β 移动', reviewed: 'bt' }
  },

    // ── Knob captions ───────────────────────────────────────────────────────
    // v1.8.1 — RE-MEASURED. The cell is 62 px, not the 55 px this block said
    // through v1.8.0: .knob-control is width:62px and .knob-label is
    // max-width:62px / nowrap / overflow:hidden + text-overflow:ellipsis
    // (index.html:544, :570). A caption over 62 px does not push a neighbour —
    // it is ELLIPSIS-TRUNCATED in place, which check-ui-labels assertion 4
    // catches. Every French caption below was measured with
    // Range.selectNodeContents on its own node at the shipping 1000x650 frame.
    // Where the tooltip title is longer than the cell the caption gets its own
    // shorter key, per the reuse rule.
    'label.speed':      { en: { t: 'Speed' },      fr: { t: 'Vitesse',   reviewed: true } , 'zh-Hans': { t: '速度', reviewed: 'bt' } },
    'label.pressure':   { en: { t: 'Pressure' },   fr: { t: 'Pression',  reviewed: true } , 'zh-Hans': { t: '压力', reviewed: 'bt' } },
    'label.position':   { en: { t: 'Position' },   fr: { t: 'Position',  reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '位置', reviewed: 'bt' } },
    'label.rosin':      { en: { t: 'Rosin' },      fr: { t: 'Colophane', reviewed: true } , 'zh-Hans': { t: '松香', reviewed: 'bt' } },
    'label.noise':      { en: { t: 'Noise' },      fr: { t: 'Bruit',     reviewed: true } , 'zh-Hans': { t: '噪声', reviewed: 'bt' } },
    // v1.8.1 — "Chute" was the caption for a title that read "Extinction"; the
    // glossary settles Release on Relâchement, and forbids both words. The root
    // measures 64.95 px and wraps the 62 px cell, so the caption takes the
    // listed abbreviation Relâch. (36.95 px) while the tip carries the root.
    'label.release':    { en: { t: 'Release' },    fr: { t: 'Relâch.',   reviewed: true } , 'zh-Hans': { t: '释音', reviewed: 'bt' } },
    'label.size':       { en: { t: 'Size' },       fr: { t: 'Taille',    reviewed: true } , 'zh-Hans': { t: '尺寸', reviewed: 'bt' } },
    // "Amortissement" measures 74.27 px — not the 84 px this line claimed —
    // and still wraps the 62 px cell, so "Amort." (33.27 px) stands. The
    // glossary lists it. Re-measured at v1.8.1; the verdict did not change.
    'label.damping':    { en: { t: 'Damping' },    fr: { t: 'Amort.',    reviewed: true } , 'zh-Hans': { t: '阻尼', reviewed: 'bt' } },
    'label.mix':        { en: { t: 'Mix' },        fr: { t: 'Mix',       reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '混合', reviewed: 'bt' } },
    'label.brightness': { en: { t: 'Brightness' }, fr: { t: 'Brillance', reviewed: true } , 'zh-Hans': { t: '明亮度', reviewed: 'bt' } },
    'label.tension':    { en: { t: 'Tension' },    fr: { t: 'Tension',   reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '张力', reviewed: 'bt' } },
    'label.stiffness':  { en: { t: 'Stiffness' },  fr: { t: 'Raideur',   reviewed: true } , 'zh-Hans': { t: '劲度', reviewed: 'bt' } },
    'label.expression': { en: { t: 'Expression' }, fr: { t: 'Expression', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '表情', reviewed: 'bt' } },
    // v1.8.1 — the three vibrato captions and the two LFO captions now carry
    // the glossary's own abbreviations, head first the way French inverts them:
    // Vit. vibr. 41.94 px, Prof. vibr. 50.08, Entrée vibr. 59.36, Vit. LFO
    // 35.47, Prof. LFO 43.61 — all one line in the 62 px cell. The roots do not
    // all fit (Vit. vibrato 56.66 does, Prof. vibrato 64.80 wraps), and a
    // matched pair is worth more here than one root beside one abbreviation.
    'label.vibRate':    { en: { t: 'Vib Rate' },   fr: { t: 'Vit. vibr.', reviewed: true } , 'zh-Hans': { t: '颤音速率', reviewed: 'bt' } },
    'label.vibDepth':   { en: { t: 'Vib Depth' },  fr: { t: 'Prof. vibr.', reviewed: true } , 'zh-Hans': { t: '颤音深度', reviewed: 'bt' } },
    'label.vibOnset':   { en: { t: 'Vib Onset' },  fr: { t: 'Entrée vibr.', reviewed: true } , 'zh-Hans': { t: '颤音起始', reviewed: 'bt' } },
    'label.lfoRate':    { en: { t: 'LFO Rate' },   fr: { t: 'Vit. LFO',   reviewed: true } , 'zh-Hans': { t: 'LFO 速率', reviewed: 'bt' } },
    'label.lfoDepth':   { en: { t: 'LFO Depth' },  fr: { t: 'Prof. LFO',  reviewed: true } , 'zh-Hans': { t: 'LFO 深度', reviewed: 'bt' } },
    // The caption half of INFINITE_SUSTAIN’s exemption. Measured in this page’s
    // own 62 px .knob-label: Tenue inf. 48.73 px, Maint. inf. 50.58 — both fit,
    // so the exemption is MEANING, not width. Maintien inf. is 63.52 and wraps.
    'label.infSustain': { en: { t: 'Inf. Sustain' },
                          fr: { t: 'Tenue inf.', reviewed: true,
                                termNote: 'the caption half of INFINITE_SUSTAIN’s exemption — '
                                        + 'sustain here is a note that keeps sounding, not the '
                                        + 'ADSR segment; this page has no envelope' } ,
      'zh-Hans': { t: '无限延音', reviewed: 'bt' }
  },
    'label.subHarm':    { en: { t: 'Sub-Harm.' },  fr: { t: 'Sous-harm.', reviewed: true } , 'zh-Hans': { t: '次谐波', reviewed: 'bt' } },
    'label.level':      { en: { t: 'Level' },      fr: { t: 'Niveau',    reviewed: true } , 'zh-Hans': { t: '电平', reviewed: 'bt' } },
    'label.width':      { en: { t: 'Width' },      fr: { t: 'Largeur',   reviewed: true } , 'zh-Hans': { t: '宽度', reviewed: 'bt' } },
    // v1.8.1 — "Saturation" measures 55.00 px, NOT the 66 px this line claimed,
    // and fits the 62 px cell with 7 px to spare. The English caption is not
    // abbreviated either, so the French stops being: one control, one name.
    'label.saturate':   { en: { t: 'Saturate' },   fr: { t: 'Saturation', reviewed: true } , 'zh-Hans': { t: '饱和', reviewed: 'bt' } },
    'label.ceiling':    { en: { t: 'Ceiling' },    fr: { t: 'Plafond',   reviewed: true } , 'zh-Hans': { t: '上限', reviewed: 'bt' } },

    // ── Strings panel ───────────────────────────────────────────────────────
    'label.activeStrings': { en: { t: 'Active Strings' }, fr: { t: 'Cordes actives', reviewed: true } , 'zh-Hans': { t: '活动弦', reviewed: 'bt' } },
    'label.fineTuners':    { en: { t: 'Fine Tuners · cents' },
                             fr: { t: 'Tendeurs fins · cents', reviewed: true } ,
      'zh-Hans': { t: '微调器 · 音分', reviewed: 'bt' }
  },
    // The Active Strings readout. NOT a bare number with a unit symbol, so it
    // is not covered by D-03: "of" is a connective word and reads as English
    // prose on a French page. The NUMBER stays a number and is substituted in
    // as {n}; only the connective is localized. Written through setLabel() with
    // vars, so the language sweep re-renders it in place.
    'readout.activeStrings': { en: { t: '{n} of 4' }, fr: { t: '{n} sur 4', reviewed: true } , 'zh-Hans': { t: '4 根中的 {n} 根', reviewed: 'bt' } },

    // ── Drone panel caption (two spans either side of a <br>) ───────────────
    'label.silentAtRest':  { en: { t: 'silent at rest —' },
                             fr: { t: 'silencieux au repos —', reviewed: true } ,
      'zh-Hans': { t: '静止时无声 —', reviewed: 'bt' }
  },
    'label.raiseToWake':   { en: { t: 'raise to wake' },
                             fr: { t: 'monter pour éveiller', reviewed: true } ,
      'zh-Hans': { t: '提高以唤醒', reviewed: 'bt' }
  },

    // ── Microtonal strip ────────────────────────────────────────────────────
    // .strip-field-label is text-transform: uppercase, so this renders SYSTÈME
    // D’ACCORD on screen; the table keeps the English caption's mixed case,
    // which is what the lint and the accessible name read. The typographic
    // apostrophe costs 0.09 px (97.52 -> 97.61) inside the 98 px pin.
    'label.tuningSystem':  { en: { t: 'Tuning System' }, fr: { t: "Système d’accord", reviewed: true } , 'zh-Hans': { t: '调音体系', reviewed: 'bt' } },
    // RENAMED from `label.loadScl` when the shared scala-tuning-engine panel was
    // localized (module v3.1.0): the module declares label.loadScl for its own
    // "Load .SCL" button, and two different buttons cannot share one key — the
    // second definition silently wins and this caption would have become
    // "Ouvrir .SCL", losing both the ellipsis and the reviewed wording. This one
    // is O-Contrabass's own Tuning System button and keeps its own key.
    'label.loadSclFile':   { en: { t: 'Load .scl…' },    fr: { t: 'Charger .scl…',    reviewed: true } , 'zh-Hans': { t: '载入 .scl…', reviewed: 'bt' } },
    'label.noteExpression': { en: { t: 'Note Expression' },
                              fr: { t: 'Note Expression', reviewed: true, sameAsEn: true } ,
      'zh-Hans': { t: '音符表情', reviewed: 'bt' }
  },
    // "monophonic" is the only translated half; the pitch range is scientific
    // pitch notation and is exempt for the reason given in I18N_EXEMPT.
    'label.rangeCaption':  { en: { t: 'E1–G3 · monophonic' },
                             fr: { t: 'E1–G3 · monophonique', reviewed: true } ,
      'zh-Hans': { t: 'E1–G3 · 单音', reviewed: 'bt' }
  },

    // ── The Tuning tab's load-failure notice ────────────────────────────────
    // Through v1.7.2 this was an innerHTML string literal in the catch arm. It
    // is the one prose string this page writes from script that is not a
    // toggle face, and it went through untranslated on a French page.
    // v1.8.1 — the glossary's settled form, which O-Bassoon, O-Bowed, O-Reed
    // and O-Wind already ship. The copies converging is the point; the panel
    // is shared code. (The glossary's TERMS key for it ends in a period, which
    // the lint's own norm() strips before the lookup, so G1 can never reach
    // this row — reported to the orchestrator, not worked around.)
    'label.tuningLoadFailed': { en: { t: 'Tuning panel failed to load.' },
                                fr: { t: 'Échec du chargement du panneau d’accord.', reviewed: true } ,
      'zh-Hans': { t: '调音面板载入失败。', reviewed: 'bt' }
  },

    // ── Tuning panel (modules/tuning/scala-tuning-engine/js/tuning-panel.js) ──
    //
    // 37 rows for the SHARED module the CMakeLists embeds by path. The module
    // declares these keys in the markup it injects and calls window.__reapplyI18n
    // after each injection; trLabel() returns the KEY on a miss and applyLabel
    // writes it to textContent, so without these rows the tuning tab would paint
    // the literal text `label.vizCircle`. en/fr are COPIES, not an authoring pass:
    // verbatim from O-MicrotonalSampler's plugin-owned copy, which took them from
    // O-Bells v4.2.0. check-i18n scans the module file for this plugin as of the
    // same change, so a missing row fails assertion 15 rather than shipping.
    //
    // NOT keyed, and deliberately: note names, the true-keys A->B readout, the
    // numeric matrix/rotation cells and headers, tuning names, the Hz and stretch
    // value readouts, the `c` unit and the arrow glyphs. Those are data.
    'label.vizCircle':        { en: { t: 'Circle' }, fr: { t: 'Cercle', reviewed: true } , 'zh-Hans': { t: '圆周', reviewed: 'bt' } },
    'label.vizPolar':         { en: { t: 'Polar' }, fr: { t: 'Polaire', reviewed: true } , 'zh-Hans': { t: '极坐标', reviewed: 'bt' } },
    'label.vizMatrix':        { en: { t: 'Matrix' }, fr: { t: 'Matrice', reviewed: true } , 'zh-Hans': { t: '矩阵', reviewed: 'bt' } },
    'label.vizTrueKeys':      { en: { t: 'True Keys' }, fr: { t: 'Touches', reviewed: true } , 'zh-Hans': { t: '真实键位', reviewed: 'bt' } },
    'label.vizRotation':      { en: { t: 'Rotation' }, fr: { t: 'Rotation', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '旋转', reviewed: 'bt' } },
    'label.scaleIntervals':   { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: true } , 'zh-Hans': { t: '音阶音程', reviewed: 'bt' } },
    'label.tkHint':           { en: { t: 'Hold 2+ notes to see intervals' }, fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: true } , 'zh-Hans': { t: '按住 2 个以上音符可查看音程', reviewed: 'bt' } },
    'label.rotationMode':     { en: { t: 'Mode' }, fr: { t: 'Mode', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '模式', reviewed: 'bt' } },
    // The count arrives in data-i18n-vars from the panel, so no inflection logic
    // lives in the string (contract §6). The fuller 'Intervalles · notes : {n}' is
    // two lines in the 142 px interval column and pushes the whole list down; the
    // one-line 'Intervalles · {n} notes' fits but inflects wrongly at n=1, so the
    // count stays after the colon beside an invariant noun.
    'label.intervalsCount':   { en: { t: 'Intervals · notes: {n}' }, fr: { t: 'Interv. · notes : {n}', reviewed: true } , 'zh-Hans': { t: '音程 · 音符：{n}', reviewed: 'bt' } },
    'label.tonic':            { en: { t: 'Tonic' }, fr: { t: 'Tonique', reviewed: true } , 'zh-Hans': { t: '主音', reviewed: 'bt' } },
    'label.tuningLibrary':    { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque de gammes', reviewed: true } , 'zh-Hans': { t: '调音库', reviewed: 'bt' } },
    'label.catAll':           { en: { t: 'All Categories' }, fr: { t: 'Toutes catégories', reviewed: true } , 'zh-Hans': { t: '全部类别', reviewed: 'bt' } },
    'label.catHistorical':    { en: { t: 'Historical' }, fr: { t: 'Historiques', reviewed: true } , 'zh-Hans': { t: '历史音律', reviewed: 'bt' } },
    'label.catJust':          { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste', reviewed: true } , 'zh-Hans': { t: '纯律', reviewed: 'bt' } },
    'label.catEdo':           { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales', reviewed: true } , 'zh-Hans': { t: '等分', reviewed: 'bt' } },
    'label.catNonOctave':     { en: { t: 'Non-Octave' }, fr: { t: 'Non octaviantes', reviewed: true } , 'zh-Hans': { t: '非八度', reviewed: 'bt' } },
    'label.catWorld':         { en: { t: 'World' }, fr: { t: 'Du monde', reviewed: true } , 'zh-Hans': { t: '世界音律', reviewed: 'bt' } },
    'label.noteCount':        { en: { t: 'notes: {n}' }, fr: { t: 'notes : {n}', reviewed: true } , 'zh-Hans': { t: '音符：{n}', reviewed: 'bt' } },
    // A4 stays A4: it is letter pitch notation, which the C++ TuningEngine and the
    // .scl/.kbm formats also speak. Only REF is a word.
    'label.a4Ref':            { en: { t: 'A4 REF' }, fr: { t: 'RÉF. A4', reviewed: true } , 'zh-Hans': { t: 'A4 基准', reviewed: 'bt' } },
    'label.stretch':          { en: { t: 'Stretch' }, fr: { t: 'Étirement', reviewed: true } , 'zh-Hans': { t: '延展', reviewed: 'bt' } },
    'label.loadScl':          { en: { t: 'Load .SCL' }, fr: { t: 'Ouvrir .SCL', reviewed: true } , 'zh-Hans': { t: '载入 .scl', reviewed: 'bt' } },
    'label.loadKbm':          { en: { t: 'Load .KBM' }, fr: { t: 'Ouvrir .KBM', reviewed: true } , 'zh-Hans': { t: '载入 .kbm', reviewed: 'bt' } },
    'label.saveScl':          { en: { t: 'Save .SCL' }, fr: { t: 'Enreg. .SCL', reviewed: true } , 'zh-Hans': { t: '保存 .scl', reviewed: 'bt' } },
    'label.saveKbm':          { en: { t: 'Save .KBM' }, fr: { t: 'Enreg. .KBM', reviewed: true } , 'zh-Hans': { t: '保存 .kbm', reviewed: 'bt' } },
    'label.exportHtml':       { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: true } , 'zh-Hans': { t: '导出 HTML', reviewed: 'bt' } },
    'label.generateScale':    { en: { t: 'Generate Scale' }, fr: { t: 'Générer une gamme', reviewed: true } , 'zh-Hans': { t: '生成音阶', reviewed: 'bt' } },
    'label.genEdo':           { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: true } , 'zh-Hans': { t: '等分八度 (EDO)', reviewed: 'bt' } },
    'label.genHarmonic':      { en: { t: 'Harmonic Series' }, fr: { t: 'Série harmonique', reviewed: true } , 'zh-Hans': { t: '泛音列', reviewed: 'bt' } },
    'label.genRank2':         { en: { t: 'Rank-2 Temperament' }, fr: { t: 'Tempérament de rang 2', reviewed: true } , 'zh-Hans': { t: '二阶音律', reviewed: 'bt' } },
    'label.genDivisions':     { en: { t: 'Divisions' }, fr: { t: 'Divisions', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '等分数', reviewed: 'bt',
                     termNote: 'the glossary root for `divisions` is the GENERIC 分割 (cutting a thing apart), and this field is not that. It is the COUNT of equal divisions of the period, and it sits two cells from label.catEdo, whose English "Equal Divisions" takes the glossary root 等分. Shipping 分割 here would put two renderings of one concept in one generator panel. 等分数 is 等分 plus the count morpheme, so the two cells read as one vocabulary' } },
    'label.genPeriod':        { en: { t: 'Period (c)' }, fr: { t: 'Période (c)', reviewed: true } , 'zh-Hans': { t: '周期 (C)', reviewed: 'bt' } },
    'label.genStartHarmonic': { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: true } , 'zh-Hans': { t: '起始泛音', reviewed: 'bt' } },
    'label.genEndHarmonic':   { en: { t: 'End Harmonic' }, fr: { t: 'Harm. de fin', reviewed: true } , 'zh-Hans': { t: '终止泛音', reviewed: 'bt' } },
    'label.genGenerator':     { en: { t: 'Generator (c)' }, fr: { t: 'Génér. (c)', reviewed: true } , 'zh-Hans': { t: '生成元 (C)', reviewed: 'bt' } },
    // The SECOND 'Period (c)': one <label> in the EDO row, one in the Rank-2 row.
    // Each is its own element and so needs its own key.
    'label.genR2Period':      { en: { t: 'Period (c)' }, fr: { t: 'Période (c)', reviewed: true } , 'zh-Hans': { t: '周期 (C)', reviewed: 'bt' } },
    'label.genNotes':         { en: { t: 'Notes' }, fr: { t: 'Notes', reviewed: true, sameAsEn: true } , 'zh-Hans': { t: '音符', reviewed: 'bt' } },
    'label.generate':         { en: { t: 'Generate' }, fr: { t: 'Générer', reviewed: true } , 'zh-Hans': { t: '生成', reviewed: 'bt' } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English
    // name is the same failure as a French page with an English caption. None
    // has a rendered box, so none is a geometry risk.
    'aria.presetPrev':  { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: true } , 'zh-Hans': { t: '上一个预设', reviewed: 'bt' } },
    'aria.presetNext':  { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: true } , 'zh-Hans': { t: '下一个预设', reviewed: 'bt' } },
    'aria.helpToggle':  { en: { t: 'Toggle hover help' },
                          fr: { t: "Activer ou désactiver les infobulles", reviewed: true } ,
      'zh-Hans': { t: '开关悬停帮助', reviewed: 'bt' }
  },
    'aria.langSelect':  { en: { t: 'Interface language' },
                          fr: { t: "Langue de l’interface", reviewed: true } ,
      'zh-Hans': { t: '界面语言', reviewed: 'bt' }
  },
    'aria.settings':    { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: true } , 'zh-Hans': { t: '设置', reviewed: 'bt' } },
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
    ['O-Contrabass',   'the product name — a product name is never translated'],
    ['Contrabass',     'the second half of the wordmark, split around a styled hyphen span'],
    ['Ouaricon · Naturalist Series', 'the company name and the product-line name'],

    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall: a
    // session saved against "Arco Foundation" would not resolve its French.
    ['Default — Arco Foundation',
     'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // The Roman numerals of the seven panels. A Roman numeral is a Roman
    // numeral. Listed individually rather than as a pattern: an exemption
    // matching /^[IVX]+$/ would silently swallow a future caption that happened
    // to be spelled the same way.
    ['I',   'panel numbering — a Roman numeral is identical in French'],
    ['II',  'panel numbering — identical in French'],
    ['III', 'panel numbering — identical in French'],
    ['IV',  'panel numbering — identical in French'],
    ['V',   'panel numbering — identical in French'],
    ['VI',  'panel numbering — identical in French'],
    ['VII', 'panel numbering — identical in French'],

    // The four open strings, and the two range captions that name them. This
    // instrument's strings are addressed as E-A-D-G everywhere it matters: the
    // four DETUNE_* parameter IDs, the four data-string attributes, the DAW's
    // own note display and every score the plugin will be played from. French
    // solfege (Mi-La-Re-Sol) would break that correspondence in the one place a
    // player checks it against the host. Scientific pitch notation for the
    // range is the same decision.
    ['E', 'open-string name — the four strings are E-A-D-G in the parameter IDs, in the host and on the page'],
    ['A', 'open-string name — see E'],
    ['D', 'open-string name — see E'],
    ['G', 'open-string name — see E'],
    ['A4', 'scientific pitch notation for the reference pitch — see E'],
    ['E1–G3', 'the playable range in scientific pitch notation — see E'],

    // The bowing direction, printed in the small italic hand under the Bow
    // panel. An Italian performance term, used unchanged in French scores.
    ['arco', 'an Italian performance direction — used unchanged in French'],

    // The three tuning sources. A file format, a protocol and a notation.
    ['Scala',   'the name of a tuning file format — a format name is not translated'],
    ['MTS-ESP', 'the name of a tuning protocol (ODDSound MTS-ESP)'],
    ['12-TET',  'the standard notation for twelve-tone equal temperament'],

    // The endonyms in the language selector. A language name is never
    // translated: a French speaker looking for their language looks for
    // "Français", and a Chinese reader looks for 简体中文.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
    ['简体中文',   'endonym — a language name is never translated'],

    // The face of the gear and the tick marks of the VU scale are glyphs and
    // numbers, not prose.
    ['⚙', 'the gear glyph'],
    ['◀', 'the previous-preset glyph'],
    ['▶', 'the next-preset glyph'],

    // ── The shared tuning module (scala-tuning-engine, localized at v3.1.0) ──
    ['12-TET Standard',
     'the tuning name the C++ TuningEngine reports through getTuningName(); it is written into .scl files and the exported HTML, so it is data, not copy — D-02. The static literal in the panel template is the same datum, seeding the node until loadInitialState() overwrites it',
     '#scale-name-display'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds on every row here: unlike
// O-Bitrot, this page authors its data-tip on the .knob-control / .finetuner
// cell itself rather than on the knob inside it, so no closest(wrapper) walk is
// needed anywhere.
//
// Five anchors carry neither an id nor a data-param — the two tab buttons, the
// Active Strings block, the Fine Tuners title and the Tuning System field. Each
// is addressed by a selector that matches exactly one element on the page.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                            'gear-btn'],
    ['#lang-select',                         'lang-select'],
    ['#help-toggle',                         'help-toggle'],

    ['#preset-prev',                         'preset-prev'],
    ['#preset-name',                         'preset-name'],
    ['#preset-next',                         'preset-next'],
    ['#preset-save',                         'preset-save'],
    ['[data-tab="main"]',                    'tab-main'],
    ['[data-tab="tuning"]',                  'tab-tuning'],

    ['[data-param="BOW_SPEED"]',             'BOW_SPEED'],
    ['[data-param="BOW_PRESSURE"]',          'BOW_PRESSURE'],
    ['[data-param="BOW_POSITION"]',          'BOW_POSITION'],
    ['[data-param="ROSIN"]',                 'ROSIN'],
    ['[data-param="BOW_NOISE"]',             'BOW_NOISE'],
    ['[data-param="RELEASE"]',               'RELEASE'],
    ['#canvas-schelleng',                    'canvas-schelleng'],

    ['[data-param="BODY_SIZE"]',             'BODY_SIZE'],
    ['[data-param="BODY_DAMPING"]',          'BODY_DAMPING'],
    ['[data-param="BODY_MIX"]',              'BODY_MIX'],
    ['[data-param="BRIGHTNESS"]',            'BRIGHTNESS'],
    ['#canvas-spectrum',                     'canvas-spectrum'],

    ['[data-param="STRING_TENSION"]',        'STRING_TENSION'],
    ['[data-param="STRING_STIFFNESS"]',      'STRING_STIFFNESS'],
    ['.stepper-block',                       'active-strings'],
    ['.finetuner-title',                     'fine-tuners'],
    ['[data-param="DETUNE_E"]',              'DETUNE_E'],
    ['[data-param="DETUNE_A"]',              'DETUNE_A'],
    ['[data-param="DETUNE_D"]',              'DETUNE_D'],
    ['[data-param="DETUNE_G"]',              'DETUNE_G'],

    ['[data-param="EXPRESSION_MACRO"]',      'EXPRESSION_MACRO'],
    ['[data-param="VIBRATO_RATE"]',          'VIBRATO_RATE'],
    ['[data-param="VIBRATO_DEPTH"]',         'VIBRATO_DEPTH'],
    ['[data-param="VIBRATO_ONSET"]',         'VIBRATO_ONSET'],
    ['[data-param="SLOW_LFO_RATE"]',         'SLOW_LFO_RATE'],
    ['[data-param="SLOW_LFO_DEPTH"]',        'SLOW_LFO_DEPTH'],

    ['[data-param="INFINITE_SUSTAIN"]',      'INFINITE_SUSTAIN'],
    ['[data-param="SUB_HARMONICS"]',         'SUB_HARMONICS'],

    ['[data-param="OUTPUT_GAIN"]',           'OUTPUT_GAIN'],
    ['[data-param="WIDTH"]',                 'WIDTH'],
    ['[data-param="MASTER_SAT_AMOUNT"]',     'MASTER_SAT_AMOUNT'],
    ['[data-param="LIMITER_CEILING_DB"]',    'LIMITER_CEILING_DB'],
    ['#canvas-vu',                           'canvas-vu'],

    ['[data-param="REFERENCE_PITCH"]',       'REFERENCE_PITCH'],
    ['.strip-field',                         'tuning-system'],
    ['#scl-load-btn',                        'scl-load-btn'],
    ['#note-expression-toggle',              'note-expression-toggle'],
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
