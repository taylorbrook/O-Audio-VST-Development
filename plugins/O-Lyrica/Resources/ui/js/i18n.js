/*
   This file is part of O-Lyrica, an Ouaricon Audio plugin.
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
// i18n.js — O-Lyrica UI copy, English + French (v2.4.3, canon v2)
//
// ── v2.4.3: ENGLISH DEFECT FOUND BY THE FRENCH (Stage O, 2026-08-31) ───────
// Item 51: the `technique` body said "Harmonics produce" while the Choice face
// on #technique reads "Harmonic" (an I18N_EXEMPT choiceMirror, English on
// screen in both languages). A body must quote the face the user can see, so
// the en body now says "Harmonic produces". The fr body already quoted the
// face ("Harmonic donne des sons de cloche") — Stage N wrote it from the face,
// not from the English — so it is unchanged and keeps reviewed: true. No key,
// binding, selector or exemption changed; 1 en entry of 167.
// Tip height, measured at 700 x 450: en 69 px before and after (the sentence
// lost one glyph and gained one), fr 83 px unchanged.
//
// ── v2.4.2: FRENCH QA PASS (Stage N, 2026-08-31) ────────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 37 entries of 167 (21 terminology, 14 typography, 2 meaning; the
// terminology count carries one gender-agreement repair). sameAsEn: kept 14,
// translated 0, added 1. termNote exemptions: 0. Lint 42 -> 0, --strict exit 0.
// Left as drafted: the other 130. reviewed: false throughout — no native
// speaker yet.
//
// THE DECISIONS THE NEXT READER NEEDS, each measured on THIS page at 700 x 450
// with Range.selectNodeContents on the real node, never inherited from a header:
//
//   ON/OFF, four faces, TWO forms of one term. .gliss-toggle is float: right in
//   a 654 px .section-header with nothing pinned, so the ROOT fits: ACTIVÉ
//   49.14 px, DÉSACTIVÉ 65.16 px (was OUI 33.13 / NON 36.23 — Oui/Non are
//   answers, not states). .fx-bypass-btn and .settings-toggle are pinned by the
//   min-widths GEOMETRY RULE 9 and the .settings-toggle comment document, so
//   they take the glossary's listed abbreviation: Marche measures 45.48 and
//   Arrêt 37.00 against the 34 px pin and move the auto-margin-centred knob row
//   5.73 px and 1.50 px; Act. is 34.00 (row unmoved, identical to English) and
//   Dés. 34.25 (row 0.12 px, an order of magnitude under check-ui-labels' 0.5 px
//   tolerance). Stage N takes no CSS decision — see item 40's family.
//
//   Réverbe was not a word (glossary F1). The ROOT does not fit: Réverbération
//   measures 109.38 px and grows .fx-title past its 61 px min-width to 109.11,
//   which moves the knob row. Réverb 51.84 px is the listed abbreviation — and
//   it is 8.35 px NARROWER than the RÉVERBE 60.19 that GEOMETRY RULE 10 in
//   index.html sized the 61 px min-width against. That comment is now stale by
//   7.94 px; it is a comment, so it moves in its own docs commit, not here.
//
//   Amortissement 87.20 px would grow the nowrap shrink-to-fit .knob-label from
//   48.95 to 87.20. Amort. 38.77 px is the listed abbreviation and is 10.18 px
//   narrower than the Amortis. it replaces.
//
//   Infobulles — the glossary root as of v2.4.4, RE-MEASURED at 45.55 px rather
//   than scaled from the caption it replaces (that caption is recorded in the
//   CHANGELOG and deliberately not repeated here, so a repo grep for it stays
//   at zero). .settings-row is space-between over 154 px of content, so
//   45.55 + 12 gap + the 40 px toggle leaves 56.45 px of slack — up from 36 —
//   the popover stays 178 px and the toggle's left edge does not move. The new
//   caption is 4.17 px NARROWER than English "Hover help" (49.72), where the
//   v2.4.3 one was 16.17 px wider; the French row is no longer the widest of
//   the two. Neither caption was ever forced by width, then or now.
//
//   Brill. chevalet replaces Chevalet: the caption had dropped what English's
//   own abbreviation keeps ("Bridge Bright" is bridge AND brightness). 84.34 px
//   in a fixed 118.8 px .slider-label, one line, .slider-group height and top
//   unchanged. The unabbreviated Brillance chevalet also fits at 108.09 px but
//   leaves 10.71 px, so the abbreviation is used — English abbreviates the same
//   caption for the same reason.
//
//   Touches KEPT for "True Keys" and reported rather than fixed: Touches réelles
//   measures 77.47 px and grows the .viz-btn from 65 to 95.47 px and the
//   five-button row from 302 to 332.47 px. The caption stays short.
//
//   Demi-tons libres now titles BOTH Custom Semitones tips. The two English
//   titles are byte-identical and the BODIES do the distinguishing; the draft's
//   Free/Scale split was a distinction French made and English did not.
//
//   "Tuning" is a gamme, never a tempérament. Three bodies moved (lang-select,
//   glissandoScale, ref-pitch-knob): tempérament is reserved for temperament —
//   "Tempérament de rang 2", "tempérament égal" — and the tuning panel is
//   already "Bibliothèque de gammes". aria.deviation takes Déviation for the
//   same reason: Écart total is the settled span term on that same page.
//
//   Le matériau, masculine, carried its three adjectives with it: "Gut est
//   chaud et doux ; Wire est brillant" where the draft agreed with la matière.
//
//   FIVE tip titles equal their English over a translated body (timbre,
//   stringTension, technique, freeTempoSync, scaleTempoSync). They take NO
//   sameAsEn — check-i18n reads the flag entry-scoped, so flagging them would
//   disarm assertion 4 for their bodies. label.knMix DID gain the flag: the
//   glossary root for "Mix" is Mix, which makes the French a straight copy.
//
//   Bodies address the user as vous; instructions are infinitive throughout
//   ("Cliquer pour jouer", "Tenir 2 notes ou plus", "Double-cliquer pour
//   modifier"), which is the register the draft already used.
//
//   The typography pass ran a scanner that skips comments and rewrites only the
//   STRING VALUE of a t:/b: pair inside an fr: { } block; its scope-leak control
//   re-scanned the output and proved 0 of 213 en string values changed.
//   22 U+00A0 in the file, none outside a t:/b: line.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// scripts/check-i18n.js assertion 7 enforces that.
//
// SERVED ROOT IS Resources/ui. O-Lyrica has only the one UI tree and
// CMakeLists.txt embeds it directly. THE BINARY-DATA TARGET HAS NO NAMESPACE
// ARGUMENT — juce_add_binary_data(OLyricaBinaryData SOURCES ...) at
// CMakeLists.txt:88 takes the default BinaryData namespace and works only
// because it is the only such target in this plugin. This file was added to
// that EXISTING SOURCES list; a second juce_add_binary_data target would
// collide on the BinaryData namespace and break the build in a way that reads
// like something else entirely (critical_dual_binary_data_namespace_collision).
//
// FOUR PLACES, ONE COMMIT: this file on disk, the existing SOURCES list, a
// getResource() branch in PluginEditor.cpp, and the import in js/app.js. Miss
// one and the page 404s at runtime and presents as a dead panel with no other
// symptom (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// NO MARKUP. This table is data, never HTML. The tooltip renderer in js/app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing the
// opening angle bracket.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. v2.3.3 authored its hover help as a
// SINGLE data-tooltip string in the shape "Label: sentence.", read by a second
// tooltip renderer this version deletes. Every tooltip entry below is that
// string split on its FIRST ": " into the t/b pair the measure-then-pin
// renderer wants, with both halves byte-identical to v2.3.3 either side of the
// separator. ALL FORTY-THREE SPLIT CLEANLY — there is no hand-split on this
// plugin, and that is measured rather than assumed: the longest surviving title
// is "Bridge Brightness" at 17 characters and no body begins before its own
// colon.
//
// THREE NEW CONTROLS carry new English copy: `settings`, `lang-select` and
// `tips-toggle`. The first two are the gear popover and the language selector,
// which did not exist before; the third is the hover-help toggle, which did
// exist as a floating "?" in the same corner and had only a native title=.
// Authoring hover-help prose for controls that have none is Stage M's job and
// is NOT done here.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
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
//
// THE KEY IS THE ANCHOR'S OWN CONTROL ID. Every one of the 43 tip anchors on
// this page wraps exactly one id'd control, and that id is unique across the
// document — which matters because this page has structurally identical Free
// and Scale-Locked glissando panels whose captions read Sync / Shape /
// Interval / Direction / Semitones in BOTH. document.querySelector returns the
// first match in document order, which is precisely how O-Octagon's
// .vunit-group tip nearly landed on the wrong control in Stage C.
export const I18N = Object.freeze({

    // ── The settings popover (v2.4.0) ───────────────────────────────────────
    // New controls, new copy. The hover-help toggle moves in here from the
    // floating "?" that sat in this exact corner: one place for the two things
    // that decide what the hover help says and whether it says it at all.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this interface and whether hover help appears. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette interface et l’affichage des infobulles. Les deux choix sont conservés avec la session.',
              reviewed: true },
        'zh-Hans': { t: '设置',
                     b: '选择此界面的语言，以及是否显示悬停帮助。两项选择都会随会话一起保存。',
                     reviewed: 'bt' },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language of this hover help and of the labels on the page. Value readouts, dropdown choices, tuning names and preset names stay in English.' },
        fr: { t: 'Langue',
              b: 'La langue de ces infobulles et des libellés de la page. Les valeurs affichées, les choix des menus, les noms de gammes et les noms de préréglages restent en anglais.',
              reviewed: true },
        'zh-Hans': { t: '语言',
                     b: '这些悬停帮助和页面标签所用的语言。数值读数、下拉选项、音阶名称和预设名称保持英文。',
                     reviewed: 'bt' },
    },
    'tips-toggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
        'zh-Hans': { t: '悬停帮助',
                     b: '开启或关闭这些悬停帮助。关闭后，只有齿轮按钮和此开关仍会说明自身。',
                     reviewed: 'bt' },
    },

    // ── SOUND tab — Main ────────────────────────────────────────────────────
    'brightness': {
        en: { t: 'Brightness',
              b: 'Controls the high-frequency content of the string tone. Higher values produce a brighter, more brilliant sound.' },
        fr: { t: 'Brillance',
              b: 'Règle le contenu aigu du timbre de la corde. Les valeurs élevées donnent un son plus clair et plus brillant.',
              reviewed: true },
        'zh-Hans': { t: '明亮度',
                     b: '控制弦音的高频成分。数值越高，声音越明亮、越辉煌。',
                     reviewed: 'bt' },
    },
    'timbre': {
        en: { t: 'Timbre',
              b: 'Adjusts the tonal warmth of the string. Lower values produce darker, warmer tones; higher values add brightness.' },
        fr: { t: 'Timbre',
              b: 'Ajuste la chaleur du timbre de la corde. Les valeurs basses donnent des sons plus sombres et plus chauds ; les valeurs hautes ajoutent de la brillance.',
              reviewed: true },
        'zh-Hans': { t: '音色',
                     b: '调节弦的音色温暖程度。数值越低，音色越暗、越温暖；数值越高则更明亮。',
                     reviewed: 'bt' },
    },
    'decayTime': {
        en: { t: 'Decay Time',
              b: 'How long the string resonates after being plucked. Longer decay creates sustained, ethereal notes.' },
        fr: { t: 'Temps de déclin',
              b: 'Durée pendant laquelle la corde résonne après avoir été pincée. Un déclin long donne des notes tenues et aériennes.',
              reviewed: true },
        'zh-Hans': { t: '衰减时间',
                     b: '弦被拨动后持续共鸣的时长。衰减越长，音符越绵延、越空灵。',
                     reviewed: 'bt' },
    },
    'attackNoise': {
        en: { t: 'Attack Noise',
              b: 'Amount of pluck transient noise. Higher values add scratchy, percussive attack character.' },
        fr: { t: 'Bruit d’attaque',
              b: 'Quantité de bruit transitoire au pincement. Les valeurs élevées ajoutent une attaque râpeuse et percussive.',
              reviewed: true },
        'zh-Hans': { t: '起音噪声',
                     b: '拨弦瞬态噪声的量。数值越高，起音越沙哑、越有敲击感。',
                     reviewed: 'bt' },
    },
    'humanize': {
        en: { t: 'Humanize',
              b: 'Adds natural per-note variation to pluck position, finger hardness, pitch, and attack noise. Creates realistic, organic performances where no two notes sound exactly alike.' },
        fr: { t: 'Humaniser',
              b: 'Ajoute une variation naturelle, note par note, du point de pincement, de la dureté du doigt, de la hauteur et du bruit d’attaque. Donne un jeu réaliste et organique où deux notes ne sonnent jamais exactement pareil.',
              reviewed: true },
        'zh-Hans': { t: '人性化',
                     b: '为拨弦位置、手指硬度、音高和起音噪声逐音添加自然变化。营造真实而有机的演奏，没有两个音符听起来完全相同。',
                     reviewed: 'bt' },
    },

    // ── SOUND tab — String ──────────────────────────────────────────────────
    'stringMaterial': {
        en: { t: 'Material',
              b: 'The string material affects tone, brightness, and sustain. Gut is warm and mellow; Wire is bright; Crystal has bell-like qualities.' },
        fr: { t: 'Matériau',
              b: 'Le matériau de la corde influe sur le timbre, la brillance et la tenue. Gut est chaud et doux ; Wire est brillant ; Crystal évoque la cloche.',
              reviewed: true },
        'zh-Hans': { t: '材质',
                     b: '弦的材质影响音色、明亮度和延音。Gut 温暖圆润；Wire 明亮；Crystal 带有钟鸣质感。',
                     reviewed: 'bt' },
    },
    'stringTension': {
        en: { t: 'Tension',
              b: 'String tightness affects resonance and brightness. Higher tension creates a brighter, more resonant tone.' },
        fr: { t: 'Tension',
              b: 'La tension de la corde influe sur la résonance et la brillance. Une tension élevée donne un timbre plus clair et plus résonant.',
              reviewed: true },
        'zh-Hans': { t: '张力',
                     b: '弦的松紧影响共振和明亮度。张力越高，音色越明亮、共振越强。',
                     reviewed: 'bt' },
    },
    'stringGauge': {
        en: { t: 'Gauge',
              b: 'String thickness affects damping. Thinner strings are brighter with quick decay; thicker strings are darker with more body.' },
        fr: { t: 'Calibre',
              b: 'L’épaisseur de la corde influe sur l’amortissement. Les cordes fines sont plus claires et s’éteignent vite ; les cordes épaisses sont plus sombres et plus corsées.',
              reviewed: true },
        'zh-Hans': { t: '弦径',
                     b: '弦的粗细影响阻尼。细弦更明亮、衰减更快；粗弦更暗、更厚实。',
                     reviewed: 'bt' },
    },
    'stringLength': {
        en: { t: 'Length',
              b: 'Virtual string length affects decay character. Shorter strings have punchy decay; longer strings sustain with diffuse tails.' },
        fr: { t: 'Longueur',
              b: 'La longueur virtuelle de la corde influe sur le caractère du déclin. Les cordes courtes s’éteignent sèchement ; les cordes longues tiennent avec des queues diffuses.',
              reviewed: true },
        'zh-Hans': { t: '长度',
                     b: '虚拟弦长影响衰减的特性。短弦衰减干脆；长弦延音更久，尾音更弥散。',
                     reviewed: 'bt' },
    },
    'stringStiffness': {
        en: { t: 'Stiffness',
              b: 'Controls harmonic stretch (inharmonicity). Higher values create bell-like, slightly detuned overtones.' },
        fr: { t: 'Raideur',
              b: 'Règle l’étirement harmonique (inharmonicité). Les valeurs élevées donnent des partiels légèrement désaccordés, proches de la cloche.',
              reviewed: true },
        'zh-Hans': { t: '劲度',
                     b: '控制泛音的展宽（非谐性）。数值越高，泛音越接近钟声，略微失谐。',
                     reviewed: 'bt' },
    },

    // ── SOUND tab — Body ────────────────────────────────────────────────────
    'bodySize': {
        en: { t: 'Size',
              b: 'The soundboard size affects bass response and overall fullness. Larger bodies produce deeper, more resonant tones.' },
        fr: { t: 'Taille',
              b: 'La taille de la table d’harmonie influe sur la réponse dans le grave et sur l’ampleur générale. Les grandes caisses donnent des sons plus profonds et plus résonants.',
              reviewed: true },
        'zh-Hans': { t: '尺寸',
                     b: '音板尺寸影响低频响应和整体的饱满度。共鸣体越大，音色越深沉、共振越强。',
                     reviewed: 'bt' },
    },
    'bodyResonance': {
        en: { t: 'Resonance',
              b: 'Body resonance amount. Higher values add more sympathetic body vibration to the string sound.' },
        fr: { t: 'Résonance',
              b: 'Quantité de résonance de caisse. Les valeurs élevées ajoutent au son de la corde davantage de vibration sympathique de la caisse.',
              reviewed: true },
        'zh-Hans': { t: '共振',
                     b: '共鸣体的共振量。数值越高，为弦音加入的共鸣体振动越多。',
                     reviewed: 'bt' },
    },
    'woodType': {
        en: { t: 'Wood Type',
              b: 'Soundboard wood affects tonal character. Spruce is balanced; Maple is bright; Exotic woods have unique timbres.' },
        fr: { t: 'Essence de bois',
              b: 'Le bois de la table influe sur le caractère du timbre. Spruce est équilibré ; Maple est brillant ; Exotic offre des timbres singuliers.',
              reviewed: true },
        'zh-Hans': { t: '木材种类',
                     b: '音板木材影响音色特性。Spruce 均衡；Maple 明亮；Exotic 木材音色独特。',
                     reviewed: 'bt' },
    },
    'bodyModeSpread': {
        en: { t: 'Mode Spread',
              b: 'Spreads body resonance modes across frequencies. Higher values create a more complex, diffuse resonance.' },
        fr: { t: 'Étalement des modes',
              b: 'Étale en fréquence les modes de résonance de la caisse. Les valeurs élevées donnent une résonance plus complexe et plus diffuse.',
              reviewed: true },
        'zh-Hans': { t: '模态扩散',
                     b: '将共鸣体的共振模态在频率上展开。数值越高，共振越复杂、越弥散。',
                     reviewed: 'bt' },
    },
    'bridgeBrightness': {
        en: { t: 'Bridge Brightness',
              b: 'Affects the transfer of high frequencies from string to body. Higher values add clarity and definition.' },
        fr: { t: 'Brillance du chevalet',
              b: 'Agit sur la transmission des aigus de la corde vers la caisse. Les valeurs élevées ajoutent de la clarté et de la définition.',
              reviewed: true },
        'zh-Hans': { t: '琴马明亮度',
                     b: '影响高频从弦传递到共鸣体的程度。数值越高，声音越清晰、越有轮廓。',
                     reviewed: 'bt' },
    },

    // ── SOUND tab — Excitation ──────────────────────────────────────────────
    'pluckPosition': {
        en: { t: 'Pluck Position',
              b: 'Where the string is plucked. Near the soundhole (0%) has more bass; near the bridge (100%) has more brilliance.' },
        fr: { t: 'Point de pincement',
              b: 'Endroit où la corde est pincée. Près de la rosace (0 %), plus de grave ; près du chevalet (100 %), plus de brillance.',
              reviewed: true },
        'zh-Hans': { t: '拨弦位置',
                     b: '拨动琴弦的位置。靠近音孔（0%）低频更多；靠近琴马（100%）更辉煌。',
                     reviewed: 'bt' },
    },
    'fingerHardness': {
        en: { t: 'Finger Hardness',
              b: 'How firm the plucking finger is. Soft fingertips (0%) sound mellow; hard fingernails (100%) sound sharp.' },
        fr: { t: 'Dureté du doigt',
              b: 'Fermeté du doigt qui pince. La pulpe souple (0 %) sonne douce ; l’ongle dur (100 %) sonne mordant.',
              reviewed: true },
        'zh-Hans': { t: '手指硬度',
                     b: '拨弦手指的硬实程度。柔软指腹（0%）音色圆润；坚硬指甲（100%）音色尖锐。',
                     reviewed: 'bt' },
    },
    'technique': {
        en: { t: 'Technique',
              b: 'Playing technique changes the excitation character. Harmonic produces bell-like tones; Muted creates percussive plucks.' },
        fr: { t: 'Technique',
              b: 'La technique de jeu change le caractère de l’excitation. Harmonic donne des sons de cloche ; Muted donne des pincements percussifs.',
              reviewed: true },
        'zh-Hans': { t: '技法',
                     b: '演奏技法改变激励的特性。Harmonic 产生钟鸣般的音色；Muted 产生敲击感的拨奏。',
                     reviewed: 'bt' },
    },

    // ── SOUND tab — Sympathetic ─────────────────────────────────────────────
    'sympatheticAmount': {
        en: { t: 'Amount',
              b: 'Sympathetic resonance intensity. How much unplayed strings vibrate in response to played notes. Creates richness.' },
        fr: { t: 'Quantité',
              b: 'Intensité de la résonance sympathique : à quel point les cordes non jouées vibrent en réponse aux notes jouées. Enrichit le son.',
              reviewed: true },
        'zh-Hans': { t: '量',
                     b: '共鸣弦共振的强度：未被弹奏的弦对已奏音符产生多大的共振。让声音更丰满。',
                     reviewed: 'bt' },
    },
    'sympatheticQ': {
        en: { t: 'Sharpness (Q)',
              b: 'The tuning sharpness of sympathetic filters. Higher Q values create more focused, ringing resonance.' },
        fr: { t: 'Finesse (Q)',
              b: 'Finesse d’accord des filtres sympathiques. Un Q élevé donne une résonance plus étroite et plus sonnante.',
              reviewed: true },
        'zh-Hans': { t: '锐度 (Q)',
                     b: '共鸣弦滤波器的调谐锐度。Q 值越高，共振越集中、余音越持久。',
                     reviewed: 'bt' },
    },

    // ── TECHNIQUES tab — Keyswitches ────────────────────────────────────────
    'freeKeyswitchNote': {
        en: { t: 'Free KS',
              b: 'MIDI note that activates Free glissando while held. Keyswitch notes are filtered from playback.' },
        fr: { t: 'Commande libre',
              b: 'Note MIDI qui active le glissando libre tant qu’elle est tenue. Les notes de commande sont filtrées et ne sonnent pas.',
              reviewed: true },
        'zh-Hans': { t: '自由键位',
                     b: '按住即可启动自由滑音的 MIDI 音符。键位音符会被滤除，不参与发声。',
                     reviewed: 'bt' },
    },
    'scaleKeyswitchNote': {
        en: { t: 'Scale KS',
              b: 'MIDI note that activates Scale-Locked glissando while held. Keyswitch notes are filtered from playback.' },
        fr: { t: 'Commande gamme',
              b: 'Note MIDI qui active le glissando sur gamme tant qu’elle est tenue. Les notes de commande sont filtrées et ne sonnent pas.',
              reviewed: true },
        'zh-Hans': { t: '音阶键位',
                     b: '按住即可启动音阶锁定滑音的 MIDI 音符。键位音符会被滤除，不参与发声。',
                     reviewed: 'bt' },
    },

    // ── TECHNIQUES tab — Free Glissando ─────────────────────────────────────
    // Sync / Shape / Interval / Direction / Semitones each appear TWICE on this
    // page, once per glissando panel, with DIFFERENT bodies. Keyed on the
    // control id rather than the caption for exactly that reason.
    'freeTempoSync': {
        en: { t: 'Sync',
              b: 'Lock the Free glissando sweep time to host tempo. When set to a rhythmic value, the Time slider is replaced by the synced duration.' },
        fr: { t: 'Sync',
              b: 'Cale la durée du balayage du glissando libre sur le tempo de l’hôte. Sur une valeur rythmique, le curseur Durée est remplacé par la durée synchronisée.',
              reviewed: true },
        'zh-Hans': { t: '同步',
                     b: '将自由滑音的扫描时长锁定到宿主的节奏速度。设为节奏值时，时间滑杆会被同步后的时长取代。',
                     reviewed: 'bt' },
    },
    'glissandoTime': {
        en: { t: 'Time',
              b: 'How long the Free mode pitch sweep takes. 10ms is a near-instant snap; 50ms (default) is a smooth glissando; 200-500ms creates slow expressive portamento.' },
        fr: { t: 'Durée',
              b: 'Durée du balayage de hauteur en mode libre. 10 ms donne un saut quasi instantané ; 50 ms (défaut) un glissando fluide ; 200-500 ms un portamento lent et expressif.',
              reviewed: true },
        'zh-Hans': { t: '时间',
                     b: '自由模式下音高扫描所需的时长。10 ms 几乎是瞬间跳变；50 ms（默认）是平滑的滑音；200-500 ms 形成缓慢而富于表情的连音滑奏。',
                     reviewed: 'bt' },
    },
    'freeShape': {
        en: { t: 'Shape',
              b: 'Acceleration curve for the free glissando sweep.' },
        fr: { t: 'Forme',
              b: 'Courbe d’accélération du balayage du glissando libre.',
              reviewed: true },
        'zh-Hans': { t: '形状',
                     b: '自由滑音扫描的加速曲线。',
                     reviewed: 'bt' },
    },
    'freeInterval': {
        en: { t: 'Interval',
              b: 'How far the free glissando sweeps in semitones.' },
        fr: { t: 'Intervalle',
              b: 'Amplitude du balayage du glissando libre, en demi-tons.',
              reviewed: true },
        'zh-Hans': { t: '音程',
                     b: '自由滑音扫描的跨度，以半音计。',
                     reviewed: 'bt' },
    },
    'freeDirection': {
        en: { t: 'Direction',
              b: 'Whether the free glissando sweeps up to the played note or down to it.' },
        fr: { t: 'Sens',
              b: 'Indique si le glissando libre monte vers la note jouée ou descend vers elle.',
              reviewed: true },
        'zh-Hans': { t: '方向',
                     b: '自由滑音是向上扫向所奏音符，还是向下扫向它。',
                     reviewed: 'bt' },
    },
    'freeCustomSemitones': {
        en: { t: 'Custom Semitones',
              b: 'Exact semitone count (1-48) for the free glissando sweep distance.' },
        fr: { t: 'Demi-tons libres',
              b: 'Nombre exact de demi-tons (1-48) pour l’amplitude du balayage du glissando libre.',
              reviewed: true },
        'zh-Hans': { t: '自定义半音',
                     b: '自由滑音扫描距离的精确半音数（1-48）。',
                     reviewed: 'bt' },
    },

    // ── TECHNIQUES tab — Scale-Locked Glissando ─────────────────────────────
    'glissandoScale': {
        en: { t: 'Scale',
              b: 'The scale used for scale-locked glissandos. Major/Minor/Pentatonic are 12-note patterns; Custom uses toggle buttons to select individual degrees. Major/Minor/Pentatonic are disabled for non-12-note tunings.' },
        fr: { t: 'Gamme',
              b: 'La gamme utilisée par le glissando sur gamme. Major/Minor/Pentatonic sont des motifs de 12 notes ; Custom permet de choisir chaque degré par des boutons. Major/Minor/Pentatonic sont désactivés pour les gammes qui ne comptent pas 12 notes.',
              reviewed: true },
        'zh-Hans': { t: '音阶',
                     b: '音阶锁定滑音所用的音阶。Major/Minor/Pentatonic 是 12 音的模式；Custom 通过开关按钮逐个选择音级。对于不是 12 音的音律，Major/Minor/Pentatonic 不可用。',
                     reviewed: 'bt' },
    },
    'glissandoTonic': {
        en: { t: 'Tonic',
              b: 'Root note for the glissando scale. Independent from the tuning tab tonic. Determines which notes belong to the selected scale pattern.' },
        fr: { t: 'Tonique',
              b: 'Note fondamentale de la gamme du glissando. Indépendante de la tonique de l’onglet Accord. Détermine quelles notes appartiennent au motif choisi.',
              reviewed: true },
        'zh-Hans': { t: '主音',
                     b: '滑音音阶的根音。与调音选项卡的主音相互独立。决定哪些音符属于所选的音阶模式。',
                     reviewed: 'bt' },
    },
    'scaleTempoSync': {
        en: { t: 'Sync',
              b: 'Lock the Scale-Locked glissando total duration to host tempo. When set to a rhythmic value, the Speed slider is replaced by tempo-synced timing.' },
        fr: { t: 'Sync',
              b: 'Cale la durée totale du glissando sur gamme sur le tempo de l’hôte. Sur une valeur rythmique, le curseur Vitesse est remplacé par la durée synchronisée.',
              reviewed: true },
        'zh-Hans': { t: '同步',
                     b: '将音阶锁定滑音的总时长锁定到宿主的节奏速度。设为节奏值时，速度滑杆会被同步后的计时取代。',
                     reviewed: 'bt' },
    },
    'glissandoSpeed': {
        en: { t: 'Speed',
              b: 'How fast the glissando sweeps through scale notes. 8 n/s is slow and expressive; 21 n/s is a fast Hollywood sweep.' },
        fr: { t: 'Vitesse',
              b: 'Rapidité du balayage du glissando à travers les notes de la gamme. 8 n/s est lent et expressif ; 21 n/s donne un balayage hollywoodien rapide.',
              reviewed: true },
        'zh-Hans': { t: '速度',
                     b: '滑音扫过音阶音符的快慢。8 n/s 缓慢而富于表情；21 n/s 是好莱坞式的快速扫弦。',
                     reviewed: 'bt' },
    },
    'glissandoShape': {
        en: { t: 'Shape',
              b: 'Acceleration curve for the scale-locked glissando sweep.' },
        fr: { t: 'Forme',
              b: 'Courbe d’accélération du balayage du glissando sur gamme.',
              reviewed: true },
        'zh-Hans': { t: '形状',
                     b: '音阶锁定滑音扫描的加速曲线。',
                     reviewed: 'bt' },
    },
    'glissandoInterval': {
        en: { t: 'Interval',
              b: 'How far the scale-locked glissando sweeps in semitones.' },
        fr: { t: 'Intervalle',
              b: 'Amplitude du balayage du glissando sur gamme, en demi-tons.',
              reviewed: true },
        'zh-Hans': { t: '音程',
                     b: '音阶锁定滑音扫描的跨度，以半音计。',
                     reviewed: 'bt' },
    },
    'glissandoDirection': {
        en: { t: 'Direction',
              b: 'Whether the scale-locked glissando sweeps up to the played note or down to it.' },
        fr: { t: 'Sens',
              b: 'Indique si le glissando sur gamme monte vers la note jouée ou descend vers elle.',
              reviewed: true },
        'zh-Hans': { t: '方向',
                     b: '音阶锁定滑音是向上扫向所奏音符，还是向下扫向它。',
                     reviewed: 'bt' },
    },
    'glissandoCustomSemitones': {
        en: { t: 'Custom Semitones',
              b: 'Exact semitone count (1-48) for the scale-locked glissando sweep distance.' },
        fr: { t: 'Demi-tons libres',
              b: 'Nombre exact de demi-tons (1-48) pour l’amplitude du balayage du glissando sur gamme.',
              reviewed: true },
        'zh-Hans': { t: '自定义半音',
                     b: '音阶锁定滑音扫描距离的精确半音数（1-48）。',
                     reviewed: 'bt' },
    },
    'glissandoExcitation': {
        en: { t: 'Gliss Softness',
              b: 'How much the excitation changes during glissando. 0% is a full deliberate pluck; 60% (default) is a realistic brush; 100% is a very light ethereal sweep.' },
        fr: { t: 'Douceur du glissando',
              b: 'Ampleur du changement d’excitation pendant le glissando. 0 % donne un pincement franc et appuyé ; 60 % (défaut) un effleurement réaliste ; 100 % un balayage très léger et aérien.',
              reviewed: true },
        'zh-Hans': { t: '滑音柔和度',
                     b: '滑音过程中激励变化的幅度。0% 是完整而有意的拨奏；60%（默认）是真实的扫拂；100% 是极轻盈空灵的扫弦。',
                     reviewed: 'bt' },
    },
    'glissandoHumanize': {
        en: { t: 'Humanize',
              b: 'Adds natural timing jitter to each glissando step. Simulates the slight irregularity of a real harpist\'s arm sweep across strings.' },
        fr: { t: 'Humaniser',
              b: 'Ajoute une irrégularité naturelle au placement de chaque pas du glissando. Simule la légère imprécision du bras d’un harpiste balayant les cordes.',
              reviewed: true },
        'zh-Hans': { t: '人性化',
                     b: '为滑音的每一步添加自然的时值抖动。模拟真实竖琴演奏者手臂扫过琴弦时的轻微不规则。',
                     reviewed: 'bt' },
    },
    'glissandoVelStart': {
        en: { t: 'Dynamics Start',
              b: 'Velocity at the beginning of the glissando sweep. Lower values mean softer attack and more damping. Default 50% gives a subtle lead-in.' },
        fr: { t: 'Dynamique au départ',
              b: 'Vélocité au début du balayage du glissando. Les valeurs basses donnent une attaque plus douce et plus d’amortissement. Le défaut de 50 % donne une entrée discrète.',
              reviewed: true },
        'zh-Hans': { t: '起始力度',
                     b: '滑音扫描起点的力度。数值越低，起音越柔和、阻尼越多。默认的 50% 给出含蓄的引入。',
                     reviewed: 'bt' },
    },
    'glissandoVelEnd': {
        en: { t: 'Dynamics End',
              b: 'Velocity at the end of the glissando sweep. Higher values mean brighter sustain. Default 70% gives a natural ascending crescendo.' },
        fr: { t: 'Dynamique à la fin',
              b: 'Vélocité à la fin du balayage du glissando. Les valeurs élevées donnent une tenue plus brillante. Le défaut de 70 % donne un crescendo ascendant naturel.',
              reviewed: true },
        'zh-Hans': { t: '结束力度',
                     b: '滑音扫描终点的力度。数值越高，延音越明亮。默认的 70% 给出自然的渐强上行。',
                     reviewed: 'bt' },
    },

    // ── TUNING tab ──────────────────────────────────────────────────────────
    'ref-pitch-knob': {
        en: { t: 'A4 Reference',
              b: 'The concert pitch for A4. Standard is 440 Hz. Historical tunings may use 415 Hz (Baroque) or 432 Hz.' },
        fr: { t: 'Référence A4',
              b: 'Le diapason pour le A4. La norme est 440 Hz. Les gammes historiques utilisent parfois 415 Hz (baroque) ou 432 Hz.',
              reviewed: true },
        'zh-Hans': { t: 'A4 基准音高',
                     b: 'A4 的音准基准。标准为 440 Hz。历史音律有时使用 415 Hz（巴洛克）或 432 Hz。',
                     reviewed: 'bt' },
    },
    'octave-stretch': {
        en: { t: 'Octave Stretch',
              b: 'Stretches octaves slightly. Values >1.0 widen octaves (piano-like). Used for compensating string inharmonicity.' },
        fr: { t: 'Étirement d’octave',
              b: 'Étire légèrement les octaves. Les valeurs supérieures à 1,0 élargissent les octaves (comme au piano). Sert à compenser l’inharmonicité des cordes.',
              reviewed: true },
        'zh-Hans': { t: '八度延展',
                     b: '略微拉伸八度。大于 1.0 的数值会加宽八度（类似钢琴）。用于补偿弦的非谐性。',
                     reviewed: 'bt' },
    },

    // ── Footer ──────────────────────────────────────────────────────────────
    'masterVolume': {
        en: { t: 'Master Volume',
              b: 'The final output level of the instrument. Shown in dB. 0 dB is unity gain (no change).' },
        fr: { t: 'Volume général',
              b: 'Le niveau de sortie final de l’instrument. Affiché en dB. 0 dB correspond au gain unitaire (aucun changement).',
              reviewed: true },
        'zh-Hans': { t: '主音量',
                     b: '乐器最终的输出电平。以 dB 显示。0 dB 为单位增益（不作改变）。',
                     reviewed: 'bt' },
    },
});

// ============================================================================
// LABELS — the on-page text (v2.4.3, canon v2)
// ============================================================================
//
// I18N above is HOVER-HELP copy: a title and a body rendered into a wrapping
// 200 px tooltip. LABELS is ON-PAGE copy: one string dropped into a cell that
// mostly does not wrap. They are different problems and this table keeps them
// apart on purpose.
//
// ── THE REUSE RULE IS DELIBERATELY NOT USED HERE ────────────────────────────
// trLabel() falls back to I18N when a key is absent from this table, and
// O-FreqPulse reuses five keys that way. O-Lyrica does not reuse ANY.
//
// The reason is the frame. 700 x 450 is the tightest text-to-area ratio in the
// repo, and .slider-group / .dropdown-group are `flex: 1` between a 100 px
// min-width and a 140 px max-width — a basis of ZERO, so the CELL never grows
// with its caption and a caption that outruns ~120 px WRAPS, raising the whole
// flex line and pushing every row below it down. A tooltip title is allowed to
// grow into a phrase; a caption in that cell is not. Reusing would make the
// next tooltip copy edit a silent layout change, so every caption below is
// sized independently of the tip that explains it. Ten pairs differ in English
// already ("Bridge Bright" vs "Bridge Brightness", "Softness" vs "Gliss
// Softness", "Key" vs "Tonic", "Master" vs "Master Volume", …), which is the
// same judgement the previous author made by hand.
//
// ── ENGLISH WAS MOVED, NOT RE-TYPED ────────────────────────────────────────
// Every en below is what index.html or js/app.js carried through v2.3.3, taken
// from scripts/i18n-extract.js's inventory rather than transcribed. The ONE
// exception is named in the CHANGELOG: `technique` option 4 read "Pres de la
// table" in the markup while createParameterLayout spells it "Près de la
// table", and the page now matches the automation lane.
//
// ── FRENCH IS SIZED, NOT SHRUNK ────────────────────────────────────────────
// D-04 forbids an auto-shrink font and a short-variant fallback: exactly ONE
// French string per key, and nothing chooses between variants at runtime.
// Where French could not fit a cell, the CELL was changed — see CHANGELOG
// v2.4.0 for the four geometry rules and what each one was measured against.
//
// ALL FRENCH IS MACHINE-DRAFTED, `reviewed: false`. No native speaker has read
// it. `node scripts/check-i18n.js` prints the worklist, LABELS included.
// ============================================================================

export const LABELS = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    // "Voices:" was a bare text node sharing #voiceCount's parent with two
    // element children. Keying the parent would have made applyLabel's
    // textContent write DELETE those children — the systemic form of
    // pattern_js_state_updater_overwrites_html_labels — so the text moved into
    // its own span first.
    'label.voices':     { en: { t: 'Voices:' },  fr: { t: 'Voix :',   reviewed: true },
                        'zh-Hans': { t: '声部：', reviewed: 'bt' } },
    'label.save':       { en: { t: 'Save' },     fr: { t: 'Enreg.',   reviewed: true },
                        'zh-Hans': { t: '保存', reviewed: 'bt' } },
    'label.load':       { en: { t: 'Load' },     fr: { t: 'Ouvrir',   reviewed: true },
                        'zh-Hans': { t: '载入', reviewed: 'bt' } },

    // ── Tab bar ─────────────────────────────────────────────────────────────
    // .tab is `flex: 1` across the full 700 px, so a caption cannot move its
    // neighbours here; these are sized for legibility, not for width.
    'label.tabSound':      { en: { t: 'SOUND' },      fr: { t: 'SON',        reviewed: true },
                        'zh-Hans': { t: '声音', reviewed: 'bt' } },
    'label.tabTechniques': { en: { t: 'TECHNIQUES' }, fr: { t: 'TECHNIQUES', reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '演奏技法', reviewed: 'bt' } },
    'label.tabTuning':     { en: { t: 'TUNING' },     fr: { t: 'ACCORD',     reviewed: true },
                        'zh-Hans': { t: '调音', reviewed: 'bt' } },
    'label.tabEffects':    { en: { t: 'EFFECTS' },    fr: { t: 'EFFETS',     reviewed: true },
                        'zh-Hans': { t: '效果', reviewed: 'bt' } },

    // ── SOUND tab — section headers ─────────────────────────────────────────
    'label.secMain':        { en: { t: 'Main' },        fr: { t: 'Principal',   reviewed: true },
                        'zh-Hans': { t: '主', reviewed: 'bt' } },
    'label.secString':      { en: { t: 'String' },      fr: { t: 'Corde',       reviewed: true },
                        'zh-Hans': { t: '弦', reviewed: 'bt' } },
    'label.secBody':        { en: { t: 'Body' },        fr: { t: 'Caisse',      reviewed: true },
                        'zh-Hans': { t: '共鸣体', reviewed: 'bt' } },
    'label.secExcitation':  { en: { t: 'Excitation' },  fr: { t: 'Excitation',  reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '激励', reviewed: 'bt' } },
    'label.secSympathetic': { en: { t: 'Sympathetic' }, fr: { t: 'Sympathiques', reviewed: true },
                        'zh-Hans': { t: '共鸣弦', reviewed: 'bt' } },

    // ── SOUND tab — captions ────────────────────────────────────────────────
    // Every one of these sits in a 100-140 px `flex: 1 1 0` cell at 9 px
    // uppercase with 0.5 px letter-spacing. THE PINNED WIDTHS WERE MEASURED AS
    // RENDERED, not from getComputedStyle().font — text-transform and
    // letter-spacing are not in the font shorthand, and a probe that ignores
    // them reads a French caption narrower than it paints.
    'label.brightness':     { en: { t: 'Brightness' },      fr: { t: 'Brillance',       reviewed: true },
                        'zh-Hans': { t: '明亮度', reviewed: 'bt' } },
    'label.timbre':         { en: { t: 'Timbre' },          fr: { t: 'Timbre',          reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '音色', reviewed: 'bt' } },
    'label.decayTime':      { en: { t: 'Decay Time' },      fr: { t: 'Déclin',          reviewed: true },
                        'zh-Hans': { t: '衰减时间', reviewed: 'bt' } },
    'label.attackNoise':    { en: { t: 'Attack Noise' },    fr: { t: 'Bruit d’attaque', reviewed: true },
                        'zh-Hans': { t: '起音噪声', reviewed: 'bt' } },
    'label.humanize':       { en: { t: 'Humanize' },        fr: { t: 'Humaniser',       reviewed: true },
                        'zh-Hans': { t: '人性化', reviewed: 'bt' } },
    'label.material':       { en: { t: 'Material' },        fr: { t: 'Matériau',        reviewed: true },
                        'zh-Hans': { t: '材质', reviewed: 'bt' } },
    'label.tension':        { en: { t: 'Tension' },         fr: { t: 'Tension',         reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '张力', reviewed: 'bt' } },
    'label.gauge':          { en: { t: 'Gauge' },           fr: { t: 'Calibre',         reviewed: true },
                        'zh-Hans': { t: '弦径', reviewed: 'bt' } },
    'label.length':         { en: { t: 'Length' },          fr: { t: 'Longueur',        reviewed: true },
                        'zh-Hans': { t: '长度', reviewed: 'bt' } },
    'label.stiffness':      { en: { t: 'Stiffness' },       fr: { t: 'Raideur',         reviewed: true },
                        'zh-Hans': { t: '劲度', reviewed: 'bt' } },
    'label.size':           { en: { t: 'Size' },            fr: { t: 'Taille',          reviewed: true },
                        'zh-Hans': { t: '尺寸', reviewed: 'bt' } },
    'label.resonance':      { en: { t: 'Resonance' },       fr: { t: 'Résonance',       reviewed: true },
                        'zh-Hans': { t: '共振', reviewed: 'bt' } },
    'label.woodType':       { en: { t: 'Wood Type' },       fr: { t: 'Bois',            reviewed: true },
                        'zh-Hans': { t: '木材种类', reviewed: 'bt' } },
    'label.modeSpread':     { en: { t: 'Mode Spread' },     fr: { t: 'Étalement',       reviewed: true },
                        'zh-Hans': { t: '模态扩散', reviewed: 'bt' } },
    'label.bridgeBright':   { en: { t: 'Bridge Bright' },   fr: { t: 'Brill. chevalet', reviewed: true },
                        'zh-Hans': { t: '琴马亮度', reviewed: 'bt' } },
    'label.pluckPosition':  { en: { t: 'Pluck Position' },  fr: { t: 'Pincement',       reviewed: true },
                        'zh-Hans': { t: '拨弦位置', reviewed: 'bt' } },
    'label.fingerHardness': { en: { t: 'Finger Hardness' }, fr: { t: 'Dureté du doigt', reviewed: true },
                        'zh-Hans': { t: '手指硬度', reviewed: 'bt' } },
    'label.technique':      { en: { t: 'Technique' },       fr: { t: 'Technique',       reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '技法', reviewed: 'bt' } },
    'label.amount':         { en: { t: 'Amount' },          fr: { t: 'Quantité',        reviewed: true },
                        'zh-Hans': { t: '量', reviewed: 'bt' } },
    'label.sharpnessQ':     { en: { t: 'Sharpness (Q)' },   fr: { t: 'Finesse (Q)',     reviewed: true },
                        'zh-Hans': { t: '锐度 (Q)', reviewed: 'bt' } },

    // ── TECHNIQUES tab ──────────────────────────────────────────────────────
    // The two glissando section headings each share their row with a toggle
    // button, so the caption moved into its own span rather than sitting as a
    // bare text node beside an element child.
    'label.secKeyswitch':   { en: { t: 'Keyswitch Settings' },      fr: { t: 'Réglages des notes de commande', reviewed: true },
                        'zh-Hans': { t: '键位设置', reviewed: 'bt' } },
    'label.freeKS':         { en: { t: 'Free KS' },                 fr: { t: 'Cmde libre',           reviewed: true },
                        'zh-Hans': { t: '自由键位', reviewed: 'bt' } },
    'label.scaleKS':        { en: { t: 'Scale KS' },                fr: { t: 'Cmde gamme',           reviewed: true },
                        'zh-Hans': { t: '音阶键位', reviewed: 'bt' } },
    'label.secFreeGliss':   { en: { t: 'Free Glissando' },          fr: { t: 'Glissando libre',      reviewed: true },
                        'zh-Hans': { t: '自由滑音', reviewed: 'bt' } },
    'label.secScaleGliss':  { en: { t: 'Scale-Locked Glissando' },  fr: { t: 'Glissando sur gamme',  reviewed: true },
                        'zh-Hans': { t: '音阶锁定滑音', reviewed: 'bt' } },
    'label.sync':           { en: { t: 'Sync' },                    fr: { t: 'Sync',                 reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '同步', reviewed: 'bt' } },
    'label.time':           { en: { t: 'Time' },                    fr: { t: 'Durée',                reviewed: true },
                        'zh-Hans': { t: '时间', reviewed: 'bt' } },
    'label.shape':          { en: { t: 'Shape' },                   fr: { t: 'Forme',                reviewed: true },
                        'zh-Hans': { t: '形状', reviewed: 'bt' } },
    'label.interval':       { en: { t: 'Interval' },                fr: { t: 'Intervalle',           reviewed: true },
                        'zh-Hans': { t: '音程', reviewed: 'bt' } },
    'label.direction':      { en: { t: 'Direction' },               fr: { t: 'Sens',                 reviewed: true },
                        'zh-Hans': { t: '方向', reviewed: 'bt' } },
    'label.semitones':      { en: { t: 'Semitones' },               fr: { t: 'Demi-tons',            reviewed: true },
                        'zh-Hans': { t: '半音数', reviewed: 'bt' } },
    'label.scale':          { en: { t: 'Scale' },                   fr: { t: 'Gamme',                reviewed: true },
                        'zh-Hans': { t: '音阶', reviewed: 'bt' } },
    'label.key':            { en: { t: 'Key' },                     fr: { t: 'Tonique',              reviewed: true },
                        'zh-Hans': { t: '主音', reviewed: 'bt' } },
    'label.degrees':        { en: { t: 'Degrees' },                 fr: { t: 'Degrés',               reviewed: true },
                        'zh-Hans': { t: '音级', reviewed: 'bt' } },
    'label.speed':          { en: { t: 'Speed' },                   fr: { t: 'Vitesse',              reviewed: true },
                        'zh-Hans': { t: '速度', reviewed: 'bt' } },
    'label.softness':       { en: { t: 'Softness' },                fr: { t: 'Douceur',              reviewed: true },
                        'zh-Hans': { t: '柔和度', reviewed: 'bt' } },
    'label.dynStart':       { en: { t: 'Dynamics: Start' },         fr: { t: 'Dynamique : début',    reviewed: true },
                        'zh-Hans': { t: '力度：起始', reviewed: 'bt' } },
    'label.dynEnd':         { en: { t: 'Dynamics: End' },           fr: { t: 'Dynamique : fin',      reviewed: true },
                        'zh-Hans': { t: '力度：结束', reviewed: 'bt' } },

    // ── TUNING tab ──────────────────────────────────────────────────────────
    // A PARAMETERISED entry, written by __setLabel from updateIntervalListUI
    // with vars = { n: the scale size }. Contract §6 says copy that needs a count
    // is authored AROUND the inflection rather than through a plural engine:
    // there is no ternary and no suffix rule here, because the reachable range
    // of `n` is 3 upward (the rank-2 generator's own minimum) and both
    // languages are plural at every value it can take. The count is a var, not
    // a template literal — a table evaluated once at module load would freeze
    // whatever the language happened to be then.
    'label.intervalsHeader': { en: { t: 'Intervals ({n} notes)' },
                               fr: { t: 'Intervalles ({n} notes)', reviewed: true },
                        'zh-Hans': { t: '音程（{n} 个音）', reviewed: 'bt' } },
    'label.vizCircle':      { en: { t: 'Circle' },          fr: { t: 'Cercle',        reviewed: true },
                        'zh-Hans': { t: '圆周', reviewed: 'bt' } },
    'label.vizPolar':       { en: { t: 'Polar' },           fr: { t: 'Polaire',       reviewed: true },
                        'zh-Hans': { t: '极坐标', reviewed: 'bt' } },
    'label.vizMatrix':      { en: { t: 'Matrix' },          fr: { t: 'Matrice',       reviewed: true },
                        'zh-Hans': { t: '矩阵', reviewed: 'bt' } },
    'label.vizTrueKeys':    { en: { t: 'True Keys' },       fr: { t: 'Touches',       reviewed: true },
                        'zh-Hans': { t: '真实键位', reviewed: 'bt' } },
    'label.vizRotation':    { en: { t: 'Rotation' },        fr: { t: 'Rotation',      reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '旋转', reviewed: 'bt' } },
    'label.scaleIntervals': { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: true },
                        'zh-Hans': { t: '音阶音程', reviewed: 'bt' } },
    'label.tkHint':         { en: { t: 'Hold 2+ notes to see intervals' },
                              fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: true },
                        'zh-Hans': { t: '按住 2 个以上音符可查看音程', reviewed: 'bt' } },
    'label.tuningLibrary':  { en: { t: 'Tuning Library' },  fr: { t: 'Bibliothèque de gammes', reviewed: true },
                        'zh-Hans': { t: '调音库', reviewed: 'bt' } },
    // "A4" is a pitch identifier and stays; only the abbreviation "REF" is
    // translated. The cell is a fixed 60 px .ref-knob-label.
    'label.a4Ref':          { en: { t: 'A4 REF' },          fr: { t: 'RÉF. A4',       reviewed: true },
                        'zh-Hans': { t: 'A4 基准', reviewed: 'bt' } },
    'label.stretch':        { en: { t: 'Stretch' },         fr: { t: 'Étirement',     reviewed: true },
                        'zh-Hans': { t: '延展', reviewed: 'bt' } },
    // The four file buttons keep their EXTENSIONS, which are file-format
    // identifiers, and translate only the verb.
    'label.loadScl':        { en: { t: 'Load .SCL' },       fr: { t: 'Ouvrir .SCL',   reviewed: true },
                        'zh-Hans': { t: '载入 .scl', reviewed: 'bt' } },
    'label.loadKbm':        { en: { t: 'Load .KBM' },       fr: { t: 'Ouvrir .KBM',   reviewed: true },
                        'zh-Hans': { t: '载入 .kbm', reviewed: 'bt' } },
    'label.saveScl':        { en: { t: 'Save .SCL' },       fr: { t: 'Enreg. .SCL',   reviewed: true },
                        'zh-Hans': { t: '保存 .scl', reviewed: 'bt' } },
    'label.saveKbm':        { en: { t: 'Save .KBM' },       fr: { t: 'Enreg. .KBM',   reviewed: true },
                        'zh-Hans': { t: '保存 .kbm', reviewed: 'bt' } },
    'label.exportHtml':     { en: { t: 'Export HTML' },     fr: { t: 'Exporter HTML', reviewed: true },
                        'zh-Hans': { t: '导出 HTML', reviewed: 'bt' } },
    'label.generateScale':  { en: { t: 'Generate Scale' },  fr: { t: 'Générer une gamme', reviewed: true },
                        'zh-Hans': { t: '生成音阶', reviewed: 'bt' } },
    // The generator-type select is a PLAIN select over the string values
    // edo / harmonic / rank2 — it is not an AudioParameterChoice, no host ever
    // shows these three strings, and translating them cannot make the page and
    // an automation lane disagree. That is the discriminator; the thirteen
    // dropdowns that ARE choice parameters are exempt below.
    'label.genEdo':         { en: { t: 'EDO (Equal Division)' },  fr: { t: 'EDO (division égale)',    reviewed: true },
                        'zh-Hans': { t: '等分八度 (EDO)', reviewed: 'bt' } },
    'label.genHarmonic':    { en: { t: 'Harmonic Series' },       fr: { t: 'Série harmonique',        reviewed: true },
                        'zh-Hans': { t: '泛音列', reviewed: 'bt' } },
    'label.genRank2':       { en: { t: 'Rank-2 Temperament' },    fr: { t: 'Tempérament de rang 2',   reviewed: true },
                        'zh-Hans': { t: '二阶音律', reviewed: 'bt' } },
    'label.genDivisions':   { en: { t: 'Divisions' },      fr: { t: 'Divisions',       reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '分割', reviewed: 'bt' } },
    'label.genPeriod':      { en: { t: 'Period (¢)' },     fr: { t: 'Période (¢)',     reviewed: true },
                        'zh-Hans': { t: '周期（¢）', reviewed: 'bt' } },
    'label.genStart':       { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: true },
                        'zh-Hans': { t: '起始泛音', reviewed: 'bt' } },
    'label.genEnd':         { en: { t: 'End Harmonic' },   fr: { t: 'Harmonique de fin',    reviewed: true },
                        'zh-Hans': { t: '终止泛音', reviewed: 'bt' } },
    'label.genGenerator':   { en: { t: 'Generator (¢)' },  fr: { t: 'Générateur (¢)',  reviewed: true },
                        'zh-Hans': { t: '生成元（¢）', reviewed: 'bt',
                                     termNote: 'the rank-2 generator, and the rendering is qualified away from the obvious one. 生成音程 was authored first and the blind reverse pass returned "Generate Interval (¢)" — a VERB PHRASE, because this caption sits directly above the 生成 (Generate) button in the same collapsed panel and a reader meets the two together as an action and its target. 生成元 is the settled term for a generator in tuning theory and reads only as a noun. Nothing mechanical could see this: neither string is a TERMS key, Z5 is silent on both, and the downstream check looks for two keys mapping to ONE string while these map to two.' } },
    'label.genCount':       { en: { t: 'Notes' },          fr: { t: 'Notes',           reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '音符', reviewed: 'bt' } },
    'label.generate':       { en: { t: 'Generate' },       fr: { t: 'Générer',         reviewed: true },
                        'zh-Hans': { t: '生成', reviewed: 'bt' } },
    'label.tonicPrefix':    { en: { t: 'Tonic:' },         fr: { t: 'Tonique :',       reviewed: true },
                        'zh-Hans': { t: '主音：', reviewed: 'bt' } },
    // The rotation matrix's first column header and the True Keys summary row.
    // Both are built inside a `html +=` accumulation, which extractJsRows does
    // NOT scan — it reads assignments to textContent / innerText / innerHTML.
    // They are keyed anyway, by a __setLabel call on the injected node, and the
    // blind spot is named in the CHANGELOG rather than left to be discovered.
    'label.mode':           { en: { t: 'Mode' },           fr: { t: 'Mode',            reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '模式', reviewed: 'bt' } },
    'label.totalSpan':      { en: { t: 'Total span' },     fr: { t: 'Écart total',     reviewed: true },
                        'zh-Hans': { t: '总跨度', reviewed: 'bt' } },

    // ── EFFECTS tab ─────────────────────────────────────────────────────────
    'label.fxChorus':       { en: { t: 'Chorus' },  fr: { t: 'Chorus',  reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '合唱', reviewed: 'bt' } },
    'label.fxDelay':        { en: { t: 'Delay' },   fr: { t: 'Délai',   reviewed: true },
                        'zh-Hans': { t: '延迟', reviewed: 'bt' } },
    'label.fxReverb':       { en: { t: 'Reverb' },  fr: { t: 'Réverb',  reviewed: true },
                        'zh-Hans': { t: '混响', reviewed: 'bt' } },
    'label.fxEq':           { en: { t: 'EQ' },      fr: { t: 'EQ',      reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '均衡', reviewed: 'bt' } },
    // The sixteen knob captions, written by makeFxKnob. Through v2.3.3 that
    // function built the whole knob with innerHTML and the caption was a raw
    // interpolated string; it is createElement + setLabel now, and the caption
    // list carries KEYS rather than English.
    'label.knRate':         { en: { t: 'Rate' },     fr: { t: 'Vitesse',     reviewed: true },
                        'zh-Hans': { t: '速率', reviewed: 'bt' } },
    'label.knDepth':        { en: { t: 'Depth' },    fr: { t: 'Profondeur',  reviewed: true },
                        'zh-Hans': { t: '深度', reviewed: 'bt' } },
    'label.knMix':          { en: { t: 'Mix' },      fr: { t: 'Mix',         reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '混合', reviewed: 'bt' } },
    'label.knFeedback':     { en: { t: 'Feedback' }, fr: { t: 'Réinjection', reviewed: true },
                        'zh-Hans': { t: '反馈', reviewed: 'bt' } },
    'label.knLow':          { en: { t: 'Low' },      fr: { t: 'Grave',       reviewed: true },
                        'zh-Hans': { t: '低', reviewed: 'bt' } },
    'label.knMid':          { en: { t: 'Mid' },      fr: { t: 'Médium',      reviewed: true },
                        'zh-Hans': { t: '中', reviewed: 'bt' } },
    // Abbreviated to fit the 70px knob cell: the full "Fréq. médium" renders
    // 75px, which is the ONLY French caption on this page that would have set
    // the cell width by itself. English abbreviates the same caption for the
    // same reason ("Mid Freq", not "Mid Frequency"), and D-04 is satisfied
    // because there is exactly ONE French string here, not a pair chosen at
    // runtime.
    'label.knMidFreq':      { en: { t: 'Mid Freq' }, fr: { t: 'Fréq. méd.', reviewed: true },
                        'zh-Hans': { t: '中频频率', reviewed: 'bt' } },
    'label.knHigh':         { en: { t: 'High' },     fr: { t: 'Aigu',        reviewed: true },
                        'zh-Hans': { t: '高', reviewed: 'bt' } },
    'label.knDamp':         { en: { t: 'Damp' },     fr: { t: 'Amort.',      reviewed: true },
                        'zh-Hans': { t: '阻尼', reviewed: 'bt' } },
    'label.knPredelay':     { en: { t: 'Pre-dly' },  fr: { t: 'Pré-délai',   reviewed: true },
                        'zh-Hans': { t: '预延迟', reviewed: 'bt' } },
    'label.knMod':          { en: { t: 'Mod' },      fr: { t: 'Mod',         reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '调制', reviewed: 'bt' } },
    'label.knShimmer':      { en: { t: 'Shimmer' },  fr: { t: 'Shimmer',     reviewed: true, sameAsEn: true },
                        'zh-Hans': { t: '微光', reviewed: 'bt' } },

    // ── Footer ──────────────────────────────────────────────────────────────
    'label.master':         { en: { t: 'Master' },        fr: { t: 'Général',          reviewed: true },
                        'zh-Hans': { t: '总控', reviewed: 'bt' } },
    'label.clickToPlay':    { en: { t: 'Click to play' }, fr: { t: 'Cliquer pour jouer', reviewed: true },
                        'zh-Hans': { t: '点击播放', reviewed: 'bt' } },

    // ── The settings popover ────────────────────────────────────────────────
    'label.language':       { en: { t: 'Language' },   fr: { t: 'Langue', reviewed: true },
                        'zh-Hans': { t: '语言', reviewed: 'bt' } },
    'label.hoverHelp':      { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true },
                        'zh-Hans': { t: '悬停帮助', reviewed: 'bt' } },

    // ── Button faces, both of them keyed ────────────────────────────────────
    // FOUR faces, not two, because this page has two casings and they are two
    // different controls: the glissando section toggles paint ON / OFF in caps,
    // the effects bypass buttons and the settings switch paint On / Off. They
    // are KEYS through setLabel, never literals — a literal holds one string, so
    // switching to French mid-session would restore an English face. Written
    // from an if/else with two literal keys, never a ternary inside the call
    // (check-i18n assertion 13).
    //
    // NOT value mirrors under D-01: freeToggle / scaleToggle / the four bypass
    // parameters are all AudioParameterBool, so no automation lane ever shows
    // either word and translating them cannot make the page and the host
    // disagree. That is carried item 8's discriminator — does it track a
    // PARAMETER's choice STRING — and the answer here is no.
    'ui.onCaps':            { en: { t: 'ON' },  fr: { t: 'ACTIVÉ',    reviewed: true },
                        'zh-Hans': { t: '开启', reviewed: 'bt' } },
    'ui.offCaps':           { en: { t: 'OFF' }, fr: { t: 'DÉSACTIVÉ', reviewed: true },
                        'zh-Hans': { t: '关闭', reviewed: 'bt' } },
    'ui.on':                { en: { t: 'On' },  fr: { t: 'Act.',      reviewed: true },
                        'zh-Hans': { t: '开', reviewed: 'bt' } },
    'ui.off':               { en: { t: 'Off' }, fr: { t: 'Dés.',      reviewed: true },
                        'zh-Hans': { t: '关', reviewed: 'bt' } },

    // ── The preset dropdown ─────────────────────────────────────────────────
    // "Factory" is the GROUP HEADING above the preset list — chrome, and it
    // localizes. The preset NAMES beneath it do not: the name IS the JSON
    // filename (OuariconPresetManager.h:283-285), so translating one breaks
    // recall outright. They are written from a variable, not a literal, so no
    // exemption entry can or need cover them.
    'label.factory':        { en: { t: 'Factory' }, fr: { t: 'Usine', reviewed: true },
                        'zh-Hans': { t: '出厂', reviewed: 'bt' } },
    'label.noPresets':      { en: { t: 'No presets available' },
                              fr: { t: 'Aucun préréglage disponible', reviewed: true },
                        'zh-Hans': { t: '没有可用预设', reviewed: 'bt' } },

    // ── Accessible names ────────────────────────────────────────────────────
    // An aria-label is user-visible text by any definition that matters — it is
    // the accessible NAME, and a screen reader in French reading an English name
    // is the same failure as a French page with an English caption. None has a
    // rendered box, so none is a geometry risk.
    //
    // ELEVEN of these replace a native title= that v2.3.3 carried: five in the
    // markup and six written from the tuning tab's interval-list builder.
    // Contract §4 DELETES a native title rather than localizing it — on an
    // element that also has a data-tip it renders a second, untranslated OS
    // tooltip competing with the measure-then-pin renderer, and check-i18n
    // assertion 11 now fails on any that survive in the markup.
    'aria.presetPrev':      { en: { t: 'Previous preset' },   fr: { t: 'Préréglage précédent',   reviewed: true },
                        'zh-Hans': { t: '上一个预设', reviewed: 'bt' } },
    'aria.presetNext':      { en: { t: 'Next preset' },       fr: { t: 'Préréglage suivant',     reviewed: true },
                        'zh-Hans': { t: '下一个预设', reviewed: 'bt' } },
    'aria.presetName':      { en: { t: 'Click to see all presets' },
                              fr: { t: 'Cliquer pour voir tous les préréglages', reviewed: true },
                        'zh-Hans': { t: '点击查看全部预设', reviewed: 'bt' } },
    'aria.presetSave':      { en: { t: 'Save preset' },       fr: { t: 'Enregistrer le préréglage', reviewed: true },
                        'zh-Hans': { t: '保存预设', reviewed: 'bt' } },
    'aria.presetLoad':      { en: { t: 'Load preset' },       fr: { t: 'Ouvrir un préréglage',   reviewed: true },
                        'zh-Hans': { t: '载入预设', reviewed: 'bt' } },
    'aria.langSelect':      { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true },
                        'zh-Hans': { t: '界面语言', reviewed: 'bt' } },
    'aria.helpToggle':      { en: { t: 'Toggle hover help' },
                              fr: { t: 'Activer ou désactiver les infobulles', reviewed: true },
                        'zh-Hans': { t: '开关悬停帮助', reviewed: 'bt' } },

    // v2.4.1: was `valueDisplay.title = 'Double-click to edit'` in app.js — a
    // native title= written from JS, which contract §4 deletes rather than
    // localizes. It shipped from v2.0.0 because assertion 11 read the markup
    // only and could not see a JS-written one; the gate learned to in the
    // commit before this one. The same eighteen characters, verbatim, in an
    // accessible name the language sweep owns.
    'aria.valueEdit':       { en: { t: 'Double-click to edit' },
                              fr: { t: 'Double-cliquer pour modifier', reviewed: true },
                        'zh-Hans': { t: '双击编辑', reviewed: 'bt' } },
    // The two faces of the tonic selector's own accessible name. Chosen by an
    // if/else over two literal keys in updateIntervalListUI, never a ternary.
    'aria.tonicSelector12': { en: { t: 'Change tonic note (transposes instrument)' },
                              fr: { t: 'Changer la tonique (transpose l’instrument)', reviewed: true },
                        'zh-Hans': { t: '更改主音（移调乐器）', reviewed: 'bt' } },
    'aria.tonicSelectorN':  { en: { t: 'Transpose by semitones (shifts scale anchor point)' },
                              fr: { t: 'Transposer en demi-tons (déplace le point d’ancrage de la gamme)', reviewed: true },
                        'zh-Hans': { t: '按半音移调（移动音阶的锚点）', reviewed: 'bt' } },
    'aria.tonicPrev':       { en: { t: 'Previous note' },     fr: { t: 'Note précédente',        reviewed: true },
                        'zh-Hans': { t: '上一个音符', reviewed: 'bt' } },
    'aria.tonicNext':       { en: { t: 'Next note' },         fr: { t: 'Note suivante',          reviewed: true },
                        'zh-Hans': { t: '下一个音符', reviewed: 'bt' } },
    'aria.deviation':       { en: { t: 'Deviation from equal temperament' },
                              fr: { t: 'Déviation par rapport au tempérament égal', reviewed: true },
                        'zh-Hans': { t: '与十二平均律的偏差', reviewed: 'bt' } },
    'aria.libraryToggle':   { en: { t: 'Show or hide the tuning library' },
                              fr: { t: 'Afficher ou masquer la bibliothèque de gammes', reviewed: true },
                        'zh-Hans': { t: '显示或隐藏调音库', reviewed: 'bt' } },
    'aria.generatorToggle': { en: { t: 'Show or hide the scale generator' },
                              fr: { t: 'Afficher ou masquer le générateur de gammes', reviewed: true },
                        'zh-Hans': { t: '显示或隐藏音阶生成器', reviewed: 'bt' } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
// ============================================================================
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one.
//
// This plugin's list is long, and the length is the finding rather than a
// shortcut: THIRTEEN of its dropdowns are AudioParameterChoice controls whose
// option text IS the choice string the host prints in its automation lane.
// Translating an option alone would make the page and the lane disagree about
// what a setting is called — the same reasoning that keeps O-FreqPulse's
// "Global" English, applied to a plugin that has far more of them.
// ============================================================================

// The three reasons this plugin repeats. Shared through ONE exported object
// rather than three bare `const` declarations: assertion 7 rejects any
// top-level statement that is not an export declaration, because a file with
// side effects at module-evaluation time takes every later initializer on the
// page with it (pattern_module_toplevel_init_tdz). Repeating a 200-character
// reason forty-eight times would be the alternative, and a reason nobody reads
// is the reasonless skip list assertion 14 exists to forbid.
export const EXEMPT_REASONS = Object.freeze({
    choiceMirror:
        'an AudioParameterChoice option — English under D-01, so the page and the host automation lane agree on what the setting is called',
    tuningIdentifier:
        'a tuning identifier, not chrome — it names a scale the way a file name does, and the same string is what getTuningName returns and what a saved session resolves against',
    libraryMetadata:
        'embedded tuning-library metadata owned by C++ (getEmbeddedTuningList) — outside the WebView-only boundary this task draws, and the filter option and the item it filters have to spell the category the same way or the dropdown filters on words the list never shows',
});

export const I18N_EXEMPT = [
    ['Ouaricon Lyrica',
     'the product name in the header — a product name is never translated, and this one is also the plugin\'s registered PRODUCT_NAME in CMakeLists.txt'],
    ['Ouaricon Audio',
     'the company name in the footer — a company name is never translated'],

    // #preset-name-display shows the loaded preset. The name IS the JSON
    // filename (OuariconPresetManager.h:283-285), so translating it breaks
    // recall. "Default" is the placeholder the manager overwrites on its first
    // pass.
    ['Default', 'a factory preset name — exempt under D-02, because the name IS the JSON filename'],

    // v2.5.0 — the Chinese endonym in #lang-select. The markup writes it as
    // the numeric character references &#31616;&#20307;&#20013;&#25991;, copied
    // byte-for-byte from the shipped convention rather than retyped, but the
    // parser decodes them before the coverage sweep ever runs, so THIS entry
    // has to carry the DECODED four characters or the exemption never matches.
    ['简体中文', 'an endonym — a language name is never translated'],

    // ── The thirteen choice dropdowns, D-01 ─────────────────────────────────
    // stringMaterial
    ['Gut', EXEMPT_REASONS.choiceMirror], ['Nylon', EXEMPT_REASONS.choiceMirror], ['Wire', EXEMPT_REASONS.choiceMirror],
    ['Carbon', EXEMPT_REASONS.choiceMirror], ['Metal Alloy', EXEMPT_REASONS.choiceMirror], ['Glass', EXEMPT_REASONS.choiceMirror],
    ['Crystal', EXEMPT_REASONS.choiceMirror], ['Energy', EXEMPT_REASONS.choiceMirror],
    // woodType
    ['Spruce', EXEMPT_REASONS.choiceMirror], ['Maple', EXEMPT_REASONS.choiceMirror], ['Exotic', EXEMPT_REASONS.choiceMirror],
    ['Synthetic', EXEMPT_REASONS.choiceMirror],
    // technique — the fourth option is already French in createParameterLayout
    // and the markup now matches it.
    ['Normal', EXEMPT_REASONS.choiceMirror], ['Harmonic', EXEMPT_REASONS.choiceMirror], ['Muted', EXEMPT_REASONS.choiceMirror],
    ['Près de la table', EXEMPT_REASONS.choiceMirror],
    // freeShape / glissandoShape
    ['Linear', EXEMPT_REASONS.choiceMirror], ['Accelerate', EXEMPT_REASONS.choiceMirror], ['Decelerate', EXEMPT_REASONS.choiceMirror],
    ['S-Curve', EXEMPT_REASONS.choiceMirror],
    // freeInterval / glissandoInterval
    ['Minor 2nd', EXEMPT_REASONS.choiceMirror], ['Major 2nd', EXEMPT_REASONS.choiceMirror], ['Minor 3rd', EXEMPT_REASONS.choiceMirror],
    ['Major 3rd', EXEMPT_REASONS.choiceMirror], ['Perfect 4th', EXEMPT_REASONS.choiceMirror], ['Tritone', EXEMPT_REASONS.choiceMirror],
    ['Perfect 5th', EXEMPT_REASONS.choiceMirror], ['Minor 6th', EXEMPT_REASONS.choiceMirror], ['Major 6th', EXEMPT_REASONS.choiceMirror],
    ['Minor 7th', EXEMPT_REASONS.choiceMirror], ['Major 7th', EXEMPT_REASONS.choiceMirror], ['Octave', EXEMPT_REASONS.choiceMirror],
    ['Octave + 5th', EXEMPT_REASONS.choiceMirror], ['2 Octaves', EXEMPT_REASONS.choiceMirror], ['2.5 Octaves', EXEMPT_REASONS.choiceMirror],
    ['3 Octaves', EXEMPT_REASONS.choiceMirror],
    // freeDirection / glissandoDirection
    ['Up to Note', EXEMPT_REASONS.choiceMirror], ['Down to Note', EXEMPT_REASONS.choiceMirror],
    // glissandoScale — "Custom" is BOTH a choice option here and the tuning
    // name written into #scale-name-display, and both readings keep it English.
    ['Major', EXEMPT_REASONS.choiceMirror], ['Minor', EXEMPT_REASONS.choiceMirror], ['Pentatonic', EXEMPT_REASONS.choiceMirror],
    ['Custom', EXEMPT_REASONS.choiceMirror + '; it is ALSO the tuning name written into the scale-name display, and ' + EXEMPT_REASONS.tuningIdentifier],
    // freeTempoSync / scaleTempoSync — the note divisions. "Off" here is the
    // first CHOICE of a tempo-sync parameter, not the button face: the button
    // faces are ui.off / ui.offCaps and do localize.
    ['Off', EXEMPT_REASONS.choiceMirror, 'option'],
    ['1 Bar', EXEMPT_REASONS.choiceMirror], ['2 Bars', EXEMPT_REASONS.choiceMirror], ['4 Bars', EXEMPT_REASONS.choiceMirror],

    // ── Tuning identifiers ──────────────────────────────────────────────────
    ['12-TET Standard', EXEMPT_REASONS.tuningIdentifier],

    // ── The embedded tuning library ─────────────────────────────────────────
    ['All Categories', EXEMPT_REASONS.libraryMetadata],
    ['Historical', EXEMPT_REASONS.libraryMetadata],
    ['Just Intonation', EXEMPT_REASONS.libraryMetadata],
    ['Equal Divisions', EXEMPT_REASONS.libraryMetadata],
    ['Non-Octave', EXEMPT_REASONS.libraryMetadata],
    ['World', EXEMPT_REASONS.libraryMetadata],
];

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE. Every anchor on this page is named
// INDIVIDUALLY by the id of the single control it wraps, then resolved up to
// the group with .closest(). That indirection is what makes the binding safe on
// a page with two structurally identical glissando panels: five captions read
// Sync / Shape / Interval / Direction / Semitones in BOTH, and a class selector
// would bind all ten tips to the five Free-panel anchors, because
// document.querySelector returns the FIRST match in document order.
//
// 46 anchors, 46 keys, one-to-one — this page reuses no tip. The 43 that
// existed at v2.3.3 are bound below in document order; `settings`,
// `lang-select` and `tips-toggle` are the three controls this version adds.
//
// EVERY ONE OF THESE ELEMENTS EXISTS IN THE MARKUP before applyI18n runs. None
// of the 43 is built at runtime, so no binding here can silently write onto
// nothing — but seven of them live in a tab that is display:none at rest, which
// is why tests/i18n-states.json drives all four tabs.
export const TIP_BINDINGS = [
    ['#gear-btn',      'settings'],
    ['#lang-select',   'lang-select'],
    ['#tips-toggle',   'tips-toggle'],

    // SOUND tab
    ['#brightness',        'brightness',        '.slider-group'],
    ['#timbre',            'timbre',            '.slider-group'],
    ['#decayTime',         'decayTime',         '.slider-group'],
    ['#attackNoise',       'attackNoise',       '.slider-group'],
    ['#humanize',          'humanize',          '.slider-group'],
    ['#stringMaterial',    'stringMaterial',    '.dropdown-group'],
    ['#stringTension',     'stringTension',     '.slider-group'],
    ['#stringGauge',       'stringGauge',       '.slider-group'],
    ['#stringLength',      'stringLength',      '.slider-group'],
    ['#stringStiffness',   'stringStiffness',   '.slider-group'],
    ['#bodySize',          'bodySize',          '.slider-group'],
    ['#bodyResonance',     'bodyResonance',     '.slider-group'],
    ['#woodType',          'woodType',          '.dropdown-group'],
    ['#bodyModeSpread',    'bodyModeSpread',    '.slider-group'],
    ['#bridgeBrightness',  'bridgeBrightness',  '.slider-group'],
    ['#pluckPosition',     'pluckPosition',     '.slider-group'],
    ['#fingerHardness',    'fingerHardness',    '.slider-group'],
    ['#technique',         'technique',         '.dropdown-group'],
    ['#sympatheticAmount', 'sympatheticAmount', '.slider-group'],
    ['#sympatheticQ',      'sympatheticQ',      '.slider-group'],

    // TECHNIQUES tab
    ['#freeKeyswitchNote',        'freeKeyswitchNote',        '.dropdown-group'],
    ['#scaleKeyswitchNote',       'scaleKeyswitchNote',       '.dropdown-group'],
    ['#freeTempoSync',            'freeTempoSync',            '.dropdown-group'],
    ['#glissandoTime',            'glissandoTime',            '.slider-group'],
    ['#freeShape',                'freeShape',                '.dropdown-group'],
    ['#freeInterval',             'freeInterval',             '.dropdown-group'],
    ['#freeDirection',            'freeDirection',            '.dropdown-group'],
    ['#freeCustomSemitones',      'freeCustomSemitones',      '.slider-group'],
    ['#glissandoScale',           'glissandoScale',           '.dropdown-group'],
    ['#glissandoTonic',           'glissandoTonic',           '.dropdown-group'],
    ['#scaleTempoSync',           'scaleTempoSync',           '.dropdown-group'],
    ['#glissandoSpeed',           'glissandoSpeed',           '.slider-group'],
    ['#glissandoShape',           'glissandoShape',           '.dropdown-group'],
    ['#glissandoInterval',        'glissandoInterval',        '.dropdown-group'],
    ['#glissandoDirection',       'glissandoDirection',       '.dropdown-group'],
    ['#glissandoCustomSemitones', 'glissandoCustomSemitones', '.slider-group'],
    ['#glissandoExcitation',      'glissandoExcitation',      '.slider-group'],
    ['#glissandoHumanize',        'glissandoHumanize',        '.slider-group'],
    ['#glissandoVelStart',        'glissandoVelStart',        '.slider-group'],
    ['#glissandoVelEnd',          'glissandoVelEnd',          '.slider-group'],

    // TUNING tab
    ['#ref-pitch-knob',  'ref-pitch-knob',  '.tuning-ref-section'],
    ['#octave-stretch',  'octave-stretch',  '.octave-stretch-section'],

    // Footer
    ['#masterVolume',    'masterVolume',    '.master-volume'],
];

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

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
