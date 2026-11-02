---
task: 260906-uu7
type: quick
subsystem: O-Chorus / WebView UI typography
status: complete
tags: [i18n, zh-Hans, font-family, css-ordering, geometry-pins, O-Chorus, auval]
requires:
  - quick-260904-qrc (wave 4a — first proof of the generic-terminates-search mechanism)
  - quick-260904-g5l (O-Chorus 1.6.2 — the tree this task started from)
provides:
  - "O-Chorus 1.6.3: the six tailed font stacks name PingFang SC and reach it"
  - "A BEFORE/AFTER CSS.getPlatformFontsForNode transcript pair for this page"
  - "A reusable CDP resolved-face probe: cdp-font-probe.js (task dir, uncommitted)"
affects:
  - plugins/O-Chorus/Source/ui/public/index.html
  - plugins/O-Chorus/CMakeLists.txt
  - plugins/O-Chorus/CHANGELOG.md
  - PLUGINS.md
tech-stack:
  added: []
  patterns:
    - "A font-family generic TERMINATES the search and is resolved against the document's lang — a CJK tail written after it is dead code."
    - "measure-ui.js's `ff` field is the DECLARED stack string, never the resolved face. Only CDP CSS.getPlatformFontsForNode answers which face rendered."
    - "A pin's derivation (intrinsic width at `width: auto`, or the `line-height: normal` counterfactual) is invisible in a measured row, which just echoes the pin back."
key-files:
  created: []
  modified:
    - plugins/O-Chorus/Source/ui/public/index.html
    - plugins/O-Chorus/CMakeLists.txt
    - plugins/O-Chorus/CHANGELOG.md
    - PLUGINS.md
decisions:
  - "Kept both inert `line-height: 1.2` declarations rather than deleting them — removing an inert declaration is a behavioural change on a host whose UA sheet does not override it, and this task's charter was ordering plus honest comments, not a cleanup sweep."
  - "Kept `.settings-select`'s `width: 65px` even though the reorder made it a no-op on this host — it is host insurance for a machine that resolves a third face."
  - "Left the three tail-less stacks untouched per the plan's explicit scope, and recorded them in shipped source (comment + CHANGELOG) rather than only in .planning."
metrics:
  duration: "3 executor sessions (Task 1 tracer, Task 2, Task 3); Task 3 wall clock ~12 min, build 50 s, auval ~90 s"
  completed: 2026-09-06
actuals:
  tokens: 7400
  tasks: 3
  commits: 2
---

# Quick 260906-uu7: O-Chorus's CJK font tail was inert — Summary

O-Chorus's Chinese rendered in **Songti SC**, a serif, while all six of its tailed
`font-family` stacks named **PingFang SC** — because a generic terminates the font
search and Chromium resolves a generic against the document's lang, so under
`lang="zh-Hans"` the tail written after `serif` was never consulted. The tail is now
ahead of the single trailing generic, every geometry pin was re-measured under the
face that actually renders, and O-Chorus ships at 1.6.3, installed and auval-green.

**Note on `actuals.tokens`:** 7400 is chars/4 over the realized committed diff
(29 606 chars across both commits). The plan's `estimate.tokens: 95000` was a
work-effort estimate covering three executor sessions and ~20 measurement artifacts,
not a diff size — the two numbers are on different scales and are reported as
measured rather than reconciled.

---

## Commits

| Task | Commit | Subject | Files |
|---|---|---|---|
| 1 (tracer) | — | measurement only, no commit | `plugins/O-Chorus` byte-unchanged, md5 `2c1ed43836f668f87ec005f980cac744` |
| 2 | **`859ab521`** | `fix(O-Chorus): CJK tail before the trailing generic — Han renders PingFang SC` | `plugins/O-Chorus/Source/ui/public/index.html` only — 183 insertions / 51 deletions |
| 3 | **`ea1b86cc`** | `fix(O-Chorus): CJK tail before the trailing generic — Han renders PingFang SC at 1.6.3` | `plugins/O-Chorus/CMakeLists.txt`, `plugins/O-Chorus/CHANGELOG.md`, `PLUGINS.md` |

Both on `main`, both path-scoped. **No tag created** (`git tag --points-at HEAD` = 0).
Neither commit carries a `.planning/` path or anything under `plugins/O-Orbit`.

