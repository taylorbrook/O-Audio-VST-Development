# O-Lyrica — visual review (260920-a91)

Rendered-evidence review of the shipped WebView UI at its real frame. Nothing under
`plugins/` was modified. This catalogues; it does not fix.

## Coverage

| Fact | Value | Source |
|---|---|---|
| UI root | `Resources/ui` (resolved from the CMake `SOURCES` block, not probed) | `shots/O-Lyrica/manifest.json` `uiRootFrom: cmake` |
| Frame | **700 x 450** | `setSize(700, 450)` parsed from `Source/PluginEditor.cpp` |
| Languages | `en`, `fr`, `zh-Hans` | `LANGUAGES` read off `Resources/ui/js/i18n.js` |
| States walked | 16 (synthetic `default` + 15 from `tests/i18n-states.json`) | `shoot-ui.js --plugin O-Lyrica --list` |
| Frames captured | **26** — all 16 states in `en`, plus states 0 / 1 / 5 / 12 / 14 in `fr` and `zh-Hans` | manifest |
| Duplicate frames | **2, in the `en` arm** (see below) | manifest `duplicateOf` |
| Skipped steps | **0** in every arm | manifest + `measure-ui --verbose` (grep `step skipped` → 0) |
| Page errors | 0 | manifest `pageErrors: []` |
| Computed-style census | exit 0 — **3018 nodes / 1006 distinct DOM keys / 284 display ids** | `measure-ui.js --plugin O-Lyrica --mode box --report all --verbose` |
| Label-clip gate | **exit 0, ALL CHECKS PASSED** | `check-ui-labels.js --plugin O-Lyrica` |

### The two duplicate frames — resolved, and they are finding 2

`en__08` (generator → Rank-2) and `en__09` (generator → back to EDO) are **byte-identical**
to `en__07` (generator → Harmonic Series). The `shoot-ui.js` sha detector named them; they are
**not** presented as captured states above.

This is **not** a silently skipped step and **not** an unreachable state. All three `eval`
steps ran (`stepSkipped: null`) and all three genuinely rebuilt the form — driving
`window.handleGeneratorTypeChange(...)` and re-reading the DOM shows `#generator-inputs` own
text change on every call:

| After state | `#generator-inputs` text | `#generator-section` height |
|---|---|---|
| 6 (`toggleGenerator`) | `Divisions…` (EDO form) | 154px |
| 7 (`harmonic`) | `Start HarmonicEnd Harmonic` | 154px |
| 8 (`rank2`) | `Generator (¢)Period (¢)Notes` | **181px** |
| 9 (`edo`) | `DivisionsPeriod (¢)` | 154px |

The frames are identical because **the rebuilt form is not inside the shipped 700x450 frame**.
That is findings 1 and 2 below, and it is the most important thing this review found.

### Design intent this is judged against

`plugins/O-Lyrica/.planning/creative-brief.md` `## UI Concept` states, verbatim:

> **Layout:** To be designed in mockup phase
> **Visual Style:** To be designed in mockup phase

**O-Lyrica's brief carries no visual specification.** There is nothing to measure it against
on its own terms, so it is judged here against the Ouaricon house language and against the
other two plugins in this batch. Cross-plugin divergences are raised again, with the
"which one is right" call, in `260920-a91-REVIEW-SUITE.md`.

### Blindnesses carried into every finding below

1. **Every slider value on this page is a FIXTURE.** `stubSeed('O-Lyrica').__from` is
   `neutral-defaults` with **no** `natives` overrides — neutral `0..1` ranges, default at the
   MIDDLE. That is why every readout is `50%`, `500 ms`, `0.5 n/s`, `1001 ms`. **No finding
   below is about a displayed value.**
2. **`TONIC: undefined` on the tuning tab is a harness artefact, not a defect.** Evidence:
   `stubSeed('O-Bells').natives` supplies `getTonicNote` and O-Bells' tonic renders correctly;
   `stubSeed('O-Lyrica').natives` is **empty**, so the generic stub infers a benign value for
   that unknown native name and it lands as the literal string. The README is explicit that
   the stub "does not reject unknown native-function names". Confirming the shipped behaviour
   needs a real instance.
