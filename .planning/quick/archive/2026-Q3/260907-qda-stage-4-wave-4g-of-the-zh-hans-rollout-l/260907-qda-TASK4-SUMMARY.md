---
task: 4
plugin: O-simpleSubtractive
phase: quick-260907-qda
wave: 4g
status: complete
shipped_version: "1.5.0"
bumped_from: "1.4.1"          # the CMakeLists VALUE, not the 1.4.0 PLUGINS.md says
rows: 133
reviewed: bt
requirements: [ZH4G-04, ZH4G-09, ZH4G-12]
commits:
  - fff62ae8   # the 'mt' table, the body correction, the font tail, the codec, the geometry
  - 09bb37cf   # promotion to 'bt', version bump, CHANGELOG
false_predictions: 4
wrap_count_baseline: 1
wrap_count_after: 1
---

# Task 4 — O-simpleSubtractive 1.4.1 → 1.5.0, Simplified Chinese

133 rows of Simplified Chinese at `reviewed: 'bt'`, shipped as **1.5.0**, built and
installed. Two path-scoped commits. Every gate green. `auval` deliberately not run — it is
Task 7's single cold sweep.

**K9 held.** The `wrap-count` baseline was **1** before a byte of Chinese and is **1**
after, on the same node and the same arm. It was not driven to 0.

---

## K9 — the finding, measured on this plugin, and the correction to its SCOPE

### The baseline, fired on the tree as found

```
wrap-count: 1 finding(s)
  92 text-bearing leaf node(s) compared; 70 of them ESTIMATED at fsn * 1.2
    html/body[1]/div[1]/section[2]/div[1]  (div.routing-label)  "Signal Path"  en=1 line(s), fr=2 line(s)
```

### The same line, on the shipped tree

```
wrap-count: 1 finding(s)
  92 text-bearing leaf node(s) compared; 70 of them ESTIMATED at fsn * 1.2
    html/body[1]/div[1]/section[2]/div[1]  (div.routing-label)  "Signal Path"  en=1 line(s), fr=2 line(s)
```

**Byte-identical.** Same count, same node, same arm, same line numbers. A 2, or a
different node, would have been this wave's regression; a 0 would have meant a French
geometry question was silently "fixed" that wave 4e D7 declined on purpose.

### The zh arm, MEASURED on this plugin rather than inherited

4e D7 records that the Chinese arm of this node is one line **on O-simpleFM**. That is a
prior, not a measurement of this plugin. Measured here, all three arms:

| arm | text | rect h | computed line-height | lines |
|---|---|---|---|---|
| en | `Signal Path` | **11** | `normal` | **1** |
| fr | `Chaîne du signal` | **22** | `normal` | **2** |
| **zh-Hans** | `信号路径` | **11** | **`11px`** (pinned) | **1** |

The prior held — but it held **because the node was pinned**, not on its own. Unpinned,
the Han line box at 10px measured 14px against English's 11px (`d = +3`), which is a
height change, not a wrap. The Chinese takes one line either way; what needed pinning was
the line's HEIGHT, not its count. **An executor who read 4e D7's note as a measurement of
this plugin would have skipped a pin this page needed.**

### The correction is to the SCOPE, not only to the plugin name

`grep -rl "routing-label" plugins/ | grep -E "index.html|styles.css"` returns **five files
across THREE plugins**:

| plugin | markup carrier | CSS carrier |
|---|---|---|
| O-simpleFM | **yes** | yes |
| **O-simpleSubtractive** | **yes** | yes |
| O-simpleGrain | no | yes |

`routing-label` is a **shared class with at least two markup carriers**, and wave 4e
measured O-simpleFM because O-simpleFM was in wave 4e. Nobody measured the other carriers,
so nobody saw the second one. This is F1's shape exactly: a finding derived from one
carrier looked measured and had no second site to check it against. Carried forward as a
shared-class finding with a named carrier list (deferred item 2 below), so the next reader
does not re-derive it from one site a third time.

### The F8 reading, and why the ESTIMATED denominator did NOT move here

`70 of 92 ESTIMATED` before, `70 of 92 ESTIMATED` after — **unchanged**, where Task 1 saw
84 → 62, Task 2 saw 70 → 37 and Task 3 saw 58 → 33.

That is a consequence of a decision, not luck. **Every pin on this plugin is scoped to
`html[lang="zh-Hans"]`**, so the en and fr arms still compute `line-height: normal` and
stay in the estimated set; only the zh nodes left it, and the screen's ESTIMATED header
counts the en-vs-fr comparison. The scoping was chosen for K9's sake — a global
`line-height` on `.routing-label` would have changed the French box on a node the French
deliberately lets wrap — and holding the denominator still is a second dividend of it:
**there is no F8 divisor movement to disentangle from a real geometry change on this
plugin.** Read alongside Tasks 1–3, whose divisors all moved, this is the controlled
contrast for what scoping buys.

---

## The false-prediction ledger

