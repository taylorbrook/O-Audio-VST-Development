---
phase: quick-260905-vp3
plan: 01
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4d, O-MultiBandCompressor, O-Formant, O-ReverseDelay, O-Marimba, O-FreqPulse, gates, fonts, geometry, canvas-text, cmake-variable]
status: complete
requires:
  - scripts/i18n-zh-glossary.js
  - scripts/i18n-zh-lint.js (a GATE since 260905-acr — exit 2 on any finding)
  - scripts/i18n-zh-backtranslate.js
  - scripts/measure-ui.js (--report all, promoted in 260905-izp)
  - scripts/check-i18n.js, scripts/check-ui-labels.js, scripts/boot-all-uis.js
provides:
  - five three-language plugins (en / fr / zh-Hans) at reviewed:'bt'
  - a clamp gate that opens its plugin's i18n table where it previously had NO table access at all
  - a tip gate whose height assertion is a MAGNITUDE rather than a DIRECTION
  - the two-arm CMake version reader, proven against both the literal and the variable form
  - the localized canvas-caption pattern applied a third time (O-MultiBandCompressor, after O-Comp and O-SpectralShaper)
affects:
  - PLUGINS.md (five rows), the zh corpus (2440 -> 3073 rows, 22 -> 27 plugins)
tech-stack:
  added: []
  patterns:
    - "derive-or-abort fired against the WALK, and against every per-language MAP LOOKUP"
    - "a gate's language list derived through a vm sandbox on a file that had no table access"
    - "line-height pins derived per leaf from its own measured English CONTENT box, padding and border subtracted"
    - "an ELEMENT-LEVEL form-4 rule, which loses to every class and id rule and so reaches only the undeclared controls"
    - "min-* FLOORS wherever the Chinese is SHORTER — a width floor at the EXACT measured English box, not rounded"
key-files:
  created: []
  modified:
    - plugins/O-MultiBandCompressor/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/styles.css,Source/ui/public/js/app.js,Source/PluginProcessor.h,tests/i18n-states.json,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-Formant/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-ReverseDelay/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/styles.css,Source/PluginProcessor.h,tests/ui_tooltip_clamp_check.js,tests/ui_frontend_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-Marimba/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-FreqPulse/{Resources/ui/js/i18n.js,Resources/ui/index.html,Resources/ui/css/styles.css,Source/PluginProcessor.h,CMakeLists.txt,CHANGELOG.md}
    - PLUGINS.md
decisions:
  - "O-MultiBandCompressor's one-space-divergent C++ codec was HAND-EDITED and proved by an explicit grep, never by a sweep's exit code; its whitespace is normalised to the sibling form so the next sweep can reach it"
  - "O-MultiBandCompressor's canvas caption is LOCALIZED to O-SpectralShaper's shipped precedent; the build-stage marker one line below it is left exactly as found and deferred as a product question"
  - "O-ReverseDelay's clamp gate got a vm-sandbox table LOADER added — the file had zero table access, so there was nothing to derive from until one was built"
  - "O-ReverseDelay's frontend gate had a stale D13 assertion, red on the tree as found since v1.11.0; INVERTED rather than deleted"
  - "The Scatter / Diffusion pair on O-ReverseDelay: only the Scatter side is qualified, because it is the side ambiguous about WHAT it scatters"
  - "O-Formant's glottal Shimmer diverges from the glossary root under a termNote — 微光 is the REVERB sense and is already spent on this same plugin's effects tab"
  - "The shared scala-tuning-engine tuning panel stays exactly as found; no plugin in this wave is a consumer"
  - "No gate file was invented for the three plugins that lack one — a gate written mid-localization is a gate nobody has calibrated"
metrics:
  duration: "~4h"
  completed: 2026-09-06
  rows_authored: 633
  corpus_before: 2440
  corpus_after: 3073
  plugins_localized_before: 22
  plugins_localized_after: 27
  commits: 11
actuals:
  tokens: 258000
  tasks: 3
  commits: 11
---

# Quick task 260905-vp3: Stage 4 wave 4d — five plugins ship zh-Hans

