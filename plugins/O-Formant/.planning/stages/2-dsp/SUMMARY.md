# Stage 2 DSP: Phase 2.1 Core Vocal Engine - Summary

**Completed:** 2026-04-04
**Phase:** 2.1 of 3
**Result:** SUCCESS - pluginval strictness 5 passed

---

## What Was Built

9 DSP files created, 5 files modified. Full vocal synthesis signal chain:

**LFGlottalSource -> AspirationNoise mix -> FormantFilterBank (5 parallel BPF) -> ADSR envelope -> stereo output**

### New DSP Components (Source/dsp/)

| File | Purpose |
|------|---------|
| GlottalWavetable.h | Flat vector storage: 128 Rd x 2048 samples x 10 mipmap levels (~10MB shared) |
| GlottalTableGenerator.h/.cpp | Fant 1995 regression, Newton-Raphson solvers, FFT mipmap generation |
| LFGlottalSource.h | Per-voice wavetable oscillator with bilinear interpolation (Rd + mipmap) |
| AspirationNoise.h | Single-pole IIR LP at 4kHz, SmoothedValue breathiness, decorrelated seeds |
| VowelData.h | Csound bass voice formant tables (A/E/I/O/U), pre-computed linear gains |
| FormantBiquad.h | 32-byte DF2T biquad with NaN protection |
| FormantFilterBank.h | 5 parallel BPFs, semitone shift, center-of-mass spread |
| VowelMorpher.h | Shepard IDW interpolation, log-domain frequency blending |

### Modified Files

| File | Changes |
|------|---------|
| FormantVoice.h/cpp | Full DSP integration, per-sample loop, block-rate formant updates (32 samples) |
| PluginProcessor.h/cpp | Wavetable generation at construction, voice preparation via dynamic_cast |
| CMakeLists.txt | Added GlottalTableGenerator.cpp, Source/dsp include path |

## Architecture Deviation

Used **mipmapped wavetable** for glottal source instead of direct LF + PolyBLEP (user decision in discuss phase). Better runtime performance, inherent anti-aliasing, no Newton-Raphson at runtime.

## Parameters Connected (11 of 21)

vowelX, vowelY, vowelFocus, glottalRd, breathiness, attack, decay, sustain, release, formantShift, formantSpread

## Parameters Deferred to Phase 2.2/2.3

vibratoRate, vibratoDepth, vibratoDelay, consonantLevel, consonantTone, sibilance, autoConsonant, pitchGlide, outputGain, stereoWidth

## Validation

- Build: clean (4 signedness warnings in mipmap generation)
- pluginval: PASSED at strictness level 5 (all sample rates, all block sizes)
- NaN protection: FormantBiquad state reset + voice-level output guard + table generation sanitization

## Known Limitations

- Wavetable generated at plugin construction (fixed 44100 Hz reference, but mipmaps handle all sample rates)
- No parameter smoothing on vowelX/Y (block-rate updates every 32 samples is sufficient)
- Mono output to both channels (stereo spread deferred to Phase 2.3)
