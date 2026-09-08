---
phase: quick-260907-ja8
task: 2
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4f, O-Wind, scala-tuning-engine, gates, fonts, geometry, enumeration-census, form-1]
status: complete
plugin: O-Wind
version: 1.20.0 -> 1.21.0
rows: 214
started_head: a92b798032bf6f75beeaeb8be63367463bba9288
commits:
  - 020409f8  # zh column at 'mt', LANGUAGES flip + module column in ONE edit, gate derived, fonts + line boxes
  - bff367f6  # promotion to 'bt', v1.21.0, CHANGELOG
metrics:
  emitter_rows: 214
  entries: 161            # 53 I18N + 108 LABELS
  module_rows_copied: 37  # byte-identical to O-Bassoon, at 'bt', excluded from the blind batch
  blind_batch_rows: 177
  gate_assertions: 788 -> 1109
  blind_chunks: 2         # 89 + 88, one salt each
  correction_rounds: 1    # round 2 (different model) corrected nothing further
  rows_reauthored: 2      # one Chinese string, on two keys
  termNote_exemptions: 4  # 3 authored here + label.genDivisions inherited from O-Bassoon
  geometry_pins_added: 9  # 8 line-box ratios + 1 width floor, plus the 11-selector 1.11 block
---

# Task 2 — O-Wind to Simplified Chinese (wave 4f, the wave's largest table)

**HEAD at start: `a92b7980`** — Task 1's promotion commit. The plan records `d37cdcc0`,
stale by three commits.

Two path-scoped commits, `git commit -- plugins/O-Wind` both times, `git branch --show-current`
and `git status --short` re-checked immediately before each. No `git add -A`, no tag, nothing
under `modules/`, nothing under `plugins/O-Orbit/libs/SAF`, `PLUGINS.md` untouched, and
`git show --name-only` over both commits reports **0** paths matching `O-Strata`.

---

## 1. Preconditions — all nine FIRED on the tree as found

| # | check | measured | plan predicted |
|---|---|---|---|
| a | `i18n-zh-lint --self-test` | **`SELF-TEST: 10/10`**, exit 0 | ✓ |
| b | `check-i18n --plugin O-Wind` | exit 0; `[1] LANGUAGES … got ["en","fr"]`; `53 I18N + 108 LABELS`; **`[12] 2 module(s)`** naming the inline `<script type="module">` **and** `modules/tuning/scala-tuning-engine/js/tuning-panel.js` | ✓ — `CMAKE_MODULE_JS_SCOPE` intact |
| c | `i18n-zh-lint --plugin O-Wind` | exit 0, VACUITY 0 zh entries | ✓ |
| d | `check-ui-labels --plugin O-Wind` | exit 0, `== ALL CHECKS PASSED ==`, `90 of 97`, **10 never-visible**, first entry `#truekeys-view>div.tk-hint:nth-child(1)` | ✓ verbatim |
| e | `plugins/O-Wind/tests/ui_tip_render_check.js` | exit 0, **788 PASS lines** | ✓ (the plan gave no baseline count) |
| f | 37 module keys present in en/fr, no duplicate key | **37/37 present in O-Wind AND O-Bassoon; duplicate scan NONE** | ✓ |
| g | git | `main`, one worktree, `git tag --points-at HEAD` = 0, **no entry under `plugins/`** | **FALSE — §7.1** |
| h | repo-wide `i18n-zh-lint` | **exit 2**, `0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read` | ✓ — J1's discriminator held |
| i | Task 1 committed, O-GrainScatter installed | `b534636b` + `a92b7980` present, `2.8.0` on disk | ✓ |

---

## 2. What shipped

**214 emitter rows** over **161 entries** (53 `I18N` × title+body, 108 `LABELS`) — the wave's
largest table. Corpus 3892 → **4106** rows of 5441. O-Wind reads `214 / 214 zh / 0 mt / 214 bt`.

**`reviewed: 'bt'`, `'native'` open.** 124 entries authored at `'mt'` (177 rows), promoted only
after the reverse read, in its own commit. The 37 module entries arrived at `'bt'` by copy —
R1's one stated exception.

**LABELS count UNCHANGED at 108** before and after the flip, and the duplicate-key scan is empty
both sides — the only detector for the s71 D8 collision class.

### The J6 fixture control — FIRED, and s71 was WRONG

`check-i18n --root /tmp/ja8t2/fixture` on a two-key fixture with `LANGUAGES = ['en','fr']` and a
`zh-Hans` value on one key. **Both arms run, and they are IDENTICAL:**

```
WITH the undeclared zh column      WITHOUT it
  PASS [1] LANGUAGES is exactly …    PASS [1] LANGUAGES is exactly …
  PASS [1] the table carries copy    PASS [1] the table carries copy
  PASS [1] every I18N key has an entry in every declared language (en, fr)   (both)
  PASS [1] every language entry has string t and b                           (both)
  PASS [1] every LABELS key has a string t in every declared language        (both)
  6 FAILED                           6 FAILED     <- all six are fixture-scaffold gaps
                                                     ([16], [6]x3, [8]x2), none is [1]
```

**Assertion `[1]` does not flag an undeclared extra column, in either direction.** s71's stated
mechanism ("assertion 1 requires every key in every declared language *and no others*") is false;
the plan's J6 correction is confirmed. The rule that the column must land WITH the flip stands
for the **stronger** reason: nothing catches the half-landing, and `i18n-zh-backtranslate` would
have counted 37 rows as shipped on a page that could never render them.

### Four termNote exemptions — three authored here

