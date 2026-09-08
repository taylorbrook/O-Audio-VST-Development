# Wave 4f — deferred items and structural findings

Quick task `260907-ja8`. Five plugins localized to Simplified Chinese: **O-GrainScatter 2.8.0,
O-Wind 1.21.0, O-Contrabass 1.10.0, O-Reed 1.6.0, O-Bowed 1.9.0**. Batch closed 2026-09-07.

Everything below is either **deferred by decision** or a **structural finding** the next wave needs.
Nothing here blocked this wave.

---

## Structural findings

### F1. The g5l inventory grep has a false-negative class, proved on TWO shapes

The grep that has been this rollout's gate-file inventory since Stage 3 —

```bash
perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g; s{^\s*\*.*$}{}gm' "$f" \
  | grep -q "en,fr\|\[.en., *.fr.\]"
```

— returned **three** files for this wave and the true answer was **five**. It is blind to:

1. **a three-element array literal** — O-GrainScatter's `['en','fr','en']` walk (the third element is
   a deliberate English return pass), which does not match `\[.en., *.fr.\]` because of the trailing
   member;
2. **per-language object property access** — O-Contrabass's `ui_frontend_check.js`, which holds no
   list, no walk and no `en,fr` string at all, only `!e.en || !e.en.t` / `!e.fr || !e.fr.t` pairs
   feeding real `check()` calls. **That file has never appeared on any inventory in the rollout**,
   and left alone after the flip it would have asserted that every markup key resolves in English and
   French and said nothing whatever about Chinese.

**Report the correction to the METHOD, not only to the count.** The working inventory is the wider
comment-stripped census on `["']fr["']|\.fr\b` (verify arm 12), which returned 13 files at planning
time and 8 after this wave.

**Re-fired at close-out:** the g5l grep now returns **nothing at all** — all five of this wave's
files are off it and C8 kept the explanatory comments from putting them back.

### F2. Eight two-language gate files remain in the repo, and nobody has audited them

Measured at close-out with the wider census. Five of the thirteen were this wave's; these eight
remain:

| file | `fr` hits (comment-stripped) |
|---|---|
| `O-Bass/tests/ui_tip_render_check.js` | 2 |
| `O-Bells/tests/ui_tip_render_check.js` | 3 |
| `O-Chorus/tests/ui_tip_render_check.js` | 2 |
| `O-Emulator/tests/ui_tip_render_check.js` | 3 |
| `O-Freeze/tests/ui_tip_render_check.js` | 2 |
| `O-MicrotonalSampler/tests/ui_tip_render_check.js` | 2 |
| `O-ReverseDelay/tests/ui_frontend_check.js` | 2 |
| `O-SimpleReverb/tests/ui_tip_render_check.js` | 3 |

**Every one of these plugins is already three-language.** Their gates therefore either derive the
list correctly, or are vacuously green on a page whose third language they never visit. Nobody has
looked. This is not a wave-4g blocker — none of these eight is in wave 4g's plugin set (F9) — but
it is the whole remaining surface of the N12 question and it is now stated with a file list.

### F3. `svg-font-attr` is a DIVERGENCE report and is blind to the defect it was expected to catch

On O-Contrabass the screen reads **`1 attribute carrier(s), 0 finding(s)` at every stage** — on the
tree as found, with the Chinese column landed and the attribute still naming no CJK face, and after
the fix. It compares `normStack(r.ffAttr)` against `normStack(r.ff)` and reports only the subset
whose attribute DIVERGES from the computed stack (`scripts/measure-ui.js:553-570`). On that element
the attribute IS the computed stack, so there is nothing to diverge.

**Its verdict is invariant across the entire defect and the entire fix.** The detector that actually
named the node was `undeclared-font`. Carry forward: on a plugin whose SVG `font-family` attribute
already equals the CSS that would otherwise apply, `svg-font-attr` proves nothing and its carrier
count (1) is its only live signal.

### F4. `measure-ui`'s `line-height-normal` residual list can name a TEXT-FREE `<canvas>`

The plan's verify block has two arms that count residuals differently, and on exactly one plugin
they disagree. O-Contrabass's screen reports **9**; the plan's own `node -e` arm reports **6**.

The three-node gap is `#canvas-schelleng`, `#canvas-spectrum` and `#canvas-vu` — `<canvas>` elements
with `own` text `""`, in the `han` set only because they carry a Chinese `aria-label`. The screen's
"leaf nodes only" filter is `kids === 0` with no text requirement; the `node -e` arm additionally
requires `own.trim()`.

