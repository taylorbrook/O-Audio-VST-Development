---
phase: quick-260905-rwh
plan: 01
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4c, O-Detune, O-Bells, O-Gain, O-SpectralShaper, O-Bassoon, O-TextureForge, gates, fonts, geometry, canvas-text, webpack]
status: complete
requires:
  - scripts/i18n-zh-glossary.js
  - scripts/i18n-zh-lint.js (flipped to a gate in 260904-acr)
  - scripts/i18n-zh-backtranslate.js
  - scripts/measure-ui.js (promoted in 260905-izp)
  - scripts/check-i18n.js, scripts/check-ui-labels.js, scripts/boot-all-uis.js
provides:
  - six three-language plugins (en / fr / zh-Hans) at reviewed:'bt'
  - four tip-render gates derived at BOTH their language sites
  - the localized canvas caption pattern applied a second time (O-SpectralShaper, after O-Comp)
  - a CSS-only repair path for form-4 nodes created by a webpack bundle
affects:
  - PLUGINS.md (six rows), the zh corpus (1830 -> 2440 rows, 16 -> 22 plugins)
tech-stack:
  added: []
  patterns:
    - "derive-or-abort on BOTH a gate's language assertion AND its language walk"
    - "line-height pins derived per leaf from its own measured English box, unitless"
    - "min-* FLOORS for the shape where the Chinese is SHORTER than the English"
    - "face-naming and CJK-tailing kept separable, so a needed tail is distinguishable from a decorative one"
