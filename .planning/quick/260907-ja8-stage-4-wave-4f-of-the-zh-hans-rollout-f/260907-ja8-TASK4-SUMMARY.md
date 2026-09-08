---
phase: quick-260907-ja8
plan: 01
task: 4
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4f, O-Reed, scala-tuning-engine, system-font-stack, enumeration-form-1, enumeration-form-7, native-fn-gap]
status: complete
requirements: [ZH4F-04, ZH4F-06, ZH4F-07, ZH4F-09, ZH4F-12]
dependency_graph:
  requires:
    - "260907-ja8 Task 1 (O-GrainScatter, b534636b + a92b7980)"
    - "260907-ja8 Task 2 (O-Wind, 020409f8 + bff367f6)"
    - "260907-ja8 Task 3 (O-Contrabass, 5a216066 + b69ca8ea)"
    - "260906-s71 (shared tuning panel localized, d848337a)"
  provides: ["O-Reed three-language UI at reviewed:'bt', v1.6.0, built and installed"]
  affects: ["Task 5 (O-Bowed)", "Task 6 (PLUGINS.md + one cold auval sweep)"]
tech_stack:
  added: []
  patterns:
    - "a .tuning-panel override that moves a consumer OFF the shared module's -apple-system stack, chosen on a measured Latin arm rather than on intent"
    - "re-measuring a French-era min-width floor after a FACE change, not only after a language change"
    - "derive-or-abort over a table-exported LANGUAGES list with a second, independent guard on the walk"
    - "a paired literal-argument sweep call generalized to a loop over the derived list"
    - "a correction round dispatched to a different model on the corrected row plus decoys"
key_files:
  created: []
  modified:
    - plugins/O-Reed/Resources/ui/js/i18n.js
    - plugins/O-Reed/Resources/ui/index.html
    - plugins/O-Reed/Source/PluginProcessor.h
    - plugins/O-Reed/tests/ui_tip_render_check.js
    - plugins/O-Reed/CMakeLists.txt
    - plugins/O-Reed/CHANGELOG.md
decisions:
  - "The .tuning-panel override was KEPT and the stale octave-stretch floor re-measured, rather than falling back to the system stack: the fr movement was caused by a floor measured against the OLD face, which is R8's exact case, not by the override being wrong."
  - "The 37 shared scala-tuning-engine rows were COPIED byte-for-byte from O-Bassoon at reviewed:'bt' and excluded from the blind batch — R1's one stated exception."
  - "label.knob.flutter takes 花舌 with a termNote rather than the glossary root 快抖, which renders the tape sense — the same exemption O-Wind already ships."
  - "tip.growl's title was RE-AUTHORED 喉音量 -> 喉音强度 after the blind read recovered it as 'Throat Volume' on a page carrying an output level and a mouthpiece volume."
  - "The XY-marker I18N_EXEMPT rationale was read, classified as a look-alike that stays true under three languages, and LEFT."
metrics:
  duration: one session
  completed: 2026-09-07
actuals:
  tokens: 79000
  tasks: 1
  commits: 2
---

# Task 4 — O-Reed to Simplified Chinese (wave 4f, the first system-stack panel)

**Commits (path-scoped to `plugins/O-Reed`, on `main`):**

| | hash | what |
|---|---|---|
| 1 | **`9d83e7a7`** | the `'mt'` table, the gate repair, the fonts and the geometry — 4 files, +575 / −131 |
| 2 | **`f3737f53`** | promotion to `reviewed: 'bt'`, version 1.6.0, CHANGELOG — 3 files, +159 / −97 |

HEAD at start: **`b69ca8ea`** (the plan's `d37cdcc0` is stale by five commits — Tasks 1–3
had landed). No tag created; `git tag --points-at HEAD` = 0 before and after.

---

## 1. Preconditions — all nine FIRED on the tree as found

| # | check | measured | plan predicted |
|---|---|---|---|
| a | `i18n-zh-lint --self-test` | **`SELF-TEST: 10/10`**, exit 0 | ✓ |
| b | `check-i18n --plugin O-Reed` | exit 0; `[1] … got ["en","fr"]`; **`36 I18N + 96 LABELS`**; `[12] 2 module(s)` naming the inline `<script type="module">` **and** `modules/tuning/scala-tuning-engine/js/tuning-panel.js`, **not** `preset-manager.js` | ✓ verbatim — `CMAKE_MODULE_JS_SCOPE` intact (s71 D2) |
| c | `i18n-zh-lint --plugin O-Reed` | exit 0, 0 findings | ✓ |
| d | `check-ui-labels --plugin O-Reed` | exit 0, `== ALL CHECKS PASSED ==`, **`111 of 84`**, **10 never-visible**, first entry `#truekeys-view>div.tk-hint:nth-child(1)` then six `#library-filter>option` and three `#generator-type>option` | ✓ verbatim, s71 D5's set exactly |
| e | `plugins/O-Reed/tests/ui_tip_render_check.js` | exit 0, `== ALL CHECKS PASSED ==` — **fired BEFORE the flip**, which is what makes every later red provably the flip's | ✓ |
| f | 37 module keys in en/fr; duplicate-key scan | **37/37 present, duplicates NONE**, 132 entry keys | ✓ |
| g | git | `main`, tags at HEAD **0**, **no entry under `plugins/O-Reed/` or `plugins/O-Bowed/`** | **corrected precondition met** (see §7.1) |
| h | repo-wide `i18n-zh-lint` | **exit 2**, `0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read` | ✓ — J1's discriminator verbatim |
| i | Tasks 1–3 committed and installed | all six commits present; O-GrainScatter **2.8.0**, O-Wind **1.21.0**, O-Contrabass **1.10.0** on disk | ✓ |

**Two-arm CMake reader, run unconditionally (R9):** `VERSION "1.5.0"` at raw **L11**, **quoted**.
One-arm reads `1.5.0`, two-arm reads `1.5.0` — **both arms agree**. After the bump both read
**`1.6.0`**. The wave's third quoted literal; no `set()` variable form.

---

## 2. What shipped

**168 emitter rows** over **132 entries** — 36 `I18N` × (title + body) = 72, plus 96 `LABELS`.
Corpus **4292 → 4460** rows of 5441. O-Reed now reads `168 / 168 zh / 0 mt / 168 bt`.

- **131 rows authored here at `reviewed: 'mt'`** (95 entries), promoted only after the reverse
  read, in its own commit.
- **37 rows arrived at `reviewed: 'bt'` by byte-for-byte copy** from
  `plugins/O-Bassoon/Resources/ui/js/i18n.js` — R1's one stated exception — **excluded from the
  blind batch**, with the exclusion stated in the commit.

**The `LANGUAGES` flip and the module column landed in the SAME edit** (J6), in one script that
refuses to write unless all 37 blocks extract and the flip's single occurrence is found.

### The D8 arithmetic detector, fired before and after — it did not recur

| detector | before | after |
|---|---|---|
| `check-i18n` LABELS count | **96** | **96** |
| `check-i18n` I18N count | 36 | 36 |
| duplicate-key scan (corrected form, `LANGUAGES` members excluded) | **NONE**, 132 entry keys | **NONE**, 132 entry keys |

All 37 module `'zh-Hans'` blocks verified **byte-identical to O-Bassoon's** by brace-matched
extraction and string comparison, before and after the promotion, all 37 at `reviewed: 'bt'`.

### The 37-key list was DERIVED, not transcribed

`grep -rhoE 'data-i18n(-aria)?="[^"]+"' modules/tuning/scala-tuning-engine/` returns **38**;
dropping `label.xxx` (inside a doc comment) gives **37**. Present **37/37** in O-Reed and in
O-Bassoon. Task 2 c/f 1 and Task 3's method transferred with **zero** parse errors and zero
manual edits.

### One termNote authored, and it is carry-forward 13 landing

`label.knob.flutter` "Flutter" → **花舌**, not the glossary root **快抖**. Task 2's carry-forward
13 said to check both remaining consumers for a `Flutter` caption; O-Reed has one, on
`.knob-control[data-param="flutterTongue"]`, and its own tooltip title is "Flutter Tongue" whose
root IS 花舌. 快抖 renders the tape wow-and-flutter sense and would name a control this plugin
does not have. The termNote cites O-Wind's identical exemption so the two plugins name one
technique one way.

The other termNote the lint reports (`label.genDivisions`) is **inherited verbatim with its module
row** and is not this task's.

---

