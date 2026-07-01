# O-Chorus Changelog

## v1.2.3 (2026-06-30)

### Changed

- **UI legibility — darker, slightly larger text.** The knob labels/values and section
  labels used low-contrast tan/beige tones (`#8B7355`, `#a08870`) on the cream paper
  background (`#F5E6D3`), making them hard to read. Darkened to the brown family and bumped
  each text element +1px:
  - Knob values (`1.00 Hz`, `50%`…): `#a08870` → `#5C4A32`, 9px → 10px (value box 12px → 13px).
  - Knob labels (Rate/Depth…): `#8B7355` → `#4A3B2A`, 8px → 9px.
  - Section labels (MODULATION/CHARACTER) + LFO label: `#8B7355` → `#5C4A32`, 7px → 8px, opacity 0.7 → 0.9.
  - Preset bar (name/Load/Save/nav arrows): `#4A3B2A` → `#3C2F2F`, +1px, opacity → 0.95–1.0.
  - Title: 15px → 16px.
- Pure cosmetic CSS change in `Source/ui/public/index.html`; no DSP, parameter, or state
  changes. Aesthetic (vintage naturalist / paper texture) preserved.

## v1.2.2 (2026-06-30)

### Fixed

- **WR-01 — Per-voice delay collapse at high Spread.** At moderate-to-high Spread the
  per-voice base delay went negative (voice 0 at Spread 1.0 = base 10ms − spread 15ms =
  −5ms). JUCE `DelayLine::popSample` silently ignores negative delays (reuses the last
  clamped value) and `setDelay` clamps to 0, so the affected voice collapsed toward ~0ms
  and stopped modulating symmetrically — thinner, lopsided chorusing (worst on the factory
  **Ensemble** preset, voices=8/spread=1.0). Fix: clamp each voice's modulated delay to
  `[1 sample, maxDelaySamples]` before `popSample`. No crash existed (verified against
  `juce_DelayLine.cpp` — no OOB read/NaN), so this was a quality/correctness fix.
- **WR-02 — Double delay-line push during voice-count crossfade.** During the ~50ms
  voice-count crossfade both the old-count and new-count passes ran `popSample` **and**
  `pushSample` on each overlapping voice, advancing that delay line's read/write pointers
  at 2× the real sample rate and writing the input into two adjacent buffer slots — an
  audible pitch/doubling glitch on every Voices change. Fix: unified the two passes into a
  single per-voice loop that multi-taps overlapping voices (`popSample(..., updateReadPointer=false)`
  for the first tap, `true` for the last) and pushes exactly once per voice per sample.
- **WR-03 — Tone filter unstable at low sample rates.** `updateToneFilter` computed
  `1/tan(pi·cutoff/fs)` with cutoff up to 20kHz and no Nyquist guard; at sample rates
  ≤ ~40kHz (e.g. 22.05kHz/32kHz, exercised by pluginval's SR sweep) `tan()` blew up or went
  negative, pushing the biquad poles outside the unit circle → NaN/Inf output. Fix: clamp
  cutoff to `0.49 × Nyquist` before computing coefficients.

### Notes

- Root causes from the 2026-06-30 deep code review (`O-Chorus-CODE-REVIEW.md`, 3 warnings).
- No parameter IDs, ranges, or state format changed — presets and sessions load unchanged.

## v1.2.1 (2026-02-25)

### Added

- Compile-flag gated licensing module (OUARICON_LICENSING)
  - OuariconLicense manager in PluginProcessor
  - License overlay UI in PluginEditor (activation gate)
  - License status listener toggles WebView visibility
  - OFF by default for local dev builds

## v1.2.0 (2026-02-08)

### Added

- **Preset system** via Ouaricon preset-manager module
  - Factory/user preset persistence (JSON-based, stored in ~/Library/O-Chorus/Presets/)
  - Preset navigation (prev/next arrows) with dropdown menu
  - Save/load preset dialogs (native file chooser)
  - DAW session state includes current preset name
  - Program API (getNumPrograms/setCurrentProgram) for DAW preset browsing
- **6 factory presets:**
  - **Classic** — Vintage 2-voice chorus (0.5 Hz, subtle)
  - **Lush** — Rich 6-voice ensemble (slow, deep, wide spread)
  - **Shimmer** — Bright sparkling 4-voice chorus (2 Hz, bright tone)
  - **Ensemble** — Dense 8-voice string ensemble (full spread, full width)
  - **Vibrato** — Pure vibrato effect (3 Hz, 100% wet, single voice)
  - **Warm** — Warm analog-style 3-voice chorus (dark tone, high drive)

## v1.1.0 (2026-02-08)

### Added

- **Spread parameter** (0.0–1.0): Offsets each voice's base delay time across ±15ms range
  - At 0%: All voices share the same base delay (original behavior)
  - At 100%: Voices are distributed symmetrically across a 30ms delay range
  - Makes the Voices parameter audibly meaningful — more voices = richer, thicker sound
  - Inspired by classic multi-voice chorus designs (Juno-60, Dimension D)

### Root Cause

- The Voices parameter previously had minimal audible effect because all voices shared
  the same 10ms base delay time. Only LFO phase offset and tiny depth variation (0.85–1.15x)
  differentiated voices, producing nearly identical tonal results regardless of voice count.

## v1.0.1 (2026-02-08)

### Changed

- Renamed UI display title from "O-Chorus" to "Ouaricon Chorus"

## v1.0.0 (2026-02-08)

### Initial Release

- 8-voice BBD-style chorus engine with Lagrange3rd interpolated delay lines
- 7 parameters: Rate, Depth, Voices, Width, Tone, Mix, Drive
- Per-voice LFO phase offset with seeded depth variation for organic modulation
- Tanh saturation with asymmetric drive for analog warmth
- One-pole tone filter (2kHz-20kHz range)
- Equal-power stereo panning with width control
- Voice count crossfade (50ms) for click-free transitions
- Naturalist-styled WebView UI (700x250) with paper texture background
- LFO ring animation with frame-rate-independent timing
- Knob interaction: vertical drag, shift for fine control, double-click reset, mouse wheel with gesture brackets
- Cross-platform: VST3 + AU, WebView2 static linking for Windows
