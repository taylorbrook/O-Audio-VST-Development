# Stage N — Checkpoint 5 as a QA pass: every French entry, against a glossary and a lint

You are one executor of several running concurrently, each on ONE plugin. This brief is
the authority for Stage N. Where it is silent, the Stage M brief's carried traps apply
(`260826-ieq-STAGE-M-BRIEF.md`, "The M1 traps that cost the most", items 1–13 of "BATCH
M2 COMPLETE", and "Shared resources"). Where the two disagree, THIS file wins.

## What this stage is, and is not

All 43 plugins are localized: labels and hover-help, English and French, one renderer, one
canon. **3751 French entries are machine drafts and not one has been read since it was
written.** Checkpoint 5 in the plan is "a native speaker reads them". No native speaker is
scheduled, so the developer chose a second reading — by you, against two instruments that
did not exist when the drafts were written:

- **`scripts/i18n-fr-glossary.js`** — the settled French for ~230 recurring terms, with
  its prose companion `260826-ieq-FR-GLOSSARY.md` beside this file. **Read the companion
  first, all of it.** It is short and it is the whole style guide.
- **`scripts/i18n-fr-lint.js`** — ten mechanical checks. On 2026-08-31 it reported **2145
  findings across 43 of 43 plugins**: 427 number–unit gaps, 347 glossary misses, 325
  punctuation marks (`; ! ?`) and 311 colons without their no-break space, 311 straight
  apostrophes, 203 forbidden words, 177 bare `%`, 26 casing splits, 10 hyphens where a
  minus belongs, 8 decimal points. Yours are in `node scripts/i18n-fr-lint.js --plugin
  <Name> --verbose`.

**This is a REVIEW, not a re-translation.** Fix what is wrong: terminology, typography,
agreement, register, idiom, and any sentence whose meaning drifted from the English. Leave
what is right. A French body that reads well and says what the English says is finished,
and rewriting it to taste is the one way to make this stage take longer than Stage M.

**`reviewed: false` stays `false`.** That flag means *a native speaker read this*. You are
not one; the header comment records your pass, the flag records theirs. The companion
explains the reasoning. Do not add a new field to record your pass — `termNote` is the
only new key this stage introduces, and it is for glossary exemptions only.

## What landed at repo level before you were dispatched (N0)

1. **`<html lang>` follows the selector.** The canon's `applyI18n()` now sets
   `document.documentElement.lang = uiLanguage` (`scripts/i18n-canon.js`), synced into all
   43 canon-bearing files in one commit with no version bumps — the same shape as
   `cec3f857`. Verified: `check-i18n` 43/43 canon v2, `boot-all-uis` 43/43 clean,
   0 DEAD; a runtime probe drove `__setLanguage('fr')` on every page and read `lang="fr"`
   back on 43/43, `'en'` after switching back, and `'fr'` after `__reapplyI18n()`; the
   negative control (a setter that does not touch `lang`) fails on the same probe.
   **Your CHANGELOG gets one bullet for it** under Changed — the user-visible half of the
   canon change ships with your version, not with the sync.
2. The glossary and the lint, above. **`scripts/` is the orchestrator's** — if the lint is
   wrong (a false positive you can prove, a term the glossary should carry), STOP on that
   item, report it with the shape of the wrong assumption, and continue with the rest. Do
   not edit `scripts/`. Do not work around the lint with a `termNote` that is not a real
   contextual exemption — that is the reasoned-exemption mechanism being used to hide a
   miss, and the M brief already names that trap for `I18N_EXEMPT`.

## The eleven steps, per plugin

1. **Baseline, before touching anything.** Run and keep the output in
   `scratchpad/<Name>/`:
   ```bash
   node scripts/i18n-fr-lint.js --plugin <Name> --verbose
   node scripts/check-i18n.js --plugin <Name>
   node scripts/check-ui-labels.js --plugin <Name>      # geometry BEFORE
   ls plugins/<Name>/tests/                              # every gate you will re-run
   grep -rn "setVisible" plugins/<Name>/Source/          # abort+report if it targets the web view
   file plugins/<Name>/<uiroot>/js/i18n.js               # CRLF? — pattern_bulk_edit_crlf
   ```
2. **Read your `i18n.js` header in full**, Stage K and Stage M blocks both. It records
   every caption whose width was measured to the pixel and every choice that was made for
   geometry rather than taste. A glossary root term that does not fit where the header says
   "1.17 px of margin" is why the glossary lists abbreviations.
3. **Read the companion glossary**, then run through your lint findings **in this
   order**, because each pass changes what the next one sees:
   1. **G1 / F1 / C1** — terminology and casing. Root term where it fits, the listed
      abbreviation where the header pinned the width. **Never invent a third form.** If
      neither root nor abbreviation fits, keep what is there, and REPORT the term with the
      measured width so the glossary can grow.
   2. **T1–T7** — typography. Apostrophe U+2019, comma decimal, U+00A0 before `% : ; ! ?`
      and between number and unit, U+2212 for a negative number. These are safe bulk edits
      **inside French strings only**; do them with a script if you like, but diff the
      result and read the diff — a regex that touches an English string, a key, or a
      selector is a regression the lint cannot see.
   3. **Read every French entry against its English**, LABELS then I18N, title then
      body. You are looking for: wrong gender/number agreement, a verb form where a noun
      belongs, *tu* where *vous* belongs, a literal calque that no French speaker would
      write, a sentence that says something the English does not (a range, a default, a
      unit, a claim about the DSP). For any DSP claim you are unsure of, the English was
      written from the processor source in Stage M — read the same source before you
      change the French to disagree with it.
   4. **`sameAsEn: true` entries** — the lint lists them as INFO. Each one is either a word
      that is French too (*Mode*, *Position*, *Gain*, *Chorus*, *Tension*, *Expression*)
      and stays, or an English word that slipped through (*Threshold*, *Detune*,
      *Straight*, *Humanized*) and gets translated — with `sameAsEn` removed. Record the
      count of each in the header.
4. **Nothing outside `fr` changes.** Not `en`, not a key, not a `TIP_BINDINGS` row, not
   `I18N_EXEMPT`, not a selector. If reviewing the French exposes an English defect (Stage
   M found five that way), REPORT it; do not fix it in a Stage N commit.
5. **Geometry.** `node scripts/check-ui-labels.js --plugin <Name>` again: the number of
   non-label elements moved between English and French must be **unchanged from your
   baseline** (it was 0 on every plugin at the end of Stage L and must still be), no French
   caption clips, and the vacuity guard still confirms French rendered. *Relâchement* is
   four glyphs longer than *Relâche*; a U+00A0 inside a caption is an unbreakable run
   where a line break used to be possible. If a settled term moves geometry, the
   abbreviation is the answer, not a CSS edit — Stage N touches no CSS.
6. **Hover-help renders, both languages.** `node plugins/<Name>/tests/ui_tip_render_check.js`
   (or the plugin's `ui_tooltip_clamp_check.js` on O-Tapestop, O-Bitrot, O-ReverseDelay) —
   the French arm must pass with your longer, less-breakable bodies. The right-edge clamp
   is where a body that gained six no-break spaces goes off-frame.
7. **`node scripts/i18n-fr-lint.js --plugin <Name> --strict` exits 0.** Every `termNote`
   you added is listed in its EXEMPT lines with a reason you would defend to the developer.
8. **Every other gate in `plugins/<Name>/tests/`**, then `node scripts/check-i18n.js
   --plugin <Name>`, then `node scripts/boot-all-uis.js` for the whole suite — 43/43 clean,
   0 DEAD, and your plugin's late count unchanged.
9. **Header comment.** Add a block at the top of the existing header:
   ```
   // ── vX.Y.Z: FRENCH QA PASS (Stage N, 2026-08-31) ──────────────────────────
   // Every fr entry read against its en and against scripts/i18n-fr-glossary.js.
   // Changed: NN entries (TT terminology, YY typography, GG grammar/agreement,
   // SS meaning). sameAsEn: kept K, translated T. termNote exemptions: E (listed).
   // Left as drafted: the rest. reviewed: false throughout — no native speaker yet.
   ```
   plus one line per **decision** you made that the next reader needs (the abbreviation
   you kept for width, the loanword you left). Not a diary.
10. **Version, CHANGELOG, build.** Patch bump (`x.y.Z+1`). `grep -rn "<old version>"
    plugins/<Name> --exclude-dir=.planning` and update every site that is not CHANGELOG
    history — CMakeLists `VERSION`, the `i18n.js` header, the console banner, the harness
    fixture (drift there was LIVE on O-simplePhysicalModelSynth in Stage I). CHANGELOG:
    ```
    ## [x.y.Z+1] - 2026-08-31

    French copy revised. Stage N of the repo-wide i18n rollout.

    ### Changed
    - **NN French entries revised** against the suite glossary and lint: … (the categories
      and the two or three most visible changes, e.g. Sauver → Enregistrer, Relâche →
      Relâchement, straight → typographic apostrophes, no-break spaces before units and
      punctuation).
    - **`<html lang>` now follows the language selector** (canon change, all plugins), so
      assistive technology reads the page in the language it is displayed in.
    ```
    Then `./scripts/build-and-install.sh <Name>` **under the mutex**
    (`/tmp/claude-501/stagek-build.lock` — same lock as Stages K–M; take it with `mkdir`,
    release it the moment the build and `auval` are done). Confirm the installed binary
    carries one of your changed French strings with **`LC_ALL=C grep -a`, not `strings`**
    — `strings` splits on the multi-byte `é` and finds nothing.
11. **Commit.** `git add` the exact paths you changed; then, IMMEDIATELY before committing,
    `git branch --show-current` (must be `main`) and `git status --short`; then
    `git commit -- plugins/<Name>`; then `git show --stat`. Never `-a`, never `-A`, never
    `--amend`, never `git checkout -- <file>` to undo a plant. **Do not touch
    `PLUGINS.md`** — report the row. Do not tag, push, or `screencapture`.

Commit message shape:

```
improve: <Name> vX.Y.Z - French copy revised against the suite glossary (Stage N batch N1)
```

followed by counts by category, the glossary decisions that changed a visible caption, any
English defect found and NOT fixed, and the gate results.

## Traps that apply here specifically

- **A regex over the whole file will touch English.** `'` appears in English bodies and in
  JS syntax; `%` and units appear in English; `-\d` appears in selectors and CSS. Scope
  every bulk edit to the `fr:` object's string values. Read the diff.
- **A U+00A0 is invisible in a diff and in most editors.** After a typography pass, `grep
  -c $'\xc2\xa0'` on the file and compare against your lint's T3+T4+T5+T7 count going to 0.
  And check that no U+00A0 landed inside a KEY or a selector: `grep -n $'\xc2\xa0' <file> |
  grep -v "t:\|b:"` should be empty.
- **U+2212 in a body is prose; U+2212 in a readout is a bug.** You are not touching
  readouts. If a `LABELS` entry is a numeric caption that a formatter also writes, it is a
  readout in disguise — leave its hyphen and report it.
- **The glossary key is the English label, lower-cased.** *Rate* on a page where the
  control IS a frequency in Hz is still keyed `rate` and the lint will ask for *Vitesse*.
  That is a real contextual exemption: `termNote: 'the control is a frequency in Hz'` and
  *Fréquence*. Two of those per plugin is normal; twenty is a sign you are exempting the
  glossary.
- **Casing on a page that uppercases with CSS.** If the caption element carries
  `text-transform: uppercase`, the French casing in the table is invisible on screen, but
  the lint (C1) and the `aria-label` are not. Follow the English casing in the table anyway.
- **The tuning panel is shared code.** O-Bells, O-Formant, O-IntonationPad, O-Lyrica,
  O-MicrotonalSampler, O-Prism, O-Reed, O-Wind carry copies of the scala-tuning-engine
  panel strings. The glossary settles those strings (*Bibliothèque de gammes*,
  *Intervalles de la gamme*, *Historiques*, *Du monde*, …). Use the settled form; the copies
  converging is the point.
- **`HEAD~1` is not your parent** in a shared checkout; use `<sha>^`.
- **The scratchpad is shared** — everything into `scratchpad/<Name>/`.
- **Restore a plant from a namespaced copy**, never with `git checkout --`.

## What to report back

Compact, not a transcript:

- version shipped, commit sha, `git show --stat` file count
- lint: baseline total → 0 (`--strict` exit 0), and the per-code counts you closed
- entries changed by category (terminology / typography / grammar / meaning), out of the
  plugin's total
- `sameAsEn`: kept N / translated M, with the translated words
- `termNote` exemptions: each with its reason
- glossary terms you could NOT apply for width, with the measured px and what you kept
- geometry: moved before / after; render gate: states driven; `boot-all-uis` verdict
- English defects found and not fixed
- anything in the glossary or lint you believe is wrong, with evidence
- what is NOT verified

## Batches

Ordered by lint findings and by family so each batch teaches the next. Three to six
executors run concurrently; builds serialize on the mutex.

| Batch | Plugins | Why together |
|---|---|---|
| **N1 (pilot)** | O-Comp (39), O-Chorus (43), O-simpleFM (63) | Three families: an inline-module effect with straight apostrophes and glossary hits; the all-caps casing case; the O-simple* family with 26 T1 and Stage K width pins. Small enough to read the whole diff. |
| N2 | O-AnalogSaturation (7), O-Bass (13), O-Texture (13), O-Polystutter (15), O-Tremolo (16), O-AnalogEQ (20) | the smallest remaining |
| N3 | O-Tapestop (20), O-FreqPulse (25), O-Gain (25), O-SpectralShaper (25), O-Bassoon (26), O-TextureForge (27) | |
| N4 | O-Bowed (28), O-Emulator (29), O-ReverseDelay (29), O-Octagon (30), O-MultiBandCompressor (31), O-Orbit (32) | |
| N5 | O-Marimba (33), O-Freeze (41), O-SimpleReverb (42), O-DigiDelay (43), O-Bitrot (45), O-Lyrica (49) | |
| N6 | O-simpleBeatmaker (44), O-simplePhysicalModelSynth (50), O-simpleSubtractive (56), O-simpleAdditive (62), O-simpleSampler (71), O-simpleGrain (92) | the O-simple* family, together, for consistency |
| N7 | O-Reed (54), O-Contrabass (67), O-GrainScatter (68), O-Detune (70), O-MicrotonalSampler (77) | |
| N8 | O-IntonationPad (76), O-Wind (76), O-Formant (77) | the tuning-panel family |
| N9 | O-Bells (134), O-Prism (262) | the two largest, last, with everything the others learned |

(Parenthesised numbers are lint findings at baseline.)

## DECISION ITEMS — for the developer, not for you

Record; do not act.

27. **`reviewed: false` after Stage N.** The flag will still read `false` on all 3751
    entries and `check-i18n` will still print a 3751-entry "native-speaker worklist". If
    no native review is ever planned, the flag is dead weight and the worklist is noise;
    if one is, the flag is exactly right. Either way it is the developer's call.
28. **Pages that uppercase with CSS.** Several pages render captions through
    `text-transform: uppercase`, so the table's casing is invisible on screen but visible
    to assistive technology and to the lint. Stage N follows the English casing in the
    table; whether those pages should drop the CSS transform and carry real caps is a
    separate question.
29. **The lint is a report.** `--strict` exists and nothing calls it. When Stage N ends
    at 0 findings on 43/43, wiring `--strict` into whatever runs `check-i18n` is a one-line
    change that keeps the French from drifting back.

---

# CORRECTIONS AND CARRIED TRAPS FROM THE N1 PILOTS

Appended as they land. Where this section disagrees with anything above, THIS section
wins.
