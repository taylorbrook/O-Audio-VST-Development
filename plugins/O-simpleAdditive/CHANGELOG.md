# Changelog — O-simpleAdditive

All notable changes to this plugin are documented here.
Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.0.5] — 2026-08-17

UI layout pass: the whole interface — including the on-screen keyboard — now fits
the 860×980 editor without scrolling. No parameter, preset, state, or DSP changes;
this is presentation only.

### Fixed
- **The on-screen keyboard was entirely below the fold and required scrolling to
  reach.** Measured at the shipped 860×980 editor size, the `.frame` scroll
  container needed 1251 px of content in 974 px of usable height — a 277 px
  overflow that pushed the lesson-preset row and the whole keyboard panel out of
  view, and clipped the Output group mid-knob.

  Root cause: the four control groups (Morph · Wavetable, Spectral Shaping, the
  envelope pair, Output) were stacked vertically, so `.controls` alone consumed
  542 px — more than half the window — while each group left most of its width
  empty. The single-knob Output group cost ~130 px of height to show one control.

  Fixed by laying the groups out in two horizontal rows rather than four stacks,
  which reclaimed 295 px of the 277 px needed. The reclaimed space was then spent
  back on the elements that carry the design, so nothing that matters got smaller:
  the drawbars keep their full 168 px travel, the knobs their original 56/48 px
  diameters, and the keyboard is now *taller* than before (96 px, was 92 px).

### Changed
- **Control groups now sit in two rows** (`.group-row`): Morph · Wavetable beside
  Spectral Shaping, then Amplitude Envelope · Modulation Envelope · Output. The
  former `.env-pair` rule is generalised into `.group-row` and the standalone
  Output section is folded into the second row as a narrow third panel. `.controls`
  drops from 542 px to 247 px.
- **The oscilloscope is now the single elastic section** (`flex: 1 0 136px`). It
  absorbs whatever vertical slack the frame has left after every other section
  takes its natural size, so the keyboard lands on the bottom edge instead of
  floating above dead paper — 136 px at minimum, 198 px at the 980 px editor
  height. `flex-shrink` stays 0, so 136 px is a hard floor: a host that renders
  text taller makes the scope grow less rather than clipping anything.
- **Padding and gaps tightened** across the frame, panels, group headers, and the
  preset-tour and keyboard footers (roughly 2–4 px each). The lesson-preset
  caption's `max-width` went 46% → 54% so it stops wrapping onto a second line.
- Header title 26 px → 25 px; combo boxes 40 px → 36 px tall.

### Testing
- Layout measured in headless Chromium against the real `index.html` / `styles.css`
  / `app.js`, with only the JUCE ES-module namespace stubbed. At an 860×980
  viewport: content height 912 px in 974 px usable, **0 px overflow**, 62 px of
  headroom, no knob-row wrapping, lesson-preset buttons on one line, keyboard
  fully within the viewport. The same probe reported the 277 px overflow before
  the change, so it discriminates.
- Font-fallback sweep (Garamond stack, Times New Roman, Georgia, generic serif,
  sans-serif) moves total height by ≤ 2 px — the layout is dominated by fixed-px
  elements — so headroom holds at ≥ 59 px whichever serif the host's WebView
  resolves.
- Short-window degradation checked at 860×720: the scope floors at exactly 136 px,
  the frame falls back to scrolling, and the keyboard stays reachable. Nothing
  clips.

## [1.0.4] — 2026-07-15

Code-review resolution pass, part 2: the six Info findings (IN-01..IN-06) deferred
from the v1.0.3 pass (CODE_REVIEW.md 2026-07-15). All are hardening/cleanup — no
audible change at standard sample rates, no parameter or state changes.

### Fixed
- **IN-03 — Nyquist edge: a fundamental at/above Nyquist now renders silence, not
  aliasing.** `computeKmax` clamped the harmonic count to a minimum of 1, so when
  `f0 ≥ 0.5·fs` (only reachable at sample rates below ~25 kHz) the fundamental was
  written above Nyquist and aliased. Root cause: `jlimit (1, …)` forbade the
  legitimate `Kmax = 0` case. Now clamped to `[0, 16]`; `nyquistGain`'s `k > Kmax`
  check already zeroes every partial at `Kmax = 0`, so `refillTable` naturally
  produces a silent table. Unreachable at 44.1 kHz+; closes the one hole in the
  otherwise-exact band-limit.