## 3. Stale copy — corrected FIRST, file re-read, then authored (C7)

The comment-stripped probe reported **5** code hits where the plan predicted 4 (§7.2), across
three substantive sites plus a sixth the probe structurally cannot see (§7.3).

### J8 — `tip.langSelect` promised the Tuning tab stays English. FALSE since v1.5.0.

- en: *"…stay in English in both languages, **and so does the Tuning tab — its panel comes from a
  shared module that is not part of this plugin.**"*
- fr: *"…**tout comme l'onglet Accord : son panneau provient d'un module partagé qui n'appartient
  pas à ce plugin.**"*

s71 localized that panel at module v3.1.0 and this plugin's own v1.5.0 shipped all 37 rows, so
the clause has been a shipped falsehood **in French** since the release that made it false. Both
halves removed. **The comment above it described the list as this page's THREE standing English
regions and was false with it** — corrected to two, with the reason written down. The other two
regions (the six dropdowns' option words, the fifteen XY-pad markers) are still true and stay.

### J10 — the same body enumerated the selector's options. Removed.

en *"English and French are available."* / fr *"L'anglais et le français sont proposés."* The
surrounding sentence was re-worded from *"in both languages"* / *"dans les deux langues"* to
*"whichever language is chosen"* / *"quelle que soit la langue choisie"*, so it holds for any
number.

### J9 — `tip.gearBtn` claimed the settings panel holds the language and nothing more. FALSE since v1.4.0.

- en: *"…**That is all it holds:** the captions on this page and this hover help change with it…"*
- fr: *"…**Il ne contient rien d'autre :** les libellés…"*

`#tips-toggle` sits inside `.settings-popover`, in a second `.settings-row` beside `#lang-select`
(markup raw L922 popover / **L949-953** the toggle) — **verified in the markup, not inferred** —
and `tip.tipsToggle` was added at v1.4.0. The comment above asserted *"O-Reed has no hover-help
on/off toggle — not in C++, not in localStorage"*; both body and comment corrected to name both
controls. The rest of the body — that the choice is kept with the session — is true and stays.

**check-i18n's repo-wide hover-help clause has been PASSING for this plugin throughout**, which is
the independent proof the switch exists and the body was the thing that was wrong.

### The CHECKED NON-DEFECT — read, classified, LEFT (J10, carry-forward 12)

The `I18N_EXEMPT` rationale for the fifteen XY-pad markers (raw **L1187**) reads *"…so the whole
set stays English rather than being split across **two languages**."* That is a statement about
one exempt set, not about the selector, and it **stays true under three languages**. Left
unchanged. Task 3's carry-forward 12 predicted exactly this and it was correct.

**Three further "and nothing else" hits in the raw file are unrelated senses and were left**:
raw L122 *"the canon writes the tip ATTRIBUTES and nothing else"*, raw L782 *"hitting Ddk in both
languages and nothing else in either"* (a CLIFF-2 geometry hit-set), and raw L1191 *"it reaches an
`<option>` and nothing else"* (the exemption's scope). The probe is a substring alternation and
cannot tell these from a J9 site; a reflexive deletion sweep would have destroyed three correct
comments.

**After correction the code-scoped probe reads 0.**

---

## 4. The gate — form 1 AND form 7, repaired BEFORE the flip

`tests/ui_tip_render_check.js`, 684 lines. Repaired in an uncommitted working state and **run
green at two languages before `LANGUAGES` gained a third member**, so no commit in this task's
history contains a table whose third language makes its own gate red.

### N12 enumeration census — O-Reed, before → after

| form | before | after | disposition |
|---|---|---|---|
| **1 — `LANGUAGES.join(',') === 'en,fr'`** | **1** (raw **L252**, untagged) | **0** | replaced by shape + derive-or-abort |
| 2 — string `'en,fr'` | 1 (inside form 1) | **0** | went with it |
| 3 — array-literal walk | 0 | 0 | absent |
| **4 — computed / UA-face form controls** | **30 nodes** | **0** | a CSS form, see §5 |
| 5 — `Map` read with a literal key | 0 | 0 | absent |
| **6 — per-language object property access** | **4** (raw L495, L515, L516, L656) | **4** | **RESTORE / SOURCE-scoped, deliberately LEFT** |
| **7 — paired literal-argument call** | **2** (`sweep('en')` raw **L480** / `sweep('fr')` raw **L509**) | **0** | generalized to a loop |
| `__setLanguage(l), 'fr'` | 1 (raw **L505**) | 0 | driven from the loop variable |
| `frLang === 'fr'` selector assertion | 1 (raw **L508**) | 0 | driven from the loop variable |
| printed height comparison naming two languages | 1 (raw **L510**) | 0 | prints each language against the baseline |
| `LANGUAGES` mentions in CODE | 3 | **8** | publish, destructure, `EN`, shape assertion, abort, `REST`, walk guard, log |
| g5l inventory grep sees this file? | **yes** | **no** | off the list, and the comment does not put it back (C8) |
| assertions | **539** at two languages | **757** at three | |

### The three repairs

1. **Derive-or-abort.** The loader at raw **L172** already published
   `{ I18N, TIP_BINDINGS, LANGUAGES }` and raw **L249** already destructured all three — nothing
   to add to a loader, exactly as J3 said. The joined-string equality became a shape assertion
   (non-empty array opening with the source language) plus `process.exit(1)` with a refusal
   message. **No fallback literal.**
2. **A SECOND, INDEPENDENT guard on the WALK.** `const REST = LANGUAGES.slice(1)` with its own
   `check()` and its own refusal. A one-member list satisfies the shape assertion and then
   compares the page to itself, passing every byte-equality assertion for the wrong reason.
3. **The paired call became a loop.** `sweep(lang)` was already language-agnostic inside — raw
   **L435** reads `const entry = (I18N[key] || {})[lang];` — so the whole defect was the caller.
   The switch, the selector assertion and the printed comparison all moved inside the loop and
   are driven from the loop variable. Live: `sweeping 3 language(s): en -> fr -> zh-Hans`.

**No page-state call sits between O-Reed's two sweeps** (unlike O-Wind's `showTab('sound')`), so
nothing extra had to be preserved inside the loop body — checked rather than assumed.

### Derive-or-abort control — fired TWICE, against the LIST and against the WALK

| plant | expected | observed |
|---|---|---|
| `LANGUAGES = []` | the shape assertion refuses | `FAIL: LANGUAGES derives from the table as a non-empty array opening with the source language — got []` → `REFUSING to sweep a guessed language list.` **exit 1** |
| `LANGUAGES = ['en']` | shape PASSES, the walk guard refuses | `PASS: … got ["en"]` then `FAIL: the derived list carries at least one language besides the source language — got ["en"]` → `REFUSING to compare the page to itself.` **exit 1** |

The second is the load-bearing one and it is the plant that catches a vacuous sweep. Both plants
reverted by **targeted `perl -i` edit on one line, never `git checkout --`**; sha256
`a00d2304…3ba1b067` byte-identical on all three readings (before, after plant 1, after plant 2).

### The four form-6 sites LEFT, classified at their own sites

Raw **L495** (`I18N['tip.breath'].en.t`, inside NC-3), raw **L515-516** (`.en.t` and `.en.b`,
after the switch back), raw **L656** (`.en.b`, after the NC-1 plant restore). L495 is
**SOURCE-LANGUAGE-scoped** — the block runs before any language switch, so the page is in the
source language by construction. The other three are **RESTORE-scoped** — the line above each
writes the source language by construction. Generalizing any of them would break the restore
semantics the assertion exists to prove. A prose comment now says so at each site.

---

## 5. Fonts — 111 Han nodes, 0 of which resolve without a named CJK face

**8 `font-family` declarations: 1 named stack + 7 `font-family: inherit`** — exactly J11, both
halves. The computed `ff` census over **491** visible English nodes reproduced J13's three
populations **exactly**:

```
345  Garamond, Georgia, "Times New Roman", serif     the house stack (raw L73)
116  -apple-system, "system-ui", sans-serif          THE ENTIRE TUNING PANEL
 30  Arial                                           the panel's undeclared form controls
```

Once the table landed, **111 visible zh nodes carried Han and every one resolved without a named
CJK face**: 91 on the house stack, 11 on bare Arial, **9 on the system stack**.

### Treatment, per site

