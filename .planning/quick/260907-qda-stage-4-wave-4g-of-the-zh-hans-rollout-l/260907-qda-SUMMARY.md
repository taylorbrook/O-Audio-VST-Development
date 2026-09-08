---
phase: quick-260907-qda
plan: 01
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4g, corpus-close, O-simpleAdditive, O-simpleGrain, O-simpleSampler, O-simpleSubtractive, O-Polystutter, O-Orbit, submodule, fonts, geometry, unscanned-modules, auval]
status: complete
dependency_graph:
  requires:
    - "wave 4f (260907-ja8) — 4621 of 5441 rows, 37 plugins carrying zh"
    - "wave 4e (260906-h8y) — D2 boot census control, D4 glossary divergence report, D5 Z6 backfill, D7 routing-label"
    - "s71 (260906-s71) — D2 unscanned module JS, D3 stale registry.yaml used_by, D11 auval budget"
  provides:
    - "Simplified Chinese on the last six localized plugins — 820 rows at reviewed:'bt'"
    - "PLUGINS.md six rows reconciled against their CMakeLists (177d0c7a)"
    - "the zh-Hans corpus CLOSED: 44 of 44 plugins, 5789 of 5789 rows, 0 below ship bar"
  affects:
    - "PLUGINS.md"
    - "plugins/{O-simpleAdditive,O-simpleGrain,O-simpleSampler,O-simpleSubtractive,O-Polystutter,O-Orbit}"
tech_stack:
  added: []
  patterns:
    - "one cold auval rescan per BATCH, never per plugin"
    - "read every AU triple off `auval -a`'s own output; reassemble an interleaved row rather than guess it"
    - "PLUGINS.md version cells written from CMakeLists, never from the row being corrected"
    - "corpus totals recomputed LIVE at close-out with a timestamp, never carried forward"
key_files:
  created:
    - .planning/quick/260907-qda-stage-4-wave-4g-of-the-zh-hans-rollout-l/260907-qda-SUMMARY.md
    - .planning/quick/260907-qda-stage-4-wave-4g-of-the-zh-hans-rollout-l/deferred-items.md
  modified:
    - PLUGINS.md
decisions:
  - "The corpus assertion is stated as 44 of 44, not the plan's 43 of 43 — O-Strata arrived fully localized mid-wave by a concurrent session (0e838512) and this wave is credited with none of its 348 rows."
  - "The six PLUGINS.md version cells are written from the CMakeLists values, not from the stale rows — the registry's SECOND consecutive whole-suite miss, both from suite-wide commits that bumped every CMakeLists and updated no row."
  - "The O-simpleAdditive `Scan LFO Rate` auval warning is reported as NEW INFORMATION relative to D10 and adjudicated as PRE-EXISTING: it is a skewed-range normalization round-trip, on a file this wave never opened."
  - "F2's 'eight two-language gate files' is corrected to THIRTEEN — the same census, with five `ui-stub/juce-stub.js` files F2's table omits."
metrics:
  duration: "~5h wall across seven fresh executors"
  completed: 2026-09-07
actuals:
  tokens: 41000
  tasks: 7
  commits: 13
---

# Wave 4g — Stage 4 of the zh-Hans rollout: six plugins, and the corpus closes

Seven fresh executors, thirteen commits, **820 emitter rows** of Simplified Chinese across the
last six localized plugins that carried none. Every row authored at `reviewed: 'mt'`, read back
blind by a different model, and promoted to `'bt'` in a separate commit.

**The wave's headline, stated as measured and not as planned:** the six plugins delivered exactly
their 820 rows and took the 43-plugin set the task named from **37 → 43** localized. The corpus
now reads **44 of 44 plugins and 5789 of 5789 rows** — because O-Strata arrived fully localized
from another session while this wave was running. **This wave is credited with none of O-Strata's
348 rows.** §12 states both readings side by side.

---

## 1. What shipped, per plugin

| # | plugin | from | ships | rows | commits |
|---|---|---|---|---|---|
| 1 | **O-simpleAdditive** (tracer) | 1.2.1 | **1.3.0** | 131 | `e130f2b9` (mt) · `663bdd07` (bt) |
| 2 | **O-simpleGrain** | 1.4.3 | **1.5.0** | 153 | `8ee82be2` (mt) · `8b5d7a70` (bt) |
| 3 | **O-simpleSampler** | 1.4.4 | **1.5.0** | 145 | `c5effaac` (mt) · `b1d9e6b2` (bt) |
| 4 | **O-simpleSubtractive** | 1.4.1 | **1.5.0** | 133 | `fff62ae8` (mt) · `09bb37cf` (bt) |
| 5 | **O-Polystutter** | 1.14.3 | **1.15.0** | 133 | `ad264e7e` (mt) · `ce5a6a8c` (bt) |
| 6 | **O-Orbit** | 1.2.3 | **1.3.0** | 125 | `0efb343e` (mt) · `7ae50ad8` (bt) |
| 7 | *batch close-out* | — | — | — | `177d0c7a` (PLUGINS.md) |

`131 + 153 + 145 + 133 + 133 + 125 = **820**`, and every one of the six per-plugin figures matched
the carry-forward exactly when re-verified off the emitter at close-out. **O-Polystutter stayed at
133 and not 134** — K3's body-less `msg-delete-preset` was mirrored rather than given a body, and
the prediction that it would hold is one of the wave's few explicitly-flagged predictions that did.

**Every `from` version in the task description was one patch low on all six**, because it was a
faithful read of six stale registry rows. See §5.

**O-simpleGrain's second version source moved with the string:** `OSIMPLEGRAIN_VERSION "1.5.0"` and
`OSIMPLEGRAIN_VERSION_CODE 0x010500`, both in `plugins/O-simpleGrain/CMakeLists.txt`, verified live
at close-out. It is the only `${VAR}` mirror in the wave; the other frozen literals are dead and
deliberately untouched (deferred D1).

**The submodule never moved.** `plugins/O-Orbit/libs/SAF` reads ` b6fe188288ecd59fef9e46bf6bfe3ed57f0ee9af … (v1.3.4)`
with its leading space, and a per-commit scan over all thirteen commits found **zero** staged paths
under it. `git commit -- plugins/O-Orbit` includes the submodule path, so this is the guard working,
not path-scoping working.

---

## 2. The cold auval sweep — ONE rescan, all six PASS

Cache cleared per CLAUDE.md (`killall -9 AudioComponentRegistrar`, both cache directories removed),
then `auval -a` in the background against a sentinel.

**Measured cold-rescan duration: 104.6 s** — first internal timestamp `2026-09-07 22:41:44.593`,
last `22:43:29.170`. The sentinel was absent at t+55 s and present at the next poll. The ~85 s
budget (wave 4g instruction 12, s71 D11) is the right **order**; this run came in ~20 s above it,
consistent with the registry having gained O-Strata since 4f. **The stale 15-minute figure is wrong
by an order of magnitude and this is the third consecutive measurement saying so.**

