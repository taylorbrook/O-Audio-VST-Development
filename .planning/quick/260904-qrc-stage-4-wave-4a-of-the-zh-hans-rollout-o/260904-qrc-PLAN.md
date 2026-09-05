---
phase: quick-260904-qrc
plan: 01
type: execute
wave: 1
depends_on: []
subsystem: i18n
tags: [i18n, zh-Hans, stage-4, wave-4a, O-Prism, O-Comp, O-Freeze, O-Texture, O-Bass, O-AnalogSaturation]
autonomous: true
requirements: [ZH4A-01, ZH4A-02, ZH4A-03, ZH4A-04, ZH4A-05, ZH4A-06, ZH4A-07, ZH4A-08]
files_modified:
  - plugins/O-AnalogSaturation/Source/ui/public/js/i18n.js
  - plugins/O-AnalogSaturation/Source/ui/public/index.html
  - plugins/O-AnalogSaturation/Source/PluginProcessor.h
  - plugins/O-AnalogSaturation/CMakeLists.txt
  - plugins/O-AnalogSaturation/CHANGELOG.md
  - plugins/O-Prism/Source/ui/public/js/i18n.js
  - plugins/O-Prism/Source/ui/public/index.html
  - plugins/O-Prism/Source/ui/public/css/wavetable-editor.css
  - plugins/O-Prism/Source/PluginProcessor.h
  - plugins/O-Prism/CMakeLists.txt
  - plugins/O-Prism/CHANGELOG.md
  - plugins/O-Comp/Source/ui/public/js/i18n.js
  - plugins/O-Comp/Source/ui/public/index.html
  - plugins/O-Comp/Source/PluginProcessor.h
  - plugins/O-Comp/tests/ui_tip_render_check.js
  - plugins/O-Comp/CMakeLists.txt
  - plugins/O-Comp/CHANGELOG.md
  - plugins/O-Freeze/Source/ui/public/js/i18n.js
  - plugins/O-Freeze/Source/ui/public/index.html
  - plugins/O-Freeze/Source/PluginProcessor.h
  - plugins/O-Freeze/CMakeLists.txt
  - plugins/O-Freeze/CHANGELOG.md
  - plugins/O-Texture/Source/ui/public/js/i18n.js
  - plugins/O-Texture/Source/ui/public/index.html
  - plugins/O-Texture/Source/ui/public/css/ouaricon-naturalist.css
  - plugins/O-Texture/Source/PluginProcessor.h
  - plugins/O-Texture/tests/ui_tip_render_check.js
  - plugins/O-Texture/CMakeLists.txt
  - plugins/O-Texture/CHANGELOG.md
  - plugins/O-Bass/Source/ui/public/js/i18n.js
  - plugins/O-Bass/Source/ui/public/index.html
  - plugins/O-Bass/Source/PluginProcessor.h
  - plugins/O-Bass/CMakeLists.txt
  - plugins/O-Bass/CHANGELOG.md
  - PLUGINS.md

estimate:
  tokens: 620000
  raw_tokens: 620000
  tasks: 3
  confidence: low       # one prior sample of this shape (Stage 3: 610k / 758 rows / 3 tasks)

must_haves:
  truths:
    - "A user opens any of the six plugins, picks 简体中文 from the language selector, and every caption, section heading and hover-help body on the page renders in Simplified Chinese."
    - "Switching back to English or French reproduces the pre-change page byte-for-byte in geometry — no element moved on the en or fr arm."
    - "The Chinese page has no clipped, wrapped or displaced element: check-ui-labels reports 0 geometry moved on the zh arm."
    - "No Chinese character exists anywhere under any plugin's Source/**/*.{h,cpp} — the C++ carries only the ASCII language code."
    - "Every zh row has been read back through a blind reverse pass and either accepted with a written reason or re-authored."
    - "All six load in Logic: auval PASS on all six triples after one cold registry rescan."
    - "The language hover-help no longer names a fixed set of languages, so it cannot go false again when a fourth lands."
  artifacts:
    - plugins/O-AnalogSaturation/Source/ui/public/js/i18n.js   # zh-Hans on every I18N and LABELS key
    - plugins/O-Prism/Source/ui/public/js/i18n.js
    - plugins/O-Comp/Source/ui/public/js/i18n.js
    - plugins/O-Freeze/Source/ui/public/js/i18n.js
    - plugins/O-Texture/Source/ui/public/js/i18n.js
    - plugins/O-Bass/Source/ui/public/js/i18n.js
    - plugins/O-Comp/tests/ui_tip_render_check.js              # zh arm, derived not hard-coded
    - plugins/O-Texture/tests/ui_tip_render_check.js           # zh arm, derived not hard-coded
    - PLUGINS.md                                               # six registry rows, corrected from CMakeLists
  key_links:
    - "LANGUAGES array in i18n.js <-> the endonym <option> in index.html <-> languageCode/languageIndex in PluginProcessor.h — a third language present in two of the three renders a page that cannot be reached or cannot be persisted."
    - "The CJK font tail on the measured literal stacks <-> the nodes that actually hold Han — a tail on the wrong stack leaves tofu; a tail on every stack changes Latin metrics and trips the en arm."
    - "The line-height / min-width pins <-> the English baseline they are measured from — a pin derived from the wrong box, or appended at a specificity that outranks a French-era pin, regresses a language this work does not touch."
    - "The blind reverse batch's manifest <-> the ids in the returned file — a manifest resolved from the wrong side joins nothing and the read is silently vacuous."