| site | treatment | why |
|---|---|---|
| the house stack (raw L73) | **tail only** | Georgia and Times New Roman both installed; Garamond installed nowhere on this machine |
| **NEW** `#tuning-container .tuning-panel` | **face + tail** | the override this plugin never had — 116 nodes on the shared module's `-apple-system` stack, see below |
| **NEW** five-selector `#tuning-container` rule | **face + tail**, Latin first | `.viz-btn`, `.tuning-file-btn`, `.generator-btn`, `.generator-type-select`, `.library-filter-select` |

**FIVE selectors, not O-Wind's four** (Task 2 c/f 4, Task 3 c/f 3, both confirmed): O-Reed's five
`.viz-btn` carry no face of their own, which is why it reads 30 form-4 nodes where O-Wind reads 25.

### The 30 form-4 ids are EXACTLY J11's measured list

```
#tonic-down  #tonic-up                     2 arrow glyphs
input.interval-input  x12                  numeric
button.viz-btn  x5                         CJK carriers
#library-filter  #generator-type           <select>, Han options
#octave-stretch  #gen-divisions  #gen-period   numeric
#btn-load-scl #btn-load-kbm #btn-save-scl #btn-save-kbm #btn-export-html   CJK carriers
#btn-generate                              CJK carrier
```

The 11 Han-bearing ones measured after the table landed:

```
.viz-btn x5           圆周 极坐标 矩阵 真实键位 旋转
.tuning-file-btn x5   载入 .scl / 载入 .kbm / 保存 .scl / 保存 .kbm / 导出 HTML
.generator-btn x1     生成
```

`measure-ui` reports bare Arial **30 → 17**; the 13 that left are the 11 Han buttons plus the 2
`<select>`s. **`undeclared-font` does not credit the two selects** (Task 2 §7.4, Task 3 §7.4,
confirmed a third time): its `han` flag is computed over a node's own text plus `data-tip` /
`data-tip-title` / `aria-label`, and a collapsed select's selected-option text is none of those.
They are in the rule anyway, on a decision about what the control **paints** —
`#library-filter`'s six category options and `#generator-type`'s three type options are all Han.
**The rule covers 13 nodes where the screen can only name 11.**

**The other 17 are the measured NEGATIVE case (N3):** twelve `.interval-input` numerics,
`#octave-stretch`, `#gen-divisions`, `#gen-period` and the two arrow glyphs `#tonic-down` /
`#tonic-up`. None renders Han, and the shared module gives none of them an `aria-label` or a
`data-tip`, so N3's tip-anchor discriminator answers **no** for every one.

### J15 — the 116-node system-stack panel, and the LATIN-ARM decision

`modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` **L66** declares
`font-family: -apple-system, BlinkMacSystemFont, sans-serif` on `.tuning-panel`. O-Wind and
O-Contrabass override it; **O-Reed did not**, so 116 nodes rendered the whole panel through a
stack naming no Han face and terminating at a bare `sans-serif`.

**The measurement that decided it, in three runs:**

| run | `undeclared-font` | nodes on `-apple-system` (en) | `check-ui-labels [7][fr]` |
|---|---|---|---|
| table landed, no fonts | **111** | **116** | **0 moved** (green) |
| + house tail, `.tuning-panel` override, 5-selector rule | **0** | **0** | **FAIL — 1 moved**, in 2 of 5 states |
| + the re-measured floor | **0** | **0** | **0 moved** (green) |

**The fr mover was `#octave-stretch  dx=2.0 dy=0.0 dw=-2.0 dh=0.0` and nothing else** — one
element, one axis, two states.

**Root cause, measured rather than assumed.** `#tuning-container .octave-stretch-label` carries a
`min-width: 51.84px` floor landed by `d848337a`, which was the **French box measured while the
panel body still rendered through the system UI face**. Under Garamond/Georgia/Times New Roman
the same French caption measures **53.86px**, so the floor stopped covering the arm it was written
for and the slider beside it — `.octave-stretch-row` gives it `flex: 1` — absorbed the 2px.

| arm | `.octave-stretch-label` before the face change | after |
|---|---|---|
| en | 51.83 | **51.83** |
| fr | 51.83 (at the floor) | **53.86** |
| zh | 51.83 | **51.83** |

**DECISION: keep the override, re-measure the floor.** The plan's fallback — appending the tail to
a copy of the system stack — was rejected on the evidence, and the reasons are:

1. **The measured cause is not the override.** It is a pin measured against one face being read as
   safety under another, which is **R8 in its exact stated shape**. R8 already requires
   re-measuring every French-era floor; the fallback would have left a stale pin in place and
   simply hidden it.
2. **The fallback does not close the defect it is offered for.** Appending a tail to
   `-apple-system` leaves the panel's Latin on the system UI face, keeps O-Reed visually divergent
   from the two siblings that already override, and leaves a fourth consumer shape in a module
   that now has three.
3. **The corrected number corroborates itself.** 53.86px is **exactly O-Contrabass's measured
   `.octave-stretch-label`** (Task 3 §6D: 53.86 on all three arms) — the two now agree because
   they now render the same face. An independent plugin arriving at the same figure is stronger
   evidence than a single measurement.

Cost: **one number**, on a selector R8 required re-reading anyway. Benefit: 116 nodes off the
document-language fallback, and all four consumers converge on one shape.

**`modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` is BYTE-UNCHANGED**, asserted:
`git status --short -- modules/ scripts/` reads **0**, and neither commit touches `modules/`.

Final: **`undeclared-font: 0 finding(s)` against 111 Han nodes, 0 of which resolve without a
named CJK face — non-vacuous.** Nodes on the shared module system stack: **en 0, zh 0** (was 116
on en). `grep -c 'PingFang SC'` = **3** (the house stack, the panel-body override, the
five-selector rule).

---

## 6. Geometry — two ratio blocks, one re-measured floor, 0 moved on all three arms

First zh run after the fonts: **`[7][zh-Hans]` FAIL, 266 and 291 moved** in two states, 69 in two
others. The fr arm carried the single `#octave-stretch` mover above and nothing else.

### A. The eleven-selector `1.11` RATIO block — transferred, and RE-DERIVED per selector

Task 1's carry-forward 2 says M8's line-box table is leaf-scoped and a `<button>` need not match
it, so the transferred ratio was verified against **O-Reed's own measured English content boxes**,
padding and border subtracted FIRST and the line count checked:

| selector | fs | en box | pad+bd | lines | content/line | 1.11 × fs | Δ |
|---|---|---|---|---|---|---|---|
| `.interval-list-header` | 10 | 24.00 * | 0 | **2** | **11.00** | 11.10 | 0.10/line |
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

\* already sitting on the `min-height: 24px` floor; its natural English box is **two** line boxes
of 11.00 in this 122px column, so the ratio is derived PER LINE. A naive box-over-font-size would
have read 2.4 and more than doubled the pin — N10/N11's trap, live on a third consumer.

**All eleven inside `check-ui-labels`' 0.5px tolerance, so 1.11 TRANSFERS on this plugin too.**
**`.viz-btn` is 9px on O-Reed** — the same as O-Contrabass, and **not** O-Wind's 10px. Task 3's
carry-forward 4 is confirmed: the per-selector check is not ceremonial, and the wave has now seen
two different values for one selector.

The zh boxes on those eleven, before the pin: 10.00 → **13.00** at 9px, 11.00 → **14.00** at 10px,
24.00 → **27.00** on the 9px buttons, 27.00 → **30.00** on the 10px one. A uniform +3px.

### B. Ten PAGE-LOCAL `line-height: normal` leaves, each pinned at its own content box

| selector | fs | en box | pad+bd | content | ratio | matches M8? |
|---|---|---|---|---|---|---|
| `.tab-btn` **BUTTON** | 11 | 22.00 | 10 | **12.00** | 1.0909091 | ✓ |
| `.section-title` ×7 | 11 | 12.00 | 0 | 12.00 | 1.0909091 | ✓ |
| `.xy-pad-label` | 11 | 12.00 | 0 | 12.00 | 1.0909091 | ✓ |
| `.settings-label` ×2 | 10 | 11.00 | 0 | 11.00 | 1.1000000 | ✓ |
| `.toggle-label` | 10 | 11.00 | 0 | 11.00 | 1.1000000 | ✓ |
| `.dropdown-label` ×5 | 9 | 10.00 | 0 | 10.00 | 1.1111111 | ✓ |
| `.xy-val-item > span` ×2 | 9 | 10.00 | 0 | 10.00 | 1.1111111 | ✓ |
| `.xy-axis-label` ×2 | 8 | 9.00 | 0 | 9.00 | 1.1250000 | ✓ |
| `.effects-placeholder h2` | **24** | 27.00 | 0 | 27.00 | **1.1250000** | **NOT IN M8** |
| `.effects-placeholder p` | **14** | 16.00 | 0 | 16.00 | **1.1428571** | **NOT IN M8** |

