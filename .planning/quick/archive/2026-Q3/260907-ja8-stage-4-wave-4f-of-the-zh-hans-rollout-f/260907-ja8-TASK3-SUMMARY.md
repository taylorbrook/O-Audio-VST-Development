---
phase: quick-260907-ja8
plan: 01
task: 3
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4f, O-Contrabass, scala-tuning-engine, svg-font-attribute, enumeration-form-6, s71-D8]
status: complete
requirements: [ZH4F-03, ZH4F-06, ZH4F-07, ZH4F-09, ZH4F-11]
dependency_graph:
  requires: [260907-ja8 Task 1 (O-GrainScatter, b534636b + a92b7980), 260907-ja8 Task 2 (O-Wind, 020409f8 + bff367f6), 260906-s71 (shared tuning panel localized, d848337a)]
  provides: [O-Contrabass three-language UI at reviewed:'bt', v1.10.0, built and installed]
  affects: [Task 4 (O-Reed), Task 5 (O-Bowed), Task 6 (PLUGINS.md + one cold auval sweep)]
tech_stack:
  added: []
  patterns: [derive-or-abort over a table-exported LANGUAGES list, CJK tail inside an SVG font-family presentation attribute, unitless line-height ratio derived per line and per size, min-width floor at the exact measured English box]
key_files:
  created: []
  modified:
    - plugins/O-Contrabass/Source/ui/public/js/i18n.js
    - plugins/O-Contrabass/Source/ui/public/index.html
    - plugins/O-Contrabass/Source/PluginProcessor.h
    - plugins/O-Contrabass/tests/ui_frontend_check.js
    - plugins/O-Contrabass/CMakeLists.txt
    - plugins/O-Contrabass/CHANGELOG.md
decisions:
  - "The 37 shared scala-tuning-engine rows were COPIED byte-for-byte from O-Bassoon at reviewed:'bt' and excluded from the blind batch — R1's one stated exception."
  - "gear-btn and lang-select were read in full and deliberately LEFT: this is the one plugin in the wave where the J8 and J9 deletion reflexes would both have been wrong."
  - "The CJK tail on the SVG plate line goes INSIDE the presentation attribute, because a CSS rule does not win against one."
  - "No row was re-authored after the blind read, so there is no correction round two."
metrics:
  duration: one session
  completed: 2026-09-07
actuals:
  tokens: 74000
  tasks: 1
  commits: 2
---

# Task 3 — O-Contrabass to Simplified Chinese (wave 4f, the gate no inventory has listed)

**Commits (path-scoped to `plugins/O-Contrabass`, on `main`):**

| | hash | what |
|---|---|---|
| 1 | **`5a216066`** | the `'mt'` table, the gate repair, the fonts and the geometry pins — 4 files, +586 / −170 |
| 2 | **`b69ca8ea`** | promotion to `reviewed: 'bt'`, version 1.10.0, CHANGELOG — 3 files, +172 / −104 |

HEAD at start: **`bff367f6`** (the plan's `d37cdcc0` was stale — Tasks 1 and 2 had landed).

---

## 1. Preconditions — all nine FIRED on the tree as found

| # | check | measured | plan predicted |
|---|---|---|---|
| a | `i18n-zh-lint --self-test` | **`SELF-TEST: 10/10`**, exit 0 | ✓ |
| b | `check-i18n --plugin O-Contrabass` | exit 0; `[1] … got ["en","fr"]`; **`46 I18N + 94 LABELS`**; `[12] 2 module(s)` naming the inline `<script type="module">` **and** `modules/tuning/scala-tuning-engine/js/tuning-panel.js`, and **NOT** `preset-manager.js` | ✓ verbatim — `CMAKE_MODULE_JS_SCOPE` intact (s71 D2) |
| c | `i18n-zh-lint --plugin O-Contrabass` | exit 0 | ✓ |
| d | `check-ui-labels --plugin O-Contrabass` | exit 0, `== ALL CHECKS PASSED ==`, **`74 of 82`**, **10 never-visible**, first entry `#truekeys-view>div.tk-hint:nth-child(1)` then six `#library-filter>option` and three `#generator-type>option` | ✓ verbatim, and s71 D5's identified set exactly |
| e | `plugins/O-Contrabass/tests/ui_frontend_check.js` | exit 0, **28 PASS**, `bridge surface is exactly 36 fns … got JS=36 C++=36` | ✓ (the plan gave no baseline PASS count) |
| f | s71 D8 state | `label.loadSclFile` at raw **L658**, `label.loadScl` at raw **L719**, markup at **L1289**, duplicate-key scan **NONE**, 140 entry keys | ✓ all four verbatim |
| g | git | `main`, `git tag --points-at HEAD` = 0, **no entry under `plugins/O-Contrabass/`** | **corrected precondition met** (see §7.1) |
| h | repo-wide `i18n-zh-lint` | **exit 2**, `0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read` | ✓ — J1's discriminator held verbatim |
| i | Tasks 1 and 2 committed and installed | `b534636b`, `a92b7980`, `020409f8`, `bff367f6` all present; O-GrainScatter **2.8.0** and O-Wind **1.21.0** on disk | ✓ |

**Two-arm CMake reader, run unconditionally (R9):** `VERSION 1.9.0` at raw **L16**, **UNQUOTED**.
One-arm reads `1.9.0`; two-arm reads `1.9.0`. **Both arms agree.** After the bump both read
`1.10.0`. This is the wave's second unquoted literal, as the plan corrected the task description
to say.

---

## 2. What shipped

**186 emitter rows** over **140 entries** — 46 `I18N` × (title + body) = 92, plus 94 `LABELS`.
Corpus **4292 → 4478** rows of 5441. O-Contrabass now reads `186 / 186 zh / 0 mt / 186 bt`.

- **149 rows authored here at `reviewed: 'mt'`** (103 entries), promoted only after the reverse
  read, in its own commit.
- **37 rows arrived at `reviewed: 'bt'` by byte-for-byte copy** from
  `plugins/O-Bassoon/Resources/ui/js/i18n.js` — R1's one stated exception — and were **excluded
  from the blind batch**, with the exclusion stated in the commit.

**The `LANGUAGES` flip and the module column landed in the SAME edit** (J6).

### The s71 D8 detector, fired before and after — it did not recur

| detector | before | after |
|---|---|---|
| `check-i18n` LABELS count | **94** | **94** |
| `check-i18n` I18N count | 46 | 46 |
| duplicate-key scan (corrected form) | **NONE**, 140 entry keys | **NONE**, 140 entry keys, 61 language sub-keys excluded |
| `label.loadSclFile` present | yes (L658) | yes, with its zh row |
| `label.loadScl` present | yes (L719) | yes, module zh row at `'bt'` |
| markup points at `label.loadSclFile` | L1289 | L1289 |

**All 37 module zh blocks verified byte-identical to O-Bassoon's** by brace-matched extraction and
string comparison, before and after the promotion. All 37 at `reviewed: 'bt'`.

### The 37-key list was DERIVED, not transcribed

`grep -rhoE 'data-i18n(-aria)?="[^"]+"' modules/tuning/scala-tuning-engine/` returns **38**;
dropping the `label.xxx` inside a doc comment gives **37**. Present **37/37** in both O-Contrabass
and O-Bassoon.

### Zero termNote exemptions authored — the first plugin in the wave with that property

The scoped lint passed on the **first** authoring pass: `0 findings`, and the only `termNote`
exemption it reports is `label.genDivisions`, inherited verbatim with its module row. Task 1
needed one (`dist lpf`), Task 2 needed three. **O-Contrabass adds no glossary defect and needed
no per-page note of its own.**

---

## 3. Stale copy — corrected FIRST, file re-read, then authored (C7)

The comment-stripped probe reported **2** hits, which is **exactly the plan's prediction** (unlike
O-Wind, where the same probe over-counted 7 for 5). Both were the same clause in the two languages.

