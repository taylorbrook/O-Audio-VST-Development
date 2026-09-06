---
phase: quick-260905-rwh
plan: 01
type: execute
wave: 1
depends_on: []
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4c, O-Detune, O-Bells, O-Gain, O-SpectralShaper, O-Bassoon, O-TextureForge, gates, fonts, geometry, canvas-text, shared-module]
autonomous: true
requirements: [ZH4C-01, ZH4C-02, ZH4C-03, ZH4C-04, ZH4C-05, ZH4C-06, ZH4C-07, ZH4C-08, ZH4C-09, ZH4C-10, ZH4C-11]
files_modified:
  # O-Detune (tracer)
  - plugins/O-Detune/Source/ui/public/js/i18n.js
  - plugins/O-Detune/Source/ui/public/index.html
  - plugins/O-Detune/Source/PluginProcessor.h
  - plugins/O-Detune/tests/ui_tip_render_check.js
  - plugins/O-Detune/tests/i18n-states.json            # CONDITIONAL — only if a zh-only panel needs driving
  - plugins/O-Detune/CMakeLists.txt
  - plugins/O-Detune/CHANGELOG.md
  # O-Bells (heavy)
  - plugins/O-Bells/Resources/ui/js/i18n.js
  - plugins/O-Bells/Resources/ui/index.html
  - plugins/O-Bells/Resources/ui/css/tuning-panel.css
  - plugins/O-Bells/Resources/ui/js/tuning-panel.js     # CONDITIONAL — only if a caption there is unkeyed
  - plugins/O-Bells/Source/PluginProcessor.h
  - plugins/O-Bells/tests/ui_tip_render_check.js
  - plugins/O-Bells/CMakeLists.txt
  - plugins/O-Bells/CHANGELOG.md
  # O-Gain
  - plugins/O-Gain/Source/ui/public/js/i18n.js
  - plugins/O-Gain/Source/ui/public/index.html
  - plugins/O-Gain/Source/PluginProcessor.h
  - plugins/O-Gain/CMakeLists.txt
  - plugins/O-Gain/CHANGELOG.md
  # O-SpectralShaper
  - plugins/O-SpectralShaper/Resources/ui/js/i18n.js
  - plugins/O-SpectralShaper/Resources/ui/index.html
  - plugins/O-SpectralShaper/Resources/ui/css/styles.css
  - plugins/O-SpectralShaper/Resources/ui/js/components/Spectrogram.js   # CONDITIONAL — the canvas caption decision (M8)
  - plugins/O-SpectralShaper/Source/PluginProcessor.h
  - plugins/O-SpectralShaper/CMakeLists.txt
  - plugins/O-SpectralShaper/CHANGELOG.md
  # O-Bassoon
  - plugins/O-Bassoon/Resources/ui/js/i18n.js
  - plugins/O-Bassoon/Resources/ui/index.html
  - plugins/O-Bassoon/Source/PluginProcessor.h
  - plugins/O-Bassoon/tests/ui_tip_render_check.js
  - plugins/O-Bassoon/CMakeLists.txt
  - plugins/O-Bassoon/CHANGELOG.md
  # O-TextureForge
  - plugins/O-TextureForge/Source/ui/public/js/i18n.js
  - plugins/O-TextureForge/Source/ui/public/index.html
  - plugins/O-TextureForge/Source/ui/public/css/ouaricon-naturalist.css
  - plugins/O-TextureForge/Source/ui/src/app.js                          # CONDITIONAL — only if the overlay buttons need a face (M10)
  - plugins/O-TextureForge/Source/ui/public/js/app.bundle.js             # CONDITIONAL — only if src/app.js changes; webpack rebuild
  - plugins/O-TextureForge/Source/PluginProcessor.h
  - plugins/O-TextureForge/tests/ui_tip_render_check.js
  - plugins/O-TextureForge/CMakeLists.txt
  - plugins/O-TextureForge/CHANGELOG.md
  # batch close-out
  - PLUGINS.md

estimate:
  tokens: 470000
  raw_tokens: 470000
  tasks: 3
  confidence: low
  # factor 1.0 — `gsd query estimate-calibration` reports sample_count 0, applied:false,
  # so no calibration multiplier is applied and tokens == raw_tokens. Confidence is DERIVED
  # from that zero sample count, not self-rated.
  #
  # BASIS (stated, because the three domain samples are not on one basis):
  #   Stage 3   610k tokens / 758 rows = 805 tok/row   } agent-context basis
  #   wave 4a   367k tokens / 580 rows = 633 tok/row   } agent-context basis
  #   wave 4b   118k / 492 rows                        — DIFF-SIZE basis (chars/4 over
  #             2375 insertions), NOT agent context. Not comparable; excluded from the rate.
  # Rate used: the mean of the two comparable samples, ~700 tok/row.
  # 609 rows x 700 = 426k, plus ~45k structural (4 gate repairs incl. the walk sites, 12 font
  # declaration edits, O-Bells' 32-node form-4 rule across a 14-state walk, the canvas-caption
  # decision, the shared-panel decision). Wave 4c's non-row work is materially LIGHTER than
  # 4b's (12 font declarations against 69, no lint flip, no late-tip fix), which is why this
  # does not scale linearly off 4b's 430k raw for 492 rows.

must_haves:
  truths:
    - "A user opens any of the six plugins, picks 简体中文 from the language selector, and every caption, section heading and hover-help body the plugin's own table reaches renders in Simplified Chinese."
    - "Switching back to English or French reproduces the pre-change page in geometry — no element moved on the en or fr arm, on any of the six."
    - "The Chinese page has no clipped, wrapped or displaced element: check-ui-labels reports 0 FAIL on the zh arm of all six, across all 14 of O-Bells' states."
    - "No ASCII text on any of the six pages changes face when the page language changes — every font stack that renders Latin names a typeface installed on the build machine before it reaches a generic."
    - "Every node that can receive a Han codepoint resolves to a CJK face named by the page, not by the document-language fallback — including the form controls that inherit no font-family at all."
    - "No Chinese character exists anywhere under any plugin's Source/**/*.{h,cpp} — the C++ carries only the ASCII language code."
    - "Every zh row has been read back through a blind reverse pass and either accepted with a written reason or re-authored."
    - "i18n-zh-lint, now a gate, exits 0 across the whole corpus and 2 on a planted violation."
    - "All six load in Logic: auval PASS on all six triples after one cold registry rescan."
  artifacts:
    - plugins/O-Detune/Source/ui/public/js/i18n.js            # zh-Hans on every I18N and LABELS key
    - plugins/O-Bells/Resources/ui/js/i18n.js
    - plugins/O-Gain/Source/ui/public/js/i18n.js
    - plugins/O-SpectralShaper/Resources/ui/js/i18n.js
    - plugins/O-Bassoon/Resources/ui/js/i18n.js
    - plugins/O-TextureForge/Source/ui/public/js/i18n.js
    - plugins/O-Detune/tests/ui_tip_render_check.js          # walk derived, direction assertion rewritten
    - plugins/O-Bells/tests/ui_tip_render_check.js           # walk derived, EXPECTED_LATE left pinned
    - plugins/O-Bassoon/tests/ui_tip_render_check.js         # walk derived
    - plugins/O-TextureForge/tests/ui_tip_render_check.js    # walk derived
    - PLUGINS.md                                             # six registry rows, corrected from CMakeLists
  key_links:
    - "LANGUAGES in i18n.js <-> the endonym <option> in index.html <-> languageCode/languageIndex in PluginProcessor.h — a third language present in two of the three renders a page that cannot be reached or cannot be persisted."
    - "A gate's language ASSERTION <-> its language WALK — they are separate code sites. Deriving `LANGUAGES.join(',') === 'en,fr'` from the table leaves two literal `sweep('en')`/`sweep('fr')` call sites that still walk exactly two languages, so the repaired gate reports green having never rendered the third."
    - "The CJK tail's POSITION relative to the trailing generic <-> whether it is consulted at all — a tail after a bare generic is dead code under a Chinese document language, and the page still looks right on macOS because the generic supplies a Chinese face."
    - "A form control's ABSENT font-family declaration <-> the UA stylesheet's Arial — 32 of O-Bells' nodes take their face from the UA, not from the page, so no grep of any file in the repo can see them and the tail never reaches them."
    - "measure-ui's Han-gated screens <-> whether the plugin has any Han yet — every one reads 0 on a pre-localization page, so a pre-work `--report all` is a vacuum, not a baseline."
    - "The blind reverse batch's manifest <-> the ids in the returned file — a manifest resolved from the wrong side joins nothing and the read is silently vacuous."
    - "A zh body <-> its own en body AFTER an enumeration deletion — no tool in this repo compares the two, so a row authored from the pre-deletion English ships a claim the English has stopped making."
---

<objective>
Localize six plugins into Simplified Chinese — O-Detune, O-Bells, O-Gain, O-SpectralShaper,
O-Bassoon, O-TextureForge — to the ship bar Stages 2–3 and waves 4a/4b established, and ship
each with a version bump, a build, an install and an auval pass. 609 emitter rows; the corpus
goes 1830 → 2439 rows and 16 → 22 localized plugins.

Purpose: wave 4c is the third of seven volume waves and the first authored end-to-end under a
blocking `i18n-zh-lint`. It is also the first wave whose plugins split across BOTH UI roots
(three under `Resources/ui/`, three under `Source/ui/public/`) and the first to carry a live
canvas text carrier, a shared cross-plugin UI module, and a webpack-bundled controller.

Output: six three-language plugins, one path-scoped commit stream per plugin, one PLUGINS.md
commit at the end of the batch, one cold auval sweep covering all six.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@.planning/quick/260905-acr-wave-4b/260905-acr-SUMMARY.md
@.planning/quick/260905-acr-wave-4b/deferred-items.md
@.planning/quick/260904-qrc-stage-4-wave-4a-of-the-zh-hans-rollout-o/260904-qrc-SUMMARY.md
@.planning/quick/260904-g5l-stage-3-of-the-zh-hans-rollout-the-hard-/260904-g5l-SUMMARY.md
@scripts/measure-ui-README.md
@scripts/i18n-zh-glossary.js
@scripts/i18n-zh-lint.js
@scripts/i18n-zh-backtranslate.js
</context>

---

## Live observations — measured on the working tree at planning time (2026-09-05)

Everything below was read off the tree at planning time under MUTABLE-SCOPE AUTHORITY. The
orchestrator's hand-off table was a starting point and is **corrected in four places** by these
measurements (entry counts, O-SpectralShaper's gate contents, the font picture, and every
"bare-generic" prediction). Wave 4a's plan got a plugin type wrong by trusting a carried
assertion; wave 4b's plan re-read instead. This one re-read.

