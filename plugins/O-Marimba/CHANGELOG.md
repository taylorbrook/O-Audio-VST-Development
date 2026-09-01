# O-Marimba Changelog

All notable changes to this project will be documented in this file.

## [1.13.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout: every French entry
read against its English and against the suite glossary
(`scripts/i18n-fr-glossary.js`) and the French lint
(`scripts/i18n-fr-lint.js`). PATCH: no parameter, no DSP, no layout and no CSS
rule changed — 24 French strings, a header comment and two version sites.

### Changed
- **24 French entries revised** against the suite glossary and lint — 9
  terminology, 12 typography, 2 meaning, 1 register — taking the lint from 31
  findings to 0 (`--strict` exit 0). The visible ones: the TUNING tab reads
  **ACCORD** rather than GAMME, so it agrees with its own tooltip "Mode
  d'accord"; the material knob reads **DURETÉ / MATÉRIAU** rather than DURETÉ /
  MATIÈRE, with the tooltip following it; the hover-help row in the settings
  popover reads **Aide au survol** rather than "Aide"; the scale circle reads
  **Intervalles de la gamme**, the settled suite form, rather than "Intervalles
  de gamme"; and straight spaces before `%`, `:` and `;` and between a number
  and its unit became **no-break spaces**, so "0 %", "440 Hz" and "Tonique :"
  can no longer break across a line.
- **Two French tooltips named a button that does not exist in French.** The
  interval-list and tuning-mode bodies said "En mode CUSTOM"; the button is
  captioned **PERSO** in French. 12-TET and MTS-ESP are unchanged — those two
  are the parameter's own option strings and stay English in both languages.
- **The A4 tooltip said *accords historiques***, which reads as historical
  *chords* in a sentence about 415 Hz and 432 Hz. It now says *accordages
  historiques*.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

### Notes
- `reviewed: false` is unchanged on all 69 entries. That flag records a native
  speaker's reading; this pass is a second machine reading against a glossary
  and a lint, and it is recorded in the `i18n.js` header instead.
- Nine entries carry a new `termNote` — a reasoned glossary exemption. Seven of
  them are the four two-line knob captions, where French inverts head and
  modifier so the caption is the PAIR and not the row; two are the .SCL/.KBM
  load buttons, where the glossary's `charger .scl` overflows the button row by
  15.91 px and `ouvrir .scl` moves five elements on the page.

## [1.13.0] - 2026-08-28

The page speaks French, not only the hover help — and the tooltip renderer is
replaced rather than translated. Stage J of the repo-wide i18n task (canon v2).
MINOR: no parameter IDs, ranges or state format changed; existing sessions and
presets load unchanged, and a session saved before this version simply opens in
English.

### Added
- **English + French across every visible string on the page.** 51 label keys
  and 18 tooltip keys in a new `Source/ui/public/js/i18n.js`. Labels, tab
  captions, knob captions, button faces, the MTS status line, the interval
  header, the preset-dropdown group headings and eleven accessible names all
  switch language with no reload and no English survivor.
- **A settings popover with the language selector**, in the exact absolute slot
  the floating "?" occupied through v1.12.1 (`bottom: 50px; right: 15px`), so
  nothing on a packed 600 x 400 layout had to move to make room for it. The
  hover-help switch moves inside it. Every colour, border, radius and transition
  on the gear is the "?" button's own, carried across unchanged.
- **`getUiLanguage` / `setUiLanguage` native functions and session
  persistence.** The choice rides the session XML as a plain `uiLanguage`
  attribute — deliberately NOT an `AudioParameterChoice`, so it never appears in
  a host automation lane and no preset can change which language somebody reads
  their interface in. The JSON preset path is untouched.

### Changed
- **The tooltip renderer is the measure-then-pin one, ported from
  O-ReverseDelay. There is now ONE renderer repo-wide.** v1.12.1's positioner
  never measured: it fell back to `tooltip.offsetWidth || 200` and
  `tooltip.offsetHeight || 40` on every first hover and clamped against
  `.plugin-container` rather than the viewport. The port brings a title/body
  pair, a 120 ms dwell delay, a width RELEASED then MEASURED then PINNED before
  `left` is applied, a vertical flip, a horizontal clamp, and an arrow offset
  recomputed AFTER the clamp so a clamped tip still points at its control.
  The old positioner and its two hard-coded literals are DELETED, not disabled —
  `grep -rn 'tooltipHeight\|tooltipWidth\|data-tooltip' Source/ui/public/`
  returns nothing outside comments.
- **The 1,247-line inline `<script type="module">` is extracted to
  `js/app.js`.** Behaviour moved, not rewritten: the same listeners in the same
  order. Only the import specifiers changed, and only because the module's depth
  did — `./js/juce/index.js` became `./juce/index.js` and `./modules/…` became
  `../modules/…`.
- **`Intervals (12 notes)` is now `Intervals: 12`.** The old form inflected a
  noun on a count, and `total` is `currentIntervals.length`, which a degenerate
  one-line `.scl` makes 1. French pluralises one AND zero as singular where
  English pluralises only one, so the two languages disagree at n = 0. The noun
  is dropped rather than a plural engine built. The English loses the word
  "notes"; that is the visible cost and it is recorded rather than hidden.
- **The three tuning-mode buttons are now three EQUAL columns** (`flex: 1 1 0`
  with `min-width: 0`, in a `width: 100%` row). CUSTOM measures 78.31 px and
  PERSO 67.13 px, and it is the MIDDLE button, so its width moved 12-TET and
  MTS-ESP in opposite directions. A width pin on `#btn-scala` alone did not fix
  it — the row is a flex item floored by its own min-content and the three used
  widths came out 53.27 / 78.31 / 59.25 in English against 58.17 / 67.13 / 65.53
  in French. Side effect: the row now FITS its 200 px panel, where it previously
  overhung it by 10.83 px in English.
