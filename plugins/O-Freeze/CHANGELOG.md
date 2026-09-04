# Changelog

All notable changes to O-Freeze will be documented in this file.

## [2.4.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `ofrz.tipsEnabled`.
- **`data-tip-always` on `#gear-btn` and on `#tips-toggle`, and on nothing
  else.** Those two controls are the ones that REACH and RESTORE the help
  layer, so they keep explaining themselves while it is off. `#lang-select`
  deliberately does not carry it: it is only reachable through the gear, which
  already explained itself on the way in.
- **Five i18n keys, four of them settled roots copied rather than authored.**
  `label.hoverHelp`, `ui.on`, `ui.off` and `aria.helpToggle` take the French
  glossary roots verbatim from `scripts/i18n-fr-glossary.js` — *Aide au survol
  / Marche / Arrêt / Activer ou désactiver l'aide au survol*. The fifth,
  `tip.tipsToggle`, is the tooltip's own title and body.

### Measured

- **The second row costs nothing.** With the popover forced open it occupies **y 42..104, 184 x 62 px — byte-identical in English and French** — inside a 550 x 530 frame. The switch face grows from **42.00 px to 43.61 px** for *Marche*, leftward into the panel's own slack; the right edge does not move and `check-ui-labels` [7] reports 0 non-label elements displaced. This page is the suite's one DARK settings plate, so the pressed state inverts to the plugin's own rgba(139, 115, 85, 0.85) hairline colour over #2a231c ink rather than to a green borrowed from a paper-ground sibling.
- **The switch's face is a `min-width: 42px` floor, not a pinned width**, so a
  longer French face grows LEFTWARD into slack the popover already has. The row
  is `space-between` and the button is a `[data-i18n]` node, so nothing the
  geometry gate measures moves. `check-ui-labels` [7] reports **0 non-label
  elements displaced** between English and French, and the visible element set
  identical in both.
- Every declaration in `.settings-toggle` above the four switch-specific ones
  is **copied from this page's own language `<select>`** — its font stack, ink,
  plate, hairline and radius — so the two controls in the popover match by
  construction rather than by a second designer re-deciding them.

### Decided

- **Default is ON.** The previous version showed hover help unconditionally, so
  ON is the setting that leaves an existing user's plugin behaving exactly as
  it did. Default OFF would additionally have made `boot-all-uis --strict-tips`
  measure an empty tip surface and call it correct.


## [2.3.0] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed
- **item 48 — Detune knob is now true cents:** the knob is labelled in
  cents and its tooltip says "0 to 50 cents", but `PluginProcessor.cpp:582`
  mapped it linearly — `playbackRate = 1 + r·(cents/1200)` — and a cent is
  `2^(1/1200)`. Knob 50 therefore spread grains across +70.67 / −73.68 ct,
  not ±50; knob 25 reached +35.7 / −36.4; knob 5 reached ±7.2. The map is now
  `2^(r·cents/1200)`; the random spread `r` (uniform in ±1, drawn per grain)
  is untouched. Measured on a frozen 440 Hz sine with one seed both sides, so
  every grain pairs up: at knob 50, 44 grains went from +63.08 / −68.98 ct to
  +45.59 / −47.01 (per-grain ratio 1.29–1.63, median 1.45); over 209 grains
  the extremes went from +69.59 / −72.51 to +49.50 / −49.96. After the fix a
  grain's pitch at knob 25 is 5.00× its pitch at knob 5 and at knob 50 it is
  10.00× — the knob is linear in cents, which it was not before.
- **Detune no longer snaps to a 3.4 ct grid** (found by that measurement).
  Each grain's read position was a `float` accumulator; at a 1000 ms grain
  (44 100 samples) a float's step is 1/256 of a sample, so the per-sample
  rate rounded to a multiple of 1/512 and every grain's pitch was quantised
  to multiples of 3.38 ct (1.69 ct at the default 400 ms) while the knob
  offers 0.1 ct. At knob 5 the grains sat at 0, ±3.38 or ±6.75 ct and nowhere
  between. `Grain::fractionalPosition` is now `double`; at knob 5 the measured
  grains are now continuous (+0.27 … +4.56 ct).

