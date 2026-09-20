# O-Bells — visual review (260920-a91)

Rendered-evidence review of the shipped WebView UI at its real frame. Nothing under
`plugins/` was modified. This catalogues; it does not fix.

## Coverage

| Fact | Value | Source |
|---|---|---|
| UI root | `Resources/ui` (resolved from the CMake `SOURCES` block, not probed) | `shots/O-Bells/manifest.json` `uiRootFrom: cmake` |
| Frame | **800 x 600** — the largest in this batch | `setSize(800, 600)` parsed from `Source/PluginEditor.cpp` |
| Languages | `en`, `fr`, `zh-Hans` | `LANGUAGES` read off `Resources/ui/js/i18n.js` |
| States walked | 15 (synthetic `default` + 14 from `tests/i18n-states.json`) | `shoot-ui.js --plugin O-Bells --list` |
| Frames captured | **23** — all 15 states in `en`, plus states 0 / 2 / 4 / 14 in `fr` and `zh-Hans` | manifest |
| Duplicate frames | **0** | manifest — every state produced a distinct frame |
| Skipped steps | **0** in every arm | manifest + `measure-ui --verbose` (grep `step skipped` → 0) |
| Page errors | 0 | manifest `pageErrors: []` |
| Computed-style census | exit 0 — **3222 nodes / 1074 distinct DOM keys / 247 display ids** | `measure-ui.js --plugin O-Bells --mode box --report all --verbose` |
| Label-clip gate | **exit 0, ALL CHECKS PASSED** | `check-ui-labels.js --plugin O-Bells` |
| i18n gate | **exit 0, ALL CHECKS PASS** | `check-i18n.js --plugin O-Bells` |

### How real the values on this page are — precisely

O-Bells is **the only plugin in this batch with a parameter dump**, and the exact shape of
that matters because it decides which findings are allowed.

- `stubSeed('O-Bells').__from` = **`param-dump (65 rows) + overrides`** — 53 sliders, 7
  toggles and **6 `natives`** (`getTuningIntervals`, `getTuningName`, `getTonicNote`,
  `getOctaveStretch`, `getMasterTune`, `getEmbeddedTuningList`). O-Lyrica and O-Marimba are
  both `neutral-defaults` with **no** natives.
- **The DEFAULTS are real.** `damping` seeds `def: 0.7`, `acousticBrightness` `0.7`,
  `airAbsorption` `0.0`, `strikePosition` `0.5` — varied, parameter-specific values, not a
  uniform midpoint. That is why this page reads `70% / 50% / 70% / 0% / 20%` instead of the
  flat `50%` wall the other two show.
- **The RANGES are still normalised `0..1`** with `label: "%"` on most rows. So a readout's
  *format* and *default* are trustworthy here; a readout's *engineering unit* is not. The
  `79525 ms` bloom-speed and `55.0 s` air-time readouts are therefore reported below as
  *formatting* findings only, never as wrong values.

**One finding below (11) cites a displayed value. It is flagged as fixture-derived.** No
other finding depends on a value.

### Blindnesses carried into every finding below

1. **A result through the generic stub proves layout and text. It never proves a control is
   wired.** No "this control is dead" finding is made or can be made.
2. **`window.__stubReport.rangesFrom` reads `neutral-defaults` on this plugin and is WRONG.**
   `scripts/ui-stub/generic-juce-stub.js:116` hard-codes that string and never reads
   `window.__stubOverrides`. The authoritative value is `stubSeed().__from` above. Recorded so
   a later batch does not read the inert field and conclude O-Bells has no dump. It is a
   harness defect in `scripts/`, deliberately not fixed by this read-only review.
3. **Every hex, px and computed background below traces to a `measure-ui.js --mode box` row, a
   `getComputedStyle` probe, or a repo grep** — never to a sampled JPEG pixel.
4. **`#library-content` and `#library-list` both measure height `0` at rest.** The Tuning
   Library is a collapsed disclosure, correctly; its contents are not part of this review.

### Design intent this is judged against

`plugins/O-Bells/.planning/BRIEF.md` `## UI/UX Design` — the only brief in this batch with a
named palette:

> **Aesthetic Direction:** Ouaricon Botanical theme with snail motif
> **Hero Image:** Spiral snail shell (*Architectonica perspectiva*)
> **Color Palette:** Warm amber, bronze, cream, aged gold — reflecting bell metal patina
> **Layout:** Two-panel design (**Main / Advanced tabs**)