- **The four Scala file buttons lose 6 px of horizontal padding each side**
  (`.btn.btn-small` 10px -> 4px), uniformly in both languages. Their row measured
  212.78 px in a 200 px panel in English — already overflowing before this work —
  and 247.91 px in French. It is now 164.78 px and 199.91 px, so both fit and the
  pre-existing English overflow is gone too.

### Fixed
- **The hover-help toggle's persistence call had never worked.** v1.12.1 called
  `window.JuceAPI.getNativeFunction('setTooltipsEnabled')(...)` inside a
  try/catch. `window.JuceAPI` has never existed on this page — the bridge
  namespace is the imported `Juce` module and the global is `window.__JUCE__` —
  and there is no `setTooltipsEnabled` native function in `PluginEditor.cpp` to
  reach. The call threw on every toggle and the catch swallowed it. The dead call
  is REMOVED rather than repaired: adding a real tooltips bridge is a
  processor-state change that does not belong in a commit about language, and
  repairing it in place would have shipped a new persisted preference under cover
  of a rename. The toggle stays session-only, which is v1.12.1's observable
  behaviour unchanged.
- **Eight native `title=` attributes are gone** (contract §4): five authored in
  `index.html` and three injected by the tonic selector. A native title renders a
  second, untranslated OS tooltip competing with the measure-then-pin renderer.
  Each one's text moved to a keyed `data-i18n-aria`; no new prose was invented.

### The counts, parsed rather than grepped
Rendered headless through `scripts/serve-ui.js` and walked with a TreeWalker:

| | |
|---|---|
| `data-tooltip` live anchors | **15** (unique strings: **15**) |
| `index.html` text nodes | 39 |
| text nodes the page injects | 2 |
| text nodes from the two SHARED FX modules | 20 |
| **rendered text nodes** | **61** |
| native `title=` | 8 (5 authored, 3 injected) |
| `aria-label` / `alt` / `placeholder` | 0 / 1 / 12 (all 12 numeric) |
| `[data-i18n]` elements after the retrofit | **36**, all 36 measured by the gate |

The plan's "15 tips" is the first figure in this task to survive being parsed.
Its "~40 static text nodes" did not: 61 render, and the 20-node difference is the
Effects tab, built by two shared registry modules this commit does not edit.

### The tooltip split: 15 clean, 0 hand-split
The plan expects copy authored as `"Label: sentence."` and warns the shape
usually does not hold — on O-IntonationPad it held on 14 of 77. Here it holds on
ALL FIFTEEN. Every string contains exactly one `": "` and in every case it is the
title separator, so the split is mechanical and the bodies are byte-identical to
v1.12.1's attribute values.

### The seven JS-written "mode names" all stay English — `I18N_EXEMPT`
The plan says they are mode names needing `setLabel`. Parsed, they are
`Edge`, `Center`, `Shimmer`, `Focused`, `Warm`, `Bright` and `MTS-ESP (stub)`,
and none is a caption. `STRIKE_POSITION`, `OVERTONE_DAMPING` and `TONE` are all
`AudioParameterFloat` in `createParameterLayout()`, so the plan's
choice-parameter test says "localize" — but the test has a third arm it does not
name: the six words are written into a `.knob-sublabel` whose id ends in
`-value`, the SAME node that shows `Math.round(v * 100) + "%"` everywhere else in
its range. They are the knob's READOUT wearing a word instead of a number at the
ends of its travel, and contract §5 is explicit that a readout is never a
`[data-i18n]` element. Keying one would also make the element enter and leave the
sweep as the knob turns, so a later language change would repaint "Chaud" over
"62%". The seventh is written to `#scale-name`, which also receives
`getActiveTuningName()` and a loaded preset's `scaleName` — a data mirror.
Reasons are recorded per entry in `I18N_EXEMPT`.

`12-TET` and `MTS-ESP` are exempt for the plan's own reason: they are the
`TUNING_MODE` `AudioParameterChoice` option strings VERBATIM. `CUSTOM` is not
(the option is `Scala`), so it is a plain caption and it localizes.

### The two shared Effects-tab modules are deliberately NOT localized
`modules/effects/analog-eq-unit` (1.2.0) and `modules/effects/compressor-unit`
(1.2.1) are registry-tracked and embedded from `${CMAKE_SOURCE_DIR}`. Localizing
them is a cross-plugin change that does not belong in a per-plugin commit, and a
local edit would be silently reverted by `/module-upgrade`. Of their 20 rendered
text nodes the genuinely English words are ANALOG, Thresh, Attack and Release;
the rest are acronyms or spelled identically in French. Recorded in
`I18N_EXEMPT` rather than left silent: a French user sees those four in English
on the Effects tab.

### Verification
- `node scripts/check-i18n.js --strict-v2` — all checks pass, 20 localized plugins.
- `node scripts/check-ui-labels.js --plugin O-Marimba` — ALL CHECKS PASSED across
  four states (default, tuning tab, Custom mode, settings popover), 36 of 36
  `[data-i18n]` elements visible in at least one state. `tests/i18n-states.json`
  is added to drive the three non-default states.
- **Tooltip sweep, both tabs and both languages, 26 rendered tips: none leaves
  the 600 x 400 frame.** The vertical clamp is carried in from O-FreqPulse but is
  **NOT independently reproducible here** — deleting it puts nothing off-frame,
  same as O-Polystutter, O-Lyrica and O-SpectralShaper. The sweep is not blind:
  re-run with the HORIZONTAL clamp deleted instead, it reports ten off-frame tips
  in both languages, worst `#gear-btn` at 72.0 px past the right edge.
