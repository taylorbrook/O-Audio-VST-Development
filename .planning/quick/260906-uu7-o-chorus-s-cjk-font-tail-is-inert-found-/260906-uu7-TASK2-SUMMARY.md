# 260906-uu7 — Task 2: reorder, re-measure, re-pin

**Status:** COMPLETE. All gates green. Task 3 (ship 1.6.3) is unblocked.
**Executed:** 2026-09-07 (UTC), repo root `/Users/taylorbrook/Dev/VST-development`, branch `main`.
**Commit:** `859ab521` — `fix(O-Chorus): CJK tail before the trailing generic — Han renders PingFang SC`
**Files in the commit:** `plugins/O-Chorus/Source/ui/public/index.html` — that one file, nothing else. No tag created.
**index.html:** md5 `2c1ed43836f668f87ec005f980cac744` → `235cd075a8bb93cecd4c81d770983675`; 183 insertions / 51 deletions (six stack reorders + nine comment rewrites).

---

## 1. The reorder

All six tailed declarations now read

```
font-family: Garamond, 'Times New Roman', 'PingFang SC', 'Microsoft YaHei', serif;
```

`.container` (65) · `.preset-action` (185) · `.settings-label` (475) · `.settings-select` (504) · `.settings-toggle` (572) · `.tooltip` (668) — pre-edit line numbers, from a live grep at execution time.

The three tail-less rules — `.preset-nav` (130), `.preset-dropdown-item` (233), `.gear-btn` (404) — were left byte-unchanged per plan scope.

Structural verifies, all PASS:

| check | result |
|---|---|
| `grep -n font-family \| grep -c "serif, 'PingFang SC'"` | **0** |
| node one-liner: >1 generic OR generic ahead of the tail | **0 bad stacks** |
| live census: tailed / total | **6 / 9** — the comment now says "SIX of this page's NINE" |
| `git diff --stat` over `js`, `modules`, `DSP`, `*.cpp` | **0 lines** — only `index.html` touched under `Source/` |

---

## 2. AFTER resolved Han face — PingFang SC on all six

CDP `CSS.getPlatformFontsForNode`, `cdp-font-probe.js` re-run **unchanged** from Task 1 (same script, same targets, same three arms). Transcript: `after-fonts.txt`.

| selector | BEFORE zh-Hans | AFTER zh-Hans |
|---|---|---|
| `.knob-label` | Songti SC (2) | **蘋方-簡 / PingFangSC-Regular (2)** |
| `.preset-action` | Songti SC (2) | **蘋方-簡 / PingFangSC-Regular (2)** |
| `.settings-label` | Songti SC (2) | **蘋方-簡 / PingFangSC-Regular (2)** |
| `.settings-select` | Songti SC (4) | **蘋方-簡 / PingFangSC-Regular (4)** |
| `.settings-toggle` | Songti SC (1) | **蘋方-簡 / PingFangSC-Regular (1)** |
| `.tooltip` | TNR (15) + Songti SC (63) | **蘋方-簡 (63)** + TNR (15) |
| `.tip-title` | Songti SC (2) | **蘋方-簡 / PingFangSC-Regular (2)** |

`蘋方-簡` is PingFang SC's localized `familyName`; the postscript name `PingFangSC-Regular` is the identifier. `grep -ci songti after-fonts.txt` = **0**.

**en and fr are byte-identical before and after** — Times New Roman / TimesNewRomanPSMT on every selector. Garamond is not a macOS face and resolved on neither side.

---

## 3. Every pin: before → after, per arm, and what was rewritten

**Headline: not one pin VALUE had to change.** Every one of the seven pinned selectors held. What changed is that each comment now quotes a number measured under the face that actually renders, and three of them had false *causes* that are now gone.