| | O-Detune | O-Bells | O-Gain | O-SpectralShaper | O-Bassoon | O-TextureForge |
|---|---|---|---|---|---|---|
| **role in this wave** | **TRACER** | **HEAVY** | rest | rest | rest | rest |
| I18N + LABELS entries | 19 + 33 = 52 | **66 + 123 = 189** | 26 + 37 = 63 | 24 + 23 = 47 | 13 + 36 = 49 | 15 + 31 = 46 |
| **emitter rows** (tool's own rule) | 71 | **255** | 89 | 71 | 62 | 61 |
| UI root | `Source/ui/public/` | **`Resources/ui/`** | `Source/ui/public/` | **`Resources/ui/`** | **`Resources/ui/`** | `Source/ui/public/` |
| controller (check-i18n [6]) | inline module | inline module | `js/app.js` | `js/app.js` | inline module | **`js/i18n_init.js`** |
| external CSS | none | **`css/tuning-panel.css`** | none | **`css/styles.css`** | none | **`css/ouaricon-naturalist.css`** |
| `tests/i18n-states.json` states | 2 | **14** | 5 | 3 | **2 — no Tuning tab** | **1** |
| gate file | `ui_tip_render_check.js` | `ui_tip_render_check.js` | **NONE** | `ui_preset_menu_check.js` — **zero language refs** | `ui_tip_render_check.js` | `ui_tip_render_check.js` |
| gate: language ASSERTION form | **none at all — off census** | `LANGUAGES.join(',')==='en,fr'` | — | — | `LANGUAGES.join(',')==='en,fr'` | `LANGUAGES.join(',')==='en,fr'` |
| gate: language WALK form | **`for (const lang of ['en','fr','en'])` L467** | two call sites L689/L800 | — | — | two call sites L383/L412 | two call sites L501/L512 |
| gate: direction-specific `[5]` | **YES — `hFr[s] > hEn[s] + 0.5` L523** | no (log line only) | — | — | no | no |
| declared font sites | **9** (index.html) | **4** (2 html + 2 css) | **1** (index.html:43) | **1 token** (`--serif`, styles.css:73 → 12 decls) | **4** (index.html) | **1 token** (`--font-primary`, css:41 → 6 decls) |
| SVG `font-family=` attrs | 0 | 0 | 0 | 0 | 0 | 0 |
| canvas `fillText` w/ a literal | 0 | 0 | 0 | **1** (`Spectrogram.js:422`) | 0 | 0 |
| bare-generic decls (measured) | **0** — Georgia is installed | **1** (`monospace`, tuning-panel.css:358) | 0 | 0 | **2** (`'EB Garamond','Garamond',serif` :137 :432) | 0 |
| stacks needing only the TAIL | 9 | 3 | 1 | 1 token | 2 | 1 token |
| **form-4 nodes** (computed style, no declaration) | **1** (`#width`, input) | **32** (13 button / 17 input / 2 select) | **0** | **0** | **0** | 0 declared + **2 undriven** (M10) |
| `line-height: normal` visible leaves | 35 | **584** | 44 | 33 | 29 | 31 |
| existing min-w / min-h / line-h / letter-sp | 4 / 0 / 5 / 11 | 9 / 3 / 5 / **23** | 1 / 1 / 6 / 11 | 9 / 0 / 5 / 16 | 1 / 4 / 6 / 9 | 1 / 0 / 2 / 12 |
| stale LANGUAGE enumeration | HIT | HIT | HIT | HIT | HIT (+ a shared-panel disclosure, M9) | HIT |
| stale GEAR exclusivity clause | **HIT** | **HIT** | **correct — names both** | **correct — names both** | **HIT** | **HIT** |
| `CMakeLists` VERSION (read as value) | 1.8.1 (L16) | 4.4.1 (L11) | 1.3.3 (L11) | 1.7.3 (L5) | 1.3.1 (L14) | 1.3.1 (L30) |
| **shipping version** | **1.9.0** | **4.5.0** | **1.4.0** | **1.8.0** | **1.4.0** | **1.4.0** |
| `juce_add_plugin` target | `O-Detune` | `O-Bells` | `O-Gain` | `O-SpectralShaper` | `O-Bassoon` | **`OuariconTextureForge`** |
| PLUGIN_CODE | `OuDt` | `OBls` | `OGan` | `OSpS` | `OBsn` | `OuTF` |
| IS_SYNTH | false | **TRUE** | false | false | **TRUE** | **TRUE** |
| `setSize` (parsed frame) | 600 × 480 | 800 × 600 | 350 × 500 | 700 × 500 | **`setSize (900, 600)`** — space form | 900 × 600 |
| `PLUGINS.md` row as found | 1.7.1 📦 | 4.3.2 📦 | 1.3.2 📦 | 1.7.2 📦 | **1.2.2 🚧 Stage 0** | 1.2.1 📦 |
| installed bundle | `O-Detune-dev` | `O-Bells-dev` | `O-Gain-dev` | `O-SpectralShaper-dev` | `O-Bassoon-dev` | `O-TextureForge-dev` |
| late tip bindings | 0 | **2 — PINNED by its own gate (M11)** | 0 | 0 | 0 | 0 |

**Live total: 446 entries, 609 emitter rows.** The corpus goes 1830 → 2439 rows, 16 → 22
localized plugins. All six are installed as `-dev` only; no alternate-variant orphan on disk.

### Baseline, measured now — every one of these is the number the wave must not regress

- `check-i18n` repo-wide: **`ALL CHECKS PASS — 43 localized plugin(s)`**, canon v2, assertion
  [16] passing on all 43.
- `i18n-zh-lint` repo-wide: **`GATE PASSED — exit 0. 0 findings across 43 plugin(s).`**
- `i18n-zh-lint --self-test`: **10/10**.
- `i18n-zh-backtranslate` stage view: **1830 of 5254 rows, `BELOW SHIP BAR … 0`**.
- `i18n-fr-lint`: exit 0.
- `boot-all-uis --strict-tips`: **DEAD 0 across 0 plugins; late 2 across 1 plugin — O-Bells
  `#ref-pitch-knob`, `#octave-stretch`**. 3842 rendered text-bearing elements, title=0.
- `check-ui-labels --plugin <Name>`: **exit 0, 0 FAIL, on all six.**
- `measure-ui --mode box --report all`: **exit 0 on all six, 0 findings on all four screens on
  all six** — and see M1, because that is a vacuum rather than a clean bill.
- Installed font families, measured with `system_profiler SPFontsDataType`: **Georgia 4,
  Times New Roman 4, Arial 4, PingFang SC 6, Songti SC 4; Garamond 0, EB Garamond 0, Adobe
  Garamond Pro 0, Microsoft YaHei 0.**
- git: on `main`, one worktree, the only modified path is `.gsd/dispatch-isolation-sentinel.json`
  (predates this work — **never stage it**). No uncommitted change under any of the six plugin
  trees or `PLUGINS.md`.

---

## Twelve findings measured at planning time that wave 4b's summary does not contain

Wave 4b's carry-forward C1–C11 and wave 4a's findings 1–13 all apply and are folded into the
tasks. These are **new**, and each one costs the executor time or a wrong answer if not read
first.

### M1. `measure-ui --report all` is STRUCTURALLY VACUOUS on a pre-localization plugin

Run at planning time on all six: **every one of the four screens reported 0 findings on every
one of the six.** That is not a clean page. Three of the four screens — `undeclared-font`,
`line-height-normal`, `wrap-count`'s Han arm — filter on `x.han`, and no plugin in this wave
holds a Han codepoint yet. A pre-work `--report all` therefore cannot find anything, and a
reader who takes those zeros as a baseline has measured nothing at all.

**The screens run AFTER the zh table lands, not before.** What IS measurable now, and was
measured, are two language-independent proxies that give the worklist up front:

| proxy | how | what it gives |
|---|---|---|
| computed `ff` census over `lang==='en' && vis` rows | group the JSON on `x.ff` | every stack the page actually resolves, including the ones no file declares (form 4) |
| `lh === 'normal' && kids === 0 && own.trim()` count | filter the JSON | the upper bound on the `line-height-normal` worklist once Han arrives |

Both numbers are in the table above. `svg-font-attr` reports **0 attribute carriers** on all
six, which is its own liveness signal reading a genuine absence — form 2 is not present in
this wave.

### M2. A gate's language ASSERTION and its language WALK are SEPARATE code sites

This is the fourth enumeration form, and it is the one that matters most, because repairing
the form the census DOES see leaves the gate exactly as blind as before.

Three of the four gate files assert `LANGUAGES.join(',') === 'en,fr'` — the on-census
joined-pair shape wave 4b's C6 calls the "loud" failure mode. Deriving that assertion from the
table's own export fixes the hard-fail. **It does not fix the walk.** In all three the actual
render sweep is two literal call sites:

| plugin | assertion site | walk sites |
|---|---|---|
| O-Bells | L314 `LANGUAGES.join(',')` | **L689 `sweep('en')`, L800 `sweep('fr')`** |
| O-Bassoon | L209 `LANGUAGES.join(',')` | **L383 `sweep('en')`, L412 `sweep('fr')`** |
| O-TextureForge | L290 `LANGUAGES.join(',')` | **L501 `sweep('en')`, L512 `sweep('fr')`** |
| O-Detune | **none — no `LANGUAGES` reference in the file at all** | **L467 `for (const lang of ['en','fr','en'])`** |

A gate whose assertion is derived and whose walk is two literals passes green having never
rendered the Chinese page. **Repair BOTH sites on every gate, and the derive-or-abort control
(C6) must be fired against the WALK** — a planted empty list must make the gate exit non-zero,
not sweep zero languages silently.

### M3. O-Detune carries BOTH the off-census walk and the only direction-specific assertion

`for (const lang of ['en', 'fr', 'en'])` at L467 — the array-literal form the Stage-3 census
cannot see, and this file names no `LANGUAGES` anywhere, so even the joined-pair census misses
it entirely. And at L523:

```js
const grew = ANCHORS_DEFAULT.filter(s => hFr[s] > hEn[s] + 0.5);
check(grew.length > 0, ...);
```

Chinese shrinks tip height (wave 4b measured O-Tremolo 98→82, O-DigiDelay 92→77, O-AnalogEQ
0 grew / 5 shrank), so this hard-fails the zh arm on a page where nothing is wrong. Rewrite as
a difference per C5: `Math.abs(h[s] - hEn[s]) > 0.5`, per non-English language off the derived
list. That is what the assertion is actually for — catching a pass that measured English twice.
This combination is why O-Detune is the tracer.

### M4. Georgia IS installed here; Garamond is not — and that flips wave 4b's font arithmetic

Measured with `system_profiler SPFontsDataType`: **Georgia 4 family matches, Times New Roman 4,
Arial 4. Garamond 0, EB Garamond 0, Adobe Garamond Pro 0.**

Wave 4b's N3 found 69 bare-generic declarations because four of its plugins stacked only
Garamond variants before a generic. This wave's plugins mostly name **Georgia or Times New
Roman** as well, so the Latin half of the defect is nearly absent: **3 bare-generic
declarations in the whole wave**, not 69.

| plugin | bare-generic | why |
|---|---|---|
| O-Bells | 1 — `font-family: monospace` at `tuning-panel.css:358` | the purest form; names nothing at all |
| O-Bassoon | 2 — `'EB Garamond', 'Garamond', serif` at `index.html:137` and `:432` | both families absent on this machine |
| everything else | 0 | Georgia or Times New Roman is named before the generic |

**Do not carry wave 4b's "69 declarations" mental model into this wave.** Re-derive from the
installed-family measurement, which is a property of the machine and not of the plugin.

### M5. The whole wave's DECLARED font work is twelve sites, two of them single-token edits

Measured by file and line, so the executor never has to hunt:

| plugin | sites | treatment |
|---|---|---|
| O-Gain | `index.html:43` | tail only — **one edit covers the page** |
| O-SpectralShaper | `css/styles.css:73` (`--serif`) | tail only — **one edit covers 12 declarations** |
| O-TextureForge | `css/ouaricon-naturalist.css:41` (`--font-primary`) | tail only — **one edit covers 6 declarations** |
| O-Detune | `index.html:57 :333 :801 :832 :911` (`'Georgia','Times New Roman',serif`) and `:555 :601 :670 :719` (`'Georgia', serif`) | tail only, both shapes — Georgia is installed so no face-naming is needed anywhere on this plugin |
| O-Bassoon | `index.html:68 :412` | tail |
| O-Bassoon | `index.html:137 :432` | **face-naming only, NO tail** — both nodes hold the product name, which is `I18N_EXEMPT` and can never receive Han (M6b) |
| O-Bells | `index.html:57 :1262`, `css/tuning-panel.css:54` | tail |
| O-Bells | `css/tuning-panel.css:358` (`monospace`) | **name a mono face AND tail** — see M6c |

Wave 4b's N2 warned that a type token can be named for the typeface ROLE rather than the word
"font". Both token plugins here are found: `--serif` and `--font-primary`. A screen greping for
a custom property containing "font" finds one of the two.

### M6. O-Bells has 32 form-4 nodes, and it already declares `inherit` at six sites

Measured by computed style, in English, so this is language-independent and available now.

**(a) The 32.** 13 `<button>`, 17 `<input>`, 2 `<select>` resolve to bare `Arial` — the UA
stylesheet's face, not anything the page names. Every one is in the tuning panel:
`#tonic-down`, `#tonic-up`, `input.interval-input`, `button.viz-btn`, `#library-filter`,
`#octave-stretch`, `#btn-load-scl`, `#btn-load-kbm`, `#btn-save-scl`, `#btn-save-kbm`,
`#btn-export-html`, `#generator-type`, `#gen-divisions`, `#gen-period`, `#btn-generate`,
`#gen-count`. Arial has no Han glyphs, so their Chinese captions would be resolved by the
document-language fallback — W1's inert-tail failure arriving through an **absent** declaration.
Repair with **Arial kept FIRST** so the Latin metrics of the en and fr arms do not move (C2).

**And the sharpest part: `index.html` already carries `font-family: inherit` at six sites**
(L256, L359, L991, L1044, L1071, L1378). Those six do not cover the 32. **A plugin that has
already been given the form-4 repair on some of its controls is not a plugin that has had it.**
Only a computed-style census can tell the difference; no grep can.

**(b) O-Bassoon's two bare generics hold an exempt string.** Both `span.header-title` and
`div.about-title` render the literal product name, which is in `I18N_EXEMPT` and stays English
in every language. They can never receive Han, so they take the **face-naming half only**
(name `'Times New Roman'` before `serif`) and **no tail** — the textbook W3 discriminator, and
the reason the two halves of the font work must stay separable.

**(c) O-Bells' three `monospace` nodes** are `span.tk-cents` holding cents readouts. Under
`zh-Hans` a bare `monospace` resolves those digits through the document-language mono face, so
the readout changes metrics with no translated string near it. Name a mono face. Put the tail
on too, because the panel is `data-i18n`-driven and the selector is broad.

### M7. O-Bells carries 584 `line-height: normal` visible leaves — ten times the wave's median

Measured now: O-Bells **584**, O-Gain 44, O-Detune 35, O-SpectralShaper 33, O-TextureForge 31,
O-Bassoon 29. For scale, O-IntonationPad's unrepaired baseline that wave 4b spent a task on was
55. This is the wave's largest single geometry risk and it is concentrated in one plugin, which
is the other reason O-Bells gets a task to itself.

584 is the **upper bound**, not the worklist: only Han-carrying leaves reach the screen. Run
`--report line-height-normal` after the table lands to get the real number, then pin each named
leaf at its measured English line box — derived from the box (height minus padding minus
border), unitless, never global, and a class rendered at two sizes needs two pins.

### M8. O-SpectralShaper has a LIVE canvas text carrier — the first in three waves

Wave 4a's finding 5 flagged canvas text as a class; wave 4b measured 0 across all six with the
control fired. Here it is live. Three `fillText` sites, and the discriminator matters:

- `CurveEditor.js:206` and `:223` draw **numeric axis ticks** (`500`, `2k`, `+12dB`) with the
  font `"9px Garamond, 'Times New Roman', serif"`. TNR is installed, so the Latin is safe and
  nothing translates. **An evidenced non-finding.**
- `Spectrogram.js:422`, inside `fallbackToCanvas2D()`, draws **one hard-coded English sentence**
  onto the canvas. It is invisible to `check-i18n` (assertion 10 walks text nodes, 12 scans
  `textContent`/`innerText` writes — neither reaches `fillText`), invisible to
  `check-ui-labels`, and invisible to `measure-ui`. **It went out in French untranslated too**,
  which is exactly the shape wave 4b's C7 is about.

**DECISION: localize it, following O-Comp's shipped precedent.** O-Comp hit this and solved it:
the string lives in `I18N` with an **empty body**, is read through `trLabel()` from inside the
render path, and is deliberately kept out of `LABELS` because assertion 15 fails a LABELS key
that no element and no `setLabel` call reaches. That precedent is documented in O-Comp's own
`i18n.js`. Reproduce it here. The cost is one entry.

The verification problem is real and is solved by asserting on the SOURCE rather than the
paint: the branch only runs when WebGL is unavailable, which cannot be produced on this
machine. The check is that `Spectrogram.js` no longer calls `fillText` with a string literal at
all — a negative grep on the call shape, not on the sentence.

### M9. O-Bassoon's tuning tab is out of reach of its own table, and its copy ALREADY says so

Measured: `plugins/O-Bassoon/CMakeLists.txt:101` embeds
`modules/tuning/scala-tuning-engine/js/tuning-panel.js` directly. That shared file is 1041
lines and holds **0 `data-i18n`, 0 `applyI18n`, 0 `I18N`, 0 `data-tip`, 0 `__setLabel`, 0
`'fr'`**. Five plugins embed it: O-Bassoon, O-Bowed, O-Contrabass, O-Reed, O-Wind. And
O-Bassoon's `tests/i18n-states.json` drives only `about tab` and `settings popover open` — the
Tuning tab never renders under any gate or measurement in this repo, so nothing would report it
either way.

**DECISION: mirror the French precedent. Leave the shared panel exactly as found, and record it
as a deferred item naming all five consumers as the blast radius.** Three independent reasons:
the panel has no hooks in any language; editing it is a five-plugin repo-wide change with its
own build and auval budget; and O-Bassoon's own language hover-help **already discloses the
situation in both en and fr** — it states that the Tuning tab stays in English because its panel
comes from a shared module that is not part of this plugin. The zh row carries the same
disclosure. That sentence is the French precedent, written into the shipped copy, and reading it
is how this was settled rather than assumed.

### M10. O-TextureForge's bundle carries no copy — and its own gate proves it

Settled by reading, and the reading is cheap because the gate already asserts it. At
`tests/ui_tip_render_check.js:276–284` the gate checks that `setupTooltips` is defined in
`js/i18n_init.js` and **absent from the webpack input `src/app.js`**, with the stated reason
that putting the renderer in the bundle would ship the label table twice and force a webpack
rebuild inside every copy change. Confirmed independently: `js/app.bundle.js` contains **0
occurrences** of `applyI18n`, `data-i18n` or `data-tip`, and every localized string in
`src/app.js` goes through `window.__setLabel(el, 'key')` against `js/i18n.js` — spelled in full
on purpose, because terser mangles a local wrapper and `check-i18n` assertion 13 would then see
nothing.

**DECISION: no webpack rebuild for the table.** The i18n pass is entirely outside the bundle,
and the control that proves it is the gate's own assertion, which is already in CI-adjacent use.

**One caveat, in scope to measure and conditional to fix.** `src/app.js:451` builds a
`#file-size-overlay` at runtime whose wrapper carries an inline `font-family:inherit` and which
holds **two `<button>` elements** whose captions are set through the label table. Buttons do not
inherit `font-family`, so those two are form-4 carriers created by the bundle — and the plugin's
state file has **one** state, so nothing drives that overlay and `measure-ui` cannot see it.
Add a state that opens the overlay, measure, and repair **only if the measurement says so**. If
it does, `src/app.js` changes and the rebuilt `app.bundle.js` must be committed with it; the
control that the shipped bundle matches its source is a rebuild that leaves `git diff` on the
bundle empty when the source is unchanged.

### M11. O-Bells' two late bindings are PINNED BY ITS OWN GATE as designed behaviour

`tests/ui_tip_render_check.js:462` reads
`const EXPECTED_LATE = ['#ref-pitch-knob', '#octave-stretch'];` and assertion `[1b]` requires
the warning set to equal it exactly — "the only warnings are the two lazily-mounted
tuning-panel anchors … they bind on the panel's own `window.__reapplyI18n()`; the warning is
the whole cost."

**DECISION: leave them.** Three reasons, and no replacement control is needed:

1. They are the boot-all-uis census control (wave 4b D4), and every `--strict-tips` run in this
   wave will be checked to still report exactly 2 — a run reporting 0 would mean the census had
   gone blind.
2. The gate PINS them, so "fixing" them means editing an assertion that currently documents the
   design. That is a change to a shipped contract, not a bug fix.
3. Wave 4b's C9 records that the stated consequence of a late binding was **false** on
   O-IntonationPad — all 80 anchors carried a tip at settle anyway. Measure the consequence
   before budgeting the fix; here the gate's own text already asserts the anchors bind.

If the executor observes a real consequence — a tuning-panel anchor carrying no Chinese tip at
settle — that is a new finding and goes in `deferred-items.md`, not into an unbudgeted fix.

### M12. Four of six gear bodies are false; two are correct — check both, never assume

Read in full, en and fr, on all six:

| plugin | gear/settings body | verdict |
|---|---|---|
| O-Bells | "It holds one control: the interface language." | **FALSE** |
| O-Detune | "It carries a single control: the language…" | **FALSE** |
| O-Bassoon | "That is all it holds…" | **FALSE** |
| O-TextureForge | "It holds one control, the interface language." | **FALSE** |
| O-Gain | names the language **and** the hover-help switch | **correct — leave it** |
| O-SpectralShaper | names both, "Both choices are remembered" | **correct — leave it** |

`check-i18n` assertion [16] passes on all 43, and it requires every plugin with a language
selector to carry exactly one hover-help switch, bound and keyed — which is precisely what
makes the four false. All six carry the fixed-pair language enumeration and all six take that
deletion. Wave 4b's ratio was 5 false / 1 correct; this wave's is 4 / 2. **The rule is check
both, and record a checked non-defect as checked** — an unexamined correct body and an
unexamined false one look identical in a diff.

### Tooling traps confirmed live during this planning pass

- `system_profiler SPFontsDataType | grep -c "Family: X$"` is the installed-family measurement.
  Do not infer availability from a stack's contents.
- `node -e "require('/tmp/mu-<P>.json')"` over `measure-ui`'s stdout is the census; **stderr
  carries the screen counts and stdout carries nothing but JSON**, so the two must be
  redirected separately or the JSON is unparseable.
- `O-Bassoon`'s editor is written `setSize (900, 600)` with a space before the paren. Both
  `check-ui-labels` and `measure-ui` parsed it correctly — verified, 340 rows measured at the
  right frame — but the space form exists and a hand-rolled parser keyed on `setSize(` misses it.
- C11 still holds and was respected throughout: **`timeout` does not exist on this machine**,
  **zsh does not word-split an unquoted variable**, and **every `--include` glob must be
  quoted** or the grep fails entirely and the count reads as a clean zero.

---

## Stage-4 executor rules — non-negotiable, cited by number below

Carried verbatim from Stage 3 (R1–R9) and wave 4a (W1–W5), with wave 4b's C-items folded in.

- **R1 — Author at `'mt'`, always.** Never write `'bt'` at authoring time. `check-i18n` accepts
  `'bt'` as a valid enum member and the lint's R1 only reports entries *below* the bar, so a
  premature `'bt'` is invisible to every automated check in this repo. Promotion happens only
  after the reverse read, in its own commit.
- **R2 — Read ALL triples with `--verbose`.** `--ingest` truncates to 12. Across three prior
  waves essentially no real finding ranked inside the default twelve. O-Bells' 255 rows make
  twelve 5% of the batch. The score is a sort key, not a verdict.
- **R3 — Screen `TERMS` for page collisions BEFORE authoring.** Report any two *different*
  English keys mapping to one root that co-occur on a page. Qualify **both** sides, never one.
- **R4 — Fresh blind reader AND fresh salt per correction round.** They guard different things:
  the salt stops correlation against seen ids, the fresh agent stops self-agreement. Per C10, a
  correction round also needs a **different model** — a fresh session of the same model guards
  correlation but not self-agreement.
- **R5 / W4 — `--emit` is CORPUS-scoped and its own usage line is wrong. Pass BOTH `--emit
  <Plugin>` AND `--plugin <Plugin>`.** Following the usage line emits the entire corpus — 1830
  already-shipped rows dispatched to an external reader as this plugin's work. `emit()` records
  the `--emit` argument as the manifest target and filters nothing; the row filter is `--plugin`.
- **R6 — Pass `--manifest` explicitly and fire the blinding controls per batch.** ids pure
  12-hex, no key fragment, ids returned identical and in order, rows well-formed with no Han
  surviving in the returned English, and a sha256 of the zh column against the committed tree.
  **Forbid repo access in the dispatch explicitly** — O-Bassoon and O-Bells name themselves in
  their own copy, and O-Bassoon's product name is one of the two bare-generic nodes (M6b).
  Per C10, `--forward-provenance` is **REQUIRED at emit**: without it `--ingest` refuses rather
  than reporting, and that refusal is correct.
- **R7 — zh geometry failures are SHRINKS. Assert equality, pin at the widest of three.**
- **R8 — A French-era `min-width` pin is a FLOOR, not safety.** Re-measure every one. Grep for
  existing declarations on a selector before appending, or a new pin at higher specificity
  silently un-pins a French one and moves the **fr** arm. Counts per plugin are in the table.
- **R9 — Version truth is `CMakeLists.txt`.** Read the value; never grep for a literal; the
  value may be quoted. None of this wave's six is quoted — measured — but read the value anyway.
- **W1 — The CJK tail goes BEFORE the trailing generic.** Chromium resolves a bare `serif` or
  `sans-serif` against the document's `lang`, so under `zh-Hans` the generic is already a
  Chinese face and a tail written after it is never consulted. The shipped convention is
  `…, 'PingFang SC', 'Microsoft YaHei', serif`, with the sans and mono cases placing the tail
  before their own generic.
- **W2 — OR the visibility flag across states.** `measure-ui` already does this and discloses
  the identity ratio on every run; any hand analysis must too.
- **W3 — Check `I18N_EXEMPT` membership BOTH ways.** Every exempt option word in an English
  body must survive verbatim into the Chinese, and every non-exempt caption must not. Units in a
  range clause follow the READOUT, not the prose. Exempt counts: O-Bells 18, O-Detune 15,
  O-TextureForge 14, O-Gain 11, O-Bassoon 4, O-SpectralShaper 2.
- **W5 — Expect a Z8 violation after any Latin→Han token swap.** The space in `显示 ON，` is
  correct while the token is Latin and becomes an intra-Han space the moment it is replaced.
  Under the flipped gate this blocks.
- **C3 — Measure at the SHIPPING FRAME and walk the states CUMULATIVELY.** `measure-ui` parses
  the frame from each plugin's own `PluginEditor.cpp` and applies states in file order with no
  reset. Do not substitute a hand-rolled sweep at a chosen viewport.
- **C6 — DERIVE-OR-ABORT on every repaired gate, and fire the control.** A derived list that
  goes empty walks zero languages and reports every assertion green. Apply the guard to the
  WALK as well as the assertion (M2). Revert the plant by **targeted edit, never
  `git checkout --`**, and confirm sha256 byte-identity against HEAD.
- **C7 — Author the new language from the CORRECTED English.** Make the enumeration deletions
  first, **re-read the file**, then author from the file as it then stands. No tool in this repo
  compares a `zh` body against its own `en`; only the reverse read catches a row that outlives
  the sentence it was translated from.
- **C8 — Do not spell the superseded literal in the comment that explains it.** Word it as
  prose, or this wave's own stale-enumeration probe keeps reporting a plugin that is fixed.
- **Comment-blind greps lie.** Strip comments before any code count:
  `perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' <file>`.
- **`grep -P` with a Han property byte-matches on this machine** and reports phantom hits on the
  middle dot U+00B7. Use the `perl -CSD` form in the verify blocks. A Han script property in
  perl **without** `-CSD` is inert.
- **Two nodes a keyed scan always misses:** the `#tooltip` surface (filled from `data-tip` at
  hover time, never keyed) and the endonym `<option>` (the only Han in the markup). Universal.

---

## The per-plugin procedure (nine steps, identical for all six)

Every task runs this loop. Written once here, referenced, not repeated.

1. **Screen for glossary page collisions (R3)** — before authoring a string. Push the plugin's
   English label set through `TERMS`; report any two different keys sharing a root that co-occur
   on a page. Then run the **mechanical downstream check**: after authoring, group the zh
   renderings and report any two *different* keys sharing one, excluding same-control pairs (a
   caption and its own tooltip title) and same-English pairs. The R3 screen alone only fires
   when both sides are glossary keys and is blind otherwise.
2. **Delete the stale enumerations FIRST (C7, M12).** In `js/i18n.js`, remove the sentence that
   names or counts the selector's options from the language hover-help in the **en body and the
   fr body both**, and — on the four where it is false — remove the clause in the gear body that
   asserts the panel holds nothing besides the language. **Both are deletions, never
   extensions**; an enumeration is false again the next time an option lands, and the selector
   already lists the languages in their endonyms. On O-Gain and O-SpectralShaper the gear body
   is correct: **read it, leave it, and record it as a checked non-defect.** Then re-read the
   file before step 3.
3. **`js/i18n.js` — the table.** Add `'zh-Hans'` to `LANGUAGES` and a `'zh-Hans'` value on
   **every** key in both `I18N` and `LABELS`, all at `reviewed: 'mt'` (R1), authored from the
   file as it stands after step 2.
4. **`index.html`** — the endonym `<option value="zh-Hans">` beside the fr option, written as
   **numeric character references** to match each file's existing convention. Verify the
   convention per plugin rather than assuming it; four of six use the entity form for `Français`.
5. **Font work, in two separable halves (W1, M4, M5, M6).** Serve the page, switch to Chinese,
   and run `node scripts/measure-ui.js --plugin <Name> --mode box --report all` — **after** the
   table lands, because before it the Han-gated screens are a vacuum (M1). Then:
   - **the tail half**, on stacks that carry or can receive Han: append before the trailing
     generic, at the sites named in M5;
   - **the face-naming half**, on the three bare-generic declarations (M4) and only there: name
     an installed face, **no tail**, because those nodes hold no Han;
   - **the form-4 half**, on the nodes `undeclared-font` names: a rule with **Arial FIRST** then
     the tail, so en/fr Latin metrics do not move.
   Record which sites took which treatment and why. A wave that appends the tail to everything
   it measured can no longer tell a needed tail from a decorative one.
6. **`PluginProcessor.h`** — widen the two-way `languageCode` / `languageIndex` ternary to
   three-way, pure ASCII. Zero Chinese characters anywhere under `Source/`.
7. **Gate repair, both sites (M2, C5, C6)** — where a gate file exists: derive the language list
   from the table's own `LANGUAGES` export **and** restructure the two literal `sweep()` call
   sites (or the array literal) into a loop over that derived list. Fire the derive-or-abort
   control against the walk. On O-Detune additionally rewrite the direction-specific `[5]` as a
   difference. Word the explanatory comment as prose (C8).
8. **Geometry pass (R7, R8, M7)** — run the zh arm of `check-ui-labels` and let it *name* the
   offenders, plus `measure-ui --report line-height-normal` and `--report wrap-count`. Expect
   four shapes: `line-height: normal` inheritance (pin each named leaf at its measured English
   line box, derived from the box, unitless, never global; a class at two sizes needs two pins);
   a content-sized element that shrank and pulled its neighbours; a Han run whose min-content
   width is one character in an auto-sized table or grid (`nowrap` **plus** a min-width); and
   Latin display conventions applied to Han, where `letter-spacing` and generous side padding
   are trimmed under `html[lang="zh-Hans"]` **with the trim value MEASURED**. Re-run until 0 FAIL
   on **all three** arms.
9. **Commit at `'mt'`, blind reverse read, promote and ship.** Path-scoped
   `git commit -- plugins/<Name>` — the reverse pass runs against a committed tree. Then
   `--emit <Plugin> --plugin <Plugin>` to a fresh `--out` with `--forward-provenance`, dispatch
   to a blind reader per C10 (`claude -p "<prompt>" --model <model> --allowed-tools ""` from a
   cwd **outside** the repo, with repo access forbidden in the prompt), `--ingest` with an
   explicit `--manifest` and `--verbose`, batching tables above ~92 rows into chunks. Read
   **every** triple. For each drift the discriminator is **collision on the page**, not drift
   distance: can a reader on this page confuse this string with another control on the same
   page? A caption must also not collide with its own tooltip title. Re-author on collision;
   accept otherwise, and **write the reason down**. Then flip to `reviewed: 'bt'`, bump the
   version from the CMakeLists value (R9), write the CHANGELOG entry naming the disclosed
   quality level, and `./scripts/build-and-install.sh <FolderName>` — the script resolves the
   `juce_add_plugin` target itself (verified in the source at L129–159), so pass the **folder**
   name even on O-TextureForge where the target differs.

**Deferred to the end of the batch:** `auval`. A cold `auval` after an install rebuilds the
whole AU registry (~15 min); six separate runs would cost six rescans. Build and install all
six, then sweep once.

**`reviewed: 'native'` stays OPEN on all six and is not a blocker.** This project has no native
Chinese reader. A disclosed quality level, printed by lint rule R1 on every run and stated in
all six CHANGELOGs.

---

## Source coverage audit

| # | Source | Item | Covered by |
|---|---|---|---|
| 1 | DESC | O-Bells localized (255 rows, largest) | Task 2 |
| 2 | DESC | O-Gain localized | Task 3 |
| 3 | DESC | O-SpectralShaper localized | Task 3 |
| 4 | DESC | O-Detune localized | Task 1 |
| 5 | DESC | O-Bassoon localized | Task 3 |
| 6 | DESC | O-TextureForge localized | Task 3 |
| 7 | DESC | Author at `'mt'`, blind reverse read, promote to `'bt'` | Tasks 1–3, step 9 |
| 8 | DESC | Drive UI states, tail font stacks, fix geometry | Tasks 1–3, steps 5 and 8 |
| 9 | DESC | Minor version bump from the CMakeLists value, build + install | Tasks 1–3, step 9 |
| 10 | DESC | End-of-batch cold auval sweep, once | Task 3 close-out |
| 11 | DESC | PLUGINS.md once at the end, six rows | Task 3 close-out |
| 12 | DESC | Exit criterion: all six green, no partial waves | `<verification>` |
| 13 | CONSTRAINT | Live-observation authority (paths, versions, gates, fonts read now) | Live observations table; M1–M12 |
| 14 | CONSTRAINT | Path-scoped commits only, never `git add -A` | Every task; Task 3 commit discipline |
| 15 | CONSTRAINT | Variant 1 decided (shared tuning panel) | M9 — DECISION recorded |
| 16 | CONSTRAINT | Variant 3 decided (O-Bells late bindings) | M11 — DECISION recorded |
| 17 | CONSTRAINT | Variant 4 decided (O-TextureForge bundle) | M10 — DECISION recorded |
| 18 | READING | Wave 4b C1–C11 carry-forward baked into task instructions | Executor rules; steps 2, 5, 7 |
| 19 | READING | Wave 4a findings 1–13 and Stage-3 R1–R9 | Executor rules section |
| 20 | READING | `measure-ui.js --report all` is the census; no scratch harness | Step 5; M1 |
| 21 | READING | Project skills checked — `.claude/skills/` holds no i18n or build rule; CLAUDE.md's cache-clear and commit discipline are the governing rules | Task 3 close-out; step 9 |

No item is uncovered. Excluded and stated: O-Chorus's inert CJK tail (wave 4b D3 — its own
plugin's pass), the three glossary roots that failed a reverse read (wave 4b D2 — reported to
the glossary owners; editing a settled root is repo-wide), O-IntonationPad's missing tip gate
(D6), the `Z6` budget backfill (D5), and the shared `scala-tuning-engine` tuning panel (M9 —
five-plugin blast radius, deferred with its consumers named).