3. **`window.__stubReport.rangesFrom` cannot be trusted as a seed report.**
   `scripts/ui-stub/generic-juce-stub.js:116` **hard-codes** `rangesFrom: 'neutral-defaults'`
   and never reads `window.__stubOverrides`. For O-Lyrica the value happens to be correct;
   for O-Bells the same field reads `neutral-defaults` while the real seed is
   `param-dump (65 rows) + overrides`. Recorded here because a later batch will read that
   field and be misled. It is a harness defect in `scripts/`, deliberately not fixed by this
   read-only review.
4. **A result through the generic stub proves layout and text. It never proves a control is
   wired.** No "this control is dead" finding is made or can be made.
5. **Every hex, px and font-stack traces to a computed-style row or a repo grep** — never to a
   sampled JPEG pixel.

## Screenshots

| File | Lang | State | What it shows |
|---|---|---|---|
| [`en__00-default.jpg`](shots/O-Lyrica/en__00-default.jpg) | en | default | SOUND tab: the 5-column horizontal-slider grid, five section headers, footer |
| [`en__01-techniques…`](shots/O-Lyrica/en__01-techniques-tab-the-section-headings-both-keyswit.jpg) | en | TECHNIQUES tab | Keyswitch settings, Free / Scale-Locked glissando, the `Dynamics: End` orphan row |
| [`en__05-tuning-tab…`](shots/O-Lyrica/en__05-tuning-tab-the-interval-list-header-with-its-n-t.jpg) | en | TUNING tab | Interval list, pitch circle, five viz buttons, TUNING LIBRARY column |
| [`en__06-the-scale-generator-expanded…`](shots/O-Lyrica/en__06-the-scale-generator-expanded-generate-scale-s-th.jpg) | en | generator expanded | **The generator is nowhere on screen** — this is the evidence for findings 1–2 |
| [`en__07-…harmonic-series…`](shots/O-Lyrica/en__07-the-generator-switched-to-harmonic-series-label-.jpg) | en | generator → harmonic | Indistinguishable from `en__06` except a 1px chevron; `en__08`/`en__09` are byte-identical to this |
| [`en__12-effects-tab…`](shots/O-Lyrica/en__12-effects-tab-the-four-section-titles-the-four-byp.jpg) | en | EFFECTS tab | Four arc-knob rows in three different column phases (finding 5) |
| [`en__13-the-chorus-section-bypassed…`](shots/O-Lyrica/en__13-the-chorus-section-bypassed-ui-off-the-other-fac.jpg) | en | chorus bypassed | The greyed-out bypass state — legible, and listed under *already good* |
| [`en__14-settings-popover-open…`](shots/O-Lyrica/en__14-settings-popover-open-label-language-label-hover.jpg) | en | settings popover | Popover over the REVERB / EQ knobs (finding 7) |
| [`fr__00-default.jpg`](shots/O-Lyrica/fr__00-default.jpg) | fr | default | `ENREG.` / `OUVRIR`, `BRILL. CHEVALET`, `Voix : 0/16` |
| [`fr__01-techniques…`](shots/O-Lyrica/fr__01-techniques-tab-the-section-headings-both-keyswit.jpg) | fr | TECHNIQUES tab | French glissando panel at the same geometry as en |
| [`fr__12-effects-tab…`](shots/O-Lyrica/fr__12-effects-tab-the-four-section-titles-the-four-byp.jpg) | fr | EFFECTS tab | French knob captions in the same 44x44 arc family |
| [`zh-Hans__00-default.jpg`](shots/O-Lyrica/zh-Hans__00-default.jpg) | zh-Hans | default | The tab bar does **not** shrink — compare O-Marimba finding 3 |
| [`zh-Hans__05-tuning-tab…`](shots/O-Lyrica/zh-Hans__05-tuning-tab-the-interval-list-header-with-its-n-t.jpg) | zh-Hans | TUNING tab | `音程（12 个音）`, `调音库`, and the same missing generator |
| [`zh-Hans__12-effects-tab…`](shots/O-Lyrica/zh-Hans__12-effects-tab-the-four-section-titles-the-four-byp.jpg) | zh-Hans | EFFECTS tab | The 16 value readouts that `line-height-normal` flags (finding 4) |
| [`zh-Hans__14-settings-popover-open…`](shots/O-Lyrica/zh-Hans__14-settings-popover-open-label-language-label-hover.jpg) | zh-Hans | settings popover | `语言` / `悬停帮助` in the same 178x63 popover |

## Findings

