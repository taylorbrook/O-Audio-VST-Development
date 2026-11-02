---
phase: quick-260906-h8y
plan: 01
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4e, O-Tapestop, O-Lyrica, O-simpleBeatmaker, O-simpleFM, O-simplePhysicalModelSynth, gates, fonts, geometry, line-box, cmake-variable, coverage-hole]
status: complete
requires:
  - scripts/i18n-zh-glossary.js
  - scripts/i18n-zh-lint.js (a GATE since 260905-acr — exit 2 on any finding)
  - scripts/i18n-zh-backtranslate.js
  - scripts/measure-ui.js (--report all, promoted in 260905-izp)
  - scripts/check-i18n.js, scripts/check-ui-labels.js, scripts/boot-all-uis.js
provides:
  - five three-language plugins (en / fr / zh-Hans) at reviewed:'bt'
  - a clamp gate that derives its language list, aborts rather than guessing, and COMPARES every non-English language where it compared two
  - the caption-ROW line-box pin — a defect class no per-leaf census can see
  - document.elementFromPoint as a state-coverage instrument, with both of its failure modes classified
  - the two-arm CMake version reader proven on the MAJORITY case (three of five)
affects:
  - PLUGINS.md (five rows), the zh corpus (3073 -> 3724 rows, 27 -> 32 plugins)
tech-stack:
  added: []
  patterns:
    - "a caption ROW pinned at its own measured English LINE BOX, as a ratio, because every leaf inside it reports enH == zhH"
    - "elementFromPoint on every click-form state, discriminating a covered target (a defect) from an off-viewport one (a scroll)"
    - "an M13 qualification that STOPS at a white-space:nowrap cell, with round 2 used to prove stopping there is safe"
    - "a min-* floor scoped by the i18n key the leaf renders, where the shared rule reaches sixteen siblings that must not move"
    - "a CJK tail on a SYMBOL font token, because the node is a TIP ANCHOR even though its own glyph is not Han"
key-files:
  created: []
  modified:
    - plugins/O-Tapestop/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/styles.css,Source/PluginProcessor.h,tests/ui_tooltip_clamp_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-Lyrica/{Resources/ui/js/i18n.js,Resources/ui/index.html,Source/PluginProcessor.h,tests/i18n-states.json,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-simpleBeatmaker/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/styles.css,Source/PluginProcessor.h,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-simpleFM/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/styles.css,Source/PluginProcessor.h,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-simplePhysicalModelSynth/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/styles.css,Source/PluginProcessor.h,tests/i18n-states.json,CMakeLists.txt,CHANGELOG.md}
    - PLUGINS.md
decisions:
  - "O-Tapestop's clamp gate: LANGUAGES added to the vm loader's published set, the list DERIVED with an abort, and the three ENUMERATING map lookups generalized — while the three RESET-scoped English reads are deliberately preserved and classified at each site"
  - "O-Lyrica's form-4 rule omits `select`, because the page already carries an equal-specificity element-level select rule and including it would have moved the en and fr arms"
  - "O-Lyrica state 6 changed from a forced click to an eval, after elementFromPoint proved the click was landing on the footer keyboard and ten authored captions were shipping unmeasured behind a green gate"
  - "O-simplePhysicalModelSynth gains a THIRD state that switches the engine to Modal through the real selector, taking its never-measured caption count from three to one"
  - "O-simpleFM's 指数 qualified to 调制指数 in fourteen prose rows and LEFT in the two nowrap arrow captions; round 2 on a different model confirmed the asymmetry is safe"
  - "reviewed: 'native' stays OPEN on all 3724 rows — a disclosed quality level, a blocker for nothing"
metrics:
  duration: "~7h across three executors"
  completed: 2026-09-06
actuals:
  tokens: 302000
  tasks: 3
  commits: 7
---

# Wave 4e — O-Tapestop 1.7.0, O-Lyrica 2.5.0, O-simpleBeatmaker 1.3.0, O-simpleFM 1.5.0, O-simplePhysicalModelSynth 1.3.0

Five plugins ship English, French and Simplified Chinese at `reviewed: 'bt'`.
**651 new rows**; the corpus goes **3073 → 3724 of 5256**, `BELOW SHIP BAR 0`,
across **27 → 32** localized plugins. Every row was authored at `'mt'`,
committed, read back through an independent blind reverse pass with the English
withheld, and promoted only afterwards.

## Shipped versions

