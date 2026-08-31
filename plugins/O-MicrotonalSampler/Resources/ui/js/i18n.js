/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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
// i18n.js — O-MicrotonalSampler UI copy, English + French (v1.25.0, canon v2)
//
// LABELS AND HOVER-HELP. v1.24.0 shipped labels only: this plugin had no
// data-tip renderer and no tooltip copy, only native title= attributes — five
// in the markup and ELEVEN written from JS — which contract §4 deletes rather
// than localizes. v1.25.0 (Stage M) adds the hover-help this file's v1.24.0
// header deferred: 20 bodied `tip.*` entries and 21 TIP_BINDINGS rows, plus the
// renderer that paints them (setupTooltips in js/sampler-app.js, ported from
// O-simpleFM) and the gate that sees them (tests/ui_tip_render_check.js).
//
// THE 51 EMPTY-BODY ENTRIES BELOW ARE UNCHANGED AND ARE NOT TOOLTIPS. They are
// toasts, dialog copy and composed accessible names reached through trLabel(),
// and the K4 decision put them in I18N with `b: ''` precisely so they would not
// demand a binding. Adding a body to one, or binding one, would make it a
// hover-help entry it is not.
//
// AND THAT DECISION NOW COSTS SOMETHING VISIBLE. check-i18n assertion 2 reads
// "0 tips bound is a state, not a gap" only while NO I18N entry carries a body.
// The first `tip.*` body below flips it: from here on every bodied entry must
// appear in TIP_BINDINGS or it fails as ORPHANED. So the orphan check now runs
// against a table where 51 entries are legitimately body-less — they pass
// because `b: ''` is empty, not because anything special was said about them.
// An entry that gains a body by accident, in either language, becomes an orphan
// on the next run.
//
// An ES module that EXPORTS ONLY. It must never self-execute: a bare top-level
// statement here throws out of module evaluation and takes every later
// initializer on the page with it (pattern_module_toplevel_init_tdz).
// Assertion 7 enforces it.
//
// SERVED ROOT IS Resources/ui, read from CMakeLists.txt before a byte was
// written here. Source/ui/public/ exists in this plugin but holds ONLY the
// shared drop-streaming module copy and is NOT embedded — CMakeLists.txt:66
// says so in as many words. Nothing here goes there.
//
// FOUR PLACES, ONE COMMIT: this file on disk, the SOURCES list in
// CMakeLists.txt, a getResource() branch in PluginEditor.cpp, and the import in
// js/sampler-app.js. Miss one and the page 404s at runtime and presents as a
// dead panel with no other symptom (assertion 8).
//
// FILENAME: no hyphen. juce_add_binary_data STRIPS hyphens rather than
// converting them to underscores (critical_binary_data_strips_hyphens), so a
// second file named i18n-fr.js would have to be reached as i18nfr_js. One
// combined file for both languages sidesteps the question.
//
// NO MARKUP. This table is data, never HTML. Assertion 9 rejects any innerHTML
// reference here and any string literal containing an opening angle bracket.
//
// ── THE TWO TABLES, AND WHY A STRING IS IN ONE RATHER THAN THE OTHER ───────
//
// LABELS holds every key a gate can SEE being referenced: a markup attribute,
// a plain-literal setLabel() call, a literal `.dataset.i18n* =` write, or a key
// declared inside markup a module injects with innerHTML. Assertion 15 sweeps
// LABELS for dead keys against exactly those four shapes.
//
// I18N holds every key reached ONLY through a trLabel() call — the toasts, the
// confirmation-dialog arguments, and the composed accessible names the grid and
// the technique strip build per element. A trLabel() call is none of the four
// shapes, so a LABELS key used that way reports DEAD while being read on every
// language change. I18N keys sit outside that sweep and, carrying no body, do
// not demand a TIP_BINDINGS entry either. This is O-Comp's shape from Stage K3,
// adopted as the standard by the K4 addendum for canvas strings; the same
// argument applies verbatim to a composed aria-label.
//
// ── WHERE THE COPY COMES FROM ─────────────────────────────────────────────
//
// 146 LABEL rows from scripts/i18n-extract.js over index.html, plus ~34 hand-
// enumerated strings from js/tuning-panel.js — which the extractor drops BY
// FILENAME (i18n-extract.js:442) with no ownership test. That skip is right for
// O-Wind, which consumes the module file by reference; it is wrong here.
// CMakeLists.txt embeds Resources/ui/js/tuning-panel.js, this plugin's own
// 317-line-diverged copy, and there is no dependencies.json listing the module,
// so /module-upgrade will not revert the edit. modules/ is untouched.
//
// ALL FRENCH IS MACHINE-DRAFTED AND FLAGGED `reviewed: false`. No native
// speaker has read it. `node scripts/check-i18n.js` prints the worklist.
//
// The tuning-panel French is taken VERBATIM from O-Bells v4.2.0 and
// O-IntonationPad v2.9.0 wherever the string is the same, because two
// hand-copies of one panel disagreeing about the French would be worse than
// either translation alone.
// ============================================================================

export const LANGUAGES = ['en', 'fr'];

