---
phase: quick-260901-h9g
plan: 01
subsystem: i18n
status: complete
tags: [i18n, zh-Hans, localization, O-Chorus, geometry, back-translation, pilot]

requires: [quick-260901-c3s, quick-260901-fr8]
provides:
  - "O-Chorus v1.5.0 shipping en / fr / zh-Hans, all 28 zh entries at reviewed:'bt'"
  - "The five structural answers Stage 3 and the Stage 4 waves inherit: font-tail scope, entity encoding, the line-height offender set, budget arithmetic, per-plugin-gate extension pattern"
  - "A worked, controlled independence protocol for the back-translation (blind ids + fired refusal controls)"
affects:
  - "Stage 3 / Stage 4: 42 remaining plugins copy this pattern"
  - "scripts/i18n-zh-lint.js Z3 set (defect reported, not fixed)"

tech-stack:
  added: []
  patterns:
    - "unitless line-height pin at the measured EN ratio, per named node, never global"
    - "CJK font tail scoped by MEASURED getComputedStyle resolution set, not by [data-i18n] membership"
    - "endonym as numeric character references, so the markup carries no literal non-ASCII"
    - "per-plugin gates derive their language loop from the table's own LANGUAGES and fail rather than defaulting"
    - "back-translation independence via blinded row ids + deliberately fired refusal controls"

key-files:
  created: []
  modified:
    - plugins/O-Chorus/Source/ui/public/js/i18n.js
    - plugins/O-Chorus/Source/ui/public/index.html
    - plugins/O-Chorus/Source/PluginProcessor.h
    - plugins/O-Chorus/Source/PluginProcessor.cpp
    - plugins/O-Chorus/Source/PluginEditor.cpp
    - plugins/O-Chorus/tests/ui_tip_render_check.js
    - plugins/O-Chorus/CMakeLists.txt
    - plugins/O-Chorus/CHANGELOG.md
    - PLUGINS.md
    - research/i18n-zh-hans-localization.md

decisions:
  - "Ship at reviewed:'bt' with NO native Chinese reader — a disclosed quality level, stated in the CHANGELOG and printed by the lint on every run"
  - "Pin line-height per named node at the measured EN ratio; never a global line-height, which would move English geometry"
  - "Pin #lang-select width to its existing English intrinsic (65px) rather than let Chromium size it from the selected option"
  - "Reword around the Z3/像 defect for O-Chorus, and report the defect rather than edit the lint (out of scope)"
  - "Leave tip.language's en/fr bodies naming two of three languages; correcting a reviewed French string needs a French review pass this task does not carry"
  - "ui_tip_render_check assertion 5 gates on the pass DIFFERING from English, not growing — Chinese legitimately shrinks"

metrics:
  duration: "~2h across two dispatches"
  completed: 2026-09-03
  tasks: 3
  commits: 2

actuals:
  tokens: 96000
  tasks: 3
  commits: 2
---

# Phase quick-260901-h9g Plan 01: Stage 2 of the zh-Hans rollout (pilot O-Chorus) Summary

O-Chorus ships v1.5.0 in three languages with all 28 Simplified Chinese entries at
`reviewed: 'bt'`, English geometry unmoved, and every structural answer the remaining 42
plugins need written into the `i18n.js` header and the CHANGELOG rather than left in a
session's context.

## Commits

| Task | Commit | What |
|---|---|---|
| 1 | `4ceb5b1f` | Forward pass — ZH2-01..06, ZH2-08. 28 draft entries, geometry closed, gates extended |
| 3 | `707d3499` | Ingest, promote to `'bt'`, ship v1.5.0 — ZH2-07 |

Task 2 was a `checkpoint:human-action` (`gate="blocking-human"`): the executor wrote the
Chinese and had seen the English, so it was disqualified from producing the reverse pass.
The orchestrator ran it and handed back the result.

## What was built

**ZH2-01 — 28 entries.** `LANGUAGES = ['en','fr','zh-Hans']`; 18 `LABELS` titles and 10
`I18N` title+body pairs. Every English string that is a glossary `TERMS` key took that
term's **root** rendering, settled in Stage 1 across 552 shared strings — *Rate 速率,
Depth 深度, Voices 复音数, Spread 扩散, Width 宽度, Tone 音色, Mix 混合, Drive 驱动,
Load 载入, Save 保存, Language 语言, Settings 设置* and the four preset aria names. `LFO`
ships as the English token keyed `sameAsEn: true` rather than exempted, so a human still
has to agree with it.