Five plugins now render English, French and Simplified Chinese:
**O-MultiBandCompressor 1.12.0, O-Formant 1.29.0, O-ReverseDelay 1.12.0,
O-Marimba 1.14.0, O-FreqPulse 1.19.0**. 633 emitter rows authored at
`reviewed: 'mt'`, read back through blind reverse passes, and promoted to
`reviewed: 'bt'` — the corpus goes **2440 → 3073 rows and 22 → 27 localized
plugins**, with `BELOW SHIP BAR 0` across all 43.

## Shipped versions, with the AU triple read off `auval -a`

| plugin | version | type | AU triple | `auval -v` | bundle version |
|---|---|---|---|---|---|
| O-MultiBandCompressor | 1.11.2 → **1.12.0** | effect | `aufx OMbc OuDv` | AU VALIDATION SUCCEEDED | 1.12.0 (VST3 + AU) |
| O-Formant | 1.28.1 → **1.29.0** | instrument | `aumu OuFm OuDv` | AU VALIDATION SUCCEEDED | 1.29.0 |
| O-ReverseDelay | 1.11.1 → **1.12.0** | effect | `aufx ORvD OuDv` | AU VALIDATION SUCCEEDED | 1.12.0 |
| O-Marimba | 1.13.2 → **1.14.0** | instrument | `aumu OuMa OuDv` | AU VALIDATION SUCCEEDED | 1.14.0 |
| O-FreqPulse | 1.18.3 → **1.19.0** | effect | `aufx OFPu OuDv` | AU VALIDATION SUCCEEDED | 1.19.0 |

Every triple was **read off `auval -a`**, not assumed. Two instruments and three
effects, manufacturer `OuDv` — which is what the plan predicted from `IS_SYNTH`,
and the rows confirm it rather than the prediction standing in for them. One cold
registry rescan covered all five, as planned.

**A tooling correction worth carrying:** `auval -v "aumu OuFm OuDv"` — the triple
quoted as one shell word — returns `Invalid Arguments: auval` and nothing else.
It reads three separate arguments. A sweep that quotes the triple reports five
failures that are not failures. Fired and corrected here; wave 4e should pass the
three words unquoted.

## Corpus, before and after

| | before | after |
|---|---|---|
| zh-Hans rows | 2440 | **3073** |
| localized plugins carrying zh | 22 | **27** |
| `BELOW SHIP BAR` (rows at `'mt'` or unflagged) | 0 | **0** |
| `i18n-zh-lint` findings, 43 plugins / 2312 entries | 0 | **0**, `GATE PASSED — exit 0` |

Per plugin, as the lint counts them: **O-Formant 245**, O-ReverseDelay 106,
O-MultiBandCompressor 99, O-FreqPulse 96, O-Marimba 87 — **633**, one more than
the plan's 632, because O-MultiBandCompressor's canvas caption is an entry the
plan budgeted as a repair and not as a row. The whole corpus totals **5256**, not
the 5255 the plan cited, for the same reason.

## Gate results, against the shipped tree

Repo-wide, once, after everything:

| gate | result |
|---|---|
| `check-i18n` | exit 0 — `ALL CHECKS PASS — 43 localized plugin(s)` |
| `i18n-zh-lint` (a GATE) | exit 0 — 0 findings across 43 plugins, 2312 entries |
| `i18n-zh-lint --self-test` | 10/10 |
| `i18n-fr-lint` | exit 0 — French untouched by the whole wave |
| `boot-all-uis --strict-tips` | 0 DEAD, 0 failed, 0 warn, **exactly 2 late** (O-Bells' pinned pair — the census control, unchanged) |
| `i18n-zh-backtranslate` stage view | **3073 zh rows of 5256**, all at `'bt'`, `BELOW SHIP BAR 0` |
| `PLUGINS.md` duplicate check | prints nothing |
| cmake vs registry | **AGREE five for five** (MISMATCH five for five before) |
| alternate-variant orphans on disk | 0 |
| `git tag --points-at HEAD` | 0 — no tag was created |

Per plugin, all five: `check-i18n` exit 0, `check-ui-labels` **exit 0 with 0 moved
on the en, fr AND zh arms**, `i18n-zh-lint` 0 findings, zero Han under
`Source/**/*.{h,cpp}` with the positive control firing on the same run (1 file
each), and every `measure-ui` zero quoted beside its non-empty Han-node count:

| plugin | nodes measured | undeclared-font | line-height-normal | wrap-count | svg-font-attr | zh Han keys / on no CJK face |
|---|---|---|---|---|---|---|
| O-MultiBandCompressor | 999 | **1 node → 0** | 1 (non-mover) | 0 | 0 | 173 / **0** |
| O-Formant | 2673 | **162 → 0** | 5 (all non-movers) | **1 → 0** | 0 | 162 / **0** |
| O-ReverseDelay | 705 | 0 | 1 (non-mover) | 0 | 0 | 87 / **0** |
| O-Marimba | 1167 | 0 | 8 (all non-movers) | 0 | 0 | 57 / **0** |
| O-FreqPulse | 1038 | **86 → 0** | 23 (all non-movers) | 0 | 0 | 86 / **0** |

`measure-ui --verbose` reports **no unresolved state** on any of the five, so
every state-gated zero above is measured rather than assumed — including
O-Formant's 15-state walk and O-FreqPulse's five, whose order is load-bearing
because its band-controls popover closes on a mousedown elsewhere.

### Every `line-height-normal` residual, named with its measured equality (N1)

The screen cannot reach 0 on a page whose chrome buttons carry Chinese accessible
names: `han` is computed over `data-tip`, `data-tip-title` and `aria-label` as
well as the node's own text, so a glyph button with a Chinese `aria-label` is a
finding forever and no pin can change it. **The load-bearing criterion is
`check-ui-labels` reporting 0 moved**, and that is what all five report. Every
residual below is an evidenced NON-MOVER:

| plugin | residuals |
|---|---|
| O-MultiBandCompressor | `#auto-makeup` enH == zhH == 30 |
| O-Formant | `div.tab` ×4 at enH == zhH == 25; `div.interval-list-header` at 22 |
| O-ReverseDelay | `#envelopeCanvas` enH == frH == zhH == 58 — it renders no text at all |
| O-Marimba | `#gear-btn`, `#preset-prev`, `#preset-next` at 20; `#tab-sound`, `#tab-effects` at 28; `#tab-tuning` at 29; `#tonic-down` at 19; `#tonic-up` at 16 |
| O-FreqPulse | the three preset-bar buttons at 22; per band ×4: mute/solo at 14, clear/random at 22, expand at 28 |

### The two repaired gates

| | O-Formant `ui_tip_render_check.js` | O-ReverseDelay `ui_tooltip_clamp_check.js` |
|---|---|---|
| language list | derived from `LANGUAGES` (was a hand-written array) | derived through a `vm` sandbox — **the file had NO table access at all** |
| walk | loops the derived list | both walks already looped the list; the list now derives |
| per-language map lookups | n/a | **3 sites** generalized: 2 feeding a hard-fail assertion, 1 feeding a print |
| direction assertion | rewritten as a **magnitude difference**, per non-English language | n/a |
| two-language literal, comments stripped | **0** | **0** |
| `LANGUAGES` references | **2** (derive + abort guard) | **4** |
| derive-or-abort fired against the WALK | yes — planted empty export → exit 1 | yes — planted empty export → exit 1, `Refusing to sweep a guessed language list` |
| revert byte-identical by sha256 | `0fcf83cf…` both sides | `d57d35f2…` both sides |
| passes with a real zh arm | yes, 1794 assertions | yes |

O-ReverseDelay's `ui_frontend_check.js` was read end to end, re-confirmed
language-blind **after** the wave's edits (0 occurrences of `'en'`, `'fr'`,
`LANGUAGES` or `__setLanguage` with comments stripped) — and found to be **red on
the tree as found**. See deviation 2.

## What each plugin cost, and what it taught

