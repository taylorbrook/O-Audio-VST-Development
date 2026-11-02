# Wave 4d — deferred items and structural findings

Carry-forward for **wave 4e** (O-Lyrica, O-simpleBeatmaker, O-simpleFM,
O-Tapestop, O-simplePhysicalModelSynth). Written in the shape waves 4a–4c used:
numbered structural findings first — each one costs a later executor time or a
wrong answer if not read — then the deferred items with their evidence, then the
explicit instruction list.

---

## Structural findings

### M1. The FIFTH enumeration form is real, and it is the one no grep can see

Wave 4c's N2 catalogued four forms across four gate files and asked whether a
fifth would turn up. It did, on `plugins/O-ReverseDelay/tests/ui_tooltip_clamp_check.js`:
**a per-language MAP LOOKUP**, three sites reading a `Map` by a language key
written as a string literal.

| form | what it looks like | what finds it |
|---|---|---|
| 1. assertion on `LANGUAGES.join(',')` | names the export | a repo-wide census keyed on the identifier |
| 2. a call-site walk | `for (const lang of …)` | reading the file |
| 3. a two-element array literal | `['en', 'fr']` | a grep for the literal pair |
| 4. no language reference at all | — | reading the file, and recording the absence |
| **5. a per-language map read** | **`tipTextByLang.get('fr')`** | **reading the file — a Map read spells no list** |

Why it matters more than the other four: **two of the three sites fed a REAL
hard-fail assertion.** Deriving the list and repairing both walks would have left
that assertion comparing exactly two languages — the Chinese page driven,
measured, and never compared, with the gate reporting green having proved nothing
about the language it had just rendered. A grep for a two-element list finds the
list and never the map read; a grep for a call site finds nothing, because a Map
read spells no list.

The third site fed a `console.log` only. **It was generalized, not left, and the
reason is recorded at the site**: an unexamined print and an unexamined assertion
look identical in a diff, and a report that names only French under a
three-language table stops being true without ever failing.

**Wave 4e should run the census for its five and report which forms are present** —
that is the carry-forward wave 4c asked for and this wave answers.

### M2. A gate can have NO path to the table at all, and then there is nothing to derive FROM

`ui_tooltip_clamp_check.js` contained **zero** occurrences of `I18N`,
`TIP_BINDINGS`, `LANGUAGES`, `loadTable` or any path to `js/i18n.js`. "Derive the
list from the table's own export" was therefore not an edit to an existing link:
there was no link, and one had to be **built** before there was anything to derive
from.

The `vm`-sandbox route is the one to copy — read the file, strip the `export`
keywords, run it in a context, publish `{ I18N, TIP_BINDINGS, LANGUAGES }`. It is
what `scripts/check-i18n.js` does and what the same-named gate on O-Bitrot already
did. `js/i18n.js` is an ES module inside a package with no `"type": "module"`, so
neither `require()` nor a synchronous `import()` can read it; O-Formant's gate
uses a copy-to-tmp-and-dynamic-import shim instead, which also works. **`vm` is
fewer moving parts.**

Budget for this: a loader plus a shape assertion plus an abort is ~35 lines, and
it must land **before** any of the language repair, because nothing else in the
file can be repaired until the list exists.

### M3. A gate can be RED on the tree as found, and a plan can record it as green

`plugins/O-ReverseDelay/tests/ui_frontend_check.js` exits **1**, not 0. The wave-4d
plan asserted it exits 0 in two places — the Task-3 precondition and the
live-observation narrative — and neither assertion had been fired before the plan
was written.

The defect: a D13 assertion written at v1.10.0 required the `ui.on` / `ui.off`
label pair to be **ABSENT**, with the comment "this plugin has NO hover-help
toggle and never will". v1.11.0 shipped that toggle, its markup and its two table
keys, and `check-i18n` assertion 16 has required it on every localized plugin
since. **The premise was falsified two releases ago and the check had been failing
ever since**, asserting the absence of a control the plugin ships.

Two lessons, and the second is the general one:

1. A gate whose comment states a permanent fact about a plugin (*"and never
   will"*) is a gate that will be wrong the first time the plugin changes. Prefer
   an assertion that reads the current table.
2. **Fire every precondition you assert, including the ones that look
   uninteresting.** "This gate passes" is a measurement, not an assumption, in
   exactly the way "this file holds no language reference" is — and wave 4c's C4
   already said so for the second half of that pair.

Inverted rather than deleted: the pair is now REQUIRED, because a toggle whose two
faces are raw literals rather than table keys is stranded in the previous language
the instant the selector fires.

### M4. The two-arm CMake version reader is a SUPERSET, proven

`set(OMBC_VERSION 1.11.2)` on line 9, `VERSION ${OMBC_VERSION}` on line 13 — the
single-arm pattern every prior wave used returns **empty** on that file, and an
executor reading "prints the shipped version" as the criterion sees a blank and
has no signal at all.

```perl
perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}
          if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}'
```

Cross-checked against the single-arm reader on this wave's other four plugins:
**AGREE four for four.** It is a superset, not a second special case, and it is
the reader wave 4e should use unconditionally. **No third shape appeared in this
wave** — the four non-outliers all write an unquoted literal after the keyword.
Wave 4e should report whether any of its five holds its version in a variable or
a quote.

### M5. The installed-family list — a property of THIS MACHINE, re-derived

`system_profiler SPFontsDataType | grep -c "Family: X$"`, run at the start of this
wave:

| family | matches | | family | matches |
|---|---|---|---|---|
| Georgia | 4 | | **Garamond** | **0** |
| Times New Roman | 4 | | **EB Garamond** | **0** |
| Arial | 4 | | **Adobe Garamond Pro** | **0** |
| Menlo | 4 | | **Microsoft YaHei** | **0** |
| Courier New | 4 | | | |
| PingFang SC | 6 | | Helvetica Neue | 14 |
| Songti SC | 4 | | | |

Unchanged from wave 4c — **but the arithmetic changed completely**, because these
plugins stack Garamond alone far more often. Wave 4c found 3 bare-generic
declarations; wave 4d found **15** (O-Formant 12, O-Marimba 3) plus **2 naked
generics** that name no family at all. **Re-derive this at the start of wave 4e.**

### M6. A pre-existing `font-family: inherit` does NOT close the form-4 class — a third and fourth confirmation

Wave 4c's M6a found this on O-Bells (6 declarations, 32 controls still on the UA
face). This wave found it twice more, at opposite scales:

| plugin | `inherit` declarations already present | form-4 controls STILL on the UA face |
|---|---|---|
| O-MultiBandCompressor | 4 | **1** |
| O-FreqPulse | **9** | **24** |
| O-Formant | 0 (no external stylesheet at all) | **31** |
| O-ReverseDelay | 0 | **0** |
| O-Marimba | 0 | **0** |

**A grep of a stylesheet cannot distinguish a repaired plugin from a
partly-repaired one.** Only the computed census can. Report wave 4e's count per
plugin **measured against the `inherit` declarations already in the file** — the
pair of numbers is the finding, not either one alone.

### M7. An ELEMENT-LEVEL form-4 rule is the safest shape when the page is already mostly repaired

O-FreqPulse's repair is `button, input, select, textarea { font-family: Arial, … }`
at the very end of the stylesheet. At specificity **(0,0,1)** it loses to every
class and id rule on the page, so:

- the 77 controls already resolving to Georgia through an explicit `inherit`
  (specificity (0,1,0)) keep Georgia;
- the 8 already naming `'Arial', sans-serif` keep their own stack;
- the 24 that declare **nothing** get the rule.

It reaches exactly the undeclared set, by construction, and needs no enumeration
of ids that would go stale. **Verify by re-running the `undeclared-font` screen
and checking that the OTHER populations did not move**, not by reading the CSS —
this wave printed the per-stack control-count before and after and they matched.

The alternative shape, used on O-Formant, is a **descendant** rule scoped to a
container (`#tuning-container button, …`). Use that when the undeclared set is
confined to one subtree and the census proves every control outside it already
carries a stack. Both shapes were measured before being chosen.