**Two font sizes are this page's contribution to M8's measured table:** 14px → 1.1428571 (the same
figure as 7px) and 24px → 1.1250000 (the same figure as 8px). Both from the FX placeholder.

### C. ONE root cause, 266 symptoms

Every `[7][zh-Hans]` entry with a `dw` or `dh` other than zero — across all five states — was a
**single** row:

```
#tab-instrument>…>div.xy-pad-container   dx=0.0 dy=0.0 dw=0.0 dh=4.0
```

Everything else was `dy` cascade. `.xy-pad-container`'s first child is `.xy-pad-label` (11px,
`line-height: normal`, `margin-bottom: 4px`), whose Han line box ran 12 → 16. Task 1's
carry-forward 3 in its sharpest form: **read the `dh` column and find the one absorbing container
before pinning anything; do not treat the count as the work.** 266 entries, one defect.

### D. The three `d848337a` floors, re-measured on the zh arm

| selector | en | fr | **zh** | verdict |
|---|---|---|---|---|
| `.tonic-label` | 41.58 × 9.98 | 41.58 × 9.98 | **41.58 × 9.98** | **HOLDS** at 41.59px |
| `.octave-stretch-label` | 53.86 × 9.98 | 53.86 × 9.98 | **53.86 × 9.98** | **RE-MEASURED** 51.84 → 53.86px (§5) |
| `.interval-list-header` | 122.00 × 24.00 | 122.00 × 24.00 | **122.00 × 24.00** | **HOLDS** at 24px |

The heights read **9.98 (= 9 × 1.11)** rather than the face's `normal` box, so the ratio block is
live on them. **Chinese is shorter than French on all three**, so the fr-widest floor covers zh
exactly as it did on O-Bassoon, O-Wind and O-Contrabass — **the wave's fourth confirmation, and
the first time one of the three needed a new number, for a reason that was not the language.**

### E. R8 fired on every selector BEFORE anything was appended

None of the 21 selectors touched carried a prior `line-height`. The five that exist on this page
are on `.knob-label` (1.1) and three unrelated rules. Pin census, comment-stripped:

| | min-width | min-height | line-height | letter-spacing | white-space |
|---|---|---|---|---|---|
| **before** | 3 | 3 | 5 | 11 | 3 | 
| **after** | 3 | 3 | **6** | 11 | 3 |

**Exactly the J14 census on all five numbers before.** After: +1 line-height rule-block and the
`min-width` count **unchanged at 3** — the octave-stretch floor was REWRITTEN, not added, which is
the arithmetic proof no other pin moved. `letter-spacing` and `white-space` untouched.

### Result

| screen | planning-time | table landed, no fonts | **final** |
|---|---|---|---|
| `undeclared-font` | 0 — **a vacuum** (J13) | **111** | **0** against 111 Han nodes |
| `line-height-normal` | 0 — vacuum | 46 | **1** |
| `wrap-count` | 0 — vacuum | 1 | **2** — see §7.5 |
| `svg-font-attr` | 0 carriers / 0 findings | 0 / 0 | **0 / 0** — inert on this plugin (Task 3 c/f 8) |
| `check-ui-labels [7]` fr | 0 moved | 0 moved | **0 moved** |
| `check-ui-labels [7]` zh | — | — | **0 moved** |

**`identity: 1689 nodes / 563 distinct DOM keys / 150 distinct display ids`** — the display-id
figure matches the plan's live table (150) exactly.

### The single `line-height-normal` residual — an evidenced NON-MOVER (N1)

| node | fs | enH | zhH | why no pin can change it |
|---|---|---|---|---|
| `div.bore-viz-placeholder` | 11 | **110.00** | **110.00** | a flex-centred box at a fixed 110px height; its own line box is invisible to its geometry |

O-Reed's residual count is **1**, against O-Wind's 21 and O-Contrabass's 9. The reason is that
this page's `.knob-value` readouts carry no `data-tip` and no `aria-label`, so N1's canonical
"Han in the attribute only" class — which produced O-Contrabass's three canvases and O-Wind's
sixteen readouts — has no instance here.

### State-EFFECT assertions — the eval state AND all three `click` states (wave 4e N1)

| state | probe | effect |
|---|---|---|
| `eval` expand-all | n/a | `.section.collapsed` **6 → 0** (six of seven ship collapsed, exactly as the state file says) |
| `.tab-btn[data-tab="tuning"]` | `elementFromPoint` = **TARGET**, rect 641.2, 7.0, 83.3×22 | `#tuning-container` h **0 → 540**, **module captions 0 → 30** |
| `#gear-btn` | **TARGET**, rect 866, 7, 22×22 | `#settings-popover` hidden **true → false**, h **0 → 66.59** |
| `.tab-btn[data-tab="fx"]` | **TARGET**, rect 732.4, 7.0, 47.4×22 | `#tab-fx` h **0 → 564** |

No `null`, no different element, **no coverage hole**. The eval state's effect was asserted
directly rather than inferred. `measure-ui --verbose` reports
`did not resolve|unresolved|skipped state` = **0** with `states default + 4` — **all four applied,
the wave's longest walk**.

**Module captions 0 → 30, not the plan's 13** — Task 2 §7.7 and Task 3 §7.7 confirmed on a THIRD
consumer. 37 module keys − s71 D5's 7 that never enter the DOM = 30 exactly.

**The `10 never became visible` NOTE is unchanged at 10**, s71 D5's identified set, **nine of ten
structurally unmeasurable** (an `<option>` inside a collapsed `<select>` has no geometry for any
DOM probe). **`111 of 84` is not a fraction** and was not read as a failure.

---

## 7. FALSE PREDICTIONS — measured value beside the predicted one

### 7.1 Precondition (g) — `plugins/O-Strata/` is TRACKED, and the count is still not the check
- **Predicted:** *"`git status --short` → only `?? plugins/O-Strata/`"*, and a downstream verify
  `git status --short | grep -c "O-Strata"  # 1`.
- **Measured:** `git status --short` shows **three** entries, none of them the plan's:
  ` M .claude/agent-memory/research-planning-agent.md`, `?? .planning/quick/260907-ja8-…/` and
  `?? plugins/O-Strata/.planning/mockups/`. The count reads 1 for O-Strata again but, as in Task 3
  §7.1, **for a different reason** — a concurrent session's new untracked subdirectory, not the
  original untracked plugin. The orchestrator's corrected form — *no entry under
  `plugins/O-Reed/` or `plugins/O-Bowed/`* — is the one that discriminates, and it was used and
  met. J1's lint consequence is unaffected: O-Strata still has no `i18n.js`.

### 7.2 The stale-body probe reports 5, not 4 — and the substance is 6
- **Predicted:** *"The comment-stripped probe reports **4** hits"*, and the verify block *"Fired at
  planning time: 4"*.
- **Measured:** **5** code hits — `That is all it holds`, `English and French`,
  `so does the Tuning tab`, `anglais et le fran`, `onglet Accord`. Same shape as Task 2 §7.2,
  where the same probe over-counted 7 for 5: **the count is a property of the alternation, not of
  the file.** Both readings go to 0 after the fix.

### 7.3 The probe is BLIND to the French half of the J9 site, for an encoding reason
- **Not predicted at all**, and it is the more interesting half of §7.2.
- The alternation's French J9 branch is `rien d.autre`. The file writes
  `rien d’autre` with U+2019, which is **three bytes** in UTF-8, and `perl` without `-CSD` matches
  `.` against ONE BYTE. **The French "and nothing else" clause is a real J9 site the probe cannot
  see**, and a plan that had used the probe's count as its worklist would have corrected the
  English and shipped the French falsehood. There are **six** substantive sites, not five and not
  four. Carried forward for O-Bowed, whose body is *"It holds the interface language and nothing
  else. Press Escape to close it."* — its French half will be invisible the same way.

### 7.4 J4's O-Reed line numbers, and the restore-scoped COUNT, are both wrong
- **Predicted:** the form-6 restore sites at raw **L372, L392-393, L533**, and a verify expecting
  `grep -c` = **3**.