**O-MultiBandCompressor (tracer, 99 rows, 1 → 2 states).** Executed by a previous
executor; its findings are read from its two commits and its 1.12.0 CHANGELOG.
Four things the other four inherited. **The C++ codec was HAND-EDITED**: its
`languageCode`/`languageIndex` pair diverged from the copies in all 42 sibling
plugins by single spaces and carried no doc comment, so a sweep keyed on the
sibling string would have matched neither line, reported success, and shipped a
two-way codec behind a three-language table; the edit was proved by a grep
asserting the Chinese code on both lines and the whitespace normalised so the next
sweep can reach it. **The two-arm CMake reader** was built here because
`set(OMBC_VERSION …)` returns EMPTY on the single-arm pattern every prior wave
used, and it was cross-checked against the single-arm reader on this wave's other
four — AGREE four for four, so it is a superset and not a second special case.
**The canvas caption** was routed through the table on O-SpectralShaper's
precedent. And **the one-state coverage hole**: the state file never opened the
preset dropdown, which carries five runtime-built keys; a second state took the
census 975 → 999 nodes. One row was re-authored on its blind read — the
gain-reduction meter cap, whose contraction came back as "Increase/Decrease", the
opposite of what the meter shows.

**O-Formant (heavy, 245 rows, 15 states).** The wave's largest surface and both
gate defects. **Its `[5]` assertion required the non-English pass to be strictly
TALLER**, which is true of French and false of Chinese: this run measures the
tallest tip at 125.3 px in en and fr and **110.0 px in zh**, so the old form would
have hard-failed a page on which nothing was wrong. Twelve declarations named
Garamond and fell straight to the serif generic — 21 nodes rendering this
plugin's **English** through a Chinese face under `lang="zh-Hans"`. 31 form
controls built by its own tuning panel took the UA face; the descendant rule
reaches exactly those 31 because the census shows every form control outside
`#tuning-container` already carrying a declared stack and every one inside it on
the UA face. 101 leaves moved and took 28 pins; two findings were not line-height
cases at all and took floors — `Intervals (11 notes)` wraps to two lines in
English and one in Chinese (a line COUNT difference), and the rotation table's
first column is auto-sized by a `Mode` header 3.20 px narrower in Chinese.
`check-ui-labels` went **240 moved → 0**.

**O-ReverseDelay (106 rows, 3 states).** The hardest gate work in the wave and the
easiest font work. **The clamp gate contained zero occurrences of `I18N`,
`TIP_BINDINGS`, `LANGUAGES` or any path to `js/i18n.js`** — so "derive the list
from the table" was not an edit to an existing link; there was no link. And the
**fifth enumeration form** was live: three sites reading a Map by a language key
written as a literal, two of them feeding a real hard-fail assertion, which would
have left the Chinese page swept, measured and never compared after the list and
both walks were repaired. Its **settings body was FALSE BY OMISSION** — it named
the language and never mentioned the hover-help switch that has been in the panel
since v1.11.0 — so it took a one-clause ADDITION, not the deletion every prior
release in the rollout has made, and no negative grep could have found it. Fonts:
one `--serif` token reaches all ten declarations, Times New Roman is already named
and installed, and the census finds ZERO form controls on the UA face and ZERO
bare generics. One edit covers the page.

**O-Marimba (87 rows, 3 states).** The wave's **two naked generics** — `index.html`
L755 and L765 declared `font-family: monospace` and named no family at all,
reaching 12 nodes, the purest form of the defect in the whole rollout. And the
**fifth knob column**: four were pinned at v1.13.0/v1.13.1 because their French
caption measured differently and RESONANCE was explicitly left out because
RÉSONANCE measures the same column; 共振 is 14.02 px narrower, so the
shrink-to-fit column collapsed onto the 55 px knob and dragged the knob, its
indicator and its readout 7 px sideways. `.mts-label` needed a pin **the census
could not see**: its row is `display: none` until MTS-ESP mode is entered, so
`measure-ui` reports it at h=0 and only the state-driven gate ever saw it grow.

**O-FreqPulse (96 rows, 5 states).** The wave's second-largest form-4 population —
**24 controls on the UA face behind nine `font-family: inherit` declarations
already in the same stylesheet**. A grep of that file reports a plugin that has had
the repair; the census reports one that has had it on some controls. The repair is
an ELEMENT-LEVEL rule, and that is what makes it safe: at specificity (0,0,1) it
loses to every class and id rule, so the 77 controls already resolving to Georgia
through an explicit `inherit` and the 8 already naming Arial keep exactly what they
had — verified by re-running the screen, not by reading the CSS. Its three `label`
populations render at **8px, 12px and 11px** and are pinned separately: the N11
case made concrete, where one rule would have been wrong in two places out of
three.

