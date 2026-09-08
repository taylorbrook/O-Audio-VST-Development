---
phase: quick-260907-ja8
plan: 01
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4f, batch-close, O-GrainScatter, O-Wind, O-Contrabass, O-Reed, O-Bowed, scala-tuning-engine, enumeration-census, auval, PLUGINS.md]
status: complete
requirements: [ZH4F-01, ZH4F-02, ZH4F-03, ZH4F-04, ZH4F-05, ZH4F-06, ZH4F-07, ZH4F-08, ZH4F-09, ZH4F-10, ZH4F-11, ZH4F-12]
dependency_graph:
  requires:
    - "260906-s71 (shared scala-tuning-engine localized, d848337a — the 37-key column this wave copies)"
    - "260906-h8y (wave 4e — the carry-forward sheet this wave answers)"
    - "260904-g5l (the corpus emitter and the inventory grep this wave corrects)"
  provides:
    - "O-GrainScatter 2.8.0, O-Wind 1.21.0, O-Contrabass 1.10.0, O-Reed 1.6.0, O-Bowed 1.9.0 — three-language UI at reviewed:'bt', built, installed, auval-clean"
    - "PLUGINS.md rows for all five, agreeing with their CMakeLists"
    - "the corrected N12 enumeration inventory METHOD and the wave-4g carry-forward sheet"
  affects:
    - "wave 4g (O-Orbit, O-Polystutter, O-simpleAdditive, O-simpleGrain, O-simpleSampler, O-simpleSubtractive — the last six, 820 rows)"
tech_stack:
  added: []
  patterns:
    - "derive-or-abort over a table-exported LANGUAGES list, with a SECOND independent guard on the walk"
    - "a paired literal-argument sweep call generalized to a loop over the derived list"
    - "CJK tail before the trailing generic, and INSIDE an SVG font-family presentation attribute"
    - "unitless line-height ratio derived from the CONTENT box, per line and per size, with form controls named separately"
    - "min-width floor at the exact measured English box, taken with the existing floor REMOVED"
    - "a .tuning-panel override that moves a consumer off the shared module's -apple-system stack, chosen on a measured Latin arm"
    - "blind reverse read at reviewed:'bt' with per-chunk salt and caption/title split across readers"
key_files:
  created:
    - .planning/quick/260907-ja8-stage-4-wave-4f-of-the-zh-hans-rollout-f/deferred-items.md
    - .planning/quick/260907-ja8-stage-4-wave-4f-of-the-zh-hans-rollout-f/260907-ja8-SUMMARY.md
  modified:
    - PLUGINS.md
    - plugins/O-GrainScatter/Source/ui/public/js/i18n.js
    - plugins/O-Wind/Resources/ui/js/i18n.js
    - plugins/O-Contrabass/Source/ui/public/js/i18n.js
    - plugins/O-Reed/Resources/ui/js/i18n.js
    - plugins/O-Bowed/Resources/ui/js/i18n.js
    - plugins/O-GrainScatter/tests/ui_tip_render_check.js
    - plugins/O-Wind/tests/ui_tip_render_check.js
    - plugins/O-Contrabass/tests/ui_frontend_check.js
    - plugins/O-Reed/tests/ui_tip_render_check.js
    - plugins/O-Bowed/tests/ui_tip_render_check.js
decisions:
  - "The 37 shared scala-tuning-engine rows were COPIED byte-for-byte from O-Bassoon at reviewed:'bt' on all four consumers and excluded from every blind batch — R1's one stated exception, and the only way to avoid the rollout's first divergence on a surface s71 proved byte-identical across five consumers."
  - "The g5l inventory grep's false-negative class is reported as a correction to the METHOD, not only to the count: the wave owned FIVE gate files, not three."
  - "Forms 6 and 7 are both admitted as NEW enumeration forms, on the discriminator that form 5 is a data-structure read and form 7 has no data structure at all."
  - "O-Contrabass's gear-btn and lang-select bodies were read in full and deliberately LEFT — the one plugin in the wave where the J8/J9 deletion reflexes would both have been wrong."
  - "O-Reed's and O-Bowed's .tuning-panel overrides were KEPT and the d848337a floors re-measured, rather than falling back to the shared system stack: the fr movement was a floor measured against the OLD face, which is R8's exact case."
  - "The two O-Contrabass orphan bundles were LEFT as found — distinct product names with distinct AU subtypes OCb5/OCbP against OCbs, confirmed off auval -a, not dev/release variant shadowing."
metrics:
  duration: one day, six tasks
  completed: 2026-09-07
  plugins: 5
  rows_shipped: 860
  commits: 11
  tasks: 6
actuals:
  tokens: 421000
  tasks: 6
  commits: 11
---

# Wave 4f — Stage 4 of the zh-Hans rollout: five plugins to Simplified Chinese

**860 new rows.** O-GrainScatter, O-Wind, O-Contrabass, O-Reed and O-Bowed ship English, French and
Simplified Chinese at `reviewed: 'bt'` on every row. The corpus reaches **4621 zh-Hans rows of 5441
across 37 localized plugins**, `BELOW SHIP BAR 0`, and **wave 4g is the last six plugins and 820
rows** (deferred-items.md, "Explicitly for wave 4g").

Five plugin-owned gate files now render and COMPARE the Chinese page. Three no longer assert a
two-member joined language list; **two had enumerations no inventory grep has ever listed**. Every
one derives its list from the table's own export and refuses rather than guessing.

**HEAD at Task 6 start: `f25090c8`.** Branch `main`, one worktree, trunk-based throughout.

---

## 1. What shipped, per plugin

| plugin | version | rows | entries | commits | gate assertions | blind chunks | rows re-authored |
|---|---|---|---|---|---|---|---|
| **O-GrainScatter** (tracer) | 2.7.1 → **2.8.0** | 131 | 92 (39 I18N + 53 LABELS) | `b534636b` + `a92b7980` | 816 → **1054** | 2 (66+65) | 1 |
| **O-Wind** | 1.20.0 → **1.21.0** | 214 | 161 (53 + 108) | `020409f8` + `bff367f6` | 788 → **1109** | 2 (89+88) | 1 string, 2 keys |
| **O-Contrabass** | 1.9.0 → **1.10.0** | 186 | 140 (46 + 94) | `5a216066` + `b69ca8ea` | 28 → **30** | 2 (75+74) | **0** |
| **O-Reed** | 1.5.0 → **1.6.0** | 168 | 132 (36 + 96) | `9d83e7a7` + `f3737f53` | 539 → **757** | 2 (66+65) | 1 |
| **O-Bowed** | 1.8.0 → **1.9.0** | 161 | 130 (48 + 82) | `089e65e6` + `f25090c8` | 470 → **659** | 2 (62+62) | 1 |
| **batch close** | — | — | — | **`d98f369d`** (PLUGINS.md) | — | — | — |

**Eleven commits, all path-scoped**, `git commit -- <paths>` every time, branch and `git status`
re-checked immediately before each. No `git add -A`, no `git commit -a`, no tag, nothing under
`modules/`, nothing under `plugins/O-Orbit/libs/SAF`, and **0 paths matching `O-Strata` in any
commit**.

**Every I18N key count and every LABELS key count is UNCHANGED** across all five, before and after
the flip, with an empty duplicate-key scan on both sides — the only detector for s71's D8 collision
class, which did not recur.

### The 37 shared-module rows — R1's one stated exception

Four of the five are `scala-tuning-engine` consumers. On each, the **37 module keys were copied
byte-for-byte from O-Bassoon at `reviewed: 'bt'`** and excluded from the blind batch. They had
already been through a blind reverse read on O-Bassoon; re-reading them would have re-read 148 rows
the corpus has already accepted, and re-authoring them would have created the rollout's first
divergence on a surface s71 proved byte-identical across five consumers.

