# Stage L — T16: O-Prism — EXECUTOR REPORT

## STATUS: COMPLETE. `e4796486`, 10 files.

The gate defect reported below was fixed by the orchestrator in `f70ea7a0` and
landed as its own commit ahead of this one, per the standing precedent. O-Prism
committed on top of it. Section 1 is kept as the record of the defect, with the
verification the orchestrator added and the narrower O-Lyrica statement it
established.

**`check-i18n --strict-v2` now passes across all 43 plugins, every one on canon
v2, canon v1 count zero** — the Stage L done-condition.

---

## 1. THE (now-fixed) BLOCKER — a nested template literal desynchronized `stripJsComments`

**Where.** `scripts/i18n-extract.js:286` `stripJsComments()`.

**The wrong assumption.** It handles a template literal with the same flat loop
it uses for `'` and `"` — copy until the next matching quote character — and
does **not** recognise `${ … }` interpolation. `readLiteralAt()` at line 934
already does this correctly, recursing through the interpolation; `stripJsComments`
never calls it.

So on this shape, which is O-Prism's interval-list row (index.html script line 1796):

```js
html += `<div>${showDev ? `<span>${devSign}</span>` : '<span></span>'}</div>`;
```

it opens at backtick 1, closes at backtick 2 (which actually OPENS the inner
template), reads the inner template's text as CODE, opens a new "string" at
backtick 3 and closes it at backtick 4. From that point the scanner is one quote
out of phase **for the rest of the file**.

**Minimal negative control** (both halves, run against the shipped extractor):

