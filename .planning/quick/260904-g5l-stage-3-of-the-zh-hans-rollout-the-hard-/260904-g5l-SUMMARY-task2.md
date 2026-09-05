---
phase: quick-260904-g5l
plan: 01
task: 2
subsystem: i18n
status: complete
tags: [i18n, zh-Hans, O-Bitrot, O-Chorus, geometry, gates]
requirements: [ZH3-02, ZH3-05, ZH3-08, ZH3-09]
commits:
  - 88c36535
  - 43246871
  - d788b769
  - 4a26b265
  - d702f6f4
key-files:
  modified:
    - plugins/O-Bitrot/Source/ui/public/js/i18n.js
    - plugins/O-Bitrot/Source/ui/public/index.html
    - plugins/O-Bitrot/Source/PluginProcessor.h
    - plugins/O-Bitrot/Source/PluginProcessor.cpp
    - plugins/O-Bitrot/Source/PluginEditor.cpp
    - plugins/O-Bitrot/tests/ui_tooltip_clamp_check.js
    - plugins/O-Chorus/Source/ui/public/js/i18n.js
    - plugins/O-Chorus/CMakeLists.txt
    - plugins/O-Chorus/CHANGELOG.md
    - PLUGINS.md
    - plugins/O-Bitrot/CMakeLists.txt
    - plugins/O-Bitrot/CHANGELOG.md
  untouched_deliberately:
    - scripts/i18n-zh-glossary.js          # no new measured BUDGETS cell was found
deferred:
  - auval -v for O-Octagon, O-Bitrot and O-Chorus — ONE end-of-batch cold rescan
actuals:
  tokens: 246000
  tasks: 1
  commits: 5
---

# Stage 3 Task 2 — O-Bitrot and O-Chorus into Simplified Chinese (COMPLETE)

**O-Bitrot v1.16.0** and **O-Chorus v1.6.2** are built, installed and shipping
three languages. **Every zh-Hans row in both is `reviewed: 'bt'`.** The repo-wide
stage view now reads **462 rows / 462 `bt` / 0 `mt`** across all three converted
plugins.

**O-Bitrot's standing 2-FAIL French geometry baseline is CLOSED** —
`check-ui-labels --plugin O-Bitrot` went from **exit 2 with 2 FAIL** to **exit 0
with 0 FAIL** (ZH3-09). Its clamp gate derives its language list from the table
and drives a real Chinese pass with 14 clamps engaged (ZH3-05). O-Chorus reports
**0 entries below the ship bar** (ZH3-08).

| | O-Bitrot | O-Chorus |
|---|---|---|
| entries / rows | 117 / **172** | 28 / **44** |
| at `reviewed: 'bt'` | **172 / 172** | **44 / 44** |
| version | `1.15.2` → **`1.16.0`** | `1.6.1` → **`1.6.2`** (see finding 14) |
| `PLUGINS.md` row | `1.15.1` → `1.16.0` / `2026-09-04` | `1.5.0` → `1.6.2` / `2026-09-04` |
| build + install | exit 0, 57 s, bundles report `1.16.0` | exit 0, 49 s, bundles report `1.6.2` |
| reverse rounds | **2** (172 + 7 = 179 triples) | 1 (44 triples) |

**`auval` is DEFERRED to one end-of-batch cold rescan** covering O-Octagon,
O-Bitrot, O-Chorus and Task 3's plugin. A cold `auval` after an install rescans
the whole AU registry (~15 min) and Task 3 builds again.

## The review that actually happened: two rounds, 179 triples on O-Bitrot

**`'bt'` asserts that a second, independent pass — one that never saw the English
source — rendered the Chinese back into English and the drift was read against
the original.** On O-Bitrot that took two rounds, because the first one found
seven rows that had to be re-authored rather than accepted.

### Round 1 — all 172 rows, and NOT ONE finding was in the twelve the tool prints

`--ingest` truncates to `MAX_SHOWN` = 12 and says `… 160 more (--verbose)`.
**Twelve is 7% of this batch.** The seven rows that needed re-authoring ranked
**1st, 14th, 39th, 50th, 56th, 79th and 84th** by lexical score; **four scored
0.50 or above**, well inside the "looks fine" band. The `CRUSH_ENABLE` body — the
one whose back-translation is not a sentence — scored **0.73** and ranked 79th.

**The `抖动` collision, and how it was caught.** `Dither` and `Jitter` share one
glossary root and sit two cells apart in the same Crush plate. My first draft
kept the root on Dither and qualified only Jitter:

| row | zh | en' |
|---|---|---|
| `label.dither` | 抖动 | **Jitter** |
| `CRUSH_DITHER` title | 抖动 | **Jitter** |
| `CRUSH_JITTER` title | 时基抖动 | Timebase jitter ✓ |
| `CRUSH_ENABLE` body | …以及抖动。 | *"…decimation with **timebase jitter, and jitter**."* |

`Z5` reported **zero findings** on that table — `抖动` is exactly what the
glossary asks for, on both keys. `F1` was silent (the rendering is accepted, so
containment never runs). `check-ui-labels` was silent (the captions are different
strings). **Every automated check in this repo passed a table that ships an
unreadable sentence.** Only the blind reverse pass found it.

The other four:

| entry | what en' revealed | fix |
|---|---|---|
| `edgeBtn` body | `旁通事件边界处的短交叉淡化` parsed as an **attributive**, not the imperative verb — *"A short crossfade at the boundaries of a bypassed event"*, which describes nothing the control does | `将…旁通掉`, which forces the verb reading |
| `CRUSH_ENV_AMT` body | `的幅度` attached to `位深` rather than to the clause — *"the input envelope pulls on the bit-depth amount"* | `能把位深推动多少` |
| `seedRo` body | `走带位置` came back **"tape position"**. On a plugin whose first family IS tape that is a live confusion; the English means the HOST transport | `宿主走带位置` |
| `TAPE_WOW` body | a bare `抖动` for *flutter* — a **third** sense of the token on a page already disambiguating two | `更快的快抖`, the term its own title establishes |

