# O-MultiBandCompressor Notes

## Status
- **Current Status:** 🚧 Stage 0 (Research & Planning Complete)
- **Version:** 1.0.0 (planned)
- **Type:** Audio Effect (Dynamics Processor - Multiband Compressor)
- **Complexity:** 5.0 (Maximum complexity - 57 parameters, 10 DSP components)

## Lifecycle Timeline

- **2026-01-25 (Ideation):** Creative brief created - 4-band multiband compressor with Linkwitz-Riley crossovers, M/S processing, sidechain filtering, and real-time FFT visualization
- **2026-01-25 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 5.0, phased implementation)

## Known Issues

None (not yet implemented)

## Additional Notes

### Description
Professional 4-band multiband compressor designed for mixing and mastering workflows. Features Linkwitz-Riley 24dB/oct crossovers, per-band sidechain filtering, Peak/RMS blend detection, Mid/Side processing, auto-makeup gain, and real-time spectrum analyzer with gain reduction metering - all wrapped in the Botanical/Ouaricon aesthetic.

### Key Features
- **4-band crossover:** Linkwitz-Riley 4th order (24 dB/octave) at 200Hz, 2kHz, 8kHz (adjustable)
- **Per-band compression:** Threshold, Ratio (1:1 to 20:1), Attack (0.1-200ms), Release (10-2000ms), Knee (0-24dB)
- **Detection modes:** Continuous Peak/RMS blend (0-100%) per band
- **Sidechain filtering:** HPF and LPF per band (frequency-selective compression)
- **Mid/Side processing:** Off/Mid/Side/Both modes (up to 8 compressors in Both mode)
- **Auto-makeup gain:** Automatic gain compensation with slow ballistics (prevents pumping)
- **Parallel compression:** Global dry/wet mix (New York compression technique)
- **Visualization:** Real-time FFT spectrum analyzer with band overlays and per-band GR meters

### Parameters (57 total)
**Global (9 parameters):**
- Input Gain, Output Gain, Mix (Dry/Wet), Auto-Makeup, M/S Mode
- Crossover 1 (20-500Hz), Crossover 2 (200-5kHz), Crossover 3 (2-16kHz)

**Per-Band (12 parameters × 4 bands = 48 parameters):**
- Threshold, Ratio, Attack, Release, Knee, Makeup Gain, Peak/RMS Blend
- Solo, Bypass, SC HPF, SC LPF, SC Listen

### DSP Architecture
- **Crossover:** Cascaded 2nd order Butterworth filters (Linkwitz-Riley 4th order)
- **Compressor:** Custom feed-forward topology with soft knee and Peak/RMS blend
- **M/S Encoding:** Power-preserving matrix (√2 scaling)
- **FFT Analysis:** 2048 samples, Hann window, 30-60fps updates (separate thread)
- **Latency:** ~10-12ms (IIR filters + attack lookahead)
- **CPU Target:** <30% single core @ 48kHz stereo (Off mode), <50% (Both mode)

### GUI
- WebView-based UI with Botanical/Ouaricon aesthetic
- Real-time spectrum analyzer (20Hz-20kHz, -80dB to 0dB)
- Draggable crossover handles on spectrum display
- Per-band gain reduction meters (vertical bars, 0 to -24dB)
- Input/output meters (stereo peak + RMS)
- 4-column layout (one per band) with all compression controls

### Implementation Strategy
**Phased implementation** (Complexity score 5.0):
1. **Phase 4.1:** Single-band compressor foundation (all features on single band)
2. **Phase 4.2:** Linkwitz-Riley crossover + 4-band architecture
3. **Phase 4.3:** Sidechain filtering + M/S processing + Auto-makeup + Dry/Wet
4. **Phase 5.1:** WebView layout + Spectrum analyzer + Crossover handles
5. **Phase 5.2:** Parameter binding (57 parameters, two-way communication)
6. **Phase 5.3:** GR meters + FFT visualization threading

### Risk Assessment
- **MEDIUM Risk:** Linkwitz-Riley coefficient smoothing (clicks on crossover frequency changes)
- **MEDIUM Risk:** FFT thread communication (lock-free audio → FFT → UI pipeline)
- **MEDIUM Risk:** Performance optimization (25-35% CPU target, may need SIMD)
- **LOW Risk:** M/S encoding/decoding (simple matrix math)
- **LOW Risk:** Soft knee gain computer (well-documented quadratic formula)

### Validation
- **Formats:** VST3, AU
- **Target Sample Rates:** 44.1kHz to 192kHz
- **Target DAWs:** Logic Pro, Ableton Live, FL Studio, Reaper
- **Expected Duration:** 18-28 hours (highly complex plugin)

### Contracts
- Creative Brief: `plugins/O-MultiBandCompressor/.ideas/creative-brief.md`
- Architecture: `plugins/O-MultiBandCompressor/.contracts/architecture.md`
- Implementation Plan: `plugins/O-MultiBandCompressor/.contracts/plan.md`

---

*Last updated: 2026-01-25*
*Next: Stage 1 (Foundation + Shell) - Run `/implement O-MultiBandCompressor`*
