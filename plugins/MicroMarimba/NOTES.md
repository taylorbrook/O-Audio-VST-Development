# MicroMarimba Notes

## Status
- **Current Status:** 🚧 Stage 0
- **Version:** 1.0.0 (planned)
- **Type:** Synth (Physical Model)
- **Complexity:** 5.0 (VERY HIGH - maximum complexity)

## Lifecycle Timeline

- **2026-01-09 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 5.0)
  - DSP architecture: Modal synthesis (8 modes), convolution body resonance, tuning engine
  - Tuning systems: MTS-ESP > Scala > 12-TET priority fallback
  - Implementation strategy: Phased (4 DSP phases, 2 GUI phases)
  - Professional research: Applied Acoustics Chromaphone, Modartt Pianoteq, Surge XT
  - JUCE modules: IIR::Filter, Convolution, Synthesiser
  - External libraries: Surge Tuning Library, MTS-ESP client

## Known Issues

None (Stage 0 - not yet implemented)

## Additional Notes

### Description
Physically modeled marimba synthesizer with native microtonal support using modal synthesis, convolution body resonance, and comprehensive tuning system integration.

### Key Features
- **Modal Synthesis:** 8 resonant modes per voice with inharmonic overtones (authentic marimba character)
- **Microtonality:** Native support for Scala files, MTS-ESP, and 12-TET with priority fallback
- **Body Resonance:** Convolution IR for resonator tube coupling (~50-100ms)
- **Voice Management:** 16-24 polyphony target with <1% CPU per voice
- **Velocity Response:** Custom curve mapping for expressive performance

### Parameters (7 total)
1. **MALLET_HARDNESS** (Float, 0.0-1.0) - Excitation brightness (soft/dark to hard/bright)
2. **BAR_MATERIAL** (Float, 0.0-1.0) - Spectral balance (rosewood to synthetic)
3. **RESONANCE** (Float, 0.0-1.0) - Decay time + body IR mix
4. **TUNING_MODE** (Choice, 0-2) - 12-TET / Scala / MTS-ESP
5. **REFERENCE_PITCH** (Float, 400-480 Hz) - A4 reference frequency
6. **VEL_CURVE** (Float, 0.0-1.0) - Velocity curve (linear to exponential)
7. **OUTPUT_GAIN** (Float, -24 to +12 dB) - Master output level

### DSP Architecture
- **Modal Resonator Bank:** 8 parallel 2nd-order IIR biquad filters per voice
  - Mode ratios: 1.00, 3.93, 9.24, 16.65, 26.3, 38.2, 52.4, 68.9 (inharmonic)
  - Coefficients: `θ = 2π*f/sr`, `r = exp(-1/(decay*sr))`, `g = amp*(1-r)`
- **Mallet Exciter:** Filtered noise burst (5-20ms) with velocity-dependent brightness
- **Body Resonance:** juce::dsp::Convolution with short IR (50-100ms)
- **Tuning Engine:** Three-tier system (MTS-ESP > Scala > 12-TET)
  - Surge Tuning Library for Scala/KBM parsing
  - MTS-ESP client for real-time DAW-wide tuning
  - Nearest-pitch mapping for unmapped notes

### Implementation Phases
- **Phase 2.1:** Basic synthesizer (12-TET, sine wave) - LOW risk, 1-2h
- **Phase 2.2:** Modal synthesis (8 modes, marimba timbre) - MEDIUM risk, 2-3h
- **Phase 2.3:** Tuning engine (Scala, MTS-ESP) - MEDIUM risk, 2-3h
- **Phase 2.4:** Body resonance (convolution) - LOW risk, 1-2h
- **Phase 3.1:** Parameter UI + tuning indicator - LOW risk, 2-3h
- **Phase 3.2:** Visual polish + feedback - LOW risk, 2-3h
- **Total estimated:** 14-22 hours

### Performance Targets
- **Per-voice CPU:** <1% (modal synthesis ~0.5%, exciter ~0.05%)
- **Shared overhead:** <10% (convolution body resonance)
- **Total CPU (24 voices):** <25% single core @ 48kHz
- **Latency:** ~50-100ms (convolution IR length, reported to host)
- **Memory:** <1KB per voice, ~50KB convolution IR

### Research Resources
- Physical modelling guide: Section 1.4 (modal synthesis with biquad coefficients)
- Microtonality implementation: Complete tuning table and Scala parser code
- Microtonality theory: Scala/KBM format specifications
- Professional references: Chromaphone 3, Pianoteq, Surge XT

### Risk Mitigation
- **Modal synthesis stability:** Reference implementation, denormal protection, coefficient clamping
- **Tuning engine integration:** Surge library (vendored), MTS-ESP client (vendored), atomic table updates
- **CPU performance:** Profile early, optimize biquad, consider SIMD for modal bank
- **Fallback architectures:** Reduce modes (6 vs 8), disable convolution, Karplus-Strong alternative

### External Dependencies
- **Surge Tuning Library:** Header-only, GPL-3.0, vendor in Source/tuning/
- **MTS-ESP Client:** Source files, MIT license, vendor in Source/mts-esp/
- **Marimba Body IR:** ~50-100ms WAV, embed in BinaryData

### Synthesizer Configuration
- **IS_SYNTH:** TRUE
- **NEEDS_MIDI_INPUT:** TRUE
- **Audio Input:** None (MIDI-only instrument)
- **Audio Output:** Stereo (no multi-output routing)
- **Voice Allocation:** 16-24 voices with oldest-first stealing

### Next Steps
1. Stage 1: Foundation + Shell (create build system and parameters)
2. Review architecture.md and plan.md
3. Execute phased DSP implementation (Stages 2.1-2.4)
4. Execute GUI implementation (Stages 3.1-3.2)
5. Testing & release prep (Stage 4)

**Last Updated:** 2026-01-09
