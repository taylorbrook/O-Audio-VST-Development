# O-Gain Changelog

All notable changes to O-Gain are documented here.

## [1.9.4] - 2026-09-12

**The ruler and the thing it measures were in different coordinate spaces.**
Found by checking the scale placement against the bars rather than by eye.
Cosmetic in size, a metering-accuracy fix in kind. PATCH: page CSS only — no
parameter, range, type or state format changed, no C++ touched, audio path
untouched.

### Fixed

- **Every mark that rides the bars sat up to 1 px off the gridline that names
  it.** The bar top, the held peak cap, the staging band, the mix-bus line,
  the 0 VU line and the LUFS target line are all absolutely positioned
  children of `.meter-bar-container`, so their `bottom: X%` resolves against
  that element's **padding** box. The container carries `border: 1px solid`,
  so its padding box is 2 px shorter than its border box and starts 1 px
  lower. `.meter-scale` had no border, so the numerals resolved against the
  **full** height. Measured at the shipping frame: gutter 47.98–387.03
  (339.05 px) against bar padding box 48.98–386.03 (337.05 px).

  Two spaces 2 px apart give a **linear drift, not a constant offset** — the
  bar sat 1.00 px below its gridline at 0 dBFS, 0.41 px below at −18 dBFS, and
  1.00 px above at −60 dBFS, crossing zero at −30. In dB: **−0.18 dB at the
  top of the scale to +0.18 dB at the bottom**, against the plugin's own
  ruler.

  The fix is a transparent 1 px top/bottom border on `.meter-scale`, which
  gives the gutter the identical padding box (`box-sizing` is `border-box`
  globally and both elements are height-stretched by the same flex row).
  Left and right stay 0 — the gutter is a hard 17 px and a side border would
  eat the v1.9.3 tick clearance. Post-fix error is **0.00 px at all seven
  gridlines in all four meter modes**, with the border removed again as a
  negative control to confirm the drift returns.

### Verified unchanged

- **Tick labels are exact in every mode.** Peak and RMS print dBFS; VU prints
  dBFS − (−18); LUFS prints dBFS − `target_level`. Checked at all seven
  gridlines × four modes against the value the bar actually draws there.
- **The two extreme numerals stay edge-anchored by design**, not centred: `0`
  has its top edge on the 0 dBFS line and `-60` its bottom edge on the −60
  line, each therefore 4.5 px off-centre at 9 px. This is the v1.3.0
  `tick-top` / `tick-bot` transform, which exists because a centred numeral at
  `bottom: 100%` overhangs into the column caption above and at `bottom: 0`
  into the readout below — the column's own gap there is 2 px, so there is no
  room to centre them. Unchanged; recorded here because the v1.9.3 size bump
  took the inset from 3.5 px to 4.5 px.

### Known, by design — the unit of the ruler vs. the unit of the readouts

Not changed in this release; recorded because the check surfaced them and they
are display-design decisions rather than defects:

- **In VU and LUFS mode the held peak cap is still a dBFS quantity on a
  relabelled ruler.** With peak −6 dBFS the cap sits where the VU ruler reads
  `+12`, while the `peak` readout beside it reads `-6.0`. For VU the two agree
  (VU is dBFS + 18, the same axis renamed); for LUFS the cap has no meaning on
  an LU-relative-to-target ruler.
- **In LUFS mode the ruler is relative and the readout is absolute.** With
  target −18 and momentary −23 LUFS, the bar top sits at `-5` on the LU ruler
  while the `LUFS` readout reads `-23.0`.

### Testing

- `check-ui-labels --plugin O-Gain` PASS; `check-i18n --plugin O-Gain` PASS.
- Placement measured from **painted geometry**, with no model of the box: a
  span's untransformed bottom edge is on its gridline, so the gridline is
  recovered from the rendered rect and the known transform (centre for the
  middle ticks, `rect.top` for `.tick-top`, `rect.bottom` for `.tick-bot`).
  The first cut of this probe modelled the gutter's box instead and was
  therefore blind to exactly the defect being hunted.
- Bar settling honoured: `.meter-bar` carries `transition: height 0.06s
  linear`, so a rect read in the same task as the `updateMeters` call returns
  the pre-transition value — the first run reported every bar pinned at 0 %.
- Built Release VST3 + AU, installed, auval PASS 1.9.4.

## [1.9.3] - 2026-09-12

The dB scale beside the meter bars, given the same treatment v1.9.2 gave the
readouts under them. Cosmetic. PATCH: page CSS only — no parameter, range,
type or state format changed, no C++ touched, audio path untouched.

### Changed

- **Meter scale numerals 7 → 9 px and black.** `.meter-scale span` was 7 px in
  `#8B7355`, the tan the selector borders use — the lowest-contrast text on the
  parchment ground, while being the ruler the bars are actually read against.
  It is now 9 px in `#000000`, the same black v1.9.1 gave `.mode-label` and
  v1.9.2 gave `.meter-db-val`.

- **Scale gutter 14 → 17 px.** Required, for the same reason the readout
  needed a wider column: the numerals are anchored `right: 0` (`left: 0` on
  the mirrored output column), so ink wider than the gutter does not clip — it
  spills OUTWARD, off the column toward the window edge, where nothing is laid
  out to receive it. The widest tick is `-60`: 10.89 px at 7 px, **13.98 px at
  9 px**, against a 14 px box — 0.02 px of slack. 17 px restores the
  baseline's clearance exactly, 3.02 px against the old 3.11 px.

- **Meter bars 26 → 24.5 px wide**, back toward their pre-v1.9.2 width. The
  3 px the gutter gained comes out of `.meter-pair`, which is `flex: 1` inside
  the column. The column itself stays 72 px, so nothing outside it moves —
  the centre section, the method strip and the readout row are untouched.

  `line-height` stays `1`: the ticks are absolutely positioned and the two
  extremes are pulled fully inside by `translateY(100%)` / `translateY(0)`, so
  the box is the numeral and nothing keys off its height. Vertical clearance
  is therefore unmoved by the size change — measured at 9 px, the top tick
  still meets the column caption across the column's own 2 px gap and the
  bottom tick the readout row across 2 px, and the tightest tick-to-tick
  spacing on the 339 px body is 33.9 px against a 9 px line box.

### Testing

- `check-ui-labels --plugin O-Gain` PASS, including assertion 8b (no label
  intersects a non-label element it cleared in English) — the check that would
  catch a grown tick colliding with the INPUT / OUTPUT caption — and assertion
  7 (no non-label element moves between en / fr / zh-Hans at 380 × 500).
- `check-i18n --plugin O-Gain` PASS.
- Gutter fit measured on the shipped CSS in en / fr / zh-Hans: widest tick
  `-60` at 13.98 px in a 17.00 px gutter, 3.02 px slack, no spill past the
  gutter's outer edge.
- The v1.9.2 readout sweep re-run unchanged: 60 cells, 4 meter modes × 3
  languages, minimum slack 2.89 px.
- Built Release VST3 + AU, installed, auval PASS 1.9.3.

## [1.9.2] - 2026-09-12

The dB readouts under the meters are the number this plugin exists to show,
and they were the smallest thing on the page. Cosmetic. PATCH: page CSS only
— no parameter, range, type or state format changed, no C++ touched, audio
path untouched.

### Changed

- **Input / output dB readouts 10 → 13 px and black.** `.meter-db-val` was
  10 px in `#3C2F2F`, the darkest brown on the page but not black; it is now
  13 px in `#000000` — the same black v1.9.1 gave `.mode-label` and
  `.gear-btn`. The line-height pin is re-derived rather than scaled: the
  measured English box at 13 px with `line-height: normal` is 15.00 px, no
  padding, no border, so the unitless pin is 15.00 / 13 = 1.1538. Keeping the
  old 1.1 would have pinned a 13 px face into a 14.3 px box, which is the
  clipping the pin exists to prevent.