### M8. The `line-height` pin must be derived from the CONTENT box, not the border box

Half of this wave's movers are `<button>` elements with padding and a border. A
naive `h / font-size` on a button with `padding: 7px` and a 1px border reads
24 / 9 = 2.67 and pins a caption to a 24px line box.

Subtract `pt + pb + bt + bb` first, then divide, then check the LINE COUNT (N10).
The measured English content line box is remarkably regular on this machine:

| font-size | English content line box | pin |
|---|---|---|
| 6px | 6px | 1.0 |
| 7px | 8px | 1.1428571 |
| 8px | 9px | 1.125 |
| 8px (a `<th>` with collapsed borders) | **8px** | **1.0** |
| 9px | 10px | 1.1111111 |
| 9.5px | 10px | 1.0526316 |
| 10px | 11px | 1.1 |
| 11px | 12px | 1.0909091 |
| 12px | 14px | 1.1666667 |
| 18px | 21px | 1.1666667 |

The `<th>` row is the exception that proves the point: it is 8px at 8px, not 9px,
and no ratio derived from a sibling would have got it right. **Measure each leaf.**

### M9. A pin the census CANNOT see, because the element is `display: none` at census time

O-Marimba's `.mts-label` reports `h=0, w=0` on all three arms in `measure-ui`,
because the MTS-ESP status row is hidden until MTS mode is entered. Only the
state-driven `check-ui-labels` run ever saw it grow, and it reported the growth as
`#mts-status dh=+3` — a container, not the leaf.

**The census is not a complete worklist.** Where `check-ui-labels` names a moved
container whose children did not move, look for a hidden-at-census leaf inside it
and derive the pin from the gate's own delta plus the leaf's font-size.

### M10. A body that names an EXEMPT value-mirror must keep it in Latin (W3, the other direction)

O-Marimba's `strike` body reads *"Center (0%) emphasizes the fundamental; edge
(100%) brings out higher partials"*. `Center` and `Edge` are `I18N_EXEMPT` VALUE
MIRRORS — the readout node shows those exact English words in every language — so
the Chinese body keeps them **verbatim**, wrapped in full-width parentheses:
`Center（0%）强调基频；Edge（100%）…`.

The mirror image is on the same plugin: `CUSTOM` in the `tuning-mode` and
`interval-list` bodies is **keyed** (`label.custom`), so the Chinese body must
name it by its localized caption instead. One body, two rules, opposite
directions. **Check `I18N_EXEMPT` membership for every option word in a body
before authoring, both ways** (W3, N8).

### M11. Z4's Latin/Han spacing consistency is TABLE-scoped, so pick the form once and hold it

`i18n-zh-lint`'s Z4 has two halves. The row-scoped half flags thin spaces. The
**table-scoped** half counts every Latin↔Han boundary in the whole file, and if
both the spaced and the unspaced form appear it flags every row using the
**minority** form. All five tables in this wave use the SPACED form throughout,
which is what the shipped corpus uses.

Practical consequence when authoring: `MIDI 力度`, not `MIDI力度`; `范围 0 到 1。`,
not `范围0到1。`. Full-width parentheses around a Latin run create no boundary at
all — `Center（0%）强调` is not a boundary because `（` is `Script=Common` — so
that shape is always safe.

### M12. The identity control catches READER faults, not just leaks, and it is the only thing that does

O-Formant's first blind pass returned **245 lines for a 245-row batch** — the
right count, looking complete — and the ids did not match the emitted order:

- one row was **dropped outright**;
- one came back with a **truncated 7-hex id and an empty English**.

Neither is a translation finding. Neither would have been visible from the
returned file's length, its Han count, or its well-formedness. The check that
found both is `diff <(cut -f1 emitted) <(cut -f1 returned)` — **ids identical AND
IN ORDER**, not merely the same set.

Both rows were re-read by a different model and spliced back at their emitted
positions before any triple was judged. **Run the order check before the ingest,
and treat a mismatch as a re-read rather than as a finding.**

### M13. Two DIFFERENT glossary roots can collide on a page, and no mechanical check can see it