The blind batches were therefore 131 / **177** / **149** / **131** / **124** rows against tables of
131 / 214 / 186 / 168 / 161.

**The 37-key list was DERIVED, not transcribed**, from the module's own markup:
`grep -rhoE 'data-i18n(-aria)?="[^"]+"' modules/tuning/scala-tuning-engine/` returns **38**, and a
`label.xxx` inside a doc comment is the one to drop.

---

## 2. The cold auval sweep — ONE rescan, all five PASS

Cache cleared per CLAUDE.md (`killall -9 AudioComponentRegistrar`, both cache paths removed), then a
single `auval -a` rescan, then five `auval -v` runs with **three unquoted words** (M14). Every triple
read off `auval -a`, none assumed.

| plugin | triple | Component Version | verdict | warnings |
|---|---|---|---|---|
| **O-GrainScatter** | `aufx OuGS OuDv` | 2.8.0 (0x20800) | **AU VALIDATION SUCCEEDED** | none |
| **O-Wind** | `aumu OWnd OuDv` | 1.21.0 (0x11500) | **AU VALIDATION SUCCEEDED** | 1 — **PRE-EXISTING** |
| **O-Contrabass** | `aumu OCbs OuDv` | 1.10.0 (0x10A00) | **AU VALIDATION SUCCEEDED** | none |
| **O-Reed** | `aumu ORed OuDv` | 1.6.0 (0x10600) | **AU VALIDATION SUCCEEDED** | none |
| **O-Bowed** | `aumu OBwd OuDv` | 1.9.0 (0x10900) | **AU VALIDATION SUCCEEDED** | 1 — **PRE-EXISTING** |

**Four instruments and one effect.** O-GrainScatter is the wave's only effect — `IS_SYNTH` is absent
from its `CMakeLists.txt` while the other four declare `IS_SYNTH TRUE` — and its type is `aufx`
accordingly. The manufacturer code auval printed is **`OuDv`** on all five, the dev code; the release
code `OuAu` is CI-only. Every `Component Version` auval reported matches the plugin's CMakeLists and
the installed bundle's `CFBundleShortVersionString` independently.

**Both warnings PREDATE this wave** (s71 D11), and the reason is structural: **this wave touched no
parameter, no range, no type, no state format and no audio path** on any of the five.

- **O-Bowed** — `Parameter ID:127996179 "Bow Position"`, Generic, min 0.02 / default 0.12 / max 0.30:
  `WARNING: retrievedValue = 1.000000 (was 1.000000), Parameter did not retain maximum value when
  set`. The parameter is normalized 0–1 at the AU boundary while its declared range is 0.02–0.30;
  auval sets the declared max and reads back the normalized one. Read verbatim out of the log with
  the parameter block above it, not inferred.
- **O-Wind** — `WARNING: Source AU supports multi-channel output but does not provide a channel
  layout`, printed directly beneath its `0-1 0-2 0-4 0-5 0-6 0-7 0-8` output-channel matrix. A
  standing shape for this family.

**`Bad Max Frames - Render should fail` appears in all five logs and is the NAME OF A TEST THAT
EXPECTS A FAILURE**, not a finding. Present once in each of the five.

**Timing: `auval -a` took 83 seconds** (00:53:14 → 00:54:37) and the five `auval -v` runs together
took **1 second**. s71's D11 correction is confirmed a second time and the stale ~15-minute budget is
dead. The batch-at-the-end discipline is still right — it is one rescan instead of five — but it does
not license refusing a mid-batch check.

---

## 3. The re-derived installed-family table (M5) — UNCHANGED, and one entry is a lie

Re-derived at close-out on this machine (M5). **Identical to waves 4d and 4e and to this plan's
planning-time reading, entry for entry:**

| family | count | | family | count |
|---|---|---|---|---|
| Georgia | **4** | | Garamond | **0** |
| Times New Roman | **4** | | EB Garamond | **0** |
| Arial | **4** | | Adobe Garamond Pro | **0** |
| Menlo | **4** | | Microsoft YaHei | **0** |
| Courier New | **4** | | Consolas | **0** |
| PingFang SC | **6** | | Segoe UI Symbol | **0** |
| Songti SC | **4** | | Noto Sans Symbols2 | **0** |
| Helvetica Neue | **14** | | **Times** | **0 — AND THAT IS FALSE** |
| Arial Unicode MS | 1 | | | |
| Apple Symbols | 1 | | | |

**The arithmetic, stated as arithmetic rather than as a conclusion.** Every house stack in this wave
names Georgia and/or Times New Roman. Both are installed, four faces each. Garamond — which four of
the five stacks name FIRST — is installed nowhere, in any of its three spellings. So on every one of
these pages the Latin half was **already safe before a byte of Chinese landed**, and the only
surviving member of each stack was the trailing bare generic, which Chromium resolves against the
document's `lang`.

