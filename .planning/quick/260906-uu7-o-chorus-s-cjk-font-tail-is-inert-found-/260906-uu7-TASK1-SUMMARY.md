# 260906-uu7 — Task 1 (tracer): BEFORE baseline

**Status:** COMPLETE. Premise CONFIRMED. `plugins/O-Chorus` is byte-unchanged.
**Executed:** 2026-09-07 (UTC), repo root `/Users/taylorbrook/Dev/VST-development`, branch `main`.
**Tree measured:** `index.html` md5 `2c1ed43836f668f87ec005f980cac744`, `git status --short -- plugins/O-Chorus` = 0 lines.
**Commits:** none (measurement task).

---

## 1. Baseline artifacts — absolute paths

| Artifact | Absolute path |
|---|---|
| Raw measurement rows (339) | `/Users/taylorbrook/Dev/VST-development/.planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/before-rows.json` |
| CDP resolved-face transcript | `/Users/taylorbrook/Dev/VST-development/.planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/before-fonts.txt` |
| Consolidated baseline (10 sections) | `/Users/taylorbrook/Dev/VST-development/.planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/before-gates.txt` |
| check-ui-labels full log | `/Users/taylorbrook/Dev/VST-development/.planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/before-check-ui-labels.log` |
| ui_tip_render_check full log | `/Users/taylorbrook/Dev/VST-development/.planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/before-ui-tip-render-check.log` |
| measure-ui four-screen report | `/Users/taylorbrook/Dev/VST-development/.planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/before-measure-ui-report.log` |
| **CDP probe script — Task 2 re-runs UNCHANGED** | scratchpad original: `/private/tmp/claude-501/-Users-taylorbrook-Dev-VST-development/c0bfbc05-32a8-4700-8e15-8f1937a76457/scratchpad/cdp-font-probe.js`<br>durable copy: `/Users/taylorbrook/Dev/VST-development/.planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/cdp-font-probe.js` |

The probe was copied into the task dir because the scratchpad path is session-scoped and Task 2 runs in a different session. **It must never be committed** — Task 3 stages only `plugins/O-Chorus` and `PLUGINS.md`, so it will not be. Run it as:

```
node .planning/quick/260906-uu7-o-chorus-s-cjk-font-tail-is-inert-found-/cdp-font-probe.js --label AFTER > <taskdir>/after-fonts.txt 2>&1
```

It hard-codes `REPO = /Users/taylorbrook/Dev/VST-development` and requires `scripts/serve-ui.js` from there, so it runs from any cwd.

---

## 2. Resolved Han face per stack — the Songti SC evidence

`CSS.getPlatformFontsForNode` over CDP, six tailed selectors × three arms. **Every zh-Hans Han run resolves to `Songti SC / STSongti-SC-Regular`. `PingFang SC` is named in all six stacks and reached by none.** Premise confirmed; the reorder is authorized.

| selector | en | fr | zh-Hans |
|---|---|---|---|
| `.knob-label` (inherits `.container`) | Times New Roman (4) | Times New Roman (7) | **Songti SC (2)** |
| `.preset-action` | Times New Roman (4) | Times New Roman (7) | **Songti SC (2)** |
| `.settings-label` | Times New Roman (8) | Times New Roman (6) | **Songti SC (2)** |
| `.settings-select` | Times New Roman (7) | Times New Roman (8) | **Songti SC (4)** |
| `.settings-toggle` | Times New Roman (2) | Times New Roman (6) | **Songti SC (1)** |
| `.tooltip` | Times New Roman (209) | Times New Roman (241) | Times New Roman (15) + **Songti SC (63)** |
| `.tip-title` | Times New Roman (5) | Times New Roman (10) | **Songti SC (2)** |

`(n)` is `glyphCount`. The zh `.tooltip` row is a two-face run: the literal `tanh` inside the Chinese body stays on Times New Roman while the Han goes to Songti SC — that mixed run is what the 384 px wrap ceiling is counted in.

**Latin never resolves to Garamond.** It resolves to `Times New Roman / TimesNewRomanPSMT` in en and fr. Garamond is not a macOS face, so the v1.5.0 comment's closing claim ("Latin still resolves to Garamond FIRST, so English geometry is unmoved") was false when written.

Mechanism confirmed in source: `index.html:21` `<html lang="en">`, `index.html:1226` `document.documentElement.lang = uiLanguage;`.

---

## 3. Gate exit codes and headline counts on the unmodified tree

