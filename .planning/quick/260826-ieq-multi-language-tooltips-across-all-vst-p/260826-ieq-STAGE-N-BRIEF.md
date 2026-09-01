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

## From the N1 pilots (O-Comp v1.7.1 `55a81c59`, O-Chorus v1.4.1 `cbbda46b`, O-simpleFM v1.3.1 `6bb4cf32`)

All three: lint → 0 `--strict`, 0 non-label elements moved, render gates unchanged,
`boot-all-uis` 43/43 / 0 DEAD, `auval` PASS, installed bundles verified with
`LC_ALL=C grep -a`. Three lint defects were found by the pilots and fixed at repo level
(`daed4a2e`, `8a387f1c`, `6eb042c8`) — **re-run your baseline after reading this; the
numbers in the batch table are pre-fix.**

1. **A `termNote` now exempts both G1 and F1.** The first draft printed an exempted entry
   as EXEMPT *and* counted it as F1 in the same run. If your baseline still shows that
   shape, you are on a stale `scripts/`; `git log -1 -- scripts/i18n-fr-lint.js` should
   be `6eb042c8` or later.
2. **A glossary-accepted rendering is never a forbidden word** (*Écart total*, *Fréq. méd.*
   no longer draw F1). **T7 catches a missing number–unit space** (`440Hz`) and `%` reports
   under T3 only.
3. **The glossary gained `porteuse nulle`** for *carrier null* (the settled term is 160.7 px
   into a 102 px badge). When a root term does not fit and no abbreviation is listed,
   report the measured width — that is how the list grows; do not invent one.
4. **Measure before you believe a header's geometry defence.** O-Comp's v1.7.0 header
   defended `Sauver` on width; measured, `Enreg.` is 23.75 px against `Sauver` 25.00 —
   *narrower*. O-Chorus's `Relâch.` 36.80 is narrower than `Relâche` 38.94 *and* than
   English `Release` 37.70. Two of three pilots found the abbreviation the glossary lists
   fits where the header said the root's competitor was the tight fit. Re-measure with the
   gate's own method (`Range.selectNodeContents` on the real node at the shipping frame).
5. **Root terms that do NOT fit, measured** (keep the listed abbreviation): *Étalement*
   60.47 px over a 50 px `.knob` gate cliff; *Profondeur* 68.02 and *Saturation* 63.52 past a
   62 px wrap cliff; *Enregistrer* 78.52 border-box against a 62 px `.preset-action` pin;
   *Relâchement* 62.92 against a 52 px `.control-group` (O-Comp) — but it FITS in
   O-simpleFM's 56 px envelope cells (77.3 px, nowrap, symmetric overflow, 15.5 px
   clearance). Same term, two verdicts: measure on your page.
6. **Scope the typography pass with a state machine over the `fr:` blocks, not a regex over
   the file.** All three pilots did; O-Chorus kept the dry-run diff and a scope-leak
   assertion. Audit afterwards: `grep -n $'\xc2\xa0' i18n.js | grep -v "t:\|b:"` must be
   empty; count `en:` lines changed = 0 by importing both revisions and comparing.
7. **The binary control must grep the shipped VALUE, not the word.** Your header comment
   now discusses the old words, and `juce_add_binary_data` embeds the whole file — so the
   old word is still in the binary. Grep `t: 'Enreg.'`, not `Enreg`.
8. **A stale width table in `index.html` or `styles.css` is a separate, comment-only
   `docs(<Name>):` commit**, after your release commit — the shape O-Chorus (`bba4626b`)
   and O-Comp (`c080de9c`) used. Stage N still touches no CSS rule.
9. **Casing under `text-transform: uppercase` is byte-identical on screen** (O-Chorus
   measured eight captions to the hundredth). Follow the English casing in the table
   anyway — the lint and the accessible name read the table. Item 28 has its evidence.
10. **If your plugin has no committed render gate** (Stage I/J plugins predate
    `ui_tip_render_check.js` — check `ls plugins/<Name>/tests/`), drive hover-help from a
    **scratchpad** probe as O-simpleFM did: every anchor, both languages, tip inside the
    frame, tip text CHANGES between languages (the arm that catches a French body equal to
    its English). Do not author a new committed gate in Stage N; report the gap.
11. **Two French names for one control is a real defect** — O-Comp's gear said *Réglages* in
    its tip and *Paramètres* in its `aria-label`. Read `aria.*` keys against the tip title
    of the same control.