---

<objective>
Localize six plugins into Simplified Chinese — O-Prism, O-Comp, O-Freeze, O-Texture,
O-Bass, O-AnalogSaturation — to the same ship bar Stages 2 and 3 established, and ship
each with a version bump, a build, an install and an auval pass.

Purpose: wave 4a is the first of seven volume waves. Stages 0–3 answered every structural
question and repaired the lint; this wave is the pattern executing. Whatever this wave does
badly, six more waves copy.

Output: six three-language plugins, six path-scoped commits (one per plugin, several per
plugin where a correction round is needed), one PLUGINS.md commit at the end of the batch,
one cold auval sweep covering all six.
</objective>

<execution_context>
@~/.claude/gsd-core/workflows/execute-plan.md
@~/.claude/gsd-core/templates/summary.md
</execution_context>

<context>
@.planning/STATE.md
@CLAUDE.md
@.planning/quick/260901-akh-research-and-plan-chinese-localization-a/260901-akh-IMPLEMENTATION-PLAN.md
@.planning/quick/260904-g5l-stage-3-of-the-zh-hans-rollout-the-hard-/260904-g5l-SUMMARY.md
@scripts/i18n-zh-glossary.js
@scripts/i18n-zh-lint.js
@scripts/i18n-zh-backtranslate.js
</context>

---

## Live observations — measured at planning time (2026-09-04), authoritative over the master plan's approximations

Everything in this table was read off the working tree during planning. **The master plan's
entry counts are approximations and every one of them is low** — use these.

| | O-Prism | O-Comp | O-Freeze | O-Texture | O-Bass | O-AnalogSaturation |
|---|---|---|---|---|---|---|
| entries (live) | **267** | **39** | **36** | **31** | **29** | **20** |
| master plan said | 262 | 34 | 31 | 26 | 24 | 15 |
| UI root | `Source/ui/public/` | same | same | same | same | same |
| external CSS | `css/wavetable-editor.css` | **none** | **none** | `css/ouaricon-naturalist.css` | **none** | **none** |
| `font-family` in `index.html` | 30 | 10 | 7 | **0** | 10 | 10 |
| `font-family` in CSS | 4 | — | — | **6** | — | — |
| existing `min-width` / `min-height` / `line-height` | 16 / 4 / 6 | 5 / 0 / 4 | 7 / 0 / 4 | 4 / 3 / 4 | 5 / 0 / 4 | 3 / 0 / 4 |
| `tests/i18n-states.json` states | **23** | 3 | 1 | 1 | 2 | 1 |
| `tests/ui_tip_render_check.js` two-language literal | clean | **HIT** | clean | **HIT** | clean | clean |
| `CMakeLists.txt` VERSION (line, unquoted) | 1.23.1 (L12) | 1.8.1 (L11) | 2.4.1 (L6) | 0.4.1 (**L42**) | 1.6.1 (L10) | 1.4.1 (L6) |
| `PLUGINS.md` row | 1.22.1 | 1.7.1 | 2.3.0 | 0.3.1 | 1.5.1 | 1.3.1 |
| build target | `O-Prism` | `O-Comp` | `O-Freeze` | **`OuariconTexture`** | `O-Bass` | `O-AnalogSaturation` |
| stale language enumeration (en **and** fr) | HIT | HIT | HIT | HIT | HIT | HIT |

Live total: **422 entries**, not the 392 the master plan sized. All six are
`Source/ui/public/`; no `[R]` plugin is in this wave.

**Six findings that will cost the executor time if not read first:**

1. **Zero type tokens, six for six.** No `--*font*` custom property and no
   `font-family: var(...)` anywhere in this wave. Every plugin is the LITERAL-STACK shape
   (the O-Chorus / O-MicrotonalSampler case, never the O-Octagon token case). The tail goes
   on measured literal stacks — there is no token to edit.
2. **O-Texture has zero `font-family` in `index.html`.** All six of its stacks live in
   `css/ouaricon-naturalist.css`. An executor who greps the markup concludes there is
   nothing to do. O-Prism is split across both files (30 markup + 4 CSS).
3. **All six PLUGINS.md rows are one patch behind CMakeLists.** Rule C-8 fires six for six:
   **bump from the CMakeLists value.** Never from the registry row, never from this plan's
   table, never from a handoff message. Correcting the registry row is part of the work.
4. **O-Texture's build target is `OuariconTexture`**, set via `set(PROJECT_NAME ...)` above
   the `juce_add_plugin` call — it is not the folder name. It is also the only ANIRA/ONNX
   plugin here (`ONNXRUNTIME_VERSION 1.19.2`), so a bare `grep VERSION` on its CMakeLists
   returns three decoys before the plugin version at line 42, and its bundle must carry the
   embedded dylibs after the build.
5. **All six carry the stale two-language enumeration in BOTH the en and the fr body** — the
   defect Stage 3 hit four for four is now ten for ten across the suite. Finding 17's fix
   applies unchanged: **remove the enumeration, do not extend it.** Naming three languages
   would put Han inside the en and fr bodies and drag the CJK tail onto the very baseline
   every gate measures against.
6. **zsh does not word-split, and it made a count read zero during planning.** Building a
   file list into a variable and then `cat $files` failed on every plugin and every count
   came back 0 — which reads exactly like "this plugin has no pins", and every one of them
   has pins. Use `find ... -exec cat {} +`. A clean sweep of zeroes or of exit 127 is a
   broken loop, not a pass.
