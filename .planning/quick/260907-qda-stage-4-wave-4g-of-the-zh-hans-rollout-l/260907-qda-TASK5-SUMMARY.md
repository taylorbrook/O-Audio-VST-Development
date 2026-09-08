---
task: 5
plugin: O-Polystutter
phase: quick-260907-qda
wave: 4g
status: complete
shipped_version: "1.15.0"
bumped_from: "1.14.3"          # the CMakeLists VALUE via the two-arm reader, not the 1.14.2 PLUGINS.md says
rows: 133                      # NOT 134 — msg-delete-preset has no body (K3)
entries: 91                    # 43 I18N + 48 LABELS — one `reviewed:` flag per ENTRY, not per row
reviewed: bt
requirements: [ZH4G-05, ZH4G-08, ZH4G-10]
commits:
  - ad264e7e   # the 'mt' table, the body correction, the endonym, the font tail + form-4 stack, the codec, the four pins
  - ce5a6a8c   # promotion to 'bt', version bump, CHANGELOG
false_predictions: 7
re_authorings: 0
correction_rounds: 1
corpus_after: "5664 of 5789 rows, 43 plugins carrying zh"
---

# Task 5 — O-Polystutter 1.14.3 → 1.15.0, Simplified Chinese

133 rows at `reviewed: 'bt'`, shipped as **1.15.0**, built and installed. Two path-scoped
commits. Every gate green, **and the geometry closed in ONE round**. `auval` deliberately
not run — it is Task 7's single cold sweep.

---

## K3 — the entry with no body, confirmed BEFORE authoring and mirrored

`2 × I18N + LABELS` gives **134**. The emitter says **133**. The cause was confirmed on the
tree before a Chinese character was written, not inferred afterwards:

```
en entries with no body: ["msg-delete-preset"]
```

Exactly one, and all 42 others carry both halves. The file's own comment says why: the
delete-preset confirmation is the argument to `confirm()`, composed per preset with a
`{name}` token, so it is neither a tooltip nor an element. `LABELS` cannot hold it —
assertion 15 requires every LABELS key to be referenced by an element or a `setLabel()`
call, and this one never can be — so `I18N` takes it **with an empty body**, the only shape
that satisfies assertions 13 and 15 at once.

Its Chinese mirrors that shape: `{ t: '删除预设 {name}？', b: '', reviewed: 'bt' }`, with the
`{name}` token byte-identical. The shape probe reads the two lists as identical:

```
en no-body: ["msg-delete-preset"]
zh no-body: ["msg-delete-preset"]
SHAPE MIRRORED — correct
```

**The emitter reports 133 after the table landed and 133 after promotion.** A body on that
entry would have added a 134th row nobody asked for and moved a number the close-out
asserts.

---

## The 105-binding many-to-one tip map, read BEFORE authoring

`TIP_BINDINGS` binds **105 selectors to 42 `I18N` keys** (`43 I18N` keys total; the 43rd,
`msg-delete-preset`, is bound to nothing because it is not a tooltip). The density comes
from one generator: 21 of the rows are a `LANES.flatMap` over `[1,2,3,4]`, so **84 of the
105 anchors are four structurally identical lanes sharing one body each**.

That is what makes the many-to-one benign here and it was checked rather than assumed. The
file's own comment records why every row names a unique `id` rather than a class:
`applyI18n()` uses `document.querySelector`, which returns the first match in document
order, so a class selector would have written all four lanes' tips onto lane 1.

**The consequence for authoring is that a lane body must read naturally on lane 1 and lane
4 alike**, which means it must not name a lane number. Every lane body was written with the
demonstrative — 这条断续轨 / 本轨, *this track* — and none carries an index. The remaining
21 anchors are one-to-one.

---

## `modules/preset-manager.js` — MEASURED, NAMED and REPORTED UNCHANGED (K8, s71 D2)

`plugins/O-Polystutter/Source/ui/public/modules/preset-manager.js`, **406 lines**.

| | |
|---|---|
| **sha1** | **`0142c1daac569f0c5e45574f8a7df06ea38b1597`** |
| O-Orbit's copy | **`d15e751b9c8650088fb841a7e36b0ae05221da3c`, 447 lines** — the plan's sha, and it is **O-Orbit's**, not this one |
| divergence | **CONFIRMED by sha1 AND by line count.** Task 2's byte-identity verdict does NOT transfer; only its REASONING does |
| `check-i18n` `[12]` on this plugin | **`2 module(s): js/app.js, js/parameter-bindings.js`** — quoted verbatim, before and after |
| `git status --short -- .../modules/` | **0** — not one byte changed. Its last commit is still `cff295d9`, the AGPL header sweep |

**This plugin is where `check-i18n`'s blindness is provable in one reading.**
`js/parameter-bindings.js` **is** scanned; `modules/preset-manager.js`, one directory up
from it, is **not** — same plugin, same UI root, same build. **The discriminator is
demonstrably the DIRECTORY and not the file.**

### Its seven user-facing English strings, and which of them reach this page

```
:73, :163   'Default'          -> REACHES THE PAGE
:283        'Loaded Preset'    -> REACHES THE PAGE
:380        title="Previous preset"        -> DEAD here
:382        title="Next preset"            -> DEAD here
:383        title="Load preset from file"  -> DEAD here
:384        title="Save preset"            -> DEAD here
:381        >Default<  (in the innerHTML)  -> DEAD here
```

