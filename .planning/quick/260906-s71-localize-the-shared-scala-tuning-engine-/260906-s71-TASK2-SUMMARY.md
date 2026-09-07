# Quick Task 260906-s71 — Task 2 Summary

**Task:** Drive the lazily-mounted shared tuning panel from all five
`tests/i18n-states.json`, then fire the geometry and boot gates.

**Status:** COMPLETE. One path-scoped commit, `d848337a`, on `main`.
**Executed:** 2026-09-06

---

## 1. Precondition — FIRED, not assumed

`node scripts/boot-all-uis.js --strict-tips`, captured **before any change**:

```
DEAD bindings:  0  across 0 plugin(s)
late bindings:  2  across 1 plugin(s)
  O-Bells    #ref-pitch-knob, #octave-stretch
```

**Precondition MET** — the standing census control reads exactly 2 LATE / 0 DEAD.

---

## 2. The states added

| Plugin | click | position | file changed |
|---|---|---|---|
| O-Bassoon | `.tab-btn[data-tab="tuning"]` | FIRST, before the about tab | yes |
| O-Bowed | `#tuning-toggle` | already present as "tuning overlay open" | **NO — left unchanged** |
| O-Contrabass | `.tab-btn[data-tab="tuning"]` | FIRST, before gear + help | yes |
| O-Reed | `.tab-btn[data-tab="tuning"]` | after expand-all eval, before the FX tab | yes |
| O-Wind | `.tab-btn[data-tab="tuning"]` | after Effects, before the gear popover | yes |

Every selector was re-confirmed in the plugin's own `index.html` before writing
it. Each existing file's documented ordering rationale (O-Wind's covering
popover, O-Reed's FX tab hiding the instrument panel) was read and preserved.

---

## 3. The state HAD AN EFFECT — measured three ways

### 3a. The coverage denominator grew

`check-ui-labels --plugin <P> --verbose`, the `N of M` line:

| Plugin | BEFORE | AFTER | ΔM |
|---|---|---|---|
| O-Bassoon | 30 of **28** | 51 of **58** | **+30** |
| O-Bowed | 66 of **76** | 66 of **76** | 0 — file unchanged, already reached |
| O-Contrabass | 52 of **52** | 74 of **82** | **+30** |
| O-Reed | 90 of **54** | 111 of **84** | **+30** |
| O-Wind | 69 of **67** | 90 of **97** | **+30** |

### 3b. `elementFromPoint` at every click target — N1 arm 1

Fired on **every** click-form state on all five, with the null/covered
discrimination. **All HIT; no `null`, no covering element anywhere:**

| Plugin | state | rect | centre | elementFromPoint | verdict |
|---|---|---|---|---|---|
| O-Bassoon | tuning | 300,36 300x36 | 450,54 | `button.tab-btn` | HIT |
| O-Bassoon | about | 600,36 300x36 | 750,54 | `button.tab-btn` | HIT |
| O-Bassoon | gear | 864,7 22x22 | 875,18 | `#gear-btn` | HIT |
| O-Bowed | gear | 860,6 28x28 | 874,20 | `#gear-btn` | HIT |
| O-Bowed | tuning-toggle | 715.8,9 62x22 | 746.8,20 | `#tuning-toggle` | HIT |
| O-Contrabass | tuning | 621,9.5 100x22 | 671,20.5 | `button.tab-btn` | HIT |
| O-Contrabass | gear | 737,7.5 26x26 | 750,20.5 | `#gear-btn` | HIT |
| O-Contrabass | help | 709,82.5 48x22 | 733,93.5 | `#help-toggle` | HIT |
| O-Reed | tuning | 641.2,7 83.3x22 | 682.8,18 | `button.tab-btn` | HIT |
| O-Reed | gear | 866,7 22x22 | 877,18 | `#gear-btn` | HIT |
| O-Reed | fx | 732.4,7 47.4x22 | 756.1,18 | `button.tab-btn` | HIT |
| O-Wind | effects | 600,40 300x35 | 750,57.5 | `button.tab-btn` | HIT |
| O-Wind | tuning | 300,40 300x35 | 450,57.5 | `button.tab-btn` | HIT |
| O-Wind | gear | 866,9 22x22 | 877,20 | `#gear-btn` | HIT |

