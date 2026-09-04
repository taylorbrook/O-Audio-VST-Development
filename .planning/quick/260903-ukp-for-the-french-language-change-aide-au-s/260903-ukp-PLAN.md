---
quick_id: 260903-ukp
type: execute
mode: quick
autonomous: false
files_modified:
  - scripts/i18n-fr-glossary.js
  - .planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-FR-GLOSSARY.md
  - plugins/{43 plugins}/Source/ui/public/js/i18n.js  or  plugins/{43}/Resources/ui/js/i18n.js
  - plugins/{43 plugins}/CMakeLists.txt
  - plugins/{43 plugins}/CHANGELOG.md
  - plugins/O-Gain/Source/ui/public/index.html            # stale width comment
  - plugins/O-Prism/Source/ui/public/index.html           # stale width comment
  - plugins/O-FreqPulse/Resources/ui/css/styles.css       # stale width comment
  - plugins/O-simplePhysicalModelSynth/Source/ui/public/css/styles.css  # stale width comment

estimate:
  tokens: 105000
  raw_tokens: 140000
  tasks: 3
  confidence: low

must_haves:
  truths:
    - "In every plugin, switching the UI to Français shows the settings caption as Infobulles, not Aide au survol."
    - "The tooltip that explains the switch reads in grammatical plural French: ces infobulles, une fois désactivées."
    - "i18n-fr-lint's G1 gate now enforces Infobulles as the only accepted rendering — the old term becomes a lint failure, not a tolerated alternate."
    - "No plugin's geometry moved: check-ui-labels reports 0 moved elements on all 43 in both languages."
    - "Every plugin whose i18n.js changed is patch-bumped, CHANGELOG'd, rebuilt, reinstalled and auval-clean, so a DAW actually shows the new word."
  artifacts:
    - "scripts/i18n-fr-glossary.js — TERMS['hover help'] and TERMS['toggle hover help'] carry the new roots, root-only (no legacy alternate)"
    - "43 i18n.js files with every French string re-agreed to the plural noun"
    - "4 re-measured width comments"
    - "43 dated CHANGELOG sections + 43 patch VERSION bumps"
  key_links:
    - "The glossary root and the plugin strings must change TOGETHER — between the two edits, i18n-fr-lint is red for every unedited plugin. That transient red IS the positive control; it must be observed once and then driven to zero."
    - "reviewed:true is only legitimate for a string the developer has read. The Task-1 checkpoint is what makes it legitimate; without it, every changed body is reviewed:false."
    - "i18n.js is juce_add_binary_data — nothing reaches a DAW until the plugin is rebuilt and reinstalled."
---

<objective>
Replace the French rendering of the hover-help surface across all 43 plugins:
**"aide au survol" → "infobulles"**, with every sentence re-agreed for the change
from a feminine singular noun phrase to a feminine plural one.

Purpose: the developer has chosen the word French interfaces actually use for the
surface. The Stage-N glossary settled on *aide au survol* — which names the
ACTION (help on hover) rather than the THING — and this reverses that choice
suite-wide, in one pass, through the same glossary + lint that made the old term
uniform.

Output: a new glossary root that the lint enforces; 231 occurrences across 43
i18n.js files rewritten in grammatical plural French; 4 stale width comments
re-measured; 43 patch releases built, installed and auval-clean.
</objective>

<context>
@/Users/taylorbrook/Dev/VST-development/CLAUDE.md
@/Users/taylorbrook/Dev/VST-development/scripts/i18n-fr-glossary.js
@/Users/taylorbrook/Dev/VST-development/scripts/i18n-fr-lint.js
</context>

---

## THE SCAN (measured 2026-09-03 by live grep — a guide, not the authority)

**231 occurrences** of `aide au survol` (case-insensitive) in **43** `i18n.js`
files, 4–9 per plugin. Two path families, both live:

- `plugins/<Name>/Source/ui/public/js/i18n.js` — 30 plugins
- `plugins/<Name>/Resources/ui/js/i18n.js` — 13 plugins (O-Bassoon, O-Bells,
  O-Bowed, O-FreqPulse, O-Lyrica, O-MicrotonalSampler, O-Orbit, O-Reed,
  O-SpectralShaper, O-Wind, …)

Enumerate at execution time; do not work from this list:

```
ls plugins/*/Source/ui/public/js/i18n.js plugins/*/Resources/ui/js/i18n.js
```