### Round 2 — the seven changed rows, fresh salt, fresh reader

Re-emitted under a **fresh per-batch salt**, so the correction batch shares no id
with round 1 and the reverse agent could not correlate it against anything it had
seen. Read by a **fresh** claude-sonnet-5 subagent with **no round-1 exposure**,
handed the batch file and nothing else.

**7 joined, 0 REFUSED, 0 unjoinable. Every one now says what its English says:**

| zh | en' |
|---|---|
| 抖动噪声 (×2) | **"Dither noise"** |
| 将事件边界处的短交叉淡化旁通掉，… | **"Bypasses the short crossfade at event boundaries, making entry and exit true steps."** |
| 双极性：输入包络能把位深推动多少。 | "Bipolar: how much the input envelope can push the bit depth." |
| …同一个宿主走带位置上… | "…at the same **host transport** position…" |
| 启用压碎族 —— 位深削减、带时基抖动的采样率抽取，以及抖动噪声。 | **"…bit depth reduction, sample rate decimation with time-base jitter, and dither noise."** — a sentence |

**Both sides of the collision are now qualified: Jitter `时基抖动` (time-base
jitter), Dither `抖动噪声` (dither noise).** Dither *is* noise added before
quantisation, so the qualifier is the definition, not a hedge.

### One drift read and ACCEPTED in round 2, with its reason

`TAPE_WOW`'s `快抖` came back *"the faster jitter"* where the English says
**flutter**. **Kept.** `慢抖` is the glossary ROOT for `Wow` and `快抖` is its
partner — the pair IS this entry's own title, `慢抖与快抖`. A contextless reader
renders `抖` as "jitter" because `抖` is the shared morpheme, not because the
Chinese names the Crush control: `快抖`, `时基抖动` and `抖动噪声` are three
distinct strings and none collides on the page.

**This is materially different from the round-1 defect, where `抖动` was
BYTE-IDENTICAL to another control's caption.** The discriminator that separates
"accept" from "re-author" is not how far en' drifts — it is whether the Chinese
on the page can be confused with another control on the same page.

### Round 1's accepted drifts, each with its reason recorded at the entry

An accepted drift with no recorded reason is indistinguishable from an unread
one. `隐藏` (Conceal) → *"Hide"* — `丢包隐藏` is the standard Chinese for
packet-loss concealment and the tip title carries it in full. `失落` (Drop) →
*"Dropout"*, the English word itself. `程度` (Severity) → *"Amount"* — root,
pinned by `光盘损坏的程度` in the body. `腐化` (Rot) → *"Corruption"*, the word
the French entry uses. `慢抖` (Wow) → *"Slow jitter"* — root. `来源`
(Provenance) → *"Source"* — the group holds the seed ledger. `每转步长` →
*"Steps per revolution"*. Chinese needs no abbreviation, so `位深` / `丢包` /
`舒适噪声` render caption and tooltip title identically where English
abbreviates. Number- and part-of-speech-only: `硬边缘`→"Hard edge",
`爆音`→"Pop", `预设`→"Preset", `乱码`→"Garbled", `剪接`→"Splice",
`20 ms 数据包`→"20 ms packet".

**O-Chorus: zero drifts across all 44 triples**, including the 38 rows already at
`'bt'`, which got a second independent reading and all held.

### Provenance, all three rounds

| round | string |
|---|---|
| O-Bitrot forward, r1 | `forward zh-Hans draft authored by the Stage-3 Task-2 executor (Claude Opus 4.6, GSD quick-260904-g5l), 2026-09-04` |
| O-Bitrot reverse, r1 | `reverse pass O-Bitrot batch: blind zh->en by claude-sonnet-5 subagent, batch file only, no manifest, no plugin context, 2026-09-04` |
| O-Bitrot forward, r2 | `forward zh-Hans CORRECTION draft (7 rows re-authored after the round-1 triple read) by the Stage-3 Task-2 executor (Claude Opus 4.6, GSD quick-260904-g5l), 2026-09-04` |
| O-Bitrot reverse, r2 | `reverse pass O-Bitrot correction batch: blind zh->en by fresh claude-sonnet-5 subagent, batch file only, no manifest, no plugin context, no round-1 exposure, 2026-09-04` |
| O-Chorus reverse | `reverse pass O-Chorus batch: blind zh->en by claude-sonnet-5 subagent, batch file only, no manifest, no plugin context, 2026-09-04` |

**All blinding controls were fired, not assumed**, on every batch:

| control | O-Bitrot r1 | O-Chorus | O-Bitrot r2 |
|---|---|---|---|
| ids opaque 12-hex | 172/172 | 44/44 | 7/7 |
| no plugin name or key fragment | 0 | 0 | 0 |
| ids returned identical, in order | ✓ | ✓ | ✓ |
| rows well-formed (`NF==2`) | 172 | 44 | 7 |
| joined / REFUSED / unjoinable | 172 / 0 / 0 | 44 / 0 / 0 | 7 / 0 / 0 |

**`'native'` stays OPEN and is not a blocker. This project has no native Chinese
reader.** A disclosed quality level, not a hidden one.

# ══ STRUCTURAL FINDINGS — READ THIS BEFORE THE STAGE-4 WAVES ══

Thirty-nine plugins across seven waves will copy whatever this stage
establishes. Everything below cost measurement to find and would otherwise be
hit blind. Findings 1, 2, 5 and 6 are **new** — Task 1 could not have seen any
of them, because O-Octagon's table was uniformly at `'mt'`, had no homograph
collision, had no content-sized `<select>`, and had no per-plugin gate that
already drove a language.

## 1. `--emit` is TABLE-scoped, not SHIP-BAR-scoped — and Task 1 could not see it

