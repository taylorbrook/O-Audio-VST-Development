---
phase: quick-260905-acr
plan: 01
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4b, O-Tremolo, O-IntonationPad, O-DigiDelay, O-AnalogEQ, O-SimpleReverb, O-Emulator, lint-gate, fonts, geometry, gate-repair]
status: complete
completed: 2026-09-05
requirements: [ZH4B-01, ZH4B-02, ZH4B-03, ZH4B-04, ZH4B-05, ZH4B-06, ZH4B-07, ZH4B-08, ZH4B-09, ZH4B-10, ZH4B-11]

dependency_graph:
  requires:
    - quick-260904-qrc   # wave 4a — the pattern, and findings 1-13
    - quick-260904-g5l   # Stage 3 — executor rules R1-R9
    - quick-260904-q4j   # i18n-zh-lint repaired to 10/10 and 0 findings
  provides:
    - "i18n-zh-lint as a GATE (exit 2), for waves 4c-4g"
    - "six three-language plugins; corpus at 1830/1830 'bt' across 16 plugins"
    - "the four-form font-carrier census, including the undeclared-element form"
    - "five gate files that derive their language list"
  affects:
    - scripts/i18n-zh-lint.js   # exit contract changed for every caller

tech_stack:
  added: []
  patterns:
    - "derive-or-abort: a gate's language list comes off the table's own export, and an unusable list exits non-zero rather than passing vacuously"
    - "line-height pins derived from each leaf's own measured English box — unitless, scoped, never global"
    - "CJK tail BEFORE the trailing generic; face-naming as a SEPARATE repair that holds no Chinese"
    - "blind reverse read via `claude -p --allowed-tools \"\"` from a cwd outside the repo"

key_files:
  created: []
  modified:
    - scripts/i18n-zh-lint.js
    - plugins/O-Tremolo/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-IntonationPad/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/ui/public/css/tuning-panel.css,Source/ui/public/js/app.js,Source/ui/public/js/tuning-panel.js,Source/PluginProcessor.h,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-DigiDelay/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-AnalogEQ/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-SimpleReverb/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - plugins/O-Emulator/{Source/ui/public/js/i18n.js,Source/ui/public/index.html,Source/PluginProcessor.h,tests/ui_tip_render_check.js,CMakeLists.txt,CHANGELOG.md}
    - PLUGINS.md

decisions:
  - "i18n-zh-lint FLIPPED to a gate, in its own commit, before any authoring — its own promotion criterion was met and measured, and attribution requires the flip and the rows to be separable"
  - "A plugin that cannot be READ now blocks the lint too — a corpus zero that includes an unchecked plugin is the vacuity failure the tool exists to refuse"
  - "Enumerations in hover-help bodies are DELETED, never extended to three — an enumeration is false again the next time an option lands"
  - "Three glossary roots that failed a reverse read were replaced per-row with a termNote; the roots themselves are REPORTED, not edited, because a settled root is repo-wide"
  - "O-IntonationPad's late tip bindings fixed by load ORDER (static import + pre-sweep render), not by a third re-sweep"

metrics:
  duration: "~4h"
  tasks: 3
  commits: 18
  files_changed: 40
  entries_authored: 363
  rows_authored: 492

actuals:
  tokens: 118000     # chars/4 over the files actually changed (2375 insertions, 469 deletions)
  tasks: 3
  commits: 18
---

# Quick Task 260905-acr: Stage 4 wave 4b of the zh-Hans rollout Summary

Six plugins ship English, French and Simplified Chinese — 363 entries / 492 rows
at `reviewed: 'bt'`, built, installed and auval-clean — and `i18n-zh-lint` is now
a gate rather than a report, flipped before the first row was written.

## What shipped

| plugin | version | entries | rows | AU triple (read off `auval -a`) |
|---|---|---|---|---|
| O-Tremolo | 1.9.1 → **1.10.0** | 34 | 43 | `aufx OuTr OuDv` ✓ |
| O-IntonationPad | "2.9.2" → **2.10.0** | 199 | 276 | `aumu OuIP OuDv` ✓ |
| O-DigiDelay | 1.5.1 → **1.6.0** | 36 | 46 | `aufx OuDD OuDv` ✓ |
| O-AnalogEQ | 1.4.1 → **1.5.0** | 35 | 49 | `aufx OuAE OuDv` ✓ |
| O-SimpleReverb | 1.8.1 → **1.9.0** | 32 | 43 | `aufx OuSr OuDv` ✓ |
| O-Emulator | 1.3.1 → **1.4.0** | 27 | 35 | `aufx OEmu OuDv` ✓ |

Corpus: **1338 → 1830 rows, 1830 at `'bt'`, 0 at `'mt'`**, 10 → 16 localized
plugins. One cold auval sweep for all six.

## The lint flip (ZH4B-10)

Made first, in its own commit (`fed08208`), touching nothing else — so an exit 0
there is the flip behaving on a clean corpus and any later exit 2 is the rows.
The tool's own promotion criterion, stated in its header since Stage 2, was met
and measured: 1034 entries, 0 findings across 43 plugins, `--self-test` 10/10.