The ten bodies mirror the English: at most three sentences, ending with the range and
unit. Full-width punctuation throughout (Z1), **no U+00A0 anywhere** — the deliberate
inverse of the French rules on the same page, where 25 of them live (Z2) — and one plain
U+0020 at every Latin/Han boundary, chosen once and applied table-wide, since the rule is
table-scoped and the *mixture* is the finding (Z4). Units stay ASCII: `ms`, `Hz`, `kHz`,
`%`, `tanh`, `LFO`, `Escape`.

**ZH2-02 — the endonym.** `<option value="zh-Hans">&#31616;&#20307;&#20013;&#25991;</option>`,
written as numeric references to match the existing `Fran&ccedil;ais` convention. This
would otherwise have been the first literal non-ASCII byte the HTML ever carried;
entities sidestep the `<meta charset>` prescan question outright. The `I18N_EXEMPT` entry
carries the **decoded** text, because the parser resolves the references before
assertion 10's uncovered-text sweep runs.

**ZH2-03 — the CJK font tail, on 5 of 8 stacks.** The resolution set was **measured** —
the page served, switched to Chinese, `getComputedStyle().fontFamily` read on every node
that holds or can receive a Han codepoint — not derived from the `[data-i18n]` list.
That distinction was load-bearing: **two of the five carry no keyed attribute at all.**

| Stack | Took the tail? | Why |
|---|---|---|
| `.container` | yes | the inherited stack: `.knob-label`, `.lfo-ring-label`, `.title`, `.knob-container` |
| `.preset-action` | yes | the LOAD / SAVE captions (载入 / 保存) |
| `.settings-label` | yes | the LANGUAGE caption (语言) |
| `.settings-select` | yes | the endonym `<option>` — the only Han in the **markup**, never `[data-i18n]` |
| `.tooltip` | yes | filled from `data-tip` at **hover time**; never `[data-i18n]`. A keyed-node scan would have left every Chinese tooltip on a system face |
| `.preset-nav` | no | the glyphs U+25C0 / U+25B6; their names live in `aria-label`, which is never rendered text |
| `.preset-dropdown-item` | no | preset names are the JSON filenames on disk (D-02) — ASCII by contract, localizing one would orphan the file |
| `#gear-btn` | no | the single gear glyph U+2699; its tip paints in `#tooltip`, not in the button |

Latin still resolves to Garamond first, so English geometry is unmoved — proved by the EN
arm of assertion 7, not asserted.

**ZH2-04 — the C++ codec.** Three branches, pure ASCII, storing the BCP-47 tag so one
spelling crosses the whole boundary. The three now-false "anything that is not `fr`"
comments were repaired in `PluginProcessor.h`, `PluginProcessor.cpp` and
`PluginEditor.cpp`. Zero Han under `Source/**/*.{h,cpp}`.

**ZH2-05 — the line-height audit.** `line-height: normal` is the font's own metrics, and
Han faces carry taller ones: **26 elements moved** on the first Chinese run. Three
unitless pins at the measured EN ratio **10.00 px / 9 px = 1.1111**:

| Node | EN → ZH at `normal` | Named by assertion 7? |
|---|---|---|
| `.knob-label` | 10.00 → 13.00 px | not directly — it is a `[data-i18n]` label, so it is outside the "others" set. Its growth is what moved 24 of the 26 (`.group`, 8× `.knob-container`, 8× `.knob`, 8× `.knob-value`) |
| `.preset-action` | 14.00 → 17.00 px border-box | yes — `#preset-load`, `#preset-save`, dy=−1.5 dh=+3.0 |
| `.settings-label` | 10.00 → 13.00 px | **no, and it could not be** — the popover is `hidden` at rest, so the gate never measures it |

`.settings-label` is a Rule 2 addition. It happens not to propagate today only because
`.settings-select` is 16 px and taller than both — luck, not design, since the row is
`space-between` with a `nowrap` caption. No global `line-height` was added.

**The 26th mover was not a line height, and this is the finding Stage 3 most needs.**
`#lang-select` measured 65 px in English and French and 64 px in Chinese, walking 1 px
right in a `space-between` row. The three endonyms are language-**invariant** (they are
never translated) and measure 27.501 / 30.489 / 36.792 px in every pass, so the widest
*option* cannot explain it. With `appearance: auto` Chromium derives the control's
intrinsic width from the **selected** option's font run, and the Han endonym resolves
through PingFang SC a pixel narrower. Pinned to 65 px — its existing English intrinsic —
so English and French are byte-unchanged and only the Chinese pass moves.