Four plan predictions were FALSE on the tree. Two are the wave's recurring pair, confirmed
a fourth and third time respectively.

### FALSE-1 — precondition (i): `git status --short | grep -c "O-Strata"` reads **0**, not 2

Tasks 1, 2 and 3's FALSE-1, confirmed a **fourth** time. A concurrent session committed
both O-Strata planning files before this wave began. The untracked set on the tree today is
this wave's own plan directory alone, which is likewise never staged.

**K2's load-bearing claim is unaffected and was re-fired live:** O-Strata still has no
`i18n.js`, and both repo-wide lints still exit 2 with `0 finding(s) across 0 plugin(s)` and
`1 plugin(s) could not be read`. The exit code and the `git status` line were never the
baseline; the finding count and the unreadable count are.

`.claude/agent-memory/research-planning-agent.md` is still modified and still another
session's file. It was never staged.

### FALSE-2 — precondition (j): the positive control CANNOT fire before the table lands

Tasks 2 and 3's FALSE-2, same mechanism, confirmed a **third** time. The precondition asks
for a Han negative grep over `Source/**/*.{h,cpp}` "beside a positive control on the same
run", fired **before any edit**. Pre-work `js/i18n.js` holds zero Han code points, so the
control necessarily prints nothing — and a green there would mean the probe was broken, not
that the file was clean.

**The control's correct home is the VERIFY step**, and there it fired: the negative printed
nothing across every `.h`/`.cpp` under `Source/`, and the control printed the `i18n.js`
path on the same run.

### FALSE-3 — the R3 glossary screen's denominator is **41**, not 16

The plan records `NONE (16 matched)` for this plugin, the wave's smallest matched set.
Re-fired here over captions **and** tooltip titles: **41 glossary-matched of 97 screened,
collisions NONE.** The same denominator difference Task 1 (22 → 50), Task 2 (27 → 76) and
Task 3 (23 → 69) recorded — the planning screen matched captions only.

**The verdict is identical and that is the part that matters.** A mismatch here is a
denominator difference, not a finding.

### FALSE-4 — the emitter has **no product-name control** in this version