The plan's verify says `wc -l < "$SCRATCH/o-chorus-zh.tsv"` → **expect 4**.
It emitted **44**.

`i18n-zh-backtranslate.js --emit` walks every row that has a `zh-Hans` value and
**ignores `reviewed` entirely** (`readPlugin()`, `scripts/i18n-zh-backtranslate.js:108-127`).
There is no `--only-mt` flag; `val()` parses `--plugin`, `--out`,
`--forward-provenance`, `--manifest`, `--provenance`, `--verbose` and nothing
else.

On O-Octagon this was **invisible**: all 246 rows were at `'mt'`, so
"every row" and "every unreviewed row" were the same set. O-Chorus is the first
table in the repo with a MIXED bar — 38 at `'bt'`, 6 at `'mt'` — and it is where
the difference first shows.

**Consequence for Stage 4.** Any plugin that lands its table in two passes — a
first batch, then a correction — re-emits **everything**, including rows already
at `'bt'`, on the second pass. That is not harmful (the extra rows get a second
independent reading, which is free verification) but it makes every row-count
expectation written against the ship bar wrong, and a wave executor who trusts
the plan's number will think the tool is broken.

I sent all 44. Trimming to 6 by hand would have discarded a second blind reading
of 38 shipped strings for no gain.

## 2. TWO English words on ONE page can share ONE glossary root — and NO LINT CAN SEE IT

`Dither` and `Jitter` both resolve to **抖动** in the 552-term glossary. On
O-Bitrot they are two knobs **two cells apart in the same Crush plate**.

**This is the single most important finding of Task 2, and it is important
because of HOW it was caught.** `Z5` reports zero findings on the wrong table:
`抖动` is exactly what the glossary asks for, on both keys. `F1` is silent — the
rendering is accepted, so containment never runs. `check-ui-labels` is silent —
the captions are different strings (`抖动` vs `时基抖动`), so nothing collides.
**Every automated check in this repo passes a table that ships an unreadable
sentence.** Only the blind reverse pass found it, and it found it three times:

| row | zh | en' |
|---|---|---|
| `label.dither` | 抖动 | **Jitter** |
| `CRUSH_DITHER` title | 抖动 | **Jitter** |
| `CRUSH_ENABLE` body | …带时基抖动的采样率抽取，以及抖动。 | *"…decimation with **timebase jitter, and jitter**."* |

The same shape twice more: `Mains` → root **主输出**, a mixer's main OUTPUT bus.
O-Bitrot's `Mains` is the **electrical** mains — it sets the 50/60 Hz hum
frequency of a line-noise bed. Two senses of one English word, and the root
renders the other one.

**The resolutions, and the reasoning a wave should copy:**

| English | shipped | root | why |
|---|---|---|---|
| Jitter | **时基抖动** | 抖动 | time-base jitter — standard Chinese audio usage |
| Dither | **抖动噪声** | 抖动 | dither *is* noise added before quantisation, so the qualifier is the definition |
| Mains | **市电** | 主输出 | the root is the *other* sense; 市电 is the power supply |

**Qualify BOTH sides, not one.** My first draft qualified only Jitter, on the
theory that jitter idiomatically takes a qualifier and dither does not. That
theory is correct about Chinese usage *in isolation* and useless *on a page that
carries both controls*: the moment one member of a homograph pair is qualified,
the unqualified one reads as the general case — which is what the reverse pass
returned, twice.

**Wave guidance, and this is a three-line script:** before authoring a table,
push the plugin's English label set through `TERMS` and look for **two different
keys mapping to the same array**. It is the only way to find this before it
ships, because nothing downstream will.

## 3. The font tail went on ONE token, and 3-for-3 confirms the two nodes a keyed scan misses

Same shape as O-Octagon: **all 13** `font-family` declarations in
`index.html` read `var(--serif)`. One token, one edit.

- O-Chorus: five appends against eight literal stacks.
- O-Octagon: two, on two tokens.
- O-Bitrot: **one**, on one token.

**Grep for `--*:` type tokens FIRST.** A wave executor who edits the thirteen
declarations makes twelve pointless edits.

The measured resolution set: **118 nodes** through `--serif`, **1** omission —
`#diceBtn.die`, which declares no `font-family` and takes Chromium's UA button
default (Arial). It is reached by the Han scan only through `data-tip`; the die
face is the glyph U+2685, and a `data-tip` paints into `#tooltip`, which **is**
`--serif`. There is no Han glyph to fall back for.

The two nodes a `[data-i18n]`-derived scan would have missed are, for the
**third consecutive plugin**, the same two: the `#tooltip` surface (filled from
`data-tip` at hover time, never keyed) and the endonym `<option>` (the only Han
in the markup). **3-for-3. Treat it as universal.**

## 4. The two content-sized `<select>`s on one page moved in OPPOSITE directions

This is the single most transferable geometry finding of this task.

| element | en | fr | zh | pinned | direction |
|---|---|---|---|---|---|
| `#viewSync > select.field` | 58 | 65 | **66** | **66** | Chinese GREW |
| `select.field[data-param="PACKET_CONCEAL"]` | **82** | 82 | 57 | **82** | Chinese SHRANK 25 px |

Same element type, same page, same mechanism — with `appearance: none` Chromium
still derives a `<select>`'s intrinsic width from its **widest option's** font
run, not from the selected one — and opposite outcomes, decided entirely by the
copy. `#viewSync`'s widest option is `1 bar` / `1 mes.` / `1 小节`, where the Han
form is wider than the English. `PACKET_CONCEAL`'s four faces are
静音 / 重复 / 衰减 / 替换, **all exactly 23.30 px**, against `Substitute`'s 48.02.

The 25 px shrink pulled the Comfort knob column 8.3 px left, which surfaced as a
*knob* moving — nothing about the report points at a `<select>`.