- **The language round-trip is MEASURED on both halves, not reasoned.**
  A compiled JUCE probe drives the exact three code paths added here and asserts
  the `uiLanguage` attribute survives the XML round-trip, comes back a STRING
  `var` (`critical_valuetree_xml_roundtrip_loses_type`), is absent on a
  pre-v1.13.0 session so English stands, and that an unknown code degrades to
  English. In the headless page, picking Français in the gear popover sends
  `setUiLanguage('fr')`, and a reloaded page calls only `getUiLanguage`, receives
  `fr` and paints French (SOUND -> SON, MALLET/HARDNESS -> DURETÉ/MAILLET) with
  no English survivor. `auval -v aumu OuMa OuDv` passes, including VERIFYING
  CLASS INFO, which exercises `get/setStateInformation` with the new attribute.

### Not verified
- **No human has seen this French UI in a DAW.** The round-trip is measured in a
  probe and in the headless harness, not by a person picking Français in a host,
  closing the session and reopening it.
- **All 69 French strings are machine drafts, every one `reviewed: false`.**
  No native speaker has read them.
- **Windows / WebView2 font metrics** remain the named hardware-blocked
  deferral. The tightest French margin measured here is 1.69 px, on CUSTOM inside
  its 48 px content box.

### Known pre-existing issue, deliberately NOT fixed here
`CMakeLists.txt:8` declares `PLUGIN_VERSION 1.13.0` inside `juce_add_plugin`.
**`PLUGIN_VERSION` is not a JUCE keyword** — JUCE reads `VERSION` — so the
unrecognised keyword is silently ignored and the plugin ships reporting **1.0.0**
to the host, whatever that line says
(`critical_plugin_version_keyword_ignored_by_juce`). The line already read
`PLUGIN_VERSION 1.12.1` before this work; the bump preserves the file's existing
pattern rather than changing host-visible behaviour inside a language commit.
Repo-wide it affects seven plugins — O-Contrabass, O-Marimba, O-Octagon, O-Reed,
O-ReverseDelay, O-MicrotonalSampler, O-Tapestop — and correcting it moves the
host-visible version from 1.0.0 to the real number, which is a session- and
host-compatibility change and the user's call across all seven at once.

## [1.12.1] - 2026-07-08

Resolves all 13 Critical + Warning findings from the v1.12.0 deep code review
(`CODE_REVIEW.md`). Info-tier findings (IN-01..16) are deferred. PATCH: no parameter
IDs, ranges, or state format changed; existing sessions and presets load unchanged.

### Fixed — Critical
- **CR-01 — Microtonal tuning mistuned across octaves.** The JS scale model sends its
  intervals without the octave period (`applyTuning` sends `currentIntervals.slice(1)`,
  ending at the 11th degree, never 1200¢), and factory/user presets send a leading unison
  but no period. `TuningEngine::setCustomIntervals` prepended a unison but never appended
  the equave, so `scaleDegrees` counted 11 (not 12) and `back()` was the 11th degree — a
  12-key octave was folded onto an 11-degree cycle with a compressed equave. Every
  non-12-TET scale drifted ~one step per octave (e.g. Just-Intonation C#5 played 1292¢
  instead of 1312¢, 20¢ flat); 12-TET survived only by arithmetic accident. **Fix:**
  `setCustomIntervals` now normalizes any input into canonical `[0, …degrees…, 1200]`
  form — dropping duplicate unisons and appending the octave period unless already present.
  Idempotent across the JS live-edit, preset, and state-restore paths. Real `.scl` loads
  (which carry their own period) are unaffected. C#5 in Just Intonation now plays 1312¢.
- **CR-02 — Editor-teardown use-after-free.** All six `FileChooser::launchAsync`
  completions (load/save Scala, load/save KBM, save/load preset) captured a bare `this`
  and dereferenced `processorRef`/`webView`/editor members. Closing the plugin window while
  a native file dialog was open ran the lambda after `~Editor` → UAF/host crash. **Fix:**
  each completion now captures a `juce::Component::SafePointer` and bails with a bare
  `return` when the editor is gone (no `complete()` on the dead path).
- **CR-03 — Preset name with "/" silently lost.** The preset name was used verbatim as the
  `.json` filename; `getChildFile` treats "/" as a path separator, so `"Warm / Bright"`
  was written into an absent subdirectory and never reappeared in the (non-recursive)
  preset list — silent data loss. **Fix:** filenames are sanitized with
  `juce::File::createLegalFileName` in save/load/delete/isFactory/init (display name kept).

### Fixed — Warning
- **WR-01 — Stale FX on preset recall.** `applyPresetJson` only wrote the keys present in
  the preset and never reset omitted params first; factory presets serialize just the 10
  synth params (no `fx_eq_*`/`fx_comp_*`). Selecting a clean preset while EQ + compression
  were engaged left the FX on. **Fix:** reset every APVTS param to its default before
  applying the stored subset.
- **WR-02 — Tuning data race.** `scaleIntervals`/`scaleDegrees` were read on the audio
  thread (via `rebuildFrequencyTable`, triggered every block by `setMode`/`setReferencePitch`)
  while mutated on the message thread with no lock → torn read / OOB. **Fix:** a
  `CriticalSection` now serializes access; message-thread mutators take the full lock, the
  audio thread only ever tries the lock and defers a missed rebuild via a dirty flag
  serviced next block (`serviceRebuild`). `getFrequency` still reads the lock-free atomic table.
