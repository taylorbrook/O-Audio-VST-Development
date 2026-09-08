---
task: 6
plugin: O-Orbit
phase: quick-260907-qda
wave: 4g
status: complete
shipped_version: "1.3.0"
bumped_from: "1.2.3"          # the CMakeLists VALUE via the two-arm reader, NOT the 1.2.2 PLUGINS.md says
rows: 125                     # 2 x 34 I18N + 57 LABELS — the arithmetic reproduces the emitter exactly
entries: 91                   # 34 I18N + 57 LABELS — one `reviewed:` flag per ENTRY, not per row
reviewed: bt
requirements: [ZH4G-06, ZH4G-07, ZH4G-08, ZH4G-11]
commits:
  - 0efb343e   # the 'mt' table, the body correction, the endonym, the WHOLE-PAGE face naming + CJK tail, the codec, the 13 geometry rules
  - 7ae50ad8   # promotion to 'bt', version bump, CHANGELOG
false_predictions: 6
re_authorings: 1
correction_rounds: 2
submodule_before: "b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af (v1.3.4)"
submodule_after:  "b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af (v1.3.4)"
corpus_after: "5789 of 5789 rows, 44 of 44 plugins carrying zh — read live 2026-09-07"
---

# Task 6 — O-Orbit 1.2.3 → 1.3.0, Simplified Chinese

125 rows at `reviewed: 'bt'`, shipped as **1.3.0**, built and installed. Two path-scoped
commits, the submodule guard run before each. Every gate green. **This task closes the
corpus: 44 of 44 plugins, 5789 of 5789 rows.** `auval` deliberately not run — Task 7 owns
the single cold sweep.

The three things this plugin was picked out for all held, and two of them are now
measurements rather than predictions.

---

## 1. THE SUBMODULE — unmoved, proved per commit and re-proved after the build

| | |
|---|---|
| `git submodule status` **before** any edit | ` b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af plugins/O-Orbit/libs/SAF (v1.3.4)` — **leading space = clean** |
| `git status --short -- plugins/O-Orbit/libs/SAF` before | **0 lines** |
| guard runs | **2 of 2 commits**, immediately after `git add` and before `git commit` — never once at the start |
| guard verdict | `submodule guard: clean` on both |
| per-commit proof | `git show --name-only` on each commit, `grep -c "^plugins/O-Orbit/libs/SAF"` → **0, 0** |
| `git submodule status` **after the build** | ` b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af … (v1.3.4)` — **unchanged** |
| `git status --short -- …/libs/SAF` after the build | **0 lines** |
| gitlink staged as a bare entry | **never** — neither commit lists `plugins/O-Orbit/libs/SAF` |

**The CMake claim got its first localization-wave test and it holds.**
`plugins/O-Orbit/CMakeLists.txt:12` reads `add_subdirectory(libs/SAF/framework saf_build)`;
the second argument is the BINARY directory, so under the out-of-source tree the SAF
artefacts land at **`build/plugins/O-Orbit/saf_build`** — verified on disk after the build —
and nothing is written inside `libs/SAF`. That was a claim about CMake's behaviour at
planning time; it is a measurement now.

I never used `git commit -- plugins/O-Orbit`. Both commits name the **individual files** in
the pathspec, so the submodule path was never inside the pathspec in the first place — the
guard is the belt and the file-level pathspec is the braces. Neither commit stages a single
byte outside `plugins/O-Orbit/`, and neither deletes a file.

---

## 2. THE WHOLE PAGE OFF A STACK NAMING NO INSTALLED FACE — the defect measured, then measured away

**The census, not a grep, is the instrument.** Comment-stripped over `index.html` +
`css/styles.css`:

```
14  font-family: Garamond, 'EB Garamond', serif
```

**Fourteen sites, ZERO `font-family: inherit`, ONE distinct stack — and `index.html`
declares none of them**, so a census run against the markup alone reads 0 and looks clean.
K10 and precondition (h) confirmed exactly.

### The CDP reading, which is the only one that settles it (F5)

`CSS.getPlatformFontsForNode` through a Playwright CDP session, on the served page at the
800x600 shipping frame, on all three language arms. **The installed-family table was not
consulted for the verdict** — it proves presence and never absence.

**BEFORE — on the tree as found:**

| arm | `h1` "O-Orbit" | resolved face | width |
|---|---|---|---|
| en | ASCII | **Times x7** | 78.89 |
| fr | ASCII | **Times x7** | 78.89 |
| **zh-Hans** | **the same ASCII string** | **Songti SC x7 — a CHINESE face** | **79.58** |

`#preset-name` "Default" — an exempt English string — did the same thing: Times, Times,
**Songti SC**. So the must-have *"no ASCII text changes face when the page language
changes"* was **FALSE on this plugin on the tree as found**, and it was false by 0.69px of
measurable width on the wordmark, not merely in theory. The bare `serif` was the only
surviving member of the stack, and Chromium rebinds a bare generic against the document
`lang`.

**AFTER — `Garamond, 'EB Garamond', Times, 'Times New Roman', 'PingFang SC', 'Microsoft
YaHei', serif` at all fourteen sites:**

| probe | en | fr | zh-Hans |
|---|---|---|---|
| `h1` "O-Orbit" | Times x7, w 78.89 | Times x7, w 78.89 | **Times x7, w 78.89** |
| `#preset-select` | Times / Lucida Grande | same | **same** |
| `#preset-name` "Default" | Times x7, w 101.89 | same | **Times x7, w 101.89** |
| `.value-display` "0.50 Hz" | Times x7, w 31.67 | same | **Times x7, w 31.67** |
| `.group-label` | Times x6 "Motion" | Times x9 "Mouvement" | **蘋方-簡 x2 "运动"** |
| `.param-container label` | Times x4 "Path" | Times x11 "Trajectoire" | **蘋方-簡 x2 "路径"** |
| `#preset-save` | Times x4 "Save" | Times x6 "Enreg." | **蘋方-簡 x2 "保存"** |

`蘋方-簡` is PingFang SC under its own name. **Latin resolves to Times on every arm; Han
resolves to PingFang SC.** The two are now decided by what the node contains, not by what
the document claims to be written in.

### What the en and fr arms DID — the prediction this task was exposed on