Task 3's FALSE-5, re-confirmed. The plan directs the executor to "expect the no-product-name
control to report a non-zero count … adjudicate it". `grep -n "product"
scripts/i18n-zh-backtranslate.js` returns nothing; the control does not exist in this
version, so there is no count to read. **The rule has no subject; the substance still
does**, so the check was made by hand:

- occurrences of the product name `simpleSubtractive` in the emitted zh column: **0**;
- rows carrying the morpheme 减法 (*subtractive*): **6**, and every one translates the
  COMMON NOUN in its own English sentence — `label.subtitle` ("Subtractive Synthesizer"),
  `body|cutoff`, `body|filterType`, `body|lessonSawSweep`, `body|lessonNoiseWind`,
  `label.captionSawSweep`, `label.captionNoiseWind`.

The wordmark is split across the two `I18N_EXEMPT` text nodes `'O – simple'` and
`'Subtractive'` and never enters the table. **Adjudicated, not counted.**

### K16 is TRUE on this plugin, unlike Task 3's

Task 3 found its fourth state was an `eval` and not a click. Checked here before asserting
anything on rects: `tests/i18n-states.json` carries **three entries and every one of them
has a `click` key**, no `eval` anywhere. The state-effect probe therefore fired on all
three, in all three languages.

---

## The second version source, classified rather than assumed (Task 1's FALSE-2 lesson)

K6 says no other plugin in the six has one. Following Task 1's warning, the harness was
grepped and the finding **classified** rather than acted on:

```
plugins/O-simpleSubtractive/tests/render-harness/CMakeLists.txt:54  JucePlugin_VersionString="1.0.0"
plugins/O-simpleSubtractive/tests/render-harness/CMakeLists.txt:55  JucePlugin_VersionCode=0x10000
```

**A frozen literal, not a `${VAR}` mirror** — the same class as O-simpleAdditive's and
O-simpleSampler's, not O-simpleGrain's. It has never tracked the plugin version and is now
five minors stale; bumping the plugin does not desynchronise it any further than it already
is. **Not touched**, deliberately: it is pre-existing, it is in a file wave 4e D6 puts out
of scope, and editing it would be a change nothing asked for inside a commit whose subject
is a caption table. Carried as deferred item 1.

---

## Blind-read accounting

| | |
|---|---|
| rows dispatched | **133 of 133** (72 + 61), 0 withheld from review |
| chunks | 2, split on the CONCEPT (F17) — every tooltip in one, every caption in the other |
| ids shared between the two chunks | **0**, measured — two separate emits, two salts |
| ids returned identical AND IN ORDER (M12) | **yes**, both chunks, diffed positionally |
| Han surviving into returned English | **0** |
| malformed returned rows | **0** |
| dispatched zh column vs the COMMITTED tree | **byte-identical**, sha256 `74bab82aef4c0da4` on both sides |
| product-name control | **NO SUBJECT — the script has none** (FALSE-4); adjudicated by hand |
| triples read with `--verbose` (R2) | **133 of 133** round 1, **13 of 13** round 2 |
| lexically identical after normalisation | 57 of 133 |
| rows re-authored | **1** |
| correction rounds | **2**; round 2 corrected nothing further, so no round three |
| forward model | `claude-opus-5` |
| round-1 reverse model | `claude-sonnet-5` — read back from the process, not the alias (F15) |
| round-2 reverse model | `claude-haiku-4-5-20251001` — a THIRD model, fresh salt, fresh session (R4) |
| refusal controls fired | **2 of 2**, against a properly provenanced emit (F10) |
| termNote exemptions | **1**, entry-scoped (N4) |

Both dispatches ran from `/tmp/qda-t4/blind`, a cwd **outside** the repository, with
`--allowed-tools ""`, `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1`, and repository access
forbidden in the prompt text itself.

### Both refusal controls, verbatim

1. `--provenance` omitted, manifest correct →
   `REFUSED: back-translation provenance is missing or identical to the forward pass — this triple proves nothing`
   `  no --provenance was given.`
2. wrong-side `--manifest`, **with** `--provenance` →
   `joined: 0   unjoinable ids: 72`
   `WRONG MANIFEST FOR THIS BATCH: all 72 returned ids are unjoinable, not one.`

The second only reaches its own refusal because the reverse provenance was supplied; run
without it, it refuses for reason 1 and proves nothing about the manifest (F10).

### The one re-authoring, and why (collision on the page, never drift distance)

**`label.sub`.** Drafted to the glossary root `低音` — the low-frequency BAND word — and
round 1 returned it as **"Bass"**. The control is the **sub-oscillator mix level**, and
this entry's own tooltip title already reads `副振荡器` (*Sub Oscillator*). A caption and
its own body naming two different things about one control is the same shape as Task 1's
`Organ`, and it is a stronger failure than a collision: the reading is wrong about what the
control does.

**The corpus site count was checked before the note was written** (N4). The English caption
`Sub` appears at exactly **three** sites, and both of the others are BAND controls:

| plugin | key | English | shipped zh | what it controls |
|---|---|---|---|---|
| O-Bells | `label.sub` | `Sub` | `低音` | the octave-BELOW blend, paired with `label.oct` |
| O-FreqPulse | `bandName.sub` | `SUB` | `超低频` | a crossover band name |
| **O-simpleSubtractive** | `label.sub` | `Sub` | **`副振荡`** | a **sub-oscillator** mix level |

Re-rendered as `副振荡`, shortened from the three-character tooltip title to match the
English caption's own register — which is likewise the abbreviation of its own title — with
an entry-scoped `termNote` recording why the root was left and what its other two sites
control. **Flagged for the glossary rather than forked silently.**

Round 2, on a third model with a fresh salt, returned it as **"Sub Oscillation"**, agreeing
with its own tooltip title, which round 2 read as "Sub Oscillator". Nothing else in that
round changed.

### Accepted drifts, each with its written reason

- **`label.res` "Res" → "Resonance"**, and `label.res`, `label.resonance` and
  `title|resonance` all render `共振`. Adjudicated, not overlooked: English abbreviates one
  of them and Chinese has no abbreviations, so the two registers collapse onto one word.
  All three name the SAME control property, so a reader cannot be misled about which is
  which — the collision R3 exists to catch is two DIFFERENT things reading alike. Recorded
  in the file's own header.
- **`title|ampAttack/Decay/Sustain/Release` "Amp X" → "Amplitude X"** — the glossary roots
  `振幅起音` etc. Task 1 accepted the identical shape; the filter family reads `滤波起音`
  etc. and the two families cannot collide.
- **`label.routeAmpEnv` and `title|ampAdsr` both render `振幅包络 → 电平`**, while the
  filter arm keeps two distinct renderings (`滤波包络` on the 8px diagram caption,
  `滤波器包络` on the tooltip title). English distinguishes both pairs by register —
  lowercase and abbreviated against title-case and spelled out. Chinese has a genuine
  short/long pair for the filter term and none for the amplitude one, so **the asymmetry is
  a property of the terms rather than of the authoring**, and both name the same signal
  route either way.
- **`label.nodeOsc` "OSC" → "Oscillation"** (`振荡`) beside `label.groupOsc` "Oscillator"
  (`振荡器`). The English does the same thing — an abbreviation in the diagram and the full
  word on the group heading — and `label.nodeFilter` (`滤波`) read back as "Filter"
  cleanly, so the pair is consistent.
- **`label.groupOutput` "Voice / Output" → "Sound, Output"** (`发声、输出`). The ASCII
  slash was replaced by a Chinese enumeration comma, which the reverse read renders as a
  list rather than an alternation. `发声` agrees with `发声模式` (Voice Mode), the group's
  own first control, which round 2 read back as "Voice Mode" exactly.
- **`label.wave` "Wave" → "Waveform"** and `title|oscWave` "Oscillator Wave" → "Oscillator
  Waveform" — `波形` is the glossary root and has no shorter form. `label.scope` "Output
  Waveform" is `输出波形`, a distinct string on a distinct control, exactly as in English.
- **`label.tourLabel` "Lesson Presets" → "Tutorial presets"** — `教学预设`, the glossary
  root, shipped on four other plugins.
- The remaining ~50 body drifts are register and article changes ("Time for X to rise" →
  "The time it takes for X to rise"), with the technical nouns, the mode names and the
  numeric claims intact on every one.

---

## The R3 mechanical downstream check, after authoring

Grouping the 133 zh renderings and reporting any two DIFFERENT keys sharing one, excluding
same-control pairs and same-English pairs:

| group | verdict |
|---|---|
| `设置` — `title\|gear-btn` / `label\|aria.settings` | same-English pair, excluded |
| `噪声` — `title\|noiseLevel` / `label\|label.noise` | same-English pair, excluded |
| `信号路径` — `title\|routing` / `label\|label.signalPath` | same-English pair, excluded |
| `键位跟踪` — "Key Tracking" / "Key Track" | **same control** (a caption and its own tooltip title), excluded |
| `共振` — "Resonance" ×2 + "Res" | ACCEPTED, reasoned above |
| `振幅包络 → 电平` — "Amp Envelope → level" / "amp env → level" | ACCEPTED, reasoned above |

The R3 `TERMS` screen fired **before** authoring returned **NONE** across 41
glossary-matched of 97 screened.

---

## Measured numbers, before and after

### Geometry — `check-ui-labels`, zh arm, across all four walked states

| | non-label elements moved | assertions 5 / 6 |
|---|---|---|
| table landed, no pins | **162** (166 on one state) | FAIL 11 spills / FAIL 11 frame crossings |
| after 10 line-box ratios + 1 width trim | **43** | PASS / PASS |
| after the reserved flex line | **0** | PASS / PASS |
| after the re-authoring and promotion | **0** | PASS / PASS |

**The fr arm was green at every one of the four measurements**, and coverage never moved off
`55 of 54` visible with **0 never-visible**. `55 of 54` is F18's shape — N counts paths seen
across the cumulative walk, M is final-DOM membership — and is not a failure.

### The ten line-box ratios, every one from that element's own English CONTENT box

Rect height less its own padding and border, divided **per line** by its own font-size, line
count verified. **None of the ten selectors carried a `line-height` or `min-width`
declaration before this block** — grepped one by one (R8/F7).

| selector | fs | en rect | pad t/b | border t/b | lines | content/line | ratio |
|---|---|---|---|---|---|---|---|
| `.subtitle` | 11 | 12 | 0/0 | 0/0 | 1 | 12 | **1.0909091** |
| `.settings-label` | 11 | 12 | 0/0 | 0/0 | 1 | 12 | **1.0909091** |
| `.tour-caption` | 11 | **24** | 0/0 | 0/0 | **2** | 12 | **1.0909091** |
| `.tour-btn` | 10.5 | 23 | **5/5** | **1/1** | 1 | 11 | **1.0476190** |
| `.routing-label` | 10 | 11 | 0/0 | 0/0 | 1 | 11 | **1.1** |
| `.tour-label` | 10 | 11 | 0/0 | 0/0 | 1 | 11 | **1.1** |
| `.group-title` | 12 | 19 | **0/3** | **0/1** | 1 | 15 | **1.25** |
| `.viz-label` (ROW) | 10 | 11 | 0/0 | 0/0 | 1 | 11 | **1.1** |
| `.keyboard-label` (ROW) | 10 | 11 | 0/0 | 0/0 | 1 | 11 | **1.1** |
| `.routing-readout` (ROW) | 13 | 16 | 0/0 | 0/0 | 1 | 16 | **1.2307692** |

**`.tour-btn` is the F6 form-control case, live.** M8's leaf table gives 10.5px → 1.1666667
against its measured 1.047619, because the UA `font` shorthand resets `line-height` on a
button. Task 1 measured 1.0476 for the same class at the same size; **this is the second
independent site for that number.**

**`.group-title` is the padding case.** Its rect is 19 and its content box is 15, because
the rect carries 3px of bottom padding and a 1px dotted border. A ratio taken from the rect
would have been 1.5833. It lands on the same **1.25** Task 1 and Task 2 each measured for
the same class at the same size — **a third independent site.**

**`.tour-caption` is the multi-line case.** Its English rect is 24 over TWO lines, so the
per-line content box is 12 and the ratio is the same 1.0909091 as the single-line 11px
leaves. Dividing the rect by the font-size would have given 2.18 and doubled the Chinese
box.

**Three ROW pins (N4, wave 4e).** `.viz-label`, `.keyboard-label` and `.routing-readout`
each grew (`dh` = +3, +3, +2) while **every inline child inside them measured `enH == zhH`**:
`getBoundingClientRect()` on an inline element returns its FONT box, not its line box, so a
row grows with no leaf able to report it. Pinned as unitless RATIOS so the children
recompute rather than inherit a length.

### The one place Chinese comes out WIDER — a measured trim, not a chosen one

`.routing-readout-col` is `margin-left: auto`, so its width is content-sized and growing it
drags its own left edge and both readout spans leftward.

| | en | fr | zh, before | zh, shipped |
|---|---|---|---|---|
| `label.res` span width | **21.00** | **21.00** | **27.53** | **21.00** |
| `.routing-readout` width | 86.61 | 86.61 | **93.14** | 86.61 |
| `#readCutoff` x | 1051.39 | 1051.39 | **1044.86** | 1051.39 |