That is why **the tail half was nearly the whole font worklist and the face-naming half was exactly
three sites**: O-GrainScatter's two `.toggle` nodes, and the four consumers' `.tuning-panel` override
(one site each on O-Reed and O-Bowed, plus O-Contrabass's SVG plate attribute). Every other
declaration needed only `…, 'PingFang SC', 'Microsoft YaHei', <generic>` appended before the generic
(W1).

**The `Times` zero is the table's own false-negative class** (F5). `system_profiler` reports
`Family: Times` = 0 while `/System/Library/Fonts/Times.ttc` is on disk and Chromium resolves
O-GrainScatter's `.toggle` to it — measured through CDP `CSS.getPlatformFontsForNode`, not inferred.
Naming Georgia there would have been tidier and would have moved both English captions; **`Times` was
named instead, because it is the face already rendering.** The fr arm's `[7]` stayed green,
confirming it. **The table proves presence and never absence.**

---

## 4. The installed-family of BUNDLES, re-derived (the dev↔release orphan check)

Read off disk at close-out, with every version taken from the installed bundle's own
`Info.plist` — not from the build log and not from the CMakeLists.

| bundle | subtype | Info.plist version | disposition |
|---|---|---|---|
| `O-GrainScatter-dev.{vst3,component}` | `OuGS` | **2.8.0** | shipping |
| `O-Wind-dev.{vst3,component}` | `OWnd` | **1.21.0** | shipping |
| `O-Contrabass-dev.{vst3,component}` | `OCbs` | **1.10.0** | shipping |
| `O-Reed-dev.{vst3,component}` | `ORed` | **1.6.0** | shipping |
| `O-Bowed-dev.{vst3,component}` | `OBwd` | **1.9.0** | shipping |
| `O-Contrabass-pre-2-5-dev.component` | **`OCb5`** | 1.0.0 | orphan, **LEFT as found** |
| `O-Contrabass-pre-port.component` | **`OCbP`** | 1.0.0 | orphan, **LEFT as found** |

**All ten shipping bundles are `-dev` only.** The unsuffixed-orphan grep

```bash
ls ~/Library/Audio/Plug-Ins/VST3/ ~/Library/Audio/Plug-Ins/Components/ \
  | grep -cE "^(O-GrainScatter|O-Wind|O-Contrabass|O-Reed|O-Bowed)\.(vst3|component)$"
```

reads **0**, and **no build in this wave emitted `⚠ Sweeping ALTERNATE-variant`**.

**The two O-Contrabass legacy bundles do NOT share the AU triple with the current one.** Read
directly off `auval -a`, which lists three distinct O-Contrabass entries:

```
aumu OCb5 OuDv  -  Ouaricon Audio Development: O-Contrabass-pre-2-5-dev
aumu OCbP OuDv  -  Ouaricon Audio Development: O-Contrabass-pre-port
aumu OCbs OuDv  -  Ouaricon Audio Development: O-Contrabass-dev
```

Three distinct product names, three distinct subtypes. **This is not the dev↔release variant
shadowing CLAUDE.md warns about** — that failure needs two bundles at the SAME triple, and there are
none. `build-and-install.sh`'s Phase 4 sweep is keyed on `<Name>` and `<Name>-dev` only and
structurally cannot see a third product name, so it correctly did not warn and these will persist
until removed by hand.

---

## 5. PLUGINS.md — one commit, five rows, one of them by TWO minors

**Commit `d98f369d`**, `git commit -- PLUGINS.md`, one file, +5 / −5.

| plugin | row before | row after | CMakeLists | installed | auval | verdict |
|---|---|---|---|---|---|---|
| **O-GrainScatter** | **2.6.1** | **2.8.0** | 2.8.0 | 2.8.0 | 2.8.0 | **AGREE** |
| O-Wind | 1.20.0 | 1.21.0 | 1.21.0 | 1.21.0 | 1.21.0 | AGREE |
| O-Contrabass | 1.9.0 | 1.10.0 | 1.10.0 | 1.10.0 | 1.10.0 | AGREE |
| O-Reed | 1.5.0 | 1.6.0 | 1.6.0 | 1.6.0 | 1.6.0 | AGREE |
| O-Bowed | 1.8.0 | 1.9.0 | 1.9.0 | 1.9.0 | 1.9.0 | AGREE |

Version and `Last Updated` (→ 2026-09-07) only, which is exactly the shape wave 4e's row edit
(`ff0de99a`) used. The registry has five columns and no language column; there was nothing else on
these rows for a localization wave to move.

**O-GrainScatter's row moved by TWO minors and that is not a typo.** Its registry row disagreed with
its own CMakeLists **before this wave started**: the row read 2.6.1 while the CMakeLists read 2.7.1.
The plugin shipped 2.7.0 and then 2.7.1 — the latter the 2026-09-03 suite-wide French hover-help
rename, task `260903-ukp` — without the row ever following. **That stale row is where the wave's task
description got its "2.6.1 → 2.7.0" premise**, and it is the wave's most consequential false premise.
The commit message states the correction explicitly rather than letting it read as a slip.

**Every value read with the TWO-ARM CMake reader (R9), never typed.** This is the **first wave since
4c with NO `set()`-variable form**: two unquoted literals (O-GrainScatter, O-Contrabass), three
quoted (O-Wind, O-Reed, O-Bowed), no third shape. One-arm and two-arm agree on all five — and the
two-arm reader was used unconditionally anyway, including where the one-arm form also works, because
a reader that only works where it was written is not a reader. **That is the answer to wave 4e
instruction 4.**

**Duplicate-row check, CLAUDE.md's union-merge guard, run after the commit:**

```
$ grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d
$
```

**Empty output.** 44 rows, no duplicates. The post-commit deletion check
(`git diff --diff-filter=D --name-only HEAD~1 HEAD`) is also empty, and `git tag --points-at HEAD`
reads **0** — no tag was created; tags belong to `/publish`.

---

## 6. The N12 enumeration-form census — the wave verdict

Both carry-forward ledgers asked for this (wave 4e instruction 2, s71 instruction 3). Stated as a
corrected inventory, not as a count.

### 6.1 The wave owns FIVE gate files, not three — and the correction is to the METHOD

Wave 4e's N12 said *"exactly three two-language gate files remain in the repo — O-Bowed, O-Reed and
O-Wind"*. **The true answer was five.** The g5l inventory grep that has driven this rollout since
Stage 3 is blind to two shapes:

| missed file | shape | what it would have certified after the flip |
|---|---|---|
| `O-GrainScatter/tests/ui_tip_render_check.js` | a **three-element array literal** `['en','fr','en']` — the third member is a deliberate English return pass, so it does not match `\[.en., *.fr.\]` | a gate that walks two languages and calls the third a return pass |
| `O-Contrabass/tests/ui_frontend_check.js` | **per-language object property access only** — no list, no walk, no `en,fr` string anywhere | that every markup key resolves in English and French, and **nothing whatever about Chinese** |

O-Contrabass's file has **never appeared on any inventory in the rollout.** It is not a Playwright
renderer at all: it evaluates `js/i18n.js` in a `vm` sandbox and asserts over the parsed table plus
the markup. Its two enumerating sites feed real `check()` calls, so left alone it would have gone
green on a page whose Chinese column was half missing.

**Report the correction to the METHOD.** The working inventory is the wider comment-stripped census
on `["']fr["']|\.fr\b`; the g5l grep has a false-negative class that has been invisible for six
waves because until now it never cost anything.

**Re-fired at close-out, the g5l grep returns NOTHING AT ALL** — all five files are off it, and C8
kept the explanatory comments from putting them back.

### 6.2 The form census, all five plugins, before → after

| form | O-GrainScatter | O-Wind | O-Contrabass | O-Reed | O-Bowed | after | disposition |
|---|---|---|---|---|---|---|---|
| **1 — `LANGUAGES.join(',') === 'en,fr'`** | 0 | **1** (L295, tagged `[0]`) | 0 | **1** (L252) | **1** (L227) | **0** | shape assertion + derive-or-abort |
| 2 — string `'en,fr'` | 0 | 1 (inside form 1) | 0 | 1 (inside form 1) | 1 (inside form 1) | **0** | went with form 1 |
| **3 — array-literal walk** | **1** (L465) | 0 | 0 | 0 | 0 | **0** | derived; return pass preserved |
| **4 — computed / UA-face form controls** | **0** | **25** | **30** | **30** | **30** | **0** | a CSS form, §8 |
| 5 — `Map` read with a literal key | 0 | 0 | 0 | 0 | 0 | 0 | **absent from the whole wave** |
| **6 — per-language OBJECT PROPERTY access** | **4** | **5** | **3 + 0** | **4** | **5** | see 6.4 | **3 generalized, 18 LEFT and classified** |
| **7 — PAIRED LITERAL-ARGUMENT CALL** | **2** (L561-562) | **2** (L588/L717) | 0 | **2** (L480/L509) | **2** (L443/L515) | **0** | generalized to a loop |
| g5l grep saw the file? | **no** | yes | **no** | yes | yes | **none** | off the list on all five |

**Form 1 appears for the first time in the rollout** (J3). Wave 4e recorded it absent. It is present
on three of the five and it is **the only benign form in the taxonomy**: it hard-fails the moment
`LANGUAGES` gains a third member rather than going vacuously green. A gate that breaks loudly when
the table changes is not the failure mode this census exists to find — but it still had to be
repaired, and repairing it BEFORE the flip is what kept every commit in this wave's history free of a
table whose third language makes its own gate red.

**Form 5 is absent from all five.** Wave 4d's per-language `Map` lookup did not recur.

### 6.3 Did a SIXTH form appear? — the verdict

**Two candidates appeared, and both are admitted. The count is six and seven; the classification is
what matters.**

**Form 6 — per-language OBJECT PROPERTY ACCESS. ADMITTED as new, narrowly.**

*Against:* it is a data-structure read with a literal key, which is form 5's essence. `I18N[k].fr.t`
and `map.get('fr')` differ only in the structure's type.

*For, and this is the verdict:* the structure is a **plain object**, and that changes what a static
analysis can see. `map.get('fr')` is a call whose argument is a literal — greppable, and greppable by
the same alternation that finds form 7. `e.fr` is **property syntax**: it has no call site, no
argument list, and matches nothing that looks like a language list. It is the shape a gate reaches
for when it has no browser, which is precisely why it turned up on the one file in the wave that
never renders a page. **A form the inventory cannot see is a different form from one it can.**

The finding that settles it is that **form 6 appeared in BOTH its guises in one wave**:

- **ENUMERATING** — O-Contrabass, 3 sites, feeding two real `check()` calls. This is the defect.
- **RESET / RESTORE / SOURCE-scoped** — all four tip gates, 18 sites, verifying that a restore put
  the page back. This is correct code that looks identical to a grep.

A taxonomy that cannot tell those two apart produces a worklist that is 18/21 wrong.

**Form 7 — the PAIRED LITERAL-ARGUMENT CALL. ADMITTED as new, and it is this wave's real work.**

`sweep('en'); … sweep('fr');` — **the language lives in the ARGUMENT of two sibling calls whose
callee is already language-agnostic.** Every one of the four gates that carried it had
`const entry = (I18N[key] || {})[lang]` inside `sweep`, so the function was correct and the caller
was the entire defect.

*The discriminator against form 5:* **it has no data structure at all.** Form 5 reads a container
with a literal key; form 7 has no container — only a call repeated once per language, with every
piece of per-language state (`__setLanguage(l)`, the selector assertion, the printed height
comparison, `showTab('sound')`) hand-written between the two calls. It is the highest-cost form to
repair and the one most likely to leave a stage behind: on O-Wind a naive substitution would have
dropped `showTab('sound')` from the second pass, and on O-GrainScatter it would have deleted an
entire return-pass gate stage and merely stopped printing `(return pass)`.

**Running total: seven forms. Waves 4a–4c found four, 4d found the fifth, 4e found none, 4f found
two.**

### 6.4 Every reset-scoped and restore-scoped look-alike, classified at its own site

**Not generalized as a class.** Twenty-one form-6 sites in the wave; **3 generalized, 18 LEFT**, each
with a prose comment at the site saying why.

| plugin | sites | raw lines | class |
|---|---|---|---|
| O-GrainScatter | 4 | — | **RESTORE-scoped** |
| O-Wind | 5 | L677, L702, L723, L724, L889 | **RESTORE-scoped** — each follows a plant restore or a `__setLanguage` reset that writes English by construction |
| **O-Contrabass** | **3** | L252, L253, L256 | **ENUMERATING — GENERALIZED**, the only three in the wave |
| O-Reed | 4 | L495, L515, L516, L656 | L495 **SOURCE-scoped** (runs before any switch); the rest **RESTORE-scoped** |
| O-Bowed | 5 | L476, L501, L520, L521, L665 | L476/L501 **SOURCE-scoped** (inside NC-3/NC-4); the rest **RESTORE-scoped** |

**The reasoning is the same at every left site and it is not a preference:** the line above each
writes the source language *by construction*, so the English row is the only correct comparand.
Reading the current language there would assert nothing, whatever the list holds. Generalizing any of
them would break the restore semantics the assertion exists to prove.

**Two further language-shaped sites on O-Contrabass, classified and LEFT:**

- **`indexOf("let uiLanguage = 'en';")`** — **CANON-scoped and POSITIONAL.** It asserts the canon
  block sits ABOVE the eager `bind*` calls, because below them `uiLanguage` is in its temporal dead
  zone and the call throws a `ReferenceError` that takes the whole UI down. The literal is the
  module's boot-time default, which is the source language by construction; deriving it would assert
  nothing about where the declaration sits.
- **the `2 language` prose fragment** — part of
  `bridge surface is exactly 36 fns (2 mockup + 10 preset + 20 tuning + 2 hover-help + 2 language)`.
  It counts the **`getUiLanguage`/`setUiLanguage` FUNCTION PAIR** on the native bridge, not the
  number of languages the page offers. Adding a language to the table adds no native fn, so the
  figure is invariant under localization. **It still reads exactly 36 (JS=36 C++=36) after the
  work** — the prediction held — and a prose comment now names it as the wrong kind of two.

### 6.5 Derive-or-abort — fired TWICE on every one of the five, against the LIST and against the WALK

Each gate now carries **two independent guards** and no fallback literal:

| plant | expected | observed, all five |
|---|---|---|
| `LANGUAGES = []` | the shape assertion refuses | `FAIL … got []` → `REFUSING to sweep a guessed list.` **exit 1** |
| `LANGUAGES = ['en']` | shape PASSES, the WALK guard refuses | `PASS … got ["en"]` then `FAIL: the derived list carries at least one language besides the source language` → `REFUSING to compare the page to itself.` **exit 1** |

**The second plant is the load-bearing one.** A one-member list satisfies the shape assertion and
then compares the page to itself, passing every byte-equality assertion for the wrong reason. It
proves the guard on the walk is *independent* of the guard on the list.

**Every plant reverted by TARGETED EDIT, never `git checkout --`** (C5/C6), with sha256 byte-identity
asserted against HEAD on both sides — `4304bcf9…` (O-Wind), `15cf56ad…` (O-Contrabass),
`a00d2304…` (O-Reed), `1da3fb29…` (O-Bowed, byte-identical on all three readings). O-GrainScatter's
own gate header records why the rule exists: its Stage-K work lost a whole uncommitted edit to
`git checkout -- <file>`.

### 6.6 After this wave — EIGHT two-language gate files remain, and nobody has audited them

Re-fired at close-out with the wider comment-stripped census. **13 at planning time, five of them
this wave's, 8 remaining:**

`O-Bass`, `O-Bells`, `O-Chorus`, `O-Emulator`, `O-Freeze`, `O-MicrotonalSampler`, `O-ReverseDelay`,
`O-SimpleReverb`.

**Every one of these plugins is already three-language.** Their gates therefore either derive the
list correctly, or are vacuously green on a page whose third language they never visit. **Nobody has
looked.** And none of the eight belongs to a wave-4g plugin — **wave 4g's six plugins carry no gate
file at all** — so this surface has no scheduled owner. It is now stated with a file list in
`deferred-items.md` F2.

---

## 7. Gate results — the whole suite, re-run on the final tree

**Repo-wide, at the J1 baseline exactly:**

```
check-i18n            exit 0    ALL CHECKS PASS — 43 localized plugin(s)
i18n-zh-lint          exit 2    0 finding(s) across 0 plugin(s), and 1 plugin(s) could not be read
                                zh-Hans entries checked: 3496   plugins with findings: 0 / 44
i18n-zh-lint --self-test        SELF-TEST: 10/10, exit 0
i18n-fr-lint          exit 2    plugins with findings: 0 / 44   (1 could not be read)
```

**Exit 2 is the baseline on both lints and is NOT a finding** (J1). O-Strata has no `i18n.js` under
either UI root; the lints enumerate `plugins/*` from disk. **The load-bearing lines are the finding
count and the unreadable count**, and both matched exactly. `i18n-fr-lint` reading 0 findings across
44 is the evidence that **French was untouched by the whole wave** — including eight corrected hover
bodies, five deleted pair clauses and two panel font overrides.

**Scoped, all five, exit 0 on every arm:**

```
check-i18n --plugin       0  0  0  0  0
i18n-zh-lint --plugin     0  0  0  0  0
check-ui-labels --plugin  0  0  0  0  0
```

**Structural probes, all five:**

| probe | O-GrainScatter | O-Wind | O-Contrabass | O-Reed | O-Bowed | criterion |
|---|---|---|---|---|---|---|
| Han under `Source/**/*.{h,cpp}` | 0 | 0 | 0 | 0 | 0 | 0 each |
| positive control on the same probe (`i18n.js`) | **1** | **1** | **1** | **1** | **1** | **1 each — a 0 means the negative gate proved nothing** |
| `PluginProcessor.h` `zh-Hans`, **comments stripped** | 2 | 2 | 2 | 2 | 2 | ≥ 2 each |
| enumerating-`fr` sites in the gate file | **0** | **0** | **0** | **0** | **0** | 0 each (was 2/3/3/3/3) |

The codec probe is comment-stripped deliberately: **all five carry the doc comment**, so an
unstripped count reports success on a file whose CODE never changed.

**`boot-all-uis --strict-tips`, whole suite:**

```
exit 0    clean: 43 / 43    warn: 0    failed: 0
DEAD bindings:  0  across 0 plugin(s)
late bindings:  2  across 1 plugin(s)  —  O-Bells  #ref-pitch-knob, #octave-stretch
```

**The O-Bells pair is the census control (wave 4e D2), and it read 2 before and 2 after.** That is
what makes the 0 DEAD beside it evidence rather than a number.

**Hygiene:**

```
modules/ dirty            0     — the shared module tree byte-unchanged by the whole wave,
                                  including snippets/tuning-panel.css and js/tuning-panel.js
scripts/ dirty            0     — no gate script edited
O-MicrotonalSampler       byte-unchanged (s71 D4 — its own diverged 317-line copy, out of scope)
plugins/O-Orbit/libs/SAF  0     — the repo's only submodule path, never staged
O-Strata in git status    0     — see §9.3
tags at HEAD              0
branch / worktrees        main / 1
unsuffixed AU orphans     0
```

---

## 8. Fonts and geometry — the batch view

### 8.1 measure-ui, before → after, all five

| screen | O-GrainScatter | O-Wind | O-Contrabass | O-Reed | O-Bowed |
|---|---|---|---|---|---|
| `undeclared-font` | 0 vacuum → **0** / 86 Han | 155 → **0** / 155 | 113 → **0** / 113 | 111 → **0** / 111 | 95 → **0** / 95 |
| `line-height-normal` | 21 → **7** | 62 → **21** | 54 → **9** | 46 → **1** | 38 → **4** |
| `wrap-count` | **0** | **0** | **0** | 1 → **2** (F8) | 1 → **1** |
| `svg-font-attr` carriers / findings | 0 / 0 | 0 / 0 | **1 / 0** | 0 / 0 | 0 / 0 |
| `identity` nodes / DOM keys / display ids | 765 / 255 / 51 | 1920 / 640 / 221 | 1695 / 565 / 169 | 1689 / 563 / 150 | 1491 / 497 / 139 |
| `check-ui-labels [7]` zh MOVED | 140 → **0** | 176 → **0** | 250 → **0** | 266 → **0** | 215 → **0** |
| `check-ui-labels [7]` fr MOVED | **0** throughout | **0** | **0** | **0** | **0** |
| states unresolved / skipped | **0** | **0** | **0** | **0** | **0** |

**Every `undeclared-font` zero is now NON-VACUOUS** — measured against 86 / 155 / 113 / 111 / 95 Han
nodes respectively. J13's screens were a vacuum before the column landed and the plan said so; that
is now closed on all five.

Derived populations at close-out, all five:

```
                zhHan  noCJKface  bareArial  systemStack  MOVED
O-GrainScatter    86       0          0           0         0
O-Wind           155       0         17           0         0
O-Contrabass     113       0         17           0         0
O-Reed           111       0         17           0         0
O-Bowed           95       0         17           0         0
```

**`noCJKface = 0` on all five is the load-bearing criterion and it is met.** **`systemStack = 0` on
all five** — the shared module's `-apple-system` stack, which rendered 116 nodes on O-Reed and 116 on
O-Bowed as found, reaches zero nodes on any arm.

**`bareArial = 17` is NOT a defect, and the plan's verify line predicting 0 is wrong** (§9.2). The 17
residual bare-Arial nodes on each consumer are **2 buttons + 15 numeric inputs**, and at close-out
**ZERO of the 17 carry Han on any arm, on any of the four plugins** — measured, not argued. That is
N3's discriminator answered in the negative for the numeric nodes: the module gives them no
`aria-label` and no `data-tip`, so they have no Han to render and the CJK tail would be inert.

### 8.2 The `line-height-normal` residuals — every one an evidenced NON-MOVER

The criterion is **`check-ui-labels` 0 moved** (N1), which is met on all three arms of all five. The
screen structurally cannot reach 0: `han` is computed over the node's own text **plus `data-tip`,
`data-tip-title` and `aria-label`**, so a numeric readout carrying a Chinese `aria-label` is a finding
forever — it has no Han to render, its box is identical on both arms, and no pin can change that.

**42 residuals across the wave, every one named with its measured `enH == zhH`** in its own task
summary. And **three of O-Contrabass's nine are `<canvas>` elements with NO TEXT AT ALL** —
`#canvas-schelleng`, `#canvas-spectrum`, `#canvas-vu` — in the `han` set only through their Chinese
`aria-label`. See §9.2, finding 4: the residual count is not a caption count.

### 8.3 Geometry — one root cause per plugin, and the symptom counts are not the work

`140 / 176 / 250 / 266 / 215` `[7]` entries at first zh run, all to **0**. Every one traced to a small
number of root causes:

- **A `flex: 1 1 auto` container turns N small caption growths into one big move.** Fourteen leaves
  growing 1–3 px produced **140** entries on O-GrainScatter. Read the `dh` column and find the ONE
  absorbing container; the count is not the worklist.
- **The eleven-selector `1.11` ratio block** transferred from `d848337a` to all four consumers and was
  **RE-DERIVED per selector**, not copied — `.viz-btn`, `.tuning-file-btn` and `.generator-btn` are
  named separately because the UA `font` shorthand resets `line-height` on form controls (F6).
- **Chinese is SHORTER**, so nearly every pin is a **floor**, set to the exact measured English box so
  neither Latin arm moves (N9). **R8 was LIVE on all five**, as wave 4e predicted and as the plan
  predicted — the first prediction of its kind in two waves to hold on every plugin.
- **The three `d848337a` floors** held on O-Wind, O-Contrabass and O-Reed as measured, and **all three
  moved on O-Bowed** — `.tonic-label` 41.59 → 42.06, `.octave-stretch-label` 51.84 → 53.86,
  `.interval-list-header` 24 → 22. That discovery is F7: **a floor measured while the floor is in
  place is not measured**, and O-Bowed's natural French box exceeded its floor by 0.47 px — inside
  `check-ui-labels`' tolerance, so no gate could ever have seen it.

**State-EFFECT assertions fired on every `click` state, not just `elementFromPoint`** (wave 4e N1), on
all five, with `eval` states recorded as needing no pointer probe.

### 8.4 The stale English that shipped false before a byte of Chinese

**Eight hover-help bodies were FALSE ON THE TREE as found**, and were corrected in **en and fr
together with their explanatory comments, BEFORE the Chinese was authored, with the file re-read in
between** (C7). No tool in this repo compares a `zh` body against its own `en`; only the reverse read
catches a row that outlives the sentence it was translated from.

- **J9 — four settings bodies** claimed the panel holds the interface language *and nothing else*,
  while `#tips-toggle` sits inside `.settings-popover` on every one of them. Clauses **DELETED, not
  widened**, following O-Detune's shipped precedent.
- **J8 — three tuning bodies** promised the tuning tab stays English. False since v1.5.0 (O-Reed),
  v1.8.0 (O-Bowed) and their siblings — s71 localized the shared panel.
- **J10 — five endonym-pair clauses** (*"English or Français."*) deleted in en and fr, with every
  still-true exception kept (host automation lane, on-screen values).

**Two bodies were read in full and deliberately LEFT** on O-Contrabass — the one plugin in the wave
where both deletion reflexes would have been wrong — each classified at its site as a checked
non-defect.

After correction the comment-stripped stale-body probe reads **0** on all five. **And the probe is
byte-blind to the French `rien d’autre` (U+2019)** — see F11; the true site count was 6, not the 4 or
5 the probe reported.

---

## 9. The false-prediction ledger

Wave 4d produced **3**. Wave 4e produced **14**. s71 produced **19**. **This wave produced 38 across
Tasks 1–5, plus 3 at batch level = 41.**

**The count is itself the finding, and after four waves its SHAPE is stable: the miss is concentrated
in what the GATES will say and where the LINES are, not in what the FILES contain.** The plan's
readings of key counts, row counts, node populations, font-declaration counts, pin censuses,
`PLUGIN_CODE`s, frames and codec line numbers were very nearly all exact.

### 9.1 The seven candidates the plan flagged in advance — each resolved

| # | prediction | verdict | evidence |
|---|---|---|---|
| 1 | **J6:** `check-i18n` `[1]` does not flag an undeclared extra language column | **HELD — and s71 was WRONG** | Task 2 fired the two-arm fixture control: both arms IDENTICAL, all six FAILs are scaffold gaps, none is `[1]`. s71's stated mechanism ("…and no others") is false. The rule that the column lands WITH the flip stands for the STRONGER reason: nothing catches the half-landing |
| 2 | **J15:** the `-apple-system` override does not move en/fr | **HELD, with a nuance** | 0 moved on both Latin arms on O-Reed and O-Bowed. O-Reed's fr DID move at first — caused by a floor measured against the OLD face, R8's exact case, not by the override |
| 3 | **J14:** the `d848337a` floors hold against the zh arm | **FALSE on O-Bowed** | held on O-Wind, O-Contrabass and O-Reed; **all three moved on O-Bowed.** Task 4 predicted one would need re-measuring; three did |
| 4 | **J11:** `undeclared-font` reports fewer than 25/30 once the table lands | **HALF TRUE — see 9.2 #2** | true of the form-4 SUBSET (25/30 → 17, and 0 of the 17 carry Han). False of the screen's headline, which counts all Han nodes without a CJK face: 155 / 113 / 111 / 95. **And its N3 positive arm for the two `<select>`s is FALSE** — the screen reads both `han: false` |
| 5 | **the bridge surface stays at exactly 36 on O-Contrabass** | **HELD** | JS=36 C++=36, unchanged before and after |
| 6 | **O-GrainScatter's `#stutter-gate-btn` 110 px fixed width does not clip** | **HELD** | measured **110.00 × 29.00 on BOTH arms**; 断续门 fits |
| 7 | **every raw line number cited in J2–J16** | **FALSE in five places, across four of five tasks** | see 9.3 |

### 9.2 New at batch level — three, all found in Task 6's own work

1. **The orchestrator's ~15-minute cold-auval budget is stale.** Measured: `auval -a` **83 seconds**,
   five `auval -v` runs **1 second**. s71's D11 correction confirmed a second time on a second day.
2. **The plan's verify block predicts `bareArial 0` on all five.** Measured: **0 on O-GrainScatter
   and 17 on each of the four consumers**, and the four task summaries each record `bare Arial 30 →
   17` as the intended shipped state. **The verify line contradicts the tasks it was written to
   check.** The load-bearing criterion beside it — `noCJKface 0` — reads 0 on all five, and at close-
   out **0 of the 17 carry Han on any arm on any of the four plugins**, which is the fact the
   criterion was reaching for.
3. **The plan's two `line-height-normal` verify arms disagree on exactly one plugin.** The screen
   reports **9** residuals on O-Contrabass; the plan's own `node -e` arm reports **6**. The gap is
   three `<canvas>` elements with `own` text `""` — the screen's leaf filter is `kids === 0` with no
   text requirement, the `node -e` arm adds `own.trim()`. Both are correct and they answer different
   questions. **The residual count is not a caption count**, and a canvas can never move and can never
   be pinned. The other four agree exactly (7/7, 21/21, 1/1, 4/4).

### 9.3 The 38 from Tasks 1–5, by task

**Task 1 — O-GrainScatter (6):** `plugins/O-Strata/` is TRACKED, not untracked, so the verify's
`grep -c "O-Strata"` reads 0 · the product-name control returns **0**, not non-zero (the wordmark is
an `I18N_EXEMPT` markup string, not a table row) · `system_profiler` cannot see `Times` · M8's
line-box table does not cover a `<button>` (10 px → **1.3**, not 1.1) · the refusal-control ordering
confounds control 2 · the description's version premise (`2.6.1 → 2.7.0`) was wrong and the plan's
correction was right.

**Task 2 — O-Wind (9 plan predictions + 2 authoring):** O-Strata again · the stale-body probe reports
**7** for 5 substantive sites · **J4's five form-6 line numbers are all wrong** (L540/L565/L586-587/
L752 against L677/L702/L723/L724/L889) while every other raw number in the same file is exact ·
`undeclared-font` cannot see a collapsed `<select>`'s Han · **the duplicate-key scan cannot read
`NONE` on any three-language table** (F13) · the ratio-block verify greps `-A14` where CSS needs
`-B14` · the tuning state mounts **30** module captions, not 13 · J14's O-Wind floor line numbers mix
RAW and STRIPPED in one table row · the ingest's refusal order has a **third** step (reverse
`--provenance` first). Plus two authoring predictions the lint corrected (`低频`/`高频` fired Z5; a
termNote containing `<select>` fired `check-i18n [9]`).

