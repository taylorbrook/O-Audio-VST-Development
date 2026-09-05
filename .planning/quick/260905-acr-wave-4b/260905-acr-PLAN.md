---
phase: quick-260905-acr
plan: 01
type: execute
wave: 1
depends_on: []
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4b, O-IntonationPad, O-DigiDelay, O-AnalogEQ, O-Tremolo, O-SimpleReverb, O-Emulator, gates, geometry, lint-flip]
autonomous: true
requirements: [ZH4B-01, ZH4B-02, ZH4B-03, ZH4B-04, ZH4B-05, ZH4B-06, ZH4B-07, ZH4B-08, ZH4B-09, ZH4B-10, ZH4B-11]
files_modified:
  - scripts/i18n-zh-lint.js
  - plugins/O-Tremolo/Source/ui/public/js/i18n.js
  - plugins/O-Tremolo/Source/ui/public/index.html
  - plugins/O-Tremolo/Source/PluginProcessor.h
  - plugins/O-Tremolo/tests/ui_tip_render_check.js
  - plugins/O-Tremolo/CMakeLists.txt
  - plugins/O-Tremolo/CHANGELOG.md
  - plugins/O-IntonationPad/Source/ui/public/js/i18n.js
  - plugins/O-IntonationPad/Source/ui/public/index.html
  - plugins/O-IntonationPad/Source/ui/public/css/tuning-panel.css
  - plugins/O-IntonationPad/Source/ui/public/js/app.js
  - plugins/O-IntonationPad/Source/ui/public/js/tuning-panel.js
  - plugins/O-IntonationPad/Source/PluginProcessor.h
  - plugins/O-IntonationPad/CMakeLists.txt
  - plugins/O-IntonationPad/CHANGELOG.md
  - plugins/O-DigiDelay/Source/ui/public/js/i18n.js
  - plugins/O-DigiDelay/Source/ui/public/index.html
  - plugins/O-DigiDelay/Source/PluginProcessor.h
  - plugins/O-DigiDelay/tests/ui_tip_render_check.js
  - plugins/O-DigiDelay/CMakeLists.txt
  - plugins/O-DigiDelay/CHANGELOG.md
  - plugins/O-AnalogEQ/Source/ui/public/js/i18n.js
  - plugins/O-AnalogEQ/Source/ui/public/index.html
  - plugins/O-AnalogEQ/Source/PluginProcessor.h
  - plugins/O-AnalogEQ/tests/ui_tip_render_check.js
  - plugins/O-AnalogEQ/CMakeLists.txt
  - plugins/O-AnalogEQ/CHANGELOG.md
  - plugins/O-SimpleReverb/Source/ui/public/js/i18n.js
  - plugins/O-SimpleReverb/Source/ui/public/index.html
  - plugins/O-SimpleReverb/Source/PluginProcessor.h
  - plugins/O-SimpleReverb/tests/ui_tip_render_check.js
  - plugins/O-SimpleReverb/CMakeLists.txt
  - plugins/O-SimpleReverb/CHANGELOG.md
  - plugins/O-Emulator/Source/ui/public/js/i18n.js
  - plugins/O-Emulator/Source/ui/public/index.html
  - plugins/O-Emulator/Source/PluginProcessor.h
  - plugins/O-Emulator/tests/ui_tip_render_check.js
  - plugins/O-Emulator/CMakeLists.txt
  - plugins/O-Emulator/CHANGELOG.md
  - PLUGINS.md

estimate:
  tokens: 365000
  raw_tokens: 430000
  tasks: 3
  confidence: low       # two prior samples of this shape: Stage 3 610k / 758 rows, wave 4a 367k / 580 rows.
                        # raw = 492 rows x ~700 tok/row + the structural work wave 4a did not carry
                        # (5 gate repairs, 71 bare-generic stacks, the late-tips fix, the lint flip).
                        # factor 0.85 — wave 4a measured actual/raw at 0.59, damped because this
                        # wave's non-row work is larger. Two samples means low, never self-rated.

must_haves:
  truths:
    - "A user opens any of the six plugins, picks 简体中文 from the language selector, and every caption, section heading and hover-help body on the page renders in Simplified Chinese."
    - "Switching back to English or French reproduces the pre-change page in geometry — no element moved on the en or fr arm, on any of the six."
    - "The Chinese page has no clipped, wrapped or displaced element: check-ui-labels reports 0 FAIL on the zh arm of all six."
    - "No ASCII text on any of the six pages changes face when the page language changes — every font stack names a typeface that exists on the build machine before it reaches a generic."
    - "No Chinese character exists anywhere under any plugin's Source/**/*.{h,cpp} — the C++ carries only the ASCII language code."
    - "Every zh row has been read back through a blind reverse pass and either accepted with a written reason or re-authored."
    - "Every one of O-IntonationPad's tuning-panel tips carries a tip at settle — boot-all-uis reports 0 late and 0 dead bindings across the whole suite."
    - "i18n-zh-lint blocks: a Z-rule violation anywhere in the corpus exits 2, and that was proven by planting one."
    - "All six load in Logic: auval PASS on all six triples after one cold registry rescan."
  artifacts:
    - scripts/i18n-zh-lint.js                                       # exit 2 on any finding, controls fired
    - plugins/O-Tremolo/Source/ui/public/js/i18n.js                 # zh-Hans on every I18N and LABELS key
    - plugins/O-IntonationPad/Source/ui/public/js/i18n.js
    - plugins/O-DigiDelay/Source/ui/public/js/i18n.js
    - plugins/O-AnalogEQ/Source/ui/public/js/i18n.js
    - plugins/O-SimpleReverb/Source/ui/public/js/i18n.js
    - plugins/O-Emulator/Source/ui/public/js/i18n.js
    - plugins/O-Tremolo/tests/ui_tip_render_check.js                # language list derived, direction assertion rewritten
    - plugins/O-DigiDelay/tests/ui_tip_render_check.js              # same
    - plugins/O-AnalogEQ/tests/ui_tip_render_check.js               # same
    - plugins/O-SimpleReverb/tests/ui_tip_render_check.js           # language list derived
    - plugins/O-Emulator/tests/ui_tip_render_check.js               # language list derived
    - plugins/O-IntonationPad/Source/ui/public/js/tuning-panel.js   # the tip re-sweep that closes 17 late bindings
    - PLUGINS.md                                                    # six registry rows, corrected from CMakeLists
  key_links:
    - "LANGUAGES in i18n.js <-> the endonym <option> in index.html <-> languageCode/languageIndex in PluginProcessor.h — a third language present in two of the three renders a page that cannot be reached or cannot be persisted."
    - "The CJK tail's POSITION relative to the trailing generic <-> whether it is consulted at all — a tail written after a bare generic is dead code under a Chinese document language, and the page still looks right on macOS because the generic supplies a Chinese face."
    - "A font stack's first INSTALLED family <-> the metrics of the ASCII it renders — a stack whose only named families are absent resolves Latin through the document language too, so the en/fr/zh arms disagree on nodes holding no translated string."
    - "The gate file's language list <-> the table's own LANGUAGES export — a gate that names its languages cannot see a third arrive, whichever syntax it uses, and the census grep only finds two of the five syntaxes present in this wave."
    - "The direction of a tip-height assertion <-> the language it is asserted against — 'the non-English pass grew' is true for French and false for Chinese, so a direction-specific assertion hard-fails the zh arm on a correct page."
    - "The blind reverse batch's manifest <-> the ids in the returned file — a manifest resolved from the wrong side joins nothing and the read is silently vacuous."
    - "tuning-panel.js's panel injection <-> setupTooltips()'s first sweep — 17 bindings resolve after the sweep, so the very nodes this wave translates carry no tip at all."
---

<objective>
Localize six plugins into Simplified Chinese — O-Tremolo, O-IntonationPad, O-DigiDelay,
O-AnalogEQ, O-SimpleReverb, O-Emulator — to the ship bar Stages 2–3 and wave 4a established,
and ship each with a version bump, a build, an install and an auval pass. Flip
`i18n-zh-lint` from report-only to a gate first, so the 492 rows this wave writes are the
first rows in the rollout that are authored under a blocking lint.

Purpose: wave 4b is the second of seven volume waves. Wave 4a proved the pattern and left
thirteen structural findings; this wave executes it and closes the three carry-forward items
wave 4a explicitly deferred into it (the lint flip, O-IntonationPad's late tip bindings, and
the assumption that the two-language gate census under-counts).