**`lang-select` (J10) — the fixed-pair enumeration, DELETED in en AND fr.**

- en: *"…and of this hover help. **English and French are available;** value readouts, note names and preset names stay in English."*
- fr: *"…de ces infobulles. **L'anglais et le français sont disponibles ;** les valeurs affichées…"*

The exception list beside it — value readouts, note names and preset names stay English — is true
and stays. The comment above the entry said *"in both languages"* and was re-worded as prose
(C8), with a second paragraph recording **why** the clause went: a sentence that enumerates what
the selector holds is false the day the selector grows. After correction the probe reads **0**,
comment-stripped and raw.

### Two CHECKED NON-DEFECTS, read in full and deliberately LEFT

**`gear-btn` (the J9 site) — already TRUE.** Verbatim: *"Choose the language of the interface,
**and turn the hover help on or off**. Both choices are remembered with the session."* It names
**both** of the settings panel's controls, where O-GrainScatter, O-Wind, O-Reed and O-Bowed all
falsely claim the panel holds the language and nothing else. Unchanged; the probe that guards it
confirms it still names both controls and now carries a zh row.

**`lang-select` makes no claim about the Tuning tab (the J8 axis) — absence confirmed.** Its
exception list names *value readouts, note names and preset names* and stops there. It is
therefore not a J8 site, and the interesting fact — recorded because a future reader will want to
know it was measured rather than assumed — is that **the one consumer whose body never made the
claim is the one that needed no s71-era correction.**

**This is the one plugin in the wave where both deletion reflexes would have been wrong.**

---

## 4. The gate — form 6, on the file no inventory has ever listed

`tests/ui_frontend_check.js`, 345 lines, is **not** a Playwright renderer: it evaluates
`js/i18n.js` in a `vm` sandbox and asserts over the parsed table plus the markup. It holds **no
list, no walk and no `en,fr` string**, so the g5l inventory grep that has driven this rollout since
Stage 3 returns nothing for it — and it was nevertheless a two-language gate.

### N12 enumeration census — O-Contrabass, before → after

| form | before | after | disposition |
|---|---|---|---|
| 1 — `LANGUAGES.join(',')` assertion | 0 | 0 | absent |
| 2 — string `'en,fr'` | 0 | 0 | absent |
| 3 — array-literal walk | 0 | 0 | absent |
| **4 — computed / UA-face form controls** | **30 nodes** | **0** | a CSS form, see §5 |
| 5 — `Map` read with a literal key | 0 | 0 | absent |
| **6 — per-language OBJECT PROPERTY access, ENUMERATING** | **3** | **0** | generalized over the derived list |
| 7 — paired literal-argument call | 0 | 0 | absent |
| **CANON-scoped `'en'` literal** | **1** | **1** | `indexOf("let uiLanguage = 'en';")` — LEFT, classified at the site |
| **prose "2 language" fragment** | **1** | **1** | counts a native-fn PAIR — LEFT, classified at the site |
| `LANGUAGES` mentions in CODE | **0** | **13** | publish, destructure, `EN`, shape assertion, abort, `REST`, walk guard, two loops, three messages, log |
| `LANGUAGES` anywhere incl. comments | 1 (a prose comment at raw L172) | — | — |
| g5l inventory grep sees this file? | **no** | **no** | it never did — that is the J2 finding; C8 kept it out |
| assertions | **28** | **30** | |

### The two ENUMERATING sites, and what they would have certified

```
raw L252-253   else if (!e.en || !e.en.t) …  else if (!e.fr || !e.fr.t) …
raw L256       check(bad.length === 0, `all … resolve in LABELS or I18N with both languages`)
raw L270-271   const missingFr = [...bound].filter(k => !I18N[k] || !I18N[k].fr
                                    || !I18N[k].fr.t || !I18N[k].fr.b);
```

Both feed real `check()` calls. Left alone after the flip, this gate would have asserted that every
markup key resolves in English and French and said **nothing at all** about Chinese — a page whose
Chinese column was half missing would have passed green.

### The three repairs

1. **`LANGUAGES` added to the publish.** The loader at raw **L229** published
   `{ I18N, LABELS, TIP_BINDINGS, I18N_EXEMPT }` and not `LANGUAGES` — wave 4e's Q2 shape exactly:
   the sandbox open, the table parsed, one identifier missing. Added to the object literal and to
   the destructure.
2. **Shape assertion plus a SECOND, INDEPENDENT guard on the walk.** `LANGUAGES` must be a
   non-empty array opening with the source language, and `REST = LANGUAGES.slice(1)` must carry at
   least one member. Each refuses with its own message and `process.exit(1)`. **No fallback
   literal.** Live: `checking 3 language(s): en -> fr -> zh-Hans`.
3. **Both property loops iterate the derived list.** The key-resolution loop now walks
   `LANGUAGES` and names the failing language in its message; the tooltip loop asserts a title
   **and** a body in every declared language. The two-language message string at raw L256 was
   re-worded as prose (C8).

### Derive-or-abort control — fired TWICE, against the LIST and against the WALK

| plant | expected | observed |
|---|---|---|
| `LANGUAGES = []` | the shape assertion refuses | `FAIL: LANGUAGES derives from the table as a non-empty array opening with the source language — got []` → `REFUSING to check a guessed language list.` **exit 1** |
| `LANGUAGES = ['en']` | shape PASSES, the walk guard refuses | `PASS: … got ["en"]` then `FAIL: the derived list carries at least one language besides the source language — got ["en"]` → `REFUSING to compare the page to itself.` **exit 1** |

The second is the load-bearing one: it proves the guard on the walk is **independent** of the guard
on the list, and it is the plant that catches a vacuous sweep. Both plants reverted by **targeted
edit, never `git checkout --`**; sha256 `15cf56ad…9015e4eb` byte-identical on both sides.

### The two language-shaped sites LEFT, each classified at its own site

- **The `2 language` fragment (raw L132)** is part of `bridge surface is exactly 36 fns (2 mockup +
  10 preset + 20 tuning + 2 hover-help + 2 language)`. It counts the
  **`getUiLanguage`/`setUiLanguage` FUNCTION PAIR** on the native bridge, not the number of
  languages the page offers. Adding a language to the TABLE adds no native fn, so the figure is
  invariant under localization and the sentence stays true. **The count still reads exactly 36
  (JS=36 C++=36) after the work.** A prose comment now names it as the wrong kind of two, so the
  next reader does not have to re-derive it.
- **`indexOf("let uiLanguage = 'en';")`** is CANON-scoped and positional: it asserts the canon
  block sits ABOVE the eager `bind*` calls, because with it below them `uiLanguage` is in its
  temporal dead zone and the call throws a `ReferenceError` that takes the whole UI down. The
  literal is the module's boot-time default, which is the source language **by construction**;
  deriving it would assert nothing about where the declaration sits.

**The gate ran GREEN AT TWO LANGUAGES before the flip**, so no commit in this task's history
contains a table whose third language makes its own gate red.

---

## 5. Fonts — 30 form-4 nodes, a panel body already safe, and the rollout's first SVG attribute

**12 declaration sites: 2 named stacks + 9 `font-family: inherit` + 1 SVG presentation
attribute.** The plan's live table said *"12 + 1 SVG attribute"*; the measured CSS count is
**11**, not 12 (see §7.4). The **9 `inherit`** matches J11 exactly.

The computed `ff` census over **505** visible English nodes reproduced J13's four populations
**exactly**:

```
473  Garamond, Georgia, "Times New Roman", serif     the house stack — Latin-safe
 30  Arial                                            <-- form 4
  1  Times                                            the html element, renders nothing
  1  Georgia, serif                                   <-- the SVG attribute carrier
```

Once the table landed, **113 visible zh nodes carried Han and every one of them resolved without a
named CJK face**: 101 on the house stack, 11 on bare Arial, 1 on the SVG carrier.