## The blind reverse reads

Every batch: `--emit <Plugin> --plugin <Plugin>` (both, per W4 — the usage line
alone emits the whole 3073-row corpus), `--forward-provenance` recorded at emit,
an explicit `--manifest` at ingest, a fresh 16-hex salt per plugin and per
correction round, and a fresh reader per chunk — `claude -p … --allowed-tools ""`
from a cwd **outside the repo**, with repository access forbidden in the prompt.
Every triple read with `--verbose`.

| plugin | rows | chunks | ids in order | Han surviving | malformed / empty | product name in batch | re-authored | correction round |
|---|---|---|---|---|---|---|---|---|
| O-MultiBandCompressor | 99 | 2 | yes | 0 | 0 / 0 | hit, adjudicated | 1 | yes, different model, corrected nothing further |
| O-Formant | 245 | 3 | **NO → yes after repair** | 0 | 0 / 0 | 0 (measured) | 2 | yes, different model, corrected nothing further |
| O-ReverseDelay | 106 | 2 | yes | 0 | 0 / 0 | 0 (measured) | 1 | yes, different model, corrected nothing further |
| O-Marimba | 87 | 2 | yes | 0 | 0 / 0 | 0 (measured) | **0** | **none — a round that corrects nothing needs no round two** |
| O-FreqPulse | 96 | 2 | yes | 0 | 0 / 0 | 0 (measured) | **0** | **none** |

**The identity control earned its keep on O-Formant.** The first pass returned 245
lines — the right count, looking complete — but the ids did not match the emitted
order: one row had been **dropped outright** and one came back with a truncated
7-hex id and an **empty English**. Neither is a translation finding, and neither
would have been visible without the control. Both were re-read by a different
model and spliced back at their emitted positions.

**The product-name control's zeros are MEASURED, not vacuous.** Wave 4c's N7 warns
to expect a hit on any plugin whose copy names itself. Four of this wave's five do
NOT: their product names live only in `I18N_EXEMPT`, which is not emitted. The
same pattern was fired against each table file on the same run — 7, 8 and 6 hits
respectively — so the zeros are the control working rather than the control
missing. Only O-MultiBandCompressor reported a hit, and it was adjudicated.

### The four rows the reverse read sent back, and why

1. **O-MultiBandCompressor, the GR meter cap.** A two-character contraction came
   back as "Increase/Decrease" — the opposite of what a gain-reduction meter
   shows. Re-authored to the full phrase.
2. **O-Formant, the glottal Shimmer** (title and caption). Came back as
   "Amplitude Jitter" — and **Jitter is the knob immediately beside it**. Not a
   drift, a page collision. Re-authored to the voice-science term for amplitude
   perturbation, which shares no character with the Jitter rendering. Round two
   returned "Amplitude Perturbation".
3. **O-Formant, the consonant pad's `mixed` manner readout.** Came back as "Mix",
   which is a knob on this plugin's effects tab. Re-authored to the phonetic term
   for a plosive–fricative blend — an affricate — which is both the correct word
   and a word that collides with nothing. Round two returned "Affricate".
4. **O-ReverseDelay, Scatter.** Its glossary root came back as "Spread" — and so
   did the settled root for the **Diffusion** knob two groups away on the same
   single-page layout. Neither is the wrong word, and the two do NOT share a
   Chinese rendering, so the mechanical downstream check that looks for two keys
   mapping to one string could not see it and no lint rule could either. **Only
   the reverse read did**, by returning the same English word twice. Round two
   returned "Delay Spread" and "Diffusion Amount".

Everything else was accepted with a written reason. The recurring shape is the
reverse read **working**, not drifting: Chinese has no abbreviations, so `Atk`,
`Fric`, `Plos`, `Lab`, `Alv`, `Pal`, `Vel`, `Trans`, `Vib Depth`, `Pre-dly`,
`Singer's F`, `Size Rnd` and `Gain Rnd` all come back as the full words they stand
for, and an uppercase caption comes back in ordinary case. The two deliberate R3
qualifications on O-Formant proved themselves in the same output: Transition Time
and Consonant Transition came back as exactly those two names, which is the pair
the qualification existed to keep apart.