### The six triples, read off `auval -a`'s own output

| plugin | triple | how it was read |
|---|---|---|
| O-simpleAdditive | `aumu OSiA OuDv` | log line 2006, verbatim |
| O-simpleGrain | `aumu OsGr OuDv` | log line 2011, verbatim |
| O-simpleSampler | `aumu OsSm OuDv` | log line 2013, verbatim |
| O-simpleSubtractive | `aumu OSiS OuDv` | log line 2009, verbatim |
| O-Polystutter | **`aumf OuPs OuDv`** | log line 1981, verbatim |
| O-Orbit | **`aufx OuOr OuDv`** | **reassembled from two fragments of one interrupted line** |

**O-Orbit's row is the finding here.** The AU component loads SAF, whose banner
(`SAF Version: 1.3.5, License: GNU GPLv2`) and O-Orbit's own `[PresetManager] Factory presets
initialized: 12` are written to the same stream with no trailing newline. The row was torn in two:

```
line  572:  aufx OuO[PresetManager] Factory presets initialized: 12
line 1752:  r OuDv  -  Ouaricon Audio Development: O-Orbit-dev
```

`aufx OuO` + `r OuDv` = **`aufx OuOr OuDv`**. That is a read of `auval -a`'s own bytes, not a guess,
and both fragments are quoted above so a later reader can check it. **A grep for the plugin name on
this log returns a row that looks truncated and would have been unusable if taken at face value.**

**Correction to the plan's split.** The plan predicted *"four instruments and two effects"*. Measured:
**four `aumu`, one `aumf` (O-Polystutter is a Music Effect, not a plain audio effect), and one
`aufx`.** `IS_SYNTH FALSE` on both non-instruments was correct; the two-code split was not.

### Verdicts — six `auval -v` runs, THREE UNQUOTED WORDS each (M14)

| plugin | verdict | warnings |
|---|---|---|
| O-simpleAdditive | **AU VALIDATION SUCCEEDED** | **1 — see below** |
| O-simpleGrain | **AU VALIDATION SUCCEEDED** | none |
| O-simpleSampler | **AU VALIDATION SUCCEEDED** | none |
| O-simpleSubtractive | **AU VALIDATION SUCCEEDED** | none |
| O-Polystutter | **AU VALIDATION SUCCEEDED** | none |
| O-Orbit | **AU VALIDATION SUCCEEDED** | none |

`grep -ciE "AU VALIDATION SUCCEEDED"` over the concatenated logs: **6**. All six runs together took
under 25 s.

`Bad Max Frames - Render should fail` appears in **all six** logs and is **the name of a test that
expects a failure**, not a finding.

### The one warning, verbatim, and its adjudication

```
Parameter ID:343751544
Name: Scan LFO Rate
Parameter Type: Generic
Values: Minimum = 0.0100000, Default = 0.5000002, Maximum = 20.0000000
Flags: Values Have Strings, High Resolution, Can Ramp, Readable, Writable
WARNING: retrievedValue = 0.328712 (was 0.328712), Parameter did not retain default value when set
```

**Reported as NEW INFORMATION, exactly as the plan requires.** D10's two known warnings belong to
**O-Bowed** (`Bow Position`) and **O-Wind** (channel layout) and to **neither of this wave's six**;
this one is not transplanted onto them.

**And then adjudicated, because "new information" is not the same as "regression":**

- `plugins/O-simpleAdditive/Source/PluginProcessor.cpp:84-87` declares
  `NormalisableRange<float> { 0.01f, 20.0f, 0.0f, 0.3f }` with default `0.5f`. **Skew 0.3.**
- `((0.5 − 0.01) / 19.99) ^ 0.3 = 0.024512 ^ 0.3 = **0.328712**`, which is `retrievedValue` to six
  decimal places. The retrieved value is the **correct normalized form of the 0.5 Hz default**;
  auval compares it against the raw `0.5` and calls the mismatch a failure to retain. `(was
  0.328712)` — the two figures are identical, which is the giveaway that the round-trip is exact.
- **This wave touched no parameter, range, type, state format or audio path.**
  `plugins/O-simpleAdditive/Source/PluginProcessor.cpp` was last committed at `b1082fc0`, **2026-08-27**,
  and appears in neither of Task 1's commits.

**Verdict: pre-existing, structural to any skewed AU parameter, not caused by this wave, and not a
blocker.** Filed as a new deferred item so the next O-simpleAdditive parameter pass has it in hand.

---

## 3. The installed-family table (M5), re-derived — UNCHANGED, and F5 restated beside it

| family | count | | family | count |
|---|---|---|---|---|
| Georgia | 4 | | Garamond | **0** |
| Times New Roman | 4 | | EB Garamond | **0** |
| Arial | 4 | | Adobe Garamond Pro | **0** |
| Menlo | 4 | | Microsoft YaHei | **0** |
| Courier New | 4 | | Consolas | **0** |
| PingFang SC | 6 | | Segoe UI Symbol | **0** |
| Songti SC | 4 | | Noto Sans Symbols2 | **0** |
| Helvetica Neue | 14 | | | |
| Arial Unicode MS | 1 | | | |
| Apple Symbols | 1 | | | |

**Identical to waves 4d, 4e and 4f — four consecutive waves, no movement.** It is a property of the
machine, re-derived rather than carried, as instruction 4 requires.

**The consequence, which is this wave's largest font finding (K10).** All six plugins lead their
house stack with **Garamond, which is absent**. Five survive on a Times New Roman or Georgia named
later in the same stack. **O-Orbit did not** — its single stack `Garamond, 'EB Garamond', serif`
named **no installed member at all**, so the entire page's Latin resolved through a bare generic
that Chromium rebinds to a Chinese face under `lang="zh-Hans"`. Task 6 repaired it by naming a face,
and that is the rollout's only whole-page face-naming repair.

**F5 restated beside the table, and it is now a measured claim about this repair rather than an
inherited warning.** A zero in this table is a claim about the QUERY, not about the machine. Task 6
proved it through CDP: `system_profiler` reports `Times` **0** and `Times New Roman` **4**, while
Chromium resolves the bare `serif` to **`Times`** and names it that through
`CSS.getPlatformFontsForNode`. **Naming `Times New Roman` first — which this table alone would have
licensed — would have moved both Latin arms onto a different face while "fixing" the Chinese one.**
The table is evidence of presence and is never evidence of absence.

---

## 4. The installed bundle table (the dev↔release orphan check)

`ls ~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/`, re-derived at
close-out:

| plugin | VST3 | Component | unsuffixed alternate variant? |
|---|---|---|---|
| O-simpleAdditive | `O-simpleAdditive-dev.vst3` | `O-simpleAdditive-dev.component` | **none** |
| O-simpleGrain | `O-simpleGrain-dev.vst3` | `O-simpleGrain-dev.component` | **none** |
| O-simpleSampler | `O-simpleSampler-dev.vst3` | `O-simpleSampler-dev.component` | **none** |
| O-simpleSubtractive | `O-simpleSubtractive-dev.vst3` | `O-simpleSubtractive-dev.component` | **none** |
| O-Polystutter | `O-Polystutter-dev.vst3` | `O-Polystutter-dev.component` | **none** |
| O-Orbit | `O-Orbit-dev.vst3` | `O-Orbit-dev.component` | **none** |

**Twelve bundles, all `-dev`, no orphan.** No build in Tasks 1–6 emitted a
`⚠ Sweeping ALTERNATE-variant` warning — every one of the six task summaries records the grep count
as **0** explicitly (T1 §"Build and install", T2, T3 §K4, T4, T5, T6 alike). Nothing on disk can pin
Logic's registry slot for any of the six.

**4f D9 re-confirmed off this run's `auval -a`.** The two O-Contrabass orphans are still present and
are still NOT variant shadowing — three distinct triples and three distinct product names:

```
aumu OCb5 OuDv  -  O-Contrabass-pre-2-5-dev
aumu OCbP OuDv  -  O-Contrabass-pre-port
aumu OCbs OuDv  -  O-Contrabass-dev
```

`build-and-install.sh`'s Phase 4 sweep is keyed on `<Name>` and `<Name>-dev` only and **structurally
cannot see a third product name**, so they persist until removed by hand. Not this wave's plugins,
not touched.

---

## 5. PLUGINS.md — one commit, six rows, one status cell

**Commit `177d0c7a`, touching `PLUGINS.md` and nothing else** (`git show --name-only` returns one
path; the non-`PLUGINS.md` count is 0).

| plugin | row as found | CMakeLists **before** the wave | ships | status cell |
|---|---|---|---|---|
| O-simpleAdditive | 1.2.0 | **1.2.1** | **1.3.0** | 📦 unchanged |
| O-simpleGrain | 1.4.2 | **1.4.3** | **1.5.0** | 📦 unchanged |
| **O-simpleSampler** | 1.4.3 | **1.4.4** | **1.5.0** | **✅ Working → 📦 Installed** |
| O-simpleSubtractive | 1.4.0 | **1.4.1** | **1.5.0** | 📦 unchanged |
| O-Polystutter | 1.14.2 | **1.14.3** | **1.15.0** | 📦 unchanged |
| O-Orbit | 1.2.2 | **1.2.3** | **1.3.0** | 📦 unchanged |

**These are not six typos.** Every one of the six rows already disagreed with its own
`CMakeLists.txt` **before this wave started**, every one by exactly one patch, and all six from the
same 2026-09-03 suite-wide French hover-help rename (task `260903-ukp`) — **the same commit that left
O-GrainScatter's row two minors behind in wave 4f**. This is the registry's **second consecutive
whole-suite miss by one mechanism**: a suite-wide edit bumps every CMakeLists and updates no row.
The commit message says so in those words, so the six simultaneous corrections cannot read as six
typos to a later reader. **Nobody has audited the other 38 rows** — filed as a deferred item.

**O-simpleSampler's status cell was wrong about the MACHINE, not merely stale.** It read `✅ Working`
while `O-simpleSampler-dev.vst3` and `O-simpleSampler-dev.component` were both already in the system
plugin folders at planning time (Task 3 §K4 has the before/after bundle evidence). A stale version
number describes an old truth; this cell described a state the machine was never in.

**Reconciliation after the edit, two-arm CMake reader, all six:**

```
O-simpleAdditive    cmake=1.3.0   registry=1.3.0   AGREE
O-simpleGrain       cmake=1.5.0   registry=1.5.0   AGREE
O-simpleSampler     cmake=1.5.0   registry=1.5.0   AGREE
O-simpleSubtractive cmake=1.5.0   registry=1.5.0   AGREE
O-Polystutter       cmake=1.15.0  registry=1.15.0  AGREE
O-Orbit             cmake=1.3.0   registry=1.3.0   AGREE
```

The **two-arm** reader is load-bearing on O-simpleGrain, whose CMakeLists uses the
`set(OSIMPLEGRAIN_VERSION "1.5.0")` shape that a one-arm reader returns EMPTY on (K5).

**Convention followed:** PLUGINS.md carries no language or localization column — the table is
`Plugin Name | Status | Version | Type | Last Updated`. A 4f-shipped row (`O-Bowed`) was read first
and mirrored exactly: version cell to the shipped value, `Last Updated` to `2026-09-07`, status
`📦 Installed`. O-Polystutter's trailing packaged-artifact cell is preserved verbatim. **No row other
than the six was touched**, and nothing under `plugins/O-Strata/` was staged (`git status --short |
grep -c O-Strata` = 0, before and after).

### The duplicate-row check — CLAUDE.md's union-merge guard

```bash
$ grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d
```

**Baseline before the edit: empty. After the edit: empty.** Both runs recorded, because a post-edit
run with no baseline cannot distinguish "the edit was clean" from "the file was already dirty".
**Four of the six edited rows are adjacent in the file** (lines 62–65: simpleAdditive, simpleGrain,
simpleSubtractive, simpleSampler), which is precisely the case the check exists for — union merge
duplicates ADJACENT rows, not only edited ones.

### Commit hygiene

`git branch --show-current` and `git status --short` were re-checked **immediately before** staging,
not once at task start. The submodule guard fired on the staged set (`submodule guard: clean`).
`git commit -F <msgfile> -- PLUGINS.md`, options before the `--` (F14.6). Post-commit deletion check:
empty. `git tag --points-at HEAD`: **0** — no tag was created. Worktrees: **1**. Branch: `main`.

---

## 6. The N12 enumeration-form census — it HAS NO SUBJECT this wave

Stated as a measured absence, not an omission.

**Not one of the six owns a gate file.** Fired with `find`, never with a glob, because a zsh glob
that matches nothing aborts the whole command and reads as an answer:

```
O-simpleAdditive       gate files: 0
O-simpleGrain          gate files: 0
O-simpleSampler        gate files: 0
O-simpleSubtractive    gate files: 0
O-Polystutter          gate files: 0
O-Orbit                gate files: 0
```

Their `tests/` directories are **not** empty — they hold `i18n-states.json`, four C++
`render-harness/` trees, and O-Orbit's `ui-stub/generic-overrides.json` — **and no JavaScript
anywhere**.

**Therefore no enumeration form was exercised, no form-6 or form-7 verdict is available from this
wave, and no new form appeared.** Wave 4f proposed two candidates and left the verdict to be argued.
**Wave 4g contributes no evidence either way and says so**, rather than restating 4f's argument as
if it had tested it.

**No gate was invented** (wave 4c D4). `check-i18n`, `check-ui-labels`, `measure-ui` and
`boot-all-uis` were the whole instrument on all six.

### The wider two-language census — and F2's number is an UNDERCOUNT

The plan predicted **8, unchanged from 4f**. Fired with 4f's own command:

```
plugins/O-Bass/tests/ui_tip_render_check.js               2
plugins/O-Bells/tests/ui_tip_render_check.js              3
plugins/O-Chorus/tests/ui_tip_render_check.js             2
plugins/O-Emulator/tests/ui_tip_render_check.js           3
plugins/O-Freeze/tests/ui_tip_render_check.js             2
plugins/O-MicrotonalSampler/tests/ui_tip_render_check.js  2
plugins/O-ReverseDelay/tests/ui_frontend_check.js         2
plugins/O-SimpleReverb/tests/ui_tip_render_check.js       3
plugins/O-Bitrot/tests/ui-stub/juce-stub.js               1
plugins/O-Octagon/tests/ui-stub/juce-stub.js              1
plugins/O-ReverseDelay/tests/ui-stub/juce-stub.js         1
plugins/O-SpectralShaper/tests/ui-stub/juce-stub.js       2
plugins/O-Tapestop/tests/ui-stub/juce-stub.js             1
                                                     ---- 13
