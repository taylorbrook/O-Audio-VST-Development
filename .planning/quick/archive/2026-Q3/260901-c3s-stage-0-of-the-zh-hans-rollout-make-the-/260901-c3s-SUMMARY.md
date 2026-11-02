---
phase: quick-260901-c3s
plan: 01
subsystem: i18n-tooling
tags: [i18n, zh-Hans, gates, canon, stage-0]
status: complete

requires:
  - "locked decisions (a)-(f) from 260901-akh-IMPLEMENTATION-PLAN.md, confirmed 2026-09-01"
provides:
  - "language-agnostic repo-level i18n gates — a 2-language and a 3-language plugin both pass"
  - "check-i18n reviewed-flag enum for zh-Hans at BOTH reviewed sites"
  - "check-ui-labels drives the plugin's OWN LANGUAGES, read from disk"
  - "i18n-extract emits a zh-Hans skeleton row at reviewed: 'mt'"
affects:
  - "every later stage of the zh-Hans rollout (Stages 1-6) — this was the hard prerequisite"

tech-stack:
  added: []
  patterns:
    - "gate reads the plugin's own declared LANGUAGES rather than a hard-coded pair"
    - "reviewed flag is per-language: fr boolean, zh-Hans enum, everything else fails closed"
    - "assertion messages interpolate the language actually measured"

key-files:
  created: []
  modified:
    - scripts/i18n-canon.js
    - scripts/check-i18n.js
    - scripts/check-ui-labels.js
    - scripts/i18n-extract.js
    - "43 shipping canon copies under plugins/ (list below)"
  deleted:
    - plugins/O-AnalogSaturation/.planning/i18n-index-draft.html
    - plugins/O-Bitrot/.planning/i18n-index-draft.html
    - plugins/O-Emulator/.planning/i18n-index-draft.html
    - plugins/O-SimpleReverb/.planning/i18n-index-draft.html

decisions:
  - "Assertions 2, 8 and 8b in check-ui-labels were generalized alongside the plan's named 5/6/7, because they sit in the same contiguous fr-vs-en region and leaving them fr-only would have left a zh-Hans vacuity hole (threat T-c3s-05)."
  - "Three assertion messages that named two languages while measuring three were corrected in a third commit — surfaced only by the three-language scratch run."
  - "Only the FOUR drafts named by locked decision (f) were deleted, not all 37 i18n-index-draft.html files; the plan's verify assertion 5 globbed all of them and is mis-specified."

metrics:
  duration: ~13min
  completed: 2026-09-01

actuals:
  tokens: 80744
  tasks: 3
  commits: 3
---

# Quick 260901-c3s: Stage 0 — language-agnostic i18n gates

Made every repo-level i18n gate read the plugin's own `LANGUAGES` instead of a hard-coded
`['en','fr']` pair, so a two-language plugin and a three-language plugin both pass while the
zh-Hans rollout is mid-flight. Zero zh-Hans content was added to the repo.

## Commits

| # | Hash | Scope |
|---|------|-------|
| 1 | `e9aab4fc` | decision (f) + P1 + P2 — 48 files: canon + 43 shipping copies + 4 draft deletions |
| 2 | `ee2e92cf` | P3-P7 — `check-i18n.js`, `check-ui-labels.js`, `i18n-extract.js` |
| 3 | `ef9f47f9` | gate-driven fix: three assertion messages named two languages while measuring three |

Commit 3 is the third path-scoped commit the plan permits when a gate forces a fix. It was
triggered by Gate B and Gate D, not by a failing automated assertion — see "Deviations".

## Task 1 — decision (f) + P1 + P2

**Derivation command** (run at execution time, not hand-typed):

```bash
grep -rl "applyI18n(code === 'fr'" plugins | grep -v 'i18n-index-draft.html' | sort
```

Returned exactly **43**. Each of the 43 was independently confirmed to contain the target
substring **exactly once** before any file was touched, so the substitution was a literal
1-for-1 replace, never a regex that could over-fire.