**96 distinct lines** carry the phrase: ~8 recurring shapes (the settled chrome
strings, below) plus **~57 one-off tooltip bodies** whose prose is unique to one
plugin. The one-offs are the real work; the 8 shapes are 174 of the 231 hits.

Outside `i18n.js`, the phrase appears in exactly **4 source comments** (below),
in ~35 `CHANGELOG.md` files (**history — never rewrite**) and in `.planning/`
(out of scope).

`scripts/i18n-canon.js` does not contain the phrase. The byte-compared
`applyI18n`/`initI18n` region is untouched by this task — **do not edit inside it.**

---

## THE GRAMMAR (this is not a find-and-replace)

`aide au survol` is **feminine singular**. `infobulles` is **feminine plural**.
Every sentence containing it has to be re-agreed:

| before | after |
|---|---|
| `cette aide au survol` | `ces infobulles` |
| `l’aide au survol` | `les infobulles` |
| `de l’aide au survol` | `des infobulles` |
| `d’aide au survol` (l’interrupteur d’…) | `d’infobulles` — or `des infobulles` where the article reads better |
| `toute l’aide au survol` | `toutes les infobulles` |
| `chaque aide au survol` | `chaque infobulle` ← **the one singular case** |
| `Une fois désactivée` | `Une fois désactivées` |
| `…l’aide au survol s’affiche` | `…les infobulles s’affichent` |
| `…cette aide au survol est rédigée` | `…ces infobulles sont rédigées` |

**The trap: bare back-references.** Several bodies name the surface once and then
refer back with a bare *l’aide* / *cette aide* on a later clause — for example
`…le réglage de l’aide est conservé sur cet ordinateur`. Those clauses contain no
occurrence of the search phrase, so a regex leaves them behind pointing at an
antecedent that no longer exists. **Read whole sentences, not matched lines.**

Preserve exactly: the typographic apostrophe **U+2019 `’`** (lint T1), every
` ` / literal U+00A0 (lints T3–T7), the `«` `»` quoting of screen text, and
the `'…' + '…'` line-continuation structure — rewriting only the fragment that
actually contains the phrase keeps continuations intact.

**Casing (lint C1):** no all-caps rendering of the caption exists today (measured:
zero `AIDE AU SURVOL` in any i18n.js). If the live grep finds one, it becomes
`INFOBULLES`. English titles pair as `Hover help` (43×) and `Hover Help` (32×);
French does not title-case, so `Infobulles` is correct for both.

---

## THE SETTLED SHAPES (verbatim before → after; 174 of the 231 hits)

```
1.  fr: { t: 'Aide au survol',
 →  fr: { t: 'Infobulles',                                   (also the "…" double-quoted variant)

2.  'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Aide au survol', reviewed: true } },
 →  'label.hoverHelp': { en: { t: 'Hover help' }, fr: { t: 'Infobulles', reviewed: true } },

3.  fr: { t: 'Activer ou désactiver l’aide au survol', reviewed: true }
 →  fr: { t: 'Activer ou désactiver les infobulles', reviewed: true }

4.  b: 'Active ou désactive cette aide au survol. Une fois désactivée, seuls '
      + 'l’engrenage et ce commutateur continuent de s’expliquer.',
 →  b: 'Active ou désactive ces infobulles. Une fois désactivées, seuls '
      + 'l’engrenage et ce commutateur continuent de s’expliquer.',
    (23 sites split across two lines exactly as shown; 6 more on one line)

5.  …et l’affichage de l’aide au survol…      →  …et l’affichage des infobulles…

6.  La langue de cette aide au survol et des libellés de la page.
 →  La langue de ces infobulles et des libellés de la page.

7.  …les libellés de cette page et de cette aide au survol…
 →  …les libellés de cette page et de ces infobulles…

8.  …les libellés de cette page et cette aide au survol changent avec elle…
 →  …les libellés de cette page et ces infobulles changent avec elle…

9.  fr: { t: 'Langue de l’aide au survol', reviewed: true }      (1 site)
 →  fr: { t: 'Langue des infobulles', reviewed: true }
```

---

## THE REVIEWED-FLAG POLICY (decided — state it in the SUMMARY)

The contract (check-i18n.js:323-350): `reviewed: true` on a French entry means
**the developer, who reads French, read that exact string.** `false` is not a
failure; it feeds the unreviewed worklist and its `TOTAL` line, which currently
reads **0**.

