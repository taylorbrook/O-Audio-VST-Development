/*
   This file is part of O-Polystutter, an Ouaricon Audio plugin.
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
// i18n.js — O-Polystutter UI copy, English + French (v1.14.2, canon v2)
//
// ── v1.14.2: ENGLISH FOLLOWS THE CODE (Stage O, item 34, 2026-08-31) ────────
// The `midi` body claimed "Notes C1-B1 trigger lanes 1-4, any other note
// triggers all enabled lanes". TriggerRouter.cpp:76-85 routes notes 60-63 to
// lanes 1-4, note 67 to every enabled lane, and drops everything else. Both
// bodies now name those notes. Octave convention: the plugin's own documents
// and source comments write middle C as C3 (= note 60, the JUCE keyboard
// component's octaveNumForMiddleC default), so the body says C3, C#3, D3, D#3
// and G3 — and carries the note NUMBERS in brackets, because a host that
// displays C4 for note 60 would otherwise disagree with it. Note names stay
// English in French (Stage N's rule). The French entry is `reviewed: false`
// again: its meaning changed.
// Rendered on #midi_toggle (1000 x 690 frame, tip max-width 220 px, placed
// above the anchor): en 71.17 -> 86.56 px, fr 86.56 -> 101.95 px; both still
// inside the frame (fr bottom edge 612.00, top 510.05). The routing itself is
// unchanged. Source comment TriggerRouter.cpp:69-74 corrected in the same
// commit (it labelled 61-63 as D3/E3/F3).
//
// ── v1.14.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 15 entries (6 terminology, 8 typography, 0 grammar/agreement,
// 0 meaning, 1 register). sameAsEn: kept 18, translated 0. termNote
// exemptions: 2 (listed). Left as drafted: the rest. reviewed: false
// throughout — no native speaker yet. Lint 15 findings → 0, --strict exit 0.
//
// The decisions the next reader needs:
//
//   ROLLOFF stays COUPURE, and it is the file's only glossary exemption.
//   The glossary settles rolloff → Pente, and Pente is a dB/octave SLOPE.
//   This control is not one: TapeDegrader.cpp:344-361 sweeps a lowpass CORNER
//   from 20 kHz to 2 kHz at a fixed 12 dB/oct Butterworth (Q = 0.707), so the
//   slope is the one thing the knob cannot move. Both the caption and the tip
//   title carry the termNote. Width was not the reason — PENTE measures
//   32.00 px against COUPURE's 47.61 in a 75 px column and would have fitted.
//
//   DRY -> DIRECT, not SEC. The glossary's Wet/Dry pair is Traité/Direct and
//   the tip title already read "Signal direct", so the caption was the odd one
//   out. DIRECT is 37.31 px on one line in the 75 px .tape-knob-container,
//   against SEC's 18.91 — measured, not assumed.
//
//   ON/OFF -> Activé/Désactivé, not Marche/Arrêt. Hover help is a FEATURE, not
//   a power state, which is the glossary's own split. The faces are 47.33 px
//   and 61.38 px border-box against .settings-toggle's 42 px min-width, so the
//   button grows — but the row is right-anchored inside a popover pinned at
//   min-width: 190 px (its widest French row is 165.15 px), so nothing moves:
//   check-ui-labels [7] still reports 0 non-label elements displaced.
//
//   "Hover help" -> the glossary root, which at v1.14.3 became "Infobulles":
//   49.61 px in a white-space: nowrap .settings-label, RE-MEASURED rather than
//   scaled from the caption it replaces (that caption is recorded in the
//   CHANGELOG and deliberately not repeated here, so a repo grep for it stays
//   at zero — it rendered 71.77). The French caption is now 4.58 px NARROWER
//   than English "Hover help" (54.19), where it used to be 17.58 px wider, and
//   the French hover-help row sums to 49.61 + 61.38 toggle = 110.99 in the
//   168 px content box. Nothing moves either way: the popover is pinned at
//   min-width 190 px. "Aide" alone had named a different thing from the
//   aria-label on the same row, which is why the root was applied at all.
//
//   PING stays PING (sameAsEn) and the tip title moved to it, not the other
//   way round. The caption is the English abbreviation and the title said
//   "Va-et-vient", so the visible name was not contained in the accessible
//   name (WCAG 2.5.3) in French while it was in English. Ping-pong is the term
//   French DAWs ship, so the title yields; no caption was invented.
//
//   MIDI note names stay ENGLISH — "les notes C1 à B1", not "do1 à si1".
//   The glossary keeps note names English because that is what the user's host
//   displays. NOTE: the English sentence this mirrors is itself WRONG about the
//   DSP (TriggerRouter.cpp:76-85 routes notes 60-63, and only note 67 triggers
//   all lanes; every other note does nothing). Stage N does not change English
//   copy, so the French mirrors the English deliberately and the defect is
//   reported for a later stage. FIXED in v1.14.2 (Stage O, item 34) — see the
//   block above; the sentence now reads "C3, C#3, D3 et D#3 (notes 60 à 63)".
//
//   DÉGRADATION BANDE stays telegraphic. "DÉGRADATION DE BANDE" measures
//   166.89 px against the current 144.97 and the header's own 10 px of
//   clearance before BYPASS, so the preposition does not fit.
//
//   Typography: U+2019 was already universal here (0 T1 findings at baseline).
//   This pass added U+00A0 before ; and ?, before %, and between 16 and "pas",
//   and U+2212 for the two negative-number ranges. The composed delete-confirm
//   string keeps its {name} token byte-identical: tr() still substitutes it.
//
//   NO COMMITTED RENDER GATE. plugins/O-Polystutter/tests/ holds only
//   i18n-states.json. The hover surface was driven from a scratchpad probe:
//   105 anchors × 2 languages, every tip inside the 1000 x 690 frame, every
//   tip's text different between languages, with the four lanes, the sequencer
//   and the four Euclidean groups driven ON through their own controls (81 of
//   105 anchors are pointer-unreachable at rest) and a negative control that
//   fires on a planted English-equal French body. Stage N authored no new
//   committed gate; the gap is reported.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces that.
//
// SERVED ROOT IS Source/ui/public, read from CMakeLists.txt before a byte was
// written here — scripts/serve-ui.js resolves the same root from the same
// juce_add_binary_data SOURCES block (uiRootFrom: cmake). This file has to be
// listed in that block, reached by a getResource() branch and imported by
// js/app.js, all in the same commit, or the page 404s at runtime and presents
// as a dead panel with no other symptom (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTooltip() in app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing the
// opening angle bracket. A line break, if one is ever needed, is \n plus CSS
// white-space: pre-line, never a markup tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. v1.13.0 authored its hover help as a
// SINGLE data-tooltip string in the shape "Label: sentence.", read by a second
// tooltip renderer this version deletes. Every entry below is that string split
// on its FIRST ": " into the t/b pair the measure-then-pin renderer wants, with
// both halves byte-identical to v1.13.0 either side of the separator.
//
// TWO tips did not split cleanly and were HAND-SPLIT — they were authored with
// no colon at all, so their titles are the only two new English strings in the
// tooltip half of this table. They are named in the commit message rather than
// left to be discovered:
//     'lane'     -> title "Stutter Lane"    (body unchanged)
//     'progress' -> title "Repeat Progress" (body unchanged)
//
// THREE NEW CONTROLS carry new English copy: `settings`, `lang-select` and
// `tips-toggle`. The first two are the gear popover and the language selector,
// which did not exist before; the third is the hover-help toggle, which did
// exist as a floating "?" over the bottom-right corner and had only a native
// title=. Authoring hover-help prose for controls that have none is Stage M's
// job and is NOT done here — these three are the controls this version itself
// adds or moves.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native speaker
// has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title (the small-caps line), b = tooltip body.
//
// Object.freeze() rather than a bare `{...}` literal for two reasons. It says
// out loud that this module is inert data nothing may mutate at runtime — and
// it keeps the export a SINGLE top-level statement, because a statement written
// `export const X = {...};` closes its brace at depth zero and segments the
// trailing `;` off on its own.
export const I18N = Object.freeze({

    // ── The settings popover (v1.14.0) ──────────────────────────────────────
    // New controls, new copy. The hover-help toggle moves in here from the
    // floating "?" that sat over the frame's bottom-right corner: one place for
    // the two things that decide what the hover help says and whether it says
    // it at all.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this interface and whether hover help appears. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette interface et l’affichage des infobulles. Les deux choix sont conservés avec la session.',
              reviewed: true },
        'zh-Hans': { t: '设置',
                     b: '选择本界面的语言，以及是否显示悬停帮助。两项选择都会随会话一起保存。',
                     reviewed: 'bt' },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. Value readouts, note divisions and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de ces infobulles et des libellés de la page. Les valeurs affichées, les divisions rythmiques et les noms de préréglages restent en anglais.',
              reviewed: true },
        'zh-Hans': { t: '语言',
                     b: '这些悬停帮助和页面标签所用的语言。数值读数、节奏划分和预设名称保持英文。',
                     reviewed: 'bt' },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
        'zh-Hans': { t: '悬停帮助',
                     b: '开启或关闭这些悬停帮助。关闭之后，只有齿轮按钮和这个开关仍会说明自己。',
                     reviewed: 'bt' },
    },

    // ── Preset bar ──────────────────────────────────────────────────────────
    'preset-prev': {
        en: { t: 'Previous',
              b: 'Navigate to the previous preset in the list.' },
        fr: { t: 'Précédent',
              b: 'Aller au préréglage précédent dans la liste.',
              reviewed: true },
        'zh-Hans': { t: '上一个',
                     b: '切换到列表中的上一个预设。',
                     reviewed: 'bt' },
    },
    'preset-dropdown': {
        en: { t: 'Preset',
              b: 'Click to browse and select presets. Factory presets are protected; user presets can be deleted.' },
        fr: { t: 'Préréglage',
              b: 'Cliquer pour parcourir et choisir un préréglage. Les préréglages d’usine sont protégés ; ceux de l’utilisateur peuvent être supprimés.',
              reviewed: true },
        'zh-Hans': { t: '预设',
                     b: '点击可浏览并选择预设。出厂预设受保护；用户预设可以删除。',
                     reviewed: 'bt' },
    },
    'preset-next': {
        en: { t: 'Next',
              b: 'Navigate to the next preset in the list.' },
        fr: { t: 'Suivant',
              b: 'Aller au préréglage suivant dans la liste.',
              reviewed: true },
        'zh-Hans': { t: '下一个',
                     b: '切换到列表中的下一个预设。',
                     reviewed: 'bt' },
    },
    'preset-save': {
        en: { t: 'Save',
              b: 'Save current settings as a new user preset.' },
        fr: { t: 'Enregistrer',
              b: 'Enregistrer les réglages actuels comme nouveau préréglage utilisateur.',
              reviewed: true },
        'zh-Hans': { t: '保存',
                     b: '把当前设置保存为新的用户预设。',
                     reviewed: 'bt' },
    },
    'preset-load': {
        en: { t: 'Load',
              b: 'Import a preset from an external file.' },
        fr: { t: 'Ouvrir',
              b: 'Importer un préréglage depuis un fichier externe.',
              reviewed: true },
        'zh-Hans': { t: '载入',
                     b: '从外部文件导入预设。',
                     reviewed: 'bt' },
    },

    // ── The four stutter lanes ──────────────────────────────────────────────
    // THE FIRST HAND-SPLIT. v1.13.0 authored this as a single sentence with no
    // ": " to split on, so the title is new English and the body is the whole
    // original string, unchanged.
    'lane': {
        en: { t: 'Stutter Lane',
              b: 'Enable or disable this stutter lane. When off, this lane won\'t process audio.' },
        fr: { t: 'Piste de bégaiement',
              b: 'Activer ou désactiver cette piste de bégaiement. Désactivée, elle ne traite aucun signal.',
              reviewed: true },
        'zh-Hans': { t: '断续轨',
                     b: '启用或停用这条断续轨。停用后，本轨不处理任何音频。',
                     reviewed: 'bt' },
    },
    // THE SECOND HAND-SPLIT, for the same reason.
    'progress': {
        en: { t: 'Repeat Progress',
              b: 'Shows playback position within the current repeat cycle.' },
        fr: { t: 'Progression de la répétition',
              b: 'Indique la position de lecture dans le cycle de répétition en cours.',
              reviewed: true },
        'zh-Hans': { t: '重复进度',
                     b: '显示当前重复周期内的播放位置。',
                     reviewed: 'bt' },
    },
    'subdiv': {
        en: { t: 'Subdivision',
              b: 'Sets the rhythmic length of each stutter slice, synced to host tempo.' },
        fr: { t: 'Subdivision',
              b: 'Fixe la durée rythmique de chaque tranche de bégaiement, synchronisée au tempo de l’hôte.',
              reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '节奏划分',
                     b: '设定每一片断续切片的节奏长度，与宿主速度同步。',
                     reviewed: 'bt' },
    },
    'repeats': {
        en: { t: 'Repeats',
              b: 'Number of times the captured audio slice plays back (1-16).' },
        fr: { t: 'Répétitions',
              b: 'Nombre de lectures de la tranche audio capturée (1-16).',
              reviewed: true },
        'zh-Hans': { t: '重复',
                     b: '捕获的音频切片播放的次数（1 到 16）。',
                     reviewed: 'bt' },
    },
    'decay': {
        en: { t: 'Decay',
              b: 'Volume reduction applied to each successive repeat. Lower values create fading echoes.' },
        fr: { t: 'Déclin',
              b: 'Baisse de volume appliquée à chaque répétition successive. Des valeurs basses créent des échos qui s’effacent.',
              reviewed: true },
        'zh-Hans': { t: '衰减',
                     b: '施加在每一次后续重复上的音量衰减。数值越低，回声消失得越快。',
                     reviewed: 'bt' },
    },
    'filter': {
        en: { t: 'Filter',
              b: 'Applies low-pass (negative) or high-pass (positive) filtering to repeats.' },
        fr: { t: 'Filtre',
              b: 'Applique aux répétitions un filtrage passe-bas (valeur négative) ou passe-haut (valeur positive).',
              reviewed: true },
        'zh-Hans': { t: '滤波器',
                     b: '对重复施加低通（负值）或高通（正值）滤波。',
                     reviewed: 'bt' },
    },
    'probability': {
        en: { t: 'Probability',
              b: 'Chance that this lane triggers on each beat. 100% = always, 50% = half the time.' },
        fr: { t: 'Probabilité',
              b: 'Chance que cette piste se déclenche à chaque temps. 100 % = toujours, 50 % = une fois sur deux.',
              reviewed: true },
        'zh-Hans': { t: '概率',
                     b: '本轨在每一拍上触发的机会。100% 为总是触发，50% 为一半的时候触发。',
                     reviewed: 'bt' },
    },
    'volume': {
        en: { t: 'Volume',
              b: 'Output level of this lane\'s stutters in the mix.' },
        fr: { t: 'Volume',
              b: 'Niveau de sortie des bégaiements de cette piste dans le mixage.',
              reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '音量',
                     b: '本轨的断续在混音中的输出电平。',
                     reviewed: 'bt' },
    },
    'pan': {
        en: { t: 'Pan',
              b: 'Stereo position of the stutters. -100 = full left, +100 = full right.' },
        fr: { t: 'Panoramique',
              b: 'Position stéréo des bégaiements. −100 = tout à gauche, +100 = tout à droite.',
              reviewed: true },
        'zh-Hans': { t: '声像',
                     b: '断续的立体声位置。−100 为全左，+100 为全右。',
                     reviewed: 'bt' },
    },
    'swing': {
        en: { t: 'Swing',
              b: 'Adds rhythmic shuffle to repeat timing. Higher values create more groove.' },
        fr: { t: 'Swing',
              b: 'Décale le rythme des répétitions. Des valeurs élevées accentuent le groove.',
              reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '摇摆',
                     b: '为重复的时值加入摇摆律动。数值越高，律动感越强。',
                     reviewed: 'bt' },
    },
    'pitch': {
        en: { t: 'Pitch',
              b: 'Transpose the stutters up or down in semitones (-12 to +12).' },
        fr: { t: 'Hauteur',
              b: 'Transposer les bégaiements vers le haut ou vers le bas, en demi-tons (−12 à +12).',
              reviewed: true },
        'zh-Hans': { t: '音高',
                     b: '把断续按半音向上或向下移调（−12 到 +12）。',
                     reviewed: 'bt' },
    },
    'pitch-rand': {
        en: { t: 'Random',
              b: 'Enable random pitch variation on each repeat within the MIN/MAX range.' },
        fr: { t: 'Aléatoire',
              b: 'Activer une variation de hauteur aléatoire à chaque répétition, dans la plage MIN/MAX.',
              reviewed: true },
        'zh-Hans': { t: '随机',
                     b: '启用后，每一次重复的音高都会在最小与最大之间随机变化。',
                     reviewed: 'bt' },
    },
    'pitch-rand-min': {
        en: { t: 'Min',
              b: 'Minimum pitch offset for randomization (added to base pitch).' },
        fr: { t: 'Min',
              b: 'Écart de hauteur minimal pour le tirage aléatoire (ajouté à la hauteur de base).',
              reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '最小',
                     b: '随机化的最小音高偏移量（叠加在基础音高上）。',
                     reviewed: 'bt' },
    },
    'pitch-rand-max': {
        en: { t: 'Max',
              b: 'Maximum pitch offset for randomization (added to base pitch).' },
        fr: { t: 'Max',
              b: 'Écart de hauteur maximal pour le tirage aléatoire (ajouté à la hauteur de base).',
              reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '最大',
                     b: '随机化的最大音高偏移量（叠加在基础音高上）。',
                     reviewed: 'bt' },
    },
    'pitch-quantize': {
        en: { t: 'Semitone',
              b: 'When ON, random pitch snaps to whole semitones. When OFF, allows microtonal intervals.' },
        fr: { t: 'Demi-ton',
              b: 'Activé, le tirage de hauteur se cale sur des demi-tons entiers. Désactivé, il autorise des intervalles microtonaux.',
              reviewed: true },
        'zh-Hans': { t: '半音',
                     b: '开启时，随机音高对齐到整半音。关闭时，允许微分音程。',
                     reviewed: 'bt' },
    },
    'pingpong': {
        en: { t: 'Ping-Pong',
              b: 'Alternates playback direction between forward and reverse on each repeat.' },
        fr: { t: 'Ping-pong',
              b: 'Alterne le sens de lecture entre avant et arrière à chaque répétition.',
              reviewed: true },
        'zh-Hans': { t: '乒乓',
                     b: '每一次重复交替一次播放方向，在正放与倒放之间轮换。',
                     reviewed: 'bt' },
    },
    'reverse': {
        en: { t: 'Reverse',
              b: 'Plays all repeats backwards.' },
        fr: { t: 'Inversion',
              b: 'Lit toutes les répétitions à l’envers.',
              reviewed: true },
        'zh-Hans': { t: '反向',
                     b: '把所有重复都倒着播放。',
                     reviewed: 'bt' },
    },
    'manual': {
        en: { t: 'Manual',
              b: 'Ignores beat-sync timing. Use with TRIG button for one-shot stutters.' },
        fr: { t: 'Manuel',
              b: 'Ignore la synchronisation au temps. À utiliser avec le bouton DÉCL pour des bégaiements ponctuels.',
              reviewed: true },
        'zh-Hans': { t: '手动',
                     b: '忽略按拍同步的时值。与触发按钮配合使用，可做单次断续。',
                     reviewed: 'bt' },
    },

    // ── The pattern sequencer ───────────────────────────────────────────────
    'seq': {
        en: { t: 'Sequencer',
              b: 'When ON, the 16-step pattern controls when each lane can trigger. When OFF, lanes trigger on every beat.' },
        fr: { t: 'Séquenceur',
              b: 'Activé, le motif de 16 pas décide quand chaque piste peut se déclencher. Désactivé, les pistes se déclenchent à chaque temps.',
              reviewed: true },
        'zh-Hans': { t: '音序器',
                     b: '开启时，16 步的模式决定每条轨可以在什么时候触发。关闭时，各轨在每一拍上触发。',
                     reviewed: 'bt' },
    },
    'sequencer': {
        en: { t: 'Pattern Sequencer',
              b: 'Click steps to enable (green) or disable (dim). Each row controls one lane. Active steps allow stutter triggers on that beat.' },
        fr: { t: 'Séquenceur de motif',
              b: 'Cliquer sur les pas pour les activer (vert) ou les désactiver (atténué). Chaque ligne pilote une piste. Les pas actifs autorisent un déclenchement sur ce temps.',
              reviewed: true },
        'zh-Hans': { t: '模式音序器',
                     b: '点击步位可以启用（绿色）或停用（暗色）。每一行控制一条轨。启用的步位允许在该拍上触发断续。',
                     reviewed: 'bt' },
    },
    'euclidean': {
        en: { t: 'Euclidean',
              b: 'Replace manual step pattern with auto-generated Euclidean rhythm.' },
        fr: { t: 'Euclidien',
              b: 'Remplace le motif de pas manuel par un rythme euclidien généré automatiquement.',
              reviewed: true },
        'zh-Hans': { t: '欧几里得',
                     b: '用自动生成的欧几里得节奏取代手动的步进模式。',
                     reviewed: 'bt' },
    },
    'euc-pulses': {
        en: { t: 'Pulses',
              b: 'Number of active hits (1-16).' },
        fr: { t: 'Impulsions',
              b: 'Nombre de frappes actives (1-16).',
              reviewed: true },
        'zh-Hans': { t: '脉冲',
                     b: '有效击点的数量（1 到 16）。',
                     reviewed: 'bt' },
    },
    'euc-steps': {
        en: { t: 'Steps',
              b: 'Total steps in pattern (2-16).' },
        fr: { t: 'Pas',
              b: 'Nombre total de pas dans le motif (2-16).',
              reviewed: true },
        'zh-Hans': { t: '步',
                     b: '模式中的总步数（2 到 16）。',
                     reviewed: 'bt' },
    },

    // ── Tape degradation, dry/wet and the trigger toggles ───────────────────
    'tape-bypass': {
        en: { t: 'Bypass',
              b: 'Skip all tape degradation processing for a clean signal comparison.' },
        fr: { t: 'Contournement',
              b: 'Ignorer tout le traitement de dégradation de bande pour comparer avec le signal propre.',
              reviewed: true },
        'zh-Hans': { t: '旁通',
                     b: '跳过全部磁带劣化处理，以便与干净信号作对比。',
                     reviewed: 'bt' },
    },
    'saturation': {
        en: { t: 'Saturation',
              b: 'Adds warm tape-style harmonic distortion. Higher values = more grit.' },
        fr: { t: 'Saturation',
              b: 'Ajoute une distorsion harmonique chaude, à la manière d’une bande. Plus la valeur est haute, plus le grain est rugueux.',
              reviewed: true, sameAsEn: true },
        'zh-Hans': { t: '饱和',
                     b: '加入温暖的磁带式谐波失真。数值越高，颗粒感越粗。',
                     reviewed: 'bt' },
    },
    'wow': {
        en: { t: 'Wow',
              b: 'Slow pitch modulation that simulates tape speed variations. Creates a warbly, lo-fi feel.' },
        fr: { t: 'Pleurage',
              b: 'Modulation lente de la hauteur qui imite les variations de vitesse d’une bande. Donne un caractère ondulant et lo-fi.',
              reviewed: true },
        'zh-Hans': { t: '慢抖',
                     b: '缓慢的音高调制，模仿磁带速度的起伏。带来摇曳的低保真质感。',
                     reviewed: 'bt' },
    },
    'flutter': {
        en: { t: 'Flutter',
              b: 'Fast pitch modulation for tape-like wobble. Adds instability and character.' },
        fr: { t: 'Scintillement',
              b: 'Modulation rapide de la hauteur, pour un tremblement de bande. Ajoute de l’instabilité et du caractère.',
              reviewed: true },
        'zh-Hans': { t: '快抖',
                     b: '快速的音高调制，做出磁带式的颤动。增加不稳定感与个性。',
                     reviewed: 'bt' },
    },
    'hiss': {
        en: { t: 'Hiss',
              b: 'Adds subtle tape noise for vintage character. Keep low for realistic results.' },
        fr: { t: 'Souffle',
              b: 'Ajoute un léger bruit de bande pour un caractère vintage. À garder discret pour rester réaliste.',
              reviewed: true },
        'zh-Hans': { t: '嘶声',
                     b: '加入轻微的磁带噪声，营造复古质感。保持低量才显得真实。',
                     reviewed: 'bt' },
    },
    'rolloff': {
        en: { t: 'Rolloff',
              b: 'High-frequency attenuation mimicking tape head wear. Higher = darker, more muffled.' },
        fr: { t: 'Coupure',
              b: 'Atténuation des aigus imitant l’usure des têtes de lecture. Plus la valeur est haute, plus le son est sombre et étouffé.',
              reviewed: true,
              termNote: 'the control is a LOWPASS CUTOFF, not a slope: TapeDegrader.cpp:347 sweeps the corner 20 kHz → 2 kHz at a FIXED 12 dB/oct Butterworth, so the glossary root Pente (a dB/octave slope) would describe something this knob cannot move' },
        'zh-Hans': { t: '滚降',
                     b: '模仿磁头磨损的高频衰减。数值越高，声音越暗、越闷。',
                     reviewed: 'bt' },
    },
    'dropout': {
        en: { t: 'Dropout',
              b: 'Random momentary signal dropouts simulating worn tape. Use sparingly for subtle effect.' },
        fr: { t: 'Chutes de signal',
              b: 'Coupures brèves et aléatoires du signal, comme sur une bande usée. À doser avec parcimonie pour rester subtil.',
              reviewed: true },
        'zh-Hans': { t: '信号跌落',
                     b: '随机的瞬时信号中断，模拟磨损的磁带。少量使用才能保持细腻。',
                     reviewed: 'bt' },
    },
    'dry': {
        en: { t: 'Dry',
              b: 'Level of the original unprocessed signal. Set to 0% for 100% wet stutters only.' },
        fr: { t: 'Signal direct',
              b: 'Niveau du signal d’origine, non traité. Régler sur 0 % pour n’entendre que les bégaiements.',
              reviewed: true },
        'zh-Hans': { t: '干',
                     b: '未经处理的原始信号的电平。设为 0% 时只听到断续。',
                     reviewed: 'bt' },
    },
    'wet': {
        en: { t: 'Wet',
              b: 'Level of the stutter effect. Controls how loud the repeats are in the mix.' },
        fr: { t: 'Signal traité',
              b: 'Niveau de l’effet de bégaiement. Règle le volume des répétitions dans le mixage.',
              reviewed: true },
        'zh-Hans': { t: '湿',
                     b: '断续效果的电平。控制重复在混音中的响度。',
                     reviewed: 'bt' },
    },
    'midi': {
        en: { t: 'MIDI',
              b: 'Enable MIDI note triggering. C3, C#3, D3 and D#3 (notes 60–63) trigger lanes 1–4; G3 (note 67) triggers all enabled lanes. Any other note is ignored.' },
        fr: { t: 'MIDI',
              b: 'Activer le déclenchement par notes MIDI. C3, C#3, D3 et D#3 (notes 60 à 63) déclenchent les pistes 1 à 4 ; G3 (note 67) déclenche toutes les pistes actives. Toute autre note est ignorée.',
              reviewed: true, sameAsEn: true },
        'zh-Hans': { t: 'MIDI',
                     b: '启用 MIDI 音符触发。C3、C#3、D3 和 D#3（音符 60 到 63）分别触发第 1 到第 4 轨；G3（音符 67）触发所有已启用的轨。其他音符一律忽略。',
                     sameAsEn: true,
                     reviewed: 'bt' },
    },
    'trig': {
        en: { t: 'Trigger',
              b: 'Manually trigger all enabled lanes instantly. Use with MAN mode for one-shot effects.' },
        fr: { t: 'Déclenchement',
              b: 'Déclencher manuellement et immédiatement toutes les pistes actives. À utiliser avec le mode MAN pour des effets ponctuels.',
              reviewed: true },
        'zh-Hans': { t: '触发',
                     b: '立即手动触发所有已启用的轨。与手动模式配合，可做单次效果。',
                     reviewed: 'bt' },
    },

    // ── A sentence shape with NO tooltip and NO element (carried item 10) ────
    //
    // The delete-preset confirmation is prose the user reads, but it is neither
    // a tooltip nor an element's text: it is the argument to confirm(), composed
    // per preset with a {name} token. It has no canon home. LABELS is the wrong
    // one — assertion 15 requires every LABELS key to be referenced by an
    // element or a setLabel() call, and this one never can be, so housing it
    // there would report as a dead key forever. I18N takes it with an EMPTY
    // BODY, which is the only shape that satisfies assertions 13 and 15 at the
    // same time, exactly as the Stage-I summary predicted this plugin would need.
    //
    // Authored AROUND the inflection per contract §6: no count, no plural, and
    // the preset name substituted rather than concatenated, so the sentence
    // reads correctly in both languages whatever the name is.
    'msg-delete-preset': {
        en: { t: 'Delete preset {name}?', b: '' },
        fr: { t: 'Supprimer le préréglage {name} ?', b: '', reviewed: true },
        'zh-Hans': { t: '删除预设 {name}？',
                     b: '',
                     reviewed: 'bt' },
    },
});

// ============================================================================
// LABELS — the on-page text (v1.14.2, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 220 px tooltip. LABELS is ON-PAGE copy: one string dropped into a cell that
// mostly does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE, AND WHY IT BUYS NOTHING HERE ───────────────────────────
// trLabel() falls back to I18N when a key is absent here, so a control whose
// tooltip TITLE already IS its caption can carry ONE key. NOT ONE CONTROL ON
// THIS PAGE QUALIFIES: every caption is an ABBREVIATION cut to a 42 px cell
// (REPS, FILTR, PROB, SAT) while every tooltip title is the whole word
// (Repeats, Filter, Probability, Saturation). Reusing there would put
// "Probability" into a 42 px column. So the two tables are disjoint by
// measurement, not by oversight.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is what index.html carried through v1.13.0, taken from
// scripts/i18n-extract.js's inventory rather than transcribed.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: exactly ONE
// French string per key, and nothing chooses between variants at runtime. This
// is a 1000 x 690 frame, but its captions live in 42 px knob columns, 40 px
// toggle chips and a 36 px sequencer column header, so every French caption
// below was MEASURED AS RENDERED inside its own element — text-transform:
// uppercase and letter-spacing are not in getComputedStyle().font, so a font
// probe reads short and the pin lands under the French.
//
// The one that did not fit is recorded rather than hidden: PITCH -> HAUTEUR is
// 48.6 px in a 42 px column, so the caption is HAUT. (30.6 px) and the word
// "Hauteur" is carried in full by the tooltip title above.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Preset bar ──────────────────────────────────────────────────────────
    // .preset-action-btn is text-transform: uppercase, so these render SAVE and
    // LOAD; the authored case is v1.13.0's and is left alone.
    'label.save':      { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: true }, 'zh-Hans': { t: '保存', reviewed: 'bt' } },
    'label.load':      { en: { t: 'Load' }, fr: { t: 'Ouvrir', reviewed: true }, 'zh-Hans': { t: '载入', reviewed: 'bt' } },
    // Written by setLabel() from buildPresetDropdown(), which through v1.13.0
    // built each row with innerHTML and a markup fragment. It is createElement +
    // setLabel now: assertion 12 reports a raw prose write inside a template
    // string, and no I18N_EXEMPT entry could cover it, because an exemption
    // lives in this file where assertion 9 forbids the opening angle bracket.
    'label.factory':   { en: { t: 'Factory' }, fr: { t: 'Usine', reviewed: true }, 'zh-Hans': { t: '出厂', reviewed: 'bt' } },

    // ── The settings popover ────────────────────────────────────────────────
    'label.language':  { en: { t: 'Language' },   fr: { t: 'Langue', reviewed: true }, 'zh-Hans': { t: '语言', reviewed: 'bt' } },
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true }, 'zh-Hans': { t: '悬停帮助', reviewed: 'bt' } },  // 49.61
    // The two faces of the hover-help switch. KEYS through setLabel(), not
    // literals: a literal holds one string, so switching to French mid-session
    // would restore an English "On". Written from an if/else with two literal
    // keys, never a ternary inside the call — check-i18n assertion 13.
    'ui.on':           { en: { t: 'On' },  fr: { t: 'Activé',    reviewed: true }, 'zh-Hans': { t: '开启', reviewed: 'bt' } },   // button 47.33
    'ui.off':          { en: { t: 'Off' }, fr: { t: 'Désactivé', reviewed: true }, 'zh-Hans': { t: '关闭', reviewed: 'bt' } },   // button 61.38

    // ── The four lane headers ───────────────────────────────────────────────
    // ONE key with a {n} token, not four keys differing by a digit. The number
    // is markup data (data-i18n-vars), so the four headers cannot drift apart
    // and a digit can never be translated. trLabel()'s var resolver looks the
    // VALUE up as a key first and falls through to the literal, which is what
    // makes "1" arrive as "1".
    'label.lane':      { en: { t: 'LANE {n}' }, fr: { t: 'PISTE {n}', reviewed: true }, 'zh-Hans': { t: '轨道 {n}', reviewed: 'bt' } },

    // ── Lane knob captions — 42 px columns, 9 px uppercase ──────────────────
    // Rendered widths in parentheses; the column is 42 px wide, pinned by the
    // 42 px knob and the min-width: 42px value readout under it.
    'label.subdiv':    { en: { t: 'SUBDIV' }, fr: { t: 'SUBDIV', reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '划分', reviewed: 'bt' } },  // 38.3
    'label.reps':      { en: { t: 'REPS' },   fr: { t: 'RÉPÉT',  reviewed: true }, 'zh-Hans': { t: '重复', reviewed: 'bt' } },                  // 31.5
    'label.decay':     { en: { t: 'DECAY' },  fr: { t: 'DÉCLIN', reviewed: true }, 'zh-Hans': { t: '衰减', reviewed: 'bt' } },                  // 37.8
    'label.filtr':     { en: { t: 'FILTR' },  fr: { t: 'FILTRE', reviewed: true }, 'zh-Hans': { t: '滤波', reviewed: 'bt' } },                  // 34.5
    'label.prob':      { en: { t: 'PROB' },   fr: { t: 'PROBA',  reviewed: true }, 'zh-Hans': { t: '概率', reviewed: 'bt' } },                  // 34.0
    'label.vol':       { en: { t: 'VOL' },    fr: { t: 'VOL',    reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '音量', reviewed: 'bt' } },
    'label.pan':       { en: { t: 'PAN' },    fr: { t: 'PAN',    reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '声像', reviewed: 'bt' } },
    'label.swing':     { en: { t: 'SWING' },  fr: { t: 'SWING',  reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '摇摆', reviewed: 'bt' } },
    // HAUT., not HAUTEUR: the full word measures 48.6 px in a 42 px column.
    'label.pitch':     { en: { t: 'PITCH' },  fr: { t: 'HAUT.',  reviewed: true }, 'zh-Hans': { t: '音高', reviewed: 'bt' } },                  // 30.6
    // The mini-knob column is 40 px, set by .mini-knob; both fit at 7 px.
    'label.min':       { en: { t: 'MIN' },    fr: { t: 'MIN',    reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '最小', reviewed: 'bt' } },
    'label.max':       { en: { t: 'MAX' },    fr: { t: 'MAX',    reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '最大', reviewed: 'bt' } },

    // ── Lane toggle chips — 40 px and 32 px, 8 px type ──────────────────────
    'label.rnd':       { en: { t: 'RND' },  fr: { t: 'ALÉA', reviewed: true }, 'zh-Hans': { t: '随机', reviewed: 'bt' } },   // 23.3 in a 32 px chip
    'label.st':        { en: { t: 'ST' },   fr: { t: 'DT',   reviewed: true }, 'zh-Hans': { t: '半音', reviewed: 'bt' } },   // demi-ton
    'label.ping':      { en: { t: 'PING' }, fr: { t: 'PING', reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '乒乓', reviewed: 'bt', termNote: 'PING here abbreviates the PING-PONG toggle on the same control (TIP_BINDINGS binds #laneN_pingpong to the title Ping-Pong), so the rendering is the Ping-Pong root. The glossary root for the bare key Ping is an EXCITATION impulse and its only corpus site is O-Octagon label.group.ping, a loudspeaker measurement ping — a different thing from a stereo bounce' } },
    'label.rev':       { en: { t: 'REV' },  fr: { t: 'INV',  reviewed: true }, 'zh-Hans': { t: '反向', reviewed: 'bt' } },
    'label.man':       { en: { t: 'MAN' },  fr: { t: 'MAN',  reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '手动', reviewed: 'bt' } },

    // ── Sequencer ───────────────────────────────────────────────────────────
    'label.euc':       { en: { t: 'EUC' },    fr: { t: 'EUC',   reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '欧几里得', reviewed: 'bt' } },
    // 36 px column headers. IMPUL, not IMPULSIONS and not IMPULS: the six-letter
    // form measures 34.1 px against a 36 px box and leaves 1.9 px, which is
    // inside the Windows/WebView2 font-metric margin this repo cannot measure.
    'label.pulses':    { en: { t: 'PULSES' }, fr: { t: 'IMPUL', reviewed: true }, 'zh-Hans': { t: '脉冲', reviewed: 'bt' } },  // 29.2
    'label.steps':     { en: { t: 'STEPS' },  fr: { t: 'PAS',   reviewed: true }, 'zh-Hans': { t: '步', reviewed: 'bt' } },  // 16.0
    'label.seq':       { en: { t: 'SEQ' },    fr: { t: 'SÉQ',   reviewed: true }, 'zh-Hans': { t: '音序', reviewed: 'bt' } },

    // ── Tape section — the 75 px .tape-knob-container columns ───────────────
    'label.tapeDegradation': { en: { t: 'TAPE DEGRADATION' },
                               fr: { t: 'DÉGRADATION BANDE', reviewed: true },
                               'zh-Hans': { t: '磁带劣化',
                     reviewed: 'bt' },
                           },     // 145.0, ends 10 px clear of BYPASS
    'label.bypass':    { en: { t: 'BYPASS' },  fr: { t: 'CONTOUR', reviewed: true }, 'zh-Hans': { t: '旁通', reviewed: 'bt' } },  // 53.8 in a 60 px chip
    'label.sat':       { en: { t: 'SAT' },     fr: { t: 'SAT',     reviewed: true, sameAsEn: true }, 'zh-Hans': { t: '饱和', reviewed: 'bt' } },
    // Pleurage and scintillement are the French audio terms for wow and flutter;
    // neither is a transliteration of the English.
    'label.wow':       { en: { t: 'WOW' },     fr: { t: 'PLEUR',   reviewed: true }, 'zh-Hans': { t: '慢抖', reviewed: 'bt' } },  // 32.5
    'label.flutter':   { en: { t: 'FLUTTER' }, fr: { t: 'SCINT.',  reviewed: true }, 'zh-Hans': { t: '快抖', reviewed: 'bt' } },  // 32.4
    'label.hiss':      { en: { t: 'HISS' },    fr: { t: 'SOUFFLE', reviewed: true }, 'zh-Hans': { t: '嘶声', reviewed: 'bt' } },  // 44.6
    'label.rolloff':   { en: { t: 'ROLLOFF' },
                         fr: { t: 'COUPURE', reviewed: true,
                               termNote: 'the control is a LOWPASS CUTOFF, not a slope: TapeDegrader.cpp:347 sweeps the corner 20 kHz → 2 kHz at a FIXED 12 dB/oct Butterworth, so the glossary root Pente (a dB/octave slope) would describe something this knob cannot move' },
                         'zh-Hans': { t: '滚降',
                     reviewed: 'bt' },
                     },                            // 47.6
    'label.dropout':   { en: { t: 'DROPOUT' }, fr: { t: 'CHUTES',  reviewed: true }, 'zh-Hans': { t: '信号跌落', reviewed: 'bt' } },  // 39.8
    'label.dry':       { en: { t: 'DRY' },     fr: { t: 'DIRECT',  reviewed: true }, 'zh-Hans': { t: '干', reviewed: 'bt' } },  // 37.3
    'label.wet':       { en: { t: 'WET' },     fr: { t: 'TRAITÉ',  reviewed: true }, 'zh-Hans': { t: '湿', reviewed: 'bt' } },  // 36.8
    'label.midi':      { en: { t: 'MIDI' },    fr: { t: 'MIDI',    reviewed: true, sameAsEn: true }, 'zh-Hans': { t: 'MIDI', sameAsEn: true, reviewed: 'bt' } },
    'label.trig':      { en: { t: 'TRIG' },    fr: { t: 'DÉCL',    reviewed: true }, 'zh-Hans': { t: '触发', reviewed: 'bt' } },  // 29.3 in a 60 px chip

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English name
    // is the same failure as a French page with an English caption. None has a
    // rendered box, so none is a geometry risk.
    //
    // Five of these replace a native title= that v1.13.0 carried. Contract §4
    // DELETES a native title rather than localizing it: on an element that also
    // has a data-tip it renders a second, untranslated OS tooltip competing with
    // the measure-then-pin renderer, and check-i18n assertion 11 now fails on
    // any that survive.
    'aria.presetPrev': { en: { t: 'Previous preset' },  fr: { t: 'Préréglage précédent', reviewed: true }, 'zh-Hans': { t: '上一个预设', reviewed: 'bt' } },
    'aria.presetNext': { en: { t: 'Next preset' },      fr: { t: 'Préréglage suivant',   reviewed: true }, 'zh-Hans': { t: '下一个预设', reviewed: 'bt' } },
    'aria.presetSave': { en: { t: 'Save preset' },      fr: { t: 'Enregistrer le préréglage', reviewed: true }, 'zh-Hans': { t: '保存预设', reviewed: 'bt' } },
    'aria.presetLoad': { en: { t: 'Load preset from file' },
                         fr: { t: 'Ouvrir un préréglage depuis un fichier', reviewed: true },
                         'zh-Hans': { t: '从文件载入预设',
                     reviewed: 'bt' },
                     },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true }, 'zh-Hans': { t: '界面语言', reviewed: 'bt' } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' },
                         fr: { t: 'Activer ou désactiver les infobulles', reviewed: true },
                         'zh-Hans': { t: '开关悬停帮助',
                     reviewed: 'bt' },
                     },
    // Bound by assigning dataset.i18nAria with a plain string literal in
    // buildPresetDropdown, which check-i18n assertion 15 counts as a reference
    // for exactly this case: an element the controller creates cannot carry the
    // attribute in the markup, and setLabel() writes textContent and so cannot
    // key an ATTRIBUTE.
    'aria.deletePreset': { en: { t: 'Delete preset' }, fr: { t: 'Supprimer le préréglage', reviewed: true }, 'zh-Hans': { t: '删除预设', reviewed: 'bt' } },
    'aria.settings':   { en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: true }, 'zh-Hans': { t: '设置', reviewed: 'bt' } },
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
    ['简体中文',
     'endonym — a language name is never translated. index.html writes it as the numeric references, but the coverage parser decodes them before the sweep runs, so the exemption has to carry the decoded text'],

    ['O-Polystutter',
     'the product name in .plugin-title — a product name is never translated, and this one is also the plugin\'s registered PRODUCT_NAME in CMakeLists.txt'],

    // #preset-name-text displays the loaded preset. The name IS the JSON
    // filename (OuariconPresetManager.h:283-285), so translating it breaks
    // recall outright. "Default" is the placeholder the manager overwrites on
    // its first pass.
    ['Default', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // The debug monitor in js/parameter-bindings.js, guarded by
    // `if (!debugParam) return;`. #debugParam, #debugValue and #debugNormalized
    // appear in no shipped markup, so these two composed strings render nowhere
    // in the plugin; they are a developer readout over a raw parameter value and
    // its normalised form, which D-01 and D-03 keep English anyway. Left in
    // place rather than deleted: removing working diagnostic code is not this
    // commit's job.
    ['Value: ${Math.round(state.normalisedValue * 15 + 1)}',
     'developer debug-monitor readout; its element exists in no shipped markup, and it mirrors a raw parameter value (D-01/D-03)'],
    ['Norm: ${state.normalisedValue.toFixed(3)}',
     'developer debug-monitor readout; its element exists in no shipped markup, and it mirrors a normalised parameter value (D-01/D-03)'],
];

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE, and on this page it is almost never the
// anchor. applyI18n() uses document.querySelector, which returns the FIRST match
// in document order — and this page has FOUR structurally identical lanes, each
// with an unattributed .knob-container per control. A class selector would have
// written all four lanes' tips onto lane 1 and left lanes 2-4 bare, which is the
// O-Octagon .vunit-group near-miss from Stage C with the multiplicity turned up.
//
// So every row names an element that already carries a UNIQUE id — the knob, the
// select, the progress fill — and the third field climbs to the box the tip
// should actually hang off. 105 anchors from 42 keys, generated from LANES so
// the four lanes cannot drift apart.
//
// EVERY ONE OF THESE ELEMENTS EXISTS IN index.html AT PARSE TIME. Nothing on
// this page is built at runtime except the preset dropdown rows, which carry no
// tip.
export const LANES = [1, 2, 3, 4];

export const TIP_BINDINGS = [
    ['#gear-btn',            'settings'],
    ['#lang-select',         'lang-select'],
    ['#tips-toggle',         'tips-toggle'],

    ['#preset-prev',         'preset-prev'],
    ['#preset-name-display', 'preset-dropdown', '.preset-dropdown'],
    ['#preset-next',         'preset-next'],
    ['#preset-save',         'preset-save'],
    ['#preset-load',         'preset-load'],

    ['#seq_toggle',          'seq'],
    ['#sequencer-section',   'sequencer'],

    ['#tape_bypass',         'tape-bypass'],
    ['#saturation',          'saturation', '.tape-knob-container'],
    ['#wow',                 'wow',        '.tape-knob-container'],
    ['#flutter',             'flutter',    '.tape-knob-container'],
    ['#hiss',                'hiss',       '.tape-knob-container'],
    ['#rolloff',             'rolloff',    '.tape-knob-container'],
    ['#dropout',             'dropout',    '.tape-knob-container'],
    ['#mix_dry',             'dry',        '.tape-knob-container'],
    ['#mix_wet',             'wet',        '.tape-knob-container'],
    ['#midi_toggle',         'midi'],
    ['#trig_toggle',         'trig'],

    ...LANES.flatMap((n) => [
        ['#lane' + n + '_enabled',             'lane'],
        ['#lane' + n + '_progress',            'progress',       '.progress-bar'],
        ['#lane' + n + '_subdivision',         'subdiv',         '.combo-container'],
        ['#lane' + n + '_repeats',             'repeats',        '.knob-container'],
        ['#lane' + n + '_decay',               'decay',          '.knob-container'],
        ['#lane' + n + '_filter',              'filter',         '.knob-container'],
        ['#lane' + n + '_probability',         'probability',    '.knob-container'],
        ['#lane' + n + '_volume',              'volume',         '.knob-container'],
        ['#lane' + n + '_pan',                 'pan',            '.knob-container'],
        ['#lane' + n + '_swing',               'swing',          '.knob-container'],
        ['#lane' + n + '_pitch',               'pitch',          '.knob-container'],
        ['#lane' + n + '_pitch_rand_enabled',  'pitch-rand'],
        ['#lane' + n + '_pitch_rand_min',      'pitch-rand-min', '.knob-container'],
        ['#lane' + n + '_pitch_rand_max',      'pitch-rand-max', '.knob-container'],
        ['#lane' + n + '_pitch_rand_quantize', 'pitch-quantize'],
        ['#lane' + n + '_pingpong',            'pingpong'],
        ['#lane' + n + '_reverse',             'reverse'],
        ['#lane' + n + '_manual',              'manual'],
        ['#lane' + n + '_euclidean_enabled',   'euclidean'],
        ['#lane' + n + '_euclidean_pulses',    'euc-pulses',     '.euc-dropdown-container'],
        ['#lane' + n + '_euclidean_steps',     'euc-steps',      '.euc-dropdown-container'],
    ]),
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. TIP_BINDINGS is evaluated once at
    // module load, so a localized string stored there would be frozen at the
    // load-time language.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
