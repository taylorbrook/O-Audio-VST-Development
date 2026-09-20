# O-Marimba — visual review (260920-a91)

Rendered-evidence review of the shipped WebView UI at its real frame. Nothing under
`plugins/` was modified. This catalogues; it does not fix.

## Coverage

| Fact | Value | Source |
|---|---|---|
| UI root | `Source/ui/public` (resolved from the CMake `SOURCES` block, not probed) | `shots/O-Marimba/manifest.json` `uiRootFrom: cmake` |
| Frame | **600 x 400** | `setSize(600, 400)` parsed from `Source/PluginEditor.cpp` |
| Languages | `en`, `fr`, `zh-Hans` | `LANGUAGES` read off `Source/ui/public/js/i18n.js` |
| States walked | 4 (synthetic `default` + 3 from `tests/i18n-states.json`) | `shoot-ui.js --plugin O-Marimba --list` |
| Frames captured | **12** (3 languages x 4 states), **12 distinct** | manifest — `duplicates=0` |
| Skipped steps | 0 | manifest `stepSkipped: null` on all 12 |
| Page errors | 0 | manifest `pageErrors: []` |
| Unplaced / missing embeds | 0 / 0 | manifest |
| Computed-style census | exit 0 — **1167 nodes / 389 distinct DOM keys / 199 display ids** | `measure-ui.js --plugin O-Marimba --mode box --report all --verbose` |

### Blindnesses carried into every finding below

1. **Every slider value on this page is a FIXTURE, not a shipped default.** The generic stub
   seeds a neutral `0..1` range with the default at the MIDDLE — `manifest.seedFrom` reads
   `neutral-defaults`. That is the whole reason six knobs read `50%`, `#output-value` reads
   `-6.0 dB` and `#strike-value` reads `Center`. **No finding below is about a displayed
   value.** Findings about readout *typography, position, alignment and width* are unaffected
   and are the only value-adjacent claims made.
2. **A result through the generic stub proves layout and text. It never proves a control is
   wired.** There is no "this control is dead" finding here and none can be made from this
   harness.
3. **Every hex, px and font-stack below traces to a `measure-ui.js --mode box` computed-style
   row or a repo grep — never to sampling a pixel in a JPEG.** The screenshots carry
   composition, hierarchy and alignment judgement only.
4. **COVERAGE HOLE — the EFFECTS tab was never opened.** `tests/i18n-states.json` contains
   three states (`tuning tab`, `custom (Scala) mode`, `settings popover`) and none of them
   clicks `#tab-effects`. The census confirms it: `#main-tab` and `#tuning-tab` appear in the
   visible-node set and **no `#effects-tab` does**. Everything behind that tab — the third of
   three top-level panels — is **unreviewed**, not clean. A later batch owes it.
5. `#tooltip` was checked and is **not** a finding: it computes `opacity: 0` /
   `pointer-events: none` at `(0,0) 22x14`, so its presence in the visible-node set is a
   parked hover layer, not a stray overlay. Recorded because the box looks alarming in a raw
   row dump.

### Design intent this is judged against

`plugins/O-Marimba/.planning/creative-brief.md` `## UI Vision (Finalized)` — the most specific
brief of the three plugins in this batch. It names a 600x400 window, paper texture, a
coral/tree botanical overlay that shifts right on the tuning tab, **browns `#8B7355`, greens
`#6B8E4E`, cream `#F5E6D3`**, Garamond serif, wooden radial knobs, and explicit component
sizes (interval list 100x200, pitch circle 150px, keyboard 400px / 2 octaves, waveform
260x120, VU 80px).

## Screenshots

