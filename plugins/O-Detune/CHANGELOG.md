# O-Detune Changelog

## [1.8.1] - 2026-09-03

The French rendering of the hover-help surface changes suite-wide (task
260903-ukp; O-Gain 1.3.3 was the tracer). PATCH: French strings and source
comments only — no parameter, range, type or state format changed.

### Changed

- **The French caption is now `Infobulles`** (feminine plural). The superseded
  rendering named the ACTION — help on hover; *infobulle* is the noun French
  DAW and OS interfaces use for the surface itself. The glossary root moved
  with it, ROOT-ONLY: `scripts/i18n-fr-glossary.js` now reads
  `'hover help': ['infobulles']` and
  `'toggle hover help': ['activer ou désactiver les infobulles']`, with the old
  rendering REMOVED rather than kept as an accepted alternate — so a plugin
  drifting back is a red G1 gate, not a silent pass.
- **Every sentence re-agreed from feminine singular to feminine plural**, not
  substituted: `cette …` → `ces infobulles`, `l’…` → `les infobulles`,
  `de l’…` → `des infobulles`, `toute l’…` → `toutes les infobulles`,
  `Une fois désactivée` → `Une fois désactivées`; the distributive `chaque …`
  → `chaque infobulle` is the one place the new term stays singular.
  Bare back-references that carried no occurrence of the old phrase — clauses
  reading *le réglage de l’aide*, *l’état de l’aide*, *son affichage ou non*,
  and the pronouns in *Lorsqu’elle est désactivée … la réactiver* — were
  rewritten too. A regex pass would have left every one of them pointing at an
  antecedent that no longer exists.
- Every changed body was read by the developer at a blocking checkpoint
  *before* it was written, so each ships `reviewed: true` legitimately and the
  repo-wide unreviewed-French TOTAL stays at 0.


## [1.8.0] - 2026-09-03

A switch for the hover help. The tooltip layer this plugin already had could
not be turned off; twenty of the suite's forty-three plugins already carried
that switch and twenty-three did not. This closes one of the twenty-three.

### Added

- **A hover-help switch in the settings popover.** `#tips-toggle`, a second
  `.settings-row` under the language selector, on the newer `#tips-toggle` /
  `label.hoverHelp` convention rather than the older `#help-toggle` spelling.
  It gates the tooltip renderer's own `show()` — a delegated renderer has no
  bindings to unbind — and persists under the localStorage key
  `odet.tipsEnabled`.
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

- **The second row costs nothing.** With the popover forced open it occupies **y 386..448, 176 x 62 px — byte-identical in English and French** — inside a 600 x 480 frame. The switch face grows from **42.00 px to 43.61 px** for *Marche*, which is the min-width floor doing exactly what it is for: the growth goes LEFTWARD into the panel's own slack, the button's right edge does not move, and `check-ui-labels` [7] reports 0 non-label elements displaced.
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


## [1.7.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **26 French entries revised** against the suite glossary and lint: ten
  terminology fixes in a caption or tooltip title, twenty typographic (straight
  apostrophes to `’`, no-break spaces before `%`, `:` and `;`, and between a
  number and its unit), two agreement fixes and four where the French had
  drifted from what the English says. The visible ones: **SAUVER → ENREG**,
  **AMPLEUR → PROFONDEUR**, **RETOUR → RÉINJ.** (*retour* is a monitor send in
  French, not delay feedback), **PRÉ-DLY → PRÉ-DÉL.**, and the blend caption
  **MÉLANGE → FONDU**. The preset button's accessible name is now *Enregistrer
  les réglages actuels*, which is also what makes the shortened ENREG caption a
  substring of its own accessible name (WCAG 2.5.3 label-in-name).
- **One French sentence said the wrong thing.** The Blend tooltip opened
  *"Fond enchaîné"* — a background, not a crossfade; it now reads *"Fondu
  enchaîné"*. Three smaller corrections went with it: the Sync body called the
  host transport *le défilement*, the Width body said *dose* where the control
  scales a level, and the Random body named the modulation amount *amplitude*
  where the knob beside it is captioned *Profondeur*.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

