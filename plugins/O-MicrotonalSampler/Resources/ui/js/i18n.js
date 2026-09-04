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
// i18n.js — O-MicrotonalSampler UI copy, English + French (v1.25.1, canon v2)
//
// ── v1.25.1: FRENCH QA PASS (Stage N, 2026-08-31) ───────────────────────────
// Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
// Changed: 73 values of 290, across 270 entries (28 terminology, 38 typography,
// 2 casing, 5 meaning; 0 grammar — the drafts were sound on agreement).
// sameAsEn: kept 9, added 4, translated 0. termNote exemptions: 5 (listed).
// Lint 74 -> 1, --strict NOT 0: the one residual is a glossary gap, below.
// Left as drafted: the other 217 values. reviewed: false throughout — no
// native speaker yet.
//
// THE DECISIONS THE NEXT READER NEEDS, each measured on THIS page at 900 x 640
// with Range.selectNodeContents on the real node in its own computed font,
// never inherited from a header:
//
//   THE CONTROL STRIP'S TWO VERDICTS GO OPPOSITE WAYS. The .ouaricon-knob cell
//   is 58.39-58.41 px and shrink-to-fit; Maintien at 55.69 is the tightest
//   survivor, as the v1.25.0 header said. DÉCLIN FITS AND CHUTE WAS WRONG:
//   Déclin measures 41.64 px against Chute's 38.78 — 16.75 px of clearance —
//   and Chute was drawing both a G1 and an F1 while the tip beside it already
//   said Déclin, which is two French names for one control (N1 correction 11).
//   RELÂCHEMENT DOES NOT: 83.84 px in a 58.41 px cell wraps to two lines, so
//   Relâch. (48.95) stays, and that half of the v1.25.0 header is CONFIRMED by
//   measurement rather than inherited. Dyn Rng had THREE French names — Ét.
//   dyn. on the knob, Plage dynamique in the tip, Amplitude dynamique in the
//   aria-label. Settled on Plage dynamique, and the caption moved to Pl. dyn.
//   (48.98, +0.64 px over Ét. dyn., 6.71 px inside Maintien) so the caption
//   stems from the accessible name. Label-in-name (WCAG 2.5.3) still does not
//   close on it — but it does not close in ENGLISH either (Dyn Rng is not a
//   substring of Dynamic Range), so no caption was invented to force it.
//
//   THE TUNING PANEL CONVERGES ON THE SETTLED FORMS, and three of them are
//   NARROWER than what shipped: Accord 41.20 vs Gamme 44.33, Rotation 33.53 vs
//   Rotations 38.03, Non octaviantes 71.72 vs Non-octaviantes 72.27. Also
//   applied: Intervalles de la gamme (104.16 in a 300 px block), Bibliothèque
//   de gammes (152.50 with 25.76 px of clearance to .library-toggle), RÉF. A4
//   (33.88), Tenir 2 notes ou plus… (234.61, 5.02 px NARROWER than the
//   imperative Tenez it replaces). Rotation is byte-equal to its English and
//   now carries sameAsEn: true, the same declaration O-Bells, O-IntonationPad,
//   O-Lyrica and O-Prism carry on the same string.
//
//   TWO TUNING-PANEL ROOTS DO NOT FIT, and the abbreviation the glossary lists
//   is the answer, not a CSS edit. Générateur (c) is 70.56 px and Harmonique de
//   fin is 88.81 px; both sit in the generator row, which already overhangs its
//   offsetParent in ENGLISH (genGenerator 36.3 px), and at those widths
//   check-ui-labels [8b] reports each of them intersecting .dynamics-mode-control
//   — a control they clear in English. Génér. (c) (48.00) and Harm. de fin
//   (59.33) ship. Harmonique de départ stays the root beside its abbreviated
//   twin because it fits: the asymmetry is measured, not an oversight.
//
//   ÉCART TOTAL IS 0.72 px TOO WIDE AND IS THE ONE LINT FINDING LEFT. The True
//   Keys total row is `justify-content: space-between` and its caption sits in
//   a `flex: 0 1 auto` <span> pinned at 53.00 px. Total span (en) is 52.91 and
//   Étendue is 42.22, so both sit inside the pin; Écart total is 53.72 and
//   forces the wrapper to 53.72, which check-ui-labels [7] reports as a
//   non-label element moved. Nothing else moves — .tk-cents stays at x=316.72
//   and the row height stays 23.39 in all three — but the baseline was 0 moved
//   and this stage does not spend it. The glossary lists no abbreviation for
//   `total span` and inventing a third form is forbidden, so Étendue ships and
//   the measurement is reported so the list can grow (Écart tot. would be 48.23
//   and fits, if the glossary chooses to accept it).
//
//   THE INTERVAL COUNT KEEPS ITS ABBREVIATION and the v1.25.0 header's REASON
//   is right while its NUMBER is stale: it says 114.45 px, the caption measures
//   119.67, and the full Intervalles · notes : {n} is 150.86 in the 142 px
//   column — so it would still wrap and still push the interval list 14 px.
//
//   CC N° / PC N° follow the English casing (C1). CC# and PC# carry no
//   lowercase letter, so the lint reads them as all-caps captions and the
//   French symbol word follows. Nothing on this page uppercases with CSS.
//
//   FIVE termNote EXEMPTIONS, and none of them is width hiding behind meaning:
//   label.ksLow / label.ksHigh and label.thLow / label.thHigh are the two ENDS
//   of a range read off a pair of number fields — MIDI note numbers in the
//   keyswitch row, controller VALUES 0-127 in the CC trigger table. Grave and
//   Aigu name a timbral register, which neither pair holds. On the keyswitch
//   pair the width agrees (Grave 30.00 in a 26.00 px box whose only clearance
//   is the 4.00 px gap to its own input); in the table it does NOT (grave fits
//   at 33.13 in a 104.66 px th), which is how you can tell the reason is the
//   meaning. aria.dynamicsMode keeps « après mixage » because that is the
//   mixing PROCESS the velocity layers are summed by — the post-mix trim of
//   the English — and there is no Mix control anywhere on this page.
//
//   VIDER BECAME EFFACER IN ALL EIGHT PLACES, not just the one the lint named.
//   `clear` roots on effacer; leaving Vider on the seven siblings would have
//   left one English verb with two French faces on one page (N2 correction 23).
//
//   sameAsEn ADDED to four straight copies the lint's fr === en condition
//   surfaced: tech. / tech. / Expr. are French abbreviations of technique and
//   expression, and Rotation is French. All four are declarations that a reader
//   looked, not gaps.
//
//   FOUR STRINGS SAID plugiciel, WHICH IS NOW FORBIDDEN IN PROSE — but the lint
//   did not report one of them. FORBIDDEN_IN_PROSE is scanned on `body` rows
//   only, and all four live in a LABELS `t:` (two missing-folder dialog
//   messages, two preset aria-labels), so they are prose that no prose check
//   reaches. Fixed here; reported to the orchestrator as a lint gap.
//
//   KEPT AS DRAFTED, deliberately: Détection auto for "Reset to auto-detect"
//   and Parcourir… for "Browse for folder…" (standard French button faces that
//   say what the button does); Valeurs par défaut for "Reset to defaults" (a
//   French UI noun phrase, and 89.08 px against the English's 82.56 in a
//   caption that already overhangs 32.2 px in English); the arrow form of
//   trimHint, because the fuller "Double-clic sur un curseur pour revenir à
//   0 dB" is 207.95 px against the English's 162.42 in a caption that already
//   overhangs its offsetParent by 214.7 px in ENGLISH, so it would spill MORE
//   in French; and STAN for DAW, which is the only French rendering in the
//   suite (2 occurrences, 0 for DAW).
//
//   THE ONE MEANING RESTORATION THE GEOMETRY REFUSED: rrBodyAfter's "explicit"
//   and "filenames" (463.03 px against the shipped 343.69) put a third line
//   into the round-robin dialog paragraph, moved 11 non-label elements by
//   20.3 px and made the caption intersect the three <code> filename tokens it
//   wraps. "couche de vélocité" in rrBodyBefore fits in the same two lines and
//   ships; "explicites … noms de fichiers" does not.
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
        fr: { t: 'Fichiers ignorés : {n}', b: '', reviewed: true } },
    'toast.nothingToDelete': {
        en: { t: 'Nothing to delete on that cell.', b: '' },
        fr: { t: 'Rien à supprimer dans cette case.', b: '', reviewed: true } },
    'toast.layerCleared': {
        en: { t: 'Layer {mark} · samples removed: {n}', b: '' },
        fr: { t: 'Couche {mark} · échantillons supprimés : {n}', b: '', reviewed: true } },
    'toast.layerAlreadyEmpty': {
        en: { t: 'Layer {mark} was already empty.', b: '' },
        fr: { t: 'La couche {mark} était déjà vide.', b: '', reviewed: true } },
    'toast.loopPointsApplied': {
        en: { t: 'Loop points applied · samples updated: {n}', b: '' },
        fr: { t: 'Points de boucle appliqués · échantillons mis à jour : {n}', b: '', reviewed: true } },
    'toast.noLoopableSamples': {
        en: { t: 'No loopable samples to update.', b: '' },
        fr: { t: 'Aucun échantillon bouclable à mettre à jour.', b: '', reviewed: true } },
    'toast.loopApplyNote': {
        en: { t: 'New loop points apply to next note-on.', b: '' },
        fr: { t: 'Les nouveaux points de boucle prennent effet à la prochaine note.', b: '', reviewed: true } },
    'toast.resizeWider': {
        en: { t: 'Resize wider to use the loop editor.', b: '' },
        fr: { t: 'Élargissez la fenêtre pour utiliser l’éditeur de boucle.', b: '', reviewed: true } },
    'toast.folderLoadFailed': {
        en: { t: 'Folder load failed', b: '' },
        fr: { t: 'Échec du chargement du dossier', b: '', reviewed: true } },
    'toast.waveformUnavailable': {
        en: { t: 'Unable to load waveform for this cell.', b: '' },
        fr: { t: 'Impossible de charger la forme d’onde de cette case.', b: '', reviewed: true } },
    'toast.presetSaved': {
        en: { t: 'Preset saved', b: '' },
        fr: { t: 'Préréglage enregistré', b: '', reviewed: true } },
    'toast.presetSaveFailed': {
        en: { t: 'Save preset failed', b: '' },
        fr: { t: 'Échec de l’enregistrement du préréglage', b: '', reviewed: true } },
    'toast.presetLoaded': {
        en: { t: 'Preset loaded', b: '' },
        fr: { t: 'Préréglage chargé', b: '', reviewed: true } },
    'toast.presetLoadFailed': {
        en: { t: 'Load preset failed', b: '' },
        fr: { t: 'Échec du chargement du préréglage', b: '', reviewed: true } },
    'toast.folderLocated': {
        en: { t: 'Folder located — loading…', b: '' },
        fr: { t: 'Dossier localisé — chargement…', b: '', reviewed: true } },
    'toast.locateFolderFailed': {
        en: { t: 'Locate folder failed', b: '' },
        fr: { t: 'Échec de la localisation du dossier', b: '', reviewed: true } },
    'toast.embedDialogMissing': {
        en: { t: 'Internal UI error: confirmation dialog unavailable — embed cancelled.', b: '' },
        fr: { t: 'Erreur interne : boîte de confirmation indisponible — intégration annulée.', b: '', reviewed: true } },
    'toast.rrDialogMissing': {
        en: { t: 'Internal UI error: round-robin confirmation dialog unavailable — load cancelled.', b: '' },
        fr: { t: 'Erreur interne : boîte de confirmation round-robin indisponible — chargement annulé.', b: '', reviewed: true } },
    'toast.techniquePresetApplied': {
        en: { t: 'Applied {family} technique names', b: '' },
        fr: { t: 'Noms de techniques « {family} » appliqués', b: '', reviewed: true } },

    // ── confirmation-dialog arguments ─────────────────────────────────────
    // showConfirmDialog() takes finished strings, not keys, so these resolve
    // through trLabel() at the call site.
    'msg.deleteSampleTitle': {
        en: { t: 'Delete this sample?', b: '' },
        fr: { t: 'Supprimer cet échantillon ?', b: '', reviewed: true } },
    'msg.deleteSampleBody': {
        en: { t: 'Remove the sample on {note}, velocity layer {mark}{tech}.', b: '' },
        fr: { t: 'Retirer l’échantillon sur {note}, couche de vélocité {mark}{tech}.', b: '', reviewed: true } },
    'msg.deleteBtn': {
        en: { t: 'Delete', b: '' },
        fr: { t: 'Supprimer', b: '', reviewed: true } },
    'msg.clearLayerTitle': {
        en: { t: 'Clear velocity layer {mark}?', b: '' },
        fr: { t: 'Effacer la couche de vélocité {mark} ?', b: '', reviewed: true } },
    'msg.clearLayerBody': {
        en: { t: 'Remove every sample in velocity layer {mark}, across all techniques. This cannot be undone.', b: '' },
        fr: { t: 'Retirer tous les échantillons de la couche de vélocité {mark}, pour toutes les techniques. Action irréversible.', b: '', reviewed: true } },
    'msg.clearLayerBtn': {
        en: { t: 'Clear layer', b: '' },
        fr: { t: 'Effacer la couche', b: '', reviewed: true } },
    'msg.clearAllTitle': {
        en: { t: 'Clear all samples?', b: '' },
        fr: { t: 'Effacer tous les échantillons ?', b: '', reviewed: true } },
    'msg.clearAllBody': {
        en: { t: 'All loaded samples will be removed from the sample map. Active notes will finish playing, but new note-ons will produce silence until samples are loaded again. This cannot be undone.', b: '' },
        fr: { t: 'Tous les échantillons chargés seront retirés de la carte d’échantillons. Les notes en cours iront à leur terme, mais les nouvelles notes resteront silencieuses jusqu’au prochain chargement. Action irréversible.', b: '', reviewed: true } },
    'msg.clearBtn': {
        en: { t: 'Clear', b: '' },
        fr: { t: 'Effacer', b: '', reviewed: true } },

    // The folder-load explain line: EIGHT keyed faces, not four ternaries.
    // The key is the branch, so nothing inflects inside a string.
    'msg.floAppendForced': {
        en: { t: 'Add samples to {layer}, ignoring filename velocity tokens.', b: '' },
        fr: { t: 'Ajouter les échantillons à {layer}, en ignorant les jetons de vélocité du nom de fichier.', b: '', reviewed: true } },
    'msg.floAppendTokens': {
        en: { t: 'Add samples; filename tokens (v1–v4, p/mp/mf/f) decide layer.', b: '' },
        fr: { t: 'Ajouter les échantillons ; les jetons du nom de fichier (v1–v4, p/mp/mf/f) décident de la couche.', b: '', reviewed: true } },
    'msg.floReplaceLayerForced': {
        en: { t: 'Clear {layer} and add the new samples there.', b: '' },
        fr: { t: 'Effacer {layer} et y ajouter les nouveaux échantillons.', b: '', reviewed: true } },
    'msg.floReplaceLayerTokens': {
        en: { t: 'Clear {layer}; filename tokens decide where new samples land.', b: '' },
        fr: { t: 'Effacer {layer} ; les jetons du nom de fichier décident où atterrissent les nouveaux échantillons.', b: '', reviewed: true } },
    'msg.floReplaceAllForced': {
        en: { t: 'Replace existing samples; new ones land on {layer}.', b: '' },
        fr: { t: 'Remplacer les échantillons existants ; les nouveaux atterrissent sur {layer}.', b: '', reviewed: true } },
    'msg.floReplaceAllTokens': {
        en: { t: 'Replace existing samples; filename tokens decide layer.', b: '' },
        fr: { t: 'Remplacer les échantillons existants ; les jetons du nom de fichier décident de la couche.', b: '', reviewed: true } },
    'msg.floMergeRrForced': {
        en: { t: 'Layer onto {layer}: collisions become round-robin variants (cap 64 per cell).', b: '' },
        fr: { t: 'Superposer sur {layer} : les collisions deviennent des variantes round-robin (max. 64 par case).', b: '', reviewed: true } },
    'msg.floMergeRrTokens': {
        en: { t: 'Layer existing notes: collisions become round-robin variants. Filename tokens decide layer.', b: '' },
        fr: { t: 'Superposer les notes existantes : les collisions deviennent des variantes round-robin. Les jetons du nom de fichier décident de la couche.', b: '', reviewed: true } },
    'msg.floTechniqueForced': {
        en: { t: 'Technique forced to "{name}".', b: '' },
        fr: { t: 'Technique forcée sur « {name} ».', b: '', reviewed: true } },

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
        fr: { t: 'Vél. {mark} ({range})', b: '', reviewed: true } },
    'aria.cellTech': {
        en: { t: 'tech: {name}', b: '' },
        fr: { t: 'technique : {name}', b: '', reviewed: true } },
    'aria.cellVariants': {
        en: { t: 'variants: {n}', b: '' },
        fr: { t: 'variantes : {n}', b: '', reviewed: true } },
    'aria.velLabel': {
        en: { t: 'Dynamic {mark} (layer {layer}): MIDI velocity {range} — right-click to clear this layer', b: '' },
        fr: { t: 'Nuance {mark} (couche {layer}) : vélocité MIDI {range} — clic droit pour effacer cette couche', b: '', reviewed: true } },
    'aria.switchToVariant': {
        en: { t: 'Switch to variant {n}', b: '' },
        fr: { t: 'Passer à la variante {n}', b: '', reviewed: true } },
    'aria.techTabLoaded': {
        en: { t: 'Technique {i}: {name} — cells loaded: {n}  (right-click to rename)', b: '' },
        fr: { t: 'Technique {i} : {name} — cases chargées : {n}  (clic droit pour renommer)', b: '', reviewed: true } },
    'aria.techTabEmpty': {
        en: { t: 'Technique {i}: {name} — empty  (right-click to rename)', b: '' },
        fr: { t: 'Technique {i} : {name} — vide  (clic droit pour renommer)', b: '', reviewed: true } },
    'aria.trimWholeTechnique': {
        en: { t: 'Trim the whole "{name}" technique (all layers)', b: '' },
        fr: { t: 'Ajuster toute la technique « {name} » (toutes les couches)', b: '', reviewed: true } },
    'aria.slotN': {
        en: { t: 'slot {n}', b: '' },
        fr: { t: 'emplacement {n}', b: '', reviewed: true } },
    'aria.midiN': {
        en: { t: 'MIDI {n}', b: '' },
        fr: { t: 'MIDI {n}', b: '', reviewed: true, sameAsEn: true } },
    'aria.loaded': {
        en: { t: 'Loaded', b: '' },
        fr: { t: 'Chargé', b: '', reviewed: true } },
    'aria.unnamed': {
        en: { t: '(unnamed)', b: '' },
        fr: { t: '(sans nom)', b: '', reviewed: true } },
    'aria.unknown': {
        en: { t: '(unknown)', b: '' },
        fr: { t: '(inconnu)', b: '', reviewed: true } },
    'aria.emptyPath': {
        en: { t: '(empty path)', b: '' },
        fr: { t: '(chemin vide)', b: '', reviewed: true } },

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
              b: 'Temps de montée entre l’enfoncement de la note et le niveau maximal. Augmentez-le pour adoucir l’attaque percussive d’un échantillon ; laissez-le près de zéro pour conserver le transitoire de l’enregistrement. 0 à 10 s.',
              reviewed: true } },
    'tip.decay': {
        en: { t: 'Decay',
              b: 'Time to fall from the peak to the sustain level, once the attack has finished. It only bites when Sustain sits below 1.00. 0 to 10 s.' },
        fr: { t: 'Déclin',
              b: 'Temps de descente du sommet vers le niveau de maintien, une fois l’attaque terminée. Il n’agit que si le maintien est inférieur à 1,00. 0 à 10 s.',
              reviewed: true } },
    'tip.sustain': {
        en: { t: 'Sustain',
              b: 'Level a held note settles at after the decay, as a fraction of the sample’s own level. At 1.00 the sample plays untouched and Decay does nothing. 0.00 to 1.00.' },
        fr: { t: 'Maintien',
              b: 'Niveau auquel se stabilise une note tenue après le déclin, en fraction du niveau propre de l’échantillon. À 1,00 l’échantillon est joué tel quel et le déclin n’a aucun effet. 0,00 à 1,00.',
              reviewed: true } },
    'tip.release': {
        en: { t: 'Release',
              b: 'Fade-out time after note-off. A long value lets a hall tail ring on; a short one cuts the note clean. 0 to 10 s.' },
        fr: { t: 'Relâchement',
              b: 'Temps de descente après le relâchement de la note. Une valeur longue laisse la queue de salle résonner ; une valeur courte coupe la note net. 0 à 10 s.',
              reviewed: true } },
    'tip.polyphony': {
        en: { t: 'Polyphony',
              b: 'Greatest number of notes that may sound at once. Lower it to cap CPU on a large library; past the limit the oldest voice is stolen. 1 to 16 voices.' },
        fr: { t: 'Polyphonie',
              b: 'Nombre maximal de notes pouvant sonner en même temps. Abaissez-le pour limiter le processeur sur une grande banque ; au-delà de la limite, la voix la plus ancienne est remplacée. 1 à 16 voix.',
              reviewed: true } },
    'tip.velocityCrossfade': {
        en: { t: 'Velocity Crossfade',
              b: 'How far neighbouring velocity layers blend into one another instead of switching abruptly. At 0.00 each layer starts exactly where the one below it stops. 0.00 to 1.00.' },
        fr: { t: 'Fondu de vélocité',
              b: 'Degré de fondu entre couches de vélocité voisines, au lieu d’un basculement net. À 0,00 chaque couche commence exactement là où s’arrête la précédente. 0,00 à 1,00.',
              reviewed: true } },
    'tip.expression': {
        en: { t: 'Expression',
              b: 'Overall playing level, driven live by MIDI CC 11. What it does depends on the Dynamics mode: a post-mix volume trim under Velocity, a layer morph under CC Crossfade. 0 to 100 %.' },
        fr: { t: 'Expression',
              b: 'Niveau de jeu global, piloté en direct par le CC MIDI 11. Son effet dépend du mode de dynamique : un simple réglage de volume après mixage en Velocity, un fondu entre couches en CC Crossfade. 0 à 100 %.',
              reviewed: true } },
    'tip.dynamicsMode': {
        en: { t: 'Dynamics Mode',
              b: 'Chooses what MIDI CC 11 controls. Velocity: note-on velocity picks the layer and CC 11 is only a volume trim. CC Crossfade: CC 11 morphs across every velocity layer mid-note, changing timbre as well as loudness.' },
        fr: { t: 'Mode de dynamique',
              b: 'Détermine ce que pilote le CC MIDI 11. Velocity : la vélocité choisit la couche et le CC 11 n’est qu’un réglage de volume. CC Crossfade : le CC 11 fond toutes les couches de vélocité en cours de note, changeant le timbre autant que l’intensité.',
              reviewed: true } },
    'tip.dynamicRange': {
        en: { t: 'Dynamic Range',
              b: 'How much quieter the softest layer sits below the loudest under CC Crossfade. Widen it for an exposed orchestral line, narrow it for a mix that must stay present throughout. 0.0 to 40.0 dB.' },
        fr: { t: 'Plage dynamique',
              b: 'Différence de niveau entre la couche la plus douce et la plus forte, en mode CC Crossfade. Élargissez-la pour une ligne orchestrale exposée, resserrez-la pour un mixage qui doit rester présent de bout en bout. 0,0 à 40,0 dB.',
              reviewed: true } },
    'tip.outputGain': {
        en: { t: 'Output Gain',
              b: 'Final level of the whole instrument, applied after every technique and layer trim. Use it to seat this instance against the rest of the mix. −24 to +24 dB.' },
        fr: { t: 'Gain de sortie',
              b: 'Niveau final de tout l’instrument, appliqué après chaque ajustement de technique et de couche. Utilisez-le pour caler cette instance sur le reste du mixage. −24 à +24 dB.',
              reviewed: true } },
    'tip.techniqueCount': {
        en: { t: 'Technique Count',
              b: 'How many playing-technique slots this instrument exposes. Add one for each articulation you have samples for; removing a slot hides its cells rather than deleting them. 1 to 8 slots.' },
        fr: { t: 'Nombre de techniques',
              b: 'Nombre d’emplacements de technique de jeu proposés par l’instrument. Ajoutez-en un par articulation dont vous avez des échantillons ; en retirer un masque ses cases sans les supprimer. 1 à 8 emplacements.',
              reviewed: true } },
    'tip.techniqueSelect': {
        en: { t: 'Technique Select',
              b: 'The technique slot currently sounding, and the one the sample map and the trims are editing. Click a tab to switch, right-click a tab to rename it. Slots 1 to 8.' },
        fr: { t: 'Technique active',
              b: 'L’emplacement de technique en cours de lecture, celui que la carte d’échantillons et les ajustements modifient. Cliquez sur un onglet pour changer, clic droit pour le renommer. Emplacements 1 à 8.',
              reviewed: true } },
    'tip.ksEnabled': {
        en: { t: 'Keyswitch Enabled',
              b: 'Turns keyswitching on: a note-on inside the range beside it selects a technique instead of sounding. It is off by default, so no note is ever swallowed until you ask for it. Off or On.' },
        fr: { t: 'Commutation par touche',
              b: 'Active la commutation par touche : une note jouée dans la plage voisine choisit une technique au lieu de sonner. Elle est désactivée par défaut, afin qu’aucune note ne soit absorbée sans votre accord. Arrêt ou Marche.',
              reviewed: true } },
    'tip.ksLowNote': {
        en: { t: 'Keyswitch Low Note',
              b: 'Bottom of the keyswitch range, as a MIDI note number. Keep it below the register you actually play — the default, MIDI 0, is well out of the way. 0 to 127.' },
        fr: { t: 'Note basse de commutation',
              b: 'Limite inférieure de la plage de commutation, en numéro de note MIDI. Gardez-la sous le registre que vous jouez réellement ; la valeur par défaut, MIDI 0, est largement à l’écart. 0 à 127.',
              reviewed: true } },
    'tip.ksHighNote': {
        en: { t: 'Keyswitch High Note',
              b: 'Top of the keyswitch range. Leave one semitone per slot above the low note, or two keys collapse onto the same technique. 0 to 127.' },
        fr: { t: 'Note haute de commutation',
              b: 'Limite supérieure de la plage de commutation. Laissez un demi-ton par emplacement au-dessus de la note basse, sans quoi deux touches visent la même technique. 0 à 127.',
              reviewed: true } },
    'tip.ccSelectEnabled': {
        en: { t: 'CC Select Enabled',
              b: 'Lets a MIDI controller choose the technique from its value, through the table below. Keyswitching still takes precedence wherever both are active. Off or On.' },
        fr: { t: 'Sélection par CC',
              b: 'Permet à un contrôleur MIDI de choisir la technique selon sa valeur, d’après le tableau ci-dessous. La commutation par touche reste prioritaire là où les deux sont actives. Arrêt ou Marche.',
              reviewed: true } },
    'tip.ccNumber': {
        en: { t: 'CC Number',
              b: 'Which MIDI controller drives technique selection. CC 32 is the default; avoid CC 11, which Expression already listens to. 0 to 119.' },
        fr: { t: 'Numéro de CC',
              b: 'Contrôleur MIDI qui pilote la sélection de technique. Le CC 32 est la valeur par défaut ; évitez le CC 11, déjà écouté par Expression. 0 à 119.',
              reviewed: true } },
    'tip.pcEnabled': {
        en: { t: 'Program Change Enabled',
              b: 'Lets a MIDI program change select the technique, through the table below. It comes last in precedence, after keyswitching and CC. Off or On.' },
        fr: { t: 'Changement de programme',
              b: 'Permet à un changement de programme MIDI de choisir la technique, d’après le tableau ci-dessous. Il vient en dernier, après la commutation par touche et le CC. Arrêt ou Marche.',
              reviewed: true } },

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
              reviewed: true } },
    'tip.langSelect': {
        en: { t: 'Language',
              b: 'Switches every caption, button and hover-help on this page between English and French. Value readouts, tuning names, note names and preset filenames stay exactly as they are.' },
        fr: { t: 'Langue',
              b: 'Bascule toutes les légendes, tous les boutons et toutes les infobulles de cette page entre l’anglais et le français. Les valeurs affichées, les noms de systèmes d’accord, les noms de notes et les noms de fichiers de préréglages restent inchangés.',
              reviewed: true } },
    // v1.26.0 — the switch that reaches this whole layer.
    'tip.tipsToggle': {
        en: { t: 'Hover Help',
              b: 'Turns this hover help on and off. With it off, only the gear and this '
               + 'switch keep explaining themselves.' },
        fr: { t: 'Infobulles',
              b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
               + 'l’engrenage et ce commutateur continuent de s’expliquer.',
              reviewed: true },
    },
});

