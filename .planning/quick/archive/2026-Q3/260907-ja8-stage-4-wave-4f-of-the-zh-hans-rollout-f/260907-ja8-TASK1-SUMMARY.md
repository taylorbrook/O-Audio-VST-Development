---
phase: quick-260907-ja8
task: 1
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4f, tracer, O-GrainScatter, gates, fonts, geometry, enumeration-census]
status: complete
plugin: O-GrainScatter
version: 2.7.1 -> 2.8.0
rows: 131
started_head: 4965c271a0d6b882ce23f1c6d013563782a629ab
commits:
  - b534636b  # zh column at 'mt', gate derived, fonts + line boxes
  - a92b7980  # promotion to 'bt', v2.8.0, CHANGELOG
metrics:
  emitter_rows: 131
  entries: 92          # 39 I18N + 53 LABELS
  gate_assertions: 816 -> 1054
  blind_chunks: 2      # 66 + 65, one salt each
  correction_rounds: 1 # round 2 corrected nothing further
  rows_reauthored: 1
  termNote_exemptions: 2  # one reason, two entry-scoped rows
---

# Task 1 — O-GrainScatter to Simplified Chinese (wave 4f TRACER)

**HEAD at start: `4965c271`** — the plan records `d37cdcc0`, which was stale by one
commit (another session landed `plugins/O-Strata/` as a TRACKED directory while the
plan was being written).

Two path-scoped commits, `git commit -- plugins/O-GrainScatter` both times, branch and
`git status` re-checked immediately before each. No `git add -A`, no tag, nothing under
`modules/`, nothing under `plugins/O-Orbit/libs/SAF`, and `git show --name-only` over
both commits reports **0** paths matching `O-Strata`.

---

## 1. Preconditions — all eight FIRED on the tree as found

| # | check | measured | plan predicted |
|---|---|---|---|
| a | `i18n-zh-lint --self-test` | **`SELF-TEST: 10/10`**, exit 0 | 10/10 ✓ |
| b | `check-i18n --plugin O-GrainScatter` | exit 0, `[1] LANGUAGES … got ["en","fr"]`, `[1] the table carries copy — 39 I18N + 53 LABELS` | ✓ |
| c | `i18n-zh-lint --plugin O-GrainScatter` | exit 0 | ✓ |
| d | `check-ui-labels --plugin O-GrainScatter` | exit 0, `== ALL CHECKS PASSED ==`, `52 of 49`, **no `never became visible` NOTE at all** | ✓ |
| e | `plugins/O-GrainScatter/tests/ui_tip_render_check.js` | exit 0, `== ALL CHECKS PASSED == (816 passed)` | ✓ |
| f | `check-i18n` repo-wide | exit 0, `ALL CHECKS PASS — 43 localized plugin(s)` | ✓ |
| g | `i18n-zh-lint` repo-wide | **exit 2**, `0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read`, `O-Strata ERROR no i18n.js under either UI root` | ✓ — J1's discriminator held exactly |
| h | git | `main`, one worktree, `git tag --points-at HEAD` = 0, **no entry under `plugins/`** | **FALSE — see §7.1** |

J1 was worth the paragraph it got. Both repo-wide lints exit 2 on a clean tree and neither
exit code is a finding; the load-bearing line is the finding count and the unreadable
count, and both matched.

---

## 2. What shipped

**131 emitter rows** — 39 tip titles, 39 tip bodies, 53 labels, over 92 entries
(39 `I18N` + 53 `LABELS`). Corpus 3761 → 3892 rows at emit time; O-GrainScatter now reads
`131 / 131 zh / 0 mt / 131 bt`.

**`reviewed: 'bt'`, `'native'` open.** Authored at `'mt'` (R1 — `BELOW SHIP BAR … 92`
proved nothing was written at `'bt'` early), promoted only after the reverse read, in its
own commit.

Key counts UNCHANGED at 39 + 53. Duplicate-key scan (J7) returns **empty before and after**,
92 keys both sides.

### One termNote exemption, two entry-scoped rows