> **Any gate assertion phrased "the non-English pass must not GROW box X" is
> vacuous against Chinese, and any assertion phrased "must not SHRINK" is
> vacuous against half of it. Assert EQUALITY.** Pin at the widest of all three.

## 5. `line-height: normal` — 185 movers, seven pins, and one class that needed TWO

`index.html` declares `line-height` on only six rules, so everything else
inherits `normal` — the **font's own metrics** — and Han faces carry taller ones.
The first Chinese run moved **185** non-label elements.

Seven unitless pins at the measured English used line-height closed 161 of them:

| selector | font-size | EN used line-height | unitless |
|---|---|---|---|
| `.ctl-label` | 9.5 px | 10 px | 10/9.5 |
| `.g-label` | 9 px | 10 px | 10/9 |
| `.settings-label` | 9 px | 10 px | 10/9 |
| `.annot` | 8.5 px | 10 px | 10/8.5 |
| `.preset-btn` ×3 | 10 px | 11 px | 1.1 |
| `.p-head .en` | 10 px | 11 px | 1.1 |
| `#clockModeSeg button` | 11 px | 12 px | 12/11 |

**`.caption` needed TWO pins for one class** — the same trap O-Octagon hit on
`.vcell-value`. Seven plate captions render at 12 px (EN line box 15) and the
global strip's renders at **11 px from an inline style** (EN line box 12). One
ratio cannot serve both:

```css
.p-head .caption          { line-height: 1.25; }        /* 15 / 12 */
.global .p-head .caption  { line-height: 1.0909091; }   /* 12 / 11 */
```

**And a pin can leak DOWN into a child at a different size.** `#edgeBtn` holds
two inline children — the caption span at 11 px and a fleuron U+2766 at 8 px.
Pinning the button alone inherits `12/11` onto the fleuron and moves its box on
the **English** arm (9 → 8.73 px). The fleuron is pinned back at its own measured
ratio, `9/8`. This is the mirror of Task 1's finding 6: that one was a *sibling*
specificity collision, this one is *inheritance* into a differently-sized child.
Both move English, both are invisible without running every language in one run.

**Not pinned, each for a measured reason** — recorded because a wave that pins
everything it measured can no longer tell a needed pin from a decorative one:
`.ro` / `.seg button` / `.caption > em` never carry Han (readouts are English by
D-03, plate numbers are `I18N_EXEMPT`, segment faces are units);
`.settings-toggle` declares `height: 21px`, so its line box cannot move its box;
`.plate` / `.brand` inherit an explicit `line-height: 1.45` that `normal` never
reaches; `.preset-menu-item` / `#preset-name` show preset names, which are the
JSON filenames on disk (D-02) and never localize.

## 6. The canon DID reach the inline-module controller — asserted, not inferred

O-Bitrot has **no `app.js`**. Its controller is one inline
`<script type="module">` at `index.html:1429`, with the canon body inline
alongside it. This was the non-standard layout the Stage-0 P1/P2 sweep's
"all 43 shipping copies" claim was least likely to cover.

`check-i18n` assertion 6 byte-compares each copy against
`scripts/i18n-canon.js`, and it PASSES here, naming the shape explicitly:

```
PASS: [O-Bitrot] [6] the inline <script type="module"> in index.html carries the
      canonical i18n.js import line verbatim (or its './js/i18n.js' inline-module
      form) — canon v2
PASS: [O-Bitrot] [6] the applyI18n/initI18n region matches scripts/i18n-canon.js
      (canon v2) — on v2
PASS: [O-Bitrot] [12] there is shipped page JS to scan — 1 module(s): the inline
      <script type="module"> in index.html
```

**That is the proof, and it is now stated rather than left inferred.** Assertion
12 also finds and scans the inline module, so the no-`app.js` shape costs a wave
nothing beyond knowing to look in `index.html`.

## 7. `ui_tooltip_clamp_check.js` had ONE hard-coded pair and NO second trap — but its discriminator was the wrong shape

The literal at `:437` was `const LANGS = ['en', 'fr'];`. It is now derived from
the table's own `LANGUAGES`, and **there is no default pair** — the gate prints
an ABORT and exits non-zero if `LANGUAGES` is unreadable. A gate that falls back
to a pair goes green on unchecked content, which is worse than one that fails,
because the failure is visible and the silence is not.

**The Stage-2 "second hard-coded pair twenty lines below" trap is NOT present in
this file.** `grep -nE "'fr'|\"fr\"|'en'|\"en\"|length <= 2"` returned exactly
four sites — the literal, the copy-differs block, the stats print, and the
`__setLanguage('en')` restore — and no `drivenStates.length <= 2` recording
guard. Confirmed rather than assumed, and the answer was clean.

**What WAS wrong was the discriminator's shape.** The Stage-D print read:

```js
console.log(`French costs … and is ${(f.tallest - e.tallest)} px taller at its tallest.`);
```

Framing that assumes the non-English pass costs MORE. Measured on this page:

```
en:      55 anchors  clamped 15  flipped-below 10  tallest 119.1
fr:      55 anchors  clamped 16  flipped-below 13  tallest 133.9
zh-Hans: 55 anchors  clamped 14  flipped-below  9  tallest  92.4

fr costs +3 vertical flip(s) and +1 clamp(s); tallest +14.8 px (TALLER)
zh-Hans costs -1 vertical flip(s) and -1 clamp(s); tallest -26.7 px (SHORTER)
```

**Chinese is 26.7 px SHORTER at its tallest and costs one FEWER clamp and one
FEWER flip.** The new assertion gates on `dTall !== 0 || dWide !== 0 ||
clamped !== clamped` — **differing, in either direction** — and *reports* the
direction rather than requiring one. The copy-differs assertion now runs per
non-English language rather than once against `fr`.

Non-vacuity is proved on the zh arm by **14 clamps engaged**, not zero.

## 8. The C-7 inventory: 14 → **13** comment-stripped, but the RAW grep still says 14