| pin | arm | BEFORE | AFTER | held? | what was rewritten |
|---|---|---|---|---|---|
| `.knob-label` `line-height: 1.1111` | en / fr / zh | pinned line box 9.984 px; `normal` 10.00 / 10.00 / **13.00** | 9.984 px; `normal` 10.00 / 10.00 / **13.00** | **HELD** | Comment gains a re-measurement paragraph: Songti and PingFang give the *same* 13.00 px `normal` box at 9 px, so the 10/9 ratio holds. Names the width move (below) and states that HEIGHT is unchanged at 9.98 px, which is what keeps `.knob-value` / `.knob-container` still. Trailing `/* measured EN line box … */` replaced with the three-arm AFTER numbers. |
| `.knob-label` rendered box | zh | 19.61 × 9.98 (2 chars) / 29.41 × 9.98 (3 chars) | **20.00** × 9.98 / **30.00** × 9.98 | MOVED +0.39 / +0.59 w | Recorded in the comment; span is centred so x shifts −0.19 px and nothing follows. |
| `.preset-action` `line-height: 1.1111` | en / fr / zh | 9.984 px; `normal` 10.00 / 10.00 / 13.00; box 62.00 × 13.98 all arms | identical | **HELD** | Re-measurement paragraph + rewritten trailing comment. Notes the width pin (62 px) is why the wider advance has nowhere to go. |
| `.settings-label` `line-height: 1.1111` | en / fr / zh | 9.984 px; `normal` 10.00 / 10.00 / 13.00 | identical | **HELD** | Re-measurement paragraph naming the caption growth and why it moved nothing (space-between row eats popover slack). |
| `.settings-label` rendered box | zh | LANGUAGE 19.20 × 9.98 · HOVER-HELP 38.41 × 9.98 | **19.61** · **39.20** | MOVED +0.41 / +0.79 w | In the comment, with the popover-unchanged corroboration. |
| **`.settings-select` `width: 65px`** | en / fr / zh | intrinsic (`width: auto`) **65.000 / 65.000 / 64.000** | **65.000 / 65.000 / 65.000** | **HELD — and the mover it existed for is GONE** | Comment fully re-derived. Removed the false cause ("that run resolves through PingFang SC" — it was Songti), the false invariance claim, and the three decimals `27.501 / 30.489 / 36.792` that matched nothing. Now carries the two-row intrinsic-width table and says the pin is a no-op on this host, kept as insurance for a host that picks a third face. |
| `.settings-select` endonym option widths | en / fr / zh | en/fr: 27.516 / 30.500 / **36.797**; zh: 27.516 / 30.500 / **36.000** | **27.516 / 30.500 / 36.797 in all three arms** | now genuinely language-invariant | In the comment, with the before-state named as the counterexample. |
| `.settings-select` `line-height: 1.2` | all | computed **`normal`** (UA sheet overrides on `<select>`) | computed **`normal`** | INERT both sides | **KEPT**, and the comment now says it is inert and why it stays: deleting it buys nothing measurable here, and a host whose UA sheet does not override `<select>` would need it. The v1.5.0 "measured 16.00 px in both languages" claim is gone — the 16 px comes from `height`/padding. |
| `.settings-toggle` `min-width: 42px` | en / fr / zh | 42.00 × 16.00 all arms | 42.00 × 16.00 all arms | **HELD** | Comment gains the re-measurement plus two honesty corrections: a floor protects nothing on the growth side, and what actually holds the rectangle is that no caption gets near 42 px. 开 fits with room. |
| `.settings-toggle` `line-height: 1.2` | all | pinned 14.000 px == `normal` 14.000 px | identical | NO-OP both sides | Documented as a no-op; `height: 16px` does the work. Kept for the same host-insurance reason. |
| `.tooltip` `line-height: 1.3` | en / fr / zh | `.tip-title` 366 × **11.69** all arms; body line box 13.00 px | identical | **HELD** | Comment rewritten: v1.5.0's "11.688 px in BOTH languages" holds and holds in the third arm too; the six-line ceiling arithmetic (109 px well, 25.70 px chrome, 83.30 px budget) is untouched. |
| `.tooltip` `max-width: 384px` | zh | Drive tip 384 × 51.69, 2 body lines | 384 × 51.69, 2 lines | HELD | **Wrap DID move elsewhere:** `voices` zh tip **384.0 × 51.7 (2 lines) → 384.0 × 64.7 (3 lines)**; `#tips-toggle` zh tip **328.0 → 334.8 px** wide. Both named in the comment. Tallest zh tip is now 64.7 px against a 109 px well — three lines under the ceiling, 44.3 px headroom intact. |
| `.settings-popover` `width: 170px` | en / fr / zh | 170.00 × 54.00 all arms | 170.00 × 54.00 all arms | **HELD** | Comment gains a re-measurement note: "language-invariant by construction" is now checked against a *second* Han face rather than only the Songti SC it was unknowingly measured under. |

### The `v1.5.0 CJK TAIL` comment block — fully rewritten

Retitled `CJK TAIL — ORDER (v1.6.3, quick-260906-uu7; the tail itself is v1.5.0)`. Four things were false and are now corrected:

1. **The ordering rule is stated and attributed.** A generic terminates the search; Chromium resolves it against the document's lang; under `lang="zh-Hans"` the generic is already a Chinese face. Attributed to `CSS.getPlatformFontsForNode`, wave 4a (`quick-260904-qrc`), with the before/after pair for *this* page in `quick-260906-uu7`. States why a second generic is dead code by construction and why the surviving generic is `serif`.
2. **Census corrected** from "FIVE of eight" to **SIX of NINE**, with `.settings-toggle` added to the list and a note that the v1.5.0 census was stale the moment v1.6.0 added it.
3. **`.gear-btn` spelling corrected** — the declaration is on the CLASS; v1.5.0 spelled it `#gear-btn`.
4. **The Garamond claim replaced.** Garamond is not a macOS face; Latin lands on Times New Roman, so "Latin still resolves to Garamond FIRST" was false when written. Replaced with what was actually measured: en/fr geometry is byte-unchanged across the reorder apart from the animated `#lfo-dot`, with check-ui-labels [7] at 0 movers on both arms.

Also added, as **recorded-not-fixed**: the three tail-less rules are untouched but *not* clean — all three reach the bare generic (◀ Hiragino Mincho ProN, ▶ Lucida Grande, ⚙ Menlo) in every arm, and ◀ 20.00 × 21.00 vs ▶ 18.33 × 18.00 is already visible on screen.

---

## 4. Row-level diff — the complete list of what moved

339 rows / 113 DOM keys / 55 display ids, identity identical before and after. Differing cells: **315** — `ff:291` (the DECLARED stack string, which merely echoes the CSS edit and is evidence of nothing), `x:11`, `y:3`, `w:10`.

```
en       #lfo-dot            x 362.62->361.99  y 45.57->45.09    (animated, excluded by [7])
fr       #lfo-dot            x 361.43->361.66  y 44.68->44.85    (animated)
zh-Hans  #lfo-dot            x 361.61->361.64  y 44.81->44.83    (animated)
zh-Hans  span.knob-label     x 56.69->56.50    w 19.61->20.00    x8 knobs (one at 29.41->30.00)
zh-Hans  span.settings-label                   w 19.20->19.61
zh-Hans  span.settings-label                   w 38.41->39.20
```

**Every non-animated mover is a zh-Hans Han-carrying label widening ~2% under PingFang's advance. No HEIGHT changed anywhere** — that is why nothing downstream was displaced and why check-ui-labels [7] stays at 0.

`Han-carrying visible rows: 26 | geometry moved on: 10`. Byte-identity check against `before-rows.json`: **DIFFERENT** — the reorder is measurable, which is the plan's own anti-guessing assertion.

---

## 5. Gate exit codes and counts on the committed tree

All four re-run **after** the comment rewrites, serialized, in background.

| gate | exit | counts |
|---|---|---|
| `node scripts/check-ui-labels.js --plugin O-Chorus` | **0** | `== ALL CHECKS PASSED ==`, **87 PASS, 0 FAIL**. `[7][GEOMETRY DIFF]` reports **0 moved elements on fr AND on zh-Hans** (1 animated `#lfo-dot` excluded and disclosed), visible element SET identical on both. Coverage 14/14 `[data-i18n]`. Not 77, not "nothing to measure". |
| `node plugins/O-Chorus/tests/ui_tip_render_check.js` | **0** | `== ALL CHECKS PASSED == (345 passed)`, **0 FAIL**. Real zh-Hans pass. |
| `node scripts/measure-ui.js --plugin O-Chorus --mode box --report all` | **0** | four screens, **none SKIPPED**, counts identical to BEFORE: `undeclared-font: 3` · `line-height-normal: 2` · `wrap-count: 0` (27 leaves, 13 estimated) · `svg-font-attr: 0 carriers / 0 findings`. |
| `node scripts/measure-ui.js --plugin O-Chorus --mode box` (rows) | **0** | 339 rows; `languages en, fr, zh-Hans  states default + 1` — the gear IS clicked, the popover IS measured open. |

Criterion met as specified: **check-ui-labels exit 0 with [7] at 0 movers on fr and zh-Hans**, and **ui_tip_render_check 0 FAIL**. `line-height-normal: 2` is informational (`#preset-prev`/`#preset-next` at 14 px — the tail-less trio; this screen cannot reach 0 on a pinned page).

---

## 6. What Task 3 must know

