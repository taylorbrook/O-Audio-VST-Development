---
phase: quick-260904-g5l
plan: 01
subsystem: i18n
status: complete
tags: [i18n, zh-Hans, O-Octagon, O-Bitrot, O-Chorus, O-MicrotonalSampler, geometry, gates, stage-3]
requirements: [ZH3-01, ZH3-02, ZH3-03, ZH3-04, ZH3-05, ZH3-06, ZH3-07, ZH3-08, ZH3-09, ZH3-10]
commits:
  - 88575ff1   # T1 O-Octagon, table at 'mt'
  - 125f8a0b   # T1 O-Octagon v1.12.0, promotion + ship
  - 88c36535   # T2 O-Bitrot, table at 'mt' + #viewSync baseline closed
  - 43246871   # T2 O-Chorus tip.language enumeration removed
  - d788b769   # T2 O-Chorus v1.6.2, promotion + ship
  - 4a26b265   # T2 O-Bitrot, seven rows re-authored after the triple read
  - d702f6f4   # T2 O-Bitrot v1.16.0, promotion + ship
  - 567deaab   # T3 O-MicrotonalSampler, table at 'mt' + tip gate unblocked
  - 5c4b326f   # T3 O-MicrotonalSampler v1.27.0, promotion + ship
key-files:
  modified:
    - plugins/O-Octagon/Source/ui/public/js/i18n.js
    - plugins/O-Octagon/Source/ui/public/css/styles.css
    - plugins/O-Octagon/tests/ui_layout_check.js
    - plugins/O-Bitrot/Source/ui/public/js/i18n.js
    - plugins/O-Bitrot/Source/ui/public/index.html
    - plugins/O-Bitrot/tests/ui_tooltip_clamp_check.js
    - plugins/O-Chorus/Source/ui/public/js/i18n.js
    - plugins/O-MicrotonalSampler/Resources/ui/js/i18n.js
    - plugins/O-MicrotonalSampler/Resources/ui/css/sampler-shell.css
    - plugins/O-MicrotonalSampler/Resources/ui/css/tuning-panel.css
    - plugins/O-MicrotonalSampler/tests/ui_tip_render_check.js
    - PLUGINS.md
  untouched_deliberately:
    - scripts/i18n-zh-glossary.js     # no new measured BUDGETS cell on any of the three; TERMS left to Stage 4 to promote deliberately
    - scripts/i18n-zh-lint.js         # the Z1/Z5 contradiction is REPORTED, not worked around
actuals:
  tokens: 610000
  tasks: 3
  commits: 9
---

# Stage 3 of the zh-Hans rollout — the hard cases (COMPLETE)

**Four plugins ship English, French and Simplified Chinese.** O-Octagon
**v1.12.0**, O-Bitrot **v1.16.0**, O-Chorus **v1.6.2** and O-MicrotonalSampler
**v1.27.0** are built, installed and auval-clean. The repo-wide ship-bar view
reads **758 rows / 758 `bt` / 0 `mt`**.

| | O-Octagon | O-Bitrot | O-Chorus | O-MicrotonalSampler |
|---|---|---|---|---|
| entries / rows | 187 / **246** | 117 / **172** | 28 / **44** | 275 / **296** |
| at `reviewed: 'bt'` | 246 / 246 | 172 / 172 | 44 / 44 | 296 / 296 |
| version | 1.11.2 → **1.12.0** | 1.15.2 → **1.16.0** | 1.6.1 → **1.6.2** | 1.26.1 → **1.27.0** |
| reverse rounds | 1 | **2** (172 + 7) | 1 | **2** (296 + 1) |
| rows re-authored | 0 | **7** | 0 | **1** |
| `auval -v` | SUCCEEDED | SUCCEEDED | SUCCEEDED | SUCCEEDED |

