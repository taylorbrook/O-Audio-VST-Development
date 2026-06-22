# Changelog — O-simpleFM

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.2.1] — 2026-06-21

A small teaching-copy addition: hover explanations on the Lesson Preset buttons.
No DSP, parameter, or mechanism changes.

### Added
- **Hover tooltips on the five Lesson Preset buttons** (E-Piano, Tubular Bell, Brass,
  Clarinet, Clang Bell). Each explains *how that voice is built* in FM terms — the
  carrier:modulator ratio (harmonic vs inharmonic), the modulation index and how the
  envelope drives it, feedback, and the resulting spectrum — so the buttons teach the
  synthesis rather than just loading a sound. Copy mirrors the actual values in
  `FactoryPresets.cpp` (e.g. E-Piano 1:1 with a fast index sweep; Tubular Bell's
  inharmonic 1.41; Clang Bell's index-14 + 60% feedback smear).
- **Implementation:** reuses the existing `setupTooltips()` engine — each `.tour-btn`
  gained a `data-tip="lesson…"` attribute and a matching entry in the `TIPS` table in
  `app.js`. No new tooltip mechanism, so the buttons inherit the same pointer-hover,
  keyboard-focus, and Escape-to-dismiss behaviour as every other annotated control.

### Validation
- WebView-only change (HTML `data-tip` + JS `TIPS` copy); no C++, DSP, or parameter
  surface touched. Verified the build and `auval`/`pluginval` still pass (see build log).

## [1.2.0] — 2026-06-21

Two teaching visuals that make the FM math legible on the live spectrum. No DSP
or parameter changes — viz-only overlays driven from the message thread.

### Added
- **Sideband markers on the spectrum.** The spectrum now overlays the predicted FM
  component frequencies — the carrier f_c (solid amber, labelled) and the sidebands
  f_c ± k·f_m for k = 1..8 (faint dashed sage) — so peaks visibly land where
  Chowning's math says. f_m mirrors the engine exactly: the fixed Hz in Fixed Mode,
  otherwise f_c·ratio with integer snap applied. Markers are mapped onto the existing
  log-frequency axis (20 Hz → Nyquist) and only drawn while a note is sounding;
  anything below 20 Hz or above Nyquist is skipped.
  - **New plumbing:** the carrier frequency reaches JS via a `carrierUpdate` event
    emitted from the editor's 30 Hz timer, *just before* `spectrumUpdate`, so the
    markers stay in sync with the frame they annotate. The processor tracks the
    most-recently-started note's pitch (from the merged host + on-screen-keyboard MIDI
    stream) and whether any voice is still audible (`getCarrierHz()` returns 0 when
    silent). All marker math runs on the message thread — the audio thread only does
    two relaxed atomic stores, preserving the PERF-01 real-time-safety model.
- **Carrier-null indicator.** A green "carrier null" badge lights next to the Signal
  Path I readout when the modulation index reaches the first Bessel J₀ zero. It is
  gated on the *effective* radian index β = 20·(I/20)^1.7 (the perceptual taper the
  DSP applies), matching the render-harness `carrier-null@2.405` test — so the badge
  fires exactly when the carrier marker sits on a nulled peak (≈ readout I 5.75), not
  at a literal readout value of 2.405 (which would contradict the spectrum).

### Validation
- Render harness 7/7 (incl. `carrier-null@2.405`), `pluginval --strictness-level 10`
  (VST3), and `auval` (AU) re-run after the changes (see build log).

## [1.1.0] — 2026-06-21

A teaching-and-cleanup release: an on-screen keyboard so the plugin makes sound
without external MIDI, two new live readouts, and a round of code de-duplication.

### Added
- **On-screen keyboard** (C3–C5). Click/glide with the mouse or play the computer
  keyboard (A S D F G H J K naturals, W E T Y U sharps → C4–C5). Notes are queued
  through a `juce::MidiMessageCollector` and merged into `processBlock`'s MIDI
  stream, so the Standalone build (and any host) makes sound — and the live
  spectrum/scope respond — without an external MIDI source. Keys light up on play.
- **Spectrum frequency-axis labels** (100 Hz / 1k / 10k) drawn on the log-frequency
  axis, so peaks read against actual pitch. Sample rate is pulled from C++ via a new
  `getSampleRate` native function.
- **Harmonic / inharmonic + sideband-count readout** under the Signal Path numbers:
  flags whether the current C:M ratio is harmonic, and shows Carson's rule estimate
  of significant sidebands (≈ I + 1).

### Changed
- **Lesson Presets now load the factory presets by name** instead of carrying a second
  hand-maintained copy of the five sounds in JS. `FactoryPresets.cpp` is the single
  source of truth; the JS only holds the teaching captions. Picking a lesson now also
  updates the preset-bar name. (Removes a silent-divergence hazard between the tour and
  the same-named factory preset.)

### Fixed
- **Index-ceiling code path was dead and self-contradicting.** `FMVoice::renderNextBlock`
  computed the Carson anti-alias index ceiling once per block (per the comment) but then
  re-armed the smoother's target *every sample*, overwriting it — so the documented
  per-block optimization never took effect. Consolidated into a single
  `computeIndexCeiling()` helper armed once per block, and snapped via
  `setCurrentAndTargetValue` on note-on so the ceiling is enforced from sample 0 (it
  previously inited to 1e9 and only ramped down over ~10 ms, briefly under-clamping the
  first note / large pitch jumps).

