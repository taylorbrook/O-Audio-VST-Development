# Wave 4c — deferred items and structural findings

Carry-forward for **wave 4d**. Written in the shape waves 4a and 4b used:
numbered structural findings first (each one costs a later executor time or a
wrong answer if not read), then the deferred items with their evidence.

---

## Structural findings

### N1. `measure-ui`'s `line-height-normal` screen CANNOT reach 0 on most pages, and the wave-4c plan's verify asked it to

The screen filters on `x.han`, and `han` is computed over the node's own text
**plus `data-tip`, `data-tip-title` and `aria-label`** (`measure-ui.js:286–303`).
So a value readout whose own text is `"50%"` but which carries a Chinese
`aria-label` is a finding *forever*: it has no Han to render, its box is
identical on both arms, and no pin can change that.

The shipped, wave-4b-passed **O-Tremolo reads 3 on this screen today** — two
chevrons and the exempt preset name. That is the precedent, and it is the right
one.

**The load-bearing criterion is `check-ui-labels` reporting 0 moved elements**,
which is a real geometry assertion. This wave pinned every leaf that MOVES
(`enH != zhH` or `enW != zhW`) and left every evidenced non-mover, recording the
residual per plugin: O-Bells 19, O-SpectralShaper 11, O-Bassoon 3,
O-TextureForge 2, O-Detune 0, O-Gain 0. On O-Bells three of the residuals are
`div.tab` at a measured `u = 2.9231`, where a pin would replace flex centring
with a 38 px line box — decoration that is also a risk.

**Wave 4d should state the criterion as "0 moved on `check-ui-labels`, plus every
`line-height-normal` residual named with its `enH == zhH`"**, not "0 findings on
every screen".

### N2. M2 generalises completely — 4 of 4 gate files had a separate WALK site, and one had NO assertion at all

| plugin | assertion site | walk site(s) | census could see the assertion? |
|---|---|---|---|
| O-Detune | **none** | one array literal, L467 | **no** |
| O-Bells | `LANGUAGES.join(',')` L314 | two calls, L689 / L800 | yes |
| O-Bassoon | `LANGUAGES.join(',')` L209 | two calls, L383 / L412 | yes |
| O-TextureForge | `LANGUAGES.join(',')` L290 | two calls, L501 / L512 | yes |

**Four for four.** No fifth enumeration form appeared. All four now derive at both
sites and all four had the derive-or-abort control **fired against the walk** —
each exits non-zero on a planted empty export, and each revert was byte-identical
by sha256.

The shape wave 4d should expect: a repo-wide census keyed on a *name* finds the
assertion and never the walk, because **a call site spells no list to grep for**.
The only thing that finds a walk is reading the file.

### N3. The installed-family list — a property of THIS MACHINE, not of the plugins

Measured with `system_profiler SPFontsDataType | grep -c "Family: X$"`:

| family | matches | | family | matches |
|---|---|---|---|---|
| Georgia | 4 | | Garamond | **0** |
| Times New Roman | 4 | | EB Garamond | **0** |
| Arial | 4 | | Adobe Garamond Pro | **0** |
| Menlo | 4 | | Microsoft YaHei | **0** |
| Courier New | 4 | | | |
| PingFang SC | 6 | | Helvetica Neue | 14 |
| Songti SC | 4 | | | |

Wave 4b found 69 bare-generic declarations because four of its plugins stacked
only Garamond variants. Wave 4c found **3**, because these plugins mostly name
Georgia or Times New Roman as well. **Re-derive this table at the start of wave
4d.** The bare-generic arithmetic changes completely between waves and it is the
machine that changes it, not the code.

### N4. A `termNote` is ENTRY-SCOPED, so a term that recurs as both a caption and a tooltip title needs TWO

O-Gain's `measure` diverges from the glossary root under a `termNote`. The note on
`label.measure` did **not** cover the tooltip title `measure-mode`, and rule Z5
reported the title with the caption already clear. Both entries now carry their
own copy.

This is the same scope trap the O-Detune French table documents from the other
direction (`sameAsEn` is entry-scoped and would disarm assertion 4 for the body
too). **Any wave-4d divergence needs one note per entry, not one per term.**

### N5. The plan's own `VERSION` measurement was wrong, and R9 saved it

The wave-4c plan states as a measurement that none of its six plugins quotes the
CMakeLists version. **`plugins/O-Bassoon/CMakeLists.txt:14` reads `VERSION
"1.3.1"`.** R9's instruction — read the *value* with a pattern that tolerates
quotes, never grep for a literal — is what made the bump land anyway.

The rule survived its own plan being wrong about it, which is the argument for
keeping R9 even when a wave "knows" the answer.

