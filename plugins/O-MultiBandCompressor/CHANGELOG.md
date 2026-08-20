# O-MultiBandCompressor Changelog

## Version 1.7.0 (2026-08-19)

Twenty-five more factory presets (50 total), and a categorised preset browser to
make a bank that size navigable.

### Added

- **Preset categories.** The browser now groups presets under seven headings — Init,
  Mastering, Mix Bus, Corrective, Instruments, Voice, Creative — plus a User group
  that appears once something has been saved. Headings stick to the top of the list
  while their group scrolls.

  The category lives in this plugin's own preset table, not in the shared
  `OuariconPresetManager`, so the twenty-odd other plugins vendoring that module are
  untouched. It is deliberately *not* written into the `Factory/*.json` files either:
  it is recovered by name lookup, so a hand-edited or renamed preset file cannot carry
  a stale category around. Anything absent from the factory table reports as "User",
  which is exactly the set that should have a delete button — so the dropdown no
  longer needs its old per-name `isFactoryPreset()` fan-out (N native round trips on
  every list change) and makes one `getPresetCategories()` call instead.

- **◀ / ▶ now walk the grouped order.** They previously stepped through
  `getPresetList()`'s flat alphabetical order, which was also the dropdown's order, so
  the two agreed. Grouping breaks that agreement, and the buttons have to follow what
  the user can see — otherwise ▶ from the last Mastering preset lands in the middle of
  Instruments. Navigation is now driven from the same flattened group order the
  browser renders.

- **Twenty-five presets.**
  - *Init* (2): **Init 3-Band** (XOVER3 parked at 16 kHz, high band bypassed),
    **Init Wide Bands** (80 / 700 / 6000 — an octave below the stock split in each
    position, so a bass fundamental sits inside the low band rather than straddling a
    crossover).
  - *Mastering* (4): **Streaming Loudness**, **Club Master** (90 Hz low crossover,
    3.5:1 — an uncontrolled sub is what actually distorts on a big rig), **Vinyl Prep**
    (deliberately asymmetric: 4.5:1 low and a filtered 3:1 top, the two things that
    damage a cutter head, with the mids left near-alone), **Mid Focus Master** (M/S
    Mid — the mirror of *Wide and Controlled*).
  - *Mix Bus* (6): **Drum Bus Glue** (the slow, RMS counterpart to *Drum Bus Punch* on
    the same source), **Guitar Bus**, **Synth Bus**, **Percussion Bus**,
    **Orchestral Bus**, **Room Mic Bus** (low thresholds plus auto-makeup, which lifts
    the tail toward the transient rather than the reverse).
  - *Corrective* (2): **Plosive Control** (0.5 ms at 8:1 — not *Low End Control* with a
    faster attack; that one levels the bass region, this catches a single blast of air
    and releases in 60 ms), **Sub Rumble Control** (XOVER1 at 45 Hz so the band holds
    rumble and not the bass fundamental).
  - *Instruments* (3): **Kick Drum**, **Snare Drum**, **Upright Bass** (20-25 ms and
    RMS-led where *Bass Tighten* is 8 ms and peak-led, because an upright's level
    varies with stopping position rather than pick attack).
  - *Voice* (3): **Lead Vocal Tight**, **Backing Vocal Stack** (gentler overall but the
    de-esser stays strong — stacking makes sibilance worse, not better, as every double
    lands its "s" slightly apart), **Dialogue Leveller**.
  - *Creative* (5): **Pumping Bass**, **Lo-Fi Squash**, **Telephone Squeeze**,
    **Mid Side Slam** (M/S Both), **Dark Tilt**.

### Changed

- **The harness's "should be inert" check keys off the Init category, not the name
  "Init Flat".** Any Init preset added later is covered without the harness learning
  its name; the flip side is that an Init preset that ships a ratio above 1:1 now fails
  there instead of passing as an ordinary working preset.

- **The dropdown's max height is 420px, up from 260px.** Fifty presets across seven
  groups did not fit; the editor is 640px tall and the bar sits at the top, so the open
  list still clears the bottom edge.

- **The delete-confirmation strip is now pinned to the top of the list.** It is
  prepended into the scrolling container, so with a 50-entry list the confirmation for
  a delete button well below the fold used to render off-screen — which reads as
  nothing having happened.

### Fixed

- **`.preset-dropdown-header` was dead CSS.** Defined in v1.5.0, never referenced by
  `app.js`. Its styling is now what `.preset-group-heading` uses, with the background
  changed from `rgba(139,168,112,0.25)` to the same olive pre-composited onto the
  dropdown's parchment — a translucent heading lets the rows it is pinned over show
  through it.

### Testing

- The preset harness passes 50/50 with zero failures, and the reverse-order pass
  reports every preset identical in both directions. Every preset that existed in
  v1.6.0 measures byte-identical gain reduction to its v1.6.0 baseline, so none of the
  existing sounds moved.
- Two new presets were retuned after their first harness measurement, which is what the
  harness is for:
  - *Sub Rumble Control*, authored at -40 dB / 8:1 on the band-level staircase,
    measured 31 dB of gain reduction — a gate, not a control. The staircase is
    calibrated for bands carrying program material and this one is not: below 45 Hz
    there should be almost nothing, so the threshold has to sit above the noise floor
    of a clean recording. At -22 dB / 6:1 it measures 14.4 dB, in line with the rest of
    the Corrective group.
  - *Dark Tilt*'s high band, at -42 dB / 8:1, measured 27 dB and was therefore
    compressing continuously — a fixed tilt wearing a compressor's clothes, and the
    exact behaviour its own comment claimed it avoided. At -32 dB / 5:1 it measures
    16.7 dB and actually tracks the material.
- Grouping, the ◀ / ▶ walk order and the sticky headings were verified in a browser
  against a stubbed JUCE backend, with the fixture generated from the C++ table so the
  test cannot drift from it: 3 / 8 / 7 / 9 / 11 / 5 / 7 factory presets per group,
  delete buttons only under User, and ▶ from the last Init preset landing on the first
  Mastering preset rather than the next name alphabetically.
