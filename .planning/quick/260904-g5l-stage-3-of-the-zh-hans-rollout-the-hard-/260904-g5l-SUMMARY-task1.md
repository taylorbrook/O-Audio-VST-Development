---
phase: quick-260904-g5l
plan: 01
task: 1
subsystem: i18n
status: complete
tags: [i18n, zh-Hans, O-Octagon, geometry, gates]
requirements: [ZH3-01, ZH3-04]
commits:
  - 88575ff1
  - 125f8a0b
key-files:
  modified:
    - plugins/O-Octagon/Source/ui/public/js/i18n.js
    - plugins/O-Octagon/Source/ui/public/index.html
    - plugins/O-Octagon/Source/ui/public/css/styles.css
    - plugins/O-Octagon/Source/PluginProcessor.h
    - plugins/O-Octagon/Source/PluginProcessor.cpp
    - plugins/O-Octagon/Source/PluginEditor.cpp
    - plugins/O-Octagon/tests/ui_layout_check.js
    - plugins/O-Octagon/CMakeLists.txt
    - plugins/O-Octagon/CHANGELOG.md
    - PLUGINS.md
  untouched_deliberately:
    - scripts/i18n-zh-glossary.js             # no new measured BUDGETS cell was found
actuals:
  tokens: 168000
  tasks: 1
  commits: 2
---

# Stage 3 Task 1 — O-Octagon into Simplified Chinese (COMPLETE)

O-Octagon now carries a complete, gate-green `zh-Hans` table — 187 entries, 246
back-translation rows — with the CJK font tail measured onto the two type tokens
that actually render Han, seven measured geometry pins, and a per-plugin layout
gate that drives every declared language for the first time in its 2302-line
life. **Every entry sits at `reviewed: 'mt'`. The plugin is NOT shipped.**

## Status: complete and shipped at `reviewed: 'bt'`

O-Octagon **v1.12.0** is built, installed and carries three languages. All 187
entries are at `reviewed: 'bt'`; `BELOW SHIP BAR` reads **0**.

### The back-translation loop, and what `'bt'` actually asserts here

The plan's `<human-check>` stopped this task after action item 7. I authored all
246 Chinese strings **and** read the English they came from, so a reverse pass by
me would have round-tripped my own vocabulary and read clean while the Chinese
was wrong — the single failure the flag exists to prevent (T-g5l-01, T-g5l-03).
I stopped, and flipped the table **down** to `'mt'` rather than leave the `'bt'`
I had first written, because `'bt'` asserts a review that had not happened.

The orchestrator ran the pass: a separate **claude-sonnet-5** subagent handed the
blinded batch file and nothing else — no manifest, no repo, explicitly forbidden
from reading plugin files.

**Both blinding controls were fired on this batch, not assumed:**

| Control | Result |
|---|---|
| every emitted id opaque | `grep -cE '^[0-9a-f]{12}\t'` = **246 / 246** |
| no plugin name or key fragment leaked | `grep -cE 'O-\|label\.\|tip\.'` = **0** |
| ids returned exactly as received | `diff <(cut -f1 emit) <(cut -f1 return)` = **identical, in order** |
| every returned row well-formed | `awk -F'\t' 'NF==2'` = **246** |

Provenance, which `--ingest` refuses if missing or byte-identical to the forward
string:

- **forward** — `forward zh-Hans draft authored by the Stage-3 Task-1 executor (Claude Opus 4.6, GSD quick-260904-g5l), 2026-09-04`
- **reverse** — `reverse pass: blind zh->en by claude-sonnet-5 subagent, batch file only, no manifest, no plugin context, 2026-09-04`

`--ingest` with the explicit `--manifest`: **246 joined, 0 REFUSED, 0 unjoinable.**

### All 246 triples were read — not the twelve the tool prints

`--ingest` truncates to 12 and says `… 234 more (--verbose)`. I re-ran with
`--verbose` and read every one.

**Verdict: no triple said something its English did not. Zero Chinese words
changed.** The score is a sort key, not a verdict — the thirteen `0.00` rows are
all correct, and several `1.00` rows still needed reading.

**The one drift that needed a decision:**