**`label.flutter` → 花舌, not the glossary root 快抖; `label.flutRate` → 花舌速率.** The glossary
root `'flutter': ['快抖']` is the *tape* sense — the fast half of wow-and-flutter, shipped that
way on O-Bitrot, whose own table spells the pair 慢抖与快抖. On O-Wind the caption sits on
`.knob-control[data-param="flutterTongue"]` and its own tooltip title is `Flutter Tongue`, whose
root IS 花舌. **This is NOT a glossary defect** — unlike Task 1's `dist lpf`, both senses are
legitimate and the corpus uses each correctly. It is a genuinely polysemous term that needs a
per-page note. `grep -rn "花舌\|快抖"` across all tables: 快抖 appears only on O-Bitrot, in the
tape sense. **Carry-forward: check O-Reed and O-Bowed for a `Flutter` caption — if either has
one, it takes the same note for the same reason.**

**`tip.instrumentPreset` → 乐器预设, not the glossary root 音色预设.** 音色 is this page's
rendering of `Tone Color`, a keyed knob caption in the Resonator group. 音色预设 would put a
reader's recovery ("tone-colour preset") on the wrong control. 乐器预设 takes the root this page
already gives the selector's own section caption `label.instrument` (乐器). The reverse read
confirmed it independently: round 2 returned **"Instrument preset"** at score 1.00.

The fourth is `label.genDivisions`, inherited verbatim with the module row.

---

## 3. Stale bodies — corrected FIRST, file re-read, then authored (C7)

The comment-stripped probe reported **7** hits, not the plan's 5 (§7.2), across four lines.