---

## 1. The resolved-face evidence — the only thing that proves the tail became live

`CSS.getPlatformFontsForNode` over CDP. The same probe script
(`cdp-font-probe.js`) was run **unchanged** on both sides — same targets, same
three arms, same gestures. A probe edited between the two runs is not a comparison.

| selector | zh-Hans BEFORE | zh-Hans AFTER | en / fr (both sides) |
|---|---|---|---|
| `.knob-label` | Songti SC (2) | **蘋方-簡 / PingFangSC-Regular (2)** | Times New Roman |
| `.preset-action` | Songti SC (2) | **蘋方-簡 (2)** | Times New Roman |
| `.settings-label` | Songti SC (2) | **蘋方-簡 (2)** | Times New Roman |
| `.settings-select` | Songti SC (4) | **蘋方-簡 (4)** | Times New Roman |
| `.settings-toggle` | Songti SC (1) | **蘋方-簡 (1)** | Times New Roman |
| `.tooltip` | TNR (15) + Songti SC (63) | **蘋方-簡 (63)** + TNR (15) | Times New Roman |
| `.tip-title` | Songti SC (2) | **蘋方-簡 (2)** | Times New Roman |

`(n)` = `glyphCount`. `蘋方-簡` is PingFang SC's localized `familyName`;
`PingFangSC-Regular` is the postscript identifier. `grep -ci songti after-fonts.txt`
= **0**. The en and fr arms are byte-identical before and after.

**Garamond never resolved on either side** — it is not a macOS face, so Latin has
been landing on Times New Roman all along. The v1.5.0 comment's "Latin still
resolves to Garamond FIRST" was false when it was written.

Transcripts: `before-fonts.txt`, `after-fonts.txt`. Mechanism confirmed in source at
`index.html:21` (`<html lang="en">`) and `index.html:1226`
(`document.documentElement.lang = uiLanguage;`).

---

## 2. The pin table — every value held; the measurements moved

**Headline: not one pin VALUE had to change.** All seven pinned selectors held. What
changed is that each comment now quotes a number measured under the face that
renders, and three of them had false *causes* that are now gone.

| pin | arm | BEFORE (Songti SC) | AFTER (PingFang SC) | verdict |
|---|---|---|---|---|
| `.knob-label` `line-height: 1.1111` | en / fr / zh | pinned box 9.984 px; `normal` 10.00 / 10.00 / **13.00** | identical | **HELD** |
| `.knob-label` rendered box | zh | 19.61 × 9.98 (2 chars) · 29.41 × 9.98 (3 chars) | **20.00** × 9.98 · **30.00** × 9.98 | width MOVED +0.39 / +0.59 |
| `.preset-action` `line-height: 1.1111` | en / fr / zh | 9.984 px; `normal` 10 / 10 / 13; box 62.00 × 13.98 | identical | **HELD** (62 px width pin leaves the wider advance nowhere to go) |
| `.settings-label` `line-height: 1.1111` | en / fr / zh | 9.984 px; `normal` 10 / 10 / 13 | identical | **HELD** |
| `.settings-label` rendered box | zh | LANGUAGE 19.20 × 9.98 · HOVER-HELP 38.41 × 9.98 | **19.61** · **39.20** | width MOVED +0.41 / +0.79 |
| **`.settings-select` `width: 65px`** | en / fr / zh | intrinsic (`width: auto`) **65.000 / 65.000 / 64.000** | **65.000 / 65.000 / 65.000** | **HELD — and the mover it existed for is GONE** |
| `.settings-select` endonym widths | en / fr / zh | en+fr 27.516 / 30.500 / **36.797**; zh 27.516 / 30.500 / **36.000** | **27.516 / 30.500 / 36.797 in all three arms** | now genuinely language-invariant |
| `.settings-select` `line-height: 1.2` | all | computed **`normal`** (UA sheet overrides on `<select>`) | computed **`normal`** | **INERT on both sides** — kept, now documented |
| `.settings-toggle` `min-width: 42px` | en / fr / zh | 42.00 × 16.00 all arms | 42.00 × 16.00 all arms | **HELD**; 开 fits with room |
| `.settings-toggle` `line-height: 1.2` | all | pinned 14.000 == `normal` 14.000 | identical | **NO-OP on both sides** — `height: 16px` does the work |
| `.tooltip` `line-height: 1.3` | en / fr / zh | `.tip-title` 366 × **11.69** all arms; body line box 13.00 px | identical | **HELD** in all three arms |
| `.tooltip` `max-width: 384px` | zh | Drive tip 384 × 51.69, 2 body lines | 384 × 51.69, 2 lines | HELD — **but wrap moved elsewhere**, see below |
| `.settings-popover` `width: 170px` | en / fr / zh | 170.00 × 54.00 all arms | 170.00 × 54.00 all arms | **HELD**, now checked against a second Han face |