### Treatment, per site

| site | treatment | why |
|---|---|---|
| the house stack (raw L81) | **tail only** | Georgia and Times New Roman both installed (4 family matches each); Garamond installed **nowhere** on this machine |
| `.tuning-panel` override (raw L903) | **tail only** | this override is what keeps the 116-node panel body off the shared module's `-apple-system, BlinkMacSystemFont, sans-serif` at `tuning-panel.css:66` |
| new **five-selector** `#tuning-container` rule | **face + tail**, Latin first | `.viz-btn`, `.tuning-file-btn`, `.generator-btn`, `.generator-type-select`, `.library-filter-select` |
| the **SVG `font-family` ATTRIBUTE** (raw L951) | **tail INSIDE the attribute** | a CSS rule does not win against an inline presentation attribute (W1) |

**FIVE selectors, not three, and not O-Wind's four.** O-Contrabass's five `.viz-btn` carry no face
of their own — which is exactly why it reads 30 form-4 nodes where O-Wind reads 25. The 11
Han-bearing bare-Arial nodes measured after the table landed are precisely:

```
.viz-btn x5           圆周 极坐标 矩阵 真实键位 旋转
.tuning-file-btn x5   载入 .scl / 载入 .kbm / 保存 .scl / 保存 .kbm / 导出 HTML
.generator-btn x1     生成
```

**`undeclared-font` does not credit the two `<select>`s** — Task 2 §7.4 confirmed a second time.
Its `han` flag is computed over a node's own text plus `data-tip` / `data-tip-title` /
`aria-label`, and a collapsed `<select>`'s selected-option text is none of those. They are in the
rule anyway, on a decision about what the control **paints**: `#library-filter`'s six category
options and `#generator-type`'s three type options are all Han. **The rule covers 13 nodes where
the screen can only name 11.**

**The other 19 bare-Arial controls are the measured NEGATIVE case (N3):** twelve
`.interval-input` numerics, `#octave-stretch`, `#gen-divisions`, `#gen-period` and the two arrow
glyphs `#tonic-down` / `#tonic-up`. None renders Han, and the shared module gives none of them an
`aria-label` or a `data-tip`, so N3's tip-anchor discriminator answers **no** for every one.
`measure-ui` reports bare Arial **30 → 17**; the 13 that left are the 11 Han buttons plus the 2
selects.

### The SVG plate carrier (J16) — measured, and the entry's own geometry claim confirmed

```html
<text x="230" y="199" text-anchor="middle"
      font-family="Georgia, 'PingFang SC', 'Microsoft YaHei', serif"
      font-style="italic" font-size="10" fill="#4A3226"
      data-i18n="label.plate">Physeter macrocephalus — Plate VII</text>
```

| arm | resolved `ff` | box | **centre** |
|---|---|---|---|
| en | `Georgia, "PingFang SC", "Microsoft YaHei", serif` | 152.96 × 11.02 | **515.23** |
| fr | same | 164.71 × 11.02 | **515.22** |
| zh | same | 150.69 × 11.02 | **515.23** |

The entry's own comment records that the Latin binomial is a scientific name identical in French so
**only the plate NUMBERING is translated**, and that the element is `text-anchor="middle"` in a
fixed viewBox so a wider string grows symmetrically about its own centre and moves no sibling.
**Both facts carry into Chinese unchanged, and the geometry claim is now measured rather than
inherited: the centre is invariant to 0.01px across all three arms.** The zh row reads
`Physeter macrocephalus — 图版 VII` and round-tripped through the blind read **exactly**.

**Result: `undeclared-font` 113 → 0 against 113 Han nodes, 0 of which resolve without a named CJK
face — non-vacuous.** `grep -c 'PingFang SC'` = 5 (four rules plus one mention in the block-5
comment prose).

---

## 6. Geometry — three defect classes, 250 symptoms, 0 moved on all three arms

`check-ui-labels` first zh run after the fonts: **`[7][zh-Hans]` FAIL, 250 moved**. The fr arm was
green throughout. Three passes took it to zero.

### A. The eleven-selector `1.11` RATIO block — transferred, and RE-DERIVED per selector

Task 1's carry-forward 2 says M8's line-box table is **leaf-scoped** and a `<button>` need not
match it, so the transferred ratio was verified against **O-Contrabass's own measured English
content boxes**, padding and border subtracted first:

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
| **`.viz-btn`** BUTTON | 9 | 24.00 | 14 | 1 | **10.00** | 9.99 | 0.01 |
| **`.tuning-file-btn`** BUTTON | 9 | 24.00 | 14 | 1 | **10.00** | 9.99 | 0.01 |
| **`.generator-btn`** BUTTON | 10 | 27.00 | 16 | 1 | **11.00** | 11.10 | 0.10 |

**All eleven inside `check-ui-labels`' 0.5px tolerance, so 1.11 TRANSFERS on this plugin too** —
and N10/N11's line-count trap is live: `.interval-list-header` is **two** line boxes in English in
its 122px column, so a naive box-over-font-size would have read 2.2 and doubled the pin.

Note that O-Contrabass's `.viz-btn` is **9px** where O-Wind's is 10px — the per-selector
verification is not ceremonial.

### B. Sixteen page-local `line-height: normal` leaves, each pinned at its own content box

| selector | fs | en box | pad+bd | content | ratio | matches M8? |
|---|---|---|---|---|---|---|
| `#preset-save` **BUTTON** | 12 | 24.00 | 10 | **14.00** | 1.1666667 | ✓ |
| `.tab-btn` **BUTTON** | 11 | 22.00 | 10 | **12.00** | 1.0909091 | ✓ |
| `.settings-label` | 9 | 10.00 | 0 | 10.00 | 1.1111111 | ✓ |
| `.sublabel` | 9 | 10.00 | 0 | 10.00 | 1.1111111 | ✓ |
| `#active-strings-value` | 9 | 10.00 | 0 | 10.00 | 1.1111111 | ✓ |
| `.strip-field-label` | 9 | 10.00 | 0 | 10.00 | 1.1111111 | ✓ |
| `.stepper-label` | 8 | 9.00 | 0 | 9.00 | 1.1250000 | ✓ |
| `.finetuner-title` | 8 | 9.00 | 0 | 9.00 | 1.1250000 | ✓ |
| `.range-caption` | 10 | 11.00 | 0 | 11.00 | 1.1000000 | ✓ |
| `.viz-caption` | **8.5** | 10.00 | 0 | 10.00 | **1.1764706** | **NOT IN M8** |

**8.5px is not in M8's measured table.** `10.00 / 8.5 = 1.1764706` is measured here and is this
page's contribution to it.

### C. Four caption ROWS that grew with no leaf reporting it (wave 4e N4)

After B, 250 was down to 14 — and the residue was four **containers**, each `dh = 3.0`, whose
leaves all reported identical boxes. `getBoundingClientRect()` on an inline element reports its
FONT box, not its LINE box.

| selector | fs | en box | pad+bd | lines | content/line | ratio |
|---|---|---|---|---|---|---|
| `.panel-label` ×6 | 10 | 16.00 | 5 | 1 | 11.00 | 1.1000000 |
| `.panel-label > span` ×6 | 10 | 11.00 | 0 | 1 | 11.00 | 1.1000000 |
| `.strip-label` | 10 | 11.00 | 0 | 1 | 11.00 | 1.1000000 |
| **`.drone-caption`** | 9 | 22.00 | 2 | **2** | **10.00** | 1.1111111 |

`.drone-caption` is two spans either side of a `<br>` — **two line boxes in every language** — so
the ratio is derived per line (20.00 / 2 = 10.00). The box over the font size would have read 2.22.

`.panel-label > span` is the wrapper the seven numbered headings put around `.numeral` +
`.panel-title`. Its `.panel-title` child is an **evidenced non-mover** (enH == zhH == 11) and
inherits the ratio unchanged at the same font size; the eighth heading, Schelleng's, carries its
key on this span directly and is pinned by the same rule. `.sublabel` is excluded by `:not()`
because it is 9px.