**`tip.langSelect` (J8) — FALSE IN FRENCH SINCE v1.20.0.** *"…and the Tuning tab stays English in
both languages."* / *"…et l'onglet Accord reste en anglais dans les deux langues."* s71 localized
the shared panel at module v3.1.0 and shipped 37 French captions on that tab; the body was never
revisited. Its load-bearing comment (*"one of this plugin's three tabs stays English for a French
user"*) was false with it and was replaced, not amended.

**`tip.gear` (J9) — FALSE SINCE v1.19.0.** *"It holds the interface language, and nothing else."*
/ *"Il ne contient que la langue de l'interface."* `#tips-toggle` is a child of
`.settings-popover` (markup L1167 / L1192 as found), and `check-i18n`'s `[16]` PASSES for this
plugin on the requirement that a page with a language selector carries exactly one bound, keyed
hover-help switch — which is what makes the claim false. Comment corrected with it.

**The pair clause (J10) — DELETED in en AND fr**, from `tip.langSelect`. *"between English and
French"* / *"entre l'anglais et le français"*. The readout-format clause beside it is true and
stays; so does the popover-direction clause in `tip.gear`.

Both verdicts are recorded at their entries **in prose (C8)**, and neither comment spells the
superseded literal. After correction the comment-stripped probe reads **0**.

---

## 4. The gate — form 1 repaired BEFORE the flip

`tests/ui_tip_render_check.js`, 913 lines. Repaired in an uncommitted working state and run green
at two languages **before** `LANGUAGES` gained a third member, so no commit in this task's history
contains a table whose third language makes its own gate red.

### N12 enumeration census — O-Wind, before → after

| form | before | after | disposition |
|---|---|---|---|
| **1 — `LANGUAGES.join(',') === 'en,fr'`** | **1** (raw L295, tagged `[0]`) | **0** | **the rollout's first form-1 site**, replaced by shape + derive-or-abort |
| 2 — string `'en,fr'` | 1 (inside form 1) | **0** | went with it |
| 3 — array-literal walk | 0 | 0 | absent |
| 4 — computed/UA-face form controls | n/a (a CSS form, see §5) | — | 25 nodes, closed |
| 5 — `Map` read with a literal key | 0 | 0 | absent |
| **6 — per-language object property access** | **5** (raw L677, L702, L723, L724, L889) | **5** | **RESTORE-scoped, deliberately LEFT** |
| **7 — paired literal-argument call** | **2** (`sweep('en')` L588 / `sweep('fr')` L717) | **0** | generalized to a loop |
| `__setLanguage(l), 'fr'` | 1 (raw L713) | 0 | driven from the loop variable |
| `showTab('sound')` between sweeps | 2 | **2** | preserved INSIDE the loop |
| `LANGUAGES` mentions | 3 | **8** | publish, destructure, shape assertion, abort guard, REST, log |
| residual language literal | 2 | **1** — `const EN = 'en'` | single-sourced; the shape assertion pins `LANGUAGES[0] === EN` |
| g5l inventory grep sees this file? | **yes** | **no** | off the list, and the comment does not put it back (C8) |

### The three repairs

1. **Derive-or-abort.** `LANGUAGES` was already published by the `vm` loader (raw L187) and
   destructured (raw L290) — nothing to add. The joined-string equality became a shape assertion
   (non-empty array opening with the English code) plus `process.exit(1)` with a refusal message.
   **No fallback literal.**

2. **A SECOND, INDEPENDENT guard on the walk.** `const REST = LANGUAGES.slice(1)` with
   `check(REST.length >= 1, …)`. Without it a one-member list passes the shape assertion and
   compares the page to itself.

3. **The paired call became a loop**, with `showTab('sound')` inside it. `sweep(lang)` was already
   language-agnostic (raw L387 `const entry = (I18N[key] || {})[lang];`) — the whole defect was
   the caller. Live: `sweeping 3 language(s): en -> fr -> zh-Hans`.

The placement-census log line said *"(both languages)"* and now says *"(every swept language)"*.

### Derive-or-abort control — fired TWICE, against the LIST and against the WALK

| plant | expected | observed |
|---|---|---|
| `LANGUAGES = []` | `[0]` refuses | `FAIL: [0] LANGUAGES derives from the table as a non-empty array opening with the English code — got []` → `REFUSING to sweep a guessed list.` **exit 1** |
| `LANGUAGES = ['en']` | shape passes, walk guard refuses | `PASS: [0] … got ["en"]` then `FAIL: [0] the derived list carries at least one language besides en` and `sweeping 1 language(s): en` **exit 1** |

The second plant is the load-bearing one: it proves the guard on the walk is **independent** of
the guard on the list. Reverted by **targeted edit, never `git checkout --`**; sha256
`4304bcf9…e05635` byte-identical both sides.

### The five restore-scoped sites — LEFT, classified

Raw **L677** (`I18N['tip.toneColor'].en.t`), **L702** (`tip.airColumn`), **L723-724**
(`tip.growl` `.t` and `.b`), **L889** (`tip.toneColor` `.en.b`). Each follows a plant restore or
a `__setLanguage` reset that writes English **by construction**, so the English row is the only
correct comparand — reading the current language there asserts nothing whatever the list holds.

**788 assertions before, 1109 after.**

---

## 5. Fonts — 25 form-4 nodes, and `.viz-btn` is the deliberate exception

13 declarations: 3 named stacks + **10 `font-family: inherit`**, exactly as J11 measured. The
computed `ff` census over 570 visible English nodes reproduced the plan's four populations
**exactly**: 425 house stack, 115 panel body, **25 bare `Arial`**, 5 `.viz-btn`.

The 25 bare-Arial ids are exactly those J11 names. After the table landed, **6 of the 25 carry
Han**: the five `.tuning-file-btn` (载入 .scl / 载入 .kbm / 保存 .scl / 保存 .kbm / 导出 HTML)
and `#btn-generate` (生成).

### Treatment, per site

| site | treatment | why |
|---|---|---|
| L69 house stack | **tail only** | Georgia + Times New Roman both installed (4 family matches each) |
| L718 `#tuning-container .tuning-panel` | **tail only** | this override is what keeps O-Wind's panel off the shared module's `-apple-system` stack |
| L763 `#tuning-container .viz-btn` | **tail only, ON THE EXISTING DECLARATION** | see below |
| new 4-selector `#tuning-container` rule | face + tail | `.tuning-file-btn`, `.generator-btn`, `.generator-type-select`, `.library-filter-select` |

**The five-selector rule became a FOUR-selector rule, deliberately.** `.viz-btn` already declares
`'Garamond', 'Georgia', serif` at L761-763 — which is why this plugin reads 25 form-4 nodes where
its three sibling consumers read 30. Naming it again in the new rule would leave two declarations
at **equal specificity** on one class, with the resolved stack decided by source order in a file
nobody reads that way. It took the tail in place instead. **Recorded as a per-plugin divergence
from O-Bassoon's shipped five-selector shape; Tasks 3–5 have no `.viz-btn` face and should use all
five.**

**The two `<select>`s are in the rule for a reason the screen cannot show.** `undeclared-font`
reads them as carrying **no Han**: a collapsed `<select>`'s selected-option text is not the
element's own text node, so the probe's `han` flag is false. But the control PAINTS that option
through its own font, and `#library-filter`'s six category options and `#generator-type`'s three
type options are all Han. **The plan asserted they "render Han through their own font" as though
the screen would show it; it does not — this is a decision about what the control paints, taken
against a probe that is structurally blind to it** (§7.4).

**The other 19 bare-Arial controls are the negative case, measured:** twelve `.interval-input`
numerics, `#octave-stretch`, `#gen-divisions`, `#gen-period`, and the two arrow glyphs
`#tonic-down` / `#tonic-up`. None renders Han, and the shared module gives none of them an
`aria-label` or a `data-tip` — **N3's tip-anchor discriminator answers in the NEGATIVE for every
one.** `measure-ui` reports bare-Arial 25 → **17** after the rule (the 8 that left are the 6
Han-bearing buttons plus the 2 selects).

`grep -c 'PingFang SC'` = **5**. `undeclared-font: 0 finding(s)` against **155 Han nodes**,
**0 of which resolve without a CJK face** — non-vacuous.

---

## 6. Geometry — two root causes, 176 symptoms

First zh run after the fonts: **`[7][zh-Hans]` FAIL, 174 moved on the Sound tab** plus **2 moved
in the popover state**. The fr arm was green throughout. The 176 were **two** defects.

### A. One caption's WIDTH — `dx=-59.9` on five elements

`.instrument-strip` is a flex row and its first item is a content-sized `.section-label`:
`"Instrument"` 84.38px → 乐器 **24.48px**, a difference of **59.90px**, which is exactly the
`dx=-59.9` reported on `#instrument-select`, `#tone-hole-toggle` and its two children and the
`.instrument-selector` wrapper. **One floor, at the exact measured English box (N9), on the
already-existing `.instrument-strip .section-label` rule** — a floor, never a fixed width, so a
future language that needs a wider caption can still take it.

### B. Eight `line-height: normal` leaves, each +3 or +4px

Every ratio **derived from this page's own measured English CONTENT box**, padding and border
subtracted FIRST, per size:

| selector | fs | en box | pad+border | content | ratio | matches M8? |
|---|---|---|---|---|---|---|
| `.section-label` | 11px | 12 / 15 | 0 / 3 | **12.00** | 1.0909091 | ✓ |
| `.fx-title` | 11px | 12 | 0 | 12.00 | 1.0909091 | ✓ |
| `.toggle-label` | 11px | 12 | 0 | 12.00 | 1.0909091 | ✓ |
| `.instrument-selector label` | 11px | 12 | 0 | 12.00 | 1.0909091 | ✓ |
| `.settings-toggle` **BUTTON** | 11px | 18 | 6 | **12.00** | 1.0909091 | ✓ |
| `.settings-label` | 10px | 11 | 0 | 11.00 | 1.1 | ✓ |
| `.preset-save-btn` **BUTTON** | 12px | 24 | 10 | **14.00** | 1.1666667 | ✓ |
| `.fx-bypass-btn` **BUTTON** | 9px | 16 | 6 | **10.00** | 1.1111111 | ✓ |

The ADSR caption is a bare `<span>` inside `.adsr-section .section-label` and **inherits** the
ratio; it is not named separately.

**R8 fired before appending:** none of the eight carried a `line-height`. The three that exist on
this page (`1`, `1.2`, `1.35`) are on other selectors.

### The transferred 1.11 ratio — VERIFIED against O-Wind's own button box (Task 1 c/f 2)

Task 1's `.toggle` proved M8's table is leaf-scoped and a `<button>` need not match it. **Measured
here rather than assumed — and on O-Wind it DOES match:**

| selector | fs | box | pad+border | content | 1.11 × fs | Δ |
|---|---|---|---|---|---|---|
| `.pitch-circle-label`, `.ref-knob-label`, `.tonic-label`, `.octave-stretch-label` | 9 | 10 | 0 | 10.00 | 9.99 | 0.01 |
| **`.tuning-file-btn` BUTTON** | 9 | 24 | 6+6 pad, 1+1 border | **10.00** | 9.99 | 0.01 |
| `.library-header-text`, `.generator-header-text`, `.gen-row label` | 10 | 11 | 0 | 11.00 | 11.10 | 0.10 |
| **`.viz-btn` BUTTON** | 10 | 23 | 5+5 pad, 1+1 border | **11.00** | 11.10 | 0.10 |
| **`.generator-btn` BUTTON** | 10 | 27 | 8+8 pad | **11.00** | 11.10 | 0.10 |
| `.interval-list-header` | 10 | **22 = 2 × 11** | 0 | 11.00/line | 11.10/line | 0.20 total |

All inside `check-ui-labels`' 0.5px tolerance. **N10/N11's line-count trap is live here:**
`.interval-list-header` is TWO line boxes in English (its 112px column), so a naive
box-over-font-size would have given 2.2 and doubled the pin.

### The three `d848337a` floors — re-measured on the zh arm, they HOLD

| selector | en | fr | zh |
|---|---|---|---|
| `.tonic-label` | 39.83 × 9.98 | 39.83 × 9.98 | **39.83 × 9.98** |
| `.octave-stretch-label` | 51.00 × 9.98 | 51.00 × 9.98 | **51.00 × 9.98** |
| `.interval-list-header` | 112.00 × 22.19 | 112.00 × 22.00 | **112.00 × 22.00** |

`git show d848337a --stat` confirms they landed in each consumer's own inline CSS, never in the
byte-frozen shared `tuning-panel.css`. The heights now read `9.98` (= 9 × 1.11) rather than the
face's `normal` box — the ratio block is live on them.

### Result

**0 moved on en, fr AND zh-Hans**, first pass after the pins, across all three states.
`line-height-normal` 62 → 42 → **21 residuals, every one an evidenced non-mover:**

| class | n | fs | enH | zhH |
|---|---|---|---|---|
| `#preset-prev` / `#preset-next` (◀ ▶) | 2 | 14px | 28.00 | **28.00** |
| `.tab-btn` ×3 (声音 调音 效果) | 3 | 11px | 35.00 | **35.00** |
| fx value readouts (`#chorusRateValue` … `#eqHighGainValue`) | 16 | 9px | 14.00 | **14.00** |

Every one is N1's exact case: the Han is in the node's `data-tip` / `aria-label`, not its text, so
it has nothing Han to render and no pin can change its box.

### State-EFFECT assertions — all three `click` states (wave 4e N1)

| state | `elementFromPoint` at centre | effect |
|---|---|---|
| `.tab-btn[data-tab="effects"]` | `button.tab-btn` — **target** | `#tab-effects` hidden `true→false`, h `0→525` |
| `.tab-btn[data-tab="tuning"]` | `button.tab-btn` — **target** | `#tuning-container` h `0→497`, **module captions `0→30`** |
| `#gear-btn` | `button#gear-btn.gear-btn` — **target** | `#settings-popover` hidden `true→false`, h `0→66` |

No `null`, no different element, no coverage hole. **`measure-ui --verbose` reports 0
`did not resolve|unresolved|skipped state`** — all three of the order-dependent states applied.

**The `10 never became visible` NOTE is unchanged at 10**, first entry
`#truekeys-view>div.tk-hint:nth-child(1)`, then six `#library-filter>option` and three
`#generator-type>option` — s71 D5's identified set, **nine of ten structurally unmeasurable**
(an `<option>` inside a collapsed `<select>` has no geometry for any DOM probe). A further
**7 module keys never enter the DOM at all** — `genStartHarmonic`, `genEndHarmonic`,
`genGenerator`, `genR2Period`, `genNotes`, `noteCount`, `rotationMode` — which `check-ui-labels`
cannot count because its denominator is final-DOM membership. **The 0 → 30 module-caption effect
above is 37 − 7 = 30, so the hole is arithmetically confirmed, not assumed.** Recorded as a hole;
closing it needs states that change the generator type and select the Rotation visualisation, and
this wave does not budget them.

---

## 7. FALSE PREDICTIONS — measured value beside the predicted one

### 7.1 Precondition (g) — `plugins/O-Strata/` is TRACKED
- **Predicted:** *"`git status --short` → only `?? plugins/O-Strata/`"*, and a downstream verify
  `git status --short | grep -c "O-Strata"  # 1`.
- **Measured:** committed as `4965c271` before Task 1 started. `git status` shows **no entry
  under `plugins/`**; the verify reads **0**. Already reported by Task 1; re-confirmed.
  **J1's lint consequence is UNAFFECTED** — O-Strata still has no `i18n.js`.

### 7.2 The stale-body probe reports 7, not 5
- **Predicted:** *"Fired at planning time: 5 (the pair clause en+fr, the false tuning clause
  en+fr, the false settings clause)"*.
