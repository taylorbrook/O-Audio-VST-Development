# French glossary and style — task 260826-ieq, Stage N

**The machine-readable list is `scripts/i18n-fr-glossary.js`. This file explains it.**
`scripts/i18n-fr-lint.js` enforces the list; a term that is not in the list is the
reviewer's call, and a term that IS in the list is not.

## Why a glossary, measured

Stage N began by importing every plugin's `i18n.js` (5078 rows, 43 plugins) and grouping
French renderings by their English source. **267 English label strings had more than one
French rendering.** The worst:

| English | French renderings found |
|---|---|
| Off | Arrêt (12) · ARRÊT (3) · Non (6) · NON · Aucun · désactivé · Désact. · DÉS. · Désactivée (2) |
| Save | Enreg. (15) · Enregistrer (7) · SAUVER (3) · Sauver (3) · Enr (3) · Enr. (3) · ENREG. |
| Release | Relâche (10) · Relâchement (3) · Extinction (3) · Chute · Extinct. · Relâch. · Relâchem. |
| Feedback | Réinjection (4) · Réinject. (3) · Réinj. (2) · RÉINJ. · Retour · Rétroaction · Réaction |
| Load | Ouvrir (12) · CHARGER (3) · Lire (3) · Charger (2) · CHARG. · Ouv. |
| Mix | Dosage (8) · Mixage (6) · Mix (5) · DOSAGE (2) · Mélange |
| Depth | Profondeur (8) · Prof. (4) · Ampleur (3) · PROF. (2) · Profond. |
| Rate | Vitesse (13) · Fréq. (2) · VITESSE (2) · Vit. |

Every one of those was a defensible choice made by one author on one page. The suite is
one product; a user who learns "Réinjection" on O-Wind should not meet "Rétroaction" on
O-Formant. Forty-three reviewers without a list would widen this, so the list came first.

The typography split was as wide: **1559 typographic apostrophes against 550 straight
ones** (eleven plugins straight, thirty-two typographic); **6 no-break spaces before a colon
against 332 plain ones**; **15 before `%` against 233 plain**.

## The settled terms — and why

Root term first; abbreviations are for captions whose width the plugin's Stage K header
pinned. Full list in the JS.

| English | French | Why this one |
|---|---|---|
| Save / Load | **Enregistrer** (Enreg.) / **Charger** (Charg.) — *Ouvrir* accepted where the button opens a file dialog and the K header pinned it | Standard French menu vocabulary. *Sauver* is a calque; *Lire* means read or play. |
| On / Off | **Marche / Arrêt** for a power state; **Activé(e) / Désactivé(e)** for a feature; **Aucun(e)** where Off means none selected | Three pairs by meaning, not nine by author. *Oui/Non* are answers, not states. |
| Attack / Decay / Sustain / Release | **Attaque / Déclin / Maintien / Relâchement** (Att. / Décl. / Maint. / Relâch.) | The textbook ADSR. *Tenue* and *Chute* are metaphors; *Relâche* is a theatre closure; *Extinction* is a reverb tail. |
| Feedback | **Réinjection** (Réinj.) | The delay term. *Rétroaction* is control theory; *Retour* is a monitor send. |
| Mix (dry/wet) | **Mix** | What every French DAW shows. *Mixage* is the mixing process; *Dosage* is elegant and nobody else uses it. Same width as English — zero geometry risk. |
| Wet / Dry | **Traité / Direct** | *Effet/Sec* also correct; one pair. |
| Rate / Depth | **Vitesse / Profondeur** (Vit. / Prof.) | *Fréq.* only when the control is literally a frequency in Hz. *Ampleur* is fine French and the minority. |
| Detune / Spread | **Désaccord** (Désacc.) / **Étalement** (Étal.) | *Écart* was doing both jobs. *Écart total* stays for a tuning span. |
| Reverb | **Réverbération**, abbreviated **Réverb** | *Réverbe* is not a word. |
| Knee | **Coude** | *Genou* is the body part. |
| Seed | **Graine** | *Germe* is a sprout. |
| Glide | **Portamento** | *Glissé* is a calque; portamento is the musical term. |
| Bypass | **Contournement** (Contour.) | Cubase FR. *Dériv.* is not recognisable. |
| Threshold / Ratio | **Seuil / Ratio** | *Ratio* is used as-is in French audio. |
| Tuning / Scale | **Accord / Gamme** | Two words for two things; *Gamme* for tuning conflates them. |
| Low / Mid / High | **Grave / Médium / Aigu** | Band names, not directions (*Bas/Haut*). |
| Sync | **Synchro** (Sync) | |
| Master | **Général** | The mix-bus term. *Maître* is a translation, not a term. |
| Delete / Reset | **Supprimer** (Suppr.) / **Réinitialiser** (Réinit.) | |
| Flutter / Wow | **Scintillement / Pleurage** | Tape terms. *Flutter tongue* (the wind technique) is **Flatterzunge**, which French scores use. |
| Tilt | **Inclinaison** | *Pente* is a slope (rolloff). |
| Jitter / Dither / Ducking / Shimmer / Swing / LFO / EQ / MIDI | **Gigue** / **Dither** / **Ducking** / **Shimmer** / **Swing** / as-is | Loanwords the French audio press uses; translating them would confuse. |

