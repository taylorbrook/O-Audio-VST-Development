# Changelog

All notable changes to Drum808 will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0] - 2025-11-13

### Added

- Initial release
- Authentic 808 drum machine emulation with 6 voices
- MIDI-triggered drum synthesis (notes 36, 38, 41, 42, 45, 46)
- 24 parameters (4 per voice: Level, Tone, Decay/Snap, Tuning)
- Multi-output routing (7 stereo buses: main + 6 individual outputs)
- Real-time trigger LEDs with smooth decay animation
- 4 factory presets: Factory Default, Deep Kick, Crispy Hats, Tight Punch

### Audio Processing

**Kick Drum:**
- Bridged-T oscillator emulation with exponential pitch envelope
- Attack transient (noise burst) controlled by Tone parameter
- Base frequency ~60 Hz, tunable ±12 semitones
- Decay range: 50-1000ms

**Low Tom & Mid Tom:**
- Bridged-T oscillator with bandpass filter
- Base frequencies: ~150 Hz (Low), ~220 Hz (Mid)
- Resonance controlled by Tone parameter (Q 0.5-5.0)
- Decay range: 50-1000ms
- Independent tuning ±12 semitones

**Clap:**
- Multi-trigger envelope (3 spikes + decay tail)
- Filtered white noise with tunable bandpass (1000 Hz center)
- Snap parameter controls attack sharpness
- Sample-rate independent timing (10ms spike intervals)

**Closed & Open Hi-Hat:**
- 6-oscillator inharmonic synthesis (ratios: 1.0, 1.4, 1.7, 2.1, 2.5, 3.0)
- Bandpass filtering for metallic character (6-12 kHz)
- Closed hat decay: 20-200ms
- Open hat decay: 100-1000ms
- Voice choking: Closed hat stops open hat (authentic 808 behavior)

**MIDI Implementation:**
- Note 36 (C1) → Kick
- Note 38 (D1) → Clap
- Note 41 (F1) → Low Tom
- Note 42 (F#1) → Closed Hat (chokes open hat)
- Note 45 (A1) → Mid Tom
- Note 46 (A#1) → Open Hat
- Velocity sensitivity (0-127 → 0.0-1.0 gain)
- Omni mode (responds to all MIDI channels)

### User Interface

- WebView-based GUI (1000×500px)
- 6 vertical channel strips (horizontal layout)
- 24 parameter knobs with canvas rendering
- Bidirectional control (UI ↔ DSP, automation ↔ UI)
- Real-time trigger LEDs (60fps animation via requestAnimationFrame)
- Exponential LED decay (200-300ms visual sustain)
- Brushed metal aesthetic matching classic 808 style
- Relative drag interaction (frame-delta pattern)

### Multi-Output Routing

- Bus 0 (channels 0-1): Main mix (all voices)
- Bus 1 (channels 2-3): Kick individual
- Bus 2 (channels 4-5): Low Tom individual
- Bus 3 (channels 6-7): Mid Tom individual
- Bus 4 (channels 8-9): Clap individual
- Bus 5 (channels 10-11): Closed Hat individual
- Bus 6 (channels 12-13): Open Hat individual

### Technical Details

- Zero latency (sample-accurate MIDI triggering)
- Real-time safe (no allocations in processBlock)
- Denormal protection (ScopedNoDenormals)
- Thread-safe parameter communication (atomic loads via APVTS)
- Multi-sample-rate support (44.1kHz, 48kHz, 96kHz, 192kHz tested)
- Cross-platform (macOS VST3 + AU formats)

### Validation

- Passed pluginval strict validation (level 5)
- VST3 format: All tests passed
- AU format: All tests passed, auval approved
- Tested at multiple sample rates and buffer sizes
- All 24 parameters validated
- Multi-output bus configuration verified
- Editor opens and closes cleanly
- No audio artifacts or denormals detected

### Factory Presets

1. **Factory Default** - Balanced 808 sound (all voices at default settings)
2. **Deep Kick** - Bass music preset (low-tuned kick, subdued toms/hats)
3. **Crispy Hats** - Bright hi-hats preset (prominent, crispy hi-hats)
4. **Tight Punch** - Short, punchy preset (fast decay, snappy attack)

### Known Limitations

- No built-in sequencer (MIDI-triggered only)
- No built-in effects (use external processing)
- AU format: Warning about program index -1 (cosmetic, does not affect functionality)
- Multi-output: Not all DAWs support 7-bus configuration (falls back to main mix)