The policy for this task, and why it differs from 260903-rjm (which kept
`reviewed: true` because its strings were byte-identical copies of already-read
ones — here every string changes):

1. **Titles and aria keep `reviewed: true`.** `Infobulles`,
   `Activer ou désactiver les infobulles`, `Langue des infobulles` are the word
   the developer dictated plus the article French grammar forces. Nothing was
   authored.
2. **Bodies are prose that had to be re-negotiated**, so they are read before
   they ship, not after. Task 1 puts the **complete before→after sheet for every
   distinct string in the suite** in front of the developer as a blocking
   checkpoint — *before* any of the 43 files are edited. Approved wording is then
   developer-read by definition and carries `reviewed: true`.
3. **Consequence, and the reason for this ordering:** the worklist `TOTAL` stays
   at 0, and every plugin is built exactly **once**. Editing at
   `reviewed: false` and flipping later would mean 43 second builds for a
   metadata-only change.
4. Any string the developer edits at the checkpoint ships in **their** wording,
   not the proposal's.

---

## THE FOUR STALE COMMENTS (re-measure, never guess)

These cite the OLD caption's measured width. `Infobulles` is 10 glyphs to
`Aide au survol`'s 14, so nothing needs widening — but the numbers become false
and a false canon line is a line the next three authors reach around.

| file | what it says now |
|---|---|
| `plugins/O-Gain/Source/ui/public/index.html:543-556` | a four-row width table; `fr Aide au survol 70.22 + pill 52 = 132.22 slack 21.78 <- widest fr` |
| `plugins/O-Prism/Source/ui/public/index.html:118-127` | prose justifying the pinned `.settings-popover` width — *"the French AIDE AU SURVOL is far wider than the 59.77 px LANGUAGE box"* |
| `plugins/O-FreqPulse/Resources/ui/css/styles.css:924` | `76.55 (Aide au survol) + 12 gap + 57 = 145.55 in the 162px content box` |
| `plugins/O-simplePhysicalModelSynth/Source/ui/public/css/styles.css:250` | `"Hover help" 72.0px / "Aide au survol" 96.0px` |

Get real numbers from `node scripts/check-ui-labels.js --plugin <Name> --verbose`,
which renders each page in both languages and reports rectangles. **Do not remove
any width pin.** O-Prism's pin exists so the panel's left edge cannot move with
language; a shorter French caption does not retire that reason. Restate the
example, keep the rule.

---

## AN ADJACENT FINDING — decide at the checkpoint, do not act unilaterally

The phrase is not the only French rendering of this concept in the suite. Eight
prose sites name it something else and would survive this change untouched,
leaving three renderings where the glossary exists to permit one:

```
plugins/O-AnalogEQ/…/i18n.js:406        'et cette aide contextuelle. …'
plugins/O-Formant/…/i18n.js:768         'les légendes de bouton et toute l’aide contextuelle de la page. …'
plugins/O-GrainScatter/…/i18n.js:729    'Choisit la langue du texte de l’interface et de cette aide contextuelle. …'
plugins/O-Bells/…/i18n.js:746           'Bascule chaque libellé et chaque bulle d’aide de la page …'
+ 2 sites reading 'ces descriptions au survol' / 'ces explications au survol'
+ 2 source COMMENTS in O-Bass:107 and O-Tremolo:78 recording the old settlement
```

**Recommendation: fold them in** — it is 6 prose lines and 2 comments, and one
concept with three names is the exact defect the glossary was built to close.
Present them as a yes/no at the Task-1 checkpoint. `Survolez …` verb forms
(6 sites) describe the ACTION and stay as they are.

---

## GATES (per plugin, exit 0 required)

```
node scripts/check-i18n.js       --plugin <Name>
node scripts/i18n-fr-lint.js     --plugin <Name>
node scripts/check-ui-labels.js  --plugin <Name>     # exit 77 = NOTHING verified, never a pass
node scripts/boot-all-uis.js     --plugin <Name> --strict-tips
node plugins/<Name>/tests/ui_tip_render_check.js     # exists for 22 of 43
node scripts/i18n-zh-lint.js     --plugin O-Chorus   # O-Chorus only (the one zh-Hans plugin)
```

Chinese is untouched: only O-Chorus speaks `zh-Hans`, its 悬停帮助 renderings are
a separate settled root, and this task does not open them.

