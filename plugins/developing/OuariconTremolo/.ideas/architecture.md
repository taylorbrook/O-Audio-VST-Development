# Ouaricon Tremolo - DSP Architecture

## Signal Flow

```
Input (Stereo)
    ↓
[LFO Generator] ← Waveform Selection
    ↓           ← Speed Control
    ↓           ← Smoothing Filter
[Modulation]    ← Pan Sync (L/R phase offset)
    ↓
[Gain Stage]    ← Depth Control
    ↓
Output (Stereo)
```

## Core Components

### 1. LFO (Low-Frequency Oscillator)

**Purpose**: Generate modulation waveform at specified frequency

**Implementation Strategy**:
- Phase accumulator design (0.0 to 1.0 per cycle)
- Phase increment calculated from speed parameter and sample rate
- Support for tempo sync via host BPM and time signature

**Phase Update**:
```
phaseIncrement = speed_Hz / sampleRate
phase += phaseIncrement
if (phase >= 1.0) phase -= 1.0
```

**Waveform Generation**:
Each waveform type uses the phase value (0.0-1.0) to generate output (-1.0 to +1.0):

- **Sine**: `sin(phase * 2π)`
- **Triangle**: Piecewise linear (0→1→0→-1→0)
- **Phasor**: Linear ramp (0→1, reset)
- **Square**: `phase < 0.5 ? 1.0 : -1.0`
- **Pulse**: `phase < 0.2 ? 1.0 : -1.0` (20% duty cycle)
- **Noise**: `random(-1.0, +1.0)` sampled at LFO rate

### 2. Smoothing Filter

**Purpose**: Soften waveform transitions via interpolation

**Implementation Strategy**:
- One-pole lowpass filter applied to LFO output
- Smoothing parameter (0-100%) maps to filter coefficient
- Higher smoothing = lower cutoff frequency = smoother output

**Algorithm**:
```
coefficient = 1.0 - (smoothing / 100.0) * 0.99
smoothedLFO = prevLFO + (rawLFO - prevLFO) * coefficient
prevLFO = smoothedLFO
```

**Behavior**:
- 0% smoothing: coefficient ≈ 1.0 (no filtering, instant response)
- 100% smoothing: coefficient ≈ 0.01 (heavy filtering, very smooth)
- Applied per-sample for continuous smoothing

### 3. Pan Sync Logic

**Purpose**: Create stereo width modulation with phase-inverted LFO

**Implementation Strategy**:
When Pan Sync is OFF:
- Left channel LFO phase = main phase
- Right channel LFO phase = main phase
- (Identical modulation, mono tremolo)

When Pan Sync is ON:
- Left channel LFO phase = main phase
- Right channel LFO phase = (main phase + 0.5) mod 1.0
- (180° phase offset, creates auto-pan effect)

**Stereo Image**:
- As left channel amplitude increases, right decreases (and vice versa)
- Creates perceived stereo movement
- Most effective with sine/triangle waveforms (smooth panning)

### 4. Depth Control & Gain Modulation

**Purpose**: Scale LFO output and apply amplitude modulation

**Modulation Calculation**:
```
lfoValue = (smoothedLFO + 1.0) / 2.0  // Convert -1..+1 to 0..1
depthScaled = depth / 100.0           // 0..100% to 0..1
gainMultiplier = 1.0 - (lfoValue * depthScaled)
```

**Gain Range**:
- Depth = 0%: gainMultiplier = 1.0 (no modulation)
- Depth = 100%, LFO = max: gainMultiplier = 0.0 (full attenuation)
- Depth = 100%, LFO = min: gainMultiplier = 1.0 (unity gain)
- Depth = 50%, LFO = max: gainMultiplier = 0.5 (half attenuation)

**Application**:
```
outputSample = inputSample * gainMultiplier
```

Applied independently to left and right channels (using their respective LFO phases).

### 5. Tempo Sync System

**Purpose**: Lock LFO speed to DAW tempo and musical divisions

**Implementation Strategy**:
- Query host for BPM and time signature via `juce::AudioPlayHead`
- Convert note division to Hz based on current tempo
- Update LFO phase increment accordingly

**Note Division to Hz**:
```
beatsPerSecond = BPM / 60.0
cycleDuration = (noteDivision * 4.0) / beatsPerSecond
frequency_Hz = 1.0 / cycleDuration
```

