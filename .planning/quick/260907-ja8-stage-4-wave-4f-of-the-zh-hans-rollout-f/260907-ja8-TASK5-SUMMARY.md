---
phase: quick-260907-ja8
plan: 01
task: 5
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4f, O-Bowed, scala-tuning-engine, gates, fonts, geometry, enumeration-census]
status: complete
requirements: [ZH4F-05, ZH4F-06, ZH4F-07, ZH4F-09]
dependency_graph:
  requires: [260907-ja8 tasks 1-4 (O-GrainScatter, O-Wind, O-Contrabass, O-Reed)]
  provides: [O-Bowed v1.9.0 three-language, built and installed as -dev]
  affects: [PLUGINS.md row (Task 6), the cold auval sweep (Task 6)]
tech_stack:
  added: []
  patterns: [derive-or-abort language list, CJK tail before the trailing generic, line-height as a RATIO, R8 floor re-measurement, blind reverse read at reviewed:'bt']
key_files:
  created: []
  modified:
    - plugins/O-Bowed/Resources/ui/js/i18n.js
    - plugins/O-Bowed/Resources/ui/index.html
    - plugins/O-Bowed/Source/PluginProcessor.h
    - plugins/O-Bowed/tests/ui_tip_render_check.js
    - plugins/O-Bowed/CMakeLists.txt
    - plugins/O-Bowed/CHANGELOG.md
decisions:
  - "KEEP the #tuning-container .tuning-panel override and re-measure the floors, rather than taking the fallback of appending a tail to the system stack — the Latin arms measured 0 moved."
  - "ALL THREE d848337a floors were re-measured, not just the octave-stretch one Task 4 predicted."
  - "label.count 数量 -> 弦数 with a termNote: 量 is a substring of 数量 and the blind reader returned 'Amount' for both captions."
metrics:
  duration: ~3h
  completed: 2026-09-07
actuals:
  tokens: 96000
  tasks: 1
  commits: 2
commits:
  - 089e65e6  i18n(O-Bowed) add Simplified Chinese at reviewed:'mt'
  - f25090c8  i18n(O-Bowed) promote 161 zh rows to reviewed:'bt' — v1.9.0
---

# Task 5: O-Bowed — Simplified Chinese, v1.9.0 Summary

O-Bowed ships English, French and Simplified Chinese at `reviewed: 'bt'` on all **161** rows,
version **1.9.0**, built and installed. Its tuning overlay no longer renders through the shared
module's `-apple-system` stack, its 30 bare-Arial form controls are closed, its gate derives and
compares every language it is given instead of asserting a two-member list, and three hover-help
clauses that were false on the tree before a byte of Chinese are corrected.

**HEAD at start: `f3737f53`** (the plan's `d37cdcc0` was five commits stale — orchestrator
correction 3 confirmed).

---

## 1. Preconditions — all nine FIRED on the tree as found, none inherited

| | check | predicted | **measured** |
|---|---|---|---|
| a | `i18n-zh-lint --self-test` | 10/10 | **10/10, exit 0** |
| b | `check-i18n --plugin O-Bowed` | exit 0, `["en","fr"]`, 48 I18N + 82 LABELS, `[12]` 2 module(s) | **all four exactly** |
| c | `i18n-zh-lint --plugin O-Bowed` | exit 0 | **exit 0, 0 findings** |
| d | `check-ui-labels --plugin O-Bowed` | exit 0, `66 of 76`, 10 never-visible | **exactly**, `== ALL CHECKS PASSED ==` |
| e | `ui_tip_render_check.js` **before the flip** | exit 0 | **exit 0, 470 PASS assertions** |
| f | 37 module keys in en/fr; dup-key scan | 37/37, 0 dups | **37/37 in O-Bowed AND in O-Bassoon; 0 dups on 130 entry keys** |
| g | `git status --short` / `git tag --points-at HEAD` | only `?? plugins/O-Strata/` / 0 | **FALSE — see §7.1**; tags **0** |
| h | repo-wide `i18n-zh-lint` | exit 2 is the baseline | **exit 2, `0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read`** — the orchestrator's stated baseline verbatim |
| i | Tasks 1-4 committed | — | **b534636b, a92b7980, 020409f8, bff367f6, 5a216066, b69ca8ea, 9d83e7a7, f3737f53 all present** |

**Two-arm CMake reader, run unconditionally on a QUOTED literal:** arm 1 (`^\s*VERSION`) returns
`1.8.0`; arm 2 (the `set(...VERSION ...)` form) matches nothing, because this file has no variable
form. The two agree — the reader that only works where it was written would also have worked here,
which is exactly why it is run anyway.

---

## 2. What shipped

| | before | after |
|---|---|---|
| `LANGUAGES` | `['en','fr']` | `['en','fr','zh-Hans']` |
| `check-i18n` I18N + LABELS | 48 + 82 | **48 + 82 — UNCHANGED** |
| duplicate-key scan (LANGUAGES members excluded) | **NONE**, 130 entry keys | **NONE**, 130 entry keys |
| zh rows | 0 | **161** — 45 LABELS + 48 titles + 31 bodies + **37 module** |
| `reviewed` enum census | — | **`{"bt": 130}`** — every entry, no `'mt'` left |
| gate assertions | **470** at two languages | **659** at three |
| `undeclared-font` | 0 (a vacuum — no Han existed) | **0 against 95 Han nodes** |
| nodes on `-apple-system` | **116** (en) | **0 en / 0 zh** |
| bare `Arial` | **30** | **17** |
| `check-ui-labels [7]` fr | 0 moved | **0 moved** |
| `check-ui-labels [7]` zh | — | **0 moved**, 4 states |
| version | `VERSION "1.8.0"` | **`VERSION "1.9.0"`** |

**Row arithmetic, checked three ways.** The emitter counts **161**. 45 + 48 + 31 = **124**
non-module (the 17 `canvas.*` entries have empty bodies and are not counted as rows), + **37**
module = 161. The blind batch is exactly the 124.

### The 37 module rows were COPIED, not authored

The key list was **derived**, not transcribed: `grep -rhoE 'data-i18n(-aria)?="[^"]+"'` over
`modules/tuning/scala-tuning-engine/` returns 38, less the `label.xxx` inside a doc comment = **37**.
Present 37/37 in O-Bowed and in O-Bassoon. A node deep-compare of the `en` and `fr` objects across
the two plugins returned **0 divergences on all 37**, so the splice could take O-Bassoon's
`'zh-Hans'` tail verbatim. One of the 37 (`label.genDivisions`) is a multi-line entry carrying a
termNote; it came across with its note intact.

All 37 landed **at `reviewed: 'bt'`** in the SAME edit as the `LANGUAGES` flip, and were **excluded
from the blind batch** — stated in the commit.

---

## 3. Stale copy — corrected FIRST, file re-read, then authored (C7)