```

**It reads 13, not 8.** The first eight are F2's table verbatim, unchanged. **The other five are
`ui-stub/juce-stub.js` files that F2's table does not list**, and every one was last committed
**2026-08-26 – 2026-08-28** — weeks before wave 4f, so they were on the tree at 4f's close-out too.

**This is not something that moved; it is a correction to F2's count.** F2's `13 → 8` reduction
describes the eight `*_check.js` files it enumerated and silently drops the `juce-stub.js` class
that the same command returns. **None of the 13 belongs to any wave-4g plugin, none moved this
wave, and nobody has audited any of them.** Filed as a deferred item with the file list, so the next
reader inherits 13 and not 8.

---

## 7. Gate results — the whole suite, on the final tree

| gate | result |
|---|---|
| `check-i18n` repo-wide | **exit 0** — `ALL CHECKS PASS — 44 localized plugin(s)` |
| `check-i18n --plugin` × 6 | **exit 0 on all six** |
| `i18n-zh-lint` repo-wide | **exit 0** — `GATE PASSED. 0 findings across 44 plugin(s)` |
| `i18n-zh-lint --self-test` | **SELF-TEST: 10/10** |
| `i18n-fr-lint` repo-wide | **exit 0** — `plugins with findings: 0 / 44`. **No French rendering changed anywhere in the suite** |
| `i18n-zh-backtranslate` | `zh-Hans strings: **5789 of 5789 rows**`, `BELOW SHIP BAR — at reviewed:'mt' or unflagged: **0**` |
| two-language scan over every plugin with an `i18n.js` | **EMPTY** |
| plugins with **no** `i18n.js` | **EMPTY** — see §12 |
| `boot-all-uis --strict-tips` | **exit 0**, `clean: 44 / 44`, warn **0**, failed **0**, **DEAD 0 across 0 plugins**, **late 2 across 1 plugin** |
| `check-ui-labels --plugin` × 6 | **exit 0 on all six**, zh-arm no-move assertions 3 / 5 / 5 / 4 / 4 / 4 |
| `git submodule status` | ` b6fe1882… (v1.3.4)`, leading space, **unmoved**; 0 staged SAF paths across 13 commits |
| `git tag --points-at HEAD` | **0** |
| `git worktree list` | **1** |
| `git show --name-only HEAD` minus `PLUGINS.md` | **0 paths** |
| wave-relevant commits in `git log -20` | **13** (12 task + 1 batch) |

**The boot census control holds, and it is what makes the `0 DEAD` beside it evidence rather than a
number** (4e D2 / 4f D12): the two late bindings are **`O-Bells #ref-pitch-knob, #octave-stretch`** —
2 before this wave and 2 after, named, on the plugin they have always been on.