---

<tasks>

<task type="tracer" tdd="false">
  <name>Task 1: settle the wording, get it read, and prove it end-to-end on O-Gain</name>
  <files>
    scripts/i18n-fr-glossary.js
    .planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-FR-GLOSSARY.md
    plugins/O-Gain/Source/ui/public/js/i18n.js
    plugins/O-Gain/Source/ui/public/index.html
    plugins/O-Gain/CMakeLists.txt
    plugins/O-Gain/CHANGELOG.md
  </files>
  <read_first>
    scripts/i18n-fr-glossary.js (lines 38-70 the normalisation contract, 76-83 the shared chrome)
    scripts/i18n-fr-lint.js (lines 24-77 the check list, G1 and C1 in particular)
    plugins/O-Gain/Source/ui/public/js/i18n.js (all 7 sites)
    plugins/O-Gain/Source/ui/public/index.html (lines 530-560, the width table)
  </read_first>
  <action>
    **Step A — extract the real inventory.** Live grep is the authority; the scan
    table above is a guide that may already be stale. Produce, from the 43
    i18n.js files, the list of every DISTINCT French string containing the phrase
    case-insensitively, with its occurrence count and the plugins it appears in.
    Expect roughly 96 distinct lines.

    **Step B — author the rewrite for every one of them.** Apply the grammar table
    and the settled shapes above. Read whole sentences and their neighbouring
    clauses so the bare back-references (l’aide / cette aide with no phrase on the
    line) are caught. Preserve U+2019, every U+00A0 and ` ` escape, the
    « » quoting, and the `+` continuation split.

    **Step C — CHECKPOINT (below).** The sheet goes to the developer before any
    plugin file is edited.

    **Step D — the glossary, root-only.** In scripts/i18n-fr-glossary.js:

        'hover help':        ['infobulles'],
        'toggle hover help': ['activer ou désactiver les infobulles'],

    Deliberately NOT additive — the old rendering is removed, not kept as an
    accepted alternate, so that from this commit forward G1 is the standing gate
    against a plugin drifting back. Add the matching row to the prose companion's
    "The settled terms — and why" table (it currently has no entry for this term
    at all, and the glossary header requires the two to change together): English
    `Hover help`, French **Infobulles** (toggle: *Activer ou désactiver les
    infobulles*), reason — reverses the Stage-N choice; *infobulle* is the word
    French DAW and OS interfaces use for the surface, while *aide au survol* named
    the action; plural because the control governs all of them, so every sentence
    agrees plural.

    **Step E — fire the positive control, and record it.** With the glossary
    changed and no plugin touched yet, run:

        node scripts/i18n-fr-lint.js --plugin O-Gain     ; echo "rc=$?"
        node scripts/i18n-fr-lint.js                     ; echo "rc=$?"

    Both MUST exit 2 with G1 findings on `hover help` / `toggle hover help`, and
    the repo-wide run must name all 43 plugins. A silent pass here means the
    glossary edit is not reaching the gate and the whole G1 arm of this task is
    decoration. Record both exit codes and the plugin count in the CHANGELOG and
    the SUMMARY. Expect and accept that fr-lint stays red repo-wide until the last
    batch of Task 2 lands — it is a scoreboard through the rollout, and only the
    per-plugin runs are green as you go.

    **Step F — O-Gain, end to end.** Apply the approved sheet to all 7 sites in
    plugins/O-Gain/Source/ui/public/js/i18n.js. Then re-measure and rewrite the
    width table in index.html lines 543-556 from
    `node scripts/check-ui-labels.js --plugin O-Gain --verbose` — the row that was
    the widest French one almost certainly is not any more, so the table's
    conclusion changes, not just its numbers. Do not touch the popover's pinned
    geometry.

    Read VERSION out of plugins/O-Gain/CMakeLists.txt and bump the PATCH digit
    (it read 1.3.2 at planning time — trust the file, not this sentence). Add a
    dated `## [x.y.z] - 2026-09-03` section to plugins/O-Gain/CHANGELOG.md above
    the current top section; never edit an existing one.

    Build and install per CLAUDE.md: `./scripts/build-and-install.sh O-Gain`
    (its Phase 4 does the dev/release dual-variant sweep — do not hand-copy).
    Then auval: get the type and 4-char code from plugins/O-Gain/CMakeLists.txt
    (`PLUGIN_CODE`, `PLUGIN_MANUFACTURER_CODE`, `IS_SYNTH`) or discover the triple
    with `auval -a | grep -i o-gain`; do not assume a code.

    Commit path-scoped, after re-checking `git branch --show-current` and
    `git status --short` in the same breath as the commit — another session shares
    this index and HEAD:

        git commit -- scripts/i18n-fr-glossary.js \
          .planning/quick/260826-ieq-multi-language-tooltips-across-all-vst-p/260826-ieq-FR-GLOSSARY.md \
          plugins/O-Gain

    Never `git add -A`. Never stage anything under plugins/O-Orbit/libs/SAF.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js --plugin O-Gain && node scripts/i18n-fr-lint.js --plugin O-Gain && node scripts/check-ui-labels.js --plugin O-Gain && node scripts/boot-all-uis.js --plugin O-Gain --strict-tips && echo GATES-0</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && grep -c "Infobulles" plugins/O-Gain/Source/ui/public/js/i18n.js && grep -q "les infobulles" plugins/O-Gain/Source/ui/public/js/i18n.js && grep -q "'infobulles'" scripts/i18n-fr-glossary.js && echo WIRED</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && grep -v '^ *[/*]' plugins/O-Gain/Source/ui/public/js/i18n.js | grep -ic "aide au survol" | grep -qx 0 && echo O-GAIN-CLEAN</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js 2>&1 | sed -n '/unreviewed French/,/TOTAL/p' | tail -1</automated>
    <human-check>Sheet review, blocking — see the checkpoint task. Then, once O-Gain is installed: open it in Logic, switch to Français, open the gear. The caption reads Infobulles. Hover it: the tip title reads Infobulles and the body reads "Active ou désactive ces infobulles. Une fois désactivées, …". Hover the language row: its body names les infobulles in agreement. Switch back to English — the panel does not move.</human-check>
  </verify>
  <done>
    The glossary carries the new roots root-only; its prose companion documents the
    reversal; the positive control fired (both fr-lint runs exited 2 naming 43
    plugins, exit codes recorded); O-Gain is patch-bumped, its width table
    re-measured, installed, auval-clean, four gates at 0, and zero occurrences of
    the old phrase remain in its i18n.js. The unreviewed-French TOTAL still reads
    0. Committed path-scoped.
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking-human">
  <name>Checkpoint: the developer reads the before→after sheet</name>
  <what-built>
    Task 1 Step B produced the complete rewrite sheet: every distinct French
    string in the suite that names the hover-help surface, with its proposed
    replacement, its occurrence count and the plugins it appears in. O-Gain has
    already been through the whole recipe end-to-end with those strings, so the
    sheet is proven to build, install, pass five gates and move no geometry.
    Nothing else has been edited yet — 42 plugins are still on the old wording,
    deliberately, because this read is what authorizes them.
  </what-built>
  <how-to-verify>
    The sheet is presented **grouped by distinct string, not by plugin** — ~96
    rows rather than 231. Order: the 8 settled shapes first with their counts,
    then the ~57 one-off tooltip bodies with the plugin each belongs to, then the
    bare back-reference clauses that had to be rewritten even though they never
    contained the search phrase.

    Three questions:

    1. **The wording.** Read every row, before and after. Anything you edit ships
       in your words. This read is what makes `reviewed: true` legitimate for the
       bodies — it is the whole reason this checkpoint sits before the edits
       rather than after them.
    2. **The adjacent finding.** 6 prose sites and 2 comments render the same
       concept as *aide contextuelle* / *bulle d’aide* / *descriptions au survol*
       / *explications au survol*. Fold them into `infobulles` too, or leave them?
       (Recommendation: fold — three names for one concept is the defect the
       glossary exists to close, and it is 8 lines.)
    3. **The root-only glossary edit.** Confirm the old term is REMOVED as an
       accepted rendering rather than kept as an alternate, so a plugin drifting
       back becomes a red G1 gate rather than a silent pass.
  </how-to-verify>
  <resume-signal>
    Reply "approved" to ship the sheet as proposed, or paste your edits row by
    row. Answer questions 2 and 3 explicitly — silence on either is not an
    approval, and Task 2 does not start without both.
  </resume-signal>
  <done>Wording approved (with any edits captured verbatim), fold-in decided, glossary policy confirmed. Only then do the remaining 42 plugins get edited.</done>