Every French entry is still flagged `reviewed: false`: this was a second
machine reading against a glossary and a lint, not a native speaker's.

## [1.7.0] - 2026-08-30

### Added

- **Hover-help, in English and French, on every control the page has.** Eighteen
  tooltips — sixteen parameters plus the gear and the language selector — each
  with a title, a body saying what the control does and when to reach for it,
  and a closing range in the unit the readout actually shows. Every French body
  is a machine draft flagged `reviewed: false`; no native speaker has read one.
- **A renderer to paint them, because the canon does not.** `applyI18n()` writes
  `data-tip-title` and `data-tip` ATTRIBUTES onto the bound anchors and stops
  there; v1.6.0 had no tooltip surface, no tooltip CSS and no hover handler, so
  eighteen bound bodies with no other change would have shipped eighteen
  invisible strings past three green gates. `index.html` now carries a
  `#tooltip` surface, a `.tooltip` rule in this page's own paper-and-walnut
  vocabulary, and a delegated `setupTooltips()` ported from O-simpleFM: it
  follows the cursor, flips to the other side of it and then clamps on all four
  edges at 8 px, and it is called after `initI18n()` inside the same try/catch.
- **A focus latch, so a mouse click cannot park a tip on screen.** A click on a
  `<button>` focuses it, and an unconditional `focusin` rule re-opens the tip
  that `pointerdown` has just hidden. Here that put the gear's own tip square on
  the language selector the click had opened — **5624 px² of overlap**, measured
  by removing the latch and re-running the gate. `:focus-visible` is
  deliberately not the discriminator: Chromium reports it false for a
  programmatic `.focus()` after a click, so a gate driving focus directly would
  measure "no tip" and record that as correct.
- **`tests/ui_tip_render_check.js` — the only gate in this repo that can see a
  rendered tooltip on this page.** 393 assertions: every binding resolves, every
  anchor shows a tip with non-empty text, the rendered title and body are
  byte-equal to the table in both languages, and the tip rectangle stays inside
  600 × 480 at every anchor. It drives `en → fr → en` across four states, plants
  a 3200-character body and confirms the viewport assertion reports the
  overflow, and asserts both halves of the focus latch separately.

### Changed

- Version 1.6.0 → 1.7.0. No parameter IDs, ranges, types or DSP behaviour
  changed; no label, caption or accessible name changed.

### Notes

- **Two parameters are host-reachable and not page-reachable.** `focus_low`
  (20–500 Hz) and `focus_high` (1–20 kHz) are automatable, are implemented in
  the DSP, are relayed to the WebView and are even given slider states and
  formatters by the page — but they have no element. They therefore get no
  tooltip, because a body with nothing to bind to fails the gate and adding two
  controls to a 600 × 480 frame is a feature change, not a localization.
  Reported, not fixed.
- **The Width tooltip cannot be opened with a pointer while Mono-Safe is on.**
  Mono-Safe defaults on and puts `.disabled` on `.slider-container`, which has
  been `pointer-events: none` since v1.5.4. The keyboard still reaches it —
  focus is unaffected by `pointer-events` — so the tip is half-reachable rather
  than dead. Making a disabled control hoverable is a UX decision, not a
  localization, so nothing was changed.
- **Option words stay English inside a French body where they are visible on the
  page.** `Sine`, `Triangle`, `Random`, `Linear`, `Exp`, `60s`/`70s`/`80s` and
  the voice-count digits are `AudioParameterChoice` options, exempt under D-01
  arm 1, and read English in both languages — so a French body naming "Sinus"
  would point at a word that is not on screen. Where the option string appears
  nowhere on the page (the two toggles render as a switch with no text), the
  range is ordinary prose and is localized.
- French bodies take French convention — decimal comma, a space before `%` —
  while the value readouts keep their point, because D-03 exempts the readout
  node and that has not moved.

