# Stage 2: DSP Phase 3.5 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User (Taylor Brook), Claude
**Phase:** Oversampling + Tuning + MPE + Bow Noise + Optimization

## Requirements Confirmed

- 2x oversampling per-voice using `juce::dsp::Oversampling<float>` wrapping friction+waveguide inner loop
- MPE per-note expression: pitch bend, aftertouch, slide, expression — all modulate knob base values
- TuningEngine integration following O-Lyrica pattern exactly — full Scala/TUN support, visualization, editable table
- Bow noise generator: bandpass noise (2-6 kHz), post-body additive mix, 1-5% of signal
- Reference pitch wired to both voice (via TuningEngine) and DroneStringEngine
- CPU profiling and optimization pass
- Connect remaining 3 parameters: referencePitch, tuningSystem, bowNoise
- Tooltips must document MPE modulation behavior once GUI stage arrives

## Constraints Identified

- 8 oversampler instances (one per voice) — acceptable, each is lightweight (mono, 2x, halfband)
- Oversampling latency must be reported via `setLatencySamples()` in prepareToPlay
- Internal sample rate for friction+waveguide = hostSampleRate * 2
- WaveguideString delay sizes must be recalculated for internal (2x) sample rate
- MPE must NOT override knob values — knobs are the base, MPE modulates/scales them
- Tuning engine is processor-level, shared by all voices via pointer (same as O-Lyrica)
- Bow noise is NOT in waveguide feedback loop — additive post-body only
- Drone strings are NOT oversampled (core tier only, run at native rate)
- ScopedNoDenormals already in processBlock; verify it covers oversampled path

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Oversampling scope | Per-voice (A) | Architecture specifies friction+waveguide only; processor-level would oversample body/sympathetic unnecessarily |
| MPE behavior | Knob = base, MPE = modulator | Preserves knob as starting point; aftertouch scales pressure, slide offsets position, expression scales speed |
| Bow noise placement | Post-body additive | Architecture spec: outside waveguide loop, 1-5% of signal, bandpass 2-6 kHz |
| Tuning integration | O-Lyrica pattern exactly | Proven: setTuningEngine() on voice, getFrequency() in startNote, full Scala/visualization/editable table |
| Optimization pass | Yes, included | Profile CPU, denormal checks, gating optimizations |

## Architecture: Per-Voice Oversampling

### Implementation Plan
- Each `BowedStringVoice` owns a `juce::dsp::Oversampling<float>(1, 1, ...)` (mono, 2x, halfband)
- In `prepareToPlay()`: call `oversampling.initProcessing(maxBlockSize)` and `setLatencySamples()`
- In `renderNextBlock()`: upsample input block → run friction+waveguide at 2x → downsample output
- WaveguideString must accept `internalSampleRate` (2x host rate) for delay line sizing
- Bridge filter coefficients recalculated at internal sample rate

### Latency Reporting
- `oversampling.getLatencyInSamples()` — typically ~8 samples for 2x halfband
- Report in processor's `prepareToPlay()` via `setLatencySamples()`
- All voices share same oversampling config → same latency

## Architecture: MPE Modulation

### Mapping (knob = base, MPE = modulator)

| MPE Dimension | Target | Modulation Type | Range |
|---------------|--------|----------------|-------|
| Pitch Bend | Frequency | Semitone offset (±48 semitones MPE default) | Smooth portamento via delay interpolation |
| Aftertouch (Z) | Bow Pressure | Multiply: base × (0.5 + 1.5 × aftertouch) | 0.5x at rest, 2.0x at max |
| CC74 (Slide/Y) | Bow Position | Offset: base + range × (slide - 0.5) | Centered at midpoint, ±range |
| CC11 (Expression) | Bow Speed | Multiply: base × expression | 0x at min, 1x at max (default 127 = 1.0) |

### Implementation
- BowedStringVoice stores per-voice MPE state: `mpePitchBend`, `mpeAftertouch`, `mpeSlide`, `mpeExpression`
- `pitchWheelMoved()`: update `mpePitchBend`, recalculate frequency with tuning engine
- `controllerMoved()`: dispatch CC74 → slide, CC11 → expression; aftertouch via `aftertouchChanged()` if available or channel pressure
- `updateParametersFromAPVTS()`: apply MPE modulation on top of knob base values each block

### Tooltip Note (for Stage 3 GUI)
- All bow parameters show "Base value — modulated by MPE [dimension]" in tooltip
- e.g., Bow Pressure tooltip: "Base bow pressure. Modulated by MPE aftertouch (Z-axis)."

## Architecture: Tuning Engine Integration

### Pattern (from O-Lyrica)
1. Processor owns `TuningEngine tuningEngine` (already done)
2. Processor calls `voice->setTuningEngine(&tuningEngine)` during voice setup
3. Voice `startNote()`: `currentFrequency = tuningEngine->getFrequency(midiNoteNumber)`
4. `referencePitch` parameter → `tuningEngine.setReferenceFrequency(refPitch)` each block
5. `tuningSystem` parameter → `tuningEngine.setTuningSystem()` on change (message thread)
6. Full Scala/TUN file loading, visualization support, editable tuning table
7. DroneStringEngine also uses tuningEngine for reference pitch

### Parameters Wired
- `tuningSystem` → TuningEngine system selection (Scala/TUN, MTS-ESP, 12TET)
- `referencePitch` → TuningEngine reference frequency + DroneStringEngine

## Architecture: Bow Noise Generator

### Design
- New class: `Source/DSP/BowNoiseGenerator.h` (header-only)
- White noise via `juce::Random` → bandpass filter (2-6 kHz center, `juce::dsp::IIR::Filter`)
- Amplitude: `bowPressure * bowSpeed * noiseAmount * 0.05f` (1-5% of signal)
- Mixed into output post-body resonator, pre-stereo width (additive)
- Per-voice (each voice generates its own noise for proper stereo imaging)
- `bowNoise` parameter (0-100%) controls dry→wet noise blend

### Signal Flow Position
```
BodyResonator → + BowNoiseGenerator → SympatheticStringEngine → StereoWidth → Output
```

## Architecture: Optimization Pass

### Targets
- Profile CPU: 2 strings core tier < 6%, max config (4 active + 12 sympathetic + quality tier + 2x OS) < 25%
- Verify `ScopedNoDenormals` covers oversampled path
- Clamp small values in bristle displacement (z) and temperature (T_contact)
- Confirm energy gating on sympathetic strings works correctly with oversampled voice output
- Check for unnecessary per-sample divisions (convert to multiplications)
- Verify SmoothedValue targets are appropriate for block-rate updates

## Parameters Status After Phase 3.5

**All 23 connected (23/23):**
bowSpeed, bowPressure, bowPosition, rosin, brightness, infiniteSustain, outputLevel, bodyMaterial, bodySize, width, stringCount, stringTuning1-4, sympatheticAmount, sympatheticCount, frictionTier, reversedFriction, subHarmonics, referencePitch, tuningSystem, bowNoise

## Open Questions (for research phase)

- Exact oversampling filter type: `filterHalfBandPolyphaseIIR` vs `filterHalfBandFIR` — which has lower latency?
- Per-voice oversampling block processing: process entire renderNextBlock at 2x, or per-sample 2x loop?
- MPE zone configuration: single lower zone (JUCE default) or configurable?
- Bow noise filter: single bandpass or cascade of highpass + lowpass for more control?
- TuningEngine thread safety: does setTuningSystem() need message-thread-only guard?

## Next Phase

Ready for: research phase