**Nothing. Measured, not predicted, on seven probes across both Latin arms: identical face
and identical width to the pixel, before and after.** The reason is that the face named
first — `Times` — is exactly the face the bare generic was already resolving to on this
machine. `Times New Roman` sits behind it for the Windows arm, and `Garamond` /
`'EB Garamond'` stay ahead of both as the Windows and print intent, exactly as the other
five plugins in the wave keep their absent Garamond members.

**The naming choice is load-bearing and it was made from the CDP reading, not the table.**
`system_profiler` reports `Times` **0** and `Times New Roman` **4** on this machine. Had I
trusted the table I would have named `Times New Roman` first and moved both Latin arms onto
a different face. F5's warning is not abstract here: it is the difference between a repair
and a regression.

Verified by **re-running the screen**, never by reading the CSS back:

- `undeclared-font` **0** against a **non-empty** input of **62 visible Han-bearing nodes**;
- a scripted check that **0 of those 62** resolve without a CJK face;
- **0 nodes on a bare generic** across all three arms;
- **1 distinct computed stack per arm, and it is the same stack on all three** — a stack
  that differs between `en` and `zh-Hans` is the rebinding this repair removes.

`--symbol-font` has **no subject on this plugin**: `grep -c symbol-font` returns **0**, so
N3 is a recorded non-defect rather than a skipped check. There is no form-4 site either —
every one of the fourteen sites carries the house stack and no visible node declares no
family.

---

## 3. NINETEEN NEVER-VISIBLE LABELS — recorded as structurally unmeasurable, not chased

`37 of 56 [data-i18n] elements were VISIBLE in at least one state; 19 never became
visible`, **before and after, unchanged**. The largest count in the rollout.

**The gate prints twelve and truncates** (R2's shape, the same as `--ingest`'s default
twelve), and the twelve it prints are:

```
#layout-select>option:nth-child(1)
#path>option:nth-child(1) … nth-child(5)
#tempo_sync>option:nth-child(1), (13), (14), (15)
#speaker_layout>option:nth-child(1), (2)
```

**Read the COUNT, not the list.** The seven it did not print are not absent. Every one of
the nineteen is an `<option>` inside a collapsed `<select>` — this plugin has **seven**
`<select>` elements — and an `<option>` in a collapsed `<select>` has **no box in any
state**. They are structurally unmeasurable by any DOM geometry probe, not merely
unmeasured (N8, D4). **No state was added to force them visible**, because none can.

**`undeclared-font` cannot see their Han either, and the SUMMARY has to say so rather than
cite its zero as evidence** (F12). `measure-ui`'s `han` flag is the node's own text plus
`data-tip`, `data-tip-title` and `aria-label`; a collapsed `<select>`'s selected-option text
is none of those, so `#layout-select`, `#path`, `#tempo_sync` and `#speaker_layout` read
`han: false` however much Chinese they paint. **The screen is not the oracle for those four
controls.** They are covered by the whole-page repair anyway, and that is a decision about
what the control PAINTS — every one of them paints Chinese option text — not a reading off
a screen that cannot see it.

---

## 4. `js/modules/preset-manager.js` — MEASURED, NAMED, REPORTED UNCHANGED (K8, s71 D2)

`plugins/O-Orbit/Resources/ui/js/modules/preset-manager.js`, **447 lines**.

| | |
|---|---|
| **sha1** | **`d15e751b9c8650088fb841a7e36b0ae05221da3c`** |
| **a SECOND copy in this same plugin** | `plugins/O-Orbit/Source/ui/public/modules/preset-manager.js` — **447 L, the SAME sha1** |
| O-Polystutter's copy | `0142c1daac569f0c5e45574f8a7df06ea38b1597`, **406 L** |
| divergence from O-Polystutter | **CONFIRMED by sha1 AND by line count** — 41 lines apart |
| `check-i18n` `[12]` | **`1 module(s): js/app.js`**, before and after — quoted verbatim |
| `git status --short -- .../js/modules/` | **0** — not one byte changed |

**The path is spelled `js/modules/` here and `modules/` on the other three carriers**, so a
census keyed on one spelling is blind to the other. Censused with `find` over the whole UI
root: **three** JS files (`js/i18n.js`, `js/app.js`, `js/modules/preset-manager.js`), and a
second `find` over the whole plugin turned up the stray fourth copy under `Source/ui/public/`
that `i18n.js`'s own header comment names as neither embedded nor served —
`CMakeLists.txt:43-48` configure-files the `Resources/ui/` copy explicitly for exactly that
reason.

### Its strings, and which reach this page

```
:79, :177   'Default'                       -> REACHES THE PAGE
:297        'Loaded Preset'                 -> REACHES THE PAGE
:421        title="Previous preset"         -> DEAD here
:423        title="Next preset"             -> DEAD here
:424        title="Load preset from file"   -> DEAD here
:425        title="Save preset"             -> DEAD here
:422        >Default<  (in the innerHTML)   -> DEAD here
```

**The five `createPresetBar()` strings are dead, and that is measured.** `app.js:1157`
carries the comment *"Constructor + explicit DOM refs (never createPresetBar(), which
innerHTML-wipes and has no delete button)"* and `:1191-1192` reads
`const els = ['preset-prev', 'preset-next', 'preset-name', …].map(byId)`. The
**comment-stripped** grep is the proof rather than the comment: `grep -c createPresetBar`
returns **1** on the raw `app.js` and **0** on the comment-stripped copy, so the only
occurrence in the whole file is the comment saying it is never called. The markup keys the
buttons itself at `index.html:75-76` — `data-i18n-aria="preset-prev"` and
`data-i18n-aria="preset-next"`, both present in this plugin's `LABELS`.

### The `#preset-name` verdict, and the markup comment honoured

`index.html:70` says `#preset-name` *"is NOT a `[data-i18n]` element and must never become
one: preset-manager.js owns that node, the name it writes IS the JSON filename (D-02), and
a sweep writing it would fight the module over one text node."* **Read and honoured.** The
element is untouched, `'Default'` remains an `I18N_EXEMPT` entry citing the filename, and
the language body's exception list still says preset names stay in English — which is the
same claim, re-verified against the same decision.

**`'Loaded Preset'` at `:297` is NOT exempt and NOT keyed** and it does reach `#preset-name`
after a successful file load. Carried as a deferred item, identical in shape to the one
Task 5 filed against O-Polystutter's copy.