**Filename shapes encountered** — five, exactly as the plan predicted:

| shape | count |
|-------|-------|
| `app.js` | 20 |
| `index.html` (canon inline in a `<script type="module">`) | 19 |
| `main.js` | 2 |
| `sampler-app.js` | 1 |
| `i18n_init.js` | 1 |

**The 43 swept files:**

```
plugins/O-AnalogEQ/Source/ui/public/index.html
plugins/O-AnalogSaturation/Source/ui/public/index.html
plugins/O-Bass/Source/ui/public/index.html
plugins/O-Bassoon/Resources/ui/index.html
plugins/O-Bells/Resources/ui/index.html
plugins/O-Bitrot/Source/ui/public/index.html
plugins/O-Bowed/Resources/ui/index.html
plugins/O-Chorus/Source/ui/public/index.html
plugins/O-Comp/Source/ui/public/index.html
plugins/O-Contrabass/Source/ui/public/index.html
plugins/O-Detune/Source/ui/public/index.html
plugins/O-DigiDelay/Source/ui/public/index.html
plugins/O-Emulator/Source/ui/public/index.html
plugins/O-Formant/Source/ui/public/js/main.js
plugins/O-Freeze/Source/ui/public/index.html
plugins/O-FreqPulse/Resources/ui/js/app.js
plugins/O-Gain/Source/ui/public/js/app.js
plugins/O-GrainScatter/Source/ui/public/js/app.js
plugins/O-IntonationPad/Source/ui/public/js/app.js
plugins/O-Lyrica/Resources/ui/js/app.js
plugins/O-Marimba/Source/ui/public/js/app.js
plugins/O-MicrotonalSampler/Resources/ui/js/sampler-app.js
plugins/O-MultiBandCompressor/Source/ui/public/js/app.js
plugins/O-Octagon/Source/ui/public/js/app.js
plugins/O-Orbit/Resources/ui/js/app.js
plugins/O-Polystutter/Source/ui/public/js/app.js
plugins/O-Prism/Source/ui/public/index.html
plugins/O-Reed/Resources/ui/index.html
plugins/O-ReverseDelay/Source/ui/public/js/app.js
plugins/O-simpleAdditive/Source/ui/public/js/app.js
plugins/O-simpleBeatmaker/Source/ui/public/js/app.js
plugins/O-simpleFM/Source/ui/public/js/app.js
plugins/O-simpleGrain/Source/ui/public/js/app.js
plugins/O-simplePhysicalModelSynth/Source/ui/public/js/app.js
plugins/O-SimpleReverb/Source/ui/public/index.html
plugins/O-simpleSampler/Source/ui/public/js/app.js
plugins/O-simpleSubtractive/Source/ui/public/js/app.js
plugins/O-SpectralShaper/Resources/ui/js/app.js
plugins/O-Tapestop/Source/ui/public/js/app.js
plugins/O-Texture/Source/ui/public/js/main.js
plugins/O-TextureForge/Source/ui/public/js/i18n_init.js
plugins/O-Tremolo/Source/ui/public/index.html
plugins/O-Wind/Resources/ui/index.html
```

`scripts/i18n-canon.js:216` is now, verbatim:

```
            .then((code) => applyI18n(code))
```

Task 1 verification, all as run:

| gate | result |
|------|--------|
| `grep -c "code === 'fr'" scripts/i18n-canon.js` | `0` |
| pass-through form present at line 216 | `216:            .then((code) => applyI18n(code))` |
| clamp remaining anywhere under `plugins/` | none |
| copies carrying `applyI18n(code))` | `43` |
| `node scripts/check-i18n.js` | exit 0, `ALL CHECKS PASS — 43 localized plugin(s)`, `canon v2    43`, no `OFF CANON` |
| `i18n.js` files in the commit | none |
| distinct `LANGUAGES` declarations across all 43 `i18n.js` | 1 (`export const LANGUAGES = ['en', 'fr'];`) |
| `plugins/O-Orbit/libs/SAF` in the commit | not present |
| files in commit `e9aab4fc` | 48 (43 + 4 deletions + canon) |