## [1.6.0] - 2026-08-28

### Added

- **The whole page speaks French, not just a tooltip.** 22 keyed elements and 8
  keyed accessible names in a new `Source/ui/public/js/i18n.js`, driven by the
  repo's canonical i18n runtime (canon v2, byte-compared against
  `scripts/i18n-canon.js`). Every French string is a machine draft flagged
  `reviewed: false` — no native speaker has read them yet.
- **A settings popover, bottom-left, carrying the language selector.** Styled in
  this plugin's own naturalist vocabulary and positioned absolutely inside
  `#app`, so it contributes nothing to any flow box. It opens upwards; the gear
  is 6 px from the bottom of a 480 px frame.
- **The language choice persists with the session.** `getUiLanguage` /
  `setUiLanguage` native functions and a non-parameter `uiLanguage` property on
  the APVTS state tree. It is deliberately NOT an `AudioParameterChoice`: it
  must not appear in a DAW automation lane, and a preset must not be able to
  change which language somebody reads their plugin in.

### Changed

- **Every native `title=` on the preset bar is deleted.** All five carried the
  only help their element had, so each text moved verbatim to `data-i18n-aria`
  and was then translated. A native title renders a second, untranslated OS
  tooltip; no new hover-help prose was authored — that is a later stage.
- **Five per-element layout pins, so French cannot move anything that is not a
  label.** The preset action buttons are pinned to 60 px, the engine knobs take
  their full grid cell, the Feedback and Mono-Safe captions are pinned to their
  English boxes, and the Width caption and its readout are spread rather than
  left-packed. Each was reverted alone and confirmed to re-break the geometry
  gate. The visible English cost: the preset bar reserves 32 px, and the Width
  readout moves to the right edge of the column it already sat in.

### Fixed

- The page's version label read `v1.5.4` and now reads `v1.6.0`, matching
  `CMakeLists.txt`.

No parameter IDs, ranges, types or DSP behaviour changed.

## [1.5.4] - 2026-08-02

### Fixed

- **Plugin bundle now reports its real version to hosts.** `juce_add_plugin` had no
  `VERSION` keyword, so every prior release shipped reporting 1.0.0 (AU version
  65536) regardless of the actual release number. `VERSION 1.5.4` is now declared
  in CMakeLists.txt.

### Changed

- Synced vendored preset-manager module to v1.0.5 (byte-identical to
  `modules/persistence/preset-manager`).
- Added AGPL-3.0 license notice headers to all Ouaricon-authored sources.

## [1.5.3] - 2026-07-01

Code-review follow-ups (see `.planning/REVIEW.md`). All changes preserve existing
audio behavior except WR-01, which activates a control that previously did nothing.

### Fixed

- **CR-01 (RT-safety): Focus filter no longer allocates on the audio thread.**
  `processBlock` called `IIR::Coefficients::makeHighPass/makeLowPass` — each a heap
  allocation — every block, unconditionally. Coefficients are now recomputed only
  when a Focus cutoff actually changes (cached `lastFocusLow`/`lastFocusHigh`),
  eliminating per-block allocation and wasted CPU when the Focus controls are static.
  Root cause: coefficient factories were called each block regardless of change.
- **WR-01: The "Randomization" (`random_amt`) knob now affects the audio.** It was
  read and target-smoothed but never applied to any DSP. It now scales per-voice
  unison humanization (LFO rate ±0.3 and modulation depth ±0.5 at full), layered on
  the selected distribution. At `random_amt = 0` the factors are 1.0, so existing
  presets/sessions are unchanged.
- **WR-03: Removed non-functional latency reporting.** `getLatencySamples()` was a
  non-virtual method in JUCE 8 (never called by the host) and `setLatencySamples()`
  was never invoked, so the host always saw 0. The ~50 ms wobble/unison delay is a
  wet-path effect (dry is undelayed), so no PDC latency should be reported; the dead
  override and `latencySamples` member were removed. Host behavior is unchanged (still 0).