**ZH2-06 — budgets.** The three Stage-1 cells are intact and untouched (`depth` 6,
`save` 6, `spread` 5); all five budgeted renderings fit at 2 characters. **No new cell
was added, and the measurement is the point:** every Chinese caption is *narrower* than
its English original, by 0.3 to 19.7 px in the real `.knob-label` node. The exact inverse
of French, where three of eight had to be abbreviated. Chinese buys width and spends
height. `scripts/i18n-zh-glossary.js` was therefore not modified.

**ZH2-08 — the per-plugin tooltip frame gate.** `ui_tip_render_check.js` now derives its
language loop from the table's own `LANGUAGES` and fails rather than falling back to a
default pair. Re-run unchanged it would have passed **vacuously** for Chinese — and it is
the only gate that measures a tooltip against the 125 px frame.

**ZH2-07 — ship.** `VERSION 1.5.0`, `## [1.5.0] - 2026-09-03`, PLUGINS.md row 47 at
1.5.0 / 2026-09-03 with no duplicate row, built and installed through
`build-and-install.sh` (dual-variant sweep), `auval` clean.

## The back-translation, and what made it independent

`--emit` withholds the English from the batch by design. On top of that the orchestrator
**blinded the row ids** to `r01..r38` before the reverse agent saw the file — an id like
`label.depth` leaks the very English word the pass is meant to recover — and rejoined the
real ids afterwards. The reverse pass was a different model in a fresh session, forbidden
to open any other file.

- forward: `claude-opus-5 gsd-executor forward draft, 2026-09-03`
- reverse: `claude-sonnet-5 independent reverse pass, blind row ids r01-r38 rejoined by orchestrator, 2026-09-03`

**The refusal controls were fired deliberately.** `--ingest` refuses a provenance that is
missing or byte-identical to the forward one; a refusal that never fires proves nothing,
so both violation shapes were run first and both produced
`REFUSED: back-translation provenance is missing or identical to the forward pass`. The
real run then produced none. 38 rows joined, 0 orphans.

**38 triples read: 24 exact, 14 accepted as synonyms, 0 corrected.** No triple said
anything its English did not, so no Chinese string was changed and all 28 entries were
promoted in one pass. Accepted drifts, each with its reason:

| Triple | Score | Why it is not an error |
|---|---|---|
| Voices → 复音数 → "Voice Count" | **0.00**, the lowest of all 38 | 复音数 states the count sense the English leaves implicit; the parameter is literally an integer 1 to 8. Also the glossary root — changing it would trip Z5. **The lowest score in the set is correct, which is the clearest possible demonstration that the score is a sort key and not a verdict.** |
| Spread → 扩散 → "Spread [or: Diffusion]" | 0.67 | glossary root; 扩散 covers both senses in audio Chinese. Nothing collides here — but a reverb with both Spread *and* Diffusion would have two English controls competing for one Chinese word. Flagged for Stage 3. |
| Tone → 音色 → "Tone [or: Timbre]" | 0.67 | glossary root; 音色 is literally *timbre* and this control is a brightness tilt. No separate Timbre control exists here. Same Stage-3 flag. |
| pan position → 声场位置 → "position in the sound field"; stereo image → 立体声场 → "stereo field" | 0.78 / 0.60 | **both are consequences of the 像 defect below, not free choices.** The natural renderings (声像 / 立体声像) are what the glossary settles for `pan`, and Z3 flags them. Recorded so the next reader knows these two strings were *steered* by a tooling defect. |
| "high values sing" → "sing out with noticeable warble"; "drift and widen" → "widens the sound field" | 0.65 / 0.85 | explicitations. Chinese does not carry an objectless "widen" comfortably, and "sing" is a term of art the surrounding clause already defines by contrast. Neither adds a claim. |
| "bucket-brigade chorus" → "bucket-brigade delay circuit" | 0.61 | a BBD chorus *is* built on a bucket-brigade delay line — the Chinese is the more precise of the two. |
| mix, spread, tone, settings bodies | 0.34–0.79 | sentence shape only, no change of control, range or unit. |

## Verification