### Changed
- **The sound of an existing session changes at the same Detune value:** the
  spread is narrower by 2^(c/1200) against 1 + c/1200 — knob 50 is now
  ±50 ct, was +70.67 / −73.68 ct (1.41× / 1.47×); knob 25 is ±25, was
  +35.7 / −36.4; knob 5 is ±5, was ±7.2 — and small values are no longer
  stepped. To keep the old width, turn the knob up by about 1.4×. Minor bump
  for that reason.

### Notes
- No parameter ID, range, default, preset format or state key changed;
  sessions load and the readout shows what it did. No English or French copy
  changed: "0 to 50 cents" / "De 0 à 50 cents" was the claim the DSP failed
  to meet, and is now true.
- Verified with a scratchpad offline render, not committed: the processor
  driven at block size 1, a 440 Hz sine frozen, GRAIN_COUNT 2, DRIFT 0, LFO
  depth 0, pitch read by zero-crossings over the stretch where one grain is
  alone in the overlap-add. O-Freeze has no committed render harness.

## [2.2.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed
- **23 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint, which went from 38 findings to
  0 under `--strict`. Four captions took the settled suite term — *DOSAGE →
  Mix*, *ÉCART → Désacc.*, *INVERSE → Invers.*, *LFO DÉRIVE → LFO de dérive* —
  and their tooltips now carry the root word in full (*Inversion*,
  *Désaccord*, *Profondeur*) where the caption is an abbreviation forced by a
  60 px knob cell. Eleven captions dropped their shouted all-caps to follow
  the English caption's casing, which is invisible on screen (every caption
  element here is `text-transform: uppercase`, measured to the hundredth) but
  visible to assistive technology. Eleven tooltip bodies gained French
  typography: no-break spaces before `;` and `%` and between a number and its
  unit (*0 dB*, *1000 ms*, *10 Hz*, *50 cents*, *100 %*).
- **Two tooltip bodies say something they did not.** Detune's now spreads the
  pitch of *each* grain rather than "of grains", matching the English, and
  thickens the freeze into an *effet de chorus* rather than an *effet de
  chœur* — a chorus is the effect, a chœur is a choir. Drift's now smears the
  texture into *un nuage* rather than *une nappe*, which the Grains tooltip
  already uses for a pad.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Notes
- No English copy, key, tip binding, selector, CSS rule or `I18N_EXEMPT` entry
  changed. `reviewed: false` stays on all 31 French entries: it records that a
  native speaker has read them, and none has.
- Threshold's tooltip keeps its English title, now under a `termNote` giving
  the reason: the knob's own caption is static English because "Threshold" is a
  MODE `AudioParameterChoice` option string byte for byte, so a tip headed
  *Seuil* over a knob captioned THRESHOLD would name one control twice.

## [2.2.0] - 2026-08-30

Hover-help, in both languages. Stage M batch M2 of the repo-wide i18n rollout.

### Added
- **Fourteen hover tooltips.** One per parameter — all twelve in
  `.planning/params.tsv` have a control on this page — plus the gear and the
  language selector. Each carries an English and a French `{title, body}`, and
  each body says what the control does, when to reach for it, and its range.
- **A tooltip renderer.** v2.1.0 had no `#tooltip` node, no `.tooltip` rule and
  no hover handler, so authoring copy alone would have shipped twenty-eight
  invisible strings: `applyI18n()` writes `data-tip-title` and `data-tip`
  ATTRIBUTES and stops there. The renderer is delegated on `document`, follows
  the cursor, flips to the other side of it and then clamps on all four edges
  with an 8 px margin, and builds its content with `createElement` +
  `textContent` — never `innerHTML`, so localized copy cannot reach a markup
  path.