**`Dist LPF` → 距离低通, NOT the glossary root 失真低通.** On this page `Dist` is
**DISTANCE**; the body reads *"Distance LPF sets how much Distance darkens the cloud"*.
The glossary renders the other sense of the abbreviation, and 失真低通 would name a control
this plugin does not have.

**O-GrainScatter is the ONLY site in the corpus carrying the term** — `grep -rn "Dist LPF"`
across all 44 tables hits this file and nothing else. So the root was settled without a
site to check it against. **Carry-forward: the glossary row `'dist lpf': ['失真低通']` at
`scripts/i18n-zh-glossary.js:433` should be corrected to 距离低通.** The blind reverse read
then confirmed the reading independently — it returned **"Distance Lowpass"**.

A termNote is ENTRY-scoped (N4), so both `label.distLpf` and `tip.distLpf` carry it.

---

## 3. Stale bodies — corrected FIRST, then the file re-read, then authored (C7)

The comment-stripped probe reported **3** hits, matching the plan.

**`tip.settings` (J9) — FALSE, corrected.** Body said the panel holds the interface
language and nothing further. `#tips-toggle` is inside `.settings-popover` (markup L577 /
L602 as found), and `check-i18n` assertion `[16]` PASSES on the requirement that this page
carry exactly one bound, keyed hover-help switch — which is what makes the claim false.
Its explanatory comment above it denied the toggle too and was corrected with it. **Clause
DELETED, not widened**, following O-Detune's shipped precedent.

**`tip.language` (J10) — pair clause deleted in en AND fr.** *"English or Français." /
"English ou Français."* removed. The rest — host automation lane and on-screen values stay
English — is true and stays.

**Both verdicts recorded at their entries in prose (C8); neither comment spells the
superseded literal.** After correction the comment-stripped stale-body probe reads **0**.

---

## 4. The gate — both shapes the inventory grep cannot see

`tests/ui_tip_render_check.js`, 816 lines. `LANGUAGES` was **already an export of the
module this file imports** and was simply never destructured — wave 4e's Q2 shape one step
easier than on O-Wind/O-Reed/O-Bowed, where a loader has to publish it.

### N12 enumeration census — O-GrainScatter, before → after

| form | before | after | disposition |
|---|---|---|---|
| 1 — `LANGUAGES.join(',')` equality | **0** | 0 | **ABSENT on this plugin** (present on O-Wind/O-Reed/O-Bowed per J3) |
| 2 — string `'en,fr'` | 0 | 0 | absent |
| 3 — array-literal walk `['en','fr','en']` | **1** (raw L465) | **0** | derived + return pass preserved |
| 4 — computed/UA-face form controls | **0** | 0 | no instance of the class (see §5) |
| 5 — `Map` read with a literal key | 0 | 0 | absent |
| 6 — per-language object property access | **4** | **4** | **RESTORE-scoped, deliberately LEFT** |
| 7 — paired literal-argument call | **2** (raw L561-562) | **0** | generalized |
| g5l inventory grep sees this file? | **no** | no | the grep's false-negative class, confirmed |

Post-repair census gates: enumeration literals **0**, `LANGUAGES` mentions **12**,
restore-scoped `.en.[tb]` reads **4**, `__seenEn` **2**.

### The three repairs

1. **Destructure + derive-or-abort.** `const { I18N, TIP_BINDINGS, LANGUAGES } = table;`
   then a shape assertion: non-empty array opening with the English code, else
   `check(false, …'REFUSING to sweep a guessed list'…)` and `process.exit(1)`. **No fallback
   literal** — a fallback is how a broken derivation survives unnoticed.

2. **The walk keeps its return pass.** `const SWEEP = [...LANGUAGES, LANGUAGES[0]];`
   The third element of the old literal was a deliberate re-visit labelled by `isReturn` /
   `page.__seenEn`; a naive substitution would have deleted a gate stage and the
   `(return pass)` log line would simply have stopped printing. Observed live:
   `sweeping 4 pass(es): en -> fr -> zh-Hans -> en (3 language(s) plus the en return pass)`.