// ── LABELS — one string, no body ───────────────────────────────────────────
export const LABELS = Object.freeze({

    // ── Header ────────────────────────────────────────────────────────────
    // The <h1> is NOT keyed: "O-MicrotonalSampler" is the product name and is
    // I18N_EXEMPT below. #tuning-readout holds the active tuning's name, which
    // is data from the C++ engine.
    'label.tabSampleMap':  { en: { t: 'Sample Map' },   fr: { t: 'Échantillons',  reviewed: true } },
    'label.tabTuning':     { en: { t: 'Tuning' },       fr: { t: 'Accord',         reviewed: true } },
    'label.tabAbout':      { en: { t: 'About' },        fr: { t: 'À propos',      reviewed: true } },
    'label.savePreset':    { en: { t: 'Save Preset…' }, fr: { t: 'Enreg. prér.…', reviewed: true } },
    'label.loadPreset':    { en: { t: 'Load Preset…' }, fr: { t: 'Ouvrir prér.…', reviewed: true } },
    'label.language':      { en: { t: 'Language' },     fr: { t: 'Langue',        reviewed: true } },

    // v1.26.0. All four renderings below are settled glossary ROOTS, copied
    // rather than authored: scripts/i18n-fr-glossary.js carries them as the
    // roots for 'hover help', 'on', 'off' and 'toggle hover help'. They take
    // the same review mark this file's other roots carry, and for the same
    // reason — they are not new machine output.
    'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },
    'ui.on':           { en: { t: 'On' },         fr: { t: 'Marche', reviewed: true } },
    'ui.off':          { en: { t: 'Off' },        fr: { t: 'Arrêt',  reviewed: true } },

    // ── Folder drop zone ──────────────────────────────────────────────────
    'label.dropFolderHere': { en: { t: 'Drop folder here' }, fr: { t: 'Déposer un dossier ici', reviewed: true } },
    'label.or':             { en: { t: 'or' },              fr: { t: 'ou',                     reviewed: true } },
    'label.loadFolder':     { en: { t: 'Load Folder…' },    fr: { t: 'Charger dossier…',       reviewed: true } },
    'label.batchLoop':      { en: { t: 'Batch loop…' },     fr: { t: 'Boucles en lot…',        reviewed: true } },
    'label.clearSamples':   { en: { t: 'Clear samples' },   fr: { t: 'Effacer',                  reviewed: true } },

    // ── Technique preset row ──────────────────────────────────────────────
    // The four family names are CAPTIONS: the <option> VALUES the code keys on
    // are 'strings' … 'generic' and are untouched, and none of the four is an
    // AudioParameterChoice option or a JSON preset filename. The toast that
    // reports the change reads these same four keys, so the two cannot drift.
    'label.techniquePreset': { en: { t: 'Technique preset' }, fr: { t: 'Préréglage de techniques', reviewed: true } },
    'label.chooseFamily':    { en: { t: 'Choose family…' },   fr: { t: 'Choisir une famille…',    reviewed: true } },
    'label.familyStrings':   { en: { t: 'Strings' },          fr: { t: 'Cordes',                  reviewed: true } },
    'label.familyWinds':     { en: { t: 'Winds' },            fr: { t: 'Bois',                    reviewed: true } },
    'label.familyBrass':     { en: { t: 'Brass' },            fr: { t: 'Cuivres',                 reviewed: true } },
    'label.familyGeneric':   { en: { t: 'Generic' },          fr: { t: 'Générique',               reviewed: true } },

    // ── Keyswitch controls ────────────────────────────────────────────────
    // KS is the standard abbreviation for a keyswitch in both languages, and
    // Dorico's own French UI keeps it; sameAsEn is a decision, not a gap.
    'label.ks':     { en: { t: 'KS' },   fr: { t: 'KS',   reviewed: true, sameAsEn: true } },
    'label.ksLow':  { en: { t: 'low' },  fr: { t: 'min',  reviewed: true,
                                                termNote: 'the two BOUNDS of a MIDI note-number range, read off the pair of number fields beside them — Grave/Aigu name a timbral register, not a range end. Width says the same: Grave is 30.00 px in a 26.00 px box whose only clearance is the 4.00 px gap to its own input' } },
    'label.ksHigh': { en: { t: 'high' }, fr: { t: 'max',  reviewed: true,
                                                termNote: 'the two BOUNDS of a MIDI note-number range, read off the pair of number fields beside them — Grave/Aigu name a timbral register, not a range end. Width says the same: Grave is 30.00 px in a 26.00 px box whose only clearance is the 4.00 px gap to its own input' } },

    // ── Trigger panel ─────────────────────────────────────────────────────
    'label.triggersHeading':   { en: { t: 'Triggers (CC + PC)' },            fr: { t: 'Déclencheurs (CC + PC)',        reviewed: true } },
    'label.triggerPrecedence': { en: { t: 'KS > CC > PC > history' },        fr: { t: 'KS > CC > PC > historique',     reviewed: true } },
    'label.ccTrigger':         { en: { t: 'CC trigger' },                    fr: { t: 'Déclencheur CC',                reviewed: true } },
    'label.ccNumber':          { en: { t: 'CC#' },                           fr: { t: 'CC N°',                         reviewed: true } },
    'label.ccHint':            { en: { t: 'value 0–127 → technique slot' },  fr: { t: 'valeur 0–127 → emplacement',    reviewed: true } },
    'label.pcTrigger':         { en: { t: 'PC trigger' },                    fr: { t: 'Déclencheur PC',                reviewed: true } },
    'label.pcHint':            { en: { t: 'program # → technique slot' },    fr: { t: 'n° de programme → emplacement', reviewed: true } },
    // The two tables have IDENTICAL headers, and a key is owned by ONE
    // element: two <th>slot</th> nodes need two keys or the second is never
    // swept. Hence the `2` suffixes rather than a shared key.
    'label.thSlot':  { en: { t: 'slot' }, fr: { t: 'empl.', reviewed: true } },
    'label.thLow':   { en: { t: 'low' },  fr: { t: 'min',   reviewed: true,
                                                 termNote: 'in the CC-trigger table, low/high are controller VALUES 0-127, not pitches; Grave/Aigu would name a register this column never holds (it fits at 33.13 px in a 104.66 px th, so this is meaning, not width)' } },
    'label.thHigh':  { en: { t: 'high' }, fr: { t: 'max',   reviewed: true,
                                                 termNote: 'in the CC-trigger table, low/high are controller VALUES 0-127, not pitches; Grave/Aigu would name a register this column never holds (it fits at 33.13 px in a 104.66 px th, so this is meaning, not width)' } },
    'label.thTech':  { en: { t: 'tech' }, fr: { t: 'tech.', reviewed: true, sameAsEn: true } },
    'label.thSlot2': { en: { t: 'slot' }, fr: { t: 'empl.', reviewed: true } },
    'label.thPc':    { en: { t: 'PC#' },  fr: { t: 'PC N°', reviewed: true } },
    'label.thTech2': { en: { t: 'tech' }, fr: { t: 'tech.', reviewed: true, sameAsEn: true } },
    'label.resetDefaults': { en: { t: 'Reset to defaults' }, fr: { t: 'Valeurs par défaut', reviewed: true } },

    // ── Trim panel ────────────────────────────────────────────────────────
    // #trim-active-tech is NOT keyed: it holds a technique NAME the user can
    // rename, so it is a readout node (D-01 arm 3) and its placeholder "ord" is
    // I18N_EXEMPT. The four layer captions p / mp / mf / f are dynamic markings
    // — Italian musical notation, not English — and are exempt too.
    'label.trimsHeading':   { en: { t: 'Trims (loudness)' },                       fr: { t: 'Ajustements (niveau)',                       reviewed: true } },
    'label.trimTechnique':  { en: { t: 'Technique' },                              fr: { t: 'Technique',                                  reviewed: true, sameAsEn: true } },
    'label.trimHint':       { en: { t: 'Double-click a slider to reset to 0 dB' }, fr: { t: 'Double-clic sur un curseur → 0 dB', reviewed: true } },
    'label.resetAllTrims':  { en: { t: 'Reset all trims' },                        fr: { t: 'Tout réinitialiser',                         reviewed: true } },

    // ── Grid context menu + issues ────────────────────────────────────────
    'label.ctxReplace':        { en: { t: 'Replace…' },         fr: { t: 'Remplacer…',             reviewed: true } },
    'label.ctxOpenLoopEditor': { en: { t: 'Open Loop Editor' }, fr: { t: 'Ouvrir l’éditeur de boucle', reviewed: true } },
    'label.ctxDeleteSample':   { en: { t: 'Delete sample' },    fr: { t: 'Supprimer l’échantillon', reviewed: true } },
    'label.issues':            { en: { t: 'Issues' },           fr: { t: 'Problèmes',              reviewed: true } },
    // §6 — the count sits after a colon beside an invariant plural noun
    // phrase, so nothing inflects at 0, 1 or n in either language.
    'label.issuesSummary':     { en: { t: 'Issues · files skipped: {n}' },
                                 fr: { t: 'Problèmes · fichiers ignorés : {n}', reviewed: true } },

    // ── Loop editor ───────────────────────────────────────────────────────
    'label.loopPlaceholder': { en: { t: 'Select a loaded sample slot to edit loop points' },
                               fr: { t: 'Sélectionnez une case chargée pour modifier ses points de boucle', reviewed: true } },
    'label.loopStartCap':    { en: { t: 'Loop start:' },          fr: { t: 'Début de boucle :',   reviewed: true } },
    'label.loopEndCap':      { en: { t: 'Loop end:' },            fr: { t: 'Fin de boucle :',     reviewed: true } },
    'label.loopModeCap':     { en: { t: 'Mode:' },                fr: { t: 'Mode :',              reviewed: true } },
    'label.loopReset':       { en: { t: 'Reset to auto-detect' }, fr: { t: 'Détection auto',      reviewed: true } },
    'label.loopCancel':      { en: { t: 'Cancel' },               fr: { t: 'Annuler',             reviewed: true } },
    'label.loopApply':       { en: { t: 'Apply' },                fr: { t: 'Appliquer',           reviewed: true } },
    'label.variantOf':       { en: { t: 'Variant {i} of {n}' },   fr: { t: 'Variante {i} sur {n}', reviewed: true } },

    // ── Generic confirmation dialog ───────────────────────────────────────
    // EVERY dialog's Cancel button carries its OWN key. A key is owned by one
    // element (contract §1) and applyI18n sweeps per element, so seven dialogs
    // sharing one 'label.cancel' would leave six of them unswept.
    'label.areYouSure': { en: { t: 'Are you sure?' }, fr: { t: 'Êtes-vous sûr ?', reviewed: true } },
    'label.cancel':     { en: { t: 'Cancel' },        fr: { t: 'Annuler',         reviewed: true } },
    'label.confirm':    { en: { t: 'Confirm' },       fr: { t: 'Confirmer',       reviewed: true } },

    // ── Batch loop dialog ─────────────────────────────────────────────────
    'label.batchLoopTitle':  { en: { t: 'Batch loop points' }, fr: { t: 'Points de boucle en lot', reviewed: true } },
    'label.batchLoopBlurb':  { en: { t: 'Apply one loop region to every loaded sample at once. One-shot samples (too short to loop) are left untouched.' },
                               fr: { t: 'Applique une même région de boucle à tous les échantillons chargés. Les one-shot (trop courts pour boucler) restent intacts.', reviewed: true } },
    'label.blUnits':         { en: { t: 'Units' },             fr: { t: 'Unités',           reviewed: true } },
    'label.blProportional':  { en: { t: 'Proportional (%)' },  fr: { t: 'Proportionnel (%)', reviewed: true } },
    'label.blMilliseconds':  { en: { t: 'Milliseconds' },      fr: { t: 'Millisecondes',    reviewed: true } },
    'label.blLoopStart':     { en: { t: 'Loop start' },        fr: { t: 'Début de boucle',  reviewed: true } },
    'label.blLoopEnd':       { en: { t: 'Loop end' },          fr: { t: 'Fin de boucle',    reviewed: true } },
    'label.cancelBl':        { en: { t: 'Cancel' },            fr: { t: 'Annuler',          reviewed: true } },
    'label.blApplyAll':      { en: { t: 'Apply to all' },      fr: { t: 'Tout appliquer',   reviewed: true } },
    'label.blErrPercent':    { en: { t: 'Start and end must be 0–100 %, with end greater than start.' },
                               fr: { t: 'Début et fin doivent être entre 0 et 100 %, la fin après le début.', reviewed: true } },
    'label.blErrMs':         { en: { t: 'Start and end must be in ms, with end greater than start.' },
                               fr: { t: 'Début et fin doivent être en ms, la fin après le début.', reviewed: true } },

    // ── Diagnostic dialog ─────────────────────────────────────────────────
    'label.diagnostic':      { en: { t: 'Diagnostic' },  fr: { t: 'Diagnostic', reviewed: true, sameAsEn: true } },
    'label.diagnosticHint':  { en: { t: 'Auto-copied to clipboard. Select the text below and ⌘C if it wasn\'t.' },
                               fr: { t: 'Copié automatiquement dans le presse-papiers. Sinon, sélectionnez le texte ci-dessous puis ⌘C.', reviewed: true } },
    'label.diagCopied':      { en: { t: 'Auto-copied to clipboard. (Select below + ⌘C if you need it again.)' },
                               fr: { t: 'Copié automatiquement dans le presse-papiers. (Sélectionnez ci-dessous + ⌘C si besoin.)', reviewed: true } },
    'label.diagCopyBlocked': { en: { t: 'Clipboard write blocked — select the text below and ⌘C to copy.' },
                               fr: { t: 'Écriture dans le presse-papiers bloquée — sélectionnez le texte ci-dessous puis ⌘C.', reviewed: true } },
    'label.copyAgain':       { en: { t: 'Copy again' },  fr: { t: 'Copier à nouveau', reviewed: true } },
    'label.copied':          { en: { t: 'Copied ✓' },    fr: { t: 'Copié ✓',          reviewed: true } },
    'label.copyFailed':      { en: { t: 'Copy failed' }, fr: { t: 'Échec de la copie', reviewed: true } },
    'label.close':           { en: { t: 'Close' },       fr: { t: 'Fermer',           reviewed: true } },

    // ── Missing-folder dialog ─────────────────────────────────────────────
    // The highest-value strings on this page: they are what a French user reads
    // when a reopened session cannot find its samples. Each face is keyed
    // separately rather than interpolated, so neither reads as a sentence with
    // a hole in it.
    'label.folderNotFound':            { en: { t: 'Sample folder not found' }, fr: { t: 'Dossier d’échantillons introuvable', reviewed: true } },
    'label.folderNotFoundMsgNamed':    { en: { t: 'The sample folder "{name}" was not found at its saved location. Locate it now, or skip and load samples manually.' },
                                         fr: { t: 'Le dossier d’échantillons « {name} » est introuvable à son emplacement enregistré. Localisez-le maintenant, ou passez et chargez les échantillons manuellement.', reviewed: true } },
    'label.folderNotFoundMsgUnnamed':  { en: { t: 'The saved sample folder was not found. Locate it now, or skip and load samples manually.' },
                                         fr: { t: 'Le dossier d’échantillons enregistré est introuvable. Localisez-le maintenant, ou passez et chargez les échantillons manuellement.', reviewed: true } },
    'label.dragDropNotEmbedded':       { en: { t: 'Drag-dropped samples not embedded' }, fr: { t: 'Échantillons déposés non intégrés', reviewed: true } },
    'label.dragDropMsgNamed':          { en: { t: 'Samples were drag-dropped from "{name}" without "Embed audio" enabled, so they could not be re-loaded automatically. Re-drag the folder onto the plugin, or browse to its current location.' },
                                         fr: { t: 'Les échantillons ont été déposés depuis « {name} » sans « Intégrer l’audio », ils n’ont donc pas pu être rechargés automatiquement. Redéposez le dossier sur le plugin, ou parcourez jusqu’à son emplacement actuel.', reviewed: true } },
    'label.dragDropMsgUnnamed':        { en: { t: 'Samples were drag-dropped without "Embed audio" enabled, so they could not be re-loaded automatically. Re-drag the folder onto the plugin, or browse to its current location.' },
                                         fr: { t: 'Les échantillons ont été déposés sans « Intégrer l’audio », ils n’ont donc pas pu être rechargés automatiquement. Redéposez le dossier sur le plugin, ou parcourez jusqu’à son emplacement actuel.', reviewed: true } },
    'label.skip':                      { en: { t: 'Skip' },              fr: { t: 'Passer',             reviewed: true } },
    'label.locateFolder':              { en: { t: 'Locate folder…' },    fr: { t: 'Localiser le dossier…', reviewed: true } },
    'label.browseForFolder':           { en: { t: 'Browse for folder…' }, fr: { t: 'Parcourir…',        reviewed: true } },

    // ── Round-robin confirmation ──────────────────────────────────────────
    // SPLIT per §5: the paragraph wraps three <code> filename tokens, which are
    // literals the C++ FilenameParser matches and must not be translated.
    'label.rrTitle':      { en: { t: 'Multiple samples for the same note' }, fr: { t: 'Plusieurs échantillons pour la même note', reviewed: true } },
    'label.rrBodyBefore': { en: { t: 'The folder contains more than one sample for the same note and velocity layer, without explicit' },
                            fr: { t: 'Le dossier contient plusieurs échantillons pour la même note et la même couche de vélocité, sans jetons', reviewed: true } },
    'label.rrBodyAfter':  { en: { t: 'tokens in the filenames. Treat them as round-robin variants?' },
                            fr: { t: 'dans les noms. Les traiter comme des variantes round-robin ?', reviewed: true } },
    'label.rrCancel':     { en: { t: 'Cancel load' },       fr: { t: 'Annuler le chargement', reviewed: true } },
    'label.rrAccept':     { en: { t: 'Treat as variants' }, fr: { t: 'Traiter en variantes',  reviewed: true } },
    'label.rrCellHead':   { en: { t: 'MIDI {n} · {mark}' }, fr: { t: 'MIDI {n} · {mark}',     reviewed: true, sameAsEn: true } },

    // ── Folder-load options dialog ────────────────────────────────────────
    'label.floTitle':            { en: { t: 'Load samples' },          fr: { t: 'Charger des échantillons', reviewed: true } },
    'label.floLayer':            { en: { t: 'Layer' },                 fr: { t: 'Couche',                   reviewed: true } },
    'label.floTechnique':        { en: { t: 'Technique' },             fr: { t: 'Technique', reviewed: true, sameAsEn: true } },
    'label.floWhenLoading':      { en: { t: 'When loading' },          fr: { t: 'Au chargement',            reviewed: true } },
    'label.floAddToLayer':       { en: { t: 'Add to layer' },          fr: { t: 'Ajouter à la couche',      reviewed: true } },
    'label.floReplaceLayer':     { en: { t: 'Replace this layer' },    fr: { t: 'Remplacer cette couche',   reviewed: true } },
    'label.floReplaceAll':       { en: { t: 'Replace all samples' },   fr: { t: 'Remplacer tout',           reviewed: true } },
    'label.floMergeRr':          { en: { t: 'Layer as round-robin' },  fr: { t: 'Superposer en round-robin', reviewed: true } },
    'label.floForceLayer':       { en: { t: 'Force all samples onto this layer' },     fr: { t: 'Forcer tous les échantillons sur cette couche',    reviewed: true } },
    'label.floForceTechnique':   { en: { t: 'Force all samples onto this technique' }, fr: { t: 'Forcer tous les échantillons sur cette technique', reviewed: true } },
    // SPLIT per §5 around the <code> filename tokens. Two keys per half,
    // because the same English opens both lines and each <span> is its own
    // element with its own key.
    'label.floTokensBefore':     { en: { t: 'Otherwise filename tokens (' }, fr: { t: 'Sinon les jetons du fichier (', reviewed: true } },
    'label.floTokensBefore2':    { en: { t: 'Otherwise filename tokens (' }, fr: { t: 'Sinon les jetons du fichier (', reviewed: true } },
    'label.floTokensAfterLayer': { en: { t: ', etc.) decide the layer' },     fr: { t: ', etc.) décident de la couche',        reviewed: true } },
    'label.floTokensAfterTech':  { en: { t: ', etc.) decide the technique' }, fr: { t: ', etc.) décident de la technique',     reviewed: true } },
    'label.floEmbed':            { en: { t: 'Embed audio in project state' }, fr: { t: 'Intégrer l’audio dans le projet', reviewed: true } },
    'label.floEmbedSize':        { en: { t: 'Project state will grow by ~{size}.' },
                                   fr: { t: 'L’état du projet augmentera d’environ {size}.', reviewed: true } },
    'label.floEmbedSizePending': { en: { t: 'Size will be confirmed after folder selection.' },
                                   fr: { t: 'La taille sera confirmée après le choix du dossier.', reviewed: true } },
    'label.cancelFlo':           { en: { t: 'Cancel' }, fr: { t: 'Annuler', reviewed: true } },
    'label.floLoad':             { en: { t: 'Load…' },  fr: { t: 'Charger…', reviewed: true } },

    // ── Embed-size confirmation ───────────────────────────────────────────
    'label.embedTitle':      { en: { t: 'Embed audio in project?' }, fr: { t: 'Intégrer l’audio au projet ?', reviewed: true } },
    'label.embedDetail':     { en: { t: 'The audio data will be written into your DAW project file. Project saves and reopens will be slower for large libraries, but the samples will travel with the project across folders and machines.' },
                               fr: { t: 'Les données audio seront écrites dans le fichier de projet de votre STAN. Les enregistrements et réouvertures seront plus lents pour les grandes bibliothèques, mais les échantillons suivront le projet d’un dossier ou d’une machine à l’autre.', reviewed: true } },
    'label.embedMsgNamed':   { en: { t: 'Embedding folder "{name}" will add ~{size} to your project state.' },
                               fr: { t: 'Intégrer le dossier « {name} » ajoutera environ {size} à l’état du projet.', reviewed: true } },
    'label.embedMsgUnnamed': { en: { t: 'Embedding this folder will add ~{size} to your project state.' },
                               fr: { t: 'Intégrer ce dossier ajoutera environ {size} à l’état du projet.', reviewed: true } },
    'label.cancelEmbed':     { en: { t: 'Cancel' },         fr: { t: 'Annuler',           reviewed: true } },
    'label.embedAndLoad':    { en: { t: 'Embed and load' }, fr: { t: 'Intégrer et charger', reviewed: true } },

    // ── Per-cell merge dialog ─────────────────────────────────────────────
    // §6: v1.23.10 built `1 variant` / `${n} variants` with a ternary and then
    // interpolated it mid-sentence. Both faces now carry the count after a
    // colon instead, so neither inflects.
    'label.mergeTitle':        { en: { t: 'Cell already has samples' }, fr: { t: 'La case contient déjà des échantillons', reviewed: true } },
    'label.mergeMsgCapped':    { en: { t: '{note} layer {mark} is at the maximum · variants held: {n}. Replace the cell, or cancel.' },
                                 fr: { t: '{note} couche {mark} est au maximum · variantes présentes : {n}. Remplacez la case, ou annulez.', reviewed: true } },
    'label.mergeMsgAdd':       { en: { t: '{note} layer {mark} · variants held: {n}. Add this sample as round-robin variant {next}, or replace the cell?' },
                                 fr: { t: '{note} couche {mark} · variantes présentes : {n}. Ajouter cet échantillon comme variante round-robin {next}, ou remplacer la case ?', reviewed: true } },
    'label.cancelMerge':       { en: { t: 'Cancel' },             fr: { t: 'Annuler',            reviewed: true } },
    'label.mergeReplaceCell':  { en: { t: 'Replace cell' },       fr: { t: 'Remplacer la case',  reviewed: true } },
    'label.mergeAddRr':        { en: { t: 'Add as round-robin' }, fr: { t: 'Ajouter en round-robin', reviewed: true } },

    // ── Technique rename dialog ───────────────────────────────────────────
    // SPLIT per §5: the paragraph wraps the live slot number.
    'label.renameTitle':  { en: { t: 'Rename technique' }, fr: { t: 'Renommer la technique', reviewed: true } },
    'label.renameSlot':   { en: { t: 'Slot' },             fr: { t: 'Empl.',                 reviewed: true } },
    'label.renameBody':   { en: { t: ': enter a new name. Names appear in the tab strip and Dorico expression maps.' },
                            fr: { t: ' : saisissez un nouveau nom. Il apparaît dans la barre d’onglets et les cartes d’expression Dorico.', reviewed: true } },
    'label.cancelRename': { en: { t: 'Cancel' }, fr: { t: 'Annuler',   reviewed: true } },
    'label.renameSave':   { en: { t: 'Save' },   fr: { t: 'Enregistrer', reviewed: true } },

    // ── About ─────────────────────────────────────────────────────────────
    // The two headings are the product name and the company name — exempt.
    'label.aboutTagline': { en: { t: 'Microtonal sample engine for Dorico microtonal playback.' },
                            fr: { t: 'Moteur d’échantillonnage microtonal pour la lecture microtonale de Dorico.', reviewed: true } },
    'label.aboutBlurb':   { en: { t: 'Per-key, per-velocity-layer sample mapping with offline loop auto-detection, manual loop editing, and the Ouaricon tuning-system family. Built on JUCE 8.' },
                            fr: { t: 'Mappage par touche et par couche de vélocité, détection automatique des boucles, édition manuelle, et les systèmes d’accord Ouaricon. Bâti sur JUCE 8.', reviewed: true } },   // 2 lines, measured; the fuller draft was 3 and grew the About card 20.14px
    'label.madeBy':       { en: { t: 'Made by' }, fr: { t: 'Réalisé par', reviewed: true } },

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
    'label.knobAttack':   { en: { t: 'Attack' },   fr: { t: 'Attaque',  reviewed: true } },
    'label.knobDecay':    { en: { t: 'Decay' },    fr: { t: 'Déclin',    reviewed: true } },
    'label.knobSustain':  { en: { t: 'Sustain' },  fr: { t: 'Maintien', reviewed: true } },
    'label.knobRelease':  { en: { t: 'Release' },  fr: { t: 'Relâch.',  reviewed: true } },
    'label.knobPoly':     { en: { t: 'Poly' },     fr: { t: 'Polyph.',  reviewed: true } },
    'label.knobVelXf':    { en: { t: 'Vel-XF' },   fr: { t: 'Vél-XF',   reviewed: true } },
    'label.knobExpr':     { en: { t: 'Expr' },     fr: { t: 'Expr.',    reviewed: true, sameAsEn: true } },
    'label.knobDynRng':   { en: { t: 'Dyn Rng' },  fr: { t: 'Pl. dyn.', reviewed: true } },
    'label.knobOutGain':  { en: { t: 'Out Gain' }, fr: { t: 'Sortie',   reviewed: true } },
    'label.dynamics':     { en: { t: 'Dynamics' }, fr: { t: 'Dynamique', reviewed: true } },

    // ── Tuning panel (js/tuning-panel.js) ─────────────────────────────────
    // Plugin-owned copy, in scope, keyed by hand. The twelve note names, the
    // interval-quality abbreviations and the generated scale names are NOT
    // keyed — see the note above I18N_EXEMPT.
    'label.vizCircle':      { en: { t: 'Circle' },     fr: { t: 'Cercle',   reviewed: true } },
    'label.vizPolar':       { en: { t: 'Polar' },      fr: { t: 'Polaire',  reviewed: true } },
    'label.vizMatrix':      { en: { t: 'Matrix' },     fr: { t: 'Matrice',  reviewed: true } },
    'label.vizTrueKeys':    { en: { t: 'True Keys' },  fr: { t: 'Touches',  reviewed: true } },
    'label.vizRotation':    { en: { t: 'Rotation' },   fr: { t: 'Rotation', reviewed: true, sameAsEn: true } },
    'label.scaleIntervals': { en: { t: 'Scale Intervals' }, fr: { t: 'Intervalles de la gamme', reviewed: true } },
    'label.tkHint':         { en: { t: 'Hold 2+ notes to see intervals' }, fr: { t: 'Tenir 2 notes ou plus pour voir les intervalles', reviewed: true } },
    'label.totalSpan':      { en: { t: 'Total span' }, fr: { t: 'Étendue',        reviewed: true } },
    'label.rotationMode':   { en: { t: 'Mode' },       fr: { t: 'Mode', reviewed: true, sameAsEn: true } },
    'label.intervalsCount': { en: { t: 'Intervals · notes: {n}' },
                              fr: { t: 'Interv. · notes : {n}', reviewed: true } },   // 114.45px in a
                              // 142px column, measured. The fuller 'Intervalles · notes : {n}'
                              // is 2 lines and pushes the whole interval list down 14px; the
                              // one-line alternative 'Intervalles · {n} notes' fits at 138.44
                              // but inflects wrongly at n=1, so per contract §6 the count stays
                              // after the colon beside an invariant noun instead.
    'label.tonic':          { en: { t: 'Tonic' },      fr: { t: 'Tonique',  reviewed: true } },
    'label.tuningLibrary':  { en: { t: 'Tuning Library' }, fr: { t: 'Bibliothèque de gammes', reviewed: true } },
    'label.catAll':         { en: { t: 'All Categories' },  fr: { t: 'Toutes catégories', reviewed: true } },
    'label.catHistorical':  { en: { t: 'Historical' },      fr: { t: 'Historiques',       reviewed: true } },
    'label.catJust':        { en: { t: 'Just Intonation' }, fr: { t: 'Intonation juste',  reviewed: true } },
    'label.catEdo':         { en: { t: 'Equal Divisions' }, fr: { t: 'Divisions égales',  reviewed: true } },
    'label.catNonOctave':   { en: { t: 'Non-Octave' },      fr: { t: 'Non octaviantes',   reviewed: true } },
    'label.catWorld':       { en: { t: 'World' },           fr: { t: 'Du monde',          reviewed: true } },
    'label.noteCount':      { en: { t: 'notes: {n}' },      fr: { t: 'notes : {n}',       reviewed: true } },
    // A4 stays A4: it is letter pitch notation, which the C++ TuningEngine and
    // the .scl/.kbm formats also speak. Only REF is a word.
    'label.a4Ref':          { en: { t: 'A4 REF' },  fr: { t: 'RÉF. A4',    reviewed: true } },
    'label.stretch':        { en: { t: 'Stretch' }, fr: { t: 'Étirement', reviewed: true } },
    'label.loadScl':        { en: { t: 'Load .SCL' },   fr: { t: 'Ouvrir .SCL',   reviewed: true } },
    'label.loadKbm':        { en: { t: 'Load .KBM' },   fr: { t: 'Ouvrir .KBM',   reviewed: true } },
    'label.saveScl':        { en: { t: 'Save .SCL' },   fr: { t: 'Enreg. .SCL',   reviewed: true } },
    'label.saveKbm':        { en: { t: 'Save .KBM' },   fr: { t: 'Enreg. .KBM',   reviewed: true } },
    'label.exportHtml':     { en: { t: 'Export HTML' }, fr: { t: 'Exporter HTML', reviewed: true } },
    'label.generateScale':  { en: { t: 'Generate Scale' },      fr: { t: 'Générer une gamme', reviewed: true } },
    'label.genEdo':         { en: { t: 'EDO (Equal Division)' }, fr: { t: 'EDO (division égale)', reviewed: true } },
    'label.genHarmonic':    { en: { t: 'Harmonic Series' },      fr: { t: 'Série harmonique',     reviewed: true } },
    'label.genRank2':       { en: { t: 'Rank-2 Temperament' },   fr: { t: 'Tempérament de rang 2', reviewed: true } },
    // Two Period (c) labels: one in the EDO row, one in the Rank-2 row. Each
    // <label> is its own element and so needs its own key.
    'label.genDivisions':     { en: { t: 'Divisions' },      fr: { t: 'Divisions', reviewed: true, sameAsEn: true } },
    'label.genPeriod':        { en: { t: 'Period (c)' },     fr: { t: 'Période (c)', reviewed: true } },
    'label.genStartHarmonic': { en: { t: 'Start Harmonic' }, fr: { t: 'Harmonique de départ', reviewed: true } },
    'label.genEndHarmonic':   { en: { t: 'End Harmonic' },   fr: { t: 'Harm. de fin',         reviewed: true } },
    'label.genGenerator':     { en: { t: 'Generator (c)' },  fr: { t: 'Génér. (c)',           reviewed: true } },
    'label.genR2Period':      { en: { t: 'Period (c)' },     fr: { t: 'Période (c)',          reviewed: true } },
    'label.genNotes':         { en: { t: 'Notes' },          fr: { t: 'Notes', reviewed: true, sameAsEn: true } },
    'label.generate':         { en: { t: 'Generate' },       fr: { t: 'Générer', reviewed: true } },
    'label.tuningPanelUnavailable': { en: { t: 'Tuning panel unavailable.' },
                                      fr: { t: 'Panneau d’accord indisponible.', reviewed: true } },

    // ── ACCESSIBLE NAMES declared in markup or by a literal dataset write ──
    //
    // Nine were already aria-labels at v1.23.10. The rest are the FIVE markup
    // native title= attributes and THREE of the JS-written ones, moved here
    // per contract §4 with their own v1.23.10 wording, VERBATIM. No new prose
    // is invented: Stage M authors hover-help, this rule does not.
    'aria.savePreset':       { en: { t: 'Save plugin state to .omspreset' },   fr: { t: 'Enregistrer l’état du plugin dans un .omspreset', reviewed: true } },
    'aria.loadPreset':       { en: { t: 'Load plugin state from .omspreset' }, fr: { t: 'Charger l’état du plugin depuis un .omspreset',  reviewed: true } },
    'aria.settings':         { en: { t: 'Settings' },           fr: { t: 'Réglages',            reviewed: true } },
    'aria.langSelect':       { en: { t: 'Interface language' }, fr: { t: 'Langue de l’interface', reviewed: true } },
    'aria.helpToggle': { en: { t: 'Toggle hover help' }, fr: { t: 'Activer ou désactiver les infobulles', reviewed: true } },
    'aria.techniquePreset':  { en: { t: 'Rename all technique slots to match a Dorico instrument family (slot order matches the keyswitch order in the O-MicrotonalSampler expression maps)' },
                               fr: { t: 'Renommer tous les emplacements de technique selon une famille d’instruments Dorico (l’ordre des emplacements suit celui des commutations par touche dans les cartes d’expression O-MicrotonalSampler)', reviewed: true } },
    'aria.playingTechniques': { en: { t: 'Playing techniques' },     fr: { t: 'Techniques de jeu',          reviewed: true } },
    'aria.addTechnique':      { en: { t: 'Add technique slot' },     fr: { t: 'Ajouter un emplacement de technique', reviewed: true } },
    'aria.removeTechnique':   { en: { t: 'Remove last technique slot' }, fr: { t: 'Retirer le dernier emplacement de technique', reviewed: true } },
    'aria.trimTech':          { en: { t: 'Technique master trim (dB)' }, fr: { t: 'Ajustement général de la technique (dB)', reviewed: true } },
    'aria.trimLayer0':        { en: { t: 'Layer p trim (dB)' },  fr: { t: 'Ajustement de la couche p (dB)',  reviewed: true } },
    'aria.trimLayer1':        { en: { t: 'Layer mp trim (dB)' }, fr: { t: 'Ajustement de la couche mp (dB)', reviewed: true } },
    'aria.trimLayer2':        { en: { t: 'Layer mf trim (dB)' }, fr: { t: 'Ajustement de la couche mf (dB)', reviewed: true } },
    'aria.trimLayer3':        { en: { t: 'Layer f trim (dB)' },  fr: { t: 'Ajustement de la couche f (dB)',  reviewed: true } },
    'aria.closeLoopEditor':   { en: { t: 'Close loop editor' },  fr: { t: 'Fermer l’éditeur de boucle', reviewed: true } },
    'aria.floLayer':          { en: { t: 'Target velocity layer' }, fr: { t: 'Couche de vélocité cible',   reviewed: true } },
    'aria.floTechnique':      { en: { t: 'Target technique slot' }, fr: { t: 'Emplacement de technique cible', reviewed: true } },
    'aria.renameInput':       { en: { t: 'New technique name' },    fr: { t: 'Nouveau nom de technique',   reviewed: true } },
    'aria.dynamicsMode':      { en: { t: 'Dynamics Mode — how MIDI CC 11 shapes dynamics. Velocity: note-on velocity picks the layer, CC 11 is a post-mix volume trim (v1.20 behaviour). CC Crossfade: CC 11 morphs across all velocity layers mid-note (timbre + loudness, like pro sustain patches).' },
                                fr: { t: 'Mode de dynamique — comment le CC MIDI 11 façonne la dynamique. Velocity : la vélocité de la note choisit la couche, le CC 11 sert d’ajustement de volume après mixage (comportement v1.20). CC Crossfade : le CC 11 fond toutes les couches de vélocité au cours de la note (timbre + niveau, comme les patches professionnels de notes tenues).', reviewed: true,
                                  termNote: '« après mixage » is the mixing PROCESS the layers are summed by — the post-mix trim of the English — not the Mix control the forbidden entry targets. There is no Mix control on this page' } },
    'aria.dynamicsModeShort': { en: { t: 'Dynamics Mode' }, fr: { t: 'Mode de dynamique', reviewed: true } },
    'aria.knobExpr':          { en: { t: 'Expression (MIDI CC 11) — dynamics control, independent of velocity layer' },
                                fr: { t: 'Expression (CC MIDI 11) — contrôle de la dynamique, indépendant de la couche de vélocité', reviewed: true } },
    'aria.knobDynRng':        { en: { t: 'Dynamic Range (CC Crossfade only) — dB span between pp and ff. 0 dB = flat; higher = louder ff / quieter pp. Fixes "forte too soft, piano too loud" in Dorico.' },
                                fr: { t: 'Plage dynamique (CC Crossfade uniquement) — différence en dB entre pp et ff. 0 dB = plat ; plus haut = ff plus fort / pp plus doux. Corrige le « forte trop faible, piano trop fort » dans Dorico.', reviewed: true } },
    'aria.loopResetOneShot':  { en: { t: 'Sample is one-shot — no loop region detected.' },
                                fr: { t: 'Échantillon one-shot — aucune région de boucle détectée.', reviewed: true } },
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
    ['#tips-toggle',                             'tip.tipsToggle'],
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