The comment-stripped probe reported **2** hits, as J10 predicted, and both were defective. After the
corrections the code-scoped probe reads **0**; the raw file still reads 2, both in unrelated comments
(`#sympAmount-ctrl and nothing else` describing a DOM scope, and `holding the 55 px SVG … and nothing
else` describing an SVG's contents). Neither is a J9 site and both were **left** — Task 4 found the
same class on O-Reed and a reflexive deletion sweep would have destroyed correct comments.

### J8 — `tip.language` promised the Tuning page stays English. FALSE since v1.8.0.

- en: *"Value readouts **and the Tuning page** stay in English."*
- fr: *"Les valeurs affichées **et la page Accord** restent en anglais."*

s71 localized the shared panel at module v3.1.0 and **this plugin's own v1.8.0 shipped all 37 of its
caption rows**, so the clause has been a shipped falsehood in both languages since the release that
made it false. The comment above it was worse than the body — *"THE TUNING CLAUSE IS LOAD-BEARING AND
IT IS TRUE"*, in capitals. Both corrected. The readout clause beside it is still true and stays.

### J10 — the same body enumerated the selector's options. Removed.

en *"English or Français."* / fr *"English ou Français."* The surrounding sentence was re-worded from
*"stay in English"* to *"stay in English **whichever language is chosen**"* / *"restent en anglais
**quelle que soit la langue choisie**"*, so it holds for any number of languages.

### J9 — `tip.settings` claimed the panel holds the language and nothing further. FALSE since v1.7.0.

- en: *"It holds the interface language **and nothing else**."*
- fr: *"Il contient la langue de l'interface **et rien d'autre**."*

`#tips-toggle` sits inside `.settings-popover`, in a second `.settings-row` beside `#lang-select` —
**verified in the markup at index.html L925 (popover) / L957 (toggle)**, not inferred — and
`tip.tipsToggle` was added at v1.7.0. The comment above asserted *"this plugin has no such control
and Stage M does not add one"*. Both corrected. The measured *"opens below this button"* clause (the
gear sits in a 40px header strip at the top of a 600px frame) and the Escape clause are true and stay.

**Carry-forward 10 landed exactly as written.** The French half `rien d'autre` uses **U+2019**, and
the transcribed probe without `perl -CSD` cannot see it. Both the English and the French halves were
corrected by hand off the file, and the probe was re-run **with `-CSD`** afterwards. Had the probe
been trusted, the English would have been fixed and the French falsehood shipped.

---

## 4. The gate — form 1 AND form 7, repaired BEFORE the flip

`tests/ui_tip_render_check.js`, 688 lines. Repaired in an uncommitted working state and **run green
at two languages before `LANGUAGES` gained a third member**, so no commit in this task's history
contains a table whose third language makes its own gate red.

### N12 enumeration census — O-Bowed, before → after (comment-stripped)

| form | before | after | disposition |
|---|---|---|---|
| **1 — `LANGUAGES.join(',') === 'en,fr'`** | **1** (raw **L227**, untagged) | **0** | shape assertion + derive-or-abort |
| 2 — string `'en,fr'` | 1 (inside form 1) | **0** | went with it |
| 3 — array-literal walk | 0 | 0 | absent |
| **4 — computed / UA-face form controls** | **30 nodes** | **0** | a CSS form, §5 |
| 5 — `Map` read with a literal key | 0 | 0 | absent |
| **6 — per-language object property access** | **5** | **5** | **SOURCE / RESTORE-scoped, deliberately LEFT** |
| **7 — paired literal-argument call** | **2** (`sweep('en')` raw **L443**, `sweep('fr')` raw **L515**) | **0** | generalized to a loop |
| `__setLanguage(l), '<literal>'` | **2** | **0** | driven from the loop variable / `SRC_LANG` |
| `LANGUAGES` mentions in CODE | **3** | **9** | publish, destructure, `SRC_LANG`, shape check, abort, `REST`, walk check, abort, log |
| g5l inventory grep sees this file? | **yes** | **no** | off the list, and the comment does not put it back (C8) |
| assertions | **470** at two languages | **659** at three | |

**Carry-forward 6 was right and the plan's line numbers were wrong.** Every site was located by
PATTERN. The form-1 assertion really is at raw **L227** (exact). But `sweep('en')` is at raw
**L443**, not the plan's L339 — off by 104 — and all five form-6 sites are at raw **L476, L501, L520,
L521, L665**, not the plan's L372/L397/L416-417/L561: those are the **comment-stripped** numbers.
The switch (L511) and the `frLang === 'fr'` assertion (L514) were exact.

### The three repairs

1. **Derive-or-abort.** The loader at raw L152 already published `{ I18N, TIP_BINDINGS, LANGUAGES }`
   and raw L221 already destructured all three, exactly as J3 said — nothing to add to a loader. The
   joined-string equality became a shape assertion (non-empty array opening with the source language)
   plus `process.exit(1)` with a refusal message. **No fallback roster.**
2. **A SECOND, INDEPENDENT guard on the WALK.** `const REST = LANGUAGES.slice(1)` with its own
   `check()` and its own refusal. A one-member list satisfies the shape and then compares the page to
   itself, passing every byte-equality assertion for the wrong reason.
3. **The paired call became a loop.** `sweep(lang)` was already language-agnostic inside — raw L279
   reads `const entry = (I18N[key] || {})[lang];` — so the whole defect was the caller. The switch,
   the selector assertion and the sweep moved inside the loop. Live: `sweeping 3 language(s): en ->
   fr -> zh-Hans`.

**No page-state call sits between O-Bowed's two sweeps** (unlike O-Wind's `showTab('sound')`) —
checked by reading the 60 lines between them, not assumed. The block that sits there is NC-3 and
NC-4, which park the pointer and leave the page in the source language.

### Derive-or-abort control — fired TWICE (carry-forward 7)

| plant | expected | observed |
|---|---|---|
| `LANGUAGES = []` | the shape assertion refuses | `FAIL: LANGUAGES derives from the table as a non-empty array opening with the source language — got []` → `REFUSING to sweep a guessed language list.` **exit 1** |
| `LANGUAGES = ['en']` | shape PASSES, the walk guard refuses | `PASS: … got ["en"]` then `FAIL: the derived list carries at least one language besides the source language — got ["en"]` → `REFUSING to compare the page to itself.` **exit 1** |

The second is the load-bearing one. Both plants reverted by **targeted `perl -i` edit on one line,
never `git checkout --`**; sha256 `1da3fb29…8bdc5670` **byte-identical on all three readings**
(before, after plant 1, after plant 2), and `git status -- plugins/O-Bowed` read 0 afterwards.

### The five form-6 sites LEFT, classified at their own sites

| raw line | site | class |
|---|---|---|
| **L476** | `I18N['tip.bowSpeed'].en.t` inside NC-3 | **SOURCE-LANGUAGE-scoped** — the block runs before any switch |
| **L501** | `I18N['tip.bowPressure'].en.t` inside NC-4 | **SOURCE-LANGUAGE-scoped**, same reasoning |
| **L520-521** | `I18N['tip.brightness'].en.t` / `.en.b` after the restore | **RESTORE-scoped** — the line above writes the source language |
| **L665** | `I18N['tip.bowSpeed'].en.b` after the NC-1 plant restore | **RESTORE-scoped** — the `finally` copied the pre-plant snapshot back |

A prose comment now says so at each site. Generalizing any of them would break the restore semantics
the assertion exists to prove. **Carry-forward 8 said to count them by grep rather than from the
plan; the grep says 5 and the plan's list also implied 5, so the count agreed and only the line
numbers did not.**

---

## 5. Fonts — 95 Han nodes, 0 of which resolve without a named CJK face

**10 `font-family` declarations: 1 named stack (raw L67) + 9 `font-family: inherit`** — exactly J11,
both halves. The computed `ff` census over **427** visible English nodes reproduced J13's three
populations **to the node**:

```
281  Garamond, Georgia, "Times New Roman", serif      the house stack (raw L67)
116  -apple-system, "system-ui", sans-serif           THE ENTIRE TUNING OVERLAY
 30  Arial                                            the overlay's undeclared form controls
```

