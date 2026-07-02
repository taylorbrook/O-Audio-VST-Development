# Changelog

All notable changes to DrumRoulette will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0] - 2025-11-12

### Added

- Initial release
- Eight-slot drum sampler with folder-based randomization
- Global randomize button with per-slot lock controls
- Individual slot randomization buttons
- Mixer-style vertical channel strip interface (1400×950px)

### Parameters (73 total)

**Global Controls:**
- Randomize All button (randomizes all unlocked slots)

**Per-Slot Controls (×8):**
- Folder Path (file browser for sample folder selection)
- Randomize button (pick random sample from folder)
- Lock toggle (exclude from global randomization)
- Volume fader (-60dB to +6dB, default 0dB)
- Decay slider (10ms-2000ms, default 500ms, sustain fixed at 0)
- Attack slider (0-50ms, default 1ms)
- Tilt Filter (-12dB to +12dB, pivot at 1kHz)
- Pitch shift (±12 semitones)
- Solo toggle (isolate channel on main mix)
- Mute toggle (silence channel on main mix)

### Audio Processing

**Sample Playback:**
- Multi-format support (WAV, AIFF, MP3, AAC)
- Recursive folder scanning (includes subfolders)
- True random sample selection (no shuffle/cycle behavior)
- MIDI note mapping: C1-G1 (chromatic, 36-43)
- Velocity-sensitive playback with linear interpolation

**DSP Chain (per voice):**
- ADSR envelope (sustain=0, release=50ms fixed)
- Variable-rate pitch shifting (±12 semitones via pow(2.0, semitones/12.0))
- Cascaded tilt filter (low-shelf + high-shelf at 1kHz pivot)
- Per-channel volume control with denormal protection

**Audio Routing:**
- 18 total outputs (2 main + 16 individual)
- Stereo main output (mix of all channels, respects solo/mute)
- 8 individual stereo outputs (always active, ignore solo/mute)
- Multi-output bus configuration: Slot N → Out (2N-1, 2N)

**Voice Architecture:**
- 8-voice polyphonic synthesis via juce::Synthesiser
- Custom SynthesiserVoice subclass (DrumRouletteVoice)
- Envelope-driven voice lifecycle (auto-stops when envelope finishes)
- Real-time safe processing (no allocations in audio thread)

### User Interface

- WebView-based UI with JUCE integration
- Eight vertical channel strips with mixer-style layout
- Vintage hardware mixer aesthetic (brushed metal, rack-mount styling)
- Real-time parameter bindings via WebSliderParameterAttachment
- 64 parameter attachments (8 slots × 8 parameters)
- Toggle button relays for Solo/Mute/Lock controls

### Validation

- Compiles successfully (VST3, AU, Standalone)
- 5 factory presets included:
  - 01-Default Kit (balanced starting point)
  - 02-Tight Drums (short decays, punchy)
  - 03-Dark and Deep (long decays, negative tilt, low pitch)
  - 04-Bright and Crispy (positive tilt, high pitch)
  - 05-Lo-Fi Character (extreme pitch/decay variations)
- Zero build errors
- Binary size: ~5.5MB (Release)

### Technical Details

**Build System:**
- CMake 4.1.1 + Ninja 1.13.1
- JUCE 8.x framework
- Multi-target compilation (VST3, AU, Standalone)
- WebView resources via binary data embedding

**Platform:**
- macOS (Apple Silicon + Intel via Universal Binary)
- Xcode 16.4 toolchain
- C++17 standard

**Development Approach:**
- Phased implementation (4 DSP phases + 3 GUI phases)
- Contract-driven design (architecture.md, parameter-spec.md, plan.md)
- Critical pattern enforcement (juce8-critical-patterns.md)
- Incremental testing at each phase
