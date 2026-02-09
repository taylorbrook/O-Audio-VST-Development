# O-Chorus Changelog

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