### Changed

- **WR-02: Removed illusory parameter smoothing.** `smoothedWobbleRate`,
  `smoothedWobbleDepth`, `smoothedUnisonDetune`, `smoothedUnisonSpread`, and
  `smoothedRandomAmt` set targets every block but were never read — the DSP used the
  raw atomic values. They were dead code (no audible effect) and have been removed;
  the four smoothers actually consumed (blend, width, delay, feedback) are retained.
- **WR-04: Simplified `wobble_era`.** The `EraPreset` struct carried `darkness` and
  `drift` fields that were never applied — only the depth multiplier had any effect.
  Replaced with a plain depth-multiplier table and corrected the misleading comments;
  Era behavior is unchanged (per-decade wobble-depth scaling).

### Removed (dead code sweep)

- Unused `wobbleLFO` (`juce::dsp::Oscillator`) that was prepared but never processed
  (IN-01); unused members `feedbackStateL/R`, `randomRefreshCounter`, `noiseLastQuarter`
  and the corresponding `generateLFO` parameter (IN-02).

### Testing

- Rebuilt Release (VST3 + AU), reinstalled with dual-variant cache sweep, `auval` pass.
- Behavior-preservation verified by construction: all removed smoothers were unread,
  and every new humanization factor collapses to 1.0 at `random_amt = 0`.

## [1.5.2] - 2026-02-18

### Added

- **Licensing module integration** (compile-flag gated, off for local dev)
  - Optional OuariconLicense support behind `OUARICON_LICENSING_ENABLED` flag
  - License overlay hides WebView UI until activation when enabled
  - CMake flag `OUARICON_LICENSING=ON` enables licensing for release builds

## [1.5.1] - 2026-02-17

### Added

- **Version number displayed in UI** - Small "v1.5.1" label in bottom-right corner
  - Uses naturalist muted color at 60% opacity, 9px serif font
  - Non-interactive (pointer-events: none)

## [1.5.0] - 2026-02-07

### Added

- **Full preset system** using Ouaricon preset-manager module
  - Preset bar in header with prev/next navigation, save, and load buttons
  - Dropdown preset browser (click preset name to see all presets)
  - 6 factory presets: Default, 70s Tape Wobble, Cassette Lo-Fi, Hybrid Wobble Unison, Supersaw Synth, Thick Vocals
  - Save/load user presets via native file dialogs
  - Factory presets are read-only; user presets stored in ~/Library/O-Detune/Presets/User/
  - DAW session state now includes preset name (restored on project reload)

## [1.4.1] - 2026-02-03

### Fixed

- **Mono-safe toggle no longer causes noise**
  - Root cause: Division in side/mid ratio calculation amplified tiny floating-point noise when signal was near zero
  - Fix: Added noise floor check (-120 dB) to skip processing on silent signals
  - Ratio calculation now only happens when mid signal is above noise floor

## [1.4.0] - 2026-02-03

### Changed

- **UI streamlined - Advanced panel removed**
  - Pre-Delay and Feedback knobs moved to Output section (as smaller knobs)
  - Randomization knob now appears in Unison panel only when Distribution = "Random"
  - Advanced section completely removed for cleaner interface

- **Tempo Sync moved under Wobble Rate knob**
  - Sync toggle now directly below the Rate knob in Wobble panel
  - When Sync enabled, Rate display shows musical divisions (1/4, 1/8, 1/16, etc.) instead of Hz
  - Same tempo sync behavior as O-Tremolo

### Technical Notes

- Window height reduced from 520px to 480px
- Output row now uses 5-column grid layout
- New small knob size class (44px) for Pre-Delay and Feedback
- Musical divisions mapped from normalized rate value (0.0 = 4 bars, 1.0 = 1/32)

## [1.3.9] - 2026-02-03

### Removed

