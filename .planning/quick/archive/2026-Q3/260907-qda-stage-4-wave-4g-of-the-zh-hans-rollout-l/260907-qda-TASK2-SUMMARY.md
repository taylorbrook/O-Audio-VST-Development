---
task: 2
plugin: O-simpleGrain
phase: quick-260907-qda
wave: 4g
status: complete
shipped_version: "1.5.0"
shipped_version_code: "0x010500"
bumped_from: "1.4.3"          # the CMakeLists VALUE via the two-arm reader, not the 1.4.2 PLUGINS.md says
rows: 153
reviewed: bt
requirements: [ZH4G-02, ZH4G-05, ZH4G-06, ZH4G-08]
commits:
  - 8ee82be2   # the 'mt' table, the body correction, the font tail, the codec, the pins and floors
  - 8b5d7a70   # promotion to 'bt', BOTH version sources, CHANGELOG
false_predictions: 4
corpus_after: "4905 of 5441 rows, 39 plugins carrying zh"
---

# Task 2 — O-simpleGrain 1.4.3 → 1.5.0, Simplified Chinese

153 rows — the wave's largest table — at `reviewed: 'bt'`, shipped as **1.5.0**,
built and installed. Two path-scoped commits. Every gate green. `auval`
deliberately not run; it is Task 7's single cold sweep.

---

## The two-arm reader pair, recorded as a measurement

This is the wave's answer to instruction 5 and to wave 4f's report that the
`set()`-variable CMake shape was absent. It is not a rule restatement — both
arms were fired, on the tree as found and again on the shipped value:

| | one-arm reader | two-arm reader |
|---|---|---|
| **before the bump** | **`EMPTY`** | **`1.4.3`** |
| **after the bump** | **`EMPTY`** | **`1.5.0`** |

```cmake
plugins/O-simpleGrain/CMakeLists.txt:9   set(OSIMPLEGRAIN_VERSION "1.5.0")
plugins/O-simpleGrain/CMakeLists.txt:24      VERSION "${OSIMPLEGRAIN_VERSION}"
```

A reader that matches a literal after `VERSION` matches the `${…}` and extracts
nothing — and it fails **silently**, returning an empty string rather than an
error. K5 reproduces exactly. **The one-arm reader is still EMPTY after the
bump**, which is the part worth carrying: the failure is a property of the file
shape, not of the value, so it will not announce itself on any later release
either.

The registry row said **1.4.2**; the CMakeLists said **1.4.3**. K4 holds — the
stale patch is the 2026-09-03 suite-wide French hover-help rename. **The bump
was taken from the CMake value** (R9). Task 7 owns the registry correction.

## The hex mirror — both version sources moved in one edit (K6)

```
plugins/O-simpleGrain/CMakeLists.txt:10                       set(OSIMPLEGRAIN_VERSION_CODE 0x010500)
plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt:57  JucePlugin_VersionString="${OSIMPLEGRAIN_VERSION}"
plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt:58  JucePlugin_VersionCode=${OSIMPLEGRAIN_VERSION_CODE}
```

All five `OSIMPLEGRAIN_VERSION` sites confirmed before either was edited.
`0x010403` → **`0x010500`**, and two things about how:

1. **The new value was DERIVED from the `0x00MMmmpp` encoding, not transcribed**
   — the bump script computes it from the version triple it is writing.
2. **The old value was verified to actually mirror 1.4.3 before it was moved.**
   The script aborts if it does not. Moving an already-drifted mirror silently
   would hide the drift it exists to prevent.

Arithmetic probe on the shipped file: `version 1.5.0 -> expects 0x010500, hex
reads 0x010500 : IN SYNC`.

**Task 1's FALSE-2 warning was followed rather than K6's wording.** K6 says no
other plugin in the six has a second version source; Task 1 measured that
O-simpleAdditive's harness carries a *frozen literal* instead. So the harness was
grepped for `JucePlugin_Version` and the finding classified rather than assumed:
this one is a live `${VAR}` reference and MUST move with the bump. It did.

---

## False-prediction ledger — four, marked per plan prediction

### FALSE-1 — precondition (i): `git status --short | grep -c "O-Strata"` reads **0**, not 2

Task 1's FALSE-1, re-confirmed. A concurrent session had already committed both
O-Strata planning files. The untracked set today is this wave's own plan
directory alone.

**K2's load-bearing claim is unaffected and re-fired live:** O-Strata still has
no `i18n.js`, and both repo-wide lints still exit 2 with `0 finding(s) across 0
plugin(s)` and `1 plugin(s) could not be read`. The finding count and the
unreadable count are the baseline; the exit code and the `git status` line never
were.