7. **`grep -P` with a Han property IS BROKEN ON THIS MACHINE — it byte-matches and reports
   phantom hits.** Measured during planning: the obvious Han gate
   `grep -rlP '\p{Han}' plugins/O-Octagon/Source --include=*.h --include=*.cpp` returns
   **13 files** on a plugin Stage 3 certified as carrying zero Chinese under `Source/`. Every
   one of the 13 is a false positive on the **middle dot `·` (U+00B7)** sitting in a doc
   comment, and it fires with `LC_ALL=en_US.UTF-8` set too. `perl -CSD` returns the correct
   **0**. An executor running the obvious grep spends the afternoon chasing thirteen
   non-defects — or worse, "fixes" comments that were never wrong. **Use the `perl -CSD` form
   in the verify blocks below; do not substitute a grep.** Note also that an unquoted
   `--include` glob is expanded by zsh and the whole grep then fails with `no matches found`,
   so the count reads 0 **and so does its positive control** — a gate and its control both
   silently dead.
8. **A Han script property in perl WITHOUT `-CSD` is INERT.** Proven on an injected positive
   control during planning: the same one-liner returns **0** on a file that genuinely contains
   a space between two Han characters. The `/u` flag does not supply the input decoding;
   `-CSD` does. **This is why a raw intra-Han whitespace scan is NOT in the verify blocks
   below:** run without `-CSD` it is vacuous, and run *with* `-CSD` over a whole `i18n.js` it
   false-positives on the header comment (O-Octagon's header quotes the very defect Stage 3
   fixed, so the scan reports 2 on a clean file — the self-invalidating-header trap). **Rule
   Z8 in `i18n-zh-lint` is the correct instrument** — it scans row values only, it is already
   covered by the per-plugin lint run, and it was proven non-inert against a corpus positive
   control in 260904-q4j.

---

## Stage-4 executor rules — non-negotiable, from Stage 3's structural findings

These are cited by number throughout the tasks.

- **R1 — Author at `'mt'`, always.** Never write `'bt'` at authoring time. `check-i18n`
  accepts `'bt'` as a valid enum member and `i18n-zh-lint` R1 only reports entries *below*
  the bar, so a premature `'bt'` is invisible to every automated check in this repo. The
  flag is the one field no gate can validate. Promotion to `'bt'` happens only after the
  reverse read, in its own commit.
- **R2 — Read ALL triples with `--verbose`.** `--ingest` truncates to 12. Across Stage 3,
  **zero of nine real findings ranked inside the default twelve**; O-Prism's 267 entries
  will emit roughly 350–400 rows and twelve is 3% of that. The score is a sort key, not a
  verdict.
- **R3 — Screen TERMS for page collisions BEFORE authoring.** Push the plugin's English
  label set through the glossary's `TERMS` and report any two *different* English keys that
  map to the same root **and** both occur on the same page. This is the Dither/Jitter→抖动
  class. Z5, F1 and check-ui-labels are all blind to it — only a blind reader finds it, and
  only after it has shipped. Resolution: qualify **both** sides, never one.
- **R4 — Fresh blind reader and fresh salt per correction round.** Both, and they guard
  different things: a fresh salt stops the agent correlating the correction batch against
  ids it has seen; a fresh agent stops it agreeing with its own earlier answer.
- **R5 — `--emit` is table-scoped.** It walks every row with a `zh-Hans` value and ignores
  `reviewed` entirely. There is no `--only-mt`. A second pass re-emits everything. Every row
  count written against the ship bar is wrong; do not treat the excess as a broken tool.
- **R6 — Pass `--manifest` and keep ids blind.** `--emit` writes its manifest beside its
  `--out` file, not beside the returned file, so pass it explicitly on `--ingest`. Fire the
  blinding controls on every batch rather than trusting the code exists: ids pure 12-hex, no
  key fragment, ids returned identical and in order, rows well-formed, and a sha256 of the
  zh column against the committed tree. **Forbid repo access in the dispatch explicitly** —
  several plugins name themselves in their own copy, so blinding cannot hide the product.
- **R7 — zh geometry failures are SHRINKS. Assert equality, pin at the widest of three.**
  Chinese buys width and spends height. An assertion phrased "must not grow" is vacuous
  against Chinese and "must not shrink" is vacuous against half of it. Five failure shapes,
  four of them shrinks or squeezes: inherited `line-height: normal`; a content-sized element
  that shrank and pulled its neighbours; the un-wrap (a prose block takes one fewer line);
  Han's min-content width being ONE character in any auto-sized table or grid; and a
  differently-sized child leaking out of a parent's pin.
- **R8 — A French min-width pin is a FLOOR, not safety.** O-MicrotonalSampler's
  `.drop-zone-text` was pinned at 134 px and Chinese rendered 137.70 and sailed past it.
  Every plugin in this wave carries French-era pins (3–16 each). Re-measure them; do not
  assume a pinned element is safe in a third language. And before appending any pin block,
  grep for existing declarations on those selectors — a Task-1 pin in Stage 3 un-pinned a
  French one on specificity and moved the **fr** arm.
- **R9 — Version truth is `CMakeLists.txt`.** Read the value; do not grep for a literal. The
  value may be quoted (it is not, on any of these six — but O-MicrotonalSampler's was, and
  the zero-match grep read as "not done" when the bump had landed correctly).

**Plus, carried from the same source:**

- **Comment-blind greps lie.** Strip comments before any code count:
  `perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' <file>`. Four verify
  commands in the Stage 3 plan were wrong for exactly this reason, and every one made a
  correct tree look broken.