- **Measured:** **7**, across four lines. The regex has nine alternates and three of them fire on
  the single French `tip.langSelect` line (`anglais et le fran`, `onglet Accord reste en anglais`,
  `deux langues`), two on the English one. The *substance* is exactly the five the plan names —
  the count is a property of the probe, not of the file. **Both readings go to 0 after the fix.**

### 7.3 J4's raw line numbers for the restore-scoped sites are wrong
- **Predicted:** O-Wind's form-6 sites at raw **L540, L565, L586-587, L752**.
- **Measured:** raw **L677, L702, L723, L724, L889**. The **count of 5 is right** and the
  classification is right; every cited line number is wrong. Every OTHER raw line number the plan
  gives for this file (L187 loader, L290 destructure, L295 form 1, L588/L717 the sweep pair, L589/
  L719 `showTab`, L713 `__setLanguage`, L716 the selector assertion, L387 the dynamic read) is
  **exact**. **Verify by the pattern, not the line number** — the plan's own verify block does
  this correctly (`grep -cE "I18N\['tip\.(toneColor|airColumn|growl)'\]\.en\.[tb]"` = 5 ✓).

### 7.4 `undeclared-font` cannot see a `<select>`'s Han
- **Predicted:** the two `<select>`s *"render Han through their own font (`#library-filter`'s six
  category options, `#generator-type`'s three type options)"* and are therefore among the nodes
  the screen names.