**Note Division Examples** (at 120 BPM):
- 1/1 (whole note): 0.5 Hz (2-second cycle)
- 1/2 (half note): 1.0 Hz
- 1/4 (quarter note): 2.0 Hz
- 1/8 (eighth note): 4.0 Hz
- 1/16 (sixteenth note): 8.0 Hz

**Sync Behavior**:
- When tempo sync OFF: Use speed parameter directly (Hz)
- When tempo sync ON: Quantize speed to nearest note division, calculate Hz from BPM
- Follow DAW transport tempo changes in real-time

## Processing Pipeline (per sample)

### Mono Tremolo (Pan Sync OFF)
```
1. Update LFO phase (based on speed/tempo)
2. Generate raw waveform value from phase
3. Apply smoothing filter
4. Calculate gain multiplier from LFO + depth
5. Multiply input samples by gain:
   - outputL = inputL * gain
   - outputR = inputR * gain
```

### Stereo Tremolo (Pan Sync ON)
```
1. Update main LFO phase
2. Calculate L/R phases (R = L + 0.5)
3. Generate raw waveform values for both channels
4. Apply smoothing filter to both
5. Calculate separate gain multipliers (L and R)
6. Multiply input samples by respective gains:
   - outputL = inputL * gainL
   - outputR = inputR * gainR
```

## Performance Considerations

### CPU Optimization
- **Waveform lookup**: Pre-calculate sine for efficiency (or use fast approximation)
- **Branch prediction**: Minimize conditionals in inner loop
- **SIMD**: Consider vectorization for multi-channel processing (if needed for many instances)
- **Parameter smoothing**: Use AudioParameterFloat with smoothing enabled to avoid zipper noise

### Latency
- **Zero algorithmic latency**: Effect is sample-accurate amplitude modulation
- **Smoothing filter**: Minimal phase shift (one-pole IIR, negligible delay)
- **Real-time safe**: No allocations in audio thread, no blocking operations

### Sample Rate Independence
- **Phase increment scaling**: Automatically adjusts to sample rate changes
- **Smoothing coefficient**: May need tuning across different sample rates for consistent behavior
- **Tested range**: 44.1kHz - 192kHz

## Parameter Interaction Matrix

| Parameter Change | Affects                    | Update Frequency |
|------------------|----------------------------|------------------|
| Speed            | Phase increment            | Per-buffer       |
| Depth            | Gain multiplier scaling    | Per-sample       |
| Waveform         | LFO calculation function   | Per-sample       |
| Smoothing        | Filter coefficient         | Per-buffer       |
| Pan Sync         | Right channel phase offset | Per-sample       |
| Tempo Sync       | Speed interpretation       | Per-buffer       |

## Boundary Conditions & Edge Cases

### Phase Wraparound
- Ensure phase stays in [0.0, 1.0) range
- Handle potential floating-point drift over long runtime

### Parameter Limits
- Speed: Clamp to [0.1, 20.0] Hz (avoid DC or aliasing)
- Depth: Clamp to [0, 100]% (no negative gain, no overdrive)
- Smoothing: Clamp to [0, 100]% (stable filter coefficients)

### State Continuity
- **Preset changes**: Avoid audible clicks when changing waveforms
- **Automation**: Smooth parameter updates (use JUCE parameter smoothing)
- **Transport start/stop**: Maintain LFO phase or reset based on UX decision

### Stereo Considerations
- Ensure left/right gain multipliers never both hit zero simultaneously (unless depth = 100%)
- Pan Sync at 100% depth creates extreme stereo separation - document as creative tool

## Future Enhancements (Optional)

- **Additional waveforms**: Custom curves, sample & hold
- **Modulation depth curves**: Exponential vs linear scaling
- **Phase offset control**: User-adjustable stereo phase relationship
- **Sidechain trigger**: External audio to trigger/reset LFO phase
- **Multiple LFO rates**: Polyrhythmic modulation
- **Waveform morphing**: Crossfade between shapes

## References

- **Classic tremolo circuits**: Fender-style bias tremolo, optical tremolo (Magnatone)
- **Digital implementations**: Robert Bristow-Johnson's Audio EQ Cookbook (filter design)
- **JUCE documentation**: AudioParameterFloat, AudioProcessorValueTreeState, dsp::Oscillator