- **A focus latch on the keyboard.** A mouse click on a `<button>` focuses it,
  so an unconditional `focusin` rule parks a tip on screen after every click.
  Hover-help opens on focus only when the last input was NOT a pointer.
  `:focus-visible` is deliberately not the discriminator: Chromium reports it
  false for a programmatic `.focus()` after a click, which would make a test
  driving focus measure "no tip" and record that as correct.
- **`tests/ui_tip_render_check.js`.** No existing gate in this repo can see a
  rendered tooltip — `check-i18n` reads the table statically, `check-ui-labels`
  has no tooltip awareness at all, and `boot-all-uis` counts `aria-label` and
  `title` but never `data-tip`. 311 assertions at the shipping 550 x 530 frame,
  in `en` / `fr` / `en`.
- **`.planning/params.tsv`.** A runtime walk of `AudioProcessor::getParameters()`
  through the `scripts/param-dump` console target, wired behind
  `OUARICON_BUILD_TESTS` (OFF by default). It is the authoritative parameter
  inventory that the tooltip ranges are written from; a regex over
  `createParameterLayout()` is not.

### Changed
- `PluginProcessor.cpp` no longer includes `PluginEditor.h` at the top of the
  translation unit. The include now sits behind `#if JUCE_WEB_BROWSER`
  immediately above `createEditor()`, with a `GenericAudioProcessorEditor`
  fallback, so the param-dump console target — which builds with
  `JUCE_WEB_BROWSER=0` and no editor sources — links. Under a normal build
  `JUCE_WEB_BROWSER=1` and behaviour is byte-identical to v2.1.0.

### Not changed
- **No hover-help on/off switch.** The settings popover still holds one row.
  Two plugins in the suite have such a toggle and nineteen do not; making that
  uniform is a separate pass across all 41, not a side effect of adding copy.
- **No parameter IDs, ranges, types, defaults or DSP behaviour.** Presets and
  automation from 2.1.0 load unchanged.
- **No geometry.** `check-ui-labels --plugin O-Freeze` is BYTE-IDENTICAL before
  and after this version, in both driven states: the surface is
  `position: fixed` and hidden at rest, so it neither moves anything nor enters
  the label sweep. `boot-all-uis` reports the same `text=25 aria=3 title=0
  i18n=15` it did at v2.1.0. **No geometry pin was added, so none is owed a
  negative control.** The two v2.1.0 pins are untouched.

### Deliberately English inside a French tooltip
`Manual`, `Threshold`, `Sine`, `Triangle` and `Random` are
`AudioParameterChoice` option strings. The option strings themselves stay
English so the page and the host automation lane agree about the same setting;
the French SENTENCE naming them is French. `tip.threshold`'s TITLE also stays
English in French, matching the caption below it, which v2.1.0 already exempts
for the same reason: the knob and the mode button name the same setting, and a
tip headed `SEUIL` over a knob captioned `THRESHOLD` describes one control as
two.

### French
All 14 new tooltip entries are machine drafts flagged `reviewed: false`, taking
the plugin's unreviewed total from 17 to 31. Bodies are prose and follow French
convention — decimal comma (`0,01 à 10 Hz`), a space before `%`, U+2212 for the
minus. The value READOUTS keep their decimal point, because D-03 exempts the
readout node and not the sentence describing it.

## [2.1.0] - 2026-08-28

The PAGE speaks French. Stage K batch 1 of the repo-wide i18n rollout, canon v2.

### Added
- **English/French UI.** Fifteen `[data-i18n]` label elements and three keyed
  `aria-label` attributes resolve through a new `Source/ui/public/js/i18n.js`.
  A settings popover in the header's top-right carries the language selector.
- **Language persistence.** `getUiLanguage` / `setUiLanguage` native functions and
  a `uiLanguage` property on the APVTS state tree, saved with the session. It is
  deliberately NOT a parameter: it must not appear in a DAW automation lane, and a
  preset must not change which language somebody reads their plugin in. Read back
  through an `isVoid()` guard, because the ValueTree XML round-trip rebuilds every
  property as a `var` over the attribute STRING.