### D. Two WIDTH FLOORS — R7/N9, and R8 at its most dangerous

The last 14 movers had **one** root cause and one rider: a caption that **shrank**.

| selector | en | fr | **zh** | shrink |
|---|---|---|---|---|
| `.sec-microtonal .strip-label` | 132.86 | 132.86 | **84.23** | **−48.63** |
| `#note-expression-toggle .strip-field-label` | 91.89 | 91.89 | **38.80** | −53.09 |

Both measure **identically in English and French** — the first because its keyed half is
` · Microtonal`, which the French table marks `sameAsEn`, and the second because "Note Expression"
is the VST3 feature name and was `sameAsEn` too. **Chinese is the first language that moves
either**, and it dragged 12 non-label elements 48.6px to the left.

Floored at the **exact measured English box**, never rounded up, and a floor rather than a fixed
width.

**R8 fired on every selector before any pin was appended.** The page carries **10 min-width, 9
min-height, 3 line-height, 20 letter-spacing and 10 white-space** declarations,
comment-stripped — **exactly the J14 census, all five numbers**. None of the 21 selectors touched
carried a prior `line-height`; the three that exist are on `.plugin-name`, `.tooltip-body` and
`.knob-label`. The nearest min-width is `.strip-field > .strip-field-label { min-width: 98px; }`,
whose **own comment records that it is child-scoped DELIBERATELY** so the Note Expression caption,
*"identical in both languages inside `.toggle-field`"*, is not widened for nothing. That reasoning
was true for French and is false for Chinese; rather than widening the v1.8.0 pin, the new floor
was added at higher specificity on the id, so the French floor it protects is left exactly as
measured. **The fr arm did not move.**

Census after the work: **12 min-width, 9 min-height, 12 line-height, 20 letter-spacing, 10
white-space** — +2 floors and +9 ratio rules, and the letter-spacing and white-space counts
unchanged, which is the arithmetic proof no existing pin was rewritten.

**`letter-spacing: 1.6px` on `.sec-microtonal .strip-label` was READ and LEFT.** It is a Latin
uppercase-tracking convention, and the ship bar's fifth geometry shape invites trimming it under
`html[lang="zh-Hans"]`. It was not trimmed: `text-transform: uppercase` has no effect on Han,
tracking is a legitimate Chinese display convention, and trimming it would have deepened the shrink
the floor exists to absorb. Recorded as a decision, not an omission.

### Result

| screen | planning-time | table landed, no fonts | after fonts + B/C | **final** |
|---|---|---|---|---|
| `undeclared-font` | 0 — **a vacuum** (J13) | **113** | 0 | **0** |
| `line-height-normal` | 0 — vacuum | 54 | 18 | **9** |
| `wrap-count` | 0 — vacuum | 0 | 0 | **0** |
| `svg-font-attr` | 1 carrier / 0 findings | 1 / 0 | 1 / 0 | **1 / 0** — and it is a DIVERGENCE report, see §7.3 |
| `check-ui-labels [7][zh-Hans]` | — | — | 250 → 14 | **0 moved** |

**`identity: 1695 nodes / 565 distinct DOM keys / 169 distinct display ids`** — the display-id
figure matches the plan's live table (169) exactly.

### The 9 `line-height-normal` residuals, every one an evidenced NON-MOVER (N1)

| node | fs | enH | zhH | why no pin can change it |
|---|---|---|---|---|
| `svg text` (`label.plate`) | 10 | 11.02 | **11.02** | its box is set by the SVG, not by a line box |
| `#preset-prev` ◀ | 12 | 26.00 | **26.00** | glyph in a fixed-height control |
| `#preset-name` | 13 | 25.00 | **25.00** | preset name is `I18N_EXEMPT` — no Han to render |
| `#preset-next` ▶ | 12 | 26.00 | **26.00** | glyph in a fixed-height control |
| `#help-toggle` 开 | 9 | 22.00 | **22.00** | fixed-height toggle |
| `#canvas-schelleng` | 16 | 186.00 | **186.00** | **Han is in `data-tip`/`aria-label` only** |
| `#canvas-spectrum` | 16 | 108.00 | **108.00** | same |
| `#canvas-vu` | 16 | 46.00 | **46.00** | same |
| `#scl-load-btn` 载入 .scl… | 11 | 28.00 | **28.00** | fixed-height button |

The three canvases are N1's exact case: `han` counts `data-tip`, `data-tip-title` and
`aria-label`, so a canvas carrying a Chinese tooltip is a finding forever — it has no Han to
render and its box is identical on both arms.

### The three `d848337a` floors, re-measured on the zh arm — they HOLD

| selector | en | fr | **zh** |
|---|---|---|---|
| `.tonic-label` | 42.05 × 9.98 | 42.05 × 9.98 | **42.05 × 9.98** |
| `.octave-stretch-label` | 53.86 × 9.98 | 53.86 × 9.98 | **53.86 × 9.98** |
| `.interval-list-header` | 122.00 × 22.19 | 122.00 × 22.00 | **122.00 × 22.00** |

The heights now read **9.98 (= 9 × 1.11)** rather than the face's `normal` box, so the ratio block
is live on them. **The wave's widest floor pair does not need redoing, and it does not stop
equalizing under Chinese** — Chinese is shorter than French on both, so the fr-widest floor covers
zh, exactly as it did on O-Bassoon.

### State-EFFECT assertions — all three `click` states (wave 4e N1)

| state | `elementFromPoint` @ centre | rect | effect |
|---|---|---|---|
| `.tab-btn[data-tab="tuning"]` | **target or descendant** | 621, 9.5, 100×22 | `#tuning-container` h **0 → 566**, **module captions 0 → 30** |
| `#gear-btn` | **target or descendant** | 737, 7.5, 26×26 | `#settings-popover` hidden **true → false**, h **0 → 71** |
| `#help-toggle` | **target or descendant** | 709, 82.5, 48×22 | help face **"Off" → "On"** |

No `null`, no different element, **no coverage hole**. The TUNING state is first, per the state
file's own cumulative-order reasoning, and `measure-ui --verbose` reports
`did not resolve|unresolved|skipped state` = **0** with `states default + 3` — all three applied.

**Module captions 0 → 30, not the plan's 13** — Task 2 §7.7 confirmed on a second consumer.
37 module keys minus s71 D5's 7 that never enter the DOM = 30 exactly, so the coverage hole is
arithmetically confirmed rather than assumed.

**The `10 never became visible` NOTE is unchanged at 10**, first entry
`#truekeys-view>div.tk-hint:nth-child(1)`, then six `#library-filter>option` and three
`#generator-type>option` — s71 D5's identified set, **nine of ten structurally unmeasurable** (an
`<option>` inside a collapsed `<select>` has no geometry for any DOM probe). **`74 of 82` is not a
fraction** and was not read as a failure.

---

## 7. FALSE PREDICTIONS — measured value beside the predicted one

### 7.1 Precondition (g) — `plugins/O-Strata/` is TRACKED, and a concurrent session moved again
- **Predicted:** *"`git status --short` → only `?? plugins/O-Strata/`"*, and a downstream verify
  `git status --short | grep -c "O-Strata"  # 1`.
- **Measured:** committed as `4965c271` before Task 1. During **this** task a concurrent session
  additionally created `?? plugins/O-Strata/.planning/mockups/`, so the count is **1** again but
  for a completely different reason — a NEW untracked subdirectory, not the original one.
  **The count is not the check.** The orchestrator's corrected form — *no entry under
  `plugins/O-Contrabass/`* — is the one that discriminates, and it was used.
  J1's lint consequence is unaffected: O-Strata still has no `i18n.js`.