| Gate | Exit | Headline |
|---|---|---|
| `node scripts/measure-ui.js --plugin O-Chorus --mode box --report all` | **0** | four screens, **none SKIPPED**. `undeclared-font: 3` · `line-height-normal: 2` · `wrap-count: 0` (27 leaves compared, 13 estimated) · `svg-font-attr: 0 carriers / 0 findings` |
| `node scripts/check-ui-labels.js --plugin O-Chorus` | **0** | `== ALL CHECKS PASSED ==`, 87 PASS lines, 0 FAIL. `[7][GEOMETRY DIFF]` passes on **both** fr and zh-Hans — 0 non-label elements moved (1 animated `#lfo-dot` excluded and disclosed). Coverage 14/14 `[data-i18n]` visible. Not 77, not "nothing to measure" — a real pass. |
| `node plugins/O-Chorus/tests/ui_tip_render_check.js` | **0** | `== ALL CHECKS PASSED == (345 passed)`, 0 FAIL. Real zh-Hans pass; every zh tip rect 384.0 × 51.7 inside 700 × 125. |
| `node scripts/measure-ui.js --plugin O-Chorus --mode box` (rows) | **0** | 339 rows / 113 DOM keys / 55 display ids; `languages en, fr, zh-Hans  states default + 1` — the gear IS clicked, the popover IS measured open. |

Precondition satisfied: measure-ui exited 0, never 77. **No pre-existing red anywhere** — any red after Task 2's edit is Task 2's.

---

## 4. What Task 2's executor must know that the plan does not say

### 4.1 `before-rows.json` cannot answer three of the questions Task 2 asks it

- **The tooltip is never measured open.** `#tooltip` and `.tip-title` read `vis=false` in all three arms. `measure-ui.js` replays `tests/i18n-states.json`, which only *clicks* `#gear-btn` — it never *hovers*, so the tip is never in `.show`. The `18 × 12` box in the rows is the empty at-rest plate. Reading it as the tooltip's geometry is a wrong number, not a missing one. The rendered geometry is in `before-fonts.txt` under `[tip geometry RENDERED]` and in `before-ui-tip-render-check.log`.
- **`#lang-select` reads `w=65` in all three arms because `width: 65px` is pinned.** The pin's derivation — the intrinsic width — is invisible in the rows.
- **Every line-height pin comment quotes a counterfactual** measured at `line-height: normal`, which the pin overrides. The rows read `9.9999px` everywhere and can never reproduce the comments' "EN 10.00 / ZH 13.00".

The probe was extended to capture all three. Task 2 gets them for free by re-running it.

### 4.2 The BEFORE counterfactuals — these are the numbers to compare against

Measured by temporarily overriding the declaration in-page and reverting immediately (nothing written to disk):

| selector | pinned line box (all arms) | at `line-height: normal`: en / fr / zh-Hans |
|---|---|---|
| `.knob-label` | 9.984 px | 10.000 / 10.000 / **13.000** |
| `.preset-action` | 9.984 px | 10.000 / 10.000 / **13.000** |
| `.settings-label` | 9.984 px | 10.000 / 10.000 / **13.000** |
| `.settings-toggle` | 14.000 px | 14.000 / 14.000 / 14.000 |

The comments' "EN line box 10.00 px / 9 px font" and "13.00 px in Chinese at line-height: normal" **reproduce exactly**, measured under Songti SC. Task 2 re-takes the zh column under PingFang SC; en/fr are Latin-derived and should not move.

`.settings-toggle`'s `line-height: 1.2` is currently a **no-op**: pinned 14.000 == normal 14.000 in all three arms. Its `height: 16px` is doing the work the comment credits to the line-height pin.

### 4.3 The `.settings-select` 65 px pin — its stated cause is already wrong

| | en | fr | zh-Hans |
|---|---|---|---|
| pinned width | 65.000 | 65.000 | 65.000 |
| **intrinsic (`width: auto`)** | 65.000 | 65.000 | **64.000** |

The comment's "measured 65 px in English, 65 px in French, 64 px in Chinese" reproduces exactly — but it attributes the 64 to "that run resolves through **PingFang SC**", and the CDP probe proves the run is **Songti SC**. The cause named in the comment is false today. After the reorder the run genuinely becomes PingFang SC and the 64 may or may not survive. **Re-derive it with `width: auto`, never from the pinned rows.**

Endonym option widths, measured in a detached span inheriting the select's stack:

| arm | English | Français | 简体中文 |
|---|---|---|---|
| en | 27.516 | 30.500 | 36.797 |
| fr | 27.516 | 30.500 | 36.797 |
| zh-Hans | 27.516 | 30.500 | **36.000** |