**Both controls fired and were recorded.**

- **Negative** — flipped, run repo-wide against the tree as found: exit 0. A gate
  that fires on a clean corpus is not a gate.
- **Positive** — one intra-Han U+0020 planted in a committed row (O-Chorus
  `tip.tipsToggle`, `悬停帮助` → `悬停 帮助`): exit 2, Z8 naming that row by key.
  Reverted by **targeted reverse edit, never `git checkout --`** because
  uncommitted lint work was in the same tree; sha256 confirmed byte-identical to
  HEAD and `git diff --stat -- plugins/` empty.

**Authoring was never blocked** — 492 rows were drafted at `reviewed: 'mt'` under
the live gate with the lint exiting 0 throughout, because `'mt'` routes to the
ship-bar line rather than to findings. That routing was verified in source
before the flip, not assumed.

**It fired on real work exactly once, and earned its keep doing so.** `拉伸`
departs from a settled glossary root, so Z5 exited 2 — and it fired on the
tooltip TITLE while the caption's `termNote` already exempted the caption. A tip
title is lint-checked separately from the label it mirrors, so the exemption has
to be stated on both. Under the old report-only tool that would have been a line
of output nobody had to read.

Rule 2 addition beyond the flip: **a plugin that could not be READ now blocks
too.** Its row printed ERROR and contributed no findings, so without this a
corpus zero could include a plugin nothing was checked on.

## What the wave found that the plan did not predict

### A FOURTH font carrier: an element with no declaration at all

The plan named three (CSS declaration, SVG presentation attribute, role-named
custom property) and all three were confirmed. A fourth was measured:

**`<button>`, `<select>` and `<input>` do not inherit `font-family`.** The UA
stylesheet gives them one — on this build, Arial. So a sweep that reads every
declaration AND every attribute in both files finds nothing wrong with them, and
only `getComputedStyle` finds them. Arial has no Han glyphs, so their Chinese
captions were resolved by the document-language fallback rather than by anything
the page names — W1's inert-tail failure arriving through an **absent**
declaration instead of a misplaced one.

Found on **13 nodes in O-IntonationPad and 1 in O-AnalogEQ**. Repaired with
Arial kept FIRST, so the Latin metrics of the en and fr arms do not move.

**Consequence for wave 4c: a grep-based font census is structurally incomplete.**
Three of the four forms are greppable and one is not.

### A gate can spell its language count as an INTEGER

Wave 4a found two string spellings; this wave found a third form, and only by
running the gate:

```js
if (drivenStates.length <= 2) {      // O-AnalogEQ — the literal 2 IS the language count
```

With three languages the zh pass recorded no heights at all and assertion 5
failed reporting that *nothing had moved*, on a page where five things had. No
census greps for an integer near a language walk.

### The consequence stated for the late tip bindings was false

The plan's N7 asserted that a late binding means the tip never carries its
Chinese. **Measured before changing anything: all 80 anchors already carried a
tip at settle in all three languages** — `app.js` re-swept after the panel
mounted, and its comment says so. The ordering defect was real (the page spent
its first frames with a 17-anchor hole in the help layer, and the census could
not tell that hole from a permanent one) and was fixed at the source: the import
is now static and the panel's synchronous `render()` runs at module top level,
ahead of the first sweep, with `render()` made idempotent.

**Two wrong turns on the way, both caught by re-measuring rather than by
reasoning.** Removing the dynamic import removed the network round trip that had
been pushing the re-sweep past `DOMContentLoaded` *by accident*, so it began
running before the handler that builds the page's forty knobs: 17 late became 40.
And gating on `readyState === 'loading'` did not fix it either — a module script
is deferred, so `readyState` already reads `'interactive'` while
`DOMContentLoaded` has NOT fired, and the guard skipped the wait entirely. Only
`'complete'` means the event is past.

Census now **0 late / 0 dead**, with O-Bells still reporting its known 2 as the
control that the census is not blind.

### A zh row can outlive the English sentence it was translated from

On O-SimpleReverb, one commit both deleted the gear body's exclusivity clause
from en and fr AND authored the zh row for that body — from the text as it stood
*before* the deletion. The Chinese went on asserting what the English had
stopped asserting, and **every gate was green**: `check-i18n` only asks that the
key resolves, the lint only checks typography and terminology, and **no tool in
this repo compares a `zh` body against its own `en`**. Only the reverse read
caught it, returning the deleted sentence in the language it had been deleted
from.

### Measurement method changes the answer

Two harness defects in this wave each hid a real page defect:

- **A wide viewport hides wrap-count defects.** O-Emulator's engraved plate wraps
  to three lines in en/fr and two in zh at the shipping 700 × 380 — and to one
  line in all three at 1200 px, where the defect does not exist.
- **The state file is a CUMULATIVE walk.** O-IntonationPad's rotation view needs
  the tuning tab first; resetting between states measures a page those states
  never produce.

