---
task: 1
role: TRACER
plugin: O-simpleAdditive
phase: quick-260907-qda
wave: 4g
status: complete
shipped_version: "1.3.0"
bumped_from: "1.2.1"          # the CMakeLists VALUE, not the 1.2.0 PLUGINS.md says
rows: 131
reviewed: bt
requirements: [ZH4G-01, ZH4G-05, ZH4G-07, ZH4G-09]
commits:
  - e130f2b9   # the 'mt' table, the body correction, the font tail, the codec, the pins
  - 663bdd07   # promotion to 'bt', version bump, CHANGELOG
false_predictions: 3
---

# Task 1 — O-simpleAdditive 1.2.1 → 1.3.0, Simplified Chinese (the wave-4g TRACER)

131 rows of Simplified Chinese at `reviewed: 'bt'`, shipped as **1.3.0**, built and
installed. Two path-scoped commits. Every gate green. `auval` deliberately not run — it is
Task 7's single cold sweep.

---

## The six things this task proves for Tasks 2–6

These are recorded so the rest of the wave inherits the method rather than re-deriving it.

### 1. The two-arm CMake reader, run where a one-arm reader also works

```bash
# two-arm: literal after VERSION, else a set()-held variable
perl -ne 'if(/^\s*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}
          if(/^\s*set\s*\(\s*\w*VERSION\s+"?([0-9]+\.[0-9]+\.[0-9]+)"?/){print "$1\n";exit}' \
     plugins/<Name>/CMakeLists.txt
```

Fired on all six at this task's end. **K5 reproduces exactly**, and the cross-check is worth
carrying:

| plugin | one-arm | two-arm |
|---|---|---|
| O-simpleAdditive | `1.3.0` | `1.3.0` (shipped) |
| **O-simpleGrain** | **`EMPTY`** | **`1.4.3`** |
| O-simpleSampler | `1.4.4` | `1.4.4` |
| O-simpleSubtractive | `1.4.1` | `1.4.1` |
| O-Polystutter | `1.14.3` | `1.14.3` |
| O-Orbit | `1.2.3` | `1.2.3` |

Proving it on the plain shape first is the point: the reader is now known-good on the
literal form *before* Task 2 meets the form that breaks the one-arm version.

### 2. The comment-stripped C++ codec grep, with the positive control on the same run

Three probes, and none of them can pass on a file whose code never changed:

```bash
# 1. strip comments FIRST, so the doc line above the codec cannot supply the evidence
perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' plugins/<Name>/Source/PluginProcessor.h > /tmp/pp.h
grep -c 'zh-Hans' /tmp/pp.h            # >= 2 — encode AND decode

# 2. match each function BY NAME and report whether it is three-way
#    (a sweep's exit code is not evidence of an edit)
node --input-type=commonjs -e '...' # languageCode: THREE-WAY / languageIndex: THREE-WAY

# 3. the negative, and its positive control ON THE SAME RUN
find plugins/<Name>/Source \( -name '*.h' -o -name '*.cpp' \) \
  -exec perl -CSD -ne 'if(/\p{Script=Han}/){print "HAN IN C++: $ARGV\n"; close ARGV}' {} +
perl -CSD -ne 'if(/\p{Script=Han}/){print "control fired: $ARGV\n"; close ARGV}' \
     plugins/<Name>/Source/ui/public/js/i18n.js
```

Measured here: `zh-Hans` count **2**, both functions **THREE-WAY**, negative printed
**nothing**, control printed the i18n.js path. Without probe 3's second line the negative
proves only that the command ran.

Located by the `languageCode` / `languageIndex` tokens, never by line number. The raw
numbers were L181-182 as the plan said, but they are not what the edit was anchored to.

### 3. The blind-dispatch shape, end to end

Runnable form, all four correctness traps closed:

```bash
# EMIT — BOTH --emit and --plugin (R5/F16), --forward-provenance takes a VALUE (F9)
node scripts/i18n-zh-backtranslate.js --emit <Plugin> --plugin <Plugin> \
     --out /tmp/A.tsv --forward-provenance "<what produced the Chinese, and when>"

# CHUNK on the emitted file — the emitter has NO row-range flag. Run one emit PER CHUNK:
# two emits of the same target share 0 blinded ids, so each chunk gets its own salt free.
# Select rows by the manifest's ids map, whose values are `<Plugin>|label|…` /
# `|title|…` / `|body|…`.

# DISPATCH — cwd OUTSIDE the repo, no tools, repo access forbidden IN THE PROMPT
cd /tmp/... && CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1 \
  claude -p "$(cat chunk.prompt.txt)" --model sonnet --allowed-tools "" > chunk.en.tsv 2> chunk.err

# INGEST — explicit --manifest, --verbose, read EVERY triple (R2)
node scripts/i18n-zh-backtranslate.js --ingest /tmp/chunkA.en.tsv \
     --manifest /tmp/A.tsv.manifest.json --provenance "<the reverse pass>" --verbose
```

**`--model sonnet` and `--model haiku` are aliases and the CLI prints no model line.** Read
the id back from the process instead — `claude -p "Reply with ONLY your exact model id
string, nothing else." --model <alias> --allowed-tools "" < /dev/null`. Measured here:
`sonnet` → **`claude-sonnet-5`**, `haiku` → **`claude-haiku-4-5-20251001`**. Note the
`< /dev/null`: without it the CLI stalls three seconds waiting on stdin and warns.

**The concept split is not ceremony — it produced this wave's best finding.** See below.

**Both refusal controls, fired against a properly provenanced emit** (F10), verbatim:

1. `--provenance` omitted →
   `REFUSED: back-translation provenance is missing or identical to the forward pass — this triple proves nothing`
   `  no --provenance was given.`
2. wrong-side `--manifest`, *with* `--provenance` →
   `WRONG MANIFEST FOR THIS BATCH: all 5 returned ids are unjoinable, not one.`

The second one only reaches its own refusal because `--provenance` was passed. Run without
it, it refuses for reason 1 and proves nothing about the manifest.

### 4. WHERE THE FONT WORK GOES WHEN THE CSS IS EXTERNAL — a first for this rollout

Wave 4f's five plugins were inline-CSS. **O-simpleAdditive's `index.html` declares ZERO
`font-family` sites; all six are in `css/styles.css`.** Search the PAIR, comment-stripped:

```bash
cat plugins/<Name>/Source/ui/public/index.html plugins/<Name>/Source/ui/public/css/styles.css \
  > /tmp/all.txt
perl -0777 -pe 's{/\*.*?\*/}{}gs' /tmp/all.txt > /tmp/all-stripped.txt
grep -oE "font-family:[^;}]*" /tmp/all-stripped.txt | sort | uniq -c
```

A census run against `index.html` alone reads **0 declarations** on this plugin and looks
like a clean bill. Four of the six carry an external stylesheet (all but O-Polystutter,
which is fully inline) — so this is the wave's common case, not its exception.

### 5. The CJK tail on a Latin-safe stack, verified by RE-RUNNING THE SCREEN

Census returned one house stack at 6 sites, 4 of them `inherit`:

```
1  font-family: 'Garamond', 'EB Garamond', 'Adobe Garamond Pro', 'Times New Roman', serif
4  font-family: inherit
1  font-family: var(--symbol-font)
```

Tail half only — Times New Roman is installed and 4th in the stack, so the Latin is already
safe. The absent Garamond members were **kept**: they are the Windows/print intent and this
machine is not the one the suite builds for. The tail goes **before** the trailing generic
(W1).

**`--symbol-font` took the tail too, and that is measured rather than assumed (N3).** Its
only consumer is `.gear-btn`; `#gear-btn` is TIP_BINDINGS row 1; and `measure-ui` reports:

```
{ "id": "#gear-btn", "han": true, "vis": true, "own": "⚙",
  "ff": "\"Segoe UI Symbol\", …, \"PingFang SC\", \"Microsoft YaHei\", serif" }
```

`han: true` on a node whose own text is a single gear glyph — because `han` counts
`data-tip`, `data-tip-title` and `aria-label`. **The verdict is on whether the node is an
anchor, never on the glyph it paints.**