### Not changed
- **No hover-help.** v2.0.1 carried no `data-tip` and no native `title=` anywhere,
  and none is invented here — authoring that copy is a later stage. `TIP_BINDINGS`
  and `I18N` are both empty, which the gate reports as "0 tip(s) bound" rather than
  passing silently.
- **No parameter IDs, ranges, types, defaults or DSP behaviour.** Presets and
  automation from 2.0.1 load unchanged.

### Deliberately English
Three visible strings stay English, each an `I18N_EXEMPT` entry with its reason:
`Manual` and `Threshold` are the MODE `AudioParameterChoice` option strings byte
for byte, and the `Threshold` knob caption names the same setting as the button
beside it. The header is a product name.

### Geometry
Six knobs share a 530 px `space-around` row whose items are floored at 60 px by
`.knob-value`'s `min-width`, so every French caption was chosen against a measured
60.00 px budget: `DÉSACCORD` (70.1 px) moves 37 boxes and `PROFONDEUR` (87.3 px)
moves 10, both verified by reverting the choice and re-running the gate. Two
per-element pins hold a French string that renders NARROWER than its English
original — `#reverse-toggle` and the third LFO shape option — and each was reverted
alone and confirmed to re-break the geometry diff. English before/after: 0 of 89
elements moved, 2 added.

## [2.0.1] - 2026-07-01

Resolves five findings from the 2026-07-01 deep code review (`.planning/CODE-REVIEW.md`).
All fixes are DSP-internal — no parameter IDs, ranges, or state format changed (presets
and automation remain compatible).

### Fixed
- **CR-01 (RT safety):** WSOLA cross-correlation search span was `grainSize / 4`, reaching
  ~48,000 samples at 192 kHz / 1000 ms grains — millions of multiply-adds plus thousands of
  `sqrt` calls executed synchronously inside a single `processBlock` sample iteration,
  blowing the audio deadline (xruns/dropouts) at small host buffers. Capped the search to a
  5 ms window (`jmin(grainSize / 4, sampleRate * 0.005)`), independent of grain size.
- **WR-01 (threshold detection):** Stereo RMS detector wrote each channel serially into the
  circular window, halving the effective window (~10 ms instead of 20 ms) and interleaving
  L/R out of temporal order. Now sums channels to a mono value per time instant before
  pushing one squared sample, restoring the documented 20 ms mono window.
- **WR-02 (click on rapid toggle):** `freezeGain.reset()` internally calls
  `setCurrentAndTargetValue()`, snapping the gain to its old target and discarding an
  in-progress fade — an amplitude click when freeze is re-toggled mid-fade. Now snapshots
  the current gain and restores it across the `reset()` on both the fade-in and fade-out
  branches, so the new ramp starts from the true current level.
- **WR-03 (corrupted release tail):** On freeze release the write head resumed advancing
  (gated only on `bufferFrozen`) while active grains kept reading the buffer for the up-to-
  ~1 s fade-out, overwriting the region being read. Introduced a per-sample `renderingTail`
  flag (`bufferFrozen || freezeGain > 0.001`) that suppresses live writes and freezes the
  write head for the entire release tail.
- **WR-04 (latent OOB):** `prepareToPlay` computed `grainSize` without the `maxGrainSize`
  clamp that `processBlock` applies, so the initial Hann-window build loop was safe only by
  the coincidence that the GRAIN_SIZE max (1000 ms) equals `maxGrainSize`. Mirrored the
  `jmin(..., maxGrainSize)` clamp for defense-in-depth.

### Notes
- Remaining review findings IN-01…IN-05 (micro-optimizations, UI value-box precision,
  round-robin grain reuse, zero-channel guard, resource-provider log noise) are deferred as
  non-blocking.

