# Changelog

All notable changes to O-Polystutter will be documented in this file.

## [1.14.3] - 2026-09-03

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
- **The stale width note in `js/i18n.js`, and the trailing `// 49.61` on the
  live `label.hoverHelp` line, re-measured rather than scaled.** From
  `check-ui-labels --plugin O-Polystutter --verbose`: `Infobulles` renders
  **49.61 px** where the superseded caption rendered 71.77, so the French
  hover-help row sums to `49.61 + 61.38 toggle = 110.99` in the 168 px content
  box. The caption is now 4.58 px NARROWER than English `Hover help` (54.19),
  where it used to be 17.58 px wider. Nothing moves either way — the popover is
  pinned at `min-width: 190px`, and that pin is unchanged.


## [1.14.2] - 2026-08-31

Defects found by reading the French against the code. Stage O of the repo-wide i18n rollout.

### Fixed

- **item 34 — MIDI tooltip:** the body said "Notes C1-B1 trigger lanes 1-4, any
  other note triggers all enabled lanes". Neither claim was true:
  `Source/DSP/TriggerRouter.cpp:76-85` routes notes 60–63 to lanes 1–4, note 67
  to every enabled lane, and ignores everything else — C1–B1 (notes 24–35) did
  nothing. Both languages now read **C3, C#3, D3 and D#3 (notes 60–63) trigger
  lanes 1–4; G3 (note 67) triggers all enabled lanes. Any other note is
  ignored.** Note names follow the convention the plugin's own documents and
  source comments use (middle C = C3 = note 60, JUCE's default) and carry the
  note numbers, so a host that displays C4 for note 60 still agrees with them.
  The routing itself is unchanged — the tooltip now follows the code. Rendered
  on the MIDI toggle inside the 1000 × 690 frame: en 71.17 → 86.56 px tall,
  fr 86.56 → 101.95 px, both still inside the frame (fr top edge 510.05).
  `check-ui-labels` still reports 0 non-label elements moved. The French entry
  is `reviewed: false` again because its meaning changed.
- **`TriggerRouter.cpp:69-74` source comment** labelled notes 61–63 as D3 / E3 /
  F3 — a diatonic run the code never routes (the four lanes sit on four
  CONSECUTIVE semitones, C3 to D#3). The comment now names C#3 / D3 / D#3, states
  the octave convention, and says that every other note is ignored, so the next
  reader fixes the right side.

## [1.14.1] - 2026-08-31

French copy revised. Stage N of the repo-wide i18n rollout.

### Changed

- **15 French entries revised** against the suite glossary
  (`scripts/i18n-fr-glossary.js`) and its lint: 6 terminology, 8 typography,
  1 register. The visible ones are the hover-help switch, which now reads
  **Activé / Désactivé** rather than Oui / Non — those are answers, not states —
  its row label, now **Aide au survol** rather than the bare "Aide" that named
  something different from the switch's own accessible name, and the tape
  section's dry caption, now **DIRECT** rather than SEC, which pairs it with the
  TRAITÉ beside it and with the tip title that already said "Signal direct".
  The Ping-Pong tip title is **Ping-pong** rather than "Va-et-vient", so the
  visible PING caption is once again part of the control's spoken name. In the
  hover-help prose: typographic no-break spaces before `;`, `?` and `%`, between
  a number and its unit, and a true minus sign (−100, −12) in the two ranges
  that had a hyphen. MIDI note names stay English — the host displays C1, not
  do1. ROLLOFF keeps **COUPURE** under a recorded exemption: the knob sweeps a
  lowpass corner from 20 kHz to 2 kHz at a fixed slope, so the glossary's
  "Pente" would name the one thing it cannot move.
- **`<html lang>` now follows the language selector** (canon change, all
  plugins), so assistive technology reads the page in the language it is
  displayed in.

French entries remain `reviewed: false`: this pass is a second machine reading
against a glossary and a lint, not a native speaker's.

## [1.14.0] - 2026-08-28

### Added

- **The PAGE speaks French, not only the hover help.** Every caption, section
  heading, button face, column header and accessible name on the interface
  switches with a language selector in a new gear popover: 105 tooltip anchors
  from 43 keys, plus 48 label keys across 97 keyed elements. Value readouts, the
  six subdivision choices (`1/4`, `1/8T`, …), the step numbers and preset names
  stay English — D-03 keeps the number formatting and the unit symbols
  language-neutral, D-01 keeps a value mirror agreeing with the host's
  automation lane, and D-02 keeps a preset name resolvable, because the name IS
  the JSON filename (`OuariconPresetManager.h:283-285`). 91 French entries (43
  tooltip, 48 label), every one `reviewed: false`: they are machine drafts and no
  native speaker has read them.
- **A settings popover**, in the exact corner the floating "?" occupied — the
  gear is the same 22px circle at the same 961,651 rectangle. It holds the
  language selector and the hover-help switch, so the two things that decide what
  the hover help says and whether it says it at all live in one place.
- **The UI language persists with the session**, riding the APVTS state tree as a
  plain `uiLanguage` property holding the language CODE (`"en"` / `"fr"`),
  written before `getStateAsXml()` copies the tree and read after
  `setStateFromXml()` replaces it. Restored behind an `isVoid()` guard and read
  with `toString()`: `NamedValueSet::setFromXmlAttributes` rebuilds every
  property as a `var` over the attribute STRING, so `isBool()` / `isInt()` type
  predicates are false for every session ever saved. Deliberately NOT an
  `AudioParameterChoice` — it must not appear in a DAW automation lane, and a
  preset must not be able to change which language somebody reads their interface
  in. A session saved before v1.14.0 has no such attribute and opens in English.

### Changed

- **The second tooltip renderer is DELETED, and this plugin is where it was
  written.** v1.13.0's positioner never measured anything: it carried
  `const tooltipHeight = 60; // Approximate max height`, `const tooltipWidth =
  220; // max-width from CSS`, and the two viewport literals `660` and `1000`
  against a frame that is 690 tall. All four are gone, not disabled — a grep for
  `tooltipHeight`, `tooltipWidth` or `data-tooltip` over the served tree now
  returns only comments. In its place is the measure-then-pin runtime already
  shipping in O-ReverseDelay, O-MultiBandCompressor and O-FreqPulse: a
  title/body pair rather than one flat string, a 120 ms dwell so a tip does not
  fire on every crossing, a width RELEASED then MEASURED then PINNED before
  `left` is applied, `position: fixed` so the clamp arithmetic and the box the
  browser lays out are the same rectangle, an arrow offset recomputed AFTER the
  horizontal clamp so a clamped tip still points at its control, and delegated
  listeners on the document rather than on `.plugin-frame`. `.tooltip`'s
  `max-width` stays **220px** — this plugin's own value, parsed from its own CSS,
  never mirrored from another plugin's 230.
- **The controller left the markup.** Through v1.13.0 it was four inline
  `<script>` blocks in `index.html` — the JUCE namespace shim, the preset
  manager, a context-menu suppressor and the 92-line positioner. They are now
  `Source/ui/public/js/app.js`, so the served tree has the same file shape as
  every other localized plugin in the suite. `js/parameter-bindings.js` is
  untouched: it is the APVTS binding layer, not page chrome.
- **The preset dropdown rows are built with `createElement`, not an `innerHTML`
  template.** The template held two localizable strings — the `Factory` badge and
  the delete button's native `title=` — and neither could have been exempted,
  because an exemption lives in `js/i18n.js` where the gate forbids the opening
  angle bracket outright. The delete confirmation is now composed from a
  `{name}` token rather than concatenated, and authored around the inflection: no
  count, so no plural to engineer.
- **Two geometry pins, both measured rather than assumed.** `.knob-container` and
  `.combo-container` gain `width: 42px` — they already measured 42px in English
  (the 42px knob and the `min-width: 42px` readout under them both reach that
  far), so no English geometry moves; the pin stops a longer caption widening the
  column. `.preset-action-btn` gains `width: 64px` in place of horizontal
  padding: `.preset-bar` is `position: absolute; right: 20px` with
  `display: flex`, so a caption that grows widens the bar LEFTWARDS and drags the
  preset name, both nav arrows and the other button with it. **This one does move
  English:** the bar widens from 325.3px to 358px and its five children shift
  32.66px left, once, and stop depending on the language for good.
- **Five native `title=` attributes deleted.** On an element that also carries a
  `data-tip` a native title renders a second, untranslated OS tooltip competing
  with the measure-then-pin renderer. Where a title was an element's only
  accessible name its text moved to `data-i18n-aria`; eight keyed accessible
  names now switch language, including two the plugin never had.

### Testing

- `check-i18n.js --plugin O-Polystutter`: 36 assertions, exit 0, canon v2.
  `check-i18n.js --strict-v2`: exit 0 across 16 canon-v2 plugins, 0 on canon v1.
- `check-ui-labels.js --plugin O-Polystutter`: ALL CHECKS PASSED over four states
  (default, preset dropdown open, settings popover open, hover help on) with
  **ZERO non-label elements moved** between English and French at the fixed
  1000 x 690 frame, 97 of 97 keyed elements measured and no coverage hole,
  vacuity 59/97 labels (61%) plus 8/8 attributes, `dataset.label === textContent`
  after init, after a language switch and after a state pass in both languages,
  no page error and every resource served.
- **Verified by rendering, not by inspection.** All 105 anchors — enumerated from
  the DOM, not transcribed — hovered with a real pointer in both languages, 210
  measurements: every tip visible with a non-empty title AND body, at or under the
  220px cap, FULLY inside the 1000 x 690 frame, and its arrow still horizontally
  inside its anchor. `serve-ui.js` picks port 0, so no concurrent session's files
  can be served instead.
- **English geometry against v1.13.0**, measured by swapping HEAD's five files in
  and back out from an in-memory copy in a `finally` (never `git checkout --`,
  which would have taken the uncommitted work with it): the preset bar and its
  five children move 32.66px left, `#tooltip-toggle` is replaced by `#gear-btn`
  at the identical 961,651 22x22 rectangle, and **nothing else moved at all**.
  Scroll extent 1000 x 690 in both builds and both languages; no page error and
  no 404 in either.
- The **vertical clamp** ported in with the renderer is **not independently
  reproducible here**, and that is recorded rather than dressed up: reverting its
  two lines alone leaves the 210-hover sweep green. It fires only when
  `anchor.top < h + 16` AND `anchor.bottom > 674 - h`, which at this page's
  tallest French tip (102px) needs an anchor over 454px tall; the tallest anchor
  here is `#sequencer-section` at 185px with 369px of clear room above it.
- `build-and-install.sh` clean; `auval -v aumf OuPs OuDv` **AU VALIDATION
  SUCCEEDED**. The build adds no new compiler warning — the 9 that remain are
  pre-existing `AudioParameterFloat` deprecation warnings.

### Not verified

- The **C++ language round-trip** (pick Français, close the session, reopen,
  confirm it held) has NOT been executed by hand. It is reasoned from the
  `isVoid()` guard and the `getStateAsXml()` / `setStateFromXml()` ordering, not
  measured.
- All **91 French strings are machine drafts**, every one `reviewed: false`.
- **Windows / WebView2 font metrics** remain the named hardware-blocked deferral.
  The tightest margin a FRENCH string introduces is **4.2px** — `DÉCLIN` at
  37.8px in the 42px knob column — followed by 4.23px on `CONTOUR` (53.8px in
  the 58px bypass chip) and 5.05px on `Non`. Two boxes are tighter still and
  neither is French: `SUBDIV` leaves 3.69px and `PULSES` leaves 2.77px, both
  unchanged English strings this version inherited.

## [1.13.0] - 2026-08-14

### Added

- **WR-02: Per-lane FILTER knob is now functional** — `laneN_filter` was declared, bound to the UI, saved in state, and automatable since v1.0, but no DSP code ever read it (2026-07-01 code review finding WR-02: "The knob rotates, automates, and does nothing"). Root cause: the parameter was never fetched (`getRawParameterValue`) nor pushed into `RepeatLane`. Implemented as the original spec's per-lane filter sweep on the repeat output: negative values sweep a 2nd-order Butterworth lowpass from 20 kHz down to 200 Hz (darkening), positive values sweep a highpass from 20 Hz up to 8 kHz (brightening), 0 bypasses — exactly what the shipped UI tooltip already promised. Coefficients via `IIR::ArrayCoefficients` assigned in place behind a cached-value guard (RT-safe, same pattern as the v1.12.3 CR-01 tape-rolloff fix); the filter-active flag is derived unconditionally on every parameter push, outside the cache-miss branch, so it cannot go stale when the knob returns to a cached value. The filter runs on every sample while active — including silent gaps between repeats — so IIR state stays continuous across repeat boundaries. Implementing (rather than removing the parameter) preserves saved-session automation; parameter IDs, ranges, and state format are unchanged, so existing sessions and presets load as before — sessions that automated the previously-dead knob will now hear filtering.

### Testing

- Liveness-gated sweep probe (offline harness compiling `RepeatLane.cpp` directly): filter at −100 drops >2 kHz energy by **54.5 dB** vs bypass; +100 drops <1 kHz energy by **45.3 dB** — the parameter is provably wired (a plain no-zipper probe is vacuous on an unwired param, which is how this shipped in v1.0). Stale-flag sequence (−100 → 0 → −100 on one lane instance): −54.5 dB / 0.0 dB / −54.5 dB — bypass and re-engage both track the knob. Full −100→+100 sweep while repeating: all output finite.
- auval PASS (aumf OuPs OuDv); pluginval strictness 10 SUCCESS ×3.

## [1.12.4] - 2026-08-02

### Fixed

- **Windows build failure (MSVC C2440/C2119).** The v1.12.3 release run failed `build-windows`:
  MSVC rejects `SafePointer(this)` init-captures inside nested lambdas. Both
  `FileChooser::launchAsync` callbacks in `PluginEditor.cpp` (`savePresetWithDialog`,
  `loadPresetFromFile`) now hoist the `SafePointer` to a local and capture it by value —
  same fix as O-Lyrica v2.3.3 / O-IntonationPad v2.8.3 / O-Prism v1.19.3. Behaviour is
  unchanged on all platforms.

## [1.12.3] - 2026-07-01

RT-safety and correctness fixes from the 2026-07-01 adversarial code review (`.planning/CODE-REVIEW.md`, findings CR-01, CR-02, WR-01, WR-05, WR-08).

### Fixed

- **CR-01: Heap allocation on the audio thread in tape rolloff filter** — `updateRolloffFilter()` used `IIR::Coefficients::makeLowPass`, which heap-allocates a ref-counted object; during a `tape_rolloff` drag or automation ramp this ran every block on the RT thread (dropout risk under load). Replaced with `IIR::ArrayCoefficients::makeLowPass` assigned in place (stack array, identical math — same pattern as O-Formant v1.25.1). `updateHissBandpass` converted for consistency and its computed-but-discarded highpass removed.
- **CR-02: Use-after-free in preset file dialogs on editor teardown** — both `savePresetWithDialog` and `loadPresetFromFile` FileChooser completions captured raw `this` and called `complete()` unconditionally; closing the plugin window while the OS dialog was open then dereferenced a dangling editor and a dead WebView Impl. Completions now capture `Component::SafePointer` and bail with a bare `return` when the editor is gone (O-MicrotonalSampler v1.23.5 W12 pattern); manual `new`/`delete` of the chooser replaced with `shared_ptr`.
- **WR-01: Euclidean rhythm generator dropped pulses** — the Bjorklund iteration counted only leftover B-group sequences; whenever an iteration ended with more A's than B's, the unpaired A sequences (each carrying a pulse) were silently discarded. E(3,8) tresillo, E(5,8) cinquillo, E(7,16) samba, and E(5,12) all produced wrong patterns — the exact rhythms the "Euclidean Groove", "Afro-Latin Stutter", and "Minimal Pulse" factory presets are built on. Fixed leftover accounting in both the C++ engine (`RepeatLane::generateEuclideanPattern`) and its JS preview mirror (`parameter-bindings.js`); both verified against canonical patterns (tresillo/cinquillo/bossa/samba/venda) plus a full pulse-count sweep of all 240 (pulses, steps) combinations.
- **WR-05: Long repeat tails corrupted by live input** — playback read the circular capture buffer whose write head keeps advancing; once `repeats × subdivision` exceeded the 5 s buffer (e.g. "Ambient Freeze": 16 × 1/4 at 120 BPM = 8 s), later repeats audibly mutated into delayed live input. The captured slice is now copied into a dedicated per-lane snapshot buffer at trigger time (bounded copy of pre-allocated memory, RT-safe) and playback reads only the snapshot. Also fixes the loop-boundary interpolation reading one sample of unrelated live audio (review IN-02).
- **WR-08: Presets inherited stale parameter state** — `applyPresetJson` only set keys present in the preset JSON, so parameters the 12 non-Euclidean factory presets omit (64 pattern steps, pitch-rand, Euclidean settings) kept whatever the previous session left behind. All parameters are now reset to defaults before a preset's values are applied; this also repairs user presets saved by older versions.

### Testing

- Bjorklund fix verified by standalone harness (C++ and JS): canonical patterns E(3,8), E(5,8), E(5,16), E(7,16), E(5,12), E(2,5) exact-match, pulse-count invariant holds for all 240 combinations.

## [1.12.2] - 2026-03-06

### Added

- **Licensing module integration** — activation overlay and license validation gated behind `OUARICON_LICENSING_ENABLED` compile flag (no impact on local dev builds)

## [1.12.1] - 2026-03-04

### Fixed

- **Euclidean sequencer audio clicks eliminated** (3 bug fixes):
  - Re-trigger mid-repeat now crossfades between old and new capture output instead of hard-resetting (primary click source with consecutive ON steps)
  - Euclidean pattern position wraps at `euclideanSteps` instead of always 16 — E(3,8) now correctly cycles every 8 steps, not 16 with an 8-step dead zone
  - Fade-out at repeat end now fades from last output level to silence instead of writing hard zeros

## [1.12.0] - 2026-03-03

### Changed

- **Euclidean controls improved for readability**
  - "PLS" and "STP" abbreviations replaced with full "PULSES" and "STEPS" column headers above the sequencer lanes (table header style)
  - Dropdown boxes enlarged (28x16px → 34x26px) with larger font (7px → 11px) for easier number reading
  - Sequencer rows taller (22px → 28px) with more vertical breathing room
  - EUC toggle buttons enlarged (24x20px → 26x28px)
  - Column headers styled as bold dark text above the grid, no longer repeated per-row
  - Plugin window height increased (660px → 690px) to accommodate taller sequencer section

## [1.11.0] - 2026-03-02

### Changed

- **Euclidean PLS and STP controls replaced with dropdown menus**
  - Rotary knobs replaced with `<select>` dropdown elements for more precise value selection
  - PLS dropdown: integer options 1-16 (number of active hits)
  - STP dropdown: integer options 2-16 (total pattern steps)
  - Added `bindDropdown()` JS function connecting `<select>` to JUCE slider state
  - Styled to match plugin aesthetic (warm/earthy palette, compact inline layout)

## [1.10.1] - 2026-03-01

### Fixed

- **Sequencer step buttons now toggleable on click when EUC mode is OFF**
  - Root cause: `updateEuclideanPatternPreview()` overwrote step button `active` classes with the Euclidean pattern, but turning EUC off only removed `euc-locked` CSS — it did not restore step visuals from JUCE parameter values
  - Clicks appeared to do nothing because visual state was out of sync with parameter state
  - Fix: restore each step button's visual state from its JUCE toggle parameter when EUC is disabled

## [1.10.0] - 2026-03-01

### Removed

- **Freeze feature removed from all 4 lanes**
  - Removed 4x `lane[N]_freeze` APVTS parameters
  - Removed freeze buffer allocation and DSP logic from RepeatLane (saves ~40KB RAM per lane)
  - Removed FRZ toggle buttons from UI and freeze pulse CSS animation
  - Removed MIDI note A3 freeze toggle mapping from TriggerRouter
  - Removed freeze indicator JS bindings and visual feedback
  - Playback now always reads from live capture buffer (no freeze branch)

## [1.9.1] - 2026-02-26

### Fixed

- **Repositioned Euclidean PULSES/STEPS knobs to avoid overlap with sequencer grid**
  - Moved dials from below toggle row (top: 310px) to compact position directly above EUC toggle
  - Created new `.euc-knob` CSS class with 22px knobs (smaller than mini-knob's 32px)
  - Labels shortened to "PLS"/"STP" to fit compact layout
  - Fixed `position: relative` override for knob-container children inside flex parent
  - No longer overlaps with sequencer L1-L4 rows

## [1.9.0] - 2026-02-26

### Added

- **Per-lane Euclidean rhythm generator (Bjorklund's algorithm)**
  - Each lane gets an "EUC" toggle that replaces manual step programming with auto-generated Euclidean patterns
  - 12 new APVTS parameters: per-lane `euclidean_enabled` (bool), `euclidean_pulses` (int, 1-16), `euclidean_steps` (int, 2-16)
  - Canonical Bjorklund's algorithm produces musically significant patterns:
    - E(3,8) = Cuban tresillo
    - E(5,8) = Cuban cinquillo
    - E(5,16) = Bossa nova
    - E(7,12) = West African bell pattern
  - When EUC is active, PULSES and STEPS mini-knobs appear below toggle row
  - Step grid becomes read-only and visually previews the generated Euclidean pattern
  - When EUC is off, manual step programming works as before (no regression)

- **3 new factory presets showcasing Euclidean patterns**
  - "Euclidean Groove" — 4 lanes with E(3,8), E(5,8), E(7,16), E(5,12) at different subdivisions
  - "Afro-Latin Stutter" — Tresillo + cinquillo + bossa nova patterns
  - "Minimal Pulse" — Sparse patterns with high decay and tape degradation

### Technical

- DSP: `RepeatLane` gains `setEuclideanEnabled/Pulses/Steps()` methods and static `generateEuclideanPattern()` using proper Bjorklund's algorithm (no heap allocations, flat 16x16 arrays)
- Processor: 12 new cached `std::atomic<float>*` parameter pointers; processBlock skips manual pattern loading when Euclidean generates the pattern
- Editor: 12 new WebSliderRelay/WebToggleButtonRelay + corresponding ParameterAttachment objects
- UI: EUC toggle added to each lane's Row 4; conditional PULSES/STEPS mini-knobs in Row 5
- JS: Full Bjorklund's algorithm port for real-time pattern preview on step grid; `setupEuclideanMode()` handles show/hide and read-only grid state

## [1.8.0] - 2026-01-26

### Added

- **Tooltip system with toggle button**
  - Question-mark (?) button in top-left corner toggles tooltips on/off
  - When enabled, hover over any control to see helpful descriptions
  - Tooltips cover all 50+ controls:
    - Lane controls: SUBDIV, REPS, DECAY, FILTER, PROB, VOL, PAN, SWING, PITCH
    - Pitch randomization: RND, MIN, MAX, ST toggles
    - Lane toggles: PING, REV, MAN, FRZ
    - Lane headers and progress bars
    - Sequencer section and SEQ toggle
    - Tape degradation: SAT, WOW, FLUTTER, HISS, ROLLOFF, DROPOUT, BYPASS
    - Mix controls: DRY, WET
    - Trigger controls: MIDI, TRIG
    - Preset bar: navigation, save, load
  - Controls get subtle outline highlight when hovered (with tooltips enabled)
  - Tooltips auto-position above or below to stay within plugin bounds
  - CSS-only styling matches the vintage paper aesthetic

### Technical

- HTML: Added `data-tooltip` attributes to all interactive elements
- CSS: New `.tooltip-toggle`, `.tooltip`, and `.tooltips-enabled` classes
- JS: Tooltip system (~80 lines) handles toggle state, positioning, and visibility
- Files modified: `index.html` only (no C++ changes required)

## [1.7.0] - 2026-01-26

### Added

- **Per-lane pitch randomization**
  - New RND toggle per lane enables random pitch variation on each stutter repetition
  - MIN/MAX knobs set the randomization range (-12 to +12 semitones)
  - ST toggle switches between semitone quantization (whole numbers) and cents mode (2 decimal places)
  - Random pitch is additive to the base PITCH knob (base ± random range)
  - New parameters (16 total, 4 per lane):
    - `lane[N]_pitch_rand_enabled` (bool, default: off)
    - `lane[N]_pitch_rand_min` (float, -12 to +12, default: 0)
    - `lane[N]_pitch_rand_max` (float, -12 to +12, default: 0)
    - `lane[N]_pitch_rand_quantize` (bool, default: on)

### Changed

- **Lane control layout reorganized from 3-3-3 to 4-4-1**
  - Row 1: SUBDIV, REPS, DECAY, FILTER (was: SUBDIV, REPS, DECAY)
  - Row 2: PROB, VOL, PAN, SWING (was: PITCH, FILTER, PROB)
  - Row 3: PITCH + pitch randomization controls (was: VOL, PAN, SWING)
  - Row 4: PING, REV, MAN, FRZ (unchanged)
  - Pitch knob now isolated on dedicated row with full randomization section
  - Controls slightly more compact to fit 4-column layout

### Technical

- DSP: Random offset generated per-repeat in `RepeatLane::startNewRepeat()`
- UI: Mini-knob CSS class for smaller MIN/MAX controls
- Files modified: `RepeatLane.h/cpp`, `PluginProcessor.h/cpp`, `PluginEditor.h/cpp`, `index.html`, `parameter-bindings.js`

## [1.6.8] - 2026-01-24

### Changed

- **Plugin renamed from OuariconPolystutter to O-Polystutter**
  - Matches the O- naming convention used by other Ouaricon plugins
  - Folder renamed: `OuariconPolystutter` → `O-Polystutter`
  - Display name: "Ouaricon Polystutter" → "O-Polystutter"
  - CMake target: `OuariconPolystutter` → `OPolystutter`
  - C++ classes: `OuariconPolystutterAudioProcessor` → `OPolystutterAudioProcessor`
  - UI title updated in WebView HTML
  - Preset directory moved to `~/Library/O-Polystutter/Presets/`
  - No functional changes, existing presets will continue to work

## [1.6.7] - 2026-01-21

### Fixed

- **BYPASS button text now properly aligned with "TAPE DEGRADATION" label**
  - Follow-up to v1.6.6: button was still slightly overlapping and misaligned
  - Adjusted `top` from 12px to 7px (moves button up so text aligns with label)
  - Adjusted `left` from 145px to 175px (more horizontal clearance)
  - Button text now sits inline with section header text

## [1.6.6] - 2026-01-21

### Fixed

- **Tape degradation BYPASS button no longer overlaps dials**
  - Root cause: BYPASS button was positioned at `top: 45px`, same level as tape knobs
  - This caused visual overlap with the SAT (saturation) knob
  - Fix: Moved BYPASS button up to `top: 12px` to align with "TAPE DEGRADATION" section label
  - Adjusted `left` from 130px to 145px to sit properly after the section text
  - Button now appears inline with section header: `TAPE DEGRADATION [BYPASS]`
  - No functional changes, purely visual layout fix

## [1.6.5] - 2026-01-20

### Added

- **Tape degradation bypass toggle**
  - New BYPASS button positioned left of tape knobs (next to section label)
  - When enabled, skips all tape degradation processing (saturation, wow, flutter, hiss, rolloff, dropout)
  - Tape knobs grey out when bypass is active (visual feedback)
  - Allows A/B comparison between clean and tape-processed signal
  - Parameter ID: `tape_bypass` (boolean, default: false/off)
  - Files modified: `PluginProcessor.cpp`, `PluginProcessor.h`, `PluginEditor.cpp`, `PluginEditor.h`, `index.html`, `parameter-bindings.js`

## [1.6.4] - 2026-01-19

### Fixed

- **Hiss level scaled down 10x to usable range**
  - Previous range: -60dB to -20dB (40dB range) - far too loud
  - New range: -60dB to -56dB (4dB range) - subtle tape hiss
  - Root cause: Original scaling made 10% hiss louder than desired 100%
  - File: `Source/DSP/TapeDegrader.cpp` line 165

## [1.6.3] - 2026-01-19

### Changed

- **DRY/WET knobs and MIDI/TRIG buttons moved to tape degradation row**
  - Controls relocated from footer section to right side of tape row
  - Unified bottom row: tape knobs (left) + mix/trigger controls (right)
  - Footer section removed entirely
  - Plugin window height reduced from 830px to 660px (more compact layout)

## [1.6.2] - 2026-01-19

### Changed

- **Tape degradation controls compacted to left half of UI**
  - Changed `.tape-knobs-row` from `justify-content: space-around` to `flex-start` (left-aligned)
  - Reduced gap between knobs from 15px to 8px for tighter spacing
  - Set explicit width of 500px (left half of 1000px plugin frame)
  - Changed knob containers from `flex: 1; max-width: 100px` to `flex: 0 0 auto; width: 75px`
  - "TAPE DEGRADATION" label now left-aligned (was centered)
  - All 6 knobs (SAT, WOW, FLUTTER, HISS, ROLLOFF, DROPOUT) fit comfortably in left half

## [1.6.1] - 2026-01-19

### Fixed

- **Title moved left to avoid preset bar overlap**
  - Changed title positioning from centered (`left: 50%`) to left-aligned (`left: 20px`)
  - Removed `translateX(-50%)` transform that caused horizontal centering
  - Title now sits at left edge, preset bar remains at right edge with no overlap

## [1.6.0] - 2026-01-19

### Added

- **Full preset system with save/load/navigation**
  - Preset bar in header area (top-right) with prev/next navigation
  - Dropdown menu showing all presets with factory/user badges
  - Save button opens native file dialog in User presets folder
  - Load button opens native file dialog to import presets
  - Delete button for user presets (factory presets protected)
  - Presets save ALL state: 128 parameters + sequencer patterns + lane enables
  - Presets stored as JSON in `~/Library/Ouaricon Polystutter/Presets/`

- **12 factory presets showcasing different stutter styles**
  1. Classic Stutter - Basic rhythmic 1/16 note stutter
  2. Glitch Machine - Fast chaotic stutters with pitch modulation
  3. Tape Echo - Slow repeats with warm tape character
  4. Rhythmic Bounce - Synced bounce effect
  5. Ambient Freeze - Long sustaining frozen moments
  6. Dub Delay - Classic reggae-style dub stutters
  7. Digital Chaos - Multi-lane glitchy madness
  8. Lo-Fi Dreams - Heavy tape degradation
  9. Rising Pitch - Pitch increases across lanes
  10. Polyrhythmic - Different subdivisions per lane
  11. Subtle Texture - Low probability, light effect
  12. Maximum Destruction - Everything cranked to max

### Technical

- Integrated OuariconPresetManager module from modules/persistence/preset-manager/
- Added native functions for WebView ↔ C++ preset communication:
  - savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile
  - getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset
  - deletePreset, isFactoryPreset
- Preset state integrated with DAW session save/restore (getStateInformation/setStateInformation)
- Factory presets initialized on first plugin load

## [1.5.6] - 2026-01-19

### Changed

- **SEQ button repositioned** to left of sequencer section (always visible and clickable)
- **Sequencer section shifted right** 70px to accommodate SEQ button placement
- **L1 label restored** in sequencer grid

## [1.5.5] - 2026-01-19

### Changed

- **Sequencer section moved up 100px** for tighter layout
- **SEQ button relocated** from footer to above sequencer
- Tape and footer sections adjusted accordingly

## [1.5.4] - 2026-01-19

### Changed

- **Bug images moved up 20px** for better alignment with lane headers

## [1.5.3] - 2026-01-19

### Changed

- **Bug images repositioned next to lane headers (one per lane)**
  - Each lane now has its own bug image positioned to the right of the lane button
  - Reduced size to 70% (56x70px) for better fit in header area
  - **State-aware opacity**: 100% when lane is active, 50% when lane is disabled
  - Smooth 300ms opacity transition when toggling lanes
  - Bug inherits lane state - dims along with other controls when lane is off

## [1.5.2] - 2026-01-18

### Changed

- **Bug images repositioned between lane sections** (superseded by v1.5.3)
  - Replaced 12 scattered fragments with 3 complete bug/spider images
  - Each bug now positioned as a visual divider between lanes:
    - Bug 1: Between Lane 1 and Lane 2 (x: 218px)
    - Bug 2: Between Lane 2 and Lane 3 (x: 468px)
    - Bug 3: Between Lane 3 and Lane 4 (x: 718px)
  - Vertically positioned at row 3 level (VOL/PAN/SWING controls, y: 280px)
  - Slightly increased opacity (0.32 vs 0.25) for better visibility
  - Cleaner visual separation between lane sections

## [1.5.1] - 2026-01-18

### Fixed

- **Lane progress bars now actually animate during repeat playback**
  - Root cause: CSS selector mismatch in parameter-bindings.js
  - HTML has `id="lane1_progress"` on the `.progress-fill` element itself
  - JavaScript was looking for `#lane1_progress .progress-fill` (nested), which returned null
  - Fix: Use `getElementById("lane1_progress")` directly for fill element
  - Use `.parentElement` to get the container for active/inactive class toggling
  - Progress bars now correctly show 0-100% animation during each repeat cycle

## [1.5.0] - 2026-01-18

### Added

- **Lane progress bars now animate during repeat playback**
  - Each lane's progress bar shows real-time playback position within the captured audio segment
  - Smooth ~30Hz updates from audio thread to WebView UI via custom event system
  - Progress is pitch-ratio aware (higher pitch = faster playback = shorter effective length)
  - Visual states:
    - **Active**: Green fill animates 0-100% as repeat plays
    - **Inactive**: Bar dimmed (30% opacity) when lane is not currently repeating
  - Implementation:
    - `RepeatLane::getProgress()` calculates 0.0-1.0 position accounting for pitch ratio
    - `PluginProcessor` stores progress/active state in atomic variables (thread-safe)
    - `PluginEditor` uses juce::Timer at 30Hz to emit `laneProgress` events to WebView
    - `parameter-bindings.js` listens for events and updates DOM progress bar widths

## [1.4.1] - 2026-01-18

### Fixed

- **Double-click text input now works on all knob types**
  - Root cause: Formatter for REPS, FILTER, and PAN returns a number, not a string
  - Calling `.match()` on a number fails silently, preventing text input from appearing
  - Fix: Convert formatter output to string before regex matching
  - Now works correctly on: REPS (1-16), PITCH (-12 to +12st), FILTER (-100 to +100), PAN (-100 to +100)

## [1.4.0] - 2026-01-18

### Added

- **Double-click text input for all knobs**
  - Double-click on any knob dial or its value readout to enter text input mode
  - Type raw numbers (e.g., "50" for 50%, "-12" for -12 semitones)
  - Suffix is added automatically based on parameter type (%, st, etc.)
  - Enter key or click-away confirms the value
  - Escape key cancels and reverts to original value
  - Values are clamped to valid parameter range
  - Works on all 40 knobs: lane controls (REPS, DECAY, PITCH, FILTR, PROB, VOL, PAN, SWING),
    tape section (SAT, WOW, FLUTTER, HISS, ROLLOFF, DROPOUT), and footer (DRY, WET)
  - Excludes SUBDIV which is a combo-box/dropdown, not a dial
  - CSS styled to match the vintage paper aesthetic

## [1.3.2] - 2026-01-18

### Fixed

- **AU not working in Logic Pro (blank UI, no audio processing)**
  - Root cause: Missing `VERSION` property in CMakeLists.txt caused AU to report Component Version 1.0.0
  - When v1.3.0 removed the sidechain bus, Logic's cached configuration (from v1.0.0) still expected sidechain
  - Logic tried to set up a bus configuration that no longer existed, causing initialization failure
  - Fix: Added `VERSION 1.3.2` to `juce_add_plugin()` in CMakeLists.txt
  - The version bump forces Logic to invalidate its AU cache and re-query the current bus configuration
  - AU now correctly reports stereo-only I/O without sidechain

## [1.3.1] - 2026-01-17

### Fixed

- **Critical: Plugin broken after v1.3.0 parameter removal**
  - Symptoms: Blank UI (only title visible), no audio processing
  - Root cause: PluginEditor.cpp/h still had relays and attachments for removed `envelope_enabled` and `sidechain_enabled` parameters
  - When the Editor constructor called `apvts.getParameter("envelope_enabled")`, it returned nullptr
  - This crashed the WebToggleButtonParameterAttachment initialization, preventing WebView from loading
  - Fix: Removed orphaned relay declarations, WebView option registrations, and parameter attachments from Editor files
  - Files modified: PluginEditor.h (4 lines removed), PluginEditor.cpp (8 lines removed)

## [1.3.0] - 2026-01-17

### Removed

- **ENV (Envelope Trigger) mode** - Removed due to unresolved audio artifacts
  - The envelope follower triggering caused graininess and clicks that couldn't be reliably fixed
  - Users should use beat-sync, manual, or MIDI trigger modes instead
  - Parameter `envelope_enabled` removed from APVTS

- **SC (Sidechain Trigger) mode** - Removed due to unresolved audio artifacts
  - Sidechain input triggering had similar issues to ENV mode
  - Sidechain input bus removed from plugin (simplified to stereo I/O only)
  - Parameter `sidechain_enabled` removed from APVTS

### Changed

- **TriggerRouter simplified to MIDI-only**
  - Removed envelope follower DSP components
  - Removed sidechain envelope follower DSP components
  - Cleaner, lighter codebase focused on working trigger modes

- **UI footer updated**
  - ENV and SC toggle buttons removed
  - Remaining triggers: SEQ, MIDI, TRIG (manual)

- **Total parameters reduced from 130 to 128**
  - Removed 2 boolean parameters (envelope_enabled, sidechain_enabled)

### Migration Notes

- Existing presets using ENV or SC modes will load with those parameters ignored
- The plugin is now simpler and more reliable for production use
- For transient-following behavior, consider using MIDI triggering from a gate plugin

## [1.2.2] - 2026-01-17

### Fixed

- **Sticky "reps" knobs on all 4 lanes now respond smoothly**
  - Root cause: Integer parameters (1-16 range) use discrete steps, but knob sensitivity was too low
  - Small mouse movements caused JUCE to snap back to nearest integer, creating "sticky" feel
  - User had to move in opposite direction first to break out of snap zone
  - Fix: Added delta accumulation for integer knobs in `parameter-bindings.js`
    - Detects integer parameters automatically (whole number min/max, range ≤20)
    - Accumulates mouse delta until it crosses a step boundary (1/15 normalized for 1-16 range)
    - Only sends value to JUCE when step change occurs, preventing snap-back
    - Prevents visual jitter by not updating UI for sub-step movements
  - Float knobs (decay, volume, etc.) unchanged - still use continuous behavior
  - ~13px drag now reliably moves one integer step (vs. previous sticky behavior)

## [1.2.1] - 2026-01-17

### Fixed

- **Clicks when reps=1 (single repeat mode)**
  - Root cause: Fade-out triggered immediately after `startNewRepeat()` incremented counter
  - With reps=1, `currentRepeat` became 1 after starting the repeat, triggering `currentRepeat >= maxRepeats`
  - The single repeat was cut off before it could play
  - Fix: Added check that repeat timer has expired (`samplesUntilNextRepeat <= 0`) before allowing fade-out
  - Now the full repeat plays before transitioning to fade-out

## [1.2.0] - 2026-01-17

### Changed

- **DSP engine reverted to v1.1.4 stable baseline**
  - Rationale: Complex crossfade/retrigger logic added in v1.1.5-v1.1.19 to fix ENV/Sidechain artifacts
    caused regressions in normal beat-sync stutter mode (clicks, audio artifacts)
  - This version prioritizes stable, click-free beat-sync stutter operation
  - RepeatLane.cpp reduced from 745 lines to 542 lines (simpler, more reliable)

### Known Issues

- **ENV and Sidechain trigger modes have audio artifacts**
  - Graininess and clicks when using envelope follower (ENV) triggering
  - Similar artifacts when using sidechain input triggering
  - These modes were not fully stable before the v1.1.5+ fixes, which introduced other regressions
  - Workaround: Use beat-sync, manual, or MIDI trigger modes for clean stutter
  - Future versions may address ENV/Sidechain with a different approach

### Migration Notes

- Beat-sync mode: Works cleanly (no changes needed)
- Manual trigger: Works cleanly (no changes needed)
- MIDI trigger: Works cleanly (no changes needed)
- ENV trigger: May experience artifacts (consider alternative trigger modes)
- Sidechain trigger: May experience artifacts (consider alternative trigger modes)

## [1.1.19] - 2026-01-17

### Fixed

- **Clicks at repeat boundaries eliminated**
  - Root cause: Gain discontinuity when decay is applied in startNewRepeat() after crossfade ends
  - Crossfade was at currentGain, but first sample after crossfade was at currentGain * decayAmount (~10% drop)
  - Fix: Added one-pole lowpass gain smoother
    - `smoothedGain` follows `currentGain` with gradual transitions (~500 samples to settle)
    - When startNewRepeat() applies decay, smoothedGain transitions smoothly instead of jumping
    - Uses `smoothedGain` for audio output instead of raw `currentGain`
  - Preserves v1.1.18's unconditional gain application (no audio loss)
  - Avoids v1.1.17's conditional gain application that caused audio regression

## [1.1.18] - 2026-01-17

### Fixed

- **Critical: Audio regression - only clicks heard, no stutter audio**
  - Root cause: v1.1.17 moved gain application inside conditional if/else blocks
  - This caused audio loss in edge cases where neither branch applied gain correctly
  - Fix: Revert to unconditional gain application AFTER crossfade logic (like v1.1.13)
  - Overlap-add crossfade from v1.1.14 is preserved for smooth loop boundaries
  - Gain is now ALWAYS applied to ensure audio output in all code paths

## [1.1.17] - 2026-01-17

### Fixed (REVERTED in v1.1.18)

- **Critical: Gain discontinuity causing clicks at every repeat boundary**
  - Root cause: Decay was applied in startNewRepeat() AFTER the crossfade completed
  - Crossfade output was at gain 1.0, but next sample was at gain 0.9 (10% drop = click)
  - Fix: Apply gain BEFORE blending in crossfade - old audio uses current gain, new audio uses next repeat's gain
  - Crossfade now smoothly transitions from oldGain to newGain (e.g., 1.0 → 0.9)
  - After startNewRepeat(), currentGain matches what the crossfade ended with = no discontinuity
  - **NOTE: This fix was reverted in v1.1.18 as it caused complete audio loss**

## [1.1.16] - 2026-01-17

### Fixed

- **Crossfade position wrap causing sample replay**
  - After crossfade, continue playback from safeCrossfade offset instead of 0
  - Applied to both timer-based and position-based repeat triggering

## [1.1.15] - 2026-01-17

### Fixed

- **Timer desynchronization causing audio gaps**
  - Playback position increment was inside audio-reading block, causing position to freeze at capture end
  - Fix: Move position increment outside the if block so it always advances every sample
  - Added position-based repeat triggering as fallback for pitch/swing desync

## [1.1.14] - 2026-01-17

### Changed

- **Loop boundary overlap-add crossfade**
  - Detect when approaching end of capture and blend with loop start
  - Crossfade starts BEFORE hitting boundary (not after)
  - Skips during retrigger crossfade or global fade-in to avoid conflicts

## [1.1.13] - 2026-01-17

### Changed

- **Per-lane trigger lockout for all trigger modes (ENV, Sidechain, MIDI)**
  - When a lane is triggered, it now ignores subsequent triggers until all its repetitions complete
  - Lockout is per-lane: Lane 1 being locked doesn't prevent Lane 2 from triggering
  - Triggers received during lockout are discarded (not queued)
  - Uses existing `isRepeating()` check: `currentRepeat < maxRepeats && isTriggered`
  - Lockout releases when last repeat starts playing, allowing natural fade-out
  - Manual trigger button and beat-sync modes are unaffected (only ENV/Sidechain/MIDI)
  - Provides more predictable stutter behavior - each trigger completes its full cycle

## [1.1.12] - 2026-01-17

### Fixed

- **Comprehensive crossfade system overhaul for ENV and Sidechain trigger modes**
  - After 7 previous fix attempts (v1.1.5-v1.1.11), conducted root cause analysis identifying 6 remaining issues
  - All crossfade-related audio artifacts now addressed with industry-standard techniques

- **Equal-power crossfade curve** (replaces linear)
  - Linear crossfades cause -6dB dip at 50% blend point, perceived as momentary volume drop
  - Now uses sine/cosine curves: `sin(t * π/2)` for fade-in, `cos(t * π/2)` for fade-out
  - Maintains constant perceived loudness throughout the crossfade transition

- **Increased crossfade duration from 5ms to 10ms**
  - 10ms covers 2+ complete cycles at 200Hz (the low end of bass frequencies)
  - Provides smoother transitions for low-frequency content while remaining imperceptible
  - Better masks any remaining discontinuities

- **Eliminated double-fading at trigger start**
  - Root cause: Both global envelope fade-in AND loop boundary fade-in were applying during first 10ms
  - This caused double attenuation at stutter onset, making the start sound weak/muffled
  - Fix: Skip loop boundary crossfade while global fade-in is active

- **Fixed deferred capture during retrigger**
  - Root cause: During retrigger, capture position was deferred to next processBlock()
  - But the crossfade needed the NEW capture position immediately to blend properly
  - During the deferral period, crossfade read from stale/incorrect buffer positions
  - Fix: Calculate capture position immediately for retriggers (buffer already has audio)
  - Deferred capture only used for initial triggers (where current block isn't in buffer yet)

- **Fixed old position exhaustion artifacts**
  - Root cause: When old playback position exceeded capture length during crossfade, it clamped to last sample
  - Repeating the same sample value creates a DC-like signal that sounds like graininess
  - Fix: When old position exhausts, its contribution is set to zero instead of clamped
  - The equal-power crossfade naturally handles this - old weight decreases as new weight increases
  - Result: Smooth transition to new audio without artifacts from repeating last sample

- **Fixed retrigger + loop boundary crossfade overlap** (additional fix)
  - Root cause: During retrigger, loop boundary crossfade was NOT being skipped
  - The skip check only looked at `fadeInSamplesRemaining <= 0`, but retriggers don't reset this
  - Result: Two crossfades applied simultaneously - retrigger blend + loop boundary fade-in
  - This caused the "new" audio to be double-attenuated during the first 10ms of retrigger
  - Fix: Also skip loop boundary crossfade when `retriggerCrossfadeActive` is true

## [1.1.11] - 2026-01-17

### Fixed

- **Sample rate compatibility for ENV and Sidechain trigger modes**
  - Root cause: Envelope follower channel index mismatch
  - `envelopeFollowerRight` and `sidechainEnvelopeRight` were using `processSample(1, ...)` (channel 1)
  - But these are separate mono filter instances - each should use channel 0
  - Channel 1 had different filter state that wasn't properly updated at some sample rates
  - This caused inconsistent envelope tracking, leading to clicks/graininess at 48k/96k/192kHz
  - Fix: Changed right channel filters to use `processSample(0, ...)` like left channel
  - Tested at: 44.1kHz, 48kHz, 96kHz, 192kHz - all modes now work correctly

## [1.1.10] - 2026-01-17

### Fixed

- **Graininess/distortion when ENV triggers near end of repeat cycle**
  - Root cause: Retrigger crossfade was cut short when old playback position exceeded capture length
  - The condition `if (oldFractionalPlaybackPosition < oldEffectiveLength)` skipped the blend
  - But the crossfade still had samples remaining, causing a sudden jump from ~50% blend to 100% new
  - This created micro-discontinuities (6 detected in test audio) causing audible graininess
  - Fix: Clamp old position to valid range instead of skipping the blend
  - Crossfade now always completes its full 5ms duration regardless of old position
  - Testing: Play audio with ENV mode - should be smooth without graininess

## [1.1.9] - 2026-01-16

### Fixed

- **Sidechain detection missing hysteresis and cooldown**
  - Root cause: Sidechain trigger detection had NO cooldown or hysteresis (only ENV was fixed in v1.1.8)
  - Sidechain could retrigger on every threshold crossing without any protection
  - Fix: Added same state machine with 6dB hysteresis and 50ms cooldown to sidechain detection
  - Added separate `sidechainCooldownSamplesRemaining` timer for sidechain

- **Sidechain triggering when no sidechain input is connected**
  - Root cause: Some DAWs mirror the main input to unconnected sidechain buses
  - The plugin checked if sidechain bus EXISTS, but not if it's ENABLED by the host
  - Fix: Added `getBus(true, 1)->isEnabled()` check to verify host actually routed audio to sidechain
  - Sidechain detection now only runs when user has explicitly connected a sidechain source

## [1.1.8] - 2026-01-16

### Fixed

- **ENV trigger retriggering due to threshold bounce**
  - Root cause: `wasAboveThreshold` was updated every sample, even during cooldown
  - With music that has varying dynamics, envelope fluctuates around threshold
  - When envelope briefly dipped below threshold during cooldown, `wasAboveThreshold` toggled to `false`
  - After cooldown ended, next sample above threshold caused another trigger
  - Fix 1: State is now locked during cooldown - `wasAboveThreshold` not updated until cooldown ends
  - Fix 2: Added 6dB hysteresis - envelope must drop to 50% of threshold (armThreshold) before re-arming
  - This prevents "threshold bounce" where envelope oscillates around the trigger point
  - State machine: TRIGGER at threshold → LOCK until below armThreshold → RE-ARM → wait for next trigger
  - Testing: Play sustained material with ENV mode - should only trigger on distinct transients, not continuously

## [1.1.7] - 2026-01-16

### Fixed

- **Beat-sync triggering conflict with ENV/Sidechain/MIDI modes**
  - Root cause: `updateBeatSync()` triggered lanes on every DAW subdivision boundary regardless of trigger mode settings
  - When ENV mode was enabled, both systems called `trigger()` simultaneously:
    - Envelope follower triggered on amplitude transients
    - Beat-sync triggered on grid boundaries
  - This caused double triggering, conflicting capture positions, and audio artifacts
  - Fix: Added exclusivity check in `updateBeatSync()` - when any alternative trigger mode (ENV, Sidechain, or MIDI) is active, beat-sync triggering is skipped
  - The selected trigger source now handles triggering exclusively
  - Testing: Enable ENV mode and verify stutter only triggers on transients, not on beat grid

## [1.1.6] - 2026-01-16

### Fixed

- **Distortion/artifacts when ENV (envelope follower) trigger mode is enabled**
  - Root cause 1: No crossfade on retrigger during active playback
    - When retriggering mid-playback, `playbackPosition` reset to 0 without any envelope
    - The v1.1.1 fade-in logic was skipped when `isTriggered` was already true
    - Result: Abrupt audio discontinuity at the trigger point = clicks/distortion
    - Fix: Save old playback state and crossfade blend old→new audio over 5ms
  - Root cause 2: No minimum time between envelope triggers
    - Rapid transients could fire triggers every few milliseconds
    - This caused constant retriggering = glitchy/distorted sound
    - Fix: Added 50ms cooldown timer between envelope triggers
  - Testing: Enable ENV mode with transient-heavy material (drums, percussion)

## [1.1.5] - 2026-01-16

### Fixed

- **Audio artifacts when ENV (envelope follower) triggers on transients**
  - Root cause: Capture timing mismatch in envelope-triggered mode
  - When `trigger()` was called before `processBlock()`, `captureStartPosition` was calculated
    based on the OLD capture buffer contents (from previous blocks)
  - The current block containing the transient hadn't been written yet
  - Result: Stutter played back OLD audio that didn't include the triggering transient
  - Fix: Implemented deferred capture using `pendingCapture` flag
    - `trigger()` now sets `pendingCapture = true` instead of calculating position immediately
    - `processBlock()` calculates `captureStartPosition` AFTER writing current block to buffer
    - This ensures the capture window includes the transient that triggered the effect
  - Testing: Play audio with sharp transients (drums, plucks) with ENV mode enabled

## [1.1.4] - 2026-01-16

### Changed

- **Bug image fragmented into 12 scattered pieces across UI**
  - Original single botanical overlay replaced with 12 randomly-shaped fragments
  - Each fragment uses CSS clip-path with irregular polygon shapes
  - Fragments positioned throughout the interface for artistic scattered effect
  - Slightly reduced opacity (0.25 vs 0.32) for subtler background presence

- **DRY/WET mix knobs repositioned to prevent cut-off**
  - Footer section moved up 15px (765px → 750px) and expanded (50px → 80px height)
  - Mix knobs now fully visible within plugin frame
  - Footer toggles vertically centered in taller footer section

### Added

- **Sequence grid greyed out when SEQ mode inactive**
  - Pattern sequencer section now dims (40% opacity, 30% grayscale) when SEQ toggle is off
  - Visual feedback matches lane disable behavior for consistency
  - Helps users understand when pattern sequencing is bypassed

## [1.1.3] - 2026-01-16

### Changed

- **SUBDIV dial text doubled in size** (8px → 16px) for better readability of subdivision values
- **Knob readout values increased by 2pt** (8px → 10px) across all lane controls
- **Wet/Dry knobs enlarged to match lane knobs** (32px → 42px) for visual consistency

### Removed

- **Parameter monitor debug panel** - Development diagnostic removed from production UI

## [1.1.2] - 2026-01-16

### Fixed

- **Lane control labels no longer overlap with dial value displays**
  - Root cause: Row spacing was 60px but each knob container required ~70px (label + knob + value)
  - This caused knob value text (e.g., "90%", "100%") to overlap with the next row's label
  - Fix: Increased row spacing from 60px to 75px (rows at top: 55px, 130px, 205px, 280px)
  - Expanded horizontal column spread to match progress bar width (columns at left: 20px, 95px, 170px)
  - Increased lanes-section height from 320px to 400px (25% larger)
  - Adjusted downstream sections and expanded plugin frame from 750px to 830px
  - All 4 lanes updated with consistent improved spacing

## [1.1.1] - 2026-01-16

### Fixed

- **Audio clicks at stutter start/end transitions eliminated**
  - Root cause: Missing global envelope for stutter effect boundaries
  - The existing crossfade logic (5ms) only smoothed transitions *within* repeat segments
  - No fade-in was applied when stutter first triggered (abrupt onset)
  - No fade-out was applied when all repeats finished (`buffer.clear()` caused abrupt cut)
  - Fix: Added global attack-release envelope to RepeatLane class
    - `globalEnvelopeGain` tracks envelope level (0.0 to 1.0)
    - `fadeInSamplesRemaining` handles fade-in over `crossfadeSamples` (5ms)
    - `fadeOutActive` / `fadeOutSamplesRemaining` handle smooth fade-out when repeats finish
  - Envelope resets on trigger, cancels fade-out if retriggered during release
  - Testing: Listen for click-free transitions on any audio material

## [1.1.0] - 2026-01-16

### Added

- **Wet/Dry mix controls** - Separate faders for dry signal (0-100%) and wet effect (0-100%)
- **Dedicated capture buffer** - Non-destructive repeat playback architecture

### Fixed

- **Pitch shifting and audio corruption** - Switched from popSample() delay line to dedicated capture buffer

## [1.0.2] - 2026-01-16

### Fixed

- **Lane buttons can now be re-enabled after being disabled**
  - Root cause: CSS `pointer-events: none` on `.lane-container.lane-disabled` blocked the lane header button
  - Fix: Added `pointer-events: auto` to `.lane-header` within disabled lanes to keep header clickable
  - This also fixes subdivision combo box being non-interactable when lane is disabled

### Added

- **Sequencer enable toggle (SEQ button in footer)**
  - New parameter: `sequencer_enabled` (default: ON)
  - When OFF, pattern sequencer is bypassed (all steps treated as enabled)
  - Allows users to easily toggle between sequenced and continuous stutter modes

## [1.0.1] - 2026-01-16

### Fixed

- **UI elements now respond to user interaction** (critical bug fix)
  - Root cause: JavaScript parameter-bindings.js was using incorrect JUCE WebView API
  - Sliders used `state.normalisedValue` (nonexistent property) instead of `state.getNormalisedValue()` / `state.setNormalisedValue()`
  - Toggles used direct `state.value = x` assignment instead of `state.setValue(x)` method call
  - ComboBoxes used `state.selectedId` (nonexistent property) instead of `state.getChoiceIndex()` / `state.setChoiceIndex()`
  - Direct property assignment doesn't emit events to JUCE backend; method calls are required

## [1.0.0] - 2026-01-15

### Added

- Initial release
- 4 independent repeat lanes with per-lane parameters
- 16-step pattern sequencer per lane (64 steps total)
- 5 trigger modes: beat sync, envelope follower, sidechain, manual, MIDI
- Tape degradation section (saturation, wow, flutter, hiss, rolloff, dropout)
- WebView UI with paper texture and botanical aesthetic
- 130 total automatable parameters
