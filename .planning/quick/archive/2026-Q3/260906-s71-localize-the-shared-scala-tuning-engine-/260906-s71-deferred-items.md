# Quick task 260906-s71 — deferred items and structural findings

Carry-forward for **wave 4f** (O-Wind, O-Contrabass, O-Reed, O-Bowed,
O-GrainScatter) and for whoever next touches `scripts/check-i18n.js` or a shared
`modules/` JS file. Written in the shape waves 4a–4e used: the closure first,
then the debt it hands forward, then the items that were left as found and why.

Everything below was **measured on this tree on 2026-09-06** during the three
tasks of 260906-s71 (`842bf6e0`, `d848337a`, and this task's six commits).

---

## D1 — CLOSED, by option 1

Wave 4e's D1 asked for a decision before wave 4f was planned: three of 4f's five
plugins (**O-Wind, O-Reed, O-Bowed**) consume
`modules/tuning/scala-tuning-engine/js/tuning-panel.js`, which had **no i18n
hooks in any language**, and a fourth wave of "verified, untouched" would have
shipped three plugins whose tuning tab is English inside a Chinese interface.

**Option 1 was taken and executed: the module was localized once, now, as its own
task.** The module is at **v3.1.0** with 37 `data-i18n` keys, three
`data-i18n-vars` sites and a `refreshPanelI18n()` call after each of its six
`innerHTML` injections. All five path-embedding consumers gained the 37 rows in
the same change set, because a missing key renders the key name.

D1's stated "revert risk of a `/module-upgrade` across six consumers" **did not
exist**: five consumers embed the file BY PATH from CMake, so editing it in place
localizes all five on their next rebuild. O-MicrotonalSampler is not a live
consumer — it embeds its own diverged 317-line copy — and was left byte-unchanged
by decision.

### What wave 4f still owes on this surface

**1. The `zh-Hans` column on the 37 new keys, for four plugins — not three.**

`check-i18n` assertion 1 requires every key in every DECLARED language and no
others, so a `zh-Hans` column on a two-language plugin is a defect, not a head
start. The four plugins that will need it:

| Plugin | in wave 4f? | LABELS now | keys needing a zh column |
|---|---|---|---|
| O-Bowed | yes | 82 | 37 |
| O-Reed | yes | 96 | 37 |
| O-Wind | yes | 108 | 37 |
| **O-Contrabass** | **NO** | 94 | **37** |

**O-Contrabass is not a wave-4f plugin and will otherwise be missed.** It is
listed in wave 4e's D1 consumer set but not in 4f's plugin list; whoever schedules
the remaining zh waves must place it explicitly. O-Bassoon already carries the zh
column (copied verbatim from O-MicrotonalSampler at `reviewed: 'bt'`).

**2. The zh debt is GEOMETRY, not only table rows — and it is already sized.**

Adding the rows will reproduce, on each of the four, the two pins that O-Bassoon
alone needed in this task. The numbers transfer directly (TASK2 §5c/§5d):

- **A `line-height` RATIO, not a length.** PingFang SC's ascent + descent against
  the Garamond/Times stack's grew every `line-height: normal` caption in the panel
  by 3 px on O-Bassoon, accumulating to +20 px down `.tuning-controls-panel`,
  moving 123 non-label elements and pushing 13 labels further outside the frame
  than English. Pinned as `line-height: 1.11` (9 px → 9.99 against a measured 10;
  10 px → 11.10 against a measured 11) on
  `.interval-list-header, .library-header-text, .generator-header-text,
  .pitch-circle-label, .ref-knob-label, .tonic-label, .octave-stretch-label,
  .gen-row label` **plus `.viz-btn, .tuning-file-btn, .generator-btn` named
  separately** — the UA `font` shorthand resets `line-height` on form controls, so
  an inherited ratio never reaches a `<button>`. Pin as a ratio, not a length:
  wave 4e N4 records a raw length growing one arm 0.77 px on a row holding an
  inline-block button.
- **The CJK font tail on the panel's 11 buttons.** `<button>` does not inherit
  `font-family`; the UA stylesheet gives it Arial, which carries no Han, so every
  Chinese caption on `.viz-btn` ×5, `.tuning-file-btn` ×5 and `.generator-btn` ×1
  resolved through an unnamed fallback face. `measure-ui --report undeclared-font`
  read **11** on O-Bassoon and went to 0 once the plugin's own house stack was
  given to those three classes.

The `min-width` floors on `.tonic-label` and `.octave-stretch-label` and the
`min-height` floor on `.interval-list-header` **already landed on all five** in
`d848337a` and are language-independent; they do not need redoing.

**3. The N12 enumeration-form census, deliberately NOT consumed.**

`plugins/{O-Bowed,O-Reed,O-Wind}/tests/ui_tip_render_check.js` are the last
two-language gate files in the repo. All three are **byte-unchanged** by this
task, asserted with `git diff --quiet` in both Task 1 and Task 2. Waves 4a–4c
found four enumeration forms, 4d found a fifth (a per-language `Map` lookup), 4e
found no sixth. Wave 4f still owes the census on these three, and must classify
every reset-scoped look-alike explicitly rather than generalizing it.

**4. O-Bassoon's three residual `line-height-normal` findings.** They are its own
`.tab-btn` (声音 / 调音 / 关于), pre-existing, outside this task's surface, left as
found. `measure-ui`'s `line-height-normal` screen structurally cannot reach 0
(wave 4c); the criterion is `check-ui-labels` **0 moved**, which is met on all
five.

---

## D2 — Four shared module JS files that `check-i18n` still does not scan

Task 1 taught `check-i18n` to add to `pageModules` every
`${CMAKE_SOURCE_DIR}/modules/**.js` a plugin's `CMakeLists.txt` embeds, derived by
scanning the build file rather than transcribing a filename list.

**Fired unrestricted, repo-wide, before a byte of the module was keyed** — the
positive control. Exit **7**:

| Plugin | `[12]` findings | newly-scanned source |
|---|---|---|
| O-Bassoon | 39 | tuning-panel.js |
| O-Bowed | 39 | tuning-panel.js |
| O-Contrabass | 46 | tuning-panel.js **+ preset-manager.js** |
| O-Marimba | **5** | analog-eq-unit.js, compressor-unit.js |
| O-Reed | 39 | tuning-panel.js |
| O-ReverseDelay | **6** | preset-manager.js |
| O-Wind | 39 | tuning-panel.js |

**The NARROWING arm was taken**, as the plan pre-authorized. The out-of-scope
findings are **rendered English with no key** — `Load`, `Save`, `Previous preset`,
`Next preset`, `Default` on O-ReverseDelay and O-Contrabass's preset manager;
`EQ`, `COMP`, `GR` on O-Marimba's units — so they fail the first arm's own test of
"a one-line trivial fix touching no rendered string". `preset-manager.js` alone has
eighteen consumers; keying it is a rollout, and shipping it inside a tuning-panel
change would have put four unrelated plugins' UIs in one commit.

Implemented as a named, commented constant in `scripts/check-i18n.js`:

```js
const CMAKE_MODULE_JS_SCOPE = /(^|\/)tuning-panel\.js$/;
```

It is a **basename regex, not a path allowlist**, so a new consumer embedding the
panel from a different directory is still picked up. The control **survived the
narrowing**: re-fired repo-wide at exit **5** — exactly the five consumers still
RED at 39 findings each, the three out-of-scope plugins back to green.

**Still unscanned by assertions 12 / 13 / 15 for their consumers** (the TODO in
`check-i18n.js` names all four, with their consumers):

| File | consumers | state |
|---|---|---|
| `modules/persistence/preset-manager/js/preset-manager.js` | O-Contrabass, O-ReverseDelay | **RED** — 6 findings on O-ReverseDelay, and it is why O-Contrabass's red was un-clearable by this task alone |
| `modules/effects/analog-eq-unit/js/analog-eq-unit.js` | O-Marimba | **RED** — part of O-Marimba's 5 |
| `modules/effects/compressor-unit/js/compressor-unit.js` | O-Marimba | **RED** — part of O-Marimba's 5 |
| `modules/core/webview-drop-streaming/js/webview-drop-streaming.js` | O-MicrotonalSampler | **CLEAN** under the unrestricted follow; excluded only because the scope is one regex |

**Deleting `CMAKE_MODULE_JS_SCOPE` turns O-ReverseDelay and O-Marimba red
immediately.** That is the intended trigger: key those modules, then delete the
constant and the filter in the same change.

Note the shape of the O-ReverseDelay and O-Marimba findings — they are the first
evidence in this rollout that **a plugin can be fully localized in its own files
and still render unkeyed English from a module it embeds**. Every "this plugin
passes check-i18n" claim made before Task 1 was scoped to the plugin's own
directory.

---

## D3 — `modules/registry.yaml` `used_by` for scala-tuning-engine is STALE

It names **O-Bells, O-Formant, O-IntonationPad** — all forks with private copies,
none of them live consumers — and **omits O-Reed, O-Wind and O-MicrotonalSampler**.
The live set, re-derived from the CMake grep at planning and again at execution,
is: O-Bassoon, O-Bowed, O-Contrabass, O-Reed, O-Wind (path-embedding), plus
O-MicrotonalSampler as a diverged private copy.

**Left as found by decision**, and now carrying a comment saying so. Nothing
should be driven from it until it is rebuilt from the CMake grep. `version:` and
the changelog block WERE brought forward to 3.1.0 in both `registry.yaml` and the
module's own `module.yaml` (which was separately stale at 2.1.0).

`modules/registry.yaml` also has **no per-module `changelog` key** — no entry in
the file has one — so the plan's "add a changelog line" was satisfied with a
comment block above the version, and the registry's own `version`/`last_updated`
were bumped per the file's own mandatory NOTE.

---

## D4 — O-MicrotonalSampler, untouched by decision

Byte-unchanged, asserted with `git diff --quiet` in Tasks 1 and 2. It embeds its
own `Resources/ui/js/tuning-panel.js` (317 lines diverged; its header says so),
already localized en/fr/zh-Hans since v1.24.0.

The module and the MTS copy **now share key names verbatim**, so a future re-sync
is a diff and not a redesign. **Two keys exist only in the MTS copy:**
`label.totalSpan` and `label.tuningPanelUnavailable`. They are MTS-only surfaces;
the module's markup renders neither, and adopting them would have put dead keys in
five tables.

---

## D5 — The 17-of-37 coverage hole, and what `check-ui-labels` can actually see

Measured directly on all five, comment-stripped, with visibility OR-ed over the
whole state walk. **Identical on every consumer:**

| | count | which |
|---|---|---|
| reached the DOM and VISIBLE | **20 / 37** | |
| in the DOM but NEVER VISIBLE | **10** | `label.tkHint` + 6 `#library-filter` `<option>`s (catAll, catHistorical, catJust, catEdo, catNonOctave, catWorld) + 3 `#generator-type` `<option>`s (genEdo, genHarmonic, genRank2) |
| **NEVER ENTERED THE DOM** | **7** | `label.genStartHarmonic, label.genEndHarmonic, label.genGenerator, label.genR2Period, label.genNotes, label.noteCount, label.rotationMode` |

**`check-ui-labels` can only see 10 of the 17.** Its NOTE reads `10 never became
visible`; its denominator is `[data-i18n]` membership of the **final** DOM, so a
caption whose code path never ran is not counted as never-visible — **it is not
counted at all.** The 7 above sit behind `setGeneratorType('harmonic'|'rank2')`,
`drawRotationTable()` and `renderLibraryList()`, none of which the JUCE stub's
invented native-function returns drive.

**Nine of the 10 in-DOM-but-invisible are `<option>` inside a collapsed
`<select>` — structurally unmeasurable by any DOM geometry probe** (wave 4e N8's
distinction: unmeasurable, not merely unmeasured). They should be recorded as such
rather than chased. Closing the rest needs states that change the generator type
and select the Rotation visualisation.

Related, and it will bite a later reader: **`N of M` in that coverage line is not
a fraction and N can EXCEED M.** O-Reed reads `111 of 84`; O-Bassoon read
`30 of 28` before this task. N counts paths seen visible across the cumulative
walk; M is final-DOM `[data-i18n]` membership. A plugin that re-renders a subtree
between states produces N > M routinely.

---

## D6 — `.interval-list-header` wraps in English at any column narrower than ~130 px

The English `Intervals · notes: 11` wraps to two lines; the French
`Interv. · notes : 11` does not, so the header gives back a line and everything
below it rises. Four of five consumers hit it (O-Bowed 122 px, O-Contrabass
122 px, O-Reed 122 px, O-Wind 112 px); O-Bassoon's 842 px column does not wrap.

**Any future consumer of scala-tuning-engine that mounts the panel in a narrow
column will need the same `min-height` floor, and the shared CSS still cannot
carry it** — `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css` has
five consumers whose measured English boxes are 24 / 22 / 24 / 22 px, four
different numbers. Wave 4c's D1 left that file as found for the same reason and
this task did too (byte-unchanged, asserted).

**Pre-existing French wrap, for the record:** there was none. The tuning panel was
English in both languages until `842bf6e0`, so its geometry was identical by
construction. **Every one of these five French regressions was caused by this
task's own localization**, and O-Bowed — the only consumer whose state file
already reached the panel — was shipping it RED on the tree as found between
`842bf6e0` and `d848337a`.

---

## D7 — Z5 collisions on O-Bassoon: none

`i18n-zh-lint --plugin O-Bassoon --verbose` after the 37 rows landed: **0
findings**, 86 zh entries checked, **0 at `'mt'`, 0 Z5 collisions**. The plan's
M13 blind-reverse-read trigger never fired, so nothing was re-read and nothing is
open. The zh rows ship at `reviewed: 'bt'` with `'native'` open, as every zh row
in this rollout does.

---

## D8 — A key collision that WOULD have shipped a regression

O-Contrabass already owned `label.loadScl` for its own `#scl-load-btn`
(`index.html:1250`, *Load .scl…* / *Charger .scl…*). The module's row of the same
name, inserted later in the same object literal, **silently overwrote it** —
duplicate object key, last one wins — changing a shipping button's French to
*Ouvrir .SCL* and losing both the ellipsis and the reviewed wording.

**Nothing but the plan's own arithmetic caught it:** 57 + 37 should be 94 and the
gate read **93**; `check-i18n` passed at 93. Fixed by renaming the *plugin-local*
key to `label.loadSclFile`; the shared module vocabulary keeps `label.loadScl`.

**Generalize before the next module lands rows in five tables:** a shared module
that adopts a vocabulary can collide with a consumer's private key of the same
name, and the collision is silent in JavaScript and invisible to the gate. The
LABELS-count arithmetic is currently the only detector. A `[N] no key is declared
twice in one table` assertion would catch it directly and does not exist.

---

## D9 — Two recorded commands that are not executable as spelled

Both are instances of `pattern_recorded_gate_command_not_executable_as_spelled`.

1. **`awk '/name: scala-tuning-engine/,/^  - name: /'`** (plan verify 4c) collapses
   to a single line: awk tests the range's END pattern on the START line, and
   `  - name: scala-tuning-engine` matches `^  - name: `. It can never see the
   version line and fails regardless of the file's contents. Working form:
   `awk '/- name: scala-tuning-engine/{f=1;next} f&&/^  - name: /{f=0} f'`.
2. **`set -- $PV` in a `for PV in "O-Bassoon 1.5.0" …` loop** does not word-split
   under zsh, so the loop body sees `plugins/O-Bassoon 1.5.0/CHANGELOG.md` as one
   path. It fired in Task 2 and again in Task 3's verify block. Already in repo
   memory as `pattern_zsh_no_word_split_backup_loop_strays`; run those loops under
   `bash -c`.

A third, for anyone writing an ad-hoc probe against `scripts/serve-ui.js`:
**`resolvePlaywright()` returns the MODULE, not a path** — passing it to
`require()` throws `ERR_INVALID_ARG_TYPE`.

---

## D10 — An inert verify arm, which should not be copied forward

The Task 2 plan grounded "the state had an effect" on
`grep -qE 'viz-btn|pitch-circle-label|library-header-text|generator-header-text|tuning-file-btn'`
over `check-ui-labels --verbose` output, calling a miss "the state is decorative".
Measured, that grep returns **0 in all three conditions** — before any tuning
state, after the state with the gate RED, and after the state with the gate GREEN.
`check-ui-labels` names an element only in a FAIL line or the never-visible list,
and those five class names are exactly the elements that are always visible and
always pass.

**The arm carries no information at all — it is not inverted, it is inert**, and
it would have failed the task in every condition. Replaced with a direct EFFECT
measurement (module caption nodes in the live DOM: 0 → 13 on all five) and the key
census in D5.

---

## D11 — Two `auval` warnings, both PRE-EXISTING, neither caused by this task

ONE cold sweep at the end of the batch, every triple read off `auval -a` and
`auval -v` given three unquoted words. **All five PASS**, `AU VALIDATION
SUCCEEDED`, exit 0. This task touched no parameter, range, type, state format or
audio path, so both warnings below predate it and neither is a new defect — but
they are recorded so a later reader does not attribute them to a Chinese wave.

- **O-Bowed** — `Parameter ID:127996179 "Bow Position"` (Generic, min 0.02,
  default 0.12, max 0.30): `WARNING: retrievedValue = 1.000000 (was 1.000000),
  Parameter did not retain maximum value when set`. The parameter is normalized
  0–1 at the AU boundary while its declared range is 0.02–0.30; auval sets the
  declared max and reads back the normalized one. Worth a look next time
  O-Bowed's parameter layout is open.
- **O-Wind** — `WARNING: Source AU supports multi-channel output but does not
  provide a channel layout`. Standing shape for the plugins in this family;
  see the repo's AudioChannelSet notes before "fixing" it.

`Bad Max Frames - Render should fail` appears in all five logs and is the name of
an auval test that expects a failure, not a finding.

**Two O-Contrabass orphan bundles are installed and were left as found:**
`O-Contrabass-pre-2-5-dev.component` and `O-Contrabass-pre-port.component`, both
at version 1.0.0. They do **not** shadow the shipping plugin — distinct AU
subtypes `OCb5` and `OCbP` against `OCbs` — so this is not the dev↔release
variant-shadowing failure CLAUDE.md warns about. `build-and-install.sh`'s Phase 4
sweep is keyed on `<Name>` and `<Name>-dev` only and structurally cannot see a
third product name, so these will persist until removed by hand.

**No build emitted `⚠ Sweeping ALTERNATE-variant`** on any of the five — no
dev↔unsuffixed orphan was present for any of them.

**The "~15 min cold auval rescan" figure did not hold on this machine today.**
`auval -a` after five consecutive installs took **84 seconds** (21:23:39 →
21:25:03), and the five `auval -v` runs together took under two minutes. The
batch-at-the-end discipline is still right — it is one rescan instead of five —
but the 15-minute budget it was sized against is stale, and a plan should not
refuse a mid-batch check on the strength of it. The five builds themselves took
**3 minutes total** (30–60 s each; the build directory was warm and only the
version string and the UI resources changed).

---

## Explicitly for the next wave

1. **Schedule O-Contrabass.** It needs the same 37 zh rows as the three 4f
   plugins and is in nobody's wave list (D1).
2. **Budget the two zh geometry pins per plugin, not just the table rows.** The
   ratio block (11 selectors, three of them buttons named separately) and the CJK
   tail on those three button classes. Numbers are in D1 item 2 and transfer
   directly.
3. **Run the N12 enumeration-form census** on the three untouched
   `ui_tip_render_check.js` files. Waves 4a–4c found four forms, 4d a fifth, 4e no
   sixth.
4. **Fire every precondition, including "this gate passes."** All three of this
   task's preconditions were fired and all three were green — the five plugins at
   `check-i18n` exit 0 with `1 module(s)`, the O-Bells 2-late/0-dead boot census,
   and Tasks 1–2 committed clean.
5. **Use the two-arm CMake reader unconditionally.** This task: four quoted, one
   unquoted (O-Contrabass), no third shape, exactly as the plan predicted — the
   one place in three tasks where a prediction about the tree held without
   correction.
6. **Do not drive anything from `modules/registry.yaml` `used_by`** until it is
   rebuilt from the CMake grep (D3).
7. **Report every prediction in the plan that was FALSE as stated.** This task
   produced **nineteen** across three tasks (7 + 10 + 2). Wave 4d had three, 4e
   had fifteen. The count is itself the finding: a plan that measures the tree at
   planning time still mispredicts what executing against it will expose, and the
   miss is concentrated in what the GATES will say, not in what the FILES contain.
