/*
   This file is part of O-simpleWavetable, an Ouaricon Audio plugin.
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
// O-simpleWavetable — interface copy table (en / fr / zh-Hans)
//
// Generated at UI mockup v1 finalization as v1-i18n.js; copied to
// Source/ui/public/js/i18n.js in Stage 3. EXPORTS ONLY (check-i18n [7]); no
// innerHTML and no `<` in any string (check-i18n [9]).
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN, where the mockup had it: every tooltip
// body below is the v1-ui-test.html TIPS table, with two deliberate additions —
// the Import Audio tip names the drop path, and the Keyboard tip names the Z / X
// octave keys (the mockup's keyboard hint had no room for them).
//
// FRENCH is machine-drafted against scripts/i18n-fr-glossary.js and flagged
// `reviewed: false` throughout. Typography: U+00A0 before : ; ! ? %, between a
// number and its unit, U+2019 apostrophes. "frame" is *trame* everywhere.
//
// SIMPLIFIED CHINESE is machine-drafted against scripts/i18n-zh-glossary.js and
// flagged `reviewed: 'mt'` throughout. Full-width punctuation inside Han prose,
// one plain space between a Latin/digit run and a Han run, no thin spaces.
// "bank" is 波表库, "frame" is 帧.
//
// WHAT STAYS ENGLISH, by design (the lang-select tip says so): value readouts
// (D-03), file names, and the Bank menu — its entries are the C++
// AudioParameterChoice option strings, which are also the host automation
// names (D-01).
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

export const I18N = Object.freeze({

    // ── The settings popover ────────────────────────────────────────────────
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface and switch this hover help off or on. The language is saved with the session; the help switch is remembered on this computer.' },
        fr: { t: 'Réglages',
              b: 'Choisissez la langue de l’interface et activez ou désactivez ces infobulles. La langue est conservée avec la session ; le réglage des infobulles est conservé sur cet ordinateur.',
              reviewed: false },
        'zh-Hans': { t: '设置',
                     b: '选择界面语言，并关闭或开启这些悬停帮助。语言随会话一起保存；帮助开关保存在本机上。',
                     reviewed: 'mt' },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. Value readouts, file names and the Bank menu stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue des libellés de cette page et de ces infobulles. Les valeurs affichées, les noms de fichiers et le menu Banque restent en anglais.',
              reviewed: false },
        'zh-Hans': { t: '语言',
                     b: '本页标签与这些悬停帮助所用的语言。数值读数、文件名和波表库菜单保持英文。',
                     reviewed: 'mt' },
    },
    'tips-toggle': {
        en: { t: 'Hover help',
              b: 'Turns these hover explanations off or back on. The switch is remembered on this computer, so it follows you from one project to the next.' },
        fr: { t: 'Infobulles',
              b: 'Désactive ou réactive ces explications au survol. Le réglage est conservé sur cet ordinateur et vous suit d’un projet à l’autre.',
              reviewed: false },
        'zh-Hans': { t: '悬停帮助',
                     b: '关闭或重新开启这些悬停说明。此开关保存在本机上，因此会跟随你从一个项目到下一个项目。',
                     reviewed: 'mt' },
    },

    // ── Header: bank, import, source ────────────────────────────────────────
    'bank': {
        en: { t: 'Bank',
              b: 'Which set of frames the oscillator reads. Each built-in bank teaches one idea: Sine → Saw adds one harmonic per frame, Sine → Square adds odd harmonics only, Pulse Width narrows a pulse, Formant moves vowel peaks, Drive pushes a sine into soft clipping. Imported is a sound you load, cut into frames.' },
        fr: { t: 'Banque',
              b: 'L’ensemble de trames que lit l’oscillateur. Chaque banque intégrée enseigne une idée : Sine → Saw ajoute un harmonique par trame, Sine → Square n’ajoute que des harmoniques impairs, Pulse Width rétrécit une impulsion, Formant déplace des pics de voyelle, Drive pousse une sinusoïde vers un écrêtage doux. Imported est un son que vous chargez, découpé en trames.',
              reviewed: false },
        'zh-Hans': { t: '波表库',
                     b: '振荡器读取的那组帧。每个内置波表库讲一个概念：Sine → Saw 每帧增加一个谐波，Sine → Square 只增加奇次谐波，Pulse Width 让脉冲变窄，Formant 移动元音共振峰，Drive 把正弦波推向软削波。Imported 是你载入的声音，被切成帧。',
                     reviewed: 'mt' },
    },
    'importBtn': {
        en: { t: 'Import Audio',
              b: 'Load any sound file. It is cut into back-to-back slices of 2048 samples, and each slice becomes one frame (up to 256). The cuts ignore the sound’s pitch, so a frame’s end rarely meets its start: the loop jumps once every cycle and you hear that jump as a buzz. That is where modern wavetables come from, and why careful ones are cut in time with the pitch. You can also drop a file anywhere on the plugin.' },
        fr: { t: 'Importer de l’audio',
              b: 'Chargez n’importe quel fichier son. Il est découpé en tranches consécutives de 2048 échantillons, et chaque tranche devient une trame (256 au maximum). Les coupes ignorent la hauteur du son : la fin d’une trame rejoint rarement son début, la boucle saute donc une fois par cycle et vous entendez ce saut comme un bourdonnement. C’est l’origine des tables d’ondes modernes, et la raison pour laquelle les plus soignées sont découpées au rythme de la hauteur. Vous pouvez aussi déposer un fichier n’importe où sur le plugin.',
              reviewed: false },
        'zh-Hans': { t: '导入音频',
                     b: '载入任意声音文件。它会被切成连续的 2048 采样片段，每个片段成为一帧（最多 256 帧）。切点不考虑声音的音高，所以一帧的结尾很少与开头相接：循环每个周期跳变一次，你会把这个跳变听成嗡嗡声。这就是现代波表的来历，也是精心制作的波表要按音高周期切分的原因。你也可以把文件拖放到插件的任何位置。',
                     reviewed: 'mt' },
    },
    'sourceReadout': {
        en: { t: 'Source',
              b: 'Where the current bank came from and how many frames it holds. Built-in banks are calculated from a formula when the plugin loads; an imported bank is saved with your project.' },
        fr: { t: 'Source',
              b: 'L’origine de la banque actuelle et le nombre de trames qu’elle contient. Les banques intégrées sont calculées à partir d’une formule au chargement du plugin ; une banque importée est enregistrée avec votre projet.',
              reviewed: false },
        'zh-Hans': { t: '源',
                     b: '当前波表库的来源以及它包含多少帧。内置波表库在插件载入时由公式计算得出；导入的波表库会随项目一起保存。',
                     reviewed: 'mt' },
    },

    // ── The three panels ────────────────────────────────────────────────────
    'bankWrap': {
        en: { t: 'Bank Stack',
              b: 'Every frame in the bank, frame 1 at the front and the last frame at the back. The amber trace is the frame being read. The brass diamond is the Position knob; the green ring is where the LFO and envelope actually put the scan. Drag up and down on the stack, or scroll, to scan.' },
        fr: { t: 'Pile de la banque',
              b: 'Toutes les trames de la banque, la trame 1 devant et la dernière au fond. Le tracé ambré est la trame lue. Le losange laiton est le bouton Position ; l’anneau vert indique où le LFO et l’enveloppe placent réellement le balayage. Faites glisser vers le haut ou le bas sur la pile, ou utilisez la molette, pour balayer.',
              reviewed: false },
        'zh-Hans': { t: '波表堆叠',
                     b: '波表库中的每一帧，第 1 帧在最前，最后一帧在最后面。琥珀色曲线是正在读取的帧。黄铜菱形是位置旋钮；绿色圆环是 LFO 和包络实际把扫描推到的位置。在堆叠上上下拖动或滚动滚轮即可扫描。',
                     reviewed: 'mt' },
    },
    'cycleWrap': {
        en: { t: 'Current Cycle',
              b: 'The single cycle the oscillator is looping right now, after crossfading and bit depth. At a note’s pitch this shape repeats hundreds of times a second, and its shape is the timbre.' },
        fr: { t: 'Cycle actuel',
              b: 'Le cycle unique que l’oscillateur boucle en ce moment, après le fondu enchaîné et la résolution. À la hauteur d’une note, cette forme se répète des centaines de fois par seconde, et sa forme est le timbre.',
              reviewed: false },
        'zh-Hans': { t: '当前周期',
                     b: '振荡器此刻循环播放的那一个周期，已经过交叉淡化和位深处理。在一个音符的音高下，这个形状每秒重复数百次，它的形状就是音色。',
                     reviewed: 'mt' },
    },
    'harmWrap': {
        en: { t: 'Harmonics',
              b: 'How strong harmonics 1 to 32 of the current cycle are, in decibels. A frame is a frozen additive spectrum: these are the partials O-simpleAdditive would stack to build it. Play a key to see what band-limiting keeps at that pitch.' },
        fr: { t: 'Harmoniques',
              b: 'L’intensité des harmoniques 1 à 32 du cycle actuel, en décibels. Une trame est un spectre additif figé : ce sont les partiels qu’O-simpleAdditive empilerait pour la construire. Jouez une touche pour voir ce que la limitation de bande conserve à cette hauteur.',
              reviewed: false },
        'zh-Hans': { t: '谐波',
                     b: '当前周期第 1 到第 32 次谐波的强度，以分贝表示。一帧就是一个冻结的加法频谱：这些就是 O-simpleAdditive 叠加来构建它的分音。弹一个键，看看带限在该音高下保留了哪些谐波。',
                     reviewed: 'mt' },
    },

    // ── Oscillator ──────────────────────────────────────────────────────────
    'position': {
        en: { t: 'Position',
              b: 'Where the scan sits in the bank, from frame 1 to the last frame. Turning it changes the timbre, because the oscillator reads a different stored cycle. Dragging on the bank stack moves the same control.' },
        fr: { t: 'Position',
              b: 'L’endroit du balayage dans la banque, de la trame 1 à la dernière. Le tourner change le timbre, car l’oscillateur lit un autre cycle stocké. Un glissement sur la pile de la banque agit sur la même commande.',
              reviewed: false },
        'zh-Hans': { t: '位置',
                     b: '扫描在波表库中的位置，从第 1 帧到最后一帧。转动它会改变音色，因为振荡器读取的是另一个存储的周期。在波表堆叠上拖动控制的是同一个参数。',
                     reviewed: 'mt' },
    },
    'interp': {
        en: { t: 'Interpolation',
              b: 'On: between two frames the oscillator blends them, so scanning morphs smoothly. Off: it jumps to the nearest frame, so you hear the scan in steps — the classic PPG sound.' },
        fr: { t: 'Interpolation',
              b: 'Activée : entre deux trames, l’oscillateur les fond l’une dans l’autre, et le balayage se transforme en douceur. Désactivée : il saute à la trame la plus proche, et vous entendez le balayage par paliers — le son PPG classique.',
              reviewed: false },
        'zh-Hans': { t: '插值',
                     b: '开：在两帧之间，振荡器会把它们融合，扫描因此平滑过渡。关：它跳到最近的一帧，你会听到逐级的扫描——经典的 PPG 声音。',
                     reviewed: 'mt' },
    },
    'bandlimit': {
        en: { t: 'Band-limiting',
              b: 'On: each octave reads a smoothed copy of the frames with the harmonics that would pass the Nyquist limit removed. Off: the raw frames are read at every pitch. Play high on a bright frame with it Off and you hear aliasing — extra tones that slide the wrong way as you play upward.' },
        fr: { t: 'Bande limitée',
              b: 'Activée : chaque octave lit une copie lissée des trames, sans les harmoniques qui dépasseraient la limite de Nyquist. Désactivée : les trames brutes sont lues à toutes les hauteurs. Jouez dans l’aigu sur une trame brillante en position désactivée et vous entendez le repliement — des sons parasites qui glissent dans le mauvais sens quand vous montez.',
              reviewed: false },
        'zh-Hans': { t: '带限',
                     b: '开：每个八度读取一份平滑过的帧副本，去掉会超过奈奎斯特极限的谐波。关：在所有音高下都读取原始帧。关闭时在明亮的帧上弹高音，你会听到混叠——当你向上弹奏时，多出来的音朝错误的方向滑动。',
                     reviewed: 'mt' },
    },
    'bit_depth': {
        en: { t: 'Bit Depth',
              b: 'Rounds every output sample to a fixed number of levels. Full is untouched. At 3 bits there are only 8 levels: the cycle turns into a staircase, and the steps add a gritty layer of new harmonics.' },
        fr: { t: 'Résolution',
              b: 'Arrondit chaque échantillon de sortie à un nombre fixe de niveaux. Full ne change rien. À 3 bits, il n’y a que 8 niveaux : le cycle devient un escalier, et les marches ajoutent une couche granuleuse de nouveaux harmoniques.',
              reviewed: false },
        'zh-Hans': { t: '位深',
                     b: '把每个输出采样取整到固定数量的电平。Full 表示不做处理。在 3 bit 时只有 8 个电平：周期变成阶梯，台阶会加入一层粗糙的新谐波。',
                     reviewed: 'mt' },
    },

    // ── Movement: LFO ───────────────────────────────────────────────────────
    'lfo_sync': {
        en: { t: 'LFO Sync',
              b: 'Free: the LFO rate is set in hertz. Tempo: the LFO follows the song tempo, and the Rate knob becomes a note length (Division).' },
        fr: { t: 'Synchro du LFO',
              b: 'Libre : la vitesse du LFO se règle en hertz. Tempo : le LFO suit le tempo du morceau, et le bouton Vitesse devient une durée de note (Division).',
              reviewed: false },
        'zh-Hans': { t: 'LFO 同步',
                     b: '自由：LFO 速率以赫兹设定。速度：LFO 跟随歌曲速度，速率旋钮变成音符时值（分割）。',
                     reviewed: 'mt' },
    },
    'lfo_shape': {
        en: { t: 'LFO Shape',
              b: 'The path the LFO traces as it moves Position: smooth (sine, triangle), a ramp (saw), a jump between two points (square), or a new random value each cycle (S&H).' },
        fr: { t: 'Forme du LFO',
              b: 'Le chemin que suit le LFO en déplaçant Position : doux (sinus, triangle), une rampe (dent de scie), un saut entre deux points (carré) ou une nouvelle valeur aléatoire à chaque cycle (S&H).',
              reviewed: false },
        'zh-Hans': { t: 'LFO 形状',
                     b: 'LFO 推动位置时走过的路径：平滑（正弦、三角）、斜坡（锯齿）、两点之间跳变（方波），或每个周期一个新的随机值（S&H）。',
                     reviewed: 'mt' },
    },
    'lfo_rate': {
        en: { t: 'LFO Rate',
              b: 'How many times a second the LFO sweeps Position. Slow rates make evolving pads; fast rates blur into a buzzing timbre.' },
        fr: { t: 'Vitesse du LFO',
              b: 'Le nombre de balayages de Position par seconde. Les vitesses lentes donnent des nappes qui évoluent ; les vitesses rapides se fondent en un timbre bourdonnant.',
              reviewed: false },
        'zh-Hans': { t: 'LFO 速率',
                     b: 'LFO 每秒扫过位置的次数。慢速产生缓慢演变的铺底音色；快速则融成嗡嗡的音色。',
                     reviewed: 'mt' },
    },
    'lfo_div': {
        en: { t: 'LFO Division',
              b: 'The length of one LFO sweep as a note value at the song tempo. 1/1 is one sweep per bar; dotted (.) and triplet (T) lengths are there too.' },
        fr: { t: 'Division du LFO',
              b: 'La durée d’un balayage du LFO en valeur de note, au tempo du morceau. 1/1 correspond à un balayage par mesure ; les valeurs pointées (.) et de triolet (T) sont aussi disponibles.',
              reviewed: false },
        'zh-Hans': { t: 'LFO 分割',
                     b: '一次 LFO 扫描的长度，以歌曲速度下的音符时值表示。1/1 表示每小节扫描一次；也有附点和三连音的时值。',
                     reviewed: 'mt' },
    },
    'lfo_depth': {
        en: { t: 'LFO Depth',
              b: 'How far the LFO swings Position to either side of the knob. At 100% with Position at 50%, it sweeps the whole bank from the first frame to the last. Watch the green ring move.' },
        fr: { t: 'Profondeur du LFO',
              b: 'L’amplitude du balancement de Position par le LFO, de part et d’autre du bouton. À 100 % avec Position à 50 %, il balaie toute la banque, de la première trame à la dernière. Regardez bouger l’anneau vert.',
              reviewed: false },
        'zh-Hans': { t: 'LFO 深度',
                     b: 'LFO 把位置向旋钮两侧摆动的幅度。深度 100% 且位置为 50% 时，它会扫过整个波表库，从第一帧到最后一帧。看看绿色圆环如何移动。',
                     reviewed: 'mt' },
    },

    // ── Movement: mod envelope ──────────────────────────────────────────────
    'menvCanvas': {
        en: { t: 'Mod Envelope',
              b: 'The shape of the modulation envelope that moves Position for each note. The dashed line shows its level right now while a note plays.' },
        fr: { t: 'Enveloppe de modulation',
              b: 'La forme de l’enveloppe de modulation qui déplace Position à chaque note. La ligne pointillée montre son niveau actuel pendant qu’une note joue.',
              reviewed: false },
        'zh-Hans': { t: '调制包络',
                     b: '每个音符推动位置的调制包络形状。音符演奏时，虚线显示它此刻的电平。',
                     reviewed: 'mt' },
    },
    'menv_attack': {
        en: { t: 'Mod Env Attack',
              b: 'How long the modulation envelope takes to rise after you press a key. Its movement is added to Position, so each note travels through the bank.' },
        fr: { t: 'Attaque de modulation',
              b: 'Le temps que met l’enveloppe de modulation à monter après l’appui sur une touche. Son mouvement s’ajoute à Position : chaque note voyage ainsi dans la banque.',
              reviewed: false },
        'zh-Hans': { t: '调制包络起音',
                     b: '按下琴键后，调制包络上升所需的时间。它的运动会叠加到位置上，所以每个音符都会在波表库中移动。',
                     reviewed: 'mt' },
    },
    'menv_decay': {
        en: { t: 'Mod Env Decay',
              b: 'How long the modulation envelope takes to fall from its peak to the sustain level.' },
        fr: { t: 'Déclin de modulation',
              b: 'Le temps que met l’enveloppe de modulation à redescendre de son sommet au niveau de maintien.',
              reviewed: false },
        'zh-Hans': { t: '调制包络衰减',
                     b: '调制包络从峰值下降到延音电平所需的时间。',
                     reviewed: 'mt' },
    },
    'menv_sustain': {
        en: { t: 'Mod Env Sustain',
              b: 'The level the modulation envelope holds while the key stays down.' },
        fr: { t: 'Maintien de modulation',
              b: 'Le niveau que garde l’enveloppe de modulation tant que la touche reste enfoncée.',
              reviewed: false },
        'zh-Hans': { t: '调制包络延音',
                     b: '按住琴键期间调制包络保持的电平。',
                     reviewed: 'mt' },
    },
    'menv_release': {
        en: { t: 'Mod Env Release',
              b: 'How long the modulation envelope takes to return to zero after you let go.' },
        fr: { t: 'Relâchement de modulation',
              b: 'Le temps que met l’enveloppe de modulation à revenir à zéro une fois la touche relâchée.',
              reviewed: false },
        'zh-Hans': { t: '调制包络释音',
                     b: '松开琴键后调制包络回到零所需的时间。',
                     reviewed: 'mt' },
    },
    'env_amount': {
        en: { t: 'Env Amount',
              b: 'How far the modulation envelope moves Position, and which way. Positive scans toward the back of the bank, negative toward the front. At 0% (the centre notch) the envelope does nothing.' },
        fr: { t: 'Quantité d’enveloppe',
              b: 'De combien l’enveloppe de modulation déplace Position, et dans quel sens. Une valeur positive balaie vers le fond de la banque, une valeur négative vers l’avant. À 0 % (le cran central), l’enveloppe n’a aucun effet.',
              reviewed: false },
        'zh-Hans': { t: '包络量',
                     b: '调制包络推动位置的距离和方向。正值向波表库后方扫描，负值向前方扫描。在 0%（中央刻度）时包络不起作用。',
                     reviewed: 'mt' },
    },

    // ── Amp + output ────────────────────────────────────────────────────────
    'ampCanvas': {
        en: { t: 'Amp Envelope',
              b: 'The shape of each note’s volume over time. The dashed line shows the level right now while a note plays.' },
        fr: { t: 'Enveloppe d’amplitude',
              b: 'La forme du volume de chaque note dans le temps. La ligne pointillée montre le niveau actuel pendant qu’une note joue.',
              reviewed: false },
        'zh-Hans': { t: '振幅包络',
                     b: '每个音符音量随时间变化的形状。音符演奏时，虚线显示此刻的电平。',
                     reviewed: 'mt' },
    },
    'amp_attack': {
        en: { t: 'Amp Attack',
              b: 'How long a note takes to fade in after you press a key.' },
        fr: { t: 'Attaque d’amplitude',
              b: 'Le temps que met une note à apparaître après l’appui sur une touche.',
              reviewed: false },
        'zh-Hans': { t: '振幅起音',
                     b: '按下琴键后，音符淡入所需的时间。',
                     reviewed: 'mt' },
    },
    'amp_decay': {
        en: { t: 'Amp Decay',
              b: 'How long a note takes to fall from its peak to the sustain level.' },
        fr: { t: 'Déclin d’amplitude',
              b: 'Le temps que met une note à redescendre de son sommet au niveau de maintien.',
              reviewed: false },
        'zh-Hans': { t: '振幅衰减',
                     b: '音符从峰值下降到延音电平所需的时间。',
                     reviewed: 'mt' },
    },
    'amp_sustain': {
        en: { t: 'Amp Sustain',
              b: 'The volume a note holds while the key stays down.' },
        fr: { t: 'Maintien d’amplitude',
              b: 'Le volume que garde une note tant que la touche reste enfoncée.',
              reviewed: false },
        'zh-Hans': { t: '振幅延音',
                     b: '按住琴键期间音符保持的音量。',
                     reviewed: 'mt' },
    },
    'amp_release': {
        en: { t: 'Amp Release',
              b: 'How long a note takes to fade out after you let go.' },
        fr: { t: 'Relâchement d’amplitude',
              b: 'Le temps que met une note à s’éteindre une fois la touche relâchée.',
              reviewed: false },
        'zh-Hans': { t: '振幅释音',
                     b: '松开琴键后，音符淡出所需的时间。',
                     reviewed: 'mt' },
    },
    'voice_mode': {
        en: { t: 'Voice Mode',
              b: 'Poly plays up to 16 notes at once. Mono plays one: the newest key wins, and letting go returns to a key you are still holding.' },
        fr: { t: 'Mode de voix',
              b: 'Poly joue jusqu’à 16 voix à la fois. Mono n’en joue qu’une : la touche la plus récente l’emporte, et en la relâchant vous revenez à une touche encore enfoncée.',
              reviewed: false },
        'zh-Hans': { t: '复音模式',
                     b: '复音最多同时演奏 16 个音符。单音只演奏一个：最新按下的键优先，松开它会回到你仍按住的键。',
                     reviewed: 'mt' },
    },
    'output_level': {
        en: { t: 'Output Level',
              b: 'The master volume. All the way down is silence (−inf).' },
        fr: { t: 'Niveau de sortie',
              b: 'Le volume général. Tout en bas, c’est le silence (−inf).',
              reviewed: false },
        'zh-Hans': { t: '输出电平',
                     b: '总音量。调到最低就是静音（−inf）。',
                     reviewed: 'mt' },
    },

    // ── Lesson presets ──────────────────────────────────────────────────────
    'lessonSteppedSmooth': {
        en: { t: 'Lesson · Stepped vs Smooth',
              b: 'A slow LFO sweeps the whole Sine → Saw bank with Interpolation Off. Hold a note, hear the steps, then switch Interpolation On.' },
        fr: { t: 'Leçon · Paliers ou fondu',
              b: 'Un LFO lent balaie toute la banque Sine → Saw, interpolation désactivée. Tenez une note, écoutez les paliers, puis activez l’interpolation.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 阶梯与平滑',
                     b: '一个慢速 LFO 在插值关闭时扫过整个 Sine → Saw 波表库。按住一个音，听那些台阶，然后打开插值。',
                     reviewed: 'mt' },
    },
    'lessonAliasDemo': {
        en: { t: 'Lesson · Alias Demo',
              b: 'A bright frame with Band-limiting Off. Play in the top octaves and listen for tones moving the wrong way, then switch Band-limiting On.' },
        fr: { t: 'Leçon · Repliement',
              b: 'Une trame brillante, bande limitée désactivée. Jouez dans les octaves aiguës et écoutez les sons qui partent dans le mauvais sens, puis activez la bande limitée.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 混叠演示',
                     b: '一个明亮的帧，带限关闭。在最高的几个八度弹奏，听那些朝错误方向移动的音，然后打开带限。',
                     reviewed: 'mt' },
    },
    'lessonDriveSweep': {
        en: { t: 'Lesson · Drive Sweep',
              b: 'The envelope sweeps each note back through the Drive bank: every note strikes fully driven and relaxes to a clean sine as the envelope decays, like turning a drive knob down.' },
        fr: { t: 'Leçon · Balayage saturé',
              b: 'L’enveloppe fait retraverser la banque Drive à chaque note : chaque note attaque pleinement saturée, puis revient à une sinusoïde propre à mesure que l’enveloppe décroît, comme si l’on baissait un bouton de saturation.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 过载扫描',
                     b: '包络让每个音符反向扫过 Drive 波表库：每个音符起音时过载最强，随着包络衰减回到干净的正弦波，就像把过载旋钮调低。',
                     reviewed: 'mt' },
    },
    'lessonVowelPad': {
        en: { t: 'Lesson · Vowel Pad',
              b: 'A slow LFO drifts through the vowels of the Formant bank. Play an octave higher and notice the vowels move up with the pitch.' },
        fr: { t: 'Leçon · Nappe de voyelles',
              b: 'Un LFO lent dérive à travers les voyelles de la banque Formant. Jouez une octave plus haut et remarquez que les voyelles montent avec la hauteur.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 元音铺底',
                     b: '一个慢速 LFO 在 Formant 波表库的元音之间漂移。高八度弹奏，注意元音随音高一起上移。',
                     reviewed: 'mt' },
    },
    'lessonPpg8bit': {
        en: { t: 'Lesson · 8-bit PPG',
              b: 'Stepped scanning at 8 bits, with a random LFO choosing frames — the sound of early digital wavetable synths.' },
        fr: { t: 'Leçon · PPG 8 bits',
              b: 'Un balayage par paliers à 8 bits, avec un LFO aléatoire qui choisit les trames — le son des premiers synthés numériques à tables d’ondes.',
              reviewed: false },
        'zh-Hans': { t: '课程 · 8 bit PPG',
                     b: '8 bit 下的阶梯式扫描，由随机 LFO 选择帧——早期数字波表合成器的声音。',
                     reviewed: 'mt' },
    },

    // ── Keyboard ────────────────────────────────────────────────────────────
    'keyboard': {
        en: { t: 'Keyboard',
              b: 'Play notes without a MIDI controller: click the keys, or use the computer keys A to K. The panels follow the newest note. Z and X, or the arrow buttons, change the octave.' },
        fr: { t: 'Clavier',
              b: 'Jouez des notes sans contrôleur MIDI : cliquez sur les touches, ou utilisez les touches A à K de l’ordinateur. Les panneaux suivent la note la plus récente. Z et X, ou les boutons fléchés, changent d’octave.',
              reviewed: false },
        'zh-Hans': { t: '键盘',
                     b: '无需 MIDI 控制器即可演奏：点击琴键，或使用电脑键盘的 A 到 K。面板跟随最新的音符。Z 和 X 或箭头按钮可切换八度。',
                     reviewed: 'mt' },
    },
    'octDown': {
        en: { t: 'Octave down',
              b: 'Moves the on-screen keyboard and the computer keys down one octave. Shortcut: Z.' },
        fr: { t: 'Octave inférieure',
              b: 'Descend le clavier à l’écran et les touches de l’ordinateur d’une octave. Raccourci : Z.',
              reviewed: false },
        'zh-Hans': { t: '降八度',
                     b: '将屏幕键盘和电脑按键降低一个八度。快捷键：Z。',
                     reviewed: 'mt' },
    },
    'octUp': {
        en: { t: 'Octave up',
              b: 'Moves the on-screen keyboard and the computer keys up one octave. Shortcut: X.' },
        fr: { t: 'Octave supérieure',
              b: 'Monte le clavier à l’écran et les touches de l’ordinateur d’une octave. Raccourci : X.',
              reviewed: false },
        'zh-Hans': { t: '升八度',
                     b: '将屏幕键盘和电脑按键升高一个八度。快捷键：X。',
                     reviewed: 'mt' },
    },

    // ── Presets (FUNC-08) ───────────────────────────────────────────────────
    'presetSelect': {
        en: { t: 'Presets',
              b: 'Factory presets come first — each one shows one idea. Your own presets follow. Loading a preset never changes Output Level or the imported audio. A dot means you have changed it since loading.' },
        fr: { t: 'Préréglages',
              b: 'Les préréglages d’usine d’abord — chacun montre une idée. Les vôtres suivent. Charger un préréglage ne change jamais le niveau de sortie ni l’audio importé. Un point signale une modification depuis le chargement.',
              reviewed: false },
        'zh-Hans': { t: '预设',
                     b: '出厂预设在前——每个演示一个概念，你自己的预设在后。载入预设不会改变输出电平，也不会改变导入的音频。圆点表示载入后你已作修改。',
                     reviewed: 'mt' },
    },
    'presetPrev': {
        en: { t: 'Previous preset',
              b: 'Loads the previous preset in the list.' },
        fr: { t: 'Préréglage précédent',
              b: 'Charge le préréglage précédent de la liste.',
              reviewed: false },
        'zh-Hans': { t: '上一个预设',
                     b: '载入列表中的上一个预设。',
                     reviewed: 'mt' },
    },
    'presetNext': {
        en: { t: 'Next preset',
              b: 'Loads the next preset in the list.' },
        fr: { t: 'Préréglage suivant',
              b: 'Charge le préréglage suivant de la liste.',
              reviewed: false },
        'zh-Hans': { t: '下一个预设',
                     b: '载入列表中的下一个预设。',
                     reviewed: 'mt' },
    },
    'presetSave': {
        en: { t: 'Save preset',
              b: 'Saves every setting except Output Level as one of your presets. On the Imported bank only the bank choice is saved, not the audio.' },
        fr: { t: 'Enregistrer le préréglage',
              b: 'Enregistre tous les réglages sauf le niveau de sortie dans un de vos préréglages. Avec la banque Imported, seul le choix de banque est enregistré, pas l’audio.',
              reviewed: false },
        'zh-Hans': { t: '保存预设',
                     b: '将除输出电平外的所有设置保存为你的预设。使用 Imported 波表库时只保存所选波表库，不保存音频。',
                     reviewed: 'mt' },
    },
    'presetDelete': {
        en: { t: 'Delete preset',
              b: 'Deletes the selected preset of your own. Factory presets cannot be deleted.' },
        fr: { t: 'Supprimer le préréglage',
              b: 'Supprime le préréglage sélectionné, s’il est à vous. Les préréglages d’usine ne peuvent pas être supprimés.',
              reviewed: false },
        'zh-Hans': { t: '删除预设',
                     b: '删除所选的用户预设。出厂预设无法删除。',
                     reviewed: 'mt' },
    },
});

// ============================================================================
// LABELS — visible captions, aria names and every JS-written status line.
// A `{token}` is filled by setLabel(el, key, vars); numbers stay language-
// neutral. State-dependent copy is TWO keys written in two branches, never one
// key with an inflection decided in JS (check-i18n [13]).
// ============================================================================
export const LABELS = Object.freeze({
    // ── Chrome ──────────────────────────────────────────────────────────────
    'ui.on':             { en: { t: 'On' },  fr: { t: 'Marche', reviewed: false }, 'zh-Hans': { t: '开', reviewed: 'mt' } },
    'ui.off':            { en: { t: 'Off' }, fr: { t: 'Arrêt', reviewed: false },  'zh-Hans': { t: '关', reviewed: 'mt' } },
    'label.hoverHelp':   { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: false }, 'zh-Hans': { t: '悬停帮助', reviewed: 'mt' } },
    'aria.langSelect':   { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false }, 'zh-Hans': { t: '界面语言', reviewed: 'mt' } },
    'aria.helpToggle':   { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: false }, 'zh-Hans': { t: '开关悬停帮助', reviewed: 'mt' } },
    'label.subtitle':    { en: { t: 'Wavetable · Frame Bank & Scan · A Field Guide' },
                           fr: { t: 'Table d’ondes · Trames, balayage · Guide de terrain', reviewed: false },
                           'zh-Hans': { t: '波表 · 帧库与扫描 · 野外指南', reviewed: 'mt' } },

    // ── Header: bank, import, source readout ────────────────────────────────
    'label.bank':          { en: { t: 'Bank' }, fr: { t: 'Banque', reviewed: false }, 'zh-Hans': { t: '波表库', reviewed: 'mt' } },
    'import.label':        { en: { t: 'Import audio…' }, fr: { t: 'Importer…', reviewed: false }, 'zh-Hans': { t: '导入音频…', reviewed: 'mt' } },
    'import.busy':         { en: { t: 'Slicing…' }, fr: { t: 'Découpage…', reviewed: false }, 'zh-Hans': { t: '切分中…', reviewed: 'mt' } },
    'src.builtIn':         { en: { t: 'built-in · generated from a formula' }, fr: { t: 'intégrée · calculée par une formule', reviewed: false }, 'zh-Hans': { t: '内置 · 由公式生成', reviewed: 'mt' } },
    'src.builtInMeta':     { en: { t: '32 frames · 2048 samples each' }, fr: { t: '32 trames · 2048 échantillons chacune', reviewed: false }, 'zh-Hans': { t: '32 帧 · 每帧 2048 个采样', reviewed: 'mt' } },
    'src.empty':           { en: { t: 'no audio imported yet' }, fr: { t: 'aucun audio importé', reviewed: false }, 'zh-Hans': { t: '尚未导入音频', reviewed: 'mt' } },
    'src.emptyMeta':       { en: { t: '0 frames · the oscillator is silent' }, fr: { t: '0 trame · l’oscillateur est muet', reviewed: false }, 'zh-Hans': { t: '0 帧 · 振荡器无声', reviewed: 'mt' } },
    'src.importedMeta':    { en: { t: '{n} frames · sliced every 2048 samples' }, fr: { t: '{n} trames · découpées tous les 2048 échantillons', reviewed: false }, 'zh-Hans': { t: '{n} 帧 · 每 2048 个采样切分一次', reviewed: 'mt' } },
    'src.importedMetaOne': { en: { t: '1 frame · sliced every 2048 samples' }, fr: { t: '1 trame · découpée tous les 2048 échantillons', reviewed: false }, 'zh-Hans': { t: '1 帧 · 每 2048 个采样切分一次', reviewed: 'mt' } },
    'import.err.tooShort':   { en: { t: 'too short — needs one 2048-sample frame (≈ 46 ms)' }, fr: { t: 'trop court — il faut 2048 échantillons (≈ 46 ms)', reviewed: false }, 'zh-Hans': { t: '太短——至少需要一帧 2048 个采样（约 46 ms）', reviewed: 'mt' } },
    'import.err.unreadable': { en: { t: 'could not read that file' }, fr: { t: 'ce fichier n’a pas pu être ouvert', reviewed: false }, 'zh-Hans': { t: '无法读取该文件', reviewed: 'mt' } },
    // W2 (D-AK): the DROP cap (16 MiB); the Import button streams and has none.
    // .src-meta content is 240 px (10.5 px italic): en 175.4 / fr 218.4 / zh 197.6.
    'import.err.tooLarge':   { en: { t: 'too large to drop — use the Import button' }, fr: { t: 'trop volumineux pour un dépôt — utilisez Importer', reviewed: false }, 'zh-Hans': { t: '文件过大，无法拖放——请使用导入按钮', reviewed: 'mt' } },
    'import.err.folder':     { en: { t: 'drop one audio file, not a folder' }, fr: { t: 'déposez un fichier audio, pas un dossier', reviewed: false }, 'zh-Hans': { t: '请拖放单个音频文件，而不是文件夹', reviewed: 'mt' } },
    'import.err.type':       { en: { t: 'not an audio file (WAV, AIFF, FLAC, Ogg)' }, fr: { t: 'pas un fichier audio (WAV, AIFF, FLAC, Ogg)', reviewed: false }, 'zh-Hans': { t: '不是音频文件（WAV、AIFF、FLAC、Ogg）', reviewed: 'mt' } },
    // N9: a session saved by a newer build; the blob is kept, never played.
    // en 213.9 / fr 224.6 / zh 208.8 of the 240 px .src-meta content.
    'import.err.unsupported': { en: { t: 'saved by a newer version — kept, not playable here' }, fr: { t: 'd’une version plus récente — conservé, non jouable ici', reviewed: false }, 'zh-Hans': { t: '由更新版本保存——已保留，此处无法播放', reviewed: 'mt' } },
    'import.err.generic':    { en: { t: 'import failed — the current bank is unchanged' }, fr: { t: 'échec de l’import — la banque actuelle est inchangée', reviewed: false }, 'zh-Hans': { t: '导入失败——当前波表库未改变', reviewed: 'mt' } },

    // ── Panel captions ──────────────────────────────────────────────────────
    'label.bankStack':        { en: { t: 'Bank Stack' }, fr: { t: 'Pile de la banque', reviewed: false }, 'zh-Hans': { t: '波表堆叠', reviewed: 'mt' } },
    'label.bankStackHint':    { en: { t: 'every frame, front to back — drag to scan' }, fr: { t: 'toutes les trames, de l’avant au fond — glissez pour balayer', reviewed: false }, 'zh-Hans': { t: '所有帧，由前到后——拖动即可扫描', reviewed: 'mt' } },
    'label.legendKnob':       { en: { t: 'Position knob' }, fr: { t: 'bouton Position', reviewed: false }, 'zh-Hans': { t: '位置旋钮', reviewed: 'mt' } },
    'label.legendEff':        { en: { t: 'after LFO + env' }, fr: { t: 'après LFO + env.', reviewed: false }, 'zh-Hans': { t: '经 LFO 与包络后', reviewed: 'mt' } },
    'label.emptyBig':         { en: { t: 'Import audio to fill this bank' }, fr: { t: 'Importez de l’audio pour remplir cette banque', reviewed: false }, 'zh-Hans': { t: '导入音频来填充这个波表库', reviewed: 'mt' } },
    'label.emptySmall':       { en: { t: 'any sound file → sliced into 2048-sample frames (up to 256)' }, fr: { t: 'tout fichier son → découpé en trames de 2048 échantillons (256 au maximum)', reviewed: false }, 'zh-Hans': { t: '任意声音文件 → 切成每帧 2048 个采样（最多 256 帧）', reviewed: 'mt' } },
    'label.currentCycle':     { en: { t: 'Current Cycle' }, fr: { t: 'Cycle actuel', reviewed: false }, 'zh-Hans': { t: '当前周期', reviewed: 'mt' } },
    'label.currentCycleHint': { en: { t: 'the one cycle being read' }, fr: { t: 'le cycle en cours de lecture', reviewed: false }, 'zh-Hans': { t: '正在读取的那个周期', reviewed: 'mt' } },
    'label.cycleFoot':        { en: { t: 'one cycle · 2048 samples' }, fr: { t: 'un cycle · 2048 échantillons', reviewed: false }, 'zh-Hans': { t: '一个周期 · 2048 个采样', reviewed: 'mt' } },
    'label.harmonics':        { en: { t: 'Harmonics' }, fr: { t: 'Harmoniques', reviewed: false }, 'zh-Hans': { t: '谐波', reviewed: 'mt' } },
    'label.harmonicsHint':    { en: { t: '1–32 of that cycle, in dB' }, fr: { t: '1 à 32 de ce cycle, en dB', reviewed: false }, 'zh-Hans': { t: '该周期的第 1–32 次，单位 dB', reviewed: 'mt' } },

    // ── Panel readouts (JS, through setLabel) ───────────────────────────────
    'readout.frame':        { en: { t: 'frame {n}' }, fr: { t: 'trame {n}', reviewed: false }, 'zh-Hans': { t: '第 {n} 帧', reviewed: 'mt' } },
    'readout.frameOf':      { en: { t: '/ {total}' }, fr: { t: '/ {total}', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '/ {total}', reviewed: 'mt', sameAsEn: true } },
    'readout.heroFrame':    { en: { t: 'frame {n} / {total}' }, fr: { t: 'trame {n} / {total}', reviewed: false }, 'zh-Hans': { t: '帧 {n} / {total}', reviewed: 'mt' } },
    'readout.noFrames':     { en: { t: 'no frames' }, fr: { t: 'aucune trame', reviewed: false }, 'zh-Hans': { t: '无帧', reviewed: 'mt' } },
    'readout.importedName': { en: { t: 'Imported · {file}' }, fr: { t: 'Importée · {file}', reviewed: false }, 'zh-Hans': { t: '已导入 · {file}', reviewed: 'mt' } },
    'mix.none':             { en: { t: 'no frames yet' }, fr: { t: 'pas encore de trame', reviewed: false }, 'zh-Hans': { t: '尚无帧', reviewed: 'mt' } },
    'mix.stepped':          { en: { t: 'stepped · snaps to frame {f}' }, fr: { t: 'par paliers · se cale sur la trame {f}', reviewed: false }, 'zh-Hans': { t: '阶梯式 · 吸附到第 {f} 帧', reviewed: 'mt' } },
    'mix.on':               { en: { t: 'on frame {f}' }, fr: { t: 'sur la trame {f}', reviewed: false }, 'zh-Hans': { t: '位于第 {f} 帧', reviewed: 'mt' } },
    'mix.blend':            { en: { t: '{a}% f{i}  +  {b}% f{j}' }, fr: { t: '{a} % t{i}  +  {b} % t{j}', reviewed: false }, 'zh-Hans': { t: '{a}% 第 {i} 帧 + {b}% 第 {j} 帧', reviewed: 'mt' } },
    'lesson.bankSineSaw':    { en: { t: 'frame k holds harmonics 1 … k at 1/n — a frozen additive build-up' }, fr: { t: 'la trame k contient les harmoniques 1 … k à 1/n — une construction additive figée', reviewed: false }, 'zh-Hans': { t: '第 k 帧包含第 1 … k 次谐波，幅度为 1/n——一次冻结的加法叠加', reviewed: 'mt' } },
    'lesson.bankSineSquare': { en: { t: 'odd harmonics only — the newest one fades in frame by frame, up to h31' }, fr: { t: 'harmoniques impairs seulement — le plus récent apparaît trame après trame, jusqu’à h31', reviewed: false }, 'zh-Hans': { t: '只有奇次谐波——最新的一个逐帧淡入，直到 h31', reviewed: 'mt' } },
    'lesson.bankPulse':      { en: { t: 'a pulse narrowing from 50% to 3% — the first gap in the spectrum climbs from h2 to h32' }, fr: { t: 'une impulsion qui se rétrécit de 50 % à 3 % — le premier creux du spectre monte de h2 à h32', reviewed: false }, 'zh-Hans': { t: '脉冲从 50% 收窄到 3%——频谱中的第一个空缺从 h2 升到 h32', reviewed: 'mt' } },
    'lesson.bankFormant':    { en: { t: 'vowels A → E → I → O → U, baked in at 110 Hz — they move with pitch, unlike a real voice' }, fr: { t: 'voyelles A → E → I → O → U, figées à 110 Hz — elles suivent la note, contrairement à la voix', reviewed: false }, 'zh-Hans': { t: '元音 A → E → I → O → U，固定在 110 Hz——随音高移动，不同于真实人声', reviewed: 'mt' } },
    'lesson.bankDrive':      { en: { t: 'a sine pushed into rising soft clipping — scanning sounds like turning up drive' }, fr: { t: 'une sinusoïde poussée vers un écrêtage doux croissant — balayer revient à monter la saturation', reviewed: false }, 'zh-Hans': { t: '正弦波被推向逐渐增强的软削波——扫描听起来就像调高过载', reviewed: 'mt' } },
    'lesson.bankImported':   { en: { t: 'slices aren’t matched to the pitch — each loop seam jumps, and you hear it as a buzz' }, fr: { t: 'tranches non calées sur la hauteur — chaque raccord de boucle saute, et bourdonne', reviewed: false }, 'zh-Hans': { t: '切片没有对齐音高——每个循环接缝都会跳变，你会听到嗡嗡声', reviewed: 'mt' } },
    'cyc.silence':  { en: { t: 'silence' }, fr: { t: 'silence', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '静音', reviewed: 'mt' } },
    'cyc.frames':   { en: { t: 'frames {a} ↔ {b}' }, fr: { t: 'trames {a} ↔ {b}', reviewed: false }, 'zh-Hans': { t: '第 {a} ↔ {b} 帧', reviewed: 'mt' } },
    'cyc.frame':    { en: { t: 'frame {a}' }, fr: { t: 'trame {a}', reviewed: false }, 'zh-Hans': { t: '第 {a} 帧', reviewed: 'mt' } },
    'cyc.full':     { en: { t: 'full resolution' }, fr: { t: 'pleine résolution', reviewed: false }, 'zh-Hans': { t: '完整分辨率', reviewed: 'mt' } },
    'cyc.bits':     { en: { t: '{b} bit · {lv} levels' }, fr: { t: '{b} bits · {lv} niveaux', reviewed: false }, 'zh-Hans': { t: '{b} bit · {lv} 个电平', reviewed: 'mt' } },
    'cyc.seam':     { en: { t: 'loop seam jumps {j} → buzz' }, fr: { t: 'le raccord de boucle saute de {j} → bourdonnement', reviewed: false }, 'zh-Hans': { t: '循环接缝跳变 {j} → 嗡嗡声', reviewed: 'mt' } },
    'harm.noNote':    { en: { t: 'no note held' }, fr: { t: 'aucune note jouée', reviewed: false }, 'zh-Hans': { t: '未按住音符', reviewed: 'mt' } },
    'harm.noNoteSub': { en: { t: 'raw frame · play a key to see the mip copy' }, fr: { t: 'trame brute · jouez une touche pour voir la copie mip', reviewed: false }, 'zh-Hans': { t: '原始帧 · 弹一个键查看 mip 副本', reviewed: 'mt' } },
    'harm.keeps':     { en: { t: 'octave copy keeps h1–{k}' }, fr: { t: 'la copie d’octave garde h1–{k}', reviewed: false }, 'zh-Hans': { t: '八度副本保留 h1–{k}', reviewed: 'mt' } },
    'harm.keepsAll':  { en: { t: 'octave copy keeps h1–{k} (all shown)' }, fr: { t: 'la copie d’octave garde h1–{k} (toutes visibles)', reviewed: false }, 'zh-Hans': { t: '八度副本保留 h1–{k}（全部可见）', reviewed: 'mt' } },
    'harm.folds':     { en: { t: 'above h{k} folds back — aliasing' }, fr: { t: 'au-delà de h{k}, repliement', reviewed: false }, 'zh-Hans': { t: 'h{k} 以上折返——混叠', reviewed: 'mt' } },
    'harm.raw':       { en: { t: 'raw frame · Nyquist at h{k}' }, fr: { t: 'trame brute · Nyquist à h{k}', reviewed: false }, 'zh-Hans': { t: '原始帧 · 奈奎斯特位于 h{k}', reviewed: 'mt' } },

    // ── Oscillator ──────────────────────────────────────────────────────────
    'label.oscillator': { en: { t: 'Oscillator' }, fr: { t: 'Oscillateur', reviewed: false }, 'zh-Hans': { t: '振荡器', reviewed: 'mt' } },
    'label.position':   { en: { t: 'Position' }, fr: { t: 'Position', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '位置', reviewed: 'mt' } },
    'label.interp':     { en: { t: 'Interpolation' }, fr: { t: 'Interpolation', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '插值', reviewed: 'mt' } },
    'label.bandlimit':  { en: { t: 'Band-limiting' }, fr: { t: 'Bande limitée', reviewed: false }, 'zh-Hans': { t: '带限', reviewed: 'mt' } },
    'label.bitDepth':   { en: { t: 'Bit Depth' }, fr: { t: 'Résolution', reviewed: false }, 'zh-Hans': { t: '位深', reviewed: 'mt' } },

    // ── Movement ────────────────────────────────────────────────────────────
    'label.movement':      { en: { t: 'Movement' }, fr: { t: 'Mouvement', reviewed: false }, 'zh-Hans': { t: '运动', reviewed: 'mt' } },
    'label.route':         { en: { t: '→ position' }, fr: { t: '→ position', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '→ 位置', reviewed: 'mt' } },
    'label.lfo':           { en: { t: 'LFO' }, fr: { t: 'LFO', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: 'LFO', reviewed: 'mt', sameAsEn: true } },
    'label.free':          { en: { t: 'Free' }, fr: { t: 'Libre', reviewed: false }, 'zh-Hans': { t: '自由', reviewed: 'mt' } },
    'label.tempo':         { en: { t: 'Tempo' }, fr: { t: 'Tempo', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '速度', reviewed: 'mt' } },
    'aria.shapeSine':      { en: { t: 'Sine' }, fr: { t: 'Sinus', reviewed: false }, 'zh-Hans': { t: '正弦', reviewed: 'mt' } },
    'aria.shapeTriangle':  { en: { t: 'Triangle' }, fr: { t: 'Triangle', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '三角', reviewed: 'mt' } },
    'aria.shapeSaw':       { en: { t: 'Saw' }, fr: { t: 'Dent de scie', reviewed: false }, 'zh-Hans': { t: '锯齿', reviewed: 'mt' } },
    'aria.shapeSquare':    { en: { t: 'Square' }, fr: { t: 'Carré', reviewed: false }, 'zh-Hans': { t: '方波', reviewed: 'mt' } },
    'aria.shapeSH':        { en: { t: 'Sample and hold' }, fr: { t: 'Échantillonnage-blocage', reviewed: false }, 'zh-Hans': { t: '采样保持', reviewed: 'mt' } },
    'label.rate':          { en: { t: 'Rate' }, fr: { t: 'Vitesse', reviewed: false }, 'zh-Hans': { t: '速率', reviewed: 'mt' } },
    'label.division':      { en: { t: 'Division' }, fr: { t: 'Division', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '分割', reviewed: 'mt' } },
    'label.depth':         { en: { t: 'Depth' }, fr: { t: 'Profondeur', reviewed: false }, 'zh-Hans': { t: '深度', reviewed: 'mt' } },
    'label.modEnv':        { en: { t: 'Mod Envelope' }, fr: { t: 'Enveloppe de modulation', reviewed: false }, 'zh-Hans': { t: '调制包络', reviewed: 'mt' } },
    'label.amount':        { en: { t: 'Amount' }, fr: { t: 'Quantité', reviewed: false }, 'zh-Hans': { t: '量', reviewed: 'mt' } },
    'label.attack':        { en: { t: 'Attack' }, fr: { t: 'Attaque', reviewed: false }, 'zh-Hans': { t: '起音', reviewed: 'mt' } },
    'label.decay':         { en: { t: 'Decay' }, fr: { t: 'Déclin', reviewed: false }, 'zh-Hans': { t: '衰减', reviewed: 'mt' } },
    'label.sustain':       { en: { t: 'Sustain' }, fr: { t: 'Maintien', reviewed: false }, 'zh-Hans': { t: '延音', reviewed: 'mt' } },
    // The glossary abbreviation: the full root (Relâchement) is ~80 px in a 52 px
    // .knob-cell.sm between two neighbours 4 px away.
    'label.release':       { en: { t: 'Release' }, fr: { t: 'Relâch.', reviewed: false }, 'zh-Hans': { t: '释音', reviewed: 'mt' } },

    // ── Amp + output ────────────────────────────────────────────────────────
    'label.ampOutput': { en: { t: 'Amp + Output' }, fr: { t: 'Ampli + sortie', reviewed: false }, 'zh-Hans': { t: '振幅 + 输出', reviewed: 'mt' } },
    'label.ampEnv':    { en: { t: 'Amp Envelope' }, fr: { t: 'Enveloppe d’amplitude', reviewed: false }, 'zh-Hans': { t: '振幅包络', reviewed: 'mt' } },
    'label.voices':    { en: { t: 'Voices' }, fr: { t: 'Voix', reviewed: false }, 'zh-Hans': { t: '复音数', reviewed: 'mt' } },
    'label.poly':      { en: { t: 'Poly' }, fr: { t: 'Poly', reviewed: false, sameAsEn: true }, 'zh-Hans': { t: '复音', reviewed: 'mt' } },
    'label.mono':      { en: { t: 'Mono' }, fr: { t: 'Mono', reviewed: false, sameAsEn: true },
                         'zh-Hans': { t: '单音', reviewed: 'mt',
                                      termNote: 'Voice-mode Mono is monophonic playing (one note at a time), not a mono audio channel; the glossary form 单声道 names the channel and would mislead here.' } },
    'label.output':    { en: { t: 'Output' }, fr: { t: 'Sortie', reviewed: false }, 'zh-Hans': { t: '输出', reviewed: 'mt' } },

    // ── Lesson presets ──────────────────────────────────────────────────────
    'label.lessons':             { en: { t: 'Lesson Presets' }, fr: { t: 'Leçons', reviewed: false }, 'zh-Hans': { t: '教学预设', reviewed: 'mt' } },
    'label.lessonSteppedSmooth': { en: { t: 'Stepped vs Smooth' }, fr: { t: 'Paliers ou fondu', reviewed: false }, 'zh-Hans': { t: '阶梯与平滑', reviewed: 'mt' } },
    'label.lessonAliasDemo':     { en: { t: 'Alias Demo' }, fr: { t: 'Repliement', reviewed: false }, 'zh-Hans': { t: '混叠演示', reviewed: 'mt' } },
    'label.lessonDriveSweep':    { en: { t: 'Drive Sweep' }, fr: { t: 'Balayage saturé', reviewed: false }, 'zh-Hans': { t: '过载扫描', reviewed: 'mt' } },
    'label.lessonVowelPad':      { en: { t: 'Vowel Pad' }, fr: { t: 'Voyelles', reviewed: false }, 'zh-Hans': { t: '元音铺底', reviewed: 'mt' } },
    'label.lessonPpg8bit':       { en: { t: '8-bit PPG' }, fr: { t: 'PPG 8 bits', reviewed: false }, 'zh-Hans': { t: '8 bit PPG', reviewed: 'mt' } },
    'tour.hint':                 { en: { t: 'Hover any control for an explanation · pick a lesson to hear one idea.' }, fr: { t: 'Survolez une commande pour une explication · choisissez une leçon pour entendre une idée.', reviewed: false }, 'zh-Hans': { t: '悬停在任意控件上查看说明 · 选择一节课来聆听一个概念。', reviewed: 'mt' } },
    'tour.caption.steppedSmooth': { en: { t: 'Interpolation Off: the scan jumps frame by frame. Switch it On to hear the morph.' }, fr: { t: 'Interpolation désactivée : le balayage saute de trame en trame. Activez-la pour entendre la transformation.', reviewed: false }, 'zh-Hans': { t: '插值关闭：扫描逐帧跳变。打开插值即可听到平滑过渡。', reviewed: 'mt' } },
    'tour.caption.aliasDemo':     { en: { t: 'Band-limiting Off: play these high keys, hear the wrong-way tones, then switch it On.' }, fr: { t: 'Bande limitée désactivée : jouez ces touches aiguës, écoutez les sons à contresens, puis activez-la.', reviewed: false }, 'zh-Hans': { t: '带限关闭：弹这些高音键，听那些反向移动的音，然后打开带限。', reviewed: 'mt' } },
    'tour.caption.driveSweep':    { en: { t: 'Each note strikes driven and relaxes back to a sine as the envelope decays.' }, fr: { t: 'Chaque note attaque saturée, puis revient à une sinusoïde à mesure que l’enveloppe décroît.', reviewed: false }, 'zh-Hans': { t: '每个音符起音时过载最强，随着包络衰减回到正弦波。', reviewed: 'mt' } },
    'tour.caption.vowelPad':      { en: { t: 'A slow LFO drifts A → E → I → O → U. Play an octave up: the vowels move too.' }, fr: { t: 'Un LFO lent dérive de A → E → I → O → U. Jouez une octave plus haut : les voyelles bougent aussi.', reviewed: false }, 'zh-Hans': { t: '慢速 LFO 在 A → E → I → O → U 之间漂移。高八度弹奏：元音也会随之移动。', reviewed: 'mt' } },
    'tour.caption.ppg8bit':       { en: { t: '8 bits, stepped frames and a random LFO — early digital wavetable grit.' }, fr: { t: '8 bits, trames par paliers et LFO aléatoire — le grain des premières tables d’ondes numériques.', reviewed: false }, 'zh-Hans': { t: '8 bit、阶梯式帧和随机 LFO——早期数字波表的粗粝感。', reviewed: 'mt' } },

    // ── Keyboard ────────────────────────────────────────────────────────────
    'label.play':    { en: { t: 'Play' }, fr: { t: 'Jouer', reviewed: false }, 'zh-Hans': { t: '演奏', reviewed: 'mt' } },
    'label.kbdHint': { en: { t: 'keys A S D F G H J K · W E T Y U' },
                       fr: { t: 'touches A S D F G H J K · W E T Y U', reviewed: false },
                       'zh-Hans': { t: '按键 ASDFGHJK · WETYU', reviewed: 'mt' } },
    'aria.octDown':  { en: { t: 'Octave down' }, fr: { t: 'Octave inférieure', reviewed: false }, 'zh-Hans': { t: '降八度', reviewed: 'mt' } },
    'aria.octUp':    { en: { t: 'Octave up' }, fr: { t: 'Octave supérieure', reviewed: false }, 'zh-Hans': { t: '升八度', reviewed: 'mt' } },

    // ── Presets (FUNC-08) ───────────────────────────────────────────────────
    // Widths (EB Garamond + PingFang, headless Chromium) are in index.html
    // beside each pin. The popover message line is 330 px, nowrap.
    'label.presets':          { en: { t: 'Presets' }, fr: { t: 'Préréglages', reviewed: false }, 'zh-Hans': { t: '预设', reviewed: 'mt' } },
    'preset.save':            { en: { t: 'Save' }, fr: { t: 'Enregistrer', reviewed: false }, 'zh-Hans': { t: '保存', reviewed: 'mt' } },
    'preset.delete':          { en: { t: 'Delete' }, fr: { t: 'Supprimer', reviewed: false }, 'zh-Hans': { t: '删除', reviewed: 'mt' } },
    'preset.cancel':          { en: { t: 'Cancel' }, fr: { t: 'Annuler', reviewed: false }, 'zh-Hans': { t: '取消', reviewed: 'mt' } },
    'preset.saveTitle':       { en: { t: 'Save preset' }, fr: { t: 'Enregistrer le préréglage', reviewed: false }, 'zh-Hans': { t: '保存预设', reviewed: 'mt' } },
    'preset.namePlaceholder': { en: { t: 'Preset name' }, fr: { t: 'Nom du préréglage', reviewed: false }, 'zh-Hans': { t: '预设名称', reviewed: 'mt' } },
    'preset.errFactoryName':  { en: { t: 'A factory preset has that name — choose another.' }, fr: { t: 'Nom d’un préréglage d’usine — choisissez-en un autre.', reviewed: false }, 'zh-Hans': { t: '该名称属于出厂预设——请换一个。', reviewed: 'mt' } },
    'preset.errEmpty':        { en: { t: 'Type a name first.' }, fr: { t: 'Saisissez d’abord un nom.', reviewed: false }, 'zh-Hans': { t: '请先输入名称。', reviewed: 'mt' } },
    'preset.errSave':         { en: { t: 'Could not save the preset.' }, fr: { t: 'Impossible d’enregistrer le préréglage.', reviewed: false }, 'zh-Hans': { t: '无法保存预设。', reviewed: 'mt' } },
    'preset.confirmReplace':  { en: { t: 'Replace “{name}”?' }, fr: { t: 'Remplacer « {name} » ?', reviewed: false }, 'zh-Hans': { t: '替换“{name}”？', reviewed: 'mt' } },
    // Straight quotes in the en: the zh glossary key is 'delete preset "{name}"?'.
    'preset.confirmDelete':   { en: { t: 'Delete preset "{name}"?' }, fr: { t: 'Supprimer le préréglage « {name} » ?', reviewed: false }, 'zh-Hans': { t: '删除预设“{name}”？', reviewed: 'mt' } },
    'aria.presetPrev':        { en: { t: 'Previous preset' }, fr: { t: 'Préréglage précédent', reviewed: false }, 'zh-Hans': { t: '上一个预设', reviewed: 'mt' } },
    'aria.presetNext':        { en: { t: 'Next preset' }, fr: { t: 'Préréglage suivant', reviewed: false }, 'zh-Hans': { t: '下一个预设', reviewed: 'mt' } },
    // Factory display names (13 px in the select; the file names stay ASCII).
    // The other four factory presets reuse the label.lesson* captions.
    'preset.init':            { en: { t: 'Init · Additive Build' }, fr: { t: 'Init · construction additive', reviewed: false }, 'zh-Hans': { t: '初始 · 加法叠加', reviewed: 'mt' } },
    'preset.steppedScan':     { en: { t: 'Stepped Scan' }, fr: { t: 'Balayage par paliers', reviewed: false }, 'zh-Hans': { t: '阶梯扫描', reviewed: 'mt' } },
    'preset.smoothScan':      { en: { t: 'Smooth Scan' }, fr: { t: 'Balayage fondu', reviewed: false }, 'zh-Hans': { t: '平滑扫描', reviewed: 'mt' } },
    'preset.pulseNarrowing':  { en: { t: 'Pulse Narrowing' }, fr: { t: 'Impulsion qui rétrécit', reviewed: false }, 'zh-Hans': { t: '脉冲收窄', reviewed: 'mt' } },
    'preset.ppg4bit':         { en: { t: '4-bit PPG' }, fr: { t: 'PPG 4 bits', reviewed: false }, 'zh-Hans': { t: '4 bit PPG', reviewed: 'mt' } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
export const I18N_EXEMPT = [
    // The product name, split across the <h1>'s own text node and the italic
    // .title-accent span. The markup authors HAIR SPACES around the en dash;
    // the scanner collapses whitespace to U+0020 before it classifies.
    ['O – simple', 'first half of the product name O–simpleWavetable — a product name is never translated'],
    ['Wavetable',  'the italic half of the product name, in .title-accent — a product name is never translated', '.title-accent'],
    // Readouts (D-03): written by FORMAT / STEP_FORMAT, never a [data-i18n] node.
    ['−inf', 'output_level readout at the range floor — typographic minus, matching the page’s −6.0 dB; the host string stays ASCII -inf (N12)'],
    ['Full', 'bit_depth readout at index 0 — byte-identical to the AudioParameterChoice option, which is also the host automation name (D-01)'],
];

export const TIP_BINDINGS = [
    ['#gear-btn',            'settings'],
    ['#lang-select',         'lang-select'],
    ['#tips-toggle',         'tips-toggle'],

    ['#combo-bank',          'bank'],
    ['#importBtn',           'importBtn'],
    ['#sourceReadout',       'sourceReadout'],

    ['#bankWrap',            'bankWrap'],
    ['#cycleWrap',           'cycleWrap'],
    ['#harmWrap',            'harmWrap'],

    ['#knob-position',       'position'],
    ['#seg-interp',          'interp'],
    ['#seg-bandlimit',       'bandlimit'],
    ['#knob-bit_depth',      'bit_depth'],

    ['#seg-lfo_sync',        'lfo_sync'],
    ['#seg-lfo_shape',       'lfo_shape'],
    ['#knob-lfo_rate',       'lfo_rate'],
    ['#knob-lfo_div',        'lfo_div'],
    ['#knob-lfo_depth',      'lfo_depth'],

    ['#menvWrap',            'menvCanvas'],
    ['#knob-env_amount',     'env_amount'],
    ['#knob-menv_attack',    'menv_attack'],
    ['#knob-menv_decay',     'menv_decay'],
    ['#knob-menv_sustain',   'menv_sustain'],
    ['#knob-menv_release',   'menv_release'],

    ['#ampWrap',             'ampCanvas'],
    ['#knob-amp_attack',     'amp_attack'],
    ['#knob-amp_decay',      'amp_decay'],
    ['#knob-amp_sustain',    'amp_sustain'],
    ['#knob-amp_release',    'amp_release'],
    ['#seg-voice_mode',      'voice_mode'],
    ['#knob-output_level',   'output_level'],

    ['#lesson-steppedSmooth', 'lessonSteppedSmooth'],
    ['#lesson-aliasDemo',     'lessonAliasDemo'],
    ['#lesson-driveSweep',    'lessonDriveSweep'],
    ['#lesson-vowelPad',      'lessonVowelPad'],
    ['#lesson-ppg8bit',       'lessonPpg8bit'],

    ['#keyboard',            'keyboard'],
    ['#octDown',             'octDown'],
    ['#octUp',               'octUp'],

    ['#preset-select',       'presetSelect'],
    ['#preset-prev',         'presetPrev'],
    ['#preset-next',         'presetNext'],
    ['#preset-save',         'presetSave'],
    ['#preset-delete',       'presetDelete'],
];

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