**The five `createPresetBar()` strings are dead, and that is measured, not assumed:**
`grep -c createPresetBar js/app.js` returns **0**. This plugin takes the explicit-DOM-refs
path at `app.js:461-472`:

```js
const presetNameText = document.getElementById('preset-name-text');
presetManager = new PresetManager({
    displayElement: presetNameText,
    prevButton: document.getElementById('preset-prev'),
```

so the module's `innerHTML` block at `:378-386` never executes. The markup keys the buttons
itself — `index.html:1221` carries `data-i18n-aria="aria.presetPrev"`, and all four aria
keys exist in `LABELS`. The module's own default selectors (`.preset-name`, `.preset-prev`)
do not even match this markup, which is a second independent confirmation.

### The `#preset-name` verdict the task asked for

`_updateDisplay()` at `:345-346` writes `displayElement.textContent = this.currentPreset`
into **`#preset-name-text`**, and that element **is NOT keyed** — it carries no
`data-i18n`, and `check-i18n` assertion 10 passes because its content is covered by an
`I18N_EXEMPT` entry with a stated reason:

```
['Default', 'a factory preset name — exempt under D-02, because the name IS the JSON filename']
```

**`'Loaded Preset'` at `:283` is NOT exempt and NOT keyed.** It is reachable only after a
successful file load, so no static scan sees it and no state in `i18n-states.json` fires
it. Carried as a deferred item rather than fixed.

**Verdict: REPORTED, NOT FIXED**, for s71 D2's own reasons, restated against this tree:
`preset-manager.js` has **eighteen consumers**; the two copies in this wave alone already
diverge by 41 lines and a different sha; no gate in this repo opens either; and a
half-keyed toast/label layer is worse than an unkeyed one because it looks done (wave 4c
D4). No gate was invented.

---

## The false-prediction ledger — seven, marked per plan prediction

### FALSE-1 — precondition (i): `git status --short | grep -c "O-Strata"` reads **4**, not 2

Tasks 1–4 recorded **0**. This task read **4** at its preconditions and **0** at its
self-check. Five readings of one line across one working period:

| moment | reading |
|---|---|
| plan, written 2026-09-07 | 2 |
| tasks 1–4 preconditions | 0 |
| task 4 self-check | 4 |
| **this task's preconditions** | **4** |
| **this task's self-check** | **0** |

None is a regression. The number describes another session's progress on an unrelated
plugin. Nothing under `plugins/O-Strata/` was ever staged.

### FALSE-2 — precondition (j): the positive control CANNOT fire before the table lands

Tasks 2, 3 and 4's FALSE-2, confirmed a fourth time. Pre-work `js/i18n.js` holds **0** Han
code points, so a control fired against it necessarily prints nothing and a green there
would mean the probe was broken. **The probe itself was proved live against a shipped
three-language file** (`plugins/O-Detune/.../i18n.js` → `control fired`).

**Its correct home is the VERIFY step**, and there it fired: the negative printed nothing
across every `.h`/`.cpp` under `Source/`, and the control printed this plugin's own
`i18n.js` path on the same run.

### FALSE-3 — the R3 glossary denominator is **69**, not 34, and the two hits are same-control

The plan records `NONE (34 matched)`. Re-fired over captions **and** tooltip titles:
**69 glossary-matched of 91 screened.** The same denominator difference Task 1 (22→50),
Task 2 (27→76), Task 3 (23→69) and Task 4 (16→41) recorded — the planning screen matched
captions only. **Confirmed on all five tasks.**

Two root-sharing pairs surfaced where the planning screen reported none:

```
root 概率  [["label.prob","PROB"],["probability","Probability"]]
root 随机  [["label.rnd","RND"],["pitch-rand","Random"]]
```

**Both are a caption and its OWN tooltip title** — `#laneN_probability` and
`#laneN_pitch_rand_enabled` — which the ship bar excludes by name. **Verdict after the
exclusion: NONE**, matching the plan. The mismatch is a denominator, not a finding.

### FALSE-4 — the emitter has **no product-name control** in this version

Task 3's FALSE-5, confirmed. `grep -cin "product" scripts/i18n-zh-backtranslate.js` returns
**0** — the control does not exist, so there is no count to adjudicate and no line to read.
The rule has no subject; **the substance still does**, so the check was made by hand:

- occurrences of `Polystutter` in the emitted zh column: **0**;
- the wordmark lives in a `.plugin-title` div at `index.html:1198` and is the first
  `I18N_EXEMPT` entry, with `PRODUCT_NAME` in `CMakeLists.txt` cited as the reason. **It
  never enters the table.**

There is no common-noun arm to adjudicate here: *polystutter* is not an English word, so
unlike O-simpleSampler's 采样器 there is no morpheme that could be both wordmark and noun.

### FALSE-5 — the verify block's `coverage still 97 of 97` cannot hold once a third arm walks

`check-ui-labels` now reports **`103 of 97`**, with **0 never became visible** unchanged.
This is F18's shape — `N` counts paths seen visible across the cumulative walk and `M` is
final-DOM `[data-i18n]` membership, so a third language pass adds paths without adding
members. **The plan named this plugin as one of only two EXACT fractions in the wave** and
used that as a property of the page; it is a property of the number of arms walked.
`0 never-visible` is the part that is a page property, and it is unchanged.