**Three repo-wide totals moved, and their arithmetic reconciles to O-Strata alone:**

| reading | plan predicted | measured | delta | O-Strata's own |
|---|---|---|---|---|
| `clean:` | 43 / 43 | **44 / 44** | +1 | 1 plugin |
| rendered text-bearing elements | 3842 | **4701** | +859 | **859** |
| `aria-label` | 798 | **804** | +6 | **6** |

`3842 + 859 = 4701` and `798 + 6 = 804` **exactly**. The only delta in the whole-suite boot census
is the plugin another session added. `title: 0` unchanged.

---

## 8. Fonts and geometry — the batch view

| plugin | `undeclared-font` after | Han-bearing nodes screened | `line-height-normal` residuals | `wrap-count` | `svg-font-attr` | leaves ESTIMATED |
|---|---|---|---|---|---|---|
| O-simpleAdditive | **0** | 111 | 1 (`#help-toggle`, enH==zhH==24) | 0 | 0 | 84 → **62** of 102 |
| O-simpleGrain | **0** | 100 | 2 (both `enH == zhH`) | **3** | 0 | 70 → **37** of 90 |
| O-simpleSampler | **0** | 95 | 1 (`#help-toggle`, enH==frH==zhH==24) | 0 | 0 | 58 → **33** of 81 |
| O-simpleSubtractive | **0** | 104 | 5 (all `enH == zhH`) | **1** (baseline 1, K9) | 0 | 70 → **70**, unchanged |
| O-Polystutter | **0** | — | 84, all `enH == zhH` | 0 | 0 | — |
| O-Orbit | **0** | 62 | 1 | 0 | 0 | — |

**Every zero after the table landed is a measurement. Every zero before it was a vacuum** (K12) —
five of the six baselines were `0` only because the screen's input was EMPTY on a two-language page.
Closing the K12 vacuum was a per-task deliverable and all six closed it.

**`#help-toggle` is one residual with one cause on three plugins** — a fixed-height form control
that cannot move and cannot be pinned to any effect. Tasks 1, 2 and 3 each named it independently.

**F8 confirmed live, four times.** The ESTIMATED denominator moved 84→62, 70→37, 58→33 **with no
pixel changing**, because the pins made previously-estimated leaves measurable. On O-simpleSubtractive
it did **not** move (70 of 92 both sides) — Task 4 recorded why, which is the reading that makes the
other three interpretable rather than alarming.

**`wrap-count` is non-zero twice and neither is a defect.** O-simpleGrain's three `span.viz-hint`
findings are `en = 2 lines, zh-Hans = 1 line` — **Chinese is shorter and fits**; the `0` baseline was
an en-vs-fr number. O-simpleSubtractive's 1 is K9's pre-existing French wrap, same node, same arm,
before and after. **The load-bearing criterion beside them is the moved count, and it is 0 on the zh
arm of all six across every walked state.**

**O-Orbit is the wave's one whole-page font repair** and the only place a face was named rather than
a tail appended. See §3.

---

## 9. The false-prediction ledger