</task>

<task type="auto" tdd="false">
  <name>Task 2: the other 42 plugins, in seven path-scoped batches</name>
  <files>
    plugins/{42 remaining}/Source/ui/public/js/i18n.js  or  plugins/{…}/Resources/ui/js/i18n.js
    plugins/{42 remaining}/CMakeLists.txt
    plugins/{42 remaining}/CHANGELOG.md
    plugins/O-Prism/Source/ui/public/index.html
    plugins/O-FreqPulse/Resources/ui/css/styles.css
    plugins/O-simplePhysicalModelSynth/Source/ui/public/css/styles.css
  </files>
  <precondition>Task 1 is committed, its gates are green, and the sheet is approved — the wording is only canon once the developer has read it.</precondition>
  <action>
    Apply the approved sheet to the remaining 42 plugins in **batches of six**.

    **Scripting is allowed for the settled shapes only.** A mechanical
    substitution may run against a string whose exact before-text is a row of the
    approved sheet. Anything the live grep turns up that is NOT on the sheet
    **halts the batch** and goes back to the developer — the scan is a guide, the
    grep is the authority, and a string nobody read must not ship at
    `reviewed: true`. The ~57 one-off bodies and every bare back-reference clause
    are read individually as they are edited, not pattern-matched.

    Per plugin:
      - Rewrite every French string per the sheet. Preserve U+2019, U+00A0 and
        ` `, « » quoting, and the `+` continuation split — rewrite only the
        fragment carrying the phrase.
      - Keep every `reviewed: true` as-is (policy above). Do not touch any
        `zh-Hans` arm; only O-Chorus has one.
      - Stay OUT of the `applyI18n`/`initI18n` region — check-i18n assertion [6]
        byte-compares it against scripts/i18n-canon.js.
      - Read VERSION from that plugin's CMakeLists.txt and bump the PATCH digit.
        **Never carry a version number in from a table** — the 260903-rjm plan's
        table had O-Texture four minor versions too high and inventing them would
        have shipped releases that never existed.
      - Add a dated `## [x.y.z] - 2026-09-03` CHANGELOG section. Never rewrite an
        existing section: ~35 CHANGELOGs mention the old phrase as history and
        that history is correct.

    Three plugins carry extra work beyond their i18n.js:
      - **O-Prism** — re-state the popover-width justification comment
        (index.html:118-127) with the measured French caption width. KEEP the pin.
      - **O-FreqPulse** — re-measure `Resources/ui/css/styles.css:924`; the sum
        `76.55 + 12 + 57 = 145.55 in the 162px content box` changes.
      - **O-simplePhysicalModelSynth** — re-measure
        `Source/ui/public/css/styles.css:250` (`"Aide au survol" 96.0px`).
      Numbers come from `check-ui-labels --plugin <Name> --verbose`, not arithmetic.

    Per batch, in this order:
      1. The four gates on each plugin, plus `ui_tip_render_check.js` where the
         file exists (22 of 43 have one; `n/a` is a legitimate answer, a skipped
         run that exists is not).
      2. `./scripts/build-and-install.sh <Name>` for each — i18n.js is
         juce_add_binary_data, so nothing reaches a DAW without this.
      3. `auval` each, with the triple read from CMakeLists.txt or discovered
         via `auval -a | grep -i "$NAME"`.
      4. Re-check `git branch --show-current` and `git status --short`, then
         commit path-scoped:
         `git commit -- plugins/<A> plugins/<B> plugins/<C> plugins/<D> plugins/<E> plugins/<F>`

    Never `git add -A`. Never stage under plugins/O-Orbit/libs/SAF.

    Report after each batch. **Stop on a failing batch** rather than carrying a
    broken recipe into the next six. This is 42 builds — minutes each, not
    seconds.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && for f in plugins/*/Source/ui/public/js/i18n.js plugins/*/Resources/ui/js/i18n.js; do n=$(grep -v '^ *[/*+]' "$f" | grep -ic "aide au survol"); [ "$n" != "0" ] && echo "REMAINS $n: $f"; done; echo "sweep done"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/i18n-fr-lint.js; echo "fr-lint rc=$?"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js; echo "check-i18n rc=$?"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && for p in $(ls plugins); do [ -f "plugins/$p/CMakeLists.txt" ] || continue; ls plugins/$p/Source/ui/public/js/i18n.js plugins/$p/Resources/ui/js/i18n.js >/dev/null 2>&1 || continue; node scripts/check-ui-labels.js --plugin $p >/dev/null 2>&1; rc=$?; [ $rc -ne 0 ] && echo "UI-LABELS rc=$rc: $p"; done; echo "ui-labels sweep done"</automated>
    <human-check>Spot-check four installed plugins from different families in Logic — one Resources/ui plugin (O-Wind or O-Reed), O-Prism (the pinned flat popover), O-FreqPulse (the 162 px content box) and O-simplePhysicalModelSynth (the 96 px caption note). In each: Français shows Infobulles, the tooltip prose agrees plural, nothing clips, and the panel does not move between languages.</human-check>
  </verify>
  <done>
    The first verify prints only "sweep done" — no runtime occurrence of the old
    phrase survives in any i18n.js outside a comment. All 42 are patch-bumped,
    CHANGELOG'd, built, installed, auval-clean and committed in seven path-scoped
    batches. The three extra comment sites carry re-measured numbers and their
    width pins are intact.
  </done>