- **When you repair a gate file, do not spell the literal in the comment that explains it.**
  Word it as prose, or the inventory grep keeps the file on the list forever.
- **Drive the plugin's own `tests/i18n-states.json`, not the screens you can think of.** The
  gate cannot name what never renders. Half of O-MicrotonalSampler's offenders lived in
  dialogs and popovers absent from the resting DOM. O-Prism has 23 states.
- **Two nodes a keyed scan always misses, four for four:** the `#tooltip` surface (filled
  from `data-tip` at hover time, never keyed) and the endonym `<option>` (the only Han in
  the markup). Treat as universal.
- **Han-space-Han is now rule Z8** and is live in the lint. It is scoped to the plain
  U+0020 only.
- **Run `i18n-zh-lint --self-test` and require 10/10** before trusting any zero from it.

---

## The per-plugin procedure (nine steps, identical for all six)

Every task below runs this loop. It is written once here and referenced, not repeated.

1. **Screen for glossary page collisions (R3)** — before authoring a single string. Push the
   plugin's English label set through `TERMS`; report any two different keys sharing a root
   that co-occur on a page. Resolve by qualifying **both** sides, and record the reasoning.
   Where the collision is objective, a `termNote` is the sanctioned override; where it is a
   terminology *preference* with no collision, the glossary wins and the concern is recorded
   rather than acted on.
2. **`js/i18n.js`** — add `'zh-Hans'` to `LANGUAGES`, and a `'zh-Hans'` value on **every**
   key in both `I18N` and `LABELS`, every one at `reviewed: 'mt'` (R1). Quoted key; the
   canon reads `entry[lang]` unchanged. While in this file, **remove the sentence that
   counts the selector's options** from the language hover-help in the en body and the fr
   body both — the selector already lists the languages in their endonyms, which is the one
   form a reader recognises without knowing the page language.
3. **`index.html`** — the endonym `<option value="zh-Hans">` beside the fr option, written
   as **numeric entities** to match the existing convention and sidestep the literal
   non-ASCII risk in the HTML path.
4. **CJK font tail** — serve the page, switch to Chinese, and read
   `getComputedStyle(node).fontFamily` on every node that **holds or can receive** a Han
   codepoint (own text, `data-tip`, `data-tip-title`, `aria-label`), driving every state in
   `tests/i18n-states.json`. Append the tail to the **measured** literal stacks only — all
   six are the literal-stack shape, so there is no token shortcut. Where a bare-`Arial` UA
   default is reached only through `aria-label`, leave it alone: an accessible name is
   spoken, not rendered, and there is no glyph to fall back for. Where it carries **visible**
   Chinese, take the tail with Arial kept **first** so Latin metrics are unchanged. Record
   both halves — a wave that appends the tail to everything it measured can no longer tell a
   needed tail from a decorative one.
5. **`PluginProcessor.h`** — the three-way `languageCode` and matching `languageIndex`, pure
   ASCII. Zero Chinese characters anywhere under `Source/`.
6. **Geometry pass (R7, R8)** — run the zh arm of `check-ui-labels` and let it *name* the
   offenders. Pin each named leaf at its **measured English line box**, unitless, derived
   from the box (height − padding − border), never from a text ink rect. A class appearing
   at two sizes needs two pins. Never a global `line-height`. Grep for existing declarations
   on those selectors before appending (R8). Re-run until 0 geometry moved on **all three**
   arms — en, fr and zh.
7. **Commit at `'mt'`** — path-scoped, `git commit -- plugins/<Name>`. The reverse pass runs
   against a committed tree.
8. **Blind reverse read (R2, R4, R5, R6)** — `--emit` to a fresh `--out`, dispatch to a blind
   reader with repo access explicitly forbidden, `--ingest` with an explicit `--manifest` and
   `--verbose`. Read **every** triple. For each drift, the discriminator is **collision on
   the page**, not drift distance: can a reader on this page confuse this string with another
   control on the same page? A caption must also not collide with its own tooltip title.
   Re-author on collision; accept otherwise — and **write the reason down**, because an
   accepted drift with no recorded reason is indistinguishable from an unread one. A
   correction round is a full re-emit to a new `--out` with a fresh salt and a fresh reader,
   trimmed by hand, in its own commit that says plainly what the read found.
9. **Promote and ship** — flip to `reviewed: 'bt'`, bump the version from the CMakeLists
   value (R9), write the CHANGELOG entry, and `./scripts/build-and-install.sh <target>`.
   Promotion and ship metadata land together in a final commit whose message names every
   provenance string.

**Deferred to the end of the batch:** `auval`. A cold `auval` after an install rebuilds the
whole AU registry (~15 min); six separate runs would cost six rescans. Build and install all
six, then sweep once.

---

<tasks>

<task type="tracer">
  <name>Task 1: O-AnalogSaturation end-to-end — the thinnest complete path (ZH4A-01, ZH4A-07)</name>
  <files>plugins/O-AnalogSaturation/Source/ui/public/js/i18n.js, plugins/O-AnalogSaturation/Source/ui/public/index.html, plugins/O-AnalogSaturation/Source/PluginProcessor.h, plugins/O-AnalogSaturation/CMakeLists.txt, plugins/O-AnalogSaturation/CHANGELOG.md</files>
  <precondition>`node scripts/i18n-zh-lint.js --self-test` reports 10/10 and `node scripts/check-i18n.js` exits 0 on the tree as found. If either fails, stop — the 260904-q4j lint repair is not in the tree and every zero this task produces is unevidenced.</precondition>
  <action>