Han is full-width, one em per glyph, and Chinese has no abbreviation for the term, so no
shorter faithful rendering exists. Two full-width glyphs at **10.5px with the Latin
`letter-spacing: 0.5px` removed** is 21.00px **exactly** — the English box to the pixel.
10.5px is a size this page already uses on `.tour-btn` and the neighbouring `.routing-meta`
is 10px, so the strip stays typographically of a piece. **Not a fixed width and not a
floor:** a floor cannot cap a box that is already too wide.

### The reserved flex line — the largest single cascade, and Task 1's shape exactly

`.preset-tour` is `flex-wrap: wrap` and holds three items: the 99px pinned `.tour-label`,
`.tour-buttons` (`flex: 1 1 auto`) and `.tour-caption` (`margin-left: auto`,
`max-width: 46%`).

| | en / fr | zh, before the floor |
|---|---|---|
| `section.preset-tour` height | — | **−26.0** |
| `div.tour-buttons` width | — | **−258.3** |
| `section.keyboard-panel` y | — | **−26.0** |
| `#keyboard` and its 25 keys, y | — | **−26.0** |

In English and French the resting caption's natural width exceeds the 46% cap, so it is
clamped to **505.08px — 46% of the 1098px content line, to the pixel** — and the three items
no longer fit on one flex line. The Chinese caption is far shorter than the cap, so all
three fitted on ONE line: the section lost a whole flex line and the button group collapsed
258.3px around its shorter faces, taking the entire keyboard panel up with it. **Thirty-one
of the 43 remaining moved elements were downstream of this one cause.**