- **IN-04 — `uiMidi` native boundary now validates its arguments.** `handleUiMidi`
  built MIDI messages from the raw bridge int (out-of-range → JUCE `jassert` /
  malformed message) and a NaN velocity passed through `jlimit` (NaN comparisons
  are false). Note number is clamped to 0–127 and non-finite velocity falls back
  to 0.8 before the message is queued.
- **IN-05 — `midiCollector` given a valid timestamp base at construction.** The
  collector was only `reset()` in `prepareToPlay`; on-screen-keyboard messages
  queued before the host's first prepare (Standalone startup, editor on a
  suspended plugin) hit an unreset collector (debug assertion, undefined
  timestamp base). Constructor now seeds `reset (44100.0)`; `prepareToPlay`
  re-resets with the real rate.
- **IN-06 — knobs and drawbars now publish ARIA value attributes.** `role="slider"`
  elements had no `aria-valuenow/-min/-max`, so screen readers announced valueless
  sliders. Knobs set min/max from the pushed C++ range properties (skew-safe, no
  hardcoded JS ranges), `aria-valuenow` from `getScaledValue()`, `aria-valuetext`
  via the existing FORMAT map, and an `aria-label` from the tooltip title;
  drawbars publish 0–100 (%) matching their readout.

### Removed
- **IN-01 — dead `getSampleRate` bridge endpoint.** Registered "for future
  frequency-axis labels" but never called from JS; removed until a caller exists
  so it cannot drift into an assumed-working API.
- **IN-02 — dead `currentNote` voice member.** Written in `startNote`, never read
  (the base class already tracks `getCurrentlyPlayingNote()`).

## [1.0.3] — 2026-07-15

Code-review resolution pass (CODE_REVIEW.md 2026-07-15, WR-01..WR-06). Also
**recovers the lost v1.0.1/v1.0.2 source**: those releases were built, installed,
and recorded in PLUGINS.md but never committed — the working tree had silently
reverted to v1.0.0. The complete v1.0.2 source was restored from
`backups/O-simpleAdditive/v1.0.2/` before applying the fixes below (WR-06).

### Fixed
- **WR-01 — sine LUT no longer constructed on the audio thread.** `fastSine`'s
  function-local `static SineTable` was first touched via `startNote →
  refillTable`, i.e. inside the first note-on's render call: a magic-static guard
  (potential mutex), a `LookupTableTransform` heap allocation, and 1024 `std::sin`
  calls on the RT thread — at the most audible moment. Root cause: lazy
  initialization with an audio-thread-only call site. The LUT is now touched once
  in `AdditiveVoice::prepareToPlay` (message/host thread), so the static is fully
  constructed before any `renderNextBlock`.
- **WR-02 — Spectral Decay / Vel→Decay knobs no longer go dead after 2 s.** The
  only re-dirty path for a decay-rate change was the `rate > 0 && tau < 1` render
  branch; once `tau` saturated at 1 (≈2 s into a note) rate changes were silently
  ignored, and turning the knob to 0 mid-note left the table frozen at its darkest
  state instead of restoring the undecayed spectrum. Root cause: the dirty check
  tracked the ramp, not the rate. `renderNextBlock` now compares the effective
  rate against `lastRenderedDecayRate` and re-dirties the table on any change
  while `tau > 0` — covering both the saturated-tau sweep and the rate→0 restore.
- **WR-03 — `applyFactoryPreset` parameter writes now gestured.** All 33
  `setValueNotifyingHost` calls (reset loop + `setReal`/`setChoice`) are bracketed
  by `beginChangeGesture`/`endChangeGesture`. Un-gestured edits map to a
  `performEdit` without `beginEdit` in the VST3 wrapper; hosts that gate
  automation recording on gestures (Logic touch/latch, Cubase) could drop or
  mis-record lesson-preset moves.
- **WR-04 — stuck knob/drawbar drags eliminated.** Drag end depended on a
  `pointerup` reaching `window`; releasing the mouse outside the plugin window (or
  a `pointercancel`) left the drag active — knob glued to the cursor and the host
  automation gesture open indefinitely. Knobs and drawbar tracks now
  `setPointerCapture` on `pointerdown` (guaranteeing up/cancel delivery) and treat
  `pointercancel` as `pointerup`.
- **WR-05 — stuck on-screen-keyboard notes eliminated.** Note-off depended on
  `pointerup`/`keyup` reaching the WebView; releasing outside the window, a
  `pointercancel`, or clicking away to the DAW mid-keypress left `heldNotes`
  populated and the note droning indefinitely. Added a panic path: `releaseAll()`
  on window `blur` and `visibilitychange` (hidden), plus a `pointercancel`
  handler for pointer-driven notes. (No pointer capture on the keyboard — key
  glissando relies on `pointerover` retargeting.)