### FALSE-2 — precondition (j): the positive control CANNOT fire before the table lands

The precondition asks for zero Han under `Source/` "with the positive control on
`js/i18n.js` firing". Pre-work, `js/i18n.js` holds **0 Han code points** — the
plugin is not localized yet — so the control necessarily prints nothing, and a
green here would have meant the probe was broken. Verified the probe itself was
live by firing it against a shipped three-language file
(`plugins/O-Detune/.../i18n.js` → `control fired`).

**The control's correct home is the VERIFY step, not the precondition**, and
there it fired: negative printed nothing across every `.h`/`.cpp` under
`Source/`, control printed the `i18n.js` path.

### FALSE-3 — the R3 glossary screen returns **1 collision**, not NONE, and the denominator is 76 not 27

The plan records `NONE (27 matched)` at planning time. Re-fired here over
captions **and** tooltip titles: **76 glossary-matched, 1 collision** —

```
COLLISION root 颗粒  [["label.readoutGrains","Grains"],["label.groupGrain","Grain"]]
```

The denominator difference is the same shape Task 1 recorded as its FALSE-3 (the
planning screen matched captions only). **The collision is new information**, and
it is real: the glossary maps `'grain'` and `'grains'` to one root deliberately —
Chinese has no plural inflection — so a page carrying both a Grain group heading
and a Grains readout key paints the same two characters twice for two different
things. English distinguishes them by a single `s`.

Resolved by qualifying the side that is ambiguous about what it does (below).

### FALSE-4 — the verify block's `wrap-count 0 (baseline 0)` cannot hold once Chinese lands

Final `wrap-count`: **3 findings**, all one shape —

```
span.viz-hint  "each grain scatters as a dot (read-posit…"   en=2 line(s), zh-Hans=1 line(s)
span.viz-hint  "playhead, freeze pin & spray range"          en=2 line(s), zh-Hans=1 line(s)
span.viz-hint  "discrete sidebands at scatter 0 → noise…"    en=2 line(s), zh-Hans=1 line(s)
```

**The baseline 0 was an en-vs-fr number** — French also takes two lines on all
three. Chinese is shorter and fits one. That is the language, not a defect, and
it is not removable without padding the Chinese artificially.

**The geometric consequence is fully absorbed and separately proved:** each of
the three sits in a `.viz-label` row now floored at the exact measured English
box, and `check-ui-labels` reports **0 non-label elements moved** on the zh arm
across all four states. The load-bearing criterion (N1) is the moved count, and
it is zero. `wrap-count` here is a true observation with no pixel consequence.

Also worth recording against F8: the ESTIMATED denominator moved **70 → 37** as
the pins made half the leaves measurable, so the divisor changed with no pixel
changing — exactly the effect F8 warns about, live.

---

## Measured numbers, before and after

### Geometry — `check-ui-labels`, zh arm, across all four states

| | non-label elements moved | assertions 5 / 6 |
|---|---|---|
| table landed, no pins | **152** | FAIL 13 spills / FAIL 14 frame crossings |
| after 9 line-box pins + 3 reserved-line floors | **6** | PASS / PASS |
| after the readout width floor | **0** | PASS / PASS |
| after the round-1 re-authoring and promotion | **0** | PASS / PASS |

**The fr arm was green at every one of the four measurements**, and coverage never
moved off `56 of 54` visible with **0 never-visible**. `56 of 54` is F18's shape
and is not a failure — N counts paths seen across the cumulative walk, M is
final-DOM membership.

### The nine line-box pins, every ratio derived from that element's own English CONTENT box

Rect height less its own padding and border, divided by its own font-size, line
count verified as exactly 1. **None of the eleven selectors carried a
`line-height` or `min-height` declaration before this block** — grepped one by
one (R8/F7).

| selector | fs | en rect | pad t/b | border t/b | content | ratio |
|---|---|---|---|---|---|---|
| `.viz-label` | 10 | 11 (per line) | 0/0 | 0/0 | 11 | **1.1** |
| `.keyboard-label` | 10 | 11 | 0/0 | 0/0 | 11 | **1.1** |
| `.settings-label` | 11 | 12 | 0/0 | 0/0 | 12 | **1.0909091** |
| `.group-title` | 12 | 19 | 0/3 | 0/1 | 15 | **1.25** |
| `.window-inset .inset-label` | 8 | 9 | 0/0 | 0/0 | 9 | **1.125** |
| `.grain-readout .readout-key` | 9.5 | 10 | 0/0 | 0/0 | 10 | **1.0526316** |
| `.preset-bar-tour .tour-btn` | 9.5 | 20 | 4/4 | 1/1 | 10 | **1.0526316** |
| `#btnLoad` | 10.5 | 25 | 6/6 | 1/1 | 11 | **1.047619** |
| `#toggle-freeze` | 10 | 29 | 7/7 | 2/2 | 11 | **1.1** |