12. **French label-in-name (WCAG 2.5.3) can hold only by stem** when a caption is an
    abbreviation (`Enreg.` ⊂ `Enregistrer le préréglage`). Record it in the header; do not
    invent a caption to close it.
13. **The C:M ratio on O-simpleFM is inverted in ENGLISH** (`FMVoice.h:210`: modulator =
    carrier × ratio, so the caption `Ratio C:M` is backwards). Decision item 30. The French
    mirrors the English deliberately. If your plugin's tooltips make a ratio/direction
    claim, check it against the source before trusting either language.
14. `HEAD~1` was not the parent for two of three pilots — other executors' commits landed
    between. `<sha>^`.

## From N2 (first three to land: O-AnalogSaturation `d793390e`, O-Texture `8fad691a`, O-Bass `86dae51d`)

15. **`sameAsEn: true` is a key you WILL add, and the brief was wrong to say otherwise.**
    When a glossary root term is the English word (*Mix*, *Ratio*, *Dither*, *Ducking*,
    *Shimmer*, *Swing*, *Sync*, *Mono*, *Min*, *Max*, *Mode*, *Type*, *Position*, *Tempo*,
    *Distance*, *Diffusion*, *Phase*, *Sub*, *Grains*, *Notes*), applying it makes the
    French a straight copy, and `check-i18n` assertion 4 hard-FAILS a LABELS entry (or an
    I18N entry whose title AND body both match) until it carries `sameAsEn: true`. Add it —
    it is the existing, correct declaration that a human looked and agreed the word is
    French too. `termNote` remains the only key that is NEW to this stage.
16. **The lint's straight-copy INFO now counts the CONDITION (`fr === en`), not the flag**,
    and lists unflagged copies per plugin (`INFO straight copy, unflagged:`). Use that list
    for step 3.4; the old counter printed 0 on a page that had one.
17. **A glossary term can be right as a caption and wrong inside a body on the same
    page.** O-AnalogSaturation kept *gain d'entrée* for "input drive" because on a plugin
    named for saturation, *la saturation d'entrée* names the effect where the sentence
    means the gain into it. O-Texture kept *filtre en bascule* for a tilt filter that is
    described, not captioned. Bodies are not matched against TERMS; do not `termNote` a
    body. Record the choice in the header.
18. **An idiom that is better and three characters longer can cost 17 px of tip
    clearance.** O-Texture measured *appuyer dessus* (27.8 → 11.0 px bottom clearance) and
    reverted to *les presser*. On the tightest tip of a page, the calque stays.
19. **A French body that DROPPED a clause the English has is a meaning defect** — O-Texture
    restored a closing range ("De 0,00 à 1,00.") the draft had lost; O-AnalogSaturation
    restored DIODE's "harder". Read for omissions, not only for errors.

## From N2 complete (6 of 6: + O-Polystutter `911a516e`, O-Tremolo `d0bc5840`, O-AnalogEQ `04ac4aaf`)

20. **Third header width defence proven backwards.** O-AnalogEQ's v1.3.0 header defended
    SAUVER on width; ENREG. measures 35.50 px against SAUVER's 39.25 — 3.75 px narrower.
    Three of nine plugins so far. O-Bass's and O-Tremolo's headers held to the hundredth.
    The rule stands: re-measure, never inherit.
21. **Label-in-name (WCAG 2.5.3) decides the PERIOD on a narrow abbreviation.** O-Bass and
    O-Tremolo both shipped `OUV` / `ENR` without the period: `Ouv` ⊂ *Ouvrir un préréglage
    depuis un fichier* and `Enr` ⊂ *Enregistrer les réglages actuels*, while `Ouv.` and
    `Enreg.` are not substrings. The glossary's trailing-period normalisation accepts both.
    On a 420–600 px frame, prefer the period-less form when it closes label-in-name.
22. **The glossary gained `enr` (save), `sync tempo` (tempo sync) and a `pan sync` row.**
    O-Tremolo found the lint flagging exactly half of a matched pair (`SYNC PAN` / `SYNC
    TEMPO`, *Synchro Tempo* 51.30 px in a 42 px `overflow: hidden` box). Two termNotes on
    that plugin are now redundant; harmless, and the next executor on it may drop them.
23. **A body's terminology can be settled by its own caption.** O-Polystutter tied *à la
    main* → *manuellement* to the MAN/Manuel caption; O-Bass moved *bulle d'aide* to the
    *aide au survol* 21 siblings use. Read the body against the captions on the same page.