**Task 3 — O-Contrabass (7):** O-Strata, count 1 again **for a different reason** · the product-name
control returns 0 a second time · **`svg-font-attr` CANNOT see the defect** — it is a divergence
report, invariant across the whole defect and the whole fix (F3) · the font-declaration count is
11 + 1, not 12 + 1 · **J4's O-Contrabass line numbers are the COMMENT-STRIPPED ones and the same
finding mixes both conventions** · "no external stylesheet" is true of the plugin and false of the
page (`index.html:28` loads the shared module CSS) · two new tooling traps (`serve()` has no `url`
field; a `'bt'` literal inside `node -e '…'` loses its quotes).

**Task 4 — O-Reed (8):** O-Strata, count 1 for a third different reason · the stale-body probe
reports 5 where the substance is **6** · **the probe is BLIND to the French half of the J9 site**, on
a U+2019 encoding trap (F11) · **J4's O-Reed line numbers AND the restore-scoped COUNT are both
wrong** (4 sites, verify expected 3) · `wrap-count` goes 1 → 2 with no pixel changing (F8) ·
**`--forward-provenance` takes a VALUE and the rules never say so** (F9) · `--model
claude-opus-4-1` is silently remapped to Opus 5 · Task 3's corpus arithmetic double-counts by one
plugin.