```js
const nested = 'let h = `<div>${c ? `<span>${x}</span>` : \'<b></b>\'}</div>`;\n'
             + '// a comment\n'
             + 'el.innerHTML = \'<p data-i18n="label.live">Live</p>\';';
const flat   = 'let h = `<div>` + (c ? `<span>` : \'<b></b>\') + `</div>`;\n'
             + '// a comment\n'
             + 'el.innerHTML = \'<p data-i18n="label.live">Live</p>\';';
```

| | comment stripped? | `markupKeyRefs` |
|---|---|---|
| with the nested template | **NO** | `[]` — the live key is invisible |
| same code, no nesting | yes | `['label.live']` |

**Measured on the real page.** The inline module has 206 `//` comments; after
`stripJsComments` **6 survive**, the first at script line 1822 — i.e. everything
after line 1796 is scanned in a corrupted phase. `markupKeyRefs` runs
`collectLiterals(stripJsComments(code))`, so assertion 15 reports **four live
keys as DEAD**:

```
FAIL: [15] ... 4 dead: label.genStartHarm, label.genEndHarm,
                       label.genGenerator, label.genNotes
```

All four are declared in `data-i18n` attributes inside the scale-generator's
`inputsDiv.innerHTML = '...'` templates, are read by `localizeSubtree()` on
every language change, and render correctly in the browser.

**Repo-wide blast radius: exactly two plugins.** Scanning every inline script and
every served `js/*.js` for surviving `//` comments after stripping:

| Plugin | raw `//` | surviving |
|---|---|---|
| O-Lyrica | 321 | 5 |
| O-Prism | 311 | 6 |

Nothing else in the repo trips it.

**O-Lyrica, stated accurately.** It shipped at v2.4.1 under a **partially-corrupted
comment strip**, with **no demonstrated hole**. The orchestrator planted raw
unkeyed English inside the exact region the old scanner swallowed in O-Lyrica's
inline module and both scanners caught it identically, because assertion 12's
discovery runs through `readLiteralAt`, which was never broken. My first draft of
this report called it a shipped defect; that was stronger than the evidence.

**It fails SAFE, and I measured that rather than assuming it.** Four plants of
raw unkeyed English — one *before* the nested template, two *after* it, one
*inside* an `innerHTML` markup template after it — were **all four reported** by
assertion 12. Its `.textContent =` regex and `readExpression` are
phase-independent, and `markupRows` parses a literal read locally. So the defect
produces a **false FAILURE, never a false pass**. It cannot let anything ship; it
only stops something correct from shipping.

**The fix.** In `stripJsComments`, when the opening quote is a backtick, walk
`${ … }` with the same brace-depth + recursive-literal logic `readLiteralAt`
already has, instead of scanning flat to the next backtick.

**Did it block me?** Yes, and only this. It is now fixed.

### The fix, as landed (`f70ea7a0`, `scripts/i18n-extract.js` only)

`stripJsComments()` delegates its quote branch to a recursive `scanQuoted()`, and
a backtick descends into `${ … }` via `scanInterpolation()`, which handles nested
templates, quotes, regex literals and comments. `'` and `"` keep the flat
behaviour that was always right for them.

Orchestrator's verification, which I did not repeat:

- **`check-i18n --strict-v2` across all 43 plugins: the before→after diff is
  EXACTLY ONE LINE** — O-Prism `[15]` FAIL → PASS, naming my four keys. Every
  other plugin byte-identical.
- Swallowed leading-`//` comments: O-Lyrica 38 → 0, O-Prism 12 → 0. The 17 that
  remain are correct — `//` inside template-literal *string content*
  (O-SpectralShaper's GLSL, O-Octagon's two test tools holding an HTML page in a
  template).
- **Position preservation across 270 script blocks: 0 violations.** A first draft
  broke it (+1 char on O-simpleBeatmaker) because the interpolation scanner
  inherited `n` as a regex-preceder and read `${Math.floor(n / 12) - 1}`'s
  division as a regex; `INTERP_REGEX_PRECEDERS` now drops `n`, since an
  interpolation holds an expression and `return` cannot appear there.
- Reverting `scripts/i18n-extract.js` re-fails O-Prism's assertion 15 by name
  with the same four keys.

### The four keys are LIVE, not merely no longer reported

Driven in the harness, both languages, all **three** generator variants, reading
what each keyed `<label>` actually renders plus its `dataset.label` mirror:

```
en  edo       label.genDivisions="Divisions"        label.genPeriod="Period (cents)"
en  harmonic  label.genStartHarm="Start Harmonic"   label.genEndHarm="End Harmonic"
en  rank2     label.genGenerator="Generator (cents)" label.genPeriod="Period (cents)"  label.genNotes="Notes"
fr  edo       label.genDivisions="Divisions"        label.genPeriod="Période (cents)"
fr  harmonic  label.genStartHarm="Harmonique de départ"  label.genEndHarm="Harmonique de fin"
fr  rank2     label.genGenerator="Générateur (cents)"    label.genPeriod="Période (cents)"  label.genNotes="Notes"
page errors: 0
```

`label.genStartHarm` / `label.genEndHarm` render in the **Harmonic Series**
variant; `label.genGenerator` / `label.genNotes` in the **Rank-2 Temperament**
variant; `label.genDivisions` / `label.genPeriod` in **EDO** (which also has a
static-markup twin). Every one visible, no `MIRROR-BROKEN`, no `HIDDEN`.
`label.genNotes` and `label.genDivisions` read the same in both languages by
design — both carry `sameAsEn: true`.

---

## 2. Version, commit, files

- **Version shipped:** `1.21.0` (CMake `VERSION 1.20.0` → `1.21.0`; the real
  `VERSION` keyword, no `PLUGIN_VERSION` on this plugin — nothing to raise).
- **Commit sha:** **`e4796486`**, parent **`f70ea7a0`** (the gate fix). Confirmed
  with `git show --stat e4796486`: **10 files changed, 1257 insertions, 206
  deletions**, nothing foreign. Parent read as `<sha>^`, never `HEAD~1`.
- **File set carried — 10 files:**

```
plugins/O-Prism/CHANGELOG.md                              M
plugins/O-Prism/CMakeLists.txt                            M
plugins/O-Prism/Source/PluginEditor.cpp                   M
plugins/O-Prism/Source/PluginProcessor.cpp                M
plugins/O-Prism/Source/PluginProcessor.h                  M
plugins/O-Prism/Source/ui/public/css/wavetable-editor.css  M
plugins/O-Prism/Source/ui/public/index.html               M
plugins/O-Prism/tests/ui-stub/generic-overrides.json      M
plugins/O-Prism/Source/ui/public/js/i18n.js               NEW  (git add'ed explicitly)
plugins/O-Prism/tests/i18n-states.json                    NEW  (git add'ed explicitly)
```

`git branch --show-current` was `main` and `git status --short` showed an empty
index immediately before the commit — nothing staged by another session in the
gap. No tag was created; nothing was pushed (3 commits sit unpushed ahead of
`origin/main`).

`plugins/O-Prism/.planning/{i18n-inventory.tsv,i18n-index-draft.html,i18n-labels-skeleton.js,params.tsv}`
stay **untracked and uncommitted**, as instructed.

**PLUGINS.md row I would have written (I did not touch the file):**

```
| O-Prism | Microtonal wavetable synthesizer | 📦 Installed | 1.21.0 |
```

Substitute the registry's own column order — I did not read or edit it, per the
ownership rule.

---

## 3. Counts, as I measured them

| | brief | plan | **measured by me** |
|---|---|---|---|
| LABEL | 272 | 247 | **272** |
| READOUT | 1 | – | **1** |
| UNSURE | 6 | – | **6** |
| UNIT | 0 | – | **0** |
| `src:html-text` | 247 | 247 | **247** |
| `src:html-attr` | 8 | – | **8** |
| `src:js-prose` | 24 | 9 | **24** |
| rendered text-bearing | 925 | – | **925** (now 940) |
| rendered `title=` | 6 | – | **6** (now **0**) |
| rendered `aria-label` | 0 | – | **0** (now **7**) |
| **param count (runtime dump)** | – | **173** | **173** |

The brief's table matched my re-measurement in **every column**. That is 21 for
21 across K2, K3, K4 and L. The plan was wrong on LABEL (247 vs 272) and badly
wrong on js-prose (9 vs 24); **the plan was RIGHT about the parameter count** —
173, confirmed by `O-Prism-param-dump | grep -vc '^#'`.

**What shipped:** 155 `LABELS` keys (19 `sameAsEn`), 105 `I18N_EXEMPT` entries
(101 scoped, 4 unscoped), 202 `data-i18n`, 7 `data-i18n-aria`, 2
`data-i18n-placeholder`, 14 `setLabel` call sites, 11 `localizeSubtree` calls,
**0** native `title=`.

---

## 4. The 64 `data-label` knob captions (brief §2)

- **64 keyed, 35 distinct keys** — one per distinct string, shared wherever a
  caption repeats. No French term needed a context split.
- Shape: exactly as prescribed. The key is declared on the static
  `div.knob-container`, `expandKnobMarkup()` reads it, calls
  `removeAttribute('data-i18n')` on the container, and re-attaches it to the
  generated `.knob-label` after the `innerHTML` write.

**Post-init assertion, run in the headless harness:**

```
AFTER initI18n (en):    containersTotal=64  containersKeyed=0  labelsKeyed=65
                        labelsTotal=65  distinctKeys=36  vinesAlive=64  valuesAlive=64
AFTER en->fr->en->fr:   containersKeyed=0   labelsKeyed=65
                        vinesAlive=64  valuesAlive=64   page errors: 0
```

`.knob-container[data-i18n]` is **0** after init and stays 0 across four language
switches; all 64 vines and all 64 value readouts survive. (65 and 36 rather than
64 and 35 because the bespoke A4 Ref knob carries a hand-written
`.knob-label[data-i18n="label.a4Ref"]`.)

**Negative control — `check-i18n` is blind, `check-ui-labels` is not.** Deleting
the `data-i18n` from ONE container, restored afterwards from a namespaced
scratchpad copy (never `git checkout --`):

| plant | `check-i18n --strict-v2` | `check-ui-labels` |
|---|---|---|
| un-key `filtACutoff` (key **shared** with `filtBCutoff`) | **unchanged** — the same single pre-existing failure | `[2]` count **189 → 188** |
| un-key `eqMidFreq` (key **unique**) | 4 dead → **5 dead**, names `label.midFreq` | (same class) |

So for the **29 of 35** keys that appear on more than one knob, `check-i18n` is
**completely blind** — assertions 10, 11, 12 and 15 all unchanged. For the 6
unique ones it catches the deletion *incidentally*, as a dead key, which is not a
caption-coverage check. **`check-ui-labels` is the evidence for this whole class**,
via its `[data-i18n]` element count and its geometry sweep.

**The "65th container" is a DIVERGENCE from the brief.** There is no 65th. The
markup contains exactly **64** `div.knob-container[data-knob]`, all 64 carrying a
`data-label`; `document.querySelectorAll('div.knob-container[data-knob]').length`
is 64 at runtime. The 65th match is the literal
`<div class="knob-container" data-knob="...">` **inside the source comment at
index.html:1658**, which `grep -o` counts and a browser does not.

---

## 5. Divergences from the brief's structural claims

1. **CORRECTION TO BRIEF §2: THE 65TH KNOB CONTAINER DOES NOT EXIST.** The brief
   states "**65 of them**" and asks which container carries no `data-label` and
   what it renders. The answer is that there is no such container. Measured
   three ways, all agreeing:
   - `grep -o '<div class="knob-container"[^>]*>' index.html | wc -l` → **65**
   - `grep -c 'data-label=' index.html` → **64**
   - the one match without a `data-label` is
     `<div class="knob-container" data-knob="...">` — **the literal inside the
     source comment at `index.html:1658**, which `grep -o` counts and a browser
     does not.
   - `document.querySelectorAll('div.knob-container[data-knob]').length` at
     runtime → **64**, in every state, before and after this change.

   So the count is **64 containers, 64 `data-label` attributes, 64 keyed, 35
   distinct strings, 35 keys**. Nothing renders unlabelled and nothing was
   missed. The 65th `.knob-label[data-i18n]` on the page is the bespoke A4 Ref
   knob's hand-written span, which is a different element in different markup.
2. **§2 "35 distinct strings"** — correct, 35.
3. **§6 "8 native `title=` in markup, 6 rendered"** — the static parse
   `check-i18n` assertion 11 actually walks finds **6**, not 8. `scanHtml` does
   not parse inside a `<script>`, so the two
   `title="${interval.toFixed(1)}¢"` attributes in the injected templates are
   invisible to it. And **"6 rendered" is badly low**: the page renders 6 at load,
   **127** once the Matrix view is opened and **248** after Rotation. Every gate
   and the boot report saw only 6. Both template titles are now deleted; the
   rendered count is **0** in every state.
4. **§8 "the two `${interval}` titles are visible to assertion 11"** — they are
   not, by either route: not by the markup scan (inside a `<script>`), and not by
   assertion 12 (the accumulator shape, `html += …; container.innerHTML = html`,
   whose RHS carries no literal — a blindness already documented in
   `markupKeyRefs`'s own comment as deliberately not fixed).
5. **§13 "rebuild the caption-fit probe"** — rebuilt, and **the O-Formant version
   of it is vacuous on this page**. O-Formant's `.knob-label` is a fixed 55 px
   box, so painted-text-vs-content-box is the right comparison. O-Prism's
   `.knob-container` SHRINK-WRAPS, so a caption can never be wider than the box
   that grew to hold it: my first run reported 0 with a planted **38-character**
   caption in place. Re-pointed at the **design column** (`.knob-visual`, 52 px)
   it works — see §9.
6. **§10 "the plan says 173; the plan has been wrong about every number"** — on
   this one the plan was right. 173, measured.
7. **§12 "`tests/` holds only `ui-stub/generic-overrides.json`, so step 10 is a
   no-op"** — confirmed. **There are no gate scripts under `plugins/O-Prism/tests/`
   and none were run there.** I added `tests/i18n-states.json` (data for the
   repo-level gate, not a gate) and extended `generic-overrides.json`.
8. **`BUG-tuning-tab-cutoff.md`** (v0.10.0) describes a Tuning tab that renders
   nothing but the viz buttons. **The defect is not present.** The tuning tab
   renders its interval list, pitch circle, viz toggle and 210 px controls panel
   in the harness at the shipping 1200x800 frame, in every state I drove, in both
   languages. `#tuning-tab` no longer carries `position: relative`. Nothing in
   this commit touches it and I did not attempt to close the file.

---

## 6. Geometry

**EN → FR, non-label elements, fixed 1200x800 frame, 23 states:**

| | moved |
|---|---|
| first full pass (before geometry work) | **24 `[7]` failures**, 38 elements in the widest state; plus 3 `[8b]` failures |
| **shipped** | **0** `[7]`, **0** `[8]`, **0** `[8b]`, 0 `[4]`, 0 `[5]`, 0 `[6]` |

**Both settle times.** `check-ui-labels` measures at 180 ms. I took independent
16-state snapshots at 180 ms **and** 1.7 s in both languages and at HEAD:

```
en@180 vs en@1700        moved=0  appeared=0  vanished=0
fr@180 vs fr@1700        moved=0  appeared=0  vanished=0
HEAD en@180 vs en@1700   moved=0  appeared=0  vanished=0
```

The page is pixel-identical at the two settle times, so every 180 ms number here
holds at 1.7 s by construction and no CSS transition is in flight behind it.

**ENGLISH before → after (v1.20.0 → v1.21.0), 180 ms and 1.7 s, identical:**

```
8 elements move, in every one of 16 states, and nothing else:
  .preset-browser and its 6 children   dx=-77.5  dy=0  dw=0  dh=0
  .subtitle                            dx=-155.0 dy=0  dw=0  dh=0
```

One cause: the settings gear is a fourth child of a `justify-content:
space-between` header, so the free space now splits three ways instead of two.
**No English change outside the header bar, no size change anywhere, no vertical
movement.** New elements: `.settings-wrap`, `#gear-btn`, and the four LFO header
`<span>`s from the §5 splits.

### Every pin, with both `dx` and `dw`, and its negative control

Each pin was reverted **alone** and `check-ui-labels` re-run in full.

| Pin | sized to | the delta it holds | control: failures when removed |
|---|---|---|---|
| `.settings-label` 59.77px | English | `Langue` is 14.97 px narrower; the popover is `right:0`, so its left edge moved | **1** |
| `.hn-label` 64.45px | English | `Intervals:` 64.45 → `Interv. :` 50.70 | **7** |
| `.tonic-label` 27.80px | English | `Tonic:` 27.80 → `Ton. :` 23.63; `.tonic-selector` is `space-between` | **13** |
| `#lbl-filt-routing` 82.08px | English | 82.08 → 73.73; the group sits between two `flex:1` sections, so 6.8 px redistributes 3.4 px each way | **2** |
| `.hn-span` 26.03px | **French** | `Span` 24.50 → `Écart` 26.03 | **7** |
| `.knob-container[data-knob="delayFeedback"]` + its `.knob` 54.25px | English | `dx=0.6 dw=-1.2` — **`dx` alone would have called this decoration** | **2** |
| `.knob-container[data-knob="oscA/BWarpAmt"]` + `.knob` 55.16px | English | `dx=1.6 dw=-3.2` — same shape | **2** |
| `.wt-label` 77.41px | English | `Harmonics` 77.41 → `Harmon.` 60.86; the four bin buttons sit to its right | **6** |
| 7 × `#wt-op-*` border-box widths | English | the seven ops captions net **+38.3 px**, moving 3 separators and undo/redo | **6** |

**One pin was measured as DECORATION and removed:** `.subtitle { width: 202.78px }`.
Reverted alone, `check-ui-labels` stayed at **0 failures** — 1.31 px of slack
splits across two `space-between` gaps and lands under tolerance. The constraint
now lives as a comment on `label.subtitle` in `i18n.js`, where a reviewer
changing the French will read it.

**A pin trap worth recording.** My first ops-bar pins were sized to the **painted
text** width. `* { box-sizing: border-box }` is set globally, so a 93.23 px pin on
a button with 24 px of padding gives a 69 px content box and **`Normalize Global`
wrapped onto two lines in ENGLISH**. Measured, not reasoned: the pins are
border-box (`text + 26`).

**French getting SHORTER was the dominant failure mode here, not longer.**
`Réinj.` −19.75, `Étir.` −16.83, `Langue` −14.97, `Bibliothèque` −9.34,
`Effets` −9.20, `Normaliser tout` −8.17, `Touches` −8.04, `Qté déf.` −8.00. Five
of the nine load-bearing pins exist to stop a French caption from **shrinking**
its box.

### The caption-fit probe (§13) and the number nobody had for this page

Rebuilt, re-pointed at the design column (see §5.5), and proven with a planted
**38-character** French caption:

```
CLEAN (fr):  1 caption exceeds its 52 px column
   label.feedback "Réaction" tw=53.09 column=52 over=1.09
CLEAN (en):  1
   label.feedback "Feedback" tw=54.25 column=52 over=2.25
PLANTED:     2
   label.midFreq "Fréquence médiane de la bande centrale" tw=243.42 over=191.42
```

The one live finding is symmetric: English `Feedback` is 2.25 px over its own
column and French `Réaction` 1.09 px over, and that knob is width-pinned, so
French overhangs **less** than English does.

**And the shrink-to-fit hole does not exist on this page.** Planting the same
38-character caption in the **source** and running the real gate:
`check-ui-labels` reports **2 failures, both `[7]`**. Because `.knob-label` has no
fixed width, an oversized caption PUSHES rather than silently overflowing, and
`[7]` is the correct catcher. The O-Formant hole is a property of a fixed-width
caption box; O-Prism has none.

### A ROTATED SVG'S BOUNDING BOX IS 41% WIDER THAN THE KNOB — a general fact about this knob family

State this as a property of the vine-arc knob, not as O-Prism's French problem.
It holds for every plugin in the suite that draws this knob, in both languages,
before any localization exists.

```
.knob-visual        52 x 52 px     the design column
.knob-visual svg    transform: rotate(-135deg)
getBoundingClientRect(svg)  ->  73.54 x 73.54 px      (52 x sqrt(2))
                                +10.77 px on EVERY side
```

`getBoundingClientRect` on a CSS-transformed element returns the **axis-aligned
box of the transformed shape**, so a 52 px square rotated 45° reports 73.5 px.
The visible artwork — a circle of r=22 inside the 52 px viewBox — has not moved
by a pixel. Only the measured rectangle has.

Consequences, in order of how much they cost:

1. **Adjacent knob columns are 4 px apart** (`.knob-container { margin: 2px 2px }`),
   so each neighbour's reported box reaches **10.77 − 4 = 6.77 px INTO your
   column** from each side. A `.knob-label` is centred in 52 px, so the widest
   caption that clears both neighbours is `52 − 2 × 6.77` ≈ **38.45 px**, not
   52 px. **Every `.knob-label` in this repo sits inside a column 13.5 px
   narrower than it looks.**
2. **Against the neighbour's `.knob-track` circle** (r=22, so a 44 px square
   rotated to 62.2 px, overhang 5.1 px) the cap is **49.8 px** — tighter still
   for the innermost pair, and the one that decided `Coarse`.
