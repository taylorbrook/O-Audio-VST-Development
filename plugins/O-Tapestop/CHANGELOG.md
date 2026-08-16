# Changelog — O-Tapestop

All notable changes to O-Tapestop are documented here.

## [1.0.0] — 2026-08-15

Initial release.

### Added
- **Stop mode** — tape-stop / tape-start varispeed transport: tempo-synced
  (1/16 – 4 bars) or free-time (10 ms – 8 s) spin-down and spin-up with
  independent curve shaping, Signalsmith-style resync (1.25× catch-up +
  crossfade skip, bitwise-dry post-resync null).
- **Scratch mode** — drawable bipolar speed-vs-time envelope (2–64 points,
  per-segment curvature), r ∈ [−2, +2], tempo-synced or free pass length.
- **Tone Track** — speed-tracking low-pass that darkens the wet path as the
  tape slows, wet-path/engaged-only.
- **Mix / Output Gain** on the engaged chain only; the Bypassed state is a
  true bitwise pass-through.
- **WebView UI** (860 × 580, Ouaricon Naturalist): large latching ENGAGE,
  mode-switched center panel, drawable envelope editor with live playhead,
  live playback-ratio readout.
- **Preset system** — shared preset-manager module v1.0.5: save / save-as /
  load / load-from-file / prev / next / two-click delete, factory + user
  banks under `~/Library/Ouaricon Tapestop/Presets/`. The scratch envelope
  rides inside preset JSON as an opaque `customState` blob.
- **Factory bank** — 8 presets: Classic Half-Bar Stop, Classic 1-Bar Stop,
  DJ Spinup, Baby Scratch, Chirp Flare, Tempo-Synced Short Stop,
  Slow-Tape Drag, Stutter-Scratch.