**The residual count is not a caption count.** Both readings are correct and they answer different
questions; a canvas can never move and can never be pinned. The other four plugins agree exactly
(7/7, 21/21, 1/1, 4/4).

### F5. `system_profiler` proves presence, NEVER absence — confirmed twice

`system_profiler SPFontsDataType | grep -c "Family: Times$"` reports **0** on this machine, while
`/System/Library/Fonts/Times.ttc` is on disk and Chromium resolves `.toggle` to it. Re-fired at
close-out: still **0**.

The installed-family table the rollout has used since wave 4b has a false-negative class. Where a
face matters, probe the resolved face through CDP `CSS.getPlatformFontsForNode`. A zero in the
table is a claim about the query, not about the machine.

### F6. M8's line-box ratio table is LEAF-scoped and does not cover a form control

M8 gives `10px → 1.1`. Measured, a `<button>` at 10 px has a **13 px** content box, ratio **1.3** —
the UA `font` shorthand resets `line-height` on form controls. This is the same mechanism that makes
`d848337a`'s ratio block name `.viz-btn`, `.tuning-file-btn` and `.generator-btn` separately.
**Derive form-control ratios from the element's own box; never from the table.**

### F7. A `min-width` floor that BINDS hides the natural box, and a floor measured while in place is not measured

O-Bowed's `.tonic-label` natural French box is **42.06 px** against a `d848337a` floor of **41.59**
— a 0.47 px excess, *inside* `check-ui-labels`' 0.5 px tolerance. The gate stays green while the
French arm renders 0.47 px wider than the English one. **The only way to see it is to remove the
declaration and measure.**

And `min-width: 0` is **not** "no floor": the default for a flex item is `auto` (its content size),
so `0` explicitly permits shrinking BELOW content. Setting it to 0 to take a natural box read
`.tonic-label` at 12.58 px against a true 28.47 and wrapped the Chinese arm to two lines. **Remove
the declaration entirely, measure, restore.**

### F8. `wrap-count` divides a box by an ASSUMED line box, so a ratio pin changes its verdict with no pixel changing

O-Reed's screen went **1 → 2** with `span.xy-axis-label` reporting byte-identical boxes on all three
runs (en `9.00 × 62.78`, fr `9.00 × 69.61`). Pinning `.xy-axis-label` to 1.125 moved it out of the
ESTIMATED set (142 estimated before, 95 after), so 69.61 is now divided by 9.00 instead of 9.6.

The direction is **not** guaranteed: O-Bowed's stayed at **1** while its estimated denominator fell
110 → 76. Read the screen's own `N of them ESTIMATED at fsn * 1.2` header before treating a delta
as a defect.

### F9. `--forward-provenance` takes a VALUE, and the rule text says otherwise

It is `val('--forward-provenance')` at `scripts/i18n-zh-backtranslate.js:228` — it consumes the NEXT
argument. Written as `--emit X --plugin X --forward-provenance --out /tmp/f`, the manifest records
`forwardProvenance: "--out"`, the emit succeeds, the ingest joins, **the refusal control passes and
the identity check is vacuous.** The executor rules (R5/R6/ship-bar step 10) phrase it throughout as
a bare flag. **The rule text should read `--forward-provenance "<string>"`.** The script's own usage
line at `:69` is correct; this is a rule-text fix, not a script fix.

### F10. The ingest's refusal ORDER has three steps, and only the third is about the manifest

**reverse-pass `--provenance` → forward provenance → id join.** A wrong-manifest control run without
`--provenance` refuses with *"no --provenance was given"*; one run without forward provenance
refuses for that reason. Either way control 2 proves nothing about the manifest and reads like a
pass. Both refusal controls must be fired with a properly provenanced emit, and each then fires for
its own reason:
`WRONG MANIFEST FOR THIS BATCH: all N returned ids are unjoinable, not one.`

### F11. The stale-body probe as every plan in this rollout spells it is BYTE-BLIND to a French curly apostrophe

The alternation's French J9 branch is `rien d.autre`. The files write `rien d’autre` with **U+2019**,
three bytes in UTF-8, and `perl` without `-CSD` matches `.` against ONE byte. **The French "and
nothing else" clause is a real J9 site the probe cannot see** — a plan that used the probe's count as
its worklist would have corrected the English and shipped the French falsehood. Run it with
`perl -CSD`, or grep the French clause separately.