- **Meter columns 58 → 72 px.** Required, not incidental: at 58 the value
  cell was 58 − 25 cap − 3 gap = 30 px and the widest string it can hold
  measured **30.09 px** of ink, so the readout had 0.01 px of room to grow.
  The widest string is not `-60.0` (27.58 px) as the v1.8.1 note assumed but
  the signed VU reading `+24.0` — the second readout row runs `fmtSigned()`
  in VU mode, and `+` is 3.25 px wider than `-` because the sign is not
  covered by `tabular-nums`. At 72 the row is 27 + 3 + 42 and the 13 px face
  clears `+24.0` by 2.89 px.

- **Readout caption column 25 → 27 px.** 25 never contained the second
  caption either: it carries the meter mode, and `LUFS` measures 26.02 px at
  10 px — 1.02 px past the track, painting into the 3 px gutter. 27 px
  contains it by 0.98 px and still leaves the French `crête` 4.23 px. The
  column stays pinned for the v1.8.1 reason: a `[data-i18n]` element must not
  size the track that positions the four readouts beside it.

- **Meter bars 19 → 26 px wide, as a consequence.** `.meter-pair` is `flex: 1`
  inside the column, so it absorbed the 14 px the column gained. Not part of
  the request; called out because it is the most visible change in the frame
  after the numerals themselves.

- **Centre section 232 → 204 px**, its children 220 → 192, and the method
  strip's four columns 46 → 42 px (against `+24.0` at 30.83 px in their own
  10 px face). The meter body gives up 8 px of height, 347 → 339, to the
  taller readout row; the scale ticks are percentage-positioned and re-space.

### Testing

- `check-ui-labels --plugin O-Gain` PASS, including assertion 7 (no non-label
  element moves between en / fr / zh-Hans at the fixed 380 × 500 frame) and
  assertion 6 (no language enlarges the scroll extent).
- `check-i18n --plugin O-Gain` PASS.
- Fit measured directly in the stub, all four meter modes × en / fr /
  zh-Hans: 60 readout cells, every one clearing its widest reachable string,
  minimum slack 2.89 px (`+24.0` in VU). No element on the page reports
  horizontal overflow and the document stays 380 × 500.
- Built Release VST3 + AU, installed, auval PASS 1.9.2.

## [1.9.1] - 2026-09-12

**First GitHub release of O-Gain.** No earlier version was ever tagged, so this
build is the first one published with macOS, Windows and Linux binaries. The
1.9.1 change itself is cosmetic (below); what the release carries is the whole
1.x line:

- **1.0.0 – 1.2.1** — the stereo gain-staging utility: phase invert L/R,
  channel swap, mono sum, M/S encode/decode, BS.1770 K-weighted LUFS "Learn"
  auto-gain, VU / peak / RMS metering; two code-review remediation passes
  (working mono support) and a Learn-mode safety fix.
- **1.3.0 – 1.4.0** — French and Simplified Chinese UI with a rewritten
  hover-help layer.
- **1.5.0 – 1.6.0** — the meter shows the pair: a held peak over the average
  bar in every mode, post-gain VU ballistics, and a LUFS mode that measures
  LUFS on the output (it drew RMS whenever Learn was idle).
- **1.7.0 – 1.8.1** — six target-level presets, the three dials on one row,
  readout and layout legibility fixes.
- **1.9.0** — Peak | RMS | VU | LUFS method strip with live In / Out readouts,
  and a mode-aware ruler (dBFS band and bus line in Peak/RMS only, VU scale
  with 0 VU = -18 dBFS, LU scale against the Target with a target line on the
  output bars).

Cosmetic. PATCH: page CSS only — no parameter, range, type or state format
changed, no C++ touched, audio path untouched.

### Changed

- **Measure / Meter captions black.** `.mode-label` was the `#8B7355` tan
  shared with the selector borders; it is now `#000000`.
- **Settings cog black.** `.gear-btn` glyph and ring were the same tan; both
  are now `#000000`. Hover no longer swaps to the `#5C4033` brown (that would
  read as lighter than the new black) and instead tints the disc with an 8 %
  black wash. The open state (`aria-expanded="true"`, green) and the
  focus ring are unchanged.

### Testing

- Built Release VST3 + AU, installed, auval PASS 1.9.1.

## [1.9.0] - 2026-09-12

The four meter methods now look different from each other. MINOR: page,
controller and i18n table only — no parameter, range, type or state format
changed, no C++ touched, and the audio path is untouched. Every v1.x session
and preset loads identically.

### Added

- **Method strip.** A Peak | RMS | VU | LUFS row of live In / Out readouts
  under the target presets, the column the bars are drawing lit. Every value
  was already in the `updateMeters` payload each frame (`inputPeak*`,
  `inputRms*`, `vuLevel*`, `momentaryLufsIn` and their output pairs); the
  page simply never showed more than the one the mode selected. Peak and RMS
  read in dBFS, VU in VU against `VU_REFERENCE_DBFS` (0 VU = -18 dBFS, the
  line the v1.5.0 staging band has always sat on) with its sign, LUFS in
  LUFS. 32 px tall (three 10 px lines, two 1 px gaps) in the 220 px centre
  column, inside the 78 px of slack the stack had with the Learn panel open,
  so the meter columns keep their height in every state. Tooltip
  `method-strip` (en / fr / zh-Hans), bound to `#method-strip`.
- **Mode-aware ruler and marks.** `applyMeterMode()` writes `mode-dbfs` /
  `mode-vu` / `mode-lufs` on `.meter-section` from `meter_mode` and rewrites
  the seven gutter numerals from each span's new `data-db`:
  - Peak and RMS: the dBFS ruler as before, with the -18 to -12 dBFS staging
    band and the -6 dBFS bus line. These two marks are dBFS marks and now
    render **only** here; through v1.8.1 they were drawn under a LUFS ruler
    too, where "-18" is -18 LUFS.
  - VU: the ruler reads in VU (+18 at the top, 0 on the old -18 line, -42 at
    the floor) and a dashed 0 VU reference line replaces the band.
  - LUFS: the ruler reads in LU against `target_level` (0 LU is the Target)
    and a cream target line with a dark outline is drawn on both output bars
    at the Target's own height. `watchTargetLevel()` mirrors the slider
    through `getScaledValue()` (the range the backend pushed, not the JS
    table — pattern_webview_knob_readout_scaled_value) so the ruler and the
    line follow a knob drag or a preset click. Numerals are rounded to whole
    units: the gutter is 14 px and "+16.5" would not fit; every preset
    target is a whole number anyway.
  - The bar range itself stays -60..0 dBFS in every mode, so the held peak
    cap, the clip strip and the bar are on one ruler as before.

### Changed

- **The second readout caption under each column is the mode's name.** It
  read `avg` in all four modes, so VU, RMS and LUFS were indistinguishable on
  the readout, LUFS carried no unit at all, and in Peak mode "avg" was the
  peak repeated. It now reads `RMS` / `VU` / `LUFS` (the `meter_mode` option
  string verbatim, exempt under D-01, written by `applyMeterMode()` and never
  keyed) with the value in that unit — VU signed — and in Peak mode the row
  shows RMS rather than the peak twice.
- The `input-meter`, `output-meter` and `meter-mode` tooltips describe the
  per-mode marks and units (en rewritten; fr re-drafted at `reviewed: false`;
  zh-Hans at `'mt'`).

### Removed

- **`label.avg`** (en / fr / zh-Hans) — nothing references it after the
  caption change, and check-i18n [15] rejects a dead key.

### Testing