Output: six three-language plugins, one lint-flip commit, one path-scoped commit stream per
plugin, one PLUGINS.md commit at the end of the batch, one cold auval sweep covering all six.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@.planning/quick/260904-qrc-stage-4-wave-4a-of-the-zh-hans-rollout-o/260904-qrc-SUMMARY.md
@.planning/quick/260904-g5l-stage-3-of-the-zh-hans-rollout-the-hard-/260904-g5l-SUMMARY.md
@.planning/quick/260904-qrc-stage-4-wave-4a-of-the-zh-hans-rollout-o/deferred-items.md
@scripts/i18n-zh-glossary.js
@scripts/i18n-zh-lint.js
@scripts/i18n-zh-backtranslate.js
</context>

---

## Live observations — measured on the working tree at planning time (2026-09-05)

Everything below was read off the tree, not carried forward from wave 4a's assumptions. Wave
4a's own plan asserted "O-Prism is an instrument, the other five are effects" and was wrong
about O-Texture; that is the argument for reading rather than trusting.

| | O-Tremolo | O-IntonationPad | O-DigiDelay | O-AnalogEQ | O-SimpleReverb | O-Emulator |
|---|---|---|---|---|---|---|
| entries (I18N + LABELS) | 34 (9+25) | **199** (79+120) | 36 (10+26) | 35 (14+21) | 32 (11+21) | 27 (8+19) |
| **emitter rows** (measured) | 43 | **276** | 46 | 49 | 43 | 35 |
| UI root | `Source/ui/public/` | same | same | same | same | same |
| external CSS | none | **`css/tuning-panel.css`** | none | none | none | none |
| controller files | **none** (inline module) | **`app.js` + `tuning-panel.js` + `constants.js`** | **none** | **none** | **none** | **none** |
| CSS `font-family:` decls | 12 | 23 (**21 markup + 2 CSS**) | 9 | 15 | 11 | 9 |
| **SVG `font-family=` attrs** | 0 | 0 | 0 | **24** | 0 | 0 |
| **bare-generic stacks** | **6** | **19** | **0** | **39** | **5** | **0** |
| type token | none | none | none | none | none | **`--serif`** (1 edit ⇒ 8 decls) |
| canvas `fillText` / `.font=` | **0** | **0** | **0** | **0** | **0** | **0** |
| existing min-w / min-h / line-h | 4 / 0 / 4 | 7 / 3 / 3 | 5 / 0 / 4 | 5 / 0 / 5 | 6 / 0 / 5 | 3 / 2 / 6 |
| `tests/i18n-states.json` states | 2 | **19** | 2 | 2 | 2 | 2 |
| gate file | `ui_tip_render_check.js` | **NONE** | `ui_tip_render_check.js` | `ui_tip_render_check.js` | `ui_tip_render_check.js` | `ui_tip_render_check.js` |
| how it names its languages | 3-element loop — **off census** | — | 3-element loop — **off census** | 3-element loop — **off census** | joined pair — on census | joined pair — on census |
| direction-specific `[5]` assertion | **YES** | — | **YES** | **YES** | no | no |
| `CMakeLists` VERSION | 1.9.1 (L13) | **"2.9.2" — QUOTED** (L9) | 1.5.1 (L12) | 1.4.1 (L12) | 1.8.1 (L8) | 1.3.1 (L15) |
| shipping version | **1.10.0** | **2.10.0** | **1.6.0** | **1.5.0** | **1.9.0** | **1.4.0** |
| `juce_add_plugin` target | `OuariconTremolo` | `O-IntonationPad` | `OuariconDigitalDelay` | `OuariconAnalogEQ` | `O-SimpleReverb` | `OEmulator` |
| PLUGIN_CODE | `OuTr` | `OuIP` | `OuDD` | `OuAE` | `OuSr` | `OEmu` |
| IS_SYNTH | false → `aufx` | **TRUE → `aumu`** | false | false | false | FALSE |
| `PLUGINS.md` row as found | 1.8.2 | 2.9.1 | 1.4.1 | 1.3.1 | 1.7.2 | 1.2.1 |
| stale LANGUAGE enumeration | HIT (en+fr) | HIT (en+fr) | HIT | HIT | HIT (**counts them**) | HIT |
| stale GEAR/SETTINGS enumeration | **HIT** | correct — names both | **HIT** | **HIT** | **HIT** | **HIT** |
| late tip bindings (measured) | 0 | **17, live** | 0 | 0 | 0 | 0 |
| installed bundle | `O-Tremolo-dev` | `O-IntonationPad-dev` | `O-DigiDelay-dev` | `O-AnalogEQ-dev` | `O-SimpleReverb-dev` | `O-Emulator-dev` |

**Live total: 363 entries, 492 emitter rows.** The corpus goes 1338 → 1830 rows, 10 → 16
localized plugins. All six are `Source/ui/public/`; all six are installed as `-dev` only, and
the alternate-variant sweep found no orphan.

### Baseline, measured now — every one of these is the number the wave must not regress

- `check-i18n` repo-wide: **`ALL CHECKS PASS — 43 localized plugin(s)`**, including the new
  assertion **[16]** (every language selector carries exactly one hover-help switch, bound and keyed).
- `i18n-zh-lint` repo-wide: **1034 entries checked, 0 findings, 0 / 43 plugins**, `BELOW SHIP BAR 0`.
- `i18n-zh-lint --self-test`: **10/10**.
- `i18n-zh-backtranslate` stage view: **1338 rows, 1338 at `bt`, 0 at `mt`**.
- `i18n-fr-lint`: exit 0.
- `boot-all-uis --plugin O-IntonationPad --strict-tips`: **late-tips = 17, DEAD 0, failed 0**.
- git: on `main`, one worktree, the only modified path is `.gsd/dispatch-isolation-sentinel.json`
  (predates this work — **never stage it**).

---

## Thirteen findings measured at planning time that wave 4a's summary does not contain

Wave 4a's findings 1–13 all apply and are carried into the tasks below. These are **new**, and
each one costs the executor time or a wrong answer if not read first.

### N1. SVG `font-family=` ATTRIBUTES are a fourth font carrier, and O-AnalogEQ has 24 of them

O-AnalogEQ's four dial faces draw their frequency ticks as `<text>` inside inline SVG, each
carrying a `font-family=` **presentation attribute** — not a CSS declaration. A sweep that
greps for the property-and-colon form finds **15 of 39** declarations on that plugin. All 24
attributes are the bare-generic shape and all 24 hold ASCII (`30`, `124`, `1.2k`, `20k`).
`getComputedStyle` does reach SVG `<text>`, so a computed-style measurement finds them —
which is the argument for measuring rather than grepping either file.

### N2. A type token can be named for the TYPEFACE ROLE, not for the word "font"

O-Emulator resolves its whole page through `var(--serif)`. Wave 4a's token screen greps for a
custom property whose name contains `font`, returns zero, and concludes "literal-stack shape,
no token shortcut" — which on this plugin is **false**. O-Emulator is the O-Octagon token
case and takes **one edit covering eight declarations**. Its token already names
`'Times New Roman'` before the generic, so its Latin is safe and only the tail is needed.

### N3. Garamond is absent on this machine — measured — and 71 declarations depend on it

`system_profiler SPFontsDataType` reports **0 family matches** for `Garamond`,
`EB Garamond` and `Adobe Garamond Pro`, and installed families for `Times New Roman` (4),
`PingFang SC` (6) and `Songti SC` (4). So wave 4a's finding 2 is not a two-site curiosity
here — it is **69 declarations across four plugins**:

| plugin | bare-generic decls | what they hold |
|---|---|---|
| O-AnalogEQ | **39** (15 CSS + 24 SVG attrs) | the entire page, including every dial tick |
| O-IntonationPad | **19** (18 `'Garamond', serif` in the markup + 1 bare `monospace` at `tuning-panel.css:317`) | captions across 19 states |
| O-Tremolo | 6 | half the page; the other 6 stacks name Times New Roman and are safe |
| O-SimpleReverb | 5 | same split, 6 safe |

Every one of these renders **Latin** through the document language's generic. Under
`zh-Hans` that is Songti SC, so the ASCII on those nodes changes metrics with no translated
string anywhere near it. **On O-AnalogEQ this affects the whole page.** The fix is to name the
face — `'Times New Roman'` — and it is invisible to every gate until a third language arrives.

### N4. THREE gate files carry the direction-specific assertion, not one

Wave 4a hit this once, on O-Prism. Here it is on **O-Tremolo, O-DigiDelay and O-AnalogEQ**:
each has an assertion `[5]` that requires the non-English pass to be strictly taller than
English on at least one tip, built from two height maps and a strict greater-than. Chinese
shrinks tip height (wave 4a measured O-Prism: en 123.9, fr 139.3, **zh 108.5**), so all three
hard-fail on the zh arm on a page where nothing is wrong. Rewrite to assert a **difference**:
what the gate needs is that the non-English pass measured something else.

