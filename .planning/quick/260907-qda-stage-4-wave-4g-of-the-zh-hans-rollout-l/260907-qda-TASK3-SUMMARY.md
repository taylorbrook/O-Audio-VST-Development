---
task: 3
plugin: O-simpleSampler
phase: quick-260907-qda
wave: 4g
status: complete
shipped_version: "1.5.0"
bumped_from: "1.4.4"          # the CMakeLists VALUE via the two-arm reader, not the 1.4.3 PLUGINS.md says
rows: 145
entries: 109                  # 36 I18N (t+b) + 73 LABELS — one `reviewed:` flag per ENTRY, not per row
reviewed: bt
requirements: [ZH4G-03, ZH4G-08, ZH4G-11]
commits:
  - c5effaac   # the 'mt' table, the body correction, the endonym, the font tail, the codec, the nine pins
  - b1d9e6b2   # promotion to 'bt', version bump, CHANGELOG
false_predictions: 5
corpus_after: "5050 of 5441 rows, 40 plugins carrying zh"
---

# Task 3 — O-simpleSampler 1.4.4 → 1.5.0, Simplified Chinese

145 rows at `reviewed: 'bt'`, shipped as **1.5.0**, rebuilt and reinstalled through
CLAUDE.md's cache sequence. Two path-scoped commits. Every gate green. `auval`
deliberately not run — it is Task 7's single cold sweep.

---

## K4 — THE REGISTRY ROW IS WRONG ABOUT THE MACHINE, IN BOTH CELLS

This is the task's centre of gravity and the evidence Task 7 needs. **Measured
before any edit, and again after the build.**

| | `PLUGINS.md` row | the machine | the source of truth |
|---|---|---|---|
| **status** | `✅ Working` | **both `-dev` bundles ALREADY on disk**, installed 2026-09-04T09:57:54Z | — |
| **version** | `1.4.3` | installed bundles reported `CFBundleShortVersionString` **1.4.4** | `CMakeLists.txt:20` read **`VERSION "1.4.4"`** |

```
| O-simpleSampler | ✅ Working | 1.4.3 | Synth (Pedagogical Sampler) | 2026-08-31 |   ← PLUGINS.md:65, as found
```

**So "install it" was never a new install — it is a rebuild-and-reinstall of an
already-present bundle beside a registry cell that never followed.** The task
description's framing is a faithful read of a stale cell, and both cells are stale
from the same cause: the 2026-09-03 suite-wide French hover-help rename (task
260903-ukp), which produced the 1.4.4 CHANGELOG entry the registry never saw.

### On-disk bundle state, BEFORE and AFTER

| | before | after |
|---|---|---|
| `~/Library/Audio/Plug-Ins/VST3/O-simpleSampler-dev.vst3` | mtime **2026-09-04T09:57:54Z**, 6.3M, **`1.4.4`** | mtime **2026-09-07T21:09:56Z**, 6.3M, **`1.5.0`** |
| `~/Library/Audio/Plug-Ins/Components/O-simpleSampler-dev.component` | mtime **2026-09-04T09:57:54Z**, 6.3M, **`1.4.4`** | mtime **2026-09-07T21:09:56Z**, 6.3M, **`1.5.0`** |
| unsuffixed alternate variant beside either | **none** | **none** |

**No `⚠ Sweeping ALTERNATE-variant` warning fired** — grepped for and counted
**0**. Phase 4 of the script removed the two `-dev` bundles it found and Phase 5
installed the rebuilt pair; only the `-dev` variant is on disk, in both formats,
before and after. There was no unsuffixed `O-simpleSampler.{vst3,component}` to
pin Logic's registry slot.

`killall -9 AudioComponentRegistrar` and both AudioUnit cache paths were cleared
BEFORE `build-and-install.sh` ran, per CLAUDE.md.

**For Task 7: the row needs BOTH cells edited in one edit — `✅ Working` →
`📦 Installed`, and `1.4.3` → `1.5.0`.**

---

## The thirty-seventh tip binding — READ, NAMED, and a checked non-defect

`check-i18n [2]` reports **37 tip(s) bound** against **36** declared `I18N` keys.
Measured through the module rather than by eye:

```
I18N keys 36   LABELS keys 73   TIP_BINDINGS rows 37   distinct bound keys 36
keys bound MORE THAN ONCE: [ [ 'pitchMode', 2 ] ]
I18N keys NOT bound: []          bound keys NOT in I18N: []
```

**The thirty-seventh binding is `pitchMode`, bound to two anchors:**

```js
['[data-param="pitchMode"]',  'pitchMode'],   // the select cell
['#pitchModeReadout',         'pitchMode'],   // the readout beside it
```

It is a **legitimate many-to-one binding, and the file says so in its own words**
at the head of `TIP_BINDINGS`: *"pitchMode appears TWICE, on two different anchors:
the select cell and the readout beside it are two halves of one control and say
the same thing."* Not an off-by-one. Every `I18N` key is bound; no bound key is
missing from `I18N`; no Chinese body was written for a key nothing reaches.