## Checked non-defects — read, left, and recorded

An unexamined correct body and an unexamined false one look identical in a diff.

| plugin | what was read | verdict |
|---|---|---|
| O-MultiBandCompressor | both hover-help bodies | already true — first plugin in the rollout whose language body carries no enumeration; the probe read 0 **before** the work too |
| O-Marimba | the `settings` body | already names both of the popover's controls |
| O-FreqPulse | the `settings` body | already names both |
| O-Formant | `js/tuning-panel.js`, 1068 lines, 43 `data-i18n` hooks | its own copy, not the shared module, so in scope — but every caption was already keyed and `check-i18n` assertions 12/13/15 scan it. No edit needed, none made |
| O-Formant | the `fillText` at `js/main.js:677` | paints an `F1..F5` marker already in `I18N_EXEMPT`, through an installed Courier New stack. No change |
| O-Formant | the whole `I18N_EXEMPT` list, before authoring | exempt option words (Cascade / Parallel / Hybrid, Normal / PingPong) survive verbatim into the Chinese; keyed ones are named by their localized caption, so `Off or On` reads `关或开` |
| O-ReverseDelay | `tests/ui_frontend_check.js`, 1303 lines | language-blind by measurement, re-confirmed after the wave — but see deviation 2 |
| O-ReverseDelay | the shape option words Hann / Tukey / Expo-Decay | populated from the `AudioParameterChoice`, so the control keeps them; kept verbatim in the Chinese bodies, as the French does |
| all five | the shared `scala-tuning-engine` tuning panel | untouched; no plugin in this wave is a consumer |

## Deviations from the plan

1. **The canvas caption is a ROW, not just a repair.** O-MultiBandCompressor ships
   99 emitter rows, not 98, and the corpus lands at 3073 of 5256 rather than 3072
   of 5255. The plan budgeted the caption as structural work; the emitter counts
   it as an entry. This is the same deviation wave 4c recorded for
   O-SpectralShaper, and it is now twice-observed: **a canvas caption routed
   through the table always adds a row.**

2. **O-ReverseDelay's `ui_frontend_check.js` was RED on the tree as found, and the
   plan recorded it as passing.** The plan's precondition and its live-observation
   table both state that this gate needs no repair. It is indeed language-blind —
   0 occurrences of `'en'`, `'fr'`, `LANGUAGES` or `__setLanguage` with comments
   stripped, re-confirmed after the wave rather than before it — but it carried a
   D13 assertion written at v1.10.0 requiring the `ui.on` / `ui.off` label pair to
   be **ABSENT**, on the reasoning that "this plugin has NO hover-help toggle and
   never will". v1.11.0 shipped that toggle, its markup and its two table keys,
   and `check-i18n` assertion 16 has required it since. The premise was falsified
   two releases ago and the check had been failing ever since. **Fixed under
   deviation Rule 1** and INVERTED rather than deleted: the pair is now required,
   because a toggle whose two faces are raw literals rather than table keys is
   stranded in the previous language the instant the selector fires. Verified
   pre-existing by reading `HEAD`'s own `index.html`, which already carried a
   `data-i18n="ui.on"`.

3. **O-Marimba's zh endonym was added to `I18N_EXEMPT`; the other four needed
   nothing.** O-Marimba's exempt list already carried `English` and `Français` as
   endonyms; the third is added beside them for symmetry and to document the
   decision. `check-i18n` passes with or without it on the other four, so no entry
   was invented where none was needed.

4. **O-Formant's `label.transition` needed a `termNote` the plan did not
   predict.** The page carries TWO transition controls — the Character section's
   Transition Time and the consonant envelope's Transition — and both English
   names normalise to a glossary key. R3 says qualify both sides, never one; doing
   so meant diverging from the settled root on both, which needs an entry-scoped
   note on each. Four notes in total for that pair.

## Corrections to the plan's own measurements

Wave 4c's N5 records that a plan can be wrong about its own measurement and be
saved by a rule. Three here, in the shape wave 4c used.