**Three are the F6 form-control case, live.** `#btnLoad`, `.tour-btn` and
`#toggle-freeze` are all buttons whose UA `font` shorthand resets `line-height`;
M8's leaf table would have given 10.5px → 1.1666667 for `#btnLoad` against its
measured 1.047619, and moved English. **`.group-title` is the padding case:** its
rect is 19 but its content box is 15, because the rect includes 3px of bottom
padding and a 1px border. A ratio taken from the rect would have been 1.5833 and
moved English on every group heading on the page.

`.group-title` lands on the same 1.25 Task 1 measured for the same class at the
same size — an independent second site for that number.

### The three reserved-line floors and the width floor — the N9 shape, twice

**Reserved second caption line.** Chinese is shorter, so each of three viz hints
fit one line where English takes two; the label row lost a line and the canvas
below grew into it:

| | en | zh, before the floor |
|---|---|---|
| `div.viz-label` (cloud / wave / spectrum) | **22** | 14 |
| `div.canvas-wrap` | 180 | **188** |
| `#cloudCanvas` | 176 | **184** |

`min-height: 22px` — the exact measured English box — on those three cells only.
**The Output Scope cell is deliberately excluded:** its English pair is one line
by design (the v1.4.1 note records the French caption being shortened to buy
exactly that), it measures 11px, and a floor there would have moved the Latin
arms.

**The readout key, and it is the *opposite* direction to every other floor in
this wave.** The Grains key is the one place Chinese came out WIDER:

| | en | fr | zh (4-char) | zh (3-char, shipped) |
|---|---|---|---|---|
| `#readoutGrains` key width | **41.38** | 41.38 | **42.84** | floored to 41.38 |
| `#readoutOverlap`, `#cpuBar` x-shift | — | — | **+1.5 px** | 0 |

The three readout keys sit in one left-aligned flex strip, so the width of any
key positions everything to its right — the same mechanism the existing
`min-width: 91px` French-era pin on the Overlap key exists for. Fixed by trimming
the rendering to three characters and flooring at the exact measured English box.
**Not rounded, and not a fixed width:** a fixed width would have moved French.

### `measure-ui` screens — the K12 vacuum, closed

| screen | baseline (vacuum) | after |
|---|---|---|
| `undeclared-font` | 0 — **input EMPTY** | **0 against 100 visible Han-bearing nodes**, 0 of which resolve without a CJK face |
| `line-height-normal` | 0 — input empty | 33 → **2**, both named below |
| `wrap-count` | 0 (en-vs-fr only) | **3** — FALSE-4 above, 0 moved |
| `svg-font-attr` | 0 carriers | 0 carriers |
| identity | 502 nodes / 251 keys / 122 ids | 756 / 252 / 122 (a third language pass) |
| leaves ESTIMATED | 70 of 90 | **37 of 90** |

Every zero after the table landed is a **measurement**. Every zero before it was
a vacuum.

**The two `line-height: normal` residuals, each named with its measurement (N1):**

| node | fs | enH | zhH | verdict |
|---|---|---|---|---|
| `p.subtitle` | 11px | **24.19** | **24.19** | EQUAL — cannot move, cannot be pinned to any effect |
| `#help-toggle` | 10px | **24** | **24** | EQUAL — a fixed-height form control |

### The state-EFFECT assertion — all FOUR click states, in all three languages (N1, K16)

The wave has no `eval` states; all four here are clicks.

| state | target | rect | `elementFromPoint` | took effect |
|---|---|---|---|---|
| settings popover OPEN | `#gear-btn` | 22×22 @851,19 | target or descendant | **YES** — visible `[data-i18n]` 51 → 54 |
| hover help OFF | `#help-toggle` | 100×24 @762,87 | target or descendant | **YES** — face flips (`On`→`Off` / `Activée`→`Désactivée` / `开启`→`关闭`) |
| Async Cloud lesson | `.tour-btn[data-preset="Asynchronous Cloud"]` | 110.5×20 @497.5,45 | target or descendant | **YES** — 54 → 51, caption text changes |
| Pitched Buzz lesson | `.tour-btn[data-preset="Pitched Buzz"]` | 110.5×20 @497.5,19 | target or descendant | **YES** — caption text changes |