Once the table landed, **95 visible zh nodes carried Han and every one of them resolved without a
named CJK face**.

### Treatment, per site

| site | treatment | why |
|---|---|---|
| the house stack (raw L67) | **tail only** | Georgia and Times New Roman both installed; Garamond installed nowhere |
| **NEW** `#tuning-container .tuning-panel` | **face + tail** | the override this plugin never had — 116 nodes on the module's `-apple-system` stack |
| **NEW** five-selector `#tuning-container` rule | **face + tail**, Latin first | `.viz-btn`, `.tuning-file-btn`, `.generator-btn`, `.generator-type-select`, `.library-filter-select` |

**FIVE selectors, not three** (J14; Task 2 c/f 4, Task 3 c/f 3, Task 4 §5, all confirmed). O-Bowed's
five `.viz-btn` carry no face of their own, which is why it reads 30 form-4 nodes where O-Wind reads
25. The 11 Han-bearing ones measured after the table landed:

```
.viz-btn x5           圆周 极坐标 矩阵 真实键位 旋转
.tuning-file-btn x5   载入 .scl / 载入 .kbm / 保存 .scl / 保存 .kbm / 导出 HTML
.generator-btn x1     生成
```

`measure-ui` reports bare Arial **30 → 17**; the 13 that left are the 11 Han buttons plus the two
`<select>`s. **`undeclared-font` does not credit the two selects** — its `han` flag reads a node's own
text plus `data-tip` / `data-tip-title` / `aria-label`, and a collapsed select's selected-option text
is none of those. **The rule covers 13 where the screen can name 11**, exactly as carry-forward 3
predicted. The remaining 17 are the measured **negative case (N3)**: twelve `.interval-input`
numerics, `#octave-stretch`, `#gen-divisions`, `#gen-period` and the two arrow glyphs `#tonic-down` /
`#tonic-up` — none renders Han and the module gives none of them an `aria-label` or a `data-tip`.

### J15 — the LATIN-ARM decision, measured on this plugin and not inherited

Carry-forward 1 predicted the override would move the Latin arm by **one element**, caused by the
`.octave-stretch-label` floor and not by the override, and told me to re-measure rather than take the
fallback. **The direction was right and the scope was bigger.**

`check-ui-labels` after the fonts landed with the floors as s71 left them read **`[7][fr]` 0 moved
and `[7][zh]` 187/198/215/215 moved** — so the fr arm never went red at all on this plugin, because
I re-measured the floors in the same pass rather than after a failure. The natural boxes were
measured with **all three floors REMOVED** (not zeroed — `min-width: 0` on a flex item lets it shrink
below its content and produced a nonsense 12.58px reading before I caught it):

| selector | s71's floor, measured under the SYSTEM face | **v1.9.0, measured under this stack** | verdict |
|---|---|---|---|
| `.tonic-label` | en 28.75 fr **41.59** | en 28.47 fr **42.06** zh 9.20 | **RE-MEASURED 41.59 → 42.06** |
| `.octave-stretch-label` | en 42.42 fr **51.84** | en 41.50 fr **53.86** zh 18.41 | **RE-MEASURED 51.84 → 53.86** |
| `.interval-list-header` | en **24** (2 lines) fr 12 | en **22** (2 lines of 11) fr 11 zh 14 | **RE-MEASURED 24 → 22** |

**All three of O-Bowed's v1.9.0 numbers are O-Contrabass's measured values from J14 to the
hundredth** — 42.06, 53.86 and 22 — and the octave-stretch figure is also O-Reed's. That is the
corroboration: three consumers rendering one face agree on one number, and the numbers differed
before only because the faces differed.

**DECISION: keep the override, re-measure the floors.** The plan's fallback (append a tail to a copy
of the system stack) was rejected on the same three grounds Task 4 recorded, and one more measured
here: the cause is a pin measured against one face and read as safety under another, which is R8 in
its exact stated shape; the fallback leaves the panel's Latin on the system UI face and leaves a
fourth consumer shape in a module that now has three; and the corrected numbers corroborate against
two independent plugins. Cost: **three numbers, on selectors R8 required re-reading anyway.**

**`modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` is BYTE-UNCHANGED**, asserted:
`git status --short -- modules/ scripts/` reads **0**, and neither commit touches `modules/`.

Final: **`undeclared-font: 0 finding(s)` against 95 Han nodes, 0 of which resolve without a named CJK
face — non-vacuous.** Nodes on the shared module system stack: **en 0, zh 0** (was 116 on en).
`grep -c 'PingFang SC'` = **3**. `svg-font-attr: 0 attribute carrier(s), 0 finding(s)` — inert on
this plugin, as carry-forward 16 said.

---

## 6. Geometry — two ratio blocks, three re-measured floors, 0 moved on all three arms

### A. The eleven-selector `1.11` RATIO block — transferred, and RE-DERIVED per selector

Verified against **O-Bowed's own** measured English content boxes, padding and border subtracted
FIRST and the line count checked (carry-forward 4):

| selector | fs | en box | pad+bd | lines | content/line | 1.11 × fs | Δ |
|---|---|---|---|---|---|---|---|
| `.interval-list-header` | 10 | 22.00 | 0 | **2** | **11.00** | 11.10 | 0.10/line |
| `.library-header-text` | 10 | 11.00 | 0 | 1 | 11.00 | 11.10 | 0.10 |
| `.generator-header-text` | 10 | 11.00 | 0 | 1 | 11.00 | 11.10 | 0.10 |
| `.gen-row label` | 10 | 11.00 | 0 | 1 | 11.00 | 11.10 | 0.10 |
| `.pitch-circle-label` | 9 | 10.00 | 0 | 1 | 10.00 | 9.99 | 0.01 |
| `.ref-knob-label` | 9 | 10.00 | 0 | 1 | 10.00 | 9.99 | 0.01 |
| `.tonic-label` | 9 | 10.00 | 0 | 1 | 10.00 | 9.99 | 0.01 |
| `.octave-stretch-label` | 9 | 10.00 | 0 | 1 | 10.00 | 9.99 | 0.01 |
| **`.viz-btn`** BUTTON | **9** | 24.00 | 14 | 1 | **10.00** | 9.99 | 0.01 |
| **`.tuning-file-btn`** BUTTON | 9 | 24.00 | 14 | 1 | **10.00** | 9.99 | 0.01 |
| **`.generator-btn`** BUTTON | 10 | 27.00 | 16 | 1 | **11.00** | 11.10 | 0.10 |

All eleven inside the 0.5px tolerance, so 1.11 transfers here too. **`.viz-btn` is 9px on O-Bowed —
the same as O-Reed and O-Contrabass, and not O-Wind's 10px.** Two values across four consumers; the
per-selector check has now earned itself three times. `.interval-list-header` is **two line boxes in
English** on a fourth consecutive consumer; a naive box-over-font-size would have read 2.2 and
doubled the pin.

### B. SIX page-local `line-height: normal` leaves, and the one that hides on the other axis

| selector | fs | en box | pad+bd | content | ratio | matches M8? |
|---|---|---|---|---|---|---|
| `.preset-save-btn` **BUTTON** | 12 | 24.00 | 10 | 14.00 | 1.1666667 | ✓ |
| `.tuning-btn` **BUTTON** | 11 | 22.00 | 10 | 12.00 | 1.0909091 | ✓ |
| `.section-label` ×5 | 10 | 15.00 | 4 | 11.00 | 1.1 | ✓ |
| `.settings-label` ×2 | 10 | 11.00 | 0 | 11.00 | 1.1 | ✓ |
| `.humanize-col-label` ×4 | 8 | 9.00 | 0 | 9.00 | 1.125 | ✓ |
| **`.impossible-label`** | 9 | **14.00 wide** | 4 | **10.00** | 1.1111111 | ✓ |