**Verdict: REPORTED, NOT FIXED**, for s71 D2's reasons restated against this tree — eighteen
consumers, copies that provably diverge, no gate in this repo that opens any of them, and a
half-keyed layer that looks done being worse than an unkeyed one. **No gate was invented.**

---

## The false-prediction ledger — six, marked per plan prediction

### FALSE-1 — precondition (j): `git status --short | grep -c "O-Strata"` reads **0**, not 2

Sixth reading of one line across one working period. Not a regression: the number describes
another session's progress on an unrelated plugin, and that session has since committed the
files. **0** at my preconditions and **0** at my self-check. Nothing under `plugins/O-Strata/`
was ever staged.

### FALSE-2 — precondition (k): the positive control CANNOT fire before the table lands

Tasks 2–5's FALSE-2, confirmed a fifth time. Pre-work `js/i18n.js` held **0** Han code
points, so a control fired against it necessarily prints nothing and a green there would
mean the probe was broken. **Its correct home is the verify step**, and there it fired:
the negative printed nothing across every `.h`/`.cpp` under `Source/`, and the control
printed `plugins/O-Orbit/Resources/ui/js/i18n.js` on the same run.

**And on this plugin the scope of that negative grep is smaller than on the other five, so
it is stated rather than implied:** `plugins/O-Orbit/Source/` does **not** contain the UI —
the UI root is `Resources/ui/` — so the grep covers the C++ only. That is exactly the scope
the assertion needs (no Chinese in the C++), but a reader comparing tasks should know the
denominator differs.

### FALSE-3 — the R3 glossary denominator is **65**, not 28

The plan records `NONE (28 matched)`. Re-fired over captions **and** tooltip titles:
**65 glossary-matched of 91 screened.** The same denominator difference Tasks 1–5 recorded
(22→50, 27→76, 23→69, 16→41, 34→69) — the planning screen matched captions only.
**Confirmed on all six tasks; it is a property of the planning screen, not of any plugin.**

Two root-sharing pairs surfaced where the planning screen reported none:

```
root 预设  [["preset-select|title","Preset"],["aria.presets|label","Presets"]]
root 删除  [["preset-delete|title","Delete"],["label.delete|label","Del"]]
```

`删除` is **a caption and its own tooltip title** on `#preset-delete`, which the ship bar
excludes by name. `预设` is `Preset` against `Presets` — the singular/plural of one English
noun on the browse button and on the listbox it opens, and Chinese does not inflect for
number. **Verdict after the exclusions: NONE**, matching the plan. The mismatch is a
denominator, not a finding.

### FALSE-4 — the emitter has **no product-name control** in this version

Tasks 3 and 5's finding, confirmed. `grep -cin "product" scripts/i18n-zh-backtranslate.js`
returns **0** — the control does not exist, so there is no count to adjudicate and no line
to read. **The substance still holds**, so the check was made by hand: occurrences of
`Orbit` or `O-Orbit` in the emitted zh column, **0**; the wordmark lives in an `<h1>` and is
the **first** `I18N_EXEMPT` entry, so it never enters the table. There is no common-noun arm
to adjudicate — *orbit* as an English word never appears in the Chinese, because the path
option that means it is 环形.

### FALSE-5 — the K2 / D5 baseline is GONE, confirming Task 5's FALSE-6

| | plan baseline | measured here, at preconditions |
|---|---|---|
| `i18n-zh-lint` repo-wide | exit 2, 0 across 0, 1 unreadable | **exit 0, 0 findings across 44** |
| `i18n-fr-lint` repo-wide | exit 2, 0 / 44, 1 unreadable | **exit 0, CLEAN** |
| `check-i18n` repo-wide | 43 localized | **`ALL CHECKS PASS — 44 localized plugin(s)`** |
| emitter | 4621 of 5441, 37 carrying zh, 1 unreadable | **5664 of 5789, 43 carrying zh, 0 unreadable** |

O-Strata is a fork of O-Prism and arrived already localized (commit `0e838512`).
**Unlike Task 5, `check-i18n` repo-wide was GREEN at my preconditions as well as at my
self-check** — that session finished its assertion-10 work in the interval. Both readings
are recorded because either alone would mislead.

### FALSE-6 — the corpus closes at **44 of 44**, not the plan's 43 of 43

K1's framing — *"43 of 43, with O-Strata named explicitly as the 44th and excluded for
having no i18n.js"* — is **superseded**. O-Strata has an `i18n.js`, it carries zh rows, and
it is readable. **`check-i18n`'s 43-vs-emitter's-37 convergence still happened; it just
converged on 44.** Read live at 2026-09-07 22:35 after both my commits:

```
zh-Hans strings: 5789 of 5789 rows
BELOW SHIP BAR — at reviewed:'mt' or unflagged: 0
ALL CHECKS PASS — 44 localized plugin(s)
plugins with an i18n.js: 44   of which carrying zh rows: 44
```

`5664 + 125 = 5789` **exactly** — this task's 125 rows are the last of the corpus. **Task 7
must read this live and state the reading with its timestamp**; `43 of 43` and `5441` are
both dead numbers.

**Two predictions that were TRUE and worth recording as such**, because five waves of false
ones make a true one easy to overlook:

- **the one-arm CMake reader works here.** Both arms return `1.2.3`, as the plan says. Fired
  both anyway (R9); the two-arm reader is used unconditionally because a reader that only
  works where it was written is not a reader.
- **the pin surface is exactly as measured**: 10 `min-width`, 2 `min-height`, 3
  `line-height`, 16 `letter-spacing`, 4 `white-space`, comment-stripped over the pair.

---

## Measured numbers, before and after

### Geometry — `check-ui-labels`, zh arm

| | non-label elements moved |
|---|---|
| table + font repair landed, no pins | **64 / 66 / 66 / 73** across the four walked passes |
| after ten zh-scoped line-box ratios | **8**, at rest only; all three click states already green |
| **after the four format-chip pins** | **0** — `== ALL CHECKS PASSED ==`, exit 0 |
| after promotion, the version bump and the build | **0**, re-run, unchanged |

**Two rounds.** The **fr arm was green at every single measurement** — 8 of 8 green
`GEOMETRY DIFF` assertions on the two non-English arms in the final run — and the
never-visible count never moved off **19**.

### The ten line-box ratios, every one from that element's own English CONTENT box