**Twelve assertions (4 states × 3 languages), no `null` rects**, so there was
nothing to discriminate as a scrolled target and no coverage hole.

### The fixture's French figure — RE-MEASURED, and it is stale by 183.91 px

`tests/i18n-states.json` names *"997.22 px natural NOWRAP"* for
`label.captionPitchedBuzz`. Re-measured on an offscreen clone (`scrollWidth` on
the live node is clamped to its 846px box and returns 846 for all three
languages — a false reading):

| | natural NOWRAP | box | lines | slack |
|---|---|---|---|---|
| en | 606.84 | 846 × 13.19 | 1 | 239.16 |
| **fr** | **813.31** | 846 × 13.19 | 1 | 32.69 |
| zh-Hans | 656.08 | 846 × 13.19 | 1 | 189.92 |

The fixture figure is explicitly a *pre-v1.4.2* number, and v1.4.2's reweording
brought it down. **A fixture that mirrors a measurement drifts silently** — this
one had, by 183.91 px, and inheriting it would have made the French slack look
like −151 px rather than the +32.69 px it is.

---

## Blind-read accounting

| | |
|---|---|
| rows dispatched | **153 of 153** (76 + 77), 0 withheld from review |
| chunks | 2, split on the CONCEPT (F17), each from its own emit → own 16-hex salt |
| salts | `538346d3389b5207d75f4ac178bf0e92` / `a645a674bd0e7b75d8099f4442282802` / round 2 `ac10af92dd9a0d3562be7df20587be26` |
| ids shared between chunks | **0**, measured, all three pairings |
| ids returned identical AND IN ORDER (M12) | **yes**, all three batches, diffed |
| Han surviving into returned English | **0** |
| malformed returned rows | **0** |
| dispatched zh column vs the COMMITTED tree | **byte-identical** (`git diff HEAD` empty at dispatch time) |
| product-name control | **0** on both chunks |
| triples read with `--verbose` (R2) | **153 of 153**, plus 12 in round 2 |
| rows re-authored | **1** |
| correction rounds | **2**; round 2 corrected nothing further, so no round three |
| forward model | `claude-opus-5` |
| round-1 reverse model | `claude-sonnet-5` — read back from the process, not the alias |
| round-2 reverse model | `claude-haiku-4-5-20251001` — a **third** model, fresh salt, fresh session (R4) |
| refusal controls fired | **2 of 2**, against a properly provenanced emit (F10) |
| termNote exemptions | **7** across 4 terms, entry-scoped (N4) |

**Both refusal controls, verbatim:**

1. `--provenance` omitted →
   `REFUSED: back-translation provenance is missing or identical to the forward pass — this triple proves nothing` / `  no --provenance was given.`
2. wrong-side `--manifest`, **with** `--provenance` →
   `WRONG MANIFEST FOR THIS BATCH: all 76 returned ids are unjoinable, not one.`

`--forward-provenance` was passed a real STRING (F9), naming the model, the date,
the task and the commit the Chinese was read from.

### The one re-authoring, and why (collision on the page, never drift distance)

**`readout` body.** It named the readout control **颗粒数量** while the readout key
beside it paints **颗粒数** — the caption had been trimmed to three characters
during the geometry pass and the body did not follow. **A body must name the face
the page paints**; this file's own v1.4.1 note settled that rule for French ("A
FRENCH BODY MUST NAME THE FRENCH FACE", ten bodies corrected). Round 2, on a
third model, read the corrected body back as *"Grain count = how many active
grains of the global limit of 192"*, matching the caption's own *"Grain Count"*.

**This one is only visible because two things were compared that no tool
compares** — a tooltip body against a caption that a *geometry* fix had changed
hours earlier. No gate in this repo reads a zh body against its own en, and none
reads a body against a sibling caption.

### The concept split paid for itself twice

**1. Granular Fire — the glossary root is wrong, and two readers say so.**

| chunk | reader | id | en′ |
|---|---|---|---|
| A (titles + bodies) | `claude-sonnet-5` | `title\|lessonGranularFire` | **Granular Fire** (1.00) |
| B (captions) | `claude-sonnet-5` | `label\|label.tourGranularFire` | **Grain Fire** |
| C (round 2) | `claude-haiku-4-5` | both of the above | **Grain Fire** |