// ── I18N — composed and JS-written strings, EMPTY BODIES ───────────────────
//
// Every entry here is reached through trLabel() from sampler-app.js. `b` is ''
// on both languages: these are not tooltips, and assertion 2 reads a non-empty
// body as an orphaned hover-help entry.
//
// CONTRACT §6 IS WHAT SHAPES THE COUNTED ONES. v1.23.10 carried the repo's only
// inline English pluralization — six sites spelling `file${n === 1 ? '' : 's'}`,
// `sample${...}`, `cell${...}` and `1 variant` / `${n} variants`. French
// pluralizes 0 as SINGULAR where English pluralizes it, so a mechanical port of
// the ternary is wrong at n=0 before it is wrong anywhere else. None of them is
// ported. The copy is authored around the inflection instead: the count sits
// after a colon, beside a noun phrase held in its plural form as a category
// header, which reads correctly at 0, 1 and n in both languages. No plural
// engine, and nothing for assertion 13 to reject.
export const I18N = Object.freeze({

    // ── toasts ────────────────────────────────────────────────────────────
    'toast.filesSkipped': {
        en: { t: 'Files skipped: {n}', b: '' },
        fr: { t: 'Fichiers ignorés : {n}', b: '', reviewed: false } },
    'toast.nothingToDelete': {
        en: { t: 'Nothing to delete on that cell.', b: '' },
        fr: { t: 'Rien à supprimer dans cette case.', b: '', reviewed: false } },
    'toast.layerCleared': {
        en: { t: 'Layer {mark} · samples removed: {n}', b: '' },
        fr: { t: 'Couche {mark} · échantillons supprimés : {n}', b: '', reviewed: false } },
    'toast.layerAlreadyEmpty': {
        en: { t: 'Layer {mark} was already empty.', b: '' },
        fr: { t: 'La couche {mark} était déjà vide.', b: '', reviewed: false } },
    'toast.loopPointsApplied': {
        en: { t: 'Loop points applied · samples updated: {n}', b: '' },
        fr: { t: 'Points de boucle appliqués · échantillons mis à jour : {n}', b: '', reviewed: false } },
    'toast.noLoopableSamples': {
        en: { t: 'No loopable samples to update.', b: '' },
        fr: { t: 'Aucun échantillon bouclable à mettre à jour.', b: '', reviewed: false } },
    'toast.loopApplyNote': {
        en: { t: 'New loop points apply to next note-on.', b: '' },
        fr: { t: 'Les nouveaux points de boucle prennent effet à la prochaine note.', b: '', reviewed: false } },
    'toast.resizeWider': {
        en: { t: 'Resize wider to use the loop editor.', b: '' },
        fr: { t: 'Élargissez la fenêtre pour utiliser l’éditeur de boucle.', b: '', reviewed: false } },
    'toast.folderLoadFailed': {
        en: { t: 'Folder load failed', b: '' },
        fr: { t: 'Échec du chargement du dossier', b: '', reviewed: false } },
    'toast.waveformUnavailable': {
        en: { t: 'Unable to load waveform for this cell.', b: '' },
        fr: { t: 'Impossible de charger la forme d’onde de cette case.', b: '', reviewed: false } },
    'toast.presetSaved': {
        en: { t: 'Preset saved', b: '' },
        fr: { t: 'Préréglage enregistré', b: '', reviewed: false } },
    'toast.presetSaveFailed': {
        en: { t: 'Save preset failed', b: '' },
        fr: { t: 'Échec de l’enregistrement du préréglage', b: '', reviewed: false } },
    'toast.presetLoaded': {
        en: { t: 'Preset loaded', b: '' },
        fr: { t: 'Préréglage chargé', b: '', reviewed: false } },
    'toast.presetLoadFailed': {
        en: { t: 'Load preset failed', b: '' },
        fr: { t: 'Échec du chargement du préréglage', b: '', reviewed: false } },
    'toast.folderLocated': {
        en: { t: 'Folder located — loading…', b: '' },
        fr: { t: 'Dossier localisé — chargement…', b: '', reviewed: false } },
    'toast.locateFolderFailed': {
        en: { t: 'Locate folder failed', b: '' },
        fr: { t: 'Échec de la localisation du dossier', b: '', reviewed: false } },
    'toast.embedDialogMissing': {
        en: { t: 'Internal UI error: confirmation dialog unavailable — embed cancelled.', b: '' },
        fr: { t: 'Erreur interne : boîte de confirmation indisponible — intégration annulée.', b: '', reviewed: false } },
    'toast.rrDialogMissing': {
        en: { t: 'Internal UI error: round-robin confirmation dialog unavailable — load cancelled.', b: '' },
        fr: { t: 'Erreur interne : boîte de confirmation round-robin indisponible — chargement annulé.', b: '', reviewed: false } },
    'toast.techniquePresetApplied': {
        en: { t: 'Applied {family} technique names', b: '' },
        fr: { t: 'Noms de techniques « {family} » appliqués', b: '', reviewed: false } },

    // ── confirmation-dialog arguments ─────────────────────────────────────
    // showConfirmDialog() takes finished strings, not keys, so these resolve
    // through trLabel() at the call site.
    'msg.deleteSampleTitle': {
        en: { t: 'Delete this sample?', b: '' },
        fr: { t: 'Supprimer cet échantillon ?', b: '', reviewed: false } },
    'msg.deleteSampleBody': {
        en: { t: 'Remove the sample on {note}, velocity layer {mark}{tech}.', b: '' },
        fr: { t: 'Retirer l’échantillon sur {note}, couche de vélocité {mark}{tech}.', b: '', reviewed: false } },
    'msg.deleteBtn': {
        en: { t: 'Delete', b: '' },
        fr: { t: 'Supprimer', b: '', reviewed: false } },
    'msg.clearLayerTitle': {
        en: { t: 'Clear velocity layer {mark}?', b: '' },
        fr: { t: 'Vider la couche de vélocité {mark} ?', b: '', reviewed: false } },
    'msg.clearLayerBody': {
        en: { t: 'Remove every sample in velocity layer {mark}, across all techniques. This cannot be undone.', b: '' },
        fr: { t: 'Retirer tous les échantillons de la couche de vélocité {mark}, pour toutes les techniques. Action irréversible.', b: '', reviewed: false } },
    'msg.clearLayerBtn': {
        en: { t: 'Clear layer', b: '' },
        fr: { t: 'Vider la couche', b: '', reviewed: false } },
    'msg.clearAllTitle': {
        en: { t: 'Clear all samples?', b: '' },
        fr: { t: 'Vider tous les échantillons ?', b: '', reviewed: false } },
    'msg.clearAllBody': {
        en: { t: 'All loaded samples will be removed from the sample map. Active notes will finish playing, but new note-ons will produce silence until samples are loaded again. This cannot be undone.', b: '' },
        fr: { t: 'Tous les échantillons chargés seront retirés de la carte. Les notes en cours iront à leur terme, mais les nouvelles notes resteront silencieuses jusqu’au prochain chargement. Action irréversible.', b: '', reviewed: false } },
    'msg.clearBtn': {
        en: { t: 'Clear', b: '' },
        fr: { t: 'Vider', b: '', reviewed: false } },

    // The folder-load explain line: EIGHT keyed faces, not four ternaries.
    // The key is the branch, so nothing inflects inside a string.
    'msg.floAppendForced': {
        en: { t: 'Add samples to {layer}, ignoring filename velocity tokens.', b: '' },
        fr: { t: 'Ajouter les échantillons à {layer}, en ignorant les jetons de vélocité du nom de fichier.', b: '', reviewed: false } },
    'msg.floAppendTokens': {
        en: { t: 'Add samples; filename tokens (v1–v4, p/mp/mf/f) decide layer.', b: '' },
        fr: { t: 'Ajouter les échantillons ; les jetons du nom de fichier (v1–v4, p/mp/mf/f) décident de la couche.', b: '', reviewed: false } },
    'msg.floReplaceLayerForced': {
        en: { t: 'Clear {layer} and add the new samples there.', b: '' },
        fr: { t: 'Vider {layer} et y ajouter les nouveaux échantillons.', b: '', reviewed: false } },
    'msg.floReplaceLayerTokens': {
        en: { t: 'Clear {layer}; filename tokens decide where new samples land.', b: '' },
        fr: { t: 'Vider {layer} ; les jetons du nom de fichier décident où atterrissent les nouveaux échantillons.', b: '', reviewed: false } },
    'msg.floReplaceAllForced': {
        en: { t: 'Replace existing samples; new ones land on {layer}.', b: '' },
        fr: { t: 'Remplacer les échantillons existants ; les nouveaux atterrissent sur {layer}.', b: '', reviewed: false } },
    'msg.floReplaceAllTokens': {
        en: { t: 'Replace existing samples; filename tokens decide layer.', b: '' },
        fr: { t: 'Remplacer les échantillons existants ; les jetons du nom de fichier décident de la couche.', b: '', reviewed: false } },
    'msg.floMergeRrForced': {
        en: { t: 'Layer onto {layer}: collisions become round-robin variants (cap 64 per cell).', b: '' },
        fr: { t: 'Superposer sur {layer} : les collisions deviennent des variantes round-robin (max. 64 par case).', b: '', reviewed: false } },
    'msg.floMergeRrTokens': {
        en: { t: 'Layer existing notes: collisions become round-robin variants. Filename tokens decide layer.', b: '' },
        fr: { t: 'Superposer les notes existantes : les collisions deviennent des variantes round-robin. Les jetons du nom de fichier décident de la couche.', b: '', reviewed: false } },
    'msg.floTechniqueForced': {
        en: { t: 'Technique forced to "{name}".', b: '' },
        fr: { t: 'Technique forcée sur « {name} ».', b: '', reviewed: false } },

    // ── composed ACCESSIBLE NAMES ─────────────────────────────────────────
    //
    // Every one of these replaces a native title= that v1.23.10 wrote from JS
    // and contract §4 deletes. They are composed from per-element data, and
    // applyI18nAttributes() resolves a data-i18n-aria key with NO vars, so a
    // keyed attribute cannot express them: they are built by their own
    // renderer and re-built by refreshComposedUi() on a language change, which
    // is the single re-render path §3 asks for.
    'aria.cellVel': {
        en: { t: 'Vel {mark} ({range})', b: '' },
        fr: { t: 'Vél. {mark} ({range})', b: '', reviewed: false } },
    'aria.cellTech': {
        en: { t: 'tech: {name}', b: '' },
        fr: { t: 'technique : {name}', b: '', reviewed: false } },
    'aria.cellVariants': {
        en: { t: 'variants: {n}', b: '' },
        fr: { t: 'variantes : {n}', b: '', reviewed: false } },
    'aria.velLabel': {
        en: { t: 'Dynamic {mark} (layer {layer}): MIDI velocity {range} — right-click to clear this layer', b: '' },
        fr: { t: 'Nuance {mark} (couche {layer}) : vélocité MIDI {range} — clic droit pour vider cette couche', b: '', reviewed: false } },
    'aria.switchToVariant': {
        en: { t: 'Switch to variant {n}', b: '' },
        fr: { t: 'Passer à la variante {n}', b: '', reviewed: false } },
    'aria.techTabLoaded': {
        en: { t: 'Technique {i}: {name} — cells loaded: {n}  (right-click to rename)', b: '' },
        fr: { t: 'Technique {i} : {name} — cases chargées : {n}  (clic droit pour renommer)', b: '', reviewed: false } },
    'aria.techTabEmpty': {
        en: { t: 'Technique {i}: {name} — empty  (right-click to rename)', b: '' },
        fr: { t: 'Technique {i} : {name} — vide  (clic droit pour renommer)', b: '', reviewed: false } },
    'aria.trimWholeTechnique': {
        en: { t: 'Trim the whole "{name}" technique (all layers)', b: '' },
        fr: { t: 'Ajuster toute la technique « {name} » (toutes les couches)', b: '', reviewed: false } },
    'aria.slotN': {
        en: { t: 'slot {n}', b: '' },
        fr: { t: 'emplacement {n}', b: '', reviewed: false } },
    'aria.midiN': {
        en: { t: 'MIDI {n}', b: '' },
        fr: { t: 'MIDI {n}', b: '', reviewed: false, sameAsEn: true } },
    'aria.loaded': {
        en: { t: 'Loaded', b: '' },
        fr: { t: 'Chargé', b: '', reviewed: false } },
    'aria.unnamed': {
        en: { t: '(unnamed)', b: '' },
        fr: { t: '(sans nom)', b: '', reviewed: false } },
    'aria.unknown': {
        en: { t: '(unknown)', b: '' },
        fr: { t: '(inconnu)', b: '', reviewed: false } },
    'aria.emptyPath': {
        en: { t: '(empty path)', b: '' },
        fr: { t: '(chemin vide)', b: '', reviewed: false } },

    // ── HOVER-HELP, v1.25.0 (Stage M) — THE ONLY BODIED ENTRIES IN THIS FILE ──
    //
    // 18 parameter tips + 2 chrome tips = 20. Each is bound in TIP_BINDINGS at
    // the foot of this file; an unbound bodied entry fails assertion 2 as an
    // ORPHAN, which is the check that makes "the copy exists" mean "the copy is
    // reachable".
    //
    // THE PARAMETER INVENTORY IS .planning/params.tsv — a RUNTIME walk of
    // AudioProcessor::getParameters(), not a regex over createParameterLayout().
    // 19 parameters. EIGHTEEN get a tip. `rr_mode` (Round-Robin Mode: Cycle /
    // Random No-Repeat / Random) HAS NO CONTROL ON THIS PAGE in any version —
    // it is automatable and host-reachable and page-unreachable, so a body for
    // it could not be bound and would fail as an orphan. A control was NOT added
    // to satisfy the count: that is a feature change with a geometry cost on a
    // control strip whose knobs are already flex: 1 1 0 at min-width 56px.
    //
    // TITLES ARE THE PARAMETER'S FULL NAME, NOT THE PAGE'S CAPTION, and this is
    // a deliberate departure from the Stage M brief's "the caption wins" rule.
    // That rule is for a caption that DISAGREES with the parameter name. Here
    // five captions — Poly, Vel-XF, Expr, Dyn Rng, Out Gain — are truncations
    // forced by a 56px column, and expanding a truncation the user cannot read
    // is the single most useful thing a 260px tooltip can do. Where the caption
    // and the name agree (Attack, Decay, Sustain, Release) nothing changes.
    //
    // RANGES. Ten parameters carry a real `label` in the dump (s, %, dB) and are
    // quoted as dumped. Three do not — sustain, polyphony and velocity_crossfade
    // — and their ranges are phrased from the page's OWN formatter, KNOB_FORMATS
    // in js/sampler-app.js: sustain at :504-505 and velocity_crossfade at
    // :510-511 render `v.toFixed(2)` with an EMPTY suffix, so the range is a bare
    // 0.00 to 1.00 and no unit is invented; polyphony at :508-509 renders
    // `Math.round(v)` with an empty suffix, so "voices" is the page's own word
    // for what it counts, taken from the caption Poly rather than from a unit
    // string. The six technique / keyswitch / CC / PC parameters have empty
    // labels too, and their ranges are the min/max attributes on their own
    // number inputs in index.html plus the option words from the dump's
    // textAtMin / textAtMax.
    //
    // OPTION STRINGS INSIDE A BODY. `Velocity` and `CC Crossfade` are
    // dynamics_mode AudioParameterChoice options, byte-identical, and are
    // I18N_EXEMPT on the page under D-01 arm 1 so the segmented toggle and the
    // host automation lane keep saying the same word. They appear UNTRANSLATED
    // inside the French bodies as well, deliberately: the sentence around them
    // is French, but the word the reader has to go and find on screen is the
    // word that is on screen. The two rules do not conflict — the option is
    // data, the sentence is prose.
    //
    // D-03 AND THE DECIMAL SEPARATOR. A tooltip body is PROSE and takes French
    // convention: decimal COMMA, a space before %, U+2212 for the minus. The
    // READOUT keeps its point, because D-03 exempts the readout NODE and that
    // has not moved. They differ on purpose — the readout is a machine-formatted
    // value, the body is a sentence. Settled by the developer 2026-08-30 after
    // M1 split on it three ways.
    //
    // NOTE NAMES stay in letter notation in both languages, the same verdict the
    // tuning panel's twelve note names carry above: the C++ TuningEngine, the
    // .scl / .kbm formats and the exported HTML all speak the letter system.

    'tip.attack': {
        en: { t: 'Attack',
              b: 'Fade-in time from note-on to full level. Raise it to soften a percussive sample onset; leave it near zero to keep the recording’s own transient. 0 to 10 s.' },
        fr: { t: 'Attaque',
              b: 'Temps de montée entre l’enfoncement de la note et le niveau maximal. Augmentez-le pour adoucir une attaque percussive ; laissez-le près de zéro pour conserver le transitoire de l’enregistrement. 0 à 10 s.',
              reviewed: false } },
    'tip.decay': {
        en: { t: 'Decay',
              b: 'Time to fall from the peak to the sustain level, once the attack has finished. It only bites when Sustain sits below 1.00. 0 to 10 s.' },
        fr: { t: 'Déclin',
              b: 'Temps de descente du sommet vers le niveau de maintien, une fois l’attaque terminée. Il n’agit que si le maintien est inférieur à 1,00. 0 à 10 s.',
              reviewed: false } },
    'tip.sustain': {
        en: { t: 'Sustain',
              b: 'Level a held note settles at after the decay, as a fraction of the sample’s own level. At 1.00 the sample plays untouched and Decay does nothing. 0.00 to 1.00.' },
        fr: { t: 'Maintien',
              b: 'Niveau auquel se stabilise une note tenue après le déclin, en fraction du niveau propre de l’échantillon. À 1,00 l’échantillon est joué tel quel et le déclin n’a aucun effet. 0,00 à 1,00.',
              reviewed: false } },
    'tip.release': {
        en: { t: 'Release',
              b: 'Fade-out time after note-off. A long value lets a hall tail ring on; a short one cuts the note clean. 0 to 10 s.' },
        fr: { t: 'Extinction',
              b: 'Temps de descente après le relâchement de la note. Une valeur longue laisse la queue de salle résonner ; une valeur courte coupe la note net. 0 à 10 s.',
              reviewed: false } },
    'tip.polyphony': {
        en: { t: 'Polyphony',
              b: 'Greatest number of notes that may sound at once. Lower it to cap CPU on a large library; past the limit the oldest voice is stolen. 1 to 16 voices.' },
        fr: { t: 'Polyphonie',
              b: 'Nombre maximal de notes pouvant sonner en même temps. Abaissez-le pour limiter le processeur sur une grande banque ; au-delà de la limite, la voix la plus ancienne est remplacée. 1 à 16 voix.',
              reviewed: false } },
    'tip.velocityCrossfade': {
        en: { t: 'Velocity Crossfade',
              b: 'How far neighbouring velocity layers blend into one another instead of switching abruptly. At 0.00 each layer starts exactly where the one below it stops. 0.00 to 1.00.' },
        fr: { t: 'Fondu de vélocité',
              b: 'Degré de fondu entre couches de vélocité voisines, au lieu d’un basculement net. À 0,00 chaque couche commence exactement là où s’arrête la précédente. 0,00 à 1,00.',
              reviewed: false } },
    'tip.expression': {
        en: { t: 'Expression',
              b: 'Overall playing level, driven live by MIDI CC 11. What it does depends on the Dynamics mode: a post-mix volume trim under Velocity, a layer morph under CC Crossfade. 0 to 100 %.' },
        fr: { t: 'Expression',
              b: 'Niveau de jeu global, piloté en direct par le CC MIDI 11. Son effet dépend du mode de dynamique : un simple réglage de volume après mixage en Velocity, un fondu entre couches en CC Crossfade. 0 à 100 %.',
              reviewed: false } },
    'tip.dynamicsMode': {
        en: { t: 'Dynamics Mode',
              b: 'Chooses what MIDI CC 11 controls. Velocity: note-on velocity picks the layer and CC 11 is only a volume trim. CC Crossfade: CC 11 morphs across every velocity layer mid-note, changing timbre as well as loudness.' },
        fr: { t: 'Mode de dynamique',
              b: 'Détermine ce que pilote le CC MIDI 11. Velocity : la vélocité choisit la couche et le CC 11 n’est qu’un réglage de volume. CC Crossfade : le CC 11 fond toutes les couches de vélocité en cours de note, changeant le timbre autant que l’intensité.',
              reviewed: false } },
    'tip.dynamicRange': {
        en: { t: 'Dynamic Range',
              b: 'How much quieter the softest layer sits below the loudest under CC Crossfade. Widen it for an exposed orchestral line, narrow it for a mix that must stay present throughout. 0.0 to 40.0 dB.' },
        fr: { t: 'Plage dynamique',
              b: 'Écart de niveau entre la couche la plus douce et la plus forte, en mode CC Crossfade. Élargissez-la pour une ligne orchestrale exposée, resserrez-la pour un mixage qui doit rester présent de bout en bout. 0,0 à 40,0 dB.',
              reviewed: false } },
    'tip.outputGain': {
        en: { t: 'Output Gain',
              b: 'Final level of the whole instrument, applied after every technique and layer trim. Use it to seat this instance against the rest of the mix. −24 to +24 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Niveau final de tout l’instrument, appliqué après chaque ajustement de technique et de couche. Utilisez-le pour caler cette instance sur le reste du mixage. −24 à +24 dB.',
              reviewed: false } },
    'tip.techniqueCount': {
        en: { t: 'Technique Count',
              b: 'How many playing-technique slots this instrument exposes. Add one for each articulation you have samples for; removing a slot hides its cells rather than deleting them. 1 to 8 slots.' },
        fr: { t: 'Nombre de techniques',
              b: 'Nombre d’emplacements de technique de jeu proposés par l’instrument. Ajoutez-en un par articulation dont vous avez des échantillons ; en retirer un masque ses cases sans les supprimer. 1 à 8 emplacements.',
              reviewed: false } },
    'tip.techniqueSelect': {
        en: { t: 'Technique Select',
              b: 'The technique slot currently sounding, and the one the sample map and the trims are editing. Click a tab to switch, right-click a tab to rename it. Slots 1 to 8.' },
        fr: { t: 'Technique active',
              b: 'L’emplacement de technique en cours de lecture, celui que la carte d’échantillons et les ajustements modifient. Cliquez sur un onglet pour changer, clic droit pour le renommer. Emplacements 1 à 8.',
              reviewed: false } },
    'tip.ksEnabled': {
        en: { t: 'Keyswitch Enabled',
              b: 'Turns keyswitching on: a note-on inside the range beside it selects a technique instead of sounding. It is off by default, so no note is ever swallowed until you ask for it. Off or On.' },
        fr: { t: 'Commutation par touche',
              b: 'Active la commutation par touche : une note jouée dans la plage voisine choisit une technique au lieu de sonner. Elle est désactivée par défaut, afin qu’aucune note ne soit absorbée sans votre accord. Arrêt ou Marche.',
              reviewed: false } },
    'tip.ksLowNote': {
        en: { t: 'Keyswitch Low Note',
              b: 'Bottom of the keyswitch range, as a MIDI note number. Keep it below the register you actually play — the default, MIDI 0, is well out of the way. 0 to 127.' },
        fr: { t: 'Note basse de commutation',
              b: 'Limite inférieure de la plage de commutation, en numéro de note MIDI. Gardez-la sous le registre que vous jouez réellement ; la valeur par défaut, MIDI 0, est largement à l’écart. 0 à 127.',
              reviewed: false } },
    'tip.ksHighNote': {
        en: { t: 'Keyswitch High Note',
              b: 'Top of the keyswitch range. Leave one semitone per slot above the low note, or two keys collapse onto the same technique. 0 to 127.' },
        fr: { t: 'Note haute de commutation',
              b: 'Limite supérieure de la plage de commutation. Laissez un demi-ton par emplacement au-dessus de la note basse, sans quoi deux touches visent la même technique. 0 à 127.',
              reviewed: false } },
    'tip.ccSelectEnabled': {
        en: { t: 'CC Select Enabled',
              b: 'Lets a MIDI controller choose the technique from its value, through the table below. Keyswitching still takes precedence wherever both are active. Off or On.' },
        fr: { t: 'Sélection par CC',
              b: 'Permet à un contrôleur MIDI de choisir la technique selon sa valeur, d’après le tableau ci-dessous. La commutation par touche reste prioritaire là où les deux sont actives. Arrêt ou Marche.',
              reviewed: false } },
    'tip.ccNumber': {
        en: { t: 'CC Number',
              b: 'Which MIDI controller drives technique selection. CC 32 is the default; avoid CC 11, which Expression already listens to. 0 to 119.' },
        fr: { t: 'Numéro de CC',
              b: 'Contrôleur MIDI qui pilote la sélection de technique. Le CC 32 est la valeur par défaut ; évitez le CC 11, déjà écouté par l’expression. 0 à 119.',
              reviewed: false } },
    'tip.pcEnabled': {
        en: { t: 'Program Change Enabled',
              b: 'Lets a MIDI program change select the technique, through the table below. It comes last in precedence, after keyswitching and CC. Off or On.' },
        fr: { t: 'Changement de programme',
              b: 'Permet à un changement de programme MIDI de choisir la technique, d’après le tableau ci-dessous. Il vient en dernier, après la commutation par touche et le CC. Arrêt ou Marche.',
              reviewed: false } },

    // ── the two chrome tips ────────────────────────────────────────────────
    // The gear tip is what tells a user hover-help exists at all, so it must
    // describe ONLY what this popover actually holds. It holds the language
    // selector and nothing else — no hover-help toggle, which O-Tapestop's
    // wording promises and this plugin does not have. A tip that lies is worse
    // than no tip.
    'tip.gear': {
        en: { t: 'Settings',
              b: 'Opens the settings panel. It holds the interface language and nothing else. Hover any control on this page for the same kind of help you are reading now.' },
        fr: { t: 'Réglages',
              b: 'Ouvre le panneau de réglages. Il contient la langue de l’interface, et rien d’autre. Survolez n’importe quelle commande de cette page pour obtenir la même aide que celle-ci.',
              reviewed: false } },
    'tip.langSelect': {
        en: { t: 'Language',
              b: 'Switches every caption, button and hover-help on this page between English and French. Value readouts, tuning names, note names and preset filenames stay exactly as they are.' },
        fr: { t: 'Langue',
              b: 'Bascule toutes les légendes, tous les boutons et toute l’aide au survol de cette page entre l’anglais et le français. Les valeurs affichées, les noms de systèmes d’accord, les noms de notes et les noms de fichiers de préréglages restent inchangés.',
              reviewed: false } },
});