**Contextual exceptions carry a `termNote`.** O-Octagon's "Delay" is a loudspeaker
alignment delay and **Retard** is correct there; O-Formant's "Voicing" is phonetic
**Voisement** while O-IntonationPad's is a chord **Disposition**. The lint reports a
`termNote` as EXEMPT and does not count it. A `termNote` must say *why*, in the spirit of
`I18N_EXEMPT` — a reason, never silence.

## Typography — one rule each

| | Rule | Lint |
|---|---|---|
| Apostrophe | U+2019 **’** everywhere: *l’entrée*, *d’archet* | T1 |
| Decimal | comma in prose: *0,1 à 100 ms*. The **readout keeps its point** (D-03 exempts the readout node) | T2 |
| Before `%` | U+00A0: *50 %* | T3 |
| Before `:` | U+00A0: *Plage : −6 à +6 dB* | T4 |
| Before `; ! ?` | U+00A0: *Confirmer ?* | T5 |
| Minus | U+2212 **−** before a negative number: *−40 à +40 dB*. Hyphen stays in *1-8* ranges and compound words | T6 |
| Number–unit | U+00A0 between: *5 dB*, *250 ms*, *440 Hz* | T7 |
| Ranges | *de 0,1 à 100 ms* in a sentence; *0–100 %* (en dash) in a caption | — |

**U+00A0, not U+202F.** The Imprimerie nationale wants a thin no-break space before
`; ! ?`. Plugin pages ship their own web fonts and U+202F is missing from some of them — it
would render as a box where no gate looks. One character, everywhere.

## Casing

A French caption **follows the casing of the English caption it replaces**. An all-caps
page (O-Marimba, O-Polystutter, O-Octagon's speaker labels) stays all-caps **with accents
on capitals** — RÉF., DÉCLIN, ÉTALEMENT. A mixed-case English caption gets French
**sentence case**: *Forme d’onde*, *Taille de grain*, not *Forme d’Onde*. Acronyms keep
their caps (LFO, EQ, MIDI, PB). Lint C1 checks all-caps parity; sentence case is the
reviewer's.

## Register

- **Tooltip bodies address the user as *vous***. Instructions may be imperative (*Cliquez
  deux fois pour réinitialiser*) or infinitive (*Double-cliquer pour réinitialiser*) — pick
  one per plugin and keep it.
- **Captions and tip titles are nouns or infinitives**, never conjugated: *Enregistrer le
  préréglage*, not *Enregistrez*.
- Sentences end with a period in a body; a title carries none.
- No anglicism where a French term is established (the table above), and no forced
  translation where the loanword is the term (the last row).
- **The product is *ce plugin* (masculine)** — not *plugiciel* (OQLF, three plugins had it),
  not *extension*. Settled 2026-08-31 from a 12-to-3 split; *plugiciel* is forbidden in prose.

## What stays English

- **Factory preset names** (CONTEXT, scope expansion §"Factory preset names").
- **Readout values and unit symbols** — `1.5 kHz`, `−12 dB`, `4.0:1` in both languages.
- **Tuning identifiers** (*12-TET Standard*, Scala file names), note names (*A4*), file
  extensions.
- An `I18N_EXEMPT` entry with its reason. Stage N does not re-open exemptions.

## What Stage N does NOT do

- It does not flip `reviewed: false`. The flag means *read by a native speaker*, and this
  pass is a second machine reading against a glossary and a lint. The header of each
  reviewed file records the pass; the flag records the human.
- It does not change English copy, keys, bindings, or anything outside `i18n.js` except
  the version sites and the CHANGELOG.