- **WR-06 — version drift resolved by restoring the lost releases.** Source said
  1.0.0 while PLUGINS.md and the installed binaries said 1.0.2. Investigation
  showed v1.0.1/v1.0.2 were real (built, installed, backed up) but never
  committed, and the tree had reverted. Restored the v1.0.2 source (including
  `tests/render-harness/`) and released this pass as 1.0.3 across CMakeLists,
  CHANGELOG, STATUS.md, and PLUGINS.md.

### Notes
- Info findings IN-01..IN-06 from the same review are deferred (opt-in); see
  NOTES.md Known Limitations.

## [1.0.2] — 2026-06-25

Per-voice CPU optimization for continuous wavetable motion. No change to static
patches (bit-identical) and no change at common large host block sizes.

### Changed
- **Refill-cadence cap (PERF).** `AdditiveVoice::refillTable()` rebuilds the whole
  2048-point single-cycle table — a fixed ~2048×16 sine-sum cost *per call,
  independent of the host block size*. During continuous motion (scan/LFO/mod-env/
  spectral-decay) the table is marked dirty every block, so on small host blocks
  the rebuild fired far more often per second than on large ones and the cost did
  not amortize. Motion-driven refills are now bounded to a control-rate interval
  (~5 ms, resolved from the sample rate in `prepareToPlay`): a minimum number of
  samples must elapse between rebuilds.

  **Root cause / profile (offline render-harness, 16-voice Morph-Pad chord, dense
  Saw spectrum):** MOVING-regime CPU climbed from 4.7 % of a core at 512-sample
  blocks to 36.7 % at 64-sample blocks (perfect 2× per block-size halving) while a
  STATIC patch stayed flat at ~0.2–0.3 % — i.e. the per-block full-table rebuild
  was the hot cost and did not amortize across small blocks.

  **After:** 64-sample MOVING dropped 36.7 % → 9.0 % (4.1×), 128-sample 18.6 % →
  9.0 % (2.1×); 256/512-sample blocks unchanged (the cap is a no-op once the host
  block already exceeds the interval). The capped small-block cadence (~256 samples
  ≈ 5.8 ms) matches the cadence a 256-sample host already used, so the 20 ms scan
  smoother's zipper-free guarantee is preserved by construction.

- **No-regression guarantees (verified by the render-harness golden battery):**
  static patches (decay 0, LFO depth 0) keep the once-per-note refill and are
  **bit-identical** (`maxAbsDiff = 0`); moving patches rendered at host blocks ≥ the
  cap interval (512) are **also bit-identical**, proving the fill math itself is
  untouched — only the small-block refill *cadence* changed.

### Added
- **`tests/render-harness/`** — offline Stage-2 correctness gate + refill profile
  for this plugin (mirrors the O-simpleFM/O-simpleGrain harnesses). `--profile`
  measures STATIC-vs-MOVING per-voice CPU across host block sizes; `--dump-golden`/
  `--check-golden` capture and bit-compare the static + large-block-moving battery.
  Off by default (`-DOUARICON_BUILD_TESTS=ON`).

## [1.0.1] — 2026-06-25

Code-quality cleanup bundle. No change to the synth's audio output; one
display-accuracy refinement to the live drawbar glow.

### Fixed
- **Live drawbar glow now band-limited.** `refillTable()` published the
  *pre*-band-limit partial amplitudes into `activeSpectrum[]`, so on high notes
  partials above Nyquist (`k > Kmax`) lit the green live-glow even though they are
  never written into the table and make no sound. The snapshot is now taken
  *after* the `nyquistGain(k+1, Kmax)` band-limit, so the glow shows only what is
  actually sounding — matching the drawbar tooltip's promise (QUAL-02). Audio
  output (the wavetable itself) is unchanged.
- **`frameBSource` choice resolved robustly.** `pushParamsToVoices()` cast the raw
  parameter value with a truncating `(int)` cast; it now uses
  `jlimit(0, 3, (int) std::round(...))`, matching how the bit-depth choice is
  resolved. Same result for the four valid choice indices, but no longer relies on
  the float landing exactly on an integer.