Verified by re-running `measure-ui --report all` and reading `undeclared-font: 0` against a
**non-empty** input (111 visible Han-bearing nodes), plus a scripted check that 0 of those
111 resolve without a CJK face — never by reading the CSS back.

### 6. The F13-corrected duplicate-key scan and the F14-corrected helper forms

The duplicate scan **must exclude the declared `LANGUAGES` members**, or a three-language
table can never read `NONE`: a multi-line `'zh-Hans':` block starts a line and the bare
`matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)` returns it on every entry. Corrected form reads
`keys=48 duplicates NONE` before and after.

F14's six helper corrections, all used live here and all as recorded:

- `serve(root, onMiss)` resolves `{ server, port, close }` and has **no `url`** — build it
  as `` `http://127.0.0.1:${srv.port}/` ``;
- `buildRoot(name)` returns an **object** — pass `built.root` to `serve()`;
- `resolvePlaywright()` returns the **module** — use `pw.chromium.launch()`;
- a `'bt'` literal inside `node -e` loses its quotes — **write promotion scripts to a
  FILE**;
- `git commit -F <file> -- <paths>`, options **before** the `--`.

**F14.5 earned itself twice on this task, and the second time from a direction it does not
name.** A hand-quoted `termNote` explaining the word `'Organ'` and containing the words
`preset's` put two raw apostrophes inside a single-quoted JS literal. The module stopped
parsing and every gate reported `ERROR import failed: Unexpected identifier 'Organ'` — the
*same failure signature* as the `node -e` case, from static text rather than from shell
quoting. Recovered with `git restore --source=HEAD -- <file>` (safe: the file had just been
committed and nothing else was uncommitted in it — checked with `git status --short` on
that path first), then re-applied through an escaping function.

**Generalised for Tasks 2–6: never hand-quote a string into generated JS. Route every
string through an escaper, including prose in comments and notes.** Every note in this wave
quotes English terms and will hit this.

---

## Corrections to the plan, measured

Three plan predictions were FALSE on the tree. All three are recorded so Tasks 2–6 do not
inherit them.

### FALSE-1 — the O-Strata untracked count is **0**, not 2 (precondition (g), K2)

The plan's precondition asserts `git status --short | grep -c "O-Strata"` reads **2**. It
reads **0**. A concurrent session committed both planning files between the planning pass
and this one. The untracked set on the tree today is a single line — this wave's own plan
directory, `?? .planning/quick/260907-qda-…/`, which is likewise never staged.

**K2's load-bearing claim is unaffected and re-confirmed live**: O-Strata still has no
`i18n.js`, and both repo-wide lints still exit 2 with `0 finding(s) across 0 plugin(s)` and
`1 plugin(s) could not be read`. The exit code and the `git status` line were never the
baseline; the finding count and the unreadable count are. **This is exactly the failure the
precondition exists to catch: an inherited `git status` line goes stale in hours.** Do not
treat a mismatch here as a blocker — re-read the line and carry on.

`.claude/agent-memory/research-planning-agent.md` is still modified and still another
session's file. It was never staged.

### FALSE-2 — O-simpleAdditive DOES have a second version source (K6)

K6 says *"No other plugin in the six has a second version source"*. Measured:

```
plugins/O-simpleAdditive/tests/render-harness/CMakeLists.txt:48  JucePlugin_VersionString="1.0.2"
plugins/O-simpleAdditive/tests/render-harness/CMakeLists.txt:49  JucePlugin_VersionCode=0x10002
```

**It is a different shape from O-simpleGrain's and the difference is what matters.**
O-simpleGrain's harness reads `${OSIMPLEGRAIN_VERSION}` — a live variable, so bumping the
string without the hex ships a harness compiled against a stale code. O-simpleAdditive's is
a **hard-coded literal frozen at 1.0.2**; it has never tracked and bumping the plugin does
not desynchronise it any further than it already is.

**Not fixed, deliberately.** It is pre-existing, it is in a file outside this task's scope
(wave 4e D6 puts the render-harness trees out of scope), and editing it would be a change
nothing asked for in a commit whose subject is a caption table. Carried as a deferred item.