## [2.0.0] - 2026-04-04

### Added
- First public release
- Licensing overlay integration (conditional compilation)

### Changed
- Simplified UI update functions (single generic handler)
- Inlined PI constants, hoisted grain scratch arrays to class members
- Removed dead `readPosition` member

### Fixed
- Stereo RMS threshold calculation (~3dB too low)

## [1.9.5] - 2026-04-04

### Changed
- **Replaced 8 identical one-liner knob update functions** (`updateThresholdUI`, `updateDriftUI`, `updateGrainSizeUI`, `updateGrainCountUI`, `updateDetuneUI`, `updateMixUI`, `updateLfoRateUI`, `updateLfoDepthUI`) with single generic `updateKnobUI(knobId)`
- **Replaced manual `classList.add`/`remove` with `classList.toggle(cls, condition)`** in `updateFreezeUI`, `updateReverseUI`, `updateModeUI`, `updateLfoShapeUI`
- Collapsed verbose 3-line listener registrations to one-liner arrows

## [1.9.4] - 2026-04-04

### Changed
- **Inlined `juce::MathConstants<double>::pi`** — removed duplicate `const double PI` locals in `prepareToPlay` and the grain-size-change block
- **Moved per-sample grain scratch arrays to class members** — `windowValues`, `grainPos0`, `grainPos1`, `grainFrac` (all `MAX_GRAINS`-sized) were stack-allocated and zero-initialized every sample inside the inner loop; now persistent members with no per-sample init cost

## [1.9.3] - 2026-04-04

### Changed
- **Removed dead `readPosition` member** — declared in PluginProcessor.h and assigned in `prepareToPlay` but never read; vestige of pre-granular design where a single read head traversed the freeze buffer (each grain now tracks its own position via `Grain::position` and `Grain::fractionalPosition`)

## [1.9.2] - 2026-04-04

### Fixed
- **RMS threshold calculation ~3dB too low in stereo** — `rmsSamplesPerWindow * numChannels` divided by 2x the actual buffer size since the circular buffer holds `rmsSamplesPerWindow` interleaved entries (both channels), not `rmsSamplesPerWindow` per channel
- Root cause: divisor assumed per-channel buffer size, but write index wraps at `rmsSamplesPerWindow` regardless of channel count, so stereo writes interleave into the same buffer
- Effect: threshold gate triggered ~3dB earlier than the knob indicated in stereo; mono operation was unaffected

## [1.9.0] - 2026-04-04

### Added
- **Per-grain pitch micro-detuning** (DETUNE, 0–50 cents, default 5, step 0.1) — each grain receives a random playback rate offset that spectrally decorrelates overlapping grains for richer, chorus-like frozen textures
- **Linear interpolation on grain reads** — fractional sample positions produce smooth pitch-shifted output without aliasing artifacts
- Detune knob in WebView UI between Grains and Mix knobs

### Changed
- Grain position advancement replaced from integer increment to fractional accumulator (`fractionalPosition += playbackRate` per sample)
- Editor width increased from 500px to 550px to accommodate 6-knob row

### Technical Notes
- `playbackRate` stored per-grain: `1.0 + random(±1) * (cents / 1200)` — cents-to-rate conversion
- First grain at freeze engage always gets `playbackRate = 1.0` (no detune) for clean initial capture
- `fractionalPosition` (float) accumulates playback rate; integer part selects buffer sample, fractional part drives linear interpolation between `floor` and `floor+1`
- At max detune (50 cents), playback rate varies ±0.0417 — subtle pitch shift with effective spectral decorrelation
- WSOLA tail capture updated to compute end position from `base + int(fractionalPosition)` rather than advanced integer position
- Reverse playback direction fully supported (interpolation direction flips correctly)
- No breaking changes — existing presets default to 5 cents detune; set to 0 for previous behavior
- Float precision sufficient: max `fractionalPosition` ≈ 49000 at 1000ms/48kHz, well within float mantissa range