**`.impossible-label` is this task's own contribution to the mover census, and it hides on the axis
nobody looks at.** It is `writing-mode: vertical-rl`, so the line box is its **WIDTH**, not its
height: en 14.00 × 62.84 for ten uppercase letters, zh 17.00 × 29.41 for three Han glyphs. It got
SHORTER and WIDER, and only the width mattered — `.impossible-row` is `justify-content: center`, so
3px of extra block extent pushed each of the three knobs beside it 1.5px sideways. The residual
mover list after the other five pins was **21/24 entries, every one `dx=1.5 dy=0.0 dw=0.0 dh=0.0`,
with no `dw` or `dh` anywhere above them** — a mover list with no root cause in the `dh` column,
which reads like a mystery until the writing mode is taken into account. Same ratio, same
derivation, subtracting the 4px `padding-right` the vertical box carries on its inline edge.

### C. ONE root cause, 215 symptoms

Before the page-local pins, every `[7][zh-Hans]` entry with a non-zero `dw` or `dh` across all four
states was a **single** row:

```
div.main-content > div.left-panel > div.section-box:nth-child(1)   dx=0.0 dy=0.0 dw=0.0 dh=3.0
```

Everything else in the 187/198/215/215 lists was `dy` cascade below it. Task 1's carry-forward 3 and
Task 4's carry-forward 5, in their sharpest form: **read the `dh` column and find the one absorbing
container; do not treat the count as the work.** The container is the Bow section box and its first
child is `.section-label` at 10px, whose Han content line box ran 11 → 14.

### D. R8 fired on every selector BEFORE anything was appended

None of the 17 selectors touched carried a prior `line-height`. Pin census, comment-stripped:

| | min-width | min-height | line-height | letter-spacing | white-space |
|---|---|---|---|---|---|
| **before** | 6 | 5 | 5 | 13 | 4 |
| **after** | **6** | **5** | **11** | 13 | 4 |

**Exactly the J14 census on all five numbers before.** After: `min-width` and `min-height`
**unchanged**, which is the arithmetic proof that the three floors were REWRITTEN rather than added
and that no other pin moved; `line-height` +6 (one 11-selector rule block plus five page-local
declarations, with `.impossible-label` making the sixth); `letter-spacing` and `white-space`
untouched.

### E. Result

| screen | planning-time | table landed, no fonts | **final** |
|---|---|---|---|
| `undeclared-font` | 0 — **a vacuum** (J13) | **95** | **0** against 95 Han nodes |
| `line-height-normal` | 0 — vacuum | 38 | **4**, all evidenced NON-movers |
| `wrap-count` | 0 — vacuum | **1** | **1** — did NOT grow (see §7.5) |
| `svg-font-attr` | 0 carriers / 0 findings | 0 / 0 | **0 / 0** — inert |
| `check-ui-labels [7]` fr | 0 moved | 0 moved | **0 moved** |
| `check-ui-labels [7]` zh | — | 187 / 198 / 215 / 215 | **0 moved** |

**`identity: 1491 nodes / 497 distinct DOM keys / 139 distinct display ids`** — the display-id figure
matches the plan's live table (139) exactly.

### The four `line-height-normal` residuals — all evidenced NON-movers (N1)

| node | fs | enH | zhH | why no pin can change it |
|---|---|---|---|---|
| `#preset-prev` | 14 | 28.00 | 28.00 | renders `◀`; the Han is in its `aria-label` only, and its box is fixed |
| `#preset-next` | 14 | 28.00 | 28.00 | same |
| `button.viz-tab-btn.active` | 10 | 28.00 | 28.00 | `弓弦` inside a fixed-height tab strip |
| `button.viz-tab-btn` | 10 | 28.00 | 28.00 | `共鸣体频谱`, same |

`enH == zhH` on all four, measured. O-Bowed's residual count is **4**, against O-Wind's 21,
O-Contrabass's 9 and O-Reed's 1.

### The three floors, re-measured at the shipped state

| selector | en | fr | **zh** |
|---|---|---|---|
| `.tonic-label` | 42.05 × 9.98 | 42.05 × 9.98 | **42.05 × 9.98** |
| `.octave-stretch-label` | 53.86 × 9.98 | 53.86 × 9.98 | **53.86 × 9.98** |
| `.interval-list-header` | 122.00 × **22.19** | 122.00 × 22.00 | 122.00 × **22.00** |

Heights read **9.98 (= 9 × 1.11)** rather than the face's `normal` box, so the ratio block is live on
them. The header reads **22.19** on the English arm and 22.00 on the other two — the English is two
ratio line boxes (2 × 11.10) sitting just above the 22px floor, and the 0.19px difference is inside
the 0.5px tolerance, which `check-ui-labels` confirms by reading 0 moved. Chinese is the narrowest
arm on both width floors, so the fr-widest floor covers it — **the wave's fifth confirmation**.

### State-EFFECT assertions — the eval state AND both `click` states (wave 4e N1)

| state | probe | effect |
|---|---|---|
| `eval` sympatheticCount → 0 | n/a | `#sympAmount-ctrl` / `#sympDecay-ctrl` `display: none` **before and after** — the parameter's DEFAULT is already 0, so the eval is confirming the state rather than creating it |
| `#gear-btn` | `elementFromPoint` = **TARGET**, rect 860.0, 6.0, 28.0 × 28.0 | `#settings-popover` h **0 → 67.2** |
| `#tuning-toggle` | **TARGET**, rect 715.8, 9.0, 62.0 × 22.0 | `#tuning-container` h **0 → 418**, **module captions 0 → 20** |