24. **Pointer-unreachable anchors are the shipped default on tabbed/lane pages** — 81 of 105
    on O-Polystutter at rest. A scratchpad probe pins the rest state and drives out through
    the page's own controls (M2 trap 5), never by stripping a class.
25. **Dispatch correction:** the `TIGHT` clip on O-AnalogEQ is item 10 of the M brief's
    "DECISION ITEMS — added by M2" (line ~1212), not item 10 of the M2 corrections list.
    Two English tooltip defects were found by reading the French — O-Polystutter's MIDI body
    (says C1–B1 / any other note; the router takes notes 60–63 and only 67 triggers all)
    and O-Tremolo's `tip.panSync` (says a stereo *signal* is needed; the gate is on the
    *bus*, and a mono source on a stereo bus hears it). Decision items 34 and 35. Neither
    language was changed.

## From N3 complete (6 of 6: O-Tapestop `a51c7468`, O-FreqPulse `1d16dd0d`, O-Gain `efd92f51`, O-SpectralShaper `3e7631aa`, O-Bassoon `e76224af`, O-TextureForge `6de042ff`)

26. **A tooltip TITLE that equals its English over a translated body takes NO `sameAsEn`
    flag.** `check-i18n` reads the flag entry-scoped (title AND body); flagging such an entry
    disarms assertion 4 for it. The lint now counts those titles as covered and lists only
    the real unflagged copies (O-Tapestop, O-Gain both reasoned this out independently).
27. **The product is *ce plugin*, masculine.** Not *plugiciel* (now forbidden in prose), not
    *extension*. O-Tapestop's one occurrence was fixed by the orchestrator as a copy-only
    `fix:`; O-Formant (2) and O-MicrotonalSampler (4) will meet it as F1.
28. **`timing` accepts *cadence*** for the time-base heading sense (O-Tapestop); *décalage*
    stays for a nudge. **`crossfade`, `scatter x/y`** rows exist now; *fondu* alone is a fade.
    O-TextureForge found the glossary matched 3 of its 12 captions — **a 0 G1 on a page the
    glossary barely covers is coverage, not a verdict**; read those captions by hand.
29. **Two tooling traps from O-SpectralShaper, both caught by a control:** a `fr: {` inside a
    COMMENT opened a line-scoped state machine 17 lines early and rewrote the version string
    `v1.7.0` → `v1,7,0` under the decimal rule — skip comment lines and scope rewrites to the
    STRING VALUE of a `t:`/`b:` pair; and a U+00A0 constant typed in a script can be an
    ASCII space (or a NUL) — write `' '` and assert `ord(...) == 0xA0`.
30. **Fourth and fifth header width defences proven backwards** (O-Gain's popover note,
    O-Tapestop's *Suivi tonal* "97 px in an 88 px cell" — measured 91.97 px with 28 px clear
    on each side because `.knob-label` is shrink-to-fit with `overflow: visible`). Score:
    5 of 15 headers wrong about the string they defended; O-Bass, O-Tremolo, O-Bassoon,
    O-SpectralShaper (and O-TextureForge on *Dispersion X*) held to the hundredth.
    Also: **a renderer's bottom FLOOR can absorb a grown tip** and park it over the controls
    above with `inFrame` still green (O-Gain, +440 chars) — read tip HEIGHTS before/after,
    not only in-frame. And `.settings-toggle`'s `min-width: 40px` no longer covers *Marche* /
    *Désactivée* on O-FreqPulse and O-SpectralShaper: the button resizes between its own
    French faces, inside popover slack, both geometry gates green — a CSS decision recorded
    in comments, not taken (item 40).

## From N4 complete (6 of 6: O-Bowed `a7ff4f35`, O-Emulator `8d2bfcba`, O-ReverseDelay `33feb09c`, O-Octagon `af42a44e`, O-MultiBandCompressor `5247ce85`, O-Orbit `79b4cc93`)

31. **BUILD TRAP — `touch plugins/<Name>/CMakeLists.txt` immediately before the build, and
    read the INSTALLED `Info.plist` before calling a version bump shipped.** A concurrent
    executor's CMake regenerate can stamp `build/build.ninja` NEWER than your `CMakeLists`
    edit, so ninja treats the re-run edge as satisfied: the binary-data `i18n.js` re-embeds
    (every string grep passes) while `JuceLibraryCode/<target>/Info.plist` keeps the OLD
    version. O-ReverseDelay shipped 1.10.1 source in a 1.10.0 bundle on its first build and
    caught it. Verify with
    `/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" ~/Library/Audio/Plug-Ins/Components/<Name>-dev.component/Contents/Info.plist`.
    (All 25 bundles shipped so far were audited after the fact: none stale.)