| Gate | Result |
|---|---|
| `check-i18n --plugin O-Chorus` | exit 0, ALL CHECKS PASS, languages `en, fr, zh-Hans` |
| `check-i18n` (all) | exit 0, **ALL CHECKS PASS 43/43** — one plugin's three-language `LANGUAGES` did not disturb the byte-compared canon in the other 42 |
| `check-ui-labels --plugin O-Chorus` | exit 0, **0 FAIL**, PASS on `[7][GEOMETRY DIFF][fr]` and `[7][GEOMETRY DIFF][zh-Hans]`, zh vacuity 11/12 = **92%** (needs ≥25%) |
| `ui_tip_render_check.js` | exit 0, **303 passed**, 1 `zh-Hans` pass and 2 `en` passes (initial + return) |
| `i18n-zh-lint --plugin O-Chorus` | exit 0, **0 findings across all nine rules including R1**, 0 entries below the ship bar |
| `boot-all-uis --strict-tips` | exit 0, **0 DEAD** bindings |
| `i18n-fr-lint` | exit 0 — French untouched |
| Han gate over `Source/**/*.{h,cpp}` | **empty**, with `HAN_CONTROL` firing on `i18n.js` |
| `PluginProcessor.h` tag literals | exactly 2 (both codec halves) |
| `auval -v aufx OuCh OuDv` | **AU VALIDATION SUCCEEDED** |
| PLUGINS.md duplicate rows | empty |

Every negative gate carries a positive control that fired: the Han gate is paired with
`HAN_CONTROL`, the `check-ui-labels` zh arm with its 92% vacuity fraction, the tooltip
gate with an explicit grep for a `zh-Hans` pass, the ship-bar grep is comment-stripped,
and the provenance refusal was fired on both violation shapes.

## Deviations from plan

### Auto-fixed

**1. [Rule 1 — Bug] A second hard-coded language pair inside `ui_tip_render_check.js`.**
C-4 named `:382` and `:386`. Below them, tip heights were recorded only while
`drivenStates.length <= 2` — "the first two passes", which is `en` and `fr` and nothing
else. The Chinese pass drove, rendered and asserted correctly while recording **nothing**,
and assertion 5 then read `undefined`. Now keyed per language (record on each language's
first pass, skip only the return pass). Commit `4ceb5b1f`.

**2. [Rule 1 — Bug] Assertion 5 stated its discriminator in a direction that fails a
correct table.** It asserted "French **GROWS** at least one tip's height". French wraps to
more lines and grows; Chinese says the same thing in fewer characters and **shrinks** —
measured 6 tips shrank, 0 grew. A growth assertion would have failed a correct Chinese
table while adding nothing to the French claim. What makes a pass non-vacuous is that it
measured *different* boxes at all, so the assertion now gates on **differing** and reports
the direction. Commit `4ceb5b1f`.

**3. [Rule 2 — Missing critical functionality] `.settings-label` line-height pin.** Not
named by assertion 7 and unnameable by it (the popover is `hidden` at rest). Measured with
the popover forced open it is the identical 10.00 → 13.00 px defect. Pinned on the same
ratio. Commit `4ceb5b1f`.

**4. [Rule 1 — Bug] `#lang-select` width.** The one assertion-7 mover no line-height pin
could close; diagnosed by measurement rather than assumption and fixed with a width pin at
the existing English intrinsic. Commit `4ceb5b1f`.

**5. [Rule 3 — Blocking] `--ingest` needed an explicit `--manifest`.** The tool resolves
the manifest as `<ingest-file>.manifest.json`, i.e. beside the **ingest** file, not beside
the emitted batch. The first ingest therefore printed
`forward pass recorded at emit: (none recorded — the manifest was not found beside the
batch)` — meaning **the entire forward-provenance independence check (T-h9g-02's whole
mitigation) ran inert while still producing a clean-looking report.** Re-run with
`--manifest` pointing at the batch's manifest, then both refusal shapes fired as controls.
This is a live trap for Stage 3, which will run the same loop 42 more times.

### Two plan gate commands that returned non-zero without a defect behind them

Neither is a content failure; both are grep breadth. Verified substantively rather than
reinterpreted, and recorded here so Stage 3 inherits the correct discriminators.

1. `grep -cE '^\s*(Z1|…|F1|R1)\b'` returns **1**, not 0. The match is the lint's
   unconditional summary line `Z6 coverage: 3 of 552 glossary terms…`, which is a
   *disclosure*, not a finding — it printed identically in Stage 1 at zero zh entries.
   Correct discriminators: the per-finding detail section (empty) and the TOTAL row's last
   column (0).