- **Character panel completely removed** (Drive, Color, Age parameters)
  - User-requested simplification of plugin interface
  - Removes 3 APVTS parameters: `drive`, `color`, `age`
  - Removes tube saturation, tone shaping, and tape hiss/drift processing
  - Reduces CPU usage by eliminating character DSP processing chain

### Technical Notes

- Removed from PluginProcessor.cpp: Character parameter definitions, `processDrive()` helper, color filter processing, age hiss/envelope follower, filter drift modulation
- Removed from PluginProcessor.h: `smoothedDrive`, `smoothedColor`, `smoothedAge`, `colorFilterL/R`, `filterDriftPhase`, `envelopeL/R`, `processDrive()` declaration
- Removed from PluginEditor: `driveRelay`, `colorRelay`, `ageRelay` and their attachments
- Removed from UI: Character panel HTML, CSS styles, JavaScript knob configs and formatters
- Parameter count reduced from 21 to 18

## [1.3.8] - 2026-02-03

### Fixed

- **Age hiss is now dynamic (envelope-following)** - No more constant hiss
  - Root cause: Hiss noise was generated at constant level based only on Age parameter value
  - The noise did not respond to input signal level, creating constant background hiss
  - Fix: Added envelope follower that tracks input signal amplitude
  - Hiss is now proportional to signal level (silent when no audio passes through)
  - Fast attack (~1ms) catches transients, slower release (~50ms) provides natural decay

### Technical Notes

- Added `envelopeL` and `envelopeR` state variables for per-channel envelope tracking
- Envelope follower uses asymmetric attack/release coefficients
- Envelope scaled by 3x for sensitivity, clamped to 1.0 max
- Hiss level = `ageMix * 0.05 * envScale` (was just `ageMix * 0.05`)

## [1.3.7] - 2026-02-03

### Fixed

- **Character panel (Drive, Color, Age) now has audible effect** (critical bug fix)
  - Root cause: Smoothed values for Color and Age were never advanced with `getNextValue()`
  - The parameter smoothing system requires calling `getNextValue()` per-sample to progress the ramp
  - Color and Age used `getCurrentValue()` for threshold checks but never advanced the smoothing state
  - Result: Parameters were stuck at their initial values and never responded to user adjustment
  - Fix: Restructured Color and Age processing to advance smoothed values per-sample
  - Both parameters now properly respond to knob/automation changes in real-time

### Technical Notes

- `SmoothedValue<float>` requires `getNextValue()` to be called to progress the interpolation
- `getCurrentValue()` only reads current state without advancing
- Color filter now updates coefficients per-sample (was per-block) for smoother automation
- Age hiss level now calculated per-sample for proper parameter tracking

## [1.3.6] - 2026-02-03

### Fixed

- **Restored wobble engine functionality** (was accidentally not working in v1.3.5)
  - Restored from v1.3.0 backup and carefully reapplied only unison changes
  - Wobble engine code is now identical to the working v1.3.0 version
  - Unison engine updated with new chorus algorithm (from v1.3.4/v1.3.5)

### Technical Notes

- Wobble engine unchanged from v1.3.0
- Unison engine uses per-voice sine LFO (from v1.3.4)
- Gain compensation: 1/N with always-on tanh() saturation (from v1.3.5)
- voiceRandomOffsets initialized in prepareToPlay for Random distribution

## [1.3.5] - 2026-02-03

### Fixed

- **Eliminated clipping on loud signals** (especially with Random distribution)
  - Root cause: `1/sqrt(N)` gain compensation wasn't enough when LFO phases aligned
  - Fix: Changed to `1/N` gain compensation (more conservative)
  - Added always-on `tanh()` soft saturation (not just above threshold)
  - This provides gentle compression that increases naturally with level

### Technical Notes

- Previous: `gainCompensation = 1/sqrt(N)`, conditional clipper above 0.9
- Now: `gainCompensation = 1/N`, always-on `tanh(s)` saturation
- `tanh(x)` is transparent at low levels, compresses smoothly at high levels