- `scripts/check-i18n.js --plugin O-Gain`: ALL CHECKS PASS.
- `scripts/i18n-fr-lint.js` / `i18n-zh-lint.js --plugin O-Gain`: CLEAN /
  0 findings (9 entries at `'mt'`, counted not failed).
- `scripts/check-ui-labels.js --plugin O-Gain`: ALL CHECKS PASSED at
  380 x 500 across eleven states — the seven existing plus Peak, RMS and VU
  meter modes and LUFS with the -23 preset lit (`tests/i18n-states.json`),
  32 of 32 `[data-i18n]` elements visible, 0 moved elements on the fr and
  zh arms. The strip's caption column is pinned at 28 px for the same reason
  `.meter-db-label`'s is (assertion 7).
- Rendered through `scripts/serve-ui.js` in all four modes and with the
  Learn panel open: document scroll extent 380 x 500 throughout, meter body
  347 px (279 px with Learn open) exactly as at v1.8.1. The first draft's
  green target line vanished the moment the green bar reached it; it is
  cream with a dark outline.

## [1.8.1] - 2026-09-12

Three small placement and legibility fixes. PATCH: CSS only, no markup,
no parameter, no state change, no C++.

### Fixed

- **Presets title centred** over its two rows (`text-align: center` on the
  title, not `align-items` on the block, which would shrink the rows to
  content).
- **Peak / avg meter readouts larger**: `.meter-db-label` 8 → 10 px, the
  captions in the v1.8.0 brown `#5C4033` and the values semibold `#3C2F2F`.
  The caption column is re-pinned 20 → 25 px for the larger face (fr
  `crête`, the widest, scales from 18.41 to about 23 px), leaving the value
  cell 30 px against `-60.0` at roughly 22 px. Both line-height pins go
  1.125 → 1.1 (11 / 10 px, the `.settings-label` derivation).
- **Measure / Meter row and the Learn Analysis panel centred.** Both are
  300 px blocks inside the 364 px `.container`, which is a column flex with
  default `align-items: stretch` and so placed them at the left edge.
  `align-self: center` on each.

### Testing

- `scripts/check-ui-labels.js --plugin O-Gain`: ALL CHECKS PASSED at
  380 x 500, all seven states; no readout cell overflows its column in
  English, French or zh-Hans.
- Rendered through `scripts/serve-ui.js` with the Learn panel open: the
  meter section shortened 2 px for the taller readout row, and the centre
  stack still clears the panel.

## [1.8.0] - 2026-09-12

Dial legibility and placement. MINOR: the centre stack is re-laid, two
dead localisation entries are retired, and nothing else — no parameter,
range, type or state format changed, no C++ touched, and the audio path is
untouched. Every v1.x session and preset loads identically.

### Changed

- **The three dials share one row.** v1.7.0 spread them over three: Gain
  and Trim side by side but vertically centred, so a 48 px dial and a
  36 px dial lined up on nothing; Target alone on a third row to the left
  of its caption; and the Target readout one row above its dial, beside
  LEARN. Now Trim | Gain | Target sit in the 220 px column the display and
  the preset block already use, the large dial in the middle, top-aligned.
  The two small dials carry a 6 px vertical margin so every dial box is
  48 px, which is what puts the three captions on one baseline and the two
  readouts on the next without a wrapper element. LEARN is alone on its row
  and centred; its 121 px pin (v1.3.0) stays, for the same reason it was
  set.
- **Captions darker and larger**: `.knob-label` 9 → 10 px, `#8B7355`
  (the tan the paper texture swallows) → `#5C4033` (the brown the buttons
  already use), pinned at 1.1 like `.settings-label`, `white-space: nowrap`
  so a caption that wrapped in one language only could not move the readout
  under it. The preset unit captions (`dBFS` / `LUFS`) go 8 → 9 px in the
  same brown, and the `Gain Offset` display caption takes the brown at its
  existing size.
- **Readouts darker and larger**: `.knob-value` 10 → 11 px, weight 600,
  `#3C2F2F`, pinned at 1.0909 like `.learn-btn`. The Target readout now
  uses this class; `.target-group`, `.target-label`, `.target-value` and
  `.target-knob-row` are gone.

### Removed

- **`label.targetLevel`** (en / fr / zh-Hans) — the caption under the
  Target dial is `label.target` (`Target` / `Cible` / `目标`), which fits
  the 65 px column in all three languages where `Target Level` did not.
  check-i18n [15] rejects a key nothing references, so the key is deleted
  rather than left.
- **The `target-group` tooltip and its `#target-group` binding** — the
  element it anchored to no longer exists. Its loudness references
  (-18 dB = 0 VU, -14 LUFS Spotify, -23 LUFS EBU R128) are already in the
  `target-presets` tooltip body.

### Testing

- `scripts/check-i18n.js --plugin O-Gain`: ALL CHECKS PASS.
- `scripts/i18n-fr-lint.js` / `i18n-zh-lint.js`: CLEAN / 0 findings.
- `scripts/check-ui-labels.js --plugin O-Gain`: ALL CHECKS PASSED at
  380 x 500, all seven states in `tests/i18n-states.json`, 32 / 32
  `[data-i18n]` elements visible; assertion 7 reports no non-label element
  moved between English, French and zh-Hans.
- Rendered at 380 x 500 through `scripts/serve-ui.js` before and after:
  knob row 180.1 → 255.1 px, LEARN 261.1 → 287.1, presets 293.1 → 346.1,
  inside the 383 px centre column.

## [1.7.0] - 2026-09-12

Target-level presets. MINOR: one new block of six buttons in the editor and
nothing else — no parameter, range, type or state format changed, no C++
touched, and the audio path is untouched. Every v1.x session and preset loads
identically.

### Added

- **Target presets under the Target Level knob**: two segmented rows,
  `dBFS -18 / -16 / -20` and `LUFS -14 / -16 / -23`, the six references
  BRIEF.md's "Target level selector with presets" named for v1.0 and which
  never shipped — only the continuous knob did. Each button writes
  `target_level` through the knob's own `WebSliderRelay` / attachment as a
  gesture (drag-started, one value, drag-ended), so the host records it the
  way it records a knob drag. The block owns no parameter.
- **The lit preset is the current value.** Matching reads
  `getScaledValue()` — the value the C++ `NormalisableRange` holds, not the
  JS `paramDefs` table — with a tolerance of 0.05 dB, half the parameter's
  0.1 dB step, so a knob nudged one step off a preset goes dark. `-16` sits in
  both rows and both light together: the parameter holds one number, and
  which unit the user reads it in is the Measure selector's business.
- **Writes go through `state.properties`** (start / end / skew pushed by the
  backend), not a JS constant, for the same reason
  (`pattern_webview_knob_readout_scaled_value`).
- **`label.presets`** keyed in en / fr / zh-Hans (`Presets` / `Préréglages` /
  `预设`) and a `target-presets` tooltip on `#target-preset-row` naming what
  each of the six values is for. The numerals are READOUT-class text and the
  row captions are the unit symbols `dBFS` / `LUFS` (D-03), so the title is
  the block's only keyed string. fr `reviewed: false`, zh-Hans `'mt'`.
- **`tests/ui-stub/generic-overrides.json`**: the three sliders' real ranges.
  The generic stub seeds every slider 0..1 linear, under which a preset click
  stored 0 and nothing could light — the new gate state would have measured a
  page in which the feature cannot work.
- **Gate state `target preset -23 LUFS lit`** in `tests/i18n-states.json`.

### Layout

- The block reuses `.mode-selector` / `.mode-option`, is the same 220 px
  column as every other centre-stack row, and its unit caption is a fixed
  30 px flex basis so no caption can reposition a selector. Measured at
  380 x 500 through the gate's own boot: 52 px tall, identical rectangles in
  en / fr / zh-Hans, bottom edge 325.6 against the centre stack's 351.1 with
  the Learn panel open. `check-ui-labels` 0 FAIL, 33 of 33 labels visible
  across 8 states; `check-i18n`, `i18n-fr-lint`, `i18n-zh-lint` clean.