### Internal
- De-duplicated the knob wheel/arrow-key fine-adjust into a shared `nudge()` helper and
  consolidated two `resize` listeners into one (resize backing store + redraw). Removed a
  dead empty `dblclick` handler.

### Validation
- Render harness 7/7, `pluginval --strictness-level 10` (VST3), and `auval` (AU) re-run
  after the changes (see build log).

## [1.0.2] — 2026-06-20

### Added
- **Explanatory tooltip for the Signal Path readout** (`1.00 : 1 · I = 0.0`). The
  numbers at the right of the routing row now have their own hover/focus tooltip
  explaining that the left value is the **C : M ratio** (modulator frequency vs. the
  played note → which harmonics appear) and the right value is **I**, the modulation
  index (→ brightness / number of sidebands). Previously the readout inherited the
  generic "Signal Path" tip, which didn't explain the numbers. Added a `cursor:help`
  affordance to signal it's hoverable.

### Fixed
- `focusin` handlers now `stopPropagation`, so a nested `[data-tip]` element (the new
  readout inside the routing panel) isn't overridden by its ancestor's tooltip on the
  event bubble — keyboard focus shows the correct tip.

## [1.0.1] — 2026-06-20

### Fixed
- **UI overlap / clipping.** At the 760×720 editor size the field-guide content
  (~880px tall) exceeded the window. `.frame` was a `height:100%` flex column and
  `.controls` had `flex:1; min-height:0`, so the controls block shrank *below* its
  intrinsic height; its children (Output knob) overflowed the collapsed box and
  collided with the **Lesson Presets** row that follows it. With `html,body`
  clipping (`overflow:hidden`), nothing could scroll.

### Changed
- `.frame` is now the scroll container (`overflow-y:auto`); the decorative border
  stays fixed framing the window while content scrolls. All direct sections are
  `flex-shrink:0` so they keep their natural height instead of squishing together.
- Added an aged-paper styled scrollbar (`::-webkit-scrollbar`).
- Editor window grown 760×720 → **760×860** so the full layout seats without
  scrolling on a normal desktop; the frame scroll handles shorter screens / hosts
  that clamp window height.

## [1.0.0] — 2026-06-20

First release. A pedagogical 2-operator FM / phase-modulation synthesizer with a
field-guide "Naturalist" WebView UI, built to make *"oh, THAT's how FM works"* land
in about five minutes.

### Synth engine (DSP)
- 16-voice polyphonic 2-operator **phase-modulation** voice (radians convention,
  1:1 with Chowning/Bessel math). MIDI instrument, audio out.
- Modulation Index `I` 0–20 with a perceptual `I = 20·norm^1.7` taper; carrier null
  reachable at I ≈ 2.405 (Bessel J₀ zero) as a teaching marker.
- C:M **Ratio** with optional integer **Snap** (harmonic ⇄ inharmonic), plus a
  **Fixed-Hz** modulator mode for key-independent, formant-like colour.
- DX7-style modulator **self-feedback** (two-sample average / Tomisawa anti-hunting,
  history clamp + NaN scrub + note-on reset) — sine → saw → noise, stable at 100%.
- Independent **mod** and **amp** ADSR envelopes; mod-env → index (depth default 1.0),
  optional velocity → index; amp envelope governs voice lifetime.
- Anti-aliasing: sine LUT + key-tracked **Carson index ceiling** + 2× polyphase-IIR
  oversampling, always on (v1.0 sine-only chain).

### Interface (WebView)
- Single-page Ouaricon-Naturalist UI; all 17 parameters two-way bound (relative-drag
  knobs, wheel, and **keyboard** arrow-key control).
- Live **spectrum** (4096-pt FFT, Blackman-Harris) + **oscilloscope**, pushed at 30 Hz
  off a lock-free audio-thread tap (no audio-thread FFT/alloc).
- Live operator **routing diagram** (MOD → CAR + feedback loop, thickness ← index/feedback).
- Plain-language **tooltips** on every parameter, reachable by mouse *and* keyboard focus.
- **Lesson Presets** tour (E-Piano, Tubular Bell, Brass, Clarinet, Clang Bell) — each
  isolates one FM concept.

### Presets
- Suite-canonical **preset manager**: factory + user presets persisted as JSON under
  `~/Library/O-simpleFM/Presets/`, surviving DAW session reloads.
- In-UI **preset browser**: factory/user list, prev/next navigation, save (native
  dialog), and delete (factory presets protected — Delete disables on them).
- 6 factory presets shipped (Default + the five lessons).

### Validation
- `auval` SUCCEEDED (AU); `pluginval --strictness-level 10` SUCCESS (VST3).
- Offline render harness: 7/7 — makes-sound, pitch, index→sidebands, carrier-null@2.405,
  feedback-stable, plus high-pitch and fixed-Hz **aliasing-budget** audits.

### Platforms
- macOS: VST3 + AU + Standalone. Windows VST3 cross-platform flags in place
  (WebView2 static-link + user-data-folder); Windows build not produced this cycle.
