---
task: 2
plan: 260906-h8y
role: HEAVY
plugin: O-Lyrica
status: complete
shipped_version: "2.5.0"
plugin_type: instrument
au_triple: "aumu OLyr OuDv"
rows: 213
corpus_before: 3187
corpus_after: 3400
plugins_before: 28
plugins_after: 29
commits: [8f2e2535, a62cf6c4]
---

# Task 2 — O-Lyrica 2.5.0, the heavy plugin of wave 4e

**Shipped:** O-Lyrica **2.5.0**, type **instrument** (`IS_SYNTH TRUE`). AU triple read off
`auval -a`: **`aumu OLyr OuDv`** — registered as `Ouaricon Audio Development: O-Lyrica-dev`.
**`auval -v` NOT run** — deferred to Task 3's single cold sweep, per the plan.

**Corpus:** 3187 → **3400** rows of 5256, all at `reviewed: 'bt'`, `BELOW SHIP BAR 0`.
28 → **29** plugins carrying Chinese. 213 rows = 46 `I18N` entries × 2 (title + body) + 121 `LABELS`
— read off the emitter's own `rows` column, and the plan's 213 held exactly.

`build-and-install.sh O-Lyrica` exit 0 in 54 s; VST3 + AU installed as `O-Lyrica-dev`, both
`CFBundleShortVersionString` **2.5.0**; **no alternate-variant orphan on disk**.

---

## Preconditions — all FIRED on the tree as found, none inherited (M3 / instruction 3)

| precondition | result |
|---|---|
| `i18n-zh-lint --self-test` | **10/10** |
| `check-i18n --plugin O-Lyrica` | **exit 0, ALL CHECKS PASS** |
| `check-ui-labels --plugin O-Lyrica` | **exit 0, `== ALL CHECKS PASSED ==`** |
| **absence of a gate file confirmed, not assumed** | `plugins/O-Lyrica/tests/` holds **exactly one file**, `i18n-states.json`. No gate invented (wave 4c D4) |

---

## Gate results

Every one run in the background and polled — no foreground build, gate, `measure-ui` or blind
dispatch, per the watchdog rule.

| gate | as found | shipped | note |
|---|---|---|---|
| `i18n-zh-lint --self-test` | **10/10** | **10/10** | no zero trusted without it |
| `check-i18n --plugin O-Lyrica` | exit 0 | **exit 0** | `LANGUAGES` reads three |
| `check-i18n` repo-wide | — | **exit 0, 43 plugins** | |
| `i18n-zh-lint --plugin O-Lyrica` | — | **exit 0, 0 findings, 167 entries / 213 rows** | **0 findings on its FIRST pass**, before any correction; 1 `termNote` |
| `i18n-zh-lint` repo-wide | — | **exit 0, 0 findings across 43** | gate mode (exit 2) |
| `i18n-fr-lint` | exit 0 | **exit 0** | French untouched, and no French box moved |
| `check-ui-labels --plugin O-Lyrica` | exit 0 | **exit 0 — 0 moved on en, fr AND zh, all 15 states** | `states: default + 15 from tests/i18n-states.json` |
| `measure-ui --verbose` | — | **0 unresolved states** | every state-gated zero is measured |
| `boot-all-uis --plugin O-Lyrica --strict-tips` | — | **0 late, 0 dead** | |
| `boot-all-uis --strict-tips` repo-wide | — | **0 dead, 0 failed, 0 warn, exactly 2 late across 1 plugin** | D2 census control unchanged (O-Bells `late-tips=2`) |
| `i18n-zh-backtranslate` stage view | 3187 | **3400 of 5256, BELOW SHIP BAR 0, 29 plugins** | |
| `git tag --points-at HEAD` | 0 | **0** | no tag created |
| `git status --short -- modules/` | 0 | **0** | shared module tree untouched |

---

## measure-ui, three passes