- **WR-03 — Nyquist fold-back on high notes.** Modal frequencies (`baseFreq × ratio`, ratios
  up to 54×) had no Nyquist check; upper modes on high notes aliased back into the audible
  band as inharmonic partials. **Fix:** any mode at/above `0.45·fs` is silenced.
- **WR-04 — Click on every note.** The voice hard-terminated at 1.5× decay time (~−13 dB)
  with an abrupt `clearCurrentNote()`, clicking on every note. **Fix:** a 5 ms linear
  tail fade ramps the final samples to zero; total voice lifetime is unchanged.
- **WR-05 — Latent sticky-silence.** `ModalMode::processSample` had a denormal flush but no
  NaN guard (`std::abs(NaN) < 1e-8f` is false), so a non-finite value could latch
  permanently into the biquad state. **Fix:** reset the biquad state on any non-finite output.
- **WR-06 — No master output safety net.** **Fix:** a final pass replaces non-finite
  samples with 0 and clamps to ±2 before the waveform FIFO / VU calc, so a DSP blow-up
  can't corrupt the meters or reach the host. Transparent for normal-level audio.
- **WR-07 — Per-block string-keyed param lookups.** processBlock did 11
  `getRawParameterValue("id")` hashed-string lookups per callback. **Fix:** atomic pointers
  are cached once in `prepareToPlay`. No functional change.
- **WR-08 — Unguarded channel layout.** **Fix:** added `isBusesLayoutSupported` accepting
  only mono/stereo output (auval verifies both).
- **WR-09 — EQ frequency readouts ~3.5× wrong (shared `analog-eq-unit` module).** The
  band tooltips formatted the normalised value linearly, but the freq params are skewed 0.3,
  so a filter at ~77 Hz displayed 265 Hz; double-click reset also landed off the C++ default.
  **Fix (module v1.1.1):** readouts use `SliderState.getScaledValue()` (true Hz/dB) and
  reset targets each band's skew-aware default. Benefits every `analog-eq-unit` dependent on
  their next rebuild.
- **WR-10 — Convolution dry/wet latency.** Verified: the default `juce::dsp::Convolution`
  runs the zero-latency algorithm (`getLatency()` == 0), so no comb exists today. Added a
  defensive `dryWetMixer.setWetLatency(convolution.getLatency())` to stay correct if the
  engine ever reports latency.

### Note
- CMakeLists `PLUGIN_VERSION` and this changelog had drifted to 1.11.0 while the plugin
  shipped 1.12.0 (tag `O-Marimba-v1.12.0`, +6 dB synthesis gain + wider velocity dynamics);
  both are reconciled to 1.12.1 here.

## [1.11.0] - 2026-01-26

### Added
- **Tooltip system** - Click the "?" button (bottom-right) to enable tooltips
  - Hover over any control to see a detailed description of its function
  - Tooltips explain each parameter's musical effect
  - Visual indicator (dashed outline) shows which elements have tooltips
  - Tooltip state can be persisted via JUCE native function

### Tooltips Added For
- **Sound tab**: Mallet Hardness, Material Hardness, Resonance, Strike Position, Overtone Damping, Tone, Velocity, Output, VU Meter, Waveform Display
- **Tuning tab**: Tuning Mode, A4 Reference, Interval List, Pitch Circle

## [1.10.0] - 2026-01-25

### Changed
- **Renamed plugin from "Ouaricon Marimba" to "O-Marimba"**
  - Plugin display name changed to "O-Marimba"
  - Folder renamed from `OuariconMarimba` to `O-Marimba`
  - CMake target renamed from `OuariconMarimba` to `OMarimba`
  - Class names updated: `OMarimbaAudioProcessor`, `OMarimbaAudioProcessorEditor`
  - Preset folder path updated to `~/Library/Application Support/O-Marimba/Presets/`

### Migration Notes
- Existing DAW sessions using "Ouaricon Marimba" will need to re-load the plugin manually
- User presets stored in the old location (`~/Library/Application Support/Ouaricon Marimba/`) can be copied to the new location

## [1.9.9] - 2026-01-14

### Fixed
- **Effects tab layout: Compressor module no longer overlaps EQ module**
  - Moved compressor panel from `top: 95px` to `top: 109px`
  - Now has proper 4px gap between EQ and Compressor modules

## [1.9.8] - 2026-01-14

### Fixed
- **Output gain and VU meter moved to end of processing chain**
  - Root cause: Output gain was applied twice - once per-voice during synthesis, then again after effects
  - Fix: Removed gain application from `MarimbaVoice::renderNextBlock`; now applied only once after EQ/Compressor
  - VU meter now shows true final output level after all processing

### Changed
- **Signal chain clarified and documented**:
  ```
  Synth → Body Resonance → EQ (if enabled) → Compressor (if enabled) → Output Gain → VU Meter
  ```

### Technical Details
- MarimbaVoice.cpp: Removed `* outputGain` from finalSample calculation (line 131)
- PluginProcessor.cpp: Removed `voice->setOutputGain()` call; gain applied once via `buffer.applyGain()`
- No change to parameter behavior - output knob works identically, just applied at correct point in chain

## [1.9.7] - 2026-01-14

### Fixed
- **Compressor attack clicking eliminated** - Added 3ms look-ahead buffer
  - Root cause: Gain was applied to same sample that triggered detection
  - Transient leading edge passed through at full level, then suddenly attenuated
  - Fix: Audio delayed by 3ms; detection runs on current input, gain applied to delayed audio
  - Gain changes now happen BEFORE transients arrive, eliminating discontinuities