- **Measured:** the screen reads both as `han: false`. `measure-ui`'s `han` is computed over the
  node's OWN text plus `data-tip` / `data-tip-title` / `aria-label`; a collapsed `<select>`'s
  selected-option text is none of those. **The reasoning is right and the screen cannot confirm
  it.** They are in the rule anyway, on a decision about what the control paints. This is the
  same structural blindness as s71 D5's nine `<option>`s.

### 7.5 The plan's duplicate-key scan reports `zh-Hans` on EVERY three-language table
- **Predicted:** `node -e "…matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)…"` → **`NONE`**.
- **Measured:** **`zh-Hans`** — and the same scan returns `zh-Hans` on **O-Bassoon** (the copy
  source), **O-Detune** (the endonym source) and **O-GrainScatter** (Task 1's own output). The
  regex matches any quoted property name at any indent, and a multi-line `'zh-Hans':` block starts
  a line. **The scan cannot read NONE after any zh column lands.** Corrected form, used here:
  exclude the members of the declared `LANGUAGES` — a language code is never an entry key.
  Corrected result: **O-Wind 161 entry keys, 62 language sub-keys excluded, duplicates NONE**;
  O-Bassoon and O-GrainScatter also NONE.

### 7.6 The ratio-block verify greps the wrong direction
- **Predicted:** `grep -A14 "line-height: 1.11" … | grep -c "tuning-container"` → non-zero.
- **Measured:** **0** — and **0 on the transfer source O-Bassoon too**. `d848337a` writes the
  eleven selectors BEFORE the declaration, which is how CSS is written. The correct form is
  `-B14`, which reads **11** on both files.

### 7.7 The tuning state mounts 30 module captions, not 13
- **Predicted:** *"module caption nodes 0 → 13 is the effect `d848337a` used"*.
- **Measured:** **0 → 30**. 37 module keys minus s71 D5's 7 that never enter the DOM = 30 exactly.
  The 13 is presumably a different consumer's or a different probe's number; **30 is the one that
  reconciles with the plan's own coverage arithmetic.**

### 7.8 J14's O-Wind floor line numbers mix RAW and STRIPPED
- **Predicted:** `.interval-list-header` min-height at **L583**, `.tonic-label` at **L700**,
  `.octave-stretch-label` at **L701**.
- **Measured:** L700 and L701 are **raw-correct**; `.interval-list-header` is at raw **L685** and
  at comment-stripped **L582**. One table row, two numbering conventions.

### 7.9 The ingest's refusal ORDER has a third step (extends Task 1 §7.5)
- Task 1 found that ingest checks **forward** provenance before the id join. **There is a step
  before that:** the **reverse-pass `--provenance`** check fires first. A wrong-manifest control
  run without `--provenance` refuses with *"no --provenance was given"* and proves nothing about
  the manifest. **Order: reverse provenance → forward provenance → id join.** Both refusal
  controls here were run with `--provenance` supplied, and each then fired for its own reason.

### 7.10 Two authoring predictions of mine that the lint corrected
Not plan predictions, recorded because they cost a round trip. `label.fx.low` 低频 and
`label.fx.high` 高频 fired **Z5** — the lint requires the glossary root itself (低 / 高), not a
compound containing it. The three EQ band captions now read **低 / 中 / 高**, all three roots
(`'mid'` carries 中 among its three). And `label.tuningFailed` has a **whole-sentence** glossary
root, 调音面板载入失败, which the lint named. A termNote in `tip.instrumentPreset` containing
`<select>` fired **check-i18n `[9]`** — no string literal in `i18n.js` may contain `<`.

### 7.11 Everything the plan predicted that HELD
The `[12] 2 module(s)` line verbatim; `90 of 97` with 10 never-visible and its first entry
verbatim; the four computed-`ff` populations 425/115/25/5 **exactly**; the 25 form-4 ids
**exactly**; the R8 pin census 4/4/3/14/2 **exactly**; 13 font declarations of which 10 `inherit`;
53 + 108 keys; 214 emitter rows; 37/37 module keys in en/fr on both plugins; the three floors at
their measured values; `VERSION "1.20.0"` quoted; target = folder; `PLUGIN_CODE OWnd`;
`PluginProcessor.h` L108-109; the endonym-form convention at L1176; `#tips-toggle` inside
`.settings-popover`; the form-1 assertion at raw L295 tagged `[0]`; the sweep pair and its
`showTab` neighbours at their exact raw lines; and J1's discriminator verbatim.

---

## 8. The blind reverse read

Emitted **`--emit O-Wind --plugin O-Wind`** (BOTH — W4). **Two disjoint chunks of 89 and 88, each
from its OWN emit so each carries its OWN salt** (`9eb7ba8c…` / `bc442f74…`). Proved disjoint
before dispatch: **0 overlapping real ids, union exactly 177**, and two emits of the same target
share **0 of 214** blinded ids. **The 37 module rows were excluded** — dropped on the manifest's
real-key mapping, since the emitter has no row-range flag (Task 1 c/f 6).

Dispatched `claude -p --model claude-sonnet-4-5 --allowed-tools ""` from `/tmp/ja8t2/blindrun` —
outside the repo — with file, web and repository access forbidden in the prompt.

### Controls — sixteen, every one fired

| control | result |
|---|---|
| line count matches sent | 89/89 and 88/88 |
| ids identical **AND IN ORDER** (M12) | clean on both chunks |
| ids pure 12-hex | 177/177 |
| ids unique | 177/177 |
| no key fragment in any id | 177/177 |
| rows well-formed (exactly 2 tab fields) | 177/177 |
| Han surviving in returned English | **0**, with the positive control fired on the same probe (89 and 88 Han lines **sent**) |
| sha256 of the zh column vs the committed tree | **MATCH** — `0cd4dd86…568a58`, emitted batch and committed file's 214 rows |
| refusal 1 — missing `--forward-provenance` | **FIRED** (with `--provenance` supplied — §7.9) |
| refusal 2 — wrong-side `--manifest` | **FIRED**: `WRONG MANIFEST FOR THIS BATCH: all 89 returned ids are unjoinable, not one.` |
| product-name control | **2 hits, adjudicated** — see below |
| working tree == HEAD at read time | **YES** — the reverse pass ran against a committed tree |

**Product-name control, adjudicated:** *"which is also the usual behavior of **wind instruments**"*
(`tip.adsrEnabled`) and *"bury the pitch in **wind noise**"* (`tip.breathNoise`). Both are the
English common noun in the source copy, not the product name. Not a leak.

**A sort-locale trap, recorded:** the first sha256 comparison used the shell's default `sort` and
mismatched. `sort` is unstable across runs for equal-length Han strings under a UTF-8 locale.
`LC_ALL=C sort` gives a stable multiset hash and matched immediately.

### All 177 triples read with `--verbose` (R2)

95 round-tripped exactly. Every one of the remaining 82 was read and adjudicated on **collision
on the page**, not drift distance.

### ONE ROW RE-AUTHORED — and the CHUNK SPLIT is what found it

**`反向气束` (Reversed Jet).** The same Chinese string went to **two different readers** — the
title `tip.reversedJet` in chunk 1, the caption `label.revJet` in chunk 2. Chunk 1 returned
**"Reverse air jet"**. Chunk 2 returned **"REFLECTED jet"**.

This page carries a separate knob three sections away whose caption `label.jetRefl` AND tooltip
title `tip.jetReflection` both render **气束反射 (Jet Reflection)**. 反向 and 反射 share their
first character in the same position. **That is a collision on the page, not a drift distance.**

Re-authored to **反转气束** on both keys: the distinguishing morpheme moves position, the word
order reverses against the neighbour (modifier-first vs head-first), and it is the term this
entry's own **body** already used (`反转气束延迟的方向`) — so the caption and its body now agree
where they did not before.

**Correction round 2 — fresh reader, DIFFERENT MODEL (Haiku 4.5), fresh salt sharing 0 of 214
ids**, sent the two corrected rows inside a 24-row context set of every neighbouring caption so
the reader could not tell which row was under test:

```
label.revJet        反转气束  -> "Reversed air stream"
tip.reversedJet     反转气束  -> "Reversed air stream"
label.jetRefl       气束反射  -> "Air stream reflection"
tip.jetReflection   气束反射  -> "Air stream reflection"
```

The collision is closed. **Round 2 corrected nothing further, so there is no round 3.**

### Accepted with reasons — the classes worth naming

**The ABBREVIATION class.** `Vib Rate`, `Vib Pitch`, `Vib Tremolo`, `Inf. Sustain`, `Jet Refl.`,
`End Refl.`, `Sub Harm.`, `Rev. Jet`, `Flut Rate`, `Pre-dly`, `Mod`, `Mid Freq` all returned
**unabbreviated**. Deliberate and identical to Task 1's finding: French had to abbreviate against
this page's 72px `.knob-label` cap (its own header records `"Embouchure"` at 70.73px in a 72px
box) and Chinese does not, so the Chinese caption carries the full term. Each maps to exactly one
control.