// ── LABELS — one string, no body ───────────────────────────────────────────
export const LABELS = Object.freeze({

    // ── Header ────────────────────────────────────────────────────────────
    // The <h1> is NOT keyed: "O-MicrotonalSampler" is the product name and is
    // I18N_EXEMPT below. #tuning-readout holds the active tuning's name, which
    // is data from the C++ engine.
    'label.tabSampleMap':  { en: { t: 'Sample Map' },   fr: { t: 'Échantillons',  reviewed: false } },
    'label.tabTuning':     { en: { t: 'Tuning' },       fr: { t: 'Gamme',         reviewed: false } },
    'label.tabAbout':      { en: { t: 'About' },        fr: { t: 'À propos',      reviewed: false } },
    'label.savePreset':    { en: { t: 'Save Preset…' }, fr: { t: 'Enreg. prér.…', reviewed: false } },
    'label.loadPreset':    { en: { t: 'Load Preset…' }, fr: { t: 'Ouvrir prér.…', reviewed: false } },
    'label.language':      { en: { t: 'Language' },     fr: { t: 'Langue',        reviewed: false } },

    // ── Folder drop zone ──────────────────────────────────────────────────
    'label.dropFolderHere': { en: { t: 'Drop folder here' }, fr: { t: 'Déposer un dossier ici', reviewed: false } },
    'label.or':             { en: { t: 'or' },              fr: { t: 'ou',                     reviewed: false } },
    'label.loadFolder':     { en: { t: 'Load Folder…' },    fr: { t: 'Charger dossier…',       reviewed: false } },
    'label.batchLoop':      { en: { t: 'Batch loop…' },     fr: { t: 'Boucles en lot…',        reviewed: false } },
    'label.clearSamples':   { en: { t: 'Clear samples' },   fr: { t: 'Vider',                  reviewed: false } },

    // ── Technique preset row ──────────────────────────────────────────────
    // The four family names are CAPTIONS: the <option> VALUES the code keys on
    // are 'strings' … 'generic' and are untouched, and none of the four is an
    // AudioParameterChoice option or a JSON preset filename. The toast that
    // reports the change reads these same four keys, so the two cannot drift.
    'label.techniquePreset': { en: { t: 'Technique preset' }, fr: { t: 'Préréglage de techniques', reviewed: false } },
    'label.chooseFamily':    { en: { t: 'Choose family…' },   fr: { t: 'Choisir une famille…',    reviewed: false } },
    'label.familyStrings':   { en: { t: 'Strings' },          fr: { t: 'Cordes',                  reviewed: false } },
    'label.familyWinds':     { en: { t: 'Winds' },            fr: { t: 'Bois',                    reviewed: false } },
    'label.familyBrass':     { en: { t: 'Brass' },            fr: { t: 'Cuivres',                 reviewed: false } },
    'label.familyGeneric':   { en: { t: 'Generic' },          fr: { t: 'Générique',               reviewed: false } },

    // ── Keyswitch controls ────────────────────────────────────────────────
    // KS is the standard abbreviation for a keyswitch in both languages, and
    // Dorico's own French UI keeps it; sameAsEn is a decision, not a gap.
    'label.ks':     { en: { t: 'KS' },   fr: { t: 'KS',   reviewed: false, sameAsEn: true } },
    'label.ksLow':  { en: { t: 'low' },  fr: { t: 'min',  reviewed: false } },
    'label.ksHigh': { en: { t: 'high' }, fr: { t: 'max',  reviewed: false } },

    // ── Trigger panel ─────────────────────────────────────────────────────
    'label.triggersHeading':   { en: { t: 'Triggers (CC + PC)' },            fr: { t: 'Déclencheurs (CC + PC)',        reviewed: false } },
    'label.triggerPrecedence': { en: { t: 'KS > CC > PC > history' },        fr: { t: 'KS > CC > PC > historique',     reviewed: false } },
    'label.ccTrigger':         { en: { t: 'CC trigger' },                    fr: { t: 'Déclencheur CC',                reviewed: false } },
    'label.ccNumber':          { en: { t: 'CC#' },                           fr: { t: 'CC n°',                         reviewed: false } },
    'label.ccHint':            { en: { t: 'value 0–127 → technique slot' },  fr: { t: 'valeur 0–127 → emplacement',    reviewed: false } },
    'label.pcTrigger':         { en: { t: 'PC trigger' },                    fr: { t: 'Déclencheur PC',                reviewed: false } },
    'label.pcHint':            { en: { t: 'program # → technique slot' },    fr: { t: 'n° de programme → emplacement', reviewed: false } },
    // The two tables have IDENTICAL headers, and a key is owned by ONE
    // element: two <th>slot</th> nodes need two keys or the second is never
    // swept. Hence the `2` suffixes rather than a shared key.
    'label.thSlot':  { en: { t: 'slot' }, fr: { t: 'empl.', reviewed: false } },
    'label.thLow':   { en: { t: 'low' },  fr: { t: 'min',   reviewed: false } },
    'label.thHigh':  { en: { t: 'high' }, fr: { t: 'max',   reviewed: false } },
    'label.thTech':  { en: { t: 'tech' }, fr: { t: 'tech.', reviewed: false } },
    'label.thSlot2': { en: { t: 'slot' }, fr: { t: 'empl.', reviewed: false } },
    'label.thPc':    { en: { t: 'PC#' },  fr: { t: 'PC n°', reviewed: false } },
    'label.thTech2': { en: { t: 'tech' }, fr: { t: 'tech.', reviewed: false } },
    'label.resetDefaults': { en: { t: 'Reset to defaults' }, fr: { t: 'Valeurs par défaut', reviewed: false } },

    // ── Trim panel ────────────────────────────────────────────────────────
    // #trim-active-tech is NOT keyed: it holds a technique NAME the user can
    // rename, so it is a readout node (D-01 arm 3) and its placeholder "ord" is
    // I18N_EXEMPT. The four layer captions p / mp / mf / f are dynamic markings
    // — Italian musical notation, not English — and are exempt too.
    'label.trimsHeading':   { en: { t: 'Trims (loudness)' },                       fr: { t: 'Ajustements (niveau)',                       reviewed: false } },
    'label.trimTechnique':  { en: { t: 'Technique' },                              fr: { t: 'Technique',                                  reviewed: false, sameAsEn: true } },
    'label.trimHint':       { en: { t: 'Double-click a slider to reset to 0 dB' }, fr: { t: 'Double-clic sur un curseur → 0 dB', reviewed: false } },
    'label.resetAllTrims':  { en: { t: 'Reset all trims' },                        fr: { t: 'Tout réinitialiser',                         reviewed: false } },

    // ── Grid context menu + issues ────────────────────────────────────────
    'label.ctxReplace':        { en: { t: 'Replace…' },         fr: { t: 'Remplacer…',             reviewed: false } },
    'label.ctxOpenLoopEditor': { en: { t: 'Open Loop Editor' }, fr: { t: 'Ouvrir l’éditeur de boucle', reviewed: false } },
    'label.ctxDeleteSample':   { en: { t: 'Delete sample' },    fr: { t: 'Supprimer l’échantillon', reviewed: false } },
    'label.issues':            { en: { t: 'Issues' },           fr: { t: 'Problèmes',              reviewed: false } },
    // §6 — the count sits after a colon beside an invariant plural noun
    // phrase, so nothing inflects at 0, 1 or n in either language.
    'label.issuesSummary':     { en: { t: 'Issues · files skipped: {n}' },
                                 fr: { t: 'Problèmes · fichiers ignorés : {n}', reviewed: false } },

    // ── Loop editor ───────────────────────────────────────────────────────
    'label.loopPlaceholder': { en: { t: 'Select a loaded sample slot to edit loop points' },
                               fr: { t: 'Sélectionnez une case chargée pour modifier ses points de boucle', reviewed: false } },
    'label.loopStartCap':    { en: { t: 'Loop start:' },          fr: { t: 'Début de boucle :',   reviewed: false } },
    'label.loopEndCap':      { en: { t: 'Loop end:' },            fr: { t: 'Fin de boucle :',     reviewed: false } },
    'label.loopModeCap':     { en: { t: 'Mode:' },                fr: { t: 'Mode :',              reviewed: false } },
    'label.loopReset':       { en: { t: 'Reset to auto-detect' }, fr: { t: 'Détection auto',      reviewed: false } },
    'label.loopCancel':      { en: { t: 'Cancel' },               fr: { t: 'Annuler',             reviewed: false } },
    'label.loopApply':       { en: { t: 'Apply' },                fr: { t: 'Appliquer',           reviewed: false } },
    'label.variantOf':       { en: { t: 'Variant {i} of {n}' },   fr: { t: 'Variante {i} sur {n}', reviewed: false } },

    // ── Generic confirmation dialog ───────────────────────────────────────
    // EVERY dialog's Cancel button carries its OWN key. A key is owned by one
    // element (contract §1) and applyI18n sweeps per element, so seven dialogs
    // sharing one 'label.cancel' would leave six of them unswept.
    'label.areYouSure': { en: { t: 'Are you sure?' }, fr: { t: 'Êtes-vous sûr ?', reviewed: false } },
    'label.cancel':     { en: { t: 'Cancel' },        fr: { t: 'Annuler',         reviewed: false } },
    'label.confirm':    { en: { t: 'Confirm' },       fr: { t: 'Confirmer',       reviewed: false } },

    // ── Batch loop dialog ─────────────────────────────────────────────────
    'label.batchLoopTitle':  { en: { t: 'Batch loop points' }, fr: { t: 'Points de boucle en lot', reviewed: false } },
    'label.batchLoopBlurb':  { en: { t: 'Apply one loop region to every loaded sample at once. One-shot samples (too short to loop) are left untouched.' },
                               fr: { t: 'Applique une région de boucle à tous les échantillons chargés. Les one-shot (trop courts) restent intacts.', reviewed: false } },
    'label.blUnits':         { en: { t: 'Units' },             fr: { t: 'Unités',           reviewed: false } },
    'label.blProportional':  { en: { t: 'Proportional (%)' },  fr: { t: 'Proportionnel (%)', reviewed: false } },
    'label.blMilliseconds':  { en: { t: 'Milliseconds' },      fr: { t: 'Millisecondes',    reviewed: false } },
    'label.blLoopStart':     { en: { t: 'Loop start' },        fr: { t: 'Début de boucle',  reviewed: false } },
    'label.blLoopEnd':       { en: { t: 'Loop end' },          fr: { t: 'Fin de boucle',    reviewed: false } },
    'label.cancelBl':        { en: { t: 'Cancel' },            fr: { t: 'Annuler',          reviewed: false } },
    'label.blApplyAll':      { en: { t: 'Apply to all' },      fr: { t: 'Tout appliquer',   reviewed: false } },
    'label.blErrPercent':    { en: { t: 'Start and end must be 0–100 %, with end greater than start.' },
                               fr: { t: 'Début et fin doivent être entre 0 et 100 %, la fin après le début.', reviewed: false } },
    'label.blErrMs':         { en: { t: 'Start and end must be in ms, with end greater than start.' },
                               fr: { t: 'Début et fin doivent être en ms, la fin après le début.', reviewed: false } },

    // ── Diagnostic dialog ─────────────────────────────────────────────────
    'label.diagnostic':      { en: { t: 'Diagnostic' },  fr: { t: 'Diagnostic', reviewed: false, sameAsEn: true } },
    'label.diagnosticHint':  { en: { t: 'Auto-copied to clipboard. Select the text below and ⌘C if it wasn\'t.' },
                               fr: { t: 'Copié automatiquement dans le presse-papiers. Sinon, sélectionnez le texte ci-dessous puis ⌘C.', reviewed: false } },
    'label.diagCopied':      { en: { t: 'Auto-copied to clipboard. (Select below + ⌘C if you need it again.)' },
                               fr: { t: 'Copié automatiquement dans le presse-papiers. (Sélectionnez ci-dessous + ⌘C si besoin.)', reviewed: false } },
    'label.diagCopyBlocked': { en: { t: 'Clipboard write blocked — select the text below and ⌘C to copy.' },
                               fr: { t: 'Écriture dans le presse-papiers bloquée — sélectionnez le texte ci-dessous puis ⌘C.', reviewed: false } },
    'label.copyAgain':       { en: { t: 'Copy again' },  fr: { t: 'Copier à nouveau', reviewed: false } },
    'label.copied':          { en: { t: 'Copied ✓' },    fr: { t: 'Copié ✓',          reviewed: false } },
    'label.copyFailed':      { en: { t: 'Copy failed' }, fr: { t: 'Échec de la copie', reviewed: false } },
    'label.close':           { en: { t: 'Close' },       fr: { t: 'Fermer',           reviewed: false } },

    // ── Missing-folder dialog ─────────────────────────────────────────────
    // The highest-value strings on this page: they are what a French user reads
    // when a reopened session cannot find its samples. Each face is keyed
    // separately rather than interpolated, so neither reads as a sentence with
    // a hole in it.
    'label.folderNotFound':            { en: { t: 'Sample folder not found' }, fr: { t: 'Dossier d’échantillons introuvable', reviewed: false } },
    'label.folderNotFoundMsgNamed':    { en: { t: 'The sample folder "{name}" was not found at its saved location. Locate it now, or skip and load samples manually.' },
                                         fr: { t: 'Le dossier d’échantillons « {name} » est introuvable à son emplacement enregistré. Localisez-le maintenant, ou passez et chargez les échantillons manuellement.', reviewed: false } },
    'label.folderNotFoundMsgUnnamed':  { en: { t: 'The saved sample folder was not found. Locate it now, or skip and load samples manually.' },
                                         fr: { t: 'Le dossier d’échantillons enregistré est introuvable. Localisez-le maintenant, ou passez et chargez les échantillons manuellement.', reviewed: false } },
    'label.dragDropNotEmbedded':       { en: { t: 'Drag-dropped samples not embedded' }, fr: { t: 'Échantillons déposés non intégrés', reviewed: false } },
    'label.dragDropMsgNamed':          { en: { t: 'Samples were drag-dropped from "{name}" without "Embed audio" enabled, so they could not be re-loaded automatically. Re-drag the folder onto the plugin, or browse to its current location.' },
                                         fr: { t: 'Les échantillons ont été déposés depuis « {name} » sans « Intégrer l’audio », ils n’ont donc pas pu être rechargés automatiquement. Redéposez le dossier sur le plugiciel, ou parcourez jusqu’à son emplacement actuel.', reviewed: false } },
    'label.dragDropMsgUnnamed':        { en: { t: 'Samples were drag-dropped without "Embed audio" enabled, so they could not be re-loaded automatically. Re-drag the folder onto the plugin, or browse to its current location.' },
                                         fr: { t: 'Les échantillons ont été déposés sans « Intégrer l’audio », ils n’ont donc pas pu être rechargés automatiquement. Redéposez le dossier sur le plugiciel, ou parcourez jusqu’à son emplacement actuel.', reviewed: false } },
    'label.skip':                      { en: { t: 'Skip' },              fr: { t: 'Passer',             reviewed: false } },
    'label.locateFolder':              { en: { t: 'Locate folder…' },    fr: { t: 'Localiser le dossier…', reviewed: false } },
    'label.browseForFolder':           { en: { t: 'Browse for folder…' }, fr: { t: 'Parcourir…',        reviewed: false } },

    // ── Round-robin confirmation ──────────────────────────────────────────
    // SPLIT per §5: the paragraph wraps three <code> filename tokens, which are
    // literals the C++ FilenameParser matches and must not be translated.
    'label.rrTitle':      { en: { t: 'Multiple samples for the same note' }, fr: { t: 'Plusieurs échantillons pour la même note', reviewed: false } },
    'label.rrBodyBefore': { en: { t: 'The folder contains more than one sample for the same note and velocity layer, without explicit' },
                            fr: { t: 'Le dossier contient plusieurs échantillons pour la même note et la même couche, sans jetons', reviewed: false } },
    'label.rrBodyAfter':  { en: { t: 'tokens in the filenames. Treat them as round-robin variants?' },
                            fr: { t: 'dans les noms. Les traiter comme des variantes round-robin ?', reviewed: false } },
    'label.rrCancel':     { en: { t: 'Cancel load' },       fr: { t: 'Annuler le chargement', reviewed: false } },
    'label.rrAccept':     { en: { t: 'Treat as variants' }, fr: { t: 'Traiter en variantes',  reviewed: false } },
    'label.rrCellHead':   { en: { t: 'MIDI {n} · {mark}' }, fr: { t: 'MIDI {n} · {mark}',     reviewed: false, sameAsEn: true } },

    // ── Folder-load options dialog ────────────────────────────────────────
    'label.floTitle':            { en: { t: 'Load samples' },          fr: { t: 'Charger des échantillons', reviewed: false } },
    'label.floLayer':            { en: { t: 'Layer' },                 fr: { t: 'Couche',                   reviewed: false } },
    'label.floTechnique':        { en: { t: 'Technique' },             fr: { t: 'Technique', reviewed: false, sameAsEn: true } },
    'label.floWhenLoading':      { en: { t: 'When loading' },          fr: { t: 'Au chargement',            reviewed: false } },
    'label.floAddToLayer':       { en: { t: 'Add to layer' },          fr: { t: 'Ajouter à la couche',      reviewed: false } },
    'label.floReplaceLayer':     { en: { t: 'Replace this layer' },    fr: { t: 'Remplacer cette couche',   reviewed: false } },
    'label.floReplaceAll':       { en: { t: 'Replace all samples' },   fr: { t: 'Remplacer tout',           reviewed: false } },
    'label.floMergeRr':          { en: { t: 'Layer as round-robin' },  fr: { t: 'Superposer en round-robin', reviewed: false } },
    'label.floForceLayer':       { en: { t: 'Force all samples onto this layer' },     fr: { t: 'Forcer tous les échantillons sur cette couche',    reviewed: false } },
    'label.floForceTechnique':   { en: { t: 'Force all samples onto this technique' }, fr: { t: 'Forcer tous les échantillons sur cette technique', reviewed: false } },
    // SPLIT per §5 around the <code> filename tokens. Two keys per half,
    // because the same English opens both lines and each <span> is its own
    // element with its own key.
    'label.floTokensBefore':     { en: { t: 'Otherwise filename tokens (' }, fr: { t: 'Sinon les jetons du fichier (', reviewed: false } },
    'label.floTokensBefore2':    { en: { t: 'Otherwise filename tokens (' }, fr: { t: 'Sinon les jetons du fichier (', reviewed: false } },
    'label.floTokensAfterLayer': { en: { t: ', etc.) decide the layer' },     fr: { t: ', etc.) décident de la couche',        reviewed: false } },
    'label.floTokensAfterTech':  { en: { t: ', etc.) decide the technique' }, fr: { t: ', etc.) décident de la technique',     reviewed: false } },
    'label.floEmbed':            { en: { t: 'Embed audio in project state' }, fr: { t: 'Intégrer l’audio dans le projet', reviewed: false } },
    'label.floEmbedSize':        { en: { t: 'Project state will grow by ~{size}.' },
                                   fr: { t: 'L’état du projet augmentera d’environ {size}.', reviewed: false } },
    'label.floEmbedSizePending': { en: { t: 'Size will be confirmed after folder selection.' },
                                   fr: { t: 'La taille sera confirmée après le choix du dossier.', reviewed: false } },
    'label.cancelFlo':           { en: { t: 'Cancel' }, fr: { t: 'Annuler', reviewed: false } },
    'label.floLoad':             { en: { t: 'Load…' },  fr: { t: 'Charger…', reviewed: false } },

    // ── Embed-size confirmation ───────────────────────────────────────────
    'label.embedTitle':      { en: { t: 'Embed audio in project?' }, fr: { t: 'Intégrer l’audio au projet ?', reviewed: false } },
    'label.embedDetail':     { en: { t: 'The audio data will be written into your DAW project file. Project saves and reopens will be slower for large libraries, but the samples will travel with the project across folders and machines.' },
                               fr: { t: 'Les données audio seront écrites dans le fichier de projet de votre STAN. Les enregistrements et réouvertures seront plus lents pour les grandes bibliothèques, mais les échantillons suivront le projet d’un dossier ou d’une machine à l’autre.', reviewed: false } },
    'label.embedMsgNamed':   { en: { t: 'Embedding folder "{name}" will add ~{size} to your project state.' },
                               fr: { t: 'Intégrer le dossier « {name} » ajoutera environ {size} à l’état du projet.', reviewed: false } },
    'label.embedMsgUnnamed': { en: { t: 'Embedding this folder will add ~{size} to your project state.' },
                               fr: { t: 'Intégrer ce dossier ajoutera environ {size} à l’état du projet.', reviewed: false } },
    'label.cancelEmbed':     { en: { t: 'Cancel' },         fr: { t: 'Annuler',           reviewed: false } },
    'label.embedAndLoad':    { en: { t: 'Embed and load' }, fr: { t: 'Intégrer et charger', reviewed: false } },

    // ── Per-cell merge dialog ─────────────────────────────────────────────
    // §6: v1.23.10 built `1 variant` / `${n} variants` with a ternary and then
    // interpolated it mid-sentence. Both faces now carry the count after a
    // colon instead, so neither inflects.
    'label.mergeTitle':        { en: { t: 'Cell already has samples' }, fr: { t: 'La case contient déjà des échantillons', reviewed: false } },
    'label.mergeMsgCapped':    { en: { t: '{note} layer {mark} is at the maximum · variants held: {n}. Replace the cell, or cancel.' },
                                 fr: { t: '{note} couche {mark} est au maximum · variantes présentes : {n}. Remplacez la case, ou annulez.', reviewed: false } },
    'label.mergeMsgAdd':       { en: { t: '{note} layer {mark} · variants held: {n}. Add this sample as round-robin variant {next}, or replace the cell?' },
                                 fr: { t: '{note} couche {mark} · variantes présentes : {n}. Ajouter cet échantillon comme variante round-robin {next}, ou remplacer la case ?', reviewed: false } },
    'label.cancelMerge':       { en: { t: 'Cancel' },             fr: { t: 'Annuler',            reviewed: false } },
    'label.mergeReplaceCell':  { en: { t: 'Replace cell' },       fr: { t: 'Remplacer la case',  reviewed: false } },
    'label.mergeAddRr':        { en: { t: 'Add as round-robin' }, fr: { t: 'Ajouter en round-robin', reviewed: false } },

    // ── Technique rename dialog ───────────────────────────────────────────
    // SPLIT per §5: the paragraph wraps the live slot number.
    'label.renameTitle':  { en: { t: 'Rename technique' }, fr: { t: 'Renommer la technique', reviewed: false } },
    'label.renameSlot':   { en: { t: 'Slot' },             fr: { t: 'Empl.',                 reviewed: false } },
    'label.renameBody':   { en: { t: ': enter a new name. Names appear in the tab strip and Dorico expression maps.' },
                            fr: { t: ' : saisissez un nouveau nom. Il apparaît dans la barre d’onglets et les cartes d’expression Dorico.', reviewed: false } },
    'label.cancelRename': { en: { t: 'Cancel' }, fr: { t: 'Annuler',   reviewed: false } },
    'label.renameSave':   { en: { t: 'Save' },   fr: { t: 'Enregistrer', reviewed: false } },

    // ── About ─────────────────────────────────────────────────────────────
    // The two headings are the product name and the company name — exempt.
    'label.aboutTagline': { en: { t: 'Microtonal sample engine for Dorico microtonal playback.' },
                            fr: { t: 'Moteur d’échantillons microtonal pour la lecture microtonale de Dorico.', reviewed: false } },
    'label.aboutBlurb':   { en: { t: 'Per-key, per-velocity-layer sample mapping with offline loop auto-detection, manual loop editing, and the Ouaricon tuning-system family. Built on JUCE 8.' },
                            fr: { t: 'Mappage par touche et par couche de vélocité, détection automatique des boucles, édition manuelle, et les systèmes d’accord Ouaricon. Bâti sur JUCE 8.', reviewed: false } },   // 2 lines, measured; the fuller draft was 3 and grew the About card 20.14px
    'label.madeBy':       { en: { t: 'Made by' }, fr: { t: 'Réalisé par', reviewed: false } },

    // ── Control strip ─────────────────────────────────────────────────────
    // The nine captions live in SLIDER_BINDINGS and are interpolated into an
    // innerHTML template, so nine LITERAL setLabel() call sites key them —
    // see labelControlStrip().
    //
    // THE TIGHTEST BOX ON THE PAGE, AND MEASURED RATHER THAN GUESSED. Each
    // .ouaricon-knob is `flex: 1 1 0; min-width: 56px` in a 900px strip that
    // also carries a 182px non-shrinking Dynamics control, so the caption gets
    // 58.39px and there is no slack to widen it into. English's longest,
    // "Out Gain", measures 55.38px; the wrap threshold sits between that and
    // 65.83. Three French drafts crossed it — Fondu-Vél, Ampl. dyn. and Gain
    // sortie each wrapped to TWO lines, growing #control-strip 113px -> 127px
    // and pushing the flex:1 #tab-bodies down by exactly 14px in every state.
    // They were re-drafted to Vél-XF (39.97), Ét. dyn. (48.34) and Sortie
    // (41.22), each measured IN this element so letter-spacing and the real
    // font are in play. Maintien is the tightest survivor at 55.69px.
    'label.knobAttack':   { en: { t: 'Attack' },   fr: { t: 'Attaque',  reviewed: false } },
    'label.knobDecay':    { en: { t: 'Decay' },    fr: { t: 'Chute',    reviewed: false } },
    'label.knobSustain':  { en: { t: 'Sustain' },  fr: { t: 'Maintien', reviewed: false } },
    'label.knobRelease':  { en: { t: 'Release' },  fr: { t: 'Relâch.',  reviewed: false } },
    'label.knobPoly':     { en: { t: 'Poly' },     fr: { t: 'Polyph.',  reviewed: false } },
    'label.knobVelXf':    { en: { t: 'Vel-XF' },   fr: { t: 'Vél-XF',   reviewed: false } },
    'label.knobExpr':     { en: { t: 'Expr' },     fr: { t: 'Expr.',    reviewed: false } },
    'label.knobDynRng':   { en: { t: 'Dyn Rng' },  fr: { t: 'Ét. dyn.', reviewed: false } },
    'label.knobOutGain':  { en: { t: 'Out Gain' }, fr: { t: 'Sortie',   reviewed: false } },
    'label.dynamics':     { en: { t: 'Dynamics' }, fr: { t: 'Dynamique', reviewed: false } },

    // ── Tuning panel (js/tuning-panel.js) ─────────────────────────────────
    // Plugin-owned copy, in scope, keyed by hand. The twelve note names, the
    // interval-quality abbreviations and the generated scale names are NOT
    // keyed — see the note above I18N_EXEMPT.
    'label.vizCircle':      { en: { t: 'Circle' },     fr: { t: 'Cercle',   reviewed: false } },
    'label.vizPolar':       { en: { t: 'Polar' },      fr: { t: 'Polaire',  reviewed: false } },
    'label.vizMatrix':      { en: { t: 'Matrix' },     fr: { t: 'Matrice',  reviewed: false } },
    'label.vizTrueKeys':    { en: { t: 'True Keys' },  fr: { t: 'Touches',  reviewed: false } },
    'label.vizRotation':    { en: { t: 'Rotation' },   fr: { t: 'Rotations', reviewed: false } },
    'label.scaleIntervals': { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles', reviewed: false } },
    'label.tkHint':         { en: { t: 'Hold 2+ notes to see intervals' }, fr: { t: 'Tenez 2 notes ou plus pour voir les intervalles', reviewed: false } },
    'label.totalSpan':      { en: { t: 'Total span' }, fr: { t: 'Étendue',        reviewed: false } },
    'label.rotationMode':   { en: { t: 'Mode' },       fr: { t: 'Mode', reviewed: false, sameAsEn: true } },
    'label.intervalsCount': { en: { t: 'Intervals · notes: {n}' },
                              fr: { t: 'Interv. · notes : {n}', reviewed: false } },   // 114.45px in a
                              // 142px column, measured. The fuller 'Intervalles · notes : {n}'
                              // is 2 lines and pushes the whole interval list down 14px; the
                              // one-line alternative 'Intervalles · {n} notes' fits at 138.44
                              // but inflects wrongly at n=1, so per contract §6 the count stays
                              // after the colon beside an invariant noun instead.
    'label.tonic':          { en: { t: 'Tonic' },      fr: { t: 'Tonique',  reviewed: false } },
    'label.tuningLibrary':  { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque', reviewed: false } },
    'label.catAll':         { en: { t: 'All Categories' },  fr: { t: 'Toutes catégories', reviewed: false } },
    'label.catHistorical':  { en: { t: 'Historical' },      fr: { t: 'Historiques',       reviewed: false } },
    'label.catJust':        { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste',  reviewed: false } },
    'label.catEdo':         { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales',  reviewed: false } },
    'label.catNonOctave':   { en: { t: 'Non-Octave' },      fr: { t: 'Non-octaviantes',   reviewed: false } },
    'label.catWorld':       { en: { t: 'World' },           fr: { t: 'Du monde',          reviewed: false } },
    'label.noteCount':      { en: { t: 'notes: {n}' },      fr: { t: 'notes : {n}',       reviewed: false } },
    // A4 stays A4: it is letter pitch notation, which the C++ TuningEngine and
    // the .scl/.kbm formats also speak. Only REF is a word.
    'label.a4Ref':          { en: { t: 'A4 REF' },  fr: { t: 'RÉF A4',    reviewed: false } },
    'label.stretch':        { en: { t: 'Stretch' }, fr: { t: 'Étirement', reviewed: false } },
    'label.loadScl':        { en: { t: 'Load .SCL' },   fr: { t: 'Ouvrir .SCL',   reviewed: false } },
    'label.loadKbm':        { en: { t: 'Load .KBM' },   fr: { t: 'Ouvrir .KBM',   reviewed: false } },
    'label.saveScl':        { en: { t: 'Save .SCL' },   fr: { t: 'Enreg. .SCL',   reviewed: false } },
    'label.saveKbm':        { en: { t: 'Save .KBM' },   fr: { t: 'Enreg. .KBM',   reviewed: false } },
    'label.exportHtml':     { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: false } },
    'label.generateScale':  { en: { t: 'Generate Scale' },      fr: { t: 'Générer une gamme', reviewed: false } },
    'label.genEdo':         { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: false } },
    'label.genHarmonic':    { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique',     reviewed: false } },
    'label.genRank2':       { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: false } },
    // Two Period (c) labels: one in the EDO row, one in the Rank-2 row. Each
    // <label> is its own element and so needs its own key.
    'label.genDivisions':     { en: { t: 'Divisions' },      fr: { t: 'Divisions', reviewed: false, sameAsEn: true } },
    'label.genPeriod':        { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: false } },
    'label.genStartHarmonic': { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: false } },
    'label.genEndHarmonic':   { en: { t: 'End Harmonic' },   fr: { t: 'Harm. de fin',         reviewed: false } },
    'label.genGenerator':     { en: { t: 'Generator (c)' },  fr: { t: 'Génér. (c)',           reviewed: false } },
    'label.genR2Period':      { en: { t: 'Period (c)' },     fr: { t: 'Période (c)',          reviewed: false } },
    'label.genNotes':         { en: { t: 'Notes' },          fr: { t: 'Notes', reviewed: false, sameAsEn: true } },
    'label.generate':         { en: { t: 'Generate' },       fr: { t: 'Générer', reviewed: false } },
    'label.tuningPanelUnavailable': { en: { t: 'Tuning panel unavailable.' },
                                      fr: { t: 'Panneau de gamme indisponible.', reviewed: false } },

    // ── ACCESSIBLE NAMES declared in markup or by a literal dataset write ──
    //
    // Nine were already aria-labels at v1.23.10. The rest are the FIVE markup
    // native title= attributes and THREE of the JS-written ones, moved here
    // per contract §4 with their own v1.23.10 wording, VERBATIM. No new prose
    // is invented: Stage M authors hover-help, this rule does not.
    'aria.savePreset':       { en: { t: 'Save plugin state to .omspreset' },   fr: { t: 'Enregistrer l’état du plugiciel dans un .omspreset', reviewed: false } },
    'aria.loadPreset':       { en: { t: 'Load plugin state from .omspreset' }, fr: { t: 'Charger l’état du plugiciel depuis un .omspreset',  reviewed: false } },
    'aria.settings':         { en: { t: 'Settings' },           fr: { t: 'Réglages',            reviewed: false } },
    'aria.langSelect':       { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: false } },
    'aria.techniquePreset':  { en: { t: 'Rename all technique slots to match a Dorico instrument family (slot order matches the keyswitch order in the O-MicrotonalSampler expression maps)' },
                               fr: { t: 'Renommer tous les emplacements de technique selon une famille d’instruments Dorico (l’ordre des emplacements suit celui des keyswitches dans les cartes d’expression O-MicrotonalSampler)', reviewed: false } },
    'aria.playingTechniques': { en: { t: 'Playing techniques' },     fr: { t: 'Techniques de jeu',          reviewed: false } },
    'aria.addTechnique':      { en: { t: 'Add technique slot' },     fr: { t: 'Ajouter un emplacement de technique', reviewed: false } },
    'aria.removeTechnique':   { en: { t: 'Remove last technique slot' }, fr: { t: 'Retirer le dernier emplacement de technique', reviewed: false } },
    'aria.trimTech':          { en: { t: 'Technique master trim (dB)' }, fr: { t: 'Ajustement général de la technique (dB)', reviewed: false } },
    'aria.trimLayer0':        { en: { t: 'Layer p trim (dB)' },  fr: { t: 'Ajustement de la couche p (dB)',  reviewed: false } },
    'aria.trimLayer1':        { en: { t: 'Layer mp trim (dB)' }, fr: { t: 'Ajustement de la couche mp (dB)', reviewed: false } },
    'aria.trimLayer2':        { en: { t: 'Layer mf trim (dB)' }, fr: { t: 'Ajustement de la couche mf (dB)', reviewed: false } },
    'aria.trimLayer3':        { en: { t: 'Layer f trim (dB)' },  fr: { t: 'Ajustement de la couche f (dB)',  reviewed: false } },
    'aria.closeLoopEditor':   { en: { t: 'Close loop editor' },  fr: { t: 'Fermer l’éditeur de boucle', reviewed: false } },
    'aria.floLayer':          { en: { t: 'Target velocity layer' }, fr: { t: 'Couche de vélocité cible',   reviewed: false } },
    'aria.floTechnique':      { en: { t: 'Target technique slot' }, fr: { t: 'Emplacement de technique cible', reviewed: false } },
    'aria.renameInput':       { en: { t: 'New technique name' },    fr: { t: 'Nouveau nom de technique',   reviewed: false } },
    'aria.dynamicsMode':      { en: { t: 'Dynamics Mode — how MIDI CC 11 shapes dynamics. Velocity: note-on velocity picks the layer, CC 11 is a post-mix volume trim (v1.20 behaviour). CC Crossfade: CC 11 morphs across all velocity layers mid-note (timbre + loudness, like pro sustain patches).' },
                                fr: { t: 'Mode de dynamique — comment le CC MIDI 11 façonne la dynamique. Velocity : la vélocité de la note choisit la couche, le CC 11 sert d’ajustement de volume après mixage (comportement v1.20). CC Crossfade : le CC 11 fond toutes les couches de vélocité au cours de la note (timbre + niveau, comme les patches de tenue professionnels).', reviewed: false } },
    'aria.dynamicsModeShort': { en: { t: 'Dynamics Mode' }, fr: { t: 'Mode de dynamique', reviewed: false } },
    'aria.knobExpr':          { en: { t: 'Expression (MIDI CC 11) — dynamics control, independent of velocity layer' },
                                fr: { t: 'Expression (CC MIDI 11) — contrôle de la dynamique, indépendant de la couche de vélocité', reviewed: false } },
    'aria.knobDynRng':        { en: { t: 'Dynamic Range (CC Crossfade only) — dB span between pp and ff. 0 dB = flat; higher = louder ff / quieter pp. Fixes "forte too soft, piano too loud" in Dorico.' },
                                fr: { t: 'Amplitude dynamique (CC Crossfade uniquement) — écart en dB entre pp et ff. 0 dB = plat ; plus haut = ff plus fort / pp plus doux. Corrige le « forte trop faible, piano trop fort » dans Dorico.', reviewed: false } },
    'aria.loopResetOneShot':  { en: { t: 'Sample is one-shot — no loop region detected.' },
                                fr: { t: 'Échantillon one-shot — aucune région de boucle détectée.', reviewed: false } },
});