Run the full nine-step per-plugin procedure on **O-AnalogSaturation** — 20 entries, the
smallest table in the wave, one state, no external CSS, ten literal stacks in `index.html`,
three existing min-width pins and four line-height declarations, CMakeLists version 1.4.1 at
line 6, build target `O-AnalogSaturation`.

This is the tracer: it wires one plugin through every layer this wave touches — glossary
screen, table, markup, font tail, C++ codec, geometry pins, commit, blind reverse read,
promotion, build, install — before the other five copy it. It is production work, not a
trial; nothing here is thrown away.

Author at `'mt'` (R1). Screen TERMS first (R3). Remove the option-counting sentence from the
en and the fr language hover-help bodies both — this plugin carries it in both, confirmed at
planning time. Measure the font stacks by serving the page and reading computed styles;
there is no type token to edit. Pin geometry at the measured English box, checking the three
existing min-width declarations for specificity conflict before appending (R8). Commit at
`'mt'`, then run the blind reverse pass with `--verbose`, an explicit `--manifest`, a fresh
salt and a dispatch that forbids repo access (R2, R5, R6). Read every triple. Promote in a
second commit with the version bumped from the CMakeLists value (R9) and the CHANGELOG
entry. Build and install. **Do not run auval** — it is deferred to Task 3.

Record, for Tasks 2 and 3 to reuse: the exact set of stacks that needed the tail versus the
ones that did not, the geometry offenders the gate named and the pins that closed them, any
glossary collision the screen found, and the shape of the blind dispatch that worked.
  </action>
  <verify>
    <automated>node scripts/i18n-zh-lint.js --self-test 2>&1 | tail -3   # 10/10 before anything is trusted</automated>
    <automated>node scripts/check-i18n.js --plugin O-AnalogSaturation; echo "exit=$?"   # exit 0, LANGUAGES lists three</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-AnalogSaturation; echo "exit=$?"   # exit 0, 0 FAIL, zh arm 0 geometry moved</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-AnalogSaturation 2>&1 | tail -20   # 0 findings, BELOW SHIP BAR 0</automated>
    <automated>node scripts/i18n-fr-lint.js; echo "exit=$?"   # exit 0 — French untouched</automated>
    <automated>find plugins/O-AnalogSaturation/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +   # MUST print nothing</automated>
    <automated>perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-AnalogSaturation/Source/ui/public/js/i18n.js   # positive control for the line above — MUST print the file</automated>
    <automated>perl -0777 -ne 'print "STALE ENUMERATION: $&\n" while /(English and French|anglais et le fran)/gi' plugins/O-AnalogSaturation/Source/ui/public/js/i18n.js   # MUST print nothing</automated>
    <automated>node scripts/boot-all-uis.js --strict-tips 2>&1 | tail -5   # 0 DEAD, 0 late, 0 failed</automated>
    <human-check>Read every back-translation triple. For each drift, record the collision-on-the-page verdict and its reason in the task summary.</human-check>
  </verify>
  <done>
O-AnalogSaturation ships three languages at `reviewed: 'bt'` on every row, version bumped
from 1.4.1 per R9, CHANGELOG written, installed. Two or more path-scoped commits: the `'mt'`
table, then the promotion. All gates above green with the Han positive control fired. The
reusable findings (stack set, pin set, collisions, dispatch shape) are written down.
  </done>
</task>

<task type="auto">
  <name>Task 2: O-Prism — 267 entries, 23 states, split font stacks (ZH4A-02, ZH4A-07)</name>
  <files>plugins/O-Prism/Source/ui/public/js/i18n.js, plugins/O-Prism/Source/ui/public/index.html, plugins/O-Prism/Source/ui/public/css/wavetable-editor.css, plugins/O-Prism/Source/PluginProcessor.h, plugins/O-Prism/CMakeLists.txt, plugins/O-Prism/CHANGELOG.md</files>
  <precondition>Task 1's commits are in the tree and its recorded findings are available — O-Prism reuses the stack-measurement method and the blind-dispatch shape the tracer proved.</precondition>
  <action>
Run the same nine-step procedure on **O-Prism** — the heavy plugin of the wave, and it gets
this task to itself for that reason. 267 entries (the master plan said 262), **23 states** in
`tests/i18n-states.json`, font stacks split across `index.html` (30) and
`css/wavetable-editor.css` (4), the wave's densest existing pin set (16 min-width, 4
min-height, 6 line-height), CMakeLists version 1.23.1 at line 12, build target `O-Prism`, a
clean `tests/ui_tip_render_check.js`.

Three things make this plugin different from the tracer and each is a known trap:

**Twenty-three states.** The gate cannot name what never renders. Drive every state in the
plugin's own state file for both the font-stack measurement and the geometry pass — a sweep
of the resting page will look complete and will not be. This is where the un-wrap and the
auto-table squeeze hide (R7).

**Stacks in two files.** A measurement that reads only the markup misses the wavetable
editor's four; one that reads only the stylesheet misses thirty. Measure by computed style,
not by grepping either file.

**Sixteen French-era min-width pins.** Every one is a floor, not protection (R8). Re-measure
each against the Chinese render; the O-MicrotonalSampler `.drop-zone-text` shape — pinned at
134, Chinese rendered 137.70, sailed straight past and displaced four siblings — is exactly
what sixteen pins on a 267-entry page invites. Grep for existing declarations on any
selector before appending a block, or a new pin at higher specificity silently un-pins a
French one and moves the **fr** arm.