## [1.8.0] - 2026-04-04

### Added
- **WSOLA-style best-overlap grain positioning** — when activating a new grain, searches ±(grainSize/4) samples around the nominal position to find the offset that maximizes normalized cross-correlation with the previous grain's tail, producing smoother transitions on pitched/tonal content
- Cross-correlation computed on 64-sample mono segments (both channels summed), stepping by 4 samples for CPU efficiency
- First grain after freeze engage uses nominal position (no previous tail to compare against)
- Per-grain jitter offset applied ON TOP of WSOLA-chosen position (both features stack)

### Technical Notes
- `lastGrainTail[64]` member stores final 64 samples of each completed grain (mono, playback order)
- `hasLastGrainTail` flag reset on freeze engage, set true on first grain completion
- Search window: `grainSize/4` samples in each direction, step 4 → ~1100 xcorr evaluations per grain activation at 400ms grain size
- Normalized cross-correlation: `sumXY / sqrt(sumX2 * sumY2)` with epsilon guard (1e-6)
- `sumX2` (tail energy) precomputed once per search — constant across all offsets
- Tail capture handles both forward and reverse playback directions
- No new parameters — transparent improvement to existing granular engine
- Existing presets unaffected (WSOLA only adjusts internal grain start positions)

## [1.7.1] - 2026-04-04

### Fixed
- **Stochastic grain trigger timing** — grain trigger intervals now randomized ±30% from nominal to break up periodic comb-filter artifacts caused by perfectly regular grain spacing
- Minimum trigger interval clamped to `grainTriggerInterval / 2` to prevent grain stacking

### Technical Notes
- New `nextTriggerInterval` member computed each time a grain fires: `grainTriggerInterval + random(±30%)`
- Comparison changed from fixed `grainTriggerInterval` to jittered `nextTriggerInterval`
- `nextTriggerInterval` reset to nominal when grain parameters change or `prepareToPlay` called
- No new parameters — transparent improvement to existing granular engine
- Existing presets unaffected (timing jitter is purely internal, no parameter changes)

## [1.7.0] - 2026-04-04

### Added
- **Per-grain position jitter** — each grain receives a random buffer offset at activation, decorrelating overlapping grains for richer, less phasey frozen textures
- Jitter range scales with grain size and DRIFT parameter: `jitterOffset = random * grainSize * driftValue`
- First grain activated at freeze engage always gets `jitterOffset = 0` for clean initial capture

### Changed
- **Replaced COLA normalization with per-sample window normalization** — granular sum divided by sum of active Hann window values each sample, maintaining constant amplitude regardless of grain phase alignment
- Removed COLA scaling factor (`2/N`) from Hann window; raw Hann values stored instead
- Drift parameter now serves dual purpose: controls both shared drift range and per-grain jitter spread

### Technical Notes
- `jitterOffset` stored per-grain in Grain struct (int, computed once at activation)
- Jitter uses existing `driftValue` (0–1) so drift=0 means zero jitter (backward compatible)
- Window normalization: `frozenSample = granularSum / windowSum` with epsilon guard (1e-6) to avoid division by zero
- No new parameters — reuses DRIFT for jitter control
- Existing presets unaffected when drift=0 (jitter range is zero)

## [1.6.0] - 2026-04-04

### Added
- **Reverse playback mode** (REVERSE, toggle, default off) — reads grains backwards through the freeze buffer when enabled
- Grain start positions calculated from end of grain region, read index decrements instead of incrementing
- Hann window envelope remains forward (fade-in then fade-out) regardless of read direction — preserves COLA compliance
- Toggle button in WebView UI below the freeze button, matching existing botanical aesthetic

### Technical Notes
- REVERSE parameter read once per processBlock (atomic, real-time safe)
- Reverse affects both initial freeze grain and subsequent triggered grains
- Drift offset applied identically in both directions (read-time application preserved)
- Can toggle reverse while frozen — grains change direction immediately
- Existing presets default to off (non-breaking — playback behavior unchanged)