Floored at the cap, **in the cap's own unit** — `min-width: 46%` — so the floor and the cap
meet exactly at any frame rather than at a pixel measured at one. Neither Latin arm can see
it (both are already clamped there), and the lesson-preset state, whose Chinese caption IS
long, is clamped too, so the floor is a no-op there as well.

Task 1 recorded the identical mechanism on the identical class names with a 256.34px
collapse. **This is its second site, and it is now a two-site pattern rather than a
one-plugin observation.**

### `measure-ui` screens — the K12 vacuum, closed

| screen | baseline (VACUUM) | after |
|---|---|---|
| `undeclared-font` | 0 — **input EMPTY** | **0 against 104 visible Han-bearing nodes**, 0 of which resolve without a CJK face |
| `line-height-normal` | 0 — input empty | 33 → **5**, all five named below, all `enH == zhH` |
| `wrap-count` | **1 — REAL, not a vacuum** (K9) | **1**, same node, same arm |
| `svg-font-attr` | 0 carriers | 0 carriers |
| identity | 520 nodes / 260 keys / 120 ids | **783** / 261 / 120 (a third language pass) |
| leaves ESTIMATED | **70 of 92** | **70 of 92 — UNCHANGED** (see the F8 reading above) |

Every zero after the table landed is a **measurement**. Every zero before it was a vacuum.

**The five `line-height: normal` residuals, each named with its measured `enH == zhH`
(N1):**

| node | fs | enH | zhH | why it cannot be pinned to any effect |
|---|---|---|---|---|
| `#help-toggle` | 10 | **24** | **24** | a fixed-height form control |
| `svg text` (`滤波包络 → 截止`) | 8 | **6** | **6** | an SVG text node; the box is the glyph run |
| `svg text` (`振幅包络 → 电平`) | 8 | **6.23** | **6** | as above; the 0.23px is a Latin-vs-Han glyph-box difference, under the 0.5px threshold |
| `svg text` (`振荡`) | 11 | **9** | **9** | as above |
| `svg text` (`滤波`) | 11 | **9** | **9** | as above |

### The state-EFFECT assertion — all THREE click states, in all three languages (N1)