Rect height less its own padding and border, one line (verified), divided by its own
font-size. **None of the ten selectors carried a `line-height` or `min-width` declaration
before this block** — the file's only three pre-existing `line-height` declarations are on
`.preset-nav`, `.gear-btn` and the tooltip, none of them a mover (R8, grepped one by one).

| selector | fs | en rect | pad t/b | border t/b | content | ratio | zh before |
|---|---|---|---|---|---|---|---|
| `.preset-btn-hdr` | 10 | 21 | 3/3 | 1/1 | **13** | **1.3** | 22 |
| `.preset-btn` | 10 | 19 | 2/2 | 1/1 | **13** | **1.3** | 20 |
| `#file-buttons button` | 10 | 19 | 2/2 | 1/1 | **13** | **1.3** | 20 |
| `#layout-library button` | 10 | 19 | 2/2 | 1/1 | **13** | **1.3** | 20 |
| `.settings-toggle` | 10 | 19 | 2/2 | 1/1 | **13** | **1.3** | 20 |
| `#view-toggle` | 12 | 24 | 4/4 | 1/1 | **14** | **1.1666667** | 27 |
| `#downmix-badge` | 9 | 14 | 1/1 | 1/1 | **10** | **1.1111111** | 17 |
| `.settings-label` | 11 | 13 | 0/0 | 0/0 | **13** | **1.1818182** | 16 |
| `.param-container label` (x18) | 11 | 13 | 0/0 | 0/0 | **13** | **1.1818182** | 16 |
| `.group-label` (x3) | 14 | 17 | 0/0 | 0/0 | **17** | **1.2142857** | 20 |

**F6 has a large subject here and it is exactly the value F6 records.** Five of the ten are
form controls and all five land on **1.3** — a 10px `<button>` has a 13px content box
because the UA `font` shorthand resets `line-height` on a form control. M8's leaf table
would have given 1.1 and left every button 1px tall.

**And M8's leaf table does not transfer to this plugin at all, which is a finding rather
than a caveat.** Its 11px row records a 12px content line box; both 11px leaves here measure
**13**. The cause is the same one this task's font work turned up: M8 was measured on pages
whose Latin resolves through a `'Times New Roman'` stack, and this page's Latin resolves to
**Times**, a different face with a different `normal` line box. **Deriving each ratio from
the element rather than looking it up is what made this correct, and on this plugin it was
not optional.**

### The format-chip row — one shrink, three growths, and they do not cancel

The eight speaker-format chips are a flex row in a fixed 800px frame. Measured, en identical
to fr on all eight:

| chip | en | fr | zh | Δ |
|---|---|---|---|---|
| 0 `Stereo` / `Stéréo` / `立体声` | 50.78 | 50.78 | **42.17** | **−8.61** |
| 1 `Quad` / `Quad` / `四声道` | 40.89 | 40.89 | **42.17** | **+1.28** |
| 2–5 `5.1` `7.1` `5.1.4` `7.1.4` | 24 / 24 / 32.5 / 32.5 | same | same | 0 |
| 6 `Hex` / `Hexa` / `六声道` | 40.00 | 40.00 | **42.17** | **+2.17** |
| 7 `Oct` / `Octo` / `八声道` | 40.00 | 40.00 | **42.17** | **+2.17** |
| **row** | **312.67** | **312.67** | **309.69** | **−2.98** |

The arithmetic closes: `−8.61 + 1.28 + 2.17 + 2.17 = −2.99`, and the gate reported the row
at `dw = −3.0` with the four digit chips at `dx = −7.33` — which is `−8.61 + 1.28` exactly.
**The four digit chips are the ones the gate names because they are the only ones in that
row that are not `[data-i18n]` elements**; the four worded chips moved the same way and were
invisible to `[7]` for that reason alone.

**A floor alone would not have closed it** — three of the four grow. The four Han chips are
pinned to their exact measured English boxes under `html[lang="zh-Hans"]`, with the Latin
`letter-spacing: 0.5px` trimmed (it buys nothing on a Han run and costs 1.5px across three
glyphs) and the side padding dropped 1px, leaving 32px of content area in the narrowest of
the four boxes against a 30.67px Han run. **The four digit chips are deliberately excluded
from that selector list**: their text stays Latin under Chinese and trimming their tracking
would have moved them.

### R8 / F7 done properly — every floor REMOVED, measured, restored

All six real `min-width` declarations were removed **entirely**, the natural boxes measured
on all three arms, and the file restored from a byte-verified backup — **never**
`git restore`, because the tree held uncommitted work at the time
(`pattern_negative_control_checkout_wipes_uncommitted_fix`). Restored sha256 prefix
`87d9faa9` matched the backup exactly.

| selector | floor | en natural | fr natural | **zh natural** | verdict |
|---|---|---|---|---|---|
| `#view-toggle` | 170 | 132.13 | 169.31 | **97.52** | **LIVE on all three.** French sets it; without it Chinese shrinks **72.48px** |
| `#file-buttons button` | 74 | 59.30 | 73.08 | **39.45** | **LIVE.** Chinese shrinks 34.55 |
| `.preset-btn-hdr` | 58 | 42.83 | 54.84 | **37.45** | **LIVE.** Chinese shrinks 20.55 |
| `#layout-library button` | 53 | 40.83 | 52.84 | **35.45** | **LIVE.** Chinese shrinks 17.55 |
| `#preset-select` | 130 | 61.11 | 61.11 | **61.11** | LIVE, identical on all three — its content is the exempt English `Default` |
| `.preset-btn[data-preset="6"/"7"]` | 40 | 32.06 / 31.50 | 39.78 / 39.05 | **40.00** | live on en/fr; the zh arm sits on this task's own `width` pin |

**Six French-era floors, every one of them live on the Chinese arm, every one absorbing a
17px-to-72px Chinese shrink that would otherwise have moved the header and the editor
toolbar.** No new pin was appended to any of those selectors, so F7's un-pinning hazard has
**no subject** here. The four `min-width: 0` declarations were left alone and are recorded as
**not floors at all** — the flex-item default is `auto`, so `0` explicitly permits shrinking
below content.

### `measure-ui` screens — the K12 vacuum, closed

| screen | baseline (VACUUM) | after |
|---|---|---|
| `undeclared-font` | 0 — **input EMPTY** | **0 against 62 visible Han-bearing nodes**, 0 of which resolve without a CJK face |
| `line-height-normal` | 0 — input empty | 38 → **1** |
| `wrap-count` | 0 (en-vs-fr only) | **0** — baseline 0, held |
| `svg-font-attr` | 0 carriers | 0 carriers |
| identity | 360 / 180 / 88 | **543 / 181 / 88** (a third language pass) |
| leaves ESTIMATED | **56 of 59** | **56 of 59 — unmoved**, because every pin is zh-scoped |