The count did **not** move: `37 tip(s) bound` before and after. And `2 · 36 + 73 =
145` reproduces the emitter's row count exactly, so the double binding costs the
table nothing — a binding is not a row.

---

## The unscanned drop module — Task 2's verdict INHERITED, the sha RE-FIRED

| | |
|---|---|
| **sha1** | `c7fe612ee5d3701381e9efc764480e79f12d377e` — matches Task 2's and the plan's `c7fe612e…` |
| **sha256** | **`349a0c285ababd7fc7741855f1d908c70dbf979d9c135073cc2bcff837f2a9cd`** — **byte-identical to Task 2's recorded digest**, re-fired on this plugin's copy rather than assumed |
| `diff -q` against O-simpleGrain's copy | **no difference** |
| `check-i18n` `[12]` on this plugin | **`1 module(s): js/app.js`** — quoted verbatim, before and after |
| `git status --short -- .../modules/` | **0** — not one byte changed |
| `opts.showToast(` call sites | **22** | 
| distinct English strings passed to them | **17** |

Both counts reproduce Task 2's exactly, which is what byte-identity predicts and
is the check that it holds. **The plan's "~12" is a distinct-string undercount;
17 / 22 is the measured pair.**

**Verdict INHERITED with its reasons restated, not re-derived:** no gate in this
repo opens the file — `check-i18n`'s `[12]` clause is keyed on the `js/`
directory and this file is under `modules/`, so its PASS is a claim about the
SCAN and not about the page; the strings are not `[data-i18n]` elements and no
state in `i18n-states.json` fires a drop event; they render English on the French
page today and this release does not cause them; and a fix is a two-plugin change
to an unkeyed toast layer with no calibrated gate. **No partial keying was added.**

The whole census was fired with `find` over the WHOLE UI root, never a glob and
never `<root>/js`: three JS files, `js/i18n.js`, `js/app.js` and
`modules/webview-drop-streaming.js`. A census scoped to `js/` reads two.

---

## False-prediction ledger — five, marked per plan prediction

### FALSE-1 — precondition (i): `git status --short | grep -c "O-Strata"` reads **0**, not 2

Task 1's and Task 2's FALSE-1, confirmed a third time. A concurrent session
committed both O-Strata planning files before the wave started. The untracked set
today is this wave's own plan directory alone.

**K2's load-bearing claim is unaffected and re-fired live**: O-Strata still has no
`i18n.js`, and both repo-wide lints still exit 2 with `0 finding(s) across 0
plugin(s)` and `1 plugin(s) could not be read`. The finding count and the
unreadable count are the baseline; the exit code and the `git status` line never
were.

### FALSE-2 — precondition (j): the positive control CANNOT fire before the table lands

Task 2's FALSE-2, same mechanism. Pre-work `js/i18n.js` holds 0 Han code points,
so the control necessarily prints nothing and a green there would mean the probe
was broken. **The control's correct home is the VERIFY step**, and there it fired:
the negative printed nothing across every `.h`/`.cpp` under `Source/`, and the
control printed the `i18n.js` path on the same run.

### FALSE-3 — **K16 IS FALSE ON THIS PLUGIN: the fourth state is an `eval`, not a click**

The plan asserts, in K16 and again in this task's own `<action>` and
`<human-check>`, that all four of this plugin's states are clicks — *"FOUR CLICK
STATES … #gear-btn, #help-toggle, a concept-preset click, and pitch-mode
STRETCH"* — and the live-observations table records `click states: 4 of 4`.
`tests/i18n-states.json` reads:

```json
{ "name": "pitch mode STRETCH (label.pitchStretch — the readout face only setupPitchModeReadout writes…)",
  "eval": "window.__stubStates.combos.get('pitchMode').setChoiceIndex(1)" }
```

**Three clicks and one `eval`.** The state name the plan quotes is correct; the
mechanism it attributes to that state is not. **The wave's "nineteen click states"
arithmetic is wrong by at least one** — an executor on another plugin should
re-read its own states file rather than inherit the count.

The consequence is concrete and was handled: `elementFromPoint` at the target's
centre is meaningless for a state with no target, so the probe fired on the three
clicks in all three languages and the fourth state was asserted on its EFFECT
alone. Recording it as "4 of 4 clicks probed" would have claimed a probe that
cannot exist.

### FALSE-4 — the R3 glossary screen's denominator is **69**, not 23

The plan records `NONE (23 matched)` at planning time. Re-fired here over captions
**and** tooltip titles: **69 glossary-matched of 109 screened, collisions NONE.**
The same denominator difference Task 1 (22 → 50) and Task 2 (27 → 76) recorded —
the planning screen matched captions only. **The verdict is identical and that is
the part that matters.** A mismatch here is a denominator difference, not a
finding.

### FALSE-5 — the emitter has **no product-name control** in this version