No `null` was returned anywhere, so the "scrolled vs covered" discrimination
never had to fire — recorded because its absence is itself the result.

### 3c. The state EFFECT — N1 arm 2, which subsumes arm 1

Module caption classes reaching the LIVE DOM (`.viz-btn, .interval-list-header,
.tuning-library, .generator-section, .tuning-file-btn, .tk-hint`):

| Plugin | before any state | after the tuning state |
|---|---|---|
| all five | **0** | **13** (interval-list-header, viz-btn, tk-hint, tuning-file-btn, generator-section) |

---

## 4. What driving the panel EXPOSED — a shipped French regression

**O-Bowed was RED on the tree AS FOUND**, before this task changed anything —
it is the only consumer whose state file already reached the panel, so Task 1's
localization landed a French geometry defect that nothing else could see:

```
FAIL: [7][GEOMETRY DIFF][fr] no non-label element moved between English and fr
      at a fixed frame — 53 moved:
        #tonic-down  dx=6.4 dy=-12.0
        #interval-list>div.interval-item:nth-child(3)  dx=0.0 dy=-12.0
        ... 51 more, all dy=-12.0
```

Root cause, measured by `measure-ui --report wrap-count`:

```
div.interval-list-header  "Intervals · notes: 11"  en=2 line(s), fr=1 line(s)
```

The English caption **wraps** in the narrow interval column; the French
`Interv. · notes : 11` does not. The header gives back a line and everything
below it rises. The four narrow-column consumers all carry it; O-Bassoon's
842px column does not wrap and instead showed the Chinese arm GROWING.

### The four failure sets, before the floors

| Plugin | check-ui-labels | what |
|---|---|---|
| O-Bassoon | 4 failures | fr: 4 moved (tonic + stretch dx); zh: 123 moved dy=+3, and [5]/[6] 13 labels spilling further than English |
| O-Bowed | 1 failure | fr: 53 moved dy=−12 — **RED as found** |
| O-Contrabass | 3 failures | fr: 53 moved dy=−11 |
| O-Reed | 2 failures | fr: 53 moved dy=−12 |
| O-Wind | 4 failures | fr: 55 moved dy=−11, plus [5] `label.tonic` spilling 0.0 → 2.8px |

---

## 5. The floors — measured per plugin, in each consumer's OWN CSS

**`modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` is
byte-unchanged**, asserted with `git diff --quiet`. Five consumers share it.

### 5a. `min-height` on `.interval-list-header` — the wrap floor

Each at ITS OWN measured English box. The four numbers are **not** the same,
which is why a single shared value would have been wrong:

| Plugin | column | en box | fr box | floor |
|---|---|---|---|---|
| O-Bowed | 122px | 122 × **24** | 122 × 12 | `min-height: 24px` |
| O-Contrabass | 122px | 122 × **22** | 122 × 11 | `min-height: 22px` |
| O-Reed | 122px | 122 × **24** | 122 × 12 | `min-height: 24px` |
| O-Wind | 112px | 112 × **22** | 112 × 11 | `min-height: 22px` |
| O-Bassoon | 842px | 842 × 11 | 842 × 11 | none — does not wrap |

### 5b. `min-width` on the two captions sharing a flex row with a non-caption

`.tonic-selector` is `justify-content: center`; `.octave-stretch-row` gives its
slider `flex: 1`. Both floored at the **widest** arm (French in every case):

| Plugin | `.tonic-label` en / fr / zh | floor | `.octave-stretch-label` en / fr / zh | floor |
|---|---|---|---|---|
| O-Bassoon | 27.34 / 39.83 / 18.41 | 39.83px | 40.02 / 51.00 / 18.41 | 51px |
| O-Bowed | 28.75 / 41.59 / — | 41.59px | 42.42 / 51.84 / — | 51.84px |
| O-Contrabass | 28.47 / 42.06 / — | 42.06px | 41.50 / 53.86 / — | 53.86px |
| O-Reed | 28.75 / 41.59 / — | 41.59px | 42.42 / 51.84 / — | 51.84px |
| O-Wind | 27.34 / 39.83 / — | 39.83px | 40.02 / 51.00 / — | 51px |