| # | Severity | Area | Element | What's wrong (measured evidence) | Recommended change |
|---|---|---|---|---|---|
| 1 | P3 *(was P1 — retracted 2026-09-20)* | layout | `#tuning-tab` | **Retracted as a defect.** The tab is a scroll region (`overflow-y: auto`, `clientHeight: 334`, `scrollHeight: 535`) and **the shipping WKWebView shows a scrollbar** (user-verified). The capture showed none because macOS overlay scrollbars only paint during a scroll gesture and the walk never scrolled — a harness blind spot, not a plugin defect. What remains is a preference: `div.tuning-controls-panel` is `position: absolute; height: 520` in a 334px box, so 38% of the tuning column is reached by scrolling rather than at a glance. | Optional polish only: collapsing the right column into the existing `TUNING LIBRARY ▼` disclosure (toggle at `(670, 95)`) would let the column fit 334px without scrolling, as O-Bells' tuning tab does. Not a priority. |
| 2 | P3 *(was P1 — retracted 2026-09-20)* | layout | `#generator-section` | **Retracted as "unreachable"** — it is reachable by scrolling (see 1). What the measurement actually shows: the section sits at `y = 384…538`, so at `scrollTop: 0` only its header strip (`y 384…403`) is in the viewport, and that strip is painted over by `div.footer` (`elementFromPoint(572, 392)` → `div.footer`, finding 3). The three byte-identical generator frames are explained by the same thing: the rebuilt `#generator-inputs` rendered below the un-scrolled capture viewport, not by a dead handler. | Nothing beyond finding 3. If the disclosure in finding 1 is ever done, the generator is the natural thing to fold. |
| 3 | P2 *(was P1)* | layout | `div.footer` vs `.tab-content` | `div.footer` is `(3,392) 694x55` at `z-index: 10`; every `.tab-content` is `(3,73) 694x334`, ending at `y=407`. The footer therefore **paints over the bottom 15px of every tab, on every tab, in every language**. It is not cosmetic: on TECHNIQUES with both custom groups revealed, `div.slider-group` "Dynamics: Start" occupies `y 389…413` and `#glissandoCustomSemitonesValue` sits at `y=403` — both under the footer. | Reduce `.tab-content` height to `319px` (ending at `y=392`) so the two never overlap, or give the footer `position: static` and let the tab flow above it. The 15px is currently paid for by whichever control happens to land last. |
| 4 | P2 | i18n | 25 zh-Hans leaf nodes | `measure-ui`'s `line-height-normal` screen reports **25 findings on the zh-Hans arm** — more than three times O-Marimba's 8 on a page only 55% larger. They include **all four tabs** (`#tab-sound`, `#tab-techniques`, `#tab-tuning`, `#tab-effects` at 10px) and **all 16 effect value readouts** (`#chorusRateValue` … `#eqHighGainValue` at 9px). With `line-height: normal` the UA picks the line box from the resolved CJK face, so these boxes are the only ones on the page whose height is not under the stylesheet's control. | Set an explicit `line-height` on `.tab`, `.knob-value` and `.slider-value` — `1.1` at these sizes. This also removes 266 of the 386 nodes that `wrap-count` currently has to **estimate** at `fsn * 1.2` rather than measure. |
| 5 | P2 | layout | `#chorus-knobs`, `#delay-knobs`, `#reverb-knobs` | The four effect rows each centre their own knobs, so **no knob in any row aligns with a knob in any other row**. Measured knob x-origins: chorus `262 / 342 / 422`; delay `234 / 314 / 450`; reverb `142 / 222 / 302 / 382 / 462 / 542`. All three use an 80px pitch, but chorus is 40px out of phase with reverb and delay is 92px out of phase with both. Delay is additionally irregular *within* itself — a **136px** gap between Feedback (314) and Mix (450) where the `Mode` select is injected, against 80px everywhere else. [`en__12-effects-tab…`](shots/O-Lyrica/en__12-effects-tab-the-four-section-titles-the-four-byp.jpg) | Left-align every row to a shared 80px column grid starting at `x=142` (reverb's origin — the row with the most knobs, so it defines the grid). Give the `Mode` select its own column slot rather than letting it stretch the row it sits in. |
| 6 | P2 | consistency | SOUND/TECHNIQUES vs EFFECTS controls | One plugin, two control families for the same kind of parameter. SOUND and TECHNIQUES use horizontal `input[type=range]` sliders — `#brightness`, `#sympatheticAmount` etc., `97x6` track, computed `font-size: 13.3333px` (the UA default, inherited by nothing else on the page). EFFECTS uses `44x44` arc knobs. `Mix`, `Depth`, `Amount` and `Humanize` exist in **both** families with different affordances. | Pick one. The arc knob is the house form (O-Marimba uses radial knobs throughout) and reads better at this density — but converting 30+ sliders is expensive, so the cheaper correct answer is to state the rule: **sliders for continuous timbre parameters, knobs for effect sends**, and document it so the next plugin does not have to guess. |
| 7 | P2 | affordance | `#settings-popover` | `(504,307) 178x63`, `z-index: 1001`, `background: rgba(245,230,211,0.98)`. On the EFFECTS tab it **fully occludes the REVERB `Mix` readout (`#reverbMixValue`) and the EQ `High` knob**, with no backdrop, no dimming and no tail pointing at its anchor `#gear-btn` at `(662,377)` — which is *below and right of* the popover. Because `serve-ui`/`measure-ui` click with `force: true`, a control under it is clicked *through* it silently (`pattern_forced_click_under_popover_silent_coverage_hole`). [`en__14-settings-popover-open…`](shots/O-Lyrica/en__14-settings-popover-open-label-language-label-hover.jpg) | Move the gear into the header row beside `#preset-load`, drop the popover downward from it over the tab bar, and add a `rgba(60,47,47,0.25)` backdrop so occluded controls read as unavailable. The identical fix applies to O-Marimba finding 7 — this is shared-module-shaped. |
| 8 | P2 | colour | `div.section-header` | **One colour carries three different semantic roles.** `rgb(92,64,51)` (`#5C4033`) is the computed `color` of `div.section-header` (9px, the five SOUND section names), of the **inactive** tab labels `#tab-techniques` / `#tab-tuning` / `#tab-effects` (10px), and of every value readout (`#humanizeValue`, `#masterVolumeValue`, 9–10px). A section heading, a navigable-but-inactive tab and a live numeric value are visually the same class of text; the hierarchy is carried by position alone. | Give headings their own treatment — `#3C2F2F` (already the active-tab colour) at the same 9px with the existing `letter-spacing: 1.5px`, and drop the readouts to `#8B7355`. That leaves `#5C4033` to mean "inactive control" and nothing else. |
| 9 | P2 | i18n | `#preset-save`, `#bridgeBrightnessValue` label | Both preset buttons are pinned at `56x18` / `font-size: 9px`, which forces French to ship the abbreviation **`ENREG.`** beside the unabbreviated `OUVRIR` — one abbreviated and one not, side by side in the header. The SOUND grid shows the same compromise: `Bridge Bright` → **`Brill. Chevalet`**. Visible in [`fr__00-default.jpg`](shots/O-Lyrica/fr__00-default.jpg). | There are **110px** of free header width to the right of `#preset-load` (right edge `x=567`, container right `x=697`). Widen both buttons to `min-width: 68px` and ship `ENREGISTRER` / `OUVRIR` unabbreviated. |
| 10 | P3 | typography | page-wide type floor | Nine declared sizes — `6px`(1) `7px`(7) `8px`(18) `9px`(18) `10px`(21) `11px`(4) `12px`(2) `14px`(1) `16px`(1). Eight nodes are below the 8px floor, including `div.footer-keyboard-help` ("Click to play") at `7px` in a `36.2x8` box. At the AU window scale a 700x450 editor gets in Logic, 6–7px strokes are sub-pixel. | Raise the 6px and 7px declarations to 8px — the page's most-used small size, 18 declarations — and delete the 14px and 16px one-offs, which each appear once and buy no hierarchy. |
| 11 | P3 | affordance | `div.settings-cluster` | The gear sits at `(662,377)` with `z-index: 100`, which is **inside `.tab-content` (`y 73…407`)**, not in the header or the footer. It therefore floats over live tab content on all four tabs and has to out-stack it to stay clickable. It is also 20x20 with a 12px glyph — the smallest target on the page. | Move it into `div.header` (finding 7 wants it there anyway) and raise the hit area to 24x24. |
| 12 | P3 | consistency | `#preset-save` / `#preset-load` order | O-Lyrica renders **Save at `x=447`, Load at `x=511`**. O-Bells matches it (`#preset-save` `x=613`, `#preset-load` `x=683`). **O-Marimba is the outlier** — `#preset-load-btn` `x=454`, `#preset-save-btn` `x=514`, the same two buttons in the opposite order at almost exactly the same coordinates. A user who learns one mis-clicks the other. | **Leave O-Lyrica alone and change O-Marimba.** Save-then-Load is already 2 of 3 and is the cheaper convergence; the two plugins that agree also both put a preset-name display between the arrows, so the whole cluster is one copyable pattern. Carried into the SUITE file, where the full three-way comparison lives. |
| 13 | P3 | colour | palette outliers | The palette is otherwise disciplined — 14 distinct hexes with four carrying the page (`#3C2F2F` 38 uses, `#8B7355` 35, `#5C4033` 31, `#6B8E4E` 14). Against that, **`#FF9800` (Material Design Orange 500)**, **`#FFD700` (web "gold")** and **`#8BC34A` (Material Light Green 500)** each appear once or twice — stock framework colours in a hand-built parchment palette. | Replace `#FF9800` and `#FFD700` with a single warning/highlight tone derived from the existing browns (e.g. `#C9A27B`), and fold `#8BC34A` into `#6B8E4E`. |
| 14 | P3 | layout | `#botanicalOverlay` | `(442,-15) 285x480` — right edge `x=727` against a 700px frame and bottom edge `y=465` against 450. The decorative bleed on the top and bottom reads as intentional, but the **27px cut at the right edge lands mid-canopy**, so the tree ends in a vertical straight line rather than fading. [`en__00-default.jpg`](shots/O-Lyrica/en__00-default.jpg), right side. | Shift to `x=415` (so `right = 700`) or add a 40px `linear-gradient` mask on the right edge. The overlay is at `opacity` low enough that either is a one-line change. |

**Severity counts: P1 = 0, P2 = 7, P3 = 7 (14 total).** *Revised 2026-09-20: findings 1–2 retracted to P3 (tab scrolls; scrollbar present in the shipping WKWebView), finding 3 lowered to P2.*

## What's already good

Do not touch these. Three of them are the answer to defects filed against the other plugins
in this batch.

- **The tab bar is the suite's correct pattern.** `#tab-sound` / `#tab-techniques` /
  `#tab-tuning` / `#tab-effects` measure `173.8 / 173.8 / 173.8 / 172.8` and flex-distribute
  across the full 694px bar — **byte-for-byte identical geometry in en, fr and zh-Hans**.
  O-Marimba's content-sized tabs shrink 26% in Chinese; this design cannot. It is what
  O-Marimba finding 3 should copy.
- **The label geometry is clean and measured clean.** `check-ui-labels --plugin O-Lyrica`
  exits **0** with ALL CHECKS PASSED: 117 `[data-i18n]` elements visible, zero clipping, zero
  label-on-label intersection, zero label-on-decoration collision, in all three languages.
  The 5-column SOUND grid is exact — 133.5px pitch, `118.8px` groups, last column's right
  edge at `676.8` against a `677` container. Nothing here needs touching.
- **The bypass state is real feedback.** Bypassing CHORUS greys the entire section — knobs,
  captions and readouts together — rather than just flipping a button label
  ([`en__13-the-chorus-section-bypassed…`](shots/O-Lyrica/en__13-the-chorus-section-bypassed-ui-off-the-other-fac.jpg)).
  This is the clearest affordance in the batch.
- **The effects arc knob.** `44x44` with a green arc, 9px caption and 9px value stacked
  beneath reads cleanly at four-rows-deep density. Finding 5 is about where the columns sit,
  not about the knob — the knob itself is right.
- **The persistent footer.** Master volume, playable keyboard and wordmark in one 55px band
  that survives every tab switch, with `div.white-key.mapped` carrying real semantics rather
  than a paint-only state. Finding 3 is about its z-order, not its existence.

## Open questions

- **Does the shipped plugin render `TONIC: undefined`?** Under this harness it does, and the
  cause is measured as a stub artefact (blindness 2) — O-Bells supplies `getTonicNote` as a
  native override and renders correctly, O-Lyrica supplies no natives at all. The asymmetry
  is worth one check on a real instance, because a `getTonicNote` that returns nothing in the
  DAW would present exactly this way.
- ~~Was the generator ever reachable?~~ Resolved 2026-09-20: it is — the tuning tab scrolls
  and the shipping WKWebView shows a scrollbar. The capture walk never scrolled, so the
  overlay scrollbar never painted. Recorded as a harness blindness in the SUITE file.
- **Should O-Lyrica get a visual spec?** Its brief says "To be designed in mockup phase" for
  both layout and visual style, so every finding here is measured against the house language
  rather than a stated intent. The other two plugins in this batch have real UI sections.