## [1.3.4] - 2026-02-03

### Changed

- **Complete rewrite of unison engine using classic chorus algorithm**
  - Previous approach (drift modulation with smoothing) still had click artifacts
  - New approach: Per-voice sine LFO modulating delay time (proven click-free design)
  - This is how classic chorus pedals work (Boss CE-1, Roland Dimension D, etc.)

### How It Works Now

- Each voice has a sine LFO at a slightly different rate
- LFO continuously modulates delay time (sine is always smooth)
- Detune parameter controls modulation depth (50 cents ≈ ±3ms sweep)
- Distribution affects LFO rate spread between voices:
  - Linear: Even rate spread
  - Exponential: Outer voices have more rate difference
  - Random: Uses stored random values to vary rates (no updates during playback)

### Technical Notes

- Base LFO rate: 0.5 Hz (2-second cycle)
- Max modulation depth: ±3ms around 50ms center
- Sine wave is continuous everywhere (no discontinuities at phase wrap)
- Random offsets now only affect LFO rate, not detune values directly

## [1.3.3] - 2026-02-03

### Fixed

- **Eliminated remaining clicks in unison engine** (especially with Random distribution)
  - Root cause: When detune values changed (random offset updates), driftRate could flip sign causing sudden direction reversal
  - Fix: Added dual-layer smoothing system
    1. Per-voice detune smoothing (α=0.0002, ~100ms time constant)
    2. Per-voice delay time smoothing (α=0.001, ~20ms time constant)
  - All parameter changes now interpolate smoothly over time

### Changed

- Reduced drift range from ±10ms to ±5ms for subtler chorusing effect
- Detune direction now based on smoothed detune value (prevents sign-flip clicks)

### Technical Notes

- `smoothedVoiceDetunes[voice]` prevents sudden detune jumps from random offsets
- `smoothedDelayTimes[voice]` provides final safety net for delay discontinuities
- Exponential smoothing: `value += (target - value) * coefficient`

## [1.3.2] - 2026-02-03

### Fixed

- **Eliminated clicks in unison engine** (critical bug fix)
  - Root cause: Sawtooth drift phase wrapped abruptly from 1.0→0.0, causing ~20ms delay jump
  - Fix: Replaced sawtooth with triangle wave modulation
  - Triangle wave reverses direction smoothly at boundaries (no discontinuity)
  - Creates chorus-like oscillation around target pitch (sounds natural)

### Changed

- Reduced drift range from ±20ms to ±10ms for faster cycles and more stable pitch perception
- Delay offset now centered around centerDelay (±driftRange/2) for symmetric modulation

### Technical Notes

- Triangle wave: phase 0→0.5 rising, 0.5→1 falling
- Direction reversal at 0.5 creates smooth pitch oscillation
- Smaller drift range = more frequent direction changes = less perceived pitch drift

## [1.3.1] - 2026-02-03

### Fixed

- **Unison engine now produces audible pitch detuning** (critical bug fix)
  - Root cause: Static delay times don't create pitch shift - delay-based pitch shifting requires continuously modulating delay time
  - Fix: Implemented per-voice sawtooth drift phases that continuously modulate delay time
  - Each voice now has a drifting delay offset that creates actual pitch variation
  - Drift phases staggered at init for richer chorusing effect
  - Proper pitch ratio calculation: 2^(cents/1200) determines drift rate

- **Fixed glitch distortion when Spread + Random distribution used with loud signals**
  - Root cause #1: Random distribution double-applied `voiceRandomOffsets` (once in distribution calc, again in effectiveDetune)
  - Root cause #2: Voice accumulation had no output limiting, causing clipping on loud signals
  - Root cause #3: Random offsets refreshed too frequently (1024 samples), causing audible jumps
  - Fix: Random offsets only applied once per distribution mode
  - Fix: Added soft clipper after voice accumulation (tanh compression above 0.9)
  - Fix: Increased random refresh interval to 4096 samples with smoothed interpolation

### Changed