### 7.2 The product-name control returns 0, not non-zero
- **Predicted (R6):** *"Expect the 'no product name in the batch' control to report a **non-zero**
  count on any plugin whose own copy names itself (O-Contrabass's markup carries a `.plugin-name`
  wordmark)."*
- **Measured:** **0** on both chunks, before dispatch and on the returned English. The wordmark IS
  in the markup — and it is an `I18N_EXEMPT` entry, not a table row, so it never enters the
  emitted batch. **The control's subject is the emitted zh column, and the exemption list is
  exactly what keeps the product name out of it.** Identical to Task 1 §7.2; the prediction has now
  been wrong twice in one wave for the same structural reason.

### 7.3 `svg-font-attr` CANNOT see this defect — it is a DIVERGENCE report
- **Predicted:** J16 — *"the `svg-font-attr` screen will report it the moment the column lands"*,
  and the verify block — *"`svg-font-attr` must read `1 attribute carrier(s), 0 finding(s)` … the
  FINDING must be zero WITH Han present, which is the first non-vacuous reading of that screen in
  the rollout."*
- **Measured:** the screen reads **`1 attribute carrier(s), 0 finding(s)` at every stage** — on the
  tree as found, with the Chinese column landed and the attribute still naming no CJK face, and
  after the fix. Its implementation (`scripts/measure-ui.js:553-570`) compares
  `normStack(r.ffAttr)` against `normStack(r.ff)` and reports the **subset whose attribute DIVERGES
  from the computed stack**. On this element the attribute IS the computed stack, so there is
  nothing to diverge — before or after.
  **The screen's verdict is invariant across the entire defect and the entire fix, and it never
  becomes non-vacuous.** Its only live signal is the carrier count (1), which the plan correctly
  calls language-independent.
  **What actually named this node was `undeclared-font`**, which had it as one of the 113 Han nodes
  resolving without a CJK face. **Carry-forward: on a plugin whose SVG attribute already equals the
  CSS that would otherwise apply, `svg-font-attr` is blind and `undeclared-font` is the detector.**

### 7.4 The font-declaration count is 11 + 1, not 12 + 1
- **Predicted:** live table — `font-family` declaration sites **12 + 1 SVG attribute**.
- **Measured:** **11 CSS declarations** (raw L81, L147, L249, L344, L365, L413, L632, L729, L762,
  L824, L903) **+ 1 SVG attribute** at L951 = **12 lines total**. The `9 font-family: inherit`
  half of the same table row is **exact**. Off by one on the named-stack count.

### 7.5 J4's O-Contrabass line numbers are COMMENT-STRIPPED, not raw — and the same finding mixes both
- **Predicted:** the enumerating form-6 sites at raw **L196-197**, feeding a `check()` at raw
  **L199**; and `missingFr` at raw **L269-273**.
- **Measured:** raw **L252-253** and raw **L256** for the first pair — but those are **L195/L198 in
  the comment-stripped file**, which is what the plan's numbers are. The `missingFr` numbers are
  **raw-correct** (raw L270-271, inside the predicted L269-273).
  **One finding, two numbering conventions**, the same shape as Task 2 §7.8 on J14's floors. Every
  other cited number is **exact**: loader publish raw **L229** ✓, prose `LANGUAGES` comment raw
  **L172** ✓, `2 language` fragment raw **L132** ✓, form-6 code-hit count **3** ✓, `LANGUAGES` in
  code **0** ✓. **Verify by pattern, never by line number** — Task 2 c/f 9 confirmed a second time.

### 7.6 "no external stylesheet" is true of the plugin and false of the page
- **Predicted:** live table — *external CSS: **none — all inline in `index.html`***.
- **Measured:** `index.html:28` carries `<link rel="stylesheet" href="css/tuning-panel.css">`,
  served by `PluginEditor.cpp:834` from
  `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` via
  `CMakeLists.txt:123`. The plugin's `Source/ui/public/css/` directory is **empty** — it owns no
  stylesheet — but the PAGE loads one, and it is the byte-frozen shared module CSS whose
  `.tuning-panel { font-family: -apple-system, … }` at line 66 is the whole subject of J15.
  **The claim is true about ownership and false about what the page loads, and only the second
  reading matters for the font work.**

### 7.7 Two new tooling traps, neither in the plan nor in Tasks 1–2
- **`serve-ui.js`'s `serve()` returns `{ server, port, close }` and has NO `url` field.**
  `page.goto(srv.url, …)` fails with `url: expected string, got undefined`. Build it:
  `` `http://127.0.0.1:${srv.port}/` ``. This is the third member of the family that already
  contains `resolvePlaywright()` returning the module (confirmed live again here) and `buildRoot()`
  returning an object.
- **A `'bt'` string literal inside `node -e '…'` under bash loses its quotes.** `s.replace(/…/g,
  "reviewed: 'bt'")` written inside a single-quoted `node -e` argument emitted
  `reviewed: bt` — a bare identifier — and the file then threw `ReferenceError: bt is not defined`
  on the very next parse check. **Write promotion scripts to a FILE.** The parse check caught it
  immediately; a promotion committed without one would have shipped an unloadable table.

### 7.8 Everything the plan predicted that HELD
`46 I18N + 94 LABELS`; **186** emitter rows; the `[12] 2 module(s)` line verbatim and NOT naming
`preset-manager.js`; `74 of 82` with **10** never-visible and its first entry verbatim; the four
computed-`ff` populations **473 / 30 / 1 / 1 exactly**; the 30 form-4 ids exactly; the R8 pin
census **10 / 9 / 3 / 20 / 10 exactly**; **9** `font-family: inherit`; **37/37** module keys in
en/fr on both plugins; s71 D8 closed at raw L658 / L719 / markup L1289 with an empty duplicate
scan; the stale-body probe reading **2**; the form-6 code-hit count **3** and `LANGUAGES` in code
**0**; the loader publish, the prose comment and the bridge-count fragment at their **exact** raw
lines; `VERSION 1.9.0` **unquoted**; target = folder; `PLUGIN_CODE OCbs`; `IS_SYNTH TRUE`;
`PluginProcessor.h` L161-162; the endonym-form convention at L1001; **1000 × 650**;
`identity … 169 distinct display ids`; the three `d848337a` floors at their measured values; the
XML `setAttribute`/`getStringAttribute` persistence as the wave's only one; **no** language
`AudioParameterChoice`; the two orphan bundles with **distinct subtypes `OCb5` and `OCbP` against
`OCbs`**, both at 1.0.0, and **zero** `Sweeping ALTERNATE-variant` warnings; and J1's discriminator
verbatim.

---

## 8. The blind reverse read

Emitted **`--emit O-Contrabass --plugin O-Contrabass`** (BOTH — W4) **twice**, so each chunk
carries its **own salt**: `f6d72a78…` and `90497d91…`, sharing **0 of 186** blinded ids.
149 rows split **75 + 74**, proved disjoint on the **real** ids (0 overlap, union exactly 149) and
proved to contain **no module row**. Dispatched
`claude -p --model claude-sonnet-4-5 --allowed-tools ""` from `/tmp/ja8t3/blindrun` — outside the
repo — with file, web and repository access forbidden in the prompt.

**All 46 I18N entries had their title and their body sent to DIFFERENT readers deliberately**
(Task 2 c/f 13).

### Controls — sixteen, every one fired

| control | result |
|---|---|
| line count matches sent | **75/75 and 74/74** |
| ids identical **AND IN ORDER** (M12) | **clean on both chunks** |
| ids pure 12-hex | 149/149 |
| ids unique | 149/149 |
| no key fragment in any id | 149/149 |
| rows well-formed (exactly 2 tab fields) | 149/149 |
| Han surviving in the returned English | **0**, with the positive control fired on the same probe (**75 and 74** Han lines SENT) |
| sha256 of the zh column vs the committed tree | **MATCH** — `233bedc7…95b51c34`, using **`LC_ALL=C sort`** |
| working tree == HEAD at read time | **YES** — the reverse pass ran against `5a216066` |
| module rows excluded from the batch | **37 excluded, 0 present in either chunk** |
| refusal 1 — missing `--forward-provenance` at emit | **FIRED** — *"the batch was emitted with no recorded forward pass"* |
| refusal 2 — wrong-side `--manifest` | **FIRED** — *"WRONG MANIFEST FOR THIS BATCH: all 75 returned ids are unjoinable, not one."* |
| product-name control (batch **and** returned English) | **0 / 0** — see §7.2 |