Also carry: this plugin's `tip.language` en body names a fixed pair, and its fr body does
too — both go. Note this plugin's i18n.js header already records a deliberate French decimal
quotation in that same tip body; preserve that reasoning when rewriting, and leave the
readout-exemption claim intact.

Expect the reverse batch to be large (R5 — `--emit` is table-scoped, so roughly 350–400 rows,
not 267) and expect the real findings to rank outside the first twelve (R2). Author at `'mt'`,
commit, read, correct with a fresh reader and fresh salt if needed (R4), promote, bump from
1.23.1, build and install. **Do not run auval.**
  </action>
  <verify>
    <automated>node scripts/check-i18n.js --plugin O-Prism; echo "exit=$?"</automated>
    <automated>node scripts/check-ui-labels.js --plugin O-Prism; echo "exit=$?"   # exit 0, 0 FAIL on all three arms</automated>
    <automated>node scripts/i18n-zh-lint.js --plugin O-Prism 2>&1 | tail -20   # 0 findings, BELOW SHIP BAR 0</automated>
    <automated>node scripts/i18n-fr-lint.js; echo "exit=$?"   # exit 0 — the fr arm must not have moved</automated>
    <automated>find plugins/O-Prism/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +   # MUST print nothing</automated>
    <automated>perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' plugins/O-Prism/Source/ui/public/js/i18n.js   # positive control — MUST print the file</automated>
    <automated>perl -0777 -ne 'print "STALE ENUMERATION: $&\n" while /(English and French|anglais et le fran)/gi' plugins/O-Prism/Source/ui/public/js/i18n.js   # MUST print nothing</automated>
    <automated>node plugins/O-Prism/tests/ui_tip_render_check.js; echo "exit=$?"   # exit 0 with a real zh pass</automated>
    <automated>node scripts/boot-all-uis.js --strict-tips 2>&1 | tail -5</automated>
    <human-check>Every one of the ~350–400 triples read with --verbose. Each accepted drift carries a written collision-on-the-page reason.</human-check>
  </verify>
  <done>
O-Prism ships three languages at `'bt'` on every row, version bumped from 1.23.1, CHANGELOG
written, installed, geometry equal on all three arms across all 23 states. Path-scoped
commits: `'mt'` table, any correction round, then promotion.
  </done>
</task>

<task type="auto">
  <name>Task 3: O-Comp, O-Freeze, O-Texture, O-Bass — plus the two gate repairs and the batch close-out (ZH4A-03..08)</name>
  <files>plugins/O-Comp/Source/ui/public/js/i18n.js, plugins/O-Comp/Source/ui/public/index.html, plugins/O-Comp/Source/PluginProcessor.h, plugins/O-Comp/tests/ui_tip_render_check.js, plugins/O-Comp/CMakeLists.txt, plugins/O-Comp/CHANGELOG.md, plugins/O-Freeze/Source/ui/public/js/i18n.js, plugins/O-Freeze/Source/ui/public/index.html, plugins/O-Freeze/Source/PluginProcessor.h, plugins/O-Freeze/CMakeLists.txt, plugins/O-Freeze/CHANGELOG.md, plugins/O-Texture/Source/ui/public/js/i18n.js, plugins/O-Texture/Source/ui/public/index.html, plugins/O-Texture/Source/ui/public/css/ouaricon-naturalist.css, plugins/O-Texture/Source/PluginProcessor.h, plugins/O-Texture/tests/ui_tip_render_check.js, plugins/O-Texture/CMakeLists.txt, plugins/O-Texture/CHANGELOG.md, plugins/O-Bass/Source/ui/public/js/i18n.js, plugins/O-Bass/Source/ui/public/index.html, plugins/O-Bass/Source/PluginProcessor.h, plugins/O-Bass/CMakeLists.txt, plugins/O-Bass/CHANGELOG.md, PLUGINS.md</files>
  <precondition>Tasks 1 and 2 are committed and their plugins are installed — the cold auval sweep at the end of this task covers all six and cannot run until every bundle is on disk.</precondition>
  <action>
Run the nine-step procedure on the four remaining plugins, **one path-scoped commit stream
per plugin** so a failure in one does not strand the others. Per-plugin specifics, all
measured at planning time:

**O-Comp** — 39 entries, 3 states, no external CSS, 10 literal stacks in the markup, 5
min-width + 4 line-height existing, version 1.8.1 at line 11, target `O-Comp`. Its
`tests/ui_tip_render_check.js` **hard-codes a two-element language pair** and will hard-fail
the day a zh entry lands. Repair it by **deriving** the language list from the plugin's own
`LANGUAGES` export, with a derive-or-abort control: a planted empty list must make the gate
exit non-zero rather than pass vacuously. Per-language copy comparison, and a
direction-agnostic geometry discriminator that asserts **equality** (R7). **Word the
explanatory comment as prose — do not spell the pair literal in it**, or the repo-wide
inventory grep keeps this file on the list forever.

**O-Freeze** — 36 entries, 1 state, no external CSS, 7 stacks in the markup, 7 min-width + 4
line-height existing, version 2.4.1 at line 6, target `O-Freeze`. Gate file clean; the
lightest of the four.