Task 1's finding 8 reproduced exactly, and I hit its trap while fixing it. My
first draft of the new ZH3-05 comment block spelled `const LANGS = ['en', 'fr'];`
and `['en','fr']` verbatim while explaining why they were removed — which would
have kept O-Bitrot on the inventory forever, exactly as O-Chorus still is. Both
were reworded to prose.

**After Task 2, the comment-stripped inventory is 13 files. The plan's Task-3
verify says "expect 12" after Task 3 — that is correct for the STRIPPED form and
wrong for the raw grep, which will report 13.** The difference is O-Chorus's
comment, and an executor reading the raw number will chase a phantom. Verified
after this task: stripped **13**, raw `grep -rln` **14**.

```
O-Bassoon O-Bells O-Bowed O-Comp O-Emulator O-Reed O-SimpleReverb
O-Texture O-TextureForge O-Wind            → tests/ui_tip_render_check.js
O-MicrotonalSampler                        → tests/ui_tip_render_check.js  (Task 3)
O-ReverseDelay O-Tapestop                  → tests/ui_tooltip_clamp_check.js
```

**O-ReverseDelay and O-Tapestop carry the SAME `ui_tooltip_clamp_check.js` file
O-Bitrot did.** The three-part fix committed here — derive-or-abort, per-language
copy-differs, direction-agnostic geometry discriminator — ports to both nearly
verbatim. That is two of the thirteen already solved on paper.

Use the stripped form:
```bash
for f in plugins/*/tests/*.js; do
  grep -v '^\s*//' "$f" | grep -v '^\s*\*' | grep -q "en,fr\|\['en', *'fr'\]" && echo "${f#plugins/}"
done | sort
```

## 9. The stale-enumeration defect is systemic, and it is now confirmed on THREE plugins

O-Octagon (Task 1), O-Bitrot and O-Chorus all shipped a `lang-select` /
`tip.language` body that **counted the selector's options** and went false the
moment a third `<option>` landed.

- **O-Bitrot** en: *"English and French are available"* — false at v1.16.0.
- **O-Chorus** en: *"English or Français."* / fr: *"English ou Français."* —
  false since **v1.5.0**, shipped, and missed by every gate for a day.

Fixed the same way in all three: **the enumeration is REMOVED, not extended.**
Naming all three would put Han inside the `en` and `fr` bodies, which moves the
English tooltip's own geometry and drags the CJK tail onto the very baseline
every gate measures against. The selector already lists them in their endonyms —
the one form a reader recognises without knowing the page language.

**O-Chorus's `zh-Hans` body enumerated all three and was TRUE.** It came out
anyway. Three bodies describing one control differently is precisely what let
the other two rot unnoticed, and a blind reverse pass on the zh row would have
returned a sentence its English source no longer contained — reading as a large
drift caused by the process rather than by the translation.

**This is a 43-plugin sweep waiting to happen.** Every plugin with a language
selector has a body describing it, and every one of those bodies is a candidate.
No gate can see it: the sentence is grammatical, the tooltip renders, the lint
has no notion of "counts its own options".

## 10. `tip.tipsToggle`'s standing `'mt'` comment: READ, and its reason is exactly why it is in this batch

`plugins/O-Chorus/Source/ui/public/js/i18n.js:671-679` states it is deliberately
the file's one entry below the bar:

> `'bt'` asserts that a SECOND, INDEPENDENT pass — one that never saw the English
> — rendered this Chinese back into English and the drift was read. […] the BODY
> is new prose written in this same session, and the session that wrote it cannot
> be the session that blindly reverses it. […] Queued for the next reverse batch.

**The reason still holds, and it is honoured rather than overridden.** This WAS
the next reverse batch. The comment named the exact command that produces it, and
nothing about the entry needed superseding — it needed the pass it was waiting
for, and got it. The blind reverse pass returned *"Toggles this hover help. When
off, only the settings gear and this switch keep explaining themselves."* against
an English source it never saw, and the entry is now `'bt'`.

**Wave guidance: a comment that reserves an entry below the ship bar and names
the command that clears it is the right shape.** It survived a version bump, a
language rollout stage and a change of executor without anyone having to
re-derive why the entry was held — and it told the next session exactly what to
run. Copy the shape, not just the flag.

## 11. `grep -c 'zh-Hans' PluginProcessor.h` returns **3**, not the 2 the plan expects

The same comment-blind-grep defect Task 1 recorded (which returned 4). Two in the
code — `languageCode`, `languageIndex` — and one in the doc comment that explains
them. The code count is exactly 2:

```bash
grep -nE 'static (juce::String|int)' -A3 plugins/O-Bitrot/Source/PluginProcessor.h | grep -c 'zh-Hans'   # 2
```

**A comment-blind grep used as a code gate is the same defect as finding 8, and
it now appears in this plan three times.** Stage 4 should write these as
comment-stripped from the start.

## 12. The past-prescan-window charset question does not arise here, and finding 4 of Task 1 says why

O-Bitrot's `<meta charset>` sits at **byte 46**, well inside the 1024-byte
window. Independently, `PluginEditor.cpp` serves `index.html` as
`text/html; charset=utf-8` — the same transport guarantee Task 1 found on
O-Octagon. The numeric-entity endonym is kept anyway, for one convention across
all 43 plugins.

## 13. Neither `tests/ui_preset_menu_check.js` nor the harness needed a language arm

`grep -cE "lang|i18n|french|locale|LANGUAGES"` over
`plugins/O-Bitrot/tests/ui_preset_menu_check.js` returns **0**, and it exits 0.
The grounding said "no language sweep, but confirm rather than assume" — the
answer is clean. It measures the preset menu, whose contents are JSON filenames
that never localize (D-02), so a language arm there would be measuring nothing.

## 14. THREE stale version numbers pointed at O-Chorus, and the CMakeLists was right