O-ReverseDelay's Scatter and Diffusion knobs carry two **different** settled
glossary roots. The blind reverse read returned **"Spread" for both**.

Why nothing else catches it:

- Z5 is silent — each rendering IS the accepted root for its own English;
- the downstream mechanical check looks for two keys mapping to **one** string,
  and these map to two different strings;
- R3's screen fires only when both sides are glossary keys sharing a **root**, and
  these are different roots.

Only the reverse read saw it, by returning the same English word twice. The
resolution is the discriminator: **can a reader on this page confuse them?** Yes —
the two share a character and both mean spreading. **Only ONE side was qualified**,
and the reason is recorded at its entry: Scatter is the side ambiguous about WHAT
it scatters (the delay reach); Diffusion is not ambiguous about anything.

R3's "qualify both sides, never one" is written for a shared **root**. This is a
different shape, and the asymmetry is deliberate and documented rather than
sloppy. Wave 4e should expect it again wherever two near-synonym roots co-occur.

### M14. `auval -v` takes THREE arguments, and a quoted triple fails silently-looking

`auval -v "aumu OuFm OuDv"` prints `Invalid Arguments: auval` and nothing else —
no usage, no hint. Run across five plugins in a loop with a quoted variable it
reads exactly like five validation failures. Pass the three words unquoted:

```bash
auval -v aumu OuFm OuDv
```

Same family as C11's zsh word-splitting trap, met by a tool whose error message
does not say what is wrong.

---

## Deferred items

### D1. The shared `scala-tuning-engine` tuning panel — SIX consumers, corrected upward

Wave 4c's D1 names five consumers of
`modules/tuning/scala-tuning-engine/js/tuning-panel.js`. A CMake grep run at wave-4d
planning time returns **six**: `O-Bassoon`, `O-Bowed`, `O-Contrabass`, `O-Reed`,
`O-Wind` and **`O-MicrotonalSampler`**. The blast radius is larger than recorded;
nothing else about the deferral changes.

**This wave adds no consumer and touched nothing.** O-Formant embeds its OWN
1068-line copy — its header says so, it is 45 lines diverged from the module, and
O-Formant has no `dependencies.json` listing it — so localizing there reached no
other plugin. O-Marimba has no tuning-panel file at all.

Still open, still a separate task: the module has no i18n hooks in ANY language,
so a six-plugin change with a `/module-upgrade` revert risk is not something a
localization wave should carry.

### D2. O-Bells' two late tip bindings — the census control, unchanged

`boot-all-uis --strict-tips` reports **exactly 2 late across 1 plugin**:
O-Bells `#ref-pitch-knob` and `#octave-stretch`. Pinned by that gate's own
`EXPECTED_LATE` assertion. This wave used the pair as its boot census control and
it read 2 before and 2 after — which is what makes the 0 DEAD beside it evidence
rather than a number.

### D3. `js/app.js:899` on O-MultiBandCompressor — a build-stage marker on a user-visible surface

A parenthesised internal development-phase number is painted onto the spectrum
canvas, one line below the caption this wave localized. **Left exactly as found.**

It is not a localization defect — it is a build-stage string on a shipping
surface, and removing it is a product decision with no localization budget
attached. Recorded here for the developer, with the file and line. The canvas
caption above it now reads from the table; this one does not, deliberately, and
that split is documented at the site so the next reader does not conclude it was
forgotten.

### D4. The glossary divergence report — inherited from wave 4c's D3, plus one new pair

Wave 4c's D3 records `WebGL 不可用` against `不支持 WebGL` for near-identical
English on two plugins, invisible to Z5 because neither English is a `TERMS` key.

**New this wave, and a different shape (M13):** Scatter and Diffusion, two
different settled roots that a reader recovered as one English word. It is
resolved in the table by qualifying one side, but the *glossary* still carries two
roots whose Chinese shares a character. A future glossary pass might widen one of
them; that is a corpus-wide decision, not a per-plugin one.

### D5. The `Z6` budget backfill — inherited, unchanged