### FALSE-6 — the K2 / D5 baseline is GONE: both repo-wide lints now exit **0**, not 2

The plan's baseline is `exit 2`, `0 finding(s) across 0 plugin(s)`, **1 plugin could not be
read** — O-Strata having no `i18n.js`. Measured at this task's preconditions:

| | plan baseline | measured here |
|---|---|---|
| `i18n-zh-lint` repo-wide | exit 2, 0 across 0, 1 unreadable | **exit 0, 0 findings across 44** |
| `i18n-fr-lint` repo-wide | exit 2, 0 / 44, 1 unreadable | **exit 0, 0 / 44** |
| emitter plugin count | 37 carrying zh, 1 could not be read | **43 carrying zh, 0 unreadable** |

The cause is commit **`0e838512`**, a concurrent session's *"feat(O-Strata): Stage 1
foundation — fork of O-Prism v1.24.0"*. **O-Strata is a fork of a fully localized plugin,
so it arrived with a three-language `i18n.js` already at `reviewed: 'bt'`** — 375 rows at
the preconditions, 348 an hour later as that session trims it.

**Three consequences for Task 7's close-out, all load-bearing:**

1. **O-Strata is no longer the 44th plugin excluded by name.** It is a localized plugin
   with zh rows, so K1's *"43 of 43"* framing is stale. The correct assertion after Task 6
   is **44 of 44**.
2. **The corpus denominator is not 5441 and it is MOVING.** It read **5816** at these
   preconditions and **5789** an hour later — the same O-Strata trim. Any close-out
   arithmetic pinned to a constant will be wrong by the time it is written. **Read it
   live and state the reading with its timestamp.**
3. **The standing cause of both lints' exit 2 is gone**, so `exit 2` is no longer a
   baseline any task may inherit as green.

### FALSE-7 — `check-i18n` repo-wide FAILED at this task's preconditions, and passed at its self-check

Measured before any edit:

```
FAIL: [O-Strata (Source/ui/public)] [10] every LABEL text node sits inside a [data-i18n]
      element, or is I18N_EXEMPT — 21 uncovered: Sine @option | Triangle @option | ...
1 FAILED — 44 localized plugin(s)          exit 1
```

**Pre-existing, on another session's in-flight fork, and none of my paths.** Re-fired at the
self-check after both my commits:

```
ALL CHECKS PASS — 44 localized plugin(s)
```

That session fixed it in the interval. Both readings are recorded because either one alone
is misleading: a task that inherited the plan's `exit 0` would have reported a green it did
not measure, and a task that stopped at the first reading would have reported a failure it
did not cause.

---

## Measured numbers, before and after

### Geometry — `check-ui-labels`, zh arm, across all three states

| | non-label elements moved |
|---|---|
| table landed, no pins | **90 / 100 / 100 / 90** across the four walked passes |
| **after four line-box ratios** | **0** — `== ALL CHECKS PASSED ==`, exit 0 |
| after promotion and the version bump | **0**, re-run, unchanged |

**One round.** The **fr arm was green at every measurement** — 8 of 8 green
`GEOMETRY DIFF` assertions on the two non-English arms in the final run — and the
never-visible count never moved off **0**.

### The four line-box ratios, every one from that element's own English CONTENT box

Rect height less its own padding and border, divided per line by its own font-size, line
count verified as 1. **None of the four selectors carried a `line-height` or `min-width`
declaration before this block** — grepped one by one (R8/F7).

| selector | fs | en rect | pad t/b | border t/b | content | ratio | zh before |
|---|---|---|---|---|---|---|---|
| `.settings-label` | 11 | 12 | 0/0 | 0/0 | 12 | **1.0909091** | 16 |
| `.section-label` | 11 | 12 | 0/0 | 0/0 | 12 | **1.0909091** | 16 |
| `.preset-dropdown-item .factory-badge` | 8 | **11** | **1/1** | 0/0 | **9** | **1.125** | 13 |
| `.euc-column-header` | 8 | 9 | 0/0 | 0/0 | 9 | **1.125** | 11 |

**The badge is the padding case.** Its rect is 11 and its content box is 9, because
`padding: 1px 4px` puts 2px of vertical padding inside the rect. **A ratio taken from the
rect would have been 1.375** and would have made the Chinese badge 11px against English 9 —
the growth this pin exists to remove. The two 8px selectors land on the same 1.125 from
*different* rect heights, 11 and 9, which is the same shape Task 1 recorded for
`.group-title` and `.drawbar-title`.

**F6 has no subject on this page and that is a measurement.** The task predicted this page
would be dense with buttons needing form-control ratios. It is dense with buttons — 105 tip
anchors — but **not one of them moved**: every `.lane-toggle`, `.tape-toggle` and
`#tips-toggle` sits in a fixed-height chip, and all 84 `line-height: normal` residuals are
`enH == zhH`. There was no form-control ratio to derive, so M8's leaf table was never
consulted for one.

### The two growth sites, both ROWS, and the cascade each caused

| container | kids | enH | zhH | dh | what it dragged |
|---|---|---|---|---|---|
| `div.euc-column-headers` | 2 | 9 | 11 | **+2** | `div.pattern-grid` +2, then `dy = +2.0` on every step button, lane label and Euclidean control below it |
| `#preset-dropdown-menu` | 5 | 146 | 151 | **+5** | `+1` per `div.preset-dropdown-item`, five rows |