**O-Texture** — 31 entries, 1 state, and the trap of the wave: **zero `font-family` in
`index.html`** — all six stacks live in `css/ouaricon-naturalist.css`. It carries 4 min-width
+ 3 min-height + 4 line-height. Its `tests/ui_tip_render_check.js` carries the same hard-coded
pair as O-Comp and takes the same repair. Its version is **line 42** — `0.4.1` — behind three
`ONNXRUNTIME_VERSION` decoys; read the value, do not grep for a literal (R9). Its build
target is **`OuariconTexture`**, not the folder name, so
`./scripts/build-and-install.sh` must be given the target the CMakeLists declares. It is the
only ANIRA/ONNX plugin in the wave: after the build, confirm the ONNX dylibs are embedded in
`Contents/Frameworks` of the installed bundle before calling it shipped.

**O-Bass** — 29 entries, 2 states, no external CSS, 10 stacks in the markup, 5 min-width + 4
line-height existing, version 1.6.1 at line 10, target `O-Bass`. Gate file clean. Its i18n.js
header records a settled suite-wide French term for the hover-help surface and a prior
finding about two French names for one control — keep the Chinese consistent with the same
one-name-per-control discipline.

All four carry the option-counting sentence in the en and fr language hover-help bodies;
remove it in all four.

**Batch close-out, after all four are built and installed:**

1. **One cold auval sweep covering all six.** Read each triple off `auval -a` — never guess
   one; O-Prism is an instrument (`aumu`), the other five are effects. Expect one registry
   rescan of roughly fifteen minutes for the whole sweep. Before the sweep, confirm no
   alternate-variant orphan is on disk for any of the six — a leftover unsuffixed bundle
   beside a `-dev` one pins Logic's registry slot to whichever was installed first.
2. **`PLUGINS.md`, once, in its own commit.** All six rows are stale by one patch as found
   (see the live table). Write the shipped versions and mark the rows installed. Then run
   the duplicate check — `grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d`
   must print nothing.
3. **Repo-wide regression sweep** — the whole suite, not just this wave.

**Commit discipline throughout this task and the two before it:** re-check
`git branch --show-current` and `git status --short` immediately before every commit, not
once at the start. This is a shared checkout and another session's staging can join your
commit in the gap. Name paths explicitly; never `git add -A`, never `git commit -a`. Note
`.gsd/dispatch-isolation-sentinel.json` is modified in the tree and predates this work —
never stage it.
  </action>
  <verify>
    <automated>for p in O-Comp O-Freeze O-Texture O-Bass; do node scripts/check-i18n.js --plugin $p >/dev/null 2>&1; echo "$p check-i18n exit=$?"; done</automated>
    <automated>for p in O-Comp O-Freeze O-Texture O-Bass; do node scripts/check-ui-labels.js --plugin $p >/dev/null 2>&1; echo "$p check-ui-labels exit=$?"; done</automated>
    <automated>for p in O-Comp O-Freeze O-Texture O-Bass; do echo "--- $p"; node scripts/i18n-zh-lint.js --plugin $p 2>&1 | tail -8; done   # 0 findings each</automated>
    <automated>node scripts/check-i18n.js; echo "repo-wide exit=$?"   # 43 localized plugins, six now three-language</automated>
    <automated>node scripts/i18n-zh-lint.js 2>&1 | tail -12   # repo-wide 0 findings</automated>
    <automated>node scripts/i18n-fr-lint.js; echo "exit=$?"   # exit 0, 0/43 — French untouched by the whole wave</automated>
    <automated>for p in O-Comp O-Freeze O-Texture O-Bass; do echo -n "$p han-in-cpp: "; find plugins/$p/Source \( -name '*.h' -o -name '*.cpp' \) -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' {} + | wc -l | tr -d ' '; done   # 0 each</automated>
    <automated>for p in O-Comp O-Freeze O-Texture O-Bass; do echo -n "$p control: "; perl -CSD -ne 'if(/\p{Script=Han}/){print "$ARGV\n"; close ARGV}' plugins/$p/Source/ui/public/js/i18n.js | wc -l | tr -d ' '; done   # 1 each — the control for the line above; a 0 here means the gate above proved nothing</automated>
    <automated>for p in O-Prism O-Comp O-Freeze O-Texture O-Bass O-AnalogSaturation; do echo -n "$p stale-enum: "; perl -0777 -ne '$n++ while /(English and French|anglais et le fran)/gi; END{print $n+0,"\n"}' plugins/$p/Source/ui/public/js/i18n.js; done   # 0 on all six</automated>
    <automated>for f in plugins/O-Comp/tests/ui_tip_render_check.js plugins/O-Texture/tests/ui_tip_render_check.js; do perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' "$f" | grep -c "en,fr\|\['en', *'fr'\]"; done   # 0 and 0, comments included</automated>
    <automated>for p in O-Comp O-Texture; do node plugins/$p/tests/ui_tip_render_check.js >/dev/null 2>&1; echo "$p tip-gate exit=$?"; done   # exit 0 with a real zh pass</automated>
    <automated>node scripts/boot-all-uis.js --strict-tips 2>&1 | tail -5   # 0 DEAD, 0 late, 0 failed</automated>
    <automated>node scripts/i18n-zh-backtranslate.js 2>&1 | tail -12   # stage view: every wave-4a row at bt, 0 mt</automated>
    <automated>grep "^| O-" PLUGINS.md | awk -F'|' '{print $2}' | sort | uniq -d   # MUST print nothing</automated>
    <automated>for p in O-Prism O-Comp O-Freeze O-Texture O-Bass O-AnalogSaturation; do cm=$(perl -ne 'print "$1\n" if /^\s+VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/ \&\& !$d++' plugins/$p/CMakeLists.txt); reg=$(grep -E "^\| $p \|" PLUGINS.md | cut -d'|' -f4 | tr -d ' '); [ "$cm" = "$reg" ] \&\& v=AGREE || v=MISMATCH; echo "  $p cmake=$cm registry=$reg $v"; done   # AGREE six for six (it reads MISMATCH six for six before the work)</automated>
    <human-check>auval -v on all six triples, triples read off auval -a, one cold rescan. All six report AU VALIDATION SUCCEEDED.</human-check>
    <human-check>O-Texture's installed bundle carries the ONNX dylibs in Contents/Frameworks.</human-check>
    <human-check>Every back-translation triple for all four plugins read with --verbose; accepted drifts carry a written reason.</human-check>
  </verify>
  <done>
