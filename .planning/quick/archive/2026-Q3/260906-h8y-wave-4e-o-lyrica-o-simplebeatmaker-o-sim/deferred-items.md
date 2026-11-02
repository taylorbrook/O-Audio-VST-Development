# Wave 4e — deferred items and structural findings

Carry-forward for **wave 4f** (O-Wind, O-Contrabass, O-Reed, O-Bowed,
O-GrainScatter). Written in the shape waves 4a–4d used: numbered structural
findings first — each one costs a later executor time or a wrong answer if not
read — then the deferred items with their evidence, then the explicit
instruction list.

---

## Structural findings

### N1. `elementFromPoint` is the instrument, and it has TWO failure modes — only one of which is a miss

Wave 4e's task 2 found the first: a `{"click": …}` state whose target is covered
by another element. Playwright's `force: true` dispatches a real mouse event at
the point, the covering element receives it, the step **succeeds**, and
`--verbose` reports nothing. O-Lyrica's state 6 did nothing for the whole of its
life and ten authored captions shipped unmeasured behind a green gate.

Task 3 fired the same probe on all three of its plugins, on every `click`-form
state, before trusting any zero. Seven of eight states HIT. The eighth found the
**second failure mode, and it is not a miss**:

```
O-simpleBeatmaker state 5  [click [data-preset="Backbeat + Accents"]]
  rect = x 96, y 990, w 119, h 22        (in a 1060 x 900 frame)
  document.elementFromPoint(155, 1001) -> null
```

`null`, not a different element — the target is **below the fold** inside an
`overflow-y: auto` pane. Playwright scrolls it into view and the click lands, so
this is a false alarm for the coverage question. It was resolved by asserting the
state's **EFFECT** rather than its geometry: `#tourCaption` changed from
`label.tourCaption` to `label.tourLoaded`, so the state does fire.

Two things follow, and the second is the general one:

1. **A `null` from `elementFromPoint` is not a coverage hole.** Discriminate on
   the rect: if the centre is outside the viewport, the answer is "scrolled", not
   "covered". A *different element* is the defect; `null` is not.
2. **The real test is the state's EFFECT, not the click's landing point.**
   `elementFromPoint` is a cheap screen that catches the covered case; asserting
   that the panel the state opens actually opened catches both. Wave 4f should
   run the effect assertion, because it subsumes the geometry one.

A third consequence, recorded because it will bite: the scroll Playwright
performs **persists into every later state**, so a state file whose covering-fold
click is not last measures every subsequent state on a scrolled page. On
O-simpleBeatmaker the offending state is last and the `.frame` scroll of 147 px
harms nothing. It would not be harmless in the middle of a fifteen-state walk.

### N2. `undeclared-font` is NOT the form-4 census — confirmed a third time, and from the zero side

Tasks 1 and 2 recorded this from the non-zero side: the screen read 77 on a page
with 0 form-4 nodes and 177 on a page with 39. Task 3 confirms it from the other
direction. All three `O-simple*` plugins report `undeclared-font: 0` **before and
after** the table lands — and their genuine form-4 populations are also 0, which
looks like agreement and is a coincidence of two different measurements both
being empty.

The form-4 census is the `x.ff` grouping over `lang === 'en' && vis`, and on these
three it returns:

| plugin | stacks the page actually resolves | form-4 (UA face) | naked generic |
|---|---|---|---|
| O-simpleBeatmaker | 327 Garamond-stack, 12 `Menlo, Consolas, monospace`, 1 `--symbol-font`, 1 `Times` | **0** | **0** |
| O-simpleFM | 203 Garamond-stack, 5 `--symbol-font`, 1 `Times` | **0** | **0** |
| O-simplePhysicalModelSynth | 194 four-face Garamond, 6 three-face Garamond, 2 `--symbol-font`, 1 `Times` | **0** | **0** |

The single `Times` node on each is the `html` element on the UA face, which
renders no text. Zero-and-zero is a page with no instance of the class, not a page
nobody measured — wave 4c's C4 converse, now recorded for the fourth, fifth and
sixth time.