**Wrap points that moved (the tooltip's 384 px cap):** the `voices` zh tip went
**384.0 × 51.7 px, 2 body lines → 384.0 × 64.7 px, 3 lines**; `#tips-toggle`'s zh tip
went **328.0 → 334.8 px** wide. Tallest zh tip is now 64.7 px against a 109 px well —
44.3 px of headroom intact, still under the six-line ceiling. No fixture re-recording
was needed: `ui_tip_render_check.js` asserts containment, not an exact rect, and
`plugins/O-Chorus/tests/` is byte-unchanged.

### Row-level diff — the complete list of what moved

339 rows / 113 DOM keys / 55 display ids, identity identical on both sides.
Differing cells: **315** — of which `ff: 291` is the DECLARED stack string, which
merely echoes the CSS edit and is evidence of nothing. The real movers:

```
en       #lfo-dot            x 362.62->361.99  y 45.57->45.09    (animated, excluded by [7])
fr       #lfo-dot            x 361.43->361.66  y 44.68->44.85    (animated)
zh-Hans  #lfo-dot            x 361.61->361.64  y 44.81->44.83    (animated)
zh-Hans  span.knob-label     x 56.69->56.50    w 19.61->20.00    ×8 (one at 29.41->30.00)
zh-Hans  span.settings-label                   w 19.20->19.61
zh-Hans  span.settings-label                   w 38.41->39.20
```

**Every non-animated mover is a zh-Hans Han-carrying label widening ~2% under
PingFang's advance. No HEIGHT changed anywhere** — the two faces give the same
13.00 px `line-height: normal` box at 9 px — which is why nothing downstream was
displaced and why `check-ui-labels` assertion 7 stays at 0.
`Han-carrying visible rows: 26 | geometry moved on: 10`. `after-rows.json` is
byte-**different** from `before-rows.json`, satisfying the plan's own anti-guessing
assertion.

---

## 3. The `v1.5.0 CJK TAIL` comment block — four false claims, all corrected

Retitled `CJK TAIL — ORDER (v1.6.3, quick-260906-uu7; the tail itself is v1.5.0)`.

1. **The ordering rule is now stated and attributed** — a generic terminates the
   search, Chromium resolves it against the document's lang, and under `zh-Hans`
   it is already a Chinese face. Attributed to `CSS.getPlatformFontsForNode`,
   wave 4a (`quick-260904-qrc`), with this page's own BEFORE/AFTER pair in
   `quick-260906-uu7`. Also says why a *second* generic is dead by construction and
   why the survivor is `serif`.
2. **Census corrected** from "FIVE of this page's EIGHT" to **SIX of NINE**, with
   `.settings-toggle` added — it arrived with the v1.6.0 settings row and the census
   was never updated.
3. **`.gear-btn` spelling corrected** — v1.5.0 wrote `#gear-btn`; the declaration is
   on the class.
4. **The Garamond claim replaced** with what was measured.

Plus, as recorded-not-fixed: the three tail-less rules are untouched but **not
clean** (see §6).

---

## 4. Gates, build, install, auval

| gate / step | verdict |
|---|---|
| `node scripts/check-ui-labels.js --plugin O-Chorus` | **exit 0** — `== ALL CHECKS PASSED ==`, **87 PASS / 0 FAIL**. `[7][GEOMETRY DIFF]` **0 moved elements on fr AND on zh-Hans** (1 animated `#lfo-dot` excluded and disclosed), visible element SET identical. Coverage 14/14 `[data-i18n]`. Not 77, not "nothing to measure". |
| `node plugins/O-Chorus/tests/ui_tip_render_check.js` | **exit 0** — `== ALL CHECKS PASSED == (345 passed)`, 0 FAIL, real zh-Hans pass. |
| `node scripts/measure-ui.js --plugin O-Chorus --mode box --report all` | **exit 0** — four screens, **none SKIPPED**, counts identical to BEFORE: `undeclared-font: 3` · `line-height-normal: 2` · `wrap-count: 0` (27 leaves, 13 estimated) · `svg-font-attr: 0 carriers / 0 findings`. |
| `node scripts/measure-ui.js --plugin O-Chorus --mode box` (rows) | **exit 0** — 339 rows; `languages en, fr, zh-Hans  states default + 1` — the gear IS clicked, the popover IS measured open. |
| `./scripts/build-and-install.sh O-Chorus` | **exit 0**, 50 s, 7 phases all ✓. Phase 4 removed `O-Chorus-dev.vst3` and `O-Chorus-dev.component`; **no `⚠ Sweeping ALTERNATE-variant` warning printed — no unsuffixed orphan existed.** Confirmed independently: `ls ~/Library/Audio/Plug-Ins/{Components,VST3}/O-Chorus*` returns exactly the two `-dev` bundles. Log: `logs/O-Chorus/build_20260906_224336.log`, archived as `build-install-1.6.3.log`. |
| Installed-bundle version | `CFBundleShortVersionString` = `CFBundleVersion` = **1.6.3** on **both** `O-Chorus-dev.component` and `O-Chorus-dev.vst3`. |
| `auval -v aufx OuCh OuDv` | **`AU VALIDATION SUCCEEDED.`** — verbatim, line 201. `AudioUnit Name: O-Chorus-dev`, **`Component Version: 1.6.3 (0x10603)`** (auval validated the freshly installed build, not a stale registry entry). Unanchored `grep -c FAIL` = **0**; zero `warn`/`error` lines anywhere in the 203-line transcript. Exit 0. Archived as `auval-1.6.3.log`. |

`line-height-normal: 2` is informational (`#preset-prev`/`#preset-next` at 14 px —
the tail-less trio). Per wave 4c this screen cannot reach 0 on a pinned page; the
acceptance criterion used throughout was **check-ui-labels [7] at 0 movers**.

---

## 5. Version — three sources, one number

| source | value |
|---|---|
| `plugins/O-Chorus/CMakeLists.txt:12` | `VERSION 1.6.3` |
| `plugins/O-Chorus/CHANGELOG.md:3` | `## [1.6.3] - 2026-09-06` |
| `PLUGINS.md:47` | `\| O-Chorus \| 📦 Installed \| 1.6.3 \| Audio Effect (Chorus) \| 2026-09-06 \|` |

Bumped from the CMakeLists value (which read `1.6.2` unquoted, as predicted), never
from the registry row. `grep -rn '1\.6\.2' plugins/O-Chorus/CMakeLists.txt PLUGINS.md`
= 0 live occurrences. **Three `1.6.2` mentions were deliberately left**, all
historical records of when something happened: `index.html:45` ("Through v1.6.2 all
six of these stacks read…"), `index.html:611` (the intrinsic-width comparison table's
own before-row), and `js/i18n.js:630` (the v1.6.2 ZH3-08 note). Rewriting any of them
would destroy the record. No duplicate `PLUGINS.md` rows.

---

## 6. Known / deferred — the tail-less trio is NOT clean

Written up in full in `260906-uu7-deferred-items.md`, and — more importantly —
recorded in **shipped source**: the rewritten `CJK TAIL — ORDER` comment block and
the 1.6.3 CHANGELOG's `### Known / deferred` section both carry it, so the next
reader learns it without needing `.planning/`.

The three tail-less rules read `Garamond, 'Times New Roman', serif`. Times New Roman
holds none of the glyphs they render, so the bare generic **is** reached — identically
in all three languages, so this is the same mechanism, permanently, not a language
defect:

| element | glyph | resolved face |
|---|---|---|
| `#preset-prev` | ◀ U+25C0 | Hiragino Mincho ProN (a Japanese serif) |
| `#preset-next` | ▶ U+25B6 | Lucida Grande |
| `.gear-btn` | ⚙ U+2699 | Menlo (a monospace face) |

Already visible on screen: `#preset-prev` 20.00 × 21.00 px at y=9.5 against
`#preset-next` 18.33 × 18.00 px at y=11.0 — a matched pair in the markup that is not
one on screen. Out of scope (the plan said "leave the three tail-less declarations
alone"); a candidate for its own quick task.

`.preset-dropdown-item`'s resolved face is **UNMEASURED** — `NO PLATFORM FONTS
REPORTED` on both runs. Absence of evidence, not a clean result.

---

## 7. What the plan predicted that turned out FALSE

1. **"The `width: 65px` pin on `.settings-select` is the one most likely to break."**
   FALSE, and inverted: it is the pin that most clearly *held*. The zh intrinsic
   width went 64 → 65 and now matches en/fr exactly, so the reorder **CLOSED** the
   assertion-7 mover the pin was written to absorb. The pin is now a no-op on this
   host, kept as insurance.
2. **"What moves is the ZH `normal` line box the comment quotes as 13.00 px, which
   was Songti's metric — re-quote it."** FALSE, and this was the plan's *central*
   expectation about the re-measurement. Songti SC and PingFang SC give the **same**
   13.00 px `normal` line box at 9 px, so every `line-height: 1.1111` held at 9.984 px
   in all three arms and no line-height pin needed a new number.
3. **`.settings-toggle`'s `min-width: 42px` "does not protect against growth."** Right
   in principle, but it never fired — 42.00 × 16.00 in all three arms on both sides.
   The real correction is that the floor was never what held the rectangle; `height:
   16px` and short captions are.
4. **The v1.5.0 census "FIVE of this page's EIGHT."** FALSE — **six of nine**
   (`.settings-toggle` was the uncounted sixth, added by v1.6.0). The plan flagged
   this as likely-wrong and it was.
5. **The comment names the third tail-less rule `#gear-btn`.** The declaration is on
   `.gear-btn`, a class.
6. **"Latin still resolves to Garamond FIRST, so English geometry is unmoved"**
   (v1.5.0 comment). FALSE when written — Garamond is not a macOS face; Latin lands
   on Times New Roman.
7. **"The Chinese endonym resolves through PingFang SC"** (v1.5.0 comment on the
   65 px derivation). FALSE — the run was Songti SC, because the tail was dead. The
   number was right and the cause was wrong.
8. **The endonyms "are identical in every language, whatever the page language."**
   FALSE before the fix — 36.797 px on en/fr but 36.000 px on zh. It became true
   *because of* this change. The three quoted decimals `27.501 / 30.489 / 36.792`
   matched nothing; measured values are 27.516 / 30.500 / 36.797.
9. **`.settings-select`'s `line-height: 1.2` "measured 16.00 px in both languages."**
   The declaration is **INERT** — computed line-height reads `normal`; Chromium's UA
   sheet overrides it on `<select>`. The 16 px comes from `height` and padding.
10. **`.settings-toggle`'s `line-height: 1.2`** is also a NO-OP (pinned 14.000 ==
    `normal` 14.000 under both faces). **Predicted by neither the plan nor Task 1** —
    a third dead declaration found on this page.
11. **Plan step 7: the three tail-less declarations "should be clean" because "'Times
    New Roman' is a macOS face."** FALSE — being installed is not the same as holding
    the glyph. All three reach the generic (§6).
12. **`measure-ui.js`'s `ff` field as face evidence.** The plan warned about it and it
    was the dominant trap in the row diff: 291 of 315 differing cells are `ff`, which
    merely echoes the edited CSS. Only the CDP probe answers which face rendered.
13. **`before-rows.json` as the source for the pin rewrites** (plan Task 1 step 4).
    The rows structurally cannot carry three of the numbers the pins need: the tooltip
    is never measured open (`vis=false` in all arms — `i18n-states.json` clicks the
    gear but never hovers, so the `18 × 12` box in the rows is the empty at-rest
    plate), `#lang-select` reads `w=65` in every arm *because the pin is there*, and
    every `line-height` pin comment quotes a `normal` counterfactual the pin overrides.
    Task 1 extended the CDP probe to capture all three and re-took the BEFORE run end
    to end so the comparison stayed symmetric.
14. **Plan Task 1 verify #6 and Task 3 verifies #4 and #6 as spelled.** FALSE REDs on
    macOS: BSD `wc -l` pads its output with leading spaces, so `| wc -l | grep -qx '0'`
    can never match. Working form inserts `| tr -d ' '`. The `grep -c … | grep -qx '0'`
    forms (Task 2 verify 1, Task 3 verifies 7 and 8) run as written.
15. **"The tooltip's 384 px cap moves wrap points."** TRUE — and the *only* movement
    prediction that fired. It fired on the surface the plan ranked below the select.

---

## 8. Deviations from plan

**None under Rules 1–4 in Task 3.** No bug, missing functionality, blocker or
architectural question arose. The version bump, CHANGELOG, build, install and auval
all ran first-time green.

**Recorded in Task 1 (carried forward for the record):** one Rule 2 deviation — the
CDP probe was extended with a rendered-tip-geometry block, a `line-height: normal`
counterfactual block and the three tail-less selectors, and the BEFORE run was
re-taken end to end with the final script, because `before-rows.json` structurally
could not supply three numbers the pin rewrites required (prediction 13 above). Per
the plan's own Rule C — "if a number is missing from the BEFORE capture, go back and
capture it, do not reason about what it would have been."

**Scope note (not a deviation):** the two inert `line-height: 1.2` declarations were
**kept**, not removed. Removing an inert declaration is a behavioural change on any
host whose UA stylesheet does not override it, and the charter was a font-ordering
fix plus honest comments, not a cleanup sweep. Both are now documented as inert with
the measurement that proves it.

---

## 9. Known stubs

None. No placeholder, TODO, empty-value or unwired data path was introduced. No test
was skipped and every `<verify>` in the plan was run (three of them in a corrected
form — see prediction 14).

---

## 10. Threat flags

None. No new network endpoint, auth path, file-access pattern or schema change at a
trust boundary. The task's only boundary is WebView page → host font stack, which the
plan's threat model already registers, and no untrusted input crosses it — the change
selects which locally-installed face renders text. All three registered mitigations
held: T-uu7-01 (the byte-difference assertion between the row files made an unmeasured
pin rewrite impossible), T-uu7-02 (dual-variant sweep + installed-bundle check, no
orphan), T-uu7-03 (path-scoped commits, immediate pre-commit `git status --short`,
and the two commit-content assertions — no `.planning/`, no `O-Orbit`).

---

## Self-Check: PASSED

| claim | check | result |
|---|---|---|
| `plugins/O-Chorus/CMakeLists.txt` at 1.6.3 | `grep -n 'VERSION 1.6.3'` | FOUND, line 12 |
| `plugins/O-Chorus/CHANGELOG.md` has the entry | `grep -n '^## \[1.6.3\]'` | FOUND, line 3 |
| `PLUGINS.md` row at 1.6.3 | `grep -n '^\| O-Chorus \|'` | FOUND, line 47 |
| no live `1.6.2` | `grep -rn … \| wc -l \| tr -d ' '` | 0 |
| commit `859ab521` exists | `git log --oneline --all \| grep` | FOUND |
| commit `ea1b86cc` exists | `git log -1 --format=%h` | FOUND |
| commit content | `git log -1 --name-only` | `PLUGINS.md`, `plugins/O-Chorus/CHANGELOG.md`, `plugins/O-Chorus/CMakeLists.txt` — nothing else |
| no `.planning/` in the commit | `grep -c '^\.planning/'` | 0 |
| no `O-Orbit` in the commit | `grep -c 'O-Orbit'` | 0 |
| no tag created | `git tag --points-at HEAD \| wc -l` | 0 |
| AU installed at 1.6.3 | PlistBuddy on both bundles | 1.6.3 / 1.6.3 |
| no alternate-variant orphan | `ls ~/Library/Audio/Plug-Ins/{Components,VST3}/O-Chorus*` | exactly the two `-dev` bundles |
| auval green | `grep -n 'AU VALIDATION'` | `AU VALIDATION SUCCEEDED.` line 201; `grep -c FAIL` = 0 |
| artifacts on disk | `ls` the task dir | `before-*` ×6, `after-*` ×7, `cdp-font-probe.js`, `auval-1.6.3.log`, `build-install-1.6.3.log`, three summaries, deferred-items |