3. **The paired call, and A DIRECTION DECISION.** See §4.1.

### 4.1 The [5] assertion's direction — decided on a measurement, not a preference

The old assertion demanded the second language be **TALLER** (`grew.length > 0`). Measured
on the first zh run:

```
tallest en 139.3 px
tallest fr 139.3 px (25 taller,  0 shorter, 11 unchanged)
tallest zh 123.9 px ( 0 taller, 32 shorter,  4 unchanged)
```

**The old assertion would have FAILED on a correct page.** Chinese says the same thing in
fewer glyphs.

**Chosen:** assert the boxes **DIFFER**, per language, and *report* the direction.
**Rejected:** a per-language direction table (grow for these codes, shrink for those) —
that is the same enumeration this repair exists to delete: it needs an edit every time a
language lands, in a place nobody looks, and the day it is forgotten it asserts the wrong
direction rather than nothing. The difference criterion is also the one the file's own
failure note always named (*"the fr pass measured the same boxes as en, so its clamp half
is decoration"*) — the growth wording was a French-specific accident.

### 4.2 Restore-scoped sites — LEFT, classified at each site

Four `I18N['tip.grainSize'|'tip.probability'].en.[tb]` reads (raw L579-580, L630, L679) and
three `__setLanguage('en')` resets (raw L493, L544, L593). Each sits immediately after a
reset that writes English **by construction**, so the English row is the only correct
comparand — reading the current language there asserts nothing whatever the list holds.
Prose comments at each site say so. One label string was reworded from *"after the French
pass"* to *"after every other language pass"*, which is wording, not scope.

### 4.3 Derive-or-abort control — fired TWICE, against the walk AND the paired call

| plant | expected | observed |
|---|---|---|
| `LANGUAGES = []` | `[0]` refuses | `FAIL: [0] LANGUAGES derives from the table as a non-empty array opening with the English code — got []. REFUSING to sweep a guessed list…` **exit 1** |
| `LANGUAGES = ['en']` | clears `[0]`, `[5]` refuses | `sweeping 2 pass(es): en -> en` then `FAIL: [5] the derived list carries at least one language besides en — got ["en"]…` **exit 1** |

The second plant is the one that matters: it proves the guard on the **paired call** is
independent of the guard on the list. Reverted by **targeted edit, never
`git checkout --`** (this plugin's own gate header records losing an uncommitted edit to
exactly that command), `sha256` **byte-identical to pre-plant**
(`fc3e24d4…3522f13` both sides).

**816 assertions before, 1054 after.**

---

## 5. Fonts — the census, and a THIRD "the tool is not the oracle" trap

Eight declarations. **ZERO `font-family: inherit` against ZERO form-4 nodes** — a page with
no instance of the class (wave 4c's C4 converse), *not* a page nobody measured. Recorded as
measured; the four consumers in Tasks 2–5 invert this.

CJK tail added before the trailing generic at **all 8 sites** (`grep -c 'PingFang SC'` = 8).

### The face-naming half — measured through CDP, not inferred

`.toggle` declared `'Garamond', serif`. Garamond is absent, so its only survivor was a bare
`serif`, which Chromium resolves against the **document's** lang. A grep for a bare generic
finds nothing here.

**`CSS.getPlatformFontsForNode` says the two `.toggle` nodes resolve to `Times`, while
every other caption on the page resolves to `Georgia`.** Naming Georgia would have been
tidier and would have moved both English captions. `Times` is named instead — the face
already rendering — then the tail, then the generic. The fr arm's `[7]` stayed green,
confirming it.

### FALSE PREDICTION IN THE SHARED METHOD — J12's oracle has a false-negative class

`system_profiler SPFontsDataType | grep -c "Family: Times$"` reports **0**, yet
`/System/Library/Fonts/Times.ttc` is on disk and Chromium resolves to it. **The
installed-family table the rollout has used since wave 4b cannot see `Times`.** Everything
else re-derived unchanged (Georgia 4, Times New Roman 4, Arial 4, PingFang SC 6,
Songti SC 4, Garamond 0, Microsoft YaHei 0).

**For Tasks 2–5: `system_profiler` proves presence, never absence.** Where a face matters,
probe the resolved face through CDP.

### measure-ui, before → after

| screen | before (vacuum, J13) | after |
|---|---|---|
| `undeclared-font` | 0 (no Han to find) | **0 against 86 Han nodes** — non-vacuous |
| `line-height-normal` | 0 (vacuum) → 21 once Han landed | **7**, all evidenced non-movers |
| `wrap-count` | 0 (vacuum) | **0** |
| `svg-font-attr` | 0 carriers | **0 carriers** — matches the plan |
| identity | 508 nodes / 254 keys / 51 ids | **765 / 255 / 51** (three languages) |
| naked generic / absent-face stack | — / **2** | **0 / 0** |

Both states applied on every run; `did not resolve|unresolved|skipped state` = **0**.

---

## 6. Geometry — one root cause, 140 symptoms

First zh run: **`[7][zh-Hans]` FAIL, 140 moved**, plus
`[8b][zh-Hans] label.freeze x #grain-canvas`. The fr arm was green throughout.

**Root cause: fourteen `line-height: normal` leaves each grew 1–3 px under Han, and
`.viz-area` (`flex: 1 1 auto`) absorbed the sum — shrinking 12 px and dragging everything
below it up the page.** The 140 were one defect, not 140.

Pinned as **RATIOS derived from this page's own measured English CONTENT box**, padding
and border subtracted FIRST (N10/N11), per size:

| selector | fs | en box | minus pad/border | ratio | reproduces en |
|---|---|---|---|---|---|
| `.header .tagline` | 10px | 11 | 11 | **1.1** | 11.0 ✓ |
| `.viz-panel .viz-label` | 9px | 10 | 10 | **1.1111111** | 10.0 ✓ |
| `.group-label` | 9px | 10 | 10 | **1.1111111** | 10.0 ✓ |
| `.settings-label` | 9px | 10 | 10 | **1.1111111** | 10.0 ✓ |
| `.pitch-hint`, `.spatial-hint` | 8px | 11 | 9 (pt 2) | **1.125** | 11.0 ✓ |
| `.toggle` | 10px | 29 | 13 (pad 6+6, border 2+2) | **1.3** | 29.0 ✓ |

Four of the six match M8's measured table exactly. **`.toggle`'s 1.3 does not** — a
`<button>`'s content box is 13 px for a 10 px font, not the 11 px M8 gives for a bare leaf.
Derived from this page's own box rather than taken from the table (see §7.4).

**R8 fired before appending:** none of the six selectors carried an existing `line-height`.
The five that exist (`.knob-name` 1.1, and four others) are on different selectors.

**Result: 0 moved on en, fr AND zh-Hans, across both states.** `[8b]` cleared with the
same fix — it was a symptom of the 12 px shrink, not a separate defect.
`line-height-normal` 21 → **7 residuals, every one named:**

| node | fs | enH | zhH | verdict |
|---|---|---|---|---|
| `span.dropdown-name` ×7 (形状 音阶 根音 音高模式 同步模式 模式 轨迹) | 8px | **18** | **18** | EQUAL — evidenced non-movers inside a fixed-height container |

**J14's fixed-width question, answered by measurement:** `#stutter-gate-btn { width: 110px }`
measured **110.00 × 29.00 on BOTH arms** — 断续门 fits, no clip.

**State-EFFECT assertion (wave 4e N1), fired on the one `click` state:**
`elementFromPoint` at `#gear-btn`'s centre returns `button#gear-btn.gear-btn` — the target
itself, so neither a coverage hole nor a `null`/scrolled target — and the popover measurably
opened, `hidden:true, h:0` → `hidden:false, h:55`, with the selector reporting
`en=English fr=Français zh-Hans=简体中文` and `selected: zh-Hans`. **State 2 is an `eval`
and needs no pointer probe; recorded as such.**

---

## 7. FALSE PREDICTIONS — measured value beside the predicted one

### 7.1 Precondition (h) — `plugins/O-Strata/` is TRACKED, not untracked
- **Predicted:** *"`git status --short` → the only entry is `?? plugins/O-Strata/`"*
- **Measured:** committed as `4965c271` by a concurrent session before this task started.
  `git status` shows **no entry under `plugins/`** at all.
- The plan's downstream verify `git status --short | grep -c "O-Strata"  # 1` therefore
  reads **0**, and the orchestrator's correction (no entries under any wave plugin) is the
  criterion that actually discriminates. **J1's lint consequence is UNAFFECTED** — O-Strata
  still has no `i18n.js`, both repo-wide lints still exit 2 for that reason.

### 7.2 The product-name control returns 0, not non-zero
- **Predicted:** *"Expect the 'no product name in the batch' control to report a **non-zero**
  count on any plugin whose own copy names itself."*
- **Measured:** **0 hits** on `GrainScatter|Ouaricon|O-Grain` across all 131 rows. This
  plugin's copy never names itself — the product name is an `I18N_EXEMPT` **markup** string,
  not a table row. Reported as a measured zero, not a skipped check.

### 7.3 `system_profiler` cannot see `Times` (see §5)
- **Predicted:** J12's table is the availability oracle.
- **Measured:** `Family: Times` = 0 while `/System/Library/Fonts/Times.ttc` exists and
  Chromium resolves to it.

### 7.4 M8's line-box table does not cover a `<button>`
- **Predicted:** M8's measured English content line boxes — `10px→1.1`.
- **Measured:** true for a bare leaf; a `<button>` at 10 px has a **13 px** content box,
  ratio **1.3**. The UA `font` shorthand on form controls is the same mechanism J14's ratio
  block already names for `.viz-btn`/`.tuning-file-btn`/`.generator-btn`. **M8's table is
  leaf-scoped; derive form-control ratios from the element's own box.**

### 7.5 The refusal-control ordering confounds control 2
- **Predicted:** fire both refusal controls (missing `--forward-provenance`, wrong-side
  `--manifest`).
- **Measured:** ingest checks provenance **before** the id join, so a wrong-side manifest
  that *also* lacks forward provenance refuses for control 1's reason and control 2 proves
  nothing. **Reported rather than papered over.** Re-run with a properly provenanced second
  emit, which produced the right refusal:
  `WRONG MANIFEST FOR THIS BATCH: all 131 returned ids are unjoinable, not one.`

### 7.6 The version — the plan's correction was RIGHT, the description's was not
- Description said `2.6.1 → 2.7.0`. **CMakeLists reads an unquoted `VERSION 2.7.1` at L11;
  both readers agree.** Shipped **2.8.0**. `PLUGINS.md` still reads **2.6.1** — two minors
  stale, for Task 6.

### 7.7 Everything the plan predicted that HELD
816-assertion baseline, `52 of 49` with 0 never-visible, 0 form-4 / 0 `inherit`,
`svg-font-attr` 0, 2-node absent-face population, 131 emitter rows, `#tips-toggle` inside
`.settings-popover`, three stale-body probe hits, both new enumeration forms at the
predicted raw lines, the `PluginProcessor.h` L112-113 codec, target `OuariconGrainScatter`
vs folder `O-GrainScatter`, and the J1 discriminator verbatim.

---

## 8. The blind reverse read

Emitted **`--emit O-GrainScatter --plugin O-GrainScatter`** (both — W4; `--emit` alone
hands 3892 already-shipped rows to an external reader as this plugin's work). **Two disjoint
chunks of 66 and 65, each from its OWN emit so each carries its OWN salt** (`426da650…` /
`bce93c1c…`). Proved disjoint before dispatch: **0 overlapping real ids, union exactly 131**,
and two emits of the same target share **0 of 131** blinded ids.

Dispatched `claude -p --model claude-sonnet-4-5 --allowed-tools ""` from
`/tmp/ja8t1/blindrun` — outside the repo — with file, web and repository access forbidden
in the prompt.

### Controls, every one fired

| control | result |
|---|---|
| ids identical **AND IN ORDER** (M12) | `diff` clean on both chunks; 66/66 and 65/65 |
| ids pure 12-hex, unique, no key fragment | 131/131, 131 unique, 0 fragments |
| rows well-formed (exactly 2 tab fields) | 131/131 |
| Han surviving in returned English | **0**, with the positive control fired on the same probe (**sent** column reports 66 and 65 Han lines) |
| sha256 of the zh column vs the committed tree | **MATCH** — `9a51b6a5…850a32` both sides |
| refusal 1 — missing `--forward-provenance` | **FIRED** |
| refusal 2 — wrong-side `--manifest` | **FIRED** (after §7.5's correction) |
| product-name control | **0** — adjudicated, see §7.2 |

### All 131 triples read with `--verbose` (R2)

71 round-tripped exactly. Every one of the remaining 60 was read and adjudicated on
**collision on the page**, not drift distance.

**ONE ROW RE-AUTHORED.** `tip.smoothing`'s body rendered *ambisonic* as 高保真立体声
("high-fidelity **stereo**") and the blind reader returned exactly that. **This page already
describes a 立体声路径 (stereo path) in `tip.panRnd`**, so a reader could take spatial
smoothing for a stereo control — a collision on the page, not a preference. There is **no
corpus precedent** for the term (O-Octagon, the suite's other spatial plugin, never uses the
English word). The Latin term is kept instead, on the same footing as `tanh` in
`tip.feedback`. **Correction round 2 — fresh reader, DIFFERENT MODEL (Haiku 4.5), fresh
salt sharing no id with round 1 — returned *"allowing Ambisonic coefficients to glide rather
than jump"*, recovering the source. Round 2 corrected nothing further, so no round 3.**

### Accepted with reasons — the classes worth naming

**The stutterGate control is the strongest evidence this wave has produced about the METHOD.**
`label.stutterGate` came back **"Gate"** (0.67) while `tip.stutterGate` — the **same** Chinese
string 断续门, in the **other** chunk, read by the **other** reader — came back **"Stutter
gate"** (1.00). One string, two independent readers, two answers. That is reader variance,
not an authoring defect, and **splitting the table into two independently-dispatched chunks
is what made it visible.** No other control on this page could be mistaken for it. Accepted.

**The abbreviation class.** `Amp Rnd`, `Size Rnd`, `Pitch Rnd`, `Pan Rnd`, `Traj Speed`,
`Az Spread`, `El Spread` all returned **unabbreviated** (0.00–0.39). Deliberate: French had
to abbreviate against this page's 62 px `.knob-container` cap and Chinese does not, so the
Chinese caption carries the full term. Each maps to exactly one control; no collision.

**`Dist LPF` → "Distance Lowpass"** — the independent confirmation of §2's termNote.

**Bare-noun shortfalls** — 平滑 → "Smooth", 脉冲 → "Pulse". Single-control terms on this
page, no collision. Accepted.

### R3 glossary screen — clean

83 of 92 captions are glossary-covered. **One root shared by two different English strings:**
悬停帮助 for `label.hoverHelp` ("Hover help") and `tip.tipsToggle` ("Hover Help") — the
caption and the tooltip title of the **same** `.settings-row` control, which R3 excludes
explicitly. Mechanical downstream check after authoring: **0 distinct-control collisions**;
all 39 shared-rendering groups are same-control or same-English.

---

## 9. Gate results — every actual output line

**Preconditions (tree as found)** — §1.

**After the work:**

```
check-i18n --plugin O-GrainScatter        exit 0
  PASS: [1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr","zh-Hans"]
  PASS: [1] the table carries copy — 39 I18N + 53 LABELS
  ALL CHECKS PASS — 1 localized plugin(s)

i18n-zh-lint --plugin O-GrainScatter      exit 0
  O-GrainScatter   131  92   ·  ·  ·  ·  ·  ·  ·  ·  ·  ·      0
  BELOW SHIP BAR — entries at reviewed:'mt': 0
  termNote exemptions (info): 2
  GATE PASSED — exit 0. 0 findings across 1 plugin(s).

check-ui-labels --plugin O-GrainScatter   exit 0
  PASS: [7][GEOMETRY DIFF][fr]      no non-label element moved …
  PASS: [7][GEOMETRY DIFF][zh-Hans] no non-label element moved …
  PASS: [7][GEOMETRY DIFF][zh-Hans] the visible element SET is identical …
  PASS: [8b][zh-Hans] no label intersects a NON-label element it cleared in English
  PASS: [2][vacuity][zh-Hans] the zh-Hans pass actually rendered — 49/49 labels (100%)
  PASS: [2][vacuity][zh-Hans] keyed ATTRIBUTES actually changed language — 4/4
  coverage: 52 of 49 [data-i18n] elements were VISIBLE in at least one state
  == ALL CHECKS PASSED ==

ui_tip_render_check.js                    exit 0
  sweeping 4 pass(es): en -> fr -> zh-Hans -> en (3 language(s) plus the en return pass)
  == ALL CHECKS PASSED ==   (1054 passed)      [was 816]

boot-all-uis --plugin O-GrainScatter --strict-tips   exit 0
  clean: 1/1   DEAD bindings: 0   late bindings: 0

measure-ui --report all
  undeclared-font: 0    line-height-normal: 7    wrap-count: 0
  svg-font-attr: 0 attribute carrier(s), 0 finding(s)
  identity: 765 nodes / 255 distinct DOM keys / 51 distinct display ids
measure-ui --verbose:   did not resolve|unresolved|skipped state = 0

i18n-zh-backtranslate --plugin O-GrainScatter
  O-GrainScatter   131  131   0 mt   131 bt   0 native   0 none

C++:  comment-stripped 'zh-Hans' count = 2
      languageCode: THREE-WAY   languageIndex: THREE-WAY   (matched by NAME, not by a sweep)
      Han under Source/**/*.{h,cpp}: NONE
      positive control on the same probe: FIRED (js/i18n.js)

Repo-wide, unchanged from baseline:
  check-i18n     exit 0   ALL CHECKS PASS — 43 localized plugin(s)
  i18n-zh-lint   exit 2   0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read
  i18n-fr-lint   exit 2   plugins with findings: 0 / 44   (1 could not be read)

Two-arm CMake reader:  O-GrainScatter 2.8.0
  one-arm == two-arm on ALL FIVE wave plugins — O-Wind 1.20.0, O-Contrabass 1.9.0,
  O-Reed 1.5.0, O-Bowed 1.8.0. NO set()-variable form anywhere in this wave.

Non-consumption of scala-tuning-engine, re-confirmed AFTER the work: 0 refs, 0 private copies
modules/ touched: 0        tags at HEAD: 0        O-Strata in either commit: 0

Build:  ./scripts/build-and-install.sh O-GrainScatter   exit 0, 49s
        VST3 + AU installed, -dev only, no alternate-variant orphan
        Installed CFBundleShortVersionString: 2.8.0
auval:  NOT RUN — deferred to Task 6 by design
```

---

## 10. Carry-forwards for Tasks 2–5 — things the plan does not say

1. **`system_profiler` proves presence, never absence** (§5, §7.3). Probe the resolved face
   through CDP `CSS.getPlatformFontsForNode` before naming one. A ~40-line probe is in this
   summary's method; the pattern is: `S.buildRoot` → `S.serve(built.root, …)` →
   `page.context().newCDPSession(page)` → `DOM.enable`/`CSS.enable` →
   `DOM.getDocument` → `DOM.querySelector` → `CSS.getPlatformFontsForNode`.
   **`buildRoot` returns an OBJECT — pass `built.root` to `serve`, not the return value**
   (`ERR_INVALID_ARG_TYPE` otherwise, the sibling of the `resolvePlaywright` trap).

2. **M8's line-box table is LEAF-scoped.** A `<button>`'s content box is not
   `fontSize × M8[fontSize]` — measure the element's own box. This bites directly on
   Tasks 2–5, whose `1.11` ratio block names `.viz-btn`, `.tuning-file-btn` and
   `.generator-btn` **separately for exactly this reason** (J14 correction 1). Verify the
   transferred `1.11` against each plugin's own measured button box rather than assuming it
   transfers.

3. **A `flex: 1 1 auto` container turns N small caption growths into one big move.** 14
   leaves growing 1–3 px produced **140** `[7]` entries. Read the `dh` column and find the
   ONE absorbing container before pinning anything; do not treat the count as the work.

4. **Split the batch into independently-dispatched chunks even when one chunk would fit.**
   The `label.stutterGate` / `tip.stutterGate` divergence (§8) is only visible because the
   same Chinese string went to two different readers. Tasks 2–5 all exceed the threshold
   anyway, but chunk on the split, not on size alone.

5. **Ingest checks provenance BEFORE the id join** (§7.5). A wrong-side manifest control
   needs a manifest that HAS forward provenance, or it fires for the wrong reason and reads
   like a pass.

6. **The `--emit` + `--plugin` pair is load-bearing and silently so.** With both, 131 rows.
   Chunking must be done on the emitted file with the manifest as the key — the emitter has
   no row-range flag.

7. **`git commit -- <paths>` requires options BEFORE the `--`.** `git commit -- <paths> -F -`
   fails with `pathspec '-F' did not match any file(s)`. Write the message to a file and use
   `git commit -F <file> -- <paths>`.

8. **The zh column injection is mechanical and safe if the comma is handled.** A
   brace-matching insert before each entry's closing `}` must add a `,` after the preceding
   `fr` block when it does not already end in one; without that the file fails to parse with
   a bare `Unexpected string`. The scan cost one round trip here.

9. **`grep -rn "Dist LPF"` proves O-GrainScatter is the term's only site.** Before writing a
   termNote against a glossary root, check the corpus site count — a root derived from ONE
   plugin's caption+title pair looks "measured" (2 occurrences) but had no second site to
   check it. **This may not be the only such root.**

10. **`check-i18n` repo-wide still reads 43, not 44, after this task.** O-GrainScatter was
    already counted (it had en/fr). The count tracks localized plugins, not zh plugins — do
    not read a change into it.

11. **The corpus stage view is the honest row count**: 3761 → **3892** after this task.
    Tasks 2–5 add 214 / 186 / 168 / 161 to reach 4621.

12. **This plugin's `.knob-name` already carries `line-height: 1.1`** and its
    `span.dropdown-name` sits in a fixed-height container — which is why 7 Han leaves read
    `enH == zhH == 18` and needed nothing. The four consumers have no such head start; expect
    their residual counts to be larger and their movers to include the shared panel's
    `.interval-list-header` and `.tonic-label`.

13. **Not verified here, and Tasks 2–5 own it:** the shared-module 37-key column, the
    `1.11` ratio block, the five-selector CJK tail, O-Reed's and O-Bowed's 116-node
    system-stack panel, O-Contrabass's SVG attribute carrier, and enumeration form 1.
    O-GrainScatter is the wave's only non-consumer, so **none of the shared-module method is
    exercised by this tracer** — that debt starts fresh in Task 2.

---

## 11. Deferred / out of scope

- **auval — NOT RUN**, deferred to Task 6's single cold sweep by design. O-GrainScatter is
  the wave's only **effect** (`IS_SYNTH` absent from its `CMakeLists.txt`), so its triple is
  `aufx OuGS OuDv`-shaped rather than `aumu` — the odd one out in Task 6's sweep. Remember
  `auval -v` takes **THREE UNQUOTED WORDS**.
- **`PLUGINS.md` NOT touched** — Task 6 commits it once, and must correct O-GrainScatter's
  row by **two minors** (2.6.1 → 2.8.0).
- **Glossary correction NOT made** — `'dist lpf': ['失真低通']` at
  `scripts/i18n-zh-glossary.js:433` should read `['距离低通']`. A shared-script edit is
  outside a per-plugin task's path scope and would touch a file five other tasks read.
  Carried as a corpus-level item.
- **`reviewed: 'native'` OPEN** on all 131 rows — no native Chinese reader exists on this
  project. Disclosed in the CHANGELOG and printed by lint rule R1 on every run.