Both absorbed entirely by pinning their leaves. `div.seq-header` moved **−0.5**, below the
gate's own 0.5 threshold, and is left alone rather than pinned to a sub-pixel.

### The pins are scoped to the Chinese arm, and the ESTIMATED denominator held

Task 4's recommendation, taken and measured: with the pins under `html[lang="zh-Hans"]`
the `wrap-count` ESTIMATED fraction reads **154 of 164 before and after** — identical, so
there is no F8 divisor movement to disentangle from a real change. This page's 94% is the
wave's second-highest and was the most exposed to that effect; scoping removed the exposure
rather than reasoning around it. **This is the first `html[lang="zh-Hans"]` block in this
plugin** (grep count 0 before), written on the shipped precedent at
`plugins/O-Prism/Source/ui/public/index.html:1298`.

### R8 / F7 done properly — the floor REMOVED, measured, restored

`.settings-toggle` carries a French-era `min-width: 42px`. A floor measured while in place
is not measured, so it was removed entirely, the natural boxes measured, and the file
restored with `git restore --source=HEAD` against a byte-clean `git status`:

| `#tips-toggle` | floor REMOVED (natural) | floor IN PLACE |
|---|---|---|
| en | **31.45** @ x 940.55 | 42 @ x 930 |
| fr | **47.33** @ x 924.67 | 47.33 @ x 924.67 |
| zh | **40.48** @ x 931.52 | 42 @ x 930 |

**The floor is LIVE on the Chinese arm.** It lifts English from 31.45 and Chinese from
40.48 onto the same 42, and never caps French, which exceeds it either way. Without it the
Chinese switch would have shrunk 1.52px and shifted right, moving the settings row — **the
one place on this page a Chinese shrink could have moved something, and an existing pin
already absorbs it.** No new pin was appended to that selector, so F7's un-pinning hazard
has no subject here.

The other three floors were re-measured in place and all three arms sit on them
identically: `.preset-name-display` 150/150/150, `.settings-popover` 190/190/190.

### `measure-ui` screens — the K12 vacuum, closed

| screen | baseline (VACUUM) | after |
|---|---|---|
| `undeclared-font` | 0 — **input EMPTY** | 2 → **0 against 168 visible Han-bearing nodes**, 0 of which resolve without a CJK face |
| `line-height-normal` | 0 — input empty | 94 → **84, every one measured at `enH == zhH`** |
| `wrap-count` | 0 (en-vs-fr only) | **0** |
| `svg-font-attr` | 0 carriers | 0 carriers |
| identity | 1068 nodes / 534 keys / 212 ids | 1605 / 535 / 212 (a third language pass) |
| leaves ESTIMATED | 154 of 164 | **154 of 164 — unmoved**, because the pins are zh-scoped |

Every zero after the table landed is a **measurement**. Every zero before it was a vacuum.

**The 84 residuals are not a caption count (F4).** They are named as classes with their
measurement rather than listed: `div.knob-label` ×40 at 9px and ×8 at 7px, the 20 lane
toggle chips at 8px, the four Euclidean toggles, `#seq_toggle`, `#tape_bypass`,
`#midi_toggle`, `#trig_toggle`, `div.combo-label` ×4, and the four preset-bar buttons.
**Every single one reads `enH == zhH`**, and the scripted check over the whole zh arm
returns **0 boxes whose height differs from English by more than 0.5px**. They are all
fixed-height chips or equal-height leaves: they cannot move and cannot be pinned to any
effect.

### The state-EFFECT assertion — all THREE click states, in all three languages (N1, K16)

`tests/i18n-states.json` was re-read for the state KIND rather than inherited: **three
clicks, no `eval`.** K16 is true on this plugin (it was false on O-simpleSampler).

| state | target | rect | `elementFromPoint` | took effect |
|---|---|---|---|---|
| preset dropdown OPEN | `#preset-name-display` | 150×24 @ 652,17 | target or descendant | **YES** — menu `none`→`block`, visible `[data-i18n]` 89 → 94 |
| settings popover OPEN | `#gear-btn` | 22×22 @ 961,651 | target or descendant | **YES** — 94 → 97 |
| hover help ON | `#tips-toggle` | 42×19.19 @ 930,615.81 | target or descendant | **YES** — face flips (`Off`→`On` / `Désactivé`→`Activé` / `关闭`→`开启`) |

**Nine assertions, no `null` rects**, so there was nothing to discriminate as a scrolled
target and no coverage hole. **The ORDER was checked and it matters here**: the states
file's own first entry says the dropdown is FIRST on purpose because its click handler
calls `stopPropagation`, so the document-level dismiss never fires and the menu stays up
while the popover opens behind it. No state scrolls, so no state moves the ones after it.

The French `#tips-toggle` rect is 61.38 wide against English and Chinese at 42 — the floor
above, visible in the probe.

`measure-ui --verbose`: **0** `did not resolve` / `unresolved` / `skipped state`. All three
states applied, including the dropdown that is `display: none` until clicked.

---

## Blind-read accounting