### N6. Two renderings of near-identical English, one wave, two plugins — and Z5 is BLIND to it

- O-TextureForge: `placeholder.webglUnavailable` "WebGL unavailable" → **`WebGL 不可用`**
- O-SpectralShaper: `canvas.webglUnsupported` "WebGL not supported" → **`不支持 WebGL`**

Neither English string is a `TERMS` key, so **rule Z5 cannot see the divergence**
and no gate in this repo can. Both round-tripped correctly in isolation; the
problem only exists when the two batches are read side by side.

This is exactly the class of drift the glossary exists to prevent. It is
**reported, not settled** — editing a shared term list is a repo-wide change with
its own budget (wave 4b's D2 records the same discipline for three roots that
failed a reverse read).

### N7. The "no plugin name in the batch" blinding control CANNOT pass on a plugin whose own copy names it

O-Gain's `ms-enc` body reads *"Use with a second **O-Gain** set to DEC after
processing."* The product name is the string being translated; withholding it
means withholding the row.

The control **fired** (1 hit) and was adjudicated rather than waved through. What
actually covers the trust boundary is `--allowed-tools ""` plus the explicit
prohibition in the prompt, both of which were in force for every dispatch in this
wave. **Wave 4d should expect a non-zero count on this control for any
self-naming plugin and should read the hit rather than treat the count as a
pass/fail.**

### N8. A body naming a KEYED option in English is a defect the reverse read finds and no gate can

Two instances, in opposite directions, both caught by the blind read:

- O-SpectralShaper's `drawMode` body spelled "Freehand" and "Node" — both keyed
  labels that render 手绘 and 节点 on the page. The body told a Chinese reader to
  press two buttons that are not on screen.
- O-Bells' `velocityCurve` body correctly keeps "Linear" verbatim (it is
  `I18N_EXEMPT`, byte-identical to a choice string) while localizing 指数 and
  对数 (which are keyed).

**The rule:** an option word visible on the page stays verbatim **only when it is
exempt**, because an exempt option reads the same in every language. A keyed one
must be named by its localized caption. Check every body that names a control.

### N9. `check-ui-labels` failures in wave 4c were overwhelmingly SHRINKS, not clips

Every geometry pin in this wave that was not a `line-height` pin is a **floor**,
and every one exists because the Chinese is smaller:

| plugin | node | en | fr | zh | consequence |
|---|---|---|---|---|---|
| O-Bells | `.footer-gain-label` | 26.50 | 26.50 | **20.41** | 44 elements moved, all 14 states |
| O-Bells | `.rotation-table th[data-i18n]` | 30.56 | 30.56 | **27.36** | + broke between its two characters |
| O-Bassoon | `.about-blurb` | 3 lines | 3 lines | **2 lines** | card shrank 12.4 px, credit row rose 15.4 |
| O-Bassoon | `.about-version [data-i18n]` | 57.72 | 57.72 | **24.89** | version number slid 16.4 px |
| O-TextureForge | `.midi-mode-label` | 55.17 | 55.17 | **44.94** | select slid 10.23 px |
| O-TextureForge | `#file-size-msg` | 270.00 / 2 lines | 270.00 | **255.45 / 1 line** | dialog 14.5 px narrower, shifted 7.3 |

All are `min-width` / `min-height`, **never fixed**: a floor must not cap a
language that needs a wider sentence or a third line. Wave 4d should budget for
floors, not for clips.

### N10. A `line-height` pin has to be derived PER LINE when the element already wraps

O-Gain's `.utility-btn` measured a 22.00 px box at a 10 px font. That is not a
2.2 line-height — seven buttons share a 334 px flex row, so their captions
already wrap in English and the box is **two** line boxes. The pin is
`22.00 / 2 lines = 11.00 / 10px = 1.1`.

