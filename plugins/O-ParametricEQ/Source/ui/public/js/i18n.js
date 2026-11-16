/*
   This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
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
// i18n.js - O-ParametricEQ hover-help copy and labels: en / fr / zh-Hans.
// Generated as v1-i18n.js by UI mockup finalization; copied to
// Source/ui/public/js/i18n.js during Stage 3.
//
// EXPORTS ONLY. No top-level statement outside an export declaration: a bare
// statement here throws out of module evaluation and takes the whole page with
// it (check-i18n [7]).
//
// Copy is textContent on every path. No markup, and no less-than sign in any
// string (check-i18n [9]).
//
// en is authored from parameter-spec.md.
// fr is MACHINE-DRAFTED against scripts/i18n-fr-glossary.js: every entry is
//    `reviewed: false`. No native speaker has read it.
// zh-Hans is MACHINE-DRAFTED against scripts/i18n-zh-glossary.js: every entry
//    is `reviewed: 'mt'`.

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

// Tooltip copy: title `t` and body `b`. A control whose caption IS its tooltip
// title uses the same key for its data-i18n caption (trLabel falls back here).
export const I18N = Object.freeze({
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface, and whether this hover help appears at all. '
               + 'The choice is remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisissez la langue de l’interface, et l’affichage ou non de ces infobulles. '
               + 'Le choix est conservé avec la session.',
              reviewed: false },
        'zh-Hans': { t: '设置',
              b: '选择界面语言，以及是否显示悬停帮助。该选择随工程一起保存。',
              reviewed: 'mt' },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. Value readouts and preset names stay as they are.' },
        fr: { t: 'Langue',
              b: 'La langue de ces infobulles et des libellés de la page. Les valeurs affichées et les noms de préréglages ne changent pas.',
              reviewed: false },
        'zh-Hans': { t: '语言',
              b: '悬停帮助和页面标签所用的语言。数值读数和预设名称保持不变。',
              reviewed: 'mt' },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: false },
        'zh-Hans': { t: '悬停帮助',
              b: '开启或关闭悬停帮助。关闭后，只有齿轮按钮和这个开关仍会显示说明。',
              reviewed: 'mt' },
    },

    // ── Global parameters ───────────────────────────────────────────────────
    'toggle-bypass': {
        en: { t: 'Bypass',
              b: 'Bypasses the whole equalizer. The switch is click-free, and it is the same bypass the host automates.' },
        fr: { t: 'Contour.',
              b: 'Contourne tout l’égaliseur. La commutation se fait sans clic, et c’est le même contournement que celui automatisé par l’hôte.',
              reviewed: false },
        'zh-Hans': { t: '旁通',
              b: '旁通整个均衡器。切换时没有咔嗒声，与宿主自动化所用的旁通是同一个。',
              reviewed: 'mt' },
    },
    'knob-output_gain': {
        en: { t: 'Output gain',
              b: 'Level trim after the equalizer, from -36 to +36 dB. Drag, scroll or use the arrow keys; hold Shift for fine steps; double-click to reset. Click the readout to type a value.' },
        fr: { t: 'Gain de sortie',
              b: 'Réglage du niveau après l’égaliseur, de \u221236 à +36\u00A0dB. Faites glisser, utilisez la molette ou les flèches\u00A0; maintenez Maj pour un réglage fin\u00A0; double-cliquez pour réinitialiser. Cliquez sur la valeur pour la saisir.',
              reviewed: false },
        'zh-Hans': { t: '输出增益',
              b: '均衡器之后的电平微调，范围 -36 至 +36 dB。可拖动、滚动或使用方向键；按住 Shift 进行微调；双击复位。点击读数可直接输入数值。',
              reviewed: 'mt' },
    },
    'knob-gain_scale': {
        en: { t: 'Gain scale',
              b: 'Scales the gain of every band together, from 0 to 200 %. At 100 % each band plays at the gain it shows; at 0 % the gain-bearing bands are flat.' },
        fr: { t: 'Échelle de gain',
              b: 'Met à l’échelle le gain de toutes les bandes ensemble, de 0 à 200\u00A0%. À 100\u00A0%, chaque bande joue au gain affiché\u00A0; à 0\u00A0%, les bandes à gain sont plates.',
              reviewed: false },
        'zh-Hans': { t: '增益缩放',
              b: '同时缩放所有频段的增益，范围 0 至 200 %。100 % 时各频段按显示的增益工作；0 % 时带增益的频段变为平直。',
              reviewed: 'mt' },
    },
    'toggle-auto_gain': {
        en: { t: 'Auto gain',
              b: 'Compensates the loudness change the curve introduces. The correction is computed from the curve, not from the signal, so it does not pump.' },
        fr: { t: 'Gain auto',
              b: 'Compense le changement de niveau sonore introduit par la courbe. La correction est calculée à partir de la courbe et non du signal\u00A0: elle ne pompe pas.',
              reviewed: false },
        'zh-Hans': { t: '自动增益',
              b: '补偿曲线带来的响度变化。补偿量由曲线计算得出，而不是由信号得出，因此不会产生抽吸感。',
              reviewed: 'mt' },
    },

    // ── Session controls (UI state, not host parameters) ────────────────────
    'analyzer': {
        en: { t: 'Analyzer',
              b: 'What the spectrum analyzer shows behind the curve: nothing, the input (Pre), the output (Post), or both. It is display only and is not automated.' },
        fr: { t: 'Analyseur',
              b: 'Ce que l’analyseur de spectre affiche derrière la courbe\u00A0: rien, l’entrée (Pré), la sortie (Post) ou les deux. Affichage uniquement, sans automation.',
              reviewed: false },
        'zh-Hans': { t: '频谱分析',
              b: '曲线后方频谱分析的显示内容：不显示、输入（前）、输出（后）或两者同时显示。仅用于显示，不参与自动化。',
              reviewed: 'mt' },
    },
    'display-range': {
        en: { t: 'Display range',
              b: 'The vertical span of the response display: 6, 12 or 30 dB above and below zero. It changes the view only, never the sound.' },
        fr: { t: 'Plage d’affichage',
              b: 'L’étendue verticale de l’affichage de la réponse\u00A0: 6, 12 ou 30\u00A0dB au-dessus et au-dessous de zéro. Cela ne change que la vue, jamais le son.',
              reviewed: false },
        'zh-Hans': { t: '显示范围',
              b: '响应显示的纵向范围：零线上下各 6、12 或 30 dB。只改变视图，不改变声音。',
              reviewed: 'mt' },
    },
    'bands': {
        en: { t: 'Bands',
              b: 'The 24 bands. Click a band to select it. Click an empty slot to add that band at its stored settings.' },
        fr: { t: 'Bandes',
              b: 'Les 24 bandes. Cliquez sur une bande pour la sélectionner. Cliquez sur un emplacement vide pour ajouter cette bande avec ses réglages enregistrés.',
              reviewed: false },
        'zh-Hans': { t: '频段',
              b: '共 24 个频段。点击某个频段将其选中。点击空位可按其已存储的设置添加该频段。',
              reviewed: 'mt' },
    },
    'preset': {
        en: { t: 'Presets',
              b: 'Step through the presets with the arrows, or save the current settings to a file. The analyzer mode and the window size are not part of a preset.' },
        fr: { t: 'Préréglages',
              b: 'Parcourez les préréglages avec les flèches, ou enregistrez les réglages actuels dans un fichier. Le mode de l’analyseur et la taille de la fenêtre ne font pas partie d’un préréglage.',
              reviewed: false },
        'zh-Hans': { t: '预设',
              b: '用箭头逐个切换预设，或将当前设置保存到文件。频谱分析模式和窗口大小不属于预设内容。',
              reviewed: 'mt' },
    },
    'meter': {
        en: { t: 'Output level',
              b: 'Peak level after the output gain and the bypass, from -60 to +6 dB. Above 0 dB the bar turns red.' },
        fr: { t: 'Niveau de sortie',
              b: 'Niveau de crête après le gain de sortie et le contournement, de \u221260 à +6\u00A0dB. Au-dessus de 0\u00A0dB, la barre passe au rouge.',
              reviewed: false },
        'zh-Hans': { t: '输出电平',
              b: '输出增益和旁通之后的峰值电平，范围 -60 至 +6 dB。超过 0 dB 时电平条变为红色。',
              reviewed: 'mt' },
    },
    'resize': {
        en: { t: 'Resize',
              b: 'Drag to resize the window, from 800 x 500 to 1600 x 1000. The response display takes the extra space.' },
        fr: { t: 'Redimensionner',
              b: 'Faites glisser pour redimensionner la fenêtre, de 800 x 500 à 1600 x 1000. L’affichage de la réponse occupe l’espace supplémentaire.',
              reviewed: false },
        'zh-Hans': { t: '调整大小',
              b: '拖动以调整窗口大小，范围 800 x 500 至 1600 x 1000。多出的空间由响应显示占用。',
              reviewed: 'mt' },
    },

    // ── Selected-band strip ─────────────────────────────────────────────────
    'band-type': {
        en: { t: 'Type',
              b: 'The filter shape of the selected band: bell, shelves, cuts, notch, band pass or tilt. The change is crossfaded, so it does not click.' },
        fr: { t: 'Type',
              b: 'La forme du filtre de la bande sélectionnée\u00A0: cloche, plateaux, coupes, réjecteur, passe-bande ou inclinaison. Le changement se fait en fondu, sans clic.',
              reviewed: false, sameAsEn: true },
        'zh-Hans': { t: '类型',
              b: '所选频段的滤波器形状：钟形、搁架、切除、陷波、带通或倾斜。切换时采用交叉淡化，不会产生咔嗒声。',
              reviewed: 'mt' },
    },
    'band-freq': {
        en: { t: 'Frequency',
              b: 'Centre or corner frequency of the selected band, from 10 Hz to 30 kHz. Drag (Shift for fine), scroll, double-click to reset, or click to type a value; "2.5k" is accepted.' },
        fr: { t: 'Fréquence',
              b: 'Fréquence centrale ou de coupure de la bande sélectionnée, de 10\u00A0Hz à 30\u00A0kHz. Faites glisser (Maj pour un réglage fin), utilisez la molette, double-cliquez pour réinitialiser, ou cliquez pour saisir une valeur\u00A0; «\u00A02.5k\u00A0» est accepté.',
              reviewed: false },
        'zh-Hans': { t: '频率',
              b: '所选频段的中心频率或转折频率，范围 10 Hz 至 30 kHz。可拖动（按住 Shift 微调）、滚动、双击复位，或点击后输入数值；可输入“2.5k”。',
              reviewed: 'mt' },
    },
    'band-gain': {
        en: { t: 'Gain',
              b: 'Gain of the selected band, from -30 to +30 dB. It applies to bell, shelf and tilt shapes; cuts, notch and band pass ignore it.' },
        fr: { t: 'Gain',
              b: 'Gain de la bande sélectionnée, de \u221230 à +30\u00A0dB. Il s’applique aux formes en cloche, en plateau et inclinées\u00A0; les coupes, le réjecteur et le passe-bande l’ignorent.',
              reviewed: false, sameAsEn: true },
        'zh-Hans': { t: '增益',
              b: '所选频段的增益，范围 -30 至 +30 dB。适用于钟形、搁架和倾斜形状；切除、陷波和带通会忽略它。',
              reviewed: 'mt' },
    },
    'band-q': {
        en: { t: 'Q',
              b: 'Bandwidth of a bell, notch or band pass, from 0.025 to 40. On cuts, shelves and tilt it is the resonance at the corner: 1.0 is flat. Scrolling on a node also changes it.' },
        fr: { t: 'Q',
              b: 'Largeur de bande d’une cloche, d’un réjecteur ou d’un passe-bande, de 0,025 à 40. Pour les coupes, les plateaux et l’inclinaison, c’est la résonance à la fréquence de coupure\u00A0: 1,0 est plat. La molette sur un point la modifie aussi.',
              reviewed: false, sameAsEn: true },
        'zh-Hans': { t: 'Q 值',
              b: '钟形、陷波或带通的带宽，范围 0.025 至 40。对于切除、搁架和倾斜形状，它是转折处的共振：1.0 为平直。在节点上滚动也可调节。',
              reviewed: 'mt' },
    },
    'band-slope': {
        en: { t: 'Slope',
              b: 'Steepness of cuts, shelves and tilt, from 6 to 96 dB per octave. Bell, notch and band pass ignore it.' },
        fr: { t: 'Pente',
              b: 'Raideur des coupes, des plateaux et de l’inclinaison, de 6 à 96\u00A0dB par octave. La cloche, le réjecteur et le passe-bande l’ignorent.',
              reviewed: false },
        'zh-Hans': { t: '斜率',
              b: '切除、搁架和倾斜形状的陡峭程度，范围每倍频程 6 至 96 dB。钟形、陷波和带通会忽略它。',
              reviewed: 'mt' },
    },
    'band-placement': {
        en: { t: 'Placement',
              b: 'Which part of the signal the selected band processes: both channels (LR), left only, right only, the mid signal or the side signal. It has no effect on a mono track.' },
        fr: { t: 'Placement',
              b: 'La partie du signal traitée par la bande sélectionnée\u00A0: les deux canaux (LR), la gauche seule, la droite seule, le signal du milieu ou le signal des côtés. Sans effet sur une piste mono.',
              reviewed: false, sameAsEn: true },
        'zh-Hans': { t: '声道位置',
              b: '所选频段处理信号的哪一部分：左右两个声道（LR）、仅左、仅右、中置信号或侧向信号。在单声道音轨上不起作用。',
              reviewed: 'mt' },
    },
    'band-bypass': {
        en: { t: 'Bypass band',
              b: 'Mutes the effect of the selected band without removing it. Its node and curve stay on the display, drawn hollow.' },
        fr: { t: 'Contournement de la bande',
              b: 'Coupe l’effet de la bande sélectionnée sans la supprimer. Son point et sa courbe restent affichés, en tracé évidé.',
              reviewed: false },
        'zh-Hans': { t: '频段旁通',
              b: '关闭所选频段的效果，但不删除它。其节点和曲线仍留在显示中，以空心方式绘制。',
              reviewed: 'mt' },
    },
    'band-listen': {
        en: { t: 'Listen',
              b: 'Auditions the region the selected band works on, one band at a time. It is not saved with the session.' },
        fr: { t: 'Écoute',
              b: 'Fait entendre la zone sur laquelle agit la bande sélectionnée, une bande à la fois. Ce réglage n’est pas enregistré avec la session.',
              reviewed: false },
        'zh-Hans': { t: '监听',
              b: '单独试听所选频段作用的区域，每次只能监听一个频段。该状态不随工程保存。',
              reviewed: 'mt' },
    },
    'band-delete': {
        en: { t: 'Delete band',
              b: 'Removes the selected band. Delete or Backspace does the same. The band keeps its settings and comes back with them if you add it again from its slot.' },
        fr: { t: 'Supprimer la bande',
              b: 'Retire la bande sélectionnée. Les touches Suppr ou Retour arrière font de même. La bande conserve ses réglages et les retrouve si vous l’ajoutez de nouveau depuis son emplacement.',
              reviewed: false },
        'zh-Hans': { t: '删除频段',
              b: '移除所选频段。按 Delete 或退格键效果相同。该频段会保留其设置，从其空位重新添加时会恢复。',
              reviewed: 'mt' },
    },
});

// Visible captions and accessible names: title only.
export const LABELS = Object.freeze({
    'ui.on':            { en: { t: 'On' },  fr: { t: 'Marche', reviewed: false }, 'zh-Hans': { t: '开', reviewed: 'mt' } },
    'ui.off':           { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: false }, 'zh-Hans': { t: '关', reviewed: 'mt' } },
    'label.hoverHelp':  { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: false }, 'zh-Hans': { t: '悬停帮助', reviewed: 'mt' } },
    'aria.langSelect':  { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false }, 'zh-Hans': { t: '界面语言', reviewed: 'mt' } },
    'aria.helpToggle':  { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: false }, 'zh-Hans': { t: '开关悬停帮助', reviewed: 'mt' } },

    // ── Section captions ────────────────────────────────────────────────────
    'label.output':     { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false }, 'zh-Hans': { t: '输出', reviewed: 'mt' } },
    'label.band':       { en: { t: 'Band' },   fr: { t: 'Bande',  reviewed: false }, 'zh-Hans': { t: '频段', reviewed: 'mt' } },
    'label.save':       { en: { t: 'Save' },   fr: { t: 'Enreg.', reviewed: false }, 'zh-Hans': { t: '保存', reviewed: 'mt' } },
    'label.stripHint':  { en: { t: 'Click the display to add a band' },
                          fr: { t: 'Cliquez sur l’affichage pour ajouter une bande', reviewed: false },
                          'zh-Hans': { t: '点击显示区域以添加频段', reviewed: 'mt' } },
    // The tip at the pointer when a click on the display finds all 24 bands in
    // use (CONTEXT U5). Shown through setLabel, so it is a LABELS entry.
    'bands-full':       { en: { t: 'All 24 bands are in use' },
                          fr: { t: 'Les 24 bandes sont toutes utilisées', reviewed: false },
                          'zh-Hans': { t: '全部 24 个频段均已使用', reviewed: 'mt' } },

    // ── Analyzer segments (Off is ui.off) ───────────────────────────────────
    'opt.analyzer.pre':     { en: { t: 'Pre' },      fr: { t: 'Pré',      reviewed: false }, 'zh-Hans': { t: '前',    reviewed: 'mt' } },
    'opt.analyzer.post':    { en: { t: 'Post' },     fr: { t: 'Post',     reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '后', reviewed: 'mt' } },
    'opt.analyzer.prePost': { en: { t: 'Pre+Post' }, fr: { t: 'Pré+Post', reviewed: false }, 'zh-Hans': { t: '前+后', reviewed: 'mt' } },

    // ── Filter types, in bNN_type choice order ──────────────────────────────
    'opt.type.bell':      { en: { t: 'Bell' },       fr: { t: 'Cloche',          reviewed: false }, 'zh-Hans': { t: '钟形',     reviewed: 'mt' } },
    'opt.type.lowShelf':  { en: { t: 'Low Shelf' },  fr: { t: 'Plateau grave',   reviewed: false }, 'zh-Hans': { t: '低频搁架', reviewed: 'mt' } },
    'opt.type.highShelf': { en: { t: 'High Shelf' }, fr: { t: 'Plateau aigu',    reviewed: false }, 'zh-Hans': { t: '高频搁架', reviewed: 'mt' } },
    'opt.type.lowCut':    { en: { t: 'Low Cut' },    fr: { t: 'Coupe-bas',       reviewed: false }, 'zh-Hans': { t: '低切',     reviewed: 'mt' } },
    'opt.type.highCut':   { en: { t: 'High Cut' },   fr: { t: 'Coupe-haut',      reviewed: false }, 'zh-Hans': { t: '高切',     reviewed: 'mt' } },
    'opt.type.notch':     { en: { t: 'Notch' },      fr: { t: 'Réjecteur',       reviewed: false }, 'zh-Hans': { t: '陷波',     reviewed: 'mt' } },
    'opt.type.bandPass':  { en: { t: 'Band Pass' },  fr: { t: 'Passe-bande',     reviewed: false }, 'zh-Hans': { t: '带通',     reviewed: 'mt' } },
    'opt.type.tiltShelf': { en: { t: 'Tilt Shelf' }, fr: { t: 'Plateau incliné', reviewed: false }, 'zh-Hans': { t: '倾斜搁架', reviewed: 'mt' } },

    // ── Accessible names ────────────────────────────────────────────────────
    'aria.prevPreset':  { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: false }, 'zh-Hans': { t: '上一个预设', reviewed: 'mt' } },
    'aria.nextPreset':  { en: { t: 'Next preset' },     fr: { t: 'Préréglage suivant',   reviewed: false }, 'zh-Hans': { t: '下一个预设', reviewed: 'mt' } },
    'aria.savePreset':  { en: { t: 'Save preset to file' },
                          fr: { t: 'Enregistrer un préréglage dans un fichier', reviewed: false },
                          'zh-Hans': { t: '保存预设到文件', reviewed: 'mt' } },
    'aria.response':    { en: { t: 'Frequency response. Click to add a band, drag a node for frequency and gain, scroll on a node for Q.' },
                          fr: { t: 'Réponse en fréquence. Cliquez pour ajouter une bande, faites glisser un point pour la fréquence et le gain, utilisez la molette sur un point pour le Q.', reviewed: false },
                          'zh-Hans': { t: '频率响应。点击以添加频段，拖动节点调节频率和增益，在节点上滚动调节 Q 值。', reviewed: 'mt' } },
    'aria.placement.stereo': { en: { t: 'Stereo' },        fr: { t: 'Stéréo',       reviewed: false }, 'zh-Hans': { t: '立体声',   reviewed: 'mt' } },
    'aria.placement.left':   { en: { t: 'Left channel' },  fr: { t: 'Canal gauche', reviewed: false }, 'zh-Hans': { t: '左声道',   reviewed: 'mt' } },
    'aria.placement.right':  { en: { t: 'Right channel' }, fr: { t: 'Canal droit',  reviewed: false }, 'zh-Hans': { t: '右声道',   reviewed: 'mt' } },
    'aria.placement.mid':    { en: { t: 'Mid signal' },    fr: { t: 'Signal du milieu', reviewed: false }, 'zh-Hans': { t: '中置信号', reviewed: 'mt' } },
    'aria.placement.side':   { en: { t: 'Side signal' },   fr: { t: 'Signal des côtés', reviewed: false }, 'zh-Hans': { t: '侧向信号', reviewed: 'mt' } },
});

// Text that must NOT localize. Every entry carries a reason (check-i18n [14]).
export const I18N_EXEMPT = [
    ['O-Parametric EQ', 'the product name; a product name is never translated'],
    ['Default', 'the placeholder preset name the preset manager overwrites; a preset name IS its JSON filename, so translating it breaks recall'],
    ['LR', 'placement glyph for Stereo (left + right); channel letters, as printed on every console'],
    ['L',  'placement glyph for the left channel; a channel letter'],
    ['R',  'placement glyph for the right channel; a channel letter'],
    ['M',  'placement glyph for the mid signal; M/S convention letter'],
    ['S',  'placement glyph for the side signal; M/S convention letter'],
    ['±6',  'display range in dB; a number'],
    ['±12', 'display range in dB; a number'],
    ['±30', 'display range in dB; a number'],
    ['dB',  'unit symbol'],
];

// One row per tooltip anchor. applyI18n() writes data-tip / data-tip-title from
// these; the markup authors neither.
export const TIP_BINDINGS = [
    ['#gear-btn',          'settings'],
    ['#lang-select',       'lang-select'],
    ['#tips-toggle',       'tips-toggle'],

    ['#toggle-bypass',     'toggle-bypass'],
    ['#knob-output_gain',  'knob-output_gain'],
    ['#knob-gain_scale',   'knob-gain_scale'],
    ['#toggle-auto_gain',  'toggle-auto_gain'],

    ['#seg-analyzer',      'analyzer'],
    ['#seg-range',         'display-range'],
    ['#chips',             'bands'],
    ['#preset-bar',        'preset'],
    ['#meter-mount',       'meter'],
    ['#grip',              'resize'],

    ['#combo-type',        'band-type'],
    ['#field-freq',        'band-freq'],
    ['#field-gain',        'band-gain'],
    ['#field-q',           'band-q'],
    ['#combo-slope',       'band-slope'],
    ['#seg-placement',     'band-placement'],
    ['#btn-band-bypass',   'band-bypass'],
    ['#btn-listen',        'band-listen'],
    ['#btn-delete',        'band-delete'],
];

// Copied unchanged from O-ReverseDelay's committed js/i18n.js.
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