2. `grep -ci 'BELOW SHIP BAR'` returns **1**, not 0. The lint prints
   `BELOW SHIP BAR — entries at reviewed:'mt' (machine draft, unchecked): 0`
   unconditionally; the *count* is 0, which is the substantive expectation. Correct
   discriminator: the number after the colon.
3. On the emitted batch, `grep -ci 'depth\|profondeur'` returns **3**, not 0. All three are
   in the **id** column (`label.depth`, `tip.depth` ×2) — the join key `--ingest` requires.
   `cut -f2 | grep -ci` returns 0; the only Latin in the payload is `LFO ms kHz Hz tanh
   Escape` plus the two endonyms, with zero 3-word Latin runs. The English source is
   genuinely withheld.

## Known issues — reported, not fixed

**1. The lint's Traditional-only set and the glossary contradict each other on 像
(U+50CF).** Z3's set is derived from OpenCC as `keys(TSCharacters) \ keys(STCharacters)`,
and that difference **contains 像** — a standard simplified character, and the one the
glossary's own root rendering for `pan` uses (声像). So **rule Z5 will *require* a
rendering that rule Z3 then *flags*** on any plugin with a Pan control. Two drafts here
tripped it and were reworded to 声场 / 立体声场, which is legitimate for a chorus but is a
route around the defect, not a fix — a plugin with a Pan knob cannot reword past it. Not
fixed because scope permitted only `BUDGETS` under `scripts/`. **This will block Stage 3
on the first plugin with a Pan control.**

**2. `tip.language`'s English and French bodies name two of three languages.** Both end
"English or Français" / "English ou Français", authored when the selector had two options.
The Chinese body names all three, and the back-translation surfaced the mismatch cleanly.
Correcting the other two means editing a French string a human has already signed off
(`reviewed: true`), which needs a French review pass this task does not carry.

**3. Risk A5 is open and disclosed, not dropped.** The `line-height: normal` +30% figure
was reconfirmed in Chromium on the real page (10.00 → 13.00 px at 9 px, exactly the
research table's row), but the **WKWebView re-measurement was not taken because it cannot
be**: this plugin's bridge exposes twelve native functions (ten preset, `getUiLanguage`,
`setUiLanguage`) and no `evaluateJavascript` path, so reading a computed style from inside
the host would mean shipping a debug hook in a release build. Recorded in
`research/i18n-zh-hans-localization.md` §3.4 and its A5 risk row.

**4. No native Chinese reader.** All 28 entries are at `'bt'` — drafted, then independently
back-translated and read against the English. Nobody who reads Chinese as a first language
has seen any of it. Disclosed in the CHANGELOG and printed by the lint on every run.

**5. The ≤9 px legibility tier.** Five localized nodes render at 9 px, where Han is at the
legibility floor. The suite ships at parity size because raising the Chinese size would
move English geometry and break the zero-shift guarantee the gates enforce. Disclosed in
the CHANGELOG.

## What Stage 3 inherits

1. **Font-tail scope is a measured set, not a reasoned one** — and two of O-Chorus's five
   carry no `[data-i18n]`, so a keyed-node scan silently under-covers.
2. **Entity encoding for the endonym**, with the `I18N_EXEMPT` entry carrying decoded text.
3. **The line-height offender set is "everything inheriting `normal` from the UA"**, and
   the fix is a per-node unitless pin at the measured EN ratio — never a global one. Nodes
   inside a `hidden` popover need pinning too and the gate cannot name them.
4. **A `<select>` in a `space-between` row will move**, and it is a width problem, not a
   line-height one.
5. **Budget arithmetic runs the opposite way from French** — Chinese captions shrink.
6. **Per-plugin gates must derive their language loop from `LANGUAGES` and fail rather than
   default**, and must be checked for *second* hard-coded pairs below the obvious one.
   Assertion wording that demands growth will fail a correct Chinese table.
7. **`--ingest` needs an explicit `--manifest`**, or the independence check runs inert.
8. **The 像 defect blocks any plugin with a Pan control** until Z3's set is corrected.

## Self-Check: PASSED

All modified files verified present on disk; both commit hashes verified in `git log`;
`git show --stat` on each confirms only this plan's files landed. No stubs, no skipped
tests, no unrun `<verify>` commands.