| plugin | version | type | AU triple (read off `auval -a`) | rows | `auval -v` |
|---|---|---|---|---|---|
| O-Tapestop | **1.7.0** | effect (`IS_SYNTH` absent — the wave's only one) | `aufx OTsp OuDv` | 114 | **AU VALIDATION SUCCEEDED** |
| O-Lyrica | **2.5.0** | instrument | `aumu OLyr OuDv` | 213 | **FAILED — one pre-existing, documented assertion; see below** |
| O-simpleBeatmaker | **1.3.0** | instrument | `aumu OSiB OuDv` | 111 | **AU VALIDATION SUCCEEDED** |
| O-simpleFM | **1.5.0** | instrument | `aumu OSiF OuDv` | 112 | **AU VALIDATION SUCCEEDED** |
| O-simplePhysicalModelSynth | **1.3.0** | instrument | `aumu OsPM OuDv` | 101 | **AU VALIDATION SUCCEEDED** |

One cold registry rescan for the whole sweep. Every triple was read off
`auval -a` rather than guessed, and every `auval -v` was passed **three unquoted
words** (M14). All five installed as `-dev` only; **no alternate-variant orphan
on disk** for any of them, and every installed `CFBundleShortVersionString`
matches its shipped version in both VST3 and AU.

### The one auval failure, and why it is not this wave's

`auval -v aumu OLyr OuDv` reports **nine PASS sections and one FAIL**, on a
single error:

```
ParameterID=1275870432, Scope=0, Element=0: Saved Value = 0.337891, Current Value 0.000000
ERROR: Parameter values are different since last set - probable cause: a Meta Param
       Flag is NOT set on a parameter that will change values of other parameters.
```

Parameter `1275870432` is **`Free Glissando` (`freeToggle`)**, and O-Lyrica's own
CHANGELOG has recorded this since v2.4.x:

> `auval` reports a static "Meta Param Flag" warning caused by the intentional
> `freeToggle`/`scaleToggle` mutual exclusion (v1.30.0) — pre-existing and
> benign; all render/MIDI/parameter tests pass. Not introduced by this release.

**Proved, not assumed.** `git diff 56537bce..HEAD --stat -- 'plugins/*/Source/*.cpp'`
is **empty**: this wave changed no `.cpp` file at all, on any of the five.
O-Lyrica's entire C++ delta is two lines in a header — the `languageCode` /
`languageIndex` ternary. The `freeToggle` parameter and the mutual-exclusion
logic that trips this assertion are untouched.

**This contradicts the plan's own human-check**, which asserts "All five report
AU VALIDATION SUCCEEDED" — an M3-shaped assertion nobody had fired. See the
corrections section.

## Corpus

| | before | after |
|---|---|---|
| zh-Hans rows | 3073 of 5256 | **3724 of 5256** |
| plugins carrying Chinese | 27 | **32** |
| `BELOW SHIP BAR` (rows at `'mt'` or unflagged) | 0 | **0** |
| `i18n-zh-lint` entries checked repo-wide | 2312 | **2804** |

Row counts read off the emitter's own `rows` column (R1), never estimated. The
plan's 114 / 213 / 111 / 112 / 101 held exactly, against a hand-off estimate of
494 that was **157 rows low** — more than a whole plugin.

---

## Gate results — repo-wide, after everything

| gate | as found | shipped |
|---|---|---|
| `i18n-zh-lint --self-test` | **10/10** | **10/10** |
| `check-i18n` repo-wide | exit 0, 43 plugins | **exit 0, 43 plugins** |
| `i18n-zh-lint` repo-wide (**a GATE — exit 2 on any finding**) | exit 0, 0 findings | **exit 0, 0 findings across 43, 2804 entries** |
| `i18n-fr-lint` | exit 0 | **exit 0** — French untouched by the whole wave |
| `boot-all-uis --strict-tips` | 0 dead, 0 failed, 0 warn, exactly 2 late | **0 dead, 0 failed, 0 warn, exactly 2 late across 1 plugin** — O-Bells' pinned pair, the D2 census control, unchanged |
| `i18n-zh-backtranslate` stage view | 3073 of 5256, `BELOW SHIP BAR 0` | **3724 of 5256, `BELOW SHIP BAR 0`, 32 plugins** |
| `check-ui-labels --plugin <N>` | exit 0 on all five | **exit 0 on all five — 0 moved on en, fr AND zh, every state** |
| `measure-ui --verbose` | — | **0 unresolved states on all five** |
| `plugins/O-Tapestop/tests/ui_tooltip_clamp_check.js` | **exit 0, FIRED not assumed** | **exit 0, `== ALL CHECKS PASSED ==`, with the real zh arm swept AND compared** |
| `PLUGINS.md` duplicate check | — | **prints nothing** |
| cmake-vs-registry | **MISMATCH 5 of 5** (every row one patch behind) | **AGREE 5 of 5** |
| alternate-variant orphans on disk | 0 | **0** |
| `git status --short -- modules/` | 0 | **0** |
| `git tag --points-at HEAD` | 0 | **0** |
| dispatch sentinel | modified, unstaged | **modified, unstaged — never staged** |

---

## measure-ui, all five, at each plugin's own parsed shipping frame

`--report all`, cumulative over each plugin's states, with the identity
disclosure so every zero below is measured rather than vacuous.

| plugin | frame | identity (nodes / DOM keys / display ids) | undeclared-font | wrap-count | svg-font-attr | line-height-normal | of which MOVED | visible Han-bearing nodes | on no CJK face | on a naked generic |
|---|---|---|---|---|---|---|---|---|---|---|
| O-Tapestop | 860 × 580 | 630 / 210 / 120 | **0** | 2 † | **0** | 4 | **0** | 77 | **0** | **0** |
| O-Lyrica | 700 × 450 | 3018 / 1006 / 284 | **0** | **0** | **0** | 25 | **0** | 181 | **0** | **0** |
| O-simpleBeatmaker | 1060 × 900 | 1356 / 452 / 167 | **0** | **0** | **0** | 6 | **0** | 213 | **0** | **0** |
| O-simpleFM | 760 × 980 | 669 / 223 / 118 | **0** | 1 ‡ | **0** | 11 | **0** | 90 | **0** | **0** |
| O-simplePhysicalModelSynth | 1040 × 860 | 758 / 253 / 109 | **0** | **0** | **0** | 12 | **0** | 86 | **0** | **0** |

† O-Tapestop's two are a pre-existing French wrap on `label.envHint1` and a
Chinese *shrink* on `label.envHint2`, both inside the fixed-height `.env-plate`.
‡ O-simpleFM's one is `div.routing-label` "Signal Path", **en = 1 line,
fr = 2 lines** — measured on the tree AS FOUND before any of this work and
untouched by it. Both are evidenced non-movers; see the deviations.

**`line-height-normal` is not required to reach 0, and reached it on none of the
five — which is correct** (N1). The screen computes `han` over the node's own
text **plus `data-tip`, `data-tip-title` and `aria-label`**, so a numeric readout
carrying a Chinese accessible name is a finding forever: it has no Han to render,
its box is identical on both arms, and no pin can change that. **The
load-bearing criterion is `check-ui-labels` reporting 0 moved, and it does on all
three arms of all five.** Every residual is a measured non-mover; the classes:

| plugin | residuals | what they are |
|---|---|---|
| O-Tapestop | 4 | `#help-toggle` 21 == 21, `#preset-name` 30 == 30 (renders the `I18N_EXEMPT` Latin `Default`), `#engage-btn` 58 == 58, `#envCanvas` (a `<canvas>`, no line box at all) |
| O-Lyrica | 25 | 15 numeric readouts at 9px 10 == 10, the 4 tabs at 10px 29 == 29, 4 glyph buttons, `#preset-name-display` 15 == 15 |
| O-simpleBeatmaker | 6 | the six `button.tour-btn` lesson faces at 11.5px 22 == 22 — `I18N_EXEMPT` factory preset names that render no Han and reach the screen only through their Chinese `data-tip` |
| O-simpleFM | 11 | `#presetName` `Default` 24 == 24, the two 10px preset-button spans 11 == 11, `#help-toggle` 24 == 24, the two SVG operator faces 10 == 10, the five `button.tour-btn` preset faces 23 == 23 |
| O-simplePhysicalModelSynth | 12 | `#presetName` 24 == 24, two 10px preset spans, `#help-toggle` 24 == 24, the four SVG diagram boxes (`<rect>`, no own text) and their four `<text>` faces at 8–11 == 8–11 |

---

## Per-plugin work

### O-Tapestop 1.7.0 — the tracer (task 1)

114 rows. The wave's only **effect**, its only **gate file**, and its simplest
page: one `--serif` token reaching all 13 declarations, **zero** form-4 nodes,
**zero** naked generics.

**The gate repair is the task.** `tests/ui_tooltip_clamp_check.js` already
required `vm`, already read `js/i18n.js` and already ran it in a context — and
published `{ I18N, TIP_BINDINGS }` while referring to `LANGUAGES` **zero times**.
Measured as found: `grep -c LANGUAGES` = 0, `grep -c vm` = 1, and only the first
number decides whether the loader work is needed. `LANGUAGES` joined the
published set; the list is derived, its shape asserted, and an unreadable list
**ABORTS**. Six enumeration sites classified by raw line number: the two-element
literal at L455 derived, both walks (L459, L654) free, and the **three
enumerating map lookups at L639/L640/L661 generalized** — including the
copy-differs hard-fail at L645, which left alone would have compared exactly two
languages while the Chinese page was driven, measured and never compared. The
three **reset-scoped** English reads at L671/L673/L674 are deliberately preserved
with the classification recorded at each site, because generalizing them would
have made the stress stage's numbers incomparable with every prior release.

**The derive-or-abort control fired against the WALK**, not merely against the
guard: a planted empty `LANGUAGES` export produced
`ABORT: LANGUAGES is unreadable … Refusing to sweep a guessed language list.`,
exit 1, and **zero languages swept**. Reverted by targeted edit, never
`git checkout --`; `shasum -a 256 -c` reported byte-identical to the pre-plant
file.

Nine `line-height` pins. Two rows re-authored (`label.modeStop` 停止 → **停转**
for an M13 page collision with 启动, plus its own tip title; `aria.helpToggle`
切换悬停帮助 → **开关悬停帮助** on a Z5 finding). One deferral to D4
(`通过长度` for `pass length`).

### O-Lyrica 2.5.0 — the heavy plugin (task 2)

213 rows, 15 states, a 4874-line `index.html`, the wave's only tight frame at
700 × 450, and **the largest font surface in the whole rollout**.

**The form-4 / `inherit` controlled pair, from the O-Lyrica side: 5 declarations
already present, 39 controls still on the UA face.** A grep of this file reports
a plugin that has had the repair. The computed census reports one that has had it
on five controls out of forty-four.

Three separable halves of font work: the **tail** (11 Garamond stacks + 1
Georgia, Latin already safe), the **face-naming** half (**all seven naked
`monospace` declarations, 157 nodes — thirteen times the previous worst**, and
the defect is invisible from the Chinese copy because they are digit readouts),
and the **form-4** half (`button, input, textarea` at specificity (0,0,1), with
`select` **deliberately omitted** because the page carries its own equal-
specificity `select` rule and including it would have pulled every dropdown off
its serif stack and moved the en and fr arms).

Twenty-five `line-height` pins, each from its own measured English content box,
across six ratios including the `<th>` collapsed-border exception at 7px → 1.0.
R8 live and run per selector against 29 CSS `min-width` floors placed for French;
none of the 25 already declared a `line-height` and no French floor was lowered.

**The coverage hole is the finding of the wave.** `tests/i18n-states.json`
state 6 read `{"click": ".generator-header"}`. Measured with
`document.elementFromPoint` at the target's own centre, the topmost element was
`<div class="white-key mapped" data-note="71">` — the footer keyboard paints over
the generator header, Playwright's `force: true` dispatched a real mouse event to
the keyboard key, and `window.toggleGenerator` never fired. **The step did not
throw, `--verbose` logged no skip, and `measure-ui --verbose` reported 0
unresolved states.** The whole scale-generator form — ten captions authored in
that task — went unmeasured on every arm. Changing the step to
`{"eval": "window.toggleGenerator();"}` closed it (6 → 3 never visible, the
remaining three being `<option>` elements inside a closed `<select>`) and **turned
a green gate red — 12 FAILED** — which two further pins closed.

One re-author for terminology (`label.genGenerator` 生成音程 → **生成元**, an
M13 collision with the 生成 button directly below it) and one for **geometry**
(`label.voices` 复音数 → **声部**, because the longer of two settled roots
overran a 37 px French-era floor). Two deferrals to D4 (分割, 键位).

### O-simpleBeatmaker 1.3.0 (task 3)

111 rows, 5 states, 1060 × 900. **Zero form-4 nodes against SIX `font-family:
inherit` declarations** — the controlled counter-case to O-Lyrica's 5-that-do-not.
Zero naked generics. Four font sites take a CJK tail: the page serif, the two
`'Menlo','Consolas',monospace` declarations (Menlo installed, Consolas not) and
`--symbol-font`.

**Sixteen `line-height` pins and a seventeenth on a caption row**, derived per
leaf and per size:

| ratio | font-size | English content box | selectors |
|---|---|---|---|
| 1.047619 | 10.5px | 11px | `.tr-key-tempo` `.tr-key-length` `.tr-unit-steps` `.clear-btn` `.toggle-btn` |
| 1.0657143 | 10.5px | 11.19px | `.lane-legend` — its four `.lk` spans are FLEX ITEMS, so each is block-level and its box IS its line box |
| 1.0909091 | 11px | 12px | `.tour-caption` |
| 1.0434783 | 11.5px | 12px | `.settings-label` `.vs-name` |
| 1.1 | 10px | 11px | `.settings-toggle` |
| 1.25 | 12px | 15px | `.tr-state #readTransport` — 15 rather than M8's 14, because its leading bullet comes from `--symbol-font` and Apple Symbols sets a taller line box than the page serif |
| 1.25 | 12px | 15px | `.grid-label, .lane-label, .midi-label` and `.tour-label` — the four caption ROWS |
| 1.2 | 12.5px | 15px | `.subtitle` `.group-title` |

Two floors, both at the exact measured English box: a **width** floor of 37.5 px
on the tempo transport key (速度 is 22.47 px and un-floored it narrowed the strip
15 px and widened the title block 8.8 px), and the copy lever for the state cell
— `label.freeRun` shortened to `● 自由` because `● 自由运行` measures 59.3 px
against `#readTransport`'s 53 px French-era floor.

`label.gridHintKbd` keeps **`Del`** with a `termNote`: it is a keycap, decided by
the hardware, not the Delete action the glossary root 删除 names.

**Nothing was re-authored after the reverse read**, so there is no round 2.

### O-simpleFM 1.5.0 (task 3)

112 rows, 4 states, 760 × 980. **Zero form-4 nodes against SIX `inherit`
declarations**, zero naked generics, two font sites take a tail.

Six strings took their settled glossary root over the first draft; four Z1
findings closed by unspacing `M : C` into the masked Latin token `M:C` and by
naming the operator pair with a conjunction rather than a colon between two Han
characters.

Nine pins (9px → 1.1111111, 10px → 1.1 on five selectors, 11px → 1.0909091 on
three, 12px → 1.25) and **two floors on one element**, because this page produced
the wave's only shrink in BOTH axes: the English strapline WRAPS to two lines at
346.31 × 24 and the Chinese fits on one at 190.36 × 16, so `.title-block`
shrink-wrapped 156 px narrower and 8 px shorter and dragged the whole preset bar
78 px left. `min-width: 346.31px` **and** `min-height: 24px`.

Fourteen rows re-authored after the reverse read — see below.

### O-simplePhysicalModelSynth 1.3.0 (task 3)

101 rows, **2 states as found and 3 as shipped**, 1040 × 860. Zero form-4 nodes
against four `inherit` declarations, zero naked generics. Four font sites take a
tail, across **two distinct serif shapes on different selectors** plus
`--symbol-font`. `i18n-zh-lint` reported **0 findings on its first pass**.

Five pins and one floor. The floor is scoped by key —
`.knob-label[data-i18n="label.knobModeBright"] { min-height: 20.88px }` — because
`Mode Bright` is the page's only caption that wraps in English and not in Chinese,
and `.knob-label` is shared by seventeen captions of which sixteen are one line
in English.

**The third state, and it exists because of a measurement.** `check-ui-labels`
reported three `[data-i18n]` captions that "never became visible". Two of them sit
in cells carrying `.pm-disabled` — `opacity: 0.38` — so they are rendered, driven,
and never COMPARED across languages, while `measure-ui` independently saw the
Mode Bright caption's cell shrink 10.4 px. The new state switches the engine to
Modal through the **real** selector (`applyEngineGating` and `applyDiagramSkin`
both fire from `resonatorType`'s own change event) and takes the hole **3 → 1**.
The remaining one is `String Model`, whose cell has carried `hidden` by design
since v1.2.2 and is structurally unmeasurable rather than unmeasured.

One row re-authored (`label.group4`).

---

## Blind reverse read — all 651 rows, every triple read with `--verbose`

**Shape.** Per plugin, N independently salted `--emit <P> --plugin <P>` runs
(both flags — W4), each with `--forward-provenance`. The REAL ids were
partitioned into disjoint chunks covering every id exactly once, and **each chunk
was cut from its OWN emit through its OWN manifest** — never sliced by line
number, because the emitter sorts by blinded id and the emits are in different
orders. Dispatched with `claude -p … --allowed-tools ""` from `/tmp/h8y-blind*`,
a cwd **outside** the repo, with repo, file and web access forbidden in the
prompt.

| plugin | emits | chunks | round-1 model |
|---|---|---|---|
| O-Tapestop | 2 | 57 + 57 | sonnet |
| O-Lyrica | 3 | 71 + 71 + 71 | sonnet |
| O-simpleBeatmaker | 2 | 56 + 55 | sonnet |
| O-simpleFM | 2 | 56 + 56 | sonnet |
| O-simplePhysicalModelSynth | 2 | 51 + 50 | sonnet |

**Controls, all fired and observed, on every batch:**

| control | result |
|---|---|
| partition disjoint AND covering every real id exactly once | **asserted in code, true on all five** |
| ids pure 12-hex | **0 non-conforming** |
| ids carrying a key fragment | **0** |
| **M12 — ids identical AND IN ORDER, checked BEFORE ingest** | **clean on 10 of 11 chunks; see the fault below** |
| Han surviving in the returned English | **0** |
| malformed returned lines | **0** |
| **refusal 1 — emit without `--forward-provenance`** | **FIRED:** `REFUSED: the batch was emitted with no recorded forward pass — the identity check cannot run` |
| **refusal 2 — wrong-side `--manifest`** | **FIRED:** `joined: 0   unjoinable ids: 3` … `WRONG MANIFEST FOR THIS BATCH: all 3 returned ids are unjoinable, not one.` |
| product-name-in-batch | **0 hits on all five.** Reported, not scored: none of these plugins' own copy names itself |

**M12 caught a reader fault, which is the reason it runs before the ingest.**
O-simpleFM chunk 2 returned **55 lines for a 56-row batch**, and only the order
diff named which row was missing — `606a9579ab75`, `label.subtitle`. It was
re-read on its own by a **different model** and spliced back at its emitted
position before any triple was judged. Six blind chunks in task 3, one reader
fault; the rate does not justify skipping the check.

### Rows re-authored, and why

| # | plugin | key | from | to | reason |
|---|---|---|---|---|---|
| 1 | O-Tapestop | `label.modeStop` | 停止 | **停转** | M13 page collision with 启动 (Engage). Only the ambiguous side qualified; `termNote` at the entry |
| 2 | O-Tapestop | `seg-mode-stop` title | 停止模式 | **停转模式** | keeps the caption and its own tooltip title in agreement |
| 3 | O-Tapestop | `aria.helpToggle` | 切换悬停帮助 | **开关悬停帮助** | Z5: the settled glossary root |
| 4 | O-Lyrica | `label.genGenerator` | 生成音程（¢） | **生成元（¢）** | M13 collision with the 生成 button directly below; the reader returned a verb phrase. `termNote` |
| 5 | O-Lyrica | `label.voices` | 复音数： | **声部：** | GEOMETRY, not terminology — 44.89 px against a 37 px French-era floor. Both are settled roots |
| 6–19 | O-simpleFM | fourteen rows carrying a bare `指数` | 指数 | **调制指数** | read back as **"Exponent"** on every row — a different quantity from the modulation index |
| 20 | O-simpleFM | `routing` body | 调制器调制载波的相位 | **调制器对载波的相位进行调制** | read back as a NOUN PHRASE; the 对…进行 frame cannot be read either way |
| 21 | O-simplePhysicalModelSynth | `label.group4` | 4 · 放大 · 输出 | **4 · 振幅 · 输出** | read back "4 · Amplify · Output". `Amp` is AMPLITUDE, and the two knob tips beneath already said 振幅起音 / 振幅释音 — a page disagreeing with itself |

**Where an M13 qualification STOPS.** O-simpleFM's two arrow captions
`Env→Index` and `Vel→Index` kept the short `指数`, because `.knob-label` carries
`white-space: nowrap` and `包络→调制指数` measures ~71 px against the English
caption's 63.98 px. **Round 2 vindicated the asymmetry**: a different model
returned `Env → Index` and `Vel → Index` correctly for exactly those two rows
while returning "modulation index" for every prose row — an arrow caption
standing beside the `调制指数` knob has an anchor a sentence does not.

### Correction rounds

Every round 2 used a **fresh salt**, a **fresh session** and a **DIFFERENT model**
(opus against round one's sonnet), covering the re-authored rows plus every other
caption on the same panel.

| plugin | round-2 rows | M12 | verdict |
|---|---|---|---|
| O-Tapestop | 6 | 6/6 in order | 停转 → "Stall", 停转模式 → "Stall mode" — the pair is broken. Corrected nothing further |
| O-Lyrica | 11 | 11/11 in order | 生成元 → "Generator (¢)", 生成 → "Generate" — the pair is broken. Corrected nothing further |
| O-simpleBeatmaker | — | — | **nothing was re-authored, so there is no round 2** |
| O-simpleFM | 26 | 26/26 in order | every corrected row read back correctly; the two LEFT captions read back correctly too. Corrected nothing further |
| O-simplePhysicalModelSynth | 10 | 10/10 in order | `4 · 振幅 · 输出` → "4 · Amplitude · Output". Corrected nothing further |

**No round 3 anywhere.**

### The M13 collision that was PREVENTED, not found

On O-Lyrica, R3's screen was run before a single string was authored: 167 English
titles through `TERMS`, 34 groups where more than one key reaches one root, and
**zero groups where two DIFFERENT English keys share a root**. The one that was
not on the screen at all: **`TECHNIQUES` (the tab) and `Technique` (the SOUND-tab
dropdown) are visible at the same time**, and 技法 is the settled root for the
second. Rendering both as 技法 would have been exactly M13's shape, with Z5
silent because each rendering IS the accepted root for its own English. The tab is
not a `TERMS` key, so it was qualified to **演奏技法** at authoring time; both
readers, on two models, duly returned "Playing Technique" and "Technique" as two
different things.

The same screen on task 3's three plugins: **0 groups where two different English
keys share a root** on all three (12, 7 and 8 same-control or same-English groups
respectively).

---

## Checked non-defects — read in full, deliberately left, recorded

1. **Five settings bodies, five left.** `'settings'` on O-Lyrica and O-Tapestop,
   `'gear-btn'` on the three `O-simple*`. Each names both of its panel's controls
   and is already true. **This is the first wave in the rollout with no P6 defect
   at all**, and five waves of mechanical deletion have made deletion the reflex —
   here the reflex would have stripped five true sentences. A probe in each verify
   block asserts each was not edited.
2. **`js/app.js:877` on O-simpleFM** paints `ctx.fillText("fc", …)`, the carrier
   marker. Language-neutral notation, the O-Formant `F1..F5` class. Read, checked
   against `I18N_EXEMPT`, left. The neighbouring `fillText(fmtTickHz(f), …)`
   paints a formatted number and is not a finding either.
3. **`Pluck` / `Strike` / `Bow` and `String` / `Modal` stay Latin inside
   O-simplePhysicalModelSynth's Chinese bodies** — `AudioParameterChoice` entries
   the three combos display in English under D-01. Both directions of the
   `I18N_EXEMPT` membership check were run (W3/N8): the group heading beside them
   IS keyed, so "unless Excitation = Bow" reads `除非激励为 Bow`.
4. **The diagram node `激励` and the column heading `1 · 激励` are the same
   string**, as `EXCITE` and `1 · Excitation` are nearly the same string in
   English. The parallel is the page's own pedagogy — the diagram mirrors the four
   columns — and both readers recovered both correctly.
5. **`节奏手感` (Timing Feel) and `节奏位置 / 律动轨道` (Timing / Groove Lane)
   share `节奏`, as the English shares "Timing".** Not M13's shape: the readers
   returned "Groove feel" and "Groove track", two different phrases, and the heads
   `手感` and `轨道` share nothing.
6. **`静音` is both the Mute control and the word two O-simpleBeatmaker bodies use
   for silence.** English has the same overlap, and the reader distinguished them
   correctly on both ("−60 dB is silence" / "−60 dB mutes it").
7. **The `html` element on the UA `Times`**, on all five. It renders no text and
   everything under `body` re-declares. The carrier no census entry accounts for.
8. **P13 re-confirmed on all five:** no `AudioParameterChoice` names a language,
   measured by scanning each declaration's own text rather than the containing
   file, which reports 1–4 false hits per plugin.
9. **Q12 / D1 verified two ways BEFORE the work and re-confirmed AFTER it.** The
   CMake grep for `scala-tuning-engine` returns exactly six consumers — O-Bassoon,
   O-Bowed, O-Contrabass, O-MicrotonalSampler, O-Reed, O-Wind — and none of this
   wave's five; a `find` for `tuning-panel.js` under each of the five returns 0, so
   no plugin here embeds a private copy either. **O-Lyrica is the plugin one would
   most expect to be a consumer and is not.**
10. **The three `tests/render-harness/` directories** under the `O-simple*`
    plugins are the C++ Stage-2 DSP harness, confirmed by reading their
    `CMakeLists.txt` and `main.cpp` rather than assuming it. Out of scope, untouched.
11. **No gate was invented** for the four plugins that lack one (wave 4c D4).

---

## Deviations from the plan

1. **`plugins/O-Lyrica/tests/i18n-states.json` was edited**, which the plan's
   `<files>` list does not name. One line: state 6's step form, `click` → `eval`,
   with the `elementFromPoint` measurement recorded at the entry. A fixture
   **repair**, not an invented gate — the same file already uses `eval` for four
   other states — and without it ten captions authored in that task would have
   shipped unmeasured behind a green gate.
2. **`plugins/O-simplePhysicalModelSynth/tests/i18n-states.json` gained a third
   state**, which the plan's `<files>` list does not name either — but the plan's
   Task-3 action text explicitly invites it: *"If a panel with copy is
   unreachable, add a state and re-measure."* The reason is recorded at the entry
   as a measurement rather than a conclusion.
3. **`plugins/O-simpleBeatmaker/Source/ui/public/index.html` gained one class
   attribute**, `tr-key-tempo`, so the tempo transport key could take a width
   floor in the shape this file already uses for its other two transport keys.
   The alternative — floring the shared `.tr-label` — would have overridden the
   existing 60 px `.tr-key-length` floor and moved the French arm.
4. **O-Tapestop's `wrap-count` criterion of 0 was not met, and should not have
   been.** It reads 2, and both findings are evidenced non-movers. No pin was
   added.
5. **O-simpleFM's `wrap-count` criterion of 0 was not met either**, for a
   different reason: its one finding is a **pre-existing French wrap** measured on
   the tree as found, before any of this work.
6. **`select` was excluded from O-Lyrica's form-4 rule**, departing from M7's
   shipped `button, input, select, textarea` shape, with the reason measured and
   recorded at the site.
7. **`label.voices` and `label.freeRun` were changed for GEOMETRY, not
   terminology.** Recorded because a diff cannot tell a copy change made for
   meaning from one made for width.
8. **One extra edit on O-Tapestop not in the plan's step list:**
   `aria.helpToggle` 切换悬停帮助 → 开关悬停帮助, forced by a Z5 finding.
9. **`auval -v` is 4 of 5, not 5 of 5** — see the shipped-versions section. The
   fifth is a documented pre-existing failure, proved untouched by an empty
   `.cpp` diff across the whole wave.

---

## Corrections to the plan's own measurements

Instruction 18. **Fifteen predictions were FALSE as stated** across the three
tasks — wave 4d had three, wave 4c had five.

### From task 1 (O-Tapestop)

1. **`existing min-w / min-h / line-h / letter-sp = 0 / 0 / 0 / 0` for
   O-Tapestop is WRONG.** Measured: **5 `line-height`, 1 `min-width`, 21
   `letter-spacing`**, plus several `white-space: nowrap`. The plan's derived
   claim that "every pin you add is the first on its selector and the R8
   specificity grep comes back clean **by construction**" is therefore also
   false — R8 is **live** on this plugin. The grep was run per selector instead
   and the conclusion survived on the evidence.
2. **The verify block's `stats.get('en')` criterion of exactly `2` is WRONG — the
   correct number after the shipped port is `3`.** The generalized print block the
   plan *instructs* porting introduces `const e = stats.get('en');` as the
   baseline every other language is diffed against. The criterion as written would
   fail the very port it mandates.
3. **The verify block's "Fired at planning time: 4" for the two-language-literal
   alternation is WRONG — it fires 3.**
4. **`measure-ui --report all`'s `undeclared-font` screen is not the form-4
   census the plan implies.** It reported **77** findings on a page the plan
   correctly measured at **zero** form-4 nodes.

### From task 2 (O-Lyrica)

5. **Every `index.html` line number in the plan's font worklist is WRONG.** The
   COUNTS are all exactly right — 11, 1, 7, 5 — and not one line number is. The
   offsets are not constant (113, 116, 134 …), so they were measured on a
   transformed file. This is the plan's own tooling trap applied to itself.
6. **`undeclared-font` is not the form-4 census** — re-confirmed from the other
   direction: 177 on a page whose genuine form-4 population is 39.
7. **The verify block's `grep -rn "scala-tuning-engine" plugins/O-Lyrica/ | wc -l`
   criterion of `0` is WRONG — it reads `1`.** The hit is
   `plugins/O-Lyrica/CHANGELOG.md:1739`, a historical prose line. A criterion that
   greps a whole plugin tree, CHANGELOG included, for a module name cannot
   distinguish consumption from a mention of it.
8. **"30 existing `min-width` declarations" counts a JS inline style.** 29 are CSS
   declarations; the thirtieth is `btn.style.cssText = 'min-width: 24px; …'` at
   `index.html:3638`, an inline style at (1,0,0,0) that beats every rule on the
   page and cannot be appended to.
9. **`--verbose` reporting "no unresolved state" does NOT establish that every
   state applied.** Both `measure-ui --verbose` (0 unresolved) and
   `check-ui-labels` (`states: default + 15`) reported clean while one of fifteen
   states was silently doing nothing, because a `force: true` click that lands on
   a covering element **succeeds**.

### From task 3 (the three `O-simple*`)

10. **`existing min-w / min-h / line-h / letter-sp / white-space = 0 / 0 / 0 / 0 / 0`
    is WRONG on all three, and the derived "clean by construction" claim with it.**
    Measured on the trees as found:

    | plugin | min-width | min-height | line-height | letter-spacing | white-space |
    |---|---|---|---|---|---|
    | O-simpleBeatmaker | **14** | 0 | **6** | **7** | **2** |
    | O-simpleFM | **14** | 0 | **6** | **24** | **5** |
    | O-simplePhysicalModelSynth | **11** | **1** | **4** | **19** | 0 |

    Combined with correction 1, **R8 was live on all five plugins in this wave and
    the plan predicted it dead on four.** The greps were run per selector and the
    conclusion survived on the evidence; the pre-existing `line-height` owners were
    named and left untouched.
11. **`font-family: inherit` counts of 5 / 5 / 4 are WRONG on two of the three.**
    Measured: **6 / 6 / 4**. The *finding* — that all three land at zero form-4
    nodes — is unaffected and is if anything sharper: O-simpleBeatmaker declares
    `inherit` **one more time** than O-Lyrica and still reaches zero where O-Lyrica
    reaches thirty-nine.
12. **Every CSS line number in the plan's font worklist for these three is
    WRONG.** The plan cites the serif stacks at L55 / L54 / L54, O-simpleBeatmaker's
    mono pair at L245 / L378, and O-simplePhysicalModelSynth's three-face form at
    L335 / L343. The real numbers are **L79 / L76 / L77**, **L300 / L433** and
    **L404 / L412**. The counts are right; the lines are not — the same class of
    error as correction 5, on a different file type.
13. **`wrap-count: 0` is WRONG on O-simpleFM as a criterion**, because it reads
    **1 on the tree AS FOUND**: `div.routing-label` "Signal Path", en 1 line /
    fr 2 lines, a pre-existing French wrap with no Chinese content. A criterion
    that requires 0 would have failed a plugin before the work started. The
    `--report all` baseline that found it was run before a single string was
    authored, which is what made it attributable.
14. **The plan's Q5 conclusion that `--symbol-font` may need no tail is WRONG as a
    decision procedure.** The plan says to "measure whether any of its 1 / 5 / 2
    nodes can carry Han before deciding on a tail". Measured pre-table, `han` is
    **false** on every one of them — they render `⚙`, `❦` and `♪`. But `han` is
    computed over the node's own text **plus `data-tip`, `data-tip-title` and
    `aria-label`**, and `#gear-btn` is a TIP ANCHOR: it acquires a Chinese
    `data-tip-title` the moment the table lands, at which point the screen flags
    it. **A pre-table `han` census cannot answer this question** — Q11's shape one
    level down. The tail was added on all three; Apple Symbols still wins for the
    glyphs and the Latin arms were re-measured unmoved.

### From the close-out

15. **The plan's human-check "All five report AU VALIDATION SUCCEEDED" is WRONG.**
    Four do. O-Lyrica reports nine PASS sections and one FAIL on a **single**
    error — a Meta Param Flag assertion on `Free Glissando` (`freeToggle`) — which
    its own CHANGELOG has documented since v2.4.x as *"pre-existing and benign …
    Not introduced by this release"*. **Nobody fired it before writing the
    criterion**, which is M3's shape exactly: an asserted precondition that turns
    out to have been red for many releases. It is proved untouched here rather
    than assumed: `git diff 56537bce..HEAD --stat -- 'plugins/*/Source/*.cpp'` is
    **empty** — this wave changed no `.cpp` file on any of the five, and
    O-Lyrica's entire C++ delta is the two-line codec ternary in a header.

**Predictions that held exactly:** all five row counts (114 / 213 / 111 / 112 /
101) and the 651 total, the corpus landing at **3724 of 5256** across **32**
plugins, the state counts (6 / 15 / 5 / 4 / 2), all five parsed frames
(860 × 580, 700 × 450, 1060 × 900, 760 × 980, 1040 × 860), all five identity
triples as found, the `line-height: normal` visible-leaf counts (66 / 380 / 87 /
65 / 55), the declared font-site counts (13 / 24 / 10 / 9 / 9), the four
O-Lyrica font-site counts 11 / 1 / 7 / 5 and its **39** form-4 nodes against
**157** naked-generic nodes, the zero form-4 and zero naked-generic result on the
other four, `svg-font-attr` **0** on all five, the stale-enumeration probe firing
**2** on every one of the five and the settings body being already true on every
one of the five, all five CMake shapes and the one-arm reader returning EMPTY on
exactly three, the byte-identical C++ codec on all five and its per-plugin line
numbers, the installed-family table, every O-ReverseDelay port line, O-Tapestop's
raw gate line numbers L201 / L455 / L459 / L639 / L640 / L654 / L661 / L671 /
L673 / L674, the enumeration-form census (forms 2, 3 and 5, no sixth), the
`--symbol-font` node counts 1 / 5 / 2, O-Lyrica's non-consumption of
`scala-tuning-engine`, PLUGINS.md being one patch stale on all five rows, and the
prediction that O-Lyrica would produce an M13-shaped collision — it produced two.

---

## Commits

| hash | message | plugin |
|---|---|---|
| `1ad239b4` | `feat(O-Tapestop): Simplified Chinese at reviewed:'mt', clamp gate derives its language list` | O-Tapestop |
| `04a7561a` | `feat(O-Tapestop): promote zh-Hans to reviewed:'bt', v1.7.0` | O-Tapestop |
| `8f2e2535` | `feat(O-Lyrica): Simplified Chinese at reviewed:'mt', the wave's whole font surface, one measured coverage hole closed` | O-Lyrica |
| `a62cf6c4` | `feat(O-Lyrica): promote zh-Hans to reviewed:'bt', v2.5.0` | O-Lyrica |
| `8a0199d5` | `feat(O-simpleBeatmaker): Simplified Chinese at reviewed:'mt', 16 line-box pins and a transport-key floor` | O-simpleBeatmaker |
| `b5ddecf3` | `feat(O-simpleBeatmaker): promote zh-Hans to reviewed:'bt', v1.3.0` | O-simpleBeatmaker |
| `85fb3287` | `feat(O-simpleFM): Simplified Chinese at reviewed:'mt', a two-dimensional strapline floor` | O-simpleFM |
| `04c7337a` | `feat(O-simpleFM): promote zh-Hans to reviewed:'bt', v1.5.0` | O-simpleFM |
| `092f57d3` | `feat(O-simplePhysicalModelSynth): Simplified Chinese at reviewed:'mt', a third state that closes a dimmed coverage hole` | O-simplePhysicalModelSynth |
| `995e6e9e` | `feat(O-simplePhysicalModelSynth): promote zh-Hans to reviewed:'bt', v1.3.0` | O-simplePhysicalModelSynth |
| `ff0de99a` | `docs(PLUGINS.md): five wave-4e rows corrected from their CMakeLists values` | — |

One path-scoped commit stream per plugin, PLUGINS.md once at the end. No
`git add -A`, no `git commit -a`, **no tag**, nothing pushed, `modules/`
untouched. `.gsd/dispatch-isolation-sentinel.json` is modified in the tree,
predates this work, and was never staged.

---

## Self-Check: PASSED

Files asserted to exist, checked on disk:

```
FOUND: plugins/O-simpleBeatmaker/Source/ui/public/js/i18n.js
FOUND: plugins/O-simpleFM/Source/ui/public/js/i18n.js
FOUND: plugins/O-simplePhysicalModelSynth/Source/ui/public/js/i18n.js
FOUND: plugins/O-simplePhysicalModelSynth/tests/i18n-states.json
FOUND: PLUGINS.md
```

Commits asserted, checked with `git log --oneline --all | grep -q <hash>`:

```
FOUND: 1ad239b4   FOUND: 04a7561a   FOUND: 8f2e2535   FOUND: a62cf6c4
FOUND: 8a0199d5   FOUND: b5ddecf3   FOUND: 85fb3287   FOUND: 04c7337a
FOUND: 092f57d3   FOUND: 995e6e9e   FOUND: ff0de99a
```

Numbers asserted, re-measured after the last commit:

```
corpus                       3724 of 5256 rows, BELOW SHIP BAR 0, 32 plugins   ✓
i18n-zh-lint repo-wide       exit 0, 0 findings across 43, 2804 entries         ✓
i18n-zh-lint --self-test     10/10                                              ✓
check-i18n repo-wide         exit 0, 43 localized plugins                       ✓
i18n-fr-lint                 exit 0                                             ✓
boot-all-uis --strict-tips   0 dead, 0 failed, 0 warn, exactly 2 late (O-Bells) ✓
check-ui-labels              exit 0 on all five                                 ✓
measure-ui --verbose         0 unresolved states on all five                    ✓
clamp gate                   exit 0, == ALL CHECKS PASSED ==                    ✓
PLUGINS.md duplicates        none                                               ✓
cmake vs registry            AGREE 5 of 5                                       ✓
alternate-variant orphans    0                                                  ✓
git tag --points-at HEAD     0                                                  ✓
auval -v                     4 of 5 SUCCEEDED; the fifth is documented above    ✓
```