The shipped glossary root is **颗粒触发** — *granular TRIGGER*, reading "Fire" as
the verb. This preset is the worked example on the crackling-fire recording, and
the page's own body says so. **The site count was checked before the note was
written** (N4): `'granular fire'` has exactly **one** corpus site — this page —
and the root **颗粒触发 is shipped nowhere**, so it was derived from a single
English caption with no second site to check it against. That is D1's shape
exactly. Corrected to **颗粒之火** per entry with a reasoned note, flagged for the
glossary rather than forked silently. **Three readings, three models-or-salts,
all three the flame.**

**2. Taper — one string, two readings, and the pair is what settles it.**

| chunk | what the reader had | en′ for 渐变 |
|---|---|---|
| A | the tooltip title **with its body** (which explains the fade) | **Taper** (1.00, exact) |
| B | the bare caption, no context | **Gradient** |
| C (round 2, third model) | both, no context | **Gradient** |

A single reader returns one of those and nothing to check it against.
**渐变 is the shipped corpus root for `'taper'`**, and the only reader that had
the fade context returned the English exactly. There is no second gradient-like
control on this page to collide with. **Accepted, not forked** — recorded as a
glossary-level observation below.

### The R3 collision, resolved — and the blind read confirms the resolution worked

Two qualifications were made, both under the R3 arm that qualifies *the side
that is ambiguous about what it does* and writes down why the other was left:

| key | root | shipped | why the other side was left |
|---|---|---|---|
| `label.readoutGrains` "Grains" | 颗粒 | **颗粒数** ("grain count") | `label.groupGrain` keeps the bare root: it names the group of grain controls, which is what the root means |
| `label.knobScatter` / `scatter` "Scatter" | 散布 | **时间散布** ("time scatter") | `label.groupSpray` keeps the bare root as the family heading, exactly as the French keeps *Dispersions* |

**Scatter's qualification is the load-bearing one.** The three Spray knobs carry
the same root as their tail morpheme (音高散布 / 位置散布 / 声像散布) *and* the bare
root is the group heading directly above them — so an unqualified caption would
have been a fourth spray by shape and identical to its own group heading by
string. Both blind readers, on different models and salts, returned it as **Time
Spread** and **Time Scatter**, distinct from **Position Spread** on the same
page. The qualification did what it was for.

### Accepted drifts, each with its written reason

- **`label.btnLoad` 载入… → "Loading…"** while `toast.loading` 正在载入 {name}… →
  "Loading {name}…". Two keys, one English reading — but **on the page they are
  distinct**: 正在 is the progressive marker and the button face has none. The
  button is the glossary root verbatim. The collapse is in the English, not the
  Chinese.
- **`label.captionFragments` "between a single grain and a smooth cloud"** — one
  reader took the phrase generically, the other as the preset names 单颗粒 and
  平滑云团. **Both readings are true of the page**: the Fragments preset does sit
  between those two presets. English has the same ambiguity modulo case, which
  Chinese does not have.
- **`title|readout` 颗粒读数 → "Grain Reading"** (haiku) / "Grain Readout"
  (sonnet). Same referent.
- **`position` body 搜索 → "search through the recording"** where the English says
  *scrub*. The sentence carries the mechanism (sweep the control to move through
  the recording) and nothing on the page collides with it.
- **`vizCloud` body 棕褐色 → "tan-colored"** where the English says *sepia*. 棕褐 is
  the sepia/umber word.
- Every remaining 1.00 triple is exact, including the ones that matter for W3:
  **Fire / Voice / Water / Piano and Hann / Gauss / Tukey / Rectangular survive
  verbatim into the Chinese bodies** — they are `AudioParameterChoice` strings the
  user reads on the combo and stay English under D-01, in both directions.

---

## The unscanned module — MEASURED, NAMED and REPORTED UNCHANGED (K8, s71 D2)

`plugins/O-simpleGrain/Source/ui/public/modules/webview-drop-streaming.js`, 504
lines.

| | |
|---|---|
| **sha1** | **`c7fe612ee5d3701381e9efc764480e79f12d377e`** — matches the plan's `c7fe612e…` |
| **sha256** | **`349a0c285ababd7fc7741855f1d908c70dbf979d9c135073cc2bcff837f2a9cd`** |
| byte-identity with O-simpleSampler's copy | **CONFIRMED** — identical sha1 *and* sha256, and `diff -q` reports no difference |
| `check-i18n` `[12]` on this plugin | **`1 module(s): js/app.js`** — quoted verbatim, before and after |
| `git status --short -- .../modules/` | **0** — not one byte changed |