Related: **the probe's COUNT is a property of its alternation, not of the file.** It reported 7 for
O-Wind's 5 substantive sites and 5 for O-Reed's, whose true substance was 6.

### F12. `undeclared-font` cannot see a collapsed `<select>`'s Han — the same blindness as s71 D5's nine `<option>`s

`measure-ui`'s `han` is computed over the node's OWN text plus `data-tip`, `data-tip-title` and
`aria-label`. A collapsed `<select>`'s selected-option text is none of those, so `#library-filter`
and `#generator-type` read `han: false` however much Chinese they paint. They are in the CJK-tail
rule anyway, on a decision about what the control paints. **The screen cannot confirm the reasoning
and is not the oracle for it.**

### F13. The plan's duplicate-key scan CANNOT read `NONE` on any three-language table

`node -e "…matchAll(/^\s+'([A-Za-z0-9._-]+)':/gm)…"` returns **`zh-Hans`** on every table that has a
zh column — including O-Bassoon, O-Detune and this wave's own five — because a multi-line
`'zh-Hans':` block starts a line and the regex matches any quoted property name at any indent.
**Corrected form: exclude the members of the declared `LANGUAGES`.** A language code is never an
entry key. Under the corrected form all five read `duplicates NONE`, which is what makes s71 D8's
collision class detectable at all.

### F14. Six recorded commands and helpers that are not usable as written

All instances of `pattern_recorded_gate_command_not_executable_as_spelled`. New in this wave:

1. **`grep -A14 "line-height: 1.11" | grep -c tuning-container`** reads **0**, including on the
   transfer source O-Bassoon. `d848337a` writes the eleven selectors BEFORE the declaration, which
   is how CSS is written. Working form: **`grep -B14`**, which reads 11 on both.
2. **`serve()` in `scripts/serve-ui.js` returns `{ server, port, close }` and has NO `url` field.**
   `page.goto(srv.url)` fails with `url: expected string, got undefined`. Build it:
   `` `http://127.0.0.1:${srv.port}/` ``.
3. **`buildRoot()` returns an OBJECT** — pass `built.root` to `serve()`, not the return value.
4. **`resolvePlaywright()` returns the MODULE, not a path** (s71 D9, confirmed live again).
5. **A `'bt'` string literal inside `node -e '…'` under bash loses its quotes** and emits
   `reviewed: bt`, a bare identifier, which throws `ReferenceError: bt is not defined` on the next
   parse. **Write promotion scripts to a FILE.**
6. **`git commit -- <paths> -F <file>` fails** with `pathspec '-F' did not match any file(s)`.
   Options go BEFORE the `--`: `git commit -F <file> -- <paths>`.

### F15. `claude -p --model claude-opus-4-1` is silently remapped to Opus 5

stderr: `⚠ claude-opus-4-1 is automatically remapped to Opus 5 (the latest Opus).` Confirmed twice
(Tasks 4 and 5). R4's "different model" requirement is still satisfied — round 1 Sonnet 4.5, round 2
Opus 5, genuinely different families — **but not by the name requested**, and a summary that records
the requested name without reading stderr names a model that did not run.
`CLAUDE_CODE_DISABLE_LEGACY_MODEL_REMAP=1` keeps the requested one.

### F16. The `--emit` + `--plugin` pair is load-bearing and silently so

`--emit` alone is CORPUS-scoped and its own usage line is wrong. With `--emit X` only, the emitter
hands an external reader every already-shipped row in the corpus as that plugin's work. **Pass
both.** The emitter has no row-range flag, so chunking must be done on the emitted file with the
manifest as the key.

### F17. Chunk on the CONCEPT SPLIT, not on size

O-GrainScatter's `label.stutterGate` / `tip.stutterGate` divergence is visible only because the same
Chinese string went to two different readers. Tasks 3, 4 and 5 adopted it deliberately — 46 / 36 / 34
concepts split caption-from-title across the two chunks. Two emits of the same target share **0**
blinded ids, so each chunk carries its own salt for free.

### F18. `N of M` in the coverage line is not a fraction and N can EXCEED M

Confirmed again: O-Reed reads `111 of 84`, O-GrainScatter `52 of 49`. N counts paths seen visible
across the cumulative walk; M is final-DOM `[data-i18n]` membership. A plugin that re-renders a
subtree between states produces N > M routinely.

---

## Deferred items

### D1. The glossary root `'dist lpf'` is WRONG for the only site in the corpus that uses it