- **Autogain now matches OuariconComp standalone** - Theoretical formula replaces slow tracking
  - Root cause: Old autogain used measured GR × 0.6 with extremely slow smoothing (0.0005 coeff)
  - Fix: Now uses `autoGainDB = -threshold × (1 - 1/ratio)` - same as standalone compressor
  - Result: Full loudness compensation, instant response

### Technical Details
- Compressor module updated to v1.3.0 (`modules/effects/compressor-unit/cpp/`)
- Look-ahead implemented via circular delay buffer (stereo, sized for block + lookahead)
- Bypass mode also uses delay buffer to maintain consistent latency
- Autogain formula: at -20dB threshold, 4:1 ratio = 15dB makeup (was ~6dB × slow ramp)

## [1.9.6] - 2026-01-14

### Changed
- **Compressor module updated** - Synced with module system v1.2.3 cleanup
  - Added named constants for magic numbers (MIN_DB, AUTOGAIN_COEFF, etc.)
  - Moved version history from headers to module CHANGELOG
  - Code condensed and formatting improved (no DSP changes)
- **Added PLUGIN_VERSION** to CMakeLists.txt for proper version tracking

### Technical Details
- CompressorUnit.h: Named constants, cleaner code structure
- compressor-unit.js: Constants for UI values, CSS compacted, template-generated meter segments
- No functional changes - identical audio behavior to v1.9.5

## [1.9.5] - 2026-01-14

### Fixed
- **Compressor clicking on enable** - Smooth bypass-to-enabled transition
  - Root cause: `smoothedGainDB` retained stale value during bypass, causing gain jump when re-enabled
  - Fix: Bypass now smoothly ramps gain toward unity (0 dB) each block
  - Envelope also resets during bypass so compression starts fresh
- **GR meter now correctly shows 0 when bypassed** - Verified meter behavior
- **Increased minimum smoothing time** from 5ms to 10ms for better click prevention

### Technical Details
- Compressor module DSP updated to v1.2.3 (`modules/effects/compressor-unit/cpp/`)
- Bypass handling: iterates through buffer samples ramping `smoothedGainDB` toward 0
- When enabled, gain is already near unity - no discontinuity

## [1.9.4] - 2026-01-14

### Fixed
- **Compressor clicking fully resolved** - Complete overhaul of gain smoothing DSP
  - Root cause 1: Smoothing coefficient was sample-rate independent (faster at higher rates)
  - Root cause 2: Smoothing in linear domain caused non-uniform perceptual response
  - Root cause 3: Gain smoother ignored attack/release settings

### Technical Details
- Compressor module DSP updated to v1.2.2 (`modules/effects/compressor-unit/cpp/`)
- **Sample-rate independent smoothing:** Coefficient now calculated in `prepare()` using:
  `gainSmoothCoeff = 1 - exp(-1000 / (5ms × sampleRate))` - consistent ~5ms at any rate
- **dB domain smoothing:** `smoothedGainDB` replaces `smoothedGainLinear` for perceptually
  uniform response across all gain reduction levels
- **Attack/release-aware gain changes:**
  - Gain decreasing (compression engaging) → uses attack coefficient
  - Gain increasing (compression releasing) → uses release coefficient
  - Minimum 5ms smoothing always applied to prevent clicks

## [1.9.3] - 2026-01-14

### Fixed
- **Compressor clicking at high gain reduction** - DSP clicking/popping eliminated
  - Root cause: Gain was applied sample-by-sample without smoothing
  - Fix: Added gain coefficient smoothing (`GAIN_SMOOTH_COEFF = 0.005`)
  - Smoothed gain interpolates toward target to prevent abrupt discontinuities

### Technical Details
- Compressor module DSP updated to v1.2.1 (`modules/effects/compressor-unit/cpp/`)
- New member: `smoothedGainLinear` - tracks smoothed gain coefficient
- Smoothing preserves transient response while eliminating audible artifacts

## [1.9.2] - 2026-01-14

### Changed
- **Effects tab module dimensions** - Both EQ and Compressor modules resized
  - 10px shorter vertically (padding reduced from 8px to 3px)
  - 100px wider (horizontal padding increased from 10px to 60px)
- **Compressor knobs updated** to canonical 10-segment Ouaricon seed cross-section
  - Pattern now matches EQ inner dial and official Ouaricon naturalist aesthetic
  - 36° segments with 1° brown dividers (was 17° segments with 20 divisions)

### Technical Details
- EQ module updated to v1.2.0 (`modules/effects/analog-eq-unit/`)
- Compressor module updated to v1.2.0 (`modules/effects/compressor-unit/`)
- CSS padding changed: `.eq-unit-compact` and `.comp-unit-compact` now use `3px 60px`

## [1.9.1] - 2026-01-14

### Changed
- **Compressor UI redesigned** - Compact single-row layout matching EQ module
  - **COMP button** (left) - Title/bypass toggle (green when ON)
  - **Centered knobs** - 4 knobs aligned with EQ bands above
  - **AUTO button** (right) - Autogain toggle for automatic makeup gain
  - **GR meter** (far right) - Vertical LED meter
- Layout: `[COMP] | [Thresh] [Ratio] [Attack] [Release] | [AUTO] | [GR]`

### Added
- **Autogain feature** - Automatic makeup gain compensates for compression
  - When enabled, applies makeup gain equal to peak gain reduction
  - Maintains perceived loudness while compressing
- New parameter: `fx_comp_autogain` (boolean, default OFF)

### Technical Details
- Compressor module now uses same dark background as EQ (`#2a2318`)
- UI height reduced from 100px to ~80px (compact row)
- Button styles match EQ module toggles

## [1.9.0] - 2026-01-14

