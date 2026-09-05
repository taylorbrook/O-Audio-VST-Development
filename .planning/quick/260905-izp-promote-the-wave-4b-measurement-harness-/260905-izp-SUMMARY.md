---
quick_id: 260905-izp
type: quick
status: complete
description: "Promote the wave-4b measurement harness from scratch to scripts/ (D7)"
completed: 2026-09-05
files_created:
  - scripts/measure-ui.js
  - scripts/measure-ui-README.md
commits:
  - 7dcc4308
  - 59ff5826
  - 61382a34
actuals:
  tokens: 34000
  tasks: 3
  commits: 3
---

# Quick task 260905-izp — the wave-4b measurement harness, promoted

**D7 is closed.** The computed-style census that found four defect classes in
wave 4b now lives at `scripts/measure-ui.js`, with the four defect screens it
fed by hand built in as `--report` filters and `scripts/measure-ui-README.md`
carrying the reasoning. No plugin was changed, no existing gate was edited,
`trace.js` was not promoted, and the `tipcheck.js` decision is written down with
its reasoning rather than left implicit.

## Commits — three, path-scoped, on `main`

| Commit | What | Files |
|---|---|---|
| `7dcc4308` | The measurement harness, six defects repaired | `scripts/measure-ui.js` |
| `59ff5826` | The four defect screens as `--report` filters | `scripts/measure-ui.js` |
| `61382a34` | The README, and the header pointer to it | `scripts/measure-ui.js`, `scripts/measure-ui-README.md` |

`git diff --name-only 7dcc4308^..HEAD` over all three lists **only**
`scripts/measure-ui.js` and `scripts/measure-ui-README.md`. Branch was
re-checked as `main` and staging re-checked immediately before each commit; the
submodule guard (`plugins/O-Orbit/libs/SAF`) ran clean before each and never
fired.

## The measured counts

### O-Emulator — frame `620 x 430`, 2 states, 3 languages

| | value |
|---|---|
| languages walked | `en, fr, zh-Hans` |
| rows measured | **210** |
| distinct DOM keys | **70** |
| distinct display ids | **47** |
| visible Han-bearing nodes | **21** |

| screen | `--mode box` | `--mode fonts` |
|---|---|---|
| `undeclared-font` | **0** finding(s) | 0 finding(s) |
| `line-height-normal` | **0** finding(s) | SKIPPED (needs box) |
| `wrap-count` | **0** finding(s) — 29 leaves compared, 14 estimated | SKIPPED (needs box) |
| `svg-font-attr` | **0** carriers, 0 findings | 0 carriers, 0 findings |

All three repair screens read `0`, as expected on a plugin wave 4b already
repaired. O-Emulator carries **no** SVG `font-family` attributes at all — its
carrier form was #3, the custom property `--serif` — so `svg-font-attr` reading
`0 carriers` there is a correct report of an absent carrier form, not a
malfunction.

### O-IntonationPad — frame `800 x 500`, 19 states, 3 languages

| | value |
|---|---|
| languages walked | `en, fr, zh-Hans` |
| rows measured | **2844** |
| distinct DOM keys | **948** |
| distinct display ids | **318** |
| visible Han-bearing nodes | **212** |

| screen | `--mode box` | `--mode fonts` |
|---|---|---|
| `undeclared-font` | **0** finding(s) | 0 finding(s) |
| `line-height-normal` | **55** finding(s) — all 55 on the `zh-Hans` arm | SKIPPED (needs box) |
| `wrap-count` | **2** finding(s) — 368 leaves compared, 126 estimated | SKIPPED (needs box) |
| `svg-font-attr` | **0** carriers, 0 findings | 0 carriers, 0 findings |

The two non-zero results are **findings to read, not defects fixed here** — this
task changed no plugin:

- `line-height-normal` 55, every one on `zh-Hans`, because the `han` carrier
  test only fires on that arm. They are the knob readouts, the viz buttons and
  the two icon buttons, all leaves with a Chinese `data-tip`.
- `wrap-count` 2, both the same node: `div.tooltip-body`, `en=4 line(s)`,
  `fr=3`, `zh-Hans=2`. That is the shared floating tooltip body holding whatever
  tip the walk last opened, so it is a genuine cross-language wrap difference on
  a real rendered element, but not a caption defect.

## The positive controls — both fired, both non-zero

A screen reading `0` is worth nothing beside a control that reports non-zero on
the same data. Two were fired.

**Control 1 — `undeclared-font` on O-Emulator** (the invocation the plan
specifies), verbatim output:

```
POSITIVE CONTROL — button stacks under zh-Hans:
   #preset-prev | "EB Garamond", Garamond, "Adobe Garamond Pro", "Times New Roman", "PingFang SC", "Microsoft YaHei", serif
   #preset-next | ... (identical stack)
   #preset-save | ...   #preset-load | ...   #preset-delete | ...
   button.sel | ...   button | ... x4
   #gear-btn | ...   #tips-toggle | ...
OK: 12/12 buttons name a CJK face, so the 0 above is a measured 0
```

Twelve buttons, twelve naming a CJK face. Plus the independent scan the plan
asks for: `rows.filter(x => x.vis && x.han).length` is **21** on O-Emulator and
**212** on O-IntonationPad, so the screens had non-empty input and their `0` is
a result rather than a vacuum.

Note in passing that this control is itself a live demonstration of the identity
fix: 12 rows, 8 distinct display ids — five `<button>` nodes share the id
`button` and the scratch key would have reported them as one.

**Control 2 — the divergence arm of `svg-font-attr`** (added, not in the plan).
Its finding count is 0 on every plugin measured, which is exactly the shape of a
dead screen, so it needed its own control. One attribute mutated in a saved
measurement, re-run through `--from` with no browser:

```
svg-font-attr: 24 attribute carrier(s), 1 finding(s)
  24 node(s) carry a font-family presentation attribute
    html/body[1]/div[1]/div[3]/div[5]/svg[1]/text[1]  (text)  attr=[Helvetica, sans-serif]  computed=[Garamond, "Times New Roman", serif]
```

The arm reports the divergence it is meant to report. The real `0` is earned.

## The load-bearing check: node identity

The single sharpest proof this is not a copy of the scratch file. Measured, on
the pages themselves:

| plugin | rows | distinct DOM keys | distinct display ids |
|---|---|---|---|
| O-AnalogEQ (`--select text`) | 87 | **29** | **1** |
| O-Emulator | 210 | **70** | **47** |
| O-IntonationPad | 2844 | **948** | **318** |

O-AnalogEQ's sibling SVG `<text>` nodes all share the display id `text`, so the
scratch harness's `lang + ' ' + id` key reported the entire set as **one row**.
Every per-node count wave 4b produced was therefore a lower bound: on
O-IntonationPad the page has 948 nodes where the old key could name 318.

## Corrections to the wave-4b record

Both are carried in the README, not back-edited into the wave-4b documents;
those are a historical record and were left alone.

1. **O-Emulator's shipping frame is `620 x 430`**, parsed from that plugin's own
   `PluginEditor.cpp`. Wave 4b's SUMMARY and `deferred-items.md` both name
   "700 x 380". The wrap-count LESSON is unaffected — a wide viewport hides the
   defect at either number — but the number quoted alongside it is wrong.

2. **`svg-font-attr` reports `0` divergences on O-AnalogEQ, not 24.** The plan
   expected the 24 attributes to read `Garamond, 'Times New Roman', serif`
   against a computed stack carrying the CJK tail. Measured, the 24 attribute
   carriers compute to *exactly their attribute* — no tail. The tail appears
   only on the **5** `<text>` nodes carrying **no** attribute, whose computed
   stack is `Garamond, "Times New Roman", "PingFang SC", "Microsoft YaHei",
   serif`. No CSS rule targets the 24, so the lowest-specificity source is the
   only source and the presentation attribute is what keeps the tail off them.
   Those 24 are numeric tick labels carrying no Han, so it remains a finding to
   read rather than a defect — but it runs the opposite way round from the
   account wave 4b left behind.

## The tipcheck / trace decisions

Both recorded in the README's "What was NOT promoted, and why", and both
confirmed by reading the scratch files rather than taken on the plan's word.

**`tipcheck.js` — NOT promoted, and deliberately not folded in as
`--report tips`.** Three reasons: its unit is a tip binding read from
`TIP_BINDINGS`, not a measured DOM node, so it does not share this file's row
schema and folding it in would make `--from` mean two different things;
`boot-all-uis --strict-tips` already re-queries every missed selector after
settle and distinguishes LATE from DEAD, so the only thing it adds over a
committed tool is the per-language arm; and it carries a fixed `1200 x 900`
viewport — confirmed at line 22 of the scratch file — which is the exact defect
C3 names, so promoting it as written would commit the mistake this task exists
to prevent. Its real home is the tip-render gate **D6** says O-IntonationPad
lacks, a separate pass with its own budget.

**`trace.js` — NOT promoted.** Confirmed by reading it: a one-off console-stack
tracer that hardcodes the warning substring `'tip target'`, takes Chromium's
default viewport (`b.newPage()` with no options), and has no output contract.
Nothing in it that `boot-all-uis --verbose` does not already give.

## Deviations from plan

### 1. [Rule 1 — Bug] Task 1's verify block expected 24 O-AnalogEQ `<text>` rows; there are 29