---

<tasks>

<task type="tracer">
  <name>Task 1: O-Detune end-to-end — the thinnest plugin that touches every layer (ZH4C-01, ZH4C-07, ZH4C-08)</name>
  <files>plugins/O-Detune/Source/ui/public/js/i18n.js, plugins/O-Detune/Source/ui/public/index.html, plugins/O-Detune/Source/PluginProcessor.h, plugins/O-Detune/tests/ui_tip_render_check.js, plugins/O-Detune/CMakeLists.txt, plugins/O-Detune/CHANGELOG.md</files>
  <precondition>`node scripts/i18n-zh-lint.js --self-test` reports 10/10, `node scripts/check-i18n.js` exits 0 on 43 plugins, `node scripts/i18n-zh-lint.js` exits 0 with 0 findings, and `node plugins/O-Detune/tests/ui_tip_render_check.js` exits 0 on the tree as found. If any fails, stop — every zero this task produces would be unevidenced, and the gate repair's control could not be distinguished from a pre-existing defect.</precondition>
  <reversibility rating="reversible">One plugin's table, markup, C++ ternary and gate file. Every change is path-scoped to `plugins/O-Detune` and revertible with `git restore --source=<sha> -- plugins/O-Detune`.</reversibility>
  <action>
Run the full nine-step procedure on **O-Detune**. It is the tracer because it is the thinnest
plugin that touches every layer this wave modifies, and because it is the only one carrying the
two gate defects that the other three gate files carry between them.