| File | Lang | State | What it shows |
|---|---|---|---|
| [`en__00-default.jpg`](shots/O-Marimba/en__00-default.jpg) | en | default | SOUND tab at rest: six radial knobs, waveform panel, velocity curve, OUTPUT + VU |
| [`en__01-tuning-tab.jpg`](shots/O-Marimba/en__01-tuning-tab.jpg) | en | tuning tab | Interval list, pitch circle, mode buttons, A4 REF, scale name, keyboard |
| [`en__02-custom-scala-mode.jpg`](shots/O-Marimba/en__02-custom-scala-mode.jpg) | en | custom (Scala) mode | CUSTOM active; the four `.SCL` / `.KBM` file buttons at 7px |
| [`en__03-settings-popover.jpg`](shots/O-Marimba/en__03-settings-popover.jpg) | en | settings popover | Popover open over the file-button row; Language select + Hover help |
| [`fr__00-default.jpg`](shots/O-Marimba/fr__00-default.jpg) | fr | default | French SOUND tab — `AMORT. HARMON.`, `CHARG.` / `ENREG.` in the header |
| [`fr__01-tuning-tab.jpg`](shots/O-Marimba/fr__01-tuning-tab.jpg) | fr | tuning tab | French file buttons rendering `CHARG. .SCL` / `ENREG. .KBM` |
| [`fr__02-custom-scala-mode.jpg`](shots/O-Marimba/fr__02-custom-scala-mode.jpg) | fr | custom (Scala) mode | Same row after the CUSTOM/`PERSO` click |
| [`fr__03-settings-popover.jpg`](shots/O-Marimba/fr__03-settings-popover.jpg) | fr | settings popover | `Langue` / `Infobulles`, popover at the same anchor |
| [`zh-Hans__00-default.jpg`](shots/O-Marimba/zh-Hans__00-default.jpg) | zh-Hans | default | The tab-bar shrink is visible here against `en__00` |
| [`zh-Hans__01-tuning-tab.jpg`](shots/O-Marimba/zh-Hans__01-tuning-tab.jpg) | zh-Hans | tuning tab | `音程：12`, `主音`, `自定义`, `A4 基准` |
| [`zh-Hans__02-custom-scala-mode.jpg`](shots/O-Marimba/zh-Hans__02-custom-scala-mode.jpg) | zh-Hans | custom (Scala) mode | Chinese file buttons — the only language where this row is not crowded |
| [`zh-Hans__03-settings-popover.jpg`](shots/O-Marimba/zh-Hans__03-settings-popover.jpg) | zh-Hans | settings popover | `语言` / `悬停帮助`, select holding `简体中文` in the same 71px box |

## Findings