**For Tasks 2–6:** do not read K6 as "grep for a hex mirror". Grep
`tests/render-harness/CMakeLists.txt` for `JucePlugin_Version` on your plugin and classify
what you find — a `${VAR}` reference must move with the bump; a frozen literal must not be
touched inside a localization commit.

### FALSE-3 — the R3 glossary-match denominator is 50, not 22

The plan records *"22 glossary-matched captions"* for this plugin. Re-fired here it matched
**50** — because the planning screen matched captions only, while this one pushed the
tooltip TITLES through `TERMS` as well. **The verdict is identical and that is the part
that matters: NONE.** Recorded because the numbers on the other five will differ the same
way and a mismatch is a denominator difference, not a finding.

---

## The best finding: the concept split produced two readings of one string

This is the result worth carrying into every remaining task.

`谐音拉杆` is one string in one table. It went to two readers — the title chunk and the
caption chunk — and came back as **two different English words**:

| chunk | id | en′ |
|---|---|---|
| A (86 tooltip titles + bodies) | `O-simpleAdditive|title|drawbars` | **Harmonic drawbar** |
| B (45 captions) | `O-simpleAdditive|label|label.drawbarTitle` | **Harmonic sliders** |

Same Chinese, same day, different reader. **Only the split could produce both**, and the
pair is what settles the question: the rendering is right, and chunk B's reader simply had
no organ context around it — chunk B is captions only. The same reader rendered the same
morpheme as *"drawbar registration"* the moment a body supplied context (`label.captionOrgan`).

A single reader returns one of those two answers and nothing to check it against. **Chunk on
the concept even when the table would fit in one batch.**

---

## Blind-read accounting

| | |
|---|---|
| rows dispatched | **131 of 131** (86 + 45), 0 withheld from review |
| chunks | 2, split on the CONCEPT (F17), each from its own emit → own 16-hex salt |
| ids shared between the two chunks | **0**, measured |
| ids returned identical AND IN ORDER (M12) | **yes**, both chunks, diffed |
| Han surviving into returned English | **0** |
| malformed returned rows | **0** |
| dispatched zh column vs the COMMITTED tree | **byte-identical on all 131 rows** |
| product-name control | **0** — adjudicated, not read as a pass: the wordmark is `I18N_EXEMPT` and never enters the table |
| triples read with `--verbose` (R2) | **131 of 131**, both rounds |
| rows re-authored | **4**, across **2** findings |
| correction rounds | **2**; round 2 corrected nothing further, so no round three |
| forward model | `claude-opus-5` |
| round-1 reverse model | `claude-sonnet-5` (read back from the process, not the alias) |
| round-2 reverse model | `claude-haiku-4-5-20251001` — a THIRD model, fresh salt, fresh session (R4) |
| refusal controls fired | **2 of 2**, against a properly provenanced emit (F10) |
| termNote exemptions | **6** — 3 per term, entry-scoped (N4) |

### The two re-authorings, and why (collision on the page, never drift distance)

**`label.waveformHint`.** Round 1 returned *"Sum of single-cycle waveformS"*, plural, while
the caption on the same row reads *Waveform*, singular, and the scope draws one trace. The
hint mis-described the control it labels, on that control's own row. Re-authored so the
summing is the operation and the waveform is the result. Round 2: *"post-summation
single-period waveform (deforms with scanning)"*.

**The `Organ` lesson, 3 entries.** Round 1 returned *"Pipe organ"* for the tooltip title
while that entry's own **body** says a Hammond-style drawbar registration. A Hammond is an
electric organ; a pipe organ is not a Hammond and has no drawbars at all — a title and its
own body naming two different instruments on one control.

The glossary root is the pipe-organ word, and **the site count was checked before the note
was written** (N4): `Organ` is a caption on exactly **one** plugin in the corpus — this one,
in three entries — and the root is shipped nowhere else. It was derived from this page's own
caption and never had a second site to check it against. **O-Bells is the independent
confirmation of what the root means**: its own body uses that same word to name the *pipes*
of one. Corrected at the three places it exists, flagged for the glossary rather than forked
silently. Round 2: *"electronic organ"* on all three.