71 rows, 2 states, inline-module controller, no external CSS, `Source/ui/public/`, version
1.8.1 at line 16, `juce_add_plugin` target equal to the folder name, 4 min-width / 5
line-height / 11 letter-spacing declarations existing, 15 `I18N_EXEMPT` entries.

Four things it proves for the other five. Each must be **recorded for reuse**, because Tasks 2
and 3 inherit the method rather than re-deriving it:

**The two-site gate repair (M2).** Its `tests/ui_tip_render_check.js` names no `LANGUAGES`
anywhere in the file — its language walk is a three-element array literal at line 467, the form
the Stage-3 census cannot see. Repair by **deriving** the walk from the table's own `LANGUAGES`
export, and fire a derive-or-abort control against the walk itself: a planted empty export must
make the gate exit non-zero rather than sweep zero languages and report green. Revert the plant
by targeted edit and confirm the file is byte-identical to HEAD by sha256. The other three
gates additionally have a joined-pair assertion that this one lacks; the walk repair proved here
is the half that all four share.

**The direction assertion (M3, C5).** Its assertion `[5]` at line 523 requires the non-English
pass to be strictly taller than English on at least one tip. Chinese shrinks tip height, so this
hard-fails the zh arm on a correct page. Rewrite it to assert a **difference** — that the
non-English pass measured something other than English — evaluated per non-English language off
the derived list. If no tip's height differs from English, that pass is the same measurement
twice and its half of the clamp assertion is decoration, which is what the assertion is for.