32. **The glossary grew again from measurements:** `crush` accepts *écras.* / *broyage*;
    `confirm?` accepts *sûr ?*; `makeup` accepts *compens.*; `on`/`off` accept *act.* /
    *dés.* for a pill under 46 px; `m/s` is a unit. **T2 skips surround-format names**
    (`5.1`, `7.1`, `7.1.4` — "7,1" is not a channel format). **A forbidden key ending in a
    period now matches** (`Dériv.`, `Fréq.`, `Flatt.` drew G1 and not F1 before).
33. **An exemption on MEANING is the glossary working, not failing** — record it and move
    on: *Fréq.* for a control that IS a frequency under a column already captioned
    *Vitesse* (O-Bowed); *Tenue* for an infinite-sustain switch on a page with no ADSR
    (O-Bowed); *Mid* for the M/S encoding beside bands already named *Médium* (MBC);
    *Retard* for an alignment delay and *Décroissance* for a DBAP distance law on a page
    whose real filter would take *Pente* (O-Octagon); *Temps* as a panel heading over three
    controls of which only one is a duration (O-ReverseDelay); *Mixage réducteur* for a
    fold-down badge (O-Orbit). None of these is width, and none is a `termNote` hiding a miss.
34. **Parameter-choice faces are English on screen in BOTH languages** (`AudioParameterChoice`
    option strings are `I18N_EXEMPT` by design) — so a French body that names a choice must
    name the ENGLISH face the user can see, capitalised: *Orbit*, *Free*, not *Orbite*,
    *Libre* (O-Octagon, 21 choices; O-MultiBandCompressor's M/S tip named faces that do not
    exist in French). A lower-case generic (*une orbite*) is prose and is translated.
35. **A probe that does not park the pointer reports a healthy anchor dead**, in one
    language, non-deterministically — the closing click leaves the cursor on the gear and
    `target === tipTarget` short-circuits (O-Orbit). `mouse.move(2,2)` before each sweep.