`scripts/i18n-zh-glossary.js:433` reads `'dist lpf': ['失真低通']` — *distortion* lowpass. On
O-GrainScatter, the only site in all 44 tables carrying the term (`grep -rn "Dist LPF"` hits that
file and nothing else), `Dist` is **DISTANCE**: the body reads *"Distance LPF sets how much Distance
darkens the cloud"*. 失真低通 names a control this plugin does not have.

Shipped as a `termNote` exemption on both `label.distLpf` and `tip.distLpf` (a termNote is
ENTRY-scoped, N4). The blind reverse read independently returned **"Distance Lowpass"**.

**The correction is `['距离低通']`, and it is NOT made here** — a shared-script edit is outside a
per-plugin task's path scope and touches a file five other tasks read. Carried as a corpus-level
item.

**The general lesson:** a glossary root derived from ONE plugin's caption+title pair looks measured
(2 occurrences) but had no second site to check it against. **This may not be the only such root.**
Before writing a termNote against a root, check the corpus site count.

### D2. O-Reed's 20 missing and 7 dead native-function registrations

A C++/JS bridge defect on the tuning tab, **not a localization one**. Measured before and after the
work, **unchanged in both directions: 22 called, 9 registered, 20 MISSING, 7 DEAD.** It blocked
nothing — every tuning caption still mounts, renders and is compared, because the panel's markup and
its i18n bindings are independent of whether its data calls resolve.

```
MISSING (called from JS, never registered in C++) — 20:
  getTuningIntervals getTuningName getTonicNote getOctaveStretch setSingleInterval
  setTonicNote getEmbeddedTuningList loadEmbeddedTuning generateEDO generateHarmonicSeries
  generateRank2 applyGeneratedScale loadScalaFile loadKBMFile saveScalaFile saveKBMFile
  exportTuningHTML setOctaveStretch getMasterTune setMasterTune
```

Repairing 20 native functions is a release of its own. Both lists are in Task 4's SUMMARY §10.

### D3. Two stale `d848337a` floors on O-Reed

`.tonic-label` (41.59 px) and `.interval-list-header` (24 px) were **not** re-measured with the floor
removed. O-Bowed's equivalents measured 42.06 and 22 once the declarations were taken out entirely
(F7). Both O-Reed values are almost certainly stale in the same direction — ≤0.5 px and 2 px — both
inside `check-ui-labels`' tolerance, both harmless today, and **no gate in this repo can see either
of them.** Not fixed here: O-Reed's commits are closed and it is another plugin's tree.

### D4. The 17-of-37 shared-module coverage hole — inherited from s71 D5, unchanged

Identical on all four consumers: **20 of 37 module keys reach the DOM and are visible; 10 never
become visible; 7 never enter the DOM at all** (`genStartHarmonic, genEndHarmonic, genGenerator,
genR2Period, genNotes, noteCount, rotationMode`).

Nine of the ten never-visible are `<option>` elements inside a collapsed `<select>` —
**structurally unmeasurable by any DOM geometry probe, not merely unmeasured.** The seventh class is
invisible to `check-ui-labels` entirely, because its denominator is final-DOM `[data-i18n]`
membership.

Closing it needs states that change the generator type and select the Rotation visualisation. This
wave did not budget them.

Note the two figures that both describe this surface and answer different questions: the tuning state
mounts **30** module captions under a cumulative OR-ed-visibility walk (37 − 7 never in the DOM), and
**20** under a single `getBoundingClientRect().height > 0` snapshot (37 − 7 − 10). Neither is a
regression against the other.

### D5. `plugins/O-Strata/` still makes both repo-wide lints exit 2, for a reason no longer visible in `git status`

O-Strata was committed at `4965c271` / `8dae0cc7` by a concurrent session and its working tree is
clean, so `git status --short | grep -c "O-Strata"` now reads **0**. The lints enumerate `plugins/*`
from **disk**, not from the index, and O-Strata still has no `i18n.js`.

`?? plugins/O-Strata/` and `1 plugin(s) could not be read` were always two independent facts; this
wave is where they came apart. **The load-bearing baseline is the finding count and the unreadable
count, never the exit code and never the `git status` line.** Closing it means either an `i18n.js` in
O-Strata or an unreadable-plugin allowance in the lints; neither belongs in a localization wave.

### D6. The glossary divergence report — SIX entries open, none added by this wave

Inherited verbatim from wave 4e D4: `WebGL 不可用` vs `不支持 WebGL`; Scatter and Diffusion; `通过长度`
for *pass length*; `分割` for *Divisions*; `键位` for *keyswitch*; `调制` for *MOD*. Every one is a
corpus-wide decision, not a per-plugin one.