**The tail-only font repair, both stack shapes, no face-naming anywhere (M4, M5).** All nine of
its declarations are in `index.html`: five read `'Georgia', 'Times New Roman', serif` (lines 57,
333, 801, 832, 911) and four read `'Georgia', serif` (lines 555, 601, 670, 719). Georgia is
installed on this machine — measured, 4 family matches — so **every one of them is already
Latin-safe and none needs a face named**. They need the tail and nothing else. The four shorter
stacks are the `#gear-btn`, `#prevPreset`, `#nextPreset`, `#loadPreset` and `#savePreset`
controls, and the last two carry localized captions, so the tail there is load-bearing rather
than precautionary. This is the clean case that makes the two halves of the font work
distinguishable; record it as the reference for the plugins where they are not.

**The single form-4 node.** Exactly one node on this page resolves to the UA stylesheet's face
rather than to anything the page declares: the `#width` input. Repair with Arial kept first,
then the tail, so the en and fr Latin metrics do not move. One node is the smallest possible
instance of the class that costs O-Bells 32 nodes, which is why proving the repair shape here
is worth more than its own size.

Then the rest of the loop in order: screen `TERMS` (R3); **delete the enumerations first** —
the language body's fixed-pair phrase in en and fr, and the gear body's single-control clause in
en and fr, both of which are false since the hover-help switch landed (M12) — then re-read the
file and author from the corrected English (C7); author at `'mt'` (R1); add the endonym option
in the file's own numeric-reference convention; widen the ternary; pin geometry after grepping
the four existing min-width declarations for specificity conflict (R8); commit at `'mt'`; run
the blind reverse pass per C10 with `--forward-provenance`, an explicit `--manifest`,
`--verbose`, a fresh salt, **both** `--emit` and `--plugin` (W4), and repo access forbidden;
read every triple; promote in a second commit with the version bumped from the CMakeLists value
to 1.9.0 (R9) and the CHANGELOG entry; build and install with
`./scripts/build-and-install.sh O-Detune`. **Do not run auval** — it is deferred to Task 3.
  </action>
  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10 before any zero is trusted</automated>
    <automated>node scripts/check-i18n.js --plugin O-Detune >/dev/null 2>&1; echo "check-i18n exit=$?"   # 0, LANGUAGES lists three</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-Detune >/dev/null 2>&1; echo "check-ui-labels exit=$?"   # 0, 0 FAIL on the en, fr and zh arms</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-Detune 2>&1 | tail -12   # 0 findings, BELOW SHIP BAR 0</automated>
    <automated>node scripts/i18n-fr-lint.js >/dev/null 2>&1; echo "fr-lint exit=$?"   # 0 — French untouched</automated>
    <automated>node plugins/O-Detune/tests/ui_tip_render_check.js >/dev/null 2>&1; echo "tip-gate exit=$?"   # 0, with a real zh arm swept</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/O-Detune/tests/ui_tip_render_check.js | grep -cE "\[ *'en' *, *'fr'|\"en\" *, *\"fr\"|=== *'en,fr'"   # 0 — no two-language literal in any syntax, comments stripped</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/O-Detune/tests/ui_tip_render_check.js | grep -cE "h(Fr|Zh)\[[A-Za-z]+\] *> *hEn"   # 0 — the strict one-directional height comparison is gone</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-Detune/tests/ui_tip_render_check.js | grep -c "LANGUAGES"   # >= 2 — the walk AND its guard both read the table's export</automated>
    <automated>find plugins/O-Detune/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +   # MUST print nothing</automated>
    <automated>perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-Detune/Source/ui/public/js/i18n.js   # positive control for the line above — MUST print the file</automated>
    <automated>perl -0777 -ne 'print "STALE ENUMERATION: $&\n" while /(English and Fran|English or Fran|English et Fran|anglais et le fran|between English and|a single control|carries a single)/gi' plugins/O-Detune/Source/ui/public/js/i18n.js   # MUST print nothing, en and fr both</automated>
    <automated>grep -c "PingFang SC" plugins/O-Detune/Source/ui/public/index.html   # >= 9 — the tail landed on every measured stack</automated>
    <automated>grep -coE "font-family: *'Georgia', *serif *;" plugins/O-Detune/Source/ui/public/index.html   # 0 — the four short stacks all carry a tail now</automated>
    <automated>node scripts/measure-ui.js --plugin O-Detune --mode box --report all 2>&1 1>/dev/null | grep -E "finding|carrier|identity"   # undeclared-font 0 with a NON-ZERO identity count, so the 0 is measured (M1)</automated>
    <automated>node -e "const r=JSON.parse(require('fs').readFileSync('/dev/stdin','utf8')); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); console.log('zh visible Han-bearing nodes:', z.length, '| resolving to no CJK face:', z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff)).length)" < <(node scripts/measure-ui.js --plugin O-Detune --mode box 2>/dev/null)   # first number > 0 (input non-empty), second 0</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-Detune --strict-tips 2>&1 | tail -6   # 0 DEAD, 0 late, 0 failed</automated>
    <automated>perl -ne 'print "$1\n" if /^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ && !$d++' plugins/O-Detune/CMakeLists.txt   # 1.9.0 — read as a value</automated>
    <human-check>The gate's derive-or-abort control fired against the WALK, not only the assertion: a planted empty LANGUAGES export makes the gate exit non-zero, and the reverting edit leaves the file byte-identical to HEAD by sha256.</human-check>
    <human-check>Every one of the ~71 back-translation triples read with --verbose. Each accepted drift carries a written collision-on-the-page reason.</human-check>
    <human-check>Which of the nine font sites took the tail and why is recorded, together with the finding that none needed a face named because Georgia is installed — the reference case for Tasks 2 and 3.</human-check>
  </verify>
  <done>
O-Detune ships three languages at `reviewed: 'bt'` on all 71 rows, version 1.9.0 read from the
CMakeLists value, CHANGELOG written, built and installed, geometry equal on all three arms. Its
gate file derives BOTH its language assertion and its language walk from the table's own export
with the derive-or-abort control fired against the walk, holds no two-language literal in any
syntax with comments stripped, and asserts a height difference rather than a direction. All nine
of its font sites carry the tail before their generic; its one form-4 node names Arial first.
Both stale enumerations are deleted in en and fr, and the zh rows were authored from the file
as it stood after the deletions. Two or more path-scoped commits: the `'mt'` table, then the
promotion. The reusable findings — the two-site gate repair, the difference assertion, the
tail-only font case, the blind-dispatch shape and the two control procedures — are written down
for Tasks 2 and 3.
  </done>
</task>

<task type="auto">
  <name>Task 2: O-Bells — 255 rows, 14 states, its own tuning panel, 32 form-4 nodes, 584 normal line boxes (ZH4C-02, ZH4C-08, ZH4C-09)</name>
  <files>plugins/O-Bells/Resources/ui/js/i18n.js, plugins/O-Bells/Resources/ui/index.html, plugins/O-Bells/Resources/ui/css/tuning-panel.css, plugins/O-Bells/Resources/ui/js/tuning-panel.js, plugins/O-Bells/Source/PluginProcessor.h, plugins/O-Bells/tests/ui_tip_render_check.js, plugins/O-Bells/CMakeLists.txt, plugins/O-Bells/CHANGELOG.md</files>
  <precondition>Task 1's commits are in the tree and its recorded findings are available — this plugin reuses the two-site gate repair, the derive-or-abort control shape, the font-treatment discriminator and the blind-dispatch shape the tracer proved. `node plugins/O-Bells/tests/ui_tip_render_check.js` exits 0 on the tree as found, and `node scripts/boot-all-uis.js --plugin O-Bells --strict-tips` reports exactly 2 late and 0 dead — the census control this whole wave leans on.</precondition>
  <action>
Run the nine-step procedure on **O-Bells** — the heavy plugin of the wave, and it gets this task
to itself for that reason. 189 entries / **255 rows**, **14 states**, `Resources/ui/`, an
external `css/tuning-panel.css`, an inline-module controller, `IS_SYNTH TRUE`, 66 tip bindings,
18 `I18N_EXEMPT` entries, version 4.4.1 at line 11 → **4.5.0**.

Six things make this plugin different from the tracer, and each is a measured trap.

**It carries its OWN 1075-line `js/tuning-panel.js`, not the shared module.** Measured: 45
`data-i18n` hooks and 8 `applyI18n` calls, and it differs from
`modules/tuning/scala-tuning-engine/js/tuning-panel.js` byte-for-byte. So unlike O-Bassoon
(M9), O-Bells' tuning panel **is** in reach of its own table and is in scope. Its captions are
part of the 255 rows. The file itself only changes if a caption there is unkeyed — check, and
if every caption is already keyed, record that as a checked non-finding rather than editing.