## Task 2 — P3 through P7

Every plan line citation was confirmed against the live file with **zero drift**: `:43`,
`:497-498`, `:516`, `:548`, `:576-578`, `:605`, `:623-625` in `check-i18n.js`; `:598, 658, 669,
703, 717, 851` and the `:634-635` `enControl` block in `check-ui-labels.js`; `:1252` in
`i18n-extract.js`.

- **P3** — assertion [1] accepts `en,fr` or `en,fr,zh-Hans` via a `LANG_SHAPES` list, and nothing
  else. The message names both shapes. Proven non-loosening by negative controls C7 and C8 below.
- **P4** — the three field loops derive from a `LANGS` binding taken from the plugin's own
  `LANGUAGES`. `:570-573` and `:612-618` (assertion [4]'s fr-passthrough checks) left alone as the
  plan directs.
- **P5** — a shared `reviewedValid(lang, entry)` helper is applied at **both** sites. `fr` still
  requires `typeof === 'boolean'`; `zh-Hans` accepts `'mt' | 'bt' | 'native'` with none privileged;
  `en` is exempt; anything else fails closed. The `reviewed === false` worklist counters at the
  old `:581` / `:628-629` are untouched and the repo-wide unreviewed-French TOTAL is unchanged
  at **0**.
- **P6** — `check-ui-labels.js` parses `export const LANGUAGES = [...]` from
  `<built.uiRoot>/js/i18n.js` and **fails closed** (a named `[0]` assertion) if the file is
  missing or the declaration does not parse. All six loops now read `LANGS`. It prints the parsed
  list: `languages en, fr (from plugins/O-Chorus/Source/ui/public/js/i18n.js)`.
- **P7** — `i18n-extract.js` emits `'zh-Hans': { t: 'TODO', b: '', reviewed: 'mt' },` immediately
  after the `fr` row, which is unchanged.

Task 2 verification:

| gate | result |
|------|--------|
| `['en', 'fr']` literal in `check-i18n.js` | `0` |
| `['en', 'fr']` literal in `check-ui-labels.js` | `0` |
| `zh-Hans` named in `check-i18n.js` | yes |
| `'mt'` / `'native'` / `'boolean'` counts in `check-i18n.js` | `5` / `6` / `3` (all `>= 2`) |
| `LANGUAGES` referenced in `check-ui-labels.js` | 5 occurrences |
| P7 zh row present, fr row intact | both yes |
| `node scripts/check-i18n.js` | `ALL CHECKS PASS — 43 localized plugin(s)` |
| files touched under `plugins/` by commit `ee2e92cf` | none |

## Task 3 — exit gates

### Gate A — the real suite, unchanged at two languages

```
ALL CHECKS PASS — 43 localized plugin(s)
  canon v2    43  O-AnalogEQ, O-AnalogSaturation, ... (all 43)
  TOTAL                             0
```
exit 0, zero `FAIL` lines, no `OFF CANON`. The unreviewed-French worklist TOTAL was **0 before
Task 2 and 0 after** — P5 did not perturb the worklist counters.

### Gate B — the three-language scratch (P3, P4)

Scratch built OUTSIDE the repo after Task 1, so it carries the post-P1 canon. O-Chorus's table
was extended to `LANGUAGES = ['en', 'fr', 'zh-Hans']` with a `zh-Hans` entry on all 28 keys
(10 tooltip + 18 label), each `t` a placeholder distinct from the English, each
`reviewed: 'bt'`.

```
node scripts/check-i18n.js --root "$SCRATCH" --plugin O-Chorus
  -> exit 0
ALL CHECKS PASS — 1 localized plugin(s)
```

The three-language evidence from that run:

```
PASS: [O-Chorus] [1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr","zh-Hans"]
PASS: [O-Chorus] [1] every I18N key has an entry in every declared language (en, fr, zh-Hans)
PASS: [O-Chorus] [1] every LABELS key has a string t in every declared language (en, fr, zh-Hans)
PASS: [O-Chorus] [5] every I18N entry carries a valid reviewed flag — fr a 'boolean', zh-Hans one of 'mt' | 'bt' | 'native'
PASS: [O-Chorus] [5] every LABELS entry carries a valid reviewed flag — fr a 'boolean', zh-Hans one of 'mt' | 'bt' | 'native'
```

### Gate C — the reviewed enum at both sites, plus non-vacuity controls

Eight one-line perturbations of the Gate B scratch, each re-running
`node scripts/check-i18n.js --root "$SCRATCH" --plugin O-Chorus` and restoring afterwards. The
five the plan asked for are C1-C5; C6-C8 are extra negative controls proving Gate B is not
vacuous.

| # | entry | `reviewed` / change | outcome | assertion that fired |
|---|-------|---------------------|---------|----------------------|
| C1 | `tip.rate` **zh-Hans** (TOOLTIP site) | `'bt'` -> `'mt'` | **PASS** (exit 0) | — |
| C2 | `tip.rate` **zh-Hans** (TOOLTIP site) | `'bt'` -> `'oops'` | **FAIL** (exit 1) | `[5] every I18N entry carries a valid reviewed flag — fr a 'boolean', zh-Hans one of 'mt' \| 'bt' \| 'native' — invalid on 1: tip.rate.zh-Hans` |
| C3 | `label.rate` **zh-Hans** (LABELS site) | `'bt'` -> `'mt'` | **PASS** (exit 0) | — |
| C4 | `label.rate` **zh-Hans** (LABELS site) | `'bt'` -> `'oops'` | **FAIL** (exit 1) | `[5] every LABELS entry carries a valid reviewed flag — ... — invalid on 1: label.rate.zh-Hans` |
| C5 | `label.rate` **fr** | `true` -> `'mt'` | **FAIL** (exit 1) | `[5] every LABELS entry carries a valid reviewed flag — ... — invalid on 1: label.rate.fr` |
| C6 | `label.rate` **zh-Hans** | entry DELETED | **FAIL** (exit 2) | `[1] every LABELS key has a string t in every declared language (en, fr, zh-Hans) — 1 malformed: label.rate.zh-Hans` **and** `[5] ... invalid on 1: label.rate.zh-Hans` |
| C7 | `LANGUAGES` | `['en','fr','de']` | **FAIL** (exit 5) | `[1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","fr","de"]` **and** `[1] every I18N key has an entry in every declared language (en, fr, de) — missing: tip.rate.de, ...` |
| C8 | `LANGUAGES` | `['en','zh-Hans','fr']` (reordered) | **FAIL** (exit 1) | `[1] LANGUAGES is exactly en,fr or en,fr,zh-Hans — got ["en","zh-Hans","fr"]` |

C3 and C4 are the ones that matter: they prove the LABELS reviewed site was patched, not just the
tooltip site. C5 proves `fr`'s boolean requirement survived. C7/C8 prove P3 is a **widening**, not
a loosening. After restoring, the scratch returned to `ALL CHECKS PASS — 1 localized plugin(s)`.

### Gate D — check-ui-labels reads LANGUAGES (P6)

**Real repo, two languages** — passes exactly as before:

```
node scripts/check-ui-labels.js --plugin O-Chorus
   languages en, fr (from plugins/O-Chorus/Source/ui/public/js/i18n.js)
   53 PASS, 0 FAIL
== ALL CHECKS PASSED ==   (exit 0)
```

**Gate B scratch, three languages** — the run visibly exercises three, not two:

```
node scripts/check-ui-labels.js --plugin O-Chorus --root "$SCRATCH"
   languages en, fr, zh-Hans (from plugins/O-Chorus/Source/ui/public/js/i18n.js)
   85 PASS, 2 FAIL          (vs 53 PASS on the two-language real repo)
```

Per-language `zh-Hans` assertion lines, quoted from that run:

```
PASS: [1][zh-Hans] every visible [data-i18n] renders non-empty text
PASS: [1][zh-Hans] no {token} placeholder survives into rendered text
PASS: [2][vacuity][zh-Hans] the zh-Hans pass actually rendered — 12/12 labels (100%) differ from English, need >= 25%
PASS: [2][vacuity][zh-Hans] keyed ATTRIBUTES actually changed language — 7/7
PASS: [3][zh-Hans] dataset.label === textContent for every [data-i18n] after switch ...
PASS: [3][zh-Hans] dataset.label === textContent for every [data-i18n] after a state pass ...
PASS: [4][zh-Hans] no leaf label is clipped by its own overflow
PASS: [5][zh-Hans] no label spills its offsetParent's padding box MORE in zh-Hans than in English
PASS: [6][zh-Hans] no label crosses the 700 x 125 frame MORE in zh-Hans than in English
FAIL: [7][GEOMETRY DIFF][zh-Hans] no non-label element moved between English and zh-Hans at a fixed frame (1 animated element(s) excluded — see NOTE) — 30 moved:
PASS: [7][GEOMETRY DIFF][zh-Hans] the visible element SET is identical in English and zh-Hans
PASS: [8][zh-Hans] two labels disjoint in English do not intersect in zh-Hans
PASS: [8b][zh-Hans] no label intersects a NON-label element it cleared in English
NOTE: [7] 1 element(s) MOVE WITH THE LANGUAGE HELD CONSTANT — an animation this page drives itself,
      not a zh-Hans push. Excluded from the diff: #lfo-dot dx=8.2 dy=7.6
```

Alongside them, all 34 `[fr]`-labelled assertions PASSED in the same run, including
`PASS: [7][GEOMETRY DIFF][fr] no non-label element moved between English and fr at a fixed frame`.

**The 2 FAILs are the same assertion in two page states, both `[zh-Hans]`, and are the fixture's
doing, not the gate's.** The scratch `zh-Hans` strings are invented placeholders
(`'中文-1'`, `'测试正文-1'`) with arbitrary widths, so they really do push 30 non-label elements.
That the gate measured that, named `zh-Hans` as the culprit, and left every `fr` assertion green
is precisely the discrimination P6 exists to provide — a vacuous pass would have reported 53
assertions and no language column at all.

### Gate E — every UI still boots

```
node scripts/boot-all-uis.js --strict-tips      -> exit 0
  clean:  43 / 43
  DEAD bindings:  0  across 0 plugin(s)
  rendered text-bearing elements: 3796   aria-label: 775   title: 0
```

### Gate F — scope containment across all three commits

| check | result |
|-------|--------|
| any `i18n.js` under `plugins/` touched | none |
| distinct `LANGUAGES` declarations across all 43 tables | 1 — `export const LANGUAGES = ['en', 'fr'];` |
| `zh-Hans` present in any plugin `i18n.js` | none |
| `CMakeLists.txt` / `CHANGELOG.md` / `STATUS.md` touched | none |
| `plugins/O-Orbit/libs/SAF` submodule pointer | unmoved |
| total files across all three commits | 51 |
| working tree after cleanup | clean except `.gsd/dispatch-isolation-sentinel.json` (pre-existing, left unstaged as instructed) |

All scratch trees deleted.

## Deviations from Plan

### 1. [Rule 2 — missing critical functionality] check-ui-labels assertions 2, 8, 8b generalized too

- **Found during:** Task 2, P6.
- **Issue:** the plan named assertions 5/6/7 as the ones to make per-language. Assertions 2
  (the vacuity guard), 8 and 8b sit in the same contiguous `fr`-vs-`en` region and also read `fr`
  by name. Leaving them would have meant a `zh-Hans` page whose `__setLanguage` silently did
  nothing still passed the vacuity guard — the exact hole threat **T-c3s-05** names.
- **Fix:** the whole region is wrapped in `for (const other of NON_EN) { const fr = snaps[other].before; ... }`,
  so 2, 5, 6, 7, 8 and 8b all run once per non-English language.
- **Commit:** `ee2e92cf`.

### 2. [Rule 1 — bug] three assertion messages named two languages while measuring three

- **Found during:** Task 3, Gates B and D — invisible at two languages, which is why only the
  scratch run could surface it.
- **Issue:** `check-i18n` `[1]`'s two field-presence messages still said "both en and fr" after
  their loops became `LANGS`-driven; `check-ui-labels` `[6]`'s scroll-extent message printed
  `en <WxH>, fr <WxH>` with a hard-coded `fr` in front of the *other* language's numbers.
- **Fix:** all three interpolate the language actually measured. No assertion logic changed.
- **Commit:** `ef9f47f9` (the third commit the plan permits when a gate forces a fix).

### 3. Task 1 verify assertion 5 is mis-specified in the plan

- The plan's gate is `test "$(ls plugins/*/.planning/i18n-index-draft.html | wc -l)" = "0"`, but
  the tree holds **37** such drafts, not 4. Locked decision (f), the plan's `files_deleted`
  frontmatter, and the executor constraints all name exactly **four**. Only those four were
  deleted; 33 remain.
- The four named (O-AnalogSaturation, O-Bitrot, O-Emulator, O-SimpleReverb) were independently
  confirmed to be **exactly** the four drafts carrying the clamp literal — 47 total hits minus 43
  shipping copies — so decision (f) itself is coherent; only the glob in the gate is wrong.
- **Substituted assertion:** the four named drafts are gone (verified `0` remaining), and no draft
  anywhere carries the clamp (covered by Task 1 gate 3, which returned empty).

### 4. Task 2 verify's line-pinned check on `check-ui-labels.js:634-635` cannot hold

- The plan asserts `sed -n '635p'` still shows the `enControl` EN->EN spread. P6 step 1 mandates a
  new disk-side read at the `buildRoot` site (`:487`), ~37 lines **above** it, so the block
  necessarily moves. The two requirements are mutually exclusive as literally written.
- **Substituted verification of the gate's intent:** the block is **byte-unchanged**, only
  relocated to `:683-684`. Proven by `git diff -U0 -- scripts/check-ui-labels.js | grep -E '^[-+].*enControl'`
  returning no hit — no added or removed line touches `enControl` or its `__setLanguage(l), 'en'`
  call. The vacuity control is intact and still measured 1 animated element (`#lfo-dot`) in the
  Gate D runs.

### 5. `git diff HEAD~1 --name-only` compares against the WORKING TREE

- Momentarily read as commit `e9aab4fc` having swept in `.gsd/dispatch-isolation-sentinel.json`.
  It had not: the two-argument form `git diff HEAD~1 HEAD --name-only` shows 48 files and no
  `.gsd/` entry. The one-argument form lists the sentinel only because it is modified in the
  worktree. Gate F therefore used the two-argument form throughout.
- Worth knowing for any future plan: the plan's Task 1 gates 7-8 and Task 3 Gate F use the
  one-argument form and will report worktree noise as commit content.

## Authentication Gates

None.

## Known Stubs

None. No stub, TODO, skipped test or unrun `<verify>` was introduced.

## Threat Flags

None. No new network endpoint, auth path, file-access pattern or trust-boundary schema change.
The one new file read (`<uiRoot>/js/i18n.js` in `check-ui-labels.js`) is inside a path the same
process already copies wholesale via `S.buildRoot`, and is test tooling, not shipped runtime code.

## Self-Check: PASSED

- `scripts/i18n-canon.js`, `scripts/check-i18n.js`, `scripts/check-ui-labels.js`,
  `scripts/i18n-extract.js` — all present and modified.
- All 43 swept files present; 4 drafts confirmed deleted from HEAD.
- Commits `e9aab4fc`, `ee2e92cf`, `ef9f47f9` all resolve in `git log`.
- Gates A, B, C, D, E, F all re-run against the final tree state.