### N5. Five of five gate files name their languages; the census sees only two

| plugin | syntax | on the Stage-3 census? |
|---|---|---|
| O-SimpleReverb | joins the array and compares to a pair string | yes |
| O-Emulator | same | yes |
| **O-DigiDelay** | iterates a three-element array literal | **no** |
| **O-AnalogEQ** | same | **no** |
| **O-Tremolo** | same | **no** |

Wave 4a predicted exactly this and the prediction holds at a higher rate — 3 of 5 invisible
here against 2 of 4 there. **Read every gate file. The census is not the inventory.**

### N6. O-IntonationPad has NO gate file at all — 276 rows, 19 states, 80 tip bindings

The largest table in the wave is the one plugin with no `tests/ui_tip_render_check.js`. Its
tips are covered only by `boot-all-uis` (a report) and `check-ui-labels` (geometry). There is
nothing to repair and nothing to lean on; the state file and the geometry gate are the whole
instrument. Do not write a new gate for it — that is a separate pass with its own budget.

### N7. All 17 late tip bindings live in the TUNING tab, and they are this wave's own copy

Measured, live: `#interval-list`, the five `.viz-btn[data-mode=…]`, `#library-section`,
`#ref-pitch-knob`, `#scale-name-display`, `#octave-stretch`, `#pitch-bend-range`, the five
`#btn-load/save/export` buttons, `#generator-section`. Every one is injected by
`js/tuning-panel.js` **after** the first tip sweep, so the binding misses. These are exactly
the nodes whose captions this wave translates — a late binding means the tip never carries
its Chinese either. This is in scope, and the fix is a re-sweep after injection, not a
selector change.

### N8. O-IntonationPad's CMakeLists VERSION is QUOTED

`VERSION "2.9.2"`. This is the R9 trap that made a Stage-3 verify grep report "not done" on a
bump that had landed. **Read the value; never grep for a bare literal.** Its shipping version
is **2.10.0** — a minor bump off `2.9`, not `2.9.3`.

### N9. Canvas text: zero across all six, with the control fired

`fillText` and canvas `.font =` return **0 on every one of the six**, and the same grep run
against O-Comp returns its two known captions. Wave 4a's finding 5 is an evidenced
non-finding here, not an unchecked one.

### N10. The gear/settings enumeration is false on FIVE of six — and correct on the sixth