3. `check-ui-labels` assertion **`[8b]`** compares rectangles and has no z. It
   therefore reports a caption crossing 38.45 px as intersecting the neighbour's
   svg, which is **true of the boxes and false of the pixels**. It is over-strict
   here, never a false pass, and it is why 13 knobs went red before the captions
   were sized.
4. It is also why **`dx` alone is useless on this family**: a caption that grows
   inside a centred column moves its siblings without widening anything, so the
   signal is in `dw`.

**The honest fix is at source, and I did not make it.** Rotating an inner
`<g transform="rotate(-135 26 26)">` instead of the `<svg>` element is
rendering-identical and makes the box 52 px again, which would give every knob
caption on every plugin its full column back and retire the `[8b]` false
positives. It is a change to working visual code across three viewBox variants
(52 / 64 / 44) plus three hand-written knobs, for a measurement artefact, and
nobody asked for it. Recorded as a NEEDS-A-DECISION item rather than taken.

13 knobs crossed it. Six French captions were shortened for that reason **and
that reason only** — `Niv.`, `Larg.`, `Chute`, `Vit.`, `Amor.`, `Méd.` — plus
`Gross.` for the 49.8 px variant. They are recorded as layout constraints in
`i18n.js` so a native speaker does not read them as preferred vocabulary.