**The count is 17 distinct English strings across 22 `showToast` call sites** —
more call sites than the plan's "~12", because the plan counted distinct strings
and several are emitted from two places. Named, with line numbers:

```
:184  'Drop a single file on a cell, or a folder on the top zone.'
:195  'Drop a .wav/.aif on a cell'
:200  'Drop a folder, not a file'
:206  `Drop failed: ${err…}`
:230  'Scanning folder…'
:235  'No audio files in folder'
:252  'Folder load dialog failed — aborted'
:271, :274, :353, :356   'Drop session start failed'
:284  `Loading ${i+1} of ${all.length}: ${entry.name}`
:290  `Skipped: ${entry.name} (backend rejected)`
:295, :366  `Skipped: ${entry.name} (read failed)`
:302  'No samples loaded — all files failed'
:319  (a two-armed template)
:335  'Folder load failed at commit step'
:358  `Loading ${fileEntry.name}…`
:376, :379  'File transfer failed'
:386  'File load failed at commit step'
```

**Verdict: REPORTED, NOT FIXED, and the reason is written down.**

1. **No gate in this repo can see them.** `check-i18n`'s `[12]` clause scans
   `js/app.js` only — **the discriminator is the DIRECTORY**, files under `js/`
   are scanned and files under `modules/` are not — so its PASS is a claim about
   the scan and not about the page. The strings are not `[data-i18n]` elements,
   and no state in `i18n-states.json` fires a drop event.
2. **They are not a regression this wave causes.** They render English on the
   French page today and have since the module was written.
3. **A fix is a two-plugin change.** The file is byte-identical to
   O-simpleSampler's copy (Task 3), so keying it would put an untested toast
   layer with no calibrated gate inside a commit whose subject is a caption
   table. **s71 took exactly this narrowing arm on `preset-manager.js` for
   exactly these reasons.**
4. **No partial keying was added** (wave 4c D4). A half-keyed toast layer is
   worse than an unkeyed one because it looks done.

**Task 3 inherits:** the sha1 `c7fe612e…` and sha256 `349a0c28…` above are the
identity to verify (do not assume it), and the same verdict applies — the
`git status --short -- .../modules/` probe must read **0** on that plugin too.

---

## Stale-body and dead-class verdicts (K13, C7)

| class | verdict | evidence |
|---|---|---|
| the fixed-pair language enumeration | **LIVE, deleted in en AND fr** | comment-stripped `-CSD` probe **2 → 0** |
| its numeric exception list | **KEPT, re-verified** | markup carries **3** `<select>`; minus the language selector = **2**; *"the two drop-down menus"* is true today |
| the "and nothing else" settings class | **NO SUBJECT — checked non-defect** | `grep -c "'tip.settings'"` = **0**; `grep -c "tips-toggle"` in markup = **0** |
| the "Tuning tab stays English" class | **NO SUBJECT — checked non-defect** | not a `scala-tuning-engine` consumer (grep 0 in CMakeLists); no tuning tab |

The correction was made FIRST and the file **re-read** before a single Chinese
row was authored (C7) — the zh body was written from the corrected English, not
from the English the plan quoted. The superseded phrasings are in the CHANGELOG
and are **not** respelled in any source comment (C8), which is why the probe
reads the file as fixed.

---

## Zeros recorded AS MEASURED

- **gate files: 0.** `find plugins/O-simpleGrain/tests -name '*.js'` returns
  nothing; `tests/` holds `i18n-states.json` and a C++ `render-harness/`. Fired
  with `find`, never a glob. **No gate was invented** (C5/C6 has no subject).
- **duplicate keys: NONE**, F13-corrected form (LANGUAGES members excluded),
  `keys=80`, before and after.
- **language `AudioParameterChoice`: none.** The two on this plugin are
  `sourceSample` and `windowShape`; their option strings stay English under D-01
  and are named in English inside the Chinese bodies (W3, both directions).
- **Han under `Source/`: 0**, negative grep across every `.h`/`.cpp`, with the
  positive control firing on `js/i18n.js` in the same run.
- **`zh-Hans` in the comment-stripped `PluginProcessor.h`: 2** (encode and
  decode), and both functions report **THREE-WAY** when matched by name.
- **submodule untouched.** `git status --short -- plugins/O-Orbit/libs/SAF` →
  empty; `git submodule status` → ` b6fe1882… (v1.3.4)`, leading space = clean.
  The guard ran before **both** commits even though no path here is inside it.