- **Measured:** raw **L495, L515, L516, L656** — four lines, so the verify's expected **3** is
  wrong as well as the line numbers. L372/L392/L393/L533 are the numbers in the
  **comment-stripped** file, where the four sites sit at stripped L372, L392, L393 and L533 —
  **exactly the plan's list.** One finding, two numbering conventions, for the third time in this
  wave (Task 2 §7.8, Task 3 §7.5).
- The plan also gives `const entry = (I18N[key] || {})[lang]` at raw **L312**; it is at raw
  **L435** (stripped L279). **Every other cited raw number is exact**: loader publish **L172** ✓,
  destructure **L249** ✓, form 1 **L252** ✓, `sweep('en')` **L480** ✓, `sweep('fr')` **L509** ✓,
  `__setLanguage` **L505** ✓, the selector assertion **L508** ✓, the printed comparison **L510** ✓.
  **Verify by pattern, never by line number.**

### 7.5 `wrap-count` goes 1 → 2 with no pixel changing, because a pin changes what it MEASURES
- **Not predicted.** The screen reports one finding before the geometry work and two after.
- **Measured:** the added entry is `span.xy-axis-label.xy-axis-y  "Double Reed"  en=7 line(s),
  fr=8 line(s)` — a **Latin-arm** entry appearing after a Chinese localization.
  The element's boxes are **byte-identical across all three runs**: en `9.00 × 62.78`, fr
  `9.00 × 69.61`, before the fonts, after the fonts and after the ratio block. Only the zh box
  changed (11.00 → 9.00 wide, the Han growth the pin removed).
  The screen's own header explains it: *"171 text-bearing leaf node(s) compared; **95 of them
  ESTIMATED at fsn × 1.2** (line-height: normal) rather than measured"* — 142 were estimated
  before the pins, 95 after. Pinning `.xy-axis-label` to 1.125 moved it out of the estimated set,
  so 69.61 is now divided by 9.00 (→ 8) instead of by 9.6 (→ 7).
  **`wrap-count` counts lines by dividing a box by an ASSUMED line box, so pinning a
  line-height changes its verdict without changing a pixel.** `check-ui-labels [7]` reads 0 moved
  on both Latin arms and `[8]` — the only assertion that can see an absolutely-positioned
  centre-grown caption — passes. Both entries are on the same element, whose own CLIFF 2 comment
  already documents that it grows from its centre inside a 200×170 `overflow: hidden` pad and can
  move nothing.

### 7.6 `--forward-provenance` takes a VALUE, and the rules never say so
- **Predicted (R5/R6/step 10):** *"`--forward-provenance` is **REQUIRED at emit**: without it
  `--ingest` refuses"*, phrased throughout as a flag to be present.
- **Measured:** it is `val('--forward-provenance')` at `scripts/i18n-zh-backtranslate.js:228` — it
  consumes the NEXT ARGUMENT. Written as `--emit X --plugin X --forward-provenance --out /tmp/f`,
  the manifest records `forwardProvenance: "--out"`, the emit still succeeds, and the ingest still
  prints `forward pass recorded at emit: "--out"` and joins. **The refusal control passes and the
  identity check is vacuous**, because it compares the reverse-pass string against `"--out"` and
  finds them different for a reason that has nothing to do with independence.
  Caught here by reading the ingest's own echo line. Corrected by re-emitting with a real
  provenance string and **re-keying the returned English through the real id** — a pure bijection
  old-blinded → real → new-blinded that changes nothing about what the reader saw or said. The
  M12 order control was fired on the ORIGINAL ids before any remap.
  **The rule text should read `--forward-provenance "<string>"`.**

### 7.7 `--model claude-opus-4-1` is silently remapped
- **Not predicted.** R4 requires a **different model** for a correction round. `claude -p --model
  claude-opus-4-1` prints
  `⚠ claude-opus-4-1 is automatically remapped to Opus 5 (the latest Opus)` on stderr and runs Opus
  5. R4 is satisfied — Opus 5 is a different model from claude-sonnet-4-5 — **but not by the name
  requested**, and a summary that recorded "claude-opus-4-1" without reading stderr would have
  named a model that did not run. `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` keeps the requested
  one.

### 7.8 Task 3's corpus arithmetic double-counts by one plugin
- **Predicted (Task 3 §9):** *"corpus: 4292 → 4478 rows of 5441"*.
- **Measured:** the emitter's own TOTAL read **4292 before this task** and **4460 after**, and
  4460 − 168 = 4292. The plan's own chain reconciles: 3761 + 131 + 214 + 186 = **4292** after
  O-Contrabass, + 168 = **4460**, + O-Bowed's 161 = **4621**, which is the plan's stated wave
  target exactly. **Task 3 added its own 186 to a figure that already contained them.** The live
  reading is the one to carry: **4460 of 5441 after O-Reed.**

### 7.9 Everything the plan predicted that HELD
`36 I18N + 96 LABELS`; **168** emitter rows; the `[12] 2 module(s)` line verbatim, still not
naming `preset-manager.js`; `111 of 84` with **10** never-visible and its first entry verbatim;
the three computed-`ff` populations **345 / 116 / 30 exactly**; the 30 form-4 ids **exactly**; the
R8 pin census **3 / 3 / 5 / 11 / 3 exactly**; **8** font declarations of which **7** `inherit`;
37/37 module keys in en/fr on both plugins; s71 D5's ten never-visible entries in order; the
form-1 assertion at raw **L252**; the sweep pair, the switch, the selector assertion and the
printed comparison at their **exact** raw lines; the loader publish and destructure at **L172** and
**L249**; the codec at `PluginProcessor.h` **L103-104**; the endonym-form convention at
**L934**; `VERSION "1.5.0"` **quoted** with both reader arms agreeing; target = folder;
`PLUGIN_CODE ORed`; `IS_SYNTH TRUE`; **900 × 600**; ValueTree `getProperty("uiLanguage")`
persistence; **no** language `AudioParameterChoice`; `#tips-toggle` inside `.settings-popover`;
the XY-marker look-alike; the native-fn figures **22 / 9 / 20 / 7 exactly**; `identity … 150
distinct display ids`; the `-dev`-only install with **zero** alternate-variant orphans; and J1's
discriminator verbatim.

---

## 8. The blind reverse read

Emitted **`--emit O-Reed --plugin O-Reed`** (BOTH — W4) **twice**, so each chunk carries its
**own salt**. 131 rows split **66 + 65**, proved disjoint on the **real** ids (0 overlap, union
exactly 131), proved to contain **no module row**, and the two emits share **0 of 168** blinded
ids. Dispatched `claude -p --model claude-sonnet-4-5 --allowed-tools ""` from
`/tmp/ja8t4/blindrun` — outside the repo — with file, web and repository access forbidden in the
prompt.