The plan's grounding, the coordinator's instruction and `PLUGINS.md` all said
O-Chorus was at **1.5.0** and should go to 1.5.1. `CMakeLists.txt` read
**1.6.1** — the 260903-ukp *Infobulles* pass had bumped it twice since.

**C-8 held, and it is worth restating as a rule rather than a per-plugin note:
bump from the CMakeLists value, never from the registry row, never from the
plan, never from a handoff message.** O-Chorus shipped `1.6.1 → 1.6.2`, and the
`PLUGINS.md` row was stale in both columns.

This also changed the SIZE of the bump. 1.5.0→1.5.1 would have been the first
patch after the language landed; 1.6.1→1.6.2 is a patch on a plugin that had
already shipped Chinese for a day — which is what makes "the en and fr bodies
were false in the field" (finding 9) a *shipped* defect rather than a
caught-before-release one.

## 15. `--verbose` is not optional, and the numbers say why for the third stage running

`--ingest` truncates to `MAX_SHOWN` = 12 and prints `… 160 more (--verbose)`.
**Twelve is 7% of this batch.**

**Not one of the seven rows that had to be re-authored was in the top twelve.**
Their ranks by lexical score: 1, 14, 39, 50, 56, 79, 84. Four scored **0.50 or
above**. The `CRUSH_ENABLE` body — the one whose back-translation is not a
sentence — scored **0.73** and ranked **79th of 172**.

Task 1 recorded the same thing (both of its findings ranked outside the twelve).
That is two consecutive stages, four consecutive plugins, and it is now a
measured property of this tool rather than an anecdote: **the score is a sort
key and the tail is where the defects live.**

Round 2 makes the same point from the other direction: the seven corrected rows
came back scoring 0.61–0.91 and every one of them was RIGHT. **A high score is
not a pass and a low score is not a fail. Only reading is.**

## 16. A per-batch salted re-emit is the correct instrument for a correction loop

`--emit` cannot be scoped to a subset (finding 1), so a 7-row correction loop
needs a full 172-row emit trimmed by hand. That works cleanly and the tool
supports it by contract:

- re-emit the whole table to a NEW `--out`, which mints a fresh salt and a fresh
  manifest — no id is shared with round 1, so the reverse agent cannot correlate
  the correction batch against anything it saw before;
- trim the emitted file to the changed rows by matching on the zh column;
- `--ingest` the partial return with the FULL manifest. `ingest()` joins whatever
  ids the returned file carries and reports `joined: N`; a partial return is not
  a refusal (`scripts/i18n-zh-backtranslate.js:399-432`).

**Only the CHANGED rows need re-reading, not the whole entry.** The unchanged
rows of a touched entry were read and passed in round 1. `reviewed` is
entry-scoped, so the entry sits at `'mt'` until its changed row clears — which
is why O-Bitrot shows **13 rows at `'mt'` for 7 changed strings**.

## 17. zsh does not word-split, and a gate loop written for bash reports 127

`for c in "name:cmd with args"; do ...; $cmd; done` runs the whole command as one
word under zsh and every gate reported `exit=127` — "command not found" — which
reads exactly like a broken gate. This project's shell is zsh
(`pattern_zsh_no_word_split_backup_loop_strays`). Run the gates individually, or
use `eval`. A wave executor who batches gate invocations in a loop will get a
clean sweep of 127s and no indication why.

## 18. The discriminator between "re-author" and "accept" is COLLISION, not drift distance

Round 1 and round 2 both produced a triple where a Chinese string carrying the
morpheme 抖 came back as "jitter" against an English source that said something
else. One was re-authored and one was accepted, and the reason is worth stating
because a wave will face the same call:

| | round 1 | round 2 |
|---|---|---|
| zh | `抖动` (Dither) | `快抖` (TAPE_WOW body, flutter) |
| en' | "Jitter" | "the faster jitter" |
| verdict | **RE-AUTHORED** | **ACCEPTED** |
| why | BYTE-IDENTICAL to nothing — but it was the glossary root that `Jitter` would also take, so the two controls' captions were one qualifier apart and the body read as nonsense | `快抖` collides with no other string on the page; `快抖`, `时基抖动` and `抖动噪声` are three distinct renderings, and `快抖` is the partner of the glossary root its own title establishes |

**The question is not "how far did en' drift". It is "can a reader on the page
confuse this string with another control on the same page".** A contextless
reverse pass cannot answer that — it has no page — so the answer has to come from
the executor, and it has to be written down. Both calls are recorded at their
entries in `i18n.js`.

## 19. A correction loop needs a FRESH reader, not just a fresh salt

Round 2 used a fresh per-batch salt (finding 16) AND a fresh reverse agent with
no round-1 exposure. Both matter and they guard different things:

- **Fresh salt** stops the agent correlating the correction batch against ids it
  has already seen, which would tell it which rows were rejected.
- **Fresh agent** stops it remembering what it returned last time. An agent that
  answered "Jitter" for `抖动` an hour ago and is now handed `抖动噪声` has a
  strong prior toward consistency with its own earlier answer — which is exactly
  the round-tripping-your-own-vocabulary failure the whole flag exists to
  prevent, arriving one step later in the process.

The orchestrator supplied both without being asked. **Stage 4 should require
both explicitly**: a correction round read by the same agent that produced the
round-1 en-prime is not an independent pass.

---

## Deviations from plan

### [Rule 1 — Bug] `lang-select` enumerated two languages on a three-option selector

- **Found during:** action item 1, O-Bitrot.
- **Issue:** the `en` body read *"English and French are available"* and the `fr`
  body *"L'anglais et le français sont disponibles"*. Adding the third
  `<option>` made both shipped copies false.
- **Fix:** enumeration **removed, not extended** — reasoning in finding 9,
  recorded at the entry in `i18n.js`.
- **Commit:** `88c36535`.

### [Rule 1 — Bug] O-Chorus `tip.language` had the identical defect, shipped since v1.5.0