`--report all`, at the parsed shipping frame **700 × 450** (the wave's only tight form), cumulative
over 15 states. **Identity disclosure: 3018 nodes / 1006 distinct DOM keys / 284 distinct display
ids — keyed by DOM path.** 737 visible nodes on the English arm.

| screen | as found (pre-table) | post-table, pre-font | **shipped** |
|---|---|---|---|
| `undeclared-font` | 0 — **VACUOUS** (Q11) | **177** | **0** against **181** visible Han-bearing nodes |
| `line-height-normal` | 0 — **VACUOUS** | **127** | **25**, of which **0 MOVED** |
| `wrap-count` | 0 — **VACUOUS** | **13** | **0** |
| `svg-font-attr` | 0 carriers, 0 | 0 carriers, 0 | **0 carriers, 0** — form 2 absent |
| nodes on a **naked generic** | 157 | 157 | **0** |

Q11 held exactly: the three Han-gated screens print four zeros before the table lands and a reader
who took them as a baseline would have measured nothing. The identity ratio moves 2010 → 3018 nodes
between the as-found and post-table runs purely because a third language is being walked.

**The `line-height-normal` residuals — all 25, each with its measured `enH == zhH` (N1):**

| group | n | font-size | enH | zhH | why it is a permanent residual |
|---|---|---|---|---|---|
| `#chorus*Value`, `#delay*Value`, `#reverb*Value`, `#eq*Value` | 15 | 9px | **10** | **10** | numeric readouts — `5.05 Hz`, `50%`, `0.0 dB`. **No Han to render**; they reach the screen only because their `aria-label` carries Han |
| `#tab-sound`, `#tab-techniques`, `#tab-tuning`, `#tab-effects` | 4 | 10px | **29** | **29** | the four tabs DO render Han (声音 / 演奏技法 / 调音 / 效果) inside a fixed-height `.tab` |
| `#preset-prev`, `#preset-next`, `#tonic-down`, `#tonic-up` | 4 | 12 / 8px | 24 / 14 | 24 / 14 | `◀` / `▶` glyph buttons; Han only via `data-tip` |
| `#preset-name-display` | 1 | 10px | **15** | **15** | renders `Default` — an `I18N_EXEMPT` Latin string |

N1 holds as written: the screen **cannot** reach 0 on this page, the load-bearing criterion is
`check-ui-labels` reporting 0 moved, and it does on all three arms across all fifteen states.

---

## The font work — the largest surface in the rollout, in three separable halves

**The form-4 / `inherit` PAIR (M6, instruction 6), measured not grepped: 5 declarations already
present, 39 controls still on the UA face.** The five `font-family: inherit` sites are at raw
`index.html` **L794 (`.fx-bypass-btn`), L897 (`.fx-dropdown`), L1926 (`.settings-label`),
L1941 (`.settings-popover select`), L1966 (`.settings-toggle`)**. A grep of this file reports a
plugin that has had the form-4 repair. The computed census reports one that has had it on five
controls out of forty-four. **This is Q6's controlled pair confirmed from the O-Lyrica side.**

**The census, `x.ff` grouped over `lang === 'en' && vis`, before and after — this is the M7
verification, and it is the whole point that the COUNTS are unchanged:**

| n | stack, as found | stack, shipped |
|---|---|---|
| **539** | `Garamond, "Times New Roman", serif` | `Garamond, "Times New Roman", "PingFang SC", "Microsoft YaHei", serif` |
| **157** | `monospace` — **names no family at all** | `Menlo, "Courier New", "PingFang SC", "Microsoft YaHei", monospace` |
| **39** | `Arial` — **the user-agent stylesheet** | `Arial, "PingFang SC", "Microsoft YaHei", sans-serif` |
| **1** | `Times` | `Times` — the `html` element, renders no text |
| **1** | `Georgia, serif` | `Georgia, "PingFang SC", "Microsoft YaHei", serif` |

539 / 157 / 39 / 1 / 1 before **and** after. The repair reached exactly the undeclared set and
nothing else — verified by re-running the screen and comparing per-stack populations, never by
reading the CSS.

**M7's second half, en+fr node boxes before vs after the font work: 1474 compared, 2 MOVED.** Both
are the same element on two arms — `#lang-select`, 50 → 51 px — and the cause is the **endonym**,
not the fonts: a `<select>` grows to admit a third option. It moves identically on all three arms,
`.settings-row` absorbs it in its own slack, and `check-ui-labels` reports 0 moved.

**Half 1 — the TAIL, 12 sites, no face-naming needed.** Eleven `'Garamond', 'Times New Roman',
serif` at raw L56 (`body`), L284, L511 (`select`), L1172, L1253, L1286, L1303, L1415, L1460, L1755,
L2022 (`.tooltip`), and one `'Georgia', serif` at L1870 (`.gear-btn`). Times New Roman and Georgia
are both installed, so the Latin half was already safe.

**Half 2 — the FACE-NAMING half, all SEVEN naked generics, 157 nodes.** `font-family: monospace` at
raw L1056 (`.interval-input`), L1389 (`.octave-stretch-value`), L1565 (`.matrix-cell`),
L1612 (`.tk-cents`), L1652 (`.int-cell`), L1682 (`.cents-value`), L1688 (`.deviation`).
**Thirteen times O-Marimba's two-declaration / twelve-node case**, and Q7's mechanism confirmed:
these are digit readouts with no translated string anywhere near them, so the defect is invisible
from the Chinese copy and is a geometry surface as much as a font one. Menlo first — installed, and
Chromium's own macOS default monospace, so the en/fr metrics do not move — then Courier New, then
the CJK tail before the generic.

**Half 3 — the FORM-4 half, an element-level rule (M7, instruction 7).** The 39 are **27
`<input type="range">` sliders and the 12 custom-degree toggle buttons**. Q13 is exactly right that
an id list would be wrong: `handleGeneratorTypeChange` rebuilds `#generator-inputs` three times
during the walk, the twelve degree toggles are created by a loop with an inline `cssText`, and the
sixteen EFFECTS captions are written by `setLabel` from `js/app.js`. The rule shipped is

```css
button, input, textarea { font-family: Arial, 'PingFang SC', 'Microsoft YaHei', sans-serif; }
```

**`select` is DELIBERATELY ABSENT from the selector list, and that is a departure from the shipped
shape with a measured reason.** M7's precedent is `button, input, select, textarea`. This page
already carries an element-level `select { font-family: 'Garamond', … }` rule of its own at raw
L511 — the same specificity (0,0,1) — so a later `select` here would win the cascade and pull every
dropdown on the page off its declared serif stack, moving the English and French arms. Measured:
**none of the 39 undeclared controls is a `select`.** `textarea` is included and reaches **0** nodes
today; it is a guard, and it is recorded as reaching nothing rather than counted as a repair.

**The installed-family table, re-derived on this machine (M5, instruction 5):**

| family | matches | | family | matches |
|---|---|---|---|---|
| Georgia | 4 | | **Garamond** | **0** |
| Times New Roman | 4 | | **EB Garamond** | **0** |
| Arial | 4 | | **Microsoft YaHei** | **0** |
| Menlo | 4 | | **Consolas** | **0** |
| Courier New | 4 | | Arial Unicode MS | 1 |
| PingFang SC | 6 | | Apple Symbols | 1 |
| Songti SC | 4 | | Helvetica Neue | 14 |

Unchanged from wave 4d's and this plan's Q5. Menlo and Courier New at 4 each is what makes the
naked-generic repair a face-naming edit rather than a tail-only one.

---

## Geometry

**Twenty-five `line-height` pins**, each derived from the measured English **CONTENT** box —
`padding-top + padding-bottom + border-top + border-bottom` subtracted first — per line and per font
size, and each placed **inside the rule that already owns its selector**, at that rule's own
specificity (R8). The file now carries 29 `line-height` declarations: the 4 it already had, and 25
marked `zh pin:` at the site with the content box and the font size that produced them.

| ratio | font-size | English content box | selectors |
|---|---|---|---|
| **1.0** | 7px | 7px | `.rotation-table th` — the `<th>` collapsed-border exception M8 records, at 7px rather than its 8px case |
| **1.0909091** | 11px | 12px | `.fx-title`, `.tk-hint` |
| **1.1** | 10px | 11px | `.voice-label`, `.settings-label`, `.generator-btn` |
| **1.1111111** | 9px | 10px | `.slider-label`, `.dropdown-label`, `#effects-tab .knob-label`, `.section-header`, `.preset-action-btn`, `.master-label`, `.fx-bypass-btn`, `.generator-header-text`, `.library-header-text`, `.gen-row label` |
| **1.125** | 8px | 9px | `.viz-btn`, `.gliss-toggle`, `.interval-list-header`, `.pitch-circle-label`, `.ref-knob-label`, `.octave-stretch-label`, `.tuning-file-btn` |
| **1.1428571** | 7px | 8px | `.tonic-label`, `.footer-keyboard-help` |

Every ratio agrees with M8's measured table, including the `<th>` exception.

**R8, run per selector and clean ON THE EVIDENCE rather than by construction.** This is the
distinction the plan itself drew, and here it is the live case: the file carries **4** pre-existing
`line-height` declarations (`.gear-btn`, `.settings-popover select`, `.settings-toggle`,
`.tooltip-body`) and **29 CSS `min-width` floors placed for French**, and the grep was run against
each of the 25 selectors before anything was appended. **None of the 25 already declared a
`line-height`.** Seven of the 25 sit in a rule that ALSO declares a pre-existing `min-width` —
`.voice-label`, `.preset-action-btn`, `.master-label`, `.fx-title`, `.fx-bypass-btn`,
`.tonic-label`, `.octave-stretch-label` — and in each the pin went into that same rule at that same
specificity, so **no `min-width` was lowered or shadowed anywhere.** `check-ui-labels` reports
**0 moved on the fr arm**, which is the load-bearing evidence that no French floor was un-pinned.

**Two shapes beyond the line box, both measured:**

1. **`label.voices` — a caption that OVERRAN a French-era floor.** `复音数：` measures **44.89 px**
   against `.voice-label`'s **37 px** floor, so it set the width and dragged `.voice-display` +7.9 px
   and the whole preset browser −3.9 px, in every one of the fifteen states. The glossary lists
   **two** settled roots for `Voices` — `复音数` and `声部` — and the three-glyph one lands under the
   floor, so the cell renders at 37 px on all three arms. **The copy was changed, not the cell**,
   because a settled alternate existed; raising the floor would have moved English.
2. **`.rotation-table th` — a Han run wrapping in an auto-sized cell** (step 8's third shape). `模式`
   needs ~20.4 px of min-content in an 18 px column and **wrapped to two lines**, growing the header
   row 11 → 21 px and the table 7 px. `white-space: nowrap` fixed the wrap; the column was then
   19.31 px against English's 21.34 px, because this table is auto-layout and **overflows its 220 px
   container**, so its own columns set its width. A floor at the **exact measured English border
   box, 21.34 px**, scoped to `:first-child` at (0,2,1) — the twelve digit columns keep the shared
   rule's 18 px. The floor is a **raise** of an existing floor, never a lowering, which is why R8's
   hazard does not apply.

**The fourth shape did not appear, and that is a measured absence.** The 25 pre-existing
`letter-spacing` declarations were left untouched: an `html[lang="zh-Hans"]` trim is only warranted
where the Latin display convention displaces something, and with the pins in place
`check-ui-labels` reports 0 moved on all three arms in all fifteen states. **56 of 737 nodes have a
width difference across the three arms** — every one a shrink-to-fit label, which is what a
translation is supposed to do; the criterion is that no NON-label element moved, and none did.

---

## The coverage hole — a forced click that landed on a different element

**This is the finding of the task, and no instrument in the plan's verify block could see it.**

`tests/i18n-states.json` state 6 read `{"click": ".generator-header"}`. Measured with
`document.elementFromPoint`:

```
.generator-header rect       = x 472, y 393, w 200, h 19
topmost element at its centre = <div class="white-key mapped" data-note="71"><span>B</span></div>
```

The footer keyboard paints over the generator header. Playwright's `force: true` dispatches a real
mouse event at that point, so the event went to the keyboard key, `window.toggleGenerator` never
fired, and `.generator-content` stayed `display: none`. **The step did not throw. `--verbose` logged
no skip. `measure-ui --verbose` reported 0 unresolved states.** The plan's own instruction —
*"run it with `--verbose` first and confirm no step failed to resolve"* — is **blind to this class**,
because the step succeeded; it just succeeded somewhere else.

What it cost: the entire scale-generator form — **ten captions I had just authored** — was never
measured on any arm. `check-ui-labels` said so, in a line reported rather than asserted:
**6 `[data-i18n]` elements never became visible**, identical before and after my work.

Changing the step to `{"eval": "window.toggleGenerator();"}` — the form four other states in this
same file already use — closed it: **6 → 3 never visible**, and the remaining three are
`#generator-type>option:nth-child(1..3)`, `<option>` elements inside a closed `<select>`, which have
no box and are structurally unmeasurable by a DOM geometry sweep.

**Closing it turned a green gate red — 12 FAILED.** `#generator-content` grew 3 px in Chinese and
pushed three labels further past the frame edge (they already sit outside it in English; the tuning
tab is an authored scrolling pane). Two more pins — `.gen-row label` at 1.1111111 and
`.generator-btn` at 1.1 — closed those, and `check-ui-labels` returned to
`== ALL CHECKS PASSED ==` with the panel actually being looked at.

The reason recorded at the state entry is the measurement, not the conclusion, so the next reader
can re-fire it.

---

## Blind reverse read

**Shape, following Task 1's finding 4 exactly.** Three `--emit O-Lyrica --plugin O-Lyrica` runs
(both flags — W4), each with `--forward-provenance`, each producing its own 16-hex salt
(`ad2e5773…`, `e7b285f7…`, `1222ae7a…`, all different). The 213 real ids were partitioned into three
disjoint thirds of **71 + 71 + 71**, and each chunk was cut from its **own** emit by resolving real
ids through that emit's manifest — never by slicing one emit by line number, because the emitter
sorts by blinded id and the three emits are in three different orders. Dispatched with
`claude -p … --model sonnet --allowed-tools ""` from `/tmp/h8y-blind`, a cwd **outside** the repo,
with repo and web access forbidden in the prompt.

**Controls, all fired and observed:**

| control | result |
|---|---|
| partition disjoint AND covering every real id exactly once | **true / true**, asserted in code |
| ids pure 12-hex | **0 non-conforming** on all three chunks |
| ids carrying a key fragment | **0** |
| **M12 — ids identical AND IN ORDER, checked BEFORE ingest** | **71/71 ×3, `diff` clean on all three** |
| malformed returned lines | **0 on all three** |
| Han surviving in the returned English | **0 on all three** |
| **refusal 1 — emit without `--forward-provenance`** | **FIRED:** `REFUSED: the batch was emitted with no recorded forward pass — the identity check cannot run` |
| **refusal 2 — wrong-side `--manifest`** | **FIRED:** `joined: 0   unjoinable ids: 3` … `WRONG MANIFEST FOR THIS BATCH: all 3 returned ids are unjoinable, not one.` |
| product-name-in-batch | **0 hits.** Reported, not scored: this plugin's own copy does not name itself — `Ouaricon Lyrica` lives in the markup as an `I18N_EXEMPT` string and never enters the table |
| joined | **71 + 71 + 71 = 213** |

**All 213 triples read with `--verbose`.** The findings that mattered:

| key | en | zh | en′ | verdict |
|---|---|---|---|---|
| `label.genGenerator` | Generator (¢) | 生成音程（¢） → **生成元（¢）** | "Generate Interval (¢)" → **"Generator (¢)"** | **RE-AUTHORED** |
| `label.tabTechniques` | TECHNIQUES | 演奏技法 | **"Playing Technique"** | accepted — the M13 collision **prevented at authoring time**, see below |
| `label.technique` / `technique` | Technique | 技法 | **"Technique"** | accepted — reads as a different thing from the tab, which is the point |
| `label.genDivisions` | Divisions | 分割 | "Split", then **"Divide"** | **accepted, filed to D4** — see below |
| `label.freeKS` / `label.scaleKS` | Free KS / Scale KS | 自由键位 / 音阶键位 | "Free Keymap" / "Scale Key Position" | **accepted, filed to D4** — see below |
| `label.key` | Key | 主音 | "Root" | accepted — it is the caption of the `glissandoTonic` dropdown whose own tip title is 主音; a same-control pair |
| `label.tonicPrefix` | Tonic: | 主音： | "Root Note:" | accepted — TUNING tab; `label.key` is on TECHNIQUES. They do not co-occur |
| `label.vizCircle` | Circle | 圆周 | "Circumference" | accepted — settled root; the other four viz buttons are 极坐标 / 矩阵 / 真实键位 / 旋转, none confusable |
| `label.secBody` / `label.secSympathetic` | Body / Sympathetic | 共鸣体 / 共鸣弦 | "Resonator" / "Sympathetic Strings" | accepted — adjacent section headers sharing 共鸣, and the reader distinguished them |
| `ui.onCaps` / `ui.offCaps` | ON / OFF | 开启 / 关闭 | "Enable" / "Disable" | accepted — the emphatic pair; `ui.on` / `ui.off` 开 / 关 came back "On" / "Off", so the two casings stay two readings, as in English |
| `label.dynStart` / `label.dynEnd` | Dynamics: Start / End | 力度：起始 / 结束 | "Velocity: Start / End" | accepted — 力度 is the settled root for *velocity*, and this plugin's own bodies say "Velocity at the beginning of the glissando sweep" |
| `label.voices` | Voices: | 声部： | "Voice:" | accepted — number only |

### The M13 collision was PREVENTED, not found — and that is provable

R3's screen was run before a single string was authored: 167 English titles pushed through `TERMS`,
**146 in the glossary, 21 not**, producing **34 groups where more than one key reaches one root** —
every one of them a same-control pair (a caption and its own tip title) or a same-English pair (the
structurally identical Free and Scale-Locked glissando panels, whose captions read Sync / Shape /
Interval / Direction in both). **Zero groups where two DIFFERENT English keys share a root.**

The one that was not on the screen at all: **`TECHNIQUES` (the tab) and `Technique` (the SOUND-tab
dropdown) are visible at the same time**, and 技法 is the settled root for the second. Rendering
both as 技法 would have been the exact M13 shape — two different controls, one string, Z5 silent
because each rendering is the accepted root for its own English. The tab is **not** a `TERMS` key,
so it was qualified to **演奏技法** at authoring time and the glossary side was left. Both readers,
on two different models, duly returned **"Playing Technique"** and **"Technique"** as two different
things. The prevention is visible in the reverse read rather than the collision.

### The one re-author, and why it is exactly M13's shape

`label.genGenerator` was authored **生成音程（¢）** and read back as **"Generate Interval (¢)"** — a
**verb phrase**. The caption sits directly above the **生成** (Generate) button inside the same
collapsed generator panel, so a reader meets the two together as an action and its target rather
than as a noun and a control.

Nothing mechanical could see it. Neither `Generator` nor `Generate` is a `TERMS` key, so Z5 is
silent on both. The downstream check looks for two keys mapping to **one** string, and these map to
two different strings. R3's screen fires only when both sides are glossary keys sharing a root, and
neither is a glossary key at all. **Only the reverse read saw it.**

**生成元（¢）** — the settled term for a generator in tuning theory, and a noun only — with the
reason recorded in a `termNote` at the entry. **Only ONE side is qualified**, per M13: 生成 is not
ambiguous about what it does, and the reader recovered it correctly every time.

**Round 2** — fresh session, fresh salt (`afb7803347d7…`, different from all three round-1 salts),
and a **different model** (`--model opus` against round one's `sonnet`) — over the re-authored row
plus every other caption on the same panel. M12 clean, **11/11 ids in order**. It returned
**"Generator (¢)"** for 生成元 and **"Generate"** for 生成: **the pair is broken.** It corrected
nothing further, **so there is no round 3.**

### The two deferrals to D4

1. **分割 for `Divisions` reads as a verb.** Round 1 returned **"Split"**, round 2 on a different
   model returned **"Divide"** — two independent readers agreeing that the settled root reads as an
   action. Nothing on the generator panel collides with it (the panel holds 等分八度 (EDO) / 泛音列 /
   二阶音律, 分割, 周期（¢）, 生成), so by the discriminator — *collision on the page, not drift
   distance* — it is accepted. Diverging from a settled root in one plugin unilaterally is precisely
   what D4 exists to prevent. **Filed, not fixed.**
2. **键位 for `keyswitch` reads as "keymap" / "key position".** 自由键位 and 音阶键位 are settled
   roots, and this plugin **also loads and saves `.kbm` keyboard-mapping files** — 载入 .kbm /
   保存 .kbm — which is the thing a reader would take 键位 to mean. They sit on **different tabs**
   (TECHNIQUES vs TUNING) so they do not co-occur, and the bodies disambiguate cleanly: the reader
   returned *"The MIDI note that triggers free glide when held"* for the body. Accepted here, filed
   to D4 as a corpus-wide observation. `label.vizTrueKeys` 真实键位 is a third member of the same
   family, also on TUNING.

---

## Rows re-authored, and why

| # | key | from | to | reason |
|---|---|---|---|---|
| 1 | `label.genGenerator` | 生成音程（¢） | **生成元（¢）** | M13-shaped page collision with the 生成 button directly below it; the reverse read returned a verb phrase. `termNote` at the entry, one side qualified |
| 2 | `label.voices` | 复音数： | **声部：** | GEOMETRY, not terminology. 复音数： measures 44.89 px against a 37 px French-era floor and dragged the header 3.9 px; 声部 is the glossary's OTHER settled root for the same English and fits under the floor |

Nothing else was re-authored. `i18n-zh-lint --plugin O-Lyrica` reported **0 findings on its first
pass**, before either change.

---

## Checked non-defects — read in full, deliberately left, recorded

1. **The `'settings'` body.** Reads *"Choose the language of this interface and whether hover help
   appears. Both choices are remembered with the session."* — it **names both** of the panel's
   controls and is **already true**. Left unchanged, and a probe in the verify block asserts it was
   not edited. Q8 confirmed on the second of five.
2. **`js/app.js` needed NO edit** — the plan's CONDITIONAL, resolved as a non-finding with evidence.
   Every runtime-written caption in `app.js` goes through `setLabel(el, '<table key>')`: the sixteen
   EFFECTS knob captions at L747–791, the four bypass faces at L719–720, the glissando toggle faces
   at L386–387 and the hover-help switch at L1226–1227. Mechanically confirmed by `check-i18n`
   assertion **[12]**, which scans **three** modules — `index.html` inline `<script>`,
   `index.html` inline `<script type="module">`, and `js/app.js` — and reports *"no prose string is
   written to textContent / innerText outside setLabel"*. **A correction to the plan's own map:**
   the runtime-built captions the plan attributes to `js/app.js` — the rotation-table header, the
   True Keys summary row, the three generator forms, the preset dropdown — actually live in
   `index.html`'s **second inline module**, not in `app.js`; each is already keyed by a
   `window.__setLabel` call on the injected node, with the `html +=` blind spot named at the site.
3. **The `html` element on the UA `Times`.** Read, left, recorded: it renders no text and everything
   under `body` re-declares. The carrier no census entry accounts for — the same node O-Tapestop
   reported.
4. **`.matrix-table` and the POLAR / CIRCLE views were checked for a Han geometry risk and have
   none.** The matrix builder writes `getDegreeLabel()` output and `Math.round(interval)` — digits
   and note letters only — so no pin was added there, and the absence of those two views from the
   state file is not an i18n coverage hole. Recorded rather than pinned by analogy with the rotation
   table, which would have been a guess.
5. **`aria.tonicSelector12` / `aria.tonicSelectorN`** — the two faces of one accessible name, chosen
   by an if/else over two literal keys. Both authored, both read back correctly.
6. **P13 re-confirmed.** No `AudioParameterChoice` in `PluginProcessor.cpp` names a language —
   measured by scanning each declaration's own text, not the containing file.
7. **Q12 / D1 re-confirmed AFTER the work.** The CMake grep for `scala-tuning-engine` in
   `plugins/O-Lyrica/CMakeLists.txt` returns **0**; `find plugins/O-Lyrica -name tuning-panel.js`
   returns **0**. O-Lyrica is a physical-modeling harp with a full TUNING tab — a scale library,
   five visualisations and a rank-2/EDO/harmonic-series generator — and is the plugin one would most
   expect to be a consumer. **It is not, and it has no private copy either.** Its tuning captions are
   part of the 213 rows.

---

## Deviations

1. **`tests/i18n-states.json` was edited**, which the plan's `<files>` list does not name. One
   line: state 6's step form, `click` → `eval`, with the `elementFromPoint` measurement recorded at
   the entry. This is a fixture **repair**, not an invented gate — the same file already uses `eval`
   for four other states — and without it ten captions authored in this task would have shipped
   unmeasured behind a green gate. Committed path-scoped with everything else.
2. **`select` was excluded from the form-4 rule**, departing from M7's shipped
   `button, input, select, textarea` shape. The reason is measured and recorded at the site: this
   page carries its own element-level `select` rule at the same specificity, and including `select`
   would have moved the en and fr arms.
3. **`label.voices` changed for GEOMETRY, not terminology.** Both renderings are settled glossary
   roots; the shorter was chosen because the longer overran a French-era floor. Recorded because a
   diff cannot tell a copy change made for meaning from one made for width.
4. **No `auval -v`**, by instruction. The triple was read off `auval -a` instead — **`aumu OLyr
   OuDv`**.

---

## Corrections to the plan's own measurements

Instruction 18. **Five predictions were FALSE as stated.**

1. **Every `index.html` line number in the plan's font worklist is WRONG.** The plan cites the 11
   Garamond stacks at L52 / L263 / L462 / L1059 / L1140 / L1173 / L1190 / L1297 / L1342 / L1621 /
   L1850, Georgia at L1724, the seven naked generics at L943 / L1273 / L1431 / L1478 / L1518 /
   L1548 / L1554, and the five `inherit` sites at L707 / L800 / L1779 / L1794 / L1814. **The COUNTS
   are all exactly right — 11, 1, 7, 5 — and not one line number is.** The real raw numbers are
   L56/284/511/1172/1253/1286/1303/1415/1460/1755/2022, L1870, L1056/1389/1565/1612/1652/1682/1688
   and L794/897/1926/1941/1966. The offsets are not constant (113, 116, 134 …), so they were
   measured on a comment-stripped or otherwise transformed file. This is the plan's own tooling trap
   — *"comment-stripped line numbers are NOT the numbers you edit"* — applied to itself.
2. **`measure-ui --report all`'s `undeclared-font` screen is not the form-4 census** — Task 1's
   correction 4, re-confirmed from the other direction. On this page it reads **177** immediately
   after the table lands, on a plugin whose genuine form-4 population is **39**. The 177 are nodes
   whose stack names no CJK face — the *tail* half. The form-4 count must be computed by grouping
   `x.ff` over `lang === 'en' && vis`, per Q11, which is how the 39 / 157 / 539 / 1 / 1 census here
   was obtained.
3. **The verify block's `grep -rn "scala-tuning-engine" plugins/O-Lyrica/ | wc -l` criterion of `0`
   is WRONG — it reads `1`.** The hit is `plugins/O-Lyrica/CHANGELOG.md:1739`, a historical prose
   line that predates this work by many releases. The *conclusion* is intact and was checked with
   the two discriminating instruments instead — the CMake grep returns 0 and the `tuning-panel.js`
   find returns 0 — but a criterion that greps a whole plugin tree, CHANGELOG included, for a module
   name cannot distinguish consumption from a mention of it.
4. **"30 existing `min-width` declarations" counts a JS inline style.** The raw grep is 30; **29**
   are CSS declarations and the thirtieth is `btn.style.cssText = 'min-width: 24px; …'` inside the
   degree-toggle builder at `index.html:3638`. It is not a rule, it cannot be appended to, and R8's
   specificity hazard does not apply to it — it is an inline style at (1,0,0,0) that beats every
   rule on the page. Worth separating, because a pin surface of 29 rules plus one inline style
   behaves differently from 30 rules.
5. **`--verbose` reporting "no unresolved state" does NOT establish that every state applied.**
   The plan states the criterion as *"run it with `--verbose` first and confirm no step failed to
   resolve"*, and both `measure-ui --verbose` (0 unresolved) and `check-ui-labels`
   (`states: default + 15`) reported clean while one of the fifteen states was silently doing
   nothing, because a `force: true` click that lands on a covering element **succeeds**. The
   instrument that finds this class is `document.elementFromPoint` at the target's own centre, and
   nothing in the plan asks for it. **Wave 4f should add it**, and it costs one page evaluation per
   click-form state.

**Predictions that held exactly:** the 213-row count and its 46 × 2 + 121 decomposition, the 15
states, the 700 × 450 tight-form frame, `Resources/ui/` as the UI root, **24** inline font
declarations with no external stylesheet, **11 / 1 / 7 / 5** as the four font-site counts,
**39** form-4 nodes against **5** `inherit` declarations, **157** nodes on a bare `monospace` from
**seven** declarations, **380** `line-height: normal` visible leaves, **2010** measured en+fr nodes
and the 2010 / 1005 / 284 identity triple, **30 / 2 / 4 / 25 / 2** as the raw pin-surface greps,
`VERSION "2.4.4"` as a **quoted** literal, the codec at **L208–209** with **no doc comment** above
it, the stale-enumeration probe firing **2** (the en body and the fr body), the `'settings'` body
being already true, `svg-font-attr` **0**, the runtime-rebuild mechanism forcing a CSS rule rather
than an id list, O-Lyrica's **non-consumption** of `scala-tuning-engine`, and the prediction that
this page would produce an M13-shaped collision — it produced two, one prevented at authoring time
and one caught by the reverse read.

---

## Findings Task 3 must know

1. **Fire `document.elementFromPoint` on every `click`-form state before trusting a zero.** This is
   the new instrument. `--verbose` and the gate's own state counter are both blind to a forced click
   that lands on a covering element, and the failure is silent in both directions: the state does
   nothing, and the panel it would have opened reports as never-visible in a line the gate prints as
   a NOTE rather than a FAIL. One page evaluation per state.
2. **The three `O-simple*` plugins are font-clean by the plan's census (0 form-4, 0 naked generics)
   — but that census is the `x.ff` grouping, not `undeclared-font`.** Do not read
   `undeclared-font: N` as N form-4 nodes; it is the CJK-tail half.
3. **The two-arm CMake reader is mandatory on all three of Task 3's plugins** — they are the three
   that return EMPTY on the one-arm form. O-Lyrica's own bump went on a **quoted `VERSION` literal**.
4. **The blind-dispatch shape works at three chunks.** Emit N times for N salts, partition the REAL
   ids, then cut each chunk from its own emit through its own manifest. M12 before ingest, both
   refusal controls, `--verbose`, every triple.
5. **Two glossary items are queued for D4** from this task: 分割 for *Divisions* reading as a verb on
   two independent models, and 键位 for *keyswitch* reading as *keymap* on a plugin that also ships
   `.kbm` files. Task 1 queued 通过长度 for *pass length*. **That is three D4 entries for wave 4e.**
6. **Line-box ratios reconfirmed at five sizes**, each from the CONTENT box: **7px → 1.1428571,
   8px → 1.125, 9px → 1.1111111, 10px → 1.1, 11px → 1.0909091**, plus the `<th>` exception at
   **7px → 1.0**. All agree with M8.
7. **A settled root can be the wrong choice for GEOMETRY reasons**, and when the glossary lists two
   the shorter one is a legitimate lever — cheaper and safer than moving a French-era floor.
   `label.voices` is the worked example.
8. **The endonym costs 1 px on the `<select>`**, on all three arms. Expect it on Task 3's three and
   do not mistake it for a font regression.

---

## Commits

| hash | message | files |
|---|---|---|
| `8f2e2535` | `feat(O-Lyrica): Simplified Chinese at reviewed:'mt', the wave's whole font surface, one measured coverage hole closed` | `Resources/ui/js/i18n.js`, `Resources/ui/index.html`, `Source/PluginProcessor.h`, `tests/i18n-states.json` |
| `a62cf6c4` | `feat(O-Lyrica): promote zh-Hans to reviewed:'bt', v2.5.0` | `Resources/ui/js/i18n.js`, `CMakeLists.txt`, `CHANGELOG.md` |

Both path-scoped to `plugins/O-Lyrica`, both carrying the session trailer. No `git add -A`, no tag,
nothing pushed, `modules/` untouched. The only other change in the tree is
`.gsd/dispatch-isolation-sentinel.json`, which predates this work and was never staged.