`Morph Pad` is the same shape, found by the R3/Z5 screen rather than by the reverse read:
the root renders as a morph *panel*, a user-interface surface, where the English names a
synth *pad*. One corpus site, shipped nowhere. Three entry-scoped notes.

### Accepted drifts, each with its written reason

All are glossary roots with **no collision on this page**: the lesson-preset label reading
*"Teaching presets"*; sawtooth and square captions gaining the word *"wave"*; the lo-fi bell
reading as a mass noun; the amp/mod envelope titles spelling out *Amplitude* / *Modulation*
where the English abbreviates (the two families read distinctly, so they cannot collide);
*"a spread bell spectrum"* returning as *"expanded"*, where nothing else on the page says
spread; and `谐音拉杆` itself, settled by the split above.

---

## Measured numbers, before and after

### Geometry — `check-ui-labels`, zh arm, across both states

| | non-label elements moved |
|---|---|
| table landed, no pins | **210** |
| after round 1 — 8 leaf line-height pins | **46** |
| after round 2 — 2 row ratios + 1 width floor | **0** |

The **fr arm was green at every one of the three measurements**, and coverage never moved
off `40 of 40` visible with **0 never-visible**. Re-run once more after the correction round
(both re-authored strings changed length): still **0**.

### The eleven pins, every ratio derived from the element's own English CONTENT box

Rect height less padding less border, divided by the element's own font-size, line count
verified as exactly 1 on each. **None of the eleven selectors carried a `line-height` or
`min-width` declaration before this block** — checked per selector (R8/F7).

| selector(s) | fs | en rect | pad t/b | border t/b | content | ratio |
|---|---|---|---|---|---|---|
| `.subtitle`, `.settings-label`, `.viz-hint`, `.tour-caption` | 11 | 12 | 0/0 | 0/0 | 12 | **1.0909** |
| `.drawbar-title`, `.group-title` | 12 | 15 / 18 | 0/0, 0/2 | 0/0, 0/1 | 15 | **1.2500** |
| `.tour-label`, `.keyboard-label` | 10 | 11 | 0/0 | 0/0 | 11 | **1.1000** |
| `.viz-label` | 10 | 12 | 0/0 | 0/0 | 12 | **1.2000** |
| `.tour-btn` | 10.5 | 21 | 4/4 | 1/1 | 11 | **1.0476** |

`.group-title` and `.drawbar-title` land on the same 15 px content box from *different* rect
heights — 18 and 15 — because one has bottom padding and a border and the other does not.
**A ratio taken from the rect would have given 1.5 and 1.25 and moved English on one of
them.**

**`.tour-btn` is the F6 case, live.** M8's leaf table says 10 px → 1.1; a `<button>` at
10.5 px measures an 11 px content box, ratio **1.0476**, because the UA `font` shorthand
resets `line-height` on form controls. Derived from its own box, never from the table.

### The two ROW pins — the N4 (wave 4e) shape, live

`.viz-label` and `.keyboard-label` each grew (`dh=2.0`, `dh=3.0`) while **both spans inside
each measured `enH == zhH`**. `getBoundingClientRect()` on an inline element reports its
font box, not its line box, so a row grows with no leaf reporting it. Pinned as ratios on
the row; the unpinned inline children inherit, and `.viz-hint` keeps its own higher-priority
1.0909 so the 11 px child and the 10 px child both land on a 12 px box.

### The width FLOOR — the N9 shape, live, and the wave's largest single cascade

| | en | fr | zh |
|---|---|---|---|
| `.tour-caption` width | **311.52** | 395.83 | **244.34** |
| `.tour-caption` y | 743.75 (line 2) | 743.75 (line 2) | **715.27 (line 1)** |
| `.tour-buttons` width | 679.00 | 679.00 | **422.66** |
| `section.preset-tour` height | 53.97 | 53.97 | **29.98** |

Chinese is shorter, so the caption fit on flex line 1 beside the six buttons: the section
lost a whole flex line and the button group collapsed 256.34 px with it. **A floor at the
exact measured English box — `min-width: 311.52px`, not rounded** — restores the wrap.
English is unchanged to the pixel; French is 395.83 px and never sees the floor; the
existing `max-width: 54%` still caps the other direction. **Not a fixed width**: that would
have moved French.