### Added
- **Compressor Module** in Effects tab - Compact dynamics processor below EQ
  - **Threshold** (-60 to 0 dB) - Input level where compression starts
  - **Ratio** (1:1 to 20:1) - Amount of gain reduction applied
  - **Attack** (0.1-100 ms) - How quickly compression engages
  - **Release** (10-1000 ms) - How quickly compression releases
  - **Fixed 6dB soft knee** for musical response
  - **Clickable title** for bypass toggle (ON by default)
  - **Vertical GR LED meter** showing real-time gain reduction
- Naturalist seed-knob aesthetic matching plugin visual design
- Module uses Ouaricon Module System (`modules/effects/compressor-unit/`)

### Technical Details
- Compressor processes audio after EQ: Synth → Body Resonance → EQ → **Compressor** → Output Gain
- 5 new parameters with `fx_comp_` prefix for DAW automation:
  - `fx_comp_enabled` (bool, default ON)
  - `fx_comp_threshold` (float, -60 to 0 dB, default -20)
  - `fx_comp_ratio` (float, 1 to 20, default 2)
  - `fx_comp_attack` (float, 0.1-100 ms, default 10)
  - `fx_comp_release` (float, 10-1000 ms, default 100)
- WebView relays: 4 slider, 1 toggle
- GR meter updates via `compressorGR` event at 30Hz
- UI positioned at top: 130px in Effects tab (below EQ at top: 10px)

### Signal Flow
```
Input → LF Shelf → LMF Bell → HMF Bell → HF Shelf → Saturation → Compressor → Output Gain → Output
        (if on)    (if on)    (if on)    (if on)    (if on)      (if on)
```

## [1.8.1] - 2026-01-14

### Changed
- **Compact EQ module** - Redesigned to half height for better UI space efficiency
  - Single-row layout: [EQ bypass] | [LF] [LMF] [HMF] [HF] | [ANALOG]
  - Removed header section and footer (output knob)
  - Smaller dual-ring knobs (60px → 46px) with proportionally scaled SVG notches

### Added
- **EQ bypass toggle** - "EQ" button on left side serves as:
  - Module title/label
  - Master EQ on/off switch (OFF by default = bypassed)
  - When OFF, bands and analog toggle are dimmed and non-interactive
- New parameter: `fx_eq_enabled` (boolean, default OFF)

### Removed
- Output gain knob from EQ module (DSP parameter still exists for preset compatibility)
- EQ header section with "ANALOG EQ" title

### Technical Details
- Analog toggle moved inline with band knobs (right side)
- EQ module uses `eq-unit-compact` CSS class with `.bypassed` state
- WebView relay added: `eqEnabledRelay` with attachment to `fx_eq_enabled`
- DSP: `eqUnit.process()` only called when `fx_eq_enabled` is true

## [1.8.0] - 2026-01-13

### Added
- **Analog EQ Module** in Effects tab - 4-band analog-style parametric EQ
  - **LF Shelf** (30-500 Hz) - Low frequency boost/cut
  - **LMF Bell** (100-2000 Hz) - Low-mid parametric with WIDE/MED/TIGHT Q selection
  - **HMF Bell** (500-8000 Hz) - High-mid parametric with WIDE/MED/TIGHT Q selection
  - **HF Shelf** (2000-20000 Hz) - High frequency boost/cut
  - **Analog saturation** circuit for subtle warmth (toggleable)
  - **Output gain** trim (±12 dB)
- **Dual-ring knob design** - Outer ring controls frequency, inner dial controls gain
  - Click position determines which parameter is adjusted
  - Double-click to reset to center (0 dB gain, center frequency)
  - Drag vertically to adjust values
  - Tooltip shows both values on hover
- Module uses Ouaricon Module System (first effects module integration)

### Technical Details
- EQ processes audio at end of chain: Synth → Body Resonance → **Analog EQ** → Output Gain
- 16 new parameters with `fx_eq_` prefix for DAW automation
- Uses AnalogEQUnit from `modules/effects/analog-eq-unit/`
- WebView relays: 9 slider, 5 toggle, 2 combobox
- UI positioned in top 1/4 of Effects tab (80px), leaving room for future modules

### Signal Flow
```
Input → LF Shelf → LMF Bell → HMF Bell → HF Shelf → Saturation → Output Gain → Output
        (if on)    (if on)    (if on)    (if on)    (if on)
```

## [1.7.0] - 2026-01-13

### Added
- **New EFFECTS tab** - Third tab in the UI for future effects controls
  - Empty placeholder tab ready for effects implementation
  - Parallax tree background shifts further left when navigating to Effects tab
  - Consistent visual transition behavior matching Sound → Tuning navigation

### Technical Details
- Added `.botanical-overlay.effects-position` CSS class (right: -180px, opacity: 0.12)
- Added `#effects-tab` content container
- Updated tab switching JavaScript to handle three-tab navigation
- Tree parallax progression: Sound (-60px) → Tuning (-120px) → Effects (-180px)

## [1.6.2] - 2026-01-11

### Changed
- **UI refinements:**
  - Velocity knob now same size as other control knobs (small class)
  - Velocity curve display scaled to 75% (105x75px)
  - Tone dial verified aligned with Resonance dial above (left: 200px)
- **Preset system updated:**
  - Added STRIKE_POSITION, OVERTONE_DAMPING, TONE to preset format
  - Updated all 10 factory presets with new parameter values
  - Added 2 new presets: "Pad Marimba" and "Staccato Marimba"
  - Factory presets now regenerate on plugin load to include new parameters