A naive box-over-font-size rule produces 2.2, which makes a single line 22 px
tall and a two-line case 44. The wave-4b precedent (O-Tremolo's `.toggle-button`)
records the same trap. **Check the line count before dividing.**

### N11. A class rendered at two sizes needs two pins, and needs two SELECTORS to hang them on

O-TextureForge's `.knob-label` renders at 9 px in `.knob-row` and 8 px in
`.bottom-knob-group`. 10/9 and 9/8 are different ratios, so one unitless number
cannot serve both. The stylesheet happened to scope the two by parent already; a
class rendered at two sizes behind **one** selector would have needed a new one
invented.

---

## Deferred items

### D1. The shared `scala-tuning-engine` tuning panel — FIVE plugins, no i18n hooks in ANY language

`modules/tuning/scala-tuning-engine/js/tuning-panel.js` — **1041 lines holding 0
`data-i18n`, 0 `applyI18n`, 0 occurrences of `'fr'`.** Measured, not assumed.

**Blast radius — five consumers, each embedding it in its own CMakeLists:**
`plugins/O-Bassoon`, `plugins/O-Bowed`, `plugins/O-Contrabass`,
`plugins/O-Reed`, `plugins/O-Wind`. Any edit is a five-plugin change with five
builds and five `auval`s.

**Why it is deferred and not simply missing:** O-Bassoon's own language
hover-help **already discloses it in all three languages** — the Tuning tab stays
in English because its panel comes from a shared module that is not part of this
plugin. That sentence was the French precedent, it was deliberately KEPT through
this wave's enumeration deletions, and the blind reader read it back intact. No
user-visible claim goes stale by leaving the module alone.

Note also that O-Bassoon's `tests/i18n-states.json` never drives the Tuning tab,
so **no gate or measurement in this repo would report it either way**.

**Contrast with O-Bells**, which carries its OWN 1075-line copy with 45
`data-i18n` hooks and 8 `applyI18n` calls — its tuning panel *was* in reach and
its captions are part of this wave's 255 rows.

### D2. O-Bells' two late tip bindings — pinned by the gate, kept as the census control

`#ref-pitch-knob` and `#octave-stretch`. `tests/ui_tip_render_check.js:462` reads
`const EXPECTED_LATE = ['#ref-pitch-knob', '#octave-stretch'];` and assertion
`[1b]` requires the warning set to equal it exactly, with the surrounding text
documenting the behaviour as designed — they bind on the tuning panel's own
`window.__reapplyI18n()`.

**Left untouched, deliberately.** They are the suite's `boot-all-uis` census
control: every `--strict-tips` run in this wave reported exactly **2 late, 0
dead**, and a run reporting 0 would mean the census had gone blind rather than
that anything had been fixed. No consequence was observed — no tuning-panel
anchor carried a missing Chinese tip at settle.

### D3. The glossary divergence report (N6) — `WebGL 不可用` vs `不支持 WebGL`

For the glossary owners. Neither English string is a `TERMS` key. Settling one
rendering would make Z5 able to see the second, which it currently cannot.

### D4. Re-inherited from wave 4b (D1–D7, minus D7 which is closed)

- **D1** O-Chorus's inert CJK tail — its own plugin's pass.
- **D2** the three glossary roots that failed a reverse read — reported to the
  glossary owners; editing a settled root is repo-wide.
- **D3** (as re-numbered in 4b) O-IntonationPad has no tip gate. **O-Gain has
  none either**, confirmed this wave, and none was written: a render gate is a
  separate pass with its own budget, and a gate invented mid-localization is a
  gate nobody has calibrated.
- **D5** the `Z6` budget backfill — 3 of 552 glossary terms carry a measured
  budget; **549 are UNBUDGETED and Z6 is inert on them**, printed on every lint
  run. Unchanged by this wave.
- **D7** the measurement harness — **CLOSED** by 260905-izp
  (`scripts/measure-ui.js`), and used throughout this wave.

### D5. `reviewed: 'native'` — open on all 2440 rows, a blocker for nothing

This project has no native Chinese reader. The ship bar is `'bt'`: an independent
back-translation the developer read against the English. `i18n-zh-lint` rule R1
prints the level on every run, and all six CHANGELOGs state it. Unchanged.

---

## Explicitly for wave 4d

1. **Do not run `measure-ui --report all` before the table lands.** Three of its
   four screens filter on `x.han`, so on a pre-localization plugin every one
   reads 0 and the reading is a vacuum. State the two language-independent
   proxies up front instead: the computed-`ff` census over `lang==='en' && vis`
   rows (which finds form-4 nodes no grep can see), and the
   `lh==='normal' && kids===0 && own.trim()` count (the upper bound on the
   line-height worklist). **M1 holds and was confirmed twice** — once at wave-4c
   planning time on all six, and once on O-TextureForge, where the
   `undeclared-font` screen read 0 because the state file never opened the
   overlay that carried the defect.
2. **Expect the walk and the assertion to be separate sites (N2, 4/4).** Repair
   both, and fire the derive-or-abort control against the **walk**.
3. **Re-derive the installed-family table (N3).** It is a property of the machine.
4. **State the `line-height-normal` criterion as N1 gives it**, not as "0".
5. **Read every body that names a control (N8).** Exempt option words stay
   English; keyed ones take their localized caption.
6. **Budget for FLOORS, not clips (N9).** Every non-`line-height` pin in this wave
   was a floor because the Chinese was smaller.
7. **One `termNote` per ENTRY (N4)**, not per term.
8. **A CSS rule beats a bundle edit** for form-4 nodes created at runtime by a
   webpack bundle — it reaches the node just as well and keeps the committed
   artefact and its source in sync by construction.