**Task 5 — O-Bowed (8):** `plugins/O-Strata/` does not appear in `git status` **at all** and the
verify's count reads **0**, not 1 · **carry-forward 1 predicted ONE floor would need re-measuring;
ALL THREE did** · **`min-width: 0` is not "no floor"** — it un-pins a flex item below its content
(F7) · the plan's `sweep('en')` line number is off by **104** · `wrap-count` did **not** grow, the
ESTIMATED denominator fell instead (110 → 76) · **the product-name control reads 1, not 0** — a
SECOND route into the batch, an exempt token quoted inside a body (`tip.hairStiff` names *O-Bowed*,
and W3 direction 1 requires it to survive verbatim) · the model remap confirmed a second time · the
state-EFFECT module-caption figure is **20** under a snapshot and **30** under a cumulative walk, and
both are correct.

**The O-Strata precondition is the wave's most instructive miss.** It was wrong in Task 1 (tracked,
count 0), wrong in Tasks 3 and 4 (count 1 again, but for a NEW untracked subdirectory a concurrent
session created), and wrong in Task 5 (count 0, tree clean). **The count was never the check.** The
orchestrator's replacement — *no `git status` entry under the plugin being worked on* — is the form
that discriminates, and it was used and met by every task. J1's lint consequence was unaffected
throughout, because the lints enumerate `plugins/*` from **disk**, not from the index.