### N3. A `--symbol-font` token needs the CJK tail even when every glyph it renders is a symbol

Q5 measured `--symbol-font` as Latin-safe: `Apple Symbols` is installed and wins,
`Segoe UI Symbol` and `Noto Sans Symbols2` are not. The plan then asks whether any
of its 1 / 5 / 2 nodes can carry Han before deciding on a tail. Measured, the
answer is **yes, and not through its own text**:

```
O-simpleBeatmaker  #gear-btn            own "⚙"   han=false at census time
```

`measure-ui` computes `han` over the node's own text **plus `data-tip`,
`data-tip-title` and `aria-label`**. `#gear-btn` renders `⚙` and nothing else —
and the moment the table lands it carries a Chinese `data-tip-title` and a Chinese
`data-tip`, so the `undeclared-font` screen flags it. The token still ends at a
bare `serif`.

The tail was added on all three. It costs nothing: `Apple Symbols` stays first and
still wins for `⚙`, `❦` and `♪`, and the en/fr arms were re-measured unmoved.
**A pre-table census of `han` cannot answer this question** — that is Q11's shape
one level down, and the discriminator is whether the node is a TIP ANCHOR, not
what it renders.

### N4. The caption ROW is a defect class the per-leaf census structurally cannot see

Six containers across the three plugins grew 2–3 px in Chinese while **every leaf
inside them reported an identical box on both arms**:

| plugin | container | en | zh | leaves |
|---|---|---|---|---|
| O-simpleBeatmaker | `.grid-label` | 17 | 18.77 | all enH == zhH |
| O-simpleBeatmaker | `.lane-label`, `.midi-label`, `.tour-label` | 15 | 17 | all enH == zhH |
| O-simpleFM | `.viz-label`, `.keyboard-label` | 11 | 14 | all enH == zhH |
| O-simplePhysicalModelSynth | `.viz-label`, `.keyboard-label` | 11 | 14 | all enH == zhH |

The mechanism: `getBoundingClientRect()` on an **inline** element reports its font
box, not its line box, so a `<span>` whose face changed reports the same height.
The **line box** of the row is computed from the tallest inline box in it, and an
inline box's height is the used face's ascent + descent. PingFang SC's are taller
than Times New Roman's. So the row grows and no leaf reports it.

This is M9's shape one level up: M9 says look for a hidden leaf inside a moved
container. **N4 says the container may have no moved leaf at all**, and the pin
belongs on the ROW, at its own measured English line box.

Two mechanical notes for wave 4f:

- A **ratio** is the right form (`line-height: 1.25` at 12px = the row's own 15px),
  not a length. A length pinned to the measured row height inherits as a computed
  length and **grew the English arm 0.77 px** on `.grid-label`, because the row
  also holds a 17 px inline-block button whose box then dominated a strut that had
  been shorter. Measured, reverted, re-pinned as a ratio.
- Pin the rule that already owns the row, and check its selector list: on
  O-simpleBeatmaker `.grid-label, .lane-label, .midi-label` share one rule and all
  three needed the same 1.25.

### N5. The width-pinned knob caption is where an M13 qualification has to STOP

O-simpleFM's blind reverse read returned **"Exponent"** for every bare `指数` in
the table — a different physical quantity from the modulation index, and a wrong
thing for a user to learn. Fourteen rows were qualified to `调制指数`. **Two were
not**, and the reason is measured:

```
.knob-label has white-space: nowrap
  "Env→Index"  en 63.98px      包络→指数    51.34px      包络→调制指数  ~71px
  "Vel→Index"  en 62.92px      力度→指数    51.34px      力度→调制指数  ~71px
```

Qualifying them would put the Chinese caption **7 px wider than the English** in a
cell that cannot wrap. So the two arrow captions keep the short form.

**Round 2 vindicated the asymmetry, and that is the transferable part.** A
different model, on a fresh salt, returned `Env → Index` and `Vel → Index`
correctly for exactly those two rows while returning "modulation index" for every
prose row. An arrow caption standing beside the `调制指数` knob has an anchor a
sentence in a tooltip does not. **M13's "qualify the ambiguous side" has a
geometry ceiling, and the reverse read can tell you where the ceiling is
harmless.**

### N6. R8 was LIVE on all five of this wave's plugins, and the plan predicted it dead on four

The plan's Q14 table states `0 / 0 / 0 / 0 / 0` for O-Tapestop and all three
`O-simple*`, and Task 3's action text derives from it that "every pin you add is
the first on its selector and the R8 specificity grep comes back clean **by
construction**". Measured on the trees as found:

| plugin | min-width | min-height | line-height | letter-spacing | white-space | plan said |
|---|---|---|---|---|---|---|
| O-Tapestop | 1 | 0 | 5 | 21 | several | 0 / 0 / 0 / 0 |
| O-Lyrica | 29 CSS + 1 inline | 2 | 4 | 25 | 2 | 30 / 2 / 4 / 25 |
| **O-simpleBeatmaker** | **14** | 0 | **6** | **7** | **2** | 0 / 0 / 0 / 0 |
| **O-simpleFM** | **14** | 0 | **6** | **24** | **5** | 0 / 0 / 0 / 0 |
| **O-simplePhysicalModelSynth** | **11** | **1** | **4** | **19** | 0 | 0 / 0 / 0 / 0 |

R8 was therefore run PER SELECTOR on every plugin in the wave and the conclusion
survived on the evidence rather than by construction. The pre-existing
`line-height` owners were named and left untouched in every case
(`.gear-btn`, `.row-label`, `.cell-mark`, `.midi-readout`, `.knob-label`,
`.tooltip`, `.routing-label`, `.preset-nav`, `.confirm-message`).

**A clean grep on a file with no declarations and a clean grep on a file with
fourteen look identical in a terminal.** The plan says exactly this and then
asserts the first case for four plugins that are in the second. Wave 4f must
MEASURE the pin surface rather than inherit a table.

### N7. A shrink can be two-dimensional, and then one floor is not enough

Wave 4d's N9 says Chinese is shorter, so nearly every non-`line-height` pin is a
floor. O-simpleFM produced the case where the shrink is in BOTH axes at once:

```
p.subtitle   en 346.31 x 24 (TWO lines, it wraps)      zh 190.36 x 16 (ONE line)
```

`.title-block` shrink-wrapped **156 px narrower and 8 px shorter** and dragged the
whole preset bar 78 px left. It needs `min-width: 346.31px` AND `min-height: 24px`,
both at the exact measured English box. A width floor alone leaves the header 8 px
short; a height floor alone leaves the preset bar 78 px out of place.

The converse case is on O-simplePhysicalModelSynth and needs a **scoped** floor:
`Mode Bright` is the only caption on that page that wraps in English (20.88 px)
and not in Chinese (`模态亮度`, 10.44 px). `.knob-label` is shared by seventeen
captions, sixteen of which are one line in English, so a floor on the shared rule
would have grown all sixteen. The floor is scoped by the key the leaf renders:
`.knob-label[data-i18n="label.knobModeBright"]`.

### N8. `opacity: 0.38` is a coverage hole that reports as a coverage hole, in a line nobody reads as one

`check-ui-labels` prints, as a NOTE rather than a FAIL:

```
39 of 39 [data-i18n] elements were VISIBLE in at least one state
 3 never became visible and were therefore NEVER MEASURED
```

On O-simplePhysicalModelSynth the three were `String Model`, `Inharmonicity` and
`Mode Bright`. The latter two are not hidden — they carry `.pm-disabled`, which is
`opacity: 0.38, pointer-events: none, filter: grayscale(30%)`. They are rendered,
they are driven, and `measure-ui` measured the Mode Bright caption's cell
shrinking 10.4 px in Chinese — while the GATE never compared either of them across
languages, because it counts an element under full opacity as not visible.

A third state was added that switches the engine to Modal through the **real**
selector — `applyEngineGating` and `applyDiagramSkin` both fire from
`resonatorType`'s own change event — and the hole went **3 → 1**. The one that
remains is `String Model`, whose cell has carried the `hidden` attribute by design
since v1.2.2 (a reserved parameter for an engine that was never written), and is
structurally unmeasurable rather than unmeasured.

**Read that NOTE on every plugin in wave 4f**, and check the `.pm-disabled`-style
state classes specifically: a greyed-out control is a control a user can see.

### N9. The two-arm CMake reader is now the majority case, and no third shape has appeared

| plugin | shape | two-arm | one-arm |
|---|---|---|---|
| O-Tapestop | `VERSION 1.6.3` — unquoted literal | 1.7.0 | 1.7.0 |
| O-Lyrica | `VERSION "2.4.4"` — quoted literal | 2.5.0 | 2.5.0 |
| O-simpleBeatmaker | `set(OSIMPLEBEATMAKER_VERSION "1.2.1")` | 1.3.0 | **EMPTY** |
| O-simpleFM | `set(OSIMPLEFM_VERSION "1.4.1")` | 1.5.0 | **EMPTY** |
| O-simplePhysicalModelSynth | `set(OSIMPLEPHYSICALMODELSYNTH_VERSION "1.2.3")` | 1.3.0 | **EMPTY** |

Three of five, up from wave 4d's one of five. Fired again after the bumps: the
one-arm form still returns EMPTY on all three. **No third shape.**

### N10. M12 earns its keep again, and this time the line count DID differ

Wave 4d's M12 records a batch that returned the right line count with a dropped
row. This wave's O-simpleFM chunk 2 returned **55 lines for a 56-row batch** — a
count a careless reader could have caught — but only the ORDER diff named which
row:

```
diff <(cut -f1 emitted) <(cut -f1 returned)
26d25
< 606a9579ab75            # label.subtitle
```

Re-read on its own by a DIFFERENT model and spliced back at its emitted position
before any triple was judged. Five other chunks and both round-2 chunks were
identical and in order. **Six blind chunks, one reader fault** — the rate is not
low enough to skip the check.

### N11. The installed-family list, re-derived on this machine (M5)

`system_profiler SPFontsDataType | grep -c "Family: X$"`, unchanged from wave 4d
and from this plan's Q5:

| family | matches | | family | matches |
|---|---|---|---|---|
| Georgia | 4 | | **Garamond** | **0** |
| Times New Roman | 4 | | **EB Garamond** | **0** |
| Arial | 4 | | **Adobe Garamond Pro** | **0** |
| Menlo | 4 | | **Microsoft YaHei** | **0** |
| Courier New | 4 | | **Consolas** | **0** |
| PingFang SC | 6 | | **Segoe UI Symbol** | **0** |
| Songti SC | 4 | | **Noto Sans Symbols2** | **0** |
| Helvetica Neue | 14 | | Apple Symbols | 1 |
| Arial Unicode MS | 1 | | | |

The arithmetic inverted again relative to wave 4d, in the direction Q5 predicted:
**every** Garamond stack in this wave names `'Times New Roman'` after it and TNR is
installed, so the Latin half was already safe everywhere and the whole wave took
**tail-only** work except for O-Lyrica's seven naked generics and its 39 form-4
controls. Re-derive it at the start of wave 4f: it is a property of the machine.

### N12. The enumeration-form census for wave 4e: forms 2, 3 and 5, and NO sixth

Measured on the wave's only gate file, `plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js`
(task 1). Form 1 (an assertion on `LANGUAGES.join(',')`) and form 4 (no language
reference at all) are absent — form 4 in the trivial sense that the other four
plugins have no gate FILE, which is an absence of a file, not of a reference, and
the two must not be conflated in the report.

Three sites that LOOK like form 5 and are not were classified explicitly and LEFT:
`__setLanguage('en')` at raw L671 is a deliberate reset that keeps the stress
stage's numbers comparable with every prior release, and L673/L674 read the
language it was just reset to.

**Wave 4f is the last wave with gate files.** After O-Tapestop's repair, exactly
**three** two-language gate files remain in the repo — `O-Bowed`, `O-Reed` and
`O-Wind`, all `ui_tip_render_check.js`, all in wave 4f. Say whether a sixth form
appears in them.

### N13. Five settings bodies read, five left — the first wave in the rollout with no P6 defect at all

Q8's prediction held on all five. Each names both of its panel's controls and is
already true, and each was read in full and recorded as a checked non-defect
citing the entry (`'settings'` on O-Lyrica and O-Tapestop, `'gear-btn'` on the
three `O-simple*`). The stale LANGUAGE body, by contrast, was present on all five,
in both en and fr, and all ten clauses were deleted.

Five waves of mechanical deletion have made deletion the reflex. Here the reflex
would have stripped five true sentences. **A diff cannot tell a body that was
examined and left from one that was never opened** — only the record can.

---

## Deferred items

### D1. The shared `scala-tuning-engine` tuning panel — SIX consumers, and WAVE 4F CONTAINS THREE OF THEM

`modules/tuning/scala-tuning-engine/js/tuning-panel.js` has no i18n hooks in ANY
language. The CMake grep returns exactly six consumers: **O-Bassoon, O-Bowed,
O-Contrabass, O-MicrotonalSampler, O-Reed, O-Wind.**

**This wave added no consumer and touched nothing** — verified two ways before the
work (Q12) and re-confirmed after it: the CMake grep returns 0 for each of the
five, and a `find` for `tuning-panel.js` under each returns 0. Notably O-Lyrica,
a physical-modeling harp with a full TUNING tab — a scale library, five
visualisations and a rank-2/EDO/harmonic-series generator, the plugin one would
most expect to be a consumer — **is not one, and has no private copy either.**

**Wave 4f is where this stops being deferrable.** Three of its five plugins —
**O-Wind, O-Reed and O-Bowed** — are consumers, and a fourth wave of "verified,
untouched" would ship three plugins whose tuning tab is English in a Chinese
interface. It needs a DECISION BEFORE wave 4f is planned, not during it. The
options, stated so the decision is cheap:

1. localize the module once, as its own task, with a `/module-upgrade` across all
   six consumers and the revert risk that carries;
2. localize it inside wave 4f and accept that the wave reaches three plugins it
   did not plan to touch;
3. ship wave 4f with an English tuning panel and record it as a known gap.

None of the three is free. **Choosing at plan time is much cheaper than choosing
at execute time**, which is what this entry exists to say.

### D2. O-Bells' two late tip bindings — the census control, unchanged

`boot-all-uis --strict-tips` reports **exactly 2 late across 1 plugin**: O-Bells
`#ref-pitch-knob` and `#octave-stretch`, pinned by that gate's own `EXPECTED_LATE`
assertion. This wave used the pair as its boot census control and it read 2 before
and 2 after, which is what makes the 0 DEAD beside it evidence rather than a
number.

### D3. `js/app.js:899` on O-MultiBandCompressor — a build-stage marker on a user-visible surface

Inherited unchanged from wave 4d. A parenthesised internal development-phase
number is painted onto the spectrum canvas, one line below a caption that reads
from the table. Not a localization defect — a build-stage string on a shipping
surface, and removing it is a product decision with no localization budget
attached. Recorded for the developer, with the file and line.

### D4. The glossary divergence report — SIX entries open, three of them from this wave

1. **`WebGL 不可用` vs `不支持 WebGL`** (wave 4c D3) — near-identical English on
   two plugins, invisible to Z5 because neither English is a `TERMS` key.
2. **Scatter and Diffusion** (wave 4d M13) — two different settled roots that a
   reader recovered as one English word, resolved in the table by qualifying one
   side. The *glossary* still carries two roots whose Chinese shares a character.
3. **`通过长度` for `pass length`** (wave 4e task 1) — read back as
   "By Length Division"; the reader parsed 通过's first character as the
   preposition *by/through* rather than the noun *pass*. It is the settled root,
   nothing on O-Tapestop's page collides with it, and diverging in one plugin
   unilaterally is what this report exists to prevent.
4. **`分割` for `Divisions`** (wave 4e task 2) — read back as "Split" on one model
   and "Divide" on another. Two independent readers agree the settled root reads
   as an action.
5. **`键位` for `keyswitch`** (wave 4e task 2) — reads as "keymap" / "key position"
   on a plugin that also loads and saves `.kbm` keyboard-mapping files. They sit
   on different tabs so they do not co-occur, and the bodies disambiguate.
6. **NEW, wave 4e task 3: `调制` for `MOD`** — the settled root for `mod`, used as
   the face of a signal-path diagram node. Both readers, on two different models,
   returned **"Modulation"** — the process — where the English abbreviation `MOD`
   names the **operator**. It is left as it stands (nothing else on O-simpleFM's
   page can be confused with it, the tooltip beneath names the operator in a full
   sentence, and `调制器` measures 29.6 px against `MOD`'s 23.08 px inside a fixed
   SVG operator box), but the glossary carries one root for a word English uses
   in two grammatical roles. **A future glossary pass should decide whether `mod`
   deserves a second accepted form for the operator sense.**

Every one of the six is a corpus-wide decision, not a per-plugin one.

### D5. The `Z6` budget backfill — inherited, unchanged

`i18n-zh-lint` prints it on every run: **3 of 552 glossary terms carry a measured
character budget; 549 are UNBUDGETED and Z6 is inert on them.** Disclosed by
design rather than silent. Five waves in, three are filled. **This wave added
none**, and the reason is the same as wave 4d's: every geometry finding was a
SHRINK, and a shrink wants a floor, not a budget. The two English-wraps-Chinese-
does-not cases (O-simpleFM's strapline, O-simplePhysicalModelSynth's Mode Bright)
are the same shape inverted and also want floors.

### D6. Re-inherited from waves 4b, 4c and 4d

- wave 4c **D4**: O-Chorus's inert CJK tail; O-Gain's and O-IntonationPad's
  missing tip gates.
- wave 4c **D5** / every wave since: `reviewed: 'native'` is OPEN on all **3724**
  rows. This project has no native Simplified Chinese reader. A disclosed quality
  level, printed by lint rule R1 on every run and stated in all five CHANGELOGs.
  **A blocker for nothing.**
- **The four plugins in this wave with no gate file** — O-Lyrica,
  O-simpleBeatmaker, O-simpleFM, O-simplePhysicalModelSynth — took no invented
  gate, for the reason wave 4c's D4 gives: a gate written mid-localization is a
  gate nobody has calibrated. `check-i18n`, `check-ui-labels`, `measure-ui` and
  `boot-all-uis` are the whole instrument for them. Same disposition for any
  wave-4f plugin that lacks one.
- The three `tests/render-harness/` directories under the `O-simple*` plugins are
  the **C++ Stage-2 DSP** harness, not UI gates — confirmed by reading their
  `CMakeLists.txt` and `main.cpp` rather than assuming it — and were not touched.

### D7. NEW — a pre-existing French wrap on O-simpleFM

`measure-ui --report wrap-count` reports `div.routing-label` "Signal Path" at
**en = 1 line, fr = 2 lines**. It was measured on the tree AS FOUND, before any of
this wave's work, and nothing in this release touches it. Recorded here rather
than fixed, because it is a French geometry question with no Chinese content and
no localization budget attached; the Chinese arm of the same node is one line.

---

## Explicitly for wave 4f

1. **DECIDE the `scala-tuning-engine` question BEFORE planning (D1).** Three of
   wave 4f's five plugins — O-Wind, O-Reed, O-Bowed — are consumers of a shared
   panel with no i18n hooks in any language. This is the first wave that cannot
   verify its way past it. The three options are written out in D1; pick one at
   plan time.
2. **Run the enumeration-form census on wave 4f's THREE remaining gate files
   (N12).** They are `O-Bowed`, `O-Reed` and `O-Wind`, all `ui_tip_render_check.js`
   — the last two-language gates in the repo. Waves 4a–4c found four forms, 4d
   found the fifth (a per-language map lookup), 4e found no sixth. Say whether one
   appears, and classify every reset-scoped look-alike explicitly rather than
   generalizing it.
3. **Fire every precondition you assert, including "this gate passes" (M3).**
   Wave 4d had one that was false and had been for two releases; wave 4e fired all
   of its own and every one was green.
4. **Use the two-arm CMake reader unconditionally (N9)**, and report whether any
   wave-4f plugin holds its version in a variable or a quote. This wave: three of
   five in a quoted variable, no third shape.
5. **Re-derive the installed-family table (N11).** It is a property of the machine
   and the bare-generic arithmetic turns on it — this wave's Latin half was
   already safe almost everywhere because every Garamond stack named TNR, which
   inverted wave 4d's worklist entirely.
6. **Report the form-4 population per plugin AGAINST the `inherit` declarations
   already in the file (N2, M6).** The pair of numbers is the finding. Wave 4e
   produced the controlled pair: O-Lyrica 5 declarations / 39 controls still on
   the UA face against O-simpleBeatmaker 6 / 0. Note the correction — the plan
   said 5 `inherit` declarations on O-simpleBeatmaker and O-simpleFM and both
   carry **6**.
7. **MEASURE the existing pin surface; never inherit a table (N6).** R8 was live
   on all five of this wave's plugins and the plan predicted it dead on four.
8. **Pin caption ROWS as well as leaves (N4).** Six containers in this wave grew
   2–3 px with every leaf inside them reporting an identical box. Use a RATIO, not
   a length: a length grew the English arm 0.77 px on a row that also held an
   inline-block button.
9. **State the `line-height-normal` criterion as N1 gives it** — `check-ui-labels`
   0 moved, plus every residual named with its measured `enH == zhH`. It reached 0
   on none of this wave's five and that is correct.
10. **Budget for FLOORS, and expect a shrink in BOTH axes (N7).** O-simpleFM's
    strapline needed `min-width` AND `min-height`; O-simplePhysicalModelSynth's
    Mode Bright needed a floor scoped by key, because sixteen sibling captions
    share its rule and are one line in English.
11. **Read the `never became visible` NOTE and check the state classes (N8).** An
    `opacity: 0.38` disabled control is rendered, driven, and never compared.
12. **Fire the state-EFFECT assertion on every state, not just
    `elementFromPoint` (N1).** The geometry probe has two failure modes and only
    one of them is a defect; the effect assertion subsumes both. Watch for a
    scrolling click that is not the last state.
13. **Check `I18N_EXEMPT` membership BOTH ways for every option word in a body
    (M10, W3, N8).** O-simplePhysicalModelSynth needed both directions in one
    body: `Bow` stays Latin because it is an `AudioParameterChoice` entry, and
    `Excitation` beside it becomes `激励` because the group heading is keyed.
14. **Hold one Latin/Han spacing form across the whole table (M11), and expect W5
    to fire.** O-simpleBeatmaker's `grid` body produced exactly one Z8: a space
    that was correct while the token to its right was Latin became an intra-Han
    space the moment `循环` abutted `普通`.
15. **Check the returned ids are identical AND IN ORDER before ingesting (M12,
    N10).** Six blind chunks this wave, one reader fault.
16. **Expect two DIFFERENT roots to collide, and know where qualification STOPS
    (M13, N5).** A `white-space: nowrap` caption cell is a hard ceiling; the
    reverse read can tell you whether stopping there is safe.
17. **Pass `auval -v` three unquoted words (M14).**
18. **Re-inherit D1–D7 above**, with D1's consumer list at six and the decision
    now due.
19. **Report any prediction in the wave-4f plan that was FALSE as stated**, in the
    shape this wave's "Corrections to the plan's own measurements" section used.
    Wave 4d had three; wave 4e had **fourteen** across its three tasks.