### Verified

- Headless click probe against the seeded stub: -18 lit at boot, the -23
  LUFS click writes -23.0 dB and lights that button, -16 lights both rows,
  a 0.1 dB nudge darkens all six. pluginval strictness 5 SUCCESS on the
  installed VST3; auval PASS on `aufx OGan OuDv` (Component Version 1.7.0).

## [1.6.0] - 2026-09-12

The LUFS meter mode shows LUFS. MINOR: one new continuous measurement pair and
a second K-weight chain on the post-gain buffer. No parameter, range, type or
state format changed, and the audio path is untouched — every v1.x session and
preset loads identically.

### Fixed

- **LUFS meter mode drew RMS under a LUFS label whenever Learn was idle — on
  both columns, which is nearly always.** The BS.1770 K-weight chain ran only
  inside the `isLearnActive` branch of STEP 4, and the sole loudness value the
  editor pushed was the Learn snapshot's momentary field, which sits at -100
  outside a Learn. `case 3` in app.js therefore fell back to `inputRms*` on the
  input column, and the output column was RMS by design because the plugin had
  never measured post-gain loudness. Root cause: a meter mode wired to a
  measurement that existed only during a different feature's window. Fix: the
  chain runs on every block, and there are now two of them.

### Added

- **`momentaryLufsIn` / `momentaryLufsOut`**, published every 100 ms hop in
  LUFS (not amplitude; -100 is the silence floor), Learn or no Learn. Both
  columns of the LUFS mode read them and nothing else — the RMS fallback is
  gone, and one value drives both bars of a column because loudness is not
  per-channel.
- **`MomentaryLoudnessMeter`**: the two K-weight stages per channel plus the
  four rotating 100 ms sub-block accumulators, as a struct, because there are
  two instances — `inputLoudness` on the post-utilities/pre-gain buffer (which
  Learn also reads) and `outputLoudness` on the post-gain buffer (STEP 8, new).
  They are prepared with the same spec, coefficients and hop, so their windows
  close on the same sample and the two columns always describe the same 400 ms.
- **Learn consumes the input meter's closed blocks** in the new `onLearnHop()`
  rather than owning the filters: gating store, short-term, integrated,
  confidence and the seqlock publish are unchanged line for line, only moved.
  The Learn panel and the LUFS column are the SAME measurement, not two chains
  that could disagree.
- **`tests/lufs-harness/`**, an offline console target built with the shared
  processor-console helper (`OUARICON_BUILD_TESTS=ON`, `ninja O-Gain-lufs-test`,
  JUCE_WEB_BROWSER=0, no editor TU). Stereo pink noise is calibrated to
  -18.00 LUFS by an INDEPENDENT BS.1770 implementation (ITU-R BS.1770-4 Table
  1/2 coefficients typed from the Recommendation, not read from the plugin's
  `KWeight` namespace) and run with Learn idle at 48 kHz / 480-sample blocks.
- **`createEditor()` and the editor include are guarded on `JUCE_WEB_BROWSER`**
  so the console target links (pattern_render_harness_breaks_on_webview_editor).

### Changed

- **`resetLearnAccumulators()` no longer resets the K-weight filter state.**
  It restarts the input meter's 400 ms window so the first block Learn
  accumulates lies wholly inside the Learn window (the v1.5.0 semantics, kept),
  but the filters keep their steady-state history of the very signal being
  measured. v1.5.0 reset them, which put a start-up transient into the first
  gating block. The continuous readout holds its last value for one window at
  Learn start.
- **`powerToLufs()` is FLOORED at -100, not merely guarded against zero.**
  After the signal stops the IIR tails leave a vanishing but nonzero power, and
  the unfloored formula published -1265 LUFS (seen in the harness's silence
  phase before the floor went in). The page already treats <= -99 as -inf, so
  nothing was visible, but a published value should not be absurd.
- **The meter-mode tooltip** in en / fr / zh-Hans no longer promises the RMS
  fallback: "LUFS = K-weighted momentary loudness (400ms window), measured
  continuously on both columns".
- **`tests/i18n-states.json`**: all six `updateMeters` payloads carry the new
  pair, and a seventh state clicks the LUFS meter button with Learn idle and
  pushes -18 / -12, so the fixtures drive the code this version ships.

### Verified

- `O-Gain-lufs-test`: **ALL GATES PASSED, 11/11.** With Learn idle
  (`learnActive` false, snapshot state 0 throughout) and gain 0 dB, over 12 s
  of -18 LUFS pink noise: input column max |dev| **0.199 dB** over 1150 blocks
  (mean -18.00 LUFS), output column max |dev| **0.199 dB**, max |in - out|
  **0.0000 dB**; the plugin's 115 closed windows match the independent reference
  on the same hop grid to **0.0000 dB**. The reading is a loudness and not the
  RMS it replaced: mean LUFS sits **+2.52 dB** above the loudest channel's RMS
  in dBFS on the same signal (gate: > 0.5). At `gain_offset` +6 dB the output
  column reads -12.00 LUFS (max |dev| 0.199 dB) while the input stays at -18.
  Nothing is published before the first full window (block 39: -100 / -100;
  block 40: -18.05 / -18.19 — the 0.14 dB gap on the very first window is the
  gain stage's pre-existing 20 ms fade-in from `reset()`, excluded by the 0.5 s
  settle). One second of silence: both columns exactly -100.
- `check-ui-labels --plugin O-Gain`: **ALL CHECKS PASSED** across en / fr /
  zh-Hans at 380 x 500, default + 6 states from `tests/i18n-states.json`, 32 of
  32 `[data-i18n]` elements visible, no uncaught page error, every requested
  resource served.
- `check-i18n` all checks pass; `i18n-fr-lint` **CLEAN, exit 0**;
  `i18n-zh-lint` **0 findings, exit 0**.
- `auval -v aufx OGan OuDv` (the dev-branded triple this checkout installs):
  **AU VALIDATION SUCCEEDED**, Component Version 1.6.0 (0x10600), after
  `build-and-install.sh` swept both variants and cleared the AU cache.

### Below the ship bar, stated rather than buried

- **The rewritten French string is at `reviewed: false` and the Chinese one at
  `reviewed: 'mt'`** — the developer re-reads the French; the zh lint counts the
  mt row (`BELOW SHIP BAR: 3`, the two v1.5.0 rows plus this one) and passes.
- **Mono in LUFS mode** feeds channel 0 into the L side only, R stays zero, so
  L + R collapses to single-channel loudness (WR-02) — the same rule Learn has
  used since v1.1.0, now shared by construction. Not exercised by the harness,
  which runs stereo.

## [1.5.0] - 2026-09-12

The meter learns to show the PAIR. MINOR: new UI surfaces, one new DSP
measurement (post-gain VU ballistics), a 30 px wider frame. No parameter, range,
type or state format changed, and the audio path is untouched — every v1.x
session and preset loads identically.

### Added

- **A held peak riding over the average bar, in every meter mode.** Through
  v1.4.0 one bar carried whichever single reading `meter_mode` selected, so the
  peak and the average were never on screen at the same time — the one
  comparison a level meter exists to support. The cap reads the SAME decayed
  peak the clip strip already used (a ~300 ms per-block decay computed in the
  processor), so **no new measurement was added for it**: the value was being
  sent every frame and only the clip strip read it. In Peak mode the cap sits
  exactly on the bar top, which is the honest reading rather than a redundancy
  worth hiding.