| | |
|---|---|
| rows dispatched | **133 of 133**, 0 withheld from review |
| chunks | 2, split on the CONCEPT (F17), each from its OWN emit → its own 16-hex salt |
| salts | `842e7738…` and `63b92cd0…` |
| ids shared between the two chunks | **0**, measured |
| ids returned identical AND IN ORDER (M12) | **YES**, both chunks, diffed position by position |
| Han surviving into returned English | **0** |
| malformed returned rows | **0** |
| dispatched zh column vs the COMMITTED tree | **byte-identical on all 133 rows**, joined back through the manifest |
| dispatched zh column sha256 | `e7f25ad50887c698c6afb51046de88ee9efda14e594bb322df32e184f095edf0` |
| product-name control | **does not exist in this version** — adjudicated by hand, 0 hits |
| triples read with `--verbose` (R2) | **133 of 133** |
| **rows re-authored** | **0** |
| correction rounds | **1**; it corrected nothing, so there is no round two (R4) |
| forward model | `claude-opus-5` |
| reverse model | `claude-sonnet-5` — **read back from the process**, not the alias |
| refusal controls fired | **2 of 2**, against a properly provenanced emit (F10) |
| termNote exemptions | **1**, entry-scoped |

**The concept split:** chunk A was the hover-help surface — 43 titles + 42 bodies = **85** —
and chunk B the on-page captions — **48**. 85 + 48 = 133.

### Both refusal controls, verbatim

1. `--provenance` omitted, correct manifest →
   `REFUSED: back-translation provenance is missing or identical to the forward pass — this triple proves nothing`
   `  no --provenance was given.`
2. The OTHER batch's manifest, **with** `--provenance` →
   `joined: 0   unjoinable ids: 85`
   `WRONG MANIFEST FOR THIS BATCH: all 85 returned ids are unjoinable, not one.`

The second reaches its own refusal only because the first condition was satisfied. Run
without `--provenance`, it refuses for reason 1 and proves nothing about the manifest.

### What the split bought — two readers, one morpheme, agreement

`轨` is the lane word. It went to both readers and came back the same way from both:

| chunk | id | en′ |
|---|---|---|
| A (titles + bodies, with context) | `O-Polystutter\|title\|lane` | **Stutter track** |
| B (captions only, no context) | `O-Polystutter\|label\|label.lane` | **Track {n}** |

English says *lane*; Chinese says *track*. The rendering is accepted, and the reason is
that Chinese draws no lane/track distinction for a parallel processing strip: 轨道 is the
term, and the caption and its own tooltip title now agree with each other, which is the
property that matters. **A single reader returns one of those two and nothing to check it
against.**

The split also settled the sequencer pair, which was the collision I flagged before
authoring. `seq` (title *Sequencer*) and `sequencer` (title *Pattern Sequencer*) are two
different keys on two different controls — `#seq_toggle` and `#sequencer-section` — and a
careless rendering would have painted 音序器 on both. They came back **`Sequencer`** and
**`Pattern sequencer`**, distinct.

### The one termNote, with its corpus site count checked first (N4)

`label.ping` renders **乒乓**, the ping-pong root, against a glossary root of
`['脉冲激励','激励']` — an EXCITATION impulse.

**The site count was checked before the note was written.** `Ping` as an English label has
exactly **one** other corpus site: `O-Octagon label.group.ping`, which ships `激励` and
means a loudspeaker-measurement ping. That is a different thing from a stereo bounce, and
the root was derived from that one caption with no second site to check it against — the
D1 shape exactly.

On this page `PING` abbreviates the PING-PONG toggle: `TIP_BINDINGS` binds
`#laneN_pingpong` to the title *Ping-Pong*, whose own root is `['乒乓']`. **The blind
reader, given two characters and captions-only context, read them back as `Ping Pong`.**
The note is entry-scoped and names both roots and O-Octagon's site.

### Accepted drifts, each with its written reason

Every one is a glossary root or a grammatical property of Chinese, and **none produces a
collision on the page** — which is the discriminator, not drift distance.

- **No plural inflection.** *Repeats* → 重复 → *Repeat*; *Steps* → 步 → *Step*; *Pulses* →
  脉冲 → *Pulse*. Chinese does not inflect for number and the glossary roots are singular.
  Nothing else on the page says repeat, step or pulse.
- **Every caption abbreviates its own title, in both languages.** *SUBDIV* → 划分 →
  *Division* beside *Subdivision* → 节奏划分 → *Rhythm division*; *SEQ* → 音序 →
  *Sequence* beside *Sequencer* → 音序器. The English does exactly this; the Chinese
  mirrors it.
- **Wow and flutter read as slow and fast wobble** (慢抖 / 快抖, the glossary roots). They
  are the tape-machine distinction the English names by jargon, they read distinctly from
  each other, and both the caption and title readers returned the same pair.
- **`body|dry` drops the "100% wet" clause.** English: *"Set to 0% for 100% wet stutters
  only."* Chinese: 设为 0% 时只听到断续 → *"At 0%, only the stutter is heard."* The English
  clause is self-referential — setting Dry to 0 IS 100% wet — and the Chinese states the
  audible consequence instead. No control is mis-named and nothing on the page collides.
  Accepted, and recorded because it is the only place the Chinese carries less than the
  English.