### `measure-ui` screens — the K12 vacuum, closed

| screen | baseline (vacuum) | after |
|---|---|---|
| `undeclared-font` | 0 — **input EMPTY** | **0 against 111 visible Han-bearing nodes** |
| `line-height-normal` | 0 — input empty | **1**, named below |
| `wrap-count` | 0 (real, baseline 0) | **0** |
| `svg-font-attr` | 0 carriers | 0 carriers |
| identity | 595 nodes / 298 keys / 159 ids | 896 / 299 / 159 (a third language pass) |
| leaves ESTIMATED | 84 of 102 | **62 of 102** — 22 leaves left the estimated set as the pins made them measurable |

Every zero after the table landed is a **measurement**. Every zero before it was a vacuum.

**The one `line-height: normal` residual, named with its measurement (N1):**
`#help-toggle`, font-size 10 px, **enH == zhH == 24**. A fixed-height form control; it
cannot move and cannot be pinned to any effect.

### The state-EFFECT assertion — both click states (N1, wave 4e)

Both of this plugin's states are clicks; the wave has no `eval` states at all.

| state | target | rect | `elementFromPoint` | took effect |
|---|---|---|---|---|
| settings popover OPEN | `#gear-btn` | 22×22 @ (815,15) | target or descendant | **YES** — visible `[data-i18n]` 37 → 40 |
| hover help OFF | `#help-toggle` | 96×24 @ (730,83) | target or descendant | **YES** — face `On` → `Off` |

No `null` rects, so there was nothing to discriminate as a scrolled target. Neither state is
a coverage hole.

### Gate results

| gate | result |
|---|---|
| `check-i18n --plugin` | **exit 0** — `LANGUAGES … got ["en","fr","zh-Hans"]`, `43 I18N + 45 LABELS` unchanged, `43 tip(s) bound` |
| `i18n-zh-lint --plugin` | **0 findings**, 88 entries, **BELOW SHIP BAR 0** |
| `i18n-zh-lint --self-test` | **10/10** |
| `check-ui-labels --plugin` | **exit 0**, `== ALL CHECKS PASSED ==` |
| `boot-all-uis --strict-tips` | **clean 1/1**, 0 warn, 0 failed, **0 DEAD, 0 late** |
| `i18n-zh-lint` repo-wide | 0 findings across 0 plugins, 1 unreadable — exit 2, the K2 **baseline** |
| `i18n-fr-lint` repo-wide | 0 of 44 with findings, 1 unreadable — exit 2, same baseline. **No French rendering changed** |
| `check-i18n` repo-wide | `ALL CHECKS PASS — 43 localized plugin(s)` |
| corpus | **4621 → 4752** of 5441 rows |

### Stale-body and dead-class verdicts

| class | verdict | evidence |
|---|---|---|
| the fixed-pair language enumeration | **LIVE, deleted in en and fr** | comment-stripped `-CSD` probe **2 → 0** |
| its numeric exception list | **KEPT, re-verified** | markup carries **3** `<select>`; minus the language selector = **2**; *"the two drop-down menus"* is true today |
| the "and nothing else" settings class | **NO SUBJECT — checked non-defect** | `grep -c "'tip.settings'"` = **0**; `grep -c "tips-toggle"` in markup = **0** |
| the "Tuning tab stays English" class | **NO SUBJECT — checked non-defect** | not a `scala-tuning-engine` consumer; no tuning tab |

The superseded phrasings are in the CHANGELOG and are **not** respelled in any source
comment (C8), which is why the probe reads the file as fixed.

### Zeros recorded AS MEASURED

- **gate files: 0.** `find plugins/O-simpleAdditive/tests -name '*.js'` → nothing. Fired
  with `find`, never a glob, because a zsh glob matching nothing aborts the whole command
  and reads as an answer. **No gate was invented** (C5/C6 has no subject in wave 4g).
