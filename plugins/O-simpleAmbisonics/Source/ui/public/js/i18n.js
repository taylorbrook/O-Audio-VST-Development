/*
   This file is part of O-simpleAmbisonics, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleAmbisonics interface copy: English + French + Simplified
// Chinese (mockup v1 finalization → Source/ui/public/js/i18n.js in Stage 3).
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion [7] enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens; this file becomes
// BinaryData::i18n_js (critical_binary_data_strips_hyphens).
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n [9] rejects
// any innerHTML reference here and any string literal containing "<".
//
// THE ENGLISH TOOLTIPS WERE MOVED FROM THE MOCKUP, NOT REWRITTEN. Every I18N
// `en` body below is the TIPS entry of .planning/mockups/v1-ui-test.html
// (the pedagogy the designer approved); labels are the page's own captions.
//
// FRENCH is machine-drafted against scripts/i18n-fr-glossary.js and flagged
// `reviewed: false` throughout. Typography follows scripts/i18n-fr-lint.js:
// U+2019 apostrophes, U+00A0 before : ; ! ? and between a number and its unit,
// U+2212 for a minus sign, decimal commas. Two reasoned glossary exemptions
// carry a `termNote` (rotation Pitch is `tangage`, never `hauteur`; the
// ear-level Elevation word is not a gain level).
//
// SIMPLIFIED CHINESE is machine-drafted against scripts/i18n-zh-glossary.js
// and flagged `reviewed: 'mt'` throughout (below the 'bt' ship bar — Stage 3.3
// back-translates). Full-width punctuation inside Han prose; one plain U+0020
// between every Latin/digit run and the Han text around it.
//
// Choice strings (Binaural / Stereo, Mono sum / Left only, 1 / 2 / 3) stay
// English everywhere: they are the host's automation names (I18N_EXEMPT).
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

// key -> { en: {t, b}, fr: {t, b, reviewed}, 'zh-Hans': {t, b, reviewed} }
//   t = tooltip title (the small-caps line), b = tooltip body.
// Object.freeze(): inert data, one top-level statement.
export const I18N = Object.freeze({

    // ── The settings popover ────────────────────────────────────────────────
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this page and its hover help, and whether the hover help appears at all. '
               + 'The choice is remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisissez la langue de cette page et de ses infobulles, et leur affichage ou non. '
               + 'Le choix est conservé avec la session.',
              reviewed: false },
        'zh-Hans': { t: '设置',
              b: '选择本页及其悬停帮助的语言，以及是否显示悬停帮助。该选择随会话一起记住。',
              reviewed: 'mt' },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'Interface language (English, French, Simplified Chinese). Choice names in the three menus stay English '
               + 'because hosts record them as automation names.' },
        fr: { t: 'Langue',
              b: 'Langue de l’interface (anglais, français, chinois simplifié). Les noms de choix des trois menus restent '
               + 'en anglais, car les hôtes les enregistrent comme noms d’automation.',
              reviewed: false },
        'zh-Hans': { t: '语言',
              b: '界面语言（英语、法语、简体中文）。三个菜单中的选项名称保持英文，因为宿主会把它们记录为自动化名称。',
              reviewed: 'mt' },
    },
    'tips-toggle': {
        en: { t: 'Hover help',
              b: 'Turn these explanations on or off. Off keeps the page quiet for a performance; the gear and this switch '
               + 'still explain themselves so you can find the way back.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces explications. Désactivées, la page reste silencieuse pour une prestation ; '
               + 'la roue dentée et cet interrupteur continuent de s’expliquer pour retrouver le chemin.',
              reviewed: false },
        'zh-Hans': { t: '悬停帮助',
              b: '打开或关闭这些说明。关闭后页面在演出时保持安静；齿轮和这个开关仍会自我说明，方便你找回来。',
              reviewed: 'mt' },
    },

    // ── Header chips ────────────────────────────────────────────────────────
    'chipFormat': {
        en: { t: 'Format',
              b: 'AmbiX = ACN channel order + SN3D normalisation. That is what the channel view shows and what any AmbiX '
               + 'plugin would expect.' },
        fr: { t: 'Format',
              b: 'AmbiX = ordre de canaux ACN + normalisation SN3D. C’est ce que montre la vue des canaux et ce '
               + 'qu’attendrait tout plugin AmbiX.',
              reviewed: false },
        'zh-Hans': { t: '格式',
              b: 'AmbiX = ACN 声道顺序 + SN3D 归一化。这就是声道视图所显示的，也是任何 AmbiX 插件所期望的。',
              reviewed: 'mt' },
    },
    // ── Source ──────────────────────────────────────────────────────────────
    'azimuth': {
        en: { t: 'Azimuth',
              b: 'Where the source sits around you, in degrees. 0° is straight ahead, +90° is to your LEFT (the AmbiX '
               + 'convention), −90° to your right, ±180° behind. Drag the top-down view to set it. Watch the X row '
               + '(front–back) hand its energy to the Y row (left–right) as you move from front to left; W never changes. '
               + 'Crossing behind jumps from +180° to −180°: host automation drawn between the two sweeps through the front.' },
        fr: { t: 'Azimut',
              b: 'Où se trouve la source autour de vous, en degrés. 0° est droit devant, +90° à votre GAUCHE (la '
               + 'convention AmbiX), −90° à votre droite, ±180° derrière. Faites glisser la vue de dessus pour le régler. '
               + 'Observez la rangée X (avant–arrière) céder son énergie à la rangée Y (gauche–droite) quand vous passez '
               + 'de l’avant à la gauche ; W ne change jamais. Passer derrière fait sauter la valeur de +180° à −180° : '
               + 'une automation de l’hôte tracée entre les deux balaie l’avant.',
              reviewed: false },
        'zh-Hans': { t: '方位角',
              b: '声源在你周围的位置，以度为单位。0° 为正前方，+90° 在你的左侧（AmbiX 约定），−90° 在右侧，±180° 在后方。'
               + '拖动俯视图即可设置。从前方移到左侧时，看 X 行（前后）把能量交给 Y 行（左右）；W 始终不变。'
               + '从后方穿过时数值会从 +180° 跳到 −180°：宿主在两者之间绘制的自动化会扫过前方。',
              reviewed: 'mt' },
    },
    'elevation': {
        en: { t: 'Elevation',
              b: 'How high the source is: 0° is at ear level, +90° straight overhead, −90° below. Drag the side view to '
               + 'set it. Overhead, only the channels with a Z in them carry the sound — X and Y go to zero.' },
        fr: { t: 'Élévation',
              b: 'La hauteur de la source : 0° au niveau des oreilles, +90° à la verticale au-dessus, −90° en '
               + 'dessous. Faites glisser la vue de côté pour la régler. Au zénith, seuls les canaux contenant un Z '
               + 'portent le son — X et Y tombent à zéro.',
              reviewed: false },
        'zh-Hans': { t: '仰角',
              b: '声源有多高：0° 与耳朵齐平，+90° 正上方，−90° 在下方。拖动侧视图即可设置。在正上方时，只有带 Z 的声道承载声音——X 和 Y 归零。',
              reviewed: 'mt' },
    },
    'distance': {
        en: { t: 'Distance',
              b: 'Distance = level + air. The level roughly halves each time the distance doubles (0 dB at 1 m), and a '
               + 'low-pass filter dulls the top end as the source gets far away. The air model is deliberately '
               + 'exaggerated so the lesson is audible. It also sets the puck’s radius; it does not change the pattern bars.' },
        fr: { t: 'Distance',
              b: 'Distance = niveau + air. Le niveau baisse d’environ la moitié chaque fois que la distance double '
               + '(0 dB à 1 m), et un filtre passe-bas assourdit l’aigu quand la source s’éloigne. Le modèle d’air '
               + 'est volontairement exagéré pour que la leçon s’entende. Il règle aussi le rayon du palet ; il ne '
               + 'change pas les barres de motif.',
              reviewed: false },
        'zh-Hans': { t: '距离',
              b: '距离 = 电平 + 空气。距离每翻一倍，电平大约减半（1 m 处为 0 dB），声源远离时低通滤波器会使高频变暗。'
               + '空气模型被刻意夸大，好让这一课听得见。它还决定圆点的半径；它不会改变指向图样条。',
              reviewed: 'mt' },
    },
    'inputMode': {
        en: { t: 'Input',
              b: 'What counts as the mono source. Mono sum averages left and right; Left only takes the left channel. '
               + 'Everything after this point is one signal placed in space.' },
        fr: { t: 'Entrée',
              b: 'Ce qui tient lieu de source mono. Mono sum fait la moyenne de gauche et droite ; Left only prend le '
               + 'canal gauche. Tout ce qui suit est un seul signal placé dans l’espace.',
              reviewed: false },
        'zh-Hans': { t: '输入',
              b: '什么算作单声道声源。Mono sum 取左右声道的平均；Left only 只取左声道。此后的一切都是被放置在空间中的一个信号。',
              reviewed: 'mt' },
    },
    'orbit': {
        en: { t: 'Orbit',
              b: 'On: the source circles you automatically, counter-clockwise, at the Orbit Rate. The puck shows where it '
               + 'is; watch X and Y trade places every quarter turn.' },
        fr: { t: 'Orbite',
              b: 'En marche, la source tourne automatiquement autour de vous, dans le sens antihoraire, à la vitesse '
               + 'd’orbite. Le palet montre où elle est ; observez X et Y échanger leurs places à chaque quart de tour.',
              reviewed: false },
        'zh-Hans': { t: '环绕',
              b: '开启时，声源以环绕速度自动逆时针绕你旋转。圆点显示它的位置；每转四分之一圈，看 X 和 Y 互换位置。',
              reviewed: 'mt' },
    },
    'orbitRate': {
        en: { t: 'Orbit Rate',
              b: 'How many circles per second the orbit makes. 0.05 Hz is one slow lap every 20 seconds; 2 Hz is a blur.' },
        fr: { t: 'Vitesse d’orbite',
              b: 'Le nombre de tours par seconde de l’orbite. 0,05 Hz est un tour lent toutes les 20 secondes ; '
               + '2 Hz est un tourbillon.',
              reviewed: false },
        'zh-Hans': { t: '环绕速度',
              b: '环绕每秒转多少圈。0.05 Hz 是每 20 秒慢慢转一圈；2 Hz 则快得模糊。',
              reviewed: 'mt' },
    },

    // ── Encoder ─────────────────────────────────────────────────────────────
    'order': {
        en: { t: 'Order',
              b: 'How finely the soundfield is described. Order N uses (N+1)² channels: 1st = 4, 2nd = 9, 3rd = 16. '
               + 'Higher order = a sharper beam in the sphere view and more rows in the channel view. Each step up '
               + 'switches on one more band of channels; nothing in the lower bands changes.' },
        fr: { t: 'Ordre',
              b: 'La finesse de description du champ sonore. L’ordre N utilise (N+1)² canaux : 1er = 4, 2e = 9, '
               + '3e = 16. Un ordre plus élevé = un faisceau plus fin dans la vue de la sphère et plus de rangées dans la '
               + 'vue des canaux. Chaque cran supplémentaire allume une bande de canaux de plus ; rien ne change '
               + 'dans les bandes inférieures.',
              reviewed: false },
        'zh-Hans': { t: '阶数',
              b: '声场被描述得有多精细。N 阶使用 (N+1)² 个声道：1 阶 = 4，2 阶 = 9，3 阶 = 16。阶数越高，球面视图中的波束越锐利，声道视图中的行数越多。'
               + '每升一阶就多开启一个声道带；较低的带不会有任何变化。',
              reviewed: 'mt' },
    },

    // ── Rotation ────────────────────────────────────────────────────────────
    'yaw': {
        en: { t: 'Yaw',
              b: 'Turns the whole room about the vertical axis, like turning your head the other way. + turns the scene '
               + 'counter-clockwise seen from above, so a source in front is heard to the left. Yaw rotates the ROOM, '
               + 'not the source: the puck stays put while the compass rose and the beam turn.' },
        fr: { t: 'Lacet',
              b: 'Fait tourner toute la pièce autour de l’axe vertical, comme si vous tourniez la tête dans l’autre '
               + 'sens. + tourne la scène dans le sens antihoraire vu de dessus, donc une source devant s’entend à '
               + 'gauche. Le lacet tourne la PIÈCE, pas la source : le palet reste en place tandis que la rose '
               + 'des vents et le faisceau tournent.',
              reviewed: false },
        'zh-Hans': { t: '偏航',
              b: '让整个房间绕垂直轴旋转，就像把头转向另一边。+ 使场景从上方看逆时针旋转，所以前方的声源会被听成在左侧。'
               + '偏航旋转的是房间，不是声源：圆点留在原地，而罗盘和波束在转动。',
              reviewed: 'mt' },
    },
    'pitch': {
        en: { t: 'Pitch',
              b: 'Tilts the whole room forward and back: + raises the front of the scene. Note the gimbal case: at ±90° '
               + 'the yaw and roll axes line up, so those two knobs do the same thing — a display quirk, the audio is '
               + 'still rotated correctly.' },
        fr: { t: 'Tangage',
              b: 'Incline toute la pièce d’avant en arrière : + relève l’avant de la scène. Notez le cas du '
               + 'cardan : à ±90°, les axes de lacet et de roulis se confondent, et ces deux boutons font la même '
               + 'chose — une curiosité d’affichage, le son reste correctement tourné.',
              reviewed: false,
              termNote: 'rotation pitch (tangage), not musical pitch' },
        'zh-Hans': { t: '俯仰',
              b: '让整个房间前后倾斜：+ 抬起场景的前方。注意万向节的情况：在 ±90° 时偏航轴与横滚轴重合，这两个旋钮会做同一件事——只是显示上的小问题，音频仍被正确旋转。',
              reviewed: 'mt',
              termNote: 'rotation pitch (俯仰), not musical pitch' },
    },
    'roll': {
        en: { t: 'Roll',
              b: 'Tips the whole room sideways: + raises the left side of the scene. A source on your left rises; one '
               + 'on your right sinks.' },
        fr: { t: 'Roulis',
              b: 'Fait basculer toute la pièce sur le côté : + relève le côté gauche de la scène. Une source à '
               + 'votre gauche monte ; une source à votre droite descend.',
              reviewed: false },
        'zh-Hans': { t: '横滚',
              b: '让整个房间向一侧倾斜：+ 抬起场景的左侧。你左边的声源升高；右边的声源下沉。',
              reviewed: 'mt' },
    },

    // ── Decoder / output ────────────────────────────────────────────────────
    'decoder': {
        en: { t: 'Decoder',
              b: 'How the B-format reaches your two ears. Binaural: every channel is filtered through a measured dummy '
               + 'head (SADIE II KU100, MagLS) — height and behind survive on headphones. Stereo: two virtual cardioid '
               + 'microphones at ±60°, which only ever use W, X and Y, so orders 2–3 are ignored and height is folded '
               + 'flat. Both add the same 64 samples of latency.' },
        fr: { t: 'Décodeur',
              b: 'Comment le format B atteint vos deux oreilles. Binaural : chaque canal est filtré à travers une '
               + 'tête artificielle mesurée (SADIE II KU100, MagLS) — la hauteur et l’arrière survivent au casque. '
               + 'Stereo : deux microphones cardioïdes virtuels à ±60°, qui n’utilisent jamais que W, X et Y, '
               + 'donc les ordres 2–3 sont ignorés et la hauteur est aplatie. Les deux ajoutent les mêmes 64 '
               + 'échantillons de latence.',
              reviewed: false },
        'zh-Hans': { t: '解码器',
              b: 'B 格式如何到达你的双耳。Binaural：每个声道都经过一个实测人工头（SADIE II KU100，MagLS）滤波——在耳机上高度和后方得以保留。'
               + 'Stereo：两个位于 ±60° 的虚拟心形话筒，只使用 W、X 和 Y，因此 2–3 阶被忽略，高度被压平。两者都增加同样的 64 个采样延迟。',
              reviewed: 'mt' },
    },
    'outputLevel': {
        en: { t: 'Output Level',
              b: 'The master volume after the decoder. All the way down is silence (−inf). The peak meters read after '
               + 'this knob.' },
        fr: { t: 'Niveau de sortie',
              b: 'Le volume général après le décodeur. Tout en bas, c’est le silence (−inf). Les crête-mètres lisent '
               + 'après ce bouton.',
              reviewed: false },
        'zh-Hans': { t: '输出电平',
              b: '解码器之后的总音量。拧到底即为静音（−inf）。峰值表在此旋钮之后读取。',
              reviewed: 'mt' },
    },
    'outMeter': {
        en: { t: 'Output Peak',
              b: 'Peak level of the left and right outputs after the Output Level knob.' },
        fr: { t: 'Crête de sortie',
              b: 'Niveau de crête des sorties gauche et droite après le bouton Niveau de sortie.',
              reviewed: false },
        'zh-Hans': { t: '输出峰值',
              b: '经过输出电平旋钮之后，左右输出的峰值电平。',
              reviewed: 'mt' },
    },

    // ── The three visual wells ──────────────────────────────────────────────
    'topWrap': {
        en: { t: 'Source on the Sphere · from above',
              b: 'You are the head in the centre, nose pointing up the page. The brass puck is the source: its angle is '
               + 'the azimuth, its distance from the head is the Distance (log scale, 0.2 m at the head to 20 m at the '
               + 'rim). Left is on the LEFT. The sage beam is where the sound is heard after rotation; it sharpens with '
               + 'order. The compass rose turns with Yaw — the puck does not. Drag to set the azimuth.' },
        fr: { t: 'Source sur la sphère · vue de dessus',
              b: 'Vous êtes la tête au centre, le nez vers le haut de la page. Le palet de laiton est la source : '
               + 'son angle est l’azimut, sa distance à la tête est la Distance (échelle logarithmique, 0,2 m à la '
               + 'tête, 20 m au bord). La gauche est à GAUCHE. Le faisceau sauge est l’endroit où le son s’entend '
               + 'après rotation ; il s’affine avec l’ordre. La rose des vents tourne avec le lacet — pas le '
               + 'palet. Faites glisser pour régler l’azimut.',
              reviewed: false },
        'zh-Hans': { t: '球面上的声源 · 俯视',
              b: '你是中央的那颗头，鼻子朝页面上方。黄铜圆点是声源：它的角度是方位角，它与头的距离是距离（对数刻度，头处 0.2 m，边缘 20 m）。'
               + '左就在左边。鼠尾草色的波束是旋转后声音被听到的位置；阶数越高越锐利。罗盘随偏航转动——圆点不转。拖动即可设置方位角。',
              reviewed: 'mt' },
    },
    'sideWrap': {
        en: { t: 'Source on the Sphere · from the side',
              b: 'The same source seen from your right: up the page is overhead, down is below. Drag to set the '
               + 'elevation. The sage tick on the rim is where the sound is heard after pitch and roll.' },
        fr: { t: 'Source sur la sphère · vue de côté',
              b: 'La même source vue depuis votre droite : le haut de la page est le zénith, le bas est en dessous. '
               + 'Faites glisser pour régler l’élévation. Le repère sauge sur le bord est l’endroit où le son s’entend '
               + 'après tangage et roulis.',
              reviewed: false },
        'zh-Hans': { t: '球面上的声源 · 侧视',
              b: '从你右侧看到的同一声源：页面上方是头顶，下方是脚下。拖动即可设置仰角。边缘的鼠尾草色刻度是经过俯仰和横滚后声音被听到的位置。',
              reviewed: 'mt' },
    },
    'chanWell': {
        en: { t: 'B-Format Channels',
              b: 'The soundfield as the channels that carry it — FuMa letters, ACN order. W is the omnidirectional part (always 1); '
               + 'X, Y, Z are front–back, left–right and up–down (a figure-of-eight each). Higher bands add finer '
               + 'patterns. Each bar is that channel’s gain for the current direction: sage to the right is positive, '
               + 'amber to the left is negative (the sound arrives inverted on that channel). The thin line beneath is '
               + 'the actual level, which breathes with the music. Normalisation is SN3D, channel order is ACN — '
               + 'together, AmbiX.' },
        fr: { t: 'Canaux du format B',
              b: 'Le champ sonore vu comme les canaux qui le portent — lettres FuMa, ordre ACN. W est la part omnidirectionnelle '
               + '(toujours 1) ; X, Y, Z sont avant–arrière, gauche–droite et haut–bas (un huit chacun). Les '
               + 'bandes supérieures ajoutent des motifs plus fins. Chaque barre est le gain de ce canal pour la '
               + 'direction courante : sauge vers la droite est positif, ambre vers la gauche est négatif (le son '
               + 'arrive inversé sur ce canal). Le trait fin dessous est le niveau réel, qui respire avec la musique. '
               + 'La normalisation est SN3D, l’ordre des canaux ACN — ensemble, AmbiX.',
              reviewed: false },
        'zh-Hans': { t: 'B 格式声道',
              b: '以承载它的各声道来呈现声场——FuMa 字母，ACN 顺序。W 是全向部分（始终为 1）；X、Y、Z 分别是前后、左右和上下（各为一个 8 字形）。'
               + '更高的带增加更精细的图样。每根条是该声道在当前方向上的增益：向右的鼠尾草色为正，向左的琥珀色为负（声音在该声道上反相到达）。'
               + '下方的细线是实际电平，随音乐起伏。归一化为 SN3D，声道顺序为 ACN——合起来就是 AmbiX。',
              reviewed: 'mt' },
    },
    'decWrap': {
        en: { t: 'Decoder',
              b: 'Binaural shows the dummy head whose ears were measured for the filters (KU100). Stereo shows the two '
               + 'virtual microphones. On headphones a source straight ahead is easy to confuse with one straight '
               + 'behind — the ears receive almost the same thing — which is one reason Overhead and Behind You are '
               + 'lessons. The decoder adds a constant 64 samples in both modes (one convolver partition) and reports '
               + 'it to the host, which lines the track up automatically.' },
        fr: { t: 'Décodeur',
              b: 'Binaural montre la tête artificielle dont les oreilles ont été mesurées pour les filtres (KU100). '
               + 'Stereo montre les deux microphones virtuels. Au casque, une source droit devant se confond '
               + 'facilement avec une source droit derrière — les oreilles reçoivent presque la même chose — et c’est '
               + 'une des raisons pour lesquelles Au-dessus et Derrière vous sont des leçons. Le décodeur ajoute 64 '
               + 'échantillons constants dans les deux modes (une partition du convolueur) et le signale à l’hôte, qui '
               + 'réaligne la piste automatiquement.',
              reviewed: false },
        'zh-Hans': { t: '解码器',
              b: 'Binaural 显示为滤波器测量过耳朵的人工头（KU100）。Stereo 显示两个虚拟话筒。在耳机上，正前方的声源很容易与正后方的混淆——双耳收到的几乎一样——这正是“头顶”和“在你身后”成为课程的原因之一。解码器在两种模式下都增加固定的 64 个采样（一个卷积分区），并报告给宿主，宿主会自动对齐音轨。',
              reviewed: 'mt' },
    },

    // ── Signal-path stations ────────────────────────────────────────────────
    'st-source': {
        en: { t: 'Signal path · Source',
              b: 'One mono signal, with the distance law applied before anything else: a level drop and an air filter.' },
        fr: { t: 'Chaîne du signal · Source',
              b: 'Un seul signal mono, avec la loi de distance appliquée avant tout le reste : une baisse de '
               + 'niveau et un filtre d’air.',
              reviewed: false },
        'zh-Hans': { t: '信号路径 · 源',
              b: '一个单声道信号，先于一切应用距离定律：一次电平下降和一个空气滤波器。',
              reviewed: 'mt' },
    },
    'st-encoder': {
        en: { t: 'Signal path · Encoder',
              b: 'The mono signal becomes (N+1)² channels, each weighted by its spherical-harmonic pattern at the source '
               + 'direction. This highlights whenever azimuth, elevation, distance, orbit or order change.' },
        fr: { t: 'Chaîne du signal · Encodeur',
              b: 'Le signal mono devient (N+1)² canaux, chacun pondéré par son motif d’harmonique sphérique dans la '
               + 'direction de la source. S’allume dès que l’azimut, l’élévation, la distance, l’orbite ou l’ordre changent.',
              reviewed: false },
        'zh-Hans': { t: '信号路径 · 编码器',
              b: '单声道信号变成 (N+1)² 个声道，每个都按其球谐图样在声源方向上的值加权。方位角、仰角、距离、环绕或阶数变化时会高亮。',
              reviewed: 'mt' },
    },
    'st-rotation': {
        en: { t: 'Signal path · Rotation',
              b: 'One rotation matrix in the harmonic domain turns the whole scene by yaw, pitch and roll. Highlights '
               + 'when any of the three move.' },
        fr: { t: 'Chaîne du signal · Rotation',
              b: 'Une seule matrice de rotation dans le domaine harmonique tourne toute la scène selon le lacet, le '
               + 'tangage et le roulis. S’allume dès que l’un des trois bouge.',
              reviewed: false },
        'zh-Hans': { t: '信号路径 · 旋转',
              b: '谐波域中的一个旋转矩阵按偏航、俯仰和横滚转动整个场景。三者任一移动时会高亮。',
              reviewed: 'mt' },
    },
    'st-decoder': {
        en: { t: 'Signal path · Decoder',
              b: 'Binaural or Stereo: the channels become two ear signals. Highlights when the mode changes.' },
        fr: { t: 'Chaîne du signal · Décodeur',
              b: 'Binaural ou Stereo : les canaux deviennent deux signaux d’oreille. S’allume quand le mode change.',
              reviewed: false },
        'zh-Hans': { t: '信号路径 · 解码器',
              b: 'Binaural 或 Stereo：各声道变成两路耳朵信号。模式改变时会高亮。',
              reviewed: 'mt' },
    },
    'st-output': {
        en: { t: 'Signal path · Output',
              b: 'Left and right, after the Output Level.' },
        fr: { t: 'Chaîne du signal · Sortie',
              b: 'Gauche et droite, après le niveau de sortie.',
              reviewed: false },
        'zh-Hans': { t: '信号路径 · 输出',
              b: '左和右，在输出电平之后。',
              reviewed: 'mt' },
    },

    // ── Lesson buttons (stable ids; the captions localize, the ids do not) ──
    'lesson.pointSource': {
        en: { t: 'Lesson · Point Source (FOA)',
              b: 'One source at 30° left, first order. W holds steady; X and Y share the direction. The simplest '
               + 'picture of B-format.' },
        fr: { t: 'Leçon · Source ponctuelle (FOA)',
              b: 'Une source à 30° à gauche, premier ordre. W reste stable ; X et Y se partagent la direction. '
               + 'L’image la plus simple du format B.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 点声源（FOA）',
              b: '一个声源在左侧 30°，一阶。W 保持稳定；X 和 Y 分担方向。B 格式最简单的图景。',
              reviewed: 'mt' },
    },
    'lesson.orderStep': {
        en: { t: 'Lesson · Order 1 → 2 → 3',
              b: 'Starts at 3rd order with the source at 45°. Step the Order back to 1 and watch bands switch off and '
               + 'the beam blur; step up and they return.' },
        fr: { t: 'Leçon · Ordre 1 → 2 → 3',
              b: 'Commence au 3e ordre avec la source à 45°. Ramenez l’ordre à 1 et regardez les bandes s’éteindre et le '
               + 'faisceau se brouiller ; remontez et elles reviennent.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 阶数 1 → 2 → 3',
              b: '从 3 阶开始，声源在 45°。把阶数降回 1，看各带关闭、波束变模糊；再升回去，它们又会回来。',
              reviewed: 'mt' },
    },
    'lesson.behindYou': {
        en: { t: 'Lesson · Behind You',
              b: 'The source at 180°. On headphones, front and back arrive almost identically — the X row flips sign, '
               + 'and that is most of the difference.' },
        fr: { t: 'Leçon · Derrière vous',
              b: 'La source à 180°. Au casque, l’avant et l’arrière arrivent presque à l’identique — la rangée X change '
               + 'de signe, et c’est l’essentiel de la différence.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 在你身后',
              b: '声源在 180°。在耳机上，前方和后方到达的声音几乎相同——X 行翻转符号，这就是差异的大部分。',
              reviewed: 'mt' },
    },
    'lesson.overhead': {
        en: { t: 'Lesson · Overhead',
              b: 'The source straight up. Only the Z-bearing rows carry it (Z, R, K); X and Y fall to zero.' },
        fr: { t: 'Leçon · Au-dessus',
              b: 'La source à la verticale. Seules les rangées portant un Z la transmettent (Z, R, K) ; X et Y '
               + 'tombent à zéro.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 头顶',
              b: '声源在正上方。只有带 Z 的行承载它（Z、R、K）；X 和 Y 降为零。',
              reviewed: 'mt' },
    },
    'lesson.rotateRoom': {
        en: { t: 'Lesson · Rotate the Room',
              b: 'Source at 60°, yaw at 120°: the puck stays at 60° but the beam is heard behind. Yaw rotates the room, '
               + 'not the source.' },
        fr: { t: 'Leçon · Tourner la pièce',
              b: 'Source à 60°, lacet à 120° : le palet reste à 60° mais le faisceau s’entend derrière. Le lacet '
               + 'tourne la pièce, pas la source.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 旋转房间',
              b: '声源在 60°，偏航 120°：圆点仍在 60°，但波束被听成在后方。偏航旋转的是房间，不是声源。',
              reviewed: 'mt' },
    },
    'lesson.farNear': {
        en: { t: 'Lesson · Far Away / Up Close',
              b: '12 m away: level drops and the air filter dulls the top. Try 0.2 m next — the pattern bars never '
               + 'change, only level and air.' },
        fr: { t: 'Leçon · Loin / Tout près',
              b: 'À 12 m : le niveau baisse et le filtre d’air assourdit l’aigu. Essayez ensuite 0,2 m — les '
               + 'barres de motif ne changent jamais, seulement le niveau et l’air.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 远处 / 近处',
              b: '在 12 m 之外：电平下降，空气滤波器使高频变暗。接着试试 0.2 m——指向图样条从不改变，变的只有电平和空气。',
              reviewed: 'mt' },
    },
    'lesson.slowOrbit': {
        en: { t: 'Lesson · Slow Orbit',
              b: 'The source circles you once every 20 seconds. Watch X and Y trade places, and W stay still.' },
        fr: { t: 'Leçon · Orbite lente',
              b: 'La source fait le tour de vous toutes les 20 secondes. Regardez X et Y échanger leurs places, et W '
               + 'rester immobile.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 慢速环绕',
              b: '声源每 20 秒绕你一圈。看 X 和 Y 互换位置，而 W 保持不动。',
              reviewed: 'mt' },
    },
    'lesson.speakersHeadphones': {
        en: { t: 'Lesson · Speakers vs Headphones',
              b: 'Third order, decoded to Stereo: only W, X and Y are used by a first-order cardioid pair. Switch back '
               + 'to Binaural to hear what orders 2–3 add.' },
        fr: { t: 'Leçon · Enceintes ou casque',
              b: 'Troisième ordre, décodé en Stereo : seuls W, X et Y sont utilisés par une paire cardioïde du '
               + 'premier ordre. Repassez en Binaural pour entendre ce qu’ajoutent les ordres 2–3.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 音箱与耳机',
              b: '三阶，解码为 Stereo：一阶心形话筒对只使用 W、X 和 Y。切回 Binaural，听听 2–3 阶增加了什么。',
              reviewed: 'mt' },
    },

    // ── Preset bar (Stage 4, FUNC-08) ───────────────────────────────────────
    'presetPrev': {
        en: { t: 'Previous preset',
              b: 'Loads the previous preset in the list: the factory presets first, then your own.' },
        fr: { t: 'Préréglage précédent',
              b: 'Charge le préréglage précédent de la liste : les préréglages d’usine d’abord, puis les vôtres.',
              reviewed: false },
        'zh-Hans': { t: '上一个预设',
              b: '载入列表中的上一个预设：先是出厂预设，然后是你自己的预设。',
              reviewed: 'mt' },
    },
    'presetName': {
        en: { t: 'Presets',
              b: 'Click to browse the presets. Init and the eight lessons come first — each one shows one idea — then '
               + 'your own. A preset never changes Output Level or Input Mode. A dot means you have changed something '
               + 'since it loaded.' },
        fr: { t: 'Préréglages',
              b: 'Cliquez pour parcourir les préréglages. Init et les huit leçons d’abord — chacune montre une idée — '
               + 'puis les vôtres. Un préréglage ne change jamais le niveau de sortie ni le mode d’entrée. Un point '
               + 'signale une modification depuis le chargement.',
              reviewed: false },
        'zh-Hans': { t: '预设',
              b: '点击浏览预设。先是 Init 和八个课程——每个演示一个概念——然后是你自己的预设。预设不会改变输出电平或输入模式。圆点表示载入后你已作修改。',
              reviewed: 'mt' },
    },
    'presetNext': {
        en: { t: 'Next preset',
              b: 'Loads the next preset in the list: the factory presets first, then your own.' },
        fr: { t: 'Préréglage suivant',
              b: 'Charge le préréglage suivant de la liste : les préréglages d’usine d’abord, puis les vôtres.',
              reviewed: false },
        'zh-Hans': { t: '下一个预设',
              b: '载入列表中的下一个预设：先是出厂预设，然后是你自己的预设。',
              reviewed: 'mt' },
    },
    'presetSave': {
        en: { t: 'Save preset',
              b: 'Saves the scene as one of your presets — every setting except Output Level and Input Mode, which '
               + 'stay yours.' },
        fr: { t: 'Enregistrer le préréglage',
              b: 'Enregistre la scène dans un de vos préréglages — tous les réglages sauf le niveau de sortie et le '
               + 'mode d’entrée, qui restent les vôtres.',
              reviewed: false },
        'zh-Hans': { t: '保存预设',
              b: '将场景保存为你的预设——保存除输出电平和输入模式以外的所有设置，这两项始终由你决定。',
              reviewed: 'mt' },
    },
    'presetDelete': {
        en: { t: 'Delete preset',
              b: 'Deletes the current preset if it is one of your own. Factory presets cannot be deleted.' },
        fr: { t: 'Supprimer le préréglage',
              b: 'Supprime le préréglage actuel s’il est à vous. Les préréglages d’usine ne peuvent pas être supprimés.',
              reviewed: false },
        'zh-Hans': { t: '删除预设',
              b: '若当前预设是你自己的，则将其删除。出厂预设无法删除。',
              reviewed: 'mt' },
    },
});

// key -> { en: {t}, fr: {t, reviewed}, 'zh-Hans': {t, reviewed} } — visible
// captions only. A control whose tooltip title IS its caption (the knobs, the
// segmented rows, the three wells' names) carries ONE I18N key and no LABELS
// copy: trLabel() falls back to I18N.
export const LABELS = Object.freeze({
    // ── chrome ──────────────────────────────────────────────────────────────
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: false }, 'zh-Hans': { t: '悬停帮助', reviewed: 'mt' } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false }, 'zh-Hans': { t: '界面语言', reviewed: 'mt' } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: false }, 'zh-Hans': { t: '开关悬停帮助', reviewed: 'mt' } },
    // The switch's two faces (feminine plural: they agree with "les infobulles").
    'ui.on':  { en: { t: 'On' },  fr: { t: 'Activées', reviewed: false },    'zh-Hans': { t: '开', reviewed: 'mt' } },
    'ui.off': { en: { t: 'Off' }, fr: { t: 'Désactivées', reviewed: false }, 'zh-Hans': { t: '关', reviewed: 'mt' } },

    // ── header ──────────────────────────────────────────────────────────────
    'label.subtitle': {
        en: { t: 'Encode · Rotate · Decode · A Field Guide to the Soundfield' },
        fr: { t: 'Encoder · Tourner · Décoder · Un guide du champ sonore', reviewed: false },
        'zh-Hans': { t: '编码 · 旋转 · 解码 · 声场实地指南', reviewed: 'mt' },
    },
    'label.formatBody':    { en: { t: 'AmbiX · ACN · SN3D ·' }, fr: { t: 'AmbiX · ACN · SN3D ·', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: 'AmbiX · ACN · SN3D ·', reviewed: 'mt', sameAsEn: true } },
    'label.chipOrder':     { en: { t: '{ord} order, {ch} ch' }, fr: { t: '{ord} ordre, {ch} can.', reviewed: false }, 'zh-Hans': { t: '{ord}，{ch} 声道', reviewed: 'mt' } },
    // Ordinals: the Order segment faces and the {ord} token of the readouts.
    'label.ord1': { en: { t: '1st' }, fr: { t: '1er', reviewed: false }, 'zh-Hans': { t: '1 阶', reviewed: 'mt' } },
    'label.ord2': { en: { t: '2nd' }, fr: { t: '2e', reviewed: false },  'zh-Hans': { t: '2 阶', reviewed: 'mt' } },
    'label.ord3': { en: { t: '3rd' }, fr: { t: '3e', reviewed: false },  'zh-Hans': { t: '3 阶', reviewed: 'mt' } },

    // ── visual band ─────────────────────────────────────────────────────────
    'label.sphereTitle': { en: { t: 'Source on the Sphere' }, fr: { t: 'Source sur la sphère', reviewed: false }, 'zh-Hans': { t: '球面上的声源', reviewed: 'mt' } },
    'label.sphereHint':  { en: { t: 'drag from above = azimuth, from the side = elevation' }, fr: { t: 'glisser vu de dessus = azimut, de côté = élévation', reviewed: false }, 'zh-Hans': { t: '俯视拖动 = 方位角，侧视拖动 = 仰角', reviewed: 'mt' } },
    'label.chanTitle':   { en: { t: 'B-Format Channels' }, fr: { t: 'Canaux du format B', reviewed: false }, 'zh-Hans': { t: 'B 格式声道', reviewed: 'mt' } },
    'label.chanHint1':   { en: { t: 'signed gain per ACN channel ·' }, fr: { t: 'gain signé par canal ACN ·', reviewed: false }, 'zh-Hans': { t: '每个 ACN 声道的带符号增益 ·', reviewed: 'mt' } },
    'label.chanHintPos': { en: { t: '+ sage' }, fr: { t: '+ sauge', reviewed: false }, 'zh-Hans': { t: '+ 鼠尾草色', reviewed: 'mt' } },
    'label.chanHintNeg': { en: { t: '− amber' }, fr: { t: '− ambre', reviewed: false }, 'zh-Hans': { t: '− 琥珀色', reviewed: 'mt' } },
    'label.chanHint2':   { en: { t: '· level beneath' }, fr: { t: '· niveau dessous', reviewed: false }, 'zh-Hans': { t: '· 下方为电平', reviewed: 'mt' } },
    'label.decHint':     { en: { t: 'to your two ears' }, fr: { t: 'vers vos deux oreilles', reviewed: false }, 'zh-Hans': { t: '到你的双耳', reviewed: 'mt' } },

    // the top-down well's readouts
    'label.unitAz':        { en: { t: 'az' }, fr: { t: 'az', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '方位', reviewed: 'mt' } },
    'label.unitEl':        { en: { t: 'el' }, fr: { t: 'él', reviewed: false }, 'zh-Hans': { t: '仰角', reviewed: 'mt' } },
    'label.az.front':      { en: { t: 'in front' },     fr: { t: 'devant', reviewed: false },           'zh-Hans': { t: '正前方', reviewed: 'mt' } },
    'label.az.frontLeft':  { en: { t: 'front left' },   fr: { t: 'avant gauche', reviewed: false },     'zh-Hans': { t: '左前方', reviewed: 'mt' } },
    'label.az.frontRight': { en: { t: 'front right' },  fr: { t: 'avant droite', reviewed: false },     'zh-Hans': { t: '右前方', reviewed: 'mt' } },
    'label.az.left':       { en: { t: 'to the left' },  fr: { t: 'à gauche', reviewed: false },         'zh-Hans': { t: '左侧', reviewed: 'mt' } },
    'label.az.right':      { en: { t: 'to the right' }, fr: { t: 'à droite', reviewed: false },         'zh-Hans': { t: '右侧', reviewed: 'mt' } },
    'label.az.behindLeft': { en: { t: 'behind left' },  fr: { t: 'arrière gauche', reviewed: false },   'zh-Hans': { t: '左后方', reviewed: 'mt' } },
    'label.az.behindRight':{ en: { t: 'behind right' }, fr: { t: 'arrière droite', reviewed: false },   'zh-Hans': { t: '右后方', reviewed: 'mt' } },
    'label.az.behind':     { en: { t: 'behind' },       fr: { t: 'derrière', reviewed: false },         'zh-Hans': { t: '正后方', reviewed: 'mt' } },
    'label.orbiting':      { en: { t: 'orbiting {rate}' }, fr: { t: 'en orbite à {rate}', reviewed: false }, 'zh-Hans': { t: '以 {rate} 环绕', reviewed: 'mt' } },
    'label.heardAt':       { en: { t: 'heard at' }, fr: { t: 'entendu à', reviewed: false }, 'zh-Hans': { t: '听到于', reviewed: 'mt' } },
    'label.wellYaw':       { en: { t: 'yaw' },   fr: { t: 'lacet', reviewed: false },   'zh-Hans': { t: '偏航', reviewed: 'mt' } },
    'label.wellPitch':     { en: { t: 'pitch' }, fr: { t: 'tangage', reviewed: false, termNote: 'rotation pitch (tangage), not musical pitch' }, 'zh-Hans': { t: '俯仰', reviewed: 'mt', termNote: 'rotation pitch (俯仰), not musical pitch' } },
    'label.wellRoll':      { en: { t: 'roll' },  fr: { t: 'roulis', reviewed: false },  'zh-Hans': { t: '横滚', reviewed: 'mt' } },
    'label.keySource':     { en: { t: 'source (azimuth, distance)' }, fr: { t: 'source (azimut, distance)', reviewed: false }, 'zh-Hans': { t: '声源（方位角、距离）', reviewed: 'mt' } },
    'label.keyBeam':       { en: { t: 'beam · where it is heard' }, fr: { t: 'faisceau · où on l’entend', reviewed: false }, 'zh-Hans': { t: '波束 · 被听到的位置', reviewed: 'mt' } },
    'label.orderNote':     { en: { t: 'order {n} ·' }, fr: { t: 'ordre {n} ·', reviewed: false }, 'zh-Hans': { t: '{n} 阶 ·', reviewed: 'mt' } },
    'label.beamWide':      { en: { t: 'wide beam' }, fr: { t: 'faisceau large', reviewed: false }, 'zh-Hans': { t: '宽波束', reviewed: 'mt' } },
    'label.beamNarrower':  { en: { t: 'narrower' },  fr: { t: 'plus étroit', reviewed: false },   'zh-Hans': { t: '更窄', reviewed: 'mt' } },
    'label.beamSharpest':  { en: { t: 'sharpest' },  fr: { t: 'le plus fin', reviewed: false },   'zh-Hans': { t: '最锐利', reviewed: 'mt' } },
    'label.sideCap':       { en: { t: 'side' }, fr: { t: 'côté', reviewed: false }, 'zh-Hans': { t: '侧视', reviewed: 'mt' } },

    // the decoder well's readouts
    'label.decToLR':    { en: { t: 'to L / R' }, fr: { t: 'vers G / D', reviewed: false }, 'zh-Hans': { t: '到 L / R', reviewed: 'mt' } },
    'label.decChAll':   { en: { t: '{n} ch in' }, fr: { t: '{n} can. en entrée', reviewed: false }, 'zh-Hans': { t: '输入 {n} 声道', reviewed: 'mt' } },
    'label.decChSome':  { en: { t: '4 of {n} ch in' }, fr: { t: '4 des {n} can. en entrée', reviewed: false }, 'zh-Hans': { t: '输入 {n} 声道中的 4 个', reviewed: 'mt' } },
    'label.decLineBin': { en: { t: 'MagLS · SADIE II KU100 · 64-sample latency' }, fr: { t: 'MagLS · SADIE II KU100 · latence de 64 éch.', reviewed: false }, 'zh-Hans': { t: 'MagLS · SADIE II KU100 · 64 个采样延迟', reviewed: 'mt' } },
    'label.decLineSt':  { en: { t: 'cardioids at ±60° · W X Y only · 64 samples' }, fr: { t: 'cardioïdes à ±60° · W X Y seulement · 64 éch.', reviewed: false }, 'zh-Hans': { t: '±60° 心形 · 仅 W X Y · 64 个采样', reviewed: 'mt' } },

    // ── signal path + group titles (the same five words serve both) ─────────
    'label.source':   { en: { t: 'Source' },   fr: { t: 'Source', reviewed: false, sameAsEn: true },   'zh-Hans': { t: '源', reviewed: 'mt' } },
    'label.encoder':  { en: { t: 'Encoder' },  fr: { t: 'Encodeur', reviewed: false },                 'zh-Hans': { t: '编码器', reviewed: 'mt' } },
    'label.rotation': { en: { t: 'Rotation' }, fr: { t: 'Rotation', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '旋转', reviewed: 'mt' } },
    'label.output':   { en: { t: 'Output' },   fr: { t: 'Sortie', reviewed: false },                   'zh-Hans': { t: '输出', reviewed: 'mt' } },
    'label.routeSource':   { en: { t: 'where the sound is' }, fr: { t: 'où est le son', reviewed: false }, 'zh-Hans': { t: '声音所在之处', reviewed: 'mt' } },
    'label.routeEncoder':  { en: { t: 'to B-format' }, fr: { t: 'en format B', reviewed: false }, 'zh-Hans': { t: '转为 B 格式', reviewed: 'mt' } },
    'label.routeRotation': { en: { t: 'turns the room, not the source' }, fr: { t: 'tourne la pièce, pas la source', reviewed: false }, 'zh-Hans': { t: '旋转房间，而非声源', reviewed: 'mt' } },
    // The Output group's route word: a label (not bare markup) so a translated group
    // title cannot slide a non-label span; French writes the pair G / D.
    'label.routeOutput':   { en: { t: 'L / R' }, fr: { t: 'G / D', reviewed: false }, 'zh-Hans': { t: 'L / R', reviewed: 'mt', sameAsEn: true } },
    'label.routeDecoder':  { en: { t: 'to two ears' }, fr: { t: 'aux oreilles', reviewed: false }, 'zh-Hans': { t: '到双耳', reviewed: 'mt' } },
    'label.stSourceMono':  { en: { t: 'mono sum · level + air' }, fr: { t: 'somme mono · niveau + air', reviewed: false }, 'zh-Hans': { t: '单声道求和 · 电平 + 空气', reviewed: 'mt' } },
    'label.stSourceLeft':  { en: { t: 'left only · level + air' }, fr: { t: 'gauche seule · niveau + air', reviewed: false }, 'zh-Hans': { t: '仅左声道 · 电平 + 空气', reviewed: 'mt' } },
    'label.stEncoder':     { en: { t: 'B-format · {n} ch' }, fr: { t: 'format B · {n} can.', reviewed: false }, 'zh-Hans': { t: 'B 格式 · {n} 声道', reviewed: 'mt' } },
    'label.stRotation':    { en: { t: '{n} × {n} matrix' }, fr: { t: 'matrice {n} × {n}', reviewed: false }, 'zh-Hans': { t: '{n} × {n} 矩阵', reviewed: 'mt' } },
    'label.stDecoderBin':  { en: { t: 'binaural · {n} ch → 2' }, fr: { t: 'binaural · {n} can. → 2', reviewed: false }, 'zh-Hans': { t: '双耳 · {n} 声道 → 2', reviewed: 'mt' } },
    'label.stDecoderSt':   { en: { t: 'stereo · 4 of {n} ch → 2' }, fr: { t: 'stéréo · 4 des {n} can. → 2', reviewed: false }, 'zh-Hans': { t: '立体声 · 4 / {n} 声道 → 2', reviewed: 'mt' } },
    'label.stOutput':      { en: { t: 'L / R · {db}' }, fr: { t: 'G / D · {db}', reviewed: false }, 'zh-Hans': { t: 'L / R · {db}', reviewed: 'mt', sameAsEn: true } },

    // ── controls ────────────────────────────────────────────────────────────
    'label.input':     { en: { t: 'Input' }, fr: { t: 'Entrée', reviewed: false }, 'zh-Hans': { t: '输入', reviewed: 'mt' } },
    'label.level':     { en: { t: 'Level' }, fr: { t: 'Niveau', reviewed: false }, 'zh-Hans': { t: '电平', reviewed: 'mt' } },
    'label.peak':      { en: { t: 'Peak' },  fr: { t: 'Crête', reviewed: false },  'zh-Hans': { t: '峰值', reviewed: 'mt' } },
    'label.orbitRate': { en: { t: 'Orbit Rate' }, fr: { t: 'Vit. orbite', reviewed: false }, 'zh-Hans': { t: '环绕速度', reviewed: 'mt' } },
    // The Orbit segment's two faces. On is `Activé` (a glossary alternate), not
    // `Marche`: MARCHE measures 53.7 px in the 49 px pill, ACTIVÉ 45.2 — the
    // O-Orbit precedent. A generic state word, so the masculine form is kept.
    'label.orbitOff':  { en: { t: 'Off' }, fr: { t: 'Arrêt', reviewed: false },  'zh-Hans': { t: '关', reviewed: 'mt' } },
    'label.orbitOn':   { en: { t: 'On' },  fr: { t: 'Activé', reviewed: false }, 'zh-Hans': { t: '开', reviewed: 'mt' } },

    // the italic word under each knob's value
    'label.el.level': { en: { t: 'level' }, fr: { t: 'horizon', reviewed: false, termNote: 'ear-level elevation (the horizon), not a gain level — À L’HORIZONTALE (56.9 px) crossed the distance readout beside it at 12 m' }, 'zh-Hans': { t: '水平', reviewed: 'mt', termNote: 'ear-level elevation (水平), not a gain level' } },
    'label.el.above': { en: { t: 'above' }, fr: { t: 'au-dessus', reviewed: false }, 'zh-Hans': { t: '上方', reviewed: 'mt' } },
    'label.el.below': { en: { t: 'below' }, fr: { t: 'en dessous', reviewed: false }, 'zh-Hans': { t: '下方', reviewed: 'mt' } },
    'label.el.up':    { en: { t: 'straight up' },   fr: { t: 'à la verticale', reviewed: false }, 'zh-Hans': { t: '正上方', reviewed: 'mt' } },
    'label.el.down':  { en: { t: 'straight down' }, fr: { t: 'droit dessous', reviewed: false },  'zh-Hans': { t: '正下方', reviewed: 'mt' } },
    // zh carries no 空气 in the two distance readouts: two Han glyphs (≈ 21 px) against
    // "air" (≈ 13 px) grew the English-authored 15.4 px overhang of this 58 px knob cell to
    // 20.2 px (check-ui-labels [5]). The cutoff alone (or 全开, "fully open") reads cleanly;
    // the Distance tooltip names the air filter.
    'label.distOpen': { en: { t: '{db} · air open' }, fr: { t: '{db} · air libre', reviewed: false }, 'zh-Hans': { t: '{db} · 全开', reviewed: 'mt' } },
    'label.distAir':  { en: { t: '{db} · air {fc}' }, fr: { t: '{db} · air {fc}', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '{db} · {fc}', reviewed: 'mt' } },
    'label.orbitTurnIn':      { en: { t: 'one turn in {s} s' }, fr: { t: 'un tour en {s} s', reviewed: false }, 'zh-Hans': { t: '{s} s 转一圈', reviewed: 'mt' } },
    'label.orbitTurnsPerSec': { en: { t: '{n} turns / s' }, fr: { t: '{n} tours / s', reviewed: false }, 'zh-Hans': { t: '每秒 {n} 圈', reviewed: 'mt' } },
    'label.yawIdle':  { en: { t: 'scene turns about the top' }, fr: { t: 'tourne sur l’axe vertical', reviewed: false }, 'zh-Hans': { t: '场景绕顶部旋转', reviewed: 'mt' } },
    'label.yawHeard': { en: { t: 'front is heard at {deg}' }, fr: { t: 'avant entendu à {deg}', reviewed: false }, 'zh-Hans': { t: '前方被听到于 {deg}', reviewed: 'mt' } },
    'label.pitchSub': { en: { t: 'front tilts up' }, fr: { t: 'avant relevé', reviewed: false }, 'zh-Hans': { t: '前方上抬', reviewed: 'mt' } },
    'label.rollSub':  { en: { t: 'left side rises' }, fr: { t: 'gauche relevée', reviewed: false }, 'zh-Hans': { t: '左侧升高', reviewed: 'mt' } },

    // encoder group
    'label.encReadOrd': { en: { t: '{ord} order ·' }, fr: { t: '{ord} ordre ·', reviewed: false }, 'zh-Hans': { t: '{ord} ·', reviewed: 'mt' } },
    'label.encReadN':   { en: { t: '({n}+1)² = {ch} ch' }, fr: { t: '({n}+1)² = {ch} can.', reviewed: false }, 'zh-Hans': { t: '({n}+1)² = {ch} 声道', reviewed: 'mt' } },
    'label.encOrderN':  { en: { t: 'order {n}' }, fr: { t: 'ordre {n}', reviewed: false }, 'zh-Hans': { t: '{n} 阶', reviewed: 'mt' } },
    'label.bandCh':     { en: { t: '+{n} ch' }, fr: { t: '+{n} can.', reviewed: false }, 'zh-Hans': { t: '+{n} 声道', reviewed: 'mt' } },
    'label.scaleGain':  { en: { t: 'gain (pattern)' }, fr: { t: 'gain (motif)', reviewed: false }, 'zh-Hans': { t: '增益（图样）', reviewed: 'mt' } },
    'label.scalePeak':  { en: { t: 'peak' }, fr: { t: 'crête', reviewed: false }, 'zh-Hans': { t: '峰值', reviewed: 'mt' } },

    // rotation group
    'label.rotNote': { en: { t: '{n} × {n} matrix in the SH domain' }, fr: { t: 'matrice {n} × {n} dans le domaine SH', reviewed: false }, 'zh-Hans': { t: 'SH 域中的 {n} × {n} 矩阵', reviewed: 'mt' } },

    // decoder group
    'label.decReadBin': { en: { t: 'HRTF filters for every channel · headphones' }, fr: { t: 'filtres HRTF pour chaque canal · casque', reviewed: false }, 'zh-Hans': { t: '每个声道的 HRTF 滤波器 · 耳机', reviewed: 'mt' } },
    'label.decReadSt':  { en: { t: 'two virtual microphones · speakers or headphones' }, fr: { t: 'deux microphones virtuels · enceintes ou casque', reviewed: false }, 'zh-Hans': { t: '两个虚拟话筒 · 音箱或耳机', reviewed: 'mt' } },
    'label.decNoteBin': { en: { t: '· every channel reaches both ears through a measured head (KU100), so height and behind survive.' }, fr: { t: '· chaque canal atteint les deux oreilles à travers une tête mesurée (KU100), donc la hauteur et l’arrière survivent.', reviewed: false }, 'zh-Hans': { t: '· 每个声道都经由实测人工头（KU100）到达双耳，因此高度和后方得以保留。', reviewed: 'mt' } },
    'label.decNoteSt':  { en: { t: '· a first-order cardioid pair: orders 2–3 are ignored, height is folded flat.' }, fr: { t: '· une paire cardioïde du premier ordre : les ordres 2–3 sont ignorés, la hauteur est aplatie.', reviewed: false }, 'zh-Hans': { t: '· 一对一阶心形话筒：2–3 阶被忽略，高度被压平。', reviewed: 'mt' } },

    // ── lessons + footer ────────────────────────────────────────────────────
    'label.lessons': { en: { t: 'Lessons' }, fr: { t: 'Leçons', reviewed: false }, 'zh-Hans': { t: '课程', reviewed: 'mt' } },
    'label.lesson.pointSource':        { en: { t: 'Point Source (FOA)' },     fr: { t: 'Source ponctuelle (FOA)', reviewed: false }, 'zh-Hans': { t: '点声源（FOA）', reviewed: 'mt' } },
    'label.lesson.orderStep':          { en: { t: 'Order 1 → 2 → 3' },        fr: { t: 'Ordre 1 → 2 → 3', reviewed: false },          'zh-Hans': { t: '阶数 1 → 2 → 3', reviewed: 'mt' } },
    'label.lesson.behindYou':          { en: { t: 'Behind You' },             fr: { t: 'Derrière vous', reviewed: false },            'zh-Hans': { t: '在你身后', reviewed: 'mt' } },
    'label.lesson.overhead':           { en: { t: 'Overhead' },               fr: { t: 'Au-dessus', reviewed: false },                'zh-Hans': { t: '头顶', reviewed: 'mt' } },
    'label.lesson.rotateRoom':         { en: { t: 'Rotate the Room' },        fr: { t: 'Tourner la pièce', reviewed: false },         'zh-Hans': { t: '旋转房间', reviewed: 'mt' } },
    'label.lesson.farNear':            { en: { t: 'Far Away / Up Close' },    fr: { t: 'Loin / Tout près', reviewed: false },         'zh-Hans': { t: '远处 / 近处', reviewed: 'mt' } },
    'label.lesson.slowOrbit':          { en: { t: 'Slow Orbit' },             fr: { t: 'Orbite lente', reviewed: false },             'zh-Hans': { t: '慢速环绕', reviewed: 'mt' } },
    'label.lesson.speakersHeadphones': { en: { t: 'Speakers vs Headphones' }, fr: { t: 'Enceintes ou casque', reviewed: false },      'zh-Hans': { t: '音箱与耳机', reviewed: 'mt' } },
    'label.tourHint':    { en: { t: 'hover anything for help' }, fr: { t: 'survolez n’importe quoi pour l’aide', reviewed: false }, 'zh-Hans': { t: '悬停任意处查看帮助', reviewed: 'mt' } },
    'label.tourDefault': { en: { t: 'one mono source → B-format → rotated → decoded to your ears' }, fr: { t: 'une source mono → format B → tournée → décodée vers vos oreilles', reviewed: false }, 'zh-Hans': { t: '一个单声道声源 → B 格式 → 旋转 → 解码到你的双耳', reviewed: 'mt' } },
    'label.tourCaption.pointSource':        { en: { t: 'One source at 30° left, first order. W holds; X and Y share the direction.' }, fr: { t: 'Une source à 30° à gauche, premier ordre. W tient ; X et Y se partagent la direction.', reviewed: false }, 'zh-Hans': { t: '一个声源在左侧 30°，一阶。W 保持不变；X 和 Y 分担方向。', reviewed: 'mt' } },
    'label.tourCaption.orderStep':          { en: { t: 'Click again to step the order 1 → 2 → 3: each step lights a band and narrows the beam.' }, fr: { t: 'Cliquez encore pour passer l’ordre de 1 à 2 puis 3 : chaque pas allume une bande et resserre le faisceau.', reviewed: false }, 'zh-Hans': { t: '再次点击，阶数依次为 1 → 2 → 3：每一步点亮一个带，波束变窄。', reviewed: 'mt' } },
    'label.tourCaption.behindYou':          { en: { t: 'Straight behind: the X row flips sign. On headphones, front and back are easily confused.' }, fr: { t: 'Droit derrière : la rangée X change de signe. Au casque, l’avant et l’arrière se confondent facilement.', reviewed: false }, 'zh-Hans': { t: '正后方：X 行翻转符号。在耳机上，前方和后方很容易混淆。', reviewed: 'mt' } },
    'label.tourCaption.overhead':           { en: { t: 'Straight up: only the Z-bearing rows carry it; X and Y fall to zero.' }, fr: { t: 'À la verticale : seules les rangées portant un Z le transmettent ; X et Y tombent à zéro.', reviewed: false }, 'zh-Hans': { t: '正上方：只有带 Z 的行承载它；X 和 Y 降为零。', reviewed: 'mt' } },
    'label.tourCaption.rotateRoom':         { en: { t: 'Puck at 60°, yaw 120°: heard behind. Yaw turns the room, not the source.' }, fr: { t: 'Palet à 60°, lacet 120° : entendu derrière. Le lacet tourne la pièce, pas la source.', reviewed: false }, 'zh-Hans': { t: '圆点在 60°，偏航 120°：被听成在后方。偏航旋转的是房间，不是声源。', reviewed: 'mt' } },
    'label.tourCaption.farNear':            { en: { t: '12 m: level drops and the air dulls the top. Try 0.2 m — the bars never change.' }, fr: { t: '12 m : le niveau baisse et l’air assourdit l’aigu. Essayez 0,2 m — les barres ne changent jamais.', reviewed: false }, 'zh-Hans': { t: '12 m：电平下降，空气使高频变暗。试试 0.2 m——条形从不改变。', reviewed: 'mt' } },
    'label.tourCaption.slowOrbit':          { en: { t: 'One lap every 20 s. Watch X and Y trade places while W stays still.' }, fr: { t: 'Un tour toutes les 20 s. Regardez X et Y échanger leurs places pendant que W reste immobile.', reviewed: false }, 'zh-Hans': { t: '每 20 s 一圈。看 X 和 Y 互换位置，而 W 保持不动。', reviewed: 'mt' } },
    'label.tourCaption.speakersHeadphones': { en: { t: '3rd order into Stereo: only W, X, Y are used. Switch back to Binaural to hear 2–3.' }, fr: { t: '3e ordre en Stereo : seuls W, X, Y sont utilisés. Repassez en Binaural pour entendre 2–3.', reviewed: false }, 'zh-Hans': { t: '3 阶进入 Stereo：只使用 W、X、Y。切回 Binaural 听听 2–3 阶。', reviewed: 'mt' } },
    'label.footerRight': { en: { t: 'AmbiX · MagLS on SADIE II' }, fr: { t: 'AmbiX · MagLS sur SADIE II', reviewed: false }, 'zh-Hans': { t: 'AmbiX · 基于 SADIE II 的 MagLS', reviewed: 'mt' } },

    // ── Preset bar (Stage 4, FUNC-08) ───────────────────────────────────────
    // Widths (EB Garamond + PingFang, headless Chromium) are in index.html
    // beside each pin. The popover message line is nowrap.
    'label.presetSave':       { en: { t: 'Save' }, fr: { t: 'Enreg.', reviewed: false }, 'zh-Hans': { t: '保存', reviewed: 'mt' } },
    'label.presetDelete':     { en: { t: 'Delete' }, fr: { t: 'Suppr.', reviewed: false }, 'zh-Hans': { t: '删除', reviewed: 'mt' } },
    'preset.init':            { en: { t: 'Init' }, fr: { t: 'Initial', reviewed: false }, 'zh-Hans': { t: '初始', reviewed: 'mt' } },
    'preset.saveTitle':       { en: { t: 'Save preset' }, fr: { t: 'Enregistrer le préréglage', reviewed: false }, 'zh-Hans': { t: '保存预设', reviewed: 'mt' } },
    'preset.namePlaceholder': { en: { t: 'Preset name' }, fr: { t: 'Nom du préréglage', reviewed: false }, 'zh-Hans': { t: '预设名称', reviewed: 'mt' } },
    'preset.ok':              { en: { t: 'OK' }, fr: { t: 'OK', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '确定', reviewed: 'mt' } },
    'preset.cancel':          { en: { t: 'Cancel' }, fr: { t: 'Annuler', reviewed: false }, 'zh-Hans': { t: '取消', reviewed: 'mt' } },
    'preset.errEmpty':        { en: { t: 'Type a name first.' }, fr: { t: 'Saisissez d’abord un nom.', reviewed: false }, 'zh-Hans': { t: '请先输入名称。', reviewed: 'mt' } },
    'preset.errFactoryName':  { en: { t: 'A factory preset has that name — choose another.' }, fr: { t: 'Nom d’un préréglage d’usine — choisissez-en un autre.', reviewed: false }, 'zh-Hans': { t: '该名称属于出厂预设——请换一个。', reviewed: 'mt' } },
    'preset.errSave':         { en: { t: 'Could not save the preset.' }, fr: { t: 'Impossible d’enregistrer le préréglage.', reviewed: false }, 'zh-Hans': { t: '无法保存预设。', reviewed: 'mt' } },
    'preset.confirmReplace':  { en: { t: 'Replace “{name}”?' }, fr: { t: 'Remplacer « {name} » ?', reviewed: false }, 'zh-Hans': { t: '替换“{name}”？', reviewed: 'mt' } },
    // Straight quotes in the en: the zh glossary key is 'delete preset "{name}"?'.
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
    ['O – simple',  'the product name, first half of the split wordmark in the page heading — a product name is never translated', '.title'],
    ['Ambisonics',  'the product name, second half of the split wordmark (.title-accent) — a product name is never translated', '.title-accent'],
    ['Ouaricon Audio · O-simpleAmbisonics', 'the colophon: company + product name — never translated', '.footer'],

    // Endonyms in the language selector: a language name is written in its own language.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
    ['简体中文', 'endonym — a language name is never translated'],

    // AudioParameterChoice strings are the host automation contract and stay
    // English (CONTEXT.md §i18n): the Decoder segment faces, the decoder well's
    // name, and the Input menu entries built from properties.choices.
    ['Binaural', 'AudioParameterChoice entry (decoder) — the host automation name, English in every language', '#seg-decoder, #decName'],
    ['Stereo',   'AudioParameterChoice entry (decoder) — the host automation name, English in every language', '#seg-decoder, #decName'],
    ['Mono sum',  'AudioParameterChoice entry (inputMode) — the host automation name, English in every language', 'option'],
    ['Left only', 'AudioParameterChoice entry (inputMode) — the host automation name, English in every language', 'option'],
];

// One row per tooltip anchor: [selector, key]. A knob's row names its whole
// .knob-cell so the label and the readout under the knob share its tip.
export const TIP_BINDINGS = [
    ['#gear-btn',    'settings'],
    ['#lang-select', 'lang-select'],
    ['#tips-toggle', 'tips-toggle'],

    ['#chipFormat',  'chipFormat'],

    ['#presetPrev',   'presetPrev'],
    ['#presetName',   'presetName'],
    ['#presetNext',   'presetNext'],
    ['#presetSave',   'presetSave'],
    ['#presetDelete', 'presetDelete'],

    ['#topWrap',  'topWrap'],
    ['#sideWrap', 'sideWrap'],
    ['#chanWell', 'chanWell'],
    ['#decWrap',  'decWrap'],

    ['#st-source',   'st-source'],
    ['#st-encoder',  'st-encoder'],
    ['#st-rotation', 'st-rotation'],
    ['#st-decoder',  'st-decoder'],
    ['#st-output',   'st-output'],

    ['[data-param="azimuth"]',     'azimuth'],
    ['[data-param="elevation"]',   'elevation'],
    ['[data-param="distance"]',    'distance'],
    ['[data-param="inputMode"]',   'inputMode'],
    ['[data-param="orbit"]',       'orbit'],
    ['[data-param="orbitRate"]',   'orbitRate'],
    ['[data-param="order"]',       'order'],
    ['[data-param="yaw"]',         'yaw'],
    ['[data-param="pitch"]',       'pitch'],
    ['[data-param="roll"]',        'roll'],
    ['[data-param="decoder"]',     'decoder'],
    ['[data-param="outputLevel"]', 'outputLevel'],
    ['#outMeterWrap',              'outMeter'],

    ['#lesson-pointSource',        'lesson.pointSource'],
    ['#lesson-orderStep',          'lesson.orderStep'],
    ['#lesson-behindYou',          'lesson.behindYou'],
    ['#lesson-overhead',           'lesson.overhead'],
    ['#lesson-rotateRoom',         'lesson.rotateRoom'],
    ['#lesson-farNear',            'lesson.farNear'],
    ['#lesson-slowOrbit',          'lesson.slowOrbit'],
    ['#lesson-speakersHeadphones', 'lesson.speakersHeadphones'],
];

// Copied unchanged from O-ReverseDelay's committed i18n.js (the canon is one shape).
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