All six wave-4a plugins ship English, French and Simplified Chinese at `reviewed: 'bt'` on
every row. Both gate files derive their language list and hold no two-language literal,
comments included, with the derive-or-abort control fired. PLUGINS.md carries six corrected
rows in one commit, no duplicates, agreeing with CMakeLists six for six. Repo-wide
`check-i18n`, `i18n-zh-lint` and `i18n-fr-lint` all clean. auval PASS on all six.
  </done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| executor → blind reverse reader | An external agent receives Chinese strings with the English deliberately withheld. Anything that lets it recover the English, or correlate a correction batch against a prior one, makes the read vacuous. |
| glossary root → shipped copy | A settled `TERMS` root applied to two different English keys on one page ships an unreadable sentence that no automated check in this repo can see. |
| zh edits → en/fr rendering | CSS pins and font tails are shared surfaces. A change made for Chinese can move a language this work does not touch. |

## STRIDE Threat Register

| Threat ID | Category | Component | Severity | Disposition | Mitigation Plan |
|-----------|----------|-----------|----------|-------------|-----------------|
| T-qrc-01 | Information disclosure | `i18n-zh-backtranslate --emit` batch | high | mitigate | R6 — blind 12-hex ids, per-batch salt, explicit `--manifest`, repo access forbidden in the dispatch; fire all five blinding controls per batch rather than trusting the code exists |
| T-qrc-02 | Repudiation | correction rounds | medium | mitigate | R4 — fresh reader **and** fresh salt per round, so an agreement is independent rather than self-consistent |
| T-qrc-03 | Tampering | shared `.git` index across concurrent sessions | high | mitigate | Re-check branch and `git status --short` immediately before every commit; path-scoped commits only; never stage `.gsd/dispatch-isolation-sentinel.json` |
| T-qrc-04 | Denial of service | en/fr geometry regression from a zh pin | high | mitigate | R8 — grep existing declarations before appending; `check-ui-labels` drives all three arms in one run and must report 0 FAIL on each; `i18n-fr-lint` clean repo-wide |
| T-qrc-05 | Spoofing | a gate that passes vacuously | high | mitigate | Every negative gate carries a positive control that fired; derive-or-abort control on both repaired gate files; `i18n-zh-lint --self-test` 10/10 before any zero is trusted |
| T-qrc-SC | Tampering | package installs | low | accept | This wave installs nothing — no npm, pip or cargo operation is in scope. No legitimacy audit required. |
</threat_model>

<verification>
The wave is not done until **all six** are green — no partial waves.

Per plugin, all six:
- `check-i18n --plugin <Name>` exit 0, LANGUAGES reads three
- `check-ui-labels --plugin <Name>` exit 0, 0 FAIL, zh arm 0 geometry moved, en and fr arms unchanged
- `i18n-zh-lint --plugin <Name>` 0 findings, `BELOW SHIP BAR 0`
- every back-translation triple read with `--verbose`, drifts resolved with recorded reasons
- `auval -v` AU VALIDATION SUCCEEDED
- zero Han under `Source/**/*.{h,cpp}`, with the positive control fired on that plugin's own i18n.js

Repo-wide, once:
- `check-i18n` exit 0
- `i18n-zh-lint` 0 findings; `--self-test` 10/10
- `i18n-fr-lint` exit 0, 0/43
- `boot-all-uis --strict-tips` 0 DEAD, 0 late, 0 failed
- `PLUGINS.md` no duplicate rows; six rows agree with their CMakeLists
- both repaired gate files carry no two-language literal, comments included

**A zero from a gate whose positive control was never run is not evidence.**
</verification>

<success_criteria>
Six plugins ship English, French and Simplified Chinese, built, installed and auval-clean.
Every zh row at `reviewed: 'bt'`, promoted only after a blind reverse read. English and
French geometry byte-unchanged on all six. Two gate files repaired to derive their language
list. One path-scoped commit stream per plugin, PLUGINS.md once at the end, six registry rows
corrected from the CMakeLists values.
</success_criteria>

<output>
Create `.planning/quick/260904-qrc-stage-4-wave-4a-of-the-zh-hans-rollout-o/260904-qrc-SUMMARY.md` when done.

Carry forward for wave 4b, in the same shape Stage 3 used: anything structural this wave hit
that the remaining six waves will copy. Wave 4b is O-IntonationPad (199), O-DigiDelay,
O-AnalogEQ, O-Tremolo, O-SimpleReverb, O-Emulator — all `Source/ui/public/`, and O-Emulator
and O-SimpleReverb both carry a two-language literal in their gate file.
</output>