I did **not** change the SVG rotation to fix the bounding box at source (rotating
an inner `<g>` instead of the element would make the box honest and is
rendering-identical). It is a change to working visual code across three
viewBox variants plus three hand-written knobs, for a measurement artefact, and
nobody asked for it. Recorded as a NEEDS-A-DECISION item.

---

## 7. `I18N_EXEMPT` — 105 entries, and the D-01 arm each rests on

| group | n | arm | scope | why the scope |
|---|---|---|---|---|
| parameter dropdown options (`Off`, `Sync`, `Bend`, `FM`, `Window`, `-1..-4 Oct`, `Post/Pre-Filter`, `White/Pink/Brown/Digital/Vinyl`, `Legato/Always`, `LP12..Notch`, `Serial/Parallel`, `Normal/PingPong`, `SoftClip..Fold`) | 33 | **arm 1** | `.param-select` | `Off`, `Sync`, `Wind` and `Digital` are ALSO live keys elsewhere on the page |
| factory wavetable names (`Saw`…`Filtered Noise`) | 28 | not arm 1 — see below | `.param-select` | `Sine`, `Square`, `Triangle`, `Saw` and `Harmonic Series` are byte-identical to arm-1 siblings or to a live key |
| modulation source/destination names | 36 | **arm 1** | `#mod-matrix-rows` | `Osc Mix` is also the footer caption `label.oscMix`, which IS keyed |
| `#toggle-delaySync` faces `On` / `Off` | 2 | **arm 1** | `#toggle-delaySync` | both collide with live keys |
| `— Init —` | 1 | **D-02** | `#preset-current-name` | |
| `12-TET Standard` | 1 | **arm 3** | `#scale-name-display` | |
| `O-PRISM`, `Custom`, `English`, `Français` | 4 | product name / arm 1+3 / endonyms | unscoped | genuinely unambiguous |