</task>

<task type="auto" tdd="false">
  <name>Task 3: repo-wide close-out — every gate green at once, and the proof of zero</name>
  <files>.planning/quick/260903-ukp-for-the-french-language-change-aide-au-s/260903-ukp-SUMMARY.md</files>
  <precondition>All 43 plugins are committed — the repo-wide fr-lint is red by construction until the last batch lands, so running it earlier proves nothing.</precondition>
  <action>
    Run every repo-wide gate in one pass and record each exit code verbatim. A
    number you did not watch print is not a result.

      1. `node scripts/check-i18n.js` — 43/43, exit 0, assertion [16] still live.
         Read the `-- unreviewed French` TOTAL line: it must still be **0**. A
         non-zero TOTAL means a body shipped that the developer did not read, and
         it is listed by plugin in that same block.
      2. `node scripts/i18n-fr-lint.js` — exit 0. This is G1 now enforcing
         `infobulles`; it was exit 2 across all 43 the moment the glossary changed
         in Task 1, so a 0 here is the closed loop, not an assumption.
      3. `node scripts/boot-all-uis.js --strict-tips` — 43/43, 0 DEAD bindings.
      4. `node scripts/i18n-zh-lint.js --plugin O-Chorus` — exit 0 (the Chinese
         arm must be untouched).
      5. `check-ui-labels` across all 43 — 0 moved elements. Exit 77 is NOT a pass.
      6. The zero-occurrence proof, repo-wide, excluding the four places the phrase
         legitimately survives — CHANGELOG history, `.planning/` (this plan and its
         predecessors quote the old string by necessity), `backups/`, `build/`,
         and the SAF submodule:

         grep -ril "aide au survol" --exclude-dir=build --exclude-dir=backups \
           --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=SAF \
           --exclude-dir=.planning --exclude=CHANGELOG.md . ; echo "rc=$?"

         `rc=1` (grep found nothing) is the pass. Any file listed is a miss.
         Two source comments in O-Bass and O-Tremolo record the OLD settlement as
         history; if the checkpoint chose to leave them, they are the only
         permitted hits and must be named explicitly in the SUMMARY as such.

    Then write the SUMMARY at
    `.planning/quick/260903-ukp-for-the-french-language-change-aide-au-s/260903-ukp-SUMMARY.md`
    in the 260903-rjm house format:
      - frontmatter: quick_id, type, mode, status, date, tasks_completed,
        plugins_touched / built / auval_pass, commits (one line per batch sha),
        files_changed, actuals {tokens, tasks, commits}
      - a per-plugin table: `# | Plugin | version before → after | build | auval |
        CI | UL | BU | FR | TR` — every cell an exit code, `n/a` where the file
        does not exist. No blanks.
      - **the review sheet as an appendix**: every distinct string, before → after,
        with occurrence count. This is the durable record of what the developer
        read, and it is what makes every `reviewed: true` in this task auditable a
        year from now.
      - the four re-measured width comments, old numbers → new numbers.
      - the positive-control record from Task 1 Step E: both fr-lint exit codes and
        the plugin count the repo-wide run named.
      - the fold-in decision from the checkpoint, and what was done about it.
      - anything the scan got wrong. The 260903-rjm SUMMARY's most useful lines
        were the two that corrected its own plan; do the same here.

    Commit: `git commit -- .planning/quick/260903-ukp-for-the-french-language-change-aide-au-s`
    after re-checking branch and staging.
  </action>
  <verify>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/check-i18n.js >/tmp/ukp-ci.txt 2>&1; echo "check-i18n rc=$?"; sed -n '/unreviewed French/,/TOTAL/p' /tmp/ukp-ci.txt | tail -1</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/i18n-fr-lint.js; echo "fr-lint rc=$?"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/boot-all-uis.js --strict-tips; echo "boot rc=$?"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && node scripts/i18n-zh-lint.js --plugin O-Chorus; echo "zh rc=$?"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && grep -ril "aide au survol" --exclude-dir=build --exclude-dir=backups --exclude-dir=.git --exclude-dir=node_modules --exclude-dir=SAF --exclude-dir=.planning --exclude=CHANGELOG.md . ; echo "grep rc=$? (1 = clean)"</automated>
    <automated>cd /Users/taylorbrook/Dev/VST-development && git status --short | grep -v '^?? ' | grep . && echo "DIRTY — uncommitted work remains" || echo "tree clean"</automated>
  </verify>
  <done>
    check-i18n exits 0 with the unreviewed TOTAL still at 0; i18n-fr-lint exits 0
    repo-wide with `infobulles` as the enforced root; boot-all-uis reports 43/43
    and 0 DEAD; the zh arm is untouched; the repo-wide grep finds nothing outside
    CHANGELOG history and .planning; the working tree is clean; the SUMMARY carries
    the per-plugin table, the full review sheet and the positive-control record.
  </done>