- `auval -v aufx OMbc OuDv`: PASS. `pluginval --strictness-level 10`: SUCCESS.
## Version 1.6.1 (2026-08-19)

Resolves the Critical and all five Warning findings from the 2026-08-19 deep code
review (CODE_REVIEW.md): CR-01, WR-01..WR-05. No parameter, preset, or state-format
changes.

### Fixed

- **M/S "Both" mode was not independent mid/side compression (CR-01).** Mode 3 sent
  the [mid, side] buffer through the same stereo-linked path as L/R. The linked
  detector averages its channels, and (M+S)/2 = ((L+R)+(L−R))/(2·√2) = L/√2 — the
  detector was the left channel only, so a right-only signal produced zero gain
  reduction. The single computed gain applied to both M and S is algebraically the
  same as applying it to L and R, i.e. not M/S processing at all. Root cause: no
  unlinked detector topology existed. `Compressor` now has a per-channel
  (dual-mono) path — per-channel envelope detectors, gain-reduction ballistics,
  sidechain filter state, and makeup state — selected by a `linkedDetector` flag
  that only "Both" mode clears; every other path is element-[0]-only and
  bit-compatible with v1.6.0. The band meter reports the deeper of the two
  channels. Verified by probe: right-only noise now drives −16..−29 dB GR
  (previously exactly 0), and a quiet-mid/loud-side signal's output side/mid ratio
  collapses from 45:1 to 2.1:1 (shared-gain behaviour would keep 45:1).

- **Meters and crossover lines froze on comma-decimal host locales (WR-01).** The
  three 30 Hz UI pushes (`sendGainReductionMeters`, `sendInputOutputMeters`,
  `sendCrossoverPositions`) built their JavaScript with
  `juce::String::formatted("%f", …)`, which routes through `vswprintf` and honours
  `LC_NUMERIC` — a comma-decimal locale (several Windows hosts) emitted
  `updateGainReductionMeters(0,500000, …)`, a JS syntax error on every tick, and
  all meters silently stopped. Now built with locale-independent
  `juce::String(value, digits)` concatenation, matching `sendSpectrumData`.

- **Mid/Side modes mirrored the stereo image near each crossover (WR-02).** The
  processed channel exits the crossover carrying its AP(f1)·AP(f2)·AP(f3) all-pass
  rotation (each LR4 pair sums to a 2nd-order all-pass); the passthrough channel
  carried none, so around each crossover the two sat ≈180° apart and the M/S
  decode swapped L and R even at 1:1 ratios. The passthrough channel now runs
  through a matching chain of the same three all-passes (new `PhaseMatchChain`,
  corners clamped identically to the crossover's via the shared
  `CrossoverNetwork::clampCrossoverFrequencies`).

- **Parallel (Mix < 100%) combing (WR-03).** The dry path was captured before the
  crossover, so at intermediate Mix settings the un-rotated dry signal cancelled
  against the wet path's all-pass rotation wherever the chain's phase passed −180°
  — multiple notches, deepest near 50% (the shipped Parallel Crush preset at 35%
  bakes this in). The dry copy now runs through the same `PhaseMatchChain` before
  `pushDrySamples`, so dry and wet sum coherently at every Mix value.

- **Makeup and Auto-Makeup gain stepped at block boundaries (WR-04).** `makeupDB`
  was latched once per block and applied per sample unsmoothed — audible zipper on
  Makeup knob turns and a hard multi-dB click when toggling AUTO_MAKEUP. Total
  makeup (manual + auto) is now one-pole smoothed per sample (~15 ms), snapping on
  the first sample after prepare/reset so sessions don't open with a fade-in.

- **A single NaN input permanently disabled a band's compression (WR-05).** One
  non-finite sample entering `EnvelopeDetector`'s sliding RMS sum could never be
  subtracted back out; `rmsSum` stayed NaN, `GainComputer` returned 0 GR forever
  (`std::min(0, NaN)` → 0), and the band silently stopped compressing until the
  next `prepareToPlay`. The detector now zeroes non-finite inputs at entry and
  self-heals a poisoned accumulator by rebuilding the window from silence. This is
  the codebase's documented envelope-follower sticky-NaN pattern.

### Testing