- **A dB scale gutter on each meter column**, 14 px, on the OUTER edge of each
  (left of the input bars, right of the output bars) so neither scale is
  stranded in the middle of the window beside the gain readout. Seven gridlines
  — 0, -6, -12, -18, -24, -36, -60 — positioned off the meter's own
  `METER_DB_MIN` = -60 linear mapping, so a gridline and the bar beside it
  cannot disagree.
- **The staging targets drawn ON the bar**: a shaded band from -18 to -12 dBFS
  and a line at -6 dBFS. Both are z-indexed ABOVE `.meter-bar`, deliberately.
  Painted behind it they would be visible only while the signal sits below
  them, which is exactly the moment a staging target does not matter — the
  reading a user needs is "the bar is IN the band", and that is unreadable if
  arriving at the band erases the band.
- **A peak-versus-average readout pair under each column**, replacing the single
  rounded number. Both now show one decimal.
- **`vuLevelOutL` / `vuLevelOutR` and a second pair of `BallisticsFilter`s**
  (STEP 7 in `processBlock`, the mirror of STEP 3 on the post-gain buffer),
  configured IDENTICALLY to the input pair — identical is the point, since the
  two columns are read against each other and any difference in attack, release
  or level-calculation type would surface as a gain error that is not there.
  Mono mirrors into both meters exactly as STEP 3 does (WR-02).

### Fixed

- **VU mode compared a 300 ms ballistic input against a per-block RMS output.**
  `case 2` drove the input bars from `vuLevel*` and the output bars from
  `outputRms*` — two different integrations of the same signal, in the DEFAULT
  meter mode, on the one plugin whose job is confirming that a chain passes at
  unity. A transient therefore read as a gain error that was not there. The
  output pair is now the post-gain VU.
- **The meter mode was never SEEDED from its parameter, only listened to.**
  `currentMeterMode` was declared as a hard-coded `2` (VU) and `watchMeterMode()`
  registered two LISTENERS — which fire on a change, and opening an editor is not
  a change. So a session saved in Peak, RMS or LUFS reopened with the mode
  buttons showing the saved mode and the bars drawing VU: the control and the
  meter disagreeing about the same setting, silently, until the user's first
  click. Pre-existing since the mode selector landed; found at v1.5.0 by driving
  the page headless, where the stub seeded `meter_mode = Peak`, the Peak button
  lit, and the bars kept painting the VU value. One line: read
  `state.getChoiceIndex()` before attaching the listeners.
- **`ampToDb` now rejects undefined and NaN, not just silence.** The guard was
  `amp <= 0.00001`, which is false for `undefined`, so any payload missing a
  field reached `Math.log10` and painted every bar `NaN%` tall. Written as
  `!(amp > 0.00001)` it catches both. This is not hypothetical: this version's
  own `tests/i18n-states.json` fixtures predated the output-VU pair.

### Changed

- **The editor frame is 350 x 500 -> 380 x 500** and the meter columns 44 -> 58
  px. The extra 30 px is the two 14 px gutters plus 2 px landing in the centre
  column. It is NOT cosmetic: `.learn-section` is pinned at 121 + 8 + 91 = 220
  px exactly, so taking the gutters out of the centre instead would have
  overflowed that row. Both UI gates parse `setSize` for their viewport, so they
  re-measure at the new frame with no argument of their own.
- **`.meter-pair` went from `width: 100%` to `min-width: 0`.** It used to be a
  direct column child, where 100% meant "the column"; it is now a flex item in
  a row, where a 100% basis is the parent's FULL width and the gutter beside it
  would push the pair into overflow.
- **`label.pk` / `label.avg` are KEYED, not exempt**, and the distinction is
  worth recording because this page carries an `I18N_EXEMPT` entry for the
  string `Peak`. That exemption is the `meter_mode` OPTION STRING, matched byte
  for byte and capitalised. These are lowercase captions the page invented for
  its own readout; they never reached the host and no automation lane has shown
  them. Exemption matching is `e.text === text` — case sensitive — so `peak` is
  not silenced by `Peak`, and assertion 10 demands a key for it. It has one.
- **The seven scale numerals are exempt WITH A SCOPE** (`.meter-scale`). An
  unscoped entry would have silenced assertion 10 for any future caption
  anywhere on the page that happened to read `0`, which is the precise hole the
  scope argument was added to close.
- **Both meter tooltips were rewritten in all three languages** to name the
  bar, the cap, the band and the -6 line, since a tip that describes a meter
  the page no longer draws is worse than no tip.

### Caught by the gates before shipping, and recorded

- **`grid-template-columns: auto 1fr` let the French caption move the value
  cell.** Column one was sized from its content — a `[data-i18n]` element — so
  `#input-db-peak`, `#input-db-avg`, `#output-db-peak` and `#output-db-avg` each
  moved `dx=0.8 dw=-0.8` between English and French. check-ui-labels assertion 7
  named all four. The column is now a hard 20 px against measured captions of
  **en 17.56 / fr 18.41 / zh 16.77 px** (rendered in this element at 8px with
  its 0.2px letter-spacing) — the widest cleared by 1.59 px, and the row's
  geometry is language-invariant by construction, which is the only thing
  assertion 7 accepts.
- **The two new leaves shipped without line-height pins and the Chinese arm
  cascaded.** `line-height: normal` is whatever the RESOLVED FACE reports, and
  PingFang SC reports a taller box than Georgia: both leaves grew 9 -> 11 px,
  `.meter-db-label` went 18 -> 22 px, the `flex: 1` meter body above it
  shortened, and **34 elements moved**. Each leaf now carries a pin derived from
  its OWN measured English box (9.00 px, no padding, no border, over 8px) =
  **1.125**, unitless — the same derivation as the v1.4.0 pin block.

### Verified

- `check-ui-labels --plugin O-Gain`: **ALL CHECKS PASSED**, 255 assertions, 0
  FAIL, across the en / fr / zh-Hans arms at 380 x 500; 32 of 32 `[data-i18n]`
  elements visible in at least one state; no uncaught page error; every
  requested resource served.
- `check-i18n` all checks pass; `i18n-fr-lint` **CLEAN, exit 0**;
  `i18n-zh-lint` **0 findings, exit 0**.
- `tests/i18n-states.json`: all six `updateMeters` payloads carry the new output
  VU pair, so the fixtures drive the code this version actually ships.

### Below the ship bar, stated rather than buried

- **The two new Chinese strings are at `reviewed: 'mt'`** — machine draft, no
  blind back-translation run on them. `i18n-zh-lint` counts them
  (`BELOW SHIP BAR: 2`) and passes; every other zh row on this page remains at
  `'bt'`. The two new French strings are at `reviewed: false` for the same
  reason: no QA pass has read them.

## [1.4.0] - 2026-09-05

Simplified Chinese joins English and French (task 260905-rwh, Stage 4 wave 4c).
MINOR: a third language, one font edit and eleven geometry pins. No parameter,
range, type or state format changed, and no audio path was touched.

### Added

- **`zh-Hans` on every one of the 89 emitter rows** — 26 `I18N` entries (title
  and body) and 37 `LABELS` entries. Authored at `reviewed: 'mt'`, committed
  there, and promoted to **`reviewed: 'bt'`** only after an independent blind
  reverse read. `'native'` stays open on every row: this project has no native
  Chinese reader, and `i18n-zh-lint` rule R1 prints that quality level on every
  run rather than leaving it to a reader's memory.
- **The endonym `简体中文`**, written as numeric character references to match
  this file's existing convention for `Français`.
- **A three-way `languageCode` / `languageIndex` codec.** Anything that is
  neither `fr` nor `zh-Hans` degrades to English. **No Chinese character appears
  anywhere under `Source/`** — the C++ carries only the ASCII code.

### Changed

