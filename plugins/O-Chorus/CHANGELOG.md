# O-Chorus Changelog

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