`i18n-zh-lint` prints it on every run: **3 of 552 glossary terms carry a measured
character budget; 549 are UNBUDGETED and Z6 is inert on them.** Disclosed by
design rather than silent. Stages 2–4 were meant to fill these from the
`check-ui-labels` zh arm; four waves in, three are filled. This wave added none,
because no caption in it was clipped — every geometry finding was a SHRINK
(see N9), and a shrink is a floor, not a budget.

### D6. Re-inherited from waves 4b and 4c

- wave 4c **D4**: O-Chorus's inert CJK tail; O-Gain's and O-IntonationPad's
  missing tip gates.
- wave 4c **D5** / this wave: `reviewed: 'native'` is OPEN on all **3073** rows.
  This project has no native Simplified Chinese reader. A disclosed quality level,
  printed by lint rule R1 on every run and stated in all five CHANGELOGs. **A
  blocker for nothing.**
- **The three plugins in this wave with no gate file** — O-MultiBandCompressor,
  O-Marimba, O-FreqPulse — took no invented gate, for the reason wave 4c's D4
  gives: a gate written mid-localization is a gate nobody has calibrated. The
  repo-wide instruments are the criterion for them. Same disposition for wave 4e's
  plugins that lack one.

---

## Explicitly for wave 4e

1. **Run the enumeration-form census on all five and say which forms are
   present.** Waves 4a–4c found four; this wave found the fifth, a per-language
   MAP LOOKUP (M1). Report whether a sixth appears. Remember that a Map read and a
   call-site walk both spell **no list to grep for** — only reading the file finds
   them.
2. **Check whether a gate has ANY path to the table before planning its repair
   (M2).** If it has none, the loader is the first edit and everything else waits
   on it. Copy the `vm` shape.
3. **Fire every precondition you assert, including "this gate passes" (M3).** One
   of wave 4d's was false and had been for two releases.
4. **Use the two-arm CMake reader unconditionally (M4)**, and report whether any
   wave-4e plugin holds its version in a variable or a quote.
5. **Re-derive the installed-family table (M5).** It is a property of the machine
   and the bare-generic arithmetic turns on it.
6. **Report the form-4 population per plugin AGAINST the `inherit` declarations
   already in the file (M6).** The pair of numbers is the finding; a grep cannot
   tell a repaired plugin from a partly-repaired one.
7. **Prefer an element-level or container-scoped form-4 rule to an id list (M7)**,
   and verify by re-running the screen and checking that the OTHER stack
   populations did not move.
8. **Derive every `line-height` pin from the CONTENT box, per line (M8, N10,
   N11).** Subtract padding and border first. A class rendered at N sizes needs N
   pins on N selectors — O-FreqPulse needed three for one element type.
9. **State the `line-height-normal` criterion as N1 gives it** — `check-ui-labels`
   0 moved, plus every residual named with its measured `enH == zhH`. It reached
   0 on none of this wave's five and that is correct.
10. **Budget for FLOORS, not clips (N9).** Every non-`line-height` pin in this wave
    was a floor because the Chinese is smaller — including a width floor set to
    the EXACT measured English box rather than rounded up, so neither Latin arm
    moves at all.
11. **Look for hidden-at-census leaves when a gate names a moved container whose
    children did not move (M9).**
12. **Check `I18N_EXEMPT` membership BOTH ways for every option word in a body
    (M10, W3, N8).** Exempt value mirrors stay Latin; keyed options take their
    localized caption. One body can need both.
13. **Hold one Latin/Han spacing form across the whole table (M11).** Z4's
    consistency half is table-scoped and flags the minority form.
14. **Check the returned ids are identical AND IN ORDER before ingesting (M12).**
    A reader can drop a row and return the right line count.
15. **Expect two DIFFERENT roots to collide (M13).** Only the reverse read sees it.
    Qualify the side that is ambiguous, and write down why the other was left.
16. **Pass `auval -v` three unquoted words (M14).**
17. **Re-inherit D1–D6 above**, with D1's consumer list at **six**.
18. **Report any prediction in the wave-4e plan that was FALSE as stated**, in the
    shape this wave's "Corrections to the plan's own measurements" section used.
    Wave 4d had three.