**The BARE-NOUN class.** 音色 → "Tone" (round 1) / "Timbre" (round 2), 阻尼 → "Damping",
次谐波 → "Subharmonic", 预延迟 → "Predelay", 口型 → "Mouth shape", 喉音 → "Throat tone". Each is
a single-control term on this page with nothing to collide with, and each is its glossary root.

**端口反射 → "Port reflection"** — returned identically by BOTH rounds, and by a different model.
The page has exactly two reflection controls, 气束反射 and 端口反射, and both readers separated
them correctly. The open end of a bore IS the port. Accepted.

**气压 → "Air pressure"** — both rounds. The page's only pressure control; 气压 is the glossary
root for `Breath Pressure`. Accepted.

**`tip.formant`'s 笛头共振峰 → "embouchure formant" — ACCEPTED, and the reason matters.** This
page HAS an Embouchure knob, so the English paraphrase collides. But the collision is in the
reader's ENGLISH, not on the page: the Embouchure knob is captioned **口型**, which shares no
morpheme and no position with **笛头**. R3's discriminator asks whether a reader ON THIS PAGE can
confuse the two strings, and a Chinese reader cannot. 笛头 is the correct term for a flute's
headjoint.

### R3 glossary screen — clean, before and after

**Before authoring:** 119 of 161 captions glossary-covered; **3 root collisions, all
same-control pairs** — `tip.vibratoRate`/`label.vibRate` (颤音速率),
`tip.infiniteSustain`/`label.infSustain` (无限延音), `tip.tipsToggle`/`label.hoverHelp`
(悬停帮助), which R3 excludes explicitly.