- **Found during:** action item 9 (folded in per the Task-1 carryover).
- **Issue:** en *"English or Français."*, fr *"English ou Français."* — false
  since v1.5.0. The `zh-Hans` body enumerated all three and was true.
- **Fix:** removed from all three; the `zh-Hans` entry demoted to `'mt'` and
  added to this batch, because the string it was back-translated at no longer
  exists.
- **Commit:** `43246871`.

### [Scope] O-Chorus's batch is 44 rows, not the 4 the plan predicted

Two causes, and only one is mine. The `tip.language` demotion took the ship-bar
count from 4 rows to 6. `--emit` then emitted all **44** because it is
table-scoped, not ship-bar-scoped — finding 1. Not trimmed by hand: the 38
already-`'bt'` rows got a second independent reading for free, and all held.

### [Scope] O-Chorus shipped 1.6.1 → 1.6.2, not the 1.5.0 → 1.5.1 that was asked for

The plan, the coordinator's instruction and `PLUGINS.md` all named 1.5.0.
`CMakeLists.txt` read 1.6.1. C-8 is explicit that the CMakeLists is the source
of truth, so the bump came from there. Finding 14.

### [Rule 1 — Bug] Seven O-Bitrot rows had to be re-authored after the triple read

Not a deviation from the plan — it is the plan's own instruction ("fix the
Chinese, then re-emit and re-ingest those ids") firing for the first time in
this stage. Task 1 found zero rows needing a re-loop; O-Bitrot found seven, and
one of them (`CRUSH_ENABLE`'s body) back-translated into a sentence that is not
a sentence. Detail in findings 2, 15, 18 and 19. **The consequence was a second
checkpoint, which is what `'bt'` costs when the read actually finds something.**
Round 2 came back clean and all 172 rows are now at the bar.

### [Rule 3 — Blocking] The zh block formatting was normalised

The insertion pass left stray blank lines between continuation lines and
mis-indented continuations by four spaces in 68 blocks. Cosmetic — the file
parsed and every gate was green throughout — but it made exact-string edits
unreliable, which blocked applying the corrections. Normalised in `4a26b265`;
**no zh value changed by that pass**, verified by re-running the lint, the
geometry gate and the clamp gate after it.

### [Rule 3 — Blocking] Two verify commands are wrong as written

Neither is a code defect; both would make a correct tree look broken.

1. `wc -l < "$SCRATCH/o-chorus-zh.tsv"` → **expect 4**. Returns **44**. Finding 1.
2. `grep -c 'zh-Hans' plugins/O-Bitrot/Source/PluginProcessor.h` → **expect 2**.
   Returns **3**. Finding 11.

A third is *arguably* wrong: `grep -c 'viewSync' "$SCRATCH/bitrot-uilabels.txt"`
→ expect 0. It returns **1** — but that hit is in the **coverage** section
(`#viewSync>select.field>option:nth-child(7)` was never visible in any driven
state, reported rather than asserted), not in any FAIL context. The plan's own
comment says "expect 0 in any FAIL context", so the prose is right and the
command under it is not.

### [Scope] `scripts/i18n-zh-glossary.js` untouched

The plan permits a `BUDGETS` addition "only if a new measured cell was found".
None was — the two longest Chinese captions are four characters against a
measured six-character budget (64 px cell / 9.5 px caption), measuring 43.64 px
in a 64 px cell, and `check-ui-labels` assertion 4 passes on all three arms. The
three existing cells are O-Chorus's and are intact.

### [Process] The table was authored at `'mt'` throughout

Task 1's carryover item 1, applied. All 117 entries were inserted carrying
`reviewed: 'mt'` and never briefly at `'bt'`. `BELOW SHIP BAR` reads **117**
against the committed tree, which is the honest state.

---

## Gate results

All run against the shipped tree at `d702f6f4`, after both promotions and both builds.

| Gate | Result |
|---|---|
| `check-i18n --plugin O-Bitrot` | **exit 0**, ALL CHECKS PASS, `LANGUAGES … got ["en","fr","zh-Hans"]` |
| — assertion 6, the inline module | **PASS**, canon v2 — finding 6 |
| `check-i18n` (repo-wide) | **exit 0**, `ALL CHECKS PASS — 43 localized plugin(s)` |
| `check-i18n --plugin O-Chorus` | **exit 0** |
| `check-ui-labels --plugin O-Bitrot` | **exit 0**, **0 FAIL** — was **exit 2 / 2 FAIL** (ZH3-09) |
| — geometry PASSes | 4 × `[7][GEOMETRY DIFF][fr]`, 4 × `[7][GEOMETRY DIFF][zh-Hans]` |
| — zh vacuity control | `70/71 labels (99%) differ from English, need >= 25%`; attributes `8/8` |
| — fr vacuity control | `64/71 labels (90%) differ`; attributes `8/8` |
| `check-ui-labels --plugin O-Chorus` | **exit 0** |
| `ui_tooltip_clamp_check.js` | **exit 0**, ALL CHECKS PASSED; real zh pass, **14 clamps engaged**, 55/55 anchors |
| — two-language literal | `grep -cE "\[ *'en' *, *'fr' *\]\|'en,fr'"` → **0**, comments included |
| `ui_preset_menu_check.js` | **exit 0**; 0 language references (finding 13) |
| `i18n-zh-lint --plugin O-Bitrot` | **0 findings** across Z1 Z2 Z3 Z4 Z5 Z6 Z7 F1 R1 — and it was 0 findings on the WRONG dither table too (finding 2) |
| — ship bar | **`BELOW SHIP BAR … 0`** · straight copies zh === en: 2 (both `sameAsEn`) · termNote exemptions: **6** |
| `i18n-zh-lint --plugin O-Chorus` | **0 findings**; **`BELOW SHIP BAR … 0`** |
| `i18n-zh-lint --self-test` | `SELF-TEST: 9/9` |
| `i18n-fr-lint` | **exit 0**, `CLEAN`, 0 / 43 plugins with findings |
| `boot-all-uis --plugin O-Bitrot --strict-tips` | **exit 0**, **0 DEAD**, 0 late, 0 failed |
| `boot-all-uis --plugin O-Chorus --strict-tips` | **exit 0** |
| Han gate over `plugins/O-Bitrot/Source/**/*.{h,cpp}` | **NO OUTPUT** |
| — Han positive control | **FIRED** — printed `…/js/i18n.js` on the same one-liner |
| `PluginProcessor.h` codec | **2** code occurrences of `zh-Hans` (3 raw — finding 11) |
| Han-space-Han scan (Task 1 finding 12) | **0** occurrences across all 172 rows |
| `--ingest` round 1, O-Bitrot | **joined 172**, 0 REFUSED, 0 unjoinable; all 172 read with `--verbose`; **7 rows re-authored** |
| `--ingest` round 2, O-Bitrot | **joined 7**, 0 REFUSED, 0 unjoinable; all 7 read; every one now says what its English says |
| `--ingest`, O-Chorus | **joined 44**, 0 REFUSED, 0 unjoinable; all 44 read; **zero drifts** |
| `i18n-zh-backtranslate` (stage view) | O-Bitrot **172 / 172 bt / 0 mt** · O-Chorus **44 / 44 bt / 0 mt** · O-Octagon **246 / 246 bt** · **TOTAL 462 / 462 bt / 0 mt** |
| ship-bar grep, comment-stripped | `reviewed: 'bt'` **117** · `'mt'` **0** · `'native'` **0** |
| Han-space-Han scan | O-Bitrot **0** · O-Chorus **0** |
| `O-Chorus/tests/ui_tip_render_check.js` | **exit 0** |
| `build-and-install.sh O-Bitrot` | **exit 0**, 57 s; `O-Bitrot-dev.{vst3,component}` both report **1.16.0** |
| `build-and-install.sh O-Chorus` | **exit 0**, 49 s; `O-Chorus-dev.{vst3,component}` both report **1.6.2** |
| `PLUGINS.md` rows | O-Bitrot `1.16.0` · O-Chorus `1.6.2` · both `2026-09-04`; duplicate check **EMPTY** |
| two-language literal inventory | comment-stripped **13** files (was 14); raw `grep -rln` still **14** — finding 8 |

**DEFERRED:** `auval -v`, per CLAUDE.md and the plan's Task-3 verify item 10. A
cold `auval` after an install rescans the whole AU registry (~15 min) and Task 3
builds again. **O-Octagon (v1.12.0), O-Bitrot (v1.16.0) and O-Chorus (v1.6.2)
must all be in that one end-of-batch sweep.**

---

## Commits

Both path-scoped; the submodule guard was run before each and reported clean.
`.gsd/dispatch-isolation-sentinel.json` was modified before this task began and
was deliberately **never** staged.

| Commit | Scope | Content |
|---|---|---|
| `88c36535` | `plugins/O-Bitrot` | the table (117 entries at `'mt'`), the endonym, the exemption reasons, the codec, the CJK tail on `--serif`, nine geometry pins including the `#viewSync` baseline closure, and the clamp gate's language derivation — 6 files, +843 / −109 |
| `43246871` | `plugins/O-Chorus` | `tip.language`'s enumeration removed from all three bodies; the `zh-Hans` entry demoted to `'mt'` — 1 file, +25 / −10 |
| `d788b769` | `plugins/O-Chorus` `PLUGINS.md` | promotion of all 44 rows to `'bt'` after the triple read, version `1.6.2`, CHANGELOG naming both provenance strings, registry row — 4 files, +65 / −7 |
| `4a26b265` | `plugins/O-Bitrot` | the seven re-authored rows, 159 rows promoted to `'bt'`, 13 held at `'mt'`, and the zh block formatting normalised — 1 file, +190 / −343 |
| `d702f6f4` | `plugins/O-Bitrot` `PLUGINS.md` | the last 7 entries promoted after the round-2 read, the two-round review recorded in the header, version `1.16.0`, CHANGELOG with every measured table, registry row — 4 files, +349 / −9 |

The split is the shape Task 1 established and any Stage-4 wave should copy: the
work lands first at `'mt'`, the reverse pass runs against a **committed** tree,
and the promotion plus the ship metadata land together in a later commit whose
message names both provenance strings. A single commit would make the bar
unauditable after the fact.

**Five commits, and the shape is the one any Stage-4 wave should copy:** the
work lands first at `'mt'`, the reverse pass runs against a **committed** tree,
a correction round gets its own commit that says plainly what the read found,
and the promotion plus the ship metadata land together in a final commit whose
message names every provenance string. A single commit would have made the bar
unauditable after the fact — and this plugin needed four before it earned it.

---

## Self-Check: PASSED

Files verified present on disk: `plugins/O-Bitrot/Source/ui/public/js/i18n.js`,
`plugins/O-Bitrot/Source/ui/public/index.html`,
`plugins/O-Bitrot/tests/ui_tooltip_clamp_check.js`,
`plugins/O-Bitrot/CHANGELOG.md`, `plugins/O-Bitrot/CMakeLists.txt`,
`plugins/O-Chorus/Source/ui/public/js/i18n.js`, `plugins/O-Chorus/CHANGELOG.md`,
`plugins/O-Chorus/CMakeLists.txt`, `PLUGINS.md`, this summary. Installed bundles
verified via `CFBundleShortVersionString`: `O-Bitrot-dev.vst3` = **1.16.0**,
`O-Chorus-dev.vst3` = **1.6.2**. Commits verified in `git log`: `88c36535`,
`43246871`, `d788b769`, `4a26b265`, `d702f6f4`. Working tree carries no
uncommitted O-Bitrot or O-Chorus change; the only modified path is
`.gsd/dispatch-isolation-sentinel.json`, which predates this task and was never
staged.