key-files:
  created: []
  modified:
    - plugins/O-Detune/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-Bells/{Resources/ui/js/i18n.js,Resources/ui/index.html,Resources/ui/css/tuning-panel.css,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-Gain/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-SpectralShaper/{Resources/ui/js/i18n.js,Resources/ui/index.html,Resources/ui/css/styles.css,Resources/ui/js/components/Spectrogram.js,Source/PluginProcessor.h,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-Bassoon/{Resources/ui/js/i18n.js,Resources/ui/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-TextureForge/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/ouaricon-naturalist.css,Source/PluginProcessor.h,tests/ui_tip_render_check.js,tests/i18n-states.json,CMakeLists.txt,CHANGELOG.md}
    - PLUGINS.md
decisions:
  - "O-SpectralShaper's canvas caption is LOCALIZED, following O-Comp's shipped precedent — I18N with an empty body, read through tr() from the render path, deliberately not in LABELS"
  - "The shared scala-tuning-engine tuning panel stays exactly as found; deferred with all five consumers named"
  - "O-Bells' two late tip bindings stay; they are pinned by the gate's own EXPECTED_LATE assertion and are the wave's boot-all-uis census control"
  - "O-TextureForge's form-4 repair is a CSS rule, NOT a src/app.js edit — no webpack rebuild, and the bundle stays byte-identical"
  - "O-Bells' 阻尼 collision is a screened NON-collision: the two controls live in mutually exclusive tab panels"
  - "O-Gain's `measure` diverges from the glossary root under a termNote — the root means a musical BAR"
metrics:
  duration: "~5h"
  completed: 2026-09-05
  rows_authored: 610
  corpus_before: 1830
  corpus_after: 2440
  plugins_localized_before: 16
  plugins_localized_after: 22
  commits: 16
actuals:
  tokens: 268000
  tasks: 3
  commits: 16
---

# Quick task 260905-rwh: Stage 4 wave 4c — six plugins ship zh-Hans

Six plugins now render English, French and Simplified Chinese: **O-Detune 1.9.0,
O-Bells 4.5.0, O-Gain 1.4.0, O-SpectralShaper 1.8.0, O-Bassoon 1.4.0,
O-TextureForge 1.4.0**. 610 emitter rows authored at `reviewed: 'mt'`, read back
through blind reverse passes, and promoted to `reviewed: 'bt'` — the corpus goes
**1830 → 2440 rows and 16 → 22 localized plugins**, with `BELOW SHIP BAR 0`
across all 43.

## Shipped versions, with the AU triple read off `auval -a`

| plugin | version | type | AU triple | `auval -v` | bundle version |
|---|---|---|---|---|---|
| O-Detune | 1.8.1 → **1.9.0** | effect | `aufx OuDt OuDv` | AU VALIDATION SUCCEEDED | 1.9.0 (VST3 + AU) |
| O-Bells | 4.4.1 → **4.5.0** | instrument | `aumu OBls OuDv` | AU VALIDATION SUCCEEDED | 4.5.0 |
| O-Gain | 1.3.3 → **1.4.0** | effect | `aufx OGan OuDv` | AU VALIDATION SUCCEEDED | 1.4.0 |
| O-SpectralShaper | 1.7.3 → **1.8.0** | effect | `aufx OSpS OuDv` | AU VALIDATION SUCCEEDED | 1.8.0 |
| O-Bassoon | 1.3.1 → **1.4.0** | instrument | `aumu OBsn OuDv` | AU VALIDATION SUCCEEDED | 1.4.0 |
| O-TextureForge | 1.3.1 → **1.4.0** | instrument | `aumu OuTF OuDv` | AU VALIDATION SUCCEEDED | 1.4.0 |

Every triple was **read off `auval -a`**, not assumed. Three effects and three
instruments, manufacturer `OuDv`. One cold registry rescan covered all six, as
planned — six separate runs would have cost six rescans.

## Corpus, before and after

| | before | after |
|---|---|---|
| zh-Hans rows | 1830 | **2440** |
| localized plugins carrying zh | 16 | **22** |
| `BELOW SHIP BAR` (rows at `'mt'` or unflagged) | 0 | **0** |
| `i18n-zh-lint` findings, 43 plugins / 1844 entries | 0 | **0**, `GATE PASSED — exit 0` |

Per plugin, as the lint counts them: O-Bells 255, O-Gain 89,
**O-SpectralShaper 72**, O-Detune 71, O-Bassoon 62, O-TextureForge 61 — **610**,
one more than the plan's 609, because the canvas caption is an entry the plan
budgeted as a repair and not as a row.

## Gate results, against the shipped tree

Repo-wide, once, after everything:

| gate | result |
|---|---|
| `check-i18n` | exit 0 — `ALL CHECKS PASS — 43 localized plugin(s)` |
| `i18n-zh-lint` (now a GATE) | exit 0 — 0 findings across 43 plugins, 1844 entries |
| `i18n-zh-lint --self-test` | 10/10 |
| `i18n-fr-lint` | exit 0 — French untouched by the whole wave |
| `boot-all-uis --strict-tips` | 0 DEAD, 0 failed, **exactly 2 late** (O-Bells' pinned pair — the census control, unchanged) |
| `i18n-zh-backtranslate` stage view | 2440 zh rows of 5255, `BELOW SHIP BAR 0` |
| `PLUGINS.md` duplicate check | prints nothing |
| cmake vs registry | **AGREE six for six** (MISMATCH six for six before) |
| alternate-variant orphans on disk | 0 |

Per plugin, all six: `check-i18n` 0, `check-ui-labels` **0 FAIL on the en, fr AND
zh arms**, `i18n-zh-lint` 0 findings, zero Han under `Source/**/*.{h,cpp}` with
the positive control firing on the same run, and every `measure-ui` zero quoted
beside its non-empty Han-node count:

| plugin | nodes measured | undeclared-font | line-height-normal | wrap-count | svg-font-attr | zh Han nodes / on no CJK face |
|---|---|---|---|---|---|---|
| O-Detune | 489 | 0 | 0 | 0 | 0 | 45 / **0** |
| O-Bells | 3222 | **196 → 0** | 19 (all non-movers) | 0 | 0 | 196 / **0** |
| O-Gain | 366 | 0 | 0 | 0 | 0 | 46 / **0** |
| O-SpectralShaper | 375 | 0 | 11 (all non-movers) | 0 | 0 | 41 / **0** |
| O-Bassoon | 513 | 0 | 3 (all non-movers) | 0 | 0 | 43 / **0** |
| O-TextureForge | 342 | **2 → 0** | 2 (all non-movers) | **1 → 0** | 0 | 38 / **0** |

All four tip-render gates pass **with a real Chinese arm swept**, hold no
two-language literal in any syntax with comments stripped (0, 0, 0, 0), and had
their derive-or-abort control **fired against the WALK** — a planted empty
`LANGUAGES` export makes each exit non-zero, and every reverting edit left the
table byte-identical to HEAD by sha256. O-SpectralShaper's preset gate was read
end to end, found to hold no language reference of any kind, left untouched, and
still passes.

## What each plugin cost, and what it taught

**O-Detune (tracer, 71 rows).** Proved the four things the other five inherited:
the two-site gate repair; the direction assertion rewritten as a difference (its
`[5]` required the non-English pass to be strictly *taller*, and Chinese **shrank
13 of 14 tips** — 124→109, 124→93, 93→78 — so the old form would have hard-failed
a correct page); the tail-only font case (all nine sites, no face named anywhere,
because Georgia is installed); and the blind-dispatch shape.

**O-Bells (heavy, 255 rows, 14 states).** 32 form-4 nodes closed with Arial
first — `undeclared-font` went **196 → 0** against a 3222-node input. Its bare
`monospace` is the wave's only declaration taking *both* halves of the font work.
584 `line-height: normal` leaves, of which the movers needed 26 pins. Two width
FLOORS, both because the Chinese is narrower: the footer gain caption (26.50 →
20.41) moved 44 elements in all 14 states, and the rotation table's corner header
broke *between* its two characters — min-content of one glyph — which grew the
header row and reported all thirteen ASCII note-name headers beside it as wrap
defects because their **row** grew.

**O-Gain (89 rows).** The simplest page in the wave and still two findings: the
glossary root for `measure` means a musical **bar** and this control selects the
measurement *algorithm*, so it diverges under a `termNote` — and the `termNote`
turned out to be **entry-scoped**, so the caption's note did not cover the tooltip
title and Z5 reported the title with the caption already clear. `.utility-btn`'s
pin had to be derived **per line**: seven buttons share a 334 px flex row, so
their captions already wrap in English and the measured box is two line boxes.

**O-SpectralShaper (72 rows).** The wave's live canvas text carrier. One
hard-coded English sentence painted into a canvas by the Canvas-2D fallback,
invisible to every gate in this repo, **shipped untranslated on the French page
for the whole of the French rollout**. Localized to O-Comp's precedent. Its
correctness rests entirely on two independent readers recovering "WebGL not
supported" verbatim, because the branch that paints it cannot be produced on this
machine.

**O-Bassoon (62 rows).** The wave's only face-naming-**without**-a-tail: two
nodes whose stacks fell through to a bare generic but which render the exempt
product name and can never receive Han. The About tab produced both
shorter-Chinese shapes at once — a paragraph that wraps to two lines instead of
three, and a caption 32.8 px narrower than its English, displacing the version
number beside it.

**O-TextureForge (61 rows).** The M10 conditional fired. Two `<button>` elements
built at runtime by the webpack bundle took bare Arial under Chinese captions —
and the plugin's single-state file meant nothing ever opened that overlay, so the
`undeclared-font` screen read **0 as a vacuum**. A second state made it read 2.
**The repair is a CSS rule, not a source edit**, so the bundle was never rebuilt
and `git diff --stat` on it is empty.

## Deviations from the plan

1. **The canvas caption is a ROW, not just a repair.** O-SpectralShaper ships 72
   emitter rows, not 71, and the corpus lands at 2440 rather than 2439. The plan
   budgeted `canvas.webglUnsupported` as structural work; the emitter counts it
   as an entry.
2. **O-TextureForge's form-4 repair went into CSS instead of `src/app.js`.** The
   plan's conditional said that if the overlay measurement demanded a repair,
   `src/app.js` changes and the rebuilt bundle is committed alongside. A
   stylesheet rule reaches a runtime-created node just as well, and it makes the
   bundle-matches-source control trivially true rather than something to
   re-verify. Strictly better, and the plan's own control (`git diff --stat` on
   the bundle) is satisfied by construction.
3. **The overlay state is a selector-reach probe, and is labelled as one.**
   `showFileSizeWarning()` is a bundle-local function the page does not expose, so
   no state file can drive the shipped code path. The added state builds an
   element with the same id and button structure — which is exactly the question a
   selector can answer.
4. **Four correction rounds, not the one the plan implies.** O-Detune (2 rows),
   O-Bells (1), O-SpectralShaper (1) and O-TextureForge (3) each needed a
   re-author and therefore a different-model round two. O-Gain and O-Bassoon
   produced no re-author and therefore ran **one** round — stated in their commits
   rather than left as an unexplained asymmetry, because round two exists to keep
   a *correction* independent, and there is nothing for it to be independent of
   when nothing was corrected.

## Corrections to the plan's own measurements

Wave 4b's C9 records that a plan's stated consequence can be wrong even when the
defect is real. Five of this plan's measurements were wrong as stated:

1. **`VERSION` IS quoted on O-Bassoon.** The plan states, as a measurement, that
   *none* of this wave's six quotes the value. `plugins/O-Bassoon/CMakeLists.txt`
   line 14 reads `VERSION "1.3.1"`. R9's instruction to read the value rather than
   match a literal is what made the bump land anyway — the rule survived its own
   plan being wrong about it.
2. **`measure-ui`'s `line-height-normal` screen cannot reach 0 on most pages, and
   the plan's verify asks it to.** The screen filters on `x.han`, and `han` is
   computed over `own` text **plus `data-tip`, `data-tip-title` and
   `aria-label`** — so a value readout whose own text is `"50%"` but which carries
   a Chinese `aria-label` is a finding, forever, and its box is identical on both
   arms. The shipped, wave-4b-passed O-Tremolo reads **3** on this screen today.
   The load-bearing criterion is `check-ui-labels` **0 moved**, which all six
   meet; the residuals here (19, 11, 3, 2) are each an evidenced non-mover with
   `enH == zhH`. Pinning them would be decoration, and on O-Bells' `div.tab`
   (measured `u = 2.9231`) actively risky.
3. **O-Bells has 33 form-4 nodes to fix, not 32 — or rather, the 32 are right and
   the `html` element is a 33rd carrier the plan does not name.** `html` resolves
   to `Times` (the UA default) because the page declares its stack on `body`. It
   renders no text and everything under `body` re-declares, so it is left; but it
   is a stack ending at a generic that no census entry in the plan accounts for.
4. **The "no plugin name in the dispatched batch" blinding control cannot pass on
   a plugin whose own copy names it.** O-Gain's `ms-enc` body tells the user to
   add a second **O-Gain** set to DEC. That is the string being translated; it
   cannot be withheld without withholding the row. The control fired, was
   adjudicated rather than waved through, and what actually covers the boundary is
   `--allowed-tools ""` plus the prompt prohibition.
5. **`node -e "require(...)"` over `measure-ui`'s stdout needs BOTH streams
   redirected to files, not `2>&1 1>/dev/null`.** The plan's stated form is
   correct as shell semantics but does not survive this harness; `>/tmp/x.json
   2>/tmp/x.err` does. Minor, and it cost two runs to find.

## Findings for the glossary owners

- **Two renderings of near-identical English, in one wave, across two plugins.**
  O-TextureForge renders "WebGL unavailable" as `WebGL 不可用`; O-SpectralShaper
  renders "WebGL not supported" as `不支持 WebGL`. Neither English string is a
  `TERMS` key, so **rule Z5 cannot see the divergence** and no gate in this repo
  can. This is exactly the class of drift the glossary exists to prevent, found by
  reading two batches side by side. Reported, not settled — editing a shared term
  list is a repo-wide change with its own budget.
- **`measure` → 小节 is wrong on any loudness meter.** The root means a musical
  bar. O-Gain diverges under a `termNote` and the blind reader recovered 测量 as
  "Measure", which is the strongest evidence available that the divergence is
  right. A second consumer of that root should be looked for before wave 4d.

## Self-Check: PASSED

All six installed bundles report the shipped `CFBundleShortVersionString` in both
VST3 and AU. All 16 commits are in `git log`. `PLUGINS.md` agrees with
`CMakeLists.txt` six for six and holds no duplicate row.