Wave 4d produced **3**. Wave 4e produced **14**. s71 produced **19**. Wave 4f produced **41**
(38 across five tasks plus 3 at batch level). **This wave produced 29 across Tasks 1–6, plus 8 at
batch level = 37.**

**The count is itself the finding, and after five waves the shape has changed once.** Four waves ran
with the miss concentrated in *what the GATES will say and where the LINES are*. **This wave's
largest single cause is neither: it is a CONCURRENT SESSION.** Nine of the 37 trace to commit
`0e838512` and its two follow-ups, which landed a fully-localized 44th plugin in the middle of a
wave whose entire close-out arithmetic was pinned to that plugin being absent.

### 9.1 The per-task totals, recounted from the six summaries

| task | plugin | count | of which recurring template misses |
|---|---|---|---|
| 1 | O-simpleAdditive | **3** | O-Strata count · R3 denominator |
| 2 | O-simpleGrain | **4** | O-Strata count · positive control · R3 denominator |
| 3 | O-simpleSampler | **5** | O-Strata count · positive control · R3 denominator |
| 4 | O-simpleSubtractive | **4** | O-Strata count · positive control · R3 denominator |
| 5 | O-Polystutter | **7** | O-Strata count · positive control · R3 denominator |
| 6 | O-Orbit | **6** | O-Strata count · positive control · R3 denominator |
| | **total** | **29** | |

**Three of them are the same three on every task, and they are properties of the PLAN TEMPLATE, not
of any plugin:**

1. **The `git status --short | grep -c "O-Strata"` precondition.** The plan says 2. Readings across
   one working period: **2** (plan) → **0** (T1–T4) → **4** (T4 self-check, T5 preconditions) → **0**
   (T5 self-check, T6 both, T7 both). **Seven readings, four values, no regression in any of them** —
   the number describes another session's progress on an unrelated plugin. An inherited `git status`
   line goes stale in hours and must never be treated as a blocker.
2. **The positive Han control CANNOT fire at preconditions.** Pre-work `js/i18n.js` holds **0** Han
   code points on every plugin, so a control fired against it necessarily prints nothing and a green
   there means the probe is broken. **Its correct home is the VERIFY step**, and there it fired on
   all six. Tasks 2 and 5 additionally proved the probe live against a shipped three-language file
   (`plugins/O-Detune/.../i18n.js` → `control fired`) rather than asserting it.
3. **The R3 glossary-match denominator**, wrong on all six in the same direction: 22→**50**,
   27→**76**, 23→**69**, 16→**41**, 34→**69**, 28→**65**. The planning screen matched captions only;
   the executors' screens pushed tooltip titles through `TERMS` as well. **Every verdict was
   identical (NONE, after excluding same-control and same-English pairs); only the divisor differed.**

**And a fourth that appeared on four of six:** the emitter's **product-name control does not exist**
in this version of `scripts/i18n-zh-backtranslate.js` (`grep -cin "product"` returns **0**). Four
tasks were told to read a line that is never printed; all four adjudicated by hand instead and
recorded 0 wordmark occurrences each.

### 9.2 New at batch level — eight, every one measured in Task 7's own work

1. **The corpus does not close at `43 of 43` / `5441 of 5441`. It closes at `44 of 44` / `5789 of
   5789`.** Attributed to `0e838512`. §12.
2. **The verify line predicting exactly one `NO i18n.js:` row reading O-Strata printed ZERO rows.**
   Every one of the 44 plugin directories now carries an `i18n.js`. The 44th is no longer excluded
   from anything.
3. **The K2 / D5 baseline of `exit 2` on both repo-wide lints is GONE.** Both exit **0**, with
   **0 findings across 44** and **0 unreadable**. Confirms Task 5's FALSE-6 and Task 6's FALSE-5 at
   batch level. **`exit 2` must not be inherited as green by any future task.**
4. **`boot-all-uis` predicted `clean: 43/43`, 3842 text-bearing, 798 aria-label.** Measured
   **44/44, 4701, 804** — and the deltas are O-Strata's own 859 and 6 exactly (§7).
5. **The wider two-language gate-file census predicted 8. It reads 13.** F2's table undercounts the
   same census by five `ui-stub/juce-stub.js` files, all last touched before wave 4f (§6).
6. **The auval type split is not "four instruments and two effects".** Four `aumu`, one `aumf`
   (O-Polystutter), one `aufx` (O-Orbit) — three type codes, not two (§2).
7. **The prediction that a parameter warning on any of these six would be a real finding.** One
   fired, on O-simpleAdditive's `Scan LFO Rate`. It is genuinely new information relative to D10 —
   and it is **pre-existing**, a skewed-range normalization round-trip on a file this wave never
   opened (§2). *"New information"* and *"caused by this wave"* are different claims and the plan
   text conflates them.
8. **The Task 7 precondition `git status --short | grep -c "O-Strata"` → 2.** It reads **0**, the
   seventh reading of the line in this wave (§9.1 item 1).

### 9.3 The candidates the plan flagged in advance — every one resolved

| # | prediction | verdict | evidence |
|---|---|---|---|
| 1 | naming a face on O-Orbit does not move the en and fr arms | **HELD** | Task 6 §2 — probed through CDP because `system_profiler` could not settle it |
| 2 | `undeclared-font` reports 0 on all six | **HELD, and it is partly a blindness** | 0 on all six against 62–111 Han-bearing nodes each; F12 stands — the screen cannot see a collapsed `<select>`'s Han |
| 3 | O-simpleSubtractive's `wrap-count` stays at exactly 1 | **HELD** | same node, same arm, before and after (Task 4 §K9) |
| 4 | O-Polystutter's emitter count stays at 133, not 134 | **HELD** | 133 live at close-out; the body-less entry was mirrored, not filled |
| 5 | the R3 screen's NONE holds after authoring | **HELD on all six**, after excluding same-control and same-English pairs — but the **denominator was wrong on all six** (§9.1) |
| 6 | the six PLUGINS.md rows are the only stale ones | **UNTESTED — and now the second consecutive whole-suite miss.** The other 38 rows are unaudited. Deferred |
| 7 | every raw line number cited in K1–K16 | **verified by PATTERN, never by number**, per instruction 11 — no task cited a raw line as a worklist |

---

## 10. The blind reverse read — batch accounting