- **unscanned module JS: 0.** `find <ui root> -name '*.js' -not -path '*/juce/*'` returns
  exactly **two** files, `js/i18n.js` and `js/app.js`. **This is the wave's only plugin with
  none**, which is what makes it the controlled converse to the four that have one — the
  zero is a measurement rather than an artefact of where the query looked.
- **language `AudioParameterChoice`: none.** Re-confirmed by scanning each declaration's own
  text, not the containing file. The two on this plugin are `frameBSource` and `bitDepth`,
  whose option strings stay English under D-01 and are named in English inside the Chinese
  bodies (W3, both directions).
- **duplicate keys: NONE**, F13-corrected form, `keys=48`, before and after.
- **submodule untouched.** `git status --short -- plugins/O-Orbit/libs/SAF` → empty;
  `git submodule status` → ` b6fe1882… (v1.3.4)` (leading space = clean). The guard ran
  before **both** commits even though no path here is inside it, so Task 6's is not the
  first time it runs.
- **tags created: 0.**

---

## Commits

| hash | subject |
|---|---|
| **`e130f2b9`** | `i18n(O-simpleAdditive): add Simplified Chinese at reviewed:'mt' — 131 rows, the tracer for wave 4g` |
| **`663bdd07`** | `i18n(O-simpleAdditive): promote 131 zh rows to reviewed:'bt' after a two-round blind reverse read — v1.3.0` |

Both path-scoped `git commit -F <file> -- plugins/O-simpleAdditive`, options before the `--`
(F14.6). `git branch --show-current` and `git status --short` re-checked immediately before
each, never once at the start. Nothing under `plugins/O-Strata/`, nothing under
`.claude/agent-memory/`, nothing under `.planning/` was ever staged.

---

## Deferred items

1. **O-simpleAdditive's render-harness carries a frozen version literal.**
   `plugins/O-simpleAdditive/tests/render-harness/CMakeLists.txt` hard-codes
   `JucePlugin_VersionString="1.0.2"` / `JucePlugin_VersionCode=0x10002`. It has never
   tracked the plugin version and is now four minors stale. Unlike O-simpleGrain's, it is a
   dead literal rather than a `${VAR}` mirror, so it does not desynchronise on a bump. Out
   of scope here (wave 4e D6). **Grep-able token: `JucePlugin_VersionString="1.0.2"`.**
   Worth a suite-wide sweep — other plugins likely carry the same frozen pattern.

2. **Two single-sited glossary roots need a glossary-level decision.** Both are corrected
   per-entry here with reasoned `termNote`s and neither is forked silently:
   - `'Morph Pad'` → `变形面板` reads as a morph **panel**, a UI surface; the English names
     a synth **pad**. Rendered `变形音垫`.
   - `'Organ'` → `管风琴` is specifically a **pipe** organ; this preset's own body names a
     Hammond drawbar registration. Rendered `电风琴`.

     Both roots have exactly **one** corpus site — this plugin — and are shipped nowhere,
     so both were derived from a single caption. **Grep-able tokens: `变形面板`, `管风琴`
     in `scripts/i18n-zh-glossary.js`.**

3. **The `label.kbdHint` hair spaces are not carried into the zh arm.** The en and fr arms
   write the QWERTY key run with ` ` hair spaces, which this file's own comment calls
   load-bearing. The Chinese row uses plain spaces and ASCII parentheses — that is the form
   the glossary root uses **and** the form O-simpleFM and O-simplePhysicalModelSynth already
   ship, so it is the corpus convention and this file was the outlier. Recorded because the
   three arms now spell one run three ways. Geometry is unaffected (`check-ui-labels` 0
   moved). **Grep-able token: `点击琴键或使用电脑键盘`.**

4. **The glossary root for `label.kbdHint` is self-contradictory with Z1 as written.** It
   is stored lowercased — `点击琴键或使用电脑键盘 (a s d f g h j k · w e t y u)` — so the
   root lowercases a run of PHYSICAL KEY names. `normZh` lowercases both sides, so an
   uppercase rendering still matches and nothing fires; but a plugin copying the root
   verbatim would ship lowercase key names that do not match the keys. **Grep-able token:
   `click the keys or use your computer keyboard` in `scripts/i18n-zh-glossary.js`.**

---

## For Tasks 2–6, in one paragraph