**Both refusals were run WITH `--provenance` supplied**, and refusal 2 additionally against a
manifest that HAS forward provenance, so neither could fire for the other's reason. Task 2 §7.9's
three ordered steps — reverse `--provenance` → forward provenance → id join — confirmed.

### All 149 triples read with `--verbose` (R2)

**90 round-tripped word for word.** Every one of the remaining **59** was read and adjudicated on
**collision on the page**, not drift distance.

### ZERO ROWS RE-AUTHORED — and therefore NO correction round two

R4's stated condition: *"A round that corrects nothing needs no round two; say so in the commit."*
It is said in the commit and it is said here.

The authoritative answer to R3's downstream half is **mechanical, not asserted**: 29
shared-rendering groups, **5 carrying different English**, and all 5 proved **SAME-CONTROL** by
walking each label's markup node up to its enclosing `data-param` and comparing it against the
tip's own `TIP_BINDINGS` selector:

```
label.vibRate     -> data-param=VIBRATO_RATE      tip selector [data-param="VIBRATO_RATE"]
label.vibDepth    -> data-param=VIBRATO_DEPTH     tip selector [data-param="VIBRATO_DEPTH"]
label.vibOnset    -> data-param=VIBRATO_ONSET     tip selector [data-param="VIBRATO_ONSET"]
label.infSustain  -> data-param=INFINITE_SUSTAIN  tip selector [data-param="INFINITE_SUSTAIN"]
label.subHarm     -> data-param=SUB_HARMONICS     tip selector [data-param="SUB_HARMONICS"]
```

**0 distinct-control collisions.** The other 24 groups are same-English caption/title pairs, which
R3 excludes explicitly.

### Accepted with reasons — the classes worth naming

**The ABBREVIATION class** — identical to Tasks 1 and 2. `Vib Rate`, `Vib Depth`, `Vib Onset`,
`Inf. Sustain`, `Sub-Harm.`, `Fine Tuners · cents` all returned **unabbreviated** ("Vibrato Rate",
"Infinite Sustain", "Subharmonics", "Fine Tune · Cents"). English abbreviates against this page's
**62px** `.knob-label` cap, which the table's own header records re-measuring at v1.8.1; Chinese
does not need to, so the Chinese caption carries the full term. Each maps to exactly one control.

**The BARE-NOUN class** — 饱和 → "Saturation", 张力 → "Tension", 劲度 → "Stiffness", 明亮度 →
"Brightness", 阻尼 → "Damping", 电平 → "Level". Each is its glossary root and this page's only
control of that name.

**运弓工作点 → "Bow Point" (score 0.78) — ACCEPTED, and the reason is the discriminator.** The page
also carries **弓位** (`BOW_POSITION`), which **both readers recovered correctly and separately**
as "Bow Position" in the same batch. The two share one character in different positions, sit in
different panels, and the concepts are *deliberately* related — the Schelleng diagram plots the
operating point, which shifts with bow position β. **A collision in the reader's English is not a
collision on the page**, which is Task 2's `笛头共振峰` precedent applied.

**E1–G3 · 单音 → "Mono" (0.56) — ACCEPTED.** The channel sense on this page is **单声道**, inside
the `WIDTH` tooltip body, and it round-tripped separately and correctly. 单音 beside a pitch range
reads as *one note at a time* and cannot be the other.

**持续音 → "Drone" — the round trip settled it.** `label.secDrone` (` · 持续音`) returned
**"· Drone"** at score 1.00, so the `INFINITE_SUSTAIN` body's *"the sustainer awakens"* was the
reader's own paraphrase of a word it had already recovered as *Drone*. 持续音 (the panel) and
无限延音 (the knob inside it) share only 音 and are no closer than the English "Drone" and
"Infinite Sustain".

**`readout.activeStrings` `4 根中的 {n} 根` → `{n} of 4` at score 1.00**, placeholder intact. The
entry's own comment demands the connective be localized rather than dropped to a symbol, and the
exact round trip is the evidence that it was.

**`label.loadSclFile` `载入 .scl…` → `Load .scl…` at score 1.00.** See §8.1.

**`label.plate` `Physeter macrocephalus — 图版 VII` → exact round trip**, binomial preserved,
numbering translated.

### 8.1 The one R3 adjudication that mattered — the D8 pair, seen from the Chinese side

The pre-authoring R3 screen (140 rows, **109 glossary-covered**, **24 root collisions**) named one
group with **three** members and two genuinely different controls:

```
ROOT  load .scl -> 载入 .scl
      label.loadSclFile [LABEL]  "Load .scl…"     <- this plugin's own button
      label.loadScl     [LABEL]  "Load .SCL"      <- the shared module's button
      scl-load-btn      [TITLE]  "Load .scl"      <- the tooltip title of the FIRST one
```

This is s71's D8 site seen from the other side: the key collision is closed, but the two buttons
still say nearly the same thing. **Measured rather than assumed:** `#scl-load-btn` lives inside
`#tab-main` (opens at markup L1033, the button at L1289) and the module's `#btn-load-scl` lives
inside `#tuning-container` inside `#tab-tuning` (L1304). **They are on different tabs and only one
`.tab-panel` is `.active` at a time, so they are never co-visible.**

The module row is frozen at `'bt'` and cannot be the side that moves, so R3's *"qualify the side
that is ambiguous"* would have to fall on the local one. It was **not** qualified: the glossary
root is `载入 .scl`, the English and French both keep the ellipsis as the file-dialog convention
(`Load .scl…` / `Charger .scl…`), and the tooltip title on the same control carries the full
disambiguating body. **Authored `载入 .scl…`, taking the root and the ellipsis.** The blind reader
returned `Load .scl…` exactly.

The second three-member group, `period (c)` (`label.genPeriod` and `label.genR2Period`, both
`周期 (C)`), is **two module rows inherited at `'bt'` with identical English** — R3 excludes
same-English pairs, `genR2Period` is one of s71 D5's seven keys that never enter the DOM, and
neither was in the batch.

### R3 glossary screen — before and after

- **Before authoring:** 140 rows screened, **109 glossary-covered**, **24 root collisions** — 22
  same-control caption/title pairs, plus the two three-member groups adjudicated above.
- **After authoring, mechanically:** 29 shared-rendering groups, 5 with different English, **all 5
  proved same-control**, **0 distinct-control collisions**.

### W3 / N8 checked BOTH ways

**Direction 1 — every exempt token in an English body survives verbatim into the Chinese:** the
four open-string names **E A D G** (in `active-strings` and all four `DETUNE_*` bodies), **A4**
(`REFERENCE_PITCH`), **Scala**, **MTS-ESP** and **12-TET** (`tuning-system`, `scl-load-btn`) — all
present.

**Direction 2 — every non-exempt caption named in a body is named by its LOCALIZED caption:**
Size → 尺寸, Damping → 阻尼, Brightness → 明亮度 (in `canvas-spectrum`), Level → 电平, Save →
保存, Expression → 表情, Note Expression → 音符表情. All present.

**ONE checked non-defect.** `lang-select`'s body contains the word "English" in *"preset names stay
in English"*, and the Chinese renders **英文** rather than the endonym `English`. That is correct
and the shipped French proves it: the same clause has read *"restent en anglais"* since the entry
was written. **The discriminator is whether the token names the SELECTOR'S OPTION — which must
survive verbatim so a user can find it in the dropdown — or describes a language as prose. This one
is prose.** Recorded rather than "fixed", which is W3's *"one body can need both directions"*.