O-DigiDelay ("That is all it holds"), O-AnalogEQ ("It holds one control"), O-Tremolo
("nothing else"), O-SimpleReverb ("Nothing else lives in there"), O-Emulator ("That is all it
holds") — every one false, because every one of the six carries a hover-help switch in its
markup. **O-IntonationPad's names both choices and is correct**: the first non-defective one
in three stages. So finding 13's guidance is *check both*, not *assume both*.

### N11. The language body on O-SimpleReverb states a COUNT

"Two settings: English and Français." The other five name a pair; this one numbers it. Same
fix — remove the enumeration, do not extend it — but it is the form that reads most obviously
wrong once a third option is in the selector.

### N12. Five of six have no controller file — the copy renderer is an inline module

Only O-IntonationPad has `app.js`. The other five carry just `i18n.js` and `juce/`, so
`applyI18n` and `setupTooltips` live in an inline `<script type="module">` in `index.html`
(the O-Bitrot shape). `check-i18n` assertion 6 passes on that layout and names it explicitly.
An executor looking for `js/app.js` to wire the third language will not find one on five of six.

### N13. The unquoted `--include` glob trap fired during planning, and read as a clean zero

`grep -r … --include=*.html` under zsh expands the glob against the *current directory*, the
whole grep fails with `no matches found`, and the count comes back **0** — indistinguishable
from a genuine non-finding. It fired on the canvas-text sweep here before being caught by its
positive control. **Quote every `--include` pattern.** `timeout` does not exist on this
machine either.

---

## The lint gate flip — DECISION: MAKE IT, in its own commit, before any authoring

The constraint asks for an explicit decision. **Wave 4b flips it.** Wave 4a deferred it as
"not this task's to make"; it is this task's to make, and the reasoning is stateable rather
than a matter of turn-taking.

**The promotion criterion the tool sets for itself is met, and was measured today:** its own
header says it becomes a gate "once the O-Chorus pilot is at zero findings"; the repo reads
**0 findings across 43 plugins, 1034 entries**, and `--self-test` reads **10/10**.

**The one thing that could have made the flip incompatible with this wave does not hold.**
Authoring happens at `reviewed: 'mt'`, and `'mt'` is routed to the informational bucket, not
to `findings` — so a flipped gate reports `BELOW SHIP BAR: N` and still exits 0 while the
tables are being drafted. Verified in the source, not assumed.

**Why now rather than after the wave:** wave 4a's finding 10 is that Z8 caught ten live
violations introduced *in the same session that shipped the rule*, and it caught them only
because the executor chose to run a report-only tool. Five more waves follow this one. A
report that must be remembered is not a gate.

**Why in its own commit, before the authoring:** so the result is attributable. After the
flip and before any row is written, the tree is at 0 findings — an exit 0 there is the flip
behaving. An exit 2 later is the rows. Making the flip inside a 492-row commit makes those
two indistinguishable, which is the failure shape this rollout keeps hitting from the other
direction.

**Controls, both fired, in Task 1 before O-Tremolo is touched:**
- **Negative** — flip, run repo-wide against the tree as found: still **exit 0**.
- **Positive** — plant one intra-Han space in a committed zh row, run: **exit 2** naming Z8.
  Revert by **targeted edit, never `git checkout --`** (uncommitted lint work is in the same
  tree — `pattern_negative_control_checkout_wipes_uncommitted_fix`), then confirm the file
  digest is byte-identical to HEAD.

---

## Stage-4 executor rules — non-negotiable, cited by number below

Carried verbatim from Stage 3 and wave 4a. R1–R9 are unchanged; W1–W5 are wave 4a's.

- **R1 — Author at `'mt'`, always.** Never write `'bt'` at authoring time. `check-i18n`
  accepts `'bt'` as a valid enum member and the lint's R1 only reports entries *below* the
  bar, so a premature `'bt'` is invisible to every automated check in this repo. Promotion
  happens only after the reverse read, in its own commit.
- **R2 — Read ALL triples with `--verbose`.** `--ingest` truncates to 12. Across Stage 3 and
  wave 4a, essentially no real finding ranked inside the default twelve. O-IntonationPad's
  276 rows make twelve 4% of the batch. The score is a sort key, not a verdict.
- **R3 — Screen `TERMS` for page collisions BEFORE authoring.** Report any two *different*
  English keys mapping to one root that co-occur on a page. Qualify **both** sides, never one.
- **R4 — Fresh blind reader AND fresh salt per correction round.** They guard different
  things: the salt stops correlation against seen ids, the fresh agent stops self-agreement.
- **R5 — `--emit` is CORPUS-scoped, and its own usage line is wrong.** See W4.
- **R6 — Pass `--manifest` explicitly and fire the blinding controls per batch.** ids pure
  12-hex, no key fragment, ids returned identical and in order, rows well-formed with no Han
  surviving in the returned English, and a sha256 of the zh column against the committed tree.
  **Forbid repo access in the dispatch explicitly** — several of these plugins name themselves
  in their own copy (O-IntonationPad's `.title` product name is `I18N_EXEMPT` and present).
- **R7 — zh geometry failures are SHRINKS. Assert equality, pin at the widest of three.**
- **R8 — A French-era `min-width` pin is a FLOOR, not safety.** Re-measure every one. Grep for
  existing declarations on a selector before appending, or a new pin at higher specificity
  silently un-pins a French one and moves the **fr** arm.
- **R9 — Version truth is `CMakeLists.txt`.** Read the value; never grep for a literal; the
  value may be quoted, and on O-IntonationPad it is (N8).
- **W1 — The CJK tail goes BEFORE the trailing generic.** Chromium resolves a bare `serif` or
  `sans-serif` against the document's `lang`, so under `zh-Hans` the generic is already a
  Chinese face and a tail written after it is never consulted. The shipped convention is
  `…, 'PingFang SC', 'Microsoft YaHei', serif` — measured on this tree across five plugins,
  with `Arial, 'PingFang SC', 'Microsoft YaHei', sans-serif` for the sans case and the tail
  before `monospace` for the mono case.
- **W2 — OR the visibility flag across states.** Keying a node on first sighting while walking
  the state file cumulatively makes every node read as invisible for any panel the walk later
  navigates away from. On O-Prism that was 19 visible-Han nodes found where 48 existed.
- **W3 — Check `I18N_EXEMPT` membership BOTH ways.** A choice-parameter option string stays
  English in every language because it is what the button says; a localized caption must be
  in Chinese or the body sends a reader looking for a control that is not on the page. The
  discriminator is mechanical: every exempt option word in an English body must survive
  verbatim into the Chinese, and every non-exempt caption must not. Units in a range clause
  follow the READOUT, not the prose.
- **W4 — Pass BOTH `--emit <Plugin>` AND `--plugin <Plugin>`.** The tool's own usage line
  shows only `--emit`, and following it emits the **entire corpus** — 1338 already-shipped
  rows would be dispatched to an external reader as this plugin's work. `emit()` records the
  `--emit` argument as the manifest target and filters nothing; the row filter is `--plugin`.
- **W5 — Expect a Z8 violation after any Latin→Han token swap.** The space in `显示 ON，` is
  correct while the token is Latin and becomes an intra-Han space the moment it is replaced.
  Ten such violations landed in one edit on O-Prism. Under the flipped gate this now blocks.

**Plus, carried from the same source:**

- **Comment-blind greps lie.** Strip comments before any code count:
  `perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' <file>`.
- **When you repair a gate file, do not spell the pair literal in the comment that explains
  it.** Word it as prose, or the inventory grep keeps the file on the list forever.
- **Drive the plugin's own `tests/i18n-states.json`.** The gate cannot name what never renders.
- **Two nodes a keyed scan always misses:** the `#tooltip` surface (filled from `data-tip` at
  hover time, never keyed) and the endonym `<option>` (the only Han in the markup). Universal.
- **`grep -P` with a Han property byte-matches on this machine** and reports phantom hits on
  the middle dot U+00B7. Use the `perl -CSD` form in the verify blocks; do not substitute a
  grep. A Han script property in perl **without** `-CSD` is inert.
- **zsh does not word-split**, so `cat $files` and `grep … $FILES` fail silently; use
  `find … -exec … +`. And quote every `--include` glob (N13).

---

## The per-plugin procedure (nine steps, identical for all six)

Every task runs this loop. Written once here, referenced, not repeated.

1. **Screen for glossary page collisions (R3)** — before authoring a string. Push the plugin's
   English label set through `TERMS`; report any two different keys sharing a root that
   co-occur on a page. Then run the **mechanical downstream check** wave 4a added: after
   authoring, group the zh renderings and report any two *different* keys sharing one,
   excluding same-control pairs (a caption and its own tooltip title) and same-English pairs.
   The R3 screen alone only fires when both sides are glossary keys and is blind otherwise.
2. **`js/i18n.js`** — add `'zh-Hans'` to `LANGUAGES`, and a `'zh-Hans'` value on **every** key
   in both `I18N` and `LABELS`, all at `reviewed: 'mt'` (R1). While in this file, remove the
   sentence that names or counts the selector's options from the language hover-help in the en
   body and the fr body both, and remove the clause in the gear/settings body that asserts the
   panel holds nothing besides the language — **both are deletions, never extensions** (N10,
   N11). The selector already lists the languages in their endonyms, the one form a reader
   recognises without knowing the page language.
3. **`index.html`** — the endonym `<option value="zh-Hans">` beside the fr option, written as
   **numeric character references** to match the existing convention, which is numeric on all
   six (`Fran&ccedil;ais`).
4. **CJK font tail and the bare-generic repair** — serve the page, switch to Chinese, and read
   `getComputedStyle(node).fontFamily` on every node that **holds or can receive** a Han
   codepoint (own text, `data-tip`, `data-tip-title`, `aria-label`), driving every state in
   `tests/i18n-states.json` and **OR-ing the visibility flag across states** (W2). Append the
   tail **before** the trailing generic (W1) on the measured stacks only. Separately, and this
   is the half that holds no Chinese: name a face on every stack whose only non-generic
   families are absent on this machine (N3) — no tail there, because those nodes hold no Han.
   Measure by computed style, not by grepping either file: the carriers are CSS declarations,
   SVG presentation attributes (N1) and custom properties (N2). Record both halves — a wave
   that appends the tail to everything it measured can no longer tell a needed tail from a
   decorative one.
5. **`PluginProcessor.h`** — widen the two-way `languageCode` / `languageIndex` ternary to
   three-way, pure ASCII. Zero Chinese characters anywhere under `Source/`.
6. **Geometry pass (R7, R8)** — run the zh arm of `check-ui-labels` and let it *name* the
   offenders. Expect four shapes: `line-height: normal` inheritance (pin each named leaf at
   its measured English line box, derived from the box — height minus padding minus border —
   unitless, never global, and a class at two sizes needs two pins); a content-sized element
   that shrank and pulled its neighbours; a Han run whose min-content width is one character
   in any auto-sized table or grid (`nowrap` **plus** a min-width); and Latin display
   conventions applied to Han, where `letter-spacing` and generous side padding are trimmed
   under `html[lang="zh-Hans"]` **with the trim value MEASURED, not derived from the glyph
   advance**. Re-run until 0 FAIL on **all three** arms.
7. **Commit at `'mt'`** — path-scoped, `git commit -- plugins/<Name>`. The reverse pass runs
   against a committed tree.
8. **Blind reverse read (R2, R4, R5, R6, W4)** — `--emit <Plugin> --plugin <Plugin>` to a
   fresh `--out`, dispatch to a blind reader with repo access explicitly forbidden, `--ingest`
   with an explicit `--manifest` and `--verbose`. Read **every** triple. For each drift, the
   discriminator is **collision on the page**, not drift distance: can a reader on this page
   confuse this string with another control on the same page? A caption must also not collide
   with its own tooltip title. Re-author on collision; accept otherwise — and **write the
   reason down**, because an accepted drift with no recorded reason is indistinguishable from
   an unread one. A correction round is a full re-emit to a new `--out` with a fresh salt and
   a fresh reader, trimmed by hand, in its own commit that says plainly what the read found.
9. **Promote and ship** — flip to `reviewed: 'bt'`, bump the version from the CMakeLists value
   (R9), write the CHANGELOG entry naming the disclosed quality level, and
   `./scripts/build-and-install.sh <FolderName>` — the script resolves the `juce_add_plugin`
   target itself, so pass the folder name even where the target differs (it differs on four of
   six). Promotion and ship metadata land together in a final commit whose message names every
   provenance string.

**Deferred to the end of the batch:** `auval`. A cold `auval` after an install rebuilds the
whole AU registry (~15 min); six separate runs would cost six rescans. Build and install all
six, then sweep once.

**`reviewed: 'native'` stays OPEN on all six and is not a blocker.** This project has no
native Chinese reader. A disclosed quality level, printed by lint rule R1 on every run and
stated in all six CHANGELOGs.

---

## Source coverage audit

| # | Source | Item | Covered by |
|---|---|---|---|
| 1 | DESC | O-IntonationPad localized (276 rows, largest) | Task 2 |
| 2 | DESC | O-DigiDelay localized | Task 3 |
| 3 | DESC | O-AnalogEQ localized | Task 3 |
| 4 | DESC | O-Tremolo localized | Task 1 |
| 5 | DESC | O-SimpleReverb localized | Task 3 |
| 6 | DESC | O-Emulator localized | Task 3 |
| 7 | DESC | Author at `'mt'`, blind reverse read, promote to `'bt'` | Tasks 1–3, step 8 |
| 8 | DESC | Drive UI states, tail font stacks, fix geometry | Tasks 1–3, steps 4 and 6 |
| 9 | DESC | Minor version bump, build + install | Tasks 1–3, step 9 |
| 10 | DESC | End-of-batch cold auval sweep, once | Task 3 close-out |
| 11 | DESC | PLUGINS.md rows | Task 3 close-out |
| 12 | CONSTRAINT | Live-observation authority (paths, versions, gates read now) | Live observations table |
| 13 | CONSTRAINT | Lint flip decided explicitly and recorded | "The lint gate flip" section; Task 1 |
| 14 | CONSTRAINT | Path-scoped commits only, never `git add -A` | Task 3 commit discipline; every task |
| 15 | READING | Wave 4a findings 1–13 baked in as task instructions | Executor rules W1–W5 + steps 1, 4, 6, 8 |
| 16 | READING | O-IntonationPad's 17 late tip bindings in scope | Task 2, ZH4B-09 |
| 17 | READING | Stage-3 executor rules R1–R9 | Executor rules section |

No item is uncovered. Excluded and stated: O-Chorus's inert CJK tail (wave 4a finding 1 — its
own plugin's pass), the two defective glossary roots (wave 4a finding 12 — reported to the
glossary owners, editing a settled root is a repo-wide change this task does not own), and
O-Bells' 2 late tip bindings (not in this wave).

---

<tasks>

<task type="tracer">
  <name>Task 1: Flip the lint to a gate, then take O-Tremolo end-to-end (ZH4B-01, ZH4B-07, ZH4B-08, ZH4B-10)</name>
  <files>scripts/i18n-zh-lint.js, plugins/O-Tremolo/Source/ui/public/js/i18n.js, plugins/O-Tremolo/Source/ui/public/index.html, plugins/O-Tremolo/Source/PluginProcessor.h, plugins/O-Tremolo/tests/ui_tip_render_check.js, plugins/O-Tremolo/CMakeLists.txt, plugins/O-Tremolo/CHANGELOG.md</files>
  <precondition>`node scripts/i18n-zh-lint.js --self-test` reports 10/10, `node scripts/check-i18n.js` exits 0, and `node scripts/i18n-zh-lint.js` reports 0 findings across 43 plugins on the tree as found. If any of the three fails, stop — the flip's negative control cannot be distinguished from a pre-existing defect, and every zero this task produces is unevidenced.</precondition>
  <reversibility rating="costly">The lint flip changes the exit contract of a tool eleven plugins' CI-adjacent checks call. Reverting is one line, but any wave that ships between the flip and a revert was authored under a different contract.</reversibility>
  <action>
**Part A — the gate flip, in its own commit, before O-Tremolo is touched.**

Change `scripts/i18n-zh-lint.js` to exit 2 when any plugin reports a finding, and exit 0
otherwise. Update the closing banner and the header block so the tool no longer describes
itself as report-only. Entries at the machine-draft review level must continue to be counted
in the ship-bar line and must **not** become findings — that routing already exists and must
not change, because it is what lets this wave author 492 rows under a live gate.

Fire both controls and record both outputs:

- **Negative control.** Run repo-wide immediately after the flip, against the tree as found.
  It must still exit 0 on 43 plugins. A gate that fires on a clean corpus is not a gate.
- **Positive control.** Plant one intra-Han space inside a committed zh row on any already
  shipped plugin, run repo-wide, and observe **exit 2** with Z8 naming that row. Then revert
  by a **targeted reverse edit** — never `git checkout --`, because uncommitted lint work is
  in the same tree — and confirm the file's sha256 is byte-identical to HEAD.

Commit the flip alone, path-scoped to the script, with a message that names both control
outcomes. Nothing else is in that commit.

**Part B — O-Tremolo, the full nine-step procedure.**

O-Tremolo is the tracer because it is the thinnest path that touches every layer this wave
modifies, and every layer is present on it: 34 entries / 43 rows, 2 states, no external CSS,
twelve literal stacks **split evenly between the two font shapes** (six that name a face this
machine has and six that do not), a gate file that names its languages in the syntax the
census cannot see, a direction-specific tip-height assertion, and both stale enumerations.

Three things it proves for the other five, and each must be recorded for reuse:

**The discriminating font repair.** Six of its twelve stacks are safe and six are not. The
tail goes only on stacks that carry Han, positioned before the trailing generic; naming a
face goes only on stacks whose non-generic families are absent from this machine. A blanket
edit that touches all twelve cannot tell the two apart afterwards, and a blanket tail changes
Latin metrics on the en arm. Record which stacks took which treatment and why.

**The gate-file repair.** Its `ui_tip_render_check.js` iterates a three-element array literal
naming its languages. Repair it by **deriving** the list from the table's own `LANGUAGES`
export, with a derive-or-abort control: a planted empty list must make the gate exit non-zero
rather than pass vacuously. Fire that control and revert it by targeted edit. Word the
explanatory comment as prose — do not spell the pair literal in it, or the repo-wide inventory
grep keeps this file on the list forever.

**The direction assertion.** Its assertion `[5]` requires the non-English pass to be strictly
taller than English on at least one tip. Chinese shrinks tip height, so this hard-fails the zh
arm on a correct page. Rewrite it to assert a **difference** — that the non-English pass
measured something other than English. If no tip's height differs from English, that pass is
the same measurement twice and its half of the clamp assertion is decoration, which is what
the assertion actually needs to catch.

Then the rest of the loop: screen `TERMS` first (R3); author at `'mt'` (R1); remove the
fixed-pair phrase from the language body in en and fr, and the "nothing else" clause from the
gear body in en and fr (N10); add the endonym option as numeric references; widen the
`languageCode` / `languageIndex` ternary to three-way; pin geometry at the measured English
box after grepping the four existing min-width and four line-height declarations for
specificity conflict (R8); commit at `'mt'`; run the blind reverse pass with `--verbose`, an
explicit `--manifest`, a fresh salt, **both** `--emit` and `--plugin` (W4), and a dispatch
that forbids repo access (R6); read every triple; promote in a second commit with the version
bumped from the CMakeLists value 1.9.1 to 1.10.0 (R9) and the CHANGELOG entry; build and
install with `./scripts/build-and-install.sh O-Tremolo` (the script resolves the differing
CMake target itself). **Do not run auval** — it is deferred to Task 3.
  </action>
  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -3   # 10/10 before anything is trusted</automated>
    <automated>node scripts/i18n-zh-lint.js >/dev/null 2>&1; echo "flipped-lint repo-wide exit=$?"   # 0 on a clean corpus — the flip's negative control</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' scripts/i18n-zh-lint.js | grep -c "process.exit(2)"   # >= 1 — the flip is in code, not only in the banner</automated>
    <automated>node scripts/check-i18n.js --plugin O-Tremolo; echo "exit=$?"   # exit 0, LANGUAGES lists three</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-Tremolo; echo "exit=$?"   # exit 0, 0 FAIL, all three arms</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-Tremolo 2>&1 | tail -20   # 0 findings, BELOW SHIP BAR 0</automated>
    <automated>node scripts/i18n-fr-lint.js; echo "fr-lint exit=$?"   # exit 0 — French untouched</automated>
    <automated>node plugins/O-Tremolo/tests/ui_tip_render_check.js; echo "tip-gate exit=$?"   # exit 0 with a real zh arm</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/O-Tremolo/tests/ui_tip_render_check.js | grep -cE "en,fr|\['en', *'fr'\]|'en', *'fr', *'en'"   # 0 — no two-language literal in any syntax, comments included</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/O-Tremolo/tests/ui_tip_render_check.js | grep -cE "hFr\[[a-z]+\] *> *hEn"   # 0 — the strict one-directional height comparison is gone</automated>
    <automated>find plugins/O-Tremolo/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +   # MUST print nothing</automated>
    <automated>perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-Tremolo/Source/ui/public/js/i18n.js   # positive control for the line above — MUST print the file</automated>
    <automated>perl -0777 -ne 'print "STALE ENUMERATION: $&\n" while /(English and French|English or Fran|anglais et le fran|English ou Fran|Two settings: English)/gi' plugins/O-Tremolo/Source/ui/public/js/i18n.js   # MUST print nothing</automated>
    <automated>grep -c "PingFang SC" plugins/O-Tremolo/Source/ui/public/index.html   # >0 — the tail landed on measured stacks</automated>
    <automated>grep -oE "font-family: *'?Garamond'?, *serif" plugins/O-Tremolo/Source/ui/public/index.html | wc -l | tr -d ' '   # 0 — no stack ends at a generic without naming an installed face</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-Tremolo --strict-tips 2>&1 | tail -6   # 0 DEAD, 0 late, 0 failed</automated>
    <human-check>Both lint-flip controls fired and their outputs recorded: exit 0 on the clean corpus, exit 2 on the planted intra-Han space, and the script's sha256 byte-identical to HEAD after the targeted revert.</human-check>
    <human-check>The gate file's derive-or-abort control fired: a planted empty language list exits non-zero, and the reverting edit leaves the file byte-identical.</human-check>
    <human-check>Every one of the ~43 back-translation triples read with --verbose. Each accepted drift carries a written collision-on-the-page reason.</human-check>
  </verify>
  <done>
The lint exits 2 on a finding, with both controls fired and recorded, in a commit of its own
that touches nothing else. O-Tremolo ships three languages at `reviewed: 'bt'` on every row,
version 1.10.0 from the CMakeLists value, CHANGELOG written, built and installed. Its gate
file derives its language list, holds no two-language literal in any syntax with comments
included, and asserts a height difference rather than a direction. Its six bare-generic stacks
name an installed face and its Han-bearing stacks carry the tail before the generic. Two or
more path-scoped commits for the plugin: the `'mt'` table, then the promotion. The reusable
findings — which stacks took which treatment, the pin set, the collisions, the blind-dispatch
shape and the two control procedures — are written down for Tasks 2 and 3.
  </done>
</task>

<task type="auto">
  <name>Task 2: O-IntonationPad — 276 rows, 19 states, two controllers, 17 late tip bindings (ZH4B-02, ZH4B-08, ZH4B-09)</name>
  <files>plugins/O-IntonationPad/Source/ui/public/js/i18n.js, plugins/O-IntonationPad/Source/ui/public/index.html, plugins/O-IntonationPad/Source/ui/public/css/tuning-panel.css, plugins/O-IntonationPad/Source/ui/public/js/app.js, plugins/O-IntonationPad/Source/ui/public/js/tuning-panel.js, plugins/O-IntonationPad/Source/PluginProcessor.h, plugins/O-IntonationPad/CMakeLists.txt, plugins/O-IntonationPad/CHANGELOG.md</files>
  <precondition>Task 1's commits are in the tree, the lint is flipped, and the tracer's recorded findings are available — this plugin reuses the stack-measurement method, the pin derivation and the blind-dispatch shape the tracer proved.</precondition>
  <action>
Run the nine-step procedure on **O-IntonationPad** — the heavy plugin of the wave, and it gets
this task to itself for that reason. 199 entries / **276 rows**, **19 states**, an external
`css/tuning-panel.css` alongside the markup, **two controllers** (`app.js` and
`tuning-panel.js`, plus `constants.js`), 7 min-width + 3 min-height + 3 line-height existing,
`IS_SYNTH TRUE`, and 80 tip bindings of which 17 currently resolve late.

Five things make this plugin different from the tracer, and each is a measured trap:

**Nineteen states, and the copy lives behind tabs.** The state file navigates a SYNTH tab, an
EFFECTS tab with four bypass faces, and a TUNING tab that `tuning-panel.js` injects wholesale.
The gate cannot name what never renders, so drive every state for both the font-stack
measurement and the geometry pass — and **OR the visibility flag across states** (W2), or the
whole tuning panel reads as invisible the moment the walk navigates to the next tab. Wave 4a
measured that exact miss on O-Prism: 19 nodes found where 48 existed, and a missing tail is a
font fallback rather than a geometry change, so no gate would have said so.

**Nineteen bare-generic stacks, one of them a lone `monospace`.** Eighteen declarations resolve
through a serif generic with no installed named family, and one names nothing at all but the
mono generic — the purest form of the defect, where both Latin and Han are resolved by
document language. Name a face on all nineteen; the shipped mono convention places the CJK
tail before `monospace` with a named mono family ahead of it. Three `-apple-system` stacks are
Latin-safe on this machine but reach a bare `sans-serif` for Han, so they take the tail before
that generic if they carry visible Chinese.

**Stacks in two files, and the split is lopsided in a way that matters.** Twenty-one
declarations sit in `index.html` and only **two** in `css/tuning-panel.css` — but one of those
two is the lone `monospace` at line 317, the wave's purest bare generic. An executor who reads
the markup and stops has covered 18 of the 19 defects and left the worst one. Measure by
computed style; a sweep of either file alone under-covers.

**The version is quoted and the bump is a decade rollover.** `VERSION "2.9.2"` — read the
value, do not grep for a bare literal (R9, N8). The shipping version is **2.10.0**.

**Seventeen late tip bindings, all in the TUNING tab, all this wave's own copy.** They are
`#interval-list`, the five visualisation buttons, `#library-section`, `#ref-pitch-knob`,
`#scale-name-display`, `#octave-stretch`, `#pitch-bend-range`, the five scale file buttons and
`#generator-section` — every one injected by `tuning-panel.js` after the first tooltip sweep,
so the binding finds nothing and the node carries no tip. Resolving them is in scope because
they are precisely the nodes whose captions this task translates: a late binding means the
Chinese tip never renders either. **Fix by re-sweeping the bindings after the panel is
injected, not by changing selectors** — the selectors are correct and resolve at settle, which
is why the census reports them as late rather than dead. Then prove it: `boot-all-uis
--strict-tips` must report 0 late and 0 dead for this plugin, and the same run must still
report O-Bells' known 2 late bindings, or the gate has gone blind rather than the defect
having been fixed.

Also carry: this plugin's gear/settings body is the one in the wave that correctly names both
of its choices — **read it and leave it**, do not apply the deletion mechanically (N10). Its
language body does carry the fixed-pair phrase in en and fr and takes the deletion. Its
`I18N_EXEMPT` list holds 14 entries including the product name in `.title` and several
choice-parameter values; screen every one both ways (W3), and expect the reverse batch to be
large — `--emit` is corpus-scoped, so pass `--plugin` as well (W4) — and expect the real
findings to rank outside the first twelve of 276 (R2).

Author at `'mt'`, commit, read every triple, correct with a fresh reader and fresh salt if
needed (R4), promote, bump to 2.10.0, build and install with
`./scripts/build-and-install.sh O-IntonationPad`. **Do not run auval.**
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --plugin O-IntonationPad; echo "exit=$?"</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-IntonationPad; echo "exit=$?"   # exit 0, 0 FAIL on all three arms across all 19 states</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-IntonationPad 2>&1 | tail -20   # 0 findings, BELOW SHIP BAR 0</automated>
    <automated>node scripts/i18n-fr-lint.js; echo "fr-lint exit=$?"   # exit 0 — the fr arm must not have moved</automated>
    <automated>find plugins/O-IntonationPad/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +   # MUST print nothing</automated>
    <automated>perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-IntonationPad/Source/ui/public/js/i18n.js   # positive control — MUST print the file</automated>
    <automated>perl -0777 -ne 'print "STALE ENUMERATION: $&\n" while /(English and French|English or Fran|anglais et le fran|English ou Fran)/gi' plugins/O-IntonationPad/Source/ui/public/js/i18n.js   # MUST print nothing</automated>
    <automated>find plugins/O-IntonationPad/Source/ui/public \( -name '*.html' -o -name '*.css' \) -exec grep -hoE "font-family: *'?Garamond'?, *serif|font-family: *monospace" {} + | wc -l | tr -d ' '   # 0 — every stack names an installed face before its generic</automated>
    <automated>find plugins/O-IntonationPad/Source/ui/public \( -name '*.html' -o -name '*.css' \) -exec grep -c "PingFang SC" {} + | awk -F: '{s+=$NF} END{print s}'   # >0 in BOTH files — the split-file sweep reached the stylesheet</automated>
    <automated>perl -ne 'print "$1\n" if /^\s+VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ && !$d++' plugins/O-IntonationPad/CMakeLists.txt   # 2.10.0 — read as a value, quoted form tolerated</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-IntonationPad --strict-tips 2>&1 | tail -8; echo "exit=$?"   # late-tips 0, DEAD 0, failed 0 — was 17 late</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-Bells --strict-tips 2>&1 | grep -i "late"   # control: O-Bells still reports its known 2 late bindings, so the census is not blind</automated>
    <human-check>Every one of the 276 back-translation triples read with --verbose. Each accepted drift carries a written collision-on-the-page reason.</human-check>
    <human-check>The gear/settings body was read and deliberately left alone because it names both of its choices — recorded as a checked non-defect, not an unchecked one.</human-check>
  </verify>
  <done>
O-IntonationPad ships three languages at `reviewed: 'bt'` on all 276 rows, version 2.10.0 read
from the quoted CMakeLists value, CHANGELOG written, built and installed, geometry equal on
all three arms across all 19 states. All nineteen bare-generic stacks name an installed face,
the tail lands before the generic in both the markup and `tuning-panel.css`, and the seventeen
late tip bindings resolve at the first sweep with O-Bells' two still reported as the census
control. Path-scoped commits: the `'mt'` table, any correction round, then the promotion.
  </done>
</task>

<task type="auto">
  <name>Task 3: O-DigiDelay, O-AnalogEQ, O-SimpleReverb, O-Emulator — four gate repairs and the batch close-out (ZH4B-03..08, ZH4B-11)</name>
  <files>plugins/O-DigiDelay/Source/ui/public/js/i18n.js, plugins/O-DigiDelay/Source/ui/public/index.html, plugins/O-DigiDelay/Source/PluginProcessor.h, plugins/O-DigiDelay/tests/ui_tip_render_check.js, plugins/O-DigiDelay/CMakeLists.txt, plugins/O-DigiDelay/CHANGELOG.md, plugins/O-AnalogEQ/Source/ui/public/js/i18n.js, plugins/O-AnalogEQ/Source/ui/public/index.html, plugins/O-AnalogEQ/Source/PluginProcessor.h, plugins/O-AnalogEQ/tests/ui_tip_render_check.js, plugins/O-AnalogEQ/CMakeLists.txt, plugins/O-AnalogEQ/CHANGELOG.md, plugins/O-SimpleReverb/Source/ui/public/js/i18n.js, plugins/O-SimpleReverb/Source/ui/public/index.html, plugins/O-SimpleReverb/Source/PluginProcessor.h, plugins/O-SimpleReverb/tests/ui_tip_render_check.js, plugins/O-SimpleReverb/CMakeLists.txt, plugins/O-SimpleReverb/CHANGELOG.md, plugins/O-Emulator/Source/ui/public/js/i18n.js, plugins/O-Emulator/Source/ui/public/index.html, plugins/O-Emulator/Source/PluginProcessor.h, plugins/O-Emulator/tests/ui_tip_render_check.js, plugins/O-Emulator/CMakeLists.txt, plugins/O-Emulator/CHANGELOG.md, PLUGINS.md</files>
  <precondition>Tasks 1 and 2 are committed and their plugins are installed — the cold auval sweep at the end of this task covers all six and cannot run until every bundle is on disk.</precondition>
  <action>
Run the nine-step procedure on the four remaining plugins, **one path-scoped commit stream per
plugin** so a failure in one does not strand the others. Per-plugin specifics, all measured at
planning time:

**O-DigiDelay** — 36 entries / 46 rows, 2 states, no external CSS, nine literal stacks, 5
min-width + 4 line-height existing, version 1.5.1 at line 12 → **1.6.0**, folder name
`O-DigiDelay` (CMake target differs; the build script resolves it). **Its nine stacks all name
Times New Roman already, so it needs the tail and no bare-generic repair** — the only plugin in
the wave with zero Latin exposure. Its gate file iterates a three-element array literal naming
its languages and is not on the census; repair it by deriving from `LANGUAGES` with a
derive-or-abort control fired, and rewrite its direction-specific `[5]` assertion to assert a
difference, both exactly as the tracer did. Its gear body claims the panel holds nothing but
the language — false since the hover-help switch landed; remove the clause in en and fr.

**O-AnalogEQ** — 35 entries / 49 rows, 2 states, no external CSS, version 1.4.1 at line 12 →
**1.5.0**, folder name `O-AnalogEQ`. **This is the font plugin of the wave.** All 15 CSS
declarations and all **24 SVG `<text>` presentation attributes** are the bare-generic shape,
and the 24 attributes hold nothing but ASCII frequency ticks on the four dial faces (N1, N3).
Expect the zh geometry arm to name a large number of movers that carry no translated string at
all — that is the Latin half of the defect, and naming a face on those stacks is what closes
it, with no tail there because those nodes hold no Han. A sweep that greps for the
property-and-colon form finds 15 of 39; measure by computed style. Its gate file is the
off-census three-element shape and carries the direction-specific `[5]`; both take the same
repair. Its gear body asserts the panel holds one control — false; remove in en and fr.

**O-SimpleReverb** — 32 entries / 43 rows, 2 states, no external CSS, eleven stacks of which
five are bare-generic and six name Times New Roman, 6 min-width + 5 line-height existing,
version 1.8.1 at line 8 → **1.9.0**, folder name `O-SimpleReverb`. Its gate file is the
on-census joined-pair shape with a hard assert that will fail outright the day a zh entry
lands; derive it with the control fired. It has **no** direction-specific assertion. Its
language body **counts** its options ("Two settings…") and its gear body says nothing else
lives in the panel — both false, both removed in en and fr (N10, N11).

**O-Emulator** — 27 entries / 35 rows, 2 states, version 1.3.1 at line 15 → **1.4.0**, folder
name `O-Emulator`. **The only tokenised plugin in the wave**: eight declarations resolve
through a custom property whose value already names Times New Roman ahead of the generic, so
**one edit to that token puts the tail on the whole page** and no bare-generic repair is
needed (N2). A token screen that looks for a property named after the word "font" returns zero
here and is wrong. Its gate file is the on-census joined-pair shape and takes the derive
repair with no direction rewrite. Its `I18N_EXEMPT` list is the largest relative to its table —
sixteen entries covering the five console names and the product name — so the both-ways
membership screen (W3) matters more here than anywhere else in the wave: the five console
names stay English inside a Chinese sentence because that is what the segments and the
automation lane say, while every non-exempt caption must be in Chinese. Its gear body claims
the panel holds only the language — false; remove in en and fr.

All four carry the fixed-pair phrase in the language hover-help in en and fr; remove it in all
four. All four hold a hover-help switch in their markup, which is what makes their gear bodies
false.

**Batch close-out, after all four are built and installed:**

1. **One cold auval sweep covering all six.** Read each triple off `auval -a` — never guess
   one. Measured at planning time: O-IntonationPad declares `IS_SYNTH TRUE` and the other five
   do not, so expect one instrument and five effects, with plugin codes `OuIP`, `OuTr`, `OuDD`,
   `OuAE`, `OuSr`, `OEmu` — but **read the rows, because wave 4a's plan asserted the split and
   got it wrong**. Expect one registry rescan of roughly fifteen minutes for the whole sweep.
   Before the sweep, confirm no alternate-variant orphan is on disk for any of the six: all six
   were `-dev` only at planning time, and a leftover unsuffixed bundle beside a `-dev` one pins
   Logic's registry slot to whichever was installed first.
2. **`PLUGINS.md`, once, in its own commit.** All six rows are behind their CMakeLists as found
   (see the live table). Write the shipped versions and mark the rows installed. Then run the
   duplicate check — it must print nothing.
3. **Repo-wide regression sweep** — the whole suite, not just this wave, including the flipped
   lint, which now exits 2 rather than reporting.

**Commit discipline throughout this task and the two before it:** re-check
`git branch --show-current` and `git status --short` immediately before every commit, not once
at the start. This is a shared checkout and another session's staging can join your commit in
the gap. Name paths explicitly; never `git add -A`, never `git commit -a`. Note
`.gsd/dispatch-isolation-sentinel.json` is modified in the tree and predates this work —
never stage it.
  </action>
  <verify>
    <automated>for p in O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do node scripts/check-i18n.js --plugin $p >/dev/null 2>&1; echo "$p check-i18n exit=$?"; done</automated>
    <automated>for p in O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do node scripts/check-ui-labels.js --plugin $p >/dev/null 2>&1; echo "$p check-ui-labels exit=$?"; done</automated>
    <automated>for p in O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do echo "--- $p"; node scripts/i18n-zh-lint.js --plugin $p 2>&1 | tail -8; done   # 0 findings each</automated>
    <automated>node scripts/check-i18n.js; echo "repo-wide exit=$?"   # 43 localized plugins, sixteen now three-language</automated>
    <automated>node scripts/i18n-zh-lint.js >/dev/null 2>&1; echo "flipped lint repo-wide exit=$?"   # 0 — and it is now a gate, so this zero is load-bearing</automated>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/i18n-fr-lint.js; echo "exit=$?"   # exit 0 — French untouched by the whole wave</automated>
    <automated>for p in O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do echo -n "$p han-in-cpp: "; find plugins/$p/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' {} + | wc -l | tr -d ' '; done   # 0 each</automated>
    <automated>for p in O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do echo -n "$p control: "; perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' plugins/$p/Source/ui/public/js/i18n.js | wc -l | tr -d ' '; done   # 1 each — a 0 means the gate above proved nothing</automated>
    <automated>for p in O-Tremolo O-IntonationPad O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do echo -n "$p stale-enum: "; perl -0777 -ne '$n++ while /(English and French|English or Fran|anglais et le fran|English ou Fran|Two settings: English)/gi; END{print $n+0,"\n"}' plugins/$p/Source/ui/public/js/i18n.js; done   # 0 on all six</automated>
    <automated>for p in O-Tremolo O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do echo -n "$p two-lang literal: "; perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/$p/tests/ui_tip_render_check.js | grep -cE "en,fr|\['en', *'fr'\]|'en', *'fr', *'en'"; done   # 0 on all five, comments included</automated>
    <automated>for p in O-Tremolo O-DigiDelay O-AnalogEQ; do echo -n "$p direction assert: "; perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/$p/tests/ui_tip_render_check.js | grep -cE "hFr\[[a-z]+\] *> *hEn"; done   # 0 on all three</automated>
    <automated>for p in O-Tremolo O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do node plugins/$p/tests/ui_tip_render_check.js >/dev/null 2>&1; echo "$p tip-gate exit=$?"; done   # exit 0 each with a real zh arm</automated>
    <automated>for p in O-Tremolo O-IntonationPad O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do echo -n "$p bare-generic: "; find plugins/$p/Source/ui/public \( -name '*.html' -o -name '*.css' \) -exec grep -hoE "font-family[:=] *\"?'?Garamond'?, *serif|font-family: *monospace" {} + | wc -l | tr -d ' '; done   # 0 on all six — includes the SVG attribute form</automated>
    <automated>node scripts/boot-all-uis.js --strict-tips 2>&1 | tail -8   # 0 DEAD, 0 failed; late count down by 17 from the baseline</automated>
    <automated>node scripts/i18n-zh-backtranslate.js 2>&1 | tail -6   # stage view: 1830 rows, 1830 at bt, 0 at mt</automated>
    <automated>grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d   # MUST print nothing</automated>
    <automated>for p in O-Tremolo O-IntonationPad O-DigiDelay O-AnalogEQ O-SimpleReverb O-Emulator; do cm=$(perl -ne 'print "$1\n" if /^\s+VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ && !$d++' plugins/$p/CMakeLists.txt); reg=$(grep -E "^\| $p \|" PLUGINS.md | cut -d'|' -f4 | tr -d ' '); [ "$cm" = "$reg" ] && v=AGREE || v=MISMATCH; echo "  $p cmake=$cm registry=$reg $v"; done   # AGREE six for six (MISMATCH six for six before the work)</automated>
    <automated>ls ~/Library/Audio/Plug-Ins/VST3/ ~/Library/Audio/Plug-Ins/Components/ | grep -E "^(O-Tremolo|O-IntonationPad|O-DigiDelay|O-AnalogEQ|O-SimpleReverb|O-Emulator)\.(vst3|component)$" | wc -l | tr -d ' '   # 0 — no unsuffixed alternate-variant orphan beside the -dev bundles</automated>
    <human-check>auval -v on all six triples, every triple read off auval -a rather than assumed, one cold rescan. All six report AU VALIDATION SUCCEEDED.</human-check>
    <human-check>Every back-translation triple for all four plugins read with --verbose; accepted drifts carry a written reason.</human-check>
    <human-check>Installed bundle CFBundleShortVersionString matches the shipped version for all six, VST3 and AU each.</human-check>
  </verify>
  <done>
All six wave-4b plugins ship English, French and Simplified Chinese at `reviewed: 'bt'` on
every row — 492 new rows, corpus at 1830 / 1830 `bt` / 0 `mt` across sixteen plugins. All five
gate files derive their language list and hold no two-language literal in any syntax, comments
included, with the derive-or-abort control fired on each; the three direction-specific
assertions assert a difference instead. No stack on any of the six ends at a generic without
naming an installed face, the SVG attribute form included. PLUGINS.md carries six corrected
rows in one commit, no duplicates, agreeing with CMakeLists six for six. Repo-wide
`check-i18n`, the flipped `i18n-zh-lint` and `i18n-fr-lint` all clean. auval PASS on all six.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| executor → blind reverse reader | An external agent receives Chinese strings with the English deliberately withheld. Anything that lets it recover the English, or correlate a correction batch against a prior one, makes the read vacuous. |
| glossary root → shipped copy | A settled `TERMS` root applied to two different English keys on one page ships an unreadable sentence that no automated check in this repo can see. |
| zh edits → en/fr rendering | CSS pins, font stacks and font tokens are shared surfaces. A change made for Chinese can move a language this work does not touch — and on this wave, naming a face changes the Latin metrics of 69 declarations. |
| report-only lint → gate | Flipping the exit contract of a tool the whole rollout depends on. A flip that fires on a clean corpus blocks five later waves; one that never fires is decoration. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-acr-01 | Information disclosure | `i18n-zh-backtranslate --emit` batch | high | mitigate | R6 + W4 — blind 12-hex ids, per-batch salt, explicit `--manifest`, **both** `--emit` and `--plugin` so the batch is not the whole 1338-row corpus, repo access forbidden in the dispatch; fire all five blinding controls per batch rather than trusting the code exists |
| T-acr-02 | Repudiation | correction rounds | medium | mitigate | R4 — fresh reader **and** fresh salt per round, so an agreement is independent rather than self-consistent |
| T-acr-03 | Tampering | shared `.git` index across concurrent sessions | high | mitigate | Re-check branch and `git status --short` immediately before every commit; path-scoped commits only; never stage `.gsd/dispatch-isolation-sentinel.json` |
| T-acr-04 | Denial of service | en/fr regression from a zh-motivated font or pin edit | high | mitigate | R8 — grep existing declarations before appending; naming a face on the 69 bare-generic stacks changes en/fr Latin metrics by design, so `check-ui-labels` must report 0 FAIL on the en and fr arms **after** that edit, not only after the tail; `i18n-fr-lint` clean repo-wide |
| T-acr-05 | Spoofing | a gate that passes vacuously | high | mitigate | Every negative gate carries a positive control that fired; derive-or-abort control on all five repaired gate files; the lint flip's own negative and positive controls; `--self-test` 10/10 before any zero is trusted; O-Bells' 2 late bindings as the boot-census control |
| T-acr-06 | Elevation of privilege | the flipped lint blocking unrelated work | medium | mitigate | The flip lands in its own commit against a corpus measured at 0 findings, with `reviewed: 'mt'` verified in source to route to the informational bucket rather than to findings — so authoring at the machine-draft level is not blocked |
| T-acr-SC | Tampering | package installs | low | accept | This wave installs nothing — no npm, pip or cargo operation is in scope. No legitimacy audit required. |
</threat_model>

<verification>
The wave is not done until **all six** are green — no partial waves.

Per plugin, all six:
- `check-i18n --plugin <Name>` exit 0, LANGUAGES reads three
- `check-ui-labels --plugin <Name>` exit 0, 0 FAIL, zh arm 0 geometry moved, en and fr arms unchanged
- `i18n-zh-lint --plugin <Name>` 0 findings, `BELOW SHIP BAR 0`
- every back-translation triple read with `--verbose`, drifts resolved with recorded reasons
- no stack ends at a generic without naming an installed face — CSS, SVG attribute and token forms
- `auval -v` AU VALIDATION SUCCEEDED, triple read off `auval -a`
- zero Han under `Source/**/*.{h,cpp}`, with the positive control fired on that plugin's own i18n.js

Repo-wide, once:
- `check-i18n` exit 0, 43 localized plugins
- `i18n-zh-lint` exits **2 on a finding** and 0 here; `--self-test` 10/10; both flip controls fired
- `i18n-fr-lint` exit 0
- `boot-all-uis --strict-tips` 0 DEAD, 0 failed, and O-IntonationPad's 17 late bindings gone
  with O-Bells' 2 still reported as the census control
- `i18n-zh-backtranslate` stage view 1830 rows / 1830 `bt` / 0 `mt`
- `PLUGINS.md` no duplicate rows; six rows agree with their CMakeLists
- all five repaired gate files carry no two-language literal in any syntax, comments included,
  and the three direction-specific assertions assert a difference

**A zero from a gate whose positive control was never run is not evidence.**
</verification>

<success_criteria>
Six plugins ship English, French and Simplified Chinese, built, installed and auval-clean.
Every zh row at `reviewed: 'bt'`, promoted only after a blind reverse read. English and French
geometry unchanged on all six after both halves of the font work. Five gate files repaired to
derive their language list, three of them also rewritten off a direction-specific assertion.
`i18n-zh-lint` is a gate, flipped in its own commit with both controls fired. O-IntonationPad's
seventeen late tip bindings resolve at the first sweep. One path-scoped commit stream per
plugin, PLUGINS.md once at the end, six registry rows corrected from the CMakeLists values.
</success_criteria>

<output>
Create `.planning/quick/260905-acr-wave-4b/260905-acr-SUMMARY.md` when done, and a
`deferred-items.md` beside it for anything out of scope that a later wave inherits.

Carry forward for wave 4c, in the same shape wave 4a used: anything structural this wave hit
that the remaining five waves will copy — in particular whether the flipped lint fired on any
wave-4b authoring round, and what the font-carrier census (CSS declaration, SVG presentation
attribute, role-named custom property) looks like on the next six.
</output>
