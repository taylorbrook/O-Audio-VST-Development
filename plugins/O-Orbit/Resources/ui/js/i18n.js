/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
// i18n.js — O-Orbit interface copy, English + French (v1.2.2)
//
// ── v1.2.2: STAGE O (2026-08-31) — items 44 and 58, no copy changed ─────────
// No en or fr value, key, TIP_BINDINGS row or I18N_EXEMPT entry changed.
//   · Item 58: app.js gained the keyboard half of hover help (the Stage M
//     lastInputWasPointer latch): Tab into an anchor opens its tip, a click
//     does not. Nothing here to change — the bodies were always the same in
//     both paths.
//   · Item 44: the dead `.toggle-label { font-size: 9px }` was DELETED from
//     styles.css, not promoted. The pill keeps rendering at the 11px it has
//     always rendered at, so every width in this file — all measured at 11px —
//     still holds, and the ui.on / ui.off exemption below is now permanent
//     rather than "until the specificity is settled". The sentences that said
//     otherwise are corrected in place.
//
// ── v1.2.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 23 of 91 entries — 6 terminology, 15 typography, 1 grammar/agreement,
// 1 meaning, 3 exemption-only (the categories overlap: tempo_sync took two
// no-break spaces AND an agreement fix, speed took a term AND a cross-reference).
// sameAsEn: kept 6, translated 0, ADDED 2 (label.mix,
// label.groupSourceMix — a glossary root term that IS the English word makes
// the French a straight copy, and check-i18n assertion 4 requires the flag).
// termNote exemptions: 3 (ui.on, ui.off, downmix-badge — each listed at its
// entry with the measurement or the sense that earns it).
// Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
//
// Decisions the next reader needs:
//   · Mix wins over Mixage in three places (the group heading, the caption, the
//     tip title). "Mixage réducteur" on the downmix badge does NOT change — a
//     channel fold-down is not the dry/wet control, and it carries a termNote.
//     The speaker_layout BODY keeps "mixage réducteur" for the same reason;
//     bodies are not matched against the term list.
//   · Oui/Non stay on the elevation toggle. No glossary form fits either of the
//     two controls that key pair drives — the numbers are at the entry. At
//     v1.2.1 this was reported as a CSS specificity consequence; at v1.2.2 the
//     dead 9px rule was removed and 11px is the pill's size by decision.
//   · "Sync tempo" stays as the Tempo Sync caption: "Synchro tempo" WRAPS to
//     two lines in the 100.28px grid cell (59.58 x 26.00 over two lines against
//     79.19 x 13.00 on one), which is the one v1.2.0 width defence on this page
//     that re-measured true. The full "Synchro tempo" survives as the tip title.
//   · The Speed body now says "Vitesse", not "Fréquence", and names the control
//     it cross-refers to by its CAPTION ("Sync tempo"), which is what the other
//     five cross-references in this file already do.
//   · Fifteen no-break spaces (U+00A0) are now load-bearing across thirteen
//     French strings — before % : ; ? and between a number and its unit. They
//     are invisible in a diff and in most editors. Do not retype a French
//     string by hand; edit it and re-run scripts/i18n-fr-lint.js.
//
// UI ROOT IS Resources/ui, NOT Source/ui/public. CMakeLists.txt:54-62 embeds
// exactly seven files and every one of them is under Resources/ui; the stray
// Source/ui/public/modules/preset-manager.js in this plugin's tree is NOT
// embedded and NOT served. This file is the eighth SOURCES entry.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely. The
// symbol this file becomes is BinaryData::i18n_js, which does not collide with
// BinaryData::index_js (js/juce/index.js) already served by the same editor.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTip() builds the tip
// with createElement + textContent, and check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing `<`.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below was extracted
// mechanically from index.html at v1.1.1 rather than re-typed, with its HTML
// entities decoded to the characters they named (&#176; -> °, &#8230; -> …)
// because setAttribute + textContent do not decode entities.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and the
// element id otherwise. The eighteen parameter cells carried neither an id nor
// a data-param at v1.1.1 — a `.param-container` selector would have matched
// the FIRST of eighteen, which is exactly how O-Octagon's .vunit-group tip
// nearly landed on the wrong control in Stage C — so each one gained a
// data-param attribute naming the parameter its <label for> already names.
//
// ALL FRENCH WAS MACHINE-DRAFTED at v1.2.0 and REVIEWED at v1.2.1 against the
// suite glossary and lint (see the Stage N block above). It stays flagged
// `reviewed: false`: that flag means a NATIVE SPEAKER has read the entry, and
// none has. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    // The gear is new. The `help-toggle` entry below is the v1.1.0 "?" toggle's
    // copy, MOVED here unchanged along with the control itself — not
    // duplicated. One place for the two things that decide what the hover help
    // says and whether it says it.
    'gear-btn': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface, and turn the hover help on or off. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: "Choisir la langue de l’interface et activer ou désactiver les infobulles. Les deux choix sont conservés avec la session.",
              reviewed: true },
        'zh-Hans': { t: '设置',
                     b: '选择界面语言，并开启或关闭悬停帮助。两项选择都随会话一起保存。',
                     reviewed: 'mt' },
    },

    // Written to say what is TRUE of canon v2, in every declared language: the labels DO
    // change, and the halves that stay English are named rather than left to be
    // discovered — value readouts (D-03) and preset names (D-02, the name IS
    // the JSON filename). Speaker-layout format names are named too: 5.1, 7.1
    // and 7.1.4 are numeric designations and read identically in both.
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. Value readouts and preset names stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de ces infobulles. Les valeurs affichées et les noms de préréglages restent en anglais.",
              reviewed: true },
        'zh-Hans': { t: '语言',
                     b: '本页标签与这些悬停帮助所用的语言。数值读数和预设名称保持英文。',
                     reviewed: 'mt' },
    },

    // ── Header preset band ──────────────────────────────────────────────────
    'preset-prev': {
        en: { t: 'Previous preset',
              b: 'Step back through the preset list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Reculer dans la liste des préréglages.',
              reviewed: true },
        'zh-Hans': { t: '上一个预设',
                     b: '在预设列表中后退一项。',
                     reviewed: 'mt' },
    },
    'preset-next': {
        en: { t: 'Next preset',
              b: 'Step forward through the preset list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Avancer dans la liste des préréglages.',
              reviewed: true },
        'zh-Hans': { t: '下一个预设',
                     b: '在预设列表中前进一项。',
                     reviewed: 'mt' },
    },
    'preset-select': {
        en: { t: 'Preset',
              b: 'The preset currently loaded — click to browse by category. The 12 factory presets are read-only; saving under the same name writes a user copy instead.' },
        fr: { t: 'Préréglage',
              b: "Le préréglage actuellement chargé — cliquer pour parcourir par catégorie. Les 12 préréglages d’usine sont en lecture seule ; enregistrer sous le même nom écrit plutôt une copie utilisateur.",
              reviewed: true },
        'zh-Hans': { t: '预设',
                     b: '当前载入的预设，点击可按类别浏览。12 个出厂预设为只读；以同名保存时会改为写入一份用户副本。',
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
        fr: { t: 'Ouvrir',
              b: 'Charger un préréglage depuis un fichier.',
              reviewed: true },
        'zh-Hans': { t: '载入',
                     b: '从文件载入预设。',
                     reviewed: 'mt' },
    },
    'preset-delete': {
        en: { t: 'Delete',
              b: 'Delete the current user preset. Click once to arm it, again to confirm.' },
        fr: { t: 'Supprimer',
              b: 'Supprimer le préréglage utilisateur actuel. Cliquer une fois pour armer, une seconde fois pour confirmer.',
              reviewed: true },
        'zh-Hans': { t: '删除',
                     b: '删除当前的用户预设。点击一次进入待确认状态，再点击一次确认。',
                     reviewed: 'mt' },
    },

    // ── Header right ────────────────────────────────────────────────────────
    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Infobulles',
              b: 'Afficher une courte description lorsque le pointeur se pose sur un contrôle. Le réglage est conservé avec la session.',
              reviewed: true },
        'zh-Hans': { t: '悬停帮助',
                     b: '当指针停在控件上时显示简短说明。该设置随会话一起保存。',
                     reviewed: 'mt' },
    },
    'view-toggle': {
        en: { t: 'View',
              b: 'Switch between the motion visualizer and the speaker layout editor.' },
        fr: { t: 'Vue',
              b: "Basculer entre le visualiseur de mouvement et l’éditeur de disposition des enceintes.",
              reviewed: true },
        'zh-Hans': { t: '视图',
                     b: '在运动可视化视图与扬声器布局编辑器之间切换。',
                     reviewed: 'mt' },
    },

    // ── Speaker-editor toolbar: the named layout library ────────────────────
    'layout-select': {
        en: { t: 'Saved layouts',
              b: 'Load a named custom speaker layout from your library.' },
        fr: { t: 'Dispositions enregistrées',
              b: 'Charger une disposition d’enceintes personnalisée nommée depuis votre bibliothèque.',
              reviewed: true },
        'zh-Hans': { t: '已保存的布局',
                     b: '从库中载入一个已命名的自定义扬声器布局。',
                     reviewed: 'mt' },
    },
    'layout-name': {
        en: { t: 'Layout name',
              b: 'Name for saving the current speaker arrangement to your library.' },
        fr: { t: 'Nom de la disposition',
              b: 'Nom sous lequel enregistrer la disposition d’enceintes actuelle dans votre bibliothèque.',
              reviewed: true },
        'zh-Hans': { t: '布局名称',
                     b: '将当前扬声器摆位保存到库中时使用的名称。',
                     reviewed: 'mt' },
    },
    'layout-save-btn': {
        en: { t: 'Save layout',
              b: 'Save the current speaker arrangement under the name to the left.' },
        fr: { t: 'Enregistrer la disposition',
              b: "Enregistrer la disposition d’enceintes actuelle sous le nom saisi à gauche.",
              reviewed: true },
        'zh-Hans': { t: '保存布局',
                     b: '以左侧输入的名称保存当前扬声器摆位。',
                     reviewed: 'mt' },
    },
    'layout-delete-btn': {
        en: { t: 'Delete layout',
              b: 'Delete the selected saved layout. Click once to arm, again to confirm.' },
        fr: { t: 'Supprimer la disposition',
              b: 'Supprimer la disposition enregistrée sélectionnée. Cliquer une fois pour armer, une seconde fois pour confirmer.',
              reviewed: true },
        'zh-Hans': { t: '删除布局',
                     b: '删除选中的已保存布局。点击一次进入待确认状态，再点击一次确认。',
                     reviewed: 'mt' },
    },
    'export-btn': {
        en: { t: 'Export',
              b: 'Export the current speaker layout to a JSON file for sharing.' },
        fr: { t: 'Exporter',
              b: "Exporter la disposition d’enceintes actuelle vers un fichier JSON à partager.",
              reviewed: true },
        'zh-Hans': { t: '导出',
                     b: '将当前扬声器布局导出为 JSON 文件以便分享。',
                     reviewed: 'mt' },
    },
    'import-btn': {
        en: { t: 'Import',
              b: 'Import a speaker layout from a JSON file.' },
        fr: { t: 'Importer',
              b: "Importer une disposition d’enceintes depuis un fichier JSON.",
              reviewed: true },
        'zh-Hans': { t: '导入',
                     b: '从 JSON 文件导入扬声器布局。',
                     reviewed: 'mt' },
    },

    // ── Motion ──────────────────────────────────────────────────────────────
    // The five path names inside this body are the SAME five strings the Path
    // dropdown renders, and both are localized — so a French reader is told
    // about "Orbite" and then finds "Orbite" in the list. Composing the body
    // from the option keys would pin it to the load-time language: TIP_BINDINGS
    // is static data evaluated once, and tr()'s var-resolving arm exists for
    // exactly that case. It is not used here because the sentence needs its
    // verbs inflected around each name, not a name slotted into a template.
    'path': {
        en: { t: 'Path',
              b: 'Motion trajectory: Orbit circles, Pendulum swings, Linear sweeps and snaps back, Drift wanders organically, Ping-Pong sweeps back and forth without the snap.' },
        fr: { t: 'Trajectoire',
              b: 'Trajectoire du mouvement : Orbite décrit un cercle, Pendule oscille, Linéaire balaie puis revient d’un coup, Dérive vagabonde de façon organique, Va-et-vient balaie dans les deux sens sans le retour brusque.',
              reviewed: true },
        'zh-Hans': { t: '路径',
                     b: '运动的轨迹：环绕绕圈运行，钟摆来回摆动，线性扫过后瞬间跳回，漂移自然游走，乒乓来回扫动但没有跳回。',
                     reviewed: 'mt' },
    },
    'speed': {
        en: { t: 'Speed',
              b: 'Motion rate in cycles per second. Ignored while Tempo Sync is set to a division.' },
        fr: { t: 'Vitesse',
              b: 'Vitesse du mouvement en cycles par seconde. Ignorée tant que Sync tempo est réglée sur une division.',
              reviewed: true },
        'zh-Hans': { t: '速度',
                     b: '运动速率，单位为每秒周期数。当节拍同步设为某个划分时忽略此项。',
                     reviewed: 'mt' },
    },
    'width': {
        en: { t: 'Width',
              b: 'Angular span of the motion in degrees — 360 is a full circle around the listener.' },
        fr: { t: 'Largeur',
              b: 'Étendue angulaire du mouvement en degrés — 360 fait un cercle complet autour de l’auditeur.',
              reviewed: true },
        'zh-Hans': { t: '宽度',
                     b: '运动的角度跨度，单位为度；360 表示绕听者一整圈。',
                     reviewed: 'mt' },
    },
    'depth': {
        en: { t: 'Depth',
              b: 'Near/far motion. At 0% the source stays at the Distance radius; higher values move it toward and away from you each cycle.' },
        fr: { t: 'Profondeur',
              b: 'Mouvement de rapprochement et d’éloignement. À 0 %, la source reste au rayon défini par Distance ; au-delà, elle avance et recule à chaque cycle.',
              reviewed: true },
        'zh-Hans': { t: '深度',
                     b: '远近方向的运动。为 0% 时，声源停在距离所设的半径上；数值越高，它每个周期靠近再远离您的幅度就越大。',
                     reviewed: 'mt' },
    },
    'tilt': {
        en: { t: 'Tilt',
              b: 'Static elevation of the path in degrees, used while Elevation motion is off.' },
        fr: { t: 'Inclinaison',
              b: 'Élévation fixe de la trajectoire en degrés, utilisée tant que le mouvement Élévation est désactivé.',
              reviewed: true },
        'zh-Hans': { t: '倾斜',
                     b: '轨迹的固定仰角，单位为度，在仰角运动关闭时使用。',
                     reviewed: 'mt' },
    },
    'phase': {
        en: { t: 'Phase',
              b: 'Offset into the motion cycle in degrees — shifts where the source starts.' },
        fr: { t: 'Phase',
              b: 'Décalage dans le cycle du mouvement en degrés — déplace le point de départ de la source.',
              reviewed: true },
        'zh-Hans': { t: '相位',
                     b: '在运动周期中的偏移量，单位为度，用于改变声源的起始位置。',
                     reviewed: 'mt' },
    },
    'elevation_enable': {
        en: { t: 'Elevation',
              b: 'Adds vertical motion — the source rises and falls with the cycle instead of staying at the Tilt angle.' },
        fr: { t: 'Élévation',
              b: 'Ajoute un mouvement vertical — la source monte et descend avec le cycle au lieu de rester à l’angle défini par Inclinaison.',
              reviewed: true },
        'zh-Hans': { t: '仰角',
                     b: '加入垂直方向的运动：声源随周期上下起伏，而不是停在倾斜所设的角度上。',
                     reviewed: 'mt' },
    },
    'elevation_range': {
        en: { t: 'Elev Range',
              b: 'How far the elevation swings when Elevation motion is on, in degrees.' },
        fr: { t: 'Plage d’élévation',
              b: 'Amplitude du balayage vertical lorsque le mouvement Élévation est actif, en degrés.',
              reviewed: true },
        'zh-Hans': { t: '仰角范围',
                     b: '仰角运动开启时垂直摆动的幅度，单位为度。',
                     reviewed: 'mt' },
    },
    'tempo_sync': {
        en: { t: 'Tempo Sync',
              b: 'Locks the motion rate to the host tempo at the chosen division: 1/4 is one cycle per beat, 1 Bar one cycle per four beats (4/4 assumed). While the transport plays, motion phase locks to the beat position, so bounces are deterministic.' },
        fr: { t: 'Synchro tempo',
              b: 'Verrouille la vitesse du mouvement sur le tempo de l’hôte à la division choisie : 1/4 donne un cycle par temps, 1 mesure un cycle tous les quatre temps (4/4 supposé). Pendant la lecture, la phase du mouvement se cale sur la position rythmique, ce qui rend les exports déterministes.',
              reviewed: true },
        'zh-Hans': { t: '节拍同步',
                     b: '按所选划分把运动速率锁定到宿主速度：1/4 为每拍一个周期，1 小节为每四拍一个周期（按 4/4 计）。走带播放时，运动相位会锁定到节拍位置，因此导出结果是确定的。',
                     reviewed: 'mt' },
    },

    // ── Spatial ─────────────────────────────────────────────────────────────
    'speaker_layout': {
        en: { t: 'Speaker Layout',
              b: 'Target speaker arrangement. When the track has fewer channels, an energy-preserving downmix kicks in automatically (badge below shows when active).' },
        fr: { t: 'Disposition des enceintes',
              b: 'Disposition d’enceintes visée. Si la piste compte moins de canaux, un mixage réducteur à énergie constante s’active automatiquement (la pastille ci-dessous l’indique).',
              reviewed: true },
        'zh-Hans': { t: '扬声器布局',
                     b: '目标扬声器摆位。当音轨的声道数较少时，会自动启用保持能量的缩混（下方的标记会在其生效时显示）。',
                     reviewed: 'mt' },
    },
    'downmix-badge': {
        en: { t: 'Downmix',
              b: 'Shown when the layout has more channels than the track output — an energy-preserving fold-down is active.' },
        fr: { t: 'Mixage réducteur',
              b: 'Apparaît lorsque la disposition compte plus de canaux que la sortie de la piste — un repliement à énergie constante est actif.',
              reviewed: true,
              termNote: 'a downmix is a CHANNEL fold-down, not the dry/wet Mix control four cells away; '
                      + '"mixage réducteur" is the term French DAWs print for it, and the glossary forbids '
                      + '"mixage" precisely because it means the mixing process — which is what this badge names' },
        'zh-Hans': { t: '缩混',
                     b: '当布局的声道数多于音轨输出时显示，表示保持能量的折叠缩混正在生效。',
                     reviewed: 'mt' },
    },
    'distance': {
        en: { t: 'Distance',
              b: 'Base distance of the source in meters — farther is quieter and darker.' },
        fr: { t: 'Distance',
              b: 'Distance de base de la source en mètres — plus loin, plus faible et plus sombre.',
              reviewed: true },
        'zh-Hans': { t: '距离',
                     b: '声源的基准距离，单位为米；越远越轻，也越暗。',
                     reviewed: 'mt' },
    },
    'air_absorption': {
        en: { t: 'Air Absorption',
              b: 'High-frequency loss with distance — more absorption makes distant sources darker.' },
        fr: { t: 'Absorption de l’air',
              b: 'Perte des aigus avec la distance — plus d’absorption assombrit les sources lointaines.',
              reviewed: true },
        'zh-Hans': { t: '空气吸收',
                     b: '随距离产生的高频损失；吸收越多，远处的声源就越暗。',
                     reviewed: 'mt' },
    },
    'attenuation_curve': {
        en: { t: 'Atten Curve',
              b: 'How level falls with distance: Linear, Inverse (1/d), or Inverse Square (1/d²).' },
        fr: { t: 'Courbe d’atténuation',
              b: 'Manière dont le niveau décroît avec la distance : Linéaire, Inverse (1/d) ou Inverse carrée (1/d²).',
              reviewed: true },
        'zh-Hans': { t: '衰减曲线',
                     b: '电平随距离下降的方式：线性、反比（1/d）或反比平方（1/d²）。',
                     reviewed: 'mt' },
    },
    'center_diverge': {
        en: { t: 'Center Diverge',
              b: 'Spreads energy into more speakers as it rises — 0% is the sharpest point-source imaging.' },
        fr: { t: 'Divergence centrale',
              b: 'Répartit l’énergie sur davantage d’enceintes à mesure qu’elle augmente — à 0 %, l’image ponctuelle est la plus précise.',
              reviewed: true },
        'zh-Hans': { t: '中置发散',
                     b: '数值越高，能量分散到越多的扬声器上；为 0% 时点声源的定位最锐利。',
                     reviewed: 'mt' },
    },

    // ── Source / Mix ────────────────────────────────────────────────────────
    'source_mode': {
        en: { t: 'Source Mode',
              b: 'Mono sums the input into one moving source; L+R Split moves the left and right channels as two separate sources.' },
        fr: { t: 'Mode source',
              b: 'Mono additionne l’entrée en une seule source mobile ; Séparé G+D déplace les canaux gauche et droit comme deux sources distinctes.',
              reviewed: true },
        'zh-Hans': { t: '源模式',
                     b: '单声道把输入相加为一个移动的声源；L+R 分离则把左右声道作为两个独立的声源移动。',
                     reviewed: 'mt' },
    },
    'lr_offset': {
        en: { t: 'L/R Offset',
              b: 'Angle between the left and right sources in L+R Split mode — 180° keeps them opposite.' },
        fr: { t: 'Décalage G/D',
              b: 'Angle entre les sources gauche et droite en mode Séparé G+D — 180° les maintient opposées.',
              reviewed: true },
        'zh-Hans': { t: 'L/R 偏移',
                     b: 'L+R 分离模式下左右两个声源之间的夹角；180° 使二者始终相对。',
                     reviewed: 'mt' },
    },
    'mix': {
        en: { t: 'Mix',
              b: 'Dry/wet balance. Wet is the spatialized signal on all outputs; dry stays on its native input channels.' },
        fr: { t: 'Mix',
              b: 'Équilibre son direct / son traité. Le son traité est le signal spatialisé sur toutes les sorties ; le son direct reste sur ses canaux d’entrée d’origine.',
              reviewed: true },
        'zh-Hans': { t: '混合',
                     b: '干湿平衡。湿声是所有输出上的空间化信号；干声则留在其原有的输入声道上。',
                     reviewed: 'mt' },
    },
});