---

## 10. The blind reverse read — batch accounting

**Every plugin's read is accounted for.** 712 non-module rows read blind across the wave, in ten
chunks, by fresh agents from a cwd **outside the repo** with `--allowed-tools ""` and file, web and
repository access forbidden in the prompt.

| plugin | table rows | blind batch | module rows excluded | chunks | correction rounds | rows re-authored |
|---|---|---|---|---|---|---|
| O-GrainScatter | 131 | **131** | 0 (not a consumer) | 2 (66 + 65) | 1 (round 2 corrected nothing) | 1 |
| O-Wind | 214 | **177** | **37** | 2 (89 + 88) | 1 (different model; corrected nothing further) | 1 string on 2 keys |
| O-Contrabass | 186 | **149** | **37** | 2 (75 + 74) | **0 — nothing to correct** | **0** |
| O-Reed | 168 | **131** | **37** | 2 (66 + 65) | 2 (round 2 = Opus 5, a different family) | 1 |
| O-Bowed | 161 | **124** | **37** | 2 (62 + 62) | 2 (round 2 = Opus 5) | 1 |
| **total** | **860** | **712** | **148** | **10** | — | **4 rows / 5 strings** |

**The 148 module rows were excluded from every batch** and are byte-identical copies of an
already-reverse-read O-Bassoon column — R1's one stated exception, said so in every commit.