`tests/i18n-states.json` carries three entries and **every one has a `click` key** — no
`eval` on this plugin (K16 TRUE here, FALSE on Task 3's).

| state | target | rect | `elementFromPoint` | took effect |
|---|---|---|---|---|
| settings popover OPEN | `#gear-btn` | 22×22 @(1131,19) | target-or-descendant | **YES** — visible `[data-i18n]` 51 → 54, all three langs |
| hover help OFF | `#help-toggle` | 96×24 @(1046,87) | target-or-descendant | **YES** — face `On`→`Off`, `Activées`→`Désactivées`, `开启`→`关闭` |
| lesson preset picked | `.tour-btn[data-preset="Acid Bass"]` | 71×23 @(462,915) en / 74×23 fr / 69×23 zh | **`NULL`** | **YES** — visible 54 → 51, all three langs |

**The `NULL` is discriminated on the rect, not read as a coverage hole.** The target's y is
**915** against an 820px frame — it is below the fold in the `overflow-y: auto` pane, and
Playwright scrolled it into view before clicking. N1's own discriminator. It is also **the
LAST state**, so the scroll it causes cannot displace any state after it.

`measure-ui --verbose` reports **0** unresolved or skipped states.

### Gate results

| gate | result |
|---|---|
| `check-i18n --plugin` | **exit 0** — `LANGUAGES … got ["en","fr","zh-Hans"]`, `36 I18N + 61 LABELS` unchanged, `36 tip(s) bound`, `[12] 1 module(s): js/app.js` |
| `i18n-zh-lint --plugin` | **0 findings**, 133 rows / 97 entries, **BELOW SHIP BAR 0**, 1 termNote exemption |
| `i18n-zh-lint --self-test` | **10/10** |
| `check-ui-labels --plugin` | **exit 0**, `== ALL CHECKS PASSED ==` |
| `measure-ui --verbose` | 0 unresolved / skipped states |
| `boot-all-uis --strict-tips` (repo-wide) | **clean 43/43**, 0 warn, 0 failed, **0 DEAD**, **2 late across 1 plugin** — O-Bells, the unchanged 4e D2 / 4f D12 census control |
| `i18n-zh-lint` repo-wide | **0 findings across 0 plugins, 1 unreadable** — exit 2, the K2 **baseline**, unchanged |
| `i18n-fr-lint` repo-wide | **0 of 44 with findings, 1 unreadable** — exit 2, same baseline. **No French rendering changed** |
| `check-i18n` repo-wide | `ALL CHECKS PASS — 43 localized plugin(s)` |
| corpus | **5050 → 5183** of 5441 rows; **41** plugins carrying at least one zh row |

**The K1 reading at this point**, both denominators side by side: `check-i18n` reports **43**
(plugins that have an `i18n.js` at all) and the emitter reports **41** (plugins that have zh
rows in it). They are two apart, and the two are exactly O-Polystutter and O-Orbit — Tasks 5
and 6. O-Strata is the 44th, excluded by name for having no `i18n.js` under either UI root.

### Stale-body and dead-class verdicts

| class | verdict | evidence |
|---|---|---|
| the fixed-pair language enumeration | **LIVE, deleted in en and fr** | comment-stripped `-CSD` probe **2 → 0** |
| its numeric exception list | **KEPT, re-verified** | markup carries **5** `select` elements; minus the language selector = **4**; *"the four drop-down menus"* is true today. **This is the wave's only exception list naming a number other than two**, so the check was not a formality |
| the "and nothing else" settings class | **NO SUBJECT — checked non-defect** | `grep -c "'tip.settings'"` = **0**; `grep -c "tips-toggle"` in markup = **0** |
| the "Tuning tab stays English" class | **NO SUBJECT — checked non-defect** | not a `scala-tuning-engine` consumer; no tuning tab |

The superseded phrasing is in the CHANGELOG and is **not** respelled in any source comment
(C8), which is why the probe reads the file as fixed.

### Font work — the tail half only, and both files searched

`index.html` declares **ZERO** `font-family` sites; all 11 are in `css/styles.css`, 7 of
them `inherit`. A census of the markup alone would have read a clean bill of zero — Task 1's
finding 4, live again.

| declaration | treatment | why |
|---|---|---|
| `'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', serif` | **tail**, before the trailing generic | Times New Roman is installed and 4th in the stack, so the Latin is already safe. The absent Garamond members were KEPT — they are the Windows/print intent |
| `--symbol-font: 'Segoe UI Symbol', 'Apple Symbols', 'Noto Sans Symbols2', 'Arial Unicode MS', serif` | **tail** | N3, measured not assumed — see below |
| 7 × `font-family: inherit` | none needed | they inherit the tailed house stack |

**The `--symbol-font` verdict (N3), measured.** Three consumers: `.gear-btn` and
`.node-out` twice.

- **`.gear-btn` is a TIP ANCHOR** — `#gear-btn` is row 1 of `TIP_BINDINGS`. `measure-ui`'s
  `han` counts `data-tip`, `data-tip-title` and `aria-label`, so the node carries Han
  regardless of the single gear glyph it paints. **The discriminator is whether the node is
  an anchor, never what it renders.** Tail required.
- **`.node-out` is NOT an anchor** — it is the ♪ glyph inside `#routingSvg`, whose tip is
  bound on the enclosing `#routingPanel`. It takes the tail incidentally through the shared
  token, which is harmless: Apple Symbols is installed and resolves the glyph first.

Verified by re-running `measure-ui --report all` and reading `undeclared-font: 0` against a
**non-empty** input — 104 visible Han-bearing nodes, **0** of which resolve without a CJK
face — never by reading the CSS back.

### Zeros recorded AS MEASURED

- **gate files: 0.** `find plugins/O-simpleSubtractive/tests -name '*.js'` returns nothing.
  Fired with `find`, never a glob, because a zsh glob matching nothing aborts the whole
  command and reads as an answer. **No gate was invented** (C5/C6 has no subject in wave 4g).
- **unscanned module JS: 0.** `find <ui root> -name '*.js' -not -path '*/juce/*'` over the
  WHOLE root returns exactly **two** files, `js/i18n.js` and `js/app.js`. Together with the
  tracer this is the controlled converse to the four plugins that carry one — **a
  whole-root `find` zero is a measurement; a `js/*.js` glob zero is a query artefact.**
- **duplicate keys: NONE.** Two forms, both run: the plan's F13-corrected regex reads
  `quoted keys=64 duplicates NONE` (it matches quoted keys only, so bare identifiers such
  as `oscWave` are outside its denominator), and a module-level scan over every loaded key
  reads `keys=97 duplicates NONE`.
- **language `AudioParameterChoice`: none.** Re-confirmed by scanning each declaration's own
  text, not the containing file. The four on this plugin — `oscWave`, `filterType`,
  `filterSlope`, `voiceMode` — keep English option strings under D-01 and are named in
  English inside the Chinese bodies (W3, both directions).
- **submodule untouched.** `git status --short -- plugins/O-Orbit/libs/SAF` → empty;
  `git submodule status` → ` b6fe1882… (v1.3.4)` (leading space = clean). The guard ran
  before **both** commits even though no path here is inside it.
- **tags created: 0.**
- **alternate-variant orphan: none.** Phase 4's dual-variant sweep emitted no
  `⚠ Sweeping ALTERNATE-variant` warning.

---

## Commits

| hash | subject |
|---|---|
| **`fff62ae8`** | `i18n(O-simpleSubtractive): add Simplified Chinese at reviewed:'mt' — 133 rows, the wrap-count baseline held at 1` |
| **`09bb37cf`** | `i18n(O-simpleSubtractive): promote 133 zh rows to reviewed:'bt' after a two-round blind reverse read — v1.5.0` |

Both path-scoped `git commit -F <file> -- plugins/O-simpleSubtractive`, options before the
`--` (F14.6). `git branch --show-current` and `git status --short` were re-checked
immediately before each, never once at the start. The submodule guard ran before each.
Nothing under `plugins/O-Strata/`, nothing under `.claude/agent-memory/`, nothing under
`.planning/` was ever staged. `PLUGINS.md` was not touched — Task 7 owns it.

---

## Build and install

`./scripts/build-and-install.sh O-simpleSubtractive`, run in the background and polled on a
sentinel — **exit 0, 50 s.** The folder name equals the `juce_add_plugin` target here.

```
VST3: ~/Library/Audio/Plug-Ins/VST3/O-simpleSubtractive-dev.vst3            4.6M, age 0s
AU:   ~/Library/Audio/Plug-Ins/Components/O-simpleSubtractive-dev.component 4.5M, age 0s
```

`CFBundleShortVersionString` read back from BOTH installed bundles: **`1.5.0`** — the
version reached the binaries, not just the CMakeLists. Only the `-dev` variants are on
disk; no alternate-variant orphan.

**`auval` was NOT run.** It is deferred to Task 7's single cold registry sweep, as the plan
directs. Budget it at ~85 s, not the stale 15-minute figure.

---

## Deferred items

1. **O-simpleSubtractive's render-harness carries a frozen version literal.**
   `plugins/O-simpleSubtractive/tests/render-harness/CMakeLists.txt` hard-codes
   `JucePlugin_VersionString="1.0.0"` / `JucePlugin_VersionCode=0x10000`. It has never
   tracked the plugin version and is now five minors stale. Unlike O-simpleGrain's it is a
   dead literal rather than a `${VAR}` mirror, so it does not desynchronise on a bump. Out
   of scope here (wave 4e D6). **Grep-able token: `JucePlugin_VersionString="1.0.0"`.**
   This is the **third** plugin in the wave found carrying the pattern (O-simpleAdditive at
   1.0.2, O-simpleSampler at 0.1.0, this one at 1.0.0) — it is a suite-wide sweep, not a
   per-plugin oddity.

2. **`routing-label` is a SHARED CLASS and the carry-forward names one carrier.** Wave 4e
   D7 and wave 4f D12 both record the French wrap as "a pre-existing French wrap on
   O-simpleFM". It is a property of the CLASS: five files across three plugins carry it —
   **O-simpleFM** (markup + CSS), **O-simpleSubtractive** (markup + CSS) and
   **O-simpleGrain** (CSS only, no markup carrier). Both markup carriers wrap in French and
   neither wraps in English or Chinese. The finding should be re-scoped to the class with
   that carrier list attached, and any future decision about it taken once for all
   carriers. **Grep-able token: `routing-label` in `plugins/*/Source/ui/public/index.html`.**

3. **The `sub` glossary root is a BAND word being used for a sub-oscillator.**
   `scripts/i18n-zh-glossary.js` gives `'sub': ['低音','超低频']`, both low-frequency BAND
   words. Both of its other corpus sites are band controls; this plugin's is a
   sub-oscillator mix level, and the root back-translated as "Bass". Corrected per-entry
   here with a reasoned `termNote` and **not forked silently**, but the root itself needs a
   glossary-level decision — either a second accepted member for the oscillator sense, or a
   scope note saying the root is band-only. **Grep-able token: `'sub':` in
   `scripts/i18n-zh-glossary.js`.**

4. **`.tour-caption`'s reserved flex line is now a TWO-SITE pattern.** Task 1 recorded it on
   O-simpleAdditive (256.34px collapse, floored at a measured 311.52px) and this task
   records it on O-simpleSubtractive (258.3px collapse, floored at the 46% cap). Both are
   `.preset-tour` / `.tour-buttons` / `.tour-caption` with `flex-wrap: wrap` and a
   percentage `max-width`. **Every remaining plugin in the O-simple family with a preset
   tour will hit it.** The floor is cheaper expressed in the cap's own unit than as a
   measured pixel. **Grep-able token: `.tour-caption` with `max-width` in
   `plugins/*/Source/ui/public/css/styles.css`.**

---

## For Tasks 5–7

Scope the geometry pins under `html[lang="zh-Hans"]` rather than pinning globally unless
there is a reason not to: it costs nothing, it makes the fr arm green by construction, and
on this plugin it was the only way to pin a node the French deliberately lets wrap. It also
holds the `wrap-count` ESTIMATED denominator still, so there is no F8 divisor movement to
disentangle from a real change — the one number Tasks 1–3 all had to reason around.

Read each plugin's own `tests/i18n-states.json` for the state KIND before asserting on
rects: K16's "nineteen click states" is wrong by at least one (Task 3's `eval`), and it is
right here. Discriminate a `null` `elementFromPoint` on the target's **y against the frame
height**, not on the fact that it is null.