**36 concepts were deliberately SPLIT across the two chunks** — a caption in one, its own tooltip
title in the other (Task 2 c/f 13, Task 3's practice).

### Controls — sixteen, every one fired

| control | result |
|---|---|
| line count matches sent | **66/66 and 65/65** |
| ids identical **AND IN ORDER** (M12) | **CLEAN on both chunks**, fired on the ORIGINAL ids before any remap |
| ids as a set | identical |
| ids pure 12-hex | 131/131 |
| ids unique | 131/131 |
| no key fragment in any id | 131/131 |
| rows well-formed (exactly 2 tab fields) | 131/131 |
| Han surviving in the returned English | **0**, positive control fired on the same probe (**131** Han lines SENT) |
| sha256 of the zh column vs the committed tree | **MATCH** — `c6459b40…b1a5684b`, using `LC_ALL=C sort` |
| working tree == HEAD at read time | **YES** — the pass ran against `9d83e7a7` |
| module rows excluded from the batch | **37 excluded, 0 present in either chunk** |
| refusal 1 — missing `--forward-provenance` at emit | **FIRED** — *"the batch was emitted with no recorded forward pass"* |
| refusal 2 — wrong-side `--manifest` | **FIRED** — *"WRONG MANIFEST FOR THIS BATCH: all 3 returned ids are unjoinable, not one."* |
| refusal 2 re-fired against a manifest with REAL forward provenance | **FIRED again** (§7.6) |
| product-name control, emitted batch | **0** |
| product-name control, returned English | **0** |

**Both refusals were run WITH `--provenance` supplied**, and refusal 2 additionally against a
manifest that HAS forward provenance, so neither could fire for the other's reason. Task 2 §7.9's
three ordered steps confirmed.

**The product-name control read 0 on both sides for the THIRD time in this wave** (Task 1 §7.2,
Task 3 §7.2). O-Reed's markup carries an `O-Reed` wordmark, but it is an `I18N_EXEMPT` entry
rather than a table row, so it never enters the emitted batch. The prediction has now been wrong
three times for one structural reason.

### All 131 triples read with `--verbose` (R2)

**60 round-tripped word for word.** Every one of the remaining **71** was read and adjudicated on
**collision on the page**, not drift distance.

### ONE ROW RE-AUTHORED — and it is exactly what the read exists to find

**`tip.growl` title, `Growl Amount` → 喉音量 → the reader returned "Throat Volume".**

喉音量 parses two ways: **喉音 + 量** (growl + amount) or **喉 + 音量** (throat + VOLUME). The
reader took the second. That is a real collision on this page, not a reader artefact: the page
carries `tip.output` "Output Gain" → 输出增益 and `tip.mouthpiece` "Mouthpiece Volume" → 吹口容积,
so a reader who parses 喉音量 as a volume has three level-shaped controls to choose between — and
`Throat` is separately a `vibratoSource` option word on the same page, which the body of
`tip.vibratoSource` names in Latin.

**Re-authored to 喉音强度** (growl + intensity). 喉音 is still the glossary root; 强度 appears
nowhere else on the page and cannot be read as a level.

### Correction round 2 — fresh agent, fresh salt, DIFFERENT MODEL (R4)

Emitted afresh (**0 blinded ids shared with round 1**) and dispatched to Opus 5 (§7.7). The chunk
carried the corrected row plus **four neighbours as decoys** so the reader could not tell which
row was under test:

| row | zh | en' |
|---|---|---|
| **`tip.growl` title** | **喉音强度** | **"Throat tone intensity"** |
| `label.knob.growl` | 喉音 | "Throat tone" |
| `tip.mouthpiece` title | 吹口容积 | "Mouthpiece volume" |
| `tip.output` title | 输出增益 | "Output gain" |
| `tip.vibratoSource` title | 颤音源 | "Vibrato source" |

**The correction holds and the round is not vacuous.** The volume sense now attaches to 容积
(mouthpiece) and the level sense to 输出增益 (output); 喉音强度 attaches to neither, and the
caption and its title read as one vocabulary. The three decoys returned exactly, which is what
makes the round evidence rather than ceremony.

**Round 2 corrected nothing further, so there is no round three.** R4's stated condition, said
here and said in the commit.

### R3 — before and after

- **Before authoring:** 132 rows screened, **95 glossary-covered**, **20 root collisions**.
  Eighteen were same-control caption/title pairs. Two three-member groups were adjudicated:
  - **`双簧` — `label.xy.axisY`, `label.knob.doubleReed`, `tip.doubleReed`.** Two different DOM
    elements naming ONE parameter (`doubleReed` is the pad's Y axis and the knob), and **all three
    carry identical English**, which R3 excludes explicitly.
  - **`周期 (C)` — `label.genPeriod` and `label.genR2Period`.** Two module rows inherited at `'bt'`
    with identical English; `genR2Period` is one of s71 D5's seven keys that never enter the DOM,
    and neither was in the batch. Same adjudication Task 3 reached.
- **After authoring, mechanically:** **26 shared-rendering groups**, **10 carrying different
  English**, and **all 10 proved SAME-CONTROL** — nine by key
  (`reedHard`, `register`, `vibDepth`, `vibRate`, `flutter`, `chiff`, `infSustain`, `revBore`,
  `polyMode`, each a caption and its own tooltip title) and the tenth
  (`label.hoverHelp` "Hover help" / `tip.tipsToggle` "Hover Help") the caption row and the tooltip
  title of `#tips-toggle`, differing only in case.
  **0 distinct-control collisions.**

### Accepted with reasons — the classes worth naming

**The ABBREVIATION class**, for the fourth time in this wave. `Reed Hard.`, `Rev. Bore`,
`Vib Depth`, `Vib Rate`, `Inf. Sustain`, `Max Voices` all returned unabbreviated ("Reed
Stiffness", "Reverse Bore", "Vibrato Depth", "Vibrato Rate", "Infinite sustain", "Max
polyphony"). English abbreviates against this page's **68px** `.knob-control` cap with
`text-overflow: ellipsis` at 8px — the file's own CLIFF 1 note; Chinese does not need to, and the
longest Chinese caption is five characters (≈41.5px at 8px + 0.3px tracking).

**`双管` → "Double Pipe" beside `双簧` → "Double reed" — ACCEPTED.** They share 双 and differ in
the head (管 bore / 簧 reed), exactly as the English "Dual Bore" / "Double Reed" does, and **both
readers recovered the distinction separately**. 双管 is the glossary root for `dual bore`.

**`音色预设` → "Voice Preset" / "Tone presets" — ACCEPTED, where O-Wind had to re-author.** Task 2
re-authored O-Wind's to 乐器预设 because 音色 is O-Wind's own rendering of a `Tone Color` knob two
sections away. **O-Reed has no `Tone Color` caption** — checked against the full label set, not
assumed — so the glossary root is unambiguous here. Recorded because the difference between the
two plugins is a page fact, not a translation preference.

**`泛音孔` → "Register Hole" beside `音孔` → "Tone Holes" — ACCEPTED.** Parallel to the English
"Register Hole" / "Tone Hole", and both recovered separately.

**`起音气声` (chiff) beside `气噪` (air noise) and `气息` (breath) — ACCEPTED.** Three 气-initial
captions, three distinct heads, and the readers returned "Attack Breath", "Breath noise" and
(for the tooltip title) "Air Pressure" — each attached to the right control.

**`气压` for "Breath Pressure" — the glossary root, and O-Wind ships it.** The first authoring pass
wrote 气息压力; the scoped lint's Z5 named the root and it was taken. The caption beside it is
气息 ("Breath"), which mirrors the English caption/title pair exactly.

**`最大复音数` beside `复音模式` — ACCEPTED.** The tooltip body explicitly makes them a pair (the
voice ceiling applies only while the mode is Polyphonic), the strings differ in their heads
(数 count / 模式 mode), and both readers recovered both.

### W3 / N8 checked BOTH ways

**Direction 1 — every exempt token in an English body survives verbatim into the Chinese:**
`Simple`, `Multi-segment`, `Lip`, `Breath`, `Throat`, `Monophonic`, `Polyphonic`, `2x`, `4x`,
`Bb Clarinet`, `Impossible Bore`, `arghul`, `launeddas` — **13 of 13 present in both arms**,
checked mechanically. All eight of the ones that appear in the returned English came back
verbatim too.

**Direction 2 — every non-exempt caption named in a body is named by its LOCALIZED caption:**
Bore Character → 管体特性, Vibrato Source → 颤音源, Dual Bore → 双管, Drone Pitch → 持续音音高,
Feedback Path → 反馈路径, Max Voices → 最大复音数, Polyphony Mode → 复音模式, Breath Pressure →
气压. **8 of 8 present**, and the blind readers recovered all eight as the right control
("bore characteristic", "vibrato source", "dual bore", "drone pitch", "feedback path",
"max polyphony", "polyphony mode", "air pressure") — which is direction 2 confirmed from the
other side rather than asserted.

**One body needs BOTH directions and got both:** `tip.vibratoSource` keeps `Lip`, `Breath` and
`Throat` in Latin (direction 1, they are host automation option strings) while naming 口型 for the
embouchure it modulates (direction 2, a keyed caption).

---

## 9. Gate results — every actual output line

```
check-i18n --plugin O-Reed                  exit 0
  PASS: [1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr","zh-Hans"]
  PASS: [1] the table carries copy — 36 I18N + 96 LABELS        [UNCHANGED from 36 + 96]
  PASS: [1] every I18N key has an entry in every declared language (en, fr, zh-Hans)
  PASS: [1] every LABELS key has a string t in every declared language
  PASS: [12] there is shipped page JS to scan — 2 module(s): the inline <script type="module">
             in index.html, modules/tuning/scala-tuning-engine/js/tuning-panel.js
             [preset-manager.js still correctly OUT of scope — s71 D2]
  ALL CHECKS PASS — 1 localized plugin(s)

i18n-zh-lint --plugin O-Reed                exit 0
  O-Reed   168 rows  132 zh   Z1 · Z2 · Z3 · Z4 · Z5 · Z6 · Z7 · F1 · R1 · Z8 ·   total 0
  BELOW SHIP BAR — entries at reviewed:'mt': 0
  straight copies zh === en (info): 0   termNote exemptions (info): 2
     [label.knob.flutter authored here; label.genDivisions inherited with its module row]
  GATE PASSED — exit 0. 0 findings across 1 plugin(s).

check-ui-labels --plugin O-Reed             exit 0
  10 x PASS: [7][GEOMETRY DIFF][fr|zh-Hans] no non-label element moved   (5 states x 2 arms)
  PASS: [7][GEOMETRY DIFF][zh-Hans] the visible element SET is identical in English and zh-Hans
  PASS: [8][zh-Hans] two labels disjoint in English do not intersect in zh-Hans
  PASS: [8b][zh-Hans] no label intersects a NON-label element it cleared in English
  PASS: [2][vacuity][zh-Hans] the zh-Hans pass actually rendered — 84/84 labels (100%)
  PASS: [2][vacuity][zh-Hans] keyed ATTRIBUTES actually changed language — 4/4
  PASS: [2][vacuity][fr]      the fr pass actually rendered — 76/84 labels (90%)
  coverage: 111 of 84 [data-i18n] elements were VISIBLE in at least one state
            10 never became visible                          [UNCHANGED, s71 D5's set]
  == ALL CHECKS PASSED ==

ui_tip_render_check.js                      exit 0
  PASS: LANGUAGES derives from the table as a non-empty array opening with the source
        language — got ["en","fr","zh-Hans"]
  PASS: the derived list carries at least one language besides the source language
    sweeping 3 language(s): en -> fr -> zh-Hans
    FR takes the tallest tip 169.9 -> 186.5px (against the EN baseline)
    ZH-HANS takes the tallest tip 169.9 -> 136.5px (against the EN baseline)
  == ALL CHECKS PASSED ==   (757 PASS)                       [was 539 at two languages]

boot-all-uis --plugin O-Reed --strict-tips  exit 0
  clean: 1/1   DEAD bindings: 0   late bindings: 0

measure-ui --report all
  undeclared-font:    0 finding(s)   [was 111 Han nodes on 3 stacks; bare Arial 30 -> 17]
  line-height-normal: 1 finding(s)   [was 46; the one residual is an evidenced non-mover]
  wrap-count:         2 finding(s)   [was 1 — see 7.5, no pixel changed]
  svg-font-attr:      0 attribute carrier(s), 0 finding(s)   [inert on this plugin]
  identity: 1689 nodes / 563 distinct DOM keys / 150 distinct display ids
measure-ui --verbose:  did not resolve|unresolved|skipped state = 0   (states default + 4)
  nodes on the shared module -apple-system stack:  en 0   zh 0    [was 116 on en]
  zh Han visible nodes 111, resolving WITHOUT a named CJK face: 0

i18n-zh-backtranslate --plugin O-Reed
  O-Reed   168  168   0 mt   168 bt   0 native   0 none
  BELOW SHIP BAR — at reviewed:'mt' or unflagged: 0. The ship bar is 'bt'.
  corpus: 4292 -> 4460 rows of 5441            [see 7.8 for Task 3's arithmetic]

C++:  comment-stripped 'zh-Hans' count = 2
      languageCode: THREE-WAY   languageIndex: THREE-WAY   (matched by NAME, not swept)
      Han under Source/**/*.{h,cpp}: NONE
      positive control on the same probe: FIRED (Resources/ui/js/i18n.js)
      persistence: ValueTree getProperty("uiLanguage") — unchanged
      language AudioParameterChoice: none

Repo-wide, identical to the J1 baseline:
  check-i18n     exit 0   ALL CHECKS PASS — 43 localized plugin(s)
  i18n-zh-lint   exit 2   0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read
                          (zh-Hans entries checked: 3234 -> 3366)
  i18n-fr-lint   exit 2   plugins with findings: 0 / 44   (1 could not be read)
  i18n-zh-lint --self-test   10/10, exit 0

Two-arm CMake reader (R9), run unconditionally on a QUOTED literal:
  one-arm 1.5.0 == two-arm 1.5.0  ->  bumped  ->  one-arm 1.6.0 == two-arm 1.6.0

Ratio-block verify:  grep -B14 "line-height: 1.11" | grep -c tuning-container  ->  11
R8 pin census:  before  min-w 3 / min-h 3 / line-h 5 / letter-sp 11 / white-sp 3
                after   min-w 3 / min-h 3 / line-h 6 / letter-sp 11 / white-sp 3
                (min-w UNCHANGED: the octave-stretch floor was rewritten, not added)

Gate site census (comment-stripped):
  form-1 + form-7 residue          0
  LANGUAGES mentions in code       8
  restore-scoped tip.breath reads  4          [the plan's verify expected 3 — see 7.4]
  g5l inventory grep sees it?      NO

Native-fn defect, OUT OF SCOPE, asserted UNCHANGED in both directions:
  called=22  registered=9  MISSING=20  DEAD=7        [before AND after]

modules/ dirty: 0       modules/ in either commit: 0
scripts/ dirty: 0
O-Strata in either commit: 0    libs/SAF in either commit: 0
PLUGINS.md in either commit: 0  tags at HEAD: 0

Build:  ./scripts/build-and-install.sh O-Reed   exit 0, all 7 phases
        VST3 + AU installed, 0 "Sweeping ALTERNATE-variant" warnings
        Installed CFBundleShortVersionString: 1.6.0  (both bundles)
        ~/Library/Audio/Plug-Ins/VST3/O-Reed-dev.vst3
        ~/Library/Audio/Plug-Ins/Components/O-Reed-dev.component
        -dev only; no alternate-variant orphan on disk
auval:  NOT RUN — deferred to Task 6 by design. O-Reed's triple is
        `aumu ORed OuDv` (IS_SYNTH TRUE). auval -v takes THREE UNQUOTED WORDS.
```

---

## 10. How the OUT-OF-SCOPE native-fn defect actually behaved under the gates

**Measured before and after, unchanged in both directions: 22 called, 9 registered, 20 missing,
7 dead.** The two lists, recorded so the next reader does not re-derive them:

```
MISSING (called from JS, never registered in C++) — 20:
  getTuningIntervals getTuningName getTonicNote getOctaveStretch setSingleInterval
  setTonicNote getEmbeddedTuningList loadEmbeddedTuning generateEDO generateHarmonicSeries
  generateRank2 applyGeneratedScale loadScalaFile loadKBMFile saveScalaFile saveKBMFile
  exportTuningHTML setOctaveStretch getMasterTune setMasterTune
DEAD (registered in C++, never called from JS) — 7:
  getPresetList getCurrentPreset loadPreset savePreset selectNextPreset
  selectPreviousPreset savePresetWithDialog
```

**It blocked nothing, and the plan's prediction of how it would behave was exactly right.**

- **The captions render and the data does not.** The tuning state mounted **30 module captions**
  and `#tuning-container` grew 0 → 540px, so `check-ui-labels` measured the panel's Chinese
  captions in full on all five states. The interval table shows its default 11 rows with default
  values because the 20 missing functions hang silently rather than throwing.
- **No gate went red on this surface, in any language.** Precondition (e) fired the tip-render
  gate BEFORE the flip and it passed at two languages, so the discriminator the plan installed for
  exactly this purpose was available and never needed.
- **`boot-all-uis --strict-tips` reads 0 DEAD and 0 late bindings.** A hanging native promise is
  invisible to it: it measures TIP_BINDINGS selector resolution at settle, and every anchor
  resolves whether or not its data arrived.
- **`check-i18n` scans `tuning-panel.js` statically** and calls nothing, so its `[12] 2 module(s)`
  line is unaffected.
- **The one place it is visible is the coverage line**, and only indirectly: 7 of the 37 module
  keys never enter the DOM and 9 of the 10 never-visible entries are `<option>`s inside collapsed
  `<select>`s. That is s71 D5's coverage hole and it is a state-file gap, not the bridge defect.

**Nothing was fixed, no function was registered, and `Source/PluginEditor.cpp` is untouched by
both commits.** Recorded below as a deferred item.

---

## 11. Carry-forwards for Task 5 (O-Bowed)

O-Bowed is the wave's **second** `-apple-system` panel and the only consumer whose state file
already reached the panel.

1. **The `.tuning-panel` override is the fix and it WILL move O-Bowed's Latin arm — but probably
   by one element, not by 116.** On O-Reed the whole 116-node face change produced **exactly one**
   `[7][fr]` mover, and its cause was not the override: it was
   `#tuning-container .octave-stretch-label { min-width: 51.84px; }`, a floor measured against the
   OLD face. **O-Bowed carries the identical 51.84px floor** (J14). Expect the same single mover,
   expect the same cause, and **re-measure the floor rather than taking the fallback.** The
   corrected value on O-Reed is **53.86px**, which is also O-Contrabass's measured value — if
   O-Bowed lands on 53.86 too, three consumers agree and the number is the face's, not the page's.
2. **Check `.tonic-label` (41.59px) and `.interval-list-header` (24px) too, but expect them to
   hold.** Both held on O-Reed on all three arms after the face change; only the octave-stretch
   caption crossed its floor.
3. **Use all FIVE tail selectors.** O-Bowed reads 30 form-4 nodes like O-Reed and O-Contrabass —
   its five `.viz-btn` carry no face. Expect bare Arial **30 → 17**, expect **11** Han-bearing
   nodes named by `undeclared-font`, and say "the rule covers 13 where the screen names 11".
4. **Verify the `1.11` ratio per selector anyway. `.viz-btn` is 9px on O-Reed and on
   O-Contrabass, and 10px on O-Wind** — two values across three consumers, so the check has now
   earned itself twice. `.interval-list-header` is **two line boxes in English on every
   consumer**; derive per LINE or the pin doubles.
5. **Budget for the page-local block as well as the panel block.** O-Reed needed **ten** page-local
   selectors on top of the panel's eleven, and 266 movers reduced to **one root cause** — a single
   absorbing container (`.xy-pad-container`, `dh=4.0`) with everything below it cascading. **Read
   the `dh` column first; a `dw`/`dh` row is a root cause and a `dy` row is cascade.** On O-Reed
   the entire mover list contained exactly one non-cascade row.
6. **Its gate carries form 1 at raw L227 and form 7 (the paired `sweep()`), like O-Reed's.** The
   repair shape is Task 2's, proved again here. **Verify every site by PATTERN, not by the plan's
   line number**: on O-Reed the form-6 restore lines were the COMMENT-STRIPPED numbers and the
   dynamic-read line was off by 123, while every other cited raw number was exact (§7.4).
7. **Fire derive-or-abort TWICE** — empty list AND one-member list. The one-member plant needs a
   **second, independent** guard on the walk; a shape assertion alone passes it.
8. **Its form-6 sites are RESTORE-scoped — LEAVE them, classified at each site.** And **count them
   by grep, not from the plan**: O-Reed's predicted 3 measured **4**.
9. **Its J8 and J9 sites are REAL, and its J9 comment says so out loud** — O-Bowed's reads *"THE
   TUNING CLAUSE IS LOAD-BEARING AND IT IS TRUE"*. Correct the body AND the comment, in en AND fr,
   re-read the file, then author (C7).
10. **THE PROBE CANNOT SEE THE FRENCH HALF OF THE J9 SITE (§7.3).** `rien d'autre` is written with
    U+2019 and `perl` without `-CSD` matches `.` against one byte. **Run the stale-body probe with
    `perl -CSD`, or grep the French clause separately**, or you will correct the English and ship
    the French falsehood. O-Bowed's en body is *"It holds the interface language and nothing else.
    Press Escape to close it."* — its French half is the one at risk.
11. **`--forward-provenance` takes a VALUE (§7.6).** Write
    `--forward-provenance "<string>" --out <file>`, never `--forward-provenance --out <file>`, or
    the manifest records `"--out"`, the ingest joins anyway, and the identity control passes
    vacuously. **Read the ingest's own `forward pass recorded at emit:` echo line to confirm.**
12. **`--model claude-opus-4-1` is silently remapped to Opus 5 (§7.7).** R4's "different model" is
    still satisfied, but read stderr before naming a model in the summary.
13. **The product-name control returns 0 whenever the wordmark is an `I18N_EXEMPT` entry.** Three
    for three in this wave. Predict 0 and adjudicate if it is not.
14. **The tuning state mounts 30 module captions** — third consecutive consumer. 37 − s71 D5's 7 =
    30. Use it as the state-EFFECT figure.
15. **`wrap-count` can grow without a pixel changing (§7.5).** It divides a box by an ASSUMED line
    box for `line-height: normal` nodes and by the real one once you pin it, so a ratio pin moves
    nodes out of the estimated set and changes the reported line count. Compare the BOXES across
    runs before treating a new `wrap-count` entry as a regression.
16. **`svg-font-attr` reads 0 carriers on O-Bowed** and is inert; `undeclared-font` is the detector.
17. **Use `grep -B14`, not `-A14`**, for the ratio block. **Use the corrected duplicate-key scan**
    (exclude the declared `LANGUAGES` members). **Write promotion scripts to a FILE.** **`serve()`
    returns `{ server, port, close }` with no `url`** — all four confirmed again here.
18. **Check O-Bowed for a `Flutter` caption** — O-Reed had one and needed the 花舌 termNote.
    O-Bowed is a bowed-string model, so the likelier trap is a `Growl`/`Bow`/`Noise` term; run the
    R3 screen before authoring rather than after.
19. **Watch for a compound caption whose glossary root parses two ways.** O-Reed's only
    re-authoring was 喉音量, which reads as `growl + amount` or `throat + volume`. **Before
    authoring an X+量 or X+度 compound, check whether the page carries another level, volume or
    gain control** — the blind reader will find it, but a round trip costs a correction round.
20. **Task 3's corpus figure is off by one plugin (§7.8).** The live pre-O-Bowed total is **4460**;
    O-Bowed's 161 takes it to **4621**, which is the plan's stated wave target exactly.

---

## 12. Deferred / out of scope

- **The 20 missing and 7 dead native-fn registrations on O-Reed's tuning tab.** Both lists are in
  §10. It is a C++/JS bridge defect, not a localization one; repairing 20 native functions is a
  release of its own. Measured before and after, unchanged, and it blocked nothing.
- **auval — NOT RUN**, deferred to Task 6's single cold sweep by design. Triple `aumu ORed OuDv`.
- **`PLUGINS.md` NOT touched.** Its O-Reed row reads **1.5.0** and Task 6 must set it to **1.6.0**
  (one minor). O-Wind's must go to 1.21.0, O-Contrabass's to 1.10.0, and O-GrainScatter's is two
  minors stale at 2.6.1 → 2.8.0.
- **The glossary is NOT edited.** Task 1's `'dist lpf': ['失真低通']` → `['距离低通']` at
  `scripts/i18n-zh-glossary.js:433` remains open as a corpus-level item. **O-Reed adds no glossary
  defect**: its one exemption (`flutter` → 花舌) is a per-page sense note, not a wrong root, and
  the root is correct for the sense it was written for.
- **`--forward-provenance`'s value-consuming behaviour is not documented in the executor rules**
  (§7.6). A rule text fix, not a script fix; the script's own usage line at
  `scripts/i18n-zh-backtranslate.js:69` is correct.
- **The stale-body probe's byte-matching blindness to French curly apostrophes** (§7.3). A probe
  fix (`perl -CSD`), owned by whoever next writes one; not edited here because it lives in a plan,
  not in a script.
- **The 17-of-37 module coverage hole is NOT closed** (s71 D5): 10 never-visible + 7 that never
  enter the DOM. Closing it needs states that change the generator type and select the Rotation
  visualisation; this wave does not budget them.
- **`reviewed: 'native'` OPEN** on all 168 rows — no native Chinese reader exists on this project.
  Disclosed in the CHANGELOG and printed by lint rule R1 on every run.

## Self-Check: PASSED

- `plugins/O-Reed/Resources/ui/js/i18n.js` FOUND;
  `plugins/O-Reed/Resources/ui/index.html` FOUND;
  `plugins/O-Reed/Source/PluginProcessor.h` FOUND;
  `plugins/O-Reed/tests/ui_tip_render_check.js` FOUND;
  `plugins/O-Reed/CMakeLists.txt` FOUND;
  `plugins/O-Reed/CHANGELOG.md` FOUND.
- Commit `9d83e7a7` FOUND; commit `f3737f53` FOUND.
- `~/Library/Audio/Plug-Ins/VST3/O-Reed-dev.vst3` FOUND at **1.6.0**;
  `~/Library/Audio/Plug-Ins/Components/O-Reed-dev.component` FOUND at **1.6.0**.