**Mechanical downstream check after authoring:** 26 shared-rendering groups, 11 carrying different
English — and **all 11 proved SAME-CONTROL mechanically**, not asserted: for each, the label's
markup node was walked up to its enclosing `.knob-control[data-param="…"]` and compared against
the tip's own `TIP_BINDINGS` selector. All eleven matched exactly (the twelfth,
`tip.tipsToggle`/`label.hoverHelp`, shares a `.settings-row`). **0 distinct-control collisions.**

---

## 9. Gate results — every actual output line

**Preconditions (tree as found)** — §1.

**After the work:**

```
check-i18n --plugin O-Wind                exit 0
  PASS: [1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr","zh-Hans"]
  PASS: [1] the table carries copy — 53 I18N + 108 LABELS          [UNCHANGED from 53 + 108]
  PASS: [12] there is shipped page JS to scan — 2 module(s): the inline <script type="module">
             in index.html, modules/tuning/scala-tuning-engine/js/tuning-panel.js
  ALL CHECKS PASS — 1 localized plugin(s)

i18n-zh-lint --plugin O-Wind              exit 0
  O-Wind                        214  161    ·  ·  ·  ·  ·  ·  ·  ·  ·  ·      0
  BELOW SHIP BAR — entries at reviewed:'mt': 0
  straight copies zh === en (info): 0   termNote exemptions (info): 4
  GATE PASSED — exit 0. 0 findings across 1 plugin(s).

check-ui-labels --plugin O-Wind           exit 0
  8 x PASS: [7][GEOMETRY DIFF][fr|zh-Hans] no non-label element moved   (4 states x 2 arms)
  PASS: [7][GEOMETRY DIFF][zh-Hans] the visible element SET is identical in English and zh-Hans
  PASS: [8b][zh-Hans] no label intersects a NON-label element it cleared in English
  PASS: [2][vacuity][zh-Hans] the zh-Hans pass actually rendered — 67/67 labels (100%)
  PASS: [2][vacuity][zh-Hans] keyed ATTRIBUTES actually changed language — 23/23
  coverage: 90 of 97 [data-i18n] elements were VISIBLE in at least one state
            10 never became visible                                  [UNCHANGED, s71 D5's set]
  == ALL CHECKS PASSED ==

ui_tip_render_check.js                    exit 0
  sweeping 3 language(s): en -> fr -> zh-Hans
  placement census over 159 hovers (every swept language): 42 left, 18 above, 0 on rail
  == ALL CHECKS PASSED ==   (1109 PASS)                              [was 788]

boot-all-uis --plugin O-Wind --strict-tips   exit 0
  clean: 1/1   DEAD bindings: 0   late bindings: 0

measure-ui --report all
  undeclared-font: 0 finding(s)      [was 155 Han nodes on 4 stacks; bareArial 25 -> 17]
  line-height-normal: 21 finding(s)  [was 62; all 21 evidenced non-movers, enH == zhH]
  wrap-count: 0 finding(s)
  svg-font-attr: 0 attribute carrier(s), 0 finding(s)
  identity: 1920 nodes / 640 distinct DOM keys / 221 distinct display ids
measure-ui --verbose:   did not resolve|unresolved|skipped state = 0   (all THREE states)

i18n-zh-backtranslate --plugin O-Wind
  O-Wind   214  214   0 mt   214 bt   0 native   0 none
  TOTAL   5441 4106   0 mt  4106 bt      BELOW SHIP BAR: 0

C++:  comment-stripped 'zh-Hans' count = 2
      languageCode: THREE-WAY   languageIndex: THREE-WAY   (matched by NAME, not swept)
      Han under Source/**/*.{h,cpp}: NONE
      positive control on the same probe: FIRED (Resources/ui/js/i18n.js)

Repo-wide, unchanged from baseline:
  check-i18n     exit 0   ALL CHECKS PASS — 43 localized plugin(s)
  i18n-zh-lint   exit 2   0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read
                          (zh-Hans entries checked: 2933 -> 3094)
  i18n-fr-lint   exit 2   plugins with findings: 0 / 44   (1 could not be read)

Two-arm CMake reader, run unconditionally on ALL FIVE wave plugins:
  O-GrainScatter 2.8.0 | O-Wind 1.20.0 -> 1.21.0 | O-Contrabass 1.9.0
  O-Reed 1.5.0 | O-Bowed 1.8.0
  one-arm == two-arm on all five.  NO set()-variable form anywhere in this wave.
  O-Wind's is a QUOTED literal a naive digit grep misses.

modules/ dirty: 0     modules/ in either commit: 0
O-Strata in either commit: 0    libs/SAF in either commit: 0
PLUGINS.md in either commit: 0    tags at HEAD: 0

Build:  ./scripts/build-and-install.sh O-Wind   exit 0, 52s
        VST3 + AU installed, -dev only, 0 alternate-variant sweep warnings
        Installed CFBundleShortVersionString: 1.21.0  (both bundles)
auval:  NOT RUN — deferred to Task 6 by design
```

---

## 10. Carry-forwards for Tasks 3–5 (O-Contrabass, O-Reed, O-Bowed)

**The shared-module method, which Task 1 could not exercise at all:**

1. **The 37 module rows copy cleanly and mechanically.** Extract each key's `'zh-Hans'` block from
   O-Bassoon by brace-matching from the key line, and insert it verbatim. All 37 came across
   byte-identical on the first attempt, `termNote` and all (`label.genDivisions` carries a long
   one). **Derive the 37-key list from the module's own markup** —
   `grep -rhoE 'data-i18n(-aria)?="[^"]+"' modules/tuning/scala-tuning-engine/` returns **38**,
   and `label.xxx` inside a doc comment is the one to drop.

2. **The insertion needs ONE comma rule and nothing else.** Insert before the entry's matching
   closing `}`; add a `,` unless the text immediately before it already ends in `,` or `{`. Use
   the inline form (` , 'zh-Hans': { … }`) for single-line entries and an indented line for
   multi-line ones. 161 entries, zero parse errors.