| # | Severity | Area | Element | What's wrong (measured evidence) | Recommended change |
|---|---|---|---|---|---|
| 1 | P1 | typography | `span.axis-label.x`, `span.axis-label.y` | Computed `font-size: 6px`, boxes `7.3x6` ("In") and `13.8x6` ("Out"). A repo grep of `Source/ui/public` finds **1 declaration at 6px and 6 at 7px** — 7 nodes below the 8px floor. At the AU window scale Logic gives a 600x400 editor these are sub-pixel strokes. See [`en__00-default.jpg`](shots/O-Marimba/en__00-default.jpg), bottom-left panel. | Delete the two axis labels (the curve's meaning is self-evident from its shape) and raise the remaining 7px nodes to **9px**, the smallest size already used elsewhere on the page. |
| 2 | P1 | i18n | `#btn-load-scl`, `#btn-load-kbm`, `#btn-save-scl`, `#btn-save-kbm` | The French strings are pre-abbreviated to survive a `43.1x28` box at `font-size: 7px`, producing the literal rendered text **`CHARG. .SCL`** and **`ENREG. .KBM`** — an abbreviation period immediately followed by an extension period. Computed `own` text per `measure-ui`: en `LOAD .SCL` (42.1px), fr `CHARG. .SCL` (43.1px), zh `载入 .scl` (41.3px). Visible in [`fr__01-tuning-tab.jpg`](shots/O-Marimba/fr__01-tuning-tab.jpg). | Drop the file extension from the button face entirely — label the four buttons `LOAD` / `SAVE` and put `.scl` / `.kbm` in a `data-tip`. That removes the double period, removes the two-line wrap, and lets the fr string be the unabbreviated `CHARGER` / `ENREGISTRER`. |
| 3 | P1 | i18n | `#tab-sound`, `#tab-tuning`, `#tab-effects` | The tab group is content-sized with no `min-width`, so it **shrinks 26% in Chinese**: en `87.41 + 93.17 + 98.36`, last tab ends at **x=299.94**; zh-Hans `67.48 + 67.48 + 67.48`, last tab ends at **x=223.45**. The bar itself (`div.tab-bar`) stays `594px` wide in every language, so in zh the primary navigation occupies 38% of its own band. Compare [`en__00-default.jpg`](shots/O-Marimba/en__00-default.jpg) with [`zh-Hans__00-default.jpg`](shots/O-Marimba/zh-Hans__00-default.jpg). | Pin all three tabs to a shared `min-width: 98px` (the widest measured face, `#tab-effects` in en). This is a floor, so it also holds if a future language is wider — but re-measure, because a pin is not a guarantee. |
| 4 | P2 | spacing | `#mallet-value`, `#material-value` vs `#resonance-value` | The `50%` readouts in a single three-knob row sit at **two different baselines, 11px apart**: `#mallet-value` y=177, `#material-value` y=177, `#resonance-value` **y=166**. Cause is measured: `div.knob-label` is `h=22` for the two-line captions (`MALLET HARDNESS`, `MATERIAL HARDNESS`) and `h=11` for the one-line `RESONANCE`. The second row repeats it exactly — `#strike-value`/`#damping-value` y=277 vs `#tone-value` **y=266**. | Give `div.knob-label` a fixed `height: 22px` (two lines) with the caption vertically centred inside it. Every readout in the row then lands on one baseline regardless of caption length, in all three languages. |
| 5 | P2 | colour | page-wide palette | The brief names **three** colours (`#8B7355`, `#6B8E4E`, `#F5E6D3`). A grep of `Source/ui/public` finds **19 distinct hex values**, of which **8 are greens**: `#6B8E4E`(8) `#3C5C1A`(8) `#6B8E23`(3) `#8BC34A`(2) `#A8C08A`(1) `#5A8B3A`(1) `#4A6B2A`(1) `#2C3E10`(1). `#8BC34A` is Material Design Light Green 500 — a stock framework colour in a naturalist parchment UI. | Collapse to three greens with declared roles — `#6B8E4E` (brief green / active), `#3C5C1A` (border + active outline, already the second-most used) and one tint for fills — as CSS custom properties. Delete `#8BC34A`, `#5A8B3A`, `#4A6B2A`, `#2C3E10`, `#A8C08A`, `#6B8E23`. |
| 6 | P2 | layout | `div.waveform-display` | The single largest element on the SOUND tab — `260x120` at `(312,97)`, **13% of the 600x400 frame** — renders at rest as a flat olive block: `#waveform-path` computes to `258x0` (a zero-height path) over `background: rgba(139,168,112,0.15)`. It carries no caption, no axis and no title, and it sits directly on the botanical overlay, reading as a rectangle cut out of the tree. [`en__00-default.jpg`](shots/O-Marimba/en__00-default.jpg), top right. | Give the panel a resting state: a faint centre rule plus a 7px→9px title label (`WAVEFORM`) in the same treatment as `span.title-label` on the velocity panel, so the two display panels read as a matched pair rather than one labelled and one blank. |
| 7 | P2 | affordance | `#settings-popover` | The popover (`414,259 168x62`, `z-index: 1001`, `background: rgba(245,230,211,0.98)`) **fully occludes `#btn-load-kbm` (`424,254 44.3x28`)** and clips `#btn-load-scl`, with no backdrop, no dimming and no tail pointing at its anchor. Its anchor `#gear-btn` is at `(562,327)` — *below and right of* the popover, so it opens up-and-left from a bottom-right corner with nothing connecting the two. [`en__03-settings-popover.jpg`](shots/O-Marimba/en__03-settings-popover.jpg). | Move the gear to the header row beside `#preset-save-btn` and drop the popover downward from it, over the tab bar rather than over live tuning controls. If it must stay bottom-right, add a 6px tail at the gear edge and a `rgba(60,47,47,0.25)` backdrop so occluded controls read as disabled rather than merely hidden. |
| 8 | P2 | typography | `div.vu-meter-label` | The `Level` caption computes to `font-size: 7px` in a `76x8` box at `(454,343)`, inside the `80x80` `div.vu-meter` whose ring stroke passes through the same band — the text sits **on** the green arc rather than beside it. French `Niveau` (same box) and Chinese `电平` both inherit the collision. [`fr__00-default.jpg`](shots/O-Marimba/fr__00-default.jpg), bottom right. | Move the label below the meter box (`y: 366`, matching `#output-value` at y=358) and raise it to 8px, so it reads as the meter's caption in the same rhythm as every other `div.knob-label` → value pair on the page. |
| 9 | P2 | affordance | `#keyboard-viz` key colouring | 12 keys render (`#key-c1`…`#key-b1` + 5 accidentals) and exactly **5 are green** — the accidental positions — while `div.interval-list-header` on the same screen reads `Intervals: 12`. The brief says green marks *mapped* notes; with 12 of 12 intervals present, 5 green keys cannot mean "mapped". Green is doing double duty as both the accent colour and a semantic state. [`en__01-tuning-tab.jpg`](shots/O-Marimba/en__01-tuning-tab.jpg). | Pick one meaning and label it. Either colour *all* mapped keys and use a second tint for accidentals, or keep the piano convention (accidentals dark `#3C2F2F`) and mark mapped notes with a 2px green underline. Add a one-line legend under the keyboard where `div.keyboard-help` already sits. |
| 10 | P2 | i18n | `#preset-load-btn`, `#preset-save-btn` | Both are hard-pinned at `52x15.98` with `font-size: 9px` in every language. That box is the reason French must ship the abbreviations **`CHARG.`** and **`ENREG.`** (rendered in [`fr__00-default.jpg`](shots/O-Marimba/fr__00-default.jpg)) while en gets `LOAD` / `SAVE` and zh gets the unabbreviated `载入` / `保存`. One language is visibly second-class in the header. | Widen both to `min-width: 62px` and let them grow — there are **91px** of unused header width between `#preset-save-btn`'s right edge (x=566) and the container inset. `CHARGER` measures comfortably at 9px in 62px. |
| 11 | P3 | typography | `#gear-btn` | The only node on the page whose computed `font-family` begins **`Georgia`** — every other element resolves `Garamond, "Times New Roman", "PingFang SC", "Microsoft YaHei", serif`. It is also the smallest interactive target on the page at `20x20` with a `12px` glyph, and `measure-ui`'s `line-height-normal` screen flags it on the zh-Hans arm. | Drop the `font-family` override and let it inherit; the `⚙` glyph resolves from the system fallback either way. Raise the hit area to `24x24`. |
| 12 | P3 | layout | `div.tab-bar` | The bar is `594px` wide with `background: rgba(245,230,211,0.85)` while its three tabs end at `x=299.94` (en). That leaves a **297px empty cream band** running to the right edge on every tab, in every language — the widest unexplained flat area in the UI, and the thing that makes the zh shrink in finding 3 read as breakage rather than as a narrower word. | Either right-align the settings gear into that band (which also resolves finding 7), or stop the bar's background at the last tab and let the parchment show through. |
| 13 | P3 | consistency | `#keyboard-viz` | Measured `280x70` at `(160,305)` carrying **one** octave (7 naturals + 5 accidentals). The brief specifies a **"2-octave keyboard visualization (400px)"**. The shipped element is 30% narrower and half the range. | Either widen to the specified `400px` / 2 octaves (there is room: the section spans x=160..440 inside a 600px frame with 160px free on each side), or correct the brief. Do not leave the two disagreeing. |
| 14 | P3 | typography | page-wide type scale | Eight declared sizes — `6px`(1) `7px`(6) `8px`(7) `9px`(5) `10px`(11) `11px`(1) `12px`(2) `18px`(1). Five of the eight crowd into the 6–10px range where they are barely distinguishable, and there is **nothing between 12px and 18px**, so the title floats with no secondary tier beneath it. | Adopt a four-step scale: `18 / 12 / 10 / 9`. Map 6px and 7px → 9px, 8px → 9px, 11px → 12px. The hierarchy gets more legible, not less, because the steps become distinguishable. |
| 15 | P3 | colour | `input.interval-input` | Twelve inputs at `55x14`, `font-size: 8px`, `background: rgba(255, 255, 255, 0.7)` — pure white at 70% inside a UI whose every other surface is a parchment tint (`rgba(235,217,199,0.6)` on the parent `#interval-list`, `rgb(245,230,211)` on the active tab). The row of white slots reads as an un-styled form dropped into the plate. [`en__01-tuning-tab.jpg`](shots/O-Marimba/en__01-tuning-tab.jpg). | Change to `rgba(250, 240, 230, 0.85)` — the value already used by `#keyboard-viz` on the same screen — keeping the `#8B7355` border. Reserve pure white for the focused state so editability stays discoverable. |
| 16 | P3 | layout | tuning tab, region `x 440..597, y 300..375` | A **157 x 75px** empty parchment block bottom-right of the tuning tab, occupied only by the 20x20 gear. `#scala-buttons` ends at y=282 and `#keyboard-viz` ends at x=440, so nothing claims it. Visible in every tuning frame. | Move the MTS-ESP status indicator (called for by the brief, absent from the render) into this block, or extend `#keyboard-viz` to the full 400px the brief specifies (finding 13) and let it absorb the space. |
| 17 | P3 | layout | `#botanical` vs `div.waveform-display` | The overlay computes `opacity: 0.22` at `x=350` on SOUND and `0.15` at `x=410` on TUNING — the brief's "shifts right on tuning tab" is working. But on SOUND the tree's densest region sits behind `div.waveform-display`, whose 0.15-alpha olive fill is opaque enough to visibly sever the trunk. | Shift `#botanical` to `x=430` on the SOUND tab too, or drop the waveform panel's fill to `rgba(139,168,112,0.08)` so the tree reads continuously behind it. |

**Severity counts: P1 = 3, P2 = 7, P3 = 7 (17 total).**

## What's already good

Do not touch these. They are the strongest parts of the UI and several are the house
reference for the rest of the suite.

- **The wooden radial knobs** (`#mallet-knob` … `#output-knob`, `60x60` with `#ref-pitch-knob`
  and `#velocity-knob` at `55x55`) are the single strongest identity element in this batch —
  the radial spoke pattern reads as a turned wooden disc at 600x400 and delivers the brief's
  "wooden knob aesthetic with radial patterns" exactly. Any redesign that flattens these to
  generic arc knobs is a regression.
- **The parchment + botanical system.** `images/paper1.jpg` under a `#botanical` overlay at
  0.22/0.15 opacity with the tab-driven x-shift (350 → 410) is the brief's naturalist
  aesthetic delivered as specified, and it is what makes the plugin recognisable at a glance.
- **The tuning-mode button group** `#btn-12tet` / `#btn-scala` / `#btn-mts` — all three pinned
  at exactly `60x27` and holding `12-TET / CUSTOM / MTS-ESP`, `12-TET / PERSO / MTS-ESP` and
  `12-TET / 自定义 / MTS-ESP` **without a single pixel of movement across languages**. This is
  the only correctly width-pinned group on the page and it is the model findings 3 and 10
  should copy.
- **The pitch circle** (`div.pitch-circle`, `150x150`) with 12 radiating `#6B8E4E` lines and
  its 12 degree labels — legible, centred, and the clearest information graphic in the batch.
- **`div.title` at 18px Garamond** rendering `O-MARIMBA` with letter-spacing against the
  parchment. It reads as an engraved plate and needs nothing.

## Open questions

- **Title wording.** The brief's `## Core Identity` says the plugin title is
  *"Ouaricon Marimba"*; `div.title` renders `O-Marimba`. The `O-` prefix matches every other
  plugin in the suite, so the render is almost certainly right and the brief is stale — but
  one of the two should be corrected rather than left in disagreement.
- **Green semantics on the keyboard** (finding 9) cannot be settled from this harness: the
  generic stub proves what is painted, not what the tuning engine reported. Confirming which
  notes are "mapped" needs a real instance.
- **Does the waveform panel animate in a DAW?** `#waveform-path` is `258x0` at rest here, which
  is correct for a silent page. Finding 6 is about the *resting* state only and stands either
  way, but the resting design should be chosen knowing what the active one looks like.