| | T1 | T2 | T3 | T4 | T5 | T6 | **batch** |
|---|---|---|---|---|---|---|---|
| rows dispatched | 131 | 153 | 145 | 133 | 133 | 125 | **820 of 820** |
| rows withheld from review | 0 | 0 | 0 | 0 | 0 | 0 | **0** |
| chunks (concept split, F17) | 2 | 2 | 2 | 2 | 2 | 2 | **12** |
| ids shared between chunks | 0 | 0 | 0 | 0 | 0 | 0 | **0** |
| ids identical AND IN ORDER (M12) | ✔ | ✔ | ✔ | ✔ | ✔ | ✔ | **all** |
| Han surviving into returned English | 0 | 0 | 0 | 0 | 0 | 0 | **0** |
| malformed returned rows | 0 | 0 | 0 | 0 | 0 | 0 | **0** |
| dispatched zh column vs COMMITTED tree | byte-identical on all six |||||| **820 of 820** |
| triples read with `--verbose` (R2) | 131 | 153 | 145 | 133 | 133 | 125 | **820 of 820** |
| correction rounds | 2 | 2 | 2 | 2 | **1** | 2 | **11** |
| **rows re-authored** | 4 | 1 | **0** | 1 | **0** | 1 | **7** |
| refusal controls fired | 2/2 | 2/2 | 2/2 | 2/2 | 2/2 | 2/2 | **12 of 12** |
| `termNote` exemptions | 6 | 7 | 7 | 1 | 1 | **0** | **22** |

**Models actually run — read back from the process, never from the alias (F15):**

- **forward:** `claude-opus-5` on all six.
- **round-1 reverse:** `claude-sonnet-5` on all six.
- **round-2 reverse:** `claude-haiku-4-5-20251001` on **five** — a genuine third model, fresh salt,
  fresh session (R4). **Task 5 ran one round only**, because round 1 corrected nothing and the rule
  is that a round exists to correct something, not to be counted.

**Seven rows re-authored out of 820 — 0.85%.** Every re-authoring was decided on a **collision on
the page**, never on drift distance:

- `label.waveformHint` (T1) — a plural hint against a singular caption on its own row.
- **the `Organ` lesson, 3 entries** (T1) — the glossary root is the *pipe*-organ word; the entry's
  own body names a Hammond drawbar registration. Corrected to *electronic organ*.
- `颗粒` (T2) — `Grain` heading and `Grains` readout painting the same two characters for two
  different things, because Chinese does not inflect for number.
- one on T4 and one (2 strings) on T6, both page collisions.

**Every `termNote` was written with its corpus site count checked FIRST (N4)** — D1's general lesson,
applied. On T1's two divergences the site count was **one** in both cases: `Morph Pad` and `Organ`
are shipped on exactly one plugin each and were derived from a single caption with nothing to check
them against. Those two, plus T2's `granular fire`, T2/T3's `load your own` and T4's `sub`, are all
filed as **glossary-level** items rather than forked silently.

**Dispatch hygiene, on all six:** cwd **outside** the repository, `--allowed-tools ""`,
`CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1`, repository access forbidden **in the prompt text
itself**, `--emit X --plugin X` together (F16), `--forward-provenance` passed a real STRING (F9), and
an explicit `--manifest` at ingest.

**Both refusal controls fired on every task, and the ORDER matters (F10):** control 2 only reaches
its own refusal (`WRONG MANIFEST FOR THIS BATCH: all N returned ids are unjoinable, not one`) because
the reverse provenance was supplied; run without it, it refuses for reason 1 and proves nothing about
the manifest.

---

## 11. Answers to every carry-forward ledger, by number

### 11.1 Wave 4f — D1 to D12

| # | item | disposition |
|---|---|---|
| **D1** | `'dist lpf': ['失真低通']` should be `['距离低通']` | **OPEN, unchanged, and now the oldest item in the ledger.** Not this wave's plugins. **But its GENERAL LESSON was applied on all six**: every termNote written this wave had its corpus site count checked first, and that check found **five more single-sited roots** (§10) |
| **D2** | O-Reed's 20 missing / 7 dead native functions | **NOT INHERITED.** O-Reed is not in this wave; no task opened it. Unchanged |
| **D3** | two stale `d848337a` floors on O-Reed | **NOT INHERITED**, same reason. Unchanged |
| **D4** | the 17-of-37 shared-module coverage hole | **EXPLICITLY NOT INHERITED.** None of the six is a `scala-tuning-engine` consumer — verified two ways on all six (CMake embed grep, and absence of any tuning tab / `tuning-panel.js`). No `1.11` ratio block, no three-button CJK tail, no `d848337a` floors, no coverage hole. Unchanged at 20 visible / 10 never-visible / 7 never in DOM |
| **D5** | `plugins/O-Strata/` makes both repo-wide lints exit 2 | **CLOSED — but not by this wave.** O-Strata gained an `i18n.js` via `0e838512`. Both lints now exit **0** with **0 unreadable**. D5's own lesson survives intact and was proved again: the load-bearing baseline was always the **finding count and the unreadable count**, never the exit code and never the `git status` line — which is why no task treated the changed exit code as a failure |
| **D6** | the glossary divergence report — SIX entries | **STILL SIX. This wave added no seventh.** Task 5 considered `Rolloff` and deliberately did not fork it. The wave's 22 termNotes are **per-page sense exemptions against correct roots**, which is a different thing from a divergence. §11.4 |
| **D7** | the Z6 budget backfill | **UNCHANGED at `3 of 552`; 549 UNBUDGETED.** Re-fired live at close-out. **This wave added none**, for the predicted reason: every geometry finding was a line-box growth wanting a ratio, or a shrink an existing floor already absorbed. Neither wants a character budget. §11.4 |
| **D8** | `modules/registry.yaml` `used_by` stale | **UNCHANGED and DRIVEN FROM NOTHING.** No task read it. The consumer question was answered from the CMake grep in every case. = s71 D3 |
| **D9** | the two O-Contrabass orphan bundles | **RE-CONFIRMED off this wave's own `auval -a`** — three distinct triples, three distinct product names, not variant shadowing. Left as found. §4 |
| **D10** | two pre-existing auval warnings (O-Bowed, O-Wind) | **NOT TRANSPLANTED.** Neither belongs to any of the six. One warning did appear on O-simpleAdditive and is reported as new information and separately adjudicated as pre-existing. §2 |
| **D11** | `reviewed: 'native'` open on every row | **STILL OPEN, now on all 5789 corpus rows** including this wave's 820. A disclosed quality level printed by lint rule R1 on every run and stated in all six CHANGELOGs. **A blocker for nothing** |
| **D12** | re-inherited unchanged | **4e D2** — O-Bells' two late bindings: **used as this wave's boot census control, 2 before and 2 after** (§7). **4e D7** — the `routing-label` French wrap: **RE-SCOPED, see §11.3.** **4c D4** — O-Chorus / O-Gain / O-IntonationPad: not this wave's plugins, unchanged. **s71 D2** — **EXTENDED, see §11.4.** **s71 D4** (O-MicrotonalSampler) and **s71 D6** (`.interval-list-header`): not this wave's plugins, unchanged |

### 11.2 Wave 4f — the thirteen "Explicitly for wave 4g" instructions