</task>

</tasks>

<verification>
1. `node scripts/check-i18n.js` — 43/43, exit 0, unreviewed French TOTAL = 0.
2. `node scripts/i18n-fr-lint.js` — exit 0 repo-wide, with G1 now enforcing
   `infobulles`, and the Task-1 record showing it exited 2 across 43 plugins the
   moment the glossary changed (the control that makes the 0 mean something).
3. `node scripts/boot-all-uis.js --strict-tips` — 43/43, 0 DEAD.
4. `node scripts/i18n-zh-lint.js --plugin O-Chorus` — exit 0; Chinese untouched.
5. `check-ui-labels` exit 0 on all 43 (77 is not a pass); 0 moved elements.
6. Repo-wide grep finds the old phrase nowhere outside CHANGELOG.md, `.planning/`,
   `backups/` and `build/`.
7. Every plugin patch-bumped, CHANGELOG'd, rebuilt, reinstalled, auval-clean.
8. `git log --oneline` shows only path-scoped commits under `plugins/<Name>`,
   `scripts/i18n-fr-glossary.js` and the two `.planning` docs. No `git add -A`,
   nothing staged under `plugins/O-Orbit/libs/SAF`.
</verification>

<success_criteria>
- A French user of any of the 43 plugins sees **Infobulles**, and every sentence
  about it agrees in the plural.