- Voice gain compensation changed from `1/N` to `1/sqrt(N)` for better loudness consistency across voice counts
- Random offset interpolation now uses 70/30 blend toward new target (smoother transitions)

### Technical Notes

- Drift range: ±20ms around 50ms center delay
- Drift rate derived from pitch ratio: `driftRate = 1 - pitchRatio`
- Soft clipper threshold: 0.9, uses `tanh(x * 1.5) / 1.5` for gentle saturation

## [1.3.0] - 2026-02-03

### Added

- **All 14 placeholder parameters now functional** - Complete parameter implementation
  - Wobble: Era presets (60s/70s/80s), multi-waveform LFO (Sine/Triangle/Random), tempo sync
  - Unison: Voice count (2/3/4/5/7), distribution modes (Linear/Exp/Random), spread, randomization
  - Character: Drive (tube saturation), Color (tone shaping with age drift), Age (hiss + filter drift)
  - Output: Width (M/S stereo spread), Pre-delay, Feedback, Mono-safe

### Changed

- **Mono-safe now forces width to 0** - Enabling mono-safe automatically sets stereo width to mono
  - Width slider becomes disabled (grayed out) when mono-safe is active
  - Previous width value is preserved and restored when mono-safe is disabled
  - Width automation is ignored while mono-safe is active

### Technical Notes

- SmoothedValue used for all 12 continuous parameters (zipper-free automation)
- Era presets affect wobble depth and age drift intensity
- DSP helper functions: generateLFO, processDrive, processWidth, processMonoSafe
- Real-time safe: all buffers preallocated, no allocations in processBlock

## [1.2.0] - 2026-02-02

### Added

- **Blend-responsive panel opacity** - Wobble and Unison panels now fade based on blend knob position
  - Blend at 0%: Wobble panel fully visible, Unison panel faded (35% opacity)
  - Blend at 100%: Wobble panel faded, Unison panel fully visible
  - Provides clear visual feedback for which engine is active
  - Smooth 200ms CSS transition between states
  - Minimum 35% opacity ensures controls remain usable

### Technical Notes

- Panel opacity updates via blend parameter's valueChangedEvent
- Initial state synced on load from JUCE parameter value

## [1.1.1] - 2026-02-02

### Fixed

- **UI knobs now move smoothly** (visual animation improvement)
  - Root cause: Knob visuals relied solely on JUCE parameter callbacks without frontend animation interpolation. Backend updates fire at audio block rate, causing discrete visual jumps.
  - Fix: Implemented `requestAnimationFrame` loop with exponential smoothing interpolation (factor 0.15) for vine arc SVG updates
  - Removed CSS `transition` property on `.knob-vine` stroke-dashoffset (now handled by JS)
  - Text value displays update immediately (no lag) while visual arcs interpolate smoothly

### Added

- **Mouse wheel support for knobs** - scroll up/down to adjust values with fine control (±2% per scroll tick)

### Technical Notes

- Animation system tracks target vs current normalized values per knob
- Loop runs only when knobs are animating (no idle CPU cost)
- Smoothing factor 0.15 provides balance between responsiveness and smoothness

## [1.1.0] - 2026-02-02

### Changed

- **Complete UI redesign to Ouaricon Naturalist aesthetic**
  - Replaced dark gradient background with aged parchment paper texture
  - Added nudibranch (sea slug) botanical illustration overlay at 32% opacity
  - Converted all knobs from CSS indicator style to SVG vine-arc design (O-Freeze style)
  - Changed typography from system sans-serif to Georgia serif with brown earth tones
  - Restyled panels with subtle translucent backgrounds and brown borders
  - Updated dropdowns with parchment background and custom arrow indicator
  - Converted toggles to botanical green theme matching Ouaricon brand
  - Restyled slider with green thumb matching accent palette

### Visual Elements