- **tags created: 0.** **`PLUGINS.md` touched: no** (Task 7 owns it).
- **file deletions across both commits: 0.**
- **N3, the `--symbol-font` verdict:** its two carriers are `.gear-btn` and
  `.fleuron-row, .fleuron-corner, .preset-fleuron, .op-out`. **`#gear-btn` is
  TIP_BINDINGS row 1** — a tip anchor, and `measure-ui`'s `han` counts
  `data-tip` / `data-tip-title` / `aria-label`, not the glyph. The tail was
  added to the token itself, covering both carriers.

## Font work — the tail half only, and where it lives

The census must search the PAIR: `index.html` declares **zero** `font-family`
sites on this plugin; all eleven are in the external 1129-line `css/styles.css`.
A markup-only census reads a false clean bill.

```
1  font-family: 'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif
8  font-family: inherit
2  font-family: var(--symbol-font)
```

Two declarations edited: the house stack (`:80`) and the `--symbol-font` token
(`:65`). Times New Roman is installed and 4th in the stack, so the Latin arm was
already safe and this is the **tail half only**; the absent Garamond members
were **kept** — Windows and print intent. The tail goes **before** the trailing
generic (W1). Verified by **re-running the screen**, never by reading the CSS
back: `undeclared-font: 0` against **100** visible Han-bearing nodes, and a
scripted check that **0 of the 100** resolve without a CJK face.

## Gate results

| gate | result |
|---|---|
| `check-i18n --plugin` | **exit 0** — `got ["en","fr","zh-Hans"]`, `38 I18N + 77 LABELS` unchanged, `38 tip(s) bound`, `[12] 1 module(s): js/app.js` |
| `i18n-zh-lint --plugin` | **0 findings**, 115 entries, **BELOW SHIP BAR 0** |
| `i18n-zh-lint --self-test` | **10/10** |
| `check-ui-labels --plugin` | **exit 0**, `== ALL CHECKS PASSED ==`, 10 green geometry-diff assertions across both non-English arms |
| `boot-all-uis --strict-tips` | **clean 1/1**, 0 warn, 0 failed, **0 DEAD, 0 late** |
| `measure-ui --verbose` | **0** unresolved or skipped states — all four applied |
| `i18n-zh-lint` repo-wide | 0 findings across 0 plugins, 1 unreadable — exit 2, the **K2 baseline**, unchanged |
| `i18n-fr-lint` repo-wide | 0 of 44 with findings, 1 unreadable. **No French rendering changed** |
| `check-i18n` repo-wide | `ALL CHECKS PASS — 43 localized plugin(s)` |
| **corpus** | **4752 → 4905** of 5441 rows; **38 → 39** plugins carrying zh |

`auval` **not run** — Task 7's single cold sweep. Budget ~85 s, not the stale
15-minute figure.

---

## Commits

| hash | subject |
|---|---|
| **`8ee82be2`** | `i18n(O-simpleGrain): add Simplified Chinese at reviewed:'mt' — 153 rows, the wave's largest table` |
| **`8b5d7a70`** | `i18n(O-simpleGrain): promote 153 zh rows to reviewed:'bt' after a two-round blind reverse read — v1.5.0` |

Both path-scoped `git commit -F <file> -- plugins/O-simpleGrain`, options before
the `--` (F14.6). `git branch --show-current` and `git status --short` re-checked
immediately before each, never once at the start. The submodule guard ran before
each. Nothing under `.claude/agent-memory/`, nothing under `.planning/`,
nothing outside `plugins/O-simpleGrain/` was ever staged — verified after the
fact: **0 files outside the plugin across both commits, 0 deletions.**

## Build and install

`./scripts/build-and-install.sh O-simpleGrain`, backgrounded and polled on a
sentinel — **exit 0, 54 s.** The folder name equals the `juce_add_plugin` target
here (`O-simpleGrain`), so this is not one of the two plugins where it does not.

```
VST3: ~/Library/Audio/Plug-Ins/VST3/O-simpleGrain-dev.vst3            7.7M, age 0s
AU:   ~/Library/Audio/Plug-Ins/Components/O-simpleGrain-dev.component 7.7M, age 0s
```

`CFBundleShortVersionString` read back from the installed AU bundle: **`1.5.0`**
— the version reached the binary, not just the CMakeLists.

**No alternate-variant orphan.** Phase 4's dual-variant sweep emitted no
`⚠ Sweeping ALTERNATE-variant` warning; only the `-dev` bundles are on disk.

---

## Deferred items