**The 28 wavetable names are NOT arm 1, and that matters.** `oscATable` is an
`AudioParameterInt` (0..27, host text `"0"`..`"27"`, confirmed in the runtime
dump) — there is no choice option to be byte-identical to. They are exempt on two
other grounds, either sufficient: they are a hand-mirrored copy of
`WavetableFactory::getTableInfoList()` (WavetableFactory.cpp:93-130), a C++-owned
catalogue of factory content names; and four of the 28 are byte-identical to
`subShape` / `lfoNShape` options that ARE arm 1, so translating the osc list would
make the same word French in one dropdown and English in the next.

**Two arm-1 overrules, stated plainly:**

1. **The five BYPASS buttons are KEYED, not exempt.** Their faces are `ON`/`OFF`
   in the markup's own upper case, which is byte-identical to nothing —
   `delayBypass` reports `Off`/`On`. They become `MARCHE`/`ARRÊT`.
2. **The delay Sync button is EXEMPT.** Its `On`/`Off` **is** byte-identical to
   `delaySync`'s host text. I initially keyed it, on the argument that a bool's
   Off/On is JUCE boilerplate rather than an authored choice name. Geometry made
   the argument moot: that button and its caption share a 44.44 px cell in the
   middle of the delay row, and `Arrêt`/`Marche` push every control to their right
   by 16 px. Arm 1 covers it, so I took arm 1 rather than a pin. **Reversing a
   decision for a layout reason is worth a reviewer's eye.**