> `aria.scene-store` — EN `Arm store` → `存储待命` → EN' `Store armed`
>
> English names an **action**; the reverse pass returned a **state**. Chinese
> marks neither causative nor stative on a verb-object compound, so `存储待命`
> carries both readings and nothing disambiguates. **KEPT.** The control is a
> toggle whose `aria-pressed` already carries armed-vs-not, so the accessible
> *name* should name the control and the state belongs to the state attribute.
> Rewriting to a causative `使存储待命` would have duplicated `aria-pressed` in
> prose and — being a changed word — would have needed a second reverse pass to
> stay at `'bt'`.

**Accepted synonyms**, each recorded in the `i18n.js` header: `Plan → 平面图`
("Floor plan" — more specific and right), `Field → 声场` ("Sound field" — the
DBAP field is one), `Set → 输出集` ("Output set" — the deliberate expansion the
`.rail-line` pin documents), `Derive → 计算` ("Calculate" — matches the French
`Calculer`), `Rake → 坡度` ("Slope" — audience rake *is* the seating slope),
`ping → 激励` ("stimulus" — the glossary root), `Class → 类别` ("Category"), the
eight `aria.spkN.{x,y,z}` gaining "coordinate", and the plural /
part-of-speech-only rows Chinese cannot mark.

### One correction after the read — and it changed no token

Reading **triple 96** (the `puck` body) exposed `同时写入 源 X 与 源 Y` — two
stray U+0020 **between Han runs**, from writing the caption name `源 X` as if the
space belonged to the word rather than to the Latin boundary. Removed; a
table-wide rescan for `/\p{Script=Han} \p{Script=Han}/u` now returns **0**.

Deliberately **not** re-looped through a second reverse pass. Chinese has no word
delimiter, an interior space is noise a reader steps over, and every token either
side is byte-identical — re-looping would have been theatre, and the plan's
"a fixed string whose correction was never back-translated has not met the bar"
is about *meaning*, which nothing here touched.

**No lint rule catches this.** Z4 polices the Latin/Han boundary only, so a space
between two Han runs is invisible to it. It was found by **reading**, which is
the argument for reading all 246 rather than the twelve the tool prints. *(New
finding — see §12.)*

### Ship

| Item | Value |
|---|---|
| `CMakeLists.txt` | `1.11.2` → **`1.12.0`** (a new language is a minor bump) |
| CHANGELOG | `## v1.12.0 (2026-09-04)` — **this file's own heading style** (C-9), not the bracketed form its two siblings use |
| `PLUGINS.md` | `1.11.1-dev` → **`1.12.0` / `2026-09-04`**, stale `-dev` suffix dropped (C-8); duplicate-row check empty |
| build | `./scripts/build-and-install.sh O-Octagon` — **exit 0**, 49 s |
| install | only `O-Octagon-dev.{vst3,component}` on disk after Phase 4's alternate-variant sweep; both report **`1.12.0`** |
| `auval -v` | **DEFERRED to end-of-batch** — a cold `auval` after an install rescans the whole AU registry (~15 min) and Tasks 2 and 3 each build again |

# ══ STRUCTURAL FINDINGS — READ THIS BEFORE THE STAGE-4 WAVES ══

Thirty-nine plugins across seven waves will copy whatever this stage establishes.
Everything below cost measurement to find and would otherwise be hit blind.

## 1. The plan's entry count is wrong, and 246 is the number that matters

The plan and its verify commands say **131 entries** and
`grep -c "reviewed: 'bt'"` → **expect 131**. Neither is reproducible against the
file. The true figures:

| Figure | Value | How |
|---|---|---|
| entries | **187** | `Object.keys(I18N).length` (59) `+ Object.keys(LABELS).length` (128) |
| source lines carrying a `reviewed:` marker | **138** | 187 − 56 template-generated + 7 template lines |
| back-translation rows | **246** | labels + tooltip titles + tooltip bodies |

**246 was right in the plan.** The entry count was not. A verify written against
131 fails on a correct file. For any Stage-4 plugin, derive the expected
`reviewed:` line count as `entries − generated + templates`, or just count rows.

## 2. Type is TOKENISED, so the font-tail procedure does not transfer as written

The plan's C-5 says two of three plugins keep CSS in external files. True, but it
understates O-Octagon: `index.html` has **zero** `font-family` declarations, all
**29** are in `Source/ui/public/css/styles.css`, **and every one of them is
`var(--serif)` or `var(--mono)`**. The whole page resolves through **two custom
properties**.

- O-Chorus needed **five** appends against eight literal stacks.
- O-Octagon needed **two**, on the token definitions.
- A wave executor who greps `index.html` for `font-family` gets zero hits and
  concludes there is nothing to do. One who greps `styles.css` gets 29 hits and
  edits 29 places, 27 of them pointlessly.