The plan directs: *"Expect the product-name control to report a non-zero count …
adjudicate it, do not treat the count as pass/fail."* `grep -n "product"
scripts/i18n-zh-backtranslate.js` returns **nothing** — the control does not exist
in this version of the script, so there is no count to adjudicate and no line to
read. **The rule has no subject; the substance still does**, so the check was made
by hand:

- occurrences of the product name `simpleSampler` in the emitted zh column: **0**;
- rows carrying the sampler MORPHEME `采样器`: **7**, and every one of them
  translates the COMMON NOUN in its own English sentence — `label.subtitle`
  ("Keyboard Sampler · A Field Guide"), `body|vintage` ("Old-sampler grit"),
  `body|dropZone` ("a sampler can play any sound"), `body|lessonSp1200`,
  `body|lessonRawOneShot`, `label.captionRawOneShot`, `label.captionSp1200`.

The wordmark is split across two `I18N_EXEMPT` text nodes — `'O – simple'` and
`'Sampler'` — and never enters the table. **Adjudicated, not counted.**

---

## The second version source, classified rather than assumed (Task 1's FALSE-2 lesson)

K6 says no other plugin in the six has one. Following Task 1's warning, the
harness was **grepped and the finding classified**:

```
plugins/O-simpleSampler/tests/render-harness/CMakeLists.txt:65  JucePlugin_VersionString="0.1.0"
plugins/O-simpleSampler/tests/render-harness/CMakeLists.txt:66  JucePlugin_VersionCode=0x000100
```

**A FROZEN LITERAL, the O-simpleAdditive shape — not O-simpleGrain's `${VAR}`
mirror.** It has never tracked the plugin version (it is at 0.1.0 against a 1.4.4
plugin) so the bump does not desynchronise it any further, and a `${VAR}`
reference would have HAD to move. **Deliberately untouched** — it is pre-existing,
out of scope (wave 4e D6), and editing it inside a commit whose subject is a
caption table is a change nobody asked for. Carried as a deferred item; **this is
now the SECOND site of the frozen-literal pattern in this wave**, which upgrades
Task 1's "worth a suite-wide sweep" from a guess to a measurement.

The promotion script prints the classification on every run rather than asserting
it:

```
harness: JucePlugin_VersionString="0.1.0"  -> A FROZEN LITERAL — never tracked, must NOT be touched inside a localization commit
```

## The two-arm reader, fired on the shape where one arm also works

| | one-arm | two-arm |
|---|---|---|
| before the bump | `1.4.4` | `1.4.4` |
| after the bump | `1.5.0` | `1.5.0` |

A quoted literal at `CMakeLists.txt:20`, so both arms agree. **Fired both anyway
(R9):** a reader that is only exercised where it was written is not a reader, and
this is the controlled contrast to O-simpleGrain, where the one-arm reader returns
EMPTY and keeps returning EMPTY after the bump.

---

## Measured numbers, before and after

### Geometry — `check-ui-labels`, across all four states plus the resting page

| | non-label elements moved, zh arm | assertions 5 / 6 |
|---|---|---|
| table landed, no pins | **146** | FAIL 2 spills 12.8px / FAIL 2 frame crossings 9.8px |
| after 6 leaf line-box pins | **142** | FAIL 4.8px / FAIL 1.8px |
| after 3 ROW ratios | **0** | PASS / PASS |
| after the promotion and the bump | **0** | PASS / PASS |

**The fr arm was green at every one of the four measurements** — `5` green
`[7][GEOMETRY DIFF][fr]` assertions on the final run, matching the `5` on the zh
arm. Coverage never moved off **48 of 47** visible with **0 never-visible**;
`48 of 47` is F18's shape (N counts paths seen across the cumulative walk, M is
final-DOM membership) and is not a failure.

### The nine pins, every ratio derived from that element's own English CONTENT box

Rect height less its own padding and border, divided by its own font-size, line
count verified as exactly 1. **None of the nine selectors carried a `line-height`
before this block** — grepped one by one (R8/F7).

| selector | fs | en rect | pad t/b | border t/b | content | ratio |
|---|---|---|---|---|---|---|
| `.subtitle` | 11 | 12 | 0/0 | 0/0 | 12 | **1.0909091** |
| `.settings-label` | 11 | 12 | 0/0 | 0/0 | 12 | **1.0909091** |
| `.group-title` | 12 | **19** | 0/3 | 0/1 | **15** | **1.25** |
| `.preset-bar-tour .tour-btn` | 9.5 | 20 | 4/4 | 1/1 | 10 | **1.0526316** |
| `.btn-load` | 10.5 | 25 | 6/6 | 1/1 | 11 | **1.047619** |
| `.source-status` | 10 | 13 | 0/0 | 0/0 | 13 | **1.3** |
| `.viz-label` **(ROW)** | 10 | 11 | 0/0 | 0/0 | 11 | **1.1** |
| `.keyboard-label` **(ROW)** | 10 | 11 | 0/0 | 0/0 | 11 | **1.1** |
| `#toggle-reverse` **(ROW)** | 10 | 29 | 7/7 | 2/2 | 11 | **1.1** |

**`.group-title` is the padding case, live**: its rect is 19 and its content box
is 15, because 3 px of bottom padding and a 1 px border are not line box. A ratio
from the rect would have been **1.5833** and moved English on every group heading
on the page. **It lands on the same 1.25 Task 1 and Task 2 measured for the same
class at the same size — a third independent site for that number.**

