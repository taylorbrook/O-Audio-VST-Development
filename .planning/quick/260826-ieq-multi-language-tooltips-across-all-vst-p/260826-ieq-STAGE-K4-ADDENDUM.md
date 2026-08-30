# Stage K — batch K4 ADDENDUM

Read `260826-ieq-STAGE-K-BRIEF.md` in full first. This file carries only what
K4 adds or corrects. Where the two disagree, THIS file wins.

## Your numbers, MEASURED (the plan is wrong; the inventory is the authority)

| Plugin | Root | Frame | LABEL | READOUT | UNIT | UNSURE | attr | js-prose | js-composed |
|---|---|---|---|---|---|---|---|---|---|
| O-Wind | `Resources/ui` | 900x600 | **61** | 7 | – | 3 | 3 | **4** | – |
| O-Bells | `Resources/ui` | 800x600 | **79** | 8 | 1 | 3 | 1 | **4** | **3** |
| O-Formant | `Source/ui/public` | 800x600 | **94** | **21** | – | 3 | 5 | **4** | 1 |
| O-MicrotonalSampler | `Resources/ui` | 900x640 | **146** | 6 | 4 | **19** | 14 | **24** | **12** |

These are a BUDGET, not a worklist. Step 1 still stands: run the extractor on
your own plugin and hand-review every `UNSURE` and `READOUT` row.

The measured LABEL count has matched the executor's own re-measurement on
**15 of 15** plugins across K2 and K3, and the plan's number has been wrong on
all 15. Trust the table above; do not trust the plan's prose.

## DECIDED: canvas `ctx.fillText` — localize, via `I18N` with EMPTY BODIES

K3 shipped two answers. The decision is **O-Comp's shape**, adopted as the
standard:

- Canvas strings go in **`I18N` with an empty body**, not in `LABELS`.
- A `trLabel()` call from inside `fillText` **fails assertion 15 as a dead
  key** — 15's `referenced` set collects markup attributes, literal `setLabel`
  keys, literal `.dataset.i18n* =` writes and innerHTML-injected keys, and a
  `trLabel()` call is none of those. The `I18N`-empty-bodies shape is already
  legal under the contract as written and needs no gate change.
- Verify with a **`fillText`-recording probe**, en→fr→en, plus a negative
  control. Neither gate can see these: assertion 10 walks TEXT NODES,
  assertion 12 scans `textContent`/`innerText` writes, and `fillText` is
  neither. **Leaving them in English passes green.**

**Measured, so you do not have to go looking:**

| Plugin | `fillText`/`strokeText` sites |
|---|---|
| O-Wind | **0** |
| O-Bells | **0** |
| O-Formant | **5** |
| O-MicrotonalSampler | **0** |

So this decision binds **O-Formant only** in K4. If your plugin is not
O-Formant and you find a canvas string anyway, the count above was wrong —
report it and apply the same shape.

The standing English canvas strings elsewhere (O-Bowed's 15, O-GrainScatter's
1, O-Orbit, O-MultiBandCompressor, O-simpleSampler) are a named **Stage-M
backlog**. Do not touch them.

## The second indirection shape — grep for it, do not wait for a gate

O-Tremolo shipped English inside a French UI with every gate green, because its
section headers were `header.textContent = headerText` with the English
literals sitting one frame away at the call site. **Assertion 12 is blind to
this.** Measured counts of `textContent`/`innerText` assigned a NON-literal:

| Plugin | sites |
|---|---|
| O-Wind | **9** |
| O-Bells | **28** |
| O-Formant | **17** |
| O-MicrotonalSampler | **57** |

Most are legitimate numeric readouts (exempt, D-01 arm 2/3). **Walk every one
to its call site** and confirm the right-hand side is a number, not an English
word one frame away. This is mechanical, and it is the single cheapest way to
avoid O-Tremolo's near-miss.

## THE TUNING PANEL — the scope divergence the plan does not describe

`scripts/i18n-extract.js:442` skips `tuning-panel.js` **by filename**, with no
ownership test. Every count in this document therefore EXCLUDES it, and both
gates are blind to it.

But ownership is not uniform, and the brief's blanket module exemption is
**wrong for three of the four K4 plugins**:

| Plugin | What it actually consumes | In scope? |
|---|---|---|
| **O-Wind** | `${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/js/tuning-panel.js` — the MODULE file, by reference | **NO.** Genuinely module-owned. Exempt per the brief. Name the module in the exempt reason. |
| **O-Bells** | `Resources/ui/js/tuning-panel.js` — a plugin-owned copy, header reads "part of O-Bells", **279 lines diverged** from the module | **YES** |
| **O-Formant** | `Source/ui/public/js/tuning-panel.js` — plugin-owned, **45 lines diverged** | **YES** |
| **O-MicrotonalSampler** | `Resources/ui/js/tuning-panel.js` — plugin-owned, **317 lines diverged** | **YES** |

The three local copies each carry roughly **34 visible strings** and none of
the three plugins has a `dependencies.json` listing the module, so
`/module-upgrade` will not revert an edit to them.

**DECIDED: the three plugin-owned copies are IN SCOPE and must be localized.**
The file belongs to the plugin, not the module, so "the PAGE speaks French" is
not true of these three unless the Tuning tab speaks it too. This is ~34
strings ON TOP of your LABEL budget, which excludes them entirely.

Consequences you must handle, because the tooling does not:

- **The extractor will not list these strings.** `i18n-extract.js:442` drops
  the file by filename. Enumerate them by hand from the file itself.