**Controls: every one fired in every batch.** Sixteen per batch on Tasks 2–5. Ids pure 12-hex with no
key fragment; **returned ids identical AND IN ORDER** (M12); rows well-formed with no Han surviving in
the returned English; a sha256 of the zh column against the committed tree; **two emits of the same
target sharing 0 blinded ids**, so each chunk carries its own salt; chunks proved disjoint on the
REAL ids with the union exactly equal to the batch; and each chunk proved to contain no module row.

**Both refusal controls fired in every batch**, run with `--provenance` supplied so each fired for
its own reason and not for the one above it (F10):

```
WRONG MANIFEST FOR THIS BATCH: all N returned ids are unjoinable, not one.
```

**All 712 triples were read with `--verbose` (R2).** `--ingest` truncates to 12, and across seven
waves essentially no real finding has ranked inside the default twelve — this wave's four
re-authorings included.

**Chunking was on the CONCEPT SPLIT, not on size** (F17): 46 / 36 / 34 concepts deliberately sent
caption-in-one-chunk, title-or-body-in-the-other on Tasks 3, 4 and 5. The method's own strongest
evidence is O-GrainScatter's `label.stutterGate` / `tip.stutterGate` divergence — the **same** Chinese
string came back as two different English words from two different readers, and only the split made
it visible.

**R4 was satisfied on every correction round** — fresh agent, fresh salt, different model. Note that
`--model claude-opus-4-1` is silently remapped to Opus 5; round 1 was Sonnet 4.5 and round 2 Opus 5,
genuinely different families, **but not by the name requested** (F15).

**R3 glossary screens were run BEFORE authoring on all five and came back clean**, which is what
caught O-Bowed's real collision. **Four termNote exemptions were authored across the wave**, plus
`label.genDivisions` inherited with its module row; **none of them is a glossary divergence**, and
the one genuine glossary defect the wave found — `'dist lpf'` → 失真低通 where the only site in the
corpus means DISTANCE — is carried as a corpus-level item and **not edited here** (D1).

---

## 11. Answers to the carry-forward ledgers

### 11.1 Wave 4e — structural findings N1–N13

| # | item | verdict | evidence |
|---|---|---|---|
| N1 | `elementFromPoint` has two failure modes; assert the state's EFFECT | **ANSWERED** | effect assertion fired on every `click` state on all five; `eval` states recorded as needing no pointer probe |
| N2 | `undeclared-font` is not the form-4 census | **ANSWERED, and extended** | reported per plugin against the `inherit` declarations (§11.2 #6); F12 adds the collapsed-`<select>` blindness |
| N3 | a `--symbol-font` token needs the CJK tail if the node is a TIP ANCHOR | **ANSWERED — the negative case, measured** | the module's 15 numeric inputs and 2 arrow buttons have no `aria-label` and no `data-tip`; **0 of the 17 carry Han on any arm on any of the four consumers** |
| N4 | pin caption ROWS as well as leaves, and use a RATIO | **ANSWERED** | four caption rows on O-Contrabass grew with no leaf reporting it; ratio blocks, never lengths |
| N5 | where an M13 qualification has to STOP | **NOT APPLICABLE** | no M13-shaped two-root collision arose; the wave's four termNotes are per-page sense notes against correct roots |
| N6 | measure the pin surface; never inherit a table | **ANSWERED** | R8 fired on all five; the pin census was measured before anything was appended on every plugin |
| N7 | a shrink can be two-dimensional | **ANSWERED** | O-Bowed needed floors on both axes; O-Contrabass's two width floors are R8 at its most dangerous |
| N8 | read the `never became visible` NOTE and check the state classes | **ANSWERED** | **10 each on all four consumers**, exactly s71 D5's set; 9 of the 10 are `<option>` inside a collapsed `<select>` and are **structurally unmeasurable**, not merely unmeasured |
| N9 | the two-arm CMake reader is the majority case | **ANSWERED — and the answer changed** | this wave: **two unquoted, three quoted, NO `set()`-variable form**; the first wave since 4c with no third shape |
| N10 | M12 earns its keep | **ANSWERED** | id-order control fired on all ten chunks; no reader fault this wave |
| N11 | re-derive the installed-family table | **ANSWERED** | §3 — unchanged entry for entry, and the `Times` zero named as the table's own false-negative class |
| N12 | the enumeration census, and whether a sixth form appears | **ANSWERED IN FULL** | §6 — five files not three; forms 6 AND 7 admitted; every look-alike classified at its site |
| N13 | five settings bodies read, five left — no P6 defect | **ANSWERED, and INVERTED** | this wave read eight and **corrected eight**; the streak ends here, and the defects predate the wave |

### 11.2 Wave 4e — "Explicitly for wave 4f", 1–19

| # | instruction | verdict |
|---|---|---|
| 1 | DECIDE the `scala-tuning-engine` question before planning | **ANSWERED — closed by s71 before this wave, option 1** (localize the module once). Four of five plugins consumed the 37-key column; the module tree is byte-unchanged by this wave |
| 2 | run the enumeration census on wave 4f's **three** gate files | **ANSWERED, and the premise corrected: there were FIVE** (§6.1) |
| 3 | fire every precondition you assert, including "this gate passes" | **ANSWERED** — 8 / 9 / 9 / 9 / 9 fired on the tree as found; the only false one was the O-Strata git precondition (§9.3) |
| 4 | use the two-arm CMake reader unconditionally; report the shapes | **ANSWERED** — two unquoted, three quoted, no `set()` form (§5) |
| 5 | re-derive the installed-family table | **ANSWERED** (§3) |
| 6 | report the form-4 population per plugin AGAINST the `inherit` declarations | **ANSWERED** — O-GrainScatter **0 form-4 / 0 `inherit`** (a page with no instance of the class, not a page nobody measured); O-Wind **25 / 10 of 13**; O-Contrabass **30 / 9 of 11 + 1 SVG**; O-Reed **30 / 7 of 8**; O-Bowed **30 / 9 of 10** |
| 7 | measure the existing pin surface; never inherit a table | **ANSWERED** — R8 live on all five, as predicted; the plan predicted it dead on none |
| 8 | pin caption ROWS as well as leaves, with a RATIO | **ANSWERED** — four rows on O-Contrabass |
| 9 | state the `line-height-normal` criterion as N1 gives it | **ANSWERED** — `check-ui-labels` 0 moved on all five; 42 residuals, every one named with its `enH == zhH` |
| 10 | budget for FLOORS; expect a shrink in both axes | **ANSWERED** — nearly every pin is a floor; F7 is the wave's own correction to how a floor is measured |
| 11 | read the `never became visible` NOTE | **ANSWERED** — 10 each, s71 D5's set, unchanged |
| 12 | fire the state-EFFECT assertion on every state | **ANSWERED** — every `click` state on all five |
| 13 | check `I18N_EXEMPT` membership BOTH ways | **ANSWERED** — the rollout's largest exempt surfaces (O-Wind 9 mentions, O-Reed 8 including fifteen XY-pad markers and six dropdowns' option words), checked in both directions on all five |
| 14 | hold ONE Latin/Han spacing form; expect W5 to fire | **ANSWERED** — the SPACED form held table-wide on all five |
| 15 | check the returned ids identical AND IN ORDER | **ANSWERED** — fired on all ten chunks; no fault |
| 16 | expect two DIFFERENT roots to collide; know where qualification stops | **NOT APPLICABLE** — no M13-shaped collision arose (see N5) |
| 17 | pass `auval -v` three unquoted words | **ANSWERED** (§2) |
| 18 | re-inherit D1–D7 | **ANSWERED** — carried in `deferred-items.md` D6, D7, D12; D1 closed by s71; D2 used as the boot census control; D3 **CLOSED** (`abb0ea2d`, O-MultiBandCompressor v1.12.1) |
| 19 | report every prediction FALSE as stated | **ANSWERED** — 41 (§9) |

