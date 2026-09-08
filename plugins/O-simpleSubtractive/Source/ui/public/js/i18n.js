/*
   This file is part of O-simpleSubtractive, an Ouaricon Audio plugin.
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
// i18n.js — O-simpleSubtractive interface copy (v1.5.0)
//
// ── v1.5.0: SIMPLIFIED CHINESE (Stage 4, wave 4g, 2026-09-07) ──────────────
// 133 rows added: a 'zh-Hans' arm on all 36 I18N entries (title + body) and on
// all 61 LABELS entries. Authored against scripts/i18n-zh-glossary.js and read
// back through a blind reverse pass by two further models; every row carries
// reviewed: 'bt'. NO NATIVE SPEAKER HAS READ IT — 'bt' is the disclosed
// quality level and scripts/i18n-zh-lint.js prints it on every run.
//
// The decisions the next reader needs:
//
//  · CHOICE FACES STAY ENGLISH in the Chinese bodies too, for the same reason
//    they stay English in French (D-01): they are the host automation
//    contract. Saw, Square, Triangle, Sine, Low-pass, High-pass, Band-pass,
//    Notch, Poly, Mono and Legato are named untranslated inside Chinese
//    sentences, with a space on each side of the Latin run (the corpus's
//    SPACED Latin/Han form).
//
//  · label.res AND label.resonance BOTH RENDER 共振, and that is deliberate.
//    English abbreviates one of them ("Res" beside the routing readout numeral,
//    "Resonance" on the knob); Chinese has no abbreviations, so the two
//    registers collapse onto one word. They name the SAME control property, so
//    a reader cannot be misled about which is which — the collision the R3
//    screen exists to catch is two DIFFERENT things reading alike.
//
//  · THE TWO DIAGRAM ROUTE CAPTIONS AND THEIR TOOLTIP TITLES COLLAPSE ON ONE
//    ARM AND NOT ON THE OTHER, and that asymmetry is a property of the terms
//    rather than of the authoring. English distinguishes the 8px SVG route
//    caption from the canvas's tooltip title by register — lowercase and
//    abbreviated against title-case and spelled out. Chinese has a genuine
//    short/long pair for the filter term and none for the amplitude one, so the
//    filter arm keeps two distinct renderings and the amp arm has only one
//    word to reach for. Both name the same signal route either way, so neither
//    reading can mislead about which control is meant.
//
//  · THE QWERTY RUN KEEPS ITS HAIR SPACES on the Chinese arm. The glossary root
//    spells the run with plain spaces, and the plain-space form is what three
//    sibling plugins ship; this page is the one that documents the hair spaces
//    as load-bearing, and a plain space here would widen the run on the zh arm
//    alone — a geometry change no en-vs-fr diff can see. normZh collapses runs
//    of whitespace, so the glossary root still matches either way.
//
//  · GEOMETRY. The zh arm is pinned, not reflowed: see the block of ratios and
//    floors in css/styles.css under the html[lang="zh-Hans"] selector. Chinese
//    is SHORTER than English on nearly every row here, so almost every pin is a
//    floor rather than a clip guard.
//
// ── v1.3.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 44 of 93 entries (16 terminology, 39 typography, 3 grammar; 0
// meaning — no French sentence was found saying something the English does
// not, and none had dropped a clause). The three sets overlap: 14 entries took
// both a term and a typographic fix. sameAsEn: kept 4, translated 0, ADDED 1
// (label.sub). termNote exemptions: 0 — every glossary term this page uses
// fits, so nothing needed exempting. Left as drafted: the other 49 entries.
// reviewed: false throughout — no native speaker yet.
//
// The decisions the next reader needs:
//
//  · RELÂCHEMENT FITS, measured on this page. The .knob-cell is 54 px and
//    .knob-label is shrink-to-fit with overflow: visible, so the caption is not
//    clipped by the cell: Relâche 48.02 px, Relâchement 77.33 px, both ONE line
//    (h 10.44 px), and the nearest same-row caption is still 218.25 px away.
//    The glossary abbreviation Relâch. (44.58 px) was not needed. Same verdict
//    as O-simpleFM's 56 px envelope cells, opposite verdict to O-Comp's 52 px
//    .control-group — the term is measured per page, never inherited.
//
//  · "Enveloppe du filtre" / "Enveloppe d'amplitude" are the glossary roots and
//    both fit the h2: 158.17 px and 183.06 px in a 255.6 px group title, one
//    line, the .group-route span still inline beside them. "Enveloppe d'ampli"
//    also disagreed with the tip title of the very canvas it captions
//    ("Enveloppe d'amplitude → niveau") — two French names for one control.
//
//  · "de filtre" vs "du filtre" IS a distinction, not an inconsistency. The
//    caption and every DEFINITE reference take the glossary root, "l'enveloppe
//    du filtre". An INDEFINITE mention keeps "de": "une enveloppe de filtre
//    rapide" is French, "une enveloppe du filtre rapide" is not. Seven bodies
//    read one way and five the other for that reason.
//
//  · Portamento replaces Glissando for Glide (glossary root), on the caption,
//    in the tip title and in the two lesson bodies that named it. The English
//    title glosses itself, "Glide (portamento)"; in French the gloss IS the
//    term, so the French title is the bare word. 69.42 px in a 60 px cell,
//    shrink-to-fit, 23.27 px of clearance to the next caption.
//
//  · "Sub" is the French too (glossary `sub`) and carries sameAsEn: true. The
//    draft's "Sous" is a preposition standing alone as a noun caption.
//
//  · Three fixes that are neither term nor typography: "Abaissez-la" ->
//    "Abaissez-le" (the antecedent inside the French sentence is "le coude",
//    and the English "Lower it" means the corner); "Legato joue une seule note
//    mais lie" -> "mais liée" (lier is transitive and had no object); and
//    "cliquez les touches" -> "cliquez sur les touches", the calque, in the
//    form the rest of the O-simple family ships.
//
//  · Choice faces stay English in both languages (I18N_EXEMPT, D-01), so the
//    French bodies name them capitalised and untranslated — Saw, Square, Sine,
//    Low-pass, Poly, Mono, Legato — while the same words used generically are
//    prose and are translated ("une dent de scie brillante").
//
//  · GEOMETRY, both gates green and unmoved: 0 non-label elements moved between
//    languages, and the .frame pane's scroll extent held at 1118 px in both
//    languages before and after (html{overflow:hidden} clamps
//    documentElement.scrollHeight at the 820 px viewport, so 1118 is the number
//    that can move). 35 of 35 tooltip boxes measured identical in size before
//    and after the pass — the no-break spaces cost no line.
//
// UI ROOT IS Source/ui/public. There is no second UI root in this plugin and no
// Resources/ui staging directory. This file is the sixth SOURCES entry in
// juce_add_binary_data(O-simpleSubtractive_UIResources).
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
// COPY IS textContent ON EVERY PATH — never innerHTML. v1.2.5's tooltip
// renderer built its tip with `tip.innerHTML = ...`; v1.3.0 builds it with
// createElement + textContent, because the tip text is now table-sourced and
// localized rather than a fixed literal. check-i18n assertion 9 rejects any
// innerHTML reference here and any string literal containing an opening angle
// bracket.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every `en` entry below was extracted
// mechanically from v1.2.5's `TIPS` / `LESSONS` tables in js/app.js and from
// index.html, and compared back to the source with entities decoded, rather
// than re-typed. HTML entities are decoded to the characters they named
// (&#183; -> ·, &#8594; -> →, &#8202; -> a \u200a escape, NOT a plain space)
// because setAttribute and textContent do not decode entities.
//
// ONE DELIBERATE ENGLISH CHANGE, recorded in the CHANGELOG: the tip bodies have
// lost their strong/em emphasis tags. The WORDS are unchanged. Assertion 9
// forbids an angle bracket in a string literal here, and it is right to: the
// renderer now writes textContent, so a tag would render as literal characters
// rather than as emphasis.
//
// KEYS ARE THE PARAMETER ID where the anchor is a parameter cell, and a
// `label.*` / `aria.*` slug otherwise. The parameter cells are addressed by the
// data-param attribute added in v1.3.0; through v1.2.5 they carried the tip KEY
// in their own data-tip attribute, and a `.knob-cell` selector would have
// matched the FIRST of sixteen — the failure canon §1 names and the one
// O-Octagon's .vunit-group tip hit for real in Stage C.
//
// LABELS NEVER REUSE A TOOLTIP KEY HERE. trLabel() falls back to I18N and four
// of this page's captions do happen to equal their tip title today ("Noise",
// "Resonance", "Signal Path", "Output Waveform") — but twenty others do not
// ("Cutoff" vs "Cutoff Frequency", "Level" vs "Output Level", "Mode" vs "Voice
// Mode"), and a rule that holds for four of twenty-four is a rule nobody can
// apply. A caption and a tip title also diverge the moment either is edited,
// which would make a tooltip copy edit a silent geometry change to a control.
// The ONE place the fallback is used on purpose is `data-i18n-aria` on the
// knobs, combos and canvases: an accessible name IS the control's name, so it
// reads the tooltip title by design.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

export const I18N = Object.freeze({

    // ── The settings popover (v1.3.0) ───────────────────────────────────────
    // The gear holds the language selector and, since v1.4.0, the hover-help
    // switch. Through v1.3.1 this plugin's help layer was always on and the
    // panel held the selector alone; the switch is the one O-simpleGrain
    // carries, and its copy is that plugin's, verbatim.
    'gear-btn': {
        en: { t: 'Settings',
              b: 'Choose the language of the interface and switch this hover help off or on. The language is remembered with the session; the help switch is remembered on this computer.' },
        fr: { t: 'Réglages',
              b: "Choisir la langue de l’interface et activer ou désactiver ces infobulles. La langue est conservée avec la session ; le réglage des infobulles est conservé sur cet ordinateur.",
              reviewed: true },
        'zh-Hans': { t: "设置",
                     b: "选择界面语言，并开启或关闭这些悬停帮助。语言随会话保存；帮助开关保存在这台电脑上。",
                     reviewed: 'mt' },
    },

    // Written to say what is TRUE of canon v2, in every language the page
    // offers: the labels DO change, and the halves that stay English are named
    // rather than left to be discovered — value readouts (D-03) and the four
    // drop-down menus, whose entries come from the C++ AudioParameterChoice and
    // are the host automation contract (D-01).
    //
    // The body no longer ENUMERATES the languages on offer. Through v1.4.1 it
    // named a fixed set, which is a claim the selector itself falsifies the
    // moment a language is added — as v1.5.0 did. The selector IS the list; the
    // body says only what does not follow it. The exception list stays, and its
    // count was re-verified against the markup: five select elements minus the
    // language selector itself is four.
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of the labels on this page and of this hover help. Value readouts and the four drop-down menus stay in English.' },
        fr: { t: 'Langue',
              b: "La langue des libellés de cette page et de ces infobulles. Les valeurs affichées et les quatre menus déroulants restent en anglais.",
              reviewed: true },
        'zh-Hans': { t: "语言",
                     b: "本页面标签与这些悬停帮助的语言。数值读数和四个下拉菜单保持英文。",
                     reviewed: 'mt' },
    },
    'help-toggle': {
        en: { t: 'Hover help',
              b: 'Turns these hover explanations off or back on. The switch is remembered on this computer rather than in the session, so it follows you from one project to the next.' },
        fr: { t: "Infobulles",
              b: "Active ou désactive ces infobulles. Le réglage est conservé sur cet ordinateur et non dans la session : il vous suit d’un projet à l’autre.",
              reviewed: true },
        'zh-Hans': { t: "悬停帮助",
                     b: "开启或关闭这些悬停说明。该开关保存在这台电脑上而不是会话里，因此它会随你从一个项目带到下一个项目。",
                     reviewed: 'mt' },
    },

    // ── Oscillator group ────────────────────────────────────────────────────
    oscWave: {
        en: { t: 'Oscillator Wave',
              b: 'The raw tone the filter carves from. Saw is the brightest (all harmonics), Square is hollow (odd harmonics), Triangle is soft, Sine is pure — nothing for the filter to remove.' },
        fr: { t: "Onde de l’oscillateur",
              b: "Le timbre brut dans lequel le filtre taille. Saw est la plus brillante (tous les harmoniques), Square est creuse (harmoniques impairs), Triangle est douce, Sine est pure — le filtre n’a rien à retirer.",
              reviewed: true },
        'zh-Hans': { t: "振荡器波形",
                     b: "滤波器由此雕刻的原始音色。Saw 最明亮（全部谐波），Square 空心（奇次谐波），Triangle 柔和，Sine 纯净 —— 滤波器无物可减。",
                     reviewed: 'mt' },
    },
    subLevel: {
        en: { t: 'Sub Oscillator',
              b: 'Mixes in a square wave one octave below the note. Adds body and weight underneath the main oscillator — useful for basses.' },
        fr: { t: 'Sous-oscillateur',
              b: "Ajoute au mélange une onde carrée une octave sous la note. Apporte du corps et du poids sous l’oscillateur principal — utile pour les basses.",
              reviewed: true },
        'zh-Hans': { t: "副振荡器",
                     b: "混入一个比音符低八度的方波。在主振荡器下方增添厚度与重量 —— 对低音很有用。",
                     reviewed: 'mt' },
    },
    noiseLevel: {
        en: { t: 'Noise',
              b: "Mixes in white noise — every frequency at once. Feed it through the filter to hear the filter's shape on its own, or add breath/air to a tone." },
        fr: { t: 'Bruit',
              b: "Ajoute au mélange du bruit blanc — toutes les fréquences à la fois. Passez-le dans le filtre pour entendre la forme du filtre seule, ou pour ajouter du souffle et de l’air à un timbre.",
              reviewed: true },
        'zh-Hans': { t: "噪声",
                     b: "混入白噪声 —— 所有频率同时出现。让它通过滤波器，可以单独听见滤波器的形状，也可以为音色添加气息与空气感。",
                     reviewed: 'mt' },
    },

    // ── Filter group ────────────────────────────────────────────────────────
    filterType: {
        en: { t: 'Filter Type',
              b: 'Which side of the cutoff is kept. Low-pass keeps lows (the classic subtractive sound), High-pass keeps highs, Band-pass keeps a band around cutoff, Notch removes a band.' },
        fr: { t: 'Type de filtre',
              b: "Quel côté de la coupure est conservé. Low-pass garde le grave (le son soustractif classique), High-pass garde l’aigu, Band-pass garde une bande autour de la coupure, Notch retire une bande.",
              reviewed: true },
        'zh-Hans': { t: "滤波器类型",
                     b: "截止点的哪一侧被保留。Low-pass 保留低频（经典的减法音色），High-pass 保留高频，Band-pass 保留截止点周围的一条频带，Notch 则去掉一条频带。",
                     reviewed: 'mt' },
    },
    filterSlope: {
        en: { t: 'Filter Slope (poles)',
              b: 'How sharply the filter cuts past the cutoff. 6 dB/oct = 1 pole, gentle. 24 dB/oct = 4 poles, steep and aggressive. Steeper = more of the spectrum removed just past the knee.' },
        fr: { t: 'Pente du filtre (pôles)',
              b: 'Avec quelle netteté le filtre coupe au-delà de la coupure. 6 dB/oct = 1 pôle, doux. 24 dB/oct = 4 pôles, raide et agressif. Plus raide = plus de spectre retiré juste après le coude.',
              reviewed: true },
        'zh-Hans': { t: "滤波器斜率（极点）",
                     b: "滤波器在截止点之外切得有多陡。6 dB/oct 是 1 个极点，平缓。24 dB/oct 是 4 个极点，陡峭而强烈。越陡，拐点之后被去掉的频谱就越多。",
                     reviewed: 'mt' },
    },
    cutoff: {
        en: { t: 'Cutoff Frequency',
              b: "The corner where the filter starts working. Lower it and watch the spectrum bars above the curve fall away — that's harmonics being removed, the heart of subtractive synthesis." },
        fr: { t: 'Fréquence de coupure',
              b: 'Le coude où le filtre commence à agir. Abaissez-le et regardez les barres du spectre au-dessus de la courbe disparaître — ce sont des harmoniques qui sont retirés, le cœur de la synthèse soustractive.',
              reviewed: true },
        'zh-Hans': { t: "截止频率",
                     b: "滤波器开始起作用的拐点。把它调低，看着曲线上方的频谱条纷纷落下 —— 那就是正在被去掉的谐波，减法合成的核心。",
                     reviewed: 'mt' },
    },
    resonance: {
        en: { t: 'Resonance',
              b: "Boosts a peak right at the cutoff. A little adds vocal emphasis; push it far and the filter rings, then self-oscillates into a pure sine whistle at the cutoff — the curve's peak grows into a spike." },
        fr: { t: 'Résonance',
              b: 'Accentue un pic juste à la coupure. Un peu apporte une emphase vocale ; poussez loin et le filtre sonne, puis auto-oscille en un sifflement sinusoïdal pur à la coupure — le pic de la courbe devient une pointe.',
              reviewed: true },
        'zh-Hans': { t: "共振",
                     b: "在截止点处提升出一个峰。少量会带来人声般的强调；推得远了滤波器就会鸣响，接着在截止点自激成一声纯正弦的哨音 —— 曲线上的峰会长成一根尖刺。",
                     reviewed: 'mt' },
    },
    filterEnvAmount: {
        en: { t: 'Filter Env Amount',
              b: 'How far the filter envelope sweeps the cutoff, and in which direction. Positive opens the filter on each note (bright attack); negative closes it. Bipolar: zero means the envelope does nothing.' },
        fr: { t: "Taux de l’enveloppe du filtre",
              b: "De combien l’enveloppe du filtre balaie la coupure, et dans quel sens. Positif ouvre le filtre à chaque note (attaque brillante) ; négatif le ferme. Bipolaire : à zéro l’enveloppe ne fait rien.",
              reviewed: true },
        'zh-Hans': { t: "滤波器包络量",
                     b: "滤波器包络把截止点扫过多远，以及朝哪个方向。正值在每个音符上打开滤波器（明亮的起音）；负值则关上它。双极性：为零时包络不起任何作用。",
                     reviewed: 'mt' },
    },
    keyTrack: {
        en: { t: 'Key Tracking',
              b: 'Makes the cutoff follow the note pitch — higher notes open the filter more. At 100% the filter tracks the keyboard so timbre stays consistent across the range.' },
        fr: { t: 'Suivi de clavier',
              b: 'Fait suivre la hauteur de la note par la coupure — les notes aiguës ouvrent davantage le filtre. À 100 % le filtre suit le clavier, si bien que le timbre reste constant sur toute la tessiture.',
              reviewed: true },
        'zh-Hans': { t: "键位跟踪",
                     b: "让截止点跟随音符的音高 —— 越高的音符把滤波器打得越开。在 100% 时滤波器完全跟随键盘，音色在整个音域上保持一致。",
                     reviewed: 'mt' },
    },

    // ── Filter envelope ─────────────────────────────────────────────────────
    filterAttack: {
        en: { t: 'Filter Attack',
              b: 'Time for the filter envelope to rise after note-on — how fast the filter sweep opens.' },
        fr: { t: 'Attaque du filtre',
              b: "Temps que met l’enveloppe du filtre à monter après le début de note — la vitesse à laquelle le balayage du filtre s’ouvre.",
              reviewed: true },
        'zh-Hans': { t: "滤波起音",
                     b: "音符触发后滤波器包络上升所需的时间 —— 滤波扫描打开得有多快。",
                     reviewed: 'mt' },
    },
    filterDecay: {
        en: { t: 'Filter Decay',
              b: 'Time for the filter envelope to fall from its peak to the sustain level — shapes the bright-to-dark motion of a pluck.' },
        fr: { t: 'Déclin du filtre',
              b: "Temps que met l’enveloppe du filtre à retomber de son sommet au niveau de maintien — façonne le passage du brillant au sombre d’un pincement.",
              reviewed: true },
        'zh-Hans': { t: "滤波衰减",
                     b: "滤波器包络从峰值回落到延音电平所需的时间 —— 塑造拨奏由亮转暗的走向。",
                     reviewed: 'mt' },
    },
    filterSustain: {
        en: { t: 'Filter Sustain',
              b: 'The cutoff-sweep level held while the key stays down.' },
        fr: { t: 'Maintien du filtre',
              b: 'Le niveau de balayage de coupure tenu tant que la touche reste enfoncée.',
              reviewed: true },
        'zh-Hans': { t: "滤波延音",
                     b: "按键保持按下时维持的截止扫描电平。",
                     reviewed: 'mt' },
    },
    filterRelease: {
        en: { t: 'Filter Release',
              b: 'Time for the filter sweep to fall back after the key is released.' },
        fr: { t: 'Relâchement du filtre',
              b: 'Temps que met le balayage du filtre à redescendre après le relâchement de la touche.',
              reviewed: true },
        'zh-Hans': { t: "滤波释音",
                     b: "松开按键后滤波扫描回落所需的时间。",
                     reviewed: 'mt' },
    },

    // ── Amp envelope ────────────────────────────────────────────────────────
    ampAttack: {
        en: { t: 'Amp Attack',
              b: 'Time for loudness to rise after note-on. Short = a percussive start; long = a slow swell.' },
        fr: { t: "Attaque d’amplitude",
              b: 'Temps que met le volume à monter après le début de note. Court = un départ percussif ; long = une montée lente.',
              reviewed: true },
        'zh-Hans': { t: "振幅起音",
                     b: "音符触发后响度上升所需的时间。短是打击性的起头；长是缓慢的涨起。",
                     reviewed: 'mt' },
    },
    ampDecay: {
        en: { t: 'Amp Decay',
              b: 'Time for loudness to fall from its peak to the sustain level.' },
        fr: { t: "Déclin d’amplitude",
              b: 'Temps que met le volume à retomber de son sommet au niveau de maintien.',
              reviewed: true },
        'zh-Hans': { t: "振幅衰减",
                     b: "响度从峰值回落到延音电平所需的时间。",
                     reviewed: 'mt' },
    },
    ampSustain: {
        en: { t: 'Amp Sustain',
              b: 'Loudness held while the key stays down.' },
        fr: { t: "Maintien d’amplitude",
              b: 'Volume tenu tant que la touche reste enfoncée.',
              reviewed: true },
        'zh-Hans': { t: "振幅延音",
                     b: "按键保持按下时维持的响度。",
                     reviewed: 'mt' },
    },
    ampRelease: {
        en: { t: 'Amp Release',
              b: 'Time for loudness to fade after the key is released — also sets how long the voice rings out.' },
        fr: { t: "Relâchement d’amplitude",
              b: 'Temps que met le volume à disparaître après le relâchement de la touche — fixe aussi la durée pendant laquelle la voix continue de sonner.',
              reviewed: true },
        'zh-Hans': { t: "振幅释音",
                     b: "松开按键后响度消退所需的时间 —— 也决定了这个声部延续鸣响的长度。",
                     reviewed: 'mt' },
    },

    // ── Voice / Output ──────────────────────────────────────────────────────
    voiceMode: {
        en: { t: 'Voice Mode',
              b: 'Poly plays chords. Mono plays one note, retriggering the envelopes each time. Legato plays one note but slurs — overlapping notes glide without retriggering.' },
        fr: { t: 'Mode de voix',
              b: 'Poly joue des accords. Mono joue une seule note, en redéclenchant les enveloppes à chaque fois. Legato joue une seule note mais liée — les notes qui se chevauchent glissent sans redéclenchement.',
              reviewed: true },
        'zh-Hans': { t: "发声模式",
                     b: "Poly 演奏和弦。Mono 只发一个音，每次都重新触发包络。Legato 也只发一个音，但会连起来 —— 重叠的音符之间滑行，不重新触发。",
                     reviewed: 'mt' },
    },
    glide: {
        en: { t: 'Glide (portamento)',
              b: 'Time to slide pitch from one note to the next. Most audible in Mono/Legato — zero is an instant jump.' },
        fr: { t: 'Portamento',
              b: "Temps de glissement de la hauteur d’une note à la suivante. Surtout audible en Mono/Legato — à zéro le saut est instantané.",
              reviewed: true },
        'zh-Hans': { t: "滑音（portamento）",
                     b: "音高从一个音符滑到下一个音符所需的时间。在 Mono/Legato 下最明显 —— 为零时是瞬间的跳跃。",
                     reviewed: 'mt' },
    },
    outputLevel: {
        en: { t: 'Output Level',
              b: 'Master output trim in decibels. -60 dB is silence.' },
        fr: { t: 'Niveau de sortie',
              b: 'Réglage de la sortie générale en décibels. −60 dB, c’est le silence.',
              reviewed: true },
        'zh-Hans': { t: "输出电平",
                     b: "以分贝为单位的总输出微调。-60 dB 即静音。",
                     reviewed: 'mt' },
    },

    // ── The three displays and the diagram ──────────────────────────────────
    headline: {
        en: { t: 'Filter Response over Spectrum',
              b: "The amber line is the filter's frequency response — the shape it imposes. The bars are the live output spectrum. Harmonics sitting above the curve's knee get pushed down: you are watching the filter remove sound." },
        fr: { t: 'Réponse du filtre sur le spectre',
              b: "La ligne ambrée est la réponse en fréquence du filtre — la forme qu’il impose. Les barres sont le spectre de sortie en direct. Les harmoniques situés au-dessus du coude de la courbe sont abaissés : vous regardez le filtre retirer du son.",
              reviewed: true },
        'zh-Hans': { t: "频谱上的滤波器响应",
                     b: "琥珀色的线是滤波器的频率响应 —— 它所强加的形状。柱条是实时的输出频谱。位于曲线拐点之上的谐波会被压下去：你正看着滤波器在减去声音。",
                     reviewed: 'mt' },
    },
    scope: {
        en: { t: 'Output Waveform',
              b: 'The post-filter signal in the time domain. Watch a bright saw round off into a smooth shape as you lower the cutoff, or ring as resonance climbs.' },
        fr: { t: 'Forme d’onde de sortie',
              b: 'Le signal après filtrage, dans le domaine temporel. Regardez une dent de scie brillante s’arrondir en une forme lisse quand vous abaissez la coupure, ou se mettre à sonner quand la résonance monte.',
              reviewed: true },
        'zh-Hans': { t: "输出波形",
                     b: "滤波之后的信号在时域中的样子。调低截止点，看着明亮的锯齿波被磨圆成平滑的形状；共振升高时，看着它开始鸣响。",
                     reviewed: 'mt' },
    },
    filterAdsr: {
        en: { t: 'Filter Envelope → cutoff',
              b: "The shape that sweeps the cutoff over time (Attack-Decay-Sustain-Release). The dashed marker shows the envelope's live output as you play — this scale drives brightness, not loudness." },
        fr: { t: 'Enveloppe du filtre → coupure',
              b: "La forme qui balaie la coupure dans le temps (attaque, déclin, maintien, relâchement). Le repère pointillé montre la sortie de l’enveloppe en direct pendant que vous jouez — cette échelle pilote la brillance, pas le volume.",
              reviewed: true },
        'zh-Hans': { t: "滤波器包络 → 截止",
                     b: "随时间扫过截止点的形状（起音、衰减、延音、释音）。虚线标记显示你演奏时包络的实时输出 —— 这条标尺驱动的是明亮度，不是响度。",
                     reviewed: 'mt' },
    },
    ampAdsr: {
        en: { t: 'Amp Envelope → level',
              b: 'The shape that controls loudness over time. The dashed marker shows its live output — an independent scale from the filter envelope, so brightness and volume can move separately.' },
        fr: { t: 'Enveloppe d’amplitude → niveau',
              b: "La forme qui contrôle le volume dans le temps. Le repère pointillé montre sa sortie en direct — une échelle indépendante de celle de l’enveloppe du filtre, si bien que brillance et volume peuvent bouger séparément.",
              reviewed: true },
        'zh-Hans': { t: "振幅包络 → 电平",
                     b: "随时间控制响度的形状。虚线标记显示它的实时输出 —— 与滤波器包络各自独立的一条标尺，因此明亮度和音量可以分开变化。",
                     reviewed: 'mt' },
    },
    routing: {
        en: { t: 'Signal Path',
              b: 'Oscillator → Filter → Amplifier. The filter envelope routes up into the filter (sweeping cutoff); the amp envelope routes up into the VCA (shaping loudness). Two envelopes, two destinations.' },
        fr: { t: 'Chaîne du signal',
              b: "Oscillateur → filtre → amplificateur. L’enveloppe du filtre remonte dans le filtre (elle balaie la coupure) ; l’enveloppe d’amplitude remonte dans le VCA (elle façonne le volume). Deux enveloppes, deux destinations.",
              reviewed: true },
        'zh-Hans': { t: "信号路径",
                     b: "振荡器 → 滤波器 → 放大器。滤波器包络向上接入滤波器（扫动截止点）；振幅包络向上接入 VCA（塑造响度）。两条包络，两个目的地。",
                     reviewed: 'mt' },
    },

    // ── The eight lesson presets ────────────────────────────────────────────
    lessonSawSweep: {
        en: { t: "Saw → LP Sweep · how it's built",
              b: 'The headline move: a bright saw through a 24 dB low-pass with a slow filter envelope. Watch the upper harmonics fall away under the curve as the cutoff opens and closes — the literal subtraction the method is named for.' },
        fr: { t: 'Dent de scie → balayage passe-bas · comment c’est fait',
              b: 'Le geste emblématique : une dent de scie brillante à travers un passe-bas 24 dB avec une enveloppe de filtre lente. Regardez les harmoniques aigus disparaître sous la courbe pendant que la coupure s’ouvre et se referme — la soustraction littérale qui donne son nom à la méthode.',
              reviewed: true },
        'zh-Hans': { t: "锯齿波 → 低通扫描 · 如何做成",
                     b: "招牌动作：一段明亮的锯齿波，经过一个 24 dB 低通，配上一条缓慢的滤波器包络。截止点开合时，看着高次谐波在曲线之下纷纷落去 —— 这就是这套方法得名的那种字面意义上的减法。",
                     reviewed: 'mt' },
    },
    lessonPluck: {
        en: { t: "Pluck · how it's built",
              b: 'A fast filter envelope (short decay, low sustain) snaps the cutoff bright-then-dark, while a quick amp decay makes a percussive note. Filter env does the timbral work.' },
        fr: { t: 'Pincement · comment c’est fait',
              b: "Une enveloppe de filtre rapide (déclin court, maintien bas) fait claquer la coupure du brillant vers le sombre, pendant qu’un déclin d’amplitude rapide donne une note percussive. C’est l’enveloppe du filtre qui fait le travail de timbre.",
              reviewed: true },
        'zh-Hans': { t: "拨奏 · 如何做成",
                     b: "一条快速的滤波器包络（衰减短、延音低）让截止点先亮后暗地一弹，同时快速的振幅衰减做出一个打击性的音。音色上的活儿是滤波器包络干的。",
                     reviewed: 'mt' },
    },
    lessonSweep: {
        en: { t: "Sweep Pad · how it's built",
              b: 'Slow amp attack swells the level in; a long, deep filter envelope opens the cutoff gradually — you hear the spectrum brighten over seconds. Pads are about slow envelopes.' },
        fr: { t: 'Nappe balayée · comment c’est fait',
              b: "Une attaque d’amplitude lente fait monter le niveau ; une enveloppe de filtre longue et profonde ouvre la coupure peu à peu — on entend le spectre s’éclaircir sur plusieurs secondes. Les nappes sont affaire d’enveloppes lentes.",
              reviewed: true },
        'zh-Hans': { t: "扫描音垫 · 如何做成",
                     b: "缓慢的振幅起音把电平涨进来；一条又长又深的滤波器包络把截止点逐渐打开 —— 你会听见频谱在几秒之间变亮。音垫讲的就是慢包络。",
                     reviewed: 'mt' },
    },
    lessonAcid: {
        en: { t: "Acid Bass · how it's built",
              b: 'High resonance + a snappy filter envelope on a saw through a 24 dB low-pass — the squelchy, ringing peak that defines the acid sound. Mono with a touch of glide.' },
        fr: { t: 'Basse acid · comment c’est fait',
              b: 'Résonance élevée et enveloppe de filtre nerveuse sur une dent de scie à travers un passe-bas 24 dB — le pic sonnant et gluant qui définit le son acid. En mono, avec un soupçon de portamento.',
              reviewed: true },
        'zh-Hans': { t: "酸性贝斯 · 如何做成",
                     b: "高共振加上一条利落的滤波器包络，作用在经过 24 dB 低通的锯齿波上 —— 那个黏滑而鸣响的峰，正是酸性音色的定义。单声道，再加一点滑音。",
                     reviewed: 'mt' },
    },
    lessonSelfOsc: {
        en: { t: "Self-Oscillation · how it's built",
              b: 'Resonance pushed to the limit with the oscillators down: the filter rings on its own into a pure sine at the cutoff. The filter becomes the sound source.' },
        fr: { t: 'Auto-oscillation · comment c’est fait',
              b: 'Résonance poussée à la limite, oscillateurs baissés : le filtre sonne tout seul en une sinusoïde pure à la coupure. Le filtre devient la source sonore.',
              reviewed: true },
        'zh-Hans': { t: "自激振荡 · 如何做成",
                     b: "共振推到极限，振荡器压下去：滤波器自己在截止点鸣响成一条纯正弦。滤波器变成了声源。",
                     reviewed: 'mt' },
    },
    lessonBrass: {
        en: { t: "Brass Stab · how it's built",
              b: 'Positive filter-env amount so the cutoff opens with the attack and holds — brightness tracks the note like a blown brass instrument. A short, firm amp envelope gives the stab.' },
        fr: { t: 'Coup de cuivres · comment c’est fait',
              b: "Taux d’enveloppe de filtre positif, si bien que la coupure s’ouvre avec l’attaque et s’y tient — la brillance suit la note comme un cuivre soufflé. Une enveloppe d’amplitude courte et ferme donne le coup.",
              reviewed: true },
        'zh-Hans': { t: "铜管短奏 · 如何做成",
                     b: "正的滤波器包络量，让截止点随起音打开并保持住 —— 明亮度像被吹响的铜管乐器一样跟着音符走。一条短而结实的振幅包络给出那记短奏。",
                     reviewed: 'mt' },
    },
    lessonSquareBass: {
        en: { t: "Square Bass · how it's built",
              b: 'A hollow square wave (odd harmonics only) plus the sub-oscillator an octave down for weight, through a 24 dB low-pass. Mono, so it plays as one solid bass voice — a polysynth is just several of these in parallel.' },
        fr: { t: 'Basse carrée · comment c’est fait',
              b: 'Une onde carrée creuse (harmoniques impairs seulement) plus le sous-oscillateur une octave en dessous pour le poids, à travers un passe-bas 24 dB. En mono, pour une seule voix de basse bien pleine — un polysynthé, ce sont simplement plusieurs de ces voix en parallèle.',
              reviewed: true },
        'zh-Hans': { t: "方波贝斯 · 如何做成",
                     b: "一段空心的方波（只有奇次谐波），加上低八度的副振荡器来添重量，一起经过 24 dB 低通。单声道，于是它作为一个结实的低音声部演奏 —— 所谓复音合成器，不过就是若干个这样的声部并联。",
                     reviewed: 'mt' },
    },
    lessonNoiseWind: {
        en: { t: "Filtered Noise · how it's built",
              b: 'Push the noise source up and band-pass it: with no harmonic source the filter sculpts pitchless air into wind. Slow envelopes swell it in and out — how subtractive synthesis makes breath and percussion, not just notes.' },
        fr: { t: 'Bruit filtré · comment c’est fait',
              b: "Montez la source de bruit et passez-la en passe-bande : sans source harmonique, le filtre sculpte de l’air sans hauteur en vent. Des enveloppes lentes le font entrer et sortir — comment la synthèse soustractive fabrique du souffle et de la percussion, et pas seulement des notes.",
              reviewed: true },
        'zh-Hans': { t: "滤波噪声 · 如何做成",
                     b: "把噪声源推上去，再给它一个带通：没有谐波源，滤波器就把没有音高的空气雕成风。缓慢的包络让它涨起又退去 —— 这就是减法合成除了音符之外，做出气息与打击声的办法。",
                     reviewed: 'mt' },
    },
});

// ============================================================================
// LABELS — the page's own captions, v1.3.0
//
// Separate from I18N because a tooltip entry is a {title, body} PAIR and a
// label is one string.
//
// THE REUSE RULE, as applied on this page: NO caption reuses a tooltip key.
// The header comment above records why — four of twenty-four captions match
// their tip title and twenty do not, so reuse would be a rule nobody could
// apply, and a tooltip copy edit would become a silent geometry change to a
// control.
//
// The `aria.*` keys are the exception the fallback exists for: they are read by
// data-i18n-aria on the knobs, combos and canvases, where the accessible name
// IS the control's name and reading the tooltip title is the point.
//
// `sameAsEn: true` marks the three captions whose faithful French IS the
// English word. It is a declaration, not a skip: assertion 4 rejects a silent
// passthrough and accepts a declared one.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // The product name itself is NOT here — see I18N_EXEMPT. Only the strapline
    // under it is copy.
    'label.subtitle': {
        en: { t: 'Subtractive Synthesizer · Osc → Filter → Amp · A Field Guide' },
        fr: { t: 'Synthétiseur soustractif · osc → filtre → ampli · un guide de terrain',
              reviewed: true },
        'zh-Hans': { t: "减法合成器 · 振荡 → 滤波 → 放大 · 实地指南", reviewed: 'mt' },
    },

    // ── The two displays ────────────────────────────────────────────────────
    // "Filter Response over Spectrum ·" keeps its trailing separator: the
    // fleuron belongs to the caption, not to the hint span beside it, and
    // moving it would change where the two boxes meet.
    'label.headline': {
        en: { t: 'Filter Response over Spectrum ·' },
        fr: { t: 'Réponse du filtre sur le spectre ·', reviewed: true },
        'zh-Hans': { t: "频谱上的滤波器响应 ·", reviewed: 'mt' },
    },
    'label.headlineHint': {
        en: { t: 'the curve is the filter; the bars are what it lets through' },
        fr: { t: 'la courbe est le filtre ; les barres sont ce qu’il laisse passer',
              reviewed: true },
        'zh-Hans': { t: "曲线是滤波器；柱条是它放过去的部分", reviewed: 'mt' },
    },
    'label.scope': {
        en: { t: 'Output Waveform ·' },
        fr: { t: 'Forme d’onde de sortie ·', reviewed: true },
        'zh-Hans': { t: "输出波形 ·", reviewed: 'mt' },
    },
    'label.scopeHint': {
        en: { t: 'morphs with cutoff / res / envelope' },
        fr: { t: 'change avec coupure / rés. / enveloppe', reviewed: true },
        'zh-Hans': { t: "随截止、共振与包络而变形", reviewed: 'mt' },
    },

    // ── Signal-path diagram ─────────────────────────────────────────────────
    // The three node captions and the two route captions are SVG <text> nodes.
    // applyLabel writes textContent, which an SVG text node has; they are
    // text-anchor="middle" at a fixed x, so translating them moves nothing.
    'label.signalPath': {
        en: { t: 'Signal Path' },
        fr: { t: 'Chaîne du signal', reviewed: true },
        'zh-Hans': { t: "信号路径", reviewed: 'mt' },
    },
    'label.routeFilterEnv': {
        en: { t: 'filter env → cutoff' },
        fr: { t: 'env. filtre → coupure', reviewed: true },
        'zh-Hans': { t: "滤波包络 → 截止", reviewed: 'mt' },
    },
    'label.routeAmpEnv': {
        en: { t: 'amp env → level' },
        fr: { t: 'env. ampli → niveau', reviewed: true },
        'zh-Hans': { t: "振幅包络 → 电平", reviewed: 'mt' },
    },
    'label.nodeOsc': {
        en: { t: 'OSC' },
        fr: { t: 'OSC', sameAsEn: true, reviewed: true },
        'zh-Hans': { t: "振荡", reviewed: 'mt' },
    },
    'label.nodeFilter': {
        en: { t: 'FILTER' },
        fr: { t: 'FILTRE', reviewed: true },
        'zh-Hans': { t: "滤波", reviewed: 'mt' },
    },
    // VCA is the standard French term too — voltage-controlled amplifier keeps
    // its English initialism in French synth vocabulary.
    'label.nodeVca': {
        en: { t: 'VCA' },
        fr: { t: 'VCA', sameAsEn: true, reviewed: true },
        'zh-Hans': { t: "VCA", reviewed: 'mt' },
    },
    // The abbreviation beside the resonance numeral. Its own span, so the
    // fleuron and the hair space around it stay literal text nodes: applyLabel
    // writes textContent, which on the parent would delete both readout spans.
    'label.res': {
        en: { t: 'Res' },
        fr: { t: 'Rés', reviewed: true },
        'zh-Hans': { t: "共振", reviewed: 'mt' },
    },

    // ── Oscillator group ────────────────────────────────────────────────────
    'label.groupOsc': {
        en: { t: 'Oscillator' },
        fr: { t: 'Oscillateur', reviewed: true },
        'zh-Hans': { t: "振荡器", reviewed: 'mt' },
    },
    'label.wave': { en: { t: 'Wave' },  fr: { t: 'Onde',  reviewed: true },
        'zh-Hans': { t: "波形", reviewed: 'mt' }, },
    // "Sub" is the settled French too (glossary `sub`). "Sous" alone is a
    // preposition, not a noun: the caption for the sub-oscillator read as the
    // bare word "under". sameAsEn is the declaration that a reviewer looked
    // and agreed the English word IS the French one, not a skip.
    'label.sub':   { en: { t: 'Sub' },   fr: { t: 'Sub', sameAsEn: true, reviewed: true },
        'zh-Hans': { t: "低音", reviewed: 'mt' }, },
    'label.noise': { en: { t: 'Noise' }, fr: { t: 'Bruit', reviewed: true },
        'zh-Hans': { t: "噪声", reviewed: 'mt' }, },

    // ── Filter group ────────────────────────────────────────────────────────
    'label.groupFilter': { en: { t: 'Filter' }, fr: { t: 'Filtre', reviewed: true },
        'zh-Hans': { t: "滤波器", reviewed: 'mt' }, },
    'label.type':      { en: { t: 'Type' },      fr: { t: 'Type', sameAsEn: true, reviewed: true },
        'zh-Hans': { t: "类型", reviewed: 'mt' }, },
    'label.slope':     { en: { t: 'Slope' },     fr: { t: 'Pente', reviewed: true },
        'zh-Hans': { t: "斜率", reviewed: 'mt' }, },
    'label.cutoff':    { en: { t: 'Cutoff' },    fr: { t: 'Coupure', reviewed: true },
        'zh-Hans': { t: "截止", reviewed: 'mt' }, },
    'label.resonance': { en: { t: 'Resonance' }, fr: { t: 'Résonance', reviewed: true },
        'zh-Hans': { t: "共振", reviewed: 'mt' }, },
    'label.envAmt':    { en: { t: 'Env Amt' },   fr: { t: 'Taux env.', reviewed: true },
        'zh-Hans': { t: "包络量", reviewed: 'mt' }, },
    'label.keyTrack':  { en: { t: 'Key Track' }, fr: { t: 'Suivi clavier', reviewed: true },
        'zh-Hans': { t: "键位跟踪", reviewed: 'mt' }, },

    // ── The two envelope groups ─────────────────────────────────────────────
    // Each title is a caption plus a routing suffix in its own .group-route
    // span. The two halves are two keys: applyLabel writes textContent, so a
    // key on the <h2> would delete the span.
    'label.groupFilterEnv': {
        en: { t: 'Filter Envelope' },
        fr: { t: 'Enveloppe du filtre', reviewed: true },
        'zh-Hans': { t: "滤波器包络", reviewed: 'mt' },
    },
    'label.routeToCutoff': {
        en: { t: '→ cutoff' },
        fr: { t: '→ coupure', reviewed: true },
        'zh-Hans': { t: "→ 截止", reviewed: 'mt' },
    },
    'label.groupAmpEnv': {
        en: { t: 'Amp Envelope' },
        fr: { t: 'Enveloppe d’amplitude', reviewed: true },
        'zh-Hans': { t: "振幅包络", reviewed: 'mt' },
    },
    'label.routeToLevel': {
        en: { t: '→ level' },
        fr: { t: '→ niveau', reviewed: true },
        'zh-Hans': { t: "→ 电平", reviewed: 'mt' },
    },
    // The four ADSR captions are shared by BOTH envelope groups — eight
    // elements, four keys.
    'label.attack':  { en: { t: 'Attack' },  fr: { t: 'Attaque', reviewed: true },
        'zh-Hans': { t: "起音", reviewed: 'mt' }, },
    'label.decay':   { en: { t: 'Decay' },   fr: { t: 'Déclin', reviewed: true },
        'zh-Hans': { t: "衰减", reviewed: 'mt' }, },
    'label.sustain': { en: { t: 'Sustain' }, fr: { t: 'Maintien', reviewed: true },
        'zh-Hans': { t: "延音", reviewed: 'mt' }, },
    'label.release': { en: { t: 'Release' }, fr: { t: 'Relâchement', reviewed: true },
        'zh-Hans': { t: "释音", reviewed: 'mt' }, },

    // ── Voice / Output group ────────────────────────────────────────────────
    'label.groupOutput': {
        en: { t: 'Voice / Output' },
        fr: { t: 'Voix / sortie', reviewed: true },
        'zh-Hans': { t: "发声、输出", reviewed: 'mt' },
    },
    'label.mode':  { en: { t: 'Mode' },  fr: { t: 'Mode', sameAsEn: true, reviewed: true },
        'zh-Hans': { t: "模式", reviewed: 'mt' }, },
    'label.glide': { en: { t: 'Glide' }, fr: { t: 'Portamento', reviewed: true },
        'zh-Hans': { t: "滑音", reviewed: 'mt' }, },
    'label.level': { en: { t: 'Level' }, fr: { t: 'Niveau', reviewed: true },
        'zh-Hans': { t: "电平", reviewed: 'mt' }, },

    // ── Preset tour ─────────────────────────────────────────────────────────
    // The button FACES are localized; the data-preset beside each one is the
    // C++ snapshot name and is never translated (it is the applyFactoryPreset
    // argument and the LESSON_CAPTION_WRITERS key in js/app.js).
    'label.tourLabel': {
        en: { t: 'Lesson Presets' },
        fr: { t: 'Leçons', reviewed: true },
        'zh-Hans': { t: "教学预设", reviewed: 'mt' },
    },
    'label.lessonSawSweep':   { en: { t: 'Saw Sweep' },        fr: { t: 'Balayage scie', reviewed: true },
        'zh-Hans': { t: "锯齿扫描", reviewed: 'mt' }, },
    'label.lessonPluck':      { en: { t: 'Pluck' },            fr: { t: 'Pincement', reviewed: true },
        'zh-Hans': { t: "拨奏", reviewed: 'mt' }, },
    'label.lessonBrass':      { en: { t: 'Brass Stab' },       fr: { t: 'Coup de cuivres', reviewed: true },
        'zh-Hans': { t: "铜管短奏", reviewed: 'mt' }, },
    'label.lessonSweep':      { en: { t: 'Sweep Pad' },        fr: { t: 'Nappe balayée', reviewed: true },
        'zh-Hans': { t: "扫描音垫", reviewed: 'mt' }, },
    'label.lessonAcid':       { en: { t: 'Acid Bass' },        fr: { t: 'Basse acid', reviewed: true },
        'zh-Hans': { t: "酸性贝斯", reviewed: 'mt' }, },
    'label.lessonSquareBass': { en: { t: 'Square Bass' },      fr: { t: 'Basse carrée', reviewed: true },
        'zh-Hans': { t: "方波贝斯", reviewed: 'mt' }, },
    'label.lessonNoiseWind':  { en: { t: 'Noise Wind' },       fr: { t: 'Vent de bruit', reviewed: true },
        'zh-Hans': { t: "噪声风声", reviewed: 'mt' }, },
    'label.lessonSelfOsc':    { en: { t: 'Self-Oscillation' }, fr: { t: 'Auto-oscillation', reviewed: true },
        'zh-Hans': { t: "自激振荡", reviewed: 'mt' }, },

    // The tour caption. Nine entries: the resting one authored in the markup,
    // and one per lesson written by setLabel when a button is clicked. Through
    // v1.2.5 the eight lesson captions lived in a LESSONS table in js/app.js
    // and were written with a raw textContent assignment; a string written that
    // way is stranded in the language it was picked in the instant the selector
    // fires, and it is the one string on this page chosen by a click.
    'label.captionDefault': {
        en: { t: 'Hover any control for an explanation · pick a lesson to hear a concept.' },
        fr: { t: 'Survolez n’importe quel réglage pour une explication · choisissez une leçon pour entendre un concept.',
              reviewed: true },
        'zh-Hans': { t: "悬停任意控件查看说明 · 选择一课来听一个概念。", reviewed: 'mt' },
    },
    'label.captionSawSweep': {
        en: { t: 'Saw → LP Sweep — a bright saw through a 24 dB low-pass with a slow filter envelope. Watch the harmonics fall away under the curve: the subtraction the method is named for.' },
        fr: { t: 'Dent de scie → balayage passe-bas — une dent de scie brillante à travers un passe-bas 24 dB avec une enveloppe de filtre lente. Regardez les harmoniques disparaître sous la courbe : la soustraction qui donne son nom à la méthode.',
              reviewed: true },
        'zh-Hans': { t: "锯齿波 → 低通扫描 —— 一段明亮的锯齿波，经过 24 dB 低通，配上一条缓慢的滤波器包络。看着谐波在曲线之下落去：这就是这套方法得名的减法。", reviewed: 'mt' },
    },
    'label.captionPluck': {
        en: { t: 'Pluck — a fast filter envelope snaps bright-then-dark while the amp decays quickly; the filter envelope does the timbral work.' },
        fr: { t: 'Pincement — une enveloppe de filtre rapide fait claquer le son du brillant vers le sombre pendant que l’amplitude décline vite ; c’est l’enveloppe du filtre qui fait le travail de timbre.',
              reviewed: true },
        'zh-Hans': { t: "拨奏 —— 一条快速的滤波器包络让音色先亮后暗地一弹，同时振幅迅速衰减；音色上的活儿是滤波器包络干的。", reviewed: 'mt' },
    },
    'label.captionBrass': {
        en: { t: 'Brass Stab — positive filter-env amount opens the cutoff with the attack and holds it, so brightness tracks the note like brass.' },
        fr: { t: 'Coup de cuivres — un taux d’enveloppe de filtre positif ouvre la coupure avec l’attaque et la tient, si bien que la brillance suit la note comme un cuivre.',
              reviewed: true },
        'zh-Hans': { t: "铜管短奏 —— 正的滤波器包络量随起音打开截止点并保持住，于是明亮度像铜管一样跟着音符走。", reviewed: 'mt' },
    },
    'label.captionSweep': {
        en: { t: 'Sweep Pad — slow amp swell + a long, deep filter sweep open the spectrum gradually. Pads live in slow envelopes.' },
        fr: { t: 'Nappe balayée — une montée d’amplitude lente et un balayage de filtre long et profond ouvrent le spectre peu à peu. Les nappes vivent dans les enveloppes lentes.',
              reviewed: true },
        'zh-Hans': { t: "扫描音垫 —— 缓慢的振幅涨起，加上一次又长又深的滤波扫描，把频谱逐渐打开。音垫活在慢包络里。", reviewed: 'mt' },
    },
    'label.captionAcid': {
        en: { t: 'Acid Bass — high resonance and a snappy filter envelope through a 24 dB low-pass make the squelchy, ringing acid sound. Mono with a touch of glide.' },
        fr: { t: 'Basse acid — une résonance élevée et une enveloppe de filtre nerveuse à travers un passe-bas 24 dB donnent le son acid gluant et sonnant. En mono, avec un soupçon de portamento.',
              reviewed: true },
        'zh-Hans': { t: "酸性贝斯 —— 高共振与一条利落的滤波器包络，经过 24 dB 低通，做出那种黏滑而鸣响的酸性音色。单声道，再加一点滑音。", reviewed: 'mt' },
    },
    'label.captionSquareBass': {
        en: { t: 'Square Bass — a hollow square plus the sub-oscillator for weight, played mono. The same voice as a polysynth, just one note at a time.' },
        fr: { t: 'Basse carrée — une onde carrée creuse plus le sous-oscillateur pour le poids, jouée en mono. La même voix qu’un polysynthé, mais une seule note à la fois.',
              reviewed: true },
        'zh-Hans': { t: "方波贝斯 —— 一段空心的方波加上副振荡器来添重量，以单声道演奏。和复音合成器里的声部一模一样，只是一次一个音。", reviewed: 'mt' },
    },
    'label.captionNoiseWind': {
        en: { t: 'Noise Wind — band-passed white noise with no harmonic source: the filter sculpts pitchless air into wind. Subtractive synthesis beyond notes.' },
        fr: { t: 'Vent de bruit — du bruit blanc en passe-bande, sans source harmonique : le filtre sculpte de l’air sans hauteur en vent. La synthèse soustractive au-delà des notes.',
              reviewed: true },
        'zh-Hans': { t: "噪声风声 —— 带通处理过的白噪声，没有谐波源：滤波器把没有音高的空气雕成风。这是音符之外的减法合成。", reviewed: 'mt' },
    },
    'label.captionSelfOsc': {
        en: { t: 'Self-Oscillation — resonance at the limit: the filter rings into a pure sine that plays in tune across the keyboard. The filter becomes the source.' },
        fr: { t: 'Auto-oscillation — résonance à la limite : le filtre sonne en une sinusoïde pure qui joue juste sur tout le clavier. Le filtre devient la source.',
              reviewed: true },
        'zh-Hans': { t: "自激振荡 —— 共振推到极限：滤波器鸣响成一条纯正弦，在整个键盘上都奏得准。滤波器变成了声源。", reviewed: 'mt' },
    },

    // ── Keyboard ────────────────────────────────────────────────────────────
    'label.play': {
        en: { t: 'Play ·' },
        fr: { t: 'Jouer ·', reviewed: true },
        'zh-Hans': { t: "播放 ·", reviewed: 'mt' },
    },
    // The letter run is the QWERTY key map, not prose: it names physical keys
    // and stays exactly as it is in both languages. Only the sentence around it
    // is translated.
    //
    // The letter run keeps its HAIR SPACES (U+200A, `&#8202;` in the markup) as
    // \u200a escapes. applyLabel writes this string over the authored markup,
    // so a plain space here would silently widen the key run in BOTH languages
    // — invisible to an en-vs-fr geometry diff, and a change to the shipped
    // English nobody asked for.
    'label.kbdHint': {
        en: { t: 'click the keys or use your computer keyboard (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)' },
        fr: { t: 'cliquez sur les touches ou utilisez le clavier de l’ordinateur (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)',
              reviewed: true },
        'zh-Hans': { t: "点击琴键或使用电脑键盘 (A\u200aS\u200aD\u200aF\u200aG\u200aH\u200aJ\u200aK · W\u200aE\u200aT\u200aY\u200aU)", reviewed: 'mt' },
    },

    // ── Accessible names ────────────────────────────────────────────────────
    // Read by data-i18n-aria. The knobs, combos and the two focusable canvases
    // read their tooltip TITLE through trLabel's I18N fallback instead — an
    // accessible name IS the control's name — so only the names that have no
    // tooltip live here.
    'aria.settings': {
        en: { t: 'Settings' }, fr: { t: 'Réglages', reviewed: true },
        'zh-Hans': { t: "设置", reviewed: 'mt' },
    },
    'aria.langSelect': {
        en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true },
        'zh-Hans': { t: "界面语言", reviewed: 'mt' },
    },
    'aria.helpToggle': {
        en: { t: 'Toggle hover help' }, fr: { t: "Activer ou désactiver les infobulles", reviewed: true },
        'zh-Hans': { t: "开关悬停帮助", reviewed: 'mt' },
    },
    // The switch's two faces, written through setLabel from applyTipsEnabled.
    // FEMININE PLURAL, agreeing with the noun naming the hover-help surface —
    // "les infobulles" as of v1.4.1. Singular through v1.4.0, agreeing with the
    // singular noun the suite used then. The dependency is stated only here:
    // these faces carry no occurrence of that noun, so a sweep over strings
    // that mention it does not reach them. Same pair, same reason, in
    // O-simpleSampler and O-SpectralShaper.
    'ui.on': {
        en: { t: 'On' }, fr: { t: 'Activées', reviewed: true },
        'zh-Hans': { t: "开启", reviewed: 'mt' },
    },
    'ui.off': {
        en: { t: 'Off' }, fr: { t: 'Désactivées', reviewed: true },
        'zh-Hans': { t: "关闭", reviewed: 'mt' },
    },
    'aria.keyboard': {
        en: { t: 'On-screen keyboard' }, fr: { t: 'Clavier à l’écran', reviewed: true },
        'zh-Hans': { t: "屏幕键盘", reviewed: 'mt' },
    },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    // The h1 splits the product name across two text nodes so the second half
    // can carry the green italic .title-accent. Both halves are the same
    // untranslatable name; keying either would translate half a wordmark.
    ['O – simple',
     'the product name, first half of the split wordmark in the page heading — a product name is never translated'],
    ['Subtractive',
     'the product name, second half of the split wordmark (.title-accent) — a product name is never translated'],

    // The three endonyms in the language selector. A language name is written
    // in its OWN language: a French speaker looking for their language looks
    // for "Français", not "French"; a Chinese reader looks for 简体中文.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
    ['简体中文', 'endonym — a language name is never translated'],

    // ── The signal-path diagram's three VALUE displays ──────────────────────
    // #oscModeText, #filterModeText and #routeMeta are written by
    // updateDiagram() from the four AudioParameterChoice values. Those choice
    // strings are the host automation contract and stay English under D-01;
    // translating the diagram without translating the automation lane would
    // make the page and the host disagree about what the plugin is set to. The
    // strings below are the authored MARKUP DEFAULTS for the same three nodes,
    // overwritten by updateDiagram on the first frame.
    ['Saw',
     'AudioParameterChoice entry (oscWave), mirrored in the OSC node of the diagram — the host automation name, English under D-01'],
    ['24 dB LP',
     'the FILTER node value display, composed from the filterSlope and filterType choice values — English under D-01'],
    ['low-pass · 24 dB/oct',
     'the routing meta readout, composed from the filterType and filterSlope choice values — English under D-01'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key, wrapper?, vars?]
//
// The tip anchor IS the element the selector finds: this page authors its tips
// on the cell rather than on the knob inside it, so no closest(wrapper) walk is
// needed anywhere.
//
// Through v1.2.5 the anchors carried the tip KEY in their own data-tip
// attribute and js/app.js looked the copy up in a TIPS object. That cannot
// survive canon v2 — applyI18n WRITES data-tip as the tip BODY, so the key and
// the copy would fight over one attribute, and check-i18n assertion 3 requires
// index.html to carry zero data-tip literals. The twenty-two parameter cells
// gained a data-param attribute naming the APVTS parameter they drive; the
// three panels and the two envelope canvases gained an id; the eight lesson
// buttons are addressed by the data-preset they already carried.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',                       'gear-btn'],
    ['#lang-select',                    'lang-select'],
    ['#help-toggle',                    'help-toggle'],

    ['#headlineWrap',                   'headline'],
    ['#scopeWrap',                      'scope'],
    ['#routingPanel',                   'routing'],

    ['[data-param="oscWave"]',          'oscWave'],
    ['[data-param="subLevel"]',         'subLevel'],
    ['[data-param="noiseLevel"]',       'noiseLevel'],

    ['[data-param="filterType"]',       'filterType'],
    ['[data-param="filterSlope"]',      'filterSlope'],
    ['[data-param="cutoff"]',           'cutoff'],
    ['[data-param="resonance"]',        'resonance'],
    ['[data-param="filterEnvAmount"]',  'filterEnvAmount'],
    ['[data-param="keyTrack"]',         'keyTrack'],

    ['#filterAdsrWrap',                 'filterAdsr'],
    ['[data-param="filterAttack"]',     'filterAttack'],
    ['[data-param="filterDecay"]',      'filterDecay'],
    ['[data-param="filterSustain"]',    'filterSustain'],
    ['[data-param="filterRelease"]',    'filterRelease'],

    ['#ampAdsrWrap',                    'ampAdsr'],
    ['[data-param="ampAttack"]',        'ampAttack'],
    ['[data-param="ampDecay"]',         'ampDecay'],
    ['[data-param="ampSustain"]',       'ampSustain'],
    ['[data-param="ampRelease"]',       'ampRelease'],

    ['[data-param="voiceMode"]',        'voiceMode'],
    ['[data-param="glide"]',            'glide'],
    ['[data-param="outputLevel"]',      'outputLevel'],

    ['.tour-btn[data-preset="Saw Sweep"]',        'lessonSawSweep'],
    ['.tour-btn[data-preset="Pluck"]',            'lessonPluck'],
    ['.tour-btn[data-preset="Brass Stab"]',       'lessonBrass'],
    ['.tour-btn[data-preset="Sweep Pad"]',        'lessonSweep'],
    ['.tour-btn[data-preset="Acid Bass"]',        'lessonAcid'],
    ['.tour-btn[data-preset="Square Bass"]',      'lessonSquareBass'],
    ['.tour-btn[data-preset="Noise Wind"]',       'lessonNoiseWind'],
    ['.tour-btn[data-preset="Self-Oscillation"]', 'lessonSelfOsc'],
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