Check the corpus site count and what each site CONTROLS before accepting a glossary root:
this task's one re-authoring came from a root that is correct at both of its other sites and
wrong at this one. And expect the R3 denominator to differ from the plan's by roughly
2–3× — the planning screen matched captions only, on all four tasks so far.

---

## A note on the shared checkout — FALSE-1's mechanism, live twice in one task

The O-Strata untracked count moved **during this task**, which is the sharpest available
demonstration of why the precondition's `git status` line is not a baseline:

| moment | `git status --short \| grep -c "O-Strata"` |
|---|---|
| plan, written 2026-09-07 | **2** (two `.planning/` files) |
| this task's preconditions, before any edit | **0** (a concurrent session had committed them) |
| this task's self-check, after both commits | **4** (`CMakeLists.txt`, `Source/`, `tests/`, a gate report) |

Three different readings of one line inside one working period. **None of them is a
regression and none is a defect**; the number describes another session's progress on an
unrelated plugin. The load-bearing claim under it — O-Strata has no `i18n.js`, so both
repo-wide lints exit 2 with `0 finding(s) across 0 plugin(s)` and `1 plugin(s) could not be
read` — was re-fired at the start and again at the end and is **identical at both**.

Nothing under `plugins/O-Strata/` was ever staged. The commit discipline held it out by
construction: every commit was path-scoped to `plugins/O-simpleSubtractive`, and the branch
and staging were re-checked immediately before each rather than once at the start. Both
commits carry **0** files outside the plugin and **0** deletions.

---

## Self-Check: PASSED

Files claimed, verified on disk:

- `plugins/O-simpleSubtractive/Source/ui/public/js/i18n.js` — FOUND
- `plugins/O-simpleSubtractive/Source/ui/public/index.html` — FOUND
- `plugins/O-simpleSubtractive/Source/ui/public/css/styles.css` — FOUND
- `plugins/O-simpleSubtractive/Source/PluginProcessor.h` — FOUND
- `plugins/O-simpleSubtractive/CMakeLists.txt` — FOUND
- `plugins/O-simpleSubtractive/CHANGELOG.md` — FOUND

Commits claimed, verified in `git log --all`:

- `fff62ae8` — FOUND
- `09bb37cf` — FOUND

Scope claims, verified:

- files staged outside `plugins/O-simpleSubtractive/` across both commits — **0**
- paths staged under `plugins/O-Orbit/libs/SAF` — **0**; submodule still at `b6fe1882`
  (v1.3.4), clean
- tags created — **0**
- `PLUGINS.md` touched — **no** (Task 7 owns it)
- `ROADMAP.md` / `STATE.md` / `PLAN.md` / this summary committed — **no**
