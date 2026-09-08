---
phase: quick-260907-qda
plan: 01
type: execute
wave: 1
depends_on: []
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4g, corpus-close, O-simpleAdditive, O-simpleGrain, O-simpleSampler, O-simpleSubtractive, O-Polystutter, O-Orbit, submodule, fonts, geometry, unscanned-modules]
autonomous: true
requirements: [ZH4G-01, ZH4G-02, ZH4G-03, ZH4G-04, ZH4G-05, ZH4G-06, ZH4G-07, ZH4G-08, ZH4G-09, ZH4G-10, ZH4G-11, ZH4G-12]
files_modified:
  # O-simpleAdditive (TRACER)
  - plugins/O-simpleAdditive/Source/ui/public/js/i18n.js
  - plugins/O-simpleAdditive/Source/ui/public/index.html
  - plugins/O-simpleAdditive/Source/ui/public/css/styles.css
  - plugins/O-simpleAdditive/Source/PluginProcessor.h
  - plugins/O-simpleAdditive/CMakeLists.txt
  - plugins/O-simpleAdditive/CHANGELOG.md
  # O-simpleGrain
  - plugins/O-simpleGrain/Source/ui/public/js/i18n.js
  - plugins/O-simpleGrain/Source/ui/public/index.html
  - plugins/O-simpleGrain/Source/ui/public/css/styles.css
  - plugins/O-simpleGrain/Source/PluginProcessor.h
  - plugins/O-simpleGrain/CMakeLists.txt
  - plugins/O-simpleGrain/CHANGELOG.md
  # O-simpleSampler
  - plugins/O-simpleSampler/Source/ui/public/js/i18n.js
  - plugins/O-simpleSampler/Source/ui/public/index.html
  - plugins/O-simpleSampler/Source/ui/public/css/styles.css
  - plugins/O-simpleSampler/Source/PluginProcessor.h
  - plugins/O-simpleSampler/CMakeLists.txt
  - plugins/O-simpleSampler/CHANGELOG.md
  # O-simpleSubtractive
  - plugins/O-simpleSubtractive/Source/ui/public/js/i18n.js
  - plugins/O-simpleSubtractive/Source/ui/public/index.html
  - plugins/O-simpleSubtractive/Source/ui/public/css/styles.css
  - plugins/O-simpleSubtractive/Source/PluginProcessor.h
  - plugins/O-simpleSubtractive/CMakeLists.txt
  - plugins/O-simpleSubtractive/CHANGELOG.md
  # O-Polystutter
  - plugins/O-Polystutter/Source/ui/public/js/i18n.js
  - plugins/O-Polystutter/Source/ui/public/index.html
  - plugins/O-Polystutter/Source/PluginProcessor.h
  - plugins/O-Polystutter/CMakeLists.txt
  - plugins/O-Polystutter/CHANGELOG.md
  # O-Orbit  — NOTHING under plugins/O-Orbit/libs/SAF is EVER staged
  - plugins/O-Orbit/Resources/ui/js/i18n.js
  - plugins/O-Orbit/Resources/ui/index.html
  - plugins/O-Orbit/Resources/ui/css/styles.css
  - plugins/O-Orbit/Source/PluginProcessor.h
  - plugins/O-Orbit/CMakeLists.txt
  - plugins/O-Orbit/CHANGELOG.md
  # batch close-out
  - PLUGINS.md

estimate:
  tokens: 365000
  raw_tokens: 365000
  tasks: 7
  confidence: low
  # factor 1.0 — no calibration sample set exists for this repo, so tokens == raw_tokens
  # and confidence is DERIVED from that zero sample count, never self-rated.
  #
  # BASIS. Six comparable agent-context samples:
  #   Stage 3   610k / 758 rows = 805 tok/row
  #   wave 4a   367k / 580 rows = 633 tok/row
  #   wave 4c   268k / 610 rows = 439 tok/row
  #   wave 4d   258k / 633 rows = 408 tok/row
  #   wave 4e   ~300k / 651 rows = 461 tok/row
  #   wave 4f   ~380k / 860 rows = 442 tok/row
  # Rate used: 400 tok/row on 820 rows = 328k, plus ~37k structural across SEVEN tasks.
  # The structural mix is the LIGHTEST of the rollout in three dimensions — zero gate
  # files (waves 4a-4f carried 1-5 each), zero scala-tuning-engine consumers, zero
  # shared-module caption columns — and the HEAVIEST in two: O-Orbit is the first
  # plugin in the rollout whose ENTIRE page renders through a stack naming no
  # installed face, and four of the six carry an unscanned module JS file that must be
  # adjudicated rather than fixed. Split across SEVEN fresh executors, so no single
  # agent carries more than ~55k.

must_haves:
  truths:
    - "A user opens any of the six plugins, picks the Chinese endonym from the language selector, and every caption, section heading and hover-help body the plugin's own table reaches renders in Simplified Chinese."
    - "Switching back to English or French reproduces the pre-change page in geometry — no element moved on the en or fr arm, on any of the six, across every state."
    - "The Chinese page has no clipped, wrapped or displaced element: check-ui-labels reports 0 moved on the zh arm of all six."
    - "No ASCII text on any of the six pages changes face when the page language changes. On five of six that is already true because every house stack reaches an installed Times New Roman or Georgia; on O-Orbit it is FALSE on the tree as found and must be MADE true — its single house stack `Garamond, 'EB Garamond', serif` names no installed family, so its whole page's Latin resolves through a bare generic that Chromium rebinds to a Chinese face under lang=zh-Hans."
    - "Every node that can receive a Han codepoint resolves to a CJK face named by the page, not by the document-language fallback."
    - "No Chinese character exists anywhere under any plugin's Source/**/*.{h,cpp} — the C++ carries only the ASCII language code, on both the encode and the decode line, proved per plugin by a comment-stripped grep beside a positive control."
    - "Every zh row has been read back through a blind reverse pass and either accepted with a written reason or re-authored."
    - "Six PLUGINS.md rows, every one of them one patch stale against its own CMakeLists, are corrected to the shipped version in one commit — and O-simpleSampler's status cell flips from an equally stale ✅ Working to 📦 Installed."
    - "O-simpleGrain's version bump moves BOTH the version string and the OSIMPLEGRAIN_VERSION_CODE hex mirror that tests/render-harness/CMakeLists.txt compiles into JucePlugin_VersionCode."
    - "Not one path under plugins/O-Orbit/libs/SAF is staged in any commit of this wave, and the submodule is left at b6fe1882 (v1.3.4)."
    - "All six load in Logic: auval PASS on all six triples after one cold registry rescan."
    - "The corpus closes: 43 of 43 plugins carrying an i18n.js also carry zh rows, 5441 of 5441 rows, with O-Strata named explicitly as the 44th and excluded for having no i18n.js under either UI root."
  artifacts:
    - plugins/O-simpleAdditive/Source/ui/public/js/i18n.js    # zh-Hans on every I18N and LABELS key
    - plugins/O-simpleGrain/Source/ui/public/js/i18n.js
    - plugins/O-simpleSampler/Source/ui/public/js/i18n.js
    - plugins/O-simpleSubtractive/Source/ui/public/js/i18n.js
    - plugins/O-Polystutter/Source/ui/public/js/i18n.js
    - plugins/O-Orbit/Resources/ui/js/i18n.js
    - plugins/O-Orbit/Resources/ui/css/styles.css             # the wave's only whole-page face-naming repair
    - PLUGINS.md                                              # six rows, every one corrected from CMakeLists, one status cell flipped
  key_links:
    - "LANGUAGES in i18n.js <-> the endonym <option> in index.html <-> languageCode/languageIndex in PluginProcessor.h — a third language present in two of the three renders a page that cannot be reached or cannot be persisted."
    - "O-Orbit's ONE house stack <-> every text node on its page — 14 font-family sites, 0 `inherit`, one distinct declaration. There is no second stack to fall back to, so naming an installed face is not a per-node repair on O-Orbit, it is THE repair."
    - "OSIMPLEGRAIN_VERSION_CODE (0x010403) <-> tests/render-harness/CMakeLists.txt:58 JucePlugin_VersionCode — the hex mirror is a SECOND version source on exactly one of the six. Bumping only the string ships a harness compiled against a stale code, and no gate in this repo reads it."
    - "The one-arm CMake reader <-> O-simpleGrain's set(OSIMPLEGRAIN_VERSION \"1.4.3\") — the one-arm reader returns EMPTY on it, measured. R9 is load-bearing this wave, not ceremonial."
    - "plugins/O-Orbit/libs/SAF <-> every `git commit -- plugins/O-Orbit` in Task 6 — the pathspec `plugins/O-Orbit` INCLUDES the submodule path. The guard is not optional and it is not implied by path-scoping."
    - "The four unscanned module JS files <-> check-i18n's [12] clause — it scans `js/app.js` (and `js/parameter-bindings.js` on O-Polystutter) and reports PASS. Files at `modules/` and `js/modules/` are opened by no gate in this repo, and two of them emit user-facing English."
    - "PLUGINS.md's six rows <-> the CMakeLists values — every row is exactly one patch behind, all six from the same 2026-09-03 suite-wide French hover-help rename (task 260903-ukp). The task description's `from` versions are a faithful read of the stale rows."
    - "O-simpleSubtractive's div.routing-label <-> wave 4e D7 — the pre-existing French wrap the carry-forwards attribute to O-simpleFM is on a SHARED class with at least two carriers, and one of them is in this wave's own set. Its wrap-count baseline is 1, not 0."
---

<objective>
Localize six plugins into Simplified Chinese — **O-simpleAdditive, O-simpleGrain, O-simpleSampler,
O-simpleSubtractive, O-Polystutter, O-Orbit** — to the ship bar Stages 2-3 and waves 4a-4f
established, and ship each with a minor version bump, a build, an install and one cold auval sweep at
the end of the batch.