1. **The unscanned drop-streaming module ships English toasts on two plugins'
   Chinese pages.** 17 distinct strings, 22 call sites, byte-identical copies in
   `plugins/O-simpleGrain/` and `plugins/O-simpleSampler/`. No gate in this repo
   opens either. Keying it is a rollout of its own: it needs a calibrated gate
   for an unkeyed toast layer, and the fix must land on both copies at once.
   **Grep-able token: `opts.showToast('Scanning folder…')`.**
   **Related, and worth a suite-wide sweep:** `check-i18n`'s `[12]` module scan
   is keyed on the `js/` directory, so **any** page JS under `modules/` or
   `js/modules/` is invisible to it on all 44 plugins. Widening that scan is the
   root fix; the four files K8 names are only the ones this wave looked at.

2. **`'granular fire' → 颗粒触发` is wrong in the glossary and needs a
   glossary-level decision.** The root reads "Fire" as the verb *to trigger*;
   this page's own tooltip body names the crackling-fire recording. **Exactly one
   corpus site** (this plugin) and shipped nowhere, so it was derived from a
   single caption with nothing to check it against. Corrected per entry here to
   颗粒之火 with a reasoned `termNote`, not forked silently. Three independent
   blind readings all returned the flame.
   **Grep-able token: `'granular fire':` in `scripts/i18n-zh-glossary.js`.**

3. **`'load your own' → 载入自己的` is a dangling modifier and has TWO English
   sites.** The root ends on a possessive with no head noun — grammatically
   incomplete in Chinese. Rendered 载入自己的素材 here with a `termNote`.
   **O-simpleSampler carries the same English (Task 3) and will hit it too.**
   **Grep-able token: `'load your own':` in `scripts/i18n-zh-glossary.js`.**

4. **`'taper' → 渐变` reads as "Gradient" to a caption-only reader.** Two of three
   blind readings returned *Gradient*; the one reader that also had the tooltip
   body returned *Taper* exactly. **Accepted, not forked** — it is the shipped
   corpus root and no second gradient-like control exists on this page to collide
   with. Recorded because any plugin pairing a Taper caption with a gradient
   control would have a real collision.
   **Grep-able token: `'taper':` in `scripts/i18n-zh-glossary.js`.**

5. **The `tests/i18n-states.json` state names carry MEASUREMENTS in their prose,
   and one of them is 183.91 px stale.** The Pitched Buzz state name records
   *"997.22 px natural NOWRAP"*; the real figure today is **813.31 px**. The
   number is a description, not an assertion, so nothing fails — which is exactly
   why it drifted. Every wave that inherits it inherits a wrong number.
   **Grep-able token: `997.22 px natural NOWRAP`.**

6. **`check-i18n`'s repo-wide `43 localized plugin(s)` and the emitter's plugin
   count are different denominators (K1).** After this task they read 43 and 39.
   The close-out must state both readings side by side; quoting the 43 alone
   would quote a number that was already 43 before a byte of this wave's work.

---

## A note on the shared checkout

No concurrent commit landed between this task's two commits this time (unlike
Task 1's, which had `0d443637` land between them). The discipline was applied
identically regardless: branch and staging re-checked immediately before each
commit rather than once at the start, every commit path-scoped, and
`.claude/agent-memory/research-planning-agent.md` — still modified, still another
session's file — never staged.

---

## Self-Check: PASSED

Files claimed, verified on disk:

- `plugins/O-simpleGrain/Source/ui/public/js/i18n.js` — FOUND
- `plugins/O-simpleGrain/Source/ui/public/index.html` — FOUND
- `plugins/O-simpleGrain/Source/ui/public/css/styles.css` — FOUND
- `plugins/O-simpleGrain/Source/PluginProcessor.h` — FOUND
- `plugins/O-simpleGrain/CMakeLists.txt` — FOUND
- `plugins/O-simpleGrain/CHANGELOG.md` — FOUND

Commits claimed, verified in `git log --all`:

- `8ee82be2` — FOUND
- `8b5d7a70` — FOUND

Scope claims, verified:

- files staged outside `plugins/O-simpleGrain/` across both commits — **0**
- file deletions across both commits — **0**
- paths staged under `plugins/O-Orbit/libs/SAF` — **0**; submodule still at
  `b6fe1882` (v1.3.4), clean
- tags created — **0**
- `PLUGINS.md` touched — **no** (Task 7 owns it)
- `ROADMAP.md` / `STATE.md` / `PLAN.md` / this summary committed — **no**
- `modules/webview-drop-streaming.js` changed — **no**, on either copy