- The glossary enforces the new root and would fail a plugin that drifts back —
  proven by the control firing, not asserted.
- No geometry moved anywhere; the four width comments state measured numbers
  rather than the old caption's.
- Every `reviewed: true` in this change is backed by a string the developer
  actually read, and the sheet that records which ones is in the SUMMARY.
</success_criteria>

<risks>
- **Scale.** 43 plugins, 231 occurrences, 43 builds. Bigger than 260903-rjm (23)
  and than Stage N's copy-only pass. Batches of six, stop on failure.
- **Grammar, not substitution.** The plural agreement and the bare back-references
  (`le réglage de l’aide…`) are where a scripted pass silently ships broken
  French. The one-off bodies are read, not matched.
- **The transient red.** From the glossary commit until the last batch,
  `i18n-fr-lint` is exit 2 repo-wide. That is intended and is the positive
  control; it is also a window in which another session running the gate will see
  red for a reason that is not their fault. Land Task 2 promptly.
- **Concurrent sessions** share this checkout's index and HEAD. Re-check branch
  and staging immediately before every commit, not once at the start.
- **The adjacent finding** (aide contextuelle / bulle d’aide) is a scope question,
  not a scope creep — it is 8 lines and it is put to the developer, not decided
  by the executor.
</risks>

<output>
Write `.planning/quick/260903-ukp-for-the-french-language-change-aide-au-s/260903-ukp-SUMMARY.md` when done.
</output>