**`.btn-load`, `.tour-btn` and `#toggle-reverse` are the F6 form-control case**:
the UA `font` shorthand resets `line-height` on a `<button>`. M8's leaf table
would have given `#btn-load` 10.5px → 1.1666667 against its measured 1.047619.

**`.source-status` is the F7 case and is recorded honestly**: its English rect is
13 because THAT RULE'S OWN French-era `min-height: 13px` floors it there, not
because its natural line box is 13. The pin is derived from the same 13 so the
floor and the line box agree, English is unchanged to the pixel, and the Chinese
14 comes back to 13.

### The three ROW pins — the wave-4e N4 shape, three times

Each grew while **every leaf inside it measured `enH == zhH` in all three
languages** — `getBoundingClientRect()` on an inline element reports its font box,
not its line box, so a row grows with nothing inside it reporting the growth.

| row | en | zh before | zh after | what it dragged |
|---|---|---|---|---|
| `div.viz-label` | 11 | **14** | 11 | `#viz-waveform` +3, `canvas-wrap` and `#waveformCanvas` +3 y, then the whole `.rack` below it |
| `div.keyboard-label` | 11 | **14** | 11 | pushed `label.play` and `label.kbdHint` **1.8 px past the 980 × 720 frame** |
| `#toggle-reverse` | 29 | **32** | 29 | `div.toggle-cell` +3 |

The keyboard row is the one that mattered most: assertions [5] and [6] were
failing on a **frame crossing**, and no leaf reported it.

### `measure-ui` screens — the K12 vacuum, closed

| screen | baseline (VACUUM) | after |
|---|---|---|
| `undeclared-font` | 0 — **input EMPTY** | **0 against 95 visible Han-bearing nodes**, 0 of which resolve without a CJK face |
| `line-height-normal` | 0 — input empty | 25 → 6 → **1**, named below |
| `wrap-count` | 0 (real, baseline 0) | **0** |
| `svg-font-attr` | 0 carriers | 0 carriers |
| identity | 454 nodes / 227 keys / 109 ids | 684 / 228 / 109 (a third language pass) |
| leaves ESTIMATED | 58 of 81 | **33 of 81** — 25 leaves left the estimated set as the pins made them measurable |

Every zero after the table landed is a **measurement**. Every zero before it was a
vacuum. The ESTIMATED divisor moving 58 → 33 with no pixel changing is exactly the
effect F8 warns about, live for the third time in this wave.

**The one `line-height: normal` residual, named with its measurement (N1):**
`#help-toggle`, font-size 10 px, **enH == frH == zhH == 24**. A fixed-height form
control; it cannot move and cannot be pinned to any effect. **The same node Task 1
and Task 2 each named** — three plugins, one residual, one cause.

### The state-EFFECT assertion — THREE clicks probed, one eval asserted on effect

| state | mechanism | target | rect | `elementFromPoint` | took effect |
|---|---|---|---|---|---|
| settings popover OPEN | click | `#gear-btn` | 21×21 @932,21 | target or descendant | **YES** — visible `[data-i18n]` 44 → 47 |
| hover help OFF | click | `#help-toggle` | 96×24 @846,88 | target or descendant | **YES** — face `Off`→`On` / `Désactivées`→`Activées` / `关闭`→`开启` |
| SP-1200 Crunch lesson | click | `.tour-btn[data-preset="SP-1200 Crunch"]` | 54×20 @~815,22 | target or descendant | **YES** — 47 → 44, caption text changes |
| pitch mode STRETCH | **eval** | — (no target) | — | **n/a** | **YES** — readout `Repitch — pitch & time linked` → `Stretch — time held, pitch independent`, and its zh and fr faces |

**Nine click probes (3 states × 3 languages), no `null` rects and no coverage
holes**, so there was nothing to discriminate as a scrolled target. Twelve
state/language pairs, twelve effects. The states were walked in the file's own
ORDER, cumulatively, and no click scrolled.

---

## Blind-read accounting

| | |
|---|---|
| rows dispatched | **145 of 145** (72 + 73), 0 withheld from review |
| chunks | 2, split on the CONCEPT (F17), each from its own emit → own 16-hex salt |
| salts | A `c7f41b8ad7539a1739ee0c1879d785e0` / B `dd0e7fd7659438a992c622f63cc5609c` / round 2 `c6323985bfab72f2870b668788114777` |
| ids shared between chunks | **0**, measured on all three pairings |
| ids returned identical AND IN ORDER (M12) | **yes**, all three batches, diffed |
| Han surviving into returned English | **0** |
| malformed returned rows | **0** |
| dispatched zh column vs the COMMITTED tree | **byte-identical** — `git diff HEAD -- plugins/O-simpleSampler` empty at dispatch time; sha256 of the emitted zh column `79e6cbf90eea91679f286381aec90e4fe01d9b6d7a5a56ecafff7a76ca4ab9ab` |
| product-name control | **NO SUBJECT — the script has none** (FALSE-5); adjudicated by hand: 0 wordmark occurrences, 7 common-noun 采样器 rows |
| triples read with `--verbose` (R2) | **145 of 145**, plus 20 in round 2 |
| rows re-authored | **0** |
| correction rounds | **2**; round 2 corrected nothing, so no round three |
| forward model | `claude-opus-5` |
| round-1 reverse model | `claude-sonnet-5` — read back from the process, not the alias |
| round-2 reverse model | `claude-haiku-4-5-20251001` — a **third** model, fresh salt, fresh session (R4) |
| refusal controls fired | **2 of 2**, against a properly provenanced emit (F10) |
| termNote exemptions | **7**, entry-scoped (N4) |
| `sameAsEn` zh rows | **2** |

