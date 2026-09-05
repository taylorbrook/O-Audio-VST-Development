/*
   This file is part of O-AnalogSaturation, an Ouaricon Audio plugin.
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
// i18n.js — O-AnalogSaturation page copy, English, French and Simplified
// Chinese (v1.5.0)
//
// ── v1.5.0: SIMPLIFIED CHINESE (zh-Hans rollout Stage 4, wave 4a) ───────────
//
// This plugin is the TRACER of wave 4a — the smallest table in the wave, wired
// end to end through every layer the wave touches before the other five copy
// it. 20 entries: 7 I18N (a title and a body each) + 13 LABELS (a title each).
// LANGUAGES is three long, the widening shape check-i18n assertion [1] accepts
// alongside the two-language one.
//
// ── THE RENDERINGS ARE THE GLOSSARY'S ───────────────────────────────────────
//
// Twelve of this page's fourteen name strings are TERMS keys in
// scripts/i18n-zh-glossary.js and take that term's ROOT rendering. Lint rule Z5
// is what holds them there:
//
//     IN 输入   OUT 输出   INTENSITY 强度   QUALITY 质量   AUTOGAIN 自动增益
//     Language 语言   Hover help 悬停帮助   On 开   Off 关   Settings 设置
//     Interface language 界面语言   Toggle hover help 开关悬停帮助
//
// The two that are not TERMS keys are `Model` (模型) and the snake plate's alt
// text (蛇形插画).
//
// NO GLOSSARY COLLISION ON THIS PAGE. The screen that finds the Dither/Jitter
// class — push every English name string through TERMS and report two DIFFERENT
// keys that land on one root and co-occur here — returns ZERO for this plugin.
// The screen is not inert: run against O-Bitrot it names that page's three
// collisions, jitter/dither on 抖动 among them.
//
// ── TYPOGRAPHY: THE RULES THE BODIES ANSWER TO ──────────────────────────────
//
//   Z1  full-width punctuation throughout — ，。；、： never the ASCII forms.
//       ASCII punctuation inside a Latin or numeric token (0.1, 2x) is legal
//       and is masked before the scan.
//   Z2  NO U+00A0 anywhere. The French bodies on this same page carry eight of
//       them; the full-width forms already carry their own half-em sidebearing.
//       Same page, opposite rule.
//   Z4  ONE PLAIN U+0020 at every Latin/digit-to-Han boundary — "0 到 100%",
//       "MAGNETIC 模拟磁带磁滞", "信号的 RMS". Table-scoped: the MIXTURE is the
//       finding, not either form, and every boundary here is the spaced one.
//   Z7  no full-width Latin or digits. Units and option words stay ASCII — ms,
//       Hz, tanh, RMS, 2x, 4x, MAGNETIC, TUBE, TRANSFORMER, DIODE, LOW, MID,
//       HIGH.
//   Z8  no plain space between two Han code points.
//
// THE SEVEN OPTION WORDS STAY ENGLISH INSIDE THE CHINESE BODIES, for exactly
// the reason the French entries give: they are AudioParameterChoice option
// strings (I18N_EXEMPT, D-01 arm 1) and they are what the BUTTONS say in every
// language. A Chinese body that translated them would send a Chinese reader
// looking for a button that is not on the page.
//
// ── THE CJK FONT TAIL: SIX OF THE TEN STACKS, AND WHERE IT GOES ─────────────
//
// The set was MEASURED, never reasoned from the [data-i18n] list: the page was
// served, switched to Chinese, every state in tests/i18n-states.json driven,
// every [data-tip] anchor hovered, and getComputedStyle().fontFamily read on
// every node that HOLDS or CAN RECEIVE a Han codepoint. Seventeen nodes, all
// resolving through one of ten literal declarations — there is no type token on
// this page to edit.
//
//   TOOK THE TAIL (6)
//     body               the INHERITED stack: .knob-label, .quality-label and
//                        .vu-label render their captions through it.
//     .autogain-toggle   the button's own caption — 自动增益.
//     .settings-toggle   the hover-help switch's caption — 开 / 关.
//     .settings-label    the LANGUAGE caption inside the popover — 语言.
//     .settings-select   the endonym <option>, and the closed select's own face
//                        when that option is the selected one. NOT a
//                        [data-i18n] node: the one place Han reaches the markup.
//     .tooltip           the runtime hover surface, and .tip-title inheriting
//                        from it. NEVER carries [data-i18n] — the renderer
//                        fills it from data-tip / data-tip-title at hover time,
//                        so a scan of keyed nodes alone misses it and every
//                        Chinese tooltip would fall back to a system face.
//
//   LEFT UNTOUCHED (4), each justified by what it RENDERS
//     .model-button      MAGNETIC / TUBE / TRANSFORMER / DIODE — option strings,
//     .quality-button    LOW / MID / HIGH — likewise. ASCII by contract; no Han
//                        can ever reach either.
//     .vu-scale          the dB ticks. Numbers.
//     .gear-btn          the glyph U+2699. Its Han arrives only through
//                        data-tip, which paints in #tooltip, and through
//                        aria-label, which is spoken rather than rendered —
//                        there is no glyph to fall back for.
//
// THE TAIL GOES BEFORE THE TRAILING GENERIC, NOT AFTER IT, and that placement
// is the finding this tracer contributes. Chromium resolves a bare `serif` or
// `sans-serif` against the DOCUMENT'S LANG, so under zh-Hans the generic is
// already a Chinese face and a tail written after it is never consulted.
// Measured with CSS.getPlatformFontsForNode: with the tail after `serif` the
// Han rendered Songti SC; moved before it, the same node renders PingFang SC,
// which is what the stack names. Latin is unaffected either way — Times New
// Roman precedes the tail and answers for it, so the en and fr arms are
// byte-unchanged, which the geometry gate proves rather than this comment.
//
// AND THE SAME MECHANISM CAUSED THE ONLY GEOMETRY FAILURE ON THIS PAGE.
// .vu-scale read `'Garamond', serif`; Garamond is not a macOS face, so the
// generic was the only survivor and the ASCII dB ticks rendered Times under en
// and fr and Songti SC under zh. All six ticks shrank, the space-between row
// redistributed, and assertion [7] named twelve movers — not one of them
// holding a translated string. Fixed by NAMING the face the other nine
// declarations already name. No line-height pin was needed anywhere on this
// page: the four existing declarations (1, 1.2, 1.2, 1.35) already cover every
// node at risk, and the three existing min-width pins were re-measured against
// the Chinese render rather than assumed safe.
//
// ── TWO FALSE SENTENCES REMOVED, NOT EXTENDED ───────────────────────────────
//
// tip.lang's en and fr bodies both COUNTED the selector's options ("English and
// French are available"), the defect Stage 3 hit four for four. tip.gear's en
// and fr bodies both claimed the popover holds no hover-help switch — and
// v1.4.0 put one there, so that claim shipped false for a whole version. Same
// class, same fix: the enumeration is REMOVED, in both languages, never
// extended. Naming three languages would put Han inside the en and fr bodies
// and drag the CJK tail onto the very baseline every gate measures against.
// The selector already lists the languages in their endonyms — the one form a
// reader recognises without knowing the page language.
//
// The French flags stay `reviewed: true` because both edits are DELETIONS of a
// sentence a French reader already read. No new French was authored here.
//
// ── THE REVIEW LIFECYCLE ────────────────────────────────────────────────────
//
// The zh flag is an ENUM, not the boolean French uses, because nobody on this
// project reads Chinese. Three levels: machine draft, back-translated, native.
// The THIRD STAYS OPEN and is not a blocker — a disclosed quality level, printed
// by lint rule R1 on every run and stated in the CHANGELOG in the same words.
//
// ── v1.3.1: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en, against scripts/i18n-fr-glossary.js, and
// — for every DSP claim — against PluginProcessor.cpp.
// Changed: 5 of 15 entries, 16 individual changes (2 terminology, 8 typography,
// 1 grammar, 5 meaning/idiom). sameAsEn: kept 0, translated 0 — no fr value on
// this page equals its en. termNote exemptions: 0.
// Left as drafted: the other 10 entries, all 9 LABELS among them.
// reviewed: false throughout — no native speaker yet.
//
// The decisions the next reader needs:
//
//   "input drive" STAYS "gain d'entrée", not the glossary's 'drive' → saturation.
//   On this page saturation is the WHOLE effect (the plugin is named for it and
//   tip.model opens "Choisit le circuit de saturation"), so "la saturation
//   d'entrée" would name the effect where the sentence means the input gain into
//   it — and the code is literally an input multiply (drive = 1 + wetMix * RANGE,
//   x = input * drive, processDiodeSample and its three siblings). No termNote:
//   the lint's G1 runs on LABELS and tip TITLES only, and "drive" is neither here.
//
//   "clean" → "signal direct", not "signal propre". The glossary settles dry as
//   'direct' and this IS the dry path — mixDryWet weights wet by intensity/100
//   and dry by the complement (PluginProcessor.cpp:518-524).
//
//   "low-end weight / top-end sheen" → "dans le grave / dans l'aigu", not "en bas
//   / en haut". Band names, not directions — the glossary's low/high rule. It
//   also makes tip.model internally consistent: its MAGNETIC clause already said
//   "dans le grave".
//
//   THREE CAPTIONS APPEAR IN TWO CASINGS ON THIS PAGE and each French follows its
//   OWN English: LABELS carry INTENSITY / QUALITY / AUTOGAIN (all caps, accents
//   ON the capitals — INTENSITÉ, QUALITÉ, GAIN AUTO) while the tip TITLES carry
//   Intensity / Quality / Autogain (sentence case — Intensité, Qualité, Gain
//   auto). .tip-title and .knob-label are both text-transform: uppercase, so the
//   split is invisible on screen and visible to the lint and to the accessible
//   name. Lint C1 is 0 either way; the table's casing is what was followed.
//
//   "2x" and "4x" kept as spelled in the English. A multiplier is not a unit;
//   the lint's UNITS list does not carry it and French audio writes it the same.
//
// Typography: 8 U+00A0 inserted (5 before ';', 2 before ':', 1 before '%'), all
// inside fr b: values — `grep -n $'\xc2\xa0' | grep -v "t: \|b: "` is empty and
// re-importing both revisions reports 0 en entries changed, keys, TIP_BINDINGS
// and I18N_EXEMPT byte-identical.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). This
// plugin's controller is an inline <script type="module"> in index.html, so
// that failure mode would take the WHOLE UI, not a panel of it.
// scripts/check-i18n.js assertion 7 enforces it.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// ── v1.3.0 GIVES THIS PLUGIN HOVER-HELP, RENDERER INCLUDED ──────────────────
//
// v1.2.0 shipped an EMPTY I18N and an EMPTY TIP_BINDINGS, correctly: the page
// carried no data-tip, no data-tooltip and no native title= anywhere, and the
// boot report read `title= 0`. v1.3.0 authors the copy — six entries, four
// parameters plus the two chrome controls.
//
// AUTHORING THE COPY WAS NEVER ENOUGH ON ITS OWN. applyI18n() writes
// data-tip-title and data-tip ATTRIBUTES onto the anchors named below and stops
// there; the thing that READS those attributes and paints a surface is
// per-plugin code outside the canon, and this page had none of it — no
// #tooltip element, no .tooltip rule, no hover handler. Six bodies bound with
// no renderer would have passed check-i18n (assertion 2 only counts bindings),
// check-ui-labels (no tooltip awareness at all) and boot-all-uis (counts
// aria-label and title, never data-tip) while showing the user nothing. So
// v1.3.0 ports the renderer into index.html beside the copy, and
// tests/ui_tip_render_check.js is the gate that actually hovers each anchor and
// reads the surface back.
//
// THE OPTION WORDS STAY ENGLISH INSIDE THE FRENCH BODIES. MAGNETIC, TUBE,
// TRANSFORMER, DIODE, LOW, MID and HIGH are AudioParameterChoice option strings
// (I18N_EXEMPT below, D-01 arm 1) and they are what the BUTTONS say in both
// languages. A French body that translated them would send a French reader
// looking for a button that is not on the page, and would disagree with the
// host's automation lane. The sentence AROUND them is French; the tokens are
// not. AUTOGAIN's Off/On is different — it is an AudioParameterBool, its Off/On
// is JUCE's default bool text and appears on no button here, so that pair is
// ordinary prose and IS localized.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing an
// angle bracket, so machine-drafted French cannot open a markup path.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr', 'zh-Hans'];

// ============================================================================
// I18N — hover-help copy. {t, b}: a title and a body.
//
// SIX entries: the four APVTS parameters (INTENSITY, MODEL, QUALITY, AUTOGAIN —
// the runtime dump in .planning/params.tsv, not a regex over
// createParameterLayout()) plus #gear-btn and #lang-select.
//
// TITLES ARE THE PAGE'S OWN CAPTIONS, not the parameter display names, wherever
// the two differ: the user is reading the page. .tip-title is
// text-transform: uppercase, so "Intensity" paints as INTENSITY and "Gain auto"
// as GAIN AUTO — byte-for-byte what the caption under the control says in that
// language. MODEL is the one control with no caption at all on this page (the
// four buttons ARE the row), so its title falls back to the parameter's display
// name, "Model".
//
// EVERY BODY ENDS WITH THE RANGE. INTENSITY's unit came straight from the dump's
// `label` column (%) — it did NOT have to be recovered from a formatter,
// because this page renders no numeric readout for the knob at all: the value is
// shown by the indicator dot and by the snake's opacity. The other three are
// discrete, so their range is their option words.
//
// THE NUMBERS INSIDE A BODY ARE PROSE AND THEY LOCALIZE (D-03 binds to NODES,
// not to sentences): "0 to 100 %" becomes "0 à 100 %", and the decimal comma is
// used in French. A READOUT NODE would be exempt; there is no readout node here.
// ============================================================================

export const I18N = Object.freeze({

    // ── The settings popover ────────────────────────────────────────────────
    // The gear tip is what tells a user hover-help exists at all, so its body
    // describes what the popover is FOR and stops there.
    //
    // v1.4.0 IS WHY IT NO LONGER ENUMERATES. Until then this body listed what
    // the popover does not hold, and v1.4.0 added the hover-help switch three
    // lines of markup below it — so the tip shipped a false claim for a whole
    // version. A sentence that counts what a surface holds goes false the
    // moment the surface gains a member, and no gate in this repo can see it:
    // the sentence stays grammatical and the tooltip still renders. The
    // language body two entries down had the identical defect and took the
    // identical fix. Describe the purpose; never enumerate the contents.
    'tip.gear': {
        en: { t: 'Settings',
              b: 'Opens the panel holding the interface language. The choice is saved with the session, so a project reopens in the language it was saved in.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau qui contient la langue de l’interface. Le choix est enregistré avec la session, donc un projet se rouvre dans la langue où il a été enregistré.',
              reviewed: true },
        'zh-Hans': { t: '设置',
              b: '打开包含界面语言的面板。该选择会随会话一同保存，因此项目会以保存时的语言重新打开。',
              reviewed: 'mt' },
    },

    // The last sentence is not decoration: it is the page telling the user why
    // seven of its captions stay English when everything around them turns
    // French. Without it the model and quality rows read as a missed translation.
    'tip.lang': {
        en: { t: 'Language',
              b: 'The language of this page’s labels and of this hover help. The model and quality buttons keep their English names on purpose, so the page and the host’s automation lane name the same setting the same way.' },
        fr: { t: 'Langue',
              b: 'La langue des libellés de cette page et de ces infobulles. Les boutons de modèle et de qualité gardent leur nom anglais à dessein, afin que la page et la voie d’automation de l’hôte désignent le même réglage de la même façon.',
              reviewed: true },
        'zh-Hans': { t: '语言',
              b: '本页标签与这些悬停帮助的语言。模型与质量按钮特意保留英文名称，'
               + '让页面与宿主的自动化通道用同样的方式称呼同一个设置。',
              reviewed: 'mt' },
    },
    // v1.4.0 — the switch that reaches this whole layer.
    'tip.tipsToggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this '
               + 'switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
               + 'l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
        'zh-Hans': { t: '悬停帮助',
              b: '开启或关闭这些悬停帮助。关闭后，'
               + '只有齿轮和这个开关仍会自我说明。',
              reviewed: 'mt' },
    },

    // ── The four parameters ─────────────────────────────────────────────────

    // INTENSITY — AudioParameterFloat, 0..100, label "%", default 50
    // (PluginProcessor.cpp:41-47). It is BOTH the input drive into the model and
    // the wet proportion of the dry/wet mix — mixDryWet() weights the wet path
    // by intensity/100 (PluginProcessor.cpp:518-524) while each model scales its
    // own drive by the same figure. The body says both, because a user who
    // reads only "drive" cannot explain why 0 % is silent-clean.
    'tip.intensity': {
        en: { t: 'Intensity',
              b: 'Sets the input drive into the selected model and, with it, how much of the saturated signal is mixed back over the clean one. Low values add a gentle harmonic warmth; high values round the peaks and thicken the tone. 0 to 100 %.' },
        fr: { t: 'Intensité',
              b: 'Règle le gain d’entrée dans le modèle choisi et, du même geste, la proportion de signal saturé remélangée au signal direct. Les valeurs basses ajoutent une chaleur harmonique discrète ; les valeurs hautes arrondissent les crêtes et épaississent le timbre. 0 à 100 %.',
              reviewed: true },
        'zh-Hans': { t: '强度',
              b: '设定进入所选模型的输入驱动，并同时决定有多少饱和信号叠回干净信号之上。'
               + '低值带来柔和的泛音暖意；高值则修圆峰值并加厚音色。'
               + '0 到 100%。',
              reviewed: 'mt' },
    },

    // MODEL — AudioParameterChoice, 4 options (PluginProcessor.cpp:50).
    // The four option words appear VERBATIM in both bodies; see the header.
    // The descriptions are read off the implementations rather than invented:
    // MAGNETIC is a Jiles-Atherton hysteresis model with a head-bump peak and an
    // HF rolloff (processMagneticSample); TUBE clips asymmetrically and lifts
    // 3 kHz (processTubeSample); TRANSFORMER is a tanh core with a 60 Hz bump
    // and an 8 kHz shelf (processTransformerSample); DIODE is the symmetric
    // TS-style x/(1+|x|)^n clipper (processDiodeSample).
    'tip.model': {
        en: { t: 'Model',
              b: 'Chooses the saturation circuit. MAGNETIC models tape hysteresis, with a low head bump and softened highs; TUBE clips asymmetrically for even harmonics and a presence lift; TRANSFORMER is a soft tanh curve with low-end weight and top-end sheen; DIODE clips symmetrically for a harder, odd-harmonic edge. Four settings: MAGNETIC, TUBE, TRANSFORMER, DIODE.' },
        fr: { t: 'Modèle',
              b: 'Choisit le circuit de saturation. MAGNETIC modélise l’hystérésis de la bande, avec une bosse dans le grave et des aigus adoucis ; TUBE écrête de façon asymétrique, pour des harmoniques paires et un relèvement de présence ; TRANSFORMER est une courbe tanh douce, avec du poids dans le grave et de l’éclat dans l’aigu ; DIODE écrête symétriquement, pour un mordant plus dur, riche en harmoniques impaires. Quatre réglages : MAGNETIC, TUBE, TRANSFORMER, DIODE.',
              reviewed: true },
        'zh-Hans': { t: '模型',
              b: '选择饱和电路。MAGNETIC 模拟磁带磁滞，带有低频磁头凸起与柔化的高频；'
               + 'TUBE 非对称削波，产生偶次泛音与临场感提升；'
               + 'TRANSFORMER 是柔和的 tanh 曲线，低频有分量、高频有光泽；'
               + 'DIODE 对称削波，带来更硬的奇次泛音棱角。'
               + '四个设定：MAGNETIC、TUBE、TRANSFORMER、DIODE。',
              reviewed: 'mt' },
    },

    // QUALITY — AudioParameterChoice, 3 options (PluginProcessor.cpp:58).
    // The factors are read from osFactorForQuality() and the latency claim from
    // computeLatencyForQuality(), which returns 0 for LOW and the oversampler's
    // own FIR latency for MID and HIGH. Saying "adds latency" is not padding:
    // it is the one consequence of this control a user cannot hear until they
    // are looking for a timing problem.
    'tip.quality': {
        en: { t: 'Quality',
              b: 'Sets the internal oversampling that keeps the saturation from folding aliasing back down into the audible band. LOW runs at the host’s own rate and adds no latency; MID oversamples 2x and HIGH 4x, each reporting its filter latency to the host for compensation. LOW, MID or HIGH.' },
        fr: { t: 'Qualité',
              b: 'Règle le suréchantillonnage interne qui empêche la saturation de rabattre du repliement de spectre dans la bande audible. LOW travaille à la fréquence d’échantillonnage de l’hôte et n’ajoute aucune latence ; MID suréchantillonne 2x et HIGH 4x, chacun signalant sa latence de filtre à l’hôte pour compensation. LOW, MID ou HIGH.',
              reviewed: true },
        'zh-Hans': { t: '质量',
              b: '设定内部过采样，防止饱和把混叠折回可听频段。'
               + 'LOW 以宿主自身的采样率运行，不增加延迟；'
               + 'MID 以 2x 过采样，HIGH 以 4x，'
               + '两者都会把各自的滤波器延迟报告给宿主以便补偿。'
               + 'LOW、MID 或 HIGH。',
              reviewed: 'mt' },
    },

    // AUTOGAIN — AudioParameterBool, default false (PluginProcessor.cpp:63-67).
    // NOT an AudioParameterChoice, so its Off/On is JUCE's default bool text and
    // appears on no button on this page — D-01 arm 1 does not reach it and the
    // pair is localized as ordinary prose, unlike the seven option words above.
    // The 0.1..10 bound is the jlimit in applyAutoGain().
    'tip.autogain': {
        en: { t: 'Autogain',
              b: 'Matches the output level back to the input level, so a change of intensity or of model is judged on tone rather than on loudness. It follows the signal’s RMS on a smoothed ramp and is bounded to a factor of 0.1 to 10, so it lifts a quiet passage without running away. Off or On.' },
        fr: { t: 'Gain auto',
              b: 'Ramène le niveau de sortie au niveau d’entrée, pour qu’un changement d’intensité ou de modèle se juge au timbre plutôt qu’au volume. Il suit la valeur efficace du signal sur une rampe lissée et reste borné à un facteur de 0,1 à 10, de sorte qu’il relève un passage de faible niveau sans s’emballer. Désactivé ou activé.',
              reviewed: true },
        'zh-Hans': { t: '自动增益',
              b: '把输出电平拉回到输入电平，让强度或模型的改变以音色而非响度来判断。'
               + '它以平滑的斜坡跟随信号的 RMS，并被限制在 0.1 到 10 倍之间，'
               + '因此能抬起安静的段落而不失控。关或开。',
              reviewed: 'mt' },
    },
});

// ============================================================================
// LABELS — the visible text of the page. {en:{t}, fr:{t, reviewed}}.
//
// One string per entry, no body: a label is not a tooltip.
//
// ── WHAT IS *NOT* HERE, AND WHY (the D-01 test) ─────────────────────────────
//
// Seven of the thirteen visible strings on this page are EXEMPT, and every one
// of them is an I18N_EXEMPT entry with its reason rather than a silent skip.
// The four model captions and the three quality captions are the
// AudioParameterChoice option strings VERBATIM — see the note in I18N_EXEMPT.
//
// ── GEOMETRY ────────────────────────────────────────────────────────────────
//
// Every label on this page lives in an ABSOLUTELY POSITIONED box or a
// fixed-width one, so no French string can push a sibling: .knob-label is
// `left: 175px; width: 90px; text-align: center`, .vu-label is
// `left: 50%; transform: translateX(-50%)`, .quality-label is a bare absolute
// box with no following sibling in flow, and .autogain-toggle is a hard
// `width: 100px`. The French was still chosen to fit rather than to rely on
// that. MEASURED at 600 x 450, rendered text width against the box it sits in:
//
//     .autogain-toggle   AUTOGAIN  57.4 -> GAIN AUTO 59.9  in an 84.0 content
//                        box (100 px border-box, 2 px borders, the UA button's
//                        own 6 px side padding) — 24.1 px spare
//     .knob-label        INTENSITY 68.2 -> INTENSITÉ 67.0  in 90 px — SHRANK
//     .quality-label     QUALITY   47.0 -> QUALITÉ   46.0  — SHRANK
//     .vu-label in       IN         8.9 -> ENTRÉE    32.9  centred by transform
//                        in a 90 px face: 83.5..116.4, well inside 55..145
//     .vu-label out      OUT       17.3 -> SORTIE    30.0  — same, inside
//
// Two of the five SHRINK rather than grow, which is the half a clip check is
// blind to and the half Stage J found four times in twelve.
//
// The check-ui-labels assertion-7 diff reports zero moved elements, and that
// verdict is NOT vacuous: a negative control that lengthened label.language
// until it pushed #lang-select inside the popover row made assertion 7 report
// the move by name, so the sweep can see this page.
// ============================================================================

export const LABELS = Object.freeze({

    // ── The two VU meters ───────────────────────────────────────────────────
    // Full words rather than the ENT/SORT abbreviations: .vu-label is centred
    // by transform inside a 90 px face with nothing beside it, so the extra
    // characters cost nothing and the meaning is not left to be guessed.
    'label.in':  { en: { t: 'IN' },  fr: { t: 'ENTRÉE', reviewed: true }, 'zh-Hans': { t: '输入', reviewed: 'mt' } },
    'label.out': { en: { t: 'OUT' }, fr: { t: 'SORTIE', reviewed: true }, 'zh-Hans': { t: '输出', reviewed: 'mt' } },

    // ── The intensity knob ──────────────────────────────────────────────────
    // The caption under the knob, NOT a readout: this node never holds a
    // number. The knob has no numeric readout at all on this page — the value
    // is shown by the indicator dot and by the snake's opacity — so there is no
    // readout/label node to split (contract §5).
    'label.intensity': { en: { t: 'INTENSITY' }, fr: { t: 'INTENSITÉ', reviewed: true }, 'zh-Hans': { t: '强度', reviewed: 'mt' } },

    // ── The quality section heading ─────────────────────────────────────────
    // The heading localizes; the three BUTTONS under it do not. The heading is
    // this page's own caption for the group, and "Quality" is the
    // AudioParameterChoice's DISPLAY NAME rather than one of its option
    // strings, so nothing in the host automation lane is spelled "QUALITY".
    'label.quality': { en: { t: 'QUALITY' }, fr: { t: 'QUALITÉ', reviewed: true }, 'zh-Hans': { t: '质量', reviewed: 'mt' } },

    // ── The auto-gain toggle ────────────────────────────────────────────────
    // AUTOGAIN is the APVTS parameter ID, not a choice option — the parameter
    // is an AudioParameterBool whose display name is "Auto Gain"
    // (PluginProcessor.cpp:63-67). A bool has no option strings, so there is no
    // automation-lane string for a French caption to disagree with, and arm 1
    // of D-01 does not apply. GAIN AUTO is the standard French word order.
    'label.autogain': { en: { t: 'AUTOGAIN' }, fr: { t: 'GAIN AUTO', reviewed: true }, 'zh-Hans': { t: '自动增益', reviewed: 'mt' } },

    // ── The settings popover (v1.2.0) ───────────────────────────────────────
    'label.language': { en: { t: 'Language' }, fr: { t: 'Langue', reviewed: true }, 'zh-Hans': { t: '语言', reviewed: 'mt' } },

    // v1.4.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true }, 'zh-Hans': { t: '悬停帮助', reviewed: 'mt' } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true }, 'zh-Hans': { t: '开', reviewed: 'mt' } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true }, 'zh-Hans': { t: '关', reviewed: 'mt' } },

    // ── Accessible names ────────────────────────────────────────────────────
    // Resolved through the same sweep via data-i18n-aria / data-i18n-alt, so a
    // screen reader hears the same language the page is showing.
    'aria.settings':   { en: { t: 'Settings' },           fr: { t: 'Réglages',              reviewed: true }, 'zh-Hans': { t: '设置', reviewed: 'mt' } },
    'aria.langSelect': { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true }, 'zh-Hans': { t: '界面语言', reviewed: 'mt' } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true }, 'zh-Hans': { t: '开关悬停帮助', reviewed: 'mt' } },

    // The decorative plate behind the knob. Its alt text was the page's only
    // prose-bearing attribute at v1.1.6 and was unkeyed; it is keyed here
    // rather than emptied, because the illustration changes with the model and
    // a blind user is entitled to know something is there.
    'alt.snake': { en: { t: 'Snake illustration' }, fr: { t: 'Illustration de serpent', reviewed: true }, 'zh-Hans': { t: '蛇形插画', reviewed: 'mt' } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
// ============================================================================

export const I18N_EXEMPT = [
    ['OUARICON SATURATION',
     'the product display name in .title — a product name is never translated, and this is the brand-plus-product form of the plugin\'s registered PRODUCT_NAME "O-AnalogSaturation" in CMakeLists.txt'],

    // ── D-01 arm 1: the captions that ARE the option strings ────────────────
    //
    // The four model buttons and the three quality buttons carry the
    // AudioParameterChoice option strings BYTE FOR BYTE
    // (PluginProcessor.cpp:47-61). Translating the caption alone would make the
    // page and the host automation lane disagree about the same setting: a DAW
    // showing MODEL = "TRANSFORMER" beside a page reading "TRANSFO" is a bug
    // report, not a localization.
    //
    // Byte-identity is the test, and it is the reason these seven differ from
    // O-Gain's LOW/MED/HIGH confidence verdict, which localizes: that node is
    // not backed by a parameter at all.
    ['MAGNETIC',
     'a MODEL AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:50, StringArray {"MAGNETIC","TUBE","TRANSFORMER","DIODE"}) — D-01 arm 1'],
    ['TUBE',
     'a MODEL option string VERBATIM (PluginProcessor.cpp:50) — D-01 arm 1'],
    ['TRANSFORMER',
     'a MODEL option string VERBATIM (PluginProcessor.cpp:50) — D-01 arm 1'],
    ['DIODE',
     'a MODEL option string VERBATIM (PluginProcessor.cpp:50) — D-01 arm 1. Also spelled identically in French'],
    ['LOW',
     'a QUALITY AudioParameterChoice option string VERBATIM (PluginProcessor.cpp:58, StringArray {"LOW","MID","HIGH"}) — D-01 arm 1'],
    ['MID',
     'a QUALITY option string VERBATIM (PluginProcessor.cpp:58) — D-01 arm 1'],
    ['HIGH',
     'a QUALITY option string VERBATIM (PluginProcessor.cpp:58) — D-01 arm 1'],

    // ── Endonyms ────────────────────────────────────────────────────────────
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
    ['简体中文', 'endonym — a language name is never translated'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper].
//
// applyI18n() runs document.querySelector(selector), then closest(wrapper) when
// a wrapper is given, and writes data-tip-title + data-tip onto whatever that
// resolves to. Any CSS selector is legal, so an anchor does NOT have to be an
// id — and on this page two of the four parameters have no id anywhere near
// them.
//
// BIND THE BOX THE USER AIMS AT.
//
//   MODEL and QUALITY have no id and no per-parameter element at all: each is a
//   ROW of buttons (.model-buttons, .quality-buttons) and the parameter is the
//   row, not any one button. Binding a single button would leave three quarters
//   of the target dead; the wrapper form binds the row, including the 10 px
//   flex gaps between the buttons, and closest() finds it from any child.
//
//   INTENSITY does have an id, #intensityKnob, but its two visible children
//   (.knob-segments, .knob-indicator) are pointer-events: none and the caption
//   .knob-label is a SEPARATE absolutely-positioned box outside the container.
//   .knob-container is the 90x90 cell the user actually points at.
//
// Every selector here is asserted to RESOLVE by
// tests/ui_tip_render_check.js §1 — an applyI18n console warning
// "tip target not found" is a real failure, not noise, and boot-all-uis is the
// gate that would otherwise see it first.
//
// NOT BOUND, deliberately: the .knob-label caption (pointer-events: none, so it
// could not receive a hover even if it were bound), the two VU meters and their
// IN/OUT captions (readouts, not controls — no parameter behind them), the
// title, the version label and the snake plate.
// ============================================================================

export const TIP_BINDINGS = [
    ['#gear-btn',       'tip.gear'],
    ['#lang-select',    'tip.lang'],
    ['#tips-toggle',    'tip.tipsToggle'],

    ['#intensityKnob',  'tip.intensity', '.knob-container'],
    ['.model-button',   'tip.model',     '.model-buttons'],
    ['.quality-button', 'tip.quality',   '.quality-buttons'],
    ['#autogainToggle', 'tip.autogain'],
];

// The tooltip lookup. Returns {t, b} — never null, never a bare key without a
// console.warn saying so, because a silently-missing tip renders as an empty
// surface that looks like a positioning bug rather than a missing entry.
//
// Live as of v1.3.0: applyI18n() calls it once per TIP_BINDINGS row, six times
// per language change. It is exported verbatim, byte-identical to the other
// forty-two copies, so the canon gate has one shape to compare against.
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