| # | instruction | outcome |
|---|---|---|
| 1 | none of the six carries a gate file; the N12 census has no subject; invent no gate | **CONFIRMED live with `find`, 0 on all six.** No gate invented. §6 |
| 2 | run the submodule guard before every commit; never stage a path under SAF | **DONE on every commit including this one.** Submodule at `b6fe1882`, 0 staged SAF paths across 13 commits |
| 3 | none of the six is a `scala-tuning-engine` consumer; the 37-row column is DONE | **CONFIRMED two ways on all six.** 4f D4 explicitly not inherited |
| 4 | re-derive the installed-family table anyway | **DONE — unchanged for a fourth consecutive wave**, with F5 restated and now proved through CDP. §3 |
| 5 | use the two-arm CMake reader unconditionally; 4f found no `set()`-variable form | **The report that the shape was ABSENT is FALSE here** — O-simpleGrain uses it and the one-arm reader returns EMPTY on it (K5, Task 2). The two-arm reader was used unconditionally, including in Task 7's reconciliation |
| 6 | measure the pin surface; never inherit a table; remove a floor before measuring the box it floors (F7) | **DONE on all six.** Tasks 5 and 6 removed every floor, measured, and restored — R8/F7 done properly rather than described |
| 7 | fire every precondition you assert (M3) | **All seven tasks fired all of theirs**, which is exactly why the seven readings of the O-Strata line exist to be reported |
| 8 | ids identical AND IN ORDER (M12); both refusal controls (F10); `--forward-provenance` a STRING (F9) | **DONE on all six.** 12 of 12 refusal controls fired. §10 |
| 9 | chunk on the concept split (F17); `--emit X --plugin X` together (F16) | **DONE on all six** — 2 chunks each, 12 total, 0 shared ids in every pairing |
| 10 | report every FALSE prediction; the count is the finding | **DONE — 37.** §9 |
| 11 | never cite a raw line number as a worklist; verify by pattern | **DONE.** No task drove work off a raw line number |
| 12 | `auval -v` takes THREE UNQUOTED WORDS; read triples off `auval -a`; budget ~85 s | **DONE.** Six runs, three unquoted words each. **104.6 s measured** — the right order, ~20 s above the band, consistent with a registry that gained a plugin. §2 |
| 13 | re-inherit D1–D12 | **DONE by number.** §11.1 |

### 11.3 Wave 4e — D4, D5, D7, M14, N11

- **4e D4 (glossary divergence report, SIX entries).** **Did this wave add a seventh? NO.** The
  register stands at six: `WebGL 不可用` vs `不支持 WebGL`; Scatter/Diffusion; `通过长度` for *pass
  length*; `分割` for *Divisions*; `键位` for *keyswitch*; `调制` for *MOD*. **The R3 page-collision
  screen fired on all six plugins before authoring and returned NONE on all six after excluding
  same-control and same-English pairs**, and the mechanical downstream check fired again after
  authoring with the same verdict. The wave's 22 termNotes are per-page sense exemptions against
  correct roots — five of them against roots that are single-sited in the corpus and therefore had
  nothing to check themselves against, which is D1's shape and is filed as such.
- **4e D5 / 4f D7 (the Z6 budget backfill).** **Did this wave add any? NO.** Re-fired live at
  close-out: `Z6 coverage: 3 of 552 glossary terms carry a measured budget; 549 are UNBUDGETED`.
  Unchanged, and the reason is the predicted one.