### 5c. O-Bassoon only — the line-box RATIO (N4), not a length

The Chinese arm grew every `line-height: normal` caption 3px (PingFang SC's
ascent + descent against the Garamond/Times stack's). Pinned as a **ratio**
because wave 4e N4 records a length growing one arm 0.77px:

```css
line-height: 1.11;   /* 9px -> 9.99 vs a measured 10; 10px -> 11.10 vs a measured 11 */
```

on `.interval-list-header, .library-header-text, .generator-header-text,
.pitch-circle-label, .ref-knob-label, .tonic-label, .octave-stretch-label,
.gen-row label` **plus** `.viz-btn, .tuning-file-btn, .generator-btn` named
separately — the UA `font` shorthand resets `line-height` on form controls, so
an inherited ratio never reaches a `<button>`.

Result: `measure-ui line-height-normal` **23 → 3** on O-Bassoon. The 3 residual
are its own `.tab-btn` (声音 / 调音 / 关于), outside this task's surface,
recorded and left.

### 5d. O-Bassoon only — the CJK tail on 11 form-4 buttons

`measure-ui --report undeclared-font` reported **11 findings**, every one a
tuning-panel button rendering Han on `ff=Arial`: `.viz-btn` ×5 (圆周 极坐标
矩阵 真实键位 旋转), `.tuning-file-btn` ×5, `.generator-btn` ×1 (生成).

`<button>` does not inherit `font-family` — the UA stylesheet gives it Arial —
so the panel's `'Garamond','Times New Roman',serif` never reached them and
neither did any CJK face. Arial carries no Han, so each caption resolved
through an unnamed fallback. Given this plugin's own house stack from
`index.html:75`. **11 → 0.**

---

## 6. Gate results — every one fired in the BACKGROUND (600s watchdog)

| Gate | O-Bassoon | O-Bowed | O-Contrabass | O-Reed | O-Wind |
|---|---|---|---|---|---|
| `check-ui-labels` BEFORE | 0 | **1 FAILED** | 0 | 0 | 0 |
| `check-ui-labels` with state, no floors | 4 | — | 3 | 2 | 4 |
| **`check-ui-labels` FINAL** | **0 PASSED** | **0 PASSED** | **0 PASSED** | **0 PASSED** | **0 PASSED** |
| `measure-ui` undeclared-font | 11 → **0** | 0 | 0 | 0 | 0 |
| `measure-ui` line-height-normal | 23 → **3** | 0 | 0 | 0 | 0 |
| `measure-ui` wrap-count | 0 | 1 → **0** | 1 → **0** | 1 → **0** | 1 → **0** |
| `measure-ui` svg-font-attr | 0 carriers, 0 | 0, 0 | 1 carrier, 0 | 0, 0 | 0, 0 |
| plugin's own gate file | tip **0** | tip **0** | frontend **0** | tip **0** | tip **0** |
| `check-i18n --plugin` | 0 | — | — | — | — |

`measure-ui` identity ratios (final): O-Bassoon 1011/337/133, O-Bowed
992/496/139, O-Contrabass 1128/564/169, O-Reed 1124/562/150, O-Wind
1278/639/221 — nodes / distinct DOM keys / distinct display ids.

**`boot-all-uis --strict-tips` census control:** `diff` of the BEFORE and AFTER
binding lines is **empty** — 2 late across 1 plugin (O-Bells `#ref-pitch-knob`,
`#octave-stretch`), 0 dead, identically. The 0 DEAD is evidence because the 2
LATE held on both sides.

### The `line-height-normal` criterion, stated as N1 gives it

`check-ui-labels` **0 moved** on all five — that is the criterion, and it is
met. `measure-ui`'s `line-height-normal` screen structurally cannot reach 0
(wave 4c) and is not stated as one; its 3 residual on O-Bassoon are named above
with the elements that carry them.

### Untouchability, asserted with `git diff --quiet`

- `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` — byte-unchanged
- `plugins/{O-Bowed,O-Reed,O-Wind}/tests/ui_tip_render_check.js` — byte-unchanged
  (wave 4f's N12 enumeration census NOT consumed)
- `plugins/O-MicrotonalSampler` — byte-unchanged

---

## 7. The coverage hole, reported as a hole (step F) — and it is NOT the 8 the plan predicted

Measured directly, comment-stripped, with visibility OR-ed over the whole walk.
**Identical on all five:**

| | count | which |
|---|---|---|
| reached the DOM and VISIBLE | **20 / 37** | |
| in the DOM but NEVER VISIBLE | **10** | `label.tkHint` + 6 `#library-filter` options (catAll/catHistorical/catJust/catEdo/catNonOctave/catWorld) + 3 `#generator-type` options (genEdo/genHarmonic/genRank2) |
| **NEVER ENTERED THE DOM** | **7** | `label.genStartHarmonic, label.genEndHarmonic, label.genGenerator, label.genR2Period, label.genNotes, label.noteCount, label.rotationMode` |

**Total residual 17 of 37, and `check-ui-labels` can only see 10 of them.**

Its NOTE reads `10 never became visible`. Its denominator is `[data-i18n]` nodes
present in the **final DOM**, so a caption whose code path never ran is not
counted as never-visible — it is not counted at all. The 7 above are behind
`setGeneratorType('harmonic'|'rank2')`, `drawRotationTable()` and
`renderLibraryList()`, none of which the JUCE stub's invented native-function
returns drive.

Nine of the 10 in-DOM-but-invisible are `<option>` inside a collapsed
`<select>` — **structurally unmeasurable** by any DOM geometry probe, N8's
distinction, not merely unmeasured.

---

## 8. Plan predictions that turned out FALSE

1. **Verify arm 2 is INERT — it would have failed the task in every condition.**
   The plan grounds "the state had an effect" on
   `grep -qE 'viz-btn|pitch-circle-label|library-header-text|generator-header-text|tuning-file-btn'`
   over the `check-ui-labels --verbose` output, calling a miss "the state is
   decorative". Measured, that grep returns **0 in all three conditions**:

   | condition | grep hits |
   |---|---|
   | BEFORE (no tuning state at all) | **0** |
   | AFTER, gate RED (4/3/2/4 failures) | **0** |
   | AFTER, gate GREEN | **0** |

   `check-ui-labels` names an element only in a FAIL line or the never-visible
   list, and those five class names are exactly the elements that are always
   visible and always pass. The arm carries no information at all — it is not
   merely inverted, it is inert. Replaced with the direct EFFECT measurement in
   §3c (0 → 13 module caption nodes) and the key census in §7.
   (`pattern_recorded_gate_command_not_executable_as_spelled`.)

2. **"M must GROW … a state that does not move M is a FAILURE of this task."**
   True for the four files edited, but **false as a universal criterion**:
   O-Bowed's M is unchanged at 76 because its state file already reached the
   panel and was correctly left alone. The plan's own step A says not to
   duplicate it, so the two instructions contradict each other.

3. **`N of M` is not a coverage fraction and N can EXCEED M.** O-Reed reads
   `111 of 84`, O-Bassoon read `30 of 28` before the change. N is the count of
   paths seen visible across the cumulative walk; M is `[data-i18n]` membership
   of the **final** DOM. A plugin that re-renders a subtree between states
   produces N > M routinely.

4. **"The 8 dynamic-only captions … may never appear."** The count is **7**, and
   the named set differs: `label.tonic` IS reached (the tonic selector mounts
   with `updateIntervalList()`), while `label.noteCount` and `label.rotationMode`
   are not. The real residual is **17**, not 8, because the plan did not
   anticipate the 10 in-DOM-but-invisible.

5. **"only O-Bowed has a `tests/i18n-states.json`"** (CONTEXT, already corrected
   by Task 1's plan) — confirmed again: all five exist, four were edited.

6. **The plan predicted no geometry work would be needed on the French arm**
   ("the French column MAY already move rows … separate a pre-existing French
   wrap from anything this task caused"). Every one of the five needed French
   floors, and none of it was pre-existing — the tuning panel was English in
   both languages until commit 842bf6e0, so its geometry was identical by
   construction. **All of it was caused by Task 1**, and O-Bowed was already
   shipping it RED.

7. **The plan expected shrinks needing floors on the CHINESE arm (N7/N10).** On
   O-Bassoon the Chinese arm **grew** (dy=+3 accumulating to +20px on
   `.tuning-controls-panel`), which wants a line-height ratio, not a floor. The
   shrink-and-floor rule held on the FRENCH arm instead.

8. **`undeclared-font` was expected quiet.** It read **11** on O-Bassoon — a
   real form-4 defect Task 1 introduced by putting Han into buttons whose
   computed stack is `Arial`.

9. **`resolvePlaywright()` returns the MODULE, not a path.** Passing it to
   `require()` throws `ERR_INVALID_ARG_TYPE: The "id" argument must be of type
   string. Received an instance of Playwright2`. Noted for anyone writing an
   ad-hoc probe against `scripts/serve-ui.js`.

10. **zsh does not word-split `set -- $P`** — the loop form in several recorded
    commands silently produced `plugins/O-Bassoon Resources/ui/index.html` as a
    single path. Already in the repo memory as
    `pattern_zsh_no_word_split_backup_loop_strays`; it fired again here.

---

## 9. Handed forward to Task 3's deferred-items

1. **Wave 4f inherits a sized zh-Hans geometry debt, not just table rows.** When
   O-Bowed, O-Contrabass, O-Reed and O-Wind gain the `zh-Hans` column on the 37
   keys, each will need **the same two O-Bassoon-only pins** this task measured:
   the `line-height: 1.11` ratio block (11 selectors, including the three button
   classes) and the CJK tail on those same buttons. The numbers are in §5c/§5d
   and transfer directly; the `min-width` floors already landed and are
   language-independent.
2. **The 17-of-37 coverage hole** (§7) is the same on all five and will not
   close by adding table rows. Closing it needs states that change the generator
   type and select the Rotation visualisation; 9 of the 10 `<option>` captions
   are unmeasurable by any DOM probe and should be recorded as such rather than
   chased.
3. **O-Bassoon's 3 residual `line-height-normal` findings** are its own
   `.tab-btn` (声音 / 调音 / 关于), pre-existing and outside this task's surface.
4. **Verify arm 2 of this plan is inert** (§8.1) and should not be copied into a
   later plan as a state-liveness check.
5. **`.interval-list-header` wraps in English at any column narrower than about
   130px.** Four of five consumers hit it. Any future consumer of
   scala-tuning-engine that mounts the panel in a narrow column will need the
   same floor, and the shared CSS still cannot carry it.

---

## 10. Commit

**`d848337a`** — `fix(tuning-panel): drive the localized shared panel from four
state files and floor the geometry it moved`

Path-scoped (`git commit -F <msg> -- <explicit paths>`, message before `--`).
9 files changed, 265 insertions, 1 deletion. `git branch --show-current` and
`git status --short` re-checked immediately before. Another session's
` M .gsd/dispatch-isolation-sentinel.json` was present throughout and was **not**
swept in. The untracked `.planning/quick/260906-s71-…/` directory was not
committed. No tag.

```
plugins/O-Bassoon/Resources/ui/index.html        | 92 ++++++++++++++++++++++++
plugins/O-Bassoon/tests/i18n-states.json         |  4 ++
plugins/O-Bowed/Resources/ui/index.html          | 39 ++++++++++
plugins/O-Contrabass/Source/ui/public/index.html | 39 ++++++++++
plugins/O-Contrabass/tests/i18n-states.json      |  4 ++
plugins/O-Reed/Resources/ui/index.html           | 39 ++++++++++
plugins/O-Reed/tests/i18n-states.json            |  4 ++
plugins/O-Wind/Resources/ui/index.html           | 39 ++++++++++
plugins/O-Wind/tests/i18n-states.json            |  6 +-
```

`plugins/O-Bowed/tests/i18n-states.json` is deliberately absent — its tuning
state already existed and was verified to fire rather than duplicated.