- **The language hover-help lost its enumeration, in English and French alike.**
  It named the selector's options in full, which was true for exactly as long as
  the selector held two entries. Deleted rather than widened to three: a body
  that lists a control's options has to be re-edited every time the control
  grows, in every language, and it is that edit that gets forgotten. The
  selector already lists the languages in their own endonyms — the one place the
  list cannot go stale. The Chinese was authored from the corrected English.
- **`Measure` diverges from the glossary root, on the record.** The suite
  glossary settles `measure` as the musical **bar**. This control selects the
  measurement *algorithm* Learn uses, so the settled root would have printed a
  bar line on a loudness meter. It renders 测量 under a `termNote`, which is the
  lint's own sanctioned escape — and the blind reverse reader, who had never
  seen the English, read 测量 back as "Measure".
- **One font edit covers the whole page.** Every `font-family` here other than
  the one at `index.html:43` reads `inherit`, so the CJK tail lands once —
  **before the trailing generic**, because Chromium resolves a bare `serif`
  against the document's `lang` and a tail written after it is never consulted.
  No face had to be named: Garamond is not installed on the build machine
  (measured, 0 family matches) but Georgia and Times New Roman both are.
- **Eleven `line-height` pins.** `line-height: normal` is whatever the resolved
  face reports as its natural line box, and the resolved face changes with the
  document language — so every Han-carrying leaf grew 2–3 px and the growth
  cascaded in *both* directions on a 350 × 500 frame: the meter section lost 8 px
  of height while the utility row gained 6, moving 52 elements. Each pin is
  derived from that leaf's own measured English box, unitless, never global.
  `.utility-btn` needed the pin derived **per line**: seven buttons share a
  334 px flex row, so their captions already wrap in English and the measured box
  is two line boxes, not one.

### Deliberately unchanged, and recorded as checked

- **The gear hover-help is CORRECT and is untouched.** It names both of the
  popover's controls — the language and the hover-help switch — and says which of
  the two is remembered. Four of this wave's six plugins carry a gear body
  claiming the panel holds nothing but the language; this is one of the two that
  does not. Read in full, in both languages, and recorded as a checked
  non-defect.
- **No `tests/ui_tip_render_check.js` was written.** This plugin has never had
  one; writing a render gate is a separate pass with its own budget, and a gate
  invented mid-localization is a gate nobody has calibrated.

### Verified

- `check-i18n` exit 0; `check-ui-labels` exit 0 with **0 FAIL on the English,
  French and Chinese arms**; `i18n-zh-lint` 0 findings and `BELOW SHIP BAR 0`;
  `i18n-fr-lint` exit 0 — French untouched.
- `measure-ui --mode box --report all` reads **0 on all four screens beside 46
  visible Han-bearing nodes, none of which resolves to a non-CJK face**. A zero
  from a Han-gated screen run before the table lands is not a zero at all.
- Zero Han under `Source/**/*.{h,cpp}`, positive control fired on the same run.
- `auval -v` AU VALIDATION SUCCEEDED, triple read off `auval -a`.

## [1.3.3] - 2026-09-03

The French rendering of the hover-help surface changes suite-wide, and O-Gain
is the tracer. PATCH: seven French strings and two width comments; no
parameter, range, type or state format changed.

### Changed

- **The French caption is now `Infobulles`** (plural), replacing
  `Aide au survol`. The old term named the ACTION — help on hover; *infobulle*
  is the noun French DAW and OS interfaces use for the surface itself. The
  glossary root moved with it, ROOT-ONLY: `scripts/i18n-fr-glossary.js` now
  reads `'hover help': ['infobulles']` and
  `'toggle hover help': ['activer ou désactiver les infobulles']`, with the old
  rendering REMOVED rather than kept as an accepted alternate — so a plugin
  drifting back is a red G1 gate, not a silent pass.
- **Every sentence re-agreed from feminine singular to feminine plural.** All
  seven French sites in `js/i18n.js`:
  `label.hoverHelp` and the `tips-toggle` title → `Infobulles`;
  `aria.helpToggle` → `Activer ou désactiver les infobulles`;
  `tips-toggle` body → *Active ou désactive **ces infobulles**. Une fois
  **désactivées**, seuls l'engrenage et cet interrupteur continuent de
  s'expliquer.*; `lang-select` body → *La langue de **ces infobulles** et des
  libellés de la page…*; `settings` body → *…et l'affichage **des
  infobulles**. La langue est conservée avec la session ; l'interrupteur **des
  infobulles** ne l'est pas.*
- **The positive control fired before any plugin file was touched.** With the
  glossary changed and all 43 plugins still on the old wording,
  `node scripts/i18n-fr-lint.js --plugin O-Gain` exited **2** with 3 G1
  findings, and the repo-wide `node scripts/i18n-fr-lint.js` exited **2** with
  **117 G1 findings across 43 / 43 plugins**. G1 is therefore proven to be
  reading the new root, not decorating a pass.

### Fixed

- **Two stale width comments re-measured, not scaled.** Both cited the old
  caption's rendered width and would have become false the moment it changed.
  `index.html` (`.settings-popover` width table) and the `js/i18n.js` header
  note now read from `check-ui-labels --plugin O-Gain --verbose`:
  `Infobulles` renders **49.69 px** against the old caption's 70.22, so the
  hover-help row falls from 132.22 to **111.69** in a 154 px content box and
  goes from the WIDEST French row to the narrowest of the four. The widest row
  is now the LANGUAGE row in both languages (en 124.8, slack 29.2; fr 113.67,
  slack 40.33), and the panel's tightest fit is 29.2 px rather than 21.78.
  `Infobulles` is 4.39 px NARROWER than English `Hover help` (54.08), reversing
  a caption that used to be 16.14 px wider.
- **The `.settings-popover` width pin is unchanged and stays.** It exists so
  the panel's rect cannot move with language in EITHER direction; a caption
  that happens to be short today is not a reason to retire it.

## [1.3.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide
i18n rollout. PATCH: no parameter, range, type or state format changed; one
tooltip body in two languages.

### Fixed

- **item 37 — Confidence tooltip (`info-confidence`):** the body stated only
  the 5 s / 15 s elapsed-time thresholds. The gate at
  `PluginProcessor.cpp:1027` also holds the verdict at LOW while fewer than
  `kConfidenceMinBlocks` = 50 gating blocks (`:133`) have been analysed,
  whatever the elapsed time. A gating block is 400 ms of K-weighted power
  stored once per 100 ms hop (`:947-978`), so the 50th arrives at ~5.3 s —
  after the 5 s clock — and the block count is the condition that actually
  governs. Both bodies now read: Low = under 5 s, **or fewer than 50 analysis
  blocks (400 ms each, one every 100 ms) whatever the time elapsed**; Medium =
  5–15 s; High = over 15 s. The former "High … with stable signal" lost its
  last clause: nothing about signal stability feeds `confidence` in the source.
  The French entry is `reviewed: false` again. Tip height at 350×500 read
  before and after through the page's own renderer: 67.64 px (3 lines) in both
  languages → 81.14 px en (4 lines) / 94.64 px fr (5 lines), placed above the
  anchor with 334 px of clearance to the frame top in French; no anchor
  overlap, no tip off-frame across all 26 anchors.

## [1.3.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout. PATCH: no
parameter, range, type or state format changed; only French strings, and only
their values.

### Changed

- **19 French entries revised** against the suite glossary and lint, out of 63.
  Terminology: the Learn confidence verdict reads **Faible / Moyen / Élevé**
  rather than Bas / Moy / Haut (French says *confiance faible*, not *confiance
  basse*), and the tooltip that explains it now uses the same three words the
  cell shows; the settings caption is **Aide au survol** rather than the
  abbreviated "Aide", which also ends a disagreement with the tip title on the
  same control; the hover-help switch's accessible name is **Activer ou
  désactiver l'aide au survol**; one French word for "switch" (*interrupteur*)
  where two were in use. Typography: 19 no-break spaces before a colon, before
  a semicolon and between a number and its unit, and 7 typographic minus signs
  (−40 à +40 dB) in place of hyphens. Grammar and idiom: *recommandée* agreeing
  with *sonie*, *norme de l'industrie* for "industry standard", and *par paire*
  for the pleonastic *comme paire appariée*.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