1. **`ui_frontend_check.js` exits 0 on the tree as found — FALSE.** It exits 1.
   See deviation 2. The plan asserted this in two places (the Task-3 precondition
   and the live-observation narrative) and neither had been fired. The rule that
   saved it is C4's own converse: *absence of a language reference is a
   measurement, not an assumption* — and so is the presence of a passing exit
   code. **Fire every precondition you assert, including the ones that look
   uninteresting.**

2. **The row count is 632 — off by one.** It is 633, and the corpus totals 5256
   rather than 5255, because the canvas caption becomes an entry. R1's own
   instruction — read the emitter's `rows` column rather than estimating — is
   what caught it, and it caught the same thing in wave 4c.

3. **`auval -v "<triple>"` — the quoted form in the plan's own close-out prose
   does not work.** `auval` reads three arguments; a quoted triple returns
   `Invalid Arguments: auval` for every plugin, which reads exactly like five
   failures. This is C11's family of trap (zsh quoting) meeting a tool that gives
   no useful error. Recorded so wave 4e passes the three words unquoted.

Everything else the plan measured held: 632→633 aside, the five row counts, the
18 O-Formant font declarations split 3 + 12 + 3, the 31 / 24 / 1 form-4
populations, the 12 O-Formant and 3 O-Marimba generic-only declarations, the 2
O-Marimba naked generics, the two-arm reader agreeing with the single-arm reader
four for four, the endonym entity string, the fifth enumeration form, and the
gear/language body dispositions on all five.

## Commits

| # | hash | what |
|---|---|---|
| 1 | `7bb7f39d` | feat(O-MultiBandCompressor): zh-Hans table at reviewed:'mt', fonts, geometry and the hand-edited codec |
| 2 | `b974b64c` | feat(O-MultiBandCompressor): promote zh-Hans to reviewed:'bt', v1.12.0 |
| 3 | `49ea5baa` | feat(O-Formant): zh-Hans table at reviewed:'mt', fonts, geometry and both gate defects |
| 4 | `8697c3f8` | feat(O-Formant): promote zh-Hans to reviewed:'bt', v1.29.0 |
| 5 | `fa4a9235` | feat(O-ReverseDelay): zh-Hans table at reviewed:'mt', the FIFTH enumeration form, and a gate that had no table access at all |
| 6 | `c152bff2` | feat(O-ReverseDelay): promote zh-Hans to reviewed:'bt', v1.12.0 |
| 7 | `dfc37cb3` | feat(O-Marimba): zh-Hans table at reviewed:'mt', two naked generics and a fifth column pin |
| 8 | `f72acd8f` | feat(O-Marimba): promote zh-Hans to reviewed:'bt', v1.14.0 |
| 9 | `f95b10f3` | feat(O-FreqPulse): zh-Hans table at reviewed:'mt', and 24 form controls nine inherit declarations never reached |
| 10 | `d5a1e9ad` | feat(O-FreqPulse): promote zh-Hans to reviewed:'bt', v1.19.0 |
| 11 | `a56e2ace` | docs(PLUGINS.md): five wave-4d rows corrected from their CMakeLists values |

One path-scoped commit stream per plugin, `PLUGINS.md` once at the end, every
message ending with the session trailer. **No git tag was created** —
`git tag --points-at HEAD` reads 0. Nothing was pushed.

## Known stubs

None. No file in this wave carries a placeholder, a hard-coded empty value that
reaches the UI, or a `TODO` introduced by this work.

`reviewed: 'native'` stays OPEN on all 3073 rows of the corpus and is not a
blocker — this project has no native Simplified Chinese reader. It is a
**disclosed** quality level, printed by lint rule R1 on every run and stated in
all five CHANGELOGs.

## Self-Check: PASSED

All five plugins' `i18n.js`, `CMakeLists.txt` and `CHANGELOG.md` exist and carry
the shipped values; `PLUGINS.md` carries five corrected rows agreeing with their
CMakeLists five for five; all eleven commit hashes above resolve in
`git log --oneline --all`; the four installed bundles checked report the shipped
`CFBundleShortVersionString`; all five `auval -v` runs report
`AU VALIDATION SUCCEEDED`.