`label.sync` (the caption above that button) ships as `Sync` with
`sameAsEn: true` for the same 16 px; the LFO Sync BUTTON keeps `Synchro`, where a
281 px section header gives it room. **The same English word gets two answers on
one page, decided by geometry** — the O-Bass `OUT`/`OUTPUT` precedent.

---

## 8. UI states DRIVEN, and states NOT driven

**Driven (23, via `tests/i18n-states.json`, both languages, every one measured):**
default (synth), mod matrix, tuning/circle, tuning/polar, tuning/matrix,
tuning/truekeys, tuning/rotation, tuning back to circle, **held notes**
(`updateHeldNotes([60,64,67])` — the held-notes bar and the True Keys grid),
tuning library **expanded and populated**, scale generator **expanded** in all
three variants (EDO / Harmonic / Rank-2 — the three different input forms),
effects, wavetable, wavetable **Save dialog**, **user-wavetable manager dialog
populated**, **settings popover**, **preset Save dialog**, **preset menu**, all
four LFOs **switched to Sync** (the division dropdown replacing the rate knob),
all five effects **bypassed**.

`198 of 201 [data-i18n] elements were VISIBLE in at least one state.`

**NOT driven, and therefore NOT verified:**
- The two `.wt-drop-overlay` "Drop WAV" captions — they need a real drag-over.
- The `Import WAV...` / `Manage...` `<option>`s and the six `#library-filter`
  `<option>`s: their TEXT is confirmed to change language (assertion 2's sweep
  reads them), but **a closed native select's options have no box, so their
  geometry is unknown** — the O-Bells precedent.