---

## 9. Gate results — every actual output line

```
check-i18n --plugin O-Contrabass            exit 0
  PASS: [1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr","zh-Hans"]
  PASS: [1] the table carries copy — 46 I18N + 94 LABELS      [UNCHANGED from 46 + 94]
  PASS: [12] there is shipped page JS to scan — 2 module(s): the inline <script type="module">
             in index.html, modules/tuning/scala-tuning-engine/js/tuning-panel.js
             [preset-manager.js still correctly OUT of scope — s71 D2]
  ALL CHECKS PASS — 1 localized plugin(s)

i18n-zh-lint --plugin O-Contrabass          exit 0
  TOTAL                             140    ·  ·  ·  ·  ·  ·  ·  ·  ·  ·      0
  BELOW SHIP BAR — entries at reviewed:'mt': 0
  termNote exemptions (info): 1   [label.genDivisions, inherited with its module row]
  GATE PASSED — exit 0. 0 findings across 1 plugin(s).

check-ui-labels --plugin O-Contrabass       exit 0
  8 x PASS: [7][GEOMETRY DIFF][fr|zh-Hans] no non-label element moved   (4 states x 2 arms)
  PASS: [7][GEOMETRY DIFF][zh-Hans] the visible element SET is identical in English and zh-Hans
  PASS: [8][zh-Hans] two labels disjoint in English do not intersect in zh-Hans
  PASS: [8b][zh-Hans] no label intersects a NON-label element it cleared in English
  PASS: [2][vacuity][zh-Hans] the zh-Hans pass actually rendered — 52/52 labels (100%)
  PASS: [2][vacuity][zh-Hans] keyed ATTRIBUTES actually changed language — 6/6
  PASS: [4][zh-Hans] no leaf label is clipped by its own overflow
  coverage: 74 of 82 [data-i18n] elements were VISIBLE in at least one state
            10 never became visible                          [UNCHANGED, s71 D5's set]
  == ALL CHECKS PASSED ==

ui_frontend_check.js                        exit 0
  PASS: LANGUAGES derives from the table as a non-empty array opening with the source
        language — got ["en","fr","zh-Hans"]
  PASS: the derived list carries at least one language besides the source language
     checking 3 language(s): en -> fr -> zh-Hans
  PASS: all 56 distinct markup keys resolve in LABELS or I18N in every declared language
        (en, fr, zh-Hans)
  PASS: every bound tooltip carries a title AND body in every declared language
        (en, fr, zh-Hans)
  PASS: bridge surface is exactly 36 fns (2 mockup + 10 preset + 20 tuning + 2 hover-help
        + 2 language) — got JS=36 C++=36                     [UNCHANGED]
  == ALL CHECKS PASSED ==   (30 PASS)                        [was 28]

boot-all-uis --plugin O-Contrabass --strict-tips   exit 0
  clean: 1/1   DEAD bindings: 0   late bindings: 0

measure-ui --report all
  undeclared-font:    0 finding(s)   [was 113 Han nodes on 3 stacks; bare Arial 30 -> 17]
  line-height-normal: 9 finding(s)   [was 54; all 9 evidenced non-movers, enH == zhH]
  wrap-count:         0 finding(s)
  svg-font-attr:      1 attribute carrier(s), 0 finding(s)   [INVARIANT — see 7.3]
  identity: 1695 nodes / 565 distinct DOM keys / 169 distinct display ids
measure-ui --verbose:  did not resolve|unresolved|skipped state = 0   (states default + 3)

i18n-zh-backtranslate --plugin O-Contrabass
  O-Contrabass   186  186   0 mt   186 bt   0 native   0 none
  BELOW SHIP BAR — at reviewed:'mt' or unflagged: 0. The ship bar is 'bt'.
  corpus: 4292 -> 4478 rows of 5441

C++:  comment-stripped 'zh-Hans' count = 2
      languageCode: THREE-WAY   languageIndex: THREE-WAY   (matched by NAME, not swept)
      Han under Source/**/*.{h,cpp}: NONE
      positive control on the same probe: FIRED (Source/ui/public/js/i18n.js)
      persistence: XML setAttribute / getStringAttribute — the wave's only one, unchanged
      language AudioParameterChoice: none

Repo-wide, identical to the baseline:
  check-i18n     exit 0   ALL CHECKS PASS — 43 localized plugin(s)
  i18n-zh-lint   exit 2   0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read
                          (zh-Hans entries checked: 3094 -> 3234)
  i18n-fr-lint   exit 2   plugins with findings: 0 / 44   (1 could not be read)

Two-arm CMake reader (R9), run unconditionally on an UNQUOTED literal:
  one-arm 1.9.0 == two-arm 1.9.0  ->  bumped  ->  one-arm 1.10.0 == two-arm 1.10.0

Ratio-block verify:  grep -B14 "line-height: 1.11" | grep -c tuning-container  ->  11
                     grep -A14 (the plan's form)                               ->   0
R8 pin census:  before  min-w 10 / min-h 9 / line-h  3 / letter-sp 20 / white-sp 10
                after   min-w 12 / min-h 9 / line-h 12 / letter-sp 20 / white-sp 10

modules/ dirty: 0       modules/ in either commit: 0
scripts/ dirty: 0       scripts/check-i18n.js byte-unchanged
O-Strata in either commit: 0    libs/SAF in either commit: 0
PLUGINS.md in either commit: 0  tags at HEAD: 0

Build:  ./scripts/build-and-install.sh O-Contrabass   exit 0, all 7 phases
        VST3 + AU installed, 0 "Sweeping ALTERNATE-variant" warnings
        Installed CFBundleShortVersionString: 1.10.0  (both bundles)
        Components/ carries 3 O-Contrabass* bundles:
          O-Contrabass-dev            subtype OCbs   1.10.0   <- the shipping one
          O-Contrabass-pre-2-5-dev    subtype OCb5   1.0.0    <- orphan, LEFT as found
          O-Contrabass-pre-port       subtype OCbP   1.0.0    <- orphan, LEFT as found
        The two orphans are DISTINCT PRODUCT NAMES with distinct AU subtypes, not
        dev/release variants, so Phase 4's <Name>/<Name>-dev sweep structurally
        cannot see them and correctly did not warn. The expectation HELD (s71 D11).
auval:  NOT RUN — deferred to Task 6 by design. O-Contrabass's triple is
        `aumu OCbs OuDv` (IS_SYNTH TRUE). auval -v takes THREE UNQUOTED WORDS.
```

---

## 10. Carry-forwards for Tasks 4–5 (O-Reed and O-Bowed)

**Both render their WHOLE tuning panel through `-apple-system` (J15, 116 nodes each). What
transfers from here and what does not:**

1. **The `.tuning-panel` override IS the fix, and O-Contrabass's is the shape to copy** — a
   plain `.tuning-panel { … font-family: <house stack> + CJK tail; }` sitting with the
   parchment-palette variable overrides, at raw L894-903 here. O-Wind's is
   `#tuning-container .tuning-panel` at its L709-718. **Either specificity beats the module's
   `.tuning-panel` rule**; use whichever fits the file, and record which.
2. **What does NOT transfer: O-Reed's and O-Bowed's Latin arm WILL move.** O-Contrabass and O-Wind
   were already overridden, so their tail-only edits changed nothing in en/fr. On O-Reed and
   O-Bowed the panel body goes from the system UI face to Garamond/Georgia/TNR — a real metric
   change on **116 nodes each**. **Re-run `check-ui-labels` on en and fr and expect movement**, and
   have the fallback ready: append the tail to a *copy of the system stack*
   (`-apple-system, BlinkMacSystemFont, 'PingFang SC', 'Microsoft YaHei', sans-serif`), which
   leaves Latin untouched. **Measure, then choose, and record which and why.** The shared
   `tuning-panel.css` stays byte-unchanged either way; assert it.