**Fourteen states, and the copy lives behind tabs.** The state file drives a bloom-fine toggle,
the settings popover open and shut, the **tuning tab**, the library and generator sections,
three generator types, three visualisation modes, a held-notes eval and the effects tab. Every
one must be driven for the font measurement and the geometry pass, cumulatively and with the
visibility flag OR-ed across states (W2, C3). `measure-ui` already does both; do not substitute
a hand-rolled sweep. Measured now at the shipping 800 × 600 frame: 2146 nodes, 1073 distinct DOM
keys, 247 distinct display ids — so any analysis keyed on a display id sees roughly a quarter of
the page (this is exactly the defect the README's identity disclosure exists to surface).

**Thirty-two form-4 nodes, and six existing `inherit` declarations that do NOT cover them
(M6a).** 13 `<button>`, 17 `<input>`, 2 `<select>` resolve to the UA stylesheet's face — every
one in the tuning panel, and every one a node this task translates. `index.html` already carries
`font-family: inherit` at six sites (L256, L359, L991, L1044, L1071, L1378), which is the same
repair applied to a different set of controls; it proves the class was known and does not close
it. No grep in this repo can see the 32. Repair with **Arial kept FIRST** then the tail, so
Latin metrics on the en and fr arms do not move, and verify by re-running the
`undeclared-font` screen rather than by reading the CSS.

**The wave's only bare generic that also needs a tail (M4, M6c).** `css/tuning-panel.css:358`
reads `font-family: monospace` — names nothing at all, so under `zh-Hans` even the cents
readouts' digits are resolved by the document language. Three `span.tk-cents` nodes render
through it today. Name a mono face **and** put the tail before the mono generic; this is the one
site in the wave where both halves of the font work land on one declaration. The other three
sites (`index.html:57`, `:1262`, `css/tuning-panel.css:54`) already name Times New Roman or
resolve `-apple-system` on macOS and take the tail only.

**584 `line-height: normal` visible leaves — ten times the wave's median and ten times
O-IntonationPad's repaired baseline (M7).** This is the largest geometry risk in the wave. 584
is the upper bound, not the worklist: only Han-carrying leaves reach the screen. Run
`--report line-height-normal` **after** the table lands to get the real number, then pin each
named leaf at its measured English line box — derived from the box, unitless, never global, and
a class rendered at two sizes needs two pins. Note also 23 `letter-spacing` declarations, the
highest in the wave: Latin display conventions applied to Han need a **measured** trim under
`html[lang="zh-Hans"]`, never a value derived from the glyph advance.

**Its two late tip bindings are PINNED BY ITS OWN GATE and stay (M11).** Assertion `[1b]` at
line 462 asserts the warning set equals `EXPECTED_LATE` exactly, and the surrounding text
documents the behaviour as designed — the anchors bind on the panel's own re-apply. Leave them.
They remain this wave's boot-all-uis census control, and every `--strict-tips` run in Tasks 2
and 3 must still report exactly 2, or the census has gone blind rather than anything having been
fixed. If a real consequence is observed — a tuning-panel anchor carrying no Chinese tip at
settle — that is a new finding for `deferred-items.md`, not an unbudgeted fix.

Its gate file additionally carries the on-census joined-pair assertion at line 314 **and** two
literal walk call sites at 689 and 800 (M2). Repair both, exactly as the tracer did, with the
derive-or-abort control fired against the walk. It has no direction-specific assertion — the
`tallestEn`/`tallestFr` pair at 689/800 feeds a `console.log` only; **read it and leave it**,
and record that as a checked non-defect.

Then the rest of the loop: screen `TERMS` first (R3), and expect collisions to be likelier here
than anywhere in the wave because the tuning vocabulary and the bell-resonator vocabulary share
roots; **delete both stale enumerations first** — the language body's fixed-pair phrase and
the gear body's single-control clause, in en and fr — then re-read and author from the corrected
English (C7); author at `'mt'` (R1); endonym option in the file's own convention; widen the
ternary; commit at `'mt'`; blind reverse read in chunks of roughly 92 rows (C10 — 255 in one
dispatch is a long single response) with a fresh salt, `--forward-provenance`, an explicit
`--manifest`, `--verbose`, both `--emit` and `--plugin`, and repo access forbidden — this plugin
names itself in its own copy; read every triple; promote, bump to 4.5.0, CHANGELOG, and
`./scripts/build-and-install.sh O-Bells`. **Do not run auval.**
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --plugin O-Bells >/dev/null 2>&1; echo "check-i18n exit=$?"</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-Bells >/dev/null 2>&1; echo "check-ui-labels exit=$?"   # 0, 0 FAIL on all three arms across all 14 states</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-Bells 2>&1 | tail -12   # 0 findings, BELOW SHIP BAR 0</automated>
    <automated>node scripts/i18n-fr-lint.js >/dev/null 2>&1; echo "fr-lint exit=$?"   # 0 — the fr arm must not have moved</automated>
    <automated>node plugins/O-Bells/tests/ui_tip_render_check.js >/dev/null 2>&1; echo "tip-gate exit=$?"   # 0, with a real zh arm swept</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/O-Bells/tests/ui_tip_render_check.js | grep -cE "=== *'en,fr'|\[ *'en' *, *'fr'|sweep\('fr'\)"   # 0 — assertion AND walk both derived</automated>
    <automated>find plugins/O-Bells/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +   # MUST print nothing</automated>
    <automated>perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-Bells/Resources/ui/js/i18n.js   # positive control — MUST print the file</automated>
    <automated>perl -0777 -ne 'print "STALE ENUMERATION: $&\n" while /(English and Fran|English or Fran|anglais et le fran|holds one control|une seule commande)/gi' plugins/O-Bells/Resources/ui/js/i18n.js   # MUST print nothing</automated>
    <automated>find plugins/O-Bells/Resources/ui \( -name '*.html' -o -name '*.css' \) -exec grep -hoE "font-family: *monospace *;" {} + | wc -l | tr -d ' '   # 0 — the wave's only naked generic now names a face</automated>
    <automated>find plugins/O-Bells/Resources/ui \( -name '*.html' -o -name '*.css' \) -exec grep -c "PingFang SC" {} + | awk -F: '{s+=$NF} END{print s+0}'   # > 0 in BOTH index.html and css/tuning-panel.css — the split-file sweep reached the stylesheet</automated>
    <automated>node scripts/measure-ui.js --plugin O-Bells --mode box --report all 2>&1 1>/dev/null | grep -E "undeclared-font|line-height-normal|wrap-count|identity"   # undeclared-font 0 — was 32 nodes on Arial before the repair</automated>
    <automated>node -e "const r=JSON.parse(require('fs').readFileSync('/dev/stdin','utf8')); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); console.log('zh visible Han-bearing nodes:', z.length, '| no CJK face:', z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff)).length, '| lh normal leaves:', z.filter(x=>x.lh==='normal'&&x.kids===0&&x.own.trim()).length)" < <(node scripts/measure-ui.js --plugin O-Bells --mode box 2>/dev/null)   # first > 0 (non-empty input), second 0, third 0</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-Bells --strict-tips 2>&1 | grep -iE "late|dead"   # exactly 2 late (#ref-pitch-knob, #octave-stretch), 0 dead — the pinned census control, unchanged</automated>
    <automated>perl -ne 'print "$1\n" if /^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ && !$d++' plugins/O-Bells/CMakeLists.txt   # 4.5.0 — read as a value</automated>
    <human-check>Every one of the 255 back-translation triples read with --verbose, in chunks of roughly 92 rows. Each accepted drift carries a written collision-on-the-page reason.</human-check>
    <human-check>The two late bindings were read in the gate's own EXPECTED_LATE assertion and deliberately left — recorded as a checked non-defect with the gate line cited, not as an unchecked one. The same run still reports them, so the census is not blind.</human-check>
    <human-check>The 32 form-4 nodes were repaired with Arial first, and the six pre-existing `font-family: inherit` declarations were read and left alone — the distinction between "some controls repaired" and "the class closed" is recorded.</human-check>
  </verify>
  <done>
O-Bells ships three languages at `reviewed: 'bt'` on all 255 rows, version 4.5.0 read from the
CMakeLists value, CHANGELOG written, built and installed, geometry equal on all three arms
across all 14 states. Its 32 form-4 nodes name Arial first then the tail and the
`undeclared-font` screen reads 0 against a non-empty input; its lone naked `monospace`
declaration names a face and carries the tail; `line-height-normal` and `wrap-count` read 0. Its
gate file derives both its language assertion and its two walk call sites, with the
derive-or-abort control fired against the walk, and its `EXPECTED_LATE` pin is untouched and
still reports exactly 2 as the wave's census control. Both stale enumerations deleted in en and
fr, zh authored from the corrected English. Path-scoped commits: the `'mt'` table, any
correction round, then the promotion.
  </done>
</task>

<task type="auto">
  <name>Task 3: O-Gain, O-SpectralShaper, O-Bassoon, O-TextureForge — two token edits, a canvas caption, a shared module, a bundle, and the batch close-out (ZH4C-03..06, ZH4C-10, ZH4C-11)</name>
  <files>plugins/O-Gain/Source/ui/public/js/i18n.js, plugins/O-Gain/Source/ui/public/index.html, plugins/O-Gain/Source/PluginProcessor.h, plugins/O-Gain/CMakeLists.txt, plugins/O-Gain/CHANGELOG.md, plugins/O-SpectralShaper/Resources/ui/js/i18n.js, plugins/O-SpectralShaper/Resources/ui/index.html, plugins/O-SpectralShaper/Resources/ui/css/styles.css, plugins/O-SpectralShaper/Resources/ui/js/components/Spectrogram.js, plugins/O-SpectralShaper/Source/PluginProcessor.h, plugins/O-SpectralShaper/CMakeLists.txt, plugins/O-SpectralShaper/CHANGELOG.md, plugins/O-Bassoon/Resources/ui/js/i18n.js, plugins/O-Bassoon/Resources/ui/index.html, plugins/O-Bassoon/Source/PluginProcessor.h, plugins/O-Bassoon/tests/ui_tip_render_check.js, plugins/O-Bassoon/CMakeLists.txt, plugins/O-Bassoon/CHANGELOG.md, plugins/O-TextureForge/Source/ui/public/js/i18n.js, plugins/O-TextureForge/Source/ui/public/index.html, plugins/O-TextureForge/Source/ui/public/css/ouaricon-naturalist.css, plugins/O-TextureForge/Source/ui/src/app.js, plugins/O-TextureForge/Source/ui/public/js/app.bundle.js, plugins/O-TextureForge/Source/PluginProcessor.h, plugins/O-TextureForge/tests/ui_tip_render_check.js, plugins/O-TextureForge/CMakeLists.txt, plugins/O-TextureForge/CHANGELOG.md, plugins/O-TextureForge/tests/i18n-states.json, PLUGINS.md</files>
  <precondition>Tasks 1 and 2 are committed and O-Detune and O-Bells are installed — the cold auval sweep at the end of this task covers all six and cannot run until every bundle is on disk. `node scripts/boot-all-uis.js --strict-tips` still reports exactly 2 late and 0 dead.</precondition>
  <action>
Run the nine-step procedure on the four remaining plugins, **one path-scoped commit stream per
plugin** so a failure in one does not strand the others. Per-plugin specifics, all measured at
planning time.

**O-Gain** — 63 entries / 89 rows, 5 states, `Source/ui/public/`, `js/app.js` controller, no
external CSS, 11 `I18N_EXEMPT` entries, version 1.3.3 at line 11 → **1.4.0**, target equals the
folder. **The simplest plugin in the wave**: exactly one font declaration (`index.html:43`,
`Garamond, 'Georgia', 'Times New Roman', serif`) covering the whole page, zero form-4 nodes,
zero bare generics, and 44 `line-height: normal` leaves. One tail edit. It has **no gate file**
— like O-IntonationPad in wave 4b, there is nothing to repair and nothing to lean on; do not
write one, that is a separate pass with its own budget. `check-ui-labels`, `measure-ui` and
`boot-all-uis` are the whole instrument. Its states drive four learn-confidence evals through
`window.updateMeters`, so the confidence captions are reachable and must be measured. **Its gear
body is CORRECT** — it names the language and the hover-help switch (M12). Read it, leave it,
record it as a checked non-defect. Its language body does carry the enumeration and takes the
deletion in en and fr.

**O-SpectralShaper** — 47 entries / 71 rows, 3 states, `Resources/ui/`, `js/app.js` plus
`js/components/` and `modules/`, external `css/styles.css`, only 2 `I18N_EXEMPT` entries — the
smallest exempt list in the wave, so the W3 both-ways screen is cheap here — version 1.7.3 at
line 5 → **1.8.0**. **A one-token font edit**: `css/styles.css:73` defines `--serif` and 12
declarations resolve through it, so the tail lands once. Zero form-4 nodes; the token reaches
even `html`. Its `tests/ui_preset_menu_check.js` is a **preset-menu gate with no language
reference of any kind** — read end to end at planning time, 407 lines, no `LANGUAGES`, no
`'en'`/`'fr'`, no integer near a language walk (C4 applied, non-finding recorded). It needs no
repair; leave it. **Its gear body is CORRECT** and names both choices (M12) — read, leave,
record. **And it carries the wave's canvas caption (M8).** `js/components/CurveEditor.js` draws
numeric axis ticks through a stack that names Times New Roman — an evidenced non-finding, no
change. `js/components/Spectrogram.js` draws one hard-coded English sentence inside its Canvas-2D
fallback path, invisible to every gate in this repo and shipped untranslated in French too.
**Route it through the table following O-Comp's shipped precedent**: house it in `I18N` with an
empty body and read it through the table from the render path, keeping it out of `LABELS`
because assertion 15 fails a LABELS key that no element and no `setLabel` call reaches. Read
O-Comp's `js/i18n.js` for the exact shape before writing it. The branch cannot be produced on
this machine, so the verification is on the source: no `fillText` call in that file takes a
string literal afterwards. Also verify the same is true of the other five (measured: 0 each).

**O-Bassoon** — 49 entries / 62 rows, 2 states, `Resources/ui/`, inline-module controller, no
external CSS, only 4 `I18N_EXEMPT` entries, `IS_SYNTH TRUE`, version 1.3.1 at line 14 →
**1.4.0**, frame written `setSize (900, 600)` with a space, which both parsers handle
(verified). Four font declarations in `index.html`: `:68` and `:412` name Times New Roman and
take the **tail only**; `:137` and `:432` read `'EB Garamond', 'Garamond', serif` and are the
wave's only Latin-exposed bare generics — but both render the **product name**, which is
`I18N_EXEMPT` and can never receive Han, so they take **face-naming only and NO tail** (M6b).
Getting that discrimination right is the whole point of keeping the two halves separable. Its
gate is the joined-pair assertion at line 209 with two literal walk sites at 383 and 412 — repair
both (M2). **The shared tuning panel stays as found (M9)**: its Tuning tab comes from
`modules/tuning/scala-tuning-engine/js/tuning-panel.js`, which has zero i18n hooks in any
language and is embedded by five plugins; its own state file does not drive that tab; and its
language hover-help **already discloses in en and fr** that the Tuning tab stays in English
because its panel comes from a shared module. Carry that disclosure into the zh row. Record the
shared module in `deferred-items.md` with all five consumers named as the blast radius. **Its
gear body is FALSE** and takes the deletion in en and fr.

**O-TextureForge** — 46 entries / 61 rows, **1 state**, `Source/ui/public/`, controller
`js/i18n_init.js`, external `css/ouaricon-naturalist.css`, 14 `I18N_EXEMPT` entries,
`IS_SYNTH TRUE`, version 1.3.1 at line 30 → **1.4.0**, and the folder name differs from the
`juce_add_plugin` target — pass the **folder** name to the build script, which resolves the
target itself. **A one-token font edit**: `css/ouaricon-naturalist.css:41` defines
`--font-primary` and 6 declarations resolve through it; buttons on the main page inherit it, so
zero form-4 nodes are declared-visible. **No webpack rebuild for the table (M10)** — the
renderer is outside the bundle and the gate already asserts it. **One conditional**: the bundle
builds a file-size overlay at runtime holding two `<button>` elements whose captions come from
the label table, and the single-state file never opens it, so no measurement reaches it. **Add a
state that opens that overlay, measure, and repair only if the measurement says so.** If it
does, the change is in `src/app.js`, the rebuilt `js/app.bundle.js` is committed with it, and
the control that the shipped bundle matches its source is a rebuild that leaves `git diff` on
the bundle empty when the source is unchanged. Its gate is the joined-pair assertion at 290 with
walk sites at 501 and 512 — repair both. **Its gear body is FALSE** and takes the deletion.

All four carry the fixed-pair language enumeration in en and fr; remove it in all four.

**Batch close-out, after all four are built and installed:**

1. **One cold auval sweep covering all six.** Read each triple off `auval -a` — **never guess
   one**. Wave 4b's plan assumed the manufacturer code and got it wrong on six CHANGELOG lines;
   the value read off `auval -a` there was `OuDv`, and it must be read again rather than carried.
   Measured at planning time: O-Bells, O-Bassoon and O-TextureForge declare `IS_SYNTH TRUE` and
   O-Detune, O-Gain and O-SpectralShaper do not, so expect three instruments and three effects,
   with plugin codes `OBls`, `OGan`, `OSpS`, `OuDt`, `OBsn`, `OuTF` — **but read the rows.**
   Expect one registry rescan of roughly fifteen minutes for the whole sweep. Before the sweep,
   confirm no alternate-variant orphan is on disk for any of the six: all six were `-dev` only at
   planning time, and a leftover unsuffixed bundle beside a `-dev` one pins Logic's registry slot
   to whichever was installed first. Follow CLAUDE.md's cache sequence; `build-and-install.sh`
   Phase 4 already does the dual-variant sweep and warns on an orphan.
2. **`PLUGINS.md`, once, in its own commit.** All six rows are behind their CMakeLists as found,
   and O-Bassoon's row additionally carries a stale STATUS — it reads a pre-implementation stage
   marker while the plugin is installed and at 1.3.1. Write the shipped versions and correct the
   status from the installed state. Then run the duplicate check — it must print nothing.
3. **Repo-wide regression sweep** — the whole suite, not just this wave, including the flipped
   lint, which exits 2 rather than reporting.

**Commit discipline throughout this task and the two before it:** re-check
`git branch --show-current` and `git status --short` **immediately before every commit**, not
once at the start. This is a shared checkout and another session's staging can join your commit
in the gap. Name paths explicitly; never `git add -A`, never `git commit -a`.
`.gsd/dispatch-isolation-sentinel.json` is modified in the tree and predates this work — never
stage it. Never stage anything under `plugins/O-Orbit/libs/SAF`. Every commit message ends with
the session trailer.
  </action>
  <verify>
    <automated>for p in O-Gain O-SpectralShaper O-Bassoon O-TextureForge; do node scripts/check-i18n.js --plugin $p >/dev/null 2>&1; echo "$p check-i18n exit=$?"; done</automated>
    <automated>for p in O-Gain O-SpectralShaper O-Bassoon O-TextureForge; do node scripts/check-ui-labels.js --plugin $p >/dev/null 2>&1; echo "$p check-ui-labels exit=$?"; done</automated>
    <automated>for p in O-Gain O-SpectralShaper O-Bassoon O-TextureForge; do echo "--- $p"; node scripts/i18n-zh-lint.js --plugin $p 2>&1 | tail -6; done   # 0 findings each</automated>
    <automated>node scripts/check-i18n.js >/dev/null 2>&1; echo "repo-wide check-i18n exit=$?"   # 0, 43 localized plugins, 22 now three-language</automated>
    <automated>node scripts/i18n-zh-lint.js >/dev/null 2>&1; echo "flipped lint repo-wide exit=$?"   # 0 — it is a gate, so this zero is load-bearing</automated>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/i18n-fr-lint.js >/dev/null 2>&1; echo "fr-lint exit=$?"   # 0 — French untouched by the whole wave</automated>
    <automated>for p in O-Detune O-Bells O-Gain O-SpectralShaper O-Bassoon O-TextureForge; do echo -n "$p han-in-cpp: "; find plugins/$p/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' {} + | wc -l | tr -d ' '; done   # 0 each</automated>
    <automated>for p in O-Detune O-Gain O-TextureForge; do echo -n "$p control: "; perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' plugins/$p/Source/ui/public/js/i18n.js | wc -l | tr -d ' '; done; for p in O-Bells O-SpectralShaper O-Bassoon; do echo -n "$p control: "; perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' plugins/$p/Resources/ui/js/i18n.js | wc -l | tr -d ' '; done   # 1 each — a 0 means the gate above proved nothing</automated>
    <automated>for p in O-Detune O-Gain O-TextureForge; do echo -n "$p stale-enum: "; perl -0777 -ne '$n++ while /(English and Fran|English or Fran|anglais et le fran|holds one control|carries a single|That is all it holds|une seule commande|un seul contr)/gi; END{print $n+0,"\n"}' plugins/$p/Source/ui/public/js/i18n.js; done; for p in O-Bells O-Bassoon; do echo -n "$p stale-enum: "; perl -0777 -ne '$n++ while /(English and Fran|English or Fran|anglais et le fran|holds one control|That is all it holds|une seule commande|Il ne contient rien)/gi; END{print $n+0,"\n"}' plugins/$p/Resources/ui/js/i18n.js; done   # 0 on all five that took the deletion</automated>
    <automated>perl -0777 -ne '$n++ while /(English and Fran|anglais et le fran)/gi; END{print "O-SpectralShaper language enum: ",$n+0,"\n"}' plugins/O-SpectralShaper/Resources/ui/js/i18n.js   # 0 — the language body took the deletion; its GEAR body was correct and is untouched</automated>
    <automated>for p in O-Detune O-Bells O-Bassoon O-TextureForge; do echo -n "$p gate two-lang literal: "; perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' plugins/$p/tests/ui_tip_render_check.js | grep -cE "=== *'en,fr'|\[ *'en' *, *'fr'|sweep\('fr'\)"; done   # 0 on all four — assertion AND walk, comments stripped</automated>
    <automated>for p in O-Detune O-Bells O-Bassoon O-TextureForge; do node plugins/$p/tests/ui_tip_render_check.js >/dev/null 2>&1; echo "$p tip-gate exit=$?"; done   # 0 each with a real zh arm</automated>
    <automated>node plugins/O-SpectralShaper/tests/ui_preset_menu_check.js >/dev/null 2>&1; echo "O-SpectralShaper preset-gate exit=$?"   # 0 — the untouched gate still passes</automated>
    <automated>grep -crE "fillText\( *['\"]" plugins/O-SpectralShaper/Resources/ui/js/ | awk -F: '{s+=$NF} END{print "SpectralShaper fillText string literals: " s+0}'   # 0 — the canvas caption is read from the table now</automated>
    <automated>for p in O-Detune O-Gain O-TextureForge; do echo -n "$p fillText literals: "; grep -rhoE "fillText\( *['\"]" plugins/$p/Source/ui 2>/dev/null | grep -v node_modules | wc -l | tr -d ' '; done; for p in O-Bells O-Bassoon; do echo -n "$p fillText literals: "; grep -rhoE "fillText\( *['\"]" plugins/$p/Resources/ui 2>/dev/null | wc -l | tr -d ' '; done   # 0 on all five — the evidenced non-finding, re-confirmed</automated>
    <automated>for p in O-Detune O-Bells O-Gain O-SpectralShaper O-Bassoon O-TextureForge; do echo "--- $p"; node scripts/measure-ui.js --plugin $p --mode box --report all 2>&1 1>/dev/null | grep -E "undeclared-font|line-height-normal|wrap-count|svg-font-attr|identity"; done   # 0 findings on every screen, each beside a NON-ZERO identity count</automated>
    <automated>for p in O-Detune O-Bells O-Gain O-SpectralShaper O-Bassoon O-TextureForge; do echo -n "$p zh-Han-nodes/no-CJK-face: "; node -e "const r=JSON.parse(require('fs').readFileSync(0,'utf8')); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); console.log(z.length+'/'+z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff)).length)" < <(node scripts/measure-ui.js --plugin $p --mode box 2>/dev/null); done   # first number > 0 on each (input non-empty), second 0 on each</automated>
    <automated>git diff --stat -- plugins/O-TextureForge/Source/ui/public/js/app.bundle.js   # empty unless src/app.js changed in the same commit — the bundle-matches-source control</automated>
    <automated>node scripts/boot-all-uis.js --strict-tips 2>&1 | tail -8   # 0 DEAD, 0 failed, exactly 2 late (O-Bells' pinned census control, unchanged)</automated>
    <automated>node scripts/i18n-zh-backtranslate.js 2>&1 | tail -5   # stage view: 2439 zh-Hans rows, BELOW SHIP BAR 0</automated>
    <automated>grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d   # MUST print nothing</automated>
    <automated>for p in O-Detune O-Bells O-Gain O-SpectralShaper O-Bassoon O-TextureForge; do cm=$(perl -ne 'print "$1\n" if /^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ && !$d++' plugins/$p/CMakeLists.txt); reg=$(grep -E "^\| $p \|" PLUGINS.md | cut -d'|' -f4 | tr -d ' '); [ "$cm" = "$reg" ] && v=AGREE || v=MISMATCH; echo "  $p cmake=$cm registry=$reg $v"; done   # AGREE six for six (MISMATCH six for six before the work)</automated>
    <automated>grep -E "^\| O-Bassoon \|" PLUGINS.md | grep -c "Installed"   # 1 — the stale pre-implementation status marker is gone</automated>
    <automated>ls ~/Library/Audio/Plug-Ins/VST3/ ~/Library/Audio/Plug-Ins/Components/ | grep -cE "^(O-Detune|O-Bells|O-Gain|O-SpectralShaper|O-Bassoon|O-TextureForge)\.(vst3|component)$"   # 0 — no unsuffixed alternate-variant orphan beside the -dev bundles</automated>
    <human-check>auval -v on all six triples, every triple read off `auval -a` rather than assumed, one cold rescan. All six report AU VALIDATION SUCCEEDED, and the manufacturer code written into all six CHANGELOGs is the one auval printed.</human-check>
    <human-check>Every back-translation triple for all four plugins read with --verbose; accepted drifts carry a written reason.</human-check>
    <human-check>O-Gain's and O-SpectralShaper's gear bodies were read in full and deliberately left alone because each names both of its choices — recorded as checked non-defects, not unchecked ones.</human-check>
    <human-check>O-TextureForge's file-size overlay was driven by a new state and measured. Either its two buttons already name a CJK face (recorded as a measured non-finding) or they were repaired in src/app.js and the rebuilt bundle is committed alongside.</human-check>
    <human-check>Installed bundle CFBundleShortVersionString matches the shipped version for all six, VST3 and AU each.</human-check>
  </verify>
  <done>
All six wave-4c plugins ship English, French and Simplified Chinese at `reviewed: 'bt'` on every
row — 609 new rows, corpus at 2439 zh-Hans rows with `BELOW SHIP BAR 0` across 22 plugins. All
four tip-render gate files derive both their language assertion and their language walk, hold no
two-language literal in any syntax with comments stripped, and carry the derive-or-abort control
fired against the walk; O-Detune's direction assertion asserts a difference; O-SpectralShaper's
preset gate is untouched and still passes. No stack on any of the six ends at a generic without
naming an installed face, and the `undeclared-font` screen reads 0 on all six beside a non-empty
input. O-SpectralShaper's canvas caption is read from the table and no `fillText` in the wave
takes a string literal. The shared tuning panel and O-Bells' two pinned late bindings are
recorded in `deferred-items.md` as deliberate carry-forwards with their blast radius named.
PLUGINS.md carries six corrected rows in one commit, no duplicates, agreeing with CMakeLists six
for six, with O-Bassoon's stale status corrected. Repo-wide `check-i18n`, the gated
`i18n-zh-lint` and `i18n-fr-lint` all clean; `boot-all-uis --strict-tips` 0 dead, 0 failed, 2
late. auval PASS on all six.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| executor → blind reverse reader | An external agent receives Chinese strings with the English deliberately withheld. Anything that lets it recover the English, or correlate a correction batch against a prior one, makes the read vacuous. Two of this wave's plugins name themselves in their own copy. |
| glossary root → shipped copy | A settled `TERMS` root applied to two different English keys on one page ships an unreadable sentence that no automated check in this repo can see. |
| zh edits → en/fr rendering | CSS declarations, font tokens and geometry pins are shared surfaces. A change made for Chinese can move a language this work does not touch — and two of this wave's six font edits are single tokens fanning out to 12 and 6 declarations. |
| shared module → five plugins | `modules/tuning/scala-tuning-engine/js/tuning-panel.js` is embedded by five plugins' CMakeLists. Any edit is a five-plugin change with five builds and five auvals. |
| webpack source → shipped bundle | `js/app.bundle.js` is a committed build artefact. A source edit without a rebuild, or a rebuild without a commit, ships a page that does not match its source and nothing reports it. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-rwh-01 | Information disclosure | `i18n-zh-backtranslate --emit` batch | high | mitigate | R6 + W4 + C10 — blind 12-hex ids, per-batch salt, explicit `--manifest`, `--forward-provenance`, **both** `--emit` and `--plugin` so the batch is not the whole 1830-row corpus, `--allowed-tools ""` from a cwd outside the repo, repo access forbidden in the prompt; fire all five blinding controls per batch rather than trusting the code exists |
| T-rwh-02 | Repudiation | correction rounds | medium | mitigate | R4 + C10 — fresh reader, fresh salt **and a different model** per round, so an agreement is independent rather than self-consistent |
| T-rwh-03 | Tampering | shared `.git` index across concurrent sessions | high | mitigate | Re-check branch and `git status --short` immediately before every commit; path-scoped commits only; never stage `.gsd/dispatch-isolation-sentinel.json` or anything under `plugins/O-Orbit/libs/SAF` |
| T-rwh-04 | Denial of service | en/fr regression from a zh-motivated font or pin edit | high | mitigate | R8 — grep existing declarations before appending; the form-4 repair keeps **Arial first** and the face-naming repair keeps the installed face first, so Latin metrics do not move; `check-ui-labels` must read 0 FAIL on the **en and fr** arms after the font work, not only after the tail; `i18n-fr-lint` clean repo-wide |
| T-rwh-05 | Spoofing | a gate or screen that passes vacuously | high | mitigate | Every negative gate carries a positive control that fired: the Han-in-C++ grep beside a control on the same run; every `measure-ui` zero beside a non-zero Han-node count on the same rows (M1 is exactly this failure, caught at planning time); derive-or-abort fired against the **walk** on all four gates; `--self-test` 10/10 before any lint zero is trusted; O-Bells' 2 late bindings as the boot-census control |
| T-rwh-06 | Tampering | webpack source / bundle divergence | medium | mitigate | M10 — no bundle edit unless the overlay measurement demands one; if it does, the rebuilt bundle is committed in the same commit and `git diff --stat` on the bundle is the control that source and artefact agree |
| T-rwh-07 | Elevation of privilege | an unbudgeted five-plugin change | medium | mitigate | M9 — the shared tuning panel is left exactly as found and recorded as a deferred item with all five consumers named; the plugin's own copy already discloses the situation in en and fr, so no user-visible claim goes stale |
| T-rwh-SC | Tampering | package installs | low | accept | This wave installs nothing — no npm, pip or cargo operation is in scope. A webpack rebuild, if the M10 conditional fires, runs against the `node_modules` already present and adds no dependency. No legitimacy audit required. |
</threat_model>

<verification>
The wave is not done until **all six** are green — no partial waves.

Per plugin, all six:
- `check-i18n --plugin <Name>` exit 0, `LANGUAGES` reads three
- `check-ui-labels --plugin <Name>` exit 0, 0 FAIL, zh arm 0 geometry moved, en and fr arms unchanged
- `i18n-zh-lint --plugin <Name>` 0 findings, `BELOW SHIP BAR 0`
- every back-translation triple read with `--verbose`, drifts resolved with recorded reasons
- `measure-ui --mode box --report all` 0 findings on all four screens, **each beside a non-zero
  count of visible Han-bearing nodes on the same rows** — a zero without that number is M1
  repeating itself
- no stack ends at a generic without naming an installed face, and no node takes its face from
  the UA stylesheet
- zero Han under `Source/**/*.{h,cpp}`, with the positive control fired on the same run
- `auval -v` AU VALIDATION SUCCEEDED, triple read off `auval -a`

Repo-wide, once:
- `check-i18n` exit 0, 43 localized plugins
- `i18n-zh-lint` exit 0 and `--self-test` 10/10
- `i18n-fr-lint` exit 0
- `boot-all-uis --strict-tips` 0 DEAD, 0 failed, **exactly 2 late** — O-Bells' pinned pair,
  unchanged, as the census control
- `i18n-zh-backtranslate` stage view 2439 zh-Hans rows, `BELOW SHIP BAR 0`
- `PLUGINS.md` no duplicate rows; six rows agree with their CMakeLists; O-Bassoon's status corrected
- all four repaired gate files derive their language list **at both the assertion and the walk**,
  hold no two-language literal in any syntax with comments stripped, and pass with a real zh arm

**A zero from a gate whose positive control was never run is not evidence — and on this wave a
zero from a Han-gated screen run before the table lands is not even a zero.**
</verification>

<success_criteria>
Six plugins ship English, French and Simplified Chinese, built, installed and auval-clean. Every
zh row at `reviewed: 'bt'`, promoted only after a blind reverse read. English and French geometry
unchanged on all six after both halves of the font work. Four gate files repaired at both their
language assertion and their language walk, one of them also rewritten off a direction-specific
assertion, and the derive-or-abort control fired against the walk on each. O-Bells' 32 form-4
nodes and its 584 normal line boxes closed. O-SpectralShaper's canvas caption localized to
O-Comp's shipped precedent. The shared tuning panel and O-Bells' two pinned late bindings
deliberately untouched and recorded. One path-scoped commit stream per plugin, PLUGINS.md once at
the end, six registry rows corrected from the CMakeLists values.
</success_criteria>

<output>
Create `.planning/quick/260905-rwh-wave-4c/260905-rwh-SUMMARY.md` when done with
`status: complete` frontmatter, and a `deferred-items.md` beside it in the shape waves 4a and 4b
used.

**Carry forward for wave 4d**, explicitly:

- Whether the M1 correction holds — that `measure-ui`'s Han-gated screens are vacuous before the
  table lands, and that the language-independent proxies (the computed-`ff` census and the
  `line-height: normal` leaf count) are the right pre-work baseline. Wave 4d should state its
  proxies up front rather than reporting four zeros.
- Whether M2 generalises — the count of gate files whose language **walk** was a separate site
  from the language **assertion**, and whether any wave-4d gate hides a fifth enumeration form.
- The measured installed-family list (M4). It is a property of the machine, not the plugin, and
  the bare-generic arithmetic changes completely between waves because of it.
- Every re-inherited item: wave 4b's D1–D7 minus D7 (closed), plus this wave's shared
  `scala-tuning-engine` tuning panel with its five consumers named, and O-Bells' two pinned late
  bindings with the gate line that pins them.
- Whether any prediction in this plan was **false as stated** — wave 4b's C9 is that a plan's
  stated consequence can be wrong even when the defect is real (N7 on O-IntonationPad). Name any
  that were, in the shape wave 4b's "Corrections to the plan's own measurements" section used.

Do **not** commit the PLAN, the SUMMARY, `deferred-items.md` or `STATE.md` — the orchestrator
does that. Do **not** touch `ROADMAP.md`.
</output>