Findings 1 and 2 are both direct contradictions of that text, measured.

## Screenshots

| File | Lang | State | What it shows |
|---|---|---|---|
| [`en__00-default.jpg`](shots/O-Bells/en__00-default.jpg) | en | default | INSTRUMENT tab: CPU banner, Synthesis / Ensemble / Onsets — and nothing below Onsets |
| [`en__01-bloom-fine-expanded.jpg`](shots/O-Bells/en__01-bloom-fine-expanded.jpg) | en | bloom fine expanded | The override state: main sliders greyed, helper line, six per-band sliders revealed |
| [`en__02-settings-popover-open.jpg`](shots/O-Bells/en__02-settings-popover-open.jpg) | en | settings popover | Popover drops **down** from the header gear — and covers the EFFECTS tab (finding 7) |
| [`en__04-tuning-tab.jpg`](shots/O-Bells/en__04-tuning-tab.jpg) | en | TUNING tab | Interval list, pitch circle, library, the gold `Export HTML`, `GENERATE SCALE ▼` |
| [`en__05-tuning-library-open.jpg`](shots/O-Bells/en__05-tuning-library-open.jpg) | en | library open | The library disclosure expanded over the right column |
| [`en__06-tuning-generator-open.jpg`](shots/O-Bells/en__06-tuning-generator-open.jpg) | en | generator open | The generator form — **fully inside the frame**, unlike O-Lyrica's |
| [`en__11-viz-rotation.jpg`](shots/O-Bells/en__11-viz-rotation.jpg) | en | rotation viz | The rotation table view |
| [`en__14-effects-tab.jpg`](shots/O-Bells/en__14-effects-tab.jpg) | en | EFFECTS tab | Four knob rows in four column phases, and 131px of empty space below EQ |
| [`fr__00-default.jpg`](shots/O-Bells/fr__00-default.jpg) | fr | default | `ENREG.` / `OUVRIR`, `DÉSACC.`, the translated CPU banner |
| [`fr__04-tuning-tab.jpg`](shots/O-Bells/fr__04-tuning-tab.jpg) | fr | TUNING tab | French tuning column at identical geometry |
| [`fr__14-effects-tab.jpg`](shots/O-Bells/fr__14-effects-tab.jpg) | fr | EFFECTS tab | French knob captions, same four phases |
| [`zh-Hans__00-default.jpg`](shots/O-Bells/zh-Hans__00-default.jpg) | zh-Hans | default | `乐器 / 调音 / 效果` — tabs hold their geometry, no shrink |
| [`zh-Hans__04-tuning-tab.jpg`](shots/O-Bells/zh-Hans__04-tuning-tab.jpg) | zh-Hans | TUNING tab | `音程` list and `调音库` at the same 160x300 / 220x29 boxes |
| [`zh-Hans__14-effects-tab.jpg`](shots/O-Bells/zh-Hans__14-effects-tab.jpg) | zh-Hans | EFFECTS tab | The 16 readouts `line-height-normal` flags (finding 9) |

## Findings