## [1.5.1] - 2026-04-03

### Changed
- Moved freeze button up 25px for better visual balance
- Lightened Drift LFO group box background for better visibility
- Darkened all font colors for richer, more saturated text appearance

## [1.5.0] - 2026-04-03

### Added
- **LFO modulation for Drift parameter** with 3 new controls:
  - **LFO Rate** (LFO_RATE, 0.01–10 Hz, default 0.5 Hz) — skewed range for fine control at low frequencies
  - **LFO Depth** (LFO_DEPTH, 0–100%, default 50%) — scales LFO influence on drift offset
  - **LFO Shape** (LFO_SHAPE, Sine/Triangle/Random) — waveform selector; Random uses sample-and-hold (new value each cycle)
- LFO group in WebView UI: two small knobs (Rate, Depth) + shape toggle, grouped below existing knob row with "Drift LFO" label
- Editor height increased from 450px to 530px to accommodate LFO group

### Technical Notes
- LFO computed once per processBlock (per-block, not per-sample) for efficiency
- LFO phase accumulates by `rate * blockSize / sampleRate` per block
- LFO output (-1 to +1) scaled by depth and added to frozen drift offset, clamped 0–1
- COLA phase alignment preserved — all grains still share the same modulated drift offset
- LFO only active while frozen and depth > 0 (no unnecessary computation)
- Existing presets default to 50% depth with Sine shape (non-breaking — drift behavior unchanged when depth is 0)

## [1.4.0] - 2026-04-03

### Added
- **Grain Count parameter** (GRAIN_COUNT, 2–32, default 8, integer steps) — replaces hardcoded 8-grain system with dynamic grain pool
- Rotary knob in WebView UI matching existing botanical aesthetic (labeled "Grains")

### Changed
- Granular engine dynamically recalculates hop size (`grainSize / grainCount`) and COLA scaling factor (`2.0 / grainCount`) on parameter change
- Grain array expanded to MAX_GRAINS=32; only `grainCount` grains are activated from the pool
- Grains beyond active count are cleanly deactivated on count reduction; `nextGrainIndex` clamped to new range
- Editor width increased from 450px to 500px to accommodate 5 knobs
- Grain Size knob label shortened from "Grain" to "Size" to avoid confusion with new "Grains" knob

### Technical Notes
- COLA identity preserved for all grain counts: N Hann windows at (1−1/N) overlap sum to N/2, scaled by 2/N → unity
- Existing presets default to 8 grains (non-breaking — matches previous hardcoded behavior)
- No audio-thread allocations: window buffer pre-allocated for MAX_GRAINS=32, Hann window pre-allocated for max grain size (1000ms)

## [1.3.1] - 2026-04-03

### Fixed
- `releaseResources()` now properly clears freeze buffer, resets all grain states, and resets crossfade gain (was empty stub from Stage 2)
- MODE relay changed from `WebToggleButtonRelay` to `WebComboBoxRelay` to correctly match the `AudioParameterChoice` type; JS updated from boolean `getToggleState` to index-based `getComboBoxState`
- DRIFT parameter default changed from 25% to 0% to match documentation
- STATUS.md Known Issues updated to reflect drift clicking was resolved in v1.2.2

## [1.3.0] - 2026-04-03

### Added
- **Grain Size parameter** (GRAIN_SIZE, 50ms–1000ms, default 400ms) — replaces hardcoded 400ms grain size
- Rotary knob in WebView UI matching existing botanical aesthetic

### Changed
- Granular engine dynamically recalculates grain size, hop size (grainSize/8), and Hann window on parameter change
- COLA scaling factor computed from NUM_GRAINS (2/NUM_GRAINS) for unity gain at any grain size
- Release fade time now tracks current grain size instead of hardcoded 400ms
- Hann window pre-allocated for max grain size (1000ms) — no audio-thread allocations on parameter change
- Active grains past new grain boundary are deactivated cleanly on size reduction

