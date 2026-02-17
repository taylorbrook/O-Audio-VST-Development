# O-Prism Requirements

*Auto-extracted from BRIEF.md on 2026-02-16*

## Functional Requirements

### FR-01: Wavetable Oscillators (x2)
- Two independent wavetable oscillators with position/morph control
- 2048 samples per frame, up to 256 frames per table
- Continuous position morphing between frames
- Per-oscillator: level, pan, coarse tune (±24st), fine tune (±100c), phase
- Unison: 1-8 voices with detune spread and stereo width
- Bandlimited interpolation for anti-aliasing

### FR-02: Sub Oscillator
- Waveform shapes: Sine, Triangle, Saw, Square
- Octave: -2 to 0 relative to fundamental
- Level control
- Routes directly to output (bypasses filters)

### FR-03: Noise Generator
- Types: White, Pink, Brown, Digital, Vinyl, Wind
- Level control
- Routable to filter chain or direct output

### FR-04: Dual Multi-Mode Filters
- Filter A and Filter B, each with:
  - Types: LP12, LP24, HP12, HP24, BP12, BP24, Notch
  - Cutoff: 20Hz - 20kHz
  - Resonance: 0-100% (self-oscillating at max for LP/HP)
  - Drive: 0-100%
  - Key tracking: 0-100%
- Routing modes: Serial (A→B), Parallel (A+B)

### FR-05: Amplitude Envelope
- ADSR with: Attack (0.001-10s), Decay (0.001-10s), Sustain (0-100%), Release (0.001-20s)
- Per-voice, applied after filter chain

### FR-06: Filter Envelope
- ADSR with depth control (-100% to +100%)
- Modulates filter cutoff
- Per-voice

### FR-07: Effects Chain (5 Effects)
- **Reverb:** Hall/Plate, size, damping, pre-delay, mix
- **Delay:** Sync/Free, ping-pong, feedback, filter, mix
- **Chorus:** Rate, depth, voices, stereo, mix
- **Distortion:** SoftClip/HardClip/Tube/Fold, drive, mix
- **EQ:** 3-band parametric (low/mid/high gain, mid freq)

### FR-08: Microtonal Engine
- Direct port of TuningEngine from O-Lyrica/O-Bells
- 24+ factory tunings across 5 categories
- Scala (.scl) file import with cents and ratio support
- Keyboard mapping (.kbm) file support
- Scale generators: EDO, harmonic series, rank-2 temperament
- Tonic selection with anchor-point shifting
- Master tuning (A4 = 420-460 Hz)
- Octave stretch (0.95-1.25)
- Per-note pitch bend
- Lock-free 128-note frequency table

### FR-09: Wavetable Import
- Import WAV/AIFF audio files as wavetables
- Auto-split into 2048-sample frames
- Resampling for non-standard frame sizes

### FR-10: Factory Wavetable Library
- 100+ wavetables across categories: Analog, Digital, Spectral, Organic, Textural

### FR-11: Voice Management
- 16-voice polyphony
- Voice stealing (oldest note priority)
- Glide/Portamento: Off, Legato, Always with adjustable time
- Pitch bend range: ±1 to ±48 semitones

### FR-12: Global Controls
- Master volume
- Oscillator A/B mix control
- Polyphony control (1-16)

## Non-Functional Requirements

### NFR-01: Audio Quality
- 64-bit double precision internal processing
- 2x oversampling for wavetable anti-aliasing
- Zero latency (no lookahead)
- Support: 44.1kHz, 48kHz, 88.2kHz, 96kHz sample rates

### NFR-02: CPU Performance
- Under 25% CPU at 16 voices with unison on modern hardware
- Lock-free audio thread (no mutex in processBlock)
- Pre-computed frequency tables for microtonal

### NFR-03: UI Performance
- WebView at 1200x800 pixels
- 60fps wavetable visualization
- Responsive parameter updates

### NFR-04: Plugin Formats
- VST3 (macOS + Windows)
- AU (macOS only)
- Standalone

### NFR-05: Aesthetic
- Ouaricon Naturalist brand (ouaricon-naturalist-001)
- Butterfly botanical overlay
- Adapted for 60+ parameters at 1200x800

## Dependencies

### DEP-01: Existing Code
- TuningEngine.h/cpp (from O-Lyrica or O-Bells)
- ScaleGenerator.h/cpp
- EmbeddedTunings.h/cpp
- TuningExporter.h/cpp (optional)

### DEP-02: External
- JUCE 8.0.4
- WebView2 (Windows)
- Factory wavetable generation/sourcing

## Constraints

### CON-01: Complexity
- Tier 4 (Very High) — most complex plugin in catalog
- 60+ APVTS parameters
- Novel DSP (wavetable engine not in any existing plugin)

### CON-02: Roadmap
- v1.0: All requirements above
- v2.0: Modulation matrix, wavetable editor, additional effects/filters, preset browser