- **`Rolloff` keeps 滚降 where the FRENCH forked.** The French carries a `termNote`
  rendering this control as a CUTOFF, because the French root *Pente* is a dB/octave SLOPE
  and `TapeDegrader.cpp:347` sweeps a corner at a FIXED 12 dB/oct. **The Chinese root is
  not forked, deliberately**: 滚降 is a curve-shape term carrying the same looseness the
  English *Rolloff* already has, where *Pente* introduces a numeric quantity the English
  never claimed. The blind read confirms it: the title came back *Rolloff* and its own body
  came back *"Mimics the high-frequency loss of worn tape heads"* — a title and a body that
  do not name two different things. **No seventh glossary divergence is added.**

---

## Stale-body and dead-class verdicts (K13, C7)

The correction was made FIRST and the file **re-read** before a single Chinese row was
authored. The Chinese was written from the corrected English, not from the English the plan
quotes.

| class | verdict | evidence |
|---|---|---|
| the fixed-pair language enumeration | **LIVE, deleted in en AND fr** | comment-stripped `-CSD` probe **2 → 0** |
| its exception list | **KEPT, and its two CLASSES verified** — it names no number, so the numeric re-verification other tasks run has no subject | below |
| the "and nothing else" settings class | **NO SUBJECT — checked non-defect** | `grep -c "'tip.settings'"` = **0** |
| the "Tuning tab stays English" class | **NO SUBJECT — checked non-defect** | 0 `scala-tuning-engine` refs in CMakeLists, 0 private copies, 0 occurrences of *tuning* in the markup |

**The French deletion was built by REGEX, never hand-typed.** The source carries a U+00A0
before the semicolon (`disponibles ;`, added in the v1.14.1 typography pass) that a
typed literal cannot reproduce — the first attempt aborted at 0 occurrences and was
rewritten to match rather than to guess. `reviewed: true` is kept on the French: the same
clause is removed from both arms, so the two still say the same thing.

### The exception list names CLASSES, so both classes were checked

- **"note divisions stay in English."** The subdivision controls are four
  `AudioParameterChoice` declarations whose option strings are
  `["1/4","1/8","1/16","1/32","1/8T","1/16T"]` (`PluginProcessor.cpp:47` ff., mirrored at
  `js/parameter-bindings.js:551`). **They carry no word to translate in any language** —
  the strongest form the claim can take. They are also not `<select>` elements: the markup
  renders them as `div.combo-box`, which is why the `<select>` count is 9 and none of them
  is a division menu.
- **"preset names stay in English."** `#preset-name-text` is not keyed, and its placeholder
  `'Default'` is an `I18N_EXEMPT` entry citing the JSON filename. This is K8's finding and
  it is exactly this claim's subject.

### The `#tips-toggle` pairing, recorded so a later reader does not infer one from the other

**This is the one plugin in the wave where the toggle EXISTS and the false claim does not.**

```
grep -c "'tip.settings'"  (comment-stripped i18n.js)  ->  0
grep -c "tips-toggle"     (index.html)                ->  2
```

`check-i18n` `[16]` confirms the shape from the other direction: *"found 1: `#tips-toggle`"*
and *"the key #tips-toggle binds exists in I18N — got `tips-toggle`"*. So the hover-help
switch on this plugin is `#tips-toggle` rather than the `#help-toggle` the other five
carry, it IS a tip anchor, and **no body makes an exclusivity claim about it.** The
presence of the control is not evidence of the defect.

---

## Font work — the tail half, plus one form-4 stack, all inline

**Censused with `find`, never with an `ls <root>/css/*.css` glob**, which would have
aborted the whole command: this plugin has **0** CSS files. All twelve declaration sites
are inline in a 1776-line `index.html` — the wave's only fully inline page.

```
6  font-family: 'Garamond', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif
1  font-family: 'Georgia', 'PingFang SC', 'Microsoft YaHei', serif
5  font-family: inherit
```

Both real stacks were already Latin-safe — Times New Roman is the second member of one and
Georgia the first member of the other, and both are installed — so this is the **tail half
only**, before the trailing generic (W1). The absent Garamond member was **kept**: it is
the Windows and print intent and this machine is not the one the suite builds for.

**N3 has no subject and that is measured:** `grep -c "symbol-font"` returns **0**. This is
the only one of the six that defines no `--symbol-font` token.

**The form-4 site, which the census found and a grep never would.** `.preset-nav-btn`
declares **no** `font-family` at all. A `<button>` does not inherit one, so the UA resolved
it to Arial — and `measure-ui` named both arrows as `undeclared-font` findings once the
table landed, because their own text is `<` and `>` but their `aria-label` is Chinese:

```
zh-Hans  #preset-prev  "<"  ff=Arial
zh-Hans  #preset-next  ">"  ff=Arial
```

**Arial stays FIRST** — it is what already renders, so neither Latin arm can move — with the
tail appended before a `sans-serif` generic, which is W1's own form-4 shape verbatim.

Verified by **re-running the screen**, never by reading the CSS back: `undeclared-font`
**2 → 0** against 168 visible Han-bearing nodes, with a scripted check that **0 of the 168**
resolve without a CJK face, and the other stack populations unmoved (M7).

---

## Zeros recorded AS MEASURED

- **gate files: 0.** `find plugins/O-Polystutter/tests -name '*.js'` returns nothing;
  `tests/` holds `i18n-states.json` and nothing else — **the wave's only plugin with no
  `render-harness/` either.** Fired with `find`, never a glob. **No gate was invented**
  (C5/C6 has no subject).