- **Color Palette:** Paper (#F5E6D3), Text (#3C2F2F, #8b7355), Accent green (#5a7a6a)
- **Typography:** Georgia/Times New Roman serif, letter-spacing for labels
- **Knob Design:** 52px standard / 64px large SVG circles with animated vine arc fill
- **Botanical Overlay:** Nudibranch illustration positioned right side

### Technical Notes

- Window size changed from 600x400 to 600x520 to accommodate improved layout
- Added paper1.jpg and slug.png to BinaryData resources
- All parameter bindings preserved from v1.0.1 (no DSP changes)

## [1.0.1] - 2026-02-02

### Fixed

- **UI knobs and dropdowns now respond to interaction** (critical bug fix)
  - Root cause #1: ComboBox API used wrong method names (`getChosenItemIndex` → `getChoiceIndex`)
  - Root cause #2: Build system cache issue - HTML changes weren't being picked up by ninja incremental builds
  - Fix: Changed to correct JUCE 8 API `getChoiceIndex()`/`setChoiceIndex()` + clean rebuild required

### Technical Notes

- **JUCE 8 ComboBox API:** Uses `getChoiceIndex()`/`setChoiceIndex()`, NOT `getChosenItemIndex()`/`setChosenItemIndex()`
- **Build cache issue:** When modifying WebView HTML/JS, delete `juce_binarydata_*` directory and run `cmake ..` to force BinaryData regeneration
- Files modified: Source/ui/public/index.html (3 method calls corrected)

## [1.0.0] - 2026-02-01

### Initial Release

**O-Detune** - Colorful lo-fi detuning plugin that combines analog tape wobble with unison thickness in one mono-safe package.

### Features

#### Dual-Engine Architecture
- **Wobble Engine**: Delay-based pitch modulation with tape-style wow/flutter
  - Rate: 0.1-10 Hz (slow wow to fast flutter)
  - Depth: 0-100 cents pitch deviation
  - Era presets: 60s (Ampex), 70s (Teac), 80s (Cassette)
  - LFO shapes: Sine, Triangle, Random
  - Tempo sync support

- **Unison Engine**: Multi-voice pitch shifting for supersaw-style thickness
  - 3-voice detuning (fixed in v1.0, expansion planned)
  - Detune range: 0-50 cents
  - Linear distribution
  - Stereo spread control

- **Blend Control**: Crossfade between engines (0 = Wobble, 1 = Unison)

#### Character Processing
- **Drive**: Subtle warmth to tube-style saturation (tanh waveshaping)
- **Color**: Tone shaping from dark (low-pass) to bright (high-shelf)
- **Age**: Combined degradation (hiss + filter drift)

#### Output Section
- **Width**: Stereo spread (0-200%, mono to extra-wide)
- **Mix**: Wet/dry blend with latency compensation
- **Focus Filter**: Frequency-selective processing (20Hz-20kHz)
- **Mono-Safe**: Guaranteed mono compatibility toggle

#### Advanced Controls
- **Pre-Delay**: 0-50ms spatial depth
- **Feedback**: 0-80% recirculation
- **Randomization**: Per-voice variation amount

### Technical Details
- 21 automatable parameters
- Latency: 50ms (2400 samples @ 48kHz)
- CPU-efficient delay-based architecture
- Real-time safe DSP (no allocations in processBlock)
- WebView UI with colorful lo-fi aesthetic

### Factory Presets
1. **Default** - Balanced starting point
2. **Thick Vocals** - 3-voice unison for vocal thickening
3. **Supersaw Synth** - Wide 5-voice detuning for synths
4. **70s Tape Wobble** - Authentic Teac-style pitch variation
5. **Cassette Lo-Fi** - Degraded 80s tape character
6. **Hybrid Wobble Unison** - Combined wobbling unison voices

### Supported Formats
- VST3 (macOS)
- Audio Units (AU) (macOS)
- Standalone application

### System Requirements
- macOS 11+ (Apple Silicon native)
- DAW supporting VST3 or AU plugins

---

*Developed by Ouaricon Development*
*Taylor Brook*