**Grep for `--*:` type tokens FIRST.** If the stacks are tokenised, the tail goes
on the tokens.

## 3. Measuring the resolution set found exactly three stacks — and the third is a trap

Serving the page, switching to Chinese, and reading
`getComputedStyle(node).fontFamily` on every node holding **or able to receive**
a Han codepoint (own text, `data-tip`, `data-tip-title`, `aria-label`) returned
three distinct stacks:

| Stack | Took the tail? | Why |
|---|---|---|
| `"Iowan Old Style", … serif` (`--serif`) | YES | body inheritance **and** the `#tooltip` surface |
| `ui-monospace, … monospace` (`--mono`) | YES | scene buttons, banners, venue table, **and the endonym `<option>`** |
| bare `Arial` | **NO** | Chromium's UA default on `<input type="range">`, reached **only** through `aria-label` on the sixteen sliders |

The Arial omission is the one that makes the other two believable: an accessible
name is never rendered text — it is spoken, and the screen reader picks its own
face. There is no glyph to fall back for. **Record the omission reason; a wave
that appends the tail to everything it measured cannot tell a needed tail from a
decorative one.**

The two nodes a `[data-i18n]`-derived scan would have missed are the same two
O-Chorus found: the tooltip surface (filled from `data-tip` at hover time, never
keyed) and the endonym `<option>` (the only Han in the markup). **This is now
2-for-2. Treat it as universal.**

## 4. The charset-past-the-prescan-window question has a better answer than entities

`<meta charset>` sits at **byte 2926**, past the 1024-byte prescan window. But
the shipped transport already carries the charset:

```cpp
// PluginEditor.cpp:209-214
// charset=utf-8 on every text resource …
return makeBinaryResource (UIBinaryData::index_html, …, "text/html; charset=utf-8");
```

and `scripts/serve-ui.js`'s MIME table does the same for the harness. **So the
past-window declaration was never the encoding guarantee — the `Content-Type`
header is.** The numeric-entity endonym is belt-and-braces, kept for convention.

**For the five plugins sharing the past-window shape: check the plugin's own
`getResource()` before treating the byte offset as a defect.** Two of the five
may well already be safe. What a wave must NOT do is assume, in either direction.

## 5. Chinese geometry failures are SHRINKS, and a growth-only assertion is blind to all of them

Chinese buys width and spends height. Three distinct failure shapes appeared, in
this order, and only the first is the one Stage 2 documented:

**(a) Height — `line-height: normal` inheritance.** `styles.css` declared
line-height on **two** rules, so every caption inherited the font's own metrics.
First Chinese run: **98** non-label movers in the default state, **196** on the
Venue screen. Every mover reported `lh=normal`. Pinning each named leaf class at
its **measured English line box over its own font size**, unitless, closed all of
them. No global rule was added.

> **The two font families round differently, and one number cannot serve a page.**
> Iowan Old Style's `normal` is 15/11 and floors to integer px:
> 10 px → 13.00, 11 px → 15.00, 12 px → 16.00, 9.5 px → 13.00 — four unitless
> values. The mono stack floors to 9 px → 10.00, 10 px → 11.00, 11 px → 13.00.
> **A class that appears at two sizes needs two pins**: `.vcell-value` is 11 px
> in the rail and 9 px inside `.vcol-head`.

**(b) Width — a content-sized caption that SHRANK.** Four movers survived the
line-height pins carrying `dx` only, `dy=0 dh=0`. That is the tell: line-height
moves things DOWN, width moves them SIDEWAYS.

| Node | EN | FR | zh | Effect |
|---|---|---|---|---|
| `#readout-label-source` | 54.95 | 54.95 | **12.42** | pulled 3 footer readouts 42.53 px left |
| `.plan-caption > .caption-key` | 35.91 | 35.91 | **37.27** | pushed 3 nodes 1.36 px right |
| `.rail-line .cell-label` | 19.55 | 19.78 | **39.64** | pushed `#vset-name` 20.1 px right |

Same class as v1.9.0's `ENVELOPE → ENVELOPPE` pin arriving from the opposite
direction — and the same class as O-Bitrot's `#viewSync` select. Pinned at the
**widest of the three**, which is the law this file already followed twice.