### Changed
- **Single source of truth for the 16 partial IDs.** The `partialIds[16]` string
  array was duplicated four times (createParameterLayout, pushParamsToVoices,
  applyFactoryPreset, and the editor's `sliderIds`). Hoisted one
  `constexpr std::array` into `OSimpleAdditive::ParamIDs` (PluginProcessor.h) and
  referenced everywhere.
- **Single source of truth for lesson captions (WebView).** The lesson copy lived
  twice in `app.js` — once in `TIPS.lesson*` and again in `LESSONS`. `LESSONS` is
  now canonical; the per-button hover tooltips are derived from it. (Hover-tooltip
  wording for the six lesson buttons changes slightly as a result; tour-caption
  text is unchanged.)
- `-Wfloat-equal` hygiene: the `band[k] != 0.0f` silent-partial skip in
  `refillTable()` now uses `juce::exactlyEqual`, matching the rest of the codebase.
- Removed stale stage-process comments (the "lifted verbatim from FmVizAnalyzer",
  "not yet constructed", and "Stage 2 (complete)" framing in the file headers).

## [1.0.0] — 2026-06-22

First complete release. A pedagogical 16-partial **additive** synth with a light
**wavetable** dimension — the additive sibling to O-simpleFM. The 16 drawbars *are* the
spectrum: you build a tone harmonic-by-harmonic and watch it on the live display.

### Synthesis engine (Stage 2)
- **16-partial additive voice**, 16-voice polyphonic (`AdditiveVoice : juce::SynthesiserVoice`).
  Each note is summed into a band-limited single-cycle wavetable (2048-pt) and read by phase —
  not a per-sample sum-of-sines — so CPU stays flat and there is **zero added latency**
  (`setLatencySamples(0)`).
- **Exact per-note anti-aliasing** — partials above Nyquist (`k > Kmax = floor(0.5·fs/f0)`) are
  never written; a raised-cosine taper on the top 2 surviving harmonics avoids boundary clicks.
  No oversampling. High notes with all drawbars up stay clean.
- **Wavetable scan/morph** — per-partial linear *spectral* morph from Frame A (the drawbars)
  toward a Frame B preset (Sine / Saw / Square / Odd), driven by a manual knob, a global sine
  LFO, and a mod-envelope. Zipper-free via a 20 ms smoother + control-rate table refill.
- **Spectral-decay macro** — per-partial exponential tilt over the note (`D_k = exp(−rate·k·τ)`),
  with optional velocity routing (`velToDecay`).
- **Bit-depth quantizer** — discrete `{Off, 12, 10, 8, 6, 4, 2}` mid-tread crusher for lo-fi grit.
- **Dual ADSR** — amp envelope (voice lifetime) + an independent mod-envelope routed to scan.
- Headroom-normalized table sum (÷ max(1, Σ amplitudes)) so 16 maxed drawbars don't clip.
- `ScopedNoDenormals` + block-level `isfinite` scrub; floor-modulo phase wrap.

### Interface (Stage 3) — "Additive Field Guide" WebView
- Single-page classroom/projector-readable layout (sibling aesthetic to O-simpleFM).
- **16 drawbars double as the live spectrum** — brass set-level + green live-glow showing the
  exact *morphed + decayed* active-spectrum snapshot (not an FFT estimate).
- Live **oscilloscope** of the summed waveform (30 Hz message-thread Timer over a lock-free
  `VizRing`; analyzer copies the window before its in-place FFT — no corruption, no audio-thread
  allocation).
- 33 parameters two-way bound (31 `WebSliderRelay` + 2 `WebComboBoxRelay`).
- Plain-language **hover tooltips on every control** (overtone series, why odd-only is hollow,
  what scan/morph/bit-depth do).
- **6 lesson presets** — Pure Sine, Sawtooth, Square (hollow), Organ, Morph Pad, Lo-Fi Bells —
  each isolates one concept via a full APVTS snapshot.
- On-screen keyboard MIDI (`MidiMessageCollector` + drain into the synth).
- Cross-platform WebView wired: `NEEDS_WEBVIEW2` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING` +
  `withUserDataFolder` for Windows; custom-scheme resource provider for macOS.

### Validation (Stage 4)
- **pluginval strictness 8 — VST3: SUCCESS, AU: SUCCESS** (render, automation, parameter fuzz,
  state restore, threading, bus layouts).
- **`auval -v aumu OSiA OuDv` — AU VALIDATION SUCCEEDED** (incl. Test MIDI render).
- Factory-preset sweep: all 6 lessons apply finite, in-range snapshots (reset-to-default first).
- Aliasing audit: band-limit + taper + headroom + finite-phase guard confirmed exact; fuzz/render
  at the top of the keyboard surfaced no NaN/Inf/denormal.
- Default patch (H1=100%, rest 0) is a pure sine — unregressed from the Stage 2 DSP.

### Deferred to v1.1
- Persistent user preset save/load bar (`OuariconPresetManager`).
- Per-partial mod-env decay routing.