- The `.SCL` / `.KBM` file dialogs and `Export HTML` — they call into C++
  `FileChooser`s.
- The user-wavetable **empty** state. `label.noUserWavetables` renders only when
  `getUserWavetableList` returns `[]`, and I gave the stub a populated list
  instead (the K4 lesson: a harness that cannot reach a state certifies nothing).
  Its French is confirmed present in the table; its geometry is unmeasured.

**Four `generic-overrides.json` natives were added, and each one bought a real
state that was previously unreachable:** `getEmbeddedTuningList` (the whole
Tuning Library panel rendered nothing), `getUserWavetableList` (no user rows, no
`Manage...`, no `Delete`), `startWavetableEditor` + `getAllEditorFrameWaveforms`
(the frame strip and harmonic editor painted nothing — **this is what made the
`fillText` probe's own negative control fail to fire the first time**).

---

## 9. The `fillText` probe and its negative control

Two sites, both confirmed **D-01 arm 2, exempt, no `I18N` entries needed** — and
confirmed by recording, not by reading:

```
en#1   calls=19  distinct=11  ["0","1","2","3","4","5","6","7","8","9","10"]
fr#1   calls=19  distinct=11  ["0","1","2","3","4","5","6","7","8","9","10"]
en#2   calls=19  distinct=11  ["0","1","2","3","4","5","6","7","8","9","10"]
en-only: []   fr-only: []
```

19 calls = 11 pitch-circle degree labels + 8 wavetable frame indices. Every
painted string is a number, identical en→fr→en.

**Negative control, both halves:**
- Restoring a hardcoded English `ctx.fillText('PLANTED ENGLISH FRAME', …)`:
  the probe **reports it** (`distinct=12`).
- The same plant in the shipped source leaves `check-i18n --strict-v2` at
  **exactly the same single pre-existing failure** — the gate is blind to canvas
  text, as documented in K4.

**The first version of this control FAILED TO FIRE**, and the reason is worth
carrying: the frame-strip painter never ran, because `drawFrameStrip()` is only
called from `getAllEditorFrameWaveforms`'s own `.then()`, and the generic stub
returned `null`. A negative control that does not fire is not a passing control.

---

## 10. Found and deliberately NOT fixed

1. **`window.confirm` / `window.prompt`** — not applicable here; O-Prism already
   routes both through in-DOM dialogs (CR-10). Nothing to do.
2. **The generated scale NAME stays English.** `Harmonics 8-16`,
   `Rank-2 (696.6¢, 12 notes)` and `{n}-EDO` are composed in JS and passed to
   C++ `applyGeneratedScale(intervalsJson, scaleName)`, which **persists** them —
   the string is the stored scale identifier, returned by `getScaleName()` and
   written into the session. Localizing it would make a French-generated scale
   name differ from an English one in saved state. A correct fix separates a
   display name from the stored name, which is a functional change. `notes` is the
   only English word a French user sees there.
3. **Six `<optgroup label="...">` strings stay English** — `Analog`, `Digital`,
   `Formant`, `Spectral`, `Organic`, `User`. No gate scans `optgroup label`, and
   canon v2 has no attribute that can key it (`data-i18n-aria/-placeholder/-alt`
   only). Five of the six mirror `WavetableFactory`'s own category column, so they
   sit with the 28 exempt table names anyway; `User` does not.
4. **`<html lang="en">` does not follow the language.** Setting it belongs inside
   `applyI18n`, which is the byte-compared canon block — a repo-wide change to all
   43 plugins, not a local edit.
5. **The rotated-SVG bounding box** (§6) — a 73.5 px reported box on a 52 px
   knob, across the whole vine-arc knob family and every plugin that draws it,
   costing every caption 13.5 px of its column and generating `[8b]` collisions
   that exist only in the rectangles. Not fixed; it is working visual code.
6. **`BUG-tuning-tab-cutoff.md`** stays in the plugin root. The defect it
   describes does not reproduce (§5.8), but closing a v0.10.0 bug file is not a
   localization dispatch's call.

---

## 11. NEEDS A HUMAN DECISION

1. **The gate defect in §1**, and the fact that **O-Lyrica v2.4.1 already shipped
   under it**. Worth re-running `check-i18n` on O-Lyrica once the fix lands, to
   see whether it was reporting dead keys there too.
2. **Twelve French captions are abbreviations chosen by geometry, not by
   vocabulary**: `Niv.`, `Larg.`, `Chute`, `Vit.`, `Amor.`, `Méd.`, `Gross.`,
   `Fq. méd`, `Étir.`, `Harmon.`, `Norm.`, `Enr.`. Each is recorded with its
   measurement in `i18n.js`. A native speaker may prefer a layout change to any of
   them.
3. **`Mix` and `Sync` ship as `sameAsEn`** where `Dosage` and `Synchro` are the
   better French, both purely for width (21 px and 16 px respectively).
4. **`OBF 1..4` for `LFO 1..4`.** `OBF` (*oscillateur basse fréquence*) is correct
   French but far less recognised than the borrowed `LFO`; the widths are within
   0.52 px either way, so this is a vocabulary decision with no layout cost.
5. **The delay Sync toggle exemption** (§7) — an arm-1 verdict I first overruled
   and then took, for a geometry reason.
6. **English moves 8 elements in the header bar** (§6), on every state, purely
   because the settings gear is a fourth `space-between` child.
7. **The two `${interval}¢` hover titles are gone**, and with them a value a user
   could previously read on the interval-matrix and rotation cells. In-contract
   (§4), but it removes information.

---

## 12. What is NOT verified

- **No human has seen the French UI.** Checkpoint 5 outstanding.
- **All 155 French strings are machine drafts**, every one `reviewed: false`.
- **No DAW session was saved and reopened.** Checkpoint 4 is reasoned from the
  `isVoid()` guard, from `getUiLanguage`/`setUiLanguage` being present in the
  built binary, and from `auval` — not executed.
- **No DAW test at all.** `auval -v aumu OuPr OuDv` PASSES; the headless harness
  and `auval` are the whole of it.
- **The Standalone `.app` is stale** — `build-and-install.sh` builds VST3 + AU only.
- **Windows / WebView2 font metrics.** The standing hardware-blocked deferral,
  and it bites harder here than anywhere: **nine geometry pins carry
  two-decimal-place pixel numbers measured on macOS Garamond**. Tightest margins
  shipped: `Satur.` **0.71 px**, `Prof A` **0.78 px**, `Prof B` **0.86 px**,
  `Taille` **0.90 px**, `Tenue` **1.70 px** — all against the rotated-SVG budget.
  A wider Windows face eats those first.
- **The `optgroup` labels, the drag overlays, the file dialogs and the empty
  user-wavetable state** were never rendered (§8).
- **`plugins/O-Prism/tests/` contains no gate scripts**, so step 10's "run every
  gate in that plugin's own `tests/`" was a **no-op**. Nothing was run there.

---

## 13. Gate results, verbatim — re-run from a clean state after `f70ea7a0`

```
node scripts/check-i18n.js --plugin O-Prism --strict-v2
    ALL CHECKS PASS — 1 localized plugin(s)
    [6] --strict-v2: the plugin is on canon v2 — it is on v2
    155 / 155 entries unreviewed  (0 tooltip, 155 label)

node scripts/check-i18n.js --strict-v2              (repo-wide)
    ALL CHECKS PASS — 43 localized plugin(s)
    canon v2  43   canon v1  0   canon none  0
    TOTAL unreviewed French across the repo: 3202

node scripts/check-ui-labels.js --plugin O-Prism
    == ALL CHECKS PASSED ==      rc=0, 0 FAIL
    189 [data-i18n] elements at load; 198 of 201 VISIBLE in at least one
    of 23 states, measured in both languages.

node scripts/boot-all-uis.js
    clean: 43 / 43   warn: 0   failed: 0
    O-Prism  1200x800  stub=generic  text=940  aria=7  title=0  i18n=189
    repo-wide: 3789 text-bearing, 771 aria-label, native title= 0  (was 6)

(no gate scripts exist under plugins/O-Prism/tests/ — step 10 was a no-op there)

./scripts/build-and-install.sh O-Prism      (under the build mutex)
    rc=0, VST3 + AU installed, 57s
    auval -v aumu OuPr OuDv  ->  AU VALIDATION SUCCEEDED
    CFBundleShortVersionString = 1.21.0
    binary carries the embedded table (a French string only i18n.js holds),
    the "/js/i18n.js" getResource url, and both language native functions.

git show --stat e4796486
    10 files changed, 1257 insertions(+), 206 deletions(-)
    parent e4796486^ = f70ea7a0 (the gate fix)
```

Build mutex held around the cmake configure, the param-dump build,
`build-and-install.sh` and both `auval` runs; released immediately after each.
No `screencapture` was run. **Nothing was tagged and nothing was pushed** — three
commits sit unpushed ahead of `origin/main`. `PLUGINS.md` and `scripts/` were not
touched by me. The four `.planning/` extractor outputs and this report remain
untracked and uncommitted. Every scratch file is under `scratchpad/O-Prism/`.