## Verification

**Per plugin, all six:** `check-i18n` exit 0 with `LANGUAGES` reading three;
`check-ui-labels` exit 0, 0 FAIL on the en, fr and zh arms (all 19 states on
O-IntonationPad); `i18n-zh-lint` 0 findings, `BELOW SHIP BAR 0`; zero Han under
`Source/**` with the positive control fired on the same run; bare-generic census
0 including the SVG attribute form.

**Repo-wide:** `check-i18n` exit 0, 43 localized plugins. The flipped
`i18n-zh-lint` exit 0 on 1397 entries, 0 findings / 43 plugins; `--self-test`
10/10. `i18n-fr-lint` exit 0 — French untouched by the whole wave.
`boot-all-uis --strict-tips` 0 dead, 0 failed, 2 late (O-Bells' control, down
from 19). Back-translate stage view 1830 rows / 1830 `bt` / 0 `mt`. PLUGINS.md no
duplicate rows, six for six AGREE with CMakeLists (six for six MISMATCH before).

**Gate files:** all five derive their language list, 0 two-language literals in
any syntax with comments included, 0 direction-specific assertions, and the
derive-or-abort control fired on each with the reverting edit byte-identical.
O-Tremolo 186 → 263 checks, O-DigiDelay 186 → 300, O-AnalogEQ 260 → 415.

**auval:** all six SUCCEEDED, one cold rescan, every triple read off `auval -a`.

## Deviations from Plan

### Auto-fixed

**1. [Rule 1 — Bug] The comment explaining a deleted enumeration was itself spelling it**
- **Found during:** the tracer feedback gate, re-running Task 1's verify
- **Issue:** O-Tremolo's comment explaining the deleted fixed-pair sentence
  quoted it verbatim, so the wave's own probe kept reporting a fixed plugin
- **Fix:** reworded as prose. **Widened:** the rule is not about gate files, it
  is about any comment that explains a deleted string
- **Commit:** `98b49d5a`

**2. [Rule 2 — Missing correctness] The lint did not block an unreadable plugin**
- **Issue:** a plugin whose table could not be read printed ERROR and contributed
  no findings, so a corpus zero could include a plugin nothing was checked on
- **Fix:** errors now block. `errors = 0` on the tree, so the negative control is
  unaffected. **Commit:** `fed08208`

**3. [Rule 1 — Bug] O-AnalogEQ's gate recorded no heights for a third language**
- **Issue:** `if (drivenStates.length <= 2)` — the literal 2 was the language
  count. **Fix:** derived from `LANGUAGES.length`. **Commit:** `ad11d940`

**4. [Rule 1 — Bug] A zh body contradicted its own corrected English**
- **Found during:** the blind reverse read. **Commit:** `6a7663d9`

**5. [Rule 1 — Bug] Three glossary roots failed the reverse read**
- `延展` → "Sustain"/"Spread", `时值` → "Duration" (two models), `植物律` →
  "Plant Law". Each replaced per-row with a `termNote` recording the evidence;
  the roots themselves reported, not edited. **Commits:** `f72ecfd6`, others

### Corrections to the plan's own measurements

- **N7 is false as stated.** A late tip binding on O-IntonationPad did NOT mean
  the tip carried no Chinese — measured, 80/80 anchors carried one at settle in
  all three languages. The ordering defect was real; the stated consequence was
  not.
- **The auval manufacturer is `OuDv`, not `Ouar`.** Read off `auval -a` as the
  plan required. Six CHANGELOG lines had been written with the assumed value and
  were corrected in `d0227c6c`.
- **Font carriers are four forms, not three.**

## Known Stubs

None. No stub, placeholder or unwired data path was introduced. Every
`reviewed: 'native'` remains open across the corpus, which is a **disclosed
quality level rather than a stub** — printed by the lint's R1 rule on every run
and stated in all six CHANGELOGs. See `deferred-items.md` D1.

## Human-verify items (end-of-phase)

All three of Task 1's `<human-check>` items were performed and their evidence is
recorded above and in the commit messages:

1. **Both lint-flip controls fired.** Exit 0 on the clean corpus; exit 2 on the
   planted intra-Han space naming Z8; sha256 byte-identical to HEAD after the
   targeted revert.
2. **The derive-or-abort control fired on all five repaired gate files.** Planted
   empty export exits non-zero; every reverting edit left the file
   byte-identical.
3. **Every back-translation triple read with `--verbose`** — 492 rows over 7
   dispatches, three rounds on O-IntonationPad and two on O-SimpleReverb. Each
   accepted drift carries a written collision-on-the-page reason in its
   promotion commit; four rows failed and were re-authored.

## Self-Check: PASSED

All 18 commits present in `git log`. All modified files exist on disk. All six
`-dev` bundles present in both plugin folders with no alternate-variant orphan.
No file deleted anywhere in the task (`git diff --diff-filter=D` over the whole
range is empty).
