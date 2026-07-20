# TapeAge - Implementation Plan

**Date:** 2025-11-10
**Complexity Score:** 5.0 (Complex)
**Strategy:** Phase-based implementation

## Complexity Breakdown

```
Complexity Calculation:
- Parameters: 3 parameters (0.6 points, capped at 2.0) = 0.6
- Algorithms: 6 DSP components = 6
- Features: Modulation systems (LFO) = 1
Total: 7.6 / 5.0 → capped at 5.0
```

## Complexity Factors

- **Parameters:** 3 (0.6 points)
- **Algorithms:** 6 (6 points)
  - Oversampling Engine (juce::dsp::Oversampling)
  - Saturation Waveshaper (juce::dsp::WaveShaper with custom tanh)
  - Wow/Flutter Modulator (juce::DelayLine with LFO)
  - Dropout Generator (custom random attenuation)
  - Tape Noise Generator (juce::Random with filtering)
  - Dry/Wet Mixer (juce::dsp::DryWetMixer)
- **Features:** Modulation systems (LFO for wow/flutter) (1 point)

## Stages

- Stage 0: Research ✓
- Stage 1: Planning ← Current
- Stage 2: Foundation
- Stage 3: Shell
- Stage 4: DSP (phased)
- Stage 5: GUI (phased)
- Stage 6: Validation

## Stage 4: DSP Phases

### Phase 4.1: Core Saturation Processing

**Goal:** Implement oversampling and saturation waveshaper—the foundation of tape character

**Components:**
- Oversampling engine (2x, FIR filters)
- Saturation waveshaper with tanh-based transfer function
- Drive parameter mapping (progressive curve: subtle → moderate → heavy)

**Test Criteria:**
- [ ] Saturation produces warm harmonics at 50% drive
- [ ] No aliasing artifacts at high drive settings
- [ ] Smooth parameter transitions (no clicks/pops)
- [ ] DC offset remains at zero

**Duration:** 45 min

### Phase 4.2: Wow/Flutter Modulation

**Goal:** Implement pitch modulation for authentic tape speed variations

**Components:**
- Delay line (200ms buffer with Lagrange3rd interpolation)
- LFO system (0.5-2Hz wow, optional flutter component)
- Age parameter mapping to modulation depth

**Test Criteria:**
- [ ] Pitch wobble audible at max age (±10 cents)
- [ ] Smooth, musical warble (no clicks or artifacts)
- [ ] Stereo width via random phase offset per channel
- [ ] No audible warble at 0% age

**Duration:** 60 min

### Phase 4.3: Degradation Features (Dropout + Noise)

**Goal:** Add dropout and tape noise for complete vintage character

**Components:**
- Dropout generator (random events with smooth envelope)
- Tape noise generator (filtered white noise at subtle amplitude)
- Age parameter mapping to both components

**Test Criteria:**
- [ ] Dropouts occur rarely at max age (5-10 second intervals)
- [ ] Dropout envelope prevents clicks (smooth attack/release)
- [ ] Tape noise remains VERY subtle even at 100% age
- [ ] No dropouts or noise at 0% age

**Duration:** 30 min

### Phase 4.4: Dry/Wet Mixing

**Goal:** Add equal-power dry/wet blend for parallel processing flexibility

**Components:**
- Dry/Wet mixer (juce::dsp::DryWetMixer)
- Mix parameter mapping (0% = dry, 100% = wet)
- Latency compensation if needed

**Test Criteria:**
- [ ] No volume drop at 50% mix (equal-power curve)
- [ ] Fully dry at 0% (bypass)
- [ ] Fully wet at 100% (full processing)
- [ ] Smooth transitions across mix range

**Duration:** 15 min

## Stage 5: GUI Phases

### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate WebView UI with three rotary knobs and parameter bindings

**Components:**
- Load v3 mockup HTML/CSS/JS from `.ideas/mockups/`
- WebView integration (JUCE WebBrowserComponent)
- JavaScript ↔ C++ bridge for parameter communication
- Bind drive, age, mix parameters to knobs

**Test Criteria:**
- [ ] WebView loads and displays v3 mockup
- [ ] All three knobs visible and positioned correctly
- [ ] Knob dragging updates parameters in real-time
- [ ] Parameter changes from DAW update knob positions

**Duration:** 40 min

### Phase 5.2: VU Meter Visualization

**Goal:** Implement animated VU meter showing output level

**Components:**
- Peak metering in processBlock (track output level)
- JavaScript callback to update VU meter needle position
- Smooth animation for authentic vintage needle movement

**Test Criteria:**
- [ ] VU meter needle responds to audio output level
- [ ] Smooth needle animation (no jitter)
- [ ] Meter reflects post-processing output (after mix)
- [ ] Meter reads accurately (peak detection)

**Duration:** 30 min

## Estimated Duration

**Total: ~4.0 hours**

- Stage 2: Foundation - 10 min
- Stage 3: Shell - 5 min
- Stage 4: DSP (phased) - 150 min (2.5 hours)
  - Phase 4.1: Core Saturation - 45 min
  - Phase 4.2: Wow/Flutter - 60 min
  - Phase 4.3: Degradation Features - 30 min
  - Phase 4.4: Dry/Wet Mixing - 15 min
- Stage 5: GUI (phased) - 70 min (1.2 hours)
  - Phase 5.1: Layout and Basic Controls - 40 min
  - Phase 5.2: VU Meter - 30 min
- Stage 6: Validation - 20 min

## Notes

- Complexity score is maximum (5.0) due to 6 distinct DSP components
- Phased implementation ensures each component is tested before proceeding
- Age parameter controls 3 components: wow/flutter, dropout, noise—test each independently
- VU meter is visualization only (not a parameter)—can be implemented last if needed
- Critical constraint: Tape noise must remain VERY subtle at all settings (primary character from saturation + wow/flutter)