### Technical Notes
- COLA identity preserved: 8 Hann windows at 87.5% overlap sum to NUM_GRAINS/2 = 4.0, scaled by 0.25 → unity
- Overlap ratio (87.5%) is grain-size-independent — only depends on NUM_GRAINS
- Parameter change detection via `lastGrainSizeMs` comparison avoids per-block recalculation

## [1.2.2] - 2026-02-03

### Fixed
- **Drift clicking eliminated** - Two-part fix for granular synthesis artifacts
  1. Removed per-sample normalization that caused amplitude modulation (dividing by fluctuating `windowSum`)
  2. Lock drift offset when freeze engages - all grains now share identical position offset for proper COLA phase alignment
  - Root cause: Drift smoothing continued while frozen, causing each new grain to start at a slightly different buffer position. This broke COLA's phase alignment requirement, creating progressively worse clicking as drift wandered.

### Changed
- Replaced custom trapezoidal window with standard Hann window (pre-scaled by 0.25 for unit COLA sum)
- Drift applied at READ time (not activation time) so all grains shift together

### Technical Notes
- COLA identity: 8 Hann windows offset by N/8 each sum to exactly 4.0 at every sample point
- Window scaled by 0.25 so overlapping grains sum to 1.0 without per-sample division
- All grains must read from phase-aligned positions - shared drift offset ensures this

## [1.2.1] - 2026-02-03

### Fixed
- **Drift clicking eliminated** - Replaced per-grain random offsets with smoothed drift
  - Root cause: Each grain received an independent random position offset, causing phase discontinuities between overlapping grains that Hann windowing couldn't smooth
  - Solution: All grains now share a slowly-interpolating drift offset that wanders organically over ~500ms, maintaining phase coherence while preserving texture variation

### Changed
- Reduced grain count from 12 to 8 (87.5% overlap, proper COLA compliance)

### Technical Notes
- `currentDriftOffset` smoothly interpolates toward `targetDriftOffset` (new random target every 500ms)
- Smooth coefficient ~0.0005 gives ~50ms convergence for click-free transitions
- Drift parameter still controls range of movement, but movement is now continuous rather than discontinuous

## [1.2.0] - 2026-02-03

### Fixed
- Removed incorrect COLA normalization (dividing by active grain count caused amplitude pumping)

### Changed
- Increased grain size from 200ms to 350ms for smoother, more lush frozen textures
- Increased grain count from 8 to 12 for denser overlap (91.7% vs 87.5%)
- Extended release fade from 250ms to 400ms to accommodate longer grains

### Technical Notes
- COLA (Constant Overlap-Add) now works correctly: overlapping Hann windows sum to ~1.0 without division
- 12 grains with 91.7% overlap provides more continuous, artifact-free frozen sound
- Longer grains = slower envelope = gentler transitions

## [1.1.0] - 2026-02-02

### Changed
- Replaced asymmetric Blackman-Harris window with symmetric Hann window for warmer, smoother frozen textures
- Implemented staggered grain activation on freeze engage (eliminates burst sound)
- Implemented soft grain deactivation on freeze release (eliminates release clicks)
- Extended release fade from 100ms to 250ms to cover grain completion time

### Technical Notes
- Domain: DSP
- Milestone: eliminate-clicks-smooth-windowing
- Window now has true zero at endpoints for artifact-free grain boundaries
- COLA compliance verified at 87.5% overlap (8 grains)
- Debug assertion added to verify COLA sum ≈ 1.0

## [1.0.1] - 2026-02-02

### Fixed
- Smooth knob animation in WebView UI

## [1.0.0] - 2026-02-01

### Added
- Initial release
- Granular freeze effect with 8 overlapping grains
- Manual and Threshold freeze modes
- Drift parameter for texture variation
- Mix control for dry/wet blend
- WebView-based UI with naturalist aesthetic