**Every requirement is met.** ZH3-01/02/03 (the three plugins), ZH3-04 (a
language arm built into a 2302-line gate that had none), ZH3-05 (the clamp
gate's zh arm), ZH3-06 (the pluralization claim verified rather than assumed),
ZH3-07 (the tip gate's hard-assert unblocked), ZH3-08 (O-Chorus's last four
rows), ZH3-09 (O-Bitrot's standing 2-FAIL French baseline **closed, not
inherited**), ZH3-10 (this document).

`reviewed: 'native'` stays **OPEN** on all four and is not a blocker.
**This project has no native Chinese reader.** A disclosed quality level.

## The end-of-batch auval sweep

Deferred through all three tasks and run **once**, cold, after the last install —
a cold `auval` rebuilds the whole AU registry, so four separate runs would have
cost four rescans. Triples read off the `auval -a` rows, never guessed. Note
O-MicrotonalSampler is `aumu` (an instrument), not `aufx`.

| Plugin | Version | Triple | Verdict |
|---|---|---|---|
| O-Octagon | 1.12.0 | `aufx OuOc OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Bitrot | 1.16.0 | `aufx OBrt OuDv` | **AU VALIDATION SUCCEEDED** |
| O-Chorus | 1.6.2 | `aufx OuCh OuDv` | **AU VALIDATION SUCCEEDED** |
| O-MicrotonalSampler | 1.27.0 | `aumu OMtS OuDv` | **AU VALIDATION SUCCEEDED** |

Registry rescan 17:41:43 → 17:43:04. Only `-dev` bundles on disk for all four;
the alternate-variant sweep found no orphan to shadow an AU slot.

---

# ══ STRUCTURAL FINDINGS — READ THIS BEFORE THE STAGE-4 WAVES ══

**Thirty-nine plugins across seven waves will copy whatever this stage
establishes.** Everything below cost measurement to find. This section merges
all three tasks; the task-local summaries hold the per-plugin detail.

## 1. The twelve remaining two-language gate files (ZH3-10) — and the count that lies

**Twelve per-plugin gate files still hard-code a two-language pair, and every one
of them belongs to a Stage-4 wave plugin.** Each will either hard-fail (the
`ui_tip_render_check` shape) or go vacuously green (the `ui_tooltip_clamp_check`
shape) on the day its plugin gets a zh entry.

```
O-Bassoon/tests/ui_tip_render_check.js
O-Bells/tests/ui_tip_render_check.js
O-Bowed/tests/ui_tip_render_check.js
O-Comp/tests/ui_tip_render_check.js
O-Emulator/tests/ui_tip_render_check.js
O-Reed/tests/ui_tip_render_check.js
O-ReverseDelay/tests/ui_tooltip_clamp_check.js
O-SimpleReverb/tests/ui_tip_render_check.js
O-Tapestop/tests/ui_tooltip_clamp_check.js
O-Texture/tests/ui_tip_render_check.js
O-TextureForge/tests/ui_tip_render_check.js
O-Wind/tests/ui_tip_render_check.js
```

**The obvious grep over-counts and will send you chasing a phantom.**
`grep -rln "en,fr\|\['en', *'fr'\]" plugins/*/tests/*.js` returns **13** — the
thirteenth is O-Chorus, whose only remaining hit is a *comment* explaining the
literal Stage 2 removed. Use the comment-stripped form, which also strips block
comments and JSDoc continuation lines:

```bash
for f in plugins/*/tests/*.js; do
  perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' "$f" \
    | grep -q "en,fr\|\['en', *'fr'\]" && echo "${f#plugins/}"
done | sort
```

**O-ReverseDelay and O-Tapestop carry the SAME `ui_tooltip_clamp_check.js` file
O-Bitrot did**, so Task 2's three-part fix — derive-or-abort, per-language
copy-differs, direction-agnostic geometry discriminator — ports to both nearly
verbatim. Two of the twelve are already solved on paper.

**And when you fix one, do not spell the literal in the comment that explains
it.** All three tasks hit this: the inventory grep is comment-blind, so a comment
containing `['en', 'fr']` keeps the file on the list forever. Word it as prose.

## 2. A comment-blind grep is the wrong instrument, and this plan used it four times

The same defect class, four separate times, and each one would make a *correct*
file look broken:

| Verify as written | Actual | Cause |
|---|---|---|
| `grep -c "reviewed: 'bt'"` → expect 131 (O-Octagon) | **138** | entry count vs `reviewed:` line count; templates and generated rows |
| `grep -c 'zh-Hans' PluginProcessor.h` → expect 2 | **4 / 3 / 3** | doc comments explaining the codec |
| `wc -l < o-chorus-zh.tsv` → expect 4 | **44** | `--emit` is table-scoped (finding 8) |
| `grep -n 'VERSION 1\.27\.0' CMakeLists.txt` | **no match** | the value is **quoted**: `VERSION "1.27.0"` |

The last one is new in Task 3 and is the nastiest, because a zero-match grep in a
verify block reads as "not done" when the bump landed correctly. **Write code
gates comment-stripped from the start, and never assume a CMake value is
unquoted.** For a code-only count:

```bash
perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' <file> | grep -c 'zh-Hans'
```

## 3. Type may be tokenised — and there are THREE shapes, not two

The CJK font tail cannot be applied from a `font-family` count. Measured across
four plugins:

| Plugin | `font-family` in `index.html` | where the stacks live | edits needed |
|---|---|---|---|
| O-Chorus | 8 (inline `<style>`) | inline | **5** literal stacks |
| O-Octagon | **0** | `css/styles.css`, all `var(--serif)` / `var(--mono)` | **2** tokens |
| O-Bitrot | 13 (inline) | all `var(--serif)` | **1** token |
| O-MicrotonalSampler | **0** | **two** external files, **7 literal stacks** | **23** declarations + a new rule |

A wave executor who greps `index.html` gets **zero hits** on two of the four. One
who greps the stylesheet and edits every declaration makes 27 pointless edits on
O-Octagon. **Grep for `--*:` type tokens FIRST**; if the stacks are tokenised the
tail goes on the tokens, and if they are not you are in the O-MicrotonalSampler
case and must measure.

## 4. MEASURE the resolution set — and the aria-label split is per-plugin

Serve the page, switch to Chinese, read `getComputedStyle(node).fontFamily` on
every node that **holds or can receive** a Han codepoint (own text, `data-tip`,
`data-tip-title`, `aria-label`). A `[data-i18n]`-derived list under-covers.

**The two nodes a keyed scan always misses are the same two on all four
plugins: the `#tooltip` surface (filled from `data-tip` at hover time, never
keyed) and the endonym `<option>` (the only Han in the markup). 4-for-4 — treat
it as universal.**

**The bare-Arial stack splits differently per plugin, and this is the finding
that stops a wave copying the wrong precedent.** Chromium's UA sheet gives a
`<button>`, `<select>` or `<input type=range>` that names no font-family a bare
`Arial`:

- **O-Octagon:** reached *only* through `aria-label` on 16 sliders. Correctly
  **left alone** — an accessible name is never rendered text, it is spoken, and
  the screen reader picks its own face. There is no glyph to fall back for.
- **O-MicrotonalSampler:** 16 node shapes, of which **11 carry VISIBLE Chinese**
  on `<button>` and `<option>` (载入 .scl, 生成, 圆周, 全部类别 …) and 5 are
  aria-label-only sliders. The 11 **took the tail with Arial kept FIRST** so
  Latin metrics are unchanged; the 5 were left alone for O-Octagon's reason.

**Record both halves.** A wave that appends the tail to everything it measured
can no longer tell a needed tail from a decorative one.

## 5. Chinese geometry: FIVE failure shapes, and four of them are shrinks or squeezes

Chinese buys width and spends height. Only the first shape was known before this
stage.

**(a) `line-height: normal` inheritance — the dominant cause every time.** Han
faces carry taller metrics. O-Chorus 26 movers, O-Octagon 98/196, O-Bitrot 185,
O-MicrotonalSampler **504 of 646**. Pin each named leaf at its **measured English
line box**, unitless. **No global rule** — a global `line-height` moves English,
which is the regression the gates exist to catch.

> **A class that appears at two sizes needs TWO pins.** O-Octagon's
> `.vcell-value` (11 px and 9 px), O-Bitrot's `.caption` (12 px and an 11 px
> inline override). One ratio cannot serve both.
>
> **And a pin can leak DOWN into a differently-sized child.** O-Bitrot's
> `#edgeBtn` holds an 11 px caption and an 8 px fleuron; pinning the button
> moved the fleuron on the **English** arm.
>
> **Derive the pin from the BOX (height − padding − border), not from
> `Range.getBoundingClientRect()`.** On O-MicrotonalSampler the two disagree by
> 1 px, because Range measures the text's own ink rect while the box is sized by
> the strut.

**(b) Width — a content-sized element that SHRANK.** O-Octagon's
`#readout-label-source` 54.95 → 12.42 pulled three footer readouts 42.5 px left.
O-MicrotonalSampler's `#save-preset-btn` 89.66 → 83.02 pushed `#tab-strip`
7.3 px. **The report names a container with no copy in it at all.**

**(c) The UN-WRAP.** Chinese says the same thing in fewer characters, so a prose
block takes one fewer line and its container gets SHORTER — every report is
`dh = -(one line-height)` and **nothing grew**. Pin `min-height` to the English
box.

> O-MicrotonalSampler needed a second pin *inside* the paragraph: three inline
> `<code>` tokens sat one line higher because the prose **before** them
> shortened. Pinning the paragraph's own box does not hold its inline flow.

**(d) NEW — HAN WRAPS BETWEEN CHARACTERS, so a Han run's MIN-CONTENT width is
ONE character.** A Latin word's min-content is the whole word. In
O-MicrotonalSampler's 13-column auto table that let `模式` (22.88 px) be squeezed
**below its own two-character width and wrap to two lines — while being NARROWER
than the "Mode" it replaced**. The header row followed it and thirteen `<th>`
reported `dh=11.2`. **"Chinese is narrower so it fits" is false in any
auto-sized table or grid.** Fix with `white-space: nowrap` **plus** a min-width
at the measured English column; `nowrap` alone stops the wrap but leaves the
column narrower, so the table width still differs.

**(e) NEW — `min-width` IS A FLOOR, NOT A CAP, so an existing French-era pin does
NOT protect you.** O-MicrotonalSampler's `.drop-zone-text` was pinned at 134 px
and Chinese renders **137.70**; it sailed straight past and pushed four
separator dots sideways. **An element that already carries a pin is not
automatically safe in a third language — re-measure it.**

> **Wave guidance for gate authors:** any assertion phrased "the non-English pass
> must not GROW box X" is vacuous against Chinese, and "must not SHRINK" is
> vacuous against half of it. **Assert EQUALITY.** Pin at the widest of all three.

## 6. Check specificity before appending a pin block — a Task-1 pin un-pinned a FRENCH one

O-Octagon's `.plan-caption > .caption-key { min-width: 38px }` at specificity
(0,2,0), appended at the end of the file, **overrode a v1.9.0 French pin**
`.caption-field { min-width: 50px }` at (0,1,0). `CHAMP` fell back from 50 to 38
and `#field-legend` moved 8.1 px on the **fr** arm — a regression the Chinese
work had no business causing, caught only because the gate drives every language
in one run. Fixed with `:not(.caption-field)`.

**Grep for existing `min-width` / `min-height` / `line-height` on the classes you
are about to touch, before appending.** On O-MicrotonalSampler this check came
back clean (`tuning-panel.css` declared no line-height at all), which is why the
13 pins there were safe.

## 7. THE GATE CANNOT NAME WHAT NEVER RENDERS

Half of O-MicrotonalSampler's line-height families live in **dialogs, a context
menu and a popover** — none of which is in the DOM at rest. A first sweep of the
resting page named 13 leaf families and looked complete; driving **every state in
`tests/i18n-states.json`** — the same list the geometry gate drives — found nine
more, in four families, plus the entire rotation-view finding (5d) which lives
behind a visualiser button.

**Drive the plugin's own state file, not the screens you can think of.** O-Chorus
hit the same trap on `.settings-label`, hidden at rest.

## 8. `--emit` is TABLE-scoped, not ship-bar-scoped

`i18n-zh-backtranslate.js --emit` walks every row that has a `zh-Hans` value and
**ignores `reviewed` entirely**. There is no `--only-mt`. On a table that is
uniformly `'mt'` this is invisible; O-Chorus was the first with a MIXED bar
(38 `bt` + 6 `mt`) and emitted **44** where the plan said 4.

Consequence: any plugin that lands its table in two passes re-emits
*everything* on the second pass. That is not harmful — the extra rows get a
second free reading — but **every row-count expectation written against the ship
bar is wrong**, and an executor who trusts the plan's number will think the tool
is broken.

**A correction loop is a full re-emit to a NEW `--out`, trimmed by hand.** That
mints a fresh salt and a fresh manifest, so no id is shared with round 1 and the
reverse agent cannot correlate which rows were rejected. `--ingest` joins
whatever ids the returned file carries; a partial return is not a refusal.

## 9. A correction round needs a FRESH READER, not just a fresh salt

Both matter and they guard different things:

- **Fresh salt** stops the agent correlating the correction batch against ids it
  has already seen, which would tell it which rows were rejected.
- **Fresh agent** stops it agreeing with **its own earlier answer**. An agent
  that returned "Jitter" for 抖动 an hour ago and is now handed 抖动噪声 has a
  strong prior toward self-consistency — which is the round-tripping failure the
  whole flag exists to prevent, arriving one step later.

Both O-Bitrot's round 2 and O-MicrotonalSampler's round 2 used both.
**Stage 4 should require both explicitly.**

## 10. `--verbose` is not optional — three consecutive stages, and the tail holds the defects

`--ingest` truncates to `MAX_SHOWN` = 12 and prints `… N more (--verbose)`.

- **O-Octagon:** both findings ranked **22nd and 96th** of 246.
- **O-Bitrot:** the seven re-authored rows ranked **1, 14, 39, 50, 56, 79, 84**
  of 172; four scored **0.50+**. The `CRUSH_ENABLE` body — whose
  back-translation is not a sentence — scored **0.73**, rank 79.
- **O-MicrotonalSampler:** the one re-authored row ranked outside the twelve of
  296; twelve is **4%** of that batch.

**Zero of nine real findings across the stage ranked inside the default twelve.**
The score is a **sort key, not a verdict** — O-Bitrot's round-2 corrections came
back scoring 0.61–0.91 and every one was right.

## 11. TWO English words on ONE page can share ONE glossary root — and NO LINT SEES IT

**The single highest-value screen to run BEFORE authoring.** `Dither` and
`Jitter` both root on **抖动** and sit two cells apart in O-Bitrot's Crush plate.
`Z5` reported **zero findings** on the wrong table (抖动 is exactly what the
glossary asks, on both keys), `F1` was silent, `check-ui-labels` was silent —
**every automated check in this repo passed a table that ships an unreadable
sentence.** Only the blind reverse pass found it, three times.

Same shape twice more: `Mains` roots on 主输出 (a mixer's main OUTPUT bus) where
O-Bitrot means the **electrical** mains; and `divisions` roots on 分割 (generic
"cutting apart") where O-MicrotonalSampler means the **count of equal divisions
of the octave**, two cells from `Equal Divisions` (等分) and `EDO (Equal
Division)` (等分八度) — **the glossary contradicting itself inside one panel**.

**Resolutions, and the reasoning to copy:** qualify **BOTH** sides, never one.
Jitter → 时基抖动, Dither → 抖动噪声, Mains → 市电, Divisions → 等分数. The
moment one member of a homograph pair is qualified, the unqualified one reads as
the general case — which is what the reverse pass returned, twice.

**The screen is a few lines**, and it is the only way to find this before it
ships, because nothing downstream will:

```js
// push the plugin's English label set through TERMS; report two DIFFERENT
// keys that map to the same array AND both occur on this page
```

## 12. The discriminator for re-author vs accept is COLLISION ON THE PAGE, not drift distance

Three times this stage a Chinese string came back saying something its English
did not, and the calls went different ways for a stateable reason:

| | O-Bitrot r1 | O-Bitrot r2 | O-MicrotonalSampler r2 |
|---|---|---|---|
| zh | `抖动` (Dither) | `快抖` (flutter) | `力度交叉` (Vel-XF) |
| en' | "Jitter" | "the faster jitter" | "Dynamics Crossover" |
| verdict | **RE-AUTHORED** | **ACCEPTED** | **ACCEPTED** |
| why | it was the glossary root `Jitter` would also take, so two controls' captions were one qualifier apart and the body read as nonsense | collides with no other string; 快抖 / 时基抖动 / 抖动噪声 are three distinct renderings and 快抖 is the partner of the root its own title establishes | 交叉 occurs in exactly two strings — this caption and its own tooltip title; 力度 is velocity everywhere while Dynamics has its own rendering 动态; no crossover control exists to confuse it with |

**The question is not "how far did en' drift". It is "can a reader ON THE PAGE
confuse this string with another control on the same page".** A contextless
reverse pass cannot answer that — it has no page — so the answer must come from
the executor and **must be written down**. An accepted drift with no recorded
reason is indistinguishable from an unread one.

**A caption must also not collide with its OWN tooltip title.** O-MicrotonalSampler's
Vel-XF knob read 力度渐变 ("velocity *fade*") while its own tip said 力度交叉渐变
("velocity crossfade") — two names for one control, the exact defect that
plugin's French pass had already recorded as N1 correction 11. The fix is an
abbreviation that is a literal **prefix** of its own title (力度交叉), mirroring
how "Vel-XF" abbreviates "Velocity Crossfade".

## 13. Where the executor's authority ENDS — a reverted correction

`label.genRank2` (二阶音律) came back **"Second-order tuning"**. In regular-
temperament theory "rank" is the number of independent generators — 秩 in Chinese
mathematics, not 阶 (order). I changed it to 秩 2 音律 and **Z5 fired: 二阶音律
IS the glossary's own root.**

**Reverted.** Unlike `genDivisions`, which earned its `termNote` on an objective
**page collision**, this is a pure terminology preference with **no collision and
no native reader to adjudicate it**. Overriding a settled glossary root on one
executor's judgement is exactly what the glossary exists to prevent. The concern
is recorded for the glossary owners instead.

**`termNote` is the sanctioned override and Z5 is exact-match**, so the mechanism
is there — but spend it on collisions, not preferences.

## 14. The lint contradicts itself on six of the glossary's own roots

`i18n-zh-lint` reports **8 `Z1` findings** on O-MicrotonalSampler and **six are
byte-identical to `TERMS` roots**: 载入 .scl, 载入 .kbm, 保存 .scl, 保存 .kbm,
否则使用文件名标记 `(`. **`Z5` compels the root and `Z1` then flags it.**

Cause, diagnosed not guessed: `ruleZ1`'s `maskLatin()` masks ASCII punctuation
only **between two alphanumerics** (`1.5`, `kHz/ms`, `20-40`). A **leading-dot
file extension** (`.scl`) has no alphanumeric before the dot, so the dot survives
masking and reads as Han-prose punctuation. The unbalanced `(` is the second gap:
`maskLatin` masks only **balanced** parens, and that string is deliberately split
around markup. The remaining two (`aria.savePreset` / `aria.loadPreset`,
`.omspreset`) are the same extension shape following the house style the glossary
sets.

**Reported as a tool defect, not worked around**, on the Task-1 precedent for
Z3's 像 false positive: *if the lint fires on the glossary's own rendering, that
is a tool regression to report, not a string to change.* The lint is
**report-only (exit 0)**, so nothing is blocked. **Fix `maskLatin` before Stage 4
turns the lint into a gate**, or twelve plugins with `.scl`/`.kbm` buttons will
inherit it.

## 15. A space between two Han runs is invisible to every lint rule

Found by **reading** triple 96 on O-Octagon, not by any gate:
`同时写入 源 X 与 源 Y` carried two stray U+0020 between Han runs. **Z4 polices
only the Latin/Han boundary**, so Han-space-Han never enters the census, and
nothing else in the nine rules looks at intra-Han whitespace.

Run this before promoting any plugin to `'bt'` (0 on all four here):

```js
const re = /\p{Script=Han} \p{Script=Han}/gu;   // must find nothing
```

**Candidate rule Z8.** Cheap, mechanical, and it caught a real defect on the
first plugin after the pilot. Whoever adds it must check its zero column against
this standalone scan across all 43 first
(`pattern_new_lint_zero_must_agree_with_independent_scan`).

## 16. The em-dash convention has already drifted, and no rule sees it

Measured across the three shipped zh tables: **28 double em-dashes, 18 spaced,
10 unspaced.** O-Chorus and O-Octagon ship `——` unspaced; O-Bitrot ships ` —— `
spaced. **Standard Chinese typography is UNSPACED** — the full-width dash carries
its own sidebearing, which is the same reasoning the lint's own `Z2` uses to
reject U+00A0 before full-width punctuation.

O-MicrotonalSampler was authored unspaced. **Settle this in the glossary or a
rule before 39 more plugins pick a side at random**; nothing currently catches it,
and it is invisible to the Han-space-Han scan too (the dash is not Han).

## 17. The stale language-enumeration defect is systemic — 4 for 4

**Every plugin in this stage shipped a `tip.language` / `lang-select` body that
COUNTED the selector's options and went false the moment a third landed.**

- O-Octagon en: *"English and French are available"*
- O-Bitrot en: *"English and French are available"*
- O-Chorus en/fr: *"English or Français."* — **false in the field since v1.5.0**
- O-MicrotonalSampler en/fr: *"…between English and French."*

**Fixed the same way in all four: the enumeration is REMOVED, not extended.**
Naming all three would put Han inside the `en` and `fr` bodies, which moves the
English tooltip's own geometry and drags the CJK tail onto the very baseline
every gate measures against. The selector already lists them in their endonyms —
the one form a reader recognises without knowing the page language.

**No gate can see this**: the sentence is grammatical, the tooltip renders, and
no lint has a notion of "counts its own options". **This is a 43-plugin sweep
waiting to happen** — every plugin with a language selector has a body describing
it, and every one is a candidate.

## 18. Per-row blinding cannot hide a product name that appears in the copy

O-MicrotonalSampler's `aria.techniquePreset` legitimately contains
`O-MicrotonalSampler` (an `I18N_EXEMPT` product name, present identically in the
English), so the emitted batch **identifies the plugin** even though ids are
blinded and no translated row's English source is present.

Not a `reviewed: 'bt'` failure — the reverse agent cannot learn any row's English
from it — but **the dispatch must forbid repo access explicitly**, which is what
was done here. Several of the 39 name themselves in copy.

**Fire the blinding controls on every batch rather than trusting the code
exists:** ids pure 12-hex, no key fragment, ids returned identical and in order,
rows well-formed, and a sha256 of the zh column against the committed tree.

## 19. Structural variants a wave will hit blind

- **UI root:** `Source/ui/public/` on most, **`Resources/ui/`** on
  O-MicrotonalSampler and **nine** others. The C++ still lives under `Source/`
  either way, so the Han gate's scope is unchanged.
- **Controller:** a normal `js/app.js`; **none at all** (O-Bitrot — an inline
  `<script type="module">` at `index.html:1429`); or **two**
  (O-MicrotonalSampler — `sampler-app.js` + `tuning-panel.js`).
  `check-i18n` assertion 6 byte-compares each copy against `scripts/i18n-canon.js`
  and **PASSES on the inline-module shape**, naming it explicitly — so the
  Stage-0 "all 43 shipping copies" claim is now *asserted* for the layout it was
  least likely to cover, not inferred.
- **`<meta charset>` past the 1024-byte prescan window** (O-Octagon, byte 2926).
  **The byte offset is not the guarantee and never was**: `PluginEditor.cpp`
  serves `index.html` as `text/html; charset=utf-8`, and `scripts/serve-ui.js`
  does the same for the harness. **Check the plugin's own `getResource()` before
  treating the offset as a defect.** Numeric entities are kept for convention.
- **Content-sized `<select>` is a WIDTH problem, not a line-height one**, and it
  moves in **both** directions: O-Bitrot's `#viewSync` GREW (58/65/66) while
  `PACKET_CONCEAL` SHRANK 25 px (82/82/57) on the same page. With
  `appearance: none` Chromium still derives the intrinsic width from the
  **widest option's** font run. The 25 px shrink surfaced as a *knob* moving.

## 20. Two more traps that cost real time

- **zsh does not word-split.** `for c in "name:cmd with args"; do $cmd; done`
  runs the whole string as one word and every gate reports **exit 127**, which
  reads exactly like a broken gate. It also bites `set -- $spec` and
  `grep ... $FILES`. **A clean sweep of 127s is a broken loop, not a pass.**
- **Author at `'mt'`, ALWAYS.** Task 1 inserted 138 blocks already carrying
  `'bt'` and **every gate in the repo went green**: `check-i18n` accepts `'bt'`
  as a valid enum member and `i18n-zh-lint` R1 only reports entries *below* the
  bar, so a premature `'bt'` is **invisible to every automated check**. Nothing
  would have caught it. The flag is the one field no gate can validate.

## 21. The commit shape to copy

Work lands first at `'mt'`; the reverse pass runs against a **committed** tree; a
correction round gets its own commit that says plainly what the read found; and
the promotion plus ship metadata land together in a final commit whose message
names **every** provenance string. A single commit makes the bar unauditable
after the fact — and O-Bitrot needed four before it earned it.

---

## Deviations from plan

### [Rule 1 — Bug] Four plugins shipped a false language enumeration
Finding 17. Removed, not extended, in all four. Commits `88575ff1`, `88c36535`,
`43246871`, `567deaab`.

### [Rule 1 — Bug] Eight rows re-authored after the triple reads
Seven on O-Bitrot (findings 11, 12), one on O-MicrotonalSampler (finding 12).
Not a deviation from the plan so much as the plan's own instruction firing —
"fix the Chinese, then re-emit and re-ingest those ids" — at the cost of two
extra checkpoints, which is what `'bt'` costs when the read actually finds
something.

### [Rule 3 — Blocking] Four verify commands are wrong as written
Finding 2. None is a code defect; all four would make a correct tree look broken.

### [Scope] `scripts/i18n-zh-glossary.js` untouched on all three plugins
The plan permits a `BUDGETS` addition "only if a new measured cell was found".
**None was**, on any of the three — `check-ui-labels` assertion 4 passes on all
arms and every already-pinned cell has room. The three existing cells are
O-Chorus's and are intact. New domain vocabulary is recorded in each plugin's
`i18n.js` header and in finding 11 here **rather than promoted into `TERMS`**:
adding roots retroactively risks putting other plugins out of Z5 conformance,
which is a repo-wide change this plan does not own. **Stage 4 should promote them
deliberately and re-run the repo-wide lint to prove no plugin regressed.**

### [Scope] `scripts/i18n-zh-lint.js` untouched
Finding 14. The Z1/Z5 contradiction is real and diagnosed, but the lint is
report-only and fixing it is a repo-wide tooling change outside this plan's
scope.

### [Scope] O-Chorus shipped 1.6.1 → 1.6.2, not the 1.5.0 → 1.5.1 asked for
The plan, the coordinator's instruction and `PLUGINS.md` all said 1.5.0;
`CMakeLists.txt` read **1.6.1**. **C-8 as a rule: bump from the CMakeLists value,
never from the registry row, never from the plan, never from a handoff message.**
O-MicrotonalSampler's CMakeLists agreed with the plan (1.26.1) — but its value is
**quoted**, which broke the verify grep (finding 2).

---

## Gate results — final, against the shipped tree

| Gate | Result |
|---|---|
| `check-i18n` (repo-wide) | **exit 0** — `ALL CHECKS PASS — 43 localized plugin(s)`, four now three-language |
| `check-i18n --plugin` ×4 | **exit 0** each; `LANGUAGES … ["en","fr","zh-Hans"]` |
| — assertion 6, O-Bitrot inline module | **PASS**, canon v2 — the no-`app.js` shape is covered |
| `check-ui-labels` O-Octagon | **exit 0**, 0 FAIL; 20 fr + 20 zh geometry PASSes; zh vacuity 100% |
| `check-ui-labels` O-Bitrot | **exit 0**, 0 FAIL — **was exit 2 / 2 FAIL** (ZH3-09) |
| `check-ui-labels` O-Chorus | **exit 0** |
| `check-ui-labels` O-MicrotonalSampler | **exit 0**, 0 FAIL — **was 160 FAIL**; 84 fr + 84 zh geometry PASSes; zh vacuity **99%** |
| `ui_layout_check.js` (O-Octagon) | **exit 0**, 34 sections; new section 34 drives every language; **positive control FIRED** (zh-only FAIL, fr arm still PASS) |
| `ui_tooltip_clamp_check.js` (O-Bitrot) | **exit 0**; real zh pass, **14 clamps engaged**, 55/55 anchors |
| `ui_tip_render_check.js` (O-MicrotonalSampler) | **exit 0**; real zh pass, **136 `zh-Hans` mentions**, 22 anchors byte-compared against the Chinese table; **derive-or-ABORT control FIRED** (exit 2 on a planted empty `LANGUAGES`) |
| two-language literal in the three fixed gates | **0**, comments included |
| `i18n-zh-lint` per plugin | **0 findings** on O-Octagon / O-Bitrot / O-Chorus; **8 Z1** on O-MicrotonalSampler, six of them the glossary's own roots (finding 14) |
| — ship bar | `BELOW SHIP BAR … 0` on all four |
| `i18n-zh-lint --self-test` | **9/9** |
| `i18n-zh-backtranslate` (stage view) | O-Octagon 246 · O-Bitrot 172 · O-Chorus 44 · O-MicrotonalSampler 296 — **TOTAL 758 / 758 bt / 0 mt** |
| `i18n-fr-lint` | **exit 0**, `CLEAN`, **0 / 43** plugins with findings — French untouched throughout |
| `boot-all-uis --strict-tips` | **exit 0**, **0 DEAD**, 0 late, 0 failed on all four |
| Han gate over each `Source/**/*.{h,cpp}` | **NO OUTPUT** ×3, each with its **positive control FIRED** on that plugin's own `i18n.js` |
| `PluginProcessor.h` codec | exactly **2** code occurrences of `zh-Hans` each (comment-stripped) |
| Han-space-Han scan | **0** on all four |
| ZH3-06 pluralization scan | **0 live sites**, comment-stripped, with the regex **4/4 on a positive control** |
| `PLUGINS.md` duplicate rows | **EMPTY** |
| `auval -v` ×4 | **AU VALIDATION SUCCEEDED** ×4, one cold rescan |

**Every negative gate has a positive control that fired.** A zero from a gate
whose control was never run is not evidence.

---

## Commits

Nine, all path-scoped; the submodule guard ran clean before each.
`.gsd/dispatch-isolation-sentinel.json` was modified before this work began and
was deliberately **never** staged.

| Commit | Scope | Content |
|---|---|---|
| `88575ff1` | `plugins/O-Octagon` | 187 entries at `'mt'`, endonym, codec, CJK tail on two tokens, seven pins, gate section 34 |
| `125f8a0b` | `plugins/O-Octagon` `PLUGINS.md` | promotion after the 246-triple read, v1.12.0, CHANGELOG, registry row |
| `88c36535` | `plugins/O-Bitrot` | 117 entries at `'mt'`, tail on one token, nine pins incl. the `#viewSync` baseline closure, clamp-gate derivation |
| `43246871` | `plugins/O-Chorus` | `tip.language` enumeration removed from all three bodies |
| `d788b769` | `plugins/O-Chorus` `PLUGINS.md` | all 44 rows promoted, v1.6.2, CHANGELOG, registry row |
| `4a26b265` | `plugins/O-Bitrot` | the seven re-authored rows; 159 promoted, 13 held at `'mt'` |
| `d702f6f4` | `plugins/O-Bitrot` `PLUGINS.md` | last 7 promoted after round 2, v1.16.0, CHANGELOG, registry row |
| `567deaab` | `plugins/O-MicrotonalSampler` | 275 entries at `'mt'`, endonym, codec, tail across two stylesheets + the UA-default rule, 13 line-height pins, 5 width/un-wrap pins, tip-gate shape assertion |
| `5c4b326f` | `plugins/O-MicrotonalSampler` `PLUGINS.md` | all 296 rows promoted after two rounds, v1.27.0, CHANGELOG, registry row |

---

## Self-Check: PASSED

Files verified present on disk: all four `i18n.js` tables, O-Octagon
`css/styles.css`, O-MicrotonalSampler `css/sampler-shell.css` and
`css/tuning-panel.css`, the three repaired gate files, four `CHANGELOG.md`, four
`CMakeLists.txt`, `PLUGINS.md`, and this summary. Installed bundles verified via
`CFBundleShortVersionString`: O-Octagon **1.12.0**, O-Bitrot **1.16.0**,
O-Chorus **1.6.2**, O-MicrotonalSampler **1.27.0** — `-dev` variants only, no
alternate-variant orphan. All nine commits verified in `git log`. Working tree
carries no uncommitted plugin change; the only modified path is
`.gsd/dispatch-isolation-sentinel.json`, which predates this work and was never
staged.