## [1.3.0] - 2026-08-28

The page speaks French, not only the hover help — and the tooltip layer is
replaced rather than translated. Stage J of the repo-wide i18n task (canon v2),
plugin 7 of 7, on the narrowest frame in the suite. MINOR: no parameter IDs,
ranges, types or state format changed; existing sessions and presets load
unchanged, and a session saved before this version simply opens in English.

### Added

- **English + French across every visible string on the page.** 37 label keys
  and 26 tooltip keys in a new `Source/ui/public/js/i18n.js`. Meter captions,
  the gain-offset caption, knob captions, mode-selector captions, the whole
  Learn Analysis panel, the seven utility buttons, the Learn button's four
  faces, the confidence verdict and three accessible names all switch language
  with no reload and no English survivor. Counts parsed out of the RENDERED
  DOM, not grepped: 23 live tooltip anchors (23 unique strings) and 40 text
  nodes, against a plan figure of 23 / 41 and a raw grep of 34 attribute-token
  hits.
- **A settings popover with the language selector**, in the exact absolute slot
  the "?" help button occupied through v1.2.1 (`right: 8px; top: 0` inside the
  22 px header), so nothing on a 350 x 500 layout had to move to make room for
  it. The hover-help switch moves inside it. Every colour, border, radius and
  transition on the gear is the "?" button's own, carried across unchanged.
- **`getUiLanguage` / `setUiLanguage` native functions and session
  persistence.** This plugin had no bridge beyond `toggleLearn`, so the pair is
  new. The choice rides the session XML as a plain `uiLanguage` attribute —
  deliberately NOT an `AudioParameterChoice`, so it never appears in a host
  automation lane and no preset can change which language somebody reads their
  interface in. Read back with `isVoid()` and `toString()`, because a
  ValueTree property restored from XML is a string `var` whatever it was
  written as.
- `tests/i18n-states.json`, driving the settings popover and four Learn states
  so all 28 keyed elements — including the three confidence verdicts and all
  four Learn-button faces — are measured at 350 x 500 rather than shipped
  unmeasured.

### Changed

- **The hover help is the measure-then-pin renderer, ported from
  O-ReverseDelay. There is now ONE tooltip renderer repo-wide.** v1.2.1 had no
  tooltip JavaScript at all: its help was a pure-CSS `[data-tooltip]::after`
  pseudo-element with `content: attr(data-tooltip)` and three hand-picked
  direction override classes. It could not be measured, flipped, clamped or
  re-aimed. The port brings a title/body pair, a 120 ms dwell delay, a width
  RELEASED then MEASURED then PINNED before `left` is applied, an automatic
  vertical flip, a horizontal clamp, and an arrow offset recomputed AFTER the
  clamp so a clamped tip still points at its control. The old layer is DELETED,
  not disabled — `grep -rn 'tooltipHeight\|tooltipWidth\|data-tooltip'
  Source/ui/public/` returns nothing outside comments.
- **The 447-line inline `<script type="module">` is extracted to `js/app.js`.**
  Behaviour moved, not rewritten: the same listeners in the same order. Only
  the import specifier changed, and only because the module's depth did —
  `./js/juce/index.js` became `./juce/index.js`.
- **The Learn button's width is pinned to 121 px and the Target block to
  91 px.** `.learn-section` is 220 px of intrinsic-width button plus a `flex: 1`
  target group, so the button's own text positioned everything to its right.
  That was already a defect in English: the four faces measure LEARN 80.8,
  LEARNING... 120.7, DONE 73.4 and TOO QUIET 109.9 px, so the Target caption
  and its readout slid up to 40 px sideways every time Learn ran or finished.
  121 px is the widest ENGLISH face rounded up, so no English face re-wraps.
  Visible cost, stated rather than hidden: the resting LEARN pill is 40.2 px
  wider than in v1.2.1 and the Target block sits 40.2 px further right. That is
  the ONLY English-at-rest geometry change in this release, measured HEAD vs
  working tree over all 84 rendered boxes.
- `.meter-label` gains `white-space: nowrap`. The column is a hard 44 px and
  ENTRÉE renders 42.4 px inside it. Without nowrap a caption one glyph wider
  does not clip, it takes a second line and shoves the meter bars down. Costs
  English nothing (INPUT 33.3 px, OUTPUT 42.9 px).

### Fixed

- **The page no longer overflows its own frame.** v1.2.1's twenty-three in-flow
  `[data-tooltip]::after` boxes inflated the document's scroll extent to
  435 x 540 inside a 350 x 500 window, in English, at rest. The replacement
  surface is a single `position: fixed` element that is out of flow, so the
  scroll extent is now exactly 350 x 500 in both languages.

### Notes

- **All 63 French strings are machine drafts, every one flagged
  `reviewed: false`.** No native speaker has read them.
  `node scripts/check-i18n.js` prints the worklist.
- **All 23 tooltips were HAND-SPLIT; none split on a `": "`.** O-Gain's copy is
  bare sentences with no title prefix, so every tooltip title is the control's
  own existing English caption reused verbatim. Four strings DO contain a
  `": "` — the two `Range: ` tips, `Sum to mono: ` and the DEC `Note: ` — and a
  mechanical split on the first one would have made most of the sentence into
  the title. All 23 bodies are byte-identical to v1.2.1's attribute values.
- **The M/S captions localize and the meter/measure captions do not**, on the
  byte-identity test against the `AudioParameterChoice` option strings. `Peak`,
  `RMS`, `VU` and `LUFS` match `meter_mode` and `measurement_mode` verbatim and
  are `I18N_EXEMPT` under D-01 — translating them would make the page and the
  host automation lane disagree. `M/S OFF`, `ENC` and `DEC` match none of
  `ms_mode`'s `Off` / `Encode` / `Decode`, so they are page-invented captions
  and they translate.
- The seven-button utility row already renders PH L, PH R and M/S OFF on two
  lines in ENGLISH at this frame — 344.8 px of min-content in a 334 px row.
  That is pre-existing and untouched; every French caption was chosen so its
  widest word matches its English counterpart's, so the row keeps exactly two
  lines and its 32 px height.
- The vertical tooltip clamp is carried in from O-FreqPulse but is **not
  independently reproducible on this page** — measured, with the clamp deleted,
  over 52 tips with the Learn panel open and 40 with it closed: none left the
  frame. The same sweep with the HORIZONTAL clamp deleted instead reports 26
  off-frame tips, worst 89.3 px, so the probe is not blind.
- The hover-help switch is still SESSION-ONLY, exactly as v1.2.1's "?" was.
  This plugin has no `setTooltipsEnabled` bridge and this release does not add
  one.
- The DAW round-trip (pick Français, close the session, reopen) has not been
  executed by hand. It is reasoned from the `isVoid()` guard, not measured.

## [1.2.1] - 2026-07-21

Learn-mode safety fix. No parameter IDs, ranges, types, or state format changed —
v1.0.0 / v1.1.0 / v1.2.0 sessions and presets load unchanged.

### Fixed