The comment claims all three are "identical in every language … whatever the page language" and quotes `27.501 / 30.489 / 36.792`. **Both halves are wrong**: the Chinese endonym is 36.797 on en/fr and 36.000 on zh (the generic resolves to a different Han face per document lang), and none of the three quoted decimals matches. Prediction for Task 2: after the reorder all three arms name PingFang SC, so the endonym width should become genuinely language-invariant for the first time.

### 4.4 `.settings-select`'s `line-height: 1.2` is INERT

Computed `line-height` on `#lang-select` reads **`normal`**, not `10.8px`, in all three arms — Chromium's UA stylesheet overrides it on `<select>`. The comment's "line-height: 1.2 … measured 16.00 px in both languages, so it needed no new pin" describes a declaration the engine discards; the 16 px comes from `height`/padding, not from the ratio. This is a *second* dead declaration on the same rule as the dead font tail. Task 2's comment rewrite should say so.

### 4.5 The census — CONFIRMED, with one naming correction

Six tailed, three tail-less, nine total (plus the comment at line 40). The plan's numbers hold.

| | line | selector |
|---|---|---|
| tailed | 65 | `.container` |
| tailed | 185 | `.preset-action` |
| tailed | 475 | `.settings-label` |
| tailed | 504 | `.settings-select` |
| tailed | 572 | `.settings-toggle` |
| tailed | 668 | `.tooltip` |
| tail-less | 130 | `.preset-nav` |
| tail-less | 233 | `.preset-dropdown-item` |
| tail-less | 404 | **`.gear-btn`** — a CLASS. The v1.5.0 comment spells it `#gear-btn`; the id exists on the same element and is what `tests/i18n-states.json` clicks, but the comment rewrite must name the selector that carries the declaration. |

The line numbers were re-derived from a live grep at execution time and are current as of md5 `2c1ed43…`.

### 4.6 The tail-less trio is NOT clean — the plan's step-7 prediction is FALSE

The plan predicted these are fine because "'Times New Roman' is a macOS face". Times New Roman holds none of the three glyphs they render, so the bare `serif` generic IS reached, and hands each a different fallback — **identically in en, fr and zh-Hans**, so this is not a language defect but the same mechanism, permanently:

| element | glyph | resolved face |
|---|---|---|
| `#preset-prev` | ◀ U+25C0 | **Hiragino Mincho ProN / HiraMinProN-W3** (a Japanese serif) |
| `#preset-next` | ▶ U+25B6 | **Lucida Grande / LucidaGrande** |
| `#gear-btn` | ⚙ U+2699 | **Menlo / Menlo-Regular** (a monospace face) |

It already has a **visible consequence**: the two nav arrows are a matched pair in the markup and are not one on screen, because they resolve to two different faces —

```
#preset-prev  20.00 × 21.00 px  at y=9.5
#preset-next  18.33 × 18.00 px  at y=11.0
```

a 1.67 px width and 3.00 px height mismatch between ◀ and ▶.

**Scope:** the plan says "Do not edit the three tail-less declarations", so this is recorded, not fixed, in 260906-uu7. It is a candidate for its own quick task and is the same defect class this task closes. Task 3's CHANGELOG should not claim these three were reviewed and found clean.

`.preset-dropdown-item` could **not** be probed — the dropdown rendered no item under the probe's gesture (`NO PLATFORM FONTS REPORTED`). Its resolved face is **unmeasured**; do not record it as clean.

### 4.7 The three `undeclared-font` findings are the trio, via aria-label

All three findings sit on the zh-Hans arm and are `#preset-prev` / `#preset-next` / `#gear-btn`. They flag because the screen treats `aria-label` as a Han *carrier* and their zh aria-labels hold Han (`上一个预设`, `下一个预设`, `设置`). Their *rendered* text is a symbol glyph. So the count of 3 is expected and is not the tail defect — but per 4.6 it is not a clean bill of health either. Expect the same 3 in the AFTER run; a change in that number is the signal.

`svg-font-attr` reads `0 attribute carrier(s), 0 finding(s)` — this page has no SVG `font-family` presentation attribute at all, so the screen's own liveness signal is zero. Read as **not applicable**, not as clean.

`line-height-normal: 2` is `#preset-prev`/`#preset-next` at 14 px. Consistent with memory (wave 4c): this screen cannot reach 0 on this page. It is **not** the acceptance criterion — `check-ui-labels [7]` at 0 moved elements is.

### 4.8 RULE-D BLOCKING FINDING — four verify commands do not run as spelled

`Task 1 verify #6` returns **exit 1 on a clean tree** — a false RED:

```
git status --short -- plugins/O-Chorus | tee /dev/stderr | wc -l | grep -qx '0'
```

macOS/BSD `wc -l` pads its output (`od -c` shows seven leading spaces before the `0`), so `grep -qx '0'` can never match. The measured tree state IS clean. Working form adds `| tr -d ' '`:

```
git status --short -- plugins/O-Chorus | wc -l | tr -d ' ' | grep -qx '0'   # exit 0
```

**Same defect, same false RED, in the remaining tasks:**

- Task 2, last verify: `git diff --stat -- … | wc -l | grep -qx '0'`
- Task 3, verify 4: `grep -rn '1\.6\.2' … | wc -l | grep -qx '0'`
- Task 3, verify 6: `… | uniq -d | wc -l | grep -qx '0'`

**Not affected:** the `grep -c … | grep -qx '0'` forms — `grep -c` emits an unpadded count, so Task 2 verify 1 and Task 3 verifies 7 and 8 run as written.

### 4.9 Rendered tooltip geometry — the only BEFORE numbers for the 384 px cap

| arm | tip w × h | line-height | body lines | `.tip-title` w × h |
|---|---|---|---|---|
| en | 384 × 64.69 | 13px | 3 | 366 × 11.69 |
| fr | 384 × 64.69 | 13px | 3 | 366 × 11.69 |
| zh-Hans | **384 × 51.69** | 13px | **2** | 366 × 11.69 |

Drive-knob tip; all three arms hit the 384 px `max-width` cap. `.tip-title` measures 11.69 px in all three — the comment's "11.688 px in BOTH languages" reproduces, and it holds in the third language too. Chinese is the *shortest* tip (2 lines vs 3), which matches memory that Chinese geometry failures are shrinks. `ui_tip_render_check` corroborates: every zh tip rect 384.0 × 51.7, except `#tips-toggle`'s at 328.0 × 38.7.

### 4.10 Practical notes for the AFTER run

- **Serialize the Playwright runs.** Two harnesses at once risk a port clash (memory: a UI-test server on a taken port serves the other session's files). Every capture here was run one at a time.
- **Wall clock:** each measure-ui sweep ≈ 20 s, each gate ≈ 30–60 s, the probe ≈ 25 s. None came near the 600 s watchdog, but all were run with `run_in_background` anyway.
- **`measure-ui.js --verbose` is worth keeping** in the AFTER run: the `states default + 1` line is the only proof the popover was measured open, and the `identity:` line lets the row counts be compared honestly.
- `--report all` writes the JSON array to **stdout** and every count to **stderr**; capture both.

---

## 5. Predictions this task already falsified

1. "Latin still resolves to Garamond FIRST" (index.html:57) — **false**, it is Times New Roman; Garamond is not a macOS face.
2. "the Chinese endonym … resolves through PingFang SC" (index.html:513) — **false**, it is Songti SC.
3. The endonyms "are identical in every language … whatever the page language" (index.html:508) — **false**, 36.797 on en/fr vs 36.000 on zh.
4. The quoted endonym widths `27.501 / 30.489 / 36.792` — **none matches** today's measurement (27.516 / 30.500 / 36.797).
5. `.settings-select`'s `line-height: 1.2` "measured 16.00 px" — the declaration is **inert**; computed line-height is `normal`.
6. The v1.5.0 census "FIVE of this page's EIGHT" — **six of nine**; `.settings-toggle` is missing from its list.
7. The comment names the third tail-less rule `#gear-btn`; the declaration is on `.gear-btn`.
8. Plan step 7: the three tail-less declarations "should be clean" — **false**, all three render through the generic (Hiragino Mincho ProN / Lucida Grande / Menlo) in every language.
9. Plan Task 1 verify #6 as spelled — **false RED on macOS**; three more `wc -l | grep -qx '0'` verifies downstream share the defect.

## 6. Deviations

- **[Rule 2 — missing critical measurement]** The plan's step 4 directs Task 2's pin rewrites to be driven off `before-rows.json`, but the rows structurally cannot carry the tooltip's rendered box, the select's intrinsic width, or any `line-height: normal` counterfactual (4.1). Per Rule C ("if a number is missing from the BEFORE capture, go back and capture it"), the CDP probe was extended with a rendered-tip-geometry block, a counterfactual-pin block, and the three tail-less selectors, and the BEFORE run was re-taken end to end with the final script. `before-fonts.txt` is the output of the **same** script Task 2 will re-run — the comparison is symmetric.
- No files under `plugins/` were read-modified. `git status --short -- plugins/O-Chorus` is 0 lines; `index.html` md5 unchanged.