**Every zero after the table landed is a measurement. Every zero before it was a vacuum.**

**The ESTIMATED denominator did not move**, on the wave's most exposed page (95%). That is
the point of scoping the pins under `html[lang="zh-Hans"]`, taken from Task 5's
recommendation: there is no F8 divisor movement to disentangle from a real change, because
`wrap-count` compares en against fr and neither arm was touched.

**The one `line-height-normal` residual, named with its measurement (N1, F4):**

```
zh-Hans  #layout-name  (input)  ""  font-size=10px   enH = 19   zhH = 19
```

It is F4's shape exactly — the screen's "leaf nodes only" filter is `kids === 0` with **no
text requirement**, so a text-free `<input>` whose `han` comes entirely from its Chinese
placeholder lands in the residual set. **It cannot move and it cannot be pinned to any
effect**, and `enH == zhH` is the evidence. The residual count is not a caption count.

### The state-EFFECT assertion — all THREE click states, in all three languages (N1, K16)

`tests/i18n-states.json` was re-read for the state KIND rather than inherited: **three
clicks, no `eval`.** K16 is true on this plugin.

| state | target | rect | `elementFromPoint` | took effect |
|---|---|---|---|---|
| settings popover OPEN | `#gear-btn` | 20x20 @ 590,6 | target or descendant | **YES** — visible `[data-i18n]` 26 → 29 |
| hover help ON | `#help-toggle` | 46x19 @ 553,69 | target or descendant | **YES** — the FACE flips, count unchanged by design |
| SPEAKER LAYOUT VIEW | `#view-toggle` | 170x24 @ 618,4 | target or descendant | **YES** — 29 → **37**, the fifteen labels the state file names |

**Nine assertions, no `null` rects**, so there was nothing to discriminate as a scrolled
target and no coverage hole. **The `#help-toggle` state is the one whose effect a count
cannot show**, so its face was probed directly rather than inferred:

```
en       #help-toggle  "Off"  -> "On"
fr       #help-toggle  "Non"  -> "Oui"
zh-Hans  #help-toggle  "关闭" -> "开启"
```

`measure-ui --verbose`: **0** `did not resolve` / `unresolved` / `skipped state` — all three
states applied, including `#view-toggle`, behind which 15 of this plugin's labels live.
**No state scrolls**, so no state moves the ones after it. The ORDER matters and was checked:
the gear opens the popover that the help toggle then lives inside.

---

## Blind-read accounting

| | |
|---|---|
| rows dispatched | **125 of 125**, 0 withheld from review |
| chunks | 2, split on the CONCEPT (F17), each from its OWN emit → its own salt |
| salts | `0e8293819f3e9050a44ce48c7a409f8b` (A) and `fbb5f9ddddeb313b270c8bcf8e114ae6` (B) |
| ids shared between the two chunks | **0**, measured |
| ids returned identical AND IN ORDER (M12) | **YES**, both chunks, diffed position by position |
| Han surviving into returned English | **0**, both chunks |
| malformed returned rows | **0** |
| dispatched zh column vs the tree | **byte-identical on all 125 rows**, joined through the manifest |
| dispatched zh column sha256 | `dd50cb8fd841586e8d6e2fb60a8eeeeddf0ad8ae5323d83bfef4485828b5262e` |
| product-name control | **does not exist in this version** — adjudicated by hand, 0 hits |
| triples read with `--verbose` (R2) | **125 of 125** |
| **rows re-authored** | **1** (2 strings) |
| correction rounds | **2**; round 2 corrected nothing, so there is no round three (R4) |
| forward model | `claude-opus-5` |
| round-1 reverse model | **`claude-sonnet-5`** — read back from the process, not the alias |
| round-2 reverse model | **`claude-haiku-4-5-20251001`** — a DIFFERENT model, fresh salt `de345f10770d6d6882576e891305504a`, 0 ids shared with A or B |
| refusal controls fired | **2 of 2**, against a properly provenanced emit (F10) |
| termNote exemptions | **0** |

**The concept split:** chunk A was the hover-help surface — 34 titles + 34 bodies = **68** —
and chunk B the on-page captions — **57**. 68 + 57 = 125.

### Both refusal controls, verbatim

1. `--provenance` omitted, correct manifest →
   `REFUSED: back-translation provenance is missing or identical to the forward pass — this triple proves nothing`
   `  no --provenance was given.`
2. The OTHER batch's manifest, **with** `--provenance` →
   `joined: 0   unjoinable ids: 68`
   `WRONG MANIFEST FOR THIS BATCH: all 68 returned ids are unjoinable, not one.`

The second reaches its own refusal only because the first condition was satisfied. Run
without `--provenance`, it refuses for reason 1 and proves nothing about the manifest.