- **Found during:** Task 1, running the verify block as written.
- **Issue:** the plan derived its expected count from
  `grep -o 'font-family="[^"]*"' | wc -l` = **24**, which counts SVG
  `font-family` *attributes*, not `<text>` *elements*. O-AnalogEQ's markup has
  **29** `<text>` elements, 24 of which carry the attribute and 5 of which do
  not. The verify as spelled therefore fails on a correct implementation.
- **Both spellings, and the observed output of each:**

  As written:
  ```
  if(en.length!==24){...} → en text rows: 29
  FAIL: expected 24 sibling SVG text rows (scratch reported 1), got 29
  ```

  Corrected — it asserts BOTH halves, which is strictly stronger:
  ```
  en text rows: 29
  ffAttr carriers: 24
  distinct display ids among them: 1
  first row ffAttr: "Garamond, 'Times New Roman', serif"
  OK: 29 rows where the scratch key gave 1, 24 of them ffAttr carriers
  ```

  The `ffAttr` half of the original assertion (`en[0].ffAttr` truthy) passes as
  spelled. `distinct display ids: 1` is the collapse the plan predicted,
  reproduced directly.
- **No code change.** The implementation was right; the expected number was
  wrong. The 24 survives as the correct expectation for `svg-font-attr`'s
  carrier count, where the plan also uses it, and that verify passes as written.

### 2. [Rule 1 — Bug] Task 2's third verify block redirects stderr in the wrong order

- **Found during:** Task 2.
- **Issue:** `node ... 2>&1 >/dev/null | tee file` was intended to capture
  stderr only. As spelled it put both streams into the pipe, so the
  `/\b24\b/` assertion could in principle have matched the raw JSON rather than
  the screen's count line — a check that can pass without the screen running.
- **Both spellings run:** as written it passed and the tee file held 3 lines
  including the JSON. Re-run as `2>&1 1>/dev/null`, stderr only:
  ```
  svg-font-attr: 24 attribute carrier(s), 0 finding(s)
    24 node(s) carry a font-family presentation attribute
  ```
  The corrected form is the sound check and it passes.

### 3. [Rule 2 — Missing critical functionality] `LANGUAGES` read from the RESOLVED ui root

- The scratch original hardcoded `plugins/<Name>/Source/ui/public/js/i18n.js`,
  and the plan's `key_links` carried that path. Two plugins (O-MicrotonalSampler,
  O-Orbit) carry both a `Source/ui/public` and a `Resources/ui`, and only one is
  embedded — `serve-ui.js`'s own header records that the other renders a stale
  page. The promoted file reads `built.uiRoot`, which `resolveUiRoot()` derives
  from the CMake SOURCES block. Correct for the two plugins measured here and
  correct for the other 41.

### 4. [Rule 2] A positive control added for `svg-font-attr`'s divergence arm

Not requested by the plan, which specifies only the `undeclared-font` control.
The divergence arm reports 0 on every plugin measured, so without a control it
is indistinguishable from a dead screen — the exact failure the plan's own
"print the count at 0" rule exists to expose. Fired, output above.

## Verification — the plan's seven, each run

| # | Check | Result |
|---|---|---|
| 1 | `node scripts/measure-ui.js` alone prints usage, exits 2 | PASS (exit 2) |
| 2 | No user-home path in the file | PASS (`grep -c '/Users/'` = 0 in both files) |
| 3 | O-AnalogEQ's sibling SVG text nodes report per node, not as 1 | PASS — **29 rows / 1 distinct display id** (corrected from the plan's 24; see deviation 1) |
| 4 | All four screens print a count from a saved JSON via `--from`, no browser | PASS — all four printed from `/tmp/izp-emu-box.json` |
| 5 | `undeclared-font` reads 0 on O-Emulator AND the control reports CJK faces | PASS — 0 findings, 12/12 buttons name a CJK face |
| 6 | README carries 620x430, the discrepancy note, both not-promoted decisions | PASS |
| 7 | HEAD lists exactly the two files | PASS — and so does the union of all three commits |

## Nothing was left out

Playwright resolved on every run (no exit 77 anywhere), both plugins served, and
every task in the plan was executed in full. The two verify commands that did
not run correctly as spelled are recorded above with both spellings and the real
output of each.

## Self-Check: PASSED

Files verified present on disk: `scripts/measure-ui.js`,
`scripts/measure-ui-README.md`, this SUMMARY. Commits verified in
`git log --all`: `7dcc4308`, `59ff5826`, `61382a34`. Working tree clean of
anything outside `.planning/` and the pre-existing
`.gsd/dispatch-isolation-sentinel.json` modification, which this task did not
touch.