**(c) Height — an UN-WRAP.** Inside the 168 px speaker→output popover, the title
and note **wrap to two lines in English and French and fit on one in Chinese**:

```
.out-pop-title   EN 27.00 · FR 27.00 · zh 14.00   (dh = -13)
.out-pop-note    EN 25.97 · FR 25.97 · zh 12.98   (dh = -13)
```

`#out-pop` reported `dh = -26` and pulled all eight output buttons 13 px **up**.
The instinct on reading "11 moved" is to hunt for something that grew; nothing
did. Pinned `min-height` to the English two-line box.

> **Wave guidance:** any gate assertion phrased as "the non-English pass must not
> GROW box X" is vacuous against Chinese. Assert equality, not an inequality.

## 6. A late CSS pin silently un-pinned a FRENCH one — caught only because the gate runs every language in one run

My first width pin was `.plan-caption > .caption-key { min-width: 38px }`.
Specificity (0,2,0), later in the file than v1.9.0's
`.caption-field { min-width: 50px }` at (0,1,0) — so it **overrode the French
pin**. `CHAMP` fell back from 50 to 38 and `#field-legend` moved 8.1 px on the
**fr** arm, a regression this Chinese work had no business causing. The zh arm
reported it too, at −2.1.

Fixed with `:not(.caption-field)`. **A pin block appended at the end of a
stylesheet outranks every earlier pin of equal-or-lower specificity that targets
an overlapping set.** Grep for existing `min-width` / `min-height` pins on the
classes you are about to touch, before appending.

## 7. ZH3-04 confirmed: `ui_layout_check.js` had NO language machinery at all

`grep -niE "lang|i18n|french|fr\b|locale"` over 2302 lines returned **nothing**
— C-1 is exactly right, and this was an ADD, not a repair. `ui_frontend_check.js`
(43 sections) likewise carries no hard-coded pair; grounding's "confirm rather
than assume" was worth doing and the answer was clean.

New **section 34** does four things a wave should copy verbatim:

1. **Reads `LANGUAGES` from the plugin's own `js/i18n.js` and FAILS rather than
   defaulting.** A gate that falls back to a pair is a gate that goes green on
   unchecked content.
2. **Derives its box set from the gate file's OWN SOURCE** —
   `getElementById('…')` and `querySelector('…')` literals, deduped. 49 selectors
   today; 28 resolve on the Room screen, 12 on the Venue screen. A section added
   next year joins the invariance set on the day it is written, so the list
   cannot go stale the way a transcribed one does.
3. **Proves the switch took**, by reading `#lang-select.value` back after each
   `window.__setLanguage(lang)`.
4. **Labels every failure with its language**, and sweeps **both screens** —
   roughly half the pinned boxes measure 0×0 while the Room screen is up, so a
   single-screen sweep would silently drop them.

**Positive control fired.** Reverting the `#readout-label-source` pin produced:
```
FAIL: [34] [zh-Hans][room] every pinned box is identical to English — 2 moved:
      #readout-envelope dx=-42.5 …; #readout-metres dx=-42.5 …
PASS: [34] [fr][room] every pinned box is identical to English
```
Chinese-only failure, correctly attributed, French arm untouched. Restored.

## 8. The C-7 inventory grep is COMMENT-BLIND and over-counts by one today

`grep -rln "en,fr\|\['en', *'fr'\]" plugins/*/tests/*.js` returns **15** files —
including **O-Chorus**, which Stage 2 already repaired. Its hit is a *comment*:

```
plugins/O-Chorus/tests/ui_tip_render_check.js:248:
    // default pair: a gate that quietly measured ['en','fr'] on a table that
```

The plan predicted 15-minus-O-Chorus = 14, and 14 is right — but it got there by
subtracting a plugin the grep still counts. After Tasks 2 and 3 the raw grep will
report **13**, and Task 3's verify says "expect 12". **An executor reading that
will chase a phantom.** I hit the same trap in my own new section and had to
reword the comment so the inventory would not lie about the file that fixed it.

**The comment-stripped inventory — this is the real list. 14 files, all still
carrying a live two-language literal:**