// ============================================================================
// I18N_EXEMPT — reasoned exclusions, never silence
//
// An entry is [text, reason] or [text, reason, scope]. The scope is a
// comma-separated list of `tag`, `.class` or `#id` matched against the node's
// own parent and its ancestors, and it is REQUIRED wherever the same string is
// also KEYED on this page — the one state in which the gate cannot tell a
// deliberate skip from a label somebody forgot.
//
// ── LETTER PITCH NOTATION AND FILENAME TOKENS, THE TWO STANDING SETS ───────
//
// The tuning panel's twelve note names (C … B), its interval-quality
// abbreviations (m2 … P8, TT) and the generated scale names (19-EDO, Harmonics
// 8-16, Rank-2 (696.6c, 12 notes)) are NOT localized. French solfège would be
// Do/Ré/Mi and 2m/2M/…/8J, and the C++ TuningEngine, the .scl and .kbm formats
// and the exported HTML all speak the letter system. Translating the page alone
// desyncs it from the files it reads and writes; translating both is a
// data-format change, not a localization. Same verdict as O-Bells v4.2.0 and
// O-IntonationPad v2.9.0 over the same panel. Most of them classify as READOUT
// and need no entry; the ones that survive the readout filter are listed below.
//
// The filename tokens are the same argument at the other end: rr / take / tk /
// _v1 / _ff / _pizz / _harm are literals Source/FilenameParser.cpp matches
// against a filename on disk. A French user still types _pizz.
// ============================================================================
export const I18N_EXEMPT = [

    // ── product and company ───────────────────────────────────────────────
    ['O-MicrotonalSampler',
     'the product name — the header, the About card and the plugin bundle all say it, and a product name is never translated'],
    ['Ouaricon',
     'the company name — never translated'],

    // ── D-01 arm 1: byte-identical AudioParameterChoice option strings ────
    // dynamics_mode is StringArray { "Velocity", "CC Crossfade" }
    // (PluginProcessor.cpp:209). The page and the host automation lane must
    // agree, so the segmented toggle keeps the host's words.
    ['Velocity',
     'a dynamics_mode AudioParameterChoice option VERBATIM (PluginProcessor.cpp:209) — D-01 arm 1; the host automation lane shows this exact word'],
    ['CC Crossfade',
     'the other dynamics_mode option (PluginProcessor.cpp:209), spelled with a non-breaking space in the markup so it cannot wrap mid-name — D-01 arm 1',
     '.seg-toggle'],

    // ── D-01 arm 3: readout nodes ─────────────────────────────────────────
    ['ord',
     'the #trim-active-tech chip and the technique tab strip both DISPLAY a technique name the user can rename; "ord" is only the seeded default, so the node is a readout and never a [data-i18n] element — D-01 arm 3',
     '#trim-active-tech'],
    ['12-TET Standard',
     'the tuning name the C++ TuningEngine reports through getTuningName(); it is written into .scl files and the exported HTML, so it is data, not copy — D-02',
     '#scale-name-display'],

    // ── musical notation ──────────────────────────────────────────────────
    ['mp',
     'an Italian dynamic marking, the notation every language uses. Its three siblings p, mf and f classify as READOUT and need no entry'],
    ['mf',
     'an Italian dynamic marking, as above'],
    ['TT',
     'the tritone’s interval-quality abbreviation in the True Keys view, beside letter pitch names the C++ engine and the .scl/.kbm formats also use — see the note above. The other eleven (m2, M2, m3, M3, P4, P5, m6, M6, m7, M7, P8) classify as READOUT and need no entry'],

    // ── filename tokens the C++ parser matches literally ──────────────────
    ['rr',   'a round-robin filename token Source/FilenameParser.cpp matches on disk — a French user still types it'],
    ['take', 'a round-robin filename token, as above'],
    ['tk',   'a round-robin filename token, as above'],
    ['_ff',  'a velocity-layer filename token, as above'],
    ['_pizz','a technique filename token, as above'],
    ['_harm','a technique filename token, as above'],

    // ── owned by a shared module under modules/ ────────────────────────────
    // Roughly twenty drag-drop progress and failure toasts live in
    // modules/core/webview-drop-streaming/js/webview-drop-streaming.js, which
    // CMakeLists.txt embeds from ${CMAKE_SOURCE_DIR}/modules/ BY REFERENCE.
    // Editing it here edits every plugin that embeds it, and /module-upgrade
    // would revert a local change. They are pushed through showToast() as
    // finished strings, which is why showToast() still takes text rather than a
    // key. THREE of them are also emitted from PluginEditor.cpp for the
    // non-WKWebView drop path; localizing only the C++ half would make the same
    // message appear in French or English depending on which code path fired,
    // so those three stay English too. Named here rather than left to be
    // rediscovered.
    ['Drop a single file on a cell, or a folder on the top zone.',
     'owned by the shared modules/core/webview-drop-streaming module, and emitted verbatim from PluginEditor.cpp for the non-WKWebView drop path'],
    ['Drop a .wav/.aif on a cell',
     'owned by modules/core/webview-drop-streaming, and emitted verbatim from PluginEditor.cpp'],
    ['Drop a folder, not a file',
     'owned by modules/core/webview-drop-streaming, and emitted verbatim from PluginEditor.cpp'],
    ['File not found — drop from Finder',
     'emitted from PluginEditor.cpp on the non-WKWebView drop path, alongside the three module-owned strings above'],
];