// ============================================================================
// LABELS — the page's own captions, v1.2.1
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
// FRENCH REVIEWED at v1.2.1 against the glossary; still `reviewed: false`,
// which records the native-speaker reading that has not happened yet.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Preset band ─────────────────────────────────────────────────────────
    // NOT the `preset-save` / `preset-load` / `preset-delete` tooltip keys.
    // Those titles are the full verbs ("Enregistrer", "Supprimer"); these three
    // buttons sit in a 34px header band beside a 130-190px preset readout, two
    // 20px nav circles and the gear, at 10px uppercase. The abbreviations are
    // the same ones O-ReverseDelay settled on in Stage H.
    'label.save':    { en: { t: 'Save' },   fr: { t: 'Enreg.',  reviewed: true },
                       'zh-Hans': { t: '保存', reviewed: 'mt' } },
    'label.load':    { en: { t: 'Load' },   fr: { t: 'Ouvrir',  reviewed: true },
                       'zh-Hans': { t: '载入', reviewed: 'mt' } },
    'label.delete':  { en: { t: 'Del' },    fr: { t: 'Suppr.',  reviewed: true },
                       'zh-Hans': { t: '删除', reviewed: 'mt' } },

    // The armed face of BOTH two-click delete buttons — the preset one in the
    // header and the layout one in the editor toolbar. It goes through
    // setLabel(), so the element becomes a [data-i18n] element and the language
    // sweep owns it. Through v1.1.1 it was a data-confirm ATTRIBUTE, which was
    // the right answer while the page was English-only — it kept the copy out
    // of app.js, which is what pattern_js_state_updater_overwrites_html_labels
    // asks for — and the wrong one the moment the page had two languages: an
    // attribute holds ONE string, so a language switch while a button was armed
    // would have restored the ENGLISH armed face.
    'ui.confirm':    { en: { t: 'Sure?' },  fr: { t: 'Sûr ?',   reviewed: true },
                       'zh-Hans': { t: '确定？', reviewed: 'mt' } },

    // ── The view toggle, both faces ─────────────────────────────────────────
    // Written from script on every click, so both faces are keys. Two separate
    // setLabel() calls in the two arms of the if/else, never one call with a
    // ternary in its argument: check-i18n assertion 13 rejects that shape.
    'label.viewMotion': { en: { t: 'Motion View' },
                          fr: { t: 'Vue mouvement', reviewed: true },
                          'zh-Hans': { t: '运动视图', reviewed: 'mt' } },
    'label.viewEditor': { en: { t: 'Speaker Editor' },
                          fr: { t: 'Éditeur d’enceintes', reviewed: true },
                          'zh-Hans': { t: '扬声器编辑器', reviewed: 'mt' } },

    // ── The elevation toggle, both faces ────────────────────────────────────
    // ONE key pair, TWO controls: the 50px elevation pill (.toggle-label) and
    // the 46px hover-help button in the settings popover (.settings-toggle).
    // The tighter of the two governs, and neither takes a glossary form.
    //
    // RE-MEASURED at v1.2.1 with Range.selectNodeContents on the shipping frame,
    // because the v1.2.0 defence above it was arithmetic on the WRONG font. It
    // said "9px uppercase … the budget is three glyphs". It was never 9px: the
    // `.toggle-label { font-size: 9px }` rule lost to `.param-container label
    // { font-size: 11px }` (0,1,1 beats 0,1,0), so the pill rendered at 11px
    // from v1.0.0, and at v1.2.2 the dead declaration was deleted — 11px is the
    // pill's size by decision now, not by accident. Measured at that 11px,
    // in a 46.00px content box (re-confirmed at v1.2.2, same numbers):
    //
    //     OUI    22.56   NON    26.84   ARRÊT   41.06   AUCUN  44.13   fit
    //     ACTIVÉ 46.33 (over by 0.33)   MARCHE  53.06 (over by 7.06)
    //     DÉSACTIVÉ 70.11              DÉSACTIVÉE 77.83               do not
    //
    // and in the settings toggle's 44.00px content box (10px, flex: 0 0 46px,
    // so it cannot grow): ARRÊT 35.28 and ACTIVÉ 39.67 fit, MARCHE 45.80 is
    // over by 1.80, DÉSACTIVÉE 66.67 is far over.
    //
    // So the OFF side has a glossary form that fits (ARRÊT, both sites) and the
    // ON side has none — and half a pair is worse French than the calque: an
    // ARRÊT face whose opposite reads OUI names two different oppositions on
    // one 24px pill. Both stay, both carry a termNote with the number.
    //
    // The exemption is a size consequence, not a translation one. At 9px MARCHE
    // measures 44.52px and the settled MARCHE/ARRÊT pair would fit; but 9px was
    // never rendered, promoting it would shrink the face of a shipped control,
    // and v1.2.2 removed the dead rule instead. The exemption is permanent at
    // 11px unless the pill itself is widened.
    'ui.on':         { en: { t: 'On' },
                       fr: { t: 'Oui', reviewed: true,
                             termNote: 'MARCHE measures 53.06px and ACTIVÉ 46.33px in the elevation pill\'s '
                                     + '46.00px content box, and MARCHE 45.80px in the settings toggle\'s 44.00px '
                                     + 'box — no glossary form fits either site, so the pair stays as drafted' },
                       'zh-Hans': { t: '开启', reviewed: 'mt' } },
    'ui.off':        { en: { t: 'Off' },
                       fr: { t: 'Non', reviewed: true,
                             termNote: 'ARRÊT does fit (41.06px of 46.00, 35.28px of 44.00) but its partner does '
                                     + 'not, and ARRÊT opposite OUI is a mismatched pair on one toggle — held '
                                     + 'with ui.on; the pill is 11px by decision since v1.2.2 (dead 9px rule removed)' },
                       'zh-Hans': { t: '关闭', reviewed: 'mt' } },

    // ── Editor toolbar: the eight layout preset buttons ─────────────────────
    // 5.1, 7.1, 5.1.4 and 7.1.4 are absent: a channel-count designation is
    // digits and dots, it is identical in both languages, and the coverage scan
    // classifies it as a non-label for exactly that reason. Only the four
    // WORDED buttons need keys.
    'label.fmtStereo': { en: { t: 'Stereo' }, fr: { t: 'Stéréo', reviewed: true },
                         'zh-Hans': { t: '立体声', reviewed: 'mt' } },
    'label.fmtQuad':   { en: { t: 'Quad' },   fr: { t: 'Quad',   reviewed: true, sameAsEn: true },
                         'zh-Hans': { t: '四声道', reviewed: 'mt' } },
    'label.fmtHex':    { en: { t: 'Hex' },    fr: { t: 'Hexa',   reviewed: true },
                         'zh-Hans': { t: '六声道', reviewed: 'mt' } },
    'label.fmtOct':    { en: { t: 'Oct' },    fr: { t: 'Octo',   reviewed: true },
                         'zh-Hans': { t: '八声道', reviewed: 'mt' } },

    // ── Editor toolbar: the layout library and file buttons ─────────────────
    'label.layoutsPlaceholder': { en: { t: 'Layouts…' },
                                  fr: { t: 'Dispositions…', reviewed: true },
                                  'zh-Hans': { t: '布局…', reviewed: 'mt' } },
    'label.export':  { en: { t: 'Export' }, fr: { t: 'Exporter', reviewed: true },
                       'zh-Hans': { t: '导出', reviewed: 'mt' } },
    'label.import':  { en: { t: 'Import' }, fr: { t: 'Importer', reviewed: true },
                       'zh-Hans': { t: '导入', reviewed: 'mt' } },

    // ── Group headings ──────────────────────────────────────────────────────
    // "Source / Mix", not the v1.2.0 draft's "Source / Mixage": the suite
    // glossary settles Mix as Mix — it is what every French DAW prints — and
    // forbids "mixage" in a label because mixage is the mixing PROCESS. The
    // heading then equals its English, so it carries sameAsEn: true, which is
    // the declaration that a reader looked and agreed the word is French too.
    'label.groupMotion':   { en: { t: 'Motion' },
                             fr: { t: 'Mouvement', reviewed: true },
                             'zh-Hans': { t: '运动', reviewed: 'mt' } },
    'label.groupSpatial':  { en: { t: 'Spatial' },
                             fr: { t: 'Spatial', reviewed: true, sameAsEn: true },
                             'zh-Hans': { t: '空间', reviewed: 'mt' } },
    'label.groupSourceMix': { en: { t: 'Source / Mix' },
                              fr: { t: 'Source / Mix', reviewed: true, sameAsEn: true },
                              'zh-Hans': { t: '源 / 混合', reviewed: 'mt' } },

    // ── Motion parameter captions ───────────────────────────────────────────
    // Eight of these are identical to their tooltip TITLE in both languages and
    // could have reused the tooltip key. They do not, deliberately: a caption
    // and a tip title diverge the moment either is edited, and the two that
    // ALREADY diverge here (Elev Range / Plage élév., Atten Curve / Courbe
    // attén.) prove the divergence is not hypothetical on this page — the
    // captions live in 90px grid cells and the titles live in a 230px tip.
    'label.path':      { en: { t: 'Path' },       fr: { t: 'Trajectoire', reviewed: true },
                         'zh-Hans': { t: '路径', reviewed: 'mt' } },
    'label.speed':     { en: { t: 'Speed' },      fr: { t: 'Vitesse',     reviewed: true },
                         'zh-Hans': { t: '速度', reviewed: 'mt' } },
    'label.width':     { en: { t: 'Width' },      fr: { t: 'Largeur',     reviewed: true },
                         'zh-Hans': { t: '宽度', reviewed: 'mt' } },
    'label.depth':     { en: { t: 'Depth' },      fr: { t: 'Profondeur',  reviewed: true },
                         'zh-Hans': { t: '深度', reviewed: 'mt' } },
    'label.tilt':      { en: { t: 'Tilt' },       fr: { t: 'Inclinaison', reviewed: true },
                         'zh-Hans': { t: '倾斜', reviewed: 'mt' } },
    'label.phase':     { en: { t: 'Phase' },      fr: { t: 'Phase', reviewed: true, sameAsEn: true },
                         'zh-Hans': { t: '相位', reviewed: 'mt' } },
    'label.elevation': { en: { t: 'Elevation' },  fr: { t: 'Élévation',   reviewed: true },
                         'zh-Hans': { t: '仰角', reviewed: 'mt' } },
    'label.elevRange': { en: { t: 'Elev Range' }, fr: { t: 'Plage élév.', reviewed: true },
                         'zh-Hans': { t: '仰角范围', reviewed: 'mt' } },
    // SIZED, and this is the one French string on this page that was.
    // "Synchro tempo" measures 105.4px in this cell's own font and the Motion
    // group's grid track is 100.3px (repeat(auto-fit, minmax(90px, 1fr)) over
    // nine items resolves to seven 100.3px columns), so it WRAPPED to two lines
    // and pushed the #tempo_sync select down 13px. "Sync tempo" measures 79.2 —
    // 21px of margin — and "Sync" is what French DAW interfaces call this. The
    // full phrase survives as the tooltip TITLE, which renders in a 230px box.
    'label.tempoSync': { en: { t: 'Tempo Sync' }, fr: { t: 'Sync tempo', reviewed: true },
                         'zh-Hans': { t: '节拍同步', reviewed: 'mt' } },

    // ── Path dropdown options ───────────────────────────────────────────────
    'label.pathOrbit':    { en: { t: 'Orbit' },     fr: { t: 'Orbite',      reviewed: true },
                            'zh-Hans': { t: '环绕', reviewed: 'mt' } },
    'label.pathPendulum': { en: { t: 'Pendulum' },  fr: { t: 'Pendule',     reviewed: true },
                            'zh-Hans': { t: '钟摆', reviewed: 'mt' } },
    'label.pathDrift':    { en: { t: 'Drift' },     fr: { t: 'Dérive',      reviewed: true },
                            'zh-Hans': { t: '漂移', reviewed: 'mt' } },
    'label.pathPingPong': { en: { t: 'Ping-Pong' }, fr: { t: 'Va-et-vient', reviewed: true },
                            'zh-Hans': { t: '乒乓', reviewed: 'mt' } },

    // Shared by the Path dropdown and the Atten Curve dropdown. Identical in
    // BOTH languages at both sites, which is the only condition under which the
    // reuse rule allows one key to serve two controls.
    'label.linear':       { en: { t: 'Linear' },    fr: { t: 'Linéaire',    reviewed: true },
                            'zh-Hans': { t: '线性', reviewed: 'mt' } },

    // ── Tempo Sync dropdown: the four WORDED options ────────────────────────
    // The eleven note-value options (1/16T … 1/2D) are digits, slashes and the
    // T/D suffixes that mean triplet and dotted in both languages' notation.
    // They carry no key and are exempt below.
    //
    // NOT `ui.off`: that key is the elevation toggle's OFF face, pinned to a
    // 46px pill. This one sits in an 84.28px dropdown with room, so it takes
    // the glossary's feature-sense form in full — "Désactivé" 43.38px, 40.90px
    // to spare, against the v1.2.0 draft's clipped "Désact." 32.69px. The
    // feature sense, not "Aucune": the English is Off, and Off here means the
    // sync is not running, not that a division is unselected.
    'label.syncOff':   { en: { t: 'Off' },     fr: { t: 'Désactivé',  reviewed: true },
                         'zh-Hans': { t: '关闭', reviewed: 'mt' } },
    'label.bar1':      { en: { t: '1 Bar' },   fr: { t: '1 mesure',   reviewed: true },
                         'zh-Hans': { t: '1 小节', reviewed: 'mt' } },
    'label.bars2':     { en: { t: '2 Bars' },  fr: { t: '2 mesures',  reviewed: true },
                         'zh-Hans': { t: '2 小节', reviewed: 'mt' } },
    'label.bars4':     { en: { t: '4 Bars' },  fr: { t: '4 mesures',  reviewed: true },
                         'zh-Hans': { t: '4 小节', reviewed: 'mt' } },

    // ── Spatial parameter captions ──────────────────────────────────────────
    // "Enceintes", not "Disposition des enceintes": this caption sits in a
    // grid cell whose track is minmax(90px, 1fr) and the English "Speaker
    // Layout" already wraps to two lines there. The full phrase is what the
    // TOOLTIP title says.
    'label.speakerLayout': { en: { t: 'Speaker Layout' },
                             fr: { t: 'Enceintes', reviewed: true },
                             'zh-Hans': { t: '扬声器布局', reviewed: 'mt' } },
    'label.distance':      { en: { t: 'Distance' },
                             fr: { t: 'Distance', reviewed: true, sameAsEn: true },
                             'zh-Hans': { t: '距离', reviewed: 'mt' } },
    // "Absorption air" is the glossary's short form and it is FREE here: at
    // 11px uppercase it measures 104.17px, the same 104.17px as the English
    // "Air Absorption" it replaces, letter for letter. The v1.2.0 draft's bare
    // "Absorption" (79.08px) dropped the half of the name that says WHAT is
    // absorbing. The full "Absorption de l'air" is 135.88px and does not fit;
    // it is what the TOOLTIP title says.
    'label.airAbsorption': { en: { t: 'Air Absorption' },
                             fr: { t: 'Absorption air', reviewed: true },
                             'zh-Hans': { t: '空气吸收', reviewed: 'mt' } },
    'label.attenCurve':    { en: { t: 'Atten Curve' },
                             fr: { t: 'Courbe attén.', reviewed: true },
                             'zh-Hans': { t: '衰减曲线', reviewed: 'mt' } },
    'label.centerDiverge': { en: { t: 'Center Diverge' },
                             fr: { t: 'Divergence', reviewed: true },
                             'zh-Hans': { t: '中置发散', reviewed: 'mt' } },

    // ── Speaker Layout dropdown: the four WORDED options ────────────────────
    // Stereo and Quad reuse the toolbar buttons' keys — same string, same two
    // languages, same meaning. Hexaphonic and Octaphonic are the full words the
    // toolbar abbreviates and need their own.
    'label.hexaphonic':    { en: { t: 'Hexaphonic' },
                             fr: { t: 'Hexaphonique', reviewed: true },
                             'zh-Hans': { t: '六声道', reviewed: 'mt' } },
    'label.octaphonic':    { en: { t: 'Octaphonic' },
                             fr: { t: 'Octophonique', reviewed: true },
                             'zh-Hans': { t: '八声道', reviewed: 'mt' } },

    // ── Atten Curve dropdown options ────────────────────────────────────────
    'label.inverse':       { en: { t: 'Inverse' },
                             fr: { t: 'Inverse', reviewed: true, sameAsEn: true },
                             'zh-Hans': { t: '反比', reviewed: 'mt' } },
    'label.inverseSquare': { en: { t: 'Inverse Square' },
                             fr: { t: 'Inverse carrée', reviewed: true },
                             'zh-Hans': { t: '反比平方', reviewed: 'mt' } },

    // ── Source / Mix ────────────────────────────────────────────────────────
    'label.sourceMode': { en: { t: 'Source Mode' },
                          fr: { t: 'Mode source', reviewed: true },
                          'zh-Hans': { t: '源模式', reviewed: 'mt' } },
    'label.mono':       { en: { t: 'Mono' },
                          fr: { t: 'Mono', reviewed: true, sameAsEn: true },
                          'zh-Hans': { t: '单声道', reviewed: 'mt' } },
    'label.lrSplit':    { en: { t: 'L+R Split' },
                          fr: { t: 'Séparé G+D', reviewed: true },
                          'zh-Hans': { t: 'L+R 分离', reviewed: 'mt' } },
    'label.lrOffset':   { en: { t: 'L/R Offset' },
                          fr: { t: 'Décalage G/D', reviewed: true },
                          'zh-Hans': { t: 'L/R 偏移', reviewed: 'mt' } },
    'label.mix':        { en: { t: 'Mix' },
                          fr: { t: 'Mix', reviewed: true, sameAsEn: true },
                          'zh-Hans': { t: '混合', reviewed: 'mt' } },

    // ── The downmix badge ───────────────────────────────────────────────────
    // A COMPOSED entry, and the only one on this page. The two numbers are
    // channel counts and stay numbers (D-03); "ch" is not a unit symbol like Hz
    // or dB, it is an abbreviation of the WORD "channels", so it localizes.
    // This is the O-Contrabass "4 of 4" shape: the connective is what changes,
    // never the number. Rendered through setLabel(el, key, vars), so the vars
    // ride on the element as data-i18n-vars and the language sweep re-renders
    // the badge with the SAME counts rather than a stale English face.
    'ui.downmix':       { en: { t: '{from}ch → {to}ch' },
                          fr: { t: '{from} can. → {to} can.', reviewed: true },
                          'zh-Hans': { t: '{from} 声道 → {to} 声道', reviewed: 'mt' } },

    // ── Accessible names ────────────────────────────────────────────────────
    // #preset-prev and #preset-next do NOT appear here: their accessible name
    // is word-for-word their tooltip title in both languages, so they point
    // data-i18n-aria at the tooltip key and trLabel()'s I18N fallback resolves
    // it. The four below differ from their control's tip title, or belong to a
    // control that has no tip.
    'aria.browsePresets': { en: { t: 'Browse presets' },
                            fr: { t: 'Parcourir les préréglages', reviewed: true },
                            'zh-Hans': { t: '浏览预设', reviewed: 'mt' } },
    'aria.presets':       { en: { t: 'Presets' },
                            fr: { t: 'Préréglages', reviewed: true },
                            'zh-Hans': { t: '预设', reviewed: 'mt' } },
    'aria.helpToggle':    { en: { t: 'Toggle hover help' },
                            fr: { t: 'Activer ou désactiver les infobulles', reviewed: true },
                            'zh-Hans': { t: '开关悬停帮助', reviewed: 'mt' } },
    'aria.settings':      { en: { t: 'Settings' },
                            fr: { t: 'Réglages', reviewed: true },
                            'zh-Hans': { t: '设置', reviewed: 'mt' } },
    'aria.langSelect':    { en: { t: 'Interface language' },
                            fr: { t: 'Langue de l’interface', reviewed: true },
                            'zh-Hans': { t: '界面语言', reviewed: 'mt' } },

    // The layout-name field's placeholder. Lower-case in both, matching the
    // authored English — a placeholder on this page is a hint, not a caption.
    'placeholder.layoutName': { en: { t: 'name…' },
                                fr: { t: 'nom…', reviewed: true },
                                'zh-Hans': { t: '名称…', reviewed: 'mt' } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['O-Orbit',
     'the product name — a product name is never translated'],
    ['Ouaricon Audio',
     'the company name, in the footer plate'],

    // #preset-name displays the loaded preset. The name IS the JSON filename
    // (OuariconPresetManager.h:283-285), so translating it breaks recall: a
    // session saved against "Default" would not resolve its French. The element
    // is also written by modules/preset-manager.js on every load, so making it
    // a [data-i18n] element would put the sweep and the module in a fight over
    // one node (pattern_js_state_updater_overwrites_html_labels).
    ['Default',
     'the factory preset name shown at rest — exempt under D-02, because the name IS the JSON filename'],

    // The two endonyms in the language selector. A language name is written in
    // its OWN language: a French speaker looking for their language looks for
    // "Français", not "French".
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
    ['简体中文', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds on every row: this page
// authors its tips on the .param-container cell rather than on the knob inside
// it, so no closest(wrapper) walk is needed anywhere.
//
// The eighteen parameter cells are addressed by the data-param attribute added
// in v1.2.0. Before it they carried neither an id nor any distinguishing
// attribute, and `.param-container` would have matched the FIRST of eighteen —
// the failure mode canon §1 names, and the one O-Octagon's .vunit-group tip hit
// for real in Stage C. The value of data-param is exactly the string the cell's
// own <label for="..."> already carries, so the two cannot drift apart without
// the browser's own label association breaking first.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                            'gear-btn'],
    ['#lang-select',                         'lang-select'],
    ['#help-toggle',                         'help-toggle'],

    ['#preset-prev',                         'preset-prev'],
    ['#preset-next',                         'preset-next'],
    ['#preset-select',                       'preset-select'],
    ['#preset-save',                         'preset-save'],
    ['#preset-load',                         'preset-load'],
    ['#preset-delete',                       'preset-delete'],

    ['#view-toggle',                         'view-toggle'],

    ['#layout-select',                       'layout-select'],
    ['#layout-name',                         'layout-name'],
    ['#layout-save-btn',                     'layout-save-btn'],
    ['#layout-delete-btn',                   'layout-delete-btn'],
    ['#export-btn',                          'export-btn'],
    ['#import-btn',                          'import-btn'],

    ['[data-param="path"]',                  'path'],
    ['[data-param="speed"]',                 'speed'],
    ['[data-param="width"]',                 'width'],
    ['[data-param="depth"]',                 'depth'],
    ['[data-param="tilt"]',                  'tilt'],
    ['[data-param="phase"]',                 'phase'],
    ['[data-param="elevation_enable"]',      'elevation_enable'],
    ['[data-param="elevation_range"]',       'elevation_range'],
    ['[data-param="tempo_sync"]',            'tempo_sync'],

    ['[data-param="speaker_layout"]',        'speaker_layout'],
    ['#downmix-badge',                       'downmix-badge'],
    ['[data-param="distance"]',              'distance'],
    ['[data-param="air_absorption"]',        'air_absorption'],
    ['[data-param="attenuation_curve"]',     'attenuation_curve'],
    ['[data-param="center_diverge"]',        'center_diverge'],

    ['[data-param="source_mode"]',           'source_mode'],
    ['[data-param="lr_offset"]',             'lr_offset'],
    ['[data-param="mix"]',                   'mix'],
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