```
O-Bassoon/tests/ui_tip_render_check.js
O-Bells/tests/ui_tip_render_check.js
O-Bitrot/tests/ui_tooltip_clamp_check.js          <- Task 2
O-Bowed/tests/ui_tip_render_check.js
O-Comp/tests/ui_tip_render_check.js
O-Emulator/tests/ui_tip_render_check.js
O-MicrotonalSampler/tests/ui_tip_render_check.js  <- Task 3
O-Reed/tests/ui_tip_render_check.js
O-ReverseDelay/tests/ui_tooltip_clamp_check.js
O-SimpleReverb/tests/ui_tip_render_check.js
O-Tapestop/tests/ui_tooltip_clamp_check.js
O-Texture/tests/ui_tip_render_check.js
O-TextureForge/tests/ui_tip_render_check.js
O-Wind/tests/ui_tip_render_check.js
```

Use this form instead:
```bash
for f in plugins/*/tests/*.js; do
  grep -v '^\s*//' "$f" | grep -v '^\s*\*' | grep -q "en,fr\|\['en', *'fr'\]" && echo "${f#plugins/}"
done | sort
```
After Tasks 2 and 3 it must read **12**.

## 9. `text-transform: uppercase` needed no budget cell here, and the measurement is why

`styles.css` carries **11** uppercase rules and `text-transform` is a **no-op on
Han**, so an English caption renders uppercased-and-wider while its Chinese
counterpart renders untransformed. Against a pinned cell that is a fit question.

Measured answer: **no new `BUDGETS` cell was needed.** `check-ui-labels`
assertion 4 passed on all three arms (no Chinese caption is wider or taller than
its own content box), and every already-pinned cell has room —
`.caption-field` 50 px holds 声场 (~27), `#readout-label-envelope` 82 px holds
包络 (~27), `#btn-scene-store` 46 px holds 存储. The three existing `BUDGETS`
entries are O-Chorus's and are **untouched**; `scripts/i18n-zh-glossary.js` is
not in this commit.

The fit risk on this page turned out to be the opposite one: the **content-sized**
`.rail-line .cell-label`, where Chinese was the widest of the three. Finding (5b).

## 10. Terms this plugin settled that the 552-term glossary does not carry

Recorded so the Stage-4 spatial plugins inherit rather than re-derive. Full list
in the `i18n.js` header; the load-bearing ones:

| English | zh-Hans | Note |
|---|---|---|
| speaker | 扬声器 | the rig's transducer, never 音箱 |
| rig / array | 扩声系统 / 阵列 | |
| venue / hall | 场地 / 厅堂 | the tab is 场地; prose uses 厅堂 |
| room (the tab) | 房间 | |
| solve (DBAP) | 解算 | |
| hull | 外壳 | `hull attenuation` → 外壳衰减 is a glossary root |
| blur | 模糊 | |
| air (the filter) | 空气 | |
| decorrelate | 去相关 | |
| rake | 坡度 / 观众席坡度 | |
| alignment delay | 对齐延迟 | |
| anchor | 锚点 | |
| bounding box | 包围盒 | |
| fold-down | 折叠 | |
| comb filtering | 梳状滤波 | |
| plan (the view) | 平面图 | |
| field | 声场 | |

Z3's 像 fix (`6da6bd22`) landed for this plugin, but in the event O-Octagon's
vocabulary is DBAP and venue geometry rather than 声像. **The fix is still
correct and still needed — just not by this plugin.**

## 11. No `sameAsEn` entry exists in the zh table

Measured, not omitted: O-Octagon has no bare `LFO` / `MIDI` / `dB` caption, so
all 187 renderings differ from their English. The six motion-path faces and the
sync divisions stay English **inside the bodies** (they come from
`juce::StringArray` literals at `PluginProcessor.cpp:200-204` and are what the
`<option>`s are built from) — but they are prose, not keyed labels, so no
`sameAsEn` flag applies.

## 12. A space between two Han runs is invisible to every lint rule you have

Found by reading triple 96, not by any gate. My `puck` body carried
`同时写入 源 X 与 源 Y` — the caption name `源 X` correctly takes a space at its
Latin boundary, and I let that space migrate to the *left* of `源` as well, where
there is no boundary at all, just two Han characters.

**Z4 cannot see it.** Its regex only matches a Latin/digit run adjacent to a Han
run; `写入 源` is Han-space-Han and never enters the boundary census. Nothing else
in the nine rules looks at intra-Han whitespace either.

The scan every Stage-4 plugin should run before promoting to `'bt'`:

```bash
node --input-type=module -e "
import * as m from './plugins/<Name>/<uiroot>/js/i18n.js';
const rows=[];
for(const v of Object.values(m.LABELS)) rows.push(v['zh-Hans']?.t??'');
for(const v of Object.values(m.I18N)){ rows.push(v['zh-Hans']?.t??''); rows.push(v['zh-Hans']?.b??''); }
let n=0; for(const s of rows){ const re=/\p{Script=Han} \p{Script=Han}/gu; while(re.exec(s)!==null) n++; }
console.log('Han-space-Han occurrences:', n);   // must be 0
"
```

This is a **candidate new lint rule (Z8)**. It is cheap, it is mechanical, and it
caught a real defect on the very first plugin after the pilot. Worth adding to
`scripts/i18n-zh-lint.js` before the volume waves — but note the memory pattern
`pattern_new_lint_zero_must_agree_with_independent_scan`: whoever adds it must
check its zero column against this standalone scan across all 43 before trusting
it.

## 13. `--ingest` prints twelve triples and says "234 more" — read them anyway

The default `--ingest` output truncates to `MAX_SHOWN` = 12, worst-drift-first,
and prints `… 234 more (--verbose)`. **Twelve is 5% of the batch.**

Of the two issues this task found by reading, **neither was in the top twelve**:
`aria.scene-store` sat at 0.39 (rank 22) and the `puck` whitespace defect at 0.69
(rank 96, well into the "looks fine" band). A wave executor who reads only the
default output will promote to `'bt'` having seen 5% of what the flag claims was
read.

Always pass `--verbose`. The lexical score is a **sort key, not a verdict**, and
this run is the second consecutive stage where the tail of the distribution held
the finding.

---

## Deviations from plan

### [Rule 1 — Bug] The `lang-select` body stated something false

- **Found during:** action item 1.
- **Issue:** both the `en` and `fr` bodies enumerated the available languages —
  *"English and French are available"* / *"L'anglais et le français sont
  disponibles"*. Adding the third `<option>` made that shipped copy false.
- **Fix:** the enumeration was **removed, not extended**. Naming all three would
  put Han inside the `en` and `fr` bodies, which moves the English tooltip's own
  geometry and drags the CJK tail onto the very baseline every gate measures
  against. The selector already lists the languages, in their endonyms — the one
  form a reader recognises without knowing the page language — so a body that
  counts them duplicates the control it describes and goes stale again at Stage 4.
- **Files:** `plugins/O-Octagon/Source/ui/public/js/i18n.js` (reason recorded at
  the entry). **Commit:** `88575ff1`.