- Preset render harness: 47/47 real presets engage, load-order-independence pass
  clean across all 50. ("Init 3-Band" / "Init Wide Bands" flags come from the
  parallel v1.7.0 preset work's on-disk factory set — 1:1-ratio init templates,
  inert by design, not part of this branch's 25-preset factory list.)
- CR-01 behavioral probe (right-only detector visibility, mono engagement,
  channel-separation ratio): 3/3 pass.
- auval (aufx OMbc OuDv): PASS.

## Version 1.6.0 (2026-07-23)

Nine more factory presets (25 total), and a sidechain-filter bug found while
verifying them.

### Fixed

- **Per-band sidechain HPF and LPF could silently stay off when the parameter
  asked for them.** `Compressor::updateSidechainFilters()` set `scHPFEnabled = true`
  only inside its "the frequency changed" branch. When the requested frequency was
  above zero *and* equal to the cached `currentSCHPFFreq`, neither branch ran and the
  enabled flag kept whatever value it already had — so any path that had previously
  disabled the filter left it disabled while the UI showed a frequency. Same hole in
  the low-pass branch.

  Reachable with a single knob: set SC HPF to 100 Hz, turn it down to Off, then back
  to 100 Hz — the filter stayed off. It also fired on every preset switch, because
  `currentSCHPFFreq` survives the switch, which is how it was found: presets showed
  3–4 dB of different gain reduction depending purely on which preset had been loaded
  before them.

  The enabled flags are now derived from the requested frequency on every call, and
  only the coefficients are recomputed conditionally.

- **Sidechain coefficient updates no longer allocate on the audio thread.**
  `Coefficients::makeHighPass`/`makeLowPass` return a newly heap-allocated
  ref-counted object; they ran inside `processBlock` on every sidechain frequency
  change (any knob move or automation ramp). Now uses `ArrayCoefficients`, which
  returns a stack `std::array`, assigned through
  `Coefficients::operator=(std::array)`. `prepare()` seeds both filters with a real
  biquad so that assignment reuses already-allocated storage rather than growing it.

  Note for anyone touching this again: `Coefficients` stores **5** normalised values,
  not the 6 raw ones `ArrayCoefficients` returns — `assignImpl` drops `a0` and divides
  the rest by it. Copying the array over `getRawCoefficients()` writes unnormalised
  coefficients and overruns the logical size.

### Added

- **Three more harshness presets.** *Cymbal Sizzle Control* (7 kHz+, peak detection,
  1 ms attack, for overhead and hat wash), *Nasal Honk Control* (crossovers at 800 Hz
  and 2 kHz put the high-mid band exactly on the honky region; deliberately slower
  than the other harshness presets because honk is a sustained resonance rather than a
  transient) and *Amp Fizz Control* (3.5 kHz+ with a 10 kHz sidechain low-pass, so the
  band responds to amp-sim fizz rather than being held down by genuine air).

- **Six instrument presets.** *Electric Guitar*, *Piano* (widest dynamic range of
  anything here, so every band is gentle with a wide knee), *Strings Ensemble* (the
  slowest preset in the set — 40–60 ms attacks so the bow articulation is never
  clamped), *Brass Section* (the 1–4 kHz blare band is the aggressive one), *Mallet
  Percussion* (25–35 ms attacks let the mallet strike through; what is controlled is
  the ring after it) and *Woodwind Breath* (gentle body bands, with the work happening
  above 5 kHz where breath and key noise live).

- **Order-independence check in the preset harness.** It now measures every preset a
  second time in reverse order and fails if any band differs by more than 0.01 dB.
  This is the check that caught the sidechain bug above, and it guards against any
  future DSP state that survives a preset switch.

### Changed

- The version is now set once as `OMBC_VERSION` in the plugin CMakeLists and consumed
  by both the plugin and the test harness. The harness previously repeated it as a
  literal, and a stale value there would restamp the real factory-preset directory
  with the wrong `.factory-version`.

### Testing

- `auval -v aufx OMbc OuDv` — PASS
- pluginval strictness 10 — SUCCESS, 3 consecutive runs
- Preset harness: 24 presets active, Init Flat correctly inert, 0 failures, no
  non-finite output, **all 25 presets identical in forward and reverse order**
- All 25 factory `.json` files regenerated by the installed binary

## Version 1.5.0 (2026-07-22)

Adds preset management with 16 factory presets, and puts on-screen controls on
three per-band parameters that previously had none.

No parameter IDs, ranges or types changed. Sessions saved by v1.4.x and earlier
restore unchanged — the state is still the APVTS tree under the same tag, now with
one extra `currentPreset` attribute that older saves simply do not carry.

### Added

- **Preset bar in the header** — previous / next, a clickable name that opens the
  preset list, and Save / Load buttons that use the native file dialogs. Presets
  live in `~/Library/O-MultiBandCompressor/Presets/` under `Factory/` and `User/`.
  Factory presets are read-only; user presets carry a delete button in the list,
  behind an in-DOM confirmation. (`window.confirm()` is a silent no-op in some JUCE
  WebView backends, so the module's `onConfirmDelete` hook is supplied rather than
  relying on the fallback.)

  Uses the shared `preset-manager` module (v1.0.5) via `ouaricon_add_module`, so
  the plugin picks up module fixes automatically rather than through a vendored copy.

- **16 factory presets.** Mastering and bus: Init Flat, Mastering Glue, Transparent
  Master, Loud and Punchy, Warm Tape Glue. Corrective: Low End Control, Mud Tamer,
  De-Esser Vocal, Harshness Tamer. Instrument and bus: Vocal Bus, Drum Bus Punch,
  Bass Tighten, Acoustic Guitar, Podcast Voice. Effect: Parallel Crush, Wide and
  Controlled (M/S Side).

- **Per-band Detector / Sidechain controls.** `*_PEAK_RMS`, `*_SC_HPF` and
  `*_SC_LPF` have existed since the plugin shipped and were fully wired into the
  DSP, but had no control in the UI — they were reachable only through host
  automation. Each band now carries them in a sub-group below the six gain-stage
  knobs, separated by a rule because they shape what the compressor *hears* rather
  than what it does about it.

  Their readouts are driven by `SliderState.getScaledValue()`, so the displayed
  value comes from the real C++ `NormalisableRange` instead of a range restated in
  JavaScript. The six existing knobs keep their current formulas — those readouts
  are correct and rewriting them carries regression risk for no gain.

- **Factory preset verification harness** (`tests/render-harness`, off by default,
  build with `-DOUARICON_BUILD_TESTS=ON`). Loads each factory preset, renders
  correlated stereo pink noise, and reports the gain reduction every band actually
  reaches. This is what the preset values below were tuned against.

### Changed

- Plugin window is 900×640, up from 900×600, to seat the new per-band knob row
  without crowding the band columns. `.plugin-container` in `styles.css` and
  `setSize()` in `PluginEditor.cpp` must stay in agreement.

- `getStateInformation` / `setStateInformation` now route through the preset
  manager so a session remembers which preset is loaded.

### Preset design notes

**Thresholds follow a band-level staircase.** Threshold here is measured on the
*band* signal, not the full-range input, and in a mix balanced to roughly
−12 dBFS RMS the 8 kHz+ band typically sits 25–30 dB below the low band. A preset
using one threshold across all four bands would squash the lows and never engage
up top. The four values step down about −20 / −24 / −30 / −38 and shift from there
per application.

**Values are authored in engineering units, not normalised fractions.**
`FactoryPresetDef` stores normalised 0–1 values, but `ATTACK`, `RELEASE`,
`XOVER1/2/3` and `SC_HPF`/`SC_LPF` all carry a skew of 0.3. Writing the fractions
by hand would recall those parameters 10–30× wrong — a hand-written `0.3` for the
de-esser's 0.3 ms attack would have produced 4.0 ms. The table is therefore written
in dB / ms / Hz / ratio and converted through each parameter's own
`NormalisableRange` via `convertTo0to1()` in `buildFactoryPresets()`.

**Solo and SC Listen are never engaged in a preset.** Both are audition states; a
preset shipping either one would mute or replace the mix the moment it loaded.

**Where the invisible-until-now parameters are used.** Peak detection (`PEAK_RMS`
at or near 0) on de-essers and plosive bands, where an RMS window would average the
sibilant away; RMS-leaning detection (70–85) on the glue and levelling presets.
`SC_HPF` at 25–30 Hz on low bands so inaudible subsonic energy does not pull a
whole band down. `SC_LPF` at 11–12 kHz on the de-esser bands so cymbals and air do
not hold the de-esser open.

### Testing

- `auval -v aufx OMbc OuDv` — PASS
- pluginval strictness 10 — SUCCESS, 3 consecutive runs
- Frontend rendered in a browser against a JUCE-bridge stub: no console errors, all
  56 readouts correct, no band column overflows its height, preset dropdown escapes
  the header stacking context, load / next / previous / delete-with-confirmation all
  verified end to end
- `withNativeFunction` vs `getNativeFunction` diffed: no bridge gaps
- All 16 factory `.json` files decode back to their authored engineering values;
  all 56 parameters present in each; no solo or SC-listen engaged; all values in [0, 1]
- Preset harness: 15 presets active, Init Flat correctly inert, 0 failures, no
  non-finite output. Single-purpose presets engage exactly their intended band
  (De-Esser Vocal high only, Harshness Tamer high-mid only, Low End Control low
  only, Mud Tamer low-mid only), and peak gain reduction rises monotonically with
  intended density: Transparent Master −2.6…−4.6 dB, Mastering Glue −4.9…−7.6,
  Warm Tape Glue −6.3…−12.7, Loud and Punchy −9.3…−14.7, Parallel Crush −30.8…−33.9.

  Three presets were retuned on the strength of those numbers: Vocal Bus and
  Podcast Voice had their low crossover moved to 120 Hz with a gentler low band, so
  a male fundamental (roughly 85–180 Hz) sits in the RMS body band rather than under
  the fast plosive band; Acoustic Guitar's low band went from 4:1 at −26 dB to 3:1
  at −22 dB, having hollowed out the 82 Hz low E and the ~100 Hz body resonance.

  Caveat on reading those figures: pink noise is continuously dense, so it is a
  worst case for bands deliberately designed to stay dormant on real material. The
  vocal low bands still read −15…−17 dB against noise; on speech, where there is
  little below 120 Hz except plosives, they engage far less. The figures are most
  useful as a relative ordering, not as absolute expectations.

## Version 1.4.2 (2026-07-22)

UI-only release. No DSP, parameter, or state-format changes — presets and
automation load unchanged.

### Fixed

- **The three buttons under each band now say what they are: SOLO, BYPASS and
  SC LISTEN.** Until now all three showed the word "Off" (or "On" once engaged),
  so a band's button row read "Off Off Off" and nothing on screen distinguished
  solo from bypass from sidechain listen. The only way to tell them apart was to
  hover for a tooltip or to click one and listen.

  Root cause: `updateToggleUI()` in `app.js` was shared by every toggle in the
  plugin and unconditionally rewrote the button's text to "On"/"Off" on bind and
  on every state change. The `S` / `B` / `SC` glyphs authored in `index.html`
  were therefore overwritten the moment the UI connected to the backend, and had
  never actually been visible in a DAW.

  Each band button now carries its own name in a `data-label` attribute, and
  `updateToggleUI()` keeps that name instead of relabelling. Engaged state is
  shown by the existing `.active` styling — the olive fill the buttons already
  had — which is now the sole indicator rather than a redundant one. The global
  Auto-MU toggle has no `data-label` and is unchanged: it sits beside its own
  "AUTO-MU" caption, so On/Off is the informative thing to show there.

### Changed

- **Band buttons size to their text** instead of a fixed 28 px square. The row
  measures 163 px inside a 189 px band column, so it stays centred with room to
  spare and the panel does not reflow. `white-space: nowrap` keeps "SC LISTEN"
  on one line; the buttons also inherit the panel's Garamond stack, which the
  browser default button font had been overriding.

- **`aria-pressed` now tracks each band button's state.** Engaged/disengaged is
  conveyed by fill colour alone now that the text is a fixed label, so the state
  is exposed to assistive technology explicitly.

### Testing

Verified in a browser against a stubbed JUCE bridge at the plugin's real 900×600
size: all four bands render an identical single-line row with no overflow, the
labels survive toggling on and off, `.active` and `aria-pressed` both track
state, and the Auto-MU toggle still reads On/Off. Built clean and validated with
auval.

## Version 1.4.1 (2026-07-22)

UI-only release, following on from the v1.4.0 tooltip work. No DSP, parameter, or
state-format changes — presets and automation load unchanged.

### Added

- **"?" button in the header to switch the tooltips off and on.** The hover help
  added in v1.4.0 is useful while learning the plugin and gets in the way once you
  know it, so it is now switchable. The button sits to the right of the Ouaricon
  mark: filled olive when help is on, a quiet outline when it is off.

  The setting is stored **outside** the plugin state, in a machine-wide preference
  file (`~/Library/Application Support/Ouaricon/O-MultiBandCompressor.settings` on
  macOS, `%APPDATA%\Ouaricon\` on Windows), read through two new WebView native
  functions (`getTooltipsEnabled` / `setTooltipsEnabled`). Keeping it out of the
  APVTS is deliberate: tooltip visibility is a preference about *you*, not about a
  mix, so every instance agrees on it, it survives DAW restarts, and loading
  somebody else's preset can never switch your help text off. It is written through
  on every click rather than left to the auto-save timer, so a force-quit does not
  lose it.

  Defaults to **on**, matching v1.4.0 exactly for anyone updating.

  The "?" keeps its own tooltip while help is switched off — the control that turns
  the tips back on has to be able to explain itself — and re-shows that tip in place
  straight after a click, so the new state is confirmed without moving the mouse.

### Fixed

- **Tooltips near the right edge of the window wrapped into a narrow ribbon.** The
  positioner measured the tip, then set `left`; because a fixed-position box with
  `width:auto` shrink-to-fits whatever space remains to its right, applying a
  near-the-edge `left` afterwards re-wrapped the copy and re-flowed it taller than
  the measurement it had just been placed from. The header "?" button, as the
  right-most control in the interface, rendered its tip 70 px wide instead of 230.

  Root cause: `showTooltip()` took a single `getBoundingClientRect()` at the tip's
  *previous* offset and treated that width as final. It now releases the width,
  measures from `left: 0` where the full viewport is available, pins the measured
  width in px so the box cannot re-flow, and only then measures height and places
  the tip. Present since v1.4.0; it went unnoticed because every other control sits
  far enough inboard that the squeeze was small.

### Testing

- Browser harness against a ~20-line JUCE-bridge stub at the true 900×600 plugin
  size (see `pattern_module_toplevel_init_tdz` — build/auval cannot see a dead
  WebView UI): toggle off suppresses every control tip, the "?" keeps its own,
  toggle on restores them, `setTooltipsEnabled` is called once per click and *not*
  on the start-up read.
- Tooltip-geometry regression sweep across 11 controls spanning all four corners
  (header button, far-left knob, far-right knob, all three crossover handles, both
  level meters, a GR meter, a band header, the analyzer): all now measure the full
  230 px, sit fully on-screen, and flip above/below correctly.
- JS↔C++ bridge diff: both `getNativeFunction` names in `app.js` have matching
  `withNativeFunction` registrations in `PluginEditor.cpp` (see
  `pattern_webview_native_fn_bridge_gap` — an unregistered name fails silently).

## Version 1.4.0 (2026-07-22)

UI-only release. No DSP, parameter, or state-format changes — presets and automation from
v1.3.0 load unchanged.

### Added

- **Tooltips across the whole interface.** Every control now shows hover help: the six
  knobs and three buttons in each of the four bands, all five global controls, both level
  meters, the four gain-reduction meters, the band headers, the spectrum analyzer, and the
  three crossover handles. Copy explains what the control does and states its range, so
  the numbers in the readouts have context.
  Implemented as a styled tooltip layer rather than native `title=` attributes: the OS
  tooltip has a fixed ~1 s delay and system chrome that clashes with the parchment theme.
  The custom layer uses a 120 ms delay, parchment fill with a `#5C4033` border in Garamond,
  flips above/below when it would run off the top, clamps horizontally to the viewport
  while its arrow keeps pointing at the control, and hides instantly on mouse-down so it
  never sits over a knob being dragged.
  The tooltip element lives outside `.plugin-container` (which is `overflow: hidden`) so
  tips on edge controls are not clipped. Copy is written via `textContent`, never
  `innerHTML`. Per-band wording is defined once in `BAND_TOOLTIPS` and applied to all four
  bands, so the four copies cannot drift apart.

### Fixed

- **Band header frequency ranges never updated.** The four `.band-range` readouts were
  literal strings in `index.html` (`20 Hz - 200 Hz`, etc.) and no code ever wrote to them.
  Moving a crossover repositioned the line, relabelled that one line, and repatched the
  DSP, while all four band headers went on advertising the factory defaults — so at any
  non-default crossover setting the headers were simply wrong. Preset loads and host
  automation had the same gap.
  Fix: `updateBandRanges()` rewrites all four readouts from the current crossover
  frequencies, called from both paths that can change them — `updateCrossoverPositions()`
  (the 30 Hz C++ push, which covers automation and preset recall) and `handleCrossoverDrag()`
  (using the live drag value, so the headers track the handle instead of trailing it by up
  to a frame).

- **Plugin bundle reported version 1.0.0.** `juce_add_plugin()` had no `VERSION` argument,
  so JUCE fell back to `PROJECT_VERSION` and every build since the first shipped as 1.0.0
  regardless of the CHANGELOG — confirmed against the installed bundle's
  `CFBundleShortVersionString`. Added `VERSION 1.4.0`. Hosts that key their plugin cache on
  the bundle version were unable to tell releases apart before this.

- **Crossover lines rendered at the wrong position on open.** The markup hardcoded
  `left: 15% / 45% / 75%`, but the log-scale positions of the 200 Hz / 2 kHz / 8 kHz
  defaults are `33.3% / 66.7% / 86.7%`. The lines were visibly misplaced until the first
  timer push corrected them. Initial values now match the defaults.

- **`applyOrderingConstraints()` misread a crossover parked at its minimum.** It used
  `getNormalisedValue() || 0.5`, so a legitimate normalised value of exactly `0` (the
  bottom of the range) was replaced by the `0.5` fallback — e.g. XOVER1 at 20 Hz was
  treated as ~68 Hz when constraining its neighbours. Changed to `??` and factored the
  three reads into `getCrossoverFreqs()`, now shared with the drag handler.

### Changed

- `formatFrequency()` drops a trailing `.0`, so 2000 Hz reads `2 kHz` rather than `2.0 kHz`,
  matching the strings the markup already shipped. Affects the crossover line labels as
  well as the new band ranges.

### Verification

- **Static frontend check:** all 55 tooltip selectors resolve against `index.html`; every
  `.closest()` wrapper class exists; the `.tooltip*` classes, `--arrow-x`, and
  `[data-placement]` rules are present in `styles.css`; all four `range-*` ids exist in the
  markup and are written by `app.js`; and `updateBandRanges` is reachable from **both**
  `updateCrossoverPositions` and `handleCrossoverDrag` — a regression in either alone would
  leave half the feature dead while the other half still looked correct.

- **Browser harness** — `app.js`/`index.html`/`styles.css` loaded unmodified against a stub
  of the JUCE `getSliderState`/`getToggleState`/`getComboBoxState` bridge, driven with real
  mouse events. This caught a defect that every other gate passed:
  `initializeTooltips()` was called from `initializeUI()`, which runs at module top level —
  above the `let`/`const` tooltip state, still in the temporal dead zone — throwing
  `ReferenceError: Cannot access 'tooltipEl' before initialization`. The throw escaped
  module evaluation, so **`initializeCrossoverDrag()` at the foot of the file never ran and
  crossover dragging was entirely dead**. The C++ build, `auval`, and the static check above
  all passed with the UI in that state. Tooltip init now happens at the foot of the file,
  after its state is evaluated, with a comment recording why it must stay there.
  Post-fix results: 0 console errors; 41/41 interactive controls covered by a tooltip
  (55 tip targets total); dragging crossover 1 to ~316 Hz updated LOW and LOW-MID live, and
  crossover 3 to ~3.5 kHz updated HIGH-MID and HIGH; the arrow lands within 1 px of the
  control centre; tips clamp inside the viewport at both panel edges while the arrow keeps
  tracking; tips over the analyzer flip below; and tips hide on mouse-down and stay
  suppressed for the duration of a drag.

- `node --check` clean on `app.js`; `auval -v aufx OMbc OuDv` **PASS**.
- Bundle version confirmed at the binary: `CFBundleShortVersionString` 1.4.0 and
  `AudioComponents` version `66560` (`0x010400`), up from `65536` (`0x010000` = 1.0.0).

## Version 1.3.0 (2026-07-01)

Transparency fix from `.planning/CODE-REVIEW.md` (WR-03). **Changes the sound** (for the
better): the plugin is now magnitude-flat at unity with all compressors bypassed.

### Fixed

- **WR-03 — Serial crossover summed with magnitude ripple even at rest.**
  Root cause: the 4-way split is serial (LOW exits at crossover 1, the remainder is
  split again at crossovers 2 and 3), so LOW never accumulated the phase rotation the
  upper bands pick up from crossovers 2/3, and LOMID never saw crossover 3's. Summing
  the bands therefore rippled up to **0.63 dB** around the crossover points with every
  compressor bypassed. The "Linkwitz-Riley guarantees flat magnitude" assumption only
  holds for a single 2-way split.
  Fix: all-pass compensation in `CrossoverNetwork` — an LR4 pair sums to a 2nd-order
  all-pass at its crossover frequency (Q = 1/√2), so LOW now passes through AP(f2)·AP(f3)
  and LOMID through AP(f3). The 4-band sum is then AP(f1)·AP(f2)·AP(f3): pure all-pass,
  flat magnitude. Costs 3 extra biquads per channel; coefficients follow the existing
  RT-safe in-place `ArrayCoefficients` pattern (CR-01) — no audio-thread allocation.
  Side benefit: bands are now phase-coherent at the sum, so per-band gain changes
  (compression, makeup) produce less phase-cancellation artifact around the crossovers.

### Verification

- **Offline A/B harness** (v1.2.2 crossover vs v1.3.0, 48 kHz, 20 Hz–20 kHz):
  - FFT of the summed impulse response (131k samples): old ripple 0.455–0.625 dB
    depending on crossover settings; new worst-case **0.014 dB** (float32 coefficient
    quantization floor, at 20 Hz with the 60/300/2.5k setting).
  - Stepped swept sine (120 log-spaced tones, whole-cycle RMS windows): old
    0.454–0.625 dB; new worst-case **0.013 dB**. Both methods, 4 crossover settings
    (default 200/2k/8k, 60/300/2.5k, 500/5k/16k, 120/800/3.5k) — PASS at ±0.02 dB.
- **pluginval** strictness 10 — PASS.
- **auval** (`aufx OMbc`) — PASS.

## Version 1.2.2 (2026-07-01)

Correctness + polish pass from `.planning/CODE-REVIEW.md` (WR-02, WR-04, IN-05, IN-06).

### Fixed

- **WR-02 — Mid/Side modes under-detected by −6 dB (compressed too little).**
  Root cause: the band buffers are preallocated stereo, but in mono M/S modes the
  crossover only fills channel 0 — `Compressor::processStereo` then averaged the silent
  channel 1 into the detector, halving the detected level. The active channel count is
  now threaded from `processMultiband` into `processStereo`, so detection runs over
  channels that actually carry signal. Mid/Side now apply the same gain reduction as
  Off/Both for identical threshold/ratio settings.
- **WR-04 — Attack/Release readouts didn't match the DSP value.** The APVTS ranges use
  skew 0.3 (`value = min + (max−min)·norm^(1/0.3)`) but the labels used a pure-log
  interpolation — at mid-travel the Attack label read ~4.5 ms while the DSP ran ~20 ms.
  The formatters in `app.js` now use the skew-aware mapping. Display-only; no DSP change.
- **IN-05 — Resource provider matched on basename only.** `getResource` now matches the
  full relative path via an explicit path→resource table (same pattern as O-DigiDelay),
  so same-named files in different folders can never collide.

### Changed

- **IN-06 — Spectrum analyzer now uses a log frequency axis.** The 64 UI bins are
  log-spaced 20 Hz–20 kHz (edges precomputed in `prepareToPlay`, matching the crossover
  overlay's log axis) instead of linear FFT-bin grouping that crammed everything below
  ~5 kHz into the left sliver. Each UI bin takes the peak (not average) of its FFT bins,
  so narrowband energy is no longer smeared. Cosmetic/analyzer-fidelity only.

## Version 1.2.1 (2026-07-01)

Real-time-safety pass from `.planning/CODE-REVIEW.md`. Removes all audio-thread
allocation, locking, and redundant work. **No intended sonic change** — the crossover
refactor is verified bit-identical to v1.2.0 (see Verification).

### Fixed (Real-Time Safety)

- **CR-01 — Crossover redesigned every block on the audio thread.**
  `CrossoverNetwork::updateCoefficients` no longer calls the heap-allocating,
  trig-heavy `FilterDesign::designIIR...ButterworthMethod` ×6 per block. It now caches
  the last `xover1/2/3` + sample rate and early-outs when unchanged (the common case),
  and when they change it designs with stack-only `IIR::ArrayCoefficients` and assigns
  into pre-allocated coefficient storage — zero heap allocation on the audio thread. The
  2nd-order Butterworth (Q = 1/√2) is numerically identical to the old order-2 design.
- **CR-02 — 48 `juce::String` allocations per block.** The per-band parameter IDs are no
  longer built as runtime strings each block. All 56 `std::atomic<float>*` pointers are
  resolved once in `prepareToPlay` into a `[band][param]` table and indexed in
  `processBlock`.
- **CR-03 — Per-block `AudioBuffer` allocation in M/S Mid/Side modes.** The mono M/S
  scratch buffer is now pre-allocated in `prepareToPlay` and reused via
  `setSize(..., avoidReallocating=true)` — no allocation on the audio thread.
- **WR-01 — `std::mutex` locked on the audio thread for the spectrum publish.** Replaced
  with a lock-free triple-buffer + atomic slot hand-off. `processBlock` never locks; the
  UI thread claims the most-recent frame without blocking the audio thread.

### Changed (Minor / Efficiency)

- **IN-01** — Crossover and compressor hot loops use cached `getReadPointer`/
  `getWritePointer` instead of per-sample bounds-checked `getSample`/`setSample`.
- **IN-02** — Attack/release ballistics coefficients are recomputed only when the
  attack/release time actually changes (was ~8 `exp()` per block, unconditionally).
- **IN-03** — Input/Output `dsp::Gain` now uses a ~20 ms ramp so gain automation no
  longer zippers.
- **IN-04** — Removed dead members (`EnvelopeDetector::peakValue`/`rmsValue`,
  `MultiBandProcessor::maxSamplesPerBlock`/`channelCount`, and the crossover's unused
  `spec` member).

### Fixed (regression caught during verification)

- The first cut of the CR-01 in-place coefficient write assumed `IIR::Coefficients`
  stores 6 taps; it actually stores the normalized 5-tap form (`{b0,b1,b2,a1,a2}`).
  Writing 6 floats overflowed the coefficient array (a heap overrun that produced NaNs
  under pluginval's parameter fuzz at strictness 10). Fixed by assigning through
  `IIR::Coefficients::operator=(std::array)`, which normalizes correctly and reuses the
  pre-allocated storage (RT-safe).

### Out of Scope (deferred — would change the sound)

- **WR-02** (M/S Mid/Side −6 dB detection), **WR-03** (serial-crossover all-pass
  compensation), **WR-04** (Attack/Release readout skew), **IN-05/06** (resource-provider
  basename match, log-frequency analyzer mapping) are intentionally *not* addressed here;
  several change audible behavior and belong in a separate MINOR release.

### Verification

- **Bit-exact A/B:** offline harness ran identical white noise + a swept crossover
  through the old (`designIIR`) and new (`ArrayCoefficients`) `CrossoverNetwork` for
  ~4.3 s; max sample difference = `0.000e+00` (bit-identical).
- **pluginval** strictness 10 — PASS (4 runs, including 3 randomized seeds); the fuzz
  test exercises M/S modes, solo, and bypass.
- **auval** (`aufx OMbc`) — PASS at 44.1/11/192 kHz, mono and 1→2-channel.

---

## Version 1.2.0 (2026-01-26)

### Added

- **Real-Time FFT Spectrum Analyzer:** Live frequency visualization in the spectrum display
  - 2048-sample FFT with Hann windowing (`juce::dsp::FFT`)
  - 64-bin downsampled output for efficient UI transfer
  - Mutex-protected thread-safe audio→UI communication
  - Smooth visual transitions (0.7 smoothing factor)
  - Gradient fill with olive/brown color scheme
  - Grid overlay for frequency reference

### Technical Details

- FFT computed when 2048 samples accumulated (mono sum of L/R)
- Magnitude values converted to normalized dB scale (-80 to 0 dB → 0 to 1)
- 30 Hz UI update rate with conditional send (only when new data ready)
- Error handling in JavaScript to prevent UI crashes

---

## Version 1.1.0 (2026-01-25)

### Added

- **Draggable Crossover Controls:** Click and drag crossover lines to adjust frequency split points directly in the UI
  - XOVER1 (Low/Low-Mid): 20-500 Hz
  - XOVER2 (Low-Mid/High-Mid): 200-5000 Hz
  - XOVER3 (High-Mid/High): 2000-16000 Hz
- Ordering constraints enforced during drag (XOVER1 < XOVER2 < XOVER3 with 100 Hz minimum gap)
- Visual feedback: hover highlights, drag state with green accent, enlarged handles
- Touch support for tablet/trackpad use
- Proper JUCE drag start/end notifications for undo/redo grouping

### Changed

- Crossover lines now have smooth hover transitions
- Disabled CSS transitions during drag for instant visual response

---

## Version 1.0.0 - Stage 3 Complete (2026-01-25)

**PRODUCTION READY:** All stages complete - build system, DSP processing, and GUI with real-time metering functional.

### Phase 5.3 - Real-Time Metering (2026-01-25) - FINAL PHASE

**Metering Infrastructure:**
- Timer callback at 30 Hz for smooth updates
- 4 per-band gain reduction meters (LOW, LOMID, HIMID, HIGH)
- GR meters display 0 to -24 dB compression activity
- Input/output level meters with stereo averaging
- Peak level detection using buffer.getMagnitude()
- Atomic floats for thread-safe communication (audio → UI)
- JavaScript meter update functions
- CSS transitions for smooth animations (30-50ms ease-out)

**Crossover Visualization:**
- Dynamic crossover line positioning
- Reads XOVER1/XOVER2/XOVER3 parameters in real-time
- Logarithmic frequency-to-position mapping (20 Hz - 20 kHz)
- Auto-updating frequency labels with Hz/kHz formatting

**Performance:**
- 30 Hz UI refresh rate (33ms interval)
- will-change CSS optimization for 60fps rendering
- No UI thread starvation
- Stable in DAW during playback

### Phase 5.2 - Parameter Binding (2026-01-25)

**Parameter Bindings:**
- 56 WebSliderParameterAttachment (8 global + 48 per-band)
- 13 WebToggleButtonParameterAttachment (1 global + 12 per-band)
- 1 WebComboBoxParameterAttachment (MS_MODE)
- All relays registered with .withOptionsFrom()
- Bidirectional sync: UI ↔ APVTS
- Automation support verified
- Preset save/load functional

**UI Controls:**
- Per-band knobs: Threshold, Ratio, Attack, Release, Knee, Makeup (24 total)
- Per-band buttons: Solo, Bypass, SC Listen (12 total)
- Global knobs: Input, Output, Mix (3 total)
- Global toggle: Auto-Makeup
- Global select: M/S Mode
- Value formatting: dB, ms, ratio, %

### Phase 5.1 - WebView Layout (2026-01-25)

**UI Structure:**
- WebView integration using juce::WebBrowserComponent
- Botanical/Ouaricon aesthetic (900x600px)
- HTML/CSS layout with 4-band sections
- Spectrum analyzer placeholder with grid
- Input/output meter placeholders
- Per-band GR meter placeholders
- Crossover visualization (3 handles)
- JUCE JavaScript bridge
- Resource provider for embedded assets
- Color-coded bands (brown, green, gold, orange)

### Phase 4.3 - Advanced Features (2026-01-25)

**Sidechain Filtering:**
- Per-band sidechain HPF (20-2000 Hz)
- Per-band sidechain LPF (500-20000 Hz)
- Filters applied to detector path only
- SC Listen mode for monitoring filtered signal

**Mid/Side Processing:**
- M/S encoding/decoding (power-preserving with sqrt(2))
- 4 processing modes: Off/Mid/Side/Both
- Both mode = 8 independent compressors (mid + side per band)

**Global Features:**
- Auto-makeup gain (80% compensation, 500ms smoothing)
- Dry/wet mixer (juce::dsp::DryWetMixer)
- Parallel compression capability
- All 56 parameters fully integrated

### Phase 4.2 - Crossover Network (2026-01-25)

**Multiband Architecture:**
- Linkwitz-Riley 4th order crossover (24 dB/oct)
- 3 crossover points with frequency validation
- Cascaded 2nd order Butterworth filters (12 IIR per channel)
- 4-band output: LOW, LOMID, HIMID, HIGH
- Flat magnitude summing (Linkwitz-Riley property)
- 4 independent compressor instances
- Per-band solo/bypass routing
- Gain reduction metering for all 4 bands

### Phase 4.1 - Compressor Foundation (2026-01-25)

**Compression Engine:**
- Feed-forward compressor topology
- Soft knee with quadratic interpolation (0-24 dB)
- Peak detector (absolute value)
- RMS detector (circular buffer, 10ms window)
- Peak/RMS blend (0-100% continuous)
- Envelope smoother with attack/release ballistics
- Input/output gain stages
- Bypass logic
- Stereo-linked compression
- Gain reduction metering (atomic float)

### Stage 1 - Foundation Complete (2026-01-25)

**Build System:**
- Created CMakeLists.txt with JUCE 8 configuration
- Plugin code: OMbc (4 chars)
- Manufacturer code: OuAu (Ouaricon Audio)
- Formats: VST3, AU, Standalone
- NEEDS_WEB_BROWSER TRUE for future WebView UI
- juce_dsp module added for DSP components

**Parameters (56 total):**
- 8 global parameters implemented (INPUT_GAIN, OUTPUT_GAIN, MIX, AUTO_MAKEUP, MS_MODE, XOVER1, XOVER2, XOVER3)
- 48 per-band parameters implemented (12 per band × 4 bands)
- Band prefixes: LOW, LOMID, HIMID, HIGH
- All parameters use JUCE 8 ParameterID format
- Logarithmic scaling for crossover frequencies (0.3 skew factor)
- State management (save/load) implemented

**Audio Processing:**
- PluginProcessor with pass-through audio (no DSP yet)
- Stereo input/output bus configuration
- prepareToPlay/releaseResources stubs ready for DSP
- Real-time safe parameter access example provided

**UI:**
- PluginEditor placeholder (900x600)
- Shows plugin name and parameter count
- WebView integration pending Stage 3

---

## Stage 0 - Research & Planning (2026-01-25)

**Research:**
- Professional multiband compressor examples studied (FabFilter Pro-MB, Waves C6, UAD Precision Multiband)
- JUCE modules identified for implementation
- DSP feasibility verified

**Architecture:**
- Linkwitz-Riley 4th order crossovers (24 dB/octave)
- Feed-forward compressor topology with soft knee
- Peak/RMS blend detection
- Mid/Side processing modes
- Per-band sidechain filtering
- Real-time FFT spectrum analyzer
- Auto-makeup gain with slow ballistics
- Global dry/wet mixer

**Complexity Assessment:**
- Score: 5.0 (Maximum complexity)
- 56 parameters
- 10 DSP components
- Phased implementation strategy defined

**Contracts Created:**
- architecture.md (complete DSP specification)
- plan.md (phase breakdown with test criteria)
- creative-brief.md (original concept)
- parameter-spec.md (56 parameter definitions)