**Both refusal controls, verbatim:**

1. `--provenance` omitted →
   `REFUSED: back-translation provenance is missing or identical to the forward pass — this triple proves nothing` / `  no --provenance was given.`
2. wrong-side `--manifest`, **with** `--provenance` →
   `WRONG MANIFEST FOR THIS BATCH: all 72 returned ids are unjoinable, not one.`

`--forward-provenance` was passed a real STRING (F9), naming the model, the date,
the task and the commit the Chinese was read from.

### The concept split paid for itself twice

**1. `循环铺垫` — one string, three readings, and the pair is what settles it.**

| chunk | reader | id | en′ |
|---|---|---|---|
| A (titles + bodies) | `claude-sonnet-5` | `title\|lessonLoopedPad` | **Loop Pad** |
| B (captions) | `claude-sonnet-5` | `label.presetLoopedPad`, `label.captionLoopedPad` | **Loop Bed** |
| C (round 2) | `claude-haiku-4-5` | all three of the above | **Loop Pad** |

The caption-only reader had no context and read `铺垫` in its literal
bedding/underlay sense; the reader that also had the tooltip body ("a loop with a
crossfade turns a short sound into an endless one you can hold") read it as the
synth **pad**. A single reader returns one of those and nothing to check it
against. **The site count was checked before any note was written** (N4):
`'looped pad' → 循环铺垫` is the shipped glossary root, and the `铺垫` morpheme has
a **second** corpus site (`'frozen pad' → 冻结铺垫`), so it was not derived from one
caption and has something to be checked against. **Accepted, not forked** — two of
three independent readings, on two models, returned the pad.

**2. `循环渐变` vs `循环交叉渐变` — round 2 proved the abbreviation is doing its job.**

| id | zh | en′ (round 2, haiku) |
|---|---|---|
| `label.loopXf` | 循环渐变 | **Loop Fade** |
| `title\|loopCrossfade` | 循环交叉渐变 | **Loop Crossfade** (1.00) |

Round 1 read the caption as "Loop Crossfade", collapsing it into its own title.
Round 2, on a different model with both strings in one batch, returned **two
different English strings** — which is exactly what the caption abbreviation is
for, and exactly what the English `Loop XF` / `Loop Crossfade` pair and the French
`Fondu de boucle` do.

### The R3 qualifications, confirmed by an independent model

The R3 screen found NONE at authoring time, but three English words on this page
are the past participles of three controls that also carry captions. Each was
qualified with the perfective marker and an entry-scoped `termNote`. **Round 2, on
a third model, returned all six at 1.00 exact:**

| the preset button | zh | en′ | the control it had to stay distinct from | zh | en′ |
|---|---|---|---|---|---|
| `label.presetTuned` | 已调音 | **Tuned** | `label.tune` | 调音 | **Tune** |
| `label.presetReversed` | 已反向 | **Reversed** | `label.reverse` | 反向 | **Reverse** |
| `label.presetFiltered` | 已滤波 | **Filtered** | `label.groupFilter` | 滤波器 | **Filter** |

The mechanical downstream check (group the zh renderings, report any two
*different* keys sharing one, excluding same-control and same-English pairs) reads
**NONE** on the shipped table. The reverse-side check — two different keys
returning the same en′ — found 8 pairs, and **all 8 are a caption and its own
tooltip title where the ENGLISH already collapses** (`Sustain`/`Sustain`,
`Settings`/`Settings`, `Vintage`/`Vintage`, `Waveform Editor`/`Waveform Editor ·`,
…). **0 genuine cross-control collisions.**

### Accepted drifts, each with its written reason (collision on the page, never drift distance)

- **`label.btnLoad` 载入… → "Loading…"** beside `toast.loading` 正在载入 {name}… →
  "Loading {name}…" (1.00). Two keys, one English stem — but **on the page they
  are distinct**: 正在 is the progressive marker and the button face has none. The
  button is the glossary root verbatim. **The collapse is in the English, not the
  Chinese.** This is a SECOND corpus site for the identical finding O-simpleGrain
  recorded on the identical pair, from two models.
- **`label.fine` / `title|fine` 微调 → "Fine Tune"** on both models, both rounds,
  where `label.tune` 调音 → "Tune". Two separate captions, two separate roots, no
  collision.
- **`调音` read as the gerund "Tuning" by one model and the verb "Tune" (1.00) by
  another.** Chinese does not inflect for it. The three renderings the page needs
  — 调音 / 已调音 / 跨键盘调音 — came back as Tune / Tuned / Tuning-across-the-keyboard.
- **`跨键盘调音` → "Pitch Across Keys"** (haiku) vs "Cross-Keyboard Tuning"
  (sonnet). The reader's word choice, not the string: 跨键盘调音 and 音高 share no
  character and cannot collide on the page. Majority reading is the tuning word.
- **The toast imperatives gained 请** — `toast.dropFolder` and `toast.dropFileType`
  read as "Please drag in…". 请 is the standard Chinese instruction form and adds
  no claim.
- **`label.subtitle` lost its article** ("A Field Guide" → "Field Guide"). Chinese
  has no article.
- **`title|lessonFilteredEnv` 滤波与包络 → "Filter & Envelope"** where the English
  nominalizes as "Filtered & Enveloped". Distinct from 滤波器 (Filter group) and
  已滤波 (the preset face) on the page.
- Every remaining triple is exact or a casing difference, including the W3 rows
  that matter: **Off / Forward / Ping-Pong and Repitch / Stretch survive verbatim
  into the Chinese bodies** — they are `AudioParameterChoice` strings the user
  reads on the combo and stay English under D-01, in both directions.

### Every body names the face the page paints — checked, the Task 2 finding shape

Task 2's single re-authoring came from a body naming a caption that a geometry fix
had changed hours earlier. No tool in this repo compares a zh body against its own
en, and none compares a body against a sibling caption, so it was checked by hand
here:

| body | names | the page paints |
|---|---|---|
| `dropZone` | “载入…” | `label.btnLoad` = 载入… ✓ |
| `loopStart` | 循环模式 | `label.loopMode` = 循环模式 ✓ |
| `loopEnd` | 循环起点 → 循环终点 | `label.loopStart` / `label.loopEnd` ✓ |
| `end` | 开始 / 结束 | `label.start` / `label.end` ✓ |
| `rootKey`, `lessonTunedKeyboard` | 根音调 | `label.rootKey` = 根音调 ✓ |
| `lessonSp1200` | 复古 | `label.groupVintage` = 复古 ✓ |
| `vizAmp` | 起音—衰减—延音—释音 | the four captions, verbatim ✓ |
| `loadSource` | 30 秒 | `label.sourceTruncated` = 已截断至 30 秒 ✓ |

**No body names a face the page does not paint.** That is why there was nothing to
re-author.

---

## Stale-body and dead-class verdicts (K13, C7)

| class | verdict | evidence |
|---|---|---|
| the fixed-pair language enumeration | **LIVE, deleted in en AND fr** | comment-stripped `-CSD` probe **2 → 0** |
| its numeric exception list | **KEPT, re-verified** | markup carries **3** `<select>`; minus the language selector = **2**; *"the two drop-down menus"* is true today |
| the "and nothing else" settings class | **NO SUBJECT — checked non-defect** | `grep -c "'tip.settings'"` = **0**; `grep -c "tips-toggle"` in markup = **0** |
| the "Tuning tab stays English" class | **NO SUBJECT — checked non-defect** | `grep -c scala-tuning-engine` in CMakeLists = **0**; no tuning tab |

The correction was made FIRST and the file **re-read** before a single Chinese row
was authored (C7). The superseded phrasing is in the CHANGELOG and is **not**
respelled in any source comment (C8), which is why the probe reads the file as
fixed.

---

## Font work — the tail half only, and where it lives

`index.html` declares **zero** `font-family` sites on this plugin; all nine are in
the external 939-line `css/styles.css`. **A markup-only census reads a false clean
bill** — the third confirmation in this wave.

```
1  font-family: 'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif
6  font-family: inherit
2  font-family: var(--symbol-font)
```

Two declarations edited: the house stack (`:80`) and the `--symbol-font` token
(`:65`). **Times New Roman is installed and 4th in the stack, so the Latin arm was
already safe** — this is the tail half only; the absent Garamond members were
KEPT (Windows and print intent). The tail goes **before** the trailing generic
(W1).

**N3, and it is measured rather than assumed.** `--symbol-font` has two carriers:
`.gear-btn`, and `.fleuron-row, .fleuron-corner, .preset-fleuron, .op-out`.
**`#gear-btn` is TIP_BINDINGS row 1** — a tip anchor — and the screen reports it as
Han-bearing on a node whose own text is a single gear glyph:

```
{ "id": "#gear-btn", "han": true, "vis": true, "own": "⚙",
  "ff": "\"Segoe UI Symbol\", …, \"PingFang SC\", \"Microsoft YaHei\", serif" }
```

`han` counts `data-tip`, `data-tip-title` and `aria-label`. **The verdict is on
whether the node is an anchor, never on the glyph it paints.**

Verified by **re-running the screen**, never by reading the CSS back:
`undeclared-font: 0` against **95** visible Han-bearing nodes, and a scripted check
that **0 of the 95** resolve without a CJK face.

---

## Zeros recorded AS MEASURED

- **gate files: 0.** `find plugins/O-simpleSampler/tests -name '*.js'` returns
  nothing; `tests/` holds `i18n-states.json` and a C++ `render-harness/`. Fired
  with `find`, never a glob. **No gate was invented** (C5/C6 has no subject).
- **duplicate keys: NONE**, F13-corrected form (LANGUAGES members excluded),
  `keys=76`, before and after.
- **language `AudioParameterChoice`: none.** Each declaration's own text was
  scanned, not the containing file: the two on this plugin are `loopMode`
  (`Off`/`Forward`/`Ping-Pong`) and `pitchMode` (`Repitch`/`Stretch`).
- **Han under `Source/`: 0**, negative grep across every `.h`/`.cpp`, with the
  positive control firing on `js/i18n.js` in the same run.
- **`zh-Hans` in the comment-stripped `PluginProcessor.h`: 2** (encode and
  decode), and both functions report **THREE-WAY** when matched by name.
- **The `juce::Identifier`-wrapped persistence key needs no change** — the wave's
  only one. `kLanguageProp = "uiLanguage"` still round-trips the language CODE;
  the runtime form is still an index.
- **submodule untouched.** `git status --short -- plugins/O-Orbit/libs/SAF` →
  empty; `git submodule status` → ` b6fe1882… (v1.3.4)`, leading space = clean.
  The guard ran before **both** commits even though no path here is inside it.
- **tags created: 0.** **`PLUGINS.md` touched: no** (Task 7 owns it).
- **file deletions across both commits: 0.** **files staged outside
  `plugins/O-simpleSampler/`: 0.**
- **`modules/webview-drop-streaming.js` changed: no.**

## Gate results

| gate | result |
|---|---|
| `check-i18n --plugin` | **exit 0** — `got ["en","fr","zh-Hans"]`, `36 I18N + 73 LABELS` unchanged, **`37 tip(s) bound`** unchanged, `[12] 1 module(s): js/app.js` |
| `i18n-zh-lint --plugin` | **0 findings**, 145 rows / 109 entries, **BELOW SHIP BAR 0** |
| `i18n-zh-lint --self-test` | **10/10** |
| `check-ui-labels --plugin` | **exit 0**, `== ALL CHECKS PASSED ==`, **5 green geometry-diff assertions on each non-English arm** |
| `measure-ui --verbose` | **0** unresolved or skipped states — all four applied |
| `boot-all-uis --strict-tips` | **clean 1/1**, 0 warn, 0 failed, **0 DEAD, 0 late** |
| `i18n-zh-lint` repo-wide | 0 findings across 0 plugins, 1 unreadable — exit 2, the **K2 baseline**, unchanged |
| `i18n-fr-lint` repo-wide | 0 of 44 with findings, 1 unreadable. **No French rendering changed** |
| `check-i18n` repo-wide | `ALL CHECKS PASS — 43 localized plugin(s)` |
| **corpus** | **4905 → 5050** of 5441 rows; **39 → 40** plugins carrying zh |

`auval` **not run** — Task 7's single cold sweep. Budget ~85 s, not the stale
15-minute figure.

---

## Commits

| hash | subject |
|---|---|
| **`c5effaac`** | `i18n(O-simpleSampler): add Simplified Chinese at reviewed:'mt' — 145 rows, the second carrier of the unscanned drop module` |
| **`b1d9e6b2`** | `i18n(O-simpleSampler): promote 145 zh rows to reviewed:'bt' after a two-round blind reverse read — v1.5.0` |

Both path-scoped `git commit -F <file> -- plugins/O-simpleSampler`, options before
the `--` (F14.6). `git branch --show-current` and `git status --short` re-checked
immediately before each, never once at the start. **The submodule guard ran before
each** and printed `submodule guard: clean` both times. Nothing under
`.claude/agent-memory/`, nothing under `.planning/`, nothing under
`plugins/O-Strata/`, and nothing outside `plugins/O-simpleSampler/` was ever
staged — verified after the fact: **0 files outside the plugin across both
commits, 0 deletions.**

## Build and install

`./scripts/build-and-install.sh O-simpleSampler`, backgrounded and polled on a
sentinel — **exit 0, 51 s** (04:09:05Z → 04:09:56Z). The folder name equals the
`juce_add_plugin` target here, so this is not one of the two plugins where it does
not. CLAUDE.md's cache sequence ran first: `killall -9 AudioComponentRegistrar`,
`rm -rf ~/Library/Caches/AudioUnitCache/`,
`rm -rf ~/Library/Caches/com.apple.audiounits.cache`.

```
VST3: ~/Library/Audio/Plug-Ins/VST3/O-simpleSampler-dev.vst3            6.3M, CFBundleShortVersionString 1.5.0
AU:   ~/Library/Audio/Plug-Ins/Components/O-simpleSampler-dev.component 6.3M, CFBundleShortVersionString 1.5.0
```

**The version reached the binary, not just the CMakeLists**, and it is the number
the registry row has to be corrected to.

---

## Deferred items

1. **`tests/render-harness/CMakeLists.txt` carries a FROZEN version literal — the
   SECOND site of the pattern in this wave.**
   `plugins/O-simpleSampler/tests/render-harness/CMakeLists.txt:65-66` hard-codes
   `JucePlugin_VersionString="0.1.0"` / `JucePlugin_VersionCode=0x000100`. It has
   never tracked the plugin version — it is at 0.1.0 against a plugin that shipped
   1.5.0 today. Unlike O-simpleGrain's, it is a dead literal rather than a `${VAR}`
   mirror, so it does not desynchronise on a bump. Out of scope here (wave 4e D6).
   Task 1 found the same shape on O-simpleAdditive at `1.0.2`. **Two of the three
   render-harness trees examined in this wave carry it, which makes the suite-wide
   sweep Task 1 suggested a measurement rather than a guess.**
   **Grep-able token: `JucePlugin_VersionString="0.1.0"`.**

2. **K16 is wrong: this plugin's fourth state is an `eval`, not a click.** The plan
   asserts 19 click states across the wave and `4 of 4` on this plugin;
   `tests/i18n-states.json` carries three clicks and one
   `"eval": "window.__stubStates.combos.get('pitchMode').setChoiceIndex(1)"`. The
   arithmetic is wrong by at least one and any remaining executor should re-read
   its own states file. The consequence is that the `elementFromPoint` probe
   cannot fire on that state and its effect assertion is the whole verdict.
   **Grep-able token: `setChoiceIndex(1)` in `plugins/O-simpleSampler/tests/i18n-states.json`.**

3. **The emitter has no product-name control.** `grep -n "product"
   scripts/i18n-zh-backtranslate.js` returns nothing, so R6's *"expect the
   product-name control to report a non-zero count"* has no subject in this version
   of the script. Three tasks have now been told to read a line that is not
   printed. Either the control was removed, or the rule describes a script that was
   never written. **Either the rule text or the script needs correcting** — an
   executor following the rule literally will look for a line, not find it, and
   have to choose between reporting a false zero and adjudicating by hand.
   **Grep-able token: `product` in `scripts/i18n-zh-backtranslate.js` (currently 0 hits).**

4. **The unscanned drop-streaming module ships English toasts on this plugin's
   Chinese page too** — 17 distinct strings across 22 `opts.showToast(` call sites,
   byte-identical to O-simpleGrain's copy (sha256 `349a0c28…`). This is the
   SECOND consumer, which is what makes keying it a two-plugin change rather than a
   one-file fix. Carried from Task 2's deferred item 1 unchanged; the root fix is
   widening `check-i18n`'s `[12]` module scan past the `js/` directory, which is
   blind on all 44 plugins.
   **Grep-able token: `opts.showToast('Scanning folder…')`.**

5. **`'load your own' → 载入自己的` is a dangling modifier and this is its SECOND
   English site**, as Task 2 predicted it would be. Rendered 载入自己的素材 here
   with the same reasoned `termNote` and the same head noun, so the two plugins do
   not diverge. The glossary root still needs a glossary-level decision.
   **Grep-able token: `'load your own':` in `scripts/i18n-zh-glossary.js`.**

6. **`.settings-label` carries `min-width: 0`, which is not "no floor" (R8/F7).**
   The flex-item default is `auto` (content size), so `0` explicitly permits
   shrinking BELOW content. It was not load-bearing here — the finding on that
   selector was a height, and the width never moved on any arm — but it is a live
   instance of the trap on a plugin in this wave, recorded so a later width pass
   removes the declaration and measures before restoring it.
   **Grep-able token: `min-width: 0;` in `plugins/O-simpleSampler/Source/ui/public/css/styles.css`.**

---

## A note on the shared checkout

No concurrent commit landed between this task's two commits. The discipline was
applied identically regardless: branch and staging re-checked immediately before
each commit rather than once at the start, every commit path-scoped, the submodule
guard fired before each, and `.claude/agent-memory/research-planning-agent.md` —
still modified, still another session's file — never staged.

---

## Self-Check: PASSED

Files claimed, verified on disk:

- `plugins/O-simpleSampler/Source/ui/public/js/i18n.js` — FOUND
- `plugins/O-simpleSampler/Source/ui/public/index.html` — FOUND
- `plugins/O-simpleSampler/Source/ui/public/css/styles.css` — FOUND
- `plugins/O-simpleSampler/Source/PluginProcessor.h` — FOUND
- `plugins/O-simpleSampler/CMakeLists.txt` — FOUND
- `plugins/O-simpleSampler/CHANGELOG.md` — FOUND

Commits claimed, verified in `git log --all`:

- `c5effaac` — FOUND
- `b1d9e6b2` — FOUND

Scope claims, verified:

- files staged outside `plugins/O-simpleSampler/` across both commits — **0**
- file deletions across both commits — **0**
- paths staged under `plugins/O-Orbit/libs/SAF` — **0**; submodule still at
  `b6fe1882` (v1.3.4), clean
- tags created — **0**
- `PLUGINS.md` touched — **no** (Task 7 owns it, including the ✅ Working →
  📦 Installed flip this task supplies the evidence for)
- `ROADMAP.md` / `STATE.md` / `PLAN.md` / this summary committed — **no**
- `modules/webview-drop-streaming.js` changed — **no**, and still byte-identical
  to O-simpleGrain's copy