### Factory Presets (v1.6.2)
| Preset | Strike | Damping | Tone | Character |
|--------|--------|---------|------|-----------|
| Default Marimba | 0.5 | 0.5 | 0.75 | Natural, balanced |
| Bright Marimba | 0.15 | 0.3 | 0.95 | Edge strike, shimmery |
| Soft Marimba | 0.5 | 0.6 | 0.5 | Center, warm |
| Pad Marimba | 0.5 | 0.1 | 0.7 | Long sustain, shimmer |
| Staccato Marimba | 0.5 | 0.9 | 0.6 | Tight, focused |

## [1.6.1] - 2026-01-11

### Changed
- **UI refinement:** Made all 6 Sound panel knobs the same size (small) and shifted both rows up to prevent label overlap with lower GUI elements
- **2x more extreme parameter ranges** for all 6 timbre controls:
  - **Mallet Hardness:** Duration 2-25ms (was 5-20ms), filter 800Hz-14kHz (was 2-8kHz)
  - **Bar Material:** Mode boost 0.4x-4.0x (was 1.0x-2.0x) - now can attenuate or strongly boost
  - **Resonance:** Decay 0.15-10s (was 0.5-5s) - staccato to pad-like sustain
  - **Strike Position:** Mode multipliers doubled - much stronger edge/center contrast
  - **Overtone Damping:** Factor 0.02-0.9 (was 0.1-0.5) - from shimmering to very tight
  - **Tone:** Cutoff 400Hz-20kHz (was 2kHz-20kHz) - much darker low end possible

## [1.6.0] - 2026-01-11

### Added
- **Three new timbre refinement controls** in Sound panel for deeper sound shaping:
  - **Strike Position** (0-100%, default 50%) - Simulates mallet strike location on the bar
    - Center strikes emphasize fundamental and double-octave (modes 0 & 1)
    - Edge strikes bring out higher partials (modes 2-7)
    - Based on physical nodal point modeling
  - **Overtone Damping** (0-100%, default 50%) - Controls upper harmonic decay rate
    - Low (Shimmer): All modes sustain similarly for bell-like overtones
    - High (Focused): Upper partials decay quickly for tight, woody tone
    - Adjusts damping factor from 0.1 to 0.5 per mode index
  - **Tone** (0-100%, default 75%) - Post-synthesis brightness control
    - One-pole lowpass filter on final output (2kHz–20kHz cutoff)
    - Shapes sustained sound without affecting attack character
    - "Warm" label at low values, "Bright" at high values

### Technical Notes
- New parameters: STRIKE_POSITION, OVERTONE_DAMPING, TONE (all float 0-1)
- Strike position uses mode-specific amplitude multipliers based on nodal patterns
- Overtone damping modifies getDecayTime() modeFactor (0.1–0.5 range)
- Tone filter: toneFilterCoeff = ω/(ω+1) where ω = 2π·fc/fs
- UI: 3 new small knobs added below existing Sound panel row
- All parameters integrated with APVTS, preset system, and DAW automation

## [1.5.0] - 2026-01-11

### Changed
- **Improved physical model realism** - Major overhaul of modal synthesis for more authentic marimba sound
  - **Corrected modal frequency ratios** from acoustic research measurements (Euphonics/ISMA2019)
    - Mode 2 now tuned to exactly 4.0x fundamental (double octave) - the signature of professional marimbas
    - Higher modes corrected: was [26.3, 38.2, 52.4, 68.9], now [24.22, 33.54, 42.97, 54.0]
    - Root cause: Original ratios were approximations; higher modes were 8-22% off measured values
  - **Improved mode amplitude distribution** based on spectral analysis
    - Strong fundamental + strong mode 2 (double octave) for characteristic marimba timbre
    - Faster exponential rolloff for higher modes (more natural overtone balance)
    - Material parameter now only affects modes 3+ (preserves marimba character at all settings)
  - **Enhanced body resonance** with wood-like characteristics
    - Extended IR from 75ms to 100ms for richer sustain
    - 6 resonant modes (was 3) across 180-1100 Hz range
    - Multi-stage decay envelope: quick attack, fast initial decay, slow tail
    - Individual decay rates per mode (higher frequencies decay faster)
    - Subtle early reflections for wood diffusion character

### Technical Notes
- Research sources: Euphonics.org marimba acoustics, ISMA2019 modal measurement studies
- Modal ratios now match measured professional marimba bar spectra
- Body IR now simulates resonator tube coupling more accurately

## [1.4.0] - 2026-01-11

### Added
- **Export tuning files** - Save current tuning as Scala (.scl) and keyboard mapping (.kbm) files
  - SAVE .SCL and SAVE .KBM buttons in Custom tuning mode
  - Remembers last-used export directory for convenience
  - Standard Scala format compatible with other microtonal software

### Changed
- Renamed "SCALA" button to "CUSTOM" for clarity (Custom mode allows editing intervals)
- Interval table is now **non-editable in 12-TET mode**
  - Root cause: Table was editable but changes had no effect (12-TET ignores custom intervals)
  - All inputs disabled when 12-TET is selected, editable only in Custom mode
  - Prevents user confusion about why edits don't affect tuning

## [1.3.1] - 2026-01-10

### Added
- **LOAD button** - Opens file dialog to load preset files directly
- **Preset dropdown menu** - Click preset name to show dropdown with all presets
  - Separate sections for Factory and User presets
  - Currently active preset highlighted
  - Click to instantly load any preset

### Changed
- Preset name display now shows dropdown indicator (▼)
- Improved preset browser UX with direct selection

## [1.3.0] - 2026-01-10