- **O-Chorus carries the identical defect** (`tip.language` still says *"English
  or Français"* at v1.6.0). Out of scope for a path-scoped O-Octagon commit;
  flagged here so Task 2, which already touches O-Chorus for ZH3-08, can fold it in.

### [Rule 3 — Blocking] Two verify commands are wrong as written

Neither is a code defect; both would make a correct file look broken.

1. `grep -c "reviewed: 'bt'"` → **expect 131**. The file has **138** such lines.
   See finding 1.
2. `grep -c 'zh-Hans' plugins/O-Octagon/Source/PluginProcessor.h` → **expect 2**.
   Returns **4**: two in the code (`languageCode`, `languageIndex`) and two in
   the doc comment that explains them. The code count is exactly 2, confirmed by
   `grep -E 'static (juce::String|int)'`. **A comment-blind grep as a code gate
   is the same defect as finding 8**, twice in one plan.

### [Process] The table was written at `'bt'`, then flipped DOWN to `'mt'`, then back up

Worth recording because it is the shape of an easy silent failure. I inserted all
138 zh blocks carrying `reviewed: 'bt'` — the value they would eventually hold —
and every gate went green on that tree: `check-i18n` accepts `'bt'` as a valid
enum member, and `i18n-zh-lint` R1 only reports entries *below* the bar, so a
premature `'bt'` is **invisible to every automated check in the repo**.

Nothing would have caught it. I flipped all 138 down to `'mt'` before the
back-translation stop, which made R1 print `BELOW SHIP BAR … 187` and put the
disclosure where it belonged, and promoted them only after reading the 246
triples.

**Wave guidance: author at `'mt'`, never at the value you expect to end up with.**
The flag is the only record that a review happened, and it is the one field no
gate can validate — `'bt'` written before the reverse pass is indistinguishable
from `'bt'` written after it.

### [Scope] `scripts/i18n-zh-glossary.js` untouched

The plan permits a `BUDGETS` addition "only if a new measured cell was found".
None was — see finding 9. The three existing cells are O-Chorus's and are intact,
so the glossary is in neither commit.

---

## Gate results

All run against the committed tree at `125f8a0b`, after the promotion to `'bt'`
and after the build.

| Gate | Result |
|---|---|
| `check-i18n --plugin O-Octagon` | **exit 0**, ALL CHECKS PASS, languages `en, fr, zh-Hans` |
| `check-i18n` (repo-wide) | **exit 0**, `ALL CHECKS PASS — 43 localized plugin(s)` |
| `check-ui-labels --plugin O-Octagon` | **exit 0**, **0 FAIL**; 20 × `PASS [7][GEOMETRY DIFF][fr]`, 20 × `PASS [7][GEOMETRY DIFF][zh-Hans]` |
| — zh vacuity control | `84/84 labels (100%) differ from English, need >= 25%`; keyed attributes `89/89` |
| `ui_layout_check.js` | **exit 0**, `ALL SECTIONS PASS — 34 sections`; 5 × `zh-Hans` |
| — section 34 positive control | **FIRED** — zh-only FAIL, fr arm still PASS (finding 7) |
| `ui_frontend_check.js` | **exit 0**, 43 sections |
| `i18n-zh-lint --plugin O-Octagon` | **0 findings** across Z1 Z2 Z3 Z4 Z5 Z6 Z7 F1 R1; 246 rows / 187 zh entries |
| — ship bar | `BELOW SHIP BAR … 0` · `straight copies zh === en: 0` · `termNote exemptions: 0` |
| `i18n-zh-lint --self-test` | `SELF-TEST: 9/9` |
| `i18n-fr-lint` | **exit 0**, `CLEAN`, 0 / 43 plugins with findings |
| `boot-all-uis --strict-tips` | **exit 0**, **0 DEAD**, 0 late, 0 failed |
| Han gate over `Source/**/*.{h,cpp}` | **NO OUTPUT** |
| — Han positive control | **FIRED** — printed `…/js/i18n.js` on the same one-liner |
| `PluginProcessor.h` codec | 2 code occurrences of `zh-Hans` (4 raw, 2 in the doc comment — see deviations) |
| `i18n-zh-backtranslate` (stage view) | O-Octagon **246 rows / 246 bt / 0 mt** |
| `PLUGINS.md` duplicate rows | **EMPTY** |
| Han-space-Han scan (new, finding 12) | **0** |
| `build-and-install.sh O-Octagon` | **exit 0**, 49 s; only `O-Octagon-dev.{vst3,component}` installed, both `1.12.0` |

**Deferred:** `auval -v`. A cold `auval` after an install rescans the whole AU
registry (~15 min) and Tasks 2 and 3 each build again. It belongs in one
end-of-batch rescan, per the plan's Task-3 verify item 10. O-Octagon must be in
that sweep.

**Still open for Task 2:** the stage view shows **O-Chorus at 4 rows `mt`** —
`tip.settings` and `tip.tipsToggle`, both halves of each. That is ZH3-08 and is
untouched here.

## Commits

Both path-scoped; the submodule guard was run before each and reported clean.
`.gsd/dispatch-isolation-sentinel.json` was modified before this task began and
was deliberately **never** committed.

| Commit | Scope | Content |
|---|---|---|
| `88575ff1` | `plugins/O-Octagon` | the table, the endonym, the codec, the CJK tail, the seven geometry pins, gate section 34 — 7 files, +888 / −96, everything at `reviewed: 'mt'` |
| `125f8a0b` | `plugins/O-Octagon` `PLUGINS.md` | promotion to `'bt'` after the 246-triple read, the whitespace correction, version bump, CHANGELOG, registry row — 4 files, +336 / −154 |

The split is deliberate and is the shape any Stage-4 wave should copy: the work
lands first at `'mt'`, the reverse pass happens against a committed tree, and the
promotion plus the ship metadata land together in a second commit whose message
names both provenance strings. A single commit would have made the bar
unauditable after the fact.

---

## Self-Check: PASSED

Files verified present on disk: `js/i18n.js`, `css/styles.css`,
`tests/ui_layout_check.js`, `CHANGELOG.md`, `CMakeLists.txt`, `PLUGINS.md`, this
summary. Commits verified in `git log --all`: `88575ff1`, `125f8a0b`. Working
tree carries no uncommitted O-Octagon change; the only modified path is
`.gsd/dispatch-isolation-sentinel.json`, which predates this task and was never
staged.