// ============================================================================
// TIP_BINDINGS — [selector, key] or [selector, key, wrapper]
//
// applyI18n() does `document.querySelector(selector)`, then `closest(wrapper)`
// where a wrapper is given, and writes data-tip-title + data-tip onto whatever
// that lands on. setupTooltips() in js/sampler-app.js then walks UP from the
// pointer's target with closest('[data-tip]'), so a tip is held open by the
// whole subtree of its anchor.
//
// T17 SAYS "BIND TO THE IDS THE UI ALREADY USES." That was false on five M1
// plugins out of five, for a different reason each time, and it is false here
// too — in BOTH halves independently:
//
//   - THE SELECTOR HALF. The nine control-strip knobs carry no id of their own.
//     `#ctrl-attack` is the 1x1px opacity-0 <input type="range"> INSIDE the
//     knob, which is `pointer-events: none` (sampler-shell.css) and therefore
//     cannot be hovered at all — a tip bound to it would be a tip nobody can
//     open, and it would pass every static check. The addressable node is the
//     cell itself, `[data-knob-id="ctrl-attack"]`, which renderControlStrip()
//     writes. The dynamics toggle is the same shape: `.dynamics-mode-control`.
//   - THE WRAPPER HALF. Six anchors ARE ids and still need the walk, because
//     the id'd node is a checkbox or a number field a few px across sitting
//     inside the label that names it. `.ks-range-label` MATCHES TWICE (low and
//     high) and `.trigger-toggle-label` matches twice (CC and PC) — closest()
//     from each input reaches its own, which a bare querySelector on the class
//     would only do by luck for one of the pair (the O-Tremolo trap).
//
// THE CHROME BINDS BARE. #gear-btn and #lang-select share `.settings-cluster`,
// so any wrapper walk would make hovering the selector resolve to the gear's
// tip — O-Comp measured exactly that. Both are bound to themselves.
//
// TWO ROWS SHARE ONE KEY, and that is not the failure mode. `tip.techniqueCount`
// is bound to BOTH `#technique-add` and `#technique-remove`, because one
// parameter is what those two buttons move and neither is more the control than
// the other. The thing that silently breaks is two bindings on the SAME NODE —
// the second setAttribute overwrites the first while check-i18n cheerfully
// reports two bound tips — and tests/ui_tip_render_check.js asserts every
// binding resolves to a DISTINCT node by identity for that reason.
//
// NOT BOUND, deliberately: the preset bar (Save/Load), the tab strip, the drop
// zone and the six dialogs. Those got accessible names from their deleted
// native title= attributes in v1.24.0 and are self-describing; tips there are
// polish, not this stage's scope.
// ============================================================================
export const TIP_BINDINGS = [
    // ── the nine control-strip knobs (the cell, never the hidden input) ────
    ['[data-knob-id="ctrl-attack"]',             'tip.attack'],
    ['[data-knob-id="ctrl-decay"]',              'tip.decay'],
    ['[data-knob-id="ctrl-sustain"]',            'tip.sustain'],
    ['[data-knob-id="ctrl-release"]',            'tip.release'],
    ['[data-knob-id="ctrl-polyphony"]',          'tip.polyphony'],
    ['[data-knob-id="ctrl-velocity-crossfade"]', 'tip.velocityCrossfade'],
    ['[data-knob-id="ctrl-expression"]',         'tip.expression'],
    ['[data-knob-id="ctrl-dynamic-range"]',      'tip.dynamicRange'],
    ['[data-knob-id="ctrl-output-gain"]',        'tip.outputGain'],

    // ── the dynamics segmented toggle ─────────────────────────────────────
    ['.dynamics-mode-control',                   'tip.dynamicsMode'],

    // ── the technique bar ─────────────────────────────────────────────────
    ['#technique-tabs',                          'tip.techniqueSelect'],
    ['#technique-add',                           'tip.techniqueCount'],
    ['#technique-remove',                        'tip.techniqueCount'],
    ['#technique-ks-enabled',                    'tip.ksEnabled',   '.ks-toggle-label'],
    ['#technique-ks-low',                        'tip.ksLowNote',   '.ks-range-label'],
    ['#technique-ks-high',                       'tip.ksHighNote',  '.ks-range-label'],

    // ── the CC + PC trigger panel ─────────────────────────────────────────
    ['#cc-trigger-enabled',                      'tip.ccSelectEnabled', '.trigger-toggle-label'],
    ['#cc-trigger-number',                       'tip.ccNumber',        '.trigger-cc-number-label'],
    ['#pc-trigger-enabled',                      'tip.pcEnabled',       '.trigger-toggle-label'],

    // ── chrome, BARE (see above) ──────────────────────────────────────────
    ['#gear-btn',                                'tip.gear'],
    ['#lang-select',                             'tip.langSelect'],
];

// The canon block imports this by name and calls it for every TIP_BINDINGS
// entry. TIP_BINDINGS is empty here, so nothing calls it today — but the canon
// is ONE shape across all 43 plugins and is byte-compared, so the function it
// imports is not trimmed per plugin.
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