- **Learn no longer slams `gain_offset` to the +40 dB max (blasting/clipping the
  output) when engaged over an unrepresentatively quiet passage.**
  Root cause: `finalizeLearn()` normalizes to the loudness measured *only during the
  learn window*, and the inter-sample-peak anti-clip ceiling only protects peaks seen
  *in that same window*. A quiet-but-valid capture — a soft intro, a low noise floor,
  anything above the −70 LUFS "invalid" floor — implied a normalization boost all the
  way to the +40 dB parameter maximum; the ISP ceiling happily allowed it because the
  window's own peaks were low, then real (louder) material clipped hard when it played
  through the boost. Confidence was computed but never gated the magnitude of the write.

  **Fix:** a new `kMaxLearnBoostDB` (**+24 dB**) caps the *automatic* learn-derived
  boost. When the raw normalization boost (`target − measured`, before the ISP ceiling)
  exceeds the cap, Learn treats the capture as too quiet / unrepresentative and **does
  not write `gain_offset`** — it publishes the existing complete + none-confidence
  snapshot instead. The Learn button now reads **"TOO QUIET"** (rather than a
  misleading "DONE") so the user knows to re-run Learn over a louder / representative
  section, or set the gain manually. The manual `gain_offset` slider keeps its full
  ±40 dB range — the cap applies only to Learn's automatic write. The ISP ceiling and
  all prior guards (−70 dB floor, 1 s minimum capture) are unchanged.

### Notes

- UI change is display-only (button label reuses the already-published `learnState` /
  `learnConfidence` fields — no new native-function or snapshot plumbing). A successful
  Learn still keeps its low/medium/high confidence and reads "DONE".

## [1.2.0] - 2026-07-01

Second code-review remediation pass (WR-03, WR-04, WR-05, IN-01…IN-06). Metering
label/behavior changes are user-facing → MINOR bump. No parameter IDs, ranges,
types, or state format changed — v1.0.0 / v1.1.0 sessions and presets load unchanged
(verified with pluginval strictness 5 + auval).

### Fixed

- **WR-03: the "True Peak / dBTP" readout is now labeled "Sample Peak / dBFS".**
  The metric was never oversampled — it is the max absolute *sample* value, so
  inter-sample peaks (which can exceed the sample peak by several dB) were never
  measured, yet it was presented as dBTP and used as a -1 dBTP safety ceiling. The
  label, tooltip, and internal names (`samplePeakMax` / `samplePeakDBFS`) now say
  dBFS, and the Learn safety ceiling drops an extra **3 dB** below -1 dBFS
  (`kInterSamplePeakHeadroomDB`) to cover the un-measured ISPs. No oversampling was
  added — relabel + headroom only.

- **WR-04: the running integrated-LUFS recompute no longer runs every 100 ms hop
  on the audio thread.** `calculateIntegratedLUFS()` is O(gatingBlockCount) (two
  passes over up to 4000 blocks) and was recomputed on every hop, so per-hop RT cost
  grew with learn duration. It is display-only, so it is now throttled to ~1 Hz
  (`kIntegratedRecomputeHops`); the authoritative final value is still computed once
  in `finalizeLearn()` (off the audio thread, per v1.1.0 CR-01).

- **WR-05: the Learn panel is published to the UI as one coherent snapshot.**
  The editor timer previously read ~17 independent atomics, so learnState /
  confidence / integrated / momentary could come from different `processBlock`
  iterations (e.g. "DONE" shown against a stale integrated value). The Learn-panel
  fields are now published together through a seqlock (`LearnSnapshot` +
  `learnSnapshotSeq`) and read coherently by the editor. Plain peak / RMS / VU meters
  remain independent atomics.

- **IN-01: removed the dead/empty branch in the `toggleLearn` native function.**

- **IN-02: `learnState` now clears back to idle when the gain is edited manually.**
  After a Learn completed, the button read "DONE" indefinitely. Editing `gain_offset`
  or `trim` (a real user edit, not Learn's own write — guarded by
  `ignoreLearnGainWrite`) now resets the display to idle.

- **IN-03: meter mode "LUFS" now shows momentary loudness during Learn.**
  It previously always showed RMS. It now drives the input meters from momentary
  K-weighted LUFS while Learn runs, falling back to RMS otherwise; the meter tooltip
  documents this.

- **IN-04: `withBackend(webview2)` (and its WinWebView2 options) are guarded with
  `#if JUCE_WINDOWS`** so the editor no longer references a Windows-only enum on macOS.

### Changed

- **IN-05: the M/S "DEC" tooltip documents the +6 dB behavior.** Decoding a normal
  (un-encoded) L/R signal raises level by +6 dB — the inverse of ENC's -6 dB — so
  ENC→DEC is a matched unity pair. DSP convention unchanged (no rescale).

- **IN-06: promoted magic numbers to named constants.** Gain ramp (0.02 s), VU
  ballistics (300 ms), LUFS block/hop (0.4 s / 0.1 s), gating capacity (4000),
  confidence thresholds (5 s / 15 s / 50 blocks), and the JS meter range (-60…0 dB) /
  clip threshold (-0.5 dB) are now `constexpr` / JS consts with unit comments.

### Notes

- Real-time safety preserved: the per-hop audio-thread path only writes a seqlock
  (no allocation, locking, or host notification).

## [1.1.0] - 2026-07-01

Code-review remediation (CR-01, CR-02, WR-01, WR-02). Adds working mono support
for VU metering and the Learn auto-gain feature (behavior change → MINOR bump).

### Fixed

- **CR-01 (critical): `finalizeLearn()` no longer runs on the audio thread.**
  Stopping Learn previously called `setValueNotifyingHost()` — a locking/allocating
  host + listener notification path — directly from `processBlock`, along with the
  log10/gate loops over up to 4000 gating blocks. This violated real-time safety and
  risked priority inversion / dropouts. The processor now derives from
  `juce::AsyncUpdater`: the audio thread only detects the learn-stop edge, sets a
  `learnFinalizePending` flag, and calls `triggerAsyncUpdate()` (once, not per block);
  the full measurement + gain write happens in `handleAsyncUpdate()` on the message
  thread. Robust even when the editor is closed.

- **CR-02 (critical): Learn can no longer slam `gain_offset` to +40 dB.**
  Learning over silence, near-silence, or (previously) any mono instance produced a
  measured level of −100 dB, driving the computed gain to the +40 dB clamp — a
  full-scale output and hearing-safety hazard. `finalizeLearn()` now bails out
  (confidence → none, `gain_offset` left untouched) when there is no valid
  measurement (`gatingBlockCount == 0` / `rmsSampleCount == 0`), the measured level is
  ≤ −70 dB, or the capture ran shorter than a 1 s minimum. The measured LUFS / true
  peak are still published to the UI so the user sees why nothing changed.

- **WR-01: peak-hold meters no longer freeze.**
  The ~300 ms decay coefficient is per-sample but was applied only once per block
  (~0.6 %/s at 512-sample blocks), so the peak latched at its maximum and its decay
  varied with block size. Decay is now a proper per-block coefficient
  (`pow(perSampleDecay, numSamples)`) in both the input and output metering stages.

### Added / Changed

- **WR-02: VU metering and Learn now function on mono instances.**
  Both were gated on `numChannels >= 2` and were silently dead in mono (the direct
  cause of the CR-02 mono +40 dB slam). Added a mono path that drives the VU
  ballistics filter from channel 0 (mirrored into both meters) and feeds channel 0
  into the K-weight / LUFS and RMS accumulators. The active channel count is snapshotted
  at learn start (`learnChannelsAtStart`) so the RMS divisor and LUFS block power yield
  the correct single-channel loudness rather than a 3 dB error.

### Notes

- No parameter IDs, ranges, types, or state format changed — v1.0.0 sessions and
  presets load unchanged.
- Real-time safety verified: no allocation, locking, or host notification remains in
  `processBlock`.

## [1.0.0] - 2026-03-07

Initial release. Stereo gain-staging utility with channel utilities (phase invert
L/R, channel swap, mono sum, M/S encode/decode), BS.1770 K-weighted LUFS "Learn"
auto-gain, VU / peak / RMS metering, and a WebView UI.