36. **`sameAsEn` on an I18N entry is still entry-scoped** — the four executors of N4 all got
    it right; O-Octagon left ten titles unflagged over translated bodies. And the sixth,
    seventh, eighth header width defences went backwards (O-Bowed's "62 px hard cap" was
    raised to 64 at v1.5.0 and never updated; O-Octagon's *Décroissance* "72 px cell"
    measured 81.59 in 88; O-ReverseDelay's *Profondeur* "0.4 px over, a clip" was
    shrink-to-fit with `overflow: visible`; O-Orbit's "9 px pill" renders at 11 px because
    `.toggle-label`'s rule loses on specificity). Score: 9 of 21 headers wrong about the
    string they defended.

## From N5 complete (6 of 6: O-Marimba `2240a80f`, O-Freeze `3bd6b5e8`, O-SimpleReverb `dd1b58e6`, O-DigiDelay `c4a7eac9`, O-Bitrot `143f75c7`, O-Lyrica `d80b0bed`)

37. **T2 keeps its point on an IDENTIFIER and on nothing else:** a surround-format name, a
    token with two or more dots, a token after a version word (*v*, *version*, *pre-*).
    O-Bitrot proposed "any token the English also carries"; tried, it zeroed the column —
    the English writes real decimals with a point too. Rejected; `dfdc9732` is the rule.
38. **A caption that is an `AudioParameterChoice` face byte-for-byte is exempt and stays
    English on the knob** (O-Freeze's THRESHOLD, exempt with a reason since Stage K) — so its
    tip TITLE stays English too, or the control has two names. A `termNote` there is a
    meaning exemption (correction 33), not a miss.
39. **A narrower string can cost a line and move 164 elements** — O-Bitrot's correct
    elision *lorsqu'il est allumé* (117.95 px, narrower than the shipped 128.27) wrapped in
    a 132 px box; *lorsqu'allumé* (124.8, one line) restored the baseline. And a body edit
    can FLIP a clamp verdict: O-Bitrot's first pass pushed one tip 74.5 → 89.4 px and
    flipped it below; trimming two phrases restored 16/13. Read the per-anchor rows.
40. **Glossary: `mallet` roots on *maillet*** (a *mailloche* beats a bass drum); `load
    .scl/.kbm` accept *charg.* like their `save` twins; `on`/`off` accept *act./dés.*
    Two-line fragment captions where French inverts head and modifier (O-Marimba's
    MALLET/HARDNESS → DURETÉ/MAILLET) are the pair, not the row — one `termNote` per half.
41. **Two footers hard-code a version in an exempted string** (O-SimpleReverb `v1.5.5`, four
    versions stale — item 47) — do not touch a text-matched exemption to fix a version; report
    it. And two more plugins have no focus latch at all (O-Lyrica, O-Orbit): no keyboard half.

## From N6 complete (the O-simple* family, 6 of 6: O-simpleBeatmaker `768b07c7`, O-simplePhysicalModelSynth `efadae9c`, O-simpleSubtractive `2d7ee7f9`, O-simpleAdditive `3cc2e392`, O-simpleSampler `e1684b33`, O-simpleGrain `dd69886e`)

42. **The same 77.33 px *Relâchement* got THREE verdicts on three O-simple* pages:** fits
    O-simpleFM's and O-simpleSubtractive's 56/54 px shrink-to-fit cells with room, crosses
    its neighbour by 2.14 px on O-simpleSampler's 54 px cells with 14 px gaps. Measure the
    GAP to the neighbour, not the cell.
43. **One typographic apostrophe can be the line.** O-simpleGrain's tour caption sat at
    5.28 px of slack in an 846 px box; `'` → `’` took it to 1.34; the terminology fix
    wrapped it and moved 159 elements. Reworded shorter, same claim. Measure captions
    NOWRAP, not the wrapped Range box (which reports the widest line and reads as narrower).
44. **Reach low anchors through `scrollIntoView` / the page's own scroll path.** Four of six
    family pages scroll (`.frame` or an inner stage); a raw `mouse.move` past `innerHeight`
    reports healthy anchors dead — three executors lost a probe run to it.
45. **The ui-stub disables hover-help at boot on this family** (`getTipsEnabled` resolves
    `undefined` → `applyTipsEnabled(false)`); drive it on through the page's own toggle
    before the sweep. And a probe verdict written as `title === title && body === body`
    passes with an English TITLE planted — gate the BODY (O-simpleSampler's control).
46. **Glossary:** `fine` collides with an `end` caption (both *Fin*) — an `end` row exists
    now and *affinage* is accepted for `fine` on such a page; *mailloche* is out of the list
    and forbidden in prose; `loop crossfade` has a row. And `bit depth → Résolution` was
    NARROWER than the draft it replaced (65.64 vs 81.77 px) — measure before assuming a
    root is longer.

## From N7 complete (5 of 5: O-Reed `a382dae4`, O-Contrabass `33cecfe5`, O-GrainScatter `f438d23e`, O-Detune `29152c60`, O-MicrotonalSampler `2dadaeeb`)

47. **Prose-forbidden words are now scanned on EVERY row** — a dialog message or an aria
    name in `LABELS` is prose; O-MicrotonalSampler's four *plugiciel* in `label.dragDrop*`
    and `aria.*Preset` drew nothing while the scan was body-only. O-Formant's two will now.
48. **Read a body's cross-references against the captions BOTH ways.** O-Reed's caption said
    *Mode polyphonique* while its tip title and a neighbour's body said *Mode de polyphonie*
    (caption moved); its `tip.polyMode` said *Voix maximales* where the knob reads *Voix
    max* (body moved). O-GrainScatter's draft had spent ONE French word on both Spread
    and Scatter on a page that has a real Scatter — the glossary split is the proof case.
49. **Glossary rows added from measurements:** `az spread` / `el spread`, `anche dble`,
    `prof. vib`, `écart tot` + `étendue` for `total span`, a `body` row carrying both
    *Corps* and *Caisse* (item 60). **A TERMS key ending in a period is reachable now**
    (`tuning panel failed to load.` was the one unreachable row of ~240). **T7 skips a
    decade name** (`60s/70s/80s` faces on O-Detune) but still fires on `20s`.
50. **Bare `s` in UNITS matches identifiers** the way bare `m`/`N` did — report, do not
    `termNote` a typography code (it exempts G1/F1 only). And a `%` split across two
    concatenated literals (`'… à 100 '` + `'% il …'`) reports T3 that no per-literal
    rewriter sees — re-run the lint after any body re-wrap, not only after the pass.
51. **A stylistic rewrite that hits the renderer's bottom floor gets reverted** (O-Reed's
    `tip.revBore`: top 409.9 → 393.2, bottom clearance pinned, growth going upward over the
    controls with `inFrame` green). And *Écart total* at 53.72 px in a 53.00 px `flex: 0 1
    auto` pin moves the wrapper by 0.72 px — assertion 7 sees it; the abbreviation exists now.