3. **Use all FIVE tail selectors** (Task 2 c/f 4, confirmed here). O-Contrabass's 30 form-4 nodes
   became **11 Han-bearing** after the table landed — 5 `.viz-btn` + 5 `.tuning-file-btn` + 1
   `.generator-btn` — and O-Reed and O-Bowed also read 30, so expect the same 11 plus the 2
   invisible selects. Say "the rule covers 13 where the screen names 11" rather than reporting the
   smaller number.
4. **The `1.11` ratio TRANSFERS on a third consumer — verify it anyway, per selector.** All eleven
   selectors here landed inside 0.5px, but **`.viz-btn` is 9px on O-Contrabass and 10px on
   O-Wind**. The per-selector check is not ceremonial. And `.interval-list-header` is **two line
   boxes in English on every consumer** — derive per LINE or the pin doubles.
5. **The three `d848337a` floors HOLD on the zh arm on a second consumer** (42.05 / 53.86 / 122×22
   here, all three arms equal). O-Reed's and O-Bowed's are **24px / 41.59px / 51.84px** — different
   numbers, same expectation. Re-measure, do not redo.
6. **Budget for the ROW class as well as the leaf class.** Here the leaf pins took 250 movers only
   down to 14; **four containers** (`.panel-label`, its wrapper `span`, `.strip-label`,
   `.drone-caption`) carried the rest with every leaf inside them reporting an identical box. Read
   the `dh` column in `check-ui-labels`' `[7]` output — a `dh != 0` row is a root cause and a
   `dy != 0` row is cascade.
7. **And budget for a WIDTH SHRINK.** Two captions here were identical in en and fr — one
   `sameAsEn`, one a product term — so **Chinese was the first language that moved them**, and one
   dragged 12 elements 48.6px. Grep each consumer's table for `sameAsEn: true` before assuming a
   caption is stable.
8. **`svg-font-attr` is a DIVERGENCE report and both O-Reed and O-Bowed read 0 carriers**, so the
   screen is inert on them. `undeclared-font` is the detector for a missing CJK face. (§7.3)
9. **Their J4 form-6 sites are RESTORE-scoped — LEAVE them, classified at each site.** O-Contrabass
   was the only ENUMERATING instance in this wave. Their gates carry **form 1** (`LANGUAGES.join`)
   at raw L252 / L227 and **form 7** (the paired `sweep()` call) instead — Task 2's repair on
   O-Wind is the shape, including preserving any page-state call between the sweeps inside the loop.
   **Verify every site by PATTERN, not by the plan's line number** — J4's O-Contrabass numbers were
   comment-stripped and its `missingFr` numbers were raw, in one finding (§7.5).
10. **Fire the derive-or-abort control TWICE** — empty list and one-member list. The one-member
    plant is the load-bearing one and it needs a **second, independent** guard on the walk; a shape
    assertion alone passes it.
11. **Their J8 and J9 sites are REAL** — unlike O-Contrabass's. O-Reed's `tip.langSelect` and
    O-Bowed's `tip.language` both promise the Tuning tab stays English (false since s71), and both
    settings bodies claim the panel holds the language and nothing else (false since v1.4.0 /
    v1.7.0). **Correct the body AND the load-bearing comment above it**, re-read the file, then
    author (C7). O-Bowed's comment literally says *"THE TUNING CLAUSE IS LOAD-BEARING AND IT IS
    TRUE"*.
12. **O-Reed's fifth probe hit is a look-alike** — its `I18N_EXEMPT` rationale for the XY markers
    says the set *"stays English rather than being split across two languages"*, which is about one
    exempt set and stays true under three. **Classify it; do not delete it.**
13. **Check both for a `Flutter` caption** (Task 2 c/f 14). The glossary root `快抖` is the *tape*
    sense; a wind/reed instrument wants `花舌` and a per-page `termNote`.
14. **The module rows copy cleanly and mechanically, 37/37 first attempt**, brace-matched from
    O-Bassoon. Derive the key list from the module's own markup (38 hits, drop the `label.xxx` in
    the doc comment). Comma rule: insert before the entry's matching `}`, add a `,` unless the
    preceding text already ends in `,` or `{`.
15. **Use the corrected duplicate-key scan** (exclude the declared `LANGUAGES` members) and assert
    the LABELS count is unchanged — those two are still the only detectors for the D8 class, and
    there is still no `no key is declared twice in one table` assertion in this repo.
16. **`grep -B14`, not `-A14`**, for the ratio block (11 here, 0 the other way).
17. **Write promotion scripts to a FILE**, never `node -e '…'` under bash — the `'bt'` quotes are
    eaten and the table becomes unparseable (§7.7). Always re-parse after promoting.
18. **`serve()` returns `{ server, port, close }` with no `url`** — build
    `` `http://127.0.0.1:${srv.port}/` `` yourself (§7.7). `resolvePlaywright()` returns the module
    and `buildRoot()` returns an object: three members of one family.
19. **The tuning state mounts 30 module captions** on a second consumer too, and it is the
    state-EFFECT figure to assert (37 − s71 D5's 7 = 30).
20. **The product-name control returns 0 whenever the wordmark is an `I18N_EXEMPT` entry rather
    than a table row** — it has now been predicted non-zero and measured zero twice in this wave.

---

## 11. Deferred / out of scope

- **auval — NOT RUN**, deferred to Task 6's single cold sweep by design. Triple `aumu OCbs OuDv`.
- **`PLUGINS.md` NOT touched.** Its O-Contrabass row reads **1.9.0** and Task 6 must set it to
  **1.10.0** (one minor). O-Wind's must go to 1.21.0 and O-GrainScatter's is two minors stale at
  2.6.1 → 2.8.0.
- **The glossary is NOT edited.** Task 1's `'dist lpf': ['失真低通']` → `['距离低通']` at
  `scripts/i18n-zh-glossary.js:433` remains open as a corpus-level item. **O-Contrabass adds no
  glossary defect** — the scoped lint passed on the first authoring pass with 0 findings and no
  termNote of its own.
- **The 17-of-37 module coverage hole is NOT closed** (s71 D5): 10 never-visible + 7 that never
  enter the DOM. Closing it needs states that change the generator type and select the Rotation
  visualisation; this wave does not budget them.
- **The two orphan bundles are LEFT as found** — `O-Contrabass-pre-2-5-dev.component` (subtype
  `OCb5`) and `O-Contrabass-pre-port.component` (subtype `OCbP`), both at 1.0.0. Distinct product
  names, not dev/release variants; they do not shadow `OCbs`.
- **`.sec-microtonal .strip-label`'s `letter-spacing: 1.6px` was read and LEFT** under Chinese —
  a decision, recorded in §6D, not an omission.
- **`reviewed: 'native'` OPEN** on all 186 rows — no native Chinese reader exists on this project.
  Disclosed in the CHANGELOG and printed by lint rule R1 on every run.

## Self-Check: PASSED

- `plugins/O-Contrabass/Source/ui/public/js/i18n.js` FOUND;
  `plugins/O-Contrabass/Source/ui/public/index.html` FOUND;
  `plugins/O-Contrabass/Source/PluginProcessor.h` FOUND;
  `plugins/O-Contrabass/tests/ui_frontend_check.js` FOUND;
  `plugins/O-Contrabass/CMakeLists.txt` FOUND;
  `plugins/O-Contrabass/CHANGELOG.md` FOUND.
- Commit `5a216066` FOUND; commit `b69ca8ea` FOUND.
- `~/Library/Audio/Plug-Ins/VST3/O-Contrabass-dev.vst3` FOUND at **1.10.0**;
  `~/Library/Audio/Plug-Ins/Components/O-Contrabass-dev.component` FOUND at **1.10.0**.