### 11.3 s71 — "What wave 4f still owes on this surface", 1–4

| # | item | verdict |
|---|---|---|
| 1 | the zh column on the 37 new keys, **for four plugins not three** — and **O-Contrabass is in nobody's wave list** | **ANSWERED. O-Contrabass WAS scheduled**, as Task 3, and all four consumers carry the 37 rows at `reviewed: 'bt'`. The warning was correct and it worked |
| 2 | the zh debt is GEOMETRY, not only table rows — the `1.11` ratio block and the CJK tail on three button classes | **ANSWERED** — both landed on all four, and the ratio block was **re-derived per selector rather than copied** (the numbers transferred, the derivation did not) |
| 3 | the N12 census on the three untouched `ui_tip_render_check.js` files | **ANSWERED, premise corrected to five files** (§6.1) |
| 4 | O-Bassoon's three residual `line-height-normal` findings | **NOT APPLICABLE** — O-Bassoon is not a wave-4f plugin and was not touched. The finding's general form is answered: the screen cannot reach 0, and the criterion is `check-ui-labels` 0 moved |

### 11.4 s71 — "Explicitly for the next wave", 1–7

| # | instruction | verdict |
|---|---|---|
| 1 | **schedule O-Contrabass** | **ANSWERED** — Task 3, v1.10.0, 186 rows |
| 2 | budget the two zh geometry pins per plugin, not just the rows | **ANSWERED** — both budgeted and both landed on all four consumers |
| 3 | run the N12 census on the three files | **ANSWERED, premise corrected to five** |
| 4 | fire every precondition, including "this gate passes" | **ANSWERED** — see 11.2 #3 |
| 5 | use the two-arm CMake reader unconditionally | **ANSWERED** — see §5 |
| 6 | drive nothing from `modules/registry.yaml` `used_by` until rebuilt | **ANSWERED by compliance** — the CMake grep was used, the registry was not read; the registry is still stale and is carried as `deferred-items.md` D8 |
| 7 | report every prediction FALSE as stated | **ANSWERED** — 41 against s71's 19, 4e's 14, 4d's 3 |

---

## 12. The corpus, recomputed LIVE at close-out

Not carried from Task 5. Re-run on the final tree:

```
$ node scripts/i18n-zh-backtranslate.js
  zh-Hans strings: 4621 of 5441 rows   (1 plugins could not be read)
  BELOW SHIP BAR — at reviewed:'mt' or unflagged: 0. The ship bar is 'bt'.

  O-Bowed         161  161   0 mt  161 bt   0 native   0 none
  O-Contrabass    186  186   0 mt  186 bt   0 native   0 none
  O-GrainScatter  131  131   0 mt  131 bt   0 native   0 none
  O-Reed          168  168   0 mt  168 bt   0 native   0 none
  O-Wind          214  214   0 mt  214 bt   0 native   0 none

  plugins carrying at least one zh row: 37     (32 before this wave)
```

**4621 of 5441 rows, `BELOW SHIP BAR 0`, across 37 localized plugins.** Task 5's figure is confirmed
by independent recomputation, and it resolves Task 3's arithmetic slip (which added 186 to a total
that already contained them): the chain reconciles as
`3761 + 131 + 214 + 186 + 168 + 161 = 4621`.

**Wave 4g is 820 rows and six plugins** — O-simpleGrain 153, O-simpleSampler 145, O-Polystutter 133,
O-simpleSubtractive 133, O-simpleAdditive 131, O-Orbit 125. `5441 − 4621 = 820` exactly. **It closes
the corpus.** Details, and the answer to whether the 4e sheet names them (it does not), in
`deferred-items.md`.

---

## 13. Deferred

Full ledger in `deferred-items.md` beside this file: **18 structural findings (F1–F18)** and
**12 deferred items (D1–D12)**, plus the wave-4g carry-forward sheet.

The five that matter most:

- **D1** — `scripts/i18n-zh-glossary.js:433` `'dist lpf': ['失真低通']` should read `['距离低通']`.
  The only site in the corpus means **distance**, not distortion. A corpus-level edit outside any
  per-plugin task's path scope; now the oldest open item in the ledger.
- **D2** — O-Reed's **20 missing and 7 dead** native-function registrations. A C++/JS bridge defect,
  measured unchanged before and after, blocked nothing.
- **D3** — two stale `d848337a` floors on O-Reed, ≤0.5 px and 2 px, that **no gate in this repo can
  see** (F7).
- **D4** — the 17-of-37 shared-module coverage hole (s71 D5), unchanged.
- **D5** — `plugins/O-Strata/` still makes both repo-wide lints exit 2, now for a reason **no longer
  visible in `git status`**.

---

## Self-Check: PASSED

**Files:**

- `PLUGINS.md` FOUND, five rows corrected, dup-check empty.
- `.planning/quick/260907-ja8-…/deferred-items.md` FOUND.
- `.planning/quick/260907-ja8-…/260907-ja8-SUMMARY.md` FOUND.
- All five task summaries FOUND (`260907-ja8-TASK1-SUMMARY.md` … `TASK5-SUMMARY.md`).

**Commits** — all eleven verified present with `git log --oneline --all | grep -q`:

`b534636b` · `a92b7980` · `020409f8` · `bff367f6` · `5a216066` · `b69ca8ea` · `9d83e7a7` ·
`f3737f53` · `089e65e6` · `f25090c8` · **`d98f369d`**

**Installed bundles** — all ten FOUND at their shipped versions, `-dev` only, no unsuffixed orphan:

`O-GrainScatter-dev` 2.8.0 · `O-Wind-dev` 1.21.0 · `O-Contrabass-dev` 1.10.0 · `O-Reed-dev` 1.6.0 ·
`O-Bowed-dev` 1.9.0 — VST3 and AU each.

**auval** — five for five `AU VALIDATION SUCCEEDED`, exit 0, after one cold rescan.
