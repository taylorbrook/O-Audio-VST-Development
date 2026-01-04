# Drum808 - Creative Brief

## Overview

**Type:** Synth (Drum Instrument)
**Core Concept:** MIDI-triggered 808 drum machine clone with six authentic analog-modeled voices
**Status:** 💡 Ideated
**Created:** 2025-11-13

## Vision

Drum808 is an authentic emulation of the legendary Roland TR-808 drum machine, focusing on circuit-accurate synthesis for six classic voices: kick, low tom, mid tom, clap, closed hat, and open hat. Unlike hardware-style step sequencers, this plugin integrates seamlessly with modern DAW workflows via MIDI triggering, allowing you to program patterns in your sequencer while retaining the warm, analog character of the original 808.

The synthesis engine faithfully recreates the analog circuits of each drum voice—bridged-T oscillators for toms, twin-T resonators for clap, noise-based hi-hats with tuned resonance—delivering the punchy, musical character that defined electronic music genres from electro to hip-hop to house. Modern enhancements include individual outputs per drum for external processing, velocity sensitivity for dynamic performance, and per-drum tuning for harmonic adjustment.

## Parameters

### Kick (4 parameters)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Level | 0-100% | 80% | Output volume |
| Tone | 0-100% | 50% | High-frequency content (attack click) |
| Decay | 50-1000ms | 400ms | Envelope decay time |
| Tuning | -12 to +12 st | 0 st | Pitch offset in semitones |

**DSP:** Bridged-T twin oscillator (low-frequency sine + higher-frequency component), exponential pitch envelope, amplitude envelope with tone-controlled attack transient.

### Low Tom (4 parameters)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Level | 0-100% | 75% | Output volume |
| Tone | 0-100% | 50% | Resonance peak frequency |
| Decay | 50-1000ms | 300ms | Envelope decay time |
| Tuning | -12 to +12 st | 0 st | Pitch offset in semitones |

**DSP:** Bridged-T oscillator with tuned resonance, exponential decay envelope.

### Mid Tom (4 parameters)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Level | 0-100% | 75% | Output volume |
| Tone | 0-100% | 50% | Resonance peak frequency |
| Decay | 50-1000ms | 250ms | Envelope decay time |
| Tuning | -12 to +12 st | +5 st | Pitch offset in semitones |

**DSP:** Same architecture as low tom, tuned higher by default.

### Clap (4 parameters)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Level | 0-100% | 70% | Output volume |
| Tone | 0-100% | 50% | High-frequency filtering |
| Snap | 0-100% | 60% | Initial transient intensity |
| Tuning | -12 to +12 st | 0 st | Resonance frequency offset |

**DSP:** Filtered noise burst with multi-trigger envelope (3-4 closely-spaced hits), bandpass resonance for body, snap controls initial attack amplitude.

### Closed Hi-Hat (4 parameters)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Level | 0-100% | 65% | Output volume |
| Tone | 0-100% | 60% | Brightness (bandpass center frequency) |
| Decay | 20-200ms | 80ms | Envelope decay time |
| Tuning | -12 to +12 st | 0 st | Resonance frequency offset |

**DSP:** Six-oscillator metallic noise (square wave ring modulation), tuned bandpass filters (metallic resonance), short decay envelope.

### Open Hi-Hat (4 parameters)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Level | 0-100% | 60% | Output volume |
| Tone | 0-100% | 60% | Brightness (bandpass center frequency) |
| Decay | 100-1000ms | 500ms | Envelope decay time |
| Tuning | -12 to +12 st | 0 st | Resonance frequency offset |

**DSP:** Same six-oscillator architecture as closed hat, longer decay envelope.

**Total Parameters:** 24

## MIDI Implementation

**Note Mapping (General MIDI standard):**
- C1 (36): Kick
- D1 (38): Clap
- F1 (41): Low Tom
- A1 (45): Mid Tom
- F#1 (42): Closed Hi-Hat
- A#1 (46): Open Hi-Hat

**Velocity Sensitivity:**
- Velocity affects output level (0-127 mapped to volume scaling)
- All voices respond to velocity for dynamic performance

**Polyphony:**
- 6 voices (one per drum sound)
- Monophonic per voice (retriggerable)
- Closed hi-hat chokes open hi-hat (authentic behavior)

## UI Concept

**Layout:** Six vertical channel strips (one per drum), each containing four knobs (Level, Tone, Decay/Snap, Tuning) arranged vertically. Global section at top with master output level. Visual trigger indicators for each voice.

**Visual Style:** Authentic 808 aesthetic—red/orange LED-style trigger indicators, brushed metal panel background, sans-serif labeling. Functional and utilitarian, not skeuomorphic recreation of hardware.

**Key Elements:**
- Per-drum trigger LEDs (light up on MIDI note-on)
- Individual output routing indicators
- Master level meter
- Preset browser (user presets only)

**Dimensions:** Approximately 1000×500px (horizontal layout, six channels)

## Use Cases

- **Hip-Hop Production:** Authentic 808 kick, clap, and hi-hats for boom-bap and trap beats
- **Electronic Music:** Classic drum sounds for house, techno, electro, and bass music
- **Live Performance:** Velocity-sensitive MIDI triggering for dynamic drum programming
- **Sound Design:** Individual outputs allow external processing (compression, saturation, reverb per drum)
- **Layering:** Blend with acoustic samples or use as foundation for hybrid drum kits

## Inspirations

- **Roland TR-808:** Original analog drum machine (circuit-level emulation)
- **AudioKit's Drum Synth:** Clean implementation of analog drum synthesis
- **Sonic Charge MicroTonic:** Flexible drum synthesis with 808-inspired voices
- **D16 Nepheton:** Detailed 808 emulation with individual outputs

## Technical Notes

**Synthesis Architecture:**
- Each voice uses dedicated synthesis method (bridged-T oscillators for toms/kick, filtered noise for clap/hats)
- All envelopes are exponential decay (authentic analog behavior)
- Closed hat choke logic requires voice state tracking (when C1 triggers, immediately release A#1)
- Individual outputs implemented via multi-output bus configuration (12 stereo outputs: main mix + 6 individual drums)

**Performance Considerations:**
- Six voices × moderate CPU per voice = medium CPU footprint
- Individual outputs increase memory bandwidth but not computation
- Velocity sensitivity requires per-voice gain scaling
- Tuning requires pitch-shifted oscillators (potentially resampling or frequency modulation)

**DSP Accuracy:**
- Bridged-T oscillator emulation requires careful tuning of resonance/damping
- Clap multi-trigger timing critical for authentic sound (3-4 hits spaced 10-30ms apart)
- Hi-hat square wave ring modulation ratios determine metallic character (ratios like 1:1.4:1.7 for inharmonic spectrum)

## Next Steps

- [ ] Create UI mockup (`/dream Drum808` → option 3)
- [ ] Start implementation (`/implement Drum808`)