Use the two-arm reader unconditionally (it returns EMPTY on O-simpleGrain). Author at `'mt'`
and promote from a script written to a FILE — and route **every** generated string through
an escaper, including prose inside a `termNote`, because a raw apostrophe in an explanatory
note breaks the module with the same signature as the `node -e` case. Search `index.html`
**and** `css/styles.css` for font declarations — four of six carry an external stylesheet
and a census of the markup alone reads a false zero. Check whether a `var(--symbol-font)`
node is a TIP ANCHOR, not what it paints. Expect the geometry to need two rounds: leaf
`line-height` ratios first, then row ratios for containers that grew while every leaf inside
them read `enH == zhH`, and a `min-width` floor for anything that shrank enough to change a
flex wrap. Chunk the blind read on the concept even if the table would fit in one batch —
it is what produced this task's only cross-checked reading. And before writing a `termNote`
against a glossary root, grep the corpus for the root and for the English key: on both of
this page's divergences the site count was **one**.

---

## Build and install

`./scripts/build-and-install.sh O-simpleAdditive`, run in the background and polled on a
sentinel — **exit 0, 52 s.** The folder name equals the `juce_add_plugin` target here, which
is why this plugin proves the script's argument convention on the simple case before Tasks
5 and 6, whose targets are `OPolystutter` and `OuariconOrbit` and do **not** equal their
folder names.

```
VST3: ~/Library/Audio/Plug-Ins/VST3/O-simpleAdditive-dev.vst3            5.0M, age 0s
AU:   ~/Library/Audio/Plug-Ins/Components/O-simpleAdditive-dev.component 5.0M, age 0s
```

`CFBundleShortVersionString` read back from the installed AU bundle: **`1.3.0`** — the
version reached the binary, not just the CMakeLists.

**No alternate-variant orphan.** Phase 4's dual-variant sweep emitted no
`⚠ Sweeping ALTERNATE-variant` warning; only the `-dev` bundles are on disk, which matches
the state the plan measured for all six.

**`auval` was NOT run.** It is deferred to Task 7's single cold registry sweep, as the plan
directs. Budget it at ~85 s, not the stale 15-minute figure.

---

## A note on the shared checkout

A concurrent session committed **`0d443637`** (an O-Strata Stage-1 phase commit) *between*
this task's two commits. That is the exact hazard the commit discipline exists for, and it
came through clean because every commit here was path-scoped and the branch and staging were
re-checked immediately before each one rather than once at the start:

- both commits are present and untouched — `e130f2b9`, `663bdd07`;
- `e130f2b9` carries **4** files and `663bdd07` carries **3**, all under
  `plugins/O-simpleAdditive/`, with no stranger's path in either;
- `0d443637` touches **0** files under `plugins/O-simpleAdditive/`;
- **0 deletions** across both commits.

It also explains FALSE-1 above: that session is what committed the two O-Strata planning
files the plan's precondition expected to find untracked.

---

## Self-Check: PASSED

Files claimed, verified on disk:

- `plugins/O-simpleAdditive/Source/ui/public/js/i18n.js` — FOUND
- `plugins/O-simpleAdditive/Source/ui/public/index.html` — FOUND
- `plugins/O-simpleAdditive/Source/ui/public/css/styles.css` — FOUND
- `plugins/O-simpleAdditive/Source/PluginProcessor.h` — FOUND
- `plugins/O-simpleAdditive/CMakeLists.txt` — FOUND
- `plugins/O-simpleAdditive/CHANGELOG.md` — FOUND

Commits claimed, verified in `git log --all`:

- `e130f2b9` — FOUND
- `663bdd07` — FOUND

Scope claims, verified:

- files staged outside `plugins/O-simpleAdditive/` across both commits — **0**
- paths staged under `plugins/O-Orbit/libs/SAF` — **0**; submodule still at `b6fe1882`
  (v1.3.4), clean
- tags created — **0**
- `PLUGINS.md` touched — **no** (Task 7 owns it)
- `ROADMAP.md` / `STATE.md` / `PLAN.md` / this summary committed — **no** (the orchestrator
  commits docs at batch end)
