# Stage 2: DSP — Context

**Plugin:** O-Gain
**Stage:** 2 (DSP Implementation)
**Mode:** Express (auto-generated)

## What This Stage Must Produce

Complete audio processing implementation in PluginProcessor.cpp including:

### Phase 1: Core Processing
- Gain stage: apply gain_offset + trim as smoothed gain multiplication
- Channel utilities: phase inversion (L/R), channel swap, mono sum, M/S encode/decode
- Basic metering: peak detection (input/output), RMS calculation
- Atomic float metering values for UI display
- Signal chain: Input -> Utilities -> Metering (input) -> Gain -> Metering (output) -> Output

### Phase 2: Learn Mode + LUFS Measurement
- K-weighting filters: pre-filter (high-shelf) + RLB (high-pass) using juce::dsp::IIR::Filter<double>
- LUFS gating block accumulator: 400ms blocks with 100ms hop
- EBU R128 dual-gate: absolute gate (-70 LUFS) + relative gate (-10 LU)
- True peak detection (digital peak for MVP)
- VU meter ballistics: juce::dsp::BallisticsFilter<float> (300ms attack/release)
- Learn state machine: idle/learning/complete with atomic flag
- Gain calculation: target - measured, clamped, true peak safety check
- Confidence indicator: based on duration and block count
- Momentary/short-term LUFS for display during Learn

## Parameters (already wired in APVTS)

| ID | Type | Range | Default |
|----|------|-------|---------|
| gain_offset | Float | -40 to +40 | 0 |
| trim | Float | -6 to +6 | 0 |
| target_level | Float | -36 to 0 | -18 |
| measurement_mode | Choice | LUFS/RMS | LUFS |
| meter_mode | Choice | Peak/RMS/VU/LUFS | VU |
| phase_invert_l | Bool | - | false |
| phase_invert_r | Bool | - | false |
| channel_swap | Bool | - | false |
| mono_sum | Bool | - | false |
| ms_mode | Choice | Off/Encode/Decode | Off |

## Technical Constraints

- Zero latency (0 samples)
- No allocations on audio thread
- No mutexes on audio thread
- Use juce::ScopedNoDenormals in processBlock
- Double precision for measurement path
- Single precision for gain/utilities
- Pre-allocate block vectors in prepareToPlay
- Metering via std::atomic<float> (audio writes, UI reads)
- Learn active via std::atomic<bool> (UI writes, audio reads)
- VU ballistics: 300ms attack/release (ANSI standard)
- K-weight coefficients: published for 48kHz, need bilinear transform for other rates
- Common rates to support: 44100, 48000, 88200, 96000

## Signal Flow

```
Input → [Phase Invert] → [Channel Swap] → [M/S Encode] → [Mono Sum]
     → [Learn Measurement (pre-gain, K-weighted)] (only during Learn)
     → [Apply Gain (gain_offset + trim)]
     → [Output Metering]
     → Output
```

## References

- Architecture: plugins/O-Gain/.planning/research/ARCHITECTURE.md
- BRIEF: plugins/O-Gain/.planning/BRIEF.md
- Roadmap: plugins/O-Gain/.planning/ROADMAP.md
- Existing code: plugins/O-Gain/Source/PluginProcessor.cpp
