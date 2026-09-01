/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
// ── v1.11.1: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 40 entries (29 terminology, 8 typography, 1 grammar/agreement,
// 2 meaning). sameAsEn: kept 14, translated 0, added 1 (label.group.ping, which
// the glossary's settled "ping" makes a straight copy). termNote exemptions: 4
// (listed below). Left as drafted: the rest. reviewed: false throughout — no
// native speaker yet. Lint 30 -> 1; the 1 is reported, not fixed, below.
//
// THE DECISIONS A NEXT READER NEEDS, each measured with the check-ui-labels
// method (Range.selectNodeContents on the real node at the shipping 1100x720):
//
//  - Décroissance (rolloff) is 81.59 px in an 88 px content box. THE v1.9.0
//    HEADER BELOW WAS WRONG about this: it defended "Décroiss." on "a 72 px
//    cell that holds 9", and the cell is 88. The root term fits, so the caption,
//    the tip title and the aria-label are now ONE string — they were two
//    ("Décroiss." on screen, "Atténuation" in the accessible name), which is a
//    WCAG 2.5.3 label-in-name miss as well as two French names for one control.
//  - Trajectoire (motionPath) is 66.73 px in a 52 px box and does NOT fit, so
//    the caption stays "Tracé" — and the TIP TITLE moved to "Tracé" to meet it,
//    rather than leaving the aria-label saying "Trajectoire" over a caption
//    saying "Tracé". "Trajectoire" stays in the prose bodies, where it is the
//    right word and costs no geometry. The glossary accepts both for `path`.
//  - Enregistrer 72.83 px and Charger 46.34 px both fit the 121 px content box
//    of the .vbtn pairs, so label.save / label.load carry the ROOT terms rather
//    than "Enreg." / "Ouvrir". The v1.9.0 comment below defended the two
//    abbreviations on a 168 px rail; the buttons measure 121 px of content each
//    and 133 px border-box, two of them inside a 278 px row. Three sibling
//    plugins still ship "Enreg." — that is now a suite question, recorded in
//    the Stage N report, not a reason to keep a form that does not fit anything.
//  - "H.-parleur" KEPT: "Haut-parleur" wraps inside the 74 px popover box (both
//    measure 61.92 px because both wrap), and a wrapped caption fails gate [4].
//  - "Att. env." KEPT: it is the glossary's listed abbreviation for `hull atten`
//    and the root "Atténuation hors enveloppe" is far past the same 88 px box.
//  - THE SIX MOTION PATH FACES AND THE SYNC DIVISIONS ARE ENGLISH ON SCREEN in
//    both languages: they come from juce::StringArray literals in
//    PluginProcessor.cpp:200-204 (a host automation lane must read "Figure-8"),
//    and <option>s are built from those choices. So the French bodies now name
//    Orbit / Figure-8 / Sweep / Drift / Pendulum / Spiral and Free / 1 Bar
//    exactly as the English bodies do, instead of Orbite / Huit / Balayage /
//    Dérive / Pendule / Spirale, which the user cannot find in the dropdown.
//    The mirror rule is the English author's own: a CAPITALISED name is a face
//    and stays English; a lower-case "une orbite", "un huit" is generic prose
//    and is translated.
//  - FOUR CAPTIONS THAT *ARE* LOCALIZED were still named in English inside
//    French bodies, and are now named in French: « Derive » -> « Calculer »,
//    « Ear » -> « Oreille », "la colonne Delay" -> "la colonne Retard", and
//    "Armez d'abord STORE" -> "Armez d'abord MÉM.". The comment at scene-slot
//    that justified the last one has been corrected: it predates the LABELS
//    table and page chrome IS localized now.
//  - termNote x4: rolloff and label.rolloff (a DBAP dB-per-doubling distance
//    law, not the glossary's filter `pente` — and this page HAS a filter, Air),
//    label.delay and label.vcol.delay (the alignment delay the glossary names
//    this plugin for).
//  - REPORTED, NOT FIXED — the lint's one remaining finding is a FALSE
//    POSITIVE: T2 reads every `\d.\d` in French prose as a decimal needing a
//    comma, and safe-banner's "Stereo -> 7.1" is Logic's surround-format
//    identifier and the literal menu entry the user has to find. "7,1" is not a
//    channel format in any DAW. The glossary already exempts readout values and
//    identifiers; the lint has no exemption for one, and termNote covers G1/F1
//    only.
//  - NO ENGLISH DEFECT FOUND. The three claims worth checking against the
//    processor all hold: motionRate's "centred on 0.3" is rateRange's skew
//    centre (PluginProcessor.cpp:188-189), srcZ's "−2 to 8 m" is its range
//    (:120), and motionSize's "a 6 m orbit" is the 0-24 m range's default (:206).
// ============================================================================
// i18n.js — O-Octagon hover help AND page labels, English + French (v1.9.0)
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz). Two
// independent gates enforce that — scripts/check-i18n.js assertion 7, and
// section 2 of tests/ui_frontend_check.js, which additionally requires init()
// to remain the literal last statement of app.js. That is why app.js calls
// initI18n() from INSIDE init() rather than from a foot-of-file block.
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores, so a second file named i18n-fr.js would have
// to be reached as the symbol i18nfr_js (critical_binary_data_strips_hyphens).
// One combined file for both languages sidesteps the question entirely.
//
// COPY IS textContent ON EVERY PATH — never innerHTML. showTip() in app.js
// builds the tip with createElement + textContent, and check-i18n assertion 9
// rejects any innerHTML reference here and any string literal containing `<`.
// A line break, if one is ever needed, is \n plus CSS white-space: pre-line,
// never a markup tag.
//
// THE ENGLISH WAS MOVED, NOT REWRITTEN. Every en entry below is byte-for-byte
// what index.html carried through v1.5.0, with its HTML entities decoded to the
// characters they named (&#8212; -> —, &#8202; -> \u200A, and so on) because
// setAttribute + textContent do not decode entities.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// key -> { en: {t, b}, fr: {t, b, reviewed} }
//   t = tooltip title (the small caps line), b = tooltip body.
//
// Object.freeze() rather than a bare `{...}` literal for two reasons. It says
// out loud that this module is inert data nothing may mutate at runtime — and
// it keeps the export a SINGLE top-level statement, because a statement written
// `export const X = {...};` closes its brace at depth zero and segments the
// trailing `;` off on its own.
//
// PARAMETERISED ENTRIES carry {token} placeholders substituted by tr()'s `vars`
// argument. They are NOT template literals: the table is inert data, evaluated
// once at module load, so a value interpolated here would be frozen at whatever
// the language happened to be then.
export const I18N = Object.freeze({

    // ── Header ──────────────────────────────────────────────────────────────
    'tab-room': {
        en: { t: 'Room',
              b: 'The performance view — plan, puck, weights, scenes, meters and the DBAP field.' },
        fr: { t: 'Salle',
              b: 'La vue de jeu — le plan, le repère de source, les poids, les scènes, les indicateurs de niveau et le champ DBAP.',
              reviewed: false },
    },
    'tab-venue': {
        en: { t: 'Venue',
              b: 'The 42 measured values that define the room — positions, trims, rake — plus venue files, presets, output order and the verify ping.' },
        fr: { t: 'Lieu',
              b: 'Les 42 valeurs mesurées qui définissent la salle — positions, corrections, inclinaison des gradins — ainsi que les fichiers de lieu, les préréglages, l’ordre des sorties et le ping de vérification.',
              reviewed: false },
    },

    // ── The settings popover (v1.6.0) ───────────────────────────────────────
    // The gear and the language selector are new. The hover-help entry is the
    // v1.2.0 "?" toggle's copy, moved here unchanged along with the control.
    'settings': {
        en: { t: 'Settings',
              b: 'Choose the language of this hover help, and turn the hover help on or off. Both choices are remembered with the session.' },
        fr: { t: 'Réglages',
              b: 'Choisir la langue de cette aide au survol et activer ou désactiver cette aide. Les deux choix sont conservés avec la session.',
              reviewed: false },
    },
    'lang-select': {
        en: { t: 'Language',
              b: 'The language this hover help is written in. English and French are available; the labels on the page change with it, but numbers and unit symbols stay as they are.' },
        fr: { t: 'Langue',
              b: 'La langue dans laquelle cette aide au survol est rédigée. L’anglais et le français sont disponibles ; les libellés de la page changent avec elle, mais les nombres et les symboles d’unité restent inchangés.',
              reviewed: false },
    },
    // ONE key covering both states, never a state-swapped pair. applyI18n()
    // re-renders every tip straight from this table on a language change, so a
    // state-dependent string written outside it would be stranded in the
    // previous language the moment the selector fires — and window.__setLanguage(),
    // which the clamp gates drive, fires no `change` event at all, so no
    // listener workaround would cover both paths. The two buttons and their
    // aria-pressed carry the state instead.
    'tips-toggle': {
        en: { t: 'Hover help',
              b: 'Show a short description when the pointer rests on a control. The setting is remembered with the session.' },
        fr: { t: 'Aide au survol',
              b: 'Affiche une courte description lorsque le pointeur s’arrête sur une commande. Le réglage est conservé avec la session.',
              reviewed: false },
    },

    // ── Banners ─────────────────────────────────────────────────────────────
    'safe-banner': {
        en: { t: 'Safe mode',
              b: 'The host negotiated stereo, not the 8\u200A·\u200Achannel rig — what you hear is a fold-down. In Logic, insert via the slot\'s Stereo → 7.1 entry.' },
        fr: { t: 'Mode sécurisé',
              b: 'L’hôte a négocié une sortie stéréo, pas le dispositif à 8\u200A·\u200Acanaux — ce que vous entendez est un repli. Dans Logic, insérez le plugin par l’entrée Stéréo → 7.1 de l’emplacement.',
              reviewed: false },
    },
    'map-banner': {
        en: { t: 'Map invalid',
              b: 'A speaker label no longer resolves against the negotiated output set — the reason and the row to fix are printed here. Until fixed, the rig falls back to a raw stereo split.' },
        fr: { t: 'Affectation invalide',
              b: 'Un libellé de haut-parleur ne correspond plus au jeu de sorties négocié — la raison et la ligne à corriger sont indiquées ici. Tant que ce n’est pas corrigé, le dispositif revient à un simple partage stéréo.',
              reviewed: false },
    },

    'monitor-banner': {
        en: { t: 'Monitor fold-down',
              b: 'Headphone fold of the eight solved feeds into outputs 1\u20112, with the other six muted. Never included in an offline bounce, never remembered across a reload, and switched off when this window closes.' },
        fr: { t: 'Repli de contrôle',
              b: 'Repli casque des huit sorties calculées vers les sorties 1\u20112, les six autres étant coupées. Jamais inclus dans un export hors ligne, jamais conservé au rechargement, et désactivé à la fermeture de cette fenêtre.',
              reviewed: false },
    },
    'monitor-toggle': {
        en: { t: 'Monitor on headphones',
              b: 'Fold the eight speaker feeds to a stereo pair so the piece can be heard away from the venue. Position, distance and inter-aural delay come from the measured venue geometry. On the 8‑channel rig this is a listening aid that cannot reach an offline render; on a stereo bus it is on by default, is remembered with the session, and is what the bus outputs.' },
        fr: { t: 'Contrôle au casque',
              b: 'Replie les huit sorties haut-parleur en une paire stéréo pour écouter la pièce hors du lieu. La position, la distance et le retard interaural proviennent de la géométrie mesurée du lieu. Sur le dispositif à 8 canaux, c’est une aide à l’écoute qui ne peut pas atteindre un export hors ligne ; sur un bus stéréo, le repli est actif par défaut, conservé avec la session, et constitue la sortie du bus.',
              reviewed: false },
    },

    // ── Room plan ───────────────────────────────────────────────────────────
    'puck': {
        en: { t: 'Source',
              b: 'Drag to move the source over the plan — writes Source X and Y together. Double-click a knob or slider elsewhere to reset it.' },
        fr: { t: 'Source',
              b: 'Faites glisser pour déplacer la source sur le plan — écrit Source X et Y ensemble. Double-cliquez ailleurs sur un bouton ou un curseur pour le réinitialiser.',
              reviewed: false },
    },
    // {n} is a speaker number, substituted literally — not an I18N key. The
    // eight weight cells shared one sentence in the markup and share one entry
    // here, which is also what stops seven of the eight going stale when the
    // wording changes.
    'weight': {
        en: { t: 'Weight {n}',
              b: 'Speaker {n}’s share of the DBAP solve, 0–1. Double-click the glyph to reassign its physical output; double-click the slider to reset.' },
        fr: { t: 'Poids {n}',
              b: 'Part du haut-parleur {n} dans la résolution DBAP, de 0 à 1. Double-cliquez sur le glyphe pour réaffecter sa sortie physique ; double-cliquez sur le curseur pour réinitialiser.',
              reviewed: false },
    },

    // ── Position ────────────────────────────────────────────────────────────
    'srcX': {
        en: { t: 'Source X',
              b: 'Left–right position of the source, resolved against the measured speaker bounding box — the metres readout below is live.' },
        fr: { t: 'Source X',
              b: 'Position gauche–droite de la source, rapportée au rectangle englobant mesuré des haut-parleurs — l’affichage en mètres ci-dessous est en temps réel.',
              reviewed: false },
    },
    'srcY': {
        en: { t: 'Source Y',
              b: 'Front–back position of the source, resolved against the measured speaker bounding box.' },
        fr: { t: 'Source Y',
              b: 'Position avant–arrière de la source, rapportée au rectangle englobant mesuré des haut-parleurs.',
              reviewed: false },
    },
    'srcZ': {
        en: { t: 'Source Z',
              b: 'Source height, −2 to 8 m. Rising toward the speaker plane gets louder and sharper; flying above the array recedes (±6 dB proximity cue). The elevation strip shows it against the rake and the speaker heights — the marker clamps, the numbers never do.' },
        fr: { t: 'Source Z',
              b: 'Hauteur de la source, de −2 à 8 m. En montant vers le plan des haut-parleurs, le son gagne en niveau et en netteté ; en s’élevant au-dessus du dispositif, il s’éloigne (indice de proximité de ±6 dB). La bande d’élévation la situe par rapport à l’inclinaison des gradins et aux hauteurs des haut-parleurs — le repère est borné, jamais les valeurs.',
              reviewed: false },
    },
    'width': {
        en: { t: 'Width',
              b: 'Spreads the source into sub-points around its position, up to 12 m apart — wider reads as a broader image across the rig. On stereo material it pulls the L and R feeds to different parts of the room; on mono material, reach for Decorrelate below.' },
        fr: { t: 'Largeur',
              b: 'Répartit la source en points secondaires autour de sa position, jusqu’à 12 m d’écart — plus la valeur est élevée, plus l’image s’élargit sur le dispositif. Sur du matériel stéréo, les voies G et D sont envoyées vers des zones différentes de la salle ; sur du mono, utilisez Décorréler ci-dessous.',
              reviewed: false },
    },
    // ── v1.8.0 — the motion engine ──
    'gtab-position': {
        en: { t: 'Position',
              b: 'The anchor: where the source sits, its height and its width. With motion running the anchor is the hollow ghost on the map and the path travels with it.' },
        fr: { t: 'Position',
              b: 'L’ancrage : la position de la source, sa hauteur et sa largeur. Quand le mouvement tourne, l’ancrage est le fantôme creux sur le plan et la trajectoire se déplace avec lui.',
              reviewed: false },
    },
    'gtab-motion': {
        en: { t: 'Motion',
              b: 'Generative trajectories around the anchor — orbits, figure-8s, sweeps, a seeded random walk — drawn in venue metres and locked to the host tempo. Switch Motion on and the map draws the trace the source will follow. The dot lights while motion runs.' },
        fr: { t: 'Mouvement',
              b: 'Trajectoires génératives autour de l’ancrage — orbites, huit, balayages, marche aléatoire à graine — tracées en mètres de salle et calées sur le tempo de l’hôte. Activez le mouvement et le plan trace le chemin que suivra la source. Le point s’allume quand le mouvement tourne.',
              reviewed: false },
    },
    'motionOn': {
        en: { t: 'Run',
              b: 'Starts the trajectory. Off, the plugin renders exactly as it did before motion existed — bit for bit — and the three position lanes are never written either way: motion is an offset added downstream of them.' },
        fr: { t: 'Marche',
              b: 'Lance la trajectoire. Désactivé, le plugin rend exactement ce qu’il rendait avant l’existence du mouvement — bit pour bit — et les trois pistes de position ne sont jamais écrites dans un cas comme dans l’autre : le mouvement est un décalage ajouté en aval.',
              reviewed: false },
    },
    'motionPath': {
        en: { t: 'Path',
              b: 'Orbit (ellipse, Ratio sets the minor axis), Figure-8 (a 1:2 Lissajous), Sweep (a line with a ping-pong fold), Drift (a seeded Perlin walk — no trace, a tail), Pendulum (a single-axis swing) and Spiral (winds in over the first half-cycle, out over the second).' },
        fr: { t: 'Tracé',
              b: 'Orbit (ellipse, Ratio règle le petit axe), Figure-8 (Lissajous 1:2), Sweep (une ligne avec repli aller-retour), Drift (marche de Perlin à graine — pas de trace, une traîne), Pendulum (balancement sur un axe) et Spiral (s’enroule sur la première moitié du cycle, se déroule sur la seconde).',
              reviewed: false },
    },
    'motionSync': {
        en: { t: 'Sync',
              b: 'Free runs at the Rate in Hz. A division locks one cycle to the host clock — 1/4 is one cycle per beat, 1 Bar one cycle per four beats (4/4 assumed) — so a bounce is downbeat-aligned and repeatable; with the transport stopped the source rests where playback will resume.' },
        fr: { t: 'Synchro',
              b: 'Le mode Free suit la Vitesse en Hz. Une division cale un cycle sur l’horloge de l’hôte — 1/4 fait un cycle par temps, 1 Bar un cycle par quatre temps (4/4 supposé) — de sorte qu’un export soit aligné sur le temps fort et reproductible ; transport arrêté, la source se pose là où la lecture reprendra.',
              reviewed: false },
    },
    'motionRate': {
        en: { t: 'Rate',
              b: 'Cycles per second in Free mode, 0.01 to 4 Hz, centred on 0.3. Ignored while Sync is a tempo division.' },
        fr: { t: 'Vitesse',
              b: 'Cycles par seconde en mode Free, de 0,01 à 4 Hz, centré sur 0,3. Ignoré tant que Synchro est une division du tempo.',
              reviewed: false },
    },
    'motionSize': {
        en: { t: 'Size',
              b: 'The path’s extent in venue metres — a 6 m orbit is 6 m across in any hall. It may leave the speaker rig: the hull trim and the rolloff model that honestly, and it is the one gesture no host automation can draw.' },
        fr: { t: 'Taille',
              b: 'L’étendue de la trajectoire en mètres de salle — une orbite de 6 m mesure 6 m de large dans n’importe quelle salle. Elle peut sortir du dispositif de haut-parleurs : l’atténuation hors enveloppe et la décroissance le modélisent honnêtement, et c’est le geste qu’aucune automation d’hôte ne peut dessiner.',
              reviewed: false },
    },
    'motionRatio': {
        en: { t: 'Ratio',
              b: 'Minor axis over major, 0 to 1. 1 is a circle; 0 collapses Orbit and Figure-8 onto a line.' },
        fr: { t: 'Ratio',
              b: 'Petit axe sur grand axe, de 0 à 1. 1 donne un cercle ; 0 aplatit Orbit et Figure-8 sur une ligne.',
              reviewed: false },
    },
    'motionAngle': {
        en: { t: 'Angle',
              b: 'Rotates the path about the anchor, so a Sweep or a Pendulum can run front-to-back in a portrait room.' },
        fr: { t: 'Angle',
              b: 'Fait pivoter la trajectoire autour de l’ancrage, pour qu’un Sweep ou un Pendulum puisse aller d’avant en arrière dans une salle en longueur.',
              reviewed: false },
    },
    'motionHeight': {
        en: { t: 'Height',
              b: 'Vertical amplitude in metres, coupled to the same phase: an orbit tilts into a ring, a figure-8 becomes a lobe. Added to Source Z; the elevation strip shows the live height.' },
        fr: { t: 'Hauteur',
              b: 'Amplitude verticale en mètres, couplée à la même phase : une orbite s’incline en anneau, un huit devient un lobe. Ajoutée à Source Z ; la bande d’élévation montre la hauteur en temps réel.',
              reviewed: false },
    },
    'motionPhase': {
        en: { t: 'Phase',
              b: 'Where on the path the cycle starts, in degrees. Offset two instances to stagger them against one beat.' },
        fr: { t: 'Phase',
              b: 'Point de départ du cycle sur la trajectoire, en degrés. Décalez deux instances pour les échelonner sur un même temps.',
              reviewed: false },
    },
    'motionSeed': {
        en: { t: 'Seed',
              b: 'Drift only. Picks the wander — shop for a shape, and the preset keeps it. Two bounces of one session are identical.' },
        fr: { t: 'Graine',
              b: 'Drift uniquement. Choisit l’errance — cherchez une forme, le préréglage la conserve. Deux exports d’une même session sont identiques.',
              reviewed: false },
    },
    'decorr': {
        en: { t: 'Decorrelate',
              b: 'Makes Width audible on mono material. Width alone moves two IDENTICAL copies of the signal apart in the room, and two identical copies comb rather than widen; this gives each copy its own all-pass network so they share a spectrum but not a phase. Off by default, and inert at Width 0 — where the two feeds land on the same speakers, decorrelating them would only cost you the coherent sum. Expect up to 3 dB less level where the two feeds overlap: that is the combing going away.' },
        fr: { t: 'Décorréler',
              b: 'Rend la Largeur audible sur du matériel mono. La Largeur seule éloigne deux copies IDENTIQUES du signal dans la salle, et deux copies identiques produisent un filtrage en peigne au lieu d’élargir ; ici, chaque copie reçoit son propre réseau passe-tout, de sorte qu’elles partagent le spectre mais pas la phase. Désactivé par défaut, et sans effet à Largeur 0 — là où les deux voies aboutissent aux mêmes haut-parleurs, les décorréler ne ferait que vous coûter la somme cohérente. Attendez-vous à jusqu’à 3 dB de niveau en moins là où les deux voies se recouvrent : c’est le filtrage en peigne qui disparaît.',
              reviewed: false },
    },

    // ── Solve ───────────────────────────────────────────────────────────────
    'rolloff': {
        en: { t: 'Rolloff',
              b: 'DBAP distance rolloff, 3–12 dB per distance doubling. Higher concentrates energy hard into the nearest speakers; lower spreads it across the whole array.' },
        fr: { t: 'Décroissance',
              termNote: 'DBAP distance rolloff — a dB-per-doubling attenuation law, not a filter slope; this page has a real filter (Air) and Pente would name that instead',
              b: 'Décroissance DBAP avec la distance, de 3 à 12 dB par doublement de distance. Une valeur élevée concentre fortement l’énergie sur les haut-parleurs les plus proches ; une valeur faible la répartit sur tout le dispositif.',
              reviewed: false },
    },
    'blur': {
        en: { t: 'Blur',
              b: 'Softens the distance differences the solve sees — full blur washes the source across the whole array. Scaled with the rig, not in metres, so a patch means the same thing in a club and a hall.' },
        fr: { t: 'Flou',
              b: 'Adoucit les écarts de distance que voit la résolution — au maximum, la source se répand sur tout le dispositif. La valeur suit l’échelle du dispositif et non des mètres, si bien qu’un réglage a le même sens dans un club et dans une grande salle.',
              reviewed: false },
    },

    // ── Space ───────────────────────────────────────────────────────────────
    'hullAtten': {
        en: { t: 'Hull attenuation',
              b: 'How strongly the source fades as it crosses outside the speaker hull.' },
        fr: { t: 'Atténuation hors enveloppe',
              b: 'Détermine l’intensité avec laquelle la source s’efface lorsqu’elle sort de l’enveloppe des haut-parleurs.',
              reviewed: false },
    },
    'airAmount': {
        en: { t: 'Air',
              b: 'Distance air filter — high frequencies fall away as the source sits farther from the array, with the cutoff derived from the venue geometry.' },
        fr: { t: 'Air',
              b: 'Filtre d’air lié à la distance — les aigus s’estompent à mesure que la source s’éloigne du dispositif, la fréquence de coupure étant déduite de la géométrie du lieu.',
              reviewed: false },
    },

    // ── Output ──────────────────────────────────────────────────────────────
    'outputGain': {
        en: { t: 'Output',
              b: 'Master trim for all eight channels.' },
        fr: { t: 'Sortie',
              b: 'Réglage général de niveau pour les huit canaux.',
              reviewed: false },
    },

    // ── Scenes ──────────────────────────────────────────────────────────────
    'scene-store': {
        en: { t: 'Store',
              b: 'Arm, then click a U slot to capture the current eight weights into it. Disarms after one capture.' },
        fr: { t: 'Mémoriser',
              b: 'Armez, puis cliquez sur un emplacement U pour y enregistrer les huit poids actuels. Se désarme après une seule capture.',
              reviewed: false },
    },
    'scene-all': {
        en: { t: 'All',
              b: 'All eight speakers at full weight.' },
        fr: { t: 'Tous',
              b: 'Les huit haut-parleurs à poids maximal.',
              reviewed: false },
    },
    'scene-front': {
        en: { t: 'Front',
              b: 'The front speakers only — membership is derived from the measured geometry, not from fixed slot numbers.' },
        fr: { t: 'Avant',
              b: 'Les haut-parleurs avant uniquement — l’appartenance est déduite de la géométrie mesurée, et non de numéros d’emplacement fixes.',
              reviewed: false },
    },
    'scene-rear': {
        en: { t: 'Rear',
              b: 'The rear speakers only, derived from the measured geometry.' },
        fr: { t: 'Arrière',
              b: 'Les haut-parleurs arrière uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    'scene-left': {
        en: { t: 'Left',
              b: 'The left-side speakers only, derived from the measured geometry.' },
        fr: { t: 'Gauche',
              b: 'Les haut-parleurs du côté gauche uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    'scene-right': {
        en: { t: 'Right',
              b: 'The right-side speakers only, derived from the measured geometry.' },
        fr: { t: 'Droite',
              b: 'Les haut-parleurs du côté droit uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    'scene-sides': {
        en: { t: 'Sides',
              b: 'The side speakers only, derived from the measured geometry.' },
        fr: { t: 'Côtés',
              b: 'Les haut-parleurs latéraux uniquement, déduits de la géométrie mesurée.',
              reviewed: false },
    },
    // {n} is a slot number, substituted literally. The body names the STORE
    // button by the face the reader can actually see, which since the LABELS
    // table landed (canon v2, v1.9.0) is "MÉM." in French — the sentence here
    // said "STORE" until Stage N, from a v1.2.0 comment written when page
    // chrome was still English everywhere.
    'scene-slot': {
        en: { t: 'User scene {n}',
              b: 'Click to recall this stored weight scene. Arm STORE first to capture the current weights into it.' },
        fr: { t: 'Scène utilisateur {n}',
              b: 'Cliquez pour rappeler cette scène de poids enregistrée. Armez d’abord MÉM. pour y enregistrer les poids actuels.',
              reviewed: false },
    },

    // ── Elevation strip ─────────────────────────────────────────────────────
    'elevation': {
        en: { t: 'Elevation',
              b: 'The room side-on — rake line, speaker heights and the source marker. Ear is the listener height under the source; Source is its absolute height.' },
        fr: { t: 'Élévation',
              b: 'La salle vue de côté — ligne d’inclinaison des gradins, hauteurs des haut-parleurs et repère de la source. « Oreille » est la hauteur d’écoute sous la source ; « Source » est sa hauteur absolue.',
              reviewed: false },
    },

    // ── Venue screen ────────────────────────────────────────────────────────
    'rake': {
        en: { t: 'Rake',
              b: 'Audience rake — ear height at the front and rear of the seating. Heights between are interpolated along the room depth.' },
        fr: { t: 'Inclinaison',
              b: 'Inclinaison des gradins — hauteur d’oreille à l’avant et à l’arrière du public. Les hauteurs intermédiaires sont interpolées sur la profondeur de la salle.',
              reviewed: false },
    },
    'delay': {
        en: { t: 'Alignment delay',
              b: 'Per-speaker delay that time-aligns arrivals at one seat. Derive fills all eight from the measured distances; every value stays editable afterwards.' },
        fr: { t: 'Retard d’alignement',
              b: 'Retard par haut-parleur qui aligne temporellement les arrivées à une place donnée. « Calculer » remplit les huit valeurs à partir des distances mesurées ; chaque valeur reste modifiable ensuite.',
              reviewed: false },
    },
    'delay-unit': {
        en: { t: 'Delay unit',
              b: 'Show the Delay column in milliseconds or in metres of path difference. Values are stored as milliseconds either way.' },
        fr: { t: 'Unité de retard',
              b: 'Affiche la colonne Retard en millisecondes ou en mètres de différence de trajet. Les valeurs sont enregistrées en millisecondes dans les deux cas.',
              reviewed: false },
    },
    'delay-derive': {
        en: { t: 'Derive delays',
              b: 'Fill all eight delays to time-align arrivals at the centre of the array, at the ear height the rake gives that depth. The farthest speaker gets zero. Every value stays editable afterwards.' },
        fr: { t: 'Calculer les retards',
              b: 'Remplit les huit retards afin d’aligner temporellement les arrivées au centre du dispositif, à la hauteur d’oreille que l’inclinaison donne à cette profondeur. Le haut-parleur le plus éloigné reçoit zéro. Chaque valeur reste modifiable ensuite.',
              reviewed: false },
    },
    'output-set': {
        en: { t: 'Output set',
              b: 'The surround format the host negotiated — speaker labels must resolve against it.' },
        fr: { t: 'Jeu de sorties',
              b: 'Le format surround négocié par l’hôte — les libellés des haut-parleurs doivent y correspondre.',
              reviewed: false },
    },
    'venue-save': {
        en: { t: 'Save venue',
              b: 'Write the 42 measured values to a .venue file — independent of musical presets.' },
        fr: { t: 'Enregistrer le lieu',
              b: 'Écrit les 42 valeurs mesurées dans un fichier .venue — indépendant des préréglages musicaux.',
              reviewed: false },
    },
    'venue-load': {
        en: { t: 'Load venue',
              b: 'Load a .venue file. Rejected if its labels do not resolve against the negotiated output set.' },
        fr: { t: 'Charger un lieu',
              b: 'Charge un fichier .venue. Refusé si ses libellés ne correspondent pas au jeu de sorties négocié.',
              reviewed: false },
    },
    'preset-list': {
        en: { t: 'Preset',
              b: 'Musical presets carry the 28 parameters and the four user scenes — never the 42 measured venue values.' },
        fr: { t: 'Préréglage',
              b: 'Les préréglages musicaux contiennent les 28 paramètres et les quatre scènes utilisateur — jamais les 42 valeurs mesurées du lieu.',
              reviewed: false },
    },
    'preset-save': {
        en: { t: 'Save preset',
              b: 'Save the current parameters and user scenes as a preset.' },
        fr: { t: 'Enregistrer le préréglage',
              b: 'Enregistre les paramètres et les scènes utilisateur actuels sous forme de préréglage.',
              reviewed: false },
    },
    'preset-load': {
        en: { t: 'Load preset',
              b: 'Load the selected preset. The venue is untouched.' },
        fr: { t: 'Charger un préréglage',
              b: 'Charge le préréglage sélectionné. Le lieu n’est pas modifié.',
              reviewed: false },
    },
    'oo-direct': {
        en: { t: 'Direct 1–8',
              b: 'Wire speaker n to physical output n under the measured CoreAudio device order — the one-click fix for a rig cabled 1–8.' },
        fr: { t: 'Direct 1–8',
              b: 'Relie le haut-parleur n à la sortie physique n selon l’ordre de périphérique CoreAudio mesuré — la correction en un clic pour un dispositif câblé de 1 à 8.',
              reviewed: false },
    },
    'oo-roles': {
        en: { t: 'Roles',
              b: 'Restore the factory surround-role labels.' },
        fr: { t: 'Rôles',
              b: 'Rétablit les libellés de rôles surround d’usine.',
              reviewed: false },
    },
    'ping-grid': {
        en: { t: 'Verify ping',
              b: 'Sound a confirmation ping from one speaker. The lit number is what the plugin reports playing — never a timer.' },
        fr: { t: 'Ping de vérification',
              b: 'Émet un ping de confirmation depuis un haut-parleur. Le numéro allumé est celui que le plugin déclare en train de jouer — jamais une minuterie.',
              reviewed: false },
    },
    'ping-auto': {
        en: { t: 'Auto ping',
              b: 'Step the ping around all eight speakers in order.' },
        fr: { t: 'Ping automatique',
              b: 'Fait passer le ping successivement sur les huit haut-parleurs, dans l’ordre.',
              reviewed: false },
    },
    'ping-stop': {
        en: { t: 'Stop',
              b: 'Stop the ping.' },
        fr: { t: 'Arrêter',
              b: 'Arrête le ping.',
              reviewed: false },
    },

    // ── Footer readouts ─────────────────────────────────────────────────────
    'readout-source': {
        en: { t: 'Source',
              b: 'The source position in metres, resolved against the live venue geometry.' },
        fr: { t: 'Source',
              b: 'La position de la source en mètres, rapportée à la géométrie du lieu en temps réel.',
              reviewed: false },
    },
    'readout-envelope': {
        en: { t: 'Envelope',
              b: 'The drawn plan extent in metres — the speaker bounding box plus its margin.' },
        fr: { t: 'Enveloppe',
              b: 'L’étendue du plan dessiné en mètres — le rectangle englobant des haut-parleurs augmenté de sa marge.',
              reviewed: false },
    },
});

// [selector, key] or [selector, key, wrapperSelector] or
// [selector, key, wrapperSelector, vars].
//
// The selector is the BINDING SITE. It is a selector rather than a
// key-on-the-element attribute because several tips here attach to elements the
// markup does not give an id — the ten `.cell` control groups, the venue rake
// and delay rows, the ping grid — and the wrapper form reaches them from the
// control they already contain. Everything with an id simply uses '#<id>'.
//
// `vars` is static data. A var value is substituted literally by tr() unless it
// is itself an I18N key, in which case it resolves against the CURRENT
// language; O-Octagon's two parameterised entries both take a plain number.
// ============================================================================
// LABELS — the page's own captions, English + French (canon v2, v1.9.0)
//
// Separate from I18N because the two tables answer different questions. I18N
// holds hover-help: a TITLE and a BODY, prose, read only when the pointer
// rests. LABELS holds the words printed ON the control — one string, no body,
// and every one of them occupies a box whose width the layout depends on.
//
// trLabel() falls back to I18N, so a control whose tooltip TITLE already IS its
// label carries ONE key rather than two copies of the same string in two tables
// drifting apart. THE REUSE RULE, settled in Stage F and applied here: a label
// reuses a tooltip key only where the string is right in BOTH languages. An
// English-only match is not enough. #ctl-rolloff was the worked example here
// through v1.11.0, and Stage N retired it: the cell measures 88 px, not the 72
// this paragraph claimed, so the root "Décroissance" (81.59 px) fits and the
// separate label.rolloff entry now carries the SAME string as the tip title.
// The entry is kept rather than folded into the reuse, because the reason to
// keep it is unchanged — a future tooltip edit must not silently resize a cell.
// #ctl-motionPath is the live example: "Trajectoire" is 66.73 px in a 52 px
// box, so label.motionPath keeps its own, shorter caption.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read one. Entries whose French was chosen with WIDTH as a
// constraint say so at the entry — those are the ones a reviewer should
// challenge first.
//
// EVERY `en` VALUE WAS MOVED, NEVER RE-TYPED: each came out of
// scripts/i18n-extract.js's inventory of index.html, and the generated table
// was compared back against the markup before it was pasted.
// ============================================================================

// The eight speaker rows in the Venue table carry six accessible names each,
// and the eight weight cells one each — 56 of this plugin's 89 aria-labels.
// GENERATED FROM A TEMPLATE, not transcribed 56 times, for the reason §21 of
// ui_frontend_check gives about the module registry: a transcribed list is a
// list that goes stale in one row and passes every gate.
//
// They are generated rather than carrying one parameterised key with {n},
// because canon v2's applyI18nAttributes() calls trLabel(key, lang, null) —
// an ATTRIBUTE sweep has no vars argument. A {n} key would render the literal
// "Speaker {n} X metres" into the accessible name, which is worse than English.
// The eight speaker rows in the Venue table carry six accessible names each,
// and the eight weight cells one each — 56 of this plugin's 89 aria-labels.
// GENERATED FROM A TEMPLATE, not transcribed 56 times, for the reason §21 of
// ui_frontend_check gives about the module registry: a transcribed list is a
// list that goes stale in one row and passes every gate.
//
// They are generated rather than carrying ONE parameterised key with {n},
// because canon v2's applyI18nAttributes() calls trLabel(key, lang, null) — an
// ATTRIBUTE sweep has no vars argument. A {n} key would put the literal
// "Speaker {n} X metres" into the accessible name, which is worse than English.
//
// THE GENERATORS LIVE INSIDE THE EXPORT DECLARATION, not above it. assertion 7
// of scripts/check-i18n.js bans every top-level statement here that is not an
// export, and it is right to: this module must never self-execute
// (pattern_module_toplevel_init_tdz). A top-level `const f = (n) => [...]` is
// inert in fact, but "this particular top-level statement is harmless" is the
// judgement the rule exists to refuse.

export const LABELS = Object.freeze({

    // ...spread in, so the whole table is still ONE export declaration.

    // ── Header ──────────────────────────────────────────────────────────────
    // The product name itself is I18N_EXEMPT; only the strapline is localized.
    'label.subtitle':    { en: { t: 'Eight · Channel DBAP' },
                           fr: { t: 'DBAP · Huit canaux', reviewed: false } },
    'aria.screens':      { en: { t: 'Screen' },  fr: { t: 'Écran', reviewed: false } },

    // ── Settings popover ────────────────────────────────────────────────────
    // 'lang-select', 'tips-toggle' and 'settings' come from I18N through
    // trLabel's fallback: their tooltip titles ARE these captions, in both
    // languages. The two BUTTON faces do not — a tip title of "Hover help"
    // is not the word printed on the On button.
    'aria.lang-select':  { en: { t: 'Hover help language' },
                           fr: { t: 'Langue de l’aide au survol', reviewed: false } },
    'label.on':          { en: { t: 'On' },  fr: { t: 'Marche', reviewed: false } },
    'label.off':         { en: { t: 'Off' }, fr: { t: 'Arrêt',  reviewed: false } },

    // ── The three frame banners ─────────────────────────────────────────────
    // The TAGS are small-caps badges in a fixed-width slot, so each French tag
    // was chosen to sit inside the English one's box wherever it could.
    'label.safe-tag':    { en: { t: 'SAFE' },    fr: { t: 'REPLI',    reviewed: false } },
    'label.safe-copy':   { en: { t: 'Stereo fold — not the 8 · channel rig' },
                           fr: { t: 'Repli stéréo — pas le dispositif à 8 · canaux', reviewed: false } },
    'label.map-tag':     { en: { t: 'MAP' },     fr: { t: 'AFFECT.',  reviewed: false } },
    'label.monitor-tag': { en: { t: 'MONITOR' }, fr: { t: 'CONTRÔLE', reviewed: false } },

    // The MAP banner's copy half. C++ sends a REASON CODE, never prose — see
    // MAP_REASON_COPY's replacement in app.js — so each code gets two entries:
    // one bare, one naming the row. TWO KEYS RATHER THAN A {n} THAT IS
    // SOMETIMES ABSENT, because contract §6 authors around the inflection
    // instead of engineering it, and because assertion 13 rejects the ternary
    // that a single key would need at its call site.
    'map.notEightChannels':     { en: { t: 'output set is not 8 channels' },
                                  fr: { t: 'le jeu de sorties n’a pas 8 canaux', reviewed: false } },
    'map.notEightChannels.spk': { en: { t: 'output set is not 8 channels — speaker {n}' },
                                  fr: { t: 'le jeu de sorties n’a pas 8 canaux — haut-parleur {n}', reviewed: false } },
    'map.labelNotInSet':        { en: { t: 'label not in the negotiated set' },
                                  fr: { t: 'libellé absent du jeu négocié', reviewed: false } },
    'map.labelNotInSet.spk':    { en: { t: 'label not in the negotiated set — speaker {n}' },
                                  fr: { t: 'libellé absent du jeu négocié — haut-parleur {n}', reviewed: false } },
    'map.duplicateLabel':       { en: { t: 'duplicate label' },
                                  fr: { t: 'libellé en double', reviewed: false } },
    'map.duplicateLabel.spk':   { en: { t: 'duplicate label — speaker {n}' },
                                  fr: { t: 'libellé en double — haut-parleur {n}', reviewed: false } },

    // The MONITOR banner's copy half and the Venue rail's state line. Both are
    // written by renderMonitor() through setLabel(), so the language sweep owns
    // them: through v1.8.0 they were JS literals, which would have left the
    // banner stranded in English the instant the selector fired mid-fold.
    'monitor.folding':     { en: { t: 'Headphone fold — rig outputs muted' },
                             fr: { t: 'Repli casque — sorties du dispositif coupées', reviewed: false } },
    // WIDTH, and it is the widest string on the whole page. The full form
    // "Désactivé pour l’export hors ligne — l’export est propre" measures
    // 370.8 px against the English 311.2, which puts the banner at 461.7 in a
    // 1100 px header that already carries the title, the tab pair and the gear.
    // Shortened ONCE, here, in the table — never chosen at runtime (D-04).
    'monitor.suppressed':  { en: { t: 'Suppressed for offline render — bounce is clean' },
                             fr: { t: 'Désactivé hors ligne — l’export est propre', reviewed: false } },
    'monitor.unavailable': { en: { t: 'unavailable on this output' },
                             fr: { t: 'indisponible sur cette sortie', reviewed: false } },
    'monitor.armed':       { en: { t: 'armed — suppressed offline' },
                             fr: { t: 'armé — désactivé hors ligne', reviewed: false } },
    'monitor.folding.rail':{ en: { t: 'folding to outputs 1–2' },
                             fr: { t: 'repli vers les sorties 1–2', reviewed: false } },
    'monitor.off':         { en: { t: 'off' }, fr: { t: 'désactivé', reviewed: false } },

    // v1.11.0 — the stereo-bus binaural arm. The banner's copy half and the
    // rail line, through the same renderMonitor() / setLabel() path.
    // WIDTH, as monitor.suppressed: the header already carries the title, the tab pair and
    // the gear, and check-ui-labels measured the long form ("Repli binaural du dispositif à
    // 8 · canaux — bus stéréo", 358 px) crossing the 1100 px frame. Shortened ONCE, here.
    'monitor.binaural':    { en: { t: 'Binaural fold — stereo bus, not the rig' },
                             fr: { t: 'Repli binaural — bus stéréo, pas le dispositif', reviewed: false } },
    'monitor.binaural.rail':{ en: { t: 'binaural on the stereo bus' },
                             fr: { t: 'binaural sur le bus stéréo', reviewed: false } },

    // ── Room screen: the plan, the puck, the weights ────────────────────────
    'aria.puck':         { en: { t: 'Source position' },
                           fr: { t: 'Position de la source', reviewed: false } },
    ...Object.fromEntries([1, 2, 3, 4, 5, 6, 7, 8].flatMap((n) => [
        [`aria.w${n}`, { en: { t: `Weight ${n}` },
                         fr: { t: `Poids ${n}`, reviewed: false } }],
    ])),

    // The speaker→output popover.
    'label.speaker':     { en: { t: 'Speaker' },  fr: { t: 'H.-parleur', reviewed: false } },
    // WIDTH: the caption sits in a 3-part title line inside a 168 px popover;
    // "Haut-parleur" is the correct full form and is used everywhere it fits.
    'label.to-output':   { en: { t: '→ output' }, fr: { t: '→ sortie', reviewed: false } },
    'label.out-pop-note':{ en: { t: 'CoreAudio order · confirm with ping' },
                           fr: { t: 'Ordre CoreAudio · à confirmer par le ping', reviewed: false } },

    // The plan caption. "Plan" is the same word in both languages.
    'label.plan':        { en: { t: 'Plan' },  fr: { t: 'Plan', sameAsEn: true, reviewed: false } },
    'label.field':       { en: { t: 'Field' }, fr: { t: 'Champ', reviewed: false } },

    // ── Controls column ─────────────────────────────────────────────────────
    'aria.gtabs':        { en: { t: 'Position or Motion' },
                           fr: { t: 'Position ou mouvement', reviewed: false } },
    // 'gtab-position', 'gtab-motion', 'srcX', 'srcY', 'srcZ', 'width',
    // 'decorr', 'motionOn', 'motionPath', 'motionSync', 'motionRate',
    // 'motionSize', 'motionRatio', 'motionAngle', 'motionHeight',
    // 'motionPhase', 'motionSeed', 'blur', 'airAmount' and 'outputGain' all
    // reuse their I18N tip title: identical in BOTH languages, and every one
    // fits its cell.
    //
    // These do NOT reuse, and each says why.
    // MEASURED, not inherited (Stage N): the cell's content box is 88 px, and
    // "Décroissance" is 81.59 px in it — 6.41 px of clearance, nowrap,
    // overflow: visible. It carries the same string as the tip title so the
    // caption and the aria-label (which reads that title) are one name; the
    // entry stays separate only so a tooltip edit cannot resize the cell.
    'label.rolloff':     { en: { t: 'Rolloff' },
                           fr: { t: 'Décroissance', reviewed: false,
                                 termNote: 'DBAP distance rolloff — a dB-per-doubling attenuation law, not a filter slope; this page has a real filter (Air) and Pente would name that instead' } },
    // THE REUSE RULE, applied. "Trajectoire" is right for a sentence and
    // measures 66.73 px in a caption track that reaches 52 even after v1.8.0
    // widened it (Stage N re-measured: 66.73, not the 66.7 recorded here —
    // the number held). Stage N moved the TIP TITLE to "Tracé" to match this
    // caption, because the select's aria-label reads that title and a caption
    // the accessible name does not contain is a WCAG 2.5.3 miss. "Tracé" is the better caption in
    // its own right — the plugin's own English internals call this the TRACE
    // (refreshTrace, TRACE_SHAPE_IDS) — so this is a case where the width
    // constraint and the better French agree, which is the only kind of
    // width-driven wording that should not worry a reviewer. Same shape as
    // Stage F's "Suivi tonal", and it is recorded here for the same reason. */
    'label.motionPath':  { en: { t: 'Path' },       fr: { t: 'Tracé', reviewed: false } },
    // The tip title is the full "Hull attenuation"; the cell carries the
    // abbreviation the English markup already used.
    'label.hullAtten':   { en: { t: 'Hull Atten' }, fr: { t: 'Att. env.', reviewed: false } },
    'aria.hullAtten':    { en: { t: 'Hull Atten' },
                           fr: { t: 'Atténuation hors enveloppe', reviewed: false } },

    // Group headings. None of these has a tooltip of its own.
    'label.group.solve':   { en: { t: 'Solve' },    fr: { t: 'Calcul', reviewed: false } },
    'label.group.space':   { en: { t: 'Space' },    fr: { t: 'Espace', reviewed: false } },
    'label.group.output':  { en: { t: 'Output' },   fr: { t: 'Sortie', reviewed: false } },
    'label.group.scenes':  { en: { t: 'Scenes' },   fr: { t: 'Scènes', reviewed: false } },

    // ── Scenes row ──────────────────────────────────────────────────────────
    // UPPERCASE IS AUTHORED, not a text-transform: these seven captions are the
    // literal strings §36 of ui_frontend_check greps for as `>ALL<`, so the
    // French halves are authored uppercase too rather than relying on CSS that
    // is not there.
    'aria.scene-store':  { en: { t: 'Arm store' },     fr: { t: 'Armer la mémorisation', reviewed: false } },
    'aria.scene-row':    { en: { t: 'Weight scenes' }, fr: { t: 'Scènes de poids', reviewed: false } },
    'label.store':       { en: { t: 'STORE' }, fr: { t: 'MÉM.',   reviewed: false } },
    'label.all':         { en: { t: 'ALL' },   fr: { t: 'TOUS',   reviewed: false } },
    'label.front':       { en: { t: 'FRONT' }, fr: { t: 'AVANT',  reviewed: false } },
    'label.rear':        { en: { t: 'REAR' },  fr: { t: 'ARR.',   reviewed: false } },
    'label.left':        { en: { t: 'LEFT' },  fr: { t: 'GAUCHE', reviewed: false } },
    'label.right':       { en: { t: 'RIGHT' }, fr: { t: 'DROITE', reviewed: false } },
    'label.sides':       { en: { t: 'SIDES' }, fr: { t: 'CÔTÉS',  reviewed: false } },

    // ── Elevation strip ─────────────────────────────────────────────────────
    'label.ear':         { en: { t: 'Ear' },    fr: { t: 'Oreille', reviewed: false } },
    'label.source':      { en: { t: 'Source' }, fr: { t: 'Source', sameAsEn: true, reviewed: false } },
    'label.envelope':    { en: { t: 'Envelope' }, fr: { t: 'Enveloppe', reviewed: false } },

    // ── Venue screen: the table head ────────────────────────────────────────
    'label.vcol.label':  { en: { t: 'Label' },   fr: { t: 'Libellé', reviewed: false } },
    'label.vcol.trim':   { en: { t: 'Trim dB' }, fr: { t: 'Corr. dB', reviewed: false } },
    // SPLIT OUT OF ITS <th>. The header cell holds this caption AND the
    // #vcol-delay-unit value span, so the caption needed its own leaf: a keyed
    // element with element children would have its siblings deleted by
    // applyLabel's textContent write. §6 of ui_frontend_check asserts the split.
    'label.vcol.delay':  { en: { t: 'Delay' },
                           fr: { t: 'Retard', reviewed: false,
                                 termNote: 'loudspeaker alignment delay, not the effect — the glossary names this plugin as the case' } },
    'label.vcol.class':  { en: { t: 'Class' },   fr: { t: 'Classe', reviewed: false } },

    ...Object.fromEntries([1, 2, 3, 4, 5, 6, 7, 8].flatMap((n) => [
        [`aria.spk${n}.label`, { en: { t: `Speaker ${n} channel label` },
                                 fr: { t: `Libellé de canal du haut-parleur ${n}`, reviewed: false } }],
        [`aria.spk${n}.x`,     { en: { t: `Speaker ${n} X metres` },
                                 fr: { t: `Haut-parleur ${n}, X en mètres`, reviewed: false } }],
        [`aria.spk${n}.y`,     { en: { t: `Speaker ${n} Y metres` },
                                 fr: { t: `Haut-parleur ${n}, Y en mètres`, reviewed: false } }],
        [`aria.spk${n}.z`,     { en: { t: `Speaker ${n} Z metres` },
                                 fr: { t: `Haut-parleur ${n}, Z en mètres`, reviewed: false } }],
        [`aria.spk${n}.trim`,  { en: { t: `Speaker ${n} trim dB` },
                                 fr: { t: `Haut-parleur ${n}, correction en dB`, reviewed: false } }],
        [`aria.spk${n}.delay`, { en: { t: `Speaker ${n} alignment delay` },
                                 fr: { t: `Haut-parleur ${n}, retard d’alignement`, reviewed: false } }],
    ])),

    // ── Venue screen: the rail ──────────────────────────────────────────────
    'aria.rake-front':   { en: { t: 'Rake front metres' },
                           fr: { t: 'Inclinaison, avant en mètres', reviewed: false } },
    'aria.rake-rear':    { en: { t: 'Rake rear metres' },
                           fr: { t: 'Inclinaison, arrière en mètres', reviewed: false } },
    // The rake row's two keys are Title Case where the scenes row is uppercase,
    // and they are different words in French too, so they are separate keys.
    'label.rake-front':  { en: { t: 'Front' }, fr: { t: 'Avant',   reviewed: false } },
    'label.rake-rear':   { en: { t: 'Rear' },  fr: { t: 'Arrière', reviewed: false } },
    // The tip title is the full "Alignment delay"; the rail caption is short.
    'label.delay':       { en: { t: 'Delay' },
                           fr: { t: 'Retard', reviewed: false,
                                 termNote: 'loudspeaker alignment delay, not the effect — the glossary names this plugin as the case' } },
    'label.derive':      { en: { t: 'Derive' }, fr: { t: 'Calculer', reviewed: false } },
    // The tip title is "Output set"; the caption is the one word.
    'label.set':         { en: { t: 'Set' },    fr: { t: 'Jeu', reviewed: false } },

    'label.group.venue-file':   { en: { t: 'Venue file' },
                                  fr: { t: 'Fichier de lieu', reviewed: false } },
    'label.group.preset':       { en: { t: 'Preset' },  fr: { t: 'Préréglage', reviewed: false } },
    'label.group.output-order': { en: { t: 'Output order' },
                                  fr: { t: 'Ordre des sorties', reviewed: false } },
    'label.group.ping':         { en: { t: 'Ping' },
                                  fr: { t: 'Ping', sameAsEn: true, reviewed: false } },
    'label.group.monitor':      { en: { t: 'Monitor' }, fr: { t: 'Contrôle', reviewed: false } },

    // RE-MEASURED at Stage N, and the v1.9.0 defence above it did not hold. The
    // venue and preset rows are 278 px, each .vbtn is 133 px border-box with a
    // 121 px content box, and "Enregistrer" is 72.83 px in it — 48 px clear.
    // "Charger" is 46.34. Both ROOT terms fit, so both are used: the caption is
    // now literally contained in its own tip title ("Enregistrer le lieu",
    // "Charger un préréglage"), which "Enreg." and "Ouvrir" were not.
    // Three sibling plugins still ship the abbreviations; converging them is a
    // suite decision, recorded in the Stage N report rather than taken here.
    'label.save':        { en: { t: 'Save' },   fr: { t: 'Enregistrer', reviewed: false } },
    'label.load':        { en: { t: 'Load' },   fr: { t: 'Charger', reviewed: false } },
    'label.auto':        { en: { t: 'Auto' },   fr: { t: 'Auto', sameAsEn: true, reviewed: false } },
    'label.headphones':  { en: { t: 'Headphones' }, fr: { t: 'Casque', reviewed: false } },

    // The output-order advisory line, written by venue.js on the completion of
    // applyOutputOrderPreset. Localized through window.__setLabel for the same
    // reason the monitor line is: an English literal here is stranded English.
    'oo.direct':         { en: { t: 'direct 1–8' }, fr: { t: 'direct 1–8', sameAsEn: true, reviewed: false } },
    'oo.roles':          { en: { t: 'roles' },   fr: { t: 'rôles', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// Every visible string the coverage scan finds must be a [data-i18n] element, a
// setLabel() call, or an entry HERE WITH A REASON. A bare skip list would let a
// missed label hide as a deliberate one, which is the failure this plugin's own
// §6 whitelist was already guarding against in a different form.
// ============================================================================

export const I18N_EXEMPT = [
    // The product name, split across the <h1>'s own text node and the italic
    // .title-accent span, so both halves need an entry. The markup authors HAIR
    // SPACES around the en dash (&#8202;), but the scanner collapses every
    // whitespace run to one U+0020 before it classifies, so these entries carry
    // ORDINARY spaces — exempting the as-authored form would silently fail to
    // match and report the product name as an unlocalized label.
    ['O – Octa', 'half of the product name O–Octagon — a product name is never translated'],
    ['gon',      'the italic half of the product name O–Octagon, in .title-accent'],

    // Unit symbols. D-03: a readout and its unit are language-neutral, and the
    // ms/m pair is a TOGGLE whose two faces are the symbols themselves.
    ['ms', 'unit symbol, language-neutral (D-03) — the Delay column head and the ms/m toggle'],
    ['m',  'unit symbol, language-neutral (D-03) — the metres face of the ms/m toggle'],

    // Endonyms.
    ['English',  'endonym — a language name is never translated'],
    ['Français', 'endonym — a language name is never translated'],
];

export const TIP_BINDINGS = [
    ['#tab-room',              'tab-room'],
    ['#tab-venue',             'tab-venue'],

    ['#gear-btn',              'settings'],
    ['#lang-select',           'lang-select'],
    ['#tips-toggle',           'tips-toggle'],

    ['#safe-banner',           'safe-banner'],
    ['#map-banner',            'map-banner'],
    ['#monitor-banner',        'monitor-banner'],
    ['#monitor-toggle',        'monitor-toggle'],

    ['#puck',                  'puck'],

    // The ten column controls. The tip belongs on the whole `.cell`, so the
    // label and the value readout raise it too, not just the slider track.
    ['#ctl-srcX',              'srcX',        '.cell'],
    ['#ctl-srcY',              'srcY',        '.cell'],
    ['#ctl-srcZ',              'srcZ',        '.cell'],
    ['#ctl-width',             'width',       '.cell'],
    ['#ctl-decorr',            'decorr',      '.cell'],

    // v1.8.0 — the Position | Motion tab pair and the ten motion controls.
    ['#gtab-position',         'gtab-position'],
    ['#gtab-motion',           'gtab-motion'],
    ['#ctl-motionOn',          'motionOn',     '.cell'],
    ['#ctl-motionPath',        'motionPath',   '.cell'],
    ['#ctl-motionSync',        'motionSync',   '.cell'],
    ['#ctl-motionRate',        'motionRate',   '.cell'],
    ['#ctl-motionSize',        'motionSize',   '.cell'],
    ['#ctl-motionRatio',       'motionRatio',  '.cell'],
    ['#ctl-motionAngle',       'motionAngle',  '.cell'],
    ['#ctl-motionHeight',      'motionHeight', '.cell'],
    ['#ctl-motionPhase',       'motionPhase',  '.cell'],
    ['#ctl-motionSeed',        'motionSeed',   '.cell'],
    ['#ctl-rolloff',           'rolloff',     '.cell'],
    ['#ctl-blur',              'blur',        '.cell'],
    ['#ctl-hullAtten',         'hullAtten',   '.cell'],
    ['#ctl-airAmount',         'airAmount',   '.cell'],
    ['#ctl-outputGain',        'outputGain',  '.cell'],

    ['#btn-scene-store',       'scene-store'],
    ['#scene-ALL',             'scene-all'],
    ['#scene-FRONT',           'scene-front'],
    ['#scene-REAR',            'scene-rear'],
    ['#scene-LEFT',            'scene-left'],
    ['#scene-RIGHT',           'scene-right'],
    ['#scene-SIDES',           'scene-sides'],

    ['#elev-stage',            'elevation'],

    ['.venue-rake',            'rake'],
    ['.venue-delay',           'delay'],
    ['.vunit-group',           'delay-unit'],
    ['#btn-delay-derive',      'delay-derive'],
    ['.rail-line',             'output-set'],
    ['#btn-venue-save',        'venue-save'],
    ['#btn-venue-load',        'venue-load'],
    ['#preset-list',           'preset-list'],
    ['#btn-preset-save',       'preset-save'],
    ['#btn-preset-load',       'preset-load'],
    ['#btn-oo-direct',         'oo-direct'],
    ['#btn-oo-roles',          'oo-roles'],
    ['.ping-grid',             'ping-grid'],
    ['#btn-ping-auto',         'ping-auto'],
    ['#btn-ping-stop',         'ping-stop'],

    ['#readout-label-source',   'readout-source'],
    ['#readout-label-envelope', 'readout-envelope'],

    // The eight weight cells and the four user-scene slots, expanded from one
    // parameterised entry each rather than transcribed twelve times.
    ...[1, 2, 3, 4, 5, 6, 7, 8].map(n => ['#wcell-' + n, 'weight', null, { n: String(n) }]),
    ...[1, 2, 3, 4].map(n => ['#scene-slot' + n, 'scene-slot', null, { n: String(n) }]),
];

export function tr(key, lang, vars) {
    const entry = I18N[key];
    if (!entry) { console.warn(`i18n: missing key ${key}`); return { t: key, b: '' }; }
    const s = entry[lang] || entry.en;

    // A var VALUE that is itself an I18N key resolves to that key's localized
    // title; anything else is used literally. O-Octagon only needs the literal
    // arm today, but the resolving arm is what lets a plugin compose a
    // localized name into a tip without pinning TIP_BINDINGS — which is static
    // data evaluated once — to the load-time language.
    const resolve = (v) => {
        const nested = I18N[v];
        return nested ? String((nested[lang] || nested.en).t) : String(v);
    };

    const sub = (v) => vars
        ? String(v).replace(/\{(\w+)\}/g, (m, n) => (n in vars ? resolve(vars[n]) : m))
        : String(v);

    return { t: sub(s.t), b: sub(s.b) };
}