**820 emitter rows** (measured off the emitter's own `rows` column at planning time, not estimated);
the corpus goes 4621 → **5441** rows of 5441 and 37 → **43** localized plugins. **This wave closes the
corpus.**

Purpose: wave 4g is the seventh and last volume wave, and it is the first in which:

- **not one plugin owns a gate file** — six of six, verified with `find` and not a glob, where waves
  4a-4f carried between one and five. The N12 enumeration census has no subject;
- **no plugin is a `scala-tuning-engine` consumer** — no 37-key module column, no `1.11` ratio block,
  no `d848337a` floors, no 17-of-37 coverage hole. The single largest per-plugin cost reduction since
  the module question opened in wave 4e;
- **a plugin's ENTIRE page renders through a font stack that names no installed family** — O-Orbit's
  `Garamond, 'EB Garamond', serif`, 14 declaration sites, zero `font-family: inherit`, one distinct
  stack. Every earlier wave's face-naming half was two or three nodes; here it is the page;
- **all three CMake version shapes appear together**, including the `set()`-variable form wave 4f
  reported absent — and on that one plugin the one-arm reader returns **EMPTY**, measured;
- **one plugin carries a SECOND version source** — O-simpleGrain's `OSIMPLEGRAIN_VERSION_CODE` hex
  mirror, compiled into the render-harness as `JucePlugin_VersionCode`;
- **every one of the six PLUGINS.md rows is stale**, all by exactly one patch and all from the same
  2026-09-03 commit, where wave 4f found one stale row and called it the wave's most consequential
  false premise;
- **four of six carry a module JS file no gate in this repo opens**, two of which emit user-facing
  English to the page.

Output: six three-language plugins, one path-scoped commit stream per plugin, one PLUGINS.md commit
at the end of the batch, one cold auval sweep covering all six, and a corpus that reads 43 of 43.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@.planning/quick/260907-ja8-stage-4-wave-4f-of-the-zh-hans-rollout-f/deferred-items.md
@.planning/quick/260906-h8y-wave-4e-o-lyrica-o-simplebeatmaker-o-sim/deferred-items.md
@.planning/quick/260906-s71-localize-the-shared-scala-tuning-engine-/260906-s71-deferred-items.md
@.planning/quick/260904-g5l-stage-3-of-the-zh-hans-rollout-the-hard-/260904-g5l-SUMMARY.md
@scripts/measure-ui-README.md
@scripts/i18n-zh-glossary.js
@scripts/i18n-zh-lint.js
@scripts/i18n-zh-backtranslate.js
</context>

---

## Live observations — measured on the working tree at planning time (2026-09-07)

Everything below was read off the tree at planning time under MUTABLE-SCOPE AUTHORITY, at HEAD
`e84b41da`, branch `main`, **one worktree**, `git tag --points-at HEAD` **0**.

**The task description's hand-off is corrected in four places by these measurements** — all six
`from` versions, O-simpleSampler's install state, the claim that wave 4f's set of CMake shapes
carries forward, and the assumption that the six carry no external stylesheet. Wave 4a's plan got a
plugin type wrong by trusting a carried assertion; every plan since has re-read. This one re-read.

| | O-simpleAdditive | O-simpleGrain | O-simpleSampler | O-simpleSubtractive | O-Polystutter | O-Orbit |
|---|---|---|---|---|---|---|
| **role in this wave** | **TRACER** | rest | rest | rest | rest | rest |
| **emitter rows** (the emitter's own count) | **131** | **153** | **145** | **133** | **133** | **125** |
| UI root | `Source/ui/public/` | same | same | same | same | **`Resources/ui/`** |
| external CSS | **`css/styles.css` 730 L** | **1129 L** | **939 L** | **743 L** | **none — inline** | **`css/styles.css` 819 L** |
| `index.html` size | 281 L | 341 L | 314 L | 330 L | **1776 L** | 418 L |
| `data-i18n*` hooks in markup | 62 | 81 | 71 | 81 | **104** | 64 |
| `data-tip=` attributes in markup | **0 — tips come from `TIP_BINDINGS`** | 0 | 0 | 0 | 0 | 0 |
| `i18n.js` size | 871 L | 991 L | 866 L | 820 L | 779 L | 751 L |
| `I18N` + `LABELS` keys (check-i18n's own count) | 43 + 45 | 38 + 77 | 36 + 73 | 36 + 61 | 43 + 48 | 34 + 57 |
| tips bound (check-i18n `[2]`) | 43 | 38 | **37** | 36 | **105** | 34 |
| row arithmetic `2·I18N + LABELS` | 131 ✓ | 153 ✓ | 145 ✓ | 133 ✓ | **134 ≠ 133** — see K3 | 125 ✓ |
| `tests/i18n-states.json` states | **2** | **4** | **4** | 3 | 3 | 3 |
| of those, `click` states | **2 of 2** | **4 of 4** | **4 of 4** | **3 of 3** | **3 of 3** | **3 of 3** |
| **gate file in `tests/`** | **NONE** | **NONE** | **NONE** | **NONE** | **NONE** | **NONE** |
| `tests/` actually holds | states + render-harness | same | same | same | states only | states + `ui-stub/` |
| **unscanned module JS under the UI root** | **none** | `modules/webview-drop-streaming.js` | **same file, byte-identical** | **none** | `modules/preset-manager.js` | `js/modules/preset-manager.js` |
| `check-i18n` `[12]` modules scanned | 1 — `js/app.js` | 1 | 1 | 1 | **2** — + `js/parameter-bindings.js` | 1 |
| `font-family` declaration sites | 6 | 11 | 9 | 11 | 12 | **14** |
| of those, `font-family: inherit` | 4 | 8 | 6 | 7 | 5 | **0** |
| distinct declared stacks | 3 | 3 | 3 | 3 | **4** | **1** |
| house stack reaches an INSTALLED face | **yes — Times New Roman** | yes | yes | yes | yes — TNR **and** Georgia | **NO — Garamond 0, EB Garamond 0** |
| SVG `font-family=` attribute carriers | 0 | 0 | 0 | 0 | 0 | 0 |
| canvas `fillText` with a literal | 0 | 0 | 0 | 0 | 0 | 0 |
| existing min-w / min-h / line-h / letter-sp / white-space | 6 / 1 / 3 / 18 / **0** | **12 / 7 / 7 / 28 / 4** | 6 / 4 / 7 / 18 / 4 | 6 / 1 / 3 / 23 / 2 | 4 / 1 / 6 / 14 / 3 | 10 / 2 / 3 / 16 / 4 |
| identity: nodes / DOM keys / display ids | 595 / 298 / 159 | 502 / 251 / 122 | 454 / 227 / 109 | 520 / 260 / 120 | **1068 / 534 / 212** | 360 / 180 / 88 |
| leaves compared / of them ESTIMATED | 102 / **84** | 90 / 70 | 81 / 58 | 92 / 70 | 164 / **154** | 59 / **56** |
| `wrap-count` baseline | 0 | 0 | 0 | **1 — `div.routing-label`** | 0 | 0 |
| `check-ui-labels` coverage line | **40 of 40**, 0 never-visible | 56 of 54, 0 | 48 of 47, 0 | 55 of 54, 0 | **97 of 97**, 0 | **37 of 56, 19 never-visible** |
| **`CMakeLists` VERSION shape** | quoted `"1.2.1"` | **`set()` VARIABLE `"1.4.3"` + hex code** | quoted `"1.4.4"` | quoted `"1.4.1"` | **unquoted `1.14.3`** | **unquoted `1.2.3`** |
| one-arm reader returns | 1.2.1 | **EMPTY** | 1.4.4 | 1.4.1 | 1.14.3 | 1.2.3 |
| two-arm reader returns | 1.2.1 | **1.4.3** | 1.4.4 | 1.4.1 | 1.14.3 | 1.2.3 |
| **shipping version** | **1.3.0** | **1.5.0** | **1.5.0** | **1.5.0** | **1.15.0** | **1.3.0** |
| second version source | — | **`OSIMPLEGRAIN_VERSION_CODE 0x010403`** | — | — | — | — |
| `juce_add_plugin` target | `O-simpleAdditive` | `O-simpleGrain` | `O-simpleSampler` | `O-simpleSubtractive` | **`OPolystutter`** | **`OuariconOrbit`** |
| PLUGIN_CODE | `OSiA` | `OsGr` | `OsSm` | `OSiS` | `OuPs` | `OuOr` |
| IS_SYNTH | TRUE | TRUE | TRUE | TRUE | **FALSE** | **FALSE** |
| `setSize` (parsed frame) | **860 x 930** | 900 x 760 | 980 x 720 | **1180 x 820** | 1000 x 690 | **800 x 600** |
| `PluginProcessor.h` codec block | **L181-182** | **L176-177** | **L179-180** | **L204-205** | **L82-83** | **L103-104** |
| doc comment above the codec | yes | yes | yes | yes | **NO — the wave's only one** | yes |
| language persistence | ValueTree `"uiLanguage"` | ValueTree `kUiLanguageProp` | ValueTree `juce::Identifier(kLanguageProp)` | ValueTree `"uiLanguage"` | ValueTree `"uiLanguage"` | ValueTree `"uiLanguage"` |
| `I18N_EXEMPT` mentions | 4 | 3 | 3 | 4 | 3 | 3 |
| stale LANGUAGE-enumeration probe hits | **2** | **2** | **2** | **2** | **2** | **2** |
| `tip.settings` entry | **ABSENT** | ABSENT | ABSENT | ABSENT | ABSENT | ABSENT |
| `#tips-toggle` in markup | 0 | 0 | 0 | 0 | **2** | 0 |
| duplicate-key scan (F13 corrected form) | NONE | NONE | NONE | NONE | NONE | NONE |
| R3 glossary collisions at planning time | NONE (22 matched) | NONE (27) | NONE (23) | NONE (16) | NONE (34) | NONE (28) |
| `scala-tuning-engine` consumer | **NO** | **NO** | **NO** | **NO** | **NO** | **NO** |
| **`PLUGINS.md` row as found** | **1.2.0 — one patch stale** | **1.4.2 — stale** | **1.4.3 — stale, and ✅ Working** | **1.4.0 — stale** | **1.14.2 — stale** | **1.2.2 — stale** |
| installed bundle | `-dev` only | `-dev` only | **`-dev` only — it IS on disk** | `-dev` only | `-dev` only | `-dev` only |
| submodule under the plugin | — | — | — | — | — | **`libs/SAF` @ `b6fe1882` (v1.3.4), CLEAN** |

**Live total: 820 emitter rows.** `153 + 145 + 133 + 133 + 131 + 125 = 820`, and `5441 − 4621 = 820`
**exactly**. The corpus goes 4621 → **5441** of 5441 and 37 → **43** localized plugins. All six are
installed as `-dev` only; **no alternate-variant orphan on disk for any of the six.**

### Baseline, FIRED at planning time — every one is a number the wave must not regress

Every line below was run against the tree as found. **None is inherited.** (M3 / wave 4e instruction
3 / s71 instruction 4 / wave 4f instruction 7.)

- `node scripts/check-i18n.js` repo-wide: **`ALL CHECKS PASS — 43 localized plugin(s)`, exit 0.**
  **Read K1 before reconciling that 43 against the emitter's 37** — they are different denominators.
- `node scripts/check-i18n.js --plugin <Name>`: **exit 0 on all six**, each reporting
  `[1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr"]`.
- `node scripts/i18n-zh-lint.js` repo-wide: **exit 2** — and read D5/K2 before treating that as a
  failure of anything in this wave. `0 finding(s) across 0 plugin(s)`, **1 plugin could not be read**.
- `node scripts/i18n-zh-lint.js --plugin <Name>`: **exit 0 on all six.**
- `node scripts/i18n-zh-lint.js --self-test`: **10/10, exit 0.**
- `node scripts/i18n-fr-lint.js` repo-wide: **exit 2**, same cause, `0 / 44 plugins with findings`,
  1 could not be read.
- `node scripts/i18n-zh-backtranslate.js` stage view: **4621 of 5441 rows**, `BELOW SHIP BAR … 0`,
  **37** plugins carrying at least one zh row, **1 plugin could not be read**
  (`O-Strata — ERROR no i18n.js under either UI root`).
- `node scripts/check-ui-labels.js --plugin <Name>`: **exit 0, `== ALL CHECKS PASSED ==` on all six**,
  with `[7][GEOMETRY DIFF][fr] no non-label element moved` green on every state. Coverage lines and
  never-visible counts are in the table above and are **part of the baseline**, not decoration.
- `node scripts/measure-ui.js --plugin <Name> --mode box --report all`: run on all six and
  **deliberately reported as a VACUUM for every Han-gated screen** — see K12. The numbers that are
  NOT vacuous are the identity counts, the ESTIMATED denominators, and
  **O-simpleSubtractive's `wrap-count: 1 finding(s)`** (K9).
- `node scripts/boot-all-uis.js --strict-tips`: **exit 0** — `clean: 43/43`, 0 warn, 0 failed,
  **0 DEAD across 0 plugins, exactly 2 late across 1 plugin** (O-Bells `#ref-pitch-knob`,
  `#octave-stretch` — the 4e D2 / 4f D12 census control, unchanged). 3842 rendered text-bearing
  elements, aria-label 798, title 0.
- **Zero gate files.** `find plugins/<Name>/tests -name '*.js'` returns **nothing** on all six.
  Fired with `find`, never a glob, because a zsh glob that matches nothing aborts the whole command
  and reads as an answer (wave 4g instruction 1).
- Installed font families, `system_profiler SPFontsDataType | grep -c "Family: X$"` — see K11.
  **Unchanged across waves 4d, 4e, 4f and this one.**
- `git submodule status` → ` b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af plugins/O-Orbit/libs/SAF (v1.3.4)`
  — **leading space = clean.** `git status --short -- plugins/O-Orbit/libs/SAF` → **empty**.
- git: on `main`, **one worktree**, `git tag --points-at HEAD` **0**. `git status --short` shows
  ` M .claude/agent-memory/research-planning-agent.md` and **two** untracked paths inside
  `plugins/O-Strata/` — see K2, which is a **correction to wave 4f's D5 as it is written**.

---

## Sixteen findings measured at planning time that the carry-forwards do not contain

Wave 4f's F1-F18, D1-D12 and its thirteen explicit wave-4g instructions, wave 4e's N/M rules and
D4/D5, and s71's D2/D3 all apply and are folded into the tasks below — each is answered **by number**
in the coverage audit.

### K1. `check-i18n`'s 43 and the emitter's 37 are DIFFERENT DENOMINATORS, and the wave's headline number is their convergence

`check-i18n` repo-wide has printed `ALL CHECKS PASS — 43 localized plugin(s)` on every wave since
Stage 3, including today, while the emitter reports **37** plugins carrying at least one zh row.
Neither number moved because of a defect: **`check-i18n`'s 43 counts plugins that have an `i18n.js`
at all** (44 total minus O-Strata), and **the emitter's 37 counts plugins that have zh rows in it**.

**That is why "43 of 43" is the correct assertion for this wave's close-out and "43 localized
plugins" alone is not.** Before this wave the two disagree by exactly six — this wave's set. After
it they must be the same 43, and the close-out must state both readings side by side rather than
quoting the one that was already 43 before a byte of work.

The 44th plugin, **O-Strata**, is excluded **by name** and for a stated reason: it has no `i18n.js`
under either UI root, it is at Stage 0, and it is the standing cause of both repo-wide lints' exit 2.
Its emitter row reads `O-Strata  ERROR no i18n.js under either UI root`.

### K2. 4f's D5 is CORRECT in mechanism and STALE in its `git status` line — the untracked set is now TWO FILES, not a directory

Wave 4f recorded `?? plugins/O-Strata/` as the single untracked line and wrote a precondition
asserting `git status --short | grep -c "O-Strata"` reads **1**. On the tree today it reads **2**:

```
?? plugins/O-Strata/.planning/mockups/v1-ui.html
?? plugins/O-Strata/.planning/parameter-spec.md
```

The directory itself is now tracked — a concurrent session committed it — and two planning files
inside it are not. **D5's load-bearing claim is unchanged and is re-confirmed live:** the lints
enumerate `plugins/*` from **disk**, O-Strata still has no `i18n.js`, and both repo-wide lints still
exit 2 with `0 finding(s) across 0 plugin(s)` and `1 plugin(s) could not be read`. The exit code and
the `git status` line were never the baseline; **the finding count and the unreadable count are.**

**A second never-stage path appeared:** `.claude/agent-memory/research-planning-agent.md` is modified
in the working tree. It belongs to a different session. Every task's never-stage list carries it.

### K3. O-Polystutter's row arithmetic is off by one because ONE entry has no body

`2 · I18N + LABELS` reproduces the emitter's row count exactly on five of six. On O-Polystutter it
gives **134** against the emitter's **133**. The cause is measured, not guessed: **`msg-delete-preset`
has a `t` and no non-empty `b`.** Every other one of its 43 I18N entries has both.

**The consequence for authoring is concrete.** An executor that writes a `zh-Hans` block with a `b`
on that entry adds a 134th row the emitter did not ask for and changes a number the close-out
asserts. An executor that assumes every I18N entry has two halves will also mis-chunk its blind
batch. **Mirror the English shape per entry; do not normalise it.**

### K4. Every one of the six PLUGINS.md rows is stale, all by one patch, all from ONE commit — and the description's `from` versions are a faithful read of them

| plugin | `PLUGINS.md` row | `CMakeLists` (two-arm reader) | description said | **ships** |
|---|---|---|---|---|
| O-simpleAdditive | 1.2.0 | **1.2.1** | 1.2.0 → 1.3.0 | **1.3.0** |
| O-simpleGrain | 1.4.2 | **1.4.3** | 1.4.2 → 1.5.0 | **1.5.0** |
| O-simpleSampler | 1.4.3 | **1.4.4** | 1.4.3 → 1.5.0 | **1.5.0** |
| O-simpleSubtractive | 1.4.0 | **1.4.1** | 1.4.0 → 1.5.0 | **1.5.0** |
| O-Polystutter | 1.14.2 | **1.14.3** | 1.14.2 → 1.15.0 | **1.15.0** |
| O-Orbit | 1.2.2 | **1.2.3** | 1.2.2 → 1.3.0 | **1.3.0** |

**All six `from` versions in the task description are FALSE as stated, and all six describe the
registry row rather than the source of truth.** Every plugin's CHANGELOG names the same date for the
stale patch — **2026-09-03** — which is the 2026-09-03 suite-wide French hover-help rename, task
`260903-ukp`. That is the same commit that left O-GrainScatter's row two minors behind in wave 4f.
The registry has now missed a whole suite-wide patch on at least seven plugins.

**The shipping versions are unaffected**, because a MINOR bump zeroes the patch either way. **The
`from` values are not**, and the CHANGELOG entry, the commit message and the registry edit all have
to name the real one. Ship the bump from the CMake value (R9).

### K5. All THREE CMake version shapes appear in this wave, and on one of them the ONE-ARM reader returns EMPTY

Wave 4f reported "two unquoted literals, three quoted, **no `set()`-variable form at all** — the
first wave since 4c with no third shape", and instruction 5 said that does not license a one-arm
reader. **Wave 4g has all three**, and the point is no longer theoretical:

```
O-simpleAdditive    one-arm='1.2.1'  two-arm='1.2.1'
O-simpleGrain       one-arm='EMPTY'  two-arm='1.4.3'      <-- the one-arm reader FAILS
O-simpleSampler     one-arm='1.4.4'  two-arm='1.4.4'
O-simpleSubtractive one-arm='1.4.1'  two-arm='1.4.1'
O-Polystutter       one-arm='1.14.3' two-arm='1.14.3'
O-Orbit             one-arm='1.2.3'  two-arm='1.2.3'
```

O-simpleGrain writes `set(OSIMPLEGRAIN_VERSION "1.4.3")` at L9 and
`VERSION "${OSIMPLEGRAIN_VERSION}"` at L24. A reader that only matches a literal after `VERSION`
matches the `${...}` and extracts nothing. **Use the two-arm reader unconditionally on all six**,
including the four where a one-arm reader also works — a reader that only works where it was written
is not a reader.

### K6. O-simpleGrain has a SECOND version source, it is a hex mirror, and the render-harness compiles it

```
plugins/O-simpleGrain/CMakeLists.txt:9   set(OSIMPLEGRAIN_VERSION "1.4.3")
plugins/O-simpleGrain/CMakeLists.txt:10  set(OSIMPLEGRAIN_VERSION_CODE 0x010403)
plugins/O-simpleGrain/CMakeLists.txt:24      VERSION "${OSIMPLEGRAIN_VERSION}"
plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt:57  JucePlugin_VersionString="${OSIMPLEGRAIN_VERSION}"
plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt:58  JucePlugin_VersionCode=${OSIMPLEGRAIN_VERSION_CODE}
```

The hex encoding is `0x00MMmmpp`. `1.4.3` is `0x010403`; **`1.5.0` is `0x010500`.** The file's own
comment at L5-9 says the pair is the single source of truth and that the hand-rolled
`JucePlugin_VersionString/Code` "previously drifted" — the drift this structure exists to prevent.
Its CHANGELOG at L35 records the mirror being moved deliberately on the last bump.

**No other plugin in the six has a second version source, and no gate in this repo reads the hex.**
A bump that edits only the string ships a render-harness compiled against a stale version code and
nothing catches it. Task 2 moves both, in the same edit, and proves both by grep.

### K7. Not one of the six carries a gate file — and the `tests/` directories are NOT empty, which is what makes a glob dangerous here

```
O-simpleAdditive    tests/ = i18n-states.json  render-harness/
O-simpleGrain       tests/ = i18n-states.json  render-harness/
O-simpleSampler     tests/ = i18n-states.json  render-harness/
O-simpleSubtractive tests/ = i18n-states.json  render-harness/
O-Polystutter       tests/ = i18n-states.json
O-Orbit             tests/ = i18n-states.json  ui-stub/
```

`find plugins/<Name>/tests -name '*.js'` returns **nothing** on all six. Those `render-harness/`
trees are the C++ Stage-2 DSP harnesses (wave 4e D6) and are out of scope; `O-Orbit/tests/ui-stub/`
holds a single `generic-overrides.json` and no JavaScript at all.

**The N12 enumeration-form census has no subject in wave 4g.** Take no invented gate (wave 4c D4: a
gate written mid-localization is a gate nobody has calibrated). **`check-i18n`, `check-ui-labels`,
`measure-ui` and `boot-all-uis` are the whole instrument for this wave.** The remaining N12 surface is
F2's eight already-three-language files, none of which is in this set.

### K8. FOUR of the six carry a module JS file that NO gate in this repo opens, and two of them emit user-facing English

This is a live, larger instance of s71 D2, measured on this wave's own plugins rather than inherited.

| plugin | file | lines | `check-i18n` `[12]` scans | user-facing English |
|---|---|---|---|---|
| O-simpleGrain | `modules/webview-drop-streaming.js` | 504 | **no** — scans `js/app.js` only | **~12 status/error strings** |
| O-simpleSampler | `modules/webview-drop-streaming.js` (**byte-identical**, sha `c7fe612e…`) | 504 | **no** | same 12 |
| O-Polystutter | `modules/preset-manager.js` | 406 | **no** — scans `js/app.js`, `js/parameter-bindings.js` | 7, **5 of them dead** |
| O-Orbit | `js/modules/preset-manager.js` (**divergent copy**, sha `d15e751b…`) | 447 | **no** | 7, **5 of them dead** |

**The discriminator is the DIRECTORY, not the file.** O-Polystutter's `js/parameter-bindings.js` IS
scanned; its `modules/preset-manager.js` is not. And the two module directories are spelled
differently across the wave — `modules/` on three plugins, `js/modules/` on O-Orbit — so **a census
keyed on one path is blind to the other.** Measured the hard way: a first pass of this planning run
globbed `<root>/js/*.js` and missed O-Polystutter's copy entirely.

**The two `preset-manager.js` copies are the benign case and it is measured, not assumed.** Both
consumers took the **explicit-DOM-refs** path (`document.getElementById('preset-prev')`), not the
module's `createPresetBar()` innerHTML block — O-Orbit's `app.js` L1157 says so in a comment. So the
five strings in that block (`"Previous preset"`, `"Next preset"`, `"Load preset from file"`,
`"Save preset"`, `Default` in markup) never reach either page. Both plugins key their preset buttons
in their own markup: O-Orbit `data-i18n-aria="preset-prev"` / `"preset-next"` (both present in its
LABELS), O-Polystutter `data-i18n-aria="aria.presetPrev"`. **What DOES reach the page is
`displayElement.textContent = this.currentPreset`**, writing `'Default'` or `'Loaded Preset'` into a
`#preset-name` span that O-Orbit's own markup comment at L70 says "is NOT a `[data-i18n]` element and
must never become one".

**`webview-drop-streaming.js` is the live case.** It writes nothing to the DOM itself — it hands
strings to `opts.showToast(...)`:

```
:195  if (!isAudio) { opts.showToast('Drop a .wav/.aif on a cell'); return; }
:230  opts.showToast('Scanning folder…');
:235  opts.showToast('No audio files in folder');
```

plus `'Drop a folder, not a file'`, `'Drop session start failed'`, `'File load failed at commit
step'`, `'File transfer failed'`, `'Folder load dialog failed — aborted'`, `'Folder load failed at
commit step'`, `'No samples loaded — all files failed'` and two template forms. **Those toasts render
English on the Chinese page of O-simpleGrain and O-simpleSampler**, they render English on the French
page today, and no gate can see them: they are not `[data-i18n]` elements, `check-i18n` never opens
the file, and no state in `i18n-states.json` fires a drop event.

**This wave REPORTS it and does not fix it**, for the reason s71 gave when it narrowed
`preset-manager.js` out: keying a shared module with two byte-identical copies is a rollout of its
own, it touches a surface with no calibrated gate, and shipping it inside a localization wave would
put an untested toast layer in a commit whose subject is a caption table. **Record the verdict per
plugin with the file cited; carry it as a deferred item.** It is not a licence to invent a gate.

### K9. O-simpleSubtractive's `wrap-count` baseline is 1, and the carry-forward names the wrong plugin

`measure-ui --plugin O-simpleSubtractive --mode box --report all` reports, on the tree as found:

```
wrap-count: 1 finding(s)
    html/body[1]/div[1]/section[2]/div[1]  (div.routing-label)  "Signal Path"  en=1 line(s), fr=2 line(s)
```

Wave 4e D7 records this as "a pre-existing French wrap on **O-simpleFM**", and 4f D12 re-inherits it
under that name. **It is not a property of O-simpleFM.** `routing-label` is a shared class with
carriers in three trees — `O-simpleFM` (markup + CSS), `O-simpleSubtractive` (markup + CSS) and
`O-simpleGrain` (CSS only, no markup carrier). O-simpleFM was measured in wave 4e because O-simpleFM
was in wave 4e; **nobody measured the other carriers, so nobody saw the second one.**

Two consequences, both load-bearing:

1. **O-simpleSubtractive's `wrap-count` baseline is 1, not 0.** A task that treats 1 as a regression
   will chase a French wrap it did not cause; a task that treats 0 as the target will never reach it.
2. Wave 4e D7 also recorded that the **Chinese arm of the same node is one line** on O-simpleFM. That
   is a useful prior for Task 4 and it is a prior, not a measurement of this plugin.

**Report the correction to the carry-forward's SCOPE, not only to its plugin name.** This is the
same shape as F1: a finding derived from one carrier looked measured and had no second site to check
it against.

### K10. O-Orbit's ENTIRE page renders through a stack that names no installed family — the rollout's largest single font surface

Every other plugin in this wave, and every plugin in waves 4a-4f, declares at least one stack whose
Latin reaches an installed face. O-Orbit declares **one stack, fourteen times, and zero `inherit`:**

```css
font-family: Garamond, 'EB Garamond', serif;
```

Against the machine table (K11): **Garamond 0, EB Garamond 0.** The only survivor is the bare
`serif`, and Chromium resolves a bare generic against the document's `lang`. Under `lang="zh-Hans"`
that is a Chinese face — **for the Latin text too.** So on O-Orbit the Latin arm does not merely risk
moving when Chinese lands; it changes typeface on every ASCII node on the page.

The other five are the controlled contrast, and their stacks are Latin-safe **because of a family the
declaration lists third or fourth, not the one it names first**:

| plugin | stack | first installed member |
|---|---|---|
| O-simpleAdditive / Grain / Sampler / Subtractive | `'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', serif` | **Times New Roman (4th)** |
| O-Polystutter | `'Garamond', 'Times New Roman', serif` **and** `'Georgia', serif` | **Times New Roman (2nd)** / **Georgia (1st)** |
| **O-Orbit** | `Garamond, 'EB Garamond', serif` | **NONE** |

**A grep for a bare generic finds nothing on any of the six**, because every declaration names
families. It just names ones that do not exist. **The census, not the grep, is the instrument**
(wave 4c C4).

Four of the six also define `--symbol-font: 'Segoe UI Symbol', 'Apple Symbols', 'Noto Sans Symbols2',
'Arial Unicode MS', serif`. Segoe UI Symbol and Noto Sans Symbols2 are absent; **Apple Symbols (1)
and Arial Unicode MS (1) are present**, so the token is not naked. N3 still applies: **if a node
using it is a TIP ANCHOR it needs the CJK tail regardless of what glyphs it renders**, because
`measure-ui`'s `han` counts `data-tip`, `data-tip-title` and `aria-label`.

### K11. The installed-family table, re-derived on this machine (M5) — unchanged across four waves

`system_profiler SPFontsDataType | grep -c "Family: X$"`:

| present | | absent | |
|---|---|---|---|
| Georgia | 4 | Garamond | **0** |
| Times New Roman | 4 | EB Garamond | **0** |
| Arial | 4 | Adobe Garamond Pro | **0** |
| Menlo | 4 | Microsoft YaHei | **0** |
| Courier New | 4 | Consolas | 0 |
| **PingFang SC** | **6** | Segoe UI Symbol | 0 |
| **Songti SC** | **4** | Noto Sans Symbols2 | 0 |
| Helvetica Neue | 14 | | |
| Arial Unicode MS | 1 | | |
| Apple Symbols | 1 | | |

Identical to waves 4d, 4e and 4f. **And a zero here is a claim about the QUERY, not about the
machine** (F5, confirmed twice on `Times`): `system_profiler` proves presence, never absence. Where a
resolved face is load-bearing — which on O-Orbit is the whole page — **probe it through CDP
`CSS.getPlatformFontsForNode`, not through this table.**

The consequence for this wave: `'PingFang SC', 'Microsoft YaHei', serif` remains the correct tail
(PingFang is the one that resolves here; YaHei is the Windows arm and its 0 is expected), and the
face-naming half is **one plugin, fourteen sites** rather than the two or three nodes of earlier
waves.

### K12. The Han-gated screens were run and are a VACUUM — the proxies are the measurement (Q11, wave 4c instruction 1)

`measure-ui --mode box --report all` was fired on all six. Every Han-gated screen reads 0 **because
no plugin holds a Han codepoint yet**, not because the pages are clean:

```
undeclared-font:      0 finding(s)   on all six   — the input set is EMPTY
line-height-normal:   0 finding(s)   on all six   — same
svg-font-attr:        0 attribute carrier(s), 0 finding(s)   on all six
```

**Report them as a vacuum and re-fire every one after the table lands** (ship-bar step 6). The
numbers from the same runs that are NOT vacuous, and which the wave must not regress:

- the **identity** line on each (595 / 502 / 454 / 520 / 1068 / 360 nodes) — a non-zero input count;
- the **ESTIMATED denominator** on each (84 of 102, 70 of 90, 58 of 81, 70 of 92, **154 of 164**,
  **56 of 59**). Read F8 before treating any post-work `wrap-count` delta as a defect: a ratio pin
  moves a node out of the estimated set and changes the divisor with no pixel changing, and the
  direction is not guaranteed. **O-Polystutter (94% estimated) and O-Orbit (95%) are the two most
  exposed pages in the rollout to that effect;**
- **O-simpleSubtractive's `wrap-count: 1`** (K9), which is not Han-gated and is real today.

### K13. Both stale-body classes: the pair enumeration is present on all six in six spellings, and the "nothing else" class has NO SUBJECT

The comment-stripped probe, run with `perl -CSD` (F11 — without it the French U+2019 branch is
byte-blind), reports **exactly 2 hits on every one of the six**, and on all six they are the same
two: the fixed-pair clause in the **en** body and again in the **fr** body of the language entry.

| plugin | the English clause, verbatim |
|---|---|
| O-simpleAdditive | `English and French are available; value readouts and the two drop-down menus stay in English.` |
| O-simpleGrain | `English and French are available; value readouts and the two drop-down menus stay in English.` |
| O-simpleSampler | `English and French are available; value readouts and the two drop-down menus stay in English.` |
| O-simpleSubtractive | `English and French are available; value readouts and the four drop-down menus stay in English.` |
| O-Polystutter | `English and French are available; value readouts, note divisions and preset names stay in English.` |
| O-Orbit | `English and French are available; value readouts and preset names stay in English.` |

**Only the pair clause takes the deletion; the exception list stays, and its NUMERIC claims were
checked and hold.** `<select>` counts in markup are 3 / 3 / 3 / 5 / 9 / 7, and subtracting the
language selector itself gives 2 / 2 / 2 / 4 — matching "the two drop-down menus" on three plugins
and "the four drop-down menus" on O-simpleSubtractive exactly. O-Polystutter and O-Orbit name no
number. **Re-verify per plugin before deleting anything: a hidden preset dropdown could change the
count, and the claim is in the body an executor is about to rewrite.**

**The "and nothing else" settings class has no subject in wave 4g.** `tip.settings` **does not exist
on any of the six** (grep count 0, all six), and the probe returned no `and nothing else` /
`ne contient que` / `rien d’autre` hit anywhere. Five of six have no `#tips-toggle` in markup at all;
**only O-Polystutter has one** (2 mentions, 6 `settings-popover` mentions), and it has no settings
body making a false exclusivity claim about it. **Read the entry, record the checked non-defect
citing the key, and move on** — five waves of mechanical deletion have made deletion the reflex, and
here the reflex has nothing to act on.

The "the Tuning tab stays English" class also has no subject: none of the six is a
`scala-tuning-engine` consumer and none has a tuning tab.

### K14. O-Orbit reports NINETEEN never-visible labels — the largest in the rollout — and every one shown is structurally unmeasurable

```
37 of 56 [data-i18n] elements were VISIBLE in at least one state
19 never became visible and were therefore NEVER MEASURED:
    #layout-select>option:nth-child(1)
    #path>option:nth-child(1) … nth-child(5)
    #tempo_sync>option:nth-child(1), (13), (14), (15)
    #speaker_layout>option:nth-child(1), (2)
    [the gate prints 12 and truncates]
```

Wave 4f's four consumers reported 10 each and s71 D5 identified all ten. **O-Orbit's are the same
CLASS and nearly double the count**: `<option>` elements inside collapsed `<select>`s, which are
**structurally unmeasurable by any DOM geometry probe, not merely unmeasured** (N8, D4). O-Orbit has
seven `<select>` elements.

**Two things follow, and both are traps.** First, `undeclared-font` cannot see a collapsed
`<select>`'s selected-option Han either (F12) — `han` is the node's own text plus `data-tip`,
`data-tip-title`, `aria-label`, and a selected option's text is none of those. **The screen is not
the oracle for whether those controls paint Chinese; the CJK-tail decision covers them anyway, on a
decision about what the control paints.** Second, **the gate itself prints only the first 12 and
truncates** — the same shape as `--ingest`'s default twelve (R2). **Read the count, not the list**,
and do not treat the seven it did not print as absent.

The other five all report **0 never-visible**, which makes O-Orbit's 19 a per-plugin property and not
a machine one. Record it as measured.

### K15. `N of M` is not a fraction and N exceeds M on three of six — F18 confirmed a third time

`56 of 54` (O-simpleGrain), `48 of 47` (O-simpleSampler), `55 of 54` (O-simpleSubtractive). N counts
paths seen visible across the cumulative walk; M is final-DOM `[data-i18n]` membership. A plugin that
re-renders a subtree between states produces N > M routinely. **Do not read it as a failure**, and do
not "fix" it.

The exact-fraction pair — **O-simpleAdditive 40 of 40** and **O-Polystutter 97 of 97** — are the
controlled contrast: both walk without re-rendering. That is one of the reasons O-simpleAdditive is
the tracer.

### K16. Every state on every plugin is a `click` state — nineteen of them, and each needs the state-EFFECT assertion

Wave 4f's tracer had one `click` state and one `eval` state and could skip the probe on the second.
**Wave 4g has no `eval` states at all:** 2 + 4 + 4 + 3 + 3 + 3 = **19 click states**, every one of
which needs the wave-4e N1 assertion — `elementFromPoint` at the target's centre must return the
target or a descendant, and the state must measurably take effect.

Discriminate on the rect (N1): a **different element** at the point is a coverage hole; a **`null`**
means the target is below the fold in an `overflow-y: auto` pane and Playwright scrolled it into
view, which is not a defect. **And a scrolling click that is not the LAST state scrolls every state
after it.**

The state names are self-documenting and worth reading before firing — several name the exact label
they exist to reveal (`label.captionPitchedBuzz` is described in O-simpleGrain's own states file as
*"the widest of the eight captions in French — 997.22 px natural NOWRAP"*, which is a French
measurement an executor should re-take rather than inherit).

---

## Execution shape — every long command runs in the BACKGROUND

**Non-negotiable, and the reason is a measured failure.** Wave 4d's first executor stalled on the
600-second no-progress watchdog **three times**, and a stalled agent's context cannot be resumed —
the work was lost and a fresh executor had to re-derive it. Every command in this plan that can
exceed ~60 seconds MUST be dispatched with `run_in_background: true` and polled, never run in the
foreground.

That covers, without exception:

| command class | why it stalls |
|---|---|
| `./scripts/build-and-install.sh <Folder>` | a full CMake + ninja build, minutes |
| `auval -a` / `auval -v <type> <subtype> <mfr>` | a cold registry rescan |
| `node scripts/measure-ui.js …` | Playwright, every state, three languages |
| `node scripts/check-ui-labels.js …` | same |
| `node scripts/boot-all-uis.js --strict-tips` | boots all 43 UIs — measured ~4 min |
| `node scripts/i18n-zh-lint.js` (repo-wide) | 44 plugins, ~2841 entries |
| the blind reverse dispatch (`claude -p …`) | an external agent reading up to ~92 rows |

Poll with an until-loop on a sentinel written by the backgrounded command itself
(`echo ALLDONE >> …`), not with a fixed `sleep`. **`timeout` does not exist on this machine.**

**A trap this planning pass hit and every executor will hit:** a `{ …; } &` inside a command that is
itself dispatched in the background double-backgrounds it — the wrapper exits immediately and reports
success while the real work is still running. **Poll the sentinel, never the wrapper's exit.**

**The auval budget is s71's corrected one, not the stale 15-minute figure.** Measured on this machine
2026-09-06 and again in wave 4f: `auval -a` after five consecutive installs took **83-84 seconds**,
and five `auval -v` runs together took **1 second**. Budget **~85 s** for the cold rescan. Run it in
the background and poll a sentinel anyway — the watchdog does not care how fast a command usually is.

**Each task below is independently executable by a FRESH executor.** Each carries its own required
reading, its own preconditions, its own measured numbers, the full ship bar, the submodule guard and
the commit discipline. **No task may assume another task's context.**

---

## Stage-4 executor rules — non-negotiable, cited by number below

Carried from Stage 3 (R1-R9) and wave 4a (W1-W5), with waves 4b-4f and s71 folded in, and with wave
4f's structural corrections **F9, F11, F13, F14, F15 and F18 folded into the rule text itself**.
**Every task below restates the subset it needs; this block is the full text.**

- **R1 — Author at `'mt'`, always.** Never write `'bt'` at authoring time. `check-i18n` accepts `'bt'`
  as a valid enum member and the lint's R1 only reports entries *below* the bar, so a premature
  `'bt'` is invisible to every automated check in this repo. Promotion happens only after the reverse
  read, in its own commit. **And read the emitter's `rows` column rather than estimating a budget.**
  **Wave 4f's stated exception does NOT apply to wave 4g** — there is no shared-module column to copy
  at `'bt'`, because none of the six is a `scala-tuning-engine` consumer. **Every row in this wave is
  authored at `'mt'` and promoted only after a blind read. There is no exempt row.**
- **R2 — Read ALL triples with `--verbose`.** `--ingest` truncates to 12. Across seven prior waves
  essentially no real finding ranked inside the default twelve. The score is a sort key, not a
  verdict. **`check-ui-labels`' never-visible list truncates at 12 the same way** (K14).
- **R3 — Screen `TERMS` for page collisions BEFORE authoring.** Push the plugin's English label set
  through `TERMS` in `scripts/i18n-zh-glossary.js` and report any two *different* English keys mapping
  to one root that co-occur on a page. Qualify **both** sides, never one — **except** in M13's shape,
  where two *different* roots collide; there, qualify the side that is ambiguous about what it does
  and **write down why the other was left**. **Fired at planning time on all six: NONE** (22 / 27 /
  23 / 16 / 34 / 28 glossary-matched captions). **That screen only fires when both sides are glossary
  keys and it matched only full captions — M13's two-different-roots collision is invisible to all of
  it and only the reverse read sees it.** Re-fire it per plugin and run the mechanical downstream
  check after authoring.
- **R4 — Fresh blind reader AND fresh salt per correction round.** They guard different things: the
  salt stops correlation against seen ids, the fresh agent stops self-agreement. A correction round
  also needs a **different model**. **Set `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1`, or read stderr
  and record the model that ACTUALLY ran** — `claude -p --model claude-opus-4-1` is silently remapped
  to Opus 5 and a summary that records the requested name names a model that did not run (F15).
  A round that corrects nothing needs no round two; say so in the commit.
- **R5 / W4 / F16 — `--emit` is CORPUS-scoped and its own usage line is wrong. Pass BOTH `--emit
  <Plugin>` AND `--plugin <Plugin>`.** Confirmed live at planning time: `const target = val('--emit')`
  at `scripts/i18n-zh-backtranslate.js:161` and `const only = val('--plugin')` at `:94` are separate,
  and the usage line at `:68` shows `--emit O-Chorus --out /tmp/b.tsv` with no `--plugin`. Following
  it emits the entire corpus — **4621** already-shipped rows dispatched to an external reader as this
  plugin's work. The emitter has **no row-range flag**, so chunking is done on the emitted file with
  the manifest as the key.
- **R6 / F9 / F10 — the blinding controls, with the two corrections that make them non-vacuous.**
  Pass `--manifest` explicitly. ids pure 12-hex, no key fragment, **returned identical AND IN ORDER**
  (M12 — a reader can drop a row and return the right line count), rows well-formed with no Han
  surviving in the returned English, and a sha256 of the zh column against the committed tree.
  **Forbid repo access in the dispatch explicitly**, dispatch from a cwd **outside** the repo with
  `--allowed-tools ""`.
  - **`--forward-provenance` takes a VALUE, not a bare flag (F9).** It is
    `val('--forward-provenance')` at `:228`, confirmed live. Written bare as
    `--forward-provenance --out /tmp/f`, the manifest records `forwardProvenance: "--out"`, the emit
    succeeds, the ingest joins, **and the refusal control passes while the identity check is
    vacuous.** Write it as `--forward-provenance "<what produced the Chinese, and when>"`.
  - **The ingest's refusal ORDER is three steps and only the third is about the manifest (F10):**
    reverse-pass `--provenance` → forward provenance → id join. A wrong-manifest control run without
    `--provenance` refuses for the *first* reason and proves nothing. **Fire BOTH refusal controls
    with a properly provenanced emit**, and record the message each actually printed. The
    wrong-manifest one reads:
    `WRONG MANIFEST FOR THIS BATCH: all N returned ids are unjoinable, not one.`
  - Expect the "no product name in the batch" control to report a **non-zero** count on any plugin
    whose own copy names itself. **Read the hit and adjudicate it; do not treat the count as
    pass/fail.**
- **R7 — zh geometry failures are SHRINKS. Assert equality, pin at the widest of three.**
- **R8 / F7 — a French-era `min-width` pin is a FLOOR, not safety, and a floor measured while in
  place is not measured.** Re-measure every one. Grep for existing declarations on a selector before
  appending, or a new pin at higher specificity silently un-pins a French one and moves the **fr**
  arm. **R8 is LIVE on all six** — the measured pin surfaces are 6/1/3/18/0, 12/7/7/28/4, 6/4/7/18/4,
  6/1/3/23/2, 4/1/6/14/3, 10/2/3/16/4 (min-w / min-h / line-h / letter-sp / white-space) and the plan
  predicts it dead on none. **And `min-width: 0` is NOT "no floor"** — the flex-item default is `auto`
  (content size), so `0` explicitly permits shrinking BELOW content and reads a false natural box.
  **Remove the declaration entirely, measure, restore.**
- **R9 — Version truth is `CMakeLists.txt`.** Read the **value**; never grep for a literal; the value
  may be quoted, unquoted, or held in a variable. **Use the two-arm reader unconditionally.** All
  three shapes are in this wave and **the one-arm reader returns EMPTY on O-simpleGrain** (K5). And
  on O-simpleGrain **there is a second version source** — the hex mirror (K6).
- **W1 — The CJK tail goes BEFORE the trailing generic.** Chromium resolves a bare `serif`,
  `sans-serif` or `monospace` against the document's `lang`, so under `zh-Hans` the generic is already
  a Chinese face and a tail written after it is never consulted. The shipped convention, copied
  verbatim from `plugins/O-Detune/Source/ui/public/index.html:57` and `:481`, is
  `…, 'PingFang SC', 'Microsoft YaHei', serif` for the serif arm and
  `Arial, 'PingFang SC', 'Microsoft YaHei', sans-serif` for a form-4 rule.
- **W2 — OR the visibility flag across states.** `measure-ui` already does this and discloses the
  identity ratio on every run; any hand analysis must too.
- **W3 / N8 — Check `I18N_EXEMPT` membership BOTH ways.** Every exempt option word in an English body
  must survive verbatim into the Chinese; every non-exempt caption must not, and must be named by its
  localized caption instead. Units in a range clause follow the READOUT, not the prose. **One body can
  need both directions.** Mentions this wave: 4 / 3 / 3 / 4 / 3 / 3.
- **W5 — Expect a Z8 violation after any Latin→Han token swap.** A space that is correct while a token
  is Latin becomes an intra-Han space the moment it is replaced.
- **C3 — Measure at the SHIPPING FRAME and walk the states CUMULATIVELY.** `measure-ui` parses the
  frame from each plugin's own `PluginEditor.cpp`. The frames are **860x930 / 900x760 / 980x720 /
  1180x820 / 1000x690 / 800x600** — six different frames, no two alike. Do not substitute a
  hand-rolled sweep at a chosen viewport.
- **C5 / C6 — DERIVE-OR-ABORT on every repaired gate, and fire the control.** **NO SUBJECT IN WAVE
  4G** (K7) — there is no plugin-owned gate file to repair on any of the six. The rule is restated so
  that its absence is a recorded verdict rather than an omission. **Do not write one to satisfy it.**
- **C7 — Author the new language from the CORRECTED English.** Make the enumeration deletions FIRST,
  **re-read the file**, then author from the file as it then stands. No tool in this repo compares a
  `zh` body against its own `en`; only the reverse read catches a row that outlives the sentence it
  was translated from. **This wave's correction surface is two hits per plugin and it is uniform**
  (K13) — the pair clause in en and fr, in six different spellings, with a numeric exception list
  that must be re-verified before it is preserved.
- **C8 — Do not spell the superseded literal in the comment that explains it.** Word it as prose, or
  this wave's own probes keep reporting a file that is fixed.
- **N1 — the `line-height-normal` screen CANNOT reach 0 on most pages.** `han` is computed over the
  node's own text **plus `data-tip`, `data-tip-title` and `aria-label`**, so a numeric readout carrying
  a Chinese `aria-label` is a finding forever. **The load-bearing criterion is `check-ui-labels`
  reporting 0 moved.** Pin every leaf that MOVES; leave every evidenced non-mover and **name each
  residual with its measured `enH == zhH`**. **And the residual count is not a caption count (F4)** —
  the screen's "leaf nodes only" filter is `kids === 0` with no text requirement, so a text-free
  `<canvas>` with a Chinese `aria-label` lands in it and can never move and can never be pinned.
- **N3 — a `--symbol-font`-class token needs the CJK tail even when every glyph it renders is a
  symbol**, if the node is a TIP ANCHOR. The discriminator is whether the node is an anchor, not what
  it renders. **Four of the six define that token** (K10) — check each carrier and record the verdict.
- **N4 — a `termNote` is ENTRY-scoped.** A term used as both a caption and a tooltip title needs
  **two** notes, or Z5 fires on the title with the caption already clear. **And before writing a
  termNote against a glossary root, check the corpus site count** (D1's general lesson): a root derived
  from ONE plugin's caption+title pair looks measured and had no second site to check it against.
- **N4 (wave 4e) — pin caption ROWS as well as leaves, and use a RATIO.** `getBoundingClientRect()` on
  an inline element reports its font box, not its line box, so a row can grow 2-3px with every leaf
  inside it reporting an identical box. A length pinned to a measured row height inherits as a computed
  length and grew one arm 0.77px.
- **N9 — budget for FLOORS, not clips.** Chinese is shorter, so nearly every non-`line-height` pin is
  a `min-width` or `min-height` floor. Set a width floor to the **exact measured English box**, not
  rounded up, so neither Latin arm moves. Never fixed: a floor must not cap a language that needs a
  wider sentence or a third line. **Expect a shrink in BOTH axes.**
- **N10 / N11 / M8 — derive every `line-height` pin from the CONTENT box, PER LINE and PER SIZE.**
  Subtract `padding-top + padding-bottom + border-top + border-bottom` **first**, then divide, then
  check the LINE COUNT. M8's measured English content line boxes on this machine: 6px→1.0,
  7px→1.1428571, 8px→1.125, 9px→1.1111111, 9.5px→1.0526316, 10px→1.1, 11px→1.0909091, 12px→1.1666667,
  18px→1.1666667. **M8's table is LEAF-scoped and does not cover a form control (F6)** — a `<button>`
  at 10px has a 13px content box, ratio 1.3, because the UA `font` shorthand resets `line-height` on
  form controls. **Derive form-control ratios from the element's own box; never from the table.**
- **M9 — look for hidden-at-census leaves.** Where `check-ui-labels` names a moved container whose
  children did not move, a leaf inside it is `display: none` at census time.
- **M11 — hold ONE Latin/Han spacing form across the whole table.** Z4's consistency half is
  **table-scoped**: it counts every Latin↔Han boundary in the file and flags every row using the
  minority form. The shipped corpus uses the **SPACED** form. Full-width parentheses create no
  boundary at all (`（` is `Script=Common`).
- **M12 — check the returned ids are identical AND IN ORDER before ingesting.** Wave 4e had six blind
  chunks and one reader fault: 55 lines returned for a 56-row batch, and only the ORDER diff named
  which row.
- **N1 (wave 4e) — fire the state-EFFECT assertion on every `click` state.** **All nineteen states in
  this wave are `click` states** (K16). Two failure modes, only one a defect: a **different element**
  at the point is a coverage hole; a **`null`** means the target is below the fold and Playwright
  scrolled it into view. Discriminate on the rect. A scrolling click that is not the LAST state
  scrolls every state after it.
- **N8 (wave 4e) — read the `never became visible` NOTE and check the state classes.** **O-Orbit
  reports 19, the rollout's largest** (K14); the other five report 0. Nine-of-ten in wave 4f and all
  nineteen shown here are `<option>` inside a collapsed `<select>` — **structurally unmeasurable by
  any DOM geometry probe, not merely unmeasured.** Record them as such rather than chasing them.
- **`N of M` is NOT a fraction and N can EXCEED M (F18).** `56 of 54`, `48 of 47`, `55 of 54` on this
  wave's own tree. Do not read it as a failure.
- **F17 — chunk on the CONCEPT SPLIT, not on size.** Sending a caption and its own tooltip title to
  two different readers is what made O-GrainScatter's `label.stutterGate` / `tip.stutterGate`
  divergence visible at all. Two emits of the same target share **0** blinded ids, so each chunk
  carries its own salt for free.
- **Two nodes a keyed scan always misses:** the `#tooltip` surface (filled from `data-tip` at hover
  time, never keyed) and the endonym `<option>` (the only Han in the markup). Universal — and on all
  six of this wave's plugins **markup carries no `data-tip` attributes at all**, so the tooltip
  surface is driven entirely from `TIP_BINDINGS` and is that much easier to overlook.

### Tooling traps confirmed live during this planning pass

- **A zsh glob that matches nothing ABORTS the whole command**, including the parts that would have
  matched. Hit twice this pass: once on `--include=*.h` (unquoted) and once on a `js/*.js` census
  that silently missed O-Polystutter's `modules/preset-manager.js` (K8). **Use `find`, or quote every
  `--include` glob, or guard with `(N)`.**
- **A census's scope is a property of the QUERY.** `find <root>/js -name '*.js'` and
  `find <root> -name '*.js'` return different answers on four of the six. Census the whole root.
- **`grep -P` with a Han property byte-matches on this machine** and reports phantom hits on the
  middle dot U+00B7. Use the `perl -CSD` form. **A Han script property in perl WITHOUT `-CSD` is
  inert**, and the same omission makes the French `rien d’autre` (U+2019, three UTF-8 bytes) invisible
  to a `.`-wildcard alternation (F11). **Grep the French clause separately, or run with `-CSD`.**
- **`perl -0777` piped into a shell variable then `grep -c`'d silently returns empty** on large files.
  **Write the stripped file to a temp path and grep the FILE.**
- **Comment-blind greps lie.** Strip comments before any code count:
  `perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' <file>`.
- **The duplicate-key scan must EXCLUDE the declared `LANGUAGES` members (F13).** The bare form
  `matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)` returns `zh-Hans` on every three-language table, because a
  multi-line `'zh-Hans':` block starts a line. Under the corrected form all six read `duplicates
  NONE` at planning time — **which is what makes s71 D8's collision class detectable at all.**
- **`auval -v` takes THREE UNQUOTED words** (M14). `auval -v "aumu OsGr OuDv"` prints
  `Invalid Arguments: auval` and no further output, which across six plugins reads exactly like six
  validation failures.
- **`./scripts/build-and-install.sh` resolves the `juce_add_plugin` target itself, so pass the FOLDER
  name** — load-bearing on **two** plugins this wave, whose targets are `OPolystutter` and
  `OuariconOrbit`.
- **`timeout` does not exist on this machine**, and **zsh does not word-split an unquoted variable**
  (`set -- $PV` in a `for PV in "O-Orbit 1.3.0" …` loop hands the body one path — run those loops
  under `bash -c`).
- **`awk '/name: X/,/^  - name: /'` range patterns collapse to one line** — awk tests the END pattern
  on the START line. Working form: `awk '/- name: X/{f=1;next} f&&/^  - name: /{f=0} f'`.
- **Six recorded helpers that are not usable as written (F14), all RE-VERIFIED live this pass:**
  1. `grep -A14 "line-height: 1.11" | grep -c tuning-container` reads 0 — `d848337a` writes the
     selectors BEFORE the declaration. Working form: **`grep -B14`**. *(No subject in wave 4g — no
     consumer — but the mechanism generalises to any selector-list-before-declaration grep.)*
  2. **`serve(root, onMiss)` in `scripts/serve-ui.js:500` returns a Promise resolving
     `{ server, port, close }` and has NO `url` field.** `page.goto(srv.url)` fails with
     `url: expected string, got undefined`. Build it: `` `http://127.0.0.1:${srv.port}/` ``.
  3. **`buildRoot(name, opts)` at `:377` returns an OBJECT** `{ root, plugin, uiRoot, uiRootLabel, … }`
     — pass `built.root` to `serve()`, not the return value.
  4. **`resolvePlaywright()` at `:524` returns `require(c)` — the MODULE, not a path.** Passing it to
     `require()` throws `ERR_INVALID_ARG_TYPE`.
  5. **A `'bt'` string literal inside `node -e '…'` under bash loses its quotes** and emits
     `reviewed: bt`, a bare identifier, which throws `ReferenceError: bt is not defined` on the next
     parse. **Write promotion scripts to a FILE.**
  6. **`git commit -- <paths> -F <file>` fails** with `pathspec '-F' did not match any file(s)`.
     Options go BEFORE the `--`: **`git commit -F <file> -- <paths>`**.
- **A `{ …; } &` inside a backgrounded command double-backgrounds it** — the wrapper exits and reports
  success while the work runs on. Poll the sentinel.
- **Comment-stripped line numbers are NOT the numbers you edit.** Every raw line number cited in
  K1-K16 is a **raw** number. Census with the stripped file; edit with the raw one. **And never cite a
  raw line number as a worklist** (wave 4g instruction 11) — verify by pattern, and record a grep-able
  token rather than a line number, because line numbers drift in days.

---

## THE SHIP BAR — the ten-step per-plugin procedure

Every plugin task runs this loop. Written once here in full; **each task restates it in its own
`<action>` with that plugin's measured numbers substituted**, so a fresh executor dispatched to any
single task has the whole bar in front of it.

1. **Fire the preconditions (M3).** Every one, in the background, on the tree as found, BEFORE any
   edit. Never inherit a green. **Read K2 before treating a repo-wide lint exit code as a verdict.**
2. **Screen for glossary page collisions (R3)** — before authoring a string. Push the plugin's English
   label set through `TERMS`; report any two different keys sharing a root that co-occur on a page.
   **Fired at planning time on all six: NONE.** Then run the **mechanical downstream check** after
   authoring: group the zh renderings and report any two *different* keys sharing one, excluding
   same-control pairs (a caption and its own tooltip title) and same-English pairs. The R3 screen
   alone only fires when both sides are glossary keys — **M13's two-different-roots collision is
   invisible to all of it and only the reverse read sees it.**
3. **Correct the stale bodies FIRST (C7, K13), then RE-READ the file.** In wave 4g there is exactly
   **one** live class and two dead ones, and each verdict must be recorded rather than assumed:
   - **LIVE — the fixed-pair enumeration** in the language-selector body, in the **en body and the fr
     body both**. All six carry it, in six different spellings. **Delete the pair clause only; the
     exception list stays** — and **re-verify its numeric claim** against the `<select>` count in the
     markup before preserving it (K13);
   - **DEAD — the "and nothing else" settings clause.** `tip.settings` does not exist on any of the
     six. **Read the entry list, confirm its absence, record the checked non-defect citing the grep;**
   - **DEAD — the "the Tuning tab stays English" clause.** None of the six is a `scala-tuning-engine`
     consumer and none has a tuning tab. **Record it as measured.**
   Then re-read the file and author step 4 from the file as it then stands. **No tool in this repo
   compares a zh body against its own en.**
4. **`js/i18n.js` — the table.** Add the Chinese code to `LANGUAGES` and a `'zh-Hans'` value on
   **every** key in both `I18N` and `LABELS`, at `reviewed: 'mt'` (R1). **There is no exempt row in
   this wave.** Hold one Latin/Han spacing form throughout — the corpus form is **SPACED** (M11).
   **Mirror the English SHAPE per entry, do not normalise it** — O-Polystutter's `msg-delete-preset`
   has a `t` and no body, and adding one changes the emitter's row count (K3). Run the duplicate-key
   scan **in its F13-corrected form** before and after.
5. **`index.html` — the endonym.** `<option value="zh-Hans">` beside the fr option, its content
   **copied byte-for-byte from `plugins/O-Detune/Source/ui/public/index.html:1039`**:
   `<option value="zh-Hans">&#31616;&#20307;&#20013;&#25991;</option>` — and never retyped. **All six
   files already write the French endonym as `Fran&ccedil;ais`**, verified per plugin at planning
   time, so the HTML-entity convention is confirmed. Add an `I18N_EXEMPT` entry for the endonym.
6. **Font work (W1, K10, K11).** Serve the page, switch to Chinese, and run
   `node scripts/measure-ui.js --plugin <Name> --mode box --report all` — **after** the table lands,
   because before it the Han-gated screens are a vacuum (K12). Then:
   - **the tail half**, on stacks whose Latin is already safe: append `'PingFang SC', 'Microsoft
     YaHei'` **before** the trailing generic. **On five of six this is the whole font worklist**,
     because every one of their stacks reaches an installed Times New Roman or Georgia;
   - **the face-naming half**, on declarations whose only survivor is a generic. **One plugin,
     O-Orbit, and it is its entire page** — `Garamond, 'EB Garamond', serif` at fourteen sites with
     zero `inherit`. Name an installed face **first** (so the en and fr arms do not move) **and** add
     the tail. **Verify the resolved face through CDP `CSS.getPlatformFontsForNode`, not through the
     installed-family table** (F5);
   - **the form-4 half**, on any node `undeclared-font` names once the table lands, using **this**
     plugin's house stack with the Latin face FIRST.
   Record which sites took which treatment and why, and **verify by re-running the screen and checking
   that the OTHER stack populations did not move** (M7) — never by reading the CSS. **Note where the
   CSS lives:** five of six have an external `css/styles.css`; only O-Polystutter is fully inline.
7. **`Source/PluginProcessor.h`** — widen the two-way `languageCode` / `languageIndex` ternary to
   three-way, pure ASCII, at this plugin's line numbers. **Five of six carry the doc comment above it;
   O-Polystutter does not** — so the verify strips comments before counting either way, because a
   comment-blind count reports success on a file whose code never changed on the five that do. There
   is no language `AudioParameterChoice` on any of the six — re-confirm by scanning each declaration's
   own text, not the containing file. Zero Chinese characters anywhere under `Source/`, proved with a
   `perl -CSD` negative grep **beside a positive control on the same run**.
8. **Gate repair — NO SUBJECT (K7, C5/C6).** None of the six owns a gate file. **Verify with `find`,
   record the zero as measured, and take no invented gate.** This step exists so its absence is a
   verdict rather than an omission.
9. **Geometry pass (R7, R8, N1, N4, N9, N10, N11, M8, M9).** Run the zh arm of `check-ui-labels` and
   let it *name* the offenders, plus `measure-ui --report line-height-normal` and `--report
   wrap-count`. Expect four shapes: `line-height: normal` inheritance (pin each named **mover** at its
   measured English **content** line box, per line, unitless, never global); a caption ROW that grew
   with no leaf reporting it (pin the row as a RATIO); a content-sized element that shrank and pulled
   its neighbours (a **floor** at the exact measured English box, never a fixed size); and Latin
   display conventions applied to Han, where `letter-spacing` and generous side padding are trimmed
   under `html[lang="zh-Hans"]` **with the trim value MEASURED** — the shipped precedent for that
   selector form is `plugins/O-Prism/Source/ui/public/index.html:1298`. **The letter-spacing surface is
   the wave's largest: 18 / 28 / 18 / 23 / 14 / 16 declarations.** **Grep every selector for an
   existing declaration before appending (R8), and remove a floor entirely before measuring the box it
   floors (F7).** Re-run until **0 moved** on all three arms, and name every `line-height-normal`
   residual with its measured `enH == zhH`. **Read F8 before treating a `wrap-count` delta as a
   defect**, and on O-simpleSubtractive remember the baseline is **1**, not 0 (K9).
10. **Commit at `'mt'`, blind reverse read, promote and ship.** Path-scoped
    `git commit -- plugins/<Name>` — the reverse pass runs against a committed tree. Then
    `--emit <Plugin> --plugin <Plugin>` (BOTH — R5/F16) to a fresh `--out` with
    `--forward-provenance "<string>"` (a VALUE — F9), dispatch to a blind reader
    (`claude -p "<prompt>" --model <model> --allowed-tools ""` from a cwd **outside** the repo, with
    repo access forbidden in the prompt and `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` set — F15),
    `--ingest` with an explicit `--manifest` and `--verbose`, batching tables above ~92 rows into
    chunks **split on the CONCEPT** (F17), not on size. **Check the returned ids are identical AND IN
    ORDER before ingesting** (M12). **Fire BOTH refusal controls with a properly provenanced emit**
    (F10). Read **every** triple (R2). For each drift the discriminator is **collision on the page**,
    not drift distance. Re-author on collision; accept otherwise, and **write the reason down**.
    Then flip to `reviewed: 'bt'` **using a promotion script written to a FILE, never `node -e`**
    (F14.5), bump the version from the CMakeLists value with the two-arm reader (R9), write the
    CHANGELOG entry naming the disclosed quality level and the +30% Han line-box figure, and
    `./scripts/build-and-install.sh <FolderName>` **in the background**. **Do not run auval** — it is
    deferred to Task 7.

**Deferred to the end of the batch:** `auval`. Build and install all six, then sweep once.

**`reviewed: 'native'` stays OPEN on all six and is not a blocker** (D11). This project has no native
Simplified Chinese reader. A **disclosed quality level**, printed by lint rule R1 on every run and
stated in all six CHANGELOGs.

<!-- planner-discipline-allow: and nothing else -->
<!-- planner-discipline-allow: ne contient que -->
<!-- planner-discipline-allow: two languages -->
<!-- planner-discipline-allow: rien d -->
<!-- planner-discipline-allow: English and French are available -->
<!-- planner-discipline-allow: anglais et le fran -->

**Comment-text discipline, stated once and allowlisted above.** Several stale-body literals this plan
quotes in prose are the same literals its `<verify>` blocks negative-grep for. That is deliberate and
it is safe **because every one of those greps runs against a COMMENT-STRIPPED copy of the file**
(`perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g'`), so a C8-worded comment cannot self-invalidate
the gate. **The executor must still not paste a superseded literal into a code comment** (C8) — word
the replacement as prose — but if one slips in, the gate still reads the code and not the comment.

### Commit discipline — restated in every task, and it is not optional

- Re-check `git branch --show-current` and `git status --short` **immediately before every commit**,
  not once at the start. This is a shared checkout and another session's staging can join your commit
  in the gap between the check and the commit.
- **Path-scoped commits only: `git commit -- plugins/<Name>`.** Never `git add -A`, never
  `git commit -a`. Options go BEFORE the `--` (F14.6).
- **THE SUBMODULE GUARD — run it before EVERY commit in EVERY task, not only in Task 6.**
  `plugins/O-Orbit/libs/SAF` is the repo's **only** submodule. **The pathspec `plugins/O-Orbit`
  INCLUDES it** — path-scoping does not imply the guard. Verified at planning time: clean at
  `b6fe1882` (v1.3.4), and `add_subdirectory(libs/SAF/framework saf_build)` at
  `plugins/O-Orbit/CMakeLists.txt:12` writes its binaries to `saf_build` under the out-of-source
  `build/` tree, so **no build step writes into the submodule.**

  ```bash
  SUBMODULE_PATHS="plugins/O-Orbit/libs/SAF"
  for sp in $SUBMODULE_PATHS; do
    if git diff --cached --name-only | grep -q "^$sp"; then
      echo "ABORT: a path under the submodule $sp is staged"; exit 1
    fi
  done
  git submodule status   # must still read " b6fe1882…  plugins/O-Orbit/libs/SAF (v1.3.4)"
  ```

  **No path under it is ever staged, in any task, for any reason.** If the guard trips, unstage and
  investigate — never force it through.
- **Never stage the strangers' paths.** Two on the tree today (K2):
  `plugins/O-Strata/.planning/mockups/v1-ui.html` and
  `plugins/O-Strata/.planning/parameter-spec.md` (untracked, not git-ignored), and
  `.claude/agent-memory/research-planning-agent.md` (modified, another session's file).
- **Never create a git tag.** Tags belong to `/publish` and nothing here authorises one. Push is not
  required.
- **Do NOT commit `PLAN.md`, any `SUMMARY.md`, `deferred-items.md` or `STATE.md`** — the orchestrator
  commits those. Do not touch `ROADMAP.md`.
- Every commit message ends with:
  `Claude-Session: https://claude.ai/code/session_01Ek97dmx4fvCBV9Wa9pB9go`

---

## Source coverage audit

### The thirteen "Explicitly for wave 4g" instructions from wave 4f, by number

| # | instruction | where it is honoured |
|---|---|---|
| 1 | None of the six carries a gate file; verify with `find`; take no invented gate; the four repo instruments are the whole instrument; the N12 census has no subject | **K7**, ship-bar step 8, C5/C6 restated as no-subject, every task's precondition (a) |
| 2 | `plugins/O-Orbit/libs/SAF` is the only submodule; run the guard before every commit; never stage a path under it | **Commit discipline** (guard block, in every task), **Task 6**, must_haves truth 10 |
| 3 | None of the six is a `scala-tuning-engine` consumer; the 37-row module column is DONE and 4g inherits none of it | **Live table** (six NOs), **R1's exception voided**, ship-bar steps 3/8/9, **D4 explicitly NOT inherited** below |
| 4 | Re-derive the installed-family table anyway (M5); a zero is a claim about the query, not the machine (F5) | **K11**, ship-bar step 6, **Task 7** item 3 |
| 5 | Use the two-arm CMake reader unconditionally (R9) | **K5** — and this wave is where a one-arm reader would have FAILED, on O-simpleGrain |
| 6 | Measure the existing pin surface (R8); remove a floor before measuring the box it floors (F7) | **R8** rule text with all six surfaces measured, ship-bar step 9, every task's verify |
| 7 | Fire every precondition you assert (M3) | ship-bar step 1, every task's `<precondition>` block |
| 8 | ids identical AND IN ORDER before ingest (M12); BOTH refusal controls with a properly provenanced emit (F10); `--forward-provenance "<string>"` takes a VALUE (F9) | **R6** rewritten with F9/F10 folded in and re-verified live at `:228` / `:288-364`; ship-bar step 10 |
| 9 | Chunk on the concept split (F17); `--emit X --plugin X` together (F16) | **R5** re-verified live at `:94` / `:161` / `:68`; **F17** in the rules; ship-bar step 10 |
| 10 | Report every FALSE prediction in the plan; the count is the finding | **Task 7** item 6, and the ledger is pre-seeded with the six already false at planning time (**K4**) |
| 11 | Never cite a raw line number as a worklist; verify by pattern; record grep-able tokens | **Tooling traps**, last bullet; every task action names a token beside every line number |
| 12 | `auval -v` takes three unquoted words; read every triple off `auval -a`; budget ~85 s | **Tooling traps**, **Execution shape**, **Task 7** item 1 |
| 13 | Re-inherit D1-D12 | below |

### 4f's D1-D12, re-inherited by number, with this wave's disposition

| item | disposition in wave 4g |
|---|---|
| **D1** — the `'dist lpf'` glossary root is wrong (`失真低通` where the only corpus site means *distance*) | **Corpus-level, still not this wave's.** `grep -rn "Dist LPF"` hits O-GrainScatter and nothing else; none of the six carries the term. **Its general lesson IS carried** into N4: check the corpus site count before writing a termNote against a root. Now the oldest open item in the ledger. |
| **D2** — O-Reed's 20 missing / 7 dead native registrations | Another plugin's tree. Not touched. |
| **D3** — two stale `d848337a` floors on O-Reed | Another plugin's tree; no `d848337a` block exists on any of the six (no consumers). Not touched. |
| **D4** — the 17-of-37 shared-module coverage hole | **NOT INHERITED BY WAVE 4G, and this is stated rather than assumed.** All four consumers named in D4 (O-Bassoon, O-Bowed, O-Contrabass, O-Reed, plus O-Wind) are outside this set, and **none of the six is a consumer** — 0 CMake references and 0 private copies on all six, verified. The 37-key column, the ten never-visible module options and the seven never-in-DOM keys have no subject here. **O-Orbit's 19 never-visible are the SAME CLASS but a different, per-plugin instance (K14)** and are recorded as their own finding. |
| **D5** — `plugins/O-Strata/` makes both repo-wide lints exit 2 | **Re-confirmed live and CORRECTED in its `git status` line (K2):** the directory is now tracked and two planning files inside it are untracked, so the grep reads **2**, not 1. The mechanism is unchanged. Still not a localization wave's item. |
| **D6** — the glossary divergence report, SIX entries open | **This wave must report whether it adds a seventh.** R3 fired at planning time on all six: **no collisions**. Task 7 states the count and names any addition. |
| **D7** — the Z6 budget backfill, 549 unbudgeted | **Re-confirmed live: `3 of 552 glossary terms carry a measured budget; 549 are UNBUDGETED`.** This wave must report whether it adds any. **Predicted none**, for the same reason as 4d/4e/4f: every geometry finding is expected to be a SHRINK, and a shrink wants a floor, not a budget. Task 7 states the outcome either way. |
| **D8** — `modules/registry.yaml` `used_by` is stale | **Drive nothing from it.** This wave reads the CMake grep. Confirmed unchanged. |
| **D9** — the two O-Contrabass orphan bundles | Another plugin. **No orphan on disk for any of the six** — all `-dev` only, verified. |
| **D10** — two pre-existing auval warnings (O-Bowed `Bow Position`, O-Wind channel layout) | Other plugins. **Task 7 must not attribute any new warning on this wave's six to those.** `Bad Max Frames - Render should fail` is the NAME OF A TEST THAT EXPECTS A FAILURE, not a finding. |
| **D11** — `reviewed: 'native'` open on all 4621 corpus rows | **Open on all 820 this wave ships, disclosed in all six CHANGELOGs. A blocker for nothing.** After this wave it is open on all 5441. |
| **D12** — re-inherited unchanged | **4e D2** (O-Bells' 2 late tip bindings) is used as this wave's boot census control: **2 late before, 2 late after**, which is what makes the 0 DEAD beside it evidence. **4e D7** (the `div.routing-label` French wrap) is **CORRECTED IN SCOPE by K9** — it is on O-simpleSubtractive too, which is in this wave. **4c D4**, **s71 D4**, **s71 D6** — other plugins, untouched. |

### Inherited items from wave 4e and s71, answered by number

- **4e D4 / 4f D6 — the glossary divergence report, six entries open** (`WebGL 不可用` vs `不支持
  WebGL`; Scatter and Diffusion; `通过长度` for *pass length*; `分割` for *Divisions*; `键位` for
  *keyswitch*; `调制` for *MOD*). **This wave must report whether it adds a seventh.** Every one is a
  corpus-wide decision, not a per-plugin one.
- **4e D5 / 4f D7 — the Z6 budget backfill.** 549 unbudgeted, re-fired live. Report whether this wave
  adds any.
- **s71 D2 — four shared module JS files `check-i18n` does not scan.** **This wave measures a LIVE,
  LARGER instance of the same class on its own plugins (K8): four unscanned files across four of the
  six, two of which emit user-facing English.** Reported, scoped out with a written reason, carried
  forward. **Not fixed here**, for s71's own reason.
- **s71 D3 / 4f D8 — `modules/registry.yaml` `used_by` is stale. Drive nothing from it.** This wave
  read the CMake grep instead, on all six, and got six NOs for `scala-tuning-engine`.

### Sources

- Task description and its ten hard constraints — every one mapped above and into the tasks below.
- `.planning/quick/260907-ja8-…/260907-ja8-PLAN.md` — the template. Its execution shape, executor
  rules, tooling traps, ship bar and commit discipline are carried forward verbatim in substance,
  with F9/F11/F13/F14/F15/F18 folded into the rule text.
- `.planning/quick/260907-ja8-…/deferred-items.md` — F1-F18, D1-D12, the thirteen wave-4g
  instructions.
- `.planning/quick/260907-ja8-…/260907-ja8-SUMMARY.md` — §9 false-prediction ledger, §12 corpus
  recount.
- `.planning/quick/260906-h8y-…/deferred-items.md` — N1-N13, M-rules, D4, D5, D7.
- `.planning/quick/260906-s71-…/260906-s71-deferred-items.md` — D2, D3, D5, D9, D11.
- `.planning/quick/260904-g5l-…/260904-g5l-SUMMARY.md` — the zh rules of the rollout.
- `CLAUDE.md` — build/install cache-clearing, dual-variant sweep, path-scoped commits, PLUGINS.md
  union-merge duplicate check, never tag, phase handoff protocol.
- `scripts/measure-ui-README.md`, `scripts/serve-ui.js`, `scripts/i18n-zh-glossary.js`,
  `scripts/i18n-zh-lint.js`, `scripts/i18n-zh-backtranslate.js` — read at the flag-parsing level, not
  only the README; F9, F14.2, F14.3, F14.4 and F16 re-verified against the source this pass.

---

<tasks>

<task type="tracer">
  <name>Task 1: O-simpleAdditive end-to-end — the wave's smallest table, thinnest walk, only exact-coverage-with-zero-never-visible page, and the plainest of the three CMake shapes (ZH4G-01, ZH4G-05, ZH4G-07, ZH4G-09)</name>
  <files>plugins/O-simpleAdditive/Source/ui/public/js/i18n.js, plugins/O-simpleAdditive/Source/ui/public/index.html, plugins/O-simpleAdditive/Source/ui/public/css/styles.css, plugins/O-simpleAdditive/Source/PluginProcessor.h, plugins/O-simpleAdditive/CMakeLists.txt, plugins/O-simpleAdditive/CHANGELOG.md</files>

  <required_reading>
- This plan's **Live observations**, **K1-K16**, **Execution shape**, **Stage-4 executor rules**, **THE SHIP BAR** and **Commit discipline** — all above.
- `.planning/quick/260907-ja8-.../deferred-items.md` — **F4** (a residual is not a caption), **F5** (`system_profiler` proves presence never absence), **F7** (remove a floor before measuring), **F8** (`wrap-count` divides by an ASSUMED line box), **F9**, **F10**, **F11**, **F13**, **F14** all six, **F15**, **F17**, **F18**, and the **thirteen wave-4g instructions**.
- `.planning/quick/260906-h8y-.../deferred-items.md` — **N1** (the two `elementFromPoint` failure modes), **N2** (`undeclared-font` is not the census), **N6**, **N9**, **M3**, **M12**.
- `.planning/quick/260904-g5l-.../260904-g5l-SUMMARY.md` — the zh rules of the rollout, and its "do not spell the literal in the comment" rule (C8).
- `scripts/measure-ui-README.md` — the parsed frame, the cumulative walk, OR-ed visibility, DOM-path identity, and the `--report` screen list.
- `CLAUDE.md` — build/install/cache-clearing, path-scoped commits, never `git add -A`, never tag.
  </required_reading>

  <precondition>Fire ALL of these on the tree as found, in the BACKGROUND, before any edit — and do not proceed if any disagrees (M3). Each was fired at planning time and its measured result is given; **re-fire rather than inherit**.
(a) `find plugins/O-simpleAdditive/tests -name '*.js'` → **NOTHING**. Use `find`, never a glob (K7). Its `tests/` holds `i18n-states.json` and a `render-harness/` tree only, and the harness is C++ Stage-2 and out of scope.
(b) `node scripts/check-i18n.js --plugin O-simpleAdditive` → **exit 0**, `[1] LANGUAGES … got ["en","fr"]`, `[1] the table carries copy — 43 I18N + 45 LABELS`, `[2] 43 tip(s) bound`, `[12] 1 module(s): js/app.js`.
(c) `node scripts/i18n-zh-lint.js --plugin O-simpleAdditive` → **exit 0**. `node scripts/i18n-zh-lint.js --self-test` → **10/10**.
(d) `node scripts/check-ui-labels.js --plugin O-simpleAdditive` → **exit 0**, `== ALL CHECKS PASSED ==`, coverage **40 of 40**, **0 never became visible** — the wave's only page with an exact fraction AND zero never-visible.
(e) `node scripts/measure-ui.js --plugin O-simpleAdditive --mode box --report all` → identity **595 nodes / 298 DOM keys / 159 display ids**; `undeclared-font 0`, `line-height-normal 0`, `wrap-count 0`, `svg-font-attr 0 carriers` — **all four are a VACUUM (K12), not a clean bill.** `102 text-bearing leaf node(s) compared; 84 of them ESTIMATED`.
(f) The two-arm CMake reader → **1.2.1** (quoted literal at L17). The one-arm reader also returns 1.2.1 here — **run both anyway** (R9, K5).
(g) `git branch --show-current` → `main`; `git worktree list` → one line; `git tag --points-at HEAD` → 0; `git status --short | grep -c "O-Strata"` → **2** (K2, a correction to 4f's precondition).
(h) `find plugins/O-simpleAdditive/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' {} +` → **nothing**. Zero Han in C++ today.
(i) `find plugins/O-simpleAdditive/Source/ui/public -name '*.js' -not -path '*/juce/*'` → exactly **two** files, `js/i18n.js` and `js/app.js`. **This is the wave's only plugin with NO unscanned module JS file** (K8) — it is the controlled converse to four plugins that have one.</precondition>

  <reversibility rating="reversible">Two to three path-scoped commits on one plugin's tree. Revertible with `git restore --source=&lt;sha&gt; -- plugins/O-simpleAdditive`.</reversibility>

  <action>
**Why O-simpleAdditive is the tracer, and it is not only because it is smallest.** A tracer's job is
to be where the first mistakes are cheap and to prove, on the executor's best early-context tokens,
every structural answer the rest of the wave inherits. At 131 rows it is the smallest table here, and
on top of that:

- **It is the wave's thinnest walk — 2 states**, against 3 or 4 everywhere else. Both are `click`
  states (K16), so it exercises the state-EFFECT assertion at its smallest.
- **It is one of two pages with an EXACT coverage fraction (40 of 40) and it has 0 never-visible.**
  Three of the six report N > M (F18) and O-Orbit reports 19 never-visible; this one has neither
  complication, so a coverage anomaly seen here is a defect rather than a known artefact.
- **It carries the plainest of the three CMake shapes** — a quoted literal — so the two-arm reader is
  proven where a one-arm reader also works, **before** Task 2, where the one-arm reader returns EMPTY.
- **Its `juce_add_plugin` target equals its folder name**, so it proves the build-script argument
  convention on the simple case before Tasks 5 and 6, where the targets are `OPolystutter` and
  `OuariconOrbit`.
- **It is the wave's only plugin with no unscanned module JS file** (K8) — the controlled converse to
  the four that have one, and the reason a "zero unscanned files" reading here is a measurement rather
  than a query artefact.
- **Its house stack is Latin-safe** (`'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New
  Roman', serif` — TNR installed, 4th member), so it proves the **tail-only** convention cleanly
  before O-Orbit, whose whole page needs face-naming.
- **It has the wave's smallest pin surface** — and it is the **only** one of the six with **zero**
  `white-space` declarations, which is a measured absence worth recording rather than a gap.

**131 rows, 2 states, `Source/ui/public/`, an EXTERNAL `css/styles.css` (730 L) beside a 281-line
`index.html`, 860 x 930 parsed frame, `PLUGIN_CODE OSiA`, `IS_SYNTH TRUE`, 43 I18N + 45 LABELS keys,
43 tips bound, `I18N_EXEMPT` mentioned 4 times.**

**THE VERSION IS 1.2.1, NOT 1.2.0 — and this is a correction to the task description.** `CMakeLists.txt`
L17 reads a **quoted** `VERSION "1.2.1"`. `PLUGINS.md` says **1.2.0**, one patch stale, because the
plugin shipped 1.2.1 on **2026-09-03** (the suite-wide French hover-help rename, task `260903-ukp`)
without the registry row following. **All six rows in this wave are stale in exactly this way (K4).**
The description's "1.2.0 → 1.3.0" is a faithful read of a stale registry row. **The MINOR bump from
the CMakeLists value is 1.2.1 → 1.3.0**, the shipping version is unchanged, and Task 7 corrects the
registry row.

**Six things this task proves for the other five. Each must be RECORDED FOR REUSE in the SUMMARY**,
because Tasks 2-6 inherit the method rather than re-deriving it:

1. the two-arm CMake reader run against a shape a one-arm reader also handles;
2. the comment-stripped C++ codec grep discipline, with the positive control on the same run;
3. the blind-dispatch shape end to end, including **both** refusal controls and the concept split;
4. **where the font work goes when the CSS is EXTERNAL** — five of six have a `css/styles.css` and
   wave 4f's five had none, so the tail lands in a different file from every prior wave's;
5. the CJK-tail convention on a Latin-safe stack, verified by re-running the screen rather than by
   reading the CSS;
6. the F13-corrected duplicate-key scan and the F14-corrected helper forms.

**THE FONT WORK — the tail half only, and the CSS is in a different file than every prior wave.**
The census returns one house stack, declared 6 times with 4 of those being `font-family: inherit`:

```
font-family: 'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', serif
font-family: inherit
font-family: var(--symbol-font)
```

Garamond, EB Garamond and Adobe Garamond Pro are **all absent** on this machine (K11) — but **Times
New Roman is installed**, so the stack is **Latin-safe** and takes the **tail only**: append
`'PingFang SC', 'Microsoft YaHei'` **before** the trailing `serif` (W1). **Do not remove the absent
Garamond members** — they are the Windows/print intent and removing them is a rendering change on a
machine this repo does not build on.

`--symbol-font` resolves to `'Segoe UI Symbol', 'Apple Symbols', 'Noto Sans Symbols2', 'Arial Unicode
MS', serif`; Apple Symbols and Arial Unicode MS are installed so the token is not naked. **N3 still
applies:** check whether any node using it is a TIP ANCHOR (`measure-ui`'s `han` counts `data-tip`,
`data-tip-title` and `aria-label`) and give it the tail if so. **Record the verdict either way.**

**Search BOTH files.** Concatenate `index.html` and `css/styles.css`, comment-strip, and census the
pair — a font declaration in this plugin can live in either, and every prior wave in this rollout had
inline CSS only.

**THE PIN SURFACE — R8 is live here (K10/R8).** Measured, comment-stripped, across `index.html` +
`css/styles.css`: **6 `min-width`, 1 `min-height`, 3 `line-height`, 18 `letter-spacing`, 0
`white-space`.** **Grep every selector for an existing declaration before appending.** And **remove a
floor entirely before measuring the box it floors** (F7) — `min-width: 0` is not "no floor": the
flex-item default is `auto`, so `0` explicitly permits shrinking BELOW content and reads a false
natural box. The **zero `white-space` declarations** is the wave's only such zero and it means a Han
run whose min-content width is one character has nothing holding it — expect to need `nowrap` **plus**
a min-width if a caption collapses in an auto-sized container.

**THE STALE BODY — ONE class, and the other two have no subject (K13, C7).** The comment-stripped
probe, run with `perl -CSD`, reports **exactly 2 hits**, and both are the same clause in the two
languages of the language entry:

> `English and French are available; value readouts and the two drop-down menus stay in English.`
> `… l’anglais et le français sont disponibles ; les valeurs affichées et les deux menus déroulants restent en anglais.`

**Delete the pair clause only.** The exception list stays — and **re-verify its numeric claim first**:
markup carries **3** `<select>` elements, and subtracting the language selector gives **2**, so "the
two drop-down menus" is TRUE today. Say so, citing the count. **Correct the comment above the entry
too if it repeats the pair**, and word the replacement as prose without spelling the superseded
literal (C8), or this task's own probe keeps reporting a file that is fixed.

**Record the two DEAD classes as checked non-defects, citing the grep, not as omissions:**
`tip.settings` **does not exist** on this plugin (grep count 0) and there is no `#tips-toggle` in its
markup, so the "and nothing else" class has no subject; and it is **not** a `scala-tuning-engine`
consumer, so the "Tuning tab stays English" class has none either.

**THE C++ CODEC, HAND-EDITED AND GREP-PROVED.** `Source/PluginProcessor.h` **L181-182** (raw), with
the standard doc comment above at L178-180 — locate them by the tokens `languageCode` and
`languageIndex`, not by the line numbers, which drift (instruction 11). Widen the two-way ternary pair
to three-way, pure ASCII. **Edit in place; do not sweep** — a sweep's exit code is not evidence of an
edit. Prove it with a **comment-stripped** grep asserting the Chinese code on both the encode and the
decode line, and a second probe that matches each function **by name** and reports whether it is
three-way. Language persistence is a ValueTree `getProperty("uiLanguage")` with a string-literal key
and needs no change. There is no language `AudioParameterChoice` — re-confirm by scanning each
declaration's own text.

**THE BLIND-DISPATCH SHAPE.** 131 rows is above the ~92-row chunking threshold, so this is also where
the two-chunk shape is proven. **Chunk on the CONCEPT SPLIT, not on size (F17)** — send each control's
caption and its own tooltip title to *different* chunks, so a divergence between the two is visible to
a reader rather than to nobody. `--emit O-simpleAdditive --plugin O-simpleAdditive` (**BOTH** — R5), a
fresh 16-hex salt per chunk, `--forward-provenance "<a real string>"` at emit (**a VALUE** — F9), an
explicit `--manifest` at ingest, `--verbose`, `claude -p … --allowed-tools ""` from a cwd **outside**
the repo with repo access forbidden in the prompt and `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` set
(F15 — and read stderr and record the model that actually ran). **Fire BOTH refusal controls with a
properly provenanced emit** (F10): omit `--provenance` on one run and confirm it refuses for *that*
reason, then pass a wrong-side `--manifest` on a properly provenanced run and confirm it prints
`WRONG MANIFEST FOR THIS BATCH: all N returned ids are unjoinable, not one.` **Record both messages
verbatim.** **Check the returned ids are identical AND IN ORDER** before ingesting (M12).

**TWO STATES IS THE WAVE'S THINNEST WALK, AND BOTH ARE CLICKS.** Before trusting any screen's zero,
run `measure-ui --verbose` and confirm **both** states resolved, then fire the state-EFFECT assertion
on **each**: `#gear-btn` (settings popover) and `#help-toggle` (hover-help off). `elementFromPoint` at
each target's centre must return the button or a descendant, and the state must measurably take
effect. A `null` there is a scrolled target, not a coverage hole (N1); discriminate on the rect.

Then the rest of the loop in order: screen `TERMS` (R3 — fired at planning time, **NONE**, 22
glossary-matched captions; re-fire); make the body correction and **re-read the file** (C7); author at
`'mt'` (R1) holding the SPACED Latin/Han form (M11); endonym copied byte-for-byte from
`plugins/O-Detune/Source/ui/public/index.html:1039` with an `I18N_EXEMPT` entry; widen the ternary;
font tail; geometry pass; commit at `'mt'`; blind reverse read; promote in a second commit **using a
promotion script written to a FILE, never `node -e`** (F14.5), with the version bumped to **1.3.0**
and the CHANGELOG entry; build and install with `./scripts/build-and-install.sh O-simpleAdditive`
**in the background**. **Do not run auval** — it is deferred to Task 7.

**Commit discipline.** Re-check `git branch --show-current` and `git status --short` immediately
before every commit. `git commit -- plugins/O-simpleAdditive` only; never `git add -A`, never
`git commit -a`; options before the `--` (F14.6). **Run the submodule guard before every commit** even
though no path here is inside it — the guard is unconditional, so that Task 6's is not the first time
it runs. **Never stage `plugins/O-Strata/.planning/*` or `.claude/agent-memory/*`.** **Never create a
tag.** Every message ends with
`Claude-Session: https://claude.ai/code/session_01Ek97dmx4fvCBV9Wa9pB9go`.
  </action>

  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10 before any zero is trusted</automated>
    <automated>node scripts/check-i18n.js --plugin O-simpleAdditive >/tmp/qda-t1-ci.txt 2>&1; echo "check-i18n exit=$?"; grep -E "\[1\] LANGUAGES|\[1\] the table carries copy|\[2\] .* tip" /tmp/qda-t1-ci.txt   # exit 0; LANGUAGES lists THREE; key counts UNCHANGED at 43 I18N + 45 LABELS; 43 tips bound</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-simpleAdditive >/tmp/qda-t1-cul.txt 2>&1; echo "check-ui-labels exit=$?"; grep -cE "GEOMETRY DIFF\]\[(fr|zh-Hans)\] no non-label element moved" /tmp/qda-t1-cul.txt; grep -E "elements were VISIBLE|never became visible" /tmp/qda-t1-cul.txt   # exit 0; the moved-count assertion GREEN on BOTH non-English arms across BOTH states; coverage still 40 of 40 with 0 never-visible</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-simpleAdditive 2>&1 | tail -12   # 0 findings, 131 rows, BELOW SHIP BAR 0</automated>
    <automated>node scripts/i18n-zh-lint.js 2>&1 | tail -3   # repo-wide: exit 2 is the BASELINE (K2). The load-bearing line is "0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read"</automated>
    <automated>node scripts/i18n-fr-lint.js 2>&1 | grep -E "plugins with findings"   # 0 / 44, 1 could not be read — French untouched. The exit code is 2 for the K2 reason and is NOT the criterion</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleAdditive/Source/PluginProcessor.h > /tmp/qda-t1-pp.h; grep -c 'zh-Hans' /tmp/qda-t1-pp.h   # >= 2 — comments stripped, so the count is the CODE and not the doc line above it</automated>
    <automated>node -e "const s=require('fs').readFileSync('/tmp/qda-t1-pp.h','utf8'); for (const fn of ['languageCode','languageIndex']) { const m = s.match(new RegExp(fn + '[\\\\s\\\\S]{0,240}?;')); console.log(fn + ': ' + (m && /zh/.test(m[0]) ? 'THREE-WAY' : 'STILL TWO-WAY — the edit did not land on this line')); }"   # THREE-WAY on both — encode AND decode, proved by NAME and not by a sweep's exit code</automated>
    <automated>find plugins/O-simpleAdditive/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +   # MUST print nothing</automated>
    <automated>perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-simpleAdditive/Source/ui/public/js/i18n.js   # positive control for the line above — MUST print the file, or the negative gate proved nothing</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleAdditive/Source/ui/public/js/i18n.js > /tmp/qda-t1-i18n.js; perl -CSD -0777 -ne '$n++ while /(English (and|or) (French|Fran)|anglais et le fran|and nothing else|ne contient que|two languages|deux langues|rien d.autre|rien d\x{2019}autre)/gi; END{print "stale-body hits in CODE: ",$n+0,"\n"}' /tmp/qda-t1-i18n.js   # 0 — comments stripped so a C8-worded comment cannot self-invalidate this gate, and run with -CSD so the French U+2019 branch is not byte-blind (F11). Fired at planning time: 2</automated>
    <automated>grep -c "'tip\.settings'" /tmp/qda-t1-i18n.js   # 0 — the "and nothing else" class HAS NO SUBJECT on this plugin, recorded as a checked non-defect rather than an omission (K13)</automated>
    <automated>node -e "const langs=new Set(['en','fr','zh-Hans']); const s=require('fs').readFileSync('/tmp/qda-t1-i18n.js','utf8'); const seen=new Set(), dup=[]; for(const m of s.matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)){const k=m[1]; if(langs.has(k))continue; if(seen.has(k))dup.push(k); else seen.add(k);} console.log('keys='+seen.size+' duplicates '+(dup.length?dup.join(','):'NONE'))"   # duplicates NONE — the F13-CORRECTED form, which excludes the declared LANGUAGES members. The uncorrected form returns 'zh-Hans' on every three-language table and can never read NONE</automated>
    <automated>node -e "const m=require('fs').readFileSync('plugins/O-simpleAdditive/Source/ui/public/js/i18n.js','utf8'); const n=(m.match(/reviewed:\s*'bt'/g)||[]).length, mt=(m.match(/reviewed:\s*'mt'/g)||[]).length; console.log('bt='+n+' mt='+mt)"   # after promotion: mt=0. A bare `bt` identifier here means a node -e promotion script lost its quotes (F14.5)</automated>
    <automated>cat plugins/O-simpleAdditive/Source/ui/public/index.html plugins/O-simpleAdditive/Source/ui/public/css/styles.css > /tmp/qda-t1-all.txt; grep -c "PingFang SC" /tmp/qda-t1-all.txt   # >= 1 — the CJK tail landed. BOTH files are searched because this plugin's CSS is EXTERNAL, unlike every plugin in wave 4f</automated>
    <automated>grep -oE "font-family:[^;}]*" /tmp/qda-t1-all.txt | sort -u   # every non-inherit stack ends '…, PingFang SC, Microsoft YaHei, <generic>' with the tail BEFORE the generic (W1), and the absent Garamond members are still present as the Windows/print intent</automated>
    <automated>grep -c "&#31616;&#20307;&#20013;&#25991;" plugins/O-simpleAdditive/Source/ui/public/index.html   # 1 — the endonym, copied byte-for-byte from O-Detune index.html:1039 and never retyped</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleAdditive --mode box --report all >/tmp/qda-t1-mu.json 2>/tmp/qda-t1-mu.err; grep -E "undeclared-font|line-height-normal|wrap-count|svg-font-attr|identity|ESTIMATED" /tmp/qda-t1-mu.err   # undeclared-font 0 against a NON-EMPTY input this time, wrap-count 0 (baseline 0), svg-font-attr 0 carriers, identity ~595 nodes. Read F8 before treating any wrap-count delta as a defect — a ratio pin changes the divisor with no pixel moving</automated>
    <automated>node -e "const r=require('/tmp/qda-t1-mu.json'); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); const noface=z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff||'')); const lh=z.filter(x=>x.lh==='normal'&&x.kids===0&&x.own&&x.own.trim()); const mv=lh.filter(x=>{const e=r.find(y=>y.key===x.key&&y.lang==='en'); return e&&Math.abs(e.h-x.h)>0.5}); console.log('zhHan='+z.length,'noCJKface='+noface.length,'lhResiduals='+lh.length,'ofWhichMOVED='+mv.length)"   # zhHan > 0 (the input is non-empty, so the zeros above are measurements and not the K12 vacuum), noCJKface 0, ofWhichMOVED 0. Residuals are permitted and NAMED with their measured enH == zhH; movers are not (N1)</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleAdditive --mode box --verbose >/dev/null 2>/tmp/qda-t1-muv.err; grep -icE "did not resolve|unresolved|skipped state" /tmp/qda-t1-muv.err   # 0 — BOTH states applied, so every state-gated zero is measured. This is the wave's thinnest walk and both states are CLICKS</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-simpleAdditive --strict-tips 2>&1 | grep -iE "late|dead|clean"   # 0 late, 0 dead</automated>
    <automated>V=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' plugins/O-simpleAdditive/CMakeLists.txt); echo "resolved version: ${V:-EMPTY-READER-IS-THE-DEFECT}"   # 1.3.0 — the TWO-ARM reader, run unconditionally even though this one is a quoted literal a one-arm reader also handles (R9)</automated>
    <automated>for p in O-simpleGrain O-simpleSampler O-simpleSubtractive O-Polystutter O-Orbit; do a=$(perl -ne 'print "$1\n" if /^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ && !$d++' plugins/$p/CMakeLists.txt); b=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' plugins/$p/CMakeLists.txt); echo "  $p one-arm='${a:-EMPTY}' two-arm='${b:-EMPTY}'"; done   # O-simpleGrain reads one-arm='EMPTY' two-arm='1.4.3' (or 1.5.0 after Task 2). ALL THREE CMake shapes are in this wave and the one-arm reader FAILS on one of them (K5) — record this cross-check for Tasks 2-6</automated>
    <automated>find plugins/O-simpleAdditive/Source/ui/public -name '*.js' -not -path '*/juce/*' | wc -l | tr -d ' '   # 2 — js/i18n.js and js/app.js only. NO unscanned module JS file: the controlled converse to the four plugins that have one (K8)</automated>
    <automated>find plugins/O-simpleAdditive/tests -name '*.js' | wc -l | tr -d ' '   # 0 — no gate file, verified with find and not a glob (K7). No gate was invented</automated>
    <automated>git status --short -- plugins/O-Orbit/libs/SAF | wc -l | tr -d ' '   # 0 — the submodule guard ran and the submodule is untouched</automated>
    <automated>git status --short | grep -c "O-Strata"   # 2, and both must be UNTRACKED and UNSTAGED (K2)</automated>
    <automated>git tag --points-at HEAD | wc -l | tr -d ' '   # 0 — no tag was created</automated>
    <human-check>Every one of the ~131 back-translation triples was read with --verbose, in two chunks split on the CONCEPT (caption and its own tooltip title to different readers, F17) with a fresh salt per chunk. Each accepted drift carries a written collision-on-the-page reason. BOTH refusal controls FIRED against a properly provenanced emit and each printed its own message verbatim (F10); `--forward-provenance` was passed a real STRING, not left bare (F9); the ids were confirmed identical AND IN ORDER before ingest (M12); and the model that ACTUALLY ran is recorded from stderr, not the one requested (F15).</human-check>
    <human-check>The stale body was read in full and its verdict recorded citing the entry: the endonym-pair clause was deleted in en and fr, its exception list was KEPT after the numeric claim was re-verified against 3 &lt;select&gt; elements minus the language selector = 2, and the comment above the entry was corrected without spelling the superseded literal (C8).</human-check>
    <human-check>The two DEAD stale-body classes were recorded as checked non-defects with the grep cited, not omitted: `tip.settings` does not exist on this plugin (count 0) and there is no #tips-toggle in its markup; and the plugin is not a scala-tuning-engine consumer, so the "Tuning tab stays English" class has no subject either.</human-check>
    <human-check>The state-EFFECT assertion fired on BOTH click states: elementFromPoint at #gear-btn's and #help-toggle's centres returns the button or a descendant (not a different element, and not null), and each state measurably took effect. Where a rect was null it was discriminated as a scrolled target rather than a coverage hole (N1).</human-check>
    <human-check>The `--symbol-font` verdict is recorded: whether any node using the token is a TIP ANCHOR (measure-ui's `han` counts data-tip, data-tip-title and aria-label) and therefore needs the CJK tail regardless of the glyphs it renders (N3) — stated as measured, either way.</human-check>
    <human-check>The reusable findings are written down for Tasks 2-6 to inherit rather than re-derive: the two-arm reader cross-check and its EMPTY result on O-simpleGrain, the comment-stripped codec grep discipline with its positive control, the blind-dispatch shape end to end, WHERE THE FONT WORK GOES WHEN THE CSS IS EXTERNAL (a first for this rollout), the CJK-tail convention on a Latin-safe stack verified by re-running the screen rather than reading the CSS, and the F13-corrected duplicate-key scan.</human-check>
  </verify>

  <done>
O-simpleAdditive ships three languages at `reviewed: 'bt'` on all 131 rows, version **1.3.0** read back
through the two-arm reader from a quoted CMakeLists literal that was **1.2.1, not the 1.2.0 the
description claimed**, CHANGELOG written, built and installed, geometry equal on all three arms across
both states with both states confirmed to have applied. Its Latin-safe house stack carries the CJK
tail before the generic in whichever of `index.html` / `css/styles.css` declares it, and
`undeclared-font` reads 0 against a **non-empty** input. Its C++ codec is three-way and grep-proved on
both lines with comments stripped; zero Han exists under `Source/`, with the positive control fired on
the same run. Its endonym-pair clause is deleted in en and fr with its re-verified exception list
kept, and both dead stale-body classes are recorded as checked non-defects citing the grep. Its zero
gate files and its zero unscanned module JS files are recorded **as measured**, as the controlled
converse to the rest of the wave. Two or more path-scoped commits: the `'mt'` table, then the
promotion. The submodule guard ran before every one. The reusable findings — the two-arm reader
cross-check, the codec grep discipline, the blind-dispatch shape, the external-CSS font location and
the tail convention — are written down for Tasks 2-6.
  </done>
</task>

<task type="auto">
  <name>Task 2: O-simpleGrain — the wave's largest table, the ONLY plugin where the one-arm CMake reader returns EMPTY, the ONLY one with a second version source, and the first of two carrying an unscanned module that emits English (ZH4G-02, ZH4G-05, ZH4G-06, ZH4G-08)</name>
  <files>plugins/O-simpleGrain/Source/ui/public/js/i18n.js, plugins/O-simpleGrain/Source/ui/public/index.html, plugins/O-simpleGrain/Source/ui/public/css/styles.css, plugins/O-simpleGrain/Source/PluginProcessor.h, plugins/O-simpleGrain/CMakeLists.txt, plugins/O-simpleGrain/CHANGELOG.md</files>

  <required_reading>
- This plan's **Live observations**, **K1-K16**, **Execution shape**, **Stage-4 executor rules**, **THE SHIP BAR** and **Commit discipline** — all above. **K5 and K6 are this task's centre of gravity.**
- **Task 1's SUMMARY** — the two-arm reader cross-check, the codec grep discipline, the blind-dispatch shape, the external-CSS font location and the tail convention. **Inherit the method; re-fire the measurements.**
- `.planning/quick/260907-ja8-.../deferred-items.md` — **F7**, **F8**, **F9**, **F10**, **F11**, **F13**, **F14**, **F15**, **F17**, **F18**, and wave-4g instructions **5**, **8**, **9**, **10**, **11**.
- `.planning/quick/260906-s71-.../260906-s71-deferred-items.md` — **D2** in full: the four shared module JS files `check-i18n` does not scan, and the NARROWING arm s71 took and why.
- `plugins/O-simpleGrain/CMakeLists.txt` **L1-30 in full**, and `plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt` **L50-62** — the two files that share the version.
- `plugins/O-simpleGrain/tests/i18n-states.json` — four states, all clicks, and one of them names a French measurement (`997.22 px natural NOWRAP`) that must be RE-TAKEN, not inherited.
- `scripts/measure-ui-README.md`, `CLAUDE.md`.
  </required_reading>

  <precondition>Fire ALL of these on the tree as found, in the BACKGROUND, before any edit (M3). Measured at planning time; **re-fire rather than inherit**.
(a) `find plugins/O-simpleGrain/tests -name '*.js'` → **NOTHING** (K7). `tests/` holds `i18n-states.json` and a C++ `render-harness/` only.
(b) `node scripts/check-i18n.js --plugin O-simpleGrain` → **exit 0**, `got ["en","fr"]`, `38 I18N + 77 LABELS`, `[2] 38 tip(s) bound`, `[12] 1 module(s): js/app.js`.
(c) `node scripts/i18n-zh-lint.js --plugin O-simpleGrain` → **exit 0**; `--self-test` → **10/10**.
(d) `node scripts/check-ui-labels.js --plugin O-simpleGrain` → **exit 0**, coverage **56 of 54** (N &gt; M is NOT a failure — F18), **0 never became visible**.
(e) `measure-ui --mode box --report all` → identity **502 / 251 / 122**; `wrap-count 0`; `90 leaves compared, 70 ESTIMATED`; every Han-gated screen a **VACUUM** (K12).
(f) **The two-arm reader → `1.4.3`. The ONE-arm reader → `EMPTY`.** Fire BOTH and record the pair; this is the wave's proof that R9 is load-bearing (K5).
(g) `grep -rn "OSIMPLEGRAIN_VERSION" plugins/O-simpleGrain/` → **five** hits: `CMakeLists.txt:9` (the string), `:10` (the hex code `0x010403`), `:24` (the consumer), and `tests/render-harness/CMakeLists.txt:57` and `:58`. **Confirm all five before editing either (K6).**
(h) `find plugins/O-simpleGrain/Source/ui/public -name '*.js' -not -path '*/juce/*'` → **three** files, including `modules/webview-drop-streaming.js`, which `check-i18n` does **not** scan (K8).
(i) `git branch --show-current` → `main`; one worktree; `git tag --points-at HEAD` → 0; `git status --short | grep -c "O-Strata"` → 2.
(j) Zero Han in `Source/**/*.{h,cpp}` today, with the positive control on `js/i18n.js` firing.</precondition>

  <reversibility rating="reversible">Two to three path-scoped commits on one plugin's tree. Revertible with `git restore --source=&lt;sha&gt; -- plugins/O-simpleGrain`.</reversibility>

  <action>
**153 rows — the wave's largest table. 4 states, all clicks. `Source/ui/public/`, an external
`css/styles.css` (1129 L — the wave's largest stylesheet) beside a 341-line `index.html`, 900 x 760
parsed frame, `PLUGIN_CODE OsGr`, `IS_SYNTH TRUE`, target name equals folder name, 38 I18N + 77
LABELS, 38 tips bound, `I18N_EXEMPT` mentioned 3 times.** Run **the full ship bar** as Task 1 did.
Three things are specific to this plugin and none of them exists anywhere else in the wave.

**1. THE VERSION IS HELD IN A VARIABLE, AND THE ONE-ARM READER RETURNS NOTHING (K5, R9).**

```cmake
L9   set(OSIMPLEGRAIN_VERSION "1.4.3")
L24      VERSION "${OSIMPLEGRAIN_VERSION}"
```

A reader that matches a literal after `VERSION` matches the `${…}` and extracts **nothing**. Measured
at planning time: `one-arm='EMPTY' two-arm='1.4.3'`. **Wave 4f reported this shape absent and said so
explicitly; wave 4g has it, and this is the plugin.** Use the two-arm reader, fire both arms, and
**record the pair in the SUMMARY** — it is the direct answer to wave 4g instruction 5 and it converts
R9 from a ceremony into a measured requirement.

**The registry says 1.4.2. The CMakeLists says 1.4.3. The MINOR bump is 1.4.3 → 1.5.0** (K4). The
stale patch is the 2026-09-03 suite-wide French hover-help rename.

**2. THE HEX MIRROR — A SECOND VERSION SOURCE THAT THE RENDER-HARNESS COMPILES (K6).**

```cmake
plugins/O-simpleGrain/CMakeLists.txt:10                  set(OSIMPLEGRAIN_VERSION_CODE 0x010403)
plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt:57  JucePlugin_VersionString="${OSIMPLEGRAIN_VERSION}"
plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt:58  JucePlugin_VersionCode=${OSIMPLEGRAIN_VERSION_CODE}
```

The encoding is `0x00MMmmpp` — major, minor, patch, one byte each. **`1.4.3` is `0x010403`;
`1.5.0` is `0x010500`.** The file's own comment at L5-9 states the pair is the single source of truth
and that the hand-rolled `JucePlugin_VersionString/Code` "previously drifted" — the exact failure this
structure prevents. Its CHANGELOG at L35 records the mirror being moved deliberately on the last bump,
so the discipline is established and this task continues it rather than inventing it.

**Move BOTH, in the same edit. No gate in this repo reads the hex**, so a bump that edits only the
string ships a render-harness compiled against a stale code and nothing catches it. **No other plugin
in the six has a second version source.** Locate both by the token `OSIMPLEGRAIN_VERSION`, not by line
number (instruction 11).

**3. AN UNSCANNED MODULE THAT EMITS TWELVE ENGLISH STRINGS TO THE PAGE — REPORT IT, DO NOT FIX IT (K8, s71 D2).**

`Source/ui/public/modules/webview-drop-streaming.js` (504 L) is **not** scanned by `check-i18n`, whose
`[12]` clause reports `1 module(s): js/app.js` and passes. **The discriminator is the DIRECTORY:**
files under `js/` are scanned, files under `modules/` are not. The file is **byte-identical** to
O-simpleSampler's copy (sha `c7fe612e…` — verify it, do not assume it).

It writes nothing to the DOM itself; it hands strings to `opts.showToast(...)`:

```
:195  if (!isAudio) { opts.showToast('Drop a .wav/.aif on a cell'); return; }
:230  opts.showToast('Scanning folder…');
:235  opts.showToast('No audio files in folder');
```

plus `'Drop a folder, not a file'`, `'Drop session start failed'`, `'File load failed at commit step'`,
`'File transfer failed'`, `'Folder load dialog failed — aborted'`, `'Folder load failed at commit
step'`, `'No samples loaded — all files failed'` and two template forms.

**Those toasts render English on the Chinese page — and they render English on the French page
today.** They are not a regression this wave causes and they are not this wave's to fix:

- keying them is a toast-localization surface with **no calibrated gate** anywhere in this repo;
- the file has a **second byte-identical consumer** (Task 3), so a fix is a two-plugin change inside a
  commit whose subject is a caption table;
- s71 took exactly this NARROWING arm on `preset-manager.js` for exactly these reasons, and its
  reasoning is the precedent.

**Measure it, name the strings, state the verdict with the file and the `showToast` call sites cited,
and carry it forward.** Take no invented gate (wave 4c D4). **Do not add a partial keying** — a
half-keyed toast layer is worse than an unkeyed one because it looks done.

**THE FONT WORK — the tail half only.** One house stack, declared 11 times with **8** of those being
`font-family: inherit`, plus `var(--symbol-font)`:

```
font-family: 'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', serif
```

Times New Roman is installed (K11), so the stack is **Latin-safe** and takes the **tail only**:
`'PingFang SC', 'Microsoft YaHei'` before the trailing `serif` (W1). **Keep the absent Garamond
members** — Windows/print intent. **Search `index.html` AND `css/styles.css`** — the CSS is external
here and the 1129-line stylesheet is where most of the 11 sites live. Check the `--symbol-font`
carriers for TIP-ANCHOR status (N3) and record the verdict. Re-run the screen to verify; never read
the CSS to verify.

**THE PIN SURFACE — the wave's LARGEST, and R8 is live (R8, F7).** Measured comment-stripped across
both files: **12 `min-width`, 7 `min-height`, 7 `line-height`, 28 `letter-spacing`, 4 `white-space`** —
the biggest surface in the wave on four of five axes. **Grep every selector before appending**, or a
new pin at higher specificity silently un-pins a French one and moves the **fr** arm. **Remove a floor
entirely before measuring the box it floors** (F7); `min-width: 0` is not "no floor". The 28
`letter-spacing` declarations are the largest Latin-display-convention surface in the wave — trim
under `html[lang="zh-Hans"]` **with the trim value MEASURED**, precedent
`plugins/O-Prism/Source/ui/public/index.html:1298`.

**FOUR CLICK STATES, AND ONE CARRIES A FRENCH MEASUREMENT THAT MUST BE RE-TAKEN (K16, N1).** The
states are `#gear-btn`, `#help-toggle`, a lesson-preset click, and the Pitched Buzz lesson — whose own
state name records *"the widest of the eight captions in French — 997.22 px natural NOWRAP"*. **That
is a French number from an earlier wave written into a fixture. Re-measure it; do not inherit it**
(`pattern_test_fixture_mirrors_drift_silently`). Fire the state-EFFECT assertion on **all four**
clicks, discriminating a `null` rect as a scrolled target rather than a coverage hole.

**THE STALE BODY — the pair clause, en and fr, 2 hits (K13).** The English reads *"English and French
are available; value readouts and the two drop-down menus stay in English."* Markup carries **3**
`<select>`; minus the language selector that is **2**, so the numeric claim is TRUE today —
**re-verify it, then keep the exception list and delete only the pair clause**, in both languages, and
correct the comment above the entry as prose without spelling the superseded literal (C8). Record the
two DEAD classes as checked non-defects: `tip.settings` count **0**, no `#tips-toggle` in markup, not
a `scala-tuning-engine` consumer.

**THE C++ CODEC.** `Source/PluginProcessor.h` **L176-177** raw, doc comment at L173-175, plus
`kUiLanguageProp` at L502 — locate them by the tokens `languageCode`, `languageIndex`,
`kUiLanguageProp`. Widen the ternary pair to three-way, pure ASCII, in place. Persistence is a
ValueTree `getProperty(kUiLanguageProp)` and needs no change. No language `AudioParameterChoice`.

**THE BLIND DISPATCH.** 153 rows is the wave's largest and well above the ~92-row threshold — **two
chunks minimum, split on the CONCEPT** (F17), fresh 16-hex salt per chunk,
`--emit O-simpleGrain --plugin O-simpleGrain` (BOTH — R5), `--forward-provenance "<string>"` (a VALUE
— F9), explicit `--manifest`, `--verbose`, `--allowed-tools ""` from outside the repo,
`CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` and the model read from stderr (F15). **BOTH refusal
controls against a properly provenanced emit** (F10). **ids identical AND IN ORDER** (M12). Read every
triple (R2).

Then: screen `TERMS` (R3 — fired at planning time, **NONE**, 27 matched; re-fire); body correction and
**re-read** (C7); author at `'mt'` (R1) in the SPACED form (M11); endonym from O-Detune L1039 with an
`I18N_EXEMPT` entry; ternary; font tail; geometry; commit at `'mt'`; blind read; promote **via a script
written to a FILE** (F14.5) with the version at **1.5.0** and **the hex at `0x010500`**; CHANGELOG;
`./scripts/build-and-install.sh O-simpleGrain` **in the background**. **No auval** — Task 7.

**Commit discipline.** Re-check branch and status immediately before every commit.
`git commit -- plugins/O-simpleGrain` only; options before the `--` (F14.6). **Run the submodule
guard before every commit.** Never stage `plugins/O-Strata/.planning/*` or `.claude/agent-memory/*`.
Never tag. Session trailer on every message.
  </action>

  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/check-i18n.js --plugin O-simpleGrain >/tmp/qda-t2-ci.txt 2>&1; echo "exit=$?"; grep -E "\[1\] LANGUAGES|carries copy|tip\(s\) bound" /tmp/qda-t2-ci.txt   # exit 0; THREE languages; 38 I18N + 77 LABELS unchanged; 38 tips</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-simpleGrain >/tmp/qda-t2-cul.txt 2>&1; echo "exit=$?"; grep -cE "GEOMETRY DIFF\]\[(fr|zh-Hans)\] no non-label element moved" /tmp/qda-t2-cul.txt; grep -E "elements were VISIBLE|never became visible" /tmp/qda-t2-cul.txt   # exit 0; green on BOTH non-English arms across all FOUR states; coverage still 56 of 54 (N &gt; M is not a failure — F18) with 0 never-visible</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-simpleGrain 2>&1 | tail -12   # 0 findings, 153 rows, BELOW SHIP BAR 0</automated>
    <automated>A=$(perl -ne 'print "$1\n" if /^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ && !$d++' plugins/O-simpleGrain/CMakeLists.txt); B=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' plugins/O-simpleGrain/CMakeLists.txt); echo "one-arm='${A:-EMPTY}' two-arm='${B:-EMPTY}'"   # one-arm='EMPTY' two-arm='1.5.0'. THIS IS THE WAVE'S PROOF THAT R9 IS LOAD-BEARING (K5) — record the pair</automated>
    <automated>grep -n "OSIMPLEGRAIN_VERSION" plugins/O-simpleGrain/CMakeLists.txt plugins/O-simpleGrain/tests/render-harness/CMakeLists.txt   # the string reads 1.5.0 AND the hex reads 0x010500 — both moved in the same edit. A hex still reading 0x010403 ships a render-harness compiled against a stale JucePlugin_VersionCode and no gate in this repo can see it (K6)</automated>
    <automated>node -e "const s=require('fs').readFileSync('plugins/O-simpleGrain/CMakeLists.txt','utf8'); const v=s.match(/set\(OSIMPLEGRAIN_VERSION\s+\"(\d+)\.(\d+)\.(\d+)\"\)/); const c=s.match(/set\(OSIMPLEGRAIN_VERSION_CODE\s+0x([0-9a-fA-F]{6})\)/); if(!v||!c){console.log('PROBE BROKEN — one of the two declarations did not match');process.exit()} const want=[v[1],v[2],v[3]].map(n=>String(Number(n)).padStart(2,'0')).join(''); console.log('version '+v.slice(1).join('.')+' -> expects 0x'+want+', hex reads 0x'+c[1]+' : '+(want===c[1].toLowerCase()?'IN SYNC':'DRIFTED — K6'))"   # IN SYNC. The hex encoding is 0x00MMmmpp and the two declarations are derived from each other, not transcribed</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleGrain/Source/PluginProcessor.h > /tmp/qda-t2-pp.h; grep -c 'zh-Hans' /tmp/qda-t2-pp.h; node -e "const s=require('fs').readFileSync('/tmp/qda-t2-pp.h','utf8'); for (const fn of ['languageCode','languageIndex']) { const m = s.match(new RegExp(fn + '[\\\\s\\\\S]{0,240}?;')); console.log(fn + ': ' + (m && /zh/.test(m[0]) ? 'THREE-WAY' : 'STILL TWO-WAY')); }"   # >= 2, and THREE-WAY on both — proved by NAME, comments stripped</automated>
    <automated>find plugins/O-simpleGrain/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +; perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-simpleGrain/Source/ui/public/js/i18n.js   # nothing, then the control MUST fire — a negative gate with no positive control proves nothing</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleGrain/Source/ui/public/js/i18n.js > /tmp/qda-t2-i18n.js; perl -CSD -0777 -ne '$n++ while /(English (and|or) (French|Fran)|anglais et le fran|and nothing else|ne contient que|two languages|deux langues|rien d.autre|rien d\x{2019}autre)/gi; END{print "stale-body hits: ",$n+0,"\n"}' /tmp/qda-t2-i18n.js; grep -c "'tip\.settings'" /tmp/qda-t2-i18n.js   # 0 hits, and tip.settings count 0 — the pair clause is gone and the "nothing else" class had no subject (K13). Run with -CSD so the French U+2019 branch is not byte-blind (F11)</automated>
    <automated>node -e "const langs=new Set(['en','fr','zh-Hans']); const s=require('fs').readFileSync('/tmp/qda-t2-i18n.js','utf8'); const seen=new Set(), dup=[]; for(const m of s.matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)){const k=m[1]; if(langs.has(k))continue; if(seen.has(k))dup.push(k); else seen.add(k);} console.log('duplicates '+(dup.length?dup.join(','):'NONE'))"   # NONE — F13-corrected form</automated>
    <automated>shasum plugins/O-simpleGrain/Source/ui/public/modules/webview-drop-streaming.js plugins/O-simpleSampler/Source/ui/public/modules/webview-drop-streaming.js   # the two sums MUST still match (c7fe612e… at planning time). This task changes NEITHER copy — a divergence here means the unscanned module was edited, which this wave explicitly scoped out (K8)</automated>
    <automated>git status --short -- plugins/O-simpleGrain/Source/ui/public/modules/ | wc -l | tr -d ' '   # 0 — the unscanned module is REPORTED, not fixed. s71's narrowing arm, for s71's reasons</automated>
    <automated>node scripts/check-i18n.js --plugin O-simpleGrain 2>&1 | grep -oE "[0-9]+ module\(s\): .*"   # still "1 module(s): js/app.js" — check-i18n's [12] clause is BLIND to modules/webview-drop-streaming.js and its PASS is a claim about the SCAN, not about the page (K8)</automated>
    <automated>cat plugins/O-simpleGrain/Source/ui/public/index.html plugins/O-simpleGrain/Source/ui/public/css/styles.css > /tmp/qda-t2-all.txt; grep -c "PingFang SC" /tmp/qda-t2-all.txt; grep -oE "font-family:[^;}]*" /tmp/qda-t2-all.txt | sort -u   # >= 1, and every non-inherit stack ends '…, PingFang SC, Microsoft YaHei, <generic>' with the tail BEFORE the generic (W1). BOTH files searched — the CSS is external and 1129 L</automated>
    <automated>grep -c "&#31616;&#20307;&#20013;&#25991;" plugins/O-simpleGrain/Source/ui/public/index.html   # 1 — endonym byte-copied from O-Detune index.html:1039</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleGrain --mode box --report all >/tmp/qda-t2-mu.json 2>/tmp/qda-t2-mu.err; grep -E "undeclared-font|line-height-normal|wrap-count|svg-font-attr|identity|ESTIMATED" /tmp/qda-t2-mu.err   # undeclared-font 0 against a NON-EMPTY input, wrap-count 0 (baseline 0), identity ~502 nodes. Read the ESTIMATED header before treating a wrap-count delta as a defect (F8)</automated>
    <automated>node -e "const r=require('/tmp/qda-t2-mu.json'); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); const lh=z.filter(x=>x.lh==='normal'&&x.kids===0&&x.own&&x.own.trim()); const mv=lh.filter(x=>{const e=r.find(y=>y.key===x.key&&y.lang==='en'); return e&&Math.abs(e.h-x.h)>0.5}); console.log('zhHan='+z.length,'noCJKface='+z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff||'')).length,'lhResiduals='+lh.length,'ofWhichMOVED='+mv.length)"   # zhHan > 0, noCJKface 0, ofWhichMOVED 0. Residuals NAMED with their measured enH == zhH (N1)</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleGrain --mode box --verbose >/dev/null 2>/tmp/qda-t2-muv.err; grep -icE "did not resolve|unresolved|skipped state" /tmp/qda-t2-muv.err   # 0 — all FOUR states applied</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-simpleGrain --strict-tips 2>&1 | grep -iE "late|dead|clean"   # 0 late, 0 dead</automated>
    <automated>git status --short -- plugins/O-Orbit/libs/SAF | wc -l | tr -d ' '; git tag --points-at HEAD | wc -l | tr -d ' '; git status --short | grep -c "O-Strata"   # 0, 0, 2</automated>
    <human-check>The one-arm / two-arm reader pair was fired on this plugin and the result recorded verbatim — one-arm EMPTY, two-arm the real value. This is the wave's answer to wave 4g instruction 5 and to wave 4f's report that the set()-variable form was absent, and it is stated as a measurement rather than a rule restatement.</human-check>
    <human-check>Both version sources moved in the SAME edit: the string to 1.5.0 and OSIMPLEGRAIN_VERSION_CODE to 0x010500, with the 0x00MMmmpp encoding derived rather than transcribed, and tests/render-harness/CMakeLists.txt:57-58 confirmed to be the consumers. No gate in this repo reads the hex, so this is proved by the arithmetic probe and by reading the two files.</human-check>
    <human-check>`modules/webview-drop-streaming.js` was measured and REPORTED, not fixed: the ~12 showToast strings are named with their call sites, the byte-identity with O-simpleSampler's copy is confirmed by sha256, check-i18n's blindness to it is stated with the `[12]` line quoted, and the reason for scoping it out is written down (no calibrated gate, a second consumer, s71's narrowing-arm precedent). No partial keying was added.</human-check>
    <human-check>Every one of the ~153 triples was read with --verbose, in two or more chunks split on the CONCEPT (F17) with a fresh salt per chunk. BOTH refusal controls FIRED against a properly provenanced emit and each printed its own message (F10); --forward-provenance was passed a real STRING (F9); ids were identical AND IN ORDER before ingest (M12); the model that actually ran is recorded from stderr (F15). Each accepted drift carries a written collision-on-the-page reason.</human-check>
    <human-check>The state-EFFECT assertion fired on ALL FOUR click states, and the French `997.22 px natural NOWRAP` figure written into the states file was RE-MEASURED rather than inherited — a fixture that mirrors a measurement drifts silently.</human-check>
    <human-check>The pair clause was deleted in en and fr after its numeric claim ("the two drop-down menus") was re-verified against 3 &lt;select&gt; minus the language selector = 2; the exception list was kept; the comment was corrected as prose without spelling the superseded literal (C8). Both dead classes recorded as checked non-defects with their greps.</human-check>
  </verify>

  <done>
O-simpleGrain ships three languages at `reviewed: 'bt'` on all 153 rows, version **1.5.0** — read back
through the **two-arm** reader from a `set()` variable where **the one-arm reader returns EMPTY**, with
that pair recorded as the wave's proof that R9 is load-bearing. **Both** version sources moved in one
edit: the string to 1.5.0 and `OSIMPLEGRAIN_VERSION_CODE` to `0x010500`, verified by an arithmetic
probe against the `0x00MMmmpp` encoding. CHANGELOG written, built and installed. Geometry equal on all
three arms across all four states, with the states confirmed applied and the French `997.22 px` fixture
figure re-measured. Its Latin-safe house stack carries the CJK tail across `index.html` and the
1129-line external `css/styles.css`, and the wave's largest pin surface (12/7/7/28/4) was grepped
selector-by-selector before any append, with every floor removed before the box it floors was measured.
Its C++ codec is three-way and grep-proved on both lines; zero Han under `Source/`, positive control
fired. Its unscanned `modules/webview-drop-streaming.js` is **measured, named and reported unchanged**,
byte-identical to O-simpleSampler's copy, with `check-i18n`'s blindness to it quoted and the
scope-out reason written down. Two or more path-scoped commits; the submodule guard ran before each.
  </done>
</task>

<task type="auto">
  <name>Task 3: O-simpleSampler — the one row that reads ✅ Working while its bundle is already on disk, the second carrier of the byte-identical unscanned drop module, and the wave's only 37-tips-against-36-keys table (ZH4G-03, ZH4G-08, ZH4G-11)</name>
  <files>plugins/O-simpleSampler/Source/ui/public/js/i18n.js, plugins/O-simpleSampler/Source/ui/public/index.html, plugins/O-simpleSampler/Source/ui/public/css/styles.css, plugins/O-simpleSampler/Source/PluginProcessor.h, plugins/O-simpleSampler/CMakeLists.txt, plugins/O-simpleSampler/CHANGELOG.md</files>

  <required_reading>
- This plan's **Live observations**, **K1-K16**, **Execution shape**, **Stage-4 executor rules**, **THE SHIP BAR** and **Commit discipline** — all above. **K4 and K8 are this task's centre of gravity.**
- **Task 1's SUMMARY** (the method) and **Task 2's SUMMARY** (the unscanned-module verdict on the byte-identical copy — **inherit the verdict, re-fire the sha256**).
- `.planning/quick/260907-ja8-.../deferred-items.md` — **F7**, **F8**, **F9**, **F10**, **F11**, **F13**, **F14**, **F15**, **F17**, **F18**; wave-4g instructions **8**, **9**, **10**, **11**.
- `.planning/quick/260906-s71-.../260906-s71-deferred-items.md` — **D2**, the narrowing arm.
- `CLAUDE.md` — **the AU cache-clearing sequence and the dual-variant sweep in full.** This is the plugin whose registry row claims it is not installed.
- `scripts/measure-ui-README.md`.
  </required_reading>

  <precondition>Fire ALL of these on the tree as found, in the BACKGROUND, before any edit (M3). Measured at planning time; **re-fire rather than inherit**.
(a) `find plugins/O-simpleSampler/tests -name '*.js'` → **NOTHING** (K7).
(b) `node scripts/check-i18n.js --plugin O-simpleSampler` → **exit 0**, `got ["en","fr"]`, `36 I18N + 73 LABELS`, **`[2] 37 tip(s) bound`** — thirty-seven bound against thirty-six I18N keys, the wave's only such pair. **Read it and record what the extra binding is** before touching `TIP_BINDINGS`.
(c) `node scripts/i18n-zh-lint.js --plugin O-simpleSampler` → **exit 0**; `--self-test` → **10/10**.
(d) `node scripts/check-ui-labels.js --plugin O-simpleSampler` → **exit 0**, coverage **48 of 47** (N &gt; M, F18), **0 never became visible**.
(e) `measure-ui --mode box --report all` → identity **454 / 227 / 109**; `wrap-count 0`; `81 leaves compared, 58 ESTIMATED`; Han-gated screens a **VACUUM** (K12).
(f) Two-arm reader → **`1.4.4`** (quoted literal, L20). One-arm also returns it — **fire both** (R9).
(g) **`ls ~/Library/Audio/Plug-Ins/VST3/ | grep -i simplesampler` and the same for `Components/` → `O-simpleSampler-dev.vst3` and `O-simpleSampler-dev.component` are BOTH ALREADY ON DISK.** `PLUGINS.md` says `✅ Working`. **The row is wrong about the machine (K4).** Confirm both readings and record the contradiction.
(h) `find plugins/O-simpleSampler/Source/ui/public -name '*.js' -not -path '*/juce/*'` → **three** files including `modules/webview-drop-streaming.js`; `shasum` it against O-simpleGrain's copy → **identical** (`c7fe612e…` at planning time).
(i) `git branch --show-current` → `main`; one worktree; 0 tags at HEAD; `git status --short | grep -c "O-Strata"` → 2.
(j) Zero Han in `Source/**/*.{h,cpp}`, positive control firing on `js/i18n.js`.</precondition>

  <reversibility rating="reversible">Two to three path-scoped commits on one plugin's tree. Revertible with `git restore --source=&lt;sha&gt; -- plugins/O-simpleSampler`.</reversibility>

  <action>
**145 rows. 4 states, all clicks. `Source/ui/public/`, an external `css/styles.css` (939 L) beside a
314-line `index.html`, 980 x 720 parsed frame, `PLUGIN_CODE OsSm`, `IS_SYNTH TRUE`, target name equals
folder name, 36 I18N + 73 LABELS, 37 tips bound, `I18N_EXEMPT` mentioned 3 times.** Run **the full
ship bar** as Task 1 did. Three things are specific to this plugin.

**1. THE REGISTRY ROW IS WRONG ABOUT THE MACHINE, AND IT IS WRONG IN TWO CELLS (K4).**

`PLUGINS.md` reads `| O-simpleSampler | ✅ Working | 1.4.3 | …`. Both cells are stale:

- **the version** is one patch behind its own `CMakeLists.txt`, which reads a quoted `VERSION "1.4.4"`
  at L20 — the same 2026-09-03 suite-wide French hover-help rename that left all six rows behind;
- **the status** says the plugin is not installed, and **`O-simpleSampler-dev.vst3` and
  `O-simpleSampler-dev.component` are already on disk.** Measured at planning time.

**So "install it" is not a new install — it is a rebuild-and-reinstall of an already-present bundle,
and a registry cell that never followed.** The task description's framing is a faithful read of a
stale cell, and it is a **false prediction to record** (instruction 10).

**The MINOR bump from the CMakeLists value is 1.4.4 → 1.5.0.** This task rebuilds and reinstalls;
**Task 7 flips the status cell to `📦 Installed` and writes 1.5.0 in the same edit.**

**Follow CLAUDE.md's cache sequence exactly** — `killall -9 AudioComponentRegistrar`, clear both
AudioUnit caches, and let `build-and-install.sh`'s Phase 4 do the dual-variant sweep. **Watch for a
`⚠ Sweeping ALTERNATE-variant` warning and record whether one fired**: an unsuffixed
`O-simpleSampler.{vst3,component}` beside the `-dev` one would pin Logic's registry slot to whichever
was installed first. **None was present at planning time** — `-dev` only, both formats.

**2. THE SECOND CARRIER OF THE UNSCANNED DROP MODULE — INHERIT TASK 2's VERDICT, RE-FIRE THE SHA (K8).**

`Source/ui/public/modules/webview-drop-streaming.js` (504 L) is **byte-identical** to O-simpleGrain's
copy — `shasum` both and confirm, do not assume. `check-i18n`'s `[12]` reports `1 module(s):
js/app.js` and passes; the file is under `modules/`, not `js/`, and **the directory is the
discriminator**. Its ~12 `opts.showToast(...)` strings (`'Drop a .wav/.aif on a cell'`, `'Scanning
folder…'`, `'No audio files in folder'`, `'No samples loaded — all files failed'`, …) render English
on the Chinese page and on the French page today.

**Report it; do not fix it.** Task 2 wrote the verdict and the reasons — no calibrated gate, a second
consumer, s71's narrowing-arm precedent. **This task's job is to confirm the copy is byte-identical
and to leave it byte-identical.** A `git status --short -- .../modules/` that is not empty at the end
of this task means the wave silently widened its own scope.

**3. THIRTY-SEVEN TIPS AGAINST THIRTY-SIX I18N KEYS.** `check-i18n [2]` reports **37 tip(s) bound**
where the table declares 36 `I18N` keys. Every other plugin in the wave binds a count equal to or far
above its key count for an understandable reason (O-Polystutter binds 105 against 43 — a many-to-one
`TIP_BINDINGS` map). **Here the excess is exactly one.** **Read `TIP_BINDINGS` and name what the
thirty-seventh binding is** before authoring — an off-by-one in a binding map is the kind of thing a
localization pass makes permanent by writing a Chinese body for a key nothing reaches, or by leaving a
bound selector with no key. **Record the finding either way**; it is a checked non-defect if the map
legitimately binds two selectors to one key.

**THE FONT WORK — the tail half only.** One house stack, declared 9 times with **6** `inherit`, plus
`var(--symbol-font)`: `'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', serif`.
Times New Roman is installed (K11) → **Latin-safe, tail only**. Append `'PingFang SC', 'Microsoft
YaHei'` before the trailing `serif` (W1); keep the absent Garamond members. **Search `index.html` AND
`css/styles.css`.** Check `--symbol-font` carriers for TIP-ANCHOR status (N3) and record the verdict.
Verify by re-running the screen, never by reading the CSS.

**THE PIN SURFACE (R8, F7).** Comment-stripped across both files: **6 `min-width`, 4 `min-height`,
7 `line-height`, 18 `letter-spacing`, 4 `white-space`.** Grep every selector before appending. Remove
a floor entirely before measuring the box it floors; `min-width: 0` is not "no floor".

**FOUR CLICK STATES (K16, N1).** `#gear-btn`, `#help-toggle` (popover still open), a concept-preset
click, and pitch-mode STRETCH — whose state name records it as *"the readout face only
setupPitchModeReadout writes, never the resting string"*. Fire the state-EFFECT assertion on all four,
discriminating a `null` rect as a scrolled target rather than a coverage hole. **A scrolling click
that is not the LAST state scrolls every state after it** — check the order.

**THE STALE BODY — the pair clause, en and fr, 2 hits (K13).** English: *"English and French are
available; value readouts and the two drop-down menus stay in English."* Markup carries **3**
`<select>`; minus the language selector that is **2** — **re-verify, keep the exception list, delete
only the pair clause** in both languages, and correct the comment as prose (C8). Record the two DEAD
classes as checked non-defects: `tip.settings` count **0**, no `#tips-toggle`, not a
`scala-tuning-engine` consumer.

**THE C++ CODEC.** `Source/PluginProcessor.h` **L179-180** raw, doc comment at L176-178; the
accessors are at L173-174 and the atomic at L468; persistence uses
`juce::Identifier(kLanguageProp)` at L475 — **the wave's only `juce::Identifier`-wrapped key**, and it
needs no change. Locate everything by token, not by line number. Widen the ternary pair to three-way,
pure ASCII, in place. No language `AudioParameterChoice`.

**THE BLIND DISPATCH.** 145 rows — above the ~92-row threshold, so **two chunks minimum, split on the
CONCEPT** (F17), fresh salt per chunk, `--emit O-simpleSampler --plugin O-simpleSampler` (BOTH — R5),
`--forward-provenance "<string>"` (F9), explicit `--manifest`, `--verbose`, `--allowed-tools ""` from
outside the repo, `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` with the model read from stderr (F15).
**BOTH refusal controls against a properly provenanced emit** (F10). **ids identical AND IN ORDER**
(M12). Every triple read (R2). **Expect the product-name control to report a non-zero count** if the
table names the instrument — adjudicate it, do not treat the count as pass/fail.

Then: `TERMS` screen (R3 — **NONE** at planning time, 23 matched; re-fire); body correction and
**re-read** (C7); author at `'mt'` (R1), SPACED form (M11); endonym from O-Detune L1039 +
`I18N_EXEMPT`; ternary; font tail; geometry; commit at `'mt'`; blind read; promote **via a script
written to a FILE** (F14.5) at **1.5.0**; CHANGELOG; `./scripts/build-and-install.sh O-simpleSampler`
**in the background**, following CLAUDE.md's cache sequence. **No auval** — Task 7.

**Commit discipline.** Re-check branch and status immediately before every commit.
`git commit -- plugins/O-simpleSampler` only; options before the `--`. **Run the submodule guard
before every commit.** Never stage `plugins/O-Strata/.planning/*` or `.claude/agent-memory/*`. Never
tag. Session trailer on every message.
  </action>

  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/check-i18n.js --plugin O-simpleSampler >/tmp/qda-t3-ci.txt 2>&1; echo "exit=$?"; grep -E "\[1\] LANGUAGES|carries copy|tip\(s\) bound" /tmp/qda-t3-ci.txt   # exit 0; THREE languages; 36 I18N + 73 LABELS unchanged; still 37 tips bound — the count must not move, and what the 37th binding is has been named</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-simpleSampler >/tmp/qda-t3-cul.txt 2>&1; echo "exit=$?"; grep -cE "GEOMETRY DIFF\]\[(fr|zh-Hans)\] no non-label element moved" /tmp/qda-t3-cul.txt; grep -E "elements were VISIBLE|never became visible" /tmp/qda-t3-cul.txt   # exit 0; green on BOTH non-English arms across all four states; coverage still 48 of 47 (F18) with 0 never-visible</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-simpleSampler 2>&1 | tail -12   # 0 findings, 145 rows, BELOW SHIP BAR 0</automated>
    <automated>V=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' plugins/O-simpleSampler/CMakeLists.txt); echo "resolved: ${V:-EMPTY-READER-IS-THE-DEFECT}"   # 1.5.0 — bumped from the CMakeLists value 1.4.4, NOT from the registry's stale 1.4.3 (K4)</automated>
    <automated>ls ~/Library/Audio/Plug-Ins/VST3/ ~/Library/Audio/Plug-Ins/Components/ | grep -i simplesampler   # O-simpleSampler-dev.vst3 AND O-simpleSampler-dev.component present and freshly rebuilt. NO unsuffixed alternate-variant bundle beside either, or Logic's registry slot is pinned to whichever was installed first (CLAUDE.md)</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleSampler/Source/PluginProcessor.h > /tmp/qda-t3-pp.h; grep -c 'zh-Hans' /tmp/qda-t3-pp.h; node -e "const s=require('fs').readFileSync('/tmp/qda-t3-pp.h','utf8'); for (const fn of ['languageCode','languageIndex']) { const m = s.match(new RegExp(fn + '[\\\\s\\\\S]{0,240}?;')); console.log(fn + ': ' + (m && /zh/.test(m[0]) ? 'THREE-WAY' : 'STILL TWO-WAY')); }"   # >= 2, THREE-WAY on both — proved by NAME, comments stripped</automated>
    <automated>find plugins/O-simpleSampler/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +; perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-simpleSampler/Source/ui/public/js/i18n.js   # nothing, then the control MUST fire</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleSampler/Source/ui/public/js/i18n.js > /tmp/qda-t3-i18n.js; perl -CSD -0777 -ne '$n++ while /(English (and|or) (French|Fran)|anglais et le fran|and nothing else|ne contient que|two languages|deux langues|rien d.autre|rien d\x{2019}autre)/gi; END{print "stale-body hits: ",$n+0,"\n"}' /tmp/qda-t3-i18n.js; grep -c "'tip\.settings'" /tmp/qda-t3-i18n.js   # 0 and 0 — pair clause gone; the "nothing else" class had no subject (K13). -CSD so the French U+2019 branch is not byte-blind (F11)</automated>
    <automated>node -e "const langs=new Set(['en','fr','zh-Hans']); const s=require('fs').readFileSync('/tmp/qda-t3-i18n.js','utf8'); const seen=new Set(), dup=[]; for(const m of s.matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)){const k=m[1]; if(langs.has(k))continue; if(seen.has(k))dup.push(k); else seen.add(k);} console.log('duplicates '+(dup.length?dup.join(','):'NONE'))"   # NONE — F13-corrected form</automated>
    <automated>shasum plugins/O-simpleSampler/Source/ui/public/modules/webview-drop-streaming.js plugins/O-simpleGrain/Source/ui/public/modules/webview-drop-streaming.js; git status --short -- plugins/O-simpleSampler/Source/ui/public/modules/ | wc -l | tr -d ' '   # the two sums MATCH, and 0 lines changed — the unscanned module is REPORTED, not fixed, and is left byte-identical to its twin (K8)</automated>
    <automated>node scripts/check-i18n.js --plugin O-simpleSampler 2>&1 | grep -oE "[0-9]+ module\(s\): .*"   # still "1 module(s): js/app.js" — the [12] clause is BLIND to modules/webview-drop-streaming.js and its PASS is a claim about the SCAN (K8)</automated>
    <automated>cat plugins/O-simpleSampler/Source/ui/public/index.html plugins/O-simpleSampler/Source/ui/public/css/styles.css > /tmp/qda-t3-all.txt; grep -c "PingFang SC" /tmp/qda-t3-all.txt; grep -oE "font-family:[^;}]*" /tmp/qda-t3-all.txt | sort -u   # >= 1, tail BEFORE the generic on every non-inherit stack (W1). BOTH files searched — the CSS is external</automated>
    <automated>grep -c "&#31616;&#20307;&#20013;&#25991;" plugins/O-simpleSampler/Source/ui/public/index.html   # 1 — endonym byte-copied from O-Detune index.html:1039</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleSampler --mode box --report all >/tmp/qda-t3-mu.json 2>/tmp/qda-t3-mu.err; grep -E "undeclared-font|line-height-normal|wrap-count|svg-font-attr|identity|ESTIMATED" /tmp/qda-t3-mu.err   # undeclared-font 0 against a NON-EMPTY input, wrap-count 0 (baseline 0), identity ~454 nodes. Read the ESTIMATED header before treating a wrap-count delta as a defect (F8)</automated>
    <automated>node -e "const r=require('/tmp/qda-t3-mu.json'); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); const lh=z.filter(x=>x.lh==='normal'&&x.kids===0&&x.own&&x.own.trim()); const mv=lh.filter(x=>{const e=r.find(y=>y.key===x.key&&y.lang==='en'); return e&&Math.abs(e.h-x.h)>0.5}); console.log('zhHan='+z.length,'noCJKface='+z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff||'')).length,'lhResiduals='+lh.length,'ofWhichMOVED='+mv.length)"   # zhHan > 0, noCJKface 0, ofWhichMOVED 0 — residuals NAMED with enH == zhH (N1)</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleSampler --mode box --verbose >/dev/null 2>/tmp/qda-t3-muv.err; grep -icE "did not resolve|unresolved|skipped state" /tmp/qda-t3-muv.err   # 0 — all FOUR states applied</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-simpleSampler --strict-tips 2>&1 | grep -iE "late|dead|clean"   # 0 late, 0 dead</automated>
    <automated>git status --short -- plugins/O-Orbit/libs/SAF | wc -l | tr -d ' '; git tag --points-at HEAD | wc -l | tr -d ' '; git status --short | grep -c "O-Strata"   # 0, 0, 2</automated>
    <human-check>The registry contradiction was measured and recorded in BOTH cells before any edit: the row says ✅ Working while both bundles were already on disk, and it says 1.4.3 while the CMakeLists reads 1.4.4. The bump was taken from the CMakeLists value. Whether `build-and-install.sh` emitted a `⚠ Sweeping ALTERNATE-variant` warning is recorded either way.</human-check>
    <human-check>The thirty-seventh tip binding was READ and NAMED — either a legitimate many-to-one binding recorded as a checked non-defect, or an off-by-one recorded as a finding. It was not left as an unexplained number, and no Chinese body was written for a key nothing reaches.</human-check>
    <human-check>`modules/webview-drop-streaming.js` was confirmed byte-identical to O-simpleGrain's copy by sha256 and left byte-identical. Task 2's scope-out verdict was inherited with its reasons restated, check-i18n's blindness re-quoted from the `[12]` line, and no partial keying was added.</human-check>
    <human-check>Every one of the ~145 triples was read with --verbose, in two or more chunks split on the CONCEPT (F17) with a fresh salt per chunk. BOTH refusal controls FIRED against a properly provenanced emit (F10); --forward-provenance was passed a real STRING (F9); ids identical AND IN ORDER (M12); the model recorded from stderr (F15). The product-name control's hits were adjudicated rather than treated as pass/fail. Each accepted drift carries a written collision-on-the-page reason.</human-check>
    <human-check>The state-EFFECT assertion fired on ALL FOUR click states, with the state ORDER checked — a scrolling click that is not the last state scrolls every state after it. Any null rect was discriminated as a scrolled target rather than a coverage hole (N1).</human-check>
  </verify>

  <done>
O-simpleSampler ships three languages at `reviewed: 'bt'` on all 145 rows, version **1.5.0** read back
through the two-arm reader from a CMakeLists value that was **1.4.4, not the 1.4.3 the registry
claimed**, CHANGELOG written, rebuilt and reinstalled through CLAUDE.md's cache sequence with the
dual-variant sweep result recorded. **The registry contradiction is measured and written down in both
cells** — status and version — for Task 7 to correct in one edit. Geometry equal on all three arms
across all four states, with the state order checked. Its Latin-safe house stack carries the CJK tail
across `index.html` and the external `css/styles.css`. Its C++ codec is three-way and grep-proved on
both lines; zero Han under `Source/`, positive control fired; its `juce::Identifier`-wrapped
persistence key is recorded as needing no change. Its `modules/webview-drop-streaming.js` is confirmed
byte-identical to O-simpleGrain's copy and left unchanged, with the verdict inherited and restated.
The thirty-seventh tip binding is named. Two or more path-scoped commits; the submodule guard ran
before each.
  </done>
</task>

<task type="auto">
  <name>Task 4: O-simpleSubtractive — the ONLY plugin in the wave with a non-zero geometry baseline, and the second carrier of a French wrap the carry-forwards attribute to a different plugin (ZH4G-04, ZH4G-09, ZH4G-12)</name>
  <files>plugins/O-simpleSubtractive/Source/ui/public/js/i18n.js, plugins/O-simpleSubtractive/Source/ui/public/index.html, plugins/O-simpleSubtractive/Source/ui/public/css/styles.css, plugins/O-simpleSubtractive/Source/PluginProcessor.h, plugins/O-simpleSubtractive/CMakeLists.txt, plugins/O-simpleSubtractive/CHANGELOG.md</files>

  <required_reading>
- This plan's **Live observations**, **K1-K16**, **Execution shape**, **Stage-4 executor rules**, **THE SHIP BAR** and **Commit discipline** — all above. **K9 is this task's centre of gravity.**
- **Task 1's SUMMARY** — the method, the external-CSS font location, the blind-dispatch shape.
- `.planning/quick/260906-h8y-.../deferred-items.md` — **D7 in full**: "a pre-existing French wrap on O-simpleFM", `div.routing-label` "Signal Path", en=1 fr=2, **and its note that the Chinese arm of the same node is one line on O-simpleFM**. That note is a PRIOR for this plugin, not a measurement of it.
- `.planning/quick/260907-ja8-.../deferred-items.md` — **F1** (a finding derived from one carrier had no second site to check it against — the same shape as K9), **F7**, **F8**, **F9**, **F10**, **F11**, **F13**, **F14**, **F15**, **F17**, **F18**.
- `scripts/measure-ui-README.md` — the `wrap-count` screen and its ESTIMATED denominator in full.
- `CLAUDE.md`.
  </required_reading>

  <precondition>Fire ALL of these on the tree as found, in the BACKGROUND, before any edit (M3). Measured at planning time; **re-fire rather than inherit**.
(a) `find plugins/O-simpleSubtractive/tests -name '*.js'` → **NOTHING** (K7).
(b) `node scripts/check-i18n.js --plugin O-simpleSubtractive` → **exit 0**, `got ["en","fr"]`, `36 I18N + 61 LABELS`, `[2] 36 tip(s) bound`, `[12] 1 module(s): js/app.js`.
(c) `node scripts/i18n-zh-lint.js --plugin O-simpleSubtractive` → **exit 0**; `--self-test` → **10/10**.
(d) `node scripts/check-ui-labels.js --plugin O-simpleSubtractive` → **exit 0**, coverage **55 of 54** (F18), **0 never became visible**.
(e) **`measure-ui --mode box --report all` → `wrap-count: 1 finding(s)`, NOT zero:**
`html/body[1]/div[1]/section[2]/div[1] (div.routing-label) "Signal Path" en=1 line(s), fr=2 line(s)`.
**That is the baseline. It is pre-existing, it is French, and it is not this wave's to fix.** Identity **520 / 260 / 120**; `92 leaves compared, 70 ESTIMATED`; the Han-gated screens are a **VACUUM** (K12).
(f) Two-arm reader → **`1.4.1`** (quoted literal, L16). One-arm also returns it — **fire both** (R9).
(g) `grep -rl "routing-label" plugins/ | grep -E "index.html|styles.css"` → **five files across THREE plugins** — O-simpleFM (markup + CSS), **O-simpleSubtractive (markup + CSS)**, O-simpleGrain (CSS only, no markup carrier). **Fire this before believing the carry-forward (K9).**
(h) `find plugins/O-simpleSubtractive/Source/ui/public -name '*.js' -not -path '*/juce/*'` → **two** files. **No unscanned module JS** — like the tracer, unlike four of the six (K8).
(i) `git branch --show-current` → `main`; one worktree; 0 tags at HEAD; `git status --short | grep -c "O-Strata"` → 2.
(j) Zero Han in `Source/**/*.{h,cpp}`, positive control firing on `js/i18n.js`.</precondition>

  <reversibility rating="reversible">Two to three path-scoped commits on one plugin's tree. Revertible with `git restore --source=&lt;sha&gt; -- plugins/O-simpleSubtractive`.</reversibility>

  <action>
**133 rows. 3 states, all clicks. `Source/ui/public/`, an external `css/styles.css` (743 L) beside a
330-line `index.html`, 1180 x 820 parsed frame — the wave's WIDEST, `PLUGIN_CODE OSiS`, `IS_SYNTH
TRUE`, target name equals folder name, 36 I18N + 61 LABELS, 36 tips bound, `I18N_EXEMPT` mentioned 4
times.** Run **the full ship bar** as Task 1 did. Two things are specific to this plugin.

**1. THE WRAP-COUNT BASELINE IS ONE, AND THE CARRY-FORWARD NAMES A DIFFERENT PLUGIN (K9).**

On the tree as found, before a byte of Chinese:

```
wrap-count: 1 finding(s)
    html/body[1]/div[1]/section[2]/div[1]  (div.routing-label)  "Signal Path"  en=1 line(s), fr=2 line(s)
```

Wave 4e D7 records this as *"a pre-existing French wrap on **O-simpleFM**"* and wave 4f D12
re-inherits it under that name. **It is not a property of O-simpleFM.** `routing-label` is a shared
class with carriers in three trees; O-simpleFM was measured in wave 4e because O-simpleFM was in wave
4e, and **nobody measured the other carriers, so nobody saw the second one.** This is F1's shape
exactly: a finding derived from one carrier looked measured and had no second site to check it
against.

**Three consequences, and each is a different mistake if it is missed:**

- **The baseline is 1, not 0.** A task that treats 1 as a regression will chase a French wrap it did
  not cause. A task that drives to 0 will "fix" a French geometry question with no Chinese content and
  no localization budget attached — which is exactly what 4e D7 declined to do, on purpose.
- **The target is "still 1, and the finding is still the same node on the same arm."** Not zero. If
  the count rises to 2, or if the *node* changes, **that is this wave's regression** and must be
  chased.
- **4e D7's note that the Chinese arm of that node is one line is a PRIOR from O-simpleFM, not a
  measurement of this plugin.** Measure this plugin's zh arm and record what it actually read.

**Report the correction to the carry-forward's SCOPE, not only to its plugin name** — carry it into
the deferred items as a shared-class finding with a named carrier list, so the next reader does not
re-derive it from one site a third time.

**And read F8 before treating any `wrap-count` movement as a defect.** The screen divides a box by an
**assumed** line box: `92 text-bearing leaf node(s) compared; 70 of them ESTIMATED at fsn * 1.2`.
Pinning a `line-height` ratio moves a node out of the estimated set and changes its divisor **with no
pixel changing**, and the direction is not guaranteed. **Read the screen's own ESTIMATED header on
every run and compare it to the 70 measured here.**

**2. IT IS ONE OF TWO PLUGINS WITH NO UNSCANNED MODULE JS FILE (K8).** `find` over the whole UI root
returns exactly `js/i18n.js` and `js/app.js`. Together with the tracer that is the controlled converse
to the four plugins that carry one — **record it as measured**, because a zero from a whole-root
`find` is a measurement and a zero from a `js/*.js` glob is a query artefact (this planning pass made
that exact error and it is why the census is spelled with `find` over the root).

**THE FONT WORK — the tail half only.** One house stack, declared 11 times with **7** `inherit`, plus
`var(--symbol-font)`: `'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', serif`.
Times New Roman installed (K11) → **Latin-safe, tail only**: `'PingFang SC', 'Microsoft YaHei'` before
the trailing `serif` (W1). Keep the absent Garamond members. **Search `index.html` AND
`css/styles.css`.** Check `--symbol-font` carriers for TIP-ANCHOR status (N3) and record the verdict.
Verify by re-running the screen.

**THE PIN SURFACE (R8, F7).** Comment-stripped across both files: **6 `min-width`, 1 `min-height`,
3 `line-height`, 23 `letter-spacing`, 2 `white-space`.** The 23 `letter-spacing` declarations are the
wave's second-largest such surface — trim under `html[lang="zh-Hans"]` **with the trim value
MEASURED**, precedent `plugins/O-Prism/Source/ui/public/index.html:1298`. Grep every selector before
appending. **Remove a floor entirely before measuring the box it floors** (F7). **And if a
`line-height` ratio pin is added, expect the `wrap-count` ESTIMATED denominator to move and read F8
before calling the result a change.**

**THREE CLICK STATES (K16, N1).** `#gear-btn`, `#help-toggle`, and a lesson-preset click
(`label.captionAcid`). Fire the state-EFFECT assertion on all three; discriminate a `null` rect as a
scrolled target rather than a coverage hole; check the ORDER, because a scrolling click that is not
the last state scrolls every state after it.

**THE STALE BODY — the pair clause, en and fr, 2 hits, and this one names FOUR (K13).** English:
*"English and French are available; value readouts and the **four** drop-down menus stay in English."*
Markup carries **5** `<select>`; minus the language selector that is **4**, so the claim is TRUE
today — **re-verify it, keep the exception list, and delete only the pair clause** in both languages.
This is the wave's only plugin whose exception list names a number other than two, so the verification
is not a formality. Correct the comment as prose without spelling the superseded literal (C8). Record
the two DEAD classes as checked non-defects: `tip.settings` count **0**, no `#tips-toggle`, not a
`scala-tuning-engine` consumer.

**THE C++ CODEC.** `Source/PluginProcessor.h` **L204-205** raw, doc comment at L201-203, atomic at
L199 — locate by the tokens `languageCode` / `languageIndex`. Widen to three-way, pure ASCII, in
place. Persistence is a ValueTree `getProperty("uiLanguage")` with a string-literal key; no change. No
language `AudioParameterChoice`.

**THE BLIND DISPATCH.** 133 rows — above ~92, so **two chunks minimum, split on the CONCEPT** (F17),
fresh salt per chunk, `--emit O-simpleSubtractive --plugin O-simpleSubtractive` (BOTH — R5),
`--forward-provenance "<string>"` (F9), explicit `--manifest`, `--verbose`, `--allowed-tools ""` from
outside the repo, `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` with the model read from stderr (F15).
**BOTH refusal controls against a properly provenanced emit** (F10). **ids identical AND IN ORDER**
(M12). Every triple read (R2).

Then: `TERMS` screen (R3 — **NONE**, 16 matched, the wave's smallest matched set; re-fire); body
correction and **re-read** (C7); author at `'mt'` (R1), SPACED form (M11); endonym from O-Detune L1039
+ `I18N_EXEMPT`; ternary; font tail; geometry; commit at `'mt'`; blind read; promote **via a script
written to a FILE** (F14.5) at **1.5.0** (from the CMakeLists **1.4.1**, not the registry's stale
1.4.0); CHANGELOG; `./scripts/build-and-install.sh O-simpleSubtractive` **in the background**. **No
auval** — Task 7.

**Commit discipline.** Re-check branch and status immediately before every commit.
`git commit -- plugins/O-simpleSubtractive` only; options before the `--`. **Run the submodule guard
before every commit.** Never stage `plugins/O-Strata/.planning/*` or `.claude/agent-memory/*`. Never
tag. Session trailer on every message.
  </action>

  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/check-i18n.js --plugin O-simpleSubtractive >/tmp/qda-t4-ci.txt 2>&1; echo "exit=$?"; grep -E "\[1\] LANGUAGES|carries copy|tip\(s\) bound" /tmp/qda-t4-ci.txt   # exit 0; THREE languages; 36 I18N + 61 LABELS unchanged; 36 tips</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-simpleSubtractive >/tmp/qda-t4-cul.txt 2>&1; echo "exit=$?"; grep -cE "GEOMETRY DIFF\]\[(fr|zh-Hans)\] no non-label element moved" /tmp/qda-t4-cul.txt; grep -E "elements were VISIBLE|never became visible" /tmp/qda-t4-cul.txt   # exit 0; green on BOTH non-English arms across all three states; coverage still 55 of 54 (F18) with 0 never-visible</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-simpleSubtractive 2>&1 | tail -12   # 0 findings, 133 rows, BELOW SHIP BAR 0</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleSubtractive --mode box --report all >/tmp/qda-t4-mu.json 2>/tmp/qda-t4-mu.err; grep -A3 "wrap-count" /tmp/qda-t4-mu.err   # STILL EXACTLY 1 finding, STILL div.routing-label "Signal Path" en=1 fr=2. The BASELINE IS 1, NOT 0 (K9) — a 2, or a different node, is this wave's regression; a 0 means a French geometry question was silently "fixed" that 4e D7 declined on purpose. Read the ESTIMATED header on the same run and compare it to the 70 measured at planning time (F8)</automated>
    <automated>grep -rl "routing-label" plugins/ 2>/dev/null | grep -E "index.html|styles.css"   # FIVE files across THREE plugins — O-simpleFM, O-simpleSubtractive, O-simpleGrain. The carry-forward names only O-simpleFM; the finding is a SHARED CLASS with at least two markup carriers, and one of them is in this wave's own set (K9, F1's shape)</automated>
    <automated>V=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' plugins/O-simpleSubtractive/CMakeLists.txt); echo "resolved: ${V:-EMPTY-READER-IS-THE-DEFECT}"   # 1.5.0 — bumped from the CMakeLists value 1.4.1, NOT the registry's stale 1.4.0 (K4)</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleSubtractive/Source/PluginProcessor.h > /tmp/qda-t4-pp.h; grep -c 'zh-Hans' /tmp/qda-t4-pp.h; node -e "const s=require('fs').readFileSync('/tmp/qda-t4-pp.h','utf8'); for (const fn of ['languageCode','languageIndex']) { const m = s.match(new RegExp(fn + '[\\\\s\\\\S]{0,240}?;')); console.log(fn + ': ' + (m && /zh/.test(m[0]) ? 'THREE-WAY' : 'STILL TWO-WAY')); }"   # >= 2, THREE-WAY on both</automated>
    <automated>find plugins/O-simpleSubtractive/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +; perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-simpleSubtractive/Source/ui/public/js/i18n.js   # nothing, then the control MUST fire</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-simpleSubtractive/Source/ui/public/js/i18n.js > /tmp/qda-t4-i18n.js; perl -CSD -0777 -ne '$n++ while /(English (and|or) (French|Fran)|anglais et le fran|and nothing else|ne contient que|two languages|deux langues|rien d.autre|rien d\x{2019}autre)/gi; END{print "stale-body hits: ",$n+0,"\n"}' /tmp/qda-t4-i18n.js; grep -c "'tip\.settings'" /tmp/qda-t4-i18n.js   # 0 and 0 (K13, F11)</automated>
    <automated>grep -c '<select' plugins/O-simpleSubtractive/Source/ui/public/index.html   # 5 — minus the language selector that is FOUR, so the exception list's "four drop-down menus" claim is TRUE and was KEPT. This is the wave's only exception list naming a number other than two, so the check is not a formality</automated>
    <automated>node -e "const langs=new Set(['en','fr','zh-Hans']); const s=require('fs').readFileSync('/tmp/qda-t4-i18n.js','utf8'); const seen=new Set(), dup=[]; for(const m of s.matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)){const k=m[1]; if(langs.has(k))continue; if(seen.has(k))dup.push(k); else seen.add(k);} console.log('duplicates '+(dup.length?dup.join(','):'NONE'))"   # NONE — F13-corrected form</automated>
    <automated>find plugins/O-simpleSubtractive/Source/ui/public -name '*.js' -not -path '*/juce/*' | wc -l | tr -d ' '   # 2 — no unscanned module JS. A whole-root `find` zero is a measurement; a `js/*.js` glob zero is a query artefact (K8)</automated>
    <automated>cat plugins/O-simpleSubtractive/Source/ui/public/index.html plugins/O-simpleSubtractive/Source/ui/public/css/styles.css > /tmp/qda-t4-all.txt; grep -c "PingFang SC" /tmp/qda-t4-all.txt; grep -oE "font-family:[^;}]*" /tmp/qda-t4-all.txt | sort -u   # >= 1, tail BEFORE the generic on every non-inherit stack (W1). BOTH files searched</automated>
    <automated>grep -c "&#31616;&#20307;&#20013;&#25991;" plugins/O-simpleSubtractive/Source/ui/public/index.html   # 1 — endonym byte-copied from O-Detune index.html:1039</automated>
    <automated>node -e "const r=require('/tmp/qda-t4-mu.json'); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); const lh=z.filter(x=>x.lh==='normal'&&x.kids===0&&x.own&&x.own.trim()); const mv=lh.filter(x=>{const e=r.find(y=>y.key===x.key&&y.lang==='en'); return e&&Math.abs(e.h-x.h)>0.5}); const rl=r.filter(x=>/routing-label/.test(x.id||'')||/routing-label/.test(x.cls||'')); console.log('zhHan='+z.length,'noCJKface='+z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff||'')).length,'lhResiduals='+lh.length,'ofWhichMOVED='+mv.length,'routingLabelRows='+rl.length)"   # zhHan > 0, noCJKface 0, ofWhichMOVED 0. routingLabelRows > 0 so the zh arm of div.routing-label WAS measured on this plugin rather than inherited from 4e D7's O-simpleFM note</automated>
    <automated>node scripts/measure-ui.js --plugin O-simpleSubtractive --mode box --verbose >/dev/null 2>/tmp/qda-t4-muv.err; grep -icE "did not resolve|unresolved|skipped state" /tmp/qda-t4-muv.err   # 0 — all THREE states applied</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-simpleSubtractive --strict-tips 2>&1 | grep -iE "late|dead|clean"   # 0 late, 0 dead</automated>
    <automated>git status --short -- plugins/O-Orbit/libs/SAF | wc -l | tr -d ' '; git tag --points-at HEAD | wc -l | tr -d ' '; git status --short | grep -c "O-Strata"   # 0, 0, 2</automated>
    <human-check>The wrap-count baseline of 1 was measured BEFORE any edit and is still 1 after, on the same node and the same arm. The `div.routing-label` French wrap was NOT fixed — 4e D7 declined it on purpose as a French geometry question with no Chinese content — and the correction to the carry-forward's SCOPE is written down: it is a shared class with carriers in three trees, not a property of O-simpleFM, and this plugin is a second carrier nobody had measured.</human-check>
    <human-check>The zh arm of div.routing-label was MEASURED on this plugin and its line count recorded. 4e D7's note that the Chinese arm is one line is a prior from O-simpleFM and was treated as one.</human-check>
    <human-check>The ESTIMATED denominator was read on every measure-ui run and compared against the 70-of-92 baseline, so that any wrap-count movement caused by a ratio pin moving a node out of the estimated set (F8) is distinguished from a real geometry change. The direction of such a movement is not guaranteed and was not assumed.</human-check>
    <human-check>Every one of the ~133 triples was read with --verbose, in chunks split on the CONCEPT (F17) with a fresh salt per chunk. BOTH refusal controls FIRED against a properly provenanced emit (F10); --forward-provenance was passed a real STRING (F9); ids identical AND IN ORDER (M12); the model recorded from stderr (F15). Each accepted drift carries a written reason.</human-check>
    <human-check>The pair clause was deleted in en and fr after its "four drop-down menus" claim was re-verified against 5 &lt;select&gt; minus the language selector = 4 — the wave's only exception list naming a number other than two. The list was kept; the comment corrected as prose (C8). Both dead classes recorded as checked non-defects with their greps.</human-check>
  </verify>

  <done>
O-simpleSubtractive ships three languages at `reviewed: 'bt'` on all 133 rows, version **1.5.0** read
back through the two-arm reader from a CMakeLists value that was **1.4.1, not the 1.4.0 the registry
claimed**, CHANGELOG written, built and installed. Geometry equal on all three arms across all three
states — **with the `wrap-count` baseline held at 1, not driven to 0**, on the same node and the same
arm, and with the correction to wave 4e D7's SCOPE written down: `div.routing-label` is a shared class
with carriers in three trees and this plugin is a second one nobody had measured. Its Latin-safe house
stack carries the CJK tail across `index.html` and the external `css/styles.css`, with the wave's
second-largest `letter-spacing` surface trimmed at a MEASURED value. Its C++ codec is three-way and
grep-proved on both lines; zero Han under `Source/`, positive control fired. Its zero unscanned module
JS files is recorded as measured from a whole-root `find`. Two or more path-scoped commits; the
submodule guard ran before each.
  </done>
</task>

<task type="auto">
  <name>Task 5: O-Polystutter — the wave's largest markup and densest tip map, its only fully-inline stylesheet, its only codec with no doc comment, and the entry with no body that makes its row arithmetic disagree (ZH4G-05, ZH4G-08, ZH4G-10)</name>
  <files>plugins/O-Polystutter/Source/ui/public/js/i18n.js, plugins/O-Polystutter/Source/ui/public/index.html, plugins/O-Polystutter/Source/PluginProcessor.h, plugins/O-Polystutter/CMakeLists.txt, plugins/O-Polystutter/CHANGELOG.md</files>

  <required_reading>
- This plan's **Live observations**, **K1-K16**, **Execution shape**, **Stage-4 executor rules**, **THE SHIP BAR** and **Commit discipline** — all above. **K3 and K8 are this task's centre of gravity.**
- **Task 1's SUMMARY** (the method) and **Task 2's SUMMARY** (the unscanned-module scope-out verdict and its reasons).
- `.planning/quick/260907-ja8-.../deferred-items.md` — **F6** (M8's ratio table is LEAF-scoped and does NOT cover a form control), **F8**, **F9**, **F10**, **F11**, **F13**, **F14**, **F15**, **F17**, **F18**.
- `.planning/quick/260906-s71-.../260906-s71-deferred-items.md` — **D2** in full, including the sentence that `preset-manager.js` alone has **eighteen consumers** and that keying it is a rollout.
- `plugins/O-Polystutter/Source/ui/public/js/i18n.js` — the `TIP_BINDINGS` map in full. **105 bindings against 43 I18N keys**; the many-to-one shape must be understood before a single body is authored.
- `scripts/measure-ui-README.md`, `CLAUDE.md`.
  </required_reading>

  <precondition>Fire ALL of these on the tree as found, in the BACKGROUND, before any edit (M3). Measured at planning time; **re-fire rather than inherit**.
(a) `find plugins/O-Polystutter/tests -name '*.js'` → **NOTHING**. Its `tests/` holds **only** `i18n-states.json` — the wave's only plugin with no `render-harness/` either (K7).
(b) `node scripts/check-i18n.js --plugin O-Polystutter` → **exit 0**, `got ["en","fr"]`, `43 I18N + 48 LABELS`, **`[2] 105 tip(s) bound`** — the wave's densest map, many-to-one, and `[12] 2 module(s): js/app.js, js/parameter-bindings.js`.
(c) `node scripts/i18n-zh-lint.js --plugin O-Polystutter` → **exit 0**; `--self-test` → **10/10**.
(d) `node scripts/check-ui-labels.js --plugin O-Polystutter` → **exit 0**, coverage **97 of 97** — an EXACT fraction, one of only two in the wave — **0 never became visible**.
(e) `measure-ui --mode box --report all` → identity **1068 / 534 / 212** — the wave's largest page by every measure; `wrap-count 0`; **`164 leaves compared, 154 ESTIMATED`** — a 94% estimated fraction, the wave's second-highest, so read F8 before treating any post-work `wrap-count` delta as a defect. Han-gated screens a **VACUUM** (K12).
(f) Two-arm reader → **`1.14.3`** (UNQUOTED literal, L11). One-arm also returns it — **fire both** (R9).
(g) **The row arithmetic disagrees by one: `2 × 43 + 48 = 134`, the emitter says `133`.** Confirm the cause before authoring: `I18N['msg-delete-preset']` has a `t` and **no non-empty `b`**, and every other one of the 43 has both (K3).
(h) `find plugins/O-Polystutter/Source/ui/public -name '*.js' -not -path '*/juce/*'` → **four** files, including `modules/preset-manager.js` which `check-i18n` does **not** scan — while `js/parameter-bindings.js`, one directory up, **is** scanned. **The directory is the discriminator** (K8).
(i) `git branch --show-current` → `main`; one worktree; 0 tags at HEAD; `git status --short | grep -c "O-Strata"` → 2.
(j) Zero Han in `Source/**/*.{h,cpp}`, positive control firing on `js/i18n.js`.</precondition>

  <reversibility rating="reversible">Two to three path-scoped commits on one plugin's tree. Revertible with `git restore --source=&lt;sha&gt; -- plugins/O-Polystutter`.</reversibility>

  <action>
**133 rows. 3 states, all clicks. `Source/ui/public/`, NO external stylesheet — all 12 font
declarations inline in a 1776-line `index.html`, the wave's largest markup by a factor of four. 1000 x
690 parsed frame, `PLUGIN_CODE OuPs`, `IS_SYNTH FALSE` — an EFFECT, `juce_add_plugin` target
`OPolystutter` which is NOT the folder name, 43 I18N + 48 LABELS, 105 tips bound, `I18N_EXEMPT`
mentioned 3 times.** Run **the full ship bar** as Task 1 did. Four things are specific to this plugin.

**1. ONE ENTRY HAS NO BODY, AND WRITING ONE CHANGES A NUMBER THE CLOSE-OUT ASSERTS (K3).**

`2 × I18N + LABELS` reproduces the emitter's row count exactly on five of six. Here it gives **134**
against the emitter's **133**. The cause is measured: **`msg-delete-preset` has a `t` and no non-empty
`b`.** All 42 others have both.

**Mirror the English SHAPE per entry. Do not normalise it.** An executor that writes a `zh-Hans` block
with a `b` on that entry adds a 134th row the emitter did not ask for, breaks the wave's `820 → 5441`
arithmetic, and will also mis-count its own blind batch. An executor that assumes every I18N entry has
two halves will chunk wrong. **Confirm the shape per entry as you author, and state the 133 in the
SUMMARY with the reason.**

**2. 105 TIP BINDINGS AGAINST 43 KEYS — A MANY-TO-ONE MAP, AND IT IS THE WAVE'S DENSEST.**

`TIP_BINDINGS` binds **105** selectors to **43** `I18N` keys. That is not an error — it is the
shape a plugin with many controls sharing a concept takes. But it has two consequences for this task:

- **the tooltip surface is entirely driven from `TIP_BINDINGS`** (markup carries **0** `data-tip`
  attributes, as on all six), so one Chinese body reaches up to several controls. **A body that reads
  naturally on one anchor must read naturally on all of them** — check the anchors a body serves
  before writing it, not after;
- **the R3 downstream collision check has more surface here than anywhere in the wave.** 34 captions
  matched the glossary at planning time, the highest of the six. Run the mechanical downstream check
  after authoring — group the zh renderings and report any two *different* keys sharing one, excluding
  same-control pairs and same-English pairs.

**3. THE ONLY CODEC IN THE WAVE WITH NO DOC COMMENT ABOVE IT.**

`Source/PluginProcessor.h` **L82-83** raw, with the accessors at L79-80 and the atomic at L107.
**There is no `/** The codec…` block above it** — the other five all carry one. That matters for the
verify, not for the edit: **strip comments before counting anyway**, so the same probe works on all
six, and so a reader comparing this task's numbers with another's is comparing like with like. Locate
by the tokens `languageCode` / `languageIndex`, never by the line numbers. Widen to three-way, pure
ASCII, in place. Persistence is a ValueTree `getProperty("uiLanguage")`; no change. No language
`AudioParameterChoice`.

**4. THE SECOND UNSCANNED `preset-manager.js`, AND ON THIS PLUGIN THE SCANNED/UNSCANNED PAIR SITS ONE DIRECTORY APART (K8).**

`check-i18n`'s `[12]` reports **`2 module(s): js/app.js, js/parameter-bindings.js`** and passes.
`Source/ui/public/modules/preset-manager.js` (406 L) is **not** in that list. **`js/parameter-bindings.js`
is scanned and `modules/preset-manager.js` is not, on the same plugin — so the discriminator is
demonstrably the DIRECTORY and not the file**, and this plugin is where that is provable in one
reading.

**Its untranslated strings are mostly DEAD here, and that is measured, not assumed.** This plugin
takes the **explicit-DOM-refs** path (`app.js:472-473` reads
`prevButton: document.getElementById('preset-prev')`), **not** the module's `createPresetBar()`
`innerHTML` block at `:378`. So the five strings in that block — `"Previous preset"`, `"Next preset"`,
`"Load preset from file"`, `"Save preset"`, `Default` — never reach this page. The markup keys the
buttons itself: `index.html:1221` carries `data-i18n-aria="aria.presetPrev"`.

**What DOES reach the page is `displayElement.textContent = this.currentPreset` at `:346`**, writing
`'Default'` or `'Loaded Preset'` into the preset-name element. **Measure whether that element is
keyed on this plugin and record the verdict**, then leave the module alone.

**Report it; do not fix it.** `preset-manager.js` has **eighteen consumers** (s71 D2) and the two
copies in this wave already **diverge** (O-Orbit's is 447 L, this one is 406 L, different sha). Keying
it is a rollout of its own. Take no invented gate. **`git status --short -- .../modules/` must be
empty at the end of this task.**

**THE FONT WORK — the tail half, and this is the wave's only FOUR-stack page.** Twelve declaration
sites, **5** of them `inherit`, and **two** distinct real stacks:

```
font-family: 'Garamond', 'Times New Roman', serif      <- TNR installed -> Latin-safe
font-family: 'Georgia', serif                          <- Georgia installed -> Latin-safe
```

**Both are Latin-safe** (K11) and both take the **tail only**: `'PingFang SC', 'Microsoft YaHei'`
before the trailing `serif` (W1). **This plugin defines no `--symbol-font` token** — the only one of
the six without it — so N3 has no subject here; record that as measured. **All twelve sites are inline
in `index.html`**; there is no `css/` directory, so **use `find` over the UI root rather than an
`ls <root>/css/*.css` glob**, which aborts the whole command when it matches nothing.

**THE PIN SURFACE — the wave's SMALLEST, and R8 is still live (R8, F7).** Comment-stripped:
**4 `min-width`, 1 `min-height`, 6 `line-height`, 14 `letter-spacing`, 3 `white-space`.** Grep every
selector before appending. **Remove a floor entirely before measuring the box it floors** (F7). **And
read F6 before deriving any `line-height` ratio for a form control** — M8's table is LEAF-scoped, a
`<button>` at 10px has a 13px content box (ratio 1.3) because the UA `font` shorthand resets
`line-height` on form controls. **This page is dense with buttons; derive form-control ratios from the
element's own box, never from the table.**

**THREE CLICK STATES, AND THE FIRST ONE IS A DROPDOWN (K16, N1).** State 1 opens the preset dropdown
(`.preset-dropdown-menu` is `display: none` until `#preset-name-display` is clicked), state 2 opens
the settings popover, state 3 turns hover help ON. Fire the state-EFFECT assertion on all three;
discriminate a `null` rect as a scrolled target rather than a coverage hole; **check the ORDER**,
because a scrolling click that is not the last state scrolls every state after it, and this page is
1776 lines of markup at a 690 px frame height.

**THE STALE BODY — the pair clause, en and fr, 2 hits, and its exception list names no number (K13).**
English: *"English and French are available; value readouts, note divisions and preset names stay in
English."* **Delete the pair clause only.** The exception list names **note divisions** and **preset
names** rather than a count, so the numeric re-verification the other tasks run has no subject here —
instead **verify the two named classes are still true**: note divisions stay English (check the
division dropdown's option words against `I18N_EXEMPT`, W3) and preset names stay English (K8's
`#preset-name` finding is exactly this claim's subject). Correct the comment as prose without spelling
the superseded literal (C8).

**This is the wave's ONLY plugin with a `#tips-toggle` in its markup** — 2 mentions, 6
`settings-popover` mentions. **That does NOT make the "and nothing else" class live**, because
`tip.settings` does not exist here either (grep count 0) and no body makes the exclusivity claim.
**Read the entry list, confirm the absence, and record the checked non-defect citing both greps** —
this is the one plugin where the toggle exists and the false claim does not, and the pairing is worth
stating so a later reader does not infer one from the other.

**THE BLIND DISPATCH.** 133 rows — above ~92, so **two chunks minimum, split on the CONCEPT** (F17),
fresh salt per chunk, `--emit O-Polystutter --plugin O-Polystutter` (BOTH — R5),
`--forward-provenance "<string>"` (F9), explicit `--manifest`, `--verbose`, `--allowed-tools ""` from
outside the repo, `CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` with the model read from stderr (F15).
**BOTH refusal controls against a properly provenanced emit** (F10). **ids identical AND IN ORDER**
(M12). Every triple read (R2). **Expect the product-name control to report a non-zero count** — the
markup carries a wordmark — and adjudicate it rather than treating the count as pass/fail.

Then: `TERMS` screen (R3 — **NONE** at planning time, **34 matched, the wave's largest**; re-fire, and
run the mechanical downstream check after authoring); body correction and **re-read** (C7); author at
`'mt'` (R1), SPACED form (M11), **mirroring the per-entry shape** (K3); endonym from O-Detune L1039 +
`I18N_EXEMPT`; ternary; font tail; geometry; commit at `'mt'`; blind read; promote **via a script
written to a FILE** (F14.5) at **1.15.0** (from the CMakeLists **1.14.3**, not the registry's stale
1.14.2); CHANGELOG; **`./scripts/build-and-install.sh O-Polystutter`** — pass the **FOLDER** name; the
script resolves the `OPolystutter` target itself — **in the background**. **No auval** — Task 7.

**Commit discipline.** Re-check branch and status immediately before every commit.
`git commit -- plugins/O-Polystutter` only; options before the `--`. **Run the submodule guard before
every commit.** Never stage `plugins/O-Strata/.planning/*` or `.claude/agent-memory/*`. Never tag.
Session trailer on every message.
  </action>

  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/check-i18n.js --plugin O-Polystutter >/tmp/qda-t5-ci.txt 2>&1; echo "exit=$?"; grep -E "\[1\] LANGUAGES|carries copy|tip\(s\) bound|module\(s\)" /tmp/qda-t5-ci.txt   # exit 0; THREE languages; 43 I18N + 48 LABELS unchanged; still 105 tips bound; still "2 module(s): js/app.js, js/parameter-bindings.js" — and modules/preset-manager.js is STILL not among them (K8)</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-Polystutter >/tmp/qda-t5-cul.txt 2>&1; echo "exit=$?"; grep -cE "GEOMETRY DIFF\]\[(fr|zh-Hans)\] no non-label element moved" /tmp/qda-t5-cul.txt; grep -E "elements were VISIBLE|never became visible" /tmp/qda-t5-cul.txt   # exit 0; green on BOTH non-English arms across all three states; coverage still 97 of 97 with 0 never-visible</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-Polystutter 2>&1 | tail -12   # 0 findings, 133 rows — NOT 134 — BELOW SHIP BAR 0</automated>
    <automated>node -e "import('file://'+process.cwd()+'/plugins/O-Polystutter/Source/ui/public/js/i18n.js').then(m=>{const noB=[],noZhB=[]; for(const [k,v] of Object.entries(m.I18N)){ if(!(v.en&&v.en.b&&v.en.b.length)) noB.push(k); const z=v['zh-Hans']; if(z&&!(z.b&&z.b.length)) noZhB.push(k);} console.log('en entries with no body: '+JSON.stringify(noB)); console.log('zh entries with no body: '+JSON.stringify(noZhB)); console.log(JSON.stringify(noB)===JSON.stringify(noZhB)?'SHAPE MIRRORED — correct':'SHAPE DIVERGED — a body was added or dropped, and the emitter row count moves (K3)');})"   # both lists read ["msg-delete-preset"] and the shapes MIRROR. Writing a zh body on that entry adds a 134th row the emitter did not ask for and breaks the wave's 820 -> 5441 arithmetic</automated>
    <automated>V=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' plugins/O-Polystutter/CMakeLists.txt); echo "resolved: ${V:-EMPTY-READER-IS-THE-DEFECT}"   # 1.15.0 — from the CMakeLists UNQUOTED 1.14.3, not the registry's stale 1.14.2 (K4). The two-arm reader is run even though this shape needs one arm (R9)</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-Polystutter/Source/PluginProcessor.h > /tmp/qda-t5-pp.h; grep -c 'zh-Hans' /tmp/qda-t5-pp.h; node -e "const s=require('fs').readFileSync('/tmp/qda-t5-pp.h','utf8'); for (const fn of ['languageCode','languageIndex']) { const m = s.match(new RegExp(fn + '[\\\\s\\\\S]{0,240}?;')); console.log(fn + ': ' + (m && /zh/.test(m[0]) ? 'THREE-WAY' : 'STILL TWO-WAY')); }"   # >= 2, THREE-WAY on both. This is the wave's ONLY codec with no doc comment above it — the strip runs anyway so the probe is identical across all six</automated>
    <automated>find plugins/O-Polystutter/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +; perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-Polystutter/Source/ui/public/js/i18n.js   # nothing, then the control MUST fire</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-Polystutter/Source/ui/public/js/i18n.js > /tmp/qda-t5-i18n.js; perl -CSD -0777 -ne '$n++ while /(English (and|or) (French|Fran)|anglais et le fran|and nothing else|ne contient que|two languages|deux langues|rien d.autre|rien d\x{2019}autre)/gi; END{print "stale-body hits: ",$n+0,"\n"}' /tmp/qda-t5-i18n.js; grep -c "'tip\.settings'" /tmp/qda-t5-i18n.js; grep -c "tips-toggle" plugins/O-Polystutter/Source/ui/public/index.html   # 0 hits, tip.settings 0, tips-toggle 2. THIS IS THE ONE PLUGIN WHERE THE TOGGLE EXISTS AND THE FALSE CLAIM DOES NOT — record the pairing so a later reader does not infer one from the other (K13)</automated>
    <automated>node -e "const langs=new Set(['en','fr','zh-Hans']); const s=require('fs').readFileSync('/tmp/qda-t5-i18n.js','utf8'); const seen=new Set(), dup=[]; for(const m of s.matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)){const k=m[1]; if(langs.has(k))continue; if(seen.has(k))dup.push(k); else seen.add(k);} console.log('duplicates '+(dup.length?dup.join(','):'NONE'))"   # NONE — F13-corrected form</automated>
    <automated>git status --short -- plugins/O-Polystutter/Source/ui/public/modules/ | wc -l | tr -d ' '; grep -n "createPresetBar\|getElementById('preset-prev')" plugins/O-Polystutter/Source/ui/public/js/app.js | head -3   # 0 lines changed — modules/preset-manager.js is REPORTED, not fixed (18 consumers, two already-divergent copies). And app.js takes the explicit-DOM-refs path, so the module's five innerHTML strings never reach this page — measured, not assumed</automated>
    <automated>grep -oE "font-family:[^;}]*" plugins/O-Polystutter/Source/ui/public/index.html | sort -u; grep -c "PingFang SC" plugins/O-Polystutter/Source/ui/public/index.html; find plugins/O-Polystutter/Source/ui/public -name '*.css' | wc -l | tr -d ' '   # both real stacks end '…, PingFang SC, Microsoft YaHei, serif' with the tail BEFORE the generic (W1); >= 1; and 0 CSS files — the wave's only fully-inline page, censused with `find` and never with an `ls <root>/css/*.css` glob that would abort the command</automated>
    <automated>grep -c "symbol-font" plugins/O-Polystutter/Source/ui/public/index.html   # 0 — this plugin defines NO --symbol-font token, the only one of the six. N3 has no subject here and that is recorded as measured, not skipped</automated>
    <automated>grep -c "&#31616;&#20307;&#20013;&#25991;" plugins/O-Polystutter/Source/ui/public/index.html   # 1 — endonym byte-copied from O-Detune index.html:1039</automated>
    <automated>node scripts/measure-ui.js --plugin O-Polystutter --mode box --report all >/tmp/qda-t5-mu.json 2>/tmp/qda-t5-mu.err; grep -E "undeclared-font|line-height-normal|wrap-count|svg-font-attr|identity|ESTIMATED" /tmp/qda-t5-mu.err   # undeclared-font 0 against a NON-EMPTY input, wrap-count 0 (baseline 0), identity ~1068 nodes — the wave's largest page. The ESTIMATED fraction was 154 of 164 at planning time; read F8 before treating any wrap-count delta as a defect</automated>
    <automated>node -e "const r=require('/tmp/qda-t5-mu.json'); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); const lh=z.filter(x=>x.lh==='normal'&&x.kids===0&&x.own&&x.own.trim()); const mv=lh.filter(x=>{const e=r.find(y=>y.key===x.key&&y.lang==='en'); return e&&Math.abs(e.h-x.h)>0.5}); console.log('zhHan='+z.length,'noCJKface='+z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff||'')).length,'lhResiduals='+lh.length,'ofWhichMOVED='+mv.length)"   # zhHan > 0, noCJKface 0, ofWhichMOVED 0 — residuals NAMED with enH == zhH (N1)</automated>
    <automated>node scripts/measure-ui.js --plugin O-Polystutter --mode box --verbose >/dev/null 2>/tmp/qda-t5-muv.err; grep -icE "did not resolve|unresolved|skipped state" /tmp/qda-t5-muv.err   # 0 — all THREE states applied, including the dropdown that is display:none until clicked</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-Polystutter --strict-tips 2>&1 | grep -iE "late|dead|clean"   # 0 late, 0 dead — across 105 bindings, the wave's densest map</automated>
    <automated>git status --short -- plugins/O-Orbit/libs/SAF | wc -l | tr -d ' '; git tag --points-at HEAD | wc -l | tr -d ' '; git status --short | grep -c "O-Strata"   # 0, 0, 2</automated>
    <human-check>`msg-delete-preset`'s missing body was confirmed BEFORE authoring and its shape was MIRRORED, not normalised. The emitter still reports 133 rows, not 134, and the SUMMARY states the number with its reason so the wave's 820 -> 5441 arithmetic holds.</human-check>
    <human-check>The 105-binding many-to-one TIP_BINDINGS map was read before any body was authored, and every body that serves more than one anchor was checked to read naturally on ALL of them. The R3 mechanical downstream check ran after authoring on the wave's largest glossary-matched set (34 captions), grouping zh renderings and reporting any two different keys sharing one, excluding same-control and same-English pairs.</human-check>
    <human-check>`modules/preset-manager.js` was measured and REPORTED, not fixed: the explicit-DOM-refs path at app.js:472-473 is confirmed, the five createPresetBar innerHTML strings are confirmed dead on this page, whether the #preset-name element is keyed is recorded, and the scope-out reason (18 consumers, two already-divergent copies, no calibrated gate) is written down. `js/parameter-bindings.js` being scanned while `modules/preset-manager.js` is not — on the same plugin — is recorded as the proof that the discriminator is the DIRECTORY.</human-check>
    <human-check>Every line-height ratio derived for a form control came from the element's OWN content box, never from M8's table, because the UA `font` shorthand resets line-height on form controls and M8's table is leaf-scoped (F6). This page is dense with buttons and the distinction was applied per element.</human-check>
    <human-check>Every one of the ~133 triples was read with --verbose, in chunks split on the CONCEPT (F17) with a fresh salt per chunk. BOTH refusal controls FIRED against a properly provenanced emit (F10); --forward-provenance was passed a real STRING (F9); ids identical AND IN ORDER (M12); the model recorded from stderr (F15). The product-name control's non-zero hits were adjudicated rather than treated as pass/fail.</human-check>
    <human-check>The pair clause was deleted in en and fr, and its exception list — which names classes rather than a count — was verified by CHECKING THE CLASSES: note-division option words still English and covered by I18N_EXEMPT (W3, both directions), and preset names still English. The comment was corrected as prose (C8). The tips-toggle-exists-but-no-false-claim pairing is recorded with both greps.</human-check>
    <human-check>The state-EFFECT assertion fired on all THREE click states including the preset dropdown, which is display:none until #preset-name-display is clicked. The state ORDER was checked — a scrolling click that is not the last state scrolls every state after it, and this is a 1776-line page at a 690 px frame height.</human-check>
  </verify>

  <done>
O-Polystutter ships three languages at `reviewed: 'bt'` on all **133** rows — not 134 — with
`msg-delete-preset`'s body-less shape mirrored and the reason stated. Version **1.15.0** read back
through the two-arm reader from an unquoted CMakeLists literal that was **1.14.3, not the 1.14.2 the
registry claimed**, CHANGELOG written, built and installed via `./scripts/build-and-install.sh
O-Polystutter` with the FOLDER name against the `OPolystutter` target. Geometry equal on all three
arms across all three states, with form-control line-height ratios derived from each element's own box
rather than M8's leaf-scoped table. Both its Latin-safe stacks carry the CJK tail inline in its
1776-line `index.html` — the wave's only fully-inline page, censused with `find`. Its C++ codec at the
`languageCode`/`languageIndex` tokens is three-way and grep-proved on both lines with comments
stripped even though it is the wave's only codec with no doc comment; zero Han under `Source/`,
positive control fired. Its 105-binding many-to-one tip map was read before authoring and its bodies
checked against every anchor they serve. Its `modules/preset-manager.js` is **measured, named and
reported unchanged**, with the scanned/unscanned pair one directory apart recorded as the proof that
`check-i18n`'s blindness is directory-scoped. Two or more path-scoped commits; the submodule guard ran
before each.
  </done>
</task>

<task type="auto">
  <name>Task 6: O-Orbit — the repo's ONLY submodule, the rollout's first plugin whose ENTIRE page renders through a stack naming no installed family, and nineteen structurally-unmeasurable labels (ZH4G-06, ZH4G-07, ZH4G-08, ZH4G-11)</name>
  <files>plugins/O-Orbit/Resources/ui/js/i18n.js, plugins/O-Orbit/Resources/ui/index.html, plugins/O-Orbit/Resources/ui/css/styles.css, plugins/O-Orbit/Source/PluginProcessor.h, plugins/O-Orbit/CMakeLists.txt, plugins/O-Orbit/CHANGELOG.md</files>

  <required_reading>
- This plan's **Live observations**, **K1-K16**, **Execution shape**, **Stage-4 executor rules**, **THE SHIP BAR** and **Commit discipline** — all above. **K10, K14 and the SUBMODULE GUARD are this task's centre of gravity.**
- **Tasks 1-5's SUMMARY records** — the method, the tail convention on Latin-safe stacks, the unscanned-module scope-out verdict. **This task is the one where the tail convention is NOT enough.**
- `.planning/quick/260907-ja8-.../deferred-items.md` — **F5** (`system_profiler` proves presence NEVER absence — this task's face verdict cannot rest on the table), **F8**, **F9**, **F10**, **F11**, **F12** (`undeclared-font` cannot see a collapsed `<select>`'s Han), **F13**, **F14**, **F15**, **F17**, **F18**; wave-4g instructions **2**, **4**, **8**, **9**, **10**, **11**.
- `.planning/quick/260906-s71-.../260906-s71-deferred-items.md` — **D2** and **D5** (the nine `<option>`s that are structurally unmeasurable — this plugin has nineteen).
- `CLAUDE.md` — **the submodule and path-scoped-commit sections in full**, plus the build/install cache sequence.
- `plugins/O-Orbit/CMakeLists.txt` **L1-30** — including `add_subdirectory(libs/SAF/framework saf_build)` at L12.
- `plugins/O-Orbit/Resources/ui/index.html` **L60-85** — the preset bar and the comment at L70 saying `#preset-name` must never become a `[data-i18n]` element.
  </required_reading>

  <precondition>Fire ALL of these on the tree as found, in the BACKGROUND, before any edit (M3). Measured at planning time; **re-fire rather than inherit**. **(a) is a HALT condition.**
(a) **`git submodule status` → ` b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af plugins/O-Orbit/libs/SAF (v1.3.4)` with a LEADING SPACE (clean), and `git status --short -- plugins/O-Orbit/libs/SAF` → EMPTY.** If either disagrees, **HALT and report** — do not begin work on a dirty submodule.
(b) `find plugins/O-Orbit/tests -name '*.js'` → **NOTHING**. Its `tests/` holds `i18n-states.json` and a `ui-stub/` directory containing a single `generic-overrides.json` and **no JavaScript at all** (K7).
(c) `node scripts/check-i18n.js --plugin O-Orbit` → **exit 0**, `[O-Orbit (Resources/ui)]`, `got ["en","fr"]`, `34 I18N + 57 LABELS`, `[2] 34 tip(s) bound`, `[12] 1 module(s): js/app.js`.
(d) `node scripts/i18n-zh-lint.js --plugin O-Orbit` → **exit 0**; `--self-test` → **10/10**.
(e) **`node scripts/check-ui-labels.js --plugin O-Orbit` → exit 0, coverage `37 of 56` with `19 never became visible` — the largest never-visible count in the rollout, and the gate PRINTS ONLY THE FIRST TWELVE.** Read the count, not the list (K14, R2).
(f) `measure-ui --mode box --report all` → identity **360 / 180 / 88** — the wave's smallest page; `wrap-count 0`; **`59 leaves compared, 56 ESTIMATED`** — a 95% estimated fraction, the wave's highest, so read F8 before treating any post-work `wrap-count` delta as a defect. Han-gated screens a **VACUUM** (K12).
(g) Two-arm reader → **`1.2.3`** (UNQUOTED literal, L20). One-arm also returns it — **fire both** (R9).
(h) **The font census: 14 `font-family` sites, ZERO `font-family: inherit`, and exactly ONE distinct stack — `Garamond, 'EB Garamond', serif`.** Against the installed table, **Garamond 0 and EB Garamond 0** — no member of that stack exists on this machine (K10, K11).
(i) `find plugins/O-Orbit/Resources/ui -name '*.js' -not -path '*/juce/*'` → **three** files including `js/modules/preset-manager.js`, which `check-i18n` does **not** scan, and whose path is spelled `js/modules/` here where O-Polystutter's is `modules/` (K8).
(j) `git branch --show-current` → `main`; one worktree; 0 tags at HEAD; `git status --short | grep -c "O-Strata"` → 2.
(k) Zero Han in `Source/**/*.{h,cpp}`, positive control firing on `Resources/ui/js/i18n.js`.</precondition>

  <reversibility rating="costly">This is the only task whose plugin contains a git submodule. A commit that stages a path under `plugins/O-Orbit/libs/SAF` changes the recorded submodule pointer, which a path-scoped `git restore` does not cleanly undo. **The guard makes it reversible; without the guard it is not.** Absent a submodule stage, revertible with `git restore --source=&lt;sha&gt; -- plugins/O-Orbit`.</reversibility>

  <action>
**125 rows — the wave's smallest table. 3 states, all clicks. UI root is `Resources/ui/` — the wave's
ONLY `[R]` plugin — with an external `css/styles.css` (819 L) beside a 418-line `index.html`.
800 x 600 parsed frame, the wave's smallest. `PLUGIN_CODE OuOr`, `IS_SYNTH FALSE` — an EFFECT,
`juce_add_plugin` target `OuariconOrbit` which is NOT the folder name, 34 I18N + 57 LABELS, 34 tips
bound, `I18N_EXEMPT` mentioned 3 times.** Run **the full ship bar** as Task 1 did. Four things are
specific to this plugin and **the first is a hard constraint, not a finding.**

**1. THE SUBMODULE. `plugins/O-Orbit/libs/SAF` IS THE REPO'S ONLY ONE, AND THE PATHSPEC INCLUDES IT.**

**`git commit -- plugins/O-Orbit` INCLUDES `plugins/O-Orbit/libs/SAF`.** Path-scoping is not the
guard; path-scoping is what makes the guard necessary. **Run this before EVERY commit in this task,
and abort rather than force it through:**

```bash
SUBMODULE_PATHS="plugins/O-Orbit/libs/SAF"
for sp in $SUBMODULE_PATHS; do
  if git diff --cached --name-only | grep -q "^$sp"; then
    echo "ABORT: a path under the submodule $sp is staged"; exit 1
  fi
done
git submodule status   # must still read " b6fe1882…  plugins/O-Orbit/libs/SAF (v1.3.4)" with a LEADING SPACE
```

**No path under it is ever staged, in any commit, for any reason.** Verified at planning time and
worth restating because it removes one class of worry: **no build step writes into the submodule.**
`plugins/O-Orbit/CMakeLists.txt:12` reads `add_subdirectory(libs/SAF/framework saf_build)` — the
second argument is the **binary** directory, so under the out-of-source `build/` tree the SAF build
artefacts land in `build/.../saf_build` and never inside `libs/SAF`. **Re-confirm `git submodule
status` after the build**, because that is a claim about CMake's behaviour and this task is where it
gets its first localization-wave test.

**2. THE ENTIRE PAGE RENDERS THROUGH A STACK THAT NAMES NO INSTALLED FAMILY (K10, K11, F5).**

This is the largest single font surface in the rollout and it is nothing like the other five.
**Fourteen `font-family` declaration sites, ZERO `font-family: inherit`, one distinct stack:**

```css
font-family: Garamond, 'EB Garamond', serif;
```

**Garamond 0. EB Garamond 0.** The only survivor is the bare `serif`, and Chromium resolves a bare
generic against the document's `lang`. Under `lang="zh-Hans"` that is a Chinese face — **for the Latin
text too.** So on this plugin the Latin arm does not merely risk moving when Chinese lands: **every
ASCII node on the page changes typeface.** That is a direct hit on the wave's must-have that no ASCII
text changes face across a language switch, and on this plugin it is **FALSE on the tree as found**
and must be **made** true.

The other five are the controlled contrast and their stacks are safe because of a family listed
**fourth** (`'Times New Roman'`) or **first** (`'Georgia'`). **This one has no such member.** A grep
for a bare generic finds nothing here, because the declaration names families — it just names ones
that do not exist. **The census is the instrument, not the grep** (wave 4c C4).

**The repair is the face-naming half applied to the whole page:** name an installed serif face
**first**, so the en and fr arms get a stable Latin metric, **then** the CJK tail, **then** the
generic. Keep `Garamond` and `'EB Garamond'` ahead of it — they are the Windows/print intent and
removing them is a rendering change on machines this repo does not build on. **Measure the en and fr
arms before and after**: naming a face where a generic was resolving **will** change Latin metrics on
this machine, and R7's equality assertion is against the post-change baseline, not the pre-change one.
**Say explicitly in the SUMMARY what the en and fr arms did**, because "the Latin arms do not move"
is a prediction this task is exposed on and it may be false here.

**And do NOT rest the face verdict on the installed-family table.** `system_profiler` proves presence,
never absence (F5) — it reports 0 for `Times` on a machine where `/System/Library/Fonts/Times.ttc`
exists and Chromium resolves to it. **Probe the resolved face through CDP
`CSS.getPlatformFontsForNode`** on a representative Latin node and a representative Han node, on all
three language arms, and record what the browser actually chose. That is the only reading that
settles this plugin.

**3. NINETEEN NEVER-VISIBLE LABELS, EVERY ONE STRUCTURALLY UNMEASURABLE (K14, N8, D4, F12, R2).**

```
37 of 56 [data-i18n] elements were VISIBLE in at least one state
19 never became visible and were therefore NEVER MEASURED
```

The largest count in the rollout — wave 4f's four consumers reported 10 each. The twelve the gate
prints are all `<option>` elements inside collapsed `<select>`s: `#layout-select`, `#path` (five),
`#tempo_sync` (four), `#speaker_layout` (two). **The gate truncates at twelve** — the same shape as
`--ingest`'s default twelve (R2) — so **read the count, not the list**, and do not treat the seven it
did not print as absent. This plugin has **seven** `<select>` elements.

**These are structurally unmeasurable by any DOM geometry probe, not merely unmeasured** (N8/D4).
**Record them as such rather than chasing them**, and do not add states to force them visible — an
`<option>` in a collapsed `<select>` has no box in any state.

**And `undeclared-font` cannot see their Han either** (F12): `measure-ui`'s `han` is the node's own
text plus `data-tip`, `data-tip-title` and `aria-label`, and a collapsed `<select>`'s selected-option
text is none of those. So `#layout-select`, `#path`, `#tempo_sync` and `#speaker_layout` will read
`han: false` however much Chinese they paint. **They get the CJK tail anyway**, on a decision about
what the control paints rather than on what the screen reports. **The screen is not the oracle for
this, and the SUMMARY must say so** rather than citing a zero as evidence.

**4. THE THIRD UNSCANNED `preset-manager.js`, AT A THIRD SPELLING OF THE PATH (K8).**

`Resources/ui/js/modules/preset-manager.js` (447 L) is **not** scanned — `check-i18n`'s `[12]`
reports `1 module(s): js/app.js`. Note the path: **`js/modules/` here, `modules/` on O-Polystutter,
O-simpleGrain and O-simpleSampler.** A census keyed on one spelling is blind to the other; use `find`
over the whole UI root.

**This copy DIVERGES from O-Polystutter's** — 447 L against 406 L, different sha256. **Confirm the
divergence rather than assuming either copy is canonical**, and record it: a shared module with
forked copies is a different (and worse) problem from a shared module with identical ones, and s71 D2
recorded eighteen consumers without recording that they had drifted.

**Its strings are mostly DEAD here, and that is measured.** `app.js:1157` carries the comment
*"Constructor + explicit DOM refs (never createPresetBar(), which…"* and `:1192` reads
`const els = ['preset-prev', 'preset-next', 'preset-name', …]`. So the module's `createPresetBar()`
`innerHTML` block at `:419-427`, with its unkeyed `title="Previous preset"` / `"Next preset"` /
`"Load preset from file"` / `"Save preset"` attributes, **never runs on this page**. The markup keys
the buttons itself at `index.html:75-76` — `data-i18n-aria="preset-prev"` and
`data-i18n-aria="preset-next"`, **both present in this plugin's LABELS** (verified at planning time).

**What DOES reach the page is `#preset-name`**, at `index.html:81`, showing `Default` and written at
runtime by `displayElement.textContent = this.currentPreset` (`'Default'` or `'Loaded Preset'`). And
**the markup comment at L70 says it "is NOT a `[data-i18n]` element and must never become one"** —
a deliberate decision by an earlier author about a live region. **Read that comment, honour it, and
record the verdict**: the preset name stays English by design, and the language body's exception list
already says preset names stay in English (K13).

**Report the module; do not fix it.** Take no invented gate. **`git status --short -- .../js/modules/`
must be empty at the end of this task.**

**THE PIN SURFACE (R8, F7).** Comment-stripped across `index.html` + `css/styles.css`: **10
`min-width`, 2 `min-height`, 3 `line-height`, 16 `letter-spacing`, 4 `white-space`.** Ten `min-width`
floors is the wave's second-largest such surface, and **every one of them was set in the French era
against a Latin metric that this task is about to change** by naming a face where a generic was
resolving. **Grep every selector before appending, and remove each floor entirely before measuring the
box it floors** (F7) — `min-width: 0` is not "no floor"; the flex-item default is `auto`, so `0`
explicitly permits shrinking BELOW content and reads a false natural box.

**THREE CLICK STATES (K16, N1).** `#gear-btn` (settings popover), `#help-toggle` (hover help ON,
popover still open), and `#view-toggle` — the SPEAKER LAYOUT VIEW, whose own state name records that
**15 of this plugin's labels live behind it and are invisible at rest**. Fire the state-EFFECT
assertion on all three; discriminate a `null` rect as a scrolled target rather than a coverage hole;
check the ORDER.

**THE STALE BODY — the pair clause, en and fr, 2 hits (K13).** English: *"English and French are
available; value readouts and preset names stay in English."* **Delete the pair clause only.** The
exception list names **preset names**, which is exactly the `#preset-name` claim above — **verify it
is still true** (it is, by the L70 design decision) and keep it. Correct the comment as prose without
spelling the superseded literal (C8). Record the two DEAD classes as checked non-defects:
`tip.settings` count **0**, no `#tips-toggle` in markup, not a `scala-tuning-engine` consumer.

**THE C++ CODEC.** `Source/PluginProcessor.h` **L103-104** raw, doc comment at L100-102, atomic at
L98 — locate by the tokens `languageCode` / `languageIndex`. Widen to three-way, pure ASCII, in
place. Persistence is a ValueTree `getProperty("uiLanguage")`; no change. No language
`AudioParameterChoice`. **Zero Han anywhere under `Source/`** — and note that `Source/` here does NOT
include the UI, which lives under `Resources/`, so the negative grep's scope is smaller on this plugin
than on the other five. **State the scope you grepped.**

**THE BLIND DISPATCH.** 125 rows — above ~92, so **two chunks minimum, split on the CONCEPT** (F17),
fresh salt per chunk, `--emit O-Orbit --plugin O-Orbit` (BOTH — R5), `--forward-provenance "<string>"`
(F9), explicit `--manifest`, `--verbose`, `--allowed-tools ""` from outside the repo,
`CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` with the model read from stderr (F15). **BOTH refusal
controls against a properly provenanced emit** (F10). **ids identical AND IN ORDER** (M12). Every
triple read (R2).

Then: `TERMS` screen (R3 — **NONE** at planning time, 28 matched; re-fire); body correction and
**re-read** (C7); author at `'mt'` (R1), SPACED form (M11); endonym from O-Detune L1039 +
`I18N_EXEMPT`; ternary; **the whole-page face-naming plus tail**; geometry; commit at `'mt'`; blind
read; promote **via a script written to a FILE** (F14.5) at **1.3.0** (from the CMakeLists **1.2.3**,
not the registry's stale 1.2.2); CHANGELOG; **`./scripts/build-and-install.sh O-Orbit`** — pass the
**FOLDER** name; the script resolves the `OuariconOrbit` target itself — **in the background**. **No
auval** — Task 7.

**Commit discipline.** Re-check branch and status immediately before every commit.
`git commit -- plugins/O-Orbit` only; options before the `--`. **RUN THE SUBMODULE GUARD BEFORE EVERY
COMMIT** and abort if it trips. Never stage `plugins/O-Strata/.planning/*` or
`.claude/agent-memory/*`. Never tag. Session trailer on every message.
  </action>

  <verify>
    <automated>git submodule status; git status --short -- plugins/O-Orbit/libs/SAF | wc -l | tr -d ' '   # " b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af plugins/O-Orbit/libs/SAF (v1.3.4)" with a LEADING SPACE, and 0 lines. The submodule pointer is UNMOVED and its tree is CLEAN — after the build as well as before it</automated>
    <automated>git log --format=%H -3 -- plugins/O-Orbit | while read c; do echo -n "$c: staged-under-SAF="; git show --name-only --format= $c | grep -c "^plugins/O-Orbit/libs/SAF" ; done   # 0 on EVERY commit this task made. `git commit -- plugins/O-Orbit` INCLUDES the submodule path, so a zero here is the guard working and not path-scoping working</automated>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/check-i18n.js --plugin O-Orbit >/tmp/qda-t6-ci.txt 2>&1; echo "exit=$?"; grep -E "\[1\] LANGUAGES|carries copy|tip\(s\) bound|module\(s\)" /tmp/qda-t6-ci.txt   # exit 0; THREE languages; 34 I18N + 57 LABELS unchanged; 34 tips; still "1 module(s): js/app.js" — js/modules/preset-manager.js is STILL not among them (K8)</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-Orbit >/tmp/qda-t6-cul.txt 2>&1; echo "exit=$?"; grep -cE "GEOMETRY DIFF\]\[(fr|zh-Hans)\] no non-label element moved" /tmp/qda-t6-cul.txt; grep -E "elements were VISIBLE|never became visible" /tmp/qda-t6-cul.txt   # exit 0; green on BOTH non-English arms across all three states; coverage still 37 of 56 with 19 never-visible. The 19 are structurally unmeasurable &lt;option&gt;s in collapsed &lt;select&gt;s and were NOT chased (K14, N8). The gate prints only 12 of them — read the COUNT (R2)</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-Orbit 2>&1 | tail -12   # 0 findings, 125 rows, BELOW SHIP BAR 0</automated>
    <automated>V=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' plugins/O-Orbit/CMakeLists.txt); echo "resolved: ${V:-EMPTY-READER-IS-THE-DEFECT}"   # 1.3.0 — from the CMakeLists UNQUOTED 1.2.3, not the registry's stale 1.2.2 (K4)</automated>
    <automated>cat plugins/O-Orbit/Resources/ui/index.html plugins/O-Orbit/Resources/ui/css/styles.css > /tmp/qda-t6-all.txt; perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' /tmp/qda-t6-all.txt > /tmp/qda-t6-strip.txt; grep -oE "font-family:[^;}]*" /tmp/qda-t6-strip.txt | sort -u   # EVERY stack now names an INSTALLED serif face BEFORE the CJK tail and BEFORE the generic, with Garamond and 'EB Garamond' kept ahead of it as the Windows/print intent. NO stack may end '…, serif' with no installed member before it — that is the defect this task exists to fix (K10)</automated>
    <automated>node -e "const s=require('fs').readFileSync('/tmp/qda-t6-strip.txt','utf8'); const installed=/Georgia|Times New Roman|Times|Helvetica|Arial|Menlo|Courier New/; let bad=[]; for(const m of s.matchAll(/font-family:\s*([^;}]+)/g)){const st=m[1].trim(); if(/inherit|var\(/.test(st)) continue; const members=st.split(',').map(x=>x.trim().replace(/^['\"]|['\"]$/g,'')); const generics=['serif','sans-serif','monospace','cursive','fantasy','system-ui']; const named=members.filter(x=>!generics.includes(x)); if(!named.some(x=>installed.test(x))) bad.push(st);} console.log(bad.length? 'STACKS WITH NO INSTALLED MEMBER: '+JSON.stringify(bad) : 'every declared stack reaches an installed face — 0');"   # 0. At planning time this reported the single stack `Garamond, 'EB Garamond', serif`, whose only survivor was a bare generic that Chromium rebinds to a Chinese face under lang=zh-Hans — so the Latin arm changed typeface with the language (K10)</automated>
    <automated>grep -c "PingFang SC" /tmp/qda-t6-all.txt   # >= 1 — the CJK tail landed, BEFORE the trailing generic and AFTER the named Latin face (W1)</automated>
    <automated>grep -c "&#31616;&#20307;&#20013;&#25991;" plugins/O-Orbit/Resources/ui/index.html   # 1 — endonym byte-copied from O-Detune index.html:1039</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-Orbit/Source/PluginProcessor.h > /tmp/qda-t6-pp.h; grep -c 'zh-Hans' /tmp/qda-t6-pp.h; node -e "const s=require('fs').readFileSync('/tmp/qda-t6-pp.h','utf8'); for (const fn of ['languageCode','languageIndex']) { const m = s.match(new RegExp(fn + '[\\\\s\\\\S]{0,240}?;')); console.log(fn + ': ' + (m && /zh/.test(m[0]) ? 'THREE-WAY' : 'STILL TWO-WAY')); }"   # >= 2, THREE-WAY on both</automated>
    <automated>find plugins/O-Orbit/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +; perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-Orbit/Resources/ui/js/i18n.js   # nothing, then the control MUST fire. NOTE the scope: Source/ here does NOT contain the UI, which lives under Resources/ — state the scope grepped</automated>
    <automated>perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/O-Orbit/Resources/ui/js/i18n.js > /tmp/qda-t6-i18n.js; perl -CSD -0777 -ne '$n++ while /(English (and|or) (French|Fran)|anglais et le fran|and nothing else|ne contient que|two languages|deux langues|rien d.autre|rien d\x{2019}autre)/gi; END{print "stale-body hits: ",$n+0,"\n"}' /tmp/qda-t6-i18n.js; grep -c "'tip\.settings'" /tmp/qda-t6-i18n.js   # 0 and 0 (K13, F11)</automated>
    <automated>node -e "const langs=new Set(['en','fr','zh-Hans']); const s=require('fs').readFileSync('/tmp/qda-t6-i18n.js','utf8'); const seen=new Set(), dup=[]; for(const m of s.matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)){const k=m[1]; if(langs.has(k))continue; if(seen.has(k))dup.push(k); else seen.add(k);} console.log('duplicates '+(dup.length?dup.join(','):'NONE'))"   # NONE — F13-corrected form</automated>
    <automated>git status --short -- plugins/O-Orbit/Resources/ui/js/modules/ | wc -l | tr -d ' '; shasum plugins/O-Orbit/Resources/ui/js/modules/preset-manager.js plugins/O-Polystutter/Source/ui/public/modules/preset-manager.js   # 0 lines changed, and the two sums DIFFER — the shared module has FORKED copies (447 L vs 406 L). Recorded as a finding; s71 D2 named eighteen consumers without recording that they had drifted (K8)</automated>
    <automated>find plugins/O-Orbit/Resources/ui -name '*.js' -not -path '*/juce/*'   # THREE files, and the module sits at `js/modules/` here where O-Polystutter's sits at `modules/` — a census keyed on one spelling is blind to the other, which is why this is a `find` over the whole UI root (K8)</automated>
    <automated>grep -n "data-i18n-aria=\"preset-prev\"\|data-i18n-aria=\"preset-next\"" plugins/O-Orbit/Resources/ui/index.html; grep -c "'preset-prev'\|'preset-next'" /tmp/qda-t6-i18n.js   # both aria keys are in the MARKUP and both resolve in the TABLE — the preset buttons are keyed by this plugin, not by the module, so createPresetBar()'s unkeyed title attributes never reach this page (K8)</automated>
    <automated>node scripts/measure-ui.js --plugin O-Orbit --mode box --report all >/tmp/qda-t6-mu.json 2>/tmp/qda-t6-mu.err; grep -E "undeclared-font|line-height-normal|wrap-count|svg-font-attr|identity|ESTIMATED" /tmp/qda-t6-mu.err   # undeclared-font 0 against a NON-EMPTY input, wrap-count 0 (baseline 0), identity ~360 nodes. The ESTIMATED fraction was 56 of 59 — the wave's highest — so read F8 before treating any wrap-count delta as a defect</automated>
    <automated>node -e "const r=require('/tmp/qda-t6-mu.json'); const z=r.filter(x=>x.lang==='zh-Hans'&&x.vis&&x.han); const lh=z.filter(x=>x.lh==='normal'&&x.kids===0&&x.own&&x.own.trim()); const mv=lh.filter(x=>{const e=r.find(y=>y.key===x.key&&y.lang==='en'); return e&&Math.abs(e.h-x.h)>0.5}); const gen=r.filter(x=>x.vis&&/^(serif|sans-serif|monospace)$/.test((x.ff||'').trim())); console.log('zhHan='+z.length,'noCJKface='+z.filter(x=>!/PingFang|YaHei|Songti/.test(x.ff||'')).length,'lhResiduals='+lh.length,'ofWhichMOVED='+mv.length,'nodesOnABareGeneric='+gen.length)"   # zhHan > 0, noCJKface 0, ofWhichMOVED 0, and nodesOnABareGeneric 0 across ALL THREE arms — the whole-page repair landed. Residuals NAMED with enH == zhH (N1)</automated>
    <automated>node -e "const r=require('/tmp/qda-t6-mu.json'); const byLang={}; for(const x of r){ if(!x.vis) continue; (byLang[x.lang]=byLang[x.lang]||[]).push(x); } for(const L of ['en','fr','zh-Hans']){ const s=new Set((byLang[L]||[]).map(x=>(x.ff||'').trim())); console.log(L+': '+s.size+' distinct computed stack(s) -> '+[...s].slice(0,4).join(' | ')); }"   # the SAME distinct computed stack on all three arms. A stack that differs between en and zh-Hans is Chromium rebinding a bare generic against the document lang, which is the defect this task fixes (K10)</automated>
    <automated>node scripts/measure-ui.js --plugin O-Orbit --mode box --verbose >/dev/null 2>/tmp/qda-t6-muv.err; grep -icE "did not resolve|unresolved|skipped state" /tmp/qda-t6-muv.err   # 0 — all THREE states applied, including #view-toggle, behind which 15 of this plugin's labels live</automated>
    <automated>node scripts/boot-all-uis.js --plugin O-Orbit --strict-tips 2>&1 | grep -iE "late|dead|clean"   # 0 late, 0 dead</automated>
    <automated>ls ~/Library/Audio/Plug-Ins/VST3/ ~/Library/Audio/Plug-Ins/Components/ | grep -i orbit   # O-Orbit-dev.vst3 and O-Orbit-dev.component, freshly rebuilt, with NO unsuffixed alternate-variant bundle beside either (CLAUDE.md)</automated>
    <automated>git tag --points-at HEAD | wc -l | tr -d ' '; git status --short | grep -c "O-Strata"   # 0, 2</automated>
    <human-check>The submodule guard ran before EVERY commit this task made, not once at the start, and `git submodule status` was re-confirmed AFTER the build as well as before it — because `add_subdirectory(libs/SAF/framework saf_build)` sending artefacts outside the submodule is a claim about CMake's behaviour and this task is its first localization-wave test. No commit stages a path under `plugins/O-Orbit/libs/SAF`, and the pointer is still b6fe1882 (v1.3.4).</human-check>
    <human-check>The resolved face was probed through CDP `CSS.getPlatformFontsForNode` on a representative Latin node and a representative Han node, on all three language arms, and what the browser actually chose is recorded. The verdict does NOT rest on the installed-family table, which proves presence and never absence (F5).</human-check>
    <human-check>What the en and fr arms DID when a face was named where a bare generic had been resolving is stated explicitly and measured, not predicted. "The Latin arms do not move" is a prediction this task is exposed on; whichever way it went, the numbers are in the SUMMARY and the geometry assertion is made against the post-change baseline.</human-check>
    <human-check>The nineteen never-visible labels were recorded as STRUCTURALLY UNMEASURABLE `<option>` elements in collapsed `<select>`s and were not chased, and no state was added to force them visible. The gate's truncation at twelve is noted so the seven it did not print are not read as absent (R2). The four collapsed selects were given the CJK tail on a decision about what the control PAINTS, with it stated that `undeclared-font` reads `han: false` on them and is not the oracle for that decision (F12).</human-check>
    <human-check>`js/modules/preset-manager.js` was measured and REPORTED, not fixed: the explicit-DOM-refs path is confirmed from app.js:1157/:1192, the createPresetBar innerHTML strings are confirmed dead, the `#preset-name` markup comment at index.html:70 was read and HONOURED, and the divergence from O-Polystutter's copy is confirmed by sha256 and recorded as a finding s71 D2 does not contain.</human-check>
    <human-check>Every one of the ~125 triples was read with --verbose, in chunks split on the CONCEPT (F17) with a fresh salt per chunk. BOTH refusal controls FIRED against a properly provenanced emit (F10); --forward-provenance was passed a real STRING (F9); ids identical AND IN ORDER (M12); the model recorded from stderr (F15). Each accepted drift carries a written reason.</human-check>
    <human-check>The pair clause was deleted in en and fr and its "preset names stay in English" exception was KEPT after being verified against the #preset-name design decision at index.html:70. The comment was corrected as prose (C8). Both dead classes recorded as checked non-defects with their greps. The ten French-era min-width floors were each removed entirely, measured, and restored or re-set (F7) — they were set against a Latin metric this task changed.</human-check>
  </verify>

  <done>
O-Orbit ships three languages at `reviewed: 'bt'` on all 125 rows, version **1.3.0** read back through
the two-arm reader from an unquoted CMakeLists literal that was **1.2.3, not the 1.2.2 the registry
claimed**, CHANGELOG written, built and installed via `./scripts/build-and-install.sh O-Orbit` with
the FOLDER name against the `OuariconOrbit` target. **The submodule is untouched**: `git submodule
status` still reads ` b6fe1882… (v1.3.4)` clean, after the build as well as before it, and no commit
this task made stages a path under `plugins/O-Orbit/libs/SAF` — proved per commit, not assumed from
path-scoping. **Its single house stack now names an installed serif face before the CJK tail and
before the generic**, so no node on the page resolves through a bare generic on any arm and the
computed stack is the same on en, fr and zh-Hans — with the resolved faces probed through CDP rather
than inferred from the installed-family table, and with what the Latin arms actually did stated as a
measurement. Geometry equal on all three arms across all three states, with all ten French-era
`min-width` floors removed, re-measured and re-set. Its C++ codec is three-way and grep-proved on both
lines; zero Han under `Source/`, positive control fired, and the grep's scope stated (the UI lives
under `Resources/`). Its nineteen never-visible labels are recorded as structurally unmeasurable and
not chased. Its `js/modules/preset-manager.js` is **measured, named and reported unchanged**, with the
fork from O-Polystutter's copy recorded as a finding s71 D2 does not contain. Two or more path-scoped
commits; the submodule guard ran before each.
  </done>
</task>

<task type="auto">
  <name>Task 7: Batch close-out — one cold auval sweep over six triples, one PLUGINS.md commit correcting six stale rows and one stale status cell, the dup-row check, the LIVE corpus recount asserting 43 of 43, and the false-prediction ledger (ZH4G-10, ZH4G-11, ZH4G-12)</name>
  <files>PLUGINS.md</files>

  <required_reading>
- This plan's **Live observations**, **K1-K16**, **Execution shape**, **Stage-4 executor rules**, **Commit discipline** and **Source coverage audit** — all above. **K1, K4 and K8 are this task's centre of gravity.**
- **Tasks 1-6's SUMMARY records in full** — this task's job is to reconcile them and state the wave's answers, not to re-derive them.
- `.planning/quick/260907-ja8-.../deferred-items.md` — **D1-D12 in full**, the **thirteen wave-4g instructions**, **F5**, **F15**, **F18**, and §"Explicitly for wave 4g"'s row table (`153 + 145 + 133 + 133 + 131 + 125 = 820`, `5441 − 4621 = 820`).
- `.planning/quick/260907-ja8-.../260907-ja8-SUMMARY.md` — **§9** the false-prediction ledger's shape, **§12** the corpus recount's shape. **This task reproduces both.**
- `.planning/quick/260906-h8y-.../deferred-items.md` — **D2** (the O-Bells boot census control), **D4** (the glossary divergence report, six entries), **D5** (the Z6 budget backfill), **D7** (the routing-label wrap — corrected in scope by K9), **M14**, **N11**.
- `.planning/quick/260906-s71-.../260906-s71-deferred-items.md` — **D2**, **D3**, **D11** (the corrected auval budget and the pre-existing warnings).
- `CLAUDE.md` — the AU cache-clearing sequence, the dual-variant sweep, **the PLUGINS.md union-merge duplicate check**, path-scoped commits, **never tag**, and the **phase handoff protocol**.
  </required_reading>

  <precondition>**Tasks 1-6 are committed and all six plugins are INSTALLED** — the cold auval sweep covers all six and cannot run until every bundle is on disk. Fire on the tree as found, in the BACKGROUND:
(a) `ls ~/Library/Audio/Plug-Ins/VST3/ ~/Library/Audio/Plug-Ins/Components/` → a `-dev` bundle in **both** formats for each of the six. **Measured at planning time: `O-simpleAdditive-dev`, `O-simpleGrain-dev`, `O-simpleSampler-dev`, `O-simpleSubtractive-dev`, `O-Polystutter-dev`, `O-Orbit-dev`, VST3 and Component alike — no unsuffixed alternate-variant orphan for any of the six, and no third-product-name orphan of the O-Contrabass kind (D9).**
(b) `node scripts/boot-all-uis.js --strict-tips` → **exit 0**, `clean: 43/43`, 0 warn, 0 failed, **0 DEAD across 0 plugins, exactly 2 late across 1 plugin** (O-Bells `#ref-pitch-knob`, `#octave-stretch`). **That pair is the census control (4e D2 / 4f D12): 2 before and 2 after is what makes the 0 DEAD beside it evidence rather than a number.** 3842 rendered text-bearing elements, aria-label 798, title 0 at planning time.
(c) `git status --short` → `M .claude/agent-memory/research-planning-agent.md` plus the two untracked `plugins/O-Strata/.planning/*` files, plus whatever Tasks 1-6 left uncommitted. `git branch --show-current` → `main`. `git worktree list` → one line. `git tag --points-at HEAD` → 0.
(d) `git submodule status` → ` b6fe1882…  plugins/O-Orbit/libs/SAF (v1.3.4)` with a **leading space**. **HALT if it moved.**
(e) Repo-wide `i18n-zh-lint` → **exit 2 is the BASELINE** (K2); the criterion is `0 finding(s) across 0 plugin(s)` and `1 plugin(s) could not be read`.
(f) `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d` → **empty** on the tree as found. Re-fire it now, before the edit, so the post-edit run has a baseline.</precondition>

  <reversibility rating="reversible">One PLUGINS.md commit. Revertible with `git restore --source=&lt;sha&gt; -- PLUGINS.md`.</reversibility>

  <action>
This task ships nothing new. It **closes the batch, closes the corpus, and states the wave's
answers**, and every item below is a deliverable the carry-forwards explicitly asked for.

**1. ONE COLD AUVAL SWEEP COVERING ALL SIX, IN THE BACKGROUND.**

Read each triple off `auval -a` — **never guess one**. Measured at planning time: **four instruments
and two effects** (`IS_SYNTH TRUE` on O-simpleAdditive, O-simpleGrain, O-simpleSampler and
O-simpleSubtractive; **FALSE** on O-Polystutter and O-Orbit), with plugin codes `OSiA`, `OsGr`,
`OsSm`, `OSiS`, `OuPs`, `OuOr` and the **dev** manufacturer code `OuDv` (the release code is CI-only).
**But read the rows** — the codes are a prediction and `auval -a` is the measurement.

**Pass `auval -v` THREE UNQUOTED WORDS** (M14): `auval -v aumu OsGr OuDv`, never
`auval -v "aumu OsGr OuDv"`, which returns `Invalid Arguments: auval` and no further output — across
six plugins that reads exactly like six validation failures.

**The timing budget is ~85 seconds for the cold rescan, not fifteen minutes** (wave 4g instruction 12;
measured 83-84 s in wave 4f and again in s71). Six `auval -v` runs together took ~1 s in 4f. Run it in
the background and poll a sentinel anyway.

**Two warnings are known PRE-EXISTING on OTHER plugins and must not be transplanted onto these six**
(D10): O-Bowed's `Bow Position` retained-max warning and O-Wind's channel-layout note. **Neither
belongs to any plugin in this wave.** If a warning appears on one of these six, it is **new
information** and must be reported as such — this wave touched **no parameter, range, type, state
format or audio path** on any of the six, so a parameter warning here would be a real finding.
`Bad Max Frames - Render should fail` appears in every log and is **the name of a test that expects a
failure**, not a finding.

**Before the sweep, re-confirm no alternate-variant orphan is on disk for any of the six.** All six
were `-dev` only, both formats, at planning time. A leftover unsuffixed bundle beside a `-dev` one
pins Logic's registry slot to whichever was installed first (CLAUDE.md's dev↔release shadowing
warning). Follow CLAUDE.md's cache sequence. **Record whether any build emitted a
`⚠ Sweeping ALTERNATE-variant` warning**, and note that `build-and-install.sh`'s Phase 4 sweep is keyed
on `<Name>` and `<Name>-dev` only and structurally cannot see a third product name (D9).

**2. `PLUGINS.md`, ONCE, IN ITS OWN COMMIT — SIX STALE ROWS AND ONE STALE STATUS CELL.**

Rows as found, measured at planning time. **Every single one is one patch behind its own
`CMakeLists.txt`, and all six from the same 2026-09-03 commit** — the suite-wide French hover-help
rename, task `260903-ukp`, which is also what left O-GrainScatter's row two minors behind in wave 4f
(K4):

| plugin | registry row | status cell | CMakeLists as found | **ships** |
|---|---|---|---|---|
| O-simpleAdditive | 1.2.0 | 📦 Installed | **1.2.1** | **1.3.0** |
| O-simpleGrain | 1.4.2 | 📦 Installed | **1.4.3** | **1.5.0** |
| **O-simpleSampler** | 1.4.3 | **✅ Working** | **1.4.4** | **1.5.0**, and the cell flips to **📦 Installed** |
| O-simpleSubtractive | 1.4.0 | 📦 Installed | **1.4.1** | **1.5.0** |
| O-Polystutter | 1.14.2 | 📦 Installed | **1.14.3** | **1.15.0** |
| O-Orbit | 1.2.2 | 📦 Installed | **1.2.3** | **1.3.0** |

**Write the shipped versions. Flip O-simpleSampler's status cell to `📦 Installed`.** And note in the
commit message that **all six rows disagreed with their own CMakeLists before this wave started**,
rather than letting six simultaneous corrections read as six typos. **This is the registry's second
consecutive whole-suite miss and it is now a pattern worth naming in the deferred items**, not a
one-off.

**O-simpleSampler's status cell was wrong about the machine, not merely stale:** its `-dev` bundles
were **already on disk** in both formats at planning time while the row said `✅ Working`. Record that
as a false prediction in the ledger (item 6).

Then run the duplicate check. **CLAUDE.md requires it after any change that touches PLUGINS.md**, and
the union merge driver duplicates ADJACENT rows, not only the edited one — and this wave edits **six
rows, four of which are adjacent in the file**:

```bash
grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d
```

**It must print nothing.** It printed nothing at planning time.

**3. THE LIVE CORPUS RECOUNT — AND IT IS THE WAVE'S HEADLINE (K1).**

**Recompute it LIVE from the emitter. Do not carry a number forward.**

```bash
node scripts/i18n-zh-backtranslate.js 2>/dev/null | tail -30
```

**The assertion is `5441 of 5441 rows` and `43 of 43 plugins`, and it needs BOTH denominators stated
because they are different questions (K1):**

- **`check-i18n` repo-wide has read `ALL CHECKS PASS — 43 localized plugin(s)` on every wave since
  Stage 3, including before this one.** Its 43 counts plugins that have an `i18n.js` at all. **Quoting
  it alone proves nothing about this wave**, because it was already 43.
- **The emitter's count is the one that moves: 37 → 43.** It counts plugins carrying at least one zh
  row. **Before this wave the two disagreed by exactly six — this wave's set. After it they must be
  the SAME 43.** State both readings side by side and state that their convergence is the closure.
- **Rows: 4621 → 5441 of 5441.** Verify the arithmetic per plugin against the emitter's own `rows`
  column: **simpleGrain 153, simpleSampler 145, Polystutter 133, simpleSubtractive 133, simpleAdditive
  131, Orbit 125 = 820**, and `5441 − 4621 = 820` exactly. **Every one of those six figures was
  verified live off the emitter at planning time and all six matched the carry-forward.** Re-verify
  them; a per-plugin figure that moved means a row was added or dropped — **and O-Polystutter is the
  one to watch, because `msg-delete-preset` has no body and writing one would make it 134** (K3).
- **`BELOW SHIP BAR — at reviewed:'mt' or unflagged: 0.** Every one of the 5441 must be at `'bt'`.

**THE 44TH PLUGIN IS O-STRATA AND IT IS EXCLUDED BY NAME AND FOR A STATED REASON.** Its emitter row
reads `O-Strata  ERROR no i18n.js under either UI root`. It is at Stage 0, it has no `i18n.js` under
`Source/ui/public/` or `Resources/ui/`, and it is the standing cause of both repo-wide lints' exit 2
(D5/K2). **Say all of that explicitly. "43 of 44, one excluded" with no name is not the assertion the
wave was asked for.**

**AND IF ANY PLUGIN WITH AN `i18n.js` IS STILL TWO-LANGUAGE AFTER THIS WAVE, NAME IT AND DO NOT
DECLARE THE ROLLOUT DONE.** Fire this as its own check rather than inferring it from the totals:

```bash
for d in plugins/*/; do
  p=$(basename "$d")
  f=$(find "$d" -name i18n.js -not -path '*/libs/*' -not -path '*/build/*' 2>/dev/null | head -1)
  [ -z "$f" ] && continue
  grep -q "zh-Hans" "$f" || echo "STILL TWO-LANGUAGE: $p"
done
```

**Empty output is the only reading that permits "the rollout is done."**

**4. RE-DERIVE THE INSTALLED-FAMILY TABLE (M5, wave 4g instruction 4).** It is a property of the
machine and the bare-generic arithmetic turns on it. Measured at planning time and **unchanged from
waves 4d, 4e and 4f**: Georgia 4, Times New Roman 4, Arial 4, Menlo 4, Courier New 4, PingFang SC 6,
Songti SC 4, Helvetica Neue 14, Arial Unicode MS 1, Apple Symbols 1; **Garamond 0, EB Garamond 0,
Adobe Garamond Pro 0, Microsoft YaHei 0, Consolas 0, Segoe UI Symbol 0, Noto Sans Symbols2 0.**
Re-derive it at close-out and state whether it moved.

**And state the consequence, which is this wave's largest font finding (K10):** every one of the six
leads its house stack with **Garamond, which is absent**. Five survive on a Times New Roman or Georgia
listed later. **O-Orbit does not** — its single stack `Garamond, 'EB Garamond', serif` had no
installed member at all, so its entire page's Latin was resolving through a generic that Chromium
rebinds to a Chinese face under `lang="zh-Hans"`. **A zero in this table is a claim about the QUERY,
not about the machine** (F5), which is why Task 6's verdict was probed through CDP.

**5. THE N12 ENUMERATION-FORM CENSUS VERDICT — and this wave's answer is that it HAS NO SUBJECT.**

State it as a measured absence, not an omission:

- **Not one of the six owns a gate file.** `find plugins/<Name>/tests -name '*.js'` returns nothing on
  all six, fired with `find` and never with a glob, because a zsh glob that matches nothing aborts the
  whole command and reads as an answer. Their `tests/` directories hold `i18n-states.json`, four C++
  `render-harness/` trees and O-Orbit's `ui-stub/generic-overrides.json` — **no JavaScript anywhere**.
- **So no enumeration form was exercised, no form-6 or form-7 verdict is available from this wave, and
  no new form appeared.** Wave 4f proposed two candidates and left the verdict to be argued;
  **wave 4g contributes no evidence either way and must say so** rather than restating 4f's argument
  as if it had tested it.
- **No gate was invented** (wave 4c D4: a gate written mid-localization is a gate nobody has
  calibrated). `check-i18n`, `check-ui-labels`, `measure-ui` and `boot-all-uis` were the whole
  instrument.
- **Re-run the wider two-language census and report the remaining set.** It returned **13** files
  before wave 4f and **8** after (F2). **None of this wave's six is among them** — they own no gate
  files at all — so **the number should be unchanged at 8**, and every one of those eight belongs to a
  plugin that is already three-language. **Fire it and report the count**; if it moved, something
  outside this wave changed it.

  ```bash
  for f in $(find plugins/*/tests -name '*.js' 2>/dev/null); do
    perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' "$f" > /tmp/qda-gate-strip.js
    n=$(grep -cE "['\"]fr['\"]|\.fr\b" /tmp/qda-gate-strip.js)
    [ "$n" -gt 0 ] && echo "$f  $n"
  done
  ```

**6. THE FALSE-PREDICTION LEDGER (wave 4g instruction 10). The count is itself the finding.**

Wave 4d produced 3, wave 4e 14, s71 19, **wave 4f 41 (38 across five tasks plus 3 at batch level)**.
Collect this wave's from Tasks 1-6's SUMMARY records and state every one that was **false as stated**,
in the shape wave 4f's §9 used. **Six are already known false at planning time and are pre-seeded:**

- **All six `from` versions in the task description** — 1.2.0, 1.4.2, 1.4.3, 1.4.0, 1.14.2, 1.2.2
  against the CMakeLists' 1.2.1, 1.4.3, 1.4.4, 1.4.1, 1.14.3, 1.2.3 (K4). Every one is a faithful read
  of a stale registry row.
- **"O-simpleSampler … currently ✅ Working not 📦 Installed — install it."** The row said so; **both
  bundles were already on disk.** The row was wrong about the machine, not describing it.
- **Wave 4f instruction 5's report that the `set()`-variable CMake shape was absent** — it is present
  here, on O-simpleGrain, and **the one-arm reader returns EMPTY on it** (K5).
- **Wave 4e D7 / 4f D12's attribution of the `div.routing-label` French wrap to O-simpleFM** — it is a
  shared class with carriers in three trees and one of them is O-simpleSubtractive, in this wave's own
  set (K9).
- **The implicit assumption that these six carry no external stylesheet** (true of all five in wave
  4f) — **five of the six here have a `css/styles.css`**, and only O-Polystutter is fully inline.
- **Wave 4f's D5 precondition that `git status --short | grep -c "O-Strata"` reads 1** — it reads
  **2**, because the directory is now tracked and two planning files inside it are not (K2).

**The candidates this plan knows it is exposed on and which the executors must adjudicate:**

- **that naming a face on O-Orbit does not move the en and fr arms** (K10) — a prediction about a font
  swap on a page whose Latin was resolving through a generic, and it may well be wrong;
- **that `undeclared-font` reports 0 on all six once the tables land** — predicted from every stack
  being tail-repaired, but F12 says the screen cannot see a collapsed `<select>`'s Han at all, so a 0
  on O-Orbit is partly a blindness;
- **that O-simpleSubtractive's `wrap-count` stays at exactly 1** (K9);
- **that O-Polystutter's emitter count stays at 133 and not 134** (K3);
- **that the R3 screen's NONE holds after authoring** — it was fired on captions only, and M13's
  two-different-roots collision is invisible to it;
- **that the six PLUGINS.md rows are the only stale ones** — this is the second consecutive wave to
  find a whole-suite miss and nobody has audited the other 37;
- **every raw line number cited in K1-K16 and in the task actions.** They were read at planning time
  from an unedited tree; any file another session touches in the interim moves them. **Verify by
  pattern** (instruction 11).

**7. ANSWER EVERY INHERITED ITEM BY NUMBER.** The SUMMARY must contain a section that walks **4f
D1-D12**, **4e D4 (glossary divergence, six entries)**, **4e D5 / 4f D7 (Z6 budget, 549 unbudgeted)**,
**s71 D2 (unscanned module JS)** and **s71 D3 (stale `registry.yaml` `used_by`)** and gives each a
disposition. Three need a positive answer this wave was specifically asked for:

- **Did this wave add a SEVENTH glossary divergence?** R3 fired at planning time on all six and
  returned **NONE** (22 / 27 / 23 / 16 / 34 / 28 glossary-matched captions). Report the outcome after
  authoring, and report any `termNote` written with its corpus site count checked (D1's general
  lesson).
- **Did this wave add any Z6 budget?** `3 of 552 carry a measured budget; 549 are UNBUDGETED`, re-fired
  live. **Predicted none**, because every geometry finding is expected to be a shrink and a shrink
  wants a floor. Report the outcome either way.
- **s71 D2 is EXTENDED by this wave, not merely inherited (K8).** Report the four unscanned module JS
  files found on this wave's own plugins — `webview-drop-streaming.js` on O-simpleGrain and
  O-simpleSampler (byte-identical, ~12 `showToast` strings that render English on the Chinese page)
  and `preset-manager.js` on O-Polystutter and O-Orbit (**divergent copies**, strings mostly dead) —
  with the directory-scoped blindness stated and the scope-out reason written down. **And record the
  new fact s71 D2 does not contain: the copies have FORKED.**

**And state explicitly that 4f D4 (the 17-of-37 shared-module coverage hole) is NOT inherited**,
because none of the six is a `scala-tuning-engine` consumer — verified two ways on all six.

**8. THE REPO-WIDE REGRESSION SWEEP** — the whole suite, not just this wave. Every command in the
`<verify>` block below.

**9. WRITE THE SUMMARY AND `deferred-items.md`.** Both go beside this plan. **Do not commit them** —
the orchestrator does. See `<output>`.

**10. THE HANDOFF.** CLAUDE.md requires a two-step handoff at the end of a workflow stage: what was
completed, then **Step 1:** `/clear` and **Step 2:** the next command, then STOP. **Do not auto-invoke
anything.**

**Commit discipline.** Re-check `git branch --show-current` and `git status --short` **immediately
before** the PLUGINS.md commit, not once at the start of this task. `git commit -- PLUGINS.md` only;
never `git add -A`, never `git commit -a`; options before the `--` (F14.6). **Run the submodule guard
before the commit** — it is unconditional. **Never stage `plugins/O-Strata/.planning/*` or
`.claude/agent-memory/*`.** **Never create a tag** — tags belong to `/publish`. Push is not required.
Session trailer: `Claude-Session: https://claude.ai/code/session_01Ek97dmx4fvCBV9Wa9pB9go`
  </action>

  <verify>
    <automated>node scripts/check-i18n.js >/tmp/qda-t7-ci.txt 2>&1; echo "repo-wide check-i18n exit=$?"; tail -2 /tmp/qda-t7-ci.txt   # exit 0, "ALL CHECKS PASS — 43 localized plugin(s)". This number was ALREADY 43 before the wave (K1) — it counts plugins with an i18n.js, not plugins with zh rows. Quoting it alone proves nothing</automated>
    <automated>for p in O-simpleAdditive O-simpleGrain O-simpleSampler O-simpleSubtractive O-Polystutter O-Orbit; do node scripts/check-i18n.js --plugin $p >/dev/null 2>&1; echo "  $p check-i18n exit=$?"; done   # 0 each</automated>
    <automated>node scripts/i18n-zh-lint.js >/tmp/qda-t7-zl.txt 2>&1; echo "repo-wide zh-lint exit=$?"; grep -E "finding\(s\) across|could not be read|GATE|Z6 coverage" /tmp/qda-t7-zl.txt   # exit 2 is the BASELINE (K2/D5). The load-bearing line is "0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read" — a non-zero finding count, or an unreadable count other than 1, IS a failure. The Z6 line answers D7: report whether "549 are UNBUDGETED" moved</automated>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -2   # 10/10</automated>
    <automated>node scripts/i18n-fr-lint.js 2>&1 | grep -E "plugins with findings"   # 0 / 44, 1 could not be read — French untouched across the whole suite</automated>
    <automated>node scripts/i18n-zh-backtranslate.js 2>/dev/null | tail -8   # 5441 of 5441 rows, "BELOW SHIP BAR — at reviewed:'mt' or unflagged: 0", 1 plugin could not be read. RECOMPUTED LIVE, never carried forward</automated>
    <automated>for d in plugins/*/; do p=$(basename "$d"); f=$(find "$d" -name i18n.js -not -path '*/libs/*' -not -path '*/build/*' 2>/dev/null | head -1); [ -z "$f" ] && continue; grep -q "zh-Hans" "$f" || echo "STILL TWO-LANGUAGE: $p"; done   # EMPTY. This is the only reading that permits "the rollout is done". A named plugin here means the SUMMARY must name it and must NOT declare closure</automated>
    <automated>for d in plugins/*/; do p=$(basename "$d"); f=$(find "$d" -name i18n.js -not -path '*/libs/*' -not -path '*/build/*' 2>/dev/null | head -1); [ -z "$f" ] && echo "NO i18n.js: $p"; done   # exactly one line, and it must read O-Strata. The 44th plugin is excluded BY NAME and for a stated reason — Stage 0, no i18n.js under either UI root, the standing cause of both lints' exit 2 (D5/K1)</automated>
    <automated>node scripts/i18n-zh-backtranslate.js 2>/dev/null | grep -E "O-simpleAdditive|O-simpleGrain|O-simpleSampler|O-simpleSubtractive|O-Polystutter|O-Orbit"   # 131 / 153 / 145 / 133 / 133 / 125, every row fully at bt. Sum 820, and 5441 - 4621 = 820 exactly. A per-plugin figure that MOVED means a row was added or dropped — watch O-Polystutter, whose msg-delete-preset has no body and would read 134 if one were written (K3)</automated>
    <automated>grep -E "^\| (O-simpleAdditive|O-simpleGrain|O-simpleSampler|O-simpleSubtractive|O-Polystutter|O-Orbit) " PLUGINS.md   # 1.3.0 / 1.5.0 / 1.5.0 / 1.5.0 / 1.15.0 / 1.3.0, and O-simpleSampler's status cell now reads 📦 Installed. All six were ONE PATCH STALE against their own CMakeLists before this wave, all from the same 2026-09-03 commit (K4)</automated>
    <automated>for p in O-simpleAdditive O-simpleGrain O-simpleSampler O-simpleSubtractive O-Polystutter O-Orbit; do V=$(perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1";exit} if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1";exit}' plugins/$p/CMakeLists.txt); R=$(grep -oE "^\| $p \| [^|]+ \| [0-9]+\.[0-9]+\.[0-9]+" PLUGINS.md | grep -oE "[0-9]+\.[0-9]+\.[0-9]+$"); echo "  $p cmake=$V registry=$R $([ "$V" = "$R" ] && echo AGREE || echo DISAGREE)"; done   # AGREE on all six. This is the reconciliation the registry has now missed twice in a row across a whole suite — name the pattern in the deferred items, do not let six simultaneous corrections read as six typos</automated>
    <automated>grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d   # MUST print nothing. CLAUDE.md requires this after any change touching PLUGINS.md, and the union merge driver duplicates ADJACENT rows, not only edited ones — this wave edits six rows, four of them adjacent</automated>
    <automated>node scripts/boot-all-uis.js --strict-tips >/tmp/qda-t7-boot.txt 2>&1; echo "exit=$?"; grep -iE "clean:|DEAD bindings|late bindings|warn:|failed:|rendered text-bearing" /tmp/qda-t7-boot.txt   # exit 0, clean 43/43, 0 warn, 0 failed, 0 DEAD across 0 plugins, and EXACTLY 2 late across 1 plugin (O-Bells). 2 before and 2 after is what makes the 0 DEAD beside it evidence rather than a number (4e D2 / 4f D12)</automated>
    <automated>for p in O-simpleAdditive O-simpleGrain O-simpleSampler O-simpleSubtractive O-Polystutter O-Orbit; do node scripts/check-ui-labels.js --plugin $p >/tmp/qda-t7-cul-$p.txt 2>&1; echo "  $p exit=$? $(grep -c 'GEOMETRY DIFF\]\[zh-Hans\] no non-label element moved' /tmp/qda-t7-cul-$p.txt) zh-arm no-move assertions"; done   # exit 0 on all six with a non-zero assertion count each. Run in the BACKGROUND</automated>
    <automated>ls ~/Library/Audio/Plug-Ins/VST3/ ~/Library/Audio/Plug-Ins/Components/ | grep -iE "simpleAdditive|simpleGrain|simpleSampler|simpleSubtractive|Polystutter|Orbit" | sort   # twelve bundles — six VST3 and six Components — every one `-dev` suffixed, with NO unsuffixed alternate-variant beside any of them. A leftover unsuffixed bundle pins Logic's registry slot to whichever was installed first (CLAUDE.md)</automated>
    <automated>auval -a > /tmp/qda-t7-auval-a.txt 2>&1; echo "auval -a exit=$?"; grep -iE "simpleAdditive|simpleGrain|simpleSampler|simpleSubtractive|Polystutter|Orbit" /tmp/qda-t7-auval-a.txt   # SIX rows. READ THE TRIPLES OFF THIS OUTPUT — never guess one. Expect four instruments (aumu) and two effects (aufx), codes OSiA OsGr OsSm OSiS OuPs OuOr with the dev manufacturer OuDv. Budget ~85 s for the cold rescan (instruction 12), run in the BACKGROUND, poll a sentinel</automated>
    <automated>for T in "$(grep -i simpleAdditive /tmp/qda-t7-auval-a.txt | head -1)"; do echo "$T"; done   # then, per plugin, `auval -v <type> <subtype> <mfr>` with THREE UNQUOTED WORDS (M14) — never `auval -v "aumu OsGr OuDv"`, which prints `Invalid Arguments: auval` and across six plugins reads exactly like six validation failures</automated>
    <automated>grep -ciE "AU VALIDATION SUCCEEDED" /tmp/qda-t7-auval-v-all.txt   # 6 — one per plugin. D10's two known warnings belong to O-Bowed and O-Wind and to NEITHER of this wave's six; a parameter warning appearing here is NEW INFORMATION, because this wave touched no parameter, range, type, state format or audio path. `Bad Max Frames - Render should fail` is the NAME OF A TEST THAT EXPECTS A FAILURE, not a finding</automated>
    <automated>for f in Georgia "Times New Roman" Arial "PingFang SC" "Songti SC" Garamond "EB Garamond" "Microsoft YaHei"; do printf "  %-20s %s\n" "$f" "$(system_profiler SPFontsDataType 2>/dev/null | grep -c "Family: $f\$")"; done   # Georgia 4, Times New Roman 4, Arial 4, PingFang SC 6, Songti SC 4, Garamond 0, EB Garamond 0, Microsoft YaHei 0 — UNCHANGED across waves 4d, 4e, 4f and this one (M5). And a zero here is a claim about the QUERY, not about the machine (F5), which is why O-Orbit's face verdict was probed through CDP</automated>
    <automated>for p in O-simpleAdditive O-simpleGrain O-simpleSampler O-simpleSubtractive O-Polystutter O-Orbit; do echo -n "  $p gate files: "; find plugins/$p/tests -name '*.js' 2>/dev/null | wc -l | tr -d ' '; done   # 0 on all six. Fired with `find`, never a glob — a zsh glob that matches nothing aborts the whole command and reads as an answer. The N12 enumeration census HAS NO SUBJECT in this wave and no gate was invented (K7, wave 4c D4)</automated>
    <automated>for f in $(find plugins/*/tests -name '*.js' 2>/dev/null); do perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' "$f" > /tmp/qda-gate-strip.js; n=$(grep -cE "['\"]fr['\"]|\.fr\b" /tmp/qda-gate-strip.js); [ "$n" -gt 0 ] && echo "$f  $n"; done | tee /tmp/qda-t7-gates.txt | wc -l | tr -d ' '   # 8 — UNCHANGED from wave 4f's close-out, because none of this wave's six owns a gate file at all. If it moved, something outside this wave changed it. Every one of the eight belongs to a plugin that is already three-language and nobody has audited them (F2)</automated>
    <automated>git submodule status; git log --format=%H --since="1 day ago" | while read c; do n=$(git show --name-only --format= $c 2>/dev/null | grep -c "^plugins/O-Orbit/libs/SAF"); [ "$n" -gt 0 ] && echo "SUBMODULE STAGED IN $c"; done; echo "submodule scan done"   # " b6fe1882… (v1.3.4)" with a LEADING SPACE, and no commit in this wave stages a path under it. `git commit -- plugins/O-Orbit` INCLUDES the submodule path, so this is the guard working, not path-scoping working</automated>
    <automated>git status --short | grep -c "O-Strata"; git status --short | grep -c "agent-memory"; git tag --points-at HEAD | wc -l | tr -d ' '; git branch --show-current; git worktree list | wc -l | tr -d ' '   # 2, 1, 0, main, 1 — the two O-Strata planning files and the other session's agent-memory file are still UNTRACKED/UNSTAGED, no tag was created, and the wave stayed on main in one worktree</automated>
    <automated>git log --oneline -20 | grep -cE "O-simpleAdditive|O-simpleGrain|O-simpleSampler|O-simpleSubtractive|O-Polystutter|O-Orbit|PLUGINS.md"   # >= 13 — at least two path-scoped commits per plugin plus one PLUGINS.md commit at batch end</automated>
    <automated>git show --name-only --format= HEAD | grep -v "^PLUGINS.md$" | wc -l | tr -d ' '   # 0 — the batch-end commit touches PLUGINS.md and NOTHING else</automated>
    <human-check>The corpus was RECOMPUTED LIVE from the emitter and the assertion states BOTH denominators: check-i18n's 43 (plugins with an i18n.js, which was already 43 before the wave and proves nothing on its own) and the emitter's 37 -> 43 (plugins carrying zh rows). Their convergence on the same 43 is stated as the closure. Rows read 5441 of 5441 with BELOW SHIP BAR 0, and the six per-plugin figures were re-verified against the emitter's own rows column.</human-check>
    <human-check>O-Strata is named explicitly as the 44th plugin and excluded for a stated reason — Stage 0, no i18n.js under either UI root, the standing cause of both repo-wide lints' exit 2. The independent two-language scan over every plugin carrying an i18n.js returned EMPTY; if it had named any plugin, that name is in the SUMMARY and the rollout is NOT declared done.</human-check>
    <human-check>The false-prediction ledger is written in wave 4f §9's shape and contains, at minimum, the six already false at planning time: all six `from` versions (stale registry rows, not CMakeLists values); O-simpleSampler's ✅ Working cell against bundles already on disk; wave 4f instruction 5's report that the set()-variable CMake shape was absent (it is present, and the one-arm reader returns EMPTY on it); wave 4e D7's attribution of the routing-label wrap to O-simpleFM (a shared class with a carrier in this wave's own set); the assumption that these six carry no external stylesheet (five of six do); and 4f's D5 precondition that the O-Strata grep reads 1 (it reads 2). The COUNT is stated as the finding.</human-check>
    <human-check>Every inherited item is answered BY NUMBER: 4f D1-D12, 4e D4 (glossary divergence — whether a seventh was added), 4e D5 / 4f D7 (Z6 budget — whether any was added against the re-fired 549), s71 D2 (extended by this wave with four unscanned module JS files on its own plugins, the directory-scoped blindness, and the NEW fact that the preset-manager copies have FORKED), and s71 D3 (registry.yaml used_by — driven from nothing; the CMake grep was read instead). 4f D4 is stated as explicitly NOT INHERITED, because none of the six is a scala-tuning-engine consumer, verified two ways on all six.</human-check>
    <human-check>Every auval triple was READ OFF `auval -a` rather than guessed, and every `auval -v` was passed THREE UNQUOTED WORDS. The four-instruments/two-effects split was confirmed against the rows rather than assumed from IS_SYNTH. Any warning appearing on one of these six is reported as NEW INFORMATION and is not attributed to D10's O-Bowed and O-Wind warnings, which belong to other plugins.</human-check>
    <human-check>PLUGINS.md was edited ONCE, in its own commit touching nothing else, with all six version cells corrected from the CMakeLists values and O-simpleSampler's status cell flipped to 📦 Installed. The commit message names the six-row correction as a whole-suite registry miss — the second consecutive one — rather than letting it read as six typos. The duplicate-row check ran AFTER the edit and printed nothing.</human-check>
    <human-check>The two-step handoff required by CLAUDE.md was presented and the task STOPPED: what was completed, then Step 1 `/clear` and Step 2 the specific next command. Nothing was auto-invoked.</human-check>
  </verify>

  <done>
The batch is closed and the corpus is closed. One cold `auval -a` sweep (~85 s, background) covered all
six triples read off its own output, and six `auval -v` runs with three unquoted words each returned
AU VALIDATION SUCCEEDED. `PLUGINS.md` is corrected in one commit touching nothing else: six version
cells written from their CMakeLists values — **every one of which was a patch ahead of its registry
row, all six from the same 2026-09-03 suite-wide commit** — and O-simpleSampler's status cell flipped
from a `✅ Working` that was wrong about a machine already carrying its bundles. The duplicate-row
check printed nothing. The corpus was **recomputed live**: **5441 of 5441 rows, BELOW SHIP BAR 0, and
43 of 43 plugins**, with both denominators stated and their convergence named as the closure, with
**O-Strata named as the excluded 44th and the reason given**, and with an independent per-plugin scan
returning no plugin still on two languages. The installed-family table is re-derived and unchanged.
The N12 census is recorded as **having no subject** — zero gate files on all six, fired with `find`,
no gate invented — and the wider two-language census still reads 8 files on other plugins. The
submodule is unmoved at `b6fe1882` and no commit in the wave staged a path under it. The
false-prediction ledger is written with its count as the finding, pre-seeded with the six already
false at planning time. Every inherited item from 4f, 4e and s71 is answered by number, with s71 D2
**extended** by four unscanned module JS files found on this wave's own plugins and by the new fact
that the shared `preset-manager.js` copies have forked. The SUMMARY and `deferred-items.md` are
written beside this plan and **not committed**.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| blind reverse-read dispatch → external agent | 820 Chinese strings leave the machine as blinded rows. The withheld English must not leak, and the returned file is untrusted input joined back into a shipped table. |
| shared git checkout → HEAD and `.git/index` | Another session's staging can join a commit in the gap between the status check and the commit. Two sessions share HEAD. |
| git submodule pointer → `plugins/O-Orbit/libs/SAF` | The only submodule in the repo. A staged path under it rewrites the recorded pointer, which a path-scoped restore does not cleanly undo. |
| plugin bundle → macOS AudioUnit registry | An install writes into `~/Library/Audio/Plug-Ins/` and a cold `auval` rescan rebuilds a system registry Logic reads. |
| unscanned module JS → the rendered page | Four files under `modules/` and `js/modules/` reach the DOM and are opened by no gate in this repo. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-qda-01 | Information disclosure | `i18n-zh-backtranslate --emit` → blind reader | medium | mitigate | Pass **both** `--emit X --plugin X` (R5/F16) — `--emit` alone is corpus-scoped and hands an external reader all 4621 already-shipped rows as one plugin's work. Blinded 12-hex ids, per-batch salt, `--allowed-tools ""`, cwd outside the repo, repo access forbidden in the prompt. |
| T-qda-02 | Tampering | the returned `--ingest` file | high | mitigate | Untrusted input joined into a shipped table. `--manifest` explicit; ids checked **identical AND IN ORDER** before ingest (M12); rows checked well-formed with no Han surviving in the returned English; sha256 of the zh column against the committed tree. |
| T-qda-03 | Repudiation | the provenance chain | medium | mitigate | `--forward-provenance "<string>"` takes a **VALUE** (F9) — written bare, the manifest records `"--out"`, the ingest joins, and the identity check is silently vacuous. **Both** refusal controls fired against a properly provenanced emit, each printing its own message (F10). Model read from stderr, not from the request (F15). |
| T-qda-04 | Tampering | `.git/index` in a shared checkout | high | mitigate | Re-check `git branch --show-current` and `git status --short` **immediately before every commit**, never once at session start. Path-scoped `git commit -- plugins/<Name>` only; never `git add -A`, never `git commit -a`; options before the `--` (F14.6). |
| T-qda-05 | Tampering | `plugins/O-Orbit/libs/SAF` submodule pointer | high | mitigate | The `SUBMODULE_PATHS` guard runs before **every** commit in **every** task, aborting if any staged path starts with the submodule path. `git submodule status` re-confirmed after the build as well as before it. Verified at planning time that `add_subdirectory(libs/SAF/framework saf_build)` writes outside the submodule. |
| T-qda-06 | Denial of service | macOS AudioUnit registry | medium | mitigate | One cold sweep at batch end rather than six mid-batch. CLAUDE.md's cache sequence followed; alternate-variant orphans swept and the sweep's result recorded, because a leftover unsuffixed bundle pins Logic's registry slot to whichever was installed first. |
| T-qda-07 | Elevation of privilege | `plugins/O-Strata/.planning/*`, `.claude/agent-memory/*` | medium | mitigate | Another session's in-flight files, untracked/modified and not git-ignored. Named in every task's never-stage list and asserted by count in every task's verify (`grep -c "O-Strata"` = 2). |
| T-qda-08 | Information disclosure | the four unscanned module JS files | low | accept | `webview-drop-streaming.js` emits ~12 English toasts to the Chinese page and `preset-manager.js` writes an English preset name. **Accepted for this wave with a written reason**: keying them is a rollout with no calibrated gate, one file has two consumers and the other has eighteen with already-forked copies. Reported per plugin, carried as a deferred item, and explicitly not partially keyed. |
| T-qda-09 | Spoofing | version identity | medium | mitigate | Version truth is `CMakeLists.txt` read as a **value** with the two-arm reader (R9) — the one-arm reader returns EMPTY on O-simpleGrain. O-simpleGrain's `OSIMPLEGRAIN_VERSION_CODE` hex mirror moved in the same edit and proved by an arithmetic probe, because no gate in this repo reads it. |
| T-qda-10 | Tampering | npm/pip/cargo installs | n/a | accept | **No package-manager install occurs in this wave.** No `package.json`, `requirements.txt` or `Cargo.toml` is modified; all four instruments are committed repo scripts and Playwright is resolved from an existing install, never installed (`resolvePlaywright()` "deliberately does NOT install anything"). The Package Legitimacy Gate has no subject and this row records that as measured. |
| T-qda-11 | Repudiation | git tags | low | mitigate | **Never create a tag.** Tags belong to `/publish`. `git tag --points-at HEAD` asserted 0 in every task's verify. |
</threat_model>

<verification>
Batch-level checks, all of which live in Task 7's `<verify>` and are restated here as the phase gate:

- `check-i18n` repo-wide exit 0, `ALL CHECKS PASS — 43 localized plugin(s)`; exit 0 on each of the six.
- `i18n-zh-lint` repo-wide: **exit 2 is the baseline** (K2/D5); the criterion is `0 finding(s) across
  0 plugin(s)` and `1 plugin(s) could not be read`. `--self-test` 10/10. Exit 0 on each of the six.
- `i18n-fr-lint` repo-wide: `0 / 44 plugins with findings`, 1 could not be read — French untouched.
- `i18n-zh-backtranslate`: **5441 of 5441 rows, `BELOW SHIP BAR … 0`, 43 of 44 plugins carrying zh
  rows**, with O-Strata named as the exclusion and an independent per-plugin scan returning no plugin
  still on two languages.
- `check-ui-labels` exit 0 on all six with `no non-label element moved` green on the **fr** and
  **zh-Hans** arms across every state; coverage lines unchanged (40/40, 56/54, 48/47, 55/54, 97/97,
  37/56-with-19-never-visible).
- `measure-ui --report all` on all six against a **non-empty** Han input: `undeclared-font` 0,
  `wrap-count` 0 on five and **1 on O-simpleSubtractive** (its baseline), no node on a bare generic on
  any arm of O-Orbit, and every `line-height-normal` residual named with its measured `enH == zhH`.
- `boot-all-uis --strict-tips` exit 0, `clean: 43/43`, 0 DEAD across 0 plugins, exactly 2 late across 1
  plugin (the O-Bells census control).
- Zero Han under every plugin's `Source/`, each proved by a `perl -CSD` negative grep **beside a
  positive control fired on the same run**.
- `auval -v` SUCCEEDED on all six triples, read off `auval -a`, three unquoted words each.
- `PLUGINS.md`: six version cells corrected from CMakeLists, one status cell flipped, one commit
  touching nothing else, duplicate-row check empty.
- `git submodule status` unmoved at `b6fe1882`; no commit stages a path under it; no tag at HEAD; on
  `main`, one worktree; `O-Strata` still untracked at 2 lines.
</verification>

<success_criteria>
- Six plugins ship en/fr/zh-Hans at `reviewed: 'bt'` on all 820 rows, each with a MINOR bump taken
  from its **CMakeLists** value: 1.3.0, 1.5.0, 1.5.0, 1.5.0, 1.15.0, 1.3.0.
- **The corpus closes: 5441 of 5441 rows and 43 of 43 plugins**, asserted live from the emitter with
  both denominators stated, O-Strata named as the excluded 44th, and no plugin with an `i18n.js` left
  on two languages. If any is, it is **named** and the rollout is **not** declared done.
- O-simpleGrain's hex version mirror moved in lockstep with its version string.
- O-Orbit's page names an installed Latin face on every stack, its computed stack is identical on all
  three arms, and its submodule is untouched.
- Every gate green as listed in `<verification>`; no repo-wide baseline regressed.
- One PLUGINS.md commit at batch end; duplicate-row check empty; no tag created.
- The SUMMARY answers 4f D1-D12, 4e D4/D5/D7, s71 D2/D3 by number, states the false-prediction count,
  and carries the wave's own findings forward in `deferred-items.md`.
</success_criteria>

<output>
Create `.planning/quick/260907-qda-stage-4-wave-4g-of-the-zh-hans-rollout-l/260907-qda-TASK{N}-SUMMARY.md`
for each of Tasks 1-6 as the wave progresses, and at close-out:

- `.planning/quick/260907-qda-stage-4-wave-4g-of-the-zh-hans-rollout-l/260907-qda-SUMMARY.md`
- `.planning/quick/260907-qda-stage-4-wave-4g-of-the-zh-hans-rollout-l/deferred-items.md`

**Do NOT commit any of them, and do not touch `ROADMAP.md` or `STATE.md`** — the orchestrator commits
docs.

The batch SUMMARY must contain, at minimum:

1. **Per-plugin results** — rows, shipped version, the `from` version read from CMakeLists, gates.
2. **The LIVE corpus recount** with both denominators (K1) and O-Strata named.
3. **The false-prediction ledger** in wave 4f §9's shape, with the count stated as the finding.
4. **Every inherited item answered by number** — 4f D1-D12, 4e D4/D5/D7, s71 D2/D3 — plus the explicit
   statement that 4f D4 is not inherited.
5. **The N12 verdict: no subject** — zero gate files on all six, fired with `find`, no gate invented.
6. **The unscanned-module report** (K8) with the four files, their strings, the directory-scoped
   blindness, the fork between the two `preset-manager.js` copies, and the scope-out reason.
7. **The registry pattern** — six stale rows from one commit, the second consecutive whole-suite miss.
8. **The auval log summary**, with any warning on these six flagged as new information.

`deferred-items.md` must carry forward, at minimum: the unscanned module JS files and their fork; the
`div.routing-label` scope correction (K9); the registry-staleness pattern; O-Orbit's nineteen
structurally-unmeasurable labels; D1's `'dist lpf'` glossary root; the Z6 backfill; the eight remaining
two-language gate files; and `reviewed: 'native'` as a disclosed, open quality level on all 5441 rows.
</output>