1. **No gate fixture was changed.** `plugins/O-Chorus/tests/` is untouched — `ui_tip_render_check.js` and `i18n-states.json` are byte-unchanged. The wrap move (`voices` 2 → 3 lines) needed no re-recording; the gate asserts containment, not an exact rect.
2. **The width pin value did NOT change.** `.settings-select` still reads `width: 65px`. Only its *derivation* changed (zh intrinsic 64 → 65). Nothing downstream depends on a new number.
3. **No pin value changed at all.** If the CHANGELOG needs "which pins moved", the honest answer is *none of the pin values*; what moved are **measurements**: the select's zh intrinsic width 64.000 → 65.000 px, the zh endonym 36.000 → 36.797 px, eight `.knob-label` widths 19.61 → 20.00 (one 29.41 → 30.00), two `.settings-label` widths 19.20 → 19.61 and 38.41 → 39.20, and two tooltip wraps (`voices` 384.0 × 51.7 → 384.0 × 64.7, `#tips-toggle` 328.0 → 334.8 px). Those are the numbers the CHANGELOG should carry.
4. **Do not claim the three tail-less declarations were reviewed and found clean.** Task 1 proved they all reach the bare generic (Hiragino Mincho ProN / Lucida Grande / Menlo) with a visible ◀/▶ mismatch. Out of scope here, recorded in the comment and in the final SUMMARY as a candidate for its own quick task.
5. **`.preset-dropdown-item`'s resolved face is still UNMEASURED** — the probe reported `NO PLATFORM FONTS REPORTED` on both runs. Do not record it as clean.
6. **RULE-D trap, still live for Task 3.** BSD `wc -l` pads its output, so `| wc -l | grep -qx '0'` can never match on macOS — it is a **false RED**. This affects Task 3 verify 4 (`grep -rn '1\.6\.2' … | wc -l | grep -qx '0'`) and verify 6 (`uniq -d | wc -l | grep -qx '0'`). Insert `| tr -d ' '` before the grep. The `grep -c … | grep -qx '0'` forms (verifies 7 and 8) run as written.
7. **Version truth is `plugins/O-Chorus/CMakeLists.txt`.** Bump from what it actually reads. `Source/ui/public/js/i18n.js:630` mentions 1.6.2 inside a historical comment — that is a record of when something happened and must NOT be rewritten.
8. **Working tree at handoff:** clean under `plugins/`. Uncommitted and NOT to be staged: `.gsd/dispatch-isolation-sentinel.json` (pre-existing, untouched by this task) and the untracked `.planning/quick/260906-uu7-…/` artifacts.

---

## 7. Plan predictions that turned out false

1. **"The `width: 65px` pin on `.settings-select` is the one most likely to break."** — FALSE. It is the pin that most clearly *held*: the zh intrinsic went 64 → 65 and now matches en/fr exactly. The reorder **closed** the assertion-7 mover the pin was written for.
2. **"What moves is the ZH `normal` line box the comment quotes as 13.00 px, which was Songti's metric. Re-quote it."** — FALSE. Songti SC and PingFang SC give the *same* 13.00 px `normal` line box at 9 px. Every `line-height` pin held at its existing value; none needed a new number. This was the plan's central expectation about what the re-measurement would find.
3. **`.settings-toggle`'s 42 px floor "does not protect against growth"** — the *warning* was right in principle but did not fire: the button is 42.00 × 16.00 in all three arms on both sides. The real correction is that the floor was never what held it — `height: 16px` and short captions are.
4. **`.settings-select`'s `line-height: 1.2` "measured 16.00 px"** (v1.5.0 comment, flagged by Task 1) — confirmed INERT; computed reads `normal`. Kept, now documented.
5. **`.settings-toggle`'s `line-height: 1.2`** — also a NO-OP (pinned 14.000 == `normal` 14.000 under both faces). **Not predicted by the plan or by Task 1.** Third dead declaration found on this page.
6. **The tooltip's 384 px cap "moves wrap points"** — TRUE, and the only movement prediction that fired. It fired on the surface the plan ranked *below* the select.
7. **Census "six of nine"** — CONFIRMED (the plan itself flagged it as one of three likely-wrong numbers).

---

## 8. Deviations

- **None from Rules 1–4.** No bug, missing functionality or blocker was encountered. The reorder applied cleanly to all six stacks in one pass and every gate was green on the first AFTER run.
- **Scope note (not a deviation):** `.settings-select`'s and `.settings-toggle`'s inert `line-height: 1.2` declarations were **kept**, not removed. Removing an inert declaration is a behavioural change on any host whose UA stylesheet does not override it, and the task's charter is a font-ordering fix plus honest comments — not a cleanup sweep. Both are now documented as inert with the measurement that proves it.
- **Artifacts written to the task dir only:** `after-rows.json`, `after-fonts.txt`, `after-gates.txt`, `after-check-ui-labels.log`, `after-ui-tip-render-check.log`, `after-measure-ui-report.log`/`.txt`, `after-rows.stderr.log`. None committed.