`--forward-provenance` was passed a real STRING naming what produced the Chinese, at which
commit, and when (F9) — confirmed in the ingest's own echo of `forward pass recorded at
emit`.

### The one re-authoring, and why — the discriminator is the page

**`label.pathOrbit` shipped `环绕`.** The two readers, given the same morpheme in two
different contexts, returned two different English words and **neither was "Orbit"**:

| chunk | context | en′ |
|---|---|---|
| B (captions only) | none | **Surround** |
| A (the `path` body, in a sentence) | five path names in a row | **Circle** |

Strictly, **there is no collision on the page**: nothing else on this page renders `环绕`,
and Chinese for surround sound is `环绕声`, which the layout controls do not use — they say
`立体声 / 四声道 / 六声道 / 八声道` and `5.1 / 7.1 / 5.1.4 / 7.1.4`. By the stated
discriminator this was acceptable.

**It was re-authored anyway, and the reason is written down.** A path option that a Chinese
reader parses as *surround*, sitting one control away from a Speaker Layout dropdown listing
surround formats, is a false friend the blind read surfaced from two independent directions.
`环形` was chosen over `轨道`: 轨道 is the literal "orbit" but overwhelmingly reads as
**track** in audio software, and this page already says `音轨` for *audio track* in two
bodies — swapping one ambiguity for a worse one. `环形` reads as a path SHAPE beside
`钟摆 / 线性 / 漂移 / 乒乓`, which are all shapes or behaviours.

The correction round confirms the fix, and it is a fresh reader, a fresh salt AND a
different model:

```
label.pathOrbit   环形                              -> "Circular"
body|path         运动的轨迹：环形绕圈运行，…        -> "Motion trajectory: circular orbiting, …"
```

The surround reading is gone in both the bare caption and the sentence.

### Accepted drifts, each with its written reason

Every one is a glossary root or a grammatical property of Chinese, and **none produces a
collision on the page**.

- **No plural inflection, three times.** `Presets` → 预设 → *Preset*; `Layouts…` → 布局… →
  *Layout…*; `{from}ch → {to}ch` → *Channel*, not *Channels*. Chinese does not inflect for
  number and the glossary roots are singular.
- **`Del` and `Delete` are one word in Chinese.** `label.delete` (the 34px header band's
  abbreviation) and `preset-delete|title` (the full verb) both render 删除, and the reader
  returned *Delete* for the caption. They are **the caption and the tooltip title of the same
  button**, which the ship bar excludes by name, and Chinese has no shorter form of 删除.
- **`Hex`/`Hexaphonic` and `Oct`/`Octaphonic` collapse to one string each.** 六声道 and
  八声道 serve the toolbar chip and the dropdown option. **Two different English keys, one
  Chinese rendering, and it is correct**: they name the SAME speaker format, the English
  distinction is purely an abbreviation, and Chinese has no shorter form. Recorded rather
  than "fixed", because inventing a shorter one would name a format that does not exist.
- **`Sure?` → 确定？ → *Confirm?***. The armed face of both two-click delete buttons.
  确定？ is what Chinese interfaces print on a confirm affordance; the English is an
  elliptical *Are you sure?* and the Chinese is the same speech act.
- **`Spatial` → 空间 → *Space***. A group heading. 空间 is the standard Chinese heading for
  the spatial section of an audio interface; the reader's *Space* is a lexical reading of the
  noun, and the heading names the same group either way.
- **`Center Diverge` → 中置发散 → *Center Spread*** and **`Atten Curve` → 衰减曲线 →
  *Falloff curve* / *Attenuation Curve***. Both are the glossary roots, both name their own
  control, and nothing else on the page shares either.
- **`L+R Split` → L+R 分离 → *L+R Separate***. 分离 is the standard term; *split* and
  *separate* are the same operation and no other control says 分离.
- **`ui.off` and `label.syncOff` both render 关闭.** They are the SAME English word `Off` on
  two controls — a same-English pair, which the ship bar excludes. **The French forked them
  deliberately** (`Non` on the 46px pill, `Désactivé` in the 84px dropdown) for a size reason
  that has **no Chinese subject**: 关闭 is two glyphs and fits both boxes, and the pill's
  measured English content box is 13px against a 20px Chinese line box that the 1.3 ratio
  already returns to 19px. The two French `termNote`s explaining that fork are untouched and
  still true of the French.

---

## Stale-body and dead-class verdicts (K13, C7)

The correction was made FIRST and the file **re-read** before a single Chinese row was
authored. The Chinese was written from the corrected English, not from the English the plan
quotes.

| class | verdict | evidence |
|---|---|---|
| the fixed-pair language enumeration | **LIVE, deleted in en AND fr** | comment-stripped `perl -CSD` probe **2 → 0** |
| its exception list | **KEPT, and re-verified** | below |
| the "and nothing else" settings class | **NO SUBJECT — checked non-defect** | `grep -c "'tip.settings'"` = **0**; `grep -c tips-toggle` in the markup = **0** (this plugin's switch is `#help-toggle`) |
| the "Tuning tab stays English" class | **NO SUBJECT — checked non-defect** | 0 `scala-tuning-engine` refs in CMakeLists; 0 occurrences of *tuning* in the markup |

**The French deletion was built by REGEX, never hand-typed.** The source carries a **U+00A0**
before the semicolon (`disponibles<NBSP>;`) that a typed literal cannot reproduce — dumped
the line's non-ASCII code points first (4 × U+00E9, 1 × U+2019, 1 × U+00E7, **1 × U+00A0**)
and wrote the pattern to match rather than to guess. `reviewed: true` is kept on the French:
the same clause is removed from both arms, so the two still say the same thing.

### The exception list's claim, re-verified before it was preserved

The English body now reads *"The language of the labels on this page and of this hover help.
Value readouts and preset names stay in English."*

**This plugin's exception list names no NUMBER**, so the `<select>`-count re-verification the
other tasks run has no subject here — confirmed rather than assumed: the markup carries
**7** `<select>` elements and the body counts none of them. What it does name is **preset
names**, and that claim is exactly the `#preset-name` design decision at `index.html:70`,
which was read and honoured. **The two are the same fact stated twice, and both were checked
against each other rather than one being assumed from the other.**

The comment above the entry was corrected as prose (C8): it said the body is written to be
true *"in both languages"*, which is false of three. It never spelled the deleted literal, so
there was nothing to un-spell.

---

## Font work — the FACE-NAMING half, applied to a whole page

Censused with `find` and with a comment-stripped concatenation of the PAIR, never against
`index.html` alone — which declares **zero** `font-family` sites and reads as a clean bill.
All fourteen are in `css/styles.css`.

```
BEFORE   14  font-family: Garamond, 'EB Garamond', serif
AFTER    14  font-family: Garamond, 'EB Garamond', Times, 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif
```

**One distinct declared stack before and one after** — this is the rollout's only plugin
where "the font work" and "the page" are the same set. The order is deliberate and every
position is load-bearing:

1. `Garamond`, `'EB Garamond'` — **kept**. The Windows and print intent; absent here, and
   this machine is not the one the suite builds for.
2. `Times` — **the face the bare generic was already resolving to on this machine**, named
   so it is chosen by the declaration rather than by the document language. This is the
   repair.
3. `'Times New Roman'` — the Windows and Linux arm of the same intent.
4. `'PingFang SC'`, `'Microsoft YaHei'` — the CJK tail, **before** the trailing generic (W1).
5. `serif` — now unreachable on this machine, which is the point.

Verified by re-running the screen and by a second CDP pass, **never by reading the CSS
back**, and the other stack populations could not move because there is only one population.

---

## Zeros recorded AS MEASURED