### Added
- **Preset System** - Save and load complete patch states including tuning
  - Factory presets: Default Marimba, Bright Marimba, Soft Marimba, Just Intonation, Pythagorean, Quarter-Comma Meantone, Baroque A=415, Concert A=442
  - User presets saved to `~/Library/Application Support/Ouaricon Marimba/Presets/User/`
  - JSON format for easy editing and sharing
  - Preset browser in header: ◀ ▶ navigation, SAVE button with file dialog
  - Each preset stores: all 7 APVTS parameters, tuning intervals (any scale size), scale name, tonic note
- **DAW Session State** now includes tuning configuration
  - Custom tuning intervals persist when saving/reloading DAW projects
  - Tonic note (transposition) is preserved
  - Scale name is restored
- New C++ PresetManager class for save/load/list operations
- Native functions for WebView: savePreset, loadPreset, getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset, deletePreset

### Changed
- getStateInformation/setStateInformation now serialize complete tuning state (not just APVTS parameters)
- Preset name display in header updates dynamically when navigating or loading presets

## [1.2.6] - 2026-01-10

### Fixed
- Circular scale interval indicators now highlight polyphonically
  - Root cause: Only one note was tracked at a time (single atomic variable)
  - Added lock-free MIDI event queue to track all note-on AND note-off events
  - All held notes now highlight red simultaneously when playing chords
  - Velocity-based intensity: harder hits show brighter red (rgb(220,0,0)), softer hits show darker red (rgb(120,40,40))
  - Proper note-off handling ensures highlights clear when keys are released
  - Octave stacking support: multiple notes on same scale degree correctly tracked

## [1.2.5] - 2026-01-10

### Added
- Live VU meter in Sound tab now responds to audio output
  - Peak level measurement after all processing (synth + body resonance + output gain)
  - Ballistic needle motion: fast attack (0.5), slow decay (0.08)
  - Scale: -60dB to +3dB with full semicircle sweep (-90° to +90°)
  - Dynamic needle color: green (quiet) → red (loud) gradient
  - 30 FPS update rate via C++ timer and WebView events

## [1.2.4] - 2026-01-10

### Changed
- Increased dynamic range of velocity response (+6dB at max velocity)
  - Low velocity (1) remains unchanged
  - High velocity (127) now +6dB louder than before
  - Smooth linear scaling in dB between extremes
  - Independent of VEL_CURVE parameter (applied on top of curve shaping)
  - Makes the instrument more dynamically expressive

## [1.2.3] - 2026-01-10

### Fixed
- Waveform display in Sound tab now functions as a live oscilloscope
  - Root cause: Display was static (flat line), no audio data was being sent to WebView
  - Added lock-free WaveformFifo in PluginProcessor to capture audio samples
  - Added getWaveformData native function to provide 128-point downsampled waveform
  - WebView polls at 60fps using requestAnimationFrame for smooth display

### Changed
- Renamed "MALLET" knob label to "MALLET HARDNESS" for clarity
- Renamed "MATERIAL" knob label to "MATERIAL HARDNESS" for clarity
- Adjusted knob positions slightly to accommodate longer labels

## [1.2.2] - 2026-01-10

### Fixed
- Tonic now correctly sets the root note for interval calculations (not transposition)
  - In 12-TET: Tonic has no audible effect (all semitones equal)
  - In Just Intonation/Scala: Intervals are calculated FROM the tonic note
  - When tonic = D, D is the 1/1 reference; other notes tuned relative to D
  - C still plays as C, but tuned as an interval from D
- Frequency table now rebuilds when tonic changes

## [1.2.1] - 2026-01-10

### Fixed
- Tonic selector now has bi-directional navigation
  - Left arrow (◀) moves down: C → B → A# → ... → C# → C
  - Right arrow (▶) moves up: C → C# → D → ... → B → C
- Keyboard always shows C-D-E-F-G-A-B (physical layout, not relabeled)

## [1.2.0] - 2026-01-10

### Added
- Tonic note selection in Tuning tab
  - Click the "Tonic" selector to cycle through C, C#, D, ... B
  - Updates interval list labels, pitch circle, and keyboard key labels
  - Only available for 12-tone scales

### Fixed
- Scale interval indicator flash bug: UI keyboard no longer causes permanent red lines
  - Root cause: Race condition when both UI and C++ timer called flashIntervalLine()
  - The second call captured already-red color as "original", restoring to red after timeout
  - Solution: Removed direct flashIntervalLine() call from UI keyboard handler
  - C++ timer now handles all note visualization uniformly (external MIDI + UI keyboard)

## [1.1.0] - 2026-01-10

### Added
- Circular scale indicator flashes red when ANY note is played (GUI keyboard, external MIDI, or DAW)
  - C++ Timer polls processor for note-on events and calls WebView via evaluateJavascript
  - Function exported to window scope for cross-thread communication
- A4 reference pitch dial resets to 440 Hz on double-click

### Fixed
- Keyboard animation bug: adjacent black key no longer depresses when white key is clicked
  - Root cause: Black keys were children of white keys, inheriting parent transform
  - Solution: Restructured HTML so black keys are siblings, positioned absolutely
- Black key click detection: right half of black keys now responds correctly
  - Root cause: Black keys extended outside parent container, clicks hit adjacent white key
  - Solution: Black keys as siblings with explicit left positioning (not right: -11px on parent)

## [1.0.0] - 2026-01-09

### Added
- Initial release
- Physically modeled marimba synthesis with bar/mallet interaction
- Microtonality support: 12-TET, Scala file loading, MTS-ESP stub
- WebView UI with botanical paper aesthetic
- Parameters: Mallet Hardness, Bar Material, Resonance, Velocity Curve, Output Gain
- Tuning parameters: Mode selection, A4 reference pitch (400-480 Hz)
- Body resonance via convolution IR
- Playable on-screen keyboard with MIDI output