- **The EXTRACTOR skips it, but `check-i18n` does NOT** — a correction to the
  first draft of this file, which told the O-Bells and O-Formant executors
  otherwise. `check-i18n` builds `pageModules` from every `.js` in the served
  js dir except `i18n.js` and unparseable bundles, and **deliberately does not
  apply `JS_SKIP`** (borrowing it wholesale once reported 37 live
  O-IntonationPad keys as dead, for this exact reason). So assertions 12, 13
  and 15 DO scan your local `tuning-panel.js`. What is missing is the
  worklist — you must enumerate its strings by hand — and `check-ui-labels`,
  which only measures what it renders. **Verify your keys are live by DRIVING
  the panel**, and say in your report exactly which of its states you rendered
  and which you did not.
- **Do not "fix" the extractor's skip list** — `scripts/` is orchestrator-owned.
  Report the shape if it blocks you.
- **Do not edit `modules/tuning/scala-tuning-engine/js/tuning-panel.js`.** Your
  edit goes only in your plugin's own copy. Note in your commit message that
  this widens an already-large divergence from the module, deliberately.

**O-Wind is the exception and is already dispatched as EXEMPT** — its panel is
the module file consumed by reference, not a local copy.

## Carried from K3 — the traps that cost real time

1. **`HEAD~1` is NOT your commit's parent in a shared checkout.** Another
   executor commits on top of yours between your commit and your verification.
   Use `<sha>^`, never `HEAD~1`.
2. **`git checkout -- <file>` to revert a plant wipes the UNCOMMITTED fix too.**
   Restore plants from a namespaced scratchpad copy while your fix is
   uncommitted. O-GrainScatter lost its whole edit this way.
3. **`nth-child` path keys turn a pure INSERTION into phantom MOVED rows.**
   Inserting a settings cluster renumbers every following sibling and reads
   exactly like a regression. Key geometry paths on class-ordinals.
4. **A `grep -a` binary probe can miss on its own punctuation** — a straight
   apostrophe against the source's U+2019. Check a known-present control first.
5. **`dx` alone mislabels a pin as DECORATION** — confirmed four times across
   two batches. Check `dw` too.
6. **An explicit `width` caps a flex item's automatic minimum** (Flexbox §4.5),
   and **`repeat(4, 1fr)` is `minmax(auto, 1fr)`** — `width: 100%` on a
   content-sized grid track changes nothing; the fix is `minmax(0, 1fr)`.
7. **French SHRANK on a large fraction of captions everywhere** — up to half.
   A clip-only check certified four whole pages in K3. Scan for shrink.
8. **Keying a node EXPOSES pre-existing ENGLISH defects.** Five batches
   running. Expect one; negative-control it to re-fail in English before any
   French exists; fix it in the same commit and say so.
9. **`[8b]` does not account for an ancestor's `overflow: hidden` clip**, so a
   collapsed section's children report collisions a user can never see.
   Over-strict, never a false pass. Verify in the all-expanded state before you
   shorten a translation to satisfy it.
10. **`boot-all-uis` is blind to the C++ half of assertion 8** by design — it
    serves the copied file tree, not `getResource()`. Control both halves
    independently.
11. **A red sweep during a concurrent batch is NOT automatically batch-mate
    noise.** K3's two `SyntaxError`s had been shipping. Check the pre-image
    before explaining a failure away.
12. **Four of five K3 plugins were an inline `<script type="module">` in
    `index.html`**, not the `js/app.js` shape the plan assumes. Verify the
    controller shape; never assume it.

## A NATIVE `title=` DOES NOT HAVE TO BE IN THE MARKUP — gate widened mid-batch

`5e9813c3` widened `check-i18n` assertion 11 to scan the JS as well as the
markup. It used to walk only `scanHtml()`'s parse of `index.html`, so
`el.title = '...'` in a controller was invisible and "zero native title=
remain" was reachable while any number of them shipped.

Found by the O-Wind executor: its page rendered **19** native titles against
the **3** its markup declared — every FX readout carried
`valueDisplay.title = 'Double-click to edit'`. **O-Lyrica had already shipped
16 of them under a passing claim**, and was the only already-shipped plugin the
widened assertion caught (fixed in `bd89aa11`, v2.4.1).

Two things generalise:

1. **A source grep counts WRITES; the page renders INSTANCES.** O-Lyrica's
   source had ONE write, in a per-knob setup path, and the page rendered
   SIXTEEN. **`boot-all-uis`'s per-plugin `title=` column is the evidence** —
   and it is a REPORT, not a gate, so it never failed anything.
2. **Resolution shape, per §4:** DELETE the title; where it is an element's
   only help, its text moves to `data-i18n-aria` via a literal
   `dataset.i18nAria = 'aria.<key>'` write — the shape assertion 15 collects,
   so the key reads live rather than dead. Reuse the existing English verbatim.

**O-MicrotonalSampler has 8 live JS-written titles** in `js/sampler-app.js`
(lines ~993-2999), all composed template literals carrying real user-facing
prose — cell contents, dynamic-layer descriptions, variant tabs, technique trim.
They are the largest such set in the repo. `btn.title = ''` at 2265 is a CLEAR
and is correctly exempt.

## `PLUGIN_VERSION` — CLOSED, do not report it again

Fixed in `d6061e5c` for the only two plugins that shipped `1.0.0` to the host
(O-Reed, O-Marimba), measured with `PlistBuddy` before and after. The remaining
mentions — including **O-MicrotonalSampler's** — sit beside a real `VERSION`
keyword and are dead clutter, not a defect. **Leave them; do not report them.**

## Unchanged from the brief

Build mutex, `PLUGINS.md` and `scripts/` orchestrator-owned, namespaced
scratchpad (`scratchpad/<yourplugin>/`), `git add` the exact new paths then
`git show --stat`, no tag, no push, no `screencapture`.