- **gate files: 0.** `find plugins/O-Orbit/tests -name '*.js'` returns nothing; `tests/`
  holds `i18n-states.json` and a `ui-stub/` directory containing a single
  `generic-overrides.json` and **no JavaScript at all**. Fired with `find`, never a glob.
  **No gate was invented** (C5/C6 has no subject).
- **second version source: NONE, classified rather than assumed.** The one
  `JucePlugin_Version` occurrence under the plugin is a **comment** at
  `Source/PluginProcessor.cpp:100`. There is exactly one `CMakeLists.txt` outside the
  submodule and it holds no hex mirror. K6 is an O-simpleGrain property, not a wave one.
- **duplicate keys: NONE**, F13-corrected form (`LANGUAGES` members excluded), `keys=91`,
  before and after. The bare form would have returned `zh-Hans` on every entry.
- **language `AudioParameterChoice`: none** — scanned by declaration, not by containing file.
- **Han under `Source/`: 0**, negative grep across every `.h`/`.cpp`, positive control firing
  on `Resources/ui/js/i18n.js` in the same run. **Scope stated:** `Source/` on this plugin
  does not contain the UI.
- **`zh-Hans` in the comment-stripped `PluginProcessor.h`: 2** (encode and decode), and both
  functions report **THREE-WAY** when matched by name. The doc comment above the codec was
  reworded from *"anything that is not `fr`"* to *"anything it does not recognise"* — it
  described a two-way ternary.
- **`--symbol-font`: 0 occurrences.** N3 has no subject; recorded rather than skipped.
- **`git status --short -- .../js/modules/`: 0.** Not one byte of `preset-manager.js` changed,
  in either of this plugin's two copies of it.
- **`html[lang="zh-Hans"]` blocks before this task: 0.** All thirteen new rules are the first,
  written on the shipped precedent at `plugins/O-Prism/Source/ui/public/index.html:1298`.
- **Z6 budget backfill: unchanged at `3 of 552`.** This task adds none — every geometry
  finding was a line-box growth wanting a ratio or a width change wanting a pin, and neither
  wants a character budget.
- **glossary divergence report: still SIX entries.** This task adds no seventh; the `环绕`
  question was settled inside this plugin rather than by forking a root.
- **tags created: 0.** **`PLUGINS.md` touched: no** (Task 7 owns it).
- **file deletions across both commits: 0.** **Files staged outside `plugins/O-Orbit/`: 0.**

---

## Gate results

| gate | result |
|---|---|
| `check-i18n --plugin O-Orbit` | **exit 0** — `got ["en","fr","zh-Hans"]`, `34 I18N + 57 LABELS` unchanged, `34 tip(s) bound` unchanged, `[12] 1 module(s): js/app.js` unchanged — `js/modules/preset-manager.js` is STILL not among them |
| `i18n-zh-lint --plugin O-Orbit` | **0 findings**, 125 rows / 91 entries, **BELOW SHIP BAR 0** |
| `i18n-zh-lint --self-test` | **10/10** |
| `check-ui-labels --plugin O-Orbit` | **exit 0**, `== ALL CHECKS PASSED ==`, **0 moved** on both non-English arms across all three states, `37 of 56` with **19 never-visible** (unchanged) |
| `measure-ui --report all` | `undeclared-font` **0** against 62 Han-bearing nodes, `line-height-normal` **1** at `enH == zhH`, `wrap-count` **0**, `svg-font-attr` 0 |
| `measure-ui --verbose` | **0** unresolved or skipped states |
| `boot-all-uis --strict-tips` (repo-wide) | **clean 44/44**, 0 warn, 0 failed, **0 DEAD**, **2 late across 1 plugin — O-Bells, the 4e D2 census control, unchanged** |
| `i18n-zh-lint` repo-wide | **exit 0**, 0 findings across **44** plugins, 4329 zh entries |
| `i18n-fr-lint` repo-wide | **exit 0, CLEAN**. No French rendering changed |
| `check-i18n` repo-wide | `ALL CHECKS PASS — 44 localized plugin(s)` at preconditions **and** at self-check |
| corpus | **5789 of 5789 rows, 44 of 44 plugins carrying zh** — read live 2026-09-07 |
| `auval` | **NOT RUN** — deferred to Task 7's single cold sweep, as the plan directs |

---

## Commits

| hash | subject |
|---|---|
| **`0efb343e`** | `i18n(O-Orbit): add Simplified Chinese at reviewed:'mt' — 125 rows, the page off a stack naming no installed face` |
| **`7ae50ad8`** | `i18n(O-Orbit): promote 125 zh rows to reviewed:'bt' after a blind reverse read — v1.3.0` |

Both `git commit -F <file> -- <the individual files>`, options before the `--` (F14.6).
`git branch --show-current` (main) and `git status --short` re-checked immediately before
each, never once at the start. **The submodule guard ran before each and reported clean
both times.** Nothing under `plugins/O-Strata/`, nothing under `.claude/agent-memory/`,
nothing under `.planning/`, and not one byte of `PLUGINS.md` was ever staged. No tag was
created.

---

## Build and install

`./scripts/build-and-install.sh O-Orbit` — the **FOLDER** name; the script resolved the
`OuariconOrbit` target itself, which is one of the two places in this wave where the target
is not the folder name. Run in the background and polled on a sentinel — **exit 0**, all
seven phases green.

```
VST3: ~/Library/Audio/Plug-Ins/VST3/O-Orbit-dev.vst3            22:33
AU:   ~/Library/Audio/Plug-Ins/Components/O-Orbit-dev.component 22:33
```

`CFBundleShortVersionString` read back from the installed AU bundle: **`1.3.0`** — the
version reached the binary, not just the CMakeLists.

**No alternate-variant orphan.** Phase 4's dual-variant sweep emitted no
`⚠ Sweeping ALTERNATE-variant` warning (grep count 0); only the `-dev` bundles are on disk.

**SAF built to `build/plugins/O-Orbit/saf_build`** — outside the submodule, exactly as
`add_subdirectory(libs/SAF/framework saf_build)` promises, and the submodule tree was clean
after the build as well as before it.

**`auval` was NOT run.** Task 7's single cold sweep. Budget ~85 s, not the stale 15-minute
figure.

---

## Deferred items