- **second version source: NONE, classified rather than assumed** (Task 1's FALSE-2
  lesson). Every `CMakeLists.txt` under the plugin was grepped for `JucePlugin_Version`:
  there is one CMakeLists and it holds no mirror, frozen or live. K6 holds here.
- **duplicate keys: NONE**, F13-corrected form (LANGUAGES members excluded), `keys=91`,
  before and after.
- **language `AudioParameterChoice`: none.** The four on this plugin are the lane
  subdivisions, and their option strings stay English under D-01 — trivially, since they
  are fraction tokens.
- **Han under `Source/`: 0**, negative grep across every `.h`/`.cpp`, with the positive
  control firing on `js/i18n.js` in the same run.
- **`zh-Hans` in the comment-stripped `PluginProcessor.h`: 2** (encode and decode), and both
  functions report **THREE-WAY** when matched by name. **This is the wave's only codec with
  no doc comment above it**; the strip runs anyway, so the probe is identical across all six
  and a reader comparing tasks is comparing like with like.
- **`git status --short -- .../modules/`: 0.** Not one byte of `preset-manager.js` changed.
- **submodule untouched.** `git status --short -- plugins/O-Orbit/libs/SAF` → empty;
  `git submodule status` → ` b6fe1882… (v1.3.4)`, leading space = clean. **The guard ran
  before BOTH commits** even though no path here is inside it.
- **tags created: 0.** **`PLUGINS.md` touched: no** (Task 7 owns it).
- **file deletions across both commits: 0.**
- **files staged outside `plugins/O-Polystutter/`: 0**, across both commits.

---

## Gate results

| gate | result |
|---|---|
| `check-i18n --plugin` | **exit 0** — `got ["en","fr","zh-Hans"]`, `43 I18N + 48 LABELS` unchanged, **`105 tip(s) bound`** unchanged, `[12] 2 module(s): js/app.js, js/parameter-bindings.js` unchanged — and `modules/preset-manager.js` is STILL not among them |
| `i18n-zh-lint --plugin` | **0 findings**, 91 entries, **BELOW SHIP BAR 0** |
| `i18n-zh-lint --self-test` | **10/10** |
| `check-ui-labels --plugin` | **exit 0**, `== ALL CHECKS PASSED ==`, 0 moved on both non-English arms across all three states, `103 of 97` visible with **0 never-visible** |
| `measure-ui --report all` | `undeclared-font` **0**, `line-height-normal` 84 all at `enH == zhH`, `wrap-count` **0**, `svg-font-attr` 0 |
| `measure-ui --verbose` | **0** unresolved or skipped states |
| `boot-all-uis --strict-tips` | **clean 1/1**, **0 DEAD, 0 late** across 105 bindings |
| `i18n-zh-lint` repo-wide | **exit 0**, 0 findings across **44** plugins, 4238 zh entries — see FALSE-6 |
| `i18n-fr-lint` repo-wide | **exit 0**, 0 / 44. **No French rendering changed** |
| `check-i18n` repo-wide | `ALL CHECKS PASS — 44 localized plugin(s)` at self-check; it **FAILED on O-Strata at preconditions** — see FALSE-7 |
| corpus | **5664 of 5789 rows, 43 plugins carrying zh** — read live; the denominator is moving (FALSE-6) |
| `auval` | **NOT RUN** — deferred to Task 7's single cold sweep, as the plan directs |

---

## Commits

| hash | subject |
|---|---|
| **`ad264e7e`** | `i18n(O-Polystutter): add Simplified Chinese at reviewed:'mt' — 133 rows, the body-less entry mirrored` |
| **`ce5a6a8c`** | `i18n(O-Polystutter): promote 133 zh rows to reviewed:'bt' after a blind reverse read — v1.15.0` |

Both path-scoped `git commit -F <file> -- plugins/O-Polystutter`, options before the `--`
(F14.6). `git branch --show-current` and `git status --short` re-checked immediately before
each, never once at the start. The submodule guard ran before each. Nothing under
`plugins/O-Strata/`, nothing under `.claude/agent-memory/`, nothing under `.planning/`, and
not one byte of `PLUGINS.md` was ever staged.

---

## Build and install

`./scripts/build-and-install.sh O-Polystutter` — the **FOLDER** name; the script resolved
the `OPolystutter` target itself, which is one of the two places in this wave where the
target is not the folder name. Run in the background and polled on a sentinel — **exit 0,
51 s.**

```
VST3: ~/Library/Audio/Plug-Ins/VST3/O-Polystutter-dev.vst3            5.5M, age 0s
AU:   ~/Library/Audio/Plug-Ins/Components/O-Polystutter-dev.component 5.4M, age 0s
```

`CFBundleShortVersionString` read back from the installed AU bundle: **`1.15.0`** — the
version reached the binary, not just the CMakeLists.

**No alternate-variant orphan.** Phase 4's dual-variant sweep emitted no
`⚠ Sweeping ALTERNATE-variant` warning; only the `-dev` bundles are on disk.

**`auval` was NOT run.** Task 7's single cold sweep. Budget ~85 s, not the stale 15-minute
figure.

---

## Deferred items