| # | Severity | Area | Element | What's wrong (measured evidence) | Recommended change |
|---|---|---|---|---|---|
| 1 | P3 *(was P1 — retracted 2026-09-20)* | layout | `#instrument-tab` | **Retracted as a defect.** The tab is a scroll region (`overflow-y: auto`, `clientHeight: 449`, `scrollHeight: 945`) and **the shipping WKWebView shows a scrollbar** (user-verified); the capture showed none because macOS overlay scrollbars paint only during a scroll gesture and the walk never scrolled. What the numbers still say: 52% of the tab — Advanced (`y=624`), Multi-Stage Envelope (`707`), Filter (`816`), Performance (`888`), Output (`952`) — is reached by scrolling, and the brief specified a **"Two-panel design (Main / Advanced tabs)"** where the shipped UI folds Advanced into Instrument. | Optional restructure, not a fix: promoting **Advanced** to a fourth tab would put Synthesis + Ensemble + Onsets in 449px unscrolled and give finding 3's empty EFFECTS space a sibling to match. Only worth doing if scrolling the instrument tab is felt as friction. |
| 2 | P1 | colour | page-wide palette vs the brief | The brief names **"warm amber, bronze, cream, aged gold — reflecting bell metal patina"**. What ships is green. Counted from computed backgrounds on the tuning tab: `rgba(139,168,112,0.35)` **x14**, `rgb(74,107,42)` **x10**, `rgba(139,168,112,0.3)` **x3**, `rgba(139,195,74,0.15)` **x1** — **28 green surfaces** — against **2** uses of the brief's aged gold `rgb(184,134,11)` (`#B8860B`). Every effect bypass button is `rgba(107,142,35,0.5)` (olivedrab) and every effect knob arc is green ([`en__14-effects-tab.jpg`](shots/O-Bells/en__14-effects-tab.jpg)). The CSS census agrees: `#3C5C1A`(18) `#2C3E10`(5) `#8BC34A`(4) `#6B8E23`(4) `#8BA870`(2) — five greens, and **no amber or bronze hex at all**. | Re-skin the accent from green to the brief's gold. `rgb(184,134,11)` already exists in the stylesheet and already reads correctly on `Export HTML` — promote it to the active/accent role, take bronze `#8B7355` (already the most-used hex at 32) for borders, and delete the five greens. This is the single change that would most change how the plugin looks, and it is the one the brief explicitly asked for. |
| 3 | P2 | layout | `#effects-tab` | `#eqSection` — the last content on the tab — ends at `y = 335 + 76 = **411**`. The tab runs to `y=542`. That leaves **131 x 734px of empty cream, 29% of the tab's height**, on the largest frame in the batch — while the INSTRUMENT tab scrolls through 945px (finding 1). [`en__14-effects-tab.jpg`](shots/O-Bells/en__14-effects-tab.jpg) | Either give the four effect sections the space — grow each `76px` row toward `~105px` so the knobs, captions and readouts breathe — or move a section from the instrument tab's scrolled region into it. Do not leave a third of the tab blank. |
| 4 | P2 | layout | `#chorus-knobs`, `#delay-knobs`, `#reverb-knobs`, `#eq-knobs` | Each effect row centres its own knobs on `x=407`, so **no two rows share a left edge**: container origins measure `304 / 266 / 196 / 268` with widths `206 / 282 / 422 / 278`. Nothing lines up vertically down a 4-row stack. `#delayModeSelect` `(410,202) 66x18` is additionally injected *inside* the delay row, stretching it out of step with the others. **This is the same defect, in the same shape, as O-Lyrica finding 5** — two plugins, one bug. | Left-align every row to a shared column grid anchored at `x=196` (reverb's origin — the row with the most knobs defines the grid) and give `#delayModeSelect` its own slot outside the knob run. Fix it once as a shared effects-rack layout and land it in both plugins. |
| 5 | P2 | layout | `div.slider` grid, SYNTHESIS section | **The column grid changes pitch in the middle of one section.** Row 1 is **four** sliders of `171.5px` at `x = 33 / 221 / 408 / 596` (pitch 187.5). Rows 2 and 3 are **three** sliders of `234px` at `x = 33 / 283 / 533` (pitch 250). Only the first column is shared; every other control in the section sits on a different vertical line than the one above it. [`en__00-default.jpg`](shots/O-Bells/en__00-default.jpg), the SYNTHESIS block. | Commit to the 3-column / 234px grid used by five of the six rows on the tab and move `Material` down to its own row (it is a discrete choice control, not a continuous slider, so it does not belong in the row anyway). |
| 6 | P2 | colour | `#interval-list`, `button.viz-btn`, `#scale-name-display`, `#btn-load-scl` | The brief says cream; the surfaces are **white**. Twelve computed backgrounds are pure white at partial alpha — `rgba(255,255,255,0.5)` **x7** (`#interval-list`, the four inactive `button.viz-btn`, `#library-section`) and `rgba(255,255,255,0.6)` **x5** (`#scale-name-display`, the four `.SCL`/`.KBM` buttons). Against them the page's actual cream, `rgba(255,248,230,0.9)`, is used 17 times. Two near-identical surface tones with no rule separating them. | Replace every `rgba(255,255,255,0.5|0.6)` with `rgba(255,248,230,0.9)` — the value already dominant on the page — and reserve pure white for focus/edit states so editability stays discoverable. |
| 7 | P2 | affordance | `#settings-popover` | The popover drops **down** from a header gear at `(753,15)` — the correct direction, and better than either sibling plugin — but it lands on the tab bar and **covers the EFFECTS tab label**, so the third of three top-level navigation targets is unreadable while settings are open. No backdrop and no tail. [`en__02-settings-popover-open.jpg`](shots/O-Bells/en__02-settings-popover-open.jpg) | Shift the popover's left edge to align with the gear's right edge minus its own width and push it 8px further down so it clears `div.tab` (`y 53…93`) and lands on tab *content* instead of tab *navigation*. Add a `rgba(60,47,47,0.2)` backdrop. |
| 8 | P2 | affordance | `#btn-export-html` | `Export HTML` is the **only solid-filled button on the entire plugin** — `rgb(184,134,11)`, the page's one saturated fill — while `Load .SCL`, `Save .KBM` and the five `button.viz-btn` tuning-mode controls are all outline buttons on `rgba(255,255,255,0.5|0.6)`. A documentation-export utility therefore carries more visual weight than loading a tuning. [`en__04-tuning-tab.jpg`](shots/O-Bells/en__04-tuning-tab.jpg) | Demote `Export HTML` to the same outline treatment as its neighbours, and spend the gold fill on the **active** state instead — `button.viz-btn.active` currently gets only `rgba(184,134,11,0.3)`, a tint so faint that which visualization is selected is hard to read at a glance. |
| 9 | P2 | i18n | 19 zh-Hans leaf nodes | `measure-ui`'s `line-height-normal` screen reports **19 findings on the zh-Hans arm**, including **all three tabs** (`div.tab` `乐器` / `调音` / `效果` at 13px) and the 16 effect value readouts at 9px. With `line-height: normal` the UA derives the line box from the resolved CJK face, so these are the only boxes on the page whose height the stylesheet does not control. It also forces `wrap-count` to **estimate** 445 of its 589 compared nodes at `fsn * 1.2` rather than measure them. | Set an explicit `line-height: 1.1` on `.tab`, `.knob-value` and `.slider-value`. Same fix as O-Lyrica finding 4 and O-Marimba finding 11 — shared-module-shaped. |
| 10 | P3 | affordance | `#interval-list` | `(48,122) 160x300` with `overflow-y: auto`, `clientHeight: 298`, `scrollHeight: **421**` — 123px, roughly 3.5 of the 12 intervals, reached by scrolling (the overlay scrollbar paints on gesture; the capture never scrolled, so none is visible in the frames). The header reads `Intervals: 12` while nine rows are shown at rest. Its parent `div.tuning-panel` is `421px` tall, so the list's full content would fit if the box were allowed to grow. [`en__04-tuning-tab.jpg`](shots/O-Bells/en__04-tuning-tab.jpg) | Grow `#interval-list` to `height: 391px` (matching `div.tuning-intervals-column`) so all 12 rows show at 12-TET, and add a bottom fade for tunings with more degrees. |
| 11 | P3 | typography | `#bloomSpeedValue`, `#airTimeValue` | Readout **formatting**, not value (fixture-derived, flagged per `## Coverage`): the bloom-speed readout renders `79525 ms` and air-time renders `55.0 s` on the same tab. Five digits of milliseconds is unreadable at a glance and inconsistent with the seconds formatting used one row up. Per-band bloom compounds it — `31035 ms` / `79525 ms` / `318800 ms` in three adjacent boxes ([`en__01-bloom-fine-expanded.jpg`](shots/O-Bells/en__01-bloom-fine-expanded.jpg)). | Switch the bloom readouts to seconds with one decimal above 1000 ms (`79.5 s`), matching `Air Time`. Three adjacent readouts then compare at a glance instead of by counting digits. |
| 12 | P3 | layout | `#cpu-warning` | `(33,107) 734x32.2`, `background: rgba(205,133,63,0.15)`, text at **9px**. It occupies the prime slot directly under the tab bar on **every** visit to the INSTRUMENT tab, pushing `SYNTHESIS` down to `y=149`, and it is present in the default state with no dismiss control in any captured frame. A permanent warning is one a user learns to stop reading. | Shrink to a single-line inline badge beside the `Bloom Speed` control that raises the estimate, and show the full banner only when the estimate crosses a threshold. It is advice about one parameter, not about the tab. |
| 13 | P3 | colour | palette outliers | 16 distinct hexes, of which three are stock framework colours: **`#C0392B`** (Flat UI "Pomegranate") x4, **`#8BC34A`** (Material Design Light Green 500) x4, and **`#6B8E23`** (CSS `olivedrab`) x4. Finding 2 deletes the greens; the red is separate and is presumably an error/clip state. | Replace `#C0392B` with a warm red derived from the bronze family (e.g. `#A0522D`) so the alert state still belongs to the palette, and delete `#8BC34A` / `#6B8E23` with the rest of the greens. |
| 14 | P3 | typography | page-wide type scale | Ten declared sizes — `6px`(1) `7px`(2) `8px`(5) `9px`(23) `10px`(23) `11px`(9) `11.5px`(1) `13px`(3) `18px`(1) `22px`(1). The `11.5px` one-off buys nothing against the 9 uses of 11px, and three nodes are below the 8px floor on an 800x600 frame that has no space pressure at all. | Delete `11.5px`, and raise the 6px and 7px declarations to 9px (the joint most-used size). On the largest frame in the suite there is no reason for any text to be under 9px. |
| 15 | P3 | layout | `#botanicalOverlay` | `(621,88) 228x423.2` — right edge at `x=849` against an **800px** frame, so **49px of the snail is clipped by `overflow: hidden`**. On the INSTRUMENT tab the shell's densest region also sits directly behind the `Material` / `Air Time` / `Shimmer` / `Sub` column, where the labels and readouts have to compete with it ([`en__00-default.jpg`](shots/O-Bells/en__00-default.jpg), right edge). | Shift to `x=572` so the full shell lands inside the frame, and drop its opacity behind the fourth slider column. The snail is the plugin's hero image per the brief — it should not be cropped mid-spiral. |

**Severity counts: P1 = 1, P2 = 7, P3 = 7 (15 total).** *Revised 2026-09-20: finding 1 retracted to P3 (tab scrolls; scrollbar present in the shipping WKWebView).*

## What's already good

Do not touch these. The first three are the best examples of their kind in this batch and
should be the reference the other two plugins are fixed toward.

- **The bloom-fine override is the best affordance in the whole batch.** Enabling
  `BLOOM FINE CONTROLS (OVERRIDE MODE)` greys out the two main sliders it supersedes, prints
  the explanatory line *"Per-band control - main sliders disabled when active"*, and reveals
  six per-band sliders in one motion
  ([`en__01-bloom-fine-expanded.jpg`](shots/O-Bells/en__01-bloom-fine-expanded.jpg)). Cause,
  effect and consequence are all visible simultaneously. Nothing else in the batch does this.
- **The gear lives in the header.** `#gear-btn` at `(753,15) 24x24` in `div.header-bar` — the
  placement both O-Marimba (finding 7/11) and O-Lyrica (finding 7/11) are told to adopt.
  O-Bells already has it right.
- **The tuning tab FITS.** `#tuning-tab` computes `scrollHeight == clientHeight == 449` — no
  overflow at all — because the generator is collapsed behind a `GENERATE SCALE ▼` disclosure
  at the bottom of the column and opens inside the frame
  ([`en__06-tuning-generator-open.jpg`](shots/O-Bells/en__06-tuning-generator-open.jpg)).
  O-Lyrica ships what is recognisably the same tuning panel and scrolls it instead. If
  O-Lyrica's optional polish (its findings 1–2) is ever taken up, this is the pattern to copy.
- **The parameter seeding.** A `.planning/params.tsv` (65 rows) plus six `natives` overrides
  in `tests/ui-stub/generic-overrides.json` is why this review could read real defaults and
  why the tonic renders as `C` rather than `undefined`. It is also what makes every future
  headless review of this plugin more truthful than one of its siblings.
- **Both gates are green.** `check-ui-labels --plugin O-Bells` and `check-i18n --plugin
  O-Bells` each exit **0** — zero clipping, zero label collision, zero i18n structure
  failures, across all three languages and all 15 states.
- **The two-octave footer keyboard** (`C3…B4`) with the hint *"Click or use Z-M, Q-P keys"* —
  twice the range of O-Marimba's single octave, and the only plugin in the batch that tells
  the user the computer keyboard works.

## Open questions

- **Was "Advanced" ever a tab?** Finding 1 (now P3) shows four sections in the scrolled
  half of a panel the brief said should have been split into Main and Advanced tabs. Only
  matters if the scroll is felt as friction; the content is reachable.
- **Is the green accent deliberate house-style, or drift?** Finding 2 is measured against
  O-Bells' own brief, which names no green. If the suite has since standardised on the green
  accent, the brief should be corrected rather than the plugin re-skinned — but one of the two
  must move. This is the single highest-impact open decision in the batch.
- **What does `#C0392B` mark?** It appears four times and never rendered in any of the 23
  captured frames, so its state was never reached. Probably a clip/error indicator; it is a
  coverage hole either way.