3. **The `1.11` ratio TRANSFERS on O-Wind — verify it anyway.** Every one of its eleven selectors,
   including all three button classes, measured a content box that 1.11 reproduces inside 0.5px.
   O-GrainScatter's `.toggle` was the exception, not the rule. **But `.interval-list-header` is
   TWO line boxes in English on every consumer** (s71's own commit message records en 2 lines /
   fr 1 line on all four) — derive per LINE or the pin doubles.

4. **Use all FIVE tail selectors on O-Contrabass, O-Reed and O-Bowed.** O-Wind's four-selector
   rule is a per-plugin divergence: only O-Wind's `.viz-btn` already declares a face
   (`#tuning-container .viz-btn` at index.html L761-763). The other three read 30 form-4 nodes
   where O-Wind reads 25, and the extra five ARE the `.viz-btn`.

5. **`undeclared-font` will NOT credit the two `<select>`s** (§7.4). Expect the screen to name
   6 nodes where the rule covers 8, and say so rather than reporting a smaller number.

6. **O-Reed and O-Bowed need the `#tuning-container .tuning-panel` override O-Wind and
   O-Contrabass already have** (J15, 116 nodes each on `-apple-system`). O-Wind's override is at
   index.html L709-718 and is the shape to copy — but it changes the LATIN arm on those two, so
   re-run `check-ui-labels` on en and fr and be ready to fall back to appending the tail to a copy
   of the system stack.

**Method and tooling corrections that transfer:**

7. **The plan's duplicate-key scan is broken on any three-language table** (§7.5). Use the
   corrected form — exclude the members of the declared `LANGUAGES` — or it reports `zh-Hans` and
   reads like a real collision.

8. **The ratio-block verify greps `-A14` and must be `-B14`** (§7.6). It reads 0 on a correct file
   AND on the transfer source.

9. **Verify gate sites by PATTERN, not by the plan's line number** (§7.3). J4's form-6 lines are
   wrong for O-Wind by 100+ lines while every other cited line is exact; assume the same for
   O-Reed's and O-Bowed's J4 rows, whose count (3 and 4) is probably right.

10. **Fire the derive-or-abort control TWICE** — an empty list AND a one-member list. The second
    proves the guard on the walk is independent of the guard on the list, and it is the one that
    catches a vacuous sweep.

11. **The ingest refuses in THREE ordered steps** (§7.9): reverse `--provenance` → forward
    provenance → id join. Supply `--provenance` to both refusal controls or control 2 fires for
    control 1's reason and proves nothing.

12. **Use `LC_ALL=C sort` for the zh-column sha256.** The default locale's `sort` is unstable
    across runs for equal-length Han strings and produces a spurious mismatch.

13. **Split the batch into independently-dispatched chunks — it is what finds the collisions.**
    Both Task 1's `stutterGate` and this task's `反向气束` were found only because one Chinese
    string reached two different readers. Send the caption and its own tooltip title to DIFFERENT
    chunks deliberately.

14. **Glossary roots that render the WRONG SENSE are a class, and it is not always a defect.**
    Task 1's `dist lpf` root is simply wrong; O-Wind's `flutter` → 快抖 is *correct for tape* and
    wrong for a flute. The first wants a glossary fix, the second wants a per-page `termNote`.
    **Check O-Reed and O-Bowed for a `Flutter` caption.**

15. **The lint requires the glossary ROOT, not a compound containing it** (§7.10). 低频 fails
    against `'low': ['低']`. Check `TERMS` for whole-sentence roots too — `label.tuningFailed` has
    one, and all four consumers carry that key.

16. **No string literal in `i18n.js` may contain `<`** (`check-i18n [9]`). A `termNote` that
    mentions `<select>` or `<button>` fails the file.

17. **The tuning state mounts 30 module captions, not 13** (§7.7). Use 0 → 30 as the state-EFFECT
    figure on every consumer, and read the 7 that never enter the DOM as the difference.

---

## 11. Deferred / out of scope

- **auval — NOT RUN**, deferred to Task 6's single cold sweep by design. O-Wind's triple is
  `aumu OWnd OuDv` (`IS_SYNTH TRUE`). `auval -v` takes **THREE UNQUOTED WORDS**.
- **`PLUGINS.md` NOT touched.** Its O-Wind row reads **1.20.0** and Task 6 must set it to
  **1.21.0** (one minor). O-GrainScatter's is two minors stale at 2.6.1 → 2.8.0.
- **The glossary is NOT edited.** Task 1's `'dist lpf': ['失真低通']` → `['距离低通']` at
  `scripts/i18n-zh-glossary.js:433` remains open, as a corpus-level item. **O-Wind adds no
  glossary defect** — its `flutter` → 快抖 row is correct for the sense it was written for and is
  handled by a per-page `termNote` instead (§2).
- **The 17-of-37 module coverage hole is NOT closed** (s71 D5). 10 never-visible + 7 never in the
  DOM. Closing it needs states that change the generator type and select the Rotation
  visualisation; this wave does not budget them.
- **`reviewed: 'native'` OPEN** on all 214 rows — no native Chinese reader exists on this project.
  Disclosed in the CHANGELOG and printed by lint rule R1 on every run.

## Self-Check: PASSED

- `plugins/O-Wind/Resources/ui/js/i18n.js` FOUND, `plugins/O-Wind/Resources/ui/index.html` FOUND,
  `plugins/O-Wind/Source/PluginProcessor.h` FOUND, `plugins/O-Wind/tests/ui_tip_render_check.js`
  FOUND, `plugins/O-Wind/CMakeLists.txt` FOUND, `plugins/O-Wind/CHANGELOG.md` FOUND.
- Commit `020409f8` FOUND, commit `bff367f6` FOUND.
- `~/Library/Audio/Plug-Ins/VST3/O-Wind-dev.vst3` FOUND at 1.21.0;
  `~/Library/Audio/Plug-Ins/Components/O-Wind-dev.component` FOUND at 1.21.0.