1. **`'Loaded Preset'` is an unkeyed, unexempt English string that DOES reach this page.**
   `plugins/O-Polystutter/Source/ui/public/modules/preset-manager.js:283` writes
   `this.currentPreset = result.name || 'Loaded Preset'`, and `_updateDisplay()` at `:346`
   puts it into `#preset-name-text`. It is reachable only after a successful file load, so
   no static scan sees it, no `[data-i18n]` covers it, and no state in `i18n-states.json`
   fires it. Its sibling `'Default'` IS an `I18N_EXEMPT` entry; this one is not, so the
   exemption list is incomplete rather than the string being deliberately English.
   **Grep-able token: `'Loaded Preset'`.**

2. **The unscanned-module class is now measured on four of six wave-4g plugins and the two
   `preset-manager.js` copies DIVERGE.** O-Polystutter's is **406 lines, sha1
   `0142c1da…`**; O-Orbit's is **447 lines, sha1 `d15e751b…`**, and O-Orbit carries **two
   copies of its own** at `Resources/ui/js/modules/` and `Source/ui/public/modules/`, both
   at that same sha. `check-i18n`'s `[12]` opens none of them, and the blindness is
   **directory-scoped**: `js/parameter-bindings.js` is scanned and `modules/preset-manager.js`
   is not, on the same plugin. Eighteen consumers (s71 D2); a rollout of its own.
   **Grep-able token: `createPresetBar` in `modules/preset-manager.js`.**

3. **O-Strata is a fork of O-Prism and arrived fully localized — the corpus close-out
   arithmetic in the plan is stale and MOVING.** Commit `0e838512`. Both repo-wide lints
   now exit **0**, the emitter counts **44** plugins with **0 unreadable**, O-Strata itself
   reads **348/348 at `bt`** (375 an hour earlier), and the total denominator read **5816**
   then **5789** within one working period. **Task 7 must read the corpus live and state
   the reading with its timestamp; `43 of 43`, `5441` and `exit 2` are all superseded.**
   **Grep-able token: `O-Strata` in the emitter's per-plugin table.**

4. **The Z6 budget backfill is unchanged at `3 of 552`.** This task added none, for the
   reason waves 4d–4f predicted: every geometry finding was a line-box growth wanting a
   ratio, or a shrink an existing floor already absorbed. Neither wants a character budget.

5. **The glossary divergence report stays at SIX entries.** This task adds no seventh.
   `Rolloff` was the candidate and was deliberately not forked — see the accepted-drifts
   section for the reasoning, which turns on the French root naming a numeric quantity that
   the Chinese root does not.

---

## For Tasks 6–7

Re-read the corpus numbers live and do not inherit any of the plan's: O-Strata became a
localized plugin mid-wave and both the numerator and the denominator have moved twice in an
hour. `check-i18n` repo-wide can be RED for reasons that are not yours — fire it at
preconditions AND at self-check and report both readings, because either one alone
misleads.

Scope the geometry pins under `html[lang="zh-Hans"]`: it made this page's four pins close
the gate in one round with the ESTIMATED denominator unmoved, on the wave's second-most
estimated page. Census the font sites with `find` and read the ones that declare NO family
— a `<button>` inherits none, and `.preset-nav-btn` was invisible to every stack census
because it has no stack to census. And do F7 properly rather than reasoning about it: the
removal measurement here took one background run and turned "the floor is probably doing
work" into 31.45 / 47.33 / 40.48.

---

## A note on the shared checkout

A concurrent session was actively working on O-Strata throughout this task and committed
**`0e838512`** — a fork of O-Prism — plus a `PLUGINS.md` edit, **between** my two commits.
That is the exact hazard the commit discipline exists for, and it came through clean:

- both commits are present and untouched — `ad264e7e`, `ce5a6a8c`;
- each carries **3** files, all under `plugins/O-Polystutter/`, with no stranger's path;
- **0 deletions** across both;
- `PLUGINS.md` was modified in the working tree at my first commit and is **not** in either
  of my commits — Task 7 owns it and nothing of mine touched it;
- the O-Strata untracked count moved **4 → 0** during the task, and the whole of the plan's
  repo-wide lint baseline was rewritten by that session's work (FALSE-6, FALSE-7).

Every commit was path-scoped and the branch and staging were re-checked immediately before
each rather than once at the start.

---

## Self-Check: PASSED

Files claimed, verified on disk:

- `plugins/O-Polystutter/Source/ui/public/js/i18n.js` — FOUND
- `plugins/O-Polystutter/Source/ui/public/index.html` — FOUND
- `plugins/O-Polystutter/Source/PluginProcessor.h` — FOUND
- `plugins/O-Polystutter/CMakeLists.txt` — FOUND
- `plugins/O-Polystutter/CHANGELOG.md` — FOUND
- `plugins/O-Polystutter/Source/ui/public/modules/preset-manager.js` — FOUND, **unchanged**

Commits claimed, verified in `git log --all`:

- `ad264e7e` — FOUND
- `ce5a6a8c` — FOUND

Scope claims, verified:

- files staged outside `plugins/O-Polystutter/` across both commits — **0**
- paths staged under `plugins/O-Orbit/libs/SAF` — **0**; submodule still at `b6fe1882`
  (v1.3.4), clean
- tags created — **0**
- `PLUGINS.md` touched — **no** (Task 7 owns it)
- `ROADMAP.md` / `STATE.md` / `PLAN.md` / this summary committed — **no**
- installed bundles on disk — `O-Polystutter-dev.vst3` and `O-Polystutter-dev.component`,
  **`-dev` only, no alternate-variant orphan**, `CFBundleShortVersionString` = `1.15.0`