No `null`, no different element, **no coverage hole**. `measure-ui --verbose` reports
`did not resolve|unresolved|skipped state` = **0** with `states default + 3` — all three applied,
including the **overlay** mount (this plugin is the wave's only overlay, not a tab).

**The z-index question was checked and is a non-issue on this page.** The markup comment records a
surface at `z-index: 200` above `.tuning-overlay` (50). Measured: clicking `#tuning-toggle` **closes**
the settings popover, so by the time the overlay's captions are measured the popover is not open —
`settings popover still open: false`, and **0 of 20 overlay captions are covered at their centre** by
anything that is not themselves or a descendant.

**Module captions 0 → 20, not carry-forward 14's 30 — and both numbers are right about different
things.** 37 module keys − s71 D5's 7 that never enter the DOM = 30 nodes MOUNTED; − the 10 that
`check-ui-labels` reports as never visible (nine `<option>` inside a collapsed `<select>`, plus
`label.tkHint`) = **20 nodes with a measurable box in a single snapshot**. 30 is the mounted figure
under a cumulative OR-ed-visibility walk; 20 is the figure a single `getBoundingClientRect` pass can
see. Use 20 for a snapshot probe and 30 for a `check-ui-labels`-style walk.

**The `10 never became visible` NOTE is unchanged at 10**, s71 D5's identified set exactly:
`#truekeys-view>div.tk-hint`, six `#library-filter>option` and three `#generator-type>option`. Nine
of ten are structurally unmeasurable. **`66 of 76` is not a fraction** and was not read as a failure.

---

## 7. FALSE PREDICTIONS — measured value beside the predicted one

### 7.1 Precondition (g) — `plugins/O-Strata/` does not appear in `git status` at all, and the verify's `grep -c "O-Strata"` reads 0

- **Predicted:** `git status --short` → only `?? plugins/O-Strata/`; and a downstream verify
  `git status --short | grep -c "O-Strata"  # 1, UNTRACKED and UNSTAGED`.
- **Measured:** `git status --short` shows **two** entries, neither of them O-Strata:
  ` M .claude/agent-memory/research-planning-agent.md` and
  `?? .planning/quick/260907-ja8-…/`. **The O-Strata count is 0, not 1.** O-Strata was committed at
  `4965c271` / `8dae0cc7` and its working tree is clean. Task 3 §7.1 and Task 4 §7.1 both reported
  the count reading 1 for the wrong reason; on this task it does not read 1 at all. The
  orchestrator's replacement precondition — no `git status` entry under `plugins/O-Bowed/` — was
  the one that fired, and it was clean.
- **The repo-wide lint still exits 2 for O-Strata's sake**, because the lint enumerates
  `plugins/*` from **disk**, not from the index. A directory being tracked and clean changes
  nothing about a missing `i18n.js`. The `?? plugins/O-Strata/` shorthand and the lint's
  `1 plugin(s) could not be read` were always two independent facts; this task is where they came
  apart.

### 7.2 Carry-forward 1 predicted ONE floor would need re-measuring; ALL THREE did

- **Predicted:** *"Check `.tonic-label` (41.59px) and `.interval-list-header` (24px) too, but
  **expect them to hold**. Both held on O-Reed."*
- **Measured:** all three moved. `.tonic-label` 41.59 → **42.06**, `.octave-stretch-label` 51.84 →
  **53.86**, `.interval-list-header` 24 → **22**.
- **Why O-Reed's held and O-Bowed's did not is a property of the measurement, not of the page.**
  A `min-width` floor that BINDS hides the natural box. O-Reed's `.tonic-label` reported 41.58 on
  all three arms because 41.59 was pinning it; whether the natural French box exceeded 41.59 by
  less than the 0.5px tolerance is not visible from a floored measurement. O-Bowed's natural
  French box is 42.06, which exceeds the floor by **0.47px** — inside the tolerance, so
  `check-ui-labels` would have stayed green with the stale floor in place and the fr arm rendering
  0.47px wider than the en arm. **The only way to see it is to remove the floor and measure.**
  I did; O-Reed's task did not have to, because its one visible mover led it to the same selector
  by a different route.
- **Consequence for Task 6 and for any later consumer:** a floor whose natural box is measured
  only while the floor is in place is not measured. Remove, measure, restore.

### 7.3 `min-width: 0` is not "no floor" — it un-pins a flex item below its content

- **What I did first:** set the two width floors to `min-width: 0` to take the natural box.
- **Measured:** `.tonic-label` read **12.58 × 10.00** on the English arm against a true natural box
  of 28.47, and the zh arm read **9.20 × 26.00** — one glyph wide and wrapped to two lines.
- **Cause:** the default `min-width` for a flex item is `auto`, which is its content size.
  `min-width: 0` explicitly **allows** shrinking below content. Removing the declaration entirely
  is the only way to measure a natural box in a flex row. Caught because 12.58 was implausible
  against s71's recorded 28.75; a number that had been merely wrong rather than absurd would have
  been taken.

### 7.4 The plan's `sweep('en')` line number is off by 104, and the form-6 line numbers are the comment-stripped ones

- **Predicted:** `sweep('en')` at raw **L339**; the five form-6 sites at raw L372, L397, L416-417,
  L561.
- **Measured:** `sweep('en')` at raw **L443**. Form-6 sites at raw **L476, L501, L520, L521, L665** —
  the plan's numbers are the comment-stripped ones, off by 104-105 throughout. The form-1 assertion
  (L227), the loader publish (L152), the destructure (L221), the dynamic read (L279), the switch
  (L511), the selector assertion (L514) and `sweep('fr')` (L515) were **all exact**.
- **Carry-forward 6 predicted exactly this shape** and located everything by pattern. The pattern
  in the plan's own text — that comment-stripped and raw numbers are interleaved without being
  labelled — is now three-for-three across O-Reed and O-Bowed.

### 7.5 `wrap-count` did NOT grow — it stayed at 1, and the ESTIMATED denominator fell instead

- **Predicted (carry-forward 15):** *"`wrap-count` can grow without a pixel changing … a ratio pin
  moves nodes out of the estimated set and changes the reported line count."*
- **Measured:** `wrap-count` reads **1 before the pins and 1 after**. What changed is the
  denominator disclosure: `110 of them ESTIMATED at fsn * 1.2` before, **`76 of them ESTIMATED`**
  after. Thirty-four nodes left the estimated set and none of them crossed a line-count boundary.
- **The single entry is the same one both times**: `span.impossible-label` `"Impossible"` `en=6
  line(s), zh-Hans=3 line(s)` — an artefact of `writing-mode: vertical-rl`, where the screen's
  height-over-line-box arithmetic counts glyphs rather than lines. It is not a wrap and no pin
  removes it. The warning was right in mechanism and the direction did not fire here.

### 7.6 The product-name control reads 1, not 0, and carry-forward 13's reason is why

- **Predicted (carry-forward 13):** *"The product-name control returns 0 whenever the wordmark is an
  `I18N_EXEMPT` entry. Three for three in this wave. Predict 0."*
- **Measured:** **1 on the emitted batch and 1 on the returned English.**
- **Adjudicated, not suppressed.** The prediction's reasoning is correct and covers the route it
  describes: O-Bowed's `.plugin-name` wordmark IS an `I18N_EXEMPT` entry and never enters the batch.
  What it does not cover is a **second route** — an exempt token quoted inside a BODY.
  `tip.hairStiff` reads *"the tone matches the classic **O-Bowed** voice"*, and W3 direction 1
  requires that token to survive verbatim into the Chinese, so it is in the batch by design.
  The blinding cost is nil: the batch already contains 弓速, 弓压, 弓位 and 松香, so the reader
  knows it is reading a bowed-string instrument long before it reaches the product name.
  The reader returned it verbatim, which is direction 1 confirmed from the other side.

### 7.7 `claude-opus-4-1` is silently remapped — confirmed a second time

stderr: `⚠ claude-opus-4-1 is automatically remapped to Opus 5 (the latest Opus).` R4's "different
model" is still satisfied — round 1 was Sonnet 4.5 and round 2 was Opus 5, genuinely different
families — but the model named in this summary is the one stderr disclosed, not the one requested.
Carry-forward 12, second confirmation.

### 7.8 The state-EFFECT module-caption figure is 20, not 30

Carry-forward 14's 30 is the MOUNTED count under a cumulative OR-ed-visibility walk. A single
snapshot with `getBoundingClientRect().height > 0` reads **20**, which is 37 − 7 (never in the DOM)
− 10 (never visible). Both are correct; they answer different questions. Recorded so the next task
does not read 20 as a regression against 30. See §6.

### 7.9 Everything the plan predicted that HELD

- The **ff census reproduced to the node**: 281 / 116 / 30 against 427 visible en nodes (J13).
- **10 `font-family` declarations, 9 of them `inherit`, 30 form-4 nodes** (J11).
- The **pin census before**: 6 / 5 / 5 / 13 / 4, all five (J14).
- **`identity` 139 distinct display ids** (live table).
- **48 I18N + 82 LABELS**, `[12]` **2 module(s)**, `66 of 76` coverage, **10** never visible.
- **The form-1 assertion at raw L227**, the loader at L152, the destructure at L221.
- **The `.tuning-panel` override is the fix and the Latin arm holds** (carry-forward 1's direction).
- **1.11 transfers**, and `.viz-btn` is 9px (carry-forward 4's warning earned itself again).
- **Bare Arial 30 → 17**, **11 Han-bearing nodes named** where the rule covers 13 (carry-forward 3,
  to the number).
- **`svg-font-attr` inert** (carry-forward 16).
- **`--forward-provenance` takes a VALUE** and the ingest echoed it back (carry-forward 11).
- **`serve()` returns `{server, port, close}` with no `url`**; **`grep -B14`** for the ratio block;
  **the corrected duplicate-key scan**; **promotion script written to a FILE** (carry-forward 17,
  all four).
- **The corpus figure**: 4460 + 161 = **4621**, recomputed live and equal to the plan's target
  (carry-forward 20).
- **No `Flutter` caption on O-Bowed** (carry-forward 18) — and the R3 screen was run before
  authoring, not after, which is what caught the real collision.
- **An X+量 compound whose glossary root parses two ways** (carry-forward 19) — see §8. The
  prediction was right about the mechanism and wrong only about which side of the page it would
  land on.

---

## 8. The blind reverse read

Emitted **`--emit O-Bowed --plugin O-Bowed`** (BOTH — W4) **twice**, so each chunk carries its own
salt. 124 non-module rows split **62 + 62**, proved disjoint on the **real** ids (0 overlap, union
exactly 124), proved to contain **no module row** (0 in either chunk), and the two emits share **0**
blinded ids across all 161 rows. Dispatched
`claude -p --model claude-sonnet-4-5 --allowed-tools ""` from `/tmp/ja8t5/blindrun` — outside the
repo — with file, web and repository access forbidden in the prompt.

**34 concepts were deliberately SPLIT across the two chunks** — a caption in one, its own tooltip
title or body in the other.

### Controls — sixteen, every one fired

| control | result |
|---|---|
| line count matches sent | **62/62 and 62/62** |
| ids identical **AND IN ORDER** (M12) | **CLEAN on both chunks**, fired on the ORIGINAL ids before any remap |
| ids as a set | identical, both chunks |
| ids pure 12-hex | 124/124 |
| ids unique | 124/124 |
| no key fragment in any id | 124/124 |
| rows well-formed (exactly 2 tab fields) | 124/124 |
| Han surviving in the returned English | **0**, positive control fired on the same probe (**123** Han lines SENT; the 124th is `label.vizSchelleng`, `sameAsEn`) |
| sha256 of the zh column vs the committed tree | **MATCH** — `49266ea6…91ce0247`, using `LC_ALL=C sort` |
| working tree == HEAD at read time | **YES** — the pass ran against `089e65e6`, `git status` 0 |
| module rows excluded from the batch | **37 excluded, 0 present in either chunk** |
| refusal 1 — missing `--forward-provenance` at emit | **FIRED** — *"The batch was emitted without --forward-provenance, so there is nothing to compare the reverse pass against"* |
| refusal 2 — wrong-side `--manifest` | **FIRED** — *"WRONG MANIFEST FOR THIS BATCH: all 3 returned ids are unjoinable, not one."* |
| refusal 2 fired against a manifest that HAS real forward provenance | **YES** — it echoed `"…(chunk B salt)"` and refused anyway, so it cannot have fired for refusal 1's reason |
| product-name control, emitted batch | **1** — adjudicated, §7.6 |
| product-name control, returned English | **1** — the same row, returned verbatim |

Both refusals were run **with `--provenance` supplied**. The ingest's own echo line
`forward pass recorded at emit: "claude-opus-5 forward draft, wave 4f task 5, 2026-09-07"` confirms
carry-forward 11's identity control is **not vacuous** — the value landed in the manifest, not
`"--out"`.

### All 124 triples read with `--verbose` (R2)

**41 round-tripped word for word.** Every one of the remaining **83** was read and adjudicated on
**collision on the page**, not drift distance.

### ONE ROW RE-AUTHORED — and it is exactly what the read exists to find

**`label.count` "Count" → 数量 → the reader returned "Amount". `label.amount` "Amount" → 量 → the
reader returned "Amount". They are ADJACENT KNOBS in the same section.**

数量 is the glossary root for `count` and 量 is the glossary root for `amount`, so both sides were on
their roots and the lint was silent. The collision is structural: **量 is a substring of 数量**, and
数量 standing alone also just reads as "quantity". Both captions sit in the Sympathetic section box,
one cell apart, with 衰减 ("Decay") beside them.

The body made it unambiguous. `tip.count` came back as *"this is the knob labeled as **amount** …
At 0, the entire section is turned off, and its **amount** and decay knobs are both hidden"* — the
sentence names the wrong control twice, and the second naming makes the first one incoherent.
`tip.amount` and `tip.decay` both cross-reference the same caption and both came back saying
*"when the **amount** is greater than 0"* and *"when the **quantity** is greater than 0"*.

**Re-authored to 弦数** (the count of strings), with a `termNote` recording why the glossary root was
left. 弦 is the classifier-bearing head for a string count, it is what the knob actually sets, and it
cannot be read as a level. `根数` was rejected: 根 also means "root", and a tuning plugin with a
泛音 vocabulary two panels away is exactly where "number of roots" is a live reading.

The title moved with it — `tip.count` 共鸣弦数量 → **共鸣弦数** — and so did the three body
cross-references. **The other side was deliberately left at its root**, and the termNote says why:
量 is the vaguer word but it is the one whose sense the section header 共鸣弦 already fixes; moving
the unambiguous side would have been R3's error of qualifying the wrong half.

### Correction round 2 — fresh agent, fresh salt, DIFFERENT MODEL (R4)

Emitted afresh (**0 blinded ids shared with either round-1 emit**, salt distinct from both) and
dispatched to Opus 5 (§7.7). The chunk carried the corrected rows plus **decoys** so the reader could
not tell which was under test:

| row | zh | en' (round 2) | en' (round 1) |
|---|---|---|---|
| **`label.count`** | **弦数** | **"Number of strings"** | *"Amount"* |
| **`tip.count` title** | **共鸣弦数** | **"Number of sympathetic strings"** | *"Sympathetic string count"* |
| **`tip.count` body** | … 标注为弦数的旋钮 … 它的量旋钮与衰减旋钮 … | *"…this is the knob labeled **number of strings**. At 0 the whole section is off, and its **amount knob and decay knob** are both hidden."* | *"…labeled as amount … its amount and decay knobs…"* |
| `label.amount` (decoy) | 量 | "Amount" | "Amount" |
| `label.amt` (decoy) | 量 | "Amount" | "Amount" |
| `label.decay` (decoy) | 衰减 | "Decay" | "Decay" |
| `tip.amount` (decoy) | 共鸣弦量 | "Sympathetic string amount" | "Resonant String Amount" |
| `tip.bodyAmt` (decoy) | 共鸣体量 | "Resonating body amount" | "Resonator body amount" |

**The correction holds and the round is not vacuous.** The count sense now attaches to 弦数 and only
to it; the amount sense stays on 量 where it belongs; and the body's two cross-references name the
right knobs. **The decoys returned as they had in round 1**, which is what makes the round evidence
rather than ceremony.

**Round 2 corrected nothing further, so there is no round three** — R4's stated condition, said here
and said in the commit.

### R3 — before and after

- **Before authoring:** 130 rows screened, **86 glossary-covered**, **9 root collision groups**, all
  nine same-control caption/title pairs or same-English pairs (周期 (C) on two module rows with
  identical English, one of which — `label.genR2Period` — is one of s71 D5's seven keys that never
  enter the DOM; 松香 on three rows with identical English). **0 distinct-control collisions at
  screen time — and the real one was invisible to the screen**, because `Count` and `Amount` are
  different roots and R3's screen only fires when two keys share ONE root. **This is M13's shape
  exactly, and only the reverse read saw it.**
- **After authoring, mechanically:** **16 shared-rendering groups**, **8 carrying different
  English**, **7 of the 8 proved SAME-CONTROL** (`hairStiff`, `infSustain`, `revFriction`, `subHarm`,
  `bodyAmt`, `gauge`, `refPitch` — each a caption and its own tooltip title, differing only by
  abbreviation). **1 DISTINCT-control group remains and is ACCEPTED**: 量 for both `label.amt` "Amt"
  and `label.amount` "Amount". The English page collides in the same place and in the same way —
  both are the word "Amount", one abbreviated for the Humanize grid — and both are disambiguated by
  their section header (人性化 / 共鸣弦) identically in all three languages.

### Accepted with reasons — the classes worth naming

**The ABBREVIATION class**, for the fifth time in this wave. `Hair Stiff.`, `Inf. Sustain`,
`Rev. Friction`, `Sub Harm.`, `Body Amt`, `Ref Pitch` all returned unabbreviated ("Bow Hair
Stiffness", "Infinite Sustain", "Reverse friction", "Subharmonic", "Resonator Amount", "Reference
Pitch"). English abbreviates against this page's 64px `.knob-label` cap at 8px — the file's own
v1.6.1 header records every rejected French candidate and its measured width; Chinese does not need
to, and its longest caption here is four glyphs.

**`共鸣体` → "Resonator" for "Body" — ACCEPTED.** 共鸣体 is the glossary root and the page carries no
other resonator: `label.string` 弦 is the string section and `label.bodyAmt` 共鸣体量 is this
control's own blend knob. The French made the same move for the same reason ("Caisse", the soundbox
of a bowed instrument, not "corps").

**`静止` → "Stationary" for "Silent" — ACCEPTED, and the alternative is the reason.** The canvas paints
two string states, 发声中 ("Sounding") and 静止 ("at rest"). The obvious literal 静音 is the standard
Chinese for **MUTE**, and shipping it would put a transport-control word on a physics readout. The
French reached the same place independently ("Au repos").

**`弦径` → "String diameter" for both `label.gauge` "Gauge" and `tip.gauge` "String Gauge" —
ACCEPTED.** 弦径 is the glossary root; the pair is a caption and its own title, and the section
header 弦 above it is exactly the English "String" / "Gauge" relationship.

**`P_max（粗糙）` → "P_max (rough)" for "P_max (raucous)" — ACCEPTED on the discriminator, and the
drift recorded.** The Schelleng axes carry exactly two regime labels, 粗糙 and 打滑 ("slipping"), and
no other node on the page names either; the reader recovered both as distinct regimes. The drift from
"raucous" to "rough" is real and is not a collision, which is what the discriminator asks.

**`亥姆霍兹` for "Helmholtz" but `Schelleng` left in Latin — ACCEPTED, and the asymmetry is
deliberate.** 亥姆霍兹 is the settled Chinese rendering used throughout Chinese acoustics writing and
it round-tripped exactly. Schelleng has no settled rendering; inventing a transliteration would put a
string on the page that no Chinese reader could look up, so it is `sameAsEn: true`, which is also what
the French does.

**`速度人性化速率` for "Speed Humanize Rate" — ACCEPTED.** 速度 (speed) and 速率 (rate) are both
glossary roots and sit in one string; the reader returned "Speed humanization rate" and separately
recovered 速度 as "Speed" and 速率 as "Rate" from their own captions, which is the pair confirmed from
both sides. Four of these exist, one per Humanize column, and all four round-tripped.

**The bodies' column cross-references — `当速度列的量旋钮为 0 时` — ACCEPTED and confirmed.** The
English says "while Speed Amt is 0", which is a column header plus a knob caption. The reader returned
*"when the amount knob in the speed column is 0"* on all four, naming both halves and the grid
relationship between them.

### W3 / N8 checked BOTH ways

**Direction 1 — every exempt token in an English body survives verbatim into the Chinese:**
`O-Bowed` in `tip.hairStiff` — **present in both arms**, and returned verbatim by the reader.
One further probe hit is a **false positive, adjudicated**: `tip.language`'s English body contains
the word "English", which matches the `English` endonym exemption. That exemption covers the
`<option>` word in the markup; this is a language name in running prose, and a language name in prose
IS translated. The Chinese reads 英文. Recorded rather than "fixed".

**Direction 2 — every non-exempt caption named in a body is named by its LOCALIZED caption:**
Rate → 速率 (×4), Speed → 速度, Pressure → 压力, Position → 位置, Rosin → 松香 (×2), Count → 弦数
(×3), Amount → 量, Decay → 衰减. **14 of 14 present**, and the readers recovered every one as the
right control from the other side.

---

## 9. Gate results — every actual output line

```
check-i18n --plugin O-Bowed                     exit 0
  PASS: [1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr","zh-Hans"]
  PASS: [1] the table carries copy — 48 I18N + 82 LABELS          [UNCHANGED from 48 + 82]
  PASS: [1] every I18N key has an entry in every declared language (en, fr, zh-Hans)
  PASS: [1] every language entry has string t and b
  PASS: [1] every LABELS key has a string t in every declared language (en, fr, zh-Hans)
  PASS: [12] there is shipped page JS to scan — 2 module(s): the inline <script type="module">
             in index.html, modules/tuning/scala-tuning-engine/js/tuning-panel.js

i18n-zh-lint --plugin O-Bowed                   exit 0
  O-Bowed   161 rows   130 zh   Z1 · Z2 · Z3 · Z4 · Z5 · Z6 · Z7 · F1 · R1 · Z8 ·   total 0
  BELOW SHIP BAR — entries at reviewed:'mt' (machine draft, unchecked): 0
  GATE PASSED — exit 0. 0 findings across 1 plugin(s).

i18n-zh-lint (repo-wide)                        exit 2  — THE J1 BASELINE, UNCHANGED
  GATE FAILED — exit 2. 0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read.

i18n-fr-lint (repo-wide)
  plugins with findings: 0 / 44   (1 could not be read)

check-ui-labels --plugin O-Bowed                exit 0
  8 of 8 [7][GEOMETRY DIFF][fr|zh-Hans] "no non-label element moved"  — 4 states x 2 arms
  66 of 76 [data-i18n] elements were VISIBLE in at least one state
  10 never became visible and were therefore NEVER MEASURED
  == ALL CHECKS PASSED ==

plugins/O-Bowed/tests/ui_tip_render_check.js    exit 0
  PASS: LANGUAGES derives from the table as a non-empty array opening with the source
        language — got ["en","fr","zh-Hans"]
  PASS: the derived list carries at least one language besides the source language
  sweeping 3 language(s): en -> fr -> zh-Hans
  659 PASS assertions   (was 470 at two languages)
  == ALL CHECKS PASSED ==

boot-all-uis --plugin O-Bowed --strict-tips     exit 0
  clean:  1 / 1     DEAD bindings: 0 across 0 plugin(s)     late bindings: 0 across 0 plugin(s)

measure-ui --plugin O-Bowed --mode box --report all
  identity: 1491 nodes / 497 distinct DOM keys / 139 distinct display ids — keyed by DOM path
  undeclared-font:     0 finding(s)        [against 95 Han nodes — non-vacuous]
  line-height-normal:  4 finding(s)        [all four evidenced NON-movers, enH == zhH]
  wrap-count:          1 finding(s)        [the vertical-rl artefact, unchanged]
  svg-font-attr:       0 attribute carrier(s), 0 finding(s)

measure-ui --verbose
  measure-ui: languages en, fr, zh-Hans   states default + 3
  did not resolve | unresolved | skipped state:  0

CMakeLists two-arm reader                        1.9.0   (a QUOTED literal; no set() form exists)
git status --short -- modules/ scripts/          0
git tag --points-at HEAD                         0

./scripts/build-and-install.sh O-Bowed           exit 0, 50s
  VST3: ~/Library/Audio/Plug-Ins/VST3/O-Bowed-dev.vst3            5.7M, age 0s
  AU:   ~/Library/Audio/Plug-Ins/Components/O-Bowed-dev.component 5.6M, age 0s
  No "⚠ Sweeping ALTERNATE-variant" warning — no unsuffixed orphan on disk.

i18n-zh-backtranslate (corpus, recomputed LIVE)
  TOTAL   5441 rows   4621 zh   mt 0   bt 4621   native 0
  zh-Hans strings: 4621 of 5441 rows        37 localized plugins
```

**Corpus: 4460 → 4621 of 5441; 36 → 37 localized plugins.** 4621 is the plan's stated wave target
exactly, which confirms Task 4's carry-forward 20 and closes the arithmetic discrepancy Task 3
introduced.

---

## 10. Carry-forwards for Task 6 (batch close)

1. **`PLUGINS.md` is NOT touched by this task.** Its O-Bowed row reads **1.8.0** and must go to
   **1.9.0** (one minor). The other four: O-GrainScatter **2.6.1 → 2.8.0** (two minors),
   O-Wind → **1.21.0**, O-Contrabass → **1.10.0**, O-Reed → **1.6.0**. **One commit for all five.**
   Then the duplicate-row check:
   `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` — empty output is clean.
2. **auval — NOT RUN**, deferred by design. O-Bowed's triple is **`aumu OBwd OuDv`**, three
   UNQUOTED words (M14). Both bundles installed as `-dev` only; **no alternate-variant orphan** on
   disk, and `build-and-install.sh` emitted no sweep warning.
3. **O-Bowed will report a `Bow Position` WARNING and it PREDATES this wave.** s71's D11 measured it:
   `Parameter ID:127996179 "Bow Position"` (Generic, min 0.02, default 0.12, max 0.30),
   *"retrievedValue = 1.000000 … Parameter did not retain maximum value when set"* — the AU boundary
   normalizes 0-1 while the declared range is 0.02-0.30. **This wave touched no parameter, range,
   type, state format or audio path**, and the CHANGELOG's scope sentence says so explicitly.
   Do not read it as a new defect and do not attribute it to v1.9.0.
4. **The installed-family table needs no re-derivation for this task's purposes** — J12's figures
   were used as given and every stack this task wrote names Georgia and/or Times New Roman for its
   Latin half and PingFang SC / Microsoft YaHei for its CJK half. If Task 6 re-derives it,
   `system_profiler` proves presence and never absence (Task 1's carry-forward), so a zero is a
   claim about the query, not about the machine.
5. **N12 verdict, for the wave-level census.** O-Bowed carried **form 1** (1 site, raw L227),
   **form 6** (5 sites, all LEFT and classified) and **form 7** (2 sites). Forms 3 and 5 absent.
   After the repair the g5l inventory grep **no longer sees this file**, and the comment does not put
   it back (C8). Across the wave the grep's false-negative class is now proved on two shapes
   (a three-element literal, and per-language property access) — **say this as a correction to the
   inventory METHOD, not only to the count.**
6. **False-prediction ledger from this task** (§7): 8 entries, of which three are new to the wave —
   `min-width: 0` is not "no floor" (§7.3), `wrap-count` did **not** grow (§7.5), and the
   product-name control has a **second route into the batch** through an exempt token quoted in a
   body (§7.6). §7.2 is the one a later consumer will hit again: **a floor measured while the floor
   is in place is not measured.**
7. **`deferred-items.md` entries owed by this task** — see §11.
8. **The three re-measured floors are now consistent across three consumers.** O-Bowed, O-Reed and
   O-Contrabass all render the tuning overlay through a Georgia-family stack and all three now agree
   on `.octave-stretch-label` 53.86px; O-Bowed and O-Contrabass agree on `.tonic-label` 42.06px and
   `.interval-list-header` 22px. **O-Reed's `.tonic-label` (41.59px) and `.interval-list-header`
   (24px) were NOT re-measured with the floors removed** — see §7.2. Both are almost certainly
   0.47px and 2px stale in the same direction, both inside `check-ui-labels`' tolerance, and both
   harmless today. Worth a follow-up, not a blocker.

---

## 11. Deferred / out of scope

- **The two stale floors on O-Reed** (carry-forward 8 above). A ≤0.5px fr-arm discrepancy that no
  gate can see. Not fixed here: it is another plugin's tree and Task 4's commits are closed.
- **`plugins/O-Strata/` still makes both repo-wide lints exit 2**, now for a reason that is no longer
  visible in `git status` (§7.1). The lints enumerate `plugins/*` from disk. Closing it means either
  an `i18n.js` in O-Strata or an unreadable-plugin allowance in the lints; neither belongs in a
  localization wave.
- **The glossary is NOT edited.** O-Bowed adds **one** termNote (`label.count` → 弦数) and it is a
  per-page collision note, not a wrong root — 数量 is correct for `count` everywhere it does not sit
  beside 量. Task 1's `'dist lpf': ['失真低通']` → `['距离低通']` at
  `scripts/i18n-zh-glossary.js:433` remains open as a corpus-level item.
- **The stale-body probe as the plan spells it is byte-blind to French curly apostrophes.** Fixed
  here by running it with `perl -CSD` and grepping the French clause separately; the probe text lives
  in a plan, not a script, so there is nothing to commit. Owned by whoever next writes one.
- **The 17-of-37 module coverage hole is NOT closed** (s71 D5): 10 never-visible + 7 that never enter
  the DOM. Closing it needs states that change the generator type and select the Rotation
  visualisation; this wave does not budget them.
- **`reviewed: 'native'` OPEN** on all 161 rows — no native Chinese reader exists on this project.
  Disclosed in the CHANGELOG and printed by lint rule R1 on every run.
- **`scripts/` and `modules/` are byte-unchanged**, asserted at both commits. No shared-script change
  was required.

## Self-Check: PASSED

- `plugins/O-Bowed/Resources/ui/js/i18n.js` FOUND
- `plugins/O-Bowed/Resources/ui/index.html` FOUND
- `plugins/O-Bowed/Source/PluginProcessor.h` FOUND
- `plugins/O-Bowed/tests/ui_tip_render_check.js` FOUND
- `plugins/O-Bowed/CMakeLists.txt` FOUND, reads `VERSION "1.9.0"`
- `plugins/O-Bowed/CHANGELOG.md` FOUND, `## [1.9.0] - 2026-09-07` present
- commit `089e65e6` FOUND in `git log --all`
- commit `f25090c8` FOUND in `git log --all`
- `~/Library/Audio/Plug-Ins/VST3/O-Bowed-dev.vst3` FOUND
- `~/Library/Audio/Plug-Ins/Components/O-Bowed-dev.component` FOUND