**This wave added no seventh.** Its four termNotes are per-page sense exemptions against correct
roots, not divergences: `Dist LPF` → 距离低通 (D1 is a wrong ROOT, which is a different thing),
`flutter` → 花舌 on O-Wind and O-Reed (the tape sense 快抖 is correct where it was written), and
`count` → 弦数 on O-Bowed (数量 is correct for `count` everywhere it does not sit beside 量).

### D7. The `Z6` budget backfill — inherited, unchanged

`i18n-zh-lint` prints it on every run: **3 of 552 glossary terms carry a measured character budget;
549 are UNBUDGETED and Z6 is inert on them.** **This wave added none**, for the same reason waves 4d
and 4e added none: every geometry finding was a SHRINK, and a shrink wants a floor, not a budget.

### D8. `modules/registry.yaml` `used_by` for scala-tuning-engine is still STALE

s71 D3, left as found by decision. It names O-Bells, O-Formant and O-IntonationPad (forks with
private copies, none live consumers) and omits O-Reed, O-Wind and O-MicrotonalSampler. **Drive
nothing from it until it is rebuilt from the CMake grep.** This wave read the CMake grep, not the
registry.

### D9. The two O-Contrabass orphan bundles — LEFT as found, and they are NOT variant shadowing

`O-Contrabass-pre-2-5-dev.component` (subtype `OCb5`) and `O-Contrabass-pre-port.component`
(subtype `OCbP`), both at 1.0.0, against the shipping `OCbs`. Confirmed at close-out by reading
`auval -a`: three distinct triples, three distinct product names. **They do not share the AU triple
with the current O-Contrabass and cannot shadow it.**

`build-and-install.sh`'s Phase 4 sweep is keyed on `<Name>` and `<Name>-dev` only and structurally
cannot see a third product name, so they persist until removed by hand. No build in this wave
emitted `⚠ Sweeping ALTERNATE-variant`.

### D10. Two pre-existing `auval` warnings, both re-confirmed at close-out

Neither is attributable to this wave, which touched **no parameter, range, type, state format or
audio path** on any of the five.

- **O-Bowed** — `Parameter ID:127996179 "Bow Position"` (Generic, min 0.02, default 0.12, max 0.30):
  `WARNING: retrievedValue = 1.000000 (was 1.000000), Parameter did not retain maximum value when
  set`. Normalized 0–1 at the AU boundary against a declared range of 0.02–0.30; auval sets the
  declared max and reads back the normalized one. **Worth a look next time O-Bowed's parameter
  layout is open** — it is not a localization item.
- **O-Wind** — `WARNING: Source AU supports multi-channel output but does not provide a channel
  layout`. A standing shape for this family; read the repo's AudioChannelSet notes before "fixing"
  it.

`Bad Max Frames - Render should fail` appears in all five logs and is **the name of a test that
expects a failure**, not a finding.

### D11. `reviewed: 'native'` is OPEN on all 860 rows this wave shipped, and on all 4621 in the corpus

This project has no native Simplified Chinese reader. A **disclosed quality level**, printed by lint
rule R1 on every run and stated in all five CHANGELOGs. **A blocker for nothing.**

### D12. Re-inherited unchanged

- **wave 4e D2 — O-Bells' two late tip bindings** (`#ref-pitch-knob`, `#octave-stretch`). Used as the
  boot census control in this wave: **2 late before, 2 late after**, which is what makes the 0 DEAD
  beside it evidence rather than a number.
- **wave 4e D7 — the pre-existing French wrap on O-simpleFM** `div.routing-label`.
- **wave 4c D4** — O-Chorus's inert CJK tail; O-Gain's and O-IntonationPad's missing tip gates.
- **s71 D2** — the four shared-module JS files `check-i18n` still does not scan.
- **s71 D4** — O-MicrotonalSampler, untouched by decision (its own diverged 317-line copy).
- **s71 D6** — `.interval-list-header` wraps in English at any column narrower than ~130 px, and the
  shared CSS still cannot carry the floor: five consumers, four different measured English boxes.

---

## Explicitly for wave 4g

**The wave-4e carry-forward sheet does NOT name wave 4g's plugins.** Its "Explicitly for wave 4f"
section is the last forward-looking list it carries and it says nothing about a wave after this one.
The set below is derived live from the emitter at close-out, not carried.

**Wave 4g is six plugins and it closes the corpus.** `i18n-zh-backtranslate` reports 37 of 44 plugins
carrying at least one zh row. The six localized plugins with **zero**:

| plugin | rows owed | gate file? |
|---|---|---|
| O-simpleGrain | 153 | none |
| O-simpleSampler | 145 | none |
| O-Polystutter | 133 | none |
| O-simpleSubtractive | 133 | none |
| O-simpleAdditive | 131 | none |
| O-Orbit | 125 | none |

`153 + 145 + 133 + 133 + 131 + 125 = 820`, and `5441 − 4621 = 820` **exactly**. Wave 4g reaches
`4621 → 5441 of 5441` and `37 → 43` localized plugins.

**The 44th plugin, O-Strata, is not in the set** — it has no `i18n.js` under either UI root, is at
Stage 0, and is the standing cause of both repo-wide lints' exit 2 (D5).

Then, by number:

1. **NONE of the six carries a gate file.** All six HAVE a `tests/` directory and every one of them
   holds **zero `.js` files** — verified with `find`, not with a glob, because a zsh glob that
   matches nothing aborts the whole command and reads as an answer. Those directories are the C++
   Stage-2 `render-harness` trees (wave 4e D6), not UI gates. **The N12 enumeration census has no
   subject in wave 4g.** Take no
   invented gate (wave 4c D4: a gate written mid-localization is a gate nobody has calibrated);
   `check-i18n`, `check-ui-labels`, `measure-ui` and `boot-all-uis` are the whole instrument. The
   remaining N12 surface is F2's eight already-three-language files, which belong to a different set
   of plugins and to nobody's wave.
2. **`plugins/O-Orbit/libs/SAF` is the repo's only submodule.** O-Orbit is in wave 4g. Run the
   submodule commit guard before every commit and **never stage a path under it**.
3. **None of the six is a `scala-tuning-engine` consumer** — the live consumer set is O-Bassoon,
   O-Bowed, O-Contrabass, O-Reed, O-Wind, plus O-MicrotonalSampler's private copy, and all six are
   now localized. **The 37-row shared-module column is DONE and wave 4g inherits none of it**: no
   `1.11` ratio block, no three-button CJK tail, no `d848337a` floors, no 17-of-37 coverage hole.
   That is the single largest reduction in per-plugin cost since the module question opened in 4e D1.
4. **Re-derive the installed-family table anyway (M5).** It is a property of the machine. It has now
   read identically across waves 4d, 4e and 4f — Georgia 4, Times New Roman 4, Arial 4, PingFang SC
   6, Songti SC 4, Helvetica Neue 14, Garamond 0, Microsoft YaHei 0 — and **a zero in it is a claim
   about the query, not about the machine** (F5).
5. **Use the two-arm CMake reader unconditionally (R9).** This wave: two unquoted literals, three
   quoted, **no `set()`-variable form at all** — the first wave since 4c with no third shape. That is
   the answer to wave 4e instruction 4 and it does not license a one-arm reader.
6. **Measure the existing pin surface; never inherit a table (R8).** R8 was LIVE on all five of this
   wave's plugins, as it was on all five of 4e's. **And remove a floor before measuring the box it
   floors** (F7) — this wave is where that became a rule.
7. **Fire every precondition you assert (M3).** All five tasks fired all of theirs.
8. **Read the returned ids as identical AND IN ORDER before ingesting (M12)**, fire BOTH refusal
   controls with a properly provenanced emit (F10), and pass `--forward-provenance` a real STRING
   (F9).
9. **Chunk on the concept split, not on size (F17)**, and use `--emit X --plugin X` together (F16).
10. **Report every prediction in the wave-4g plan that was FALSE as stated.** Wave 4d produced 3,
    4e 14, s71 19, **this wave 38 across five tasks plus 3 at batch level = 41.** The count is itself
    the finding, and its shape is stable across four waves: the miss is concentrated in **what the
    GATES will say and where the LINES are**, not in what the files contain.
11. **Never cite a raw line number as a worklist.** Four of five tasks found the plan interleaving
    comment-stripped and raw numbers in the same table row without labelling either. **Verify by
    pattern.**
12. **`auval -v` takes THREE UNQUOTED WORDS** (M14), read every triple off `auval -a`, and budget
    **~85 seconds** for the cold rescan, not fifteen minutes. Measured again this wave: `auval -a`
    83 s, five `auval -v` runs 1 s total.
13. **Re-inherit D1–D12 above**, with D1 (the `'dist lpf'` glossary root) now the oldest open
    corpus-level item in the ledger.

