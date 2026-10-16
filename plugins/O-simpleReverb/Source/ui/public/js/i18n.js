/*
   This file is part of O-simpleReverb, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleReverb interface copy: English + French + Simplified
// Chinese (v3.0.0, the O-simple family page).
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion [7] enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens; this file becomes
// BinaryData::i18n_js.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n [9] rejects
// any innerHTML reference here and any string literal containing "<".
//
// THE TOOLTIPS EXPLAIN WHY, not what: each says what the control does to the
// sound and what to watch in the wells. Character is described as what it
// is — a tilt AFTER the tank — never as in-loop damping (Q3; that is fixed per type).
//
// GLOSSARY (settled before any copy was written; suite terms from
// scripts/i18n-fr-glossary.js and scripts/i18n-zh-glossary.js):
//   en               fr                  zh-Hans
//   reverb           réverbération       混响
//   tail             queue               尾音
//   decay            déclin              衰减
//   size             taille              尺寸
//   pre-delay        pré-délai           预延迟
//   early reflection réflexion précoce   早期反射
//   tap              prise               抽头
//   tank             réservoir           混响槽
//   tone             timbre              音色
//   character        caractère           特性
//   low cut          coupe-bas           低切
//   wet / dry        traité / direct     湿 / 干
//   mix              mix                 混合
//   input / output   entrée / sortie     输入 / 输出
//   preset / lesson  préréglage / leçon  预设 / 课
//   In / Out (pill)  Act. / Dés.         开 / 关   (M3, the v2 convention)
// The six type names (Booth, Room, Hall, Spring, Plate, Ambient) are the TYPE
// choice strings and stay English in every language (M2; I18N_EXEMPT).
//
// FRENCH is machine-drafted against the glossary and flagged
// `reviewed: false` throughout. Typography follows scripts/i18n-fr-lint.js:
// U+2019 apostrophes, U+00A0 before : ; ! ? % and between a number and its
// unit, U+2212 for a minus sign, decimal commas.
//
// SIMPLIFIED CHINESE is machine-drafted and flagged `reviewed: 'mt'`
// throughout. Full-width punctuation inside Han prose; one plain U+0020
// between every Latin/digit run and the Han text around it.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

// key -> { en: {t, b}, fr: {t, b, reviewed}, 'zh-Hans': {t, b, reviewed} }
//   t = tooltip title (the small-caps line), b = tooltip body.
export const I18N = Object.freeze({

    // ── The settings popover ────────────────────────────────────────────────
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this page and its hover help, turn the hover help on or off, and load a '
               + 'preset file. The language is remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisissez la langue de cette page et de ses infobulles, activez ou non les infobulles, et chargez un '
               + 'fichier de préréglage. La langue est conservée avec la session.',
              reviewed: false },
        'zh-Hans': { t: '设置',
              b: '选择本页及其悬停帮助的语言，打开或关闭悬停帮助，并载入预设文件。语言会随会话一起记住。',
              reviewed: 'mt' },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'Interface language (English, French, Simplified Chinese). The six type names stay English because '
               + 'hosts record them as automation names.' },
        fr: { t: 'Langue',
              b: 'Langue de l’interface (anglais, français, chinois simplifié). Les six noms de type restent en anglais, '
               + 'car les hôtes les enregistrent comme noms d’automation.',
              reviewed: false },
        'zh-Hans': { t: '语言',
              b: '界面语言（英语、法语、简体中文）。六个类型名称保持英文，因为宿主会把它们记录为自动化名称。',
              reviewed: 'mt' },
    },
    'tips-toggle': {
        en: { t: 'Hover help',
              b: 'Turn these explanations on or off. Off keeps the page quiet for a session; the gear and this switch '
               + 'still explain themselves so you can find the way back.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces explications. Désactivées, la page reste silencieuse pendant une séance ; '
               + 'la roue dentée et cet interrupteur continuent de s’expliquer pour retrouver le chemin.',
              reviewed: false },
        'zh-Hans': { t: '悬停帮助',
              b: '打开或关闭这些说明。关闭后页面在工作时保持安静；齿轮和这个开关仍会自我说明，方便你找回来。',
              reviewed: 'mt' },
    },
    'loadFile': {
        en: { t: 'Load file',
              b: 'Load a preset file (.json) from anywhere on disk — one an older version saved, or one a friend sent. '
               + 'It loads like any preset; save it to keep it in your list.' },
        fr: { t: 'Charger un fichier',
              b: 'Charge un fichier de préréglage (.json) depuis n’importe quel dossier — enregistré par une version '
               + 'antérieure ou envoyé par un ami. Il se charge comme tout préréglage ; enregistrez-le pour le garder dans '
               + 'votre liste.',
              reviewed: false },
        'zh-Hans': { t: '载入文件',
              b: '从磁盘任意位置载入预设文件（.json）——旧版本保存的，或朋友发来的。它像其他预设一样载入；保存后即可留在列表中。',
              reviewed: 'mt' },
    },

    // ── Header ──────────────────────────────────────────────────────────────
    'chipTail': {
        en: { t: 'Tail',
              b: 'The tail this setting makes: the type, its decay time after Decay (RT60, the time to fall 60 dB), '
               + 'and its pre-delay, the gap before anything returns.' },
        fr: { t: 'Queue',
              b: 'La queue que produit ce réglage : le type, son temps de déclin après Déclin (RT60, le temps pour '
               + 'baisser de 60 dB) et son pré-délai, le silence avant que quoi que ce soit ne revienne.',
              reviewed: false },
        'zh-Hans': { t: '尾音',
              b: '当前设定产生的尾音：类型、经衰减调整后的衰减时间（RT60，下降 60 dB 所需的时间），以及预延迟，即任何声音返回之前的空隙。',
              reviewed: 'mt' },
    },
    'presetPrev': {
        en: { t: 'Previous preset',
              b: 'Loads the previous preset: the 48 utility presets, eight per type, then your own. The lessons are in '
               + 'the list and on the rows below.' },
        fr: { t: 'Préréglage précédent',
              b: 'Charge le préréglage précédent : les 48 préréglages utilitaires, huit par type, puis les vôtres. Les '
               + 'leçons sont dans la liste et sur les rangées du bas.',
              reviewed: false },
        'zh-Hans': { t: '上一个预设',
              b: '载入上一个预设：48 个实用预设，每种类型 8 个，然后是你自己的预设。课程在列表中和下方的两排按钮上。',
              reviewed: 'mt' },
    },
    'presetName': {
        en: { t: 'Presets',
              b: 'Click to browse. Init and the ten lessons first — each one shows one idea — then the 48 utility '
               + 'presets by type, then your own. A dot means you have changed something since it loaded.' },
        fr: { t: 'Préréglages',
              b: 'Cliquez pour parcourir. Init et les dix leçons d’abord — chacune montre une idée — puis les 48 '
               + 'préréglages utilitaires par type, puis les vôtres. Un point signale une modification depuis le chargement.',
              reviewed: false },
        'zh-Hans': { t: '预设',
              b: '点击浏览。先是初始和十课——每课展示一个概念——然后是按类型排列的 48 个实用预设，最后是你自己的预设。圆点表示载入后有改动。',
              reviewed: 'mt' },
    },
    'presetNext': {
        en: { t: 'Next preset',
              b: 'Loads the next preset: the 48 utility presets, eight per type, then your own. The lessons are in the '
               + 'list and on the rows below.' },
        fr: { t: 'Préréglage suivant',
              b: 'Charge le préréglage suivant : les 48 préréglages utilitaires, huit par type, puis les vôtres. Les '
               + 'leçons sont dans la liste et sur les rangées du bas.',
              reviewed: false },
        'zh-Hans': { t: '下一个预设',
              b: '载入下一个预设：48 个实用预设，每种类型 8 个，然后是你自己的预设。课程在列表中和下方的两排按钮上。',
              reviewed: 'mt' },
    },
    'presetSave': {
        en: { t: 'Save preset',
              b: 'Saves every setting as one of your presets. A lesson or factory name cannot be reused.' },
        fr: { t: 'Enregistrer le préréglage',
              b: 'Enregistre tous les réglages dans un de vos préréglages. Un nom de leçon ou d’usine ne peut pas être '
               + 'réutilisé.',
              reviewed: false },
        'zh-Hans': { t: '保存预设',
              b: '把所有设置保存为你的预设。课程或出厂预设的名称不能重复使用。',
              reviewed: 'mt' },
    },
    'presetDelete': {
        en: { t: 'Delete preset',
              b: 'Deletes the current preset if it is one of your own. Lessons and factory presets cannot be deleted.' },
        fr: { t: 'Supprimer le préréglage',
              b: 'Supprime le préréglage actuel s’il est à vous. Les leçons et les préréglages d’usine ne peuvent pas '
               + 'être supprimés.',
              reviewed: false },
        'zh-Hans': { t: '删除预设',
              b: '若当前预设是你自己的，则将其删除。课程和出厂预设无法删除。',
              reviewed: 'mt' },
    },

    // ── The wells ───────────────────────────────────────────────────────────
    'decayWell': {
        en: { t: 'Live decay',
              b: 'The input (amber) and the reverb tail (sage) in dBFS, scrolling. The tail is measured before the Wet '
               + 'knob, so it shows at any mix. A tail falls in a straight line on a dB scale; the brass dashed line '
               + 'is the slope Decay sets — steeper is shorter. A real tail loses its highs first, so the sage line '
               + 'can fall a little faster than the brass.' },
        fr: { t: 'Déclin en direct',
              b: 'L’entrée (ambre) et la queue de réverbération (sauge) en dBFS, qui défilent. La queue est mesurée '
               + 'avant le bouton Traité, elle reste donc visible à tout mix. Une queue tombe en ligne droite sur une '
               + 'échelle en dB ; le tireté laiton est la pente fixée par Déclin — plus raide, plus court. Une vraie '
               + 'queue perd d’abord ses aigus : la ligne sauge peut tomber un peu plus vite que le laiton.',
              reviewed: false },
        'zh-Hans': { t: '实时衰减',
              b: '以 dBFS 显示滚动的输入（琥珀色）和混响尾音（鼠尾草色）。尾音在湿旋钮之前测量，因此任何混合比例下都能看到。在 dB 刻度上尾音按直线下降；黄铜虚线是衰减设定的斜率——越陡越短。真实的尾音先失去高频，所以鼠尾草色线可能比黄铜线下降得稍快一些。',
              reviewed: 'mt' },
    },
    'erWell': {
        en: { t: 'Early reflections',
              b: 'The first separate echoes, off the nearest walls, before the tail blurs together. Left above, right '
               + 'below; hollow heads are inverted. Their spacing tells your ear how big the room is, and the shaded '
               + 'pre-delay gap tells it how far away the walls are. Drag Size and watch them slide.' },
        fr: { t: 'Premières réflexions',
              b: 'Les premiers échos distincts, sur les murs les plus proches, avant que la queue ne se fonde. Gauche en '
               + 'haut, droite en bas ; têtes creuses = inversées. Leur espacement dit à l’oreille la taille de la '
               + 'pièce, et le pré-délai ombré dit à quelle distance sont les murs. Tournez Taille et regardez-les glisser.',
              reviewed: false },
        'zh-Hans': { t: '早期反射',
              b: '尾音融合之前，来自最近墙面的第一批独立回声。左声道在上，右声道在下；空心为反相。它们的间距告诉耳朵房间有多大，阴影部分的预延迟告诉耳朵墙有多远。拖动尺寸，看它们滑动。',
              reviewed: 'mt' },
    },
    'toneWell': {
        en: { t: 'Tone of the tail',
              b: 'The frequency response of the wet path after the tank: the type’s own EQ (dotted), Character’s '
               + 'tilt (amber) and the Low Cut slope (brass, a ghost while Out). The sage line is all of them together. '
               + 'The dry signal never passes through any of it.' },
        fr: { t: 'Timbre de la queue',
              b: 'La réponse en fréquence de la voie traitée après le réservoir : l’égaliseur propre au type '
               + '(pointillé), l’inclinaison du Caractère (ambre) et la pente du coupe-bas (laiton, un fantôme quand il '
               + 'est désactivé). La ligne sauge les réunit. Le signal direct n’y passe jamais.',
              reviewed: false },
        'zh-Hans': { t: '尾音音色',
              b: '混响槽之后湿信号路径的频率响应：类型自带的均衡（点线）、特性的倾斜（琥珀色）和低切斜率（黄铜色，关闭时为虚影）。鼠尾草色线是它们的总和。干信号从不经过这里。',
              reviewed: 'mt' },
    },

    // ── Signal path ─────────────────────────────────────────────────────────
    'st-input': {
        en: { t: 'Input', b: 'The stereo signal as it arrives. The dry path leaves from here, untouched.' },
        fr: { t: 'Entrée', b: 'Le signal stéréo tel qu’il arrive. La voie directe part d’ici, intacte.', reviewed: false },
        'zh-Hans': { t: '输入', b: '到达时的立体声信号。干信号从这里出发，不作处理。', reviewed: 'mt' },
    },
    'st-early': {
        en: { t: 'Early',
              b: 'The pre-delay, then the early-reflection taps (the four room types only). Lights when Type or Size moves.' },
        fr: { t: 'Précoces',
              b: 'Le pré-délai, puis les prises des réflexions précoces (les quatre types de salle seulement). S’allume '
               + 'quand Type ou Taille bouge.',
              reviewed: false },
        'zh-Hans': { t: '早期', b: '预延迟，然后是早期反射抽头（仅四种房间类型）。改动类型或尺寸时点亮。', reviewed: 'mt' },
    },
    'st-engine': {
        en: { t: 'Engine',
              b: 'The tank that makes the tail: a 16-line feedback delay network for the four rooms, Dattorro’s '
               + 'figure-8 plate, or three dispersive springs.' },
        fr: { t: 'Moteur',
              b: 'Le réservoir qui fait la queue : un réseau de 16 lignes de retard à réinjection pour les quatre '
               + 'salles, la plaque en huit de Dattorro, ou trois ressorts dispersifs.',
              reviewed: false },
        'zh-Hans': { t: '引擎', b: '产生尾音的混响槽：四种房间用 16 线反馈延迟网络，或 Dattorro 的 8 字形板，或三根色散弹簧。', reviewed: 'mt' },
    },
    'st-tone': {
        en: { t: 'Tone', b: 'Character, then Low Cut, on the wet path only.' },
        fr: { t: 'Timbre', b: 'Caractère, puis coupe-bas, sur la voie traitée seulement.', reviewed: false },
        'zh-Hans': { t: '音色', b: '特性，然后低切，只作用于湿信号。', reviewed: 'mt' },
    },
    'st-mix': {
        en: { t: 'Mix', b: 'Wet and Dry are two separate gains, summed here.' },
        fr: { t: 'Mix', b: 'Traité et Direct sont deux gains séparés, additionnés ici.', reviewed: false, sameAsEn: true },
        'zh-Hans': { t: '混合', b: '湿和干是两个独立的增益，在这里相加。', reviewed: 'mt' },
    },
    'st-output': {
        en: { t: 'Output', b: 'Dry plus wet, left and right.' },
        fr: { t: 'Sortie', b: 'Direct plus traité, gauche et droite.', reviewed: false },
        'zh-Hans': { t: '输出', b: '干加湿，左和右。', reviewed: 'mt' },
    },

    // ── The controls ────────────────────────────────────────────────────────
    'type': {
        en: { t: 'Type',
              b: 'Picks the space. Booth, Room, Hall and Ambient are one room engine — a 16-line feedback delay '
               + 'network — each with its own delays, early reflections and decay time. Spring and Plate are other '
               + 'machines, with no early reflections. Size and Decay then scale what the type chose.' },
        fr: { t: 'Type',
              b: 'Choisit l’espace. Booth, Room, Hall et Ambient sont un même moteur de salle — un réseau de 16 '
               + 'lignes de retard à réinjection — chacun avec ses retards, ses réflexions précoces et son temps de '
               + 'déclin. Spring et Plate sont d’autres machines, sans réflexion précoce. Taille et Déclin mettent '
               + 'ensuite à l’échelle ce que le type a choisi.',
              reviewed: false },
        'zh-Hans': { t: '类型',
              b: '选择空间。Booth、Room、Hall 和 Ambient 是同一个房间引擎——16 线反馈延迟网络——各有自己的延迟、早期反射和衰减时间。Spring 和 Plate 是别的机器，没有早期反射。尺寸和衰减再缩放类型所选的东西。',
              reviewed: 'mt' },
    },
    'size': {
        en: { t: 'Size',
              b: 'Moves the walls, not the tail. The early reflections and the echoes spread apart or close up — '
               + 'watch the taps slide — while the tail length stays with Decay. It is relative to the type: Booth at '
               + '100 % is still smaller than Hall at 0 %.' },
        fr: { t: 'Taille',
              b: 'Déplace les murs, pas la queue. Les réflexions précoces et les échos s’écartent ou se resserrent — '
               + 'regardez les prises glisser — tandis que la durée de la queue reste à Déclin. Elle est relative au '
               + 'type : Booth à 100 % reste plus petit que Hall à 0 %.',
              reviewed: false },
        'zh-Hans': { t: '尺寸',
              b: '移动墙壁，而不是尾音。早期反射和回声拉开或靠拢——看抽头滑动——而尾音长度仍归衰减。它相对于类型：Booth 在 100% 时仍小于 Hall 在 0% 时。',
              reviewed: 'mt' },
    },
    'decay': {
        en: { t: 'Decay',
              b: 'How long the tail lasts, as a multiple of the type’s own RT60 — the time it takes to fall 60 dB. '
               + 'The brass slope in the decay well is this number. 0.5× to 2.0×.' },
        fr: { t: 'Déclin',
              b: 'La durée de la queue, en multiple du RT60 propre au type — le temps qu’elle met à baisser de 60 dB. '
               + 'La pente laiton du puits de déclin, c’est ce chiffre. De 0,5× à 2,0×.',
              reviewed: false },
        'zh-Hans': { t: '衰减',
              b: '尾音持续多久，以该类型自身 RT60（下降 60 dB 所需的时间）的倍数计。衰减图中的黄铜斜线就是这个数。0.5× 到 2.0×。',
              reviewed: 'mt' },
    },
    'character': {
        en: { t: 'Character',
              b: 'A tilt after the tank, on the whole tail and never on the dry. Left darkens: a low-pass closing from '
               + '20 kHz to 2 kHz. Right brightens: a 4 kHz shelf up to +6 dB. It changes the colour of the tail, not '
               + 'its length. Neutral at the centre.' },
        fr: { t: 'Caractère',
              b: 'Une inclinaison après le réservoir, sur toute la queue et jamais sur le direct. À gauche, plus sombre : '
               + 'un passe-bas qui se ferme de 20 kHz à 2 kHz. À droite, plus brillant : un plateau à 4 kHz jusqu’à '
               + '+6 dB. Il change la couleur de la queue, pas sa durée. Neutre au centre.',
              reviewed: false },
        'zh-Hans': { t: '特性',
              b: '混响槽之后的倾斜，作用于整个尾音，从不影响干信号。向左变暗：低通从 20 kHz 收到 2 kHz。向右变亮：4 kHz 高架最多 +6 dB。它改变尾音的颜色，而不是长度。中间为中性。',
              reviewed: 'mt' },
    },
    'lowCut': {
        en: { t: 'Low Cut',
              b: 'A high-pass on the tail only. Low reverb energy piles up under bass and kick drum as mud; cutting it '
               + 'keeps the space while the dry keeps its low end. It does nothing while the pill reads Out. 20 to 400 Hz.' },
        fr: { t: 'Coupe-bas',
              b: 'Un passe-haut sur la seule queue. L’énergie grave de la réverbération s’accumule sous la basse et la '
               + 'grosse caisse en boue ; la couper garde l’espace et laisse au direct ses graves. Inactif tant que la '
               + 'pastille indique Dés. De 20 à 400 Hz.',
              reviewed: false },
        'zh-Hans': { t: '低切',
              b: '只作用于尾音的高通。混响的低频能量会在贝斯和底鼓下堆成浑浊；切掉它，空间还在，干信号保留低音。按钮显示“关”时不起作用。20 到 400 Hz。',
              reviewed: 'mt' },
    },
    'lpon': {
        en: { t: 'Low Cut In / Out',
              b: 'Puts the Low Cut into the tail or takes it out. While Out the knob dims, the filter is bypassed, and '
               + 'its curve in the tone well turns to a ghost.' },
        fr: { t: 'Coupe-bas act. / dés.',
              b: 'Insère le coupe-bas dans la queue ou l’en retire. Désactivé, le bouton s’estompe, le filtre est '
               + 'contourné et sa courbe dans le puits de timbre devient un fantôme.',
              reviewed: false, termNote: 'the pill engages or bypasses the filter (v2 Act./Dés.), not a signal input / output' },
        'zh-Hans': { t: '低切开 / 关',
              b: '把低切接入或移出尾音。关闭时旋钮变暗、滤波器被旁通，音色图中的曲线变成虚影。',
              reviewed: 'mt', termNote: 'the pill engages or bypasses the filter (v2 开/关), not a signal input / output' },
    },
    'wet': {
        en: { t: 'Wet',
              b: 'How much reverb joins the output. An independent gain, not a crossfade: raising Wet never lowers Dry. '
               + '0 to 100 %.' },
        fr: { t: 'Traité',
              b: 'La quantité de réverbération ajoutée à la sortie. Un gain indépendant, pas un fondu : monter Traité ne '
               + 'baisse jamais Direct. De 0 à 100 %.',
              reviewed: false },
        'zh-Hans': { t: '湿', b: '有多少混响加入输出。独立增益，不是交叉淡化：调高湿从不调低干。0 到 100%。', reviewed: 'mt' },
    },
    'dry': {
        en: { t: 'Dry',
              b: 'How much of the untouched source passes. On an insert leave it at 100; on a send bus set it to 0 so '
               + 'only the reverb comes back. 0 to 100 %.' },
        fr: { t: 'Direct',
              b: 'La part de source intacte qui passe. En insert, laissez 100 ; sur un bus de départ, mettez 0 pour ne '
               + 'renvoyer que la réverbération. De 0 à 100 %.',
              reviewed: false },
        'zh-Hans': { t: '干', b: '有多少未处理的原声通过。插入时保持 100；在发送总线上设为 0，只返回混响。0 到 100%。', reviewed: 'mt' },
    },
    'outMeter': {
        en: { t: 'Output Peak',
              b: 'Left and right peaks at the output, dry plus wet. The lamp above a bar lights on a clip.' },
        fr: { t: 'Crête de sortie',
              b: 'Crêtes gauche et droite en sortie, direct plus traité. Le voyant au-dessus d’une barre s’allume à '
               + 'l’écrêtage.',
              reviewed: false },
        'zh-Hans': { t: '输出峰值', b: '输出端左右峰值，干加湿。削波时条形上方的灯亮起。', reviewed: 'mt' },
    },

    // ── The lessons (title = the button, body = what to watch) ──────────────
    'lesson.booth': {
        en: { t: 'Booth', b: '0.4 s, every tap inside 7 ms after a 3 ms pre-delay. Close and dry: a voice-over room. '
               + 'Watch the decay well: the steepest slope of the six.' },
        fr: { t: 'Booth', b: '0,4 s, toutes les prises en 7 ms après 3 ms de pré-délai. Proche et sec : une cabine de '
               + 'voix off. Regardez le puits de déclin : la pente la plus raide des six.', reviewed: false, sameAsEn: true },
        'zh-Hans': { t: 'Booth', b: '0.4 秒，所有抽头都在 3 ms 预延迟之后的 7 ms 内。贴近而干，像配音间。看衰减图：六种中最陡的斜率。', reviewed: 'mt', sameAsEn: true },
    },
    'lesson.room': {
        en: { t: 'Room', b: '1.1 s; the taps spread over 23 ms after 15 ms of pre-delay. The everyday space: a voice in '
               + 'a living room.' },
        fr: { t: 'Room', b: '1,1 s ; les prises s’étalent sur 23 ms après 15 ms de pré-délai. L’espace de tous les '
               + 'jours : une voix dans un salon.', reviewed: false, sameAsEn: true },
        'zh-Hans': { t: 'Room', b: '1.1 秒；抽头在 15 ms 预延迟之后分布在 23 ms 内。日常空间：客厅里的人声。', reviewed: 'mt', sameAsEn: true },
    },
    'lesson.hall': {
        en: { t: 'Hall', b: '3 s, at Size 70. A 50 ms gap before the first echo tells your ear the walls are far; the '
               + 'taps then spread out to about 60 ms.' },
        fr: { t: 'Hall', b: '3 s, à Taille 70. Un silence de 50 ms avant le premier écho dit à l’oreille que les murs '
               + 'sont loin ; les prises s’étalent ensuite jusqu’à 60 ms environ.', reviewed: false, sameAsEn: true },
        'zh-Hans': { t: 'Hall', b: '3 秒，尺寸 70。第一个回声之前 50 ms 的空隙告诉耳朵墙很远；抽头随后分布到约 60 ms。', reviewed: 'mt', sameAsEn: true },
    },
    'lesson.spring': {
        en: { t: 'Spring', b: 'No taps: three springs, each returning its own echo at 33, 37 and 41 ms. Each echo arrives '
               + 'lows first, so it chirps upward; little passes above 4.5 kHz.' },
        fr: { t: 'Spring', b: 'Aucune prise : trois ressorts, chacun renvoyant son propre écho à 33, 37 et 41 ms. Chaque '
               + 'écho arrive graves en tête et monte en sifflant ; peu de choses passent au-dessus de 4,5 kHz.', reviewed: false, sameAsEn: true },
        'zh-Hans': { t: 'Spring', b: '没有抽头：三根弹簧，各自在 33、37 和 41 ms 返回自己的回声。每个回声低频先到，所以向上滑；4.5 kHz 以上几乎不通过。', reviewed: 'mt', sameAsEn: true },
    },
    'lesson.plate': {
        en: { t: 'Plate', b: 'Dense from the first instant, with no taps, after an 8 ms pre-delay. An octave shimmer in '
               + 'the loop grows over the tail; Character is a little bright.' },
        fr: { t: 'Plate', b: 'Dense dès le premier instant, sans prise, après 8 ms de pré-délai. Un shimmer à l’octave '
               + 'dans la boucle grandit au fil de la queue ; le Caractère est un peu brillant.', reviewed: false, sameAsEn: true },
        'zh-Hans': { t: 'Plate', b: '在 8 ms 预延迟之后，一开始就很密，没有抽头。环路中的八度微光在尾音中逐渐增长；特性略亮。', reviewed: 'mt', sameAsEn: true },
    },
    'lesson.ambient': {
        en: { t: 'Ambient', b: '7 s, at Size 75, more wet than dry. Each tail outlasts the next note: watch them pile up '
               + 'in the decay well.' },
        fr: { t: 'Ambient', b: '7 s, à Taille 75, plus de traité que de direct. Chaque queue survit à la note suivante : '
               + 'regardez-les s’empiler dans le puits de déclin.', reviewed: false, sameAsEn: true },
        'zh-Hans': { t: 'Ambient', b: '7 秒，尺寸 75，湿多于干。每条尾音都比下一个音更长：在衰减图里看它们叠起来。', reviewed: 'mt', sameAsEn: true },
    },
    'lesson.init': {
        en: { t: 'Init', b: 'Room at its defaults: Wet 25, Dry 100, Decay 1.0×, Size 50, Character neutral, Low Cut out.' },
        fr: { t: 'Initial', b: 'Room par défaut : Traité 25, Direct 100, Déclin 1,0×, Taille 50, Caractère neutre, '
               + 'coupe-bas désactivé.', reviewed: false },
        'zh-Hans': { t: '初始', b: 'Room 默认值：湿 25，干 100，衰减 1.0×，尺寸 50，特性中性，低切关闭。', reviewed: 'mt' },
    },
    'lesson.sizeVsDecay': {
        en: { t: 'Size ≠ Decay', b: 'The smallest room with twice the decay: the taps pack into 12 ms, yet the slope stays '
               + 'long (2.2 s). Drag Size: the echoes spread, the tail time holds.' },
        fr: { t: 'Taille ≠ Déclin', b: 'La plus petite pièce avec un déclin doublé : les prises se serrent en 12 ms, mais la '
               + 'pente reste longue (2,2 s). Tournez Taille : les échos s’écartent, la durée reste.', reviewed: false },
        'zh-Hans': { t: '尺寸 ≠ 衰减', b: '最小的房间，衰减加倍：抽头挤在 12 ms 内，斜率却仍然很长（2.2 秒）。拖动尺寸：回声拉开，尾音时间不变。', reviewed: 'mt' },
    },
    'lesson.character': {
        en: { t: 'Character: dark ↔ bright', b: 'Hall with Character at −80: the whole tail darkens (a low-pass near '
               + '5.6 kHz) and its length does not move. Sweep to +80: a 4 kHz shelf lifts the top instead.' },
        fr: { t: 'Caractère : sombre ↔ brillant', b: 'Hall avec le Caractère à −80 : toute la queue s’assombrit (un '
               + 'passe-bas vers 5,6 kHz) et sa durée ne bouge pas. Allez à +80 : un plateau à 4 kHz relève les aigus.', reviewed: false },
        'zh-Hans': { t: '特性：暗 ↔ 亮', b: 'Hall，特性 −80：整条尾音变暗（约 5.6 kHz 的低通），长度不变。扫到 +80：改由 4 kHz 高架提升高频。', reviewed: 'mt' },
    },
    'lesson.lowCut': {
        en: { t: 'Low Cut', b: 'A long hall with the Low Cut at 250 Hz: the tail loses its mud and the dry keeps its bass. '
               + 'Toggle the pill to Out to hear the mud return.' },
        fr: { t: 'Coupe-bas', b: 'Un long hall avec le coupe-bas à 250 Hz : la queue perd sa boue et le direct garde ses '
               + 'basses. Désactivez la pastille pour entendre la boue revenir.', reviewed: false },
        'zh-Hans': { t: '低切', b: '一个长厅，低切设在 250 Hz：尾音去掉浑浊，干信号保留低音。把按钮切到“关”，听浑浊回来。', reviewed: 'mt' },
    },
    'lesson.insertSend': {
        en: { t: 'Insert vs Send', b: 'Wet 100, Dry 0: the setting for a send bus, where the dry already reaches the mix '
               + 'by another path. On an insert you would hear only the reverb — keep Dry at 100 there.' },
        fr: { t: 'Insert ou départ', b: 'Traité 100, Direct 0 : le réglage d’un bus de départ, où le direct rejoint déjà '
               + 'le mix par un autre chemin. En insert, vous n’entendriez que la réverbération — gardez Direct à 100.', reviewed: false },
        'zh-Hans': { t: '插入与发送', b: '湿 100，干 0：发送总线的设定，干信号已经从另一条路进入混音。插入时你只会听到混响——那里请把干保持在 100。', reviewed: 'mt' },
    },
});

export const LABELS = Object.freeze({
    // ── chrome ──────────────────────────────────────────────────────────────
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: false }, 'zh-Hans': { t: '悬停帮助', reviewed: 'mt' } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false }, 'zh-Hans': { t: '界面语言', reviewed: 'mt' } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: false }, 'zh-Hans': { t: '开关悬停帮助', reviewed: 'mt' } },
    // The switch's two faces (feminine plural: they agree with "les infobulles").
    'ui.on':  { en: { t: 'On' },  fr: { t: 'Activées', reviewed: false },    'zh-Hans': { t: '开', reviewed: 'mt' } },
    'ui.off': { en: { t: 'Off' }, fr: { t: 'Désactivées', reviewed: false }, 'zh-Hans': { t: '关', reviewed: 'mt' } },
    'label.presetFile': { en: { t: 'Preset file' }, fr: { t: 'Fichier', reviewed: false }, 'zh-Hans': { t: '预设文件', reviewed: 'mt' } },
    'label.loadFile':   { en: { t: 'Load file…' }, fr: { t: 'Charger…', reviewed: false }, 'zh-Hans': { t: '载入文件…', reviewed: 'mt' } },
    'label.version':    { en: { t: 'O-simpleReverb {v}' }, fr: { t: 'O-simpleReverb {v}', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: 'O-simpleReverb {v}', reviewed: 'mt', sameAsEn: true } },

    // ── header ──────────────────────────────────────────────────────────────
    'label.subtitle': {
        en: { t: 'Space · Tail · Tone · A Field Guide to Reverberation' },
        fr: { t: 'Espace · Queue · Timbre · Un guide de la réverbération', reviewed: false },
        'zh-Hans': { t: '空间 · 尾音 · 音色 · 混响实地指南', reviewed: 'mt' },
    },
    'label.chipHead': { en: { t: 'Tail' }, fr: { t: 'Queue', reviewed: false }, 'zh-Hans': { t: '尾音', reviewed: 'mt' } },
    'label.chipBody': { en: { t: '{type} · RT60 {rt} · pre {pre}' }, fr: { t: '{type} · RT60 {rt} · pré {pre}', reviewed: false }, 'zh-Hans': { t: '{type} · RT60 {rt} · 预延迟 {pre}', reviewed: 'mt' } },

    // ── visual band ─────────────────────────────────────────────────────────
    'label.decayTitle': { en: { t: 'Live Decay' }, fr: { t: 'Déclin en direct', reviewed: false }, 'zh-Hans': { t: '实时衰减', reviewed: 'mt' } },
    'label.decayHint':  { en: { t: 'level over the last 6 seconds' }, fr: { t: 'niveau des 6 dernières secondes', reviewed: false }, 'zh-Hans': { t: '最近 6 秒的电平', reviewed: 'mt' } },
    'label.dwSub':      { en: { t: '{type} × {x}' }, fr: { t: '{type} × {x}', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '{type} × {x}', reviewed: 'mt', sameAsEn: true } },
    'label.lgInput':    { en: { t: 'input' }, fr: { t: 'entrée', reviewed: false }, 'zh-Hans': { t: '输入', reviewed: 'mt' } },
    'label.lgTail':     { en: { t: 'wet tail' }, fr: { t: 'queue traitée', reviewed: false }, 'zh-Hans': { t: '湿尾音', reviewed: 'mt' } },
    'label.lgSlope':    { en: { t: 'RT60 slope' }, fr: { t: 'pente RT60', reviewed: false }, 'zh-Hans': { t: 'RT60 斜率', reviewed: 'mt' } },
    'label.erTitle':    { en: { t: 'Early Reflections' }, fr: { t: 'Premières réflexions', reviewed: false }, 'zh-Hans': { t: '早期反射', reviewed: 'mt' } },
    'label.erHint':     { en: { t: 'L above · R below' }, fr: { t: 'G dessus · D dessous', reviewed: false }, 'zh-Hans': { t: '左在上 · 右在下', reviewed: 'mt' } },
    'label.unitPre':    { en: { t: 'pre-delay' }, fr: { t: 'pré-délai', reviewed: false }, 'zh-Hans': { t: '预延迟', reviewed: 'mt' } },
    'label.erSub':      { en: { t: '8 taps a side over {span}' }, fr: { t: '8 prises par côté sur {span}', reviewed: false }, 'zh-Hans': { t: '每侧 8 个抽头，跨 {span}', reviewed: 'mt' } },
    'label.erSubNone':  { en: { t: 'no taps' }, fr: { t: 'aucune prise', reviewed: false }, 'zh-Hans': { t: '无抽头', reviewed: 'mt' } },
    'label.erSubSpring':{ en: { t: 'no taps · the springs’ own echoes' }, fr: { t: 'aucune prise · les échos des ressorts', reviewed: false }, 'zh-Hans': { t: '无抽头 · 弹簧自身的回声', reviewed: 'mt' } },
    'label.lgLeft':     { en: { t: 'left' }, fr: { t: 'gauche', reviewed: false }, 'zh-Hans': { t: '左', reviewed: 'mt' } },
    'label.lgRight':    { en: { t: 'right' }, fr: { t: 'droite', reviewed: false }, 'zh-Hans': { t: '右', reviewed: 'mt' } },
    'label.lgInverted': { en: { t: 'inverted' }, fr: { t: 'inversée', reviewed: false }, 'zh-Hans': { t: '反相', reviewed: 'mt' } },
    'label.erNone1':    { en: { t: 'No early reflections' }, fr: { t: 'Aucune réflexion précoce', reviewed: false }, 'zh-Hans': { t: '没有早期反射', reviewed: 'mt' } },
    'label.erPlate2':   { en: { t: 'the plate is dense from the first instant' }, fr: { t: 'la plaque est dense dès le premier instant', reviewed: false }, 'zh-Hans': { t: '板一开始就很密', reviewed: 'mt' } },
    'label.erSpring2':  { en: { t: 'the springs’ own echoes start at {ms}' }, fr: { t: 'les échos des ressorts commencent à {ms}', reviewed: false }, 'zh-Hans': { t: '弹簧自身的回声从 {ms} 开始', reviewed: 'mt' } },
    'label.toneTitle':  { en: { t: 'Tone of the Tail' }, fr: { t: 'Timbre de la queue', reviewed: false }, 'zh-Hans': { t: '尾音音色', reviewed: 'mt' } },
    'label.toneHint':   { en: { t: 'after the tank, wet only' }, fr: { t: 'après le réservoir, traité seul', reviewed: false }, 'zh-Hans': { t: '混响槽之后，仅湿信号', reviewed: 'mt' } },
    'label.twFlat':     { en: { t: 'flat' }, fr: { t: 'plat', reviewed: false }, 'zh-Hans': { t: '平直', reviewed: 'mt' } },
    'label.twLp':       { en: { t: 'LP {f}' }, fr: { t: 'PB {f}', reviewed: false }, 'zh-Hans': { t: '低通 {f}', reviewed: 'mt' } },
    'label.twHp':       { en: { t: 'HP {f}' }, fr: { t: 'PH {f}', reviewed: false }, 'zh-Hans': { t: '高通 {f}', reviewed: 'mt' } },
    'label.twShelf':    { en: { t: '{db} @ 4 kHz' }, fr: { t: '{db} à 4 kHz', reviewed: false }, 'zh-Hans': { t: '4 kHz 处 {db}', reviewed: 'mt' } },
    'label.twEqNone':   { en: { t: '{type} EQ · none' }, fr: { t: 'Égaliseur {type} · aucun', reviewed: false }, 'zh-Hans': { t: '{type} 均衡 · 无', reviewed: 'mt' } },
    'label.twEqHp':     { en: { t: '{type} EQ · high-pass {f}' }, fr: { t: 'Égaliseur {type} · passe-haut {f}', reviewed: false }, 'zh-Hans': { t: '{type} 均衡 · 高通 {f}', reviewed: 'mt' } },
    'label.twEqPk':     { en: { t: '{type} EQ · {g} at {f}' }, fr: { t: 'Égaliseur {type} · {g} à {f}', reviewed: false }, 'zh-Hans': { t: '{type} 均衡 · {f} 处 {g}', reviewed: 'mt' } },
    'label.twEqHs':     { en: { t: '{type} EQ · {g} above {f}' }, fr: { t: 'Égaliseur {type} · {g} dès {f}', reviewed: false }, 'zh-Hans': { t: '{type} 均衡 · {f} 以上 {g}', reviewed: 'mt' } },
    'label.lgResponse': { en: { t: 'response' }, fr: { t: 'réponse', reviewed: false }, 'zh-Hans': { t: '响应', reviewed: 'mt' } },
    'label.lgCharacter':{ en: { t: 'character' }, fr: { t: 'caractère', reviewed: false }, 'zh-Hans': { t: '特性', reviewed: 'mt' } },
    'label.lgLowCut':   { en: { t: 'low cut' }, fr: { t: 'coupe-bas', reviewed: false }, 'zh-Hans': { t: '低切', reviewed: 'mt' } },
    'label.springBand': { en: { t: 'springs pass little above 4.5 kHz' }, fr: { t: 'les ressorts passent peu au-dessus de 4,5 kHz', reviewed: false }, 'zh-Hans': { t: '弹簧几乎不通过 4.5 kHz 以上', reviewed: 'mt' } },

    // ── signal path ─────────────────────────────────────────────────────────
    'label.stInput':      { en: { t: 'Input' }, fr: { t: 'Entrée', reviewed: false }, 'zh-Hans': { t: '输入', reviewed: 'mt' } },
    'label.stEarly':      { en: { t: 'Early' }, fr: { t: 'Précoces', reviewed: false }, 'zh-Hans': { t: '早期', reviewed: 'mt' } },
    'label.stEngine':     { en: { t: 'Engine' }, fr: { t: 'Moteur', reviewed: false }, 'zh-Hans': { t: '引擎', reviewed: 'mt' } },
    'label.stTone':       { en: { t: 'Tone' }, fr: { t: 'Timbre', reviewed: false }, 'zh-Hans': { t: '音色', reviewed: 'mt' } },
    'label.stMix':        { en: { t: 'Mix' }, fr: { t: 'Mix', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '混合', reviewed: 'mt' } },
    'label.stOutput':     { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false }, 'zh-Hans': { t: '输出', reviewed: 'mt' } },
    'label.lr':           { en: { t: 'L / R' }, fr: { t: 'G / D', reviewed: false }, 'zh-Hans': { t: '左 / 右', reviewed: 'mt' } },
    'label.stEarlyFdn':   { en: { t: 'pre {pre} · 8 taps' }, fr: { t: 'pré {pre} · 8 prises', reviewed: false }, 'zh-Hans': { t: '预延迟 {pre} · 8 个抽头', reviewed: 'mt' } },
    'label.stEarlyPlate': { en: { t: 'pre {pre} · no taps' }, fr: { t: 'pré {pre} · sans prise', reviewed: false }, 'zh-Hans': { t: '预延迟 {pre} · 无抽头', reviewed: 'mt' } },
    'label.stEarlySpring':{ en: { t: 'no taps · own echoes' }, fr: { t: 'sans prise · échos propres', reviewed: false }, 'zh-Hans': { t: '无抽头 · 自身回声', reviewed: 'mt' } },
    'label.stEngFdn':     { en: { t: '16 lines · Hadamard' }, fr: { t: '16 lignes · Hadamard', reviewed: false }, 'zh-Hans': { t: '16 条线 · Hadamard', reviewed: 'mt' } },
    'label.stEngFdnMod':  { en: { t: '16 lines · Hadamard · slow modulation' }, fr: { t: '16 lignes · Hadamard · modulation lente', reviewed: false }, 'zh-Hans': { t: '16 条线 · Hadamard · 慢速调制', reviewed: 'mt' } },
    'label.stEngPlate':   { en: { t: 'Dattorro figure-8 · octave shimmer in the loop' }, fr: { t: 'huit de Dattorro · shimmer d’octave en boucle', reviewed: false }, 'zh-Hans': { t: 'Dattorro 8 字形 · 环路内八度微光', reviewed: 'mt' } },
    'label.stEngSpring':  { en: { t: '3 dispersive springs' }, fr: { t: '3 ressorts dispersifs', reviewed: false }, 'zh-Hans': { t: '3 根色散弹簧', reviewed: 'mt' } },
    'label.toneDark':     { en: { t: 'dark' }, fr: { t: 'sombre', reviewed: false }, 'zh-Hans': { t: '暗', reviewed: 'mt' } },
    'label.toneBright':   { en: { t: 'bright' }, fr: { t: 'brillant', reviewed: false }, 'zh-Hans': { t: '亮', reviewed: 'mt' } },
    'label.stCutIn':      { en: { t: 'low cut in' }, fr: { t: 'coupe-bas act.', reviewed: false, termNote: 'the filter is engaged (v2 Act.), not a signal input' }, 'zh-Hans': { t: '低切开', reviewed: 'mt', termNote: 'the filter is engaged (v2 开), not a signal input' } },
    'label.stCutOut':     { en: { t: 'low cut out' }, fr: { t: 'coupe-bas dés.', reviewed: false, termNote: 'the filter is bypassed (v2 Dés.), not a signal output' }, 'zh-Hans': { t: '低切关', reviewed: 'mt', termNote: 'the filter is bypassed (v2 关), not a signal output' } },
    'label.stMixSub':     { en: { t: 'wet {w} · dry {d}' }, fr: { t: 'traité {w} · direct {d}', reviewed: false }, 'zh-Hans': { t: '湿 {w} · 干 {d}', reviewed: 'mt' } },

    // ── the Engine group ────────────────────────────────────────────────────
    'label.gEngine':  { en: { t: 'Engine' }, fr: { t: 'Moteur', reviewed: false }, 'zh-Hans': { t: '引擎', reviewed: 'mt' } },
    'label.rEngine':  { en: { t: 'the space, and how long it rings' }, fr: { t: 'l’espace, et combien de temps il sonne', reviewed: false }, 'zh-Hans': { t: '空间，以及它响多久', reviewed: 'mt' } },
    'label.type':     { en: { t: 'Type' }, fr: { t: 'Type', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '类型', reviewed: 'mt' } },
    'label.engSpring':{ en: { t: 'spring' }, fr: { t: 'ressort', reviewed: false }, 'zh-Hans': { t: '弹簧', reviewed: 'mt' } },
    'label.engPlate': { en: { t: 'plate' }, fr: { t: 'plaque', reviewed: false }, 'zh-Hans': { t: '板', reviewed: 'mt' } },
    'label.size':     { en: { t: 'Size' }, fr: { t: 'Taille', reviewed: false }, 'zh-Hans': { t: '尺寸', reviewed: 'mt' } },
    'label.subSize':  { en: { t: 'walls ×{x}' }, fr: { t: 'murs ×{x}', reviewed: false }, 'zh-Hans': { t: '墙距 ×{x}', reviewed: 'mt' } },
    'label.decay':    { en: { t: 'Decay' }, fr: { t: 'Déclin', reviewed: false }, 'zh-Hans': { t: '衰减', reviewed: 'mt' } },
    'label.subDecay': { en: { t: 'RT60 {s}' }, fr: { t: 'RT60 {s}', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: 'RT60 {s}', reviewed: 'mt', sameAsEn: true } },
    'label.noteFdn':  {
        en: { t: 'one 16-line feedback delay network. Base decay {rt}, pre-delay {pre}, 8 early taps a side. Size moves the walls; the tail length stays with Decay.' },
        fr: { t: 'un réseau de 16 lignes de retard à réinjection. Déclin de base {rt}, pré-délai {pre}, 8 prises précoces par côté. Taille déplace les murs ; la durée reste à Déclin.', reviewed: false },
        'zh-Hans': { t: '一个 16 线反馈延迟网络。基础衰减 {rt}，预延迟 {pre}，每侧 8 个早期抽头。尺寸移动墙壁；尾音长度归衰减管。', reviewed: 'mt' },
    },
    'label.notePlate': {
        en: { t: 'Dattorro’s figure-8 tank: dense from the first instant, no early taps, pre-delay {pre}. An octave shimmer inside the loop grows over the tail. Base decay {rt}.' },
        fr: { t: 'le réservoir en huit de Dattorro : dense dès le premier instant, sans prise précoce, pré-délai {pre}. Un shimmer à l’octave dans la boucle grandit au fil de la queue. Déclin de base {rt}.', reviewed: false },
        'zh-Hans': { t: 'Dattorro 的 8 字形混响槽：一开始就很密，没有早期抽头，预延迟 {pre}。环路内的八度微光在尾音中逐渐增长。基础衰减 {rt}。', reviewed: 'mt' },
    },
    'label.noteSpring': {
        en: { t: 'three dispersive springs. Each echo arrives lows first, so it chirps upward; little passes above 4.5 kHz. No pre-delay: the first sound out is each spring’s own echo. Base decay {rt}.' },
        fr: { t: 'trois ressorts dispersifs. Chaque écho arrive graves en tête et monte en sifflant ; peu de choses passent au-dessus de 4,5 kHz. Pas de pré-délai : le premier son est l’écho propre de chaque ressort. Déclin de base {rt}.', reviewed: false },
        'zh-Hans': { t: '三根色散弹簧。每个回声低频先到，因此向上滑；4.5 kHz 以上几乎不通过。没有预延迟：最先出来的是每根弹簧自己的回声。基础衰减 {rt}。', reviewed: 'mt' },
    },

    // ── the Tone group ──────────────────────────────────────────────────────
    'label.gTone':      { en: { t: 'Tone' }, fr: { t: 'Timbre', reviewed: false }, 'zh-Hans': { t: '音色', reviewed: 'mt' } },
    'label.rTone':      { en: { t: 'colours the tail only' }, fr: { t: 'colore la queue seule', reviewed: false }, 'zh-Hans': { t: '只染色尾音', reviewed: 'mt' } },
    'label.character':  { en: { t: 'Character' }, fr: { t: 'Caractère', reviewed: false }, 'zh-Hans': { t: '特性', reviewed: 'mt' } },
    'label.chNeutral':  { en: { t: 'neutral' }, fr: { t: 'neutre', reviewed: false }, 'zh-Hans': { t: '中性', reviewed: 'mt' } },
    'label.chDark':     { en: { t: 'dark {p}' }, fr: { t: 'sombre {p}', reviewed: false }, 'zh-Hans': { t: '暗 {p}', reviewed: 'mt' } },
    'label.chBright':   { en: { t: 'bright {p}' }, fr: { t: 'brillant {p}', reviewed: false }, 'zh-Hans': { t: '亮 {p}', reviewed: 'mt' } },
    'label.subChNone':  { en: { t: 'no filter' }, fr: { t: 'aucun filtre', reviewed: false }, 'zh-Hans': { t: '无滤波', reviewed: 'mt' } },
    'label.subChWarm':  { en: { t: 'low-pass {f}' }, fr: { t: 'passe-bas {f}', reviewed: false }, 'zh-Hans': { t: '低通 {f}', reviewed: 'mt' } },
    'label.subChBright':{ en: { t: 'shelf {db}' }, fr: { t: 'plateau {db}', reviewed: false }, 'zh-Hans': { t: '高架 {db}', reviewed: 'mt' } },
    'aria.lowCutOn':    { en: { t: 'Low Cut in or out' }, fr: { t: 'Coupe-bas activé ou désactivé', reviewed: false }, 'zh-Hans': { t: '低切开或关', reviewed: 'mt' } },
    // M3: the pill's faces (CSS uppercases them: IN / OUT, ACT. / DÉS., 开 / 关).
    'label.pillIn':     { en: { t: 'In' }, fr: { t: 'Act.', reviewed: false, termNote: 'the pill engages the filter (v2 ACT.), not a signal input' }, 'zh-Hans': { t: '开', reviewed: 'mt', termNote: 'the pill engages the filter (v2 开), not a signal input' } },
    'label.pillOut':    { en: { t: 'Out' }, fr: { t: 'Dés.', reviewed: false, termNote: 'the pill bypasses the filter (v2 DÉS.), not a signal output' }, 'zh-Hans': { t: '关', reviewed: 'mt', termNote: 'the pill bypasses the filter (v2 关), not a signal output' } },
    'label.lowCut':     { en: { t: 'Low Cut' }, fr: { t: 'Coupe-bas', reviewed: false }, 'zh-Hans': { t: '低切', reviewed: 'mt' } },
    'label.subCutIn':   { en: { t: 'mud out of the tail' }, fr: { t: 'la queue sans boue', reviewed: false }, 'zh-Hans': { t: '去掉尾音浑浊', reviewed: 'mt' } },
    'label.subCutOut':  { en: { t: 'bypassed' }, fr: { t: 'contourné', reviewed: false }, 'zh-Hans': { t: '已旁通', reviewed: 'mt' } },
    'label.noteTone':   { en: { t: 'the dry signal is never filtered' }, fr: { t: 'le signal direct n’est jamais filtré', reviewed: false }, 'zh-Hans': { t: '干信号从不被滤波', reviewed: 'mt' } },

    // ── the Mix and Output groups ───────────────────────────────────────────
    'label.gMix':       { en: { t: 'Mix' }, fr: { t: 'Mix', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '混合', reviewed: 'mt' } },
    'label.rMix':       { en: { t: 'two gains, not a crossfade' }, fr: { t: 'deux gains, pas un fondu', reviewed: false }, 'zh-Hans': { t: '两个增益，非交叉淡化', reviewed: 'mt' } },
    'label.wet':        { en: { t: 'Wet' }, fr: { t: 'Traité', reviewed: false }, 'zh-Hans': { t: '湿', reviewed: 'mt' } },
    'label.subWet':     { en: { t: 'reverb {db}' }, fr: { t: 'réverb {db}', reviewed: false }, 'zh-Hans': { t: '混响 {db}', reviewed: 'mt' } },
    'label.subWetOff':  { en: { t: 'no reverb' }, fr: { t: 'pas de réverb', reviewed: false }, 'zh-Hans': { t: '无混响', reviewed: 'mt' } },
    'label.dry':        { en: { t: 'Dry' }, fr: { t: 'Direct', reviewed: false }, 'zh-Hans': { t: '干', reviewed: 'mt' } },
    'label.subDry':     { en: { t: 'source {db}' }, fr: { t: 'source {db}', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '原声 {db}', reviewed: 'mt' } },
    'label.subDryOff':  { en: { t: 'send: tail only' }, fr: { t: 'départ : queue seule', reviewed: false }, 'zh-Hans': { t: '发送：只有尾音', reviewed: 'mt' } },
    'label.noteMix':    { en: { t: 'insert: Dry 100 · send: Dry 0' }, fr: { t: 'insert : Direct 100 · départ : 0', reviewed: false }, 'zh-Hans': { t: '插入：干 100 · 发送：干 0', reviewed: 'mt' } },
    'label.gOutput':    { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false }, 'zh-Hans': { t: '输出', reviewed: 'mt' } },
    'label.peak':       { en: { t: 'Peak · dry + wet' }, fr: { t: 'Crête · dir. + traité', reviewed: false }, 'zh-Hans': { t: '峰值 · 干 + 湿', reviewed: 'mt' } },

    // ── lesson rows and footer ──────────────────────────────────────────────
    'label.rowSpaces':  { en: { t: 'Spaces' }, fr: { t: 'Espaces', reviewed: false }, 'zh-Hans': { t: '空间', reviewed: 'mt' } },
    'label.rowIdeas':   { en: { t: 'Ideas' }, fr: { t: 'Notions', reviewed: false }, 'zh-Hans': { t: '概念', reviewed: 'mt' } },
    'label.tourHint':   { en: { t: 'hover anything for help' }, fr: { t: 'survolez pour l’aide', reviewed: false }, 'zh-Hans': { t: '悬停任意处查看帮助', reviewed: 'mt' } },
    'label.tourHint2':  { en: { t: 'click a lesson, watch the wells' }, fr: { t: 'cliquez, puis regardez', reviewed: false }, 'zh-Hans': { t: '点一课，看上方图', reviewed: 'mt' } },
    'preset.init':      { en: { t: 'Init' }, fr: { t: 'Initial', reviewed: false }, 'zh-Hans': { t: '初始', reviewed: 'mt' } },
    'label.lesson.sizeVsDecay': { en: { t: 'Size ≠ Decay' }, fr: { t: 'Taille ≠ Déclin', reviewed: false }, 'zh-Hans': { t: '尺寸 ≠ 衰减', reviewed: 'mt' } },
    'label.lesson.character':   { en: { t: 'Character: dark ↔ bright' }, fr: { t: 'Caractère : sombre ↔ brillant', reviewed: false }, 'zh-Hans': { t: '特性：暗 ↔ 亮', reviewed: 'mt' } },
    'label.lesson.lowCut':      { en: { t: 'Low Cut' }, fr: { t: 'Coupe-bas', reviewed: false }, 'zh-Hans': { t: '低切', reviewed: 'mt' } },
    'label.lesson.insertSend':  { en: { t: 'Insert vs Send' }, fr: { t: 'Insert ou départ', reviewed: false }, 'zh-Hans': { t: '插入与发送', reviewed: 'mt' } },
    'label.tourDefault': { en: { t: 'input → early reflections → engine → tone → wet + dry → output' }, fr: { t: 'entrée → réflexions précoces → moteur → timbre → traité + direct → sortie', reviewed: false }, 'zh-Hans': { t: '输入 → 早期反射 → 引擎 → 音色 → 湿 + 干 → 输出', reviewed: 'mt' } },
    'label.tourCaption.init':        { en: { t: 'Room at its defaults: Wet 25, Dry 100, Character neutral, Low Cut out.' }, fr: { t: 'Room par défaut : Traité 25, Direct 100, Caractère neutre, coupe-bas désactivé.', reviewed: false }, 'zh-Hans': { t: 'Room 默认值：湿 25，干 100，特性中性，低切关闭。', reviewed: 'mt' } },
    'label.tourCaption.booth':       { en: { t: 'Booth: 0.4 s, every tap inside 7 ms. Close and dry, a voice-over room.' }, fr: { t: 'Booth : 0,4 s, toutes les prises en 7 ms. Proche et sec, une cabine de voix off.', reviewed: false }, 'zh-Hans': { t: 'Booth：0.4 秒，所有抽头都在 7 ms 内。贴近而干，像配音间。', reviewed: 'mt' } },
    'label.tourCaption.room':        { en: { t: 'Room: 1.1 s and 15 ms of pre-delay; the taps spread over 23 ms.' }, fr: { t: 'Room : 1,1 s et 15 ms de pré-délai ; les prises s’étalent sur 23 ms.', reviewed: false }, 'zh-Hans': { t: 'Room：1.1 秒，预延迟 15 ms；抽头分布在 23 ms 内。', reviewed: 'mt' } },
    'label.tourCaption.hall':        { en: { t: 'Hall: 3 s and 50 ms of pre-delay. The gap tells your ear the walls are far.' }, fr: { t: 'Hall : 3 s et 50 ms de pré-délai. Ce silence dit à l’oreille que les murs sont loin.', reviewed: false }, 'zh-Hans': { t: 'Hall：3 秒，预延迟 50 ms。这段空隙告诉耳朵墙很远。', reviewed: 'mt' } },
    'label.tourCaption.spring':      { en: { t: 'Spring: no taps; each spring’s echo sweeps low to high, little above 4.5 kHz.' }, fr: { t: 'Spring : aucune prise ; l’écho de chaque ressort glisse du grave à l’aigu, peu au-dessus de 4,5 kHz.', reviewed: false }, 'zh-Hans': { t: 'Spring：没有抽头；每根弹簧的回声从低滑到高，4.5 kHz 以上很少。', reviewed: 'mt' } },
    'label.tourCaption.plate':       { en: { t: 'Plate: dense at once, no taps; an octave shimmer grows in the tail.' }, fr: { t: 'Plate : dense d’emblée, sans prise ; un shimmer à l’octave grandit dans la queue.', reviewed: false }, 'zh-Hans': { t: 'Plate：一开始就密，没有抽头；八度微光在尾音中增长。', reviewed: 'mt' } },
    'label.tourCaption.ambient':     { en: { t: 'Ambient: 7 s. Each tail outlasts the next note; watch them pile up.' }, fr: { t: 'Ambient : 7 s. Chaque queue survit à la note suivante ; regardez-les s’empiler.', reviewed: false }, 'zh-Hans': { t: 'Ambient：7 秒。每条尾音都比下一个音更长；看它们叠起来。', reviewed: 'mt' } },
    'label.tourCaption.sizeVsDecay': { en: { t: 'Smallest room, double decay: the taps close up, the slope stays long. Size ≠ Decay.' }, fr: { t: 'La plus petite pièce, déclin doublé : les prises se serrent, la pente reste longue. Taille ≠ Déclin.', reviewed: false }, 'zh-Hans': { t: '最小的房间，衰减加倍：抽头靠拢，斜率仍然很长。尺寸 ≠ 衰减。', reviewed: 'mt' } },
    'label.tourCaption.character':   { en: { t: 'Character to the left: the whole tail tilts dark; its length and the dry stay as they were.' }, fr: { t: 'Caractère vers la gauche : toute la queue penche vers le sombre ; sa durée et le direct ne changent pas.', reviewed: false }, 'zh-Hans': { t: '特性向左：整条尾音向暗倾斜；它的长度和干信号都不变。', reviewed: 'mt' } },
    'label.tourCaption.lowCut':      { en: { t: 'Low Cut at 250 Hz: the tail loses its mud, the dry keeps its bass. Set it Out to compare.' }, fr: { t: 'Coupe-bas à 250 Hz : la queue perd sa boue, le direct garde ses basses. Désactivez-le pour comparer.', reviewed: false }, 'zh-Hans': { t: '低切 250 Hz：尾音去掉浑浊，干信号保留低音。切到“关”对比一下。', reviewed: 'mt' } },
    'label.tourCaption.insertSend':  { en: { t: 'Wet 100, Dry 0: the setting for a send bus. On an insert, keep Dry at 100.' }, fr: { t: 'Traité 100, Direct 0 : le réglage d’un bus de départ. En insert, gardez Direct à 100.', reviewed: false }, 'zh-Hans': { t: '湿 100，干 0：发送总线的设定。插入时，干保持 100。', reviewed: 'mt' } },
    'label.footerRight': { en: { t: 'FDN · Dattorro plate · dispersive spring' }, fr: { t: 'FDN · plaque Dattorro · ressort dispersif', reviewed: false }, 'zh-Hans': { t: 'FDN · Dattorro 板 · 色散弹簧', reviewed: 'mt' } },

    // ── preset bar ──────────────────────────────────────────────────────────
    'label.presetSave':       { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: false }, 'zh-Hans': { t: '保存', reviewed: 'mt' } },
    'label.presetDelete':     { en: { t: 'Delete' }, fr: { t: 'Suppr.', reviewed: false }, 'zh-Hans': { t: '删除', reviewed: 'mt' } },
    'label.groupLessons':     { en: { t: 'Lessons' }, fr: { t: 'Leçons', reviewed: false }, 'zh-Hans': { t: '课程', reviewed: 'mt' } },
    'label.groupUser':        { en: { t: 'Your presets' }, fr: { t: 'Vos préréglages', reviewed: false }, 'zh-Hans': { t: '你的预设', reviewed: 'mt' } },
    'preset.saveTitle':       { en: { t: 'Save preset' }, fr: { t: 'Enregistrer le préréglage', reviewed: false }, 'zh-Hans': { t: '保存预设', reviewed: 'mt' } },
    'preset.namePlaceholder': { en: { t: 'Preset name' }, fr: { t: 'Nom du préréglage', reviewed: false }, 'zh-Hans': { t: '预设名称', reviewed: 'mt' } },
    'preset.ok':              { en: { t: 'OK' }, fr: { t: 'OK', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '确定', reviewed: 'mt' } },
    'preset.cancel':          { en: { t: 'Cancel' }, fr: { t: 'Annuler', reviewed: false }, 'zh-Hans': { t: '取消', reviewed: 'mt' } },
    'preset.errEmpty':        { en: { t: 'Type a name first.' }, fr: { t: 'Saisissez d’abord un nom.', reviewed: false }, 'zh-Hans': { t: '请先输入名称。', reviewed: 'mt' } },
    'preset.errFactoryName':  { en: { t: 'A lesson or factory preset has that name — choose another.' }, fr: { t: 'Nom d’une leçon ou d’un préréglage d’usine — choisissez-en un autre.', reviewed: false }, 'zh-Hans': { t: '该名称属于课程或出厂预设——请换一个。', reviewed: 'mt' } },
    'preset.errSave':         { en: { t: 'Could not save the preset.' }, fr: { t: 'Impossible d’enregistrer le préréglage.', reviewed: false }, 'zh-Hans': { t: '无法保存预设。', reviewed: 'mt' } },
    'preset.confirmReplace':  { en: { t: 'Replace “{name}”?' }, fr: { t: 'Remplacer « {name} » ?', reviewed: false }, 'zh-Hans': { t: '替换“{name}”？', reviewed: 'mt' } },
    'preset.confirmDelete':   { en: { t: 'Delete preset “{name}”?' }, fr: { t: 'Supprimer le préréglage « {name} » ?', reviewed: false }, 'zh-Hans': { t: '删除预设“{name}”？', reviewed: 'mt' } },
    'aria.presetPrev':        { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: false }, 'zh-Hans': { t: '上一个预设', reviewed: 'mt' } },
    'aria.presetName':        { en: { t: 'Browse presets' }, fr: { t: 'Parcourir les préréglages', reviewed: false }, 'zh-Hans': { t: '浏览预设', reviewed: 'mt' } },
    'aria.presetNext':        { en: { t: 'Next preset' }, fr: { t: 'Préréglage suivant', reviewed: false }, 'zh-Hans': { t: '下一个预设', reviewed: 'mt' } },
    'aria.presetSave':        { en: { t: 'Save as a user preset' }, fr: { t: 'Enregistrer comme préréglage utilisateur', reviewed: false }, 'zh-Hans': { t: '保存为用户预设', reviewed: 'mt' } },
    'aria.presetDelete':      { en: { t: 'Delete the current user preset' }, fr: { t: 'Supprimer le préréglage utilisateur actuel', reviewed: false }, 'zh-Hans': { t: '删除当前用户预设', reviewed: 'mt' } },
    'aria.presetList':        { en: { t: 'Presets' }, fr: { t: 'Préréglages', reviewed: false }, 'zh-Hans': { t: '预设', reviewed: 'mt' } },
});

// [text, reason] or [text, reason, scope]. Every entry carries a reason
// (check-i18n [14]); a scope names the element(s) the exemption is honest for.
export const I18N_EXEMPT = [
    // The h1 splits the product name across two text nodes so the second half
    // can carry the green italic .title-accent. A product name is never translated.
    ['O – simple', 'the product name, first half of the split wordmark in the page heading — a product name is never translated', '.title'],
    ['Reverb',     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated', '.title-accent'],
    ['Ouaricon Audio · O-simpleReverb', 'the colophon: company + product name — never translated', '.footer'],

    // Endonyms in the language selector: a language name is written in its own language.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
    ['简体中文', 'endonym — a language name is never translated'],

    // The TYPE AudioParameterChoice strings are the host automation contract and
    // stay English (M2): the segmented row, the six space lessons, the type note.
    ['Booth',   'AudioParameterChoice entry (TYPE) — the host automation name, English in every language', '#seg-TYPE, .tour-row, #typeNoteName'],
    ['Room',    'AudioParameterChoice entry (TYPE) — the host automation name, English in every language', '#seg-TYPE, .tour-row, #typeNoteName'],
    ['Hall',    'AudioParameterChoice entry (TYPE) — the host automation name, English in every language', '#seg-TYPE, .tour-row, #typeNoteName'],
    ['Spring',  'AudioParameterChoice entry (TYPE) — the host automation name, English in every language', '#seg-TYPE, .tour-row, #typeNoteName'],
    ['Plate',   'AudioParameterChoice entry (TYPE) — the host automation name, English in every language', '#seg-TYPE, .tour-row, #typeNoteName'],
    ['Ambient', 'AudioParameterChoice entry (TYPE) — the host automation name, English in every language', '#seg-TYPE, .tour-row, #typeNoteName'],

    // Technical abbreviations that read the same in every language.
    ['FDN',  'feedback delay network — the engine’s technical name, the same in every language', '#engTags'],
    ['RT60', 'the standard decay-time measure (time to fall 60 dB) — the same in every language', '.w-big'],
];

// One row per tooltip anchor: [selector, key]. A knob's row names its whole
// .knob-cell so the label and the readout under the knob share its tip.
export const TIP_BINDINGS = [
    ['#gear-btn',    'settings'],
    ['#lang-select', 'lang-select'],
    ['#tips-toggle', 'tips-toggle'],
    ['#loadFileBtn', 'loadFile'],

    ['#chipTail',    'chipTail'],

    ['#presetPrev',   'presetPrev'],
    ['#presetName',   'presetName'],
    ['#presetNext',   'presetNext'],
    ['#presetSave',   'presetSave'],
    ['#presetDelete', 'presetDelete'],

    ['#decayWrap', 'decayWell'],
    ['#erWrap',    'erWell'],
    ['#toneWrap',  'toneWell'],

    ['#st-input',  'st-input'],
    ['#st-early',  'st-early'],
    ['#st-engine', 'st-engine'],
    ['#st-tone',   'st-tone'],
    ['#st-mix',    'st-mix'],
    ['#st-output', 'st-output'],

    ['[data-param="TYPE"]',      'type'],
    ['[data-param="SIZE"]',      'size'],
    ['[data-param="DECAY"]',     'decay'],
    ['[data-param="CHARACTER"]', 'character'],
    ['[data-param="LPFREQ"]',    'lowCut'],
    ['#pill-LPON',               'lpon'],
    ['[data-param="WET"]',       'wet'],
    ['[data-param="DRY"]',       'dry'],
    ['#outMeterWrap',            'outMeter'],

    ['#lesson-booth',       'lesson.booth'],
    ['#lesson-room',        'lesson.room'],
    ['#lesson-hall',        'lesson.hall'],
    ['#lesson-spring',      'lesson.spring'],
    ['#lesson-plate',       'lesson.plate'],
    ['#lesson-ambient',     'lesson.ambient'],
    ['#lesson-init',        'lesson.init'],
    ['#lesson-sizeVsDecay', 'lesson.sizeVsDecay'],
    ['#lesson-character',   'lesson.character'],
    ['#lesson-lowCut',      'lesson.lowCut'],
    ['#lesson-insertSend',  'lesson.insertSend'],
];

// Copied unchanged from O-simpleAmbisonics's committed i18n.js (the canon is one shape).
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