1. **`'Loaded Preset'` is an unkeyed, unexempt English string that DOES reach this page.**
   `plugins/O-Orbit/Resources/ui/js/modules/preset-manager.js:297` writes
   `this.currentPreset = result.name || 'Loaded Preset'`, and `_updateDisplay()` puts it into
   `#preset-name`. Reachable only after a successful file load, so no static scan sees it, no
   `[data-i18n]` covers it, and no state in `i18n-states.json` fires it. Its sibling
   `'Default'` **is** an `I18N_EXEMPT` entry; this one is not, so the exemption list is
   incomplete rather than the string being deliberately English. **Identical to the item
   Task 5 filed against O-Polystutter's copy — the same defect in two forked files.**
   **Grep-able token: `'Loaded Preset'`.**

2. **O-Orbit carries TWO copies of `preset-manager.js` in its own tree, at the same sha, and
   only one is served.** `Resources/ui/js/modules/preset-manager.js` (embedded, per
   `CMakeLists.txt:43-48`) and `Source/ui/public/modules/preset-manager.js` (**not** embedded,
   **not** served — `i18n.js`'s own header comment says so). The stray copy is a
   maintenance trap: an edit to it changes nothing and looks like it changed something.
   Both are `d15e751b`, 447 L, and both diverge from O-Polystutter's `0142c1da`, 406 L.
   **Grep-able token: `createPresetBar` under `Source/ui/public/modules/`.**

3. **M8's leaf line-box table is FACE-SCOPED and does not transfer across font stacks.**
   Its 11px row records a 12px content box; both 11px leaves on this page measure **13**,
   because this page's Latin resolves to **Times** where the pages M8 was measured on resolve
   to **Times New Roman**. Every ratio here was derived from the element rather than looked
   up, which is why it is right — but the table as recorded reads as machine-scoped and is
   not. **A later wave that looks a ratio up will be wrong on any page whose Latin face
   differs.** **Grep-able token: `M8's measured English content line boxes` in the wave plans.**

4. **`system_profiler` disagreed with Chromium on this machine, in the direction that would
   have caused a regression.** The table reports `Times` **0** and `Times New Roman` **4**;
   Chromium resolves the bare `serif` to **Times** and reports it by that name through
   `CSS.getPlatformFontsForNode`. Naming `Times New Roman` first — which the table alone
   would have licensed — would have moved both Latin arms onto a different face while
   "fixing" the Chinese one. F5 is now a measured claim about this repair, not an inherited
   warning. **Grep-able token: `CSS.getPlatformFontsForNode` in this summary.**

5. **The corpus is CLOSED and the plan's close-out numbers are all dead.** 44 of 44 plugins,
   5789 of 5789 rows, 0 below the ship bar, both repo-wide lints exit 0, `check-i18n` reads
   44 localized. `43 of 43`, `5441`, `37 carrying zh` and `exit 2` are every one superseded.
   **Task 7 must read this live and state the reading with its timestamp.**

---

## For Task 7

- **Read every repo-wide number live.** The corpus closed at **44 of 44 / 5789 of 5789**, not
  43 of 43 / 5441, because O-Strata arrived fully localized mid-wave (`0e838512`). Both
  repo-wide lints and `check-i18n` were **green at my preconditions AND at my self-check** —
  unlike Task 5, which saw `check-i18n` red at its preconditions. Fire them at both moments
  anyway; either reading alone can mislead in either direction.
- **PLUGINS.md's O-Orbit row reads `1.2.2` and the shipped version is `1.3.0`.** The row was
  stale by a patch before this task and is now stale by a minor. The CHANGELOG entry, the
  commit messages and this summary all name **1.2.3** as the `from`, read from the CMakeLists
  value.
- **The submodule needs no attention from you**, but the guard still does: `git commit --
  plugins/O-Orbit` would include it, and any commit you make that touches this plugin's tree
  should run the guard. The pointer is at `b6fe1882` (v1.3.4) and both my commits leave it
  there, proved per commit rather than inferred from path-scoping.
- **`auval -v` takes THREE UNQUOTED words.** O-Orbit's triple is `aufx OuOr` + the suite
  manufacturer code; read it off `auval -a` rather than composing it. `IS_SYNTH FALSE`, so
  it is an `aufx`, not an `aumu`.
- **Six false predictions here, six on Task 5, and three of them are the same three every
  task has found** — the O-Strata untracked count, the positive control that cannot fire at
  preconditions, and the R3 denominator. Those three are properties of the PLAN TEMPLATE, not
  of any plugin, and the count is the finding.

---

## A note on the shared checkout

A concurrent session was working on O-Strata throughout this task and committed
`b80b2d56` (*"phase(O-Strata): Stage 1 verify — add the smoke harness output log"*) before
my first commit. It came through clean:

- both my commits are present and untouched — `0efb343e`, `7ae50ad8`;
- **4 files and 3 files respectively, every one under `plugins/O-Orbit/`**, with no
  stranger's path in either;
- **0 deletions** across both;
- `.claude/agent-memory/research-planning-agent.md` is modified in the working tree
  throughout and is in neither commit;
- the O-Strata untracked count read **0** at my preconditions and **0** at my self-check.

Every commit was path-scoped **to individual files** and the branch and staging were
re-checked immediately before each rather than once at the start.

---

## Self-Check: PASSED

Files claimed, verified on disk:

- `plugins/O-Orbit/Resources/ui/js/i18n.js` — FOUND
- `plugins/O-Orbit/Resources/ui/index.html` — FOUND
- `plugins/O-Orbit/Resources/ui/css/styles.css` — FOUND
- `plugins/O-Orbit/Source/PluginProcessor.h` — FOUND
- `plugins/O-Orbit/CMakeLists.txt` — FOUND
- `plugins/O-Orbit/CHANGELOG.md` — FOUND
- `plugins/O-Orbit/Resources/ui/js/modules/preset-manager.js` — FOUND, **unchanged**
- `plugins/O-Orbit/libs/SAF` — FOUND, **submodule at `b6fe1882` (v1.3.4), clean**

Commits claimed, verified in `git log --all`:

- `0efb343e` — FOUND
- `7ae50ad8` — FOUND

Installed bundles, verified on disk:

- `~/Library/Audio/Plug-Ins/VST3/O-Orbit-dev.vst3` — FOUND, fresh
- `~/Library/Audio/Plug-Ins/Components/O-Orbit-dev.component` — FOUND, fresh,
  `CFBundleShortVersionString = 1.3.0`
- no unsuffixed alternate-variant bundle beside either — CONFIRMED