- **4e D7 / 4f D12 (`div.routing-label` French wrap).** **RE-SCOPED. The attribution to O-simpleFM is
  wrong — it is a property of the CLASS.** Task 4 measured five files across **three** plugins
  carrying it: **O-simpleFM** (markup + CSS), **O-simpleSubtractive** (markup + CSS — *in this
  wave's own set*) and **O-simpleGrain** (CSS only, no markup carrier). Both markup carriers wrap in
  French; neither wraps in English or Chinese. **O-simpleSubtractive's `wrap-count` baseline was
  therefore 1, not 0**, and holding it at 1 was the correct criterion.
- **4e M14 (`auval -v` takes three unquoted words).** **Honoured on all six.** No run produced
  `Invalid Arguments: auval`, which across six plugins would read exactly like six validation
  failures.
- **4e N11 (the installed-family list, M5).** **Re-derived, unchanged.** §3.

### 11.4 s71 — D2, D3, D11

- **s71 D2 (module JS files `check-i18n` does not scan) — EXTENDED by this wave, not merely
  inherited (K8).** Four unscanned module JS files were found on this wave's **own** plugins:

  | file | carriers | strings | state |
  |---|---|---|---|
  | `modules/webview-drop-streaming.js` | O-simpleGrain, O-simpleSampler | **17 distinct, 22 `opts.showToast(` call sites** that render **English on the Chinese page** | **byte-identical** copies, sha256 `349a0c28…`, re-fired on both |
  | `modules/preset-manager.js` | O-Polystutter (406 L, sha1 `0142c1da…`) · O-Orbit (447 L, sha1 `d15e751b…`, **two copies in its own tree**) | 7 user-facing English strings, mostly dead paths, but `'Loaded Preset'` **does** reach both pages | **DIVERGENT** |

  **The new fact s71 D2 does not contain: the `preset-manager.js` copies have FORKED.** s71's table
  treats it as one file with eighteen consumers; it is at least two files, 41 lines apart, and
  O-Orbit carries two copies of its own version — one embedded per `CMakeLists.txt:43-48` and one
  never served.

  **The blindness is DIRECTORY-SCOPED, and that is the root cause.** `check-i18n`'s `[12]` clause
  scans `js/app.js` and `js/parameter-bindings.js` and reports PASS on the same plugin whose
  `modules/preset-manager.js` it never opens. **It is blind on all 44 plugins, not just these four.**

  **Scope-out reason, written down:** keying an unkeyed toast layer needs a calibrated gate that
  does not exist, the fix must land on every forked copy at once, and shipping it inside a commit
  whose subject is a caption table would put four unrelated plugins' UIs in one change. Widening the
  `[12]` scan past the `js/` directory is the root fix and is a rollout of its own.
- **s71 D3 (`modules/registry.yaml` `used_by` stale).** **Driven from nothing.** No task in this wave
  read it. Every consumer question was answered from the CMake grep. It still names O-Bells,
  O-Formant and O-IntonationPad (forks with private copies, none live consumers) and still omits
  O-Reed, O-Wind and O-MicrotonalSampler. Unchanged, and nothing should be driven from it until it
  is rebuilt.
- **s71 D11 (the corrected auval budget and the pre-existing warnings).** **The budget correction is
  confirmed a third time** — 104.6 s, not fifteen minutes. **The pre-existing-warning discipline was
  applied and produced a result:** one warning fired, it was NOT attributed to D10's plugins, and it
  was adjudicated on its own evidence rather than waved through. §2.

---

## 12. The corpus, recomputed LIVE at close-out

**Read live on the final tree at 2026-09-07 22:5x local. Nothing below is carried forward.**

```
$ node scripts/i18n-zh-backtranslate.js
  plugins: 44
  zh-Hans strings: 5789 of 5789 rows
  BELOW SHIP BAR — at reviewed:'mt' or unflagged: 0. The ship bar is 'bt'.

$ node scripts/check-i18n.js
  ALL CHECKS PASS — 44 localized plugin(s)                       exit 0

$ node scripts/i18n-zh-lint.js
  GATE PASSED — exit 0. 0 findings across 44 plugin(s).

$ find plugins -name i18n.js -not -path '*/libs/*' -not -path '*/build/*' | wc -l
  44                                        # against 44 plugin directories

$ for d in plugins/*/; do … grep -q "zh-Hans" "$f" || echo "STILL TWO-LANGUAGE: $p"; done
  (empty)

$ for d in plugins/*/; do … [ -z "$f" ] && echo "NO i18n.js: $p"; done
  (empty)
```

The six wave-4g rows off the emitter's own `rows` column, every one at `bt`, 0 `mt`, 0 native, 0 none:

```
  O-simpleAdditive    131  131   0 mt  131 bt
  O-simpleGrain       153  153   0 mt  153 bt
  O-simpleSampler     145  145   0 mt  145 bt
  O-simpleSubtractive 133  133   0 mt  133 bt
  O-Polystutter       133  133   0 mt  133 bt
  O-Orbit             125  125   0 mt  125 bt
                      ---
                      820
```

### The assertion — what was asked for, and what was measured

The task asked for **43 of 43**, on the premise that O-Strata was excluded for having no `i18n.js`.
Both readings, stated side by side, because they answer different questions:

**(a) What THIS WAVE delivered.** The six plugins shipped **exactly their 820 rows** — 131 + 153 +
145 + 133 + 133 + 125, every per-plugin figure matching the carry-forward — and took the 43-plugin
set the task named from **37 → 43 localized**. `4621 + 820 = 5441`, which is precisely the number the
plan predicted for that 43-plugin set. **On its own terms the assertion the wave was asked for
HOLDS: 43 of 43, 5441 of 5441 within the set that existed when the plan was written.**

**(b) What the CORPUS now reads.** **44 of 44 plugins and 5789 of 5789 rows.** The difference is
**O-Strata's 348 rows**, and `4621 + 348 + 820 = 5789` **exactly**. O-Strata arrived during this wave
as commit `0e838512` — *"feat(O-Strata): Stage 1 foundation — fork of O-Prism v1.24.0"* — a fork of
an already three-language plugin, so it came with a complete `i18n.js` at `reviewed: 'bt'` without
any localization work being done on it. **This wave is credited with none of those 348 rows.**

**The K1 convergence still happened; it just converged on 44.** `check-i18n`'s denominator counts
plugins that have an `i18n.js` at all and read **43** on every wave since Stage 3 — including before
this one, which is why quoting it alone proves nothing. The emitter's denominator counts plugins
carrying at least one zh row and is the one that moved: **37 → 43 by this wave's work, and → 44 by
another session's fork.** The two now read the same number. **Their convergence is the closure.**

**Both denominators, at each task's own close:** 37 → 38 (T1) → 39 (T2) → 40 (T3) → 41 (T4) → 43
(T5, +O-Strata) → 44 (T6). Task 5 watched the total denominator move **5816 → 5789 within one hour**
as the other session trimmed O-Strata's table. **Any close-out arithmetic pinned to a constant would
have been wrong by the time it was written**, which is why every number in this section carries the
command that produced it.

**Plugins still two-language after this wave: NONE.** Fired as its own per-plugin scan rather than
inferred from the totals, and it returned empty. That is the only reading that permits *"the rollout
is done."*

**The 44th plugin is no longer excluded from anything.** The plan's instruction — *name O-Strata,
state that it has no `i18n.js` under either UI root, and record it as the standing cause of both
lints' exit 2* — describes a tree that no longer exists. O-Strata **has** an `i18n.js`, carries **348
zh rows all at `'bt'`**, is readable by every gate, and **both repo-wide lints now exit 0 with 0
unreadable**. `43 of 43`, `5441`, `37 carrying zh` and `exit 2` are all dead numbers and are recorded
here only so a later reader can see why they died.

**O-Strata's tree was never touched by this wave.** `git status --short | grep -c "O-Strata"` reads
**0** before and after the PLUGINS.md commit, nothing under `plugins/O-Strata/` was staged in any of
the thirteen commits, and its PLUGINS.md row was not edited.

**The zh-Hans rollout is COMPLETE.**

---

## 13. Deferred

Full ledger in `deferred-items.md` beside this file: **19 items**, of which 9 are new this wave and
10 are re-inherited unchanged, plus an "Explicitly for the next wave" section that is now short,
because the volume work is done.

The five that matter most:

- **D1 (4f)** — `scripts/i18n-zh-glossary.js:433` `'dist lpf': ['失真低通']` should read `['距离低通']`.
  Still the oldest open corpus-level item. **And this wave found FIVE MORE single-sited roots of the
  same shape** — `Morph Pad`, `Organ`, `granular fire`, `load your own`, `sub` — each corrected
  per-entry with a reasoned termNote rather than forked.
- **The unscanned-module class**, now measured on **four of six** wave-4g plugins, with the
  `preset-manager.js` copies proved **FORKED** and the `check-i18n` `[12]` blindness proved
  **directory-scoped** and therefore repo-wide (s71 D2, extended).
- **The frozen render-harness version literals** — three of the wave's plugins carry one
  (`1.0.2`, `0.1.0`, `1.0.0`), which makes a suite-wide sweep a measurement rather than a guess.
- **F2's count is 13, not 8**, and none of the thirteen has ever been audited.
- **The PLUGINS.md whole-suite miss is now a PATTERN, twice in a row.** The other 38 rows are
  unaudited.

---

## Self-Check: PASSED

**Files:**

- `PLUGINS.md` FOUND — six rows corrected, one status cell flipped, dup-check empty before and after.
- `.planning/quick/260907-qda-…/260907-qda-SUMMARY.md` FOUND.
- `.planning/quick/260907-qda-…/deferred-items.md` FOUND.
- All six task summaries FOUND (`260907-qda-TASK1-SUMMARY.md` … `TASK6-SUMMARY.md`).

**Commits** — all thirteen verified present with `git log --oneline --all | grep -q`:

`e130f2b9` · `663bdd07` · `8ee82be2` · `8b5d7a70` · `c5effaac` · `b1d9e6b2` · `fff62ae8` ·
`09bb37cf` · `ad264e7e` · `ce5a6a8c` · `0efb343e` · `7ae50ad8` · `177d0c7a`

**Guards:** submodule at `b6fe1882` (v1.3.4), 0 staged SAF paths across all thirteen commits · 0 tags
created · branch `main` · 1 worktree · `HEAD` touches `PLUGINS.md` and nothing else · nothing under
`plugins/O-Strata/` or `.claude/agent-memory/` staged at any point.
